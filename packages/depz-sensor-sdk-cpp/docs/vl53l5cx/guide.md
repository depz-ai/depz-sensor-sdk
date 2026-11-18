# VL53L5CX — user guide

The VL53L5CX shares its board, firmware, live class and every codec with the
VL53L7CX: the [VL53L7CX guide](../vl53l7cx/guide.md) — opening and
initialising `depz::Vl53l8`, what differs from the VL53L8, the I2C-board
methods, record / replay, and the decode layer — applies line for line. This
page shows the places the L5CX appears by name. For concepts see the
[introduction](introduction.md); for signatures the
[API reference](api.md).

## Open it live

`open_vl53l8()` returns a `depz::Vl53l8` with `model() == Vl53l8Model::L5CX`
for this board's USB PID (`0xED48`) or a `VL53L5CX` in the device name; on a
bare link pass the model yourself:

```cpp
#include <cstdio>
#include "depz/device.hpp"

void check_l5(depz::Vl53l8& tof) {
    if (tof.model() != depz::Vl53l8Model::L5CX) return;
    tof.init();                                   // the L5/L7 sensor firmware, ~1.3 s
    auto m = tof.module_type();                   // 0 = MZ: the silicon is an L5
    if (m && *m != 0) std::puts("board says L5CX, silicon says L7 — it still ranges");
}

std::unique_ptr<depz::Vl53l8> replay_l5(const std::string& path) {
    return depz::Vl53l8::open(depz::Link::replay(path, /*strict_tx=*/true),
                              depz::Vl53l8Model::L5CX);
}
```

The C SDK's tests replay two real L5CX captures this way
(`vl53l5cx_8x8_15hz_3s`, `vl53l5cx_4x4_15hz`).

## Recognise the board (decode layer)

```cpp
#include <string>
#include "depz/usb_ids.hpp"
#include "depz/vl53l7.hpp"

// vid/pid from your port enumeration; device_name from GET_DEVICE_NAME
bool is_vl53l5cx(std::uint16_t vid, std::uint16_t pid, const std::string& device_name) {
    return depz::vl53l7::resolve_model(depz::usb_model_hint(vid, pid), device_name) ==
           depz::vl53l7::Model::Vl53l5cx;
}
```

The production PID `0xED48` resolves to the L5CX; without it, the first
`VL53L5CX` / `VL53L7CX` / `VL53L7CH` in the device name decides.

## Decode its frames (decode layer)

Exactly as on the VL53L7CX — the same reassembler, the same decoder:

```cpp
#include <cstdio>
#include "depz/vl53l7.hpp"

// raw: one frame completed by vl53l8::FrameReassembler::feed()
void print_l5_frame(depz::byte_span raw) {
    auto f = depz::vl53l7::decode_frame(raw);
    if (!f) return;
    const int side = f->resolution == depz::vl53l8::RESOLUTION_8X8 ? 8 : 4;
    for (int row = 0; row < side; ++row) {
        for (int col = 0; col < side; ++col)
            std::printf("%5d", int(f->distance_mm[row * side + col]));
        std::printf("\n");
    }
}
```

## Gotchas

- **Peel the protective film off the lens** before measuring — with the film
  on, weak-signal zones read 0.
- **No deep sleep, no threshold auto-stop** on this sensor firmware
  (ULD 2.0.1), as on the VL53L7CX — the class throws `ArgumentError`.
- **Not checked live on an L5CX** — the same code and firmware are checked
  on an L7-family board, and L5CX captures replay.
- All the [VL53L7CX gotchas](../vl53l7cx/guide.md#gotchas) apply.
