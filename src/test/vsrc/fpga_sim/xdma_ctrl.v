/***************************************************************************************
* Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
*
* DiffTest is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/
`include "DifftestMacros.v"
module xdma_ctrl #(
  parameter DATA_WIDTH = 16000,
  parameter AXIS_DATA_WIDTH = 512
)(
  input clock,
  input reset,
  input [`CONFIG_DIFFTEST_BATCH_IO_WITDH - 1:0] difftest_data,
  input difftest_enable,
  output core_clock_enable,
  output [AXIS_DATA_WIDTH - 1:0] axi_tdata,
  output [63:0] axi_tkeep,
  output axi_tlast,
  input  axi_tready,
  output axi_tvalid
);

    localparam NUM_PACKETS_PER_BUFFER = 8; // one send packet num
    localparam AXIS_SEND_LEN = ((DATA_WIDTH  + 8 + AXIS_DATA_WIDTH - 1) / AXIS_DATA_WIDTH);

    localparam IDLE = 3'b001;
    localparam TRANSFER = 3'b010;
    localparam DONE = 3'b100;

    reg [2:0] current_state, next_state;

    reg [DATA_WIDTH + 8 - 1:0] mix_data; // Difftestdata with
    reg [AXIS_DATA_WIDTH - 1:0]  reg_axi_tdata;
    reg [7:0] datalen;
    reg [7:0] data_num;
    reg [4:0] state;

    reg                  reg_axi_tvalid;
    reg                  reg_axi_tlast;
    reg                  reg_core_clock_enable;
    // Ping-Pong Buffers
    reg [1:0]wr_en;
    reg wr_buf;
    reg rd_buf;
    reg [7:0] wr_pkt_cnt;   // (0~7)
    reg [7:0] rd_pkt_cnt;   // (0~7)
    reg [1:0] buffer_valid;
    reg [2:0] dual_buffer_wr_addr;
    reg [DATA_WIDTH-1:0] dual_buffer_wr_data;
    wire [DATA_WIDTH-1:0] dual_buffer_rd_data[1:0];
    wire [DATA_WIDTH-1:0] dual_buffer_rd_data_mux = ~rd_buf ? dual_buffer_rd_data[0] : dual_buffer_rd_data[1];
    
    assign core_clock_enable = reg_core_clock_enable;
    assign axi_tdata = reg_axi_tdata;
    assign axi_tvalid = reg_axi_tvalid;
    assign axi_tkeep = 64'hffffffff_ffffffff;
    assign axi_tlast = reg_axi_tlast;

    wire both_full = &buffer_valid;
    dual_buffer_bram #(
      .DATA_WIDTH(DATA_WIDTH),
      .NUM_PACKETS_PER_BUFFER(NUM_PACKETS_PER_BUFFER),
      .ADDR_WIDTH(3)
    ) dual_buffer0 (
      .clk(clock),
      .rst(reset),
      .wr_en(wr_en[0]),
      .wr_addr(dual_buffer_wr_addr),
      .wr_data(dual_buffer_wr_data),
      .rd_addr(rd_pkt_cnt),
      .rd_data(dual_buffer_rd_data[0])
    );
    dual_buffer_bram #(
      .DATA_WIDTH(DATA_WIDTH),
      .NUM_PACKETS_PER_BUFFER(NUM_PACKETS_PER_BUFFER),
      .ADDR_WIDTH(3)
    ) dual_buffer1 (
      .clk(clock),
      .rst(reset),
      .wr_en(wr_en[1]),
      .wr_addr(dual_buffer_wr_addr),
      .wr_data(dual_buffer_wr_data),
      .rd_addr(rd_pkt_cnt),
      .rd_data(dual_buffer_rd_data[1])
    );


    // asynchronous clock fetches the signal
`ifdef ASYNC_CLK_2N
    reg [7:0] clk_cnt;
    initial clk_cnt = 0;
    always @(posedge clock) begin
        if (clk_cnt == (2 * `ASYNC_CLK_2N - 1)) begin
            clk_cnt <= 0;
        end else begin
            clk_cnt <= clk_cnt + 'b1;
        end
    end
    wire difftest_sampling = difftest_enable && clk_cnt == (2 * `ASYNC_CLK_2N - 1);
`else
    wire difftest_sampling = difftest_enable;
`endif //ASYNC_CLK_2N

    wire can_send = buffer_valid[rd_buf];
    wire last_pkt = rd_pkt_cnt == NUM_PACKETS_PER_BUFFER;
    // Each package has AXIS_SEND_LEN send
    wire last_send = datalen == (AXIS_SEND_LEN - 1);

    always @(posedge clock) begin
        if (reset)
            current_state <= IDLE;
        else
            current_state <= next_state;
    end

/* verilator lint_off CASEINCOMPLETE */
    always @(*) begin
        case(current_state)
            IDLE:     next_state = can_send ? TRANSFER : IDLE;
            TRANSFER: next_state = (axi_tready & axi_tvalid & last_pkt & last_send) ? DONE : TRANSFER;
            DONE:     next_state = IDLE;
            default:  next_state = IDLE;
        endcase
    end

    // Write Buffer: record data from difftest
    always @(posedge clock) begin
        if (reset) begin
            wr_buf <= 0;
            wr_pkt_cnt <= 0;
            buffer_valid <= 2'b00;
        end else begin
            wr_en[1:0] <= 2'b00;
            if (difftest_sampling & reg_core_clock_enable) begin
                dual_buffer_wr_addr <= wr_pkt_cnt[2:0];
                dual_buffer_wr_data <= difftest_data;
                wr_pkt_cnt <= wr_pkt_cnt + 1'b1;
                wr_en[wr_buf] <= 1;
                if (wr_pkt_cnt == NUM_PACKETS_PER_BUFFER - 1) begin
                    buffer_valid[wr_buf] <= 1;
                    wr_pkt_cnt <= 0;
                    wr_buf <= ~wr_buf; // Switch buffers
                end
            end
            if (next_state == DONE) begin // Releasing a buffer
                buffer_valid[rd_buf] <= 0;
            end
        end
    end

    always @(posedge clock) begin // Back pressure control
        if (reset) begin
            reg_core_clock_enable <= 1'b1;
        end else begin
            reg_core_clock_enable <= ~both_full & ~(wr_pkt_cnt == NUM_PACKETS_PER_BUFFER - 1 & difftest_sampling);
        end
    end

    // Read buffer: send data to axi
    always @(posedge clock) begin
        if(reset) begin
            reg_axi_tvalid <= 0;
            reg_axi_tlast <= 0;
            rd_buf <= 0;
            rd_pkt_cnt <= 0;
            data_num <= 0;
            datalen <= 0;
        end else begin
            case(current_state)
            IDLE : begin
                if (can_send) begin
                    mix_data <= {dual_buffer_rd_data_mux, data_num};
                    rd_pkt_cnt <= 1;
                    data_num <= data_num + 1'b1;
                end
            end
            TRANSFER: begin
                if(axi_tready && axi_tvalid) begin
                    reg_axi_tdata <= mix_data[AXIS_DATA_WIDTH - 1:0];
                    if(last_pkt & last_send) begin
                        reg_axi_tlast <= 1;
                        rd_pkt_cnt <= 0;
                        datalen <= 0;
                    end else if (!last_pkt & last_send) begin
                        mix_data <= {dual_buffer_rd_data_mux, 8'b0};
                        rd_pkt_cnt <= rd_pkt_cnt + 1'b1;
                        datalen <= 0;
                    end else begin
                        datalen <= datalen + 1'b1;
                        mix_data <= mix_data >> AXIS_DATA_WIDTH;
                    end
                end else if (~reg_axi_tvalid)begin
                    reg_axi_tvalid <= 1;
                    reg_axi_tdata <= mix_data[AXIS_DATA_WIDTH - 1:0];
                    mix_data <= mix_data >> AXIS_DATA_WIDTH;
                    datalen <= datalen + 1'b1;
                end
            end
            DONE: begin
                reg_axi_tvalid <= 0;
                reg_axi_tlast <= 0;
                rd_buf <= ~rd_buf;
            end
            endcase
        end
    end
/* verilator lint_on CASEINCOMPLETE */
endmodule
