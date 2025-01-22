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
  val step_data_head = collector.step_data_head
  val step_data_tail = collector.step_data_tail
  val step_info_head = collector.step_info_head
  val step_info_tail = collector.step_info_tail
  val step_data = collector.step_data
  val step_info = collector.step_info
  val step_enable = collector.step_enable
  val step_status = collector.step_status
//  val step_grains_hint = collector.step_grains_hint


//  val out = IO(Output(new BatchOutput(param, config)))
//  out.io.data := step_data
//  out.io.info := step_info
//  out.step := Mux(step_enable, 1.U, 0.U)
//  out.enable := step_enable

  // Assemble collected data from different cycles
  val assembler = Module(new BatchAssembler(chiselTypeOf(step_data_head), chiselTypeOf(step_data_tail), param, config))
  assembler.step_data := step_data
  assembler.step_info := step_info
  assembler.step_data_head := step_data_head
  assembler.step_data_tail := step_data_tail
  assembler.step_info_head := step_info_head
  assembler.step_info_tail := step_info_tail
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
  // vec of step_data split in different loc
  val step_status = IO(Output(Vec(param.StepGroupSize, new BatchStats(param))))
  val step_enable = IO(Output(Bool()))

  val sorted = in.groupBy(_.bits.desiredCppName).values.toSeq.sortBy(_.head.bits.getByteAlignWidth).reverse
  // Stage 1：concat bundles with same desiredCppName
  val group_grain = sorted.map{ v_gens =>
    (v_gens.head.bits.getByteAlignWidth / 8 + param.grainBytes - 1 ) / param.grainBytes
  }
  val group_length = sorted.map(_.length)
  val group_info = Wire(Vec(param.StepGroupSize, UInt(param.infoWidth.W)))
  val group_status = Wire(Vec(param.StepGroupSize, new BatchStats(param)))
  val group_data = Wire(MixedVec(Seq.tabulate(param.StepGroupSize){idx =>
    UInt((group_length(idx) * group_grain(idx) * param.grainBytes * 8).W)}
  ))
  val group_vsize = Wire(Vec(param.StepGroupSize, UInt(8.W)))
//  val grains_set = collection.mutable.Set(0)
//  // Hint to shift group data, sum of grains of a random subset of bundles
//  val group_grains_hint = ListBuffer.empty[Seq[Int]]
  sorted.zipWithIndex.foreach{ case (v_gens, idx) =>
    val aligned = v_gens.map(_.bits.getByteAlign)
    val valids = v_gens.map(_.valid)
    val v_size = PopCount(VecInit(valids).asUInt)
    group_vsize(idx) := v_size
    val info = Wire(new BatchInfo)
    info.id := Batch.getBundleID(v_gens.head.bits).U
    info.num := v_size
    group_info(idx) := Mux(v_size =/= 0.U, info.asUInt, 0.U)
    val data_grains = {
      val offset_map = Seq.tabulate(valids.length + 1){vi => (vi.U, (group_grain(idx) * vi).U)}
      LookupTree(v_size, offset_map)
    }
    val info_size = Mux(v_size =/= 0.U, 1.U, 0.U)
    if (idx == 0) {
      group_status(idx).data_grains := data_grains
      group_status(idx).info_size := info_size +& 1.U // BatchInterval at head
    } else {
      group_status(idx).data_grains := group_status(idx - 1).data_grains +& data_grains
      group_status(idx).info_size := group_status(idx - 1).info_size +& info_size
    }
    group_data(idx) := MixedVecInit(aligned.zip(valids).zipWithIndex.map{ case ((gen, v), i) =>
      val shifted = if (i != 0) {
        // Count prefix valids size, e.g. for i = 2, opt valids size is 0/1/2
        val offset_map = Seq.tabulate(i + 1){g => (g.U, (gen << (g * group_grain(idx) * param.grainBytes * 8)).asUInt)}
        LookupTree(PopCount(VecInit(valids.take(i)).asUInt), offset_map)
      } else {
        gen
      }
      Mux(v, shifted, 0.U)
    }.toSeq).reduce(_ | _)
//    val base_grains = grains_set.toSeq
//    base_grains.foreach { base =>
//      (1 to v_gens.length).foreach{ n =>
//        grains_set.add(base + n * group_grain(idx))
//      }
//    }
//    group_grains_hint.addOne(grains_set.toSeq)
  }
