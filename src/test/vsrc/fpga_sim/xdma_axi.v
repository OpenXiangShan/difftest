`include "DifftestMacros.v"
module xdma_axi(
  input clock,
  input reset,
  input [511:0] axi_tdata,
  input [63:0] axi_tkeep,
  input axi_tlast,
  output axi_tready,
  input axi_tvalid
);

import "DPI-C" function bit v_xdma_tready();
import "DPI-C" function void v_xdma_write(
  input byte channel,
  input bit [511:0] axi_tdata,
  input bit axi_tlast
);

reg axi_tready_r;
assign axi_tready = axi_tready_r;
always @(posedge clock) begin
  if (reset) begin
    axi_tready_r <= 1'b0;
  end
  else begin
    axi_tready_r <= v_xdma_tready();
    if (axi_tvalid & axi_tready) begin
      v_xdma_write(0, axi_tdata, axi_tlast);
    end
  end
end

endmodule
