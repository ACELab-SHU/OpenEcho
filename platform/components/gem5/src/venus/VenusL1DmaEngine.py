from m5.objects.ClockedObject import ClockedObject
from m5.params import *
from m5.proxy import Parent


class VenusL1DmaEngine(ClockedObject):
    """Timing-visible L1 copy engine for the Venus scheduler fabric.

    The engine deliberately inherits its clock domain.  A configuration that
    models the RTL scheduler/DMA domain should attach this object to the
    250 MHz scheduler clock domain; no transfer latency is encoded here.
    """

    type = "VenusL1DmaEngine"
    cxx_header = "venus/VenusL1DmaEngine.hh"
    cxx_class = "gem5::VenusL1DmaEngine"

    system = Param.System(Parent.any, "System owning this DMA requestor")
    read_port = RequestPort("Timing request port for source reads")
    write_port = RequestPort("Timing request port for destination writes")

    beat_bytes = Param.Unsigned(64, "Maximum bytes in one DMA beat")
    fifo_entries = Param.Unsigned(
        256, "Maximum read beats buffered or in flight before write completion"
    )
    max_burst_beats = Param.Unsigned(
        64, "Maximum source-read beats outstanding in one DMA burst window"
    )
