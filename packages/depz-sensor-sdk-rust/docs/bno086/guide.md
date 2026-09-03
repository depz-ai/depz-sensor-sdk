# BNO086 — user guide

Hands-on guide to the SH-2 decode layer in [`bno086`](api.md). For what the
sensor is and its concepts, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md). Framing, CRC and discovery are
shared across sensors — see the [common guide](../guide.md).

## Contents

- [The two layers: SHTP then SH-2](#the-two-layers-shtp-then-sh-2)
- [Hello-world: decode orientation](#hello-world-decode-orientation)
- [Enabling a report](#enabling-a-report)
- [Report types and units](#report-types-and-units)
- [The gyro-integrated RV channel](#the-gyro-integrated-rv-channel)
- [Control requests and FRS](#control-requests-and-frs)
- [Gotchas](#gotchas)

## The two layers: SHTP then SH-2

Inbound bytes are SHTP frames carrying SH-2 reports. Reassemble cargos with
[`ShtpLayer`](api.md), then parse the input channels (3/4) with
[`parse_input_cargo`](api.md):

```rust
use depz_sensor_sdk::bno086::{ShtpLayer, parse_input_cargo, ShtpChannel};

let mut shtp = ShtpLayer::new();
if let Some(cargo) = shtp.feed(&shtp_frame) {          // one reassembled cargo
    if cargo.channel == ShtpChannel::InputNormal as u8 {
        for report in parse_input_cargo(&cargo.payload, capture_timestamp_us) {
            // typed Report; capture_timestamp_us is the bridge RPT_DATA capture time (MCU µs)
        }
    }
}
// shtp.discarded counts incomplete cargos thrown away
```

`ShtpLayer` also owns the per-channel TX sequence counters: build an outbound
frame with `shtp.next_frame(channel, &payload)`.

## Hello-world: decode orientation

Enable the rotation vector, then decode its reports off the cargo stream:

```rust
use depz_sensor_sdk::bno086::{parse_input_cargo, reports::Report, sh2::build_set_feature};

// Host → device: enable ROTATION_VECTOR (0x05) at 100 Hz (10 000 µs interval).
let set = build_set_feature(0x05, 10_000, 0, 0, 0, 0);
let frame = shtp.next_frame(2 /* control */, &set);    // ... write to your transport ...

// Device → host: decode quaternions (raw Q14).
for r in parse_input_cargo(&cargo.payload, capture_timestamp_us) {
    if let Report::RotationVector { i_raw, j_raw, k_raw, real_raw, .. } = r {
        let q14 = |v: i32| v as f64 / (1 << 14) as f64;
        println!("i={:+.3} j={:+.3} k={:+.3} w={:+.3}",
                 q14(i_raw), q14(j_raw), q14(k_raw), q14(real_raw));
    }
}
```

## Enabling a report

[`build_set_feature`](api.md) is the one enable path (a 17-byte Set Feature
command, `0xFD`); `interval_us = 0` disables the sensor. Give the rate as an
interval in microseconds:

```rust
use depz_sensor_sdk::bno086::sh2::{build_set_feature, build_get_feature_request};

let enable_gyro  = build_set_feature(0x02, 5_000, 0, 0, 0, 0);   // 200 Hz
let disable_gyro = build_set_feature(0x02, 0, 0, 0, 0, 0);       // interval 0 = off
let query_gyro   = build_get_feature_request(0x02);             // read back the granted config
```

The hub rounds the requested interval to its 1 kHz/2ⁿ grid; read the granted
rate back with `build_get_feature_request` and the Get Feature Response.

## Report types and units

Every report keeps its wire integers; scale with [`q_point`](api.md)
(`value = raw / 2**Q`). The common shapes of [`Report`](api.md):

| enable id | variant | fields (units) |
|---|---|---|
| `0x01` / `0x04` / `0x06` | `Vector3` | `x/y/z_raw` accel-family, Q8 (m/s²) |
| `0x02` | `Vector3` (`kind = Gyroscope`) | `x/y/z_raw` Q9 (rad/s) |
| `0x03` | `Vector3` (`kind = Magnetometer`) | `x/y/z_raw` Q4 (µT) |
| `0x07` / `0x0F` | `Vector3WithBias` | primary + bias (uncal gyro Q9 / mag Q4) |
| `0x05` / `0x09` / `0x28` | `RotationVector` | `i/j/k/real_raw` Q14 + `accuracy_raw` Q12 (radians) |
| `0x08` / `0x29` | `RotationVector` | `i/j/k/real_raw` Q14; `accuracy_raw` is `None` |
| `0x0A`–`0x0E` | `Scalar` | single environment value |
| `0x10` | `TapDetector` | `flags` (bit 6 = double tap) |
| `0x11` / `0x18` | `StepCounter` / `StepDetector` | `steps`, `latency_us` |
| `0x2A` | `GyroIntegratedRv` | quaternion Q14 + angular velocity Q10 |

Every input-channel report also carries `seq` (rolling sample counter for drop
detection), `accuracy` (0 unreliable … 3 high), `delay_us`, and an absolute
`timestamp_us` in the MCU clock. Unrecognised report IDs surface as
[`Report::Unknown`](api.md) with the raw bytes.

## The gyro-integrated RV channel

The gyro-integrated rotation vector streams on its own dense channel (SHTP
channel 5, [`ShtpChannel::GyroRv`](api.md)); decode that cargo with
[`parse_gyro_rv_cargo`](api.md):

```rust
use depz_sensor_sdk::bno086::{parse_gyro_rv_cargo, reports::Report};

if cargo.channel == 5 {
    if let Some(Report::GyroIntegratedRv { vx_raw, vy_raw, vz_raw, .. }) =
        parse_gyro_rv_cargo(&cargo.payload, capture_timestamp_us)
    {
        // quaternion is Q14; angular velocity vx/vy/vz is Q10 (rad/s)
    }
}
```

## Control requests and FRS

The [`sh2`](api.md) module builds every control-channel request (report IDs are
the `REPORT_*` constants). All are pure encoders — you frame them with
`ShtpLayer::next_frame` on the control channel:

```rust
use depz_sensor_sdk::bno086::sh2::{
    build_product_id_request, build_command_request,
    build_frs_read_request, build_frs_write_request, build_frs_write_data,
};

let pid = build_product_id_request();                       // 0xF9
let cmd = build_command_request(seq, 0x07 /* save DCD */, &[])?;   // 0xF2, ≤ 9 params
let frs_read = build_frs_read_request(frs_type, 0, 0);      // block_words = 0 → whole record
let frs_erase = build_frs_write_request(frs_type, 0);       // length_words = 0 → erase
let frs_data = build_frs_write_data(0, &[word0, word1])?;   // 1 or 2 words per packet
```

`build_command_request` returns [`Sh2Error::TooManyParams`](api.md) beyond 9
parameter bytes; `build_frs_write_data` returns
[`Sh2Error::BadWriteWordCount`](api.md) unless given 1 or 2 words.

## Gotchas

- **Reassemble SHTP before parsing** — `parse_input_cargo` wants a full cargo,
  not a raw frame; feed frames through `ShtpLayer::feed` first. A short or
  overrun cargo increments `discarded`.
- **Raw integers, not floats** — scale with the Q points (`q_point`); nothing is
  pre-converted. Rotation-vector **accuracy is in radians** (Q12), distinct from
  the integer `accuracy` reliability level (0–3).
- **Game / AR-VR-game rotation vectors have no accuracy** — `accuracy_raw` is
  `None` for `0x08` / `0x29`.
- **The gyro-integrated RV is a different channel** — decode channel 5 with
  `parse_gyro_rv_cargo`, not `parse_input_cargo`.
- **Correlation is SH-2-level** — the bridge returns everything as unsolicited
  (ERRATA E2); match responses by report ID / sequence at the SH-2 layer.
- **Host→device frames must fit one MCU slot** — keep TX frames within
  [`MAX_TX_FRAME`](api.md) = 64 bytes (ERRATA E2). `ShtpLayer::reset` clears TX
  seq counters and partial cargos on a sensor reset.
- **This crate does not own a port** — you supply the bytes and write the framed
  requests; see the [common guide](../guide.md#getting-started).
