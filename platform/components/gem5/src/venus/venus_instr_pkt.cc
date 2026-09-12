#include "venus_instr_pkt.hh"

#include <iostream>
#include <random>
#include <climits>
#include <cstdlib>

namespace gem5
{

  namespace
  {
    std::filesystem::path
    venusDebugRoot()
    {
      const char *root = std::getenv("VENUS_GEM5_DEBUG_DIR");
      return (root != nullptr && root[0] != '\0') ? root : "Debug";
    }

    bool
    venusVinsMonitorEnabled()
    {
      /* This is an observation-only switch.  The monitor is populated after
       * architectural completion and consumed only by offline comparison
       * tools; it never feeds a Venus state transition or event tick. */
      static const bool enabled =
          std::getenv("VENUS_GEM5_DISABLE_VINS_MONITOR") == nullptr;
      return enabled;
    }

    int
    rtlDispatcherVectorRows(int vl, VEW vew)
    {
      /*
       * Mirror venus_dispatcher.sv's vector_head2tail expression literally.
       * The RTL remainder reduction includes bit log2(lanes*banks), while
       * the quotient shifts by log2(lanes*banks) + EW16 - vew.  In
       * particular, an exact EW16 multiple of one physical row therefore
       * occupies two scoreboard rows.  This is observable hardware hazard
       * behavior, even though it differs from a mathematical ceil(bytes /
       * row_bytes).
       */
      const unsigned laneBanks = NrLanes * NrBankPerLane;
      unsigned laneBanksLog2 = 0;
      for (unsigned value = laneBanks; value > 1; value >>= 1)
        ++laneBanksLog2;
      panic_if(laneBanks == 0 || (laneBanks & (laneBanks - 1)) != 0,
               "Venus RTL dispatcher requires power-of-two lanes*banks");
      const unsigned quotientShift =
          laneBanksLog2 + static_cast<unsigned>(EW16) -
          static_cast<unsigned>(vew);
      const unsigned remainderMask =
          (1U << (laneBanksLog2 + 1)) - 1;
      return (static_cast<unsigned>(vl) >> quotientShift) +
          ((static_cast<unsigned>(vl) & remainderMask) != 0);
    }
  }

  unsigned int VenusInstrPkt::vns_instr_gencounter = 0;

  bool VenusInstrPkt::cmp_vns_instr_id(VenusInstrPkt *A, VenusInstrPkt *B)
  {
    return A->vns_instr_id < B->vns_instr_id;
  }

