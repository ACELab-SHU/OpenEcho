#ifndef __SIM_VENUS_LANE_HH__
#define __SIM_VENUS_LANE_HH__

#include "debug/Lane.hh"
#include "debug/LaneSequencer.hh"
#include "debug/LaneOperandRequester.hh"
#include "debug/LaneVFU.hh"
#include "debug/LaneVSPM.hh"
#include "debug/ShuffleEngine.hh"

#include "venus_extension_pkg.hh"
#include "VenusShufflePipline.hh"
#include "params/VenusLane.hh" // 包含自动生成的Params类
#include "sim/clocked_object.hh"
#include "base/trace.hh"
#include "mem/port.hh"

#include "venus_instr_pkt.hh"
#include "venus_vfu.hh"
#include "venus/venus_lane_producer_completion.hh"
#include "venus/venus_vrf_live_intent.hh"
#include <array>
#include <cmath>
#include <deque>
#include <memory>
#include <vector>

namespace gem5
{
    class VenusLane : public ClockedObject
    {
        class VenusLaneSequencerSidePort : public ResponsePort
        {
            private:
                VenusLane *owner;

            protected:
                Tick recvAtomic(PacketPtr pkt) override { panic("recvAtomic unimpl."); }
                void recvFunctional(PacketPtr pkt) override { panic("recvFunctional unimpl."); }
                bool recvTimingReq(PacketPtr pkt) override;
                // bool sendTimingResp(PacketPtr pkt) override;
                void recvRespRetry() override { panic("recvRespRetry unimpl."); }

            public:
                AddrRangeList getAddrRanges() const override { panic("getAddrRanges unimpl."); }
                void reportProducerGrantCompletion(const VenusInstrPkt *pkt);

                VenusLaneSequencerSidePort(const std::string &name, VenusLane *owner) : ResponsePort(name, owner), owner(owner)
                {
                }
        };
        class VenusLaneShuffleSidePort : public ResponsePort,
                                         public VenusVrfLiveIntentSink
        {
            private:
                VenusLane *owner;

            protected:
                Tick recvAtomic(PacketPtr pkt) override { panic("recvAtomic unimpl."); }
                void recvFunctional(PacketPtr pkt) override { panic("recvFunctional unimpl."); }
                bool recvTimingReq(PacketPtr pkt) override;
                // bool sendTimingResp(PacketPtr pkt) override;
                void recvRespRetry() override { panic("recvRespRetry unimpl."); }
                void publishVenusVrfLiveIntent(PacketPtr pkt) override;
                void withdrawVenusVrfLiveIntent(PacketPtr pkt) override;

            public:
                AddrRangeList getAddrRanges() const override { panic("getAddrRanges unimpl."); }
                void grantLiveIntent(PacketPtr pkt);

                VenusLaneShuffleSidePort(const std::string &name, VenusLane *owner) : ResponsePort(name, owner), owner(owner)
                {
                }
        };
        class VenusLaneHazardTableListenPort : public ResponsePort
        {
            private:
                VenusLane *owner;

            protected:
                Tick recvAtomic(PacketPtr pkt) override { panic("recvAtomic unimpl."); }
                void recvFunctional(PacketPtr pkt) override { panic("recvFunctional unimpl."); }
                bool recvTimingReq(PacketPtr pkt) override;
                void recvRespRetry() override { panic("recvRespRetry unimpl."); }

            public:
                AddrRangeList getAddrRanges() const override { panic("getAddrRanges unimpl."); }

                VenusLaneHazardTableListenPort(const std::string &name, VenusLane *owner) : ResponsePort(name, owner), owner(owner)
                {
                }
        };

        class VenusLaneToVrfRequestPort : public RequestPort,
                                          public VenusVrfLiveIntentSource
        {
            private:
                VenusLane *owner;
                int portid;
                PacketPtr blockedPacket = nullptr;
                PacketPtr deferredWriteGrant = nullptr;
                PacketPtr liveIntentPacket = nullptr;

            protected:
                bool recvTimingResp(PacketPtr pkt) override;
                void recvRangeChange() override {}

            public:
                bool sendPacket(PacketPtr pkt);
                bool ownsBlockedPacket(PacketPtr pkt) const
                {
                    return blockedPacket == pkt;
                }
                bool consumeDeferredWriteGrant(
                    Addr addr, unsigned size, int running_id,
                    int vns_instr_id);
                bool hasDeferredWriteGrant() const
                {
                    return deferredWriteGrant != nullptr;
                }
                void recvReqRetry() override;
                void publishLiveIntent(PacketPtr pkt);
                void withdrawLiveIntent();
                void grantVenusVrfLiveIntent(PacketPtr pkt) override;

                VenusLaneToVrfRequestPort(const std::string &name, int portid, VenusLane *owner)
                    : RequestPort(name, owner), portid(portid), owner(owner) {}
        };

        struct VrfWriteCommitState : public Packet::SenderState
        {
            int running_id;
            int vns_instr_id;

            VrfWriteCommitState(int running_id, int vns_instr_id)
                : running_id(running_id), vns_instr_id(vns_instr_id)
            {}
        };

        struct VrfReadResponseState : public Packet::SenderState
        {
            int running_id;
            int vns_instr_id;
            int operand_offset;
            bool pipelined;

            VrfReadResponseState(
                int running_id, int vns_instr_id, int operand_offset,
                bool pipelined)
                : running_id(running_id), vns_instr_id(vns_instr_id),
                  operand_offset(operand_offset), pipelined(pipelined)
            {}
        };

        Port &getPort(const std::string &if_name, PortID idx = InvalidPortID) override;
        void handleLaneToVrfRetry(int portid);
        void handleLaneToVrfGrant(int portid, PacketPtr pkt);
        void publishShuffleVrfLiveIntent(PacketPtr pkt);
        void withdrawShuffleVrfLiveIntent();
        void handleShuffleVrfLiveGrant(int portid, PacketPtr pkt);
    private:
        // 关键仿真组件
        VenusLaneSequencerSidePort port_venuslane_receivefrom_venussequencer;
        VenusLaneShuffleSidePort port_venuslane_receivefrom_venusshuffle;
        VenusLaneHazardTableListenPort port_venuslane_hazardtable_listen;
        std::vector<VenusLaneToVrfRequestPort> VenusLaneRequestPorts; // Container to hold multiple request ports
        std::map<unsigned int, std::string> portMapping;              // Maps index to a logical name/meaning
        int lane_num;
        int bank_num;
        int line_num;
        Addr vrf_base_addr;
        int lane_id;
        bool experimentalVrfRr;
        bool experimentalRequesterQVisibility;
        bool experimentalOneEntryOperandCommands;
        unsigned rtlPeCommandVisibilityCycles;
        bool rtlChainingEnabled;
        void noteVectorWriterGrant(int running_id, VFU writer_vfu,
                                   Addr writer_vaddr);
        void updateVectorWriterBoundary(int running_id);
        Addr shuffleWriterVaddr(PacketPtr pkt) const;
        Addr calculatePhysicalAddress(int headline, int offset);
        Addr calculateMaskPhysicalAddress(int offset);
        unsigned int getLocalLaneCalcLen(int vl, VEW vew, VenusOp op, bool is_write_back);
        bool sendVFUReadRequest(int headline, int offset,
                                const std::string& vfuName, int _responseid,
                                int requestport, int running_id,
                                int vns_instr_id, bool pipelined);
        void commitOperandReadGrant(
            VenusInstrPkt *pkt, OPERANDTYPE operand,
            bool &dataToRead, bool &dataArrived);
        unsigned operandQueueDataDepth(OPERANDTYPE operand) const;
        void updateOperandQueueUsageBoundary(OPERANDTYPE operand);
        bool operandQueueReadyForIssue(OPERANDTYPE operand);
        void noteOperandQueueIssue(OPERANDTYPE operand);
        void noteOperandQueuePop(OPERANDTYPE operand);
        void noteOperandQueueUnpop(OPERANDTYPE operand);
        void scheduleBitAluOperandPopBoundary(const VenusInstrPkt *instr,
                                              bool maskRowFetched);
        bool prepareMaskOperand(unsigned int &slice,
                                unsigned int &rowBits, bool &rowValid,
                                bool &rowFetched,
                                const VenusInstrPkt *instr,
                                int calcCount);
        void updateBitAluMaskLatchBoundary();
        bool prepareBitAluMaskOperand(unsigned int &slice,
                                      bool &rowHandshake,
                                      const VenusInstrPkt *instr,
                                      int calcCount);
        void commitBitAluMaskOperand(const VenusInstrPkt *instr,
                                     int calcCount);
        void updateCauMaskLatchBoundary();
        bool prepareCauMaskOperand(unsigned int &slice,
                                   bool &rowHandshake,
                                   const VenusInstrPkt *instr,
                                   int calcCount);
        void commitCauMaskOperand(const VenusInstrPkt *instr,
                                  int calcCount);
        void rollbackMaskOperand(unsigned int rowBits, bool &rowValid,
                                 bool rowFetched,
                                 const VenusInstrPkt *instr);
        void commitMaskOperand(bool &rowValid,
                               const VenusInstrPkt *instr,
                               int calcCount);
        void serviceBitAluOperandPopBoundary();
        void updateBitAluResultCountBoundary();
        int bitAluEffectiveResultCount() const;
        void noteBitAluResultEnqueue();
        void noteBitAluResultGrant();
          bool bitAluOperandAdmissionReady(const VenusInstrPkt *instr);
        bool cauOperandAdmissionReady(VenusInstrPkt *instr);
        bool deferOperandQueuePopToBitAluAdmission(
            OPERANDTYPE operand, const VenusInstrPkt *instr) const;
        bool producerCompletionPending(int runningId, int instrId) const;
        bool sendVFUWriteRequest(int headline, int offset, const std::string& vfuName, int _responseid, int requestport, unsigned int writebackdata, unsigned int size, int running_id, int vns_instr_id);
        bool sendMaskReadRequest(int offset, const std::string& vfuName,
                                 int _responseid, int requestport,
                                 int running_id, int vns_instr_id,
                                 bool pipelined);
        bool sendMaskWriteRequest(int offset, const std::string& vfuName,
                                  int _responseid, int requestport,
                                  unsigned int writebackdata);
        bool sendMaskRowWriteRequest(int maskAddr, uint8_t rowBits,
                                     int runningId, int instrId);

