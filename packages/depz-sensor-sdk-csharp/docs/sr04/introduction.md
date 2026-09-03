# SR04 — introduction

The **HC-SR04** is a low-cost ultrasonic ranging module. On the DEPZ sensor
line the firmware does the ranging: it fires the 40 kHz burst, times the echo,
and hands the host a timestamped **echo time** per measurement. The C# SDK
decodes that report and turns the echo time into a distance in millimetres
(`Depz.Sensor.Protocol.Sr04`).

## What it does

- Single-target distance from ~20 mm to ~4 m, one number per ping.
- The device measures **round-trip echo time in microseconds** (`Sr04Data.EchoTimeUs`);
  the SDK converts it to distance at 343 m/s — or a temperature-compensated
  speed of sound if you pass the air temperature.
- Two firmware sampling modes: a one-shot `MeasureOnce` (`0x36`), or a
  free-running measurement **loop** (`0x37`) that streams samples at a
  configurable period. A sample's `SourceCmd` says which produced it.
- The SDK's job is **decode + encode**: unpack the `Data` report, and pack the
  sample-period / echo-decay configuration payloads.

## When to use it

Reach for the SR04 when you need a cheap, robust "is something in front of me
and roughly how far" signal: bin-full detection, obstacle stop, liquid level,
presence. It is a **single wide cone** (~15°), not an imager — for a depth
image use the VL53L8, and for orientation use the BNO086.

## Key concepts

- **`EchoTimeUs`** — the raw round-trip time. `distance_mm = echoUs · c / 2000`
  with `c` in m/s. This is authoritative; distance is derived
  (`Sr04.DistanceMmFromEcho`).
- **No-echo timeout** — when nothing returns, the device reports the sentinel
  `EchoTimeUs == 0xFFFF` (`Sr04.EchoTimeout`). `DistanceMmFromEcho` then returns
  `null`. Always check for `null` before trusting a distance.
- **Sample period** (µs, default `Sr04.SamplePeriodDefaultUs` = 50000 → 20 Hz) —
  the minimum interval between measurement starts. The *effective* rate is
  auto-throttled by how long each echo window takes, so a read-back returns the
  stored value, not the realised rate.
- **Echo decay** (µs, `EchoDecayMinUs`..`EchoDecayMaxUs` = 4000..65000) — the
  post-echo settle pause before the next ping; the device clamps out-of-range
  values silently.
- **`SourceCmd`** — `0x36` for a one-shot `MeasureOnce` (or a SYNC_IN edge),
  `0x37` for a sample from the running loop.

## See also

- [SR04 user guide](guide.md) — decoding, distance, configuration payloads,
  temperature compensation, gotchas.
- [API reference](api.md) — `Sr04`, `Sr04Data`, `Sr04Cmd`, `Sr04Rpt`.
