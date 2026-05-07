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

class TopdownRobInfo extends Bundle {
  val valid = Bool()
  val robIdx = UInt(16.W)
  val robFlag = Bool()
  val cancelSource = UInt(3.W)
  val issued = Bool()
  val idealIssueTime = Bool()
}

class TopdownRobInfoBundle(val IQEntriesNum: Int, val RobEntriesNum: Int) extends Bundle {
  val in = Input(Vec(IQEntriesNum, new TopdownRobInfo))
  val out = Output(Vec(RobEntriesNum, new TopdownRobInfo))
}

object TopdownRobInfoPacking {
  val fieldSpecs: Seq[(String, Int)] = Seq(
    "valid" -> 1,
    "robIdx" -> 16,
    "robFlag" -> 1,
    "cancelSource" -> 3,
    "issued" -> 1,
    "idealIssueTime" -> 1,
  )

  def packedWidth(entriesNum: Int, fieldWidth: Int): Int = entriesNum * fieldWidth
}

class TopdownRobInfoPackedBundle(val IQEntriesNum: Int, val RobEntriesNum: Int) extends Bundle {
  import TopdownRobInfoPacking._

  val in_valid = Input(UInt(packedWidth(IQEntriesNum, 1).W))
  val in_robIdx = Input(UInt(packedWidth(IQEntriesNum, 16).W))
  val in_robFlag = Input(UInt(packedWidth(IQEntriesNum, 1).W))
  val in_cancelSource = Input(UInt(packedWidth(IQEntriesNum, 3).W))
  val in_issued = Input(UInt(packedWidth(IQEntriesNum, 1).W))
  val in_idealIssueTime = Input(UInt(packedWidth(IQEntriesNum, 1).W))

  val out_valid = Output(UInt(packedWidth(RobEntriesNum, 1).W))
  val out_robIdx = Output(UInt(packedWidth(RobEntriesNum, 16).W))
  val out_robFlag = Output(UInt(packedWidth(RobEntriesNum, 1).W))
  val out_cancelSource = Output(UInt(packedWidth(RobEntriesNum, 3).W))
  val out_issued = Output(UInt(packedWidth(RobEntriesNum, 1).W))
  val out_idealIssueTime = Output(UInt(packedWidth(RobEntriesNum, 1).W))
}

class TopdownRobInfoHelper(val IQEntriesNum: Int, val RobEntriesNum: Int) extends ExtModule with HasExtModuleInline {
  import TopdownRobInfoPacking._

  private case class PackedArg(name: String, totalWidth: Int, fieldWidth: Int, entriesNum: Int)

  override def desiredName: String = s"TopdownRobInfoHelper_${IQEntriesNum}_${RobEntriesNum}"

  private val dpiFuncName = s"topdown_rob_info_${IQEntriesNum}_${RobEntriesNum}"

  val io = IO(new TopdownRobInfoPackedBundle(IQEntriesNum, RobEntriesNum))

  private val inputArgs = fieldSpecs.map { case (fieldName, fieldWidth) =>
    PackedArg(s"io_in_$fieldName", packedWidth(IQEntriesNum, fieldWidth), fieldWidth, IQEntriesNum)
  }
  private val outputArgs = fieldSpecs.map { case (fieldName, fieldWidth) =>
    PackedArg(s"io_out_$fieldName", packedWidth(RobEntriesNum, fieldWidth), fieldWidth, RobEntriesNum)
  }

