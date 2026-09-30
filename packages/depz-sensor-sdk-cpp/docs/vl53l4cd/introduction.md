# VL53L4CD — introduction

The **VL53L4CD** is ST's single-zone Time-of-Flight ranger: one precise
optical distance per measurement, ~1 mm resolution out to ~1.3 m. On the DEPZ
sensor line the MCU is a **thin I2C register bridge**: it owns only the I2C
bus, the XSHUT and INT pins and one streaming loop, while the ST Ultra-Lite
Driver (ULD 2.2.3) runs **on the host** — in the C SDK, register for register.
The C++ SDK opens the board, initialises and configures the sensor, takes
single shots or streams one measurement per INT edge, and hands each result
over decoded.

The VL53L4CD is covered by both layers of the SDK:

- **The live class** `depz::Vl53l4cd` (`depz/device.hpp`) — open the board,
  `init()` the sensor, set range timing, offset, crosstalk and thresholds,
  calibrate, `measure_once()`, `start_ranging()` / `stop_ranging()`, and
  receive `Vl53l4cdMeasurement`s as callbacks or from a pull stream. It builds
  on the C SDK; see the [common guide](../guide.md#live-hardware) for the
  build and the rules every device shares.
- **The decode layer** (`depz::vl53l4`, `depz/vl53l4.hpp`) — pure codecs: the
  bridge command / report payloads, the 17-byte result-block decode, the
  range-timing register math, the tuning-word codecs and the default
  configuration block, for bytes you read yourself (your own port loop, a
  recording). Byte-exact with the other SDKs via the shared golden vectors.

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main() {
    auto tof = depz::open_vl53l4cd();
    tof->init();                               // ULD sensor_init + VHV, ~60 ms
    auto m = tof->measure_once();
    if (m.valid()) std::printf("%d mm\n", m.r.distance_mm);
    else           std::printf("%s\n", m.status_text().c_str());
}
```

## What it does

- Single-zone ranging: one 17-byte result block per measurement — distance,
  range status, sigma (the sensor's own error estimate), signal and ambient
  rates, SPAD count, a frame counter. No zone grids, no firmware download.
- **Two ways to measure**: a host **single shot** (`measure_once()` — start,
  wait data-ready, read, stop) and **ranging** (`start_ranging()`): the
  sensor measures back to back, and on each INT edge the MCU reads the result
  block and pushes it to the host, delivered to `on_measurement()` callbacks
  and `measurements()` streams.
- **The ULD's configuration surface**: range timing (budget and
  inter-measurement period), offset, crosstalk compensation, a distance
  detection window for INT, signal and sigma quality limits, offset and
  crosstalk calibration against a target, a temperature re-calibration.
- **The bridge's own view**: `bridge_info()` — model id, pin levels, INT
  edges, skipped stream slots, I2C errors, the programmed bus speed — safe to
  call while ranging. `read_reg()` / `write_reg()` reach any sensor register.

## When to use it

Reach for the VL53L4CD when you need precise single-point optical distance —
level sensing, presence, close-range positioning — without the
[SR04](../sr04/introduction.md)'s wide ultrasonic cone or the
[VL53L8](../vl53l8cx/introduction.md)'s multizone depth image.

## Key concepts

- **Register bridge** — every ULD call becomes register reads and writes over
  USB; the board does nothing on its own until the host has initialised the
  sensor. Register *contents* are big-endian sensor bytes passed through
  untouched; the wire fields (`addr`, `len`, timestamps) are little-endian.
- **`init()` first** — writes the ULD default configuration at 400 kHz,
  re-times the bus (1 MHz by default), runs the VHV calibration (a sensor
  self-tuning step) and leaves a 50 ms budget in continuous mode. ~60 ms on a real
  board. Needed again after every XSHUT power-cycle: the sensor keeps none of
  its configuration.
- **Range status** — `range_status == 0` means valid; anything else is a real
  reading with a reason ("sigma above threshold", "signal below threshold",
  "wrapped target", …) — `valid()` and `status_text()` say which. Check it
  before trusting a distance.
- **Range timing** — the *budget* (10..200 ms) is how long one measurement
  integrates; the *inter-measurement period* is 0 for continuous mode (back
  to back) or a value above the budget for autonomous low power (the sensor
  sleeps in between). Values from 1 to the budget are refused.
- **The stream owns the register bank** — while ranging, the MCU reads the
  sensor on every INT edge, so configuration throws `ArgumentError` ("stop
  ranging first"); so does `measure_once()`. Stop, reconfigure, restart.
- **Timestamps are MCU µs** — at the INT edge for streamed measurements (the
  sensor event, not the I2C completion), at the read for a single shot.
  `sync_time()` + `to_host_time_us()` put them on the host clock.

The live class has been checked on a real VL53L4CD (board `TL7TKSLW8Z`,
firmware `APP_VL53L4_v0.83`, a wall at ~1.05 m): `measure_once()` 1042 mm
valid, `ArgumentError` for configuration during ranging, 33 frames/s at a
33 ms budget with a mean of 1047 mm and none dropped.

## See also

- [VL53L4CD user guide](guide.md) — open and init, configuration, single shot
  and ranging, callbacks vs streams, calibration, XSHUT, bridge diagnostics,
  record / replay, and the decode layer.
- [API reference](api.md) — `Vl53l4cd`, `Vl53l4cdMeasurement`,
  `DetectionThresholds`, the bridge codecs, `Vl53l4Result` and the ULD math.
