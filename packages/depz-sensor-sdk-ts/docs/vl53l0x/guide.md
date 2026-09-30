---
title: VL53L0X — guide
description: Hello-world for the VL53L0X single-zone ToF sensor in Node and the browser — open, configure, stream, the measurement, calibration, driver capabilities, and gotchas.
---

# VL53L0X — guide

Hands-on guide to the `Vl53l0x` device class. For what the sensor is and
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
- [Calibration](#calibration)
- [Tuning](#tuning)
- [What each driver supports](#what-each-driver-supports)
- [Reset and bridge diagnostics](#reset-and-bridge-diagnostics)
- [Gotchas](#gotchas)

## Open and initialize

```ts
import { openDevice } from "@depz/sensor-sdk/node";
import { Vl53l0x } from "@depz/sensor-sdk";

const dev = await openDevice();                  // returns a Vl53l0x (from the board name)
if (!(dev instanceof Vl53l0x)) throw new Error("not a VL53L0X");
await dev.init();                                // driver "uld"
const info = await dev.identifyProduct();
console.log(info.driverClass, await dev.notes());
```

`openDevice()` picks the class from the board's USB id or its device name
(`… VL53L0X USB v2.1 …`) — every 1D board runs the same firmware, so the
firmware name cannot tell them apart. `identifyProduct()` resolves to
everything about the bound pair: product, driver, model id, reach, supported
groups, modes and budgets. (`identify()` is the firmware identity every DEPZ
device has.)

## Hello-world: live distance

```ts
import { openDevice } from "@depz/sensor-sdk/node";
import { Vl53l0x } from "@depz/sensor-sdk";

const dev = await openDevice();
if (!(dev instanceof Vl53l0x)) throw new Error("not a VL53L0X");

await dev.init();
await dev.configure({ budgetMs: 50, mode: "long-range" });
await dev.startRanging();
for await (const m of dev.measurements()) {      // runs until you break out
  console.log(m.plottable ? `${m.distanceMm} mm  sigma ${m.sigmaMm.toFixed(1)}` : m.statusText);
}
```

## Hello-world — browser (Web Serial)

```ts
import { openDevice } from "@depz/sensor-sdk/web";
import { Vl53l0x } from "@depz/sensor-sdk";

connectBtn.addEventListener("click", async () => {
  const port = await navigator.serial.requestPort();     // user gesture
  const dev = await openDevice(port);
  if (!(dev instanceof Vl53l0x)) throw new Error("not a VL53L0X");

  await dev.init();
  await dev.configure({ budgetMs: 50, mode: "long-range" });
  await dev.startRanging();
  const unsub = dev.onMeasurement((m) => {
    if (m.plottable) render(m.distanceMm);              // callback: keep it quick
  });
});
```

## Configure

```ts
await dev.configure({ budgetMs: 50, interMs: 0, mode: "long-range" });   // re-init + apply
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

## Tuning

Only where `supports()` says so — after `configure()`, while not ranging:

```ts
const [count, isAperture] = await dev.performRefSpadManagement();  // re-measure reference SPADs
```

## What each driver supports

| driver | capability groups |
|---|---|
| `uld` | timing budget / inter-measurement period, named ranging modes, offset correction, offset calibration, crosstalk correction, crosstalk calibration, reference-SPAD management |

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
- **No thresholds, ROI or temperature update** on this ULD — `supports()` says so; the calls throw.
- **A NACK or two right after reset is normal** — the part is still booting; the SDK retries.
- **Callbacks run on the read-pump context** — keep them short; hand heavy work to a queue or use `measurements()`.
- **`timestampUs` is a `bigint`** (device µs). Use `syncTime()` + `toHostTimeUs()` to place measurements on the host clock.
