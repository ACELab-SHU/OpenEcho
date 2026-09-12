from m5.objects.ClockedObject import ClockedObject
from m5.params import *
from m5.proxy import Parent


class VenusDagScheduler(ClockedObject):
    type = "VenusDagScheduler"
    cxx_header = "venus/VenusDagScheduler.hh"
    cxx_class = "gem5::VenusDagScheduler"

    cpu = Param.BaseCPU(Parent.any, "MinorCPU containing one context per DAG task")
    system = Param.System(Parent.any, "System hosting the scheduler DMA fabric")
    sequencer = Param.SimObject(Parent.any, "Venus vector sequencer")
    vrf = Param.SimObject(Parent.any, "Tile-local Venus VRF")
    shared_l2_object = Param.SimObject(
        Parent.any, "Shared L2/DMT fabric used by scheduler DMA"
    )
    l1_dma_engine = Param.SimObject(
        NULL, "Completion-driven L1 scheduler DMA engine"
    )
    use_l1_timing_dma = Param.Bool(
        False, "Drive v5 fire/return transfers through the L1 DMA engine"
    )
    rtl_runtime_dmt_length = Param.Bool(
        False, "Use RTL DMT input_len from vreturn for ordinary dependencies"
    )
    firmware_secondary = Param.Bool(False, "Share the primary CPU exit router")
    firmware_identity_addresses = VectorParam.Addr([], "Staged L2 identity offsets")
    firmware_identity_files = VectorParam.String([], "Exact descriptor/code identity segments")
    autostart = Param.Bool(
        True,
        "Start at SimObject startup; firmware mode waits for the L2 reset MMIO write",
    )
    tile_cpus = VectorParam.SimObject(
        [], "One MinorCPU for each physical RTL tile"
    )
    tile_sequencers = VectorParam.SimObject(
        [], "One Venus sequencer for each physical RTL tile"
    )
    tile_vrfs = VectorParam.SimObject(
        [], "One VRF/VSPM instance for each physical RTL tile"
    )
    task_context_ids = VectorParam.UInt32(
        [], "Task-local thread-context index within its assigned tile CPU"
    )
    task_names = VectorParam.String([], "Task names indexed by task id")
    task_code_files = VectorParam.String(
        [], "RTL L1 code-DMA payload indexed by task id")
    task_data_files = VectorParam.String(
        [], "RTL L1 data-DMA payload indexed by task id")
    task_tile_ids = VectorParam.UInt32(
        [], "Optional explicit physical-tile pinning indexed by task id")
    task_hardware_requirements = VectorParam.UInt32(
        [], "Packed RTL task_hardware_requirement_task_container_t by task")
    task_crcs = VectorParam.UInt32(
        [], "RTL task-container CRC by task")
    task_need_spmd = VectorParam.Bool(
        [], "RTL task-container need_spmd bit by task")
    task_minimum_spmd_tasks = VectorParam.UInt32(
        [], "RTL task-container min_spmd_task_cnt by task")
    tile_hardware_capabilities = VectorParam.UInt32(
        [], "Packed RTL tile capability (without retained CRC) by tile")
    task_code_sources = VectorParam.Addr(
        [], "Shared-L2 offsets for task code DMA payloads"
    )
    task_data_sources = VectorParam.Addr(
        [], "Shared-L2 offsets for task data DMA payloads"
    )

    # v5 is intentionally an instruction-free transfer contract.  These
    # flattened vectors are the manifest's symbolic fire_plan, not an RTL
    # capture replay: dynamic DMT addresses are resolved by the scheduler
    # only after the corresponding return DMA commits.
    timing_fire_tasks = VectorParam.UInt32([], "v5 fire task id")
    timing_fire_ordinals = VectorParam.UInt32([], "v5 per-task fire ordinal")
    timing_fire_kinds = VectorParam.UInt32([], "0=code, 1=data, 2=input")
    timing_fire_input_types = VectorParam.UInt32([], "RTL input type, or 255")
    timing_fire_source_kinds = VectorParam.UInt32(
        [], "0=shared L2, 1=DMT return, 2=ptr temp, 3=ptr global/dfe"
    )
    timing_fire_source_offsets = VectorParam.Addr(
        [], "Shared-L2/global target offsets for v5 fires"
    )
    timing_fire_destinations = VectorParam.Addr(
        [], "Tile-local destination offsets for v5 fires"
    )
    timing_fire_lengths = VectorParam.UInt32([], "Fixed v5 fire lengths; 0=runtime")
    timing_fire_parents = VectorParam.UInt32([], "Producer task for DMT fires")
    timing_fire_ports = VectorParam.UInt32([], "Producer return port for DMT fires")
    timing_fire_pointer_bytes = VectorParam.UInt32(
        [], "Packed pointer target capacity for ptr-global/temp fires"
    )
    dmt_malloc_base = Param.Addr(
        0, "RTL L2_malloc base programmed from the runtime DAG size"
    )
    dmt_slot_tasks = VectorParam.UInt32(
        [], "Static DMT producer task for each runtime output slot"
    )
    dmt_slot_ports = VectorParam.UInt32(
        [], "Static DMT producer return port for each runtime output slot"
    )
    dmt_slot_relative_offsets = VectorParam.Addr(
        [], "DMT relative offset generated in the runtime DAG JSON"
    )
    dmt_slot_capacities = VectorParam.UInt32(
        [], "Fixed DMT input_len capacity for each runtime output slot"
    )
    # RTL responder-state timing in scheduler/AXI clock cycles.  These are
    # state-machine phase counts, shared by every DAG, never task-specific
    # replay delays.
    fire_initial_admit_cycles = Param.Unsigned(
        6, "SEARCHING through registered RESPOND_ACK before a first dag DMA"
    )
    fire_cache_to_admit_cycles = Param.Unsigned(
        1, "CACHE target latch to following RESPOND_ACK DMA allocation"
    )
    fire_continue_admit_cycles = Param.Unsigned(
        4, "DMA completion through registered DONE/RESPOND_ACK phases for a following fire"
    )
    fire_complete_to_start_cycles = Param.Unsigned(
        1, "Final inbound DMA completion to FIRE_UP_TILE"
    )
    responder_cooling_cycles = Param.Unsigned(
        5, "COOLING_1 through COOLING_5 before the responder is idle"
    )
    return_initial_admit_cycles = Param.Unsigned(
        6, "Tile request through DMT read and FIRE_UP_DMA"
    )
    return_continue_admit_cycles = Param.Unsigned(
        5, "DMT commit through following registered tile return DMA"
    )
    return_complete_to_commit_cycles = Param.Unsigned(
        1, "Return DMA completion to DMT status commit"
    )
    return_commit_to_release_cycles = Param.Unsigned(
        1, "Final DMT commit to RELEASE_TILE"
    )
    task_done_visibility_cycles = Param.Unsigned(
        2,
        "Registered task-done propagation cycles before execute-complete",
    )
    task_done_visibility_uses_tile_clock = Param.Bool(
        False,
        "Count task-done propagation on the selected tile clock",
    )
    reset_task_pipeline_before_fire = Param.Bool(
        False,
        "Apply the physical scalar soft reset before each tile fire",
    )
    task_reset_vector = Param.Addr(
        0,
        "Physical scalar reset vector used when task-pipeline reset is enabled",
    )

    input_tasks = VectorParam.UInt32([], "Consumer task for each dependency input")
    input_parents = VectorParam.UInt32([], "Producer task for each dependency input")
    input_ports = VectorParam.UInt32([], "Producer output port")
    input_destinations = VectorParam.Addr([], "Consumer tile-local destination")
    input_lengths = VectorParam.UInt32([], "Consumer fire-DMA length in bytes")
    input_types = VectorParam.UInt32([], "RTL task input type (000 temp, 100 ptr_temp)")
    input_descriptor_indices = VectorParam.UInt32(
        [], "Original per-task descriptor index for each dependency input"
    )
    input_pointer_files = VectorParam.String([], "Optional captured ptr_temp payload")
    input_slot_addresses = VectorParam.Addr(
        [], "Producer DMT/shared-L2 allocation address")
    input_slot_consumer_bytes = VectorParam.UInt32(
        [], "Static consumer extent in the producer DMT slot")
    input_slot_address_valid = VectorParam.Bool(
        [], "Whether the corresponding allocation address is known")
    initial_input_tasks = VectorParam.UInt32([], "Task for each static DMA input")
    initial_input_destinations = VectorParam.Addr([], "Static DMA destination")
    initial_input_files = VectorParam.String([], "Static DMA payload file")

    output_tasks = VectorParam.UInt32([], "Producer task for each output")
    output_ports = VectorParam.UInt32([], "Producer output port")
    output_sources = VectorParam.Addr([], "Producer tile-local source")
    output_lengths = VectorParam.UInt32([], "Output length in bytes")
    output_counts = VectorParam.UInt32([], "Expected dynamic return count per task")

    poll_cycles = Param.Unsigned(
        1, "Pipeline-drain polling interval in scheduler clock cycles")
    trace_file = Param.String("venus_dag_trace.jsonl", "RTL-shaped DAG event trace")
    output_dump_dir = Param.String(
        "", "Optional directory for byte-exact task output DMA dumps")
