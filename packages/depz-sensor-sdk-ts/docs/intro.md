# JavaScript (TypeScript) SDK

`@depz/sensor-sdk` is the TypeScript SDK for the DEPZ USB sensor line
(HC-SR04 ultrasonic, VL53L4CD single-zone ToF, VL53L8CX/CH 8×8 ToF matrix,
BNO086 9-axis IMU). Pure ESM, fully typed, and it runs in two places:

**Working with one specific sensor?** Each has its own introduction, guide
and API reference:

- **[SR04](sr04/introduction.md)** — ultrasonic ranging: one distance per ping.
- **[VL53L4CD](vl53l4cd/introduction.md)** — single-zone ToF: one laser distance per sample.
- **[VL53L8CX](vl53l8cx/introduction.md)** — 8×8 ToF depth frames.
- **[VL53L8CH](vl53l8ch/introduction.md)** — the CX superset with CNH histograms.
- **[BNO086](bno086/introduction.md)** — 9-axis IMU: orientation and motion.

- **Browser (WebSerial)** — zero-install: the page talks to the sensor
  directly over `navigator.serial`; this is what powers the DEPZ web viewers.
- **Node.js (serialport)** — the same API on the server or in scripts;
  `serialport` is an optional peer dependency, pulled in only by the Node
  entry point.

It is contract-first: all wire behavior is pinned by the same
language-neutral protocol contracts and byte-exact golden vectors as the
Python SDK, so the two SDKs decode identically.

## Install

```bash
npm install @depz/sensor-sdk          # or: bun add @depz/sensor-sdk
npm install serialport                # Node only; not needed in the browser
```

## Quickstart (Node)

```ts
import { listDepzDevices, openDevice } from "@depz/sensor-sdk/node";
import { Sr04 } from "@depz/sensor-sdk";

for (const info of await listDepzDevices())
  console.log(info.path, info.sensorType, info.serialNumber);

const dev = await openDevice();        // first DEPZ device; returns the right class
if (dev instanceof Sr04) {
  await dev.setSamplePeriodUs(50_000); // 20 Hz
  await dev.start();
  for await (const m of dev.measurements())
    console.log(m.valid ? `${m.distanceMm} mm` : "no echo");
}
```

## Quickstart (browser)

```ts
import { openDevice } from "@depz/sensor-sdk/web";

// From a user gesture (button click): pick a port, get the right class back.
const port = await navigator.serial.requestPort();
const dev = await openDevice(port);
```

The device classes (`Sr04`, `Vl53l4Cd`, `Vl53l8Cx`, `Vl53l8Ch`, `Bno086`), streaming,
time sync, recording and firmware update mirror the Python SDK — one shared
protocol, one mental model.

## Where to next

- **Your sensor's pages** — [SR04](sr04/introduction.md) ·
  [VL53L4CD](vl53l4cd/introduction.md) ·
  [VL53L8CX](vl53l8cx/introduction.md) · [VL53L8CH](vl53l8ch/introduction.md) ·
  [BNO086](bno086/introduction.md): introduction, hands-on guide, and the
  sensor's own API reference.
- **[Guide](overview.md)** — the SDK-wide walkthrough: mental model,
  installation, discovery, multi-sensor recording, hardware-free testing.
- **[API Reference](api.md)** — the whole SDK's public surface.
- **[Python SDK docs](/developers/docs/sensors)** — the same concepts, same
  protocol, in Python.
- **Docs for LLMs** — the sensor SDK documentation as raw Markdown:
  [/llms-full.txt](/llms-full.txt).
- **Source & license** — the SDKs are open source (MIT):
  [github.com/depz-ai/depz-sensor-sdk](https://github.com/depz-ai/depz-sensor-sdk).
