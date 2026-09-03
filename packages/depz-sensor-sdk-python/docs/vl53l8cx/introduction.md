# VL53L8CX — introduction

The **VL53L8CX** is a STMicroelectronics multizone Time-of-Flight ranging
sensor: a tiny laser depth **imager** that returns a 4×4 or 8×8 grid of
distances up to ~4 m. On the DEPZ sensor line the MCU is a thin SPI **register
bridge** — the full ST ULD (Ultra-Lite Driver) runs **on the host**, inside
`depz_sensor_sdk.vl53l8` (`Vl53l8Cx`).

This is the base ToF sensor. Its superset — the **VL53L8CH**, which adds Compact
Network Histograms — has its own [introduction](../vl53l8ch/introduction.md) and
[guide](../vl53l8ch/guide.md); everything here applies to it unchanged.

## What it does

- Per-zone distance over a **16-zone (4×4)** or **64-zone (8×8)** grid, at up
  to 60 Hz (4×4) / 15 Hz (8×8).
- Each frame carries far more than distance: per-zone target status, number of
  targets, signal and ambient rate (kcps/SPAD), range sigma, reflectance, plus
  a per-frame silicon temperature.
- INT-driven **push streaming**: the sensor signals data-ready and the MCU
  ships each frame to the host, reassembled from ≤1528-byte chunks.
- Advanced ST features are exposed: power modes, crosstalk margin and on-device
  crosstalk calibration, calibration-data save/restore, per-zone detection
  thresholds (interrupt-on-threshold), and the motion indicator.

## CX vs CH

Two silicon/firmware variants share the same register protocol:

- **`Vl53l8Cx`** — the base ranging sensor (this page). `init()` downloads the
  CX sensor firmware blob. Ships as the dev default.
- **`Vl53l8Ch`** — a superset that additionally emits **Compact Network
  Histograms (CNH)**, and carries its own production USB PID. `Vl53l8Ch`
  inherits every `Vl53l8Cx` method and adds `configure_cnh()`. See the
  [VL53L8CH docs](../vl53l8ch/introduction.md).

`open_device()` returns `Vl53l8Ch` when the USB model hint says so, otherwise
`Vl53l8Cx`; both are instances of `Vl53l8` (the CX base). The blob variant is
fixed by the class, so you never pass a variant string to `init()`.

## When to use it

Use the VL53L8 when you need a **depth image** — gesture/zone occupancy, small
obstacle maps, people counting, hand tracking — in a package far smaller and
cheaper than a stereo camera, indoors or in the dark. For a single distance the
SR04 is simpler; for orientation use the BNO086. Reach for **CH/CNH** only when
you need raw return histograms (multi-return analysis, material work).

## Key concepts

- **Host-side ULD** — configuration and frame parsing are pure Python
  (`vl53l8.uld`), a faithful port of the ST driver. The device is just a
  register bridge.
- **`init()` is heavy** — it downloads an ~84 KB sensor firmware blob every
  power-up (tens of seconds over CDC). Show progress; it is not a hang.
- **≥ 2 Hz** — below 2 Hz the sensor never enters its ranging loop and streams
  nothing; the SDK rejects it.
- **Configure only when stopped** — while ranging, the stream owns the register
  bank; config methods raise until you `stop_ranging()`.
- **`Vl53l8Frame`** — one parsed frame. Arrays are sized to the active
  resolution (16 or 64), row-major; `frame.grid("distance_mm")` reshapes to a
  (4,4)/(8,8) NumPy array. `target_status` 5/9 = valid, 255 = no target.

## See also

- [VL53L8CX user guide](guide.md) — hello-world, configuration, frame fields,
  advanced features, gotchas.
- [VL53L8CH docs](../vl53l8ch/introduction.md) — the CNH superset.
- [API reference](api.md) — `Vl53l8Cx`, `Vl53l8Frame`, and the ToF constants.
