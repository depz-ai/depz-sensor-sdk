# VL53L8CX — user guide

Hands-on guide to the VL53L8CX in C++: the live class `depz::Vl53l8`
(`depz/device.hpp`) first, then the reassembly, frame decode and DCI codecs of
`depz/vl53l8.hpp` underneath it. For what the sensor is, CX vs CH, and the
concepts, read the [introduction](introduction.md); for exact signatures see
the [API reference](api.md). The rules every live device shares — the build
and the C SDK dependency, errors, threads and callbacks — are in the
[common guide](../guide.md#live-hardware). The **VL53L8CH** superset (CNH
histograms) has its [own guide](../vl53l8ch/guide.md) — it shares everything
below.

## Contents

- [Open and initialise](#open-and-initialise)
- [Configure](#configure)
- [Ranging](#ranging)
- [Callbacks vs streams](#callbacks-vs-streams)
- [The frame: fields and the zone grid](#the-frame-fields-and-the-zone-grid)
- [Power modes](#power-modes)
- [Crosstalk: margin, calibration, the calibration blob](#crosstalk-margin-calibration-the-calibration-blob)
- [Detection thresholds](#detection-thresholds)
- [The motion indicator](#the-motion-indicator)
- [Raw registers and DCI](#raw-registers-and-dci)
- [Device time, disconnects](#device-time-disconnects)
- [Record and replay in tests](#record-and-replay-in-tests)
- [Decode layer](#decode-layer)
- [Gotchas](#gotchas)

## Open and initialise

`open_vl53l8()` finds the DEPZ boards by USB id, orders them by USB serial,
probes the chosen one and returns a `std::unique_ptr<depz::Vl53l8>` — or
throws `NoDeviceError` (nothing found) or `WrongTypeError` (the board is not a
VL53L8). Then `init()` brings the sensor up:

```cpp
#include <cstdio>
#include <string>
#include "depz/device.hpp"

int main() {
    try {
        auto tof = depz::open_vl53l8();                  // the first VL53L8 by USB serial
        std::printf("%s on %s, model %s, sensor %s\n", tof->software_name().c_str(),
                    tof->port().c_str(),
                    tof->model() == depz::Vl53l8Model::L8CH ? "L8CH" : "L8CX",
                    tof->is_alive() ? "answers" : "silent");
        tof->init([](const std::string& phase, std::size_t done, std::size_t total) {
            if (total) std::printf("%s %zu/%zu\n", phase.c_str(), done, total);
            else       std::printf("%s\n", phase.c_str());
        });                                              // firmware download, ~0.8 s
        std::printf("initialised: %d\n", tof->initialized());
    } catch (const depz::NoDeviceError&) {
        std::puts("no DEPZ device plugged in");
    } catch (const depz::WrongTypeError& e) {
        std::printf("%s\n", e.what());
    }
}   // the unique_ptr closes the port
```

`init()` is the ULD's `init`: it boots the sensor's own MCU, **downloads the
~84 KB sensor firmware**, then uploads the NVM offset data, the default
crosstalk data and the default configuration. It takes ~0.8 s on a real
board (0.76 s measured). The optional `Vl53l8Progress` callback receives a
phase text and, during the big writes, `done` / `total` bytes (both 0
otherwise); it runs on the calling thread. `is_alive()` is an SPI probe:
device id `0xF0`, revision `0x0C`.

**The model** fixes which sensor firmware `init()` downloads:
`Vl53l8Model::L8CX` (ST ULD 2.1.0) or `Vl53l8Model::L8CH` (VL53LMZ 2.0.16,
adds CNH). `open_vl53l8()` and `open_device()` choose it from the USB PID —
`L8CH` for the production PID `0xED40`, `L8CX` otherwise — and `model()` reads
it back. Both firmwares range the same way; only the CH one can
[stream CNH histograms](../vl53l8ch/guide.md). The same class also serves the
I2C board (`APP_VL53L7`) with the models `L5CX` / `L7CX` / `L7CH` — see the
[VL53L7CX guide](../vl53l7cx/guide.md) for what differs there.

Pick a board with `OpenOptions` — a port, a USB serial or an index:

```cpp
#include "depz/device.hpp"

std::unique_ptr<depz::Vl53l8> open_on(const std::string& port) {
    depz::OpenOptions opt;
    opt.port = port;                                  // "/dev/ttyACM0", "COM7"
    opt.timeout = std::chrono::milliseconds(500);     // request timeout (default 200 ms)
    auto tof = depz::open_vl53l8(opt);
    tof->init();
    return tof;
}
```

Opened with `open_device()` instead, a VL53L8 is the `depz::Vl53l8` subclass
of the returned `depz::Device` — `dynamic_cast<depz::Vl53l8*>(dev.get())`
finds it. `depz::Vl53l8::open(Link, Vl53l8Model)` opens one on a link with no
probe at all and the model given by hand, for replays and fakes (see
[below](#record-and-replay-in-tests)).

`Vl53l8` is a `Device`, so the common commands work on it too:
`device_name()`, `software_name()`, `serial_number()`,
`read_mcu_temperature()`, `sync_time()`, `reset()`, the events stream.

## Configure

Call the setters **after `init()` and while not ranging** — before `init()`
they throw `ArgumentError` ("call init() first"), during ranging
`ArgumentError` ("stop ranging first"). Every setter has a getter that reads
the value back from the sensor:

```cpp
#include <cstdio>
#include "depz/device.hpp"

void configure(depz::Vl53l8& tof) {
    tof.set_resolution(depz::vl53l8::RESOLUTION_8X8);   // 16 (4x4) or 64 (8x8) zones
    tof.set_ranging_frequency_hz(15);                   // >= 2; max 60 (4x4) / 15 (8x8)
    tof.set_ranging_mode(3);                            // 1 continuous, 3 autonomous
    tof.set_integration_time_ms(20);                    // 2..1000 ms, autonomous only
    tof.set_sharpener_percent(20);                      // 0..99 %, 0 = off
    tof.set_target_order(1);                            // 1 closest, 2 strongest
    std::printf("%d zones at %u Hz, mode %u, %u ms, sharpener %u %%\n", tof.resolution(),
                static_cast<unsigned>(tof.ranging_frequency_hz()),
                static_cast<unsigned>(tof.ranging_mode()),
                static_cast<unsigned>(tof.integration_time_ms()),
                static_cast<unsigned>(tof.sharpener_percent()));
}
```

- **Resolution** — 16 or 64 zones; anything else throws `ArgumentError`.
  Set it first: the frequency limit and the motion indicator depend on it.
- **Ranging frequency** — **at least 2 Hz**: below that the sensor never
  enters its ranging loop and streams nothing, so `1` throws `ArgumentError`
  up front. The maximum is 60 Hz at 4×4 and 15 Hz at 8×8.
- **Ranging mode** — continuous (1, the default: the sensor ranges back to
  back, the integration time follows from the frequency) or autonomous (3:
  each frame integrates for `integration_time_ms` and the sensor idles in
  between, saving power). The integration time has no effect in continuous
  mode.
- **Sharpener** — blurs less of a near target's light into neighbouring
  zones; the read-back is rounded to the nearest percent.
- **Target order** — which target a zone reports first when it sees more
  than one: the closest (1) or the strongest (2).

## Ranging

`start_ranging()` starts the sensor and arms the MCU's stream: on every
data-ready the MCU reads the results frame and pushes it to the host in
chunks, where it is reassembled, decoded and handed to the `on_frame()`
callbacks and the `frames()` streams. `stop_ranging()` ends it and is
idempotent. `ranging()` tells the state:

```cpp
#include <cstdio>
#include "depz/device.hpp"

void range_for_a_second(depz::Vl53l8& tof) {
    tof.set_resolution(depz::vl53l8::RESOLUTION_4X4);
    tof.set_ranging_frequency_hz(30);
    auto frames = tof.frames();               // subscribe before start: nothing is missed
    tof.start_ranging();
    try {
        tof.set_sharpener_percent(0);
    } catch (const depz::ArgumentError& e) {
        std::printf("refused while ranging: %s\n", e.what());
    }
    int n = 0;
    long sum = 0;
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < until) {
        auto f = frames.next(std::chrono::milliseconds(200));
        if (!f) continue;                     // timeout (or the device closed)
        n++;
        sum += f->frame.distance_mm[5];       // an inner zone of the 4x4 grid
    }
    tof.stop_ranging();
    std::printf("%d frames, zone 5 mean %ld mm, %llu dropped, %llu undecoded, %llu discarded\n",
                n, n ? sum / n : 0L, static_cast<unsigned long long>(frames.dropped_count()),
                static_cast<unsigned long long>(tof.frame_parse_errors()),
                static_cast<unsigned long long>(tof.reassembler_discards()));
}
```

On a real board at 4×4 and 30 Hz this delivers 30 frames/s (~596 mm to a
target at ~0.6 m), none dropped. Two health counters watch the link:
`reassembler_discards()` counts chunked frames dropped on a gap (a missing
chunk loses the whole frame), `frame_parse_errors()` frames that reassembled
but did not decode — both 0 in a healthy session.

## Callbacks vs streams

Both receive the same streamed frames. Use either, or both at once.

- **`frames(maxsize)`** — a bounded queue (default **8**: a frame is several
  kilobytes) that your thread pulls with `next(timeout)`. When it is full the
  **oldest** frame goes and `dropped_count()` grows. It is the easy choice:
  your code runs on your thread, where it may block, log or call the device.
- **`on_frame(fn)`** — `fn` runs **on the device's reader thread** for every
  frame. Keep it short: never block, never call a method that waits for a
  reply (every configuration getter and setter, `caldata_xtalk()`, …), never
  close or destroy the device from it. An exception thrown from it is
  swallowed. It returns an `Unsubscribe` function; call it while the device
  is open.
- **`get_frame(timeout)`** — the next streamed frame, for "just give me one
  now" while ranging; `TimeoutError` when none comes (default 2 s). Each call
  subscribes afresh, so frames that arrive between two calls are not kept —
  use `frames()` to see every frame.

```cpp
#include <atomic>
#include <cstdio>
#include <memory>
#include <thread>
#include "depz/device.hpp"

void count_with_a_callback(depz::Vl53l8& tof) {
    // Shared, so the callback's copy stays valid however late its last call runs.
    auto near = std::make_shared<std::atomic<int>>(0);
    auto unsubscribe = tof.on_frame([near](const depz::Vl53l8LiveFrame& f) {
        for (std::size_t i = 0; i < f.frame.distance_mm.size(); i++)   // quick, non-blocking
            if (f.frame.target_status[i] != 255 && f.frame.distance_mm[i] < 300) {
                (*near)++;
                break;
            }
    });
    tof.start_ranging();
    depz::Vl53l8LiveFrame latest = tof.get_frame();   // waits up to 2 s
    std::this_thread::sleep_for(std::chrono::seconds(1));
    tof.stop_ranging();
    unsubscribe();
    std::printf("latest zone 0: %d mm; %d frames with something under 0.3 m\n",
                latest.frame.distance_mm[0], near->load());
}
```

A callback may still be running right after `unsubscribe()` returns, and the
SDK keeps the callback object itself until the device is destroyed — so
capture by value (or a `shared_ptr`) rather than a reference to a local.

## The frame: fields and the zone grid

`Vl53l8LiveFrame` is the decoded frame, `frame` (a `depz::vl53l8::Vl53l8Frame`,
the same struct the [decode layer](#decode-layer) produces), plus `motion`
when the [motion indicator](#the-motion-indicator) is armed (`std::nullopt`
otherwise). `Vl53l8Frame` carries every per-zone output as a
`std::vector<…>` sized to the active resolution (16 or 64 zones), row-major,
plus per-frame scalars. Raw wire integers throughout:

| field | type | meaning |
|---|---|---|
| `timestamp_us` | `uint64` | MCU µs, the frame's capture time |
| `distance_mm` | `int32` | per-zone distance in mm (ST /4 floor already applied) |
| `target_status` | `uint8` | 5/9 = valid, 255 = no target |
| `nb_target_detected` | `uint8` | targets found in the zone |
| `signal_per_spad` | `uint32` | signal rate, kcps/SPAD (raw) |
| `ambient_per_spad` | `uint32` | ambient rate, kcps/SPAD (raw) |
| `nb_spads_enabled` | `uint32` | enabled SPADs (light-sensing cells) in the zone |
| `range_sigma_mm_raw` | `uint16` | range std-dev, **raw** (real = raw / 128) |
| `reflectance` | `uint8` | estimated reflectance, % |
| `resolution` | `int` | 16 or 64 (active zone count) |
| `silicon_temp_degc` | `int` | per-frame sensor temperature, °C |
| `cnh_raw` | `optional<bytes>` | the CNH block — VL53L8CH with CNH armed only |

Zone index runs row-major, so reshape to `(4,4)` / `(8,8)` by
`row = i / cols, col = i % cols`, with `cols = 4` at `RESOLUTION_4X4` or `8` at
`RESOLUTION_8X8`. Mask invalid zones on `target_status == 255`:

```cpp
#include <cstdio>
#include "depz/device.hpp"

void print_grid(const depz::Vl53l8LiveFrame& f) {
    const depz::vl53l8::Vl53l8Frame& fr = f.frame;
    int cols = (fr.resolution == depz::vl53l8::RESOLUTION_8X8) ? 8 : 4;
    for (int i = 0; i < fr.resolution; ++i) {
        if (fr.target_status[i] == 255) std::printf("    - ");   // no target in this zone
        else                            std::printf("%5d ", fr.distance_mm[i]);
        if (i % cols == cols - 1) std::printf("\n");
    }
    std::printf("sigma zone 0: %.1f mm\n", fr.range_sigma_mm_raw[0] / 128.0);
}
```

## Power modes

`set_power_mode()` puts the sensor to sleep and wakes it: `0` sleep, `1`
wake-up, `2` deep sleep. Asleep, the sensor keeps its firmware and
configuration and draws little; deep sleep draws less still but loses the
firmware:

```cpp
#include "depz/device.hpp"

void nap(depz::Vl53l8& tof) {
    tof.set_power_mode(0);          // sleep (not while ranging)
    // tof.power_mode() == 0
    tof.set_power_mode(1);          // wake-up: ready to range again
    tof.set_power_mode(2);          // deep sleep: the sensor firmware is gone
    tof.set_power_mode(1);          // waking from deep sleep re-runs init()
}
```

Waking from deep sleep runs `init()` again (the firmware download included),
so the configuration is back at the defaults — re-apply it.

## Crosstalk: margin, calibration, the calibration blob

Crosstalk is light the cover glass in front of the sensor reflects straight
back; the sensor subtracts a calibrated amount of it from every zone.

```cpp
#include <cstdio>
#include "depz/device.hpp"

void crosstalk(depz::Vl53l8& tof) {
    tof.set_xtalk_margin(50.0);     // kcps/SPAD headroom for the xtalk flag, <= 10000
    // A flat target of known reflectance at a known distance, behind the glass:
    // reflectance 1..99 %, samples 1..16, distance 600..3000 mm; blocks seconds.
    if (!tof.calibrate_xtalk(3, 4, 600))
        std::puts("nothing to calibrate (no cover glass): default xtalk kept");

    depz::bytes blob = tof.caldata_xtalk();   // the 776-byte calibration data
    // … persist `blob`; after the next init():
    tof.set_caldata_xtalk(depz::as_bytes(blob));
}
```

`calibrate_xtalk()` is the ULD's calibration: it ranges at 8×8 against the
target, reads back the result and restores your configuration. It returns
`false` when the firmware answers "nothing to calibrate" — which is what a
sensor **without cover glass** does: there is no crosstalk to measure, and
the default crosstalk data stays. Out-of-range arguments throw
`ArgumentError`. The calibration lives in the sensor's RAM only: save the
776-byte blob with `caldata_xtalk()` and restore it with `set_caldata_xtalk()`
after every `init()`.

## Detection thresholds

Detection thresholds make the sensor raise its data-ready interrupt only for
frames whose zones meet a condition — and since the stream is driven by that
interrupt, the stream then carries only those frames. Each
`vl53l8::DetectionThreshold` is one condition on one zone:

```cpp
#include <vector>
#include "depz/device.hpp"

void only_near_things(depz::Vl53l8& tof) {
    std::vector<depz::vl53l8::DetectionThreshold> th(64);
    for (int z = 0; z < 64; z++) {
        th[z].measurement = depz::vl53l8::THRESH_DIST_MM;  // what to compare
        th[z].type = 0;                // 0 in window, 1 out of window, 2 <= low, 3 > high
        th[z].low_thresh = 200;        // real units (mm here), scaled on write
        th[z].high_thresh = 600;
        th[z].zone_num = static_cast<std::uint8_t>(z);
        th[z].operation = 0;           // 0 OR, 2 AND with the other thresholds
    }
    th[63].zone_num |= 128;            // bit 7 marks the last threshold of the list
    tof.set_detection_thresholds(th);
    tof.set_detection_thresholds_enabled(true);
    tof.set_detection_thresholds_auto_stop(false);  // true: stop ranging on the first hit
    // tof.detection_thresholds() reads all 64 back, in the same units
}
```

- The block has **64 slots**; a shorter vector leaves the rest zero.
  `zone_num` is the zone index, and the ST ULD wants bit 7 (`128`) set on the
  last threshold in use.
- `measurement` is a `vl53l8::THRESH_*` selector — distance, signal, sigma,
  ambient, number of targets, target status, SPADs, motion indicator; `low`
  and `high` are in its real units and scaled on the way in (and back out).
- The window types (`type`) are 0 in window, 1 out of window, 2 ≤ low,
  3 > high, 4 = low, 5 ≠ low; `operation` combines a zone's condition with the
  others: 0 OR, 2 AND. (They are `DEPZ_VL53L8_THRESH_*` in the C SDK's
  `depz_sensor_io.h`.)
- `set_detection_thresholds_enabled(false)` turns the gating off again; all
  of this is refused while ranging.

## The motion indicator

The motion indicator compares each frame with the previous ones and reports
how much changed between `min_mm` and `max_mm`. Armed, every frame carries
`motion`:

```cpp
#include <cstdio>
#include "depz/device.hpp"

void watch_motion(depz::Vl53l8& tof) {
    tof.set_resolution(depz::vl53l8::RESOLUTION_8X8);  // set the resolution first
    tof.configure_motion_indicator(400, 1500);         // 400..4000 mm, span <= 1500
    tof.start_ranging();
    depz::Vl53l8LiveFrame f = tof.get_frame();
    tof.stop_ranging();
    if (f.motion)
        std::printf("global %u, %u of %u aggregates moved, aggregate 0: %u\n",
                    static_cast<unsigned>(f.motion->global_indicator_1),
                    static_cast<unsigned>(f.motion->nb_of_detected_aggregates),
                    static_cast<unsigned>(f.motion->nb_of_aggregates),
                    static_cast<unsigned>(f.motion->motion[0]));
}
```

`configure_motion_indicator()` programs the default motion configuration for
the **current** resolution — call it after `set_resolution()`, and again
after changing the resolution. A range below 400 mm, above 4000 mm or wider
than 1500 mm throws `ArgumentError`. `motion[]` holds one value per aggregate
(a group of zones), `nb_of_aggregates` of them.

## Raw registers and DCI

The escape hatches reach what the class does not wrap. `read_reg()` /
`write_reg()` take a 16-bit sensor register address; `dci_read()` /
`dci_write()` a DCI index (the sensor firmware's own configuration and
results table, which the ULD reads and writes through a small command
protocol):

```cpp
#include <cstdio>
#include "depz/device.hpp"

void peek(depz::Vl53l8& tof) {
    // What is_alive() does: page 0, read device id and revision, back to page 2.
    const depz::bytes page0{std::byte{0x00}}, page2{std::byte{0x02}};
    tof.write_reg(0x7FFF, depz::as_bytes(page0));
    depz::bytes id = tof.read_reg(0x0000, 2);                 // F0 0C on a VL53L8
    tof.write_reg(0x7FFF, depz::as_bytes(page2));
    std::printf("id %02X, rev %02X\n", std::to_integer<unsigned>(id[0]),
                std::to_integer<unsigned>(id[1]));

    // DCI zone configuration (VL53L8CX_DCI_ZONE_CONFIG): columns x rows (after init())
    depz::bytes zc = tof.dci_read(0x5450, 8);
    std::printf("%u x %u zones\n", std::to_integer<unsigned>(zc[0]),
                std::to_integer<unsigned>(zc[1]));
}
```

`dci_read()` / `dci_write()` need `init()` (the DCI table lives in the sensor
firmware). Keep raw access to stopped ranging: while ranging the MCU reads
the sensor on every data-ready. A raw write goes around the class: the ULD
state the SDK tracks (`initialized()`, the resolution, the armed stream) does
not know about it.

## Device time, disconnects

These work exactly as for every device — see the
[SR04 guide](../sr04/guide.md#device-time--host-time) and the
[common guide](../guide.md#live-hardware):

- `frame.timestamp_us` is MCU time; `sync_time()` once, then
  `to_host_time_us(f.frame.timestamp_us)` puts it on `depz::host_now_us()`'s
  clock.
- When the board is unplugged, the device emits a `Disconnected` event, every
  `frames()` stream hands out what it holds and then reports `closed()`, and
  every call throws `DeviceLostError`. There is no automatic reconnect: open
  the board again, and `init()` it — the sensor lost power with the board.

## Record and replay in tests

Record a live session once into a `.depzrec` capture, then replay it — in CI,
with no sensor attached. Replay is causal (each recorded reply is served after
the request that preceded it), so the test must make the same calls in the
same order; with `strict_tx` any byte your code sends differently throws
`ReplayMismatchError`. `init()` alone is hundreds of register exchanges, the
84 KB firmware download included — all replayed the same way.

```cpp
#include "depz/device.hpp"

void record(const std::string& port) {
    auto link = depz::Link::recording(depz::Link::serial(port), "l8.depzrec",
                                      "\"port\": \"live\"");
    auto tof = depz::Vl53l8::open(std::move(link), depz::Vl53l8Model::L8CX);
    tof->init();
    tof->set_ranging_frequency_hz(15);
    tof->start_ranging();
    tof->get_frame();
    tof->stop_ranging();
}

bool replay_matches() {
    auto tof = depz::Vl53l8::open(depz::Link::replay("l8.depzrec", /*strict_tx=*/true),
                                  depz::Vl53l8Model::L8CX);
    tof->init();
    tof->set_ranging_frequency_hz(15);
    auto frames = tof->frames();
    tof->start_ranging();
    auto f = frames.next(std::chrono::milliseconds(5000));
    tof->stop_ranging();
    return f && f->frame.resolution == 16;
}
```

`Vl53l8::open(Link, model)` does not probe the board, so it records nothing
extra — pass the model the recording was made with (a replay has no USB PID
to choose it from). The SDK's own tests replay a real VL53L8CH session this
way (`contracts/vectors/recordings/vl53l8ch_cnh_8x8_15hz.depzrec`: `init()`
with the firmware download, 8×8 at 15 Hz with CNH, a burst of streamed frames
checked zone by zone — see the [CH guide](../vl53l8ch/guide.md#record-and-replay)).

## Decode layer

The `depz::vl53l8` codecs, for bytes you read yourself — your own port loop
(the [common guide](../guide.md#mental-model) shows the pattern), a capture,
or a plain `depz::Device` with
[`request` / `send`](../guide.md#escape-hatch-raw-requests). They are the same
reassembly and decode the live class runs (through the C SDK), and match the
other language SDKs byte for byte via the golden vectors.

### Reassemble and decode a frame

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
the accumulated bytes equal `full_size`. The decoded `Vl53l8Frame` is the one
described [above](#the-frame-fields-and-the-zone-grid).

### Variants and the footer-id offset

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

### Advanced-feature DCI codecs

These build the DCI blocks the ST ULD plugins exchange — the blocks the live
class writes for `set_xtalk_margin()`, `set_detection_thresholds()` and
`configure_motion_indicator()`. On their own they are pure codecs: you frame
and send them.

```cpp
// crosstalk margin (kcps/SPAD) <-> DCI raw word
std::uint32_t raw = depz::vl53l8::xtalk_margin_raw(50.0);
double kcps       = depz::vl53l8::xtalk_margin_kcps(raw);

// per-zone detection thresholds (interrupt-on-threshold): 64-entry block
std::vector<depz::vl53l8::DetectionThreshold> th(1);
th[0].measurement = depz::vl53l8::THRESH_DIST_MM;   // distance window on zone 0,
th[0].low_thresh = 200; th[0].high_thresh = 600; th[0].zone_num = 128;  // the last one
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

- **`init()` first, after every power-up and every deep sleep.** The sensor
  keeps neither its firmware nor its configuration; `initialized()` tracks
  it. Budget ~0.8 s for the firmware download.
- **Ranging must be ≥ 2 Hz** — below that the sensor streams nothing, so
  `set_ranging_frequency_hz(1)` throws `ArgumentError`.
- **Configuration while ranging throws `ArgumentError`** — the stream owns
  the register bank. Stop, reconfigure, restart.
- **The model is the firmware** — on the VL53L8 board `L8CH` for the
  production PID `0xED40`, else `L8CX`; with `Vl53l8::open(Link, model)` (replays, fakes) you choose.
- **Integration time only bites in autonomous mode.**
- **Crosstalk calibration returns `false` without cover glass** — nothing to
  calibrate, the default data stays. The calibration and the blob are
  volatile: re-apply the blob after every `init()`.
- **`configure_motion_indicator()` follows the resolution** — set the
  resolution first, re-arm after changing it.
- **Callbacks run on the reader thread** — no blocking, no configuration or
  other requests, no closing the device from one; use `frames()` when in
  doubt.
- **Streams drop the oldest** when full (default 8 frames) — watch
  `dropped_count()`, and `reassembler_discards()` for frames lost on the link.
- **Fields are raw integers** — `range_sigma_mm_raw` is raw (÷128 for mm);
  `signal_per_spad` / `ambient_per_spad` are raw kcps/SPAD. Convert at the edge.
- **Pass the right footer-id offset** (decode layer) — CX = 12, CH = 4. The
  wrong offset makes every frame fail the id check; `decode_frame` then
  returns `std::nullopt`.
- **CNH is CH-only** — `configure_cnh()` on an `L8CX` throws
  `WrongTypeError`; see the [VL53L8CH guide](../vl53l8ch/guide.md).
