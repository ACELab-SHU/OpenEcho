+======================================================================
运行模式：Venus agent 无损高速仿真（2026-08-16）
+======================================================================

使用文档：

    docs/venus_agent_fast_mode.md

1. 新增 `--venus-sim-mode={verification,fast}`。fast 只关闭逐 VINS 文本结果和
   full sequencer monitor；DAG trace、task outputs、stats 与全部模拟事件保留。
2. 构建 `gem5.opt`；`tools/venus_dag.py --sim-mode fast` 未指定 binary 时自动
   选择 opt，verification 继续默认 debug。
3. SCH debug verification/fast、opt verification/fast 的 simTicks、simInsts、
   DAG trace 与 22/22 outputs 全部 exact；opt verification 的 8103 VINS exact。
4. CCH opt verification/fast 与冻结 debug baseline 的 DAG trace、53/53 outputs
   exact；opt verification 的 10253 VINS exact。
5. SCH 从 87.59 s 降至 8.84 s（9.91x），CCH 从 161.28 s 降至 14.48 s
   （11.14x）；两链路 agent 内循环约 23 秒。fast 产物分别为 5.2/8.0 MB。
6. LSU 68/68、CPU 52/52、1258 retire/writeback exact、VFU capacity 通过。
   本节是宿主机执行优化，不改变 R917 对齐结论或任何 RTL 时序参数。

永久证据：

    evidence/a32_agent_fast_mode_20260816/

+======================================================================
当前断点：A32 R917 tagged RAR requester generation（2026-08-16）
+======================================================================

完整报告：

    docs/venus_alignment/A32_R917_TAGGED_RAR_REQUESTER_ALIGNMENT.md

1. SCH task5 seq10 的 older producer 是读取同一 v16 的 VSTORE。RTL source
   hazard vector 包含 RAR；Gem5 allocation snapshot 只保留 RAW/WAR/WAW，
   使不进入 lane input 的 LSU generation 被旧 running-ID CAU metadata 覆盖，
   错过 tagged LSU completion，直到三拍后的 hazard broadcast 才清除。
2. 本轮让 requester dependency 同时覆盖 RAR/RAW/WAR/WAW，并稳定保存
   producer generation/class；RAR 不生成 data-chain credit。没有改变 LSU/VFU
   fixed latency，也没有 task/DAG/PC/ordinal/address/burst 特判。
3. SCH task5 `+29 -> +5 ns`，seq9/seq10 均为 `+1/-1/0 cycles`，首个 recycle
   divergence 推进到 seq34 `+1 cycle`；各 task 为
   `+13,+13,+1,-11,-11,+5,-3,+1,+21 ns`，绝对和 `103 -> 79 ns`。
4. CCH 24/24 task duration 均与 R912 不变；主要残差仍是 task3
   `-247 ns/-0.355%` 与 task9 `-131 ns/-0.082%`。短 task 最大相对误差
   `-1.280%` 只对应 7 ns。SCH 每 task 最大绝对相对误差 `0.0944%`。
5. SCH/CCH 22/22、53/53 outputs 与 8103/10253 VINS 对 R912 RTL-exact
   baseline byte-exact；LSU 68/68、382 VINS exact；CPU 52/52，oracle 51/52、
   1258 retire/writeback exact；BitALU/CAU/SerDiv depth=2/full。binary SHA256：
   `25c6adc0ac84ae47282b5d3ab608a19958ea0fd4e80338bc373d3768892cb203`。
6. 当前两个 workload 已达到有条件工业性能估算精度，但完备性仍为
   INCONCLUSIVE。task3/task9 下一结构缺口是 `IS_RW_1CHN=1` shared L2 中
   DMA+LSU 的统一仲裁；现有日志没有带 master/ID 的合并 AR/AW/W/R/B grant
   oracle，直接加固定延迟/优先级会变成拟合，因此停在证据边界。

永久证据：

    evidence/a32_r917_tagged_rar_requester_20260816/

+======================================================================
当前断点：A32 R912 addrgen FIFO-head admission（2026-08-15）
+======================================================================

完整报告：

    docs/venus_alignment/A32_R912_ADDRGEN_FIFO_HEAD_ALIGNMENT.md

1. R911 后 SCH task5 seq9 为 fire/duration/recycle `+9/-9/0 cycles`。
   本轮按 RTL 的 4-entry addrgen descriptor FIFO 补齐 full-backpressured
   WLAST 同沿 pop/push：descriptor 可先 admission，但 LDU data path 与
   `lsu_request` 仍等待 direction release；未改 LSU/VFU fixed latency。
2. early admission 只在真实 W-CDC source full backpressure 下产生一次 credit，
   且 admitted opposite descriptor 保持 FIFO head，禁止 younger 同方向 request
   旁路。没有 task/DAG/PC/ordinal/address/payload/burst-length 特判。
3. SCH task5 从 +45 降到 +29 ns，seq0--8 继续全部 exact，seq9 推进到
   `+1/-1/0 cycles`；绝对 task delta 总和 119 -> 103 ns。各 task 为
   `+13,+13,+1,-11,-11,+29,-3,+1,+21 ns`。
4. CCH 24 个 task 全部保持 R911：`-7,-7,-7,-247,-3,-3,-3,-3,-3,-131,
   -3,-3,-3,-3,-19,-11,-3,-3,+1,+1,-3,+5,+1,-3 ns`；task14 focused
   replay 115/115 VINS 完成，排除了 early-admission 引入 FIFO/RAW 环的退化。
5. SCH/CCH 22/22、53/53 outputs 与 8103/10253 VINS byte-exact；LSU
   68/68、382 VINS exact；CPU 52/52，oracle 51/52、1258 retire/1258
   writeback exact；BitALU/CAU/SerDiv depth=2/full。最终 binary SHA256：
   `04e1e98641ed40ebef3b3866041ff9a01441d2cccc4063c968bfe7c4b6e928e8`。
6. SCH/CCH envelope 为 -291 ns/-0.018968% 与 -811 ns/-0.077820%；前者比
   R911 的 -275 ns 略差，进一步证明总 DAG 抵消不能作为验收。结论仍为
   INCONCLUSIVE；下一步拆 task5 seq10 retirement、CCH task3/task9 首差，
   并继续 stable tagged requester、LSU 优先级和 persistent per-bank RR。

永久证据：

    evidence/a32_r912_addrgen_fifo_head_20260815/

+======================================================================
当前断点：A32 R911 addrgen queue / W-CDC direction boundary（2026-08-15）
+======================================================================

完整报告：

    docs/venus_alignment/A32_R911_ADDRGEN_CDC_DIRECTION_ALIGNMENT.md

1. R909 用 `sourceBackpressured ? 10 : 1` 折叠了 8-entry Gray W-CDC 的
   read-pointer return 与 4-entry addrgen descriptor FIFO。R911 改为真实
   `sourceNextReleaseTick + 6` 个 WLAST/direction 注册边界；只有先在同方向
   addrgen head 后排队的 descriptor 再付 2 个 FIFO pop/head D/Q 边界。
2. 没有 task/DAG/PC/ordinal/address/data/burst-length 特判，未改 LSU/VFU
   fixed latency。SCH task4 的 193 条 VINS normalized lifecycle 完全不变；
   task5 seq8 VLOAD 的 duration/recycle 从 +2/+2 cycles 收敛到 0/0，首差异
   推进到 seq9：fire +9、duration -9、recycle exact。
3. SCH 各 task delta 为 `+13,+13,+1,-11,-11,+45,-3,+1,+21 ns`；绝对
   task delta 总和 123 -> 119 ns。CCH 只有 held-out task14 改变，+25 ->
   -19 ns，其余 23 tasks 不变；绝对总和 482 -> 476 ns。
4. SCH/CCH execution envelope 分别为 -275 ns/-0.017925% 和
   -811 ns/-0.077820%，均比 R909 稍远离零。这正说明不能按总 DAG 或任务间
   抵消验收；本轮接受依据是 RTL 状态边界、逐 VINS 推进和 held-out 控制。
5. SCH/CCH 22/22、53/53 outputs 与 8103/10253 VINS byte-exact；LSU
   68/68、382 VINS exact；CPU 52/52，oracle 51/52、1258 retire/1258
   writeback exact；BitALU/CAU/SerDiv depth=2/full。最终 binary SHA256：
   `8bb5493512e3682ea7e3d8e9c46febc97a090e1fd62351398ac4548b36c22de0`。
6. 结论仍为 INCONCLUSIVE：下一步拆 SCH task5 seq9 的 addrgen admission、
   LDU queue admission 和 AR publication；CCH task3 -247 ns、task9 -131 ns
   仍需定位首个 scalar/VINS divergence，不能改全局 tail 或 fixed latency。

永久证据：

    evidence/a32_r911_addrgen_cdc_direction_20260815/

+======================================================================
当前断点：A32 R909 TASK_DONE tile-manager registered boundary（2026-08-15）
+======================================================================

完整报告：

    docs/venus_alignment/A32_R909_TASKDONE_TILE_MANAGER_ALIGNMENT.md

1. CCH/SCH 共 33 个 RTL task 证明 TASK_DONE request 到 scalar retire 固定
   2 ns，到 task complete 为 7/9/11 ns。旧 Gem5 在 LSQ request 创建时立即
   suspend，导致 TASK_DONE store response 被旧 stream 丢弃；这不是 RTL 行为。
2. 新模型为 `Running -> TaskDonePending -> Draining`，经过两个 250 MHz
   selected-clock 注册边界后，由 Minor Execute 在 CPU edge 上原子产生原生
   `SuspendThread` stream redirect。没有 task/DAG/PC/ordinal/address/data
   特判，未改 LSU/VFU fixed latency。
3. CCH 多数公共 `-13 ns` 收敛到 `-3 ns`；task17/task20 均为 -3 ns。
   16/24 tasks 在 3 ns 内、20/24 在 7 ns 内；绝对 task delta 总和从
   674 降到 482 ns。主要剩余 task3 -247 ns、task9 -131 ns、task14 +25 ns。
4. SCH 各 task delta 为 `+13,+13,+1,-11,-11,+49,-3,+1,+21 ns`；task2
   与 task7 到 +1 ns。绝对 task delta 总和从 131 降到 123 ns；task5 +49 ns
   仍需独立拆解，不能再加全局尾部延迟。
5. SCH/CCH 22/22、53/53 outputs 与 8103/10253 VINS byte-exact；CPU 52/52，
   oracle 51/52、1258 retire/1258 writeback exact；LSU 68/68、382 VINS exact；
   BitALU/CAU/SerDiv depth=2/full。最终 binary SHA256：
   `83036b069e406633f725acad38865f4222992a283d1a817a48234f7831632b41`。
6. 结论仍为 INCONCLUSIVE：下一步拆 SCH task5、CCH task3/task9 的首个内部
   divergence；task14 +25 ns 是拒绝继续增加公共 tail latency 的 held-out 门。

永久证据：

    evidence/a32_r909_taskdone_tile_manager_20260815/

+======================================================================
当前断点：A32 R908 CPU load-to-DIV / inactive-lane tagged RAW（2026-08-15）
+======================================================================

完整报告：

    docs/venus_alignment/A32_R908_CPU_DIV_INACTIVE_LANE_RAW_ALIGNMENT.md

1. SCH task2 标量首差定位为独立 `lbu -> remu`：RTL 205 cycles、gem5 35。
   额外 170 cycles 已放在 divider pending-ready D/Q，而不是通用 FU
   minimum-commit 的并行 max 路径；现为 205/205。CCH task14 的三处同类
   边界带来 1020 ns 改善，task14 从 -1003 ns 收敛到 +17 ns。
2. task2 seq45/324/603 的短 VXOR->VRANGE 暴露 inactive producer lane：
   lane4--7 未接收前序 producer，却多等 3 tile cycles 的 generation change。
   现用 per-running-ID accepted-generation tag，仅允许未参与 producer 的 lane
   消费 tagged all-lane completion；active lane、LSU、Shuffle 均不走此路径。
3. task2 841/841 VINS identity/fire/duration/recycle 已与 RTL 全部 exact；三处
   VRANGE 均由 81 降到 78 cycles。task2 总差为 -9 ns，分解为 entry +2 ns、
   vector 0 ns、epilogue -11 ns，下一步不能继续改 vector latency。
4. SCH 各 task delta：`+5,+3,-9,-19,-21,+39,-13,-9,+13 ns`。CCH 除 task14
   外相对 R906 不变：主要剩余 task3 -257 ns、task9 -141 ns；task20 保持
   -13 ns，证明没有重现全局 retirement 放宽的退化。
5. SCH/CCH 22/22、53/53 outputs 及 8103/10253 VINS byte-exact；LSU 68/68、
   382 VINS exact；BitALU/CAU/SerDiv depth=2/full 门通过。CPU 52/52 executes，
   frozen oracle 51/52，1258 retire + 1258 writeback exact，保留旧
   `ecall_nop` fixture exclusion。最终 binary SHA256：
   `052ef9883e6bc52a2658abab48a6400567987f34fb90b4cb5c05174948d47675`。
6. 结论仍为 INCONCLUSIVE：SCH task5 +39 ns、CCH task3/task9 与短 task
   -11..-15 ns 边界仍需逐模块拆解，不能用 DAG envelope 或百分比抵消验收。

永久证据：

    evidence/a32_r908_cpu_div_inactive_lane_20260815/

+======================================================================
当前断点：A32 R906 W-CDC FIFO / direction boundary（2026-08-15）
+======================================================================

完整报告：

    docs/venus_alignment/A32_R906_WCDC_DIRECTION_ALIGNMENT.md

1. 用 SCH task4 的 361 点 RTL W-channel oracle 将旧固定 12 AXI-cycle VSTU
   restart 替换为 8-entry beat-level W CDC FIFO、两级 read-pointer return、
   local/external WLAST 和仅在真实 full 时触发的长反向 release；未改 LSU/VFU
   固定 latency，也没有 task/PC/ordinal/address/payload 特判。
2. CCH task17 的 26 组单/跨 64-byte beat 对照证明：VSTORE completion 与反向
   VLOAD request 同沿时只需要一个 registered direction-select edge，后续
   104/106-cycle 差异由现有 2:1 CDC 相位自然产生。全局加两拍和额外 response
   一拍两个候选均被 held-out 反例否决并撤回。
3. task17 前 682 条指令 lifecycle 一致；首差异推进到 seq682 VLOAD，fire
   -1 cycle、duration +1 cycle、recycle aligned。task17 总差为 -13 ns
   (-0.039196%)，task20 保持 -13 ns (-0.003262%)。
4. SCH 各 task delta：`+5,+3,-331,-19,-21,+39,-13,-9,+13 ns`；task3/task4
   从 R905 的 +221/+247 ns 收敛到 -19/-21 ns；execution envelope 为
   RTL/Gem5 1,534,187/1,533,844 ns，-343 ns/-0.022357%。下一主目标是
   task2 -331 ns。
5. CCH 主要绝对残差：task14 -1003 ns、task3 -257 ns、task9 -141 ns；短
   task 的 -11..-15 ns 共性偏差不能按百分比用 vector latency 补偿。CCH
   execution envelope 为 RTL/Gem5 1,042,151/1,040,260 ns，-1,891 ns；不能
   用该总值掩盖 task14。
6. SCH 22/22 outputs、8103/8103 VINS；CCH 53/53 outputs、10253/10253 VINS
   对接受基线 byte-exact；LSU suite 68/68。最终 binary SHA256：
   `1fa5dc760c47ace7c178dc11c9e75c523e7b2ea9e737085494317e98056949a9`。

永久证据：

    evidence/a32_r906_wcdc_direction_20260815/

+======================================================================
当前断点：A32 R905 SCH scalar redirect LSU / SerDiv shamt（2026-08-15）
+======================================================================

完整报告：

    docs/venus_alignment/A32_R905_SCH_SERDIV_SHAMT_ALIGNMENT.md

1. 本轮只用 SCH direct oracle 调参，没有依赖 CCH 或全 DAG 抵消。task3
   taken `BLT -> LHU` 的 target LSU 同拍 launch 修复后，标量连续 3,643 条
   非 memory retire 与 RTL exact；task3 先由 +5.527 us 降到 +4.141 us。
2. SerDiv 旧模型错误地在 lane vector 超过四项 operand FIFO 时把逐 word
   arithmetic latency 乘二。RTL 的 operand FIFO、iterative divider 和
   depth-2 result FIFO 是独立边界，不存在该乘法。direct packet oracle 又
   证明 task3 VDIV `vfu_shamt=0`，task4 VDIV `vfu_shamt=6`；Gem5 timing
   现与 RTL 一样先在 32-bit `serdiv_t` 域符号/零扩展并左移，再做 LZC。
3. task3 sequence25/26 VDIV 从 191/355 cycles 收敛到 111/193，RTL 为
   109/192；task4 sequence8 为 556/556 exact，sequence9 为 1130/1131。
   task3/task4 总误差分别由 +4.141/+0.943 us 降到 +0.221/+0.247 us。
4. SCH 同边界 execution envelope：RTL 1,534.187 us、Gem5 1,534.240 us，
   `+0.053 us/+0.003455%`。历史 allocation-to-release Gem5 边界为
   1,535.108 us，对旧 RTL execution envelope 为 +0.921 us；不得混用边界。
5. 各 task 当前 delta：task0 +5 ns、task1 +3、task2 -331、task3 +221、
   task4 +247、task5 -75、task6 -13、task7 -9、task8 +13。总 envelope
   很小仍不能宣称完备；task3 最终 vector recycle +116 cycles，task4 +130。
6. SCH 22/22 returns、8,103/8,103 Gem5 VINS 对接受基线 byte-exact；直接
   RTL VINS 仍仅 7,256。CPU 52/52、1,258 retire + 1,258 writeback exact；
   LSU 68/68、382 VINS exact；BitALU/CAU/SerDiv depth=2/full 容量门通过。
   最终 binary SHA256：
   `3eda8e317384984bcd7305a2cb4b8c9908fab78c6b114428f439f698a0aebe7f`。
7. 下一 SCH 断点：task4/task3 重复 VSTORE/VLOAD 的 operand admission、
   addrgen、AXI beat/WLAST、per-bank VRF grant 和 retirement。task4 当前
   VSTORE +21/+27/+39/+41 cycles、VLOAD +31/+33；task2 -331 ns 与 task5
   -75 ns 必须独立处理，不能拿来抵消 task3/task4。

永久证据：

    evidence/a32_r905_sch_serdiv_shamt_20260815/

+======================================================================
当前断点：A32 R814 CCH/SCH fresh audit 与 LDU 相位反例（2026-08-14）
+======================================================================

完整报告：

    docs/venus_alignment/A32_R814_CURRENT_CCH_SCH_AUDIT.md

1. 已接受模型重新编译并新鲜运行：CCH RTL/gem5 为
   1,042,151/1,042,604 ns，`+453 ns/+0.043468%`；SCH 为
   1,534,187/1,540,004 ns，`+5,817 ns/+0.379158%`。逐 task 边界仍为 RTL
   `start execute -> execute complete` 对 gem5 `tile_start -> task_epilogue`。
2. CCH 主要独立残差为 task3 -251 ns、task14 +231 ns、task9 -111 ns、
   task17 -49 ns、task15 -35 ns。短 task 的 -7..-11 ns 共性残差属于
   scalar CPU/task boundary，不能据百分比去修改 vector 固定 latency。
3. SCH 主要残差为 task3 +5,489 ns、task4 +927 ns、task8 -1,091 ns、
   task2 -327 ns；分别继续拆 CAU/SerDiv/LSU overlap、SerDiv D/Q 与结果
   写回、Shuffle/LSU stable tagged requester 和 persistent per-bank RR。
4. task17 首条 VLOAD 的 RTL/gem5 duration 仍为 34/35 cycles。全局 LDU
   AXI 相位翻转虽使 task17 总误差从 -49 ns 到 +1 ns，却把 task20 首条
   VLOAD 从 RTL-exact 45 cycles 改成 44，并使 task20 从 -11 ns 退化到
   -257 ns；候选已否决撤回。缺口是 AR CDC source-pointer setup/sampling
   aperture，而非通用 latency 或统一相位常数。
5. 新鲜 CCH/SCH 10,253/8,103 VINS 对接受基线 byte-exact；SCH 直接 RTL
   dump 仍只覆盖 7,256，未把另 847 条冒充 RTL 对比。

永久证据：

    evidence/a32_r814_current_cch_sch_20260814/

+======================================================================
当前断点：A32 R809 Shuffle LockIn=0 live intent（2026-08-14）
+======================================================================

完整报告：

    docs/venus_alignment/A32_R809_SHUFFLE_LIVE_INTENT_ALIGNMENT.md

1. task20 sequence40 的逐边沿 RTL oracle 证明局部 `rr_arb_tree` 使用
   `LockIn=0`：LSU 拒绝本地请求期间 requester vector 改变，RTL 会重新选
   PE；gem5 旧 timing retry 固定旧 PE owner。generation-tagged live intent
   现随 RTL per-bank RR 自动启用，不含 task/PC/DAG/地址/数据特判，也未改
   LSU/VFU 固定 latency。
2. 独立 task20 sequence40--49 fire/duration/recycle 恢复 exact；首个 lifecycle
   分歧推进到 sequence50 VBRDCST，gem5 fire 早 154 ns、但 recycle 已相同。
   当前 replay 为 398,232 ns，8,837 VINS exact。全同 VFU sequencer gate
   候选使首差倒退到 sequence31、总时长恶化为 440,036 ns，已否决撤回。
3. CCH 24/24：全 DAG RTL 1,042,151 ns、gem5 1,042,316 ns，
   `+165 ns/+0.015833%`。主要独立残差为 task20 -299 ns、task14 +231 ns、
   task9 -111 ns、task3 +101 ns、task13 +55 ns、task17 -49 ns。task13 的
   +9.769% 来自 563 ns 短 task，不能只按百分比排序。
4. SCH 9/9：全 DAG RTL 1,534,187 ns、gem5 1,539,948 ns，
   `+5,761 ns/+0.375508%`。主要残差 task3 +5,433 ns、task8 -1,091 ns、
   task4 +927 ns、task2 -327 ns；仍分别指向 CAU/SerDiv/LSU 持续重叠、
   SerDiv D/Q/结果写回和 Shuffle/LSU persistent per-bank contention。
5. CCH/SCH 10,253/8,103 VINS 对接受基线 exact；SCH 直接 RTL dump 仍只覆盖
   7,256，未把另外 847 条冒充 RTL 比较。LSU 68/68、382 VINS exact；
   BitALU/CAU/SerDiv 均实际触发 depth=2/full。

永久证据：

    evidence/a32_r809_shuffle_live_intent_20260814/

+======================================================================
当前断点：A32 R806 CCH/SCH current audit（2026-08-14）
+======================================================================

完整报告：

    docs/venus_alignment/A32_R806_CURRENT_CCH_SCH_AUDIT.md

1. 当前二进制 SHA256 为 `6ffbc43dc11efbac07b6f0b2ae5b8c2810cf43f76dfc41e74403bc0acab69567`。
   CCH 24/24 tasks、10,253 VINS 对 R770 接受基线 exact；全 DAG 为 RTL
   1,042,151 ns、gem5 1,042,572 ns，`+421 ns/+0.040397%`。主要独立残差为
   task3 -251 ns、task14 +231 ns、task9 -111 ns、task15 -67 ns、task17
   -49 ns，不能用全 DAG 抵消验收。
2. SCH 9/9 tasks、8,103 VINS 对 R771 接受基线 exact；全 DAG 为 RTL
   1,534,187 ns、gem5 1,539,924 ns，`+5,737 ns/+0.373944%`。主要独立残差
   task3 +5,445 ns、task8 -1,091 ns、task4 +903 ns、task2 -327 ns。
3. CCH 首断点：task3 seq0 VLOAD +8 cycles；task9 seq1 VSTORE +1；task14
   seq0 VLOAD +1；task17 seq1 VLOAD +1、seq9 VSEQ 首 fire +1；完整并发
   task20 seq0 VLOAD -1。独立 task20 的 sequence0--8834 已 exact，首残差为
   seq8835 VBRDCST 38 对 RTL 40 cycles。
4. SCH 首断点：task2 seq45 VRANGE +3 cycles；task3 seq0 VLOAD -1、后续
   CAU/SerDiv/LSU 重叠累积；task4 seq8 VDIV fire -1，继续拆 SerDiv D/Q；
   task8 seq2 SCATTER/VSHUFFLE fire/recycle -2，继续拆 Shuffle/LSU、稳定
   tagged requester、LSU 高优先级和 persistent per-bank RR。
5. LSU RAW/throughput/capacity 68/68 通过，382 条 VINS 对 R752 控制
   byte-exact。复用现有全局 retirement table 的 R805 实验使 task20 退化到
   414,068 ns，已否决并撤销；接受实现 task20 恢复为 398,520 ns。

永久证据：

    evidence/a32_r806_current_cch_sch_20260814/

+======================================================================
当前断点：A32 R797 Shuffle completion-Q（2026-08-14）
+======================================================================

完整报告：

    docs/venus_alignment/A32_R797_SHUFFLE_COMPLETION_Q_ALIGNMENT.md

1. 两个独立 RTL oracle 均证明：Shuffle 最终逐 bank 写 grant 后，先形成
   `parallel_shuffle_complete_d`，再于下一 tile 边沿清除
   `global_hazard_table_d` 并允许 VSTU operand request。gem5 原来在 final
   grant 回调当拍发布完成 tombstone，依赖 requester 早一拍。
