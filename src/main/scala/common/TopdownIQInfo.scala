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

class TopdownIQInfo extends Bundle {
  val valid = Bool()
  val robIdx = UInt(16.W)
  val robFlag = Bool()
  val pipeNum = UInt(8.W)
  val cancelSource = UInt(3.W)
  val srcReady = Bool()
  val futype = UInt(8.W) // not one-hot
  val issued = Bool()
}

class TopdownExtendedIQInfo extends Bundle {
  val idealIssueTime = Bool()
}

class TopdownInfo(val entriesNum: Int) extends Bundle {
  val in = Input(Vec(entriesNum, new TopdownIQInfo))
  val out = Output(Vec(entriesNum, new TopdownExtendedIQInfo))
}

object TopdownIQInfoPacking {
  val inputFieldSpecs: Seq[(String, Int)] = Seq(
    "valid" -> 1,
    "robIdx" -> 16,
    "robFlag" -> 1,
    "pipeNum" -> 8,
    "cancelSource" -> 3,
    "srcReady" -> 1,
    "futype" -> 8,
    "issued" -> 1,
  )

  val outputFieldSpecs: Seq[(String, Int)] = Seq(
    "idealIssueTime" -> 1,
  )

  def packedWidth(entriesNum: Int, fieldWidth: Int): Int = entriesNum * fieldWidth
}

class TopdownIQInfoPackedBundle(val entriesNum: Int) extends Bundle {
  import TopdownIQInfoPacking._

  val in_valid = Input(UInt(packedWidth(entriesNum, 1).W))
  val in_robIdx = Input(UInt(packedWidth(entriesNum, 16).W))
  val in_robFlag = Input(UInt(packedWidth(entriesNum, 1).W))
  val in_pipeNum = Input(UInt(packedWidth(entriesNum, 8).W))
  val in_cancelSource = Input(UInt(packedWidth(entriesNum, 3).W))
  val in_srcReady = Input(UInt(packedWidth(entriesNum, 1).W))
  val in_futype = Input(UInt(packedWidth(entriesNum, 8).W))
  val in_issued = Input(UInt(packedWidth(entriesNum, 1).W))

  val out_idealIssueTime = Output(UInt(packedWidth(entriesNum, 1).W))
}

class TopdownIQInfoHelper(val entriesNum: Int) extends ExtModule with HasExtModuleInline {
  import TopdownIQInfoPacking._

  private case class PackedArg(name: String, totalWidth: Int, fieldWidth: Int, entriesNum: Int)

  override def desiredName: String = s"TopdownIQInfoHelper_${entriesNum}"

  private val dpiFuncName = s"topdown_iq_info_${entriesNum}"

  val clock = IO(Input(Clock()))
  val io = IO(new TopdownIQInfoPackedBundle(entriesNum))

  private val inputArgs = inputFieldSpecs.map { case (fieldName, fieldWidth) =>
    PackedArg(s"io_in_$fieldName", packedWidth(entriesNum, fieldWidth), fieldWidth, entriesNum)
  }
  private val outputArgs = outputFieldSpecs.map { case (fieldName, fieldWidth) =>
    PackedArg(s"io_out_$fieldName", packedWidth(entriesNum, fieldWidth), fieldWidth, entriesNum)
  }

  private def scalarCType(width: Int): String = width match {
    case 1                                  => "bool"
    case w if w > 1 && w <= 8               => "uint8_t"
    case w if w > 8 && w <= 16              => "uint16_t"
    case w if w > 16 && w <= 32             => "uint32_t"
    case w if w > 32 && w <= 64             => "uint64_t"
    case w                                  => throw new Exception(s"Unsupported C width: $w")
  }

  private def scalarSvDpiType(width: Int): String = width match {
    case 1                                  => "bit"
    case w if w > 1 && w <= 8               => "byte unsigned"
    case w if w > 8 && w <= 16              => "shortint unsigned"
    case w if w > 16 && w <= 32             => "int unsigned"
    case w if w > 32 && w <= 64             => "longint unsigned"
    case w                                  => throw new Exception(s"Unsupported SV DPI width: $w")
  }

