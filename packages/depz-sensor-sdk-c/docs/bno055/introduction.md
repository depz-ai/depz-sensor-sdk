# BNO055 — introduction

The **BNO055** is a Bosch 9-axis absolute-orientation sensor: accelerometer,
gyroscope and magnetometer in one package, with Bosch's sensor fusion running
**on the chip**. It hands out a ready orientation (quaternion or Euler
angles), gravity and linear acceleration at 100 Hz, plus the raw sensors and
the chip temperature. On the DEPZ sensor line (`APP_BNO055` firmware, USB PID
`0xEE0A`) the MCU is a thin I2C **register bridge**: the host sets the sensor
up with plain register writes and the bridge streams one register block on a
timer.

Because the fusion runs on the chip, there is no driver to port: what the
host has to do — switch modes in the right order, wait the right times, keep
the register page straight, scale the numbers — is register logic, and this C
SDK does it for you in the **BNO055 sensor class** (`depz_bno055_*`).

## What the SDK covers

The BNO055 has a **sensor class** in the C live layer (`depz_sensor_io.h`),
the same register sequences as the Python SDK's `Bno055`:

- **Open** — `depz_open_device()` finds, probes and opens the board with the
  class attached; the device reports sensor type `bno055`
  (`DEPZ_SENSOR_BNO055`) and `depz_is_bno055()` is true for it.
  `depz_bno055_open_link(link, &dev)` attaches it to a link without a probe
  (tests, replay).
- **Configure in one call** — `depz_bno055_configure()`: CONFIG mode, the
  units, an optional axis remap and calibration profile, then the operating
  mode (NDOF for full fusion); in a fusion mode it returns once the fusion
  outputs are live. `depz_bno055_restore_configuration()` applies it again
  after a reset.
- **Modes and settings** — operating mode (through CONFIG, with the
  datasheet switch times and the fusion-start wait), power mode, units, axis
  remap and the eight datasheet placements, temperature source.
- **Samples** — poll a register block with `depz_bno055_read_sample()` (or
  just the quaternion), or stream one every `period_ms` as callbacks, a
  bounded stream or one sample at a time. Each `depz_bno055_sample` is
  already scaled to physical units, with flags saying which channels the
  block carried.
- **Calibration** — the calibration status, the 22-byte calibration profile
  (read and written through CONFIG and back), the soft-iron matrix.
- **Status and page 1** — system status and the built-in self-test; the
  page-1 accelerometer / gyroscope / magnetometer configs, the chip's unique
  id and the motion interrupts, with the page switched and always set back.
- **Board** — the bridge's info report (sensor ids, firmware revision, bus
  counters) and the sensor reset.
- **Record / replay** — any session records to a `.depzrec` file and replays
  byte-exact; captures made by the Python SDK replay through this class
  unchanged.
- **Decode layer** — if you own the transport: the bridge command / report
  codecs (`depz_bno055_pack_*` / `_unpack_*`), the unit, calibration,
  axis-remap and page-1 register codecs, and the register-window decode
  (`depz_bno055_decode_block()`).

**Status: verified on replays of real captures and live on a lab board**
(NDOF, 100 Hz stream, orientation followed by hand). The two committed BNO055 captures (recorded by the Python
SDK on `APP_BNO055_v0.12`, sensor firmware 03.11) replay strictly through the
class — see [record and replay](guide.md#record-and-replay-for-tests).

## When to use it

Use the BNO055 when you want orientation without writing a fusion filter:
tilt, heading, a pointing device, a level. It is simpler to drive than the
[BNO086](../bno086/introduction.md) — no report subscriptions, one register
block — at the price of fewer outputs: no step counter, no activity
classifier, no per-report rates.

## Key concepts

- **Operating modes** — `CONFIG` (configuration only, every output reads
  zero), seven non-fusion modes (raw sensors only) and five fusion modes:
  `IMU` (accel + gyro, relative heading), `COMPASS`, `M4G`, `NDOF_FMC_OFF` and
  `NDOF` (all three sensors, absolute heading). "Fusion" means the chip
  combines the three sensors into one orientation.
- **Settings only take in CONFIG** — units, axis remap, calibration profile,
  power mode: the sensor silently ignores them in any other mode, and it
  ignores a switch from one operating mode straight to another. The class's
  setters go to CONFIG and back for you.
- **Every trip through CONFIG restarts the fusion** — for ~70 ms afterwards
  the fusion outputs read zero (an all-zero quaternion means "not fusing").
  `configure()` and `set_operation_mode()` wait that out; reading the
  calibration profile or changing units costs another gap.
- **The stream is a timer** — the bridge reads one register block every
  `period_ms` and pushes it with a microsecond MCU timestamp. The sensor
  fuses at 100 Hz, so 10 ms is the useful floor. (The sensor firmware on
  these boards, 03.11, has no data-ready interrupt.)
- **Units are chosen once, then applied** — a stream is scaled by the units in
  effect when it started. Linear acceleration and gravity are **always m/s²**,
  whatever the accelerometer unit says.
- **Calibration runs by itself** — the chip calibrates continuously in the
  background (0..3 per sensor). Save the profile once all four read 3 and
  write it back after every power cycle.
- **Two register pages** — the sensor configs, interrupt setup and the
  unique id live on page 1. The class switches pages around each access, and
  refuses page 1 while a stream runs (the bridge would stream the wrong page).

## See also

- [BNO055 user guide](guide.md) — open and configure, samples and streaming,
  modes, units, axis remap, calibration, status, page 1, reset, record and
  replay, the decode layer, gotchas.
- [API reference](api.md) — the codecs and the sensor class.