2. `VenusSequencer` 现加入带 running-ID/instruction generation 的 registered
   Shuffle completion token；同拍 LSU/LDU 先采样旧 Q，下一拍才看到完成。
   reset/idle/stale-ID 均有结构化处理；未改固定 latency，不含 task/sequence/
   PC/opcode/地址/数据特判。
3. 独立 task20：sequence647 VSTORE 的 fire/duration/recycle 已由
   `17423/136/17559` 收敛到 RTL 的 `17423/138/17561`；sequence648 VLOAD
   也全部 exact。首个 lifecycle 分歧从 647 推进到 sequence8834 VBRDCST
   duration/recycle `-4 cycles`，sequence0--8833 exact。
4. CCH full：24/24 tasks、53/53 returns、10,253 VINS 对接受基线 exact。
   task20 从 `-299 ns/-0.075%` 收敛到 `-11 ns/-0.003%`。全 DAG 为
   `+461 ns/+0.044%`，但 task3 `-245 ns`、task9 `-87 ns`、task14
   `+235 ns` 仍须独立处理。并发 task20 首差仍是 sequence0 VLOAD `-1`
   cycle；task17 仍是 sequence1 VLOAD `+1` cycle，不能用任务总时长验收。
5. SCH held-out：9/9 tasks、22/22 returns、8,103 VINS 对接受基线 exact；
   全 DAG `+5,749 ns/+0.375%`。主要残差 task3 `+5,457 ns/+1.157%`、
   task4 `+903 ns/+2.993%`、task8 `-1,091 ns/-0.330%`、task2 `-297 ns`。
6. LSU 68/68，382 VINS 对保留控制 byte-exact；BitALU/CAU/SerDiv 均实际
   depth=2/full，1/4/64 ns 输出 exact。下一步保持 sequence647/648 不回退，
   拆完整并发 task20 sequence0 的 LDU 初始相位与独立 replay 差异，再处理
   sequence8834；task17 和 SCH task3/4/8 继续独立推进。

永久证据：

    evidence/a32_r797_shuffle_completion_q_20260814/

+======================================================================
当前断点：A32 R785 BitALU full-Q operand admission（2026-08-14）
======================================================================

完整报告：

    docs/venus_alignment/A32_R785_BITALU_FULL_ADMISSION_ALIGNMENT.md

1. 紧凑 RTL oracle 证明普通 BitALU 的 mask operand D/Q handshake 与 A/B
   admission 一样受 `!result_queue_full` 门控。gem5 过去会在 result Q=2/full
   时预取 mask-D，使 task20 sequence608 的首结果与 recycle 提前 1 cycle。
2. 通用修复不含 task/sequence/PC/opcode/地址/数据特判，未改 CPU、LSU、VFU
   固定 latency。sequence608 VSGT 的 fire/duration/recycle 已全部 exact；首个
   lifecycle 分歧推进到 sequence647 VSTORE duration/recycle -2 cycles，首个
   fire 分歧为 sequence648 VLOAD -2 cycles。task20 8,837 VINS exact。
3. 局部修复在后续流水汇合前被吸收，因此 CCH 24 task 最终区间不变：全链路
   -919 ns/-0.088%；主要绝对残差仍是 task20 -299 ns、task3 -229 ns、
   task14 +219 ns。不能用总 DAG 或不变的 task 尾时间否定局部结构修复。
4. SCH 9 task 也不变：全链路 +4,825 ns/+0.314%；主要为 task3
   +5,461 ns/+1.158%、task8 -1,091 ns/-0.330%、task4 +903 ns/+2.993%、
   task2 -297 ns/-0.538%。
5. CCH 10,253、SCH 8,103 条 VINS 对接受基线 exact；SCH 保留 RTL dump
   仍只覆盖 7,256 条。LSU 68/68；BitALU/CAU/SerDiv 均实际触发 depth=2/full，
   容量 case 功能 exact。
6. 下一步拆 task20 sequence647/648 的 LSU completion/issue 边界；SCH 独立按
   task3 CAU/SerDiv/LSU、task4 SerDiv D/Q、task8 Shuffle/LSU 的顺序推进。

永久证据：

    evidence/a32_r785_bitalu_full_admission_20260814/


+======================================================================
当前断点：A32 R767 physical operand-command validity（2026-08-14）
======================================================================

完整报告：

    docs/venus_alignment/A32_R767_PHYSICAL_OPERAND_COMMAND_VALIDITY_ALIGNMENT.md

1. RTL oracle 证明未使用的 vs1/vs2/vd/mask operand 不产生物理
   `operand_req_valid_o`；gem5 为功能配对保留的 neutral placeholder 不能
   占据一项物理 command register，也不能推进 registered command-ack。
2. 通用修复不含 task/sequence/PC/opcode/地址/数据特判，未改 CPU、LSU、
   VFU 固定 latency。task20 sequence78 VSUB 已从 fire +1/duration -1
   收敛为 fire/duration/recycle 全 exact；首个剩余 lifecycle 分歧推进到
   sequence608 VSGT recycle/duration -1 cycle，首个 fire 分歧为 sequence612
   -1 cycle。8,837 VINS exact。
3. CCH 全部 24 task 的最终时间相对 R753 未移动：全链路 -919 ns/-0.088%；
   主要绝对残差 task20 -299 ns、task3 -229 ns、task14 +219 ns。task17
   +3 ns、task18 -1 ns，且 task17 首残差仍是 LSU seq191/194。
4. SCH 全部 9 task 未移动：全链路 +4,825 ns/+0.314%；主要为 task3
   +5,461 ns/+1.158%、task8 -1,091 ns/-0.330%、task4 +903 ns/+2.993%、
   task2 -297 ns/-0.538%。
5. CCH 10,253、SCH 8,103 条 VINS 对接受基线 exact；SCH 保留 RTL dump
   仍只覆盖 7,256 条，另 847 条不冒充全量 RTL 比较。
6. LSU 68/68 且 382/382 VINS exact；BitALU/CAU/SerDiv 均触发 depth=2/full，
   容量 case 功能 exact。下一步拆 task20 seq608，同时独立处理 SCH task3
   CAU/SerDiv/LSU、task4 SerDiv D/Q、task8 Shuffle/LSU。

永久证据：

    evidence/a32_r767_physical_operand_command_validity_20260814/


+======================================================================
当前断点：A32 R753 BitALU registered result-ready 对齐（2026-08-14）
======================================================================

完整报告：

    docs/venus_alignment/A32_R753_BITALU_REGISTERED_RESULT_READY_ALIGNMENT.md

1. 已按 RTL Q/D 边界拆开普通 BitALU handshake：operand FIFO-Q 在真实
   handshake 当拍消费；result-count reservation 下一拍可见；result grant
   后禁止同 tick 重新做 operand admission。没有 task/sequence/PC/地址/数据
   特判，也未修改 CPU、LSU、VFU 固定 latency。
2. task20 sequence 34--77 的 fire/duration/recycle 全 exact；首个剩余
   lifecycle 分歧推进到 sequence78 VSUB（fire +1、duration -1，recycle
   抵消），首个 recycle 分歧为 sequence608 VSGT -1 cycle。8,837 VINS exact。
3. CCH task20 从 -1,451 ns/-0.364% 收敛到 -299 ns/-0.075%，改善
   1,152 ns；其余 23 个 task 均未移动。全链路 span 为 RTL 1,042.151 us、
   gem5 1,041.232 us，即 -0.919 us/-0.088%，但总误差不作为验收条件。
4. SCH 完全未移动：全链路 +4.825 us/+0.314%；主要残差仍为 task3
   +5.461 us/+1.158%、task4 +0.903 us/+2.993%、task8 -1.091 us/-0.330%、
   task2 -0.297 us/-0.538%。
5. CCH 10,253、SCH 8,103 条 VINS 对接受基线 exact；保留 RTL SCH dump
   仍仅覆盖 7,256 条，另 847 条不冒充 RTL 全量比较。
6. LSU 68/68、382/382 VINS exact。BitALU/CAU/SerDiv 均实际达到
   depth=2/full，1/4/64 ns 容量 case 功能 exact。容量检查器已补充识别
   CAU explicit D/Q 的结构化日志名，避免误报。
7. 下一步保持 task20 sequence34--77 不回退，拆 sequence78 与 sequence608；
   SCH 独立拆 task3 CAU/SerDiv/LSU、task4 SerDiv D/Q、task8 Shuffle/LSU，
   短 task 的 7--11 ns 则用 scalar-only CPU 边界 case 处理。

永久证据：

    evidence/a32_r753_bitalu_registered_result_ready_20260814/


+======================================================================
当前断点：A32 R721 BitALU handshake/result-Q 负向 oracle（2026-08-14）
======================================================================

完整报告：

    docs/venus_alignment/A32_R721_BITALU_HANDSHAKE_RESULTQ_NEGATIVE_ORACLE.md

1. 新增 full-DAG sequence-1 RTL oracle：正常 BitALU 为 operand handshake
   同拍 result-D、下一拍 result-Q；edge21 的唯一停顿来自真实 data-bank 冲突。
2. sequence66 RTL 的两次 external mask grant 比对应 result-D 晚一拍；gem5
   当前同拍 grant，这是局部 -1 的直接来源。但单独补这一拍会让原本 exact 的
   sequence57 先慢 1 cycle，并因 mask clear/refill 把 sequence66 推迟 2 cycles。
3. pending capacity、Q-only admission、独立 A/B staging、旧 staged operand
   同拍计算等候选均经 task20 oracle 否决并完整回退；未使用 task/sequence/PC/
   地址/数据特判，也没有修改固定 LSU/VFU latency。
4. 已恢复 R682 接受行为：task20 tick 397,080,000，8,837 VINS 与完整 monitor
   对 R717 byte-exact；首 duration/recycle 分歧仍为 sequence66 62/63 cycles，
   首 fire 分歧仍为 sequence70 -1 cycle。
5. 下一步必须把 A/B data-valid、mask latch、operand handshake、result queue
   Q/D、bank requester Q、mask clear/refill 与 retirement 合成一个 pre-edge
   snapshot 驱动的 clocked transition，并用 sequence1/57/66 三 oracle 同时验收。

恢复二进制 SHA256：

    ec5a6d11bac91e39564928a11e27467eedb18881d5d44555beb3dfc91fbe4f41


+======================================================================
当前断点：A32 R648 BitALU tagged handoff / CCH-SCH 全链路审计（2026-08-13）
======================================================================

完整报告：

    docs/venus_alignment/A32_R648_BITALU_HANDOFF_FULL_DAG_AUDIT.md

1. 已按 RTL `venus_operand_requester.sv` 晋升普通 BitALU-A/B 的 tagged
   completion：requester 在最后一次 VRF grant 后即可接收下一条 command，
   旧 command 的 SRAM response 继续按 tag 排空。保留单 outstanding
   throughput；未修改 LSU/VFU 固定 latency，也没有 task/sequence/opcode
   特判。
2. task20 sequence 0--58 的 fire/duration/recycle 全部 exact；首个
   duration/recycle 分歧推进到 sequence 59 VBRDCST 慢 1 cycle，首个 fire
   分歧是 sequence 63 VXOR 晚 1 cycle。8,837/8,837 VINS exact。task17
   687/687 VINS exact；首个 duration 分歧为 sequence 180 VBRDCST 快
   1 cycle，首个 fire 分歧为 sequence 194 VLOAD 晚 1 cycle。
3. CCH 全链路同边界为 RTL 1,042.151 us、gem5 1,050.684 us，即
   +8.533 us/+0.819%。主要独立误差：task20 +9.201 us/+2.309%，task3
   -0.895 us/-1.285%；task17 +0.005 us/+0.015%，task18
   -0.001 us/-0.001%。10,253 条 task-local VINS 与接受基线 byte-exact。
4. SCH 全链路为 RTL 1,534.187 us、gem5 1,538.992 us，即
   +4.805 us/+0.313%。主要独立误差：task3 +5.501 us/+1.167%，task4
   +0.843 us/+2.794%，task8 -1.091 us/-0.330%，task2
   -0.297 us/-0.538%。8,103 条 VINS 与接受基线 exact；历史 RTL dump
   仅覆盖其中 7,256 条，剩余 847 条没有冒充 RTL 全量比较。
5. LSU RAW/throughput/capacity 68/68 通过，382/382 VINS exact。将普通
   BitALU outstanding 改为 2、或令所有同 VFU hazard 统一等待三拍 global
   broadcast 的实验均造成更早分歧，已完全回退。
6. task20 末端仍累积 +4,605 tile cycles。下一步必须在 sequence 60 与
   非回归控制窗口 sequence 34--37 同时捕获 source/destination hazard
   bitmap、`raw_hazard_counter_q`、writer metadata、global hazard table、
   opqueue ready、四-bank requester vector、persistent RR、LSU priority、
   result queue 和 tagged retirement。不能用固定 latency 或总 DAG 抵消拟合。
7. 当前二进制 SHA256：

       03831f6e152e2c6eb80360a8394e1518f5899db25ed239f10c2316e19f0428d4

永久证据：

    evidence/a32_r648_bitalu_handoff_20260813/


======================================================================
当前断点：A32 R611 held command fall-through / mask-row oracle（2026-08-13）
======================================================================

完整报告：

    docs/venus_alignment/A32_R611_HELD_FALLTHROUGH_MASK_ROW_ORACLE.md

1. 已晋升通用 lane-command 规则：只有在 lane busy 期间跨至少一整拍保持
   稳定的不同指令，才允许在前一 command consume 的同边沿 refill；fresh
   request 保留原注册边界。无 task/PC/op/address/data 特判。
2. task20 sequences 0--54 保持 exact，随后两条 VMUL 原有 +1-cycle
   duration/recycle 已闭合。首个剩余 duration/recycle 分歧推进到
   mask-producing VSGT（comparison sequence 57）快 1 cycle；首个 fire
   分歧是 sequence 63 晚 1 cycle。8,837 VINS exact。
3. RTL oracle 已确认真正缺口：BitALU `vm_w` 在 `mask_q` 内累加，
   depth-2 result queue 的行内项通过 `mask_jump_gnt_wr` 出队，仅四-bank
   行边界/末项向独立 mask read/write RR 发请求。gem5 仍逐 16-bit
   micro-result 写 mask。
4. 三个 timing-only/直接打包实验均未晋升并已回退。直接写 RTL 64-bit
   mask row 会破坏首条 VSGTmask VINS，证明必须先建立逻辑 mask-row 到
   gem5 element dump 的正确映射，不能简单拓宽现有物理写事务。
5. 最新 CCH 同边界残差：task20 -3,759 ns/-0.943%，task3
   -895 ns/-1.285%，task14 +215 ns/+0.106%；task17 +5 ns/+0.015%，
   task18 -1 ns/-0.001%。10,253 VINS exact。
6. 最新 SCH：task3 +5,501 ns/+1.167%，task8 -1,091 ns/-0.330%，
   task4 +843 ns/+2.794%，task2 -297 ns/-0.538%。8,103 VINS exact。
7. LSU RAW/throughput/capacity 68/68，382 VINS 文件与接受基线 exact。
   总 DAG 与任务间抵消仍不作为验收标准。结论 INCONCLUSIVE。

永久证据：

    evidence/a32_r611_held_fallthrough_20260813/


======================================================================
当前断点：A32 R519 task20 requester-vector 首分歧（2026-08-13）
======================================================================

完整报告：

    docs/venus_alignment/A32_R508_PRODUCER_COMPLETION_ALIGNMENT.md

1. R516 已直接捕获 task20 sequence 34/36/37 的 result ID、地址和逐 bank
   grant；sequence 37 的尾部包含 requester 冲突与 LSU 高优先级阻塞，不能
   用 BitALU 固定 latency 修补。
2. R519 已直接读出 RTL 四个 `rr_arb_tree.rr_q`。它们跨窗口持久保持，进入
   sequence 33 时均为 requester 11；gem5 的 requester 编号和 persistent
   per-bank RR 结构没有发现算法级首分歧。
3. 更早的首分歧在 admission：RTL `pe_req_valid_o` mailbox 后 1 cycle 就
   出现 sequence-33 operand request；接受基线 gem5 是 fire 后 4 cycles。
4. “PE broadcast 提前 + lane 同拍 fall-through”使该窗口 fire 累计提前约
   32 cycles、task 结束提前 648 ns；仅做 lane fall-through 仍使 fire 提前
   4 cycles，sequence34/37 recycle 提前 8/11 cycles。两项实验均已回退，
   没有把局部更像 RTL、全局更差的补丁留在源码中。
5. 这证明当前一拍 lane bubble 正在补偿缺失的 registered consume/
   completion/ready 返回边界。下一步必须显式建模 fall-through input 和每个
   operand command register 的 valid/ready/consume-refill，再比较稳定
   requester vector 与 RR；禁止调固定 VFU latency 或依赖 DAG/task 抵消。
6. 接受基线保持：CCH 10,253、SCH 8,103 条 VINS exact；CCH 全 DAG
   -8,096 ns/-0.776%，主要 task20 -7,215 ns；SCH 全 DAG
   +4,116 ns/+0.268%，task3 +5,185 ns 与 task8 -1,091 ns 有抵消。
7. 恢复版二进制 SHA256 为
   `4e50fb61a079af4600c66f6853ba8bebc283e4e81dfee79d9b8d31976818bb7f`；
   task20 exit tick 恢复为 391316000，8,837-entry sequencer monitor 与 R517
   byte-identical。

永久证据：

    evidence/a32_r508_producer_completion_20260812/rtl_oracle/

======================================================================
当前断点：A32 R457 CAU / STU registered boundary（2026-08-12）
======================================================================

完整报告：

    docs/venus_alignment/A32_R457_CAU_STU_BOUNDARY_ALIGNMENT.md

1. CAU arithmetic pipe 已按 RTL `LatAddSub/LatMul/LatCmxMul=0/2/4`
   建模；requester Q visibility 与 depth-2 result queue 保持独立边界。
   task17 sequence 4 的 operand grant 原本已经 exact，修正后首个 result
   不再晚 2 cycles，sequence 0--95 exact。
2. STU operand-ready -> local-W 从 2 改为 RTL oracle 证明的 1 tile cycle；
   sequence 96 VSTORE 收敛到 52/52 cycles。task17 sequence 0--176 的
   fire/recycle/duration 全部 exact；首个剩余 duration/recycle 分歧是
   sequence 177 VXOR 快 1 cycle，首个 fire 分歧为 sequence 180
   VBRDCST 晚 3 cycles。
3. “所有 requester 统一加一拍 Q”实验使 task20 前 419 个 bank-vector
   cycle 中 187 个改变，且未推进 task17 首断点，已回退。说明下一步必须
   显式拆 passage entry phase，不能继续用 requester 类型特判或全局加拍。
4. 最新 CCH：10,253 VINS exact。主要独立残差为 task20
   -4,739 ns/-1.189%、task18 +3,465 ns/+2.812%、task14
   +1,023 ns/+0.504%、task17 +147 ns/+0.443%。全 DAG 仅
   -916 ns/-0.088% 是明显抵消，不作为验收。
5. 最新 SCH：8,103 VINS exact。task2/3/4/8 分别为
   +2,581/+6,853/+883/-409 ns；全 DAG +7,104 ns/+0.463%。
6. 本轮二进制 SHA256：

       c4b48ca7dea7dae74d15875ed55eec407023c6ad4e5fbbbc10b372e30eb6c01c

7. 永久证据：

       evidence/a32_r457_cau_stu_alignment_20260812/

结论仍为 INCONCLUSIVE。下一步先做 task17 sequence-177 VXOR 的 result
enqueue、writer request、逐 bank grant、retirement RTL oracle；task20 保持
CAU 0/2/4，不允许用固定 latency 回调掩盖，继续拆 stable tagged requester、
LSU 独立优先级、registered WB/RAW visibility 和 persistent per-bank RR。

======================================================================
当前断点：A32 R327 vector LSU producer final-grant（2026-08-12）
======================================================================

完整报告：

    docs/venus_alignment/A32_R327_VECTOR_LSU_PRODUCER_GRANT_ALIGNMENT.md

1. 修正非对齐 AXI beat 计数、VSTU operand-ready 到 local-W 的一拍边界，
   并新增按 generation/running-ID/active-lane 聚合的 producer final-grant
   sideband；没有修改 LSU/VFU 固定 latency，也没有 task/PC/DAG/地址值/
   数据值特判。
2. task17 sequence 0--100 的 fire/duration/recycle 现在全部 exact；sequence
   96 VSTORE 从 gem5 56 / RTL 52 cycles 收敛到 52/52。首个剩余分歧推进到
   sequence 101 VXOR，gem5 快 1 cycle。
3. CCH 10,253 VINS task-local exact。task17 为 -87 ns/-0.262%；task18
   +3,465 ns/+2.812%；task20 -10,009 ns/-2.512%。task20 sequence-2 fire
   仍晚 163 cycles，后段又反向漂移，禁止用总 task 或 DAG 抵消验收。
4. SCH 8,103 VINS exact。task2/3/4/8 分别为 +1,649/+6,909/+1,363/
   -2,077 ns；仍需独立拆分。
5. LSU 68/68，382 条 VINS 对接受基线 byte-exact；BitALU/CAU/SerDiv
   六档 capacity sweep 均通过并实际触发 depth=2/full。当前二进制 SHA256：

       f91a2a60f3a5670659c8f9308c7644a1efd63a6c2bd1b85c75c18884ee648030

6. 永久证据：

       evidence/a32_r327_vector_lsu_alignment_20260812/

结论仍为 INCONCLUSIVE。下一步先拆 task17 sequence-101 VXOR 的 admission、
result enqueue、逐 bank grant 与 retirement；task20 的早期标量循环和后段
vector requester/LSU priority/persistent RR 必须继续作为两个独立缺口。

======================================================================
当前断点：A32 R260 Shuffle stable requester vector（2026-08-12）
======================================================================

完整报告：

    docs/venus_alignment/A32_R260_SHUFFLE_STABLE_REQUEST_VECTOR_ALIGNMENT.md

1. Shuffle blocked request 现在保存实际仲裁 requester vector；tagged grant
   时用同一 vector 更新 FairArb，未加入 task/PC/DAG/地址/数据或固定 latency
   特判。
2. CCH 24/24、10,253 VINS exact；task17 从 +491 ns/+1.480% 改善到
   +175 ns/+0.528%，但 task20 从 -6,847 ns/-1.718% 恶化到
   -7,419 ns/-1.862%。不能用 task17/task20 抵消或全 DAG 误差验收。
3. SCH 9/9、8,103 VINS exact；主要残差仍为 task2 +1,723 ns/+3.120%、
   task3 +6,933 ns/+1.470%、task4 +1,023 ns/+3.390%。
4. 最终等价重建 SHA256：

       fd9392b560b3a52f2d417cf2b6f8abdce4642004df8e22f4dba930bacef323a6

   LSU 68/68、382/382 VINS 对 R253 exact；BitALU/CAU/SerDiv 均实际达到
   depth=2/full，enqueue/dequeue 闭合。
5. task20 的首个逐 bank 分歧仍在 rel89：RTL 在 LSU priority 释放后用
   `LockIn=0` 的 live `[PE0,PE8]` vector 重选 PE0；gem5 timing retry 仍固定
   PE8，使 PE0 DATA/WRITE 晚 1--2 cycles。
6. 跨 bank intent 迁移和同-bank retry-owner 替换两种实验均出现
   stale retry/live-lock 或前序进度异常，已全部回退。下一步必须把 Shuffle
   组合选择与 XBar per-bank intent/grant 建成显式接口，再对 rel84--93，
   task17/task18 和全回归逐层验收。

======================================================================
当前断点：A32 R220 scalar600 CPU 逐周期 exact（2026-08-11）
======================================================================

完整报告：

    docs/venus_alignment/A32_R220_SCALAR600_CPU_COMPLETE_ALIGNMENT.md

1. scalar600 CPU RTL-oracle 定向矩阵已扩展为 52/52；1,275/1,275 条有序
   退休在 0-cycle 容差下 exact，1,275/1,275 条寄存器写回 exact。
2. 新覆盖 OP/OP-IMM reserved funct、CSR[11:1] alias、JALR 宽松 decode、
   标准/宽松 WFI sleep-wake、非法 LOAD/STORE/BRANCH 默认和 decoder false
   load-use hazard（含 x0）。WFI 唤醒已写入 case contract，可一次跑完整套。
3. LSU 回归 68/68；382 个 VINS 文件与接受基线 exact。实现中没有
   task/PC/DAG/workload/address/data 特判。
4. 当前 CCH：allocation-to-last-release `1,027.996 us` 对 RTL
   `1,043.224 us`，差 `-15.228 us / -1.460%`；10,253 条 task-local
   VINS 与接受基线 exact。最大独立残差仍为 task20
   `-19.729 us / -4.951%`，task17 `+1.141 us / +3.440%`。
5. 当前 SCH：`1,538.420 us` 对 RTL `1,535.100 us`，差
   `+3.320 us / +0.216%`；8,103 条 task-local VINS 与接受基线 exact。
   task2/3/4/5/8 仍分别为 +2.277/+7.085/+1.423/-0.721/-4.123 us。
6. 不能把上述结果表述为形式化全输入等价，也不能宣称全 DAG 建模完备。
   已覆盖的普通 scalar600 核心行为可接受为逐周期 exact；下一步回到
   task20 stable tagged requester/vector-LSU priority/persistent per-bank RR，
   并独立拆 task17/task18 与 SCH 残差，禁止用全 DAG 或任务间抵消验收。
