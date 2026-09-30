# VL53L7CH — user guide

The VL53L7CH is the [VL53L7CX](../vl53l7cx/guide.md) plus CNH histograms:
class resolution, board commands, the 1536-byte ceilings and the frame decode
with `Variant::L7` all apply unchanged. This page covers the CNH block. For
signatures see the [API reference](api.md).

This crate does not drive the board: arming a CNH configuration and starting
the stream is done with the Python or TypeScript SDK. Keep the configuration's
aggregate count and bins per aggregate — the decode needs them.

## Contents

- [CNH frames arrive in chunks](#cnh-frames-arrive-in-chunks)
- [Decode the histograms](#decode-the-histograms)
- [Gotchas](#gotchas)

## CNH frames arrive in chunks

A frame with CNH is larger than one 1536-byte chunk — 3156 bytes for 16
aggregates × 20 bins, up to ~7.6 KB — so it arrives as several
`RPT_VL53_FRAME` chunks. The shared reassembler rebuilds it; nothing
CH-specific is needed:

```rust
use depz_sensor_sdk::framing::{Event, PacketParser};
use depz_sensor_sdk::vl53l8::{parse_frame, unpack_frame_chunk, FrameReassembler, Variant, Vl53l8Rpt};

let mut parser = PacketParser::new();
let mut asm = FrameReassembler::new();
let mut frames = Vec::new();
for ev in parser.feed(&rx_bytes) {
    let Event::Packet(pkt) = ev else { continue };
    if pkt.cmd != Vl53l8Rpt::Vl53Frame as u8 {
        continue;
    }
    if let Some(done) = unpack_frame_chunk(&pkt.payload).and_then(|c| asm.feed(c)) {
        frames.push(parse_frame(&done.frame, Variant::L7)?);
    }
}
```

## Decode the histograms

`parse_frame` captures the CNH output block (block id
[`CNH_DATA_IDX`](api.md#cnh_data_idx) = `0xC048`) verbatim in `cnh_raw`;
[`decode_cnh`](api.md#decode_cnh) unpacks it with the configuration the sensor
was armed with:

```rust
use depz_sensor_sdk::vl53l8::{decode_cnh, CnhDecodeConfig};

// The armed CNH configuration: 16 aggregates of 20 bins each.
let cfg = CnhDecodeConfig { nb_of_aggregates: 16, feature_length: 20 };
for res in &frames {
    let Some(raw) = &res.cnh_raw else { continue };      // None: no CNH block armed
    let cnh = decode_cnh(&cfg, raw)?;                    // CnhError on a short block
    for (i, agg) in cnh.aggregates.iter().enumerate() {
        let hist: Vec<f64> = agg
            .hist_raw
            .iter()
            .zip(&agg.hist_scaler)
            .map(|(&v, &s)| v as f64 / 2f64.powi(s as i32))   // raw / 2^scaler
            .collect();
        println!("aggregate {i}: {hist:?}");
    }
    let ref_residual = cnh.ref_residual_word as f64 / 2048.0;   // 11 fractional bits
    println!("ref residual {ref_residual}, zone 27: {:?} mm", res.distance_mm.get(27));
}
```

The per-zone depth arrays in the same `Vl53l8Results` are fully decoded and
trimmed as on the VL53L7CX.

## Gotchas

- **Pass the armed configuration** — `decode_cnh` needs the same
  `nb_of_aggregates` / `feature_length` the CNH configuration used; the block
  does not say.
- **`cnh_raw` is `None`** on frames without a CNH block (CNH not armed).
- **Use `Variant::L7`** — the L7CH carries the VL53LMZ blob, and its footer
  sits at `size-4` like the other L5/L7 parts.
- All the [VL53L7CX gotchas](../vl53l7cx/guide.md#gotchas) apply.
