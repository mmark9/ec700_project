/** @module : tb_control_unit
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
 *
 */

module tb_control_unit(); 
 
reg clock, reset; 
reg [6:0] opcode; 
reg report; // performance reporting

wire branch_op, memRead, memWrite;
wire operand_B_sel, regWrite;

wire [1:0] lbrReq, memtoReg, next_PC_sel;
wire [1:0] operand_A_sel, extend_sel;

wire [2:0] ALUOp;


localparam [6:0]  R_TYPE  = 7'b0110011,
                  I_TYPE  = 7'b0010011,
                  STORE   = 7'b0100011,
                  LOAD    = 7'b0000011,
                  BRANCH  = 7'b1100011,
                  JALR    = 7'b1100111,
                  JAL     = 7'b1101111,
                  AUIPC   = 7'b0010111,
                  LUI     = 7'b0110111,
                  FENCES  = 7'b0001111,
                  SYSCALL = 7'b1110011,
                  RDLBR   = 7'b0001011,
                  WRLBR   = 7'b0101011;

// module control_unit #(parameter CORE = 0, PRINT_CYCLES_MIN = 1, PRINT_CYCLES_MAX = 1000 )
control_unit CU (
    .clock(clock), 
    .reset(reset),
    .opcode(opcode),
    .branch_op(branch_op),
    .memRead(memRead),
    .lbrReq(lbrReq),
    .memtoReg(memtoReg), 
    .ALUOp(ALUOp),
    .next_PC_sel(next_PC_sel),
    .operand_A_sel(operand_A_sel), 
    .operand_B_sel(operand_B_sel),
    .extend_sel(extend_sel),
    .memWrite(memWrite),
    .regWrite(regWrite),
    .report(report)
); 

    // Clock generator
    always #1 clock = ~clock;

    initial begin
          clock  = 0;
          reset  = 1;
          report = 1;
          opcode = 6'b0;
          repeat (1) @ (posedge clock);
          
          #1 reset = 0;
          repeat (1) @ (posedge clock);
          
	  #1 opcode = R_TYPE;
          repeat (1) @ (posedge clock);

	  #1 opcode = I_TYPE;
          repeat (1) @ (posedge clock);

	  #1 opcode = STORE;
          repeat (1) @ (posedge clock);

	  #1 opcode = LOAD;
          repeat (1) @ (posedge clock);

	  #1 opcode = BRANCH;
          repeat (1) @ (posedge clock);

	  #1 opcode = JALR;
          repeat (1) @ (posedge clock);

	  #1 opcode = JAL;
          repeat (1) @ (posedge clock);

	  #1 opcode = AUIPC;
          repeat (1) @ (posedge clock);

	  #1 opcode = LUI;
          repeat (1) @ (posedge clock);

	  #1 opcode = FENCES;
          repeat (1) @ (posedge clock);

	  #1 opcode = SYSCALL;
          repeat (1) @ (posedge clock);

	  #1 opcode = RDLBR;
          repeat (1) @ (posedge clock);

	  #1 opcode = WRLBR;
          repeat (1) @ (posedge clock);
     end
     
endmodule