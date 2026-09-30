# VL53L8CH — introduction

The **VL53L8CH** is the superset of the [VL53L8CX](../vl53l8cx/introduction.md):
the same STMicroelectronics multizone Time-of-Flight imager, same board
firmware, same register protocol, same host-side ULD — **plus Compact Network
Histograms (CNH)**. It carries its own production USB PID (`0xED40`) and runs
the VL53LMZ 2.0.16 sensor firmware.

Everything the CX does, the CH does **identically** — one live class,
`depz::Vl53l8`, drives both (and the I2C-board
[VL53L7CH](../vl53l7ch/introduction.md), which streams CNH the same way),
and they stream the same results-frame layout, so the single
`depz::vl53l8::decode_frame` decodes both. Start with the CX docs:

- [VL53L8CX introduction](../vl53l8cx/introduction.md) — what the ToF sensor is,
  the live class, the frame, the key concepts.
- [VL53L8CX user guide](../vl53l8cx/guide.md) — open and init, configuration,
  ranging, callbacks and streams, power modes, crosstalk, thresholds, the
  motion indicator, record / replay, and the decode layer.

This page and the [CH guide](guide.md) cover **only what CH adds on top**.

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main() {
    auto tof = depz::open_vl53l8();            // PID 0xED40 -> model L8CH
    tof->init();                               // downloads the VL53LMZ firmware
    tof->set_resolution(depz::vl53l8::RESOLUTION_8X8);
    tof->set_ranging_frequency_hz(15);

    depz::CnhSetup cnh;
    cnh.init_config(/*start_bin=*/10, /*num_bins=*/20, /*sub_sample=*/2);
    cnh.create_agg_map(depz::vl53l8::RESOLUTION_8X8, 0, 0, 2, 2, 4, 4);  // 16 aggregates
    tof->configure_cnh(cnh);                   // 1708 bytes of CNH per frame

    tof->start_ranging();
    depz::Vl53l8LiveFrame f = tof->get_frame();
    tof->stop_ranging();
    auto h = depz::vl53l8::decode_cnh(cnh.nb_of_aggregates, cnh.feature_length,
                                      depz::as_bytes(*f.frame.cnh_raw));
    std::printf("%zu histograms of %zu bins\n", h.aggregates.size(),
                h.aggregates[0].hist.size());
}
```

## What CH adds: Compact Network Histograms

A normal frame gives you one distance (and status/signal/…) per zone. CNH adds a
per-**aggregate** distance **histogram**: for each aggregate — a group of
zones you choose — the photon return counts binned by range. That exposes the
raw return structure the single-distance pipeline collapses — multiple returns
in one zone, partial occlusion, glass/edge effects, material signatures.

In the SDK the CH difference is small:

- **The live class** — `open_vl53l8()` / `open_device()` give a
  `depz::Vl53l8` with `model() == Vl53l8Model::L8CH` for the production PID
  `0xED40`, and `init()` then downloads the VL53LMZ firmware. Build a
  `depz::CnhSetup` (which histogram bins, which zones form each aggregate),
  arm it with `configure_cnh()` before `start_ranging()`, and every streamed
  frame carries the raw CNH block in `frame.cnh_raw`. On an `L8CX` model
  `configure_cnh()` throws `WrongTypeError`.
- **Shared ranging path** — the CH results frame decodes through the exact same
  `decode_frame`, using the CH footer-id offset `FOOTER_ID_OFF_CH` (= 4) instead
  of the CX offset (= 12), selectable via `depz::vl53l8::Variant::CH` /
  `footer_id_off(Variant::CH)`. The live class picks it for you.
- **CNH histogram decode** — `decode_cnh()` turns the CNH block into
  per-aggregate histograms (`CnhFrame` / `CnhAggregate`): a byte-exact port of
  the ST ULD CNH plugin decode for the configuration `CnhSetup::init_config()`
  programs, checked against a golden vector. You pass it the aggregate count
  and bins per aggregate the sensor was configured with — the
  `nb_of_aggregates` and `feature_length` of your `CnhSetup`.

The CNH path has been checked on a real VL53L8CH (board `TMNQ8E3PRR`): 8×8 at
15 Hz, ~600 mm, 1708 bytes of CNH per frame; a recording of that session is
replayed strictly by the SDK's tests.

## When to use CH over CX

Reach for CH only when you need the raw return histograms — multi-return
analysis, material/reflectivity work, glass and edge disambiguation. For plain
depth imaging the [CX](../vl53l8cx/introduction.md) path is identical and
simpler (and a CH board ranges exactly like a CX with CNH left off).

## See also

- [VL53L8CH user guide](guide.md) — arming CNH on the live class, the CNH
  decode, the CH footer offset, the CH replay test.
- [VL53L8CX docs](../vl53l8cx/introduction.md) — the base sensor CH shares.
- [API reference](api.md) — the CH-specific symbols (`CnhSetup`,
  `FOOTER_ID_OFF_CH` and the CNH decode); the shared surface, `depz::Vl53l8`
  included, lives in the [CX reference](../vl53l8cx/api.md).
