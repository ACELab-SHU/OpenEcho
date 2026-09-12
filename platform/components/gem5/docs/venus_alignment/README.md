# Venus RTL—gem5 对齐基线

本目录只记录由源码、可复现输入和实际运行共同支持的结论。它不是
“已全量对齐”的声明，也不替代新鲜 RTL 仿真。

## 最新多 DAG 断点

- `A32_R908_CPU_DIV_INACTIVE_LANE_RAW_ALIGNMENT.md`：将独立 load-to-DIV
  的 170-cycle replay 放入 divider pending-ready D/Q，SCH task2 标量边界
  205/205 cycles；CCH task14 从 -1003 ns 收敛到 +17 ns。新增 lane-local
  accepted-generation tag 后，仅未参与短 producer 的 lane 可消费 tagged
  all-lane completion，task2 三处 VRANGE 81->78 cycles，841/841 VINS
  identity/fire/duration/recycle 与 RTL exact。task2 总差 -9 ns 已分解为
  entry +2、vector 0、epilogue -11 ns；task20 保持 -13 ns。双 DAG、LSU、
  CPU 和三类 VFU 容量回归通过，但其余 task 残差仍使结论为 INCONCLUSIVE。
- `A32_R905_SCH_SERDIV_SHAMT_ALIGNMENT.md`：SCH 优先轮次。修复 taken-control
  target scalar LSU 同拍 launch，并用 direct packet oracle 证明 task3 VDIV
  `vfu_shamt=0`、task4 VDIV `vfu_shamt=6`。SerDiv timing 现按 RTL 在 32-bit
  域扩展/左移后做 LZC，不再用 operand FIFO 深度乘 arithmetic latency。
  task3/task4 收敛到 +221/+247 ns；同边界 SCH execution envelope 为
  +53 ns/+0.003455%，但 task2 -331 ns 和剩余 LSU lifecycle 仍独立存在。
  8,103 VINS、CPU/LSU、三类 VFU depth=2/full 回归通过。
- `A32_R814_CURRENT_CCH_SCH_AUDIT.md`：撤回会把 task17 误差搬到 task20
  的全局 LDU AXI 相位实验后，重新编译并新鲜运行双 DAG。当前 CCH 为
  +453 ns/+0.043468%，SCH 为 +5,817 ns/+0.379158%；逐 task 明细和误差
  归因已固化。CCH/SCH 10,253/8,103 条 VINS 对接受基线 byte-exact。
  task17/task20 的 RTL oracle 证明剩余 LDU 缺口是 AR CDC
  source-pointer setup/sampling aperture，而非统一固定 latency 或全局相位。
- `A32_R809_SHUFFLE_LIVE_INTENT_ALIGNMENT.md`：用 task20 sequence40 的逐边沿
  RTL oracle 证实 Shuffle 局部 RR 为 `LockIn=0`，LSU 压制期间必须以实时
  requester vector 重选 PE。该路径现随 RTL per-bank RR 自动启用；sequence
  40--49 lifecycle exact，首差推进到 sequence50。当前 CCH 全 DAG
  +165 ns/+0.015833%，SCH +5,761 ns/+0.375508%；逐 task 残差仍未闭环。
  CCH/SCH 10,253/8,103 VINS、LSU 68/68 与 382 VINS、三类 VFU depth=2/full
  回归通过。全同 VFU sequencer gate 实验导致首差倒退到 sequence31，已拒绝。
- `A32_R797_SHUFFLE_COMPLETION_Q_ALIGNMENT.md`：两个独立 RTL oracle 证明
  Shuffle final bank grant 后存在 complete-D/Q 到 hazard/requester 可见的
  寄存边界；gem5 现以 generation-tagged token 建模该边界。独立 task20
  sequence647/648 exact，首差推进到 sequence8834；CCH task20 从 -299 ns
  收敛到 -11 ns。CCH/SCH 10,253/8,103 VINS exact，LSU 68/68、382 VINS
  exact，三类 VFU depth=2/full。并发 task20 sequence0、task17 sequence1
  及 SCH task3/4/8 仍未闭环，不能宣称完全微时序对齐。
- `A32_R785_BITALU_FULL_ADMISSION_ALIGNMENT.md`：用 RTL result-queue full
  oracle 将 masked BitALU mask-D/A/B admission 统一门控；task20
  sequence608 lifecycle exact，首差推进到 sequence647 VSTORE。双 DAG
  功能和定向容量回归通过，但当时 task20 仍为 -299 ns。
