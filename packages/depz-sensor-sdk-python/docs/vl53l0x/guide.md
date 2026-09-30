# VL53L0X — user guide

Hands-on guide to the `Vl53l0x` device class. For what the sensor is and
its concepts, read the [introduction](introduction.md); for exact signatures
see the [API reference](api.md).

## Contents

- [Open and initialize](#open-and-initialize)
- [Hello-world: live distance](#hello-world-live-distance)
- [Configure](#configure)
- [Streaming and single shot](#streaming-and-single-shot)
- [The measurement](#the-measurement)
- [Calibration](#calibration)
- [Tuning](#tuning)
- [What each driver supports](#what-each-driver-supports)
- [Reset and bridge diagnostics](#reset-and-bridge-diagnostics)
- [Gotchas](#gotchas)

## Open and initialize

```python
from depz_sensor_sdk import Vl53l0x, open_device

dev = open_device("/dev/ttyACM0")   # returns a Vl53l0x (from the board name)
assert isinstance(dev, Vl53l0x)
dev.init()                          # driver "uld"
print(dev.identify()["driver_class"], dev.notes())
```

`open_device()` picks the class from the board's USB id or its device name
(`… VL53L0X USB v2.1 …`) — every 1D board runs the same firmware, so the
firmware name cannot tell them apart. `identify()` returns everything about
the bound pair: product, driver, model id, reach, supported groups, modes and
budgets.

## Hello-world: live distance

```python
from depz_sensor_sdk import Vl53l0x, open_device

with open_device("/dev/ttyACM0") as dev:
    dev.init()
    dev.configure(budget_ms=50, mode="long-range")
    dev.start_ranging()
    for m in dev.measurements():     # blocks; Ctrl+C to stop
        if m.plottable:
            print(f"{m.distance_mm:5d} mm  sigma {m.sigma_mm:.1f}")
        else:
            print(m.status_text)
```

## Configure

```python
dev.configure(budget_ms=50, inter_ms=0, mode="long-range")   # re-init + apply
dev.get_range_timing()        # → (budget_ms, inter_ms) read back from the sensor
```

`configure()` re-initialises the sensor, then applies the mode, the timing and
any stored calibration. Call it before every run. `inter_ms=0` ranges
back-to-back; a larger value is the period between measurements and must
exceed the budget.

`dev.modes` lists the named modes of the current driver (the first one is what
`init()` leaves); `dev.get_mode()` reads back the one in use. A mode change
rewrites the timing, which is why `configure()` applies the mode first.

## Streaming and single shot

```python
m = dev.measure_once(timeout=1.0)    # start, wait, read, stop — not while streaming

dev.start_ranging()                  # INT-driven stream
it = dev.measurements(maxsize=64)    # iterator — bounded, drop-oldest
m = next(it)
unsub = dev.on_measurement(print)    # callback on the reader thread
m = dev.get_measurement(timeout=2.0) # next one; DepzTimeoutError on silence
dev.stop_ranging()
```

`dev.stream_parse_errors` counts stream reports that failed to decode — 0 in a
healthy session.

## The measurement

| field | meaning |
|---|---|
| `timestamp_us` | MCU uptime at the INT edge (stream) / the read (poll) |
| `distance_mm` | distance, mm |
| `status`, `status_text` | 0 = valid; the text names the failure otherwise |
| `sigma_mm` | range std-dev estimate |
| `signal_kcps`, `ambient_kcps` | return-signal and ambient rates |
| `spads` | active SPADs |
| `targets` | every return (histogram driver), else empty |
| `extra` | driver-specific values (stream count, per-SPAD rates …) — display only |
| `bins` | the raw 24-bin histogram (histogram driver), else `None` |

`m.valid` is `status == 0`; `m.plottable` also accepts the statuses the
histogram driver uses for usable frames — prefer it.

## Calibration

```python
offset = dev.calibrate_offset(target_dist_mm=500)   # flat target at a known distance
xtalk = dev.calibrate_xtalk(target_dist_mm=600)     # cover-glass crosstalk, kcps

# the values live in sensor RAM — store them and re-apply on every run:
dev.configure(budget_ms=50, offset_mm=offset, xtalk_kcps=xtalk)
```

Calibrations block for a burst of samples, program the sensor and return the
value now in effect. A reset or power cycle drops them.

## Tuning

Only where `supports()` says so — after `configure()`, while not ranging:

```python
dev.perform_ref_spad_management()                   # re-measure reference SPADs
```

## What each driver supports

| driver | capability groups |
|---|---|
| `uld` | timing budget / inter-measurement period, named ranging modes, offset correction, offset calibration, crosstalk correction, crosstalk calibration, reference-SPAD management |

Ask `dev.supports("roi")` (etc.) at run time; a call outside the list raises.

## Reset and bridge diagnostics

```python
from depz_sensor_sdk.vl53lx import XSHUT_RESET
dev.xshut(XSHUT_RESET)     # power-cycle the sensor — init() again afterwards

info = dev.bridge_info()   # bridge counters, safe while streaming
info.int_edges, info.slots_skipped, info.frames_dropped, info.i2c_errors, info.i2c_khz
```

A power-cycled sensor holds none of the configuration or calibration.

`i2c_errors` counts from the end of the last init: a resetting die NACKs its
own address for a moment, so the SDK clears the counter once `init()` /
`configure()` is through (VL53_CLEAR_I2C_ERRORS). That needs firmware
`APP_VL53L0_4_v0.24` or newer; on an older board `init()` raises a
`DepzError` that says to reflash it.

## Gotchas

- **`configure()` before every run** — it re-initialises the sensor, so
  nothing from an earlier session carries over.
- **Configuration while ranging raises** — stop, reconfigure, restart.
- **Prefer `plottable` over `valid`** — non-zero statuses are data, not errors.
- **No thresholds, ROI or temperature update** on this ULD — `supports()` says so; the calls raise.
- **A NACK or two right after reset is normal** — the part is still booting; the SDK retries.
- **Callbacks run on the reader thread** — keep them short.
