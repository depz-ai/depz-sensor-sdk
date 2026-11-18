# depz-sensor-sdk-cpp

C++17 SDK for the **DEPZ USB sensor line**, a small static library in two
layers:

- **The decode layer** — the framed-protocol parser and the per-sensor wire
  codecs. Pure codecs, no I/O: you feed it bytes, it gives you decoded frames.
  Byte-for-byte identical to the Python / TypeScript / Java reference SDKs via
  the golden test vectors shared by all DEPZ SDKs.
- **The live-hardware layer** (`depz/device.hpp`) — opens a board and talks to
  it: serial links, USB discovery, `.depzrec` record / replay, a reader thread
  with request / reply matching, the common commands of every board, events
  and the sensor classes (**SR04**, **VL53L4CD** and the multizone
  **VL53L5CX / VL53L7CX / VL53L7CH / VL53L8CX / VL53L8CH**, **BNO055** and
  **TMF8829** so far). A C++ wrapper over the C SDK (`depz-sensor-sdk-c`): exceptions, move-only handles that
  close themselves, `std::function` callbacks and pull streams.

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main() {
    try {
        auto sr04 = depz::open_sr04();          // find, probe and open the SR04
        auto m = sr04->measure_once();
        if (auto mm = m.distance_mm()) std::printf("%.0f mm\n", *mm);
        else                           std::puts("no echo");
    } catch (const depz::Error& e) {            // no board: "no DEPZ device found (…)"
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }
}
```

[A first program](#a-first-program) builds and runs it.

## The sensors

The DEPZ line's sensors share one framed protocol:

- **HC-SR04** — ultrasonic distance (`depz/sr04.hpp`, live class `depz::Sr04`).
- **VL53L0X / VL53L1CX / VL53L1CB / VL53L3CX / VL53L4CX** — the VL53L 1D ToF
  family on one I2C bridge firmware: bridge codecs, product table, class
  resolution and the die / VL53L0X / histogram block decoders
  (`depz/vl53lx.hpp`), and the live class `depz::Vl53lx` (every ST driver of
  the family: the VL53L0X API, the VL53L1X / VL53L4CD ULDs, the VL53L3CX ULP,
  the histogram driver with up to four targets).
- **VL53L4CD** — single-zone ToF on an I2C register bridge, the ST ULD 2.2.3
  on the host (`depz/vl53l4.hpp`, live class `depz::Vl53l4cd`).
- **VL53L5CX / VL53L7CX / VL53L7CH** — 8×8 ToF on one I2C register-bridge
  firmware (63° / 90° / 90° + CNH): bridge codecs, class resolution and the
  L5/L7 frame decode (`depz/vl53l7.hpp`), and the same live class
  `depz::Vl53l8` (models `L5CX` / `L7CX` / `L7CH`).
- **VL53L8CX** — 8×8 multizone Time-of-Flight on an SPI register bridge, the
  **base** ToF die (ST ULD 2.1.0 on the host; `depz/vl53l8.hpp`, live class
  `depz::Vl53l8`). This is the dev-default part and carries no dedicated
  production USB PID (dev units enumerate under the STMicroelectronics dev
  vid/pid).
- **VL53L8CH** — the **same ToF as CX plus CNH histograms**, and its own
  production USB PID **0xED40**. Everything the CX does, the CH does
  identically — the same live class, model `L8CH` — and adds the
  compact-network-histogram (CNH) stream on top.
- **BNO055** — 9-axis IMU with on-chip fusion on a register bridge: bridge
  codecs, units / calibration / axis-remap / page-1 config codecs and the
  register-window decode (`depz/bno055.hpp`, live class `depz::Bno055`).
- **BNO085 / BNO086** — 9-axis IMU, SH-2 over SHTP (`depz/bno086.hpp`).
- **TMF8829** — ams-OSRAM multizone direct ToF, 8×8 … 48×32 zones, up to 4
  peaks per zone, raw histograms, on a thin SPI register bridge: bridge codecs,
  chunk reassembly, frame / pixel decode with the half-frame merge, the clock
  model and capture time, presets and config-page fields, the ams sensor
  firmware the C SDK embeds (MIT) (`depz/tmf8829.hpp`, live class
  `depz::Tmf8829`).

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
  `open_bno055` / `open_bno086` / `open_vl53lx` / `open_tmf8829` — USB-id
  filtered, ordered by USB serial, identified by a protocol probe.
- The device core, for **every** board: the common commands (names, serial,
  MCU temperature, payload CRC, sync pins, reset, bootloader entry), time
  sync, events, link statistics, and the `request` / `send` escape hatch.
- Sensor classes: **SR04** (`depz::Sr04` — configuration, single shots, the
  measurement loop as callbacks and streams), the **VL53L 1D family** VL53L0X /
  L1CX / L1CB / L3CX / L4CX (`depz::Vl53lx` — init with a (product, driver)
  pair, a sibling's name borrowing its driver; `configure()` with budget, mode
  and a stored calibration; modes, budgets, calibrations, thresholds, ROI;
  `Vl53lxMeasurement`s with targets and the 24-bin histogram through callbacks
  and streams, single shots; verified on strict replays of real captures, the C
  class it wraps live on a lab VL53L4CX), **VL53L4CD** (`depz::Vl53l4cd` — the
  ST ULD 2.2.3 run on the host over the register bridge: init, range timing,
  offset, crosstalk, detection window and quality thresholds, calibration,
  XSHUT, single shots and INT-driven ranging as callbacks and streams, raw
  register access) and the multizone ToF **VL53L5CX / VL53L7CX / VL53L7CH**
  (I2C board) and **VL53L8CX / VL53L8CH** (SPI board) (`depz::Vl53l8` — the ST
  ULD run on the host over the register bridge: init with the ~84 KB
  sensor-firmware download, resolution, frequency, ranging mode, sharpener,
  target order, power modes, crosstalk margin / calibration / blob, detection
  thresholds, the motion indicator, CNH on the L7CH / L8CH, INT-driven frame
  streaming as callbacks and streams; on the I2C board also bridge info, bus
  speed and pin control, and the sensor's module type), and the **BNO055**
  (`depz::Bno055` — the register logic of the on-chip fusion: `configure()`
  through CONFIG with the boot-settle and fusion-start polls, operating / power
  mode, units, axis remap and placements, temperature source, system status and
  self-test, calibration status / profile / soft-iron matrix, page-1 sensor
  configs, unique id and motion interrupts, raw registers on pages 0 / 1,
  polled samples and the timer / INT block stream as scaled `Bno055Sample`s
  through callbacks and streams; verified on replays of real captures and live
  on a lab board), and the **BNO085 / BNO086** (`depz::Bno086` — the SH-2
  sensor-hub protocol over the SHTP pass-through: reset and product id,
  enabling outputs with the granted rate read back, `Bno086Report`s scaled to
  m/s², rad/s, µT and quaternions through callbacks and streams, calibration /
  DCD / tare / reorientation, interactive calibration and the flush of batched
  reports, FRS records and sensor metadata, error queue and counts, the busy
  back-off; verified on a replay of a real capture and live on
  a lab BNO085), and the **TMF8829** (`depz::Tmf8829` — the host runs the
  sensor: `init()` with the firmware download and progress, `configure(preset,
  options)` writing every field a preset keeps and reading it back, the config
  page, raw registers, bridge info, start / stop with the expected stop-time
  bad-header event swallowed, `Tmf8829Measurement`s with clock-corrected
  capture times and their histogram / ref-SPAD frames (`histograms`, each
  with its decoded header and whole raw frame; `ref_frames`) through callbacks
  and streams alike, raw frames of every kind through `on_frame()`, events and
  stream counters; verified against the C SDK's fake bridge).
- A typed exception hierarchy under `depz::Error`.

**The decode layer** (no dependencies):

- Framing: COBS-free length/CRC framing, packet reassembly (`depz/framing.hpp`),
  CRC (`depz/crc.hpp`), fw-DEPZ blob parsing (`depz/fwdepz.hpp`).
- Common protocol: command/report IDs and payload codecs (`depz/common.hpp`),
  firmware-name identity parsing (`depz/identity.hpp`), USB id hints
  (`depz/usb_ids.hpp`).
- SR04: full wire codecs (`depz/sr04.hpp`) — and the live class above.
- **The VL53L 1D family**: wire codecs, class resolution and stateless
  decode (`depz/vl53lx.hpp`) — and the live class above.
- VL53L4CD: register-bridge codecs, result decode, timing/tuning math and
  init block (`depz/vl53l4.hpp`) — and the live class above.
- **VL53L5CX / L7CX / L7CH**: bridge codecs, class resolution and the L5/L7
  frame decode (`depz/vl53l7.hpp`) — and the live class above.
- **VL53L8 (CX + CH)**: frame-chunk reassembly, the shared results-frame decoder
  (raw per-zone arrays), and the advanced-feature DCI codecs (xtalk margin,
  detection thresholds, motion indicator) — `depz/vl53l8.hpp` — and the live
  class above.
- **BNO055**: bridge codecs, units / calibration / axis-remap / page-1
  config codecs and the register-window decode (`depz/bno055.hpp`) — and the
  live class above.
- **BNO085 / BNO086**: SHTP framing, SH-2 encoders, report decode
  (`depz/bno086.hpp`) — and the live class above.
- **TMF8829**: bridge codecs, chunk reassembler, frame / pixel decode and the
  half-frame merge, `SensorClock` + `CaptureTimer` (bit-exact with the 27
  recorded streams of `tmf8829_frame_time.json`), `MeasurementBuilder`,
  presets / config fields, the sensor firmware image (`depz/tmf8829.hpp`) —
  and the live class above.
- **CNH histogram decode** (VL53L7CH, VL53L8CH): `depz::vl53l8::decode_cnh`.

Every board of the line has its live class.

## Documentation

Full docs live under [`docs/`](docs/):

- [Common guide](docs/guide.md) — build, the live-hardware layer (opening a
  board, errors, threads and callbacks, record / replay), byte types, the
  decode mental model, and the cross-sensor codecs (framing/CRC, identity,
  discovery, time-sync, firmware, datasets).
- Per-sensor **introduction + user guide + API**:
  [SR04](docs/sr04/introduction.md) ·
  [VL53L0X](docs/vl53l0x/introduction.md) ·
  [VL53L1CX](docs/vl53l1cx/introduction.md) ·
  [VL53L1CB](docs/vl53l1cb/introduction.md) ·
  [VL53L3CX](docs/vl53l3cx/introduction.md) ·
  [VL53L4CD](docs/vl53l4cd/introduction.md) ·
  [VL53L4CX](docs/vl53l4cx/introduction.md) ·
  [VL53L5CX](docs/vl53l5cx/introduction.md) ·
  [VL53L7CX](docs/vl53l7cx/introduction.md) ·
  [VL53L7CH](docs/vl53l7ch/introduction.md) ·
  [VL53L8CX](docs/vl53l8cx/introduction.md) ·
  [VL53L8CH](docs/vl53l8ch/introduction.md) ·
  [TMF8829](docs/tmf8829/introduction.md) ·
  [BNO055](docs/bno055/introduction.md) ·
  [BNO085](docs/bno085/introduction.md) ·
  [BNO086](docs/bno086/introduction.md)
- [Full API reference](docs/api.md) — every public symbol, generated from the
  header doc-comments.

## Install

The library ships an installable CMake package. The consumable target is
`depz::sensor_sdk_cpp` (the alias `depz::sensor_sdk` is kept for backwards
compatibility).

**Dependency.** The C++ SDK is a C++ face of the C SDK, `depz-sensor-sdk-c`
(C11, links the platform thread library): the codecs wrap `depz_sensor_sdk.h`,
the live layer `depz_sensor_io.h`. It is taken from an existing
`depz::sensor_sdk_c` target, else from an installed package
(`find_package(depz-sensor-sdk-c)`), else — in this repository — from the
sibling directory `packages/depz-sensor-sdk-c` via `add_subdirectory`.
`depz::sensor_sdk_cpp` links it publicly, so you link only the C++ target.

### CMake FetchContent

The SDK lives in a subdirectory of the repository, so point FetchContent at that
subdir with `SOURCE_SUBDIR`. The whole repository is fetched, so the C SDK is
picked up from the sibling directory. Tests are automatically off when consumed
this way.

```cmake
include(FetchContent)
FetchContent_Declare(depz-sensor-sdk-cpp
  GIT_REPOSITORY https://github.com/depz-ai/depz-sensor-sdk.git
  GIT_TAG        v1.0.0
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

A checkout that is not under your project's folder needs a build folder named
as the second argument: `add_subdirectory(/path/to/packages/depz-sensor-sdk-cpp depz-sensor-sdk-cpp)`.

### A first program

The program at the top of this page as `main.cpp`, and beside it this
`CMakeLists.txt` (CMake 3.16 or newer, a C++17 compiler and a C11 compiler —
on Debian / Ubuntu `apt-get install cmake make gcc g++ libc6-dev`):

```cmake
cmake_minimum_required(VERSION 3.16)
project(first_program CXX)
add_subdirectory(/path/to/packages/depz-sensor-sdk-cpp depz-sensor-sdk-cpp)
add_executable(first_program main.cpp)
target_link_libraries(first_program PRIVATE depz::sensor_sdk_cpp)
```

<!-- doc-check: skip — a build of the user's program, not an install command -->
```bash
cmake -B build -S .
cmake --build build
build/first_program
```

With an SR04 attached it prints one distance; with no board,
`no DEPZ device found (no serial port has a known DEPZ USB id); …`, exit code 1.

### CMake find_package (installed / packaged)

<!-- doc-check: skip — build and install from a checkout, not a package install -->
```bash
cmake -B build -S packages/depz-sensor-sdk-cpp
cmake --build build
cmake --install build --prefix /your/prefix   # installs the C SDK it built, too
```

The installed package finds `depz-sensor-sdk-c` itself; put `/your/prefix` on
`CMAKE_PREFIX_PATH`:

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
`depz::sensor_sdk_cpp` (via `CMakeDeps`/`CMakeToolchain`). The packages are not
on Conan Center: the two recipes of this repository are how to use the SDK with
Conan.

### vcpkg

The ports are not in the vcpkg registry (the submission,
[microsoft/vcpkg#52770](https://github.com/microsoft/vcpkg/pull/52770), was
closed without a merge): use the in-repo ports as
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

> The portfile references release tag `v1.0.0` of the public repository; the
> `SHA512` in `vcpkg/portfile.cmake` is the hash of that tag's archive.

## Compatibility after 1.0

From 1.0 a minor release only adds (contract 07 §5), and the C++ SDK keeps
the C SDK's policy: *source* compatibility, not ABI. A struct the SDK
produces — `LinkStats`, `DeviceInfo`, `DeviceEvent`, `TimeSync`, every
measurement / frame / sample / report, `Identity`, `Tmf8829Config`, the SH-2
replies, `dataset::Record`, … — only ever gains members at its end, an enum
(`DeviceEvent::Type`, `SensorType`, `Vl53l8Model`, `PlayerState`,
`bno086::ReportType`, …) only gains enumerators and a variant (`ParserEvent`,
`dataset::Value`) only gains alternatives; so `sizeof` may change in a minor
release. Recompile against the new headers on a minor update — a shared
library too. What *you* build — `OpenOptions`, `Tmf8829Options`,
`RecorderOptions`, `dataset::PlayerOptions`, `SyncPinConfig`,
`DetectionThresholds`, `bno086::FeatureRequest` / `CalibrationConfig`,
`CnhSetup`, `vl53l8::MotionConfig` / `DetectionThreshold`, `vl53l4::RangeTiming`,
the `bno055` register structs, `bno086::ShtpHeader` — is written one way:
`{}` then assignment (designated initialisers do the same under C++20; the
SDK itself is C++17), never positional `{a, b, c}`: a positional list names
nothing and stops being complete the moment a member is appended (`-Wextra`
warns, `-Werror` fails). A `switch` on `DeviceEvent::Type` or any other SDK
enum keeps a `default:`, a `std::visit` on a variant a generic `auto&`
overload.

<!-- doc-check: skip — right/wrong pair: the second line fails on purpose -->
```cpp
depz::OpenOptions opt{}; opt.port = "/dev/ttyACM0";            // right (C++20: depz::OpenOptions{.port = "/dev/ttyACM0"})
depz::OpenOptions opt{"/dev/ttyACM0", std::nullopt, std::nullopt, {}};  // wrong: positional
```

The handle classes (`Device`, the sensor classes, `Stream`, `DatasetPlayer`,
`SessionRecorder`, …) have private state and need no rule; the ST ULD port
types are not public API.

## Build

<!-- doc-check: skip — build from a checkout, not an install command -->
```bash
cmake -B build -S .
cmake --build build
```

The build produces the static library and, with it, the C SDK it wraps. The
`DEPZ_SENSOR_SDK_CPP_BUILD_TESTS` option is off in this repository, and
`DEPZ_SENSOR_SDK_CPP_WERROR` (warnings as errors on gcc, clang and MSVC `/W4`)
follows it: packagers keep warnings as warnings. The library is built on
Linux, macOS and Windows.

The SDK is tested without hardware against golden vectors and recorded
sessions of real boards shared by all seven SDKs. The test suite is kept in
DEPZ's development repository and is not part of this one.

The live layer has also been checked on real boards. SR04 (firmware
`APP_usonic_SR04_v0.97`): `open_sr04`, temperature, `measure_once`,
`BusyError` during the loop, 50 samples/s to a stream and a callback, and
`StatusError` for an unknown command. VL53L4CD (firmware `APP_VL53L4_v0.83`):
`init()`, single shots (1042 mm, valid), `ArgumentError` for configuration
during ranging, 33 frames/s at a 33 ms budget (mean 1047 mm), none dropped.
VL53L8CH: model `L8CH` detected from the USB PID,
`init()` with the firmware download in 0.76 s, 4×4 at 30 Hz — 30 frames/s at
~596 mm, none dropped — sleep and wake, and 1 Hz refused. VL53L7CH
(firmware `APP_VL53L7_v0.53`, 0.6 m): model `L7CH` and module
MZEVO detected, `init()` in 1.29 s, 8×8 at 15 Hz ~602 mm, 1 Hz ~596 mm, CNH
1708 bytes per frame, the I2C bus 450 → 500 kHz snap, a soft cycle then a
fresh `init()`, no I2C errors. The VL53L5CX runs the same code and sensor
firmware; its captures replay in the C SDK's tests. BNO055
(firmware `APP_BNO055_v0.12`): NDOF at 100.0 Hz, a minute turned by
hand, orientation and gravity following the board. BNO085
(firmware `APP_BNO086_v0.99`, SH-2 3.2.13): `open_bno086`, product id,
game rotation vector 201 Hz, gravity 50 Hz with |g| = 9.85 m/s², gyroscope
100 Hz, unit quaternions, metadata, no reports dropped.

## License

MIT. The bundled VL53L5 / VL53L7 / VL53L8 sensor firmware, configuration
data and the driver code ported from ST's Ultra Lite Driver are
© STMicroelectronics (BSD-3-Clause,
[`licenses/vl53l8/LICENSE-BSD-3-Clause.txt`](licenses/vl53l8/LICENSE-BSD-3-Clause.txt)).

Open source — source and issue tracker:
<https://github.com/depz-ai/depz-sensor-sdk>.
