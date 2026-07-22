# HIL 复现工程

该工程运行在 M5Stack Tough 上，通过多个 Grove UART 候选管脚探测 Unit UHF-RFID，然后验证非零 `sa` 命令帧。

已修复的 `M5Unit-UHF-RFID` 0.0.4 源码保存在 `lib/M5Unit-UHF-RFID/`，因此测试不会误用未修复的上游版本。M5Unified 和 M5GFX 固定为本次实测版本。

## 构建

```powershell
platformio run
```

生成 ESP32 合并镜像时使用：

```text
0x1000  bootloader.bin
0x8000  partitions.bin
0xE000  boot_app0.bin
0x10000 firmware.bin
```

本次测试使用合并镜像地址 `0x0`，最终 SHA256 记录在上级 `run.json`。

## 验收标记

- `UHF_ISSUE16_TRANSPORT_PASS`: 模块版本读取成功，非零 SA 命令得到有效模块响应。
- `UHF_ISSUE16_DATA_PASS`: 存在标签时，EPC bank 的 word 地址 `2/4/6` 分段读取与轮询 EPC 一致。
