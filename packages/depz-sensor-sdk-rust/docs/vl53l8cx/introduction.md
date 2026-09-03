# VL53L8CX — introduction

The **VL53L8CX** is a STMicroelectronics multizone Time-of-Flight ranging
sensor: a tiny laser depth **imager** that returns a 4×4 or 8×8 grid of
distances up to ~4 m. On the DEPZ sensor line the MCU is a thin SPI **register
bridge** — the ST ULD (Ultra-Lite Driver) frame layout is decoded **on the
host**, and this crate provides that verifiable decode layer
([`vl53l8`](api.md)): chunk reassembly, the raw results-frame decoder, and the
advanced DCI byte codecs.

This is the base ToF sensor. Its superset — the **VL53L8CH**, which adds Compact
Network Histograms — has its own [introduction](../vl53l8ch/introduction.md) and
[guide](../vl53l8ch/guide.md); everything here applies to it unchanged.

## What it does

- Per-zone distance over a **16-zone (4×4)** or **64-zone (8×8)** grid, at up
  to 60 Hz (4×4) / 15 Hz (8×8).
- Each frame carries far more than distance: per-zone target status, number of
  targets, signal and ambient rate (kcps/SPAD), range sigma, reflectance, plus
  a per-frame silicon temperature — all decoded by
  [`parse_frame`](api.md) into [`Vl53l8Results`](api.md).
- The MCU ships each sensor frame as one or more `RPT_VL53_FRAME` chunks;
  [`FrameReassembler`](api.md) rebuilds the full frame before decode.
- The advanced ST DCI byte layouts are exposed as pure codecs
  ([`vl53l8::advanced`](api.md#vl53l8cx-tof)): the motion-indicator configuration,
  crosstalk-margin scaling, and the 64-entry detection-threshold block.

## One decoder, both variants

Two silicon/firmware variants share the **same results-frame layout**, so this
crate decodes both through one module with a [`Variant`](api.md) selector — no
duplicated CH path:

- **`Variant::Cx`** — the base ranging sensor (this page). Also the geometry for
  any device streaming ULD 2.1.0 footer frames (the DEPZ firmware default).
- **`Variant::Ch`** — the VL53L8CH, a superset that additionally emits **Compact
  Network Histograms (CNH)** and carries its own production USB PID `0xED40`.
  See the [VL53L8CH docs](../vl53l8ch/introduction.md).

The only variant-specific step in the whole decode is the frame-tail footer-id
offset (`size-12` for CX / ULD 2.1.0 vs `size-4` for CH / ULD 2.0.16); the
advanced DCI codecs are shared verbatim.

## When to use it

Use the VL53L8 when you need a **depth image** — gesture/zone occupancy, small
obstacle maps, people counting, hand tracking — in a package far smaller and
cheaper than a stereo camera, indoors or in the dark. For a single distance the
[SR04](../sr04/introduction.md) is simpler; for orientation use the
[BNO086](../bno086/introduction.md). Reach for **CH/CNH** only when you need raw
return histograms (multi-return analysis, material work).

## Key concepts

- **Host-side ULD decode** — frame parsing is pure Rust
  (`parse_frame`), a faithful port of the ST driver's block walk. The device is
  just a register bridge.
- **Chunked frames** — a frame arrives as `RPT_VL53_FRAME` chunks
  ([`FrameChunk`](api.md), ≤ [`STREAM_CHUNK_MAX`](api.md) = 1528 data bytes);
  the reassembler resets on `offset == 0`, requires contiguous offsets, and
  completes at `full_size`.
- **Raw fixed-point is authoritative** — `Vl53l8Results` keeps wire integers:
  distance is pre-scaled `floor(raw/4)` mm; `signal_per_spad` / `ambient_per_spad`
  are ÷2048 for real units, `range_sigma_mm_raw` ÷128. `target_status` 5/9 =
  valid, 255 = no target.
- **Resolution follows the frame** — arrays are sized to the zones actually
  present ([`Vl53l8Results::resolution`](api.md), 16 or 64), row-major.
- **Extension points** — live ULD init/config (firmware download + the DCI
  register bridge) is out of scope, and CNH decode is a CH-only extension point;
  see [What it is not](../guide.md#what-it-is-not).

## See also

- [VL53L8CX user guide](guide.md) — reassemble, decode, read the grid, advanced
  DCI codecs, gotchas.
- [VL53L8CH docs](../vl53l8ch/introduction.md) — the CNH superset.
- [API reference](api.md) — `parse_frame`, `Vl53l8Results`, `FrameReassembler`,
  `Variant`, and the advanced DCI codecs.
