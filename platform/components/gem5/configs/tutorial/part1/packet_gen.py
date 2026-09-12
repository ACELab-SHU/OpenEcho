import argparse
import hashlib
import json
import os
import sys
from pathlib import Path

import m5
from m5.objects import *


parser = argparse.ArgumentParser(description='一个可配置的 gem5 系统.')
parser.add_argument('--binary', default='', type=str, help='要执行的二进制文件路径')
parser.add_argument(
    '--dag-manifest', default='', type=str,
    help='RTL-aligned in-process DAG manifest produced by tools/venus_l1_dag.py',
)
parser.add_argument(
    '--scheduler-firmware-elf', default='', type=str,
    help=(
        'Execute the immutable Scheduler l1.elf and let its MMIO/DMA/IRQ '
        'traffic fire the DAG instead of autostarting the semantic model'
    ),
)
parser.add_argument(
    '--scheduler-firmware-trace', default='', type=str,
    help='Optional Scheduler CPU/MMIO/DMA trace path',
)
parser.add_argument(
    '--scheduler-firmware-completion-gpio-mask', default=0,
    type=lambda value: int(value, 0),
    help=(
        'Optional external completion GPIO mask; zero keeps running until '
        '$stop or --max-ticks'
    ),
)
parser.add_argument(
    '--venus-config',
    choices=['venus1p0-64x512', 'venus1p0-64x512-legacy-bpll',
             'venus1p0-64x512-300mhz',
             'venus2p0-16x128', 'venus-rtl-16x128',
             'venus-rtl-16x128-dualclock-rebase', 'venus-rtl-16x512'],
    default='venus1p0-64x512',
    help='Venus tile configuration',
)
parser.add_argument(
    '--max-ticks', default=0, type=int,
    help='Optional simulation tick limit for bounded RTL/GEM5 diagnostics',
)
parser.add_argument(
    '--wfi-wake-tick', default=0, type=int,
    help=(
        'Optional absolute tick at which context 0 is activated, modelling '
        'an external scalar600 WFI wake input'
    ),
)
parser.add_argument(
    '--venus-pico-irq-tick', default=0, type=int,
    help=(
        'Optional absolute tick at which a Scheduler PicoRV32 interrupt is '
        'posted to context 0; intended for architecture/MMIO qualification'
    ),
)
parser.add_argument(
    '--venus-pico-irq-cause', default=0, type=lambda value: int(value, 0),
    help='Hardware IRQ cause bit mask posted at --venus-pico-irq-tick',
)
parser.add_argument(
    '--venus-pico-irq-vector', default=0x10000020,
    type=lambda value: int(value, 0),
    help='PicoRV32 PROGADDR_IRQ address used by Scheduler firmware',
)
parser.add_argument(
    '--standalone-shared-l2', action='store_true',
    help=(
        'Give a non-DAG single-task replay the same shared cluster-L2 '
        'LSU/CDC timing object used by DAG runs. This is required when a '
        'standalone microbench is compared with the RTL cluster-L2 path.'
    ),
)
parser.add_argument(
    '--venus-sim-mode',
    choices=['verification', 'fast'],
    default='verification',
    help=(
        'Observation profile only. verification retains per-VINS result '
        'files and the full sequencer monitor; fast disables those two '
        'expensive diagnostics while preserving every simulated event, '
        'DAG/task timing trace, final output and gem5 statistic.'
    ),
)
args = parser.parse_args()

# Fast mode is deliberately an observation policy, not a timing model.  Keep
# this decision before constructing any SimObject so every physical tile sees
# the same immutable profile.  Explicit focused traces such as
# VENUS_GEM5_LSU_TRACE remain available and are not suppressed here.
if args.venus_sim_mode == 'fast':
    os.environ['VENUS_GEM5_FAST_MODE'] = '1'
    os.environ['VENUS_GEM5_DISABLE_VINS_RESULT_DUMP'] = '1'
    os.environ['VENUS_GEM5_DISABLE_VINS_MONITOR'] = '1'
    print(
        'Venus simulation mode: fast '
        '(cycle model unchanged; per-VINS dumps/monitor disabled)'
    )
else:
    print('Venus simulation mode: verification')

dag_manifest = None
if args.dag_manifest:
    with open(args.dag_manifest, 'r', encoding='utf-8') as stream:
        dag_manifest = json.load(stream)
    # BreakpointFault hands an ebreak to VenusDagScheduler only for a live
    # DAG. This stays manifest-derived rather than relying on shell state.
    os.environ['VENUS_GEM5_DAG'] = '1'
    if dag_manifest.get('shared_l2_image'):
        os.environ['VENUS_GEM5_SHARED_L2_FILE'] = (
            dag_manifest['shared_l2_image']
        )
if args.scheduler_firmware_elf and not dag_manifest:
    raise ValueError('--scheduler-firmware-elf requires --dag-manifest')
if args.scheduler_firmware_elf:
    declared_firmware = dag_manifest.get('scheduler_firmware', {})

    def file_sha256(path):
        digest = hashlib.sha256()
        with open(path, 'rb') as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b''):
                digest.update(block)
        return digest.hexdigest()

    provenance_elves = {
        entry.get('source', {}).get('file')
        for entry in dag_manifest.get('l2_backing_inputs', [])
        if entry.get('source', {}).get('kind') == 'file' and
        str(entry.get('source', {}).get('file', '')).endswith('.elf')
    }
    provenance_elves.discard(None)
    if len(provenance_elves) > 1:
        raise ValueError(
            'Scheduler firmware manifest mixes multiple source ELF files'
        )
    declared_digest = declared_firmware.get('elf_sha256')
    if provenance_elves:
        provenance_elf = next(iter(provenance_elves))
        expected_firmware_digest = file_sha256(provenance_elf)
        if (declared_digest and
                declared_digest != expected_firmware_digest):
            raise ValueError(
                'Scheduler firmware manifest digest disagrees with its '
                f'input provenance: {declared_digest} != '
                f'{expected_firmware_digest}'
            )
        actual_firmware_digest = file_sha256(args.scheduler_firmware_elf)
        if actual_firmware_digest != expected_firmware_digest:
            raise ValueError(
                'Scheduler firmware ELF does not match manifest input '
                f'provenance: {actual_firmware_digest} != '
                f'{expected_firmware_digest}'
            )
        print(
            'Scheduler firmware provenance: exact ELF digest '
            f'{actual_firmware_digest}'
        )
    elif declared_digest:
        actual_firmware_digest = file_sha256(args.scheduler_firmware_elf)
        if actual_firmware_digest != declared_digest:
            raise ValueError(
                'Scheduler firmware ELF does not match manifest digest: '
                f'{actual_firmware_digest} != {declared_digest}'
            )
        print(
            'Scheduler firmware provenance: declared ELF digest '
            f'{actual_firmware_digest}'
        )
    else:
        raise ValueError(
            'Scheduler firmware execution requires exact ELF provenance in '
            'l2_backing_inputs or scheduler_firmware.elf_sha256'
        )

dag_manifests = [dag_manifest] if dag_manifest else []
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / 'tools'))
from venus_firmware_registry import validate_input_execution_mode
for entry in dag_manifests:
    validate_input_execution_mode(entry, bool(args.scheduler_firmware_elf))
if args.scheduler_firmware_elf:
    for entry in dag_manifest.get('firmware_dag_registry', []):
        if file_sha256(entry['manifest']) != entry['sha256']:
            raise ValueError('firmware registry manifest digest mismatch')
        with open(entry['manifest'], encoding='utf-8') as stream:
            dag_manifests.append(json.load(stream))
    for entry in dag_manifests:
        validate_input_execution_mode(entry, True)
        if entry.get('scheduler_firmware', {}).get('elf_sha256') != actual_firmware_digest:
            raise ValueError('firmware registry mixes source ELF digests')
        if not entry.get('firmware_identity') or entry.get('version', 0) < 5:
            raise ValueError('firmware execution requires ELF-qualified identity segments')
        for segment in entry['firmware_identity']:
            if file_sha256(segment['file']) != segment['sha256']:
                raise ValueError('firmware identity segment digest mismatch')
        layout = entry['dmt_layout']
        if file_sha256(layout['source_json']) != layout['source_json_sha256']:
            raise ValueError('firmware DMT source JSON changed after decoding')
    # Firmware must actually DMA all data; do not seed missing transfers from
    # a semantic replay image at time zero.
    os.environ.pop('VENUS_GEM5_SHARED_L2_FILE', None)
