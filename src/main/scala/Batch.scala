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
import difftest.util.Delayer

import scala.collection.mutable.ListBuffer

case class BatchParam(config: GatewayConfig, collectDataWidth: Int, collectLength: Int) {
  def infoWidth = (new BatchInfo).getWidth

  // Maximum Byte length decided by transmission function
  def MaxDataByteLen = config.batchArgByteLen._1
  def MaxDataBitLen = MaxDataByteLen * 8
  def MaxDataLenWidth = log2Ceil(MaxDataByteLen)
  def MaxInfoByteLen = config.batchArgByteLen._2
  def MaxInfoBitLen = MaxInfoByteLen * 8
  def MaxInfoLenWidth = log2Ceil(MaxInfoByteLen)

  // Width of statistic for data/info byte length
  def StatsDataLenWidth = log2Ceil((collectDataWidth + 7) / 8)
  def collectInfoWidth = collectLength * infoWidth
  def StatsInfoLenWidth = log2Ceil((collectInfoWidth + 7) / 8)

  // Truncate width when shifting to reduce useless gates
  def TruncDataBitLen = math.min(MaxDataBitLen, collectDataWidth)
  def TruncInfoBitLen = math.min(MaxInfoBitLen, collectInfoWidth)
  def TruncDataLenWidth = math.min(MaxDataLenWidth, StatsDataLenWidth)
  def TruncInfoLenWidth = math.min(MaxInfoLenWidth, StatsInfoLenWidth)
}

class BatchIO(dataType: UInt, infoType: UInt) extends Bundle {
  val data = dataType
  val info = infoType
}

class BatchStats(param: BatchParam) extends Bundle {
  val data_len = UInt(param.StatsDataLenWidth.W)
  val info_len = UInt(param.StatsInfoLenWidth.W)
}

class BatchOutput(dataType: UInt, infoType: UInt, config: GatewayConfig) extends Bundle {
  val io = new BatchIO(dataType, infoType)
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
  def vecAlignWidth = (vec: Seq[Valid[DifftestBundle]]) => vec.head.bits.getByteAlign.getWidth * vec.length

  // Collect bundles with valid of same cycle in Pipeline
  val global_enable = VecInit(in.map(_.valid).toSeq).asUInt.orR
  val inCollect =
    in.groupBy(_.bits.desiredCppName).values.toSeq.map(_.toSeq).sortBy(vecAlignWidth).reverse
  val inCollect_w = inCollect.map(vecAlignWidth)
  val param = BatchParam(config, inCollect_w.sum, inCollect.length)
  val dataCollect_vec = WireInit(
    0.U.asTypeOf(
      MixedVec(
        Seq.tabulate(inCollect.length)(i => UInt(inCollect_w.take(i + 1).sum.W))
      )
    )
  )
  val infoCollect_vec = WireInit(
    0.U.asTypeOf(
      MixedVec(
        Seq.tabulate(inCollect.length)(i => UInt(((i + 1) * param.infoWidth).W))
      )
    )
  )
  val statsCollect_vec = WireInit(0.U.asTypeOf(Vec(inCollect.length, new BatchStats(param))))
  inCollect.zipWithIndex.foreach { case (in, idx) =>
    val (dataBaseW, infoBaseW) = if (idx != 0) {
      (dataCollect_vec(idx - 1).getWidth, infoCollect_vec(idx - 1).getWidth)
    } else {
      (0, 0)
    }
    val collector = Module(
      new BatchCollector(
        chiselTypeOf(in.head),
        in.length,
        dataBaseW,
        infoBaseW,
        param,
        idx,
      )
    )
    collector.data_in := in
    collector.enable := global_enable
    if (idx != 0) {
      collector.data_base := dataCollect_vec(idx - 1)
      collector.info_base := infoCollect_vec(idx - 1)
      collector.stats_base := statsCollect_vec(idx - 1)
    } else {
      collector.data_base := 0.U
      collector.info_base := 0.U
      collector.stats_base := 0.U.asTypeOf(new BatchStats(param))
    }
    dataCollect_vec(idx) := collector.data_out
    infoCollect_vec(idx) := collector.info_out
    statsCollect_vec(idx) := collector.stats_out
  }

  val BatchInterval = WireInit(0.U.asTypeOf(new BatchInfo))
  BatchInterval.id := Batch.getTemplate.length.U
  val step_data = dataCollect_vec.last
  val step_info = infoCollect_vec.last
  val step_stats_vec = statsCollect_vec.zipWithIndex.map { case (stats, idx) =>
    Delayer(stats, inCollect.length - idx - 1)
  }

