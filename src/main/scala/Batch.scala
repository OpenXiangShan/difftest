/***************************************************************************************
 * Copyright (c) 2020-2024 Institute of Computing Technology, Chinese Academy of Sciences
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

package difftest.batch

import chisel3._
import chisel3.util._
import difftest._
import difftest.gateway.GatewayConfig
import difftest.common.DifftestPerf
import difftest.util.{LookupTree, PipelineConnect}

import scala.collection.mutable.ListBuffer

// Only instantiated once, use val instead of def
case class BatchParam(config: GatewayConfig, bundles: Seq[DifftestBundle]) {
  val infoWidth = (new BatchInfo).getWidth
  val infoByte = infoWidth / 8 // byteAligned by default

  // Maximum Byte length decided by transmission function
  val MaxDataByteLen = config.batchArgByteLen._1
  val MaxDataBitLen = MaxDataByteLen * 8
  val MaxInfoByteLen = config.batchArgByteLen._2
  val MaxInfoBitLen = MaxInfoByteLen * 8
  val MaxInfoSize = MaxInfoByteLen / infoByte

  val StepGroupSize = bundles.groupBy(_.desiredCppName).values.flatMap(_.grouped(8).toSeq).size
  val StepDataByteLen = bundles.map(_.getByteAlignWidth).map { w => w / 8 }.sum
  val StepDataBitLen = StepDataByteLen * 8
  val StepInfoByteLen = (StepGroupSize + 1) * (infoWidth / 8) // Include BatchStep to update buffer index
  val StepInfoBitLen = StepInfoByteLen * 8

  // Width of statistic for data/info byte length
  val StatsDataWidth = log2Ceil(math.max(MaxDataByteLen, StepDataByteLen))
  val StatsInfoWidth = log2Ceil(math.max(MaxInfoSize, StepGroupSize + 1))

  // Truncate width when shifting to reduce useless gates
  val TruncDataBitLen = math.min(MaxDataBitLen, StepDataBitLen)
}

class BatchIO(param: BatchParam) extends Bundle {
  val data = UInt(param.MaxDataBitLen.W)
  val info = UInt(param.MaxInfoBitLen.W)
}

class BatchStats(param: BatchParam) extends Bundle {
  val data_bytes = UInt(param.StatsDataWidth.W)
  val info_size = UInt(param.StatsInfoWidth.W)
}

class BatchOutput(param: BatchParam, config: GatewayConfig) extends Bundle {
  val io = new BatchIO(param)
  val step = UInt(config.stepWidth.W)
}

class BatchInfo extends Bundle {
  val id = UInt(8.W)
  val num = UInt(8.W)
}

class BatchStepResult(param: BatchParam, config: GatewayConfig) extends Bundle {
  val data = UInt(param.StepDataBitLen.W)
  val info = UInt(param.StepInfoBitLen.W)
  // status of step_data_head split in different loc
  val status = Vec(param.StepGroupSize, new BatchStats(param))
  val trace_info = Option.when(config.hasReplay)(new DiffTraceInfo(config))
}

object Batch {
  private val template = ListBuffer.empty[DifftestBundle]

  def apply(bundles: DecoupledIO[MixedVec[Valid[DifftestBundle]]], config: GatewayConfig): DecoupledIO[BatchOutput] = {
    template ++= chiselTypeOf(bundles.bits).map(_.bits).distinctBy(_.desiredCppName)
    val module = Module(new BatchEndpoint(chiselTypeOf(bundles.bits).toSeq, config))
    module.in <> bundles
    module.out
  }

  def getTemplate: Seq[DifftestBundle] = template.toSeq

  def getBundleID(bundleType: DifftestBundle): Int = {
    template.indexWhere(_.desiredCppName == bundleType.desiredCppName)
  }
}

class BatchEndpoint(bundles: Seq[Valid[DifftestBundle]], config: GatewayConfig) extends Module {
  val in = IO(Flipped(Decoupled(MixedVec(bundles))))
  val param = BatchParam(config, in.bits.map(_.bits).toSeq)

  // Collect valid bundles of same cycle
  val collector = Module(new BatchCollector(bundles, param, config))
  PipelineConnect(in, collector.in, collector.out.ready)

  // Assemble collected data from different cycles
  val assembler = Module(new BatchAssembler(param, config))
  assembler.in <> collector.out
  val out = IO(chiselTypeOf(assembler.out))
  out <> assembler.out
}

// Cluster Data from same group in same cycle
class BatchCluster(bundleType: DifftestBundle, groupSize: Int, param: BatchParam) extends Module {
  val alignWidth = bundleType.getByteAlignWidth
  val in = IO(Input(Vec(groupSize, Valid(bundleType))))
  val out_data = IO(Output(UInt((groupSize * alignWidth).W)))
  val out_info = IO(Output(UInt(param.infoWidth.W)))
  val status_base = IO(Input(new BatchStats(param)))
  val status_sum = IO(Output(new BatchStats(param)))

  val valid_sum = Seq.tabulate(groupSize) { idx =>
    PopCount(VecInit(in.map(_.valid.asUInt).take(idx + 1).toSeq).asUInt)
  }
  val v_size = valid_sum.last
  val aligned = in.map(_.bits.getByteAlign)
  val collect_data = Wire(Vec(groupSize, UInt(alignWidth.W)))
  collect_data.zipWithIndex.foreach { case (gen, vid) =>
    gen := VecInit((vid until groupSize).map { idx =>
      val prefix = if (idx == 0) 0.U else valid_sum(idx - 1)
      Mux(prefix === vid.U && in(idx).valid, aligned(idx), 0.U)
    }).reduce(_ | _)
  }
  out_data := collect_data.asUInt

  val info = Wire(new BatchInfo)
  info.id := Batch.getBundleID(bundleType).U
  info.num := v_size
  out_info := Mux(v_size =/= 0.U, info.asUInt, 0.U)

  val bytes_map = Seq.tabulate(groupSize + 1) { vi => (vi.U, (alignWidth / 8 * vi).U) }
  status_sum.data_bytes := status_base.data_bytes +& LookupTree(v_size, bytes_map)
  status_sum.info_size := status_base.info_size +& Mux(v_size =/= 0.U, 1.U, 0.U)
}

// Collect Data from different group in same cycle
class BatchCollector(bundles: Seq[Valid[DifftestBundle]], param: BatchParam, config: GatewayConfig) extends Module {
  val in = IO(Flipped(Decoupled(MixedVec(bundles))))
  val out = IO(Decoupled(new BatchStepResult(param, config)))

  def getGroupDataWidth: Seq[Valid[DifftestBundle]] => Int = { group =>
    group.length * group.head.bits.getByteAlignWidth
  }

  val in_group = in.bits.groupBy(_.bits.desiredCppName).values
  val in_group_single = in_group.filter(_.size == 1).toSeq
  val in_group_multi = in_group.filterNot(_.size == 1).flatMap(_.grouped(8)).toSeq
  val sorted = in_group_single.sortBy(getGroupDataWidth).reverse ++ in_group_multi.sortBy(getGroupDataWidth)

  // Stage 1: concat bundles with same desiredCppName
  class GroupBundle extends Bundle {
    val data = MixedVec(sorted.map(getGroupDataWidth).map(group_w => UInt(group_w.W)))
    val info = Vec(param.StepGroupSize, UInt(param.infoWidth.W))
    val status = Vec(param.StepGroupSize, new BatchStats(param))
    val trace_info = Option.when(config.hasReplay)(new DiffTraceInfo(config))
  }
  val grouped = Wire(Decoupled(new GroupBundle))
  grouped.valid := in.valid
  in.ready := grouped.ready

  grouped.bits.trace_info.foreach(
    _ := in.bits.map(_.bits).filter(_.desiredCppName == "trace_info").head.asInstanceOf[DiffTraceInfo]
  )
  sorted.zipWithIndex.foreach { case (v_gens, gid) =>
    val cluster = Module(new BatchCluster(chiselTypeOf(v_gens.head.bits), v_gens.length, param))
    cluster.in := v_gens
    val status_base = if (gid == 0) 0.U.asTypeOf(new BatchStats(param)) else grouped.bits.status(gid - 1)
    cluster.status_base := status_base
    grouped.bits.data(gid) := cluster.out_data
    grouped.bits.info(gid) := cluster.out_info
    grouped.bits.status(gid) := cluster.status_sum
  }

  // Stage 2: delay grouped data, concat different group
  val delay_grouped = Wire(Decoupled(new GroupBundle))
  PipelineConnect(grouped, delay_grouped, out.ready)
  delay_grouped.ready := out.ready

  val delay_group_data = delay_grouped.bits.data
  val delay_group_info = delay_grouped.bits.info
  val delay_group_status = delay_grouped.bits.status
  val info_num = delay_group_status.last.info_size
  val BatchStep = Wire(new BatchInfo)
  BatchStep.id := Batch.getTemplate.length.U
  BatchStep.num := info_num // unused, only for debugging

  out.valid := delay_grouped.valid && info_num =/= 0.U
  // append BatchStep to last step_status
  out.bits.status := delay_group_status
  out.bits.status.last.info_size := delay_group_status.last.info_size + 1.U

  val toCat_data = delay_group_data.take(in_group_single.size).reverse
  val toCat_info = delay_group_info.take(in_group_single.size).reverse
  val res_single = Wire(MixedVec(Seq.tabulate(in_group_single.size) { idx =>
    val width = toCat_data.take(idx + 1).map(_.getWidth).sum
    UInt(width.W)
  }))
  res_single(0) := toCat_data(0)
  (1 until in_group_single.size).foreach { idx =>
    val base = res_single(idx - 1)
    res_single(idx) := Mux(toCat_info(idx) =/= 0.U, Cat(base, toCat_data(idx)), base)
  }
  // Collect data by shifter
  val res_multi = if (in_group_multi.size != 0) {
    MixedVecInit(
      delay_group_data.zipWithIndex
        .drop(in_group_single.size)
        .map { case (gen, idx) =>
          val maxWidth = delay_group_data.take(idx).map(_.getWidth).sum
          // Truncate width of offset to reduce useless gates
          val offset = Wire(UInt(log2Ceil(maxWidth + 1).W))
          if (idx == 0) {
            offset := 0.U
          } else {
            offset := delay_group_status(idx - 1).data_bytes << 3
          }
          (gen << offset).asUInt
        }
        .toSeq
    ).reduce(_ | _)
  } else {
    0.U
  }
  out.bits.data := res_single.last | res_multi

  // Collect info from tail, collect(i) include last 0~i
  val toCollect_info = delay_group_info.reverse
  val info_res = Wire(MixedVec(Seq.tabulate(param.StepGroupSize) { idx => UInt(((idx + 2) * param.infoWidth).W) }))
  Seq.tabulate(param.StepGroupSize) { idx =>
    val info_base = if (idx == 0) BatchStep.asUInt else info_res(idx - 1)
    info_res(idx) := Mux(toCollect_info(idx) =/= 0.U, Cat(info_base, toCollect_info(idx)), info_base)
  }
  out.bits.info := info_res.last
  out.bits.trace_info.foreach(_ := delay_grouped.bits.trace_info.get)
}

// Assemble step_data from different cycles
class BatchAssembler(
  param: BatchParam,
  config: GatewayConfig,
) extends Module {
  val in = IO(Flipped(Decoupled(new BatchStepResult(param, config))))
  val out = IO(Decoupled(new BatchOutput(param, config)))

  val state_data = RegInit(0.U(param.MaxDataBitLen.W))
  val state_info = RegInit(0.U(param.MaxInfoBitLen.W))
  val state_status = RegInit(0.U.asTypeOf(new BatchStats(param)))
  val state_step_cnt = RegInit(0.U(config.stepWidth.W))
  val state_trace_size = Option.when(config.hasReplay)(RegInit(0.U(config.replayWidth.W)))

  // Assemble step data/info into state in 3 stage
  // Stage 1:
  //   1. RegNext signal from BatchCollector to cut of combination logic path
  //   1. data/info_exceed_vec: mark whether different length fragments of step data/info exceed available space
  //   2. concat/remain_stats: record statistic for data/info to be concatenated to output or remained to state
  val delay_step = Wire(Decoupled(new BatchStepResult(param, config)))
  PipelineConnect(in, delay_step, out.ready)
  delay_step.ready := out.ready
  val delay_step_data = delay_step.bits.data
  val delay_step_info = delay_step.bits.info
  val delay_step_status = delay_step.bits.status
  val delay_step_enable = delay_step.valid
  val delay_step_trace_info = delay_step.bits.trace_info
  val data_bytes_avail = param.MaxDataByteLen.U -& state_status.data_bytes
  // Always leave space for BatchFinish, use MaxInfoSize - 1
  val info_size_avail = (param.MaxInfoSize - 1).U -& state_status.info_size
  val data_exceed = Wire(Bool())
  val info_exceed = Wire(Bool())
  val append_data = Wire(UInt(param.TruncDataBitLen.W))
  val append_info = Wire(UInt(param.StepInfoBitLen.W))
  val finish_step = Wire(UInt(config.stepWidth.W))
  val next_state_step_cnt = Wire(UInt(config.stepWidth.W))
  val next_state_data = Wire(UInt(param.MaxDataBitLen.W))
  val next_state_info = Wire(UInt(param.MaxInfoBitLen.W))
  val next_state_stats = Wire(new BatchStats(param))

  val BatchFinish = Wire(new BatchInfo)
  BatchFinish.id := (Batch.getTemplate.length + 1).U
  BatchFinish.num := finish_step

  val step_exceed = delay_step_enable && (state_step_cnt === config.batchSize.U)
  val cont_exceed = data_exceed || info_exceed
  val state_flush = in.valid && in.bits.status.last.data_bytes >= param.MaxDataByteLen.U // use Stage 1 bytes to flush ahead

  if (config.batchSplit) {
    val data_exceed_v = VecInit(delay_step_status.map(_.data_bytes > data_bytes_avail && delay_step_enable))
    val info_exceed_v = VecInit(delay_step_status.map(_.info_size > info_size_avail && delay_step_enable))
    data_exceed := data_exceed_v.asUInt.orR
    info_exceed := info_exceed_v.asUInt.orR
    val exceed_v = VecInit(data_exceed_v.zip(info_exceed_v).map { case (de, ie) => de | ie })

    // Extract last non-exceed stats
    // When no stats exceeds, return step_stats to append whole step for state flushing
    val concat_mask = VecInit.tabulate(param.StepGroupSize - 1) { idx => exceed_v(idx) ^ exceed_v(idx + 1) }
    val concat_stats = Mux(
      !exceed_v.asUInt.orR,
      delay_step_status.last,
      VecInit(delay_step_status.dropRight(1).zip(concat_mask).map { case (stats, mask) =>
        Mux(mask, stats.asUInt, 0.U)
      }).reduceTree(_ | _).asTypeOf(new BatchStats(param)),
    )
    val remain_stats = Wire(new BatchStats(param))
    remain_stats.data_bytes := delay_step_status.last.data_bytes -& concat_stats.data_bytes
    remain_stats.info_size := delay_step_status.last.info_size -& concat_stats.info_size
    assert(remain_stats.data_bytes <= param.MaxDataByteLen.U)
    assert(remain_stats.info_size + 1.U <= param.MaxInfoSize.U)

    // Note we need only lowest bits to update state, truncate high bits to reduce gates
    val concat_data = (~(~0.U(param.TruncDataBitLen.W) <<
      (concat_stats.data_bytes << 3).asUInt)).asUInt & delay_step_data
    val concat_info = (~(~0.U(param.StepInfoBitLen.W) <<
      (concat_stats.info_size * param.infoWidth.U))).asUInt & delay_step_info
    val remain_data = (delay_step_data >> (concat_stats.data_bytes << 3).asUInt).asUInt
    val remain_info = (delay_step_info >> (concat_stats.info_size * param.infoWidth.U)).asUInt

    // Delay step can be partly appended to output for making full use of transmission param
    // Avoid appending when step equals batchSize(delay_step_exceed), last appended data will overwrite first step data
    val has_append = delay_step_enable && (state_flush || cont_exceed) && !exceed_v.asUInt.andR && !step_exceed
    // When the whole step is appended to output, state_step should be 0, and output step + 1
    val append_whole = has_append && !cont_exceed
    finish_step := state_step_cnt + Mux(append_whole, 1.U, 0.U)

    append_data := Mux(has_append, concat_data(param.TruncDataBitLen - 1, 0), 0.U)
    val append_finish_map = Seq.tabulate(param.StepGroupSize + 2) { g =>
      (g.U, (BatchFinish.asUInt << (g * param.infoWidth)).asUInt)
    }
    append_info := Mux(
      has_append,
      concat_info | LookupTree(concat_stats.info_size, append_finish_map),
      BatchFinish.asUInt,
    )

    next_state_step_cnt := Mux(has_append && append_whole, 0.U, 1.U)
    next_state_data := Mux(has_append, remain_data, delay_step_data)
    next_state_info := Mux(has_append, remain_info, delay_step_info)
    next_state_stats.data_bytes := Mux(has_append, remain_stats.data_bytes, delay_step_status.last.data_bytes)
    next_state_stats.info_size := Mux(has_append, remain_stats.info_size, delay_step_status.last.info_size)
  } else {
    data_exceed := delay_step_enable && delay_step_status.last.data_bytes > data_bytes_avail
    info_exceed := delay_step_enable && delay_step_status.last.info_size > info_size_avail
    assert(delay_step_status.last.data_bytes <= param.MaxDataByteLen.U)
    assert(delay_step_status.last.info_size <= param.MaxInfoSize.U)

    finish_step := state_step_cnt
    append_data := 0.U
    append_info := BatchFinish.asUInt

    next_state_step_cnt := 1.U
    next_state_data := delay_step_data
    next_state_info := delay_step_info
    next_state_stats.data_bytes := delay_step_status.last.data_bytes
    next_state_stats.info_size := delay_step_status.last.info_size
  }

  // Stage 2:
  // update state
  val trace_exceed = Option.when(config.hasReplay) {
    delay_step_enable && (state_trace_size.get +& delay_step_trace_info.get.trace_size >= config.replaySize.U)
  }
  val timeout_count = RegInit(0.U(32.W))
  val timeout = timeout_count === 200000.U
  if (config.hasBuiltInPerf) {
    DifftestPerf("BatchExceed_data", data_exceed)
    DifftestPerf("BatchExceed_info", info_exceed)
    DifftestPerf("BatchExceed_step", step_exceed.asUInt)
    DifftestPerf("BatchExceed_flush", state_flush.asUInt)
    DifftestPerf("BatchExceed_timeout", timeout.asUInt)
    if (config.hasReplay) DifftestPerf("BatchExceed_trace", trace_exceed.get.asUInt)
  }
  val in_replay = Option.when(config.hasReplay)(delay_step.bits.trace_info.get.in_replay)

  val should_tick = timeout || state_flush || cont_exceed || step_exceed ||
    trace_exceed.getOrElse(false.B) || in_replay.getOrElse(false.B)
  when(!should_tick) {
    timeout_count := timeout_count + 1.U
  }.otherwise {
    timeout_count := 0.U
  }

  out.bits.io.data := state_data | (append_data << (state_status.data_bytes << 3).asUInt).asUInt
  out.bits.io.info := state_info | (append_info << (state_status.info_size * param.infoWidth.U)).asUInt
  out.bits.step := Mux(out.valid, finish_step, 0.U)
  out.valid := should_tick

  val state_update = delay_step_enable || state_flush || timeout

  when(state_update) {
    when(delay_step_enable) {
      when(should_tick) {
        state_step_cnt := next_state_step_cnt
        state_data := next_state_data
        state_info := next_state_info
        state_status := next_state_stats
        if (config.hasReplay) state_trace_size.get := delay_step_trace_info.get.trace_size
      }.otherwise {
        state_step_cnt := state_step_cnt + 1.U
        state_data := state_data |
          (delay_step_data(param.TruncDataBitLen - 1, 0) << (state_status.data_bytes << 3).asUInt).asUInt
        state_info := state_info |
          (delay_step_info << (state_status.info_size * param.infoWidth.U)).asUInt
        state_status.data_bytes := state_status.data_bytes + delay_step_status.last.data_bytes
        state_status.info_size := state_status.info_size + delay_step_status.last.info_size
        if (config.hasReplay) state_trace_size.get := state_trace_size.get + delay_step_trace_info.get.trace_size
      }
    }.otherwise { // state_flush without new-coming step
      state_step_cnt := 0.U
      state_data := 0.U
      state_info := 0.U
      state_status.data_bytes := 0.U
      state_status.info_size := 0.U
      if (config.hasReplay) state_trace_size.get := 0.U
    }
  }
}