- `A32_R648_BITALU_HANDOFF_FULL_DAG_AUDIT.md`：普通 BitALU A/B 读取改为
  tagged final-grant handoff，旧 response 可按 generation 排空；扩展普通
  outstanding depth 和统一 same-VFU hazard wait 的实验均因早期 sequence
  回退而拒绝。task17 687、task20 8,837、CCH 10,253、SCH 8,103、LSU
  382 条 VINS exact，LSU 68/68。CCH task20 仍 +9.201 us，SCH task3/task4
  仍 +5.501/+0.843 us；全 DAG 存在抵消，结论 INCONCLUSIVE。
- `A32_R327_VECTOR_LSU_PRODUCER_GRANT_ALIGNMENT.md`：非对齐 AXI beat、
  VSTU operand-ready 到 local-W，以及 active-lane tagged producer final
  grant 已按 RTL 边界建模。task17 sequence 0--100 lifecycle exact，首个
  分歧推进到 sequence 101 VXOR 快 1 cycle；CCH 10,253、SCH 8,103、LSU
  382 条 VINS exact，LSU suite 68/68。task20 仍有 sequence-2 +163-cycle
  标量缺口与后段反向漂移，结论仍为 INCONCLUSIVE。
- `A32_R260_SHUFFLE_STABLE_REQUEST_VECTOR_ALIGNMENT.md`：Shuffle blocked
  request 保存实际参与选择的 requester vector，并在 tagged grant 时用同一
  vector 更新 FairArb。CCH 10,253、SCH 8,103 VINS exact，LSU 68/68、
  382 VINS exact，三类 VFU result queue 均实际触发 depth=2/full。task17
  从 +491 ns 收敛到 +175 ns，但 task20 从 -6,847 ns 变为 -7,419 ns；首个
  bank mismatch 仍为 rel89，原因已定位到 Shuffle 内层 `LockIn=0` 在 LSU
  压制期间重仲裁 PE0/PE8，而 gem5 retry 固定旧 PE owner。两版 retry 时
  动态替换实验均已拒绝并回退，不能宣称 task20 已对齐。
- `A32_R253_BITALU_TAGGED_OVERLAP_ALIGNMENT.md`：用新采 RTL BitALU
  result-queue q/d、operand-ready 与 grant 波形实现同拍 enqueue/grant、grant
  后 admission 重评估及相同 latency 指令/旧 tagged result 重叠。task20
  BitAlu_B 0--60 拍 0 mismatch，逐 bank 首分歧推进到第 89 拍的 Shuffle；
  CCH/SCH VINS exact、LSU 68/68、VFU depth=2/full 六档通过。task20 完整
  duration 仍为 -6.847 us，不能宣称 task 已对齐。
- `A32_R220_SCALAR600_CPU_COMPLETE_ALIGNMENT.md`：52/52 RTL-oracle CPU
  case、1,275/1,275 条退休周期和写回在 0-cycle 容差下 exact；包含
  decoder 宽松/非法编码、false load-use hazard、x0 依赖和两类 WFI
  sleep/wake。该结论只覆盖 scalar600 CPU directed surface；CCH/SCH
  task 微时序仍有独立 vector admission/VRF/LSU 仲裁缺口。
- `A32_R53_REQUESTER_RETIREMENT_ALIGNMENT.md`：实现 VRANGE 真实 operand
  contract，定位并移除 VBRDCST retirement 重复 writer；requester-q/RR
  原型对齐 task20 首个 blocked/grant edge 并通过双 DAG 功能回归，但
  四微秒窗口仍为 99 对 RTL 37 个真实 blocked edge，因此原型不晋升，
  下一步拆 stable bank-edge intent 与 LSU high priority。
- `A32_R52_VFU_EDGE_ALIGNMENT.md`：对 nrPDCCH task17/task20 实采
  CAU enqueue/dequeue、VRF grant、retirement 及逐 bank RR winner；
  修正 registered queue capture 后的同 timestamp 组合 grant 相位。
  task20 剩余 first divergence 已定位为 LSU 高优先级和逐 bank RR
  状态，不能用 CAU latency 拟合。nrPDCCH、PDSCH held-out 和 LSU
  68/68 功能回归通过。
- `A32_R51_VFU_RESULT_QUEUES.md`：CAU/SerDiv 扩展为独立 depth=2
  tagged result queue，并用物理 VRF grant backpressure 实际触发
  occupancy=2/full；nrPDCCH、PDSCH held-out 和 LSU 68/68 通过。
- `A32_R50_BITALU_RESULT_QUEUE.md`：把 R49 的 BitALU generation hold
  替换为独立 depth=2 tagged result queue；task20 首条 VBRDCST recycle
  与 RTL 对齐到 task-start+500 ns，nrPDCCH、PDSCH held-out 和 LSU
  68/68 全通过。task17 与 result-queue full cliff 仍未收敛，不能宣称
  完全微时序对齐。
