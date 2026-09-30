# VL53L7CH — user guide

The VL53L7CH is the [VL53L7CX](../vl53l7cx/guide.md) board plus CNH
histograms. Opening and initialising the live class, the L5/L7 differences,
the I2C-board methods, board commands, replies, class resolution and the
ranging-frame decode are on the VL53L7CX guide; this page covers CNH. For
concepts see the [introduction](introduction.md); for signatures the
[API reference](api.md).

## Contents

- [Stream CNH live](#stream-cnh-live)
- [Frames with a CNH block](#frames-with-a-cnh-block)
- [Decode the histograms](#decode-the-histograms)
- [Gotchas](#gotchas)

## Stream CNH live

On the live class (`depz::Vl53l8`, model `Vl53l8Model::L7CH`) CNH is exactly
the [VL53L8CH](../vl53l8ch/guide.md#stream-cnh-histograms) path — the same
`depz::CnhSetup`, `configure_cnh()` and `frame.cnh_raw`:

```cpp
#include <cstdio>
#include "depz/device.hpp"

void cnh_burst(depz::Vl53l8& tof) {
    tof.init();                                   // disarms CNH: arm it after every init()
    tof.set_resolution(depz::vl53l8::RESOLUTION_8X8);
    tof.set_ranging_frequency_hz(15);
    depz::CnhSetup cnh;
    cnh.init_config(/*start_bin=*/10, /*num_bins=*/20, /*sub_sample=*/2);
    cnh.create_agg_map(depz::vl53l8::RESOLUTION_8X8, 0, 0, 2, 2, 4, 4);
    tof.configure_cnh(cnh);                       // WrongTypeError on L5CX / L7CX
    auto frames = tof.frames();
    tof.start_ranging();
    for (int i = 0; i < 15; i++) {
        auto f = frames.next(std::chrono::milliseconds(1000));
        if (!f || !f->frame.cnh_raw) continue;
        auto h = depz::vl53l8::decode_cnh(cnh.nb_of_aggregates, cnh.feature_length,
                                          depz::as_bytes(*f->frame.cnh_raw));
        std::printf("zone 27 %d mm; aggregate 0 bin 0 = %.2f\n",
                    f->frame.distance_mm[27], h.aggregates[0].hist[0]);
    }
    tof.stop_ranging();
}
```

On a real VL53L7CH this streams 8×8 at 15 Hz (~602 mm) with 1708 bytes of
CNH per frame. `init()` leaves the sensor with no CNH configuration, so a
re-init (after a deep sleep, a `SoftCycle`, …) disarms CNH — call
`configure_cnh()` again. The C SDK's tests replay a real CNH session
(`vl53l7ch_cnh_8x8_15hz`) strictly.

The sections below are the decode layer, for frames you reassemble yourself.

## Frames with a CNH block

With CNH configured, every streamed frame carries the usual ranging results
**and** one CNH data block. A frame is larger than one chunk (3156 B for
16 aggregates × 20 bins, up to ~7.6 KB); `vl53l8::FrameReassembler` rebuilds
it as it does any frame. `vl53l7::decode_frame()` decodes the ranging part
exactly as on the VL53L7CX and hands the CNH block over in
`Vl53l8Frame::cnh_raw`:

```cpp
#include "depz/vl53l7.hpp"

// raw: one frame completed by vl53l8::FrameReassembler::feed()
std::optional<depz::bytes> cnh_block_of(depz::byte_span raw) {
    auto f = depz::vl53l7::decode_frame(raw);
    if (!f) return std::nullopt;       // corrupt frame (id mismatch)
    return f->cnh_raw;                 // nullopt when the frame carries no CNH block
}
```

The CNH bytes are already in decode order — word-swapped like every results
block.

## Decode the histograms

`vl53l8::decode_cnh()` is the VL53L8CH decoder; it needs the aggregate count
and the bins per aggregate the sensor was configured with (the `CnhSetup` you armed):

```cpp
#include <cstdio>
#include "depz/vl53l8.hpp"

// the configuration the sensor runs, e.g. 16 aggregates x 20 bins
void print_histograms(const depz::bytes& cnh_raw) {
    depz::vl53l8::CnhFrame h = depz::vl53l8::decode_cnh(16, 20, depz::as_bytes(cnh_raw));
    std::printf("ref residual %.3f\n", h.ref_residual);
    for (std::size_t a = 0; a < h.aggregates.size(); ++a) {
        const auto& agg = h.aggregates[a];
        std::printf("aggregate %2zu (ambient %.1f):", a, agg.ambient);
        for (double v : agg.hist)            // hist_raw / 2^hist_scaler, per bin
            std::printf(" %.1f", v);
        std::printf("\n");
    }
}
```

Each `CnhAggregate` carries the float bins (`hist`) next to their raw form
(`hist_raw`, `hist_scaler`: value = raw / 2^scaler) and the aggregate's
ambient level; `CnhFrame::ref_residual` is `ref_residual_word / 2048`.

## Gotchas

- **Decode with the config the sensor runs** — a CNH block does not say how
  many aggregates or bins it holds. `decode_cnh()` throws
  `std::length_error` when the block is too short for the counts you pass, but
  a mismatched config that still fits decodes garbage. Take both numbers from
  the configuration that was sent to the sensor.
- **Configure CNH while stopped, after `init()`** — like every setter; and
  `init()` disarms it, so re-arm after any re-init. The frame size grows with
  the CNH setup.
- **CNH is L7CH-only on this board** — `configure_cnh()` on an L5CX / L7CX
  throws `WrongTypeError`.
- All the [VL53L7CX gotchas](../vl53l7cx/guide.md#gotchas) apply.