        struct PendingVrfReadResponse
        {
            Tick visible_tick;
            int response_id;
            int running_id;
            int vns_instr_id;
            int operand_offset;
            unsigned int data;
        };
        std::list<PendingVrfReadResponse> pendingVrfReadResponses;
        std::list<PendingVrfReadResponse> readyVrfReadResponses[11];
        GenerateVRFDataEventFunctionWrapper nextVrfReadResponseStageEvent;
        EventFunctionWrapper bitAluOperandPopBoundaryEvent;
        std::deque<VenusInstrPkt *> pendingZeroLocalLaneDone;
        EventFunctionWrapper nextZeroLocalLaneDoneEvent;
        bool bitAluOperandPopA = false;
        bool bitAluOperandPopB = false;
        bool bitAluOperandPopMask = false;
        bool bitAluOperandPopReservesResult = false;
        bool bitAluAdmissionBlockedOnResultFull = false;
        unsigned bitAluResultCountQ = 0;
        int bitAluResultCountDelta = 0;
        Tick bitAluResultCountTick = 0;
        void publishVrfReadResponses();
        void reportZeroLocalLaneDone();
        bool vrfReadGenerationMatches(const PendingVrfReadResponse &resp) const;
        bool takeVrfReadResponse(int response_id, int running_id,
                                 int vns_instr_id, int operand_offset,
                                 unsigned int &data);
          void serviceDrainingOperandResponses(OPERANDTYPE operand);

        SimObject *m_venus_lane_simobject_1=nullptr, *m_venus_lane_simobject_2=nullptr, *m_venus_lane_simobject_3=nullptr, *m_venus_lane_simobject_4=nullptr, *m_venus_lane_simobject_5=nullptr, *m_venus_lane_simobject_6=nullptr, *m_venus_lane_simobject_7=nullptr, *m_venus_lane_simobject_8=nullptr, *m_venus_lane_simobject_9=nullptr, *m_venus_lane_simobject_10=nullptr, *m_venus_lane_simobject_11=nullptr, *m_venus_lane_simobject_12=nullptr, *m_venus_lane_simobject_13=nullptr, *m_venus_lane_simobject_14=nullptr, *m_venus_lane_simobject_15=nullptr, *m_venus_lane_simobject_16=nullptr, *m_venus_lane_simobject_17=nullptr, *m_venus_lane_simobject_18=nullptr, *m_venus_lane_simobject_19=nullptr, *m_venus_lane_simobject_20=nullptr, *m_venus_lane_simobject_21=nullptr, *m_venus_lane_simobject_22=nullptr, *m_venus_lane_simobject_23=nullptr, *m_venus_lane_simobject_24=nullptr, *m_venus_lane_simobject_25=nullptr, *m_venus_lane_simobject_26=nullptr, *m_venus_lane_simobject_27=nullptr, *m_venus_lane_simobject_28=nullptr, *m_venus_lane_simobject_29=nullptr, *m_venus_lane_simobject_30=nullptr, *m_venus_lane_simobject_31=nullptr, *m_venus_lane_simobject_32=nullptr, *m_venus_lane_simobject_33=nullptr, *m_venus_lane_simobject_34=nullptr, *m_venus_lane_simobject_35=nullptr, *m_venus_lane_simobject_36=nullptr, *m_venus_lane_simobject_37=nullptr, *m_venus_lane_simobject_38=nullptr, *m_venus_lane_simobject_39=nullptr, *m_venus_lane_simobject_40=nullptr, *m_venus_lane_simobject_41=nullptr, *m_venus_lane_simobject_42=nullptr, *m_venus_lane_simobject_43=nullptr, *m_venus_lane_simobject_44=nullptr, *m_venus_lane_simobject_45=nullptr, *m_venus_lane_simobject_46=nullptr, *m_venus_lane_simobject_47=nullptr, *m_venus_lane_simobject_48=nullptr, *m_venus_lane_simobject_49=nullptr, *m_venus_lane_simobject_50=nullptr, *m_venus_lane_simobject_51=nullptr, *m_venus_lane_simobject_52=nullptr, *m_venus_lane_simobject_53=nullptr, *m_venus_lane_simobject_54=nullptr, *m_venus_lane_simobject_55=nullptr, *m_venus_lane_simobject_56=nullptr, *m_venus_lane_simobject_57=nullptr, *m_venus_lane_simobject_58=nullptr, *m_venus_lane_simobject_59=nullptr, *m_venus_lane_simobject_60=nullptr, *m_venus_lane_simobject_61=nullptr, *m_venus_lane_simobject_62=nullptr, *m_venus_lane_simobject_63=nullptr;
        VenusLane *m_venus_lanes[MaxNrLanes];
        SimObject *m_venus_shuffle_pipeline_simobject=nullptr;
        VenusShufflePipline *m_venus_shuffle_pipeline;
    public:
        // 构造函数，初始化成员变量
        // 构造函数，初始化成员变量
          VenusLane(const VenusLaneParams &params) : ClockedObject(params),
              port_venuslane_receivefrom_venussequencer(params.name + ".port_venuslane_receivefrom_venussequencer", this),
              port_venuslane_receivefrom_venusshuffle(params.name + ".port_venuslane_receivefrom_venusshuffle", this),
              port_venuslane_hazardtable_listen(params.name + ".port_venuslane_hazardtable_listen", this),
              nextVrfReadResponseStageEvent(
                  [this]{publishVrfReadResponses();}, name()),
              bitAluOperandPopBoundaryEvent(
                  [this]{serviceBitAluOperandPopBoundary();},
                  name() + ".bitalu_operand_pop_boundary", false, 1),
              nextZeroLocalLaneDoneEvent(
                  [this]{reportZeroLocalLaneDone();},
                  name() + ".zero_local_lane_done"),
              nextLaneSequencerPushFIFONewInstrEvent([this]{laneSequencerPushFIFONewInstr();},name()),

