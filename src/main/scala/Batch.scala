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
import difftest.util.{Delayer, LookupTree}

import scala.collection.mutable.ListBuffer

// Only instantiated once, use val instead of def
case class BatchParam(config: GatewayConfig, bundles: Seq[DifftestBundle]) {
  val grainBytes = config.batchGrainBytes
  val grainPowers = log2Ceil(grainBytes)
  println("power "+ grainPowers)
  val infoWidth = (new BatchInfo).getWidth
  val infoByte = infoWidth / 8 // byteAligned by default

  // Maximum Byte length decided by transmission function
  val MaxDataByteLen = config.batchArgByteLen._1
  val MaxDataBitLen = MaxDataByteLen * 8
//  val MaxDataLenWidth = log2Ceil(MaxDataByteLen)
  val MaxInfoByteLen = config.batchArgByteLen._2
  val MaxInfoBitLen = MaxInfoByteLen * 8
  val MaxDataGrains = MaxDataByteLen / grainBytes
  val MaxInfoSize = MaxInfoByteLen / infoByte
//  val MaxInfoLenWidth = log2Ceil(MaxInfoByteLen)

  val StepGroupSize = bundles.distinctBy(_.desiredCppName).length
  val StepDataGrains = bundles.map(_.getByteAlignWidth).map{ w =>
    (w / 8 + grainBytes - 1) / grainBytes
  }.sum
  val StepDataByteLen = StepDataGrains * grainBytes
  val StepDataBitLen = StepDataByteLen * 8
  // Include BatchInterval
  val StepInfoSize = StepGroupSize + 1
  val StepInfoByteLen = StepInfoSize * (infoWidth / 8)
  val StepInfoBitLen = StepInfoByteLen * 8


  val StatsDataWidth = log2Ceil(math.max(MaxDataGrains, StepDataGrains))
  val StatsInfoWidth = log2Ceil(math.max(MaxInfoSize, StepInfoSize))
//  // Width of statistic for data/info byte length
//  def StatsDataLenWidth = log2Ceil((collectDataWidth + 7) / 8)
//  def collectInfoWidth = collectLength * infoWidth
//  def StatsInfoLenWidth = log2Ceil((collectInfoWidth + 7) / 8)
//
//  // Truncate width when shifting to reduce useless gates
//  def TruncDataBitLen = math.min(MaxDataBitLen, collectDataWidth)
//  def TruncInfoBitLen = math.min(MaxInfoBitLen, collectInfoWidth)
//  def TruncDataLenWidth = math.min(MaxDataLenWidth, StatsDataLenWidth)
//  def TruncInfoLenWidth = math.min(MaxInfoLenWidth, StatsInfoLenWidth)
}

class BatchIO(param: BatchParam) extends Bundle {
  val data = UInt(param.MaxDataBitLen.W)
  val info = UInt(param.MaxInfoBitLen.W)
}

class BatchStats(param: BatchParam) extends Bundle {
  val data_grains = UInt(param.StatsDataWidth.W)
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
  val param = BatchParam(config, in.map(_.bits))

  // Collect bundles with valid of same cycle in parallel
  val collector = Module(new BatchCollector(bundles, param))
  collector.in := in
  val step_data = collector.step_data
  val step_info = collector.step_info
  val step_enable = collector.step_enable
  val step_status = collector.step_status
  val step_grains_hint = collector.step_grains_hint


//  val out = IO(Output(new BatchOutput(param, config)))
//  out.io.data := step_data
//  out.io.info := step_info
//  out.step := Mux(step_enable, 1.U, 0.U)
//  out.enable := step_enable

  // Assemble collected data from different cycles
  val assembler = Module(new BatchAssembler(step_grains_hint, param, config))
  assembler.step_data := step_data
  assembler.step_info := step_info
  assembler.step_status := step_status
  assembler.step_enable := step_enable
  if (config.hasReplay) {
    val trace_info = in.map(_.bits).filter(_.desiredCppName == "trace_info").head.asInstanceOf[DiffTraceInfo]
    assembler.step_trace_info.get := trace_info
  }
//
  val out = IO(Output(new BatchOutput(param, config)))
  out := assembler.out
}

