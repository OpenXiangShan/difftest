package difftest.util.dpic

import chisel3._
import chisel3.util._
import chisel3.reflect.DataMirror

import scala.collection.mutable
import scala.collection.mutable.ListBuffer

object TypeMapping {
  
  def getSvType(data: Data): String = {
    data match {
      case u: UInt =>
        val width = u.getWidth
        if (width <= 8) "bit[7:0]"
        else if (width <= 16) "bit[15:0]"
        else if (width <= 32) "bit[31:0]"
        else if (width <= 64) "bit[63:0]"
        else s"bit [${width-1}:0]"
        
      case s: SInt =>
        val width = s.getWidth
        if (width <= 8) "bit[7:0]"
        else if (width <= 16) "bit[15:0]"
        else if (width <= 32) "bit[31:0]"
        else if (width <= 64) "bit[63:0]"
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
  
  def getDirectionString(data: Data): String = {
    if (DataMirror.directionOf(data) == ActualDirection.Input) "input" else "output"
  }

  def getVecDimensions(vec: Vec[_]): (Data, List[Int]) = {
    def getDimRecursive(d: Data, dims: List[Int]): (Data, List[Int]) = d match {
      case v: Vec[_] => getDimRecursive(v.head, dims :+ v.length)
      case other => (other, dims)
    }
    getDimRecursive(vec, List.empty[Int])
  }

  def isBundleSame(a: Bundle, b: Bundle): Boolean = {
    if (a.elements.size != b.elements.size) {
      return false
    }

    val aElements = a.elements.toSeq
    val bElements = b.elements.toSeq

    aElements.zip(bElements).forall { case ((nameA, dataA), (nameB, dataB)) =>
      if (nameA != nameB) {
        return false
      }

      isDataSame(dataA, dataB)
    }
  }

  def isDataSame(a: Data, b: Data): Boolean = (a, b) match {
    case (bundleA: Bundle, bundleB: Bundle) =>
      isBundleSame(bundleA, bundleB)

    case (vecA: Vec[_], vecB: Vec[_]) =>
      val (elementTypeA, dimA) = getVecDimensions(vecA)
      val (elementTypeB, dimB) = getVecDimensions(vecB)

      if (dimA != dimB) {
        false
      } else {
        isDataSame(elementTypeA, elementTypeB)
      }

    case _ =>
      getSvType(a) == getSvType(b)
  }

  def regLookup(
    b: Bundle,
    reg: mutable.Map[String, mutable.ListBuffer[Bundle]],
    update: Boolean
  ): (String, Boolean) = {
    val baseName = getStructName(b)
    reg.get(baseName) match {
      case Some(structs) =>
        val index = structs.indexWhere(exist => isBundleSame(b, exist))
        if (index >= 0) {
          val name = if (index == 0) baseName else s"${baseName}_$index"
          (name, false)
        }
        else if (update){
          val newIndex = structs.length
          val name = if (newIndex == 0) baseName else s"${baseName}_$newIndex"
          structs += b
          (name, true)
        }
        else {
          (baseName, true)
        }
      case None =>
        if (update) reg(baseName) = ListBuffer(b)
        (baseName, true)
    }
  }
}