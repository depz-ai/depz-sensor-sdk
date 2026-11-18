# SR04 — introduction

The **HC-SR04** is a low-cost ultrasonic ranging module. On the DEPZ sensor
line the firmware does the ranging: it fires the 40 kHz burst, times the echo,
and hands the host a timestamped **echo time** per measurement. The SDK turns
that into a distance in millimetres.

## What it does

- Single-target distance from ~20 mm to ~4 m, one number per ping.
- The device measures **round-trip echo time in microseconds**; the SDK
  converts it to distance at 343 m/s (or a temperature-compensated speed of
  sound if you pass the air temperature).
- Two ways to sample: a one-shot `measure_once()`, or a free-running
  measurement **loop** that streams samples at a configurable period.
- An AUX **SYNC_IN** edge can trigger an unsolicited single shot — those land
  in the same stream, tagged `source == "once"`.

## When to use it

Reach for the SR04 when you need a cheap, robust "is something in front of me
and roughly how far" signal: bin-full detection, obstacle stop, liquid level,
presence. It is a **single wide cone** (~15°), not an imager — for a depth
image use the VL53L8, and for orientation use the BNO086.

## Key concepts

- **`echo_time_us`** — the raw round-trip time. `distance_mm = echo_us ·
  c / 2000` with `c` in m/s. This is authoritative; distance is derived.
- **No-echo timeout** — when nothing returns, the device reports the sentinel
  `echo_time_us == 0xFFFF`. `Sr04Measurement.valid` is then `False` and
  `distance_mm` is `None`. Always check `valid` before trusting a distance.
- **Sample period** (µs, default 50000 → 20 Hz) — the minimum interval between
  measurement starts. The effective rate is auto-throttled by how long each
  echo window actually takes, so a read-back returns the *stored* value, not
  the realised rate.
- **Echo decay** (µs, 4000–65000) — the post-echo settle pause before the next
  ping; the device clamps out-of-range values silently.
- **`source`** — `"once"` for a host `measure_once()` or a SYNC_IN edge,
  `"loop"` for a sample from the running loop.

## See also

- [SR04 user guide](guide.md) — hello-world, configuration, streaming, gotchas.
- [API reference](../api.md) — `Sr04`, `Sr04Measurement`.
