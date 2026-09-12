#ifndef __VENUS_SCHEDULER_FIRMWARE_HH__
#define __VENUS_SCHEDULER_FIRMWARE_HH__

#include <cstdint>
#include <deque>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "dev/io_device.hh"
#include "params/VenusSchedulerFirmware.hh"
#include "venus/VenusDagScheduler.hh"

namespace gem5
{

class BaseCPU;
class MinorCPU;
class ThreadContext;
class VenusL1DmaEngine;
class VenusSharedL2;

/**
 * Scheduler-CPU peripheral bridge for the real GC0802 l1.elf.
 *
 * The object exposes the documented SoC MMIO registers to the SE process,
 * feeds firmware-authored descriptors through an independent timing DMA
 * engine, starts the existing semantic L2 Scheduler/DMT model on the real
 * reset-register write, and returns completion through PicoIRQ.  No task,
 * DAG name, firmware PC, or fitted delay participates in these decisions.
 */
class VenusSchedulerFirmware : public BasicPioDevice
{
  public:
    VenusSchedulerFirmware(const VenusSchedulerFirmwareParams &p);
    void startup() override;

  protected:
    Tick read(PacketPtr pkt) override;
    Tick write(PacketPtr pkt) override;

  private:
    struct Descriptor
    {
        Addr source;
        Addr destination;
        uint32_t length;
        bool last;
        uint64_t sequence;
    };

    BaseCPU *const schedulerCpu;
    MinorCPU *const schedulerMinor;
    VenusL1DmaEngine *const dmaEngine;
    VenusDagScheduler *const dagScheduler;
    std::vector<VenusDagScheduler *> dagRegistry;
    VenusDagScheduler *activeDag = nullptr;
    uint64_t fireId = 0;
    VenusSharedL2 *const sharedL2;
    const Addr hardwareBase;
    const Addr hardwareSize;
    const unsigned descriptorFifoDepth;
    const Addr firmwareRamBase;
    const Addr firmwareRamSize;
    const uint32_t irqDma;
    const uint32_t irqCluster;
    const uint32_t completionGpioMask;

    ThreadContext *schedulerContext = nullptr;
    Addr dmaSource = 0;
    Addr dmaDestination = 0;
    uint32_t dmaLength = 0;
    uint32_t clusterInterruptStatus = 0;
    bool dmaInterruptAsserted = false;
    bool clusterRunning = false;
    uint64_t nextDescriptorSequence = 0;
    std::deque<Descriptor> descriptors;
    std::vector<VenusDagScheduler::FirmwareReturn> returns;
    std::unordered_map<Addr, uint32_t> registerShadow;
    std::string uartBytes;
    std::ofstream trace;

    static constexpr Addr DmaBase = 0x1ffe0000ULL;
    static constexpr Addr GpioData = 0x1fff4000ULL;
    static constexpr Addr GpioDirection = GpioData + 4;
    static constexpr Addr DmaCfg = DmaBase + 0x00;
    static constexpr Addr DmaSrc = DmaBase + 0x08;
    static constexpr Addr DmaDst = DmaBase + 0x10;
    static constexpr Addr DmaLen = DmaBase + 0x18;
    static constexpr Addr DmaStat = DmaBase + 0x20;
    static constexpr Addr DmaError = DmaBase + 0x28;
    static constexpr uint32_t DmaPush = 1;
    static constexpr uint32_t DmaLast = 3;
    static constexpr uint32_t DmaClear = 4;

    static constexpr Addr ClusterAperture = 0x20000000ULL;
    static constexpr Addr ClusterApertureBytes = 0x02000000ULL;
    static constexpr Addr L2Cfg = 0x21ff0000ULL;
    static constexpr Addr L2Reset = L2Cfg + 0x8300;
    static constexpr Addr L2ReturnBase = L2Cfg + 0x8500;
    static constexpr Addr L2MallocInit = L2Cfg + 0x8600;
    static constexpr Addr L2OutputAddr = L2Cfg + 0x8700;
    static constexpr Addr ClusterCfg = 0x21fff000ULL;
    static constexpr Addr ClusterIntStatus = ClusterCfg + 0x08;
    static constexpr Addr ClusterIntClear = ClusterCfg + 0x10;
    static constexpr uint32_t ClusterDagDone = 1u << 14;

    Addr logicalAddress(PacketPtr pkt) const;
    Addr translateDmaAddress(Addr address, bool source) const;
    uint32_t readRegister(Addr address) const;
    void writeRegister(Addr address, uint32_t value);
    void enqueueDescriptor(bool last);
    void maybeStartDescriptor();
    void completeDescriptor(const Descriptor &descriptor,
                            const std::vector<uint8_t> &data);
    void startDag();
    void completeDag();
    void postIrq(uint32_t cause);
    void emit(const char *event, const std::string &extra = "");
};

} // namespace gem5

#endif // __VENUS_SCHEDULER_FIRMWARE_HH__
