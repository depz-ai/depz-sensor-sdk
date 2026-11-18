# @depz/sensor-sdk

TypeScript SDK for the **DEPZ USB sensor line** — fifteen sensor classes
across the USB CDC-ACM firmware families that speak one shared framed protocol,
usable from **the browser (WebSerial)** and **Node (serialport)**:

- **HC-SR04** — ultrasonic distance
- **VL53L0X / VL53L1CX / VL53L1CB / VL53L3CX / VL53L4CX** — the single-zone
  Time-of-Flight family on one I2C register bridge (host runs ST's ULD, ULP or
  histogram Bare Driver, up to four targets per frame)
- **VL53L4CD** — single-zone Time-of-Flight (host runs ST ULD 2.2.3)
- **VL53L5CX / VL53L7CX / VL53L7CH** — the same 8×8 ULD family on an I2C board;
  `Vl53l7ch` adds streamed Compact-Network-Histogram output
- **VL53L8CX / VL53L8CH** — 8×8 multizone Time-of-Flight (host runs the ST ULD);
  `Vl53l8ch` is the `Vl53l8cx` superset, adding Compact-Network-Histogram output
- **TMF8829** — multizone direct Time-of-Flight, 8×8 … 48×32 zones, up to 4
  peaks per zone, raw histograms (host runs the sensor over a register bridge)
- **BNO055** — 9-axis IMU with on-chip fusion (host configures it over a
  register bridge)
- **BNO085 / BNO086** — 9-axis IMU (host runs the SH-2 stack)

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

From a checkout of the repository, the same package from its sources (Node 18
or newer): build the tarball, then install it into your project by its path.

<!-- doc-check: skip — a build and an install from a checkout, not from the registry -->
```bash
cd packages/depz-sensor-sdk-ts
npm install          # the build's tools (TypeScript)
npm run build        # dist/
npm pack             # depz-sensor-sdk-<version>.tgz
# then, in your project:
npm install /path/to/packages/depz-sensor-sdk-ts/depz-sensor-sdk-<version>.tgz serialport
```

## Quick start (browser, WebSerial)

```ts
import { openDevice } from "@depz/sensor-sdk/web";

const port = await navigator.serial.requestPort(); // user gesture
const dev = await openDevice(port);
// dev is the correctly-typed sensor class (Sr04 | Vl53l4cd | Vl53l8cx/ch |
// Vl53l5cx | Vl53l7cx/ch | Vl53l0x … Vl53l4cx | Tmf8829 | Bno086 | Bno055)
```

## Quick start (Node)

A first program — `first.mjs` in a project with `"type": "module"`, or any
`.mjs` file — run with `node first.mjs`:

```ts
import { listDepzDevices, openDevice } from "@depz/sensor-sdk/node";

const found = await listDepzDevices();   // the DEPZ boards on this machine, probed
if (found.length === 0) {
  console.log("no DEPZ board found");
} else {
  for (const info of found) console.log(info.port, info.sensorType, info.serialNumber);
  const dev = await openDevice();        // default: the DEPZ device with the smallest serial
  console.log(await dev.getSoftwareName());
  await dev.close();
}
```

With no board attached it prints `no DEPZ board found`; `openDevice()` alone
would throw `NoDepzDeviceError`. On Linux `serialport` lists the ports by
running `udevadm`, which every desktop distribution has; a minimal container
needs it installed (`apt-get install udev`), and without it the listing throws
a `DepzError` that says so. The user must be able to open the port (group
`dialout`).

## Selecting a device (contract 02 §4.3)

Selection is by **USB identity**: only ports with a known DEPZ (vid,pid) are
considered, ordered by serial — the same rules across Node and browser (in the
browser the serial comes from a `GET_SERIAL` probe of granted ports).

```ts
await openDevice();                         // default — smallest serial
await openDevice(0);                        // by INDEX — Nth candidate, sorted by serial
await openDevice(1);                        // second SR04 when two are plugged in
await openDevice(undefined, { serial: "UEXAMPLE01" }); // by exact USB SERIAL
await openDevice("/dev/ttyACM0");           // explicit port path (Node)

for (const info of await listDepzDevices()) // enumerate candidates first
  console.log(info.port, info.sensorType, info.serialNumber, info.usbSerial);
```

No DEPZ candidate → throws (never falls back to the first system port), matching
the Python `open_device()` semantics.

Entry points: `@depz/sensor-sdk` (core, transport-agnostic), `@depz/sensor-sdk/web`
(WebSerial), `@depz/sensor-sdk/node` (serialport). See `docs/` for the per-sensor
guides and the generated API reference.

## Compatibility after 1.0

From 1.0 a minor release only adds (contract 07 §5). An object type the SDK
*produces* (`DeviceInfo`, `LinkStats`, `TimeSync`, every measurement / frame /
sample / report, `Identity`, `Tmf8829Config`, the SH-2 replies, `DatasetRecord`)
may gain a field, required or not — a reader is unaffected. An option type
*you* pass in (`OpenDeviceOptions`, `ListOptions`, `DeviceOptions`,
`Tmf8829ConfigureOptions`, `Vl53l8InitOptions`, `Vl53lxConfigureOptions`,
`Bno055ConfigureOptions`, `EnableOptions`, `DatasetPlayerOptions`, …) gains
only **optional** fields, so an object literal you wrote keeps type-checking.
A value type that is both produced and built by you is taken as a
`Partial<…>` where you pass it: a field left out of `Bno055Units` is `false`,
of `Bno055AxisRemap` its P1 value, of `Bno055AccelConfig` / `Bno055GyroConfig` /
`Bno055MagConfig` its power-on register code. Two value types keep every
field required because every field is essential: `SyncPinConfig` (pin, mode,
polarity) and `Bno055CalibrationProfile` (the 22 bytes the sensor reads
back). The VL53L8 `DetectionThreshold` already follows the same pattern:
`setDetectionThresholds` takes `Partial<DetectionThreshold>[]` (a field left
out is 0), `getDetectionThresholds` returns every field. A union (`DeviceEvent`, the
BNO086 `Report`, `PlayerState`) and a string set (`SensorType`, the sensor
class names, a `DatasetRecord`'s `kind`) may gain a member: a `switch` on it
keeps a `default` branch — an exhaustiveness check (`assertNever(ev)`,
`const _: never = ev`) in that branch is a compile error on the next minor
release.

<!-- doc-check: skip — right/wrong pair with `…` for the user's code; the second line fails on purpose -->
```ts
switch (ev.type) { case "disconnected": …; default: break; }               // right
switch (ev.type) { case "disconnected": …; default: assertNever(ev); }     // wrong: fails to compile when a kind is added
```

## License

MIT. The bundled VL53L5 / VL53L7 / VL53L8 sensor firmware, configuration
data and the driver code ported from ST's Ultra Lite Driver are
© STMicroelectronics (BSD-3-Clause,
[`licenses/vl53l8/LICENSE-BSD-3-Clause.txt`](licenses/vl53l8/LICENSE-BSD-3-Clause.txt)); the bundled TMF8829 sensor firmware is © ams-OSRAM (MIT,
`TMF8829_FIRMWARE_LICENSE`).

Open source — source and issue tracker:
<https://github.com/depz-ai/depz-sensor-sdk>.
