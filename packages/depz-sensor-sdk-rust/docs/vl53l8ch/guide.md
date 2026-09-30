# VL53L8CH — user guide

Hands-on guide to the CNH addition on top of the ToF decode layer. For what the
sensor is and why CH exists, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md).

**CH shares the entire VL53L8CX decode surface.** Chunk reassembly, frame
decode, the results fields and scaling, the grid, the `Variant` selector, and
every advanced DCI codec work exactly as in the
[VL53L8CX user guide](../vl53l8cx/guide.md) — read that first. This page covers
**only what CH adds**: the CH footer offset and the CNH decode.

This crate does not drive the board: arming a CNH configuration is done with
the Python or TypeScript SDK. Keep the configuration's aggregate count and bins
per aggregate — the decode needs them.

## Contents

- [Decode CH frames with `Variant::Ch`](#decode-ch-frames-with-variantch)
- [Decode the CNH histograms](#decode-the-cnh-histograms)
- [Gotchas](#gotchas)

## Decode CH frames with `Variant::Ch`

There is no separate CH decoder. Reassemble and decode a CH frame with the same
[`FrameReassembler`](../vl53l8cx/api.md) and [`parse_frame`](../vl53l8cx/api.md)
as the CX. The one difference: the VL53L8CH firmware (VL53LMZ 2.0.16) keeps the
frame-id footer 4 bytes from the end, where the VL53L8CX firmware (ULD 2.1.0)
keeps it 12 bytes from the end — so pass `Variant::Ch`:

```rust
use depz_sensor_sdk::vl53l8::{parse_frame, Variant};

let res = parse_frame(&completed_frame, Variant::Ch)?;   // footer id at size-4
// res.distance_mm / target_status / … exactly as on the VL53L8CX
if let Some(block) = &res.cnh_raw {
    println!("CNH block: {} bytes", block.len());
}
```

Everything in the [CX results table](../vl53l8cx/guide.md#the-results-fields-and-scaling)
applies unchanged. The one CH-specific field is
[`Vl53l8Results::cnh_raw`](../vl53l8cx/api.md): the raw bytes of the CNH output
block (block id [`CNH_DATA_IDX`](api.md#cnh_data_idx) = `0xC048`), or `None`
when the frame has no CNH block.

## Decode the CNH histograms

[`decode_cnh`](api.md#decode_cnh) turns the raw CNH block — `cnh_raw` from a
decoded frame, or the bytes of the board's CNH read — into one histogram per
aggregate. It needs the aggregate count and bins per aggregate of the CNH
configuration the sensor runs:

```rust
use depz_sensor_sdk::vl53l8::{decode_cnh, CnhDecodeConfig, Vl53l8Results};

fn print_cnh(res: &Vl53l8Results) -> Result<(), Box<dyn std::error::Error>> {
    // The configuration the sensor runs: 16 aggregates of 20 bins each.
    let cfg = CnhDecodeConfig { nb_of_aggregates: 16, feature_length: 20 };
    let Some(raw) = &res.cnh_raw else { return Ok(()) };   // no CNH block
    let cnh = decode_cnh(&cfg, raw)?;                       // CnhError on a short block
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
    println!("ref residual {ref_residual}");
    Ok(())
}
```

Each [`CnhAggregate`](api.md#cnhaggregate) holds `feature_length` bins as an
integer `hist_raw` plus a power-of-two `hist_scaler`; the bin value is
`hist_raw / 2^hist_scaler`. `decode_cnh` returns `CnhError::EmptyConfig` for a
zero aggregate count or bin count, and `CnhError::Truncated` when the block is
shorter than the layout the config implies.

## Gotchas

- **Use `Variant::Ch` for VL53L8CH frames** — its footer id sits at `size-4`.
  `Variant::Cx` (`size-12`) fails the header/footer check with
  `Vl53l8Error::CorruptedFrame` on them.
- **Pass the configuration the sensor runs** — `decode_cnh` needs the same
  `nb_of_aggregates` / `feature_length`; the block does not record them, and a
  wrong pair decodes to wrong bins or `Truncated`.
- **`cnh_raw` is `None`** on CX frames and on CH frames without a CNH block.
- **Everything else is inherited** — the CX gotchas (reassemble before decode,
  raw fixed-point scaling, resolution from the frame, no live init/config) apply
  here too; see the [CX guide](../vl53l8cx/guide.md#gotchas).
