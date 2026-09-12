# A32 R917 tagged RAR requester 对齐报告

日期：2026-08-16  
性能结论：**当前 CCH/SCH workload 已达到有条件的工业性能估算精度**。  
完备性结论：**INCONCLUSIVE**。逐 VINS lifecycle 尚未全部 exact，不能将当前 DAG/task 误差外推为全模型完备。

## 1. 本轮根因与实现

R912 后，SCH task5 sequence 10 的 CAU/VSEQ 组仍比 RTL 晚 3 cycles recycle。逐拍拆解发现：它依赖较老的 sequence 7 VSTORE，二者都读取 v16。RTL source hazard vector 保留 RAR 依赖；Gem5 的 allocation-time producer tag 只保留 RAW/WAR/WAW。

LSU 不进入 lane input register，因此缺少 RAR tag 时，lane 会从已复用 running ID 的旧 metadata 重建 producer class，把该 VSTORE 误认成旧 CAU，错过 tagged LSU completion，直到三拍后的全局 hazard broadcast 才清除。

本轮在 `VenusSequencer` allocation boundary 做通用修正：

- requester dependency 同时覆盖 RAR、RAW、WAR、WAW；
- instruction-local snapshot 同时保存 producer generation 与 producer class；
- RAR 只控制 operand requester admission，不生成 RAW data-chain credit；
- LSU completion 仍走既有 tagged sideband；
- 没有修改 LSU/VFU fixed latency，也没有 task、DAG、PC、ordinal、地址或 burst 特判。

最终 binary SHA256：`25c6adc0ac84ae47282b5d3ab608a19958ea0fd4e80338bc373d3768892cb203`。

## 2. SCH / PDSCHDag2

边界为 `tile_start -> task_epilogue`，排除 return DMA 和 tile release。

| task | Gem5 - RTL | 误差 |
|---:|---:|---:|
| 0 | +13 ns | +0.001922% |
| 1 | +13 ns | +0.016428% |
| 2 | +1 ns | +0.001811% |
| 3 | -11 ns | -0.002333% |
| 4 | -11 ns | -0.036454% |
| 5 | +5 ns | +0.045212% |
| 6 | -3 ns | -0.079051% |
| 7 | +1 ns | +0.094429% |
| 8 | +21 ns | +0.006345% |

task5 从 `+29 ns` 收敛到 `+5 ns`；其余 8 个 task 完全不变。绝对 task delta 总和从 103 ns 降到 79 ns。

sequence 9 和 10 的 fire/duration/recycle 均为 `+1/-1/0 cycles`；sequence 10 在 R912 是 `+1/+2/+3`。首个 recycle divergence 已推进到 sequence 34 的 `+1 cycle`，最终 sequence 39 recycle 为 `+2 cycles`。

由于 task5 位于依赖链上，SCH execution envelope 从 R912 的 `-291 ns` 变为 `-315 ns / -0.020532%`。局部生命周期更准确而总包络稍远离零，再次证明不能按 DAG 抵消验收。

## 3. CCH / nrPDCCH

| task | Gem5 - RTL | 误差 | task | Gem5 - RTL | 误差 |
|---:|---:|---:|---:|---:|---:|
| 0 | -7 ns | -1.279707% | 12 | -3 ns | -0.800000% |
| 1 | -7 ns | -0.065476% | 13 | -3 ns | -0.532860% |
| 2 | -7 ns | -1.279707% | 14 | -19 ns | -0.009370% |
| 3 | -247 ns | -0.354727% | 15 | -11 ns | -0.170728% |
| 4 | -3 ns | -0.074645% | 16 | -3 ns | -0.223380% |
| 5 | -3 ns | -0.335196% | 17 | -3 ns | -0.009045% |
| 6 | -3 ns | -0.564972% | 18 | +1 ns | +0.000811% |
| 7 | -3 ns | -0.335196% | 19 | +1 ns | +0.041000% |
| 8 | -3 ns | -0.564972% | 20 | -3 ns | -0.000753% |
| 9 | -131 ns | -0.082494% | 21 | +5 ns | +0.053746% |
| 10 | -3 ns | -1.016949% | 22 | +1 ns | +0.000999% |
| 11 | -3 ns | -0.800000% | 23 | -3 ns | -0.008836% |

