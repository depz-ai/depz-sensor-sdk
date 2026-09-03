---
title: VL53L4CD — guide
description: Hello-world for the VL53L4CD single-zone ToF sensor in the browser and Node, plus configuration, streaming, calibration, and gotchas.
---

# VL53L4CD — guide

Step-by-step for the `Vl53l4Cd` class. See [introduction](introduction.md) for
the concepts and [overview](../overview.md) for discovery, recording, and the
common device features shared by all sensors.

## Hello-world — Node

```ts
import { openDevice } from "@depz/sensor-sdk/node";
import { Vl53l4Cd } from "@depz/sensor-sdk";

const dev = await openDevice();          // auto-selects the DEPZ device
if (!(dev instanceof Vl53l4Cd)) throw new Error("not a VL53L4CD");

await dev.init();                        // ULD boot + VHV calibration, < 1 s
await dev.startRanging();                // default: 50 ms budget, continuous (~20 Hz)
for await (const m of dev.measurements()) {
  console.log(m.valid ? `${m.distanceMm} mm  sigma ${m.sigmaMm} mm` : m.statusText);
}
```

`init()` writes the ULD default configuration and runs VHV calibration. There
is no firmware download — the VL53L4CD carries its own — so it completes in
well under a second. It leaves the bridge's I2C bus at 1 MHz; pass
`init(400)` (or any step in `VL53L4_I2C_KHZ_STEPS`) to stay slower.

## Hello-world — browser (Web Serial)

```ts
import { openDevice } from "@depz/sensor-sdk/web";
import { Vl53l4Cd } from "@depz/sensor-sdk";

document.querySelector("#connect")!.addEventListener("click", async () => {
  const port = await navigator.serial.requestPort();  // must be a user gesture
  const dev = await openDevice(port);
  if (!(dev instanceof Vl53l4Cd)) throw new Error("not a VL53L4CD");

  await dev.init();
  await dev.startRanging();
  const unsub = dev.onMeasurement((m) => {
    if (m.valid) render(m.distanceMm);                // callback: keep it quick
  });
  // … later: unsub(); await dev.stopRanging(); await dev.close();
});
```

## Configuration

Call these **after `init()` and while not ranging** (they throw otherwise).

```ts
// timing: budget 10–200 ms; interMeasurement 0 = continuous,
// > budget = autonomous low-power mode
await dev.setRangeTiming(50, 0);         // 50 ms budget, back-to-back (~20 Hz)
await dev.setRangeTiming(200, 1000);     // autonomous: one 200 ms sample per second
await dev.getRangeTiming();              // → { timingBudgetMs, interMeasurementMs }

// corrections
await dev.setOffsetMm(-7);               // signed ranging offset
await dev.setXtalkKcps(12);              // crosstalk compensation (0 = disabled)

// quality limits — measurements outside them report a non-zero rangeStatus
await dev.setSignalThresholdKcps(1024);  // discard weak returns
await dev.setSigmaThresholdMm(15);       // discard noisy ranges (<= 16383)

// distance-window interrupt: INT only fires when the condition holds
import { WINDOW_IN } from "@depz/sensor-sdk";
await dev.setDetectionThresholds(100, 300, WINDOW_IN);  // only 100–300 mm
```

Every setter has a matching getter that reads the value back from the sensor.
After a >8 °C ambient change, call `startTemperatureUpdate()` to re-run VHV
calibration.

## Single shot vs streaming

```ts
// one-shot: start, wait for data-ready, read, stop — all in one call
const m = await dev.measureOnce(1000);   // rejects while the stream is running

// streaming: the sensor's INT pin drives one report per measurement
await dev.startRanging();
// … consume dev.measurements() / dev.getMeasurement() …
await dev.stopRanging();
```

`measureOnce()` rejects while the stream is running — stop it first, or just
consume streamed measurements. While ranging, three consumption styles:

