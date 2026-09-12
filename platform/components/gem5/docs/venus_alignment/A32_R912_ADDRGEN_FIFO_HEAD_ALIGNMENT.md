# A32 R912 addrgen FIFO-head admission 对齐报告

日期：2026-08-15  
结论：**INCONCLUSIVE**。本轮补齐了一处可由 RTL 结构支持的 LSU addrgen admission 边界，并推进 SCH task5 的逐 VINS 首差；这不代表全模型完备，也不以总 DAG 误差接近零作为验收条件。

## 1. 本轮定位

R911 后，SCH task5 的 sequence 0--8 已与 RTL 完全一致。首差位于 sequence 9 的 VLOAD：Gem5 相对 RTL 的 fire/duration/recycle 为 `+9/-9/0 cycles`。这组特征说明 load 完成时间并没有整体后移，而是 operand/descriptor admission 过晚后，执行 duration 被相应压短；继续修改 VLOAD 固定 latency 不符合证据。

RTL 的 relevant boundary 分为两部分：

1. 8-entry W CDC source FIFO 发生 full backpressure 时，VSTU 的 local WLAST 会在 source slot 返回的边界完成；
2. 同一边界可 pop addrgen direction head，并向 4-entry descriptor FIFO push 一个相反方向 descriptor；但是 LDU data path、AR publication 和完成仍须等待后续 direction release。

因此，descriptor admission 与实际 `lsu_request`/fixed response latency 必须拆开建模。

## 2. 实现

在 `VenusSequencer` 中加入了三个通用状态：WLAST pop tick、单次有效的 pop/push credit，以及被提前 admission 的 addrgen FIFO head。

- 只有真实 `sourceBackpressured` 的 WLAST 产生一次 early-admission credit；未阻塞的短 store/load 循环仍走普通 registered direction boundary。
- early admission 只发布 addrgen ack，不发布 `lsu_request`，不修改 LSU/VFU 固定 latency。
- 被 admission 的相反方向 descriptor 成为 FIFO head；任何 younger request 即使与旧 data-path direction 相同也不能旁路。
- 没有 task、DAG、PC、sequence ordinal、地址、payload 或 burst-length 特判。

FIFO-head 约束是必要的：仅加入 early admission 时，CCH task14 的 younger store 会绕过已经 admission 的 load，而该 store 又 RAW 依赖此 load，形成循环等待。保持 descriptor FIFO 顺序后，task14 恢复 115/115 VINS 完成。

源码位置：

- `src/venus/VenusSequencer.hh`：addrgen pop/push 与 admitted-head 状态；
- `src/venus/VenusSequencer.cc`：FIFO-head blocking、opposite descriptor early ack、真正 data-path grant 时清除 head，以及 backpressured WLAST credit 生成。

最终 binary SHA256：`04e1e98641ed40ebef3b3866041ff9a01441d2cccc4063c968bfe7c4b6e928e8`。

## 3. 被否决的候选

| 候选 | 局部收益 | held-out 反例 | 结论 |
|---|---:|---:|---|
| 所有 WLAST 均提前 admission | SCH task5 `+45 -> +29 ns` | task8 `+21 -> -499 ns`，31-byte 短 store/load 循环逐次错误累计 | 否决 |
| 只限 full-backpressured WLAST | task5 保留收益，task8 恢复 | CCH task14 仅完成 5 条 VINS 后停滞 | 不完整 |
| 单次 credit，无 FIFO-head 顺序 | 防止重复消费 credit | task14 仍发生 load/store RAW 环 | 不完整 |
| 单次 credit + admitted FIFO head | task5 首差推进 | CCH、LSU、CPU、VFU held-out 均通过 | 接受 |

## 4. SCH / PDSCHDag2

边界为 `tile_start -> task_epilogue`，排除 return DMA 和 tile release。

