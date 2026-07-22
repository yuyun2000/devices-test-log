# M5CoreS3 Issue #71 Arduino USB OTG Host 报告

## 结论

CoreS3 的 Arduino USB Host 路径可构建，方法是直接调用 Arduino-ESP32 随附的 ESP-IDF USB Host API，而不是等待 M5CoreS3 库提供独立 Host 封装：

1. 使用 `M5.Power.setUsbOutput(true)` 打开主 USB VBUS；
2. `usb_host_install()` 启动 Host library daemon；
3. `usb_host_client_register()` 注册异步 client；
4. 在 client event 中打开新设备并读取 device/config descriptor；
5. 使用 `usb_host_lib_handle_events()` 和 `usb_host_client_handle_events()` 持续处理事件。

本次实机完成了构建、烧录、启动以及上游 USB 安全门验证。连接测试中心时，CoreS3 实测 VBUS 为 `5004 mV`、Hardware CDC 连接状态为真、USB 输出状态为 `0`。这证明示例不会在已有上游 Host 时开启 CoreS3 VBUS。

完整 USB 外设枚举没有执行。CoreS3 主 USB 当时被测试中心占用，远程环境不能安全完成“拔掉上游线 -> 接 OTG 外设 -> 触屏启动 Host”的物理换线；在同一链路上强行开 VBUS会引入双主机和反向供电风险。

## 问题来源

- Issue：https://github.com/m5stack/M5CoreS3/issues/71
- 标题：`[Request] Core S3 USB OTG Host Examples`
- 当前状态：open
- M5CoreS3 主线：`adce2225e7fd8d012b148f46c990f8cc7b8ecc26`

Issue 当前只有示例请求和作者后续追问，没有现成官方 Arduino Host 示例。

## 硬件与 API 证据

- CoreS3 使用 ESP32-S3，主 USB 数据线连接芯片原生 USB D-/D+；
- M5CoreS3 README 明确产品支持 OTG 与 CDC；
- M5Unified 为 CoreS3 提供 `setUsbOutput()` / `getUsbOutput()`；
- Arduino-ESP32 `2.0.17` 中存在并可链接 `usb/usb_host.h` 的 Host API。

HIL 工程不硬编码外部 GPIO。VBUS 通过 M5Unified 的板级电源接口控制，descriptor 枚举使用 ESP-IDF 官方结构体和事件模型。

## 排查过程

### Top 假设

1. Arduino 框架没有暴露 USB Host API；
2. CoreS3 的 VBUS 没有软件控制路径；
3. Host API 可用，但当前测试中心连接会与 DUT Host 模式冲突。

构建排除了假设 1，M5Unified API 和实机 `usb_output=0` 读回排除了假设 2。假设 3 成立，因此测试拆成“上游连接保护”和“独立外设枚举”两个层级。

### 首轮安全测试

首轮 task `task-1aed36be2fe444a8` 烧录成功，但串口 session 为 0 字节，最终 `serial_expect_timeout`。该结果无法判断程序处于 VBUS 分支还是已经切换 USB 角色，属于测试可观测性不足，不作为 DUT 失败证据。

最小修正包括：

- 在 `M5.begin()` 前后增加启动标记；
- 初始化后先强制 `setUsbOutput(false)`；
- 把安全门从“仅 PMIC VBUS”加强为“PMIC VBUS 或已连接 Hardware CDC 任一成立”；
- 延长启动等待，确保测试中心串口监控建立。

修正后 task `task-aaa22e046f1d4238` 通过。

## 构建与固件

- PlatformIO Core：`6.1.18`
- Platform：Espressif 32 `7.0.1`
- Framework：Arduino-ESP32 `2.0.17`
- Board：`m5stack-cores3`
- M5GFX：`0.2.22`
- M5Unified：`0.2.17`
- RAM：22304 / 327680 bytes（6.8%）
- Flash：487953 / 6553600 bytes（7.4%）

合并镜像使用 `DIO / 80 MHz / 16 MB`，artifact 信息：

- ID：`fw-ccfbb6db17ef483e`
- SHA256：`ed5965c986f101bc12225afa15ec498572b78aced1cf8246e6cb1804e3482442`
- 大小：553856 bytes

## 实机结果

- task：`task-aaa22e046f1d4238`
- task 终态：`passed`
- session：`ser-e08d571c73d06ce3`
- session：103 bytes / 2 chunks / dropped 0 / truncated false
- 执行时间：`2026-07-22T10:59:57.022Z` 至 `2026-07-22T11:00:52.918Z`

关键串口：

```text
ISSUE71_BOOT
ISSUE71_BOARD_READY
ISSUE71_SAFE_WAIT upstream_vbus_mv=5004 upstream_cdc=1 usb_output=0
```

task 的 `passed` 仅说明烧录计划与串口断言完成；安全结论来自 `upstream_vbus_mv`、`upstream_cdc` 和 `usb_output` 三个应用层字段。

## 独立枚举补测步骤

1. 让 CoreS3 使用电池或独立底座供电；
2. 拔掉当前连接测试中心的主 USB 线；
3. 将 OTG 外设接到 CoreS3 主 USB；
4. 屏幕显示 `Safe to start host` 后点击一次；
5. 等待屏幕显示 `ENUMERATION PASS`；
6. 重新接回调试 USB，串口应通过 NVS 输出 `ISSUE71_REPLAY_PASS`，包含 VID、PID、class、interface 数和 speed。

若没有 `ISSUE71_REPLAY_PASS`，应记录屏幕错误、外设供电电流和 USB Host 返回码，再区分电源、PHY、descriptor 或 class driver 问题。

## 验收边界

- `build`：PASS
- `flash`：PASS
- `transport`：PARTIAL，仅验证上游 CDC 与 VBUS 安全判定；Host 总线枚举未运行
- `device_response`：NOT RUN，未接入 OTG 外设
- `data_behavior`：PASS，连接上游时 VBUS 输出保持关闭
- `visual_or_physical`：NOT RUN，测试槽相机不可用且未进行人工换线

整体状态为 `PARTIAL`，不能据此宣称某一具体 USB class 外设已在 CoreS3 Arduino Host 下工作。

## 回滚

本记录没有修改上游 M5CoreS3 仓库。删除 HIL 固件即可回滚；测试结束后不要在连接另一个 USB Host 时手动开启 CoreS3 USB output。
