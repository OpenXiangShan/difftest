// The memory used to load the checkpoint
import "DPI-C" function void ram_read_data(input longint addr,output longint data);
import "DPI-C" function void init_ram(input longint size);
import "DPI-C" function void ram_set_bin_file(string bin);
import "DPI-C" function void ram_set_gcpt_bin(string bin);
import "DPI-C" function void ram_free();
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
string bin_file;
string gcpt_bin_file;
`define RAM_SIZE_BYTE (8 * 1024 * 1024 * 1024)
`define RAM_SIZE_BITS_1G (1 * 1024 * 1024 * 1024)/8
`define RAM_SIZE_BITS_2G `RAM_SIZE_BITS_1G * 2
`define RAM_SIZE_BITS_3G `RAM_SIZE_BITS_1G * 3
`define RAM_SIZE_BITS_4G `RAM_SIZE_BITS_1G * 4

  reg [63:0] memory_1 [0 : `RAM_SIZE_BITS_1G - 1];
  reg [63:0] memory_2 [0 : `RAM_SIZE_BITS_1G - 1];
  reg [63:0] memory_3 [0 : `RAM_SIZE_BITS_1G - 1];
  reg [63:0] memory_4 [0 : `RAM_SIZE_BITS_1G - 1];

  wire [2:0]r_addr_mux = r_index[29:27];
  wire [2:0]w_addr_mux = w_index[29:27];
  // memory read
  always @(posedge clock) begin
    if (r_enable && enable) begin    
      case (r_addr_mux)
        'b000: begin r_data <= memory_1[r_index[26:0]]; end
        'b001: begin r_data <= memory_2[r_index[26:0]]; end
        'b010: begin r_data <= memory_3[r_index[26:0]]; end
        'b011: begin r_data <= memory_4[r_index[26:0]]; end
        default:begin 
          $fatal(1, "An address that exceeds the memory limit %h",r_index);
        end
      endcase
    end
  end

  // memory write
  always @(posedge clock) begin
    if (w_enable && enable) begin
      case (w_addr_mux)
        'b000: begin memory_1[w_index[26:0]] <= (w_data & w_mask) | (memory_1[w_index[26:0]] & ~w_mask); end
        'b001: begin memory_2[w_index[26:0]] <= (w_data & w_mask) | (memory_2[w_index[26:0]] & ~w_mask); end
        'b010: begin memory_3[w_index[26:0]] <= (w_data & w_mask) | (memory_3[w_index[26:0]] & ~w_mask); end
        'b011: begin memory_4[w_index[26:0]] <= (w_data & w_mask) | (memory_4[w_index[26:0]] & ~w_mask); end
        default:begin
          $fatal(1, "An address that exceeds the memory limit %h",w_index);
        end
      endcase
    end
  end

  integer i;
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

  for (i=0;i < `RAM_SIZE_BITS_1G; i ++) begin
    ram_read_data(i , memory_1[i]);
  end

  for (i=0;i < `RAM_SIZE_BITS_1G; i ++) begin
    ram_read_data(i + `RAM_SIZE_BITS_1G,memory_2[i]);
  end

  for (i=0;i < `RAM_SIZE_BITS_1G; i ++) begin
    ram_read_data(i + `RAM_SIZE_BITS_2G,memory_3[i]);
  end

  for (i=0;i < `RAM_SIZE_BITS_1G; i ++) begin
    ram_read_data(i + `RAM_SIZE_BITS_3G,memory_4[i]);
  end

  end
endmodule
     