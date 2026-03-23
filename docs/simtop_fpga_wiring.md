# SimTop FPGA 接线说明（C2H/H2C/AXI-Lite）

本文档用于把当前 DiffTest 的 `SimTop` FPGA 端口接入到另一套 SoC 工程。
目标是复用现有 HostEndpoint/H2CIntegration 逻辑，完成：
- C2H：DUT -> Host 的 difftest 数据上送
- H2C：Host -> DUT DDR 的 workload 下发
- AXI4-Lite Config BAR：H2C 控制与状态寄存器

## 1. 模块与职责

- `SimTop` 导出 FPGA 相关端口（`difftest_*`）
- `HostEndpoint` 负责 C2H 打包、H2C 接收、DDR 仲裁
- `H2CIntegration` 负责：
  - AXI-Lite 配置寄存器
  - H2C Stream -> AXI4 写 DDR
  - CPU/DDR 访存通路仲裁

参考源码：
- `difftest/src/main/scala/SimTop.scala`
- `difftest/src/main/scala/fpga/Host.scala`
- `difftest/src/main/scala/fpga/H2CIntegration.scala`
- `difftest/src/main/scala/fpga/HostAXI4LiteBar.scala`

## 2. 时钟域约束（最重要）

- `difftest_ref_clock`：HostEndpoint 主域（H2C 与 AXI-Lite 配置必须在这个域）
- `difftest_pcie_clock`：C2H AXI-Stream 发送域
- `difftest_clock_enable`：建议接到时钟门控使能，用于 H2C 占用 DDR 时暂停 CPU

建议接法：
- `cfg_*` AXI-Lite 和 `h2c_axis_*` 使用 `difftest_ref_clock` 域
- `to_host_axis_*` 使用 `difftest_pcie_clock` 域
- 若两域不同，必须加 CDC/异步桥

## 3. 必接端口清单（SimTop 扁平端口）

### 3.1 C2H（DUT -> Host）

- `difftest_to_host_axis_valid`（Output）
- `difftest_to_host_axis_ready`（Input）
- `difftest_to_host_axis_bits_data[511:0]`（Output）
- `difftest_to_host_axis_bits_last`（Output）

对接到 XDMA C2H 通道（AXI-Stream Slave 方向）。

### 3.2 H2C（Host -> DUT）

- `difftest_h2c_axis_valid`（Input）
- `difftest_h2c_axis_ready`（Output）
- `difftest_h2c_axis_bits_data[511:0]`（Input）
- `difftest_h2c_axis_bits_last`（Input）

对接到 XDMA H2C 通道（AXI-Stream Master 方向）。

### 3.3 AXI4-Lite Config BAR

写地址通道：
- `difftest_cfg_aw_valid`（Input）
- `difftest_cfg_aw_ready`（Output）
- `difftest_cfg_aw_bits_addr[31:0]`（Input）
- `difftest_cfg_aw_bits_prot[2:0]`（Input）

写数据通道：
- `difftest_cfg_w_valid`（Input）
- `difftest_cfg_w_ready`（Output）
- `difftest_cfg_w_bits_data[31:0]`（Input）
- `difftest_cfg_w_bits_strb[3:0]`（Input）

写响应通道：
- `difftest_cfg_b_valid`（Output）
- `difftest_cfg_b_ready`（Input）
- `difftest_cfg_b_bits_resp[1:0]`（Output）

读地址通道：
- `difftest_cfg_ar_valid`（Input）
- `difftest_cfg_ar_ready`（Output）
- `difftest_cfg_ar_bits_addr[31:0]`（Input）
- `difftest_cfg_ar_bits_prot[2:0]`（Input）

读数据通道：
- `difftest_cfg_r_valid`（Output）
- `difftest_cfg_r_ready`（Input）
- `difftest_cfg_r_bits_data[31:0]`（Output）
- `difftest_cfg_r_bits_resp[1:0]`（Output）

### 3.4 DDR 仲裁相关端口

- `difftest_cpu_mem_*`：CPU 原始访存 AXI4 输入
- `difftest_ddr_mem_*`：仲裁后的 DDR AXI4 输出

真实上板必须把这两组口接入你们 SoC 的 AXI/DDR 互联，不要像仿真桩那样 tie-off。

