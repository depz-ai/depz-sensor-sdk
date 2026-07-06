# BNO086 — user guide

Hands-on guide to the BNO086 codecs in `depz/bno086.hpp`. For what the sensor is
and its concepts, read the [introduction](introduction.md); for exact signatures
see the [API reference](api.md). This SDK is the **decode layer** — SHTP
reassembly, SH-2 encoders, report decoders; you own the port and the loop (see
the [common guide](../guide.md)).

## Contents

- [SHTP reassembly](#shtp-reassembly)
- [Enabling a report](#enabling-a-report)
- [Decoding input reports](#decoding-input-reports)
- [Report types and Q-point units](#report-types-and-q-point-units)
- [Gyro-integrated RV (channel 5)](#gyro-integrated-rv-channel-5)
- [Gotchas](#gotchas)

## SHTP reassembly

Feed inbound frames to an `ShtpLayer`; it reassembles fragments into an
`ShtpCargo` (`channel`, `seq`, `payload` — headers excluded). On the TX side it
tracks per-channel sequence counters:

```cpp
#include "depz/bno086.hpp"

depz::bno086::ShtpLayer rx;
for (const auto& frame : inbound_frames) {          // each ≤ MAX_TX_FRAME bytes
    auto cargo = rx.feed(depz::as_bytes(frame));    // std::optional<ShtpCargo>
    if (!cargo) continue;
    switch (cargo->channel) {
        case (int)depz::bno086::ShtpChannel::InputNormal:  // 3
        case (int)depz::bno086::ShtpChannel::InputWake:    // 4
            /* sensor reports — parse_input_cargo below */ break;
        case (int)depz::bno086::ShtpChannel::GyroRv:       // 5
            /* parse_gyro_rv_cargo below */ break;
        case (int)depz::bno086::ShtpChannel::Control:      // 2
            /* SH-2 responses */ break;
    }
}
// rx.discarded counts fragments dropped on a seq/length mismatch.
// After a sensor reset, call rx.reset() to forget seq counters + partial cargos.
```

To send a multi-fragment cargo, `shtp_fragment_cargo` splits it into wire frames;
for a single fragment use `ShtpLayer::next_frame` (which consumes the channel's
TX seq) or `shtp_build_frame`.

## Enabling a report

Reports don't flow until you enable them. Build a Set Feature command, wrap it as
an SHTP control-channel frame, and send it:

```cpp
using namespace depz::bno086;
std::uint32_t interval_us = 10'000;                 // 100 Hz
depz::bytes cmd = sh2_build_set_feature(/*sensor_id=*/0x05 /*ROTATION_VECTOR*/,
                                        interval_us);
depz::bytes frame = tx.next_frame((int)ShtpChannel::Control, depz::as_bytes(cmd));
// … write frame to the port …  (interval_us = 0 disables the sensor)
```

The other control encoders follow the same shape: `sh2_build_get_feature_request`,
`sh2_build_product_id_request`, `sh2_build_command_request`, and the FRS trio
(`sh2_build_frs_read_request`, `sh2_build_frs_write_request`,
`sh2_build_frs_write_data`).

## Decoding input reports

An input-channel (3/4) cargo can pack several reports plus timestamp control
bytes. `parse_input_cargo` returns them as a flat `std::vector<Report>`, each
with an absolute `timestamp_us` (base + delay, folded from the `0xFB`/`0xFA`
timebase records):

```cpp
auto reports = depz::bno086::parse_input_cargo(cargo->payload, capture_timestamp_us);
for (const auto& r : reports) {
    switch (r.type) {
        case depz::bno086::ReportType::RotationVector: {
            double i = r.i_raw / 16384.0, j = r.j_raw / 16384.0;   // Q14
            double k = r.k_raw / 16384.0, w = r.real_raw / 16384.0;
            double acc_rad = r.has_accuracy_raw ? r.accuracy_raw / 4096.0 : 0.0; // Q12
            break;
        }
        case depz::bno086::ReportType::Acceleration: {
            double ax = r.x_raw / 256.0;   // Q8 m/s²  (also LinearAccel / Gravity)
            break;
        }
        default: break;   // ReportType::UnknownReport keeps its tail in r.data
    }
}
```

Every input report also carries the common header fields `seq` (rolling sample
counter for drop detection), `accuracy` (0 unreliable … 3 high), `delay_us`, and
`sensor_id`.

## Report types and Q-point units

`Report` is a flat tagged struct: only the fields relevant to `type` are
populated; scale the `*_raw` integers with the report's fixed Q point
(`value = raw / 2^Q`):

| `ReportType` | raw fields | scale |
|---|---|---|
| `Acceleration` (accel / linear / gravity) | `x_raw y_raw z_raw` | Q8 → m/s² |
| `Gyroscope` | `x_raw y_raw z_raw` | Q9 → rad/s |
| `Magnetometer` | `x_raw y_raw z_raw` | Q4 → µT |
| `RotationVector` | `i_raw j_raw k_raw real_raw` (+ `accuracy_raw`) | Q14; accuracy Q12 (rad) |
| `UncalibratedGyroscope` | `x_raw…` + `bias_x_raw…` | Q9 → rad/s |
| `GyroIntegratedRV` | `i_raw…real_raw` + `vx_raw vy_raw vz_raw` | Q14; ang.vel Q10 (rad/s) |
| `StepCounter` | `steps`, `latency_us` | — |
| scalar / detector reports | `value_raw`, `flags`, … | per report |

Game / AR-VR-game rotation vectors decode as `RotationVector` **without**
accuracy — `has_accuracy_raw` is `false`. The Q constants for the RV accuracy and
gyro-RV angular velocity are exposed as `RV_ACCURACY_Q` and `GYRO_RV_ANGVEL_Q`.

## Gyro-integrated RV (channel 5)

The gyro-integrated rotation vector streams on its own dense channel; decode a
channel-5 cargo with `parse_gyro_rv_cargo` (returns `std::nullopt` if the shape
isn't recognised):

```cpp
auto r = depz::bno086::parse_gyro_rv_cargo(cargo->payload, capture_timestamp_us);
if (r) {
    double w  = r->real_raw / 16384.0;   // Q14 quaternion
    double vx = r->vx_raw   / 1024.0;    // Q10 rad/s
}
```

It handles both wire shapes: a bare 7×`int16`, or one prefixed with `0xFB` + an
`int32` base delta + a `uint16` delay (100 µs ticks).

## Gotchas

- **Enable before you decode** — no report arrives until you send a Set Feature
  for it (`interval_us > 0`).
- **Raw integers are authoritative** — apply the Q point yourself; the decoders
  never scale for you.
- **Rotation-vector accuracy is in radians** (Q12) and only present when
  `has_accuracy_raw` is `true`; game / AR-VR-game variants omit it.
- **Reset the SHTP layer on a sensor reset** — call `ShtpLayer::reset()` so the
  seq counters and any partial cargos start clean; `discarded` counts mismatched
  fragments.
- **Frames are ≤ 64 bytes** (`MAX_TX_FRAME`, ERRATA E2) — long cargos fragment;
  reassemble with `ShtpLayer` before parsing.
- **Correlation is SH-2-level** — the bridge returns everything as unsolicited
  SHTP; match responses to requests at the SH-2 layer yourself.
