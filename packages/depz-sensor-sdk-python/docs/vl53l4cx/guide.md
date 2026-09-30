# VL53L4CX — user guide

Hands-on guide to the `Vl53l4cx` device class. For what the sensor is and
its concepts, read the [introduction](introduction.md); for exact signatures
see the [API reference](api.md).

## Contents

- [Open and initialize](#open-and-initialize)
- [Hello-world: live distance](#hello-world-live-distance)
- [Configure](#configure)
- [Streaming and single shot](#streaming-and-single-shot)
- [The measurement](#the-measurement)
- [Several targets (histogram driver)](#several-targets-histogram-driver)
- [What each driver supports](#what-each-driver-supports)
- [Reset and bridge diagnostics](#reset-and-bridge-diagnostics)
- [Gotchas](#gotchas)

## Open and initialize

```python
from depz_sensor_sdk import Vl53l4cx, open_device

dev = open_device("/dev/ttyACM0")   # returns a Vl53l4cx (from the board name)
assert isinstance(dev, Vl53l4cx)
dev.init()                          # driver "histogram"
print(dev.identify()["driver_class"], dev.notes())
```

`open_device()` picks the class from the board's USB id or its device name
(`… VL53L4CX USB v2.1 …`) — every 1D board runs the same firmware, so the
firmware name cannot tell them apart. `identify()` returns everything about
the bound pair: product, driver, model id, reach, supported groups, modes and
budgets.

To run the die on the VL53L4CD's light driver (1.2 m, calibrations,
thresholds), borrow it — `notes()` then says the driver is borrowed:

```python
dev.init(product="VL53L4CD")
print(dev.notes())
```

## Hello-world: live distance

```python
from depz_sensor_sdk import Vl53l4cx, open_device

with open_device("/dev/ttyACM0") as dev:
    dev.init()
    dev.configure(budget_ms=33, mode="medium")
    dev.start_ranging()
    for m in dev.measurements():     # blocks; Ctrl+C to stop
        if m.plottable:
            print(f"{m.distance_mm:5d} mm  sigma {m.sigma_mm:.1f}")
        else:
            print(m.status_text)
```

## Configure

```python
dev.configure(budget_ms=33, inter_ms=0, mode="medium")   # re-init + apply
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

## Several targets (histogram driver)

```python
from depz_sensor_sdk.vl53lx import plot_distances, primary_target

dev.init("histogram")
dev.configure(budget_ms=33, mode="medium")
dev.start_ranging()
m = dev.get_measurement()
for t in m.targets:                      # strongest first, up to four
    print(t.distance_mm, t.status_text, t.signal_kcps)
print(plot_distances(m))                 # what a chart should draw
print(primary_target(m))                 # what a single readout should show
b = m.bins                               # the raw 24-bin histogram
dev.stop_ranging()
```

On the histogram driver `status == 0` is not the test: statuses 0, 6 (first
frame, no predecessor) and 11 (merged pulse) are all usable — use
`m.plottable`, `plot_distances()` or `primary_target()`. The driver steps an
A/B frame-pair state per frame, so every streamed frame is decoded exactly
once, in order.

## What each driver supports

| driver | capability groups |
|---|---|
| `histogram` | timing budget / inter-measurement period, named ranging modes |

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
- **Don't use the `short` preset on the L4CX** — at any distance its frames alternate: one carries the true distance but status 7, the next a wrong one (flat wall, 2026-09-28: −341 mm at 0.15 and 0.3 m, −156 mm at 0.6 m; at 1.0 m the true 1008 mm comes flagged status 4 and the other frame reads 238 mm). A plottable filter drops them all. Same on firmware v0.23 and v0.24 and with the firmware repo's own `vl53_tool.py`; the cause is not known yet. `medium` and `long` are clean at all four distances — use them.
- **Callbacks run on the reader thread** — keep them short.