//  val step_grains_hint = group_grains_hint.last

  // Stage 2: delay grouped data, concat different group
  val delay_group_data = RegNext(group_data)
  val delay_group_info = RegNext(group_info)
  val delay_group_status = RegNext(group_status)
  val delay_group_vsize = RegNext(group_vsize)
  // Use BatchInterval to update index of software buffer
  val info_num = delay_group_status.last.info_size
  val BatchInterval = Wire(new BatchInfo)
  BatchInterval.id := Batch.getTemplate.length.U
  BatchInterval.num := info_num // unused, only for debugging
  // head(i) contains group 0~i, tail(i) contains group i+1~groupsize-1
  val step_data = IO(Output(UInt(param.StepDataBitLen.W)))
  val step_info = IO(Output(UInt(param.StepInfoBitLen.W)))
  val step_data_head_w = Seq.tabulate(param.StepGroupSize - 1){idx => delay_group_data.take(idx + 1).map(_.getWidth).sum}
  val step_data_head = IO(Output(MixedVec(step_data_head_w.map{hw => UInt(hw.W)})))
  val step_data_tail = IO(Output(MixedVec(step_data_head_w.map{hw => UInt((param.StepDataBitLen - hw).W)})))
  val step_info_head_w = Seq.tabulate(param.StepGroupSize - 1){idx => (idx + 2) * param.infoWidth} // include Interval
  val step_info_head = IO(Output(MixedVec(step_info_head_w.map{hw => UInt(hw.W)})))
  val collect_info_head = Wire(MixedVec(step_info_head_w.map{hw => UInt((hw - param.infoWidth).W)}))
  val step_info_tail = IO(Output(MixedVec(step_info_head_w.map{hw => UInt((param.StepInfoBitLen - hw).W)})))
  // Collect from head
  step_data_head(0) := delay_group_data(0)
  collect_info_head(0) := delay_group_info(0)
  step_info_head(0) := Cat(delay_group_info(0), BatchInterval.asUInt)
  (1 until param.StepGroupSize).foreach{idx =>
    val cat_map = Seq.tabulate(group_length(idx) + 1){ len =>
      (len.U, Cat(step_data_head(idx - 1),
        delay_group_data(idx)(len * group_grain(idx) * param.grainBytes * 8 - 1, 0)))
    }
    val collect_data = LookupTree(delay_group_vsize(idx), cat_map)
    val collect_info = Mux(delay_group_vsize(idx) =/= 0.U,
      Cat(collect_info_head(idx - 1), delay_group_info(idx)),
      collect_info_head(idx - 1))
    if (idx != param.StepGroupSize - 1) {
      step_data_head(idx) := collect_data
      collect_info_head(idx) := collect_info
      step_info_head(idx) := Cat(collect_info, BatchInterval.asUInt)
    } else {
      step_data := collect_data
      step_info := Cat(collect_info, BatchInterval.asUInt)
    }
  }
  // Collect from tail
  step_data_tail(param.StepGroupSize - 2) := delay_group_data(param.StepGroupSize - 1)
  step_info_tail(param.StepGroupSize - 2) := delay_group_info(param.StepGroupSize - 1)
  (1 until param.StepGroupSize - 1).foreach{ idx =>
    val cat_map = Seq.tabulate(group_length(idx) + 1){ len =>
      (len.U, Cat(step_data_tail(idx), delay_group_data(idx)(len * group_grain(idx) * param.grainBytes * 8 - 1, 0)))
    }
    step_data_tail(idx - 1) := LookupTree(delay_group_vsize(idx), cat_map)
    step_info_tail(idx - 1) :=
      Mux(delay_group_vsize(idx) =/= 0.U,
        Cat(step_info_tail(idx), delay_group_info(idx)),
        step_info_tail(idx))
  }
//  (0 until param.StepGroupSize - 1).map{ idx =>
//    when(step_enable){
//      printf("%x\n",step_info_head(idx))
//      printf("%x\n",step_info_tail(idx))
//    }
//  }
  step_enable := info_num =/= 1.U // BatchInterval, at least 1.U
  step_status := delay_group_status
//  delay_group_info.zip(delay_group_status).foreach{ case(info, status) =>
//    when(step_enable) {
//      printf("%x %x\n", info, status.info_size)
//    }
//  }

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

