# C SDK

`depz-sensor-sdk-c` is a **contract-first, dependency-free C11 decode/codec
layer** for the DEPZ USB sensor line. It turns the sensors' byte stream into
typed C values and packs the command/config payloads that go the other way —
byte-exact with the protocol contracts and the shared golden vectors. The
entire public surface is one header: `include/depz_sensor_sdk.h`.

**Working with one specific sensor?** Each has its own introduction, guide
and API reference:

- **[SR04](sr04/introduction.md)** — ultrasonic ranging: one distance per ping.
- **[VL53L8CX](vl53l8cx/introduction.md)** — 8×8 ToF depth frames.
- **[VL53L8CH](vl53l8ch/introduction.md)** — the CX superset with CNH histograms.
- **[BNO086](bno086/introduction.md)** — 9-axis IMU: orientation and motion.

## Build & install

C11, no dependencies beyond libm. Build the static library and run the vector
suite with CMake:

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
  GIT_TAG        v0.1.2
  SOURCE_SUBDIR  packages/depz-sensor-sdk-c
)
FetchContent_MakeAvailable(depz_sensor_sdk_c)

target_link_libraries(my_app PRIVATE depz::sensor_sdk_c)
```

The library also ships an installable CMake package
(`find_package(depz-sensor-sdk-c CONFIG)`) plus Conan and vcpkg recipes — see
the [README](../README.md#install).

```c
#include <depz_sensor_sdk.h>
```

## Where to next

- **Your sensor's pages** — [SR04](sr04/introduction.md) ·
  [VL53L8CX](vl53l8cx/introduction.md) · [VL53L8CH](vl53l8ch/introduction.md) ·
  [BNO086](bno086/introduction.md): introduction, hands-on guide, and the
  sensor's own API reference.
- **[Guide](guide.md)** — the SDK-wide walkthrough: build, the transport/decode
  mental model, identity & discovery, per-sensor decode, extension points.
- **[API Reference](api.md)** — the whole public surface, generated from the
  header's doc-comments.
- **Docs for LLMs** — the sensor SDK documentation as raw Markdown:
  [/llms-full.txt](/llms-full.txt).
