/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
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

typedef struct packed {
  logic [63:0] instr_pc_va;
  logic [63:0] instr_pc_pa;
  logic [63:0] memory_addr_va;
  logic [63:0] memory_addr_pa;
  logic [63:0] target;
  logic [31:0] instr;
  logic [3:0]  memory_type;
  logic [3:0]  memory_size;
  logic [7:0]  branch_type;
  logic [7:0]  branch_taken;
  logic [7:0]  exception;
} TraceInstructionSV;

module TraceRTL_FileReader(
  input  wire        clock,
  input  wire        reset,
  input  wire        enable,

  output [63:0] index,
  output [63:0] instr_pc_va_0,
  output [63:0] instr_pc_pa_0,
  output [63:0] memory_addr_va_0,
  output [63:0] memory_addr_pa_0,
  output [63:0] target_0,
  output [31:0] instr_0,
  output [3:0]  memory_type_0,
  output [3:0]  memory_size_0,
  output [7:0]  branch_type_0,
  output [7:0]  branch_taken_0,
  output [7:0]  exception_0,

  output [63:0] instr_pc_va_1,
  output [63:0] instr_pc_pa_1,
  output [63:0] memory_addr_va_1,
  output [63:0] memory_addr_pa_1,
  output [63:0] target_1,
  output [31:0] instr_1,
  output [3:0]  memory_type_1,
  output [3:0]  memory_size_1,
  output [7:0]  branch_type_1,
  output [7:0]  branch_taken_1,
  output [7:0]  exception_1,

  output [63:0] instr_pc_va_2,
  output [63:0] instr_pc_pa_2,
  output [63:0] memory_addr_va_2,
  output [63:0] memory_addr_pa_2,
  output [63:0] target_2,
  output [31:0] instr_2,
  output [3:0]  memory_type_2,
  output [3:0]  memory_size_2,
  output [7:0]  branch_type_2,
  output [7:0]  branch_taken_2,
  output [7:0]  exception_2,

  output [63:0] instr_pc_va_3,
  output [63:0] instr_pc_pa_3,
  output [63:0] memory_addr_va_3,
  output [63:0] memory_addr_pa_3,
  output [63:0] target_3,
  output [31:0] instr_3,
  output [3:0]  memory_type_3,
  output [3:0]  memory_size_3,
  output [7:0]  branch_type_3,
  output [7:0]  branch_taken_3,
  output [7:0]  exception_3,

  output [63:0] instr_pc_va_4,
  output [63:0] instr_pc_pa_4,
  output [63:0] memory_addr_va_4,
  output [63:0] memory_addr_pa_4,
  output [63:0] target_4,
  output [31:0] instr_4,
  output [3:0]  memory_type_4,
  output [3:0]  memory_size_4,
  output [7:0]  branch_type_4,
  output [7:0]  branch_taken_4,
  output [7:0]  exception_4,

  output [63:0] instr_pc_va_5,
  output [63:0] instr_pc_pa_5,
  output [63:0] memory_addr_va_5,
  output [63:0] memory_addr_pa_5,
  output [63:0] target_5,
  output [31:0] instr_5,
  output [3:0]  memory_type_5,
  output [3:0]  memory_size_5,
  output [7:0]  branch_type_5,
  output [7:0]  branch_taken_5,
  output [7:0]  exception_5,

  output [63:0] instr_pc_va_6,
  output [63:0] instr_pc_pa_6,
  output [63:0] memory_addr_va_6,
  output [63:0] memory_addr_pa_6,
  output [63:0] target_6,
  output [31:0] instr_6,
  output [3:0]  memory_type_6,
  output [3:0]  memory_size_6,
  output [7:0]  branch_type_6,
  output [7:0]  branch_taken_6,
  output [7:0]  exception_6,

  output [63:0] instr_pc_va_7,
  output [63:0] instr_pc_pa_7,
  output [63:0] memory_addr_va_7,
  output [63:0] memory_addr_pa_7,
  output [63:0] target_7,
  output [31:0] instr_7,
  output [3:0]  memory_type_7,
  output [3:0]  memory_size_7,
  output [7:0]  branch_type_7,
  output [7:0]  branch_taken_7,
  output [7:0]  exception_7,

  output [63:0] instr_pc_va_8,
  output [63:0] instr_pc_pa_8,
  output [63:0] memory_addr_va_8,
  output [63:0] memory_addr_pa_8,
  output [63:0] target_8,
  output [31:0] instr_8,
  output [3:0]  memory_type_8,
  output [3:0]  memory_size_8,
  output [7:0]  branch_type_8,
  output [7:0]  branch_taken_8,
  output [7:0]  exception_8,

  output [63:0] instr_pc_va_9,
  output [63:0] instr_pc_pa_9,
  output [63:0] memory_addr_va_9,
  output [63:0] memory_addr_pa_9,
  output [63:0] target_9,
  output [31:0] instr_9,
  output [3:0]  memory_type_9,
  output [3:0]  memory_size_9,
  output [7:0]  branch_type_9,
  output [7:0]  branch_taken_9,
  output [7:0]  exception_9,

  output [63:0] instr_pc_va_10,
  output [63:0] instr_pc_pa_10,
  output [63:0] memory_addr_va_10,
  output [63:0] memory_addr_pa_10,
  output [63:0] target_10,
  output [31:0] instr_10,
  output [3:0]  memory_type_10,
  output [3:0]  memory_size_10,
  output [7:0]  branch_type_10,
  output [7:0]  branch_taken_10,
  output [7:0]  exception_10,

  output [63:0] instr_pc_va_11,
  output [63:0] instr_pc_pa_11,
  output [63:0] memory_addr_va_11,
  output [63:0] memory_addr_pa_11,
  output [63:0] target_11,
  output [31:0] instr_11,
  output [3:0]  memory_type_11,
  output [3:0]  memory_size_11,
  output [7:0]  branch_type_11,
  output [7:0]  branch_taken_11,
  output [7:0]  exception_11,

  output [63:0] instr_pc_va_12,
  output [63:0] instr_pc_pa_12,
  output [63:0] memory_addr_va_12,
  output [63:0] memory_addr_pa_12,
  output [63:0] target_12,
  output [31:0] instr_12,
  output [3:0]  memory_type_12,
  output [3:0]  memory_size_12,
  output [7:0]  branch_type_12,
  output [7:0]  branch_taken_12,
  output [7:0]  exception_12,

  output [63:0] instr_pc_va_13,
  output [63:0] instr_pc_pa_13,
  output [63:0] memory_addr_va_13,
  output [63:0] memory_addr_pa_13,
  output [63:0] target_13,
  output [31:0] instr_13,
  output [3:0]  memory_type_13,
  output [3:0]  memory_size_13,
  output [7:0]  branch_type_13,
  output [7:0]  branch_taken_13,
  output [7:0]  exception_13,

  output [63:0] instr_pc_va_14,
  output [63:0] instr_pc_pa_14,
  output [63:0] memory_addr_va_14,
  output [63:0] memory_addr_pa_14,
  output [63:0] target_14,
  output [31:0] instr_14,
  output [3:0]  memory_type_14,
  output [3:0]  memory_size_14,
  output [7:0]  branch_type_14,
  output [7:0]  branch_taken_14,
  output [7:0]  exception_14,

  output [63:0] instr_pc_va_15,
  output [63:0] instr_pc_pa_15,
  output [63:0] memory_addr_va_15,
  output [63:0] memory_addr_pa_15,
  output [63:0] target_15,
  output [31:0] instr_15,
  output [3:0]  memory_type_15,
  output [3:0]  memory_size_15,
  output [7:0]  branch_type_15,
  output [7:0]  branch_taken_15,
  output [7:0]  exception_15,

  input [63:0] redirect_instID,
  input        redirect_valid,

  input        workingState
);

