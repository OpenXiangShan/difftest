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
import difftest.util.{LookupTree, PipelineConnect, SkidBufferConnect}

import scala.collection.mutable.ListBuffer

// Only instantiated once, use val instead of def
case class BatchParam(config: GatewayConfig, bundles: Seq[DifftestBundle]) {
  val infoWidth = (new BatchInfo).getWidth
  val infoByte = infoWidth / 8 // byteAligned by default
  val ChunkByteLen = config.batchChunkBytes
  val ChunkBitLen = ChunkByteLen * 8
  val BeatChunkSize = config.batchBeatChunks
  val BeatByteLen = BeatChunkSize * ChunkByteLen
  val BeatBitLen = BeatByteLen * 8

  require(BeatChunkSize > 0, s"Batch beat chunk size $BeatChunkSize must be greater than 0")
  require(isPow2(BeatChunkSize), s"Batch beat chunk size $BeatChunkSize must be a power of 2")

  def alignChunkBytes(bytes: Int): Int = ((bytes + ChunkByteLen - 1) / ChunkByteLen) * ChunkByteLen

  val ClusterGroups = bundles.groupBy(_.desiredCppName).values.toSeq
  val StepGroupSize = ClusterGroups.size
  val ClusterDataByteLen = ClusterGroups
    .map(group => alignChunkBytes(group.length * group.head.getByteAlignWidth / 8))
    .toSeq
  val StepDataByteLen = ClusterDataByteLen.sum
  // Include BatchHead and BatchStep.
  val StepInfoSize = StepGroupSize + 2
  val StepInfoRawByteLen = StepInfoSize * infoByte
  val StepInfoByteLen = alignChunkBytes(StepInfoRawByteLen)
  val StepInfoBitLen = StepInfoByteLen * 8
  val StepInfoChunks = StepInfoByteLen / ChunkByteLen

  // Width of statistic for data chunk length
  val StepDataChunks = StepDataByteLen / ChunkByteLen
  val StatsDataWidth = math.max(1, log2Ceil(StepDataChunks + 1))
  val StatsInfoWidth = math.max(1, log2Ceil(StepInfoSize + 1))
}

class BatchStats(param: BatchParam) extends Bundle {
  val data_chunks = UInt(param.StatsDataWidth.W)
  val info_cnt = UInt(param.StatsInfoWidth.W)
  val cluster_data_chunks = UInt(param.StatsDataWidth.W)
  val cluster_info_cnt = UInt(param.StatsInfoWidth.W)
}

class BatchIO(val param: BatchParam, config: GatewayConfig) extends Bundle {
  val payload = UInt(param.BeatBitLen.W)
  val step = UInt(config.stepWidth.W)
}

class BatchInfo extends Bundle {
  val id = UInt(8.W)
  val num = UInt(8.W)
}

object Batch {
  private val template = ListBuffer.empty[DifftestBundle]

  def apply(bundles: DecoupledIO[MixedVec[Valid[DifftestBundle]]], config: GatewayConfig): DecoupledIO[BatchIO] = {
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
  PipelineConnect(in, collector.in, collector.in.fire)

  val out = IO(Decoupled(new BatchIO(param, config)))
  out <> collector.out
}

// Cluster Data from same group in same cycle
class BatchCluster(bundleType: DifftestBundle, groupSize: Int, param: BatchParam) extends Module {
  val alignWidth = bundleType.getByteAlignWidth
  val elemBytes = alignWidth / 8
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

