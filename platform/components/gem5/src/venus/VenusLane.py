from m5.objects.ClockedObject import ClockedObject
from m5.params import *

class VenusLane(ClockedObject):
    type = 'VenusLane'
    cxx_header = "venus/VenusLane.hh"
    cxx_class = 'gem5::VenusLane'

    port_venuslane_receivefrom_venussequencer = ResponsePort("receives venus instruction from VenusSequencer")
    port_venuslane_hazardtable_listen = ResponsePort("listen hazard table radio boardcast")
    port_venuslane_receivefrom_venusshuffle = ResponsePort("receives shuffle operation from VenusShuffle")
    port_venuslanetovrf_16 = RequestPort('WB_ShuffleUnit to vrf')
    port_venuslanetovrf_15 = RequestPort('WB_SerDiv to vrf')
    port_venuslanetovrf_14 = RequestPort('WB_BITALUMask to vrf')
    port_venuslanetovrf_13 = RequestPort('WB_BITALU to vrf')
    port_venuslanetovrf_12 = RequestPort('WB_CAU_B to vrf')
    port_venuslanetovrf_11 = RequestPort('WB_CAU_A to vrf')
    port_venuslanetovrf_10 = RequestPort('Mask to vrf')
    port_venuslanetovrf_9 = RequestPort('ShuffleUnit to vrf')
    port_venuslanetovrf_8 = RequestPort('SerDiv_B to vrf')
    port_venuslanetovrf_7 = RequestPort('SerDiv_A to vrf')
    port_venuslanetovrf_6 = RequestPort('CAU_D to vrf')
    port_venuslanetovrf_5 = RequestPort('CAU_C to vrf')
    port_venuslanetovrf_4 = RequestPort('CAU_B to vrf')
    port_venuslanetovrf_3 = RequestPort('CAU_A to vrf')
    port_venuslanetovrf_2 = RequestPort('BitAlu_B to vrf')
    port_venuslanetovrf_1 = RequestPort('BitAlu_A to vrf')


    max_count = Param.Int(10, "How many ticks to run before stopping")
    lane_num = Param.Int(4, "venus tile lane number")
    bank_num = Param.Int(4, "venus tile bank number per lane")
    line_num = Param.Int(32, "venus tile line number per lane")
    vrf_base_addr = Param.Addr(0x0,"venus vrf base addr from cpu point of view")
    lane_id = Param.Int(0, "venus lane id")
    experimental_vrf_rr = Param.Bool(
        False,
        "Enable the deferred per-bank VRF RR/retry research prototype",
    )
    experimental_requester_q_visibility = Param.Bool(
        False,
        "Expose tagged arithmetic requester_q commands one lane edge later",
    )
    experimental_one_entry_operand_commands = Param.Bool(
        False,
        "Keep each RTL operand command register occupied until requester ack",
    )
    rtl_pe_command_visibility_cycles = Param.Unsigned(
        1,
        "Lane clocks from PE request capture to operand/VFU command visibility",
    )
    rtl_chaining_enabled = Param.Bool(
        True,
        "Permit row-granular RAW credits implemented by the RTL requester",
    )

    venus_lane_simobject_1  = Param.SimObject(None)  
    venus_lane_simobject_2  = Param.SimObject(None)  
    venus_lane_simobject_3  = Param.SimObject(None)  
    venus_lane_simobject_4  = Param.SimObject(None)  
    venus_lane_simobject_5  = Param.SimObject(None)  
    venus_lane_simobject_6  = Param.SimObject(None)  
    venus_lane_simobject_7  = Param.SimObject(None)  
    venus_lane_simobject_8  = Param.SimObject(None)  
    venus_lane_simobject_9  = Param.SimObject(None)  
    venus_lane_simobject_10 = Param.SimObject(None)  
    venus_lane_simobject_11 = Param.SimObject(None)  
    venus_lane_simobject_12 = Param.SimObject(None)  
    venus_lane_simobject_13 = Param.SimObject(None)  
    venus_lane_simobject_14 = Param.SimObject(None)  
    venus_lane_simobject_15 = Param.SimObject(None)  
    venus_lane_simobject_16 = Param.SimObject(None)  
    venus_lane_simobject_17 = Param.SimObject(None)  
    venus_lane_simobject_18 = Param.SimObject(None)  
    venus_lane_simobject_19 = Param.SimObject(None)  
    venus_lane_simobject_20 = Param.SimObject(None)  
    venus_lane_simobject_21 = Param.SimObject(None)  
    venus_lane_simobject_22 = Param.SimObject(None)  
    venus_lane_simobject_23 = Param.SimObject(None)  
    venus_lane_simobject_24 = Param.SimObject(None)  
    venus_lane_simobject_25 = Param.SimObject(None)  
    venus_lane_simobject_26 = Param.SimObject(None)  
    venus_lane_simobject_27 = Param.SimObject(None)  
    venus_lane_simobject_28 = Param.SimObject(None)  
    venus_lane_simobject_29 = Param.SimObject(None)  
    venus_lane_simobject_30 = Param.SimObject(None)  
    venus_lane_simobject_31 = Param.SimObject(None)  
    venus_lane_simobject_32 = Param.SimObject(None)  
    venus_lane_simobject_33 = Param.SimObject(None)  
    venus_lane_simobject_34 = Param.SimObject(None)  
    venus_lane_simobject_35 = Param.SimObject(None)  
    venus_lane_simobject_36 = Param.SimObject(None)  
    venus_lane_simobject_37 = Param.SimObject(None)  
    venus_lane_simobject_38 = Param.SimObject(None)  
    venus_lane_simobject_39 = Param.SimObject(None)  
    venus_lane_simobject_40 = Param.SimObject(None)  
    venus_lane_simobject_41 = Param.SimObject(None)  
    venus_lane_simobject_42 = Param.SimObject(None)  
    venus_lane_simobject_43 = Param.SimObject(None)  
    venus_lane_simobject_44 = Param.SimObject(None)  
    venus_lane_simobject_45 = Param.SimObject(None)  
    venus_lane_simobject_46 = Param.SimObject(None)  
    venus_lane_simobject_47 = Param.SimObject(None)  
    venus_lane_simobject_48 = Param.SimObject(None)  
    venus_lane_simobject_49 = Param.SimObject(None)  
    venus_lane_simobject_50 = Param.SimObject(None)  
    venus_lane_simobject_51 = Param.SimObject(None)  
    venus_lane_simobject_52 = Param.SimObject(None)  
    venus_lane_simobject_53 = Param.SimObject(None)  
    venus_lane_simobject_54 = Param.SimObject(None)  
    venus_lane_simobject_55 = Param.SimObject(None)  
    venus_lane_simobject_56 = Param.SimObject(None)  
    venus_lane_simobject_57 = Param.SimObject(None)  
    venus_lane_simobject_58 = Param.SimObject(None)  
    venus_lane_simobject_59 = Param.SimObject(None)  
    venus_lane_simobject_60 = Param.SimObject(None)  
    venus_lane_simobject_61 = Param.SimObject(None)  
    venus_lane_simobject_62 = Param.SimObject(None)  
    venus_lane_simobject_63 = Param.SimObject(None) 

    venus_shuffle_pipeline = Param.SimObject(None) 
