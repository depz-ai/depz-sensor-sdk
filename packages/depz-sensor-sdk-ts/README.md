# @depz/sensor-sdk

TypeScript SDK for the **DEPZ USB sensor line** — fourteen sensor classes
across the USB CDC-ACM firmware families that speak one shared framed protocol,
usable from **the browser (WebSerial)** and **Node (serialport)**:

- **HC-SR04** — ultrasonic distance
- **VL53L4CD** — single-zone Time-of-Flight (host runs ST ULD 2.2.3)
- **VL53L0X / VL53L1CX / VL53L1CB / VL53L3CX / VL53L4CX** — the single-zone
  Time-of-Flight family on one I2C register bridge (host runs ST's ULD, ULP or
  histogram Bare Driver, up to four targets per frame)
- **VL53L8CX / VL53L8CH** — 8×8 multizone Time-of-Flight (host runs the ST ULD);
  `Vl53l8ch` is the `Vl53l8cx` superset, adding Compact-Network-Histogram output
- **VL53L5CX / VL53L7CX / VL53L7CH** — the same 8×8 ULD family on an I2C board;
  `Vl53l7ch` adds streamed Compact-Network-Histogram output
- **BNO086** — 9-axis IMU (host runs the SH-2 stack)
- **BNO055** — 9-axis IMU with on-chip fusion (host configures it over a
  register bridge)

Pure ESM, zero runtime dependencies on the browser path (`serialport` is an
optional peer used only by the `./node` entry). Mirrors the Python
[`depz-sensor-sdk`](https://pypi.org/project/depz-sensor-sdk/) byte-for-byte via
shared golden test vectors.

## Install

```bash
npm install @depz/sensor-sdk
# for Node serial access also:
npm install serialport
```

## Quick start (browser, WebSerial)

```ts
import { openDevice } from "@depz/sensor-sdk/web";

const port = await navigator.serial.requestPort(); // user gesture
const dev = await openDevice(port);
// dev is the correctly-typed sensor class (Sr04 | Vl53l4cd | Vl53l8cx/ch |
// Vl53l5cx | Vl53l7cx/ch | Vl53l0x … Vl53l4cx | Bno086 | Bno055)
```

## Quick start (Node)

```ts
import { openDevice, listDepzDevices } from "@depz/sensor-sdk/node";

const dev = await openDevice();          // default: the DEPZ device with the smallest serial
```

## Selecting a device (contract 02 §4.3)

Selection is by **USB identity**: only ports with a known DEPZ (vid,pid) are
considered, ordered by serial — the same rules across Node and browser (in the
browser the serial comes from a `GET_SERIAL` probe of granted ports).

```ts
await openDevice();                         // default — smallest serial
await openDevice(0);                        // by INDEX — Nth candidate, sorted by serial
await openDevice(1);                        // second SR04 when two are plugged in
await openDevice(undefined, { serial: "UMPVOB2461" }); // by exact USB SERIAL
await openDevice("/dev/ttyACM0");           // explicit port path (Node)

for (const info of await listDepzDevices()) // enumerate candidates first
  console.log(info.path, info.sensorType, info.serialNumber);
```

No DEPZ candidate → throws (never falls back to the first system port), matching
the Python `open_device()` semantics.

Entry points: `@depz/sensor-sdk` (core, transport-agnostic), `@depz/sensor-sdk/web`
(WebSerial), `@depz/sensor-sdk/node` (serialport). See `docs/` for the per-sensor
guides and the generated API reference.

## License

MIT. Bundled VL53L5/L7/L8 sensor-firmware blobs are © STMicroelectronics
(BSD-3-Clause).

Open source — source and issue tracker:
<https://github.com/depz-ai/depz-sensor-sdk>.
