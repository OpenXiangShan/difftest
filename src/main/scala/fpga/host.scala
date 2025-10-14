package difftest.fpga

import chisel3._
import chisel3.BlackBox
import chisel3.util.HasBlackBoxInline
import chisel3.util._
import difftest.fpga.xdma._

// Note: submodules `dual_buffer_bram` and `bram_port` will be emitted via inline text inside `VerilogDifftest2AXI`.

/**
	* BlackBox for Verilog module `Difftest2AXI`.
	* Wraps difftest/src/test/vsrc/fpga/Difftest2AXI.v and brings in its submodules.
	*/
class VerilogDifftest2AXI(
  val DATA_WIDTH: Int = 16000,
  val AXIS_DATA_WIDTH: Int = 512,
) extends BlackBox(
    Map(
      "DATA_WIDTH" -> DATA_WIDTH,
      "AXIS_DATA_WIDTH" -> AXIS_DATA_WIDTH,
    )
  )
  with HasBlackBoxInline {
  override def desiredName: String = "Difftest2AXI"

  // NOTE: Verilog uses macro `CONFIG_DIFFTEST_BATCH_IO_WITDH` for difftest_data width.
  // Here we model it as DATA_WIDTH.W for now; ensure the macro equals DATA_WIDTH during integration.
  val io = IO(new Bundle {
    val clock = Input(Clock())
    val reset = Input(Bool())
    val difftest_data = Input(UInt(DATA_WIDTH.W))
    val difftest_enable = Input(Bool())
    val core_clock_enable = Output(Bool())
    val axi_tdata = Output(UInt(AXIS_DATA_WIDTH.W))
    val axi_tkeep = Output(UInt(64.W))
    val axi_tlast = Output(Bool())
    val axi_tready = Input(Bool())
    val axi_tvalid = Output(Bool())
  })

  // Inline Verilog source and its dependencies
  setInline(
    "Difftest2AXI.v",
    """
      |/***************************************************************************************
      |* Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
      |* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
      |*
      |* DiffTest is licensed under Mulan PSL v2.
      |* You can use this software according to the terms and conditions of the Mulan PSL v2.
      |* You may obtain a copy of Mulan PSL v2 at:
      |*          http://license.coscl.org.cn/MulanPSL2
      |*
      |* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
      |* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
      |* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
      |*
      |* See the Mulan PSL v2 for more details.
      |***************************************************************************************/
      |`include "DifftestMacros.v"
      |module Difftest2AXI #(
      |  parameter DATA_WIDTH = 16000,
      |  parameter AXIS_DATA_WIDTH = 512
      |)(
      |  input clock,
      |  input reset,
      |  input [`CONFIG_DIFFTEST_BATCH_IO_WITDH - 1:0] difftest_data,
      |  input difftest_enable,
      |  output core_clock_enable,
      |  output [AXIS_DATA_WIDTH - 1:0] axi_tdata,
      |  output [63:0] axi_tkeep,
      |  output axi_tlast,
      |  input  axi_tready,
      |  output axi_tvalid
      |);
      |
      |    localparam NUM_PACKETS_PER_BUFFER = 8; // one send packet num
      |    localparam AXIS_SEND_LEN = ((DATA_WIDTH  + 8 + AXIS_DATA_WIDTH - 1) / AXIS_DATA_WIDTH);
      |
      |    localparam IDLE = 3'b001;
      |    localparam TRANSFER = 3'b010;
      |    localparam DONE = 3'b100;
      |
      |    reg [2:0] current_state, next_state;
      |
      |    reg [DATA_WIDTH + 8 - 1:0] mix_data; // Difftestdata with
      |    reg [AXIS_DATA_WIDTH - 1:0]  reg_axi_tdata;
      |    reg [7:0] datalen;
      |    reg [7:0] data_num;
      |    reg [4:0] state;
      |
      |    reg                  reg_axi_tvalid;
      |    reg                  reg_axi_tlast;
      |    reg                  reg_core_clock_enable;
      |    // Ping-Pong Buffers
      |    reg [1:0]wr_en;
      |    reg wr_buf;
      |    reg rd_buf;
      |    reg [7:0] wr_pkt_cnt;   // (0~7)
      |    reg [7:0] rd_pkt_cnt;   // (0~7)
      |    reg [1:0] buffer_valid;
      |    reg [2:0] dual_buffer_wr_addr;
      |    reg [DATA_WIDTH-1:0] dual_buffer_wr_data;
      |    wire [DATA_WIDTH-1:0] dual_buffer_rd_data[1:0];
      |    wire [DATA_WIDTH-1:0] dual_buffer_rd_data_mux = ~rd_buf ? dual_buffer_rd_data[0] : dual_buffer_rd_data[1];
      |
      |    assign core_clock_enable = reg_core_clock_enable;
      |    assign axi_tdata = reg_axi_tdata;
      |    assign axi_tvalid = reg_axi_tvalid;
      |    assign axi_tkeep = 64'hffffffff_ffffffff;
      |    assign axi_tlast = reg_axi_tlast;
      |
      |    wire both_full = &buffer_valid;
      |
      |    // Instantiate dual buffer BRAMs
      |    genvar i;
      |    generate
      |    for (i = 0; i < 2; i = i + 1) begin : gen_dual_buffer
      |        dual_buffer_bram #(
      |        .DATA_WIDTH(DATA_WIDTH),
      |        .NUM_PACKETS_PER_BUFFER(NUM_PACKETS_PER_BUFFER)
      |        ) dual_buffer_inst (
      |        .clk(clock),
      |        .rst(reset),
      |        .wr_en(wr_en[i]),
      |        .wr_addr(dual_buffer_wr_addr),
      |        .wr_data(dual_buffer_wr_data),
      |        .rd_addr(rd_pkt_cnt),
      |        .rd_data(dual_buffer_rd_data[i])
      |        );
      |    end
      |    endgenerate
      |
      |
      |    // asynchronous clock fetches the signal
      |`ifdef ASYNC_CLK_2N
      |    reg [7:0] clk_cnt;
      |    initial clk_cnt = 0;
      |    always @(posedge clock) begin
      |        if (clk_cnt == (2 * `ASYNC_CLK_2N - 1)) begin
      |            clk_cnt <= 0;
      |        end else begin
      |            clk_cnt <= clk_cnt + 'b1;
      |        end
      |    end
      |    wire difftest_sampling = difftest_enable && clk_cnt == (2 * `ASYNC_CLK_2N - 1);
      |`else
      |    wire difftest_sampling = difftest_enable;
      |`endif //ASYNC_CLK_2N
      |
      |    wire can_send = buffer_valid[rd_buf];
      |    wire last_pkt = rd_pkt_cnt == NUM_PACKETS_PER_BUFFER;
      |    // Each package has AXIS_SEND_LEN send
      |    wire last_send = datalen == (AXIS_SEND_LEN - 1);
      |
      |    always @(posedge clock) begin
      |        if (reset)
      |            current_state <= IDLE;
      |        else
      |            current_state <= next_state;
      |    end
      |
      |/* verilator lint_off CASEINCOMPLETE */
      |    always @(*) begin
      |        case(current_state)
      |            IDLE:     next_state = can_send ? TRANSFER : IDLE;
      |            TRANSFER: next_state = (axi_tready & axi_tvalid & last_pkt & last_send) ? DONE : TRANSFER;
      |            DONE:     next_state = IDLE;
      |            default:  next_state = IDLE;
      |        endcase
      |    end
      |
      |    // Write Buffer: record data from difftest
      |    always @(posedge clock) begin
      |        if (reset) begin
      |            wr_buf <= 0;
      |            wr_pkt_cnt <= 0;
      |            buffer_valid <= 2'b00;
      |        end else begin
      |            wr_en[1:0] <= 2'b00;
      |            if (difftest_sampling & reg_core_clock_enable) begin
      |                dual_buffer_wr_addr <= wr_pkt_cnt[2:0];
      |                dual_buffer_wr_data <= difftest_data;
      |                wr_pkt_cnt <= wr_pkt_cnt + 1'b1;
      |                wr_en[wr_buf] <= 1;
      |                if (wr_pkt_cnt == NUM_PACKETS_PER_BUFFER - 1) begin
      |                    buffer_valid[wr_buf] <= 1;
      |                    wr_pkt_cnt <= 0;
      |                    wr_buf <= ~wr_buf; // Switch buffers
      |                end
      |            end
      |            if (next_state == DONE) begin // Releasing a buffer
      |                buffer_valid[rd_buf] <= 0;
      |            end
      |        end
      |    end
      |
      |    always @(posedge clock) begin // Back pressure control
      |        if (reset) begin
      |            reg_core_clock_enable <= 1'b1;
      |        end else begin
      |            reg_core_clock_enable <= ~both_full & ~(wr_pkt_cnt == NUM_PACKETS_PER_BUFFER - 1 & difftest_sampling);
      |        end
      |    end
      |
      |    // Read buffer: send data to axi
      |    always @(posedge clock) begin
      |        if(reset) begin
      |            reg_axi_tvalid <= 0;
      |            reg_axi_tlast <= 0;
      |            rd_buf <= 0;
      |            rd_pkt_cnt <= 0;
      |            data_num <= 0;
      |            datalen <= 0;
      |        end else begin
      |            case(current_state)
      |            IDLE : begin
      |                if (can_send) begin
      |                    mix_data <= {dual_buffer_rd_data_mux, data_num};
      |                    rd_pkt_cnt <= 1;
      |                    data_num <= data_num + 1'b1;
      |                end
      |            end
      |            TRANSFER: begin
      |                if(axi_tready && axi_tvalid) begin
      |                    reg_axi_tdata <= mix_data[AXIS_DATA_WIDTH - 1:0];
      |                    if(last_pkt & last_send) begin
      |                        reg_axi_tlast <= 1;
      |                        rd_pkt_cnt <= 0;
      |                        datalen <= 0;
      |                    end else if (!last_pkt & last_send) begin
      |                        mix_data <= {dual_buffer_rd_data_mux, 8'b0};
      |                        rd_pkt_cnt <= rd_pkt_cnt + 1'b1;
      |                        datalen <= 0;
      |                    end else begin
      |                        datalen <= datalen + 1'b1;
      |                        mix_data <= mix_data >> AXIS_DATA_WIDTH;
      |                    end
      |                end else if (~reg_axi_tvalid)begin
      |                    reg_axi_tvalid <= 1;
      |                    reg_axi_tdata <= mix_data[AXIS_DATA_WIDTH - 1:0];
      |                    mix_data <= mix_data >> AXIS_DATA_WIDTH;
      |                    datalen <= datalen + 1'b1;
      |                end
      |            end
      |            DONE: begin
      |                reg_axi_tvalid <= 0;
      |                reg_axi_tlast <= 0;
      |                rd_buf <= ~rd_buf;
      |            end
      |            endcase
      |        end
      |    end
      |/* verilator lint_on CASEINCOMPLETE */
      |endmodule
      |""".stripMargin,
  )

  // Also inline dependencies here to ensure they are always emitted with this BlackBox
  setInline(
    "dual_buffer_bram.sv",
    """
      |/***************************************************************************************
      |* Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
      |* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
      |*
      |* DiffTest is licensed under Mulan PSL v2.
      |* You can use this software according to the terms and conditions of the Mulan PSL v2.
      |* You may obtain a copy of Mulan PSL v2 at:
      |*          http://license.coscl.org.cn/MulanPSL2
      |*
      |* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
      |* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
      |* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
      |*
      |* See the Mulan PSL v2 for more details.
      |***************************************************************************************/
      |module dual_buffer_bram #(
      |  parameter DATA_WIDTH = 16000,
      |  parameter NUM_PACKETS_PER_BUFFER = 8
      |)(
      |  input clk,
      |  input rst,
      |  input wr_en,
      |  input [$clog2(NUM_PACKETS_PER_BUFFER)-1:0] wr_addr,
      |  input [DATA_WIDTH-1:0] wr_data,
      |  input [$clog2(NUM_PACKETS_PER_BUFFER)-1:0] rd_addr,
      |  output [DATA_WIDTH-1:0] rd_data
      |);
      |  localparam ADDR_WIDTH = $clog2(NUM_PACKETS_PER_BUFFER);
      |  localparam BLOCK_RAM_DATA_WIDTH = 4000;
      |  localparam NUM_BLOCKS = DATA_WIDTH / BLOCK_RAM_DATA_WIDTH;
      |
      |  wire [BLOCK_RAM_DATA_WIDTH-1:0] rd_data_split [0:NUM_BLOCKS-1];
      |  wire [BLOCK_RAM_DATA_WIDTH-1:0] wr_data_split [0:NUM_BLOCKS-1];
      |
      |  genvar i;
      |  generate
      |    for (i = 0; i < NUM_BLOCKS; i = i + 1) begin : gen_bram
      |      assign wr_data_split[i] = wr_data[(i+1)*BLOCK_RAM_DATA_WIDTH-1 : i*BLOCK_RAM_DATA_WIDTH];
      |      bram_port #(
      |        .DATA_WIDTH(BLOCK_RAM_DATA_WIDTH),
      |        .ADDR_WIDTH(ADDR_WIDTH)
      |      ) bram_inst (
      |        .clk(clk),
      |        .rst(rst),
      |        .wea(wr_en),
      |        .en(1'b1),
      |        .waddr(wr_addr),
      |        .raddr(rd_addr),
      |        .wdata(wr_data_split[i]),
      |        .rdata(rd_data_split[i])
      |      );
      |      assign rd_data[(i+1)*BLOCK_RAM_DATA_WIDTH-1 : i*BLOCK_RAM_DATA_WIDTH] = rd_data_split[i];
      |    end
      |  endgenerate
      |
      |endmodule
      |""".stripMargin,
  )

  setInline(
    "bram_port.v",
    """
      |/***************************************************************************************
      |* Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
      |* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
      |*
      |* DiffTest is licensed under Mulan PSL v2.
      |* You can use this software according to the terms and conditions of the Mulan PSL v2.
      |* You may obtain a copy of Mulan PSL v2 at:
      |*          http://license.coscl.org.cn/MulanPSL2
      |*
      |* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
      |* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
      |* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
      |*
      |* See the Mulan PSL v2 for more details.
      |***************************************************************************************/
      |module bram_port #(
      |  parameter DATA_WIDTH = 4000,
      |  parameter ADDR_WIDTH = 3,
      |  parameter RAM_DEPTH = 1 << ADDR_WIDTH
      |) (
      |  input                    clk,
      |  input                    rst,
      |  input                    wea,
      |  input                    en,
      |  input [ADDR_WIDTH - 1:0] waddr,
      |  input [ADDR_WIDTH - 1:0] raddr,
      |  input [DATA_WIDTH - 1:0] wdata,
      |  output reg [DATA_WIDTH - 1:0] rdata
      |);
      |
      |  (* ram_style = "block" *) reg [DATA_WIDTH - 1:0] mem [RAM_DEPTH - 1:0];
      |
      |`ifndef SYNTHESIS
      |  integer initvar;
      |  initial begin
      |    for (initvar = 0; initvar < RAM_DEPTH; initvar = initvar + 1)
      |      mem[initvar] = {DATA_WIDTH{1'b0}};
      |  end
      |`endif
      |
      |  always @(posedge clk) begin
      |    if (en) begin
      |      rdata <= mem[raddr];
      |    end
      |  end
      |
      |  always @(posedge clk) begin
      |    if (en && wea) begin
      |      mem[waddr] <= wdata;
      |    end
      |  end
      |endmodule
      |""".stripMargin,
  )
}

