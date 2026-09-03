# VL53L4CD — introduction

The **VL53L4CD** is a STMicroelectronics single-zone Time-of-Flight ranging
sensor: one laser distance per measurement, up to ~1.3 m, with millimetre
resolution and a per-sample quality estimate. On the DEPZ sensor line the MCU
is a thin I2C **register bridge** (contract 10) — the full ST ULD 2.2.3
driver runs **on the host**, inside `depz_sensor_sdk.vl53l4` (`Vl53l4Cd`).

## What it does

- Single-target distance from ~1 mm to ~1300 mm, one `Vl53l4Measurement` per
  sample, at up to ~100 Hz (10 ms timing budget).
- Each measurement carries far more than distance: a **range status** (0 =
  valid, with `status_text` naming the failure otherwise), **sigma** (range
  std-dev estimate, mm), signal/ambient rates, SPAD count, and the sensor's
  own wrapping frame counter (`stream_count`).
- INT-driven **push streaming**: the sensor signals data-ready and the MCU
  ships the 17-byte result block to the host — one report per measurement, no
  reassembly.
- The full ULD configuration surface is exposed: timing budget and
  inter-measurement period, offset and crosstalk correction (with on-device
  calibration), signal/sigma limits and distance-window interrupts.

## When to use it

Use the VL53L4CD when you need one accurate, fast, narrow-beam distance —
presence detection, level sensing, proximity switching — in the dark or in
sunlight, where the SR04's wide ultrasonic cone is too blunt. It is a
**single zone**, not an imager: for a depth image use the VL53L8, and for
orientation use the BNO086.

## Key concepts

- **Host-side ULD** — configuration and result decoding are pure Python
  (`vl53l4.uld`), a faithful port of ST's ULD 2.2.3. The device is just a
  register bridge.
- **`init()` is light** — the VL53L4CD carries its own firmware, so there is
  no blob download (unlike the VL53L8). `init()` runs the ULD boot sequence +
  VHV calibration in well under a second, and must run once per power-up.
- **Configure only when stopped** — while ranging, the INT-driven stream owns
  the register bank; config methods raise until you `stop_ranging()`.
- **Continuous vs autonomous** — `set_range_timing(budget_ms, inter_ms)`:
  budget 10–200 ms; `inter_ms=0` ranges back-to-back (continuous), `inter_ms >
  budget` is the autonomous low-power mode. Default is 50 ms continuous
  (~20 Hz).
- **XSHUT wipes the config** — `reset_sensor()` / `xshut()` power-cycle the
  sensor; every setting is lost and `init()` is required again.
- **`bridge_info()`** — MCU-side diagnostics (INT edge / error counters, pin
  levels, bus speed), safe to call even while streaming.

## See also

- [VL53L4CD user guide](guide.md) — hello-world, configuration, streaming,
  calibration, gotchas.
- [API reference](api.md) — `Vl53l4Cd`, `Vl53l4Measurement`, `Vl53l4Info`.
