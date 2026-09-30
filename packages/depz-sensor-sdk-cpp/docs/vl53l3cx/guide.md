# VL53L3CX — user guide

Hands-on guide to the VL53L3CX in C++: the live class `depz::Vl53lx`
(`depz/device.hpp`, a wrapper over the C SDK's 1D-family class) first, then
the codecs in `depz/vl53lx.hpp` for when you own the transport. For what the
sensor is and its concepts, read the [introduction](introduction.md); for
exact signatures see the [API reference](api.md).

## Contents

- [Open and initialize](#open-and-initialize)
- [Hello world: live distance](#hello-world-live-distance)
- [Configure](#configure)
- [Streaming and single shot](#streaming-and-single-shot)
- [The measurement](#the-measurement)
- [Several targets (histogram driver)](#several-targets-histogram-driver)
- [What each driver supports](#what-each-driver-supports)
- [Settings and calibration](#settings-and-calibration)
- [Reset and bridge diagnostics](#reset-and-bridge-diagnostics)
- [Record and replay for tests](#record-and-replay-for-tests)
- [Verification status](#verification-status)
- [Decode layer (your own transport)](#decode-layer-your-own-transport)
- [Gotchas](#gotchas)

## Open and initialize

`depz::open_vl53lx()` finds a 1D-family board and returns the class (or
throws `WrongTypeError`); `depz::open_device()` returns a `Vl53lx` too. Every
1D board runs the same firmware, so the product comes from the USB PID or the
board's device name (`… VL53L3CX USB v2.1 …`). `init()` binds a (product, driver)
pair and initialises the sensor: by default the detected product and its
first driver kind — `ulp` here.

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main()
{
    auto tof = depz::open_vl53lx();
    tof->init();
    std::printf("%s / %s, model id 0x%04X\n", tof->product().value_or("?").c_str(),
                depz::vl53lx::to_string(*tof->driver_kind()).c_str(), tof->model_id());
    std::printf("caveat: %s\n", tof->caveat().c_str());
}
```

The model id (`0xEAAA`) is a cross-check only: several products share
theirs.

To run the die on another product's driver, name that product — here
the VL53L1CX light driver (long / short modes, the ROI, offset and crosstalk calibration) — ST's own advice for this part:

```cpp
#include "depz/device.hpp"

void borrow_light_driver(depz::Vl53lx& tof)
{
    tof.init(depz::vl53lx::DriverKind::Uld, std::string("VL53L1CX"));
}
```

## Hello world: live distance

```cpp
#include <chrono>
#include <cstdio>
#include "depz/device.hpp"

using namespace std::chrono_literals;

int main()
{
    auto tof = depz::open_vl53lx();
    tof->init();
    tof->configure(33, 0, std::nullopt);
    auto stream = tof->measurements(64);  // subscribe before starting
    tof->start_ranging();
    for (int i = 0; i < 100; i++) {
        auto m = stream.next(1000ms);
        if (!m) break;
        if (m->plottable())
            std::printf("%5d mm  sigma %.1f mm\n", static_cast<int>(m->distance_mm), m->sigma_mm);
        else
            std::printf("status %d (%s)\n", m->status, m->status_text.c_str());
    }
    tof->stop_ranging();
}
```

## Configure

```cpp
#include <cstdio>
#include "depz/device.hpp"

void show_timing(depz::Vl53lx& tof)
{
    tof.configure(33, 0, std::nullopt);           // re-init + apply
    const auto [budget, inter] = tof.range_timing();   // read back
    const auto [lo, hi] = tof.budget_range();
    std::printf("budget %d ms, period %d ms, mode %s, budgets %d..%d, %zu choices\n", budget, inter,
                tof.mode().value_or("-").c_str(), lo, hi, tof.budget_choices().size());
    for (const auto& m : tof.modes()) std::printf("  mode %s\n", m.c_str());
}
```

`configure(budget_ms, inter_ms, mode, offset_mm, xtalk_kcps)` re-initialises
the sensor, then applies the mode, the timing and any stored calibration.
Call it before every run. `inter_ms` 0 ranges back-to-back; a larger value
is the period and must exceed the budget. `budget_choices()` is empty when
any budget in `budget_range()` will do; the budget reads back rounded.

## Streaming and single shot

```cpp
#include <chrono>
#include <cstdio>
#include "depz/device.hpp"

using namespace std::chrono_literals;

void shots(depz::Vl53lx& tof)
{
    auto one = tof.measure_once(1000ms);  // start, wait, read, stop
    auto stream = tof.measurements(64);   // bounded, drop-oldest
    auto off = tof.on_measurement([](const depz::Vl53lxMeasurement& m) {
        std::printf("%d mm\n", static_cast<int>(m.distance_mm));  // reader thread
    });
    tof.start_ranging();
    auto m = stream.next(2000ms);
    auto next = tof.get_measurement(2000ms);
    tof.stop_ranging();
    off();
    std::printf("%d %d, parse errors %llu\n", static_cast<int>(one.distance_mm), m ? 1 : 0,
                static_cast<unsigned long long>(tof.stream_parse_errors()));
    (void)next;
}
```

Configuration while ranging throws `ArgumentError` ("stop ranging first");
so does `measure_once()`.

## The measurement

`depz::Vl53lxMeasurement` has one shape for every product: `timestamp_us`
(MCU µs), `distance_mm`, `status` / `status_text`, `sigma_mm`,
`signal_kcps`, `ambient_kcps`, `spads`; `targets` (every return of a
histogram frame, else empty) and `bins` (the histogram frame, else nullopt);
the extras a driver has — `stream_count`, `signal_per_spad_kcps` /
`ambient_per_spad_kcps` (die ULDs), `dmax_mm` / `device_range_status`
(VL53L0X), `min_range_mm` / `max_range_mm` / `peak_bin` (histogram) — are
`std::optional`. `plottable()` accepts status 0, 6 (first histogram frame)
and 11 (merged target); prefer it to `status == 0`.

## Several targets (histogram driver)

```cpp
#include <chrono>
#include <cstdio>
#include "depz/device.hpp"

using namespace std::chrono_literals;

void targets(depz::Vl53lx& tof)
{
    tof.init(depz::vl53lx::DriverKind::Histogram);
    tof.configure(33, 0, std::string("medium"));
    tof.start_ranging();
    const auto m = tof.get_measurement(2000ms);
    for (const auto& t : m.targets)  // up to four
        std::printf("%d mm  status %d (%s)\n", static_cast<int>(t.distance_mm), t.status, t.status_text.c_str());
    if (auto d = m.primary_distance_mm()) std::printf("readout: %d mm\n", static_cast<int>(*d));
    if (m.bins) std::printf("%zu bins, VCSEL period %u\n", m.bins->bin_data.size(), m.bins->vcsel_period);
    tof.stop_ranging();
}
```

The presets pick how far the 24 bins reach: `short` 1.6 m, `medium` 2.4 m
(what init leaves), `long` 4 m (`driver_reach_mm()`). Use `plottable()` /
`primary_distance_mm()`, not `status == 0`. The driver keeps frame-to-frame
state, so every streamed frame is decoded exactly once, in order.

## What each driver supports

| driver | capability groups |
|---|---|
| `ulp` | timing, distance-window interrupt, signal threshold, sigma threshold, region of interest |
| `histogram` | timing, modes |

Ask `supports(depz::Vl53lxCap::...)` at run time; a call outside the list
throws `ArgumentError` naming the missing group.

## Settings and calibration

`set_detection_thresholds(low_mm, high_mm, window)` arms the distance-window
interrupt (window 0 below, 1 above, 2 out, 3 in) until the next
`configure()`; `set_signal_threshold_kcps()` / `set_sigma_threshold_mm()`
tune when a frame counts as valid.

`set_roi(x, y)` narrows the SPAD window (4..16 each way), `set_roi_center()`
moves it.

Neither of this part's own drivers has offset or crosstalk calibration.

## Reset and bridge diagnostics

```cpp
#include <cstdio>
#include "depz/device.hpp"

void diagnostics(depz::Vl53lx& tof)
{
    const auto info = tof.bridge_info();  // safe while streaming
    std::printf("%u edges, %u dropped, %u I2C errors, %u kHz\n", info.int_edges, info.frames_dropped,
                info.i2c_errors, static_cast<unsigned>(info.i2c_khz));
    tof.xshut(depz::vl53lx::XSHUT_RESET);  // power-cycle: init() again
}
```

A power-cycled sensor holds none of the configuration or calibration. The
class clears the bridge's I2C error counter at the end of every init
(firmware `APP_VL53L0_4_v0.24` or newer). `read_reg()` / `write_reg()` are
the raw escape hatch.

## Record and replay for tests

```cpp
#include <string>
#include "depz/device.hpp"

void replay(const std::string& path)
{
    auto plain = depz::Device::open(depz::Link::replay(path, /*strict_tx=*/true));
    auto dev = depz::Device::promote(std::move(plain));
    if (auto* tof = dynamic_cast<depz::Vl53lx*>(dev.get())) tof->init();
}
```

`tests/test_device.cpp` replays these committed captures of this board
(recorded by the Python SDK) strictly through `depz::Vl53lx`:

- `vl53l3cx_ulp_33ms` — `device_vlx_l3cx_ulp_replay`

## Verification status

Replays only: no VL53L3CX board was at hand, from C or C++. The driver is a
port of the Python SDK's, which was verified on a live board.

## Decode layer (your own transport)

`depz/vl53lx.hpp` holds the codecs the class is built on — pure functions,
no I/O — for when you run the DEPZ transport yourself.

### The codec surface

| direction | `Vl53lxCmd` / `Vl53lxRpt` | what | codec |
|---|---|---|---|
| host → device | `ReadReg` / `WriteReg` (0x32 / 0x33) | registers | `pack_read_reg` / `pack_write_reg` |
| host → device | `Xshut` (0x34) | XSHUT pin | `pack_xshut` (`XSHUT_OFF` / `_ON` / `_RESET`) |
| host → device | `StartStream` (0x35) | start streaming | `pack_start_stream` |
| host → device | `StopStream` / `GetInfo` (0x36 / 0x37) | stop / bridge info | empty payload |
| host → device | `SetI2cSpeed` (0x38) | bus speed | `pack_set_i2c_speed` |
| host → device | `SetAddrWidth` (0x39) | register-address width | `pack_set_addr_width` |
| device → host | `RegData` (0x91) | register data | `RegData::unpack` |
| device → host | `Info` (0x92) | bridge info (23 B) | `Vl53lxInfo::unpack` |
| device → host | `Stream` (0x93) | streamed block | `StreamData::unpack` → a block decoder |

The register, XSHUT and speed encoders, `RegData` and `StreamData` are the
VL53L4CD codecs, re-exported into `depz::vl53lx` unchanged.

### Send a command

The snippets call one small helper that frames a payload and hands it to your
transport:

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

### Which product is this

Every 1D board runs the same firmware, so the product comes from the board:
the production USB PID, else the first `VL53L<digit>…` in the device name.
The product row then carries everything the bridge needs:

```cpp
#include <cstdio>
#include <string>
#include "depz/usb_ids.hpp"
#include "depz/vl53lx.hpp"

namespace lx = depz::vl53lx;

// vid/pid from your port enumeration; device_name from GET_DEVICE_NAME
const lx::Product* product_of(std::uint16_t vid, std::uint16_t pid,
                              const std::string& device_name) {
    if (vid == depz::DEPZ_USB_VID)
        for (const lx::Product& p : lx::products())
            if (p.usb_pid == pid) return &p;
    if (auto name = lx::product_from_board_name(device_name))
        return lx::find_product(*name);
    return nullptr;                      // unknown / unstamped board
}

void print_product(const lx::Product& p) {
    std::printf("%s: model id 0x%04X, %u mm, default driver %s, "
                "%u-byte addresses, bus up to %u kHz\n",
                p.name.c_str(), unsigned(p.model_id), unsigned(p.reach_mm),
                lx::to_string(p.default_driver).c_str(), unsigned(p.addr_width),
                unsigned(p.max_khz));
    for (lx::DriverKind k : p.driver_kinds)
        std::printf("  runs %s\n", lx::to_string(k).c_str());
}
```

For this board: `VL53L3CX`, model id `0xEAAA`, 3000 mm, drivers `ulp`
(default) and `histogram`, 2-byte addresses, one release step
`0x0086 ← 0x01`, 1000 kHz after init. `resolve_class()` gives
`SensorClass::Vl53l3cx`.

### Stream a block

`VL53_START_STREAM` names the block to read on every data-ready edge and the
interrupt-release writes the bridge plays after each read (up to
`CLEAR_STEPS_MAX`). Take them from the product row:

```cpp
#include "depz/vl53lx.hpp"

namespace lx = depz::vl53lx;

// after the driver has initialised and started the sensor
void start_block_stream(const lx::Product& p, std::uint16_t addr, std::uint16_t len) {
    if (auto payload = lx::pack_start_stream(addr, len, p.clear_steps, /*flags=*/0))
        send_command(static_cast<std::uint8_t>(lx::Vl53lxCmd::StartStream), *payload);
}
```

Call it with `DIE_BLOCK_ADDR` / `DIE_BLOCK_LEN` for the ULP driver or
`HISTOGRAM_BLOCK_ADDR` / `HISTOGRAM_BLOCK_LEN` for the histogram driver (both
release with `0x0086 ← 0x01`). `pack_start_stream` returns `nullopt` for more
than four release steps. `flags` bit `SF_INT_ACT_HIGH` selects an active-high
INT. Stop with `Vl53lxCmd::StopStream` and an empty payload.

### Decode the die block (ULP driver)

The Ultra Low Power driver streams the 17-byte block at `0x0089` and reads
it the way the VL53L4CD ULD does — pass `DieVariant::L4`:

```cpp
#include <cstdio>
#include "depz/vl53lx.hpp"

namespace lx = depz::vl53lx;

void on_stream(const depz::Packet& pkt) {
    if (pkt.cmd != static_cast<std::uint8_t>(lx::Vl53lxRpt::Stream)) return;
    auto s = lx::StreamData::unpack(depz::as_bytes(pkt.payload));
    if (!s || s->addr != lx::DIE_BLOCK_ADDR) return;
    auto r = lx::decode_die_block(depz::as_bytes(s->data), lx::DieVariant::L4);
    if (!r) return;
    if (r->range_status == 0)
        std::printf("t=%llu us  %d mm  sigma %d mm  signal %d kcps  frame %d\n",
                    static_cast<unsigned long long>(s->timestamp_us), r->distance_mm,
                    r->sigma_mm, r->signal_rate_kcps, r->stream_count);
    else
        std::printf("status %d\n", r->range_status);
}
```

`DieResult` is the VL53L4CD result struct: `range_status` through ST's status
table (0 = valid), `distance_mm`, `sigma_mm`, signal and ambient rates in
kcps, the per-SPAD rates, `number_of_spad` and the sensor's own frame counter
`stream_count` (wraps at 255). The `L4` variant is exactly
`vl53l4::parse_result_block()`.
`timestamp_us` is MCU uptime at the INT edge.

### Decode the histogram block

The histogram driver streams the 83-byte block at `0x0088`:

```cpp
#include <cstdio>
#include "depz/vl53lx.hpp"

namespace lx = depz::vl53lx;

// s: a StreamData whose addr == HISTOGRAM_BLOCK_ADDR
void print_bins(const lx::StreamData& s) {
    auto h = lx::decode_histogram_raw(depz::as_bytes(s.data));
    if (!h) return;
    std::printf("frame %u  range status 0x%02X  %u SPADs  vcsel start %u\n",
                unsigned(h->stream_count), unsigned(h->range_status),
                unsigned(h->dss_actual_effective_spads), unsigned(h->vcsel_start));
    for (std::uint32_t bin : h->bins) std::printf("%u ", unsigned(bin));
    std::printf("\n");
}
```

The decoder hands you the device's status bytes, the reference phase, the
VCSEL start and the 24 photon counts (24-bit each). Bin 23's low byte comes
from a separate MSB/LSB pair; the decoder rebuilds it before reading the
bins. **Finding targets in the bins** — the preset, the VCSEL period, the A/B
frame pairs, the phase-consistency history — is the class's job (the sections above).

### Valid or not

- **Die block** — `range_status == 0` is a valid distance; other values name
  why the measurement is suspect.
- **Histogram driver** — the family-wide rule the class applies to the
  targets they find: statuses **0, 6 and 11 are all plottable** (6 = first
  frame, no predecessor for the wrap check; 11 = merged pulse). Testing
  `status == 0` alone throws good frames away. The raw `range_status` byte in
  `HistogramRaw` is the device's own, not that status.
- **Decode each histogram frame exactly once, in order** — the target search
  steps an A/B frame-pair state per frame. `decode_histogram_raw()` itself is
  stateless, but anything you build on the bins must see every frame once, in
  stream order.

### Bridge diagnostics

```cpp
#include <cstdio>
#include "depz/vl53lx.hpp"

namespace lx = depz::vl53lx;

// the answer to Vl53lxCmd::GetInfo (empty payload); safe while streaming
void on_info(const depz::Packet& pkt) {
    if (pkt.cmd != static_cast<std::uint8_t>(lx::Vl53lxRpt::Info)) return;
    if (auto i = lx::Vl53lxInfo::unpack(depz::as_bytes(pkt.payload)))
        std::printf("%u edges, %u slots skipped, %u frames dropped, %u I2C errors, "
                    "%u kHz, width %u, %u release steps\n",
                    unsigned(i->int_edges), unsigned(i->slots_skipped),
                    unsigned(i->frames_dropped), unsigned(i->i2c_errors),
                    unsigned(i->i2c_khz), unsigned(i->addr_width), unsigned(i->n_clear));
}
```

`slots_skipped` (a read slot that never got the bus), `frames_dropped` (a good
sample the USB link had no room for) and the fault latch reset at
`START_STREAM`; `i2c_errors` runs free and wraps — watch increments.

## Gotchas

- **`configure()` before every run** — it re-initialises the sensor.
- **Configuration while ranging throws** — stop, reconfigure, restart.
- **Prefer `plottable()` over `status == 0`**.
- **No calibrations on either of its own drivers** — `supports(depz::Vl53lxCap::CalibOffset)` is false; borrow the VL53L1CX light driver if you need them.
- **The ULP is single-target** — the histogram driver is the one that finds several.
- **Callbacks run on the reader thread** — keep them short, never call a blocking method from one.