              nextOperandRequesterGetsBitAlu_A_DataEvent([this]{operandRequesterGetVectorData(BitAlu_A);},name()),
              nextOperandRequesterGetsBitAlu_B_DataEvent([this]{operandRequesterGetVectorData(BitAlu_B);},name()),
              nextOperandRequesterGetsCAU_A_DataEvent([this]{operandRequesterGetVectorData(CAU_A);},name()),
              nextOperandRequesterGetsCAU_B_DataEvent([this]{operandRequesterGetVectorData(CAU_B);},name()),
              nextOperandRequesterGetsCAU_C_DataEvent([this]{operandRequesterGetVectorData(CAU_C);},name()),
              nextOperandRequesterGetsCAU_D_DataEvent([this]{operandRequesterGetVectorData(CAU_D);},name()),
              nextOperandRequesterGetsSerDiv_A_DataEvent([this]{operandRequesterGetVectorData(SerDiv_A);},name()),
              nextOperandRequesterGetsSerDiv_B_DataEvent([this]{operandRequesterGetVectorData(SerDiv_B);},name()),
              nextOperandRequesterGetsMask_DataEvent([this]{operandRequesterGetVectorData(Mask);},name()),
              nextOperandRequesterGetsShuffleUnit_DataEvent([this]{operandRequesterGetVectorData(ShuffleUnit);},name()),

              nextVFUBitAluCalcEvent([this]{VFUCalc(VFU_BitALU);},name()),
              nextVFUCAUCalcEvent([this]{VFUCalc(VFU_CAU);},name()),
              nextVFUSerDivCalcEvent([this]{VFUCalc(VFU_SerDiv);},name()),
              nextVFUShuffleCalcEvent([this]{VFUCalc(VFU_ShuffleUnit);},name()),

              nextVFUBitAluStartCalcEvent([this]{VFUBitAluCalculating();},name()),
              nextVFUCAUStartCalcEvent([this]{VFUCAUCalculating();},name()),
              nextVFUSerdivStartCalcEvent([this]{VFUSerdivCalculating();},name()),
              nextVFUSerdivResultQueueEvent(
                  [this]{serviceSerDivResultQueueEvent();}, name()),
              nextVFUShuffleUnitStartCalcEvent([this]{VFUShuffleUnitCalculating();},name()),

              nextVFUBitAluReportandRecycleInstrEvent([this]{reportAndRecycleDoneInstr(VFU_BitALU);},name()),
              nextVFUCAUReportandRecycleInstrEvent([this]{reportAndRecycleDoneInstr(VFU_CAU);},name()),
              nextVFUSerdivReportandRecycleInstrEvent([this]{reportAndRecycleDoneInstr(VFU_SerDiv);},name()),
              nextVFUShuffleReportandRecycleInstrEvent([this]{reportAndRecycleDoneInstr(VFU_ShuffleUnit);},name()),

              nextTRICKgenerateVRFReadData_BitAlu_A([this]{generateVRFReadData(BitAlu_A   );},name()),
              nextTRICKgenerateVRFReadData_BitAlu_B([this]{generateVRFReadData(BitAlu_B   );},name()),
              nextTRICKgenerateVRFReadData_CAU_A([this]{generateVRFReadData(CAU_A      );},name()),
              nextTRICKgenerateVRFReadData_CAU_B([this]{generateVRFReadData(CAU_B      );},name()),
              nextTRICKgenerateVRFReadData_CAU_C([this]{generateVRFReadData(CAU_C      );},name()),
              nextTRICKgenerateVRFReadData_CAU_D([this]{generateVRFReadData(CAU_D      );},name()),
              nextTRICKgenerateVRFReadData_SerDiv_A([this]{generateVRFReadData(SerDiv_A   );},name()),
              nextTRICKgenerateVRFReadData_SerDiv_B([this]{generateVRFReadData(SerDiv_B   );},name()),
              nextTRICKgenerateVRFReadData_Mask([this]{generateVRFReadData(Mask       );},name()),
              nextTRICKgenerateVRFReadData_ShuffleUnit([this]{generateVRFReadData(ShuffleUnit);},name()),

              m_venus_lane_simobject_1(params.venus_lane_simobject_1), m_venus_lane_simobject_2(params.venus_lane_simobject_2), m_venus_lane_simobject_3(params.venus_lane_simobject_3), m_venus_lane_simobject_4(params.venus_lane_simobject_4), m_venus_lane_simobject_5(params.venus_lane_simobject_5), m_venus_lane_simobject_6(params.venus_lane_simobject_6), m_venus_lane_simobject_7(params.venus_lane_simobject_7), m_venus_lane_simobject_8(params.venus_lane_simobject_8), m_venus_lane_simobject_9(params.venus_lane_simobject_9), m_venus_lane_simobject_10(params.venus_lane_simobject_10), m_venus_lane_simobject_11(params.venus_lane_simobject_11), m_venus_lane_simobject_12(params.venus_lane_simobject_12), m_venus_lane_simobject_13(params.venus_lane_simobject_13), m_venus_lane_simobject_14(params.venus_lane_simobject_14), m_venus_lane_simobject_15(params.venus_lane_simobject_15), m_venus_lane_simobject_16(params.venus_lane_simobject_16), m_venus_lane_simobject_17(params.venus_lane_simobject_17), m_venus_lane_simobject_18(params.venus_lane_simobject_18), m_venus_lane_simobject_19(params.venus_lane_simobject_19), m_venus_lane_simobject_20(params.venus_lane_simobject_20), m_venus_lane_simobject_21(params.venus_lane_simobject_21), m_venus_lane_simobject_22(params.venus_lane_simobject_22), m_venus_lane_simobject_23(params.venus_lane_simobject_23), m_venus_lane_simobject_24(params.venus_lane_simobject_24), m_venus_lane_simobject_25(params.venus_lane_simobject_25), m_venus_lane_simobject_26(params.venus_lane_simobject_26), m_venus_lane_simobject_27(params.venus_lane_simobject_27), m_venus_lane_simobject_28(params.venus_lane_simobject_28), m_venus_lane_simobject_29(params.venus_lane_simobject_29), m_venus_lane_simobject_30(params.venus_lane_simobject_30), m_venus_lane_simobject_31(params.venus_lane_simobject_31), m_venus_lane_simobject_32(params.venus_lane_simobject_32), m_venus_lane_simobject_33(params.venus_lane_simobject_33), m_venus_lane_simobject_34(params.venus_lane_simobject_34), m_venus_lane_simobject_35(params.venus_lane_simobject_35), m_venus_lane_simobject_36(params.venus_lane_simobject_36), m_venus_lane_simobject_37(params.venus_lane_simobject_37), m_venus_lane_simobject_38(params.venus_lane_simobject_38), m_venus_lane_simobject_39(params.venus_lane_simobject_39), m_venus_lane_simobject_40(params.venus_lane_simobject_40), m_venus_lane_simobject_41(params.venus_lane_simobject_41), m_venus_lane_simobject_42(params.venus_lane_simobject_42), m_venus_lane_simobject_43(params.venus_lane_simobject_43), m_venus_lane_simobject_44(params.venus_lane_simobject_44), m_venus_lane_simobject_45(params.venus_lane_simobject_45), m_venus_lane_simobject_46(params.venus_lane_simobject_46), m_venus_lane_simobject_47(params.venus_lane_simobject_47), m_venus_lane_simobject_48(params.venus_lane_simobject_48), m_venus_lane_simobject_49(params.venus_lane_simobject_49), m_venus_lane_simobject_50(params.venus_lane_simobject_50), m_venus_lane_simobject_51(params.venus_lane_simobject_51), m_venus_lane_simobject_52(params.venus_lane_simobject_52), m_venus_lane_simobject_53(params.venus_lane_simobject_53), m_venus_lane_simobject_54(params.venus_lane_simobject_54), m_venus_lane_simobject_55(params.venus_lane_simobject_55), m_venus_lane_simobject_56(params.venus_lane_simobject_56), m_venus_lane_simobject_57(params.venus_lane_simobject_57), m_venus_lane_simobject_58(params.venus_lane_simobject_58), m_venus_lane_simobject_59(params.venus_lane_simobject_59), m_venus_lane_simobject_60(params.venus_lane_simobject_60), m_venus_lane_simobject_61(params.venus_lane_simobject_61), m_venus_lane_simobject_62(params.venus_lane_simobject_62), m_venus_lane_simobject_63(params.venus_lane_simobject_63),
              m_venus_shuffle_pipeline_simobject(params.venus_shuffle_pipeline),

