# VL53L4CD — user guide

Hands-on guide to the `Vl53l4Cd` device class. For what the sensor is and its
concepts, read the [introduction](introduction.md); for exact signatures see
the [API reference](api.md).

## Contents

- [Open and initialize](#open-and-initialize)
- [Hello-world: live distance](#hello-world-live-distance)
- [Configuration](#configuration)
- [Single shot vs streaming](#single-shot-vs-streaming)
- [The measurement](#the-measurement)
- [Calibration](#calibration)
- [XSHUT: power and reset](#xshut-power-and-reset)
- [Bridge diagnostics](#bridge-diagnostics)
- [Gotchas](#gotchas)

## Open and initialize

```python
from depz_sensor_sdk import Vl53l4Cd, open_device

dev = open_device("/dev/ttyACM0")   # returns a Vl53l4Cd
assert isinstance(dev, Vl53l4Cd)
dev.init()                          # ULD boot + VHV calibration, < 1 s
```

`init()` writes the ULD default configuration and runs VHV calibration. There
is no firmware download — the VL53L4CD carries its own — so it completes in
well under a second. It leaves the bridge's I2C bus at 1 MHz; pass
`bus_khz=400` (or any step in `I2C_KHZ_STEPS`) to stay slower.

## Hello-world: live distance

```python
from depz_sensor_sdk import Vl53l4Cd, open_device

with open_device("/dev/ttyACM0") as dev:
    assert isinstance(dev, Vl53l4Cd)
    dev.init()
    dev.start_ranging()              # default: 50 ms budget, continuous (~20 Hz)
    for m in dev.measurements():     # blocks; Ctrl+C to stop
        if m.valid:
            print(f"{m.distance_mm:5d} mm  sigma {m.sigma_mm} mm")
        else:
            print(m.status_text)
```

## Configuration

Call these **after `init()` and while not ranging** (they raise otherwise).

```python
# timing: budget 10–200 ms; inter_measurement 0 = continuous,
# > budget = autonomous low-power mode
dev.set_range_timing(50, 0)          # 50 ms budget, back-to-back (~20 Hz)
dev.set_range_timing(200, 1000)      # autonomous: one 200 ms sample per second
dev.get_range_timing()               # → (timing_budget_ms, inter_measurement_ms)

# corrections
dev.set_offset_mm(-7)                # signed ranging offset
dev.set_xtalk_kcps(12)               # crosstalk compensation (0 = disabled)

# quality limits — measurements outside them report a non-zero range_status
dev.set_signal_threshold_kcps(1024)  # discard weak returns
dev.set_sigma_threshold_mm(15)       # discard noisy ranges (<= 16383)

# distance-window interrupt: INT only fires when the condition holds
from depz_sensor_sdk.vl53l4 import WINDOW_IN
dev.set_detection_thresholds(100, 300, WINDOW_IN)   # only 100–300 mm
```

Every setter has a matching getter that reads the value back from the sensor.
After a >8 °C ambient change, call `start_temperature_update()` to re-run VHV
calibration.

## Single shot vs streaming

```python
# one-shot: start, wait for data-ready, read, stop — all in one call
m = dev.measure_once(timeout=1.0)

# streaming: the sensor's INT pin drives one report per measurement
dev.start_ranging()
# ... consume dev.measurements() / dev.get_measurement() ...
dev.stop_ranging()
```

`measure_once()` raises while the stream is running — stop it first, or just
consume streamed measurements. While ranging, three consumption styles:

```python
# iterator — bounded, drop-oldest; subscribes immediately
it = dev.measurements(maxsize=64)
m = next(it)
print("dropped so far:", it.dropped_count)

# callback — fires on the reader thread; keep it quick
unsub = dev.on_measurement(lambda m: q.put(m))

# convenience: block for the next measurement
m = dev.get_measurement(timeout=2.0)   # raises DepzTimeoutError on silence
```

`dev.stream_parse_errors` counts stream reports whose result block failed to
decode — 0 in a healthy session.

## The measurement

`Vl53l4Measurement` mirrors the ULD's `VL53L4CD_ResultsData_t` plus the MCU
timestamp of the INT edge:

| field | meaning |
|---|---|
| `timestamp_us` | MCU uptime at the INT edge (stream) / read (poll) |
| `range_status` | 0 = valid; see `RANGE_STATUS_NAMES` / `status_text` |
| `distance_mm` | measured distance, mm |
| `sigma_mm` | range std-dev estimate, mm |
| `signal_rate_kcps` / `signal_per_spad_kcps` | return-signal rate |
| `ambient_rate_kcps` / `ambient_per_spad_kcps` | ambient-light rate |
| `number_of_spad` | SPADs used |
| `stream_count` | sensor frame counter, wraps at 255 |

Check `m.valid` (i.e. `range_status == 0`) before trusting a distance — a
non-zero status (signal too low, sigma too high, wrap-around…) is a
legitimate reading, not a protocol error.

## Calibration

Both calibrations block for a sample burst against a target at a known
distance, program the sensor, and return the value now in effect:

```python
offset = dev.calibrate_offset(target_dist_mm=100)   # target at 10–1000 mm
xtalk = dev.calibrate_xtalk(target_dist_mm=600)     # target at 10–5000 mm
```

The programmed values do **not** survive an XSHUT power-cycle; read them back
(`get_offset_mm()` / `get_xtalk_kcps()`) and re-apply them after `init()` if
you need persistence.

## XSHUT: power and reset

The bridge drives the sensor's XSHUT (shutdown) pin:

```python
from depz_sensor_sdk.vl53l4 import XSHUT_OFF, XSHUT_ON
dev.reset_sensor()      # power-cycle (~3 ms on the MCU)
dev.xshut(XSHUT_OFF)    # sensor off — stream stops, config lost
dev.xshut(XSHUT_ON)     # sensor back on, in its boot state
```

A power-cycled sensor holds **none** of the ULD configuration: `initialized`
goes `False` and everything — timing, offset, xtalk, thresholds — is back at
sensor defaults. Call `init()` and reconfigure before ranging again.

## Bridge diagnostics

`bridge_info()` returns the MCU's own view of the sensor (`Vl53l4Info`) and is
safe to call while streaming:

```python
info = dev.bridge_info()
info.model_id      # 0xEBAA on a live VL53L4CD
info.int_edges     # INT edges seen — should grow while ranging
info.slots_skipped # stream slots the MCU could not service
info.i2c_errors    # bus errors, with last_i2c_error naming the latest
info.i2c_khz       # the programmed bus-speed step
```

Counters are free-running and wrap silently — watch increments, not absolute
values.

## Gotchas

- **`init()` after every power-cycle.** XSHUT off/reset wipes the ULD config;
  the SDK tracks this via `dev.initialized`.
- **Config while ranging raises** — the INT-driven stream owns the register
  bank; stop, reconfigure, restart.
- **Always check `m.valid`.** Non-zero `range_status` values are data
  ("signal below threshold", "wrapped target"…), not errors.
- **`inter_measurement_ms` must be 0 or > the budget** — values between 1 and
  the budget are rejected by the ULD.
- **`measure_once()` while streaming raises** — one ranging engine; stop the
  stream first.
- **Callbacks run on the reader thread** — don't block; don't call blocking
  device methods from inside one.
