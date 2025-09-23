package difftest.fpga.xdma
import chisel3._
import chisel3.util._

class AxisBundle(val dataWidth: Int) extends Bundle {
  val valid = Input(Bool())
  val data = Input(UInt(dataWidth.W))
  val ready = Output(Bool())
  val last = Input(Bool())
}
