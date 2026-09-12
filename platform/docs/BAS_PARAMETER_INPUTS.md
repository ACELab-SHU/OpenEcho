# 可选的 BAS 参数输入文件

外部参数是原有 BAS 初始化之外的可选输入方式。没有 `--params`、也没有
`dag-source.json.parameters` 时，BAS 原文件按原路径复制和编译，不经过新解析器。
不要求迁移旧 BAS；也不会把旧 BAS 注释中的测试向量当成有效数据。

## 同一语法，明确注释

```text
' 这整行是注释，不会产生输入
parameter short mode = {15} ' 行尾注释
parameter char samples = {
    1, -2, 3, ' 注释中的数字 999 不参与初始化
    4
}
' parameter short mode = {30}  这行不覆盖 mode
```

语法为 `parameter 类型 参数名 = {数值列表}`，不是 `' parameter ...`。
类型支持 BAS 的 `char`、`short`、`int`、`float`、`double`；数值使用十进制
字面量和负号，浮点数可以使用大写 `E` 指数。与现有 BAS 数值列表语法一致，
不支持空列表、尾逗号、十六进制、表达式、函数调用或任意 Python 代码。
支持多行、Windows CRLF、空行、整行及行尾 `'` 注释。

整数按位表示范围检查：char 为 -128..255，short 为 -32768..65535，
int 为 -2147483648..4294967295；超范围或给整数类型填写浮点数会报错。
这表示字节/字的可表达范围，不改变实际 C/任务接口的有符号语义。

## 静态 parameter：移出大数组或覆盖默认值

例如 BAS 保留任务结构和默认配置：

```text
parameter short mode = {15}
dag dag1 = {
    [result] = Task_example(samples, mode)
}
```

另建 `case.params` 定义 `samples`，可选覆盖 `mode`：

```text
parameter char samples = {1, -2, 3, 4}
parameter short mode = {30}
```

编译时：

```bash
./ace-echo compile dag --target YOUR_DAG --params /path/to/case.params
```

或者放入应用目录，用原有 `dag-source.json` 选择默认参数文件：

```json
{
  "schema": "ace-echo-dag-source/v1",
  "source": "YOUR_DAG.bas",
  "parameters": "cases/default.params"
}
```

显式 `--params` 优先于应用默认文件。外部同名参数覆盖 BAS 初始化；未提及的
BAS 参数保持不变。类型必须一致，重复外部定义报错。外部独有参数必须被 BAS
引用，未知名字报错。不能把 global/return_value 改成 parameter。
静态 parameter 的数组长度由本轮 initializer 决定，仍由 DSL 分配与检查。
若静态数组改变长度，要同时确认任务的处理长度和消费者契约。

## 运行时 dag_input / dfedata：保持原有 ABI

```text
dfedata char iq[4420]
dag_input short mode[1]
```

对应参数文件仍使用同样的值定义：

```text
parameter char iq = {1, -2, 3, 4}
parameter short mode = {15}
```

BAS 的 `dfedata` / `dag_input` 声明不变，不转换为静态 parameter。
容量取自 BAS 的 `[长度]`；超容量是错误，不足部分由 C 静态初始化补零。
只要参数文件选择了运行时输入，就必须完整提供该 BAS 的全部运行时输入，
避免混用旧用例。仅替换静态 parameter 时不要求填写运行时输入。

编译器适配器输出 `ace_echo_inputs.inc`，Scheduler 构建自动根据同目录的
`parameters.json` 接收它。L1 main 必须明确包含这个文件：

```c
#ifdef ACE_ECHO_EXTERNAL_INPUTS
#include "ace_echo_inputs.inc"
#else
/* 原有默认输入，继续兼容 */
#include "my_default_inputs.inc"
#endif
```

Scheduler 只在隔离副本中启用宏，并保留原有 FIFO、参数顺序、fire 和输出处理。
main 没有接入外部 include 时构建报错，不会静默忽略输入。
外部运行时输入应通过 `scheduler build` + `run application --scheduler-engine firmware`
执行；直接 `run dag --case-dir` 仍使用其 case-dir 数据，不会加载这个 C include。
本功能不生成任意应用的 L1 调度逻辑，也不生成新的 golden。

PDCCH TV9 的 `src/main_nrPDCCH_tv9.c` 已接入这个可选 include；示例输入是
`components/scheduler/dags/input/nrPDCCH_tv9.params`。其中 19 个字段从旧格式迁移，
数值不变。旧的“带注释符却被专用脚本读入”的 `.params` 写法不再作为新语法支持；
用户自有这种旧参数文件需移除有效定义前的 `'`。这不影响旧 BAS 的正常初始化。

## 证据与不变项

编译输出目录额外保留：

- `input.params`：实际读取的参数文件；
- `materialized.bas`：本轮交给 DSL 的 BAS；
- `parameters.json`：来源/合并/输入和 DAG JSON/BIN 的 SHA256、覆盖字段；
- `ace_echo_inputs.inc`：存在外部运行时输入时生成。

将 DAG JSON/BIN 与这些同目录文件一起交给 Scheduler；摘要不匹配会拒绝构建。
不要只搬走 JSON/BIN 后丢弃参数绑定文件。保留普通编译产物目录最简单。
原始 BAS、任务源和 RTL 均不由此功能修改。有效字段、返回容量、golden 和
RTL 比较规则不因输入移到外部文件而放宽。

## 本轮验证（2026-09-10）

实现提交 `41a9e52`，在 shenyihao 的 Venus 2.0 环境验证
`nrPDCCH_2p0_full_ce_candidate_a5` / TV9：

- 347 项自动测试通过，覆盖注释、类型、数值、重复定义、容量、CRLF、
  文件路径、输入摘要绑定、main 接入和旧 TV9 数值迁移。
- 原 BAS 默认输入、外部运行时输入、外部静态参数重放，三组 fast 均通过
  53 个任务输出检查点及 6 个独立旧 golden 数组检查。
- 不传外部输入时，固件 SHA256 与此前验收版本完全相同；静态参数以相同
  数值重放也得到同一固件。外部运行时输入改变存储布局，固件摘要不同，
  但输出一致。
- 外部运行时输入固件在新构建的 Venus_3 RTL 上，53/53 输出按已有有效位
  契约一致。资格状态为 `SMOKE_PASS_WITH_AXI_UNKNOWN_AND_KNOWN_NONFUNCTIONAL_WARNINGS`：
  保留 37 条 AXI unknown 和 1 条已知非功能启动告警，不声称 padding 全位一致。
- RTL 差异、配置保持不变；成功后按默认策略删除大型构建副本并保留比对证据。

本次使用已有的离线可构建 DSL `4f8c78b`、已有 fast 二进制和 Venus_3 RTL
`bf82d804`，三组环境相同。干净子模块的早期尝试遇到 JSON 头文件下载 HTTP 502；
未为此改工具链源码，也不把本次结果视为干净机器 bootstrap 已通过。
此记录限定于该 DAG/配置，不代表所有应用或后端均已逐一验证。

详细证据位于 shenyihao 工作树的
`runs/optional-bas-params-20260910-a005/final-report.json`；
软件对比为 `software-comparison.json`，硬件报告为
`rtl/artifacts/rtl/validation-report.json`。
