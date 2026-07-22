# StopWatch esp-bsp HIL smoke test

This project validates the board-level APIs added by the StopWatch BSP. It uses
the BSP directly from an esp-bsp checkout so the archived patch can be applied
and rebuilt without publishing the component first.

```powershell
$env:ESP_BSP_PATH = 'C:\path\to\esp-bsp'
idf.py -B build build
idf.py -B build merge-bin -o stopwatch-bsp-hil-merged.bin
```

Generated build directories, managed components, dependency locks, local
`sdkconfig`, and firmware binaries are ignored. The committed source and
`sdkconfig.defaults` are the portable inputs.

Expected terminal marker:

```text
STOPWATCH_HIL_DONE status=PASS failures=0
```

The automated checks cover device presence, control-path calls, display writes,
IMU events, codec playback/capture calls, and button idle levels. Physical button
presses, touch coordinates, display appearance, audible sound, and felt vibration
require operator or camera/fixture evidence and are reported separately.