  VenusInstrPkt::VenusInstrPkt() : Packet(std::make_shared<Request>(0, 0, 0, 0), MemCmd::ReadReq)
  {
    // vns_instr_id = VenusInstrPkt::vns_instr_gencounter++;
    vns_instr_stat = INSTR_GENERATED;
    vns_instr_log_firetick = 0;
    vns_instr_log_recycletick = 0;
    vns_instr_log_starttick = 0;
    vns_instr_log_endtick = 0;

    randomize();
  }
  VenusInstrPkt::VenusInstrPkt(VenusInstrPkt *pkt) : Packet(std::make_shared<Request>(0, 0, 0, 0), MemCmd::ReadReq)
  {
    vns_instr_id = pkt->vns_instr_id;
    running_id = pkt->running_id;
    use_vs1 = pkt->use_vs1;
    use_vs2 = pkt->use_vs2;
    use_vd2 = pkt->use_vd2;
    use_vd1 = pkt->use_vd1;
    use_vd1_op = pkt->use_vd1_op;
    use_vd2_op = pkt->use_vd2_op;
    op = pkt->op;
    vew = pkt->vew;
    func3 = pkt->func3;
    vl = pkt->vl;
    vm_r = pkt->vm_r;
    vm_w = pkt->vm_w;
    scalar_op = pkt->scalar_op;
    use_scalar_op = pkt->use_scalar_op;
    // These CSRs are part of the dispatched Venus request.  Packets are
    // copied several times between the sequencer, lane operand queues and
    // functional units; dropping them here silently reverts every copied
    // instruction to the class defaults (shift 0, all saturation enabled).
    // RTL pipelines the complete venus_req_t, so preserve the same state.
    vfu_shamt = pkt->vfu_shamt;
    saturate_pre_adder = pkt->saturate_pre_adder;
    saturate_multiplier = pkt->saturate_multiplier;
    saturate_post_adder = pkt->saturate_post_adder;

    std::copy(pkt->vfu_lst.begin(), pkt->vfu_lst.end(), std::back_inserter(this->vfu_lst));
    std::copy(pkt->operand_lst.begin(), pkt->operand_lst.end(), std::back_inserter(this->operand_lst));
    locallane_vs1_operand_len = pkt->locallane_vs1_operand_len;
    locallane_vs1_dataready_cnt = pkt->locallane_vs1_dataready_cnt;
    locallane_vs2_operand_len = pkt->locallane_vs2_operand_len;
    locallane_vs2_dataready_cnt = pkt->locallane_vs2_dataready_cnt;
    locallane_vd1_operand_len = pkt->locallane_vd1_operand_len;
    locallane_vd1_dataready_cnt = pkt->locallane_vd1_dataready_cnt;
    locallane_vd2_operand_len = pkt->locallane_vd2_operand_len;
    locallane_vd2_dataready_cnt = pkt->locallane_vd2_dataready_cnt;
    locallane_vmask_operand_len = pkt->locallane_vmask_operand_len;
    locallane_vmask_dataready_cnt = pkt->locallane_vmask_dataready_cnt;
    operand_issue_counter = pkt->operand_issue_counter;
    operand_request_visible_tick = pkt->operand_request_visible_tick;
    locallane_vd1_writeback_len = pkt->locallane_vd1_writeback_len;
    locallane_vd1_writeback_cnt = pkt->locallane_vd1_writeback_cnt;
    locallane_vd2_writeback_len = pkt->locallane_vd2_writeback_len;
    locallane_vd2_writeback_cnt = pkt->locallane_vd2_writeback_cnt;
    locallane_vmask_writeback_len = pkt->locallane_vmask_writeback_len;
    locallane_vmask_writeback_cnt = pkt->locallane_vmask_writeback_cnt;
    vs1_operand_ready = pkt->vs1_operand_ready;
    vs2_operand_ready = pkt->vs2_operand_ready;
    vd1_operand_ready = pkt->vd1_operand_ready;
    vd2_operand_ready = pkt->vd2_operand_ready;
    mask_operand_ready = pkt->mask_operand_ready;
    scalar_operand_ready = pkt->scalar_operand_ready;
    std::memcpy(chain_raw_hazard, pkt->chain_raw_hazard,
                sizeof(chain_raw_hazard));
    std::memcpy(chain_raw_source_dependency,
                pkt->chain_raw_source_dependency,
                sizeof(chain_raw_source_dependency));
    std::memcpy(chain_raw_credit, pkt->chain_raw_credit,
                sizeof(chain_raw_credit));
    std::memcpy(chain_raw_credit_eligible,
                pkt->chain_raw_credit_eligible,
                sizeof(chain_raw_credit_eligible));
    std::memcpy(chain_raw_credit_pipe, pkt->chain_raw_credit_pipe,
                sizeof(chain_raw_credit_pipe));
    std::memcpy(chain_raw_credit_visible_tick,
                pkt->chain_raw_credit_visible_tick,
                sizeof(chain_raw_credit_visible_tick));
    std::memcpy(chain_raw_seen_progress, pkt->chain_raw_seen_progress,
                sizeof(chain_raw_seen_progress));
    std::memcpy(chain_raw_producer_instr, pkt->chain_raw_producer_instr,
                sizeof(chain_raw_producer_instr));
    std::memcpy(chain_raw_producer_is_lsu,
                pkt->chain_raw_producer_is_lsu,
                sizeof(chain_raw_producer_is_lsu));
    std::memcpy(chain_raw_producer_vfu,
                pkt->chain_raw_producer_vfu,
                sizeof(chain_raw_producer_vfu));
    std::memcpy(chain_war_hazard, pkt->chain_war_hazard,
                sizeof(chain_war_hazard));
    std::memcpy(chain_waw_hazard, pkt->chain_waw_hazard,
                sizeof(chain_waw_hazard));
    std::memcpy(chain_retire_victim_instr,
                pkt->chain_retire_victim_instr,
                sizeof(chain_retire_victim_instr));
    std::memcpy(chain_retire_victim_is_lsu,
                pkt->chain_retire_victim_is_lsu,
                sizeof(chain_retire_victim_is_lsu));
    chain_overlap_active = pkt->chain_overlap_active;
    chain_raw_pipeline_active = pkt->chain_raw_pipeline_active;
    shuffle_hazard_snapshot_valid =
        pkt->shuffle_hazard_snapshot_valid;
    std::memcpy(shuffle_hazard_snapshot,
                pkt->shuffle_hazard_snapshot,
                sizeof(shuffle_hazard_snapshot));
    std::memcpy(shuffle_hazard_generation,
                pkt->shuffle_hazard_generation,
                sizeof(shuffle_hazard_generation));

    vs1_head = pkt->vs1_head;
    vs1_tail = pkt->vs1_tail;
    vs2_head = pkt->vs2_head;
    vs2_tail = pkt->vs2_tail;
    vd1_head = pkt->vd1_head;
    vd1_tail = pkt->vd1_tail;
    vd2_head = pkt->vd2_head;
    vd2_tail = pkt->vd2_tail;
    line_usage = pkt->line_usage;
    vns_instr_stat = pkt->vns_instr_stat;
    vns_instr_log_firetick = pkt->vns_instr_log_firetick;
    vns_instr_log_recycletick = pkt->vns_instr_log_recycletick;
    vns_instr_log_starttick = pkt->vns_instr_log_starttick;
    vns_instr_log_endtick = pkt->vns_instr_log_endtick;
    result_data = pkt->result_data;
    result_valid = pkt->result_valid;
    result_data_vd2 = pkt->result_data_vd2;
    result_valid_vd2 = pkt->result_valid_vd2;
  }
  VenusInstrPkt::VenusInstrPkt(VenusOp op, VEW vew, FUNC3 func3, int vl, int vs1_head, int vs2_head, int vd1_head, int vd2_head) : Packet(std::make_shared<Request>(0, 0, 0, 0), MemCmd::ReadReq)
  {
    // vns_instr_id = VenusInstrPkt::vns_instr_gencounter++;
    vns_instr_stat = INSTR_GENERATED;
    vns_instr_log_firetick = 0;
    vns_instr_log_recycletick = 0;
    vns_instr_log_starttick = 0;
    vns_instr_log_endtick = 0;

    this->op            = op ;
    this->vew           = vew;
    this->func3         = func3;
    this->vl            = vl;
    this->vs1_head      = vs1_head;
    this->vs2_head      = vs2_head;
    this->vd1_head      = vd1_head;
    this->vd2_head      = vd2_head;
    this->vm_r          = (VMASK_READ)getRandomInt(0,1);

    this->scalar_op           = (func3 == IVX || func3 == MVX) ? (vew == EW8 ? getRandomInt(0, 255) : getRandomInt(0, 65535)) : 0;

    this->vfu_shamt           = 0;
    this->saturate_pre_adder  = 1;
    this->saturate_multiplier = 1;
    this->saturate_post_adder = 1;

    autocomplete();
  }
  VenusInstrPkt::VenusInstrPkt(VenusOp op, VEW vew, FUNC3 func3, int vl, int vs1_head, int vs2_head, int vd1_head, int vd2_head, bool vm_r):Packet(std::make_shared<Request>(0, 0, 0, 0), MemCmd::ReadReq)
  {
    // vns_instr_id = VenusInstrPkt::vns_instr_gencounter++;
    vns_instr_stat = INSTR_GENERATED;
    vns_instr_log_firetick = 0;
    vns_instr_log_recycletick = 0;
    vns_instr_log_starttick = 0;
    vns_instr_log_endtick = 0;

    this->op            = op ;
    this->vew           = vew;
    this->func3         = func3;
    this->vl            = vl;
    this->vs1_head      = vs1_head;
    this->vs2_head      = vs2_head;
    this->vd1_head      = vd1_head;
    this->vd2_head      = vd2_head;
    this->vm_r          = (VMASK_READ)vm_r;

    this->scalar_op           = (func3 == IVX || func3 == MVX) ? (vew == EW8 ? getRandomInt(0, 255) : getRandomInt(0, 65535)) : 0;

    this->vfu_shamt           = 0;
    this->saturate_pre_adder  = 1;
    this->saturate_multiplier = 1;
    this->saturate_post_adder = 1;

    autocomplete();
  }
  VenusInstrPkt::VenusInstrPkt(VenusOp op, VEW vew, FUNC3 func3, int vl, int vs1_head, int vs2_head, int vd1_head, int vd2_head, bool vm_r, unsigned int scalar_op):Packet(std::make_shared<Request>(0, 0, 0, 0), MemCmd::ReadReq)
  {
    // vns_instr_id = VenusInstrPkt::vns_instr_gencounter++;
    vns_instr_stat = INSTR_GENERATED;
    vns_instr_log_firetick = 0;
    vns_instr_log_recycletick = 0;
    vns_instr_log_starttick = 0;
    vns_instr_log_endtick = 0;

    this->op            = op ;
    this->vew           = vew;
    this->func3         = func3;
    this->vl            = vl;
    this->vs1_head      = vs1_head;
    this->vs2_head      = vs2_head;
    this->vd1_head      = vd1_head;
    this->vd2_head      = vd2_head;
    this->vm_r          = (VMASK_READ)vm_r;

    this->scalar_op           = scalar_op          ;

    this->vfu_shamt           = 0;
    this->saturate_pre_adder  = 1;
    this->saturate_multiplier = 1;
    this->saturate_post_adder = 1;

    autocomplete();
  }
  VenusInstrPkt::VenusInstrPkt(VenusOp op, VEW vew, FUNC3 func3, int vl, int vs1_head, int vs2_head, int vd1_head, int vd2_head, bool vm_r, unsigned int scalar_op, unsigned char vfu_shamt, unsigned char saturate_pre_adder, unsigned char saturate_multiplier, unsigned char saturate_post_adder):Packet(std::make_shared<Request>(0, 0, 0, 0), MemCmd::ReadReq)
  {
    // vns_instr_id = VenusInstrPkt::vns_instr_gencounter++;
    vns_instr_stat = INSTR_GENERATED;
    vns_instr_log_firetick = 0;
    vns_instr_log_recycletick = 0;
    vns_instr_log_starttick = 0;
    vns_instr_log_endtick = 0;

    this->op            = op ;
    this->vew           = vew;
    this->func3         = func3;
    this->vl            = vl;
    this->vs1_head      = vs1_head;
    this->vs2_head      = vs2_head;
    this->vd1_head      = vd1_head;
    this->vd2_head      = vd2_head;
    this->vm_r          = (VMASK_READ)vm_r;

    this->scalar_op           = scalar_op          ;

    this->vfu_shamt           = vfu_shamt          ;
    this->saturate_pre_adder  = saturate_pre_adder ;
    this->saturate_multiplier = saturate_multiplier;
    this->saturate_post_adder = saturate_post_adder;

    autocomplete();
  }


