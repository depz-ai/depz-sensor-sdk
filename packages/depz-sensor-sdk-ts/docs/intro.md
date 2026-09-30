# JavaScript (TypeScript) SDK

`@depz/sensor-sdk` is the TypeScript SDK for the DEPZ USB sensor line
(HC-SR04 ultrasonic, the VL53L0X / L1CX / L1CB / L3CX / L4CD / L4CX
single-zone ToF family, the VL53L5CX / L7CX / L7CH / L8CX / L8CH 8×8 ToF
matrices, the BNO086 and BNO055 9-axis IMUs). Pure ESM, fully typed, and it
runs in two places:

**Working with one specific sensor?** Each has its own introduction, guide
and API reference:

- **[SR04](sr04/introduction.md)** — ultrasonic ranging: one distance per ping.
- **[VL53L4CD](vl53l4cd/introduction.md)** — single-zone ToF: one laser distance per sample.
- **[VL53L8CX](vl53l8cx/introduction.md)** — 8×8 ToF depth frames.
- **[VL53L8CH](vl53l8ch/introduction.md)** — the CX superset with CNH histograms.
- **[VL53L5CX](vl53l5cx/introduction.md)** — 8×8 ToF, 63° view, I2C board.
- **[VL53L7CX](vl53l7cx/introduction.md)** — 8×8 ToF, 90° view, I2C board.
- **[VL53L7CH](vl53l7ch/introduction.md)** — the L7CX superset with streamed CNH histograms.
- **[VL53L0X](vl53l0x/introduction.md)** — single-zone ToF to ~2 m.
- **[VL53L1CX](vl53l1cx/introduction.md)** — single-zone ToF to ~4 m, ROI, multi-target histogram driver.
- **[VL53L1CB](vl53l1cb/introduction.md)** — the L1 die behind a cover glass, to ~8 m.
- **[VL53L3CX](vl53l3cx/introduction.md)** — multi-target ToF to ~3 m.
- **[VL53L4CX](vl53l4cx/introduction.md)** — multi-target ToF to ~6 m.
- **[BNO086](bno086/introduction.md)** — 9-axis IMU: orientation and motion.
- **[BNO055](bno055/introduction.md)** — 9-axis IMU with on-chip fusion: orientation, gravity, calibration.

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

The device classes (`Sr04`, `Vl53l4cd`, `Vl53l8cx`, `Vl53l8ch`, `Vl53l5cx`,
`Vl53l7cx`, `Vl53l7ch`, `Vl53l0x`, `Vl53l1cx`, `Vl53l1cb`, `Vl53l3cx`,
`Vl53l4cx`, `Bno086`, `Bno055`), streaming,
time sync, recording and firmware update match the Python SDK — one shared
protocol, one mental model.

## Where to next

- **Your sensor's pages** — [SR04](sr04/introduction.md) ·
  [VL53L4CD](vl53l4cd/introduction.md) ·
  [VL53L8CX](vl53l8cx/introduction.md) · [VL53L8CH](vl53l8ch/introduction.md) ·
  [VL53L5CX](vl53l5cx/introduction.md) · [VL53L7CX](vl53l7cx/introduction.md) ·
  [VL53L7CH](vl53l7ch/introduction.md) · [VL53L0X](vl53l0x/introduction.md) ·
  [VL53L1CX](vl53l1cx/introduction.md) · [VL53L1CB](vl53l1cb/introduction.md) ·
  [VL53L3CX](vl53l3cx/introduction.md) · [VL53L4CX](vl53l4cx/introduction.md) ·
  [BNO086](bno086/introduction.md) · [BNO055](bno055/introduction.md):
  introduction, hands-on guide, and the sensor's own API reference.
- **[Guide](overview.md)** — the SDK-wide walkthrough: mental model,
  installation, discovery, multi-sensor recording, hardware-free testing.
- **[API Reference](api.md)** — the whole SDK's public surface.
- **[Python SDK docs](/developers/docs/sensors)** — the same concepts, same
  protocol, in Python.
- **Docs for LLMs** — the sensor SDK documentation as raw Markdown:
  [/llms-full.txt](/llms-full.txt).
- **Source & license** — the SDKs are open source (MIT):
  [github.com/depz-ai/depz-sensor-sdk](https://github.com/depz-ai/depz-sensor-sdk).
