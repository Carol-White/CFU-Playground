// Copyright 2021 The CFU-Playground Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
`include "rvv_proc_main.sv"
`include "axi_mem_queue.sv"
// `include "mock_mem.sv"

// MAX VLEN of 1024 because of dual port BWWE properties lol

`define VLEN           16384         // vector length in bits
`define XLEN           32           // not sure, data width maybe?
`define NUM_VEC        32           // number of available vector registers
`define INSN_WIDTH     32           // width of a single instruction
`define DATA_WIDTH     64
`define MEM_ADDR_WIDTH 32           // WE ONLY HAVE MEM ADDRESSES AS REGISTER IDS RIGHT NOW
`define MEM_DATA_WIDTH 32
`define MEM_DW_B       `MEM_DATA_WIDTH/8
`define VEX_DATA_WIDTH 32
`define DW_B           `DATA_WIDTH/8
`define FIFO_DEPTH_BITS 11          // the max number of packets in a full read is 2048

module Vfu (
    input                           clk,
    input                           reset,

    input                           cmd_valid,
    output                          cmd_ready,
    input   [     `INSN_WIDTH-1:0]  cmd_payload_instruction,
    input   [ `VEX_DATA_WIDTH-1:0]  cmd_payload_inputs_0,
    input   [ `VEX_DATA_WIDTH-1:0]  cmd_payload_inputs_1,
    input   [                 2:0]  cmd_payload_rounding,
    output                          rsp_valid,
    input                           rsp_ready,
    output  [ `VEX_DATA_WIDTH-1:0]  rsp_payload_output,

    output  [ `MEM_ADDR_WIDTH-1:0]  mbus_ar_addr  ,
    output                          mbus_ar_valid ,
    input                           mbus_ar_ready ,

    input   [ `MEM_DATA_WIDTH-1:0]  mbus_r_data   ,
    input                           mbus_r_valid  ,
    output                          mbus_r_ready  ,

    output  [ `MEM_ADDR_WIDTH-1:0]  mbus_aw_addr  ,
    output                          mbus_aw_valid ,
    input                           mbus_aw_ready ,

    output  [ `MEM_DATA_WIDTH-1:0]  mbus_w_data   ,
    output                          mbus_w_valid  ,
    output  [       `MEM_DW_B-1:0]  mbus_w_strb   ,

    input                           mbus_b_resp  ,
    input                           mbus_b_valid ,
    output                          mbus_b_ready
    );
  
    wire    [     `DATA_WIDTH-1:0]  rvv_data_in;
    wire                            rvv_valid_in;
    wire    [ `MEM_ADDR_WIDTH-1:0]  rvv_addr_out;
    wire    [     `DATA_WIDTH-1:0]  rvv_data_out;
    wire                            rvv_valid_out;
    wire                            rvv_req_out;
    wire    [           `DW_B-1:0]  rvv_be_out;
    wire                            rvv_done_ld;
    wire                            rvv_start_out;
    wire                            rvv_done_st;
    wire                            rvv_ready_out;

    rvv_proc_main #(.VLEN(`VLEN), .XLEN(`XLEN), .NUM_VEC(`NUM_VEC), .INSN_WIDTH(`INSN_WIDTH), .DATA_WIDTH(`DATA_WIDTH), .MEM_ADDR_WIDTH(`MEM_ADDR_WIDTH))
            rvv_proc (  .clk(clk), .rst_n(~reset), .insn_in(cmd_payload_instruction), .insn_valid(cmd_valid), .proc_rdy(cmd_ready), 
                         .mem_port_addr_out(rvv_addr_out), .mem_port_data_in(rvv_data_in), .mem_port_valid_in(rvv_valid_in), .mem_port_req_out(rvv_req_out), 
                        .mem_port_data_out(rvv_data_out),.mem_port_valid_out(rvv_valid_out), .mem_port_be_out(rvv_be_out), .mem_port_ready_out(rvv_ready_out),
                        .mem_port_done_ld(rvv_done_ld), .mem_port_done_st(rvv_done_st), .mem_port_start_out(rvv_start_out),
                        .vexrv_data_in_1(cmd_payload_inputs_0), .vexrv_data_in_2(cmd_payload_inputs_1), .vexrv_data_out(rsp_payload_output), .vexrv_valid_out(rsp_valid));

    mem_queue   #(.MBUS_DATA_WIDTH(`MEM_DATA_WIDTH), .MBUS_ADDR_WIDTH(`MEM_DATA_WIDTH), .FIFO_DEPTH_BITS(`FIFO_DEPTH_BITS))
            mq (    .clk(clk), .rst_n(~reset),
                    .mbus_ar_ready(mbus_ar_ready), .mbus_ar_addr(mbus_ar_addr), .mbus_ar_valid(mbus_ar_valid),
                     .mbus_r_data(mbus_r_data), .mbus_r_valid (mbus_r_valid), .mbus_r_ready (mbus_r_ready),
                    .mbus_aw_ready(mbus_aw_ready), .mbus_aw_addr (mbus_aw_addr), .mbus_aw_valid(mbus_aw_valid),
                    .mbus_w_data(mbus_w_data), .mbus_w_strb  (mbus_w_strb), .mbus_w_valid (mbus_w_valid),
                    .mbus_b_resp(mbus_b_resp), .mbus_b_ready (mbus_b_ready), .mbus_b_valid (mbus_b_valid),
                    .rvv_addr_out (rvv_addr_out), .rvv_data_out (rvv_data_out), .rvv_valid_out(rvv_valid_out), .rvv_done_st(rvv_done_st),
                    .rvv_req_out  (rvv_req_out), .rvv_be_out   (rvv_be_out), .rvv_data_in  (rvv_data_in), .rvv_valid_in(rvv_valid_in),
                    .rvv_start_out(rvv_start_out), .rvv_done_ld(rvv_done_ld), .rvv_ready_out(rvv_ready_out));
endmodule