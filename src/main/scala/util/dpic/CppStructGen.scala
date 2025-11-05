package difftest.util.dpic

import chisel3._

import scala.collection.mutable.{ListBuffer, Set}
import difftest.util.dpic.TypeMapping.{getDirectionString, getVecDimensions, regLookup}
import difftest.common.FileControl._

import scala.annotation.tailrec
import scala.collection.mutable

object CppStructGenerator {
  def generateCppHeader(data: Data, headerName: Option[String] = None): Unit = {
    val fileName = headerName.getOrElse(s"${data.getClass.getSimpleName}.h")
    val cpp = ListBuffer.empty[String]
    genCppHeaderList(data, cpp, fileName)
    write(cpp, fileName)
  }

  private def genCppHeaderList(data: Data, cpp: ListBuffer[String], headerName: String): Unit = {
    val in = data match {
      case b: Bundle =>
        b.elements.get("in")
      case _ => None
    }
    val out = data match {
      case b: Bundle =>
        b.elements.get("out")
      case _ => None
    }

    val guardName = headerName.replace(".", "_").toUpperCase + "_"
    cpp += s"#ifndef $guardName"
    cpp += s"#define $guardName"
    cpp += ""

    cpp += "// Auto-generated C++ structures for DPI-C"
    cpp += "#include <cstdint>"
    cpp += "#include <cstring>"
    cpp += "#include <iostream>"
    cpp += ""

    def generateCppStructRecursive(d: Data, reg: mutable.Map[String, mutable.ListBuffer[Bundle]]): Unit = {
      def processDataForCppStructs(d: Data): Unit = {
        d match {
          case b: Bundle =>
            b.elements.toSeq.reverse.foreach { case (_, field) =>
              field match {
                case nested: Bundle =>
                  processDataForCppStructs(nested)
                case vec: Vec[_] =>
                  val (elementType, _) = getVecDimensions(vec)
                  elementType match {
                    case vecBundle: Bundle =>
                      processDataForCppStructs(vecBundle)
                    case _ =>
                  }
                case _ =>
              }
            }
            generateSingleCppStruct(b, cpp, reg)
          case _ =>
        }
      }
      processDataForCppStructs(d)
    }

    val reg = mutable.Map.empty[String, mutable.ListBuffer[Bundle]]
    generateCppStructRecursive(in.get, reg)
    generateCppStructRecursive(out.get, reg)

    Seq(in, out).map(_.get).foreach {
      case data@(nested: Bundle) =>
        val direction = getDirectionString(data)
        val structName = TypeMapping.getStructName(nested)
        val baseName = structName.replace("_t", "").toLowerCase
        cpp += s"// DPI-C export functions for $structName"
        if (direction == "input")
          cpp += s"extern \"C\" void ${baseName}($structName in);"
        else
          cpp += s"extern \"C\" void ${baseName}($structName *out);"
        cpp += ""
      case _ =>
    }

    cpp += s"extern \"C\" void tick();"
    cpp += s"extern \"C\" void reset();"
    cpp += s""

    cpp += s"#endif // $guardName"
  }

  private def generateSingleCppStruct(
    bundle: Bundle,
    cpp: ListBuffer[String],
    reg: mutable.Map[String, mutable.ListBuffer[Bundle]],
    packed: Boolean = true
  ): Unit = {
    val (actualName, isNew) = regLookup(bundle, reg, update = true)
    val attr = if (packed) "__attribute__((packed)) " else ""

    if (isNew) {
      cpp += s"struct $attr$actualName {"
      genCppStructFields(bundle, cpp, reg, 1)
      cpp += "};"
    }

  }

  private def genCppStructFields(
    data: Data,
    cpp: ListBuffer[String],
    reg: mutable.Map[String, ListBuffer[Bundle]],
    indent: Int
  ): Unit = {
    def genVecDeclaration(baseType: String, dims: List[Int], name: String): String = {
      val dimStrings = dims.map(dim => s"[$dim]").mkString("")
      s"$baseType $name$dimStrings;"
    }

    val indentStr = "  " * indent
    data match {
      case b: Bundle =>
        b.elements.toSeq.reverse.foreach { case (name, field) =>
          field match {
            case nestedBundle: Bundle =>
              val (actualName, isNew) = regLookup(nestedBundle, reg, update = false)
              if (isNew) throw new Exception(s"Encounter not generated struct")
              cpp += s"${indentStr}$actualName $name;"

            case vec: Vec[_] =>
              val (elem, dims) = getVecDimensions(vec)
              elem match {
                case vecBundle: Bundle =>
                  val (actualName, isNew) = regLookup(vecBundle, reg, update = false)
                  if (isNew) throw new Exception(s"Encounter not generated struct")
                  cpp += s"$indentStr${genVecDeclaration(actualName, dims, name)}"
                case _ =>
                  val elemType = TypeMapping.getCppType(elem)
                  cpp += s"$indentStr${genVecDeclaration(elemType, dims, name)}"
              }

            case u: UInt =>
              val cppType = TypeMapping.getCppType(u)
              cpp += s"${indentStr}$cppType $name;"

            case s: SInt =>
              val cppType = TypeMapping.getCppType(s)
              cpp += s"${indentStr}$cppType $name;"

            case _ =>
              val width = field.getWidth
              val bytes = (width + 7) / 8
              cpp += s"${indentStr}uint8_t $name[$bytes];"
          }
        }
      case _ =>
    }
  }
}