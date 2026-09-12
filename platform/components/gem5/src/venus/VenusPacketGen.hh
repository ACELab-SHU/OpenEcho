#ifndef VENUSPACKET_H
#define VENUSPACKET_H

#include <iostream>
#include <random>
#include <vector>
#include <cassert>

#include "debug/VenusSequencer.hh"
#include "params/VenusPacketGen.hh" // 包含自动生成的Params类

#include "sim/clocked_object.hh"
#include "base/trace.hh"
#include "mem/port.hh"
#include "mem/packet.hh"

#include "venus_instr_pkt.hh"

namespace gem5
{
    // C++实体类声明
    class VenusPacketGen : public ClockedObject
    {

    public:
        class VenusPacketGenSequencerSidePort : public RequestPort
        {
        private:
            VenusPacketGen *owner;
            PacketPtr blockedPacket = nullptr;

        protected:
            bool recvTimingResp(PacketPtr pkt) override { panic("recvTimingResp unimpl."); }
            void recvRangeChange() override { panic("recvRangeChange unimpl."); }

        public:
            bool isBlocked = false;
            void sendPacket(PacketPtr pkt);
            void recvReqRetry() override;
            VenusPacketGenSequencerSidePort(const std::string &name, VenusPacketGen *owner) : RequestPort(name, owner), owner(owner)
            {
            }
        };
        Port &getPort(const std::string &if_name, PortID idx = InvalidPortID) override;

    private:
        // 关键仿真组件
        VenusPacketGenSequencerSidePort port_venuspacketgen_sendto_venussequencer;
        EventFunctionWrapper nextTickEvent;
        Tick afterCycles(Cycles cycles) const
        {
            return curTick() + cyclesToTicks(cycles);
        }
        // 内部私有参数
        unsigned int reschedule_interval = 1;
        unsigned int schedule_interval = 1;
        unsigned int generate_howmany_times = 227;
        // 内部私有状态
        // 内部私有变量
        VenusInstrPkt *venus_instr_pkt;

    public:
        // 构造函数
        void startup() override;
        VenusPacketGen(const VenusPacketGenParams &params) : ClockedObject(params),
                                                             port_venuspacketgen_sendto_venussequencer(params.name + ".port_venuspacketgen_sendto_venussequencer", this),
                                                             nextTickEvent([this]{ sendOneVenusPkt(); }, name()),
                                                             generate_howmany_times(params.generate_howmany_times)
        {
        }

        // 发送函数
        void sendOneVenusPkt();
    };
}
#endif // VENUS_PACKET_H