  void VenusInstrPkt::autocomplete()
  {
    running_id = INT_MIN;

    vfu_lst.clear();
    operand_lst.clear();

    if (func3 == IVV)
    {
      use_vs1 = 1, use_vs2 = 1, use_vd1 = 1, use_vd2 = 0;
      use_vd1_op = 0;
      use_vd2_op = 0;
      use_scalar_op = 0;
      vm_w = 0;
    }
    else if (func3 == IVX)
    {
      use_vs1 = 0, use_vs2 = 1, use_vd1 = 1, use_vd2 = 0;
      use_vd1_op = 0;
      use_vd2_op = 0;
      use_scalar_op = 1;
      vm_w = 0;
    }
    else if (func3 == MVV)
    {
      use_vs1 = 1, use_vs2 = 1, use_vd1 = 0, use_vd2 = 0;
      use_vd1_op = 0;
      use_vd2_op = 0;
      use_scalar_op = 0;
      vm_w = 1;
    }
    else if (func3 == MVX)
    {
      use_vs1 = 0, use_vs2 = 1, use_vd1 = 0, use_vd2 = 0;
      use_vd1_op = 0;
      use_vd2_op = 0;
      use_scalar_op = 1;
      vm_w = 1;
    }
    else
    {
      use_vs1 = 0, use_vs2 = 0, use_vd1 = 0, use_vd2 = 0;
      use_vd1_op = 0;
      use_vd2_op = 0;
      use_scalar_op = 0;
      vm_w = 0;
    }
    if (op >= VREDAND && op <= VREDSUM)
    {
      use_vs1 = 0;
    }
    if (op >= VMULADD && op <= VSUBMUL)
    {
      use_vd1 = 1;
      use_vd2 = 0;
      use_vd1_op = 0;
      use_vd2_op = 1;
    }
    if (op == VCMXMUL)
    {
      use_vd1 = 1;
      use_vd2 = 1;
      use_vd1_op = 1;
      use_vd2_op = 1;
    }
    if (op == VRANGE)
    {
      // RTL venus_dispatcher.sv treats both encoded source fields as real
      // operand requests for VRANGE.  The CAU computes the range value
      // internally, but these reads still participate in RAW tracking and
      // VRF arbitration, so they must not be elided from the timing model.
      use_vs1 = 1;
      use_vs2 = 1;
      use_vd1 = 1;
      vm_r = 0;
      vew = EW16;
    }
    if (op == VBRDCST)
    {
      /*
       * venus_dispatcher.sv keeps the encoded vs2 range live for OPIVX
       * VBRDCST even though the generated data comes from scalar_op.  The
       * read range still participates in the sequencer read-list/RAW
       * lifetime and in the lane operand requester.
       */
      use_vs2 = 1;
    }
    if (op == VABS)
    {
      use_vs1 = 0;
    }
    if (op == VSTORE)
    {
      use_vd1 = 0;
    }
    if (op == VSHUFFLE_CLBMV)
    {
      use_vs2 = 0;
      vm_r = 0;
    }
    if (op == VMNOT)
    {
      vm_r = 0;
    }

    // venus_sequencer.sv relocates an in-place VSHUFFLE destination to the
    // reserved shuffle scratch region before issuing it.  With the gc0802
    // 128-row tile this region starts at row zero (Nrlines - 128).  Preserve
    // the original source rows, but make the effective destination seen by
    // the gem5 shuffle engine match RTL.
    if (op == VSHUFFLE &&
        (vd1_head == vs1_head || vd1_head == vs2_head))
    {
      vd1_head = NrLines - 128;
    }
    else if (op == VSHUFFLE_CLBMV)
    {
      vs1_head = NrLines - 128;
    }
    line_usage = rtlDispatcherVectorRows(vl, vew);

    while (1)
    {
      const int row_mask = NrLines - 1;
      // venus_req.{vs,vd}*_tail are vrow_t in RTL.  The unsized additions
      // in venus_dispatcher.sv are narrowed on assignment and therefore wrap
      // at the physical row count.
      vs1_tail = use_vs1 ? (vs1_head + line_usage - 1) & row_mask : vs1_head;
      vs2_tail = use_vs2 ? (vs2_head + line_usage - 1) & row_mask : 0;
      vd1_tail = (use_vd1 | use_vd1_op) ?
          (vd1_head + line_usage - 1) & row_mask : 0;
      vd2_tail = (use_vd2 | use_vd2_op) ?
          (vd2_head + line_usage - 1) & row_mask : 0;
      if ((op == VSHUFFLE || op == VSHUFFLE_CLBMV) && (vew == EW8))
      {
        vs2_tail = (vs2_head + ((((vl) * (1 + 1) - 1) /
            (NrLanes * NrBankPerLane * (NrBitsPerBank / 8))))) & row_mask;
      }
      // if (op >= VREDAND && op <= VREDSUM)
      // {
      //   vd1_tail = vd1_head;
      // }

      if (validate_venus_ext_instr() == true)
        break;
      else
        {display();panic("You provided an illegal instruction.");}
    }
  }

    // 显示函数，打印成员变量
  void VenusInstrPkt::display() const
  {
    std::cout << "vnsinstrID:" << vns_instr_id << ", RunningVID:" << running_id << ", op: " << op_to_str(op) << ", func3: " << ot_to_str(func3)
              << ", usethefollowingvfus: ";
    for (const auto& elem : vfu_lst) std::cout << vfu_to_str(elem) << " ";
    std::cout << std::dec <<", use_vs1: " << use_vs1 << ", use_vs2: " << use_vs2
              << ", use_vd2: " << use_vd2 << ", use_vd1: " << use_vd1
              << ", use_vd1_op: " << use_vd1_op << ", use_vd2_op: " << use_vd2_op
              << ", scalar_op: " << scalar_op << ", use_scalar_op: " << use_scalar_op
              << ", vm_r: " << vm_r << ", vm_w: " << vm_w
              << ", vl: " << vl
              << ", vew: " << vew_to_str(vew);
    std::cout << std::dec << ", vs1_head: " << vs1_head << ", vs1_tail: " << vs1_tail
              << ", vs2_head: " << vs2_head << ", vs2_tail: " << vs2_tail
              << ", vd1_head: " << vd1_head << ", vd1_tail: " << vd1_tail
              << ", vd2_head: " << vd2_head << ", vd2_tail: " << vd2_tail
              << ", vns_instr_stat: " << instr_stat_to_str(vns_instr_stat) << "\n";

    std::cout << std::dec << "else if(VenusInstrPkt::vns_instr_gencounter == " << vns_instr_id << ") this->venus_instr_pkt = new VenusInstrPkt(" << op_to_str(op) << ", " << vew_to_str(vew) << ", " << ot_to_str(func3)
              << ", " << vl << ", " << vs1_head << ", " << vs2_head << ", " << vd1_head << ", " << vd2_head << ", " << (vm_r==true?"true":"false") << ", " << scalar_op << ");\n";
  }