- `A32_R49_VFU_RESULT_GENERATION.md`：恢复 BitALU/CAU/SerDiv 的 tagged
  command admission，并修复 younger generation 清空 older BitALU tagged
  result 的 31/32 deadlock；nrPDCCH 24/24、PDSCH 9/9 和 LSU 68/68
  通过。nrPDCCH 全图归一化误差降到 -112 ns，但 task17 等局部误差仍
  明确保留，不能宣称完全微时序对齐。
- `A32_R48_NRPDCCH_ALIGNMENT.md`：nrPDCCH 24-task 的 shared-L2 输入、
  LSU 同拍 arbitration、DMT dynamic return 与四 tile 事件流修复；
  nrPDCCH 53/53 return ports 和 PDSCH 22/22 returns、8103/8103 VINS
  通过，同时保留 nrPDCCH task17 等未收敛微时序误差。
- `A32_R47_STRUCTURED_LSU_ALIGNMENT.md`：A47 structured LSU 的
  RAW-chain、throughput、capacity cliff 与 PDSCH held-out 基线。

## 当前闭环：PDSCH DMRS indices（16×128）

- 对象：`nrPDSCHDag2_hw_2p0/Task_nrPDSCHDMRSIndices`，而不是历史的
  `nrPDSCHDag2_hw` 15-task/64×512 descriptor。
- 输入：VEMU 生成的静态 DAG case；输入字节的 SHA-256 记录在单任务
  manifest 的 `input_provenance.json`。
- L0：VEMU 与 gem5 的 5 个真实 `vreturn` payload 均逐字节一致，长度为
  `1296, 6, 64, 64, 64`。
- L1：6 条 `VSADD` 的全部 648 个 `i16` 元素 dump 严格一致。
- L2/L3：未声明通过。当前没有该 R1 输入的可执行 RTL 结果，不能比较
  总时间或声明 RTL 功能通过。

## 时间基与双时钟结构

- Venus Sequencer、Lane、PacketGen 与 DAG Scheduler 都已改为按所属
  `ClockedObject` 的 cycle 调度；默认 `1GHz` R1 回归保持退出 tick、5 个
  返回、6 个 VSADD dump、DAG trace 与 monitor 全部逐字节不变。
- RTL testbench 的任务执行期结构为 tile `500MHz`（2ns）与 AXI/L2
  `250MHz`（4ns）。新增的
  `venus-rtl-16x128-dualclock-rebase` 仅把这个拓扑显式写入 gem5，并把
  原有以 ns 表示、意图为旧 GEM5 cycle 数的延迟二倍重标。它是诊断基线，
  不是 RTL 延迟测量或性能结果。
- 双时钟 R1 的功能输出仍与 VEMU 严格一致；CPU/tile 事件均落在 2ns
  边沿，Scheduler-owned trace 事件均落在 4ns 边沿。`task_epilogue` 是
  tile-origin 通知，保留在 tile 边沿，不能据此声称 CDC 已精确建模。

精确的复现参数和验收边界见 `timebase_validation.json`。

标准命令由以下工具组成：

```sh
python3 tools/venus_single_task_manifest.py ...
build/RISCV/gem5.debug configs/tutorial/part1/packet_gen.py \
  --dag-manifest <manifest> --venus-config=venus-rtl-16x128
python3 tools/compare_vemu_returns.py ...
python3 tools/compare_vins_outputs.py ... --match-by-id --strict-x
```

一旦获得同一 case 的 RTL L2-DMA return trace，可用
`tools/compare_dag_dma.py` 对 gem5 的动态 `vreturn` dump 比较。空的
`manifest.outputs` 表示运行时从真实 `vreturn` 读取地址和有效长度；比较器
会从实际 dump 推导长度，不会把 scheduler 容量误当成动态返回长度。

同源 RTL 单任务 fixture 已放在
`../venus_soc/sim/testbenches/postsyn_tilelevel_descriptor/PDSCHDag2_hw_2p0_r1_task1`。
它由 VEMU 的标准转换器生成 testbench 所需的内嵌输入/期望 DMA 数据，且
testbench 按 `Task_nrPDSCHDMRSIndices` 名称从完整 JSON 选择 task id 1。它不
会覆盖任何现有 descriptor、scheduler 或 `l1.bin`。

## 机器可读材料

- `venus_spec.json`：本轮已确认字段及来源；未知字段保持 unknown。
- `rtl_gem5_map.json`：RTL 源码到 gem5 状态的映射和验证级别。
- `trace_contract.json`：当前可比较事件与未来 RTL trace 所需字段。
- `timebase_validation.json`：时钟域、默认回归、双时钟 rebase 与相位
  检查的机器可读证据。

使用这些文件时必须保留 `evidence_kind`。`source-review` 或
`vemu-gem5-run` 不能升级为 `rtl-run`。
