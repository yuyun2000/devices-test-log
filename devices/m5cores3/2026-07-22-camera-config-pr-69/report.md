# M5CoreS3 PR #69 问题与实机测试报告

## 结论

PR #69 在 M5CoreS3 实机上的目标行为通过：

- `CoreS3.Camera.begin()` 保持原默认行为，连续采集 4 帧 `320x240 RGB565`；
- `CoreS3.Camera.begin(&custom_config)` 使用 `640x480 GRAYSCALE` 配置成功，连续采集 4 帧；
- 自定义路径中 `CoreS3.Camera.config == &custom_config` 为真；
- 三组任务均成功烧录、重启并命中应用层 `PASS` 标记。

合并前仍建议处理一项 API 所有权风险：PR 保存调用方传入的 `camera_config_t*`，但没有复制配置或声明生命周期要求。若调用方传入栈上局部配置，公开成员 `Camera.config` 会在局部变量离开作用域后悬空。该风险没有影响本次全局配置测试，但与 PR 描述的“used config struct can always be accessed”不一致。

## 来源与改动

- PR：https://github.com/m5stack/M5CoreS3/pull/69
- PR head：`c728f1951888aafe57a225069fe71241f6532ff4`
- PR 当前 base：`bfeebde61965e1a7b6df1b7648d0b761e2f88642`
- 当前主线对照：`adce2225e7fd8d012b148f46c990f8cc7b8ecc26`

PR 新增 `GC0308` 构造函数，将默认静态配置地址赋给 `config`，并把 `begin()` 改为：

```cpp
bool begin(camera_config_t* _config = nullptr);
```

传入非空指针时，PR 直接执行 `config = _config`，然后把该指针交给 `esp_camera_init()`。

## 排查过程

### Top 假设

1. 默认 API 兼容性被破坏，`begin()` 无参调用无法初始化；
2. GC0308 或 PSRAM 无法接受 VGA 灰度双帧缓冲配置；
3. 自定义配置指针可用，但所有权和生命周期不受库控制。

实机排除了前两项：默认与自定义路径均稳定采集 4 帧。第三项由代码路径直接确认，本次 HIL 刻意使用全局 `custom_config` 规避悬空，不代表局部配置调用安全。

### 初始启动失败与根因

首次 task `task-fd3ca5be893b435e` 使用的合并镜像头是 `QIO`。设备只输出 ROM loader 第一段加载信息，随后 `TG0WDT_SYS_RST`，应用没有运行。

Arduino-ESP32 的 CoreS3 板定义在烧录阶段使用 `DIO`。PlatformIO 的上传流程会把 `qio` 规范化为 `dio`，但把多个分区预先合并为 offset `0x0` 的单文件时，测试中心不会再次修补镜像头。重新使用以下参数合并后，镜像头第三字节从 `0x00` 变为 `0x02`，实机立即正常启动：

```text
--flash_mode dio --flash_freq 80m --flash_size 16MB
```

这属于 HIL 固件打包问题，不是 PR #69 或相机故障。失败任务和修正后的任务均保留在脱敏摘要中。

## 构建环境

- PlatformIO Core：`6.1.18`
- Platform：Espressif 32 `7.0.1`
- Framework：Arduino-ESP32 `2.0.17`
- Board：`m5stack-cores3`
- M5GFX：`0.2.22`
- M5Unified：`0.2.17`
- M5CoreS3：主线 `adce222...` 或 PR `c728f19...`

| 环境 | RAM | Flash | 结果 |
| --- | ---: | ---: | --- |
| `main_baseline` | 26592 bytes | 501849 bytes | PASS |
| `pr69_default` | 26592 bytes | 501881 bytes | PASS |
| `pr69_custom` | 26696 bytes | 501937 bytes | PASS |

## 实机结果

测试中心内网地址与测试槽标识未公开。每个 task 都执行 offset `0x0` 的合并镜像烧录、重启、串口监控和 30 秒运行窗口。

| 变体 | Artifact | Task / Session | 结果 |
| --- | --- | --- | --- |
| 主线基线 | `fw-bf1fe452b34c450d` | `task-188debb33dbc42cd` / `ser-30236270baf0830e` | 4 帧 `320x240 RGB565`，PASS |
| PR 默认 | `fw-790b400cafd14cf0` | `task-13e16b17a6954898` / `ser-cfb7e208f9ee6070` | 4 帧 `320x240 RGB565`，PASS |
| PR 自定义 | `fw-430bc7b46b2145db` | `task-e929d7d4e92e4d03` / `ser-cd9a1e8f47a786ab` | 4 帧 `640x480 GRAYSCALE`，PASS |

自定义路径的每帧长度为 `640 * 480 = 307200` 字节，4 帧 FNV-1a 分别不同，证明不是固定空缓冲。完整值见 [串口证据](evidence/pr69-custom-serial.log)。

## Review 建议

建议让 `GC0308` 拥有配置存储，再把公开指针指向内部副本，例如构造时复制默认配置，`begin(const camera_config_t*)` 收到自定义配置时再按值复制。这样可以同时避免：

- 调用方局部配置离开作用域后的悬空指针；
- 多个 `GC0308` 实例共享并修改同一个静态默认配置；
- `begin(nullptr)` 在曾经传入自定义指针后继续保留外部指针的隐式状态。

如果仍采用借用指针，应把参数改为 `const camera_config_t*`（若底层允许），并明确文档化生命周期约束。

## 验收边界

- `build`：PASS
- `flash`：PASS
- `transport`：PASS，串口日志完整且 dropped 为 0
- `device_response`：PASS，GC0308 返回符合配置的帧
- `data_behavior`：PASS，帧尺寸、格式、长度、非零且变化的哈希均符合预期
- `visual_or_physical`：PARTIAL，设备显示结果但测试槽相机不可用，未保存屏幕照片

未测试长时间连续采集、重复 deinit/reinit、多个 `GC0308` 实例，以及调用方局部配置离开作用域后的行为。

## 回滚

本记录没有修改上游仓库。若回滚 PR，恢复 `GC0308::begin()` 无参实现即可；自定义相机配置能力会同时消失。
