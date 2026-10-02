# depz-sensor-sdk-c

Contract-first C11 SDK for the DEPZ USB sensor line, byte-exact with the
`contracts/` docs and the golden vectors in `contracts/vectors/`. It has two
layers, one header each:

- **Codecs — `depz_sensor_sdk.h`.** CRCs, packet framing, USB identity,
  firmware container parsing, the common command/report codecs and the
  per-sensor **decode layer**: pure functions over buffers you supply, no
  dependencies beyond libm.
- **Live hardware — `depz_sensor_io.h`.** Serial ports, `.depzrec`
  record/replay, the device core (reader thread, request/response
  correlation, events, bounded drop-oldest streams, the common commands),
  discovery, and the sensor classes. Linux, macOS and Windows.

The live layer drives every board's **common commands** (names, serial, MCU
temperature, time sync, payload CRC, sync pins, reset) and hands out
**sensor classes one sensor at a time — SR04, VL53L4CD, the multizone
ToF family (VL53L8CX / VL53L8CH, VL53L5CX / VL53L7CX / VL53L7CH), the
VL53L 1D ToF family (VL53L0X / L1CX / L1CB / L3CX / L4CX), the BNO055 and the
BNO085 / BNO086 — every board of the line** (the ToF classes run the ST
drivers on the host, over the board's register bridge — for the multizone
sensors the ~84 KB sensor-firmware download included, for the 1D family the
VL53L0X API, the VL53L1X / VL53L4CD ULDs, the VL53L3CX ULP and ST's
histogram driver; the BNO055 class does the mode, unit, calibration and page
logic over its register bridge; the BNO085 / BNO086 class runs the SH-2
sensor-hub protocol over the board's SHTP pass-through).

```c
#include <depz_sensor_io.h>

depz_device *dev;
depz_sr04_measurement m;
double mm;
if (depz_open_device(NULL, &dev) == DEPZ_OK) {       /* first DEPZ board */
    if (depz_is_sr04(dev) && depz_sr04_measure_once(dev, -1, &m) == DEPZ_OK &&
        depz_sr04_measurement_distance_mm(&m, &mm))
        printf("%.1f mm\n", mm);
    depz_device_close(dev);
} else {
    fprintf(stderr, "%s\n", depz_last_error());
}
```

Live-layer conventions, in short (details in the [guide](docs/guide.md#live-layer)):

- Calls return `0` (`DEPZ_OK`) or a negative `depz_err`; after a failure
  `depz_last_error()` has the message and `depz_last_status()` the device's
  status code — both per calling thread.
- Timeouts are milliseconds; a negative timeout means the default (200 ms,
  `depz_device_set_timeout_ms()` changes it).
- Callbacks run on the device's reader thread: keep them short, never block
  in them, never call `depz_device_close()` from one. Streams are bounded and
  drop the oldest item when full (`depz_stream_dropped_count()`).
- A device owns its link; `depz_device_close()` closes and frees both.

## Documentation

Full docs live in [`docs/`](docs/):

- **[Guide](docs/guide.md)** — build with CMake, the live layer (devices,
  streams, discovery, record/replay), the transport/decode mental model and the
  per-sensor decode layers.
- Per-sensor introduction + user guide + API reference:
  **SR04** ([intro](docs/sr04/introduction.md) ·
  [guide](docs/sr04/guide.md) · [api](docs/sr04/api.md)) ·
  **VL53L4CD** ([intro](docs/vl53l4cd/introduction.md) ·
  [guide](docs/vl53l4cd/guide.md) · [api](docs/vl53l4cd/api.md)) ·
  **VL53L8CX** ([intro](docs/vl53l8cx/introduction.md) ·
  [guide](docs/vl53l8cx/guide.md) · [api](docs/vl53l8cx/api.md)) ·
  **VL53L8CH** ([intro](docs/vl53l8ch/introduction.md) ·
  [guide](docs/vl53l8ch/guide.md) · [api](docs/vl53l8ch/api.md)) ·
  **VL53L5CX** ([intro](docs/vl53l5cx/introduction.md) ·
  [guide](docs/vl53l5cx/guide.md) · [api](docs/vl53l5cx/api.md)) ·
  **VL53L7CX** ([intro](docs/vl53l7cx/introduction.md) ·
  [guide](docs/vl53l7cx/guide.md) · [api](docs/vl53l7cx/api.md)) ·
  **VL53L7CH** ([intro](docs/vl53l7ch/introduction.md) ·
  [guide](docs/vl53l7ch/guide.md) · [api](docs/vl53l7ch/api.md)) ·
  **BNO055** ([intro](docs/bno055/introduction.md) ·
  [guide](docs/bno055/guide.md) · [api](docs/bno055/api.md)) — the ones
  with a live sensor class; every other board has the same three pages under
  [`docs/`](docs/intro.md):
  **VL53L0X** ([intro](docs/vl53l0x/introduction.md) ·
  [guide](docs/vl53l0x/guide.md) · [api](docs/vl53l0x/api.md)) ·
  **VL53L1CX** ([intro](docs/vl53l1cx/introduction.md) ·
  [guide](docs/vl53l1cx/guide.md) · [api](docs/vl53l1cx/api.md)) ·
  **VL53L1CB** ([intro](docs/vl53l1cb/introduction.md) ·
  [guide](docs/vl53l1cb/guide.md) · [api](docs/vl53l1cb/api.md)) ·
  **VL53L3CX** ([intro](docs/vl53l3cx/introduction.md) ·
  [guide](docs/vl53l3cx/guide.md) · [api](docs/vl53l3cx/api.md)) ·
  **VL53L4CX** ([intro](docs/vl53l4cx/introduction.md) ·
  [guide](docs/vl53l4cx/guide.md) · [api](docs/vl53l4cx/api.md)) ·
  **BNO086** ([intro](docs/bno086/introduction.md) ·
  [guide](docs/bno086/guide.md) · [api](docs/bno086/api.md)).
- **[API reference](docs/api.md)** — every public symbol, generated from the
  two headers' doc-comments by `scripts/gen_api_md.py` (`make docs`).

## The sensors

Note that VL53L8 is *two* distinct sensors sharing one frame format, and that
the L5/L7 boards and the VL53L 1D family each share one firmware:

| Sensor       | What it is                                | USB PID        | SDK coverage today |
|--------------|-------------------------------------------|----------------|--------------------|
| **SR04**     | Ultrasonic range finder                   | `0xEC78`       | Full codecs + live sensor class (`depz_sr04_*`: config, single shots, loop via callbacks / streams) |
| **VL53L4CD** | Single-zone ToF                           | `0xED45`       | Register-bridge codecs + host-ULD math + live sensor class (`depz_vl53l4cd_*`: ULD init, timing / offset / xtalk / thresholds, calibration, single shots, INT-driven stream via callbacks / streams) |
| **VL53L8CX** | Base multi-zone ToF (4×4 / 8×8)           | dev-default\* | Shared frame decode + advanced DCI codecs + live sensor class (`depz_vl53l8_*`: ULD init with the sensor-firmware download, resolution / frequency / mode / sharpener / target order, power modes, crosstalk margin / calibration / caldata, detection thresholds, motion indicator, INT-driven frame stream via callbacks / streams) |
| **VL53L8CH** | VL53L8CX **+ CNH** compact-network-histograms | `0xED40`   | Everything VL53L8CX has (the same class, CH firmware blob) + CNH setup (`depz_vl53l8_configure_cnh`) + CNH histogram decode |
| **VL53L5CX** / **VL53L7CX** / **VL53L7CH** | 8×8 ToF on an I2C bridge (63° / 90° / 90° + CNH) | `0xED48` / `0xED49` / `0xED4A` | Bridge codecs, class resolution, L5/L7 frame decode (+ CNH block on L7CH) + the live multizone class (`depz_vl53l8_*`, models `L5CX` / `L7CX` / `L7CH`: everything VL53L8CX has — no deep sleep / threshold auto-stop on L5CX / L7CX, CNH on L7CH, 1 Hz allowed) + board commands (`depz_vl53l7_bridge_info` / `_set_i2c_speed_khz` / `_pin_ctrl`) |
| **VL53L0X** / **VL53L1CX** / **VL53L1CB** / **VL53L3CX** / **VL53L4CX** | VL53L 1D ToF family on one I2C bridge | `0xED41` / `0xED43` / `0xED42` / `0xED44` / `0xED46` | Bridge codecs, product table + class resolution, die / VL53L0X / histogram block decoders + live sensor class (`depz_vl53lx_*`: init with a (product, driver) pair — VL53L0X API, VL53L1X / VL53L4CD ULDs, VL53L3CX ULP, histogram driver with up to four targets and the 24 bins —, configure, modes and budgets, calibrations, thresholds, ROI, INT-driven stream via callbacks / streams, single shots) |
| **BNO086** / **BNO085** | 9-axis IMU / sensor-hub (SH-2 over SHTP) | `0xEE08` / `0xEE09` | Bridge + SHTP framing, SH-2 encoders and answer parsers, input-report parsers and scaling + live sensor class (`depz_bno086_*`: reset / product id, enable with the granted-rate read-back, reports via callbacks / streams, calibration / DCD / tare / reorientation, FRS read / write and sensor metadata, error queue / counts, busy retry) |
| **BNO055**   | 9-axis IMU, fusion on the chip (register bridge) | `0xEE0A` | Bridge codecs, units / calibration / axis-remap / page-1 config codecs, register-window decode + live sensor class (`depz_bno055_*`: configure through CONFIG with the boot and fusion-start waits, operating / power mode, units, axis remap and placements, calibration status / profile / soft-iron matrix, system status and self-test, page-1 configs / unique id / motion interrupts, scaled samples polled or streamed via callbacks / streams) |

Every board in the table also opens live through `depz_sensor_io.h` —
discovery, the common commands and time sync work on all of them; the SR04,
the VL53L4CD, the multizone ToF boards (VL53L8CX / CH, VL53L5CX / L7CX / L7CH)
the BNO055 and the BNO085 / BNO086 have their sensor class so far.

\* **VL53L8CH** carries the production PID `0xED40`; **VL53L8CX** currently
enumerates on the dev/unprogrammed default id `0x0483:0x56DC` (no dedicated
production PID yet — contract 02 §4.2). Both report the same `VL53L8` firmware
identity name, so `depz_sensor_type` has a single `DEPZ_SENSOR_VL53L8`. The
live class picks its model from the USB PID (`0xED40` → `DEPZ_VL53L8_MODEL_L8CH`,
anything else → `DEPZ_VL53L8_MODEL_L8CX`; `depz_vl53l8_open_link()` names it
by hand); in the decode layer the split is named by `depz_vl53l8_variant`
(`DEPZ_VL53L8_VARIANT_CX` / `DEPZ_VL53L8_VARIANT_CH`). The L5/L7 boards share
the `APP_VL53L7` firmware (sensor type `vl53l7`); the same class resolves them
by USB PID (`0xED48` L5CX, `0xED49` L7CX, `0xED4A` L7CH), then the device
name, then L7CX.

### VL53L5CX / VL53L7CX / VL53L7CH

The L5/L7 boards run the **same live class** (`depz_vl53l8_*`) with the
models `DEPZ_VL53L8_MODEL_L5CX` / `_L7CX` / `_L7CH`, over the I2C bridge
(reads split at 1536 B internally): `init()` downloads the L5/L7 sensor
firmware (ULD 2.0.1; the VL53LMZ blob on the L7CH) in ~1.3 s, and
`depz_vl53l8_module_type()` then tells L5 (0, MZ) from L7 (1, MZEVO). 1 Hz
ranging works; the L5CX / L7CX firmware has no deep sleep and no threshold
auto-stop; the L7CH has CNH as the VL53L8CH. The bridge's own commands are
`depz_vl53l7_bridge_info()`, `depz_vl53l7_set_i2c_speed_khz()` and
`depz_vl53l7_pin_ctrl()`. Four real captures (VL53L5CX 8×8 and 4×4, VL53L7CH
with and without CNH) replay strictly through the class, and it was run on a
real VL53L7CH (see *Build & test*); a VL53L5CX / L7CX board has not been run
live in C yet.

### VL53L8CX vs VL53L8CH — what is and isn't covered

* **Shared live class (implemented, verified):** one `depz_vl53l8_*` class
  runs the ST ULD on the host for both models, ported register for register
  from the Python SDK: `init()` boots the sensor, downloads the model's ~84 KB
  firmware (CX = ULD 2.1.0, CH = VL53LMZ ULD 2.0.16), checks its checksum and
  uploads the NVM offset, default crosstalk and configuration; then the
  configuration, the advanced features and the INT-driven frame stream. The
  firmware and configuration blobs are generated into `src/io/vl53l8_blobs.c`
  from the Python SDK's data by `scripts/gen_vl53l8_blobs.py` (CI checks it is
  current with `--check`).
* **Shared decode:** RPT_FRAME chunk parse, frame reassembly, the
  raw-results ranging-frame decoder (`depz_vl53l8_decode_frame` for CX;
  `depz_vl53l8ch_decode_frame` for CH, whose VL53LMZ footer id sits at size−4)
  and the advanced DCI codecs (xtalk margin, detection thresholds, motion
  config).
* **CH-specific: CNH (compact-network-histogram).** The setup helpers and
  `depz_vl53l8_configure_cnh()` arm it; every streamed frame then carries the
  CNH block, and `depz_vl53l8ch_decode_cnh()` decodes it into per-aggregate
  histograms, checked against a golden vector.
* **Replayed and live-checked:** a real VL53L8CX capture (`vl53l8_8x8_15hz_3s`,
  `APP_VL53L8_v0.9`) and two real VL53L8CH captures (with and without CNH,
  `APP_VL53L8_v0.92`) replay strictly through the class, firmware download
  included; the class was run on a real VL53L8CH (see *Build & test*).

### BNO055

The BNO055 fuses on the chip, so its class is register logic over the
bridge, with the same request sequences as the Python SDK's `Bno055`:
`depz_bno055_configure()` (CONFIG → units → optional axis remap and
calibration profile → the operating mode, waiting for the sensor to boot and
for the fusion to start), mode switches through CONFIG with the datasheet
times, power mode, units, axis remap and placements, calibration status,
profile and soft-iron matrix, system status and self-test, the page-1 sensor
configs, unique id and motion interrupts (page 1 refused while streaming,
always back to page 0), and samples scaled by the stream's units — polled
(`depz_bno055_read_sample()`) or streamed on the bridge's timer as callbacks
or streams, each channel flagged when the block covered it. Both committed
BNO055 captures (NDOF full block at 100 Hz, IMU quaternion at 50 Hz with every
unit switched) replay strictly through the class; it is also **verified live**
on a lab board: NDOF at 100 Hz, no drops, orientation and gravity follow the
board when it is turned.

### BNO085 / BNO086

The **BNO085 / BNO086 sensor class** (`depz_bno086_*`) runs the SH-2
sensor-hub protocol on the host, request for request as the Python SDK:
hardware reset and product id (BNO085 or BNO086), enabling any SH-2 output
with the granted rate read back (re-asked while the hub still answers 0),
reports scaled by helpers and delivered as callbacks or streams, ME
calibration / DCD, tare and reorientation, FRS records and sensor metadata,
the error queue and counts, and the bridge's busy back-off. The committed
BNO085 capture (Python SDK) replays strictly through the class; it is also
**verified live** on a lab BNO085: rotation vector 100 Hz, game rotation
vector 200 Hz, gravity |g| 9.85 m/s², no drops.

### The 1D family

The **1D-family sensor class** (`depz_vl53lx_*`) runs every ST driver the
Python SDK has for these boards, request for request: the VL53L0X API 1.0.4,
the VL53L4CD ULD 2.2.3 (also borrowed by a VL53L4CX), the VL53L3CX ULP 1.0.0,
the VL53L1X ULD 3.5.5 and ST's histogram (Bare) driver with its 24-bin
post-processing into up to four targets. `depz_vl53lx_init()` binds a
(product, driver) pair — the product defaults to the one the board's name
carries, and naming a sibling borrows its driver — and `configure()`
re-initialises and applies budget, mode and a stored calibration before every
run. All ten committed 1D captures (Python SDK) replay strictly through the
class; it is also **verified live** on a lab VL53L4CX against a wall at
0.600 m: histogram 606 / 603 mm (medium / long), the borrowed light drivers
615 / 620 mm uncalibrated and 601 mm after an offset calibration from C.

## Layout

```
include/depz_sensor_sdk.h   codec / decode layer (pure functions)
include/depz_sensor_io.h    live-hardware layer (links, devices, discovery, SR04,
                            VL53L4CD, the multizone class: VL53L8CX / CH,
                            VL53L5CX / L7CX / L7CH, the VL53L 1D family,
                            BNO055, BNO085 / BNO086)
src/crc.c framing.c         transport: CRCs, packet framing
src/usb_ids.c identity.c    USB identity table + firmware-name parsing
src/common.c sr04.c         common codecs + SR04 codecs
src/fwdepz.c                .fwdepz firmware container parse/validate
src/vl53l8_decode.c         VL53L8CX/CH shared frame reassembly + decode
src/vl53l8_advanced.c       VL53L8CX/CH shared advanced DCI codecs
src/vl53l4.c                VL53L4CD register bridge + host-ULD codecs
src/vl53l7.c                VL53L5CX/L7CX/L7CH bridge codecs + frame decode
src/vl53lx.c                VL53L 1D family codecs, product table, block decode
src/bno055.c                BNO055 register-bridge + register codecs
src/bno086_shtp.c           BNO086 SHTP framing + SH-2 control encoders
src/bno086_reports.c        BNO086 SH-2 input-report parsers
src/bno086_sh2.c            BNO086 SH-2 answer parsers, metadata, scaling
src/dataset.c               .depzdata dataset reader
src/io/                     live layer: serial ports (POSIX / Win32), port
                            enumeration (Linux / macOS / Windows), links,
                            device core, discovery, SR04 class, VL53L4CD and
                            multizone classes (host ULD over the register
                            bridges, SPI and I2C), the 1D-family class and its
                            drivers (vl53lx_*.c; the histogram driver's tables
                            vl53lx_bare_tables.c, generated), BNO055 class,
                            BNO085 / BNO086 class,
                            vl53l8_blobs.c (sensor firmware + config, generated)
scripts/                    gen_api_md.py (docs/*api.md), gen_vl53l8_blobs.py
                            (src/io/vl53l8_blobs.c from the Python SDK's data),
                            gen_vl53lx_bare_tables.py (src/io/vl53lx_bare_tables.*
                            from the Python SDK's histogram driver)
examples/                   depz_list.c (ports + DEPZ boards), sr04_minimal.c,
                            bno086_minimal.c (orientation as heading / pitch / roll),
                            vl53lx_minimal.c (any 1D board: distance and targets)
tests/                      golden-vector + replay runner, live-layer runner,
                            fake SR04 / fake VL53L4CD bridge firmware, fake
                            BNO086 hub
```

## Build & test

```sh
cmake -B build -S .
cmake --build build
ctest --test-dir build
```

The suite is 83 CTests, no hardware needed: one per consumed vector / replay
target (the transport+protocol foundation plus the sensor-decode layer, 24),
and the live layer against a fake SR04 and a fake VL53L4CD bridge (with a
register map) over an in-memory loopback link, the VL53L8 state guards and CNH
setup math, plus strict replays of a real SR04, a real VL53L4CD and three real
VL53L8 captures — VL53L8CX, VL53L8CH, VL53L8CH with CNH, each with the whole
firmware download — four VL53L5CX / VL53L7CH captures through the same
class, two real BNO055 captures through the BNO055 class, and a real BNO085
capture plus a fake SH-2 hub through the BNO085 / BNO086 class, and the ten
VL53L 1D-family captures through the 1D class (`io_*`, 59). CI
builds and runs it on Linux, macOS and Windows.

The examples build next to the library (`build/depz_list`,
`build/sr04_minimal [port] [seconds]`, `build/bno086_minimal [port] [seconds]`,
`build/vl53lx_minimal [port] [seconds]`). The live layer was also checked on a
real SR04 (firmware `APP_usonic_SR04_v0.97`): discovery, the common commands,
the echo-decay clamp (100 → 4000, 65535 → 65000), single shots, `ERR_BUSY` for
a single shot while the loop runs, and 50 samples/s with none dropped. The
VL53L4CD class was checked on a real board (`APP_VL53L4_v0.83`, wall at
~1.05 m): model id `0xEBAA`, init in 60 ms, timing / offset / threshold round
trips, the temperature update, single shots of ~1043 mm (valid), and a stream
of 33 frames/s at a 33 ms budget with none dropped or undecoded. The VL53L8
class was checked on a real VL53L8CH (`APP_VL53L8_v0.92`, wall at ~0.6 m):
`init()` with the firmware download in 0.76 s, 8×8 at 15 Hz (~599–602 mm, 30
frames in 2 s, none dropped), a 1708-byte CNH block in every frame decoding
fine, 4×4 at 30 frames/s, sleep and wake-up, configuration refused while
ranging and 1 Hz refused. On a real VL53L7CH (`APP_VL53L7_v0.53`, wall at
~0.6 m) the same class resolved the L7CH model (module type MZEVO), ran
`init()` in 1.29 s, ranged 8×8 at 15 Hz (~602 mm) and 4×4 at 1 Hz, streamed a
1708-byte CNH block per frame, snapped the bus from 450 to 500 kHz, survived a
soft cycle and re-init, with no I2C errors. The BNO055 class, live on the lab
board, self-tested, configured NDOF and streamed 100 Hz for 60 s with no drops
while the board was turned, tilted and shaken by hand.

## Install

The library (`libdepz_sensor_sdk_c.a`, `depz_sensor_sdk_c.lib` with MSVC)
exports a namespaced target **`depz::sensor_sdk_c`** and ships an
installable CMake package (`find_package(depz-sensor-sdk-c CONFIG)`), a Conan 2
recipe, and a vcpkg port. Version **0.4.0**, MIT.

### CMake FetchContent

Because the SDK lives in a subdirectory of the repository, point `SOURCE_SUBDIR`
at it. Pulling us in this way builds the library only — never our test suite.

```cmake
include(FetchContent)
FetchContent_Declare(
  depz_sensor_sdk_c
  GIT_REPOSITORY https://github.com/depz-ai/depz-sensor-sdk.git
  GIT_TAG        v0.4.0
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
find_package(depz-sensor-sdk-c 0.4.0 CONFIG REQUIRED)
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

In your consumer's `conanfile.txt`: `[requires]` → `depz-sensor-sdk-c/0.4.0`.
The recipe sets the CMake target name to `depz::sensor_sdk_c` for CMakeDeps
consumers. Submission to Conan Center is in review
([conan-center-index#30564](https://github.com/conan-io/conan-center-index/pull/30564));
until it merges, consume the in-repo recipe as above.

### vcpkg

Submission to the vcpkg registry is in review
([microsoft/vcpkg#52770](https://github.com/microsoft/vcpkg/pull/52770)); until
it merges, use the in-repo port (`vcpkg/`) as an overlay:

```sh
vcpkg install depz-sensor-sdk-c --overlay-ports=packages/depz-sensor-sdk-c/vcpkg
```

The port fetches the `v0.4.0` tag; the `SHA512` in `vcpkg/portfile.cmake` is a
`0` placeholder to be filled at release (the first `vcpkg install` prints the
correct hash).

### Build options

| Option | Default | Effect |
|--------|---------|--------|
| `DEPZ_SENSOR_SDK_C_BUILD_IO` | `ON` | Build the live-hardware layer (`depz_sensor_io.h`, `src/io/`). `OFF` leaves the pure codec library with no OS dependencies. |
| `DEPZ_SENSOR_SDK_C_BUILD_EXAMPLES` | `ON` standalone / `OFF` as a subproject | Build `depz_list` and `sr04_minimal` (needs `BUILD_IO`). |
| `DEPZ_SENSOR_SDK_C_BUILD_TESTS` | `ON` standalone / `OFF` as a subproject | Build the golden-vector CTest suite and, with `BUILD_IO`, the live-layer tests (needs the golden-vector tree from the development checkout; auto-skipped when absent). |

With `BUILD_IO` the target carries its OS link dependencies as `PUBLIC`, so
`depz::sensor_sdk_c` consumers get them automatically: `Threads::Threads` on
Linux and macOS, plus `-framework IOKit -framework CoreFoundation` on macOS,
and `setupapi` + `cfgmgr32` on Windows. `libm` is linked where it exists.

## License

MIT. Open source — source and issue tracker:
<https://github.com/depz-ai/depz-sensor-sdk>.
