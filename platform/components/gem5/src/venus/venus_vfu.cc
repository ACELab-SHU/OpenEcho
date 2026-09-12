#include "venus_vfu.hh"

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace gem5
{
void venus_vfu::doVenusVFU(VFU vfu, VenusInstrPkt* instr_pkt, unsigned int *vd1_w, unsigned int *vd2_w, unsigned int *vd1_r, unsigned int *vd2_r, unsigned int *vmask_w, unsigned int *vs1, unsigned int *vs2, unsigned int *vmask_r) {

    this->MY_VENUS_INS_PARAM.vmask_read =           instr_pkt->vm_r;
    this->MY_VENUS_INS_PARAM.vmask_write =          instr_pkt->vm_w;
    this->MY_VENUS_INS_PARAM.vew =                  instr_pkt->vew;
    this->MY_VENUS_INS_PARAM.function3 =            instr_pkt->func3;
    // this->MY_VENUS_INS_PARAM.op_code =              instr_pkt->;
    // this->MY_VENUS_INS_PARAM.function5 =            instr_pkt->;
    // this->MY_VENUS_INS_PARAM.vd1_head =             instr_pkt->;
    // this->MY_VENUS_INS_PARAM.vd2_head =             instr_pkt->;
    // this->MY_VENUS_INS_PARAM.vs1_head =             instr_pkt->;
    // this->MY_VENUS_INS_PARAM.vs2_head =             instr_pkt->;
    this->MY_VENUS_INS_PARAM.scalar_op =            instr_pkt->scalar_op;
    this->MY_VENUS_INS_PARAM.op =                   instr_pkt->op;
    // this->MY_VENUS_INS_PARAM.vd1_msb =              instr_pkt->;
    // this->MY_VENUS_INS_PARAM.vs1_msb =              instr_pkt->;
    // this->MY_VENUS_INS_PARAM.vs2_msb =              instr_pkt->;
    this->MY_VENUS_INS_PARAM.vfu_shamt =            instr_pkt->vfu_shamt;
    // this->MY_VENUS_INS_PARAM.high_vd2_bits =        instr_pkt->;
    // this->MY_VENUS_INS_PARAM.high_vd1_bits =        instr_pkt->;
    // this->MY_VENUS_INS_PARAM.high_vs2_bits =        instr_pkt->;
    // this->MY_VENUS_INS_PARAM.high_vs1_bits =        instr_pkt->;
    this->MY_VENUS_INS_PARAM.saturate_pre_adder =   instr_pkt->saturate_pre_adder;
    this->MY_VENUS_INS_PARAM.saturate_multiplier =  instr_pkt->saturate_multiplier;
    this->MY_VENUS_INS_PARAM.saturate_post_adder =  instr_pkt->saturate_post_adder;

    if(vd1_r   != nullptr) this->data_vd1     = *vd1_r  ; else this->data_vd1     = 0;
    if(vd2_r   != nullptr) this->data_vd2     = *vd2_r  ; else this->data_vd2     = 0;
    if(vs1     != nullptr) this->data_vs1     = *vs1    ; else this->data_vs1     = 0;
    if(vs2     != nullptr) this->data_vs2     = *vs2    ; else this->data_vs2     = 0;
    if(vmask_r != nullptr) this->data_vmask_r = *vmask_r; else this->data_vmask_r = 0;
    this->data_vmask_w = 0;

    const char *trace_id_env =
        std::getenv("VENUS_GEM5_DEBUG_VFU_TRACE_ID");
    const bool trace_vfu =
        std::getenv("VENUS_GEM5_DEBUG_VFU_TRACE") != nullptr &&
        (trace_id_env == nullptr ||
         instr_pkt->vns_instr_id == std::strtoul(trace_id_env, nullptr, 0));
    unsigned int trace_in_vd1 = this->data_vd1;
    unsigned int trace_in_vs1 = this->data_vs1;
    unsigned int trace_in_vs2 = this->data_vs2;
    unsigned int trace_in_mask = this->data_vmask_r;

    if(this->MY_VENUS_INS_PARAM.vew == EW8)
    {
        data_vmask_r_element = (*vmask_r) & 0x1;
        DPRINTF(LaneVFUFull,"%s%s%s%s%s%s%s%s%s%s%s%d\n"
              , (instr_pkt->use_scalar_op?"   this->scalar_op = 0x":""), (instr_pkt->use_scalar_op?int2Hex(this->MY_VENUS_INS_PARAM.scalar_op&0x00ff):"")
              , (instr_pkt->use_vs1      ?"    this->data_vs1 = 0x":""), (instr_pkt->use_vs1      ?int2Hex(this->data_vs1&0x00ff)                    :"")
              , (instr_pkt->use_vs2      ?"    this->data_vs2 = 0x":""), (instr_pkt->use_vs2      ?int2Hex(this->data_vs2&0x00ff)                    :"")
              , (instr_pkt->use_vd1_op   ?"    this->data_vd1 = 0x":""), (instr_pkt->use_vd1_op   ?int2Hex(this->data_vd1&0x00ff)                    :"")
              , (instr_pkt->use_vd2_op   ?"    this->data_vd2 = 0x":""), (instr_pkt->use_vd2_op   ?int2Hex(this->data_vd2&0x00ff)                    :"")
              , ", mask is ", (data_vmask_r_element|(!instr_pkt->vm_r)));
        doVEMU(instr_pkt, 0x00ff, 0x00ff, 0x00ff, 0x00ff);
              if(this->MY_VENUS_INS_PARAM.op == VREDAND) { DPRINTF(LaneVFUFull,"and_value = %x \n", and_value); }
        else if(this->MY_VENUS_INS_PARAM.op == VREDOR) { DPRINTF(LaneVFUFull,"or_value = %x \n", or_value); }
        else if(this->MY_VENUS_INS_PARAM.op == VREDXOR) { DPRINTF(LaneVFUFull,"xor_value = %x \n", xor_value); }
        else if(this->MY_VENUS_INS_PARAM.op == VREDMIN) { DPRINTF(LaneVFUFull,"min_value = %d \n", min_value); }
        else if(this->MY_VENUS_INS_PARAM.op == VREDMINU) { DPRINTF(LaneVFUFull,"min_uvalue = %d \n", min_uvalue); }
        else if(this->MY_VENUS_INS_PARAM.op == VREDMAX) { DPRINTF(LaneVFUFull,"max_value = %d \n", max_value); }
        else if(this->MY_VENUS_INS_PARAM.op == VREDMAXU) { DPRINTF(LaneVFUFull,"max_uvalue = %d \n", max_uvalue); }
        else if(this->MY_VENUS_INS_PARAM.op == VREDSUM) { DPRINTF(LaneVFUFull,"sum_result = %d\n", sum_result); }
        else {
        DPRINTF(LaneVFUFull,"%s%s%s%s%s%s%s%d\n"
              , (instr_pkt->use_vd1      ?"    this->data_vd1 = 0x"    :""), (instr_pkt->use_vd1      ?int2Hex(this->data_vd1&0x00ff)    :"")
              , (instr_pkt->use_vd2      ?"    this->data_vd2 = 0x"    :""), (instr_pkt->use_vd2      ?int2Hex(this->data_vd2&0x00ff)    :"")
              , (instr_pkt->vm_w         ?"    this->data_vmask_w = 0x":""), (instr_pkt->vm_w         ?int2Hex(this->data_vmask_w&0x00ff):"")
              , ", mask is ", (data_vmask_r_element|(!instr_pkt->vm_r)));
        }
        if((((instr_pkt->vl)%(NrLanes*NrBankPerLane*NrBytesPerBank))/(NrBankPerLane*NrBytesPerBank) != localLaneID) ||
           (((vfu==VFU_BitALU)&&(instr_pkt->locallane_bitalu_calc_cnt <= (instr_pkt->locallane_bitalu_calctime_len)))||
            ((vfu==VFU_CAU)   &&(instr_pkt->locallane_cau_calc_cnt    <= (instr_pkt->locallane_cau_calctime_len)))||
            ((vfu==VFU_SerDiv)&&(instr_pkt->locallane_serdiv_calc_cnt <= (instr_pkt->locallane_serdiv_calctime_len)))))
        {
          data_vmask_r_element = (((*vmask_r) & 0x100)>>8);
          DPRINTF(LaneVFUFull,"%s%s%s%s%s%s%s%s%s%s%s%d\n"
              , (instr_pkt->use_scalar_op?"   this->scalar_op = 0x":""), (instr_pkt->use_scalar_op?int2Hex(this->MY_VENUS_INS_PARAM.scalar_op&0x00ff):"")
              , (instr_pkt->use_vs1      ?"    this->data_vs1 = 0x":""), (instr_pkt->use_vs1      ?int2Hex((this->data_vs1&0xff00)>>8)               :"")
              , (instr_pkt->use_vs2      ?"    this->data_vs2 = 0x":""), (instr_pkt->use_vs2      ?int2Hex((this->data_vs2&0xff00)>>8)               :"")
              , (instr_pkt->use_vd1_op   ?"    this->data_vd1 = 0x":""), (instr_pkt->use_vd1_op   ?int2Hex((this->data_vd1&0xff00)>>8)               :"")
              , (instr_pkt->use_vd2_op   ?"    this->data_vd2 = 0x":""), (instr_pkt->use_vd2_op   ?int2Hex((this->data_vd2&0xff00)>>8)               :"")
              , ", mask is ", (data_vmask_r_element|(!instr_pkt->vm_r)));
          doVEMU(instr_pkt, 0xff00, 0xff00, 0xff00, 0xff00);
                if(this->MY_VENUS_INS_PARAM.op == VREDAND) { DPRINTF(LaneVFUFull,"and_value = %x \n", and_value); }
          else if(this->MY_VENUS_INS_PARAM.op == VREDOR) { DPRINTF(LaneVFUFull,"or_value = %x \n", or_value); }
          else if(this->MY_VENUS_INS_PARAM.op == VREDXOR) { DPRINTF(LaneVFUFull,"xor_value = %x \n", xor_value); }
          else if(this->MY_VENUS_INS_PARAM.op == VREDMIN) { DPRINTF(LaneVFUFull,"min_value = %d \n", min_value); }
          else if(this->MY_VENUS_INS_PARAM.op == VREDMINU) { DPRINTF(LaneVFUFull,"min_uvalue = %d \n", min_uvalue); }
          else if(this->MY_VENUS_INS_PARAM.op == VREDMAX) { DPRINTF(LaneVFUFull,"max_value = %d \n", max_value); }
          else if(this->MY_VENUS_INS_PARAM.op == VREDMAXU) { DPRINTF(LaneVFUFull,"max_uvalue = %d \n", max_uvalue); }
          else if(this->MY_VENUS_INS_PARAM.op == VREDSUM) { DPRINTF(LaneVFUFull,"sum_result = %d\n", sum_result); }
          else {
                DPRINTF(LaneVFUFull,"%s%s%s%s%s%s%s%d\n"
                    , (instr_pkt->use_vd1      ?"    this->data_vd1 = 0x"    :""), (instr_pkt->use_vd1      ?int2Hex((this->data_vd1&0xff00)>>8)    :"")
                    , (instr_pkt->use_vd2      ?"    this->data_vd2 = 0x"    :""), (instr_pkt->use_vd2      ?int2Hex((this->data_vd2&0xff00)>>8)    :"")
                    , (instr_pkt->vm_w         ?"    this->data_vmask_w = 0x":""), (instr_pkt->vm_w         ?int2Hex((this->data_vmask_w&0xff00)>>8):"")
                    , ", mask is ", (data_vmask_r_element|(!instr_pkt->vm_r)));
          }
        } else {
          // std::cout<<"AAAAA: EW8 DONOT CALC, "<<"lastlane is "<<((instr_pkt->vl-1)%(NrLanes*NrBankPerLane*NrBytesPerBank))/(NrBankPerLane*NrBytesPerBank)<<", localLaneID is "<<localLaneID<<", "
          //                                     <<"instr_pkt->locallane_bitalu_calc_cnt <= (instr_pkt->locallane_bitalu_calctime_len)"<<  instr_pkt->locallane_bitalu_calc_cnt <<" < "<<(instr_pkt->locallane_bitalu_calctime_len)
          //                                     <<", instr_pkt->locallane_cau_calc_cnt    <= (instr_pkt->locallane_cau_calctime_len)   "<<  instr_pkt->locallane_cau_calc_cnt    <<" < "<<(instr_pkt->locallane_cau_calctime_len)
          //                                     <<", instr_pkt->locallane_serdiv_calc_cnt <= (instr_pkt->locallane_serdiv_calctime_len)"<<  instr_pkt->locallane_serdiv_calc_cnt <<" < "<<(instr_pkt->locallane_serdiv_calctime_len)<<std::endl;
        }
    }
    else if(this->MY_VENUS_INS_PARAM.vew == EW16)
    {
        data_vmask_r_element = (((*vmask_r) & 0x1)&(((*vmask_r) & 0x100)>>8));
        DPRINTF(LaneVFUFull,"%s%s%s%s%s%s%s%s%s%s%s%d\n"
            , (instr_pkt->use_scalar_op?"   this->scalar_op = 0x":""), (instr_pkt->use_scalar_op?int2Hex(this->MY_VENUS_INS_PARAM.scalar_op&0xffff):"")
            , (instr_pkt->use_vs1      ?"    this->data_vs1 = 0x":""), (instr_pkt->use_vs1      ?int2Hex(this->data_vs1&0xffff)                    :"")
            , (instr_pkt->use_vs2      ?"    this->data_vs2 = 0x":""), (instr_pkt->use_vs2      ?int2Hex(this->data_vs2&0xffff)                    :"")
            , (instr_pkt->use_vd1_op   ?"    this->data_vd1 = 0x":""), (instr_pkt->use_vd1_op   ?int2Hex(this->data_vd1&0xffff)                    :"")
            , (instr_pkt->use_vd2_op   ?"    this->data_vd2 = 0x":""), (instr_pkt->use_vd2_op   ?int2Hex(this->data_vd2&0xffff)                    :"")
            , ", mask is ", (data_vmask_r_element|(!instr_pkt->vm_r)));
        doVEMU(instr_pkt, 0xffff, 0xffff, 0xffff, 0xffff);
             if(this->MY_VENUS_INS_PARAM.op == VREDAND) { DPRINTF(LaneVFUFull,"and_value_1 = %x\n", and_value_1); }
        else if(this->MY_VENUS_INS_PARAM.op == VREDOR) { DPRINTF(LaneVFUFull,"or_value_1 = %x\n", or_value_1); }
        else if(this->MY_VENUS_INS_PARAM.op == VREDXOR) { DPRINTF(LaneVFUFull,"xor_value_1 = %x\n", xor_value_1); }
        else if(this->MY_VENUS_INS_PARAM.op == VREDMIN) { DPRINTF(LaneVFUFull,"min_value_1 = %d\n", min_value_1); }
        else if(this->MY_VENUS_INS_PARAM.op == VREDMINU) { DPRINTF(LaneVFUFull,"min_uvalue_1 = %d\n", min_uvalue_1); }
        else if(this->MY_VENUS_INS_PARAM.op == VREDMAX) { DPRINTF(LaneVFUFull,"max_value_1 = %d\n", max_value_1); }
        else if(this->MY_VENUS_INS_PARAM.op == VREDMAXU) { DPRINTF(LaneVFUFull,"max_uvalue_1 = %d\n", max_uvalue_1); }
        else if(this->MY_VENUS_INS_PARAM.op == VREDSUM) { DPRINTF(LaneVFUFull,"sum_result = %d\n", sum_result); }
        else {
          DPRINTF(LaneVFUFull,"%s%s%s%s%s%s%s%d\n"
              , (instr_pkt->use_vd1      ?"    this->data_vd1 = 0x"    :""), (instr_pkt->use_vd1      ?int2Hex(this->data_vd1&0xffff)    :"")
              , (instr_pkt->use_vd2      ?"    this->data_vd2 = 0x"    :""), (instr_pkt->use_vd2      ?int2Hex(this->data_vd2&0xffff)    :"")
              , (instr_pkt->vm_w         ?"    this->data_vmask_w = 0x":""), (instr_pkt->vm_w         ?int2Hex(this->data_vmask_w&0xffff):"")
              , ", mask is ", (data_vmask_r_element|(!instr_pkt->vm_r)));
        }
    }


    if(vd1_w   != nullptr)  *vd1_w   = this->data_vd1    ;
    if(vd2_w   != nullptr)  *vd2_w   = this->data_vd2    ;
    if(vmask_w != nullptr)  *vmask_w = this->data_vmask_w;
    if(vd1_w   != nullptr && (!instr_pkt->use_vd1))  *vd1_w   = INT_MIN;
    if(vd2_w   != nullptr && (!instr_pkt->use_vd2))  *vd2_w   = INT_MIN;
    if(vmask_w != nullptr && (!instr_pkt->vm_w   ))  *vmask_w = INT_MIN;

    if (trace_vfu) {
        std::filesystem::create_directories("Debug/venusgem5_debug");
        std::ofstream trace(
            "Debug/venusgem5_debug/vfu_trace_i" +
                std::to_string(instr_pkt->vns_instr_id) + ".txt",
            std::ios::app);
        trace << "lane=" << localLaneID
              << " op=" << op_to_str(instr_pkt->op)
              << " vew=" << vew_to_str(instr_pkt->vew)
              << " calc_cnt=" << instr_pkt->locallane_bitalu_calc_cnt
              << " in_vd1=0x" << int2Hex(trace_in_vd1 & 0xffff)
              << " in_vs1=0x" << int2Hex(trace_in_vs1 & 0xffff)
              << " in_vs2=0x" << int2Hex(trace_in_vs2 & 0xffff)
              << " in_mask=0x" << int2Hex(trace_in_mask & 0xffff)
              << " out_vd1=0x" << int2Hex(this->data_vd1 & 0xffff)
              << " out_mask=0x" << int2Hex(this->data_vmask_w & 0xffff)
              << "\n";
    }
}

//                                  instr_pkt,              vd1_w,                   vd2_w,   vd1_r,   vd2_r,   vmask_w,                 vs1,                      vs2,                      vmask_r);
// venus_function_unit.doVenusVFU(running_bitalu_instr_pkt, &bitalu_vd1_result_buf, nullptr, nullptr, nullptr, &bitalu_vmask_result_buf, &running_bitaluA_data_pkt, &running_bitaluB_data_pkt, &running_mask_data_pkt);

//

void venus_vfu::doVEMU(VenusInstrPkt* instr_pkt, uint16_t vd1_mask, uint16_t vd2_mask, uint16_t vs1_mask, uint16_t vs2_mask) {
    int16_t temp_res;
    switch (this->MY_VENUS_INS_PARAM.op) {
        case VAND: {
          this->instr_name = (char *)"VAND";
          if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
            if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
              if (vd1_mask == 0xff00)
                temp_res = (uint8_t)((this->data_vs2 & (vs2_mask)) >> 8) & (uint8_t)this->MY_VENUS_INS_PARAM.scalar_op;
              else if (vd1_mask == 0x00ff)
                temp_res = (uint8_t)((this->data_vs2 & (vs2_mask))) & (uint8_t)this->MY_VENUS_INS_PARAM.scalar_op;
              else
                temp_res = (uint16_t)((this->data_vs2 & (vs2_mask))) & (uint16_t)this->MY_VENUS_INS_PARAM.scalar_op;
            } else { // IVV
              if (vd1_mask == 0xff00)
                temp_res = (uint8_t)((this->data_vs2 & (vs2_mask)) >> 8) & (uint8_t)((this->data_vs1 & (vs1_mask)) >> 8);
              else if (vd1_mask == 0x00ff)
                temp_res = (uint8_t)((this->data_vs2 & (vs2_mask))) & (uint8_t)((this->data_vs1 & (vs1_mask)));
              else
                temp_res = (uint16_t)((this->data_vs2 & (vs2_mask))) & (uint16_t)((this->data_vs1 & (vs1_mask)));
            }
            if (vd1_mask == 0xff00)
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
            else
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
          } else {
            this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
          }
          break;
        }
        case VOR: {
          this->instr_name = (char *)"VOR";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00)
                  temp_res = (uint8_t)((this->data_vs2) >> 8) | (uint8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else if (vd1_mask == 0x00ff)
                  temp_res = (uint8_t)((this->data_vs2)) | (uint8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else
                  temp_res = (uint16_t)((this->data_vs2)) | (uint16_t)this->MY_VENUS_INS_PARAM.scalar_op;
              } else { // IVV
                if (vd1_mask == 0xff00)
                  temp_res = (uint8_t)((this->data_vs2) >> 8) | (uint8_t)((this->data_vs1) >> 8);
                else if (vd1_mask == 0x00ff)
                  temp_res = (uint8_t)((this->data_vs2)) | (uint8_t)((this->data_vs1));
                else
                  temp_res = (uint16_t)((this->data_vs2)) | (uint16_t)((this->data_vs1));
              }
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VXOR: {
          this->instr_name = (char *)"VXOR";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX)
                if (vd1_mask == 0xff00)
                  temp_res = (uint8_t)((this->data_vs2) >> 8) ^ (uint8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else if (vd1_mask == 0x00ff)
                  temp_res = (uint8_t)(this->data_vs2) ^ (uint8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else
                  temp_res = (uint16_t)(this->data_vs2) ^ (uint16_t)this->MY_VENUS_INS_PARAM.scalar_op;
              else // IVV
                  if (vd1_mask == 0xff00)
                temp_res = (uint8_t)((this->data_vs2) >> 8) ^ (uint8_t)((this->data_vs1) >> 8);
              else if (vd1_mask == 0x00ff)
                temp_res = (uint8_t)(this->data_vs2) ^ (uint8_t)(this->data_vs1);
              else
                temp_res = (uint16_t)(this->data_vs2) ^ (uint16_t)(this->data_vs1);
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VBRDCST: {
          this->instr_name = (char *)"VBRDCST";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((this->MY_VENUS_INS_PARAM.scalar_op << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->MY_VENUS_INS_PARAM.scalar_op & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VSLL: {
          this->instr_name = (char *)"VSLL";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00)
                  temp_res = (uint8_t)((this->data_vs2) >> 8) << (uint8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else if (vd1_mask == 0x00ff)
                  temp_res = (uint8_t)((this->data_vs2)) << (uint8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else
                  temp_res = (uint16_t)((this->data_vs2)) << (uint16_t)this->MY_VENUS_INS_PARAM.scalar_op;
              } else { // IVV
                if (vd1_mask == 0xff00)
                  temp_res = (uint8_t)((this->data_vs2) >> 8) << (uint8_t)((this->data_vs1) >> 8);
                else if (vd1_mask == 0x00ff)
                  temp_res = (uint8_t)((this->data_vs2)) << (uint8_t)((this->data_vs1));
                else
                  temp_res = (uint16_t)((this->data_vs2)) << (uint16_t)((this->data_vs1));
              }
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VSRL: {
          this->instr_name = (char *)"VSRL";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00)
                  temp_res = (uint8_t)((this->data_vs2) >> 8) >> (uint8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else if (vd1_mask == 0x00ff)
                  temp_res = (uint8_t)((this->data_vs2)) >> (uint8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else
                  temp_res = (uint16_t)((this->data_vs2 & (vs2_mask))) >> (uint16_t)this->MY_VENUS_INS_PARAM.scalar_op;
              } else {// IVV
                  if (vd1_mask == 0xff00)
                    temp_res = (uint8_t)((this->data_vs2) >> 8) >> (uint8_t)((this->data_vs1) >> 8);
                  else if (vd1_mask == 0x00ff)
                    temp_res = (uint8_t)((this->data_vs2)) >> (uint8_t)((this->data_vs1));
                  else
                    temp_res = (uint16_t)((this->data_vs2)) >> (uint16_t)((this->data_vs1));
              }
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VSRA: {
          this->instr_name = (char *)"VSRA";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX)
                if (vd1_mask == 0xff00)
                  temp_res = (int8_t)((this->data_vs2) >> 8) >> (int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else if (vd1_mask == 0x00ff)
                  temp_res = (int8_t)((this->data_vs2)) >> (int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else
                  temp_res = (int16_t)(this->data_vs2) >> (int16_t)this->MY_VENUS_INS_PARAM.scalar_op;
              else // IVV
                  if (vd1_mask == 0xff00)
                    temp_res = (int8_t)((this->data_vs2) >> 8) >> (int8_t)((this->data_vs1) >> 8);
                  else if (vd1_mask == 0x00ff)
                    temp_res = (int8_t)((this->data_vs2)) >> (int8_t)((this->data_vs1));
                  else
                    temp_res = (int16_t)(this->data_vs2) >> (int16_t)(this->data_vs1);
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VABS: {
          this->instr_name = (char *)"VABS";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
                if (vd1_mask == 0xff00) {
                    int8_t val = (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                    temp_res = (val == INT8_MIN) ? INT8_MAX : ((val < 0) ? (-val) : val);
                } else if (vd1_mask == 0x00ff) {
                    int8_t val = (int8_t)(this->data_vs2 & (vs2_mask));
                    temp_res = (val == INT8_MIN) ? INT8_MAX : ((val < 0) ? (-val) : val);
                } else {
                    int16_t val = (int16_t)(this->data_vs2 & (vs2_mask));
                    temp_res = (val == INT16_MIN) ? INT16_MAX : ((val < 0) ? (-val) : val);
                }
                if (vd1_mask == 0xff00)
                    this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else
                    this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VSEQ: {
          this->instr_name = (char *)"VSEQ";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              switch (this->MY_VENUS_INS_PARAM.function3) {
              case IVV: {
                if (vd1_mask == 0xff00)
                  temp_res = (int8_t)((this->data_vs2) >> 8) == (int8_t)((this->data_vs1) >> 8);
                else if (vd1_mask == 0x00ff)
                  temp_res = (int8_t)((this->data_vs2)) == (int8_t)((this->data_vs1));
                else
                  temp_res = (int16_t)((this->data_vs2)) == (int16_t)((this->data_vs1));
                if (vd1_mask == 0xff00)
                  this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else
                  this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
                break;
              }
              case IVX: {
                if (vd1_mask == 0xff00)
                  temp_res = (int8_t)((this->data_vs2) >> 8) == (int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else if (vd1_mask == 0x00ff)
                  temp_res = (int8_t)((this->data_vs2)) == (int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else
                  temp_res = (int16_t)((this->data_vs2)) == (int16_t)this->MY_VENUS_INS_PARAM.scalar_op;
                if (vd1_mask == 0xff00)
                  this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else
                  this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
                break;
              }
              case MVV: {
                if (vd1_mask == 0xff00)
                  temp_res = (int8_t)((this->data_vs2) >> 8) == (int8_t)((this->data_vs1) >> 8);
                else if (vd1_mask == 0x00ff)
                  temp_res = (int8_t)((this->data_vs2)) == (int8_t)((this->data_vs1));
                else
                  temp_res = (int16_t)((this->data_vs2)) == (int16_t)((this->data_vs1));
                if (vd1_mask == 0xff00)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else if (vd1_mask == 0x00ff)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + (temp_res & vd1_mask);
                else if (vd1_mask == 0xffff)
                  this->data_vmask_w = ((temp_res << 8) & vd1_mask) + (temp_res & vd1_mask);
                break;
              }
              case MVX: {
                if (vd1_mask == 0xff00)
                  temp_res = (int8_t)((this->data_vs2) >> 8) == (int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else if (vd1_mask == 0x00ff)
                  temp_res = (int8_t)((this->data_vs2)) == (int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else
                  temp_res = (int16_t)((this->data_vs2)) == (int16_t)this->MY_VENUS_INS_PARAM.scalar_op;
                if (vd1_mask == 0xff00)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else if (vd1_mask == 0x00ff)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + (temp_res & vd1_mask);
                else if (vd1_mask == 0xffff)
                  this->data_vmask_w = ((temp_res << 8) & vd1_mask) + (temp_res & vd1_mask);
                break;
              }
              default: {
                // TODO: other cases
                std::cout << "TODO: VSEQ other cases" << std::endl;
                panic("Unknown func3");
                break;
              }
              }
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
              this->data_vmask_w = (this->data_vmask_r & (~vd1_mask)) + (this->data_vmask_r & (vd1_mask));
            }
          break;
        }
        case VSNE: {
          this->instr_name = (char *)"VSNE";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read &&
                 data_vmask_r_element)) {
              switch (this->MY_VENUS_INS_PARAM.function3) {
              case IVV: {
                if (vd1_mask == 0xff00)
                  temp_res =(int8_t)((this->data_vs2) >> 8) !=(int8_t)((this->data_vs1) >> 8);
                else if (vd1_mask == 0x00ff)
                  temp_res =(int8_t)((this->data_vs2)) !=(int8_t)((this->data_vs1));
                else
                  temp_res =(int16_t)((this->data_vs2)) !=(int16_t)((this->data_vs1));
                if (vd1_mask == 0xff00)
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +((temp_res << 8) & vd1_mask);
                else
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +(temp_res & vd1_mask);
                break;
              }
              case IVX: {
                if (vd1_mask == 0xff00)
                  temp_res =(int8_t)((this->data_vs2) >> 8) != (int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else if (vd1_mask == 0x00ff)
                  temp_res =(int8_t)((this->data_vs2)) !=(int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else
                  temp_res =(int16_t)((this->data_vs2)) !=(int16_t)this->MY_VENUS_INS_PARAM.scalar_op;
                if (vd1_mask == 0xff00)
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +((temp_res << 8) & vd1_mask);
                else
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +(temp_res & vd1_mask);
                break;
              }
              case MVV: {
                if (vd1_mask == 0xff00)
                  temp_res =(int8_t)((this->data_vs2) >> 8) !=(int8_t)((this->data_vs1) >> 8);
                else if (vd1_mask == 0x00ff)
                  temp_res =(int8_t)((this->data_vs2)) !=(int8_t)((this->data_vs1));
                else
                  temp_res =(int16_t)((this->data_vs2)) !=(int16_t)((this->data_vs1));
                if (vd1_mask == 0xff00)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else if (vd1_mask == 0x00ff)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + (temp_res & vd1_mask);
                else if (vd1_mask == 0xffff)
                  this->data_vmask_w = ((temp_res << 8) & vd1_mask) + (temp_res & vd1_mask);
                // this->VENUS_MASK_DSPM[i] = (bool)temp_res;
                break;
              }
              case MVX: {
                if (vd1_mask == 0xff00)
                  temp_res =(int8_t)((this->data_vs2) >> 8) != (int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else if (vd1_mask == 0x00ff)
                  temp_res =(int8_t)((this->data_vs2)) !=(int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else
                  temp_res =(int16_t)((this->data_vs2)) !=(int16_t)this->MY_VENUS_INS_PARAM.scalar_op;
                if (vd1_mask == 0xff00)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else if (vd1_mask == 0x00ff)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + (temp_res & vd1_mask);
                else if (vd1_mask == 0xffff)
                  this->data_vmask_w = ((temp_res << 8) & vd1_mask) + (temp_res & vd1_mask);
                // this->VENUS_MASK_DSPM[i] = (bool)temp_res;
                break;
              }
              default: {
                // TODO: other cases
                std::cout << "TODO: VSNE other cases" << std::endl;
                panic("Unknown func3");
                break;
              }
              }
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
              this->data_vmask_w = (this->data_vmask_r & (~vd1_mask)) + (this->data_vmask_r & (vd1_mask));
            }
          break;
        }
        case VSLTU: {
          this->instr_name = (char *)"VSLTU";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              switch (this->MY_VENUS_INS_PARAM.function3) {
              case IVV: {
                if (vd1_mask == 0xff00)
                  temp_res =(uint8_t)((this->data_vs2) >>  8) >(uint8_t)((this->data_vs1) >>  8);
                else if (vd1_mask == 0x00ff)
                  temp_res =(uint8_t)((this->data_vs2)) >(uint8_t)((this->data_vs1));
                else
                  temp_res =(uint16_t)((this->data_vs2)) >(uint16_t)((this->data_vs1));
                if (vd1_mask == 0xff00)
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +((temp_res << 8) & vd1_mask);
                else
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +(temp_res & vd1_mask);
                break;
              }
              case IVX: {
                if (vd1_mask == 0xff00)
                  temp_res =(uint8_t)((this->data_vs2) >>  8) > (uint8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else if (vd1_mask == 0x00ff)
                  temp_res =(uint8_t)((this->data_vs2)) >(uint8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else
                  temp_res =(uint16_t)((this->data_vs2)) >(uint16_t)this->MY_VENUS_INS_PARAM.scalar_op;
                if (vd1_mask == 0xff00)
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +((temp_res << 8) & vd1_mask);
                else
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +(temp_res & vd1_mask);
                break;
              }
              case MVV: {
                if (vd1_mask == 0xff00)
                  temp_res =(uint8_t)((this->data_vs2) >>  8) >(uint8_t)((this->data_vs1) >>  8);
                else if (vd1_mask == 0x00ff)
                  temp_res =(uint8_t)((this->data_vs2)) >(uint8_t)((this->data_vs1));
                else
                  temp_res =(uint16_t)((this->data_vs2)) >(uint16_t)((this->data_vs1));
                if (vd1_mask == 0xff00)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else if (vd1_mask == 0x00ff)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + (temp_res & vd1_mask);
                else if (vd1_mask == 0xffff)
                  this->data_vmask_w = ((temp_res << 8) & vd1_mask) + (temp_res & vd1_mask);
                // this->VENUS_MASK_DSPM[i] = (bool)temp_res;
                break;
              }
              case MVX: {
                if (vd1_mask == 0xff00)
                  temp_res =(uint8_t)((this->data_vs2) >>  8) > (uint8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else if (vd1_mask == 0x00ff)
                  temp_res =(uint8_t)((this->data_vs2)) >(uint8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else
                  temp_res =(uint16_t)((this->data_vs2)) >(uint16_t)this->MY_VENUS_INS_PARAM.scalar_op;
                if (vd1_mask == 0xff00)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else if (vd1_mask == 0x00ff)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + (temp_res & vd1_mask);
                else if (vd1_mask == 0xffff)
                  this->data_vmask_w = ((temp_res << 8) & vd1_mask) + (temp_res & vd1_mask);
                // this->VENUS_MASK_DSPM[i] = (bool)temp_res;
                break;
              }
              default: {
                // TODO: other cases
                std::cout << "TODO: VSLTU other cases" << std::endl;
                panic("Unknown func3");
                break;
              }
              }
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
              this->data_vmask_w = (this->data_vmask_r & (~vd1_mask)) + (this->data_vmask_r & (vd1_mask));
            }
          break;
        }
        case VSLT: {
          this->instr_name = (char *)"VSLT";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              switch (this->MY_VENUS_INS_PARAM.function3) {
              case IVV: {
                if (vd1_mask == 0xff00)
                  temp_res =(int8_t)((this->data_vs2) >> 8) >(int8_t)((this->data_vs1) >> 8);
                else if (vd1_mask == 0x00ff)
                  temp_res =(int8_t)((this->data_vs2)) >(int8_t)((this->data_vs1));
                else
                  temp_res =(int16_t)((this->data_vs2)) >(int16_t)((this->data_vs1));
                if (vd1_mask == 0xff00)
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +((temp_res << 8) & vd1_mask);
                else
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +(temp_res & vd1_mask);
                break;
              }
              case IVX: {
                if (vd1_mask == 0xff00)
                  temp_res =(int8_t)((this->data_vs2) >> 8) > (int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else if (vd1_mask == 0x00ff)
                  temp_res =(int8_t)((this->data_vs2)) >(int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else
                  temp_res =(int16_t)((this->data_vs2)) >(int16_t)this->MY_VENUS_INS_PARAM.scalar_op;
                if (vd1_mask == 0xff00)
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +((temp_res << 8) & vd1_mask);
                else
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +(temp_res & vd1_mask);
                break;
              }
              case MVV: {
                if (vd1_mask == 0xff00)
                  temp_res =(int8_t)((this->data_vs2) >> 8) >(int8_t)((this->data_vs1) >> 8);
                else if (vd1_mask == 0x00ff)
                  temp_res =(int8_t)((this->data_vs2)) >(int8_t)((this->data_vs1));
                else
                  temp_res =(int16_t)((this->data_vs2)) >(int16_t)((this->data_vs1));
                if (vd1_mask == 0xff00)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else if (vd1_mask == 0x00ff)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + (temp_res & vd1_mask);
                else if (vd1_mask == 0xffff)
                  this->data_vmask_w = ((temp_res << 8) & vd1_mask) + (temp_res & vd1_mask);
                // this->VENUS_MASK_DSPM[i] = (bool)temp_res;
                break;
              }
              case MVX: {
                if (vd1_mask == 0xff00)
                  temp_res =(int8_t)((this->data_vs2) >> 8) > (int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else if (vd1_mask == 0x00ff)
                  temp_res =(int8_t)((this->data_vs2)) >(int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else
                  temp_res =(int16_t)((this->data_vs2)) >(int16_t)this->MY_VENUS_INS_PARAM.scalar_op;

                if (vd1_mask == 0xff00)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else if (vd1_mask == 0x00ff)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + (temp_res & vd1_mask);
                else if (vd1_mask == 0xffff)
                  this->data_vmask_w = ((temp_res << 8) & vd1_mask) + (temp_res & vd1_mask);
                break;
              }
              default: {
                // TODO: other cases
                std::cout << "TODO: VSLT other cases" << std::endl;
                panic("Unknown func3");
                break;
              }
              }
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
              this->data_vmask_w = (this->data_vmask_r & (~vd1_mask)) + (this->data_vmask_r & (vd1_mask));
            }
          break;
        }
        case VSLEU: {
          this->instr_name = (char *)"VSLEU";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              switch (this->MY_VENUS_INS_PARAM.function3) {
              case IVV: {
                if (vd1_mask == 0xff00)
                  temp_res =(uint8_t)((this->data_vs2) >>  8) >=(uint8_t)((this->data_vs1) >>  8);
                else if (vd1_mask == 0x00ff)
                  temp_res =(uint8_t)((this->data_vs2)) >=(uint8_t)((this->data_vs1));
                else
                  temp_res =(uint16_t)(    (this->data_vs2)) >=(uint16_t)((this->data_vs1));
                if (vd1_mask == 0xff00)
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +((temp_res << 8) & vd1_mask);
                else
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +(temp_res & vd1_mask);
                break;
              }
              case IVX: {
                if (vd1_mask == 0xff00)
                  temp_res =(uint8_t)((this->data_vs2) >>  8) >= (uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                else if (vd1_mask == 0x00ff)
                  temp_res =(uint8_t)((this->data_vs2)) >=(uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                else
                  temp_res = (uint16_t)((this->data_vs2 &                   (vs2_mask))) >=       (uint16_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                if (vd1_mask == 0xff00)
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +((temp_res << 8) & vd1_mask);
                else
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +(temp_res & vd1_mask);
                break;
              }
              case MVV: {
                if (vd1_mask == 0xff00)
                  temp_res =(uint8_t)((this->data_vs2) >>  8) >=(uint8_t)((this->data_vs1) >>  8);
                else if (vd1_mask == 0x00ff)
                  temp_res =(uint8_t)((this->data_vs2)) >=(uint8_t)((this->data_vs1));
                else
                  temp_res =(uint16_t)(    (this->data_vs2)) >=(uint16_t)((this->data_vs1));
                if (vd1_mask == 0xff00)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else if (vd1_mask == 0x00ff)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + (temp_res & vd1_mask);
                else if (vd1_mask == 0xffff)
                  this->data_vmask_w = ((temp_res << 8) & vd1_mask) + (temp_res & vd1_mask);
                // this->VENUS_MASK_DSPM[i] = (bool)temp_res;
                break;
              }
              case MVX: {
                if (vd1_mask == 0xff00)
                  temp_res =(uint8_t)((this->data_vs2) >>  8) >= (uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                else if (vd1_mask == 0x00ff)
                  temp_res =(uint8_t)((this->data_vs2)) >=(uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                else
                  temp_res = (uint16_t)((this->data_vs2 & (vs2_mask))) >= (uint16_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                if (vd1_mask == 0xff00)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else if (vd1_mask == 0x00ff)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + (temp_res & vd1_mask);
                else if (vd1_mask == 0xffff)
                  this->data_vmask_w = ((temp_res << 8) & vd1_mask) + (temp_res & vd1_mask);
                // this->VENUS_MASK_DSPM[i] = (bool)temp_res;
                break;
              }
              default: {
                // TODO: other cases
                std::cout << "TODO: VSLEU other cases" << std::endl;
                panic("Unknown func3");
                break;
              }
              }
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
              this->data_vmask_w = (this->data_vmask_r & (~vd1_mask)) + (this->data_vmask_r & (vd1_mask));
            }
          break;
        }
        case VSLE: {
          this->instr_name = (char *)"VSLE";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              switch (this->MY_VENUS_INS_PARAM.function3) {
              case IVV: {
                if (vd1_mask == 0xff00)
                  temp_res =(int8_t)((this->data_vs2) >> 8) >=(int8_t)((this->data_vs1) >> 8);
                else if (vd1_mask == 0x00ff)
                  temp_res =(int8_t)((this->data_vs2)) >=(int8_t)((this->data_vs1));
                else
                  temp_res =(int16_t)((this->data_vs2)) >=(int16_t)(this->data_vs1);
                if (vd1_mask == 0xff00)
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +((temp_res << 8) & vd1_mask);
                else
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +(temp_res & vd1_mask);
                break;
              }
              case IVX: {
                if (vd1_mask == 0xff00)
                  temp_res =(int8_t)((this->data_vs2) >> 8) >= (int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                else if (vd1_mask == 0x00ff)
                  temp_res =(int8_t)((this->data_vs2)) >=(int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                else
                  temp_res =(int16_t)((this->data_vs2)) >=(int16_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                if (vd1_mask == 0xff00)
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +((temp_res << 8) & vd1_mask);
                else
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +(temp_res & vd1_mask);
                break;
              }
              case MVV: {
                if (vd1_mask == 0xff00)
                  temp_res =(int8_t)((this->data_vs2) >> 8) >=(int8_t)((this->data_vs1) >> 8);
                else if (vd1_mask == 0x00ff)
                  temp_res =(int8_t)((this->data_vs2)) >=(int8_t)((this->data_vs1));
                else
                  temp_res =(int16_t)((this->data_vs2)) >=(int16_t)((this->data_vs1));
                if (vd1_mask == 0xff00)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else if (vd1_mask == 0x00ff)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + (temp_res & vd1_mask);
                else if (vd1_mask == 0xffff)
                  this->data_vmask_w = ((temp_res << 8) & vd1_mask) + (temp_res & vd1_mask);
                // this->VENUS_MASK_DSPM[i] = (bool)temp_res;
                break;
              }
              case MVX: {
                if (vd1_mask == 0xff00)
                  temp_res =(int8_t)((this->data_vs2) >> 8) >= (int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                else if (vd1_mask == 0x00ff)
                  temp_res =(int8_t)((this->data_vs2)) >=(int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                else
                  temp_res =(int16_t)((this->data_vs2)) >=(int16_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                if (vd1_mask == 0xff00)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else if (vd1_mask == 0x00ff)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + (temp_res & vd1_mask);
                else if (vd1_mask == 0xffff)
                  this->data_vmask_w = ((temp_res << 8) & vd1_mask) + (temp_res & vd1_mask);
                // this->VENUS_MASK_DSPM[i] = (bool)temp_res;
                break;
              }
              default: {
                // TODO: other cases
                std::cout << "TODO: VSLE other cases" << std::endl;
                panic("Unknown func3");
                break;
              }
              }
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
              this->data_vmask_w = (this->data_vmask_r & (~vd1_mask)) + (this->data_vmask_r & (vd1_mask));
            }
          break;
        }
        case VSGTU: {
          this->instr_name = (char *)"VSGTU";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              switch (this->MY_VENUS_INS_PARAM.function3) {
              case IVV: {
                if (vd1_mask == 0xff00)
                  temp_res =(uint8_t)((this->data_vs2) >>  8) <(uint8_t)((this->data_vs1) >>  8);
                else if (vd1_mask == 0x00ff)
                  temp_res =(uint8_t)((this->data_vs2)) <(uint8_t)((this->data_vs1));
                else
                  temp_res =(uint16_t)((this->data_vs2)) <(uint16_t)((this->data_vs1));
                if (vd1_mask == 0xff00)
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +((temp_res << 8) & vd1_mask);
                else
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +(temp_res & vd1_mask);
                break;
              }
              case IVX: {
                if (vd1_mask == 0xff00)
                  temp_res =(uint8_t)((this->data_vs2) >>  8) < (uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                else if (vd1_mask == 0x00ff)
                  temp_res =(uint8_t)((this->data_vs2)) <(uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                else
                  temp_res =(uint16_t)((this->data_vs2)) <(uint16_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                if (vd1_mask == 0xff00)
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +((temp_res << 8) & vd1_mask);
                else
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +(temp_res & vd1_mask);
                break;
              }
              case MVV: {
                if (vd1_mask == 0xff00)
                  temp_res =(uint8_t)((this->data_vs2) >>  8) <(uint8_t)((this->data_vs1) >>  8);
                else if (vd1_mask == 0x00ff)
                  temp_res =(uint8_t)((this->data_vs2)) <(uint8_t)((this->data_vs1));
                else
                  temp_res =(uint16_t)((this->data_vs2)) <(uint16_t)((this->data_vs1));
                if (vd1_mask == 0xff00)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else if (vd1_mask == 0x00ff)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + (temp_res & vd1_mask);
                else if (vd1_mask == 0xffff)
                  this->data_vmask_w = ((temp_res << 8) & vd1_mask) + (temp_res & vd1_mask);
                // this->VENUS_MASK_DSPM[i] = (bool)temp_res;
                break;
              }
              case MVX: {
                if (vd1_mask == 0xff00)
                  temp_res =(uint8_t)((this->data_vs2) >>  8) < (uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                else if (vd1_mask == 0x00ff)
                  temp_res =(uint8_t)((this->data_vs2)) <(uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                else
                  temp_res =(uint16_t)((this->data_vs2)) <(uint16_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                if (vd1_mask == 0xff00)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else if (vd1_mask == 0x00ff)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + (temp_res & vd1_mask);
                else if (vd1_mask == 0xffff)
                  this->data_vmask_w = ((temp_res << 8) & vd1_mask) + (temp_res & vd1_mask);
                // this->VENUS_MASK_DSPM[i] = (bool)temp_res;
                break;
              }
              default: {
                // TODO: other cases
                std::cout << "TODO: VSGTU other cases" << std::endl;
                panic("Unknown func3");
                break;
              }
              }
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
              this->data_vmask_w = (this->data_vmask_r & (~vd1_mask)) + (this->data_vmask_r & (vd1_mask));
            }
          break;
        }
        case VSGT: {
          this->instr_name = (char *)"VSGT";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              switch (this->MY_VENUS_INS_PARAM.function3) {
              case IVV: {
                if (vd1_mask == 0xff00)
                  temp_res =(int8_t)((this->data_vs2) >> 8) <(int8_t)((this->data_vs1) >> 8);
                else if (vd1_mask == 0x00ff)
                  temp_res =(int8_t)((this->data_vs2)) <(int8_t)((this->data_vs1));
                else
                  temp_res =(int16_t)((this->data_vs2)) <(int16_t)((this->data_vs1));
                if (vd1_mask == 0xff00)
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +((temp_res << 8) & vd1_mask);
                else
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +(temp_res & vd1_mask);
                break;
              }
              case IVX: {
                if (vd1_mask == 0xff00)
                  temp_res =(int8_t)((this->data_vs2) >> 8) < (int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else if (vd1_mask == 0x00ff)
                  temp_res =(int8_t)((this->data_vs2)) <(int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else
                  temp_res =(int16_t)((this->data_vs2)) <(int16_t)this->MY_VENUS_INS_PARAM.scalar_op;
                if (vd1_mask == 0xff00)
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +((temp_res << 8) & vd1_mask);
                else
                  this->data_vd1 =(this->data_vd1 & (~vd1_mask)) +(temp_res & vd1_mask);
                break;
              }
              case MVV: {
                if (vd1_mask == 0xff00)
                  temp_res =(int8_t)((this->data_vs2) >> 8) <(int8_t)((this->data_vs1) >> 8);
                else if (vd1_mask == 0x00ff)
                  temp_res =(int8_t)((this->data_vs2)) <(int8_t)((this->data_vs1));
                else
                  temp_res =(int16_t)((this->data_vs2)) < (int16_t)((this->data_vs1));
                if (vd1_mask == 0xff00)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else if (vd1_mask == 0x00ff)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + (temp_res & vd1_mask);
                else if (vd1_mask == 0xffff)
                  this->data_vmask_w = ((temp_res << 8) & vd1_mask) + (temp_res & vd1_mask);
                // this->VENUS_MASK_DSPM[i] = (bool)temp_res;
                break;
              }
              case MVX: {
                if (vd1_mask == 0xff00)
                  temp_res = (int8_t)((this->data_vs2) >> 8) < (int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else if (vd1_mask == 0x00ff)
                  temp_res = (int8_t)((this->data_vs2)) < (int8_t)this->MY_VENUS_INS_PARAM.scalar_op;
                else
                  temp_res = (int16_t)((this->data_vs2)) < (int16_t)this->MY_VENUS_INS_PARAM.scalar_op;
                if (vd1_mask == 0xff00)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
                else if (vd1_mask == 0x00ff)
                  this->data_vmask_w = (this->data_vmask_w & (~vd1_mask)) + (temp_res & vd1_mask);
                else if (vd1_mask == 0xffff)
                  this->data_vmask_w = ((temp_res << 8) & vd1_mask) + (temp_res & vd1_mask);
                // this->VENUS_MASK_DSPM[i] = (bool)temp_res;
                break;
              }
              default: {
                // TODO: other cases
                std::cout << "TODO: VSGT other cases" << std::endl;
                panic("Unknown func3");
                break;
              }
              }
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
              this->data_vmask_w = (this->data_vmask_r & (~vd1_mask)) + (this->data_vmask_r & (vd1_mask));
            }
          break;
        }

        case VADD: {
          this->instr_name = (char *)"VADD";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00)
                  temp_res = (int8_t)(this->MY_VENUS_INS_PARAM.scalar_op) + (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                else if (vd1_mask == 0x00ff)
                  temp_res = (int8_t)(this->MY_VENUS_INS_PARAM.scalar_op) + (int8_t)((this->data_vs2 & (vs2_mask)));
                else
                  temp_res = (int16_t)(this->MY_VENUS_INS_PARAM.scalar_op) + (int16_t)((this->data_vs2 & (vs2_mask)));
              } else { // IVV
                if (vd1_mask == 0xff00)
                  temp_res = (int8_t)((this->data_vs1 & (vs1_mask)) >> 8) + (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                else if (vd1_mask == 0x00ff)
                  temp_res = (int8_t)(this->data_vs1 & (vs1_mask)) + (int8_t)(this->data_vs2 & (vs2_mask));
                else
                  temp_res = (int16_t)(this->data_vs1 & (vs1_mask)) + (int16_t)(this->data_vs2 & (vs2_mask));
              }
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else if (vd1_mask == 0xff)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VSADD: {
          this->instr_name = (char *)"VSADD";
          int add_res;
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read &&  data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00)
                  add_res = (int8_t)(this->MY_VENUS_INS_PARAM.scalar_op) + (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                else if (vd1_mask == 0x00ff)
                  add_res = (int8_t)(this->MY_VENUS_INS_PARAM.scalar_op) + (int8_t)((this->data_vs2 & (vs2_mask)));
                else
                  add_res = (int16_t)(this->MY_VENUS_INS_PARAM.scalar_op) + (int16_t)((this->data_vs2 & (vs2_mask)));
              } else { // IVV
                if (vd1_mask == 0xff00)
                  add_res = (int8_t)((this->data_vs1 & (vs1_mask)) >> 8) + (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                else if (vd1_mask == 0x00ff)
                  add_res = (int8_t)((this->data_vs1 & (vs1_mask))) + (int8_t)((this->data_vs2 & (vs2_mask)));
                else
                  add_res = (int16_t)((this->data_vs1 & (vs1_mask))) + (int16_t)((this->data_vs2 & (vs2_mask)));
              }

              if ((signed int)add_res > (signed int)((1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1)) - 1))
                temp_res = (1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1)) - 1;
              else if ((signed int)add_res < ((signed int)-1 * (1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1))))
                temp_res = -1 * (1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1));
              else
                temp_res = add_res;

              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else if (vd1_mask == 0xff)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VSADDU: {
          this->instr_name = (char *)"VSADDU";
          int add_res;
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read &&  data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00)
                  add_res = (uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op) + (uint8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                else if (vd1_mask == 0x00ff)
                  add_res = (uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op) + (uint8_t)((this->data_vs2 & (vs2_mask)));
                else
                  add_res = (uint16_t)(this->MY_VENUS_INS_PARAM.scalar_op) + (uint16_t)(this->data_vs2 & (vs2_mask));
              } else { // IVV
                if (vd1_mask == 0xff00)
                  add_res = (uint8_t)((this->data_vs1 & (vs1_mask)) >> 8) + (uint8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                else if (vd1_mask == 0x00ff)
                  add_res = (uint8_t)((this->data_vs1 & (vs1_mask))) + (uint8_t)((this->data_vs2 & (vs2_mask)));
                else
                  add_res = (uint16_t)((this->data_vs1 & (vs1_mask))) + (uint16_t)(this->data_vs2 & (vs2_mask));
              }
              if ((unsigned int)add_res > (unsigned int)((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1))
                temp_res = (1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1;
              else
                temp_res = add_res;

              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else if (vd1_mask == 0xff)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VRANGE: {
          this->instr_name = (char *)"VRANGE";
          unsigned int i = instr_pkt->locallane_cau_calc_cnt - 1;
            this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((uint16_t)(i%NrBankPerLane + i/NrBankPerLane*(NrLanes*NrBankPerLane) + localLaneID*NrBankPerLane)&vd1_mask);
          break;
        }
        case VRSUB: {
          this->instr_name = (char *)"VRSUB";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read &&  data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00)
                  temp_res = (int8_t)((this->data_vs2 & (vs2_mask)) >> 8) - (int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                else if (vd1_mask == 0x00ff)
                  temp_res = (int8_t)((this->data_vs2 & (vs2_mask))) - (int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                else
                  temp_res = (int16_t)((this->data_vs2 & (vs2_mask))) - (int16_t)(this->MY_VENUS_INS_PARAM.scalar_op);
              } else { // IVV
                if (vd1_mask == 0xff00)
                  temp_res = (int8_t)((this->data_vs2 & (vs2_mask)) >> 8) - (int8_t)((this->data_vs1 & (vs1_mask)) >> 8);
                else if (vd1_mask == 0x00ff)
                  temp_res = (int8_t)((this->data_vs2 & (vs2_mask))) - (int8_t)((this->data_vs1 & (vs1_mask)));
                else
                  temp_res = (int16_t)((this->data_vs2 & (vs2_mask))) - (int16_t)((this->data_vs1 & (vs1_mask)));
              }
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else if (vd1_mask == 0xff)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VSUB: {
          this->instr_name = (char *)"VSUB";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read &&  data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00)
                  temp_res = (int8_t)this->MY_VENUS_INS_PARAM.scalar_op - (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                else if (vd1_mask == 0x00ff)
                  temp_res = (int8_t)this->MY_VENUS_INS_PARAM.scalar_op - (int8_t)((this->data_vs2 & (vs2_mask)));
                else
                  temp_res = (int16_t)this->MY_VENUS_INS_PARAM.scalar_op - (int16_t)((this->data_vs2 & (vs2_mask)));
              } else { // IVV
                if (vd1_mask == 0xff00)
                  temp_res = (int8_t)((this->data_vs1 & (vs1_mask)) >> 8) - (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                else if (vd1_mask == 0x00ff)
                  temp_res = (int8_t)((this->data_vs1 & (vs1_mask))) - (int8_t)((this->data_vs2 & (vs2_mask)));
                else
                  temp_res = (int16_t)((this->data_vs1 & (vs1_mask))) - (int16_t)((this->data_vs2 & (vs2_mask)));
              }
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else if (vd1_mask == 0xff)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VSSUB: {
          this->instr_name = (char *)"VSSUB";
          int sub_res;
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read &&  data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX)
                if (vd1_mask == 0xff00)
                  sub_res = (int8_t)(this->MY_VENUS_INS_PARAM.scalar_op) - (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                else if (vd1_mask == 0x00ff)
                  sub_res = (int8_t)(this->MY_VENUS_INS_PARAM.scalar_op) - (int8_t)((this->data_vs2 & (vs2_mask)));
                else
                  sub_res = (int16_t)(this->MY_VENUS_INS_PARAM.scalar_op) - (int16_t)(this->data_vs2 & (vs2_mask));
              else // IVV
                if (vd1_mask == 0xff00)
                  sub_res = (int8_t)((this->data_vs1 & (vs1_mask)) >> 8) - (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                else if (vd1_mask == 0x00ff)
                  sub_res = (int8_t)((this->data_vs1 & (vs1_mask))) - (int8_t)((this->data_vs2 & (vs2_mask)));
                else
                  sub_res = (int16_t)((this->data_vs1 & (vs1_mask))) - (int16_t)((this->data_vs2 & (vs2_mask)));
              if ((int)sub_res > (int)((1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1)) - 1))
                temp_res = (1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1)) - 1;
              else if ((int)sub_res <  (int)-1 * (1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1)))
                temp_res = -1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1);
              else
                temp_res = sub_res;

              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else if (vd1_mask == 0xff)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VSSUBU: {
          this->instr_name = (char *)"VSSUBU";
          int sub_res;
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read &&  data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX)
                if (vd1_mask == 0xff00)
                  sub_res = (uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op) - (uint8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                else if (vd1_mask == 0x00ff)
                  sub_res = (uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op) - (uint8_t)((this->data_vs2 & (vs2_mask)));
                else
                  sub_res = (uint16_t)(this->MY_VENUS_INS_PARAM.scalar_op) - (uint16_t)(this->data_vs2 & (vs2_mask));
              else // IVV
                  if (vd1_mask == 0xff00)
                sub_res = (uint8_t)((this->data_vs1 & (vs1_mask)) >> 8) - (uint8_t)((this->data_vs2 & (vs2_mask)) >> 8);
              else if (vd1_mask == 0x00ff)
                sub_res = (uint8_t)((this->data_vs1 & (vs1_mask))) - (uint8_t)((this->data_vs2 & (vs2_mask)));
              else
                sub_res = (uint16_t)((this->data_vs1 & (vs1_mask))) - (uint16_t)((this->data_vs2 & (vs2_mask)));

              if ((signed int)sub_res < 0)
                temp_res = 0;
              else
                temp_res = sub_res;

              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else if (vd1_mask == 0xff)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VMUL: {
          this->instr_name = (char *)"VMUL";
            int mul_res;
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read &&  data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00) {
                  mul_res = (signed int8_t)((this->data_vs2 & (vs2_mask)) >> 8) * (signed int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) { mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res; mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  temp_res = mul_res &          ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else if (vd1_mask == 0xff) {
                  mul_res = (signed int8_t)(this->data_vs2 & (vs2_mask)) * (signed int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) { mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res; mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  temp_res = mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else {
                  mul_res = (signed int16_t)(this->data_vs2 & (vs2_mask)) * (signed int16_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT16_MAX ? INT16_MAX : mul_res;
                    mul_res = mul_res < INT16_MIN ? INT16_MIN : mul_res;
                  }
                  temp_res = mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                }
              } else { // IVV
                if (vd1_mask == 0xff00) {
                  mul_res = (signed int8_t)((this->data_vs2 & (vs2_mask)) >> 8) * (signed int8_t)((this->data_vs1 & (vs1_mask)) >> 8);
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res;
                    mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  temp_res = mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else if (vd1_mask == 0xff) {
                  mul_res = (signed int8_t)((this->data_vs2 & (vs2_mask))) * (signed int8_t)((this->data_vs1 & (vs1_mask)));
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res;
                    mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  temp_res = mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else {
                  mul_res = (signed int16_t)((this->data_vs2 & (vs2_mask))) * (signed int16_t)((this->data_vs1 & (vs1_mask)));
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT16_MAX ? INT16_MAX : mul_res;
                    mul_res = mul_res < INT16_MIN ? INT16_MIN : mul_res;
                  }
                  temp_res = mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                }
              }
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VMULH: {
          this->instr_name = (char *)"VMULH";
            int mul_res;
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read &&  data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00) {
                  mul_res = (signed int8_t)((this->data_vs2 & (vs2_mask)) >> 8) * (signed int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                } else if (vd1_mask == 0xff) {
                  mul_res = (signed int8_t)((this->data_vs2 & (vs2_mask))) * (signed int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                } else {
                  mul_res = (signed int16_t)((this->data_vs2 & (vs2_mask))) * (signed int16_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                }
              } else { // IVV
                if (vd1_mask == 0xff00) {
                  mul_res = (signed int8_t)((this->data_vs2 & (vs2_mask)) >> 8) * (signed int8_t)((this->data_vs1 & (vs1_mask)) >> 8);
                } else if (vd1_mask == 0xff) {
                  mul_res = (signed int8_t)(this->data_vs2 & (vs2_mask)) * (signed int8_t)(this->data_vs1 & (vs1_mask));
                } else {
                  mul_res = (signed int16_t)(this->data_vs2 & (vs2_mask)) * (signed int16_t)(this->data_vs1 & (vs1_mask));
                }
              }
              temp_res = (mul_res << this->MY_VENUS_INS_PARAM.vfu_shamt) >> ((this->MY_VENUS_INS_PARAM.vew + 1) << 3);
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VMULHU: {
          this->instr_name = (char *)"VMULHU";
          int mul_res;
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read &&  data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00) {
                  mul_res = (uint8_t)((this->data_vs2 & (vs2_mask)) >> 8) * (uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                } else if (vd1_mask == 0xff) {
                  mul_res = (uint8_t)((this->data_vs2 & (vs2_mask))) * (uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                } else {
                  mul_res = (uint16_t)((this->data_vs2 & (vs2_mask))) * (uint16_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                }
              } else { // IVV
                if (vd1_mask == 0xff00) {
                  mul_res = (uint8_t)((this->data_vs2 & (vs2_mask)) >> 8) * (uint8_t)((this->data_vs1 & (vs1_mask)) >> 8);
                } else if (vd1_mask == 0xff) {
                  mul_res = (uint8_t)(this->data_vs2 & (vs2_mask)) * (uint8_t)(this->data_vs1 & (vs1_mask));
                } else {
                  mul_res = (uint16_t)(this->data_vs2 & (vs2_mask)) * (uint16_t)(this->data_vs1 & (vs1_mask));
                }
              }
              temp_res = (mul_res << this->MY_VENUS_INS_PARAM.vfu_shamt) >> ((this->MY_VENUS_INS_PARAM.vew + 1) << 3);
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VMULHSU: {
          this->instr_name = (char *)"VMULHSU";
          int mul_res;
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00) {
                  mul_res = (int8_t)((this->data_vs2 & (vs2_mask)) >> 8) * (uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                } else if (vd1_mask == 0xff) {
                  mul_res = (int8_t)((this->data_vs2 & (vs2_mask))) * (uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                } else {
                  mul_res = (int16_t)((this->data_vs2 & (vs2_mask))) * (uint16_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                }
              } else { // IVV
                if (vd1_mask == 0xff00) {
                  mul_res = (int8_t)((this->data_vs2 & (vs2_mask)) >> 8) * (uint8_t)((this->data_vs1 & (vs1_mask)) >> 8);
                } else if (vd1_mask == 0xff) {
                  mul_res = (int8_t)(this->data_vs2 & (vs2_mask)) * (uint8_t)(this->data_vs1 & (vs1_mask));
                } else {
                  mul_res = (int16_t)(this->data_vs2 & (vs2_mask)) * (uint16_t)(this->data_vs1 & (vs1_mask));
                }
              }
              temp_res = (mul_res << this->MY_VENUS_INS_PARAM.vfu_shamt) >> ((this->MY_VENUS_INS_PARAM.vew + 1) << 3);
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VMULADD: {
          this->instr_name = (char *)"VMULADD";
          int mul_res;
          int add_res;
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read &&  data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00) {
                  mul_res = (signed int8_t)((this->data_vs2 & (vs2_mask)) >> 8) * (signed int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) { mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res; mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  add_res = (signed int8_t)(mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1));
                  add_res = (signed int8_t)add_res + (signed int8_t)((this->data_vd2 & (vd2_mask)) >> 8);
                  if (this->MY_VENUS_INS_PARAM.saturate_post_adder == 1) {
                    add_res = add_res > INT8_MAX ? INT8_MAX : add_res;
                    add_res = add_res < INT8_MIN ? INT8_MIN : add_res;
                  }
                  temp_res = (signed int8_t)add_res;
                } else if (vd1_mask == 0xff) {
                  mul_res = (signed int8_t)((this->data_vs2 & (vs2_mask))) * (signed int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res;
                    mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  add_res = (signed int8_t)(mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1));
                  add_res = (signed int8_t)add_res + (signed int8_t)(this->data_vd2 & (vd2_mask));
                  if (this->MY_VENUS_INS_PARAM.saturate_post_adder == 1) {
                    add_res = add_res > INT8_MAX ? INT8_MAX : add_res;
                    add_res = add_res < INT8_MIN ? INT8_MIN : add_res;
                  }
                  temp_res = (signed int8_t)add_res;
                } else {
                  mul_res = (signed int16_t)(this->data_vs2 & (vs2_mask)) * (signed int16_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT16_MAX ? INT16_MAX : mul_res;
                    mul_res = mul_res < INT16_MIN ? INT16_MIN : mul_res;
                  }
                  add_res = (signed int16_t)(mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1));
                  add_res = (signed int16_t)add_res + (signed int16_t)(this->data_vd2 & (vd2_mask));
                  if (this->MY_VENUS_INS_PARAM.saturate_post_adder == 1) {
                    add_res = add_res > INT16_MAX ? INT16_MAX : add_res;
                    add_res = add_res < INT16_MIN ? INT16_MIN : add_res;
                  }
                  temp_res = (signed int16_t)add_res;
                }
              } else { // IVV
                if (vd1_mask == 0xff00) {
                  mul_res = (signed int8_t)((this->data_vs2 & (vs2_mask)) >> 8) * (signed int8_t)((this->data_vs1 & (vs1_mask)) >> 8);
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res;
                    mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  add_res = (signed int8_t)(mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1));
                  add_res = (signed int8_t)add_res + (signed int8_t)((this->data_vd2 & (vd2_mask)) >> 8);
                  if (this->MY_VENUS_INS_PARAM.saturate_post_adder == 1) {
                    add_res = add_res > INT8_MAX ? INT8_MAX : add_res;
                    add_res = add_res < INT8_MIN ? INT8_MIN : add_res;
                  }
                  temp_res = (signed int8_t)add_res;
                } else if (vd1_mask == 0xff) {
                  mul_res = (signed int8_t)((this->data_vs2 & (vs2_mask))) * (signed int8_t)((this->data_vs1 & (vs1_mask)));
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res;
                    mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  add_res = (signed int8_t)(mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1));
                  add_res = (signed int8_t)add_res + (signed int8_t)(this->data_vd2 & (vd2_mask));
                  if (this->MY_VENUS_INS_PARAM.saturate_post_adder == 1) {
                    add_res = add_res > INT8_MAX ? INT8_MAX : add_res;
                    add_res = add_res < INT8_MIN ? INT8_MIN : add_res;
                  }
                  temp_res = (signed int8_t)add_res;
                } else {
                  mul_res = (signed int16_t)((this->data_vs2 & (vs2_mask))) * (signed int16_t)((this->data_vs1 & (vs1_mask)));
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT16_MAX ? INT16_MAX : mul_res;
                    mul_res = mul_res < INT16_MIN ? INT16_MIN : mul_res;
                  }
                  add_res = (signed int16_t)(mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1));
                  add_res = (signed int16_t)add_res + (signed int16_t)(this->data_vd2 & (vd2_mask));
                  if (this->MY_VENUS_INS_PARAM.saturate_post_adder == 1) {
                    add_res = add_res > INT16_MAX ? INT16_MAX : add_res;
                    add_res = add_res < INT16_MIN ? INT16_MIN : add_res;
                  }
                  temp_res = (signed int16_t)add_res;
                }
              }
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VMULSUB: {
          this->instr_name = (char *)"VMULSUB";
          int mul_res;
          int sub_res;
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read &&  data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00) {
                  mul_res = (signed int8_t)((this->data_vs2 & (vs2_mask)) >> 8) * (signed int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res;
                    mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  sub_res = (signed int8_t)(mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1));
                  sub_res = (signed int8_t)sub_res - (signed int8_t)((this->data_vd2 & (vd2_mask)) >> 8);
                  if (this->MY_VENUS_INS_PARAM.saturate_post_adder == 1) {
                    sub_res = sub_res > INT8_MAX ? INT8_MAX : sub_res;
                    sub_res = sub_res < INT8_MIN ? INT8_MIN : sub_res;
                  }
                  temp_res = (signed int8_t)sub_res;
                } else if (vd1_mask == 0xff) {
                  mul_res = (signed int8_t)(this->data_vs2 & (vs2_mask)) * (signed int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res;
                    mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  sub_res = (signed int8_t)(mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1));
                  sub_res = (signed int8_t)sub_res - (signed int8_t)(this->data_vd2 & (vd2_mask));
                  if (this->MY_VENUS_INS_PARAM.saturate_post_adder == 1) {
                    sub_res = sub_res > INT8_MAX ? INT8_MAX : sub_res;
                    sub_res = sub_res < INT8_MIN ? INT8_MIN : sub_res;
                  }
                  temp_res = (signed int8_t)sub_res;
                } else {
                  mul_res = (signed int16_t)(this->data_vs2 & (vs2_mask)) * (signed int16_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT16_MAX ? INT16_MAX : mul_res;
                    mul_res = mul_res < INT16_MIN ? INT16_MIN : mul_res;
                  }
                  sub_res = (signed int16_t)(mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1));
                  sub_res = (signed int16_t)sub_res - (signed int16_t)(this->data_vd2 & (vd2_mask));
                  if (this->MY_VENUS_INS_PARAM.saturate_post_adder == 1) {
                    sub_res = sub_res > INT16_MAX ? INT16_MAX : sub_res;
                    sub_res = sub_res < INT16_MIN ? INT16_MIN : sub_res;
                  }
                  temp_res = (signed int16_t)sub_res;
                }
              } else { // IVV
                if (vd1_mask == 0xff00) {
                  mul_res = (signed int8_t)((this->data_vs2 & (vs2_mask)) >> 8) * (signed int8_t)((this->data_vs1 & (vs1_mask)) >> 8);
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res;
                    mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  sub_res = (signed int8_t)(mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1));
                  sub_res = (signed int8_t)sub_res - (signed int8_t)((this->data_vd2 & (vd2_mask)) >> 8);
                  if (this->MY_VENUS_INS_PARAM.saturate_post_adder == 1) {
                    sub_res = sub_res > INT8_MAX ? INT8_MAX : sub_res;
                    sub_res = sub_res < INT8_MIN ? INT8_MIN : sub_res;
                  }
                  temp_res = (signed int8_t)sub_res;
                } else if (vd1_mask == 0xff) {
                  mul_res = (signed int8_t)((this->data_vs2 & (vs2_mask))) * (signed int8_t)((this->data_vs1 & (vs1_mask)));
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res;
                    mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  sub_res = (signed int8_t)(mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1));
                  sub_res = (signed int8_t)sub_res - (signed int8_t)(this->data_vd2 & (vd2_mask));
                  if (this->MY_VENUS_INS_PARAM.saturate_post_adder == 1) {
                    sub_res = sub_res > INT8_MAX ? INT8_MAX : sub_res;
                    sub_res = sub_res < INT8_MIN ? INT8_MIN : sub_res;
                  }
                  temp_res = (signed int8_t)sub_res;
                } else {
                  mul_res = (signed int16_t)((this->data_vs2 & (vs2_mask))) * (signed int16_t)((this->data_vs1 & (vs1_mask)));
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT16_MAX ? INT16_MAX : mul_res;
                    mul_res = mul_res < INT16_MIN ? INT16_MIN : mul_res;
                  }
                  sub_res = (signed int16_t)(mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1));
                  sub_res = (signed int16_t)sub_res - (signed int16_t)(this->data_vd2 & (vd2_mask));
                  if (this->MY_VENUS_INS_PARAM.saturate_post_adder == 1) {
                    sub_res = sub_res > INT16_MAX ? INT16_MAX : sub_res;
                    sub_res = sub_res < INT16_MIN ? INT16_MIN : sub_res;
                  }
                  temp_res = (signed int16_t)sub_res;
                }
              }
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VADDMUL: {
          this->instr_name = (char *)"VADDMUL";
          long add_res;
          long mul_res;
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read &&  data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00) {
                  add_res = ((signed int8_t)this->MY_VENUS_INS_PARAM.scalar_op + (signed int8_t)((this->data_vs2 & (vs2_mask)) >> 8));
                  if (this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                    add_res = add_res > INT8_MAX ? INT8_MAX : add_res;
                    add_res = add_res < INT8_MIN ? INT8_MIN : add_res;
                  }
                  mul_res = (signed int8_t)((this->data_vd2 & (vd2_mask)) >> 8) * (signed int32_t)add_res;
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res;
                    mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  temp_res = mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else if (vd1_mask == 0xff) {
                  add_res = ((signed int8_t)this->MY_VENUS_INS_PARAM.scalar_op + (signed int8_t)(this->data_vs2 & (vs2_mask)));
                  if (this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                    add_res = add_res > INT8_MAX ? INT8_MAX : add_res;
                    add_res = add_res < INT8_MIN ? INT8_MIN : add_res;
                  }
                  mul_res = (signed int8_t)(this->data_vd2 & (vd2_mask)) * (signed int32_t)add_res;
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res;
                    mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  temp_res = mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else {
                  add_res = ((signed int16_t)this->MY_VENUS_INS_PARAM.scalar_op + (signed int16_t)(this->data_vs2 & (vs2_mask)));
                  if (this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                    add_res = add_res > INT16_MAX ? INT16_MAX : add_res;
                    add_res = add_res < INT16_MIN ? INT16_MIN : add_res;
                  }
                  mul_res = (signed int16_t)(this->data_vd2 & (vd2_mask)) * (signed int64_t)add_res;
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT16_MAX ? INT16_MAX : mul_res;
                    mul_res = mul_res < INT16_MIN ? INT16_MIN : mul_res;
                  }
                  temp_res = mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                }
              } else { // IVV
                if (vd1_mask == 0xff00) {
                  add_res = ((signed int8_t)((this->data_vs1 & (vs1_mask)) >> 8) + (signed int8_t)((this->data_vs2 & (vs2_mask)) >> 8));
                  if (this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                    add_res = add_res > INT8_MAX ? INT8_MAX : add_res;
                    add_res = add_res < INT8_MIN ? INT8_MIN : add_res;
                  }
                  mul_res = (signed int8_t)((this->data_vd2 & (vd2_mask)) >> 8) * (signed int32_t)add_res;
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res;
                    mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  temp_res = mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else if (vd1_mask == 0xff) {
                  add_res = ((signed int8_t)(this->data_vs1 & (vs1_mask)) + (signed int8_t)(this->data_vs2 & (vs2_mask)));
                  if (this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                    add_res = add_res > INT8_MAX ? INT8_MAX : add_res;
                    add_res = add_res < INT8_MIN ? INT8_MIN : add_res;
                  }
                  mul_res = (signed int8_t)(this->data_vd2 & (vd2_mask)) * (signed int32_t)add_res;
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res;
                    mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  temp_res = mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else {
                  add_res = ((signed int16_t)(this->data_vs1 & (vs1_mask)) + (signed int16_t)(this->data_vs2 & (vs2_mask)));
                  if (this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                    add_res = add_res > INT16_MAX ? INT16_MAX : add_res;
                    add_res = add_res < INT16_MIN ? INT16_MIN : add_res;
                  }
                  mul_res = (signed int16_t)(this->data_vd2 & (vd2_mask)) * (signed int64_t)add_res;
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT16_MAX ? INT16_MAX : mul_res;
                    mul_res = mul_res < INT16_MIN ? INT16_MIN : mul_res;
                  }
                  temp_res = mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                }
              }
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VSUBMUL: {
          this->instr_name = (char *)"VSUBMUL";
          long sub_res;
          long mul_res;
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read &&  data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00) {
                  sub_res = ((signed int8_t)this->MY_VENUS_INS_PARAM.scalar_op - (signed int8_t)((this->data_vs2 & (vs2_mask)) >> 8));
                  if (this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                    sub_res = sub_res > INT8_MAX ? INT8_MAX : sub_res;
                    sub_res = sub_res < INT8_MIN ? INT8_MIN : sub_res;
                  }
                  mul_res = (signed int8_t)((this->data_vd2 & (vd2_mask)) >> 8) * (signed int32_t)sub_res;
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res;
                    mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  temp_res = mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else if (vd1_mask == 0xff) {
                  sub_res = ((signed int8_t)this->MY_VENUS_INS_PARAM.scalar_op - (signed int8_t)(this->data_vs2 & (vs2_mask)));
                  if (this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                    sub_res = sub_res > INT8_MAX ? INT8_MAX : sub_res;
                    sub_res = sub_res < INT8_MIN ? INT8_MIN : sub_res;
                  }
                  mul_res = (signed int8_t)(this->data_vd2 & (vd2_mask)) * (signed int32_t)sub_res;
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res;
                    mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  temp_res = mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else {
                  sub_res = ((signed int16_t)this->MY_VENUS_INS_PARAM.scalar_op - (signed int16_t)(this->data_vs2 & (vs2_mask)));
                  if (this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                    sub_res = sub_res > INT16_MAX ? INT16_MAX : sub_res;
                    sub_res = sub_res < INT16_MIN ? INT16_MIN : sub_res;
                  }
                  mul_res = (signed int16_t)(this->data_vd2 & (vd2_mask)) * (signed int64_t)sub_res;
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT16_MAX ? INT16_MAX : mul_res;
                    mul_res = mul_res < INT16_MIN ? INT16_MIN : mul_res;
                  }
                  temp_res = mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                }
              } else { // IVV
                if (vd1_mask == 0xff00) {
                  sub_res = ((signed int8_t)((this->data_vs1 & (vs1_mask)) >> 8) - (signed int8_t)((this->data_vs2 & (vs2_mask)) >> 8));
                  if (this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                    sub_res = sub_res > INT8_MAX ? INT8_MAX : sub_res;
                    sub_res = sub_res < INT8_MIN ? INT8_MIN : sub_res;
                  }
                  mul_res = (signed int8_t)((this->data_vd2 & (vd2_mask)) >> 8) * (signed int32_t)sub_res;
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res;
                    mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  temp_res = mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else if (vd1_mask == 0xff) {
                  sub_res = ((signed int8_t)(this->data_vs1 & (vs1_mask)) - (signed int8_t)(this->data_vs2 & (vs2_mask)));
                  if (this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                    sub_res = sub_res > INT8_MAX ? INT8_MAX : sub_res;
                    sub_res = sub_res < INT8_MIN ? INT8_MIN : sub_res;
                  }
                  mul_res = (signed int8_t)(this->data_vd2 & (vd2_mask)) * (signed int32_t)sub_res;
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT8_MAX ? INT8_MAX : mul_res;
                    mul_res = mul_res < INT8_MIN ? INT8_MIN : mul_res;
                  }
                  temp_res = mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else {
                  sub_res = ((signed int16_t)(this->data_vs1 & (vs1_mask)) - (signed int16_t)(this->data_vs2 & (vs2_mask)));
                  if (this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                    sub_res = sub_res > INT16_MAX ? INT16_MAX : sub_res;
                    sub_res = sub_res < INT16_MIN ? INT16_MIN : sub_res;
                  }
                  mul_res = (signed int16_t)(this->data_vd2 & (vd2_mask)) * (signed int64_t)sub_res;
                  mul_res = mul_res >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                  if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                    mul_res = mul_res > INT16_MAX ? INT16_MAX : mul_res;
                    mul_res = mul_res < INT16_MIN ? INT16_MIN : mul_res;
                  }
                  temp_res = mul_res & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                }
              }
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }

        case VMIN: {
          this->instr_name = (char *)"VMIN";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
               if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                 if (vd1_mask == 0xff00) {
                   int8_t a = (int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                   int8_t b = (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                   temp_res = (a < b) ? a : b;
                 } else if (vd1_mask == 0x00ff) {
                   int8_t a = (int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                   int8_t b = (int8_t)(this->data_vs2 & (vs2_mask));
                   temp_res = (a < b) ? a : b;
                 } else {
                   int16_t a = (int16_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                   int16_t b = (int16_t)(this->data_vs2 & (vs2_mask));
                   temp_res = (a < b) ? a : b;
                 }
               } else { // IVV
                 if (vd1_mask == 0xff00) {
                   int8_t a = (int8_t)((this->data_vs1 & (vs1_mask)) >> 8);
                   int8_t b = (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                   temp_res = (a < b) ? a : b;
                 } else if (vd1_mask == 0x00ff) {
                   int8_t a = (int8_t)(this->data_vs1 & (vs1_mask));
                   int8_t b = (int8_t)(this->data_vs2 & (vs2_mask));
                   temp_res = (a < b) ? a : b;
                 } else {
                   int16_t a = (int16_t)(this->data_vs1 & (vs1_mask));
                   int16_t b = (int16_t)(this->data_vs2 & (vs2_mask));
                   temp_res = (a < b) ? a : b;
                 }
               }
               if (vd1_mask == 0xff00)
                 this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
               else
                 this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }

        case VMAX: {
          this->instr_name = (char *)"VMAX";
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
               if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                 if (vd1_mask == 0xff00) {
                   int8_t a = (int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                   int8_t b = (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                   temp_res = (a < b) ? b : a;
                 } else if (vd1_mask == 0x00ff) {
                   int8_t a = (int8_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                   int8_t b = (int8_t)(this->data_vs2 & (vs2_mask));
                   temp_res = (a < b) ? b : a;
                 } else {
                   int16_t a = (int16_t)(this->MY_VENUS_INS_PARAM.scalar_op);
                   int16_t b = (int16_t)(this->data_vs2 & (vs2_mask));
                   temp_res = (a < b) ? b : a;
                 }
               } else { // IVV
                 if (vd1_mask == 0xff00) {
                   int8_t a = (int8_t)((this->data_vs1 & (vs1_mask)) >> 8);
                   int8_t b = (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                   temp_res = (a < b) ? b : a;
                 } else if (vd1_mask == 0x00ff) {
                   int8_t a = (int8_t)(this->data_vs1 & (vs1_mask));
                   int8_t b = (int8_t)(this->data_vs2 & (vs2_mask));
                   temp_res = (a < b) ? b : a;
                 } else {
                   int16_t a = (int16_t)(this->data_vs1 & (vs1_mask));
                   int16_t b = (int16_t)(this->data_vs2 & (vs2_mask));
                   temp_res = (a < b) ? b : a;
                 }
               }
               if (vd1_mask == 0xff00)
                 this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
               else
                 this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }

        case VCMXMUL: {
          this->instr_name = (char *)"VCMXMUL";
          long AsubB;
          long AaddB;
          long CsubD;
          long CmulAsubB;
          long DmulAaddB;
          long BmulCsubD;
          long temp_CmulAsubB;
          long temp_DmulAaddB;
          long temp_BmulCsubD;


            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              if (vd1_mask == 0xff00) {
                AsubB = (signed int8_t)((this->data_vs1 & (vs1_mask)) >> 8) - (signed int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                // if(this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                //   AsubB = AsubB > INT8_MAX ? INT8_MAX : AsubB;
                //   AsubB = AsubB < INT8_MIN ? INT8_MIN : AsubB;
                // }
                AaddB = (signed int8_t)((this->data_vs1 & (vs1_mask)) >> 8) + (signed int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                // if(this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                //   AaddB = AaddB > INT8_MAX ? INT8_MAX : AaddB;
                //   AaddB = AaddB < INT8_MIN ? INT8_MIN : AaddB;
                // }
                CsubD = (signed int8_t)((this->data_vd2 & (vd2_mask)) >> 8) - (signed int8_t)((this->data_vd1 & (vd1_mask)) >> 8);
                // if(this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                //   CsubD = CsubD > INT8_MAX ? INT8_MAX : CsubD;
                //   CsubD = CsubD < INT8_MIN ? INT8_MIN : CsubD;
                // }

                CmulAsubB = (signed int8_t)((this->data_vd2 & (vd2_mask)) >> 8) * (signed int32_t)AsubB;
                CmulAsubB = CmulAsubB >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                  CmulAsubB = CmulAsubB > INT8_MAX ? INT8_MAX : CmulAsubB;
                  CmulAsubB = CmulAsubB < INT8_MIN ? INT8_MIN : CmulAsubB;
                }
                temp_CmulAsubB = CmulAsubB & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                DmulAaddB = (signed int8_t)((this->data_vd1 & (vd1_mask)) >> 8) * (signed int32_t)AaddB;
                DmulAaddB = DmulAaddB >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                  DmulAaddB = DmulAaddB > INT8_MAX ? INT8_MAX : DmulAaddB;
                  DmulAaddB = DmulAaddB < INT8_MIN ? INT8_MIN : DmulAaddB;
                }
                temp_DmulAaddB = DmulAaddB & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                BmulCsubD = (signed int8_t)((this->data_vs2 & (vs2_mask)) >> 8) * (signed int32_t)CsubD;
                BmulCsubD = BmulCsubD >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                  BmulCsubD = BmulCsubD > INT8_MAX ? INT8_MAX : BmulCsubD;
                  BmulCsubD = BmulCsubD < INT8_MIN ? INT8_MIN : BmulCsubD;
                }
                temp_BmulCsubD = BmulCsubD & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);

                int sum_real = (signed int8_t)temp_CmulAsubB + (signed int8_t)temp_BmulCsubD;
                if (true) {
                  sum_real = sum_real > INT8_MAX ? INT8_MAX : sum_real;
                  sum_real = sum_real < INT8_MIN ? INT8_MIN : sum_real;
                }
                int sum_imag = (signed int8_t)temp_DmulAaddB + (signed int8_t)temp_BmulCsubD;
                if (true) {
                  sum_imag = sum_imag > INT8_MAX ? INT8_MAX : sum_imag;
                  sum_imag = sum_imag < INT8_MIN ? INT8_MIN : sum_imag;
                }

                // if ((signed int)sum_real > (signed int)((1 <<
                // (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1)) - 1))
                //   sum_real = (1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1))
                //   - 1;
                // else if ((signed int)sum_real < (signed int)-1 * (1 <<
                // (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1)))
                //   sum_real = 1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1);
                // else
                //   sum_real = sum_real;

                // if ((signed int)sum_imag > (signed int)((1 <<
                // (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1)) - 1))
                //   sum_imag = (1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1))
                //   - 1;
                // else if ((signed int)sum_imag < (signed int)-1 * (1 <<
                // (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1)))
                //   sum_imag = 1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1);
                // else
                //   sum_imag = sum_imag;

                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((sum_real << 8) & vd1_mask);
                this->data_vd2 = (this->data_vd2 & (~vd2_mask)) + ((sum_imag << 8) & vd2_mask);
              } else if (vd1_mask == 0xff) {
                // int A = (this->data_vs1 & (vs1_mask));
                // int B = (this->data_vs2 & (vs2_mask));
                // int C = (this->data_vd2 & (vd2_mask));
                // int D = (this->data_vd1 & (vd1_mask));

                AsubB = (signed int8_t)((this->data_vs1 & (vs1_mask))) - (signed int8_t)((this->data_vs2 & (vs2_mask)));
                // if(this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                //   AsubB = AsubB > INT8_MAX ? INT8_MAX : AsubB;
                //   AsubB = AsubB < INT8_MIN ? INT8_MIN : AsubB;
                // }
                AaddB = (signed int8_t)((this->data_vs1 & (vs1_mask))) + (signed int8_t)((this->data_vs2 & (vs2_mask)));
                // if(this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                //   AaddB = AaddB > INT8_MAX ? INT8_MAX : AaddB;
                //   AaddB = AaddB < INT8_MIN ? INT8_MIN : AaddB;
                // }
                CsubD = (signed int8_t)((this->data_vd2 & (vd2_mask))) - (signed int8_t)((this->data_vd1 & (vd1_mask)));
                // if(this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                //   CsubD = CsubD > INT8_MAX ? INT8_MAX : CsubD;
                //   CsubD = CsubD < INT8_MIN ? INT8_MIN : CsubD;
                // }

                CmulAsubB = (signed int8_t)((this->data_vd2 & (vd2_mask))) * (signed int32_t)AsubB;
                CmulAsubB = CmulAsubB >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                  CmulAsubB = CmulAsubB > INT8_MAX ? INT8_MAX : CmulAsubB;
                  CmulAsubB = CmulAsubB < INT8_MIN ? INT8_MIN : CmulAsubB;
                }
                temp_CmulAsubB = CmulAsubB & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                DmulAaddB = (signed int8_t)((this->data_vd1 & (vd1_mask))) * (signed int32_t)AaddB;
                DmulAaddB = DmulAaddB >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                  DmulAaddB = DmulAaddB > INT8_MAX ? INT8_MAX : DmulAaddB;
                  DmulAaddB = DmulAaddB < INT8_MIN ? INT8_MIN : DmulAaddB;
                }
                temp_DmulAaddB = DmulAaddB & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                BmulCsubD = (signed int8_t)((this->data_vs2 & (vs2_mask))) * (signed int32_t)CsubD;
                BmulCsubD = BmulCsubD >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                  BmulCsubD = BmulCsubD > INT8_MAX ? INT8_MAX : BmulCsubD;
                  BmulCsubD = BmulCsubD < INT8_MIN ? INT8_MIN : BmulCsubD;
                }
                temp_BmulCsubD = BmulCsubD & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);

                int sum_real = (signed int8_t)temp_CmulAsubB + (signed int8_t)temp_BmulCsubD;
                if (true) {
                  sum_real = sum_real > INT8_MAX ? INT8_MAX : sum_real;
                  sum_real = sum_real < INT8_MIN ? INT8_MIN : sum_real;
                }
                int sum_imag = (signed int8_t)temp_DmulAaddB + (signed int8_t)temp_BmulCsubD;
                if (true) {
                  sum_imag = sum_imag > INT8_MAX ? INT8_MAX : sum_imag;
                  sum_imag = sum_imag < INT8_MIN ? INT8_MIN : sum_imag;
                }

                // if ((signed int)sum_real > (signed int)((1 <<
                // (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1)) - 1))
                //   sum_real = (1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1))
                //   - 1;
                // else if ((signed int)sum_real < (signed int)-1 * (1 <<
                // (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1)))
                //   sum_real = 1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1);
                // else
                //   sum_real = sum_real;

                // if ((signed int)sum_imag > (signed int)((1 <<
                // (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1)) - 1))
                //   sum_imag = (1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1))
                //   - 1;
                // else if ((signed int)sum_imag < (signed int)-1 * (1 <<
                // (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1)))
                //   sum_imag = 1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1);
                // else
                //   sum_imag = sum_imag;

                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((sum_real)&vd1_mask);
                this->data_vd2 = (this->data_vd2 & (~vd2_mask)) + ((sum_imag)&vd2_mask);
              } else {
                // int A = (this->data_vs1 & (vs1_mask));
                // int B = (this->data_vs2 & (vs2_mask));
                // int C = (this->data_vd2 & (vd2_mask));
                // int D = (this->data_vd1 & (vd1_mask));

                // Tips/Warning: It is assumed that no overflow will occur at 16bit
                AsubB = (signed int16_t)((this->data_vs1 & (vs1_mask))) - (signed int16_t)((this->data_vs2 & (vs2_mask)));
                // if(this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                //   AsubB = AsubB > INT16_MAX ? INT16_MAX : AsubB;
                //   AsubB = AsubB < INT16_MIN ? INT16_MIN : AsubB;
                // }
                AaddB = (signed int16_t)((this->data_vs1 & (vs1_mask))) + (signed int16_t)((this->data_vs2 & (vs2_mask)));
                // if(this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                //   AaddB = AaddB > INT16_MAX ? INT16_MAX : AaddB;
                //   AaddB = AaddB < INT16_MIN ? INT16_MIN : AaddB;
                // }
                CsubD = (signed int16_t)((this->data_vd2 & (vd2_mask))) - (signed int16_t)((this->data_vd1 & (vd1_mask)));
                // if(this->MY_VENUS_INS_PARAM.saturate_pre_adder == 1) {
                //   CsubD = CsubD > INT16_MAX ? INT16_MAX : CsubD;
                //   CsubD = CsubD < INT16_MIN ? INT16_MIN : CsubD;
                // }

                CmulAsubB = (signed int16_t)((this->data_vd2 & (vd2_mask))) * (signed int64_t)AsubB;
                CmulAsubB = CmulAsubB >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                  CmulAsubB = CmulAsubB > INT16_MAX ? INT16_MAX : CmulAsubB;
                  CmulAsubB = CmulAsubB < INT16_MIN ? INT16_MIN : CmulAsubB;
                }
                temp_CmulAsubB = CmulAsubB & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                DmulAaddB = (signed int16_t)((this->data_vd1 & (vd1_mask))) * (signed int64_t)AaddB;
                DmulAaddB = DmulAaddB >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                  DmulAaddB = DmulAaddB > INT16_MAX ? INT16_MAX : DmulAaddB;
                  DmulAaddB = DmulAaddB < INT16_MIN ? INT16_MIN : DmulAaddB;
                }
                temp_DmulAaddB = DmulAaddB & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                BmulCsubD = (signed int16_t)((this->data_vs2 & (vs2_mask))) * (signed int64_t)CsubD;
                BmulCsubD = BmulCsubD >> this->MY_VENUS_INS_PARAM.vfu_shamt;
                if (this->MY_VENUS_INS_PARAM.saturate_multiplier == 1) {
                  BmulCsubD = BmulCsubD > INT16_MAX ? INT16_MAX : BmulCsubD;
                  BmulCsubD = BmulCsubD < INT16_MIN ? INT16_MIN : BmulCsubD;
                }
                temp_BmulCsubD = BmulCsubD & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);

                int sum_real = (signed int16_t)temp_CmulAsubB + (signed int16_t)temp_BmulCsubD;
                if (true) {
                  sum_real = sum_real > INT16_MAX ? INT16_MAX : sum_real;
                  sum_real = sum_real < INT16_MIN ? INT16_MIN : sum_real;
                }
                int sum_imag = (signed int16_t)temp_DmulAaddB + (signed int16_t)temp_BmulCsubD;
                if (true) {
                  sum_imag = sum_imag > INT16_MAX ? INT16_MAX : sum_imag;
                  sum_imag = sum_imag < INT16_MIN ? INT16_MIN : sum_imag;
                }

                // if ((signed int)sum_real > (signed int)((1 <<
                // (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1)) - 1))
                //   sum_real = (1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1))
                //   - 1;
                // else if ((signed int)sum_real < (signed int)-1 * (1 <<
                // (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1)))
                //   sum_real = 1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1);
                // else
                //   sum_real = sum_real;

                // if ((signed int)sum_imag > (signed int)((1 <<
                // (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1)) - 1))
                //   sum_imag = (1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1))
                //   - 1;
                // else if ((signed int)sum_imag < (signed int)-1 * (1 <<
                // (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1)))
                //   sum_imag = 1 << (((this->MY_VENUS_INS_PARAM.vew + 1) << 3) - 1);
                // else
                //   sum_imag = sum_imag;

                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (sum_real & vd1_mask);
                this->data_vd2 = (this->data_vd2 & (~vd2_mask)) + (sum_imag & vd2_mask);
              }
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs1 & (vd1_mask));
              this->data_vd2 = (this->data_vd2 & (~vd2_mask)) + (this->data_vs2 & (vd2_mask));
            }
          break;
        }

        case VDIV: {
          this->instr_name = (char *)"VDIV";
            int div_res;
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read &&  data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00) {
                  if ((int8_t)((this->data_vs2 & (vs2_mask)) >> 8) != 0) {
                    div_res = (int16_t)((int8_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt) / (int8_t)((this->data_vs2 & (vs2_mask)) >>   8);
                    div_res = div_res > INT8_MAX ? INT8_MAX : div_res;
                    div_res = div_res < INT8_MIN ? INT8_MIN : div_res;
                  } else {
                    div_res = -1;
                  }
                  temp_res = (div_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else if (vd1_mask == 0xff) {
                  if ((int8_t)((this->data_vs2 & (vs2_mask))) != 0) {
                    div_res = (int16_t)((int8_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt) / (int8_t)(this->data_vs2 & (vs2_mask));
                    div_res = div_res > INT8_MAX ? INT8_MAX : div_res;
                    div_res = div_res < INT8_MIN ? INT8_MIN : div_res;
                  } else {
                    div_res = -1;
                  }
                  temp_res = (div_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else {
                  if ((int16_t)((this->data_vs2 & (vs2_mask))) != 0) {
                    div_res = ((int16_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt) / (int16_t)(this->data_vs2 & (vs2_mask));
                    div_res = div_res > INT16_MAX ? INT16_MAX : div_res;
                    div_res = div_res < INT16_MIN ? INT16_MIN : div_res;
                  } else {
                    div_res = -1;
                  }
                  temp_res = (div_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                }
              } else { // IVV
                if (vd1_mask == 0xff00) {
                  if ((int8_t)((this->data_vs2 & (vs2_mask)) >> 8) != 0) {
                    div_res = (int16_t)((int8_t)((this->data_vs1 & (vs1_mask)) >> 8) << this->MY_VENUS_INS_PARAM.vfu_shamt) / (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                    div_res = div_res > INT8_MAX ? INT8_MAX : div_res;
                    div_res = div_res < INT8_MIN ? INT8_MIN : div_res;
                  } else {
                    div_res = -1;
                  }
                  temp_res = (div_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else if (vd1_mask == 0xff) {
                  if ((int8_t)((this->data_vs2 & (vs2_mask))) != 0) {
                    div_res = (int16_t)( (int8_t)(this->data_vs1 & (vs1_mask)) << this->MY_VENUS_INS_PARAM.vfu_shamt) / (int8_t)((this->data_vs2 & (vs2_mask)));
                    div_res = div_res > INT8_MAX ? INT8_MAX : div_res;
                    div_res = div_res < INT8_MIN ? INT8_MIN : div_res;
                  } else {
                    div_res = -1;
                  }
                  temp_res = (div_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else {
                  if ((int16_t)((this->data_vs2 & (vs2_mask))) != 0) {
                    div_res = ((int16_t)((this->data_vs1 & (vs1_mask))) << this->MY_VENUS_INS_PARAM.vfu_shamt) / (int16_t)((this->data_vs2 & (vs2_mask)));
                    div_res = div_res > INT16_MAX ? INT16_MAX : div_res;
                    div_res = div_res < INT16_MIN ? INT16_MIN : div_res;
                  } else {
                    div_res = -1;
                  }
                  temp_res = (div_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                }
              }
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VREM: {
          this->instr_name = (char *)"VREM";
            int rem_res;
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00) {
                  if ((int8_t)((this->data_vs2 & (vs2_mask)) >> 8) != 0) {
                    rem_res = (int16_t)((int8_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt) % (int8_t)((this->data_vs2 & (vs2_mask)) >>   8);
                  } else {
                    rem_res = (int16_t)((int8_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt);
                  }
                  temp_res = (rem_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else if (vd1_mask == 0x00ff) {
                  if ((int8_t)((this->data_vs2 & (vs2_mask))) != 0) {
                    rem_res = (int16_t)((int8_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt) % (int8_t)((this->data_vs2 & (vs2_mask)));
                  } else {
                    rem_res = (int16_t)((int8_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt);
                  }
                  temp_res = (rem_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else {
                  if ((int16_t)((this->data_vs2 & (vs2_mask))) != 0) {
                    rem_res = ((int16_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt) % (int16_t)(this->data_vs2 & (vs2_mask));
                  } else {
                    rem_res = ((int16_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt);
                  }
                  temp_res = (rem_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                }
              } else { // IVV
                if (vd1_mask == 0xff00) {
                  if ((int8_t)((this->data_vs2 & (vs2_mask)) >> 8) != 0) {
                    rem_res = (int16_t)((int8_t)((this->data_vs1 & (vs1_mask)) >> 8) << this->MY_VENUS_INS_PARAM.vfu_shamt) % (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                  } else {
                    rem_res = (int16_t)((int8_t)((this->data_vs1 & (vs1_mask)) >> 8) << this->MY_VENUS_INS_PARAM.vfu_shamt);
                  }
                  temp_res = (rem_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else if (vd1_mask == 0x00ff) {
                  if ((int8_t)((this->data_vs2 & (vs2_mask))) != 0) {
                    rem_res = (int16_t)((int8_t)((this->data_vs1 & (vs1_mask))) << this->MY_VENUS_INS_PARAM.vfu_shamt) % (int8_t)((this->data_vs2 & (vs2_mask)));
                  } else {
                    rem_res = (int16_t)( (int8_t)((this->data_vs1 & (vs1_mask))) << this->MY_VENUS_INS_PARAM.vfu_shamt);
                  }
                  temp_res = (rem_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else {
                  if ((int16_t)((this->data_vs2 & (vs2_mask))) != 0) {
                    rem_res = ((int16_t)((this->data_vs1 & (vs1_mask))) << this->MY_VENUS_INS_PARAM.vfu_shamt) % (int16_t)((this->data_vs2 & (vs2_mask)));
                  } else {
                    rem_res = ((int16_t)((this->data_vs1 & (vs1_mask))) << this->MY_VENUS_INS_PARAM.vfu_shamt);
                  }
                  temp_res = (rem_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                }
              }
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VDIVU: {
          this->instr_name = (char *)"VDIVU";
            int div_res;
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00) {
                  if ((uint8_t)((this->data_vs2 & (vs2_mask)) >>  8) != 0) {
                    div_res = (uint16_t)((uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt) / (uint8_t)( (this->data_vs2 & (vs2_mask)) >> 8);
                    div_res = div_res > 255 ? 255 : div_res;
                    div_res = div_res < 0 ? 0 : div_res;
                  } else {
                    div_res = 255;
                  }
                  temp_res = (div_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else if (vd1_mask == 0xff) {
                  if ((uint8_t)((this->data_vs2 & (vs2_mask))) != 0) {
                    div_res = (uint16_t)((uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt) / (uint8_t)(this->data_vs2 & (vs2_mask));
                    div_res = div_res > 255 ? 255 : div_res;
                    div_res = div_res < 0 ? 0 : div_res;
                  } else {
                    div_res = 255;
                  }
                  temp_res = (div_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else {
                  if ((uint16_t)( (this->data_vs2 & (vs2_mask))) != 0) {
                    div_res = ((uint16_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt) / (uint16_t)(this->data_vs2 & (vs2_mask));
                    div_res = div_res > 65535 ? 65535 : div_res;
                    div_res = div_res < 0 ? 0 : div_res;
                  } else {
                    div_res = 65535;
                  }
                  temp_res = (div_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                }
              } else { // IVV
                if (vd1_mask == 0xff00) {
                  if ((uint8_t)((this->data_vs2 & (vs2_mask)) >>  8) != 0) {
                    div_res = (uint16_t)((uint8_t)((this->data_vs1 & (vs1_mask)) >> 8) << this->MY_VENUS_INS_PARAM.vfu_shamt) / (uint8_t)( (this->data_vs2 & (vs2_mask)) >> 8);
                    div_res = div_res > 255 ? 255 : div_res;
                    div_res = div_res < 0 ? 0 : div_res;
                  } else {
                    div_res = 255;
                  }
                  temp_res = (div_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else if (vd1_mask == 0xff) {
                  if ((uint8_t)((this->data_vs2 & (vs2_mask))) != 0) {
                    div_res = (uint16_t)((uint8_t)(this->data_vs1 & (vs1_mask)) << this->MY_VENUS_INS_PARAM.vfu_shamt) / (uint8_t)((this->data_vs2 & (vs2_mask)));
                    div_res = div_res > 255 ? 255 : div_res;
                    div_res = div_res < 0 ? 0 : div_res;
                  } else {
                    div_res = 255;
                  }
                  temp_res = (div_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else {
                  if ((uint16_t)( (this->data_vs2 & (vs2_mask))) != 0) {
                    div_res = ((uint16_t)((this->data_vs1 & (vs1_mask))) << this->MY_VENUS_INS_PARAM.vfu_shamt) / (uint16_t)((this->data_vs2 & (vs2_mask)));
                    div_res = div_res > 65535 ? 65535 : div_res;
                    div_res = div_res < 0 ? 0 : div_res;
                  } else {
                    div_res = 65535;
                  }
                  temp_res = (div_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                }
              }
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }
        case VREMU: {
          this->instr_name = (char *)"VREMU";
            int rem_res;
            if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
              if (this->MY_VENUS_INS_PARAM.function3 == IVX) {
                if (vd1_mask == 0xff00) {
                  if ((uint8_t)((this->data_vs2 & (vs2_mask)) >>  8) != 0) {
                    rem_res = (uint16_t)((uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt) % (uint8_t)( (this->data_vs2 & (vs2_mask)) >> 8);
                  } else {
                    rem_res = (uint16_t)((uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt);
                  }
                  temp_res = (rem_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else if (vd1_mask == 0x00ff) {
                  if ((uint8_t)((this->data_vs2 & (vs2_mask))) != 0) {
                    rem_res = (uint16_t)((uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt) % (uint8_t)((this->data_vs2 & (vs2_mask)));
                  } else {
                    rem_res = (uint16_t)((uint8_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt);
                  }
                  temp_res = (rem_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else {
                  if ((uint16_t)( (this->data_vs2 & (vs2_mask))) != 0) {
                    rem_res = ((uint16_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt) % (uint16_t)(this->data_vs2 & (vs2_mask));
                  } else {
                    rem_res = ((uint16_t)(this->MY_VENUS_INS_PARAM.scalar_op) << this->MY_VENUS_INS_PARAM.vfu_shamt);
                  }
                  temp_res = (rem_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                }
              } else { // IVV
                if (vd1_mask == 0xff00) {
                  if ((uint8_t)((this->data_vs2 & (vs2_mask)) >>  8) != 0) {
                    rem_res = (uint16_t)((uint8_t)((this->data_vs1 & (vs1_mask)) >> 8) << this->MY_VENUS_INS_PARAM.vfu_shamt) % (uint8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                  } else {
                    rem_res = (uint16_t)( (uint8_t)( (this->data_vs1 & (vs1_mask)) >> 8) << this->MY_VENUS_INS_PARAM.vfu_shamt);
                  }
                  temp_res = (rem_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else if (vd1_mask == 0xff) {
                  if ((uint8_t)((this->data_vs2 & (vs2_mask))) != 0) {
                    rem_res = (uint16_t)((uint8_t)(this->data_vs1 & (vs1_mask)) << this->MY_VENUS_INS_PARAM.vfu_shamt) % (uint8_t)((this->data_vs2 & (vs2_mask)));
                  } else {
                    rem_res = (uint16_t)( (uint8_t)(this->data_vs1 & (vs1_mask)) << this->MY_VENUS_INS_PARAM.vfu_shamt);
                  }
                  temp_res = (rem_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                } else {
                  if ((uint16_t)( (this->data_vs2 & (vs2_mask))) != 0) {
                    rem_res = ((uint16_t)((this->data_vs1 & (vs1_mask))) << this->MY_VENUS_INS_PARAM.vfu_shamt) %  (uint16_t)((this->data_vs2 & (vs2_mask)));
                  } else {
                    rem_res = ((uint16_t)((this->data_vs1 & (vs1_mask))) << this->MY_VENUS_INS_PARAM.vfu_shamt);
                  }
                  temp_res = (rem_res) & ((1 << ((this->MY_VENUS_INS_PARAM.vew + 1) << 3)) - 1);
                }
              }
              if (vd1_mask == 0xff00)
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + ((temp_res << 8) & vd1_mask);
              else
                this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (temp_res & vd1_mask);
            } else {
              this->data_vd1 = (this->data_vd1 & (~vd1_mask)) + (this->data_vs2 & (vd1_mask));
            }
          break;
        }


        case VREDAND: {
          this->instr_name = (char *)"VREDAND";
          if (this->MY_VENUS_INS_PARAM.vew == EW8) {
              if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
                if (vd1_mask == 0xff00)
                  and_value = and_value & ((uint8_t)((this->data_vs2 & (vs2_mask)) >> 8));
                else
                  and_value = and_value & ((uint8_t)((this->data_vs2 & (vs2_mask))));
              }
          } else {
              if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
                and_value_1 = and_value_1 & ((int16_t)(this->data_vs2 & (vs2_mask)));
              }
          }
          this->data_vd1 = INT_MIN;
          break;
        }
        case VREDOR: {
          this->instr_name = (char *)"VREDOR";
          if (this->MY_VENUS_INS_PARAM.vew == EW8) {
              if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
                if (vd1_mask == 0xff00)
                  or_value = or_value | ((uint8_t)((this->data_vs2 & (vs2_mask)) >> 8));
                else
                  or_value = or_value | ((uint8_t)((this->data_vs2 & (vs2_mask))));
              }
          } else {
              if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
                or_value_1 = or_value_1 | ((int16_t)(this->data_vs2 & (vs2_mask)));
              }
          }
          this->data_vd1 = INT_MIN;
          break;
        }
        case VREDXOR: {
          this->instr_name = (char *)"VREDXOR";
          if (this->MY_VENUS_INS_PARAM.vew == EW8) {
              if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
                if (vd1_mask == 0xff00)
                  xor_value = xor_value ^ ((uint8_t)((this->data_vs2 & (vs2_mask)) >> 8));
                else
                  xor_value = xor_value ^ ((uint8_t)((this->data_vs2 & (vs2_mask))));
              }
          } else {
              if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
                xor_value_1 = xor_value_1 ^ ((int16_t)(this->data_vs2 & (vs2_mask)));
              }
          }
          this->data_vd1 = INT_MIN;
          break;
        }
        case VREDMIN: {
          // min_value = 127;
          // min_value_1 = 32767;
          this->instr_name = (char *)"VREDMIN";
          if (this->MY_VENUS_INS_PARAM.vew == EW8) {
              if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
                if (vd1_mask == 0xff00) {
                  if(min_value > (int8_t)((this->data_vs2 & (vs2_mask)) >> 8)) {min_value = (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);}
                }
                else {
                  if(min_value > (int8_t)(this->data_vs2 & (vs2_mask))) {min_value = (int8_t)(this->data_vs2 & (vs2_mask));}
                }
              }
          } else {
              if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
                if(min_value_1 > (int16_t)(this->data_vs2 & (vs2_mask))) {min_value_1 = (int16_t)(this->data_vs2 & (vs2_mask));}
              }
          }
          this->data_vd1 = INT_MIN;
          break;
        }
        case VREDMINU: {
          // uint8_t min_uvalue;
          // uint16_t min_uvalue_1;
          this->instr_name = (char *)"VREDMINU";
          if (this->MY_VENUS_INS_PARAM.vew == EW8) {
              if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
                if (vd1_mask == 0xff00){
                  if(min_uvalue > (uint8_t)((this->data_vs2 & (vs2_mask)) >> 8)) {min_uvalue = (uint8_t)((this->data_vs2 & (vs2_mask)) >> 8);}
                }
                else {
                  if(min_uvalue > (uint8_t)(this->data_vs2 & (vs2_mask))) {min_uvalue = (uint8_t)(this->data_vs2 & (vs2_mask));}
                }
              }
          } else {
              if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
                if(min_uvalue_1 > (uint16_t)(this->data_vs2 & (vs2_mask))) {min_uvalue_1 = (uint16_t)(this->data_vs2 & (vs2_mask));}
              }
          }
          this->data_vd1 = INT_MIN;
          break;
        }
        case VREDMAX: {
          // int8_t max_value;
          // int16_t max_value_1;
          this->instr_name = (char *)"VREDMAX";
          if (this->MY_VENUS_INS_PARAM.vew == EW8) {
              if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
                if (vd1_mask == 0xff00) {
                  if(max_value < (int8_t)((this->data_vs2 & (vs2_mask)) >> 8)) {max_value = (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);}
                }
                else {
                  if(max_value < (int8_t)(this->data_vs2 & (vs2_mask))) {max_value = (int8_t)(this->data_vs2 & (vs2_mask));}
                }
              }
          } else {
              if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
                if(max_value_1 < (int16_t)(this->data_vs2 & (vs2_mask))) {max_value_1 = (int16_t)(this->data_vs2 & (vs2_mask));}
              }
          }
          this->data_vd1 = INT_MIN;
          break;
        }
        case VREDMAXU: {
          // uint8_t max_uvalue;
          // uint16_t max_uvalue_1;
          this->instr_name = (char *)"VREDMAXU";
          if (this->MY_VENUS_INS_PARAM.vew == EW8) {
              if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
                if (vd1_mask == 0xff00) {
                  if(max_uvalue < (uint8_t)((this->data_vs2 & (vs2_mask)) >> 8)) {max_uvalue = (uint8_t)((this->data_vs2 & (vs2_mask)) >> 8);}
                }
                else {
                  if(max_uvalue < (uint8_t)(this->data_vs2 & (vs2_mask))) {max_uvalue = (uint8_t)(this->data_vs2 & (vs2_mask));}
                }
              }
          } else {
              if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
                if(max_uvalue_1 < (uint16_t)(this->data_vs2 & (vs2_mask))) {max_uvalue_1 = (uint16_t)(this->data_vs2 & (vs2_mask));}
              }
          }
          this->data_vd1 = INT_MIN;
          break;
        }
        case VREDSUM: {
          // int32_t sum_result = 0;
          this->instr_name = (char *)"VREDSUM";
          if (this->MY_VENUS_INS_PARAM.vew == EW8) {
              if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
                if (vd1_mask == 0xff00)
                  sum_result = sum_result + (int8_t)((this->data_vs2 & (vs2_mask)) >> 8);
                else
                  sum_result = sum_result + (int8_t)(this->data_vs2 & (vs2_mask));
              }
          } else {
              if ((!this->MY_VENUS_INS_PARAM.vmask_read) || (this->MY_VENUS_INS_PARAM.vmask_read && data_vmask_r_element)) {
                sum_result = sum_result + (int16_t)(this->data_vs2 & (vs2_mask));
              }
          }
          this->data_vd1 = INT_MIN;
          break;
        }

        default: {
          panic("UNSUPPORTED INSTR!!!");
        }
    }
}



void venus_vfu::resetReduceStat() {
  and_value = 255;
  and_value_1 = 65535;
  or_value = 0;
  or_value_1 = 0;
  xor_value = 0;
  xor_value_1 = 0;
  min_value = 127;
  min_value_1 = 32767;
  min_uvalue = 255;
  min_uvalue_1 = 65535;
  max_value = -128;
  max_value_1 = -32768;
  max_uvalue = 0;
  max_uvalue_1 = 0;
  sum_result = 0;
}

}
