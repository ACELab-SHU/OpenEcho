#ifndef __VENUS_DAG_SCHEDULER_HH__
#define __VENUS_DAG_SCHEDULER_HH__

#include <cstdint>
#include <deque>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "base/types.hh"
#include "params/VenusDagScheduler.hh"
#include "sim/clocked_object.hh"
#include "sim/eventq.hh"

namespace gem5
{

class BaseCPU;
class MinorCPU;
class ThreadContext;
class VenusSequencer;
class VenusSharedL2;
class VenusL1DmaEngine;
namespace memory { class venus_vrf_mem; }

class VenusDagScheduler : public ClockedObject
{
  public:
    enum class TaskState {
        Waiting, Ready, DmaIn, Running, TaskDonePending, Draining,
        DmaOut, Complete
    };

    VenusDagScheduler(const VenusDagSchedulerParams &p);
    void startup() override;
    void requestTaskExit(ThreadContext *tc);
    struct FirmwareReturn
    {
        Addr offset;
        uint32_t length;
    };
    void startFromFirmware(uint64_t fireId = 1);
    bool matchesFirmware(Addr mallocBase) const;
    void selectFirmwareDag(VenusDagScheduler *selected);
    void setFirmwareCompletionCallback(std::function<void()> callback);
    std::vector<FirmwareReturn> firmwareReturns() const;

  private:
    struct Input {
        unsigned task;
        unsigned parent;
        unsigned port;
        Addr destination;
        uint32_t length;
        uint8_t type;
        Addr slotAddress;
        uint32_t slotConsumerBytes;
        bool hasSlotAddress;
        unsigned descriptorIndex;
        std::vector<uint8_t> pointerData;
    };
    struct Output {
        unsigned task;
        unsigned port;
        Addr source;
        uint32_t length;
    };
    struct InitialInput {
        unsigned task;
        Addr destination;
        std::vector<uint8_t> data;
    };
    struct TaskImage {
        std::vector<uint8_t> code;
        std::vector<uint8_t> data;
    };
    struct DmtSlot {
        // A DMT key names shared backing storage. Legacy profiles use a fixed
        // consumer extent; Venus1 type-0 DMA uses validBytes[15:0]. Only
        // [0, validBytes) is newly produced; backing grows when necessary.
        std::vector<uint8_t> bytes;
        uint32_t consumerBytes = 0;
        uint32_t validBytes = 0;
        bool complete = false;
        bool hasL2Address = false;
        Addr l2Address = 0;
    };
    struct TileState {
        std::vector<uint8_t> spm;
        std::vector<uint8_t> vspm;
    };
    struct TileRuntime {
        BaseCPU *cpu = nullptr;
        MinorCPU *minorCpu = nullptr;
        VenusSequencer *sequencer = nullptr;
        memory::venus_vrf_mem *vrf = nullptr;
        // RTL marks a tile busy at the first code fire, before the final
        // inbound DMA starts execution. Keep allocation distinct from a live
        // CPU context so an in-flight transfer cannot be overwritten.
        int allocatedTask = -1;
        int runningTask = -1;
        Tick nextDrainTrace = 0;
        // Falling soft_reset_n is RTL's execute-complete observation.  Gem5
        // may continue draining speculative/functional memory work before a
        // tile can safely be released, so retain both lifecycle boundaries.
        Tick executionCompleteTick = 0;
    };
    enum class FireSource : uint8_t {
        SharedL2 = 0,
        DmtReturn = 1,
        PtrTemp = 2,
        PtrGlobal = 3,
    };
    struct TimingFire {
        unsigned task;
        unsigned ordinal;
        uint8_t kind;
        uint8_t inputType;
        FireSource sourceKind;
        Addr sourceOffset;
        Addr destination;
        uint32_t length;
        unsigned parent;
        unsigned port;
        uint32_t pointerBytes;
    };
    enum class TimingDmaKind : uint8_t { Inbound, Return };
    struct TimingDmaItem {
        TimingDmaKind kind;
        unsigned task;
        unsigned ordinal;
        unsigned outputPort;
        uint8_t inputType;
        Addr source;
        Addr destination;
        uint32_t bytes;
        bool finalInboundFire;
        unsigned admitDelayCycles;
        std::vector<uint8_t> pointerRecord;
    };

