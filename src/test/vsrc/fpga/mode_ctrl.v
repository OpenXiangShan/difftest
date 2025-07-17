module mode_ctrl (/*AUTOARG*/
   // Outputs
   normal_mode,
   phy_bist_mode, 
   mbist_mode,
   scan_mode, 
   // Inputs
   chip_mode_i
   );


// from IO
input  [1:0]  chip_mode_i;

// chip mode 
output        normal_mode;
output        phy_bist_mode;
output        mbist_mode;
output        scan_mode;

wire          normal_mode;
wire          phy_bist_mode;
wire          mbist_mode;
wire          scan_mode;

assign normal_mode           =  ~chip_mode_i[1] & ~chip_mode_i[0];
assign scan_mode             =  ~chip_mode_i[1] &  chip_mode_i[0];
assign mbist_mode            =   chip_mode_i[1] & ~chip_mode_i[0];
assign phy_bist_mode         =   chip_mode_i[1] &  chip_mode_i[0];

endmodule

