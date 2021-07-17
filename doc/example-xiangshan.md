# Example: difftest in XiangShan project

<!-- difftest 使用实例: 在香山中使用 difftest. -->

这里包含一些来自香山项目的代码片段, 用来解释如何修改 RTL代码以使之支持 difftest: 

> Tips: 可以在香山项目中使用全局查找来找到这些代码出现的位置

  香山的代码和文档可以在这里取得: https://github.com/OpenXiangShan/XiangShan

## 需要添加到 RTL 代码对应位置

```scala
import difftest._  
  
  if (!env.FPGAPlatform) {
    val difftest = Module(new DifftestArchEvent)
    difftest.io.clock := clock
    difftest.io.coreid := hardId.U
    difftest.io.intrNO := RegNext(difftestIntrNO)
    difftest.io.cause := RegNext(Mux(csrio.exception.valid, causeNO, 0.U))
    difftest.io.exceptionPC := RegNext(SignExt(csrio.exception.bits.uop.cf.pc, XLEN))
  }

  if (!env.FPGAPlatform) {
  for (i <- 0 until CommitWidth) {
      val difftest = Module(new DifftestInstrCommit)
      difftest.io.clock    := clock
      difftest.io.coreid   := hardId.U
      difftest.io.index    := i.U

      val ptr = deqPtrVec(i).value
      val uop = debug_microOp(ptr)
      val exuOut = debug_exuDebug(ptr)
      val exuData = debug_exuData(ptr)
      difftest.io.valid    := RegNext(io.commits.valid(i) && !io.commits.isWalk)
      difftest.io.pc       := RegNext(SignExt(uop.cf.pc, XLEN))
      difftest.io.instr    := RegNext(uop.cf.instr)
      difftest.io.skip     := RegNext(exuOut.isMMIO || exuOut.isPerfCnt)
      difftest.io.isRVC    := RegNext(uop.cf.pd.isRVC)
      difftest.io.scFailed := RegNext(!uop.diffTestDebugLrScValid &&
      uop.ctrl.fuType === FuType.mou &&
      (uop.ctrl.fuOpType === LSUOpType.sc_d || uop.ctrl.fuOpType === LSUOpType.sc_w))
      difftest.io.wen      := RegNext(io.commits.valid(i) && uop.ctrl.rfWen && uop.ctrl.ldest =/= 0.U)
      difftest.io.wdata    := RegNext(exuData)
      difftest.io.wdest    := RegNext(uop.ctrl.ldest)
    }
  }

  if (!env.FPGAPlatform) {
    val difftest = Module(new DifftestTrapEvent)
    difftest.io.clock    := clock
    difftest.io.coreid   := hardId.U
    difftest.io.valid    := hitTrap
    difftest.io.code     := trapCode
    difftest.io.pc       := trapPC
    difftest.io.cycleCnt := GTimer()
    difftest.io.instrCnt := instrCnt
  }

  if (!env.FPGAPlatform) {
    val difftest = Module(new DifftestCSRState)
    difftest.io.clock := clock
    difftest.io.coreid := hardId.U
    difftest.io.priviledgeMode := priviledgeMode
    difftest.io.mstatus := mstatus
    difftest.io.sstatus := mstatus & sstatusRmask
    difftest.io.mepc := mepc
    difftest.io.sepc := sepc
    difftest.io.mtval:= mtval
    difftest.io.stval:= stval
    difftest.io.mtvec := mtvec
    difftest.io.stvec := stvec
    difftest.io.mcause := mcause
    difftest.io.scause := scause
    difftest.io.satp := satp
    difftest.io.mip := mipReg
    difftest.io.mie := mie
    difftest.io.mscratch := mscratch
    difftest.io.sscratch := sscratch
    difftest.io.mideleg := mideleg
    difftest.io.medeleg := medeleg
  }

  if (!env.FPGAPlatform) {
    for ((rport, rat) <- intRf.io.debug_rports.zip(io.fromCtrlBlock.debug_rat)) {
      rport.addr := rat
    }
    val difftest = Module(new DifftestArchIntRegState)
    difftest.io.clock  := clock
    difftest.io.coreid := hardId.U
    difftest.io.gpr    := VecInit(intRf.io.debug_rports.map(_.data))
  }

  if (!env.FPGAPlatform) {
    for ((rport, rat) <- fpRf.io.debug_rports.zip(io.fromCtrlBlock.debug_rat)) {
      rport.addr := rat
    }
    val difftest = Module(new DifftestArchFpRegState)
    difftest.io.clock  := clock
    difftest.io.coreid := hardId.U
    difftest.io.fpr    := VecInit(fpRf.io.debug_rports.map(p => ieee(p.data)))
  }

  // march related apis 

  if (!env.FPGAPlatform) {
    val difftest = Module(new DifftestSbufferEvent)
    difftest.io.clock := clock
    difftest.io.coreid := hardId.U
    difftest.io.sbufferResp := io.dcache.resp.fire()
    difftest.io.sbufferAddr := getAddr(tag(respId))
    difftest.io.sbufferData := data(respId).asTypeOf(Vec(CacheLineBytes, UInt(8.W)))
    difftest.io.sbufferMask := mask(respId).asUInt
  }

  if (!env.FPGAPlatform) {
    for (i <- 0 until StorePipelineWidth) {
      val storeCommit = io.sbuffer(i).fire()
      val waddr = SignExt(io.sbuffer(i).bits.addr, 64)
      val wdata = io.sbuffer(i).bits.data & MaskExpand(io.sbuffer(i).bits.mask)
      val wmask = io.sbuffer(i).bits.mask

      val difftest = Module(new DifftestStoreEvent)
      difftest.io.clock       := clock
      difftest.io.coreid      := hardId.U
      difftest.io.index       := i.U
      difftest.io.valid       := storeCommit
      difftest.io.storeAddr   := waddr
      difftest.io.storeData   := wdata
      difftest.io.storeMask   := wmask
    }
  }


  if (!env.FPGAPlatform) {
    for (i <- 0 until CommitWidth) {
      val difftest = Module(new DifftestLoadEvent)
      difftest.io.clock  := clock
      difftest.io.coreid := hardId.U
      difftest.io.index  := i.U

      val ptr = deqPtrVec(i).value
      val uop = debug_microOp(ptr)
      val exuOut = debug_exuDebug(ptr)
      difftest.io.valid  := RegNext(io.commits.valid(i) && !io.commits.isWalk)
      difftest.io.paddr  := RegNext(exuOut.paddr)
      difftest.io.opType := RegNext(uop.ctrl.fuOpType)
      difftest.io.fuType := RegNext(uop.ctrl.fuType)
    }
  }

  if (!env.FPGAPlatform) {
    val difftest = Module(new DifftestAtomicEvent)
    difftest.io.clock      := clock
    difftest.io.coreid     := hardId.U
    difftest.io.atomicResp := io.dcache.resp.fire()
    difftest.io.atomicAddr := paddr_reg
    difftest.io.atomicData := data_reg
    difftest.io.atomicMask := mask_reg
    difftest.io.atomicFuop := fuop_reg
    difftest.io.atomicOut  := resp_data_wire
  }
```


