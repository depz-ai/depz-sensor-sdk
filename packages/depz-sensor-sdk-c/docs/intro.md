# C SDK

`depz-sensor-sdk-c` is a **contract-first C11 SDK** for the DEPZ USB sensor
line, byte-exact with the protocol contracts and the shared golden vectors. It
comes in two layers, one header each:

- **`include/depz_sensor_sdk.h` — codecs.** Turns the sensors' byte stream into
  typed C values and packs the command/config payloads that go the other way.
  Pure functions over your buffers, no dependencies beyond libm.
- **`include/depz_sensor_io.h` — live hardware.** Opens the serial port, runs a
  reader thread, correlates requests with replies, delivers measurements as
  callbacks or bounded streams, finds boards by USB id, and records / replays
  sessions as `.depzrec` files. Linux, macOS and Windows.

```c
#include <depz_sensor_io.h>   /* also includes depz_sensor_sdk.h */

depz_device *dev;
if (depz_open_device(NULL, &dev) != DEPZ_OK)        /* the first DEPZ board */
    fprintf(stderr, "open: %s\n", depz_last_error());
```

Live-layer conventions, in short (details in the [guide](guide.md#live-layer)):

- Calls return `0` (`DEPZ_OK`) or a negative `depz_err`; after a failure
  `depz_last_error()` has the message and `depz_last_status()` the device's
  status code — both per calling thread.
- Timeouts are milliseconds; a negative timeout means the default (200 ms,
  `depz_device_set_timeout_ms()` changes it).
- Callbacks run on the device's reader thread: keep them short, never block
  in them, never call `depz_device_close()` from one. Streams are bounded and
  drop the oldest item when full (`depz_stream_dropped_count()`).
- A device owns its link; `depz_device_close()` closes and frees both.

**Working with one specific sensor?** Each has its own introduction, guide
and API reference:

- **[SR04](sr04/introduction.md)** — ultrasonic ranging: one distance per ping.
- **[VL53L4CD](vl53l4cd/introduction.md)** — single-zone ToF: one precise
  optical distance.
- **[VL53L8CX](vl53l8cx/introduction.md)** — multizone ToF: 4×4 / 8×8 depth
  frames.
- **[VL53L8CH](vl53l8ch/introduction.md)** — the CX superset with CNH
  histograms.
- **[VL53L5CX](vl53l5cx/introduction.md)** · **[VL53L7CX](vl53l7cx/introduction.md)**
  · **[VL53L7CH](vl53l7ch/introduction.md)** — 8×8 ToF on the I2C board
  (63° / 90° / 90° + CNH).
- **[VL53L0X](vl53l0x/introduction.md)** · **[VL53L1CX](vl53l1cx/introduction.md)**
  · **[VL53L1CB](vl53l1cb/introduction.md)** · **[VL53L3CX](vl53l3cx/introduction.md)**
  · **[VL53L4CX](vl53l4cx/introduction.md)** — the VL53L 1D family (one live class): single-zone
  and multi-target ToF on one bridge firmware.
- **[BNO086](bno086/introduction.md)** — BNO085 / BNO086 9-axis IMU: orientation
  and motion outputs, each at its own rate.
- **[BNO055](bno055/introduction.md)** — 9-axis IMU with on-chip fusion:
  orientation, gravity and linear acceleration at 100 Hz.

Every board opens live and answers the common commands (names, serial, MCU
temperature, time sync, sync pins). Full sensor classes arrive one sensor at a
time: the **SR04** (configure, single shots, a streaming loop), the
**VL53L4CD** (the ST ULD on the host: init, timing, offset/xtalk, thresholds,
calibration, single shots, an INT-driven stream) and the **VL53L8CX /
VL53L8CH** (the ST ULD on the host with the sensor-firmware download:
resolution, frequency, power modes, crosstalk, thresholds, motion, CNH
histograms on the CH, an INT-driven frame stream) have one today — the same
multizone class also drives the **VL53L5CX / VL53L7CX / VL53L7CH** I2C boards
— and so has the **BNO055** (mode, units, axis remap and calibration through
CONFIG, status and self-test, page-1 configs and motion interrupts, scaled
samples polled or streamed; verified on replays of real captures and live on a
lab board) and the **BNO085 / BNO086** (the SH-2 sensor-hub protocol on the
host: enable outputs at a rate, scaled reports, calibration and tare, FRS
records; verified live on a lab BNO085 and on a replay of a real capture), and
the **VL53L 1D ToF family** — VL53L0X / L1CX / L1CB / L3CX / L4CX — has one
class for all five boards (every ST driver of the family on the host: the
VL53L0X API, the VL53L1X / VL53L4CD ULDs, the VL53L3CX ULP and the histogram
driver with up to four targets; verified on replays of all ten real captures
and live on a lab VL53L4CX). Every board of the line now has its class.

## Build & install

C11. The codec layer needs only libm; the live layer adds threads and the OS
port-enumeration API (`setupapi`/`cfgmgr32` on Windows, IOKit/CoreFoundation on
macOS) — linked for you through the CMake target. Build the static library and
run the suite (83 tests, no hardware needed) with CMake:

```sh
cmake -B build -S .
cmake --build build
ctest --test-dir build
```

Consume it from your own CMake project with FetchContent — this builds the
library only, never the test suite — and link the namespaced target
`depz::sensor_sdk_c`:

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

The library also ships an installable CMake package
(`find_package(depz-sensor-sdk-c CONFIG)`) plus Conan and vcpkg recipes — see
the [README](../README.md#install).

```c
#include <depz_sensor_sdk.h>   /* codecs only */
#include <depz_sensor_io.h>    /* live layer (includes the codecs) */
```

`-DDEPZ_SENSOR_SDK_C_BUILD_IO=OFF` builds the codec layer alone, with no OS
dependencies. Two runnable examples ship in `examples/`: `depz_list` (every
serial port and every DEPZ board found) and `sr04_minimal` (live SR04
distance).

## Where to next

- **Your sensor's pages** — [SR04](sr04/introduction.md) ·
  [VL53L4CD](vl53l4cd/introduction.md) · [VL53L8CX](vl53l8cx/introduction.md) ·
  [VL53L8CH](vl53l8ch/introduction.md) · [VL53L5CX](vl53l5cx/introduction.md) ·
  [VL53L7CX](vl53l7cx/introduction.md) · [VL53L7CH](vl53l7ch/introduction.md) ·
  [VL53L0X](vl53l0x/introduction.md) · [VL53L1CX](vl53l1cx/introduction.md) ·
  [VL53L1CB](vl53l1cb/introduction.md) · [VL53L3CX](vl53l3cx/introduction.md) ·
  [VL53L4CX](vl53l4cx/introduction.md) · [BNO086](bno086/introduction.md) ·
  [BNO055](bno055/introduction.md): introduction, hands-on guide, and the
  sensor's own API reference.
- **[Guide](guide.md)** — the SDK-wide walkthrough: build, the live layer
  (devices, errors, threading, streams, discovery, record/replay), the
  transport/decode mental model, per-sensor decode.
- **[API Reference](api.md)** — the whole public surface, generated from the
  two headers' doc-comments.
- **Docs for LLMs** — the sensor SDK documentation as raw Markdown:
  [/llms-full.txt](/llms-full.txt).
- **Source & license** — the SDKs are open source (MIT):
  [github.com/depz-ai/depz-sensor-sdk](https://github.com/depz-ai/depz-sensor-sdk).
