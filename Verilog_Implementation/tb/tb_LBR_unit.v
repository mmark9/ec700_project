/** @module : tb_LBR_unit
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

module tb_LBR_unit(); 
 
reg clock, reset, stall;
reg [1:0] lbrReq, next_PC_sel; 
reg [15:0] RW_address, ALU_result, PC_address, JAL_target, JALR_target;

wire [15:0] output_data;

integer i;

LBR_unit #(16, 8) LBR (
    .clock(clock), 
    .reset(reset),
    .stall(stall),
    .lbrReq(lbrReq),
    .next_PC_sel(next_PC_sel),
    .RW_address(RW_address),
    .ALU_result(ALU_result),
    .PC_address(PC_address),
    .JAL_target(JAL_target),
    .JALR_target(JALR_target),
    .output_data(output_data)
); 

    // Clock generator
    always #1 clock = ~clock;

    initial begin
          clock  = 0;
          reset  = 1;
          stall = 0;
          lbrReq = 2'b00;
          next_PC_sel = 2'b00;
          RW_address = 16'h0000;
          ALU_result = 16'h0000;
          PC_address = 16'h0000;
          JAL_target = 16'h0000;
          JALR_target = 16'h0000;
          repeat (1) @ (posedge clock);
          
          #1 reset = 0;
          repeat (1) @ (posedge clock);
          
          for (i = 0; i < 8; i=i+1) begin
              PC_address = i;
              JAL_target = 1<<i;
              JALR_target = 16'hffff>>i;
              if ((i+1)%2 == 0)
                  next_PC_sel = 2'b10;
              else
                  next_PC_sel = 2'b11;
	      #1 stall = (i == 3)? 1 : 0;
              repeat (1) @ (posedge clock);
          end

          next_PC_sel = 2'b00;
	  for (i = 0; i < (1<<5); i=i+1) begin
              RW_address = i;
              ALU_result = 16'haaaa;
              #1 lbrReq = (i == 7)? 2'b11 : 2'b10;
              repeat (1) @ (posedge clock);
          end
          lbrReq = 2'b00;
     end
     
endmodule