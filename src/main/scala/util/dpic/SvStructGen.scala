package difftest.util.dpic

import chisel3._

import scala.collection.mutable.ListBuffer
import difftest.util.dpic.TypeMapping._

import scala.collection.mutable

object SVStructGenerator {
  
  def generateSvStructs(data: Data, verilog: ListBuffer[String], packed: Boolean = true): Unit = {
    val seenStructs = mutable.Set.empty[String]
    val attr = if (packed) "packed " else ""
    def generateStructRecursive(d: Data): Unit = {
      d match {
        case b: Bundle =>
          val structName = TypeMapping.getStructName(b)
          if (!seenStructs.contains(structName)) {
            seenStructs += structName
            b.elements.foreach { case (_, field) =>
              field match {
                case nested: Bundle => generateStructRecursive(nested)
                case v: Vec[_] => v.head match {
                  case vecBundle: Bundle => generateStructRecursive(vecBundle)
                  case _ =>
                }
                case _ =>
              }
            }
            verilog += s"typedef struct ${attr}{"
            genSvStructFields(b, verilog, 1)
            verilog += s"} $structName;"
            verilog += ""
          }
        case _ => 
      }
    }
    generateStructRecursive(data)
  }
  
  private def genSvStructFields(data: Data, verilog: ListBuffer[String], indent: Int): Unit = {
    val indentStr = "  " * indent
    data match {
      case b: Bundle =>
        b.elements.foreach { case (name, field) =>
          field match {
            case nestedBundle: Bundle =>
              val structName = TypeMapping.getStructName(nestedBundle)
              verilog += s"${indentStr}$structName $name;"
            case vec: Vec[_] =>
              vec.head match {
                case vecBundle: Bundle =>
                  val structName = TypeMapping.getStructName(vecBundle)
                  verilog += s"${indentStr}$structName[${vec.length-1}:0] $name;"
                case vecVec: Vec[_] =>
                  throw new Exception(s"Unsupported Vec of Vec: $vec")
                case _ =>
                  val elemType = TypeMapping.getSvType(vec.head)
                  verilog += s"${indentStr}$elemType[${vec.length-1}:0] $name;"
              }
            case u: UInt =>
              val svType = TypeMapping.getSvType(u)
              verilog += s"${indentStr}$svType $name;"
            case s: SInt =>
              val svType = TypeMapping.getSvType(s)
              verilog += s"${indentStr}$svType $name;"
            case _ =>
              val width = field.getWidth
              verilog += s"${indentStr}bit [${width-1}:0] $name;"
          }
        }
      case _ =>
        throw new Exception(s"Unsupported data type: $data")
    }
  }

  def generateDpiFunctions(data: Data, verilog: ListBuffer[String]): Unit = {
    val direction = getDirectionString(data)
    val operation = if (direction == "input") "read" else "write"
    data match {
      case b: Bundle => 
        val structName = TypeMapping.getStructName(b)
        val baseName = structName.replace("_t", "").toLowerCase
        verilog += s"import \"DPI-C\" context function void ${baseName}_${operation}(${direction} $structName bundle);"
      case _ =>
    }
  }

  def generatePortsList(data: Data, verilog: ListBuffer[String]): Unit = {
    def genPortsListRecursive(data: Data, prefix: String, verilog: ListBuffer[String]): Unit = {
      data match {
        case b: Bundle =>
          b.elements.foreach { case (name, field) =>
            val fullName = if (prefix.nonEmpty) s"${prefix}_${name}" else s"${name}"
            field match {
              case nested: Bundle => genPortsListRecursive(nested, fullName, verilog)
              case vec: Vec[_] => 
                (0 until vec.length).foreach { i =>
                  vec.head match {
                    case vecBundle: Bundle => genPortsListRecursive(vecBundle, s"${fullName}_$i", verilog)
                    case vecVec: Vec[_] => throw new Exception(s"Unsupported Vec of Vec: $vec")
                    case _ => verilog += s"${fullName}_$i,"
                  }
                }
              case _ => verilog += s"${fullName},"
            }
          }
        case _ =>
      }
    }
    genPortsListRecursive(data, "", verilog)
    verilog(verilog.length - 1) = verilog.last.stripSuffix(",")
  }
  