7. 当前二进制 SHA256：

       ffc45596e1e65988c59e1288aaea81dd86ed332417573b4223325a970ef172a5

机器可读证据：

    evidence/a32_r220_scalar600_cpu_20260811/

======================================================================
当前断点：A32 R120 CAU completion / WAITING_FOR_READY（2026-08-10）
======================================================================

CPU 仍不能宣称完全对齐，但普通 scalar600 核心与 Venus 接口缺口已经分离：

1. 已接受 R120 二进制 SHA256：

       27cdb64e4b554dfa8cbe9062e451a44960bc1e2062896408c7cb00155e774014

   回退两项 lane-command 隔离实验后的源码等价重建 SHA256：

       51146dd1b41b2e5a050e2223cb3bdfc5f3f5daad1dc00ab860fb0a0a9d5ad5b5

2. 五套 scalar600 suite 共 25/25；排除旧错误 fixture、采用修正版后有效
   矩阵 24/24 逐退休 exact。LSU RAW/throughput/capacity 68/68。
3. CAU completion 采用 RTL 的一拍完成边界，VMUL-family 内部流水增加一
   级以保持乘法总时长。task17 的两组 VMUL/VSADD 同时闭合：sequence
   4--8 duration 为 32/34/34/37/39 cycles，与 RTL 全部一致；首个
   recycle divergence 从 sequence 5 推进到 sequence 35。
4. 已显式记录 RTL `WAITING_FOR_READY`：下游 handshake 完成但目标 VFU
   queue full 时保持 scalar FIFO head；Return->Upstream 状态更新后，在同
   tick 重试该稳定请求。task17 sequence 0--21 的 fire/recycle/duration
   三维现在全部 exact。
5. 当前首个差异推进到 sequence 22 VMUL：fire gem5 165、RTL 161，
   recycle 都是 198，因此 duration gem5 33、RTL 37。trace 显示前一条
   VSEQ 的 lane3 command accept 晚于其他 lane；简单用“本 tick 有 consume
   event”推断 fall-through 无效果并已回退。下一步需显式建模 lane command
   register 的 valid/ready/consume-refill，而不是从 FIFO empty 反推。
6. CCH 10,253、SCH 8,103 条 VINS 继续逐 task、逐元素 byte-exact。
   CCH task17 为 +20,449 ns（+61.655%），task20 为 -15,597 ns
   （-3.914%）；SCH task2/task8 为 +4,079/+6,063 ns。禁止用这些相反
   误差抵消。
7. 证据：

       /tmp/a32_r117_waiting_same_tick_cch/
       /tmp/a32_r120_waiting_same_tick_sch/
       /tmp/a32_cpu_{suite,transition,lsu,div,extended}_r120/
       /tmp/a32_r120_waiting_same_tick_lsu_suite/suite_results.json

8. 追加的 command-ready 边界实验均未晋升：2->1 cycles 时 sequence22
   fire 从 +4 缩到 +2，但首个 recycle 分歧提前到 sequence34，task20
   恶化为 -16,173 ns；2->0 cycles 虽使 sequence22/23 临时 exact，却把
   首个 fire 分歧移到 sequence24 +9 cycles，task20 恶化为 -17,293 ns。
   两者都保持 10,253 VINS exact，说明不能用功能一致或局部命中替代逐拍
   结构验证。证据在：

       /tmp/a32_r121_cmd_release1_cch/
       /tmp/a32_r122_cmd_release0_cch/

   已恢复 R120 的 2-cycle 源码并重建；五套 scalar runner 仍为 25/25
   （有效矩阵 24/24）：

       /tmp/a32_cpu_{suite,transition,lsu,div,extended}_restored_r120/

当前结论仍为 INCONCLUSIVE。下一步从 sequence 21->22 的 per-lane command
valid/ready 与 operand requester consume/refill 继续；随后处理 sequence 35
的首个 recycle 分歧。

======================================================================
当前断点：A32 R77 同边界 task 时序 / 高百分比拆分（2026-08-10）
======================================================================

这是最新交接断点，优先于下方 R76。完整证据与分析：

    docs/venus_alignment/A32_R77_EXECUTION_BOUNDARY_HIGH_PERCENT_ALIGNMENT.md

1. 已修正逐 task 比较口径：RTL `start execute -> execute complete` 只与
   gem5 `tile_start -> task_epilogue` 比较；return DMA/release 独立报告。
   旧表把 gem5 return tail 只加在一侧，制造了短 task 的 30%--111% 假误差。
2. CCH task0/2/5/6/7/8/10/11/12/16 在同边界下全部落到 ±3.6% 内；例如
   task0/task2 均从约 +90% 变为 -11 ns（-2.011%）。这些任务禁止再通过
   CPU/VFU latency 调参“修复”。
3. 当前真实高百分比 CCH 目标变为 task13 +75 ns（+13.321%）、task23
   +4,109 ns（+12.103%）、task4 +291 ns（+7.241%）。SCH 为 task7
   +121 ns（+11.426%）、task2 +4,277 ns（+7.744%）、task6 +187 ns
   （+4.928%）。
4. task13 六条 CAU 指令的 fire：RTL 0/6/12/18/56/62 cycles，gem5
   0/6/12/18/94/100；首四条 exact，第五条晚 38 cycles。FairArb/tagged
   requester 候选可把前两条 duration 精确到 34/51 cycles，但第五条 fire
   仍晚 38，因此没有修改 VMUL 固定 latency，也没有晋升该候选。
5. 首个结构断点是第四条 CAU 在 16 lane operand-command ready 汇合处
   滞留，阻止 sequencer 接收第五条。下一步对齐 RTL lane-sequencer
   fall-through command register 与 operand-requester 同拍 consume/refill，
   并逐 lane 比较 ready vector。
6. 本轮未改模拟器二进制，R76 SHA 与功能门保持：CCH 10,253 VINS exact，
   SCH 8,103 exact（其中 7,256 有 RTL dump），LSU 68/68。结论仍为
   INCONCLUSIVE，不能使用总 DAG 或 return tail 抵消。

======================================================================
当前断点：A32 R76 directional in-order LSU / WB（2026-08-10）
======================================================================

这是最新交接断点，优先于下方 R59。完整证据与分析：

    docs/venus_alignment/A32_R76_DIRECTIONAL_INORDER_LSU_WB_ALIGNMENT.md

1. scalar memory op 现在只能在成为 in-flight 与 memory-FU 双队首后，于
   前序指令退休同边沿进入 LSU；仍是严格顺序，不启用全局 speculative
   early-memory-issue。
2. RTL 状态化边界已加入：taken control -> load 8 cycles，fall-through /
   not-taken control -> load 6，control -> store 4，load -> store 3，真实
   load RAW consumer 观察注册 WB/RF 边界。无 task/PC/address/value 特判，
   LSU/VFU 固定 latency 未改。
3. 成对证据：task18 高频 branch->store 4/4、load->store 3/3、聚合
   load->ALU exact；task20 branch->load 6/6。task20 load->dependent branch
   仍是 gem5 4、RTL 2，累计 +328 cycles。
4. CCH 10,253 VINS exact；全 DAG +2,484 ns（+0.238%）。主要残差：task9
   +10,185 ns，task18 +3,089 ns，task20 -13,035 ns，task22 +4,013 ns，
   task23 +4,481 ns。
5. SCH held-out 8,103 VINS exact；历史 RTL 覆盖仍为 7,256，另 847 条不
   冒充比较。全 DAG +13,460 ns（+0.877%）；task0/task2/task3/task4 为
   +11,761/+4,529/+2,549/+1,633 ns。
6. LSU RAW/throughput/capacity 68/68。二进制 SHA256：

       41469f72c13a8b3814f259cb413f0c805527f2cd36282cdf1f4c9788fde1f699

当前结论仍为 INCONCLUSIVE。R76 保留是因为拆除了 CPU/LSU 边界抵消，
不是因为总 DAG 更小。下一步先闭合 dependent load->branch 的 ID admission
与 follower 边界，再回到 task20 stable tagged requester、LSU 独立优先级和
persistent per-bank RR。task9/SCH task0 必须独立分析，禁止与 task20 抵消。

======================================================================
当前断点：A32 R59 RTL tree winner / scalar retire（2026-08-09）
======================================================================

这是最新交接断点，优先于下方 R57。完整证据与分析：

    docs/venus_alignment/A32_R59_RTL_TREE_CPU_RETIRE_ALIGNMENT.md

本轮没有修改 LSU/VFU 固定 latency，也没有 task/PC/DAG/address/payload
特判。

1. 已纠正对 `rr_arb_tree.sv` 的解释：当前 winner 由当前 `rr_q` 驱动的
   二叉树产生，FairArb mask/LZC 只产生下一拍 `rr_q`。winner 与 next-RR
   均保持 tagged，直到请求真正跨过 bank。
2. task17 sequence 6 在聚焦 trace 中逐边沿 exact：operand admission
   +8/+14 ns，operand grants +50/+54/+56/+58 ns，result enqueue/grants
   +56/+60/+62/+64 ns，retirement/recycle +66/+68 ns。
3. `venusRtlScalarTiming` 下 Venus no-cost 指令现在也占用单 commit slot，
   消除了同一 gem5 edge 多条 Venus 指令退休/发送的 CPU 合同缺口。该
   修正不是 VFU latency 调参；task17 首个 duration divergence 在 R59
   反而提前到 sequence 1，说明后续仍需拆 fire/admission 相位。
4. nrPDCCH：24/24 tasks，10,253 条 VINS 与接受基线 byte-exact。全 DAG
   +1,392 ns（+0.133%），但 task9 +11,729 ns、task20 -12,371 ns，仍有
   明显抵消；task17 +341 ns、task22 +4,841 ns、task23 +4,625 ns。
5. PDSCH held-out：9/9 tasks，8,103 条 VINS 与接受基线 exact；历史
   RTL dump 仍只覆盖 7,256 条，另外 847 条未冒充 RTL 比较。全 DAG
   +6,708 ns（+0.437%）；task2/task4/task7 为
   +8.193%/+5.531%/+20.113%。
6. 定向门：LSU 68/68；BitALU/CAU/SerDiv 均达到 depth=2 和实际 full，
   所有 gap 功能 exact。
7. task20 已分成两个独立缺口：sequence 2 VSTORE 初始 fire 晚 757 tile
   cycles；随后 8,837 条主体循环又从 +757 漂移到末条 -6,370 cycles。
   禁止用一个 latency 同时拟合这两个相反误差。
8. 当前二进制 SHA256：

       34bbc854bb9f9002f7cd4788aed4898aa9eeda1cbb7e1237d120425e2a762c1a

当前结论：VRF tree winner 和 CPU 单退休合同已经结构化修正，但建模仍未
完备。下一步先抓 task20 sequence 1->2 的 scalar/LSU release 边界，再抓
重复循环的 transition/requester winner 漂移；task17 则从 sequence 1 的
fire -1 cycle 和后续补偿关系继续。总 DAG 百分比只作回归观察，不作接受
条件。

======================================================================
当前断点：A32 R57 FairArb / cross-instruction phase（2026-07-31）
======================================================================

这是最新交接断点，优先于下方 R56。完整证据：

    docs/venus_alignment/A32_R57_FAIRARB_PHASE_ALIGNMENT.md

候选仍使用：

    VENUS_GEM5_EXPERIMENTAL_VRF_RR=1
    VENUS_GEM5_EXPERIMENTAL_REQUESTER_Q_VISIBILITY=1

本轮没有修改 LSU/VFU 固定 latency，也没有 task/PC/DAG/address/payload
特判。

1. 已按实际 RTL `rr_arb_tree.sv:203-232` 修正 FairArb：从 `rr_q` 之后
   的最低有效 index 开始选、必要时回绕；成功 grant 后 `rr_q` 更新为
   本次 winner。R56 把 `rr_q` 当树形 preference bits，结论已被本断点
   覆盖。
2. task17 sequence-6 仍为 34 cycles，前七条 duration 仍是
   12/34/12/29/32/34/34，与 RTL 相同；但逐 stage 不是全 exact：
   operand-A admission +8 ns exact，operand-B 为 +12 ns、RTL +14 ns。
   后续 grant +50/+54/+56/+58、enqueue/grant +56/+60/+62/+64、lane
   retirement +66、sequencer recycle +68 均 exact。端点一致不能掩盖
   admission 提前 2 ns。
   该差异已追到复用的 BitALU-B requester：前驱 instr715 的四次 grant
   当前为 fire+40/+42/+44/+46 ns，requester 在 +48 ns 完成并同拍 handoff；
   R56 最后一次 grant/+completion 是 +48/+50 ns，恰好得到 718 的 RTL
   +14 ns admission。下一断点是 715 最后一行 request-vector/RR winner，
   不是给 718 加固定延迟。
3. task20 首条 VRANGE 为 31 cycles，7 个 normalized blocked 和所有
   endpoint 与 RTL 相同；但 blocked winner 第 3 项 gem5 是 CAU-B，RTL
   是 CAU-A，不能只按计数验收。
4. task20 前 30 条 CAU 窗口：enqueue/grant/retire=256/256/30、实际
   result queue 峰值 2；normalized blocked 仍为 gem5 46、RTL 37。
   类别为 gem5 LSU/CAU-A/CAU-B/BitALU-A/BitALU-B/BitALU-result =
   7/6/14/1/6/12，RTL 为 6/11/10/2/4/4。
5. 首个跨指令 divergence 已定位到 1448：gem5 7 blocked，RTL 3，且
   gem5 first result 早约 2 ns。之后同时出现正负 blocked delta 和
   phase 漂移，说明剩余问题是 issue/admission/hazard state 传播，不是
   再调整 RR scalar rule。
6. 定向门：LSU 68/68，382 VINS 与 R55 exact；capacity 六档功能 exact，
   BitALU/CAU/SerDiv 均实际 depth=2/full，R56 的 SerDiv coverage exception
   已消失。
7. nrPDCCH full：24/24 tasks、53/53 returns、10,253 VINS 与 R55 exact；
   全图 +8,088 ns（+0.775%）。task9 +7.351%、task20 -1.318%、task22
   +4.834%、task23 +13.623%，仍有抵消；task17 为 +613 ns（+1.848%）。
8. PDSCHDag2 held-out：9/9 tasks、22/22 returns；7,256 条保留 RTL VINS
   exact，另 847 条仍无 RTL dump。全图 +7,184 ns（+0.468%）；task2/
   task4/task7 为 +8.266%/+4.205%/+20.113%。
9. 当前二进制 SHA256：

       4dd151555e14307972ed18d4514bdadf5ba82226b33333c349684dc59857067e

当前结论：RR 已按 RTL 源码修正，但 task17 operand-B admission 与 task20
跨指令相位仍未闭环。下一步从 task17 +12/+14 requester availability 和
task20 1447->1448 surviving hazard/requester state 入手；必须同时对齐逐指令
blocked/winner 和 stage 边沿。禁止用总 DAG 百分比、task 间抵消、duration
端点或 blocked 总数单独作为接受条件。

======================================================================
当前断点：A32 R56 VSEQ requester / per-bank RR（2026-07-31）
======================================================================

这是历史断点，已被上方 R57 覆盖。完整证据：

    docs/venus_alignment/A32_R56_VSEQ_REQUESTER_RR_ALIGNMENT.md

候选仍使用两个实验开关：

    VENUS_GEM5_EXPERIMENTAL_VRF_RR=1
    VENUS_GEM5_EXPERIMENTAL_REQUESTER_Q_VISIBILITY=1

本轮没有修改 LSU/VFU 固定 latency，也没有 task/PC/DAG/address/payload
特判。

1. task17 sequence-6 VSEQ 已逐边沿对齐：operand admission +8/+14 ns，
   operand grant +50/+54/+56/+58 ns，result enqueue/grant
   +56/+60/+62/+64 ns，lane retirement +66 ns，sequencer recycle +68 ns；
   前七条 duration 均为 12/34/12/29/32/34/34 cycles，与 RTL 完全一致。
2. task17 通过 requester RAW 首脉冲 registered boundary 和 lane-done 到
   sequencer 的一拍可见边界实现上述对齐，不是 latency 拟合。但整 task
   仍为 33,728 ns，对 RTL 33,167 ns 为 +561 ns（+1.691%），下一个
   divergence 已在 sequence 6 之后。
3. task20 已实际建立 stable tagged blocked packet、LSU 独立优先级、
   per-bank pending/selected state、persistent RR，以及 grant 后一次性
   tagged capture。首条 VRANGE 为 31 cycles，完整 fire/enqueue/grant/
   retirement/recycle 边沿与 RTL 相同。
4. task20 对齐前 30 条 CAU 指令的长窗口：enqueue/grant/retire 为
   256/256/30，result queue 峰值 2，均与 RTL 相同；归一化真实 blocked
   从旧 gem5 99 降到 46，但 RTL 是 37，仍多 9，不能晋升默认或宣告完成。
5. 定向门：LSU 68/68，382 条 VINS exact。默认 capacity 路径六档全
   通过且三类队列均 depth=2/full。实验 RR 路径输出 exact，BitALU/CAU
   full，SerDiv 达 occupancy=2 但未发生第三次 enqueue，full 未覆盖，
   自动报告保持 false，未冒充 pass。
6. nrPDCCH full：24/24 tasks、53/53 returns、10,253 VINS 与 R55 接受
   基线 exact；全图 +4,712 ns（+0.452%）。task9 +7.341%、task20
   -2.201%、task22 +4.834%、task23 +13.623%，仍有明显抵消。
7. PDSCHDag2 held-out：9/9 tasks、22/22 returns；保留的 7,256 条 RTL
   VINS 逐元素 exact，另 847 条仍无 RTL dump。全图 +7,148 ns
   （+0.466%）；task2/task4/task7 为 +8.085%/+4.789%/+20.113%。
8. 当前二进制 SHA256：

       65cf04f1681f6039eac511bb7f2cc8d57e880f17d9e23fc172db38ae27323d78

当前结论：task17 sequence 6 已局部完结，task20 仲裁结构已落地但长窗口
仍差 9 个 blocked edge，整 DAG 与逐 task 微时序均未完备。下一步先拆
task17 sequence 7 起的首个 divergence，并把 task20 剩余 46 对 37 按
winner/requester 类别定位；短 task 的 scheduler/DMT 边界继续独立处理。
禁止以全 DAG 百分比或 task 间抵消作为接受条件。

======================================================================
当前断点：A32 R55 LSU tagged completion / addrgen ack（2026-07-31）
======================================================================

这是最新交接断点，优先于下方 R53。完整证据：

    docs/venus_alignment/A32_R55_LSU_COMPLETION_ADDRGEN_ALIGNMENT.md

本轮保留 A47 dispatcher/barrier 以及后续 structured LSU、DMT、tagged
operand/VRF response、hazard-class retirement 和 BitALU/CAU/SerDiv depth-2
result queue；没有改全 DAG、VFU、LSU memory/commit 固定 latency，也没有
task/PC/DAG/address/payload 特判。

1. LSU completion 新增 `running_id + vns_instr_id` tagged lane sideband，
   只更新 lane-local retirement visibility，不重复清全局 hazard。
2. sequencer 成功 schedule LSU 后保持 `DownstreamReceive`，等待两 tile
   cycles 后的 addrgen ack visible edge；对应 RTL IDLE capture -> ADDRGEN
   ack，而不是 latency 拟合。
3. task17 前六条 duration 已与 RTL 相同；首个 duration divergence 移到
   sequence 6 VSEQ：37 对 34 cycles。task17 总时长 33,380 ns，对 RTL
   33,167 ns，+213 ns（+0.642%）。
4. 定向门：LSU 68/68、382 VINS exact；VRF backpressure 1--128 ns 六档
   全通过，BitALU/CAU/SerDiv 均实际达到 occupancy=2/full，enqueue/dequeue
   闭合且输出 hash 不变。
5. nrPDCCH full：24/24 tasks、53/53 returns、10,253 VINS 与变更前控制
   byte-exact；allocation-to-last-release 为 1,045,020 ns，对 RTL
   1,043,224 ns，+1,796 ns（+0.172%）。但 task9 +7.343%、task20
   -2.948%、task22 +4.834%、task23 +13.623%，总量接近主要有抵消。
6. PDSCHDag2 A47 held-out：9/9 tasks、8,103 gem5 VINS；保留 RTL capture
   的 7,256 条逐元素 exact，另 847 条没有保留 RTL dump，不能宣称新的
   8,103/8,103 直接 RTL 比较。全图 1,541,192 ns，对 RTL 1,535,100 ns，
   +6,092 ns（+0.397%）。task2/task4/task7 仍为 +8.121%/+7.891%/+20.113%。
7. 当前二进制 SHA256：

       bca92022835b06d4f9b273a234bc0de4c442e9afff11dcf8cd1c1d55c88bf058

当前结论：功能门和全图 sub-1% 通过，但各 task 微时序尚未完备。下一步
从 task17 sequence-6 VSEQ 拆 operand admission/result enqueue/per-bank VRF
grant/hazard retirement，并继续 task20 stable tagged requester vector、LSU
独立高优先级和 persistent per-bank RR；短 task 的 scheduler/DMT 固定边沿
另行拆分。禁止用 task 间抵消或全局 latency 拟合。

======================================================================
当前断点：A32 R53 requester visibility / retirement ownership（2026-07-30）
======================================================================

这是最新交接断点，优先于下方 R52。完整证据：

    docs/venus_alignment/A32_R53_REQUESTER_RETIREMENT_ALIGNMENT.md

本轮保留 A47 dispatcher/barrier 与 R48--R52 structured LSU、DMT、
tagged operand/VRF response、hazard-class retirement 和 fixed-depth VFU
result queue；没有调整任何全 DAG、VFU 或 LSU 固定 latency。

1. VRANGE decode 现在按 RTL OPMISC 合同保留 vs1/vs2/vd1 三个真实
   operand requester，不再因为 CAU 内部生成数值而省略仲裁/hazard。
2. 建立独立 requester_q 可见边沿：

       VENUS_GEM5_EXPERIMENTAL_REQUESTER_Q_VISIBILITY=1

   与 per-bank RR 原型组合后，task20 首条 CAU 已达到 operand request
   +2 ns、enqueue +8 ns、首拍 blocked、grant +10 ns；RTL 对应值相同。
   gem5 occupancy=2 仍晚一个 tile cycle（+12 ns 对 RTL +10 ns）。
3. 组合原型暴露并修复了通用 WAW bug：VBRDCST 1464 已通过 BitALU
   banked path 写完后，sequencer retirement 又后门重写整向量，覆盖
   younger VSGT 1465 已获 grant 的数据，导致 VSUB 1468 first
   divergence。现在：
   - BitALU timing path 是唯一 VRF writer；
   - retirement 不再 backdoor 改写；
   - VBRDCST VINS 从 tagged completion payload 取快照，不在年轻 WAW
     后重读共享 VRF。
4. 实验候选功能门全部通过：
   - nrPDCCH `10253/10253` exact；
   - capacity 六档 hash exact，BitALU/CAU/SerDiv 均达到 depth=2/full；
   - LSU `68/68`、`382/382` exact；
   - PDSCHDag2 `8103/8103` exact。
5. 但实验候选没有晋升默认。task20 四微秒窗口中，RTL 37 个真实
   blocked result edge，gem5 归一化后仍为 99；task20 时长变为
   496,680 ns，对 RTL 398,523 ns 为 +24.630%。类别差异为：
   - CAU operand 27 对 RTL 21；
   - BitALU operand 22 对 RTL 6；
   - BitALU result 9 对 RTL 4；
   - generic downstream/bank-busy 41，而 RTL 独立 LSU-priority 仅 6。
6. 安全默认只晋升 VRANGE 与 VBRDCST ownership/snapshot 修复，RR 和
   requester-q 原型仍默认关闭：

       /tmp/a32_r53_vrf_requester_retirement_20260730/attempt-003_promoted/gem5.debug
       SHA256 77ab61f8c5db6c46904f81c228e8e6fcfb9dc781521d3b1434269d4234b89234

   nrPDCCH tick 1,046,396,000，10253/10253 exact，task17 49,440 ns、
   task20 389,712 ns；PDSCHDag2 tick 1,526,048,000，9/9 tasks、
   8103/8103 exact。

当前结论：首条 requester/result overlap first divergence 已结构化定位，
但 generic XBar retry 仍把 deferred admission、downstream busy 和真实
bank block 混在一起。下一步必须建立每 lane edge 的完整 stable tagged
request vector，单独注入 LSU high-priority intent，再做每 bank persistent
RR 和一次性 grant capture，把 task20 99->37 关掉后才能晋升；禁止用
task/PC/DAG 分支或全局 latency 拟合。

======================================================================
当前断点：A32 R52 CAU/SerDiv result-edge first divergence（2026-07-30）
======================================================================

这是最新交接断点，优先于下方 R51。完整证据：

    docs/venus_alignment/A32_R52_VFU_EDGE_ALIGNMENT.md

本轮保留 A47 dispatcher/barrier 与 R48--R51 structured LSU、DMT、
tagged operand/VRF response、hazard-class retirement、result-generation
ownership 和 fixed-depth VFU result queue；没有调整任何全 DAG、VFU
或 LSU 固定 latency。

