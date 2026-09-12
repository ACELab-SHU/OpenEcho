#include "venus/VenusDagScheduler.hh"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iterator>
#include <memory>
#include <numeric>
#include <sstream>

#include "base/logging.hh"
#include "arch/generic/mmu.hh"
#include "cpu/base.hh"
#include "cpu/minor/dyn_inst.hh"
#include "cpu/minor/cpu.hh"
#include "cpu/thread_context.hh"
#include "mem/page_table.hh"
#include "mem/port_proxy.hh"
#include "mem/se_translating_port_proxy.hh"
#include "mem/venus_vrf_mem.hh"
#include "sim/core.hh"
#include "sim/full_system.hh"
#include "sim/process.hh"
#include "sim/sim_exit.hh"
#include "venus/VenusL1DmaEngine.hh"
#include "venus/VenusSequencer.hh"
#include "venus/VenusSharedL2.hh"

namespace gem5
{

VenusDagScheduler::VenusDagScheduler(const VenusDagSchedulerParams &p)
    : ClockedObject(p), taskNames(p.task_names),
      states(taskNames.size(), TaskState::Waiting),
      useL1TimingDma(p.use_l1_timing_dma),
      rtlRuntimeDmtLength(p.rtl_runtime_dmt_length),
      autostart(p.autostart),
      firmwareSecondary(p.firmware_secondary),
      sharedL2Object(dynamic_cast<VenusSharedL2 *>(p.shared_l2_object)),
      l1DmaEngine(dynamic_cast<VenusL1DmaEngine *>(p.l1_dma_engine)),
      pollCycles(p.poll_cycles),
      fireInitialAdmitCycles(p.fire_initial_admit_cycles),
      fireCacheToAdmitCycles(p.fire_cache_to_admit_cycles),
      fireContinueAdmitCycles(p.fire_continue_admit_cycles),
      fireCompleteToStartCycles(p.fire_complete_to_start_cycles),
      responderCoolingCycles(p.responder_cooling_cycles),
      returnInitialAdmitCycles(p.return_initial_admit_cycles),
      returnContinueAdmitCycles(p.return_continue_admit_cycles),
      returnCompleteToCommitCycles(p.return_complete_to_commit_cycles),
      returnCommitToReleaseCycles(p.return_commit_to_release_cycles),
      taskDoneVisibilityCycles(p.task_done_visibility_cycles),
      taskDoneVisibilityUsesTileClock(
          p.task_done_visibility_uses_tile_clock),
      resetTaskPipelineBeforeFire(p.reset_task_pipeline_before_fire),
      taskResetVector(p.task_reset_vector),
      stepEvent([this] { step(); }, name()),
      protocolEvent([this] { runProtocol(); }, name() + ".protocol"),
      outputDumpDir(p.output_dump_dir), outputCounts(p.output_counts)
{
    fatal_if(firmwareSecondary && autostart,
             "secondary firmware DAG cannot autostart");
    fatal_if(p.firmware_identity_addresses.size() != p.firmware_identity_files.size(),
             "firmware identity address/file counts differ");
    for (unsigned i = 0; i < p.firmware_identity_files.size(); ++i) {
        std::ifstream stream(p.firmware_identity_files[i], std::ios::binary);
        fatal_if(!stream, "cannot read firmware identity %s", p.firmware_identity_files[i]);
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(stream)), {});
        const Addr offset = p.firmware_identity_addresses[i];
        fatal_if(bytes.empty() || offset >= 0x02000000 ||
                 bytes.size() > 0x02000000 - offset,
                 "invalid firmware identity segment");
        firmwareIdentity.emplace_back(offset, std::move(bytes));
    }
    fatal_if(!(pollCycles > Cycles(0)),
             "VenusDagScheduler poll_cycles must be nonzero");
    fatal_if(!(taskDoneVisibilityCycles > Cycles(0)),
             "VenusDagScheduler task_done_visibility_cycles must be nonzero");
    fatal_if(taskNames.empty(), "Venus DAG has no tasks");
    if (useL1TimingDma) {
        fatal_if(!sharedL2Object || !l1DmaEngine,
                 "v5 timing DMA requires VenusSharedL2 and VenusL1DmaEngine");
        fatal_if(!(fireInitialAdmitCycles > fireCacheToAdmitCycles) ||
                 !(fireCacheToAdmitCycles > Cycles(0)) ||
                 !(fireContinueAdmitCycles > Cycles(0)) ||
                 !(fireCompleteToStartCycles > Cycles(0)) ||
                 !(responderCoolingCycles > Cycles(0)) ||
                 !(returnInitialAdmitCycles > Cycles(0)) ||
                 !(returnContinueAdmitCycles > Cycles(0)) ||
                 !(returnCompleteToCommitCycles > Cycles(0)) ||
                 !(returnCommitToReleaseCycles > Cycles(0)),
                 "v5 responder phase cycles are invalid");
    }
    fatal_if(!p.task_tile_ids.empty() &&
             p.task_tile_ids.size() != taskNames.size(),
             "Venus DAG task_tile_ids must contain one value per task");
    const bool hasAnyTileRuntime = !p.tile_cpus.empty() ||
        !p.tile_sequencers.empty() || !p.tile_vrfs.empty();
    taskTileIds.assign(taskNames.size(), UnassignedTile);
    if (!p.task_tile_ids.empty())
        taskTileIds.assign(p.task_tile_ids.begin(), p.task_tile_ids.end());

    // A v5 runtime DAG has no task->tile affinity.  The RTL allocator decides
    // only when a descriptor reaches its CACHE/RESPOND path, using live
    // occupancy and retained CRC state.  An explicit task_tile_ids vector is
    // retained solely for older diagnostic callers that intentionally pin a
    // task.  Do not turn an absent vector into an implicit tile-zero pin.
    dynamicTileAssignment = hasAnyTileRuntime && p.task_tile_ids.empty();
    unsigned tileCount = 0;
    if (hasAnyTileRuntime) {
        fatal_if(p.tile_cpus.empty() || p.tile_sequencers.empty() ||
                 p.tile_vrfs.empty(),
                 "Venus DAG tile runtime vectors must be provided together");
        tileCount = p.tile_cpus.size();
        fatal_if(tileCount == 0 || p.tile_sequencers.size() != tileCount ||
                 p.tile_vrfs.size() != tileCount,
                 "Venus DAG needs one CPU, sequencer, and VRF for each physical tile");
    } else {
        // The legacy single-resource path cannot select a live physical tile;
        // preserve its historical tile-zero behavior when no pin was passed.
        if (p.task_tile_ids.empty())
            taskTileIds.assign(taskNames.size(), 0);
        tileCount = *std::max_element(taskTileIds.begin(), taskTileIds.end()) + 1;
    }
    if (!dynamicTileAssignment) {
        for (unsigned task = 0; task < taskNames.size(); ++task)
            fatal_if(taskTileIds.at(task) >= tileCount,
                     "task %d pins invalid physical tile %d (tile count %d)",
                     task, taskTileIds.at(task), tileCount);
    }
    if (!hasAnyTileRuntime) {
        singleResourceFallback = true;
        auto *fallbackCpu = p.cpu;
        auto *fallbackMinor = dynamic_cast<MinorCPU *>(p.cpu);
        auto *fallbackSequencer = dynamic_cast<VenusSequencer *>(p.sequencer);
        auto *fallbackVrf = dynamic_cast<memory::venus_vrf_mem *>(p.vrf);
        fatal_if(!fallbackCpu || !fallbackMinor || !fallbackSequencer ||
                 !fallbackVrf,
                 "VenusDagScheduler requires a MinorCPU, VenusSequencer, and Venus VRF");
        tileRuntimes.resize(tileCount);
        for (auto &runtime : tileRuntimes) {
            runtime.cpu = fallbackCpu;
            runtime.minorCpu = fallbackMinor;
            runtime.sequencer = fallbackSequencer;
            runtime.vrf = fallbackVrf;
        }
        taskContextIds.resize(taskNames.size());
        for (unsigned task = 0; task < taskNames.size(); ++task)
            taskContextIds[task] = task;
        fallbackMinor->setVenusSequencer(fallbackSequencer);
    } else {
        fatal_if(p.task_context_ids.size() != taskNames.size(),
                 "Venus DAG task_context_ids must contain one value per task");
        tileRuntimes.resize(tileCount);
        for (unsigned tile = 0; tile < tileCount; ++tile) {
            auto &runtime = tileRuntimes[tile];
            runtime.cpu = dynamic_cast<BaseCPU *>(p.tile_cpus[tile]);
            runtime.minorCpu = dynamic_cast<MinorCPU *>(p.tile_cpus[tile]);
            runtime.sequencer =
                dynamic_cast<VenusSequencer *>(p.tile_sequencers[tile]);
            runtime.vrf = dynamic_cast<memory::venus_vrf_mem *>(p.tile_vrfs[tile]);
            fatal_if(!runtime.cpu || !runtime.minorCpu ||
                     !runtime.sequencer || !runtime.vrf,
                     "Venus DAG tile %d requires a MinorCPU, VenusSequencer, and Venus VRF",
                     tile);
            runtime.minorCpu->setVenusSequencer(runtime.sequencer);
        }
        taskContextIds.assign(p.task_context_ids.begin(),
                              p.task_context_ids.end());
    }
    taskHardwareRequirements.assign(taskNames.size(), 0);
    taskCrcs.assign(taskNames.size(), 0);
    taskNeedSpmd.assign(taskNames.size(), false);
    taskMinimumSpmdTasks.assign(taskNames.size(), 0);
    if (dynamicTileAssignment) {
        fatal_if(p.task_hardware_requirements.size() != taskNames.size() ||
                 p.task_crcs.size() != taskNames.size() ||
                 p.task_need_spmd.size() != taskNames.size() ||
                 p.task_minimum_spmd_tasks.size() != taskNames.size(),
                 "dynamic Venus DAG allocation needs RTL task hardware metadata");
        fatal_if(p.tile_hardware_capabilities.size() != tileCount,
                 "dynamic Venus DAG allocation needs one RTL capability per tile");
        taskHardwareRequirements.assign(p.task_hardware_requirements.begin(),
                                        p.task_hardware_requirements.end());
        taskCrcs.assign(p.task_crcs.begin(), p.task_crcs.end());
        taskNeedSpmd.assign(p.task_need_spmd.begin(), p.task_need_spmd.end());
        taskMinimumSpmdTasks.assign(p.task_minimum_spmd_tasks.begin(),
                                    p.task_minimum_spmd_tasks.end());
        tileHardwareCapabilities.assign(p.tile_hardware_capabilities.begin(),
                                        p.tile_hardware_capabilities.end());
        for (unsigned task = 0; task < taskNames.size(); ++task) {
            fatal_if(taskNeedSpmd.at(task),
                     "task %d requests RTL SPMD allocation; GEM5 dynamic tile "
                     "allocator currently models one-tile tasks only", task);
        }
    }
    tileLastCrc.assign(tileCount, 0);
    taskLaneCompatDistances.assign(taskNames.size(), 0);
    tileStates.resize(tileCount);
    functionalStartTicks.resize(taskNames.size(), 0);
    modeledStartTicks.resize(taskNames.size(), 0);
    modeledEndTicks.resize(taskNames.size(), 0);
    modeledTileFreeTicks.resize(tileCount, 0);
    if (singleResourceFallback) {
        for (unsigned tile = 0; tile < tileStates.size(); ++tile) {
            tileStates[tile].spm.resize(SpmSnapshotSize, 0);
            tileStates[tile].vspm.resize(
                tileRuntimes.at(tile).vrf->persistentStateBytes(), 0);
        }
    }
    fatal_if(p.task_code_files.size() != p.task_data_files.size(),
             "Venus DAG task code/data vectors have different lengths");
    fatal_if(!p.task_code_files.empty() &&
             p.task_code_files.size() != taskNames.size(),
             "Venus DAG task images must contain one entry per task");
    taskImages.resize(taskNames.size());
    for (size_t i = 0; i < p.task_code_files.size(); ++i) {
        auto readImage = [](const std::string &path) {
            std::ifstream stream(path, std::ios::binary);
            fatal_if(!stream, "cannot open Venus task image %s", path);
            return std::vector<uint8_t>(
                std::istreambuf_iterator<char>(stream),
                std::istreambuf_iterator<char>());
        };
        taskImages[i].code = readImage(p.task_code_files[i]);
        taskImages[i].data = readImage(p.task_data_files[i]);
        fatal_if(taskImages[i].code.size() > 0x8000,
                 "task %d code image is larger than RTL ISPM", i);
        fatal_if(taskImages[i].data.size() > 0x4000,
                 "task %d data image is larger than RTL DSPM", i);
    }
    if (useL1TimingDma) {
        fatal_if(!outputCounts.empty() && outputCounts.size() != taskNames.size(),
                 "v5 timing DMA output_counts must contain one value per task");
        fatal_if(p.task_code_sources.size() != taskNames.size() ||
                 p.task_data_sources.size() != taskNames.size(),
                 "v5 timing DMA needs one code/data source per task");
        fatal_if(p.dmt_slot_tasks.size() != p.dmt_slot_ports.size() ||
                 p.dmt_slot_tasks.size() != p.dmt_slot_relative_offsets.size() ||
                 p.dmt_slot_tasks.size() != p.dmt_slot_capacities.size(),
                 "v5 timing DMT slot vectors have different lengths");
        fatal_if(p.dmt_slot_tasks.empty(),
                 "v5 timing DMA needs static runtime-DAG DMT slots");
        fatal_if(p.dmt_malloc_base == 0,
                 "v5 timing DMA needs a nonzero runtime-DAG L2_malloc base");
        dmtMallocBase = p.dmt_malloc_base;
        nextDmtAddress = dmtMallocBase;
        unsigned slotCount = 0;
        for (size_t i = 0; i < p.dmt_slot_tasks.size(); ++i) {
            const unsigned task = p.dmt_slot_tasks[i];
            const unsigned port = p.dmt_slot_ports[i];
            const Addr relative = p.dmt_slot_relative_offsets[i];
            const uint32_t capacity = p.dmt_slot_capacities[i];
            fatal_if(task >= taskNames.size() ||
                     port >= std::max(1u, outputCounts.empty()
                                          ? 16u : outputCounts[task]),
                     "invalid v5 DMT slot %d:%d", task, port);
            fatal_if(relative > sharedL2Object->size() ||
                     capacity > sharedL2Object->size() - relative ||
                     dmtMallocBase > sharedL2Object->size() - relative - capacity,
                     "v5 DMT slot %d:%d exceeds shared-L2 capacity", task, port);
            auto inserted = dmt.emplace(
                std::make_pair(task, port), DmtSlot{});
            fatal_if(!inserted.second,
                     "duplicate v5 DMT slot %d:%d", task, port);
            auto &slot = inserted.first->second;
            slot.consumerBytes = capacity;
            slot.hasL2Address = true;
            slot.l2Address = dmtMallocBase + relative;
            ++slotCount;
        }
        if (!outputCounts.empty()) {
            const unsigned expected = std::accumulate(
                outputCounts.begin(), outputCounts.end(), 0u);
            fatal_if(slotCount != expected,
                     "v5 DMT layout has %d slots, expected %d runtime outputs",
                     slotCount, expected);
        }
    }
    fatal_if(p.input_tasks.size() != p.input_parents.size() ||
             p.input_tasks.size() != p.input_ports.size() ||
             p.input_tasks.size() != p.input_destinations.size() ||
             p.input_tasks.size() != p.input_lengths.size() ||
             p.input_tasks.size() != p.input_types.size() ||
             p.input_tasks.size() != p.input_pointer_files.size() ||
             p.input_tasks.size() != p.input_descriptor_indices.size(),
             "Venus DAG input parameter vectors have different lengths");
    fatal_if(p.output_tasks.size() != p.output_ports.size() ||
             p.output_tasks.size() != p.output_sources.size() ||
             p.output_tasks.size() != p.output_lengths.size(),
             "Venus DAG output parameter vectors have different lengths");
    fatal_if(p.initial_input_tasks.size() != p.initial_input_destinations.size() ||
             p.initial_input_tasks.size() != p.initial_input_files.size(),
             "Venus DAG initial input vectors have different lengths");
    const bool hasSlotMetadata = !p.input_slot_addresses.empty() ||
        !p.input_slot_consumer_bytes.empty() ||
        !p.input_slot_address_valid.empty();
    fatal_if(hasSlotMetadata &&
             (p.input_slot_addresses.size() != p.input_tasks.size() ||
              p.input_slot_consumer_bytes.size() != p.input_tasks.size() ||
              p.input_slot_address_valid.size() != p.input_tasks.size()),
             "Venus DAG DMT slot metadata vectors have different lengths");

    for (size_t i = 0; i < p.input_tasks.size(); ++i) {
        fatal_if(p.input_tasks[i] >= taskNames.size() ||
                 p.input_parents[i] >= taskNames.size(),
                 "Venus DAG input references an invalid task");
        fatal_if(p.input_types[i] != 0 && p.input_types[i] != 4,
                 "unsupported Venus dependency input type %d", p.input_types[i]);
        std::vector<uint8_t> pointerData;
        // v5 constructs ptr_temp from static DMT layout after the producer
        // commits.  A historic pointer dump is evidence only and must never
        // become a timing-model input or a mandatory filesystem dependency.
        if (!useL1TimingDma && !p.input_pointer_files[i].empty()) {
            std::ifstream stream(p.input_pointer_files[i], std::ios::binary);
            fatal_if(!stream, "cannot open ptr_temp input %s", p.input_pointer_files[i]);
            pointerData.assign(std::istreambuf_iterator<char>(stream),
                               std::istreambuf_iterator<char>());
            fatal_if(pointerData.size() != 64,
                     "ptr_temp input %s is %d bytes, expected 64",
                     p.input_pointer_files[i], pointerData.size());
        }
        Input input{
            p.input_tasks[i], p.input_parents[i], p.input_ports[i],
            p.input_destinations[i], p.input_lengths[i],
            static_cast<uint8_t>(p.input_types[i]),
            hasSlotMetadata ? p.input_slot_addresses[i] : 0,
            hasSlotMetadata ? p.input_slot_consumer_bytes[i] : 0,
            hasSlotMetadata && p.input_slot_address_valid[i],
            p.input_descriptor_indices[i],
            std::move(pointerData),
        };
        registerDmtSlot(input);
        inputs.push_back(std::move(input));
    }
    for (size_t i = 0; i < p.output_tasks.size(); ++i) {
        fatal_if(p.output_tasks[i] >= taskNames.size(),
                 "Venus DAG output references an invalid task");
        outputs.push_back({p.output_tasks[i], p.output_ports[i],
                           p.output_sources[i], p.output_lengths[i]});
    }
    for (size_t i = 0; i < p.initial_input_tasks.size(); ++i) {
        fatal_if(p.initial_input_tasks[i] >= taskNames.size(),
                 "Venus DAG initial input references an invalid task");
        std::ifstream stream(p.initial_input_files[i], std::ios::binary);
        fatal_if(!stream, "cannot open Venus initial input %s",
                 p.initial_input_files[i]);
        std::vector<uint8_t> data(
            (std::istreambuf_iterator<char>(stream)),
            std::istreambuf_iterator<char>());
        initialInputs.push_back({p.initial_input_tasks[i],
                                 p.initial_input_destinations[i],
                                 std::move(data)});
    }
    fatal_if(!outputCounts.empty() && outputCounts.size() != taskNames.size(),
             "Venus DAG output_counts must contain one value per task");
    if (useL1TimingDma) {
        const size_t n = p.timing_fire_tasks.size();
        fatal_if(p.timing_fire_ordinals.size() != n ||
                 p.timing_fire_kinds.size() != n ||
                 p.timing_fire_input_types.size() != n ||
                 p.timing_fire_source_kinds.size() != n ||
                 p.timing_fire_source_offsets.size() != n ||
                 p.timing_fire_destinations.size() != n ||
                 p.timing_fire_lengths.size() != n ||
                 p.timing_fire_parents.size() != n ||
                 p.timing_fire_ports.size() != n ||
                 p.timing_fire_pointer_bytes.size() != n,
                 "v5 timing fire vectors have different lengths");
        timingFires.resize(taskNames.size());
        for (size_t i = 0; i < n; ++i) {
            fatal_if(p.timing_fire_tasks[i] >= taskNames.size() ||
                     p.timing_fire_kinds[i] > 2 ||
                     p.timing_fire_source_kinds[i] > 3,
                     "invalid v5 timing fire %d", i);
            TimingFire fire{
                p.timing_fire_tasks[i], p.timing_fire_ordinals[i],
                static_cast<uint8_t>(p.timing_fire_kinds[i]),
                static_cast<uint8_t>(p.timing_fire_input_types[i]),
                static_cast<FireSource>(p.timing_fire_source_kinds[i]),
                p.timing_fire_source_offsets[i],
                p.timing_fire_destinations[i], p.timing_fire_lengths[i],
                p.timing_fire_parents[i], p.timing_fire_ports[i],
                p.timing_fire_pointer_bytes[i],
            };
            timingFires[fire.task].push_back(std::move(fire));
        }
        for (unsigned task = 0; task < timingFires.size(); ++task) {
            auto &fires = timingFires[task];
            std::sort(fires.begin(), fires.end(),
                      [](const TimingFire &a, const TimingFire &b) {
                          return a.ordinal < b.ordinal;
                      });
            fatal_if(fires.empty(), "v5 task %d has no fire transfers", task);
            for (unsigned ordinal = 0; ordinal < fires.size(); ++ordinal)
                fatal_if(fires[ordinal].ordinal != ordinal,
                         "v5 task %d fire ordinals are not contiguous", task);
        }
        nextTimingFire.assign(taskNames.size(), 0);
        timingReturns.resize(taskNames.size());
        nextTimingReturn.assign(taskNames.size(), 0);

        // Static addresses are generated by the runtime DAG's DMT layout;
        // only readiness and the dynamic returned prefix change at runtime.
        for (auto &entry : dmt) {
            entry.second.complete = false;
            entry.second.validBytes = 0;
        }
    }
    trace.open(p.trace_file, std::ios::out | std::ios::trunc);
    fatal_if(!trace, "cannot open Venus DAG trace %s", p.trace_file);
}