class BatchAssembler(
                      step_data_head_t: MixedVec[UInt], step_data_tail_t: MixedVec[UInt],
//                      step_grains_hint: Seq[Int],
          param: BatchParam, config: GatewayConfig) extends Module {
  val step_data = IO(Input(UInt(param.StepDataBitLen.W)))
  val step_info = IO(Input(UInt(param.StepInfoBitLen.W)))
  val step_data_head = IO(Input(step_data_head_t))
  val step_data_tail = IO(Input(step_data_tail_t))
  val step_info_head_w = Seq.tabulate(param.StepGroupSize - 1) { idx => (idx + 2) * param.infoWidth } // include Interval
  val step_info_head = IO(Input(MixedVec(step_info_head_w.map { hw => UInt(hw.W) })))
  val step_info_tail = IO(Input(MixedVec(step_info_head_w.map { hw => UInt((param.StepInfoBitLen - hw).W) })))
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
  val delay_step_data_head = RegNext(step_data_head)
  val delay_step_data_tail = RegNext(step_data_tail)
  val delay_step_info_head = RegNext(step_info_head)
  val delay_step_info_tail = RegNext(step_info_tail)
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
  val concat_mask = VecInit.tabulate(param.StepGroupSize - 1){ idx => exceed_v(idx) ^ exceed_v(idx + 1)}
  val none_exceed = !exceed_v.asUInt.orR
  val concat_stats = Mux(none_exceed, delay_step_status.last,
    VecInit(delay_step_status.dropRight(1).zip(concat_mask).map{ case (stats, mask) =>
      Mux(mask, stats.asUInt, 0.U)
    }).reduce(_ | _).asTypeOf(new BatchStats(param))
  )
  val remain_stats = Wire(new BatchStats(param))
  remain_stats.data_grains := delay_step_status.last.data_grains -& concat_stats.data_grains
  remain_stats.info_size := delay_step_status.last.info_size -& concat_stats.info_size
  assert(remain_stats.data_grains <= param.MaxDataGrains.U)
  assert(remain_stats.info_size + 1.U <= param.MaxInfoSize.U)

//  val concat_data_map = step_grains_hint.map{ g =>
//    val data_low = if (g == 0) 0.U else delay_step_data(g * param.grainBytes * 8 - 1, 0)
//    (g.U, data_low)
//  }
//  // InfoSize range: 0 ~ StepGroupSize + 1(including BatchInterval)
//  val concat_info_map = Seq.tabulate(param.StepGroupSize + 2){g =>
//    val info_low = if (g == 0) 0.U else delay_step_info(g * param.infoWidth - 1, 0)
//    (g.U, info_low)
//  }
//  val concat_data = LookupTree(concat_stats.data_grains, concat_data_map)
//  val concat_info = LookupTree(concat_stats.info_size, concat_info_map)
  val concat_data = Mux(none_exceed, delay_step_data,
    VecInit(delay_step_data_head.zip(concat_mask).map{ case (gen, mask) => Mux(mask, gen, 0.U)}).reduceTree(_ | _))
  val concat_info = Mux(none_exceed, delay_step_info,
    VecInit(delay_step_info_head.zip(concat_mask).map{ case (info, mask) => Mux(mask, info, 0.U)}).reduceTree(_ | _))
  val remain_data = VecInit(delay_step_data_tail.zip(concat_mask).map{ case (gen, mask) => Mux(mask, gen, 0.U)}).reduceTree(_ | _)
  val remain_info = VecInit(delay_step_info_tail.zip(concat_mask).map{ case (info, mask) => Mux(mask, info, 0.U)}).reduceTree(_ | _)
  //  val remain_data_map = step_grains_hint.map{g =>
//    val data_high = (delay_step_data >> (g * param.grainBytes * 8)).asUInt
//    (g.U, data_high)
//  }
//  val remain_info_map = Seq.tabulate(param.StepGroupSize + 2){ g =>
//    val info_high = (delay_step_info >> (g * param.infoWidth)).asUInt
//    (g.U, info_high)
//  }
//  val remain_data = LookupTree(concat_stats.data_grains, remain_data_map)
//  val remain_info = LookupTree(concat_stats.info_size, remain_info_map)

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
  delay_step_enable && (state_flush || cont_exceed) && !exceed_v.asUInt.andR && !step_exceed
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
//  when(state_update){
//    printf("info %d\n%x\n%x\n%x\n", state_status.info_size, state_info, concat_info, remain_info)
//    printf("%x %x %b\n", has_append, append_whole, exceed_v.asUInt)
//  }
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
