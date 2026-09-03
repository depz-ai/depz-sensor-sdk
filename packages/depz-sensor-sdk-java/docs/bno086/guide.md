# BNO086 — user guide

Hands-on guide to the BNO086 codecs (`Shtp`, `Sh2`, `Reports`). For what the
sensor is and its concepts, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md). This is a decode layer — the
snippets build SH-2 cargos, frame them for SHTP, and decode the input reports the
hub streams back; you supply the transport.

## Contents

- [SHTP reassembly](#shtp-reassembly)
- [Enabling a report](#enabling-a-report)
- [Decoding input reports](#decoding-input-reports)
- [Report catalog](#report-catalog)
- [Gyro-integrated RV](#gyro-integrated-rv)
- [Gotchas](#gotchas)

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
`buildFrsWriteData`). The SH-2 report ids are constants on `Sh2`
(`SET_FEATURE_COMMAND`, `GET_FEATURE_REQUEST`, …). Disable a report by sending
Set Feature with `intervalUs = 0`.

## Decoding input reports

The hub streams input reports as channel-3/4 cargos. `Reports.parseInputCargo`
decodes a cargo payload into a list of typed `Report`s, folding the timebase
references and per-report delay into an absolute `timestamp_us`:

```java
long captureUs = /* MCU uptime when the bridge captured this RPT_DATA */;
for (Reports.Report rep : Reports.parseInputCargo(cargo.payload(), captureUs)) {
    switch (rep.type()) {
        case "RotationVector" -> {
            long i = (long) rep.fields().get("i_raw");    // Q14 quaternion, raw
            long real = (long) rep.fields().get("real_raw");
            // scale downstream: value = raw / 2^14
        }
        case "Acceleration" -> {
            long x = (long) rep.fields().get("x_raw");     // Q8 m/s², raw
        }
        default -> { /* other report types */ }
    }
}
```

Each `Report` is a `type` tag plus a `fields` map. Common keys: `sensor_id`,
`timestamp_us`, `seq`, `accuracy` (0..3), `delay_us`, and the `*_raw` payload
integers. Unrecognised or truncated report ids surface as an `"UnknownReport"`
carrying the remaining bytes as hex.

## Report catalog

The report id you enable determines the decoded `type` and its raw fields:

| enable (`Reports.*`) | decoded `type` | raw fields |
|---|---|---|
| `ACCELEROMETER` / `LINEAR_ACCELERATION` / `GRAVITY` | `Acceleration` | `x_raw`, `y_raw`, `z_raw` (Q8) |
| `GYROSCOPE` | `Gyroscope` | `x_raw`, `y_raw`, `z_raw` (Q9) |
| `MAGNETOMETER` | `Magnetometer` | `x_raw`, `y_raw`, `z_raw` (Q4) |
| `UNCAL_GYROSCOPE` / `UNCAL_MAGNETOMETER` | `UncalibratedGyroscope` / `UncalibratedMagnetometer` | `x/y/z_raw` + `bias_x/y/z_raw` |
| `ROTATION_VECTOR` / `GEOMAG…` / `ARVR_STABILIZED_RV` | `RotationVector` | `i/j/k/real_raw` (Q14) + `accuracy_raw` (Q12 rad) |
| `GAME_ROTATION_VECTOR` / `ARVR_STABILIZED_GAME_RV` | `RotationVector` | `i/j/k/real_raw`; `accuracy_raw` is `null` |
| `GYRO_INTEGRATED_RV` | `GyroIntegratedRV` | quaternion + `vx/vy/vz_raw` (see below) |
| `STEP_COUNTER` | `StepCounter` | `steps`, `latency_us` |
| `TAP_DETECTOR` | `TapDetector` | `flags` |
| `STABILITY_CLASSIFIER` | `StabilityClassifier` | `classification` |
| `PERSONAL_ACTIVITY_CLASSIFIER` | `PersonalActivityClassifier` | `most_likely_state`, `confidences`, … |

Scalars (`PRESSURE`, `AMBIENT_LIGHT`, `HUMIDITY`, `PROXIMITY`, `TEMPERATURE`)
decode as `ScalarReport` with `value_raw`; raw ADC reports (`RAW_ACCELEROMETER`,
`RAW_GYROSCOPE`, `RAW_MAGNETOMETER`) as `RawSensor`.

## Gyro-integrated RV

The gyro-integrated rotation vector uses a dense format on its own channel
(channel 5). Decode a cargo with `Reports.parseGyroRvCargo`, which handles both
the bare 7×i16 shape and the `0xFB`-prefixed timestamped shape, returning `null`
on a too-short buffer:

```java
Reports.Report rv = Reports.parseGyroRvCargo(cargo.payload(), captureUs);
if (rv != null) {
    long i = (long) rv.fields().get("i_raw");     // Q14 quaternion
    long vx = (long) rv.fields().get("vx_raw");   // Q10 angular velocity, raw
}
```

## Gotchas

- **Reassemble before decode** — always route SHTP frames through
  `ShtpLayer.feed`; the `Sh2`/`Reports` decoders expect whole cargo payloads.
- **TX frames must fit one MCU slot** — `nextFrame` throws if the cargo exceeds
  `Shtp.MAX_TX_FRAME` (64 bytes); this is ERRATA E2.
- **Raw integers are authoritative** — apply Q-point scaling downstream
  (`value = raw / 2^Q`); the decoders never scale for you.
- **Game / AR-VR-game rotation vectors have no accuracy** — `accuracy_raw` is
  `null` for those two report ids.
- **Unknown / truncated reports don't throw** — they come back as
  `"UnknownReport"` with the leftover bytes as hex, so a single bad report never
  loses the rest of the cargo up to that point.