elif dag_manifest and dag_manifest.get('firmware_dag_registry'):
    raise ValueError('multi-DAG registry requires the firmware scheduler engine')

# The v5 scheduler DMA issues real 512-bit (64-byte) beats even when a
# diagnostic workload selects a non-RTL scalar-core profile.  Decide this
# before constructing SystemXBar so the bus never silently fractures every
# transfer into four generic 16-byte transactions.
timing_l1_dma_requested = bool(
    dag_manifest and dag_manifest.get('version', 1) >= 5 and
    dag_manifest.get('fire_plan') and
    os.environ.get('VENUS_GEM5_L1_DMA', '1') != '0'
)
if args.scheduler_firmware_elf and not timing_l1_dma_requested:
    raise ValueError(
        'Scheduler firmware requires a v5 semantic fire plan with timing DMA'
    )

VENUS_CONFIGS = {
    'venus1p0-64x512': {
        'lane_num': 64, 'line_num': 512, 'rtl_scalar_core': True,
        'rtl_runtime_dmt_length': True,
        # The V1 SoC drives system/AXI at 150 MHz.  Although firmware also
        # programs BPLL to 300 MHz, its final GC0802_DEBUG write clears bit 8.
        # venus_block_wrapper therefore selects aclk, rather than block_clk,
        # while the DAG runs.  Model the clock mux output, not merely the PLL.
        # Its shuffle engine is the original global lockstep FSM, not the
        # four-entry per-PE pipeline introduced by Venus2.
        'system_clock': '150MHz',
        'tile_clock': '150MHz',
        # V1 l1.ld exposes a 1 MiB cacheable window followed by a 512 KiB
        # uncached window.  The latter contains NOLOAD heap/stack pages that
        # are present in SRAM even though they have no ELF file payload.
        'scheduler_firmware_ram_size': '1536KiB',
        'shuffle_registered_request': True,
        'shuffle_lockstep': True,
        # V1 registers pe_resp_i before it is consumed by the
        # pe_vinsn_running_q retirement table.  Venus2 comments out that
        # register and consumes pe_resp_i directly.
        'rtl_registered_pe_response': True,
        # venus_dispatcher contains a two-entry non-bypass spill_register.
        # Its registered venus_req_o is the request already owned by the
        # sequencer issue FSM, so counting that request again in this input
        # FIFO creates four live requests (one active plus three queued),
        # whereas RTL has one active/output request plus the two spill
        # entries.  Keep the input-side capacity at two.
        'rtl_scalar_dispatch_capacity': 2,
        # The V1 sequencer FSM returns directly from DOWNSTREAM_RECEIVE to
        # UPSTREAM_RECEIVE when the selected PE queues are ready.  It has no
        # separate downstream-ack state; retaining that shared-model stage
        # stretches the minimum issue interval from five to six tile clocks.
        'rtl_direct_downstream_return': True,
        # V1's sequencer explicitly blocks upstream return when lane 0 has
        # retired a running ID but any other participating lane still owns
        # it.  This is a global structural guard, not an operator latency.
        'rtl_lane_desync_stall': True,
        # venus_sequencer broadcasts ordinary vector requests to all lanes
        # and AND-reduces every lane-ready signal.  Tail lanes determine
        # local VL zero only after acceptance and retire without data work.
        'rtl_all_lane_issue_handshake': True,
        # Fresh V1 PE/VFU observers place the lane command four tile clocks
        # after the sequencer records VINS start.  The legacy gem5 lane port
        # already accounts for one of those clocks, so expose the complete
        # four-clock capture-to-command path here.  This is a physical
        # register boundary shared by all ordinary lane operations.
        'rtl_pe_command_visibility_cycles': 3,
        # V1 assigns the truncated vector row quotient directly as an
        # inclusive head-to-tail offset.  V2 instead forms a rounded row
        # count and subtracts one.  This backend flag keeps their hazard
        # scoreboards independent.
        'rtl_dispatcher_inclusive_tail': True,
        # tile_soft_reset_n resets the sequencer, Shuffle engine and its
        # per-lane/VRF arbiter round-robin state at every task allocation.
        # Keep the switch backend-scoped until the protected V2 profile is
        # independently requalified with the same structural reset model.
        'rtl_task_soft_reset': True,
        # The task epilogue writes venustile_softresetreg and the externally
        # observed execution boundary is its z1 register.  Both registers are
        # driven by selected_block_clk in Venus1, so task-done becomes visible
        # after one selected tile-clock edge.  Do not count this propagation
        # on the independent Scheduler/system clock used by legacy BPLL runs.
        'task_done_visibility_cycles': 1,
        'task_done_visibility_uses_tile_clock': True,
        'reset_task_pipeline_before_fire': True,
        'task_reset_vector': 0,
        # V1 and V2 use the same scalar600 IF/ID branch path for ordinary
        # RV32 control transfers.  V2 only adds WFI/LRSC plumbing around it.
        # Suppress Minor's generic predictor and resolve JAL/JALR/conditional
        # redirects at the scalar600 ID boundary for both generations.
        'rtl_static_direct_taken_prediction': True,
        'rtl_id_branch_resolve': True,
        # V1 venus_operand_requester has no raw_hazard_counter credit path;
        # a captured command waits for complete producer retirement.  V2
        # added row-granular, cross-VFU chaining under extensionCFG bit 1.
        'rtl_chaining_enabled': False,
        # venus_mask stores mask bits in a synchronous SRAM.  Its read-valid
        # register feeds a second operand FIFO, so a request accepted on one
        # 150 MHz requester edge cannot be consumed until the following
        # requester edge.  Seven nanoseconds places the memory response just
        # beyond one 6.667 ns tile period and models that registered boundary;
        # this is a backend SRAM/FIFO property, not an operator latency.
        'mask_read_response': '7ns',
        # scalar600 releases an idle barrier through its ID/EX/WB path in
        # two clocks.  A Venus request which reaches the three-flop busy
        # synchronizer only at that WB edge retains the third clock.  Both
        # choices are made from live RTL state, never task/PC/DAG identity.
        'rtl_barrier_idle_release_to_commit': 2,
        'rtl_barrier_late_busy_recheck': True,
        # The V1 and V2 RTLs share the same requester/VRF arbitration
        # structures.  A clean A/B on the legacy PDCCH exposed the same
        # per-bank round-robin, requester-Q, one-entry operand-command and
        # Shuffle live-intent semantics; keep them as a backend property.
        'rtl_qualified_requester_model': True,
    },
    'venus2p0-16x128': {
        'lane_num': 16, 'line_num': 128, 'rtl_scalar_core': False,
    },
    'venus-rtl-16x128': {
        'lane_num': 16, 'line_num': 128, 'rtl_scalar_core': True,
        # R912--R917 qualified the registered requester visibility,
        # per-bank VRF round-robin arbitration, one-entry operand command
        # registers and Shuffle live intent as one coherent RTL profile.
        # These are hardware-structure settings, not workload tuning.
        'rtl_qualified_requester_model': True,
        'rtl_static_direct_taken_prediction': True,
        'rtl_id_branch_resolve': True,
        # testbench_postsyn_cluster.sv drives aclk at 250 MHz and tileclk
        # at 500 MHz.  This is topology/clock evidence, independent of the
        # historical diagnostic latency rebase below.
        'rtl_clock_domains': True,
    },
    # Diagnostic-only topology. The rebased latency constants intentionally
    # preserve a historical GEM5 experiment; they are not RTL measurements.
    'venus-rtl-16x128-dualclock-rebase': {
        'lane_num': 16, 'line_num': 128, 'rtl_scalar_core': True,
        'rtl_clock_domains': True,
        'dualclock_rebase': True,
    },
    'venus-rtl-16x512': {
        'lane_num': 16, 'line_num': 512, 'rtl_scalar_core': True,
    },
}

