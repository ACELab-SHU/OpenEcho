`timescale 1ns/1ps
module dmt_length_tb;
  import venus_soc_pkg::*;
  import l2_scheduler_pkg::*;
  logic clk=0;
  logic reset_n=0;
  always #5 clk=~clk;
  task_manager_req_t request_out;
  task_container_content descriptor;
  data_management_content dmt_record;
  data_management_table_wr_req_t published;
  logic publish_flag=0;
  int unsigned cases[9]='{1,44,64,65,172,336,6148,12288,65535};
  task_manager #(.NUM_TILE(2)) dut(
    .clk(clk),.reset_n(reset_n),.scan_mode_i(1'b0),.scan_en_i(1'b0),
    .mbist_en_n_i(1'b1),.mem_standby_i(1'b0),
    .tcen_dmt_i('1),.tgwen_dmt_i('1),.twen_dmt_i('1),.taddr_dmt_i('0),.tdin_dmt_i('0),
    .task_output_num_reg('0),.mem_resp_from_task_container_i('0),.mem_resp_from_global_para_i('0),
    .task_req_o(request_out),.arbiter_resp_i('0),.data_management_access_approved(1'b1),
    .data_management_flag_from_arbiter(publish_flag),.data_management_info_from_arbiter(published),
    .task_num_reg(6'd2),.tile_occupied_flag_i('0)
  );
  initial begin
    descriptor='0; published='0; dmt_record='0;
    descriptor.task_descriptor[0]={2'b00,6'd0,4'd0,32'h00101a00};
    #20;reset_n=1;#10;
    // Isolate the actual registered-input decode state; no replacement RTL.
    force dut.fsm_curr=dut.FSM_TASK_FIRING_2;
    force dut.temp_content_q=descriptor;
    force dut.task_firing_counter_q=0;
    force dut.task_pnt_q=1;
    force dut.data_management_resp.mem_rdata=dmt_record;
    foreach(cases[i]) begin
      published.addr=32'h01000000;published.size=cases[i];publish_flag=1;
      dmt_record.input_src_addr=28'h1000000;dmt_record.input_len=cases[i];
      #2;
      if(dut.data_management_table_req.mem_wdata[15:0] !== cases[i][15:0])
        $fatal(1,"DMT publication mismatch");
      if(dut.task_req_d.src_size !== cases[i]) $fatal(1,"DMA size mismatch");
      if(dut.task_req_d.dst_addr !== 32'h00101a00) $fatal(1,"DMA target mismatch");
      $display("DMT_CASE actual=%0d dma=%0d",cases[i],dut.task_req_d.src_size);
    end
    published.size=65536+172;dmt_record.input_len=published.size[15:0];#2;
    if(dut.data_management_table_req.mem_wdata[15:0] !== 16'd172 || dut.task_req_d.src_size !== 32'd172)
      $fatal(1,"DMT width mismatch");
    $display("DMT_WIDTH actual=65708 dma=%0d",dut.task_req_d.src_size);
    $display("DMT_RTL_UNIT_PASS");$finish;
  end
endmodule
