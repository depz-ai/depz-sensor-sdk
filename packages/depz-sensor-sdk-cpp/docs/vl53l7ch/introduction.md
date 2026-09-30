# VL53L7CH — introduction

The **VL53L7CH** is the superset of the [VL53L7CX](../vl53l7cx/introduction.md):
the same 90° multizone Time-of-Flight sensor on the same I2C board
(`APP_VL53L7` firmware, USB PID `0xED4A`), running ST's **VL53LMZ** sensor
firmware (the same blob as the [VL53L8CH](../vl53l8ch/introduction.md)) —
**plus Compact Network Histograms (CNH)**.

## What CH adds

- **CNH histograms** — per-aggregate photon-return histograms next to the
  normal per-zone distances: multiple returns in one zone, partial
  occlusion, glass and edge effects.
- **CNH streams** — unlike the VL53L8CH board, a frame with CNH (3156 B for
  16 aggregates × 20 bins, up to ~7.6 KB) comes over the normal stream in
  chunks; no poll mode is needed.
- **The fuller VL53LMZ feature set** — deep sleep and the detection-threshold
  auto-stop, which the L5CX / L7CX firmware lacks.

## What this C++ SDK offers today

Everything on the [VL53L7CX introduction](../vl53l7cx/introduction.md) —
the live class, the board commands and reports, class resolution, the frame
decode — plus CNH, on both layers:

- **The live class** `depz::Vl53l8` with the model `Vl53l8Model::L7CH`
  (`open_vl53l8()` picks it for USB PID `0xED4A` or a `VL53L7CH` in the device
  name). `init()` downloads the VL53LMZ sensor firmware; CNH then works
  exactly as on the [VL53L8CH](../vl53l8ch/guide.md): build a
  `depz::CnhSetup`, arm it with `configure_cnh()` before `start_ranging()`,
  and every streamed frame carries the CNH block in `frame.cnh_raw`. Deep
  sleep and the threshold auto-stop work too.
- **The decode layer** — `vl53l7::decode_frame()` puts the frame's CNH data
  block in `Vl53l8Frame::cnh_raw` (in the order the CNH decoder takes);
  `vl53l8::decode_cnh()` — the VL53L8CH decoder, unchanged — turns that
  block into per-aggregate histograms. You pass it the two numbers the CNH
  configuration used: the aggregate count and the bins per aggregate.

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main() {
    auto tof = depz::open_vl53l8();            // PID 0xED4A -> Vl53l8Model::L7CH
    tof->init();                               // VL53LMZ firmware over I2C, ~1.3 s
    tof->set_resolution(depz::vl53l8::RESOLUTION_8X8);
    tof->set_ranging_frequency_hz(15);
    depz::CnhSetup cnh;
    cnh.init_config(10, 20, 2);                                        // 20 bins
    cnh.create_agg_map(depz::vl53l8::RESOLUTION_8X8, 0, 0, 2, 2, 4, 4); // 16 aggregates
    tof->configure_cnh(cnh);                   // 1708 bytes of CNH per frame
    tof->start_ranging();
    depz::Vl53l8LiveFrame f = tof->get_frame();
    tof->stop_ranging();
    auto h = depz::vl53l8::decode_cnh(cnh.nb_of_aggregates, cnh.feature_length,
                                      depz::as_bytes(*f.frame.cnh_raw));
    std::printf("zone 27 %d mm, %zu histograms\n", f.frame.distance_mm[27],
                h.aggregates.size());
}
```

Checked on a real VL53L7CH (board `TXK5KAX6X4`, firmware
`APP_VL53L7_v0.53`, a target at 0.6 m): model `L7CH` and module MZEVO
detected, `init()` in 1.29 s, 8×8 at 15 Hz ~602 mm with 1708 bytes of CNH
per frame.

## When to use it

Only when you need the raw return histograms (multi-return analysis,
material or reflectivity work). For plain depth images the VL53L7CX is the
same sensor with a lighter sensor firmware.

## See also

- [VL53L7CH user guide](guide.md) — stream CNH live, decode frames with
  their CNH block.
- [VL53L7CX guide](../vl53l7cx/guide.md) — the live class on the L5/L7
  board, board commands, replies, frame decode.
- [VL53L8CH guide](../vl53l8ch/guide.md) — building a `CnhSetup`.
- [API reference](api.md) — `depz/vl53l7.hpp` plus the CNH decode and
  `CnhSetup`; the live class `depz::Vl53l8` is in the
  [VL53L8CX reference](../vl53l8cx/api.md).
