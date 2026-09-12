#include "venus/VenusL1DmaEngine.hh"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <utility>

#include "base/logging.hh"
#include "debug/VenusL1Dma.hh"
#include "mem/request.hh"
#include "sim/system.hh"

namespace gem5
{

namespace
{

constexpr Addr PageBytes = 4096;
/* dma_axi_if presents returned AXI R/B state through two registered
 * scheduler-clock boundaries before the stream/FSM may consume it.  The
 * generic gem5 timing port returns an individual packet directly, so model
 * that interface boundary here rather than changing burst throughput. */
constexpr unsigned RtlAxiResponseVisibilityCycles = 2;

size_t
bytesToPageBoundary(Addr addr)
{
    return static_cast<size_t>(PageBytes - (addr & (PageBytes - 1)));
}

uint64_t
writeStrobe(const std::vector<bool> &byteEnable)
{
    fatal_if(byteEnable.size() > 64,
             "VenusL1DmaEngine cannot format a %zu-byte WSTRB",
             byteEnable.size());
    uint64_t strobe = 0;
    for (size_t index = 0; index < byteEnable.size(); ++index) {
        if (byteEnable[index])
            strobe |= uint64_t{1} << index;
    }
    return strobe;
}

} // anonymous namespace

VenusL1DmaEngine::DmaRequestPort::DmaRequestPort(
    const std::string &name, VenusL1DmaEngine &owner, Channel channel)
    : RequestPort(name, &owner), engine(owner), channel(channel)
{}

VenusL1DmaEngine::DmaRequestPort::~DmaRequestPort()
{
    if (blockedPacket != nullptr) {
        delete blockedPacket->senderState;
        delete blockedPacket;
    }
}

bool
VenusL1DmaEngine::DmaRequestPort::send(PacketPtr pkt)
{
    panic_if(blockedPacket != nullptr,
             "%s attempted to send while a request is awaiting retry", name());
    if (sendTimingReq(pkt))
        return true;

    blockedPacket = pkt;
    return false;
}

bool
VenusL1DmaEngine::DmaRequestPort::retryBlockedPacket()
{
    panic_if(blockedPacket == nullptr,
             "%s received recvReqRetry without a blocked request", name());

    // Clear the member before invoking the peer.  A legal timing responder
    // may return a response while sendTimingReq is unwinding.
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
        return false;
    }
    return true;
}

bool
VenusL1DmaEngine::DmaRequestPort::recvTimingResp(PacketPtr pkt)
{
    engine.handleResponse(channel, pkt);
    return true;
}

void
VenusL1DmaEngine::DmaRequestPort::recvReqRetry()
{
    engine.retry(channel);
}

VenusL1DmaEngine::VenusL1DmaEngine(const VenusL1DmaEngineParams &p)
    : ClockedObject(p), beatBytes(p.beat_bytes), fifoEntries(p.fifo_entries),
      maxBurstBeats(p.max_burst_beats), system(p.system),
      requestorId(system->getRequestorId(this)),
      readPort(p.name + ".read_port", *this, Channel::Read),
      writePort(p.name + ".write_port", *this, Channel::Write),
      admitEvent([this] { admitTransfer(); }, name() + ".admit"),
      stepEvent([this] { stepTransfer(); }, name() + ".step"),
      completeEvent([this] { completeTransfer(); }, name() + ".complete")
{
    fatal_if(beatBytes == 0, "VenusL1DmaEngine beat_bytes must be nonzero");
    fatal_if(fifoEntries == 0, "VenusL1DmaEngine fifo_entries must be nonzero");
    fatal_if(maxBurstBeats == 0,
             "VenusL1DmaEngine max_burst_beats must be nonzero");
    fatal_if(maxBurstBeats > fifoEntries,
             "VenusL1DmaEngine max_burst_beats (%u) exceeds fifo_entries (%u)",
             maxBurstBeats, fifoEntries);
}

