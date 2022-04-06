spikedasm: Disassemble Instructions Using Spike
========================

When difftest reports an error, call skipe-dasm to disassemble instructions in Commit Instruction Trace. For example:

```
============== Commit Instr Trace ==============
commit inst [00]: pc 000005aba6 inst 00098593 wen 1 dst 0000000b data 0000003fff865fc8 mv      a1, s3
commit inst [01]: pc 000005aba8 inst 0007b783 wen 1 dst 0000000f data 0000000000022738 ld      a5, 0(a5)
......
commit inst [21]: pc 0000022760 inst ef1ff0ef wen 1 dst 00000001 data 0000000000022764 jal     pc - 0x110
commit inst [22]: pc 0000022650 inst 00853687 wen 0 dst 0000000d data 000000000005ac5e fld     fa3, 8(a0)
commit inst [23]: pc 0000022652 inst 0085b707 wen 0 dst 0000000e data 0000000000022764 fld     fa4, 8(a1) <--
commit inst [24]: pc 000005ab9a inst 00098513 wen 1 dst 0000000a data 0000003fff865fc8 mv      a0, s3
```

## Usage

This plugin is a warpper of skipe-dasm command. It is enabled by default, add `spike-dasm` to `PATH` to enable it.