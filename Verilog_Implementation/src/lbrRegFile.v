/** @module : lbrRegFile
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

// Parameterized register file
// Assumptions: This Register File will be used in conjunction with the top module LBR.
//              This top module should contain the appropriate logic that will prevent
//              multiple writes to the same register in a single clock cycle.
module lbrRegFile #(parameter DATA_WIDTH = 32, LBR_SIZE = 16) (
                clock, reset, read_sel,
                wEn0, wEn1, wEn2,
                write_sel0, write_sel1, write_sel2,
                write_data0, write_data1, write_data2, 
                read_data
);

input clock, reset, wEn0, wEn1, wEn2; 
input [DATA_WIDTH-1:0] write_data0, write_data1, write_data2;
input [($clog2(LBR_SIZE)+2)-1:0] read_sel, write_sel0, write_sel1, write_sel2;
output[DATA_WIDTH-1:0] read_data;  

(* ram_style = "distributed" *) 
reg   [DATA_WIDTH-1:0] register_file[0:(1<<($clog2(LBR_SIZE)+2))-1];
integer i; 

always @(posedge clock) begin
    if(reset==1)
        for (i = 0; i < (1<<($clog2(LBR_SIZE)+2)); i=i+1) begin
                if (i == (1<<($clog2(LBR_SIZE)+1)))
                    register_file[i] <= {($clog2(LBR_SIZE)){1'b1}};
                else
        	    register_file[i] <= 0; 
    end
    else begin
        register_file[write_sel0] <= (wEn0)? write_data0 : register_file[write_sel0];
        register_file[write_sel1] <= (wEn1)? write_data1 : register_file[write_sel1];
        register_file[write_sel2] <= (wEn2)? write_data2 : register_file[write_sel2];
    end
end
          
//----------------------------------------------------
// Drive the outputs
//----------------------------------------------------
assign  read_data = register_file[read_sel];

endmodule