VenusL1DmaEngine::~VenusL1DmaEngine()
{
    if (admitEvent.scheduled())
        deschedule(admitEvent);
    if (stepEvent.scheduled())
        deschedule(stepEvent);
    if (completeEvent.scheduled())
        deschedule(completeEvent);
}

Port &
VenusL1DmaEngine::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "read_port")
        return readPort;
    if (if_name == "write_port")
        return writePort;
    return ClockedObject::getPort(if_name, idx);
}

void
VenusL1DmaEngine::init()
{
    ClockedObject::init();
    panic_if(!readPort.isConnected(),
             "VenusL1DmaEngine %s read_port is not connected", name());
    panic_if(!writePort.isConnected(),
             "VenusL1DmaEngine %s write_port is not connected", name());
}

void
VenusL1DmaEngine::startTransfer(Addr src, Addr dst, size_t length,
                                AdmitCallback onAdmit,
                                CompleteCallback onComplete,
                                BeatLimitCallback beatLimit,
                                AddressTranslateCallback addressTranslate)
{
    fatal_if(busy, "VenusL1DmaEngine %s accepts only one active transfer", name());
    fatal_if(length != 0 &&
                 (length - 1 > std::numeric_limits<Addr>::max() - src ||
                  length - 1 > std::numeric_limits<Addr>::max() - dst),
             "VenusL1DmaEngine %s transfer address range overflows", name());

    // The RTL descriptor fields are 32-bit.  Its streamer treats a
    // descriptor of at least one full bus beat as an aligned transfer, and
    // treats a shorter descriptor as one narrow 64-byte AXI beat.  Reject
    // unsupported descriptors rather than silently performing a different
    // transaction sequence in gem5.
    constexpr Addr RtlAddressMax = std::numeric_limits<uint32_t>::max();
    fatal_if(src > RtlAddressMax || dst > RtlAddressMax ||
                 length > std::numeric_limits<uint32_t>::max() ||
                 (length != 0 &&
                  (length - 1 > RtlAddressMax - src ||
                   length - 1 > RtlAddressMax - dst)),
             "VenusL1DmaEngine %s received a descriptor outside RTL's "
             "32-bit address/length domain", name());
    if (length >= beatBytes) {
        fatal_if(src % beatBytes != 0 || dst % beatBytes != 0,
                 "VenusL1DmaEngine %s DMA_UNALIGNED_ERR: %zu-byte transfer "
                 "src=%#x dst=%#x is not %u-byte aligned", name(), length,
                 src, dst, beatBytes);
    } else if (length != 0) {
        const size_t srcOffset = static_cast<size_t>(src % beatBytes);
        const size_t dstOffset = static_cast<size_t>(dst % beatBytes);
        fatal_if(length > beatBytes - srcOffset ||
                     length > beatBytes - dstOffset,
                 "VenusL1DmaEngine %s DMA_NARROW_CROSS_ERR: %zu-byte "
                 "transfer src=%#x dst=%#x crosses a %u-byte AXI beat",
                 name(), length, src, dst, beatBytes);
    }

    busy = true;
    admitted = false;
    completing = false;
    streamerDescriptorLoaded = false;
    sourceBase = src;
    destinationBase = dst;
    transferLength = length;
    nextOffset = 0;
    readOutstanding = 0;
    writeOutstanding = 0;
    admitCallback = std::move(onAdmit);
    completeCallback = std::move(onComplete);
    beatLimitCallback = std::move(beatLimit);
    addressTranslateCallback = std::move(addressTranslate);
    completedData.assign(length, 0);
    activeBeats.clear();
    fifoSram.clear();
    writePipe = nullptr;
    readBurst = {};
    writeBurst = {};
    responsePipeline.clear();

    // The responder may call this from another clock domain.  Admission is
    // therefore sampled on the scheduler/DMA clock, not at the caller's tick.
    schedule(admitEvent, clockBoundary());
}

Tick
VenusL1DmaEngine::clockBoundary() const
{
    return clockEdge(Cycles(0));
}

Tick
VenusL1DmaEngine::nextClockBoundary() const
{
    return clockEdge(Cycles(1));
}

