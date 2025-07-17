// Asynchronous Input Synchronization
//
// The following code is an example of synchronizing an asynchronous input
// of a design to reduce the probability of metastability affecting a circuit.
//
// The following synthesis and implementation attributes is added to the code
// in order improve the MTBF characteristics of the implementation:
//
//  ASYNC_REG="TRUE" - Specifies registers will be receiving asynchronous data
//                     input to allow tools to report and improve metastability
//
// The following parameters are available for customization:
//
//   SYNC_STAGES     - Integer value for number of synchronizing registers, must be 2 or higher
//   PIPELINE_STAGES - Integer value for number of registers on the output of the
//                     synchronizer for the purpose of improveing performance.
//                     Particularly useful for high-fanout nets.
//   INIT            - Initial value of synchronizer registers upon startup, 1'b0 or 1'b1.

module RST_SYNC #(
   parameter SYNC_STAGES = 3,
   parameter PIPELINE_STAGES = 1,
   parameter INIT = 1'b0
) (
   input clk,
   input async_in,
   output sync_out
);

   (* ASYNC_REG="TRUE" *) reg [SYNC_STAGES-1:0] sreg = {SYNC_STAGES{INIT}};

   always @(posedge clk)
     sreg <= {sreg[SYNC_STAGES-2:0], async_in};

   generate
      if (PIPELINE_STAGES==0) begin: no_pipeline

         assign sync_out = sreg[SYNC_STAGES-1];

      end else if (PIPELINE_STAGES==1) begin: one_pipeline

         reg sreg_pipe = INIT;

         always @(posedge clk)
            sreg_pipe <= sreg[SYNC_STAGES-1];

         assign sync_out = sreg_pipe;

      end else begin: multiple_pipeline

        (* shreg_extract = "no" *) reg [PIPELINE_STAGES-1:0] sreg_pipe = {PIPELINE_STAGES{INIT}};

         always @(posedge clk)
            sreg_pipe <= {sreg_pipe[PIPELINE_STAGES-2:0], sreg[SYNC_STAGES-1]};

         assign sync_out = sreg_pipe[PIPELINE_STAGES-1];

      end
   endgenerate

endmodule

// The following is an instantiation template for async_input_sync
/*
// Asynchronous Input Synchronization
async_input_sync #(
   .SYNC_STAGES(3),
   .PIPELINE_STAGES(1),
   .INIT(1'b0)
) your_instance_name (
   .clk(clk),
   .async_in(async_in),
   .sync_out(sync_out)
);
*/
				
			
