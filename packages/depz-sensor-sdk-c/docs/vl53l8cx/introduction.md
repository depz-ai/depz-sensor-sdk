# VL53L8CX — introduction

The **VL53L8CX** is a STMicroelectronics multizone Time-of-Flight ranging
sensor: a tiny laser depth **imager** that returns a 4×4 or 8×8 grid of distances
up to ~4 m. On the DEPZ sensor line the MCU is a thin SPI **register bridge** —
the sensor streams raw ranging frames to the host, and this C SDK reassembles and
decodes them into a `depz_vl53l8_frame`.

This is the base ToF sensor. Its superset — the **VL53L8CH**, which adds Compact
Network Histograms — has its own [introduction](../vl53l8ch/introduction.md) and
[guide](../vl53l8ch/guide.md); the decode path documented here is shared by both.

## What the SDK covers

The register-bridge **streaming decode path** is implemented and verified, and is
identical for the CX and CH variants:

- **Chunk parse** — `depz_vl53l8_unpack_chunk()` decodes one `RPT_FRAME` (`0x93`)
  payload into a `depz_vl53l8_chunk` (timestamp, `full_size`, `offset`, data).
- **Reassembly** — `depz_vl53l8_reassembler` stitches the ≤1528-byte chunks back
  into a whole sensor frame (reset on `offset==0`, contiguous chunks, gap
  discards the frame).
- **Frame decode** — `depz_vl53l8_decode_frame()` turns a reassembled raw frame
  into a `depz_vl53l8_frame`: per-zone distance, target status, target count,
  signal/ambient per SPAD, SPAD count, range sigma, reflectance, plus a per-frame
  silicon temperature.
- **Advanced DCI codecs** — xtalk margin (`depz_vl53l8_xtalk_margin_to_raw` /
  `_from_raw`), detection thresholds (`depz_vl53l8_pack_thresholds`), and the
  default motion configuration (`depz_vl53l8_motion_cfg_default_pack`). These are
  pure encode/decode, shared by both variants.

What is **not** here is the live ULD init/config driver (firmware download + DCI
register sequences) — see [Extension points](../guide.md#extension-points).

## When to use it

Use the VL53L8 when you need a **depth image** — gesture/zone occupancy, small
obstacle maps, people counting, hand tracking — in a package far smaller and
cheaper than a stereo camera, indoors or in the dark. For a single distance the
SR04 is simpler; for orientation use the BNO086. Reach for **CH/CNH** only when
you need raw return histograms (multi-return analysis, material work).

## Key concepts

- **Raw fixed-point integers** — the frame arrays hold the wire integers exactly
  (no float scaling): the host applies `range_sigma_mm = raw/128`, signal/ambient
  in kcps/SPAD, etc. `distance_mm` is the ST-scaled value (`raw/4`, floored) to
  match `GetRangingData`.
- **Resolution** — `DEPZ_VL53L8_RES_4X4` (16 zones) or `DEPZ_VL53L8_RES_8X8`
  (64). The frame's arrays are valid for `[0, resolution)`, row-major; a frame is
  a flat array you index or reshape to (4,4)/(8,8) yourself.
- **Header/footer id check** — `depz_vl53l8_decode_frame` verifies the header id
  at `[8..9]` equals the footer id (`DEPZ_VL53L8CX_FOOTER_ID_OFFSET` from the
  end); a mismatch returns `-1` (corrupt frame).
- **CX vs CH** — `depz_vl53l8_variant` names the split. The decode is
  variant-agnostic; the one CH-exclusive feature (CNH) is an extension point. See
  the [VL53L8CH docs](../vl53l8ch/introduction.md).
- **`target_status`** — 5/9 = valid range, 255 = no target.

## See also

- [VL53L8CX user guide](guide.md) — reassemble + decode a frame, the frame
  fields, the advanced DCI codecs, gotchas.
- [VL53L8CH docs](../vl53l8ch/introduction.md) — the CNH superset.
- [API reference](api.md) — `depz_vl53l8_frame`, the reassembler, and the decode
  + advanced codecs.
