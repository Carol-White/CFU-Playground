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

`define VLEN           64           // vector length in bits
`define XLEN           32           // not sure, data width maybe?
`define NUM_VEC        32           // number of available vector registers
`define INSN_WIDTH     32           // width of a single instruction
`define DATA_WIDTH     64
`define MEM_ADDR_WIDTH 32           // WE ONLY HAVE MEM ADDRESSES AS REGISTER IDS RIGHT NOW
`define MEM_DATA_WIDTH 64
`define VEX_DATA_WIDTH 32

module Vfu (
  input                           cmd_valid,
  output                          cmd_ready,
  input   [     `INSN_WIDTH-1:0]  cmd_payload_instruction,
  input   [ `VEX_DATA_WIDTH-1:0]  cmd_payload_inputs_0,
  input   [ `VEX_DATA_WIDTH-1:0]  cmd_payload_inputs_1,
  input   [                 2:0]  cmd_payload_rounding,
  output                          rsp_valid,
  input                           rsp_ready,
  output  [ `VEX_DATA_WIDTH-1:0]  rsp_payload_output,

  input                           reset,
  input                           clk
);
  
  wire    [`MEM_DATA_WIDTH-1:0]   mem_port_in;
  wire                            mem_port_valid_in;
  wire                            mem_port_ready_out;
  wire    [`MEM_DATA_WIDTH-1:0]   mem_port_out;
  wire    [`MEM_ADDR_WIDTH-1:0]   mem_port_addr_out;
  wire                            mem_port_valid_out;
  wire                            mem_port_req_out;

  // TODO: Add RVV output valid signal
  rvv_proc_main #(.VLEN(`VLEN), .XLEN(`XLEN), .NUM_VEC(`NUM_VEC), .INSN_WIDTH(`INSN_WIDTH), .DATA_WIDTH(`DATA_WIDTH), .MEM_ADDR_WIDTH(`MEM_ADDR_WIDTH))
        rvv_proc (.clk(clk), .rst_n(~reset), .insn_in(cmd_payload_instruction), .insn_valid(cmd_valid),
                  .mem_port_in(mem_port_in), .mem_port_valid_in(mem_port_valid_in), .mem_port_ready_out(mem_port_ready_out),
                  .mem_port_out(mem_port_out), .mem_port_req(mem_port_req_out), .mem_port_addr_out(mem_port_addr_out), .mem_port_valid_out(mem_port_valid_out),
                  .proc_rdy(cmd_ready), .vexrv_data_in_1(cmd_payload_inputs_0), .vexrv_data_in_2(cmd_payload_inputs_1),
                  .vexrv_data_out(rsp_payload_output), .vexrv_valid_out(rsp_valid));

  reg out_valid;
  reg [31:0] out_payload;
  reg s0_valid;
  reg [31:0] s0_payload;

  always @(posedge clk) begin
    s0_valid <= cmd_valid;
    s0_payload <= cmd_payload_instruction;

    out_valid <= s0_valid;
    out_payload <= s0_payload;
  end

  // assign rsp_valid          = out_valid;
  // assign rsp_payload_output = out_payload;

endmodule