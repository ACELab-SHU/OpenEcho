#ifndef __VENUS_LANE_PRODUCER_COMPLETION_HH__
#define __VENUS_LANE_PRODUCER_COMPLETION_HH__

namespace gem5
{

/*
 * A lane's final destination-bank grant and its later PE response are two
 * distinct RTL boundaries.  Sequencer-local LSU consumers observe the
 * former through global_hazard_table_d, while running-ID recycle observes
 * the latter.  gem5's timing response represents only the latter, so carry
 * the tagged final-grant observation on a sideband instead of retiring the
 * whole instruction early.
 */
class VenusLaneProducerCompletionSink
{
  public:
    virtual ~VenusLaneProducerCompletionSink() = default;
    virtual void noteVenusLaneProducerGrant(
        int laneId, int runningId, int instructionId) = 0;
};

} // namespace gem5

#endif // __VENUS_LANE_PRODUCER_COMPLETION_HH__
