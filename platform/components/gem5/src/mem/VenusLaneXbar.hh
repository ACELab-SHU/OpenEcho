#ifndef __MEM_VENUSLANEXBAR_HH__
#define __MEM_VENUSLANEXBAR_HH__

#include "mem/xbar.hh"
#include "params/VenusLaneXBar.hh"

namespace gem5
{

    /**
     * A non-coherent crossbar connects a number of non-snooping memory-side ports
     * and cpu_sides, and routes the request and response packets based on
     * the address. The request packets issued by the memory-side port connected to
     * a non-coherent crossbar could still snoop in caches attached to a
     * coherent crossbar, as is the case with the I/O bus and memory bus
     * in most system configurations. No snoops will, however, reach any
     * memory-side port on the non-coherent crossbar itself.
     *
     * The non-coherent crossbar can be used as a template for modelling
     * PCIe, and non-coherent AMBA and OCP buses, and is typically used
     * for the I/O buses.
     */
    class VenusLaneXBar : public BaseXBar
    {

    protected:
        /**
         * Declare the layers of this crossbar, one vector for requests
         * and one for responses.
         */
        std::vector<ReqLayer *> reqLayers;
        std::vector<RespLayer *> respLayers;

        /**
         * Declaration of the non-coherent crossbar CPU-side port type, one
         * will be instantiated for each of the memory-side ports connecting to
         * the crossbar.
         */
        class VenusLaneXBarResponsePort : public QueuedResponsePort
        {
        private:
            /** A reference to the crossbar to which this port belongs. */
            VenusLaneXBar &xbar;

            /** A normal packet queue used to store responses. */
            RespPacketQueue queue;

        public:
            VenusLaneXBarResponsePort(const std::string &_name,
                                      VenusLaneXBar &_xbar, PortID _id)
                : QueuedResponsePort(_name, queue, _id), xbar(_xbar),
                  queue(_xbar, *this)
            {
            }

        protected:
            bool
            recvTimingReq(PacketPtr pkt) override
            {
                return xbar.recvTimingReq(pkt, id);
            }

            Tick
            recvAtomic(PacketPtr pkt) override
            {
                return xbar.recvAtomicBackdoor(pkt, id);
            }

            Tick
            recvAtomicBackdoor(PacketPtr pkt, MemBackdoorPtr &backdoor) override
            {
                return xbar.recvAtomicBackdoor(pkt, id, &backdoor);
            }

            void
            recvFunctional(PacketPtr pkt) override
            {
                xbar.recvFunctional(pkt, id);
            }

            void
            recvMemBackdoorReq(const MemBackdoorReq &req,
                               MemBackdoorPtr &backdoor) override
            {
                xbar.recvMemBackdoorReq(req, backdoor);
            }

            AddrRangeList
            getAddrRanges() const override
            {
                return xbar.getAddrRanges();
            }
        };

        /**
         * Declaration of the crossbar memory-side port type, one will be
         * instantiated for each of the CPU-side ports connecting to the
         * crossbar.
         */
        class VenusLaneXBarRequestPort : public RequestPort
        {
        private:
            /** A reference to the crossbar to which this port belongs. */
            VenusLaneXBar &xbar;

        public:
            VenusLaneXBarRequestPort(const std::string &_name,
                                     VenusLaneXBar &_xbar, PortID _id)
                : RequestPort(_name, _id), xbar(_xbar)
            {
            }

        protected:
            bool
            recvTimingResp(PacketPtr pkt) override
            {
                return xbar.recvTimingResp(pkt, id);
            }

            void
            recvRangeChange() override
            {
                xbar.recvRangeChange(id);
            }

            void
            recvReqRetry() override
            {
                xbar.recvReqRetry(id);
            }
        };

        virtual bool recvTimingReq(PacketPtr pkt, PortID cpu_side_port_id);
        virtual bool recvTimingResp(PacketPtr pkt, PortID mem_side_port_id);
        void recvReqRetry(PortID mem_side_port_id);
        Tick recvAtomicBackdoor(PacketPtr pkt, PortID cpu_side_port_id,
                                MemBackdoorPtr *backdoor = nullptr);
        void recvFunctional(PacketPtr pkt, PortID cpu_side_port_id);
        void recvMemBackdoorReq(const MemBackdoorReq &req,
                                MemBackdoorPtr &backdoor);

    public:
        VenusLaneXBar(const VenusLaneXBarParams &p);

        virtual ~VenusLaneXBar();
    };

} // namespace gem5

#endif //__MEM_NONCOHERENT_XBAR_HH__
