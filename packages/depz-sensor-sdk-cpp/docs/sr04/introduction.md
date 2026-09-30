# SR04 — introduction

The **HC-SR04** is a low-cost ultrasonic ranging module. On the DEPZ sensor
line the firmware does the ranging: it fires the 40 kHz burst, times the echo,
and hands the host a timestamped **echo time** per measurement. The C++ SDK
opens the board, configures it, runs single shots or the measurement loop, and
converts each echo time into a distance in millimetres.

The SR04 is covered by both layers of the SDK:

- **The live class** `depz::Sr04` (`depz/device.hpp`) — open the board, set
  the sample period and echo decay, `measure_once()`, `start()` / `stop()` the
  loop, and receive `Sr04Measurement`s as callbacks or from a pull stream.
  It builds on the C SDK; see the [common guide](../guide.md#live-hardware)
  for the build and the rules every device shares.
- **The decode layer** (`depz/sr04.hpp`) — pure codecs: the `Sr04Data` wire
  payload, the configuration payloads and `distance_mm_from_echo()`, for
  bytes you read yourself (your own port loop, a recording).

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main() {
    auto sr04 = depz::open_sr04();
    auto m = sr04->measure_once();
    if (auto mm = m.distance_mm()) std::printf("%.0f mm\n", *mm);
    else                           std::puts("no echo");
}
```

## What it does

- Single-target distance from ~20 mm to ~4 m, one number per ping.
- The device measures **round-trip echo time in microseconds**
  (`Sr04Measurement::echo_time_us`); `distance_mm()` converts it to distance
  at 343 m/s, `distance_mm_at(air_temp_c)` at a temperature-compensated speed
  of sound.
- Two sources of samples: a host **single shot** (`measure_once()`) and the
  free-running measurement **loop** (`start()` / `stop()`), delivered to
  `on_measurement()` callbacks and `stream()` queues. An AUX **SYNC_IN** edge
  also triggers an unsolicited single shot, delivered the same way.

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
  `echo_time_us == 0xFFFF` (`depz::ECHO_TIMEOUT`); `valid()` is then `false`
  and `distance_mm()` returns `std::nullopt`. Always check the optional before
  trusting a distance.
- **Sample period** (µs, default `SAMPLE_PERIOD_DEFAULT_US` = 50000 → 20 Hz) —
  the minimum interval between measurement starts. The effective rate is
  auto-throttled by how long each echo window actually takes, so a read-back
  returns the *stored* value, not the realised rate.
- **Echo decay** (µs, `ECHO_DECAY_MIN_US`..`ECHO_DECAY_MAX_US` = 4000–65000) —
  the post-echo settle pause before the next ping; the device clamps
  out-of-range values, and `set_echo_decay_us()` returns the value in effect.
- **`from_loop`** — `true` for a sample from the running loop, `false` for a
  single shot (`measure_once()` or a SYNC_IN edge). On the wire this is the
  `source_cmd` byte: `0x37` loop, `0x36` single shot.
- **One measurement path at a time** — `measure_once()` while the loop runs
  throws `BusyError`.
- **Timestamps are device µs** — `sync_time()` + `to_host_time_us()` put them
  on the host clock.

The live class has been checked on a real SR04 (firmware
`APP_usonic_SR04_v0.97`): identity and temperature, single shots (~985 mm to
a wall), `BusyError` during the loop, 50 samples/s to a stream and a
callback at a 20 ms period.

## See also

- [SR04 user guide](guide.md) — open, configure, single shot and loop,
  callbacks vs streams, disconnects, record / replay, and the decode layer.
- [API reference](api.md) — `Sr04`, `Sr04Measurement`, `Sr04Data`,
  `distance_mm_from_echo`, the command/report IDs and the config codecs.
