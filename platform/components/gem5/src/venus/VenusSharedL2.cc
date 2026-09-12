#include "venus/VenusSharedL2.hh"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <limits>

#include "base/logging.hh"

namespace gem5
{

VenusSharedL2::LogicalPort::LogicalPort(const std::string &portName,
                                        VenusSharedL2 &owner)
    : SimpleTimingPort(portName, &owner), owner(owner)
{
}

Tick
VenusSharedL2::LogicalPort::recvAtomic(PacketPtr pkt)
{
    return owner.accessLogicalPort(pkt);
}

AddrRangeList
VenusSharedL2::LogicalPort::getAddrRanges() const
{
    return owner.logicalAddrRanges();
}

VenusSharedL2::VenusSharedL2(const VenusSharedL2Params &p)
    : SimObject(p),
      bytes(p.capacity, 0),
      logicalBase(p.logical_base),
      logicalPort(name() + ".logical_port", *this),
      logicalLatency(p.logical_latency)
{
    fatal_if(bytes.empty(), "VenusSharedL2 capacity must be nonzero");
    fatal_if(logicalBase > std::numeric_limits<Addr>::max() - bytes.size(),
             "VenusSharedL2 logical range overflows: base 0x%llx, size 0x%llx",
             static_cast<unsigned long long>(logicalBase),
             static_cast<unsigned long long>(bytes.size()));
}

void
VenusSharedL2::init()
{
    SimObject::init();
    const char *path = std::getenv("VENUS_GEM5_SHARED_L2_FILE");
    if (path != nullptr && path[0] != '\0') {
        std::ifstream input(path, std::ios::binary);
        fatal_if(!input, "VenusSharedL2 cannot open shared L2 image %s", path);
        input.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
        const size_t loaded = static_cast<size_t>(input.gcount());
        fatal_if(!input.eof() && input.fail(),
                 "VenusSharedL2 failed while reading shared L2 image %s", path);
        inform("VenusSharedL2 loaded %llu shared-L2 bytes from %s\n",
               static_cast<unsigned long long>(loaded), path);
    }

    // Existing functional DAG runs do not wire this port yet.  Do not turn a
    // new timing capability into a configuration requirement; when an L2 DMA
    // engine connects it, advertise the physical range in the normal gem5
    // response-port manner.
    if (logicalPort.isConnected())
        logicalPort.sendRangeChange();
}

Port &
VenusSharedL2::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "logical_port")
        return logicalPort;
    return SimObject::getPort(if_name, idx);
}

AddrRangeList
VenusSharedL2::logicalAddrRanges() const
{
    return {AddrRange(logicalBase, logicalBase + bytes.size())};
}

std::deque<VenusSharedL2::PointerRecord> *
VenusSharedL2::pointerFifoForRead(Addr offset, size_t accessSize)
{
    // The wrapper exposes each pointer channel as exactly one 512-bit word.
    // Partial accesses remain ordinary byte-backing accesses so a generic
    // debugger/functional request never consumes half a FIFO element.
    if (accessSize != PointerRecordBytes)
        return nullptr;
    if (offset == PtrGlobalOffset)
        return &ptrGlobalFifo;
    if (offset == PtrTempOffset)
        return &ptrTempFifo;
    return nullptr;
}

Tick
VenusSharedL2::accessLogicalPort(PacketPtr pkt)
{
    const Addr addr = pkt->getAddr();
    const size_t accessSize = pkt->getSize();
    fatal_if(addr < logicalBase,
             "VenusSharedL2 logical access 0x%llx precedes base 0x%llx",
             static_cast<unsigned long long>(addr),
             static_cast<unsigned long long>(logicalBase));
    const Addr offset = addr - logicalBase;
    fatal_if(offset > bytes.size() || accessSize > bytes.size() - offset,
             "VenusSharedL2 logical access [0x%llx, +%llu] exceeds range "
             "[0x%llx, 0x%llx)",
             static_cast<unsigned long long>(addr),
             static_cast<unsigned long long>(accessSize),
             static_cast<unsigned long long>(logicalBase),
             static_cast<unsigned long long>(logicalBase + bytes.size()));

    if (pkt->isRead()) {
        uint8_t *data = pkt->getPtr<uint8_t>();
        if (auto *fifo = pointerFifoForRead(offset, accessSize)) {
            fatal_if(fifo->empty(),
                     "VenusSharedL2 pointer FIFO underflow at physical 0x%llx",
                     static_cast<unsigned long long>(addr));
            std::copy_n(fifo->front().data(), PointerRecordBytes, data);
            // Keep the last consumed record visible through the ordinary
            // byte backing as well.  This makes packet DMA and the legacy
            // direct read()/write() view observationally consistent.
            write(offset, PointerRecordBytes, data);
            fifo->pop_front();
        } else {
            read(offset, accessSize, data);
        }
    } else if (pkt->isWrite()) {
        // Pointer-window writes are normal shared-L2 writes.  Only a full
        // read consumes a queued wrapper pointer record.  writeData()
        // honors the Request byte-enable mask used by RTL narrow DMA beats.
        pkt->writeData(bytes.data() + offset);
    } else {
        panic("unsupported command %s on VenusSharedL2 logical port",
              pkt->cmdString());
    }

    if (pkt->needsResponse())
        pkt->makeResponse();
    return logicalLatency;
}