  int getRandomInt(int min, int max)
  {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(min, max);
    int tmp;
    tmp = dist(gen);
    return tmp;
  }

  int getRandomChoice(int range, int d0, int d1, int d2, int d3, int d4, int d5, int d6, int d7)
  {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, range - 1);
    int tmp;
    tmp = dist(gen);
    switch (tmp)
    {
    case 0:
      return d0;
    case 1:
      return d1;
    case 2:
      return d2;
    case 3:
      return d3;
    case 4:
      return d4;
    case 5:
      return d5;
    case 6:
      return d6;
    case 7:
      return d7;
    }
    return d0;
  }

  // 随机化设置值的函数
  void VenusInstrPkt::randomize()
  {
    running_id = INT_MIN;
    // op = static_cast<VenusOp>(getRandomInt(VAND, VREDSUM));
    // op = static_cast<VenusOp>(getRandomInt(VAND, VXOR));
    // op = static_cast<VenusOp>(getRandomInt(VBRDCST, VBRDCST));
    // op = static_cast<VenusOp>(getRandomInt(VSLL, VSRA));
    // op = static_cast<VenusOp>(getRandomInt(VSEQ, VSGT));
    // op = static_cast<VenusOp>(getRandomInt(VADD, VSADDU));
    // op = static_cast<VenusOp>(getRandomInt(VRANGE, VRANGE));
    // op = static_cast<VenusOp>(getRandomInt(VRSUB, VSSUBU));
    // op = static_cast<VenusOp>(getRandomInt(VMUL, VMULHSU));
    // op = static_cast<VenusOp>(getRandomInt(VMULADD, VSUBMUL));
    // op = static_cast<VenusOp>(getRandomInt(VCMXMUL, VCMXMUL));
    // op = static_cast<VenusOp>(getRandomInt(VDIV, VREMU));

    op = static_cast<VenusOp>(getRandomInt(VAND, VREDSUM));

    vew = static_cast<VEW>(getRandomChoice(2, EW8, EW16));
    vfu_lst.clear();
    operand_lst.clear();
    if (op >= VSEQ && op <= VSGT)
      func3 = static_cast<FUNC3>(getRandomChoice(4, IVV, IVX, MVV, MVX));
    else if (op == VBRDCST)
      func3 = static_cast<FUNC3>(getRandomChoice(1, IVX));
    else if (op == VLOAD || op == VSTORE)
      func3 = static_cast<FUNC3>(getRandomChoice(1, IVX));
    else if (op == VCMXMUL || op == VSHUFFLE)
      func3 = static_cast<FUNC3>(getRandomChoice(1, IVV));
    else if (op == VRANGE || op == VSHUFFLE_CLBMV || op == YIELD || op == VPRINTF || op == VMNOT)
      func3 = static_cast<FUNC3>(getRandomChoice(1, OPMISC));
    else
      func3 = static_cast<FUNC3>(getRandomChoice(2, IVV, IVX));

    if (func3 == IVV)
    {
      use_vs1 = 1, use_vs2 = 1, use_vd1 = 1, use_vd2 = 0;
      use_vd1_op = 0;
      use_vd2_op = 0;
      use_scalar_op = 0;
      vm_r = static_cast<VMASK_READ>(getRandomChoice(2, NORMAL_READ, READ_MASKED));
      vm_w = 0;
    }
    else if (func3 == IVX)
    {
      use_vs1 = 0, use_vs2 = 1, use_vd1 = 1, use_vd2 = 0;
      use_vd1_op = 0;
      use_vd2_op = 0;
      use_scalar_op = 1;
      vm_r = static_cast<VMASK_READ>(getRandomChoice(2, NORMAL_READ, READ_MASKED));
      vm_w = 0;
    }
    else if (func3 == MVV)
    {
      use_vs1 = 1, use_vs2 = 1, use_vd1 = 0, use_vd2 = 0;
      use_vd1_op = 0;
      use_vd2_op = 0;
      use_scalar_op = 0;
      vm_r = static_cast<VMASK_READ>(getRandomChoice(2, NORMAL_READ, READ_MASKED));
      vm_w = 1;
    }
    else if (func3 == MVX)
    {
      use_vs1 = 0, use_vs2 = 1, use_vd1 = 0, use_vd2 = 0;
      use_vd1_op = 0;
      use_vd2_op = 0;
      use_scalar_op = 1;
      vm_r = static_cast<VMASK_READ>(getRandomChoice(2, NORMAL_READ, READ_MASKED));
      vm_w = 1;
    }
    else
    {
      use_vs1 = 0, use_vs2 = 0, use_vd1 = 0, use_vd2 = 0;
      use_vd1_op = 0;
      use_vd2_op = 0;
      use_scalar_op = 0;
      vm_r = 0;
      vm_w = 0;
    }
    if (op >= VREDAND && op <= VREDSUM)
    {
      use_vs1 = 0;
    }
    if (op >= VMULADD && op <= VSUBMUL)
    {
      use_vd1 = 1;
      use_vd2 = 0;
      use_vd1_op = 0;
      use_vd2_op = 1;
    }
    if (op == VCMXMUL)
    {
      use_vd1 = 1;
      use_vd2 = 1;
      use_vd1_op = 1;
      use_vd2_op = 1;
    }
    if (op == VRANGE)
    {
      // Keep random/replay construction aligned with the RTL OPMISC decode.
      use_vs1 = 1;
      use_vs2 = 1;
      use_vd1 = 1;
      vm_r = 0;
      vew = EW16;
    }
    if (op == VBRDCST)
    {
      use_vs2 = 1;
    }
    if (op == VSHUFFLE_CLBMV)
    {
      use_vs2 = 0;
      vm_r = 0;
    }
    if (op == VMNOT)
    {
      vm_r = 0;
    }
    vl = getRandomInt(1,32767);
    vl = getRandomInt(1,8192);
    line_usage = rtlDispatcherVectorRows(vl, vew);

    scalar_op = use_scalar_op ? (vew == EW8 ? getRandomInt(0, 255) : getRandomInt(0, 65535)) : 0;
    while (1)
    {
      vs1_head = use_vs1 ? getRandomInt(0, NrLines - 1 - line_usage + 1) : getRandomInt(0, 31);
      vs1_tail = use_vs1 ? vs1_head + line_usage - 1 : vs1_head;
      vs2_head = use_vs2 ? getRandomInt(0, NrLines - 1 - line_usage + 1) : 0;
      vs2_tail = use_vs2 ? vs2_head + line_usage - 1 : 0;
      vd1_head = (use_vd1 | use_vd1_op) ? getRandomInt(0, NrLines - 1 - line_usage + 1) : 0;
      vd1_tail = (use_vd1 | use_vd1_op) ? vd1_head + line_usage - 1 : 0;
      vd2_head = (use_vd2 | use_vd2_op) ? getRandomInt(0, NrLines - 1 - line_usage + 1) : 0;
      vd2_tail = (use_vd2 | use_vd2_op) ? vd2_head + line_usage - 1 : 0;
      if ((op == VSHUFFLE || op == VSHUFFLE_CLBMV) && (vew == EW8))
      {
        vs2_tail = vs2_head + ((((vl) * (1 + 1) - 1) / (NrLanes * NrBankPerLane * (NrBitsPerBank / 8))));
      }
      // if (op >= VREDAND && op <= VREDSUM)
      // {
      //   vd1_tail = vd1_head;
      // }

      if (validate_venus_ext_instr() == true)
        break;
    }
  }

  bool VenusInstrPkt::validate_venus_ext_instr()
  {
    uint32_t use_vd1 = this->use_vd1 | this->use_vd1_op, use_vd2 = this->use_vd2 | this->use_vd2_op;

    // check if illegal vew
    if (this->op == VRANGE)
    {
      if (this->vew == EW8)
      {
        std::cerr << "validate_venus_ext_instr fail: VRANGE with EW8" << std::endl;
        return false;
      }
    }

    // check if illegal vmask read
    if (this->op == VRANGE || this->op == VMNOT || this->op == VSHUFFLE_CLBMV)
    {
      if (this->vm_r == READ_MASKED)
      {
        std::cerr << "validate_venus_ext_instr fail: VRANGE/VMNOT/VSHUFFLE_CLBMV with masked read" << std::endl;
        return false;
      }
    }

    // check if illegal funct3
    if (this->op == VBRDCST)
    {
      if (this->func3 != IVX)
      {
        std::cerr << "validate_venus_ext_instr fail: VBRDCST with non-IVX func3" << std::endl;
        return false;
      }
    }
    else if (this->op == VLOAD || this->op == VSTORE)
    {
      if (this->func3 != IVX)
      {
        std::cerr << "validate_venus_ext_instr fail: VLOAD/VSTORE with non-IVX func3" << std::endl;
        return false;
      }
    }
    else if (this->op == VCMXMUL || this->op == VSHUFFLE)
    {
      if (this->func3 != IVV)
      {
        std::cerr << "validate_venus_ext_instr fail: VCMXMUL/VSHUFFLE with non-IVV func3" << std::endl;
        return false;
      }
    }
    else if ((this->op == VAND) || (this->op == VOR) || (this->op == VXOR) || (this->op == VSLL) || (this->op == VSRL) || (this->op == VSRA) || (this->op == VABS) || (this->op == VADD) || (this->op == VSADD) || (this->op == VSADDU) || (this->op == VRSUB) || (this->op == VSUB) || (this->op == VSSUB) || (this->op == VSSUBU) || (this->op == VMUL) || (this->op == VMULH) || (this->op == VMULHU) || (this->op == VMULHSU) || (this->op == VMULADD) || (this->op == VMULSUB) || (this->op == VADDMUL) || (this->op == VSUBMUL) || (this->op == VDIV) || (this->op == VREM) || (this->op == VDIVU) || (this->op == VREMU) || (this->op == VMIN) || (this->op == VMAX) || (this->op == VREDAND) || (this->op == VREDOR) || (this->op == VREDXOR) || (this->op == VREDMAX) || (this->op == VREDMAXU) || (this->op == VREDMIN) || (this->op == VREDMINU) || (this->op == VREDSUM))
    {
      if (this->func3 != IVV && this->func3 != IVX)
      {
        std::cerr << "validate_venus_ext_instr fail: Op " << op_to_str(op) << " with non-IVV/IVX func3: " << (int)func3 << std::endl;
        return false;
      }
    }
    else if ((this->op == VSEQ) || (this->op == VSNE) || (this->op == VSLTU) || (this->op == VSLT) || (this->op == VSLEU) || (this->op == VSLE) || (this->op == VSGTU) || (this->op == VSGT))
    {
      if (this->func3 != IVV && this->func3 != IVX && this->func3 != MVV && this->func3 != MVX)
      {
        std::cerr << "validate_venus_ext_instr fail: Compare op with illegal func3" << std::endl;
        return false;
      }
    }
    else
    {
      if (this->func3 != OPMISC)
      {
        std::cerr << "validate_venus_ext_instr fail: Op " << op_to_str(op) << " with non-OPMISC func3: " << (int)func3 << std::endl;
        return false;
      }
    }

    // check out of venus row bound error
    if (vs1_tail >= NrLines)
    {
      std::cerr << "validate_venus_ext_instr fail: vs1_tail (" << vs1_tail << ") >= NrLines (" << NrLines << ")" << std::endl;
      return false;
    }
    if (vs2_tail >= NrLines)
    {
      std::cerr << "validate_venus_ext_instr fail: vs2_tail (" << vs2_tail << ") >= NrLines (" << NrLines << ")" << std::endl;
      return false;
    }
    if (vd1_tail >= NrLines)
    {
      std::cerr << "validate_venus_ext_instr fail: vd1_tail (" << vd1_tail << ") >= NrLines (" << NrLines << ")" << std::endl;
      return false;
    }
    if (vd2_tail >= NrLines)
    {
      std::cerr << "validate_venus_ext_instr fail: vd2_tail (" << vd2_tail << ") >= NrLines (" << NrLines << ")" << std::endl;
      return false;
    }
    // check operand row occupation collision error
    if ((this->op == VSHUFFLE || this->op == VSHUFFLE_CLBMV) &&
        std::getenv("VENUS_GEM5_STRICT_OPERAND_OVERLAP"))
    {
      if ((vs1_tail >= vs2_head) && (vs1_tail <= vs2_tail) && (use_vs1 == 1) && (use_vs2 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vs1 collide with vs2" << ", vs1_head = " << vs1_head << ", vs1_tail = " << vs1_tail << ", vs2_head = " << vs2_head << ", vs2_tail = " << vs2_tail << ", " << std::endl;
        return false;
      }
      if ((vs1_tail >= vd1_head) && (vs1_tail <= vd1_tail) && (use_vs1 == 1) && (use_vd1 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vs1 collide with vd1" << ", vs1_head = " << vs1_head << ", vs1_tail = " << vs1_tail << ", vd1_head = " << vd1_head << ", vd1_tail = " << vd1_tail << ", " << std::endl;
        return false;
      }
      if ((vs1_tail >= vd2_head) && (vs1_tail <= vd2_tail) && (use_vs1 == 1) && (use_vd2 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vs1 collide with vd2" << ", vs1_head = " << vs1_head << ", vs1_tail = " << vs1_tail << ", vd2_head = " << vd2_head << ", vd2_tail = " << vd2_tail << ", " << std::endl;
        return false;
      }
      if ((vs2_tail >= vs1_head) && (vs2_tail <= vs1_tail) && (use_vs2 == 1) && (use_vs1 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vs2 collide with vs1" << ", vs2_head = " << vs2_head << ", vs2_tail = " << vs2_tail << ", vs1_head = " << vs1_head << ", vs1_tail = " << vs1_tail << ", " << std::endl;
        return false;
      }
      if ((vs2_tail >= vd1_head) && (vs2_tail <= vd1_tail) && (use_vs2 == 1) && (use_vd1 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vs2 collide with vd1" << ", vs2_head = " << vs2_head << ", vs2_tail = " << vs2_tail << ", vd1_head = " << vd1_head << ", vd1_tail = " << vd1_tail << ", " << std::endl;
        return false;
      }
      if ((vs2_tail >= vd2_head) && (vs2_tail <= vd2_tail) && (use_vs2 == 1) && (use_vd2 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vs2 collide with vd2" << ", vs2_head = " << vs2_head << ", vs2_tail = " << vs2_tail << ", vd2_head = " << vd2_head << ", vd2_tail = " << vd2_tail << ", " << std::endl;
        return false;
      }
      if ((vd1_tail >= vs1_head) && (vd1_tail <= vs1_tail) && (use_vd1 == 1) && (use_vs1 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vd1 collide with vs1" << ", vd1_head = " << vd1_head << ", vd1_tail = " << vd1_tail << ", vs1_head = " << vs1_head << ", vs1_tail = " << vs1_tail << ", " << std::endl;
        return false;
      }
      if ((vd1_tail >= vs2_head) && (vd1_tail <= vs2_tail) && (use_vd1 == 1) && (use_vs2 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vd1 collide with vs2" << ", vd1_head = " << vd1_head << ", vd1_tail = " << vd1_tail << ", vs2_head = " << vs2_head << ", vs2_tail = " << vs2_tail << ", " << std::endl;
        return false;
      }
      if ((vd1_tail >= vd2_head) && (vd1_tail <= vd2_tail) && (use_vd1 == 1) && (use_vd2 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vd1 collide with vd2" << ", vd1_head = " << vd1_head << ", vd1_tail = " << vd1_tail << ", vd2_head = " << vd2_head << ", vd2_tail = " << vd2_tail << ", " << std::endl;
        return false;
      }
      if ((vd2_tail >= vs1_head) && (vd2_tail <= vs1_tail) && (use_vd2 == 1) && (use_vs1 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vd2 collide with vs1" << ", vd2_head = " << vd2_head << ", vd2_tail = " << vd2_tail << ", vs1_head = " << vs1_head << ", vs1_tail = " << vs1_tail << ", " << std::endl;
        return false;
      }
      if ((vd2_tail >= vs2_head) && (vd2_tail <= vs2_tail) && (use_vd2 == 1) && (use_vs2 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vd2 collide with vs2" << ", vd2_head = " << vd2_head << ", vd2_tail = " << vd2_tail << ", vs2_head = " << vs2_head << ", vs2_tail = " << vs2_tail << ", " << std::endl;
        return false;
      }
      if ((vd2_tail >= vd1_head) && (vd2_tail <= vd1_tail) && (use_vd2 == 1) && (use_vd1 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vd2 collide with vd1" << ", vd2_head = " << vd2_head << ", vd2_tail = " << vd2_tail << ", vd1_head = " << vd1_head << ", vd1_tail = " << vd1_tail << ", " << std::endl;
        return false;
      }
    }
    else if (std::getenv("VENUS_GEM5_STRICT_OPERAND_OVERLAP"))
    {
      if ((vs1_tail >= vs2_head) && (vs1_tail <= vs2_tail) && (!((vs1_head == vs2_head) && (vs1_tail == vs2_tail))) && (use_vs1 == 1) && (use_vs2 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vs1 collide with vs2" << ", vs1_head = " << vs1_head << ", vs1_tail = " << vs1_tail << ", vs2_head = " << vs2_head << ", vs2_tail = " << vs2_tail << ", " << std::endl;
        return false;
      }
      if ((vs1_tail >= vd1_head) && (vs1_tail <= vd1_tail) && (!((vs1_head == vd1_head) && (vs1_tail == vd1_tail))) && (use_vs1 == 1) && (use_vd1 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vs1 collide with vd1" << ", vs1_head = " << vs1_head << ", vs1_tail = " << vs1_tail << ", vd1_head = " << vd1_head << ", vd1_tail = " << vd1_tail << ", " << std::endl;
        return false;
      }
      if ((vs1_tail >= vd2_head) && (vs1_tail <= vd2_tail) && (!((vs1_head == vd2_head) && (vs1_tail == vd2_tail))) && (use_vs1 == 1) && (use_vd2 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vs1 collide with vd2" << ", vs1_head = " << vs1_head << ", vs1_tail = " << vs1_tail << ", vd2_head = " << vd2_head << ", vd2_tail = " << vd2_tail << ", " << std::endl;
        return false;
      }
      if ((vs2_tail >= vs1_head) && (vs2_tail <= vs1_tail) && (!((vs2_head == vs1_head) && (vs2_tail == vs1_tail))) && (use_vs2 == 1) && (use_vs1 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vs2 collide with vs1" << ", vs2_head = " << vs2_head << ", vs2_tail = " << vs2_tail << ", vs1_head = " << vs1_head << ", vs1_tail = " << vs1_tail << ", " << std::endl;
        return false;
      }
      if ((vs2_tail >= vd1_head) && (vs2_tail <= vd1_tail) && (!((vs2_head == vd1_head) && (vs2_tail == vd1_tail))) && (use_vs2 == 1) && (use_vd1 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vs2 collide with vd1" << ", vs2_head = " << vs2_head << ", vs2_tail = " << vs2_tail << ", vd1_head = " << vd1_head << ", vd1_tail = " << vd1_tail << ", " << std::endl;
        return false;
      }
      if ((vs2_tail >= vd2_head) && (vs2_tail <= vd2_tail) && (!((vs2_head == vd2_head) && (vs2_tail == vd2_tail))) && (use_vs2 == 1) && (use_vd2 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vs2 collide with vd2" << ", vs2_head = " << vs2_head << ", vs2_tail = " << vs2_tail << ", vd2_head = " << vd2_head << ", vd2_tail = " << vd2_tail << ", " << std::endl;
        return false;
      }
      if ((vd1_tail >= vs1_head) && (vd1_tail <= vs1_tail) && (!((vd1_head == vs1_head) && (vd1_tail == vs1_tail))) && (use_vd1 == 1) && (use_vs1 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vd1 collide with vs1" << ", vd1_head = " << vd1_head << ", vd1_tail = " << vd1_tail << ", vs1_head = " << vs1_head << ", vs1_tail = " << vs1_tail << ", " << std::endl;
        return false;
      }
      if ((vd1_tail >= vs2_head) && (vd1_tail <= vs2_tail) && (!((vd1_head == vs2_head) && (vd1_tail == vs2_tail))) && (use_vd1 == 1) && (use_vs2 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vd1 collide with vs2" << ", vd1_head = " << vd1_head << ", vd1_tail = " << vd1_tail << ", vs2_head = " << vs2_head << ", vs2_tail = " << vs2_tail << ", " << std::endl;
        return false;
      }
      if ((vd1_tail >= vd2_head) && (vd1_tail <= vd2_tail) && (use_vd1 == 1) && (use_vd2 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vd1 collide with vd2" << ", vd1_head = " << vd1_head << ", vd1_tail = " << vd1_tail << ", vd2_head = " << vd2_head << ", vd2_tail = " << vd2_tail << ", " << std::endl;
        return false;
      }
      if ((vd2_tail >= vs1_head) && (vd2_tail <= vs1_tail) && (!((vd2_head == vs1_head) && (vd2_tail == vs1_tail))) && (use_vd2 == 1) && (use_vs1 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vd2 collide with vs1" << ", vd2_head = " << vd2_head << ", vd2_tail = " << vd2_tail << ", vs1_head = " << vs1_head << ", vs1_tail = " << vs1_tail << ", " << std::endl;
        return false;
      }
      if ((vd2_tail >= vs2_head) && (vd2_tail <= vs2_tail) && (!((vd2_head == vs2_head) && (vd2_tail == vs2_tail))) && (use_vd2 == 1) && (use_vs2 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vd2 collide with vs2" << ", vd2_head = " << vd2_head << ", vd2_tail = " << vd2_tail << ", vs2_head = " << vs2_head << ", vs2_tail = " << vs2_tail << ", " << std::endl;
        return false;
      }
      if ((vd2_tail >= vd1_head) && (vd2_tail <= vd1_tail) && (use_vd2 == 1) && (use_vd1 == 1))
      { // std::cerr << "Runtime Error: Illegal instr, this instruction's vd2 collide with vd1" << ", vd2_head = " << vd2_head << ", vd2_tail = " << vd2_tail << ", vd1_head = " << vd1_head << ", vd1_tail = " << vd1_tail << ", " << std::endl;
        return false;
      }
    }
    return true;
  }

  void VenusInstrPkt::dumpVinsResult(memory::venus_vrf_mem *m_venus_vrf)
  {
    dumpVinsResult(m_venus_vrf, this->vns_instr_id);
  }

  void VenusInstrPkt::dumpVinsResult(memory::venus_vrf_mem *m_venus_vrf,
                                     unsigned int dump_id,
                                     const std::string &dump_dir)
  {
    std::filesystem::create_directories(dump_dir);
    if (std::getenv("VENUS_GEM5_DEBUG_VRF_DUMP") != nullptr &&
        !this->vm_w) {
        std::filesystem::create_directories("Debug/venusgem5_debug");
        const int debug_head = (this->op == VSTORE) ? this->vs2_head : this->vd1_head;
        std::string debug_filename = "Debug/venusgem5_debug/" +
            op_to_str(this->op) + "_" + std::to_string(dump_id) + "_vrf.txt";
        std::ofstream debugFile(debug_filename);
        debugFile << m_venus_vrf->backdoor_Read_Line(debug_head, this->vl, this->vew);
        debugFile.close();
    }

    if(this->op == VCMXMUL) {
        {std::string dump_filename = dump_dir + "/" + op_to_str(this->op) + "_" + std::to_string(dump_id) + "_vd1" + ".txt";
        std::ofstream dumpFile(dump_filename);
        dumpFile << (result_data.empty() ? m_venus_vrf->backdoor_Read_Line(this->vd1_head, this->vl, this->vew) : capturedResultString(false));
        dumpFile.close();}
        {std::string dump_filename = dump_dir + "/" + op_to_str(this->op) + "_" + std::to_string(dump_id) + "_vd2" + ".txt";
        std::ofstream dumpFile(dump_filename);
        dumpFile << (result_data_vd2.empty() ? m_venus_vrf->backdoor_Read_Line(this->vd2_head, this->vl, this->vew) : capturedResultString(true));
        dumpFile.close();}
    } else if (this->vm_w) {
        std::string dump_filename = dump_dir + "/" + op_to_str(this->op) + "mask" + "_" + std::to_string(dump_id) + ".txt";
        std::ofstream dumpFile(dump_filename);
        dumpFile << m_venus_vrf->backdoor_Read_Mask(this->vl, this->vew);
        dumpFile.close();
    } else if (this->op == VSTORE) {
        std::string dump_filename = dump_dir + "/" + op_to_str(this->op) + "_" + std::to_string(dump_id) + ".txt";
        std::ofstream dumpFile(dump_filename);
        dumpFile << m_venus_vrf->backdoor_Read_Line(this->vs2_head, this->vl, this->vew);
        dumpFile.close();
    } else if (this->op == VLOAD) {
        // LDU writes the banked VRF directly. Its packet result buffer is
        // not lane writeback data and therefore has no valid-bit capture.
        std::string dump_filename = dump_dir + "/" + op_to_str(this->op) +
            "_" + std::to_string(dump_id) + ".txt";
        std::ofstream dumpFile(dump_filename);
        dumpFile << m_venus_vrf->backdoor_Read_Line(
            this->vd1_head, this->vl, this->vew);
        dumpFile.close();
    } else if (this->op != VSHUFFLE && !result_data.empty()) {
        /*
         * Use the tagged completion payload rather than sampling the shared
         * VRF at global retirement.  A younger WAW instruction may already
         * have received a bank grant by then.  VBRDCST used to be excluded
         * here and compensated by a retirement-time backdoor VRF rewrite;
         * that duplicate writer both produced a misleading snapshot and
         * could overwrite the younger instruction.  The ordinary BitALU
         * result path records every broadcast write, so it has the same
         * single-owner completion snapshot as the other arithmetic ops.
         */
        std::string dump_filename = dump_dir + "/" + op_to_str(this->op) + "_" + std::to_string(dump_id) + ".txt";
        std::ofstream dumpFile(dump_filename);
        dumpFile << capturedResultString(false);
        dumpFile.close();
    } else {
        std::string dump_filename = dump_dir + "/" + op_to_str(this->op) + "_" + std::to_string(dump_id) + ".txt";
        std::ofstream dumpFile(dump_filename);
        dumpFile << m_venus_vrf->backdoor_Read_Line(this->vd1_head, this->vl, this->vew);
        dumpFile.close();
    }
  }

  int VenusInstrPkt::resultSlotCount() const
  {
    return (this->vew == EW8) ? ((this->vl + 1) / 2) : this->vl;
  }

  int VenusInstrPkt::resultDumpVl() const
  {
    if (this->op == VREDAND || this->op == VREDOR || this->op == VREDXOR ||
        this->op == VREDMAX || this->op == VREDMAXU ||
        this->op == VREDMIN || this->op == VREDMINU)
        return 1;
    if (this->op == VREDSUM)
        return 2;
    return this->vl;
  }

  void VenusInstrPkt::ensureResultData()
  {
    const int slots = resultSlotCount();
    if (result_data.size() != static_cast<size_t>(slots)) {
        result_data.assign(slots, 0);
        result_valid.assign(slots, 0);
    }
    if ((this->use_vd2 || this->use_vd2_op) && this->op == VCMXMUL &&
        result_data_vd2.size() != static_cast<size_t>(slots)) {
        result_data_vd2.assign(slots, 0);
        result_valid_vd2.assign(slots, 0);
    }
  }

  void VenusInstrPkt::recordResultWrite(int lane_id, bool is_vd2,
                                        int offset_bytes,
                                        uint16_t writeback_data,
                                        unsigned int size)
  {
    ensureResultData();

    const int row = offset_bytes / (2 * NrBankPerLane);
    const int bank = (offset_bytes / 2) % NrBankPerLane;
    const int slot = lane_id * NrBankPerLane + row * NrLanes * NrBankPerLane + bank;
    std::vector<uint16_t> &data = is_vd2 ? result_data_vd2 : result_data;
    std::vector<uint8_t> &valid = is_vd2 ? result_valid_vd2 : result_valid;
    if (slot < 0 || slot >= static_cast<int>(data.size()))
        return;

    if (size == 1) {
        if (offset_bytes & 0x1) {
            data[slot] = (data[slot] & 0x00ff) | (writeback_data & 0xff00);
            valid[slot] |= 0x2;
        } else {
            data[slot] = (data[slot] & 0xff00) | (writeback_data & 0x00ff);
            valid[slot] |= 0x1;
        }
    } else {
        data[slot] = writeback_data;
        valid[slot] |= 0x3;
    }
  }

  void VenusInstrPkt::mergeResultDataFrom(const VenusInstrPkt *pkt)
  {
    if (pkt == nullptr)
        return;
    ensureResultData();
    for (size_t i = 0; i < result_data.size() && i < pkt->result_data.size() && i < pkt->result_valid.size(); ++i) {
        if (pkt->result_valid[i] & 0x1)
            result_data[i] = (result_data[i] & 0xff00) | (pkt->result_data[i] & 0x00ff);
        if (pkt->result_valid[i] & 0x2)
            result_data[i] = (result_data[i] & 0x00ff) | (pkt->result_data[i] & 0xff00);
        result_valid[i] |= pkt->result_valid[i];
    }
    for (size_t i = 0; i < result_data_vd2.size() && i < pkt->result_data_vd2.size() && i < pkt->result_valid_vd2.size(); ++i) {
        if (pkt->result_valid_vd2[i] & 0x1)
            result_data_vd2[i] = (result_data_vd2[i] & 0xff00) | (pkt->result_data_vd2[i] & 0x00ff);
        if (pkt->result_valid_vd2[i] & 0x2)
            result_data_vd2[i] = (result_data_vd2[i] & 0x00ff) | (pkt->result_data_vd2[i] & 0xff00);
        result_valid_vd2[i] |= pkt->result_valid_vd2[i];
    }
  }

  std::string VenusInstrPkt::capturedResultString(bool is_vd2) const
  {
    const std::vector<uint16_t> &data = is_vd2 ? result_data_vd2 : result_data;
    const std::vector<uint8_t> &valid = is_vd2 ? result_valid_vd2 : result_valid;
    std::stringstream ss;
    const int dump_vl = resultDumpVl();
    for (int j = 0; j < dump_vl; ++j) {
        const int slot = (this->vew == EW8) ? (j / 2) : j;
        if (slot < 0 || slot >= static_cast<int>(data.size())) {
            ss << "X\n";
            continue;
        }
        if (this->vew == EW8) {
            const uint8_t mask = (j & 0x1) ? 0x2 : 0x1;
            if ((valid[slot] & mask) == 0) {
                ss << "X\n";
            } else if (j & 0x1) {
                ss << static_cast<uint32_t>((data[slot] & 0xff00) >> 8) << "\n";
            } else {
                ss << static_cast<uint32_t>(data[slot] & 0x00ff) << "\n";
            }
        } else {
            if ((valid[slot] & 0x3) != 0x3)
                ss << "X\n";
            else
                ss << static_cast<uint32_t>(data[slot] & 0xffff) << "\n";
        }
    }
    return ss.str();
  }

  using json = nlohmann::json;
  json j_array;
  void VenusInstrPkt::createVinsInfo()
  {
    if (!venusVinsMonitorEnabled())
      return;
    const auto filename = venusDebugRoot() /
                          "venusgem5_sequencer_monitor.json";
    std::filesystem::remove(filename);
    if (!std::filesystem::exists(filename)) {
      j_array = json::array();
      json initial_obj = {{"Venus_instr", json::array()}};
      j_array.push_back(initial_obj);
      std::filesystem::create_directories(std::filesystem::path(filename).parent_path());
    } else {
      std::ifstream file(filename);
      file >> j_array;
      file.close();
    }
  }
  void VenusInstrPkt::saveVinsInfo()
  {
    if (!venusVinsMonitorEnabled())
      return;
    const auto filename = venusDebugRoot() /
                          "venusgem5_sequencer_monitor.json";
    std::ofstream file(filename);
    file << j_array.dump(1);
    file.close();
  }
  void VenusInstrPkt::dumpVinsInfo()
  {
    if (!venusVinsMonitorEnabled())
      return;
    json& venus_instr_array = j_array[0]["Venus_instr"];

    venus_instr_array.push_back({ {"venus_instr_counter", vns_instr_id},
                                  {"id", running_id},
                                  {"op", (int)op},
                                  {"op_s", op_to_str(op)},
                                  {"vl", vl},
                                  {"vew", (int)vew},
                                  {"vew_s", vew_to_str(vew)},
                                  {"vfu", vfu_lst.front()},
                                  {"vfu_s", vfu_to_str(vfu_lst.front())},
                                  {"func3", (int)func3},
                                  {"func3_s", ot_to_str(func3)},
                                  {"use_vs1", (int)use_vs1},
                                  {"vs1_head", (int)vs1_head},
                                  {"vs1_tail", (int)vs1_tail},
                                  {"use_vs2", (int)use_vs2},
                                  {"vs2_head", (int)vs2_head},
                                  {"vs2_tail", (int)vs2_tail},
                                  {"use_vd1", (int)use_vd1},
                                  {"vd1_head", (int)vd1_head},
                                  {"vd1_tail", (int)vd1_tail},
                                  {"use_vd2", (int)use_vd2},
                                  {"vd2_head", (int)vd2_head},
                                  {"vd2_tail", (int)vd2_tail},
                                  {"use_scalar_op", (int)use_scalar_op},
                                  {"scalar_op", scalar_op},
                                  {"vm_r", (int)vm_r},
                                  {"vm_w", (int)vm_w},
                                  {"fire_tick", vns_instr_log_firetick},
                                  {"recycle_tick", vns_instr_log_recycletick},
                                  {"consumed_ticks", vns_instr_log_recycletick - vns_instr_log_firetick},
                                  {"actual_start_tick", vns_instr_log_starttick},
                                  {"actual_end_tick", vns_instr_log_endtick},
                                  {"actual_processing_ticks", vns_instr_log_endtick - vns_instr_log_starttick}
                                });

  }
}