  val chunks_map = Seq.tabulate(groupSize + 1) { vi =>
    (vi.U, (param.alignChunkBytes(elemBytes * vi) / param.ChunkByteLen).U)
  }
  val cluster_data_chunks = LookupTree(v_size, chunks_map)
  val cluster_info_cnt = Mux(v_size =/= 0.U, 1.U, 0.U)
  status_sum.data_chunks := status_base.data_chunks +& cluster_data_chunks
  status_sum.info_cnt := status_base.info_cnt +& cluster_info_cnt
  status_sum.cluster_data_chunks := cluster_data_chunks
  status_sum.cluster_info_cnt := cluster_info_cnt
}

// Collect Data from different group in same cycle
class BatchCollector(bundles: Seq[Valid[DifftestBundle]], param: BatchParam, config: GatewayConfig) extends Module {
  val in = IO(Flipped(Decoupled(MixedVec(bundles))))
  val out = IO(Decoupled(new BatchIO(param, config)))

  def getGroupDataWidth(group: Seq[Valid[DifftestBundle]]): Int =
    group.length * group.head.bits.getByteAlignWidth

  def getGroupDataChunks(group: Seq[Valid[DifftestBundle]]): Int = {
    val bytes = group.length * group.head.bits.getByteAlignWidth / 8
    param.alignChunkBytes(bytes) / param.ChunkByteLen
  }

  val in_group = in.bits.groupBy(_.bits.desiredCppName).values
  val in_group_single = in_group.filter(_.size == 1).toSeq
  val in_group_multi = in_group.filterNot(_.size == 1).toSeq
  val sorted = in_group_single.sortBy(getGroupDataWidth).reverse ++ in_group_multi.sortBy(getGroupDataWidth)
  val groupDataWidths = sorted.map(getGroupDataWidth)
  val clusterMaxDataChunks = sorted.map(getGroupDataChunks)
  val stepMaxDataChunks = clusterMaxDataChunks.sum
  val stepMaxPayloadChunks = param.StepInfoChunks + stepMaxDataChunks
  val stepMaxPayloadBeats = (stepMaxPayloadChunks + param.BeatChunkSize - 1) / param.BeatChunkSize
  require(stepMaxPayloadChunks <= 255, s"BatchStep.num only has 8 bits, but max batch chunks is $stepMaxPayloadChunks")

  // Stage 1: compact valid bundles inside each cluster and collect per-cluster stats.
  class GroupBundle extends Bundle {
    val data = MixedVec(groupDataWidths.map(group_w => UInt(group_w.W)))
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

  // Stage 2: build the chunk-aligned step payload from info and cluster data.
  // This register is held while Batch scans the payload beats.
  // Ready backpressure chain is cut off by skidBuffer for timing
  val delay_grouped = Wire(Decoupled(new GroupBundle))
  SkidBufferConnect(grouped, delay_grouped, splitDepth = 2)

  val delay_group_data = delay_grouped.bits.data
  val delay_group_info = delay_grouped.bits.info
  val delay_group_status = delay_grouped.bits.status
  val info_num = delay_group_status.last.info_cnt

  val cluster_chunk_count = delay_group_status.map(_.cluster_data_chunks)

  val BatchHead = Wire(new BatchInfo)
  BatchHead.id := Batch.getTemplate.length.U
  BatchHead.num := info_num
  val BatchStep = Wire(new BatchInfo)
  BatchStep.id := (Batch.getTemplate.length + 1).U

  // Collect info as BatchHead, valid bundle infos, BatchStep.
  // C++ scans BatchInfo from low bits to high bits, so append BatchHead at the
  // lowest bits and keep BatchStep after the compacted bundle infos.
  val toCollect_info = delay_group_info.reverse
  val info_res = Wire(MixedVec(Seq.tabulate(param.StepGroupSize) { idx => UInt(((idx + 2) * param.infoWidth).W) }))
  Seq.tabulate(param.StepGroupSize) { idx =>
    val info_base = if (idx == 0) BatchStep.asUInt else info_res(idx - 1)
    info_res(idx) := Mux(toCollect_info(idx) =/= 0.U, Cat(info_base, toCollect_info(idx)), info_base)
  }
  val info_raw = Cat(info_res.last, BatchHead.asUInt)
  val info_chunks = info_raw.pad(param.StepInfoBitLen).asTypeOf(Vec(param.StepInfoChunks, UInt(param.ChunkBitLen.W)))
  val info_chunk_count_map = Seq.tabulate(param.StepInfoSize + 1) { size =>
    val infoBytes = param.alignChunkBytes(size * param.infoByte)
    (size.U, (infoBytes / param.ChunkByteLen).U)
  }
  val info_chunk_count = LookupTree(info_num + 2.U, info_chunk_count_map)

