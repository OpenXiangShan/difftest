module syscfg # (
  parameter  AWID_USE = 8           //choose valid addr wid
) (
  input  wire           clk        ,
  input  wire           rst_n      ,

  input  wire [16-1:0]  apb_addr   ,
  input  wire           apb_selx   ,
  input  wire           apb_enable ,
  input  wire           apb_write  , //high active
  input  wire [32-1:0]  apb_wdata  ,
  output wire           apb_ready  ,
  output reg  [32-1:0]  apb_rdata  ,
  output wire           apb_slverr ,

  input  wire [32-1:0]    syscfg_version
);

  reg                   apb_ready1;
//declare signal
  wire                  reg_wr;
  wire                  reg_rd;
  wire [AWID_USE-1:0]   reg_addr;

//process
  assign reg_wr   = (apb_enable == 1) && (apb_selx == 1) && (apb_write == 1) && (apb_ready == 1);
  assign reg_rd   = (apb_enable == 1) && (apb_selx == 1) && (apb_write == 0);
  assign reg_addr = apb_addr[00+:AWID_USE];

  always @ (posedge clk or negedge rst_n)
  begin
      if(rst_n == 0)
          apb_rdata <= 32'h0;
      else if (reg_rd == 1)
      begin
          case (reg_addr)
              `DATA_VERSION:
                  apb_rdata <= {syscfg_version};
              default:
                  apb_rdata <= 32'h0;
          endcase
      end
  end


  always @ (posedge clk or negedge rst_n)
  begin
      if (rst_n == 0)
          apb_ready1 <= 1'b1;
      else if (apb_ready1 == 1'h0)
          apb_ready1 <= 1'b1;
      else if (apb_selx == 1'h1 && apb_enable == 1'h0)
          apb_ready1 <= 1'b0;
  end

  assign apb_ready = apb_ready1;
  assign apb_slverr = 1'h0;

endmodule
