# depz-sensor-sdk-cpp

C++17 SDK for the **DEPZ USB sensor line**, a small static library in two
layers:

- **The decode layer** — the framed-protocol parser and the per-sensor wire
  codecs. Pure codecs, no I/O: you feed it bytes, it gives you decoded frames.
  Byte-for-byte identical to the Python / TypeScript / Java reference SDKs via
  the shared golden test vectors in `contracts/vectors`.
- **The live-hardware layer** (`depz/device.hpp`) — opens a board and talks to
  it: serial links, USB discovery, `.depzrec` record / replay, a reader thread
  with request / reply matching, the common commands of every board, events
  and the sensor classes (**SR04**, **VL53L4CD** and the multizone
  **VL53L8CX / VL53L8CH / VL53L5CX / VL53L7CX / VL53L7CH** and **BNO055** so
  far). A C++ wrapper over the C SDK (`depz-sensor-sdk-c`): exceptions, move-only handles that
  close themselves, `std::function` callbacks and pull streams.

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main() {
    auto sr04 = depz::open_sr04();              // find, probe and open the SR04
    auto m = sr04->measure_once();
    if (auto mm = m.distance_mm()) std::printf("%.0f mm\n", *mm);
    else                           std::puts("no echo");
}
```

## The sensors

The DEPZ line's sensors share one framed protocol:

- **HC-SR04** — ultrasonic distance (`depz/sr04.hpp`, live class `depz::Sr04`).
- **VL53L4CD** — single-zone ToF on an I2C register bridge, the ST ULD 2.2.3
  on the host (`depz/vl53l4.hpp`, live class `depz::Vl53l4cd`).
- **VL53L8CX** — 8×8 multizone Time-of-Flight on an SPI register bridge, the
  **base** ToF die (ST ULD 2.1.0 on the host; `depz/vl53l8.hpp`, live class
  `depz::Vl53l8`). This is the dev-default part and carries no dedicated
  production USB PID (dev units enumerate under the STMicroelectronics dev
  vid/pid).
- **VL53L8CH** — the **same ToF as CX plus CNH histograms**, and its own
  production USB PID **0xED40**. Everything the CX does, the CH does
  identically — the same live class, model `L8CH` — and adds the
  compact-network-histogram (CNH) stream on top.
- **VL53L5CX / VL53L7CX / VL53L7CH** — 8×8 ToF on one I2C register-bridge
  firmware (63° / 90° / 90° + CNH): bridge codecs, class resolution and the
  L5/L7 frame decode (`depz/vl53l7.hpp`), and the same live class
  `depz::Vl53l8` (models `L5CX` / `L7CX` / `L7CH`).
- **VL53L0X / VL53L1CX / VL53L1CB / VL53L3CX / VL53L4CX** — the VL53L 1D ToF
  family on one I2C bridge firmware: bridge codecs, product table, class
  resolution and the die / VL53L0X / histogram block decoders
  (`depz/vl53lx.hpp`), and the live class `depz::Vl53lx` (every ST driver of
  the family: the VL53L0X API, the VL53L1X / VL53L4CD ULDs, the VL53L3CX ULP,
  the histogram driver with up to four targets).
- **BNO086** — 9-axis IMU, SH-2 over SHTP (`depz/bno086.hpp`).
- **BNO055** — 9-axis IMU with on-chip fusion on a register bridge: bridge
  codecs, units / calibration / axis-remap / page-1 config codecs and the
  register-window decode (`depz/bno055.hpp`, live class `depz::Bno055`).

CX and CH are two distinct sensors but one live class and one decode surface:
`depz::Vl53l8` drives both (its `Vl53l8Model` picks the sensor firmware), and
they stream the **same results-frame layout**, so a single
`depz::vl53l8::decode_frame` serves both (see `depz/vl53l8.hpp`). The only wire
difference the decoder cares about is the footer-id offset (CX = 12, CH = 4),
selectable via `depz::vl53l8::Variant`.

## What is covered today

**Live hardware** (`depz/device.hpp`, needs the C SDK — see [Install](#install)):

- Links: the serial port (`Link::serial`), `.depzrec` recording and strict /
  realtime replay, in-memory loopback pairs, custom C links.
- Discovery: `list_serial_ports`, `probe_port`, `list_depz_devices`, and
  `open_device` / `open_sr04` / `open_vl53l4cd` / `open_vl53l8` /
  `open_bno055` / `open_bno086` / `open_vl53lx` — USB-id
  filtered, ordered by USB serial, identified by a protocol probe.
- The device core, for **every** board: the common commands (names, serial,
  MCU temperature, payload CRC, sync pins, reset, bootloader entry), time
  sync, events, link statistics, and the `request` / `send` escape hatch.
- Sensor classes: **SR04** (`depz::Sr04` — configuration, single shots, the
  measurement loop as callbacks and streams), **VL53L4CD**
  (`depz::Vl53l4cd` — the ST ULD 2.2.3 run on the host over the register
  bridge: init, range timing, offset, crosstalk, detection window and quality
  thresholds, calibration, XSHUT, single shots and INT-driven ranging as
  callbacks and streams, raw register access) and the multizone ToF
  **VL53L8CX / VL53L8CH** (SPI board) and **VL53L5CX / VL53L7CX / VL53L7CH**
  (I2C board) (`depz::Vl53l8` — the ST ULD run on the host over the register
  bridge: init with the ~84 KB sensor-firmware download, resolution,
  frequency, ranging mode, sharpener, target order, power modes, crosstalk
  margin / calibration / blob, detection thresholds, the motion indicator,
  CNH on the L8CH / L7CH, INT-driven frame streaming as callbacks and
  streams; on the I2C board also bridge info, bus speed and pin control,
  and the sensor's module type), and the **BNO055** (`depz::Bno055` — the
  register logic of the on-chip fusion: `configure()` through CONFIG with the
  boot-settle and fusion-start polls, operating / power mode, units, axis
  remap and placements, temperature source, system status and self-test,
  calibration status / profile / soft-iron matrix, page-1 sensor configs,
  unique id and motion interrupts, raw registers on pages 0 / 1, polled
  samples and the timer / INT block stream as scaled `Bno055Sample`s through
  callbacks and streams; verified on replays of real captures and live on a
  lab board), and the **BNO085 / BNO086** (`depz::Bno086` — the SH-2
  sensor-hub protocol over the SHTP pass-through: reset and product id,
  enabling outputs with the granted rate read back, `Bno086Report`s scaled
  to m/s², rad/s, µT and quaternions through callbacks and streams,
  calibration / DCD / tare / reorientation, FRS records and sensor metadata,
  error queue and counts, the busy back-off; verified on a replay of a real
  capture and live on a lab BNO085), and the **VL53L 1D family** VL53L0X /
  L1CX / L1CB / L3CX / L4CX (`depz::Vl53lx` — init with a (product, driver)
  pair, a sibling's name borrowing its driver; `configure()` with budget,
  mode and a stored calibration; modes, budgets, calibrations, thresholds,
  ROI; `Vl53lxMeasurement`s with targets and the 24-bin histogram through
  callbacks and streams, single shots; verified on strict replays of real
  captures, the C class it wraps live on a lab VL53L4CX).
- A typed exception hierarchy under `depz::Error`.

**The decode layer** (no dependencies):

- Framing: COBS-free length/CRC framing, packet reassembly (`depz/framing.hpp`),
  CRC (`depz/crc.hpp`), fw-DEPZ blob parsing (`depz/fwdepz.hpp`).
- Common protocol: command/report IDs and payload codecs (`depz/common.hpp`),
  firmware-name identity parsing (`depz/identity.hpp`), USB id hints
  (`depz/usb_ids.hpp`).
- SR04: full wire codecs (`depz/sr04.hpp`) — and the live class above.
- VL53L4CD: register-bridge codecs, result decode, timing/tuning math and
  init block (`depz/vl53l4.hpp`) — and the live class above.
- **VL53L8 (CX + CH)**: frame-chunk reassembly, the shared results-frame decoder
  (raw per-zone arrays), and the advanced-feature DCI codecs (xtalk margin,
  detection thresholds, motion indicator) — `depz/vl53l8.hpp` — and the live
  class above.
- **BNO086**: SHTP framing, SH-2 encoders, report decode (`depz/bno086.hpp`) —
  and the live class above.
- **VL53L5CX / L7CX / L7CH**: bridge codecs, class resolution and the L5/L7
  frame decode (`depz/vl53l7.hpp`) — and the live class above.
- **BNO055**: bridge codecs, units / calibration / axis-remap / page-1
  config codecs and the register-window decode (`depz/bno055.hpp`) — and the
  live class above.
- **The VL53L 1D family**: wire codecs, class resolution and stateless
  decode (`depz/vl53lx.hpp`) — and the live class above.
- **CNH histogram decode** (VL53L8CH, VL53L7CH): `depz::vl53l8::decode_cnh`.

Every board of the line has its live class.

## Documentation

Full docs live under [`docs/`](docs/):

- [Common guide](docs/guide.md) — build, the live-hardware layer (opening a
  board, errors, threads and callbacks, record / replay), byte types, the
  decode mental model, and the cross-sensor codecs (framing/CRC, identity,
  discovery, time-sync, firmware, datasets).
- Per-sensor **introduction + user guide + API**:
  [SR04](docs/sr04/introduction.md) ·
  [VL53L4CD](docs/vl53l4cd/introduction.md) ·
  [VL53L8CX](docs/vl53l8cx/introduction.md) ·
  [VL53L8CH](docs/vl53l8ch/introduction.md) ·
  [VL53L5CX](docs/vl53l5cx/introduction.md) ·
  [VL53L7CX](docs/vl53l7cx/introduction.md) ·
  [VL53L7CH](docs/vl53l7ch/introduction.md) ·
  [VL53L0X](docs/vl53l0x/introduction.md) ·
  [VL53L1CX](docs/vl53l1cx/introduction.md) ·
  [VL53L1CB](docs/vl53l1cb/introduction.md) ·
  [VL53L3CX](docs/vl53l3cx/introduction.md) ·
  [VL53L4CX](docs/vl53l4cx/introduction.md) ·
  [BNO086](docs/bno086/introduction.md) ·
  [BNO055](docs/bno055/introduction.md)
- [Full API reference](docs/api.md) — every public symbol, generated from the
  header doc-comments by `scripts/gen_api_md.py` (regenerate with
  `cmake --build build --target docs`, or `python3 scripts/gen_api_md.py`).

## Install

The library ships an installable CMake package. The consumable target is
`depz::sensor_sdk_cpp` (the alias `depz::sensor_sdk` is kept for backwards
compatibility).

**Dependency.** The live-hardware layer is built by default
(`DEPZ_SENSOR_SDK_CPP_BUILD_IO=ON`) and needs the C SDK, `depz-sensor-sdk-c`
(C11, links the platform thread library). It is taken from an existing
`depz::sensor_sdk_c` target, else from an installed package
(`find_package(depz-sensor-sdk-c)`), else — in this repository — from the
sibling directory `packages/depz-sensor-sdk-c` via `add_subdirectory`.
`depz::sensor_sdk_cpp` links it publicly, so you link only the C++ target.
Pass `-DDEPZ_SENSOR_SDK_CPP_BUILD_IO=OFF` for the decode layer alone: pure
C++17, no dependencies, no `depz/device.hpp`.

### CMake FetchContent

The SDK lives in a subdirectory of the repository, so point FetchContent at that
subdir with `SOURCE_SUBDIR`. The whole repository is fetched, so the C SDK is
picked up from the sibling directory. Tests are automatically off when consumed
this way.

```cmake
include(FetchContent)
FetchContent_Declare(depz-sensor-sdk-cpp
  GIT_REPOSITORY https://github.com/depz-ai/depz-sensor-sdk.git
  GIT_TAG        v0.3.0
  SOURCE_SUBDIR  packages/depz-sensor-sdk-cpp)
