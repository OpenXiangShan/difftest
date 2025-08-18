/*
 * Copyright (C) 2025 Beijing Institute of Open Source Chip
 *
 * Description: AXIS data package helper for FPGA-difftest
 */
module fpga_clock_gate(
	input     CK,
	input	  E,
	output    Q
);

`ifdef NOTUSE_VIVADO
	reg EN;
	always_latch begin
		if (!CK) EN = E;
	end
	assign Q = EN & CK;
`else
	BUFGCE bufgce_1 (
		.O(Q),
		.I(CK),
		.CE(E)
	);
`endif
	
endmodule
