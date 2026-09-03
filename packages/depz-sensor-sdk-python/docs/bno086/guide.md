# BNO086 — user guide

Hands-on guide to the `Bno086` device class. For what the sensor is and its
concepts, read the [introduction](introduction.md); for exact signatures see
the [API reference](../api.md).

## Contents

- [Open](#open)
- [Hello-world: orientation](#hello-world-orientation)
- [Enabling reports](#enabling-reports)
- [Report types and units](#report-types-and-units)
- [Streaming and filtering](#streaming-and-filtering)
- [Tare and calibration](#tare-and-calibration)
- [FRS records](#frs-records)
- [Diagnostics and housekeeping](#diagnostics-and-housekeeping)
- [Gotchas](#gotchas)

## Open

```python
from depz_sensor_sdk import open_device, Bno086

dev = open_device("/dev/ttyACM0")   # returns a Bno086
assert isinstance(dev, Bno086)
print(dev.product_id().version)     # e.g. "3.8.4"
```

Normal use is just open, `enable()`, and read — no reset step required.
`hardware_reset()` is available to restart all SHTP state and cached features
from a clean slate. On firmware newer than v0.95 the SH-2 executable
reset-complete is emitted and the reset is confirmed reliably (ERRATA E9, now
fixed); on older units (≤ v0.95) it was never emitted, so the call is
**best-effort** and still safe there — its absence is not an error and the
sensor is fully usable without it. `wake()` pulses WAKE without losing state.

## Hello-world: orientation

```python
from depz_sensor_sdk import open_device, Bno086, SensorId

with open_device("/dev/ttyACM0") as dev:
    assert isinstance(dev, Bno086)
    dev.enable_rotation_vector(hz=100)
    for r in dev.reports(sensors=[SensorId.ROTATION_VECTOR]):
        print(f"i={r.i:+.3f} j={r.j:+.3f} k={r.k:+.3f} w={r.real:+.3f}"
              f"  acc={r.accuracy_rad:.3f} rad")
```

## Enabling reports

`enable(sensor, hz)` works for any of the ~25 SH-2 reports; give either `hz` or
`interval_us`. There is sugar for the everyday sensors:

```python
dev.enable(SensorId.ROTATION_VECTOR, hz=100)      # generic
dev.enable(SensorId.GYROSCOPE, interval_us=5_000) # 200 Hz, by interval
dev.enable_accelerometer(hz=100)                  # sugar
dev.enable_gyroscope(hz=100)
dev.enable_magnetometer(hz=50)
dev.enable_gravity(hz=100)
dev.enable_gyro_integrated_rv(hz=400)

dev.disable(SensorId.GYROSCOPE)                   # Set Feature, interval 0
```

The hub rounds to its rate grid. With `verify=True` (default) `enable()` reads
the **granted** rate back and returns the `FeatureResponse`; a rate outside
0.9–2.1× your request emits a `UserWarning` (it never raises). Pass
`verify=False` to skip the read-back (returns `None`).

## Report types and units

Every report is a frozen dataclass carrying its wire integers plus scaled
properties (`value = raw / 2**Q`). The common ones:

| enable | class | fields (units) |
|---|---|---|
| `ACCELEROMETER` / `LINEAR_ACCELERATION` / `GRAVITY` | `Acceleration` | `x/y/z` m/s² (Q8) |
| `GYROSCOPE` | `Gyroscope` | `x/y/z` rad/s (Q9) |
| `MAGNETOMETER` | `Magnetometer` | `x/y/z` µT (Q4) |
| `ROTATION_VECTOR` / `GEOMAGNETIC…` / `ARVR…` | `RotationVector` | `i/j/k/real` (Q14) + `accuracy_rad` (Q12, radians) |
| `GAME_ROTATION_VECTOR` / `ARVR…GAME` | `RotationVector` | `i/j/k/real`; `accuracy_rad` is `None` |
| `GYRO_INTEGRATED_RV` | `GyroIntegratedRV` | quaternion (Q14) + `angular_velocity` rad/s (Q10) |
| `STEP_COUNTER` | `StepCounter` | `steps`, `latency_us` |
| `TAP_DETECTOR` | `TapDetector` | `double_tap` |
| `STABILITY_CLASSIFIER` | `StabilityClassifier` | `name` |
| `PERSONAL_ACTIVITY_CLASSIFIER` | `PersonalActivityClassifier` | `most_likely_name`, `confidences` |

Every `InputReport` also carries `seq` (rolling sample counter for drop
detection), `accuracy` (0 unreliable … 3 high), `delay_us`, and an absolute
`timestamp_us` in the MCU clock. Unrecognised report IDs surface as
`UnknownReport`.

## Streaming and filtering

```python
# bounded, drop-oldest iterator; filter by SensorId
it = dev.reports(sensors=[SensorId.ROTATION_VECTOR, SensorId.ACCELEROMETER])
r = next(it)
print("dropped:", it.dropped_count)

# callback — fires on the reader thread; keep it quick
unsub = dev.on_report(lambda r: q.put(r), sensors=SensorId.GYROSCOPE)
```

Both subscribe eagerly, so reports emitted right after the call are never
missed. `sensors=None` receives everything.

## Tare and calibration

```python
from depz_sensor_sdk.bno086 import TareAxis, TareBasis

dev.tare_now(axes=TareAxis.ALL, basis=TareBasis.ROTATION_VECTOR)
dev.persist_tare()                       # store the tare into FRS
dev.set_reorientation(0.0, 0.0, 0.0, 1.0)  # runtime quaternion (Q14; zeros clears)

dev.set_calibration(accel=True, gyro=True, mag=False)   # configure ME cal
cfg = dev.get_calibration()              # which ME cals are running
dev.save_dcd()                           # persist dynamic calibration to flash
dev.configure_periodic_dcd(True)         # enable the hub's periodic autosave
```

`set_calibration` / `save_dcd` / `clear_counts` raise `Sh2Error` on a non-zero
status; the fire-and-forget commands (`tare_now`, `persist_tare`,
`set_reorientation`, `configure_periodic_dcd`) have no SH-2 response.

## FRS records

```python
from depz_sensor_sdk.bno086.sh2 import FrsRecordId

words = dev.frs_read(FrsRecordId.SYSTEM_ORIENTATION)   # 32-bit words
dev.frs_write(FrsRecordId.SYSTEM_ORIENTATION, words)   # whole-record write
md = dev.get_metadata(SensorId.ACCELEROMETER)          # parsed metadata record
print(md.min_period_us, md.max_period_us, md.q_point_1)
```

Reading an unknown record raises `Sh2Error`.

## Diagnostics and housekeeping

```python
from depz_sensor_sdk.bno086 import OscillatorType

dev.get_oscillator_type()            # → OscillatorType.EXT_CRYSTAL
dev.get_errors(severity=0)           # drain the error queue → [ErrorRecord]
counts = dev.get_counts(SensorId.ROTATION_VECTOR)  # offered/accepted/on/attempted
dev.clear_counts(SensorId.ROTATION_VECTOR)
dev.clear_dcd_and_reset()            # wipe in-RAM calibration, then reset
```

`clear_dcd_and_reset()` behaves like `hardware_reset()` — it resets the sensor
(reset-complete is reliable on firmware newer than v0.95, best-effort on older
units; see ERRATA E9, now fixed).

## Gotchas

- **Enable before you stream** — no report arrives until you `enable()` it.
- **Rotation-vector accuracy is in radians** (`accuracy_rad`, Q12); the integer
  `accuracy` field (0–3) is a separate reliability level.
- **Rate is a grid** — the granted rate can differ from your request; `enable`
  warns (never raises) when it lands outside 0.9–2.1×.
- **Game/AR-VR-game rotation vectors have no accuracy** — `accuracy_rad` is
  `None`.
- **Correlation is SH-2-level** — the bridge returns everything as unsolicited;
  the SDK reassembles SHTP and correlates for you. That's why you think in
  reports, not request/reply.
- **Callbacks run on the reader thread** — never call blocking device methods
  (`enable`/`tare`/…) from inside one; hand off to a queue.
