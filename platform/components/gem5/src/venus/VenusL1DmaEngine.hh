#ifndef __VENUS_L1_DMA_ENGINE_HH__
#define __VENUS_L1_DMA_ENGINE_HH__

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/types.hh"
#include "mem/packet.hh"
#include "mem/port.hh"
#include "params/VenusL1DmaEngine.hh"
#include "sim/clocked_object.hh"
#include "sim/eventq.hh"

namespace gem5
{

class System;

/**
 * A small, timing-port based copy engine for scheduler L1 DMA transfers.
 *
 * This object is intentionally independent of VenusDagScheduler and
 * VenusSharedL2.  Integrators connect read_port and write_port to the real
 * shared-L2/tile-memory timing paths, then call startTransfer() when the RTL
 * fire/return responder admits a DMA request.  Request/response timing and
 * backpressure come from those ports; this class contains no task-specific
 * delay constants or captured RTL payloads.
 */
class VenusL1DmaEngine : public ClockedObject
{
  public:
    using AdmitCallback = std::function<void()>;
    using CompleteCallback =
        std::function<void(const std::vector<uint8_t> &data)>;
    // A caller may cap one physical request at an address-decode boundary.
    // The callback receives raw source/destination addresses and the
    // engine's normal beat limit; it must return a value in [1, normal].
    // This preserves the logical transfer and raw bus addresses while
    // preventing one Packet from straddling a non-injective mapping.
    using BeatLimitCallback = std::function<size_t(Addr source,
                                                    Addr destination,
                                                    size_t normal)>;
    // Translate the descriptor-visible address of one physical AXI beat to
    // the gem5 system address carrying the corresponding storage/device.
    // Scheduler firmware uses SE virtual host pointers and the SoC's
    // 0x2000_0000 cluster aperture, while the timing fabric uses the SE page
    // table and the shared-L2 responder at 0x8000_0000.  Keeping translation
    // here preserves the real descriptor and beat boundaries without
    // teaching the engine about any DAG or firmware symbol.
    using AddressTranslateCallback =
        std::function<Addr(Addr address, bool source)>;

    VenusL1DmaEngine(const VenusL1DmaEngineParams &p);
    ~VenusL1DmaEngine() override;

    Port &getPort(const std::string &if_name,
                  PortID idx = InvalidPortID) override;
    void init() override;

    /**
     * Start one source-to-destination copy.  Only one transfer may be active.
     * onAdmit and onComplete are both delivered on this object's clock edge;
     * onComplete receives the bytes read from source after every destination
     * write response has arrived.
     */
    void startTransfer(Addr src, Addr dst, size_t length,
                       AdmitCallback onAdmit,
                       CompleteCallback onComplete,
                       BeatLimitCallback beatLimit = {},
                       AddressTranslateCallback addressTranslate = {});

    bool isBusy() const { return busy; }

  private:
    enum class Channel { Read, Write };

    struct Beat
    {
        // src/dst and bytes describe the physical AXI-sized packet.  A
        // narrow descriptor still issues one 64-byte aligned bus beat; the
        // logical payload is selected with source/destination offsets and
        // the write byte-enable mask below.
        Addr src = 0;
        Addr dst = 0;
        size_t bytes = 0;
        size_t logicalBytes = 0;
        size_t sourceOffset = 0;
        size_t destinationOffset = 0;
        size_t dataOffset = 0;
        std::vector<uint8_t> data;
        std::vector<uint8_t> writeData;
        std::vector<bool> writeByteEnable;
        bool readInFlight = false;
        bool readResponseArrived = false;
        bool fifoResident = false;
        bool writeInFlight = false;
        bool writeResponseArrived = false;
    };

    /*
     * These are logical AXI bursts.  The gem5 ports below still carry one
     * 64-byte Packet per beat, but the streamer is deliberately limited to
     * one AR/R stream and one AW/W/B stream with RTL-sized burst boundaries.
     */
    struct ReadBurst
    {
        bool active = false;
        std::vector<Beat *> beats;
        size_t issued = 0;
        size_t handshaken = 0;
    };