void
VenusDagScheduler::registerDmtSlot(const Input &input)
{
    uint32_t consumerBytes = input.slotConsumerBytes;
    bool hasL2Address = input.hasSlotAddress;
    Addr l2Address = input.slotAddress;

    if (input.type == 0) {
        // Legacy profiles use this consumer extent as the fire length. Venus1
        // keeps it as backing metadata only: ordinary fire DMA reads the
        // producer's runtime DMT input_len, independently of this hint.
        fatal_if(!rtlRuntimeDmtLength && input.slotConsumerBytes && input.length &&
                 input.slotConsumerBytes != input.length,
                 "DMT consumer extent disagrees with type-0 fire length for "
                 "%d:%d -> task %d: %u != %u",
                 input.parent, input.port, input.task,
                 input.slotConsumerBytes, input.length);
        if (!consumerBytes)
            consumerBytes = input.length;
    } else {
        // v5 carries allocation semantics separately from any captured
        // ptr_temp bytes.  Older manifests may still provide a file; when
        // present it is a validation input, never the timing engine's source.
        if (!input.pointerData.empty()) {
            const uint32_t pointerConsumerBytes =
                uint32_t(input.pointerData[0]) |
                (uint32_t(input.pointerData[1]) << 8);
            uint32_t pointerAddress = 0;
            for (unsigned byte = 0; byte < 4; ++byte)
                pointerAddress |= uint32_t(input.pointerData[2 + byte]) <<
                    (byte * 8);
            fatal_if(consumerBytes && consumerBytes != pointerConsumerBytes,
                     "ptr_temp consumer extent disagrees with manifest for %d:%d -> "
                     "task %d: %u != %u",
                     input.parent, input.port, input.task,
                     consumerBytes, pointerConsumerBytes);
            fatal_if(hasL2Address && l2Address != pointerAddress,
                     "ptr_temp address disagrees with manifest for %d:%d -> "
                     "task %d: 0x%x != 0x%x",
                     input.parent, input.port, input.task,
                     static_cast<unsigned>(l2Address),
                     static_cast<unsigned>(pointerAddress));
            if (!consumerBytes)
                consumerBytes = pointerConsumerBytes;
            if (!hasL2Address) {
                hasL2Address = true;
                l2Address = pointerAddress;
            }
        }
        fatal_if(!consumerBytes,
                 "ptr_temp %d:%d -> task %d has no DMT allocation capacity",
                 input.parent, input.port, input.task);
    }

    auto &slot = dmt[{input.parent, input.port}];
    // Several consumers may legally request different fixed prefixes of one
    // producer return.  Keep enough seed bytes for the largest request; the
    // backing storage itself can grow to the dynamic return length later.
    slot.consumerBytes = std::max(slot.consumerBytes, consumerBytes);
    fatal_if(hasL2Address && slot.hasL2Address &&
             slot.l2Address != l2Address,
             "inconsistent DMT slot address for producer %d:%d: "
             "0x%x != 0x%x",
             input.parent, input.port,
             static_cast<unsigned>(slot.l2Address),
             static_cast<unsigned>(l2Address));
    if (hasL2Address) {
        slot.hasL2Address = true;
        slot.l2Address = l2Address;
    }
}

