# BNO055 — user guide

Hands-on guide to the `Bno055` device class. For what the sensor is and its
concepts, read the [introduction](introduction.md); for exact signatures see
the [API reference](api.md).

## Contents

- [Open and configure](#open-and-configure)
- [Hello-world: orientation](#hello-world-orientation)
- [The sample](#the-sample)
- [Streaming](#streaming)
- [Modes and power](#modes-and-power)
- [Units](#units)
- [Axis remap](#axis-remap)
- [Calibration](#calibration)
- [Status and self-test](#status-and-self-test)
- [Page 1: sensor configs, interrupts, unique id](#page-1-sensor-configs-interrupts-unique-id)
- [Reset and bridge diagnostics](#reset-and-bridge-diagnostics)
- [Gotchas](#gotchas)

## Open and configure

```python
from depz_sensor_sdk import Bno055, OprMode, Units, open_device

dev = open_device("/dev/ttyACM0")   # returns a Bno055
assert isinstance(dev, Bno055)
dev.configure()                     # CONFIG → units → NDOF, fusion live on return
```

`configure()` is the usual session setup in one call: switch to CONFIG, write
the units, optionally an axis remap and a stored calibration profile, then
switch to the operating mode (NDOF by default). In a fusion mode it returns
only once the fusion outputs are live — they read zero for ~70 ms after every
switch out of CONFIG. The SDK remembers the arguments for
`restore_configuration()`.

```python
dev.configure(OprMode.IMU,                      # accel + gyro, no magnetometer
              units=Units(euler_rad=True),     # Euler angles in radians
              axis_remap="P2")                 # datasheet mounting P2
```

## Hello-world: orientation

```python
from depz_sensor_sdk import Bno055, open_device

with open_device("/dev/ttyACM0") as dev:
    assert isinstance(dev, Bno055)
    dev.configure()
    dev.start_stream(10)                       # 100 Hz, the full 46-byte block
    for s in dev.samples():                    # blocks; Ctrl+C to stop
        heading, roll, pitch = s.euler
        print(f"heading {heading:6.1f}  roll {roll:6.1f}  pitch {pitch:6.1f}  "
              f"calib {s.calibration}")
```

Without a stream, poll: `dev.read_sample()` reads the full block once,
`dev.read_quaternion()` just the 8 quaternion bytes (the cheapest read).

## The sample

`Bno055Sample` is one decoded register block. A channel is `None` when the
block did not cover it; values are scaled by the units in force when the
stream started (`s.units`).

| field | meaning | unit (default) |
|---|---|---|
| `timestamp_us` | MCU uptime at the timer tick (stream) / read completion (poll) | µs |
| `quaternion` | `(w, x, y, z)`, unit length | — |
| `euler` | `(heading, roll, pitch)` | degrees |
| `accel` | acceleration including gravity | m/s² |
| `linear_accel` | acceleration with gravity removed | m/s² (always) |
| `gravity` | gravity vector | m/s² (always) |
| `gyro` | angular rate | deg/s |
| `mag` | magnetic field | µT |
| `temperature` | chip temperature | °C |
| `calibration` | `CalibStatus(system, gyro, accel, mag)`, each 0..3 | — |
| `raw`, `addr` | the register bytes and their start address | — |

In non-fusion modes the fusion fields (quaternion, Euler, gravity, linear
acceleration) read zero; in CONFIG mode everything does.

## Streaming

```python
from depz_sensor_sdk.bno055 import FULL_BLOCK, QUAT_BLOCK

dev.start_stream(10)                    # full block every 10 ms (≈3.2 ms of bus)
dev.start_stream(20, QUAT_BLOCK)        # quaternion only, 50 Hz (≈1.2 ms of bus)
dev.stop_stream()
```

The bridge reads the block on its own timer and pushes it with the MCU
timestamp of the tick, so gaps show up as jumps in `timestamp_us`. Three ways
to consume:

```python
it = dev.samples(maxsize=256)       # iterator — bounded, drop-oldest
s = next(it)
print("dropped so far:", it.dropped_count)

unsub = dev.on_sample(lambda s: q.put(s))   # callback on the reader thread

s = dev.get_sample(timeout=1.0)     # next sample; DepzTimeoutError on silence
```

Commands stay usable while streaming (`calibration_status()`, `read_sample()`,
mode switches); page-1 access does not (see below). `stream_parse_errors`
counts reports that failed to decode — 0 in a healthy session.

## Modes and power

```python
dev.get_operation_mode()              # → OprMode.NDOF
dev.set_operation_mode(OprMode.AMG)   # raw accel + mag + gyro, no fusion
dev.set_operation_mode(OprMode.NDOF)  # waits for the fusion to come up

from depz_sensor_sdk.bno055 import PwrMode, TempSource
dev.set_power_mode(PwrMode.LOW_POWER)       # accel only until motion
dev.set_temperature_source(TempSource.GYRO)
```

| mode | sensors | output |
|---|---|---|
| `CONFIG` | — | configuration only, all outputs zero |
| `ACCONLY`, `MAGONLY`, `GYROONLY`, `ACCMAG`, `ACCGYRO`, `MAGGYRO`, `AMG` | as named | raw data only |
| `IMU` | accel + gyro | relative orientation, 100 Hz |
| `COMPASS` | accel + mag | absolute heading, 20 Hz |
| `M4G` | accel + mag | relative orientation from the magnetometer, 50 Hz |
| `NDOF_FMC_OFF` | all three | absolute orientation, 100 Hz, slow mag calibration |
| `NDOF` | all three | absolute orientation, 100 Hz (default) |

The sensor only switches between CONFIG and an operating mode — a write from
one operating mode straight to another is silently ignored.
`set_operation_mode()` handles that by going through CONFIG, so NDOF → AMG
just works (and costs the CONFIG round trip).

## Units

```python
from depz_sensor_sdk import Units

dev.set_units(Units(accel_mg=True, gyro_rps=True, euler_rad=True, temp_f=True))
dev.get_units()       # read back from UNIT_SEL
```

`Units()` (all flags off) is m/s², deg/s, degrees, °C and Windows orientation
— the SDK default. The sensor's own power-on value is different (Android
orientation), so `configure()` always writes it. `android=True` flips the
pitch sign convention.

Linear acceleration and gravity stay in m/s² even with `accel_mg=True` — only
the raw acceleration switches to mg. That is what the sensor does (measured),
whatever some datasheet tables say; the SDK scales accordingly.

## Axis remap

```python
from depz_sensor_sdk import AxisRemap
from depz_sensor_sdk.bno055 import PLACEMENTS

dev.set_axis_remap("P0")            # one of the datasheet placements P0..P7
dev.set_axis_remap(AxisRemap(x=1, y=0, z=2, x_negative=True))   # output X = −chip Y
dev.get_axis_remap()
```

P1 is the default (chip axes as printed). A mapping that uses one axis twice
raises `ValueError` — the sensor would silently keep the old one.

## Calibration

```python
st = dev.calibration_status()      # CalibStatus(system, gyro, accel, mag), 0..3
st.fully_calibrated                # all four at 3
```

What each sensor needs (datasheet §3.11): **gyro** — hold still for a few
seconds; **accel** — six still poses, each axis up and down; **magnetometer**
— slow figure-eights in the air. The fusion calibrates continuously in the
background; you cannot disable it.

Save the result once `fully_calibrated` is true, restore it after every
power cycle:

```python
import json
from depz_sensor_sdk import CalibrationProfile

profile = dev.read_calibration_profile()          # 22 bytes, read in CONFIG
json.dump(profile.to_dict(), open("bno055_calib.json", "w"))

saved = CalibrationProfile.from_dict(json.load(open("bno055_calib.json")))
dev.configure(calibration=saved)                  # or write_calibration_profile()
```

A restored profile is a starting point, not a lock: as soon as the fusion
runs it keeps refining the offsets (with an uncalibrated magnetometer it
rewrites the magnetometer radius straight away). To check that a write landed,
read it back without leaving CONFIG. Don't store the profile of an
uncalibrated sensor: its magnetometer radius is 0, outside the legal
144..1280, and restoring it makes the sensor report a fusion configuration
error. The soft-iron matrix is there too:
`get_sic_matrix()` / `set_sic_matrix()` (9 × i16, 1.0 = 16384).

## Status and self-test

```python
st = dev.system_status()
st.status_text        # "fusion algorithm running"
st.error_text         # "no error"
st.self_test_passed   # power-on self-test result (ST_RESULT)

st = dev.self_test()  # built-in self-test, ~0.45 s; mode is restored after
```

`error` / `error_text` only mean something when `status == 1` ("system
error") — at other times the register may still hold an old value.
`self_test()` refuses while streaming. A self-test run in CONFIG leaves
`SYS_STATUS` at 4 ("executing self-test") until the mode leaves CONFIG, so
when the sensor was in CONFIG it ends with a step into ACCONLY and back; the
returned `status` is the 4 read during the test. `configure()` and
`reset_sensor()` clear such a leftover too (measured on SW 03.11; it used to
make them fail with "BNO055 did not finish booting").

## Page 1: sensor configs, interrupts, unique id

```python
from depz_sensor_sdk.bno055 import AccelConfig
from depz_sensor_sdk.bno055.regs import INT_ACC_AM, REG1_ACC_AM_THRES

dev.unique_id().hex()                       # 16-byte chip id

dev.get_accel_config()                      # AccelConfig(range=1, bandwidth=3, power=0)
dev.set_accel_config(AccelConfig(range=2))  # ±8 g — effective in non-fusion modes only

dev.set_interrupt_setting(REG1_ACC_AM_THRES, 0x14)   # raw threshold byte
dev.set_interrupt_enable(INT_ACC_AM)                 # any-motion engine on
dev.set_interrupt_mask(INT_ACC_AM)                   # ...and routed to the INT pin
dev.read_interrupt_status()                          # INT_STA — clears on read
dev.clear_interrupt()
```

The fusion modes override the page-1 sensor configs. On these boards only the
**motion** interrupts work (any/no-motion, high-g, high-rate); the data-ready
bits exist but never fire on sensor firmware 03.11. An INT-triggered stream
(`start_stream(1000, block, trigger=TRIGGER_INT)`) reads the block on each
motion event, with `period_ms` as a watchdog. Every page-1 call raises while a
stream runs.

## Reset and bridge diagnostics

```python
dev.reset_sensor()            # nRESET pulse; waits until the sensor has booted
dev.restore_configuration()   # re-apply the last configure()

info = dev.bridge_info()
info.ids_ok                   # chip/acc/mag/gyr ids as expected
info.sw_rev_text              # sensor firmware, e.g. "03.11"
info.read_avg_us              # I2C time of one streamed block read
info.slots_skipped            # timer ticks dropped because the bus was busy
info.sensor_resets            # the bridge pulsed nRESET to recover the bus
```

After a reset the sensor is in CONFIG with power-on settings — every output
reads zero until you configure again. If `sensor_resets` grows during a long
run, the bridge recovered a stuck bus by resetting the sensor: call
`restore_configuration()`; the stream itself keeps running.

## Gotchas

- **Zero quaternion = not fusing.** CONFIG mode, a non-fusion mode, or the
  first ~70 ms after a switch. `configure()` and `set_operation_mode()` wait
  that out for you; raw register writes to OPR_MODE do not.
- **No direct mode-to-mode switch** — the sensor ignores it; the SDK goes via
  CONFIG. A raw `write_register(REG_OPR_MODE, …)` from NDOF to AMG does nothing.
- **Settings silently ignored outside CONFIG.** Use the typed setters (they go
  to CONFIG and back); a raw `write_register()` of UNIT_SEL in NDOF does nothing.
- **Every CONFIG round trip restarts the fusion** — reading the calibration
  profile or changing units costs another ~70 ms of zeros.
- **Don't benchmark in CONFIG** — the sensor stretches the bus 3–5× harder
  there; a 46-byte read takes ~10 ms instead of ~3 ms.
- **Page 1 while streaming raises** — stop the stream first.
- **Callbacks run on the reader thread** — keep them short and don't call
  blocking device methods from inside one.