/**
	* Minimal host endpoint shell that instantiates Difftest2AXI.
	* External IO will be decided and connected later.
	*/
class HostEndpoint(
  val dataWidth: Int = 16000,
  val axisDataWidth: Int = 512,
  val ddrDataWidth: Int = 64,
) extends Module {
  val io = IO(new Bundle {
    val difftest_data = Input(UInt(dataWidth.W))
    val difftest_enable = Input(Bool())
    // AXI-stream
    val host_c2h_axis = new AxisMasterBundle(axisDataWidth)
    val host_h2c_axis = Flipped(new AxisMasterBundle(axisDataWidth))
    // Bring in SoC memory AXI as xdma bundle for arbitration
    val soc_axi_in = Flipped(new AXI4(addrWidth = 64, dataWidth = ddrDataWidth, idWidth = 1))
    // AXI4 master interface (xdma bundle) after arbitration to memory
    val axi4_to_mem = new AXI4(addrWidth = 64, dataWidth = ddrDataWidth, idWidth = 1)
    // Preserve core clock enable
    val core_clock_enable = Output(Bool())
    // Keep backward compatibility (optional)
    val axi_tkeep = Output(UInt(64.W))
  })

  // Instantiate the Verilog adapter BlackBox
  val Difftest2AXI = Module(new VerilogDifftest2AXI(DATA_WIDTH = dataWidth, AXIS_DATA_WIDTH = axisDataWidth))
  // Connect clock/reset
  Difftest2AXI.io.clock := clock
  Difftest2AXI.io.reset := reset
  // Connect host IO to Verilog inputs
  Difftest2AXI.io.difftest_data := io.difftest_data
  Difftest2AXI.io.difftest_enable := io.difftest_enable
  Difftest2AXI.io.axi_tready := io.host_c2h_axis.ready
  io.host_c2h_axis.last := Difftest2AXI.io.axi_tlast
  io.host_c2h_axis.data := Difftest2AXI.io.axi_tdata
  io.host_c2h_axis.valid := Difftest2AXI.io.axi_tvalid
  // Preserve core clock enable
  io.core_clock_enable := Difftest2AXI.io.core_clock_enable
  // Legacy keep signal
  io.axi_tkeep := Difftest2AXI.io.axi_tkeep

  // Instantiate AXIS(512b) -> AXI4(64b) write converter
  val axisWriter = Module(new Axis512ToAxi64Write(addrWidth = 64))
  axisWriter.io.axis.valid := io.host_h2c_axis.valid
  axisWriter.io.axis.data  := io.host_h2c_axis.data
  axisWriter.io.axis.last  := io.host_h2c_axis.last
  io.host_h2c_axis.ready   := axisWriter.io.axis.ready

  // 2-way arbiter (xdma bundle): in(0) = axisWriter, in(1) = SoC
  val arb = Module(new AXI4Arbiter(addrWidth = 64, dataWidth = ddrDataWidth, idWidth = 1))
  arb.io.in(0) <> axisWriter.io.axi
  arb.io.in(1) <> io.soc_axi_in
  io.axi4_to_mem <> arb.io.out


}

