# Venus agent 高速仿真模式

## 目标

`fast` 是观测裁剪模式，不是近似时序模型。它不跳过 gem5 event，不改变 CPU、LSU、VFU、VRF bank 仲裁、scheduler/DMA 或 clock-domain 状态，只关闭两类不会反馈到硬件状态的昂贵产物：

- 每条 VINS 一个文本结果文件；
- 退出时写出的完整 `venusgem5_sequencer_monitor.json`。

以下产物仍保留，足够 agent 做功能和性能搜索：

- `venus_dag_trace.jsonl`，包括 tile/task lifecycle 与 task timing；
- `task_<id>_port_<id>.bin` 最终输出；
- `m5out/stats.txt`；
- DAG manifest 和 shared-L2 materialization。

## 使用

直接运行 manifest：

```bash
VENUS_GEM5_EXPERIMENTAL_VRF_RR=1 \
VENUS_GEM5_EXPERIMENTAL_REQUESTER_Q_VISIBILITY=1 \
build/RISCV/gem5.opt \
  --outdir=<run-dir>/m5out \
  configs/tutorial/part1/packet_gen.py \
  --dag-manifest=<run-dir>/venus_dag_manifest.json \
  --venus-config=venus-rtl-16x128 \
  --venus-sim-mode=fast
```

通过 `tools/venus_dag.py` 时使用 `--sim-mode fast`。未显式传 `--gem5` 时，wrapper 会自动选择 `build/RISCV/gem5.opt`；verification 模式仍默认选择 `gem5.debug`。

```bash
python3 tools/venus_dag.py <dag.json> run-dag-inprocess \
  --case-dir <case-dir> \
  --run-dir <run-dir> \
  --sim-mode fast
```

需要逐 VINS 定位时改为 `--venus-sim-mode=verification`。`gem5.opt` 同样支持 verification，而且已验证逐 VINS exact。

## 实测性能

同一台 PowerEdge R740、相同 DAG 和模型参数：

| workload / 模式 | hostSeconds | 相对 debug verification |
|---|---:|---:|
| SCH debug verification | 87.59 s | 1.00x |
| SCH debug fast | 82.70 s | 1.06x |
| SCH opt verification | 9.75 s | 8.98x |
| SCH opt fast | 8.84 s | 9.91x |
| CCH frozen debug verification | 161.28 s | 1.00x |
| CCH opt verification | 16.15 s | 9.99x |
| CCH opt fast | 14.48 s | 11.14x |

关闭观测本身只贡献约 6%--11%；主要加速来自同源码 `gem5.opt`。fast 模式的长期价值是避免 agent 大量迭代时创建上万个文件：SCH 产物由 43 MB 降到 5.2 MB，CCH 由 56 MB 降到 8.0 MB，并显著减少 inode 压力。

## 等价性门禁

- SCH 四种模式的 `simTicks=1,534,796,000`、`simInsts=247,189`，DAG trace SHA256 完全一致；22/22 outputs byte-exact。
- CCH opt verification/fast 的 `simTicks=1,042,424,000`、`simInsts=167,398`，DAG trace 与冻结 debug baseline byte-exact；53/53 outputs byte-exact。
- `gem5.opt verification` 对 debug：SCH 8,103/8,103、CCH 10,253/10,253 VINS exact。
- LSU 68/68、CPU 52/52、冻结 CPU oracle 1,258 retire/writeback exact、VFU capacity 均通过。

永久证据：`evidence/a32_agent_fast_mode_20260816/`。

## 推荐给 agent 的两级工作流

1. 搜索/优化内循环：`gem5.opt + fast`，解析最终 outputs、DAG trace 和 stats；SCH/CCH 合计约 23 秒。
2. 候选晋级门：`gem5.opt + verification`，保留全部逐 VINS 证据；SCH/CCH 合计约 26 秒。

不要通过增大 scheduler poll interval、合并 tile cycles、关闭 bank arbitration 或改变 fixed latency 提速；这些会降低时序精度。
