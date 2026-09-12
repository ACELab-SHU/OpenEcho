#ifndef VENUS_INSTR_PKT_H
#define VENUS_INSTR_PKT_H
#include "mem/packet.hh"

#include <iostream>
#include <string>
#include <random>
#include <list>
#include <fstream>
#include <filesystem>
#include <vector>
#include "json.hpp"
#include "sim/sim_object.hh"
#include "mem/venus_vrf_mem.hh"
#include "venus_extension_pkg.hh"
namespace gem5
{

    int getRandomInt(int min, int max);
    int getRandomChoice(int range, int d0, int d1 = 0, int d2 = 0, int d3 = 0, int d4 = 0, int d5 = 0, int d6 = 0, int d7 = 0);

    typedef enum
    {
        VS1,
        VS2,
        VD1,
        VD2,
        VM,
        GLOBAL
    } hazard_element;

    class VenusInstrPkt : public Packet
    {
    public:
        static unsigned int vns_instr_gencounter;

    public:
        static bool cmp_vns_instr_id(VenusInstrPkt *A, VenusInstrPkt *B);

        unsigned int vns_instr_id;
        unsigned int running_id;
        bool use_vs1;
        bool use_vs2;
        bool use_vd2;
        bool use_vd1;
        bool use_vd1_op;
        bool use_vd2_op;
        VenusOp op; // 枚举类型
        VEW vew;    // vew_e 类型
        FUNC3 func3;
        int vl; // vlen_t 类型，假设是int
        bool vm_r;
        bool vm_w;
        int scalar_op; // elen_t 类型，假设是int
        bool use_scalar_op;

        std::list<VFU> vfu_lst;
        std::list<OPERANDTYPE> operand_lst;
        int locallane_vs1_operand_len;
        int locallane_vs1_dataready_cnt = 0;
        int locallane_vs2_operand_len;
        int locallane_vs2_dataready_cnt = 0;
        int locallane_vd1_operand_len;
        int locallane_vd1_dataready_cnt = 0;
        int locallane_vd2_operand_len;
        int locallane_vd2_dataready_cnt = 0;
        int locallane_vmask_operand_len;
        int locallane_vmask_dataready_cnt = 0;
        /*
         * Requester-local admission progress.  Each operand requester owns a
         * packet copy, so this is independent from its completed/pushed
         * dataready counter.  RTL may have multiple tagged SRAM reads between
         * these two boundaries.
         */
        int operand_issue_counter = 0;
        // Edge at which a requester_q command may first drive the VRF.
        // The lane sequencer may already have delivered the VFU operation
        // and operand-queue tag while this registered boundary is pending.
        Tick operand_request_visible_tick = 0;
        int locallane_bitalu_calctime_len;
        int locallane_bitalu_calc_cnt = 0;
        int locallane_cau_calctime_len;
        int locallane_cau_calc_cnt = 0;
        int locallane_serdiv_calctime_len;
        int locallane_serdiv_calc_cnt = 0;
        int locallane_shuffle_calctime_len;
        int locallane_shuffle_calc_cnt = 0;
        int locallane_vd1_writeback_len = 2147483647; // do not change the initial val, its necessary.
        int locallane_vd1_writeback_cnt = 0;
        int locallane_vd2_writeback_len = 2147483647;
        int locallane_vd2_writeback_cnt = 0;
        int locallane_vmask_writeback_len = 2147483647;
        int locallane_vmask_writeback_cnt = 0;
        bool vs1_operand_ready = false;
        bool vs2_operand_ready = false;
        bool vd1_operand_ready = false;
        bool vd2_operand_ready = false;
        bool mask_operand_ready = false;
        bool scalar_operand_ready = true;