1. 新增 gem5/RTL 边沿归一化工具：

       tools/parse_vfu_edge_trace.py
       tools/parse_rtl_vfu_vcd.py

   可区分 admit、enqueue、dequeue、VRF grant、master_busy、
   bank_backpressure、retirement，并从 RTL requester 向量解出 bank、
   LSU 高优先级、RR contenders 和 winner。
2. task17 RTL 三微秒窗口：248 enqueue、248 grant、0 blocked、62 done、
   max occupancy=1。首条 CAU 的 enqueue 与 grant 同 timestamp；R52 gem5
   也在同 tick enqueue/grant。必须区分 global fire 与 Lane admit：
   RTL issue->release 为 58 ns，gem5 fire->retire 为 70 ns（+12 ns）；
   不能用 admit->retire 的 60 ns 代替端到端边界。
3. task20 首条 CAU：
   - RTL issue->release 56 ns，期间 7 个 blocked result edge；
   - winner 依次为 CAU-B、LSU、CAU-A、BitALU result、LSU、
     BitALU result、LSU；
   - R52 gem5 global fire->retire 46 ns，真实 bank rejection 为 0。

   正确端到端差异是 gem5 快 10 ns。旧的 20 ns 是
   Lane-admit-relative 比较，不等价于 RTL global issue。7×2 ns 缺失
   仲裁阻塞仍是直接证据，但 fire->Lane-admit 与 first-result 相位也
   必须独立建模，不能折成可调的一条 CAU latency。
4. RTL task20 四微秒窗口：256 enqueue、256 grant、37 blocked、30 done、
   max occupancy=2。37 个 blocked 中 6 个来自 LSU 高优先级，其余来自
   CAU/BitALU operand read 或 BitALU result 的逐 bank RR。
5. 结构修复：CAU/SerDiv 的 result_queue_q 在 edge capture 后，同一
   timestamp 进入组合 bank arbiter；每个 CAU/SerDiv result master 每
   lane edge 最多一次 grant。撤掉了 R51 SerDiv 额外整周期的
   enqueueTick 禁止，但 depth=2/full hold 和 tag 均保留。
6. 定向 capacity：

       /tmp/a32_r52_vfu_edge_alignment_20260730/attempt-006/post_edge_capacity

   1/4/16/32/64/128 ns 全通过、输出 hash exact、enqueue/dequeue 闭合；
   BitALU/CAU/SerDiv 均实际达到 occupancy=2/full。
7. structured LSU：

       /tmp/a32_r52_vfu_edge_alignment_20260730/attempt-010/lsu_suite_post_edge
       68/68 PASS，382/382 VINS 与 R51 exact

8. nrPDCCH full：

       /tmp/a32_r52_vfu_edge_alignment_20260730/attempt-008/gem5_task20_post_edge
       natural completion @ tick 1,046,184,000
       24/24 tasks，53/53 returns，10253 VINS 内容 exact

   task17：RTL 33,167 ns，gem5 49,440 ns，delta +16,273 ns。
   task20：RTL 398,523 ns，gem5 389,712 ns，delta -8,811 ns。
9. PDSCHDag2 A47 held-out：

       /tmp/a32_r52_vfu_edge_alignment_20260730/attempt-009/pdsch_post_edge_heldout
       natural completion @ tick 1,526,032,000
       9/9 tasks，22/22 returns，8103/8103 VINS exact

10. 晋级二进制：

       /tmp/a32_r52_vfu_edge_alignment_20260730/attempt-011/promoted/gem5.debug
       SHA256 f20550a342007504984a6b61166d48b6bf29038f783c0d9ffac40b9dbc1b62f3

11. 未晋升的 deferred per-bank RR 原型：
    - attempt-015：nrPDCCH tick 1,222,984,000，task20 首个已知内容差异
      `VADD_1522.txt`；
    - attempt-016：修正同 tick event 顺序后 tick 1,163,280,000，但
      task20 仍出现同类功能差异；
    - 冻结 call-order trace 证明 task20 首个 result edge 在 gem5 只看到
      CAU result port10，RTL 同时存在的 CAU-B operand intent 尚未建模。

   因此不能直接在 transient sendTimingReq 调用上套 RR。该原型仅由
   `VENUS_GEM5_EXPERIMENTAL_VRF_RR=1` 开启，默认关闭。

当前结论：CAU/SerDiv registered capture 后的组合 result grant 相位已按
RTL 结构实现，但微时序仍不完备。下一步先实现稳定 tagged requester
intent、明确 ownership 和 operand/result 同时可见，再实现通用 4-bank、
LSU-over-low-level priority、每 bank persistent RR arbiter，并用 task20
上述 blocked-winner 序列验证；不得按 task/PC/DAG 或全局 latency 调参。

======================================================================
当前断点：A32 R51 CAU/SerDiv fixed-depth tagged result queue（2026-07-30）
======================================================================

这是最新交接断点，优先于下方 R50。完整证据：

    docs/venus_alignment/A32_R51_VFU_RESULT_QUEUES.md

本轮保留 A47 dispatcher/barrier 与 R48--R50 structured LSU、DMT、
tagged operand/VRF response、hazard-class retirement、result-generation
ownership 和 BitALU result queue；没有调整任何全 DAG、VFU 或 LSU 固定
latency。

1. 建立了默认关闭的物理 VRF grant-backpressure 测试：
   - `SimpleMemory.request_gap` 参数化请求接受间隔，默认仍为 1 ns；
   - 只有设置 `VENUS_GEM5_VRF_GRANT_GAP` 时才改变物理 VRF data/mask
     bank 的 grant 间隔；
   - response latency、tile SPM、DMA、task/PC/address/data 均不变。
2. 独立微程序与自动 runner：

       tools/venus_vfu_result_queue_microbench.py
       tools/run_venus_vfu_result_queue_capacity.py
       /tmp/a32_r51_vfu_result_queue_20260730/attempt-009/automated_capacity

   1/4/16/32/64/128 ns sweep 全部正常退出，所有 VINS dump 与 1 ns
   control byte-exact。实际观察到：
   - BitALU：16 ns 起 occupancy=2/full；
   - CAU：4 ns 起 occupancy=2/full；
   - SerDiv：32 ns 首次 occupancy=2，64 ns 首次 full。
3. CAU 现在有独立 depth=2 paired tagged result FIFO，entry 携带
   vd1/vd2、mask、instruction/running tag 和两路独立 handshake 状态；
   full 时保持 arithmetic pipeline 头，不丢结果或 tag。
4. SerDiv 现在有独立 depth=2 tagged result FIFO。迭代完成事件和
   result-queue service event 分离，保留 data-dependent processing time；
   entry 记录 enqueue tick，禁止同边 enqueue/dequeue bypass；full 时保持
   已完成 quotient/remainder。
5. structured LSU：

       /tmp/a32_r51_vfu_result_queue_20260730/attempt-006/lsu_suite_cau_serdiv_result_queue
       68/68 PASS，382/382 VINS 与 R50 exact

6. PDSCHDag2 A47 held-out：

       /tmp/a32_r51_vfu_result_queue_20260730/attempt-007/pdsch_cau_serdiv_result_queue_heldout
       natural completion @ tick 1,526,188,000
       9/9 tasks，22/22 returns，8103/8103 VINS exact
       LSU 四阶段各 1312，strict lifecycle 1312/1312

   相对 R50 增加 752 ns。
7. nrPDCCH full：

       /tmp/a32_r51_vfu_result_queue_20260730/attempt-008/nrpdcch_cau_serdiv_result_queue_full
       natural completion @ tick 1,050,444,000
       24/24 tasks，53/53 returns，10253 VINS 内容 exact
       LSU 四阶段各 797

   task3/task9 并行 allocation 使全局 instruction ID 422/423 互换；按
   owning task 比较内容 byte-exact，不是功能差异。相对 R50 增加
   4,776 ns。
8. 当前二进制：

       build/RISCV/gem5.debug
       /tmp/a32_r51_vfu_result_queue_20260730/attempt-010/promoted/gem5.debug
       SHA256 653f7e1b77016c288a364995abdc0cdbb08a6135bb70937adebe6c660786be42

当前结论：BitALU/CAU/SerDiv 的 depth=2 result queue capacity 已被实际
触发并结构化建模，但仍不能宣称微时序完全建模。下一步应对 nrPDCCH
task17/task20 采集 CAU/SerDiv enqueue/dequeue、VRF grant 和 retirement
边沿，与 RTL 做 first-divergence，不得回到全 DAG latency 拟合。

======================================================================
当前断点：A32 R50 explicit BitALU tagged result queue 通过双 DAG（2026-07-30）
======================================================================

这是最新交接断点，优先于下方 R49。完整证据：

    docs/venus_alignment/A32_R50_BITALU_RESULT_QUEUE.md

本轮保留 A47 dispatcher/barrier 与 R48/R49 structured LSU、DMT、
tagged operand/VRF response、hazard-class retirement 和 result-generation
ownership 修复；没有调整任何全 DAG、VFU 或 LSU 固定 latency。

1. gem5 BitALU 现在有独立 depth=2 tagged result FIFO。entry 携带
   instruction/running tag、data、mask 以及 data/mask 独立 handshake
   状态；queue full 时保持 arithmetic pipe 输出，同拍 dequeue/enqueue
   不丢 tag。
2. task20 first divergence 已定位到首条 VBRDCST：
   - RTL：相对 task start，fire 458 ns、recycle 500 ns；
   - R49：fire 460 ns、recycle 498 ns；
   - R50：fire 460 ns、recycle 500 ns。
   新 registered result boundary 把 recycle 修正一个 tile cycle，不是
   task/PC 分支或 latency 拟合。
3. 定向 trace：

       /tmp/a32_r50_hazard_class_20260730/attempt-010/nrpdcch_task20_result_queue_trace

   观察窗口内 164 enqueue / 164 dequeue，稳态每 2 ns 同拍出入队；
   最大 occupancy=1，因此不能声称该 DAG 覆盖了 result queue full cliff。
4. nrPDCCH full：

       /tmp/a32_r50_hazard_class_20260730/attempt-007/nrpdcch_bitalu_result_queue_v2
       natural completion @ tick 1,045,668,000
       24/24 tasks，53/53 returns，10253/10253 VINS exact
       LSU 四阶段各 797，strict lifecycle 797/797

   task20 从 R49 的 386,632 ns 变为 389,136 ns，向 RTL 398,523 ns
   收敛 2,504 ns；task17 从 49,176 ns 变为 49,412 ns，仍比 RTL
   慢 16,245 ns。全图归一化 delta 为 +2,392 ns，不能用任务间抵消
   代替局部微时序判断。
5. structured LSU：

       /tmp/a32_r50_hazard_class_20260730/attempt-008/lsu_suite_bitalu_result_queue
       68/68 PASS，382/382 VINS exact
       LSU 四阶段各 244，strict lifecycle 244/244

6. PDSCHDag2 A47 held-out：

       /tmp/a32_r50_hazard_class_20260730/attempt-009/pdsch_bitalu_result_queue_heldout
       natural completion @ tick 1,525,436,000
       9/9 tasks，22/22 returns，8103/8103 VINS exact
       LSU 四阶段各 1312，strict lifecycle 1312/1312

7. 晋级二进制：

       /tmp/a32_r50_hazard_class_20260730/attempt-011/promoted/gem5.debug
       SHA256 5aab3d1accd8dc388b7fece1c0a713f47539aade920deaf72578d9751128f547

下一步不能调全局 latency：先建立带 VRF grant backpressure 的 VFU
result-queue capacity 定向测试，实测 occupancy=2/full；再把同一 fixed-depth
tagged instruction/result queue 结构扩到 CAU/SerDiv，并继续拆 task17 与
task20 的 enqueue/dequeue/grant/retirement first divergence。

======================================================================
当前断点：A32 R49 VFU result-generation hold 通过双 DAG（2026-07-30）
======================================================================

这是最新交接断点，优先于下方 R48。完整证据：

    docs/venus_alignment/A32_R49_VFU_RESULT_GENERATION.md

本轮没有撤回 A47 dispatcher/barrier 或 R48 structured LSU、DMT、
tagged operand/VRF response、hazard-class retirement 修复，也没有调整
任何全 DAG/VFU/LSU 固定 latency。

1. 单独把 tagged early admission 从 CAU A-D 扩到 BitALU A/B、
   SerDiv A/B 会让 nrPDCCH task17 变快，但 nrPDCCH 和 PDSCH full 都
   deadlock，因此 admission-only 假设已拒绝。
2. first divergence 在 nrPDCCH instr 1402/1403/1404：
   - 1403 已计算 32/32、写回 31/32；
   - tick 547472000 同一 BitAlu_B requester 接收 1404；
   - younger active generation 调用 `datapipe::setPipeLength()`，清掉
     1403 最后一个 tagged result；
   - 1404 随后永久等待 1403，trace 为
     `progress 31, credit eligible 0`。
3. RTL BitALU 有 4-entry vinsn queue、独立 2-entry result queue，
   result entry 携带 id。R49 结构化保证：旧 tagged result pipe 非空
   时不能切换并重置 active generation，必须先经普通 writeback
   handshake 排空；同时保留 BitALU/CAU/SerDiv tagged command
   admission。没有 task/PC/address/payload 分支。
4. 最终二进制：

       /tmp/a32_r49_task17_20260729/attempt-031/promoted/gem5.debug
       SHA256 452e662e759a4585159ed7d7d89c901ceba42127805725d78e335ab0171273b6

5. nrPDCCH full：

       /tmp/a32_r49_task17_20260729/attempt-028/nrpdcch_bitalu_result_hold_full
       natural completion @ tick 1,043,164,000
       24/24 tasks，53/53 RTL returns PASS
       10253 VINS 与 R48 功能基线逐文件零差异
       LSU 四阶段各 797，strict lifecycle 797/797

   allocation-to-last-release 为 1,043,112 ns；RTL 1,043,224 ns，
   delta -112 ns（-0.011%），相对 R48 的 +12,036 ns 显著收敛。
   但这包含 task 间正负误差抵消，不等于完全微时序：
   task17 仍为 RTL 33,167 ns、gem5 49,176 ns（+48.268%），task20
   则领先 11,891 ns。
6. structured LSU：

       /tmp/a32_r49_task17_20260729/attempt-029/lsu_suite_bitalu_result_hold
       68/68 PASS

7. PDSCHDag2 A47 held-out：

       /tmp/a32_r49_task17_20260729/attempt-030/pdsch_bitalu_result_hold_heldout
       natural completion @ tick 1,525,460,000
       9/9 tasks，22/22 returns PASS，8103/8103 VINS exact
       LSU 四阶段各 1312，strict lifecycle 1312/1312

当前结论：R49 修复了宽 requester admission 暴露出的 tagged result
generation overwrite，并通过双 DAG；但不能宣称微时序完备。下一步必须
显式建模 BitALU/CAU/SerDiv 固定深度 instruction/result queues，
采集 enqueue/dequeue/full/empty 和 retirement 事件，继续拆 task17、
task9/22/23 与 task20 的 first divergence。禁止回到全局 latency 拟合。

======================================================================
当前断点：A32 R48 nrPDCCH 通用修复通过双 DAG held-out（2026-07-29）
======================================================================

这是最新交接断点，优先于下方 A47 tagged-retirement 断点。完整证据：

    docs/venus_alignment/A32_R48_NRPDCCH_ALIGNMENT.md

本轮从冻结 A47
`4a61edd03246e52681ce450bfadd41051b1252b541a7c86de4657b2590222305`
继续；A47 dispatcher/barrier、structured LSU、VFU result hold、tagged
operand/data/VRF response 和 hazard-class admission/retirement 全部保留。
没有调整任何全 DAG、VFU 或 LSU 固定 latency。

1. `nrPDCCH_tv9_a28_full` 的功能 first divergence 依次定位为：
   - task1 GATHER index 正确但 data 为零：manifest 声明的 immutable
     global 没有进入 dense shared-L2；
   - 补齐 type-1/type-2 后，task3 前 367 条 VINS 全 exact，第 367 条
     EW16/VL576 VLOAD 在声明的 type-5 global `0x48c40` 读到零；
   - RTL LSU 在该范围返回 `48,0,49,0,...`，旧镜像全零。
2. 输入修复只在通用 materializer：
   - `tools/venus_l1_dag.py` 按 DAG 声明范围和 byte-complete RTL
     DMA/LSU evidence 物化 immutable global，并保留 canonical blob
     之外的 sparse bytes；
   - `tools/relocate_venus_dag_manifest.py` 从 manifest 的
     `initial_inputs[source.space=shared_l2]` 重建隔离 run-local image；
   - 冲突 alias、canonical overwrite、缺失证据和 32 MiB 越界均
     fail-fast。没有用 task/PC/DAG 名称/数据值改变 gem5 行为。
3. task14 旧停点的 request/response/completion trace 证明同 tick
   reinsertion 让年轻 554 越过 552/553，并形成 destination-hazard
   circular wait。`queuePendingLsu()` 现结构化规定：
   response/complete 先于同 tick request，同 tick request oldest
   `vns_instr_id` first；没有 task/address 分支或 latency 改动。
4. task20 随后暴露 DMT timing-path contract 错误：producer 动态返回
   4096 bytes，而 consumer 固定 prefix 只有 128 bytes。现在
   `consumerBytes` 只表示 consumer prefix，不再被错误当成 producer
   return capacity，与非 timing `publishOutput()` 语义一致。
5. 最终二进制：

       build/RISCV/gem5.debug
       SHA256 1b55e6a1d890f41ae3154a68098e80873e3170a2f4fb78c730ce6797d583afad

6. 定向验证：

       /tmp/a32_nrpdcch_alignment_20260729/attempt-020/lsu_suite
       68/68 PASS，覆盖 RAW-chain、independent throughput、
       load/store N-2..N+2 capacity cliff；
       所有 accept/request/response/complete 闭合，最终 outstanding=0。

   四 tile LSU logger 也改为进程内共享单调 JSONL，不再由四个
   sequencer 同时 truncate 同一文件。task0..3 probe 为 85/85/85/85，
   0 malformed、0 lifecycle gap：

       /tmp/a32_nrpdcch_alignment_20260729/attempt-021/task3_logger

7. 最终 nrPDCCH full：

       /tmp/a32_nrpdcch_alignment_20260729/attempt-023/nrpdcch_final
       natural completion @ tick 1,055,312,000
       24/24 tasks
       53/53 RTL captured return ports PASS
       LSU accept/request/response/complete 各 797
       peak outstanding total/load/store = 8/5/4，最终 0/0/0

   task3 定向 VINS 为 370/370 byte-exact。功能已对齐，但微时序尚未
   完全对齐：allocation-to-last-release RTL 1,043,224 ns，gem5
   1,055,260 ns，delta +12,036 ns = +1.154%。当前最大显式 task
   偏差是 task17：RTL 33,167 ns，gem5 49,390 ns（+48.913%）；
   task6/7 还有 allocator tile-choice swap。不得把这些差异用全 DAG
   latency 隐藏。
8. PDSCHDag2 A47 held-out 用同一最终 binary 重新通过：

       /tmp/a32_nrpdcch_alignment_20260729/attempt-022/pdsch_heldout
       natural completion @ tick 1,526,008,000
       22/22 returns byte-exact
       8103/8103 VINS byte-exact
       LSU 四阶段各 1312，strict lifecycle 1312/1312，最终 outstanding=0

当前结论：本轮修复是 manifest input、LSU arbitration、DMT dynamic
return 和多 tile observation 的通用结构修复，不是 nrPDCCH task 定制。
nrPDCCH 与 PDSCH 功能 held-out 均通过，但“完全微时序建模”仍不能宣称。
下一 first-divergence 应从 nrPDCCH task17 的 requester/result/retirement
timing 和 task6/7 allocator tie 开始，禁止重新调全 DAG 固定 latency。

======================================================================
当前断点：A32 R47 tagged retirement / LSU WAR 生命周期通过全 held-out（2026-07-29）
======================================================================

这是最新交接断点，优先于下方较早的 hazard-class 断点。A47
dispatcher/barrier、structured LSU、VFU result hold、tagged operand
queue 和 RAW/WAR/WAW split 均保留；本轮没有调整任何全 DAG 或 VFU/LSU
固定 latency。

1. PDSCH task 3 的前序 deadlock 已定位并结构化修复：
   - CAU 1057/1058 共享 result pipe 的 shape 现在归属仍在飞行的 tagged
     result generation；年轻命令不能在旧结果排空前改 pipe length；
   - `vrfReadGenerationMatches()` 现在对 passage response 校验发起请求的
     `queueing_*_instr_pkt`，不再错误地拿 VFU/operand-queue head generation
     丢弃 1058 的合法响应；
   - 修复后 1058 的两个 operand stream 均完整消费，并越过旧的 1147
     停点。
2. 随后的 held-out 首个功能分歧不是算术结果，而是 LSU WAR 生命周期：
   - task 3 `VSTORE_1061` 后的输出从 byte 49 开始错误；
   - producer `VSADD_1059` 的 operand、result 和逐 lane writeback 与接受
     A47 全部一致；
   - 年轻 `VSADD_1066` 写同一 `vd16..20`，lanes 6..15 的首拍在
     `VSTORE_1061` response 前提交；首坏 logical element 48 正好是 lane 6
     的第一个 EW8 元素；
   - 因而 first divergence 被定位为“年轻 writer 在 LSU victim 完成读取前
     retirement”，而不是 LSU response latency 或 CAU 算术。
3. retirement 现按 generation 和 hazard class 结构化传递：
   - sequencer 在 running-ID 分配后直接按 active 指令读/写区间生成
     WAR/WAW victim 快照，并随 writer instruction tag 发往所有 lane；
   - lane requester 只能合并，不能用同拍到达的旧 hazard broadcast 覆盖
     权威快照；
   - `VenusHazardTable` 携带每个 running ID 的单调 retirement
     tombstone；`retired_generation >= victim_generation` 才能证明一个
     快速复用的 victim 已完成；
   - 快照额外携带 victim class。普通 VFU victim 仍使用逐 lane
     read/write progress；LSU victim 从不经过 lane requester，因此禁止
     读取复用 running ID 留下的陈旧 progress，只由 LSU completion
     tombstone 释放。
4. 定向 first-divergence 证据：

       /tmp/a32_microtiming_20260729/attempt-054/requester_generation_fix
       /tmp/a32_microtiming_20260729/attempt-060/accepted_a47_task3_trace
       /tmp/a32_microtiming_20260729/attempt-068/lsu_retirement_class

   修复前 `VSADD_1066` 首次 VRF commit 为 tick 1,150,549,000，早于
   `VSTORE_1061` completion/recycle 1,150,604,000；最终实现的首次 commit
   为 1,150,765,000。最终 `VSTORE_1061`、`VSTORE_1062` 与接受 A47
   逐字节一致。
5. 最终候选二进制：

       /tmp/a32_microtiming_20260729/attempt-069/promoted_candidate/gem5.debug
       SHA256 4a61edd03246e52681ce450bfadd41051b1252b541a7c86de4657b2590222305

   当前 `build/RISCV/gem5.debug` 与该冻结候选 SHA256 相同。
6. 定向验证全部通过：

       attempt-070 Task1 strict: 6/6，first divergence = null
       attempt-071 LSU suite: 68/68
       attempt-072 b2b load/store: 6/6 + 6/6，零容差事件 exact

   suite 覆盖独立 LSU RAW-chain、independent-stream throughput 和
   N-2..N+2 outstanding capacity cliff；代表性 RAW/capacity JSONL 与
   requester-generation 修复前候选逐字节不变。
7. 最终 PDSCHDag2 A47 held-out：

       /tmp/a32_microtiming_20260729/attempt-073/pdsch_a47_heldout
       natural completion @ tick 1,524,632,000
       22/22 task return payloads byte-exact
       8103/8103 VINS files byte-exact，IDs 0..8102 无缺失/重复

   比对参考为：

       /tmp/a32_hazard_class_20260729/attempt-008/pdsch_final_source

   LSU trace 共 5,249 条：metadata 1，accept/request/response/complete 各
   1,312；674 loads、638 stores；峰值 outstanding 为 total 6 /
   load 4 / store 3，最终全部归零。每 task VINS 仍为
   `1,6,841,289,193,40,44,0,6689`。

当前结论：A47 已覆盖的 dispatcher/barrier、VFU tagged data/result、
hazard-class admission/retirement 和 structured LSU 边界，已经共同通过
定向微测试和完整 PDSCH functional held-out。仍不能把这等同于“任意
workload 的全部微时序已完备”；若要宣称完全建模，下一 gate 应是独立
LDPC/第二套 DAG、更多同拍 arbitration/queue-full 组合以及逐 task RTL
requester/result/retirement trace，而不是重新调固定 latency。

======================================================================
当前断点：A32 R47 hazard-class admission/retirement 已拆分并通过 held-out（2026-07-29）
======================================================================

本节是新聊天窗口的直接交接上下文。后面的项目总纲和验收标准仍然有效，
不要删除或弱化。

2026-07-29 hazard-class 续作（最终证据 attempt-006..008）：

1. 已冻结进入本轮前的源码和二进制：

       /tmp/a32_hazard_class_20260729/attempt-001/baseline
       baseline binary SHA256:
         a2cdf3ec6380be8593ff831b2f822e0e444998c60c184edfd4e8575d7900b0ab

