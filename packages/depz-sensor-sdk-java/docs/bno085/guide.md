<!-- generated from ../bno086/ — edit those pages -->
# BNO085 — user guide

Hands-on guide to the BNO085: the live device
(`ai.depz.sensor.devices.Bno086Device`, which runs the SH-2 host stack) and the
codecs under it (`Shtp`, `Sh2`, `Sh2Hub`, `Reports`). For what the sensor is and
its concepts, read the [introduction](introduction.md); for exact signatures see
the [API reference](api.md).

## Contents

- [Live device](#live-device)
- [SHTP reassembly](#shtp-reassembly)
- [Enabling a report](#enabling-a-report)
- [Sleep and power on](#sleep-and-power-on)
- [Decoding input reports](#decoding-input-reports)
- [Report catalog](#report-catalog)
- [Gyro-integrated RV](#gyro-integrated-rv)
- [Gotchas](#gotchas)

## Live device

```java
import ai.depz.sensor.devices.*;
import ai.depz.sensor.sensors.bno086.Reports;
import ai.depz.sensor.sensors.bno086.Sh2Hub;
import java.time.Duration;

try (Bno086Device imu = (Bno086Device) DepzDevices.openDevice()) {
    imu.hardwareReset();
    System.out.println(imu.productId().swPartNumber());  // 10004563 = BNO086, 10004148 = BNO085

    try (Subscription<Reports.Report> reports = imu.reports().subscribe(256,
            r -> r.sensorId() == Reports.ROTATION_VECTOR)) {
        Sh2Hub.FeatureResponse granted = imu.enable(Reports.ROTATION_VECTOR, 100); // the rate the hub granted
        Reports.RotationVector rv = (Reports.RotationVector) reports.take(Duration.ofSeconds(1));
        System.out.println(rv.i() + " " + rv.j() + " " + rv.k() + " " + rv.real()); // the unit quaternion
        System.out.println(rv.iRaw());                     // the wire integer (Q14) behind it
        imu.disable(Reports.ROTATION_VECTOR);
    }
}
```

- `enable(sensor, hz)` (or `enableInterval(sensor, intervalUs, batchUs,
  sensitivity, flags, …)`) sends Set Feature and reads the rate back; a rate the
  hub granted outside 0.9–2.1 × the request is logged as a warning.
  `getFeature`, `disable`.
- Reports are the `reports()` stream: `reports().on(callback, where)` and
  `reports().subscribe(capacity, where)` — `where` keeps the reports it
  accepts (a sensor-id test, as above) or is `null` for all —
  `reports().next(timeout)` waits for one. Batched sensors: `flush(sensor)`
  returns once the hub's Flush Completed arrives.
- Calibration: `setCalibration` / `getCalibration`, `saveDcd`,
  `configurePeriodicDcd`, `clearDcdAndReset`; tare: `tareNow`, `persistTare`,
  `setReorientation`; `setMotionIntent` and `enableMotionRequest` (interactive
  calibration, BNO085 only).
- FRS records: `frsRead` / `frsWrite` (`Sh2Hub.FRS_*` record ids),
  `getMetadata(sensor)`.
- Hub state: `sleep` / `powerOn` (SH-2), `wake` (the WAKE pin),
  `getOscillatorType`, `getErrors`, `getCounts` / `clearCounts`.
- Every request goes as one SHTP frame through the bridge; when both of its
  transmit slots are full the bridge answers ERR_BUSY and the device backs off
  200 ms and resends (`setBusyRetries`, `setBusyBackoff`).

The sections below are the layers under the device.

## SHTP reassembly

`Shtp.ShtpLayer` owns per-channel TX sequence counters and RX cargo reassembly.
Feed it whole SHTP frames (the bridge delivers them inside `RPT_DATA` packets);
`feed` returns a `ShtpCargo` once a cargo is complete, or `null` while
fragments are still arriving:

```java
import ai.depz.sensor.sensors.bno086.*;

Shtp.ShtpLayer shtp = new Shtp.ShtpLayer();
Shtp.ShtpCargo cargo = shtp.feed(shtpFrame);      // null until the cargo completes
if (cargo != null) {
    // cargo.channel(), cargo.seq(), cargo.payload()  (payload excludes all SHTP headers)
}
```

`shtp.discarded` counts incomplete cargos thrown away; `shtp.reset()` clears all
state on a sensor reset. For a one-shot decode of a captured frame list, the
static `Shtp.reassemble(frames, discardedOut)` is a convenience wrapper.

## Enabling a report

Build the SH-2 Set Feature cargo with `Sh2`, then frame it for the control
channel with `ShtpLayer.nextFrame` (which consumes that channel's TX seq):

```java
// enable ROTATION_VECTOR at ~100 Hz (interval 10 000 µs)
byte[] setFeature = Sh2.buildSetFeature(
        Reports.ROTATION_VECTOR, /*intervalUs*/10_000L, /*batchUs*/0L,
        /*sensitivity*/0, /*flags*/0, /*cfgWord*/0L);
byte[] frame = shtp.nextFrame(/*control channel*/2, setFeature);
// write `frame` (wrapped in your host→device transport) to the port
```

`Sh2` also builds Get Feature (`buildGetFeatureRequest`), Product ID
(`buildProductIdRequest`), generic Command (`buildCommandRequest`) and the FRS
read/write requests (`buildFrsReadRequest`, `buildFrsWriteRequest`,
`buildFrsWriteData`), and the executable-channel `buildSleep` / `buildPowerOn`
(below). The SH-2 report ids are constants on `Sh2`
(`SET_FEATURE_COMMAND`, `GET_FEATURE_REQUEST`, …). Disable a report by sending
Set Feature with `intervalUs = 0`.

## Sleep and power on

The hub has two power commands, one byte each on the **executable** channel
(SHTP channel 1, not control): "Sleep" stops every report but keeps the
configuration (enabled sensors, rates); "On" ends it, and every report that was
on comes back at its rate — no re-enabling. The hub answers neither.

```java
// a gyroscope at 50 Hz that keeps going while the hub sleeps
byte[] gyro = Sh2.buildSetFeature(
        Reports.GYROSCOPE, 20_000L, 0L, 0, Sh2.FEATURE_FLAG_WAKEUP | Sh2.FEATURE_FLAG_ALWAYS_ON, 0L);
byte[] sleep = shtp.nextFrame(Shtp.CHANNEL_EXECUTABLE, Sh2.buildSleep());    // [0x03]
byte[] on = shtp.nextFrame(Shtp.CHANNEL_EXECUTABLE, Sh2.buildPowerOn());     // [0x02]
```

Measured on a DEPZ BNO085 (SH-2 3.12.6, 2026-10-16): after sleep all reports
stopped within one sample; a report enabled with wake-up | always-on kept
streaming at full rate (it arrives on SHTP channel 4); power-on restored every
report at its rate.

Not to be confused with the board's SENSOR_WAKE_UP command (0x33): that pulses
the WAKE pin, a hardware signal. "On" is the SH-2 command that ends sleep.

## Decoding input reports

The hub streams input reports as channel-3/4 cargos. `Reports.parseInputCargo`
decodes a cargo payload into a list of typed `Report`s, folding the timebase
references and per-report delay into an absolute `timestampUs`:

```java
// captureUs: the MCU uptime (µs) when the bridge captured this RPT_DATA
for (Reports.Report rep : Reports.parseInputCargo(cargo.payload(), captureUs)) {
    if (rep instanceof Reports.RotationVector rv) {
        double i = rv.i();          // Q14 quaternion, scaled; rv.iRaw() is the wire integer
        double real = rv.real();
    } else if (rep instanceof Reports.Acceleration a) {
        double x = a.x();           // m/s² (Q8); a.xRaw() is the wire integer
    } else {
        // other report kinds
    }
}
```

`Reports.Report` is a sealed interface with one record per report kind
(`Reports.Acceleration`, `Reports.RotationVector`, …, the reference SDK's class
names): `sensorId()`, `timestampUs()`, the `*Raw` wire integers and the scaled
values derived from them (`x()`, `i()`, `value()`, `bias()`, …). Channel-3/4
reports also implement `Reports.InputReport` with `seq()`, `accuracy()` (0..3)
and `delayUs()`. `type()` is the record's simple name (the `type` tag of the
vectors and datasets) and `fields()` the raw wire fields under the vector names
(`sensor_id`, `x_raw`, …). Unrecognised or truncated report ids surface as a
`Reports.UnknownReport` carrying the remaining bytes.

## Report catalog

The report id you enable determines the decoded record and its fields:

| enable (`Reports.*`) | decoded record (`Reports.*`) | raw components → scaled |
|---|---|---|
| `ACCELEROMETER` / `LINEAR_ACCELERATION` / `GRAVITY` | `Acceleration` | `xRaw`, `yRaw`, `zRaw` (Q8) → `x()`, `y()`, `z()` m/s² |
| `GYROSCOPE` | `Gyroscope` | `xRaw`, `yRaw`, `zRaw` (Q9) → rad/s |
| `MAGNETOMETER` | `Magnetometer` | `xRaw`, `yRaw`, `zRaw` (Q4) → µT |
| `UNCAL_GYROSCOPE` / `UNCAL_MAGNETOMETER` | `UncalibratedGyroscope` / `UncalibratedMagnetometer` | `x/y/zRaw` + `biasX/Y/ZRaw` → `x()`…, `bias()` |
| `ROTATION_VECTOR` / `GEOMAG…` / `ARVR_STABILIZED_RV` | `RotationVector` | `i/j/k/realRaw` (Q14) + `accuracyRaw` (Q12) → `i()`…, `accuracyRad()` |
| `GAME_ROTATION_VECTOR` / `ARVR_STABILIZED_GAME_RV` | `RotationVector` | `i/j/k/realRaw`; `accuracyRaw` / `accuracyRad()` are `null` |
| `GYRO_INTEGRATED_RV` | `GyroIntegratedRV` | quaternion + `vx/vy/vzRaw` → `angularVelocity()` (see below) |
| `STEP_COUNTER` | `StepCounter` | `steps`, `latencyUs` |
| `TAP_DETECTOR` | `TapDetector` | `flags` → `doubleTap()` |
| `STABILITY_CLASSIFIER` | `StabilityClassifier` | `classification` |
| `PERSONAL_ACTIVITY_CLASSIFIER` | `PersonalActivityClassifier` | `mostLikelyState`, `confidences`, … |
| `MOTION_REQUEST` | `MotionRequest` | `intent`, `request` → `wantsStillness()` |

Scalars (`PRESSURE`, `AMBIENT_LIGHT`, `HUMIDITY`, `PROXIMITY`, `TEMPERATURE`)
decode as `ScalarReport` with `valueRaw` → `value()`; raw ADC reports
(`RAW_ACCELEROMETER`, `RAW_GYROSCOPE`, `RAW_MAGNETOMETER`) as `RawSensor`;
the answer to `flush()` is a `FlushCompleted`.

## Gyro-integrated RV

The gyro-integrated rotation vector uses a dense format on its own channel
(channel 5). Decode a cargo with `Reports.parseGyroRvCargo`, which handles both
the bare 7×i16 shape and the `0xFB`-prefixed timestamped shape, returning `null`
on a too-short buffer:

```java
Reports.GyroIntegratedRV rv = Reports.parseGyroRvCargo(cargo.payload(), captureUs);
if (rv != null) {
    double i = rv.i();                        // Q14 quaternion, scaled (rv.iRaw() is the wire integer)
    double[] w = rv.angularVelocity();        // rad/s (Q10; rv.vxRaw() …)
}
```

## Gotchas

- **Reassemble before decode** — always route SHTP frames through
  `ShtpLayer.feed`; the `Sh2`/`Reports` decoders expect whole cargo payloads.
- **TX frames must fit one MCU slot** — `nextFrame` throws if the cargo exceeds
  `Shtp.MAX_TX_FRAME` (64 bytes); this is ERRATA E2.
- **Raw integers are authoritative** — every record keeps the wire integers
  (`xRaw`, `iRaw`, …); the scaled accessors (`x()`, `i()`, `value()`, …) derive
  the physical values from them (`value = raw / 2^Q`, Q from `Reports.Q_POINTS`).
- **Game / AR-VR-game rotation vectors have no accuracy** — `accuracyRaw` and
  `accuracyRad()` are `null` for those two report ids.
- **Unknown / truncated reports don't throw** — they come back as a
  `Reports.UnknownReport` with the leftover bytes, so a single bad report never
  loses the rest of the cargo up to that point.
