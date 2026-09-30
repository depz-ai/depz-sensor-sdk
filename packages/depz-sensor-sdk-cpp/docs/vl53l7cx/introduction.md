# VL53L7CX — introduction

The **VL53L7CX** is STMicroelectronics' wide-angle multizone Time-of-Flight
sensor: an 8×8 (or 4×4) depth image over a **90° diagonal** field of view, up
to ~3.5 m, 4×4 at up to 60 Hz and 8×8 at up to 15 Hz. It is the same ST ULD
family as the [VL53L8CX](../vl53l8cx/introduction.md) — same frames, same
per-zone outputs — on a board that talks **I2C** instead of SPI (the
`APP_VL53L7` firmware). The MCU is a thin register bridge; the ST driver runs
on the host.

One board firmware serves three sensors that differ only in the soldered part:

| sensor | field of view | USB PID | sensor firmware | `module_type` |
|---|---|---|---|---|
| [VL53L5CX](../vl53l5cx/introduction.md) | 63° | `0xED48` | L5/L7 (ULD 2.0.1) | 0 = MZ |
| **VL53L7CX** | 90° | `0xED49` | L5/L7 (ULD 2.0.1) | 1 = MZEVO |
| [VL53L7CH](../vl53l7ch/introduction.md) | 90° | `0xED4A` | CH (VL53LMZ 2.0.16) + CNH histograms | 1 = MZEVO |

## What this C++ SDK offers today

The VL53L7CX is covered by both layers of the SDK:

- **The live class** `depz::Vl53l8` (`depz/device.hpp`) — the same class as
  the [VL53L8CX](../vl53l8cx/introduction.md), with the model
  `Vl53l8Model::L7CX`. Open the board, `init()` the sensor (the ~84 KB
  sensor-firmware download included, ~1.3 s over I2C), set resolution,
  frequency, ranging mode, sharpener, target order, sleep / wake, crosstalk,
  detection thresholds and the motion indicator, `start_ranging()` /
  `stop_ranging()`, and receive `Vl53l8LiveFrame`s as callbacks or from a
  pull stream. Three methods exist for the I2C board only: `bridge_info()`,
  `set_i2c_speed_khz()` and `pin_ctrl()`. It builds on the C SDK; see the
  [common guide](../guide.md#live-hardware) for the build and the rules every
  device shares.
- **The decode layer** (`depz::vl53l7`, `depz/vl53l7.hpp`) — pure codecs for
  bytes you read yourself: the bridge commands (`Vl53l7Cmd`, the
  `pack_read_reg` / `pack_write_reg` / `pack_pin_ctrl` / `pack_set_i2c_speed`
  encoders, the `READ_MAX_LEN` / `WRITE_MAX_LEN` ceilings), the bridge
  reports (`Vl53l7Info::unpack`; frame chunks are the VL53L8 `FrameChunk` /
  `FrameReassembler`), the class-resolution rule (`resolve_model()`) and
  `decode_frame()`, which fills the same `vl53l8::Vl53l8Frame` with the L5/L7
  differences applied (footer id at `size − 4`, per-zone arrays trimmed to
  the resolution).

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main() {
    auto tof = depz::open_vl53l8();            // PID 0xED49 (or the device name) -> L7CX
    tof->init();                               // firmware download over I2C, ~1.3 s
    std::printf("module %d (1 = MZEVO, an L7)\n", tof->module_type().value_or(-1));
    tof->set_resolution(depz::vl53l8::RESOLUTION_8X8);
    tof->set_ranging_frequency_hz(15);
    tof->start_ranging();
    depz::Vl53l8LiveFrame f = tof->get_frame();
    std::printf("zone 27: %d mm\n", f.frame.distance_mm[27]);
    tof->stop_ranging();
}
```

The live class has been checked on a real board of this family (a VL53L7CH,
board `TXK5KAX6X4`, firmware `APP_VL53L7_v0.53`, a target at 0.6 m): model
and module type detected, `init()` in 1.29 s, 8×8 at 15 Hz ~602 mm, 1 Hz
~596 mm, the I2C bus re-timed from a requested 450 to 500 kHz, a soft cycle
followed by a fresh `init()`, and no I2C errors.

## When to use it

Pick the VL53L7CX over the VL53L8CX for its **wider 90° view** (room corners,
close-range obstacle maps) or when you only have I2C. For a narrower beam the
[VL53L5CX](../vl53l5cx/introduction.md) is the 63° part; for raw return
histograms the [VL53L7CH](../vl53l7ch/introduction.md).

## Key concepts

- **One class for the whole multizone family** — `depz::Vl53l8` drives the
  VL53L8CX / CH on the SPI board and the VL53L5CX / L7CX / L7CH on this I2C
  board; the model picks the sensor firmware and the bridge. Everything in
  the [VL53L8CX guide](../vl53l8cx/guide.md) — configuration, ranging,
  callbacks and streams, the frame, crosstalk, thresholds, the motion
  indicator — applies; the [user guide](guide.md) lists the differences.
- **The class comes from the board** — `open_vl53l8()` / `open_device()`
  take the model from the USB PID (`0xED48` L5CX, `0xED49` L7CX, `0xED4A`
  L7CH), else from the device name, else L7CX. `module_type()` (after
  `init()`) says what the silicon is — 0 MZ = L5, 1 MZEVO = L7 — but never
  tells CX from CH.
- **Same frame as the VL53L8** — per-zone distance, status, signal, ambient,
  sigma, reflectance and SPAD count, row-major. Everything in the
  [VL53L8CX guide's frame section](../vl53l8cx/guide.md#the-frame-fields-and-the-zone-grid)
  applies; only the decoder entry point of the decode layer differs.
- **Split reads at 1536 bytes** — `READ_MAX_LEN` (the VL53L8 host splits at
  2048; reusing that fails with `ERR_INVALID_PARAM`). The live class splits
  for you; your own host code must. Writes go up to `WRITE_MAX_LEN` (2048).
- **Pin control drops the sensor state** — `PinAction::LpnOff` and
  `PinAction::SoftCycle` stop the stream and wipe the sensor firmware
  (`init()` again); the board has no power switch, so none of the pin actions
  is a true reset.
- **1 Hz works** — `MIN_RANGING_FREQUENCY_HZ` is 1 (the VL53L8 needs ≥ 2 Hz).
- **No deep sleep, no threshold auto-stop** — the L5CX / L7CX sensor firmware
  (ST ULD 2.0.1) lacks both; the class throws `ArgumentError` for them. The
  L7CH has them.
- **`RPT_VL53_INFO` has no echoed command byte** — if your own reply matching
  looks at `payload[0]`, exempt report `0x92`.

## See also

- [VL53L7CX user guide](guide.md) — open and init, what differs from the
  VL53L8 on the live class, the I2C-board methods, record / replay, and the
  decode layer (board commands, replies, class resolution, frames).
- [API reference](api.md) — `depz/vl53l7.hpp`; the live class `depz::Vl53l8`
  is in the [VL53L8CX reference](../vl53l8cx/api.md).
- [VL53L8CX user guide](../vl53l8cx/guide.md) — the shared chunk parse,
  reassembler and frame fields.
