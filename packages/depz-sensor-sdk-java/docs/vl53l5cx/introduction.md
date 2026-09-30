# VL53L5CX — introduction

The **VL53L5CX** is STMicroelectronics' original multizone Time-of-Flight
sensor: an 8×8 (or 4×4) depth image over a **63° diagonal** field of view, up
to ~4 m. On the DEPZ line it sits on the same I2C board as the
[VL53L7CX](../vl53l7cx/introduction.md) (`APP_VL53L7` firmware, contract 11)
and runs the same sensor firmware (ST ULD 2.0.1). Production USB PID `0xED48`.

## What the sensor does

Exactly what the VL53L7CX does — per-zone distances, statuses and rates, 4×4
up to 60 Hz and 8×8 up to 15 Hz, 1 Hz supported, the board commands, the
advanced ULD features without deep sleep and threshold auto-stop — with a
**narrower, longer-reaching beam** (63° instead of 90°). After initialisation
the sensor reports module type **MZ** (0); the VL53L7CX reports MZEVO (1).

## When to use it

Pick the VL53L5CX when you want a multizone depth image aimed at something
rather than a wide room view: a doorway, a conveyor, a hand in front of a
screen. For the wide view take the VL53L7CX; for raw histograms the
[VL53L7CH](../vl53l7ch/introduction.md).

## What this SDK offers

The VL53L5CX uses the same [`Vl53l7`](api.md#vl53l7) /
[`Vl53l7Uld`](api.md#vl53l7uld) classes as the VL53L7CX — the board's wire
codecs and transfer ceilings, class resolution (`Vl53l7Uld.resolveModel`
returns `Model.VL53L5CX` for this board) and frame decode through
`Vl53l7Uld.parseFrame`. Nothing in the decode is specific to the L5:
its frames have the L7 layout.

Initialising the sensor (firmware download, configuration) and running a
ranging session is done today with the Python or TypeScript SDK; this SDK
decodes the frames and builds the command payloads.

## See also

- [VL53L5CX user guide](guide.md)
- [VL53L7CX introduction](../vl53l7cx/introduction.md) and
  [guide](../vl53l7cx/guide.md) — board commands, transfer ceilings, frame
  decode.
- [VL53L8CX guide](../vl53l8cx/guide.md) — results fields, scaling, the grid,
  advanced DCI codecs.
- [API reference](api.md)
