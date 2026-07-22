# M5Stack StopWatch esp-bsp support report

## Goal And Scope

Add native M5Stack StopWatch support to `espressif/esp-bsp` and prove it on a real device. The change must remain a focused BSP addition, preserve existing BSP APIs and examples, and ship as a patch plus a reproducible HIL project. No upstream branch or pull request is created by this delivery.

Source baseline:

- Repository: <https://github.com/espressif/esp-bsp>
- Base commit: `74e3cd238fdd81cda4fd03184c32f6776c41b7cd`
- Reference implementation: <https://github.com/m5stack/M5StopWatch-UserDemo/tree/6b4aa125288b6fe9dca661f10159f6e1e5ee785c>

## Implemented Support

The patch adds `bsp/m5stack_stopwatch` with:

- M5PM1 power initialization, voltage reads, and combinable power-source flags;
- M5IOE1 control of display, touch, audio, speaker, USB/UART routing, and vibration;
- 466 x 466 CO5300 QSPI display with the required six-pixel X offset and controller brightness;
- CST820B touch through its CST816S-compatible address/register interface;
- ES8311 full-duplex speaker and microphone through one shared codec handle;
- BMI270 through the sensor-hub API;
- two active-low buttons, RTC/shared-I2C access, BSP selector entries, root index entry, and display example configurations.

The generic `audio` and `display_audio_photo` example matrices are intentionally not enabled because their single-direction/separate-codec assumptions do not match this board's one bidirectional ES8311 handle. The low-level display and display-sensor examples are included.

## Diagnosis And Corrections

### 1. ES8311 control address

The physical probe uses the normal 7-bit address `0x18`, while `esp_codec_dev` v1.5.11 expects the 8-bit ES8311 constant `ES8311_CODEC_DEFAULT_ADDR` (`0x30`) and shifts it internally. Supplying `0x18` to that API addressed `0x0C`. The BSP now passes the codec constant while keeping the public probe address at `0x18`.

### 2. Audio startup and PA control

The codec is initialized before enabling the external speaker path. The codec driver's PA pin is `GPIO_NUM_NC`; the BSP controls both board-level PA signals, then waits 10 ms for stabilization. This matches the official UserDemo ordering.

The reset-run log contains one `Fail to write to dev 30` line before codec startup completes. This is the known first-write behavior documented in `esp_codec_dev` v1.5.11: the ES8311 driver immediately repeats the noise-immunity register write. The retry succeeded, followed by codec open, write, microphone capture, and close passes on two starts, so this line is retained as evidence rather than hidden or treated as a final failure.

### 3. M5PM1 power-source register

Register `0x04` is a three-bit mask, not a single exclusive enumeration. Real hardware produced `0x07` during an earlier diagnostic run. The public type now defines USB, 5V input/output, and battery as `BIT(0..2)` values that can be combined. The final reset run reported source `1`, and the earlier `0x07` observation is also valid under the corrected API.

### 4. Voltage observation boundary

The HIL performs up to ten bounded PMIC samples and distinguishes I2C/register-read success from a physical voltage-range observation. The final reset run read input `4960 mV` and battery `1378 mV`; the battery register read passed but the physical value was explicitly recorded as `OUT_OF_RANGE`. An earlier powered run observed `4216 mV`. Therefore register access is `PASS`, while battery-voltage stability/accuracy is `PARTIAL` pending instrumented measurement.

## Build

Environment and resolved versions:

- Windows PowerShell
- ESP-IDF `v5.5.4`
- target `esp32s3`, QIO, 16 MB flash, 240 MHz
- `esp_codec_dev` 1.5.11
- `esp_lcd_co5300` 2.1.0
- `esp_lcd_touch_cst816s` 1.1.1~1
- `button` 4.2.0
- `sensor_hub` 0.1.5, BMI270 1.1.0
- LVGL 9.5.0, `esp_lvgl_port` 2.8.0~1

Rebuild commands from the archived HIL project:

```powershell
$env:ESP_BSP_PATH = 'C:\path\to\esp-bsp'
idf.py -B build build
idf.py -B build merge-bin -o stopwatch-bsp-hil-merged.bin
```

Result:

- application: `0x52520` bytes;
- app partition free: 68%;
- flashed merged image: `0x62520` / 402720 bytes, SHA256 `52741eed61e3e4a96340952564014dba1bb5a8e5aaa45fb80d515de78885659b`;
- clean delivery rebuild: `0x62520` / 402720 bytes, SHA256 `1f4f838bb086aa931ad0744e56409878e1a68c580a881e015fe502a8dfe80888`.

The two merged hashes differ because this HIL configuration has `CONFIG_APP_REPRODUCIBLE_BUILD` disabled and `CONFIG_APP_COMPILE_TIME_DATE` enabled. The clean rebuild resolved the same dependency versions and produced the same application and merged sizes; the report does not claim byte-reproducible output.

The repository's standard `examples/display` project was also built with `sdkconfig.bsp.m5stack_stopwatch`. It resolved `bsp_selector` 1.0.2 and `m5stack_stopwatch` 1.0.0, then produced a `0x9f860` application with 38% of the app partition free. This validates the selector and example integration separately from the direct HIL component path.

The firmware binary is intentionally not committed.

## Real-Device Execution

Final artifact and flash:

- Artifact: `fw-154958f6e31c489d`
- Task: `task-d93c5aacd86c481e`
- Task type: `flash_only`
- Task result: `PASS`
- Flash interval: `2026-07-22T23:20:05.986Z` to `2026-07-22T23:20:17.384Z`

Application proof was collected separately from the task result:

| Run | Session | Result | Serial integrity | Audio |
| --- | --- | --- | --- | --- |
| Post-flash | `ser-46208c88355249ee` | `PASS`, failures=0 | 1231 bytes, 4 chunks, dropped=0, not truncated | peak 30303, mean 2278 |
| Independent reset | `ser-d51e53e144acc673` | `PASS`, failures=0 | 4102 bytes, 10 chunks, dropped=0, not truncated | peak 31424, mean 2309 |

The reset run provides the complete automated coverage:

- control initialization and shared I2C bus: `PASS`;
- M5PM1 `0x6E`, M5IOE1 `0x4F`, touch `0x15`, ES8311 `0x18`, RTC `0x32`, BMI270 `0x68`: `PASS`;
- button handles and idle levels: `PASS`;
- CO5300 initialization, full-screen RGB writes, brightness call: `PASS`;
- CST820B initialization and no-touch read: `PASS`;
- BMI270 accelerometer and gyroscope events: `PASS`;
- vibration start/stop command path: `PASS`;
- ES8311 codec create/open, playback write, nonzero microphone capture, and close: `PASS`;
- terminal marker: `STOPWATCH_HIL_DONE status=PASS failures=0`.

## Acceptance Matrix

| Layer | Status | Evidence |
| --- | --- | --- |
| build | PASS | Clean HIL rebuild/merge and standard selector display-example build |
| flash | PASS | Final Device Test Center task completed |
| transport | PASS | All six expected I2C devices responded; serial had no drops/truncation |
| device_response | PASS | Display/touch/IMU/audio/control APIs returned valid results |
| data_behavior | PASS | IMU events and nonzero microphone samples; power reads returned data |
| battery physical accuracy | PARTIAL | One expected-range and one out-of-range observation; no instrument reference |
| visual_or_physical | PARTIAL | No camera/operator proof for appearance, pressed touch/buttons, audible sound, or felt vibration |

## Safety, Compatibility, And Rollback

- The patch adds a new board and selector entries; it does not change an existing board API.
- Control and sensor operations are task-context calls. No new blocking work, logging, allocation, or locks are introduced in an ISR path.
- Public I2C addresses remain 7-bit values. Only the codec driver's private configuration uses its required 8-bit constant.
- The patch can be rolled back by removing `bsp/m5stack_stopwatch`, its selector dependencies, two example sdkconfigs, and the root table row.
- Apply or reverse against the declared base with `git apply` / `git apply -R` after checking the patch.

## Delivery Boundary

This report proves the BSP builds and exercises all automated board-level paths on real hardware. It does not claim a human-visible display inspection, valid pressed touch coordinates, physical button press events, audible speaker output, felt vibration, battery calibration, component-registry publication, upstream merge, or generic example compatibility that the board architecture does not support.