    std::vector<TileRuntime> tileRuntimes;
    // Keep the old one-resource construction available for single-tile
    // diagnostic callers. A multi-tile DAG must provide one runtime per
    // physical tile; it never aliases two live tiles onto one resource.
    bool singleResourceFallback = false;
    // A runtime-DAG task has no architectural tile affinity.  The RTL
    // allocator selects an eligible idle tile at dispatch time; this marker
    // distinguishes that not-yet-selected state from physical tile zero.
    static constexpr unsigned UnassignedTile = ~0u;
    bool dynamicTileAssignment = false;
    bool useL1TimingDma = false;
    const bool rtlRuntimeDmtLength;
    const bool autostart;
    const bool firmwareSecondary;
    uint64_t firmwareFireId = 0;
    VenusDagScheduler *firmwareDelegate = nullptr;
    std::vector<std::pair<Addr, std::vector<uint8_t>>> firmwareIdentity;
    bool started = false;
    bool completionNotified = false;
    std::function<void()> firmwareCompletionCallback;
    VenusSharedL2 *sharedL2Object = nullptr;
    VenusL1DmaEngine *l1DmaEngine = nullptr;
    std::vector<std::string> taskNames;
    std::vector<TaskImage> taskImages;
    std::vector<unsigned> taskTileIds;
    std::vector<unsigned> taskContextIds;
    std::vector<uint32_t> taskHardwareRequirements;
    std::vector<uint16_t> taskCrcs;
    std::vector<bool> taskNeedSpmd;
    std::vector<unsigned> taskMinimumSpmdTasks;
    std::vector<uint32_t> tileHardwareCapabilities;
    std::vector<uint16_t> tileLastCrc;
    std::vector<int> taskLaneCompatDistances;
    std::vector<TileState> tileStates;
    std::vector<TaskState> states;
    // task_manager.sv owns one task_pnt_q for each live DAG.  It evaluates
    // only that descriptor and advances after its code/data/input fire
    // sequence is accepted; it does not search ahead for an independently
    // ready later task.  This is deliberately separate from tile occupancy.
    unsigned nextDispatchTask = 0;
    std::vector<Input> inputs;
    std::vector<InitialInput> initialInputs;
    std::vector<Output> outputs;
    std::vector<unsigned> outputCounts;
    std::map<std::pair<unsigned, unsigned>, DmtSlot> dmt;
    std::vector<std::vector<TimingFire>> timingFires;
    std::vector<unsigned> nextTimingFire;
    std::vector<std::vector<Output>> timingReturns;
    std::vector<unsigned> nextTimingReturn;
    std::deque<TimingDmaItem> inboundDmaQueue;
    std::deque<TimingDmaItem> outboundDmaQueue;
    bool timingDmaActive = false;
    bool timingResponderBusy = false;
    std::function<void()> protocolAction;
    // The RTL overall testbench programs L2_malloc from the packed runtime
    // DAG size.  v5 slots carry their generated offsets relative to this
    // base; the fallback allocator remains only for legacy manifests.
    Addr dmtMallocBase = 0;
    Addr nextDmtAddress = 0x01000000;
    // Legacy diagnostic projection. Multi-tile execution records real tile
    // lifecycle events and does not use this as a substitute for L1 timing.
    std::vector<Tick> functionalStartTicks;
    std::vector<Tick> modeledStartTicks;
    std::vector<Tick> modeledEndTicks;
    std::vector<Tick> modeledTileFreeTicks;
    Cycles pollCycles;
    Cycles fireInitialAdmitCycles;
    Cycles fireCacheToAdmitCycles;
    Cycles fireContinueAdmitCycles;
    Cycles fireCompleteToStartCycles;
    Cycles responderCoolingCycles;
    Cycles returnInitialAdmitCycles;
    Cycles returnContinueAdmitCycles;
    Cycles returnCompleteToCommitCycles;
    Cycles returnCommitToReleaseCycles;
    Cycles taskDoneVisibilityCycles;
    bool taskDoneVisibilityUsesTileClock;
    bool resetTaskPipelineBeforeFire;
    Addr taskResetVector;
    EventFunctionWrapper stepEvent;
    EventFunctionWrapper protocolEvent;
    std::ofstream trace;
    std::string outputDumpDir;