void
VenusSharedL2::read(Addr addr, size_t accessSize, uint8_t *data) const
{
    fatal_if(addr > bytes.size() || accessSize > bytes.size() - addr,
             "shared L2 read [0x%llx, +%llu] exceeds modeled capacity 0x%llx",
             static_cast<unsigned long long>(addr),
             static_cast<unsigned long long>(accessSize),
             static_cast<unsigned long long>(bytes.size()));
    std::copy_n(bytes.data() + addr, accessSize, data);
}

void
VenusSharedL2::write(Addr addr, size_t accessSize, const uint8_t *data)
{
    fatal_if(addr > bytes.size() || accessSize > bytes.size() - addr,
             "shared L2 write [0x%llx, +%llu] exceeds modeled capacity 0x%llx",
             static_cast<unsigned long long>(addr),
             static_cast<unsigned long long>(accessSize),
             static_cast<unsigned long long>(bytes.size()));
    std::copy_n(data, accessSize, bytes.data() + addr);
}

VenusSharedL2::LsuReadBurstSchedule
VenusSharedL2::reserveLsuReadBurst(
    Tick requestTick, unsigned int beats, Tick axiPeriod)
{
    fatal_if(beats == 0, "VenusSharedL2 cannot schedule an empty LSU burst");
    fatal_if(axiPeriod == 0,
             "VenusSharedL2 LSU AXI period must be nonzero");

    /*
     * RTL path from the cluster-facing external AR handshake to the
     * cluster-facing external R handshake, in physical cluster-AXI clocks:
     *
     *   shared AR timing layer                         1
     *   slave AR capture / READ_STATE_BURST entry     1
     *   SRAM request and registered data return       2
     *   slave R registration                          1
     *   shared R timing layer                         1
     *
     * The tile-local CDC FIFOs and axi_cut are deliberately not included
     * here; VenusSequencer owns those requester-specific clock boundaries.
     * venus_cluster_axi4.tcl enables AXI_AR_SHARED_PL and
     * AXI_R_SHARED_PL. axi_ram_if_venus.sv supplies the registered slave
     * state and response boundary. These are named structural stages, not
     * a DAG-, task-, or opcode-level fitted latency.
     */
    constexpr unsigned int ArSharedStages = 1;
    constexpr unsigned int SlaveCommandStages = 1;
    constexpr unsigned int SramReadStages = 2;
    constexpr unsigned int SlaveResponseStages = 1;
    constexpr unsigned int RSharedStages = 1;
    constexpr unsigned int FirstResponseStages =
        ArSharedStages + SlaveCommandStages + SramReadStages +
        SlaveResponseStages + RSharedStages;

    /*
     * R beats are contiguous. After RLAST, the slave consumes one edge to
     * return to IDLE and one edge to launch a queued burst's first response.
     */
    constexpr unsigned int InterBurstRestartStages = 2;

    Tick first = requestTick + FirstResponseStages * axiPeriod;
    if (lsuSlaveDataTailValid) {
        if (lsuSlaveDataTailKind == LsuSlaveTailKind::Read) {
            first = std::max(
                first,
                lsuSlaveDataTailTick +
                    InterBurstRestartStages * axiPeriod);
        } else {
            /*
             * WLAST makes write_state_next IDLE; ARREADY is registered on
             * the following edge.  The new read then traverses the slave
             * command, SRAM-return, response and shared-R boundaries.  The
             * shared AR layer was already allowed to queue the address while
             * the write was active, so do not count it a second time here.
             */
            constexpr unsigned int WriteToReadRestartStages =
                SlaveCommandStages + SramReadStages +
                SlaveResponseStages + RSharedStages + 1;
            first = std::max(
                first,
                lsuSlaveDataTailTick +
                    WriteToReadRestartStages * axiPeriod);
        }
    }
    const Tick last = first + (beats - 1) * axiPeriod;
    lsuSlaveDataTailTick = last;
    lsuSlaveDataTailValid = true;
    lsuSlaveDataTailKind = LsuSlaveTailKind::Read;
    return {first, last};
}