class BatchCollector(bundles: Seq[Valid[DifftestBundle]], param: BatchParam) extends Module {
  val in = IO(Input(MixedVec(bundles)))
  val step_data = IO(Output(UInt(param.StepDataBitLen.W)))
  val step_info = IO(Output(UInt(param.StepInfoBitLen.W)))
  val step_status = IO(Output(Vec(param.StepGroupSize, new BatchStats(param))))
  val step_enable = IO(Output(Bool()))

  val sorted = in.groupBy(_.bits.desiredCppName).values.toSeq.sortBy(_.head.bits.getByteAlignWidth).reverse
  val aligned = sorted.flatten.map(_.bits.getByteAlign)
  val valids = sorted.flatten.map(_.valid)
  // Number of grainBytes of each bundle
  val n_grains = aligned.map(_.getWidth).map{w => (w / 8 + param.grainBytes - 1) / param.grainBytes }
  // Optional sum of grains according to prefix bundle, less choice than simply sum up
  // i.e. for idx = 2, opt grains is opt_grains(0-2), as 0, n_g(0), n_g(0+1),
  val opt_grains = Seq.tabulate(n_grains.length + 1){idx => n_grains.take(idx).sum}
  // Hint to split step_data. Because Assembler split data by group, grains_hint is a subset of opt_grains
  val step_grains_hint = Seq.tabulate(param.StepGroupSize + 1) { idx =>
    val bundleCnt = if (idx == 0) 0 else sorted.take(idx).flatten.length
    opt_grains(bundleCnt) // sum of first bundleCnt grains
  }
  val v_grains = valids.zip(n_grains).map{ case (v, n) => Mux(v, n.U, 0.U)}
  val v_grain_sums = VecInit.tabulate(v_grains.length){ idx => v_grains.take(idx + 1).reduce(_ +& _)}
  step_data := VecInit(valids.zip(aligned).zipWithIndex.map{ case ((v, gen), idx) =>
    val shifted = if (idx != 0) {
//      val offset_grain = v_grains.take(idx).reduce(_ +& _)
//      val opt_grains = Seq.tabulate(idx + 1){i => n_grains.take(i).sum}
      val offset_map = opt_grains.take(idx + 1).map { g => (g.U, (gen << (g * param.grainBytes * 8)).asUInt)}
      LookupTree(v_grain_sums(idx - 1), offset_map)
    } else gen
//    dontTouch(shifted)
//    shifted
    Mux(v, shifted, 0.U)
  }.toSeq).reduceTree(_ | _)

  val v_infos = sorted.map{ vgens => vgens.map(_.valid).reduce(_ | _)}
//  val v_info_cnt = PopCount(VecInit(v_infos).asUInt)
  val v_info_sums = VecInit.tabulate(v_infos.length){idx => PopCount(VecInit(v_infos.take(idx + 1)).asUInt)}
  val collect_info = VecInit(sorted.zipWithIndex.map{ case (vgens, idx) =>
    val info = Wire(new BatchInfo)
    info.id := Batch.getBundleID(vgens.head.bits).U
    info.num := PopCount(VecInit(vgens.map(_.valid).toSeq).asUInt)
    val shifted = if (idx != 0) {
//      val offset_info = PopCount(VecInit(v_infos.take(idx)).asUInt)
      val offset_map = Seq.tabulate(idx + 1){ g => (g.U, (info.asUInt << (g * param.infoWidth)).asUInt)}
      LookupTree(v_info_sums(idx - 1), offset_map)
    } else info.asUInt
    Mux(v_infos(idx), shifted, 0.U)
  }.toSeq).reduceTree(_ | _)

  // Use BatchInterval to update index of software buffer
  val BatchInterval = Wire(new BatchInfo)
  BatchInterval.id := Batch.getTemplate.length.U
  BatchInterval.num := v_info_sums.last // unused, only for debugging

//  val BatchFinish = Wire(new BatchInfo)
//  BatchFinish.id := (Batch.getTemplate.length + 1).U
//  BatchFinish.num := 1.U

  step_info := Cat(collect_info, BatchInterval.asUInt)
//  step_info := Cat((collect_info | (BatchFinish.asUInt << (v_info_sums.last * param.infoWidth.U)).asUInt), BatchInterval.asUInt)

