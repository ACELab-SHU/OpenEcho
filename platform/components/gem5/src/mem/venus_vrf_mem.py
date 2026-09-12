

from m5.objects.DVFSHandler import *
from m5.objects.SimpleMemory import *
from m5.objects.Workload import StubWorkload
from m5.params import *
from m5.proxy import *
from m5.SimObject import *
class venus_vrf_mem(SimObject):
    type = 'venus_vrf_mem'
    cxx_header = "mem/venus_vrf_mem.hh"
    cxx_class = 'gem5::memory::venus_vrf_mem'
    # clk_domain = Param.ClockDomain( "Clock domain")
    
    memories = VectorParam.AbstractMemory(
        Self.all, "All memories in the system"
    )
    
    mmap_using_noreserve = Param.Bool(
        False, "mmap the backing store without reserving swap"
    )
    shared_backstore = Param.String(
        "",
        "backstore's shmem segment filename, "
        "use to directly address the backstore from another host-OS process. "
        "Leave this empty to unset the MAP_SHARED flag.",
    )
    auto_unlink_shared_backstore = Param.Bool(
        False,
        "Automatically remove the "
        "shmem segment file upon destruction. This is used only if "
        "shared_backstore is non-empty.",
    )
    # max_count = Param.Int(10, "How many ticks to run before stopping")
    # m_venus_sequencer = Param.TickedObject(None)  
    lane_num = Param.Int(4, "venus tile lane number")
    bank_num = Param.Int(4, "venus tile bank number per lane")
    line_num = Param.Int(512, "venus tile bank line number")
    logical_base = Param.Addr(
        0x80100000,
        "Physical block address of this tile's scalar/DMA VSPM window",
    )
    logical_port = ResponsePort(
        "Scalar/DMA logical VSPM port (RTL venus_mem2lanes view)"
    )
    logical_latency = Param.Latency(
        '1ns', "Latency of the scalar/DMA logical VSPM access"
    )