void
VenusDagScheduler::startup()
{
    if (!firmwareSecondary) {
        for (auto &runtime : tileRuntimes)
            runtime.minorCpu->setVenusDagScheduler(this);
    }

    // SimObject::startup runs after VenusSequencer::init has loaded the
    // optional L1-produced shared-L2 image.  Seed each static consumer range
    // now: a shorter dynamic return may only update its prefix, while RTL
    // leaves the backing-store tail intact for a later fixed-size fire DMA.
    if (!useL1TimingDma) {
        for (auto &entry : dmt) {
            auto &slot = entry.second;
            if (!slot.consumerBytes)
                continue;
            slot.bytes.resize(slot.consumerBytes, 0);
            if (slot.hasL2Address)
                tileRuntimes.front().sequencer->readSharedL2(
                    slot.l2Address, slot.consumerBytes, slot.bytes.data());
        }
    }
    // In RTL, scalar accesses to the DSPM and VSPM windows are decoded by
    // venus_block_wrapper/venus_mem2lanes and reach the same physical SRAMs
    // as DMA, LDU, and the lanes.
    // SE mode normally backs every ELF virtual page with an arbitrary page
    // from the generic physical-memory pool. Map every task's local/block
    // views to its physical RTL tile window. The guest addresses remain the
    // task-program ABI (0x000/0x800 block views); only the per-process
    // translation selects 0x82000000 + tile * 2 MiB.
    // RV32 SE page tables retain the 32-bit ELF virtual address.  Register
    // values are sign-extended when displayed by the ISA, but looking up the
    // sign-extended form here misses the mapping created by the ELF loader.
    constexpr Addr ispmLocalBase = 0x00000000ULL;
    constexpr Addr ispmBlockBase = 0x80000000ULL;
    constexpr Addr ispmSize = 0x00008000ULL;
    constexpr Addr dspmLocalBase = 0x00020000ULL;
    constexpr Addr dspmBlockBase = 0x80020000ULL;
    // .config selects CONFIG_*_SPADDMEM=0x4000 for this SoC.  Do not map the
    // entire architectural gap up to VSPM as DSPM: addresses above 0x23fff
    // are outside the implemented tile SRAM, and the 0x8002xxxx view is
    // decoded as cluster L2 rather than tile-local storage.
    constexpr Addr dspmSize = 0x00004000ULL;
    // Task binaries use both architectural views of VSPM.  Vector/DMA
    // descriptors carry the block-perspective 0x8010xxxx address, while the
    // scalar stack lives in the matching local 0x0010xxxx window and is
    // converted to a block address only when it is returned.  They must map
    // to the same banked SRAM; otherwise scalar-produced return buffers are
    // invisible to captureOutputs() and stale vector data is published.
    constexpr Addr vspmLocalBase = 0x00100000ULL;
    constexpr Addr vspmBlockBase = 0x80100000ULL;
    constexpr Addr vspmSize = 0x000ff000ULL;
    constexpr Addr controlBlockBase = 0x801ff000ULL;
    constexpr Addr controlSize = 0x00001000ULL;
    constexpr Addr rtlTileBase = 0x82000000ULL;
    constexpr Addr rtlTileStride = 0x00200000ULL;
    constexpr Addr rtlDspmOffset = 0x00020000ULL;
    constexpr Addr rtlVspmOffset = 0x00100000ULL;
    constexpr Addr rtlControlOffset = 0x001ff000ULL;

    std::vector<std::vector<bool>> contextUsed(tileRuntimes.size());
    for (unsigned tile = 0; tile < tileRuntimes.size(); ++tile)
        contextUsed[tile].resize(tileRuntimes[tile].cpu->numContexts(), false);
    const auto mapTaskContext = [this, &contextUsed,
                                 ispmLocalBase, ispmBlockBase, ispmSize,
                                 dspmLocalBase, dspmBlockBase, dspmSize,
                                 vspmLocalBase, vspmBlockBase, vspmSize,
                                 controlBlockBase, controlSize, rtlTileBase,
                                 rtlTileStride, rtlDspmOffset, rtlVspmOffset,
                                 rtlControlOffset](unsigned task, unsigned tile) {
        auto &runtime = tileRuntimes.at(tile);
        const unsigned contextId = taskContextIds.at(task);
        fatal_if(contextId >= runtime.cpu->numContexts(),
                 "task %d requests context %d on tile %d, which has only %d contexts",
                 task, contextId, tile, runtime.cpu->numContexts());
        fatal_if(contextUsed[tile][contextId],
                 "task %d reuses live tile %d context %d", task, tile, contextId);
        contextUsed[tile][contextId] = true;
        ThreadContext *tc = runtime.cpu->getContext(contextId);
        Process *process = tc->getProcessPtr();
        fatal_if(!process, "task %d has no SE process", task);
        const Addr physicalTileBase = rtlTileBase + tile * rtlTileStride;
        process->pTable->map(
            ispmLocalBase, physicalTileBase, ispmSize,
            EmulationPageTable::Clobber | EmulationPageTable::Uncacheable);
        process->pTable->map(
            ispmBlockBase, physicalTileBase, ispmSize,
            EmulationPageTable::Clobber | EmulationPageTable::Uncacheable);
        process->pTable->map(
            dspmLocalBase, physicalTileBase + rtlDspmOffset, dspmSize,
            EmulationPageTable::Clobber | EmulationPageTable::Uncacheable);
        process->pTable->map(
            dspmBlockBase, physicalTileBase + rtlDspmOffset, dspmSize,
            EmulationPageTable::Clobber | EmulationPageTable::Uncacheable);
        fatal_if(!process || !process->pTable->lookup(vspmBlockBase),
                 "task %d has no ELF mapping for the Venus VSPM window", task);
        process->pTable->map(
            vspmLocalBase, physicalTileBase + rtlVspmOffset, vspmSize,
            EmulationPageTable::Clobber | EmulationPageTable::Uncacheable);
        process->pTable->map(
            vspmBlockBase, physicalTileBase + rtlVspmOffset, vspmSize,
            EmulationPageTable::Clobber | EmulationPageTable::Uncacheable);
        process->pTable->map(
            controlBlockBase, physicalTileBase + rtlControlOffset, controlSize,
            EmulationPageTable::Clobber | EmulationPageTable::Uncacheable);
        // Fetch may have populated the SE ITLB before SimObject::startup()
        // suspends the contexts.  Page-table remapping alone does not evict
        // that translation, so the first task would continue fetching from
        // its ELF-loader page instead of the physical tile DSPM.
        tc->getMMUPtr()->flushAll();
        if (resetTaskPipelineBeforeFire)
            runtime.minorCpu->prepareVenusTaskContext(contextId);
        tc->suspend();
    };
    for (unsigned task = 0; task < taskNames.size(); ++task) {
        if (dynamicTileAssignment) {
            // Each task needs an independently mapped SE process/context on
            // every physical tile it may be assigned to.  Sharing the
            // task's original page table here would make later mappings
            // overwrite the tile selected by an earlier dispatch.
            for (unsigned tile = 0; tile < tileRuntimes.size(); ++tile)
                mapTaskContext(task, tile);
        } else {
            mapTaskContext(task, assignedTile(task));
        }
    }
    emit("dag_registered", 0,
         "\"task_count\":" + std::to_string(taskNames.size()));
    if (autostart) {
        started = true;
        schedule(stepEvent, afterCycles(pollCycles));
    } else {
        emit("dag_armed", 0, "\"source\":\"scheduler_firmware\"");
    }
}

bool
VenusDagScheduler::matchesFirmware(Addr mallocBase) const
{
    if (firmwareIdentity.empty() || !sharedL2Object || mallocBase != dmtMallocBase)
        return false;
    for (const auto &segment : firmwareIdentity) {
        std::vector<uint8_t> actual(segment.second.size());
        sharedL2Object->read(segment.first, actual.size(), actual.data());
        if (actual != segment.second)
            return false;
    }
    return true;
}

void
VenusDagScheduler::selectFirmwareDag(VenusDagScheduler *selected)
{
    fatal_if(firmwareSecondary || autostart || !selected,
             "invalid primary firmware DAG router");
    auto *previous = firmwareDelegate ? firmwareDelegate : this;
    fatal_if((previous->started && !previous->completionNotified) ||
             (selected->started && !selected->completionNotified),
             "cannot switch a live firmware DAG");
    fatal_if(tileRuntimes.size() != selected->tileRuntimes.size(),
             "firmware DAG tile topology differs");
    for (unsigned i = 0; i < tileRuntimes.size(); ++i)
        fatal_if(tileRuntimes[i].cpu != selected->tileRuntimes[i].cpu,
                 "firmware DAG must share physical tile resources");
    // CRC caching belongs to the physical tile, not to the DAG identity.
    selected->tileLastCrc = previous->tileLastCrc;
    firmwareDelegate = selected;
}

void
VenusDagScheduler::startFromFirmware(uint64_t fireId)
{
    fatal_if(autostart,
             "firmware attempted to start an autostart Venus DAG");
    fatal_if(started && !completionNotified,
             "firmware attempted to start a live Venus DAG");
    fatal_if(stepEvent.scheduled() || protocolEvent.scheduled() ||
             protocolAction || timingDmaHasWork(),
             "firmware start before previous DAG drained");
    fatal_if(!useL1TimingDma || singleResourceFallback,
             "firmware repeated execution requires physical timing-DMA tiles");
    for (auto &runtime : tileRuntimes) {
        fatal_if(runtime.runningTask >= 0 || runtime.allocatedTask >= 0,
                 "firmware start on a live tile");
        runtime.nextDrainTrace = 0;
        runtime.executionCompleteTick = 0;
    }
    states.assign(taskNames.size(), TaskState::Waiting);
    if (dynamicTileAssignment)
        taskTileIds.assign(taskNames.size(), UnassignedTile);
    nextDispatchTask = 0;
    std::fill(nextTimingFire.begin(), nextTimingFire.end(), 0);
    std::fill(nextTimingReturn.begin(), nextTimingReturn.end(), 0);
    for (auto &entries : timingReturns)
        entries.clear();
    for (auto &entry : dmt) {
        entry.second.complete = false;
        entry.second.validBytes = 0;
        entry.second.bytes.clear();
    }
    std::fill(functionalStartTicks.begin(), functionalStartTicks.end(), 0);
    std::fill(modeledStartTicks.begin(), modeledStartTicks.end(), 0);
    std::fill(modeledEndTicks.begin(), modeledEndTicks.end(), 0);
    std::fill(modeledTileFreeTicks.begin(), modeledTileFreeTicks.end(), 0);
    completionNotified = false;
    firmwareFireId = fireId;
    started = true;
    emit("dag_fire", 0, "\"source\":\"scheduler_firmware\"");
    // L2 reset is an MMIO register in the scheduler clock domain.  The task
    // manager observes it on a following edge; this is a clock-domain rule,
    // not a workload delay.
    schedule(stepEvent, afterCycles(Cycles(1)));
}

