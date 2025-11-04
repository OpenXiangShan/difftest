package difftest.util.dpic

import chisel3._

import scala.collection.mutable.ListBuffer
import difftest.util.dpic.TypeMapping._

import scala.annotation.tailrec
import scala.collection.mutable

object SVStructGenerator {

  private def getVecDimensions(vec: Vec[_]): (Data, List[Int]) = {
    @tailrec
    def getDimRecursive(d: Data, dims: List[Int]): (Data, List[Int]) = d match {
      case v: Vec[_] => getDimRecursive(v.head, dims :+ v.length)
      case other => (other, dims)
    }
    getDimRecursive(vec, List.empty[Int])
  }

  def generateSvStructs(data: Data, verilog: ListBuffer[String], packed: Boolean = true): Unit = {
    val seenStructs = mutable.Set.empty[String]
    val attr = if (packed) "packed " else ""

    def processDataForStructs(d: Data): Unit = {
      d match {
        case b: Bundle =>
          val structName = TypeMapping.getStructName(b)
          if (!seenStructs.contains(structName)) {
            seenStructs += structName
            b.elements.foreach { case (_, field) =>
              field match {
                case nested: Bundle =>
                  processDataForStructs(nested)
                case vec: Vec[_] =>
                  val (elementType, _) = getVecDimensions(vec)
                  elementType match {
                    case vecBundle: Bundle =>
                      processDataForStructs(vecBundle)
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

    processDataForStructs(data)
  }

  private def genSvStructFields(data: Data, verilog: ListBuffer[String], indent: Int): Unit = {
    val indentStr = "  " * indent

    def genVecDeclaration(baseType: String, dims: List[Int], name: String): String = {
      val dimStrings = dims.map(dim => s"[$dim-1:0]")
      s"$baseType ${dimStrings.mkString(" ")} $name;"
    }

    data match {
      case b: Bundle =>
        b.elements.foreach { case (name, field) =>
          field match {
            case nestedBundle: Bundle =>
              val structName = TypeMapping.getStructName(nestedBundle)
              verilog += s"$indentStr$structName $name;"
            case vec: Vec[_] =>
              val (elem, dims) = getVecDimensions(vec)
              elem match {
                case vecBundle: Bundle =>
                  val structName = TypeMapping.getStructName(vecBundle)
                  verilog += s"$indentStr${genVecDeclaration(structName, dims, name)}"
                case _ =>
                  val elemType = TypeMapping.getSvType(elem)
                  verilog += s"$indentStr${genVecDeclaration(elemType, dims, name)}"
              }
            case u: UInt =>
              val svType = TypeMapping.getSvType(u)
              verilog += s"$indentStr$svType $name;"
            case s: SInt =>
              val svType = TypeMapping.getSvType(s)
              verilog += s"$indentStr$svType $name;"
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

    def generateVecPorts(elementType: Data, dimensions: List[Int], baseName: String, verilog: ListBuffer[String]): Unit = {
      def generateIndices(currentIndices: List[Int], remainingDims: List[Int]): Unit = {
        if (remainingDims.isEmpty) {
          val indexString = currentIndices.map(i => s"_$i").mkString
          val fullName = s"${baseName}${indexString}"
          elementType match {
            case nestedBundle: Bundle =>
              genPortsListRecursive(nestedBundle, fullName, verilog)
            case _ =>
              verilog += s"${fullName},"
          }
        } else {
          val currentDim = remainingDims.head
          val newRemainingDims = remainingDims.tail
          for (i <- 0 until currentDim) {
            generateIndices(currentIndices :+ i, newRemainingDims)
          }
        }
      }
      generateIndices(List.empty, dimensions)
    }

    def genPortsListRecursive(data: Data, prefix: String, verilog: ListBuffer[String]): Unit = {
      data match {
        case b: Bundle =>
          b.elements.foreach { case (name, field) =>
            val fullName = if (prefix.nonEmpty) s"${prefix}_${name}" else name
            field match {
              case nested: Bundle =>
                genPortsListRecursive(nested, fullName, verilog)

              case vec: Vec[_] =>
                val (elementType, dimensions) = getVecDimensions(vec)
                generateVecPorts(elementType, dimensions, fullName, verilog)

              case _ =>
                verilog += s"${fullName},"
            }
          }
        case _ =>
      }
    }
    genPortsListRecursive(data, "", verilog)
    // 移除最后一个逗号
    if (verilog.nonEmpty) {
      verilog(verilog.length - 1) = verilog.last.stripSuffix(",")
    }
  }

  def generatePortDeclarations(data: Data, verilog: ListBuffer[String], indent: Int): Unit = {
    def generateVecPortDeclarations(
      elementType: Data,
      dimensions: List[Int],
      baseName: String,
      verilog: ListBuffer[String],
      indent: Int
    ): Unit = {
      def generateIndices(currentIndices: List[Int], remainingDims: List[Int]): Unit = {
        if (remainingDims.isEmpty) {
          val indexString = currentIndices.map(i => s"_$i").mkString
          val fullName = s"${baseName}${indexString}"
          elementType match {
            case nestedBundle: Bundle =>
              genPortDeclarationRecursive(nestedBundle, fullName, verilog, indent)
            case _ =>
              val direction = TypeMapping.getDirectionString(elementType)
              val width = elementType.getWidth
              val size = if (width > 1) s" [${width-1}:0]" else ""
              val indentStr = "  " * indent
              verilog += s"${indentStr}$direction$size $fullName;"
          }
        } else {
          val currentDim = remainingDims.head
          val newRemainingDims = remainingDims.tail
          for (i <- 0 until currentDim) {
            generateIndices(currentIndices :+ i, newRemainingDims)
          }
        }
      }

      generateIndices(List.empty, dimensions)
    }

    def genPortDeclarationRecursive(data: Data, prefix: String, verilog: ListBuffer[String], indent: Int): Unit = {
      val indentStr = "  " * indent
      data match {
        case b: Bundle =>
          b.elements.foreach { case (name, field) =>
            val fullName = if (prefix.nonEmpty) s"${prefix}_${name}" else name
            field match {
              case nested: Bundle =>
                genPortDeclarationRecursive(nested, fullName, verilog, indent)

              case vec: Vec[_] =>
                val (elementType, dimensions) = getVecDimensions(vec)
                generateVecPortDeclarations(elementType, dimensions, fullName, verilog, indent)

              case u: UInt =>
                val direction = TypeMapping.getDirectionString(field)
                val size = if (u.getWidth > 1) s" [${u.getWidth-1}:0]" else ""
                verilog += s"${indentStr}$direction$size $fullName;"

              case s: SInt =>
                val direction = TypeMapping.getDirectionString(field)
                val size = if (s.getWidth > 1) s" [${s.getWidth-1}:0]" else ""
                verilog += s"${indentStr}$direction$size $fullName;"

              case _ =>
                val direction = TypeMapping.getDirectionString(field)
                val width = field.getWidth
                val size = if (width > 1) s" [${width-1}:0]" else ""
                verilog += s"${indentStr}$direction$size $fullName;"
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
    def generateVecAssignments(
      elementType: Data,
      dimensions: List[Int],
      ioBaseName: String,
      structBaseName: String,
      verilog: ListBuffer[String],
      indent: Int,
      direction: String
    ): Unit = {
      def generateIndices(currentIndices: List[Int], remainingDims: List[Int]): Unit = {
        if (remainingDims.isEmpty) {
          val indexString = currentIndices.map(i => s"_$i").mkString
          val structIndexString = currentIndices.map(i => s"[$i]").mkString
          val fullIoName = s"${ioBaseName}${indexString}"
          val fullStructName = s"${structBaseName}${structIndexString}"

          elementType match {
            case nestedBundle: Bundle =>
              genStructAssignRecursive(nestedBundle, (fullIoName, fullStructName), verilog, indent, direction)
            case _ =>
              val indentStr = "  " * indent
              if (direction == "input")
                verilog += s"${indentStr}assign $fullStructName = $fullIoName;"
              else
                verilog += s"${indentStr}assign $fullIoName = $fullStructName;"
          }
        } else {
          val currentDim = remainingDims.head
          val newRemainingDims = remainingDims.tail
          for (i <- 0 until currentDim) {
            generateIndices(currentIndices :+ i, newRemainingDims)
          }
        }
      }

      generateIndices(List.empty, dimensions)
    }

    def genStructAssignRecursive(data: Data, prefix: (String, String), verilog: ListBuffer[String], indent: Int, direction: String): Unit = {
      val indentStr = "  " * indent
      data match {
        case b: Bundle =>
          b.elements.foreach { case (name, field) =>
            val ioName = if (prefix._1.isEmpty) name else s"${prefix._1}_$name"
            val structName = if (prefix._2.isEmpty) name else s"${prefix._2}.$name"
            field match {
              case nested: Bundle =>
                genStructAssignRecursive(nested, (ioName, structName), verilog, indent, direction)

              case vec: Vec[_] =>
                val (elementType, dimensions) = getVecDimensions(vec)
                generateVecAssignments(elementType, dimensions, ioName, structName, verilog, indent, direction)

              case _ =>
                if (direction == "input")
                  verilog += s"${indentStr}assign $structName = $ioName;"
                else
                  verilog += s"${indentStr}assign $ioName = $structName;"
            }
          }
        case _ =>
      }
    }

    val direction = getDirectionString(data)
    val dir = if (direction == "input") "in" else "out"
    genStructAssignRecursive(data, (dir, dir), verilog, indent, direction)
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