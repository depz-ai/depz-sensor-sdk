# SR04 — introduction

The **HC-SR04** is a low-cost ultrasonic ranging module. On the DEPZ sensor
line the firmware does the ranging: it fires the 40 kHz burst, times the echo,
and hands the host a timestamped **echo time** per measurement. The C++ SDK
decodes that wire payload and converts the echo time into a distance in
millimetres.

This is the **decode layer** (`depz/sr04.hpp`) — pure codecs, no I/O. You feed
it the payload bytes of an SR04 report; it gives you an `Sr04Data` and a
distance. Reading the port and sending commands is your application's job (see
the [common guide](../guide.md)).

## What it does

- Single-target distance from ~20 mm to ~4 m, one number per ping.
- The device measures **round-trip echo time in microseconds**
  (`Sr04Data::echo_time_us`); `distance_mm_from_echo()` converts it to distance
  at 343 m/s, or at a temperature-compensated speed of sound if you pass the air
  temperature.
- Two sources of samples show up on the same report: a host one-shot
  (`MeasureOnce`, `source_cmd == 0x36`) and the free-running measurement **loop**
  (`StartMeasurementLoop`, `source_cmd == 0x37`). An AUX **SYNC_IN** edge also
  triggers an unsolicited single shot (still `source_cmd == 0x36`).

## When to use it

Reach for the SR04 when you need a cheap, robust "is something in front of me
and roughly how far" signal: bin-full detection, obstacle stop, liquid level,
presence. It is a **single wide cone** (~15°), not an imager — for a depth image
use the [VL53L8](../vl53l8cx/introduction.md), and for orientation use the
[BNO086](../bno086/introduction.md).

## Key concepts

- **`echo_time_us`** — the raw round-trip time. `distance_mm = echo_us · c /
  2000` with `c` in m/s. This is authoritative; distance is derived.
- **No-echo timeout** — when nothing returns, the device reports the sentinel
  `echo_time_us == 0xFFFF` (`depz::ECHO_TIMEOUT`). `distance_mm_from_echo()` then
  returns `std::nullopt`. Always check for the empty optional before trusting a
  distance.
- **Sample period** (µs, default `SAMPLE_PERIOD_DEFAULT_US` = 50000 → 20 Hz) —
  the minimum interval between measurement starts. The effective rate is
  auto-throttled by how long each echo window actually takes, so a read-back
  returns the *stored* value, not the realised rate.
- **Echo decay** (µs, `ECHO_DECAY_MIN_US`..`ECHO_DECAY_MAX_US` = 4000–65000) —
  the post-echo settle pause before the next ping; the device clamps
  out-of-range values silently.
- **`source_cmd`** — `0x36` for a host `MeasureOnce` or a SYNC_IN edge, `0x37`
  for a sample from the running loop.

## See also

- [SR04 user guide](guide.md) — decode a report, configuration codecs, distance,
  gotchas.
- [API reference](api.md) — `Sr04Data`, `distance_mm_from_echo`, the command/
  report IDs and the config codecs.