# The Venus1 BPLL contract keeps system/AXI at 150 MHz and selects the 300 MHz
# block clock for the tile.  Keep this clock-mux contract explicit: selecting
# it is a backend/firmware evidence decision, never a DAG/PC heuristic.  The
# legacy name remains for native LTE evidence; ACE-ECHO's primary 300 MHz
# profile below reuses the same physical timing model with distinct provenance.
VENUS_CONFIGS['venus1p0-64x512-legacy-bpll'] = dict(
    VENUS_CONFIGS['venus1p0-64x512'],
    tile_clock='300MHz',
    firmware_clock_contract='legacy-bpll-debug-bit8-set',
    # venus_mask.sv registers a read-valid token for exactly one local tile
    # clock.  The base V1 profile's 7 ns response intentionally crosses one
    # 150 MHz edge, but inheriting that absolute delay at 300 MHz stretches
    # the same SRAM boundary across more than two tile clocks.  Keep the
    # memory response inside the 300 MHz period so the next lane edge sees
    # the registered token; operand-queue and VFU latch edges remain modeled
    # independently.  This is a clock-domain property, not an operator or
    # workload latency.
    mask_read_response='1ns',
)
VENUS_CONFIGS['venus1p0-64x512-300mhz'] = dict(
    VENUS_CONFIGS['venus1p0-64x512-legacy-bpll'],
    firmware_clock_contract='ace-echo-bpll-debug-bit8-set',
)

venus_config = VENUS_CONFIGS[args.venus_config]
dualclock_rebase = venus_config.get('dualclock_rebase', False)
rtl_clock_domains = venus_config.get('rtl_clock_domains', False)
rtl_vrf_read_response = os.environ.get(
    'VENUS_GEM5_RTL_VRF_READ_RESPONSE', '1ns')
rtl_vrf_write_response = os.environ.get(
    'VENUS_GEM5_RTL_VRF_WRITE_RESPONSE', '1ns')
rtl_mask_read_response = os.environ.get(
    'VENUS_GEM5_RTL_MASK_READ_RESPONSE',
    venus_config.get('mask_read_response', rtl_vrf_read_response))
rtl_mask_write_response = os.environ.get(
    'VENUS_GEM5_RTL_MASK_WRITE_RESPONSE',
    venus_config.get('mask_write_response', rtl_vrf_write_response))
rtl_vrf_request_gap = os.environ.get(
    'VENUS_GEM5_VRF_GRANT_GAP', '1ns')
experimental_vrf_rr = (
    os.environ.get(
        'VENUS_GEM5_EXPERIMENTAL_VRF_RR',
        '1' if venus_config.get('rtl_qualified_requester_model', False)
        else '0',
    ) == '1'
)
experimental_requester_q_visibility = (
    os.environ.get(
        'VENUS_GEM5_EXPERIMENTAL_REQUESTER_Q_VISIBILITY',
        '1' if venus_config.get('rtl_qualified_requester_model', False)
        else '0',
    ) == '1'
)
experimental_one_entry_operand_commands = (
    os.environ.get(
        'VENUS_GEM5_EXPERIMENTAL_ONE_ENTRY_OPERAND_COMMANDS',
        '1' if (venus_config['rtl_scalar_core'] and
                experimental_requester_q_visibility) else '0',
    ) == '1'
)
rtl_shuffle_registered_request = (
    os.environ.get(
        'VENUS_GEM5_SHUFFLE_REGISTERED_REQUEST',
        '1' if venus_config.get(
            'shuffle_registered_request',
            venus_config['rtl_scalar_core'],
        ) else '0',
    ) == '1'
)
experimental_shuffle_live_intent = (
    os.environ.get(
        'VENUS_GEM5_EXPERIMENTAL_SHUFFLE_LIVE_INTENT',
        '1' if (venus_config['rtl_scalar_core'] and
                experimental_vrf_rr) else '0',
    ) == '1'
)
if experimental_vrf_rr:
    print(
        "Experimental deferred per-bank Venus VRF RR arbitration enabled"
    )
if experimental_one_entry_operand_commands:
    print("Experimental RTL one-entry operand command registers enabled")
if experimental_shuffle_live_intent:
    if not experimental_vrf_rr:
        raise ValueError(
            'Shuffle live intent requires VENUS_GEM5_EXPERIMENTAL_VRF_RR=1'
        )
    print("Experimental Shuffle LockIn=0 live intent enabled")
if ('VENUS_GEM5_VRF_GRANT_GAP' in os.environ and
        venus_config['rtl_scalar_core']):
    print(
        "Venus VRF grant-backpressure test enabled: "
        f"request_gap={rtl_vrf_request_gap}"
    )
if (venus_config['rtl_scalar_core'] and
        'VENUS_GEM5_LEGACY_GLOBAL_HAZARDS' not in os.environ):
    os.environ['VENUS_GEM5_ENABLE_OPERAND_HAZARDS'] = '1'

LANE_NUM = venus_config['lane_num']
BANK_NUM = 4
BYTES_PER_BANK = 2
LINE_NUM = venus_config['line_num']
MAX_LANE_NUM = 64
VRF_CANONICAL_BASE = 0x80100000
RTL_TILE_BASE = 0x82000000
RTL_TILE_STRIDE = 0x00200000
RTL_ISPM_OFFSET = 0x00000000
RTL_DSPM_OFFSET = 0x00020000
RTL_VSPM_OFFSET = 0x00100000
RTL_CONTROL_OFFSET = 0x001ff000
# gc0802 generated/autoconf.sv sets CONFIG_*_SPADIMEM=0x8000 and
# CONFIG_*_SPADDMEM=0x4000.  The RTL wrapper aliases its 128-KiB
# architectural ISPM window in 32-KiB segments.  It independently routes
# its whole block-DSPM-to-VSPM gap through a 16-KiB DSPM, so each such
# segment aliases the same physical DSPM backing store.
RTL_ISPM_BYTES = 0x00008000
RTL_DSPM_BYTES = 0x00004000
# The active gc0802 simulation hierarchy instantiates tile0 through tile3.
# tile_hardware_info_t packs the task-container fields as
# {spm_size[3:0], num_lane[3:0], has_bitalu, has_serdiv, has_complexunit}.
# The RTL tiles expose 16 KiB SPM (code 2), 16 lanes (code 5), and all three
# optional units.
RTL_16X128_DYNAMIC_TILE_COUNT = 4
RTL_16X128_TILE_HARDWARE_CAPABILITY = (
    (2 << 7) | (5 << 3) | (1 << 2) | (1 << 1) | 1
)


def rtl_tile_base(tile_id):
    return RTL_TILE_BASE + tile_id * RTL_TILE_STRIDE


def timing_fire_vectors(manifest):
    """Flatten the v5 symbolic fire contract for the scheduler SimObject.

    This deliberately converts only runtime-DAG metadata.  In particular,
    it does not inspect ``rtl_observed_dma``: task/tile timing must emerge
    from the DMA state machine and memory responses, not from a capture.
    """
    source_kinds = {
        'shared_l2': 0,
        'dmt_return': 1,
        'ptr_temp': 2,
        'ptr_global': 3,
        'ptr_dfe': 3,
    }
    kind_codes = {'code': 0, 'data': 1, 'input': 2}
    fields = {
        'tasks': [], 'ordinals': [], 'kinds': [], 'input_types': [],
        'source_kinds': [], 'source_offsets': [], 'destinations': [],
        'lengths': [], 'parents': [], 'ports': [], 'pointer_bytes': [],
    }
    dmt_layout = manifest.get('dmt_layout', {})
    dmt_malloc_base = dmt_layout.get('malloc_base')
    if dmt_malloc_base is None:
        raise ValueError('v5 timing fire plan requires dmt_layout.malloc_base')

    # Old v5 materializations did not yet carry the target extent inside a
    # packed-global pointer.  Retain a compatibility fallback, but newly
    # generated manifests derive it from the global table in venus_l1_dag.py.
    legacy_pointer_bytes = {}
    for entry in manifest.get('initial_inputs', []):
        if entry.get('type') not in (5, 6):
            continue
        try:
            with open(entry['file'], 'rb') as stream:
                raw = stream.read(2)
            if len(raw) == 2:
                legacy_pointer_bytes[(entry['task'],
                                      entry.get('descriptor_index', 0))] = (
                    raw[0] | (raw[1] << 8)
                )
        except OSError:
            # The C++ side will produce a clear validation error if this old
            # manifest lacks the needed target capacity.
            pass

    for entry in manifest.get('fire_plan', []):
        source = entry.get('source', {})
        source_space = source.get('space')
        if source_space not in source_kinds:
            raise ValueError(
                f"unsupported v5 fire source space {source_space!r}"
            )
        kind = entry.get('kind')
        if kind not in kind_codes:
            raise ValueError(f"unsupported v5 fire kind {kind!r}")
        payload = entry.get('payload', {})
        producer = source.get('producer', payload.get('producer', {}))
        target = payload.get('target', {})
        length = entry.get('length', {})
        descriptor_index = entry.get('descriptor_index')
        pointer_bytes = target.get(
            'bytes', legacy_pointer_bytes.get(
                (entry['task'], descriptor_index), 0
            )
        )
        # For ptr-global/dfe, source_offsets names the target encoded in the
        # 64-byte pointer record.  A DMT/ptr-temp record is relative to the
        # L2_malloc base programmed by the RTL overall testbench; flatten it
        # only after reading that static runtime-DAG metadata.
        if source_space == 'dmt_return':
            source_offset = dmt_malloc_base + source['relative_offset']
            pointer_bytes = source['bytes']
        elif source_space == 'ptr_temp':
            source_offset = dmt_malloc_base + target['relative_offset']
            pointer_bytes = target['bytes']
        else:
            source_offset = source.get('offset', target.get('offset', 0))
        fields['tasks'].append(entry['task'])
        fields['ordinals'].append(entry['issue_ordinal'])
        fields['kinds'].append(kind_codes[kind])
        fields['input_types'].append(
            255 if entry.get('input_type') is None else entry['input_type']
        )
        fields['source_kinds'].append(source_kinds[source_space])
        fields['source_offsets'].append(source_offset)
        fields['destinations'].append(entry['destination']['offset'])
        fields['lengths'].append(length.get('bytes', 0))
        fields['parents'].append(producer.get('task', 0))
        fields['ports'].append(producer.get('retid', 0))
        fields['pointer_bytes'].append(pointer_bytes)
    return fields