2. RTL read-only 对照确认 `venus_operand_requester.sv` 的 requester
   内部保存 hazard bitmap 和逐 producer 的一位 RAW credit；credit 由
   producer writeback progress transition 产生，operand grant 后消费，
   不累积。旧 gem5 把 source-side RAW 与 destination-side WAR/WAW
   折叠在 `vd1_hazard_table`/`vd2_hazard_table` 和
   `checkifreadytofire(..., true)` 中；requester credit 却只能兑现
   RAW，因此 admission 与 retirement 生命周期不对称。
3. 已结构化拆分：
   - `src/venus/venus_instr_pkt.hh:173-175` 增加独立
     `raw_hazard_table`、`war_hazard_table`、`waw_hazard_table`；
   - `src/venus/VenusSequencer.cc:1637-1768` 在构表时显式分类
     VS1/VS2/VD-input RAW、destination WAR 和 destination WAW；
   - `src/venus/venus_instr_pkt.hh:239-258` 的
     `checkRawOnlyChainAdmission()` 只允许 pure RAW 早期 admission；
   - 只有 pure RAW、cross-VFU、non-mask、non-shuffle、non-LSU 依赖
     可以早期进入 operand requester；
   - 任何 WAR/WAW 或 RAW+WAR/WAW mixed dependency 都继续等待
     destination retirement，不能借 RAW row credit 越过；
   - `src/venus/VenusLane.cc:565-639` 在 admission 时按 requester
     保存 RAW producer generation，同时保留 WAR/WAW retirement
     victim generation；
   - `src/venus/VenusLane.cc:764-825` 使用 RTL 风格的一位、非累积
     RAW progress credit，grant 后消费；
   - `src/venus/VenusLane.cc:2383-2418` 分开 retirement clock：
     WAR 只看 victim read progress，WAW 只看 victim write progress，
     不再用 `max(read_progress, write_progress)` 提前释放。
4. 没有调整 VFU/LSU/DAG 固定 latency，没有撤回 A47
   dispatcher/barrier、structured LSU、VFU result hold、tagged
   operand queue 或 tagged VRF response。
5. 最终源码、不可变二进制和哈希：

       /tmp/a32_hazard_class_20260729/attempt-006/source
       /tmp/a32_hazard_class_20260729/attempt-006/gem5.debug
       SHA256 512867868ad81297553b2a59bd3db1d5a65161f330aeefbf74e063634476e4bd

6. 定向验证：

       /tmp/a32_hazard_class_20260729/attempt-007/final_source_lsu_suite

   68/68 case PASS，382/382 VINS；覆盖 RAW-chain、independent-stream
   throughput 和 N-2..N+2 outstanding capacity cliff，并与
   attempt-085 的 382 个 VINS 全部 byte-exact。
7. first-divergence 聚焦验证：

       /tmp/a32_hazard_class_20260729/attempt-002/task3_split_v1
       /tmp/a32_hazard_class_20260729/attempt-008/pdsch_final_source

   task3 的 1048..1055 全部与 attempt-084 byte-exact，包括旧首错
   VSSUB_1052、VDIV_1054，以及 transition-only 版本曾停住的
   VSSUB_1053、VDIV_1055；task3 全部 289 个 VINS 也 byte-exact。
   相对 gated A47 monitor 的首个时序分歧为 instruction 872：
   VBRDCST fire tick 相同，严格 destination retirement 令 recycle
   从 733,408,000 变为 733,410,000（+1 tile cycle）；紧接的
   cross-VFU VDIV_873 因 pure-RAW requester admission，recycle 从
   733,740,000 变为 733,726,000（-7 tile cycles）。这把差异定位在
   hazard-class 生命周期，而不是固定 latency。
8. PDSCHDag2 A47 held-out：

       /tmp/a32_hazard_class_20260729/attempt-008/pdsch_final_source
       natural completion @ 1,538,912,000 ticks
       22/22 returns 与独立 A47 attempt-078 byte-exact
       8103/8103 VINS 与 attempt-084 byte-exact
       LSU trace: metadata 1 + accept/request/response/complete 各 1,312
                  = 5,249 events

   相对 fresh RTL functional span 1,535,100 ns，当前 gem5 为
   1,538,912 ns，delta +3,812 ns = +1,906 tile cycles = +0.2483%。

当前可晋级结论：result hold、tagged operand/data/VRF response、
structured LSU 与 conservative hazard-class split 已共同通过完整
functional held-out。仍不能宣称全部微时序建模完备；当前 mixed
RAW+WAR/WAW 依赖采取保守 retirement gate。若继续收敛剩余 L2/L3，
应建立 per-requester mixed-class trace/retirement，而不是重新调全
DAG latency 或扩大 RAW progress lead。

2026-07-29 续作（attempt-001..011）：

1. 已结构化实现 operand command/data 配对：所有 BitALU/CAU/SerDiv
   data FIFO 携带 `{running_id, vns_instr_id}`，pop 时同时校验 generation；
   VFU 只有在该指令所需的全部 command FIFO 对齐后才消费。
2. `datapipe` 已增加独立 valid bit；result 是否有效不再用 `INT_MIN`
   猜测。output nack 时保持 result/tag，ACK 后显式清 valid、data、
   mask 和 packet，修复旧的悬空 result pointer。
3. VFU->VRF write request/response 现携带
   `{running_id, vns_instr_id}` sender tag，可分别观察 accepted 与
   committed 事件；没有改任何 VFU/LSU 固定 latency。
4. 在 tagged queue/result hold 上重新启用 requester-local RAW credit：
   attempt-006 的 producer-store 精确得到 accept 24 / request +14 /
   response +70 / complete +77，attempt-005 为 68/68、382 VINS exact。
5. held-out first divergence 已定位：
   - 仅按 progress transition 发 credit：task3 在 1053/1055 晚到
     consumer 处停住；
   - 用当前 progress 补发 credit：DAG 能自然完成 8103 条，但首个
     错误为 VSSUB_1052，随后 VDIV_1054；
   - 1053/1055 sibling stream 仍 byte-exact，证明不能靠统一 lead、
     fixed latency 或 capacity 数字修补。
6. 根因边界收窄到 hazard-class admission：当前
   `checkifreadytofire(..., true)` 同时忽略 RAW 和 WAR/WAW，而逐
   requester credit 只覆盖 RAW 生命周期。因此实验 overlap 已重新
   关在 gate 后；A47 dispatcher/barrier 和 structured LSU 未撤回。

当前源码保留 result hold、tagged operand queue 和 tagged VRF
request/response；跨指令 overlap 默认关闭，下一步必须拆分 RAW 与
WAR/WAW admission/retirement，再跑 PDSCH held-out，禁止继续调
progress lead 或全 DAG latency。

最终 gate 验证：

    binary SHA256:
      a2cdf3ec6380be8593ff831b2f822e0e444998c60c184edfd4e8575d7900b0ab
    attempt-012 PDSCH:
      natural completion @ 1,539,132,000 ticks
      22/22 returns、8103/8103 VINS 与 attempt-084 byte-exact
    attempt-013 LSU suite:
      68/68、382/382 VINS 与 attempt-085 byte-exact

最新续作：

    docs/venus_alignment/A32_R47_STRUCTURED_LSU_ALIGNMENT.md

本轮实际结论：

1. RTL trace 证明 producer-store 首分歧不在 LSU 固定 latency：
   VADD 的 CAU A/B requester 都在 cycle 2769 接收命令，producer row
   writeback 为 2761..2799，store 为 accept 2771 / request +14 /
   response +70 / complete +77。
2. gem5 提前接收 CAU requester 后，单链可以精确得到 +14/+70/+77；
   68-case 也能 68/68。
3. 该实现不能晋级：PDSCH task 3 首个功能分歧是 VDIV_1054；改成 RTL
   一位 credit 后又在 VSSUB/VDIV 边界死锁。根因是 gem5 当前未完整
   建模 operand command/data queue 与 VFU result hold/VRF arbitration，
   不能安全承载所有跨 VFU row overlap。
4. 失败尝试均保留在 attempt-062/064/066/069/071/073/075；没有用
   固定 DAG latency 或 LSU magic delay 掩盖，也没有改动 A47
   dispatcher/barrier 修复。
5. 默认已关闭未通过的 lane requester overlap。最终默认二进制：

       SHA256 188de0052f5849f9d5330788f94abbc023eb43b3732bd5d3f34df71d77e4efb9

   最终验证：

       attempt-084 PDSCH: 22/22 returns、8103/8103 VINS byte-exact
       attempt-085 LSU suite: 68/68，382/382 VINS 与 attempt-048 byte-exact

当前不能宣称 gem5/RTL 建模完备。下一步必须先结构化实现 VFU result
valid/ready 保持、双 operand queue 的命令/数据配对和逐 requester
hazard-credit 生命周期，再重新启用 chaining；不要重调全 DAG 固定 latency。

已完成：

1. 建立 RAW-chain、independent-stream throughput、N-2..N+2 capacity cliff；
2. RTL/gem5 同构采集 LSU accept/request/response/completion；
3. gem5 实现 Request / StoreOperands / Response / Complete 状态；
4. 显式建模 LDU/STU depth=4、独立入口 II、共享 channel、beat/row、
   LDU held addrgen 和 STU admission backpressure；
5. 68/68 定向 smoke 运行成功；
6. PDSCHDag2 held-out 保持 22/22 输出、8103 VINS、LSU 次数和最大
   outstanding 与 RTL 一致。

held-out 全 span：

    RTL                 1,535,100 ns
    structured LSU gem5 1,539,108 ns
    delta                   +4,008 ns = +2,004 cycles = +0.2611%

剩余首分歧：

    EW16/VL256 producer->store 中，RTL VADD 数据在 store accept 后
    43 cycles ready，gem5 为 59 cycles；STU response 因而 70 vs 86。
    基础 store 和无依赖长 store 的 LSU phase 已精确。下一项应隔离
    VADD/requester readiness，不得缩短 LSU latency 抵消这 16 cycles。

一、当前仓库与基线
------------------

RTL：

    /home/shenyihao/Project/Venus_3/venus_soc

gem5：

    /home/shenyihao/Project/Venus_3/gem5-freertos

目标配置：

    PDSCHDag2 A32 R2
    Venus 2.0，16 lanes x 128 rows
    tile clock = 500 MHz，1 cycle = 2 ns

新鲜 RTL reference：

    /home/shenyihao/Project/Venus_3/venus_soc/sim/build_gc0802_overall_PDSCHDag2_A32_R2_overall_PDSCHDag2_A32_R2

RTL 全 DAG 命令：

    cd /home/shenyihao/Project/Venus_3/venus_soc
    make sim_compileall SIM_NAME=gc0802_ TESTBENCH_NAME=overall \
      SIM_TYPE=overall_PDSCHDag2 COPY_ID=overall_PDSCHDag2 \
      TARGET_BIN_FILE_NAME=PDSCHDag2

RTL 单 Task 命令模板：

    cd /home/shenyihao/Project/Venus_3/venus_soc
    make sim_compileall SIM_NAME=tile_ SIM_TYPE=simonly \
      COPY_ID=REG_Test_log TARGET_BIN_FILE_NAME=REG_Test/simple_test

gem5 构建命令：

    cd /home/shenyihao/Project/Venus_3/gem5-freertos
    scons build/RISCV/gem5.debug -j8

当前源码和 `build/RISCV/gem5.debug` 是 A47。冻结二进制：

    /tmp/a32_r47_gem5.debug
    SHA1 efcccf8a4b724c25a90eaa1d91a453dadea88a52

当前源码二进制与冻结 A47 的 SHA1 已确认一致。

注意：gem5 工作树有大量此前积累的未提交修改，均属于用户项目状态。禁止
reset、checkout、覆盖或清理无关修改。

二、A47 已确认结论
------------------

L0 功能：

    22/22 DAG 输出 byte/bit-exact

L1 Venus instruction 数量：

    Task0       1
    Task1       6
    Task2     841
    Task3     289
    Task4     193
    Task5      40
    Task6      44
    Task7       0
    Task8    6689
    Total    8103

gem5 issue counter 0..8102 每个恰好出现一次，无缺失、无重复。

L3 时间：

| Task | RTL ns | gem5 A47 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 676,287 | 678,780 | +2,493 | +0.369% |
| 1 | 79,131 | 78,956 | -175 | -0.221% |
| 2 | 55,227 | 56,776 | +1,549 | +2.805% |
| 3 | 471,571 | 474,688 | +3,117 | +0.661% |
| 4 | 30,175 | 30,504 | +329 | +1.090% |
| 5 | 11,059 | 11,528 | +469 | +4.241% |
| 6 | 3,795 | 3,948 | +153 | +4.032% |
| 7 | 1,059 | 1,304 | +245 | +23.135% |
| 8 | 330,955 | 329,252 | -1,703 | -0.515% |

全 DAG functional span：

    RTL       1,535,100 ns = 767,550 tile cycles
    gem5 A47  1,535,992 ns = 767,996 tile cycles
    delta          +892 ns = +446 tile cycles = +0.0581%

这个结果只能宣称：

    L0 PASS
    L1 dispatch/count partial PASS
    L3 已进入 sub-1%

不能宣称完整 L1/L2 对齐。RTL trace 区分 GATHER/SCATTER，gem5 monitor 目前
都显示为 VSHUFFLE；LSU request/response/completion 事件也尚未同构。

三、A47 修复与根因
------------------

RTL `venus_dispatcher.sv` 在 scalar core 和 Venus sequencer 之间实例化
`Bypass=0` 的 `spill_register`。其 `spill_register_flushable.sv` 实际包含
A/B 两项存储，而不是单一 ready/valid 直连。

A42 曾直接加入两项 dispatcher FIFO，但 barrier 只观察
`vinsn_running_q`。gem5 将 RTL 寄存器边界拆成独立事件后，请求仍在
dispatcher 或 crossing into PE-running 时出现了 model-only idle pulse。

A42 的首个功能后果：

    Task3 第二条 VLOAD 期望：1152, 1154, 1156, ...
    A42 实际：             2,    6, 1156, ...

`2, 6` 来自上一条三元素 VLOAD。随后 Task3 标量控制流分叉，只发出
2/289 条 VINS。A42 的“加速”因此是伪结果。

A47 修复：

1. 显式实现 depth=2 的 dispatcher A/B spill；
2. scalar retirement 跟随 spill admission；
3. FIFO head 独立重试最终 sequencer admission；
4. barrier busy 连续覆盖：
   - dispatcher FIFO；
   - `rtlIssuePending`；
   - `rtlPeRunningQ`；
   - `rtlRunningQ`；
5. 不包含 task ID、PC、opcode sequence 或 payload 特判。

关键 gem5 代码：

    src/venus/VenusSequencer.hh:133
    src/venus/VenusSequencer.cc:323
    src/venus/VenusSequencer.cc:927
    src/venus/VenusSequencer.cc:1259

对应 RTL：

    hardware/venus_extension/venus_dispatcher.sv:38-68
    hardware/venus_extension/common_cells/spill_register_flushable.sv:36-95
    hardware/spiritrv32/scalar600_id_stage.sv:255-280
    hardware/spiritrv32/scalar600_id_stage.sv:852-857

shuffle read response 仍保持 RTL 非旁路一周期：

    src/venus/VenusShufflePipline.cc:675-686

不要撤回 A47 dispatcher/barrier 修复。

四、可复现实验与证据路径
------------------------

A47 完整运行：

    /tmp/a32_r47_dispatch_barriercov_full

主要文件：

    /tmp/a32_r47_dispatch_barriercov_full/run.log
    /tmp/a32_r47_dispatch_barriercov_full/venus_dag_manifest.json
    /tmp/a32_r47_dispatch_barriercov_full/venus_dag_trace.jsonl
    /tmp/a32_r47_dispatch_barriercov_full/debug/venusgem5_sequencer_monitor.json
    /tmp/a32_r47_dispatch_barriercov_full/debug/venusgem5_vins_result

RTL per-tile JSONL：

    .../venus_full_dag_perf_cluster0_tile0.jsonl
    .../venus_full_dag_perf_cluster0_tile1.jsonl
    .../venus_full_dag_perf_cluster0_tile2.jsonl
    .../venus_full_dag_perf_cluster0_tile3.jsonl

上面的 `...` 是：

    /home/shenyihao/Project/Venus_3/venus_soc/sim/build_gc0802_overall_PDSCHDag2_A32_R2_overall_PDSCHDag2_A32_R2

22 个 fresh RTL 功能 evidence：

    /tmp/a32_r30_evidence

详细状态记录：

    docs/venus_alignment/A32_R47_DISPATCHER_ALIGNMENT.md

总 conformance contract：

    docs/rtl_gem5_conformance.md

已有比较工具：

    tools/compare_dag_dma.py
    tools/compare_dag_lifecycle.py
    tools/compare_scalar_retire.py
    tools/compare_scalar_timing.py
    tools/compare_scalar_traces.py
    tools/compare_vins_outputs.py

五、已拒绝的实验，不要重复
--------------------------

A42：

    两项 dispatcher spill，但 barrier 未覆盖 pending/crossing 状态。
    Task1/3/4 功能失败；Task3 仅 2/289 VINS。

A44/A45：

    将 shuffle response 提前到更早事件相位。
    功能仍通过，但全 DAG 从 +1.659% 变差到 +1.809%，Task8 变差。

R46：

    shuffle response delay=2/3/4。
    在 Task0 即触发 lane response spill overflow。
    RTL-backed delay=1 是唯一合法点。

R48：

    将 VLOAD/VSTORE 固定 latency 扫为 10/30/50 cycles。

结果：

| Fixed LSU cycles | Full DAG error | Task8 error |
| ---: | ---: | ---: |
| 3（当前默认） | +0.058% | -0.515% |
| 10 | +0.081% | -0.511% |
| 30 | +0.282% | -0.257% |
| 50 | +1.201% | +3.087% |

这些点都保持 22/22 功能通过，但固定 latency 只是在 Task3、Task8 和总周期
之间搬移误差。禁止选择一个“看起来好”的常数作为最终修复。

六、为什么下一项必须是结构化 LSU
-------------------------------

gem5 当前实现：

    src/venus/VenusSequencer.cc:789-839

`scheduleLsuDone()` 会立即执行整条 LSU 数据移动，然后使用：

    VenusLsuLoadBaseLatency  = 3 cycles
    VenusLsuStoreBaseLatency = 3 cycles

安排统一 completion event。这没有表达：

- addrgen request/ack；
- lane/beat 拆分；
- load/store 独立 backpressure；
- maximum outstanding；
- bank/port contention；
- response merge；
- completion 条件；
- VL 相关曲线；
- 同周期 enqueue/dequeue 优先级。

RTL Task8 的观察值：

    VLOAD  559 次，平均 fire->recycle 约 45.93 cycles
    VSTORE 559 次，平均 fire->recycle 约 58.36 cycles

但这些寿命还包含依赖、仲裁和资源等待，不能直接写成 gem5 固定 latency。

需优先阅读 RTL：

    hardware/venus_extension/vlsu.sv
    hardware/venus_extension/vldu.sv
    hardware/venus_extension/vstu.sv
    hardware/venus_extension/addrgen.sv
    hardware/venus_extension/venus_sequencer.sv
    hardware/venus_extension/venus_lane_sequencer.sv
    hardware/venus_extension/venus_dspm.sv

同时沿着实际 elaborated 参数、ready/valid、queue full/early-full、
request ID、response ordering 和 done condition 追踪，不要只读模块注释。

七、新窗口立即执行的 LSU 工作
----------------------------

目标：先建立定向 microbenchmark 和统一事件证据，再修改 LSU 模型。
PDSCHDag2 A32 R47 只能作为 held-out 回归，不能继续作为固定 latency 校准集。

1. RAW-chain latency

   分开测试 VLOAD 和 VSTORE；构造严格依赖，禁止独立工作覆盖 latency。
   对 VLOAD 至少测 load->consumer/VRF-read 和 load->barrier。
   对 VSTORE 测 producer->store 和 store->barrier/task-complete。

2. Independent-stream throughput

   使用不同 VRF destination/source 和独立地址，逐渐增加并行流。
   观察 accept、addrgen、lane request、response、done 的稳定间隔。

3. Outstanding capacity cliff

   用可控长响应延迟阻塞释放，密集扫描容量边界。先从 RTL 参数/状态位推导
   候选深度，然后在 N-2、N-1、N、N+1、N+2 周围测试，不要只测很稀的点。

4. 首轮建议 sweep

   VL：

       1, 2, 15, 16, 17, 31, 32, 33, 63, 64, 65, 128

   后续再覆盖 144、215、216、431、432、576、1024、2048、4320、7488。

   每个 VL 分开测：

       VLOAD / VSTORE
       EW8 / EW16
       aligned
       same-bank / different-bank（地址映射确认后）

5. 必须采集的边界事件

   - scalar/dispatcher accept；
   - sequencer accept；
   - running ID allocation；
   - addrgen request/ack；
   - 每 lane 或每 beat 的 LSU request；
   - memory request/response；
   - VRF write 或 store commit；
   - final LSU done；
   - barrier release；
   - scalar retire/task complete。

6. 每个失败先报告

   - first divergent cycle；
   - first divergent event；
   - 前后至少 10 个相关事件；
   - RTL 信号/状态；
   - gem5 状态；
   - 单一可证伪假设。

7. 修改要求

   - 先补 queue/state/backpressure/completion mechanism；
   - 禁止用 task、PC、opcode sequence 或 payload 特判；
   - 禁止用单一 magic latency 抵消其他误差；
   - 不能修改 RTL、任务软件或 golden output；
   - 每个结构修复必须有定向 regression；
   - 保留 A47 22/22 和 8,103 VINS。

八、LSU 阶段验收
----------------

L0：

    所有定向测试和 A47 22 个输出必须 bit-exact。

L1：

    LSU accept/request/response/VRF-write/store-commit/done 的数量和顺序一致。

L2：

    RAW latency、steady-state II、outstanding cliff、first/last response 周期一致。
    如 trace observer 存在固定 0/1-cycle offset，必须全测试统一并写入 schema。

L3 held-out：

    最后才重跑 PDSCHDag2 A47。
    不仅比较 full DAG 和 Task8，还必须比较每个 Task、LSU counters、
    outstanding 分布和 stall 原因，防止误差抵消。

当前断点最重要的一句话：

    A47 dispatcher 已闭环；不要再调总周期。下一步用独立 LSU cliff tests
    找出第一个 request/response/completion 分歧，再结构化实现 LSU。

======================================================================
以下为原项目总纲、工作约束和完整 Definition of Done
======================================================================

你现在接手一个长期、工程化的 RTL—gem5 严格对齐项目。请把下面内容视为项目总纲、工作约束和验收标准。不要只提供建议、阅读清单或泛泛的架构说明；你需要实际阅读服务器上的 RTL、旧 gem5 模型、构建脚本和测试，逐步建立自动化对齐闭环并实施修改。

请始终用中文与我沟通。代码、变量名、提交信息可以使用英文。

======================================================================
一、项目背景
======================================================================

我们有一个自研硬件架构，名称为 Venus。

Venus 是一个类似 Ara 的 RISC-V 向量处理器/向量协处理器，具有 lane、VRF、向量功能单元、向量 LSU、mask、slide、reduction 等典型结构，但具体实现以服务器上的 Venus RTL 为准，不能直接假设它等同于 Ara。

此前已经开发过一个 Venus 的 gem5 模型，但存在以下问题：

1. 模型开发得比较早，Venus RTL 后来已经多次升级。
2. 旧 gem5 实现比较潦草。
3. 很多地方可能只是给向量指令配置固定 latency，没有建模真实的：
   - 队列和缓冲区；
   - ready/valid 和 backpressure；
   - VRF bank/port 冲突；
   - lane 级执行；
   - chaining/forwarding；
   - LSU outstanding 和访存拆分；
   - mask/slide/reduction；
   - replay、仲裁和恢复；
   - 同周期状态更新顺序。
4. 当前 gem5 中的标量 CPU 也没有精确建模。
5. Venus 架构和 RTL 本身可能还存在设计问题。
6. 最终目标不是“把 gem5 调到几个 benchmark 的 IPC 看起来差不多”，而是得到一个能够忠实代表指定 Venus RTL 版本的 gem5 模型。
7. 希望逐步使用 AI 自动完成：
   - 阅读 RTL 与 gem5；
   - 提取微架构规格；
   - 生成定向微基准；
   - 运行 RTL 和 gem5；
   - 对比 trace；
   - 定位第一个行为分歧；
   - 判断是参数错误、抽象缺失、实现 bug 还是 RTL 问题；
   - 修改 gem5；
   - 自动回归。

RTL 和 gem5 都在当前服务器上，而不在用户本地。

======================================================================
二、服务器路径
======================================================================

下面路径如果已经填写，直接使用；如果为空，先在当前工作目录和常见项目目录中搜索，不要立即要求用户手工查找。

VENUS_RTL_ROOT=
VENUS_GEM5_ROOT=
VENUS_TEST_ROOT=
VENUS_TOOLCHAIN_ROOT=
VENUS_RTL_BUILD_COMMAND=
VENUS_RTL_RUN_COMMAND=
VENUS_GEM5_BUILD_COMMAND=
VENUS_GEM5_RUN_COMMAND=
VENUS_REFERENCE_MODEL=
VENUS_WORKLOAD_ROOT=

如果无法自动找到，才向用户询问最少量的必要信息，优先询问：

1. Venus RTL 根目录；
2. 旧 Venus gem5 根目录；
3. 一个当前能通过 RTL 仿真的最小测试命令；
4. 一个当前能运行旧 gem5 的最小测试命令；
5. Venus 使用的 ISA/自定义扩展文档位置。

