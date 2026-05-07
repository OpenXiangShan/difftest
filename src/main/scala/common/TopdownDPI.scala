/***************************************************************************************
 * Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
 * Copyright (c) 2020-2021 Peng Cheng Laboratory
 *
 * DiffTest is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

package difftest.common

import chisel3._
import chisel3.experimental.ExtModule
import chisel3.util._

private[common] object TopdownDPI {
  final case class PackedArg(name: String, totalWidth: Int, fieldWidth: Int)

  def packedWidth(entriesNum: Int, fieldWidth: Int): Int = entriesNum * fieldWidth

  def makePackedArgs(prefix: String, entriesNum: Int, fields: Seq[(String, Int)]): Seq[PackedArg] = {
    fields.map { case (fieldName, fieldWidth) =>
      PackedArg(s"${prefix}_$fieldName", packedWidth(entriesNum, fieldWidth), fieldWidth)
    }
  }

  def packFields(data: Seq[UInt]): UInt = Cat(data.reverse)

  def unpackField(data: UInt, idx: Int, fieldWidth: Int): UInt = {
    data((idx + 1) * fieldWidth - 1, idx * fieldWidth)
  }

  def scalarCType(width: Int): String = width match {
    case 1                      => "bool"
    case w if w > 1 && w <= 8   => "uint8_t"
    case w if w > 8 && w <= 16  => "uint16_t"
    case w if w > 16 && w <= 32 => "uint32_t"
    case w if w > 32 && w <= 64 => "uint64_t"
    case w                      => throw new Exception(s"Unsupported C width: $w")
  }

  def scalarSvDpiType(width: Int): String = width match {
    case 1                      => "bit"
    case w if w > 1 && w <= 8   => "byte unsigned"
    case w if w > 8 && w <= 16  => "shortint unsigned"
    case w if w > 16 && w <= 32 => "int unsigned"
    case w if w > 32 && w <= 64 => "longint unsigned"
    case w                      => throw new Exception(s"Unsupported SV DPI width: $w")
  }

  def modulePortArg(name: String, width: Int, isOutput: Boolean): String = {
    val direction = if (isOutput) "output" else "input"
    val widthString = if (width == 1) "      " else f"[${width - 1}%2d:0]"
    Seq(direction, widthString, name).mkString(" ")
  }

  def moduleWireDecl(name: String, width: Int): String = {
    val widthString = if (width == 1) "      " else f"[${width - 1}%2d:0]"
    Seq("wire", widthString, s"$name;").mkString(" ")
  }

  def dpiArg(name: String, width: Int, isOutput: Boolean): String = {
    val direction = if (isOutput) "output" else "input"
    val typeString = if (width <= 64) scalarSvDpiType(width) else s"bit [${width - 1}:0]"
    s"$direction $typeString $name"
  }

  def cppArg(name: String, width: Int, isOutput: Boolean): String = {
    if (width <= 64) {
      val refPrefix = if (isOutput) "&" else ""
      f"${scalarCType(width)}%-8s $refPrefix$name"
    } else {
      val typeString = if (isOutput) "uint32_t*" else "const uint32_t*"
      s"$typeString $name"
    }
  }

  def gsimAlignedWidth(width: Int): Int = ((width + 63) / 64) * 64

  def gsimWideWordCount(width: Int): Int = gsimAlignedWidth(width) / 32

  def gsimCppArg(name: String, width: Int, isOutput: Boolean): String = {
    if (width <= 64) {
      cppArg(name, width, isOutput)
    } else {
      val typeString = s"unsigned _BitInt(${gsimAlignedWidth(width)})"
      if (isOutput) s"$typeString &$name" else s"$typeString $name"
    }
  }

  def localVarDecl(name: String, width: Int): String = {
    if (width <= 64) s"${scalarSvDpiType(width)} $name;" else s"bit [${width - 1}:0] $name;"
  }

  def functionType(width: Int): String = {
    if (width <= 64) scalarSvDpiType(width) else s"bit [${width - 1}:0]"
  }

  def maskExpr(width: Int): String = if (width == 32) "0xffffffffULL" else s"((1ULL << $width) - 1ULL)"

  def decodeExpr(arg: PackedArg, idx: Int): String = {
    val offset = idx * arg.fieldWidth
    if (arg.totalWidth <= 64) {
      s"static_cast<uint32_t>((static_cast<uint64_t>(${arg.name}) >> $offset) & ${maskExpr(arg.fieldWidth)})"
    } else {
      s"get_packed_bits(${arg.name}, $offset, ${arg.fieldWidth})"
    }
  }

  def encodeStmt(arg: PackedArg, idx: Int, valueExpr: String): String = {
    val offset = idx * arg.fieldWidth
    if (arg.totalWidth <= 64) {
      s"${arg.name}_packed |= static_cast<uint64_t>($valueExpr) << $offset;"
    } else {
      s"set_packed_bits(${arg.name}, $offset, ${arg.fieldWidth}, static_cast<uint32_t>($valueExpr));"
    }
  }

  def scalarOutputAccums(outputArgs: Seq[PackedArg]): Seq[String] = {
    outputArgs.filter(_.totalWidth <= 64).map(arg => s"uint64_t ${arg.name}_packed = 0;")
  }

  def wideOutputZeros(outputArgs: Seq[PackedArg]): Seq[String] = {
    outputArgs
      .filter(_.totalWidth > 64)
      .map(arg => s"std::memset(${arg.name}, 0, ${((arg.totalWidth + 31) / 32)} * sizeof(uint32_t));")
  }

  def scalarOutputWrites(outputArgs: Seq[PackedArg]): Seq[String] = {
    outputArgs
      .filter(_.totalWidth <= 64)
      .map(arg => s"${arg.name} = static_cast<${scalarCType(arg.totalWidth)}>(${arg.name}_packed);")
  }

  def packedBitHelpers(args: Seq[PackedArg]): Seq[String] = {
    if (args.exists(_.totalWidth > 64)) {
      Seq(
        "auto get_packed_bits = [](const uint32_t *data, int lsb, int width) -> uint32_t {",
        "  const int wordIndex = lsb / 32;", "  const int bitIndex = lsb % 32;",
        "  uint64_t combined = static_cast<uint64_t>(data[wordIndex]) >> bitIndex;", "  if (bitIndex + width > 32) {",
        "    combined |= static_cast<uint64_t>(data[wordIndex + 1]) << (32 - bitIndex);", "  }",
        "  const uint64_t mask = width == 32 ? 0xffffffffULL : ((1ULL << width) - 1ULL);",
        "  return static_cast<uint32_t>(combined & mask);", "};",
        "auto set_packed_bits = [](uint32_t *data, int lsb, int width, uint32_t value) {",
        "  const int wordIndex = lsb / 32;", "  const int bitIndex = lsb % 32;",
        "  const uint64_t mask = width == 32 ? 0xffffffffULL : ((1ULL << width) - 1ULL);",
        "  uint64_t combined = static_cast<uint64_t>(data[wordIndex]);", "  if (bitIndex + width > 32) {",
        "    combined |= static_cast<uint64_t>(data[wordIndex + 1]) << 32;", "  }",
        "  const uint64_t fieldMask = mask << bitIndex;",
        "  combined = (combined & ~fieldMask) | ((static_cast<uint64_t>(value) & mask) << bitIndex);",
        "  data[wordIndex] = static_cast<uint32_t>(combined);", "  if (bitIndex + width > 32) {",
        "    data[wordIndex + 1] = static_cast<uint32_t>(combined >> 32);", "  }", "};",
      )
    } else {
      Seq.empty
    }
  }
}

private[common] trait TopdownDPI { this: ExtModule with HasExtModuleInline =>
  import TopdownDPI._

  protected def dpiFuncName: String
  protected def inputArgs: Seq[PackedArg]
  protected def outputArgs: Seq[PackedArg]
  protected def wrapperBody: String
  protected def header: String
  protected def hasClockPort: Boolean

  final protected def inputAssigns(entriesNum: Int): Seq[String] = {
    inputArgs.flatMap { arg =>
      val fieldName = arg.name.stripPrefix("io_in_")
      (0 until entriesNum).map(idx => s"in[$idx].$fieldName = ${decodeExpr(arg, idx)};")
    }
  }

  final protected def outputAssigns(entriesNum: Int): Seq[String] = {
    outputArgs.flatMap { arg =>
      val fieldName = arg.name.stripPrefix("io_out_")
      (0 until entriesNum).map(idx => encodeStmt(arg, idx, s"out[$idx].$fieldName"))
    }
  }

  final protected def emitTopdownDPI(): Unit = {
    difftest.DifftestModule.createCppDPICModule(dpiFuncName, cppDPICModule, Some(header))
    difftest.DifftestModule.createCppExtModule(desiredName, cppExtModule, Some(header))
    setInline(s"$desiredName.v", inlineModule)
  }

  private def cppArgs: Seq[String] = {
    inputArgs.map(arg => cppArg(arg.name, arg.totalWidth, isOutput = false)) ++
      outputArgs.map(arg => cppArg(arg.name, arg.totalWidth, isOutput = true))
  }

  private def gsimCppArgs: Seq[String] = {
    inputArgs.map(arg => gsimCppArg(arg.name, arg.totalWidth, isOutput = false)) ++
      outputArgs.map(arg => gsimCppArg(arg.name, arg.totalWidth, isOutput = true))
  }

  private def dpiArgs: Seq[String] = {
    inputArgs.map(arg => dpiArg(arg.name, arg.totalWidth, isOutput = false)) ++
      outputArgs.map(arg => dpiArg(arg.name, arg.totalWidth, isOutput = true))
  }

  private def modulePorts: Seq[String] = {
    val dataPorts =
      inputArgs.map(arg => modulePortArg(arg.name, arg.totalWidth, isOutput = false)) ++
        outputArgs.map(arg => modulePortArg(arg.name, arg.totalWidth, isOutput = true))
    if (hasClockPort) Seq("input clock") ++ dataPorts else dataPorts
  }

  private def outputModuleTemps: Seq[(String, Int)] = {
    outputArgs.map(arg => (s"${arg.name.stripPrefix("io_out_")}_tmp", arg.totalWidth))
  }

  private def outputCallTemps: Seq[(String, Int)] = {
    outputArgs.map(arg => (s"${arg.name.stripPrefix("io_out_")}_dpi_tmp", arg.totalWidth))
  }

  private def svDpiCallArgs: Seq[String] = inputArgs.map(_.name) ++ outputCallTemps.map(_._1)

  private def outputTmpDecls: Seq[String] = {
    outputModuleTemps.map { case (tmpName, width) => moduleWireDecl(tmpName, width) }
  }

  private def synthTmpAssigns: Seq[String] = {
    outputModuleTemps.map { case (tmpName, width) =>
      if (width == 1) s"assign $tmpName = 1'b0;" else s"assign $tmpName = ${width}'b0;"
    }
  }

  private def cppExtWideInputTemps: Seq[String] = {
    inputArgs.filter(_.totalWidth > 64).map { arg =>
      val tempName = s"${arg.name}_words"
      val wordCount = gsimWideWordCount(arg.totalWidth)
      s"uint32_t $tempName[$wordCount] = {};\n  std::memcpy($tempName, &${arg.name}, sizeof($tempName));"
    }
  }

  private def cppExtWideOutputTemps: Seq[String] = {
    outputArgs.filter(_.totalWidth > 64).map { arg =>
      val tempName = s"${arg.name}_words"
      val wordCount = gsimWideWordCount(arg.totalWidth)
      s"uint32_t $tempName[$wordCount] = {};"
    }
  }

  private def cppExtCallArgs: Seq[String] = {
    inputArgs.map(arg => if (arg.totalWidth <= 64) arg.name else s"${arg.name}_words") ++
      outputArgs.map(arg => if (arg.totalWidth <= 64) arg.name else s"${arg.name}_words")
  }

  private def cppExtWideOutputCopies: Seq[String] = {
    outputArgs.filter(_.totalWidth > 64).map { arg =>
      val tempName = s"${arg.name}_words"
      s"std::memcpy(&${arg.name}, $tempName, sizeof($tempName));"
    }
  }

  private def outputEvalFuncs: Seq[String] = {
    outputArgs.map { arg =>
      val funcName = s"${arg.name}_value"
      val localDecls = outputCallTemps.map { case (tmpName, width) => localVarDecl(tmpName, width) }
      val localDefaults = outputCallTemps.map { case (tmpName, width) =>
        if (width == 1) s"$tmpName = 1'b0;" else s"$tmpName = ${width}'b0;"
      }
      val returnTmpName = s"${arg.name.stripPrefix("io_out_")}_dpi_tmp"

      s"""
         |function automatic ${functionType(arg.totalWidth)} $funcName;
         |  ${localDecls.mkString("\n  ")}
         |  begin
         |    ${localDefaults.mkString("\n    ")}
         |    $dpiFuncName(
         |      ${svDpiCallArgs.mkString(",\n      ")}
         |    );
         |    $funcName = $returnTmpName;
         |  end
         |endfunction
         |""".stripMargin
    }
  }

  private def outputTmpAssigns: Seq[String] = {
    outputArgs.map { arg =>
      val tmpName = s"${arg.name.stripPrefix("io_out_")}_tmp"
      s"assign $tmpName = ${arg.name}_value();"
    }
  }

  private def outputNetAssigns: Seq[String] = {
    outputArgs.map { arg =>
      val tmpName = s"${arg.name.stripPrefix("io_out_")}_tmp"
      s"assign ${arg.name} = $tmpName;"
    }
  }

  private def cppDPICModule: String =
    s"""
       |extern "C" void $dpiFuncName (
       |  ${cppArgs.mkString(",\n  ")}
       |) {
       |$wrapperBody
       |}
       |""".stripMargin

  private def cppExtModule: String =
    s"""
       |extern \"C\" void $dpiFuncName (
       |  ${cppArgs.mkString(",\n  ")}
       |);
       |
       |void $desiredName (
       |  ${gsimCppArgs.mkString(",\n  ")}
       |) {
       |  ${cppExtWideInputTemps.mkString("\n  ")}
       |  ${cppExtWideOutputTemps.mkString("\n  ")}
       |  $dpiFuncName(
       |    ${cppExtCallArgs.mkString(",\n    ")}
       |  );
       |  ${cppExtWideOutputCopies.mkString("\n  ")}
       |}
       |""".stripMargin

  private def inlineModule: String =
    s"""
       |`ifndef SYNTHESIS
       |import "DPI-C" function void $dpiFuncName(
       |  ${dpiArgs.mkString(",\n  ")}
       |);
       |`endif // SYNTHESIS
       |
       |module $desiredName (
       |  ${modulePorts.mkString(",\n  ")}
       |);
       |
       |  ${outputTmpDecls.mkString("\n  ")}
       |
       |`ifdef SYNTHESIS
       |  ${synthTmpAssigns.mkString("\n  ")}
       |  ${outputNetAssigns.mkString("\n  ")}
       |`else
       |  ${outputEvalFuncs.mkString("\n  ")}
       |  ${outputTmpAssigns.mkString("\n  ")}
       |  ${outputNetAssigns.mkString("\n  ")}
       |`endif // SYNTHESIS
       |
       |endmodule
       |""".stripMargin
}
