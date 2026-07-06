# VL53L8CX — user guide

Hands-on guide to the ToF decode layer in `depz/vl53l8.hpp`, the base VL53L8CX
surface. For what the sensor is, CX vs CH, and the concepts, read the
[introduction](introduction.md); for exact signatures see the
[API reference](api.md). The **VL53L8CH** superset (CNH histograms) has its
[own guide](../vl53l8ch/guide.md) — it shares everything below.

This SDK is the **decode layer** — reassembly and frame decode only. Bringing a
part up on live hardware (the ULD firmware download and DCI handshakes) is a
documented extension point, not implemented here.

## Contents

- [Reassemble and decode a frame](#reassemble-and-decode-a-frame)
- [The frame: fields and the zone grid](#the-frame-fields-and-the-zone-grid)
- [Variants and the footer-id offset](#variants-and-the-footer-id-offset)
- [Advanced-feature DCI codecs](#advanced-feature-dci-codecs)
- [Gotchas](#gotchas)

## Reassemble and decode a frame

Frames arrive as chunked `RPT_VL53_FRAME` reports. Feed each report's payload to
a `FrameReassembler`; when a frame completes, decode it:

```cpp
#include "depz/framing.hpp"
#include "depz/vl53l8.hpp"
#include <variant>

depz::PacketParser parser;
depz::vl53l8::FrameReassembler reasm;

for (const auto& ev : parser.feed(depz::as_bytes(rx))) {
    auto* pkt = std::get_if<depz::Packet>(&ev);
    if (!pkt || pkt->payload.size() < 12) continue;

    auto chunk = depz::vl53l8::FrameChunk::unpack(depz::as_bytes(pkt->payload));
    auto done  = reasm.feed(chunk);                // std::optional<{ts, bytes}>
    if (!done) continue;

    auto frame = depz::vl53l8::decode_frame(depz::as_bytes(done->second),
                                            depz::vl53l8::FOOTER_ID_OFF_CX);
    if (!frame) continue;                          // corrupt frame (id mismatch)
    frame->timestamp_us = done->first;             // carry the capture timestamp
    // … use *frame …
}
// health counters: reasm.completed / reasm.discarded (chunks dropped on a gap)
```

`FrameChunk` is `{timestamp_us, full_size, offset, data}`; the reassembler resets
on `offset == 0`, discards the frame-in-progress on any gap, and completes when
the accumulated bytes equal `full_size`.

## The frame: fields and the zone grid

`Vl53l8Frame` carries every per-zone output as a `std::vector<…>` sized to the
active resolution (16 or 64 zones), row-major, plus per-frame scalars. Raw wire
integers throughout:

| field | type | meaning |
|---|---|---|
| `distance_mm` | `int32` | per-zone distance in mm (ST /4 floor already applied) |
| `target_status` | `uint8` | 5/9 = valid, 255 = no target |
| `nb_target_detected` | `uint8` | targets found in the zone |
| `signal_per_spad` | `uint32` | signal rate, kcps/SPAD (raw) |
| `ambient_per_spad` | `uint32` | ambient rate, kcps/SPAD (raw) |
| `nb_spads_enabled` | `uint32` | enabled SPADs in the zone |
| `range_sigma_mm_raw` | `uint16` | range std-dev, **raw** (real = raw / 128) |
| `reflectance` | `uint8` | estimated reflectance, % |
| `resolution` | `int` | 16 or 64 (active zone count) |
| `silicon_temp_degc` | `int` | per-frame sensor temperature, °C |

Zone index runs row-major, so reshape to `(4,4)` / `(8,8)` by
`row = i / cols, col = i % cols`, with `cols = 4` at `RESOLUTION_4X4` or `8` at
`RESOLUTION_8X8`. Mask invalid zones on `target_status == 255`:

```cpp
int cols = (frame->resolution == depz::vl53l8::RESOLUTION_8X8) ? 8 : 4;
for (int i = 0; i < frame->resolution; ++i) {
    if (frame->target_status[i] == 255) continue;    // no target in this zone
    int r = i / cols, c = i % cols;
    std::printf("[%d,%d] %d mm\n", r, c, frame->distance_mm[i]);
}
```

## Variants and the footer-id offset

CX and CH stream the same frame; the decoder only needs the variant's footer-id
offset. Use the constant directly, or derive it from a `Variant`:

```cpp
using depz::vl53l8::Variant;
int off = depz::vl53l8::footer_id_off(Variant::CX);   // 12  (CH → 4)
auto frame = depz::vl53l8::decode_frame(raw, off);
```

`decode_frame`'s `footer_id_off` argument defaults to `FOOTER_ID_OFF_CX`, so CX
callers can omit it. `swap_buffer()` (byte-reverse every 32-bit word) is exposed
for the same ST buffer-swap the decoder applies internally.

## Advanced-feature DCI codecs

These build the DCI blocks the ST ULD plugins exchange. They are pure codecs —
you frame and send them; the SDK does not run the live handshake.

```cpp
// crosstalk margin (kcps/SPAD) <-> DCI raw word
std::uint32_t raw = depz::vl53l8::xtalk_margin_raw(50.0);
double kcps       = depz::vl53l8::xtalk_margin_kcps(raw);

// per-zone detection thresholds (interrupt-on-threshold): 64-entry block
std::vector<depz::vl53l8::DetectionThreshold> th(1);
th[0].measurement = depz::vl53l8::THRESH_DIST_MM;   // distance window, all zones
th[0].low_thresh = 200; th[0].high_thresh = 600; th[0].zone_num = 128;
depz::bytes block  = depz::vl53l8::pack_detection_thresholds(th);   // 768 bytes
depz::bytes valid  = depz::vl53l8::detection_thresholds_valid_status();  // 8 bytes

// motion indicator — default config for the active resolution
depz::vl53l8::MotionConfig mc =
    depz::vl53l8::motion_config_init(depz::vl53l8::RESOLUTION_8X8);
depz::bytes mc_bytes = mc.pack();   // 156-byte VL53L8CX_Motion_Configuration
```

Threshold `measurement` selectors are the `THRESH_*` constants
(`THRESH_DIST_MM`, `THRESH_SIGNAL_PER_SPAD_KCPS`, …); missing entries in the
64-slot block default to zeros, and `low`/`high` are scaled by the measurement's
factor on pack.

## Gotchas

- **`decode_frame` can return `std::nullopt`** — a header/footer id mismatch
  means a corrupt frame; skip it and keep the reassembler running.
- **Pass the right footer-id offset** — CX = 12, CH = 4. The wrong offset makes
  every frame fail the id check.
- **Fields are raw integers** — `range_sigma_mm_raw` is raw (÷128 for mm);
  `signal_per_spad` / `ambient_per_spad` are raw kcps/SPAD. Convert at the edge.
- **Reassembly is gap-sensitive** — a dropped chunk discards the whole
  frame-in-progress (counted in `reasm.discarded`); this is by design.
- **Live ULD init is not here** — power modes, xtalk *calibration*, and the
  firmware download are the hardware extension point; this layer covers the
  DCI codecs and the frame decode, verified against golden vectors.
- **CNH is CH-only** — see the [VL53L8CH guide](../vl53l8ch/guide.md); its decode
  is a documented, not-yet-implemented stub.
