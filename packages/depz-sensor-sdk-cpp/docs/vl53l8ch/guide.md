# VL53L8CH — user guide

Hands-on guide to the CH side of the VL53L8 in C++. For what the sensor is
and why CH exists, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md).

**The CH shares the entire VL53L8CX surface.** The live class `depz::Vl53l8`
— opening, `init()`, configuration, ranging, callbacks and streams, power
modes, crosstalk, detection thresholds, the motion indicator — and the decode
layer — reassembly, `decode_frame`, the `Vl53l8Frame` fields, the DCI codecs —
work exactly as in the [VL53L8CX user guide](../vl53l8cx/guide.md); read that
first. This page covers **only the CNH addition, the CH footer offset and the
CH replay test**.

## Contents

- [Stream CNH histograms](#stream-cnh-histograms)
- [Building a CnhSetup](#building-a-cnhsetup)
- [Record and replay](#record-and-replay)
- [Decode layer](#decode-layer)
- [Gotchas](#gotchas)

## Stream CNH histograms

Open the board (the production PID `0xED40` makes it an `L8CH`), `init()`,
set resolution and frequency, build a `depz::CnhSetup`, arm it with
`configure_cnh()` **while stopped**, then range as usual — every streamed frame
now carries the raw CNH block in `frame.cnh_raw`:

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main() {
    auto tof = depz::open_vl53l8();
    if (tof->model() != depz::Vl53l8Model::L8CH) {
        std::puts("not a VL53L8CH: no CNH");
        return 1;
    }
    tof->init();
    tof->set_resolution(depz::vl53l8::RESOLUTION_8X8);
    tof->set_ranging_frequency_hz(15);

    depz::CnhSetup cnh;
    cnh.init_config(/*start_bin=*/10, /*num_bins=*/20, /*sub_sample=*/2);
    cnh.create_agg_map(depz::vl53l8::RESOLUTION_8X8, /*start_x=*/0, /*start_y=*/0,
                       /*merge_x=*/2, /*merge_y=*/2, /*cols=*/4, /*rows=*/4);
    std::printf("CNH buffer %zu bytes\n", cnh.required_memory());   // 1708
    tof->configure_cnh(cnh);

    auto frames = tof->frames();
    tof->start_ranging();
    for (int i = 0; i < 15; i++) {
        auto f = frames.next(std::chrono::milliseconds(1000));
        if (!f || !f->frame.cnh_raw) continue;
        auto h = depz::vl53l8::decode_cnh(cnh.nb_of_aggregates, cnh.feature_length,
                                          depz::as_bytes(*f->frame.cnh_raw));
        const depz::vl53l8::CnhAggregate& a0 = h.aggregates[0];
        std::printf("zone 27 %d mm; aggregate 0: bin 0 = %.2f, ambient %.2f\n",
                    f->frame.distance_mm[27], a0.hist[0], a0.ambient);
    }
    tof->stop_ranging();
}
```

`configure_cnh()` checks the setup (`required_memory()`), writes it to the
sensor and arms the CNH block for the next `start_ranging()`. On an `L8CX`
model it throws `WrongTypeError` — the CX firmware has no CNH. The zone
fields of the frame are unchanged; only `cnh_raw` is added. On a real board,
8×8 at 15 Hz with the setup above streams ~600 mm frames with 1708 bytes of
CNH each.

## Building a CnhSetup

`depz::CnhSetup` mirrors ST's `VL53LMZ_Motion_Configuration` for CNH. Fill it
with the two plugin helpers rather than by hand:

- **`init_config(start_bin, num_bins, sub_sample)`** — resets the setup and
  picks the histogram window: start at device bin `start_bin`, keep
  `num_bins` CNH bins (the `feature_length`), each summing `sub_sample`
  device bins. It also sets the fixed CNH options the decode assumes (no
  ping-pong, no variance; ambient level, crosstalk removal and the reference
  residual kept).
- **`create_agg_map(resolution, start_x, start_y, merge_x, merge_y, cols, rows)`**
  — groups zones into aggregates: a `cols` × `rows` grid of aggregates, each
  `merge_x` × `merge_y` zones, starting at zone column `start_x`, row
  `start_y`. The example's `(64, 0, 0, 2, 2, 4, 4)` cuts the 8×8 grid into
  16 aggregates of 2×2 zones. A map that does not fit the grid throws
  `ArgumentError`; zones outside it belong to no aggregate. Pass the
  resolution the sensor runs.
- **`required_memory()`** — the bytes the sensor's CNH buffer needs for this
  setup; `ArgumentError` when the map is blank or the total is above the
  on-device cap (6160 bytes). Fewer aggregates or bins fit more easily.

CNH and the [motion indicator](../vl53l8cx/guide.md#the-motion-indicator)
program the same configuration block on the sensor (ST's motion-detector
configuration), so arm one or the other in a session, not both.

## Record and replay

A CH session records and replays like any other (see the
[CX guide](../vl53l8cx/guide.md#record-and-replay-in-tests)); pass
`Vl53l8Model::L8CH` to `Vl53l8::open`, since a replay has no USB PID. The
SDK's own test `device_l8ch_cnh_replay` does exactly this with a real
VL53L8CH session, `contracts/vectors/recordings/vl53l8ch_cnh_8x8_15hz.depzrec`
— `init()` with the whole firmware download, 8×8 at 15 Hz, CNH armed, a
burst of streamed frames — in strict mode, and checks every frame against
`vl53l8ch_cnh_8x8_15hz.expected.json`:

```cpp
#include "depz/device.hpp"

bool replay_cnh(const std::string& path) {
    auto tof = depz::Vl53l8::open(depz::Link::replay(path, /*strict_tx=*/true),
                                  depz::Vl53l8Model::L8CH);
    int phases = 0;
    tof->init([&](const std::string&, std::size_t, std::size_t) { phases++; });
    tof->set_resolution(depz::vl53l8::RESOLUTION_8X8);
    tof->set_ranging_frequency_hz(15);
    depz::CnhSetup cnh;
    cnh.init_config(10, 20, 2);
    cnh.create_agg_map(depz::vl53l8::RESOLUTION_8X8, 0, 0, 2, 2, 4, 4);
    tof->configure_cnh(cnh);
    auto frames = tof->frames(64);
    tof->start_ranging();
    auto f = frames.next(std::chrono::milliseconds(5000));
    tof->stop_ranging();
    return phases > 5 && f && f->frame.cnh_raw && tof->frame_parse_errors() == 0;
}
```

The Python, C and C++ SDKs replay the same capture byte for byte.

## Decode layer

For bytes you read yourself, the CH differs from the
[CX decode layer](../vl53l8cx/guide.md#decode-layer) in two places.

### Decoding a CH frame

Identical to the CX, except you pass the CH footer-id offset so the decoder's
header/footer id check matches the VL53LMZ 2.0.16 layout:

```cpp
#include "depz/vl53l8.hpp"

using depz::vl53l8::Variant;
// … reassemble chunks exactly as in the CX guide …
auto frame = depz::vl53l8::decode_frame(depz::as_bytes(done->second),
                                        depz::vl53l8::footer_id_off(Variant::CH));
//                                       == depz::vl53l8::FOOTER_ID_OFF_CH (4)
```

Everything else — `FrameChunk`, `FrameReassembler`, the `Vl53l8Frame` fields,
`swap_buffer`, and the xtalk / threshold / motion codecs — is the shared surface
documented in the [CX guide](../vl53l8cx/guide.md#decode-layer).

### Decode CNH histograms

The CNH data block is a distinct layout from the ranging results;
`decode_cnh()` decodes it, given the aggregate count and bins per aggregate
the sensor was configured with:

```cpp
#include <cstdio>
#include "depz/vl53l8.hpp"

// cnh: a captured CNH data block, word-swapped like the ranging blocks
void print_cnh(const depz::bytes& cnh) {
    auto h = depz::vl53l8::decode_cnh(/*nb_of_aggregates=*/16, /*feature_length=*/20,
                                      depz::as_bytes(cnh));
    std::printf("%zu aggregates, ref residual %.3f, aggregate 0 bin 0 = %.2f\n",
                h.aggregates.size(), h.ref_residual,
                h.aggregates.empty() ? 0.0 : h.aggregates[0].hist[0]);
}
```

Each `CnhAggregate` carries the float bins (`hist`), their raw form
(`hist_raw` / `hist_scaler`: value = raw / 2^scaler) and the ambient level.
Pass the configuration the sensor actually runs: `decode_cnh()` throws
`std::length_error` when the block is shorter than those counts imply and
`std::invalid_argument` for an out-of-range count, but a block long enough for
a wrong config still decodes to garbage. The live class and the
[VL53L7CH](../vl53l7ch/guide.md) both deliver the block inside every streamed
frame, in `Vl53l8Frame::cnh_raw`.

## Gotchas

- **CNH is CH-only** — `configure_cnh()` on an `L8CX` model throws
  `WrongTypeError`. With `Vl53l8::open(Link, model)` the model is yours to
  pass; `open_vl53l8()` takes it from the USB PID (`0xED40` → `L8CH`).
- **Arm CNH before ranging** — `configure_cnh()`, like every setter, is
  refused while ranging; set the resolution first and build the aggregate
  map for it.
- **Decode with the setup you armed** — `decode_cnh(cnh.nb_of_aggregates,
  cnh.feature_length, …)`; the wrong counts decode garbage or throw.
- **CNH and the motion indicator share one sensor block** — arm one or the
  other, not both.
- **Use the CH footer offset** (decode layer) — `FOOTER_ID_OFF_CH` (4), not
  the CX default (12); the wrong offset fails every frame's id check.
- **Everything else is the CX surface** — all the
  [CX gotchas](../vl53l8cx/guide.md#gotchas) apply unchanged.