void
VenusDagScheduler::setFirmwareCompletionCallback(
    std::function<void()> callback)
{
    fatal_if(autostart,
             "cannot attach firmware completion to an autostart Venus DAG");
    fatal_if(firmwareCompletionCallback,
             "Venus DAG already has a firmware completion callback");
    fatal_if(!callback, "Venus DAG firmware completion callback is empty");
    firmwareCompletionCallback = std::move(callback);
}

std::vector<VenusDagScheduler::FirmwareReturn>
VenusDagScheduler::firmwareReturns() const
{
    fatal_if(!completionNotified,
             "firmware requested Venus DAG returns before completion");
    std::vector<FirmwareReturn> result;
    result.reserve(outputs.size());
    for (const auto &output : outputs) {
        const auto slot = dmt.find({output.task, output.port});
        fatal_if(slot == dmt.end() || !slot->second.complete ||
                     !slot->second.hasL2Address,
                 "DAG output %d:%d has no completed DMT return",
                 output.task, output.port);
        result.push_back({slot->second.l2Address, slot->second.validBytes});
    }
    return result;
}

bool
VenusDagScheduler::isVrfAddress(Addr addr)
{
    // The final 4 KiB of the architectural VSPM window is the tile control
    // page, not lane SRAM.  A bit-20 test incorrectly routed that page to
    // VSPM and hid future control-DMA errors.
    return addr >= RtlVspmOffset && addr < RtlControlOffset;
}

unsigned
VenusDagScheduler::assignedTile(unsigned task) const
{
    const unsigned tile = taskTileIds.at(task);
    fatal_if(tile == UnassignedTile,
             "task %d has not reached RTL tile allocation yet", task);
    fatal_if(tile >= tileRuntimes.size(),
             "task %d selected invalid tile %d (tile count %d)", task, tile,
             tileRuntimes.size());
    return tile;
}

bool
VenusDagScheduler::tileFitsTask(unsigned task, unsigned tile,
                                 int &laneCompatDistance,
                                 unsigned &laneCompatPrice) const
{
    if (tile >= tileRuntimes.size() || tileBusy(tile))
        return false;
    const uint32_t requirement = taskHardwareRequirements.at(task);
    const uint32_t capability = tileHardwareCapabilities.at(tile);
    const unsigned requiredSpm = (requirement >> 7) & 0xf;
    const unsigned requiredLanes = (requirement >> 3) & 0xf;
    const unsigned requiredBitAlu = (requirement >> 2) & 1;
    const unsigned requiredSerDiv = (requirement >> 1) & 1;
    const unsigned requiredComplex = requirement & 1;
    const unsigned tileSpm = (capability >> 7) & 0xf;
    const unsigned tileLanes = (capability >> 3) & 0xf;
    const unsigned tileBitAlu = (capability >> 2) & 1;
    const unsigned tileSerDiv = (capability >> 1) & 1;
    const unsigned tileComplex = capability & 1;
    if (requiredSpm > tileSpm || requiredBitAlu > tileBitAlu ||
        requiredSerDiv > tileSerDiv || requiredComplex > tileComplex ||
        (tileLanes == 0 && requiredLanes != 0))
        return false;

    // Match venus_task_lv_scheduler_dag_req_allocator.sv exactly: a tile
    // with fewer lanes has an even cost, a tile with surplus lanes has an
    // odd cost, and only a strictly smaller cost replaces the earlier tile.
    laneCompatDistance = static_cast<int>(tileLanes) -
                         static_cast<int>(requiredLanes);
    laneCompatPrice = laneCompatDistance <= 0
        ? static_cast<unsigned>(-laneCompatDistance * 2)
        : static_cast<unsigned>(laneCompatDistance * 2 - 1);
    return true;
}

unsigned
VenusDagScheduler::selectTileForTask(unsigned task)
{
    if (taskTileIds.at(task) != UnassignedTile)
        return assignedTile(task);
    fatal_if(!dynamicTileAssignment,
             "task %d has no pinned tile outside dynamic tile mode", task);

    unsigned selected = UnassignedTile;
    unsigned selectedPrice = 0x1f;
    int selectedDistance = 0;
    for (unsigned tile = 0; tile < tileRuntimes.size(); ++tile) {
        int distance = 0;
        unsigned price = 0;
        if (!tileFitsTask(task, tile, distance, price))
            continue;
        // The RTL breaks on the first eligible retained-CRC hit, before
        // considering any later tile or lower lane-compatibility price.
        if (tileLastCrc.at(tile) == taskCrcs.at(task)) {
            selected = tile;
            selectedDistance = distance;
            break;
        }
        if (price < selectedPrice) {
            selected = tile;
            selectedPrice = price;
            selectedDistance = distance;
        }
    }
    fatal_if(selected == UnassignedTile,
             "task %d has no eligible idle RTL tile", task);
    taskTileIds.at(task) = selected;
    taskLaneCompatDistances.at(task) = selectedDistance;
    std::ostringstream extra;
    extra << "\"tile_id\":" << selected
          << ",\"lane_compat_distance\":" << selectedDistance
          << ",\"allocator\":\"rtl_lowest_price_crc_first\"";
    emit("tile_selected", task, extra.str());
    return selected;
}

VenusDagScheduler::TileRuntime &
VenusDagScheduler::runtimeForTask(unsigned task)
{
    return tileRuntimes.at(assignedTile(task));
}

const VenusDagScheduler::TileRuntime &
VenusDagScheduler::runtimeForTask(unsigned task) const
{
    return tileRuntimes.at(assignedTile(task));
}

Addr
VenusDagScheduler::tileBusAddress(unsigned task, Addr localAddress) const
{
    return RtlTilePhysicalBase + assignedTile(task) * RtlTileStride +
           (localAddress & 0x001fffffULL);
}

Addr
VenusDagScheduler::tileDmaDestination(unsigned task, Addr localAddress) const
{
    const int distance = taskLaneCompatDistances.at(task);
    if (distance <= 0 || !isVrfAddress(localAddress))
        return localAddress;
    // dag_req_responder preserves bits [31:20] and truncates the shifted
    // low 20 bits when assigning a wider-lane tile's VSPM destination.
    const Addr high = localAddress & ~Addr(0x000fffff);
    const Addr low = ((localAddress & 0x000fffff) << distance) & 0x000fffff;
    return high | low;
}

size_t
VenusDagScheduler::tileDmaBeatLimit(Addr source, Addr destination,
                                    size_t naturalBytes) const
{
    fatal_if(naturalBytes == 0,
             "v5 timing DMA requested a zero-byte raw beat limit");

    const auto bytesToTileDecodeBoundary = [this, naturalBytes](Addr address) {
        for (unsigned tile = 0; tile < tileRuntimes.size(); ++tile) {
            const Addr tileBase = RtlTilePhysicalBase +
                static_cast<Addr>(tile) * RtlTileStride;
            if (address < tileBase || address >= tileBase + RtlTileStride)
                continue;

            const Addr local = address - tileBase;
            Addr nextBoundary = RtlTileStride;
            if (local < RtlDspmOffset) {
                // The block wrapper exposes four 32-KiB ISPM apertures in
                // this address range.  They all select the same physical
                // ISPM SRAM, so a timing packet may not straddle one of
                // these non-injective decode boundaries.
                nextBoundary = RtlIspmOffset +
                    ((local - RtlIspmOffset) / RtlIspmBytes + 1) *
                    RtlIspmBytes;
            } else if (local < RtlVspmOffset) {
                // Each 16-KiB DSPM aperture aliases the same RTL RAM.
                nextBoundary = RtlDspmOffset +
                    ((local - RtlDspmOffset) / RtlDspmBytes + 1) *
                    RtlDspmBytes;
            } else if (local < RtlControlOffset) {
                // venus_mem2lanes also has a 16-KiB physical backing
                // aperture.  The VSPM backdoor implements that fold, but a
                // timing Packet must still stop at its raw decode boundary.
                nextBoundary = RtlVspmOffset +
                    ((local - RtlVspmOffset) / RtlVspmBytes + 1) *
                    RtlVspmBytes;
            } else if (local < RtlControlOffset + RtlControlBytes) {
                nextBoundary = RtlControlOffset + RtlControlBytes;
            }
            return std::min(naturalBytes,
                            static_cast<size_t>(nextBoundary - local));
        }
        return naturalBytes;
    };

    return std::min(bytesToTileDecodeBoundary(source),
                    bytesToTileDecodeBoundary(destination));
}

ThreadContext *
VenusDagScheduler::contextForTask(unsigned task)
{
    return contextForTileTask(assignedTile(task), task);
}

ThreadContext *
VenusDagScheduler::contextForTileTask(unsigned tile, unsigned task)
{
    auto &runtime = tileRuntimes.at(tile);
    return runtime.cpu->getContext(taskContextIds.at(task));
}

std::vector<uint8_t>
VenusDagScheduler::readTile(unsigned task, Addr addr, size_t size)
{
    auto &runtime = runtimeForTask(task);
    ThreadContext *tc = contextForTask(task);
    std::vector<uint8_t> data(size);
    if (isVrfAddress(addr)) {
        runtime.vrf->backdoor_ReadVspm(addr, size, data.data());
    } else if (addr < RtlDspmOffset) {
        // The 128-KiB architectural ISPM window is four aliases of the
        // 32-KiB physical ISPM.  Keep this backdoor path consistent with the
        // timing RangeAddrMapper, including a direct read across a wrap.
        size_t copied = 0;
        while (copied < size) {
            const Addr offset = (addr + copied - RtlIspmOffset) % RtlIspmBytes;
            const size_t chunk = std::min(
                size - copied, static_cast<size_t>(RtlIspmBytes - offset));
            SETranslatingPortProxy(tc).readBlob(
                BlockPerspectiveBase | (RtlIspmOffset + offset),
                data.data() + copied, chunk);
            copied += chunk;
        }
    } else if (addr >= RtlDspmOffset && addr < RtlVspmOffset) {
        // An L1 return DMA uses the 0x82... block perspective and the block
        // wrapper selects DSPM for the entire architectural gap from
        // BLOCK_DSPM_OFFSET to BLOCK_VSPM_OFFSET while the tile is idle.
        // ram_model then casts the word address to the implemented SRAM's
        // address width.  For this configuration the 16 KiB DSPM therefore
        // mirrors throughout that gap.  Model that structural aliasing,
        // including a transfer which crosses the physical wrap point.
        size_t copied = 0;
        while (copied < size) {
            const Addr offset =
                (addr + copied - RtlDspmOffset) % RtlDspmBytes;
            const size_t chunk =
                std::min(size - copied,
                         static_cast<size_t>(RtlDspmBytes - offset));
            SETranslatingPortProxy(tc).readBlob(
                BlockPerspectiveBase | (RtlDspmOffset + offset),
                data.data() + copied, chunk);
            copied += chunk;
        }
    } else {
        // This path is for a future descriptor form which preserves a
        // cluster-L2 offset instead of the current block-local low 21 bits.
        runtime.sequencer->readSharedL2(addr, size, data.data());
    }
    return data;
}

void
VenusDagScheduler::writeTile(unsigned task, Addr addr,
                             const std::vector<uint8_t> &data)
{
    auto &runtime = runtimeForTask(task);
    ThreadContext *tc = contextForTask(task);
    if (isVrfAddress(addr)) {
        runtime.vrf->backdoor_WriteVspm(addr, data.size(), data.data());
    } else if (addr < RtlDspmOffset) {
        // See readTile(): every 32-KiB ISPM aperture aliases physical ISPM.
        size_t copied = 0;
        while (copied < data.size()) {
            const Addr offset = (addr + copied - RtlIspmOffset) % RtlIspmBytes;
            const size_t chunk = std::min(
                data.size() - copied,
                static_cast<size_t>(RtlIspmBytes - offset));
            SETranslatingPortProxy(tc).writeBlob(
                BlockPerspectiveBase | (RtlIspmOffset + offset),
                data.data() + copied, chunk);
            copied += chunk;
        }
    } else if (addr < RtlVspmOffset) {
        // See readTile(): every 16-KiB DSPM aperture aliases physical DSPM.
        size_t copied = 0;
        while (copied < data.size()) {
            const Addr offset =
                (addr + copied - RtlDspmOffset) % RtlDspmBytes;
            const size_t chunk = std::min(
                data.size() - copied,
                static_cast<size_t>(RtlDspmBytes - offset));
            SETranslatingPortProxy(tc).writeBlob(
                BlockPerspectiveBase | (RtlDspmOffset + offset),
                data.data() + copied, chunk);
            copied += chunk;
        }
    } else {
        SETranslatingPortProxy(tc).writeBlob(
            BlockPerspectiveBase | (addr & 0x1fffff), data.data(), data.size());
    }
}