不要一次提出大量泛泛问题。能从代码、Makefile、README、CI、shell history、构建脚本和已有输出中获得的信息，自己调查。

======================================================================
三、你的角色
======================================================================

你是 Venus RTL—gem5 对齐工程师，同时承担以下职责：

1. 微架构建模工程师；
2. RTL 阅读和行为提取工程师；
3. gem5 C++/Python/SimObject 开发工程师；
4. 验证基础设施开发工程师；
5. 微基准生成工程师；
6. trace 和性能差异分析工程师；
7. 回归测试维护者；
8. 架构问题审计者。

你必须以证据驱动工作。

事实优先级为：

1. 可复现的 Venus RTL 仿真结果；
2. 已确认的 Venus 微架构规范和接口协议；
3. RTL 源码、生成参数、宏和 elaboration 后配置；
4. 已通过审查的设计文档；
5. gem5 当前实现；
6. 注释、旧文档和历史模型；
7. 你的推测。

RTL 是当前实现的行为 oracle，但 RTL 不一定代表正确的架构设计。

当 RTL 行为与设计规范冲突时：

- 不要擅自修改 RTL；
- 不要为了让 gem5 “通过”而无条件复制明显错误；
- 分别记录：
  - `RTL implemented behavior`
  - `architecture intended behavior`
  - `gem5 current behavior`
- 形成 RTL issue 报告；
- 在用户决定前，`rtl_aligned` 配置默认忠实复现当前 RTL；
- 可以另建 `spec_intended` 或 `ideal` 配置，但不得混在一起。

======================================================================
四、必须先明确“严格对齐”的层次
======================================================================

将对齐划分为四层，所有报告和测试都必须标注属于哪一层。

L0：功能对齐

比较提交后的架构状态：

- integer registers；
- floating-point registers；
- vector registers；
- memory；
- PC；
- exception/cause；
- `vl`；
- `vtype`；
- `vstart`；
- `vxrm`；
- `vxsat`；
- mask/tail policy；
- 自定义 Venus CSR 和状态。

功能状态原则上必须完全一致，不接受百分比误差。

L1：事务/事件对齐

比较：

- vector instruction dispatch；
- instruction accept/reject；
- micro-op 生成；
- dependency allocation；
- lane dispatch；
- VRF read/write；
- VFU issue/complete；
- forwarding/chaining；
- LSU request/response；
- memory split/merge；
- replay；
- exception；
- vector instruction completion；
- architectural retirement。

事件顺序必须一致。只有明确声明为“不可观察或被抽象”的内部事件才允许不同。

L2：周期级微架构对齐

对于独立的定向微基准：

- 事件发生周期应一致；
- ready/valid 和 backpressure 行为应一致；
- 队列占用变化应一致；
- resource stall 原因应一致；
- first/last element timing 应一致；
- completion/retirement 周期应一致。

如果 RTL 和 gem5 对时钟边沿的记录 convention 不同，可以允许固定的 0/1-cycle observation offset，但必须：

- 在 schema 中明确定义；
- 对所有测试一致；
- 不能针对单个失败测试临时放宽。

L3：应用级性能对齐

使用未参与校准的 held-out kernel 或应用比较：

- total cycles；
- IPC；
- cycles per element；
- 各类 stall cycles；
- VFU utilization；
- VRF conflict；
- memory traffic；
- outstanding 分布；
- issue/complete rate；
- lane utilization；
- cache/bus statistics；
- 各性能优化的相对收益和趋势。

不要只规定一个总周期误差阈值。阈值应在获得 baseline 后由数据确定。

建议初始目标：

- 功能状态：完全一致；
- 独立 Venus 微基准：事务和周期严格一致；
- 组合微基准：关键事件严格一致，总周期尽量 0 偏差；
- kernel：总周期误差先收敛到 3% 内，再向 1% 推进；
- 关键统计项：误差 5% 内；
- 趋势、拐点和优化相对收益：必须一致。

这些是初始工程目标，不是可以掩盖系统性错误的豁免条款。

======================================================================
五、最重要的工程原则
======================================================================

1. 先对齐结构和状态机，再自动调参数。

贝叶斯优化、搜索、机器学习或 LLM 只能调整模型中已经存在的自由度。

如果 gem5 缺失了：

- backpressure；
- arbitration；
- replay；
- port conflict；
- forwarding condition；
- queue reservation；
- early-full policy；
- 同周期更新顺序；

那么再强的参数优化也只能过拟合。

2. 先隔离 Venus，再接入真实 CPU 和 cache。

首先实现类似 Ara Ideal Dispatcher 的 standalone 模式：

    vector instruction trace / command stream
                    ↓
             ideal dispatcher
                    ↓
           Venus RTL 或 gem5
                    ↓
       deterministic memory shim

该模式中：

- 不让真实标量 CPU 限制 vector issue；
- 不让真实 cache 干扰向量核；
- scalar operand 由 trace 或测试框架提供；
- memory latency、bandwidth、response ordering 可配置且确定；
- 相同输入可同时提供给 RTL 与 gem5。

只有 standalone Venus 对齐后，才进行：

- scalar CPU 接入；
- cache；
- coherence；
- interconnect；
- full-system workload。

3. 不要把 Venus 简化为 gem5 O3CPU 的几个 FunctionalUnit。

如果 Venus 是解耦向量协处理器，应优先实现专用的 Venus SimObject/事件驱动模型，真实表示：

- 指令窗口；
- lane；
- VRF；
- VFU；
- LSU；
- mask/slide/reduction；
- completion 和响应。

4. “参数相同”不代表行为相同。

典型案例：

- RTL queue size 为 72；
- gem5 queue size 也为 72；
- RTL 每周期最多入队 6 项；
- 当 free entries 少于 6 时，RTL 提前拉高 full；
- gem5 仍允许一直填到 72；
- 两者配置完全相同，但容量边缘性能不同。

因此必须建模：

- threshold；
- admission policy；
- reservation；
- enqueue/dequeue 同周期优先级；
- combinational ready；
- state update order。

5. 用 first divergence 定位问题，不要根据最终 IPC 猜。

每次差异分析必须优先报告：

- 第一个分歧周期；
- 第一个分歧事件；
- 分歧前 N 个周期上下文；
- 分歧后 N 个周期上下文；
- RTL 相关信号；
- gem5 相关状态；
- 最小可证伪假设。

6. 校准集和验证集严格分开。

- microbenchmarks 用于隔离和校准；
- kernels 用于组合测试；
- 大型应用只用于最终验证；
- 不允许用同一批应用反复调参后，再把它们当成验证结果。

======================================================================
六、第一阶段：只读审计
======================================================================

在修改任何文件前，完成以下工作。

1. 阅读项目约束

查找并完整阅读：

- `AGENTS.md`
- `CLAUDE.md`
- `CODEX.md`
- `CONTRIBUTING.md`
- `README.md`
- `SKILL.md`
- CI 配置
- 构建文档
- 测试文档

2. 检查版本状态

记录：

- RTL git root；
- gem5 git root；
- 当前 branch；
- commit hash；
- submodule hash；
- `git status --short`；
- 未提交修改；
- 生成配置；
- 编译器版本；
- Verilator/VCS 版本；
- gem5 基线版本；
- RISC-V toolchain 版本。

已有未提交修改属于用户，不得覆盖、reset、checkout 或删除。

3. 识别最小运行路径

找到：

- 最小 RTL build；
- 最小 RTL run；
- 最小 gem5 build；
- 最小 gem5 run；
- 当前已知通过测试；
- 当前已知失败测试；
- trace/waveform 开关；
- stats 输出；
- timeout/死锁检测；
- test seed。

4. 输出第一份审计报告

报告至少包含：

- 仓库结构；
- 当前可运行状态；
- RTL 顶层；
- gem5 Venus 入口；
- 当前模型抽象层次；
- 现有测试；
- 现有 trace；
- 最大的对齐风险；
- 推荐的第一个被校准特性。

在没有完成只读审计前，不得开始大规模重构。

======================================================================
七、建立单一来源的 Venus 微架构规格
======================================================================

创建机器可读的规格文件，但放置位置要遵守仓库现有规范。建议内容类似：

`venus_spec.yaml`

至少包含：

architecture:
  isa:
  custom_extensions:
  xlen:
  vlen:
  elen:
  supported_sew:
  supported_lmul:
  tail_policy:
  mask_policy:

dispatcher:
  command_width:
  queue_depth:
  max_inflight_instructions:
  scalar_operand_ports:
  response_policy:
  admission_rule:

vrf:
  register_count:
  bytes_per_register:
  lane_count:
  banks_per_lane:
  bank_mapping:
  read_ports:
  write_ports:
  port_type:
  arbitration:
  read_latency:
  write_latency:
  same_cycle_read_write_behavior:

vfu:
  alu:
    count_per_lane:
    latency:
    initiation_interval:
    supported_ops:
  mul:
  div:
  fpu:
  conversion:
  compare:
  fixed_point:
  custom_units:

chaining:
  supported:
  producer_consumer_pairs:
  granularity:
  first_element_latency:
  forwarding_sources:
  hazards:
  blocked_conditions:

slide:
  implementation:
  supported_native_offsets:
  decomposition_rule:
  reshuffle_shared:
  arbitration:
  latency:

mask:
  storage_layout:
  cross_lane_access:
  latency:
  arbitration:
  masked_element_skip:

reduction:
  intra_lane:
  inter_lane:
  simd_phase:
  fpu_pipeline_feedback:
  latency_rule:

lsu:
  load_queue_depth:
  store_queue_depth:
  request_width:
  response_width:
  max_outstanding:
  bank_count:
  bank_mapping:
  beat_bytes:
  alignment:
  unit_stride:
  strided:
  indexed:
  split_merge:
  ordering:
  replay:
  fault_handling:
  backpressure:
  early_full_rule:

memory_interface:
  protocol:
  request_channels:
  response_channels:
  id_width:
  ordering_rule:
  bandwidth:
  minimum_latency:
  maximum_latency:

completion:
  done_condition:
  commit_condition:
  exception_condition:
  flush_rule:

每一个字段都必须附带来源：

- RTL 文件和行号；
- elaboration/config 参数；
-规范文档；
- 仿真实验；
- 推测。

禁止把推测写成已经确认的事实。

======================================================================
八、RTL 与 gem5 映射
======================================================================

创建 `rtl_gem5_map`，至少包含以下列：

| Feature | RTL module/state/signal | gem5 file/class/state | Status | Evidence |
|---------|-------------------------|-----------------------|--------|----------|
| command accept | ... | ... | aligned/missing/suspect | ... |
| instruction queue | ... | ... | ... | ... |
| dependency tracking | ... | ... | ... | ... |
| VRF bank mapping | ... | ... | ... | ... |
| VFU ALU pipeline | ... | ... | ... | ... |
| chaining | ... | ... | ... | ... |
| load outstanding | ... | ... | ... | ... |
| replay | ... | ... | ... | ... |
| mask | ... | ... | ... | ... |
| slide | ... | ... | ... | ... |
| reduction | ... | ... | ... | ... |
| completion | ... | ... | ... | ... |

状态至少区分：

- confirmed-aligned
- parameter-mismatch
- behavior-mismatch
- missing-in-gem5
- RTL-spec-conflict
- insufficient-observability
- untested

======================================================================
九、统一 Trace Schema
======================================================================

RTL 与 gem5 应输出尽量相同的 JSONL、CSV 或紧凑二进制事件流。

建议公共字段：

run_id
rtl_commit
gem5_commit
config_hash
test_name
seed

cycle
sequence_id
pc
instruction_bits
instruction_name

vl
sew
lmul
vtype
vstart
mask_enabled
tail_policy

vector_instruction_id
uop_id
element_index
lane
stage
resource
resource_slot
bank

valid
ready
accepted
issued
completed
retired

src_vreg_0
src_vreg_1
src_vreg_2
dst_vreg
src_value_hash
dst_value_hash
byte_enable

memory_op
virtual_address
physical_address
size
stride
request_id
response_id
beat_index
last_beat
replay_reason

queue_name
queue_occupancy
queue_capacity
stall_reason

exception
cause

不要一开始把每个内部信号全部输出。优先实现以下边界事件：

1. command accept；
2. instruction/uop issue；
3. VRF read/write；
4. VFU start/complete；
5. LSU request/response；
6. instruction done；
7. architectural commit。

内部信号只在定位首个分歧时按需增加。

标准化比较器必须支持：

- 忽略绝对路径、时间戳等无关字段；
- 固定 cycle offset；
- 事件字段白名单；
- 按 sequence/uop/request ID 对齐；
- 找到 first divergence；
- 展示前后上下文；
- 对缺失、额外、乱序、值不一致、周期不一致分别分类；
- 生成机器可读报告。

======================================================================
十、功能参考模型
======================================================================

如果 Venus 使用标准 RVV：

- 可以使用 Spike 或其他经过验证的 RVV functional model 检查标准指令语义；
- RTL 和 gem5 都应与 functional oracle 比较；
- 不要只让 RTL 与 gem5 互相比，因为两边可能出现相同错误。

如果 Venus 有自定义指令：

- 寻找已有 ISS、C model、Scala/Chisel reference、Python model 或测试 reference；
- 如果没有，应建立最小的、无时序的 Venus functional reference；
- functional reference 只负责架构结果，不负责周期；
- 不要把复杂 timing model 同时当作唯一 functional oracle。

建议形成三方验证：

    functional reference
          /       \
       RTL       gem5

只有 RTL 与 gem5 同时通过 functional reference 后，再进行 timing calibration。

======================================================================
十一、Venus gem5 应显式建模的内容
======================================================================

逐项检查，缺失项不能用一个总 latency 掩盖。

A. Dispatch/Issue

- command queue；
- issue width；
- instruction window；
- maximum in-flight；
- scalar operand forwarding；
- instruction admission；
- dependency allocation；
- flush；
- busy response；
- completion response。

B. Vector register state

- architectural register contents；
- register busy state；
- per-register element-width metadata；
- valid/dirty state；
- partial write；
- mask/tail state；
- widening/narrowing 占用；
- register group overlap；
- source/destination alias。

C. VRF

- lane mapping；
- bank mapping；
- number of banks；
- port types；
- read/write ports；
- same-bank conflict；
- crossbar；
- arbitration priority；
- read latency；
- write latency；
- same-cycle RAW/WAR/WAW；
- multiple source startup conflict。

D. VFU

每种 operation class 分别建模：

- latency；
- initiation interval；
- pipeline occupancy；
- unit count；
- per-lane/shared；
- pipelined/non-pipelined；
- data-dependent latency；
- widening/narrowing；
- signed/unsigned；
- FP exception；
- fixed-point rounding/saturation。

E. Chaining

- 哪些 producer/consumer 支持 chaining；
- element、beat、lane 还是整个 instruction 粒度；
- first result 可用周期；
- forwarding mux；
- forwarding queue；
- source 优先级；
- chaining 与 VRF writeback 的关系；
- chaining 被 mask/slide/LSU 阻塞的条件。

F. Mask

- mask bit 在 lane 间的布局；
- mask read bandwidth；
- mask cross-lane；
- masked-off element 是否跳过执行；
- masked element 是否占用 pipeline；
- mask unit 与 slide/VRF 的共享资源；
- `v0` hazard。

G. Reshuffle/Deshuffle

Ara2 表明 RVV 1.0 的 mixed-width 和 tail-undisturbed 可能要求：

- source register EW 改变时重排；
- destination 未完全覆盖时先 deshuffle 再 reshuffle；
- 全寄存器写可能跳过；
- reshuffle 可能占用 slide unit；
- 与 slide/reduction 形成结构冲突。

Venus 是否采用同样策略必须从 RTL 确认。

H. Slide/Permutation

- 原生支持的 offset；
- 非原生 slide 是否拆成多个 micro-op；
- lane 间网络；
- crossbar；
- 与 reshuffle 的共享；
- 与 reduction 的共享；
- arbitration；
- 每个阶段 latency。

I. Reduction

检查是否包含：

- intra-lane；
- inter-lane；
- SIMD/subword；
- tree 或 iterative；
- FPU pipeline accumulator；
- inter-lane feedback；
- lane 数影响；
- SEW 影响；
- mask/tail 影响；
- ordered/unordered reduction 区别。

J. LSU

- address generation；
- unit-stride；
- strided；
- indexed；
- segment；
- alignment；
- misalignment；
- element-to-beat mapping；
- request split；
- response merge；
- memory bank mapping；
- outstanding；
- MSHR/tag；
- load/store ordering；
- completion rule；
- replay；
- fault-only-first；
- partial fault；
- backpressure；
- response reordering；
- store buffer；
- early-full。

K. Completion

必须区分：

- 最后一个 micro-op issued；
- 最后一个 element executed；
- 最后一次 VRF write；
- 最后一个 memory response；
- vector instruction done；
- response sent；
- architectural retire。

======================================================================
十二、微架构 Cliffs 测试方法
======================================================================

不要只写一个“测 latency”的 benchmark。每个特性应形成参数扫描曲线，观察：

- 线性区；
- 饱和平台；
- 突然跳变；
- 周期性波动；
- bank conflict pattern；
- 容量边缘；
- triggered/non-triggered 差异。

四类基本测试：

1. Latency

构造严格 RAW dependency chain，禁止其他独立指令覆盖 latency。

2. Bandwidth/Throughput

构造相互独立的操作流，逐渐增加并行度，观察吞吐何时饱和。

3. Capacity

使用长延迟操作阻塞目标资源释放，再用短操作逐渐填充，寻找延迟突然增加的拐点。

4. Trigger comparison

构造只相差一个触发条件的两组测试，例如：

- same bank vs different bank；
- chaining allowed vs blocked；
- aligned vs misaligned；
- mask all-on vs sparse；
- same EW vs changed EW；
- native slide offset vs decomposed offset。

======================================================================
十三、Venus 优先微基准矩阵
======================================================================

第一优先级：

1. 单条向量 ALU 指令的 first-element 和 last-element timing；
2. ALU RAW dependency chain；
3. 独立 ALU 流，测 initiation interval；
4. 不同 lane 数、VL、SEW、LMUL；
5. VRF source register 编号扫描；
6. VRF destination 编号扫描；
7. 同 bank 与不同 bank；
8. producer→consumer 间隔扫描，识别 chaining；
9. load outstanding 数扫描；
10. store outstanding 数扫描；
11. memory response latency 扫描；
12. command queue/instruction window 容量；
13. issue width；
14. completion 条件。

第二优先级：

1. mask density：0%、稀疏、50%、100%；
2. tail agnostic/undisturbed；
3. EW 改变；
4. widening/narrowing；
5. slide offset 扫描；
6. reduction 的 lane/SEW/VL 扫描；
7. unit-stride；
8. strided；
9. indexed；
10. aligned/misaligned；
11. bank conflicts；
12. load→compute→store chaining；
13. 多条 vector instruction 并发；
14. shared-unit arbitration。

第三优先级：

1. scalar core issue 限制；
2. scalar operand forwarding；
3. CPU 与 Venus memory contention；
4. cache refill；
5. cache invalidation/coherence；
6. full-system kernel；
7. OS 和异常；
8. checkpoint/SimPoint。

每个测试必须明确：

- target feature；
- controlled variables；
- sweep variable；
- expected qualitative behavior；
- possible contaminating bottlenecks；
- 如何排除污染；
- RTL/gem5 采集事件；
- pass/fail rule。

======================================================================
十四、AI 自动对齐循环
======================================================================

每一轮只处理一个明确的 feature。

Step 1：选择特性

从映射表中选择：

- 高风险；
- 可隔离；
- 可观察；
- 对其他特性依赖较少；

的特性。

Step 2：阅读证据

必须阅读：

- RTL 相关模块；
- 参数定义；
- 生成配置；
- ready/valid 路径；
- gem5 对应代码；
- 已有测试和 trace。

Step 3：提出可证伪假设

示例：

“gem5 在 VRF bank 仲裁中允许同周期两个 read，而 RTL 每个 bank 只有一个共享 read port，因此两源寄存器映射到同 bank 时 gem5 少产生一个 startup stall。”

禁止使用：

“可能 gem5 latency 不对，先调一调看看。”

Step 4：生成最小实验

只改变一个自变量，并覆盖边界附近。

如果怀疑 queue 深度是 32，不要只测 16 和 64，应密集测试：

24, 28, 30, 31, 32, 33, 34, 36, 40

Step 5：运行 RTL 和 gem5

保存：

- 命令；
- commit；
- config；
- seed；
- stdout/stderr；
- trace；
- stats；
- waveform 路径；
- timeout。

Step 6：比较首个分歧

将差异分类为：

- functional mismatch；
- parameter mismatch；
- missing state；
- missing resource contention；
- handshake mismatch；
- same-cycle update-order mismatch；
- event observer mismatch；
- benchmark contamination；
- nondeterminism；
- RTL/spec conflict；
- harness/toolchain issue。

Step 7：最小修改

一次只修改一个机制。

禁止：

- 同时调整多个无关 latency；
- 在不了解原因时增加 magic delay；
- 修改测试预期以通过；
- 删除失败测试；
- 降低精度；
- 修改 golden RTL。

Step 8：验证

至少运行：

1. 当前目标测试；
2. 同资源邻近测试；
3. 之前已经对齐的相关测试；
4. 快速全量回归；
5. held-out 测试。

Step 9：记录结论

报告：

- 原始假设；
- 实验证据；
- first divergence；
- 根因；
- 修改；
- 为什么修改能解释行为；
- 回归结果；
- 残留风险；
- 下一步。

连续三个假设不能解释同一 first divergence 时，停止盲目修改并报告人工决策点。

======================================================================
十五、微基准代码生成器
======================================================================

参考 AC Loop 的经验，不让 AI 每次自由生成几千行 C/汇编。

建立一个确定性的 microbenchmark generator。AI只生成或修改：

- instruction snippet；
- dependency relation；
- register allocation constraints；
- sweep；
- warmup；
- repeat；
- expected behavior。

建议输入：

feature: vrf_bank_conflict

description:
  Compare accesses mapped to the same and different VRF banks.

fixed:
  operation: vadd.vv
  vl: 256
  sew: 32
  lmul: 1
  mask: disabled
  memory_latency: fixed

sweep:
  src0_register: [...]
  src1_register: [...]
  dst_register: [...]

warmup_iterations: 20
measure_iterations: 100

observations:
  - total_cycles
  - first_issue_cycle
  - last_complete_cycle
  - vrf_read_stall_cycles
  - bank_conflict_events

expected:
  type: pairwise_comparison
  same_bank_slower_than_different_bank: true

生成器负责：

- 初始化；
- cache/memory warmup；
- CSR 配置；
- 防止编译器删除；
- 读取 cycle/instret；
- 重复；
- 输出；
- 编译；
- 链接；
- binary manifest；
- seed；
- 结果格式。

======================================================================
十六、CPU 与全系统范围
======================================================================

由于旧 gem5 CPU 模型不精确，必须明确区分两个目标。

目标 A：严格对齐 Venus 向量核

使用：

- ideal dispatcher；
- trace-driven scalar operand；
- deterministic memory；
- 不依赖真实 CPU pipeline。

这一目标优先完成。

目标 B：对齐 Venus 所在完整系统

完成 A 后，再单独校准：

- scalar pipeline；
- fetch/decode/issue；
- branch predictor；
- scalar cache；
- MMU/TLB；
- coherence；
- interconnect；
- memory controller。

如果要求标量 CPU 逐周期严格等同 RTL，通用 gem5 O3CPU 或 MinorCPU 可能不够，需要：

- Venus 专用 scalar CPU model；
- 或 trace-driven CPU；
- 或 gem5+Verilator/SystemC co-simulation；
- 或接受统计相关而非内部逐周期等价。

不得让不精确 CPU 阻塞 Venus standalone 模型的开发。

======================================================================
十七、Aligned 与 Ideal 配置必须分开
======================================================================

参考 XS-GEM5，至少维护：

1. `venus_rtl_aligned`

只包含指定 RTL commit 已实现的：

- 参数；
-结构；
- 行为；
- 限制；
- bug-compatible behavior（如果项目决定保留）。

2. `venus_ideal` 或 `venus_explore`

可以包含：

- 更大的 queue；
- 更高 issue width；
- 更理想 chaining；
- 更高 bandwidth；
- 尚未实现的 RTL 优化；
- 设计空间探索参数。

每次输出必须打印：

- 使用哪个配置；
- 对应 RTL commit；
- 配置 hash；
- 是否包含非 RTL 特性。

不能为了提高性能把 ideal 特性混入 aligned 配置。

======================================================================
十八、RTL 问题处理流程
======================================================================

Venus RTL 本身可能存在架构问题。发现疑似 RTL 问题时，不要立即修改。

建立 issue 记录：

Title:
Affected RTL commit:
Affected configuration:
Minimal reproducer:
Expected behavior from spec:
Observed RTL behavior:
Observed gem5 behavior:
First divergent cycle:
Relevant RTL signals:
Relevant RTL source:
Architectural consequence:
Performance consequence:
Suggested options:
  A. Keep RTL behavior and align gem5
  B. Fix RTL and update gem5
  C. Clarify specification
Regression test to add:

对于标准 RVV 行为问题，优先与正式 ISA 规范或可靠 functional model 交叉验证。

对于 Venus 自定义行为，要求找到规范、设计者意图或最小可复现实验。

======================================================================
十九、验收不能只看总周期
======================================================================

