# M5CoreS3 PR #69 相机配置测试

- 日期：`2026-07-22`
- 功能结果：`PASS`
- Review 结果：配置指针生命周期需在合并前修正或明确约束
- 问题来源：https://github.com/m5stack/M5CoreS3/pull/69
- PR commit：`c728f1951888aafe57a225069fe71241f6532ff4`
- 主线对照：`adce2225e7fd8d012b148f46c990f8cc7b8ecc26`
- 硬件：M5Stack CoreS3 内置 GC0308

## 结论

PR 默认调用与自定义 `camera_config_t` 均完成实机采集。默认路径连续获得 4 帧 `320x240 RGB565`，自定义路径连续获得 4 帧 `640x480 GRAYSCALE`；帧长度、格式、配置指针和非零图像哈希均通过。

PR 当前把调用方配置地址长期保存在公开成员 `config` 中。测试使用全局配置，因此没有触发生命周期问题；如果调用方传入局部变量，`begin()` 返回后该成员可能悬空。建议库内部保存配置副本，或在 API 文档中明确调用方必须保证配置对象与 `GC0308` 同寿命。

## 关键证据

- 主线基线 task：`task-188debb33dbc42cd`，`passed`
- PR 默认 task：`task-13e16b17a6954898`，`passed`
- PR 自定义 task：`task-e929d7d4e92e4d03`，`passed`
- 默认帧：`320x240`、格式 `0`、长度 `153600`
- 自定义帧：`640x480`、格式 `3`、长度 `307200`

## 文件

- [完整报告](report.md)
- [结构化运行记录](run.json)
- [构建摘要](evidence/build-summary.txt)
- [主线基线串口](evidence/main-baseline-serial.log)
- [PR 默认串口](evidence/pr69-default-serial.log)
- [PR 自定义串口](evidence/pr69-custom-serial.log)
- [测试中心脱敏摘要](evidence/test-center-summary.json)
- [HIL 工程](hil/)