bool
VenusDagScheduler::dependenciesReady(unsigned task) const
{
    for (const auto &input : inputs) {
        if (input.task != task)
            continue;
        const auto slot = dmt.find({input.parent, input.port});
        // RTL's DMT write flag is per return port. In the completion-driven
        // path a consumer may become eligible as soon as that port commits;
        // it must not wait for unrelated returns or the producer tile release.
        if ((!useL1TimingDma && states[input.parent] != TaskState::Complete) ||
            slot == dmt.end() || !slot->second.complete)
            return false;
    }
    return true;
}

bool
VenusDagScheduler::allComplete() const
{
    return std::all_of(states.begin(), states.end(), [](TaskState state) {
        return state == TaskState::Complete;
    });
}

void
VenusDagScheduler::loadInputs(unsigned task)
{
    // Model the L1 scheduler's per-dispatch code/data DMA.  All contexts map
    // these local addresses to the same tile SRAMs, so dispatching a task
    // replaces the previous tile contents exactly as RTL does.
    if (!taskImages[task].code.empty())
        writeTile(task, 0, taskImages[task].code);
    if (!taskImages[task].data.empty())
        writeTile(task, 0x20000, taskImages[task].data);
    for (const auto &input : initialInputs) {
        if (input.task != task)
            continue;
        writeTile(task, input.destination, input.data);
        std::ostringstream extra;
        extra << "\"source_kind\":\"global\",\"destination\":\"0x"
              << std::hex << input.destination << "\",\"bytes\":"
              << std::dec << input.data.size();
        emit("dma_input", task, extra.str());
    }
    for (const auto &input : inputs) {
        if (input.task != task)
            continue;
        const auto &slot = dmt.at({input.parent, input.port});
        fatal_if(!slot.complete,
                 "DMT input for task %d from %d:%d was not published",
                 task, input.parent, input.port);
        size_t transferSize = 0;
        if (input.type == 0) {
            // Venus1 task_manager reads the producer's 16-bit DMT input_len.
            // The consumer C capacity is advisory; it is not in the RTL task
            // descriptor. Never copy or zero-fill a static capacity tail.
            transferSize = rtlRuntimeDmtLength
                ? (slot.validBytes & 0xffffu)
                : (input.length ? input.length : slot.validBytes);
            warn_if(rtlRuntimeDmtLength && slot.validBytes > 0xffffu,
                    "DMT return length %u is truncated to RTL input_len[15:0]",
                    slot.validBytes);
            fatal_if(transferSize > slot.bytes.size(),
                     "DMT edge size exceeds available backing bytes for task %d "
                     "input from %d:%d: %u > %u",
                     task, input.parent, input.port,
                     static_cast<unsigned>(transferSize),
                     static_cast<unsigned>(slot.bytes.size()));
            std::vector<uint8_t> payload(slot.bytes.begin(),
                                         slot.bytes.begin() + transferSize);
            writeTile(task, input.destination, payload);
        } else {
            // task_manager_wrapper.sv exposes ptr_temp as one 512-bit word:
            // low 16 bits are length, followed by the 32-bit shared address.
            std::vector<uint8_t> pointer = input.pointerData;
            fatal_if(pointer.empty(),
                     "ptr_temp %d:%d -> task %d has no L1/DMT allocation metadata",
                     input.parent, input.port, task);
            writeTile(task, input.destination, pointer);
            // The producer return publication has already updated the
            // dynamic prefix of this L2 slot.  A type-4 fire only transfers
            // the 64-byte pointer record; the task's LDU follows it later.
        }
        std::ostringstream extra;
        extra << "\"parent_task\":" << input.parent
              << ",\"parent_port\":" << input.port
              << ",\"destination\":\"0x" << std::hex
              << input.destination << "\",\"bytes\":" << std::dec
              << (input.type == 0
                      ? transferSize
                      : input.pointerData.size());
        if (input.type == 0)
            extra << ",\"valid_bytes\":" << slot.validBytes
                  << ",\"slot_capacity\":" << slot.consumerBytes;
        emit("dma_input", task, extra.str());
    }
}

void
VenusDagScheduler::publishOutput(unsigned task, unsigned port,
                                 const std::vector<uint8_t> &data)
{
    auto &slot = dmt[{task, port}];
    fatal_if(slot.complete, "task %d published DMT output port %d twice",
             task, port);
    // The static DMT/fire field is the consumer's transfer extent, not the
    // producer return allocation.  For example, Task20 returns 4096 bytes
    // while two later tasks each consume only its first 128 bytes.  Preserve
    // the full dynamically returned range, growing the backing store above
    // the seeded consumer extent when necessary.
    if (!slot.consumerBytes)
        slot.consumerBytes = static_cast<uint32_t>(data.size());
    // The shared-L2 image at startup is only a time-zero snapshot.  DMT
    // addresses can be reused by a prior return, so a short later return
    // must retain the tail that exists *now*, not a stale startup tail.
    // Read that fixed consumer range before overwriting its dynamic prefix.
    if (slot.hasL2Address && data.size() < slot.consumerBytes) {
        if (slot.bytes.size() < slot.consumerBytes)
            slot.bytes.resize(slot.consumerBytes, 0);
        runtimeForTask(task).sequencer->readSharedL2(
            slot.l2Address, slot.consumerBytes, slot.bytes.data());
    }
    if (slot.bytes.size() < data.size())
        slot.bytes.resize(data.size(), 0);
    std::copy(data.begin(), data.end(), slot.bytes.begin());
    slot.validBytes = data.size();
    slot.complete = true;
    // A task return is a real L2 DMA of its dynamic length.  Do not truncate
    // this write to a later consumer's fixed fire-DMA length.
    if (slot.hasL2Address && !data.empty())
        runtimeForTask(task).sequencer->writeSharedL2(
            slot.l2Address, data.size(), data.data());
}

void
VenusDagScheduler::captureOutputs(unsigned task)
{
    ThreadContext *tc = contextForTask(task);
    if (!outputCounts.empty()) {
        uint32_t count = 0;
        SETranslatingPortProxy(tc).readBlob(TileManagerBase + 0x2c,
                                            &count, sizeof(count));
        fatal_if(count > 16, "task %d wrote invalid return count %d", task, count);
        fatal_if(count != outputCounts[task],
                 "task %d returned %d values, RTL descriptor expects %d",
                 task, count, outputCounts[task]);
        for (unsigned port = 0; port < count; ++port) {
            uint32_t source = 0, length = 0;
            SETranslatingPortProxy(tc).readBlob(
                TileManagerBase + 0x30 + port * 8, &source, sizeof(source));
            SETranslatingPortProxy(tc).readBlob(
                TileManagerBase + 0x34 + port * 8, &length, sizeof(length));
            source &= 0x1fffff;
            auto data = readTile(task, source, length);
            dumpOutput(task, port, data);
            publishOutput(task, port, data);
            const auto &slot = dmt.at({task, port});
            std::ostringstream extra;
            extra << "\"output_port\":" << port
                  << ",\"source\":\"0x" << std::hex << source
                  << "\",\"bytes\":" << std::dec << length
                  << ",\"slot_capacity\":" << slot.consumerBytes;
            emit("dma_output", task, extra.str());
        }
        return;
    }
    for (const auto &output : outputs) {
        if (output.task != task)
            continue;
        auto data = readTile(task, output.source, output.length);
        dumpOutput(task, output.port, data);
        publishOutput(task, output.port, data);
        const auto &slot = dmt.at({task, output.port});
        std::ostringstream extra;
        extra << "\"output_port\":" << output.port
              << ",\"source\":\"0x" << std::hex << output.source
              << "\",\"bytes\":" << std::dec << output.length
              << ",\"slot_capacity\":" << slot.consumerBytes;
        emit("dma_output", task, extra.str());
    }
}

void
VenusDagScheduler::dumpOutput(unsigned task, unsigned port,
                              const std::vector<uint8_t> &data) const
{
    if (outputDumpDir.empty())
        return;

    std::ostringstream path;
    path << outputDumpDir << "/task_" << task << "_port_" << port << ".bin";
    std::ofstream stream(path.str(), std::ios::out | std::ios::binary |
                         std::ios::trunc);
    fatal_if(!stream, "cannot open Venus DAG output dump %s", path.str());
    stream.write(reinterpret_cast<const char *>(data.data()), data.size());
    fatal_if(!stream, "cannot write Venus DAG output dump %s", path.str());
    if (!autostart) {
        std::ostringstream instance;
        instance << outputDumpDir << "/fire_" << firmwareFireId
                 << "_task_" << task << "_port_" << port << ".bin";
        std::ofstream perFire(instance.str(), std::ios::binary | std::ios::trunc);
        perFire.write(reinterpret_cast<const char *>(data.data()), data.size());
        fatal_if(!perFire, "cannot write per-fire output %s", instance.str());
    }
}

void
VenusDagScheduler::restoreTileState(unsigned task)
{
    auto &runtime = runtimeForTask(task);
    ThreadContext *tc = contextForTask(task);
    // RTL asserts tile_soft_reset_n around every allocation, resetting the
    // sequencer, Shuffle engine and VRF arbitration phase together.  This is
    // deliberately controlled by a backend property so a Venus1 correction
    // cannot silently perturb protected Venus2 qualification profiles.
    runtime.sequencer->resetRtlTaskState();
    if (singleResourceFallback) {
        auto &tile = tileStates.at(taskTileIds.at(task));
        SETranslatingPortProxy(tc).writeBlob(
            BlockPerspectiveBase, tile.spm.data(), tile.spm.size());
        runtime.vrf->backdoor_Write(
            VspmPhysicalBase, tile.vspm.size(), tile.vspm.data());
    }

    // A dispatched task sees reset tile-manager/control registers. With real
    // tile instances this is one physical control page per tile; with the
    // legacy fallback it preserves the same task-start invariant.
    std::vector<uint8_t> resetControl(0x1000, 0);
    SETranslatingPortProxy(tc).writeBlob(
        TileManagerBase, resetControl.data(), resetControl.size());

    // tile_start_exec_i1 drives tile_soft_reset_n high in RTL, so every
    // task begins with a reset scalar/Venus control plane even when it is
    // placed on the same physical tile as its predecessor.  Scratchpad RAM
    // is restored above, but CSR state must not cross the task boundary.
    // scalar600_id_stage resets CSR0..4 to 0, 0, '1, 0x8006, and 3; the
    // MinorCPU model stores the architectural 12-bit CSR values.
    runtime.minorCpu->setMulshamt(0);
    runtime.minorCpu->setMsbhead(0);
    runtime.minorCpu->setMulsaturate(MinorCPU::VenusCsrMask);
    runtime.minorCpu->setLsuAddrMsb(0x006);
    runtime.minorCpu->setVenusCsr(4, 3);
    for (unsigned csr = 5; csr < MinorCPU::VenusCsrNum; ++csr)
        runtime.minorCpu->setVenusCsr(csr, 0);
}

void
VenusDagScheduler::saveTileState(unsigned task)
{
    if (!singleResourceFallback)
        return;
    auto &runtime = runtimeForTask(task);
    ThreadContext *tc = contextForTask(task);
    auto &tile = tileStates.at(taskTileIds.at(task));
    SETranslatingPortProxy(tc).readBlob(
        BlockPerspectiveBase, tile.spm.data(), tile.spm.size());
    runtime.vrf->backdoor_Read(
        VspmPhysicalBase, tile.vspm.size(), tile.vspm.data());
}

