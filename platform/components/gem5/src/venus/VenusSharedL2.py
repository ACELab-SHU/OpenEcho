from m5.objects.SimObject import SimObject
from m5.params import *


class VenusSharedL2(SimObject):
    type = 'VenusSharedL2'
    cxx_header = 'venus/VenusSharedL2.hh'
    cxx_class = 'gem5::VenusSharedL2'

    capacity = Param.MemorySize(
        '32MiB', 'Bytes in the shared L2/DMT backing store'
    )
    logical_base = Param.Addr(
        0x80000000,
        'Physical base exported by the shared-L2 timing port',
    )
    logical_port = ResponsePort(
        'Physical shared-L2 timing responder for scheduler/DMA traffic'
    )
    logical_latency = Param.Latency(
        '1ns', 'Latency of one shared-L2 logical-port access'
    )
