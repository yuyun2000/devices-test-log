# M5CoreS3 Issue #71 USB OTG Host 测试

- 日期：`2026-07-22`
- 结果：`PARTIAL`
- 问题来源：https://github.com/m5stack/M5CoreS3/issues/71
- 源码基线：`adce2225e7fd8d012b148f46c990f8cc7b8ecc26`
- 硬件：M5Stack CoreS3 / ESP32-S3 主 USB

## 结论

Arduino-ESP32 `2.0.17` 可以直接使用 ESP-IDF 的 `usb/usb_host.h` 在 CoreS3 上实现 USB Host；CoreS3 主 USB 的 VBUS 输出由 `M5.Power.setUsbOutput(true)` 控制。

本次实机已验证连接上游 USB 时的安全门：PMIC 读取到 `5004 mV`、Hardware CDC 已连接，固件保持 `usb_output=0`，不会形成双主机或反向供电。完整外设枚举需要拔掉测试中心 USB、接入 OTG 外设并触屏启动 Host，当前远程拓扑无法换线，因此枚举结果为 `NOT RUN`。

## 关键证据

- 构建：PlatformIO / Arduino-ESP32 `2.0.17`，`SUCCESS`
- 固件 artifact：`fw-ccfbb6db17ef483e`
- 实机 task：`task-aaa22e046f1d4238`，`passed`
- 串口 session：`ser-e08d571c73d06ce3`
- 应用标记：`ISSUE71_SAFE_WAIT upstream_vbus_mv=5004 upstream_cdc=1 usb_output=0`

## 文件

- [完整报告](report.md)
- [结构化运行记录](run.json)
- [安全门串口证据](evidence/safe-wait-serial.log)
- [构建摘要](evidence/build-summary.txt)
- [测试中心脱敏摘要](evidence/test-center-summary.json)
- [可重建 Arduino HIL 工程](hil/)

## 未覆盖项

- USB 外设枚举、VID/PID/descriptor 读取：`NOT RUN`
- Low-Speed / Full-Speed 外设兼容性：`NOT RUN`
- 屏幕枚举结果照片：`NOT RUN`
