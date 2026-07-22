# Issue #71 Arduino USB Host HIL

构建：

```powershell
pio run -e cores3_usb_host
```

测试逻辑：

- 上游 VBUS 或 Hardware CDC 已连接时，强制关闭 CoreS3 USB output；
- 两者均不存在时，屏幕允许点击启动 USB Host；
- 枚举成功后把 descriptor 摘要保存到 NVS；
- 再次连接调试 USB 时通过 `ISSUE71_REPLAY_PASS` 回放结果。

安全要求：不要在 CoreS3 主 USB 仍连接 PC、测试中心或其他 Host 时启动 VBUS。完整枚举需要人工换线，本工程不会自动绕过安全门。
