# depz-sensor-sdk

Python SDK for the **DEPZ USB sensor line** — USB CDC-ACM sensors that speak
one shared framed protocol:

- **HC-SR04** — ultrasonic distance
- **VL53L4CD** — single-zone Time-of-Flight to ~1.3 m (host runs the ST ULD)
- **VL53L8CX** — 8×8 multizone Time-of-Flight, the base ToF imager (host runs
  the ST ULD)
- **VL53L8CH** — the VL53L8CX superset, adding Compact-Network-Histogram output
- **VL53L5CX / VL53L7CX / VL53L7CH** — 8×8 Time-of-Flight on an I2C board
  (63° / 90° view; L7CH adds streamed CNH histograms)
- **VL53L0X / VL53L1CX / VL53L1CB / VL53L3CX / VL53L4CX** — single-zone
  Time-of-Flight family, 2–8 m, with ST's light and histogram (multi-target)
  drivers on the host
- **BNO086** — 9-axis IMU (host runs the SH-2 stack)
- **BNO055** — 9-axis IMU with on-chip fusion (orientation, gravity, calibration)

Sync API with a background reader thread: open a device, configure it, consume
a stream of decoded, timestamped results. A TypeScript SDK
([`@depz/sensor-sdk`](https://www.npmjs.com/package/@depz/sensor-sdk)) mirrors
this one byte-for-byte via shared golden test vectors.

## Install

```bash
pip install depz-sensor-sdk
```

Linux: your user needs serial access (group `dialout`).

## Quick start

```python
from depz_sensor_sdk import open_device
from depz_sensor_sdk.vl53l8 import Vl53l8cx, RESOLUTION_8X8

dev = open_device()                    # first DEPZ sensor by USB id; raises if none
if isinstance(dev, Vl53l8cx):
    dev.init(progress=print)           # ~25 s: downloads the sensor firmware
    dev.set_resolution(RESOLUTION_8X8)
    dev.set_ranging_frequency_hz(15)   # must be >= 2 Hz
    dev.start_ranging()
    for frame in dev.frames():
        print(frame.grid())            # 8×8 numpy array of mm
```

`open_device()` finds the device by USB VID/PID; pass a serial (`open_device(serial="...")`),
an index, or an explicit port. `depz-sensor list` shows what's connected.

See `docs/guide.md` for the full guide and `docs/api.md` for the API reference.

## License

MIT. Bundled VL53L8 sensor-firmware blobs are © STMicroelectronics
(BSD-3-Clause); see `NOTICE`.

Open source — source and issue tracker:
<https://github.com/depz-ai/depz-sensor-sdk>.
