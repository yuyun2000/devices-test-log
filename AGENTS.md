# AGENTS.md

本仓库用于保存 M5Stack 硬件测试与问题修复证据。

开始新增或更新测试记录前，必须先读取 [docs/hardware-test-workflow.md](docs/hardware-test-workflow.md) 和 [docs/record-format.md](docs/record-format.md)。实际测试任务完成后，除非用户明确要求仅本地，默认完成验证、提交、推送和远端回读；不要让结果只留在对话或测试中心。

## 新增测试记录时

- 使用 `devices/<device-slug>/<YYYY-MM-DD>-<topic>/` 目录。
- 从 `templates/hardware-test/` 复制基础文件，并补齐 `README.md`、`report.md`、`run.json` 和 `evidence/`。
- 同时更新设备级 README 和根 README 索引。
- 结论必须来自真实构建、日志、协议帧、测试中心任务或实机行为。
- 分开记录构建、传输、外设响应、数据内容和视觉结果，未执行的层级标为 `NOT RUN`。
- patch 必须能对声明的 base commit 通过 `git apply --check`。
- 同一问题的复测追加 follow-up execution 和独立 evidence，不覆盖已有原始记录。
- task 的 `passed` 不能替代应用层行为验证，必须引用串口、协议、数据、测量或相机证据。

## 文件与安全

- 文本文件使用 UTF-8 无 BOM；JSON 必须可解析。
- 不提交凭据、私钥、客户数据或未脱敏的唯一设备信息。
- 不公开内网地址、测试槽 ID、MAC/序列号、RFID EPC/UID；需要脱敏时保留原始日志 SHA256、task 和 session 以供内部追溯。
- 不提交 `.pio/`、`build/`、固件二进制、ELF 或 map 文件，除非该二进制本身是明确要求的交付物。
- 不删除或改写既有原始证据；更正结论时追加说明并保留可追溯历史。

## 发布

- 只暂存本次报告文件，确认 `git diff --cached --check`、JSON、UTF-8、链接和敏感信息扫描通过。
- 推送后确认 `HEAD...origin/main` 为 `0 0`，并回读远端关键文件验证内容一致。
- 公共 Issue 只有在任务明确要求时才回复；必须使用固定 commit 链接，并保留未覆盖项，不擅自关闭 Issue 或创建 PR。
