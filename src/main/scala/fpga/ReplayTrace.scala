/***************************************************************************************
 * Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
 *
 * DiffTest is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

package difftest.fpga

import chisel3._
import chisel3.util._
import difftest.gateway.{ReplayTraceRequest, ReplayTraceStatus}

object ReplayTraceProtocol {
  val Magic: BigInt = BigInt("5254524143453031", 16) // "RTRACE01"
  val Version = 1
}

class ReplayTraceWrite(dataWidth: Int) extends Bundle {
  val payload = UInt(dataWidth.W)
  val traceHead = UInt(16.W)
  val traceSize = UInt(16.W)
  val traceValid = Bool()
}

class ReplayTraceUram(dataWidth: Int, depth: Int) extends BlackBox with HasBlackBoxInline {
  private val addrWidth = log2Ceil(depth)

  override def desiredName: String = s"ReplayTraceUram_${dataWidth}x$depth"

  val io = IO(new Bundle {
    val clock = Input(Clock())
    val wr_en = Input(Bool())
    val wr_addr = Input(UInt(addrWidth.W))
    val wr_data = Input(UInt(dataWidth.W))
    val rd_en = Input(Bool())
    val rd_addr = Input(UInt(addrWidth.W))
    val rd_data = Output(UInt(dataWidth.W))
  })

  setInline(
    s"$desiredName.sv",
    s"""module $desiredName (
       |  input  wire                  clock,
       |  input  wire                  wr_en,
       |  input  wire [${addrWidth - 1}:0] wr_addr,
       |  input  wire [${dataWidth - 1}:0] wr_data,
       |  input  wire                  rd_en,
       |  input  wire [${addrWidth - 1}:0] rd_addr,
       |  output reg  [${dataWidth - 1}:0] rd_data
       |);
       |  (* ram_style = "ultra" *) reg [${dataWidth - 1}:0] mem [0:${depth - 1}];
       |
       |  always @(posedge clock) begin
       |    if (wr_en) begin
       |      mem[wr_addr] <= wr_data;
       |    end
       |    if (rd_en) begin
       |      rd_data <= mem[rd_addr];
       |    end
       |  end
       |endmodule
       |""".stripMargin,
  )
}

class ReplayTraceBuffer(dataWidth: Int, depth: Int, replaySize: Int) extends Module {
  require(isPow2(depth), s"Replay trace depth must be a power of two, got $depth")
  require(isPow2(replaySize), s"Replay size must be a power of two, got $replaySize")
  require(dataWidth >= 320, s"Replay trace header needs at least 320 bits, got $dataWidth")

  private val ptrWidth = log2Ceil(depth)
  private val traceIndexWidth = log2Ceil(replaySize)
  private val sequenceWidth = 32
  private val descriptorWidth = sequenceWidth + 1

  val io = IO(new Bundle {
    val in = Flipped(Decoupled(new ReplayTraceWrite(dataWidth)))
    val request = Input(new ReplayTraceRequest)
    val status = Output(new ReplayTraceStatus)
    val dump = Decoupled(UInt(dataWidth.W))
  })

  private val states = Enum(11)
  val capture = states(0)
  val frozen = states(1)
  val lookupPrev = states(2)
  val lookupPrevWait = states(3)
  val lookupEnd = states(4)
  val lookupEndWait = states(5)
  val dumpHeader = states(6)
  val dumpRead = states(7)
  val dumpEmit = states(8)
  val dumpPad = states(9)
  val dumpDone = states(10)
  val state = RegInit(capture)

  val dataMem = Module(new ReplayTraceUram(dataWidth, depth))
  dataMem.io.clock := clock
  dataMem.io.wr_en := false.B
  dataMem.io.wr_addr := 0.U
  dataMem.io.wr_data := 0.U
  dataMem.io.rd_en := false.B
  dataMem.io.rd_addr := 0.U

  val descriptors = SyncReadMem(replaySize, UInt(descriptorWidth.W))
  // SyncReadMem contents are intentionally not reset; keep validity in resettable
  // flops so an unwritten descriptor can never authorize a dump.
  val descriptorValid = RegInit(VecInit(Seq.fill(replaySize)(false.B)))
  val descReadEnable = WireDefault(false.B)
  val descReadAddr = WireDefault(0.U(traceIndexWidth.W))
  val descReadData = descriptors.read(descReadAddr, descReadEnable)

  val writePtr = RegInit(0.U(ptrWidth.W))
  val writeSeq = RegInit(0.U(sequenceWidth.W))
  val snapshotPtr = RegInit(0.U(ptrWidth.W))
  val snapshotSeq = RegInit(0.U(sequenceWidth.W))
  val oldestSeq = RegInit(0.U(sequenceWidth.W))

  val requestHead = RegInit(0.U(16.W))
  val requestSize = RegInit(0.U(16.W))
  val previousDescriptor = RegInit(0.U(descriptorWidth.W))
  val dumpStartPtr = RegInit(0.U(ptrWidth.W))
  val dumpWords = RegInit(0.U(sequenceWidth.W))
  val dumpRemaining = RegInit(0.U(sequenceWidth.W))
  val dumpReadPtr = RegInit(0.U(ptrWidth.W))
  val padRemaining = RegInit(0.U(3.W))
  val rangeLost = RegInit(false.B)
  val dumpHandled = RegInit(false.B)

  def traceIndex(value: UInt): UInt = value(traceIndexWidth - 1, 0)

  io.in.ready := true.B
  val captureFire = io.in.fire && state === capture && !io.request.freeze
  val traceTail = traceIndex(io.in.bits.traceHead + io.in.bits.traceSize - 1.U)
  dataMem.io.wr_en := captureFire
  dataMem.io.wr_addr := writePtr
  dataMem.io.wr_data := io.in.bits.payload

  when(captureFire) {
    writePtr := writePtr + 1.U
    writeSeq := writeSeq + 1.U
    when(io.in.bits.traceValid && io.in.bits.traceSize =/= 0.U) {
      descriptors.write(traceTail, Cat(true.B, writeSeq))
      descriptorValid(traceTail) := true.B
    }
  }

  when(state === capture && io.request.freeze) {
    snapshotPtr := writePtr
    snapshotSeq := writeSeq
    oldestSeq := Mux(writeSeq > depth.U, writeSeq - depth.U, 0.U)
    state := frozen
  }

  val previousHead = traceIndex(requestHead - 1.U)
  val requestTail = traceIndex(requestHead + requestSize - 1.U)

  when(state === frozen) {
    when(io.request.rearm && !io.request.freeze) {
      rangeLost := false.B
      dumpHandled := false.B
      state := capture
    }.elsewhen(!io.request.dump) {
      dumpHandled := false.B
    }.elsewhen(!dumpHandled) {
      requestHead := io.request.traceHead
      requestSize := io.request.traceSize
      rangeLost := false.B
      dumpHandled := true.B
      state := lookupPrev
    }
  }

  when(state === lookupPrev) {
    descReadEnable := true.B
    descReadAddr := previousHead
    state := lookupPrevWait
  }

  when(state === lookupPrevWait) {
    previousDescriptor := Cat(descriptorValid(previousHead), descReadData(sequenceWidth - 1, 0))
    state := lookupEnd
  }

  when(state === lookupEnd) {
    descReadEnable := true.B
    descReadAddr := requestTail
    state := lookupEndWait
  }

  val previousValid = previousDescriptor(descriptorWidth - 1)
  val previousSeq = previousDescriptor(sequenceWidth - 1, 0)
  val endValid = descriptorValid(requestTail)
  val endSeq = descReadData(sequenceWidth - 1, 0)
  val firstAvailableSeq = Mux(previousValid && previousSeq >= oldestSeq, previousSeq + 1.U, oldestSeq)
  val rangeAvailable =
    requestSize =/= 0.U && endValid && endSeq >= firstAvailableSeq && endSeq < snapshotSeq && firstAvailableSeq >= oldestSeq
  val selectedStartSeq = Mux(rangeAvailable, firstAvailableSeq, 0.U(sequenceWidth.W))
  val selectedWords = Mux(rangeAvailable, endSeq - firstAvailableSeq + 1.U, 0.U(sequenceWidth.W))
  val packetRemainder = (selectedWords + 1.U)(2, 0)
  val selectedPadding = Mux(packetRemainder === 0.U, 0.U(3.W), (8.U(4.W) - packetRemainder)(2, 0))

  when(state === lookupEndWait) {
    rangeLost := !rangeAvailable
    dumpStartPtr := selectedStartSeq(ptrWidth - 1, 0)
    dumpReadPtr := selectedStartSeq(ptrWidth - 1, 0)
    dumpWords := selectedWords
    dumpRemaining := selectedWords
    padRemaining := selectedPadding
    state := dumpHeader
  }

  // The serialized header keeps rangeLost in flags bit 0; the AXI status word
  // has its own bit layout.
  // The serialized header keeps rangeLost in flags bit 0; the AXI status word
  // has its own bit layout.
  val dumpFlags = Cat(0.U(31.W), rangeLost)
  val dumpHeaderBits = Cat(
    0.U((dataWidth - 320).W),
    snapshotSeq,
    snapshotPtr.pad(32),
    dumpWords,
    dumpStartPtr.pad(32),
    requestSize,
    requestHead,
    dumpFlags,
    ReplayTraceProtocol.Version.U(32.W),
    ReplayTraceProtocol.Magic.U(64.W),
  )

  io.dump.valid := false.B
  io.dump.bits := 0.U
  when(state === dumpHeader) {
    io.dump.valid := true.B
    io.dump.bits := dumpHeaderBits
    when(io.dump.fire) {
      when(dumpRemaining =/= 0.U) {
        state := dumpRead
      }.elsewhen(padRemaining =/= 0.U) {
        state := dumpPad
      }.otherwise {
        state := dumpDone
      }
    }
  }

  when(state === dumpRead) {
    dataMem.io.rd_en := true.B
    dataMem.io.rd_addr := dumpReadPtr
    state := dumpEmit
  }

  when(state === dumpEmit) {
    io.dump.valid := true.B
    io.dump.bits := dataMem.io.rd_data
    when(io.dump.fire) {
      dumpRemaining := dumpRemaining - 1.U
      dumpReadPtr := dumpReadPtr + 1.U
      when(dumpRemaining === 1.U) {
        when(padRemaining =/= 0.U) {
          state := dumpPad
        }.otherwise {
          state := dumpDone
        }
      }.otherwise {
        state := dumpRead
      }
    }
  }

  when(state === dumpPad) {
    io.dump.valid := true.B
    io.dump.bits := 0.U
    when(io.dump.fire) {
      padRemaining := padRemaining - 1.U
      when(padRemaining === 1.U) {
        state := dumpDone
      }
    }
  }

  io.status.frozen := state =/= capture
  io.status.dumpActive := state === dumpHeader || state === dumpRead || state === dumpEmit || state === dumpPad
  io.status.dumpDone := state === dumpDone
  io.status.rangeLost := rangeLost
  io.status.writePtr := writePtr.pad(32)
  io.status.writeSeq := writeSeq
  io.status.dumpStart := dumpStartPtr.pad(32)
  io.status.dumpBeats := dumpWords
}
