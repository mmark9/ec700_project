/** @module : tb_lbrRegFile
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

module tb_lbrRegFile(); 
 
reg clock, reset, wEn0, wEn1, wEn2; 
reg [15:0] write_data0, write_data1, write_data2;
reg [4:0] read_sel, write_sel0, write_sel1, write_sel2;

wire [15:0] read_data;

integer i;

lbrRegFile #(16, 8) lbrRegister (
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
    .read_data(read_data)
); 

    // Clock generator
    always #1 clock = ~clock;

    initial begin
          clock  = 0;
          reset  = 1;
          wEn0 = 0;
          wEn1 = 0;
          wEn2 = 0;
          read_sel = 5'b0;
          write_sel0 = 5'b0;
          write_sel1 = 5'b0;
          write_sel2 = 5'b0;
          write_data0 = 16'h0000;
          write_data1 = 16'h0000;
          write_data2 = 16'h0000;
          repeat (1) @ (posedge clock);
          
          #1 reset = 0;
          repeat (1) @ (posedge clock);
          
          for (i = 0; i < (1<<5); i=i+1) begin
	      #1 read_sel <= i;
              repeat (1) @ (posedge clock);
          end

          wEn0 <= 1;
          wEn1 <= 1;
          wEn2 <= 1;
          read_sel <= 5'b00000;
	  for (i = 0; i < (1<<5); i=i+3) begin
              write_sel0 <= i;
              write_data0 <= i;
              write_sel1 <= i+1;
              write_data1 <= i+1;
              write_sel2 <= i+2;
              write_data2 <= i+2;
              #1 repeat (1) @ (posedge clock);
          end

	  wEn0 <= 0;
          wEn1 <= 0;
          wEn2 <= 0;
          for (i = 0; i < (1<<5); i=i+1) begin
	      #1 read_sel <= i;
              repeat (1) @ (posedge clock);
          end
     end
     
endmodule