void
VenusL1DmaEngine::admitTransfer()
{
    panic_if(!busy || admitted,
             "VenusL1DmaEngine %s has no pending transfer to admit", name());
    admitted = true;

    DPRINTF(VenusL1Dma,
            "%s admit src=%#llx dst=%#llx bytes=%llu\n", name(),
            static_cast<unsigned long long>(sourceBase),
            static_cast<unsigned long long>(destinationBase),
            static_cast<unsigned long long>(transferLength));
    if (admitCallback)
        admitCallback();

    if (transferLength == 0) {
        scheduleStep();
    } else {
        /* dma_go is observed at this edge; the streamer begins its RUN/AR
         * sequence on a following DMA edge rather than issuing a packet in
         * the responder's call stack. */
        scheduleStep();
    }
}

std::unique_ptr<VenusL1DmaEngine::Beat>
VenusL1DmaEngine::makeNextBeat()
{
    panic_if(nextOffset >= transferLength,
             "VenusL1DmaEngine %s has no source bytes left", name());

    const Addr src = sourceBase + nextOffset;
    const Addr dst = destinationBase + nextOffset;
    const size_t remaining = transferLength - nextOffset;
    const size_t naturalBytes = std::min(
        {static_cast<size_t>(beatBytes), remaining, bytesToPageBoundary(src),
         bytesToPageBoundary(dst)});
    size_t logicalBytes = naturalBytes;
    if (beatLimitCallback) {
        logicalBytes = beatLimitCallback(src, dst, naturalBytes);
        panic_if(logicalBytes == 0 || logicalBytes > naturalBytes,
                 "VenusL1DmaEngine %s beat-boundary callback returned %zu "
                 "for a %zu-byte beat", name(), logicalBytes, naturalBytes);
        if (logicalBytes != naturalBytes) {
            DPRINTF(VenusL1Dma,
                    "%s split raw beat src=%#llx dst=%#llx normal=%llu "
                    "capped=%llu\n",
                    name(), static_cast<unsigned long long>(src),
                    static_cast<unsigned long long>(dst),
                    static_cast<unsigned long long>(naturalBytes),
                    static_cast<unsigned long long>(logicalBytes));
        }
    }
    panic_if(logicalBytes == 0,
             "VenusL1DmaEngine %s generated a zero-byte beat", name());

    // A legal RTL transfer can have a short final beat but never a short
    // middle beat: the DMA streamer emits full bus beats until its final
    // narrow transaction.  Decoder boundaries used by the scheduler are all
    // 64-byte aligned; failing here is safer than manufacturing a non-RTL
    // partial request if a future topology violates that invariant.
    fatal_if(logicalBytes < beatBytes && logicalBytes != remaining,
             "VenusL1DmaEngine %s cannot express a nonterminal %zu-byte "
             "RTL DMA beat at src=%#x dst=%#x", name(), logicalBytes,
             src, dst);

    const size_t sourceOffset = static_cast<size_t>(src % beatBytes);
    const size_t destinationOffset = static_cast<size_t>(dst % beatBytes);
    if (logicalBytes == beatBytes) {
        fatal_if(sourceOffset != 0 || destinationOffset != 0,
                 "VenusL1DmaEngine %s generated an unaligned full bus beat "
                 "at src=%#x dst=%#x", name(), src, dst);
    } else {
        fatal_if(logicalBytes > beatBytes - sourceOffset ||
                     logicalBytes > beatBytes - destinationOffset,
                 "VenusL1DmaEngine %s generated a narrow beat crossing its "
                 "%u-byte bus boundary", name(), beatBytes);
    }

    auto beat = std::make_unique<Beat>();
    // `dma_streamer.sv` always presents a complete DATA_BUS_WIDTH beat to
    // AXI, including the final short transfer.  It aligns both ends down,
    // strips the source offset after the read, and applies WSTRB at the
    // destination offset.  Preserve those physical addresses here.
    beat->src = src - sourceOffset;
    beat->dst = dst - destinationOffset;
    beat->bytes = beatBytes;
    beat->logicalBytes = logicalBytes;
    beat->sourceOffset = sourceOffset;
    beat->destinationOffset = destinationOffset;
    beat->dataOffset = nextOffset;
    beat->data.resize(beat->bytes);
    beat->writeData.assign(beat->bytes, 0);
    beat->writeByteEnable.assign(beat->bytes, true);
    if (logicalBytes != beat->bytes) {
        std::fill(beat->writeByteEnable.begin(),
                  beat->writeByteEnable.end(), false);
        std::fill_n(beat->writeByteEnable.begin() + destinationOffset,
                    logicalBytes, true);
    }
    nextOffset += logicalBytes;
    return beat;
}