def firmware_return_outputs(manifest):
    """Recover the Scheduler return-register contract from runtime JSON.

    Early v5 manifests materialized every DMT slot but left ``outputs``
    empty.  The immutable l1.elf still expects the top-level DAG returns in
    the source runtime JSON's ``return_output`` order.  Recover that semantic
    contract here; do not infer it from terminal tasks, task names, or a
    workload-specific list.
    """
    explicit = manifest.get('outputs', [])
    if explicit:
        return explicit

    layout = manifest.get('dmt_layout', {})
    source_json = layout.get('source_json')
    if not source_json:
        raise ValueError(
            'Scheduler firmware requires outputs or dmt_layout.source_json'
        )
    with open(source_json, 'r', encoding='utf-8') as stream:
        runtime_dag = json.load(stream)
    return_nodes = [
        node['return_output'] for node in runtime_dag
        if isinstance(node, dict) and 'return_output' in node
    ]
    if len(return_nodes) != 1:
        raise ValueError(
            f'{source_json}: expected one return_output node, '
            f'found {len(return_nodes)}'
        )

    slots = {
        (int(slot['task']), int(slot['port']))
        for slot in layout.get('slots', [])
    }
    outputs = []
    for index, entry in enumerate(return_nodes[0]):
        task = int(entry['parentTasks'])
        packed_port = int(str(entry['parentTasksPort']), 0)
        port = packed_port & 0xf
        if 'index' in entry and port != int(entry['index']):
            raise ValueError(
                f'{source_json}: return {index} port encoding disagrees '
                f'with index ({port} != {entry["index"]})'
            )
        if (task, port) not in slots:
            raise ValueError(
                f'{source_json}: return {index} references missing DMT '
                f'slot {task}:{port}'
            )
        outputs.append({
            'task': task,
            'port': port,
            # Firmware returns use the live DMT address.  ``source`` is a
            # legacy output-dump field retained only for vector ABI shape.
            'source': 0,
            'length': int(entry['length']),
        })
    return outputs


def configure_scalar_cpu(cpu):
    if not venus_config['rtl_scalar_core']:
        return
    # scalar600 is single-issue/single-retire. Keep this tied to the RTL
    # profile, not to a workload name or task ID.
    cpu.decodeInputWidth = 1
    # A redirect replaces scalar600 IF's registered PC even while the SRAM
    # response for the preceding sequential address is returning.  Minor
    # represents that old response as an in-flight line, so retain one slot
    # for it and one for the redirect target.
    cpu.fetch1FetchLimit = 2
    cpu.executeInputWidth = 1
    cpu.executeIssueLimit = 1
    cpu.executeCommitLimit = 1
    cpu.venusRtlScalarTiming = True
    cpu.venusRtlStaticDirectTakenPrediction = venus_config.get(
        'rtl_static_direct_taken_prediction', False
    )
    cpu.venusRtlIdBranchResolve = venus_config.get(
        'rtl_id_branch_resolve', False
    )
    cpu.venusRtlBarrierIdleReleaseToCommit = venus_config.get(
        'rtl_barrier_idle_release_to_commit', 3
    )
    cpu.venusRtlBarrierLateBusyRecheck = venus_config.get(
        'rtl_barrier_late_busy_recheck', False
    )
    cpu.executeAllowEarlyMemoryIssue = False
    cpu.executeMaxAccessesInMemory = 2
    scalar_fus = cpu.executeFuncUnits.funcUnits
    scalar_fus[0].opLat = 1
    scalar_fus[1].opLat = 1
    scalar_fus[2].issueLat = 3
    scalar_fus[2].opLat = 3
    scalar_fus[2].timings[0].extraCommitLat = 0
    # The RTL divider's count/result registers are modeled in Execute.  Keep
    # the generic FU as a one-edge carrier; the shadow FSM supplies both the
    # dynamic completion edge and history-dependent writeback.
    scalar_fus[3].issueLat = 1
    scalar_fus[3].opLat = 1


def tile_latency(normal, rebased):
    return rebased if dualclock_rebase else normal


