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



module Vfu (
  input              cmd_valid,
  output             cmd_ready,
  input     [31:0]   cmd_payload_instruction,
  input     [31:0]   cmd_payload_inputs_0,
  input     [31:0]   cmd_payload_inputs_1,
  input     [2:0]    cmd_payload_rounding,
  output             rsp_valid,
  input              rsp_ready,
  output      [31:0] rsp_payload_output,

  input              reset,
  input              clk
);

  reg [31:0] s0_insn_out;

  reg out_vld, s0_vld;
  reg out_rdy, s0_rdy;
  reg [31:0] insn_out;
  // Trivial handshaking for a combinational CFU
  always @(posedge clk) begin
    if(reset) begin
      out_vld <= 0;
      out_rdy <= 0;

      s0_vld <= 0;
      s0_rdy <= 0;

      s0_insn_out <= 0;
      insn_out <= 'hDEADBEEF;
    end else begin
      s0_vld <= (cmd_valid & out_rdy) ? 1 : 0;

      out_vld <= rsp_ready ? s0_vld : out_vld;//cmd_valid ? 1 : 0;//out_vld === 0 ? 1 : 0;
      out_rdy <= (cmd_valid & out_rdy) ? 0 : 1; // set to 0 when we process new data //s0_rdy;//rsp_ready ? 1 : 0;//out_rdy === 0 ? 1 : 0;

      s0_insn_out <= (cmd_valid & out_rdy) ? cmd_payload_instruction : s0_insn_out;
      insn_out <= (rsp_ready & s0_vld) ? s0_insn_out : insn_out;
    end
  end

  assign rsp_valid = out_vld; //1;//cmd_valid;
  assign cmd_ready = out_rdy; //1;//rsp_ready;

  //
  // select output -- note that we're not fully decoding the 3 function_id bits
  //
  assign rsp_payload_output = insn_out;//cmd_payload_inputs_1 + cmd_payload_inputs_0; //cmd_payload_instruction;//[0] ? cmd_payload_inputs_1 : cmd_payload_inputs_0 ;


endmodule