PacketPtr
VenusL1DmaEngine::makePacket(Beat &beat, Channel channel)
{
    const Addr logicalAddress =
        channel == Channel::Read ? beat.src : beat.dst;
    const Addr address = addressTranslateCallback
        ? addressTranslateCallback(logicalAddress, channel == Channel::Read)
        : logicalAddress;
    RequestPtr req = std::make_shared<Request>(
        address, beat.bytes, Request::UNCACHEABLE, requestorId);
    if (channel == Channel::Write && beat.logicalBytes != beat.bytes)
        req->setByteEnable(beat.writeByteEnable);
    PacketPtr pkt = new Packet(
        req, channel == Channel::Read ? MemCmd::ReadReq : MemCmd::WriteReq);
    pkt->senderState = new BeatSenderState(&beat, channel);
    if (channel == Channel::Read) {
        // A timing read response populates packet-owned storage.
        pkt->allocate();
    } else {
        // The masked, bus-width write storage survives until its response.
        pkt->dataStatic(beat.writeData.data());
    }
    return pkt;
}

void
VenusL1DmaEngine::markAccepted(Beat &beat, Channel channel)
{
    if (channel == Channel::Read) {
        panic_if(beat.readInFlight || beat.readResponseArrived,
                 "VenusL1DmaEngine %s duplicate read request", name());
        beat.readInFlight = true;
        ++readOutstanding;
    } else {
        panic_if(!beat.readResponseArrived || beat.writeInFlight ||
                     beat.writeResponseArrived,
                 "VenusL1DmaEngine %s invalid write request", name());
        beat.writeInFlight = true;
        ++writeOutstanding;
    }
}

void
VenusL1DmaEngine::undoAccepted(Beat &beat, Channel channel)
{
    if (channel == Channel::Read) {
        panic_if(!beat.readInFlight || readOutstanding == 0,
                 "VenusL1DmaEngine %s invalid read retry rollback", name());
        beat.readInFlight = false;
        --readOutstanding;
    } else {
        panic_if(!beat.writeInFlight || writeOutstanding == 0,
                 "VenusL1DmaEngine %s invalid write retry rollback", name());
        beat.writeInFlight = false;
        --writeOutstanding;
    }
}

void
VenusL1DmaEngine::scheduleStep()
{
    if (busy && admitted && !completing && !stepEvent.scheduled())
        schedule(stepEvent, nextClockBoundary());
}

void
VenusL1DmaEngine::startReadBurst()
{
    if (readBurst.active || nextOffset == transferLength)
        return;

    const size_t fifoUsed = fifoSram.size() + (writePipe ? 1 : 0);
    if (fifoUsed == fifoEntries)
        return;

    const size_t burstLimit = std::min<size_t>(
        maxBurstBeats, fifoEntries - fifoUsed);
    const Addr logicalStart = sourceBase + nextOffset;
    const size_t sourcePageBytes = bytesToPageBoundary(logicalStart);
    const size_t byteLimit = std::min(
        sourcePageBytes, burstLimit * static_cast<size_t>(beatBytes));

    readBurst = {};
    readBurst.active = true;
    while (nextOffset < transferLength &&
           readBurst.beats.size() < burstLimit &&
           sourceBase + nextOffset - logicalStart < byteLimit) {
        auto beat = makeNextBeat();
        Beat *rawBeat = beat.get();
        activeBeats.push_back(std::move(beat));
        readBurst.beats.push_back(rawBeat);
    }

    fatal_if(readBurst.beats.empty(),
             "VenusL1DmaEngine %s created an empty read burst", name());
    DPRINTF(VenusL1Dma,
            "%s rd_ar src=%#llx beats=%llu split=%s\n", name(),
            static_cast<unsigned long long>(readBurst.beats.front()->src),
            static_cast<unsigned long long>(readBurst.beats.size()),
            readBurst.beats.size() < burstLimit ? "4k-or-terminal" : "limit");
}

