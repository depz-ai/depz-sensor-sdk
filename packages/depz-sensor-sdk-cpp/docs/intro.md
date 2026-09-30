# C++ SDK

`depz-sensor-sdk-cpp` is the **C++17 SDK** for the DEPZ USB sensor line, in
two layers:

- **Live hardware** (`depz/device.hpp`) — open a board by USB identity, run
  its commands, get its data as callbacks or pull streams, record a session
  and replay it in tests. Every board opens as a `depz::Device` with the
  common commands; the sensor classes arrive one at a time — so far
  **SR04** (`depz::Sr04`), **VL53L4CD** (`depz::Vl53l4cd`), the multizone
  ToF (`depz::Vl53l8`: VL53L8CX / CH, VL53L5CX, VL53L7CX / CH) and the
  **BNO055** (`depz::Bno055`). It wraps the
  C SDK (`depz-sensor-sdk-c`); errors are exceptions.
- **The decode layer** — pure codecs, no I/O: bytes in, typed values out, and
  typed values back into bytes, byte-for-byte identical to the Python /
  TypeScript / Java reference SDKs via the shared golden test vectors. Use it
  on bytes from anywhere — your own port loop, a recording, a socket.

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main() {
    auto sr04 = depz::open_sr04();          // find, probe and open the SR04
    sr04->set_sample_period_us(20'000);     // 50 Hz ceiling
    auto samples = sr04->stream();          // a queue this thread pulls
    sr04->start();
    for (int i = 0; i < 50; i++)
        if (auto m = samples.next())
            std::printf("%.0f mm\n", m->distance_mm().value_or(-1));
    sr04->stop();
}
```

**Working with one specific sensor?** Each has its own introduction, guide
and API reference:

- **[SR04](sr04/introduction.md)** — ultrasonic ranging: one distance per ping.
  Full live class (`depz::Sr04`) and codecs.
- **[VL53L4CD](vl53l4cd/introduction.md)** — single-zone ToF: one precise
  optical distance per measurement. Full live class (`depz::Vl53l4cd`, the ST
  ULD on the host) and codecs.
- **[VL53L8CX](vl53l8cx/introduction.md)** — 8×8 ToF depth frames. Full live
  class (`depz::Vl53l8`, the ST ULD on the host, firmware download included)
  and codecs.
- **[VL53L8CH](vl53l8ch/introduction.md)** — the CX superset with CNH
  histograms: the same live class, plus CNH streaming and decode.
- **[VL53L5CX](vl53l5cx/introduction.md)** · **[VL53L7CX](vl53l7cx/introduction.md)**
  · **[VL53L7CH](vl53l7ch/introduction.md)** — 8×8 ToF on the I2C board
  (63° / 90° / 90° + CNH). The same live class (`depz::Vl53l8`) and codecs.
- **[VL53L0X](vl53l0x/introduction.md)** · **[VL53L1CX](vl53l1cx/introduction.md)**
  · **[VL53L1CB](vl53l1cb/introduction.md)** · **[VL53L3CX](vl53l3cx/introduction.md)**
  · **[VL53L4CX](vl53l4cx/introduction.md)** — the VL53L 1D family: single-zone
  and multi-target ToF on one bridge firmware. One live class
  (`depz::Vl53lx`: every ST driver of the family, up to four targets on the
  histogram driver; verified on replays of real captures) and codecs.
- **[BNO086](bno086/introduction.md)** — BNO085 / BNO086 9-axis IMU:
  orientation and motion outputs, each at its own rate. Full live class
  (`depz::Bno086`: enable outputs, scaled reports, calibration and tare, FRS
  records; verified on a replay of a real capture and live on a lab BNO085)
  and codecs.
- **[BNO055](bno055/introduction.md)** — 9-axis IMU with on-chip fusion.
  Full live class (`depz::Bno055`: configuration through CONFIG, calibration,
  scaled sample stream; verified on replays of real captures and live on a
  lab board) and codecs.

Every board of the line has its live class.

## Build & link

```bash
cmake -B build -S .
cmake --build build
ctest --test-dir build          # golden-vector suites + the live-layer tests
```

The build produces a static library exposed as the CMake target
`depz::sensor_sdk_cpp` (the alias `depz::sensor_sdk` is kept for backwards
compatibility). The live layer needs the C SDK: in this repository it is
picked up from the sibling `packages/depz-sensor-sdk-c`, otherwise from an
installed `depz-sensor-sdk-c` package; `-DDEPZ_SENSOR_SDK_CPP_BUILD_IO=OFF`
builds the decode layer alone, with no dependency. Pull it into your own tree
with FetchContent (it fetches the whole repository, C SDK included):

```cmake
include(FetchContent)
FetchContent_Declare(depz-sensor-sdk-cpp
  GIT_REPOSITORY https://github.com/depz-ai/depz-sensor-sdk.git
  GIT_TAG        v0.3.0
  SOURCE_SUBDIR  packages/depz-sensor-sdk-cpp)
FetchContent_MakeAvailable(depz-sensor-sdk-cpp)
target_link_libraries(my_app PRIVATE depz::sensor_sdk_cpp)
```

Conan, vcpkg, `add_subdirectory` and `find_package` are covered in the
[guide](guide.md#build--link). The public headers live under `include/depz/` —
include what you use (`#include "depz/device.hpp"`, `#include "depz/sr04.hpp"`);
everything is in namespace `depz` (ToF under
`depz::vl53l4` / `depz::vl53l8` / `depz::vl53l7` / `depz::vl53lx`, IMUs under
`depz::bno086` / `depz::bno055`, datasets under `depz::dataset`).

## Where to next

- **Your sensor's pages** — [SR04](sr04/introduction.md) ·
  [VL53L4CD](vl53l4cd/introduction.md) ·
  [VL53L8CX](vl53l8cx/introduction.md) · [VL53L8CH](vl53l8ch/introduction.md) ·
  [VL53L5CX](vl53l5cx/introduction.md) · [VL53L7CX](vl53l7cx/introduction.md) ·
  [VL53L7CH](vl53l7ch/introduction.md) · [VL53L0X](vl53l0x/introduction.md) ·
  [VL53L1CX](vl53l1cx/introduction.md) · [VL53L1CB](vl53l1cb/introduction.md) ·
  [VL53L3CX](vl53l3cx/introduction.md) · [VL53L4CX](vl53l4cx/introduction.md) ·
  [BNO086](bno086/introduction.md) · [BNO055](bno055/introduction.md):
  introduction, hands-on guide, and the sensor's own API reference.
- **[Guide](guide.md)** — the SDK-wide walkthrough: build & link, the live
  layer (opening a board, errors, threads and callbacks, record / replay),
  byte types, the mental model, framing & CRC, time-sync math, datasets.
- **[API Reference](api.md)** — the whole SDK's public surface, generated from
  the header doc-comments.
- **Docs for LLMs** — the sensor SDK documentation as raw Markdown:
  [/llms-full.txt](/llms-full.txt).
- **Source & license** — the SDKs are open source (MIT):
  [github.com/depz-ai/depz-sensor-sdk](https://github.com/depz-ai/depz-sensor-sdk).
