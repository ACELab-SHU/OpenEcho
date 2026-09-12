from m5.objects.ClockedObject import ClockedObject
from m5.params import *
from m5.proxy import Parent

MaxNrLanes = 64

class VenusSequencer(ClockedObject):
    type = 'VenusSequencer'
    cxx_header = "venus/VenusSequencer.hh"
    cxx_class = 'gem5::VenusSequencer'

    port_venussequencer_receivefrom_venuspacketgen = ResponsePort("receives venus instruction from SpiritRV32")
    port_venussequencer_sendto_venuslane = VectorRequestPort("send venus instruction to VenusLane")
    port_venussequencer_sendto_venusshuffle = RequestPort("send venus instruction to VenusShuffle")
    port_venussequencer_hazardtable_boardcast = VectorRequestPort("hazard table boardcasting port")
    port_venussequencer_hazardtable_boardcast_to_shuffle = RequestPort("hazard table boardcasting port")

    max_count = Param.Int(10, "How many ticks to run before stopping")
    vrf_object = Param.SimObject(None)
    vrf_xbar_object = Param.SimObject(
        NULL, "VRF crossbar receiving explicit LSU-priority bank intents"
    )
    shuffle_object = Param.SimObject(
        NULL, "Shuffle engine whose task-local RTL state follows tile reset"
    )
    shared_l2_object = Param.SimObject(NULL)
    experimental_requester_q_visibility = Param.Bool(
        False,
        "Model registered lane-done visibility at the RTL sequencer boundary"
    )
    rtl_registered_pe_response = Param.Bool(
        False,
        "Model the V1 pe_resp_i_q to pe_vinsn_running_q retirement pipeline"
    )
    rtl_scalar_second_word_boundary = Param.Bool(
        False,
        "Delay a packed 64-bit scalar Venus request until its second word retires"
    )
    rtl_scalar_dispatch_capacity = Param.Unsigned(
        2,
        "CPU-to-Venus dispatcher input spill-register capacity"
    )
    rtl_direct_downstream_return = Param.Bool(
        False,
        "Return from DOWNSTREAM_RECEIVE without a non-RTL acknowledgement stage"
    )
    rtl_lane_desync_stall = Param.Bool(
        False,
        "Block sequencer return while lane 0 is done and another lane is live"
    )
    rtl_all_lane_issue_handshake = Param.Bool(
        False,
        "Broadcast ordinary vector requests to every RTL lane before local-VL shutdown"
    )
    rtl_dispatcher_inclusive_tail = Param.Bool(
        False,
        "Use the Venus1 dispatcher head + truncated-quotient inclusive tail scoreboard"
    )
    rtl_task_soft_reset = Param.Bool(
        False,
        "Reset task-local Venus arbitration state at each RTL tile dispatch"
    )
    system = Param.System(Parent.any, "System used for functional VLOAD/VSTORE memory access")
