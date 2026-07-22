# 硬件测试、归档与发布工作流

## 适用范围

本流程适用于 M5Stack 硬件实机测试、驱动问题修复验收、硬件在环测试和循环回归。目标是让测试结论可以被复查、重建和继续追加，而不是只存在于某个 Codex 对话或测试中心日志中。

当用户只要求查看日志、解释现象或进行只读诊断时，不自动发布。用户要求实际执行测试、复测、循环测试或修复并验收时，默认完成本流程并推送到本仓库；用户明确要求仅本地时除外。

## 固定位置

- 本地报告仓库：Codex 测试项目根目录下的 `devices-test-log/`
- 远端仓库：`https://github.com/yuyun2000/devices-test-log`
- 记录目录：`devices/<device-slug>/<YYYY-MM-DD>-<topic>/`
- 记录格式：[record-format.md](record-format.md)
- 基础模板：`templates/hardware-test/`

父目录 `devices-test` 是测试工作区，包含多个源码仓库和临时产物。只能在本报告仓库中暂存、提交和推送报告文件，不要在父仓库批量暂存。

## 1. 明确目标和验收层级

开始前记录四项：

1. 目标：要证明的用户行为或工程结果。
2. 上下文：问题来源、源码版本、芯片/板卡、外设、连接和 SDK。
3. 约束：兼容范围、资源限制、禁止修改项和设备状态变更边界。
4. 验收：构建、协议、数据、视觉或物理行为分别如何判定。

将验收拆成 `build`、`flash`、`transport`、`device_response`、`data_behavior` 和 `visual_or_physical`。每层单独使用 `PASS`、`PARTIAL`、`FAIL` 或 `NOT RUN`，不要用一个总状态掩盖未覆盖项。

## 2. 先取证，再修改

- 读取真实 Issue、源码、最近 diff、构建配置、日志和官方协议或数据手册。
- 调试按“现象分类 -> Top 3 假设 -> 证据 -> 最小实验 -> 修改”推进。
- 修复保持最小且保护公共 API；patch 必须声明上游仓库和 base commit。
- ISR、实时路径和共享状态改动要额外检查阻塞、动态内存、锁、stack 和日志量。

## 3. 构建与实机执行

- 保存完整构建环境、命令、依赖版本、RAM 和 flash 结果。
- 使用测试中心时，先读取 `device-test-center` skill，再调用一次 overview 选择在线、空闲且匹配的设备。
- 优先复用已验证 artifact；需要烧录、擦除或其他设备状态改变时，先获得用户确认。
- 记录硬件连接、模块版本、artifact ID、固件 SHA256、大小、task ID、task 类型、task 终态和 serial session ID。
- task 的 `passed` 只证明任务计划完成。应用行为必须由串口标记、协议帧、数据匹配、测量结果或相机证据单独证明。
- 串口默认读取 `tail10`；只有需要完整审计且数据量有界时才读取 `all`，同时记录 bytes、chunks、dropped、truncated 和 SHA256。

## 4. 建立归档

每次运行至少包含：

```text
devices/<device-slug>/<YYYY-MM-DD>-<topic>/
|-- README.md
|-- report.md
|-- run.json
|-- evidence/
|-- patches/    # 有代码修复时
`-- hil/        # 有可重建测试工程时
```

- `README.md`：快速结论、关键任务和文件导航。
- `report.md`：根因、修复、构建、实机证据、验收边界和回滚。
- `run.json`：机器可读的源码、硬件、固件、执行和分层检查结果。
- `evidence/`：原始日志、协议帧、测量数据、事件摘要或图片。
- `patches/`：可直接应用的最小修复。
- `hil/`：固定依赖、可重建的硬件在环测试工程，不含大型构建产物。

同一问题的后续复测优先追加 follow-up execution 和独立 evidence，不覆盖首次运行。修正结论时保留时间线，并说明哪些状态发生了变化。

## 5. 公开前脱敏

本仓库是公开仓库。提交前检查：

- token、API key、Wi-Fi 密码、私钥和客户数据；
- 内网 URL、IP、服务器路径和测试槽 ID；
- MAC、序列号、RFID EPC/UID 及其他唯一设备标识；
- 未经授权的固件二进制或第三方数据。

必须隐藏唯一标识时，保留有序的脱敏摘要，并记录原始数据 SHA256、task ID、session ID、字节数和 dropped 状态。文件名应包含 `redacted` 或 `summary`，正文明确说明删除了什么；不能把脱敏内容标为 raw。

## 6. 提交前验证

至少执行：

1. `git diff --check` 或暂存后的 `git diff --cached --check`。
2. 所有 JSON 能解析。
3. 文本可严格按 UTF-8 解码且无 BOM，关键中文仍存在。
4. Markdown 相对链接全部存在。
5. 敏感信息扫描无命中；命中协议字段或示例时人工确认。
6. patch 对声明的 base commit 通过 `git apply --check`。
7. HIL 工程从归档目录重新构建成功，构建产物保持 ignored。
8. `git status` 只包含本次报告文件，没有其他源码仓库或用户修改。

验证应与风险匹配。纯 follow-up 报告不需要重复构建未变化的 HIL 工程，但必须把新增统计与原 task/session 自动比对。

## 7. 提交、推送与远端回读

在 `devices-test-log` 仓库中只暂存本次文件，检查 staged diff 后提交并推送 `main`。完成条件：

- 本地工作区干净；
- `git rev-list --left-right --count HEAD...origin/main` 输出 `0 0`；
- 远端 commit 存在；
- 远端 README、报告、`run.json`、patch 或新增 evidence 与本地 SHA256 一致。

报告和 Issue 评论使用完整 commit SHA 的固定链接，不只使用可能继续变化的 `main` 链接。

## 8. Issue 跟进边界

只有任务明确要求处理或回复公共 Issue 时才发送评论。先推送报告，再评论，并包含：

- 根因和最小修复；
- 构建及实机层级结果；
- 关键协议或行为证据；
- `PARTIAL` / `NOT RUN` 的真实限制；
- 固定 commit 下的 report、patch、HIL 和 evidence 链接。

后续复测使用新评论保留时间线，不覆盖旧评论。除非用户明确授权，不关闭 Issue、不创建 PR、不推送上游分支，也不声称代码已合并。

## 9. 最终交付

最终回复至少提供：仓库、commit、报告、patch（如有）、关键 evidence 和 Issue 评论链接；同时说明验证通过项与剩余风险。任何无法执行的验证都要明确说明原因和补测条件。
