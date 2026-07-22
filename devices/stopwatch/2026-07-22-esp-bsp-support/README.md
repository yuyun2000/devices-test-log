# M5Stack StopWatch esp-bsp support

- Date: `2026-07-22` to `2026-07-23` (Asia/Shanghai)
- Result: `PASS` for build, flash, transport, and automated device behavior; `PARTIAL` for unobserved physical output
- Source: `espressif/esp-bsp@74e3cd238fdd81cda4fd03184c32f6776c41b7cd`
- Hardware: M5Stack StopWatch, ESP32-S3

## Conclusion

The patch adds a native `m5stack_stopwatch` BSP with display, touch, power/control, audio, buttons, BMI270, RTC bus access, vibration, BSP selector integration, and compatible examples. The final firmware passed twice on the real device, including once after an independent hardware reset. Both runs ended with `STOPWATCH_HIL_DONE status=PASS failures=0`.

## Key Evidence

- ESP-IDF `v5.5.4` build: application `0x52520` bytes, 68% app partition free.
- Merged image: 402720 bytes, SHA256 `52741eed61e3e4a96340952564014dba1bb5a8e5aaa45fb80d515de78885659b`.
- Final flash task: `task-d93c5aacd86c481e`, `PASS`.
- Reset-run session: `ser-d51e53e144acc673`, 4102 bytes, 10 chunks, no drops, not truncated.
- Reset-run audio capture: peak `31424`, mean magnitude `2309`.
- All expected I2C devices responded: M5PM1, M5IOE1, CST820B, ES8311, RTC, and BMI270.

## Files

- [report.md](report.md): root cause, implementation, build, and hardware-test details.
- [run.json](run.json): machine-readable test record.
- [evidence/serial-reset-boot10-redacted.log](evidence/serial-reset-boot10-redacted.log): normalized reset-run serial transcript.
- [evidence/flash-task-summary.json](evidence/flash-task-summary.json): redacted flash lifecycle.
- [evidence/serial-sessions-summary.json](evidence/serial-sessions-summary.json): serial integrity and hashes.
- [evidence/build-summary.json](evidence/build-summary.json): build and image metadata.
- [patches/0001-add-m5stack-stopwatch-bsp.patch](patches/0001-add-m5stack-stopwatch-bsp.patch): patch against the declared base commit.
- [hil/](hil/): rebuildable ESP-IDF HIL project.

## Partial Physical Evidence

The fixture has no camera and no operator observation was used. Display initialization and framebuffer writes, touch-controller reads, button idle levels, speaker write calls, microphone samples, and vibration commands passed, but display appearance, touch coordinates, pressed-button events, audible sound, and felt vibration remain `PARTIAL`.
