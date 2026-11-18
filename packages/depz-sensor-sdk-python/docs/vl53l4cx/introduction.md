# VL53L4CX — introduction

The **VL53L4CX** is ST's extended-range sibling of the VL53L4CD: up to ~6 m, histogram only. It is one of the six VL53L 1D boards that
share one firmware (`APP_VL53L0_4`, contract 12): the MCU is a thin I2C
**register bridge** that knows no sensor, and every ST driver runs **on the
host**, inside `depz_sensor_sdk.vl53lx` (`Vl53l4cx`).

## What it does

- Single-zone distance up to **~6 m**, one `Vl53lxMeasurement` per
  frame, with a range status, sigma (std-dev estimate), signal and ambient
  rates.
- INT-driven **push streaming**: the MCU reads the result block on each
  data-ready edge and ships it with a microsecond timestamp.
- One driver on this board: `histogram`.

| driver | what it is | modes | timing budget |
|---|---|---|---|
| `histogram` | ST's Bare Driver: the die hands the host 24 photon-count bins per frame and the SDK finds **up to four targets** in them | `medium`, `long` — no `short` on this die | any 2–200 ms (default 33 ms) |

## When to use it

Use it for the widest range on a small board (up to ~6 m), with several targets per frame. If you need offset/crosstalk calibration or thresholds, run it on the VL53L4CD's light driver instead: `init(product="VL53L4CD")` (reach drops to ~1.4 m).

## Key concepts

- **Product × driver kind** — `init(driver)` binds the pair; the product comes
  from the board's device name (the class fixes it). `supports(group)` tells
  what the pair can do — ask it before offering a setting.
- **`configure()` before every run** — it re-initialises the sensor and
  applies budget, period, mode and stored calibrations. That is the only way
  to know what the configuration registers hold.
- **Same measurement for every product** — `Vl53lxMeasurement`; on the
  histogram driver `targets` lists every return, strongest first.
- **Configure only when stopped** — while ranging the stream owns the bus;
  configuration calls raise until `stop_ranging()`.
- **Borrowing a driver** — `init(product="VL53L4CD")` loads the VL53L4CD ULD on this die: single target, ~1.4 m, full calibrations and thresholds. `notes()` says so.
- **Same model id as the VL53L4CD** (0xEBAA) — the device name tells them apart.

## See also

- [VL53L4CX user guide](guide.md) — open, configure, stream, the
  measurement, calibration, gotchas.
- [API reference](api.md) — `Vl53l4cx`, the family class `Vl53lx`,
  `Vl53lxMeasurement`, `Vl53lxInfo`.
