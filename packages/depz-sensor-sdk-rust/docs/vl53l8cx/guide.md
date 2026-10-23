# VL53L8CX — user guide

Hands-on guide to the live device (`devices::Vl53l8cxDevice`, which runs the
ST ULD on the host) and the ToF decode layer in [`vl53l8`](api.md) under it. For what the
sensor is, CX vs CH, and the concepts, read the [introduction](introduction.md);
for exact signatures see the [API reference](api.md). The **VL53L8CH** superset
(CNH histograms) has its [own guide](../vl53l8ch/guide.md) — it shares
everything below.

## Contents

- [Live device](#live-device)
- [Reassemble a frame from chunks](#reassemble-a-frame-from-chunks)
- [Hello-world: decode an 8×8 depth frame](#hello-world-decode-an-8x8-depth-frame)
- [The results: fields and scaling](#the-results-fields-and-scaling)
- [Reading the grid](#reading-the-grid)
- [CX vs CH: the `Variant` selector](#cx-vs-ch-the-variant-selector)
- [Advanced DCI codecs](#advanced-dci-codecs)
- [Gotchas](#gotchas)

## Live device

```rust
use std::time::Duration;
use depz_sensor_sdk::devices::open_on_link;

let tof = open_on_link(link, None, None, None)?.into_vl53l8cx()?;
tof.init()?;                        // downloads the ~84 KB sensor firmware (about a second)
tof.set_resolution(64)?;            // 8×8
tof.set_ranging_frequency_hz(15)?;  // 2 Hz at least; 15 Hz at most at 8×8
let frames = tof.frames(8);
tof.start_ranging()?;
let frame = frames.take(Duration::from_secs(1))?;
println!("centre zone: {} mm", frame.distance_mm[27]);
tof.stop_ranging()?;
# Ok::<(), depz_sensor_sdk::devices::DepzError>(())
```

- Configure after `init` and while not ranging: resolution, frequency, ranging
  mode and integration time, sharpener, target order, power mode, xtalk margin
  and calibration (`calibrate_xtalk`, `get_caldata_xtalk` / `set_caldata_xtalk`),
  detection thresholds, `configure_motion_indicator`. While ranging they fail
  with `DepzError::InvalidState` — the stream owns the register bank.
- `init_with_progress` reports the init phases and the firmware write progress.
- Frames go to `on_frame(callback)` and `frames(capacity)`; `get_frame` takes
  the next one. `frame_parse_errors` and `reassembler_discards` count the lost
  ones.
- `with_uld(|u| …)` runs a closure on the driver (`Vl53l8Driver`) for DCI access.

The sections below are the decode layer under the device.

## Reassemble a frame from chunks

The MCU streams each frame as `RPT_VL53_FRAME` chunks. Decode each report
payload with [`unpack_frame_chunk`](api.md) and feed it to a
[`FrameReassembler`](api.md); it returns a [`CompletedFrame`](api.md) when a full
frame lands:

```rust
use depz_sensor_sdk::vl53l8::{FrameReassembler, unpack_frame_chunk};

let mut asm = FrameReassembler::new();
for pkt in vl53_frame_reports {                 // each pkt.payload is an RPT_VL53_FRAME
    if let Some(chunk) = unpack_frame_chunk(&pkt.payload) {
        if let Some(done) = asm.feed(chunk) {
            // `done.frame` is the raw results frame; `done.timestamp_us` the capture time
        }
    }
}
// asm.completed / asm.discarded are live counters (gaps / offset errors)
```

## Hello-world: decode an 8x8 depth frame

Decode a completed raw frame with [`parse_frame`](api.md) into
[`Vl53l8Results`](api.md):

```rust
use depz_sensor_sdk::vl53l8::{parse_frame, Variant, RESOLUTION_8X8};

let res = parse_frame(&done.frame, Variant::Cx)?;    // Vl53l8Error on a corrupt / short frame
let n = res.resolution();                            // 16 or 64, from the frame itself
if n == RESOLUTION_8X8 {
    for z in 0..n {
        if res.target_status[z] == 5 || res.target_status[z] == 9 {
            println!("zone {z}: {} mm", res.distance_mm[z]);   // mm, never negative
        }
    }
    println!("silicon: {} °C", res.silicon_temp_degc);
}
```

## The results: fields and scaling

[`Vl53l8Results`](api.md) keeps every per-zone output as a `Vec` sized to the
active resolution, in the real units ST `GetRangingData` gives (contract 04
§5; ST divides integers, these keep the fraction):

| field | type | meaning / scaling |
|---|---|---|
| `distance_mm` | `Vec<i32>` | distance, mm — `floor(raw/4)`, a negative one clamped to 0 |
| `target_status` | `Vec<u8>` | 5/9 = valid, 255 = no target |
| `nb_target_detected` | `Vec<u8>` | targets found in the zone |
| `signal_per_spad` | `Vec<f64>` | signal rate, kcps/SPAD (`raw/2048`) |
| `ambient_per_spad` | `Vec<f64>` | ambient rate, kcps/SPAD (`raw/2048`) |
| `nb_spads_enabled` | `Vec<u32>` | SPADs enabled in the zone |
| `range_sigma_mm` | `Vec<f64>` | range std-dev, mm (`raw/128`) |
| `reflectance` | `Vec<f64>` | estimated reflectance, % (`raw/2`) |
| `silicon_temp_degc` | `i8` | per-frame sensor temperature, °C |
| `cnh_raw` | `Option<Vec<u8>>` | raw CH histogram block (CH only; see below) |

`resolution()` returns the zone count (`nb_target_detected.len()`), 16 or 64.

## Reading the grid

Zone index is row-major, so reshape to a `(4,4)` / `(8,8)` grid yourself:

```rust
let n = res.resolution();
let side = if n == 64 { 8 } else { 4 };
for row in 0..side {
    for col in 0..side {
        let z = row * side + col;
        let mm = if res.target_status[z] == 255 { None } else { Some(res.distance_mm[z]) };
        // ... render `mm` ...
    }
}
```

Mask invalid zones on `target_status == 255` before trusting a distance.

## CX vs CH: the `Variant` selector

Both silicon variants share this decode; [`Variant`](api.md) selects only the
frame-tail footer-id geometry used for the corruption check:

```rust
parse_frame(&frame, Variant::Cx)?;   // VL53L8CX, ULD 2.1.0 footer (size-12)
parse_frame(&frame, Variant::Ch)?;   // VL53L8CH, VL53LMZ 2.0.16 footer (size-4)
```

The geometry follows the firmware each part runs: the VL53L8CX firmware (ULD
2.1.0) keeps the footer id 12 bytes from the end, the VL53L8CH firmware
(VL53LMZ 2.0.16) 4 bytes. The wrong variant fails the header/footer check with
`Vl53l8Error::CorruptedFrame`. When a CH frame carries a CNH block, its raw
bytes surface as `Vl53l8Results::cnh_raw` and
[`decode_cnh`](../vl53l8ch/api.md#decode_cnh) unpacks them — see the
[VL53L8CH guide](../vl53l8ch/guide.md).

## Advanced DCI codecs

[`vl53l8::advanced`](api.md#vl53l8cx-tof) holds the pure DCI byte layouts, ported
verbatim from the ST ULD and shared by CX and CH. These are encoders — the live
register bridge that writes them over the wire is a documented extension point.

```rust
use depz_sensor_sdk::vl53l8::advanced::{
    try_default_motion_config, try_xtalk_margin_to_raw,
    try_pack_detection_thresholds, AdvancedError, DetectionThreshold, DIST_MM,
};

// (inside a fn returning Result<_, AdvancedError>)
// Motion-indicator config (156-byte DCI payload) for a resolution (16 or 64):
let cfg = try_default_motion_config(64)?;
let bytes = cfg.pack();

// Crosstalk margin (kcps/SPAD, 0..=10000) → raw DCI value:
let raw = try_xtalk_margin_to_raw(50.0)?;  // round(kcps * 2048)

// Up to 64 detection thresholds → their two DCI blocks (low/high scaled per
// measurement; a scaled value that overflows i32 is an error, never wrapped):
let blocks = try_pack_detection_thresholds(&[DetectionThreshold {
    low_thresh: 200, high_thresh: 600, measurement: DIST_MM,
    type_: 0, zone_num: 128, operation: 0,
}])?;
// blocks.start (64×12 bytes) + blocks.valid (8 bytes of 0x05)
```

The `try_` forms carry the driver's parameter checks (contract 07 §3: an
argument out of range is refused before anything is written). The older
`default_motion_config` / `xtalk_margin_to_raw` / `pack_detection_thresholds`
cannot report an error and are deprecated.

Measurement selectors ([`DIST_MM`](api.md), [`SIGNAL_PER_SPAD_KCPS`](api.md), …)
and power-mode constants ([`POWER_MODE_SLEEP`](api.md) …) live in the same
module.

## Gotchas

- **Reassemble before you decode** — `parse_frame` wants a complete frame; feed
  chunks through `FrameReassembler` first. A gap or offset mismatch increments
  `discarded` and drops the partial frame.
- **`parse_frame` can fail** — it returns [`Vl53l8Error`](api.md):
  `ShortFrame` (< 16 bytes) or `CorruptedFrame` (header/footer id mismatch).
  Pick the `Variant` that matches the footer geometry.
- **Values are in real units already** — no scaling left to do; a negative
  distance (status 4, unreliable) comes out as 0, as ST gives it.
- **Resolution comes from the frame**, not your config — size loops by
  `res.resolution()`.
- **CNH is CH-only** — `cnh_raw` is `None` on CX frames; decoding it is a
  not-yet-implemented extension point (see the
  [VL53L8CH guide](../vl53l8ch/guide.md)).
- **Configure before ranging** — every setter refuses while the stream runs;
  `stop_ranging()`, change, `start_ranging()` again.
