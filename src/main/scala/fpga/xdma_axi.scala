package difftest.fpga.xdma
import chisel3._
import chisel3.util._

class AxisMasterBundle(val dataWidth: Int) extends Bundle {
  val valid = Output(Bool())
  val data = Output(UInt(dataWidth.W))
  val ready = Input(Bool())
  val last = Output(Bool())
}

class AXI4LiteReadMasterBundle(val addrWidth: Int, val dataWidth: Int) extends Bundle {
  val addr    = Output(UInt(addrWidth.W))
  val prot    = Output(UInt(3.W))
  val valid   = Output(Bool())
  val ready   = Input(Bool())
  val data    = Input(UInt(dataWidth.W))
}
