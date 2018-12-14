/** @module : tb_RISC_V_Core
 *  @author : Adaptive & Secure Computing Systems (ASCS) Laboratory
 
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

 module tb_RISC_V_Core(); 
 
reg clock, reset, start, stall; 
reg [19:0] prog_address; 
reg report; // performance reporting

// For I/O funstions
reg [1:0]    from_peripheral;
reg [31:0]   from_peripheral_data;
reg          from_peripheral_valid;

wire [1:0]  to_peripheral;
wire [31:0] to_peripheral_data;
wire        to_peripheral_valid;

reg [19:0] isp_address;
reg [31:0] isp_data;
reg isp_write;

// RISC_V_Core #(0, 32, 16, 6, 3, 20, 0, 15) CORE (
RISC_V_Core CORE(
		  .clock(clock), 
		  .reset(reset), 
		  .start(start), 
		  .stall_in(stall),
		  .prog_address(prog_address),	
		  .from_peripheral(from_peripheral),
       	          .from_peripheral_data(from_peripheral_data),
                  .from_peripheral_valid(from_peripheral_valid),
                  .to_peripheral(to_peripheral),
		  .to_peripheral_data(to_peripheral_data),
		  .to_peripheral_valid(to_peripheral_valid),
		  .isp_address(isp_address),
		  .isp_data(isp_data),
		  .isp_write(isp_write),
		  .report(report),
		  .current_PC()
); 

    // Clock generator
    always #1 clock = ~clock;

    initial begin
          $readmemh("./software/applications/binaries/zeros32.dat", CORE.ID.registers.register_file);  
	
          clock  = 0;
          reset  = 1;
          report = 1; 
          stall  = 0;
          isp_address = 0;
          isp_data = 0;
          isp_write = 0;
          prog_address = 'h0;
          repeat (10) @ (posedge clock);
          
          #1 reset = 0;
          start = 1; 
          stall = 0;
          repeat (1) @ (posedge clock);
          
          start = 0; 
          repeat (1) @ (posedge clock);        
     end
     
endmodule
