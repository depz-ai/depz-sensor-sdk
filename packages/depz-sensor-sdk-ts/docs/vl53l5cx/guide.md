---
title: VL53L5CX — guide
description: Open and range the VL53L5CX 8×8 ToF sensor — the VL53L7CX class with a different expected module type — plus its gotchas.
---

# VL53L5CX — guide

`Vl53l5cx` is the [VL53L7CX](../vl53l7cx/guide.md) class with a different
expected module type; everything in the VL53L7CX guide — board commands, pin
control, the L5/L7 limits — and in the [VL53L8CX guide](../vl53l8cx/guide.md)
— configuration, frames, streaming, advanced features — applies unchanged.

## Open and range

```ts
import { openDevice } from "@depz/sensor-sdk/node";
import { MODULE_TYPE_MZ, RESOLUTION_8X8, Vl53l5cx, zoneGrid } from "@depz/sensor-sdk";

const dev = await openDevice();                  // returns a Vl53l5cx for the L5CX board
if (!(dev instanceof Vl53l5cx)) throw new Error("not a VL53L5CX");

await dev.init();                                // same L5/L7 blob as the VL53L7CX
if (dev.moduleType !== MODULE_TYPE_MZ) console.warn("not an L5 die");
await dev.setResolution(RESOLUTION_8X8);
await dev.setRangingFrequencyHz(10);
await dev.startRanging();
const frame = await dev.getFrame(2000);
console.log(zoneGrid(frame.distanceMm, frame.resolution));
await dev.stopRanging();
```

## Gotchas

- **The class is chosen from the board's USB id / device name.** A board
  stamped as an L7 but carrying an L5 (or the reverse) still ranges — the blob
  is shared — and `init()` warns about the module-type mismatch.
- **No deep sleep, no threshold auto-stop** (ULD 2.0.1), as on the VL53L7CX.
- **Peel the protective film off the lens** before measuring — with the film
  on, weak-signal zones read 0.
