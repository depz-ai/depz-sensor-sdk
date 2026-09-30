# VL53L5CX — introduction

The **VL53L5CX** is STMicroelectronics' original multizone Time-of-Flight
sensor: an 8×8 (or 4×4) depth image over a **63° diagonal** field of view, up
to ~4 m. On the DEPZ line it sits on the same I2C board as the
[VL53L7CX](../vl53l7cx/introduction.md) (`APP_VL53L7` firmware, USB PID
`0xED48`) and runs the same sensor firmware (ST ULD 2.0.1). It does what the
VL53L7CX does — per-zone distances, statuses and rates, 4×4 up to 60 Hz and
8×8 up to 15 Hz, 1 Hz supported — with a **narrower, longer-reaching beam**.

## What the SDK covers

Everything on the [VL53L7CX introduction](../vl53l7cx/introduction.md)
applies unchanged — the three L5/L7 boards run the same live class and share
one header section:

- **the live multizone class** (`depz_vl53l8_*`) with the model
  `DEPZ_VL53L8_MODEL_L5CX`: `depz_open_device()` picks it for the USB PID
  `0xED48` (or a device name with `VL53L5CX` in it); `init()` downloads the
  L5/L7 sensor firmware, then configuration and ranging work as on the
  [VL53L8CX](../vl53l8cx/guide.md);
- **the board commands** — `depz_vl53l7_bridge_info()`,
  `depz_vl53l7_set_i2c_speed_khz()`, `depz_vl53l7_pin_ctrl()`;
- **the decode layer** — the bridge codecs (`depz_vl53l7_pack_*`,
  `depz_vl53l7_unpack_info`), the class-resolution rule
  (`depz_vl53l7_resolve_model()` returns `DEPZ_VL53L7_MODEL_L5CX` for this
  board) and the frame decode (`depz_vl53l7_decode_frame()`).

Two real VL53L5CX captures (8×8 and 4×4 at 15 Hz) replay strictly through the
class, firmware download included. A VL53L5CX board has not been run live
with the C class yet; it runs the same code as the verified
[VL53L7CH](../vl53l7cx/guide.md#verified-on-hardware) with the L5/L7
firmware blob.

## When to use it

Pick the VL53L5CX when you want a multizone depth image aimed at something
rather than a wide room view: a doorway, a conveyor, a hand in front of a
screen. For the wide view take the VL53L7CX; for raw histograms the
[VL53L7CH](../vl53l7ch/introduction.md).

## Key concepts

- **Same class, same frame** — an L5CX runs the VL53L8 class and fills the
  same `depz_vl53l8_live_frame`; there is nothing L5-specific to call or to
  decode.
- **The model comes from the board, not the silicon** — the USB PID or the
  device name says L5CX. The sensor reports its own module type
  (`depz_vl53l8_module_type()`, 0 = MZ for the L5) only after `init()`, so a
  board stamped L7 but carrying an L5 still ranges; compare the two if the
  difference matters to you.
- **ULD 2.0.1 limits** — no deep sleep and no detection-threshold auto-stop
  (`DEPZ_E_ARG`), as on the VL53L7CX.

## See also

- [VL53L5CX user guide](guide.md)
- [VL53L7CX guide](../vl53l7cx/guide.md) — open, the L5/L7 differences, board
  commands, record and replay, the decode layer (everything shared).
- [VL53L8CX guide](../vl53l8cx/guide.md) — the class itself.
- [API reference](api.md)
