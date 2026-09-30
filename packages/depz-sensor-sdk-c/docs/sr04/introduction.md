# SR04 — introduction

The **HC-SR04** is a low-cost ultrasonic ranging module. On the DEPZ sensor line
the firmware does the ranging: it fires the 40 kHz burst, times the echo, and
hands the host a timestamped **echo time** per measurement. This C SDK opens the
board, configures it, takes single shots or runs the measurement loop, and
converts the echo time to a distance in millimetres.

## What the SDK covers

The SR04 is the first board with a full **sensor class** in the C live layer
(`depz_sensor_io.h`), on top of its codecs (`depz_sensor_sdk.h`):

- **Open** — `depz_open_device()` finds, probes and opens it with the class
  attached (`depz_is_sr04()` confirms); `depz_sr04_open_link()` puts the class
  on any link (tests, replay).
- **Configure** — sample period (`depz_sr04_get/set_sample_period_us`) and
  echo decay (`depz_sr04_get/set_echo_decay_us`, which re-reads the value the
  device clamped to).
- **Measure** — a single shot (`depz_sr04_measure_once`), or the free-running
  loop (`depz_sr04_start` / `depz_sr04_stop`) delivered as callbacks
  (`depz_sr04_on_measurement`) and/or bounded drop-oldest streams
  (`depz_sr04_stream`).
- **Distance** — `depz_sr04_measurement_distance_mm()` (343 m/s) and
  `depz_sr04_measurement_distance_mm_at()` (temperature-compensated), both
  `false` for no echo.
- **Record / replay** — any session can be recorded to a `.depzrec` file and
  replayed without the board, byte-exact.
- **Decode layer** — if you own the transport yourself, the codecs are still
  there: `depz_sr04_unpack_data`, `depz_sr04_distance_mm`, the config
  pack/unpack helpers and the `depz_sr04_cmd` / `depz_sr04_rpt` opcode enums.

The class was checked on a real SR04 (firmware `APP_usonic_SR04_v0.97`) — see
[verified on hardware](guide.md#verified-on-hardware).

## When to use the SR04

Reach for the SR04 when you need a cheap, robust "is something in front of me and
roughly how far" signal: bin-full detection, obstacle stop, liquid level,
presence. It is a **single wide cone** (~15°), not an imager — for a depth image
use the VL53L8, and for orientation use the BNO086.

## Key concepts

- **`echo_time_us`** — the raw round-trip time, the authoritative value; distance
  is derived. `distance_mm = echo_us · c / 2000` with `c` in m/s.
- **No-echo timeout** — when nothing returns, the device reports the sentinel
  `echo_time_us == DEPZ_SR04_ECHO_TIMEOUT` (`0xFFFF`). `depz_sr04_valid()` is
  `false` for it and the distance helpers return `false`; always check before
  trusting a distance.
- **Sample period** (µs, default 50000 → 20 Hz) — the minimum interval between
  measurement starts. The effective rate is auto-throttled by how long each echo
  window takes, so a read-back returns the *stored* value, not the realised rate.
- **Echo decay** (µs, `DEPZ_SR04_ECHO_DECAY_MIN_US`..`_MAX_US`, i.e. 4000–65000)
  — the post-echo settle pause before the next ping; the device clamps
  out-of-range values silently, so the setter reads back what is in effect.
- **Single shot vs loop** — `from_loop` in a `depz_sr04_measurement` is `true`
  for a loop sample and `false` for a single shot (host `MEASURE_ONCE` or a
  SYNC_IN edge). One path at a time: a single shot while the loop runs is
  refused with `DEPZ_E_BUSY`.
- **Device time** — `timestamp_us` is the device clock; after
  `depz_device_sync_time()`, `depz_device_to_host_time_us()` maps it to host
  time.

## See also

- [SR04 user guide](guide.md) — open, configure, measure, stream, record and
  replay, the decode layer, gotchas.
- [API reference](api.md) — the SR04 class (`depz_sr04_*`) and the SR04 codecs.
- [Common guide](../guide.md#live-layer) — devices, errors, threading rules,
  streams and discovery shared by every board.