        /*
         * RTL operand requesters latch their RAW bitmap with the command and
         * retain one non-accumulating credit per producer.  These fields are
         * requester-local state: a later hazard-table broadcast or running-ID
         * reuse must not replace the dependency generation being consumed.
         */
        bool chain_raw_hazard[NrIDs] = {};
        bool chain_raw_source_dependency[NrIDs] = {};
        bool chain_raw_credit[NrIDs] = {};
        bool chain_raw_credit_eligible[NrIDs] = {};
        /*
         * Per-producer registered grant state.  Value 1 is a token computed
         * into requester_d and visible to requester_q on the next lane edge;
         * value 2 means a requester grant consumed the token and is waiting
         * for a later producer pulse.  The producer's vinsn_writeback_q is
         * modeled separately by VenusLane's registered writer boundary.
         */
        uint8_t chain_raw_credit_pipe[NrIDs] = {};
        Tick chain_raw_credit_visible_tick[NrIDs] = {};
        int chain_raw_seen_progress[NrIDs] = {};
        int chain_raw_producer_instr[NrIDs] = {};
        /*
         * Producer class belongs to the tagged RAW generation.  LSU
         * instructions do not enter VenusLane, so a lane-local running-ID
         * lookup can otherwise observe the VFU class left by an older
         * generation and incorrectly enable row chaining.
         */
        bool chain_raw_producer_is_lsu[NrIDs] = {};
        /*
         * VFU class belongs to the same tagged producer generation.  A
         * running-ID lookup after capture can already describe a younger
         * command, so same-VFU retirement rules must use this snapshot.
         */
        VFU chain_raw_producer_vfu[NrIDs] = {};
        bool chain_war_hazard[NrIDs] = {};
        bool chain_waw_hazard[NrIDs] = {};
        int chain_retire_victim_instr[NrIDs] = {};
        bool chain_retire_victim_is_lsu[NrIDs] = {};
        bool chain_overlap_active = false;
        bool chain_raw_pipeline_active = false;

        /*
         * venus_shuffle_engine.sv stores the five Shuffle-relevant hazard
         * vectors in the accepted pe_req queue entry.  The queued copy is
         * then masked by the live global table every cycle, so a cleared
         * dependency can never be resurrected by running-ID reuse or by a
         * later hazard-table image.  Preserve the equivalent combined bit
         * and the exact producer generation with the command packet.
         */
        bool shuffle_hazard_snapshot_valid = false;
        bool shuffle_hazard_snapshot[NrIDs] = {};
        int shuffle_hazard_generation[NrIDs] = {};

        // 地址范围
        int vs1_head;
        int vs1_tail;
        int vs2_head;
        int vs2_tail;
        int vd1_head;
        int vd1_tail;
        int vd2_head;
        int vd2_tail;
        int line_usage;

        INSTR_STAT vns_instr_stat;
        long vns_instr_log_firetick = -1;
        long vns_instr_log_recycletick = -1;
        long vns_instr_log_starttick = -1;
        long vns_instr_log_endtick = -1;

        unsigned char vfu_shamt = 0;
        unsigned char saturate_pre_adder = 1;
        unsigned char saturate_multiplier = 1;
        unsigned char saturate_post_adder = 1;

        std::vector<uint16_t> result_data;
        std::vector<uint8_t> result_valid;
        std::vector<uint16_t> result_data_vd2;
        std::vector<uint8_t> result_valid_vd2;

        // 显示函数
        void display() const;

        // 随机化设置值的函数
        void randomize();
        void autocomplete();

        VenusInstrPkt();
        VenusInstrPkt(VenusInstrPkt *pkt);
        VenusInstrPkt(VenusOp op, VEW vew, FUNC3 func3, int vl, int vs1_head, int vs2_head, int vd1_head, int vd2_head);
        VenusInstrPkt(VenusOp op, VEW vew, FUNC3 func3, int vl, int vs1_head, int vs2_head, int vd1_head, int vd2_head, bool vm_r);
        VenusInstrPkt(VenusOp op, VEW vew, FUNC3 func3, int vl, int vs1_head, int vs2_head, int vd1_head, int vd2_head, bool vm_r, unsigned int scalar_op);
        VenusInstrPkt(VenusOp op, VEW vew, FUNC3 func3, int vl, int vs1_head, int vs2_head, int vd1_head, int vd2_head, bool vm_r, unsigned int scalar_op, unsigned char vfu_shamt, unsigned char saturate_pre_adder, unsigned char saturate_multiplier, unsigned char saturate_post_adder);

