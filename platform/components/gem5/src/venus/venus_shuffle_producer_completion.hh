#ifndef __VENUS_SHUFFLE_PRODUCER_COMPLETION_HH__
#define __VENUS_SHUFFLE_PRODUCER_COMPLETION_HH__

namespace gem5
{

/* Final per-bank shuffle grants precede the PE response/recycle boundary. */
class VenusShuffleProducerCompletionSink
{
  public:
    virtual ~VenusShuffleProducerCompletionSink() = default;
    virtual void noteVenusShuffleProducerGrant(
        int runningId, int instructionId) = 0;
};

} // namespace gem5

#endif // __VENUS_SHUFFLE_PRODUCER_COMPLETION_HH__
