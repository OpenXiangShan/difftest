# Example: difftest in NutShell project

<!-- difftest 使用实例: 在果壳中使用 difftest. -->

这里包含一些来自果壳项目的代码片段, 用来解释如何修改 RTL代码以使之支持 difftest: 

> Tips: 可以在果壳项目中使用全局查找来找到这些代码出现的位置

  NutShell的代码和文档可以在这里取得 : https://github.com/OSCPU/NutShell, https://oscpu.github.io/NutShell-doc/

## 需要添加到 RTL 代码对应位置

```scala
import difftest._  
  if (!p.FPGAPlatform) {
    val difftest = Module(new DifftestArchIntRegState)
    difftest.io.clock  := clock
    difftest.io.coreid := 0.U // TODO
    difftest.io.gpr    := VecInit((0 to NRReg-1).map(i => rf.read(i.U)))
  }

  if (!p.FPGAPlatform) {
    val difftest = Module(new DifftestTrapEvent)
    difftest.io.clock    := clock
    difftest.io.coreid   := 0.U // TODO: nutshell does not support coreid auto config
    difftest.io.valid    := nutcoretrap
    difftest.io.code     := io.in.bits.data.src1
    difftest.io.pc       := io.in.bits.cf.pc
    difftest.io.cycleCnt := cycleCnt
    difftest.io.instrCnt := instrCnt
  }

  if (!p.FPGAPlatform) {
    val difftest = Module(new DifftestInstrCommit)
    difftest.io.clock    := clock
    difftest.io.coreid   := 0.U
    difftest.io.index    := 0.U

    difftest.io.valid    := RegNext(io.in.valid)
    difftest.io.pc       := RegNext(SignExt(io.in.bits.decode.cf.pc, AddrBits))
    difftest.io.instr    := RegNext(io.in.bits.decode.cf.instr)
    difftest.io.skip     := RegNext(io.in.bits.isMMIO)
    difftest.io.isRVC    := RegNext(io.in.bits.decode.cf.instr(1,0)=/="b11".U)
    difftest.io.scFailed := RegNext(false.B) // TODO: fixme
    difftest.io.wen      := RegNext(io.wb.rfWen && io.wb.rfDest =/= 0.U) // && valid(ringBufferTail)(i) && commited(ringBufferTail)(i)
    difftest.io.wdata    := RegNext(io.wb.rfData)
    difftest.io.wdest    := RegNext(io.wb.rfDest)
  }

  if (!p.FPGAPlatform) {
    val difftest = Module(new DifftestCSRState)
    difftest.io.clock := clock
    difftest.io.coreid := 0.U // TODO
    difftest.io.priviledgeMode := RegNext(priviledgeMode)
    difftest.io.mstatus := RegNext(mstatus)
    difftest.io.sstatus := RegNext(mstatus & sstatusRmask)
    difftest.io.mepc := RegNext(mepc)
    difftest.io.sepc := RegNext(sepc)
    difftest.io.mtval:= RegNext(mtval)
    difftest.io.stval:= RegNext(stval)
    difftest.io.mtvec := RegNext(mtvec)
    difftest.io.stvec := RegNext(stvec)
    difftest.io.mcause := RegNext(mcause)
    difftest.io.scause := RegNext(scause)
    difftest.io.satp := RegNext(satp)
    difftest.io.mip := RegNext(mipReg)
    difftest.io.mie := RegNext(mie)
    difftest.io.mscratch := RegNext(mscratch)
    difftest.io.sscratch := RegNext(sscratch)
    difftest.io.mideleg := RegNext(mideleg)
    difftest.io.medeleg := RegNext(medeleg)

    val difftestArchEvent = Module(new DifftestArchEvent)
    difftestArchEvent.io.clock := clock
    difftestArchEvent.io.coreid := 0.U // TODO
    difftestArchEvent.io.intrNO := RegNext(Mux(raiseIntr && io.instrValid && valid, intrNO, 0.U))
    difftestArchEvent.io.cause := RegNext(Mux(raiseException && io.instrValid && valid, exceptionNO, 0.U))
    difftestArchEvent.io.exceptionPC := RegNext(SignExt(io.cfIn.pc, XLEN))
  }
```

## 需要使用`RAMHelper`作为仿真内存

```scala
class AXI4RAM[T <: AXI4Lite](_type: T = new AXI4, memByte: Int,
  useBlackBox: Boolean = false) extends AXI4SlaveModule(_type) with HasNutCoreParameter {

  val offsetBits = log2Up(memByte)
  val offsetMask = (1 << offsetBits) - 1
  def index(addr: UInt) = (addr & offsetMask.U) >> log2Ceil(DataBytes)
  def inRange(idx: UInt) = idx < (memByte / 8).U

  val wIdx = index(waddr) + writeBeatCnt
  val rIdx = index(raddr) + readBeatCnt
  val wen = in.w.fire() && inRange(wIdx)

  val rdata = if (useBlackBox) {
    val mem = Module(new RAMHelper(memByte))
    mem.io.clk := clock
    mem.io.rIdx := rIdx
    mem.io.wIdx := wIdx
    mem.io.wdata := in.w.bits.data
    mem.io.wmask := fullMask
    mem.io.wen := wen
    mem.io.en := true.B
    mem.io.rdata
  } else {
    val mem = Mem(memByte / DataBytes, Vec(DataBytes, UInt(8.W)))

    val wdata = VecInit.tabulate(DataBytes) { i => in.w.bits.data(8*(i+1)-1, 8*i) }
    when (wen) { mem.write(wIdx, wdata, in.w.bits.strb.asBools) }

    Cat(mem.read(rIdx).reverse)
  }

  in.r.bits.data := RegEnable(rdata, ren)
}
```

## 需要添加到仿真顶层

```scala
// SimTop.scala
import difftest._
// ......
class SimTop extends Module {
  val io = IO(new Bundle(){
    val logCtrl = new LogCtrlIO
    val perfInfo = new PerfInfoIO
    val uart = new UARTIO
    // .......
  })
  // ......
}
```