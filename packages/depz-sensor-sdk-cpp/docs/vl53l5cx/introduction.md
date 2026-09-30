# VL53L5CX — introduction

The **VL53L5CX** is STMicroelectronics' original multizone Time-of-Flight
sensor: an 8×8 (or 4×4) depth image over a **63° diagonal** field of view, up
to ~4 m. On the DEPZ line it sits on the same I2C board as the
[VL53L7CX](../vl53l7cx/introduction.md) (`APP_VL53L7` firmware, USB PID
`0xED48`) and runs the same sensor firmware (ST ULD 2.0.1). It does what the
VL53L7CX does — per-zone distances, statuses and rates, 4×4 up to 60 Hz and
8×8 up to 15 Hz, 1 Hz supported — with a **narrower, longer-reaching beam**.

## What this C++ SDK offers today

Everything on the [VL53L7CX introduction](../vl53l7cx/introduction.md)
applies unchanged — the three L5/L7 boards share one board firmware, one live
class and one decode header:

- **The live class** `depz::Vl53l8` (`depz/device.hpp`) with the model
  `Vl53l8Model::L5CX` — `open_vl53l8()` picks it for this board's USB PID
  (`0xED48`) or a `VL53L5CX` in the device name. `init()` downloads the same
  L5/L7 sensor firmware as on the L7CX, then configuration, ranging,
  callbacks and streams work as in the
  [VL53L8CX guide](../vl53l8cx/guide.md), with the L5/L7 differences listed
  in the [VL53L7CX guide](../vl53l7cx/guide.md#what-differs-from-the-vl53l8).
- **The decode layer** (`depz/vl53l7.hpp`) — the board commands and reports
  (`depz::vl53l7::pack_*`, `Vl53l7Info`), the class-resolution rule
  (`vl53l7::resolve_model()` returns `Model::Vl53l5cx` for this board's USB
  PID or device name) and the frame decode (`vl53l7::decode_frame()` into a
  `vl53l8::Vl53l8Frame`).

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main() {
    auto tof = depz::open_vl53l8();            // PID 0xED48 -> Vl53l8Model::L5CX
    tof->init();
    std::printf("module %d (0 = MZ, an L5)\n", tof->module_type().value_or(-1));
    tof->set_ranging_frequency_hz(15);
    tof->start_ranging();
    std::printf("zone 5: %d mm\n", tof->get_frame().frame.distance_mm[5]);
    tof->stop_ranging();
}
```

The L5CX runs exactly the code and sensor firmware the L7CX does, and real
VL53L5CX captures (8×8 and 4×4 at 15 Hz) replay strictly in the C SDK's
tests; the live class itself has been checked on an L7-family board, not on
an L5CX.

## When to use it

Pick the VL53L5CX when you want a multizone depth image aimed at something
rather than a wide room view: a doorway, a conveyor, a hand in front of a
screen. For the wide view take the VL53L7CX; for raw histograms the
[VL53L7CH](../vl53l7ch/introduction.md).

## Key concepts

- **Same frame, same decoder** — an L5CX frame is byte-compatible with an
  L7CX frame; there is nothing L5-specific to decode.
- **The class comes from the board, not the silicon** — the USB PID or the
  device name says L5CX. The sensor reports its own module type
  (`module_type()`, 0 = MZ for the L5) only after `init()` has loaded its
  firmware, so a board stamped L7 but carrying an L5 still ranges — compare
  the two if you want to catch the mismatch.
- **No deep sleep, no threshold auto-stop** — the L5/L7 sensor firmware
  (ST ULD 2.0.1) lacks both; the class throws `ArgumentError` for them.

## See also

- [VL53L5CX user guide](guide.md)
- [VL53L7CX guide](../vl53l7cx/guide.md) — the live class on the L5/L7
  board, board commands, replies, frame decode (everything shared).
- [API reference](api.md) — `depz/vl53l7.hpp`; the live class `depz::Vl53l8`
  is in the [VL53L8CX reference](../vl53l8cx/api.md).
