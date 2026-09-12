from m5.objects.RiscvDecoder import RiscvDecoder
from m5.params import *

class RiscvVenusDecoder(RiscvDecoder):
    type = "RiscvVenusDecoder"
    cxx_class = "gem5::RiscvISA::RiscvVenusDecoder"
    cxx_header = "arch/riscv/venusdecoder.hh"
