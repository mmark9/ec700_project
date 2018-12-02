/** @module : LBR_unit
 *  @author : Adaptive & Secure Computing Systems (ASCS) Laboratory
 *  @author : Michael Graziano
 
 *  Copyright (c) 2018 BRISC-V (ASCS/ECE/BU)
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

module LBR_unit #(parameter DATA_WIDTH = 64, LBR_SIZE = 16) (
                clock, reset, stall, lbrReq, next_PC_sel,
		RW_address, ALU_result, PC_address, JAL_target, JALR_target,
                output_data
);

input clock, reset, stall;
input [1:0] lbrReq, next_PC_sel;
input [DATA_WIDTH-1:0] RW_address, ALU_result, PC_address, JAL_target, JALR_target;
output [DATA_WIDTH-1:0] output_data;

wire wEn0, wEn1, wEn2;
wire [($clog2(LBR_SIZE)+2)-1:0] read_sel, write_sel0, write_sel1, write_sel2;
wire [DATA_WIDTH-1:0] write_data0, write_data1, write_data2;

localparam [($clog2(LBR_SIZE)+2)-1:0] TOS_ADDR = 1<<(($clog2(LBR_SIZE)+2)-1);

assign read_sel = (lbrReq[1])? RW_address[($clog2(LBR_SIZE)+2)-1:0] : TOS_ADDR;
assign wEn0 = lbrReq[0] ^ (~stall & next_PC_sel[1]);
assign wEn1 = ~stall & next_PC_sel[1];
assign wEn2 = wEn1;
assign write_sel0 = read_sel;
assign write_sel1 = {2'b00, output_data[$clog2(LBR_SIZE)-1:0]};
assign write_sel2 = {2'b01, output_data[$clog2(LBR_SIZE)-1:0]};
assign write_data0 = (lbrReq[0])? ALU_result : output_data + 1;
assign write_data1 = PC_address;
assign write_data2 = (next_PC_sel[0])? JALR_target : JAL_target;

lbrRegFile #(DATA_WIDTH, LBR_SIZE) lbrRegister(
    .clock(clock),
    .reset(reset),
    .read_sel(read_sel),
    .wEn0(wEn0),
    .wEn1(wEn1),
    .wEn2(wEn2),
    .write_sel0(write_sel0),
    .write_sel1(write_sel1),
    .write_sel2(write_sel2),
    .write_data0(write_data0),
    .write_data1(write_data1),
    .write_data2(write_data2),
    .read_data(output_data)
);

endmodule