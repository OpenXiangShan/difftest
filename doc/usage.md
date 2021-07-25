Difftest 使用指南
===============

本仓库提供了一个基于 verilator 进行 difftest 的仿真顶层.

本文档将介绍如何将 difftest 框架接入到一个使用 Chisel 设计的处理器项目中. 对于使用 verilog 的设计, 接入 difftest 框架的整体操作是类似的.

# 将 difftest 放入工程

如下图所示建立目录:

```
.
├── build
│   └── SimTop.v // 处理器 verilog 源代码
├── difftest // difftest 仓库, 可以作为 submodule 引入
└── ......
```

其中, SimTop.v 是处理器的 verilog 源代码. 这份源代码可由 Chisel 生成, 也可以是直接使用 verilog 编写的. 后续编译的结果会放在 `build` 这个目录下.

仿真顶层模块名称设置为 `SimTop`.

# 在设计中将关键信号传递给 difftest 框架

## 信号传递的接口设计

这一版本的 difftest 采用 DPI-C 来将仿真中的信号传递到 difftest 框架中. 在仿真程序执行的过程中会调用 DPI-C 函数, 将 difftest 感兴趣的信号写入到对应的结构体中. 

### 使用 Chisel 的设计

在使用 Chisel 的设计中, 我们可以使用 blackbox 来调用 DPI-C 函数. difftest 框架已经将相关的操作进行了封装, 在代码中`import difftest._`, 之后就可以直接实例化 difftest 的各个 module. 例如:

```scala
import difftest._
// ......

class WBU {
  if (!env.FPGAPlatform) { // 只有在仿真时才需要 difftest 的 module
    val difftest = Module(new DifftestArchEvent)
    difftest.io.clock := clock
    // ......
  }
}
```

这些 blackbox 定义在 `difftest/src/main/scala/Difftest.scala` 中.

### 使用 Verilog 的设计

与 Chisel 的用法相似. Verilog 用户不需要上述的这些封装操作, 可以直接调用 DPI-C 函数. 

这些函数定义在 `difftest/src/test/vsrc/common/difftest.v` 中.

## 传递信号给 difftest 的核心原则

传递信号给 difftest 的核心原则只有一个, **在指令提交的时刻其产生的影响恰好生效**. 

为了满足**在指令提交的时刻其产生的影响恰好生效**的原则, 一些传递给 difftest 的信号需要被延迟一拍. 下面的代码展示了如何使用 Chisel 将这些信号延迟一拍传递. 

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

下面的例子将更详细地解释这一原则.

## 示例

### 简单的顺序单发射处理器`NutShell` 

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

最后, 没有用到的非必需信号无需处理:

```scala
DifftestInstrCommit <--> RegNext(指令提交)
DifftestArchIntRegState <--> 体系结构整数寄存器堆读结果
DifftestCSRState <--> RegNext(CSR读结果)
DifftestArchEvent <--> RegNext(指令提交处中断/例外)
DifftestTrapEvent <--> RegNext(指令提交处trap信号)
// DifftestArchFpRegState := DontCare
// DifftestSbufferEvent := DontCare
// DifftestStoreEvent := DontCare
// DifftestLoadEvent := DontCare
// DifftestAtomicEvent := DontCare
```

这样, 我们就完成了 difftest 框架与 RTL 设计的整合. 

### 乱序多发射处理器`香山`

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

# 配置内存

`RAMHelper` 模块位于 `difftest/src/test/vsrc/common/ram.v`. 在设计中需要使用这一模块来作为仿真内存. 可以参考 NutShell 中 `AXI4RAM` 的处理方式.

# 配置仿真顶层

在开始配置仿真顶层之前, 请再次确认用于仿真的 verilog 文件名能和 difftest 中 Makefile 中的对应上.

仿真框架的顶层默认连接了一些 IO 端口, 它们的定义在  `difftest/src/main/scala/Difftest.scala` 中. 在仿真过程中可以根据 `LogCtrlIO`, `PerfInfoIO` 传入的信号, 控制 debug 信息的输出以及性能计数器的表现. 这部分的控制需要 RTL 代码作者自行实现, 顶层只给出控制信号. `UARTIO` 用于 uart 输入输出的处理.

请注意: 这些端口必须出现在仿真顶层中, 但是 RTL 代码可以选择忽略这些信号.

Chisel 仿真顶层实现参考, 注意 `clock` 和 `reset` 会由 Chisel 自动生成:

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

Verilog 仿真顶层实现参考:

```v
module SimTop(
  input         clock,
  input         reset,
  input  [63:0] io_logCtrl_log_begin,
  input  [63:0] io_logCtrl_log_end,
  input  [63:0] io_logCtrl_log_level,
  input         io_perfInfo_clean,
  input         io_perfInfo_dump,
  output        io_uart_out_valid,
  output [7:0]  io_uart_out_ch,
  output        io_uart_in_valid,
  input  [7:0]  io_uart_in_ch
  // ......
);
```

# 使用 difftest 进行协同仿真

在完成上述所有工作之后, 我们就完成了协同仿真框架的接入.

在 difftest 目录下, 执行 `make emu` 来使用 verilator 生成 C++ 仿真程序. 仿真程序生成之后, 可以执行 `./build/emu --help` 来查看仿真程序的运行参数.

例如:

```bash
make emu
./build/emu -b 0 -e 0 -i ./ready-to-run/coremark-2-iteration.bin
```
