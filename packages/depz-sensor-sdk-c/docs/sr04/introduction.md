# SR04 — introduction

The **HC-SR04** is a low-cost ultrasonic ranging module. On the DEPZ sensor line
the firmware does the ranging: it fires the 40 kHz burst, times the echo, and
hands the host a timestamped **echo time** per measurement. This C SDK decodes
that wire data into a `depz_sr04_data` and converts the echo time to a distance
in millimetres.

## What the SDK covers

This is a **codec layer**: it decodes the SR04 reports and encodes its config
commands. It does not open the serial port or run a measurement loop for you —
you own the transport and call these pure functions on the bytes.

- Decode the measurement report (`DEPZ_SR04_RPT_DATA`) into `depz_sr04_data`
  (`depz_sr04_unpack_data`).
- Convert `echo_time_us` → distance with `depz_sr04_distance_mm()` (343 m/s, or
  a temperature-compensated speed of sound).
- Encode the two config setters — sample period (`depz_sr04_pack_sample_period`)
  and echo decay (`depz_sr04_pack_echo_decay`) — and decode their read-backs.
- Command/report opcodes are the `depz_sr04_cmd` / `depz_sr04_rpt` enums.

## When to use the SR04

Reach for the SR04 when you need a cheap, robust "is something in front of me and
roughly how far" signal: bin-full detection, obstacle stop, liquid level,
presence. It is a **single wide cone** (~15°), not an imager — for a depth image
use the VL53L8, and for orientation use the BNO086.

## Key concepts

- **`echo_time_us`** — the raw round-trip time, the authoritative value; distance
  is derived. `distance_mm = echo_us · c / 2000` with `c` in m/s.
- **No-echo timeout** — when nothing returns, the device reports the sentinel
  `echo_time_us == DEPZ_SR04_ECHO_TIMEOUT` (`0xFFFF`).
  `depz_sr04_distance_mm()` returns `false` for it; always check the return
  before trusting the output distance.
- **Sample period** (µs, default 50000 → 20 Hz) — the minimum interval between
  measurement starts. The effective rate is auto-throttled by how long each echo
  window takes, so a read-back returns the *stored* value, not the realised rate.
- **Echo decay** (µs, `DEPZ_SR04_ECHO_DECAY_MIN_US`..`_MAX_US`, i.e. 4000–65000)
  — the post-echo settle pause before the next ping; the device clamps
  out-of-range values silently.
- **`source_cmd`** — `0x36` for a single shot (host `MEASURE_ONCE` or a SYNC_IN
  edge), `0x37` for a sample from the running loop.

## See also

- [SR04 user guide](guide.md) — decode a report, config codecs, echo→distance,
  gotchas.
- [API reference](api.md) — `depz_sr04_data`, `depz_sr04_distance_mm`, and the
  SR04 codecs.