  // Assemble collected data from different cycles
  val assembler = Module(new BatchAssembler(step_data.getWidth, step_info.getWidth, inCollect.length, param, config))
  assembler.step_data := step_data
  assembler.step_info := step_info
  assembler.step_stats_vec := step_stats_vec

  assembler.enable := Delayer(global_enable, inCollect.length)
  if (config.hasReplay) {
    val trace_info = in.map(_.bits).filter(_.desiredCppName == "trace_info").head.asInstanceOf[DiffTraceInfo]
    assembler.step_trace_info.get := Delayer(trace_info, inCollect.length)
  }

  val assembled = WireInit(assembler.out)
  val out = IO(Output(chiselTypeOf(assembled)))
  out := assembled
}

// Collect Bundles with Valid by pipeline, same Class will be processed in parallel
class BatchCollector(
  bundleType: Valid[DifftestBundle],
  length: Int,
  dataBase_w: Int,
  infoBase_w: Int,
  param: BatchParam,
  delay: Int,
) extends Module {
  val alignWidth = bundleType.bits.getByteAlignWidth
  val dataOut_w = dataBase_w + alignWidth * length
  val infoOut_w = infoBase_w + param.infoWidth

  val data_in = IO(Input(Vec(length, bundleType)))
  val enable = IO(Input(Bool()))

  val data_base = IO(Input(UInt(dataBase_w.W)))
  val info_base = IO(Input(UInt(infoBase_w.W)))
  val stats_base = IO(Input(new BatchStats(param)))

  val data_out = IO(Output(UInt(dataOut_w.W)))
  val info_out = IO(Output(UInt(infoOut_w.W)))
  val stats_out = IO(Output(new BatchStats(param)))

  val data_state = RegInit(0.U(dataOut_w.W))
  val info_state = RegInit(0.U(infoOut_w.W))
  val stats_state = RegInit(0.U.asTypeOf(new BatchStats(param)))

  val align_data = VecInit(data_in.map(i => i.bits.getByteAlign).toSeq)
  val valid_vec = VecInit(data_in.map(i => i.valid && enable))
  val delay_data = Delayer(align_data.asUInt, delay, useMem = true).asTypeOf(align_data)
  val delay_valid = Delayer(valid_vec.asUInt, delay, useMem = true).asTypeOf(valid_vec)

  val valid_num = PopCount(delay_valid)
  val info = Wire(new BatchInfo)
  info.id := Batch.getBundleID(bundleType.bits).U
  info.num := valid_num

  val offset_map = (0 to length).map(i => i.U -> (i * alignWidth).U)
  val dataLen_map = (0 to length).map(i => i.U -> (i * alignWidth / 8).U)

  val data_site = WireInit(0.U((alignWidth * length).W))
  data_site := VecInit(delay_data.zipWithIndex.map { case (d, idx) =>
    val offset = if (idx == 0) 0.U else MuxLookup(PopCount(delay_valid.take(idx)), 0.U)(offset_map)
    Mux(delay_valid(idx), (d << offset).asUInt, 0.U)
  }.toSeq).reduce(_ | _)

  when(delay_valid.asUInt.orR) {
    data_state := (data_base << MuxLookup(valid_num, 0.U)(offset_map)).asUInt | data_site
    info_state := Cat(info_base, info.asUInt)
    stats_state.data_len := stats_base.data_len + MuxLookup(valid_num, 0.U)(dataLen_map)
    stats_state.info_len := stats_base.info_len + (param.infoWidth / 8).U
  }.otherwise {
    data_state := data_base
    info_state := info_base
    stats_state := stats_base
  }

  data_out := data_state
  info_out := info_state
  stats_out := stats_state
}

