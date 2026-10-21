# TMF8829 — user guide

Hands-on guide to the TMF8829 in C++: the live class `depz::Tmf8829`
(`depz/device.hpp`) first, then the codecs of `depz/tmf8829.hpp` underneath
it. For what the sensor is and its concepts, read the
[introduction](introduction.md); for exact signatures see the
[API reference](api.md). The rules every live device shares — errors as
exceptions, callbacks on the reader thread, bounded drop-oldest streams —
are in the [common guide](../guide.md#live-hardware).

## Contents

- [Open and init](#open-and-init)
- [Configure: presets and options](#configure-presets-and-options)
- [Stream measurements](#stream-measurements)
- [The measurement](#the-measurement)
- [Capture time and flags](#capture-time-and-flags)
- [Histograms and ref-SPAD frames](#histograms-and-ref-spad-frames)
- [Diagnostics: bridge, events, registers](#diagnostics-bridge-events-registers)
- [Recovery](#recovery)
- [Record and replay](#record-and-replay)
- [Decode layer (your own transport)](#decode-layer-your-own-transport)
- [Gotchas](#gotchas)

## Open and init

`open_tmf8829()` finds the DEPZ boards by USB id, probes the chosen one and
returns a `std::unique_ptr<depz::Tmf8829>` (`WrongTypeError` if the board is
something else). `open_device()` does the same for any board —
`dynamic_cast<depz::Tmf8829*>` tells — and `Tmf8829::open(Link)` attaches the
class to a link with no identity probe (tests, replay).

```cpp
#include <cstdio>
#include "depz/device.hpp"

std::unique_ptr<depz::Tmf8829> open_tof() {
    depz::OpenOptions opt;
    opt.port = "/dev/ttyACM0";                   // or opt.serial / opt.index
    auto tof = depz::open_tmf8829(opt);
    depz::Tmf8829SensorId id = tof->init([](std::size_t done, std::size_t total) {
        std::printf("\rfirmware %zu / %zu B", done, total);   // 15 calls, 14 444 B
    });
    std::printf("\nsensor app %s, serial 0x%08X\n", id.app_version.c_str(), unsigned(id.serial));
    return tof;
}
```

The board's MCU does not run the sensor, so a fresh board does nothing until
`init()`: it soft-resets the sensor, checks its ID, enters the sensor
bootloader, downloads the embedded ams firmware (14 444 B in ≤ 1023-byte
writes) and starts it — 0.1–0.15 s in all. It returns the application
version (`"2.200.0"`) and the sensor's serial; `sensor_id()` repeats them
later (`ArgumentError` before the first `init()`). `init()` stops a running
stream first and forgets the config.

## Configure: presets and options

```cpp
#include "depz/device.hpp"

void setup(depz::Tmf8829& tof) {
    depz::Tmf8829Options opt;
    opt.period_ms = 1;                    // as fast as the sensor can go
    opt.nr_peaks = 2;                     // up to 2 objects per zone
    opt.signal_strength = true;           // + signal per peak
    depz::Tmf8829Config cfg = tof.configure("16x16", opt);
    // cfg.rows == 16, cfg.cols == 16; cfg.field("kilo_iterations") == 586
}
```

`configure(preset, options)` loads the preset, sets the options in its
config page, writes the page, has the sensor accept it, reads it back and
checks every field it wrote. Call it while stopped (`ArgumentError` while
streaming). The ten presets (`depz::tmf8829::PRESETS`):

| preset | grid (rows × cols) | note |
|---|---|---|
| `8x8` | 8 × 8 | the default |
| `8x8-lr` | 8 × 8 | long range |
| `8x8-ha` | 8 × 8 | high accuracy |
| `8x8-xr` | 8 × 8 | extended range; raw distances in **mm**, not 0.25 mm |
| `16x16`, `16x16-ha` | 16 × 16 | plain / high accuracy |
| `32x32`, `32x32-ha` | 32 × 32 | two half-frames per measurement |
| `48x32`, `48x32-ha` | 32 × 48 | two half-frames per measurement |

`Tmf8829Options` — what `configure()` writes on top of the preset:

| field | default | meaning |
|---|---|---|
| `period_ms` | 33 | time between measurements; 1 = as fast as possible |
| `kilo_iterations` | the preset's own (586 / 2344 / 3516) | laser pulses per measurement, in thousands: more = less noise, slower |
| `nr_peaks` | 1 | peaks (objects) per zone, 1..4 |
| `signal_strength`, `noise_strength`, `xtalk` | false | add signal per peak, noise per zone, crosstalk per zone |
| `histograms` | false | the sensor also dumps its raw histograms |
| `ref_frame` | false | a 65-byte ref-SPAD frame before each results frame |
| `fields` | — | any other config-page field by name (`CFG_FIELDS`: `power_modes`, `dual_mode`, `ha_kilo_iterations`, …); overrides the ones above |

**Why every field, every time.** A preset load does not reset the result
format, the period, the histogram switch and more — they keep what the last
config wrote (histograms and the ref frame even survive `init()`). A stale
4-peak format then makes a 32×32 preset fail. So `configure()` always writes
`period_ms`, `kilo_iterations`, `nr_peaks`, the three strengths,
`histograms` and `ref_frame`, defaulted when you leave them.

The returned `Tmf8829Config` holds the preset, the 190-byte page as read back
(`page`), every named field (`fields`, `field(name)`), `histograms`,
`ref_frame` and the grid (`rows`, `cols`). `config()` returns the last one
(`nullopt` after `init()`), `config_page()` reads the page the sensor has now.

Errors: an unknown preset or field name, or a value out of range,
`ArgumentError`; the sensor refusing the page (`ERR_CONFIG_RESULT_SIZE`: the
result format is too big for the grid — 48×32 with 4 peaks and every
strength) or a field that does not read back, `ProtocolError`. A refused
`configure()` keeps the last good config.

## Stream measurements

```cpp
#include <cstdio>
#include "depz/device.hpp"

void stream_ways(depz::Tmf8829& tof) {
    auto stream = tof.measurements(4);          // pull: bounded, drop-oldest
    auto unsub = tof.on_measurement([](const depz::Tmf8829Measurement& m) {
        (void)m;                                // reader thread: copy out, return fast
    });

    tof.start();                                // MEASURE, bridge stream armed
    while (auto m = stream.next(std::chrono::milliseconds(1000))) {
        std::printf("fN %u  %u peak(s)  %.1f mm\n", unsigned(m->f_number), m->peaks(),
                    m->distance_mm.empty() ? -1.0 : m->distance_mm[0]);
        if (m->f_number > 100) break;
    }
    depz::Tmf8829Measurement one = tof.get_measurement();   // TimeoutError on silence
    (void)one;
    tof.stop();
    unsub();
}
```

`start()` arms the sensor interrupts and the bridge stream for what the
config asks (results, plus histograms and the ref frame) and starts
measuring; without a `configure()` since `init()` it configures `8x8` first.
It is a no-op while streaming. `stop()` stops the sensor, then the bridge
stream. Subscribe **before** `start()` to see the first measurement. Keep
`maxsize` small: a measurement is ~90 KB in the C queue underneath.
`streaming()` tells whether a stream runs.

## The measurement

`Tmf8829Measurement` (a `tmf8829::Measurement`) is one measurement: one
results frame (8×8, 16×16) or both half-frames merged (32×32, 48×32).

| field | meaning |
|---|---|
| `timestamp_us` | capture time, MCU µs — the middle of the measurement window ([below](#capture-time-and-flags)) |
| `flags` | `FT_*` capture-time flags, OR of the half-frames |
| `fp_mode`, `shape()` | grid mode and `(rows, cols)`: 8×8, 16×16, 32×32, 32×48 |
| `f_number` | the sensor's measurement counter |
| `peaks()` | peaks per zone in the result format |
| `distance_mm` | clock-corrected mm, `[peaks][rows][cols]` flattened; empty without peaks |
| `results` | the raw integers: `distance` (0.25 mm, or mm on `8x8-xr`), `snr`, `signal` (`[peaks][rows][cols]`), `noise`, `xtalk` (`[rows][cols]`); absent ones `nullopt` |
| `ratio_q16` | the sensor-clock ratio used for `distance_mm` (Q16, ~1.015 × 8) |
| `distance_in_mm()` | raw unit is mm (`8x8-xr`) |
| `temperature_c()` | sensor die temperature, °C |
| `frames`, `times` | header / footer (`FrameInfo`) and capture time (`FrameTime`) of each results frame (1 or 2) |
| `n_histograms`, `n_ref_frames` | how many histogram / ref-SPAD frames came with it |
| `problems`, `n_problems` | what went wrong since the previous measurement (the first 16 kept) |

The index of zone (row `r`, column `c`) of peak `p` is
`p * rows * cols + r * cols + c`:

```cpp
#include "depz/device.hpp"

double zone_mm(const depz::Tmf8829Measurement& m, unsigned p, unsigned r, unsigned c) {
    auto [rows, cols] = m.shape();
    return m.distance_mm.at(p * rows * cols + r * cols + c);
}
```

**Orientation.** The arrays keep the sensor's row-major order, which is
transposed against the scene: for a board held USB down and seen from
behind (looking where the sensor looks), the **row** index runs left →
right and the **column** index top → bottom. To draw what the sensor sees,
swap rows and columns (the DEPZ viewer does this, and rotates / mirrors on
top).

`distance_mm` is
`raw × ratio / 4` (or `raw × ratio` in mm): the sensor counts in its own
clock, ~1.5 % slow against the MCU, and the SDK measures that ratio from
the stream.

## Capture time and flags

The board only timestamps when it read a frame; the sensor's own clock
values in the frame say when it measured. The SDK fits the sensor clock to
the MCU clock over a sliding 1–2 s window and puts `timestamp_us` in the
**middle** of the measurement window, in MCU microseconds (map it to host
time with time sync — [common guide](../guide.md#time-sync-math)). With two
half-frames it is the mean of their two times. `flags` (`depz::tmf8829::FT_*`,
`flag_names(flags)` for text):

| flag | meaning |
|---|---|
| `FT_NOMINAL` | the clock ratio is not measured yet (first ~200 ms): nominal 8 µs per tick used |
| `FT_CLOCK_RESET` | the clock model restarted at this frame (a sensor clock jump) |
| `FT_EDGE_LATE` | the board's INT edge came > 2 ms later than usual — the frame waited |
| `FT_STALLED` | the measurement was stretched (the sensor waited for the histogram FIFO) |
| `FT_T1_UNSURE` | histograms on 32×32 / 48×32: the time may be up to ~20 ms late |

Without histograms, a healthy stream has only `FT_NOMINAL` on its first few
measurements.

## Histograms and ref-SPAD frames

With `histograms = true` the sensor dumps its raw histograms before each
results frame: 2 frames per measurement on 8×8 / 16×16, 8 on 32×32, 12 on
48×32 (twice that with `dual_mode`), 25 377 bytes each. With
`ref_frame = true` a 65-byte ref-SPAD frame comes before each results frame.
The live class **counts** them in the measurement (`n_histograms`,
`n_ref_frames`; the `histograms` / `ref_frames` vectors stay empty) and
hands their bytes to `on_frame()`, which sees every reassembled frame before
the measurement it completes:

```cpp
#include <mutex>
#include <vector>
#include "depz/device.hpp"

namespace tmf = depz::tmf8829;

std::mutex lock;
std::vector<depz::bytes> histograms;

void capture_histograms(depz::Tmf8829& tof) {
    depz::Tmf8829Options opt;
    opt.histograms = true;
    tof.configure("8x8", opt);
    auto off = tof.on_frame([](const tmf::RawFrame& f) {
        tmf::FrameInfo info = tmf::parse_frame(depz::as_bytes(f.data));
        if (info.kind() == tmf::FID_HISTOGRAMS) {
            std::lock_guard<std::mutex> g(lock);
            histograms.push_back(f.data);           // header + bins + footer
        }
    });
    tof.start();
    tof.get_measurement(std::chrono::milliseconds(2000));
    tof.stop();
    off();
}
```

`tmf::HistogramFrame{info, raw}.data()` gives the bytes between header and
footer; decoding the bins is left to you (the ams histogram layout).
Histograms slow the stream down (~10 measurements/s on 8×8, under 2/s on
48×32) — it is a calibration mode.

## Diagnostics: bridge, events, registers

```cpp
#include <cstdio>
#include "depz/device.hpp"

void diagnostics(depz::Tmf8829& tof) {
    depz::tmf8829::Tmf8829Info b = tof.bridge_info();     // safe mid-stream
    std::printf("INT edges %u, frames %u, aborted %u, bad headers %u, streaming %d\n",
                b.int_edges, b.frames, b.frames_aborted, b.frame_errors, int(b.streaming));
    depz::Tmf8829StreamStats s = tof.stream_stats();       // this stream, host side
    std::printf("completed %llu, dropped %llu, discarded %llu, parse errors %llu\n",
                (unsigned long long)s.frames_completed, (unsigned long long)s.frames_dropped,
                (unsigned long long)s.frames_discarded, (unsigned long long)s.frame_parse_errors);
    depz::bytes app = tof.read_registers(depz::tmf8829::REG_APP_ID, 4);   // raw access
    (void)app;
}
```

- `bridge_info()` — the bridge's own counters (never touches the sensor):
  INT edges, frames sent, frames aborted by a stop, refused headers, SPI
  timeouts, the INT pin, the armed mask, the last INT / FIFO status.
- `stream_stats()` — host-side counters of the current stream (reset at
  `start()`): frames reassembled, **dropped** by the bridge (`frame_seq`
  gaps — counted, not fatal), discarded by reassembly, parse errors,
  measurements, and the expected stop-time event swallowed.
- `on_event()` — `RPT_TMF_EVENT`s: `EV_INT` (a motion / proximity interrupt)
  or `EV_BAD_HEADER` (a frame the bridge refused). One `EV_BAD_HEADER` is
  expected when `stop()` interrupts a histogram dump; the class swallows
  that one. Any other means the sensor is lost — see [recovery](#recovery).
- `read_registers(addr, len)` / `write_registers(addr, data)` — raw sensor
  registers, split into ≤ 1024 / ≤ 1023-byte requests. A read that reaches
  the FIFO port `0xFF` keeps reading the FIFO; a write to `0xFF` stays
  there. SPI has no acknowledge: a silent sensor reads `0x00` / `0xFF`.

## Recovery

There is no sleep and no hardware reset over SPI: **recovery from any sensor
state is `init()`**, then your `configure()` again. Do it from your own
thread, not inside a callback:

```cpp
#include <atomic>
#include "depz/device.hpp"

std::atomic<bool> lost{false};

void watch_and_recover(depz::Tmf8829& tof, const depz::Tmf8829Options& opt) {
    auto off = tof.on_event([](const depz::tmf8829::Tmf8829Event& e) {
        if (e.code == depz::tmf8829::EV_BAD_HEADER) lost = true;   // just flag it
    });
    // ... later, on your thread:
    if (lost.exchange(false)) {
        tof.init();                   // stops the stream, soft reset, firmware again
        tof.configure("8x8", opt);
        tof.start();
    }
    off();
}
```

If `init()` itself fails (`TimeoutError` / `ProtocolError`), only a USB
re-plug (a power cycle) is left.

## Record and replay

A session records to a `.depzrec` file through a recording link and replays
byte-exact ([common guide](../guide.md#record-and-replay)):

```cpp
#include "depz/device.hpp"

void record(const std::string& path) {
    auto link = depz::Link::recording(depz::Link::serial("/dev/ttyACM0"), path, "\"port\": \"live\"");
    auto dev = depz::Device::promote(depz::Device::open(std::move(link)));
    auto* tof = dynamic_cast<depz::Tmf8829*>(dev.get());
    tof->init();
    tof->configure("16x16");
    tof->start();
    tof->get_measurement();
    tof->stop();
}
```

Replaying it with `Link::replay(path, /*strict_tx=*/true)` and the same calls
gets the same measurements. The class issues the same register sequences as
the Python SDK's `Tmf8829`, so a capture made by either replays in the other.
The C++ suite runs `device_tmf8829_fake_session` against the C SDK's fake
bridge with the frames captured on a board: promote, `init()` with the
whole firmware download, `configure()` with read-back, a 32×32 stream with
ref frames, an 8×8 stream with a dropped frame, events, raw registers, the
swallowed stop event, re-init.

A `.depzdata` dataset recorded by the Python or TS SDK carries `tmf8829`
records; `depz::dataset::Reader` reads them like any other kind (note its
`arrays` hold integers, so the `distance_mm` values come back truncated to
whole mm).

## Decode layer (your own transport)

The `depz::tmf8829` codecs, for bytes you read yourself — your own port loop
(see the [common guide](../guide.md#mental-model)), a capture, or a plain
`depz::Device`. Doing it by hand you own the whole sensor sequence (contract
14 §4: soft reset, bootloader, firmware download, presets, start / stop) —
the live class is far less work.

| direction | `Tmf8829Cmd` / `Tmf8829Rpt` | what | codec |
|---|---|---|---|
| host → device | `ReadReg` (0x32) | read 1..1024 sensor register bytes | `pack_read_reg(addr, len)` |
| host → device | `WriteReg` (0x33) | write 1..1023 bytes | `pack_write_reg(addr, data)` |
| host → device | `StartStream` (0x35) | arm the frame stream | `pack_start_stream(int_mask, flags)` |
| host → device | `StopStream` / `GetInfo` (0x36 / 0x37) | stop / bridge counters | empty payload |
| device → host | `RegData` (0x91) | register bytes | `RegData::unpack` |
| device → host | `Info` (0x92) | bridge counters (no echoed cmd byte) | `Tmf8829Info::unpack` |
| device → host | `Frame` (0x93) | one chunk of a frame | `FrameChunk::unpack` → `FrameReassembler` |
| device → host | `Event` (0x94) | interrupt / refused header | `Tmf8829Event::unpack` |

The encoders throw `std::invalid_argument` on an out-of-range argument before
anything is sent. The firmware image to download is `firmware_image()` (at
`FIRMWARE_LOAD_ADDR`); presets are `PRESETS` / `find_preset()`, page fields
`CFG_FIELDS` with `cfg_get()` / `cfg_set()`.

### From chunks to measurements

A frame arrives in chunks of ≤ 1024 bytes. `FrameReassembler` rebuilds it
(never stitching chunks of different `frame_seq`); `MeasurementBuilder` takes
the frames in stream order and does the rest — half-frame merge, histogram
and ref-SPAD grouping, the clock model, capture time, clock-corrected
distances:

```cpp
#include <cstdio>
#include "depz/framing.hpp"
#include "depz/tmf8829.hpp"

namespace tmf = depz::tmf8829;

tmf::FrameReassembler reasm;       // reset() both at every stream start
tmf::MeasurementBuilder builder;

void on_packet(const depz::Packet& pkt) {
    if (pkt.cmd != static_cast<std::uint8_t>(tmf::Tmf8829Rpt::Frame)) return;
    auto chunk = tmf::FrameChunk::unpack(depz::as_bytes(pkt.payload));
    if (!chunk) return;
    auto frame = reasm.feed(*chunk);                  // nullopt until complete
    if (!frame) return;
    auto m = builder.add(depz::as_bytes(frame->data),
                         static_cast<std::int64_t>(frame->timestamp_us),
                         static_cast<std::int64_t>(frame->edge_us));
    if (!m) return;                                   // a histogram, ref or first half
    auto [rows, cols] = m->shape();
    std::printf("%ux%u at %lld us, %zu histogram frames, dropped so far %llu\n", rows, cols,
                static_cast<long long>(m->timestamp_us), m->histograms.size(),
                static_cast<unsigned long long>(reasm.seq_gaps));
}
```

Unlike the live class, the builder keeps the histogram and ref-SPAD frames
themselves in `m->histograms` / `m->ref_frames`.

### One frame, one clock

For a single frame, or your own grouping:

```cpp
#include "depz/tmf8829.hpp"

namespace tmf = depz::tmf8829;

tmf::SensorClock sensor_clock;     // reset() both at every stream start
tmf::CaptureTimer timer;

// Every frame in stream order, with the chunk's timestamp_us and edge_us.
void frame(depz::byte_span raw, std::int64_t ts_us, std::int64_t edge_us) {
    tmf::FrameInfo f = tmf::parse_frame(raw);        // FrameError if malformed
    if (f.kind() == tmf::FID_REF_SPAD) return;        // no clock pair in it
    std::uint8_t flags = sensor_clock.add(ts_us, f.systick);
    if (f.kind() == tmf::FID_HISTOGRAMS) { timer.histograms(); return; }
    tmf::FrameTime t = timer.results_frame(sensor_clock, f, ts_us, edge_us, flags);
    tmf::PixelResults r = tmf::parse_results(raw, f);  // this frame's rows only
    std::vector<double> mm = tmf::distance_mm(r, sensor_clock.ratio(), f.distance_in_mm());
    (void)t; (void)mm;   // t.timestamp_us, t.flags; f.problems() for warnings
}
```

On 32×32 / 48×32 each frame holds half the rows (`frame_rows(fp_mode)`): the
first (`!f.is_second_half()`) the even rows, the second the odd rows;
`merge_halves(first, second)` interleaves them. The arithmetic matches
`contracts/vectors/tmf8829_frame_time.json` bit for bit.

## Gotchas

- **`init()` first** — the board does nothing until the host has downloaded
  the sensor firmware. After a USB re-plug, `init()` again.
- **`configure()` writes everything** — leave an option out and it goes back
  to its default, not to what you set last time. Pass the full
  `Tmf8829Options` every time.
- **Big formats can be refused** — 48×32 with 4 peaks and every strength
  exceeds the sensor's frame limit: `ProtocolError`
  (`ERR_CONFIG_RESULT_SIZE`).
- **Rows and columns are transposed against the scene** — see
  [orientation](#the-measurement); 48×32 is 32 rows × 48 columns.
- **`8x8-xr` raw distances are mm**, every other preset 0.25 mm; use
  `distance_mm`, which handles both.
- **Histogram bytes come through `on_frame()`**, not the live measurement.
- **No sleep** — EN is wired high; there is no power-mode API. To "wake"
  the sensor, `init()`.
- **Callbacks run on the reader thread** — copy out and return; never call
  `init()`, `configure()` or `stop()` from inside one.
