# M5CoreS3 测试记录

- 产品：M5Stack CoreS3
- SoC：ESP32-S3
- 主 USB：GPIO19 / GPIO20，支持 USB OTG
- 摄像头：GC0308
- 产品库：https://github.com/m5stack/M5CoreS3

## 记录

| 日期 | 主题 | 结果 | 链接 |
| --- | --- | --- | --- |
| 2026-07-22 | PR #69: GC0308 自定义 `camera_config_t` | PASS（功能）；REVIEW（配置指针生命周期） | [查看](2026-07-22-camera-config-pr-69/) |
| 2026-07-22 | Issue #71: Arduino USB OTG Host | PARTIAL（安全门通过；外设枚举未执行） | [查看](2026-07-22-usb-host-issue-71/) |