void
VenusL1DmaEngine::startWriteBurst()
{
    if (writeBurst.active)
        return;

    Beat *first = writePipe;
    if (first == nullptr && !fifoSram.empty())
        first = fifoSram.front();
    if (first == nullptr)
        return;

    const size_t remaining = transferLength - first->dataOffset;
    const size_t terminalBeats =
        (remaining + beatBytes - 1) / static_cast<size_t>(beatBytes);
    const size_t destinationBeats = std::max<size_t>(1,
        bytesToPageBoundary(first->dst) / beatBytes);

    writeBurst = {};
    writeBurst.active = true;
    writeBurst.expected = std::min(
        {static_cast<size_t>(maxBurstBeats), terminalBeats, destinationBeats});
    fatal_if(writeBurst.expected == 0,
             "VenusL1DmaEngine %s created an empty write burst", name());
    DPRINTF(VenusL1Dma,
            "%s wr_aw dst=%#llx beats=%llu split=%s\n", name(),
            static_cast<unsigned long long>(first->dst),
            static_cast<unsigned long long>(writeBurst.expected),
            writeBurst.expected < maxBurstBeats ? "4k-or-terminal" : "limit");
}

void
VenusL1DmaEngine::issueReadBeat()
{
    if (!readBurst.active || readPort.hasBlockedPacket() ||
        readBurst.issued == readBurst.beats.size()) {
        return;
    }

    Beat *beat = readBurst.beats[readBurst.issued];
    PacketPtr pkt = makePacket(*beat, Channel::Read);
    markAccepted(*beat, Channel::Read);
    ++readBurst.issued;
    DPRINTF(VenusL1Dma,
            "%s read beat bus_src=%#llx bus_dst=%#llx bus_bytes=%llu "
            "logical_src=%#llx logical_dst=%#llx logical_bytes=%llu\n",
            name(), static_cast<unsigned long long>(beat->src),
            static_cast<unsigned long long>(beat->dst),
            static_cast<unsigned long long>(beat->bytes),
            static_cast<unsigned long long>(beat->src + beat->sourceOffset),
            static_cast<unsigned long long>(beat->dst + beat->destinationOffset),
            static_cast<unsigned long long>(beat->logicalBytes));
    if (!readPort.send(pkt)) {
        --readBurst.issued;
        undoAccepted(*beat, Channel::Read);
    }
}

void
VenusL1DmaEngine::issueWriteBeat()
{
    if (!writeBurst.active || writePipe == nullptr ||
        writePort.hasBlockedPacket() ||
        writeBurst.issued == writeBurst.expected) {
        return;
    }

    Beat *beat = writePipe;
    PacketPtr pkt = makePacket(*beat, Channel::Write);
    markAccepted(*beat, Channel::Write);
    ++writeBurst.issued;
    writeBurst.beats.push_back(beat);
    const bool isLast = writeBurst.issued == writeBurst.expected;
    writeBurst.wlastIssued = isLast;
    writePipe = nullptr;
    DPRINTF(VenusL1Dma,
            "%s write beat bus_src=%#llx bus_dst=%#llx bus_bytes=%llu "
            "logical_src=%#llx logical_dst=%#llx logical_bytes=%llu "
            "wstrb=%#018llx\n",
            name(), static_cast<unsigned long long>(beat->src),
            static_cast<unsigned long long>(beat->dst),
            static_cast<unsigned long long>(beat->bytes),
            static_cast<unsigned long long>(beat->src + beat->sourceOffset),
            static_cast<unsigned long long>(beat->dst + beat->destinationOffset),
            static_cast<unsigned long long>(beat->logicalBytes),
            static_cast<unsigned long long>(writeStrobe(beat->writeByteEnable)));
    if (!writePort.send(pkt)) {
        writePipe = beat;
        writeBurst.wlastIssued = false;
        writeBurst.beats.pop_back();
        --writeBurst.issued;
        undoAccepted(*beat, Channel::Write);
    }
}