        static void createVinsInfo();
        static void saveVinsInfo();
        void dumpVinsInfo();
        void dumpVinsResult(memory::venus_vrf_mem *m_venus_vrf);
        void dumpVinsResult(memory::venus_vrf_mem *m_venus_vrf,
                            unsigned int dump_id,
                            const std::string &dump_dir =
                                "Debug/venusgem5_vins_result");
        void recordResultWrite(int lane_id, bool is_vd2, int offset_bytes,
                               uint16_t writeback_data, unsigned int size);
        void mergeResultDataFrom(const VenusInstrPkt *pkt);
    private:
        bool validate_venus_ext_instr();
        void ensureResultData();
        std::string capturedResultString(bool is_vd2) const;
        int resultSlotCount() const;
        int resultDumpVl() const;
    };
    class VenusHazardTable : public Packet
    {
    public:
        bool global_hazard_table[NrIDs][NrIDs];
        bool vs1_hazard_table[NrIDs][NrIDs];
        bool vs2_hazard_table[NrIDs][NrIDs];
        bool vd1_hazard_table[NrIDs][NrIDs];
        bool vd2_hazard_table[NrIDs][NrIDs];
        bool vm_hazard_table[NrIDs][NrIDs];
        bool raw_hazard_table[NrIDs][NrIDs] = {};
        bool war_hazard_table[NrIDs][NrIDs] = {};
        bool waw_hazard_table[NrIDs][NrIDs] = {};
        bool is_shuffle[NrIDs];
        int running_id_to_vns_instr_id[NrIDs];
        int retired_vns_instr_id[NrIDs];

        /*
         * The RTL sequencer exposes an LSU pe_resp completion to the lane
         * operand requesters through the combinational running/hazard table
         * before the registered global table is updated.  This tagged
         * sideband models only that requester-local observation; it is not
         * a replacement hazard-table broadcast.
         */
        bool lsu_completion_valid = false;
        int lsu_completion_running_id = -1;
        int lsu_completion_vns_instr_id = -1;

        /*
         * Non-lane producers (currently the shuffle engine) also remove
         * their running bit combinationally in the pe_resp cycle.  Carry
         * that tagged observation independently from the registered VID
         * release so lane requesters can fold the producer out at the same
         * boundary as RTL.
         */
        bool producer_completion_valid = false;
        int producer_completion_running_id = -1;
        int producer_completion_vns_instr_id = -1;

        /*
         * Lane arithmetic writeback is complete only after every active
         * lane has received its final destination-bank grant.  RTL carries
         * that reduction through a registered command-completion boundary
         * before a same-VFU requester may clear its captured hazard.
         */
        bool lane_command_completion_valid = false;
        int lane_command_completion_running_id = -1;
        int lane_command_completion_vns_instr_id = -1;