void
VenusDagScheduler::scheduleProtocol(Cycles delay,
                                    std::function<void()> action)
{
    fatal_if(!useL1TimingDma,
             "scheduled a responder phase while timing DMA is disabled");
    fatal_if(protocolEvent.scheduled(),
             "Venus L1 responder scheduled two protocol phases at once");
    fatal_if(!action, "Venus L1 responder scheduled an empty protocol phase");
    protocolAction = std::move(action);
    schedule(protocolEvent, afterCycles(delay));
}

void
VenusDagScheduler::runProtocol()
{
    fatal_if(!protocolAction,
             "Venus L1 responder protocol event has no pending action");
    auto action = std::move(protocolAction);
    protocolAction = {};
    action();
}

bool
VenusDagScheduler::tileBusy(unsigned tile) const
{
    const auto &runtime = tileRuntimes.at(tile);
    return runtime.allocatedTask >= 0 || runtime.runningTask >= 0;
}

bool
VenusDagScheduler::timingDmaHasWork() const
{
    return timingDmaActive || timingResponderBusy ||
           l1DmaEngine->isBusy() || protocolEvent.scheduled() ||
           !inboundDmaQueue.empty() || !outboundDmaQueue.empty();
}

Addr
VenusDagScheduler::allocateDmtAddress(uint32_t reserveBytes)
{
    constexpr Addr Alignment = 64;
    constexpr Addr PtrReserve = VenusSharedL2::PtrGlobalOffset;
    const Addr rounded = (Addr(reserveBytes) + Alignment - 1) &
                         ~(Alignment - 1);
    const Addr result = nextDmtAddress;
    fatal_if(result > PtrReserve || rounded > PtrReserve - result,
             "legacy DMT allocator overlaps pointer-responder registers");
    fatal_if(result > sharedL2Object->size() ||
             rounded > sharedL2Object->size() - result,
             "legacy DMT allocator exceeds shared-L2 capacity");
    nextDmtAddress += rounded;
    return result;
}

std::vector<uint8_t>
VenusDagScheduler::makeDmtPointer(const DmtSlot &slot) const
{
    fatal_if(!slot.hasL2Address,
             "ptr_temp requested before its runtime-DAG DMT address exists");
    fatal_if(slot.consumerBytes > 0xffffu,
             "DMT pointer capacity %u does not fit RTL's 16-bit input_len",
             slot.consumerBytes);
    fatal_if(slot.l2Address > 0xffffffffULL,
             "DMT pointer address %#llx does not fit RTL's 32-bit field",
             static_cast<unsigned long long>(slot.l2Address));
    std::vector<uint8_t> pointer(VenusSharedL2::PointerRecordBytes, 0);
    pointer[0] = static_cast<uint8_t>(slot.consumerBytes & 0xff);
    pointer[1] = static_cast<uint8_t>((slot.consumerBytes >> 8) & 0xff);
    for (unsigned byte = 0; byte < 4; ++byte)
        pointer[2 + byte] = static_cast<uint8_t>(
            (slot.l2Address >> (byte * 8)) & 0xff);
    return pointer;
}

VenusDagScheduler::TimingDmaItem
VenusDagScheduler::makeTimingFire(const TimingFire &fire) const
{
    TimingDmaItem item{
        TimingDmaKind::Inbound, fire.task, fire.ordinal, 0,
        // Keep the logical tile-local target until the responder actually
        // accepts this fire.  RTL latches target_tile_idx in its CACHE/
        // RESPOND path, after an earlier tile's same-edge writeback/release
        // has updated occupancy; selecting it while merely queueing the
        // descriptor wrongly steals the next-lowest idle tile.
        fire.inputType, 0, fire.destination,
        0, false, 0, {},
    };
    const auto slotFor = [&]() -> const DmtSlot & {
        const auto it = dmt.find({fire.parent, fire.port});
        fatal_if(it == dmt.end(),
                 "v5 fire task %d ordinal %d references missing DMT %d:%d",
                 fire.task, fire.ordinal, fire.parent, fire.port);
        return it->second;
    };
    switch (fire.sourceKind) {
      case FireSource::SharedL2:
        fatal_if(fire.sourceOffset > sharedL2Object->size() ||
                 fire.length > sharedL2Object->size() - fire.sourceOffset,
                 "v5 shared-L2 fire task %d ordinal %d exceeds capacity",
                 fire.task, fire.ordinal);
        item.source = SharedL2PhysicalBase + fire.sourceOffset;
        item.bytes = fire.length;
        break;
      case FireSource::DmtReturn: {
        const auto &slot = slotFor();
        fatal_if(!slot.complete || !slot.hasL2Address,
                 "v5 DMT fire task %d ordinal %d precedes %d:%d commit",
                 fire.task, fire.ordinal, fire.parent, fire.port);
        fatal_if(fire.sourceOffset && fire.sourceOffset != slot.l2Address,
                 "v5 DMT fire task %d ordinal %d has address %#llx, expected %#llx",
                 fire.task, fire.ordinal,
                 static_cast<unsigned long long>(fire.sourceOffset),
                 static_cast<unsigned long long>(slot.l2Address));
        item.source = SharedL2PhysicalBase + slot.l2Address;
        item.bytes = rtlRuntimeDmtLength
            ? (slot.validBytes & 0xffffu)
            : (fire.length ? fire.length : slot.consumerBytes);
        if (!rtlRuntimeDmtLength && !item.bytes)
            item.bytes = slot.validBytes;
        warn_if(rtlRuntimeDmtLength && slot.validBytes > 0xffffu,
                "DMT return length %u is truncated to RTL input_len[15:0]",
                slot.validBytes);
        fatal_if(!rtlRuntimeDmtLength &&
                 item.bytes > slot.consumerBytes && slot.consumerBytes != 0,
                 "v5 DMT fire task %d ordinal %d exceeds static DMT capacity",
                 fire.task, fire.ordinal);
        break;
      }
      case FireSource::PtrTemp: {
        const auto &slot = slotFor();
        fatal_if(!slot.complete,
                 "v5 ptr_temp fire task %d ordinal %d precedes %d:%d commit",
                 fire.task, fire.ordinal, fire.parent, fire.port);
        fatal_if(fire.sourceOffset && fire.sourceOffset != slot.l2Address,
                 "v5 ptr_temp fire task %d ordinal %d has address %#llx, expected %#llx",
                 fire.task, fire.ordinal,
                 static_cast<unsigned long long>(fire.sourceOffset),
                 static_cast<unsigned long long>(slot.l2Address));
        item.source = SharedL2PhysicalBase + VenusSharedL2::PtrTempOffset;
        item.bytes = VenusSharedL2::PointerRecordBytes;
        item.pointerRecord = makeDmtPointer(slot);
        break;
      }
      case FireSource::PtrGlobal:
        fatal_if(fire.pointerBytes > 0xffffu ||
                 fire.sourceOffset > 0xffffffffULL,
                 "v5 global pointer fire task %d ordinal %d does not fit RTL format",
                 fire.task, fire.ordinal);
        item.source = SharedL2PhysicalBase + VenusSharedL2::PtrGlobalOffset;
        item.bytes = VenusSharedL2::PointerRecordBytes;
        item.pointerRecord.assign(VenusSharedL2::PointerRecordBytes, 0);
        item.pointerRecord[0] = static_cast<uint8_t>(fire.pointerBytes & 0xff);
        item.pointerRecord[1] = static_cast<uint8_t>(
            (fire.pointerBytes >> 8) & 0xff);
        for (unsigned byte = 0; byte < 4; ++byte)
            item.pointerRecord[2 + byte] = static_cast<uint8_t>(
                (fire.sourceOffset >> (byte * 8)) & 0xff);
        break;
    }
    fatal_if(item.bytes == 0,
             "v5 fire task %d ordinal %d has a zero-byte DMA", fire.task,
             fire.ordinal);
    return item;
}

void
VenusDagScheduler::startTimingTask(unsigned task)
{
    fatal_if(timingFires.at(task).empty(),
             "v5 task %d has no inbound fire plan", task);
    states[task] = TaskState::DmaIn;
    enqueueNextTimingFire(task, false);
}

void
VenusDagScheduler::enqueueNextTimingFire(unsigned task, bool continuation)
{
    const unsigned index = nextTimingFire.at(task);
    const auto &fires = timingFires.at(task);
    fatal_if(index >= fires.size(),
             "task %d has no remaining timing fire to enqueue", task);
    TimingDmaItem item = makeTimingFire(fires[index]);
    item.finalInboundFire = index + 1 == fires.size();
    item.admitDelayCycles = static_cast<unsigned>(
        continuation ? fireContinueAdmitCycles : fireInitialAdmitCycles);
    inboundDmaQueue.push_back(std::move(item));
    maybeStartTimingDma();
}

void
VenusDagScheduler::maybeStartTimingDma()
{
    if (!useL1TimingDma || timingDmaActive || timingResponderBusy ||
        protocolEvent.scheduled() || l1DmaEngine->isBusy())
        return;
    if (inboundDmaQueue.empty() && outboundDmaQueue.empty())
        return;

    // The RTL distributor arbiter gives a live tile writeback responder
    // priority over a new DAG fire when both contend for its one DMA channel.
    TimingDmaItem item = !outboundDmaQueue.empty()
        ? std::move(outboundDmaQueue.front())
        : std::move(inboundDmaQueue.front());
    if (!outboundDmaQueue.empty())
        outboundDmaQueue.pop_front();
    else
        inboundDmaQueue.pop_front();
    fatal_if(item.admitDelayCycles == 0,
             "v5 responder queued a zero-cycle DMA admission");
    timingDmaActive = true;
    timingResponderBusy = true;
    // The RTL freezes the allocator's selected target at the CACHE edge,
    // then asserts the DMA and tile-allocation request on the following
    // RESPOND_ACK edge.  In particular, do not choose at task-ready time:
    // a preceding tile can become idle during the responder pipeline.
    // taskTileIds is the frozen CACHE register in the dynamic model.
    if (dynamicTileAssignment && item.kind == TimingDmaKind::Inbound &&
        taskTileIds.at(item.task) == UnassignedTile) {
        const Cycles cacheDelay(item.admitDelayCycles -
            fireCacheToAdmitCycles);
        scheduleProtocol(cacheDelay, [this, item] {
            const unsigned tile = selectTileForTask(item.task);
            emit("tile_cache", item.task,
                 "\"tile_id\":" + std::to_string(tile));
            scheduleProtocol(fireCacheToAdmitCycles, [this, item] {
                startTimingDma(item);
            });
        });
        return;
    }
    scheduleProtocol(Cycles(item.admitDelayCycles), [this, item] {
        startTimingDma(item);
    });
}

void
VenusDagScheduler::startTimingDma(const TimingDmaItem &item)
{
    fatal_if(!timingDmaActive || !timingResponderBusy || l1DmaEngine->isBusy(),
             "v5 responder started DMA outside an active protocol phase");
    TimingDmaItem resolved = item;
    if (resolved.kind == TimingDmaKind::Inbound) {
        const unsigned tile = selectTileForTask(resolved.task);
        // A task can contain several inbound fires.  The first accepted fire
        // claims the tile in onTimingDmaAdmit(); every later fire for that
        // same task must be allowed to use that already-claimed tile.  What
        // the RTL allocator forbids here is a claim belonging to *another*
        // task, not the continuation of the descriptor being admitted.
        const auto &runtime = tileRuntimes.at(tile);
        fatal_if((runtime.allocatedTask >= 0 &&
                  runtime.allocatedTask != static_cast<int>(resolved.task)) ||
                 (runtime.runningTask >= 0 &&
                  runtime.runningTask != static_cast<int>(resolved.task)),
                 "task %d selected tile %d at RTL fire admission while it is "
                 "owned by allocated task %d / running task %d",
                 resolved.task, tile, runtime.allocatedTask,
                 runtime.runningTask);
        resolved.destination = tileBusAddress(
            resolved.task,
            tileDmaDestination(resolved.task, resolved.destination));
    }
    l1DmaEngine->startTransfer(
        resolved.source, resolved.destination, resolved.bytes,
        [this, resolved] { onTimingDmaAdmit(resolved); },
        [this, resolved](const std::vector<uint8_t> &data) {
            onTimingDmaComplete(resolved, data);
        },
        [this](Addr source, Addr destination, size_t naturalBytes) {
            return tileDmaBeatLimit(source, destination, naturalBytes);
        });
}