  private def modulePortArg(name: String, width: Int, isOutput: Boolean): String = {
    val direction = if (isOutput) "output" else "input"
    val widthString = if (width == 1) "      " else f"[${width - 1}%2d:0]"
    Seq(direction, widthString, name).mkString(" ")
  }

  private def moduleRegDecl(name: String, width: Int): String = {
    val widthString = if (width == 1) "      " else f"[${width - 1}%2d:0]"
    Seq("reg", widthString, s"$name;").mkString(" ")
  }

  private def moduleWireDecl(name: String, width: Int): String = {
    val widthString = if (width == 1) "      " else f"[${width - 1}%2d:0]"
    Seq("wire", widthString, s"$name;").mkString(" ")
  }

  private def dpiArg(name: String, width: Int, isOutput: Boolean): String = {
    val direction = if (isOutput) "output" else "input"
    val typeString = if (width <= 64) scalarSvDpiType(width) else s"bit [${width - 1}:0]"
    s"$direction $typeString $name"
  }

  private def cppArg(name: String, width: Int, isOutput: Boolean): String = {
    if (width <= 64) {
      val refPrefix = if (isOutput) "&" else ""
      f"${scalarCType(width)}%-8s $refPrefix$name"
    } else {
      val typeString = if (isOutput) "uint32_t*" else "const uint32_t*"
      s"$typeString $name"
    }
  }

  private def gsimAlignedWidth(width: Int): Int = ((width + 63) / 64) * 64

  private def gsimCppArg(name: String, width: Int, isOutput: Boolean): String = {
    if (width <= 64) {
      cppArg(name, width, isOutput)
    } else {
      val typeString = s"unsigned _BitInt(${gsimAlignedWidth(width)})"
      if (isOutput) s"$typeString &$name" else s"$typeString $name"
    }
  }

  private def gsimWideWordCount(width: Int): Int = gsimAlignedWidth(width) / 32

  private val cppArgs = inputArgs.map(arg => cppArg(arg.name, arg.totalWidth, isOutput = false)) ++
    outputArgs.map(arg => cppArg(arg.name, arg.totalWidth, isOutput = true))
  private val gsimCppArgs = inputArgs.map(arg => gsimCppArg(arg.name, arg.totalWidth, isOutput = false)) ++
    outputArgs.map(arg => gsimCppArg(arg.name, arg.totalWidth, isOutput = true))
  private val dpiArgs = inputArgs.map(arg => dpiArg(arg.name, arg.totalWidth, isOutput = false)) ++
    outputArgs.map(arg => dpiArg(arg.name, arg.totalWidth, isOutput = true))
  private val modulePorts = Seq("input clock") ++
    inputArgs.map(arg => modulePortArg(arg.name, arg.totalWidth, isOutput = false)) ++
    outputArgs.map(arg => modulePortArg(arg.name, arg.totalWidth, isOutput = true))
  private val outputModuleTemps = outputArgs.map(arg => (s"${arg.name.stripPrefix("io_out_")}_tmp", arg.totalWidth, arg.name))
  private val outputCallTemps = outputArgs.map(arg => (s"${arg.name.stripPrefix("io_out_")}_dpi_tmp", arg.totalWidth, arg.name))
  private val svDpiCallArgs = inputArgs.map(_.name) ++ outputCallTemps.map(_._1)
  private val outputTmpDecls = outputModuleTemps.map { case (tmpName, width, _) => moduleWireDecl(tmpName, width) }
  private val synthTmpAssigns = outputModuleTemps.map { case (tmpName, width, _) =>
    if (width == 1) s"assign $tmpName = 1'b0;" else s"assign $tmpName = ${width}'b0;"
  }
  private val cppExtWideInputTemps = inputArgs.filter(_.totalWidth > 64).map { arg =>
    val tempName = s"${arg.name}_words"
    val wordCount = gsimWideWordCount(arg.totalWidth)
    s"uint32_t $tempName[$wordCount] = {};\n  std::memcpy($tempName, &${arg.name}, sizeof($tempName));"
  }
  private val cppExtWideOutputTemps = outputArgs.filter(_.totalWidth > 64).map { arg =>
    val tempName = s"${arg.name}_words"
    val wordCount = gsimWideWordCount(arg.totalWidth)
    s"uint32_t $tempName[$wordCount] = {};"
  }
  private val cppExtCallArgs = inputArgs.map { arg =>
    if (arg.totalWidth <= 64) arg.name else s"${arg.name}_words"
  } ++ outputArgs.map { arg =>
    if (arg.totalWidth <= 64) arg.name else s"${arg.name}_words"
  }
  private val cppExtWideOutputCopies = outputArgs.filter(_.totalWidth > 64).map { arg =>
    val tempName = s"${arg.name}_words"
    s"std::memcpy(&${arg.name}, $tempName, sizeof($tempName));"
  }