        VenusHazardTable():Packet(std::make_shared<Request>(0, 0, 0, 0), MemCmd::ReadReq) {
            for(int i=0; i<NrIDs; i++) {
                running_id_to_vns_instr_id[i] = -1;
                retired_vns_instr_id[i] = -1;
                is_shuffle[i] = false;
            }
        };
        VenusHazardTable(VenusHazardTable *pkt) : Packet(std::make_shared<Request>(0, 0, 0, 0), MemCmd::ReadReq)
        {
            if (pkt)
            {
                // 使用 memcpy 进行高效的内存拷贝
                std::memcpy(global_hazard_table, pkt->global_hazard_table, sizeof(global_hazard_table));
                std::memcpy(vs1_hazard_table, pkt->vs1_hazard_table, sizeof(vs1_hazard_table));
                std::memcpy(vs2_hazard_table, pkt->vs2_hazard_table, sizeof(vs2_hazard_table));
                std::memcpy(vd1_hazard_table, pkt->vd1_hazard_table, sizeof(vd1_hazard_table));
                std::memcpy(vd2_hazard_table, pkt->vd2_hazard_table, sizeof(vd2_hazard_table));
                std::memcpy(vm_hazard_table, pkt->vm_hazard_table, sizeof(vm_hazard_table));
                std::memcpy(raw_hazard_table, pkt->raw_hazard_table, sizeof(raw_hazard_table));
                std::memcpy(war_hazard_table, pkt->war_hazard_table, sizeof(war_hazard_table));
                std::memcpy(waw_hazard_table, pkt->waw_hazard_table, sizeof(waw_hazard_table));
                std::memcpy(is_shuffle, pkt->is_shuffle, sizeof(is_shuffle));
                std::memcpy(running_id_to_vns_instr_id, pkt->running_id_to_vns_instr_id, sizeof(running_id_to_vns_instr_id));
                std::memcpy(retired_vns_instr_id, pkt->retired_vns_instr_id, sizeof(retired_vns_instr_id));
                lsu_completion_valid = pkt->lsu_completion_valid;
                lsu_completion_running_id =
                    pkt->lsu_completion_running_id;
                lsu_completion_vns_instr_id =
                    pkt->lsu_completion_vns_instr_id;
                producer_completion_valid =
                    pkt->producer_completion_valid;
                producer_completion_running_id =
                    pkt->producer_completion_running_id;
                producer_completion_vns_instr_id =
                    pkt->producer_completion_vns_instr_id;
                lane_command_completion_valid =
                    pkt->lane_command_completion_valid;
                lane_command_completion_running_id =
                    pkt->lane_command_completion_running_id;
                lane_command_completion_vns_instr_id =
                    pkt->lane_command_completion_vns_instr_id;
            }
        };

        void display()
        {
            std::cout << "==================::::::::global_hazard_table_o::::::::==================" << std::endl;
            for (int i = 0; i < NrIDs; i++)
            {
                for (int j = 0; j < NrIDs; j++)
                {
                    std::cout << "\t" << global_hazard_table[i][j] << ",";
                }
                std::cout << std::endl;
            }
        }

        bool hazardGenerationRetired(int older_id) const
        {
            panic_if(older_id < 0 || older_id >= NrIDs,
                     "illegal hazard producer id %d", older_id);
            const int runningGeneration =
                running_id_to_vns_instr_id[older_id];
            return runningGeneration >= 0 &&
                retired_vns_instr_id[older_id] >= runningGeneration;
        }

        bool checkifreadytofire(int id, bool ignore_chainable = false)
        {
            panic_if(id >= NrIDs, "illegal id");
            for (int i = 0; i < NrIDs; i++)
            {
                if (global_hazard_table[id][i] &&
                    !hazardGenerationRetired(i)) {
                    if (ignore_chainable) {
                        // Explicit RAW/WAR/WAW classes may chain; shuffle
                        // and unclassified structural conflicts may not.
                        if (is_shuffle[i]) return false;
                        bool chainable = isRAWConflict(id, i) ||
                                         isWARConflict(id, i) ||
                                         isWAWConflict(id, i);
                        if (!chainable) return false; // 结构冲突或其他不可链接冲突
                    } else {
                        return false;
                    }
                }
            }
            return true;
        }

        bool checkRawOnlyChainAdmission(int id)
        {
            panic_if(id >= NrIDs, "illegal id");
            for (int older_id = 0; older_id < NrIDs; older_id++) {
                if (!global_hazard_table[id][older_id] ||
                    hazardGenerationRetired(older_id))
                    continue;

                /*
                 * Source-side RAW may accept before its first row credit.
                 * Destination WAR/WAW remains a retirement dependency and
                 * must not be hidden by the same admission decision.
                 */
                if (!raw_hazard_table[id][older_id] ||
                    war_hazard_table[id][older_id] ||
                    waw_hazard_table[id][older_id] ||
                    is_shuffle[older_id])
                    return false;
            }
            return true;
        }

