#ifndef __VENUS_VRF_LIVE_INTENT_HH__
#define __VENUS_VRF_LIVE_INTENT_HH__

#include "mem/packet.hh"

namespace gem5
{

/*
 * The RTL Shuffle lane arbiter is combinational and has LockIn=0.  Its
 * selected payload may therefore change while the downstream per-bank
 * arbiter keeps gnt low.  gem5's ordinary timing-port retry contract locks a
 * packet after rejection, so this small sideband protocol represents the RTL
 * intent/grant boundary explicitly.  It is used only by the opt-in Shuffle
 * path; ordinary timing requesters keep the standard protocol.
 */
class VenusVrfLiveIntentSink
{
  public:
    virtual ~VenusVrfLiveIntentSink() = default;
    virtual void publishVenusVrfLiveIntent(PacketPtr pkt) = 0;
    virtual void withdrawVenusVrfLiveIntent(PacketPtr pkt) = 0;
};

class VenusVrfLiveIntentSource
{
  public:
    virtual ~VenusVrfLiveIntentSource() = default;
    virtual void grantVenusVrfLiveIntent(PacketPtr pkt) = 0;
};

} // namespace gem5

#endif // __VENUS_VRF_LIVE_INTENT_HH__
