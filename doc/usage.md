Difftest API 的使用
===============

传递给 difftest 的各组信号需要按时钟周期对齐, 即: 一些传递给 difftest 的信号需要使用`RegNext`来暂存一拍. 本文将解释这一操作的目的, 并介绍如何在其他设计中正确设计时序逻辑来将调试信号传递给 difftest.

典型的`RegNext`操作如下所示: 

```scala
  if (!env.FPGAPlatform) {
    val difftest = Module(new DifftestArchEvent)
    difftest.io.clock := clock
    difftest.io.coreid := hardId.U
    difftest.io.intrNO := RegNext(difftestIntrNO)
    difftest.io.cause := RegNext(Mux(csrio.exception.valid, causeNO, 0.U))
    difftest.io.exceptionPC := RegNext(SignExt(csrio.exception.bits.uop.cf.pc, XLEN))
  }
```

# 传递信号给 difftest 的核心原则

传递信号给 difftest 的核心原则只有一个, **在指令提交的时刻其产生的影响恰好生效**. 

# 示例

我们用几个例子来解释这一原则:

## 简单的顺序单发射处理器`NutShell` 

  NutShell的代码和文档可以在这里取得 : https://github.com/OSCPU/NutShell, https://oscpu.github.io/NutShell-doc/

NutShell的寄存器写回操作均在写回级发生. 对于顺序处理器, 指令的写回可以对应到乱序处理器中的指令提交操作. 下面我们统一用*指令提交*来指代顺序处理器的指令写回和乱序处理器的*指令提交*.

在写回级对通用寄存器堆的写入在下一个时钟周期才会对读操作可见, 因此我们需要在下一周期读取寄存器堆的状态. 于是, 我们可以这样连接 difftest 的相关信号:

```scala
DifftestInstrCommit <--> RegNext(指令提交)
DifftestArchIntRegState <--> 体系结构整数寄存器堆读结果
```

这样, 指令提交的下一个时钟周期, difftest 从整数寄存器堆中读出受这些指令影响后的寄存器堆的值, 并将寄存器堆信息与对应周期的指令提交信息一并提交给 difftest .

NutShell的  CSR 被设计为会在执行级(写回级的上一个流水级)根据 CSR 指令或其他信息更新 CSR 的状态. 也就是说, 在指令写回的这一周期, CSR 值发生的变动已经可以被从 CSR 中读出. 因此, 我们将触发指令提交当周期从 CSR 中读取的值交给 difftest .

加入 CSR 读取之后的 difftest 连线如下所示:

```scala
DifftestInstrCommit <--> RegNext(指令提交)
DifftestArchIntRegState <--> 体系结构整数寄存器堆读结果
DifftestCSRState <--> RegNext(CSR读结果)
```

接下来, 我们需要保证中断/异常触发与它们所在的指令提交信息是同时被传递给 difftest 的. 由于写回信息被延迟一拍传递给 difftest , 中断/异常触发也要被延迟一拍:

```scala
DifftestInstrCommit <--> RegNext(指令提交)
DifftestArchIntRegState <--> 体系结构整数寄存器堆读结果
DifftestCSRState <--> RegNext(CSR读结果)
DifftestArchEvent <--> RegNext(指令提交处中断/例外)
DifftestTrapEvent <--> RegNext(指令提交处trap信号)
```

最后, 我们实例化一些没有用到的端口, 将所有信号置为低电平, 来让顶层能正确通过编译:

```scala
DifftestInstrCommit <--> RegNext(指令提交)
DifftestArchIntRegState <--> 体系结构整数寄存器堆读结果
DifftestCSRState <--> RegNext(CSR读结果)
DifftestArchEvent <--> RegNext(指令提交处中断/例外)
DifftestTrapEvent <--> RegNext(指令提交处trap信号)
DifftestArchFpRegState := DontCare
DifftestSbufferEvent := DontCare
DifftestStoreEvent := DontCare
DifftestLoadEvent := DontCare
DifftestAtomicEvent := DontCare
```

这样, 我们就完成了 difftest 框架与 RTL 设计的整合. 

## 乱序多发射处理器`香山`

相比顺序单发射处理器, 乱序处理器的行为更加复杂, 但是 difftest 的整体思想依然不变. 这里以乱序多发射处理器香山为例来展开介绍:

  香山的代码和文档可以在这里取得: https://github.com/OpenXiangShan/XiangShan

香山处理器维护了一个体系结构重命名表. 在指令提交时会根据提交信息更新体系结构重命名表. 这样, 在指令提交的下一个周期, 就可以根据体系结构重命名表的值来查询重命名寄存器堆, 获取体系结构寄存器的值. 这一查询会在指令提交后的下一个周期发生:

```scala
DifftestInstrCommit <--> RegNext(指令提交)
DifftestArchIntRegState <--> 体系结构整数寄存器堆读结果
DifftestArchFpRegState  <--> 体系结构浮点寄存器堆读结果
```

香山处理器中的 CSR 更新时机与 NutShell 的略有不同. 香山处理器会在指令提交时更新CSR 的状态. 这就意味着在提交的指令产生的影响要在下一周期才能反应在 CSR 读取结果上. 同样, 我们选择在指令提交后的下一个周期读取 CSR 的状态:

```scala
DifftestInstrCommit <--> RegNext(指令提交)
DifftestArchIntRegState <--> 体系结构整数寄存器堆读结果
DifftestArchFpRegState  <--> 体系结构浮点寄存器堆读结果
DifftestCSRState <--> CSR读结果
```

将中断/异常触发与它们所在的指令提交信息同时传递给 difftest:

```scala
DifftestInstrCommit <--> RegNext(指令提交)
DifftestArchIntRegState <--> 体系结构整数寄存器堆读结果
DifftestArchFpRegState  <--> 体系结构浮点寄存器堆读结果
DifftestCSRState <--> CSR读结果
DifftestArchEvent <--> RegNext(提交处中断/例外)
DifftestTrapEvent <--> RegNext(提交处trap信号)
```

截至目前, 我们已经完成了基本 difftest 信号的连接. Difftest 机制已经可以在乱序处理器上运行起来了. 接下来, 我们添加一些用于 SMP 验证的信号.

* DifftestSbufferEvent
* DifftestStoreEvent
* DifftestLoadEvent
* DifftestAtomicEvent

这些信号主要被用于对外提供处理器内部的微结构信息. Difftest 框架可以利用这些信息检查 SMP 场景下的访存结果是否合法.

```scala
DifftestInstrCommit <--> RegNext(指令提交)
DifftestArchIntRegState <--> 体系结构整数寄存器堆读结果
DifftestArchFpRegState  <--> 体系结构浮点寄存器堆读结果
DifftestCSRState <--> CSR读结果
DifftestArchEvent <--> RegNext(提交处中断/例外)
DifftestTrapEvent <--> RegNext(提交处trap信号)
DifftestSbufferEvent <--> 对应模块
DifftestStoreEvent <--> 对应模块
DifftestLoadEvent <--> 对应模块
DifftestAtomicEvent <--> 对应模块
```
