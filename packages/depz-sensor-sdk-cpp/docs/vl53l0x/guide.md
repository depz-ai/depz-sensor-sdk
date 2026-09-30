# VL53L0X — user guide

Hands-on guide to the VL53L0X in C++: the live class `depz::Vl53lx`
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
board's device name (`… VL53L0X USB v2.1 …`). `init()` binds a (product, driver)
pair and initialises the sensor: by default the detected product and its
first driver kind — `uld` here.

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

The model id (`0x00EE`) is a cross-check only: several products share
theirs.

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
    tof->configure(33, 0, std::string("long-range"));
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
    tof.configure(33, 0, std::string("long-range"));           // re-init + apply
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

## What each driver supports

| driver | capability groups |
|---|---|
| `uld` | timing, modes, offset, offset calibration, crosstalk, crosstalk calibration, reference-SPAD management |

Ask `supports(depz::Vl53lxCap::...)` at run time; a call outside the list
throws `ArgumentError` naming the missing group.

## Settings and calibration

```cpp
#include <cstdio>
#include "depz/device.hpp"

void calibrate(depz::Vl53lx& tof, int target_mm)
{
    const auto offset = tof.calibrate_offset(target_mm);  // flat target, known distance
    const auto xtalk = tof.calibrate_xtalk(target_mm);
    std::printf("offset %d mm, crosstalk %d kcps\n", static_cast<int>(offset), static_cast<int>(xtalk));
    tof.configure(50, 0, std::nullopt, offset, xtalk);  // re-apply after every reset
}
```

The values live in sensor RAM: store them on the host and pass them to every
`configure()`.

`perform_ref_spad_management()` re-measures the reference SPADs →
(count, is_aperture).

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

- `vl53l0x_uld_long-range_33ms` — `device_vlx_l0x_uld_replay`

## Verification status

Replays only: no VL53L0X board was at hand, from C or C++. The driver is a
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

For this board: `VL53L0X`, model id `0x00EE`, 2000 mm, driver `uld` (ST's
VL53L0X API 1.0.4), **1-byte addresses**, two release steps `0x0B ← 0x01`,
`0x0B ← 0x00`, and a **400 kHz** bus ceiling. `resolve_class()` gives
`SensorClass::Vl53l0x`.

### Switch the bridge to 1-byte addresses

The VL53L0X is the only family member with 8-bit register addresses. The
width is a bridge setting: sticky, back to 2 after a reset, and it must be
set **before the first register access of a session** (the model-id read
included) and never while a stream runs:

```cpp
#include "depz/vl53lx.hpp"

namespace lx = depz::vl53lx;

void use_1byte_addresses()
{
    auto op = [](lx::Vl53lxCmd c) { return static_cast<std::uint8_t>(c); };
    send_command(op(lx::Vl53lxCmd::SetAddrWidth), *lx::pack_set_addr_width(1));  // nullopt unless 1 or 2

    // now read the model id at 0xC0 (expect 0xEE): at width 1 only the low
    // address byte goes on the bus and addr + len must stay <= 0x100
    send_command(op(lx::Vl53lxCmd::ReadReg), lx::pack_read_reg(0xC0, 1));
}
```

The reply is `Vl53lxRpt::RegData` (0x91), decoded with `RegData::unpack`.

### Stream the result block

The VL53L0X driver streams the 12-byte block at `0x14` and releases the
interrupt with two writes to `0x0B` — both come from the product row:

```cpp
#include "depz/vl53lx.hpp"

namespace lx = depz::vl53lx;

void start_l0x_stream() {
    const lx::Product* p = lx::find_product("VL53L0X");
    if (auto payload = lx::pack_start_stream(lx::L0X_BLOCK_ADDR, lx::L0X_BLOCK_LEN,
                                             p->clear_steps))       // 6 + 3 x 2 = 12 B
        send_command(static_cast<std::uint8_t>(lx::Vl53lxCmd::StartStream), *payload);
}
```

### Decode the result block

`decode_l0x_raw()` returns the raw fields of the block — the distance, the
device's range-status byte, the signal and ambient rates as 16.16 fixed point
(Mcps) and the effective SPAD count as 8.8 fixed point:

```cpp
#include <cstdio>
#include "depz/vl53lx.hpp"

namespace lx = depz::vl53lx;

void on_stream(const depz::Packet& pkt) {
    if (pkt.cmd != static_cast<std::uint8_t>(lx::Vl53lxRpt::Stream)) return;
    auto s = lx::StreamData::unpack(depz::as_bytes(pkt.payload));
    if (!s || s->addr != lx::L0X_BLOCK_ADDR) return;
    auto r = lx::decode_l0x_raw(depz::as_bytes(s->data));
    if (!r) return;
    std::printf("t=%llu us  %u mm  device status 0x%02X  signal %.3f Mcps  "
                "ambient %.3f Mcps  %.2f SPADs\n",
                static_cast<unsigned long long>(s->timestamp_us), unsigned(r->distance_raw),
                unsigned(r->device_range_status), r->signal_rate_mcps_1616 / 65536.0,
                r->ambient_rate_mcps_1616 / 65536.0, r->effective_spad_count_88 / 256.0);
}
```

`distance_raw` is millimetres (quarter-millimetres if the driver enabled
fractional ranging). The **range status, sigma and dmax** of the ST API need
device data the driver caches at init (SPAD and reference calibration), so
they are the class's — the raw `device_range_status` byte is what the
sensor wrote, not the final status.

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
- **No thresholds, ROI or temperature update** on this part — `supports()` says so; the calls return `DEPZ_E_ARG`.
- **A NACK or two right after a reset is normal** — the die is still booting; the driver waits them out, and the class clears the bridge's I2C error counter once init is through.
- **Stay at 400 kHz** — this die has no Fast Mode Plus; the driver never raises the bus.
- **Uncalibrated it reads long** — the Python SDK measured about +3 cm on the lab board; calibrate the offset against a flat target and re-apply it with `configure()`.
- **Callbacks run on the reader thread** — keep them short, never call a blocking method from one.
