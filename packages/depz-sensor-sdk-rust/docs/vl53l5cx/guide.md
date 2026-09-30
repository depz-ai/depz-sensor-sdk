# VL53L5CX — user guide

The VL53L5CX is decoded exactly like the [VL53L7CX](../vl53l7cx/guide.md):
same board commands, same 1536-byte transfer ceilings, same frame decode with
`Variant::L7`. Everything in the VL53L7CX guide applies unchanged; this page
shows the two L5 specifics. For signatures see the [API reference](api.md).

This crate does not drive the board: initialising and streaming is done with
the Python or TypeScript SDK.

## Resolve the class

```rust
use depz_sensor_sdk::vl53l7::{resolve_model, Vl53l7Model};

// The production PID model wins; the device name is the fallback.
assert_eq!(resolve_model(Some("vl53l5cx"), ""), Vl53l7Model::Vl53l5cx);
assert_eq!(resolve_model(None, "DEPZ ToF Sensor VL53L5CX USB v2.1"), Vl53l7Model::Vl53l5cx);
assert!(!Vl53l7Model::Vl53l5cx.has_cnh());
```

A board stamped as an L7 but carrying an L5 (or the reverse) still ranges —
the sensor firmware is shared — and the module type the sensor reports after
initialisation (0 = MZ for the L5) is the only silicon-side tell.

## Decode a frame

```rust
use depz_sensor_sdk::vl53l8::{parse_frame, Variant, RESOLUTION_4X4};

let res = parse_frame(&frame, Variant::L7)?;
if res.resolution() == RESOLUTION_4X4 {
    // 4×4 frames carry 64-entry per-target blocks on L5/L7; L7 trims them to 16.
    assert_eq!(res.distance_mm.len(), 16);
}
```

## Gotchas

- **The class is chosen from the board's USB id / device name**, never from
  the silicon.
- **No deep sleep, no threshold auto-stop** on this firmware (ULD 2.0.1).
- **Peel the protective film off the lens** before measuring — with the film
  on, weak-signal zones read 0.
- All the [VL53L7CX gotchas](../vl53l7cx/guide.md#gotchas) apply.
