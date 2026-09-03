---
title: VL53L8CH — guide
description: Hello-world for the VL53L8CH ToF sensor — everything the CX does, plus configureCnh and decoding CNH histogram frames in the browser and Node.
---

# VL53L8CH — guide

The `Vl53l8Ch` class is the [`Vl53l8Cx`](../vl53l8cx/guide.md) superset: it
inherits **every** CX method (init, resolution, rate, advanced ULD features,
frame consumption). This page covers only the CH addition — **CNH histograms**.

> **Do the CX guide first.** Init, resolution, ranging rate, `frames()` /
> `getFrame()` / `onFrame()`, and the advanced ULD features (power modes,
> crosstalk, detection thresholds, motion indicator) are identical — see the
> [VL53L8CX guide](../vl53l8cx/guide.md). Only the class differs — `openDevice()`
> returns `Vl53l8Ch` for the CH USB PID and `init()` loads the `"ch"` blob from
> the class, so you never pass a variant:
>
> ```ts
> import { Vl53l8Ch, RESOLUTION_8X8, zoneGrid } from "@depz/sensor-sdk";
>
> const dev = await openDevice();
> if (!(dev instanceof Vl53l8Ch)) throw new Error("need CH firmware");
> await dev.init(undefined, { progress: (t) => console.log(t) });  // CH blob
> // …everything else is exactly as the CX guide shows…
> ```

## CNH histograms (CH only)

Arm the histogram block with a `CnhConfig` while stopped, then decode
`frame.cnhRaw` on each frame.

```ts
import { openDevice } from "@depz/sensor-sdk/node";
import { Vl53l8Ch, CnhConfig, decodeCnh } from "@depz/sensor-sdk";

const dev = await openDevice();
if (!(dev instanceof Vl53l8Ch)) throw new Error("need CH firmware");
await dev.init();

const cfg = new CnhConfig();
cfg.initConfig(0, 24, 4);                          // start bin, feature length, subsample
cfg.createAggMap(64, 0, 0, 2, 2, 4, 4);            // map 8×8 zones → aggregates
await dev.configureCnh(cfg);                        // arm for the next startRanging()
await dev.setRangingFrequencyHz(5);
await dev.startRanging();

for await (const frame of dev.frames()) {
  if (frame.cnhRaw) {
    const cnh = decodeCnh(cfg, frame.cnhRaw);       // per-aggregate histograms
    console.log(cnh.aggregates.length, "aggregates");
  }
}
```

### Browser (Web Serial)

```ts
import { openDevice } from "@depz/sensor-sdk/web";
import { Vl53l8Ch, CnhConfig, decodeCnh } from "@depz/sensor-sdk";

connectBtn.addEventListener("click", async () => {
  const port = await navigator.serial.requestPort();     // user gesture
  const dev = await openDevice(port);
  if (!(dev instanceof Vl53l8Ch)) throw new Error("need CH firmware");

  await dev.init(undefined, { progress: setStatus });    // show progress: not a hang
  const cfg = new CnhConfig();
  cfg.initConfig(0, 24, 4);
  cfg.createAggMap(64, 0, 0, 2, 2, 4, 4);
  await dev.configureCnh(cfg);
  await dev.setRangingFrequencyHz(5);
  await dev.startRanging();

  dev.onFrame((f) => {
    if (f.cnhRaw) renderHistograms(decodeCnh(cfg, f.cnhRaw));  // keep it quick
  });
});
```

Sizing the config: `cfg.requiredMemory()` must fit the sensor's CNH buffer
(`CNH_MAX_DATA_BYTES`); `cfg.maxBins(...)` / `cnhMaxBins(...)` report how many
histogram bins a given aggregate count allows. A decoded `CnhDecoded` carries a
`refResidual` and one `CnhAggregate` per aggregate (`hist`, `histRaw`,
`histScaler`, `ambient`).

## Gotchas

- All the [CX gotchas](../vl53l8cx/guide.md#gotchas) apply.
- **`configureCnh()` is CH-only** — it does not exist on `Vl53l8Cx`; `openDevice`
  returns `Vl53l8Ch` only for CH firmware (PID `0xED40`).
- **Arm CNH while stopped** — `configureCnh()` throws while ranging; it takes
  effect on the next `startRanging()`.
- **A frame's `cnhRaw` can be null** — only CH firmware with CNH armed populates
  it; always null-check before `decodeCnh`.
