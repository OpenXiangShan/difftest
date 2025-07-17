/*
 * Copyright (C) 2025 Beijing Institute of Open Source Chip
 *
 * Description: AXIS data package helper for FPGA-difftest
 * Author: XiangShan Infra Group
 * Date: Tue Feb 18 15:52:51 2025
 */
`timescale 1ns / 1ps

//`define ASYN_SEND_DATA
module axis_data_packge #(
    parameter DATA_WIDTH = 16000,
    parameter AXIS_DATA_WIDTH = 512
)(
  input                        m_axis_c2h_aclk,    //axi
  input                        m_axis_c2h_aresetn, //axi
  output [AXIS_DATA_WIDTH-1:0] m_axis_c2h_tdata,
  output [63:0]                m_axis_c2h_tkeep,
  output                       m_axis_c2h_tlast,
  input                        m_axis_c2h_tready,
  output                       m_axis_c2h_tvalid,

  input                        data_valid,
  output                       data_next,
  input  [DATA_WIDTH-1:0]      data
);
    localparam NUM_PACKETS_PER_BUFFER = 8; // one send packet num
    localparam AXIS_SEND_LEN = ((DATA_WIDTH + AXIS_DATA_WIDTH + 8 - 1) / AXIS_DATA_WIDTH) - 1;

    localparam IDLE = 3'b001;
    localparam TRANSFER = 3'b010;
    localparam DONE = 3'b100;

    reg [2:0] current_state, next_state;

    reg [DATA_WIDTH + 8 - 1:0] mix_data;
    reg [AXIS_DATA_WIDTH - 1:0]  reg_m_axis_c2h_tdata;
    reg [7:0] datalen;
    reg [7:0] data_num;
    reg [4:0] state;

    reg                  reg_m_axis_c2h_tvalid;
    reg                  reg_m_axis_c2h_tlast;
    reg                  reg_data_next;
    // Ping-Pong Buffers
    reg [DATA_WIDTH - 1:0] dual_buffer [1:0][0 : NUM_PACKETS_PER_BUFFER - 1];
    reg current_buffer; // 0 for buffer_0, 1 for buffer_1
    reg this_buffer; // need read buffer
    reg [3:0] wr_pkt_cnt;   // (0~7)
    reg [3:0] rd_pkt_cnt;   // (0~7)
    reg [1:0] buffer_valid;

    wire [AXIS_DATA_WIDTH - 1:0]first_data = {data[AXIS_DATA_WIDTH - 8 - 1:0], data_num};
    assign data_next = reg_data_next;
    assign sstate = state;
    assign m_axis_c2h_tdata = reg_m_axis_c2h_tdata;
    assign m_axis_c2h_tvalid = reg_m_axis_c2h_tvalid;
    assign m_axis_c2h_tkeep = 64'hffffffff_ffffffff;
    assign m_axis_c2h_tlast = reg_m_axis_c2h_tlast;

    wire both_full = buffer_valid[0] & buffer_valid[1];

    // asynchronous clock fetches the signal
`ifdef ASYN_SEND_DATA
    wire [3:0]core_50M_count = 'd3;
    wire [3:0]core_10M_count = 'd7;
    (* ASYNC_REG = "TRUE" *) reg [3:0]core_en_last_count;
    wire core_data_sampling_en = core_en_last_count == core_50M_count;
    always @(posedge m_axis_c2h_aclk) begin
        if (data_valid && state == 'b0) begin
            core_en_last_count <= core_en_last_count + 'b1;
        end else begin
            core_en_last_count <= 'b0;
        end
    end
`else
    wire core_data_sampling_en = data_valid;
`endif // ASYN_SEND_DATA

    wire can_send = this_buffer ? buffer_valid[1] : buffer_valid[0];
    wire can_cont_send = rd_pkt_cnt < NUM_PACKETS_PER_BUFFER;
    wire one_send_last = datalen == AXIS_SEND_LEN;

    always @(posedge m_axis_c2h_aclk) begin
        if (!m_axis_c2h_aresetn)
            current_state <= IDLE;
        else
            current_state <= next_state;
    end

    always @(*) begin
        case(current_state)
            IDLE:     next_state = can_send ? TRANSFER : IDLE;
            TRANSFER: next_state = (m_axis_c2h_tready && reg_m_axis_c2h_tvalid) ?
                                   (!can_cont_send & one_send_last ? DONE : TRANSFER) : TRANSFER;
            DONE:     next_state = IDLE;
            default:  next_state = IDLE;
        endcase
    end

    // data buffer
    always @(posedge m_axis_c2h_aclk) begin
        if (!m_axis_c2h_aresetn) begin
            current_buffer <= 0;
            wr_pkt_cnt <= 0;
            buffer_valid <= 'b00;
        end else begin
            if (data_valid & reg_data_next) begin
                dual_buffer[!current_buffer][wr_pkt_cnt] <= data;
                wr_pkt_cnt <= wr_pkt_cnt + 1'b1;
                if (wr_pkt_cnt == NUM_PACKETS_PER_BUFFER - 1) begin
                    buffer_valid[!current_buffer] <= 1;
                    wr_pkt_cnt <= 0;
                    current_buffer <= ~current_buffer; // Switch buffers
                end
            end
            if (next_state == DONE) begin // Releasing a buffer
                buffer_valid[this_buffer] <= 0;
            end
        end
    end

    always @(posedge m_axis_c2h_aclk) begin // Back pressure control
        if (!m_axis_c2h_aresetn) begin
            reg_data_next <= 1'b1;
        end else begin
            reg_data_next <= ~both_full & ~(wr_pkt_cnt == NUM_PACKETS_PER_BUFFER - 1 & data_valid);
        end
    end

    // data send to axis
    always @(posedge m_axis_c2h_aclk) begin
        if(!m_axis_c2h_aresetn) begin
            reg_m_axis_c2h_tvalid <= 0;
            reg_m_axis_c2h_tlast <= 0;
            datalen <= 0;
            data_num <= 0;
            this_buffer <= 1;
            rd_pkt_cnt <= 0;
        end else begin
            case(current_state)
            IDLE : begin
                if (can_send) begin
                    reg_m_axis_c2h_tdata <= {dual_buffer[this_buffer][0][AXIS_DATA_WIDTH - 8 - 1:0], data_num};
                    mix_data <= dual_buffer[this_buffer][0][DATA_WIDTH - 1:AXIS_DATA_WIDTH - 8];
                    reg_m_axis_c2h_tvalid <= 1;
                    data_num <= data_num + 1'b1;
                    rd_pkt_cnt <= 1;
                    datalen <= 1;
                end
            end
            TRANSFER: begin
                if(m_axis_c2h_tready && reg_m_axis_c2h_tvalid) begin
                    reg_m_axis_c2h_tdata <= mix_data[AXIS_DATA_WIDTH - 1:0];
                    if(!can_cont_send & one_send_last) begin
                        reg_m_axis_c2h_tlast <= 1;
                        rd_pkt_cnt <= 0;
                    end else if (can_cont_send & one_send_last) begin
                        mix_data <= {dual_buffer[this_buffer][rd_pkt_cnt], 8'b0};
                        rd_pkt_cnt <= rd_pkt_cnt + 1'b1;
                        datalen <= 0;
                    end else begin
                        datalen <= datalen + 1'b1;
                        mix_data <= mix_data >> AXIS_DATA_WIDTH;
                    end
                end
            end
            DONE: begin
                reg_m_axis_c2h_tvalid <= 0;
                reg_m_axis_c2h_tlast <= 0;
                datalen <= 0;
                this_buffer <= ~this_buffer;
            end
            endcase
        end
    end
endmodule