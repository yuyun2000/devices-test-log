# devices-test-log

M5Stack 硬件问题复现、修复验证和循环测试记录仓库。

这里保存可复查的工程证据，不只保存最终结论。每条记录应尽量包含问题来源、硬件连接、代码或 patch、固件哈希、测试任务 ID、原始日志、验收结果和未覆盖项。

## 测试记录

| 设备 | 日期 | 主题 | 结果 | 记录 |
| --- | --- | --- | --- | --- |
| Unit UHF-RFID | 2026-07-22 | GitHub Issue #16: `readCard()` 忽略 `sa` | PASS（协议传输）；NOT RUN（有标签数据比对） | [查看](devices/unit-uhf-rfid/2026-07-22-issue-16-readcard-start-address/) |

## 目录结构

```text
devices-test-log/
|-- devices/
|   `-- <device-slug>/
|       |-- README.md
|       `-- <YYYY-MM-DD>-<topic>/
|           |-- README.md
|           |-- report.md
|           |-- run.json
|           |-- evidence/
|           |-- patches/
|           `-- hil/
|-- docs/
|   `-- record-format.md
`-- templates/
    `-- hardware-test/
        |-- README.md
        `-- run.json
```

目录名使用小写 ASCII 和连字符。相同设备的多轮回归测试放在同一设备目录下，每次运行使用独立的日期和主题目录。

## 结果等级

- `PASS`: 目标行为在声明的验收层级通过，并有原始证据。
- `PARTIAL`: 部分核心步骤通过，但仍缺少会影响最终结论的验证。
- `FAIL`: 已执行测试，目标行为未通过。
- `NOT RUN`: 因硬件、标签、环境或权限条件不足而没有执行。

一个测试记录可以同时包含多个层级结果。例如驱动命令帧可以是 `PASS`，有标签的数据内容比对可以是 `NOT RUN`。不要用单一总状态掩盖未覆盖项。

## 新增记录

1. 复制 `templates/hardware-test/` 到对应的设备和日期目录。
2. 在 `run.json` 中记录源码 commit、patch、硬件、固件哈希、任务 ID 和各项结果。
3. 将未经改写的串口输出或测试中心事件摘要放入 `evidence/`。
4. 将可直接应用的修复放入 `patches/`，并运行 `git apply --check`。
5. 更新设备目录 README 和本文件的测试记录表。
6. 提交前检查 UTF-8、敏感信息、相对链接和工作区状态。

详细规范见 [docs/record-format.md](docs/record-format.md)。

## 安全约束

- 不提交 token、API key、Wi-Fi 密码、私钥、客户日志或设备唯一敏感信息。
- 默认不提交固件二进制；记录 artifact ID、SHA256、大小和可重建工程即可。
- 涉及擦除、写标签、量产、OTA 或其他不可逆操作时，报告必须明确风险和授权范围。
- 原始日志可裁剪无关噪声，但不能改写能够影响结论的字节、错误码或时间顺序。