def build_tile(tile_id, context_count, shared_l2):
    """Build one complete gc0802 tile with no local-state sharing."""
    tile_base = rtl_tile_base(tile_id)
    # The lane/shuffle-side VRF requests use physical addresses on the local
    # tile Xbar.  Give each tile the same VSPM window that RTL decodes, rather
    # than reusing tile 0's canonical address.  Besides matching RTL, this
    # prevents the bank memories from aliasing in gem5's global address map.
    vrf_physical_base = tile_base + RTL_VSPM_OFFSET
    cpu = MinorCPU(numThreads=context_count, clk_domain=tile_clk_domain)
    configure_scalar_cpu(cpu)
    cpu.createInterruptController()
    cpu.scalar_mem_xbar = NoncoherentXBar(
        clk_domain=tile_clk_domain,
        forward_latency=0,
        response_latency=0,
        frontend_latency=0,
        width=64,
    )
    cpu.icache_port = cpu.scalar_mem_xbar.cpu_side_ports
    cpu.dcache_port = cpu.scalar_mem_xbar.cpu_side_ports
    cpu.scalar_mem_xbar.mem_side_ports = system.physmem_xbar.cpu_side_ports
    cpu.scalar_mem_xbar.default = system.membus.cpu_side_ports

    cpu.vrf = venus_vrf_mem(
        lane_num=LANE_NUM,
        bank_num=BANK_NUM,
        line_num=LINE_NUM,
        logical_base=tile_base + RTL_VSPM_OFFSET,
        # venus_vrf_mem is not ClockedObject; this is an absolute delay.
        logical_latency=(
            '12ns' if dualclock_rebase else
            ('6ns' if venus_config['rtl_scalar_core'] else '1ns')
        ),
    )
    cpu.venus_vrf_object = cpu.vrf
    cpu.VenusSequencer = VenusSequencer(
        vrf_object=cpu.vrf,
        shared_l2_object=(shared_l2 if shared_l2 is not None else NULL),
        experimental_requester_q_visibility=(
            experimental_requester_q_visibility
        ),
        rtl_registered_pe_response=venus_config.get(
            'rtl_registered_pe_response', False
        ),
        rtl_scalar_second_word_boundary=(
            venus_config['rtl_scalar_core'] and
            os.environ.get(
                'VENUS_GEM5_RTL_SCALAR_SECOND_WORD_BOUNDARY', '1'
            ) != '0'
        ),
        rtl_scalar_dispatch_capacity=venus_config.get(
            'rtl_scalar_dispatch_capacity', 2
        ),
        rtl_direct_downstream_return=venus_config.get(
            'rtl_direct_downstream_return', False
        ),
        rtl_lane_desync_stall=venus_config.get(
            'rtl_lane_desync_stall', False
        ),
        rtl_all_lane_issue_handshake=venus_config.get(
            'rtl_all_lane_issue_handshake', False
        ),
        rtl_dispatcher_inclusive_tail=venus_config.get(
            'rtl_dispatcher_inclusive_tail', False
        ),
        rtl_task_soft_reset=venus_config.get(
            'rtl_task_soft_reset', False
        ),
        clk_domain=tile_clk_domain,
    )
    cpu.venus_sequencer = cpu.VenusSequencer

    # One external SRAM window covers ISPM and DSPM. A separate external
    # VSPM range is advertised by cpu.vrf.logical_port, and a control page is
    # independently reset by VenusDagScheduler at every dispatch.
    cpu.tile_spm = SimpleMemory(
        range=AddrRange(start=tile_base, size=RTL_VSPM_OFFSET),
        clk_domain=tile_clk_domain,
    )
    if timing_l1_dma_requested:
        # Keep raw 0x82... addresses on the timing fabric.  RangeAddrMapper
        # performs the non-injective RTL ISPM/DSPM mirrors only after the
        # request reaches the tile-memory boundary, so an L1 DMA trace
        # remains an actual bus trace rather than a scheduler-side address
        # projection.
        spm_original_ranges = []
        spm_remapped_ranges = []
        for offset in range(RTL_ISPM_OFFSET, RTL_DSPM_OFFSET,
                            RTL_ISPM_BYTES):
            spm_original_ranges.append(
                AddrRange(start=tile_base + offset, size=RTL_ISPM_BYTES)
            )
            spm_remapped_ranges.append(
                AddrRange(start=tile_base + RTL_ISPM_OFFSET,
                          size=RTL_ISPM_BYTES)
            )
        for offset in range(RTL_DSPM_OFFSET, RTL_VSPM_OFFSET,
                            RTL_DSPM_BYTES):
            spm_original_ranges.append(
                AddrRange(start=tile_base + offset, size=RTL_DSPM_BYTES)
            )
            spm_remapped_ranges.append(
                AddrRange(start=tile_base + RTL_DSPM_OFFSET,
                          size=RTL_DSPM_BYTES)
            )
        cpu.tile_spm_mapper = RangeAddrMapper(
            original_ranges=spm_original_ranges,
            remapped_ranges=spm_remapped_ranges,
        )
        cpu.tile_spm_mapper.mem_side_port = cpu.tile_spm.port
    # scalar600's data SRAM port and the block-side L1 DMA port meet at the
    # tile boundary; scalar accesses do not make a round trip through the
    # 250-MHz cluster system bus.  Keep both requesters on one local timing
    # crossbar so they still share the same backing SRAM and backpressure.
    cpu.scalar_spm_xbar = NoncoherentXBar(
        clk_domain=tile_clk_domain,
        forward_latency=0,
        response_latency=0,
        frontend_latency=0,
        width=64,
        # Both scalar_mem_xbar and the cluster/L1-DMA fabric can target this
        # SRAM boundary.  Its upstream response can therefore be rejected
        # by another timing xbar and must retain normal retry state.
        buffer_responses=True,
    )
    cpu.scalar_spm_xbar.mem_side_ports = (
        cpu.tile_spm_mapper.cpu_side_port if timing_l1_dma_requested
        else cpu.tile_spm.port
    )
    cpu.scalar_mem_xbar.mem_side_ports = \
        cpu.scalar_spm_xbar.cpu_side_ports
    cpu.tile_control = SimpleMemory(
        range=AddrRange(start=tile_base + RTL_CONTROL_OFFSET, size=0x1000),
        clk_domain=tile_clk_domain,
    )
    membus_port_base = 1 + tile_id * 3
    system.membus.mem_side_ports[membus_port_base] = cpu.vrf.logical_port
    system.membus.mem_side_ports[membus_port_base + 1] = (
        cpu.scalar_spm_xbar.cpu_side_ports
    )
    system.membus.mem_side_ports[membus_port_base + 2] = cpu.tile_control.port

    cpu.VenusShufflePipline = VenusShufflePipline(
        num_pes=LANE_NUM,
        num_banks_per_lane=BANK_NUM,
        line_num=LINE_NUM,
        vrf_base_addr=vrf_physical_base,
        # shuffle_result_req_d is registered into req_q in the RTL shuffle
        # engine before the lane arbiter can grant it.  This boundary belongs
        # to the shuffle requester itself; it is independent of the optional
        # whole-VRF round-robin arbitration prototype.
        registered_request_visibility=rtl_shuffle_registered_request,
        live_requester_intent=experimental_shuffle_live_intent,
        legacy_lockstep=venus_config.get('shuffle_lockstep', False),
        clk_domain=tile_clk_domain,
    )
    cpu.VenusSequencer.shuffle_object = cpu.VenusShufflePipline
    cpu.VenusShufflePipline.port_venusshuffle_receivefrom_venussequencer = (
        cpu.VenusSequencer.port_venussequencer_sendto_venusshuffle
    )

    def lane_kwargs(lane_id, lane_refs=None):
        kwargs = {
            'lane_num': LANE_NUM,
            'bank_num': BANK_NUM,
            'line_num': LINE_NUM,
            'lane_id': lane_id,
            'experimental_vrf_rr': experimental_vrf_rr,
            'experimental_requester_q_visibility': (
                experimental_requester_q_visibility
            ),
            'experimental_one_entry_operand_commands': (
                experimental_one_entry_operand_commands
            ),
            'rtl_pe_command_visibility_cycles': venus_config.get(
                'rtl_pe_command_visibility_cycles', 1
            ),
            'rtl_chaining_enabled': venus_config.get(
                'rtl_chaining_enabled', True
            ),
            'vrf_base_addr': vrf_physical_base,
            'venus_shuffle_pipeline': cpu.VenusShufflePipline,
            'clk_domain': tile_clk_domain,
        }
        for index in range(1, MAX_LANE_NUM):
            kwargs[f'venus_lane_simobject_{index}'] = (
                lane_refs[index]
                if lane_refs is not None and index < len(lane_refs)
                else cpu.vrf
            )
        return kwargs

    for lane in range(LANE_NUM):
        setattr(cpu, f'VenusLane_{lane}', VenusLane(**lane_kwargs(lane)))
    lane_refs = [getattr(cpu, f'VenusLane_{lane}')
                 for lane in range(LANE_NUM)]
    cpu.VenusLane_0 = VenusLane(**lane_kwargs(0, lane_refs))

    cpu.XBAR = NoncoherentXBar(
        clk_domain=tile_clk_domain,
        forward_latency=0,
        response_latency=0,
        frontend_latency=0,
        width=1,
        venus_vrf_data_banks=(
            LANE_NUM * BANK_NUM
            if venus_config['rtl_scalar_core'] and experimental_vrf_rr
            else 0
        ),
        venus_vrf_mask_banks=(
            LANE_NUM
            if venus_config['rtl_scalar_core'] and experimental_vrf_rr
            else 0
        ),
        venus_vrf_ports_per_lane=16,
    )
    cpu.VenusSequencer.vrf_xbar_object = cpu.XBAR
    cpu.VenusSequencer.port_venussequencer_receivefrom_venuspacketgen = (
        cpu.port_venusminorcpu_sendto_venussequencer
    )
    for lane in range(LANE_NUM):
        lane_obj = getattr(cpu, f'VenusLane_{lane}')
        lane_obj.port_venuslane_receivefrom_venussequencer = (
            cpu.VenusSequencer.port_venussequencer_sendto_venuslane[lane]
        )
        lane_obj.port_venuslane_hazardtable_listen = (
            cpu.VenusSequencer.port_venussequencer_hazardtable_boardcast[lane]
        )
        lane_obj.port_venuslane_receivefrom_venusshuffle = (
            cpu.VenusShufflePipline.lane_ports[lane]
        )

    sram_size = LINE_NUM * BYTES_PER_BANK
    mask_sram_size = LINE_NUM * BANK_NUM * BYTES_PER_BANK
    start_address = vrf_physical_base
    mask_start_address = (
        vrf_physical_base +
        LINE_NUM * BYTES_PER_BANK * LANE_NUM * BANK_NUM
    )
    for lane in range(LANE_NUM):
        for bank in range(BANK_NUM):
            name = f'lane{lane}_bank{bank}'
            memory_kwargs = {
                'range': AddrRange(start=start_address, size=sram_size),
                'clk_domain': tile_clk_domain,
                'request_gap': rtl_vrf_request_gap,
            }
            if dualclock_rebase:
                memory_kwargs.update(
                    latency='60ns', read_response_latency='2ns',
                    write_response_latency='2ns',
                )
            elif venus_config['rtl_scalar_core']:
                # Keep the physical-bank response path independently
                # measurable from the logical VSPM/DMA aperture.  The RTL
                # shuffle path crosses the bank SRAM, DSPM output mux and a
                # spill register; diagnostic sweeps can override these two
                # generic bank timings without changing task code or data.
                memory_kwargs.update(
                    read_response_latency=rtl_vrf_read_response,
                    write_response_latency=rtl_vrf_write_response,
                )
            setattr(cpu.vrf, name, SimpleMemory(**memory_kwargs))
            cpu.XBAR.mem_side_ports[lane * BANK_NUM + bank] = (
                getattr(cpu.vrf, name).port
            )
            start_address += sram_size

        mask_name = f'lane{lane}_mask'
        mask_kwargs = {
            'range': AddrRange(start=mask_start_address, size=mask_sram_size),
            'clk_domain': tile_clk_domain,
            'request_gap': rtl_vrf_request_gap,
        }
        if dualclock_rebase:
            mask_kwargs.update(
                latency='60ns', read_response_latency='2ns',
                write_response_latency='2ns',
            )
        elif venus_config['rtl_scalar_core']:
            mask_kwargs.update(
                read_response_latency=rtl_mask_read_response,
                write_response_latency=rtl_mask_write_response,
            )
        setattr(cpu.vrf, mask_name, SimpleMemory(**mask_kwargs))
        cpu.XBAR.mem_side_ports[LANE_NUM * BANK_NUM + lane] = (
            getattr(cpu.vrf, mask_name).port
        )
        mask_start_address += mask_sram_size

    # The final 4 KiB of the 1 MiB VSPM window is the per-tile control page.
    venus_window_limit = tile_base + RTL_CONTROL_OFFSET
    if mask_start_address < venus_window_limit:
        cpu.vrf.venus_window_pad = SimpleMemory(
            range=AddrRange(
                start=mask_start_address,
                size=venus_window_limit - mask_start_address,
            ),
            clk_domain=tile_clk_domain,
        )
        cpu.XBAR.mem_side_ports[LANE_NUM * (BANK_NUM + 1)] = (
            cpu.vrf.venus_window_pad.port
        )

    for lane in range(LANE_NUM):
        lane_obj = getattr(cpu, f'VenusLane_{lane}')
        for port_index in range(16):
            cpu.XBAR.cpu_side_ports[port_index + 16 * lane] = getattr(
                lane_obj, f'port_venuslanetovrf_{port_index + 1}'
            )
    cpu.VenusShufflePipline.port_venusshuffle_hazardtable_listen = (
        cpu.VenusSequencer.port_venussequencer_hazardtable_boardcast_to_shuffle
    )
    return cpu