    struct WriteBurst
    {
        bool active = false;
        size_t expected = 0;
        size_t issued = 0;
        size_t responses = 0;
        bool wlastIssued = false;
        bool bReady = false;
        std::vector<Beat *> beats;
    };

    /*
     * A timing-port response is not immediately an RTL DMA-channel
     * handshake.  dma_axi_if observes R/B through registered AXI paths;
     * retain the returned beat until that interface-visible edge.
     */
    struct PendingResponse
    {
        Channel channel;
        Beat *beat = nullptr;
        Tick visibleAt = 0;
    };

    struct BeatSenderState : public Packet::SenderState
    {
        Beat *beat;
        Channel channel;

        BeatSenderState(Beat *beat_, Channel channel_)
            : beat(beat_), channel(channel_)
        {}
    };

    class DmaRequestPort : public RequestPort
    {
      public:
        DmaRequestPort(const std::string &name, VenusL1DmaEngine &owner,
                       Channel channel);
        ~DmaRequestPort() override;

        bool send(PacketPtr pkt);
        bool hasBlockedPacket() const { return blockedPacket != nullptr; }
        PacketPtr blocked() const { return blockedPacket; }
        bool retryBlockedPacket();

      protected:
        bool recvTimingResp(PacketPtr pkt) override;
        void recvReqRetry() override;

      private:
        VenusL1DmaEngine &engine;
        const Channel channel;
        PacketPtr blockedPacket = nullptr;
    };

    const unsigned beatBytes;
    const unsigned fifoEntries;
    const unsigned maxBurstBeats;
    System *const system;
    const RequestorID requestorId;

    DmaRequestPort readPort;
    DmaRequestPort writePort;

    bool busy = false;
    bool admitted = false;
    bool completing = false;
    // dma_go is first captured by dma_fsm; the streamers load the
    // descriptor on the following scheduler edge before they can form AR.
    bool streamerDescriptorLoaded = false;

    Addr sourceBase = 0;
    Addr destinationBase = 0;
    size_t transferLength = 0;
    size_t nextOffset = 0;
    size_t readOutstanding = 0;
    size_t writeOutstanding = 0;

    AdmitCallback admitCallback;
    CompleteCallback completeCallback;
    BeatLimitCallback beatLimitCallback;
    AddressTranslateCallback addressTranslateCallback;
    std::vector<uint8_t> completedData;
    std::deque<std::unique_ptr<Beat>> activeBeats;
    std::deque<Beat *> fifoSram;
    Beat *writePipe = nullptr;
    ReadBurst readBurst;
    WriteBurst writeBurst;
    std::deque<PendingResponse> responsePipeline;

    EventFunctionWrapper admitEvent;
    EventFunctionWrapper stepEvent;
    EventFunctionWrapper completeEvent;

    void admitTransfer();
    void scheduleStep();
    void stepTransfer();
    void startReadBurst();
    void startWriteBurst();
    void issueReadBeat();
    void issueWriteBeat();
    void advanceResponsePipeline();
    bool moveReadResponseToFifo();
    void prefetchWritePipe();
    void completeWriteBurst();
    void retry(Channel channel);
    void handleResponse(Channel channel, PacketPtr pkt);
    void markAccepted(Beat &beat, Channel channel);
    void undoAccepted(Beat &beat, Channel channel);
    PacketPtr makePacket(Beat &beat, Channel channel);
    std::unique_ptr<Beat> makeNextBeat();
    bool transferDrained() const;
    void maybeScheduleCompletion();
    void completeTransfer();
    void removeBeat(Beat *beat);
    Tick clockBoundary() const;
    Tick nextClockBoundary() const;
};

} // namespace gem5

#endif // __VENUS_L1_DMA_ENGINE_HH__