// Convert 512-bit AXIS stream to 64-bit AXI write-only transactions using xdma_axi bundles.
class Axis512ToAxi64Write(addrWidth: Int = 64) extends Module {
  val axisWidth = 512
  val axiDataWidth = 64
  val bytesPerBeat = axiDataWidth / 8
  val subBeatsPerAxis = axisWidth / axiDataWidth // 8

  val io = IO(new Bundle {
    val axis = Flipped(new AxisMasterBundle(axisWidth))
    val axi  = new AXI4(addrWidth = addrWidth, dataWidth = axiDataWidth, idWidth = 1)
  })

  // Default tie-offs for unused read channels
  io.axi.ar.valid := false.B
  io.axi.ar.bits.addr  := 0.U
  io.axi.ar.bits.prot  := 0.U
  io.axi.ar.bits.id    := 0.U
  io.axi.ar.bits.len   := 0.U
  io.axi.ar.bits.size  := log2Ceil(bytesPerBeat).U
  io.axi.ar.bits.burst := 1.U
  io.axi.ar.bits.lock  := false.B
  io.axi.ar.bits.cache := 0.U
  io.axi.ar.bits.qos   := 0.U
  io.axi.ar.bits.user  := 0.U
  io.axi.r.ready := true.B

  // State for 8-beat burst write
  val addrPtr     = RegInit(0.U(addrWidth.W))
  val axisReg     = Reg(UInt(axisWidth.W))
  val beatCnt     = RegInit(0.U(log2Ceil(subBeatsPerAxis).W)) // 0..7