task_count = sum(len(entry['tasks']) for entry in dag_manifests) if dag_manifest else 1
dag_image_bytes_per_task = 4 * 1024 * 1024
dynamic_context_copies = 1
if (dag_manifest and dag_manifest.get('version', 1) >= 5 and
        args.venus_config == 'venus-rtl-16x128' and
        not all('tile_id' in task for task in dag_manifest['tasks'])):
    # Runtime tile allocation needs a separately mapped SE Process for every
    # task on every physical tile.  Reserve the loader's complete ELF image
    # footprint up front rather than failing half way through instantiate().
    dynamic_context_copies = RTL_16X128_DYNAMIC_TILE_COUNT
physmem_required = max(
    128 * 1024 * 1024,
    task_count * dynamic_context_copies * dag_image_bytes_per_task +
    64 * 1024 * 1024,
)
physmem_bytes = 1 << (physmem_required - 1).bit_length()

print(f'Using Venus config {args.venus_config}: {LANE_NUM} lanes, {LINE_NUM} rows')
if venus_config.get('firmware_clock_contract'):
    print(
        'Using firmware clock contract: '
        f"{venus_config['firmware_clock_contract']}"
    )
if dag_manifest:
    print(f'Using SE physical-memory pool: {physmem_bytes // (1024 * 1024)} MiB '
          f'for {task_count} DAG tasks x {dynamic_context_copies} tile context(s)')

system = System()
system.clk_domain = SrcClockDomain()
system_clock = venus_config.get(
    'system_clock', '250MHz' if rtl_clock_domains else '1GHz')
tile_clock = venus_config.get(
    'tile_clock', '500MHz' if rtl_clock_domains else system_clock)
system.clk_domain.clock = system_clock
system.clk_domain.voltage_domain = VoltageDomain()
tile_clk_domain = system.clk_domain
if tile_clock != system_clock:
    system.tile_clk_domain = SrcClockDomain()
    system.tile_clk_domain.clock = tile_clock
    system.tile_clk_domain.voltage_domain = VoltageDomain()
    tile_clk_domain = system.tile_clk_domain
    print(
        'Using backend clock domains: '
        f'system/AXI={system_clock}, tile={tile_clock}'
    )

system.mem_mode = 'timing'
system.mem_ranges = [AddrRange(physmem_bytes)]
system.membus = SystemXBar(
    clk_domain=system.clk_domain,
    # RTL L2 DMA is a 512-bit AXI master.  The scheduler DMA engine emits
    # 64-byte beats, so leaving the generic 16-byte SystemXBar width here
    # would create a fourfold, non-RTL interconnect bottleneck.
    width=64 if (venus_config['rtl_scalar_core'] or
                 timing_l1_dma_requested) else 16,
)
if venus_config['rtl_scalar_core']:
    # Local tile address decode is combinational in venus_block_wrapper.
    system.membus.frontend_latency = 0
    system.membus.forward_latency = 0
    system.membus.response_latency = 0
    system.membus.header_latency = 0
system.physmem_xbar = NoncoherentXBar(
    clk_domain=tile_clk_domain,
    forward_latency=0,
    response_latency=0,
    frontend_latency=0,
    width=64,
    # This timing xbar feeds SystemXBar.  Concurrent Scheduler-CPU I/D
    # traffic can make the downstream response layer reject a response;
    # retain it until the architectural retry instead of dropping it and
    # later issuing a retry to a queue that was never blocked.
    buffer_responses=True,
)
system.physmem = SimpleMemory(
    range=AddrRange(physmem_bytes), clk_domain=system.clk_domain,
)
system.membus.mem_side_ports[0] = system.physmem_xbar.cpu_side_ports
system.physmem_xbar.mem_side_ports = system.physmem.port
system.multi_thread = bool(dag_manifest)