  val payload_chunks = Wire(Vec(stepMaxPayloadChunks, UInt(param.ChunkBitLen.W)))
  val payload_chunk_valid = Wire(Vec(stepMaxPayloadChunks, Bool()))
  payload_chunks.foreach(_ := 0.U)
  payload_chunk_valid.foreach(_ := false.B)

  Seq.tabulate(param.StepInfoChunks) { idx =>
    payload_chunks(idx) := info_chunks(idx)
    payload_chunk_valid(idx) := idx.U < info_chunk_count
  }
  var payloadOffset = param.StepInfoChunks
  delay_group_data.zip(clusterMaxDataChunks).zipWithIndex.foreach { case ((data, chunks), groupIdx) =>
    val data_chunks = data.pad(chunks * param.ChunkBitLen).asTypeOf(Vec(chunks, UInt(param.ChunkBitLen.W)))
    Seq.tabulate(chunks) { chunkIdx =>
      payload_chunks(payloadOffset + chunkIdx) := data_chunks(chunkIdx)
      payload_chunk_valid(payloadOffset + chunkIdx) := chunkIdx.U < cluster_chunk_count(groupIdx)
    }
    payloadOffset += chunks
  }

  val payload_chunk_count = info_chunk_count +& cluster_chunk_count.reduce(_ +& _)
  BatchStep.num := payload_chunk_count.pad(8)(7, 0)

  val payload_beats = payload_chunks.asUInt
    .pad(stepMaxPayloadBeats * param.BeatBitLen)
    .asTypeOf(Vec(stepMaxPayloadBeats, Vec(param.BeatChunkSize, UInt(param.ChunkBitLen.W))))
  val payload_beat_valid = payload_chunk_valid.asUInt
    .pad(stepMaxPayloadBeats * param.BeatChunkSize)
    .asTypeOf(Vec(stepMaxPayloadBeats, UInt(param.BeatChunkSize.W)))

  val beatIdxWidth = math.max(1, log2Ceil(stepMaxPayloadBeats))
  val stateCountWidth = log2Ceil(param.BeatChunkSize + 1)
  val appendIndexWidth = log2Ceil(param.BeatChunkSize * 2 + 1)
  val remain_beat_mask = RegInit(0.U(stepMaxPayloadBeats.W))
  val state_chunks = RegInit(0.U.asTypeOf(Vec(param.BeatChunkSize, UInt(param.ChunkBitLen.W))))
  val state_count = RegInit(0.U(stateCountWidth.W))
  val pending_beat = Reg(Vec(param.BeatChunkSize, UInt(param.ChunkBitLen.W)))
  val pending_valid_mask = RegInit(0.U(param.BeatChunkSize.W))
  val pending_last = RegInit(false.B)
  val pending_valid = RegInit(false.B)

