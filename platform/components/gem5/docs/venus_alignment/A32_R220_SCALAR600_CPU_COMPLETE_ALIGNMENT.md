# A32 R220 scalar600 CPU 逐周期对齐

日期：2026-08-11

状态：**已覆盖的 scalar600 CPU 行为面逐周期 exact；全 DAG 仍未对齐。**

这里的“CPU 对齐”是指 52 个直接由 RTL 仿真产生 oracle 的定向 case 中，
PC/指令类别/相对退休周期和寄存器写回全部一致。它不是对全部 `2^32` 个
指令字、所有异步 reset/interrupt 相位的形式等价证明，也不把
CPU--Sequencer、VFU、VRF 或 vector LSU 仲裁误算成普通标量流水行为。

## 本轮闭合的 CPU 合同

除此前已闭合的 IF/ID/EX/WB、ALU、MUL、DIV、分支、标量 LSU、LR/SC、
FENCE、CSR counter 和标准 WFI 外，本轮直接审计了 RTL decoder 的默认行为：

- OP-IMM 的 reserved shift 编码：SLLI 忽略 `funct7`；右移仅以
  `funct7==0` 选择 SRLI，否则选择 SRAI。
- OP 的 reserved `funct7`：`funct7==1` 选择 RV32M；ADD/SUB、SRL/SRA
  按 RTL 的宽松选择，其余基础 ALU 子操作忽略 `funct7`。
- CSR 只实现 RTL 的 CSRRS/read-only 行为，并复现 `csr[11:1]` 比较造成的
  相邻奇地址 alias；`rs1` 不参与写 CSR。
- JALR 忽略 `funct3`，target 不清 bit 0；ID redirect 仍保持单次三拍 flush
  合同，不在 Execute 再制造第二次 redirect。
- WFI 在 SYSTEM `funct3` 解码前，仅以 opcode 和 `imm[11:0]` 选择；标准和
  宽松编码都进入同一 HALTED/registered-wake 路径。
- LUI/AUIPC、非法 LOAD/STORE/BRANCH 保留 RTL decoder 的 operand-read
  enable，因此其“无用”源寄存器也会触发真实的 registered load-use hold。
- load-use 相等比较发生在排除 x0 之前；load `rd=x0` 后的 decoded x0 read
  同样覆盖并复现该一拍边界。
- 未连接 trap 的 ECALL、unsupported SYSTEM、非法 opcode、零字、非法
  sub-op 分别按 RTL 的 no-write NOP、write-zero 或 filtered bubble 行为处理。

实现中没有 task id、PC、DAG 名、workload、operand value 或 payload 特判。
固定的地址只用于 scalar600 已有的体系结构内存映射和 task-done MMIO，不是
性能 case 匹配。

## RTL oracle 结果

最终 suite 可由以下两个工具一键生成和运行：

```sh
python3 tools/venus_scalar_cpu_suite.py --output-dir <suite>
python3 tools/run_scalar_cpu_gem5_suite.py \
  --suite <suite>/suite.json --output-dir <results> --jobs 4
```

WFI 的外部唤醒现在写在 case contract 的
`external_events.wfi_wake_tick=40000` 中，runner 自动传入刺激，不再需要
手工拆跑。最终实际结果为：

- case：`52/52` 通过；
- 有序相对退休周期：`1,275/1,275` exact，容差 `0 cycle`；
- 寄存器写回：`1,275/1,275` exact；
- 允许的唯一 Gem5-only 尾项是 test/scheduler completion 的 EBREAK；
- Gem5 binary SHA-256：
  `ffc45596e1e65988c59e1288aaea81dd86ed332417573b4223325a970ef172a5`。

52 个 case 覆盖普通/RAW ALU、RV32I immediate/branch matrix、JAL/JALR、
control transition、连续和交错 MUL/DIV、除零和历史状态、所有标量 LSU
宽度/非对齐/RAW transition、LR/SC monitor、CSR counter/stall、FENCE、
ECALL/非法默认、decoder 宽松编码、假依赖/x0 依赖及 WFI sleep/wake。

## 回归门