```ts
// iterator — bounded, drop-oldest; subscribes immediately
const it = dev.measurements(64);
const first = (await it.next()).value;   // Vl53l4Measurement
console.log("dropped so far:", it.droppedCount);

// callback — fires on the read-pump context; keep it quick
const unsub = dev.onMeasurement((m) => queue.push(m));

// convenience: wait for the next measurement
const next = await dev.getMeasurement(2000);  // DepzTimeoutError on silence
```

`dev.streamParseErrors` counts stream reports whose result block failed to
decode — 0 in a healthy session.

## The measurement

`Vl53l4Measurement` mirrors the ULD's `VL53L4CD_ResultsData_t` plus the MCU
timestamp of the INT edge:

| field | meaning |
|---|---|
| `timestampUs` | MCU uptime at the INT edge (stream) / read (poll), `bigint` µs |
| `rangeStatus` | 0 = valid; see `RANGE_STATUS_NAMES` / `statusText` |
| `distanceMm` | measured distance, mm |
| `sigmaMm` | range std-dev estimate, mm |
| `signalRateKcps` / `signalPerSpadKcps` | return-signal rate |
| `ambientRateKcps` / `ambientPerSpadKcps` | ambient-light rate |
| `numberOfSpad` | SPADs used |
| `streamCount` | sensor frame counter, wraps at 255 |

Check `m.valid` (i.e. `rangeStatus === 0`) before trusting a distance — a
non-zero status (signal too low, sigma too high, wrap-around…) is a
legitimate reading, not a protocol error.

## Calibration

Both calibrations block for a sample burst against a target at a known
distance, program the sensor, and return the value now in effect:

```ts
const offset = await dev.calibrateOffset(100);   // target at 10–1000 mm
const xtalk = await dev.calibrateXtalk(600);     // target at 10–5000 mm
```

The programmed values do **not** survive an XSHUT power-cycle; read them back
(`getOffsetMm()` / `getXtalkKcps()`) and re-apply them after `init()` if you
need persistence.

## XSHUT: power and reset

The bridge drives the sensor's XSHUT (shutdown) pin:

```ts
import { VL53L4_XSHUT_OFF, VL53L4_XSHUT_ON } from "@depz/sensor-sdk";

await dev.resetSensor();            // power-cycle (~3 ms on the MCU)
await dev.xshut(VL53L4_XSHUT_OFF);  // sensor off — stream stops, config lost
await dev.xshut(VL53L4_XSHUT_ON);   // sensor back on, in its boot state
```

A power-cycled sensor holds **none** of the ULD configuration: `initialized`
goes `false` and everything — timing, offset, xtalk, thresholds — is back at
sensor defaults. Call `init()` and reconfigure before ranging again.

## Bridge diagnostics

`bridgeInfo()` returns the MCU's own view of the sensor (`Vl53l4Info`) and is
safe to call while streaming:

```ts
const info = await dev.bridgeInfo();
info.modelId;       // 0xEBAA on a live VL53L4CD
info.intEdges;      // INT edges seen — should grow while ranging
info.slotsSkipped;  // stream slots the MCU could not service
info.i2cErrors;     // bus errors, with lastI2cError naming the latest
info.i2cKhz;        // the programmed bus-speed step
```

Counters are free-running and wrap silently — watch increments, not absolute
values.

## Gotchas

- **`init()` after every power-cycle.** XSHUT off/reset wipes the ULD config;
  the SDK tracks this via `dev.initialized`.
- **Config while ranging throws** — the INT-driven stream owns the register
  bank; `stopRanging()`, reconfigure, `startRanging()`.
- **Always check `m.valid`.** Non-zero `rangeStatus` values are data
  ("signal below threshold", "wrapped target"…), not errors.
- **`interMeasurementMs` must be 0 or > the budget** — values between 1 and
  the budget are rejected by the ULD.
- **`measureOnce()` while streaming rejects** — one ranging engine; stop the
  stream first.
- **Callbacks run on the read-pump context** — don't block; hand off to a
  queue or prefer `measurements()`.
- **`timestampUs` is a `bigint`** (device µs). Use `syncTime()` +
  `toHostTimeUs()` to place samples on the host clock.
