---
title: VL53L7CH — guide
description: Open the VL53L7CH, arm and stream Compact-Network-Histogram (CNH) frames, use the VL53LMZ-only features (deep sleep, threshold auto-stop), and gotchas.
---

# VL53L7CH — guide

`Vl53l7ch` = the [VL53L7CX](../vl53l7cx/guide.md) class + CNH. Everything in
the VL53L7CX guide (board commands, pin control) and the
[VL53L8CX guide](../vl53l8cx/guide.md) (configuration, frames, streaming,
advanced features) applies. This page covers CNH and what the VL53LMZ
firmware adds.

## Open and initialize

```ts
import { openDevice } from "@depz/sensor-sdk/node";
import { RESOLUTION_8X8, Vl53l7ch } from "@depz/sensor-sdk";

const dev = await openDevice();                  // returns a Vl53l7ch for the L7CH board
if (!(dev instanceof Vl53l7ch)) throw new Error("not a VL53L7CH");

await dev.init(undefined, { progress: (t) => console.log(t) });  // CH (VL53LMZ) blob
await dev.setResolution(RESOLUTION_8X8);
await dev.setRangingFrequencyHz(15);
```

## CNH histograms

Build and size-check a `CnhConfig` exactly as on the
[VL53L8CH](../vl53l8ch/guide.md#cnh-histograms-ch-only), arm it while stopped,
then range — the histograms arrive **in the stream**, in every frame:

```ts
import { CnhConfig, CNH_MAX_DATA_BYTES, RESOLUTION_8X8, decodeCnh, zoneGrid } from "@depz/sensor-sdk";

const cfg = new CnhConfig();
cfg.initConfig(10, 20, 2);                       // start bin, bins, sub-sample
cfg.createAggMap(RESOLUTION_8X8, 0, 0, 2, 2, 4, 4);  // 16 aggregates
console.log(cfg.requiredMemory() <= CNH_MAX_DATA_BYTES);  // ≤ 6160; throws if over

await dev.configureCnh(cfg);
await dev.startRanging();
const frame = await dev.getFrame(2000);
if (frame.cnhRaw !== null) {
  const histograms = decodeCnh(cfg, frame.cnhRaw);  // per-aggregate histograms
  console.log(histograms.aggregates.length);        // 16
}
const grid = zoneGrid(frame.distanceMm, frame.resolution);  // the usual depth image, same frame
await dev.stopRanging();
```

A frame with CNH is larger than one stream chunk (3156 B for 16 aggregates ×
20 bins, up to ~7.6 KB); the SDK reassembles the chunks. On the L7CH you just
stream — `frames()`, `getFrame()` and `onFrame()` all carry `cnhRaw`.

## What the VL53LMZ firmware adds

```ts
import { POWER_MODE_DEEP_SLEEP, POWER_MODE_WAKEUP } from "@depz/sensor-sdk";

await dev.setPowerMode(POWER_MODE_DEEP_SLEEP);   // L7CH only on this board
await dev.setPowerMode(POWER_MODE_WAKEUP);       // wake from deep sleep re-runs init()
```

The detection-threshold **auto-stop** (`setDetectionThresholdsAutoStop()`)
also works here, as on the VL53L8
([VL53L8CX guide](../vl53l8cx/guide.md#advanced-features)).

## Gotchas

- **`configureCnh()` while stopped** — like every config call.
- **Size-check first** — `requiredMemory()` must be ≤ 6160 bytes
  (`CNH_MAX_DATA_BYTES`); it throws `CnhConfigError` otherwise.
- **Waking from deep sleep re-downloads the firmware.**
- **`cnhRaw` is null until CNH is armed** — null-check before `decodeCnh()`.
- All the [VL53L7CX gotchas](../vl53l7cx/guide.md#gotchas) apply.
