#include "mem/VenusLaneXbar.hh"

#include "base/logging.hh"
#include "base/trace.hh"

namespace gem5
{

    VenusLaneXbar::VenusLaneXbar(const VenusLaneXbarParams &p)
        : BaseXBar(p)
    {
        // create the ports based on the size of the memory-side port and
        // CPU-side port vector ports, and the presence of the default port,
        // the ports are enumerated starting from zero
        for (int i = 0; i < p.port_mem_side_ports_connection_count; ++i)
        {
            std::string portName = csprintf("%s.mem_side_port[%d]", name(), i);
            RequestPort *bp = new VenusLaneXbarRequestPort(portName, *this, i);
            memSidePorts.push_back(bp);
            reqLayers.push_back(new ReqLayer(*bp, *this,
                                             csprintf("reqLayer%d", i)));
        }

        // see if we have a default CPU-side-port device connected and if so add
        // our corresponding memory-side port
        if (p.port_default_connection_count)
        {
            defaultPortID = memSidePorts.size();
            std::string portName = name() + ".default";
            RequestPort *bp = new VenusLaneXbarRequestPort(portName, *this,
                                                           defaultPortID);
            memSidePorts.push_back(bp);
            reqLayers.push_back(new ReqLayer(*bp, *this, csprintf("reqLayer%d", defaultPortID)));
        }

        // create the CPU-side ports, once again starting at zero
        for (int i = 0; i < p.port_cpu_side_ports_connection_count; ++i)
        {
            std::string portName = csprintf("%s.cpu_side_ports[%d]", name(), i);
            QueuedResponsePort *bp = new VenusLaneXbarResponsePort(portName,
                                                                   *this, i);
            cpuSidePorts.push_back(bp);
            respLayers.push_back(new RespLayer(*bp, *this,
                                               csprintf("respLayer%d", i)));
        }
    }

    VenusLaneXbar::~VenusLaneXbar()
    {
        for (auto l : reqLayers)
            delete l;
        for (auto l : respLayers)
            delete l;
    }

    bool
    VenusLaneXbar::recvTimingReq(PacketPtr pkt, PortID cpu_side_port_id)
    {
        // determine the source port based on the id
        ResponsePort *src_port = cpuSidePorts[cpu_side_port_id];

        // we should never see express snoops on a non-coherent crossbar
        assert(!pkt->isExpressSnoop());

        // determine the destination port
        PortID mem_side_port_id = findPort(pkt);

        // 向目标端口代理发送请求，如果有业务，那么就给发送端port返回失败
        if (!reqLayers[mem_side_port_id]->tryTiming(src_port))
        {
            DPRINTF(VenusLaneXbar, "recvTimingReq: src %s %s 0x%x BUSY\n",
                    src_port->name(), pkt->cmdString(), pkt->getAddr());
            return false;
        }

        DPRINTF(VenusLaneXbar, "recvTimingReq: src %s %s 0x%x\n",
                src_port->name(), pkt->cmdString(), pkt->getAddr());

        // before forwarding the packet (and possibly altering it),
        // remember if we are expecting a response
        const bool expect_response = pkt->needsResponse() &&
                                     !pkt->cacheResponding();

        // 如果成功就发送访存bank请求，这里应该一定是成功的
        bool success = memSidePorts[mem_side_port_id]->sendTimingReq(pkt);
        assert(success);

        // remember where to route the response to
        if (expect_response)
        {
            assert(routeTo.find(pkt->req) == routeTo.end());
            routeTo[pkt->req] = cpu_side_port_id;
        }

        reqLayers[mem_side_port_id]->succeededTiming(1000); // 这里应该对齐bank的具体访存时延

        // stats updates
        pktCount[cpu_side_port_id][mem_side_port_id]++;
        pktSize[cpu_side_port_id][mem_side_port_id] += pkt_size;
        transDist[pkt_cmd]++;

        return true;
    }

