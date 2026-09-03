# VL53L8CX — user guide

Hands-on guide to the ToF decode layer in [`vl53l8`](api.md). For what the
sensor is, CX vs CH, and the concepts, read the [introduction](introduction.md);
for exact signatures see the [API reference](api.md). The **VL53L8CH** superset
(CNH histograms) has its [own guide](../vl53l8ch/guide.md) — it shares
everything below.

## Contents

- [Reassemble a frame from chunks](#reassemble-a-frame-from-chunks)
- [Hello-world: decode an 8×8 depth frame](#hello-world-decode-an-8x8-depth-frame)
- [The results: fields and scaling](#the-results-fields-and-scaling)
- [Reading the grid](#reading-the-grid)
- [CX vs CH: the `Variant` selector](#cx-vs-ch-the-variant-selector)
- [Advanced DCI codecs](#advanced-dci-codecs)
- [Gotchas](#gotchas)

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
            println!("zone {z}: {} mm", res.distance_mm[z]);   // already floor(raw/4) mm
        }
    }
    println!("silicon: {} °C", res.silicon_temp_degc);
}
```

## The results: fields and scaling

[`Vl53l8Results`](api.md) keeps every per-zone output as a `Vec` sized to the
active resolution, in **raw wire fixed-point** (convert explicitly):

| field | type | meaning / scaling |
|---|---|---|
| `distance_mm` | `Vec<i32>` | per-zone distance, mm — already `floor(raw/4)` |
| `target_status` | `Vec<u8>` | 5/9 = valid, 255 = no target |
| `nb_target_detected` | `Vec<u8>` | targets found in the zone |
| `signal_per_spad` | `Vec<u32>` | signal rate, raw — ÷2048 for kcps/SPAD |
| `ambient_per_spad` | `Vec<u32>` | ambient rate, raw — ÷2048 for kcps/SPAD |
| `nb_spads_enabled` | `Vec<u32>` | SPADs enabled in the zone |
| `range_sigma_mm_raw` | `Vec<u16>` | range std-dev, raw — ÷128 for mm |
| `reflectance` | `Vec<u8>` | estimated reflectance, % |
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
parse_frame(&frame, Variant::Cx)?;   // ULD 2.1.0 footer (size-12) — DEPZ firmware default
parse_frame(&frame, Variant::Ch)?;   // ULD 2.0.16 footer (size-4)
```

Note the geometry tracks the **ULD version the firmware embeds**, not the
silicon: the DEPZ firmware streams 2.1.0-footer frames on both CX and CH
devices, so a CH capture still decodes with `Variant::Cx` geometry. When a CH
frame carries a CNH block, its raw bytes surface as
[`Vl53l8Results::cnh_raw`](../vl53l8ch/api.md) — see the
[VL53L8CH guide](../vl53l8ch/guide.md).

## Advanced DCI codecs

[`vl53l8::advanced`](api.md#vl53l8cx-tof) holds the pure DCI byte layouts, ported
verbatim from the ST ULD and shared by CX and CH. These are encoders — the live
register bridge that writes them over the wire is a documented extension point.

```rust
use depz_sensor_sdk::vl53l8::advanced::{
    default_motion_config, xtalk_margin_to_raw,
    pack_detection_thresholds, DetectionThreshold, DIST_MM,
};

// Motion-indicator config (156-byte DCI payload) for a resolution:
let cfg = default_motion_config(64);
let bytes = cfg.pack();

// Crosstalk margin (kcps/SPAD) → raw DCI value:
let raw = xtalk_margin_to_raw(50.0);       // round(kcps * 2048)

// Up to 64 detection thresholds → their two DCI blocks (low/high scaled per measurement):
let blocks = pack_detection_thresholds(&[DetectionThreshold {
    low_thresh: 200, high_thresh: 600, measurement: DIST_MM,
    type_: 0, zone_num: 128, operation: 0,
}]);
// blocks.start (64×12 bytes) + blocks.valid (8 bytes of 0x05)
```

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
- **Values are raw fixed-point** — divide (`/2048`, `/128`) for real units;
  distance is the exception (already `floor(raw/4)` mm).
- **Resolution comes from the frame**, not your config — size loops by
  `res.resolution()`.
- **CNH is CH-only** — `cnh_raw` is `None` on CX frames; decoding it is a
  not-yet-implemented extension point (see the
  [VL53L8CH guide](../vl53l8ch/guide.md)).
- **No live init/config here** — the firmware download and DCI register bridge
  are out of scope; this crate decodes frames and packs DCI payloads.
