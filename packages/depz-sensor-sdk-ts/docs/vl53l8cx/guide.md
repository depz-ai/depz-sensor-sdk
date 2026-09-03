---
title: VL53L8CX — guide
description: Hello-world for the VL53L8CX ToF sensor in the browser and Node — init, resolution/rate, consuming frames, advanced ULD features, and gotchas.
---

# VL53L8CX — guide

Step-by-step for the `Vl53l8Cx` base ToF class. See
[introduction](introduction.md) for concepts and [overview](../overview.md) for
discovery, recording, and common device features. For the CNH histogram
superset, see the [VL53L8CH guide](../vl53l8ch/guide.md).

## Hello-world — Node (8×8 depth frames)

```ts
import { openDevice } from "@depz/sensor-sdk/node";
import { Vl53l8Cx, RESOLUTION_8X8, zoneGrid } from "@depz/sensor-sdk";

const dev = await openDevice();
if (!(dev instanceof Vl53l8Cx)) throw new Error("not a VL53L8");

await dev.init(undefined, { progress: (t) => console.log(t) });  // ~seconds: fw download
await dev.setResolution(RESOLUTION_8X8);
await dev.setRangingFrequencyHz(15);                        // must be >= 2 Hz
await dev.startRanging();

for await (const frame of dev.frames()) {
  const grid = zoneGrid(frame.distanceMm, frame.resolution);  // 8×8 mm
  console.log("center ≈", grid[3][3], "mm", frame.siliconTempDegc, "°C");
}
```

The firmware blob is fixed by the class (`Vl53l8Cx` loads `"cx"`), so you don't
pass a variant — `openDevice()` already returned the right class from the USB
PID. Pass `{ writeProgress }` alongside `progress` to track the blob writes.

## Hello-world — browser (Web Serial)

```ts
import { openDevice } from "@depz/sensor-sdk/web";
import { Vl53l8Cx, RESOLUTION_8X8 } from "@depz/sensor-sdk";

connectBtn.addEventListener("click", async () => {
  const port = await navigator.serial.requestPort();     // user gesture
  const dev = await openDevice(port);
  if (!(dev instanceof Vl53l8Cx)) throw new Error("not a VL53L8");

  await dev.init(undefined, { progress: setStatus });    // show progress: not a hang
  await dev.setRangingFrequencyHz(10);
  await dev.startRanging();
  const unsub = dev.onFrame((f) => renderDepth(f));       // keep the callback quick
});
```

## Configuration (after `init()`, not while ranging)

```ts
await dev.setResolution(RESOLUTION_8X8);   // 16 (4×4) or 64 (8×8)
await dev.getRangingFrequencyHz();
await dev.setIntegrationTimeMs(20);        // no effect in continuous mode
await dev.setSharpenerPercent(20);         // 0..99
await dev.setTargetOrder(TARGET_ORDER_CLOSEST);
```

## Consuming frames

```ts
// bounded, drop-oldest iterator (size for your consumer's pace)
const stream = dev.frames(8);
const frame = (await stream.next()).value;

// or a single frame with a timeout
const one = await dev.getFrame(2000);

// or a callback on the read-pump context
const unsub = dev.onFrame((f) => queue.push(f));
```

Each `Vl53l8Frame` exposes typed per-zone arrays (`distanceMm`, `targetStatus`,
`signalPerSpad`, `ambientPerSpad`, `rangeSigmaMm`, `reflectance`,
`nbTargetDetected`, `nbSpadsEnabled`), `siliconTempDegc`, and `motion`.

## Advanced features

Stop ranging first — these touch the register bank.

```ts
// Power modes
await dev.setPowerMode(POWER_MODE_SLEEP);        // WAKEUP from DEEP_SLEEP re-runs init()

// Crosstalk
await dev.setXtalkMargin(50);                    // kcps/SPAD
await dev.calibrateXtalk(16, 4, 600);            // reflectance %, samples, distance mm
const blob = await dev.getCaldataXtalk();        // 776-byte save …
await dev.setCaldataXtalk(blob);                 // … and restore

// Detection thresholds (interrupt on threshold)
await dev.setDetectionThresholds([
  { lowThresh: 200, highThresh: 600, measurement: 1 /* DIST_MM selector */,
    type: THRESH_OUT_OF_WINDOW, zoneNum: 0, operation: THRESH_OP_NONE },
]);
await dev.setDetectionThresholdsEnable(true);

// Motion indicator — surfaces frame.motion
await dev.configureMotionIndicator(400, 1500);   // distance window mm
```

## Gotchas

- **`init()` takes a few seconds** (84 KB firmware download) — show
  `progress`; it is not a hang.
- **Ranging must be ≥ 2 Hz** — `setRangingFrequencyHz(1)` throws.
- **Configure only after `init()` and only while stopped** — otherwise the call
  throws (`stopRanging()` first).
- **Size `frames()` for your pace** — the queue is drop-oldest; a slow consumer
  silently drops frames (watch `droppedCount`).
- **CNH is CH-only** — `configureCnh()` does not exist on `Vl53l8Cx`; run
  [`Vl53l8Ch`](../vl53l8ch/guide.md) for histograms.
