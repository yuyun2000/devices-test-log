# PR #69 HIL 工程

使用 PlatformIO 构建：

```powershell
pio run -e main_baseline -e pr69_default -e pr69_custom
```

三个环境共用 `src/main.cpp`：

- `main_baseline`：当前 M5CoreS3 主线；
- `pr69_default`：PR head，无参 `Camera.begin()`；
- `pr69_custom`：PR head，VGA 灰度双缓冲自定义配置。

向 offset `0x0` 烧录单文件时，合并镜像必须使用 CoreS3 的 `DIO` 烧录头。不要提交 `.pio/`、固件二进制、ELF 或 map 文件。
