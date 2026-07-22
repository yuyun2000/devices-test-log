# AGENTS.md

本仓库用于保存 M5Stack 硬件测试与问题修复证据。

## 新增测试记录时

- 使用 `devices/<device-slug>/<YYYY-MM-DD>-<topic>/` 目录。
- 从 `templates/hardware-test/` 复制基础文件，并补齐 `README.md`、`report.md`、`run.json` 和 `evidence/`。
- 同时更新设备级 README 和根 README 索引。
- 结论必须来自真实构建、日志、协议帧、测试中心任务或实机行为。
- 分开记录构建、传输、外设响应、数据内容和视觉结果，未执行的层级标为 `NOT RUN`。
- patch 必须能对声明的 base commit 通过 `git apply --check`。

## 文件与安全

- 文本文件使用 UTF-8 无 BOM；JSON 必须可解析。
- 不提交凭据、私钥、客户数据或未脱敏的唯一设备信息。
- 不提交 `.pio/`、`build/`、固件二进制、ELF 或 map 文件，除非该二进制本身是明确要求的交付物。
- 不删除或改写既有原始证据；更正结论时追加说明并保留可追溯历史。
