# Python SDK

`depz-sensor-sdk` turns any device of the DEPZ USB sensor line — HC-SR04
ultrasonic, the VL53L0X / L1CX / L1CB / L3CX / L4CD / L4CX single-zone ToF
family, the VL53L5CX / L7CX / L7CH / L8CX / L8CH 8×8 ToF matrices, the BNO086
and BNO055 9-axis IMUs — into a typed Python object streaming decoded,
timestamped measurements.

**Working with one specific sensor?** Each has its own introduction, guide
and API reference:

- **[SR04](sr04/introduction.md)** — ultrasonic ranging: one distance per ping.
- **[VL53L4CD](vl53l4cd/introduction.md)** — single-zone ToF distance to ~1.3 m.
- **[VL53L8CX](vl53l8cx/introduction.md)** — 8×8 ToF depth frames.
- **[VL53L8CH](vl53l8ch/introduction.md)** — the CX superset with CNH histograms.
- **[VL53L5CX](vl53l5cx/introduction.md)** — 8×8 ToF, 63° view, I2C board.
- **[VL53L7CX](vl53l7cx/introduction.md)** — 8×8 ToF, 90° view, I2C board.
- **[VL53L7CH](vl53l7ch/introduction.md)** — the L7CX superset with streamed CNH histograms.
- **[VL53L0X](vl53l0x/introduction.md)** — single-zone ToF to ~2 m.
- **[VL53L1CX](vl53l1cx/introduction.md)** — single-zone ToF to ~4 m, ROI, multi-target histogram driver.
- **[VL53L1CB](vl53l1cb/introduction.md)** — the L1 die behind a cover glass, to ~8 m.
- **[VL53L3CX](vl53l3cx/introduction.md)** — multi-target ToF to ~3 m.
- **[VL53L4CX](vl53l4cx/introduction.md)** — multi-target ToF to ~6 m.
- **[BNO086](bno086/introduction.md)** — 9-axis IMU: orientation and motion.
- **[BNO055](bno055/introduction.md)** — 9-axis IMU with on-chip fusion: orientation, gravity, calibration.

- **One protocol, one mental model** — every sensor is a USB CDC-ACM device
  speaking the same framed protocol; `open_device()` probes the port and
  returns the right class (`Sr04`, `Vl53l4cd`, `Vl53l8cx`, `Vl53l8ch`,
  `Vl53l5cx`, `Vl53l7cx`, `Vl53l7ch`, `Vl53l0x`, `Vl53l1cx`, `Vl53l1cb`,
  `Vl53l3cx`, `Vl53l4cx`, `Bno086`, `Bno055`).
- **Callbacks or iterators** — register a callback, or pull from a bounded
  drop-oldest iterator; a background reader thread per device does the I/O.
- **Time sync** — NTP-style `sync_time()` maps device microseconds onto the
  host clock, so multi-device recordings share one timeline.
- **Record & replay** — capture time-synced multi-device datasets
  (`.depzdata`) and replay them; a raw-byte layer (`.depzrec`) replays the
  full SDK stack byte-for-byte for hardware-free tests.
- **Firmware update** — flash `.fwdepz` application images from Python or the
  bundled `depz-sensor` CLI.

## Install

```bash
pip install depz-sensor-sdk
```

Linux note: your user must be able to open the port (group `dialout`).

## Quickstart

Plug in a DEPZ sensor and run:

```python
from depz_sensor_sdk import list_depz_devices, open_device

for info in list_depz_devices():
    print(info.port, info.sensor_type, info.serial_number)

dev = open_device("/dev/ttyACM0")   # returns the class of whatever is plugged in
```

## A fuller example

8×8 depth frames from a VL53L8 ToF sensor:

```python
from depz_sensor_sdk import open_device
from depz_sensor_sdk.vl53l8 import RESOLUTION_8X8

dev = open_device("/dev/ttyACM0")      # returns a Vl53l8cx or Vl53l8ch
dev.init(progress=print)               # ~25 s: downloads the sensor firmware
dev.set_resolution(RESOLUTION_8X8)
dev.set_ranging_frequency_hz(15)
dev.start_ranging()
for frame in dev.frames():
    grid = frame.grid()                # 8×8 numpy array of mm
    print("center:", grid[3:5, 3:5].mean(), "°C:", frame.silicon_temp_degc)
```

## Where to next

- **Your sensor's pages** — [SR04](sr04/introduction.md) ·
  [VL53L4CD](vl53l4cd/introduction.md) · [VL53L8CX](vl53l8cx/introduction.md) ·
  [VL53L8CH](vl53l8ch/introduction.md) · [VL53L5CX](vl53l5cx/introduction.md) ·
  [VL53L7CX](vl53l7cx/introduction.md) · [VL53L7CH](vl53l7ch/introduction.md) ·
  [VL53L0X](vl53l0x/introduction.md) · [VL53L1CX](vl53l1cx/introduction.md) ·
  [VL53L1CB](vl53l1cb/introduction.md) · [VL53L3CX](vl53l3cx/introduction.md) ·
  [VL53L4CX](vl53l4cx/introduction.md) · [BNO086](bno086/introduction.md) ·
  [BNO055](bno055/introduction.md):
  introduction, hands-on guide, and the sensor's own API reference.
- **[Guide](guide.md)** — the SDK-wide walkthrough: the mental model,
  discovery, streaming, recording & replay, firmware update, and common
  pitfalls.
- **[API Reference](api.md)** — every public class and function of the whole
  SDK, generated from the docstrings.
- **Docs for LLMs** — this documentation as one raw-Markdown file:
  [/llms-full.txt](/llms-full.txt).
- **Source & license** — the SDKs are open source (MIT):
  [github.com/depz-ai/depz-sensor-sdk](https://github.com/depz-ai/depz-sensor-sdk).
