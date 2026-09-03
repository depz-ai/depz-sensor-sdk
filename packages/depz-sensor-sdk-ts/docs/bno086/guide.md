---
title: BNO086 — guide
description: Hello-world for the BNO086 IMU in the browser and Node, enabling reports, streaming, tare/calibration, FRS, diagnostics, and gotchas.
---

# BNO086 — guide

Step-by-step for the `Bno086` class. See [introduction](introduction.md) for
concepts and [overview](../overview.md) for discovery, recording, and common
device features.

## Hello-world — Node (orientation)

```ts
import { openDevice } from "@depz/sensor-sdk/node";
import { Bno086, SensorId } from "@depz/sensor-sdk";

const dev = await openDevice();
if (!(dev instanceof Bno086)) throw new Error("not a BNO086");

await dev.enableRotationVector(100);            // 100 Hz
for await (const r of dev.reports(SensorId.RotationVector)) {
  if (r.type === "RotationVector") {
    console.log(r.i, r.j, r.k, r.real, "acc:", r.accuracyRad);
  }
}
```

## Hello-world — browser (Web Serial)

```ts
import { openDevice } from "@depz/sensor-sdk/web";
import { Bno086, SensorId } from "@depz/sensor-sdk";

connectBtn.addEventListener("click", async () => {
  const port = await navigator.serial.requestPort();     // user gesture
  const dev = await openDevice(port);
  if (!(dev instanceof Bno086)) throw new Error("not a BNO086");

  await dev.enableRotationVector(100);
  const unsub = dev.onReport((r) => {
    if (r.type === "RotationVector") renderQuat(r);        // keep it quick
  }, SensorId.RotationVector);
});
```

## Enabling reports

```ts
await dev.enable(SensorId.Accelerometer, 100);   // any of the ~25 reports
await dev.enable(SensorId.RotationVector, undefined, { intervalUs: 10_000 });
await dev.disable(SensorId.Accelerometer);

// sugar for the everyday sensors
await dev.enableGameRotationVector(100);
await dev.enableGyroscope(200);
await dev.enableMagnetometer(50);
```

The hub rounds to its 1 kHz/2ⁿ grid; `enable()` reads the granted rate back and
**warns** (never throws) if it lands outside 0.9–2.1× the request. Pass
`{ verify: false }` to skip the read-back.

## Streaming

```ts
// bounded, drop-oldest iterator; optional SensorId filter
const it = dev.reports([SensorId.RotationVector, SensorId.Accelerometer], 1024);
const r = (await it.next()).value;               // discriminated Report union

// or a callback on the read-pump context
const unsub = dev.onReport((r) => queue.push(r));
```

Narrow the `Report` union by its `type` (`"RotationVector"`, `"Acceleration"`,
`"Magnetometer"`, `"StepCounter"`, `"GyroIntegratedRV"`, …). Raw wire integers
(`iRaw`, `xRaw`, …) sit next to the scaled values.

## Tare, calibration & FRS

```ts
await dev.tareNow(TareAxis.All, TareBasis.RotationVector);
await dev.persistTare();                          // store into FRS
await dev.setReorientation(0, 0, 0, 1);           // runtime reorientation quaternion

await dev.setCalibration({ accel: true, gyro: true, mag: true });
await dev.saveDcd();                              // persist dynamic calibration
const cal = await dev.getCalibration();

const words = await dev.frsRead(FrsRecordId.SystemOrientation);
await dev.frsWrite(FrsRecordId.SystemOrientation, words);
const md = await dev.getMetadata(SensorId.Accelerometer);
```

## Diagnostics

```ts
await dev.productId();                            // version, part/build numbers
await dev.getOscillatorType();
const errors = await dev.getErrors();             // hub error queue
const counts = await dev.getCounts(SensorId.RotationVector);
await dev.configurePeriodicDcd(true);
await dev.clearDcdAndReset();                     // wipe RAM calibration + reset
```

## Gotchas

- **Enable before you read** — a report only streams after `enable()`; nothing
  arrives otherwise.
- **Rate is a grant, not a promise** — the hub picks the nearest grid rate;
  read `enable()`'s returned `FeatureResponse.intervalUs` for the truth.
- **Callbacks run on the read-pump context** — don't call blocking device
  methods (`enable`, `tare`, `frsRead`) from inside one.
- **Correlation is at the SH-2 layer** — everything arrives as unsolicited
  bridge data; think in reports, not request/reply.
- **Reset is optional** — normal use is just `enable()` + read, no reset needed.
  `hardwareReset()` / `clearDcdAndReset()` restart SHTP sequence counters and
  cached features (re-`enable()` afterwards). On firmware newer than v0.95 the
  SH-2 reset-complete is emitted and the reset is confirmed reliably (ERRATA E9,
  now fixed); on older units (≤ v0.95) it was never emitted, so the call is
  best-effort — it returns on the command ack — and still safe there.
