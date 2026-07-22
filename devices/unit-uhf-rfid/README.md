# Unit UHF-RFID 测试记录

- 产品：M5Stack Unit UHF-RFID
- SKU：U107
- 模块：JRD-4035
- 接口：UART 115200 bps
- 官方文档：https://docs.m5stack.com/zh_CN/unit/uhf_rfid
- 驱动仓库：https://github.com/m5stack/M5Unit-UHF-RFID

## 记录

| 日期 | 主题 | 结果 | 链接 |
| --- | --- | --- | --- |
| 2026-07-22 | Issue #16: `readCard()` 忽略起始地址 `sa` | PASS（真实标签地址行为）；PARTIAL（单轮三地址同时匹配） | [查看](2026-07-22-issue-16-readcard-start-address/) |
