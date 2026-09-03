---
title: Overview & getting started
description: Mental model, installation, discovery, multi-sensor recording, and hardware-free testing for the DEPZ TypeScript sensor SDK.
---

# @depz/sensor-sdk — overview

TypeScript SDK for the DEPZ USB sensor line — **SR04** ultrasonic,
**VL53L4CD** single-zone time-of-flight, **VL53L8CX/CH** 8×8 time-of-flight,
**BNO086** 9-axis IMU. One codebase runs
in the **browser** (Web Serial) and in **Node** (via the optional `serialport`
peer dependency).

For the exhaustive symbol-by-symbol reference see [api.md](api.md). Each sensor
also has its own pages (the 8×8 ToF is two parts — the `Vl53l8Cx` base and the
`Vl53l8Ch` CNH superset):

- SR04 — [introduction](sr04/introduction.md) · [guide](sr04/guide.md) · [api](sr04/api.md)
- VL53L4CD — [introduction](vl53l4cd/introduction.md) · [guide](vl53l4cd/guide.md) · [api](vl53l4cd/api.md)
- VL53L8CX — [introduction](vl53l8cx/introduction.md) · [guide](vl53l8cx/guide.md) · [api](vl53l8cx/api.md)
- VL53L8CH — [introduction](vl53l8ch/introduction.md) · [guide](vl53l8ch/guide.md) · [api](vl53l8ch/api.md)
- BNO086 — [introduction](bno086/introduction.md) · [guide](bno086/guide.md) · [api](bno086/api.md)

## What it is

Each sensor is a USB CDC-ACM device speaking one shared framed protocol
(`A5 C3` header + CRC). The SDK gives you a typed object per sensor with a
background read pump; you configure it and consume a stream of decoded,
timestamped results. Five sensor classes across three philosophies baked into
the firmware and mirrored here (the three ToF classes share one philosophy):

- **SR04** — the device does the ranging; you get `echoTimeUs` → distance.
- **VL53L4CD** — the device is a thin I2C register bridge; the full ST ULD
  2.2.3 driver runs **here on the host** (`sensors/vl53l4/uld.ts`). One
  laser distance per sample, INT-driven streaming.
- **VL53L8CX / VL53L8CH** — the device is a thin SPI bridge; the full ST ULD
  driver runs **here on the host** (`sensors/vl53l8/uld.ts`). `Vl53l8Ch` is the
  `Vl53l8Cx` superset, adding Compact-Network-Histogram output.
- **BNO086** — the device is an SHTP pass-through; the full SH-2 stack runs
  **here on the host** (`sensors/bno086`).

## Installation

```bash
npm install @depz/sensor-sdk
npm install serialport            # Node only; optional peer dependency
```

- **Browser**: import from `@depz/sensor-sdk/web`. Web Serial needs a secure
  context (https / localhost) and Chrome/Edge. The app obtains a port inside a
  user gesture with `navigator.serial.requestPort()` — the SDK never opens the
  picker itself.
- **Node**: import from `@depz/sensor-sdk/node`. Requires `serialport`. On
  Linux the user must be able to open the port (group `dialout`), and
  `ModemManager` can grab CDC-ACM devices — disable it or add a udev rule if a
  device is present but every open times out.
- The root import `@depz/sensor-sdk` is browser-safe (no Node/serialport
  references) and carries the protocol, sensor classes, dataset, and codecs.

## Mental model

```
openDevice(target)  ──►  Sr04 | Vl53l4Cd | Vl53l8Cx | Vl53l8Ch | Bno086   (a DepzDevice)
                          │
                 background read pump
                          │
        ┌─────────────────┼──────────────────┐
   request/reply       callbacks         async iterators
  (correlated by      (onMeasurement/     (measurements()/
   echoed cmd byte)    onFrame/onReport)   frames()/reports();
                                           bounded, drop-oldest)
```

- **One read pump per device** drains the transport, parses frames, and
  dispatches. Solicited replies are correlated by the **echoed command byte**
  (not by sequence number); the default timeout is 200 ms.