    static constexpr Addr BlockPerspectiveBase = 0x80000000;
    static constexpr Addr SharedL2PhysicalBase = 0x80000000;
    static constexpr Addr RtlTilePhysicalBase = 0x82000000;
    static constexpr Addr RtlTileStride = 0x00200000;
    static constexpr Addr RtlIspmOffset = 0x00000000;
    static constexpr Addr RtlIspmBytes = 0x00008000;
    static constexpr Addr RtlDspmOffset = 0x00020000;
    static constexpr Addr RtlDspmBytes = 0x00004000;
    static constexpr Addr RtlVspmOffset = 0x00100000;
    static constexpr Addr RtlVspmBytes = 0x00004000;
    static constexpr Addr RtlControlOffset = 0x001ff000;
    static constexpr Addr RtlControlBytes = 0x00001000;
    static constexpr Addr TileManagerBase = 0x801ff000;
    static constexpr Addr VspmPhysicalBase = 0x80100000;
    static constexpr size_t SpmSnapshotSize = 0x24000;

    // Scheduler dispatch/drain polling is expressed in scheduler-clock
    // cycles. A task-exit notification may arrive from the tile clock domain;
    // consume it on an edge of this object's own clock so a requested cycle
    // is a complete scheduler cycle rather than an arbitrary tick interval.
    // At the legacy 1 GHz single-clock configuration this preserves every
    // existing edge and therefore its tick-for-tick behavior.
    Tick afterCycles(Cycles cycles) const
    {
        return clockEdge(cycles);
    }

    void step();
    bool dependenciesReady(unsigned task) const;
    bool allComplete() const;
    void registerDmtSlot(const Input &input);
    void publishOutput(unsigned task, unsigned port,
                       const std::vector<uint8_t> &data);
    void loadInputs(unsigned task);
    void captureOutputs(unsigned task);
    void dumpOutput(unsigned task, unsigned port,
                    const std::vector<uint8_t> &data) const;
    void startTask(unsigned task);
    void finishTask(unsigned task);
    void startTimingTask(unsigned task);
    void activateTimingTask(unsigned task);
    void enqueueNextTimingFire(unsigned task, bool continuation);
    TimingDmaItem makeTimingFire(const TimingFire &fire) const;
    void maybeStartTimingDma();
    void startTimingDma(const TimingDmaItem &item);
    void onTimingDmaAdmit(const TimingDmaItem &item);
    void onTimingDmaComplete(const TimingDmaItem &item,
                             const std::vector<uint8_t> &data);
    void beginTimingReturns(unsigned task);
    void enqueueNextTimingReturn(unsigned task);
    std::vector<Output> collectOutputs(unsigned task);
    void completeTimingReturn(unsigned task, unsigned port,
                              const std::vector<uint8_t> &data);
    void finishTimingTask(unsigned task);
    std::vector<uint8_t> makeDmtPointer(const DmtSlot &slot) const;
    Addr allocateDmtAddress(uint32_t reserveBytes);
    Addr tileBusAddress(unsigned task, Addr localAddress) const;
    size_t tileDmaBeatLimit(Addr source, Addr destination,
                            size_t naturalBytes) const;
    bool timingDmaHasWork() const;
    bool tileBusy(unsigned tile) const;
    void scheduleProtocol(Cycles delay, std::function<void()> action);
    void runProtocol();
    void restoreTileState(unsigned task);
    void saveTileState(unsigned task);
    void updateModeledTimeline(unsigned task);
    unsigned selectTileForTask(unsigned task);
    unsigned assignedTile(unsigned task) const;
    bool tileFitsTask(unsigned task, unsigned tile,
                      int &laneCompatDistance, unsigned &laneCompatPrice) const;
    Addr tileDmaDestination(unsigned task, Addr localAddress) const;
    TileRuntime &runtimeForTask(unsigned task);
    const TileRuntime &runtimeForTask(unsigned task) const;
    ThreadContext *contextForTask(unsigned task);
    ThreadContext *contextForTileTask(unsigned tile, unsigned task);
    std::vector<uint8_t> readTile(unsigned task, Addr addr, size_t size);
    void writeTile(unsigned task, Addr addr, const std::vector<uint8_t> &data);
    static bool isVrfAddress(Addr addr);
    void emit(const char *event, unsigned task, const std::string &extra = "");
};

} // namespace gem5

#endif
