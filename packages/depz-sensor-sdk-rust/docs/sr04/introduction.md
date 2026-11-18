# SR04 — introduction

The **HC-SR04** is a low-cost ultrasonic ranging module. On the DEPZ sensor
line the firmware does the ranging: it fires the 40 kHz burst, times the echo,
and hands the host a timestamped **echo time** per measurement. This crate
provides the byte-exact codecs for that exchange
([`protocol::sr04`](api.md)) — the commands you frame, the `RPT_DATA` sample
you decode, and the echo-time → distance conversion.

## What it does

- Single-target distance from ~20 mm to ~4 m, one number per ping.
- The device measures **round-trip echo time in microseconds**;
  [`distance_mm_from_echo`](api.md) converts it to distance at 343 m/s (or a
  temperature-compensated speed of sound if you pass the air temperature).
- Two sampling modes selected by command: a one-shot `MeasureOnce`, or a
  free-running measurement **loop** (`StartMeasurementLoop` /
  `StopMeasurementLoop`) that streams samples at a configurable period.
- An AUX **SYNC_IN** edge can trigger an unsolicited single shot — it arrives on
  the same report, tagged in the sample's `source_cmd` field.

## When to use it

Reach for the SR04 when you need a cheap, robust "is something in front of me
and roughly how far" signal: bin-full detection, obstacle stop, liquid level,
presence. It is a **single wide cone** (~15°), not an imager — for a depth
image use the [VL53L8](../vl53l8cx/introduction.md), and for orientation use the
[BNO086](../bno086/introduction.md).

## Key concepts

- **`echo_time_us`** — the raw round-trip time on the wire, authoritative.
  `distance_mm = echo_us · c / 2000` with `c` in m/s; distance is derived.
- **No-echo timeout** — when nothing returns, the device reports the sentinel
  [`ECHO_TIMEOUT`](api.md) (`echo_time_us == 0xFFFF`).
  [`Sr04Data::is_timeout`](api.md) is then `true` and `distance_mm_from_echo`
  returns `None`. Always check for the timeout before trusting a distance.
- **Sample period** (µs, [`SAMPLE_PERIOD_DEFAULT_US`](api.md) = 50000 → 20 Hz) —
  the minimum interval between measurement starts. The effective rate is
  auto-throttled by how long each echo window takes, so a read-back returns the
  *stored* value, not the realised rate (ERRATA E3).
- **Echo decay** (µs, [`ECHO_DECAY_MIN_US`](api.md)–[`ECHO_DECAY_MAX_US`](api.md),
  i.e. 4000–65000) — the post-echo settle pause before the next ping; the
  device clamps out-of-range values silently.
- **`source_cmd`** — `0x36` for a single shot (host `MeasureOnce` or a SYNC_IN
  edge), `0x37` for a sample from the running loop.

## See also

- [SR04 user guide](guide.md) — framing commands, decoding samples, distance,
  gotchas.
- [API reference](api.md) — `Sr04Cmd`, `Sr04Rpt`, `Sr04Data`,
  `distance_mm_from_echo`, and the period/decay codecs.
- [Common guide](../guide.md) — framing, CRC, discovery shared by every sensor.