FetchContent_MakeAvailable(depz-sensor-sdk-cpp)

target_link_libraries(your_app PRIVATE depz::sensor_sdk_cpp)
```

### CMake add_subdirectory

From a checkout of this repository (or a copy of `packages/depz-sensor-sdk-c`
and `packages/depz-sensor-sdk-cpp` kept side by side):

```cmake
add_subdirectory(path/to/packages/depz-sensor-sdk-cpp)
target_link_libraries(your_app PRIVATE depz::sensor_sdk_cpp)
```

### CMake find_package (installed / packaged)

```bash
cmake -B build -S packages/depz-sensor-sdk-cpp
cmake --build build
cmake --install build --prefix /your/prefix   # installs the C SDK it built, too
```

The installed package finds `depz-sensor-sdk-c` itself (when the live layer was
built); put `/your/prefix` on `CMAKE_PREFIX_PATH`:

```cmake
find_package(depz-sensor-sdk-cpp CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE depz::sensor_sdk_cpp)
```

### Conan 2

A Conan 2 recipe (`conanfile.py`) is provided. It requires
`depz-sensor-sdk-c` of the same version, so create that recipe first:

```bash
conan create packages/depz-sensor-sdk-c
conan create packages/depz-sensor-sdk-cpp
```

Then consume with `find_package(depz-sensor-sdk-cpp)` and link
`depz::sensor_sdk_cpp` (via `CMakeDeps`/`CMakeToolchain`). Submission to Conan
Center is in review
([conan-center-index#30564](https://github.com/conan-io/conan-center-index/pull/30564));
until it merges, use the in-repo recipe as above.

### vcpkg

Submission to the vcpkg registry is in review
([microsoft/vcpkg#52770](https://github.com/microsoft/vcpkg/pull/52770)); until
it merges, use the in-repo ports as
[overlay ports](https://learn.microsoft.com/vcpkg/concepts/overlay-ports) — this
one depends on the C SDK's port:

```bash
vcpkg install depz-sensor-sdk-cpp \
  --overlay-ports=packages/depz-sensor-sdk-c/vcpkg \
  --overlay-ports=packages/depz-sensor-sdk-cpp/vcpkg