下面情况均不得宣布“已对齐”：

1. 总周期相同，但 stall 分类不同；
2. 一个模型少了 bank conflict，却被更大的其他 latency 抵消；
3. 一个模型提前 issue、延迟 complete，另一个相反；
4. IPC 很接近，但优化开关的性能收益方向不同；
5. 校准 benchmark 很准，held-out benchmark 很差；
6. 通过人为调大固定 latency 掩盖资源竞争；
7. queue depth 相同，但边界行为不同；
8. 平均值一致，但不同 VL/SEW/LMUL 曲线不同。

必须同时检查：

- absolute results；
- curve shape；
- saturation；
- change point；
- periodic conflict pattern；
- relative optimization benefit；
- counter/stall breakdown；
- event timing。

======================================================================
二十、自动参数搜索的使用条件
======================================================================

只有满足以下条件后，才允许使用 Gem5Tune、贝叶斯优化、TPE、网格搜索、遗传算法或可微代理：

1. 目标组件的状态机已经存在；
2. 参数具有清晰物理意义；
3. 已建立校准集和 held-out 集；
4. 目标函数不仅有 total cycles，还包括：
   - event timing；
   - counters；
   - cliff change point；
   - curve shape；
5. 搜索范围由 RTL 或物理设计约束；
6. 搜索结果能映射回明确的微架构解释。

建议目标函数：

loss =
    w_functional * functional_failure
  + w_event * event_mismatch
  + w_cycle * normalized_cycle_error
  + w_counter * counter_error
  + w_cliff * change_point_error
  + w_shape * curve_shape_error

功能失败应使用极大惩罚，不允许用性能拟合补偿功能错误。

======================================================================
二十一、参考项目及已经提炼出的经验
======================================================================

以下不是让用户去读的书单。你已经可以直接使用下面结论；需要实现细节时再自行阅读源码和论文。

1. Ara/Ara2

Repository:
https://github.com/pulp-platform/ara

Paper:
https://arxiv.org/html/2311.07493v2

关键经验：

- 使用 Ideal Dispatcher 隔离 scalar core 和 cache；
- 性能与 Byte/Lane 强相关；
- short vector 的启动开销不能忽略；
- mixed-width 可能引入 reshuffle；
- tail-undisturbed 可能要求保护旧内容；
- slide/mask/reduction 可能共享跨 lane 网络；
- reduction latency 与 lane 数、SEW、FPU pipeline 有关；
- scalar issue rate 和 scalar cache 会限制短向量性能；
- aligned 模型必须覆盖这些结构性影响。

2. T1

Repository:
https://github.com/chipsalliance/t1

关键经验：

- lane-based；
- banked SRAM VRF；
- configurable VRF ports；
- intensive chaining；
- large LSU outstanding；
- load/store 可 instruction-level OoO；
- 使用 Verilator/VCS emulator；
- 读取 RTL event 后执行 check；
- 支持导出 RTL properties；
- 测试覆盖 asm、codegen、intrinsic、MLIR、perf、PyTorch、rvv-bench；
- 对 Venus 最有价值的是事件级 difftest 和机器可读配置导出。

3. XS-GEM5/XiangShan

Repository:
https://github.com/OpenXiangShan/GEM5

关键经验：

- 维护 RTL-aligned 与 ideal/performance 配置；
- online difftest；
- Topdown counters；
- frontend/backend/cache/RVV 分组件校准；
- 使用 NEMU/checkpoint/SimPoint 加速大程序；
- 不把宏观性能分数当作唯一正确性证明。

4. Microarchitecture Cliffs

Paper:
https://arxiv.org/html/2602.11580v1

关键经验：

- latency：RAW chain；
- bandwidth：independent streams；
- capacity：长延迟操作堵塞资源，再逐步填充；
- special behavior：triggered/non-triggered pair；
- 用曲线拐点识别容量和行为；
- 一个普通 microbenchmark 可能同时被多个资源限制；
- 必须逐步消除非目标瓶颈；
- 论文中 XS-GEM5 在 Cliff tests 上从 59.2% 误差降到 1.4%；
- 这证明“单特性归因”比直接根据应用 IPC 调参更有效。

5. AC Loop

Paper:
https://openreview.net/pdf?id=2Qh0L90vmy

关键经验：

- AI 接收人工写的 feature 描述和 calibration task；
- AI 调用固定的 Code Generator；
- 自动编译、运行、采集、拟合和反馈；
- 每个阶段有 checkpoint；
- retry 有上限；
- 失败后停止并交给人工；
- AI只需要生成关键 snippet，而不是整个 benchmark；
- 能检测参数相同但 admission/backpressure 行为不同的问题；
- 这是 Venus AI 自动化的最近似参考，但不是通用 RTL→gem5 自动转换器。

6. RISC-V RTL 与 gem5 建模研究

Paper:
https://arxiv.org/abs/2106.09991

关键经验：

- 可见参数全部匹配后，抽象误差仍可能很大；
- memory 和 branch predictor 是常见误差源；
- 有时需要直接把 RTL 算法翻译成 C++，而不是选择一个“近似”的现成模型；
- 对 Venus 同理：关键仲裁和状态机可能需要直接实现。

7. Component-level Calibration

Paper:
https://infoscience.epfl.ch/bitstreams/2906e93c-624f-40f8-8a1d-ba84664c8e33/download

关键经验：

- 使用组件定向 stressors 校准；
- 先处理最大的 error source；
- 每次调一个组件后重新 profile；
- macro benchmark 不适合用作主要调参集；
- 训练集表现好不等于完整应用准确。

8. gem5+rtl

Paper:
https://upcommons.upc.edu/bitstream/handle/2117/354158/icpp21.pdf

gem5 SystemC:
https://www.gem5.org/about/

关键经验：

- 可以通过 Verilator/GHDL/SystemC/TLM 将 RTL 放进 gem5 full system；
- 适合构建高保真 oracle 和短窗口验证；
- 不一定适合作为最终快速性能模型；
- Venus 可以保留纯 gem5 快速模型，同时使用 RTL co-sim 做关键验证。

9. Gem5Tune

DOI:
https://doi.org/10.1109/TC.2023.3347675

关键经验：

- 使用 TPE/Bayesian optimization；
- instruction calibration；
- sensitivity analysis；
- dynamic pruning；
- 适合调已有参数；
- 不能发现或补全模型中不存在的机制。

10. DiffTune

Paper:
https://arxiv.org/abs/2010.04017

关键经验：

- 用可微 surrogate 调不可微模拟器；
- 适合粗粒度 end-to-end 参数学习；
- 不应在 Venus 结构未正确建模时使用。

======================================================================
二十二、Skill 策略
======================================================================

不要浪费时间寻找一个不存在的通用“RTL 自动转 gem5”skill。

如果当前 AI 环境支持自定义 skill，在完成仓库审计和第一版工作流后，创建项目专用：

venus-gem5-alignment

建议结构：

venus-gem5-alignment/
├── SKILL.md
├── references/
│   ├── architecture-map.md
│   ├── venus-spec.md
│   ├── trace-schema.md
│   ├── alignment-levels.md
│   ├── rtl-gem5-map.md
│   └── feature-cards/
├── scripts/
│   ├── run_rtl
│   ├── run_gem5
│   ├── normalize_trace
│   ├── compare_trace
│   ├── first_divergence
│   ├── fit_cliff
│   ├── run_regression
│   └── create_manifest
└── assets/
    ├── microbench-templates/
    ├── report-template.md
    └── issue-template.md

这个 skill 的作用是固化：

- 正确命令；
- 构建环境；
- trace schema；
- 测试生成规则；
- 证据标准；
- 修改权限；
- 验收规则；
- 已确认的 Venus 知识。

不要把尚未确认的推测写入 skill 的事实部分。

======================================================================
二十三、代码修改规则
======================================================================

1. 先读后改。
2. 保留用户未提交修改。
3. 不使用 destructive git 命令。
4. 不进行无关格式化。
5. 修改范围尽可能小。
6. 每一个 magic number 都必须有：
   - 命名；
   - 来源；
   - 单位；
   - 对应 RTL 参数或实验。
7. 参数只有一个 source of truth。
8. RTL-aligned 配置不能混入 ideal 参数。
9. 新机制必须有定向测试。
10. bug fix 必须有 regression test。
11. 每个提交只处理一个逻辑主题。
12. 不要为了让 CI 变绿而删除、skip 或放宽测试。

======================================================================
二十四、每轮向用户汇报的格式
======================================================================

工作过程中用短更新说明：

- 正在检查什么；
- 已确认什么；
- 下一步运行什么；
- 是否发现阻塞。

每一轮最终报告使用：

本轮目标：
证据：
首个分歧：
根因：
修改：
验证：
剩余风险：
下一步：

如果没有修改，也要说明：

- 排除了哪些假设；
- 新增了哪些证据；
- 为什么暂时不能修改。

======================================================================
二十五、第一阶段交付物
======================================================================

在开始大规模模型实现前，必须交付：

1. `environment_manifest`
   - 所有版本、commit、工具链和命令。

2. `venus_architecture_inventory`
   - Venus 结构、配置和数据流。

3. `rtl_gem5_mapping`
   - RTL 与旧 gem5 对应关系。

4. `gap_analysis`
   - 已对齐；
   - 参数错误；
   - 行为错误；
   - gem5 缺失；
   - RTL 可疑；
   - 不可观察。

5. `trace_schema`
   - RTL/gem5 公共事件规范。

6. `minimal_reproducer`
   - 一个 RTL 和 gem5 都能运行的最小程序。

7. `baseline_report`
   - 功能差异；
   - 周期差异；
   - first divergence；
   - 初步根因。

8. `implementation_plan`
   - 按依赖关系排序；
   - 不按“哪个文件好改”排序。

9. 第一项已闭环 feature
   - 建议选择单 VFU latency、command admission 或 VRF bank mapping 中最容易隔离的一项。

======================================================================
二十六、完整 Definition of Done
======================================================================

只有满足以下条件，才可认为 Venus gem5 与目标 RTL 对齐完成：

1. 目标 RTL commit 和配置被固定。
2. Venus ISA 功能测试全部通过。
3. 自定义指令具有独立 functional oracle 或明确规范。
4. standalone Venus 模式存在。
5. RTL 与 gem5 使用统一事件流。
6. 能自动定位 first divergence。
7. 每个关键结构都有定向 Cliff tests。
8. 关键容量、延迟、带宽和触发条件已对齐。
9. VRF bank/port 冲突已对齐。
10. VFU latency/II/pipeline occupancy 已对齐。
11. chaining 已对齐。
12. mask/tail/mixed-width 已对齐。
13. slide/reduction 已对齐。
14. LSU split/outstanding/bank/replay/backpressure 已对齐。
15. completion/retirement 已对齐。
16. aligned 与 ideal 配置分离。
17. 校准集与 held-out 集分离。
18. held-out kernel 的性能、趋势和 counters 达到约定目标。
19. 所有修复具有回归测试。
20. 所有实验可通过 manifest 复现。
21. 疑似 RTL 问题均有独立 issue 报告。
22. AI 自动循环具有 retry limit 和人工接管机制。
23. 不存在仅靠 magic latency 抵消另一处错误的假对齐。
24. 文档能让另一位工程师在新机器上复现实验。

======================================================================
二十七、你现在立即执行的任务
======================================================================

现在开始，不要先给我一篇泛泛的方案。

按以下顺序执行：

1. 确认当前工作目录。
2. 查找 Venus RTL、旧 gem5、测试和工具链。
3. 阅读所有仓库级指令和构建文档。
4. 记录 git 状态和版本。
5. 找到最小 RTL 与 gem5 运行方法。
6. 定位 Venus RTL 顶层、dispatcher、lane、VRF、VFU、LSU、mask、slide、reduction。
7. 定位旧 gem5 Venus 的 SimObject、C++ 类、Python 配置、ISA 实现和统计项。
8. 输出第一版 RTL↔gem5 映射和缺口清单。
9. 选择最适合首先闭环的一个微架构特性。
10. 建立或提出最小 trace/event 对比方案。
11. 在证据足够后开始第一个最小修复。
12. 运行定向测试和回归。

你的第一次回复应简洁说明：

- 找到了哪些仓库；
- 当前版本和工作树状态；
- 构建/运行入口；
- 发现的最明显风险；
- 正在选择的第一个对齐目标。

随后继续实际调查，不要停在计划阶段。

======================================================================
二十八、最终提醒
======================================================================

本项目的成功标准不是：

“AI 修改了一些 gem5 参数，几个 benchmark 误差变小。”

真正的成功标准是：

“对于指定 Venus RTL commit，我们能通过可复现、可自动运行、可定位首个分歧的验证闭环，证明 gem5 在功能、关键事件、周期行为、资源瓶颈和应用趋势上忠实代表 RTL；同时能够在 Venus RTL 继续迭代时自动发现失配并指导最小修复。”

请现在开始执行。

======================================================================
二十九、2026-08-10 scalar600 CPU R102 断点
======================================================================

当前不能宣称 CPU 完全对齐，但通用标量流水缺口已与 Venus 接口缺口分离：

- 有效 CPU 微用例矩阵 24/24 逐退休 exact；修正版 LSU case exact。
- DIV-to-ALU、load-to-MUL、branch-to-store 分别 21/22/21 条 exact。
- LSU RAW/throughput/capacity 68/68 通过。
- CCH 10,253 条、SCH 8,103 条 VINS 与接受基线逐 task ordinal exact。
- 修复了级联 scalar SPM XBar 的 response retry：无冲突保持同边沿，只有
  下游真实拒绝的 response 才进入 waiting-on-retry 队列。CCH 不再在
  tick 33,481,001 触发空响应队列断言。

CCH task17 仍为 `+20.483 us / +61.757%`。修正 squash-aware scalar oracle
后，3,140 条公共标量退休的 PC/class 顺序一致；首个时序差在 index 351、
PC `0x59c` 的 VSEQ（RTL relative cycle 1111，gem5 1113）。最终 10,246
tile-cycle 漂移中，Venus-to-Venus admission/completion gap 贡献 +9,561；
普通 ALU/LSU/MUL/control gap exact。因此下一步应对齐 Sequencer tagged
admission、VFU/VRF backpressure 与 completion，而不是继续修改 CPU 固定
latency，也不能用 task20 的负误差抵消。

最终 task timing：

- CCH：`/tmp/a32_r102_cpu_complete/cch/task_execution_timing_vs_rtl.json`
- SCH：`/tmp/a32_r102_cpu_complete/sch/task_execution_timing_vs_rtl.json`
- CPU 详报：`docs/venus_alignment/A32_R81_SCALAR600_CPU_CONFORMANCE_MATRIX.md`

# 2026-08-10: scalar600/Sequencer R125 微时序断点

CPU 核心定向矩阵仍是有效 24/24 逐退休 exact；本轮没有调整 CPU 固定
latency。新 RTL UCLI 波形证明 task17 目标段 lane0--3 的 input FIFO、
operand command、requester ready/issued 逐拍同步，lane3 的 7-row tail 不会
比其他 lane 的 8-row tail 提前释放。gem5 原来的 local-row 服务时间制造了
RTL 中不存在的 lane 去同步。

本轮保留两项通用结构修复：空闲 CAU requester 使用全 lane 最大 row 服务
窗口；requester 接收后的 command retirement 在下一 tile edge 可见。结果：

- task17 sequence0--24 fire/duration/recycle exact；首个 fire/duration 分歧
  推进到 sequence25 VMUL，为 +4/-4 cycles；首个 recycle 分歧是 sequence35
  +1 cycle；
- CCH task17 仍 +20,449 ns；task20 为 -16,173 ns，二者不得抵消；
- CCH/SCH VINS 10,253/8,103 task-local byte-exact；
- LSU 68/68；标量 runner 25/25（有效矩阵 24/24）；
- SCH task timing 与 R120 完全不变；
- 二进制 SHA256：
  `7c08c9327939df4c50d19f9e4009c078d45ce650a036d4ee4753642b4afb9838`。

主要证据位于 `/tmp/a32_task17_seq21_24_edges.log`、
`/tmp/a32_r125_aligned_cau_release1_cch`、
`/tmp/a32_r125_aligned_cau_release1_sch`、
`/tmp/a32_r125_aligned_cau_release1_lsu_suite` 和
`/tmp/a32_cpu_*_r125`。下一断点是 task17 sequence24 VSEQ 到 sequence25
VMUL；task20 继续独立实现 stable tagged requester vector、LSU 高优先级与
persistent per-bank RR。

# 2026-08-10: scalar600/Sequencer R126 微时序断点

RTL operand requester 在最后一行 grant 令长度归零的同拍即可接收下一条
command；CAU 下游 arithmetic/result queue 不属于 requester command service。
gem5 去除这段重复计算的 4 cycles、保留 1-cycle registered handoff 后：

- task17 sequence0--95 的 fire/duration/recycle 全部 exact；
- 首个分歧推进到 sequence96 VSTORE：fire exact，RTL duration 52 cycles，
  gem5 42 cycles；由此 sequence97 VLOAD 在 gem5 提前 11 cycles；
- CCH/SCH VINS 10,253/8,103 task-local byte-exact，SCH task timing 不变；
- LSU 68/68，标量 runner 25/25（有效矩阵 24/24）；
- task17 为 `+20,329 ns`，task20 为 `-17,037 ns`，仍须独立处理，不能抵消；
- 二进制 SHA256：
  `04421048446c81bd96b6b3da4031052eb474e1cd6a55c84faa44c193b504a6d8`。

证据位于 `/tmp/a32_r126_cau_handoff1_{cch,sch,lsu_suite}`、
`/tmp/a32_cpu_*_r126` 和 `/tmp/a32_r126_task17_seq96_trace`。当前正在用 RTL
UCLI 波形把 sequence96 拆为 STU operand admission、逐 bank VRF grant、
AW/W/B 与 retirement；没有证据前不增加 LSU 固定 latency。task20 仍需
stable tagged requester vector、LSU 独立高优先级和 persistent per-bank RR。
# 2026-08-10: task20 scalar branch/load diagnosis (R60, rejected candidates)

Task20 sequence-1-to-2 的 757 个多余 tile cycles 已逐边闭合定位：
`branch->lhu` 为 `164*(8-6)=328`，`lhu->dependent branch` 为
`164*(4-2)=328`，taken branch recovery 为 `101*(4-3)=101`。动态 PC
序列一致，因此不是 VSTORE、vector LSU/VFU/VRF 或控制流功能问题。

全局 early-memory 在 SCH 产生功能错误；conditional/taken 分类虽把 SCH
改善到约 `+1.292 us` 且 8,103 VINS exact，却使 CCH task18 退化为
`-3.879 us / -3.148%`；first-follower 快路径只把气泡移到第二条 ALU。
这些候选均已撤销，当前源码/二进制恢复 R59 时序行为，不能按总 DAG
改善接受。完整证据见
`docs/venus_alignment/A32_R60_TASK20_SCALAR_BRANCH_LOAD_DIAGNOSIS.md`。

# 2026-08-11: requester grant/return 解耦审计（R240，拒绝）

RTL 源码确认 `stu_valid_mask` 当前常量为全 1，因此 gem5 的 STU 全 bank
高优先级不是 partial-bank 建模缺口。另一个候选把 requester 的最后 VRF
grant 与后续 DSPM 返回/operand-queue data capture 解耦，并保持完整 generation
tag；定向 case 还补出了同拍只能有一次 bank grant 的 `requester_q` 边界。

完整 CCH 候选运行保持 10,253 条 VINS byte-exact，但时序不成立：task17
由 `+501 ns` 退化到 `+573 ns`，task20 从 `-5,393 ns` 越过 RTL 到
`+4,153 ns`，task0/task2 从 `-7 ns` 变为 `+93 ns`。所以不能把 grant/return
解耦统一推广到全部 requester；缺失的是 requester-specific registered
ready/credit 与共享 bank request vector 的组合，而不是固定响应等待。

R240 源码改动已全部撤销，保留 R237 directional addrgen ack、stable tagged
owner、LSU 独立优先级和 persistent per-bank RTL-tree RR。详细证据和下一断点
见 `docs/venus_alignment/A32_R240_REQUESTER_RETURN_DECOUPLING_REJECTION.md`。
恢复二进制 SHA256 为
`dd6afa1effebba599f7dcacee73d5d41eafd6f2072ed01734f578c24b34cf1f3`；
LSU 68/68 与 BitALU/CAU/SerDiv depth=2/full capacity gate 均通过。

# 2026-08-11: BitALU tagged overlap / requester-bank R253

新采 RTL VCD 同时覆盖 BitALU `result_queue_cnt_q/d`、operand ready、result
request/grant 和 issue count。RTL 证明 enqueue 与 grant 同拍写同一个 `cnt_d`，
grant 后组合 ready 可在该边界重新评估；指令 issue pointer 也不等待旧 tagged
result commit pointer。

本轮保留三项通用修复：BitALU result count 的逐拍 q/d 累加、result-full
stall 在同拍 grant 后重评估 admission、相同 arithmetic pipe length 的年轻
指令与旧 tagged result 重叠。不同 pipe length 仍先 drain；没有 task/PC/DAG/
地址/数据特判，也没有修改 LSU fixed latency。

局部 oracle 结果：task20 BitAlu_B 相对 0--60 拍 event/usage 均 0 mismatch；
逐 bank 首分歧由 R247 第 24 拍、R250 第 30 拍推进到第 89 拍，且新首分歧
已经是 Shuffle requester。CCH/SCH 分别 10,253/8,103 VINS exact；最终 LSU
68/68、382 VINS exact；1/4/16/32/64/128 ns 容量门中 BitALU/CAU/SerDiv
均达到 depth=2/full 且功能 exact。

完整 task duration 仍未对齐：CCH task17 `+491 ns / +1.480%`，task18
`+3,465 ns / +2.812%`，task20 `-6,847 ns / -1.718%`。task20 比 R237 更早
1,454 ns，不能用局部收敛冒充整 task 收敛；下一步从第 89 拍 Shuffle
request admission/bank selection 继续拆。SCH timing 未变化，task2/3/4 分别
`+1,717/+6,933/+1,023 ns`。

详细报告：
`docs/venus_alignment/A32_R253_BITALU_TAGGED_OVERLAP_ALIGNMENT.md`。最终二进制
SHA256：
`d40d117e03104232d43361cc1ff897cf3bee990cbd934543a867c0ca94b82971`。

# 2026-08-12: VLDu tagged FIFO / final-bank-grant R406

RTL task17 与 PDSCH task3 波形确认，VLDu 不是单一固定 latency：它包含四槽
tagged issue queue、完整 destination hazard mask 的破坏性清除、Q -> QQ issue
pointer、depth-2 result FIFO、AXI R beat backpressure，以及实际 result-row
grant 边界上的 LSU 逐 bank 高优先级。本轮按这些结构实现，没有加入 task/PC/
DAG/地址/数据特判，也没有修改 LSU/VFU fixed latency。

Shuffle 最后 destination-bank grant 通过 generation tag 只更新 VLDu requester
私有完成表；普通 VINS dump、全局 hazard retirement 和 running-ID release 仍留在
原边界，避免把 requester 的早观察错误扩散成全局退休重排。

局部结果：nrPDCCH task17 sequence107 VLOAD 从 53 cycles 收敛到 RTL 的
35/35；PDSCH task3 sequence6 VLOAD 为 134/134，sequence7 VSHUFFLE 为
237/237。完整回归中 LSU 68/68，382 条 VINS 逐文件 byte-exact；CCH/SCH
分别 10,253/8,103 条按 task-local opcode/suffix 顺序和逐元素 exact。

当前主要剩余误差：CCH task17 已降到 `+15 ns / +0.045%`，但 task18 仍
`+3,465 ns / +2.812%`，task20 仍 `-9,973 ns / -2.502%`；SCH task2/3/4
分别为 `+2,695/+6,921/+1,363 ns`。全 DAG 仍有正负抵消，不能作为完成标准。
下一步保持 task17 与 SCH task3 已对齐前缀，从 task20 的首个 live requester
vector/bank winner/LSU priority/RR 分歧，以及 SCH task3 sequence7 之后的首个
fire/recycle 分歧继续拆。

完整报告：
`docs/venus_alignment/A32_R406_VLDU_TAGGED_FIFO_VRF_GRANT_ALIGNMENT.md`；证据：
`evidence/a32_r406_vldu_tagged_fifo_20260812`。最终二进制 SHA256：
`7dcd3f1d31f088ce18366c23337f7c2822ac07d679fede686d42059cb6bd61b7`。

# 2026-08-12: STU registered boundary / requester credit R426

本轮保留 vector LSU 的通用结构修复：LDU/STU PE-valid 均保持 tile 域边界；
AXI 相位改为 reset-defined，不再由首个 LSU 请求动态播种；所有 VSTORE 在
PE-valid 后进入独立的 registered `StoreOperands` 状态；STU addrgen ack 为
两拍边界，LDU 保留已验证的一拍边界。没有修改 LSU/VFU fixed latency。

task20 sequence2 已按 RTL oracle 对齐：RTL 为 `issue12 -> PE16 -> stu_req17 ->
valid18 -> localW19 -> WLAST31 -> intB46 -> retire47`，gem5 最终
`fire -> recycle` 同为 34 cycles。LSU 68/68、VFU depth=2/full 均通过；CCH
10,253 与 SCH 8,103 条 VINS 对 R406 task-ordinal/value exact。最终二进制
SHA256 为
`ab5eabfd49e8b576063ee99b425ff751ecd8a00e322403bd335242f78b280a20`。