void
VenusDagScheduler::onTimingDmaAdmit(const TimingDmaItem &item)
{
    if (item.kind == TimingDmaKind::Inbound) {
        auto &runtime = runtimeForTask(item.task);
        const unsigned tile = assignedTile(item.task);
        if (runtime.allocatedTask < 0) {
            runtime.allocatedTask = item.task;
            tileLastCrc.at(tile) = taskCrcs.at(item.task);
            restoreTileState(item.task);
            emit("tile_allocated", item.task,
                 "\"tile_id\":" + std::to_string(tile));
            emit("tile_allocate", item.task,
                 "\"tile_id\":" + std::to_string(tile));
        }
        fatal_if(runtime.allocatedTask != static_cast<int>(item.task),
                 "task %d inbound DMA admitted on tile %d allocated to task %d",
                 item.task, tile, runtime.allocatedTask);
        if (!item.pointerRecord.empty()) {
            const bool accepted = item.inputType == 4
                ? sharedL2Object->enqueuePtrTemp(item.pointerRecord.data(),
                                                  item.pointerRecord.size())
                : sharedL2Object->enqueuePtrGlobal(item.pointerRecord.data(),
                                                    item.pointerRecord.size());
            fatal_if(!accepted,
                     "v5 pointer responder FIFO backpressured task %d fire %d",
                     item.task, item.ordinal);
        }
        std::ostringstream extra;
        extra << "\"tile_id\":" << tile
              << ",\"ordinal\":" << item.ordinal
              << ",\"input_type\":" << unsigned(item.inputType)
              << ",\"source\":\"0x" << std::hex << item.source
              << "\",\"destination\":\"0x" << item.destination
              << "\",\"bytes\":" << std::dec << item.bytes;
        emit("fire_admit", item.task, extra.str());

        ++nextTimingFire[item.task];
        if (item.finalInboundFire) {
            fatal_if(nextDispatchTask != item.task,
                     "task %d final fire admitted with dispatch cursor at %d",
                     item.task, nextDispatchTask);
            ++nextDispatchTask;
            emit("task_cursor_advanced", item.task,
                 "\"next_task\":" + std::to_string(nextDispatchTask));
            if (!stepEvent.scheduled())
                schedule(stepEvent, afterCycles(pollCycles));
        }
        return;
    }

    std::ostringstream extra;
    extra << "\"tile_id\":" << taskTileIds.at(item.task)
          << ",\"retid\":" << item.outputPort
          << ",\"source\":\"0x" << std::hex << item.source
          << "\",\"destination\":\"0x" << item.destination
          << "\",\"bytes\":" << std::dec << item.bytes;
    emit("return_admit", item.task, extra.str());
}

void
VenusDagScheduler::activateTimingTask(unsigned task)
{
    auto &runtime = runtimeForTask(task);
    const unsigned tile = taskTileIds.at(task);
    fatal_if(runtime.allocatedTask != static_cast<int>(task) ||
             runtime.runningTask >= 0,
             "task %d cannot start on tile %d in its current lifecycle state",
             task, tile);
    states[task] = TaskState::Running;
    runtime.runningTask = task;
    runtime.executionCompleteTick = 0;
    functionalStartTicks.at(task) = curTick();
    runtime.sequencer->setActiveThreadContext(contextForTask(task), task);
    runtime.minorCpu->resetVenusBarrierBusySynchronizer();
    if (resetTaskPipelineBeforeFire) {
        std::unique_ptr<PCStateBase> resetPc(
            contextForTask(task)->pcState().clone());
        resetPc->set(taskResetVector);
        contextForTask(task)->pcState(*resetPc);
        runtime.minorCpu->prepareVenusTaskContext(
            taskContextIds.at(task));
    }
    emit("tile_started", task, "\"tile_id\":" + std::to_string(tile));
    emit("tile_start", task, "\"tile_id\":" + std::to_string(tile));
    contextForTask(task)->activate();

    // FIRE_UP_TILE is followed by five RTL cooling states before the shared
    // distributor can arbitrate another request.  Tile execution itself is
    // intentionally concurrent with this scheduler-side cooldown.
    scheduleProtocol(responderCoolingCycles, [this] {
        timingResponderBusy = false;
        maybeStartTimingDma();
        if (!stepEvent.scheduled())
            schedule(stepEvent, afterCycles(pollCycles));
    });
}

void
VenusDagScheduler::onTimingDmaComplete(const TimingDmaItem &item,
                                       const std::vector<uint8_t> &data)
{
    fatal_if(!timingDmaActive,
             "v5 responder received DMA completion with no active transfer");
    timingDmaActive = false;
    if (item.kind == TimingDmaKind::Inbound) {
        std::ostringstream extra;
        extra << "\"tile_id\":" << taskTileIds.at(item.task)
              << ",\"ordinal\":" << item.ordinal
              << ",\"bytes\":" << data.size();
        emit("fire_dma_complete", item.task, extra.str());
        if (item.finalInboundFire) {
            scheduleProtocol(fireCompleteToStartCycles, [this, task = item.task] {
                activateTimingTask(task);
            });
        } else {
            timingResponderBusy = false;
            enqueueNextTimingFire(item.task, true);
        }
        return;
    }

    std::ostringstream extra;
    extra << "\"tile_id\":" << taskTileIds.at(item.task)
          << ",\"retid\":" << item.outputPort
          << ",\"bytes\":" << data.size();
    emit("return_dma_complete", item.task, extra.str());
    scheduleProtocol(returnCompleteToCommitCycles,
                     [this, task = item.task, port = item.outputPort, data] {
        completeTimingReturn(task, port, data);
    });
}

std::vector<VenusDagScheduler::Output>
VenusDagScheduler::collectOutputs(unsigned task)
{
    std::vector<Output> result;
    ThreadContext *tc = contextForTask(task);
    if (!outputCounts.empty()) {
        uint32_t count = 0;
        SETranslatingPortProxy(tc).readBlob(TileManagerBase + 0x2c,
                                            &count, sizeof(count));
        fatal_if(count > 16, "task %d wrote invalid return count %d", task, count);
        fatal_if(count != outputCounts[task],
                 "task %d returned %d values, RTL descriptor expects %d",
                 task, count, outputCounts[task]);
        for (unsigned port = 0; port < count; ++port) {
            uint32_t source = 0, length = 0;
            SETranslatingPortProxy(tc).readBlob(
                TileManagerBase + 0x30 + port * 8, &source, sizeof(source));
            SETranslatingPortProxy(tc).readBlob(
                TileManagerBase + 0x34 + port * 8, &length, sizeof(length));
            result.push_back({task, port, source & 0x1fffff, length});
        }
        return result;
    }
    for (const auto &output : outputs) {
        if (output.task == task)
            result.push_back(output);
    }
    return result;
}

void
VenusDagScheduler::beginTimingReturns(unsigned task)
{
    fatal_if(states.at(task) != TaskState::Draining,
             "task %d started timing returns outside draining state", task);
    states[task] = TaskState::DmaOut;
    timingReturns[task] = collectOutputs(task);
    nextTimingReturn[task] = 0;
    if (timingReturns[task].empty()) {
        timingResponderBusy = true;
        scheduleProtocol(returnCommitToReleaseCycles, [this, task] {
            finishTimingTask(task);
        });
        return;
    }
    enqueueNextTimingReturn(task);
}

void
VenusDagScheduler::enqueueNextTimingReturn(unsigned task)
{
    const unsigned index = nextTimingReturn.at(task);
    const auto &returns = timingReturns.at(task);
    fatal_if(index >= returns.size(),
             "task %d has no remaining timing return to enqueue", task);
    const Output &output = returns[index];
    const auto slotIt = dmt.find({task, output.port});
    fatal_if(slotIt == dmt.end(),
             "task %d return port %d has no runtime-DAG DMT slot",
             task, output.port);
    const auto &slot = slotIt->second;
    fatal_if(!slot.hasL2Address,
             "task %d return port %d has no DMT destination address",
             task, output.port);
    /*
     * consumerBytes is backing metadata (a legacy type-0 fire extent and
     * type-4 pointer capacity), not a producer return limit. RTL DMA
     * writes the producer's dynamic return length even when it is larger
     * than every consumer prefix; publishOutput() follows the same rule in
     * the non-timing path.
     */
    TimingDmaItem item{
        TimingDmaKind::Return, task, index, output.port, 255,
        tileBusAddress(task, output.source),
        SharedL2PhysicalBase + slot.l2Address, output.length, false,
        static_cast<unsigned>(index == 0 ? returnInitialAdmitCycles
                                         : returnContinueAdmitCycles), {},
    };
    outboundDmaQueue.push_back(std::move(item));
    maybeStartTimingDma();
}

void
VenusDagScheduler::completeTimingReturn(unsigned task, unsigned port,
                                        const std::vector<uint8_t> &data)
{
    auto slotIt = dmt.find({task, port});
    fatal_if(slotIt == dmt.end(),
             "task %d completed unknown DMT port %d", task, port);
    auto &slot = slotIt->second;
    fatal_if(slot.complete,
             "task %d completed DMT port %d twice", task, port);
    if (!slot.hasL2Address) {
        slot.consumerBytes = std::max(slot.consumerBytes,
                                      static_cast<uint32_t>(data.size()));
        slot.l2Address = allocateDmtAddress(slot.consumerBytes);
        slot.hasL2Address = true;
    }
    const size_t snapshotBytes = std::max<size_t>(slot.consumerBytes, data.size());
    slot.bytes.resize(snapshotBytes, 0);
    if (snapshotBytes)
        sharedL2Object->read(slot.l2Address, snapshotBytes, slot.bytes.data());
    slot.validBytes = data.size();
    slot.complete = true;
    dumpOutput(task, port, data);

    std::ostringstream extra;
    extra << "\"retid\":" << port
          << ",\"address\":\"0x" << std::hex << slot.l2Address
          << "\",\"valid_bytes\":" << std::dec << slot.validBytes
          << ",\"slot_capacity\":" << slot.consumerBytes;
    emit("dmt_commit", task, extra.str());
    ++nextTimingReturn[task];
    if (!stepEvent.scheduled())
        schedule(stepEvent, afterCycles(pollCycles));

    if (nextTimingReturn[task] < timingReturns[task].size()) {
        timingResponderBusy = false;
        enqueueNextTimingReturn(task);
        return;
    }
    scheduleProtocol(returnCommitToReleaseCycles, [this, task] {
        finishTimingTask(task);
    });
}

void
VenusDagScheduler::finishTimingTask(unsigned task)
{
    auto &runtime = runtimeForTask(task);
    fatal_if(states.at(task) != TaskState::DmaOut ||
             runtime.runningTask != static_cast<int>(task),
             "task %d released outside timing-DMA output state", task);
    saveTileState(task);
    runtime.minorCpu->resetVenusBarrierBusySynchronizer();
    runtime.sequencer->resetRtlScalarBarrierState();
    updateModeledTimeline(task);
    states[task] = TaskState::Complete;
    runtime.sequencer->setActiveThreadContext(nullptr);
    runtime.runningTask = -1;
    runtime.allocatedTask = -1;
    emit("tile_released", task,
         "\"tile_id\":" + std::to_string(taskTileIds.at(task)));
    emit("tile_release", task,
         "\"tile_id\":" + std::to_string(taskTileIds.at(task)));

    scheduleProtocol(responderCoolingCycles, [this] {
        timingResponderBusy = false;
        maybeStartTimingDma();
        if (!stepEvent.scheduled())
            schedule(stepEvent, afterCycles(pollCycles));
    });
}