void
VenusL1DmaEngine::advanceResponsePipeline()
{
    while (!responsePipeline.empty() &&
           responsePipeline.front().visibleAt <= curTick()) {
        const PendingResponse response = responsePipeline.front();
        responsePipeline.pop_front();
        Beat *beat = response.beat;
        panic_if(beat == nullptr,
                 "VenusL1DmaEngine %s has an empty delayed response", name());

        if (response.channel == Channel::Read) {
            panic_if(!beat->readInFlight || readOutstanding == 0,
                     "VenusL1DmaEngine %s read response pipeline accounting "
                     "mismatch", name());
            std::copy_n(beat->data.data() + beat->sourceOffset,
                        beat->logicalBytes,
                        completedData.data() + beat->dataOffset);
            std::copy_n(beat->data.data() + beat->sourceOffset,
                        beat->logicalBytes,
                        beat->writeData.data() + beat->destinationOffset);
            beat->readInFlight = false;
            beat->readResponseArrived = true;
            --readOutstanding;
            DPRINTF(VenusL1Dma,
                    "%s r_visible src=%#llx\n", name(),
                    static_cast<unsigned long long>(beat->src));
            continue;
        }

        panic_if(!beat->writeInFlight || writeOutstanding == 0,
                 "VenusL1DmaEngine %s write response pipeline accounting "
                 "mismatch", name());
        beat->writeInFlight = false;
        beat->writeResponseArrived = true;
        --writeOutstanding;
        panic_if(!writeBurst.active ||
                     writeBurst.responses >= writeBurst.expected,
                 "VenusL1DmaEngine %s write response outside active burst",
                 name());
        ++writeBurst.responses;
        if (writeBurst.wlastIssued &&
            writeBurst.responses == writeBurst.expected) {
            writeBurst.bReady = true;
        }
        DPRINTF(VenusL1Dma,
                "%s b_visible dst=%#llx\n", name(),
                static_cast<unsigned long long>(beat->dst));
    }
}

bool
VenusL1DmaEngine::moveReadResponseToFifo()
{
    if (!readBurst.active ||
        readBurst.handshaken == readBurst.beats.size()) {
        return false;
    }

    Beat *beat = readBurst.beats[readBurst.handshaken];
    if (!beat->readResponseArrived || fifoSram.size() == fifoEntries)
        return false;

    fifoSram.push_back(beat);
    beat->fifoResident = true;
    ++readBurst.handshaken;
    DPRINTF(VenusL1Dma,
            "%s r_hs src=%#llx fifo=%llu/%u\n", name(),
            static_cast<unsigned long long>(beat->src),
            static_cast<unsigned long long>(fifoSram.size()),
            fifoEntries);

    if (readBurst.handshaken == readBurst.beats.size()) {
        DPRINTF(VenusL1Dma, "%s rlast beats=%llu\n", name(),
                static_cast<unsigned long long>(readBurst.beats.size()));
        readBurst = {};
        return true;
    }
    return false;
}

void
VenusL1DmaEngine::prefetchWritePipe()
{
    if (writePipe == nullptr && !fifoSram.empty()) {
        writePipe = fifoSram.front();
        fifoSram.pop_front();
        writePipe->fifoResident = false;
        DPRINTF(VenusL1Dma,
                "%s fifo_prefetch dst=%#llx fifo=%llu/%u\n", name(),
                static_cast<unsigned long long>(writePipe->dst),
                static_cast<unsigned long long>(fifoSram.size()),
                fifoEntries);
    }
}

