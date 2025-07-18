module interrupt_gen(
input               data_next,
input               rstn,
input 			    soc_clk_i,
input               dev_clk_i,
output              soc_clk_o,
output              dev_clk_o
    );

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

	
endmodule
