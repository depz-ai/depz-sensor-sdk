---
title: VL53L7CX — guide
description: Hello-world for the VL53L7CX 8×8 ToF sensor in Node and the browser, plus what the I2C L5/L7 board adds over the VL53L8CX — board commands, pin control, the L5/L7 limits — and gotchas.
---

# VL53L7CX — guide

Hands-on guide to `Vl53l7cx`. It subclasses `Vl53l8cx`, so the
[VL53L8CX guide](../vl53l8cx/guide.md) applies line for line — configuration,
the frame and `zoneGrid()`, streaming, the advanced features. This page
covers only what the L7 board adds. For concepts see the
[introduction](introduction.md), for discovery and the common device features
the [overview](../overview.md), for signatures the [API reference](api.md).

## Contents

- [Open and initialize](#open-and-initialize)
- [Hello-world — Node (8×8 depth frames)](#hello-world--node-88-depth-frames)
- [Hello-world — browser (Web Serial)](#hello-world--browser-web-serial)
- [Board commands](#board-commands)
- [Advanced features on L5/L7](#advanced-features-on-l5l7)
- [Gotchas](#gotchas)

## Open and initialize

```ts
import { openDevice } from "@depz/sensor-sdk/node";
import { MODULE_TYPE_MZEVO, Vl53l7cx } from "@depz/sensor-sdk";

const dev = await openDevice();                  // returns a Vl53l7cx (USB id / device name)
if (!(dev instanceof Vl53l7cx)) throw new Error("not a VL53L7CX");

await dev.init(undefined, { progress: (t) => console.log(t) });  // ~1.4 s: L5/L7 blob
console.log(dev.moduleType === MODULE_TYPE_MZEVO);  // true (1 = MZEVO, the L7)
```

All three L5/L7 boards run the same firmware, so the class comes from the
board's USB id, then from its device name (`… VL53L7CX USB v2.1 …`). After
`init()` the sensor reports its module type; if it contradicts the class (a
board stamped L7 carrying an L5), `init()` logs a `console.warn` — ranging
still works, the blob is shared. The blob is fixed by the class, so you never
pass a variant; `{ writeProgress }` next to `progress` tracks the blob writes.

## Hello-world — Node (8×8 depth frames)

```ts
import { openDevice } from "@depz/sensor-sdk/node";
import { RESOLUTION_8X8, Vl53l7cx, zoneGrid } from "@depz/sensor-sdk";

const dev = await openDevice();
if (!(dev instanceof Vl53l7cx)) throw new Error("not a VL53L7CX");

await dev.init();
await dev.setResolution(RESOLUTION_8X8);
await dev.setRangingFrequencyHz(15);             // 1..15 Hz at 8×8 (1 Hz works on L5/L7)
await dev.startRanging();
try {
  let n = 0;
  for await (const frame of dev.frames()) {      // bounded, drop-oldest iterator
    const d = frame.distanceMm;                  // 64 zones, row-major, mm
    const centre = (d[27]! + d[28]! + d[35]! + d[36]!) / 4;
    console.log("centre:", centre, "min:", Math.min(...d));
    const grid = zoneGrid(d, frame.resolution);  // 8×8 number[][] for a heatmap
    if (++n === 100) break;
  }
} finally {
  await dev.stopRanging();
  await dev.close();
}
```

The centre of an 8×8 grid is the four zones 27, 28, 35, 36 — rows 3–4,
columns 3–4 of `zoneGrid()`.

## Hello-world — browser (Web Serial)

```ts
import { openDevice } from "@depz/sensor-sdk/web";
import { RESOLUTION_8X8, Vl53l7cx } from "@depz/sensor-sdk";

connectBtn.addEventListener("click", async () => {
  const port = await navigator.serial.requestPort();     // user gesture
  const dev = await openDevice(port);
  if (!(dev instanceof Vl53l7cx)) throw new Error("not a VL53L7CX");

  await dev.init(undefined, { progress: setStatus });    // ~1.4 s: show progress
  await dev.setResolution(RESOLUTION_8X8);
  await dev.setRangingFrequencyHz(10);
  await dev.startRanging();
  const unsub = dev.onFrame((f) => renderDepth(f));       // keep the callback quick
});
```

## Board commands

```ts
import { PinAction } from "@depz/sensor-sdk";

const info = await dev.getBridgeInfo();  // Vl53l7Info: pin levels, bus speed, I2C counters
console.log(info.i2cKhz, info.i2cErrors, info.lastI2cError, info.framesDropped);

await dev.setI2cSpeedKhz(400);           // → 400; snaps to 100, 200, 400, 500 … 1000 kHz

await dev.pinCtrl(PinAction.SoftCycle);  // LPn low 1 ms, high, I2C_RST pulse
await dev.init();                        // the sensor lost its firmware — reload it
```

`getBridgeInfo()` takes the bus away from the stream for a moment: read it
before and after a run, not during one. `setI2cSpeedKhz()` rejects with a
`BusyError` mid-transfer — stop ranging first. `lastI2cError` is a
`Vl53l7I2cError` (`Ok`, `Nack`, `Timeout`, `BusError`).

| `PinAction` | effect |
|---|---|
| `LpnOff` | stop streaming, LPn low: sensor I2C interface off (state lost) |
| `LpnOn` | LPn high: interface on (the power-up default) |
| `I2cRst` | pulse the sensor's I2C_RST |
| `SoftCycle` | stop streaming, LPn low 1 ms, high, I2C_RST pulse (state lost) |

## Advanced features on L5/L7

Crosstalk calibration and caldata, detection thresholds, the motion indicator
and sleep/wake work exactly as in the
[VL53L8CX guide](../vl53l8cx/guide.md#advanced-features), with two limits of
the L5CX / L7CX firmware (ST ULD 2.0.1):

- **No `POWER_MODE_DEEP_SLEEP`** — only sleep and wake-up; deep sleep throws.
- **No threshold auto-stop** — detection thresholds work,
  `setDetectionThresholdsAutoStop()` throws.

```ts
await dev.calibrateXtalk(16, 4, 600);    // reflectance %, samples, distance mm
if (dev.xtalkCalibrationFailed) {
  console.log("nothing to calibrate — the cover glass is too good; defaults kept");
}
```

## Gotchas

- **`init()` after every pin reset** — `LpnOff` / `SoftCycle` wipe the
  sensor firmware.
- **Bridge info during ranging costs a frame** — read counters around a run.
- **Module type is only known after `init()`** — `moduleType` is `null` before.
- All the [VL53L8CX gotchas](../vl53l8cx/guide.md#gotchas) apply, except the
  ≥ 2 Hz floor: here 1 Hz is fine.