void
VenusDagScheduler::startTask(unsigned task)
{
    if (useL1TimingDma) {
        startTimingTask(task);
        return;
    }
    states[task] = TaskState::DmaIn;
    const unsigned tile = selectTileForTask(task);
    auto &runtime = tileRuntimes.at(tile);
    fatal_if(runtime.runningTask >= 0,
             "task %d attempted to start on busy tile %d (task %d)",
             task, tile, runtime.runningTask);
    emit("tile_allocated", task,
         "\"tile_id\":" + std::to_string(tile));
    restoreTileState(task);
    loadInputs(task);
    states[task] = TaskState::Running;
    runtime.runningTask = task;
    runtime.executionCompleteTick = 0;
    functionalStartTicks.at(task) = curTick();
    runtime.sequencer->setActiveThreadContext(contextForTask(task), task);
    runtime.minorCpu->resetVenusBarrierBusySynchronizer();
    if (resetTaskPipelineBeforeFire) {
        std::unique_ptr<PCStateBase> resetPc(
            contextForTask(task)->pcState().clone());
        resetPc->set(taskResetVector);
        contextForTask(task)->pcState(*resetPc);
        runtime.minorCpu->prepareVenusTaskContext(
            taskContextIds.at(task));
    }
    emit("tile_started", task,
         "\"tile_id\":" + std::to_string(tile));
    contextForTask(task)->activate();
}

void
VenusDagScheduler::updateModeledTimeline(unsigned task)
{
    if (!singleResourceFallback) {
        const Tick start = functionalStartTicks.at(task);
        const Tick cleanupEnd = curTick();
        const Tick executionEnd = runtimeForTask(task).executionCompleteTick
            ? runtimeForTask(task).executionCompleteTick : cleanupEnd;
        fatal_if(executionEnd < start || executionEnd > cleanupEnd,
                 "task %d has invalid execute/cleanup boundary %llu/%llu "
                 "after start %llu", task,
                 static_cast<unsigned long long>(executionEnd),
                 static_cast<unsigned long long>(cleanupEnd),
                 static_cast<unsigned long long>(start));
        modeledStartTicks.at(task) = start;
        // Dependency publication and tile reuse remain anchored to the safe
        // post-drain boundary.  Only the task execution metric is compared
        // with RTL's selected_block_clk soft-reset observer.
        modeledEndTicks.at(task) = cleanupEnd;
        modeledTileFreeTicks.at(taskTileIds.at(task)) = cleanupEnd;
        std::ostringstream extra;
        extra << "\"tile_id\":" << taskTileIds.at(task)
              << ",\"start_tick\":" << start
              << ",\"end_tick\":" << executionEnd
              << ",\"cleanup_end_tick\":" << cleanupEnd
              << ",\"measured_duration_ticks\":"
              << executionEnd - start;
        emit("measured_task_timing", task, extra.str());
        return;
    }

    Tick dependencyReady = 0;
    for (const auto &input : inputs) {
        if (input.task == task)
            dependencyReady =
                std::max(dependencyReady, modeledEndTicks.at(input.parent));
    }

    const unsigned tile = taskTileIds.at(task);
    const Tick start =
        std::max(dependencyReady, modeledTileFreeTicks.at(tile));
    const Tick duration = curTick() - functionalStartTicks.at(task);
    const Tick end = start + duration;
    modeledStartTicks.at(task) = start;
    modeledEndTicks.at(task) = end;
    modeledTileFreeTicks.at(tile) = end;

    std::ostringstream extra;
    extra << "\"tile_id\":" << tile
          << ",\"modeled_start_tick\":" << start
          << ",\"modeled_end_tick\":" << end
          << ",\"measured_duration_ticks\":" << duration;
    emit("modeled_task_timing", task, extra.str());
}

void
VenusDagScheduler::finishTask(unsigned task)
{
    auto &runtime = runtimeForTask(task);
    states[task] = TaskState::DmaOut;
    captureOutputs(task);
    saveTileState(task);
    /*
     * tile_soft_reset_n is deasserted when the RTL tile leaves a task.
     * That reset clears scalar600's barrier synchronizer and the Venus
     * sequencer's registered running-ID state, while SRAM contents retained
     * above remain available to the scheduler's output DMA.
     */
    runtime.minorCpu->resetVenusBarrierBusySynchronizer();
    runtime.sequencer->resetRtlScalarBarrierState();
    updateModeledTimeline(task);
    states[task] = TaskState::Complete;
    runtime.sequencer->setActiveThreadContext(nullptr);
    runtime.runningTask = -1;
    emit("tile_released", task,
         "\"tile_id\":" + std::to_string(taskTileIds.at(task)));
}

void
VenusDagScheduler::requestTaskExit(ThreadContext *tc)
{
    if (firmwareDelegate && firmwareDelegate != this) {
        firmwareDelegate->requestTaskExit(tc);
        return;
    }
    int task = -1;
    for (unsigned candidate = 0; candidate < taskNames.size(); ++candidate) {
        if (taskTileIds.at(candidate) == UnassignedTile)
            continue;
        auto &runtime = runtimeForTask(candidate);
        if (runtime.cpu->getContext(taskContextIds.at(candidate)) == tc) {
            task = candidate;
            break;
        }
    }
    fatal_if(task < 0, "task exit from a context not owned by the Venus DAG");
    auto &runtime = runtimeForTask(task);
    fatal_if(runtime.runningTask != task,
             "task exit from task %d while tile %d runs task %d",
             task, taskTileIds.at(task), runtime.runningTask);
    // A store can be replayed by the scalar memory pipeline after the
    // architectural task-done notification has already entered the tile
    // manager.  A later SE ebreak can also reach this path while the
    // registered notification is propagating.
    if (states[task] == TaskState::TaskDonePending ||
        states[task] == TaskState::Draining)
        return;
    fatal_if(states[task] != TaskState::Running,
             "task %d exit requested in state %d",
             task, static_cast<int>(states[task]));
    /*
     * TASK_DONE is not a combinational task-exit signal in RTL.  The task
     * epilogue writes venustile_softresetreg, while the lifecycle boundary is
     * venustile_softresetreg_z1/soft_reset_n_o1.  Some generations clock that
     * register chain with selected_block_clk rather than the scheduler clock.
     * Keep both the edge count and its owning clock backend-scoped; neither is
     * inferred from a task, PC, or DAG identity.
     */
    states[task] = TaskState::TaskDonePending;
    runtime.nextDrainTrace = taskDoneVisibilityUsesTileClock
        ? runtime.minorCpu->clockEdge(taskDoneVisibilityCycles)
        : clockEdge(taskDoneVisibilityCycles);
    if (!stepEvent.scheduled()) {
        schedule(stepEvent, runtime.nextDrainTrace);
    } else if (runtime.nextDrainTrace < stepEvent.when()) {
        reschedule(stepEvent, runtime.nextDrainTrace, true);
    }
}

void
VenusDagScheduler::step()
{
    for (auto &runtime : tileRuntimes) {
        const int task = runtime.runningTask;
        if (task < 0)
            continue;
        if (states[task] == TaskState::TaskDonePending &&
            curTick() >= runtime.nextDrainTrace) {
            states[task] = TaskState::Draining;
            runtime.nextDrainTrace = curTick();
            runtime.executionCompleteTick = curTick();
            emit("task_epilogue", task,
                 "\"execution_complete_tick\":" +
                 std::to_string(runtime.executionCompleteTick));
            runtime.minorCpu->requestVenusTaskSuspend(
                taskContextIds.at(task));
            // Do not begin output capture ahead of the same-tick CPU edge.
            continue;
        }
        if (states[task] == TaskState::Draining &&
            runtime.sequencer->isIdle() &&
            runtime.minorCpu->isTaskMemoryDrained()) {
            if (useL1TimingDma)
                beginTimingReturns(task);
            else
                finishTask(task);
            continue;
        }
        if (states[task] == TaskState::Draining &&
            curTick() >= runtime.nextDrainTrace) {
            emit("drain_progress", task, runtime.sequencer->drainStatus());
            // 10 us at gem5's default 1 ps tick resolution. This is sparse
            // enough for long vector divisions yet exposes real forward
            // progress and persistent queue stalls.
            runtime.nextDrainTrace = curTick() + 10000000;
        }
    }

    if (allComplete() && (!useL1TimingDma || !timingDmaHasWork())) {
        const Tick elapsed =
            *std::max_element(modeledEndTicks.begin(), modeledEndTicks.end());
        const char *field = singleResourceFallback
            ? "\"modeled_elapsed_ticks\":"
            : "\"functional_elapsed_ticks\":";
        emit("dag_complete", 0, std::string(field) + std::to_string(elapsed));
        fatal_if(completionNotified,
                 "Venus DAG completion was observed more than once");
        completionNotified = true;
        if (firmwareCompletionCallback) {
            firmwareCompletionCallback();
        } else {
            exitSimLoop("Venus RTL-aligned DAG completed");
        }
        return;
    }

    // L2 task_manager keeps one in-order task_pnt_q (task_manager.sv,
    // FSM_CHECK_FIRE_STATE through FSM_TASK2TASK_TRANSITION).  A later
    // descriptor must not bypass an earlier descriptor that is waiting on a
    // dependency or on its target tile.  Once the current task has been
    // dispatched, the hardware advances task_pnt and may issue the next
    // descriptor while the previous tile is running; retain that behavior by
    // walking only the contiguous dispatchable prefix.
    bool hasReady = false;
    const auto taskCanStart = [this](unsigned task) {
        if (!dynamicTileAssignment)
            return !tileBusy(assignedTile(task));
        for (unsigned tile = 0; tile < tileRuntimes.size(); ++tile) {
            int distance = 0;
            unsigned price = 0;
            if (tileFitsTask(task, tile, distance, price))
                return true;
        }
        return false;
    };
    if (useL1TimingDma) {
        // In the real task manager the cursor advances on the final fire
        // ACK, not when this host-side dispatcher merely queues a task.  A
        // live responder therefore owns exactly one descriptor until that
        // admission boundary; later tasks cannot leapfrog it.
        if (nextDispatchTask < states.size()) {
            const unsigned task = nextDispatchTask;
            if (states[task] == TaskState::Waiting) {
                if (dependenciesReady(task)) {
                    states[task] = TaskState::Ready;
                    emit("task_ready", task);
                }
            }
            if (states[task] == TaskState::Ready) {
                if (!taskCanStart(task)) {
                    hasReady = true;
                } else {
                    startTask(task);
                }
            } else if (states[task] == TaskState::DmaIn) {
                // The current descriptor remains cursor-owned until its
                // final fire ACK; its DMA is already represented above.
            } else if (states[task] != TaskState::Waiting) {
                fatal("Venus timing-DMA dispatch cursor %d reached unexpected "
                      "state %d", task, static_cast<int>(states[task]));
            }
        }
    } else {
        while (nextDispatchTask < states.size()) {
            const unsigned task = nextDispatchTask;
            if (states[task] == TaskState::Waiting) {
                if (!dependenciesReady(task))
                    break;
                states[task] = TaskState::Ready;
                emit("task_ready", task);
            }
            fatal_if(states[task] != TaskState::Ready,
                     "Venus DAG dispatch cursor %d reached unexpected state %d",
                     task, static_cast<int>(states[task]));
            if (!taskCanStart(task)) {
                hasReady = true;
                break;
            }
            startTask(task);
            ++nextDispatchTask;
        }
    }

    bool hasRunning = false;
    for (const auto &runtime : tileRuntimes)
        hasRunning = hasRunning || runtime.runningTask >= 0 ||
            runtime.allocatedTask >= 0;
    if (useL1TimingDma)
        hasRunning = hasRunning || timingDmaHasWork();
    if (!hasRunning && !hasReady)
        fatal("Venus DAG deadlocked with no running or ready task");
    schedule(stepEvent, afterCycles(pollCycles));
}

void
VenusDagScheduler::emit(const char *event, unsigned task,
                        const std::string &extra)
{
    trace << "{\"tick\":" << curTick() << ",\"event\":\"" << event
          << "\",\"task_id\":" << task;
    if (!autostart)
        trace << ",\"fire_id\":" << firmwareFireId;
    if (task < taskNames.size())
        trace << ",\"task_name\":\"" << taskNames[task] << "\"";
    if (!extra.empty())
        trace << "," << extra;
    trace << "}\n";
    trace.flush();
}

} // namespace gem5
