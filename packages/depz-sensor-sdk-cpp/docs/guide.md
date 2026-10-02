# depz-sensor-sdk-cpp — guide

The common guide to the C++ SDK for the DEPZ USB sensor line. It covers what
the SDK is, how to build it, the live-hardware layer (opening a board, errors,
threads and callbacks, record / replay), the shared mental model, and the
cross-sensor codecs (framing, CRC, identity, USB discovery ordering, time-sync
math, firmware containers, dataset replay). Each sensor then has its own
**introduction** and **user guide**:

- **SR04** (HC-SR04 ultrasonic) — [introduction](sr04/introduction.md) ·
  [guide](sr04/guide.md) · [api](sr04/api.md)
- **VL53L4CD** (single-zone ToF) — [introduction](vl53l4cd/introduction.md) ·
  [guide](vl53l4cd/guide.md) · [api](vl53l4cd/api.md)
- **VL53L8CX** (8×8 ToF base) — [introduction](vl53l8cx/introduction.md) ·
  [guide](vl53l8cx/guide.md) · [api](vl53l8cx/api.md)
- **VL53L8CH** (ToF superset + CNH histograms) —
  [introduction](vl53l8ch/introduction.md) · [guide](vl53l8ch/guide.md) ·
  [api](vl53l8ch/api.md)
- **VL53L5CX** / **VL53L7CX** / **VL53L7CH** (8×8 ToF, I2C board) —
  [L5CX](vl53l5cx/introduction.md) ([guide](vl53l5cx/guide.md) ·
  [api](vl53l5cx/api.md)) · [L7CX](vl53l7cx/introduction.md)
  ([guide](vl53l7cx/guide.md) · [api](vl53l7cx/api.md)) ·
  [L7CH](vl53l7ch/introduction.md) ([guide](vl53l7ch/guide.md) ·
  [api](vl53l7ch/api.md))
- **VL53L 1D family** (single-zone / multi-target ToF) —
  [L0X](vl53l0x/introduction.md) ([guide](vl53l0x/guide.md) ·
  [api](vl53l0x/api.md)) · [L1CX](vl53l1cx/introduction.md)
  ([guide](vl53l1cx/guide.md) · [api](vl53l1cx/api.md)) ·
  [L1CB](vl53l1cb/introduction.md) ([guide](vl53l1cb/guide.md) ·
  [api](vl53l1cb/api.md)) · [L3CX](vl53l3cx/introduction.md)
  ([guide](vl53l3cx/guide.md) · [api](vl53l3cx/api.md)) ·
  [L4CX](vl53l4cx/introduction.md) ([guide](vl53l4cx/guide.md) ·
  [api](vl53l4cx/api.md))
- **BNO086** (9-axis IMU) — [introduction](bno086/introduction.md) ·
  [guide](bno086/guide.md) · [api](bno086/api.md)
- **BNO055** (9-axis IMU, on-chip fusion) —
  [introduction](bno055/introduction.md) · [guide](bno055/guide.md) ·
  [api](bno055/api.md)

For the exhaustive symbol-by-symbol reference see [api.md](api.md), generated
from the header doc-comments so it never drifts from the code.

## Contents

