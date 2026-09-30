---
title: Overview & getting started
description: Mental model, installation, discovery, multi-sensor recording, and hardware-free testing for the DEPZ TypeScript sensor SDK.
---

# @depz/sensor-sdk — overview

TypeScript SDK for the DEPZ USB sensor line — **SR04** ultrasonic,
**VL53L4CD** and the **VL53L0X / L1CX / L1CB / L3CX / L4CX** single-zone
time-of-flight family, **VL53L8CX/CH** and **VL53L5CX / L7CX / L7CH** 8×8
time-of-flight, **BNO086** and **BNO055** 9-axis IMUs. One codebase runs
in the **browser** (Web Serial) and in **Node** (via the optional `serialport`
peer dependency).

For the exhaustive symbol-by-symbol reference see [api.md](api.md). Each sensor
also has its own pages (the SPI 8×8 ToF is two parts — the `Vl53l8cx` base and
the `Vl53l8ch` CNH superset; the I2C 8×8 board is three — `Vl53l5cx`,
`Vl53l7cx`, `Vl53l7ch`):

- SR04 — [introduction](sr04/introduction.md) · [guide](sr04/guide.md) · [api](sr04/api.md)
- VL53L4CD — [introduction](vl53l4cd/introduction.md) · [guide](vl53l4cd/guide.md) · [api](vl53l4cd/api.md)
- VL53L8CX — [introduction](vl53l8cx/introduction.md) · [guide](vl53l8cx/guide.md) · [api](vl53l8cx/api.md)
- VL53L8CH — [introduction](vl53l8ch/introduction.md) · [guide](vl53l8ch/guide.md) · [api](vl53l8ch/api.md)
- VL53L5CX — [introduction](vl53l5cx/introduction.md) · [guide](vl53l5cx/guide.md) · [api](vl53l5cx/api.md)
- VL53L7CX — [introduction](vl53l7cx/introduction.md) · [guide](vl53l7cx/guide.md) · [api](vl53l7cx/api.md)
- VL53L7CH — [introduction](vl53l7ch/introduction.md) · [guide](vl53l7ch/guide.md) · [api](vl53l7ch/api.md)
- VL53L0X — [introduction](vl53l0x/introduction.md) · [guide](vl53l0x/guide.md) · [api](vl53l0x/api.md)
- VL53L1CX — [introduction](vl53l1cx/introduction.md) · [guide](vl53l1cx/guide.md) · [api](vl53l1cx/api.md)
- VL53L1CB — [introduction](vl53l1cb/introduction.md) · [guide](vl53l1cb/guide.md) · [api](vl53l1cb/api.md)
- VL53L3CX — [introduction](vl53l3cx/introduction.md) · [guide](vl53l3cx/guide.md) · [api](vl53l3cx/api.md)
- VL53L4CX — [introduction](vl53l4cx/introduction.md) · [guide](vl53l4cx/guide.md) · [api](vl53l4cx/api.md)
- BNO086 — [introduction](bno086/introduction.md) · [guide](bno086/guide.md) · [api](bno086/api.md)
- BNO055 — [introduction](bno055/introduction.md) · [guide](bno055/guide.md) · [api](bno055/api.md)

## What it is

Each sensor is a USB CDC-ACM device speaking one shared framed protocol
(`A5 C3` header + CRC). The SDK gives you a typed object per sensor with a
background read pump; you configure it and consume a stream of decoded,
timestamped results. The sensor classes follow a few philosophies baked into
the firmware and reflected here (every ToF class shares one: the sensor
driver runs on the host):

- **SR04** — the device does the ranging; you get `echoTimeUs` → distance.
- **VL53L4CD** — the device is a thin I2C register bridge; the full ST ULD
  2.2.3 driver runs **here on the host** (`sensors/vl53l4/uld.ts`). One
  laser distance per sample, INT-driven streaming.
- **VL53L8CX / VL53L8CH** — the device is a thin SPI bridge; the full ST ULD
  driver runs **here on the host** (`sensors/vl53l8/uld.ts`). `Vl53l8ch` is the
  `Vl53l8cx` superset, adding Compact-Network-Histogram output.
- **VL53L5CX / VL53L7CX / VL53L7CH** — the same ULD family as the VL53L8 on
  an I2C bridge (`sensors/vl53l7`); the classes subclass `Vl53l8cx`, and
  `Vl53l7ch` adds streamed CNH.
- **VL53L0X / L1CX / L1CB / L3CX / L4CX** — one I2C bridge firmware that knows
  no sensor; every ST driver (ULD, ULP, the histogram Bare Driver) runs
  **here on the host** (`sensors/vl53lx`). One class per product on the family
  class `Vl53lx`.
- **BNO086** — the device is an SHTP pass-through; the full SH-2 stack runs
  **here on the host** (`sensors/bno086`).
- **BNO055** — the device is an I2C register bridge; the sensor fuses on chip
  and the host configures it with register writes (`sensors/bno055`).

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
  Linux the user must be able to open the port (group `dialout`).
- The root import `@depz/sensor-sdk` is browser-safe (no Node/serialport
  references) and carries the protocol, sensor classes, dataset, and codecs.

## Mental model

```
openDevice(target)  ──►  Sr04 | Vl53l4cd | Vl53l8cx/ch | Vl53l5cx | Vl53l7cx/ch
                          | Vl53l0x…Vl53l4cx | Bno086 | Bno055       (a DepzDevice)
                          │
                 background read pump
                          │
        ┌─────────────────┼──────────────────┐
   request/reply       callbacks         async iterators
  (correlated by      (onMeasurement/     (measurements()/
   echoed cmd byte)    onFrame/onReport/   frames()/reports()/
                       onSample)           samples(); bounded,
                                           drop-oldest)
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

`openDevice()` returns the correct subclass (`Sr04` / `Vl53l4cd` / `Vl53l8cx` /
`Vl53l8ch` / `Vl53l5cx` / `Vl53l7cx` / `Vl53l7ch` / `Vl53l0x` / `Vl53l1cx` /
`Vl53l1cb` / `Vl53l3cx` / `Vl53l4cx` / `Bno086` / `Bno055`); narrow it with
`instanceof`. Boards that share one firmware (the three I2C 8×8 boards, the
five 1D-family boards) are told apart by their USB id, then by the device
name stamped on the board. No candidate throws `NoDepzDeviceError`.

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

- **8×8 ToF `init()` takes seconds** — it downloads the 84 KB sensor firmware
  every power-up (VL53L8: a few seconds, ~25 s over some CDC stacks; the I2C
  L5/L7 board: ~1.4 s). Show progress; it is not a hang.
- **VL53L8 ranging must be ≥ 2 Hz** — below that the sensor never enters its
  ranging loop and streams nothing. The L5/L7 range down to 1 Hz.
- **ToF config while ranging throws** — the stream owns the register bank (or
  the bus); `stopRanging()`, reconfigure, `startRanging()`. On the 1D family
  call `configure()` before every run.
- **BNO055 settings take in CONFIG mode** — use the typed setters or
  `configure()`; page-1 access throws while a stream runs.
- **Callbacks run on the read-pump context** — don't block them; hand heavy
  work to a queue or use the async iterators.
- **BNO086 correlation is at the SH-2 layer**, not the transport (the bridge
  returns everything as unsolicited `RPT_DATA`) — you talk in reports, not
  request/reply.
- **Browser can't enumerate ports or read USB iSerial** — selection is by
  device serial obtained by probing each granted port once.
