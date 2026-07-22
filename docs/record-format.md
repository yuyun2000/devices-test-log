# 硬件测试记录规范

## 目标

每条记录应让未参与现场测试的人能够回答四个问题：测试了什么、使用了什么环境、观察到了什么、还有什么没有验证。

## 必备内容

### README.md

提供可快速浏览的摘要：问题、结论、关键证据、目录导航和剩余限制。

### report.md

保存完整问题报告，至少包含：

- 问题来源和复现现象；
- Top 假设与实际根因；
- 修复范围和兼容性；
- 构建环境及命令；
- 实机连接、固件、任务和日志证据；
- 未覆盖项、风险和回滚方式。

### run.json

提供机器可读的单次运行清单。字段可扩展，但不要删除 `schema_version`、`source`、`hardware`、`firmware`、`execution` 和 `checks`。

### evidence/

保存能够独立支撑结论的原始证据，例如串口日志、协议帧、测试中心事件摘要、测量数据或照片。日志应标注来源；图片应保留原始文件或 SHA256。

### patches/

patch 文件名使用顺序号，例如 `0001-fix-uart-timeout.patch`。记录 base commit，并验证：

```powershell
git apply --check .\patches\0001-fix-uart-timeout.patch
```

### hil/

保存硬件在环测试源码和构建说明。优先记录可重建工程，不提交大型二进制。

## 验收分层

硬件问题通常需要分别记录：

1. `build`: 源码能否构建，镜像是否合法。
2. `flash`: 固件是否成功烧录并重启。
3. `transport`: UART/I2C/SPI/USB 等链路是否真实收发。
4. `device_response`: 外设是否返回协议级有效响应。
5. `data_behavior`: 返回数据是否随地址、输入或状态正确变化。
6. `visual_or_physical`: 屏幕、LED、继电器、传感器或机械行为是否符合预期。

每层使用 `PASS`、`PARTIAL`、`FAIL` 或 `NOT RUN`。只有运行过的检查才可以标为 `PASS`。

## 可追溯性

- 外部 issue、数据手册和源码使用永久链接或明确版本。
- 记录 Git commit、artifact ID、task ID、session ID 和 SHA256。
- 时间使用 ISO 8601，并写明时区或使用 UTC。
- 测试基础设施故障与被测对象故障分开描述。
