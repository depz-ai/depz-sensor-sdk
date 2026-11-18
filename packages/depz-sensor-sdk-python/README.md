# depz-sensor-sdk

Python SDK for the **DEPZ USB sensor line** — USB CDC-ACM sensors that speak
one shared framed protocol:

- **HC-SR04** — ultrasonic distance
- **VL53L0X / VL53L1CX / VL53L1CB / VL53L3CX / VL53L4CX** — single-zone
  Time-of-Flight family, 2–8 m, with ST's light and histogram (multi-target)
  drivers on the host
- **VL53L4CD** — single-zone Time-of-Flight to ~1.3 m (host runs the ST ULD)
- **VL53L5CX / VL53L7CX / VL53L7CH** — 8×8 Time-of-Flight on an I2C board
  (63° / 90° view; L7CH adds streamed CNH histograms)
- **VL53L8CX** — 8×8 multizone Time-of-Flight, the base ToF imager (host runs
  the ST ULD)
- **VL53L8CH** — the VL53L8CX superset, adding Compact-Network-Histogram output
- **TMF8829** — multizone direct ToF, 8×8 … 48×32 zones, up to 4 targets per
  zone, raw histograms (host downloads the ams sensor firmware)
- **BNO055** — 9-axis IMU with on-chip fusion (orientation, gravity, calibration)
- **BNO085 / BNO086** — 9-axis IMU (host runs the SH-2 stack)

Sync API with a background reader thread: open a device, configure it, consume
a stream of decoded, timestamped results. A TypeScript SDK
([`@depz/sensor-sdk`](https://www.npmjs.com/package/@depz/sensor-sdk)) mirrors
this one byte-for-byte via shared golden test vectors.

## Install

```bash
pip install depz-sensor-sdk
```

Python 3.10 or newer; `pyserial` and `numpy` come with it. From a checkout of
the repository, the same package from its sources:

<!-- doc-check: skip — an install from a checkout, not from the registry -->
```bash
pip install ./packages/depz-sensor-sdk-python
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
an index, or an explicit port. With no board attached it raises
`NoDepzDeviceError` (`no DEPZ device found (no serial port has a known DEPZ USB id). …`).

The install puts the `depz-sensor` command on the PATH: `depz-sensor list`
shows what's connected (`no DEPZ devices found`, exit code 1, when nothing
is), `depz-sensor --help` lists the other commands (`info`, `monitor`,
`record`, `record-data`, `play-data`, `flash`).

Runnable programs are in [`examples/`](examples/): `python examples/sr04_minimal.py [port]`
streams an SR04, and there is one each for the VL53L4CD, the VL53L8CX, the
VL53L8CH and the BNO086. Without a port they take the first DEPZ board; with
none attached they print `open: no DEPZ device found …` and exit with code 1.
`python examples/live_check.py [seconds] [port ...]` streams every board it
finds and prints its rate.

See `docs/guide.md` for the full guide and `docs/api.md` for the API reference.

## Compatibility after 1.0

From 1.0 a minor release only adds (contract 07 §5): a field on what the SDK
produces (`LinkStats`, `DeviceInfo`, the events, every measurement / frame /
sample / report), an option on what you pass in (`Tmf8829Config`,
`SyncPinConfig`, `CalibrationConfig`, the BNO055 configs), a member of an
open set (`SensorType`, `Status`, `PlayerState`, the `DeviceEvent` kinds, the
BNO086 `Report` kinds, the `DatasetRecord` kinds). Two rules keep your code
building across those additions:

- Every public dataclass is keyword-only (`kw_only=True`), so a new field
  never shifts a positional argument. Build them with keywords:

  <!-- doc-check: skip — right/wrong pair: the second line fails on purpose -->
  ```python
  cfg = SyncPinConfig(pin=3, mode=SyncPinMode.OUT_BOTH, polarity=SyncPinPolarity.IDLE_HIGH)  # right
  cfg = SyncPinConfig(3, SyncPinMode.OUT_BOTH, SyncPinPolarity.IDLE_HIGH)                    # wrong: TypeError
  ```

- An enum (`IntEnum` or `Enum`) may gain members and an event / report
  hierarchy may gain classes: a `match` on one needs a `case _`, an
  `if`/`elif` chain an `else`.

## License

MIT. The bundled VL53L5 / VL53L7 / VL53L8 sensor firmware, configuration
data and the driver code ported from ST's Ultra Lite Driver are
© STMicroelectronics (BSD-3-Clause,
[`depz_sensor_sdk/vl53l8/data/LICENSE-BSD-3-Clause.txt`](depz_sensor_sdk/vl53l8/data/LICENSE-BSD-3-Clause.txt)); the bundled TMF8829 sensor firmware is © ams-OSRAM (MIT,
`depz_sensor_sdk/tmf8829/data/LICENSES-MIT.TXT`); see `NOTICE`.

Open source — source and issue tracker:
<https://github.com/depz-ai/depz-sensor-sdk>.
