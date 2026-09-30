# BNO055 — introduction

The **BNO055** is a Bosch 9-axis absolute-orientation sensor: accelerometer,
gyroscope and magnetometer in one package, with Bosch's sensor fusion running
**on the chip**. It hands out a ready orientation (quaternion or Euler
angles), gravity and linear acceleration at 100 Hz, plus the raw sensors and
the chip temperature. On the DEPZ sensor line (`APP_BNO055` firmware, USB PID
`0xEE0A`) the MCU is a thin I2C **register bridge**: the host sets the sensor
up with register writes and the bridge streams one register block on a
timer.

## What this C++ SDK offers today

The BNO055 is covered by both layers of the SDK:

- **The live class** `depz::Bno055` (`depz/device.hpp`) — open the board
  (`open_bno055()`), `configure()` it in one call (CONFIG → units → axis
  remap → calibration profile → operating mode, NDOF by default, returning
  once the fusion runs), then stream scaled `Bno055Sample`s as callbacks or
  from a pull stream, or poll `read_sample()` / `read_quaternion()`. Around
  that: operating and power mode (every switch goes through CONFIG with the
  datasheet waits), units, axis remap and the datasheet placements,
  temperature source, system status and self-test, calibration status, the
  calibration profile and the soft-iron matrix, the page-1 sensor configs,
  the unique id and the motion interrupts, raw register access on pages 0 and
  1, `reset_sensor()` with the boot-settle poll, `restore_configuration()`
  and the bridge's own counters. It builds on the C SDK; see the
  [common guide](../guide.md#live-hardware) for the build and the rules every
  device shares.
- **The decode layer** (`depz::bno055`, `depz/bno055.hpp`) — pure codecs for
  bytes you read yourself: the bridge commands (`Bno055Cmd`,
  `pack_read_reg` / `pack_write_reg` / `pack_start_stream`), the bridge
  reports (`RegData`, `StreamData`, `Bno055Info`), the register codecs
  (`Units` with its scale factors, `CalibStatus`, the 22-byte
  `CalibrationProfile`, `AxisRemap` and the eight `PLACEMENTS`, the page-1
  `AccelConfig` / `GyroConfig` / `MagConfig`) and `decode_block()`, which
  pulls every channel a register window covers out as raw integers. The live
  class uses the same types.

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main() {
    auto imu = depz::open_bno055();              // the first BNO055 board
    imu->configure();                            // CONFIG -> units -> NDOF, fusion live on return
    imu->start_stream(10);                       // the full 46-byte block at 100 Hz
    auto samples = imu->samples();
    for (int i = 0; i < 100; ++i) {
        auto s = samples.next(std::chrono::milliseconds(500));
        if (!s || !s->euler) continue;
        const auto& e = *s->euler;               // heading, roll, pitch (degrees)
        std::printf("heading %6.1f  roll %6.1f  pitch %6.1f  calib sys %u\n",
                    e[0], e[1], e[2], unsigned(s->calibration->system));
    }
    imu->stop_stream();
}
```

The class does what the Python SDK's `Bno055` does, request for request, so
a session recorded by either SDK replays through the other byte for byte. It
is **verified on replays of real captures and live on a lab board** — see [record and replay](guide.md#record-and-replay).

## When to use it

Use the BNO055 when you want orientation without writing a fusion filter:
tilt, heading, a pointing device, a level. It is simpler to drive than the
[BNO086](../bno086/introduction.md) — no report subscriptions, one register
block — at the price of fewer outputs: no step counter, no activity
classifier, no per-report rates.

## Key concepts

- **Operating modes** — `OprMode::Config` (configuration only, every output
  reads zero), seven non-fusion modes (raw sensors only) and five fusion
  modes: `Imu` (accel + gyro, relative heading), `Compass`, `M4g`,
  `NdofFmcOff` and `Ndof` (all three sensors, absolute heading — the
  default).
- **Configure in CONFIG** — most settings (units, axis remap, calibration
  profile, power mode, the page-1 configs) only take in CONFIG mode; the
  sensor silently ignores them elsewhere, and it ignores a direct switch from
  one operating mode to another. The typed setters go to CONFIG and back for
  you; raw `write_registers()` does not.
- **Every CONFIG round trip restarts the fusion** — the fusion outputs read
  zero for ~70 ms after each switch out of CONFIG. `configure()` and
  `set_operation_mode()` wait for the first non-zero quaternion; reading the
  calibration profile or changing units costs another gap.
- **The stream is a timer** — the bridge reads one register block every
  `period_ms` and pushes it with a microsecond MCU timestamp. The sensor
  fuses at 100 Hz, so 10 ms is the useful floor. (The sensor firmware on
  these boards, 03.11, has no data-ready interrupt; the INT trigger serves
  the motion interrupts only.)
- **Samples are scaled, channels optional** — `Bno055Sample` carries each
  channel as a `std::optional`, present when the block covered it, scaled by
  the units in force when the stream started. Linear acceleration and
  gravity are **always m/s²**, whatever the accel unit says.
- **Two register pages** — the sensor configs, interrupt setup and the
  unique id live on page 1. The class selects page 1 around each access and
  always goes back to page 0; it throws `ArgumentError` while a stream runs,
  because the bridge would stream the wrong page.
- **Calibration is continuous** — the fusion calibrates in the background;
  `calibration_status()` says how far (0..3 per sensor), and a stored
  profile restored with `configure()` gives it a head start after a power
  cycle.

## See also

- [BNO055 user guide](guide.md) — open and configure, the sample, streaming,
  modes, units, axis remap, calibration, status, page 1, reset and bridge
  diagnostics, record / replay, and the decode layer.
- [API reference](api.md) — `depz/bno055.hpp` and the live class from
  `depz/device.hpp`.