        bool checkOperandReady(int id, bool check_vs1, bool check_vs2,
                               bool check_vd1, bool check_vd2,
                               bool check_vm)
        {
            panic_if(id >= NrIDs, "illegal id");
            for (int older_id = 0; older_id < NrIDs; older_id++) {
                const bool hazard =
                    (check_vs1 && vs1_hazard_table[id][older_id]) ||
                    (check_vs2 && vs2_hazard_table[id][older_id]) ||
                    (check_vd1 && vd1_hazard_table[id][older_id]) ||
                    (check_vd2 && vd2_hazard_table[id][older_id]) ||
                    (check_vm  && vm_hazard_table[id][older_id]);
                if (hazard && !hazardGenerationRetired(older_id))
                    return false;
            }
            return true;
        }

        bool isRAWConflict(int cur_id, int older_id)
        {
            return raw_hazard_table[cur_id][older_id] &&
                !hazardGenerationRetired(older_id);
        }

        bool isWARConflict(int cur_id, int older_id)
        {
            return war_hazard_table[cur_id][older_id] &&
                !hazardGenerationRetired(older_id);
        }

        bool isWAWConflict(int cur_id, int older_id)
        {
            return waw_hazard_table[cur_id][older_id] &&
                !hazardGenerationRetired(older_id);
        }

        bool isWARWAWConflict(int cur_id, int older_id)
        {
            return isWARConflict(cur_id, older_id) ||
                   isWAWConflict(cur_id, older_id);
        }

        int getRAWProducerID(int id)
        {
            int producer_id = -1;
            int max_instr_id = -1;
            for (int i = 0; i < NrIDs; i++)
            {
                if (isRAWConflict(id, i)) {
                    if (running_id_to_vns_instr_id[i] > max_instr_id) {
                        max_instr_id = running_id_to_vns_instr_id[i];
                        producer_id = i;
                    }
                }
            }
            if (producer_id != -1 && is_shuffle[producer_id]) return -1;
            return producer_id;
        }

        int getWARProducerID(int id)
        {
            int producer_id = -1;
            int max_instr_id = -1;
            for (int i = 0; i < NrIDs; i++)
            {
                if (isWARWAWConflict(id, i)) {
                    if (running_id_to_vns_instr_id[i] > max_instr_id) {
                        max_instr_id = running_id_to_vns_instr_id[i];
                        producer_id = i;
                    }
                }
            }
            if (producer_id != -1 && is_shuffle[producer_id]) return -1;
            return producer_id;
        }