void
VenusL1DmaEngine::completeWriteBurst()
{
    panic_if(!writeBurst.active || !writeBurst.bReady ||
                 writeBurst.responses != writeBurst.expected,
             "VenusL1DmaEngine %s completed write burst before B", name());
    DPRINTF(VenusL1Dma, "%s b_hs beats=%llu\n", name(),
            static_cast<unsigned long long>(writeBurst.expected));
    for (Beat *beat : writeBurst.beats) {
        panic_if(!beat->writeResponseArrived,
                 "VenusL1DmaEngine %s B before a write response", name());
        removeBeat(beat);
    }
    writeBurst = {};
}

void
VenusL1DmaEngine::stepTransfer()
{
    if (!busy || !admitted || completing)
        return;

    /* dma_fsm captures dma_go on admission, then each streamer registers the
     * descriptor before it can form dma_req_ff/AR.  Keep this as a distinct
     * DMA-clock phase: it is observable for a one-burst transfer, while an
     * immediate post-RLAST request below cancels it for split bursts. */
    if (transferLength != 0 && !streamerDescriptorLoaded) {
        streamerDescriptorLoaded = true;
        scheduleStep();
        return;
    }

    // Make only those R/B responses visible that have crossed the two
    // registered DMA/AXI boundaries.  This must precede the stream/FIFO
    // state updates for this edge, just as dma_axi_if feeds its registered
    // handshake state into the streamers.
    advanceResponsePipeline();

    const bool consumedB = writeBurst.active && writeBurst.bReady;
    if (consumedB)
        completeWriteBurst();

    /* The FIFO output is registered.  Pull the prior-cycle entry before
     * accepting a newly returned R beat, so a new R cannot become W data in
     * the same DMA edge. */
    prefetchWritePipe();
    moveReadResponseToFifo();

    bool startedReadBurst = false;
    if (!readBurst.active) {
        startReadBurst();
        startedReadBurst = readBurst.active;
    }

    bool startedWriteBurst = false;
    if (!consumedB && !writeBurst.active) {
        startWriteBurst();
        startedWriteBurst = writeBurst.active;
    }

    /* AR/AW and their first data beat are distinct registered phases. */
    if (!startedReadBurst)
        issueReadBeat();
    if (!consumedB && !startedWriteBurst)
        issueWriteBeat();

    maybeScheduleCompletion();
    scheduleStep();
}

void
VenusL1DmaEngine::retry(Channel channel)
{
    DmaRequestPort &port = channel == Channel::Read ? readPort : writePort;
    panic_if(!port.hasBlockedPacket(),
             "VenusL1DmaEngine %s got retry without a blocked %s packet", name(),
             channel == Channel::Read ? "read" : "write");

    auto *state = dynamic_cast<BeatSenderState *>(port.blocked()->senderState);
    fatal_if(state == nullptr || state->channel != channel,
             "VenusL1DmaEngine %s has an invalid blocked %s request", name(),
             channel == Channel::Read ? "read" : "write");

    Beat *beat = state->beat;
    if (channel == Channel::Read) {
        panic_if(!readBurst.active ||
                     readBurst.issued >= readBurst.beats.size() ||
                     readBurst.beats[readBurst.issued] != beat,
                 "VenusL1DmaEngine %s retried an out-of-order read beat",
                 name());
        markAccepted(*beat, channel);
        ++readBurst.issued;
        if (!port.retryBlockedPacket()) {
            --readBurst.issued;
            undoAccepted(*beat, channel);
        }
    } else {
        panic_if(!writeBurst.active || writePipe != beat ||
                     writeBurst.issued >= writeBurst.expected,
                 "VenusL1DmaEngine %s retried an invalid write beat", name());
        markAccepted(*beat, channel);
        ++writeBurst.issued;
        writeBurst.beats.push_back(beat);
        const bool isLast = writeBurst.issued == writeBurst.expected;
        writeBurst.wlastIssued = isLast;
        writePipe = nullptr;
        if (!port.retryBlockedPacket()) {
            writePipe = beat;
            writeBurst.wlastIssued = false;
            writeBurst.beats.pop_back();
            --writeBurst.issued;
            undoAccepted(*beat, channel);
        }
    }

    // A legal retry can synchronously return a response.  State advancement
    // remains edge-sampled through stepTransfer().
    scheduleStep();
}

