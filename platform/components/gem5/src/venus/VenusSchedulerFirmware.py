from m5.objects.Device import BasicPioDevice
from m5.params import *
from m5.proxy import Parent


class VenusSchedulerFirmware(BasicPioDevice):
    type = "VenusSchedulerFirmware"
    cxx_header = "venus/VenusSchedulerFirmware.hh"
    cxx_class = "gem5::VenusSchedulerFirmware"

    scheduler_cpu = Param.BaseCPU(
        Parent.any, "MinorCPU executing the immutable Scheduler l1.elf"
    )
    dma_engine = Param.SimObject(
        Parent.any, "Independent SoC L1 DMA engine programmed by firmware"
    )
    dag_scheduler = Param.SimObject(
        Parent.any, "Semantic L2 Scheduler/DMT model started by firmware"
    )
    dag_registry = VectorParam.SimObject([], "ELF-qualified firmware DAG registry")
    shared_l2_object = Param.SimObject(
        Parent.any, "Cluster shared-L2 backing reached by SoC DMA"
    )
    hardware_base = Param.Addr(
        0x1FFE0000, "Lowest scheduler-visible SoC MMIO address"
    )
    hardware_size = Param.MemorySize(
        "32896KiB", "Span through the cluster control aperture"
    )
    descriptor_fifo_depth = Param.Unsigned(
        256, "RTL DMA descriptor FIFO entries"
    )
    firmware_ram_base = Param.Addr(
        0x10000000, "Scheduler CPU cacheable/uncacheable RAM aperture base"
    )
    firmware_ram_size = Param.MemorySize(
        "1MiB", "Scheduler CPU RAM aperture described by the l1 linker"
    )
    irq_dma = Param.UInt32(1 << 11, "Scheduler PicoIRQ DMA-done cause")
    irq_cluster = Param.UInt32(1 << 3, "Scheduler PicoIRQ cluster-0 cause")
    completion_gpio_mask = Param.UInt32(
        0,
        "Optional external GPIO completion mask; zero leaves firmware running",
    )
    trace_file = Param.String(
        "venus_scheduler_firmware_trace.jsonl",
        "Scheduler CPU/MMIO/DMA semantic event trace",
    )