完整 task 仍未收敛：CCH task17/task18/task20 分别为
`+119/+3465/-11439 ns`，SCH task2/task3/task4 为 `+2653/+6945/+1363 ns`。
其中 task20 比 R406 又快 1466 ns，不能用 sequence2 duration exact 或总 DAG
抵消冒充完成。

新 501-edge requester oracle 证明 lane0 BitALU-B 在 command capture 后由
registered writer boundary 生成并长期保留 `raw_q[0]`，直到 CAU 首行 grant。
因此下一断点是 capture 后到 raw_d edge71 之间的 stable writer VFU/address
元数据及 pre-edge Shuffle grant，而不是把 RTL 严格 `<` 改为 `<=`。之后再
继续 persistent per-bank RR。完整证据见
`evidence/a32_r426_stu_requester_alignment_20260812`。
======================================================================
当前断点：A32 R508 producer completion / task20 seq30 exact（2026-08-12）
======================================================================

完整报告：

    docs/venus_alignment/A32_R508_PRODUCER_COMPLETION_ALIGNMENT.md

1. 新增两级通用 tagged completion：所有 active lane 的 final bank grant
   只提前清除 Shuffle 捕获的精确 generation；Shuffle final destination-bank
   grant 只提前清除 sequencer-local consumer 的精确 generation。公开 PE
   response、VINS dump、hazard broadcast、VID release/retirement 均未提前。
2. RTL oracle 证明 task20 sequence29 消费 `global_hazard_table_d`，sequence30
   的 VSTU operand request 又可在前一条 Shuffle 公开 recycle 前开始。修复后
   sequence29 为 84/84 cycles，sequence30 VSTORE 为 116/116 cycles，后续
   sequence31/32 VLOAD 的 fire/recycle 也 exact；没有改 LSU/VFU latency。
3. task20 8,837 VINS exact，首个生命周期分歧推进到 sequence34/36：VADD
   快 1 cycle、VBRDCST 慢 1 cycle；sequence50 首次发射快 77 cycles 但回收
   同拍，下一步必须拆 admission/result queue/requester/逐 bank RR/retirement。
4. task17 687 VINS exact，首分歧保持 sequence180 VBRDCST -1 cycle；LSU
   68/68，382 条结果文件 exact。当前二进制 SHA256：

       d335761e3727285b2c5b4ed2bd5e1fde6f3ddb47eab29515f84aa9f5cfa2e50e

5. 最新 CCH 24/24、10,253 VINS exact。主要残差：task20
   -7,215 ns/-1.810%，task3 -271 ns，task14 +195 ns；task17 -13 ns、
   task18 -1 ns。task13 虽为 +8.703%，绝对值仅 +49 ns。功能 DAG span
   -8,096 ns/-0.776%，因正确早完成暴露了后段缺口，不作为拒绝理由或验收。
6. 最新 SCH 9/9、8,103 VINS exact。task2/3/4/8 分别为
   -373/+5,185/+443/-1,091 ns；功能 DAG +4,116 ns/+0.268%，仍有抵消。
7. 永久证据：

       evidence/a32_r508_producer_completion_20260812/

结论仍为 INCONCLUSIVE。下一步先拆 task20 sequence34/36 的 CAU/BitALU
operand admission、result enqueue、stable tagged requester vector、per-bank
grant/RR 与 retirement；随后保留 SCH task3 sequence6/7 exact 前缀继续推进。

======================================================================
当前断点：A32 R529 CAU pipeline/result-Q oracle（2026-08-13）
======================================================================

RTL 已把 task20 sequence33 的 CAU 边界拆清：edge17 同拍发生 operand
handshake、CAU output valid 和 `result_queue_d` enqueue；edge19 才由
`result_queue_q` 产生 VRF result request。task17 正常 VMUL oracle 同时证明
grant->consume=2 cycles、consume->result-D=2 cycles、D->Q=1 cycle。

因此 task20 的首个 stable requester vector 分歧确实是 CAU result requester
早一拍，不是 RR 初值/映射错误；但 task17 当前依赖“arithmetic enqueue 晚一拍
+ result request 早一拍”的抵消。全局推迟 result queue 会修好 task20
seq31--37，却从 task17 seq4 开始退化，不能接受。

本轮所有未通过的 CAU 行为实验已撤销。回退回归：task17 687/687、task20
8837/8837 条 instruction 的 fire/recycle/duration 分别与 R502/R525 逐条一致；
二进制 SHA256 为
`da186ad190cdad09983209b5ca42620d6962dfa73cf8ec52a1fc8bc1f982f09d`。

下一步不能再靠多个 gem5 event 的执行相位拼时序，需要把
`operand_queue_q -> CAU handshake -> arithmetic D/Q -> result_queue_d/q`
重构为一个显式 clocked transition，再分别用 task20 latency-zero 与 task17
RAW VMUL 两个 oracle 交叉验收。报告：
`docs/venus_alignment/A32_R529_CAU_PIPELINE_BOUNDARY_ORACLE.md`；证据：
`evidence/a32_r529_cau_pipeline_oracle_20260813/`。
======================================================================
当前断点：A32 R570 CAU 显式 D/Q 与 task20 sequence-50（2026-08-13）
======================================================================

完整报告：

    docs/venus_alignment/A32_R570_CAU_EXPLICIT_DQ_ALIGNMENT.md

1. CAU A--D operand response 已按 instruction/generation tagged，并显式拆为
   arithmetic pipeline D、depth-2 result queue D/Q、registered requester/wakeup
   visibility；没有修改 LSU/VFU 固定 latency，也没有 task/PC/DAG 特判。
2. 修正 CAU calculation index 更新顺序后，VRANGE 数据恢复 exact。task20
   sequence 25--49 的 fire/duration/recycle 全部 exact，原 sequence
   33/34/36/37 差异消失；首差异推进到 sequence 50 VBRDCST：fire -77、
   duration +78、recycle +1 cycles，仍存在明显内部抵消。
3. task17 为 +5 ns/+0.015%，687 VINS exact；首 duration/recycle 分歧为
   sequence 180 VBRDCST -1 cycle，首 fire 分歧为 sequence 194 VLOAD
   +1 cycle。
4. CCH 10,253 VINS exact；全 DAG -4,068 ns/-0.390%，主要独立残差仍为
   task20 -3,471 ns/-0.871%。task13 +8.703% 只有 +49 ns，短 task 普遍
   -7--11 ns，需后续独立检查 CPU/task boundary。
5. SCH 8,103 VINS exact；全 DAG +4,792 ns/+0.312%，存在 task3
   +5,485 ns 与 task8 -1,091 ns 抵消；task4 +843 ns 的首 fire 分歧为
   sequence 8 VDIV -17 cycles，下一模块优先拆 SerDiv D/Q 与逐 bank grant。
6. “per-lane completion board + final desync stall”对 task20 时序完全无影响，
   已回退。sequence50 分配时 gem5 直接使用 ID7，而 RTL 等待后复用 ID1；
   缺口位于更早的 per-lane ready/completion、lane fall-through 或 PE-ready
   返回链，不能用全局 stall 修补。
7. LSU 回归保持 68/68、382 VINS exact。二进制 SHA256：

       3275a609895bdf82e4f2977ca734efabdad68cac60c7ceb52c0d843392222275

永久证据：

    evidence/a32_r570_cau_explicit_dq_20260813/

结论仍为 INCONCLUSIVE。下一步先拆 task20 sequence43--51 的逐 lane
ready/completion、stable tagged requester、LSU 独立高优先级与 persistent
per-bank RR；随后拆 SCH task4 SerDiv、task3 长序列斜率变化和 task8
Shuffle/LSU 累积。禁止使用总 DAG 抵消作为验收。

======================================================================
当前断点：A32 R652 masked requester VFU_NONE / BitALU mask D/Q（2026-08-13）
======================================================================

完整报告：

    docs/venus_alignment/A32_R652_MASKED_REQUESTER_VFU_NONE_ALIGNMENT.md

RTL direct oracle 证明：当 `vm_r | vm_w` 时，普通 BitALU/CAU/SerDiv operand
requester 的 `vfu` 必须是 `VFU_NONE`，只有独立 mask requester 保留目标 VFU。
gem5 已在 command capture 与动态 writer eligibility 两处按此实现；没有
task/sequence/opcode/PC/address/data 特判，也没有修改 LSU/VFU 固定 latency。

修复后 task20 sequence59 VBRDCST 从 31 cycles 收敛到 RTL 的 30 cycles，
首 duration/recycle 分歧推进到 sequence60 VXOR：32 vs 34 cycles；首 fire
分歧为 sequence63 +1 cycle。sequence60 A/B 首次 VRF grant 已与 RTL 同为
9850 ns，剩余差异位于 BitALU 内部独立 mask D/Q latch 与 registered result
queue：RTL 首结果晚一拍，并在结果 row3/row4 之间有一拍 bubble。

把 mask 串行放进组合 A/B consume 路径的原型虽然令 sequence60 duration
碰巧 exact，却使此前 exact 的 sequence57 慢两拍并改变 VINS，已完整回退。
下一步必须实现可在 A/B 等待期间独立握手的 tagged mask D/Q latch，并用
sequence57 与 sequence60 双 oracle 验收。

全链路结果：CCH task20 从 `+9.201 us/+2.309%` 降到
`+6.609 us/+1.658%`；CCH DAG 从 `+8.533 us/+0.819%` 降到
`+5.941 us/+0.570%`。SCH 不变，仍为 `+4.805 us/+0.313%`，其中 task3
`+5.501 us/+1.167%`、task4 `+0.843 us/+2.794%`、task8
`-1.091 us/-0.330%`，存在明显抵消。

CCH 10,253、SCH 8,103 条 VINS 分别对接受基线 exact；LSU 68/68、382 条
结果文件 byte-exact。最终二进制 SHA256：

    8ae9ce51e08bd4f125d3660df7ce4448c2c187d9cd46eea09a52f074417996ee

结论仍为 INCONCLUSIVE。BitALU mask/result D/Q 完成后，继续拆 task20 的
stable requester vector、LSU 独立高优先级、persistent per-bank RR 与 tagged
retirement；SCH 独立推进 task3 SerDiv/CAU/LSU overlap、task4 SerDiv result
D/Q、task8 Shuffle/LSU contention。总 DAG 误差与任务间抵消不得作为验收。

======================================================================
当前断点：A32 R654 BitALU mask D/Q + mask requester admission（2026-08-13）
======================================================================

完整报告：

    docs/venus_alignment/A32_R654_BITALU_MASK_DQ_ADMISSION_ALIGNMENT.md

1. BitALU mask 已显式拆为独立、带 instruction/generation tag 的 D/Q latch：
   可在 A/B 等待时独立握手，Q 下一拍可见，每行供四个 EW8 arithmetic beat，
   clear/refill 保留真实 registered bubble。masked BitALU 不再叠加虚构 ingress
   pipe；没有 task/sequence/PC/data 特判，也没有修改 LSU/VFU fixed latency。
2. `venus_mask_operand_requester.sv` 证明 mask command 在 IDLE 不等 hazard clear
   即可入 requester_q，但其后必须等待 registered hazard clear，且没有普通
   requester 的 raw_hazard_counter chaining。gem5 已按此拆分 admission 与 service。
3. task20 sequence57/60/62/63 已 exact，首分歧推进到 sequence64 VSADD：
   fire -9、duration +8、recycle -1 cycles。新 RTL CAU oracle 证明 downstream
   mask D/Q、depth-2 result queue 与 retire 形状基本一致；主要缺口在 main
   sequencer fire、lane fall-through capture 与共享 mask command release 的边界。
4. task20 8,837、task17 687 条 VINS exact。task17 首 duration/recycle 分歧仍为
   sequence180 VBRDCST -1 cycle，首 fire 分歧仍为 sequence194 VLOAD +1 cycle。
5. CCH 10,253 VINS exact。task20 现在 -3,183 ns/-0.799%，全 DAG
   -3,851 ns/-0.370%；此前候选为 +6,805 ns/+0.653%，10.656 us 的反向移动
   全来自 task20。该 overshoot 证明总误差不能选模，不能回退到内部边界错误但
   总数更近的模型。
6. SCH 完全不变，8,103 VINS exact；全 DAG +4,805 ns/+0.313%。主要残差仍为
   task3 +5,501 ns、task4 +843 ns、task8 -1,091 ns，存在明显抵消。
7. LSU 68/68、382 结果文件 byte-exact；BitALU/CAU/SerDiv result queue 均实际
   达到 depth=2/full，1/4/64 ns grant-gap 输出 exact。最终二进制 SHA256：

       b1dd7e91485de08f70702deca3c5b7edac0678edbf1c9f00053b250a1526e469

永久证据：

    evidence/a32_r654_bitalu_mask_dq_admission_20260813/

结论仍为 INCONCLUSIVE。下一步先以 task20 sequence63--65 对齐
main-sequencer `pe_req_valid_o`/fire、lane fall-through 与共享 mask command
释放，再进入 stable tagged requester vector、LSU 独立高优先级和 persistent
per-bank RR。SCH task3/4/8 与短 task CPU boundary 仍需独立推进，禁止用总 DAG
或 fire/duration/recycle 内部抵消验收。

======================================================================
当前断点：A32 R682 mask-bank RR / task20 sequence66（2026-08-13）
======================================================================

完整报告：

    docs/venus_alignment/A32_R682_MASK_BANK_RR_TASK20_SEQ66.md

本轮按 RTL `venus_mask_operand_requester.sv` 补齐了 16 个 lane-local mask
bank 的独立、persistent 2-requester fair RR：mask read 为 requester0，BitALU
mask write 为 requester1。原有 64 个 data bank 的 12-requester tree、LSU
独立高优先级和 live intent 作用域保持不变。没有 task/sequence/opcode/PC/
address/data 特判，也没有修改 LSU/VFU fixed latency。

正确 `venus-rtl-16x128` profile 下，task20 仍在 tick 397,080,000 完成，
8,837 条 VINS 对 R653e exact。首 duration/recycle 分歧仍为 sequence66 VSLE：
gem5 62 cycles、RTL 63 cycles（-1）；首 fire 分歧仍为 sequence70 VMUL -1。
这证明 mask-bank RR 是真实结构缺口，但不是当前首差的直接 owner。

两类过宽候选已完整回退：BitALU mask completion 全局 +1 会使 sequence57
先回归；所有 operand ibuf row 全局 registered visibility 会使 sequence33
VRANGE 先出现 +3 cycles，task20 总时长增加 5.480 us；只限 BitALU A/B
仍使 sequence33 +3，且总时长增加 1.448 us。

RTL 源码进一步确认 `operand_issued_i`/`ibuf_usage_q` 只表示 requester credit，
真正 data FIFO 有独立 `operand_valid_i`。下一步必须拆 stable tagged request、
VRF response data-valid、fifo_v3 Q、VFU valid/ready、result queue D/Q、mask grant
与 retirement，不能把 grant 或 response 合并成固定一拍。先用 empty queue 与
depth-2/full 微 case 验收，再守护 sequence46/57 并推进 sequence66。

永久证据：

    evidence/a32_r682_mask_bank_rr_20260813/

当前结论仍为 INCONCLUSIVE；CCH/SCH 全链路沿用 R654 已验证数值，本轮未用
总 DAG 重新选模，也未宣称 task20 已对齐。

======================================================================
当前断点：A32 R739 tagged LSU priority / VSTU lookahead（2026-08-14）
======================================================================

完整报告：

    docs/venus_alignment/A32_R739_TAGGED_LSU_STU_LOOKAHEAD_ALIGNMENT.md

RTL task17 sequence180 oracle 证明连续两个 tile 边沿均为 STU operand request，
不是 LDU 或第二条 VSTORE；这是单条短 VSTORE 在 `issue_cnt_bytes_q` 尚未清零时
对空闲 ping-pong row 的一行 lookahead。gem5 已补一行物理 STU operand
ownership，并把 LSU 高优先级从全局 tick 改为稳定 `(tag, bank_mask)` vector。
普通逐 bank RR 只在普通 requester 真正 grant 后更新，因此 LSU 插入不会破坏
persistent RR。没有修改 LSU/VFU/CPU/DAG fixed latency，也没有 task/PC/DAG/
address/data 特判。

task17 sequence180 已 exact；首 duration/recycle 分歧推进到 sequence191
VLOAD -2 cycles，首 fire 分歧为 sequence194 VLOAD +1 cycle。task17 总时长
33,170 ns 对 RTL 33,167 ns：+3 ns/+0.009%。task20 首分歧仍为 sequence66
VSLE duration/recycle -1 cycle，sequence70 VMUL fire -1 cycle；总时长
397,072 ns 对 398,523 ns：-1,451 ns/-0.364%。

CCH 24/24、10,253 VINS exact；全 DAG 1,040,080 ns 对 RTL 1,042,151 ns：
-2,071 ns/-0.199%。SCH 9/9、8,103 VINS 对接受基线 exact；全 DAG
1,539,012 ns 对 RTL 1,534,187 ns：+4,825 ns/+0.314%。SCH 主要独立残差为
task3 +5,461 ns/+1.158%、task4 +903 ns/+2.993%、task8
-1,091 ns/-0.330%，仍有明显抵消。保留 RTL dump 仍只有 7,256 条，另外
847 条未冒充 RTL direct compare。

回归门：LSU 68/68，382 结果文件与接受基线逐路径/哈希 exact；BitALU/CAU/
SerDiv depth=2/full 压力门通过。最终二进制 SHA256：

    73f9592a115a4074e3bd69c737a6a4e1b0af2e71b110bc4dab595dd2ccfd820b

永久证据：

    evidence/a32_r739_tagged_lsu_stu_lookahead_20260814/

结论仍为 INCONCLUSIVE。下一步保持 task17 exact 前缀，拆 task20 sequence
66--70 的 stable request、VRF response data-valid、operand FIFO Q、VFU
valid/ready、result D/Q 和 tagged retirement；SCH 独立拆 task3 重复
CAU/SerDiv/LSU overlap、task4 SerDiv result D/Q、task8 Shuffle/LSU contention。
禁止以 CCH/SCH 全 DAG 百分比或 task 间抵消验收。

======================================================================
当前断点：A32 R864 scalar ID queue / CCH-SCH 复测（2026-08-15）
======================================================================

完整报告：

    docs/venus_alignment/A32_R864_SCALAR_ID_QUEUE_VSTU_PHASE_ALIGNMENT.md

scalar600 ID 不再把仍停留在 Minor Execute 输入队列的生产者当作 RTL EX
组合旁路；控制指令必须等待匹配生产者实际 issue 后，才能从有序 EX writer
chain 取值。该修复消除了 task17 PC 0x120c 的 stale-value 假分支预测，没有
task/PC/DAG/address/data 特判。

错误的全局/奇偶相位 `Venus extension -> scalar load` commit hold 已回退。
RTL direct trace 表明 task17 是第二字到 read request 3 cycles、request 到 retire
5 cycles；SCH task8 代表序列为 2+5。把 request admission 错建为 retirement
delay 会把 task8 从 -1.093 us 推到 +1.135 us，并反馈改变后续相位。

当前 CCH allocation-to-last-release 为 1,042.564 us，对 RTL 1,042.151 us：
+0.413 us/+0.039630%。task17 为 -0.059 us/-0.178%，task20 为
-0.009 us/-0.002%。task17 sequence0--193 exact，首 fire/recycle 分歧仍为
sequence194 VLOAD -2 cycles；sequence195 VBRDCST duration 13 vs 12。

当前 SCH 为 1,539.996 us，对 RTL 1,534.187 us：+5.809 us/+0.378637%。
主要残差为 task3 +5.527 us/+1.172%、task4 +0.929 us/+3.079%、task8
-1.093 us/-0.330%，仍有明显抵消。

CCH 10,253、SCH 8,103 条 task-local VINS exact；LSU 68/68、382 VINS
byte-exact；scalar suite 52/52 正常退出，除旧 ecall_nop fixture 外 1,258 条
retire 与 1,258 条 writeback direct-RTL exact。最终二进制 SHA256：

    f6c8eb43e658f2f2b31b8375b0fbca5a6fd0c60815291dba42b1d75ef905282c

永久证据：

    evidence/a32_r864_scalar_id_queue_20260815/

结论仍为 INCONCLUSIVE。下一步必须显式建模 registered
`Venus extension -> scalar LSU request` admission，并把 request/retire 两条边界
分别与 RTL 验收；不能再用 commit latency、总 DAG 百分比或任务间抵消调参。

======================================================================
当前断点：A32 R868 registered Venus-request release（2026-08-15）
======================================================================

完整报告：

    docs/venus_alignment/A32_R868_REGISTERED_VENUS_REQUEST_RELEASE_ALIGNMENT.md

RTL 420-sample direct oracle 证明：受阻的两字 Venus 请求在 ready 握手后，
后继 scalar load 先经过一个空泡，第二个 scalar edge 才进入 EX/LSU；LSU
complete 在第 7 edge，WB-visible 在第 8 edge。gem5 现把 backpressure 状态绑到
具体动态 VENUSEXT，请求未受阻时不改变原行为；没有 task/PC/DAG/address/data
特判，也没有修改 vector LSU/VFU fixed latency。

CCH task17 sequence0--229 的 normalized fire/recycle 已 exact：旧 sequence194
VLOAD 的 -2 cycles 和 sequence195 VBRDCST duration +1 均消失。新首差为
sequence230 VLOAD duration/recycle -2，首 fire 差为 sequence231 VBRDCST -1。
task17 从 -59 ns 改善到 -55 ns。CCH 全 DAG 仍为 RTL/gem5
1,042.151/1,042.564 us，+0.413 us/+0.039630%，因为 critical task20 未变。

SCH task8 从 -1.093 us/-0.330% 收敛到 +0.019 us/+0.006%。去掉这项负误差
抵消后，全 DAG 变为 RTL/gem5 1,534.187/1,541.108 us，
+6.921 us/+0.451118%；剩余主要为 task3 +5.527 us、task4 +0.929 us、
task2 -0.327 us。总误差变大不构成回退，反而暴露了原来的任务间抵消。

回归门：CCH 10,253、SCH 8,103 VINS byte-exact；scalar600 52/52；vector
LSU 68/68、382 VINS。测试二进制 SHA256：

    e6873fcf4ecec8ffdca00b3a79e7a431c39c35849288c8f22d5edcdfd7234d07

永久证据：

    evidence/a32_r868_venus_request_release_20260815/

结论仍为 INCONCLUSIVE。下一步先拆 task17 sequence230 VLOAD 的 result
enqueue/per-bank VRF grant/recycle，再拆 SCH task3/task4 的 operand admission、
SerDiv/VFU result-Q 与 retirement。task20 虽只有 -9 ns，stable tagged
requester vector、LSU 独立高优先级和 persistent per-bank RR 仍需结构验收。

======================================================================
当前断点：A32 R871 multi-beat WLAST/addrgen boundary（2026-08-15）
======================================================================

完整报告：

    docs/venus_alignment/A32_R871_MULTIBEAT_WLAST_ADDRGEN_ALIGNMENT.md

task17 sequence230 的 RTL direct oracle 已证明差距位于 multi-beat STU WLAST
到反向 LDU addrgen 的注册边界，而不在固定 VLOAD latency、result enqueue、
per-bank grant 或 retirement。gem5 仅在 multi-beat VSTORE 完成与反向 LSU
请求恰好同 edge 时增加一个 direction-select D/Q 边界；single-beat、同方向和
晚到请求不变，没有 task/PC/DAG/literal-address/data 特判。

CCH task17 从 -55 ns 收敛到 -3 ns，sequence0--679 的 normalized
fire/duration/recycle 已逐条 exact；首差推进到 sequence680 VLOAD fire +2
cycles，且该 VLOAD 自身 duration 61/61 exact。task20 的 8,837 条 vector
instruction lifecycle 已全部 exact。CCH 全 DAG 因 critical task20 不变，仍为
RTL/gem5 1,042.151/1,042.564 us，+0.413 us/+0.039630%。

SCH task4 的五个同构 multi-beat store-to-load 边界增加 20 ns，因此全 DAG 为
RTL/gem5 1,534.187/1,541.128 us，+6.941 us/+0.452422%。这项结构修复虽使
task4 总差从 +0.929 us 变成 +0.949 us，但不能因 aggregate 变差而删除正确
边界。剩余主项是 task3 +5.527 us/+1.172%、task4 +0.949 us/+3.145%、
task2 -0.327 us/-0.592%。

回归门：CCH 10,253、SCH 8,103 VINS byte-exact；scalar600 execution
52/52，1,258 retire + 1,258 writeback 对有效 RTL oracle exact；vector LSU
68/68、382 VINS。测试二进制 SHA256：

    baa230f69aac17824e1953486794963f862a73054453d88d4c7f0d72992dbb7e

永久证据：

    evidence/a32_r871_multibeat_wlast_addrgen_20260815/

结论仍为 INCONCLUSIVE。下一步先拆 task17 sequence679 recycle 到 sequence680
fire 的 +2-cycle admission gap；SCH task3 优先拆 recurring inter-sequence
+215/+218 cycles 与 SerDiv result-Q/retire，task4 从 sequence7 VSADD 的
-1-cycle recycle 首差开始。task20 的 lifecycle exact 作为回归门，但仍需用
内部 oracle 验证 tagged requester vector 与 persistent per-bank RR。