```

```cmake
find_package(depz-sensor-sdk-cpp CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE depz::sensor_sdk_cpp)
```

> The portfile references release tag `v0.3.0`; fill in the archive `SHA512`
> in `vcpkg/portfile.cmake` when that tag is published.

## Build & test

```bash
cmake -B build -S .
cmake --build build
ctest --test-dir build
```

Tests are gated behind the `DEPZ_SENSOR_SDK_CPP_BUILD_TESTS` option (ON for a
standalone build, OFF when the project is pulled in via `add_subdirectory` /
FetchContent). `ctest` runs 30 tests: the golden-vector suites and the
`device_*` tests, which run the live layer against the C SDK's fake SR04,
fake VL53L4CD bridge and fake BNO086 hub over a loopback link and strictly
replay a real SR04, a real VL53L4CD, a real VL53L8CH session (with the whole
firmware download and CNH), a real BNO055 NDOF session (reset, configure, the
calibration profile through CONFIG, a 100 Hz full-block stream) and a real
BNO085 session (reset, three enables with their read-backs, 150 reports, a
busy retry, calibration, metadata, counts), plus five VL53L 1D-family
captures through `depz::Vl53lx` (the VL53L0X API, the VL53L1X ULD, the
VL53L3CX ULP, the VL53L4CD ULD borrowed by a VL53L4CX and the VL53L4CX
histogram driver's short preset, frame for frame). CI builds and tests Linux, macOS and Windows.

The live layer has also been checked on real boards. SR04 (firmware
`APP_usonic_SR04_v0.97`): `open_sr04`, temperature, `measure_once`,
`BusyError` during the loop, 50 samples/s to a stream and a callback, and
`StatusError` for an unknown command. VL53L4CD (firmware `APP_VL53L4_v0.83`):
`init()`, single shots (1042 mm, valid), `ArgumentError` for configuration
during ranging, 33 frames/s at a 33 ms budget (mean 1047 mm), none dropped.
VL53L8CH (board `TMNQ8E3PRR`): model `L8CH` detected from the USB PID,
`init()` with the firmware download in 0.76 s, 4×4 at 30 Hz — 30 frames/s at
~596 mm, none dropped — sleep and wake, and 1 Hz refused. VL53L7CH (board
`TXK5KAX6X4`, firmware `APP_VL53L7_v0.53`, 0.6 m): model `L7CH` and module
MZEVO detected, `init()` in 1.29 s, 8×8 at 15 Hz ~602 mm, 1 Hz ~596 mm, CNH
1708 bytes per frame, the I2C bus 450 → 500 kHz snap, a soft cycle then a
fresh `init()`, no I2C errors. The VL53L5CX runs the same code and sensor
firmware; its captures replay in the C SDK's tests. BNO055 (board
`I0MG1KQN8DW`, `APP_BNO055_v0.12`): NDOF at 100.0 Hz, a minute turned by
hand, orientation and gravity following the board. BNO085 (board
`I5MFL1ONMCD`, `APP_BNO086_v0.99`, SH-2 3.2.13): `open_bno086`, product id,
game rotation vector 201 Hz, gravity 50 Hz with |g| = 9.85 m/s², gyroscope
100 Hz, unit quaternions, metadata, no reports dropped.

## License

MIT. Bundled/derived VL53L8 advanced-DCI codecs are © STMicroelectronics
(BSD-3-Clause).

Open source — source and issue tracker:
<https://github.com/depz-ai/depz-sensor-sdk>.
