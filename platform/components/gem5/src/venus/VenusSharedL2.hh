#ifndef __VENUS_SHARED_L2_HH__
#define __VENUS_SHARED_L2_HH__

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/addr_range.hh"
#include "base/types.hh"
#include "mem/tport.hh"
#include "params/VenusSharedL2.hh"
#include "sim/sim_object.hh"

namespace gem5
{

/**
 * The L2/DMT byte store visible to every physical Venus tile.
 *
 * A VenusSequencer used to own an independent copy of this image.  That was
 * harmless only while one sequencer time-multiplexed every task.  Real tile
 * concurrency needs one storage object so a producer's LDU/STU and the L2
 * scheduler's DMT publication are visible to all tiles.
 */
class VenusSharedL2 : public SimObject
{
  public:
    // The physical cluster-L2 address space visible to RTL DMA masters.
    static constexpr Addr PhysicalBase = 0x80000000ULL;
    static constexpr Addr PtrGlobalOffset = 0x001ff8540ULL;
    static constexpr Addr PtrTempOffset = 0x001ff8580ULL;
    static constexpr size_t PointerRecordBytes = 64;
    static constexpr size_t PointerFifoDepth = 16;

    struct LsuReadBurstSchedule
    {
        Tick firstResponseTick;
        Tick lastResponseTick;
    };

    struct LsuWriteBurstSchedule
    {
        Tick firstLocalWriteTick;
        Tick lastLocalWriteTick;
        Tick firstExternalWriteTick;
        Tick lastExternalWriteTick;
        Tick externalResponseTick;
        bool sourceBackpressured;
        unsigned int sourceFullStallCount;
        unsigned int sourceOutstandingAtEnd;
        Tick sourceNextReleaseTick;
    };

  private:
    /**
     * A small timing responder for the physical L2 view.  It deliberately
     * uses SimpleTimingPort for now: the forthcoming L2-DMA engine supplies
     * serialization and completion ordering, while this object owns the one
     * byte backing store that every access must observe.
     */
    class LogicalPort : public SimpleTimingPort
    {
      private:
        VenusSharedL2 &owner;

      protected:
        Tick recvAtomic(PacketPtr pkt) override;
        AddrRangeList getAddrRanges() const override;

      public:
        LogicalPort(const std::string &name, VenusSharedL2 &owner);
    };

    using PointerRecord = std::array<uint8_t, PointerRecordBytes>;

    std::vector<uint8_t> bytes;
    const Addr logicalBase;
    LogicalPort logicalPort;
    const Tick logicalLatency;

    // These are the task_manager_wrapper pointer-data channels, not normal
    // shared-L2 locations.  A 64-byte read at either window consumes one
    // queued record; ordinary direct read()/write() calls always operate on
    // the byte backing by offset.
    std::deque<PointerRecord> ptrGlobalFifo;
    std::deque<PointerRecord> ptrTempFifo;

    /*
     * The cluster L2 is reached through the shared AR/R layers of the
     * cluster DW_axi and the single-burst AXI SRAM slave. Keep the response
     * resource here so all physical tile sequencers observe one slave.
     */
    enum class LsuSlaveTailKind
    {
        Read,
        Write
    };

    /*
     * axi4_memory_wrapper is instantiated with IS_RW_1CHN=1 for the shared
     * SRAM.  Reads and writes therefore occupy one common slave data FSM;
     * separate R and W tails would incorrectly let a write drain while an
     * older read burst is still active.
     */
    Tick lsuSlaveDataTailTick = 0;
    bool lsuSlaveDataTailValid = false;
    LsuSlaveTailKind lsuSlaveDataTailKind = LsuSlaveTailKind::Read;
    Tick lsuWriteDataTailTick = 0;
    bool lsuWriteDataTailValid = false;
    // Source-visible slot-release ticks for each tile's eight-entry W CDC.
    std::unordered_map<uintptr_t, std::deque<Tick>>
        lsuWriteSourceReleaseTicks;

    Tick accessLogicalPort(PacketPtr pkt);
    AddrRangeList logicalAddrRanges() const;
    std::deque<PointerRecord> *pointerFifoForRead(Addr offset,
                                                  size_t accessSize);
    bool enqueuePointer(std::deque<PointerRecord> &fifo,
                        const uint8_t *data, size_t accessSize);

  public:
    VenusSharedL2(const VenusSharedL2Params &p);
    void init() override;
    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;

    // These retain the legacy sequencer ABI: addr is a shared-L2 *offset*,
    // not a PhysicalBase-relative address.
    void read(Addr addr, size_t size, uint8_t *data) const;
    void write(Addr addr, size_t size, const uint8_t *data);
    size_t size() const { return bytes.size(); }

    /*
     * Reserve one cluster-L2 AXI read burst. requestTick is the external,
     * cluster-facing AR handshake after the tile CDC and axi_cut; the returned
     * ticks are the corresponding external R handshakes before the return
     * axi_cut/CDC. axiPeriod is the physical cluster AXI period.
     */
    LsuReadBurstSchedule reserveLsuReadBurst(
        Tick requestTick, unsigned int beats, Tick axiPeriod);

    /*
     * Reserve the shared external W channel and return the structural W/B
     * boundaries. firstLocalWriteTick is the first VSTU-side W handshake;
     * addressAxiSampleTick and firstAxiSampleTick are respectively the AW
     * descriptor and first W edge aligned to the physical AXI phase.
     */
    LsuWriteBurstSchedule reserveLsuWriteBurst(
        uintptr_t requesterKey, Tick earliestLocalWriteTick,
        Tick addressAxiSampleTick,
        unsigned int beats, Tick tilePeriod, Tick axiPeriod);

    // Return false on FIFO backpressure.  Callers must retry after the DMA
    // read which consumes a record; silently dropping a pointer would make a
    // dependency appear valid with the wrong source.
    bool enqueuePtrGlobal(const uint8_t *data, size_t size);
    bool enqueuePtrTemp(const uint8_t *data, size_t size);
    bool ptrGlobalFull() const { return ptrGlobalFifo.size() >= PointerFifoDepth; }
    bool ptrTempFull() const { return ptrTempFifo.size() >= PointerFifoDepth; }
    size_t ptrGlobalPending() const { return ptrGlobalFifo.size(); }
    size_t ptrTempPending() const { return ptrTempFifo.size(); }
};

} // namespace gem5

#endif // __VENUS_SHARED_L2_HH__
