package difftest.util.dpic

import chisel3._
import chisel3.util._
import chisel3.experimental.BaseModule
import chisel3.reflect.DataMirror
import scala.collection.mutable.ListBuffer
import difftest.util.dpic.SVStructGenerator._
import difftest.util.dpic.TypeMapping._

class DPICWrapper(bundle: Bundle, wrapperName: String) extends BlackBox 
  with HasBlackBoxInline {
  val io: Bundle = IO(DataMirror.internal.chiselTypeClone(bundle))

  private val in = bundle.elements.get("in")
  private val out = bundle.elements.get("out")
  private val clock = bundle.elements.get("clock")
  private val reset = bundle.elements.get("reset")

  override def desiredName: String = wrapperName

  setInline(s"$desiredName.sv", generateVerilogModule())
  def generateVerilogModule(): String = {
    val verilog = ListBuffer.empty[String]
    
    SVStructGenerator.generateSvStructs(in.get, verilog)
    SVStructGenerator.generateSvStructs(out.get, verilog)
    verilog += s""

    SVStructGenerator.generateDpiFunctions(in.get, verilog)
    SVStructGenerator.generateDpiFunctions(out.get, verilog)
    verilog += s""

    verilog += s"module $wrapperName ("

    SVStructGenerator.generatePortsList(bundle, verilog)
    
    verilog += ");"
    verilog += ""
    
    SVStructGenerator.generatePortDeclarations(bundle, verilog, 1)
    verilog += s""
    
    SVStructGenerator.generateStructWire(in.get, verilog, 1)
    SVStructGenerator.generateStructWire(out.get, verilog, 1)
    verilog += s""

    SVStructGenerator.generateStructAssignment(in.get, verilog, 1)
    SVStructGenerator.generateStructAssignment(out.get, verilog, 1)
    verilog += s""

    verilog += s"  always @(posedge clock) begin"
    verilog += s"    if (!reset) begin"

    SVStructGenerator.genDPICall(in.get, verilog, 3)
    SVStructGenerator.genDPICall(out.get, verilog, 3)
    
    verilog += s"    end"
    verilog += s"  end"
    verilog += s""

    verilog += "endmodule"
    verilog.mkString("\n")
  }
}