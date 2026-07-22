# M5Unit-UHF-RFID Issue #16 问题报告

## 结论

- Issue: `readCard() ignores parameter 'sa' (start address)`
- 状态: 已修复并完成 Tough + Unit UHF-RFID 实机协议传输验证。
- 根因: `readCard()` 复制 `READ_STORAGE_CMD` 后设置了 Access Password、MemBank 和 DL，但没有把参数 `sa` 写入命令帧的 `buffer[10]`、`buffer[11]`，因此 SA 始终保留模板值 `0x0000`。
- 修复: 按官方协议把 `sa` 以大端顺序写入 `SA(MSB)`、`SA(LSB)`；公共 API、帧长度、响应解析均未改变。

## 证据

### 代码证据

`writeCard()` 已正确执行：

```cpp
buffer[10] = (sa >> 8) & 0xff;
buffer[11] = sa & 0xff;
```

修复前的 `readCard()` 在 `buffer[9] = membank` 后直接设置 `buffer[12:13]` 的 DL，遗漏了相同的 SA 编码。

### 协议证据

M5Stack 官方《Unit UHF-RFID 常用控制指令》2.8.1 节规定：

- SA 为 16 位标签数据区地址偏移；
- SA 和 DL 的单位均为 Word，即 2 Byte / 16 Bit；
- 命令帧字段顺序为 `MemBank, SA(MSB), SA(LSB), DL(MSB), DL(LSB)`。

官方资料地址：

- https://docs.m5stack.com/zh_CN/unit/uhf_rfid
- https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/842/Unit-RFID-UHF-Protocol-CN.pdf

## 修复内容

文件：`src/UNIT_UHF_RFID.cpp`

```diff
     buffer[9]    = membank;
+    buffer[10]   = (sa >> 8) & 0xff;
+    buffer[11]   = sa & 0xff;
     uint8_t word = size / 2;
```

该改动与 `writeCard()` 的现有实现及官方协议一致，改动范围仅两行。

## 验证结果

### 本地构建

- PlatformIO Core: 6.1.19
- Platform: Espressif 32 7.0.1
- Framework: Arduino-ESP32 2.0.17
- Board: `m5stack-core2`（运行时 M5Unified 识别为 Tough，board=8）
- M5Unified: 0.2.10
- M5GFX: 0.2.19
- M5Unit-UHF-RFID: 0.0.4 + 本修复
- 结果: `[SUCCESS]`
- RAM: 38196 bytes
- Flash: 457117 bytes

静态检查：`git diff --check` 通过。

### 实机测试

- 测试环境: M5Stack device test center（内网地址和测试槽标识未公开）
- 主机: M5Stack Tough / ESP32
- 外设: Unit UHF-RFID
- 实际连接: Tough PORT.A，RX=GPIO33，TX=GPIO32
- 模块版本: `M100 26dBm V1.0`
- 固件 artifact: `fw-4229ae67dad4460c`
- 固件 SHA256: `08ef88bf2138db80926e3b1ebb5fdabeff47503fc91c4bb6cacb8b8d0c483e53`
- 测试 task: `task-8868f076d0b54319`
- 测试终态: `passed`
- 串口 session: `ser-a4d1f7f162ba21ec`

使用 `readCard(data, 4, 0x01, 0x1234)` 得到的实机发送帧：

```text
bb003900090000000001123400028b7e
```

字段拆分：

```text
BB 00 39 00 09 00000000 01 12 34 00 02 8B 7E
                           ^^ ^^
                           SA=0x1234
```

UHF 模块返回：

```text
bb01ff0001090a7e
```

`0x09` 是官方协议定义的“标签不在场区或指定 EPC 不匹配”。这证明命令已通过真实 UART 发送到 UHF 模块并获得协议响应；测试中心成功匹配 `UHF_ISSUE16_TRANSPORT_PASS`。

## 验证边界

现场射频区域没有检测到标签，`pollingOnce()` 返回 0，因此没有执行 EPC bank 地址 `2/4/6` 的分段数据一致性验证。当前实机结果覆盖：Tough 启动、UHF UART 连通、模块版本读取、非零 SA 帧编码、校验和以及模块协议响应；不包含有标签条件下的返回数据内容比对。

补齐数据层验收只需在 Unit 天线范围内放入 EPCglobal UHF Class 1 Gen 2 / ISO 18000-6C 标签后重跑同一诊断固件。固件会读取 EPC bank 的 word 地址 `2/4/6`，分别与轮询 EPC 的三段 4 字节数据比较，并输出 `UHF_ISSUE16_DATA_PASS`。

## 回滚

删除 `readCard()` 中新增的两行 SA 赋值即可回滚。回滚后 `sa` 会再次被忽略，不建议回滚。