    bool
    VenusLaneXbar::recvTimingResp(PacketPtr pkt, PortID mem_side_port_id)
    {
        // determine the source port based on the id
        RequestPort *src_port = memSidePorts[mem_side_port_id];

        // determine the destination
        const auto route_lookup = routeTo.find(pkt->req);
        assert(route_lookup != routeTo.end());
        const PortID cpu_side_port_id = route_lookup->second;
        assert(cpu_side_port_id != InvalidPortID);
        assert(cpu_side_port_id < respLayers.size());

        // test if the layer should be considered occupied for the current
        // port
        if (!respLayers[cpu_side_port_id]->tryTiming(src_port))
        {
            DPRINTF(VenusLaneXbar, "recvTimingResp: src %s %s 0x%x BUSY\n",
                    src_port->name(), pkt->cmdString(), pkt->getAddr());
            return false;
        }

        DPRINTF(VenusLaneXbar, "recvTimingResp: src %s %s 0x%x\n",
                src_port->name(), pkt->cmdString(), pkt->getAddr());

        cpuSidePorts[cpu_side_port_id]->schedTimingResp(pkt,
                                                        curTick() + latency);

        // remove the request from the routing table
        routeTo.erase(route_lookup);

        respLayers[cpu_side_port_id]->succeededTiming(packetFinishTime);

        // stats updates
        pktCount[cpu_side_port_id][mem_side_port_id]++;
        pktSize[cpu_side_port_id][mem_side_port_id] += pkt_size;
        transDist[pkt_cmd]++;

        return true;
    }

    void
    VenusLaneXbar::recvReqRetry(PortID mem_side_port_id)
    {
        // responses never block on forwarding them, so the retry will
        // always be coming from a port to which we tried to forward a
        // request
        reqLayers[mem_side_port_id]->recvRetry();
    }

    Tick
    VenusLaneXbar::recvAtomicBackdoor(PacketPtr pkt, PortID cpu_side_port_id,
                                      MemBackdoorPtr *backdoor)
    {
        DPRINTF(NoncoherentXBar, "recvAtomic: packet src %s addr 0x%x cmd %s\n",
                cpuSidePorts[cpu_side_port_id]->name(), pkt->getAddr(),
                pkt->cmdString());

        unsigned int pkt_size = pkt->hasData() ? pkt->getSize() : 0;
        unsigned int pkt_cmd = pkt->cmdToIndex();

        // determine the destination port
        PortID mem_side_port_id = findPort(pkt);

        // stats updates for the request
        pktCount[cpu_side_port_id][mem_side_port_id]++;
        pktSize[cpu_side_port_id][mem_side_port_id] += pkt_size;
        transDist[pkt_cmd]++;

        // forward the request to the appropriate destination
        auto mem_side_port = memSidePorts[mem_side_port_id];
        Tick response_latency = backdoor ? mem_side_port->sendAtomicBackdoor(pkt, *backdoor) : mem_side_port->sendAtomic(pkt);

        // add the response data
        if (pkt->isResponse())
        {
            pkt_size = pkt->hasData() ? pkt->getSize() : 0;
            pkt_cmd = pkt->cmdToIndex();

            // stats updates
            pktCount[cpu_side_port_id][mem_side_port_id]++;
            pktSize[cpu_side_port_id][mem_side_port_id] += pkt_size;
            transDist[pkt_cmd]++;
        }

        // @todo: Not setting first-word time
        pkt->payloadDelay = response_latency;
        return response_latency;
    }

    void
    VenusLaneXbar::recvMemBackdoorReq(const MemBackdoorReq &req,
                                      MemBackdoorPtr &backdoor)
    {
        PortID dest_id = findPort(req.range());
        memSidePorts[dest_id]->sendMemBackdoorReq(req, backdoor);
    }

    void
    VenusLaneXbar::recvFunctional(PacketPtr pkt, PortID cpu_side_port_id)
    {
        if (!pkt->isPrint())
        {
            // don't do DPRINTFs on PrintReq as it clutters up the output
            DPRINTF(NoncoherentXBar,
                    "recvFunctional: packet src %s addr 0x%x cmd %s\n",
                    cpuSidePorts[cpu_side_port_id]->name(), pkt->getAddr(),
                    pkt->cmdString());
        }

        // since our CPU-side ports are queued ports we need to check them as well
        for (const auto &p : cpuSidePorts)
        {
            // if we find a response that has the data, then the
            // downstream caches/memories may be out of date, so simply stop
            // here
            if (p->trySatisfyFunctional(pkt))
            {
                if (pkt->needsResponse())
                    pkt->makeResponse();
                return;
            }
        }

        // determine the destination port
        PortID dest_id = findPort(pkt);

        // forward the request to the appropriate destination
        memSidePorts[dest_id]->sendFunctional(pkt);
    }

} // namespace gem5
