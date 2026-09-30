# VL53L7CX — user guide

Hands-on guide to the VL53L7CX in C++: the live class `depz::Vl53l8`
(`depz/device.hpp`, model `Vl53l8Model::L7CX`) first, then the codecs of
`depz/vl53l7.hpp` underneath it. For what the sensor is, read the
[introduction](introduction.md); for exact signatures see the
[API reference](api.md). The same page serves the
[VL53L5CX](../vl53l5cx/guide.md) and the [VL53L7CH](../vl53l7ch/guide.md),
which run the same board firmware.

The live class is the one the VL53L8 uses: configuration, ranging,
callbacks and streams, the frame, crosstalk, detection thresholds and the
motion indicator work exactly as in the
[VL53L8CX user guide](../vl53l8cx/guide.md) — read that for them. This page
covers what the I2C board changes. The rules every live device shares are in
the [common guide](../guide.md#live-hardware).

## Contents

- [Open and initialise](#open-and-initialise)
- [Range](#range)
- [What differs from the VL53L8](#what-differs-from-the-vl53l8)
- [Bridge info, bus speed, pin control](#bridge-info-bus-speed-pin-control)
- [Record and replay](#record-and-replay)
- [Decode layer](#decode-layer)
- [Gotchas](#gotchas)

## Open and initialise

`open_vl53l8()` finds the DEPZ boards by USB id, probes the chosen one and
returns a `std::unique_ptr<depz::Vl53l8>`. All three L5/L7 boards run the same
firmware (`APP_VL53L7`), so the model comes from the board: the production
USB PID (`0xED48` L5CX, `0xED49` L7CX, `0xED4A` L7CH), else the first
`VL53L5CX` / `VL53L7CX` / `VL53L7CH` in the device name, else `L7CX` (its
sensor firmware runs on every L5/L7 part). `Device::promote()` on a bare link
has no PID and reads the device name; `Vl53l8::open(Link, model)` takes the
model by hand.

```cpp
#include <cstdio>
#include <string>
#include "depz/device.hpp"

const char* model_name(depz::Vl53l8Model m) {
    switch (m) {
    case depz::Vl53l8Model::L5CX: return "VL53L5CX";
    case depz::Vl53l8Model::L7CX: return "VL53L7CX";
    case depz::Vl53l8Model::L7CH: return "VL53L7CH";
    case depz::Vl53l8Model::L8CH: return "VL53L8CH";
    default:                      return "VL53L8CX";
    }
}

int main() {
    auto tof = depz::open_vl53l8();
    std::printf("%s on %s\n", model_name(tof->model()), tof->port().c_str());
    tof->init([](const std::string& phase, std::size_t done, std::size_t total) {
        if (total) std::printf("%s %zu/%zu\n", phase.c_str(), done, total);
    });                                          // ~1.3 s over I2C
    if (auto m = tof->module_type())             // what the silicon says, after init()
        std::printf("module %s\n", *m == 0 ? "MZ (an L5)" : "MZEVO (an L7)");
}
```

`init()` checks the silicon, downloads the sensor firmware — the L5/L7 blob
(ST ULD 2.0.1) for L5CX / L7CX, the VL53LMZ blob for L7CH — and uploads the
default configuration; ~1.3 s at the board's 1 MHz bus (1.29 s measured).
It then reads the sensor's **module type**: `module_type()` returns 0 (MZ,
a VL53L5CX) or 1 (MZEVO, a VL53L7CX / CH), and `std::nullopt` before
`init()`. It is a diagnostic, never the class choice: it cannot tell a CX
from a CH, and a board stamped L7 but carrying an L5 still ranges.

## Range

Exactly as on the VL53L8 — only the frequency floor is lower:

```cpp
#include <cstdio>
#include "depz/device.hpp"

void slow_and_fast(depz::Vl53l8& tof) {
    tof.set_resolution(depz::vl53l8::RESOLUTION_8X8);
    tof.set_ranging_frequency_hz(1);             // fine on L5/L7 (a VL53L8 needs >= 2)
    auto frames = tof.frames();
    tof.start_ranging();
    if (auto f = frames.next(std::chrono::milliseconds(2500)))
        std::printf("1 Hz: zone 27 %d mm\n", f->frame.distance_mm[27]);
    tof.stop_ranging();

    tof.set_ranging_frequency_hz(15);            // 8x8 max; 4x4 goes to 60
    tof.start_ranging();
    std::printf("15 Hz: zone 27 %d mm\n", tof.get_frame().frame.distance_mm[27]);
    tof.stop_ranging();
}
```

On a real L7-family board at 0.6 m this reads ~596 mm at 1 Hz and ~602 mm at
15 Hz. Register reads are split at the board's 1536-byte ceiling inside the
class; frames stream in chunks of up to 1536 bytes and are reassembled and
decoded with the L5/L7 decoder for you.

## What differs from the VL53L8

| | VL53L8CX / CH | VL53L5CX / L7CX | VL53L7CH |
|---|---|---|---|
| bus to the sensor | SPI | I2C | I2C |
| `init()` | ~0.8 s | ~1.3 s | ~1.3 s |
| ranging frequency | 2..60 Hz | 1..60 Hz | 1..60 Hz |
| deep sleep (`set_power_mode(2)`) | yes | **no** (`ArgumentError`) | yes |
| threshold auto-stop | yes | **no** (`ArgumentError`) | yes |
| CNH (`configure_cnh`) | L8CH only | **no** (`WrongTypeError`) | yes |
| `module_type()` | `std::nullopt` | 0 MZ (L5) / 1 MZEVO (L7) | 1 MZEVO |
| `bridge_info()`, `set_i2c_speed_khz()`, `pin_ctrl()` | `WrongTypeError` | yes | yes |

`is_alive()` is an I2C probe on this board (revision `0x02`, or device id
`0xF0` with revision `0x01`).

## Bridge info, bus speed, pin control

Three methods talk to the I2C bridge itself; on a VL53L8 they throw
`WrongTypeError`:

```cpp
#include <cstdio>
#include "depz/device.hpp"

void bridge(depz::Vl53l8& tof) {
    depz::vl53l7::Vl53l7Info info = tof.bridge_info();   // before / after a run
    std::printf("bus %u kHz, %u I2C errors, %u frames dropped, %u INT edges\n",
                unsigned(info.i2c_khz), unsigned(info.i2c_errors),
                unsigned(info.frames_dropped), unsigned(info.int_edges));

    std::uint16_t khz = tof.set_i2c_speed_khz(450);      // snaps: returns 500
    std::printf("bus now %u kHz\n", unsigned(khz));

    tof.pin_ctrl(depz::vl53l7::PinAction::SoftCycle);    // sensor state gone
    tof.init();                                          // so init() again
}
```

- **`bridge_info()`** — the bridge's counters and pin levels
  (`vl53l7::Vl53l7Info`); the sensor is never probed. It takes the bus, so
  read it before and after a run, not during one (it can cost the frame in
  flight, counted in `frames_dropped`).
- **`set_i2c_speed_khz(khz)`** — re-times the sensor bus; the value snaps to
  100, 200, 400, 500 … 1000 kHz, and the method returns the one in effect
  (450 → 500 on a real board). Not while ranging.
- **`pin_ctrl(action)`** — `LpnOff` / `LpnOn` (the sensor's I2C interface),
  `I2cRst`, `SoftCycle` (LPn low 1 ms, high, I2C reset; clears the I2C
  counters). `LpnOff` and `SoftCycle` stop the stream and drop the sensor's
  state: `initialized()` goes `false` and `init()` must run again, firmware
  download included.

## Record and replay

Sessions record and replay as on the VL53L8 (see the
[VL53L8CX guide](../vl53l8cx/guide.md#record-and-replay-in-tests)); pass the
model to `Vl53l8::open(Link, model)`, since a replay has no USB PID. The C
SDK's tests — the same code the C++ class runs — strictly replay four real
L5/L7 captures from `contracts/vectors/recordings/`:
`vl53l5cx_8x8_15hz_3s`, `vl53l5cx_4x4_15hz`, `vl53l7ch_8x8_15hz_3s` and
`vl53l7ch_cnh_8x8_15hz`.

```cpp
#include "depz/device.hpp"

bool replay_l7(const std::string& path) {
    auto tof = depz::Vl53l8::open(depz::Link::replay(path, /*strict_tx=*/true),
                                  depz::Vl53l8Model::L7CX);
    tof->init();
    tof->set_resolution(depz::vl53l8::RESOLUTION_8X8);
    tof->set_ranging_frequency_hz(15);
    auto frames = tof->frames();
    tof->start_ranging();
    auto f = frames.next(std::chrono::milliseconds(5000));
    tof->stop_ranging();
    return f && f->frame.resolution == 64;
}
```

## Decode layer

The `depz::vl53l7` codecs, for bytes you read yourself — your own port loop
(see the [common guide](../guide.md#mental-model)), a capture, or a plain
`depz::Device` carrying the request / reply part
([`request` / `send`](../guide.md#escape-hatch-raw-requests)). They are the
wire format the live class speaks (through the C SDK).

### The codec surface

| direction | `Vl53l7Cmd` / `Vl53l7Rpt` | what | codec |
|---|---|---|---|
| host → device | `ReadReg` (0x32) | read registers | `pack_read_reg` (len 1..`READ_MAX_LEN`) |
| host → device | `WriteReg` (0x33) | write registers | `pack_write_reg` (len 1..`WRITE_MAX_LEN`) |
| host → device | `PinCtrl` (0x34) | pin control | `pack_pin_ctrl` |
| host → device | `StartStream` / `StopStream` (0x35 / 0x36) | streaming | frame size u16 / empty |
| host → device | `GetInfo` (0x37) | bridge info | empty payload |
| host → device | `SetI2cSpeed` (0x38) | sensor bus speed | `pack_set_i2c_speed` |
| device → host | `RegData` (0x91) | register data | `vl53l4`-style `cmd, ts, data` — see below |
| device → host | `Info` (0x92) | bridge info | `Vl53l7Info::unpack` |
| device → host | `Frame` (0x93) | frame chunk | `vl53l8::FrameChunk` → `FrameReassembler` → `decode_frame` |

The encoders are pure and do not police the limits — keep reads within
`1..READ_MAX_LEN` and writes within `1..WRITE_MAX_LEN` (`addr + len` ≤
`0x10000`); the firmware answers `ERR_INVALID_PARAM` otherwise.

### Send a command

The snippets call one small helper that frames a payload with
`build_packet` and hands it to your transport:

```cpp
#include "depz/framing.hpp"

// Your transport: write bytes to the board's serial port.
void port_write(const depz::bytes& frame);

void send_command(std::uint8_t cmd, const depz::bytes& payload) {
    static std::uint8_t seq = 0;
    port_write(depz::build_packet(cmd, depz::as_bytes(payload), seq++,
                                  depz::CrcType::Crc8));
}
```

### Board commands

```cpp
#include <algorithm>
#include "depz/vl53l7.hpp"

namespace v7 = depz::vl53l7;
auto op = [](v7::Vl53l7Cmd c) { return static_cast<std::uint8_t>(c); };

// a 3000-byte read, split at the L5/L7 ceiling (1536, not the VL53L8's 2048)
for (std::uint32_t off = 0; off < 3000; off += v7::READ_MAX_LEN) {
    auto len = static_cast<std::uint16_t>(
        std::min<std::uint32_t>(3000 - off, v7::READ_MAX_LEN));
    send_command(op(v7::Vl53l7Cmd::ReadReg),
                 v7::pack_read_reg(static_cast<std::uint16_t>(0x2C00 + off), len));
}

// LPn low 1 ms, high, then I2C_RST — the sensor loses its firmware
send_command(op(v7::Vl53l7Cmd::PinCtrl),
             v7::pack_pin_ctrl(static_cast<std::uint8_t>(v7::PinAction::SoftCycle)));

// re-time the sensor bus; snaps to the nearest of I2C_SPEED_STEPS_KHZ
send_command(op(v7::Vl53l7Cmd::SetI2cSpeed), v7::pack_set_i2c_speed(400));

// bridge counters (answered by Vl53l7Rpt::Info)
send_command(op(v7::Vl53l7Cmd::GetInfo), {});

// stream frames of the size the sensor reports (8x8 = 1444 B, 4x4 = 1060 B)
depz::bytes size_le{std::byte{1444 & 0xFF}, std::byte{1444 >> 8}};
send_command(op(v7::Vl53l7Cmd::StartStream), size_le);
send_command(op(v7::Vl53l7Cmd::StopStream), {});
```

| `PinAction` | effect |
|---|---|
| `LpnOff` | stop streaming, LPn low: sensor I2C interface off (state lost) |
| `LpnOn` | LPn high: interface on (the power-up default) |
| `I2cRst` | pulse the sensor's I2C_RST |
| `SoftCycle` | stop streaming, LPn low 1 ms, high, I2C_RST pulse (state lost; clears the I2C counters) |

### Decode the replies

```cpp
#include <cstdio>
#include "depz/vl53l4.hpp"
#include "depz/vl53l7.hpp"

namespace v7 = depz::vl53l7;

void on_reply(const depz::Packet& pkt) {
    if (pkt.cmd == static_cast<std::uint8_t>(v7::Vl53l7Rpt::Info)) {   // no echoed cmd
        if (auto info = v7::Vl53l7Info::unpack(depz::as_bytes(pkt.payload)))
            std::printf("bus %u kHz, %u I2C errors (last %u), %u frames dropped, %s\n",
                        unsigned(info->i2c_khz), unsigned(info->i2c_errors),
                        unsigned(info->last_i2c_error), unsigned(info->frames_dropped),
                        info->streaming ? "streaming" : "idle");
    } else if (pkt.cmd == static_cast<std::uint8_t>(v7::Vl53l7Rpt::RegData)) {
        // same layout as every DEPZ register bridge: cmd u8, timestamp u64, bytes
        if (auto r = depz::vl53l4::RegData::unpack(depz::as_bytes(pkt.payload)))
            std::printf("%zu register bytes\n", r->data.size());
    }
}
```

`Vl53l7Info` is bridge state only — the sensor is never probed. The counters
run from power-up or `DEVICE_RESET`; `SoftCycle` clears the I2C ones.
`last_i2c_error` is an `I2cError` (`Ok`, `Nack`, `Timeout` — 250 ms deadline —
`BusError`); `i2c_khz` is the speed actually in effect after snapping.

### Which board is this

All three L5/L7 boards report the same firmware name, so the firmware cannot
say which sensor is fitted. `resolve_model()` applies the rule the other SDKs
use: the production USB PID first, then the first `VL53L5CX` / `VL53L7CX` /
`VL53L7CH` in the device name, else VL53L7CX (its sensor firmware runs on every
L5/L7 part):

```cpp
#include <string>
#include "depz/usb_ids.hpp"
#include "depz/vl53l7.hpp"

// vid/pid from your port enumeration; device_name from GET_DEVICE_NAME
std::string board_name(std::uint16_t vid, std::uint16_t pid, const std::string& device_name) {
    auto m = depz::vl53l7::resolve_model(depz::usb_model_hint(vid, pid), device_name);
    return depz::vl53l7::to_string(m);   // "vl53l5cx" | "vl53l7cx" | "vl53l7ch"
}
```

A device name such as `DEPZ ToF Sensor VL53L7CH USB v2.1 …` resolves to
`Model::Vl53l7ch`; a VL53L5CH match (no such board) falls back to
`Vl53l7cx`. What the silicon itself says — L5 or L7 (`module_type`) — is only
readable after the sensor firmware is loaded, and is a diagnostic, never the
class choice.

### Reassemble and decode frames

Frames arrive as `Vl53l7Rpt::Frame` (0x93) chunks of up to
`STREAM_CHUNK_MAX` (1536) bytes. Chunk parse and reassembly are the VL53L8
ones; only the decoder differs:

```cpp
#include <cstdio>
#include "depz/vl53l7.hpp"

namespace v7 = depz::vl53l7;
namespace v8 = depz::vl53l8;

v8::FrameReassembler reasm;

void on_frame_chunk(const depz::Packet& pkt) {
    if (pkt.cmd != static_cast<std::uint8_t>(v7::Vl53l7Rpt::Frame) || pkt.payload.size() < 12)
        return;
    auto done = reasm.feed(v8::FrameChunk::unpack(depz::as_bytes(pkt.payload)));
    if (!done) return;                               // frame not complete yet

    auto frame = v7::decode_frame(depz::as_bytes(done->second));   // resolution inferred
    if (!frame) return;                              // corrupt frame (id mismatch)
    frame->timestamp_us = done->first;               // carry the capture timestamp

    if (frame->resolution == v8::RESOLUTION_8X8) {
        // the centre of an 8x8 grid is zones 27, 28, 35, 36 (row-major)
        const auto& d = frame->distance_mm;
        std::printf("centre %d mm  status %u  %d C\n",
                    int((d[27] + d[28] + d[35] + d[36]) / 4),
                    unsigned(frame->target_status[27]), frame->silicon_temp_degc);
    }
}
```

`decode_frame(raw, resolution)` takes the resolution `start_ranging` used
(`RESOLUTION_4X4` / `RESOLUTION_8X8`) when you know it; omitted, it is read
from the zone-sized ambient block (`infer_resolution()`). The frame fields,
their raw fixed-point scaling and the status values are exactly the VL53L8
ones — see the
[VL53L8CX guide](../vl53l8cx/guide.md#the-frame-fields-and-the-zone-grid).
`reasm.completed` / `reasm.discarded` count whole frames and frames lost to a
gap.

### What differs from the VL53L8 decode

| | VL53L8CX (`vl53l8::decode_frame`) | L5/L7 (`vl53l7::decode_frame`) |
|---|---|---|
| footer id | at `size − 12` | at `size − 4` (`FOOTER_ID_OFF`) |
| per-target blocks in 4×4 | 16 entries | **64 entries**, first 16 filled — trimmed by the decoder |
| resolution | from the block layout | passed in, or from the zone-sized ambient block (`0x54D0`) |
| CNH block | — | in `cnh_raw` when present (VL53L7CH) |
| chunk size | up to 1528 B | up to 1536 B |
| frame size | 8×8 ≈ 1.4 KB | 4×4 = 1060 B, 8×8 = 1444 B; up to ~7.6 KB with CNH |

Every per-zone vector is sized to the resolution. Using the VL53L8 decoder on
an L5/L7 frame fails the footer check — use `vl53l7::decode_frame()` for all
three L5/L7 boards.

## Gotchas

- **`init()` first, and again after `LpnOff` / `SoftCycle`** — both wipe the
  sensor firmware; frames stop until `init()` runs again.
- **The model comes from the board** — PID, then device name, then L7CX;
  `module_type()` is a diagnostic and never tells CX from CH.
- **No deep sleep, no threshold auto-stop on L5CX / L7CX** — `ArgumentError`;
  the L7CH has both.
- **Bridge info during a stream costs a frame** — `bridge_info()` / `GetInfo`
  takes the bus and can abandon the frame in flight (counted in
  `frames_dropped`); read it before and after a run, not in a polling loop.
- **Re-time the bus while stopped** — `SetI2cSpeed` answers `ERR_BUSY`
  mid-transfer; `set_i2c_speed_khz()` returns the snapped speed in effect.
- **Split register reads at `READ_MAX_LEN` (1536)** (decode layer) — a
  2048-byte read (the VL53L8 size) is refused with `ERR_INVALID_PARAM`. The
  live class splits for you.
- **`RPT_VL53_INFO` echoes no command byte** — do not match it on
  `payload[0]`.
- **`FrameChunk::unpack` throws on a payload shorter than 12 bytes** — check
  the size first, as above.
- **Use `vl53l7::decode_frame()` on L5/L7 frames** (decode layer) — the
  VL53L8 decoder fails their footer check.
- **1 Hz is fine here** — unlike the VL53L8, whose floor is 2 Hz.