  private def localVarDecl(name: String, width: Int): String = {
    if (width <= 64) s"${scalarSvDpiType(width)} $name;" else s"bit [${width - 1}:0] $name;"
  }

  private def functionType(width: Int): String = {
    if (width <= 64) scalarSvDpiType(width) else s"bit [${width - 1}:0]"
  }

  private val outputEvalFuncs = outputArgs.map { arg =>
    val funcName = s"${arg.name}_value"
    val localDecls = outputCallTemps.map { case (tmpName, width, _) => localVarDecl(tmpName, width) }
    val localDefaults = outputCallTemps.map { case (tmpName, width, _) =>
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
  private val outputTmpAssigns = outputArgs.map(arg => {
    val tmpName = s"${arg.name.stripPrefix("io_out_")}_tmp"
    s"assign $tmpName = ${arg.name}_value();"
  })
  private val outputNetAssigns = outputArgs.map(arg => {
    val tmpName = s"${arg.name.stripPrefix("io_out_")}_tmp"
    s"assign ${arg.name} = $tmpName;"
  })

  private def maskExpr(width: Int): String = if (width == 32) "0xffffffffULL" else s"((1ULL << $width) - 1ULL)"

  private def decodeExpr(arg: PackedArg, idx: Int): String = {
    val offset = idx * arg.fieldWidth
    if (arg.totalWidth <= 64) {
      s"static_cast<uint32_t>((static_cast<uint64_t>(${arg.name}) >> $offset) & ${maskExpr(arg.fieldWidth)})"
    } else {
      s"get_packed_bits(${arg.name}, $offset, ${arg.fieldWidth})"
    }
  }

  private def encodeStmt(arg: PackedArg, idx: Int, valueExpr: String): String = {
    val offset = idx * arg.fieldWidth
    if (arg.totalWidth <= 64) {
      s"${arg.name}_packed |= static_cast<uint64_t>($valueExpr) << $offset;"
    } else {
      s"set_packed_bits(${arg.name}, $offset, ${arg.fieldWidth}, static_cast<uint32_t>($valueExpr));"
    }
  }

  private val inputAssigns = inputArgs.flatMap { arg =>
    val fieldName = arg.name.stripPrefix("io_in_")
    (0 until entriesNum).map(idx => s"in[$idx].$fieldName = ${decodeExpr(arg, idx)};")
  }
  private val scalarOutputAccums = outputArgs.filter(_.totalWidth <= 64).map(arg => s"uint64_t ${arg.name}_packed = 0;")
  private val wideOutputZeros = outputArgs.filter(_.totalWidth > 64).map(arg =>
    s"std::memset(${arg.name}, 0, ${((arg.totalWidth + 31) / 32)} * sizeof(uint32_t));"
  )
  private val outputAssigns = outputArgs.flatMap { arg =>
    val fieldName = arg.name.stripPrefix("io_out_")
    (0 until entriesNum).map(idx => encodeStmt(arg, idx, s"out[$idx].$fieldName"))
  }
  private val scalarOutputWrites = outputArgs.filter(_.totalWidth <= 64).map(arg =>
    s"${arg.name} = static_cast<${scalarCType(arg.totalWidth)}>(${arg.name}_packed);"
  )
  private val packedBitHelpers = if ((inputArgs ++ outputArgs).exists(_.totalWidth > 64)) {
    Seq(
      "auto get_packed_bits = [](const uint32_t *data, int lsb, int width) -> uint32_t {",
      "  const int wordIndex = lsb / 32;",
      "  const int bitIndex = lsb % 32;",
      "  uint64_t combined = static_cast<uint64_t>(data[wordIndex]) >> bitIndex;",
      "  if (bitIndex + width > 32) {",
      "    combined |= static_cast<uint64_t>(data[wordIndex + 1]) << (32 - bitIndex);",
      "  }",
      "  const uint64_t mask = width == 32 ? 0xffffffffULL : ((1ULL << width) - 1ULL);",
      "  return static_cast<uint32_t>(combined & mask);",
      "};",
      "auto set_packed_bits = [](uint32_t *data, int lsb, int width, uint32_t value) {",
      "  const int wordIndex = lsb / 32;",
      "  const int bitIndex = lsb % 32;",
      "  const uint64_t mask = width == 32 ? 0xffffffffULL : ((1ULL << width) - 1ULL);",
      "  uint64_t combined = static_cast<uint64_t>(data[wordIndex]);",
      "  if (bitIndex + width > 32) {",
      "    combined |= static_cast<uint64_t>(data[wordIndex + 1]) << 32;",
      "  }",
      "  const uint64_t fieldMask = mask << bitIndex;",
      "  combined = (combined & ~fieldMask) | ((static_cast<uint64_t>(value) & mask) << bitIndex);",
      "  data[wordIndex] = static_cast<uint32_t>(combined);",
      "  if (bitIndex + width > 32) {",
      "    data[wordIndex + 1] = static_cast<uint32_t>(combined >> 32);",
      "  }",
      "};",
    )
  } else {
    Seq.empty
  }

  private val wrapperBody =
    s"""
      |  TopdownIQInfoFrame in[$entriesNum] = {};
      |  TopdownExtendedIQInfoFrame out[$entriesNum] = {};
      |  ${packedBitHelpers.mkString("\n  ")}
      |  ${scalarOutputAccums.mkString("\n  ")}
      |  ${wideOutputZeros.mkString("\n  ")}
      |  ${inputAssigns.mkString("\n  ")}
      |  topdown_iq_info_apply($entriesNum, in, out);
      |  ${outputAssigns.mkString("\n  ")}
      |  ${scalarOutputWrites.mkString("\n  ")}
      |""".stripMargin

  private val cppDPICModule =
    s"""
      |extern "C" void $dpiFuncName (
      |  ${cppArgs.mkString(",\n  ")}
      |) {
      |$wrapperBody
      |}
      |""".stripMargin

  private val cppExtModule =
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
  difftest.DifftestModule.createCppDPICModule(dpiFuncName, cppDPICModule, Some("\"topdown_iq_info.h\""))
  difftest.DifftestModule.createCppExtModule(desiredName, cppExtModule, Some("\"topdown_iq_info.h\""))

  setInline(
    s"$desiredName.v",
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
     """.stripMargin,
  )
}

class TopdownIQInfoCollect(val entriesNum: Int) extends Module {
  import TopdownIQInfoPacking._

  val io = IO(new TopdownInfo(entriesNum))

  val helper = Module(new TopdownIQInfoHelper(entriesNum))
  helper.clock := clock

  private def packFields(data: Seq[UInt]): UInt = Cat(data.reverse)

  private def unpackField(data: UInt, idx: Int, fieldWidth: Int): UInt = {
    data((idx + 1) * fieldWidth - 1, idx * fieldWidth)
  }

  helper.io.in_valid := packFields(io.in.map(_.valid.asUInt))
  helper.io.in_robIdx := packFields(io.in.map(_.robIdx))
  helper.io.in_robFlag := packFields(io.in.map(_.robFlag.asUInt))
  helper.io.in_pipeNum := packFields(io.in.map(_.pipeNum))
  helper.io.in_cancelSource := packFields(io.in.map(_.cancelSource))
  helper.io.in_srcReady := packFields(io.in.map(_.srcReady.asUInt))
  helper.io.in_futype := packFields(io.in.map(_.futype))
  helper.io.in_issued := packFields(io.in.map(_.issued.asUInt))

  for (idx <- 0 until entriesNum) {
    io.out(idx).idealIssueTime := unpackField(helper.io.out_idealIssueTime, idx, 1).asBool
  }
}