  step_enable := v_info_sums.last =/= 0.U
  step_status.zipWithIndex.foreach{ case (status, idx) =>
    val bundle_cnt = sorted.take(idx + 1).flatten.length
    status.data_grains := v_grain_sums(bundle_cnt - 1)
    status.info_size := v_info_sums(idx) +& 1.U // BatchInterval at head
  }
//  dontTouch(step_status)
//  println(n_grains)
//  when(step_enable) {
//    printf("data %x\n", step_data)
//    sorted.zip(aligned).foreach{ case (s, gen) =>
//      val id = Batch.getBundleID(s.head.bits).U
//      printf("%d %x\n", id, gen)
//    }
//    printf("info %x\n", step_info)
//  }
}

class BatchAssembler(step_grains_hint: Seq[Int], param: BatchParam, config: GatewayConfig) extends Module {
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
  val data_grain_avail = param.MaxDataGrains.U -& state_status.data_grains
  // Always leave a space for BatchFinish, use MaxInfoSize - 1
  val info_size_avail = (param.MaxInfoSize - 1).U -& state_status.info_size
  val data_exceed_v = VecInit(delay_step_status.map(_.data_grains > data_grain_avail && delay_step_enable))
  // Note: BatchInterval already contains in step_stautus, we consider BatchFinish
  val info_exceed_v = VecInit(delay_step_status.map(_.info_size > info_size_avail && delay_step_enable))
  val exceed_v = VecInit(data_exceed_v.zip(info_exceed_v).map{ case (de, ie) => de | ie})
  // Extract last non-exceed stats
  // When no stats exceeds, return step_stats to append whole step for state flushing
  val concat_stats = Mux(!exceed_v.asUInt.orR, delay_step_status.last,
    VecInit(delay_step_status.dropRight(1).zipWithIndex.map { case (stats, idx) =>
      val mask = exceed_v(idx) ^ exceed_v(idx + 1)
      Mux(mask, stats.asUInt, 0.U)
    }).reduceTree(_ | _).asTypeOf(new BatchStats(param))
  )
  val remain_stats = Wire(new BatchStats(param))
  remain_stats.data_grains := delay_step_status.last.data_grains -& concat_stats.data_grains
  remain_stats.info_size := delay_step_status.last.info_size -& concat_stats.info_size
  assert(remain_stats.data_grains <= param.MaxDataGrains.U)
  assert(remain_stats.info_size + 1.U <= param.MaxInfoSize.U)

  val concat_data_map = step_grains_hint.map{ g =>
    val data_low = if (g == 0) 0.U else delay_step_data(g * param.grainBytes * 8 - 1, 0)
    (g.U, data_low)
  }
  // InfoSize range: 0 ~ StepGroupSize + 1(including BatchInterval)
  val concat_info_map = Seq.tabulate(param.StepGroupSize + 2){g =>
    val info_low = if (g == 0) 0.U else delay_step_info(g * param.infoWidth - 1, 0)
    (g.U, info_low)
  }
  val concat_data = LookupTree(concat_stats.data_grains, concat_data_map)
  val concat_info = LookupTree(concat_stats.info_size, concat_info_map)

  val remain_data_map = step_grains_hint.map{g =>
    val data_high = (delay_step_data >> (g * param.grainBytes * 8)).asUInt
    (g.U, data_high)
  }
  val remain_info_map = Seq.tabulate(param.StepGroupSize + 2){ g =>
    val info_high = (delay_step_info >> (g * param.infoWidth)).asUInt
    (g.U, info_high)
  }
  val remain_data = LookupTree(concat_stats.data_grains, remain_data_map)
  val remain_info = LookupTree(concat_stats.info_size, remain_info_map)

  // Stage 2:
  // update state
  val step_exceed = delay_step_enable && (state_step_cnt === config.batchSize.U)
  val cont_exceed = exceed_v.asUInt.orR
  val trace_exceed = Option.when(config.hasReplay){
    delay_step_enable && (state_trace_size.get +& delay_step_trace_info.get.trace_size >= config.replaySize.U)
  }
  val state_flush = step_status.last.data_grains >= param.MaxDataGrains.U // use Stage 1 grains to flush ahead
  val timeout_count = RegInit(0.U(32.W))
  val timeout = timeout_count === 200000.U
  if (config.hasBuiltInPerf) {
    DifftestPerf("BatchExceed_data", data_exceed_v.asUInt.orR)
    DifftestPerf("BatchExceed_info", info_exceed_v.asUInt.orR)
    DifftestPerf("BatchExceed_step", step_exceed.asUInt)
    DifftestPerf("BatchExceed_flush", state_flush.asUInt)
    DifftestPerf("BatchExceed_timeout", timeout.asUInt)
    if (config.hasReplay) DifftestPerf("BatchExceed_trace", trace_exceed.get.asUInt)
  }
  val in_replay = Option.when(config.hasReplay)(step_trace_info.get.in_replay)

