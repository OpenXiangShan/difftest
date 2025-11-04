package difftest.util.dpic

import chisel3._
import chisel3.util._
import chisel3.reflect.DataMirror

object TypeMapping {
  
  def getSvType(data: Data): String = {
    data match {
      case u: UInt => 
        val width = u.getWidth
        if (width <= 8) "logic[7:0]"
        else if (width <= 16) "logic[15:0]"
        else if (width <= 32) "logic[31:0]" 
        else if (width <= 64) "logic[63:0]"
        else s"bit [${width-1}:0]"
        
      case s: SInt =>
        val width = s.getWidth
        if (width <= 8) "logic[7:0]"
        else if (width <= 16) "logic[15:0]"
        else if (width <= 32) "logic[31:0]" 
        else if (width <= 64) "logic[63:0]"
        else s"bit [${width-1}:0]"

      case _ => 
        val width = data.getWidth
        if (width > 0) s"bit [${width-1}:0]" else "bit"
    }
  }
  
  def getCppType(data: Data): String = {
    data match {
      case u: UInt => 
        val width = u.getWidth
        if (width <= 8) "uint8_t"
        else if (width <= 16) "uint16_t"  
        else if (width <= 32) "uint32_t"
        else if (width <= 64) "uint64_t"
        else {
          val bytes = (width + 7) / 8
          s"uint8_t[$bytes]"
        }
        
      case s: SInt =>
        val width = s.getWidth
        if (width <= 8) "int8_t"
        else if (width <= 16) "int16_t"
        else if (width <= 32) "int32_t" 
        else if (width <= 64) "int64_t"
        else {
          val bytes = (width + 7) / 8
          s"int8_t[$bytes]"
        }
      case _ => 
        val width = data.getWidth
        if (width > 0) {
          val bytes = (width + 7) / 8
          s"uint8_t[$bytes]"
        } else {
          "uint8_t"
        }
    }
  }
  
  def getStructName(bundle: Bundle, prefix: String = ""): String = {
    val baseName = bundle match {
      case d: DPICBundle => d.className
      case _ => 
        val className = bundle.getClass.getSimpleName.replace("$", "")
        if (className.isEmpty) "AnonymousBundle" else className
    }
    
    val fullName = if (prefix.isEmpty) baseName else s"${prefix}_$baseName"
    s"${fullName}_t"
  }
  
  def getCppStructName(bundle: Bundle, prefix: String = ""): String = {
    getStructName(bundle, prefix)
  }
  
  def getDirectionString(data: Data): String = {
    if (DataMirror.directionOf(data) == ActualDirection.Input) "input" else "output"
  }
}