VenusSharedL2::LsuWriteBurstSchedule
VenusSharedL2::reserveLsuWriteBurst(
    uintptr_t requesterKey, Tick earliestLocalWriteTick,
    Tick addressAxiSampleTick,
    unsigned int beats, Tick tilePeriod, Tick axiPeriod)
{
    fatal_if(beats == 0, "VenusSharedL2 cannot schedule an empty LSU write");
    fatal_if(tilePeriod == 0,
             "VenusSharedL2 LSU tile period must be nonzero");
    fatal_if(axiPeriod == 0,
             "VenusSharedL2 LSU AXI period must be nonzero");
    fatal_if(requesterKey == 0,
             "VenusSharedL2 LSU write requester key must be nonzero");

    /*
     * RTL does not turn a VSTU-side W handshake directly into a fixed-delay
     * B response.  W crosses the tile CDC and axi_cut, then competes for the
     * shared slave's write-data channel.  DW_axi routes W through the active
     * write-ID state established by AW.  The source-side Gray pointer is a
     * registered boundary of its own: the destination synchronizer cannot
     * sample a W payload on the same aligned edge which publishes that
     * pointer.  It is followed by two synchronizer stages, the CDC spill
     * register, and axi_cut.  With only one physical AXI edge between the
     * aligned AW and first-W samples, DW_axi's TMO=1 register slice accepts
     * beat 0 while mask_valid_o is still asserted, then holds beat 1 for one
     * edge while the buffered beat is reissued.  If the AW route has had more
     * time to settle, every W beat is accepted contiguously.  That replay
     * changes W spacing, but does not add another slave B stage after WLAST.
     * The requester-local B axi_cut and CDC belong to VenusSequencer, where
     * their distinct clock boundaries remain visible.
     *
     * task20 sequence 2 exposes every boundary:
     *
     *   local W0/W1       edges 38/40
     *   external W0/W1   edges 54/62 (early-W mask inserts one AXI edge)
     *   external B       edge 78
     *   internal B       edge 92
     *
     * task20 sequence 4 is the complementary long-operand case: its AW is
     * already routed before W arrives and all eight external beats are
     * accepted on consecutive AXI clocks (edges 86 through 114).
     *
     * This object owns only the cluster-shared W wire.  A different master
     * may use the next AXI edge after WLAST.  The longer same-master VSTU
     * restart belongs to requester-local state; placing it here stalls
     * unrelated tiles even though their AW/data are already queued.
     */
    constexpr unsigned int OutboundWStagesAfterSample = 4;
    constexpr unsigned int InterBurstRestartStages = 1;
    constexpr unsigned int ReadToWriteRestartStages = 1;
    constexpr unsigned int WriteToWriteRestartStages = 2;
    constexpr unsigned int SlaveBResponseStages = 3;
    constexpr unsigned int SourceFifoDepth = 8;
    constexpr unsigned int ReadPointerSyncTileStages = 2;
    auto alignAxiEdge = [axiPeriod](Tick candidate) {
        const Tick phase = candidate % axiPeriod;
        return candidate + (axiPeriod - phase) % axiPeriod;
    };
    auto alignTileEdge = [tilePeriod](Tick candidate) {
        const Tick phase = candidate % tilePeriod;
        return candidate + (tilePeriod - phase) % tilePeriod;
    };

    /*
     * i_cdc_w is an eight-entry Gray-pointer FIFO. VSTU may publish one
     * local W beat per tile clock, while the destination consumes at most
     * one beat per AXI clock. Once all eight slots are outstanding,
     * src_ready_o waits for the destination read pointer to cross the
     * two-stage synchronizer back into the tile domain.
     */
    auto &releaseTicks = lsuWriteSourceReleaseTicks[requesterKey];
    Tick local = earliestLocalWriteTick;
    Tick firstLocal = 0;
    Tick lastLocal = 0;
    Tick first = 0;
    Tick last = 0;
    bool earlyRouteReplay = false;
    bool sourceBackpressured = false;
    unsigned int sourceFullStallCount = 0;
    for (unsigned int beat = 0; beat < beats; ++beat) {
        if (beat != 0)
            local = lastLocal + tilePeriod;
        while (!releaseTicks.empty() && releaseTicks.front() <= local)
            releaseTicks.pop_front();
        if (releaseTicks.size() >= SourceFifoDepth) {
            sourceBackpressured = true;
            ++sourceFullStallCount;
            local = alignTileEdge(releaseTicks.front());
            while (!releaseTicks.empty() && releaseTicks.front() <= local)
                releaseTicks.pop_front();
        }

        const Tick firstAxiSample = alignAxiEdge(local);
        const bool pointerChangesOnSample = local == firstAxiSample;
        Tick external = firstAxiSample +
            (OutboundWStagesAfterSample +
             (pointerChangesOnSample ? 1 : 0)) * axiPeriod;
        if (beat == 0) {
            external = std::max(external, addressAxiSampleTick);
            if (lsuWriteDataTailValid) {
                external = std::max(
                    external,
                    lsuWriteDataTailTick +
                        InterBurstRestartStages * axiPeriod);
            }
            if (lsuSlaveDataTailValid) {
                const unsigned int restartStages =
                    lsuSlaveDataTailKind == LsuSlaveTailKind::Read
                    ? ReadToWriteRestartStages
                    : WriteToWriteRestartStages;
                external = std::max(
                    external,
                    lsuSlaveDataTailTick + restartStages * axiPeriod);
            }
            earlyRouteReplay =
                firstAxiSample <= addressAxiSampleTick + axiPeriod;
            first = external;
            firstLocal = local;
        } else {
            external = std::max(
                external,
                last + (earlyRouteReplay && beat == 1 ? 2 : 1) *
                    axiPeriod);
        }

        lastLocal = local;
        last = external;
        const Tick fifoReadTick = external - axiPeriod;
        releaseTicks.push_back(
            alignTileEdge(fifoReadTick) +
                ReadPointerSyncTileStages * tilePeriod);
    }
    /*
     * Both route states enter the same three-stage slave B path after WLAST.
     * The early-W case has already paid its route-selection penalty through
     * the explicit W replay above; adding another B stage double-counts that
     * registered boundary.  This is selected only from AW/W timing, never
     * from the task, address, payload, or burst length.
     */
    const Tick externalResponse =
        last + SlaveBResponseStages * axiPeriod;
    lsuWriteDataTailTick = last;
    lsuWriteDataTailValid = true;
    lsuSlaveDataTailTick = last;
    lsuSlaveDataTailValid = true;
    lsuSlaveDataTailKind = LsuSlaveTailKind::Write;
    const unsigned int sourceOutstandingAtEnd = releaseTicks.size();
    const Tick sourceNextReleaseTick = releaseTicks.empty()
        ? lastLocal : releaseTicks.front();
    return {firstLocal, lastLocal, first, last, externalResponse,
            sourceBackpressured, sourceFullStallCount,
            sourceOutstandingAtEnd, sourceNextReleaseTick};
}

bool
VenusSharedL2::enqueuePointer(std::deque<PointerRecord> &fifo,
                              const uint8_t *data, size_t accessSize)
{
    fatal_if(data == nullptr, "VenusSharedL2 pointer FIFO enqueue has null data");
    fatal_if(accessSize != PointerRecordBytes,
             "VenusSharedL2 pointer FIFO record is %llu bytes, expected %llu",
             static_cast<unsigned long long>(accessSize),
             static_cast<unsigned long long>(PointerRecordBytes));
    if (fifo.size() >= PointerFifoDepth)
        return false;

    PointerRecord record{};
    std::copy_n(data, PointerRecordBytes, record.data());
    fifo.push_back(record);
    return true;
}

bool
VenusSharedL2::enqueuePtrGlobal(const uint8_t *data, size_t accessSize)
{
    return enqueuePointer(ptrGlobalFifo, data, accessSize);
}

bool
VenusSharedL2::enqueuePtrTemp(const uint8_t *data, size_t accessSize)
{
    return enqueuePointer(ptrTempFifo, data, accessSize);
}

} // namespace gem5