## 需要使用`RAMHelper`作为仿真内存

```scala
class AXI4RAM
(
  address: Seq[AddressSet],
  memByte: Long,
  useBlackBox: Boolean = false,
  executable: Boolean = true,
  beatBytes: Int = 8,
  burstLen: Int = 16,
)(implicit p: Parameters)
  extends AXI4SlaveModule(address, executable, beatBytes, burstLen)
{ 
  override lazy val module = new AXI4SlaveModuleImp(this){

    val split = beatBytes / 8
    val bankByte = memByte / split
    val offsetBits = log2Up(memByte)

    require(address.length >= 1)
    val baseAddress = address(0).base

    def index(addr: UInt) = ((addr - baseAddress.U)(offsetBits - 1, 0) >> log2Ceil(beatBytes)).asUInt()

    def inRange(idx: UInt) = idx < (memByte / beatBytes).U

    val wIdx = index(waddr) + writeBeatCnt
    val rIdx = index(raddr) + readBeatCnt
    val wen = in.w.fire() && inRange(wIdx)
    require(beatBytes >= 8)

    val rdata = if (useBlackBox) {
      val mems = (0 until split).map {_ => Module(new RAMHelper(bankByte))}
      mems.zipWithIndex map { case (mem, i) =>
        mem.io.clk   := clock
        mem.io.en    := !reset.asBool() && ((state === s_rdata) || (state === s_wdata))
        mem.io.rIdx  := (rIdx << log2Up(split)) + i.U
        mem.io.wIdx  := (wIdx << log2Up(split)) + i.U
        mem.io.wdata := in.w.bits.data((i + 1) * 64 - 1, i * 64)
        mem.io.wmask := MaskExpand(in.w.bits.strb((i + 1) * 8 - 1, i * 8))
        mem.io.wen   := wen
      }
      val rdata = mems.map {mem => mem.io.rdata}
      Cat(rdata.reverse)
    } else {
      val mem = Mem(memByte / beatBytes, Vec(beatBytes, UInt(8.W)))

      val wdata = VecInit.tabulate(beatBytes) { i => in.w.bits.data(8 * (i + 1) - 1, 8 * i) }
      when(wen) {
        mem.write(wIdx, wdata, in.w.bits.strb.asBools())
      }

      Cat(mem.read(rIdx).reverse)
    }
    in.r.bits.data := rdata
  }
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