void
VenusL1DmaEngine::handleResponse(Channel channel, PacketPtr pkt)
{
    auto *state = dynamic_cast<BeatSenderState *>(pkt->senderState);
    fatal_if(state == nullptr || state->channel != channel,
             "VenusL1DmaEngine %s received an unexpected %s response", name(),
             channel == Channel::Read ? "read" : "write");
    Beat *beat = state->beat;
    pkt->senderState = state->predecessor;
    delete state;

    fatal_if(pkt->isError(), "VenusL1DmaEngine %s got an error response", name());
    if (channel == Channel::Read) {
        panic_if(!beat->readInFlight || readOutstanding == 0,
                 "VenusL1DmaEngine %s read response accounting mismatch", name());
        const uint8_t *data = pkt->getConstPtr<uint8_t>();
        std::copy_n(data, beat->bytes, beat->data.data());
    } else {
        panic_if(!beat->writeInFlight || writeOutstanding == 0,
                 "VenusL1DmaEngine %s write response accounting mismatch", name());
    }

    delete pkt;
    responsePipeline.push_back({
        channel, beat,
        clockEdge(Cycles(RtlAxiResponseVisibilityCycles))
    });
    scheduleStep();
}

void
VenusL1DmaEngine::removeBeat(Beat *beat)
{
    auto it = std::find_if(activeBeats.begin(), activeBeats.end(),
                           [beat](const std::unique_ptr<Beat> &candidate) {
                               return candidate.get() == beat;
                           });
    panic_if(it == activeBeats.end(),
             "VenusL1DmaEngine %s lost an active beat", name());
    activeBeats.erase(it);
}

bool
VenusL1DmaEngine::transferDrained() const
{
    return nextOffset == transferLength && activeBeats.empty() &&
           !readBurst.active && !writeBurst.active && fifoSram.empty() &&
           writePipe == nullptr && responsePipeline.empty() &&
           readOutstanding == 0 && writeOutstanding == 0 &&
           !readPort.hasBlockedPacket() && !writePort.hasBlockedPacket();
}

void
VenusL1DmaEngine::maybeScheduleCompletion()
{
    if (!completing && transferDrained()) {
        completing = true;
        // B and the externally visible done/callback are distinct DMA-clock
        // phases.  Do not complete from a port response call stack.
        schedule(completeEvent, nextClockBoundary());
    }
}

void
VenusL1DmaEngine::completeTransfer()
{
    panic_if(!busy || !completing || !transferDrained(),
             "VenusL1DmaEngine %s completed before DMA drained", name());

    DPRINTF(VenusL1Dma,
            "%s complete src=%#llx dst=%#llx bytes=%llu\n", name(),
            static_cast<unsigned long long>(sourceBase),
            static_cast<unsigned long long>(destinationBase),
            static_cast<unsigned long long>(transferLength));

    CompleteCallback callback = std::move(completeCallback);
    std::vector<uint8_t> data = std::move(completedData);
    admitCallback = {};
    beatLimitCallback = {};
    busy = false;
    admitted = false;
    completing = false;
    streamerDescriptorLoaded = false;
    sourceBase = 0;
    destinationBase = 0;
    transferLength = 0;
    nextOffset = 0;
    activeBeats.clear();
    fifoSram.clear();
    writePipe = nullptr;
    readBurst = {};
    writeBurst = {};
    responsePipeline.clear();

    if (callback)
        callback(data);
}

} // namespace gem5
