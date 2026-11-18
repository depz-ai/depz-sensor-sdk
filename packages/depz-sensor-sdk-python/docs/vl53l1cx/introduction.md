# VL53L1CX — introduction

The **VL53L1CX** is ST's long-distance single-zone ToF ranger (up to ~4 m) with a programmable region of interest. It is one of the six VL53L 1D boards that
share one firmware (`APP_VL53L0_4`, contract 12): the MCU is a thin I2C
**register bridge** that knows no sensor, and every ST driver runs **on the
host**, inside `depz_sensor_sdk.vl53lx` (`Vl53l1cx`).

## What it does

- Single-zone distance up to **~4 m**, one `Vl53lxMeasurement` per
  frame, with a range status, sigma (std-dev estimate), signal and ambient
  rates.
- INT-driven **push streaming**: the MCU reads the result block on each
  data-ready edge and ships it with a microsecond timestamp.
- Driver kinds on this board: `uld`, `histogram` (default `uld`).

| driver | what it is | modes | timing budget |
|---|---|---|---|
| `uld` | the VL53L1X Ultra Lite Driver: the die computes one distance per frame | `long` (default after init), `short` | only the tabulated budgets: 20, 33, 50, 100, 200, 500 ms in `long`; 15 ms is added in `short` |
| `histogram` | ST's Bare Driver: the die hands the host 24 photon-count bins per frame and the SDK finds **up to four targets** in them | `short`, `medium`, `long` | any 2–550 ms (default 33 ms) |

## When to use it

Use it for up to ~4 m of single-point ranging with a narrowable field of view (ROI), or run the histogram driver to see several targets at once (a glass pane and the wall behind it). Behind a cover glass prefer the VL53L1CB.

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
- **Region of interest** — on the light driver `set_roi(x, y)` narrows the SPAD window (4..16 each) and `set_roi_center()` moves it; a smaller ROI is a narrower beam.

## See also

- [VL53L1CX user guide](guide.md) — open, configure, stream, the
  measurement, calibration, gotchas.
- [API reference](api.md) — `Vl53l1cx`, the family class `Vl53lx`,
  `Vl53lxMeasurement`, `Vl53lxInfo`.
