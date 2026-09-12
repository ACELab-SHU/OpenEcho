from m5.params import *
from m5.objects.ClockedObject import ClockedObject

class VenusShufflePipline(ClockedObject):
    type = 'VenusShufflePipline'
    cxx_header = "venus/VenusShufflePipline.hh"
    cxx_class = "gem5::VenusShufflePipline"

    # 定义端口
    port_venusshuffle_receivefrom_venussequencer = ResponsePort("receives venus instruction from VenusSequencer")
    port_venusshuffle_hazardtable_listen = ResponsePort("receives venus instruction from hazardtable")
    lane_ports = VectorRequestPort("Ports to connect to Lane Memories")

    num_pes = Param.Int(4, "Number of Shuffle PEs (Parallel Execution Units)")
    num_banks_per_lane = Param.Int(4, "Number of Banks per Lane")
    line_num = Param.Int(512, "Number of 16-bit rows in each VRF bank")
    vrf_base_addr = Param.Addr(0x80100000, "Base address of physical VRF banks")
    registered_request_visibility = Param.Bool(
        False,
        "Expose shuffle req_q to lane arbitration on the following tile edge"
    )
    live_requester_intent = Param.Bool(
        False,
        "Expose LockIn=0 Shuffle intent separately from timing-port retry"
    )
    legacy_lockstep = Param.Bool(
        False,
        "Use the Venus1 global IDX/DATA/WRITE PE barriers"
    )