### 3.5 状态/控制输出

- `difftest_HOST_IO_RESET`
- `difftest_HOST_IO_DIFFTEST_ENABLE`
- `difftest_ddr_arb_sel`
- `difftest_h2c_active`
- `difftest_h2c_done`
- `difftest_h2c_beat_count[31:0]`
- `difftest_clock_enable`

## 4. AXI-Lite 寄存器映射

基地址为 XDMA user BAR 映射基址，偏移如下：

- `0x00` `HOST_IO_RESET` (RW)
- `0x04` `HOST_IO_DIFFTEST_ENABLE` (RW)
- `0x08` `DDR_ARB_SEL` (RW)
  - bit0: `0=CPU owns DDR`, `1=H2C owns DDR`
- `0x0C` `H2C_LENGTH` (RW)
  - 传输 beat 数（512bit/beat）
- `0x10` `H2C_STATUS` (RO)
  - bit1: `h2c_active`
  - bit2: `h2c_done`
- `0x14` `H2C_BEAT_CNT` (RO)

Host 侧常量定义可直接参考：
- `difftest/src/test/csrc/fpga/xdma.h`

## 5. 事务流程（上板 H2C 装载）

- 写 `H2C_LENGTH`
- 写 `DDR_ARB_SEL=1`，CPU 停止占用 DDR
- 从 H2C AXI-Stream 连续发送 payload（不足 64B 的末尾补零）
- 轮询 `H2C_STATUS/H2C_BEAT_CNT`，确认 done 且 beat 达到预期
- 写 `DDR_ARB_SEL=0` 恢复 CPU

## 6. 接线模板（与当前 top.v 一致）

当前工程已经把 `xdma_wrapper` 拆开为独立模块外接，建议复用这种方式：

```verilog
// 1) C2H 使用 pcie_clock 域
xdma_axi_c2h u_c2h (
  .clock(difftest_pcie_clock),
  .reset(reset),
  .axi_tvalid(difftest_to_host_axis_valid),
  .axi_tready(difftest_to_host_axis_ready),
  .axi_tdata (difftest_to_host_axis_bits_data),
  .axi_tlast (difftest_to_host_axis_bits_last)
);

// 2) H2C + AXI-Lite 使用 ref_clock 域（关键）
xdma_axi_h2c u_h2c (
  .clock(difftest_ref_clock),
  .reset(reset),
  .axi_tvalid(difftest_h2c_axis_valid),
  .axi_tready(difftest_h2c_axis_ready),
  .axi_tdata (difftest_h2c_axis_bits_data),
  .axi_tlast (difftest_h2c_axis_bits_last)
);

xdma_axilite u_cfg (
  .clock(difftest_ref_clock),
  .reset(reset),
  .awvalid(difftest_cfg_aw_valid),
  .awready(difftest_cfg_aw_ready),
  .awaddr (difftest_cfg_aw_bits_addr),
  .awprot (difftest_cfg_aw_bits_prot),
  .wvalid (difftest_cfg_w_valid),
  .wready (difftest_cfg_w_ready),
  .wdata  (difftest_cfg_w_bits_data),
  .wstrb  (difftest_cfg_w_bits_strb),
  .bvalid (difftest_cfg_b_valid),
  .bready (difftest_cfg_b_ready),
  .bresp  (difftest_cfg_b_bits_resp),
  .arvalid(difftest_cfg_ar_valid),
  .arready(difftest_cfg_ar_ready),
  .araddr (difftest_cfg_ar_bits_addr),
  .arprot (difftest_cfg_ar_bits_prot),
  .rvalid (difftest_cfg_r_valid),
  .rready (difftest_cfg_r_ready),
  .rdata  (difftest_cfg_r_bits_data),
  .rresp  (difftest_cfg_r_bits_resp)
);
```

## 7. 常见错误

- `cfg_*` 和 `h2c_axis_*` 接在 `pcie_clock` 域，导致寄存器写入和 H2C 状态机不同步
- `difftest_cpu_mem_* / difftest_ddr_mem_*` 未接真实互联，H2C 看似握手但 DDR 未真正写入
- 忽略 `difftest_clock_enable`，H2C 占用 DDR 时 CPU 仍在跑，产生仲裁冲突