if dag_manifest:
    tasks = [task for entry in dag_manifests for task in entry['tasks']]
    use_l1_timing_dma = timing_l1_dma_requested

    # A task descriptor has no architectural affinity unless every task
    # explicitly supplies one.  In the v5 RTL profile, model the four
    # physical gc0802 tiles so VenusDagScheduler can reproduce the RTL
    # allocator's runtime selection instead of silently defaulting every
    # task to tile 0.
    all_tasks_explicitly_pinned = all('tile_id' in task for task in tasks)
    dynamic_rtl_tile_topology = (
        dag_manifest.get('version', 1) >= 5 and
        args.venus_config == 'venus-rtl-16x128' and
        not all_tasks_explicitly_pinned
    )
    if dynamic_rtl_tile_topology:
        tile_count = RTL_16X128_DYNAMIC_TILE_COUNT
        # An empty vector is the scheduler ABI's unassigned-tile marker.
        task_tile_ids = []
        # Every tile needs a context for every task: the runtime allocator
        # may legally dispatch any task onto any eligible physical tile.
        tasks_by_tile = [list(range(len(tasks))) for _ in range(tile_count)]
        task_context_ids = list(range(len(tasks)))
        tile_hardware_capabilities = [
            RTL_16X128_TILE_HARDWARE_CAPABILITY
        ] * tile_count
        print('Using four runtime-assignable RTL tiles (tile0..tile3)')
    else:
        # Preserve explicit static pinning and all legacy/non-RTL DAG paths.
        task_tile_ids = [task.get('tile_id', 0) for task in tasks]
        tile_count = max(task_tile_ids) + 1
        tasks_by_tile = [[] for _ in range(tile_count)]
        for task_id, tile_id in enumerate(task_tile_ids):
            tasks_by_tile[tile_id].append(task_id)
        task_context_ids = [0] * len(tasks)
        for tile_id, task_ids in enumerate(tasks_by_tile):
            for context_id, task_id in enumerate(task_ids):
                task_context_ids[task_id] = context_id
        tile_hardware_capabilities = (
            [RTL_16X128_TILE_HARDWARE_CAPABILITY] * tile_count
            if args.venus_config == 'venus-rtl-16x128' else []
        )

    # One byte backing store is the common DMT/shared-L2 plane. Tile-local
    # SRAM is intentionally never shared.
    system.VenusSharedL2 = VenusSharedL2()
    tile_cpus = []
    for tile_id, task_ids in enumerate(tasks_by_tile):
        cpu = build_tile(tile_id, len(task_ids), system.VenusSharedL2)
        if tile_id == 0:
            system.cpu = cpu
        else:
            setattr(system, f'tile_cpu_{tile_id}', cpu)
        tile_cpus.append(cpu)

    if use_l1_timing_dma:
        # The shared byte backing has a real timing port in the physical RTL
        # L2 address window.  Keep this separate from the generic SE memory
        # pool and from every tile-local SRAM range.
        l2_port_index = 1 + tile_count * 3
        system.membus.mem_side_ports[l2_port_index] = (
            system.VenusSharedL2.logical_port
        )
        system.VenusL1DmaEngine = VenusL1DmaEngine(
            clk_domain=system.clk_domain,
        )
        system.VenusL1DmaEngine.read_port = system.membus.cpu_side_ports
        system.VenusL1DmaEngine.write_port = system.membus.cpu_side_ports

    binaries = [task['elf'] for task in tasks]
    system.workload = SEWorkload.init_compatible(binaries[0])
    if dynamic_rtl_tile_topology:
        # Do not share a Process between physical CPUs.  Each workload list
        # is task-id ordered, making context id == task id on every tile.
        processes_by_tile = []
        for tile_id in range(tile_count):
            tile_processes = []
            for task_id, binary in enumerate(binaries):
                process = Process(pid=100 + tile_id * len(tasks) + task_id)
                process.cmd = [binary]
                tile_processes.append(process)
            processes_by_tile.append(tile_processes)
        for tile_id, cpu in enumerate(tile_cpus):
            cpu.workload = processes_by_tile[tile_id]
            cpu.createThreads()
    else:
        processes = []
        for task_id, binary in enumerate(binaries):
            process = Process(pid=100 + task_id)
            process.cmd = [binary]
            processes.append(process)
        for tile_id, cpu in enumerate(tile_cpus):
            cpu.workload = [processes[task_id]
                            for task_id in tasks_by_tile[tile_id]]
            cpu.createThreads()

    system.system_port = system.membus.cpu_side_ports
    all_task_context_ids = task_context_ids
    all_task_tile_ids = task_tile_ids

    def make_dag_scheduler(dag_manifest, offset, secondary):
        dep_inputs = dag_manifest['dependency_inputs']
        initial_inputs = dag_manifest.get('initial_inputs', [])
        dag_outputs = (firmware_return_outputs(dag_manifest)
                       if args.scheduler_firmware_elf
                       else dag_manifest.get('outputs', []))
        tasks = dag_manifest['tasks']
        use_l1_timing_dma = timing_l1_dma_requested
        timing_fires = timing_fire_vectors(dag_manifest) if use_l1_timing_dma else {
            'tasks': [], 'ordinals': [], 'kinds': [], 'input_types': [],
            'source_kinds': [], 'source_offsets': [], 'destinations': [],
            'lengths': [], 'parents': [], 'ports': [], 'pointer_bytes': [],
        }
        dmt_layout = dag_manifest.get('dmt_layout', {})
        dmt_slots = dmt_layout.get('slots', [])
        if use_l1_timing_dma and (not dmt_slots or
                                  'malloc_base' not in dmt_layout):
            raise ValueError(
                'v5 timing DMA requires semantic dmt_layout from the runtime DAG')
        # The v5 fire plan includes every code/data/direct/pointer transfer.
        # Do not also load legacy per-task initial-input files: older manifests
        # materialized some of those from an RTL DMA capture, and a timing run
        # must derive its live transfers solely from the semantic plan.
        scheduler_initial_inputs = [] if use_l1_timing_dma else initial_inputs
        task_image_keys = [('code_file' in task, 'data_file' in task)
                           for task in tasks]
        if any(code != data for code, data in task_image_keys):
            raise ValueError('each DAG task must provide both code_file and data_file')
        if any(code for code, _ in task_image_keys) and not all(
                code for code, _ in task_image_keys):
            raise ValueError('task code/data images must be provided for every DAG task')
        has_task_images = all(code for code, _ in task_image_keys)

        task_context_ids = all_task_context_ids[offset:offset + len(tasks)]
        task_tile_ids = (all_task_tile_ids[offset:offset + len(tasks)]
                         if all_task_tile_ids else [])
        return VenusDagScheduler(
            clk_domain=system.clk_domain,
            # Legacy singular fields retain compatibility for diagnostic callers;
            # the vector fields are the authoritative live multi-tile topology.
            cpu=tile_cpus[0],
            sequencer=tile_cpus[0].VenusSequencer,
            vrf=tile_cpus[0].vrf,
            shared_l2_object=system.VenusSharedL2,
            l1_dma_engine=(system.VenusL1DmaEngine
                           if use_l1_timing_dma else NULL),
            use_l1_timing_dma=use_l1_timing_dma,
            rtl_runtime_dmt_length=venus_config.get('rtl_runtime_dmt_length', False),
            autostart=not bool(args.scheduler_firmware_elf),
            firmware_secondary=secondary,
            firmware_identity_addresses=[s['address'] for s in dag_manifest.get('firmware_identity', [])],
            firmware_identity_files=[s['file'] for s in dag_manifest.get('firmware_identity', [])],
            tile_cpus=tile_cpus,
            tile_sequencers=[cpu.VenusSequencer for cpu in tile_cpus],
            tile_vrfs=[cpu.vrf for cpu in tile_cpus],
            task_context_ids=task_context_ids,
            task_names=[task['name'] for task in tasks],
            task_code_files=[task['code_file'] for task in tasks]
                if has_task_images else [],
            task_data_files=[task['data_file'] for task in tasks]
                if has_task_images else [],
            task_tile_ids=task_tile_ids,
            task_hardware_requirements=[
                task.get('hardware_requirement', 0) for task in tasks
            ],
            task_need_spmd=[bool(task.get('need_spmd', False)) for task in tasks],
            task_minimum_spmd_tasks=[
                task.get('minimum_spmd_tasks', 0) for task in tasks
            ],
            tile_hardware_capabilities=tile_hardware_capabilities,
            task_crcs=[task['crc'] for task in tasks],
            task_code_sources=[task.get('code_source', 0) for task in tasks],
            task_data_sources=[task.get('data_source', 0) for task in tasks],
            timing_fire_tasks=timing_fires['tasks'],
            timing_fire_ordinals=timing_fires['ordinals'],
            timing_fire_kinds=timing_fires['kinds'],
            timing_fire_input_types=timing_fires['input_types'],
            timing_fire_source_kinds=timing_fires['source_kinds'],
            timing_fire_source_offsets=timing_fires['source_offsets'],
            timing_fire_destinations=timing_fires['destinations'],
            timing_fire_lengths=timing_fires['lengths'],
            timing_fire_parents=timing_fires['parents'],
            timing_fire_ports=timing_fires['ports'],
            timing_fire_pointer_bytes=timing_fires['pointer_bytes'],
            dmt_malloc_base=dmt_layout.get('malloc_base', 0),
            dmt_slot_tasks=[entry['task'] for entry in dmt_slots],
            dmt_slot_ports=[entry['port'] for entry in dmt_slots],
            dmt_slot_relative_offsets=[entry['relative_offset']
                                       for entry in dmt_slots],
            dmt_slot_capacities=[entry['capacity'] for entry in dmt_slots],
            input_tasks=[entry['task'] for entry in dep_inputs],
            input_parents=[entry['parent'] for entry in dep_inputs],
            input_ports=[entry['port'] for entry in dep_inputs],
            input_destinations=[entry['destination'] for entry in dep_inputs],
            input_lengths=[entry.get('length', 0) for entry in dep_inputs],
            input_types=[entry.get('type', 0) for entry in dep_inputs],
            input_descriptor_indices=[entry.get('descriptor_index', 0)
                                      for entry in dep_inputs],
            input_pointer_files=(['' for _ in dep_inputs] if use_l1_timing_dma
                                 else [entry.get('pointer_file', '')
                                       for entry in dep_inputs]),
            input_slot_addresses=[entry.get('slot_address', 0)
                                  for entry in dep_inputs],
            input_slot_consumer_bytes=[entry.get(
                'slot_consumer_bytes', entry.get(
                    'slot_capacity', entry.get('length', 0)
                    if entry.get('type', 0) == 0 else 0))
                for entry in dep_inputs],
            input_slot_address_valid=[
                entry.get('slot_address_valid', 'slot_address' in entry)
                for entry in dep_inputs
            ],
            initial_input_tasks=[entry['task'] for entry in scheduler_initial_inputs],
            initial_input_destinations=[entry['destination']
                                        for entry in scheduler_initial_inputs],
            initial_input_files=[entry['file'] for entry in scheduler_initial_inputs],
            output_tasks=[entry['task'] for entry in dag_outputs],
            output_ports=[entry['port'] for entry in dag_outputs],
            output_sources=[entry['source'] for entry in dag_outputs],
            output_lengths=[entry['length'] for entry in dag_outputs],
            output_counts=[task['output_count'] for task in tasks]
                if dag_manifest.get('version', 1) >= 2 else [],
            task_done_visibility_cycles=venus_config.get(
                'task_done_visibility_cycles', 2
            ),
            task_done_visibility_uses_tile_clock=venus_config.get(
                'task_done_visibility_uses_tile_clock', False
            ),
            reset_task_pipeline_before_fire=venus_config.get(
                'reset_task_pipeline_before_fire', False
            ),
            task_reset_vector=venus_config.get('task_reset_vector', 0),
            trace_file=dag_manifest['trace_file'],
            output_dump_dir=dag_manifest.get('output_dump_dir', ''),
        )
    firmware_dag_objects = []
    context_offset = 0
    for dag_index, entry in enumerate(dag_manifests):
        scheduler = make_dag_scheduler(entry, context_offset, dag_index != 0)
        setattr(system, 'VenusDagScheduler' if dag_index == 0 else
                f'firmware_dag_{dag_index}', scheduler)
        firmware_dag_objects.append(scheduler)
        context_offset += len(entry['tasks'])
    if args.scheduler_firmware_elf:
        # The SoC Scheduler is a separate PicoRV32 master, not a fifth Venus
        # tile and not the L2 task fire/return DMA.  Give it an independent
        # CPU and DMA master on the same system fabric.
        system.scheduler_cpu = MinorCPU(
            numThreads=1, clk_domain=system.clk_domain
        )
        system.scheduler_cpu.createInterruptController()
        system.scheduler_cpu.interrupts[0].venus_pico_irq = True
        system.scheduler_cpu.interrupts[0].venus_pico_irq_vector = (
            args.venus_pico_irq_vector
        )
        system.scheduler_cpu.icache_port = system.membus.cpu_side_ports
        system.scheduler_cpu.dcache_port = system.membus.cpu_side_ports
        scheduler_process = Process(pid=1)
        scheduler_process.cmd = [args.scheduler_firmware_elf]
        system.scheduler_cpu.workload = scheduler_process
        system.scheduler_cpu.createThreads()

        system.VenusFirmwareDmaEngine = VenusL1DmaEngine(
            clk_domain=system.clk_domain,
        )
        system.VenusFirmwareDmaEngine.read_port = (
            system.membus.cpu_side_ports
        )
        system.VenusFirmwareDmaEngine.write_port = (
            system.membus.cpu_side_ports
        )
        firmware_trace = args.scheduler_firmware_trace or os.path.join(
            os.path.dirname(dag_manifest['trace_file']),
            'venus_scheduler_firmware_trace.jsonl',
        )
        system.VenusSchedulerFirmware = VenusSchedulerFirmware(
            clk_domain=system.clk_domain,
            # Physical alias for SE MMIO pages.  The CPU continues to see
            # the real 0x1ffe.../0x21ff... addresses through its page table.
            pio_addr=0xc0000000,
            pio_latency='4ns',
            scheduler_cpu=system.scheduler_cpu,
            dma_engine=system.VenusFirmwareDmaEngine,
            dag_scheduler=system.VenusDagScheduler,
            dag_registry=firmware_dag_objects,
            shared_l2_object=system.VenusSharedL2,
            firmware_ram_size=venus_config.get(
                'scheduler_firmware_ram_size', '1MiB'),
            completion_gpio_mask=(
                args.scheduler_firmware_completion_gpio_mask
            ),
            trace_file=firmware_trace,
        )
        system.membus.mem_side_ports[l2_port_index + 1] = (
            system.VenusSchedulerFirmware.pio
        )
        print(
            'Scheduler engine: immutable l1.elf with PicoIRQ/MMIO/timing DMA'
        )