  val should_tick = timeout || cont_exceed || step_exceed ||
    trace_exceed.getOrElse(false.B) || in_replay.getOrElse(false.B)
  when(!should_tick) {
    timeout_count := timeout_count + 1.U
  }.otherwise {
    timeout_count := 0.U
  }
  // Delay step can be partly appended to output for making full use of transmission param
  // Avoid appending when step equals batchSize(delay_step_exceed), last appended data will overwrite first step data
  val has_append =
  delay_step_enable && (state_flush || cont_exceed) && RegNext(!exceed_v.asUInt.andR) && !step_exceed
  // When the whole step is appended to output, state_step should be 0, and output step + 1
  val append_whole = has_append && !cont_exceed
  val finish_step = state_step_cnt + Mux(append_whole, 1.U, 0.U)
  val BatchFinish = Wire(new BatchInfo)
  BatchFinish.id := (Batch.getTemplate.length + 1).U
  BatchFinish.num := finish_step
  val append_finish_map = Seq.tabulate(param.StepGroupSize + 1) { g =>
    (g.U, (BatchFinish.asUInt << (g * param.infoWidth)).asUInt)
  }
  val append_info = Mux(has_append,
    concat_info | LookupTree(concat_stats.info_size, append_finish_map),
    BatchFinish.asUInt
  )
  val append_data_map = Seq.tabulate(param.MaxDataGrains + 1){g =>
    (g.U, (concat_data << (g * param.grainBytes * 8)).asUInt)
  }
  val append_info_map = Seq.tabulate(param.MaxInfoSize + 1) {g =>
    (g.U, (append_info << (g * param.infoWidth)).asUInt)
  }
  out.io.data := state_data |
    Mux(has_append, LookupTree(state_status.data_grains, append_data_map), 0.U)
  out.io.info := state_info |
    LookupTree(state_status.info_size, append_info_map)
  out.enable := should_tick
  out.step := Mux(out.enable, finish_step, 0.U)

  val next_state_data_grains = Mux(
    delay_step_enable,
    Mux(
      should_tick,
      Mux(has_append, remain_stats.data_grains, delay_step_status.last.data_grains),
      state_status.data_grains + delay_step_status.last.data_grains,
    ),
    0.U,
  )
  val next_state_info_size = Mux(
    delay_step_enable,
    Mux(
      should_tick,
      Mux(has_append, remain_stats.info_size, delay_step_status.last.info_size),
      state_status.info_size + delay_step_status.last.info_size,
    ),
    0.U,
  )
  val state_update = delay_step_enable || state_flush || timeout
  when(state_update) {
    state_status.data_grains := next_state_data_grains
    state_status.info_size := next_state_info_size
    when(delay_step_enable) {
      when(should_tick) {
        when(has_append) { // include state_flush with new-coming step
          state_step_cnt := Mux(append_whole, 0.U, 1.U)
          state_data := remain_data
          state_info := remain_info
        }.otherwise {
          state_step_cnt := 1.U
          state_data := delay_step_data
          state_info := delay_step_info
        }
        if (config.hasReplay) state_trace_size.get := delay_step_trace_info.get.trace_size
      }.otherwise {
        state_step_cnt := state_step_cnt + 1.U
        val step_data_map = Seq.tabulate(param.MaxDataGrains + 1){g =>
          (g.U, (delay_step_data << (g * param.grainBytes * 8)).asUInt)
        }
        val step_info_map = Seq.tabulate(param.MaxInfoSize + 1){ g =>
          (g.U, (delay_step_info << (g * param.infoWidth)).asUInt)
        }
        state_data := state_data | LookupTree(state_status.data_grains, step_data_map)
        state_info := state_info | LookupTree(state_status.info_size, step_info_map)
        if (config.hasReplay) state_trace_size.get := state_trace_size.get + delay_step_trace_info.get.trace_size
      }
    }.otherwise { // state_flush without new-coming step
      state_step_cnt := 0.U
      state_data := 0.U
      state_info := 0.U
      if (config.hasReplay) state_trace_size.get := 0.U
    }
  }
}
