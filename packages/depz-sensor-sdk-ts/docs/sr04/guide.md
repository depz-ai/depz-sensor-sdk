---
title: SR04 — guide
description: Hello-world for the SR04 ultrasonic sensor in the browser and Node, plus configuration, streaming, and gotchas.
---

# SR04 — guide

Step-by-step for the `Sr04` class. See [introduction](introduction.md) for the
concepts and [overview](../overview.md) for discovery, recording, and the
common device features shared by all sensors.

## Hello-world — Node

```ts
import { openDevice } from "@depz/sensor-sdk/node";
import { Sr04 } from "@depz/sensor-sdk";

const dev = await openDevice();          // auto-selects the DEPZ device
if (!(dev instanceof Sr04)) throw new Error("not an SR04");

await dev.setSamplePeriodUs(50_000);     // 20 Hz
await dev.start();                        // begin the measurement loop
for await (const m of dev.measurements()) {
  console.log(m.valid ? `${m.distanceMm!.toFixed(1)} mm` : "no echo");
}
```

## Hello-world — browser (Web Serial)

```ts
import { openDevice } from "@depz/sensor-sdk/web";
import { Sr04 } from "@depz/sensor-sdk";

document.querySelector("#connect")!.addEventListener("click", async () => {
  const port = await navigator.serial.requestPort();  // must be a user gesture
  const dev = await openDevice(port);
  if (!(dev instanceof Sr04)) throw new Error("not an SR04");

  await dev.start();
  const unsub = dev.onMeasurement((m) => {
    if (m.valid) render(m.distanceMm!);               // callback: keep it quick
  });
  // … later: unsub(); await dev.stop(); await dev.close();
});
```

## Configuration

```ts
await dev.getSamplePeriodUs();           // 50000 (µs)
await dev.setSamplePeriodUs(20_000);     // 50 Hz ceiling (echo window throttles)

await dev.getEchoDecayUs();              // settle pause, µs
const inEffect = await dev.setEchoDecayUs(1_000);  // device clamps → returns 4000
```

Configuration is safe at any time; it does not require stopping the loop.

## Measuring

**Single shot** — resolves when the echo completes (or times out at ~65.5 ms):

```ts
const m = await dev.measureOnce();       // rejects with BusyError while looping
console.log(m.source, m.echoTimeUs, m.distanceMm);   // "once", µs, mm | null
```

**Continuous loop** — `start()` / `stop()` are idempotent. Consume via a
callback or a bounded async iterator:

```ts
await dev.start();

// iterator: bounded, drop-oldest; exposes droppedCount
const it = dev.measurements(256);
const first = (await it.next()).value;   // Sr04Measurement

// callback: fires on the read-pump context
const unsub = dev.onMeasurement((m) => queue.push(m));

await dev.stop();
unsub();
```

Both paths also receive **unsolicited single shots** triggered by an AUX
`SYNC_IN` edge — those arrive tagged `source === "once"` even while the loop
runs.

## Gotchas

- **`measureOnce()` is busy during the loop** — it rejects with `BusyError`;
  `stop()` first, or just read the loop stream.
- **No-echo is not an error** — `valid === false` and `distanceMm === null`;
  handle it, don't treat null as 0.
- **Callbacks run on the read-pump context** — don't block; hand off to a queue
  or prefer `measurements()`.
- **`timestampUs` is a `bigint`** (device µs). Use `syncTime()` +
  `toHostTimeUs()` to place samples on the host clock.
