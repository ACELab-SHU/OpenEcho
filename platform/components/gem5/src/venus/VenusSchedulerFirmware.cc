#include "venus/VenusSchedulerFirmware.hh"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "base/logging.hh"
#include "cpu/base.hh"
#include "cpu/minor/dyn_inst.hh"
#include "cpu/minor/cpu.hh"
#include "cpu/thread_context.hh"
#include "arch/generic/mmu.hh"
#include "mem/page_table.hh"
#include "sim/process.hh"
#include "sim/sim_exit.hh"
#include "venus/VenusDagScheduler.hh"
#include "venus/VenusL1DmaEngine.hh"
#include "venus/VenusSharedL2.hh"

namespace gem5
{

VenusSchedulerFirmware::VenusSchedulerFirmware(
    const VenusSchedulerFirmwareParams &p)
    : BasicPioDevice(p, p.hardware_size), schedulerCpu(p.scheduler_cpu),
      schedulerMinor(dynamic_cast<MinorCPU *>(p.scheduler_cpu)),
      dmaEngine(dynamic_cast<VenusL1DmaEngine *>(p.dma_engine)),
      dagScheduler(dynamic_cast<VenusDagScheduler *>(p.dag_scheduler)),
      sharedL2(dynamic_cast<VenusSharedL2 *>(p.shared_l2_object)),
      hardwareBase(p.hardware_base), hardwareSize(p.hardware_size),
      descriptorFifoDepth(p.descriptor_fifo_depth),
      firmwareRamBase(p.firmware_ram_base),
      firmwareRamSize(p.firmware_ram_size), irqDma(p.irq_dma),
      irqCluster(p.irq_cluster),
      completionGpioMask(p.completion_gpio_mask), trace(p.trace_file)
{
    fatal_if(!schedulerCpu || !schedulerMinor || !dmaEngine || !dagScheduler ||
                 !sharedL2,
             "VenusSchedulerFirmware requires MinorCPU, DMA, DAG scheduler, and shared L2");
    fatal_if(descriptorFifoDepth == 0,
             "Scheduler firmware DMA descriptor FIFO must be nonzero");
    fatal_if(hardwareBase != DmaBase ||
                 hardwareSize < ClusterCfg + 0x1000 - hardwareBase,
             "Scheduler firmware MMIO span does not cover the hardware contract");
    fatal_if(!trace, "cannot open Scheduler firmware trace %s", p.trace_file);
    for (auto *object : p.dag_registry) {
        auto *dag = dynamic_cast<VenusDagScheduler *>(object);
        fatal_if(!dag || std::find(dagRegistry.begin(), dagRegistry.end(), dag) != dagRegistry.end(),
                 "invalid or duplicate firmware DAG registry member");
        dagRegistry.push_back(dag);
    }
    fatal_if(dagRegistry.empty() || dagRegistry.front() != dagScheduler,
             "firmware execution requires an ELF-qualified registry with primary first");
}

void
VenusSchedulerFirmware::startup()
{
    fatal_if(schedulerCpu->numContexts() != 1,
             "Scheduler firmware CPU must expose exactly one context");
    schedulerContext = schedulerCpu->getContext(0);
    Process *process = schedulerContext->getProcessPtr();
    fatal_if(!process, "Scheduler firmware CPU has no SE process");

    // The ELF loader maps only LOAD segments.  The real Scheduler linker
    // nevertheless exposes the entire 1-MiB cacheable/uncacheable SRAM
    // aperture to heap and stack code, including zero-filled holes after the
    // last LOAD byte.  Back every otherwise-unmapped page without replacing
    // the ELF pages already populated by the loader.
    const Addr pageBytes = process->pTable->pageSize();
    fatal_if(firmwareRamBase % pageBytes || firmwareRamSize % pageBytes,
             "Scheduler firmware RAM aperture is not page aligned");
    for (Addr address = firmwareRamBase;
         address < firmwareRamBase + firmwareRamSize;
         address += pageBytes) {
        if (!process->pTable->lookup(address))
            process->allocateMem(address, pageBytes);
    }

    // Only CPU-visible register pages are mapped.  DMA payload addresses in
    // the 0x2000_0000 aperture are translated by the DMA callback below and
    // therefore never become artificial PIO transactions.
    const auto mapMmio = [this, process](Addr begin, Addr end) {
        constexpr Addr PageBytes = 4096;
        fatal_if(begin % PageBytes || end % PageBytes || end < begin,
                 "invalid Scheduler MMIO page range");
        for (Addr address = begin; address < end; address += PageBytes) {
            process->pTable->map(
                address, pioAddr + (address - hardwareBase), PageBytes,
                EmulationPageTable::Clobber |
                    EmulationPageTable::Uncacheable);
        }
    };
    mapMmio(0x1ffe0000ULL, 0x20000000ULL);
    mapMmio(0x21ff0000ULL, 0x22000000ULL);
    schedulerContext->getMMUPtr()->flushAll();

    for (auto *dag : dagRegistry)
        dag->setFirmwareCompletionCallback([this] { completeDag(); });
    emit("firmware_mmio_ready",
         "\"descriptor_fifo_depth\":" +
             std::to_string(descriptorFifoDepth) +
             ",\"ram_base\":" + std::to_string(firmwareRamBase) +
             ",\"ram_size\":" + std::to_string(firmwareRamSize));
}

Addr
VenusSchedulerFirmware::logicalAddress(PacketPtr pkt) const
{
    const Addr physical = pkt->getAddr();
    fatal_if(physical < pioAddr || physical >= pioAddr + hardwareSize,
             "Scheduler firmware PIO address %#llx is outside its alias",
             static_cast<unsigned long long>(physical));
    return hardwareBase + (physical - pioAddr);
}

uint32_t
VenusSchedulerFirmware::readRegister(Addr address) const
{
    if (address == DmaStat) {
        const size_t occupied = descriptors.size() + (dmaEngine->isBusy() ? 1 : 0);
        return occupied >= descriptorFifoDepth ? 1u : 0u;
    }
    if (address == DmaError)
        return 0;
    if (address == ClusterIntStatus)
        return clusterInterruptStatus;
    if (address >= L2ReturnBase && address < L2ReturnBase + 16 * 8) {
        const unsigned index = (address - L2ReturnBase) / 8;
        const bool length = ((address - L2ReturnBase) & 4) != 0;
        if (index >= returns.size())
            return 0;
        return length ? returns[index].length
                      : static_cast<uint32_t>(returns[index].offset);
    }
    const auto found = registerShadow.find(address & ~Addr(3));
    uint32_t value = found == registerShadow.end() ? 0 : found->second;
    // CCM PLL reads expose the lock bit after configuration.  This is the
    // documented peripheral state transition used by generic boot firmware.
    if (address >= 0x1fff0000ULL && address < 0x1fff000cULL)
        value |= 0x80000000u;
    return value;
}

Tick
VenusSchedulerFirmware::read(PacketPtr pkt)
{
    fatal_if(pkt->getSize() != 1 && pkt->getSize() != 2 && pkt->getSize() != 4,
             "unsupported Scheduler firmware MMIO read size %u", pkt->getSize());
    const Addr address = logicalAddress(pkt);
    const uint32_t word = readRegister(address);
    const unsigned shift = (address & 3) * 8;
    pkt->setUintX(word >> shift, ByteOrder::little);
    pkt->makeAtomicResponse();
    return pioDelay;
}

Tick
VenusSchedulerFirmware::write(PacketPtr pkt)
{
    fatal_if(pkt->getSize() != 1 && pkt->getSize() != 2 && pkt->getSize() != 4,
             "unsupported Scheduler firmware MMIO write size %u", pkt->getSize());
    const Addr address = logicalAddress(pkt);
    const uint32_t value = pkt->getUintX(ByteOrder::little);
    writeRegister(address, value);
    pkt->makeAtomicResponse();
    return pioDelay;
}

void
VenusSchedulerFirmware::writeRegister(Addr address, uint32_t value)
{
    if (address == GpioData || address == GpioDirection) {
        std::ostringstream extra;
        extra << "\"register\":\""
              << (address == GpioData ? "data" : "direction")
              << "\",\"value\":" << value;
        emit("gpio_write", extra.str());
        if (address == GpioData && completionGpioMask &&
                (value & completionGpioMask) == completionGpioMask) {
            emit("firmware_completion_gpio", extra.str());
            exitSimLoop("Scheduler firmware completion GPIO");
        }
    }
    if (address == 0x1fff1000ULL) {
        uartBytes.push_back(static_cast<char>(value & 0xff));
        constexpr char StopSequence[] = "$stop\0";
        constexpr size_t StopBytes = sizeof(StopSequence) - 1;
        if (uartBytes.size() >= StopBytes &&
            std::equal(StopSequence, StopSequence + StopBytes,
                       uartBytes.end() - StopBytes)) {
            emit("firmware_uart_stop");
            exitSimLoop("Scheduler firmware emitted $stop");
        }
    }
    if (address == DmaSrc)
        dmaSource = value;
    else if (address == DmaDst)
        dmaDestination = value;
    else if (address == DmaLen)
        dmaLength = value;
    else if (address == DmaCfg) {
        if (value == DmaPush || value == DmaLast)
            enqueueDescriptor(value == DmaLast);
        else if (value == DmaClear) {
            dmaInterruptAsserted = false;
            emit("dma_irq_clear");
        }
    } else if (address == L2Reset) {
        registerShadow[address] = value;
        if (value == 1)
            startDag();
    } else if (address == ClusterIntClear) {
        clusterInterruptStatus = 0;
        emit("cluster_irq_clear");
    } else {
        registerShadow[address & ~Addr(3)] = value;
    }
}

void
VenusSchedulerFirmware::enqueueDescriptor(bool last)
{
    fatal_if(descriptors.size() + (dmaEngine->isBusy() ? 1 : 0) >=
                 descriptorFifoDepth,
             "Scheduler firmware wrote a full DMA descriptor FIFO");
    Descriptor descriptor{dmaSource, dmaDestination, dmaLength, last,
                          nextDescriptorSequence++};
    descriptors.push_back(descriptor);
    std::ostringstream extra;
    extra << "\"sequence\":" << descriptor.sequence
          << ",\"source\":\"0x" << std::hex << descriptor.source
          << "\",\"destination\":\"0x" << descriptor.destination
          << "\",\"length\":" << std::dec << descriptor.length
          << ",\"last\":" << (last ? "true" : "false");
    emit("dma_descriptor_push", extra.str());
    maybeStartDescriptor();
}

Addr
VenusSchedulerFirmware::translateDmaAddress(Addr address, bool source) const
{
    if (address >= ClusterAperture &&
        address < ClusterAperture + ClusterApertureBytes)
        return VenusSharedL2::PhysicalBase + (address - ClusterAperture);
    Addr physical = 0;
    Process *process = schedulerContext->getProcessPtr();
    fatal_if(!process->pTable->translate(address, physical),
             "Scheduler firmware DMA %s address %#llx has no SE mapping",
             source ? "source" : "destination",
             static_cast<unsigned long long>(address));
    return physical;
}

void
VenusSchedulerFirmware::maybeStartDescriptor()
{
    if (dmaEngine->isBusy() || descriptors.empty())
        return;
    const Descriptor descriptor = descriptors.front();
    descriptors.pop_front();
    emit("dma_descriptor_start",
         "\"sequence\":" + std::to_string(descriptor.sequence));
    dmaEngine->startTransfer(
        descriptor.source, descriptor.destination, descriptor.length, {},
        [this, descriptor](const std::vector<uint8_t> &data) {
            completeDescriptor(descriptor, data);
        }, {}, [this](Addr address, bool source) {
            return translateDmaAddress(address, source);
        });
}

void
VenusSchedulerFirmware::completeDescriptor(
    const Descriptor &descriptor, const std::vector<uint8_t> &data)
{
    std::ostringstream extra;
    extra << "\"sequence\":" << descriptor.sequence
          << ",\"length\":" << data.size()
          << ",\"last\":" << (descriptor.last ? "true" : "false");
    emit("dma_descriptor_complete", extra.str());
    if (descriptor.last) {
        dmaInterruptAsserted = true;
        postIrq(irqDma);
    }
    maybeStartDescriptor();
}

void
VenusSchedulerFirmware::startDag()
{
    fatal_if(clusterRunning, "Scheduler firmware started a live cluster DAG");
    fatal_if(dmaEngine->isBusy() || !descriptors.empty(),
             "Scheduler firmware started DAG before DMA completion");
    activeDag = nullptr;
    for (auto *dag : dagRegistry) {
        if (!dag->matchesFirmware(readRegister(L2MallocInit)))
            continue;
        fatal_if(activeDag, "ambiguous staged firmware DAG identity");
        activeDag = dag;
    }
    fatal_if(!activeDag, "unknown or modified staged firmware DAG identity");
    dagScheduler->selectFirmwareDag(activeDag);
    ++fireId;
    clusterRunning = true;
    returns.clear();
    emit("cluster_dag_start", "\"dag\":\"" + activeDag->name() + "\"");
    activeDag->startFromFirmware(fireId);
}

void
VenusSchedulerFirmware::completeDag()
{
    fatal_if(!clusterRunning,
             "Venus DAG completed without a Scheduler firmware fire");
    clusterRunning = false;
    returns = activeDag->firmwareReturns();
    clusterInterruptStatus |= ClusterDagDone;
    emit("cluster_dag_complete",
         "\"return_count\":" + std::to_string(returns.size()));
    postIrq(irqCluster);
}

void
VenusSchedulerFirmware::postIrq(uint32_t cause)
{
    emit("pico_irq_post", "\"cause\":" + std::to_string(cause));
    schedulerMinor->postVenusPicoIrq(0, cause);
}

void
VenusSchedulerFirmware::emit(const char *event, const std::string &extra)
{
    trace << "{\"tick\":" << curTick() << ",\"event\":\"" << event
          << "\",\"fire_id\":" << fireId;
    if (!extra.empty())
        trace << ',' << extra;
    trace << "}\n";
    trace.flush();
}

} // namespace gem5