- LSU RAW/throughput/capacity：`68/68` 通过。
- 与接受的 R206 LSU 功能基线比较：`382` 个 VINS 文件 byte-exact。
- CCH 当前 run：`10,253` 条 task-local ordinal VINS 与接受基线 exact。
- SCH 当前 run：`8,103` 条 task-local ordinal VINS 与接受基线 exact。

后两项是对接受 Gem5 功能基线的回归，不冒充新的全量 RTL VINS 比较。
PDSCH 历史保留的直接 RTL dump 仍只有 `7,256` 条，另 `847` 条没有对应
RTL dump。

## 当前 CCH/SCH timing

比较边界统一为 RTL `start execute -> execute complete` 对 Gem5
`tile_start -> task_epilogue`；return DMA/tile release 不混入单 task 百分比。

### CCH / nrPDCCH

allocation-to-last-release：Gem5 `1,027.996 us`，RTL `1,043.224 us`，
差 `-15.228 us`（`-1.460%`）。这个总量主要被 task20 的负误差支配，不能
作为对齐接受条件。

| task | Gem5 - RTL | error |
|---:|---:|---:|
| 0 | -0.009 us | -1.645% |
| 1 | +0.211 us | +1.974% |
| 2 | -0.009 us | -1.645% |
| 3 | -2.195 us | -3.152% |
| 4 | -0.007 us | -0.174% |
| 5 | -0.009 us | -1.006% |
| 6 | -0.007 us | -1.318% |
| 7 | -0.009 us | -1.006% |
| 8 | -0.007 us | -1.318% |
| 9 | +0.067 us | +0.042% |
| 10 | -0.007 us | -2.373% |
| 11 | -0.007 us | -1.867% |
| 12 | -0.007 us | -1.867% |
| 13 | +0.049 us | +8.703% |
| 14 | +1.485 us | +0.732% |
| 15 | -0.049 us | -0.761% |
| 16 | +0.001 us | +0.074% |
| 17 | +1.141 us | +3.440% |
| 18 | +3.465 us | +2.812% |
| 19 | +0.005 us | +0.205% |
| 20 | -19.729 us | -4.951% |
| 21 | +0.131 us | +1.408% |
| 22 | +0.005 us | +0.005% |
| 23 | -0.007 us | -0.021% |

相对 R212，只有 task14 增加 `4 ns`；它的 `tile_start` 不变，
`task_epilogue` 晚两个 tile cycles。随后 task17/task20 的绝对起止均整体
平移 `4 ns`，但各自 execution duration 不变。其余 task timing 完全相同。

### SCH / PDSCHDag2

allocation-to-last-release：Gem5 `1,538.420 us`，RTL `1,535.100 us`，
差 `+3.320 us`（`+0.216%`）。

| task | Gem5 - RTL | error |
|---:|---:|---:|
| 0 | +0.017 us | +0.003% |
| 1 | +0.013 us | +0.016% |
| 2 | +2.277 us | +4.123% |
| 3 | +7.085 us | +1.502% |
| 4 | +1.423 us | +4.716% |
| 5 | -0.721 us | -6.520% |
| 6 | +0.015 us | +0.395% |
| 7 | -0.003 us | -0.283% |
| 8 | -4.123 us | -1.246% |

SCH 所有 task 与 R212 逐项相同。

## 结论与下一断点

普通 scalar600 核心不再是当前 task 误差的首要来源：覆盖到的每条标量
退休和写回都已直接对 RTL exact，decoder 的宽松/非法行为及 false hazard
也已包含。当前不能把“CPU directed surface exact”扩大成“全系统完全对齐”。

下一步继续拆独立结构边界：

1. CCH task20：stable tagged requester vector、vector LSU 独立高优先级、
   persistent per-bank RR 和一次性 tagged grant capture；
2. CCH task17/task18：operand admission、result enqueue、逐 bank VRF grant
   与 retirement/recycle；
3. SCH task2/3/4/5/8：按 sequence fire/duration/recycle 定位，禁止用正负
   task 误差或全 DAG 百分比抵消。

机器可读证据位于
`evidence/a32_r220_scalar600_cpu_20260811/`。
