/*
 * Copyright (C) 2025 Beijing Institute of Open Source Chip
 *
 * Description: AXIS data package helper for FPGA-difftest
 */
module fpga_clock_gate(
	input               data_next,
	input               rstn,
	input 			    soc_clk_i,
	input               dev_clk_i,
	output              soc_clk_o,
	output              dev_clk_o
);

`ifdef NOTUSE_VIVADO
	reg EN_SOC;
	reg EN_DEV;
	always_latch begin
		if (!soc_clk_i) EN_SOC = data_next;
	end
	assign soc_clk_o = EN_SOC & soc_clk_i;

	always_latch begin
		if (!dev_clk_i) EN_DEV = data_next;
	end
	assign dev_clk_o = EN_DEV & dev_clk_i;

`else
	BUFGCE inst_bufgce_1 (
		.O(soc_clk_o),
		.I(soc_clk_i),
		.CE(data_next || !rstn)
	);
		
	BUFGCE inst_bufgce_2 (
		.O(dev_clk_o),
		.I(dev_clk_i),
		.CE(data_next || !rstn)
	);
`endif
	
endmodule