  // Burst-level sequencing: Idle -> AW(len=7) -> send 8 W beats -> wait for B -> Idle
  val sIdle :: sAw :: sW :: sB :: Nil = Enum(4)
  val state = RegInit(sIdle)

  // Accept one 512-bit AXIS word only when idle; each word maps to one 8-beat AXI INCR burst
  io.axis.ready := (state === sIdle)
  val takeAxis = io.axis.valid && io.axis.ready


  // Default values for AW/W/B to avoid latches
  io.axi.aw.valid := false.B
  io.axi.aw.bits.addr  := addrPtr
  io.axi.aw.bits.prot  := 0.U
  io.axi.aw.bits.id    := 0.U
  io.axi.aw.bits.len   := (subBeatsPerAxis - 1).U // 8-beat burst -> len=7
  io.axi.aw.bits.size  := log2Ceil(bytesPerBeat).U // 64-bit
  io.axi.aw.bits.burst := 1.U // INCR
  io.axi.aw.bits.lock  := false.B
  io.axi.aw.bits.cache := 0.U
  io.axi.aw.bits.qos   := 0.U
  io.axi.aw.bits.user  := 0.U

  io.axi.w.valid := false.B
  io.axi.w.bits.data := 0.U
  io.axi.w.bits.strb := ((BigInt(1) << bytesPerBeat) - 1).U
  io.axi.w.bits.last := false.B

  io.axi.b.ready := true.B

  switch(state) {
    is(sIdle) {
      when(takeAxis) {
        axisReg := io.axis.data
        beatCnt := 0.U
        state   := sAw
      }
    }
    is(sAw) {
      io.axi.aw.valid := true.B
      when(io.axi.aw.fire) { state := sW }
    }
    is(sW) {
      io.axi.w.valid := true.B
      // Send least-significant 64b first, shift right by 64b per beat
      io.axi.w.bits.data := axisReg(axiDataWidth-1, 0)
      io.axi.w.bits.last := (beatCnt === (subBeatsPerAxis-1).U) // assert last on 8th beat
      when(io.axi.w.fire) {
        when(beatCnt === (subBeatsPerAxis-1).U) {
          state := sB
        }.otherwise {
          beatCnt := beatCnt + 1.U
          axisReg := axisReg >> axiDataWidth
        }
      }
    }
    is(sB) {
      when(io.axi.b.valid && io.axi.b.ready) {
        // One 8-beat burst finished; advance write address by 64B (next 512b window)
        addrPtr := addrPtr + (axisWidth/8).U
        // Reset and return to idle
        state   := sIdle
      }
    }
  }
}