  def generatePortDeclarations(data: Data, verilog: ListBuffer[String], indent: Int): Unit = {
    def genPortDeclarationRecursive(data: Data, prefix: String, verilog: ListBuffer[String], indent: Int): Unit = {
      val indentStr = "  " * indent
      data match {
        case b: Bundle =>
          b.elements.foreach { case (name, field) =>
            val fullName = if (prefix.nonEmpty) s"${prefix}_${name}" else s"${name}"
            field match {
              case nested: Bundle => 
                genPortDeclarationRecursive(nested, fullName, verilog, indent)
              case vec: Vec[_] =>
                (0 until vec.length).foreach { i =>
                  vec.head match {
                    case vecBundle: Bundle => 
                      genPortDeclarationRecursive(vecBundle, s"${fullName}_$i", verilog, indent)
                    case vecVec: Vec[_] => throw new Exception(s"Unsupported Vec of Vec: $vec")
                    case _ =>
                      val direction = TypeMapping.getDirectionString(field)
                      val size = s"[${vec.head.getWidth-1}:0]"
                      verilog += s"${indentStr}$direction $size ${fullName}_$i;"
                  }
                }
              case u: UInt =>
                val direction = TypeMapping.getDirectionString(field)
                val size = s"[${u.getWidth-1}:0]"
                verilog += s"${indentStr}$direction $size $fullName;"
            }
          }
        case _ =>
      }
    }
    genPortDeclarationRecursive(data, "", verilog, indent)
  }

  def generateStructWire(data: Data, verilog: ListBuffer[String], indent: Int): Unit = {
    val indentStr = "  " * indent
    val direction = getDirectionString(data)
    val dir = if (direction == "input") "in" else "out"
    data match {
      case b: Bundle =>
        val structName = TypeMapping.getStructName(b)
        val baseName = structName.replace("_t", "").toLowerCase
        verilog += s"${indentStr}${structName} ${dir};"
      case _ => 
    }
  }

  def generateStructAssignment(data: Data, verilog: ListBuffer[String], indent: Int): Unit = {
    val direction = getDirectionString(data)
    val dir = if (direction == "input") "in" else "out"
    def genStructAssignRecursive(data: Data, prefix:(String, String), verilog: ListBuffer[String], indent: Int): Unit = {
      val indentStr = "  " * indent
      data match {
        case b: Bundle =>
          b.elements.foreach { case (name, field) =>
            val ioName = if (prefix._1.isEmpty) name else s"${prefix._1}_$name"
            val typeName = if (prefix._2.isEmpty) name else s"${prefix._2}.$name"
            field match {
              case fb: Bundle =>
                  genStructAssignRecursive(fb, (ioName, typeName), verilog, indent)
              case fv: Vec[_] =>
                (0 until fv.length).foreach{ i =>
                  fv.head match {
                    case vb: Bundle =>
                      genStructAssignRecursive(vb, (s"${ioName}_$i", s"${typeName}[$i]"), verilog, indent)
                    case vv: Vec[_] => throw new Exception(s"Unsupported Vec of Vec: $fv")
                    case _ =>
                      if (direction == "input") 
                        verilog += s"${indentStr}assign ${typeName}[$i] = ${ioName}_$i;"
                      else
                        verilog += s"${indentStr}assign ${ioName}_$i = ${typeName}[$i];"
                  }   
                }
              case fu: UInt =>
                if (direction == "input") 
                  verilog += s"${indentStr}assign ${typeName} = ${ioName};"
                else
                  verilog += s"${indentStr}assign ${ioName} = ${typeName};"
            }
          }
          case _ =>
      }
    }
    genStructAssignRecursive(data, (dir, dir), verilog, indent)
  }

  def genDPICall(data: Data, verilog: ListBuffer[String], indent: Int): Unit = {
    def indentStr = "  " * indent
    val direction = getDirectionString(data)
    val dir = if (direction == "input") "in" else "out"
    val operation = if (direction == "input") "read" else "write"
    data match {
      case b: Bundle => 
        val structName = TypeMapping.getStructName(b)
        val baseName = structName.replace("_t", "").toLowerCase
        verilog += s"${indentStr}${baseName}_${operation}(${dir});"
      case _ =>
    }
  }
}