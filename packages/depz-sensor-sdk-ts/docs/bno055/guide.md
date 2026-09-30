---
title: BNO055 — guide
description: Hello-world for the BNO055 IMU in Node and the browser — configure, stream orientation, the sample, modes and power, units, axis remap, calibration profiles, self-test, page-1 settings and interrupts, reset, and gotchas.
---

# BNO055 — guide

Hands-on guide to the `Bno055` device class. For what the sensor is and its
concepts, read the [introduction](introduction.md); for discovery, recording
and the common device features see the [overview](../overview.md); for exact
signatures the [API reference](api.md).

## Contents

- [Open and configure](#open-and-configure)
- [Hello-world — Node (orientation)](#hello-world--node-orientation)
- [Hello-world — browser (Web Serial)](#hello-world--browser-web-serial)
- [The sample](#the-sample)
- [Streaming](#streaming)
- [Modes and power](#modes-and-power)
- [Units](#units)
- [Axis remap](#axis-remap)
- [Calibration](#calibration)
- [Status and self-test](#status-and-self-test)
- [Page 1: sensor configs, interrupts, unique id](#page-1-sensor-configs-interrupts-unique-id)
- [Reset and bridge diagnostics](#reset-and-bridge-diagnostics)
- [Gotchas](#gotchas)

## Open and configure

```ts
import { openDevice } from "@depz/sensor-sdk/node";
import { Bno055 } from "@depz/sensor-sdk";

const dev = await openDevice();                  // returns a Bno055
if (!(dev instanceof Bno055)) throw new Error("not a BNO055");
await dev.configure();                           // CONFIG → units → NDOF, fusion live on return
```

`configure()` is the usual session setup in one call: switch to CONFIG, write
the units, optionally an axis remap and a stored calibration profile, then
switch to the operating mode (NDOF by default). In a fusion mode it resolves
only once the fusion outputs are live — they read zero for ~70 ms after every
switch out of CONFIG. The SDK remembers the options for
`restoreConfiguration()`.

```ts
import { BNO055_DEFAULT_UNITS, Bno055OprMode } from "@depz/sensor-sdk";

await dev.configure({
  mode: Bno055OprMode.Imu,                          // accel + gyro, no magnetometer
  units: { ...BNO055_DEFAULT_UNITS, eulerRad: true },  // Euler angles in radians
  axisRemap: "P2",                                  // datasheet mounting P2
});
```

## Hello-world — Node (orientation)

```ts
import { openDevice } from "@depz/sensor-sdk/node";
import { Bno055 } from "@depz/sensor-sdk";

const dev = await openDevice();
if (!(dev instanceof Bno055)) throw new Error("not a BNO055");

await dev.configure();
await dev.startStream(10);                       // 100 Hz, the full 46-byte block
for await (const s of dev.samples()) {           // runs until you break out
  const [heading, roll, pitch] = s.euler!;
  console.log(
    `heading ${heading.toFixed(1)}  roll ${roll.toFixed(1)}  pitch ${pitch.toFixed(1)}`,
    "calib", s.calibration,
  );
}
```

Without a stream, poll: `dev.readSample()` reads the full block once,
`dev.readQuaternion()` just the 8 quaternion bytes (the cheapest read).

## Hello-world — browser (Web Serial)

```ts
import { openDevice } from "@depz/sensor-sdk/web";
import { Bno055 } from "@depz/sensor-sdk";

connectBtn.addEventListener("click", async () => {
  const port = await navigator.serial.requestPort();     // user gesture
  const dev = await openDevice(port);
  if (!(dev instanceof Bno055)) throw new Error("not a BNO055");

  await dev.configure();
  await dev.startStream(20);                             // 50 Hz is plenty for a UI
  const unsub = dev.onSample((s) => {
    if (s.quaternion) renderQuat(s.quaternion);          // keep it quick
  });
});
```

## The sample

`Bno055Sample` is one decoded register block. A channel is `null` when the
block did not cover it; values are scaled by the units in force when the
stream started (`s.units`).

| field | meaning | unit (default) |
|---|---|---|
| `timestampUs` | MCU uptime at the timer tick (stream) / read completion (poll) | µs, `bigint` |
| `quaternion` | `[w, x, y, z]`, unit length | — |
| `euler` | `[heading, roll, pitch]` | degrees |
| `accel` | acceleration including gravity | m/s² |
| `linearAccel` | acceleration with gravity removed | m/s² (always) |
| `gravity` | gravity vector | m/s² (always) |
| `gyro` | angular rate | deg/s |
| `mag` | magnetic field | µT |
| `temperature` | chip temperature | °C |
| `calibration` | `{ system, gyro, accel, mag }`, each 0..3 | — |
| `raw`, `addr` | the register bytes and their start address | — |

In non-fusion modes the fusion fields (quaternion, Euler, gravity, linear
acceleration) read zero; in CONFIG mode everything does.

## Streaming

```ts
import { BNO055_FULL_BLOCK, BNO055_QUAT_BLOCK } from "@depz/sensor-sdk";

await dev.startStream(10, BNO055_FULL_BLOCK);    // full block every 10 ms (≈3.2 ms of bus)
await dev.startStream(20, BNO055_QUAT_BLOCK);    // quaternion only, 50 Hz (≈1.2 ms of bus)
await dev.stopStream();
```

The bridge reads the block on its own timer and pushes it with the MCU
timestamp of the tick, so gaps show up as jumps in `timestampUs`. A new
`startStream()` replaces the running one. Three ways to consume:

```ts
const it = dev.samples(256);                     // iterator — bounded, drop-oldest
const first = (await it.next()).value;
console.log("dropped so far:", it.droppedCount);

const unsub = dev.onSample((s) => queue.push(s));  // callback on the read-pump context

const next = await dev.getSample(1000);          // next sample; DepzTimeoutError on silence
```

Commands stay usable while streaming (`calibrationStatus()`, `readSample()`,
mode switches); page-1 access does not (see below). `streamParseErrors`
counts reports that failed to decode — 0 in a healthy session.

## Modes and power

```ts
import { Bno055OprMode, Bno055PwrMode, Bno055TempSource } from "@depz/sensor-sdk";

await dev.getOperationMode();                    // → Bno055OprMode.Ndof
await dev.setOperationMode(Bno055OprMode.Amg);   // raw accel + mag + gyro, no fusion
await dev.setOperationMode(Bno055OprMode.Ndof);  // waits for the fusion to come up

await dev.setPowerMode(Bno055PwrMode.LowPower);  // accel only until motion
await dev.setTemperatureSource(Bno055TempSource.Gyro);
```

| mode | sensors | output |
|---|---|---|
| `Config` | — | configuration only, all outputs zero |
| `AccOnly`, `MagOnly`, `GyroOnly`, `AccMag`, `AccGyro`, `MagGyro`, `Amg` | as named | raw data only |
| `Imu` | accel + gyro | relative orientation, 100 Hz |
| `Compass` | accel + mag | absolute heading, 20 Hz |
| `M4g` | accel + mag | relative orientation from the magnetometer, 50 Hz |
| `NdofFmcOff` | all three | absolute orientation, 100 Hz, slow mag calibration |
| `Ndof` | all three | absolute orientation, 100 Hz (default) |

The sensor only switches between CONFIG and an operating mode — a write from
one operating mode straight to another is silently ignored.
`setOperationMode()` handles that by going through CONFIG, so NDOF → AMG
just works (and costs the CONFIG round trip).

## Units

```ts
await dev.setUnits({ accelMg: true, gyroRps: true, eulerRad: true, tempF: true, android: false });
await dev.getUnits();                            // read back from UNIT_SEL
```

`BNO055_DEFAULT_UNITS` (all flags off) is m/s², deg/s, degrees, °C and
Windows orientation — the SDK default. The sensor's own power-on value is
different (Android orientation), so `configure()` always writes it.
`android: true` flips the pitch sign convention.

Linear acceleration and gravity stay in m/s² even with `accelMg: true` — only
the raw acceleration switches to mg. That is what the sensor does (measured),
whatever some datasheet tables say; the SDK scales accordingly.

## Axis remap

```ts
import { BNO055_AXIS_X, BNO055_AXIS_Y, BNO055_AXIS_Z } from "@depz/sensor-sdk";

await dev.setAxisRemap("P0");                    // one of the datasheet placements P0..P7
await dev.setAxisRemap({                         // output X = −chip Y
  x: BNO055_AXIS_Y, y: BNO055_AXIS_X, z: BNO055_AXIS_Z,
  xNegative: true, yNegative: false, zNegative: false,
});
await dev.getAxisRemap();
```

P1 is the default (chip axes as printed); `BNO055_PLACEMENTS` holds the eight
register pairs and `bno055Placement("P2")` turns a name into a
`Bno055AxisRemap`. A mapping that uses one axis twice throws a `RangeError` —
the sensor would silently keep the old one.

## Calibration

```ts
import { bno055FullyCalibrated } from "@depz/sensor-sdk";

const st = await dev.calibrationStatus();        // { system, gyro, accel, mag }, 0..3
bno055FullyCalibrated(st);                       // all four at 3
```

What each sensor needs (datasheet §3.11): **gyro** — hold still for a few
seconds; **accel** — six still poses, each axis up and down; **magnetometer**
— slow figure-eights in the air. The fusion calibrates continuously in the
background; you cannot disable it.

Save the result once it is fully calibrated, restore it after every power
cycle. The profile is a plain object, so it round-trips through JSON:

```ts
import { readFileSync, writeFileSync } from "node:fs";
import type { Bno055CalibrationProfile } from "@depz/sensor-sdk";

const profile = await dev.readCalibrationProfile();   // 22 bytes, read in CONFIG
writeFileSync("bno055_calib.json", JSON.stringify(profile));

const saved = JSON.parse(readFileSync("bno055_calib.json", "utf8")) as Bno055CalibrationProfile;
await dev.configure({ calibration: saved });          // or writeCalibrationProfile(saved)
```

A restored profile is a starting point, not a lock: as soon as the fusion
runs it keeps refining the offsets (with an uncalibrated magnetometer it
rewrites the magnetometer radius straight away). To check that a write landed,
read it back without leaving CONFIG. Don't store the profile of an
uncalibrated sensor: its magnetometer radius is 0, outside the legal
144..1280, and restoring it makes the sensor report a fusion configuration
error. The soft-iron matrix is there too: `getSicMatrix()` /
`setSicMatrix()` (9 × i16, 1.0 = 16384; `BNO055_SIC_IDENTITY` is the
default).

## Status and self-test

```ts
let st = await dev.systemStatus();
st.statusText;        // "fusion algorithm running"
st.errorText;         // "no error"
st.selfTestPassed;    // power-on self-test result (ST_RESULT)

st = await dev.selfTest();  // built-in self-test, ~0.45 s; mode is restored after
```

`error` / `errorText` only mean something when `status === 1` ("system
error") — at other times the register may still hold an old value.
`selfTest()` throws while streaming. A self-test run in CONFIG leaves
`SYS_STATUS` at 4 ("executing self-test") until the mode leaves CONFIG, so
when the sensor was in CONFIG it ends with a step into ACCONLY and back; the
returned `status` is the 4 read during the test. `configure()` and
`resetSensor()` clear such a leftover too (measured on SW 03.11; it used to
make them fail with "BNO055 did not finish booting").

## Page 1: sensor configs, interrupts, unique id

```ts
import { BNO055_INT, BNO055_REG1_INT_SETTINGS_FIRST } from "@depz/sensor-sdk";

const id = await dev.uniqueId();                 // 16-byte chip id (Uint8Array)

const acc = await dev.getAccelConfig();          // { range: 1, bandwidth: 3, power: 0 }
await dev.setAccelConfig({ ...acc, range: 2 });  // ±8 g — effective in non-fusion modes only

const ACC_AM_THRES = BNO055_REG1_INT_SETTINGS_FIRST;  // 0x11, any-motion threshold
await dev.setInterruptSetting(ACC_AM_THRES, 0x14);    // raw threshold byte
await dev.setInterruptEnable(BNO055_INT.accAm);       // any-motion engine on
await dev.setInterruptMask(BNO055_INT.accAm);         // ...and routed to the INT pin
await dev.readInterruptStatus();                      // INT_STA — clears on read
await dev.clearInterrupt();
```

The fusion modes override the page-1 sensor configs. On these boards only the
**motion** interrupts work (any/no-motion, high-g, high-rate); the data-ready
bits exist but never fire on sensor firmware 03.11. The page-1 interrupt
settings (0x11..0x1F, `BNO055_REG1_INT_SETTINGS_FIRST` … `_LAST`) are written
as raw bytes; the SDK has no named constant per setting register, so take the
addresses from the datasheet. An INT-triggered stream
(`startStream(1000, block, BNO055_TRIGGER_INT)`) reads the block on each
motion event, with `periodMs` as a watchdog. Every page-1 call throws while a
stream runs.

## Reset and bridge diagnostics

```ts
import { bno055IdsOk, bno055SwRevText } from "@depz/sensor-sdk";

await dev.resetSensor();              // nRESET pulse; waits until the sensor has booted
await dev.restoreConfiguration();     // re-apply the last configure()

const info = await dev.bridgeInfo();
bno055IdsOk(info);                    // chip/acc/mag/gyr ids as expected
bno055SwRevText(info.swRev);          // sensor firmware, e.g. "03.11"
info.readAvgUs;                       // I2C time of one streamed block read
info.slotsSkipped;                    // timer ticks dropped because the bus was busy
info.sensorResets;                    // the bridge pulsed nRESET to recover the bus
```

After a reset the sensor is in CONFIG with power-on settings — every output
reads zero until you configure again. If `sensorResets` grows during a long
run, the bridge recovered a stuck bus by resetting the sensor: call
`restoreConfiguration()`; the stream itself keeps running. `isAlive()` is
the quick health check (handshake passed and ids match).

## Gotchas

- **Zero quaternion = not fusing.** CONFIG mode, a non-fusion mode, or the
  first ~70 ms after a switch. `configure()` and `setOperationMode()` wait
  that out for you; raw register writes to OPR_MODE do not.
- **No direct mode-to-mode switch** — the sensor ignores it; the SDK goes via
  CONFIG. A raw `writeRegister(BNO055_REG_OPR_MODE, …)` from NDOF to AMG does
  nothing.
- **Settings silently ignored outside CONFIG.** Use the typed setters (they go
  to CONFIG and back); a raw `writeRegister()` of UNIT_SEL in NDOF does nothing.
- **Every CONFIG round trip restarts the fusion** — reading the calibration
  profile or changing units costs another ~70 ms of zeros.
- **Don't benchmark in CONFIG** — the sensor stretches the bus 3–5× harder
  there; a 46-byte read takes ~10 ms instead of ~3 ms.
- **Page 1 while streaming throws** — stop the stream first.
- **Callbacks run on the read-pump context** — keep them short and don't
  `await` device methods from inside one; hand work to a queue or use
  `samples()`.
- **`timestampUs` is a `bigint`** (device µs). Use `syncTime()` +
  `toHostTimeUs()` to place samples on the host clock.
