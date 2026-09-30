---
title: VL53L1CX — guide
description: Hello-world for the VL53L1CX single-zone ToF sensor in Node and the browser — open, configure, stream, the measurement, calibration, driver capabilities, and gotchas.
---

# VL53L1CX — guide

Hands-on guide to the `Vl53l1cx` device class. For what the sensor is and
its concepts, read the [introduction](introduction.md); for discovery,
recording and the common device features see the [overview](../overview.md);
for exact signatures the [API reference](api.md).

## Contents

- [Open and initialize](#open-and-initialize)
- [Hello-world: live distance](#hello-world-live-distance)
- [Hello-world — browser (Web Serial)](#hello-world--browser-web-serial)
- [Configure](#configure)
- [Streaming and single shot](#streaming-and-single-shot)
- [The measurement](#the-measurement)
- [Several targets (histogram driver)](#several-targets-histogram-driver)
- [Calibration](#calibration)
- [Tuning](#tuning)
- [What each driver supports](#what-each-driver-supports)
- [Reset and bridge diagnostics](#reset-and-bridge-diagnostics)
- [Gotchas](#gotchas)

## Open and initialize

```ts
import { openDevice } from "@depz/sensor-sdk/node";
import { Vl53l1cx } from "@depz/sensor-sdk";

const dev = await openDevice();                  // returns a Vl53l1cx (from the board name)
if (!(dev instanceof Vl53l1cx)) throw new Error("not a VL53L1CX");
await dev.init();                                // driver "uld"
const info = await dev.identifyProduct();
console.log(info.driverClass, await dev.notes());
```

`openDevice()` picks the class from the board's USB id or its device name
(`… VL53L1CX USB v2.1 …`) — every 1D board runs the same firmware, so the
firmware name cannot tell them apart. `identifyProduct()` resolves to
everything about the bound pair: product, driver, model id, reach, supported
groups, modes and budgets. (`identify()` is the firmware identity every DEPZ
device has.)

The board has two drivers; `init()` picks `uld`. For the other one:

```ts
await dev.init("histogram");
```

## Hello-world: live distance

```ts
import { openDevice } from "@depz/sensor-sdk/node";
import { Vl53l1cx } from "@depz/sensor-sdk";

const dev = await openDevice();
if (!(dev instanceof Vl53l1cx)) throw new Error("not a VL53L1CX");

await dev.init();
await dev.configure({ budgetMs: 50, mode: "short" });
await dev.startRanging();
for await (const m of dev.measurements()) {      // runs until you break out
  console.log(m.plottable ? `${m.distanceMm} mm  sigma ${m.sigmaMm.toFixed(1)}` : m.statusText);
}
```

## Hello-world — browser (Web Serial)

```ts
import { openDevice } from "@depz/sensor-sdk/web";
import { Vl53l1cx } from "@depz/sensor-sdk";

connectBtn.addEventListener("click", async () => {
  const port = await navigator.serial.requestPort();     // user gesture
  const dev = await openDevice(port);
  if (!(dev instanceof Vl53l1cx)) throw new Error("not a VL53L1CX");

  await dev.init();
  await dev.configure({ budgetMs: 50, mode: "short" });
  await dev.startRanging();
  const unsub = dev.onMeasurement((m) => {
    if (m.plottable) render(m.distanceMm);              // callback: keep it quick
  });
});
```

## Configure

```ts
await dev.configure({ budgetMs: 50, interMs: 0, mode: "short" });   // re-init + apply
const [budgetMs, interMs] = await dev.getRangeTiming();  // read back from the sensor
```

`configure()` re-initialises the sensor, then applies the mode, the timing and
any stored calibration. Call it before every run. `interMs: 0` ranges
back-to-back; a larger value is the period between measurements and must
exceed the budget.

`dev.modes` lists the named modes of the current driver (the first one is what
`init()` leaves); `await dev.getMode()` reads back the one in use (`null` on a
driver without modes). A mode change rewrites the timing, which is why
`configure()` applies the mode first.

The light driver only accepts **tabulated** budgets, per mode:

```ts
dev.budgetChoices();     // → [20, 33, 50, 100, 200, 500] in "long"
dev.snapBudget(40);      // → 33 — the nearest one it will take
```

`configure()` refuses anything else rather than rounding silently.

## Streaming and single shot

```ts
const one = await dev.measureOnce(1000);         // start, wait, read, stop — not while streaming

await dev.startRanging();                        // INT-driven stream
const it = dev.measurements(64);                 // iterator — bounded, drop-oldest
const first = (await it.next()).value;
console.log("dropped so far:", it.droppedCount);
const unsub = dev.onMeasurement((m) => console.log(m.distanceMm));  // read-pump context
const next = await dev.getMeasurement(2000);     // next one; DepzTimeoutError on silence
await dev.stopRanging();
```

`dev.streamParseErrors` counts stream reports that failed to decode — 0 in a
healthy session.

## The measurement

| field | meaning |
|---|---|
| `timestampUs` | MCU uptime at the INT edge (stream) / the read (poll), `bigint` µs |
| `distanceMm` | distance, mm |
| `status`, `statusText` | 0 = valid; the text names the failure otherwise |
| `sigmaMm` | range std-dev estimate |
| `signalKcps`, `ambientKcps` | return-signal and ambient rates |
| `spads` | active SPADs |
| `targets` | every return (histogram driver), else empty |
| `extra` | driver-specific values (stream count, per-SPAD rates …) — display only |
| `bins` | the raw 24-bin histogram (histogram driver), else `null` |

`m.valid` is `status === 0`; `m.plottable` also accepts the statuses the
histogram driver uses for usable frames — prefer it.

## Several targets (histogram driver)

```ts
import { plotDistances, primaryTarget } from "@depz/sensor-sdk";

await dev.init("histogram");
await dev.configure({ budgetMs: 33, mode: "medium" });
await dev.startRanging();
const m = await dev.getMeasurement();
for (const t of m.targets) {                     // strongest first, up to four
  console.log(t.distanceMm, t.statusText, t.signalKcps);
}
console.log(plotDistances(m));                   // what a chart should draw
console.log(primaryTarget(m));                   // what a single readout should show
const bins = m.bins;                             // the raw 24-bin histogram object
await dev.stopRanging();
```

On the histogram driver `status === 0` is not the test: statuses 0, 6 (first
frame, no predecessor) and 11 (merged pulse) are all usable
(`PLOTTABLE_STATUSES`) — use `m.plottable`, `plotDistances()` or
`primaryTarget()`. The driver steps an A/B frame-pair state per frame, so
every streamed frame is decoded exactly once, in order. `m.bins` is typed
`unknown` — it is the driver's own bin object, for display and logging; don't
build on its shape.

## Calibration

```ts
const offset = await dev.calibrateOffset(500);   // flat target at a known distance, mm
const xtalk = await dev.calibrateXtalk(600);     // cover-glass crosstalk, kcps

// the values live in sensor RAM — store them and re-apply on every run:
await dev.configure({ budgetMs: 50, offsetMm: offset, xtalkKcps: xtalk });
```

Calibrations block for a burst of samples, program the sensor and resolve to
the value now in effect (both take an optional sample count as a second
argument). A reset or power cycle drops them.

Calibrations belong to the light driver — the histogram driver has none
(`supports("calib_offset")` is false there).

## Tuning

Only where `supports()` says so — after `configure()`, while not ranging:

```ts
import { WINDOW_IN } from "@depz/sensor-sdk";

await dev.setDetectionThresholds(100, 300, WINDOW_IN);  // INT only for 100–300 mm
await dev.setSignalThresholdKcps(1024);                 // drop weak returns
await dev.setSigmaThresholdMm(15);                      // drop noisy ranges
await dev.setRoi(8, 8);                                 // narrower beam (SPAD window)
await dev.startTemperatureUpdate();                     // after a >8 °C change
```

The window codes (`WINDOW_BELOW`, `WINDOW_ABOVE`, `WINDOW_OUT`, `WINDOW_IN`)
are the ones the VL53L4CD uses — the same register values on this die. A
detection window stays armed until the next `init()` / `configure()`.

## What each driver supports

| driver | capability groups |
|---|---|
| `uld` | timing budget / inter-measurement period, named ranging modes, offset correction, offset calibration, crosstalk correction, crosstalk calibration, distance-window interrupt, signal threshold, sigma threshold, region of interest, temperature (VHV) update |
| `histogram` | timing budget / inter-measurement period, named ranging modes |

Ask `dev.supports("roi")` (etc.) at run time; a call outside the list throws.

## Reset and bridge diagnostics

```ts
import { VL53L4_XSHUT_RESET } from "@depz/sensor-sdk";

await dev.xshut(VL53L4_XSHUT_RESET);   // power-cycle the sensor — init() again afterwards

const info = await dev.bridgeInfo();   // Vl53lxInfo: bridge counters, safe while streaming
console.log(info.intEdges, info.slotsSkipped, info.framesDropped, info.i2cErrors, info.i2cKhz);
```

`i2cErrors` counts from the end of the last init: a resetting die NACKs its
own address for a moment, so the SDK clears the counter once `init()` /
`configure()` is through (VL53_CLEAR_I2C_ERRORS). That needs firmware
`APP_VL53L0_4_v0.24` or newer; on an older board `init()` throws a
`DepzError` that says to reflash it.

The XSHUT action codes are the VL53L4CD bridge's (`VL53L4_XSHUT_OFF` /
`_ON` / `_RESET`) — the same on this firmware. A power-cycled sensor holds
none of the configuration or calibration. Counters are free-running and wrap
silently — watch increments.

## Gotchas

- **`configure()` before every run** — it is also what clears an armed detection window.
- **Configuration while ranging throws** — stop, reconfigure, restart.
- **Prefer `plottable` over `valid`** — non-zero statuses are data, not errors.
- **Budgets are a table, not a range** — `configure()` refuses a budget the current mode does not have; ask `budgetChoices()` or `snapBudget()`.
- **No free-running mode on the light driver** — a zero period is written as the budget.
- **Callbacks run on the read-pump context** — keep them short; hand heavy work to a queue or use `measurements()`.
- **`timestampUs` is a `bigint`** (device µs). Use `syncTime()` + `toHostTimeUs()` to place measurements on the host clock.
