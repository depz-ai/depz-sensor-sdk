# VL53L4CD — user guide

Hands-on guide to the VL53L4CD in C++: the live class `depz::Vl53l4cd`
(`depz/device.hpp`) first, then the codecs and ULD math of `depz/vl53l4.hpp`
underneath it. For what the sensor is and its concepts, read the
[introduction](introduction.md); for exact signatures see the
[API reference](api.md). The rules every live device shares — the build and
the C SDK dependency, errors, threads and callbacks — are in the
[common guide](../guide.md#live-hardware).

## Contents

- [Open and initialise](#open-and-initialise)
- [Configure](#configure)
- [The detection window](#the-detection-window)
- [One measurement](#one-measurement)
- [Ranging](#ranging)
- [Callbacks vs streams](#callbacks-vs-streams)
- [The measurement](#the-measurement)
- [Calibration](#calibration)
- [XSHUT: power and reset](#xshut-power-and-reset)
- [Bridge diagnostics and raw registers](#bridge-diagnostics-and-raw-registers)
- [Device time, disconnects](#device-time-disconnects)
- [Record and replay in tests](#record-and-replay-in-tests)
- [Decode layer](#decode-layer)
- [Gotchas](#gotchas)

## Open and initialise

`open_vl53l4cd()` finds the DEPZ boards by USB id, orders them by USB serial,
probes the chosen one and returns a `std::unique_ptr<depz::Vl53l4cd>` — or
throws `NoDeviceError` (nothing found) or `WrongTypeError` (the board is not a
VL53L4CD). Then `init()` brings the sensor up:

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main() {
    try {
        auto tof = depz::open_vl53l4cd();                 // the first VL53L4CD by USB serial
        std::printf("%s on %s, sensor %s\n", tof->software_name().c_str(),
                    tof->port().c_str(), tof->is_alive() ? "answers" : "silent");
        tof->init();                                      // config block + VHV, ~60 ms
        std::printf("initialised: %d\n", tof->initialized());
    } catch (const depz::NoDeviceError&) {
        std::puts("no DEPZ device plugged in");
    } catch (const depz::WrongTypeError& e) {
        std::printf("%s\n", e.what());
    }
}   // the unique_ptr closes the port
```

`init()` is the ULD's `sensor_init`: it waits for the sensor to boot, writes
the 91-byte default configuration at 400 kHz (the only speed an unconfigured
sensor is specified for), re-times the bus to `bus_khz` (default 1000; the
bridge picks the nominal step nearest to it), runs the VHV calibration and
leaves a 50 ms budget in continuous mode. There is no firmware download — the
VL53L4CD carries its own — so it takes ~60 ms on a real board.
`is_alive()` reads the model id register and is `true` for `0xEBAA`.

Pick a board with `OpenOptions` — a port, a USB serial or an index:

```cpp
#include "depz/device.hpp"

std::unique_ptr<depz::Vl53l4cd> open_on(const std::string& port) {
    depz::OpenOptions opt;
    opt.port = port;                                  // "/dev/ttyACM0", "COM7"
    opt.timeout = std::chrono::milliseconds(500);     // request timeout (default 200 ms)
    auto tof = depz::open_vl53l4cd(opt);
    tof->init(400);                                   // stay at 400 kHz
    return tof;
}
```

Opened with `open_device()` instead, a VL53L4CD is the `depz::Vl53l4cd`
subclass of the returned `depz::Device` —
`dynamic_cast<depz::Vl53l4cd*>(dev.get())` finds it. `depz::Vl53l4cd::open(Link)`
opens one on a link with no probe at all, for replays and fakes (see
[below](#record-and-replay-in-tests)).

`Vl53l4cd` is a `Device`, so the common commands work on it too:
`device_name()`, `software_name()`, `serial_number()`,
`read_mcu_temperature()`, `sync_time()`, `reset()`, the events stream.

## Configure

Call these **after `init()` and while not ranging** — during ranging they
throw `ArgumentError` ("stop ranging first"). Every setter has a getter that
reads the value back from the sensor:

```cpp
#include <cstdio>
#include "depz/device.hpp"

void configure(depz::Vl53l4cd& tof) {
    // timing: budget 10..200 ms; inter 0 = continuous, > budget = autonomous
    tof.set_range_timing(33, 0);                 // back to back, ~30 Hz
    depz::vl53l4::RangeTiming t = tof.range_timing();
    std::printf("budget %u ms, inter %u ms\n", static_cast<unsigned>(t.timing_budget_ms),
                static_cast<unsigned>(t.inter_measurement_ms));

    // corrections
    tof.set_offset_mm(-7);                       // signed, added to every distance
    tof.set_xtalk_kcps(0);                       // crosstalk compensation, 0 = off

    // quality limits: a measurement outside them has a non-zero range_status
    tof.set_signal_threshold_kcps(1024);         // too weak a return
    tof.set_sigma_threshold_mm(15);              // too noisy a range (<= 16383)
    std::printf("offset %d mm, sigma limit %u mm\n", static_cast<int>(tof.offset_mm()),
                static_cast<unsigned>(tof.sigma_threshold_mm()));
}
```

- **Range timing** — the *budget* is how long one measurement integrates
  (longer: less noise, farther reach, fewer per second). The
  *inter-measurement period* is 0 for continuous mode (the next measurement
  starts as soon as one ends) or a value **above** the budget for autonomous
  low-power mode (`set_range_timing(200, 1000)`: one 200 ms measurement per
  second, the sensor asleep in between). A value from 1 to the budget, or a
  budget outside 10..200 ms, throws `ArgumentError`. The read-back is the
  ULD's register math and may differ by a millisecond or two from what you
  set.
- **Offset** (mm) and **crosstalk** (kcps — the light the cover glass
  reflects back, subtracted from each return) — set them by hand, or measure
  them with the [calibrations](#calibration).
- **Signal / sigma thresholds** — the sensor flags (it does not drop) a
  measurement whose return signal is below the signal limit (status 2) or
  whose sigma, its own error estimate, is above the sigma limit (status 1).
  The sigma threshold above 16383 mm throws `ArgumentError` (the register
  word would overflow).
- **Temperature** — after an ambient change of more than 8 °C, call
  `start_temperature_update()` to re-run the VHV calibration.

## The detection window

By default INT — and so every streamed measurement — fires on each new
sample. A detection window makes the sensor raise INT only when the distance
meets a condition, so a stream carries only the interesting measurements:

```cpp
#include "depz/device.hpp"

void only_between(depz::Vl53l4cd& tof) {
    // INT (and the stream) only while a target is between 100 and 300 mm
    tof.set_detection_thresholds({100, 300, depz::DetectionWindow::In});

    depz::DetectionThresholds th = tof.detection_thresholds();
    // th.low_mm == 100, th.high_mm == 300, th.window == DetectionWindow::In
    (void)th;
}
```

The windows: `Below` (distance < `low_mm`), `Above` (> `high_mm`), `Out`
(outside `low_mm`..`high_mm`) and `In` (inside it). The window also gates
`measure_once()` and the calibrations, which wait for data-ready.

There is **no "off" value**: once a window is programmed, only `init()` puts
the sensor back to "INT on every sample". Right after `init()` the getter
reads `{0, 0, Below}` — but writing that same `{0, 0, Below}` is not the
same: it asks for "distance below 0 mm", which never happens, and INT stays
silent (single shots time out, the stream is empty) until the next `init()`.

## One measurement

```cpp
#include <cstdio>
#include "depz/device.hpp"

void once(depz::Vl53l4cd& tof) {
    depz::Vl53l4cdMeasurement m = tof.measure_once();   // waits up to 1 s
    if (!m.valid()) {                                    // r.range_status != 0
        std::printf("status %d: %s\n", m.r.range_status, m.status_text().c_str());
        return;
    }
    std::printf("%d mm, sigma %d mm, signal %d kcps (t=%llu us)\n", m.r.distance_mm,
                m.r.sigma_mm, m.r.signal_rate_kcps,
                static_cast<unsigned long long>(m.timestamp_us));
}
```

`measure_once()` is the ULD's poll mode: start the sensor, poll data-ready,
read the result block, clear the interrupt, stop. The `timeout` argument
(default 1000 ms) bounds the wait for data-ready, then `TimeoutError`; the
sensor is stopped either way. The result goes to the caller only, not to
callbacks or streams. While ranging it throws `ArgumentError`.

**A non-zero status is not an error**: it is a measurement with a reason —
see [the measurement](#the-measurement). On a real VL53L4CD facing a wall
about a metre away this prints ~1042 mm, valid.

## Ranging

`start_ranging()` starts the sensor and arms the MCU's stream: on every INT
edge the MCU reads the 17-byte result block and pushes it to the host, where
it goes to the `on_measurement()` callbacks and the `measurements()` streams.
`stop_ranging()` ends it and is idempotent. `ranging()` tells the state:

```cpp
#include <cstdio>
#include "depz/device.hpp"

void range_for_a_second(depz::Vl53l4cd& tof) {
    tof.set_range_timing(33, 0);
    auto frames = tof.measurements();         // subscribe before start: nothing is missed
    tof.start_ranging();
    try {
        tof.set_offset_mm(0);
    } catch (const depz::ArgumentError& e) {
        std::printf("refused while ranging: %s\n", e.what());
    }
    int n = 0, valid = 0;
    long sum = 0;
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < until) {
        auto m = frames.next(std::chrono::milliseconds(200));
        if (!m) continue;                     // timeout (or the device closed)
        n++;
        if (m->valid()) { valid++; sum += m->r.distance_mm; }
    }
    tof.stop_ranging();
    std::printf("%d frames, %d valid, mean %ld mm, %llu dropped, %llu undecoded\n", n, valid,
                valid ? sum / valid : 0L, static_cast<unsigned long long>(frames.dropped_count()),
                static_cast<unsigned long long>(tof.stream_parse_errors()));
}
```

At a 33 ms budget a real VL53L4CD delivers 33 frames/s (mean 1047 mm to a
wall at ~1.05 m, none dropped). `stream_parse_errors()` counts stream reports
whose block did not decode — 0 in a healthy session.

## Callbacks vs streams

Both receive the same streamed measurements. Use either, or both at once.

- **`measurements(maxsize)`** — a bounded queue (default 64) that your thread
  pulls with `next(timeout)`. When it is full the **oldest** measurement goes
  and `dropped_count()` grows. It is the easy choice: your code runs on your
  thread, where it may block, log or call the device.
- **`on_measurement(fn)`** — `fn` runs **on the device's reader thread** for
  every measurement. Keep it short: never block, never call a method that
  waits for a reply (every configuration getter and setter,
  `measure_once()`, `bridge_info()`, …), never close or destroy the device
  from it. An exception thrown from it is swallowed. It returns an
  `Unsubscribe` function; call it while the device is open.
- **`get_measurement(timeout)`** — the next streamed measurement, for "just
  give me one now" while ranging; `TimeoutError` when none comes. Each call
  subscribes afresh, so measurements that arrive between two calls are not
  kept — use `measurements()` to see every frame.

```cpp
#include <atomic>
#include <cstdio>
#include <memory>
#include <thread>
#include "depz/device.hpp"

void count_with_a_callback(depz::Vl53l4cd& tof) {
    // Shared, so the callback's copy stays valid however late its last call runs.
    auto near = std::make_shared<std::atomic<int>>(0);
    auto unsubscribe = tof.on_measurement([near](const depz::Vl53l4cdMeasurement& m) {
        if (m.valid() && m.r.distance_mm < 500) (*near)++;   // quick, non-blocking
    });
    tof.start_ranging();
    depz::Vl53l4cdMeasurement latest = tof.get_measurement();  // waits up to 2 s
    std::this_thread::sleep_for(std::chrono::seconds(1));
    tof.stop_ranging();
    unsubscribe();
    std::printf("latest %d mm; %d frames under 0.5 m\n", latest.r.distance_mm, near->load());
}
```

A callback may still be running right after `unsubscribe()` returns, and the
SDK keeps the callback object itself until the device is destroyed — so
capture by value (or a `shared_ptr`) rather than a reference to a local.

## The measurement

`Vl53l4cdMeasurement` is a timestamp plus the decoded result block
(`depz::vl53l4::Vl53l4Result`, the ULD's `VL53L4CD_ResultsData_t` plus the
sensor's frame counter):

| field | meaning |
|---|---|
| `timestamp_us` | MCU µs at the INT edge (stream) / at the read (single shot) |
| `r.range_status` | 0 = valid; `valid()`, `status_text()` |
| `r.distance_mm` | measured distance, mm (offset applied) |
| `r.sigma_mm` | the sensor's estimate of the range error, mm |
| `r.signal_rate_kcps` / `r.signal_per_spad_kcps` | return-signal rate |
| `r.ambient_rate_kcps` / `r.ambient_per_spad_kcps` | ambient-light rate |
| `r.number_of_spad` | SPADs (light-sensing cells) used |
| `r.stream_count` | the sensor's frame counter, wraps at 255 |

The range statuses (`status_text()`, from ST's UM2931): 0 valid, 1 sigma
above threshold, 2 signal below threshold, 3 distance below detection
threshold, 4 phase out of valid limit, 5 hardware fail, 6 no wrap-around check
done, 7 wrapped target, 8 processing fail, 9 crosstalk signal fail,
10 interrupt error, 11 merged target, 12 signal too low, 255 other error.
Check `valid()` before trusting `distance_mm`; UM2931 ranks 1, 2 and 6 as
warnings — a distance is there, but less certain.

## Calibration

Both calibrations range against a target at a known distance, program the
result into the sensor and return it. They block for the sample burst
(10 warm-up measurements, then `nb_samples`, default 20) and are refused
while ranging:

```cpp
#include <cstdio>
#include "depz/device.hpp"

void calibrate(depz::Vl53l4cd& tof) {
    // a flat target at exactly 100 mm (10..1000 mm allowed)
    std::int32_t offset = tof.calibrate_offset(100);
    // then crosstalk: a target at 600 mm (10..5000 mm), behind the cover glass
    std::uint16_t xtalk = tof.calibrate_xtalk(600, 40);
    std::printf("offset %d mm, xtalk %u kcps\n", static_cast<int>(offset),
                static_cast<unsigned>(xtalk));
}
```

`nb_samples` must be 5..255 and the target in range, else `ArgumentError`;
a crosstalk run with no valid samples, or one above 127 kcps, throws
`ProtocolError`. The programmed values do **not** survive an XSHUT
power-cycle: keep them and re-apply them with `set_offset_mm()` /
`set_xtalk_kcps()` after `init()`.

## XSHUT: power and reset

The bridge drives the sensor's XSHUT (shutdown) pin:

```cpp
#include "depz/device.hpp"

void power_cycle(depz::Vl53l4cd& tof) {
    tof.reset_sensor();                        // = xshut(XSHUT_RESET): off, on, wait for boot
    tof.xshut(depz::vl53l4::XSHUT_OFF);        // sensor off: stream stops, config lost
    tof.xshut(depz::vl53l4::XSHUT_ON);         // back on, in its boot state
    tof.init();                                // required before ranging again
}
```

`XSHUT_OFF` and `XSHUT_RESET` stop any stream. A power-cycled sensor holds
**none** of the ULD configuration: `initialized()` and `ranging()` go
`false`, and timing, offset, crosstalk, thresholds and the detection window
are back at the sensor's defaults.

## Bridge diagnostics and raw registers

`bridge_info()` returns the MCU's own view of the sensor
(`depz::vl53l4::Vl53l4Info`) and is safe to call while ranging.
`set_i2c_speed_khz()` re-times the bus to the nominal step nearest to the
value (not while ranging). `read_reg()` / `write_reg()` reach any register —
16-bit address, contents big-endian as the sensor has them, split at the
bridge's 253-byte transfer limit:

```cpp
#include <cstdio>
#include "depz/device.hpp"

void diagnostics(depz::Vl53l4cd& tof) {
    depz::vl53l4::Vl53l4Info info = tof.bridge_info();
    std::printf("model 0x%04X, %u kHz, %u INT edges, %u slots skipped, %u I2C errors\n",
                static_cast<unsigned>(info.model_id), static_cast<unsigned>(info.i2c_khz),
                static_cast<unsigned>(info.int_edges), static_cast<unsigned>(info.slots_skipped),
                static_cast<unsigned>(info.i2c_errors));

    depz::bytes id = tof.read_reg(0x010F, 2);            // IDENTIFICATION__MODEL_ID
    std::printf("model id bytes %02X %02X\n", std::to_integer<unsigned>(id[0]),
                std::to_integer<unsigned>(id[1]));       // EB AA, big-endian
}
```

The counters are free-running and wrap silently — watch increments, not
absolute values. `int_edges` grows while ranging; `slots_skipped` counts INT
edges the MCU could not serve (the host sees a gap in `stream_count`);
`last_i2c_error` names the latest bus error (1 NACK, 2 timeout, 3 bus error).
A raw `write_reg()` goes around the class: the ULD state the SDK tracks
(`initialized()`, the configuration) does not know about it.

## Device time, disconnects

These work exactly as for every device — see the
[SR04 guide](../sr04/guide.md#device-time--host-time) and the
[common guide](../guide.md#live-hardware):

- `timestamp_us` is MCU time; `sync_time()` once, then
  `to_host_time_us(m.timestamp_us)` puts it on `depz::host_now_us()`'s clock.
- When the board is unplugged, the device emits a `Disconnected` event, every
  `measurements()` stream hands out what it holds and then reports
  `closed()`, and every call throws `DeviceLostError`. There is no automatic
  reconnect: open the board again, and `init()` it — the sensor lost power
  with the board.

## Record and replay in tests

Record a live session once into a `.depzrec` capture, then replay it — in CI,
with no sensor attached. Replay is causal (each recorded reply is served after
the request that preceded it), so the test must make the same calls in the
same order; with `strict_tx` any byte your code sends differently throws
`ReplayMismatchError`. The ULD is many register reads and writes, so a
recording of `init()` alone is dozens of exchanges — all replayed the same
way.

```cpp
#include "depz/device.hpp"

void record(const std::string& port) {
    auto link = depz::Link::recording(depz::Link::serial(port), "l4.depzrec",
                                      "\"port\": \"live\"");
    auto tof = depz::Vl53l4cd::open(std::move(link));
    tof->init();
    tof->set_range_timing(33, 0);
    tof->measure_once();
}

bool replay_matches() {
    auto tof = depz::Vl53l4cd::open(depz::Link::replay("l4.depzrec", /*strict_tx=*/true));
    tof->init();
    tof->set_range_timing(33, 0);
    return tof->measure_once().valid();
}
```

`Vl53l4cd::open(Link)` does not probe the board, so it records nothing extra;
with `Device::promote(Device::open(link))` the identity probe becomes part of
the capture (and of the replay). The SDK's own tests replay a real VL53L4CD
session this way (`contracts/vectors/recordings/vl53l4cd_session.depzrec`:
identity, bridge info, `init()`, timing, single shots and a burst of streamed
frames — the Python, TypeScript, C and C++ SDKs all replay it strictly), and
run against the C SDK's fake bridge (a register map behind
`Link::loopback_pair()`).

## Decode layer

The `depz::vl53l4` codecs and ULD math, for bytes you read yourself — your
own port loop (the [common guide](../guide.md#mental-model) shows the
pattern), a capture, or a plain `depz::Device` with
[`request` / `send`](../guide.md#escape-hatch-raw-requests). They describe
the same wire format the live class speaks (through the C SDK), and match the
other language SDKs byte for byte via the golden vectors.

### The codec surface

- `Vl53l4Cmd` — host→device opcodes (`ReadReg`, `WriteReg`, `Xshut`,
  `StartStream`, `StopStream`, `GetInfo`, `SetI2cSpeed`), with the
  `pack_read_reg` / `pack_write_reg` / `pack_xshut` / `pack_start_stream` /
  `pack_set_i2c_speed` payload encoders.
- `Vl53l4Rpt` — device→host report ids (`RegData`, `Info`, `Stream`).
- `RegData` / `Vl53l4Info` / `StreamData` — decoded reports, each with a
  `static std::optional<…> unpack(byte_span)` (nullopt on a short payload).
- The pure ULD pieces: `parse_result_block`, the range-timing register math
  (`range_timing_registers` / `decode_range_timing`), the tuning-word codecs
  and `config_block()`.

### Read and write registers

Frame the command payloads with `build_packet` (from the
[common guide](../guide.md#transport-framing--crc)) and write them to your
port:

```cpp
#include "depz/framing.hpp"
#include "depz/vl53l4.hpp"

namespace v4 = depz::vl53l4;

// read the model id word at IDENTIFICATION__MODEL_ID (0x010F; expect 0xEBAA)
depz::bytes rd_payload = v4::pack_read_reg(0x010F, 2);
depz::bytes rd = depz::build_packet(
    static_cast<std::uint8_t>(v4::Vl53l4Cmd::ReadReg),
    depz::as_bytes(rd_payload));

// the reply is Vl53l4Rpt::RegData (0x91)
auto d = v4::RegData::unpack(depz::as_bytes(pkt.payload));
if (d && d->data.size() == 2) { /* big-endian: {0xEB, 0xAA} == v4::MODEL_ID */ }

// write the 91-byte init configuration block in one transaction
depz::bytes cfg = v4::config_block();
depz::bytes wr_payload = v4::pack_write_reg(v4::CONFIG_ADDR, depz::as_bytes(cfg));
depz::bytes wr = depz::build_packet(
    static_cast<std::uint8_t>(v4::Vl53l4Cmd::WriteReg),
    depz::as_bytes(wr_payload));
```

Reads and writes are capped at `XFER_MAX` (253) bytes per transfer — the
encoders are pure and don't police it; the firmware rejects an oversize
request. Register contents are big-endian; the wire fields (`addr`, `len`,
timestamps) are little-endian.

### Stream the result block

```cpp
// arm INT-driven streaming of the 17-byte result block
depz::bytes st_payload = v4::pack_start_stream(
    v4::RESULT_BLOCK_ADDR, v4::RESULT_BLOCK_LEN, /*flags=*/0);
depz::bytes start = depz::build_packet(
    static_cast<std::uint8_t>(v4::Vl53l4Cmd::StartStream),
    depz::as_bytes(st_payload));

// each sample arrives as Vl53l4Rpt::Stream (0x93)
auto s = v4::StreamData::unpack(depz::as_bytes(pkt.payload));
if (s) {
    auto r = v4::parse_result_block(depz::as_bytes(s->data));
    // s->addr / s->len echo the stream configuration; s->timestamp_us is the
    // MCU uptime at the INT edge
}
```

The `flags` bit `SF_INT_ACT_HIGH` (0x02) selects INT-active-high; the default
(0) matches the ULD init block (INT active low). `timestamp_us` is MCU uptime
at the **INT edge** — the sensor event, not the I2C completion. Stop with a
payload-less `Vl53l4Cmd::StopStream`. (This is what `start_ranging()` sends,
after starting the sensor.)

### Decode a result block

```cpp
auto r = v4::parse_result_block(raw17);        // nullopt when < 15 bytes
if (r && r->range_status == 0) {               // 0 = valid
    std::printf("%d mm, sigma %d mm\n", r->distance_mm, r->sigma_mm);
}
```

`Vl53l4Result` carries `range_status` (via the ULD `status_rtn` table; raw
≥ 24 passes through unmapped), `distance_mm`, `signal_rate_kcps` /
`ambient_rate_kcps` (×8), the per-SPAD rates, `number_of_spad`, `sigma_mm` and
the sensor's own `stream_count` frame counter (wraps at 255).

### Range timing math

The SetRangeTiming/GetRangeTiming register math is pure and bit-exact:

```cpp
// SetRangeTiming(50 ms budget, continuous): osc_frequency is the word read
// from register 0x0006, clock_pll the word from RESULT__OSC_CALIBRATE_VAL
// (only used in autonomous mode)
auto regs = v4::range_timing_registers(/*budget_ms=*/50, /*inter_ms=*/0,
                                       osc_frequency, clock_pll);
// regs->range_config_a (0x005E), regs->range_config_b (0x0061),
// regs->intermeasurement_raw (the INTERMEASUREMENT_MS dword at 0x006C)

// GetRangeTiming readback from the raw register reads
auto timing = v4::decode_range_timing(inter_raw, clock_pll, osc_frequency,
                                      regs->range_config_a);
// timing->timing_budget_ms, timing->inter_measurement_ms
```

Budget is 10..200 ms. `inter_ms == 0` selects continuous mode; a value
**greater** than the budget selects autonomous low power; anything else — or a
zero `osc_frequency` — returns `std::nullopt`.

### Tuning words

Each tuning register is a word codec pair (encode for the write, decode for
the readback):

```cpp
std::uint16_t word = v4::offset_raw(-10);          // RANGE_OFFSET_MM (0x001E)
int mm             = v4::decode_offset(word);      // → -10

v4::xtalk_raw(20);              // XTALK_PLANE_OFFSET_KCPS (0x0016)
v4::signal_threshold_raw(1024); // MIN_COUNT_RATE_RTN_LIMIT_MCPS (0x0066)
v4::sigma_threshold_raw(15);    // RANGE_CONFIG__SIGMA_THRESH (0x0064);
                                // optional — nullopt when sigma_mm > 16383
```

## Gotchas

- **`init()` first, and after every power-cycle.** XSHUT off / reset (and an
  unplugged board) wipe the ULD configuration; `initialized()` tracks it.
- **Configuration while ranging throws `ArgumentError`** — the INT-driven
  stream owns the register bank; so does `measure_once()`. Stop, reconfigure,
  restart.
- **Check `valid()` before trusting a distance.** A non-zero `range_status`
  is data ("signal below threshold", "wrapped target", …), not an error;
  `status_text()` names it.
- **The detection window has no "off".** Once programmed, only `init()`
  restores INT on every sample; `{0, 0, Below}` reads like the post-`init()`
  default but silences INT for good — single shots time out, the stream stays
  empty.
- **Inter-measurement must be 0 or above the budget** — 1..budget, or a
  budget outside 10..200 ms, throws `ArgumentError`.
- **Calibration results are volatile** — re-apply the offset and crosstalk
  after every `init()`.
- **Callbacks run on the reader thread** — no blocking, no configuration or
  other requests, no closing the device from one; use `measurements()` when
  in doubt.
- **Streams drop the oldest** when full — watch `dropped_count()`, and
  `bridge_info().slots_skipped` / gaps in `stream_count` for frames the MCU
  itself could not serve.
- **Timestamps are MCU µs** — at the INT edge for streamed measurements;
  convert with `to_host_time_us()` after a `sync_time()`.
- **Two endiannesses on purpose** (decode layer) — wire fields
  little-endian, register contents big-endian. The codecs handle both; don't
  swap bytes yourself.