else:
    if args.standalone_shared_l2:
        system.VenusSharedL2 = VenusSharedL2()
        system.cpu = build_tile(0, 1, system.VenusSharedL2)
    else:
        system.cpu = build_tile(0, 1, None)
    system.system_port = system.membus.cpu_side_ports
    binary = args.binary or 'tests/test-progs/riscv32/single_test.elf'
    system.workload = SEWorkload.init_compatible(binary)
    process = Process()
    process.cmd = [binary]
    system.cpu.workload = process
    system.cpu.createThreads()
    if args.venus_pico_irq_tick:
        if not args.venus_pico_irq_cause:
            raise ValueError(
                '--venus-pico-irq-tick requires --venus-pico-irq-cause'
            )
        system.cpu.interrupts[0].venus_pico_irq = True
        system.cpu.interrupts[0].venus_pico_irq_vector = (
            args.venus_pico_irq_vector
        )

root = Root(full_system=False, system=system)
m5.instantiate()
print('Beginning simulation!')
if args.wfi_wake_tick and args.venus_pico_irq_tick:
    raise ValueError(
        '--wfi-wake-tick and --venus-pico-irq-tick are mutually exclusive'
    )
if args.venus_pico_irq_tick:
    if args.venus_pico_irq_tick <= m5.curTick():
        raise ValueError(
            '--venus-pico-irq-tick must be later than the current tick'
        )
    if args.max_ticks and args.venus_pico_irq_tick >= args.max_ticks:
        raise ValueError(
            '--venus-pico-irq-tick must be earlier than --max-ticks'
        )

    exit_event = m5.simulate(args.venus_pico_irq_tick - m5.curTick())
    if m5.curTick() != args.venus_pico_irq_tick:
        print(
            f'Exiting @ tick {m5.curTick()} before Pico IRQ because '
            f'{exit_event.getCause()}'
        )
    else:
        print(
            f'Posting Scheduler Pico IRQ '
            f'cause=0x{args.venus_pico_irq_cause:08x} '
            f'@ tick {m5.curTick()}'
        )
        system.cpu.postVenusPicoIrq(0, args.venus_pico_irq_cause)
        remaining = args.max_ticks - m5.curTick() if args.max_ticks else 0
        exit_event = m5.simulate(remaining) if remaining else m5.simulate()
elif args.wfi_wake_tick:
    if args.wfi_wake_tick <= m5.curTick():
        raise ValueError('--wfi-wake-tick must be later than the current tick')
    if args.max_ticks and args.wfi_wake_tick >= args.max_ticks:
        raise ValueError('--wfi-wake-tick must be earlier than --max-ticks')

    exit_event = m5.simulate(args.wfi_wake_tick - m5.curTick())
    if m5.curTick() != args.wfi_wake_tick:
        print(
            f'Exiting @ tick {m5.curTick()} before WFI wake because '
            f'{exit_event.getCause()}'
        )
    else:
        print(f'Presenting scalar600 WFI wake @ tick {m5.curTick()}')
        system.cpu.requestRtlWfiWake(0)
        remaining = args.max_ticks - m5.curTick() if args.max_ticks else 0
        exit_event = m5.simulate(remaining) if remaining else m5.simulate()
else:
    exit_event = m5.simulate(args.max_ticks) if args.max_ticks else m5.simulate()
print(f'Exiting @ tick {m5.curTick()} because {exit_event.getCause()}')
