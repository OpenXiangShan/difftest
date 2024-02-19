`define MEM_DPIC
// The memory used to load the checkpoint
`ifndef MEM_DPIC
import "DPI-C" function void init_ram(input longint size);
import "DPI-C" function void ram_set_bin_file(string bin);
import "DPI-C" function void ram_set_gcpt_bin(string bin);
import "DPI-C" function void ram_free();
`else
import "DPI-C" function longint difftest_ram_read(input longint addr);
import "DPI-C" function void difftest_ram_write(input longint addr,input longint w_mask,input longint data);
`endif // MEM_DPIC
module MemRWHelper(

input             r_enable,
input      [63:0] r_index,
output reg [63:0] r_data,


input         w_enable,
input  [63:0] w_index,
input  [63:0] w_data,
input  [63:0] w_mask,

  input enable,
  input clock
);

`ifndef MEM_DPIC
string bin_file;
string gcpt_bin_file;
string mem_record_file;

`define RAM_SIZE_BYTE (8 * 1024 * 1024 * 1024)// 8G
`define RAM_SIZE_BITS_1G (1 * 1024 * 1024 * 1024)/8// 1G
`define RAM_SIZE_BITS_2G `RAM_SIZE_BITS_1G * 2
`define RAM_SIZE_BITS_3G `RAM_SIZE_BITS_1G * 3
`define RAM_SIZE_BITS_4G `RAM_SIZE_BITS_1G * 4

  reg [63:0] memory_1 [0 : `RAM_SIZE_BITS_1G - 1];
  reg [63:0] memory_2 [0 : `RAM_SIZE_BITS_1G - 1];
  reg [63:0] memory_3 [0 : `RAM_SIZE_BITS_1G - 1];
  reg [63:0] memory_4 [0 : `RAM_SIZE_BITS_1G - 1];

  wire [2:0]r_addr_mux = r_index[29:27];
  wire [2:0]w_addr_mux = w_index[29:27];
`endif // MEM_DPIC

  // memory read
  always @(posedge clock) begin
    if (r_enable && enable) begin    
`ifndef MEM_DPIC
      case (r_addr_mux)
        'b000: begin r_data <= memory_1[r_index[26:0]]; end
        'b001: begin r_data <= memory_2[r_index[26:0]]; end
        'b010: begin r_data <= memory_3[r_index[26:0]]; end
        'b011: begin r_data <= memory_4[r_index[26:0]]; end
        default:begin 
          $fatal(1, "An address that exceeds the memory limit %h",r_index);
        end
      endcase
`else
    r_data <= difftest_ram_read(r_index);
`endif // MEM_DPIC
    end
  end

  // memory write
  always @(posedge clock) begin
    if (w_enable && enable) begin
`ifndef MEM_DPIC
      case (w_addr_mux)
        'b000: begin memory_1[w_index[26:0]] <= (w_data & w_mask) | (memory_1[w_index[26:0]] & ~w_mask); end
        'b001: begin memory_2[w_index[26:0]] <= (w_data & w_mask) | (memory_2[w_index[26:0]] & ~w_mask); end
        'b010: begin memory_3[w_index[26:0]] <= (w_data & w_mask) | (memory_3[w_index[26:0]] & ~w_mask); end
        'b011: begin memory_4[w_index[26:0]] <= (w_data & w_mask) | (memory_4[w_index[26:0]] & ~w_mask); end
        default:begin
          $fatal(1, "An address that exceeds the memory limit %h",w_index);
        end
      endcase
`else
      difftest_ram_write(w_index, w_mask, w_data);
`endif // MEM_DPIC
    end
  end

`ifndef MEM_DPIC
  `define MEM_BLOACK_SIZE (32 * 1024 * 1024) / 8 // 32MB
  integer memory_record_file = 0, byte_read = 1;
  string  file_line;
  integer memory_load_base = 0, memory_record_en = 0, memory_load_addr = 0;
  integer i, j;
  integer mem_record;
  initial begin
  // workload: bin file
  if ($test$plusargs("workload")) begin
    $value$plusargs("workload=%s", bin_file);
    ram_set_bin_file(bin_file);
  end
  // override gcpt :bin file
  if ($test$plusargs("gcpt-bin")) begin
    $value$plusargs("gcpt-bin=%s", gcpt_bin_file);
    ram_set_gcpt_bin(gcpt_bin_file);
  end
  init_ram(`RAM_SIZE_BYTE);

  if ($test$plusargs("mem-record")) begin // use checkpoint compression
    $value$plusargs("mem-record=%s", mem_record_file);
    memory_record_en = 1;
  end else begin
    $display("not set mem-record file");
  end

  if(memory_record_en == 1) begin
    memory_record_file = $fopen(mem_record_file, "r+");

    if (memory_record_file == 0) begin
      $display("Error: failed to open %s", memory_record_file);
      $finish;
    end

    while (byte_read != -1) begin
      byte_read = $fgets(file_line,memory_record_file);
      if (byte_read==0) break;
      $sscanf(file_line, "%d", mem_record); // get mem block

      $display("mem load block %d bloack size %d", mem_record, `MEM_BLOACK_SIZE);
      memory_load_base = mem_record * `MEM_BLOACK_SIZE;
      if ((memory_load_base >= 'd0) && (memory_load_base < `RAM_SIZE_BITS_1G)) begin
        $display("mem load mem1 %x", memory_load_base);
        for (j = 0;j < `MEM_BLOACK_SIZE; j ++) begin
          memory_load_addr = memory_load_base  + j;
          ram_read_data(memory_load_addr, memory_1[memory_load_addr]);
        end
      end
      else if((memory_load_base >= `RAM_SIZE_BITS_1G) && (memory_load_base < `RAM_SIZE_BITS_2G)) begin
        $display("mem load mem2 %x", memory_load_base);
        for (j = 0;j < `MEM_BLOACK_SIZE; j ++) begin
          memory_load_addr = memory_load_base + j;
          ram_read_data(memory_load_addr, memory_2[memory_load_addr - `RAM_SIZE_BITS_1G]);
        end 
      end
      else if((memory_load_base >= `RAM_SIZE_BITS_2G) && (memory_load_base < `RAM_SIZE_BITS_3G)) begin
        $display("mem load mem3 %x", memory_load_base);
        for (j = 0;j < `MEM_BLOACK_SIZE; j ++) begin
          memory_load_addr = memory_load_base + j;
          ram_read_data(memory_load_addr, memory_3[memory_load_addr - `RAM_SIZE_BITS_2G]);
        end 
      end
      else if((memory_load_base >= `RAM_SIZE_BITS_3G) && (memory_load_base < `RAM_SIZE_BITS_4G)) begin
        $display("mem load mem4 %x", memory_load_base);
        for (j = 0;j < `MEM_BLOACK_SIZE; j ++) begin
          memory_load_addr = memory_load_base + j;
          ram_read_data(memory_load_addr, memory_4[memory_load_addr - `RAM_SIZE_BITS_3G]);
        end 
      end

    end

    $fclose(memory_record_file);

  end
  else begin // all init
    for (i = 0;i < `RAM_SIZE_BITS_1G; i ++) begin
      ram_read_data(i , memory_1[i]);
    end

    for (i = 0;i < `RAM_SIZE_BITS_1G; i ++) begin
      ram_read_data(i + `RAM_SIZE_BITS_1G, memory_2[i]);
    end

    for (i = 0;i < `RAM_SIZE_BITS_1G; i ++) begin
      ram_read_data(i + `RAM_SIZE_BITS_2G, memory_3[i]);
    end

    for (i = 0;i < `RAM_SIZE_BITS_1G; i ++) begin
      ram_read_data(i + `RAM_SIZE_BITS_3G, memory_4[i]);
    end
  end
  end // end init
`endif // MEM_DPIC
endmodule