  private def scalarCType(width: Int): String = width match {
    case 1                                  => "bool"
    case w if w > 1 && w <= 8               => "uint8_t"
    case w if w > 8 && w <= 16              => "uint16_t"
    case w if w > 16 && w <= 32             => "uint32_t"
    case w if w > 32 && w <= 64             => "uint64_t"
    case w                                  => throw new Exception(s"Unsupported C width: $w")
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

  private def dpiArg(name: String, width: Int, isOutput: Boolean): String = {
    val direction = if (isOutput) "output" else "input"
    val typeString = if (width == 1) "bit" else s"bit [${width - 1}:0]"
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

  private val cppArgs = inputArgs.map(arg => cppArg(arg.name, arg.totalWidth, isOutput = false)) ++
    outputArgs.map(arg => cppArg(arg.name, arg.totalWidth, isOutput = true))
  private val dpiArgs = inputArgs.map(arg => dpiArg(arg.name, arg.totalWidth, isOutput = false)) ++
    outputArgs.map(arg => dpiArg(arg.name, arg.totalWidth, isOutput = true))
  private val modulePorts =
    inputArgs.map(arg => modulePortArg(arg.name, arg.totalWidth, isOutput = false)) ++
      outputArgs.map(arg => modulePortArg(arg.name, arg.totalWidth, isOutput = true))
  private val outputRegs = outputArgs.map(arg => (s"${arg.name}_reg", arg.totalWidth, arg.name))
  private val dpiCallArgs = inputArgs.map(_.name) ++ outputRegs.map(_._1)
  private val synthDefaults = outputRegs.map { case (regName, width, _) =>
    if (width == 1) s"$regName = 1'b0;" else s"$regName = ${width}'b0;"
  }
  private val outputRegDecls = outputRegs.map { case (regName, width, _) => moduleRegDecl(regName, width) }
  private val outputNetAssigns = outputRegs.map { case (regName, _, portName) => s"assign $portName = $regName;" }

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
    (0 until IQEntriesNum).map(idx => s"in[$idx].$fieldName = ${decodeExpr(arg, idx)};")
  }
  private val scalarOutputAccums = outputArgs.filter(_.totalWidth <= 64).map(arg => s"uint64_t ${arg.name}_packed = 0;")
  private val wideOutputZeros = outputArgs.filter(_.totalWidth > 64).map(arg =>
    s"std::memset(${arg.name}, 0, ${((arg.totalWidth + 31) / 32)} * sizeof(uint32_t));"
  )
  private val outputAssigns = outputArgs.flatMap { arg =>
    val fieldName = arg.name.stripPrefix("io_out_")
    (0 until RobEntriesNum).map(idx => encodeStmt(arg, idx, s"out[$idx].$fieldName"))
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
       |  TopdownRobInfoFrame in[$IQEntriesNum] = {};
       |  TopdownRobInfoFrame out[$RobEntriesNum] = {};
       |  ${packedBitHelpers.mkString("\n  ")}
       |  ${scalarOutputAccums.mkString("\n  ")}
       |  ${wideOutputZeros.mkString("\n  ")}
       |  ${inputAssigns.mkString("\n  ")}
       |  topdown_rob_info_apply($IQEntriesNum, $RobEntriesNum, in, out);
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
       |void $desiredName (
       |  ${cppArgs.mkString(",\n  ")}
       |) {
       |  $dpiFuncName(
       |    ${dpiCallArgs.mkString(",\n    ")}
       |  );
       |}
       |""".stripMargin
  difftest.DifftestModule.createCppDPICModule(dpiFuncName, cppDPICModule, Some("\"topdown_rob_info.h\""))
  difftest.DifftestModule.createCppExtModule(desiredName, cppExtModule, Some("\"topdown_rob_info.h\""))

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
      |  ${outputRegDecls.mkString("\n  ")}
      |  ${outputNetAssigns.mkString("\n  ")}
       |
       |`ifdef SYNTHESIS
       |  always @(*) begin
       |    ${synthDefaults.mkString("\n    ")}
       |  end
       |`else
      |  always @(*) begin
      |    ${synthDefaults.mkString("\n    ")}
       |    $dpiFuncName(
       |      ${dpiCallArgs.mkString(",\n      ")}
       |    );
       |  end
       |`endif // SYNTHESIS
       |
       |endmodule
     """.stripMargin,
  )
}

class TopdownRobInfoCollect(val IQEntriesNum: Int, val RobEntriesNum: Int) extends Module {
  import TopdownRobInfoPacking._

  val io = IO(new TopdownRobInfoBundle(IQEntriesNum, RobEntriesNum))

  val helper = Module(new TopdownRobInfoHelper(IQEntriesNum, RobEntriesNum))

  private def packFields(data: Seq[UInt]): UInt = Cat(data.reverse)

  private def unpackField(data: UInt, idx: Int, fieldWidth: Int): UInt = {
    data((idx + 1) * fieldWidth - 1, idx * fieldWidth)
  }

  helper.io.in_valid := packFields(io.in.map(_.valid.asUInt))
  helper.io.in_robIdx := packFields(io.in.map(_.robIdx))
  helper.io.in_robFlag := packFields(io.in.map(_.robFlag.asUInt))
  helper.io.in_cancelSource := packFields(io.in.map(_.cancelSource))
  helper.io.in_issued := packFields(io.in.map(_.issued.asUInt))
  helper.io.in_idealIssueTime := packFields(io.in.map(_.idealIssueTime.asUInt))

  for (idx <- 0 until RobEntriesNum) {
    io.out(idx).valid := unpackField(helper.io.out_valid, idx, 1).asBool
    io.out(idx).robIdx := unpackField(helper.io.out_robIdx, idx, 16)
    io.out(idx).robFlag := unpackField(helper.io.out_robFlag, idx, 1).asBool
    io.out(idx).cancelSource := unpackField(helper.io.out_cancelSource, idx, 3)
    io.out(idx).issued := unpackField(helper.io.out_issued, idx, 1).asBool
    io.out(idx).idealIssueTime := unpackField(helper.io.out_idealIssueTime, idx, 1).asBool
  }

}