# VL53L7CX — introduction

The **VL53L7CX** is ST's wide-angle multizone Time-of-Flight sensor: an 8×8
(or 4×4) depth image over a **90° diagonal** field of view, up to ~3.5 m, 4×4
at up to 60 Hz and 8×8 at up to 15 Hz. It is the same ST ULD family as the
[VL53L8CX](../vl53l8cx/introduction.md) — same frames, same per-zone outputs —
on a board that talks **I2C** instead of SPI (the `APP_VL53L7` firmware). The
MCU is a thin register bridge; the ST driver runs on the host. This C SDK runs
it for you with the same **multizone sensor class** as the VL53L8
(`depz_vl53l8_*`), the sensor-firmware download included.

One board firmware serves three sensors that differ only in the soldered part:

| sensor | field of view | USB PID | model | sensor firmware | `module_type` |
|---|---|---|---|---|---|
| [VL53L5CX](../vl53l5cx/introduction.md) | 63° | `0xED48` | `DEPZ_VL53L8_MODEL_L5CX` | L5/L7 (ULD 2.0.1) | 0 = MZ |
| **VL53L7CX** | 90° | `0xED49` | `DEPZ_VL53L8_MODEL_L7CX` | L5/L7 (ULD 2.0.1) | 1 = MZEVO |
| [VL53L7CH](../vl53l7ch/introduction.md) | 90° | `0xED4A` | `DEPZ_VL53L8_MODEL_L7CH` | CH (VL53LMZ 2.0.16) + CNH histograms | 1 = MZEVO |

## What the SDK covers

The L5/L7 boards run the VL53L8 **sensor class** of the C live layer
(`depz_sensor_io.h`) — the [VL53L8CX guide](../vl53l8cx/guide.md) is its full
manual — with the L5/L7 branch of the ULD inside:

- **Open** — `depz_open_device()` finds, probes and opens the board with the
  class attached and the model resolved (USB PID, then the device name, then
  VL53L7CX); the device reports sensor type `vl53l7`
  (`DEPZ_SENSOR_VL53L7`), and `depz_is_vl53l8()` is true for it.
  `depz_vl53l8_open_link(link, model, &dev)` picks the model by hand.
- **Initialise** — `depz_vl53l8_init()`: the silicon check, the
  sensor-firmware download over I2C and the default configuration (~1.3 s);
  afterwards `depz_vl53l8_module_type()` reads what the silicon says, L5 or
  L7.
- **Configure and range** — resolution, ranging frequency (**1 Hz works**
  here), ranging mode, integration time, sharpener, target order, sleep /
  wake, crosstalk, detection thresholds, the motion indicator, and the frame
  stream as callbacks, streams or one frame at a time — all exactly as on the
  VL53L8.
- **Board commands** — the I2C bridge's own: `depz_vl53l7_bridge_info()`
  (pin levels, bus speed, I2C and frame counters),
  `depz_vl53l7_set_i2c_speed_khz()` and `depz_vl53l7_pin_ctrl()` (the sensor's
  LPn and I2C_RST pins).
- **Record / replay** — any session, firmware download included, records to
  a `.depzrec` file and replays byte-exact; captures made by the Python SDK
  replay through this class unchanged.
- **Decode layer** — if you own the transport: the bridge command / report
  codecs (`depz_vl53l7_pack_*`, `depz_vl53l7_unpack_info`), the class
  resolution (`depz_vl53l7_resolve_model()`) and the L5/L7 frame decode
  (`depz_vl53l7_decode_frame()`).

The class was checked on a real VL53L7CH (firmware `APP_VL53L7_v0.53`) — see
[verified on hardware](guide.md#verified-on-hardware). The VL53L7CX and
VL53L5CX run the L5/L7 sensor firmware through the same code; the two
committed VL53L5CX captures replay strictly through it.

## When to use it

Pick the VL53L7CX over the VL53L8CX for its **wider 90° view** (room corners,
close-range obstacle maps) or when you only have I2C. For a narrower beam the
[VL53L5CX](../vl53l5cx/introduction.md) is the 63° part; for raw return
histograms the [VL53L7CH](../vl53l7ch/introduction.md).

## Key concepts

- **Same class, same frame as the VL53L8** — every `depz_vl53l8_*` call and
  the `depz_vl53l8_live_frame` work unchanged; the
  [VL53L8CX guide](../vl53l8cx/guide.md) applies. Only the model, and the
  differences below, are L5/L7-specific.
- **The model comes from the board, not the silicon** — the USB PID or the
  device name says L5CX / L7CX / L7CH. `depz_vl53l8_module_type()` (after
  `init()`) tells L5 from L7, never CX from CH, and is a diagnostic, not the
  class choice.
- **What ULD 2.0.1 lacks** — the L5CX / L7CX sensor firmware has **no deep
  sleep and no detection-threshold auto-stop**; both calls fail with
  `DEPZ_E_ARG`. The L7CH (VL53LMZ firmware) has both.
- **1 Hz works** — L5/L7 range and stream down to 1 Hz (the VL53L8 needs
  ≥ 2 Hz).
- **Pin control drops the sensor state** — `DEPZ_VL53L7_PIN_LPN_OFF` and
  `DEPZ_VL53L7_PIN_SOFT_CYCLE` stop the stream and wipe the sensor firmware;
  run `init()` again. The board has no power switch, so none of the pin
  actions is a true reset.
- **Reads split at 1536 bytes** — the I2C bridge's ceiling
  (`DEPZ_VL53L7_READ_MAX_LEN`, 2048 on the VL53L8); the class splits for you.

## See also

- [VL53L7CX user guide](guide.md) — open and resolve the model, what differs
  from the VL53L8, the board commands, record and replay, the decode layer.
- [VL53L8CX user guide](../vl53l8cx/guide.md) — the class itself:
  configuration, streaming, the frame, the advanced features.
- [API reference](api.md) — the L5/L7 codecs and board commands; the class
  is in the [VL53L8CX reference](../vl53l8cx/api.md).