24/24 task duration与 R912 完全一致，证明本轮 requester 修正没有扰动 CCH held-out。绝对 task delta 总和仍为 476 ns；execution envelope 仍为 `-811 ns / -0.077820%`。

百分比较大的 task0/task2/task10 都是 295--547 ns 的短 task，绝对差只有 3--7 ns。长 task 中主要残差是 task3 `-247 ns/-0.355%` 和 task9 `-131 ns/-0.082%`。

## 4. CCH 剩余差距来源

task3 的首差已经定位到 sequence 0 VLOAD：duration/recycle `+7/+7 cycles`；sequence 5 首次出现 fire `+9 cycles`。随后重复的 512-element VSHUFFLE 与末段 VLOAD 逐组使 Gem5 转为偏快，最终约提前 121 个 tile cycles，与 task 总差约 242 ns 同量级。

task9 的首个 duration/recycle 差在 sequence 2 VSTORE，为 `-16/-16 cycles`；首次 fire 差在 sequence 5，为 `+1 cycle`。它同样不是可由公共 LSU latency 修正的固定偏差。

RTL shared L2 slave 明确配置 `IS_RW_1CHN=1`，同一时间只能处理 read 或 write；当前 `VenusSharedL2` 只统一序列化 LSU read bursts 和 LSU write-data bursts，scheduler L1 DMA 仍通过独立 timing ports，未进入同一 AR/AW/W/R/B 占用状态。task3 诊断窗口也观察到首个 VLOAD request 附近存在 DMA pointer read/write。

但现有 `L2_DMA_trx.log` 只给 DMA 侧事务，缺少在 shared-slave 边界、带 master/ID 的 LSU 与 DMA 合并 AR/AW/W/R/B grant trace；而 Gem5 与 RTL 的 DMA phase 已不完全相同。现在直接加入固定优先级或延迟，会把未知仲裁拟合到 task3/task9，并高风险破坏 task14/task17/task20 与 SCH held-out。因此本轮没有提交该候选。

## 5. 正确性与回归门禁

- SCH：22/22 outputs 与 R912 的 RTL-exact baseline byte-exact；8,103/8,103 VINS exact。
- CCH：53/53 outputs 与 R912 的 RTL-exact baseline byte-exact；10,253/10,253 VINS exact。
- LSU：68/68 cases；382/382 VINS 对 R912 exact。
- CPU：52/52 cases 执行；冻结 RTL oracle 51/52，1,258/1,258 retire 与 writeback exact；唯一排除仍为既有 `ecall_nop` fixture。
- VFU：BitALU、CAU、SerDiv result queue 都实际达到 depth=2/full；1/4/64 ns grant-gap 功能输出 exact。
- `git diff --check` 通过。

永久证据位于 `evidence/a32_r917_tagged_rar_requester_20260816/`。

## 6. 工业可用性判断与下一证据门槛

就这两个已验证 DAG 的性能估算而言：SCH 每 task 最大绝对相对误差小于 0.1%；CCH 大于 1 us 的 task 最大为 0.355%，全 DAG envelope 为 0.078%，功能结果与全部可比 VINS 均 exact。可以作为当前 CCH/SCH 的周期级性能评估模型使用。

但以下用途仍不能宣称完备：任意新 DMA/LSU 并发相位、逐指令 cycle-exact 验证、共享 L2 仲裁压力预测。继续安全对齐需要新增 shared-slave 边界的合并握手 oracle：至少记录 cycle、master/AXI ID、AR/AW/W/R/B valid-ready、burst beat/last 与获胜请求。拿到该 trace 后，应实现统一的 DMA+LSU 单通道 slave state，再以 task3/task9 为直接样本、task14/task17/task20 和 SCH 为 held-out 门禁。
