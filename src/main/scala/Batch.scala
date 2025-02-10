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
import difftest.util.LookupTree

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

  val StepGroupSize = bundles.distinctBy(_.desiredCppName).length
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
  val enable = Bool()
  val step = UInt(config.stepWidth.W)
}

class BatchInfo extends Bundle {
  val id = UInt(8.W)
  val num = UInt(8.W)
}

object Batch {
  private val template = ListBuffer.empty[DifftestBundle]

  def apply(bundles: MixedVec[Valid[DifftestBundle]], config: GatewayConfig): BatchOutput = {
    template ++= chiselTypeOf(bundles).map(_.bits).distinctBy(_.desiredCppName)
    val module = Module(new BatchEndpoint(chiselTypeOf(bundles).toSeq, config))
    module.in := bundles
    module.out
  }

  def getTemplate: Seq[DifftestBundle] = template.toSeq

  def getBundleID(bundleType: DifftestBundle): Int = {
    template.indexWhere(_.desiredCppName == bundleType.desiredCppName)
  }
}

class BatchEndpoint(bundles: Seq[Valid[DifftestBundle]], config: GatewayConfig) extends Module {
  val in = IO(Input(MixedVec(bundles)))
  val param = BatchParam(config, in.map(_.bits).toSeq)

  // Collect valid bundles of same cycle
  val collector = Module(new BatchCollector(bundles, param))
  collector.in := in
  val step_data = collector.step_data
  val step_info = collector.step_info
  val step_enable = collector.step_enable
  val step_status = collector.step_status

  // Assemble collected data from different cycles
  val assembler = Module(new BatchAssembler(param, config))
  assembler.step_data := step_data
  assembler.step_info := step_info
  assembler.step_status := step_status
  assembler.step_enable := step_enable
  if (config.hasReplay) {
    val trace_info = in.map(_.bits).filter(_.desiredCppName == "trace_info").head.asInstanceOf[DiffTraceInfo]
    assembler.step_trace_info.get := trace_info
  }
  val out = IO(Output(new BatchOutput(param, config)))
  out := assembler.out
}

class BatchCollector(bundles: Seq[Valid[DifftestBundle]], param: BatchParam) extends Module {
  val in = IO(Input(MixedVec(bundles)))
  val step_data = IO(Output(UInt(param.StepDataBitLen.W)))
  val step_info = IO(Output(UInt(param.StepInfoBitLen.W)))
  // status of step_data_head split in different loc
  val step_status = IO(Output(Vec(param.StepGroupSize, new BatchStats(param))))
  val step_enable = IO(Output(Bool()))

  val sorted =
    in.groupBy(_.bits.desiredCppName).values.toSeq.sortBy(gen => gen.length * gen.head.bits.getByteAlignWidth).reverse
  // Stage 1: concat bundles with same desiredCppName
  val group_bitlen = sorted.map(_.head.bits.getByteAlignWidth)
  val group_length = sorted.map(_.length)
  val group_info = Wire(Vec(param.StepGroupSize, UInt(param.infoWidth.W)))
  val group_status = Wire(Vec(param.StepGroupSize, new BatchStats(param)))
  val group_data = Wire(MixedVec(Seq.tabulate(param.StepGroupSize) { idx =>
    UInt((group_length(idx) * group_bitlen(idx)).W)
  }))
  val group_vsize = Wire(Vec(param.StepGroupSize, UInt(8.W)))
  sorted.zipWithIndex.foreach { case (v_gens, gid) =>
    val aligned = v_gens.map(_.bits.getByteAlign)
    val valids = v_gens.map(_.valid)
    val valid_sum = Seq.tabulate(valids.length) { idx =>
      VecInit(valids.map(_.asUInt).take(idx + 1).toSeq).reduce(_ +& _)
    }
    val v_size = valid_sum.last
    group_vsize(gid) := v_size
    val info = Wire(new BatchInfo)
    info.id := Batch.getBundleID(v_gens.head.bits).U
    info.num := v_size
    group_info(gid) := Mux(v_size =/= 0.U, info.asUInt, 0.U)
    val data_bytes = {
      val offset_map = Seq.tabulate(valids.length + 1) { vi => (vi.U, (group_bitlen(gid) / 8 * vi).U) }
      LookupTree(v_size, offset_map)
    }
    val info_size = Mux(VecInit(valids.toSeq).asUInt.orR, 1.U, 0.U)
    if (gid == 0) {
      group_status(gid).data_bytes := data_bytes
      group_status(gid).info_size := info_size
    } else {
      group_status(gid).data_bytes := group_status(gid - 1).data_bytes +& data_bytes
      group_status(gid).info_size := group_status(gid - 1).info_size +& info_size
    }
    val v_aligned = aligned.zip(valids).map { case (gen, v) => Mux(v, gen, 0.U) }
    val collect_data = Wire(Vec(group_length(gid), UInt(group_bitlen(gid).W)))
    collect_data.zipWithIndex.foreach { case (gen, vid) =>
      gen := VecInit((vid until group_length(gid)).map { idx =>
        Mux(valid_sum(idx) === (vid + 1).U, v_aligned(idx), 0.U)
      }).reduce(_ | _)
    }
    group_data(gid) := collect_data.asUInt
  }

