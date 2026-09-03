# SR04 — introduction

The **HC-SR04** is a low-cost ultrasonic ranging module. On the DEPZ sensor
line the firmware does the ranging: it fires the 40 kHz burst, times the echo,
and hands the host a timestamped **echo time** per measurement. The Java SDK's
`ai.depz.sensor.protocol.Sr04` codecs turn that wire data into a distance in
millimetres, and build the configuration/command packets that drive it.

## What it does

- Single-target distance from ~20 mm to ~4 m, one number per ping.
- The device measures **round-trip echo time in microseconds**; the SDK
  converts it to distance at 343 m/s (or a temperature-compensated speed of
  sound if you pass the air temperature).
- Two ways to sample, both driven by the command codecs: a one-shot
  `MEASURE_ONCE`, or a free-running measurement **loop**
  (`START_MEASUREMENT_LOOP` / `STOP_MEASUREMENT_LOOP`) at a configurable period.
- An AUX **SYNC_IN** edge can trigger an unsolicited single shot — it arrives on
  the same `RPT` stream, distinguished by `sourceCmd`.

## When to use it

Reach for the SR04 when you need a cheap, robust "is something in front of me
and roughly how far" signal: bin-full detection, obstacle stop, liquid level,
presence. It is a **single wide cone** (~15°), not an imager — for a depth image
use the VL53L8, and for orientation use the BNO086.

## Key concepts

- **`echoTimeUs`** — the raw round-trip time. `distance_mm = echoUs · c / 2000`
  with `c` in m/s. This is authoritative; distance is derived by
  `Sr04.distanceMmFromEcho`.
- **No-echo timeout** — when nothing returns, the device reports the sentinel
  `echoTimeUs == Sr04.ECHO_TIMEOUT` (`0xFFFF`). `distanceMmFromEcho` then returns
  `null`. Always check for `null` before trusting a distance.
- **Sample period** (µs, default `SAMPLE_PERIOD_DEFAULT_US` = 50000 → 20 Hz) —
  the minimum interval between measurement starts, a u32 packed by
  `Sr04.packSamplePeriod`. The effective rate is auto-throttled by how long each
  echo window takes, so a read-back returns the *stored* value, not the realised
  rate.
- **Echo decay** (µs, `ECHO_DECAY_MIN_US`..`ECHO_DECAY_MAX_US` = 4000..65000) —
  the post-echo settle pause; the device clamps out-of-range values silently.
- **`sourceCmd`** — `0x36` (`MEASURE_ONCE`) for a host single shot or a SYNC_IN
  edge, `0x37` (`START_MEASUREMENT_LOOP`) for a sample from the running loop.

## See also

- [SR04 user guide](guide.md) — decode a measurement, configure, convert
  distance, gotchas.
- [API reference](api.md) — `Sr04`, `Sr04.Sr04Data`, `Sr04.Sr04Cmd`,
  `Sr04.Sr04Rpt`.
