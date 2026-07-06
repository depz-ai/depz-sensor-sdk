# depz-sensor-sdk-c

Contract-first C11 SDK for the DEPZ USB sensor line: CRCs, packet framing, USB
identity, firmware container parsing, the common command/report codecs, and the
per-sensor **decode layer**. No dependencies beyond libm; byte-exact with the
`contracts/` docs and the golden vectors in `contracts/vectors/`.

This package is a **decode / codec layer only** — it has no object-oriented
sensor classes and does not drive live hardware. It turns bytes on the wire into
typed values (and packs the config/command payloads), and it deliberately marks
the hardware-dependent pieces as extension points instead of faking them.

## Documentation

Full docs live in [`docs/`](docs/):

- **[Guide](docs/guide.md)** — build with CMake, the transport/decode mental
  model, what's covered vs the hardware/CNH extension points.
- Per-sensor introduction + user guide + API reference:
  **SR04** ([intro](docs/sr04/introduction.md) ·
  [guide](docs/sr04/guide.md) · [api](docs/sr04/api.md)) ·
  **VL53L8CX** ([intro](docs/vl53l8cx/introduction.md) ·
  [guide](docs/vl53l8cx/guide.md) · [api](docs/vl53l8cx/api.md)) ·
  **VL53L8CH** ([intro](docs/vl53l8ch/introduction.md) ·
  [guide](docs/vl53l8ch/guide.md) · [api](docs/vl53l8ch/api.md)) ·
  **BNO086** ([intro](docs/bno086/introduction.md) ·
  [guide](docs/bno086/guide.md) · [api](docs/bno086/api.md)).
- **[API reference](docs/api.md)** — every public symbol, generated from the
  header's doc-comments by `scripts/gen_api_md.py` (`make docs`).

## The four sensors

The DEPZ line exposes **four** user-facing sensors. Note that the ToF is *two*
distinct sensors sharing one frame format:

| Sensor       | What it is                                | USB PID        | SDK coverage today |
|--------------|-------------------------------------------|----------------|--------------------|
| **SR04**     | Ultrasonic range finder                   | `0xEC78`       | Full codecs (commands, reports, echo→distance) |
| **VL53L8CX** | Base multi-zone ToF (4×4 / 8×8)           | dev-default\* | Shared frame decode + advanced DCI codecs |
| **VL53L8CH** | VL53L8CX **+ CNH** compact-network-histograms | `0xED40`   | Shared frame decode + advanced DCI codecs; **CNH histogram decode not yet implemented** |
| **BNO086**   | 9-axis IMU / sensor-hub (SH-2 over SHTP)  | `0xEE08`       | SHTP framing + SH-2 control encoders + input-report parsers |

\* **VL53L8CH** carries the production PID `0xED40`; **VL53L8CX** currently
enumerates on the dev/unprogrammed default id `0x0483:0x56DC` (no dedicated
production PID yet — contract 02 §4.2). Both report the same `VL53L8` firmware
identity name, so `depz_sensor_type` has a single `DEPZ_SENSOR_VL53L8`; the
CX/CH split is named by `depz_vl53l8_variant` (`DEPZ_VL53L8_VARIANT_CX` /
`DEPZ_VL53L8_VARIANT_CH`).

### VL53L8CX vs VL53L8CH — what is and isn't covered

* **Shared (implemented, verified):** the register-bridge streaming path is
  identical for both variants — RPT_FRAME chunk parse, frame reassembly, and the
  raw-results ranging-frame decoder (`depz_vl53l8_decode_frame`) — plus the
  advanced DCI codecs (xtalk margin, detection thresholds, motion config). A
  VL53L8CX (dev-default) capture (`vl53l8_8x8_15hz_3s.depzrec`,
  `software_name` `APP_VL53L8_v0.9`) is replayed end-to-end through this shared
  decode — the same path a CH device exercises — in the test suite.
* **CH-specific extension point (NOT implemented):** **CNH
  (compact-network-histogram) decode.** CNH frames are a distinct on-wire layout
  and are left as a named, documented gap in `src/vl53l8_decode.c` — no fake CH
  implementation is provided.
* **Hardware extension point (NOT implemented):** the live ULD init/config
  register-bridge driver (firmware download + DCI register sequences) is
  hardware-dependent and out of scope — see the TS/Python `uld` reference.

## Layout

```
include/depz_sensor_sdk.h   single public header
src/crc.c framing.c         transport: CRCs, packet framing
src/usb_ids.c identity.c    USB identity table + firmware-name parsing
src/common.c sr04.c         common codecs + SR04 codecs
src/fwdepz.c                .fwdepz firmware container parse/validate
src/vl53l8_decode.c         VL53L8CX/CH shared frame reassembly + decode
src/vl53l8_advanced.c       VL53L8CX/CH shared advanced DCI codecs
src/bno086_shtp.c           BNO086 SHTP framing + SH-2 control encoders
src/bno086_reports.c        BNO086 SH-2 input-report parsers
src/dataset.c               .depzdata dataset reader
tests/                      golden-vector + replay test runner
```

## Build & test

```sh
cmake -B build -S .
cmake --build build
ctest --test-dir build
```

The suite is one CTest per consumed vector / replay target: the
transport+protocol foundation plus the sensor-decode layer across the four
sensors.

## Install

The library exports a namespaced target **`depz::sensor_sdk_c`** and ships an
installable CMake package (`find_package(depz-sensor-sdk-c CONFIG)`), a Conan 2
recipe, and a vcpkg port. Version **0.1.0**, MIT.

### CMake FetchContent

Because the SDK lives in a subdirectory of the monorepo, point `SOURCE_SUBDIR`
at it. Pulling us in this way builds the library only — never our test suite.

```cmake
include(FetchContent)
FetchContent_Declare(
  depz_sensor_sdk_c
  GIT_REPOSITORY https://github.com/depz-ai/depz-sensor-sdk.git
  GIT_TAG        v0.1.0
  SOURCE_SUBDIR  packages/depz-sensor-sdk-c
)
FetchContent_MakeAvailable(depz_sensor_sdk_c)

target_link_libraries(my_app PRIVATE depz::sensor_sdk_c)
```

### CMake find_package (after install)

```sh
cmake -B build -S packages/depz-sensor-sdk-c -DDEPZ_SENSOR_SDK_C_BUILD_TESTS=OFF
cmake --build build
cmake --install build --prefix /your/prefix
```

```cmake
find_package(depz-sensor-sdk-c 0.1.0 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE depz::sensor_sdk_c)
```

Then in your source:

```c
#include <depz_sensor_sdk.h>

const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
uint8_t crc = depz_crc8_maxim(data, sizeof data);
```

### Conan 2

```sh
conan create packages/depz-sensor-sdk-c            # build & test-package locally
```

In your consumer's `conanfile.txt`: `[requires]` → `depz-sensor-sdk-c/0.1.0`.
The recipe sets the CMake target name to `depz::sensor_sdk_c` for CMakeDeps
consumers.

### vcpkg

Add `depz-sensor-sdk-c` to your manifest (`vcpkg.json`) or install directly:

```sh
vcpkg install depz-sensor-sdk-c
```

The port (`vcpkg/`) fetches the `v0.1.0` tag; the `SHA512` in
`vcpkg/portfile.cmake` is a `0` placeholder to be filled at release (the first
`vcpkg install` prints the correct hash).

### Build options

| Option | Default | Effect |
|--------|---------|--------|
| `DEPZ_SENSOR_SDK_C_BUILD_TESTS` | `ON` standalone / `OFF` as a subproject | Build the golden-vector CTest suite (needs the monorepo's `contracts/vectors/`). |