  // Stage 2: delay grouped data, concat different group
  val delay_group_data = RegNext(group_data)
  val delay_group_info = RegNext(group_info)
  val delay_group_status = RegNext(group_status)
  val delay_group_vsize = RegNext(group_vsize)
  val info_num = delay_group_status.last.info_size
  step_enable := info_num =/= 0.U
  step_status := delay_group_status
  // append BatchStep to last step_status
  step_status.last.info_size := delay_group_status.last.info_size + 1.U
  // Use BatchStep to update index of software buffer
  val BatchStep = Wire(new BatchInfo)
  BatchStep.id := Batch.getTemplate.length.U
  BatchStep.num := info_num // unused, only for debugging
  // Collect from tail, collect(i) include last 0~i
  val toCollect_data = delay_group_data.reverse
  val toCollect_info = delay_group_info.reverse
  val toCollect_vsize = delay_group_vsize.reverse
  val collect_data = Wire(MixedVec(Seq.tabulate(param.StepGroupSize) { idx =>
    UInt(toCollect_data.take(idx + 1).map(_.getWidth).sum.W)
  }))
  val collect_info = Wire(MixedVec(Seq.tabulate(param.StepGroupSize) { idx =>
    UInt(((idx + 2) * param.infoWidth).W)
  }))

  collect_data(0) := toCollect_data(0)
  collect_info(0) := Mux(toCollect_vsize(0) =/= 0.U, Cat(BatchStep.asUInt, toCollect_info(0)), BatchStep.asUInt)
  (1 until param.StepGroupSize).foreach { idx =>
    val cat_map = Seq.tabulate(group_length.reverse(idx) + 1) { len =>
      (len.U, Cat(collect_data(idx - 1), toCollect_data(idx)(len * group_bitlen.reverse(idx) - 1, 0)))
    }
    collect_data(idx) := LookupTree(toCollect_vsize(idx), cat_map)
    collect_info(idx) := Mux(
      toCollect_vsize(idx) =/= 0.U,
      Cat(collect_info(idx - 1), toCollect_info(idx)),
      collect_info(idx - 1),
    )
  }
  step_data := collect_data.last
  step_info := collect_info.last
}

class BatchAssembler(
  param: BatchParam,
  config: GatewayConfig,
) extends Module {
  val step_data = IO(Input(UInt(param.StepDataBitLen.W)))
  val step_info = IO(Input(UInt(param.StepInfoBitLen.W)))
  val step_status = IO(Input(Vec(param.StepGroupSize, new BatchStats(param))))
  val step_enable = IO(Input(Bool()))
  val step_trace_info = Option.when(config.hasReplay)(IO(Input(new DiffTraceInfo(config))))
  val out = IO(Output(new BatchOutput(param, config)))

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
  val delay_step_data = RegNext(step_data)
  val delay_step_info = RegNext(step_info)
  val delay_step_status = RegNext(step_status)
  val delay_step_enable = RegNext(step_enable)
  val delay_step_trace_info = Option.when(config.hasReplay)(RegNext(step_trace_info.get))
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
  val state_flush = step_enable && step_status.last.data_bytes >= param.MaxDataByteLen.U // use Stage 1 bytes to flush ahead

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
  val in_replay = Option.when(config.hasReplay)(step_trace_info.get.in_replay)

  val should_tick = timeout || state_flush || cont_exceed || step_exceed ||
    trace_exceed.getOrElse(false.B) || in_replay.getOrElse(false.B)
  when(!should_tick) {
    timeout_count := timeout_count + 1.U
  }.otherwise {
    timeout_count := 0.U
  }

  out.io.data := state_data | (append_data << (state_status.data_bytes << 3).asUInt).asUInt
  out.io.info := state_info | (append_info << (state_status.info_size * param.infoWidth.U)).asUInt
  out.enable := should_tick
  out.step := Mux(out.enable, finish_step, 0.U)

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