        std::string reportHazardStat(int id)
        {
            panic_if(id >= NrIDs, "illegal id");
            std::string strtemp = "insn RunningVID " + std::to_string(id) + " hazard with the following RunningID: ";
            for (int i = 0; i < NrIDs; i++)
            {
                if (global_hazard_table[id][i] &&
                    !hazardGenerationRetired(i))
                    strtemp = strtemp + std::to_string(i) + ", ";
            }
            return strtemp;
        }
    };
    class LaneSequencerPushFIFOEventFunctionWrapper : public EventFunctionWrapper
    {
    public:
        // 2. 在构造函数中，将优先级（例如100，数值越小优先级越高）传递给基类
        LaneSequencerPushFIFOEventFunctionWrapper(const std::function<void()> &callback, const std::string &name)
            : EventFunctionWrapper(callback, name, false, 4) // -1高于常规优先级（0）
        {
        }
    };
    class OperandRequesterGetDataEventFunctionWrapper : public EventFunctionWrapper
    {
    public:
        // 2. 在构造函数中，将优先级（例如100，数值越小优先级越高）传递给基类
        OperandRequesterGetDataEventFunctionWrapper(const std::function<void()> &callback, const std::string &name)
            : EventFunctionWrapper(callback, name, false, 1) // -1高于常规优先级（0）
        {
        }
    };
    class VFUCalcEventFunctionWrapper : public EventFunctionWrapper
    {
    public:
        // 2. 在构造函数中，将优先级（例如100，数值越小优先级越高）传递给基类
        VFUCalcEventFunctionWrapper(const std::function<void()> &callback, const std::string &name)
            : EventFunctionWrapper(callback, name, false, 3) // -1高于常规优先级（0）
        {
        }
    };
    class VFUStartCalcEventFunctionWrapper : public EventFunctionWrapper
    {
    public:
        // 2. 在构造函数中，将优先级（例如100，数值越小优先级越高）传递给基类
        VFUStartCalcEventFunctionWrapper(const std::function<void()> &callback, const std::string &name)
            : EventFunctionWrapper(callback, name, false, 2) // -1高于常规优先级（0）
        {
        }
    };
    class GenerateVRFDataEventFunctionWrapper : public EventFunctionWrapper
    {
    public:
        // 2. 在构造函数中，将优先级（例如100，数值越小优先级越高）传递给基类
        GenerateVRFDataEventFunctionWrapper(const std::function<void()> &callback, const std::string &name)
            : EventFunctionWrapper(callback, name, false, 0) // -1高于常规优先级（0）
        {
        }
    };
    class DoneEventFunctionWrapper : public EventFunctionWrapper
    {
    public:
        // 2. 在构造函数中，将优先级（例如100，数值越小优先级越高）传递给基类
        DoneEventFunctionWrapper(const std::function<void()> &callback, const std::string &name)
            : EventFunctionWrapper(callback, name, false, -1) // -1高于常规优先级（0）
        {
        }
    };

    class datapipe
    {
        private:
        int length;
        unsigned int pipe[11];
        unsigned int pipe_mask[11];
        VenusInstrPkt* pipe_instr[11];
        bool pipe_valid[11];
        public:

        datapipe()
        {
            length = 10;
            for (int i = 0; i < 11; i++) {
                pipe[i] = 0;
                pipe_mask[i] = 0;
                pipe_instr[i] = nullptr;
                pipe_valid[i] = false;
            }
        }

        ~datapipe()
        {
            for (int i = 0; i < 11; i++) {
                delete pipe_instr[i];
                pipe_instr[i] = nullptr;
            }
        }