localparam int STRUCT_SIZE_BYTES = 48; // sizeof(TraceInstructionSV) in bytes
// 64*5 + 32 + 4*2 + 8*3 =
localparam int NUM_PORTS = 16;
`ifdef PALLADIUM
localparam longint MAX_TRACE_INSTR_NUM = 40000000;
`else
localparam longint MAX_TRACE_INSTR_NUM = 10000000;
`endif
// parameter integer MAX_TRACE_INSTR_NUM = 1000;

TraceInstructionSV trace_instructions[MAX_TRACE_INSTR_NUM];
logic [63:0] actual_instr_num;

initial begin
  int fd;
  int bytes_read;
  logic [7:0] temp_buffer[STRUCT_SIZE_BYTES-1:0];

  string file_name;
  if ($test$plusargs("tracertl-file")) begin
    $value$plusargs("tracertl-file=%s", file_name);
  end
  else begin
    $display("Error: No tracertl-file specified, using default %s", file_name);
    $finish;
  end

  fd = $fopen(file_name, "rb");
  if (fd == 0) begin
    $display("Error: Could not open file %s", file_name);
    $finish;
  end

  $display("VCS Opened trace-file %s successfully", file_name);

  // TOOD: change the below STRUCT_SIZE_BYTES to fsize
  for (longint i = 0; i < MAX_TRACE_INSTR_NUM; i++) begin

    bytes_read = $fread(temp_buffer, fd, 0, STRUCT_SIZE_BYTES);
    if (bytes_read != STRUCT_SIZE_BYTES) begin
      // TODO: change to break
      if (i == 0) begin
        $display("Error: Could not read enough bytes for instruction. %0d", bytes_read);
        $finish;
      end
      else begin
        break;
      end
    end

    actual_instr_num = i+1;
    // trace_instructions[i] = {>>{temp_buffer}};
    trace_instructions[i].instr_pc_va = {temp_buffer[7], temp_buffer[6], temp_buffer[5], temp_buffer[4],
                                         temp_buffer[3], temp_buffer[2], temp_buffer[1], temp_buffer[0]};
    trace_instructions[i].instr_pc_pa = {temp_buffer[15], temp_buffer[14], temp_buffer[13], temp_buffer[12],
                                          temp_buffer[11], temp_buffer[10], temp_buffer[9], temp_buffer[8]};
    trace_instructions[i].memory_addr_va = {temp_buffer[23], temp_buffer[22], temp_buffer[21], temp_buffer[20],
                                             temp_buffer[19], temp_buffer[18], temp_buffer[17], temp_buffer[16]};
    trace_instructions[i].memory_addr_pa = {temp_buffer[31], temp_buffer[30], temp_buffer[29], temp_buffer[28],
                                              temp_buffer[27], temp_buffer[26], temp_buffer[25], temp_buffer[24]};
    trace_instructions[i].target = {temp_buffer[39], temp_buffer[38], temp_buffer[37], temp_buffer[36],
                                    temp_buffer[35], temp_buffer[34], temp_buffer[33], temp_buffer[32]};
    trace_instructions[i].instr = {temp_buffer[43], temp_buffer[42], temp_buffer[41], temp_buffer[40]};
    trace_instructions[i].memory_type = {temp_buffer[44][3:0]}; // TODO: may be wrong order
    trace_instructions[i].memory_size = {temp_buffer[44][7:4]};
    trace_instructions[i].branch_type = {temp_buffer[45][7:0]};
    trace_instructions[i].branch_taken = {temp_buffer[46][7:0]};
    trace_instructions[i].exception = {temp_buffer[47][7:0]};

    if (i == 0) begin
      $display("Read struct %0d:", i);
      $display("  instr_pc_va  = 0x%h", trace_instructions[i].instr_pc_va);
      $display("  instr_pc_pa  = 0x%h", trace_instructions[i].instr_pc_pa);
      $display("  memory_addr_va = 0x%h", trace_instructions[i].memory_addr_va);
      $display("  memory_addr_pa = 0x%h", trace_instructions[i].memory_addr_pa);
      $display("  target       = 0x%h", trace_instructions[i].target);
      $display("  instr        = 0x%h", trace_instructions[i].instr);
      $display("  memory_type  = 0x%h", trace_instructions[i].memory_type);
      $display("  memory_size  = 0x%h", trace_instructions[i].memory_size);
      $display("  branch_type  = 0x%h", trace_instructions[i].branch_type);
      $display("  branch_taken = 0x%h", trace_instructions[i].branch_taken);
      $display("  exception    = 0x%h", trace_instructions[i].exception);
      $display("------------------------------------");
    end
  end


  $fclose(fd);
  $display("Finished reading file %s, %d instructions are readed.", file_name, actual_instr_num);
end


reg [63:0] index_r;
always @(posedge clock) begin
  if (reset) begin
    index_r <= 0;
  end
  else if (index_r > (actual_instr_num + 1000)) begin
    index_r <= 0; // correct wrong initial value, unsolved
  end
  else if (workingState && ((index_r+NUM_PORTS) >= actual_instr_num)) begin
    $display("Reached end of trace instructions at index %0d", index_r);
  // else if ((index_r+NUM_PORTS) >= 1000) begin
    // $display("Reached end of trace instructions at index %0d", 1000);
    $finish;
  end
  else if (redirect_valid) begin
    index_r <= redirect_instID;
  end
  else if (enable) begin
    index_r <= index_r + NUM_PORTS;
  end
  else begin
    index_r <= index_r;
  end
end

reg [63:0] print_count;
always @(posedge clock) begin
  if (reset) begin
    print_count <= 10000;
  end
  else if (index_r > print_count) begin
    $display("Current fetched instruction: %0d", index_r);
    print_count <= print_count + 10000;
  end
  else begin
    print_count <= print_count;
  end
end

assign index = index_r;

assign instr_pc_va_0 = trace_instructions[index_r + 0].instr_pc_va;
assign instr_pc_pa_0 = trace_instructions[index_r + 0].instr_pc_pa;
assign memory_addr_va_0 = trace_instructions[index_r + 0].memory_addr_va;
assign memory_addr_pa_0 = trace_instructions[index_r + 0].memory_addr_pa;
assign target_0 = trace_instructions[index_r + 0].target;
assign instr_0 = trace_instructions[index_r + 0].instr;
assign memory_type_0 = trace_instructions[index_r + 0].memory_type;
assign memory_size_0 = trace_instructions[index_r + 0].memory_size;
assign branch_type_0 = trace_instructions[index_r + 0].branch_type;
assign branch_taken_0 = trace_instructions[index_r + 0].branch_taken;
assign exception_0 = trace_instructions[index_r + 0].exception;


assign instr_pc_va_1 = trace_instructions[index_r + 1].instr_pc_va;
assign instr_pc_pa_1 = trace_instructions[index_r + 1].instr_pc_pa;
assign memory_addr_va_1 = trace_instructions[index_r + 1].memory_addr_va;
assign memory_addr_pa_1 = trace_instructions[index_r + 1].memory_addr_pa;
assign target_1 = trace_instructions[index_r + 1].target;
assign instr_1 = trace_instructions[index_r + 1].instr;
assign memory_type_1 = trace_instructions[index_r + 1].memory_type;
assign memory_size_1 = trace_instructions[index_r + 1].memory_size;
assign branch_type_1 = trace_instructions[index_r + 1].branch_type;
assign branch_taken_1 = trace_instructions[index_r + 1].branch_taken;
assign exception_1 = trace_instructions[index_r + 1].exception;


assign instr_pc_va_2 = trace_instructions[index_r + 2].instr_pc_va;
assign instr_pc_pa_2 = trace_instructions[index_r + 2].instr_pc_pa;
assign memory_addr_va_2 = trace_instructions[index_r + 2].memory_addr_va;
assign memory_addr_pa_2 = trace_instructions[index_r + 2].memory_addr_pa;
assign target_2 = trace_instructions[index_r + 2].target;
assign instr_2 = trace_instructions[index_r + 2].instr;
assign memory_type_2 = trace_instructions[index_r + 2].memory_type;
assign memory_size_2 = trace_instructions[index_r + 2].memory_size;
assign branch_type_2 = trace_instructions[index_r + 2].branch_type;
assign branch_taken_2 = trace_instructions[index_r + 2].branch_taken;
assign exception_2 = trace_instructions[index_r + 2].exception;


assign instr_pc_va_3 = trace_instructions[index_r + 3].instr_pc_va;
assign instr_pc_pa_3 = trace_instructions[index_r + 3].instr_pc_pa;
assign memory_addr_va_3 = trace_instructions[index_r + 3].memory_addr_va;
assign memory_addr_pa_3 = trace_instructions[index_r + 3].memory_addr_pa;
assign target_3 = trace_instructions[index_r + 3].target;
assign instr_3 = trace_instructions[index_r + 3].instr;
assign memory_type_3 = trace_instructions[index_r + 3].memory_type;
assign memory_size_3 = trace_instructions[index_r + 3].memory_size;
assign branch_type_3 = trace_instructions[index_r + 3].branch_type;
assign branch_taken_3 = trace_instructions[index_r + 3].branch_taken;
assign exception_3 = trace_instructions[index_r + 3].exception;


assign instr_pc_va_4 = trace_instructions[index_r + 4].instr_pc_va;
assign instr_pc_pa_4 = trace_instructions[index_r + 4].instr_pc_pa;
assign memory_addr_va_4 = trace_instructions[index_r + 4].memory_addr_va;
assign memory_addr_pa_4 = trace_instructions[index_r + 4].memory_addr_pa;
assign target_4 = trace_instructions[index_r + 4].target;
assign instr_4 = trace_instructions[index_r + 4].instr;
assign memory_type_4 = trace_instructions[index_r + 4].memory_type;
assign memory_size_4 = trace_instructions[index_r + 4].memory_size;
assign branch_type_4 = trace_instructions[index_r + 4].branch_type;
assign branch_taken_4 = trace_instructions[index_r + 4].branch_taken;
assign exception_4 = trace_instructions[index_r + 4].exception;


assign instr_pc_va_5 = trace_instructions[index_r + 5].instr_pc_va;
assign instr_pc_pa_5 = trace_instructions[index_r + 5].instr_pc_pa;
assign memory_addr_va_5 = trace_instructions[index_r + 5].memory_addr_va;
assign memory_addr_pa_5 = trace_instructions[index_r + 5].memory_addr_pa;
assign target_5 = trace_instructions[index_r + 5].target;
assign instr_5 = trace_instructions[index_r + 5].instr;
assign memory_type_5 = trace_instructions[index_r + 5].memory_type;
assign memory_size_5 = trace_instructions[index_r + 5].memory_size;
assign branch_type_5 = trace_instructions[index_r + 5].branch_type;
assign branch_taken_5 = trace_instructions[index_r + 5].branch_taken;
assign exception_5 = trace_instructions[index_r + 5].exception;


assign instr_pc_va_6 = trace_instructions[index_r + 6].instr_pc_va;
assign instr_pc_pa_6 = trace_instructions[index_r + 6].instr_pc_pa;
assign memory_addr_va_6 = trace_instructions[index_r + 6].memory_addr_va;
assign memory_addr_pa_6 = trace_instructions[index_r + 6].memory_addr_pa;
assign target_6 = trace_instructions[index_r + 6].target;
assign instr_6 = trace_instructions[index_r + 6].instr;
assign memory_type_6 = trace_instructions[index_r + 6].memory_type;
assign memory_size_6 = trace_instructions[index_r + 6].memory_size;
assign branch_type_6 = trace_instructions[index_r + 6].branch_type;
assign branch_taken_6 = trace_instructions[index_r + 6].branch_taken;
assign exception_6 = trace_instructions[index_r + 6].exception;


assign instr_pc_va_7 = trace_instructions[index_r + 7].instr_pc_va;
assign instr_pc_pa_7 = trace_instructions[index_r + 7].instr_pc_pa;
assign memory_addr_va_7 = trace_instructions[index_r + 7].memory_addr_va;
assign memory_addr_pa_7 = trace_instructions[index_r + 7].memory_addr_pa;
assign target_7 = trace_instructions[index_r + 7].target;
assign instr_7 = trace_instructions[index_r + 7].instr;
assign memory_type_7 = trace_instructions[index_r + 7].memory_type;
assign memory_size_7 = trace_instructions[index_r + 7].memory_size;
assign branch_type_7 = trace_instructions[index_r + 7].branch_type;
assign branch_taken_7 = trace_instructions[index_r + 7].branch_taken;
assign exception_7 = trace_instructions[index_r + 7].exception;


assign instr_pc_va_8 = trace_instructions[index_r + 8].instr_pc_va;
assign instr_pc_pa_8 = trace_instructions[index_r + 8].instr_pc_pa;
assign memory_addr_va_8 = trace_instructions[index_r + 8].memory_addr_va;
assign memory_addr_pa_8 = trace_instructions[index_r + 8].memory_addr_pa;
assign target_8 = trace_instructions[index_r + 8].target;
assign instr_8 = trace_instructions[index_r + 8].instr;
assign memory_type_8 = trace_instructions[index_r + 8].memory_type;
assign memory_size_8 = trace_instructions[index_r + 8].memory_size;
assign branch_type_8 = trace_instructions[index_r + 8].branch_type;
assign branch_taken_8 = trace_instructions[index_r + 8].branch_taken;
assign exception_8 = trace_instructions[index_r + 8].exception;


assign instr_pc_va_9 = trace_instructions[index_r + 9].instr_pc_va;
assign instr_pc_pa_9 = trace_instructions[index_r + 9].instr_pc_pa;
assign memory_addr_va_9 = trace_instructions[index_r + 9].memory_addr_va;
assign memory_addr_pa_9 = trace_instructions[index_r + 9].memory_addr_pa;
assign target_9 = trace_instructions[index_r + 9].target;
assign instr_9 = trace_instructions[index_r + 9].instr;
assign memory_type_9 = trace_instructions[index_r + 9].memory_type;
assign memory_size_9 = trace_instructions[index_r + 9].memory_size;
assign branch_type_9 = trace_instructions[index_r + 9].branch_type;
assign branch_taken_9 = trace_instructions[index_r + 9].branch_taken;
assign exception_9 = trace_instructions[index_r + 9].exception;


assign instr_pc_va_10 = trace_instructions[index_r + 10].instr_pc_va;
assign instr_pc_pa_10 = trace_instructions[index_r + 10].instr_pc_pa;
assign memory_addr_va_10 = trace_instructions[index_r + 10].memory_addr_va;
assign memory_addr_pa_10 = trace_instructions[index_r + 10].memory_addr_pa;
assign target_10 = trace_instructions[index_r + 10].target;
assign instr_10 = trace_instructions[index_r + 10].instr;
assign memory_type_10 = trace_instructions[index_r + 10].memory_type;
assign memory_size_10 = trace_instructions[index_r + 10].memory_size;
assign branch_type_10 = trace_instructions[index_r + 10].branch_type;
assign branch_taken_10 = trace_instructions[index_r + 10].branch_taken;
assign exception_10 = trace_instructions[index_r + 10].exception;


assign instr_pc_va_11 = trace_instructions[index_r + 11].instr_pc_va;
assign instr_pc_pa_11 = trace_instructions[index_r + 11].instr_pc_pa;
assign memory_addr_va_11 = trace_instructions[index_r + 11].memory_addr_va;
assign memory_addr_pa_11 = trace_instructions[index_r + 11].memory_addr_pa;
assign target_11 = trace_instructions[index_r + 11].target;
assign instr_11 = trace_instructions[index_r + 11].instr;
assign memory_type_11 = trace_instructions[index_r + 11].memory_type;
assign memory_size_11 = trace_instructions[index_r + 11].memory_size;
assign branch_type_11 = trace_instructions[index_r + 11].branch_type;
assign branch_taken_11 = trace_instructions[index_r + 11].branch_taken;
assign exception_11 = trace_instructions[index_r + 11].exception;


assign instr_pc_va_12 = trace_instructions[index_r + 12].instr_pc_va;
assign instr_pc_pa_12 = trace_instructions[index_r + 12].instr_pc_pa;
assign memory_addr_va_12 = trace_instructions[index_r + 12].memory_addr_va;
assign memory_addr_pa_12 = trace_instructions[index_r + 12].memory_addr_pa;
assign target_12 = trace_instructions[index_r + 12].target;
assign instr_12 = trace_instructions[index_r + 12].instr;
assign memory_type_12 = trace_instructions[index_r + 12].memory_type;
assign memory_size_12 = trace_instructions[index_r + 12].memory_size;
assign branch_type_12 = trace_instructions[index_r + 12].branch_type;
assign branch_taken_12 = trace_instructions[index_r + 12].branch_taken;
assign exception_12 = trace_instructions[index_r + 12].exception;


assign instr_pc_va_13 = trace_instructions[index_r + 13].instr_pc_va;
assign instr_pc_pa_13 = trace_instructions[index_r + 13].instr_pc_pa;
assign memory_addr_va_13 = trace_instructions[index_r + 13].memory_addr_va;
assign memory_addr_pa_13 = trace_instructions[index_r + 13].memory_addr_pa;
assign target_13 = trace_instructions[index_r + 13].target;
assign instr_13 = trace_instructions[index_r + 13].instr;
assign memory_type_13 = trace_instructions[index_r + 13].memory_type;
assign memory_size_13 = trace_instructions[index_r + 13].memory_size;
assign branch_type_13 = trace_instructions[index_r + 13].branch_type;
assign branch_taken_13 = trace_instructions[index_r + 13].branch_taken;
assign exception_13 = trace_instructions[index_r + 13].exception;


assign instr_pc_va_14 = trace_instructions[index_r + 14].instr_pc_va;
assign instr_pc_pa_14 = trace_instructions[index_r + 14].instr_pc_pa;
assign memory_addr_va_14 = trace_instructions[index_r + 14].memory_addr_va;
assign memory_addr_pa_14 = trace_instructions[index_r + 14].memory_addr_pa;
assign target_14 = trace_instructions[index_r + 14].target;
assign instr_14 = trace_instructions[index_r + 14].instr;
assign memory_type_14 = trace_instructions[index_r + 14].memory_type;
assign memory_size_14 = trace_instructions[index_r + 14].memory_size;
assign branch_type_14 = trace_instructions[index_r + 14].branch_type;
assign branch_taken_14 = trace_instructions[index_r + 14].branch_taken;
assign exception_14 = trace_instructions[index_r + 14].exception;


assign instr_pc_va_15 = trace_instructions[index_r + 15].instr_pc_va;
assign instr_pc_pa_15 = trace_instructions[index_r + 15].instr_pc_pa;
assign memory_addr_va_15 = trace_instructions[index_r + 15].memory_addr_va;
assign memory_addr_pa_15 = trace_instructions[index_r + 15].memory_addr_pa;
assign target_15 = trace_instructions[index_r + 15].target;
assign instr_15 = trace_instructions[index_r + 15].instr;
assign memory_type_15 = trace_instructions[index_r + 15].memory_type;
assign memory_size_15 = trace_instructions[index_r + 15].memory_size;
assign branch_type_15 = trace_instructions[index_r + 15].branch_type;
assign branch_taken_15 = trace_instructions[index_r + 15].branch_taken;
assign exception_15 = trace_instructions[index_r + 15].exception;

endmodule

