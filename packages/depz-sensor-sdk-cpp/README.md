# depz-sensor-sdk-cpp

C++17 decode-layer SDK for the **DEPZ USB sensor line** — the framed-protocol
parser and the per-sensor wire codecs, header-only public API over a small static
library. Pure codecs, **no I/O / no transport**: you feed it bytes, it gives you
decoded frames. Byte-for-byte identical to the Python / TypeScript / Java
reference SDKs via the shared golden test vectors in `contracts/vectors`.

## The four sensors

The DEPZ line is **four** user-facing sensors that share one framed protocol:

- **HC-SR04** — ultrasonic distance (`depz/sr04.hpp`).
- **VL53L8CX** — 8×8 multizone Time-of-Flight, the **base** ToF die (ST ULD
  2.1.0). This is the dev-default part and carries no dedicated production USB
  PID (dev units enumerate under the STMicroelectronics dev vid/pid).
- **VL53L8CH** — the **same ToF as CX plus CNH histograms**, and its own
  production USB PID **0xED40**. Everything the CX decodes, the CH decodes
  identically; the CH adds the compact-noise-histogram (CNH) stream on top.
- **BNO086** — 9-axis IMU, SH-2 over SHTP (`depz/bno086.hpp`).

CX and CH are two distinct sensors but one decode surface: they stream the
**same results-frame layout**, so a single `depz::vl53l8::decode_frame` serves
both (see `depz/vl53l8.hpp`). The only wire difference the decoder cares about is
the footer-id offset (CX = 12, CH = 4), selectable via `depz::vl53l8::Variant`.

## What is covered today

This SDK is the **verifiable decode layer**. Covered:

- Framing: COBS-free length/CRC framing, packet reassembly (`depz/framing.hpp`),
  CRC (`depz/crc.hpp`), fw-DEPZ blob parsing (`depz/fwdepz.hpp`).
- Common protocol: command/report IDs and payload codecs (`depz/common.hpp`),
  firmware-name identity parsing (`depz/identity.hpp`), USB id hints
  (`depz/usb_ids.hpp`).
- SR04: full wire codecs (`depz/sr04.hpp`).
- **VL53L8 (CX + CH)**: frame-chunk reassembly, the shared results-frame decoder
  (raw per-zone arrays), and the advanced-feature DCI codecs (xtalk margin,
  detection thresholds, motion indicator) — `depz/vl53l8.hpp`.
- BNO086: SHTP framing + SH-2 report decode (`depz/bno086.hpp`).

**Not yet implemented** (the CH-specific and hardware-dependent extension points):

- **CNH histogram decode** — the CH-only compact-noise-histogram stream. This is
  the CH's extension point over CX and is intentionally left as a documented
  stub in `depz/vl53l8.hpp`; no fake implementation is provided.
- **Live ULD init / register-bridge driver** — firmware download and the
  DCI read/write handshakes that bring a ToF part up on real hardware. This
  decode-only SDK deliberately omits it; see the reference Python `vl53l8/uld.py`.

## Documentation

Full docs live under [`docs/`](docs/):

- [Common guide](docs/guide.md) — build, byte types, the decode mental model,
  and the cross-sensor codecs (framing/CRC, identity, discovery, time-sync,
  firmware, datasets).
- Per-sensor **introduction + user guide + API**:
  [SR04](docs/sr04/introduction.md) ·
  [VL53L8CX](docs/vl53l8cx/introduction.md) ·
  [VL53L8CH](docs/vl53l8ch/introduction.md) ·
  [BNO086](docs/bno086/introduction.md)
- [Full API reference](docs/api.md) — every public symbol, generated from the
  header doc-comments by `scripts/gen_api_md.py` (regenerate with
  `cmake --build build --target docs`, or `python3 scripts/gen_api_md.py`).

## Install

The library ships an installable CMake package. The consumable target is
`depz::sensor_sdk_cpp` (the alias `depz::sensor_sdk` is kept for backwards
compatibility). It is pure C++17 with no external dependencies.

### CMake FetchContent

The SDK lives in a subdirectory of the monorepo, so point FetchContent at that
subdir with `SOURCE_SUBDIR`. Tests are automatically off when consumed this way.

```cmake
include(FetchContent)
FetchContent_Declare(depz-sensor-sdk-cpp
  GIT_REPOSITORY https://github.com/depz-ai/depz-sensor-sdk-and-viewer.git
  GIT_TAG        cpp-v0.1.0
  SOURCE_SUBDIR  packages/depz-sensor-sdk-cpp)
FetchContent_MakeAvailable(depz-sensor-sdk-cpp)

target_link_libraries(your_app PRIVATE depz::sensor_sdk_cpp)
```

### CMake find_package (installed / packaged)

```bash
cmake -B build -S packages/depz-sensor-sdk-cpp
cmake --build build
cmake --install build --prefix /your/prefix
```

```cmake
find_package(depz-sensor-sdk-cpp CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE depz::sensor_sdk_cpp)
```

### Conan 2

A Conan 2 recipe (`conanfile.py`) is provided:

```bash
conan create packages/depz-sensor-sdk-cpp
```

Then consume with `find_package(depz-sensor-sdk-cpp)` and link
`depz::sensor_sdk_cpp` (via `CMakeDeps`/`CMakeToolchain`).

### vcpkg

A port lives in `vcpkg/` (`vcpkg.json` + `portfile.cmake`). Add it as an
[overlay port](https://learn.microsoft.com/vcpkg/concepts/overlay-ports):

```bash
vcpkg install depz-sensor-sdk-cpp --overlay-ports=packages/depz-sensor-sdk-cpp/vcpkg
```

```cmake
find_package(depz-sensor-sdk-cpp CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE depz::sensor_sdk_cpp)
```

> The portfile references release tag `cpp-v0.1.0`; fill in the archive `SHA512`
> in `vcpkg/portfile.cmake` when that tag is published.

## Build & test

```bash
cmake -B build -S .
cmake --build build
ctest --test-dir build
```

Tests are gated behind the `DEPZ_SENSOR_SDK_CPP_BUILD_TESTS` option (ON for a
standalone build, OFF when the project is pulled in via `add_subdirectory` /
FetchContent).

## License

MIT. Bundled/derived VL53L8 advanced-DCI codecs are © STMicroelectronics
(BSD-3-Clause).
