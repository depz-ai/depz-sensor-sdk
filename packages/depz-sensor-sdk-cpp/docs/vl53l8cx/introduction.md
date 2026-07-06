# VL53L8CX — introduction

The **VL53L8CX** is a STMicroelectronics multizone Time-of-Flight ranging
sensor: a tiny laser depth **imager** that returns a 4×4 or 8×8 grid of
distances up to ~4 m. On the DEPZ sensor line the MCU is a thin SPI **register
bridge** — the ST results frame is streamed to the host and decoded there, by
`depz::vl53l8` in `depz/vl53l8.hpp`.

This is the base ToF sensor. Its superset — the **VL53L8CH**, which adds Compact
Network Histograms — has its own [introduction](../vl53l8ch/introduction.md) and
[guide](../vl53l8ch/guide.md); the entire decode surface here applies to it
unchanged.

This SDK is the **decode layer**: frame-chunk reassembly, the shared
results-frame decoder (raw per-zone arrays), and the advanced-feature DCI codecs
(xtalk margin, detection thresholds, motion indicator). It is pure codecs — no
I/O. The live ULD init (firmware download, register-bridge handshakes) is a
hardware-dependent extension point intentionally **not** ported here; see the
[common guide](../guide.md#what-is-not-here-extension-points).

## What it does

- Per-zone distance over a **16-zone (4×4)** (`RESOLUTION_4X4`) or **64-zone
  (8×8)** (`RESOLUTION_8X8`) grid, at up to 60 Hz (4×4) / 15 Hz (8×8).
- Each frame carries far more than distance: per-zone target status, number of
  targets, signal and ambient rate, range sigma, reflectance, plus a per-frame
  silicon temperature — all decoded into `Vl53l8Frame`.
- INT-driven **push streaming**: the sensor signals data-ready and the MCU ships
  each frame to the host in ≤`STREAM_CHUNK_MAX` (1528-byte) chunks, reassembled
  by `FrameReassembler`.
- Advanced ST features have codecs: crosstalk margin
  (`xtalk_margin_raw`/`_kcps`), per-zone detection thresholds
  (`pack_detection_thresholds`), and the motion indicator (`MotionConfig`,
  `motion_config_init`).

## CX vs CH

Two silicon/firmware variants share the same register protocol and the **same
results-frame layout**, so one `decode_frame` serves both:

- **VL53L8CX** — the base ranging sensor (this page). ST ULD 2.1.0; ships as the
  dev default and carries no dedicated production USB PID.
- **VL53L8CH** — a superset that additionally emits **Compact Network Histograms
  (CNH)**, and carries its own production USB PID `0xED40` (VL53LMZ 2.0.16). See
  the [VL53L8CH docs](../vl53l8ch/introduction.md).

The only wire difference the decoder cares about is the **footer-id offset**
(`FOOTER_ID_OFF_CX` = 12, `FOOTER_ID_OFF_CH` = 4), selected via
`depz::vl53l8::Variant` / `footer_id_off(Variant)`.

## When to use it

Use the VL53L8 when you need a **depth image** — gesture/zone occupancy, small
obstacle maps, people counting, hand tracking — in a package far smaller and
cheaper than a stereo camera, indoors or in the dark. For a single distance the
[SR04](../sr04/introduction.md) is simpler; for orientation use the
[BNO086](../bno086/introduction.md). Reach for **CH/CNH** only when you need raw
return histograms.

## Key concepts

- **Host-side decode** — frame reassembly and parsing are pure C++; the device
  is just a register bridge.
- **`decode_frame` is variant-aware** — pass the right footer-id offset (it
  defaults to the CX offset). It returns `std::nullopt` for a corrupted frame
  (header/footer id mismatch).
- **`Vl53l8Frame`** — one parsed frame. Every per-zone field is a
  `std::vector<…>` sized to the active resolution (16 or 64), row-major. Values
  are raw wire integers: `distance_mm` already has the ST /4 floor scaling
  applied; `range_sigma_mm_raw` is the raw u16 (real value = raw / 128).
  `target_status` 5/9 = valid, 255 = no target.

## See also

- [VL53L8CX user guide](guide.md) — reassembly, decoding a frame, the frame
  fields, advanced DCI codecs, gotchas.
- [VL53L8CH docs](../vl53l8ch/introduction.md) — the CNH superset.
- [API reference](api.md) — `Vl53l8Frame`, `FrameReassembler`, `decode_frame`,
  and the advanced-feature codecs.
