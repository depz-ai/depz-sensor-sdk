# VL53L7CX — introduction

The **VL53L7CX** is STMicroelectronics' wide-angle multizone Time-of-Flight
sensor: an 8×8 (or 4×4) depth image over a **90° diagonal** field of view, up
to ~3.5 m. It is the same ULD family as the [VL53L8CX](../vl53l8cx/introduction.md)
— same host-side driver, same results frames — on a board that talks **I2C**
instead of SPI (`APP_VL53L7` firmware, contract 11). The MCU is a thin
register bridge; the ST ULD runs on the host.

One board firmware serves three sensors:

| board | sensor | field of view | USB PID | sensor firmware |
|---|---|---|---|---|
| [VL53L5CX](../vl53l5cx/introduction.md) | VL53L5CX | 63° | `0xED48` | L5/L7 (ULD 2.0.1) |
| **VL53L7CX** | VL53L7CX | 90° | `0xED49` | L5/L7 (ULD 2.0.1) |
| [VL53L7CH](../vl53l7ch/introduction.md) | VL53L7CH | 90° | `0xED4A` | CH (VL53LMZ 2.0.16) + CNH histograms |

## What the sensor does

- Per-zone distance, target status, signal and ambient rate, range sigma and
  reflectance over a 4×4 grid (up to 60 Hz) or an 8×8 grid (up to 15 Hz),
  plus a per-frame silicon temperature — the same frame content as the
  VL53L8CX.
- **1 Hz works** — the L5/L7 range and stream down to 1 Hz (the L8 needs
  ≥ 2 Hz).
- **Board commands** — `VL53_PIN_CTRL` drives the sensor's LPn / I2C_RST
  pins, `VL53_SET_I2C_SPEED` re-times the sensor bus (100 kHz … 1 MHz, default
  1 MHz), `VL53_GET_INFO` reads the bridge counters.
- **No deep sleep and no threshold auto-stop** on the L5CX / L7CX firmware
  (both exist on the VL53L7CH).

## When to use it

Pick the VL53L7CX over the VL53L8CX for its **wider 90° view** (room corners,
close-range obstacle maps) or when you only have I2C. For a narrower beam, the
[VL53L5CX](../vl53l5cx/introduction.md) is the 63° part; for raw return
histograms, the [VL53L7CH](../vl53l7ch/introduction.md).

## What this SDK offers

[`Vl53l7`](api.md#vl53l7) (wire codecs) and [`Vl53l7Uld`](api.md#vl53l7uld)
(decode) are the verifiable **decode and codec layer** for all three boards:

- the board's extra wire codecs — `Vl53l7.packPinCtrl`, `Vl53l7.packSetI2cSpeed` and the `RPT_VL53_INFO`
  decoder [`Vl53l7.Vl53l7Info`](api.md#vl53l7vl53l7info) — plus the register
  read/write/stream encoders it shares with the VL53L8 bridge;
- the board's tighter transfer ceilings (`Vl53l7.READ_MAX_LEN` = 1536,
  `Vl53l7.STREAM_CHUNK_MAX` = 1536);
- class resolution: which of the three sensors a board is
  (`Vl53l7Uld.resolveModel`);
- results-frame decode through the shared VL53L8 decoder with the L5/L7
  geometry (`Vl53l7Uld.parseFrame`) — footer id at `size-4`, every per-zone
  array trimmed to the frame's resolution.

What it does **not** do: open a port, download the sensor firmware, or run
the ULD configuration and ranging control. Initialising and streaming the
board is done today with the Python or TypeScript SDK; this SDK decodes what
the board sends and builds the command payloads.

## Key concepts

- **One decode path for L5, L7 and L8** — reassemble `RPT_VL53_FRAME` chunks
  with `FrameReassembler`, decode with `Vl53l7Uld.parseFrame(frame, size)`.
  The result is a `Vl53l8Uld.Results`; fields and scaling are the VL53L8CX
  ones.
- **The class comes from the board, not the silicon** — the production USB PID
  first, then the device name, then VL53L7CX (its firmware runs on every
  L5/L7 part). CX and CH silicon cannot be told apart at all.
- **Pin control drops the sensor state** — after `PinAction.LPN_OFF` or
  `PinAction.SOFT_CYCLE` the sensor firmware is gone and must be downloaded again.

## See also

- [VL53L7CX user guide](guide.md) — class resolution, board commands,
  transfer ceilings, frame decode, gotchas.
- [VL53L8CX user guide](../vl53l8cx/guide.md) — the results fields, scaling,
  the grid and the advanced DCI codecs, all shared.
- [API reference](api.md) — `Vl53l7`, `Vl53l7Uld` and the VL53L8 symbols
  they decode with.
