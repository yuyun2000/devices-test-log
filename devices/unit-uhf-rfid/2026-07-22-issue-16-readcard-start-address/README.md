# Issue #16: readCard() start address

## 结论

已修复 `Unit_UHF_RFID::readCard()` 忽略 `sa` 参数的问题。修复将 16 位起始地址按大端顺序写入 UHF 命令帧的 `SA(MSB)` 和 `SA(LSB)`，与官方协议及现有 `writeCard()` 实现一致。

Tough + Unit UHF-RFID 实机任务终态为 `passed`。使用 `sa=0x1234` 时，串口记录到真实发送帧：

```text
bb003900090000000001123400028b7e
```

首次测试中，模块返回有效的无标签错误响应 `bb01ff0001090a7e`，证明非零 SA 命令已通过真实 UART 到达模块。随后放入真实标签复测，3 次检测到标签；EPC bank 的 word 地址 `2/4/6` 分别至少有一次读取数据与轮询 EPC 对应片段一致，真实标签地址行为标为 `PASS`。

射频链路存在瞬时读失败，没有任意一轮同时完成三个地址匹配，因此严格的单轮 `UHF_ISSUE16_DATA_PASS` 验收标为 `PARTIAL`。这不影响 `sa` 已按地址生效的结论。

## 关键记录

- 上游 issue：https://github.com/m5stack/M5Unit-UHF-RFID/issues/16
- 上游 base commit：`6f886a5`
- 修复分支：`codex/fix-readcard-start-address`
- 实机：M5Stack Tough + Unit UHF-RFID
- 实际 UART：PORT.A，RX=GPIO33，TX=GPIO32
- 模块版本：`M100 26dBm V1.0`
- 首次测试 task：`task-8868f076d0b54319`
- 有标签复测 task：`task-e8491753df0d408b`
- 有标签复测 session：`ser-3acc9c2d76332e99`
- 固件 artifact：`fw-4229ae67dad4460c`
- 固件 SHA256：`08ef88bf2138db80926e3b1ebb5fdabeff47503fc91c4bb6cacb8b8d0c483e53`

## 文件

- [完整问题报告](report.md)
- [最小修复 patch](patches/0001-fix-readcard-start-address.patch)
- [结构化运行记录](run.json)
- [实机串口日志](evidence/serial.log)
- [有标签复测证据（已脱敏）](evidence/tag-read-summary.log)
- [HIL 复现工程](hil/)