  // Stage 3: scan the payload beat bitmap and register one selected beat as pending.
  // Stage 4: independently merge pending into state and refill pending with the next beat.
  val pending_count = PopCount(pending_valid_mask)
  val merged_count = state_count +& pending_count
  val pending_pos = Wire(Vec(param.BeatChunkSize, UInt(appendIndexWidth.W)))
  pending_pos(0) := state_count
  for (pid <- 1 until param.BeatChunkSize) {
    pending_pos(pid) := pending_pos(pid - 1) + pending_valid_mask(pid - 1).asUInt
  }
  val merged_chunks = Seq.tabulate(param.BeatChunkSize * 2) { idx =>
    val stateOpt = Option.when(idx < param.BeatChunkSize)(Mux(idx.U < state_count, state_chunks(idx), 0.U))
    val choices = stateOpt.toSeq ++ Seq.tabulate(param.BeatChunkSize) { pid =>
      Mux(pending_valid_mask(pid) && pending_pos(pid) === idx.U, pending_beat(pid), 0.U)
    }
    VecInit(choices).reduce(_ | _)
  }
  val merged_head_chunks = VecInit(merged_chunks.take(param.BeatChunkSize))
  val merged_tail_chunks = VecInit(merged_chunks.drop(param.BeatChunkSize))
  val merge_full = merged_count >= param.BeatChunkSize.U
  val payload_beat_mask = VecInit(payload_beat_valid.map(_.orR)).asUInt
  val input_step_valid = delay_grouped.valid && info_num =/= 0.U && payload_beat_mask =/= 0.U
  val select_beat_mask = Mux(
    remain_beat_mask === 0.U,
    Mux(!pending_valid && state_count === 0.U && input_step_valid, payload_beat_mask, 0.U),
    remain_beat_mask,
  )
  val select_beat_idx = PriorityEncoder(select_beat_mask).pad(beatIdxWidth)
  val select_beat =
    if (stepMaxPayloadBeats == 1) payload_beats(0) else payload_beats(select_beat_idx)
  val select_beat_valid =
    if (stepMaxPayloadBeats == 1) payload_beat_valid(0) else payload_beat_valid(select_beat_idx)
  val next_remain_beat_mask = select_beat_mask & ~UIntToOH(select_beat_idx, stepMaxPayloadBeats)
  val state_tick = !pending_valid && select_beat_mask === 0.U && state_count =/= 0.U
  val merged_tick = pending_valid && (merge_full || pending_last)
  // A tick means emitting one Batch beat.
  val should_tick = merged_tick || state_tick
  val state_update = !should_tick || out.ready
  val pending_consumed = pending_valid && state_update
  // Only refill pending in the same cycle when the old pending beat can be
  // merged into state without waiting for downstream ready. This cuts the
  // out.ready -> load_pending_beat -> delay_grouped.ready control path.
  val pending_consumed_without_tick = pending_valid && !should_tick
  val pending_can_refill = !pending_valid || pending_consumed_without_tick
  val load_pending_beat = pending_can_refill && select_beat_mask =/= 0.U
  val next_state_chunks =
    Mux(merged_tick, merged_tail_chunks.asUInt, merged_head_chunks.asUInt).asTypeOf(state_chunks)
  val next_state_count = Mux(
    merged_tick,
    Mux(merge_full, merged_count - param.BeatChunkSize.U, 0.U),
    merged_count,
  )
  val is_last_step_beat = state_tick || (merged_tick && pending_last && merged_count <= param.BeatChunkSize.U)

  // Keep delay_grouped.bits stable while scanning the current step payload.
  delay_grouped.ready := !input_step_valid || (load_pending_beat && next_remain_beat_mask === 0.U)
  out.valid := should_tick
  out.bits.payload := Mux(state_tick, state_chunks.asUInt, merged_head_chunks.asUInt)
  out.bits.step := Mux(out.valid && is_last_step_beat, 1.U(config.stepWidth.W), 0.U)

  when(state_update) {
    when(pending_valid) {
      state_chunks := next_state_chunks
      state_count := next_state_count(stateCountWidth - 1, 0)
    }.elsewhen(state_tick) {
      state_chunks := 0.U.asTypeOf(state_chunks)
      state_count := 0.U
    }
  }

  when(load_pending_beat) {
    remain_beat_mask := next_remain_beat_mask
    pending_beat := select_beat
    pending_valid_mask := select_beat_valid
    pending_last := next_remain_beat_mask === 0.U
    pending_valid := true.B
  }.elsewhen(pending_consumed) {
    pending_valid := false.B
  }
}