class BatchAssembler(
  step_data_w: Int,
  step_info_w: Int,
  collect_length: Int,
  param: BatchParam,
  config: GatewayConfig,
) extends Module {
  val enable = IO(Input(Bool()))
  val step_data = IO(Input(UInt(step_data_w.W)))
  val step_info = IO(Input(UInt(step_info_w.W)))
  val step_stats_vec = IO(Input(Vec(collect_length, new BatchStats(param))))
  val step_trace_info = Option.when(config.hasReplay)(IO(Input(new DiffTraceInfo(config))))

  val state_data = RegInit(0.U(param.MaxDataBitLen.W))
  val state_info = RegInit(0.U(param.MaxInfoBitLen.W))
  val state_data_len = RegInit(0.U(param.MaxDataLenWidth.W))
  val state_info_len = RegInit(0.U(param.MaxInfoLenWidth.W))
  val state_step_cnt = RegInit(0.U(config.stepWidth.W))
  val state_trace_size = Option.when(config.hasReplay)(RegInit(0.U(16.W)))

  // Interval update index of buffer, Finish end data parse and call difftest comparision if enabled
  val BatchInterval = Wire(new BatchInfo)
  BatchInterval.id := Batch.getTemplate.length.U
  BatchInterval.num := 0.U // unused

  // Assemble step data/info into state in 3 stage
  // Stage 1:
  //   1. occupy_stats: get statistic of occupied space
  //   2. data/info_exceed_vec: mark whether different length fragments of step data/info exceed available space
  //   3. concat/remain_stats: record statistic for data/info to be concatenated to output or remained to state
  // Calculate data/info space occupied when enable, assigned in the following code
  val occupy_data_len = Wire(UInt(param.MaxDataLenWidth.W))
  val occupy_info_len = Wire(UInt(param.MaxInfoLenWidth.W))
  // Calculate available space for step data/info
  val data_limit = param.MaxDataByteLen.U -& occupy_data_len
  val info_limit = param.MaxInfoByteLen.U -& occupy_info_len
  val data_exceed_vec = VecInit(step_stats_vec.map(_.data_len > data_limit && enable))
  // Note: state_info contains Interval and Finish
  val info_exceed_vec = VecInit(step_stats_vec.map(_.info_len + (2 * param.infoWidth / 8).U > info_limit && enable))
  val exceed_vec = VecInit(data_exceed_vec.zip(info_exceed_vec).map { case (de, ie) => de | ie })
  // Extract last non-exceed stats
  // When no stats exceeds, return step_stats to append whole step for state flushing
  val concat_stats = Mux(
    !exceed_vec.asUInt.orR,
    step_stats_vec.last,
    VecInit(step_stats_vec.dropRight(1).zipWithIndex.map { case (stats, idx) =>
      val mask = exceed_vec(idx) ^ exceed_vec(idx + 1)
      Mux(mask, stats.asUInt, 0.U)
    }).reduceTree(_ | _).asTypeOf(new BatchStats(param)),
  )
  val remain_stats = WireInit(0.U.asTypeOf(new BatchStats(param)))
  remain_stats.data_len := step_stats_vec.last.data_len -& concat_stats.data_len
  remain_stats.info_len := step_stats_vec.last.info_len -& concat_stats.info_len
  assert(remain_stats.data_len <= param.MaxDataByteLen.U)
  assert(remain_stats.info_len + (2 * param.infoWidth / 8).U <= param.MaxInfoByteLen.U)

  // Stage 2:
  //   1. delay*: RegNext signal calculated in stage 1 to shorten logic length
  //   2. delay_concat/remain_data/info: split step into parts of concatenated and remained
  //   3. should_tick: mark output valid
  //   4. out: append concatenated data/info to output if any
  val delay_concat_stats = RegNext(concat_stats)
  val delay_remain_stats = RegNext(remain_stats)
  val delay_step_data = RegNext(step_data)
  val delay_step_info = RegNext(step_info)
  val delay_step_stats = RegNext(step_stats_vec.last)
  val delay_concat_data = delay_step_data >> (delay_remain_stats.data_len << 3)
  val delay_concat_info = delay_step_info >> (delay_remain_stats.info_len << 3)
  // Note we need only lowest bits to update state, truncate high bits to reduce gates
  val delay_remain_data = (~(~0.U(param.TruncDataBitLen.W) <<
    (delay_remain_stats.data_len(param.TruncDataLenWidth - 1, 0) << 3).asUInt)).asUInt & delay_step_data
  val delay_remain_info = (~(~0.U(param.TruncInfoBitLen.W) <<
    (delay_remain_stats.info_len(param.TruncInfoLenWidth - 1, 0) << 3).asUInt)).asUInt & delay_step_info

  val delay_enable = RegNext(enable)
  val delay_step_exceed = delay_enable && (state_step_cnt === config.batchSize.U)
  val delay_cont_exceed = RegNext(exceed_vec.asUInt.orR)
  val delay_trace_exceed = Option.when(config.hasReplay) {
    delay_enable && (state_trace_size.get +& RegNext(
      step_trace_info.get.trace_size
    ) +& collect_length.U >= config.replaySize.U)
  }
  // Flush state to provide more space for peak data
  val state_flush = enable && step_stats_vec.last.data_len > param.MaxDataByteLen.U
  val timeout_count = RegInit(0.U(32.W))
  val timeout = timeout_count === 200000.U
  if (config.hasBuiltInPerf) {
    DifftestPerf("BatchExceed_data", data_exceed_vec.asUInt.orR)
    DifftestPerf("BatchExceed_info", info_exceed_vec.asUInt.orR)
    DifftestPerf("BatchExceed_step", delay_step_exceed.asUInt)
    DifftestPerf("BatchExceed_flush", state_flush.asUInt)
    DifftestPerf("BatchExceed_timeout", timeout.asUInt)
    if (config.hasReplay) DifftestPerf("BatchExceed_trace", delay_trace_exceed.get.asUInt)
  }
  val in_replay = Option.when(config.hasReplay)(step_trace_info.get.in_replay)

  val should_tick = timeout || state_flush || delay_cont_exceed || delay_step_exceed ||
    delay_trace_exceed.getOrElse(false.B) || in_replay.getOrElse(false.B)
  when(!should_tick) {
    timeout_count := timeout_count + 1.U
  }.otherwise {
    timeout_count := 0.U
  }
  // Delay step can be partly appended to output for making full use of transmission param
  // Avoid appending when step equals batchSize(delay_step_exceed), last appended data will overwrite first step data
  val has_append =
    delay_enable && (state_flush || delay_cont_exceed) && RegNext(!exceed_vec.asUInt.andR) && !delay_step_exceed
  // When the whole step is appended to output, state_step should be 0, and output step + 1
  val append_whole = has_append && !delay_cont_exceed
  val out = IO(Output(new BatchOutput(chiselTypeOf(state_data), chiselTypeOf(state_info), config)))
  out.io.data := state_data | Mux(
    has_append,
    delay_concat_data(param.TruncDataBitLen - 1, 0) << (state_data_len << 3),
    0.U,
  )
  val finish_step = state_step_cnt + Mux(append_whole, 1.U, 0.U)
  val BatchFinish = Wire(new BatchInfo)
  BatchFinish.id := (Batch.getTemplate.length + 1).U
  BatchFinish.num := finish_step
  val append_info = Mux(
    has_append,
    Cat(
      delay_concat_info,
      BatchInterval.asUInt,
    ) | BatchFinish.asUInt <<
      ((delay_concat_stats.info_len + (param.infoWidth / 8).U)(param.TruncInfoLenWidth - 1, 0) << 3),
    BatchFinish.asUInt,
  )
  out.io.info := state_info | append_info(param.TruncInfoBitLen - 1, 0) << (state_info_len << 3)
  out.enable := should_tick
  out.step := Mux(out.enable, finish_step, 0.U)

  // Stage 3: update state
  val next_state_data_len = Mux(
    delay_enable,
    Mux(
      should_tick,
      Mux(has_append, delay_remain_stats.data_len, delay_step_stats.data_len),
      state_data_len + delay_step_stats.data_len,
    ),
    0.U,
  )

  val next_state_info_len = Mux(
    delay_enable,
    Mux(
      should_tick,
      Mux(has_append, delay_remain_stats.info_len, delay_step_stats.info_len + (param.infoWidth / 8).U),
      state_info_len + delay_step_stats.info_len + (param.infoWidth / 8).U,
    ),
    0.U,
  )

  // Calculate occupied space when enable(stage 1), state_update means previous step is in stage 2, use next_state_stats ahead
  val state_update = delay_enable || state_flush || timeout
  occupy_data_len := Mux(state_update, next_state_data_len, state_data_len)
  occupy_info_len := Mux(state_update, next_state_info_len, state_info_len)
  when(state_update) {
    state_data_len := next_state_data_len
    state_info_len := next_state_info_len
    when(delay_enable) {
      when(should_tick) {
        when(has_append) { // include state_flush with new-coming step
          state_step_cnt := Mux(append_whole, 0.U, 1.U)
          state_data := delay_remain_data
          state_info := delay_remain_info
        }.otherwise {
          state_step_cnt := 1.U
          state_data := delay_step_data
          state_info := Cat(delay_step_info, BatchInterval.asUInt)
        }
        if (config.hasReplay) state_trace_size.get := RegNext(step_trace_info.get.trace_size)
      }.otherwise {
        state_step_cnt := state_step_cnt + 1.U
        state_data := state_data | delay_step_data(param.TruncDataBitLen - 1, 0) << (state_data_len << 3)
        state_info := state_info | Cat(
          delay_step_info,
          BatchInterval.asUInt,
        )(param.TruncInfoBitLen - 1, 0) << (state_info_len << 3)
        if (config.hasReplay) state_trace_size.get := state_trace_size.get + RegNext(step_trace_info.get.trace_size)
      }
    }.otherwise { // state_flush without new-coming step
      state_step_cnt := 0.U
      state_data := 0.U
      state_info := 0.U
      if (config.hasReplay) state_trace_size.get := 0.U
    }
  }
}