              lane_num(params.lane_num),
              bank_num(params.bank_num),
              line_num(params.line_num),
              vrf_base_addr(params.vrf_base_addr),
              lane_id(params.lane_id),
              experimentalVrfRr(params.experimental_vrf_rr),
              experimentalRequesterQVisibility(
                  params.experimental_requester_q_visibility),
              experimentalOneEntryOperandCommands(
                  params.experimental_one_entry_operand_commands),
              rtlPeCommandVisibilityCycles(
                  params.rtl_pe_command_visibility_cycles),
              rtlChainingEnabled(params.rtl_chaining_enabled)
        {
            fatal_if(rtlPeCommandVisibilityCycles == 0,
                     "PE command visibility must take at least one lane clock");
            NrLanes = lane_num;
            NrLines = line_num;
            NrBankPerLane = bank_num;
            for (unsigned int i = 0; i < 16; ++i)
            { // Example: Create 5 request ports
                VenusLaneRequestPorts.push_back(VenusLaneToVrfRequestPort(params.name + ".portrequesttovrf_" + std::to_string(i), i, this));
                portMapping[i] = "Physical meaning for port " + std::to_string(i); // Map index to name/meaning
            }
            if (lane_id == 0)
            {
                if(m_venus_shuffle_pipeline_simobject  != nullptr) m_venus_shuffle_pipeline = dynamic_cast<VenusShufflePipline*>(m_venus_shuffle_pipeline_simobject );

                                                         m_venus_lanes[0]  = this;
                if(m_venus_lane_simobject_1  != nullptr) m_venus_lanes[1]  = dynamic_cast<VenusLane*>(m_venus_lane_simobject_1 );
                if(m_venus_lane_simobject_2  != nullptr) m_venus_lanes[2]  = dynamic_cast<VenusLane*>(m_venus_lane_simobject_2 );
                if(m_venus_lane_simobject_3  != nullptr) m_venus_lanes[3]  = dynamic_cast<VenusLane*>(m_venus_lane_simobject_3 );
                if(m_venus_lane_simobject_4  != nullptr) m_venus_lanes[4]  = dynamic_cast<VenusLane*>(m_venus_lane_simobject_4 );
                if(m_venus_lane_simobject_5  != nullptr) m_venus_lanes[5]  = dynamic_cast<VenusLane*>(m_venus_lane_simobject_5 );
                if(m_venus_lane_simobject_6  != nullptr) m_venus_lanes[6]  = dynamic_cast<VenusLane*>(m_venus_lane_simobject_6 );
                if(m_venus_lane_simobject_7  != nullptr) m_venus_lanes[7]  = dynamic_cast<VenusLane*>(m_venus_lane_simobject_7 );
                if(m_venus_lane_simobject_8  != nullptr) m_venus_lanes[8]  = dynamic_cast<VenusLane*>(m_venus_lane_simobject_8 );
                if(m_venus_lane_simobject_9  != nullptr) m_venus_lanes[9]  = dynamic_cast<VenusLane*>(m_venus_lane_simobject_9 );
                if(m_venus_lane_simobject_10 != nullptr) m_venus_lanes[10] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_10);
                if(m_venus_lane_simobject_11 != nullptr) m_venus_lanes[11] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_11);
                if(m_venus_lane_simobject_12 != nullptr) m_venus_lanes[12] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_12);
                if(m_venus_lane_simobject_13 != nullptr) m_venus_lanes[13] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_13);
                if(m_venus_lane_simobject_14 != nullptr) m_venus_lanes[14] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_14);
                if(m_venus_lane_simobject_15 != nullptr) m_venus_lanes[15] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_15);
                if(m_venus_lane_simobject_16 != nullptr) m_venus_lanes[16] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_16);
                if(m_venus_lane_simobject_17 != nullptr) m_venus_lanes[17] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_17);
                if(m_venus_lane_simobject_18 != nullptr) m_venus_lanes[18] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_18);
                if(m_venus_lane_simobject_19 != nullptr) m_venus_lanes[19] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_19);
                if(m_venus_lane_simobject_20 != nullptr) m_venus_lanes[20] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_20);
                if(m_venus_lane_simobject_21 != nullptr) m_venus_lanes[21] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_21);
                if(m_venus_lane_simobject_22 != nullptr) m_venus_lanes[22] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_22);
                if(m_venus_lane_simobject_23 != nullptr) m_venus_lanes[23] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_23);
                if(m_venus_lane_simobject_24 != nullptr) m_venus_lanes[24] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_24);
                if(m_venus_lane_simobject_25 != nullptr) m_venus_lanes[25] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_25);
                if(m_venus_lane_simobject_26 != nullptr) m_venus_lanes[26] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_26);
                if(m_venus_lane_simobject_27 != nullptr) m_venus_lanes[27] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_27);
                if(m_venus_lane_simobject_28 != nullptr) m_venus_lanes[28] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_28);
                if(m_venus_lane_simobject_29 != nullptr) m_venus_lanes[29] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_29);
                if(m_venus_lane_simobject_30 != nullptr) m_venus_lanes[30] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_30);
                if(m_venus_lane_simobject_31 != nullptr) m_venus_lanes[31] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_31);
                if(m_venus_lane_simobject_32 != nullptr) m_venus_lanes[32] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_32);
                if(m_venus_lane_simobject_33 != nullptr) m_venus_lanes[33] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_33);
                if(m_venus_lane_simobject_34 != nullptr) m_venus_lanes[34] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_34);
                if(m_venus_lane_simobject_35 != nullptr) m_venus_lanes[35] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_35);
                if(m_venus_lane_simobject_36 != nullptr) m_venus_lanes[36] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_36);
                if(m_venus_lane_simobject_37 != nullptr) m_venus_lanes[37] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_37);
                if(m_venus_lane_simobject_38 != nullptr) m_venus_lanes[38] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_38);
                if(m_venus_lane_simobject_39 != nullptr) m_venus_lanes[39] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_39);
                if(m_venus_lane_simobject_40 != nullptr) m_venus_lanes[40] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_40);
                if(m_venus_lane_simobject_41 != nullptr) m_venus_lanes[41] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_41);
                if(m_venus_lane_simobject_42 != nullptr) m_venus_lanes[42] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_42);
                if(m_venus_lane_simobject_43 != nullptr) m_venus_lanes[43] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_43);
                if(m_venus_lane_simobject_44 != nullptr) m_venus_lanes[44] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_44);
                if(m_venus_lane_simobject_45 != nullptr) m_venus_lanes[45] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_45);
                if(m_venus_lane_simobject_46 != nullptr) m_venus_lanes[46] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_46);
                if(m_venus_lane_simobject_47 != nullptr) m_venus_lanes[47] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_47);
                if(m_venus_lane_simobject_48 != nullptr) m_venus_lanes[48] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_48);
                if(m_venus_lane_simobject_49 != nullptr) m_venus_lanes[49] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_49);
                if(m_venus_lane_simobject_50 != nullptr) m_venus_lanes[50] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_50);
                if(m_venus_lane_simobject_51 != nullptr) m_venus_lanes[51] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_51);
                if(m_venus_lane_simobject_52 != nullptr) m_venus_lanes[52] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_52);
                if(m_venus_lane_simobject_53 != nullptr) m_venus_lanes[53] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_53);
                if(m_venus_lane_simobject_54 != nullptr) m_venus_lanes[54] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_54);
                if(m_venus_lane_simobject_55 != nullptr) m_venus_lanes[55] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_55);
                if(m_venus_lane_simobject_56 != nullptr) m_venus_lanes[56] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_56);
                if(m_venus_lane_simobject_57 != nullptr) m_venus_lanes[57] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_57);
                if(m_venus_lane_simobject_58 != nullptr) m_venus_lanes[58] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_58);
                if(m_venus_lane_simobject_59 != nullptr) m_venus_lanes[59] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_59);
                if(m_venus_lane_simobject_60 != nullptr) m_venus_lanes[60] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_60);
                if(m_venus_lane_simobject_61 != nullptr) m_venus_lanes[61] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_61);
                if(m_venus_lane_simobject_62 != nullptr) m_venus_lanes[62] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_62);
                if(m_venus_lane_simobject_63 != nullptr) m_venus_lanes[63] = dynamic_cast<VenusLane*>(m_venus_lane_simobject_63);
            }
            for(int i=0; i<NrIDs; i++) {
                locallane_write_vinsn_progress[i] = 0;
                locallane_read_vinsn_progress[i] = 0;
                locallane_vinsn_has_mask[i] = false;
                locallane_vinsn_is_lsu[i] = false;
                locallane_vinsn_vfu[i] = VFU_NONE;
                locallane_accepted_vns_instr_id[i] = -1;
                locallane_retired_vns_instr_id[i] = -1;
                locallane_retirement_visible_tick[i] = 0;
                locallane_command_retired_vns_instr_id[i] = -1;
                locallane_command_retirement_visible_tick[i] = 0;
                locallane_writer_grant_count_q[i] = 0;
                locallane_writer_grant_count_d[i] = 0;
                locallane_writer_vfu_q[i] = VFU_NONE;
                locallane_writer_vfu_d[i] = VFU_NONE;
                locallane_writer_vaddr_q[i] = 0;
                locallane_writer_vaddr_d[i] = 0;
                locallane_writer_valid_q[i] = false;
                locallane_writer_valid_d[i] = false;
                locallane_writer_boundary_tick[i] = MaxTick;
                locallane_writer_q_tick[i] = MaxTick;
                for (int j = 0; j < NrIDs; ++j)
                    locallane_global_hazard_clear_visible_tick[i][j] =
                        MaxTick;
            }
        }

        void init() override;

        // 公共广播变量
        VenusHazardTable *venus_hazard_table = nullptr;

        bool listenHazardTableRequest(VenusHazardTable *pkt);
        bool LanehandleNewInstrRequest(VenusInstrPkt *pkt);
        bool transparentShuffleVFURequest(PacketPtr pkt);
        bool transparentVFUResponsetoShuffle(PacketPtr pkt);

        DoneEventFunctionWrapper nextVFUBitAluReportandRecycleInstrEvent;
        DoneEventFunctionWrapper nextVFUCAUReportandRecycleInstrEvent;
        DoneEventFunctionWrapper nextVFUSerdivReportandRecycleInstrEvent;
        DoneEventFunctionWrapper nextVFUShuffleReportandRecycleInstrEvent;
        VenusInstrPkt *bitalu_doneinstr_pkt = nullptr;
        VenusInstrPkt *cau_doneinstr_pkt = nullptr;
        VenusInstrPkt *serdiv_doneinstr_pkt = nullptr;
        VenusInstrPkt *tshuffle_doneinstr_pkt = nullptr;
        bool reportAndRecycleDoneInstr(VFU VFUType);

        //======================venus_lane_sequencer======================
        private:
          // 关键仿真组件
          LaneSequencerPushFIFOEventFunctionWrapper nextLaneSequencerPushFIFONewInstrEvent;
          // 内部私有变量
          VenusInstrPkt* recved_venus_instr_pkt = nullptr;
          std::list<VenusInstrPkt*> bitaluA_instr_FIFO_lanseq;
          std::list<VenusInstrPkt*> bitaluB_instr_FIFO_lanseq;
          std::list<VenusInstrPkt*> cauA_instr_FIFO_lanseq;
          std::list<VenusInstrPkt*> cauB_instr_FIFO_lanseq;
          std::list<VenusInstrPkt*> cauC_instr_FIFO_lanseq;
          std::list<VenusInstrPkt*> cauD_instr_FIFO_lanseq;
          std::list<VenusInstrPkt*> serdivA_instr_FIFO_lanseq;
          std::list<VenusInstrPkt*> serdivB_instr_FIFO_lanseq;
          std::list<VenusInstrPkt*> mask_instr_FIFO_lanseq;
          std::list<VenusInstrPkt*> shuffle_instr_FIFO_lanseq;//TRICK
          // 内部私有状态
          bool laneSequencerisBusy = false; //sequencer忙状态信息
          /*
           * RTL's lane input is a one-entry fall-through register.  A
           * handshaken request may therefore remain resident while the
           * per-operand one-entry command registers are occupied.  Keep the
           * accepted generation so repeated cycles of the main sequencer's
           * broadcast observe ready without enqueueing the same instruction
           * twice.
           */
          int laneSequencerLastAcceptedInstr = -1;
          /*
           * A request rejected while the fall-through entry is occupied is
           * still a stable valid_i payload in RTL.  If the resident entry
           * drains on the following edge, that held request may be consumed
           * by the downstream command logic on the very same edge.  Record
           * the presented generation separately from an accepted entry so
           * retry timing can reproduce that simultaneous pop/push boundary.
           */
          int laneSequencerHeldInstr = -1;
          Tick laneSequencerHeldSince = MaxTick;
          bool laneSequencerCanTransfer() const;
          Tick operandRequesterAvailableTick[10] = {};
          Tick operandCommandReleaseTick[10] = {};
          /*
           * requester_q.addr is updated by one RTL always_ff edge.  A retry
           * callback and the requester's ordinary service event may both run
           * in one gem5 tick, but they must not grant two successive rows in
           * that same lane cycle even when those rows target different banks.
           */
          Tick operandRequesterLastGrantTick[10] = {};
          Tick operandRequesterHandoffVisibleTick[10] = {};
          /*
           * RTL releases requester_q from the final VRF grant while the
           * SRAM response remains tagged with the old command.  Keep that
           * response owner separate from the next issuing owner so a
           * back-to-back command does not wait an extra response cycle.
           */
          std::array<VenusInstrPkt *, 10> operandRequesterDrainingPkt = {};
          std::array<bool, 10> operandRequesterDrainingDataValid = {};
          std::array<unsigned int, 10> operandRequesterDrainingData = {};
          std::array<int, 10> operandRequesterDrainingReadyCount = {};
          /*
           * operand_req_valid_o is cleared by an RTL always_ff block.  The
           * lane sequencer samples the old valid value on the handshake
           * edge, so an emptied command register is reusable only from the
           * following lane clock edge.
           */
          Tick operandCommandAckVisibleTick[10] = {};
          /*
           * venus_operand_queue does not derive ready from the returned-data
           * FIFO.  Its registered ibuf_usage_q is incremented by a VRF grant
           * (operand_issued_i), one memory-response boundary before data is
           * visible, and decremented by a VFU pop.  All combinational ready
           * decisions on one lane edge see the same pre-edge value.
           */
          std::array<unsigned, 10> operandQueueUsageQ = {};
          std::array<unsigned, 10> operandQueueUsageSnapshot = {};
          std::array<int, 10> operandQueueUsageDelta = {};
          std::array<Tick, 10> operandQueueUsageTick = {};
          unsigned maxLocalLaneCalcLen(const VenusInstrPkt *pkt) const;
          void occupyOperandCommandStages();
          void laneSequencerPushFIFONewInstr();
          bool laneSequencerPopFIFOforOperandRequester(OPERANDTYPE OperandType);

          //======================venus_operand_requester======================
          // 关键仿真组件
          OperandRequesterGetDataEventFunctionWrapper nextOperandRequesterGetsBitAlu_A_DataEvent;
          OperandRequesterGetDataEventFunctionWrapper nextOperandRequesterGetsBitAlu_B_DataEvent;
          OperandRequesterGetDataEventFunctionWrapper nextOperandRequesterGetsCAU_A_DataEvent;
          OperandRequesterGetDataEventFunctionWrapper nextOperandRequesterGetsCAU_B_DataEvent;
          OperandRequesterGetDataEventFunctionWrapper nextOperandRequesterGetsCAU_C_DataEvent;
          OperandRequesterGetDataEventFunctionWrapper nextOperandRequesterGetsCAU_D_DataEvent;
          OperandRequesterGetDataEventFunctionWrapper nextOperandRequesterGetsSerDiv_A_DataEvent;
          OperandRequesterGetDataEventFunctionWrapper nextOperandRequesterGetsSerDiv_B_DataEvent;
          OperandRequesterGetDataEventFunctionWrapper nextOperandRequesterGetsMask_DataEvent;
          OperandRequesterGetDataEventFunctionWrapper nextOperandRequesterGetsShuffleUnit_DataEvent;
          VenusInstrPkt* queueing_bitaluA_instr_pkt = nullptr;
          VenusInstrPkt* queueing_bitaluB_instr_pkt = nullptr;
          VenusInstrPkt* queueing_cauA_instr_pkt = nullptr;
          VenusInstrPkt* queueing_cauB_instr_pkt = nullptr;
          VenusInstrPkt* queueing_cauC_instr_pkt = nullptr;
          VenusInstrPkt* queueing_cauD_instr_pkt = nullptr;
          VenusInstrPkt* queueing_serdivA_instr_pkt = nullptr;
          VenusInstrPkt* queueing_serdivB_instr_pkt = nullptr;
          VenusInstrPkt* queueing_mask_instr_pkt = nullptr;
          VenusInstrPkt* queueing_shuffle_instr_pkt = nullptr;//TRICK
          bool operandrequester_bitaluA_busy = false;
          bool operandrequester_bitaluB_busy = false;
          bool operandrequester_cauA_busy    = false;
          bool operandrequester_cauB_busy    = false;
          bool operandrequester_cauC_busy    = false;
          bool operandrequester_cauD_busy    = false;
          bool operandrequester_serdivA_busy = false;
          bool operandrequester_serdivB_busy = false;
          bool operandrequester_mask_busy    = false;
          bool operandrequester_shuffle_busy = false;//TRICK

          bool operandrequester_bitaluA_push_instr_topush = false;
          bool operandrequester_bitaluB_push_instr_topush = false;
          bool operandrequester_cauA_push_instr_topush    = false;
          bool operandrequester_cauB_push_instr_topush    = false;
          bool operandrequester_cauC_push_instr_topush    = false;
          bool operandrequester_cauD_push_instr_topush    = false;
          bool operandrequester_serdivA_push_instr_topush = false;
          bool operandrequester_serdivB_push_instr_topush = false;
          bool operandrequester_mask_push_instr_topush    = false;
          bool operandrequester_shuffle_push_instr_topush = false;//TRICK
          bool operandrequester_bitaluA_push_data_topush = false;
          bool operandrequester_bitaluB_push_data_topush = false;
          bool operandrequester_cauA_push_data_topush    = false;
          bool operandrequester_cauB_push_data_topush    = false;
          bool operandrequester_cauC_push_data_topush    = false;
          bool operandrequester_cauD_push_data_topush    = false;
          bool operandrequester_serdivA_push_data_topush = false;
          bool operandrequester_serdivB_push_data_topush = false;
          bool operandrequester_mask_push_data_topush    = false;
          bool operandrequester_shuffle_push_data_topush = false;//TRICK
          bool operandrequester_bitaluA_datatoread_from_VRF = false;
          bool operandrequester_bitaluB_datatoread_from_VRF = false;
          bool operandrequester_cauA_datatoread_from_VRF    = false;
          bool operandrequester_cauB_datatoread_from_VRF    = false;
          bool operandrequester_cauC_datatoread_from_VRF    = false;
          bool operandrequester_cauD_datatoread_from_VRF    = false;
          bool operandrequester_serdivA_datatoread_from_VRF = false;
          bool operandrequester_serdivB_datatoread_from_VRF = false;
          bool operandrequester_mask_datatoread_from_VRF    = false;
          bool operandrequester_shuffle_datatoread_from_VRF = false;//TRICK
          bool operandrequester_bitaluA_datafrom_VRF_arrived = false;
          bool operandrequester_bitaluB_datafrom_VRF_arrived = false;
          bool operandrequester_cauA_datafrom_VRF_arrived    = false;
          bool operandrequester_cauB_datafrom_VRF_arrived    = false;
          bool operandrequester_cauC_datafrom_VRF_arrived    = false;
          bool operandrequester_cauD_datafrom_VRF_arrived    = false;
          bool operandrequester_serdivA_datafrom_VRF_arrived = false;
          bool operandrequester_serdivB_datafrom_VRF_arrived = false;
          bool operandrequester_mask_datafrom_VRF_arrived    = false;
          bool operandrequester_shuffle_datafrom_VRF_arrived = false;//TRICK

          void operandRequesterGetVectorData(OPERANDTYPE OperandType);


          int readdata_fromVRF_buf_BitAlu_A    = 0;/* init value here is trick */
          int readdata_fromVRF_buf_BitAlu_B    = 1;
          int readdata_fromVRF_buf_CAU_A       = 0;
          int readdata_fromVRF_buf_CAU_B       = 0;
          int readdata_fromVRF_buf_CAU_C       = 0;
          int readdata_fromVRF_buf_CAU_D       = 0;
          int readdata_fromVRF_buf_SerDiv_A    = 0;
          int readdata_fromVRF_buf_SerDiv_B    = 0;
          int readdata_fromVRF_buf_Mask        = 0;
          int readdata_fromVRF_buf_ShuffleUnit = 0;


          //======================venus_operand_queue======================
          std::list<VenusInstrPkt*> bitaluA_instr_FIFO_opqueue;
          std::list<VenusInstrPkt*> bitaluB_instr_FIFO_opqueue;
          std::list<VenusInstrPkt*> cauA_instr_FIFO_opqueue;
          std::list<VenusInstrPkt*> cauB_instr_FIFO_opqueue;
          std::list<VenusInstrPkt*> cauC_instr_FIFO_opqueue;
          std::list<VenusInstrPkt*> cauD_instr_FIFO_opqueue;
          std::list<VenusInstrPkt*> serdivA_instr_FIFO_opqueue;
          std::list<VenusInstrPkt*> serdivB_instr_FIFO_opqueue;
          std::list<VenusInstrPkt*> mask_instr_FIFO_opqueue;
          std::list<VenusInstrPkt*> shuffle_instr_FIFO_opqueue;//TRICK

          /*
           * RTL keeps an operand command at the head of every queue while its
           * data rows are consumed.  The old model stored commands and data in
           * unrelated FIFOs and therefore relied on arrival order alone.  Once
           * requesters overlap, that can pair a recycled running ID (or the
           * following instruction) with the wrong row.  Carry both allocation
           * identity fields with every row and validate them at VFU consume.
           */
          struct TaggedOperandData
          {
              unsigned int data = 0;
              unsigned int running_id = 0;
              unsigned int vns_instr_id = 0;
              /* fifo_v3 is not fall-through: data accepted into D on this
               * edge is available at out_valid_o only after the next edge. */
              Tick visibleTick = 0;
          };

          std::list<TaggedOperandData> bitaluA_data_FIFO_opqueue;
          std::list<TaggedOperandData> bitaluB_data_FIFO_opqueue;
          std::list<TaggedOperandData> cauA_data_FIFO_opqueue;
          std::list<TaggedOperandData> cauB_data_FIFO_opqueue;
          std::list<TaggedOperandData> cauC_data_FIFO_opqueue;
          std::list<TaggedOperandData> cauD_data_FIFO_opqueue;
          std::list<TaggedOperandData> serdivA_data_FIFO_opqueue;
          std::list<TaggedOperandData> serdivB_data_FIFO_opqueue;
          std::list<TaggedOperandData> mask_data_FIFO_opqueue;
          std::list<TaggedOperandData> shuffle_data_FIFO_opqueue;//TRICK

          VenusInstrPkt* running_bitaluA_instr_pkt  = nullptr;
          VenusInstrPkt* running_bitaluB_instr_pkt  = nullptr;
          VenusInstrPkt* running_cauA_instr_pkt     = nullptr;
          VenusInstrPkt* running_cauB_instr_pkt     = nullptr;
          VenusInstrPkt* running_cauC_instr_pkt     = nullptr;
          VenusInstrPkt* running_cauD_instr_pkt     = nullptr;
          VenusInstrPkt* running_serdivA_instr_pkt  = nullptr;
          VenusInstrPkt* running_serdivB_instr_pkt  = nullptr;
          VenusInstrPkt* running_mask_instr_pkt     = nullptr;
          VenusInstrPkt* running_shuffle_instr_pkt  = nullptr;//TRICK

          unsigned int   running_bitaluA_data_pkt;
          unsigned int   running_bitaluB_data_pkt;
          unsigned int   running_cauA_data_pkt;
          unsigned int   running_cauB_data_pkt;
          unsigned int   running_cauC_data_pkt;
          unsigned int   running_cauD_data_pkt;
          unsigned int   running_serdivA_data_pkt;
          unsigned int   running_serdivB_data_pkt;
          // RTL routes the tagged mask-queue output into an independent
          // operand latch in each VFU wrapper.  A single shared latch lets
          // an unrelated unmasked VFU clear another VFU's in-flight mask.
          unsigned int   running_bitalu_mask_data_pkt = 0;
          unsigned int   running_cau_mask_data_pkt = 0;
          unsigned int   running_serdiv_mask_data_pkt = 0;
          unsigned int   running_shuffle_mask_data_pkt = 0;
          unsigned int   running_bitalu_mask_row_pkt = 0;
          unsigned int   running_cau_mask_row_pkt = 0;
          unsigned int   running_serdiv_mask_row_pkt = 0;
          unsigned int   running_shuffle_mask_row_pkt = 0;
          bool running_bitalu_mask_row_valid = false;
          int running_bitalu_mask_row_running_id = -1;
          int running_bitalu_mask_row_instr_id = -1;
          unsigned int bitalu_mask_row_d = 0;
          bool bitalu_mask_row_valid_d = false;
          int bitalu_mask_row_running_id_d = -1;
          int bitalu_mask_row_instr_id_d = -1;
          Tick bitalu_mask_row_boundary_tick = MaxTick;
          bool running_cau_mask_row_valid = false;
          int running_cau_mask_row_running_id = -1;
          int running_cau_mask_row_instr_id = -1;
          unsigned int cau_mask_row_d = 0;
          bool cau_mask_row_valid_d = false;
          int cau_mask_row_running_id_d = -1;
          int cau_mask_row_instr_id_d = -1;
          Tick cau_mask_row_boundary_tick = MaxTick;
          bool running_serdiv_mask_row_valid = false;
          bool running_shuffle_mask_row_valid = false;
          unsigned int   running_shuffle_data_pkt;//TRICK
          bool operandQueuePushinstrFIFO(OPERANDTYPE OperandType, VenusInstrPkt* opinstr);
          bool operandQueuePushdataFIFO(OPERANDTYPE OperandType,
                                        unsigned int opdata,
                                        const VenusInstrPkt* instr);
          bool operandQueuePopinstrFIFO(OPERANDTYPE OperandType);
          bool vfuOperandInstructionsReady(VFU vfu) const;
          bool advanceCAUResultHold();
          bool operandQueuePopdataFIFO(OPERANDTYPE OperandType,
                                       const VenusInstrPkt* expected);
          bool operandQueuePopMaskData(unsigned int &opdata,
                                       const VenusInstrPkt* expected);
          bool operandQueueUNPopinstrFIFO(OPERANDTYPE OperandType, VenusInstrPkt* instr);
          bool operandQueueUNPopdataFIFO(OPERANDTYPE OperandType,
                                         unsigned int opdata,
                                         const VenusInstrPkt* instr);

          //======================venus_vfu======================
          // 关键仿真组件
          VFUCalcEventFunctionWrapper nextVFUBitAluCalcEvent;
          VFUCalcEventFunctionWrapper nextVFUCAUCalcEvent;
          VFUCalcEventFunctionWrapper nextVFUSerDivCalcEvent;
          VFUCalcEventFunctionWrapper nextVFUShuffleCalcEvent;

          VenusInstrPkt* running_bitalu_instr_pkt   = nullptr;
          VenusInstrPkt* running_cau_instr_pkt      = nullptr;
          VenusInstrPkt* running_serdiv_instr_pkt   = nullptr;
          VenusInstrPkt* running_tshuffle_instr_pkt = nullptr;//TRICK

          bool vfu_bitalu_busy  = false;
          bool vfu_cau_busy     = false;
          bool vfu_serdiv_busy  = false;
          bool vfu_shuffle_busy = false;//TRICK

          bool vfu_bitalu_busy_calculating  = false;
          bool vfu_cau_busy_calculating     = false;
          bool vfu_serdiv_busy_calculating  = false;
          bool vfu_shuffle_busy_calculating = false;//TRICK

          venus_vfu venus_function_unit;
          int getVFUProcessingTime(VenusInstrPkt* pkt);
          int getVFUPipeLength(VenusInstrPkt* pkt);
          int getCauPipeLength(VenusInstrPkt* pkt);
          unsigned int getVFUReduceResult(VenusOp op, VEW vew, int vl);

          int reduce_delay_count = 0;

          //======================venus_vfu_aluunits======================
          VFUStartCalcEventFunctionWrapper nextVFUBitAluStartCalcEvent;
          VFUStartCalcEventFunctionWrapper nextVFUCAUStartCalcEvent;
          VFUStartCalcEventFunctionWrapper nextVFUSerdivStartCalcEvent;
          VFUStartCalcEventFunctionWrapper nextVFUSerdivResultQueueEvent;
          VFUStartCalcEventFunctionWrapper nextVFUShuffleUnitStartCalcEvent;
          unsigned int bitalu_vd1_result_buf, bitalu_vmask_result_buf, bitalu_vd1_result_buf_pipeout, bitalu_vmask_result_buf_pipeout, bitalu_vd1_mask_buf_pipeout, bitalu_vmask_mask_buf_pipeout;
          VenusInstrPkt* bitalu_vd1_instr_buf_pipeout = nullptr;
          VenusInstrPkt* bitalu_vmask_instr_buf_pipeout = nullptr;
          datapipe bitalu_vd1_result_buf_pipeline, bitalu_vmask_result_buf_pipeline;
          struct TaggedBitAluResult
          {
              unsigned int vd1;
              unsigned int vmask;
              unsigned int mask;
              uint8_t maskRowBits = 0;
              unsigned maskAddr = 0;
              bool maskExternalValid = false;
              /* venus_bitalu writes result_queue_d combinationally.  Only
               * result_queue_q may drive the bank requesters. */
              Tick visibleTick = 0;
              std::shared_ptr<VenusInstrPkt> instr;
              bool vd1Done = false;
              bool maskDone = false;
          };
          static constexpr unsigned BitAluResultQueueDepth = 2;
          std::deque<TaggedBitAluResult> bitaluResultQueue;
          Tick bitaluLastVrfGrantTick = static_cast<Tick>(-1);
          int bitaluMaskAccumulatorInstr = -1;
          int bitaluMaskAccumulatorRunningId = -1;
          unsigned bitaluMaskAccumulatorRow = 0;
          uint8_t bitaluMaskAccumulatorBits = 0;
          bool serviceBitAluResultQueue();
          bool serviceBitAluMaskResult(TaggedBitAluResult &result);
          VenusInstrPkt *cloneBitAluResultTag() const;

          unsigned int cau_vd1_result_buf, cau_vd2_result_buf, cau_vd1_result_buf_pipeout, cau_vd2_result_buf_pipeout, cau_vd1_mask_buf_pipeout, cau_vd2_mask_buf_pipeout;
          VenusInstrPkt* cau_vd1_instr_buf_pipeout = nullptr;
          VenusInstrPkt* cau_vd2_instr_buf_pipeout = nullptr;
          datapipe cau_vd1_result_buf_pipeline, cau_vd2_result_buf_pipeline;
          struct TaggedCauResult
          {
              unsigned int vd1;
              unsigned int vd2;
              unsigned int mask;
              std::shared_ptr<VenusInstrPkt> instr;
              Tick visibleTick = 0;
              bool vd1Done = false;
              bool vd2Done = false;
          };
          struct TaggedCauPipelineResult
          {
              unsigned int vd1;
              unsigned int vd2;
              unsigned int mask;
              std::shared_ptr<VenusInstrPkt> instr;
              Tick readyTick;
          };
          static constexpr unsigned CauResultQueueDepth = 2;
          std::deque<TaggedCauResult> cauResultQueue;
          std::deque<TaggedCauPipelineResult> cauPipelineResults;
          Tick cauLastVrfGrantTick = static_cast<Tick>(-1);
          bool serviceCauResultQueue();

          unsigned int serdiv_vd1_result_buf;
          unsigned int serdiv_vd1_result_buf_pipeout;
          unsigned int serdiv_vd1_mask_buf_pipeout;
          VenusInstrPkt* serdiv_vd1_instr_buf_pipeout = nullptr;
          struct TaggedSerDivResult
          {
              unsigned int vd1;
              unsigned int mask;
              std::shared_ptr<VenusInstrPkt> instr;
              bool vd1Done = false;
          };
          static constexpr unsigned SerDivResultQueueDepth = 2;
          std::deque<TaggedSerDivResult> serdivResultQueue;
          Tick serdivLastVrfGrantTick = static_cast<Tick>(-1);
          bool serviceSerDivResultQueue();
          void serviceSerDivResultQueueEvent();

          unsigned int shuffle_vd1_result_buf;//TRICK

          bool bitalu_vd1_result_writeback_done, bitalu_mask_result_writeback_done, bitalu_vd1_result_calc_done;
          bool cau_vd1_result_writeback_done, cau_vd2_result_writeback_done, cau_vd1_result_calc_done;
          bool serdiv_vd1_result_writeback_done, serdiv_vd1_result_calc_done;
          bool shuffle_vd1_result_writeback_done, shuffle_vd1_result_calc_done;//TRICK
          void VFUCalc(VFU VFUType);
          void VFUBitAluCalculating();
          void VFUCAUCalculating();
          void VFUSerdivCalculating();
          void VFUShuffleUnitCalculating();
          bool getBitAluResult();
          bool getCAUResult();
          bool getSerdivResult();
          bool getShuffleUnitResult();

          //======================venus_operandrequester_writeback======================
          bool operandrequester_writeback_bitalu_busy      = false;
          bool operandrequester_writeback_bitalumask_busy  = false;
          bool operandrequester_writeback_caua_busy        = false;
          bool operandrequester_writeback_caub_busy        = false;
          bool operandrequester_writeback_serdiv_busy      = false;
          bool operandrequester_writeback_shuffle_busy     = false;

          VenusInstrPkt* writtingback_bitalu_instr_pkt = nullptr;
          VenusInstrPkt* writtingback_cau_instr_pkt = nullptr;
          VenusInstrPkt* writtingback_serdiv_instr_pkt = nullptr;
          VenusInstrPkt* writtingback_tshuffle_instr_pkt = nullptr;//TRICK

          bool operandrequester_writeback_bitalu_data_topush      = false;
          bool operandrequester_writeback_bitalumask_data_topush  = false;
          bool operandrequester_writeback_cauA_data_topush        = false;
          bool operandrequester_writeback_cauB_data_topush        = false;
          bool operandrequester_writeback_serdiv_data_topush      = false;
          bool operandrequester_writeback_shuffle_data_topush     = false;//TRICK

          bool operandRequesterSetVectorData(OPERANDPASSAGE OperandPassageType);


          //TRICK
          bool requestArbiter(int headline, int offset, int ot,
                              unsigned int writebackdata,
                              unsigned int write_size = 2,
                              int request_running_id = -1,
                              int request_vns_instr_id = -1,
                              bool pipelined_read = false);
          GenerateVRFDataEventFunctionWrapper nextTRICKgenerateVRFReadData_BitAlu_A;
          GenerateVRFDataEventFunctionWrapper nextTRICKgenerateVRFReadData_BitAlu_B;
          GenerateVRFDataEventFunctionWrapper nextTRICKgenerateVRFReadData_CAU_A;
          GenerateVRFDataEventFunctionWrapper nextTRICKgenerateVRFReadData_CAU_B;
          GenerateVRFDataEventFunctionWrapper nextTRICKgenerateVRFReadData_CAU_C;
          GenerateVRFDataEventFunctionWrapper nextTRICKgenerateVRFReadData_CAU_D;
          GenerateVRFDataEventFunctionWrapper nextTRICKgenerateVRFReadData_SerDiv_A;
          GenerateVRFDataEventFunctionWrapper nextTRICKgenerateVRFReadData_SerDiv_B;
          GenerateVRFDataEventFunctionWrapper nextTRICKgenerateVRFReadData_Mask;
          GenerateVRFDataEventFunctionWrapper nextTRICKgenerateVRFReadData_ShuffleUnit;
          /* Convert RTL cycle counts through this lane's clock domain. */
          Tick afterCycles(Cycles cycles) const
          {
              return curTick() + cyclesToTicks(cycles);
          }
          void generateVRFReadData(OPERANDTYPE ot);

          int locallane_write_vinsn_progress[NrIDs];
          int locallane_read_vinsn_progress[NrIDs];
          bool locallane_vinsn_has_mask[NrIDs];
          bool locallane_vinsn_is_lsu[NrIDs];
          VFU locallane_vinsn_vfu[NrIDs];
          int locallane_accepted_vns_instr_id[NrIDs];
          int locallane_retired_vns_instr_id[NrIDs];
          Tick locallane_retirement_visible_tick[NrIDs];
          int locallane_command_retired_vns_instr_id[NrIDs];
          Tick locallane_command_retirement_visible_tick[NrIDs];
          /*
           * Venus1 broadcasts one global hazard table to every operand
           * requester in a lane.  A true->false bit transition is sampled
           * by all requester_d instances on the same edge and is visible in
           * every requester_q on the following edge.  Keep that shared
           * edge here; deriving it independently inside A/B/C/D requester
           * callbacks incorrectly serializes physically parallel D/Q
           * transitions.
           */
          Tick locallane_global_hazard_clear_visible_tick[NrIDs][NrIDs];
          uint64_t locallane_writer_grant_count_q[NrIDs];
          uint64_t locallane_writer_grant_count_d[NrIDs];
          VFU locallane_writer_vfu_q[NrIDs];
          VFU locallane_writer_vfu_d[NrIDs];
          Addr locallane_writer_vaddr_q[NrIDs];
          Addr locallane_writer_vaddr_d[NrIDs];
          bool locallane_writer_valid_q[NrIDs];
          bool locallane_writer_valid_d[NrIDs];
          Tick locallane_writer_boundary_tick[NrIDs];
          Tick locallane_writer_q_tick[NrIDs];
    };
}
#endif // __SIM_VENUS_LANE_HH__