- **Streams** come two ways: register a callback (fires on the read-pump
  context — don't block it) *or* pull from a bounded, drop-oldest async
  iterator that exposes `droppedCount`.
- **Timestamps are device microseconds as `bigint`** (`timestampUs`). Call
  `syncTime()` once, then `toHostTimeUs()` maps them onto the host monotonic
  clock — this is also what makes multi-device recordings share one timeline.

`open()` is implicit in `openDevice()`. If you construct a sensor class
directly over a transport, call `await dev.open()` before use and
`await dev.close()` when done.

## Discovery

Selection is by **USB iSerial** in Node and by the **device serial**
(`GET_SERIAL`, read over the protocol) in the browser — never "first port in
system order". Two identical sensors are told apart by serial.

```ts
// Node
import { listDepzDevices, openDevice } from "@depz/sensor-sdk/node";

for (const info of await listDepzDevices()) {
  console.log(info.path, info.sensorType, info.softwareName, info.serialNumber);
}
const dev = await openDevice();                 // smallest USB iSerial
const byIndex = await openDevice(1);            // Nth candidate, sorted by serial
const byPath = await openDevice("/dev/ttyACM0");
const bySerial = await openDevice(undefined, { serial: "SN000005" });
```

```ts
// Browser — obtain a port in a click handler, then hand it in
import { openDevice } from "@depz/sensor-sdk/web";

button.addEventListener("click", async () => {
  const port = await navigator.serial.requestPort();
  const dev = await openDevice(port);           // probes → right subclass
});
```

`openDevice()` returns the correct subclass (`Sr04` / `Vl53l4Cd` / `Vl53l8Cx` /
`Vl53l8Ch` / `Bno086`); narrow it with `instanceof`. No candidate throws
`NoDepzDeviceError`.

## Common device features

Available on every sensor (they all extend `DepzDevice`):

```ts
await dev.readMcuTemperature();                 // °C
const sync = await dev.syncTime(5);             // NTP-style; sync.offsetUs / rttUs (bigint µs)
dev.toHostTimeUs(frame.timestampUs);            // device µs → host µs
await dev.getSyncPin(1);
await dev.setSyncPin({ pin: 1, mode, polarity });   // AUX sync pins
const unsub = dev.onEvent((ev) => console.log(ev)); // seqError, temperature, text, disconnected, …
await dev.reset();                              // reboots the device (link drops)
```

## Multi-sensor recording & replay

Two layers:

```ts
// Decoded, multi-device, time-synced dataset (.depzdata)
import { DatasetRecorder, DatasetReader, DatasetPlayer } from "@depz/sensor-sdk";

const rec = new DatasetRecorder({ note: "bench run" });
await rec.add(sr04, "range");                   // runs syncTime; add several devices
await rec.add(imu, "imu");
rec.start();
// … drive the devices …
rec.stop();
const text = rec.dump();                        // JSONL file content

const reader = new DatasetReader(text);         // records merged by host time
for (const r of reader.records) console.log(r.deviceId, r.kind, r.tHostUs);

const player = new DatasetPlayer(reader);       // paced play/pause/seek/speed
player.onRecord((r) => render(r));
player.play();
```

`syncTimeAll(devices)` syncs a set of devices onto the shared host clock in one
call. The raw-byte layer (`RecordingTransport` / `ReplayTransport`, `.depzrec`)
records/replays the wire bytes for protocol regression and hardware-free tests.

## Testing without hardware

Every layer takes a `SerialTransport`, so full sensor classes drive off a
`LoopbackTransport` (in-process fake firmware) or a recorded `ReplayTransport`
session — no serial port required. That is how this SDK's own suite runs,
including a real VL53L8 capture replayed end-to-end.

## Gotchas

- **VL53L8 `init()` takes a few seconds** (~25 s over some CDC stacks) — it
  downloads the 84 KB sensor firmware every power-up. Show progress; it is not
  a hang.
- **VL53L8 ranging must be ≥ 2 Hz** — below that the sensor never enters its
  ranging loop and streams nothing.
- **VL53L8 config while ranging throws** — the stream owns the register bank;
  `stopRanging()`, reconfigure, `startRanging()`.
- **Callbacks run on the read-pump context** — don't block them; hand heavy
  work to a queue or use the async iterators.
- **BNO086 correlation is at the SH-2 layer**, not the transport (the bridge
  returns everything as unsolicited `RPT_DATA`) — you talk in reports, not
  request/reply.
- **Browser can't enumerate ports or read USB iSerial** — selection is by
  device serial obtained by probing each granted port once.