- [What it is](#what-it-is)
- [Build & link](#build--link)
- [Live hardware](#live-hardware)
- [Byte types: `bytes`, `byte_span`, `span<T>`](#byte-types-bytes-byte_span-spant)
- [Mental model](#mental-model)
- [Transport: framing & CRC](#transport-framing--crc)
- [Common commands & reports](#common-commands--reports)
- [Time-sync math](#time-sync-math)
- [Discovery: USB identity](#discovery-usb-identity)
- [Firmware containers](#firmware-containers)
- [Datasets: record & replay](#datasets-record--replay)
- [What is not here](#what-is-not-here)
- [Testing without hardware](#testing-without-hardware)

## What it is

The SDK has two layers:

- **The decode layer** — pure codecs, no I/O. The SDK turns bytes into typed
  values and typed values back into bytes; where the bytes come from (a
  serial port, a recording, a socket) is up to you. It gives you
  - a byte-exact frame parser and packet builder (`depz/framing.hpp`,
    `depz/crc.hpp`), and
  - one header of wire codecs per sensor or board firmware — `depz/sr04.hpp`,
    `depz/vl53l4.hpp`, `depz/vl53l8.hpp` (CX **and** CH), `depz/vl53l7.hpp`
    (VL53L5CX / L7CX / L7CH), `depz/vl53lx.hpp` (the VL53L 1D family),
    `depz/bno086.hpp`, `depz/bno055.hpp` — plus the cross-sensor pieces
    (`depz/common.hpp`, `depz/identity.hpp`, `depz/usb_ids.hpp`,
    `depz/fwdepz.hpp`, `depz/dataset.hpp`).
- **The live-hardware layer** — `depz/device.hpp`: it opens a board and talks
  to it. Byte links (serial port, `.depzrec` record / replay, in-memory
  loopback), USB discovery with a protocol probe, a device core (a reader
  thread that matches replies to requests and routes reports, the common
  commands of every board, time sync, events) and the sensor classes. It is a
  thin C++ wrapper over the C SDK's `depz_sensor_io.h`: errors are
  exceptions, handles are move-only and close themselves, callbacks are
  `std::function`.

Every board opens as a `depz::Device` and the common commands work on all of
them. Sensor classes arrive one at a time; so far **`depz::Sr04`**,
**`depz::Vl53l4cd`**, **`depz::Vl53l8`** (the multizone ToF: VL53L8CX /
CH, VL53L5CX, VL53L7CX / CH), **`depz::Bno055`** and **`depz::Bno086`**
(BNO085 / BNO086) and **`depz::Vl53lx`** (the VL53L 1D family: VL53L0X /
L1CX / L1CB / L3CX / L4CX) — every board of the line.

Every sensor speaks one shared framed protocol (`A5 C3` header + optional
payload CRC). Everything lives in namespace `depz` (ToF under `depz::vl53l4` /
`depz::vl53l8` / `depz::vl53l7` / `depz::vl53lx`, IMUs under `depz::bno086` /
`depz::bno055`, datasets under `depz::dataset`). It targets **C++17** and is
byte-for-byte identical to the Python / TypeScript / Java reference SDKs via the
shared golden vectors in `contracts/vectors`.

The firmware philosophies mirror the firmware itself:

- **SR04** — the device does the ranging; you decode `echo_time_us` → distance.
- **VL53L4CD** — the device is a thin I2C register bridge; the ST ULD runs on
  the host — the driver in the C SDK behind `depz::Vl53l4cd`, the math and the
  single-zone result-block decode in `depz::vl53l4`.
- **VL53L8CX / VL53L8CH** — the device is a thin SPI register bridge; the ST
  ULD runs on the host — the driver in the C SDK behind `depz::Vl53l8`, the
  firmware download included — and the results frame is reassembled and
  decoded on the host (`depz::vl53l8`). CX is the base ToF imager; CH is its
  superset, adding Compact-Network-Histogram output.
- **VL53L5CX / VL53L7CX / VL53L7CH** — one I2C register-bridge firmware for
  three 8×8 ToF boards; the same ULD on the host behind `depz::Vl53l8`, the
  frames reassembled with the VL53L8 pipeline and decoded on the host
  (`depz::vl53l7`; L7CH adds CNH histograms).
- **VL53L0X / L1CX / L1CB / L3CX / L4CX** — one I2C register-bridge firmware
  for the whole 1D family; every ST driver of the family runs on the host —
  in the C SDK behind `depz::Vl53lx` — and the die, VL53L0X and histogram
  block codecs are in `depz::vl53lx`.
- **BNO085 / BNO086** — the device is an SHTP pass-through; the whole SH-2
  sensor-hub protocol runs on the host — the driver in the C SDK behind
  `depz::Bno086`, the SHTP framing and report decode in `depz::bno086`.
- **BNO055** — the device is a thin register bridge; Bosch's fusion runs on the
  chip. The register logic (mode switches through CONFIG, boot and
  fusion-start polls, page discipline, calibration) runs on the host — the
  driver in the C SDK behind `depz::Bno055` — and the register codecs and
  window decode are in `depz::bno055`.

## Build & link

```bash
cmake -B build -S .
cmake --build build
ctest --test-dir build          # golden-vector suites + the live-layer tests
```

The build produces a static library exposed as the CMake target
`depz::sensor_sdk_cpp` (the alias `depz::sensor_sdk` is kept for backwards
compatibility). The public headers are under `include/depz/`; include what
you use (`#include "depz/sr04.hpp"`, `#include "depz/device.hpp"`).

**The live layer needs the C SDK.** It is built by default
(`DEPZ_SENSOR_SDK_CPP_BUILD_IO=ON`) and links `depz::sensor_sdk_c` publicly —
your program links it too, with no extra line. CMake looks for the C library
in this order:

1. a `depz::sensor_sdk_c` target already in your build;
2. an installed package — `find_package(depz-sensor-sdk-c CONFIG)`;
3. in this repository, the sibling directory `packages/depz-sensor-sdk-c`,
   pulled in with `add_subdirectory` (its tests and examples off).

For the decode layer alone, configure with `-DDEPZ_SENSOR_SDK_CPP_BUILD_IO=OFF`:
no dependency, and no `depz/device.hpp` symbols in the library. The installed
CMake package remembers the choice and finds `depz-sensor-sdk-c` for you
when the live layer was built.

How to pull it into your project:

- **FetchContent** — fetches the whole repository, so the C SDK is found as
  the sibling directory:

  ```cmake
  include(FetchContent)
  FetchContent_Declare(depz-sensor-sdk-cpp
    GIT_REPOSITORY https://github.com/depz-ai/depz-sensor-sdk.git
    GIT_TAG        v0.4.0
    SOURCE_SUBDIR  packages/depz-sensor-sdk-cpp)
  FetchContent_MakeAvailable(depz-sensor-sdk-cpp)
  target_link_libraries(my_app PRIVATE depz::sensor_sdk_cpp)
  ```

- **add_subdirectory** — from a checkout of this repository (or a copy of
  both `packages/depz-sensor-sdk-c` and `packages/depz-sensor-sdk-cpp`, side
  by side): `add_subdirectory(path/to/packages/depz-sensor-sdk-cpp)`, then link
  `depz::sensor_sdk_cpp`.
- **find_package** — `cmake --install` of the C++ build also installs the C
  library it pulled in; then `find_package(depz-sensor-sdk-cpp CONFIG REQUIRED)`.
- **Conan / vcpkg** — the recipe and the port depend on `depz-sensor-sdk-c`;
  see the [README](../README.md#install) for the commands.

Linux, macOS and Windows are built and tested in CI.

To regenerate the API reference after editing header doc-comments:

```bash
cmake --build build --target docs      # or: python3 scripts/gen_api_md.py
```

## Live hardware

`depz/device.hpp` is the part of the SDK that talks to a board. The
[SR04 guide](sr04/guide.md), the [VL53L4CD guide](vl53l4cd/guide.md) and the
[VL53L8CX guide](vl53l8cx/guide.md) walk through a whole sensor class; this
section covers what every board shares.

### Open a device

```cpp
#include <cstdio>
#include <memory>
#include "depz/device.hpp"

int main() {
    // The DEPZ device with the smallest USB serial, probed and opened with
    // its sensor class attached.
    std::unique_ptr<depz::Device> dev = depz::open_device();

    std::printf("%s on %s\n", dev->software_name().c_str(), dev->port().c_str());
    if (auto* sr04 = dynamic_cast<depz::Sr04*>(dev.get())) {
        std::printf("%.0f mm\n", sr04->measure_once().distance_mm().value_or(0));
    } else if (auto* tof = dynamic_cast<depz::Vl53l4cd*>(dev.get())) {
        tof->init();
        std::printf("%d mm\n", tof->measure_once().r.distance_mm);
    } else if (auto* l8 = dynamic_cast<depz::Vl53l8*>(dev.get())) {
        l8->init();                               // sensor firmware download, ~0.8 s
        l8->start_ranging();
        std::printf("%d mm\n", l8->get_frame().frame.distance_mm[0]);
        l8->stop_ranging();
    } else if (auto* imu = dynamic_cast<depz::Bno055*>(dev.get())) {
        imu->configure();                         // NDOF, returns once the fusion runs
        auto q = imu->read_quaternion();
        std::printf("q = %.3f %.3f %.3f %.3f\n", q[0], q[1], q[2], q[3]);
    }
}   // the destructor stops the reader thread and closes the port
```

- `open_device(OpenOptions)` filters the serial ports to known DEPZ USB ids,
  orders them by USB serial, probes the chosen one
  (`GET_NAME_ACTIVE_SOFTWARE`) and returns the matching class.
  `OpenOptions` picks the board: `port` (exactly this port, even with an
  unknown USB id), else `serial` (the USB serial), else `index` (the Nth
  candidate); `timeout` sets the request timeout (default 200 ms).
- `dynamic_cast<depz::Sr04*>` (or `<depz::Vl53l4cd*>`, `<depz::Vl53l8*>`,
  `<depz::Bno055*>`)
  tells whether you got a sensor class; `sensor_type()` names the firmware
  family either way. A multizone board also gets its `Vl53l8Model`: on the
  VL53L8 board `0xED40` is a VL53L8CH, anything else a VL53L8CX; on the L5/L7
  board the PID (`0xED48` / `0xED49` / `0xED4A`), else the device name, else
  VL53L7CX.
- `open_sr04(OpenOptions)` / `open_vl53l4cd(OpenOptions)` /
  `open_vl53l8(OpenOptions)` / `open_bno055(OpenOptions)` are `open_device`
  plus a type check: they throw `WrongTypeError` when the board is not an
  SR04 / a VL53L4CD / a multizone ToF / a BNO055.
- `Device::open(port)` / `Device::open(Link)` open a plain device with no
  probe; `Device::promote(std::move(dev))` probes it later and returns the
  matching class. `Sr04::open(Link)` / `Vl53l4cd::open(Link)` /
  `Vl53l8::open(Link, Vl53l8Model)` / `Bno055::open(Link)` skip the probe
  for a link you know is that sensor (tests, replay).
- Listing without opening: `list_serial_ports()` (every port the OS lists,
  with its USB id), `list_depz_devices()` (the probed DEPZ boards, as
  `DeviceInfo`), `probe_port(port)` (one port; `nullopt` when nothing
  DEPZ-shaped answers).

A `Device` owns its link and its reader thread. The destructor (or `close()`)
stops the thread and closes the port; handles are move-only. The serial port
is opened exclusively, so a second program gets `IoError` instead of a share
of the byte stream.

### Common commands

Every board answers the contract-02 commands:

```cpp
#include <cstdio>
#include "depz/device.hpp"

void show(depz::Device& dev) {
    std::printf("name     %s\n", dev.device_name().c_str());
    std::printf("software %s\n", dev.software_name().c_str());
    std::printf("serial   %s\n", dev.serial_number().c_str());
    std::printf("MCU      %.1f °C\n", dev.read_mcu_temperature());   // cached ~2 Hz

    depz::TimeSync ts = dev.sync_time();          // 5 round trips, lowest RTT kept
    std::printf("offset %lld us, rtt %lld us\n",
                static_cast<long long>(ts.offset_us), static_cast<long long>(ts.rtt_us));
    std::int64_t host_us = dev.to_host_time_us(1'000'000);   // device µs → host µs
    (void)host_us;
}
```

Also there: `payload_crc_type()` / `set_payload_crc_type()` (the device→host
payload CRC), `sync_pin()` / `set_sync_pin()` (AUX sync pins), `reset()` (the
device acknowledges, reboots and the link drops), `enter_bootloader()` (the
device reboots into the bootloader and this `Device` closes), `set_timeout()`
(the request timeout) and `stats()` (packet, byte, CRC and sequence-gap
counters).

### Errors

Every failure is an exception derived from `depz::Error` (itself a
`std::runtime_error`; `code()` is the C SDK's `depz_err` value, `what()` the
detail):

| Exception | When |
|---|---|
| `ArgumentError` | a bad argument, or a call not valid in this state (a VL53L4CD / VL53L8 setting while ranging) |
| `IoError` | the OS refused — the port does not exist, is busy or not permitted |
| `DeviceLostError` | the device is closed or was unplugged |
| `TimeoutError` | no reply in time |
| `StatusError` | the device answered a non-OK status; `cmd()` is the opcode, `status()` the `depz::Status` |
| `BusyError` (a `StatusError`) | `ERR_BUSY`, or the same opcode is already in flight |
| `ProtocolError` | a reply that does not parse |
| `NoDeviceError` | discovery found no (matching) DEPZ device |
| `WrongTypeError` | a sensor call on a board of another type (`open_sr04` on a non-SR04) |
| `ReplayMismatchError` | strict replay: the host wrote bytes the recording does not have |
| `BootloaderModeError` | the device is in bootloader mode |

Catch `BusyError` before `StatusError`:

```cpp
#include <cstdio>
#include "depz/device.hpp"

void try_command(depz::Device& dev, std::uint8_t cmd) {
    try {
        dev.request_ok(cmd);
    } catch (const depz::BusyError&) {
        std::puts("busy, try later");
    } catch (const depz::StatusError& e) {
        std::printf("0x%02X refused, status %d\n", e.cmd(), static_cast<int>(e.status()));
    } catch (const depz::TimeoutError&) {
        std::puts("no answer");
    }
}
```

### Threads, callbacks and streams

Each open device runs one **reader thread**. It parses the incoming bytes,
completes the waiting request, and hands reports and events to you in two
ways:

- **Callbacks** — `on_event(fn)` (and the sensor's own, such as
  `Sr04::on_measurement(fn)`, `Vl53l4cd::on_measurement(fn)`,
  `Vl53l8::on_frame(fn)`, `Bno055::on_sample(fn)`) return a
  `Device::Unsubscribe` function. The callback runs **on the reader thread**:
  keep it short, never block in it, never call a method that waits for a reply
  (`measure_once`, `sync_time`, …: the reply is what this thread would
  deliver), never close or destroy the device from it. An exception thrown
  from a callback is swallowed — it must not unwind into the C reader. Call
  the `Unsubscribe` while the device is open; the callback may still be
  running right after it returns, so whatever it captures must live as long as
  the device.
- **Streams** — `events(maxsize)` (and e.g. `Sr04::stream(maxsize)`,
  `Vl53l4cd::measurements(maxsize)`, `Vl53l8::frames(maxsize)`,
  `Bno055::samples(maxsize)`) return a
  `Stream<T>`: a bounded queue the
  reader fills and your thread pulls with `next(timeout)`. It is registered
  when created, so nothing produced after that call is missed. When it is full
  the **oldest** item goes and `dropped_count()` grows. `next()` returns
  `std::nullopt` on timeout, and — once the device is closed and the queue
  drained — for good, with `closed()` then `true`. A stream may outlive its
  device.

Callbacks and streams can be mixed and there can be several of each; every
subscriber gets every item. Use a stream when your own thread does the work
(it is the easy way to stay off the reader thread), a callback for a quick
hand-off.

The device's methods may be called from any thread. One request per opcode is
in flight at a time: a second concurrent request with the same opcode fails
with `BusyError`.

### Events and disconnects

```cpp
#include <cstdio>
#include "depz/device.hpp"

void watch(depz::Device& dev) {
    auto events = dev.events(64);
    while (auto ev = events.next(std::chrono::milliseconds(500))) {
        switch (ev->type) {
        case depz::DeviceEvent::Type::Temperature:
            std::printf("MCU %.1f °C\n", ev->celsius);
            break;
        case depz::DeviceEvent::Type::Disconnected:
            std::printf("lost: %s\n", ev->text.c_str());
            break;
        default:
            break;   // SequenceError, CrcError, Trash, UnsolicitedStatus, Text
        }
    }
    if (events.closed()) std::puts("device closed");
}
```

`DeviceEvent::Type` is one of `SequenceError` (a gap in the packet sequence;
`by_device` when the device reported it), `CrcError`, `Trash` (bytes skipped
while hunting for a frame), `UnsolicitedStatus` (`is_hardware_fault()` for
`ERR_HARDWARE_FAULT`), `Text` (a text report — or, as hex, a report no
sensor class routes), `Temperature` and `Disconnected`.

When the board is unplugged (or the link closes) the reader emits
`Disconnected` (its `text` is the reason), every stream drains what it holds
and then reports `closed()`, `closed()` on the device turns `true`, and every
later call throws `DeviceLostError`. The `Device` object stays valid — destroy
it (or call `close()`) as usual. There is no automatic reconnect: open the
board again.

### Record and replay

A `Link` is the byte pipe under a device, and it can be taken apart:

```cpp
#include "depz/device.hpp"

// Record a live session into a .depzrec capture...
void record() {
    auto link = depz::Link::recording(depz::Link::serial("/dev/ttyACM0"),
                                      "session.depzrec", "\"port\": \"live\"");
    auto dev = depz::Device::promote(depz::Device::open(std::move(link)));
    dev->serial_number();
}

// ... and replay it later, with no hardware attached.
void replay() {
    auto link = depz::Link::replay("session.depzrec", /*strict_tx=*/true);
    auto dev = depz::Device::promote(depz::Device::open(std::move(link)));
    dev->serial_number();   // same answer as on the day it was recorded
}
```

Replay is causal: a recorded reply is served only after the host has written
the request that preceded it, so the same calls in the same order get the same
answers. With `strict_tx` any write that differs from the recording throws
`ReplayMismatchError` — a regression test that your code still sends exactly
the same bytes. `realtime` paces the replies by the recorded times.
`Link::loopback_pair()` gives two in-memory ends for a fake device in tests,
and `Link::adopt()` takes a custom C link (`depz_link_new()`).

### Escape hatch: raw requests

For a command no class wraps yet:

```cpp
#include <cstdint>
#include "depz/device.hpp"

// Send `cmd` and wait for RPT_STATUS(cmd, OK).
void raw_ok(depz::Device& dev, std::uint8_t cmd) { dev.request_ok(cmd); }

// Send `cmd` and wait for the first report `match` accepts; returns its payload.
depz::bytes raw_query(depz::Device& dev, std::uint8_t cmd, std::uint8_t reply_rpt) {
    return dev.request(cmd, {}, [reply_rpt](std::uint8_t rpt, depz::byte_span) {
        return rpt == reply_rpt;    // runs on the reader thread: decide, nothing else
    });
}
```

`send(cmd, payload)` is fire-and-forget. A non-OK status that echoes the
opcode fails the request with `StatusError` / `BusyError`. The payload that
`request` returns decodes with the codecs of the decode layer.

A plain device (one opened without `promote()`, or a board of an unknown
firmware) can drive request / reply commands this way; its continuous reports
are not routed (they come as hex `Text` events) — the sensor classes route
them.

## Byte types: `bytes`, `byte_span`, `span<T>`

The SDK avoids allocating on the read path: **inputs are non-owning views,
outputs are owned buffers** (`depz/span.hpp`).

- `depz::bytes` — `std::vector<std::byte>`, the owned buffer every `pack`/
  `build` returns.
- `depz::byte_span` — `depz::span<const std::byte>`, a non-owning `(ptr, len)`
  view; every `unpack`/`decode`/`feed` takes one. It implicitly converts from a
  `std::vector<std::byte>` or `std::array<std::byte, N>`.
- `depz::as_bytes(vec)` — wrap a `std::vector<std::byte>` as a `byte_span`.
- `depz::span<T>` is a tiny C++17 stand-in for `std::span` (this library
  predates C++20); you rarely name it directly.

```cpp
#include "depz/span.hpp"

depz::bytes buf = /* bytes you read off the wire */;
depz::byte_span view = depz::as_bytes(buf);   // no copy
```

## Mental model

This is the decode layer's picture — the path you drive yourself when you own
the port. The live layer runs the same pipeline on its reader thread and
hands you the typed values (see [Live hardware](#live-hardware)).

```
  serial port / recording / socket   (you own this)
            │   read bytes            │   write bytes
            ▼                         ▲
     PacketParser::feed()       build_packet()
            │                         │
      ParserEvent variant       framed A5C3 packet
   (Packet | Trash | CrcError)
            │
   std::visit / std::get_if
            │  Packet{cmd,seq,payload}
            ▼
   per-sensor payload codec  ──►  typed value
   (Sr04Data::unpack, decode_frame, parse_input_cargo, …)
```

- **You drive the loop.** In the decode layer there is no reader thread and no
  callback registry; you read bytes on your own schedule and feed them to a
  `PacketParser`.
- **Correlation is yours to do.** Solicited replies echo the request's command
  byte in `Packet::cmd` (unsolicited reports use `cmd == 0x00`,
  `depz::UNSOLICITED`); match replies to requests on that byte, exactly as the
  reference SDKs do — and as `depz::Device` does for you.
- **Raw integers are authoritative.** Codecs return the wire integers; unit
  conversions (0.1 °C, µs → mm, Q-point floats) are thin helpers on top.

## Transport: framing & CRC

`PacketParser` is incremental: feed arbitrary byte chunks, get an ordered
`std::vector<ParserEvent>`. Event order is invariant to how the stream is
chunked (only `Trash` byte-run boundaries depend on chunking).

```cpp
#include "depz/framing.hpp"
#include <variant>

depz::PacketParser parser;
for (const auto& ev : parser.feed(depz::as_bytes(rx))) {
    if (auto* p = std::get_if<depz::Packet>(&ev)) {
        // p->cmd, p->seq, p->payload   (payload is CRC-stripped)
    } else if (std::get_if<depz::CrcError>(&ev)) {
        // a well-framed packet whose payload CRC failed (dropped)
    } else if (auto* t = std::get_if<depz::Trash>(&ev)) {
        // bytes discarded while hunting for a frame
    }
}
// running counters: parser.packets / crc_errors / header_errors / trash_bytes
```

Build a packet to send (per firmware TX, the header advertises the CRC type
even for an empty payload, but ERRATA E6 means empty payloads carry no CRC
bytes):

```cpp
depz::bytes frame = depz::build_packet(
    static_cast<std::uint8_t>(depz::Cmd::SyncTime),
    depz::as_bytes(payload), /*seq=*/0, depz::CrcType::Crc16);
```

The four wire CRCs (`depz/crc.hpp`) are standalone reflected-table functions —
`crc8_maxim`, `crc16_modbus`, `crc32_iso_hdlc`, and `crc16_ccitt_false` (used
only for the `.fwdepz` header, never on the wire).

## Common commands & reports

`depz/common.hpp` holds the shared command/report IDs (`Cmd`, `Rpt`, `Status`)
and the payload codecs every sensor uses: `StatusReport`, `TextReport`,
`SyncTimeReport`, `TemperatureReport`, `SequenceErrorReport`, and the AUX
`SyncPinConfig`. Each is a plain struct with a `static … unpack(byte_span)` (and
`pack()` where the host sends it):

```cpp
auto st = depz::StatusReport::unpack(pkt.payload);
if (st.cmd == static_cast<std::uint8_t>(depz::Cmd::SetSyncPinConfig) &&
    st.status == static_cast<std::uint8_t>(depz::Status::Ok)) { /* … */ }

auto temp = depz::TemperatureReport::unpack(pkt.payload);
double c = temp.celsius();     // raw 0.1 °C → °C
```

## Time-sync math

Multi-device correlation is built on the same NTP-style clock math the firmware
uses. `sync_time_offset_rtt(t1, t2, t3, t4)` returns `{offset_us, rtt_us}` where
`offset = device_clock − host_clock`; drive it from a `SyncTimeReport`:

```cpp
auto r = depz::SyncTimeReport::unpack(pkt.payload);   // T1(echoed), T2, T3
std::int64_t t4 = depz::host_now_us();                // host monotonic µs
auto [offset_us, rtt_us] = depz::sync_time_offset_rtt(
    r.pc_timestamp_us, r.mcu_rx_us, r.mcu_tx_us, t4);
// host_us = device_timestamp_us − offset_us
```

Keep the lowest-RTT sample across a few rounds (contract 02 §5). Applying the
same offset per device is what lets several sensors — including two of the same
model — share one host timeline. On a live device, `Device::sync_time()` runs
the rounds and keeps the best one, and `to_host_time_us()` applies it.

## Discovery: USB identity

Selection is by **USB identity**, never "first port the OS enumerated"
(`depz/usb_ids.hpp`). Filter candidate ports to known DEPZ (vid, pid) pairs,
then order them deterministically by USB iSerial:

```cpp
#include "depz/usb_ids.hpp"

if (depz::is_known_depz_usb(vid, pid)) { /* a DEPZ (or dev-default) unit */ }
auto hint = depz::usb_model_hint(vid, pid);     // std::optional<std::string>

std::vector<depz::PortInfo> ordered =
    depz::order_by_serial(std::move(enumerated));  // ascending; empty serial last
```

On live hardware, `list_depz_devices()` and `open_device()` do this filtering,
ordering and probing for you (see [Live hardware](#open-a-device)).

The PID→model map is an informational **hint**; the protocol probe
(`GET_NAME_ACTIVE_SOFTWARE`, decoded by `depz::parse_software_name` in
`depz/identity.hpp`) is the source of truth for what a device actually is.

```cpp
std::string name = depz::strip_device_string(pkt.payload);   // drop NUL/0xFF filler
depz::Identity id = depz::parse_software_name(name);
// id.mode (App/Bootloader), id.sensor_type (Sr04/Vl53l8/…), id.version
```

The L5/L7 boards and the 1D family each share one firmware
(`SensorType::Vl53l7`, `SensorType::Vl53lx`), so the board comes from the USB
PID or the device name: `vl53l7::resolve_model()`, `vl53lx::resolve_class()`
/ `vl53lx::product_from_board_name()`. (`open_device()` applies the L5/L7
rule itself and returns a `depz::Vl53l8` with the right `Vl53l8Model`.)

## Firmware containers

`depz/fwdepz.hpp` parses and validates the `.fwdepz` application-firmware
container and holds the resident-bootloader wire codecs (`BlCmd`, `BlRpt`,
`FlashInfo`, `pack_write_page`, …):

```cpp
depz::FwDepzImage img = depz::FwDepzImage::parse(depz::as_bytes(blob));
// throws depz::FwDepzError on a bad length / magic / header CRC / size
bool ok = img.payload_crc_ok();      // CRC-32 over the payload
// img.load_addr, img.fw_size, img.cur_sec / tot_sec, img.payload
```

`Device::enter_bootloader()` reboots a live board into its bootloader. Driving
the actual erase/write/verify handshake is host-application work; this SDK
supplies the codecs it is built from.

## Datasets: record & replay

`depz::dataset::Reader` reads a decoded, multi-device, time-synced
`.depzdata` file (JSON-lines: a header line + one record per line). Records come
back **merged by host time** across every device, including two of the same
model:

```cpp
#include "depz/dataset.hpp"

depz::dataset::Reader ds(file_contents);
for (const auto& rec : ds.records) {         // stable-sorted by t_host_us
    // rec.device_id, rec.t_host_us, rec.kind
    // rec.ints / rec.strings / rec.arrays   (value split by JSON type)
}
// ds.schema, ds.devices (per-device DeviceMeta incl. TimeSync), ds.duration_us()
```

Writing datasets is a host-side concern; the SDK provides the reader used for
replay and regression. The raw-byte layer below it — the `.depzrec` capture
of everything that crossed the wire — is `Link::recording` / `Link::replay`
(see [record and replay](#record-and-replay)).

## What is not here

Every board of the line has its live class — `depz::Sr04`, `depz::Vl53l4cd`,
`depz::Vl53l8`, `depz::Vl53lx`, `depz::Bno055`, `depz::Bno086`. Not here is
the firmware-update flow: the `.fwdepz` container parses (`depz/fwdepz.hpp`)
and a board reboots into its bootloader (`Device::enter_bootloader()`), but
erasing and writing pages is left to your transport.

## Testing without hardware

Because every decode entry point takes bytes, the decode layer is exercised
off recorded captures — no serial port required. That is exactly how the
golden-vector suites run (`ctest --test-dir build`), including a real VL53L8
capture replayed through `PacketParser` → `FrameReassembler` → `decode_frame`.

The live layer is tested the same way, through its links: the `device_*`
tests run `depz::Sr04` against the C SDK's fake SR04 over
`Link::loopback_pair` (errors, timeouts, callbacks, streams, disconnect,
record → replay) and `depz::Vl53l4cd` against its fake register bridge
(init, configuration round trips, the "stop ranging first" guard), and a
**strict replay** of a real SR04 session, a real VL53L4CD session, a real
VL53L8CH session and a real BNO055 session
(`contracts/vectors/recordings/sr04_session.depzrec`,
`vl53l4cd_session.depzrec`, `vl53l8ch_cnh_8x8_15hz.depzrec` — with
`init()`'s whole firmware download, 8×8 at 15 Hz and CNH — and
`bno055_ndof_full_100hz.depzrec` in `device_bno_ndof_replay`: reset with the
boot-settle poll, `configure()` into NDOF with the fusion-start poll, the
calibration profile through CONFIG, system status, a 100 Hz full-block
stream) and `bno086_session.depzrec` in `device_bno086_session_replay` (a
BNO085: reset, three enables with their read-backs, 150 reports, a busy
retry, calibration, metadata, counts) makes the C++ SDK send every request
byte for byte as recorded and decode the same values;
`device_bno086_fake_errors_and_reports` runs `depz::Bno086` against the C
SDK's fake hub (busy, refusals, the rarer report types); five VL53L 1D-family
captures replay through `depz::Vl53lx` (`device_vlx_l0x_uld_replay`,
`_l1cx_uld_replay`, `_l3cx_ulp_replay`, `_l4cd_uld_replay` — the VL53L4CD ULD
borrowed by a VL53L4CX — and `_l4cx_hist_short_replay`, which checks that the
histogram driver now refuses the short preset on an L4 die). `ctest` runs 30
tests in all: `vectors`, 18 `device_*` tests and 11 per-suite vector
tests.
Your own tests can do the same: record a session once with `Link::recording`, then
replay it with `Link::replay(path, true)` in CI.