| task | Gem5 - RTL | 误差 |
|---:|---:|---:|
| 0 | +13 ns | +0.001922% |
| 1 | +13 ns | +0.016428% |
| 2 | +1 ns | +0.001811% |
| 3 | -11 ns | -0.002333% |
| 4 | -11 ns | -0.036454% |
| 5 | +29 ns | +0.262230% |
| 6 | -3 ns | -0.079051% |
| 7 | +1 ns | +0.094429% |
| 8 | +21 ns | +0.006345% |

绝对 task delta 总和由 R911 的 119 ns 降到 103 ns。只有 task5 改变：`+45 -> +29 ns`。

task5 sequence 0--8 的 fire/duration/recycle 继续全部 exact；sequence 9 从 R911 的 `+9/-9/0 cycles` 收敛为 `+1/-1/0 cycles`。sequence 10 当前为 `+1/+2/+3 cycles`，说明下一首要断点已经转向后续 sequencing/retirement，而不是 VLOAD 固定 latency。

SCH execution envelope 为 Gem5 1,533,896 ns、RTL 1,534,187 ns，即 `-291 ns / -0.018968%`。R911 为 -275 ns；包络略微变差是 task5 提前后改变依赖路径的结果，不能用它否定已经由逐 VINS 和 held-out 支持的结构修正，也不能用包络抵消宣称模型完备。

## 5. CCH / nrPDCCH

| task | Gem5 - RTL | task | Gem5 - RTL |
|---:|---:|---:|---:|
| 0 | -7 ns | 12 | -3 ns |
| 1 | -7 ns | 13 | -3 ns |
| 2 | -7 ns | 14 | -19 ns |
| 3 | -247 ns | 15 | -11 ns |
| 4 | -3 ns | 16 | -3 ns |
| 5 | -3 ns | 17 | -3 ns |
| 6 | -3 ns | 18 | +1 ns |
| 7 | -3 ns | 19 | +1 ns |
| 8 | -3 ns | 20 | -3 ns |
| 9 | -131 ns | 21 | +5 ns |
| 10 | -3 ns | 22 | +1 ns |
| 11 | -3 ns | 23 | -3 ns |

24 个 task 相对 R911 全部不变，绝对 task delta 总和仍为 476 ns。execution envelope 为 Gem5 1,041,340 ns、RTL 1,042,151 ns，即 `-811 ns / -0.077820%`。task14 的 focused replay 完成 115/115 VINS，证明本轮修复没有借助 task5 特判，也没有破坏其 held-out LSU/RAW 序列。

## 6. 正确性与回归门禁

- SCH：22/22 task outputs byte-exact；8,103/8,103 VINS result dumps byte-exact。
- CCH：53/53 task outputs byte-exact；10,253/10,253 VINS result dumps byte-exact。
- LSU：68/68 cases 通过；382/382 VINS exact。
- CPU：52/52 cases 执行；冻结 oracle 51/52，通过项含 1,258/1,258 retire 与 1,258/1,258 writeback exact；唯一排除仍是既有 `ecall_nop` fixture。
- VFU：BitALU、CAU、SerDiv 的 result queue 均实际达到 depth=2/full；1/4/64 ns grant-gap 输出均与 control exact。
- `git diff --check` 通过。

永久证据位于 `evidence/a32_r912_addrgen_fifo_head_20260815/`。

## 7. 下一断点

1. SCH task5：先拆 sequence 10 的 result enqueue、main sequencer registered return 与 retirement；sequence 9 已接近 exact，禁止再改公共 VLOAD latency。随后检查 sequence 15--17 的重复组是否累积同类差异。
2. CCH：task3 `-247 ns`、task9 `-131 ns` 仍占绝对差距主体，应分别定位首个 scalar retire 或 VINS lifecycle divergence；不能加公共 tail latency。
3. requester/VRF：继续核对 stable tagged requester vector、LSU 独立高优先级与 persistent per-bank RR，尤其以 task20 的内部 lifecycle 为证据，而不是其当前 `-3 ns` 总 task 差。

在这些内部边界逐一 exact 前，整体结论维持 **INCONCLUSIVE**。