        void setPipeLength(int latency)
        {
            // std::cout<<"in to setPipeLength\n";
            if(latency > 10)
                panic("latency is too big.");
            if(latency < 0)
                panic("latency is too small.");
            if(this->length < latency)
            {
                for (int i = this->length; i >= 0; i--)
                    {pipe[i-this->length+latency] = pipe[i]; pipe_mask[i-this->length+latency] = pipe_mask[i]; pipe_instr[i-this->length+latency] = pipe_instr[i]; pipe_valid[i-this->length+latency] = pipe_valid[i]; pipe_instr[i] = nullptr; pipe_valid[i] = false;}
                for (int i = 0; i < latency-this->length; i++)
                    {pipe[i] = 0; pipe_mask[i] = 0; if(pipe_instr[i] != nullptr){/*std::cout<<"delete "<<pipe_instr[i]<<"\n";*/ delete pipe_instr[i]; pipe_instr[i] = nullptr;} pipe_valid[i] = false;}
                for (int i = latency+1; i < 11; i++)
                    {pipe[i] = 0; pipe_mask[i] = 0; if(pipe_instr[i] != nullptr){/*std::cout<<"delete "<<pipe_instr[i]<<"\n";*/ delete pipe_instr[i]; pipe_instr[i] = nullptr;} pipe_valid[i] = false;}
            }
            if(this->length > latency)
            {
                for (int i = 0; i <= latency; i++)
                    {pipe[i] = pipe[i+this->length-latency]; pipe_mask[i] = pipe_mask[i+this->length-latency]; pipe_instr[i] = pipe_instr[i+this->length-latency]; pipe_valid[i] = pipe_valid[i+this->length-latency]; pipe_instr[i+this->length-latency] = nullptr; pipe_valid[i+this->length-latency] = false;}
                for (int i = 10; i > latency; i--)
                    {pipe[i] = 0; pipe_mask[i] = 0; if(pipe_instr[i] != nullptr){/*std::cout<<"delete "<<pipe_instr[i]<<"\n";*/ delete pipe_instr[i]; pipe_instr[i] = nullptr;} pipe_valid[i] = false;}
            }
            this->length = latency;

            // std::cout<<"pipe = ";
            // for(int i = 0; i <= length; i++)
            //     std::cout<<"("<<pipe[i]<<",iid="<<(pipe_instr[i]!=nullptr?(pipe_instr[i]->vns_instr_id):-1)<<",addr="<<pipe_instr[i]<<") ";
            // std::cout<<", size = "<<size()<<"\n";
        }
        unsigned int front()
        {
            return pipe[length];
        }
        unsigned int front_mask()
        {
            return pipe_mask[length];
        }
        VenusInstrPkt* front_instr()
        {
            return pipe_instr[length];
        }
        bool front_valid() const
        {
            return pipe_valid[length];
        }
        unsigned int size()
        {
            unsigned int entries = 0;
            for (int i = 0; i <= length; i++)
                entries += pipe_valid[i] ? 1 : 0;
            return entries;
        }
        int pipeLength() const
        {
            return length;
        }
        void pop_pipe()
        {
            if(front_instr() != nullptr)
                delete front_instr();
            pipe_instr[length] = nullptr;
            pipe[length] = 0;
            pipe_mask[length] = 0;
            pipe_valid[length] = false;
            // std::cout<<"pipe = ";
            // for(int i = 0; i <= length; i++)
            //     std::cout<<"("<<pipe[i]<<",iid="<<(pipe_instr[i]!=nullptr?(pipe_instr[i]->vns_instr_id):-1)<<",addr="<<pipe_instr[i]<<") ";
            // std::cout<<", size = "<<size()<<"\n";
        }
        void push_pipe(unsigned int data = INT_MIN, unsigned int mask = INT_MIN, VenusInstrPkt* instr = nullptr)
        {
            for (int i = 10; i > length; i--)
                {pipe[i] = 0; pipe_mask[i] = 0; if(pipe_instr[i] != nullptr){/*std::cout<<"delete "<<pipe_instr[i]<<"\n";*/ delete pipe_instr[i]; pipe_instr[i] = nullptr;} pipe_valid[i] = false;}
            if (pipe_valid[length])
                panic("datapipe output overwritten before result handshake");
            for (int i = length; i > 0; i--)
                {pipe[i] = pipe[i-1]; pipe_mask[i] = pipe_mask[i-1]; pipe_instr[i] = pipe_instr[i-1]; pipe_valid[i] = pipe_valid[i-1]; pipe_instr[i-1] = nullptr; pipe_valid[i-1] = false;}
            pipe[0] = data;
            pipe_mask[0] = mask;
            pipe_instr[0] = instr;
            pipe_valid[0] = instr != nullptr;
            if(data != INT_MIN && instr == nullptr) panic("illegal push_pipe, assert !(data != INT_MIN && instr == nullptr) failed.");
            // if(data != INT_MIN)
            //     {std::cout<<"DATA "<<data<<" push_pipe, size = "<<size()<<"\n";}
            // else
            //     {std::cout<<"pipe movefront, size = "<<size()<<"\n";}

            // std::cout<<"pipe = ";
            // for(int i = 0; i <= length; i++)
            //     std::cout<<"("<<pipe[i]<<",iid="<<(pipe_instr[i]!=nullptr?(pipe_instr[i]->vns_instr_id):-1)<<",addr="<<pipe_instr[i]<<") ";
            // std::cout<<", size = "<<size()<<"\n";
        }

    };
}
#endif // VENUS_INSTR_PKT_H
