package difftest.util.dpic

import chisel3._

import scala.collection.mutable.{ListBuffer, Set}
import difftest.util.dpic.TypeMapping.getDirectionString
import difftest.common.FileControl._

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

    val seenStructs = mutable.Set.empty[String]
    
    val guardName = headerName.replace(".", "_").toUpperCase + "_"
    cpp += s"#ifndef $guardName"
    cpp += s"#define $guardName"
    cpp += ""

    cpp += "// Auto-generated C++ structures for DPI-C"
    cpp += "#include <cstdint>"
    cpp += "#include <cstring>"
    cpp += "#include <iostream>"
    cpp += ""
    
    def generateCppStructRecursive(d: Data): Unit = {
      d match {
        case b: Bundle =>
          val structName = TypeMapping.getCppStructName(b)
          if (!seenStructs.contains(structName)) {
            seenStructs += structName
            
            b.elements.toSeq.reverse.foreach { case (_, field) =>
              field match {
                case nested: Bundle => generateCppStructRecursive(nested)
                case v: Vec[_] => v.head match {
                  case vecBundle: Bundle => generateCppStructRecursive(vecBundle)
                  case vecVec: Vec[_] => throw new Exception(s"Unsupported Vec of Vec: $v")
                  case _ =>
                }
                case _ =>
              }
            }
            
            generateSingleCppStruct(b, cpp)
            cpp += ""
          }
        case _ =>
      }
    }
    
    generateCppStructRecursive(in.get)
    generateCppStructRecursive(out.get)
    
    
    Seq(in, out).map(_.get).foreach {
      case data@(nested: Bundle) =>
        val direction = getDirectionString(data)
        val structName = TypeMapping.getCppStructName(nested)
        val baseName = structName.replace("_t", "").toLowerCase
        cpp += s"// DPI-C export functions for $structName"
        if (direction == "input")
          cpp += s"extern \"C\" void ${baseName}_read($structName in);"
        else
          cpp += s"extern \"C\" void ${baseName}_write($structName *out);"
        cpp += ""
      case _ =>
    }

    

    cpp += s""
    cpp += s"#endif // $guardName"
  }
  
  private def generateSingleCppStruct(bundle: Bundle, cpp: ListBuffer[String], packed: Boolean = true): Unit = {
    val structName = TypeMapping.getCppStructName(bundle)
    val attr = if (packed) "__attribute__((packed)) " else ""

    cpp += s"struct $attr$structName {"
    genCppStructFields(bundle, cpp, 1)
    cpp += ""

    cpp += s"  $structName() { memset(this, 0, sizeof(*this)); }"
    cpp += ""
    
    cpp += s"  bool operator==(const $structName& other) const {"
    cpp += s"    return memcmp(this, &other, sizeof($structName)) == 0;"
    cpp += s"  }"
    cpp += ""
    
    cpp += s"  bool operator!=(const $structName& other) const {"
    cpp += s"    return !(*this == other);"
    cpp += s"  }"
    cpp += "};"
  }
  
  private def genCppStructFields(data: Data, cpp: ListBuffer[String], indent: Int): Unit = {
    val indentStr = "  " * indent
    data match {
      case b: Bundle =>
        b.elements.toSeq.reverse.foreach { case (name, field) =>
          field match {
            case nestedBundle: Bundle =>
              val structName = TypeMapping.getCppStructName(nestedBundle)
              cpp += s"${indentStr}$structName $name;"
            case vec: Vec[_] =>
              vec.head match {
                case vecBundle: Bundle =>
                  val structName = TypeMapping.getCppStructName(vecBundle)
                  cpp += s"${indentStr}$structName $name[${vec.length}];"
                case vecVec: Vec[_] => throw new Exception(s"Unsupported Vec of Vec: $vec")
                case _ =>
                  val elemType = TypeMapping.getCppType(vec.head)
                  cpp += s"${indentStr}$elemType $name[${vec.length}];"
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