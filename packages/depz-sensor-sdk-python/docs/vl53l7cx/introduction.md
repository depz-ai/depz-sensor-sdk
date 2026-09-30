# VL53L7CX — introduction

The **VL53L7CX** is STMicroelectronics' wide-angle multizone Time-of-Flight
sensor: an 8×8 (or 4×4) depth image over a **90° diagonal** field of view, up
to ~3.5 m. It is the same ULD family as the [VL53L8CX](../vl53l8cx/introduction.md)
— same host-side driver, same frames, same advanced features — on a board
that talks **I2C** instead of SPI (`APP_VL53L7` firmware, contract 11). In
the SDK it is `Vl53l7cx`, a subclass of `Vl53l8cx`.

One board firmware serves three sensors:

| class | sensor | field of view | firmware blob |
|---|---|---|---|
| `Vl53l5cx` | VL53L5CX | 63° | L5/L7 (ULD 2.0.1) |
| `Vl53l7cx` | VL53L7CX | 90° | L5/L7 (ULD 2.0.1) |
| `Vl53l7ch` | VL53L7CH | 90° | CH (VL53LMZ 2.0.16) + CNH histograms |

## What it does

Everything the VL53L8CX does — per-zone distance, status, signal, ambient,
sigma, up to four targets per zone, 4×4 at up to 60 Hz and 8×8 at up to 15 Hz,
the heatmap grid, crosstalk calibration, detection thresholds, the motion
indicator, sleep/wake — read the [VL53L8CX introduction](../vl53l8cx/introduction.md)
for those. What differs on this board:

- **1 Hz works** — the L5/L7 range and stream down to 1 Hz (the L8 needs ≥ 2 Hz).
- **Board commands** — `pin_ctrl()` drives the sensor's LPn / I2C_RST pins,
  `set_i2c_speed_khz()` re-times the sensor bus (100 kHz … 1 MHz, default
  1 MHz), `get_bridge_info()` reads the bridge counters.
- **`module_type`** — after `init()` the sensor itself says L5 (MZ) or L7
  (MZEVO); the SDK warns if that contradicts the class.
- **No deep sleep and no threshold auto-stop** on the L5CX / L7CX firmware
  (both exist on the [VL53L7CH](../vl53l7ch/introduction.md)).

## When to use it

Pick the VL53L7CX over the VL53L8CX for its **wider 90° view** (room corners,
close-range obstacle maps) or when you only have I2C. For a narrower beam at
the same price, the [VL53L5CX](../vl53l5cx/introduction.md) is the 63° part;
for raw return histograms, the [VL53L7CH](../vl53l7ch/introduction.md).

## Key concepts

- **Same API as the VL53L8CX** — `init()`, `set_resolution()`,
  `set_ranging_frequency_hz()`, `start_ranging()`, `frames()`, the advanced
  features: all inherited unchanged. This page and the [guide](guide.md)
  cover only what the L7 board adds.
- **`init()` is the slow step** — it downloads the ~84 KB sensor firmware
  (~1.4 s at the board's 1 MHz bus) and must run after every power-up or pin
  reset.
- **Pin control drops the sensor state** — `PinAction.LPN_OFF` and
  `PinAction.SOFT_CYCLE` stop the stream and wipe the firmware; run `init()`
  again.

## See also

- [VL53L7CX user guide](guide.md) — what the L7 board adds.
- [VL53L8CX user guide](../vl53l8cx/guide.md) — everything shared.
- [API reference](api.md) — `Vl53l7cx`, `Vl53l7Info`, `PinAction`, `I2cError`.
