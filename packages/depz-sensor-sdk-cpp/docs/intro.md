# C++ SDK

`depz-sensor-sdk-cpp` is the **C++17 decode layer** for the DEPZ USB sensor
line — pure codecs, **no I/O and no transport**. You own the serial port (or a
recording, or a socket); the SDK turns bytes into typed values and typed values
back into bytes, byte-for-byte identical to the Python / TypeScript / Java
reference SDKs via the shared golden test vectors.

**Working with one specific sensor?** Each has its own introduction, guide
and API reference:

- **[SR04](sr04/introduction.md)** — ultrasonic ranging: one distance per ping.
- **[VL53L8CX](vl53l8cx/introduction.md)** — 8×8 ToF depth frames.
- **[VL53L8CH](vl53l8ch/introduction.md)** — the CX superset with CNH histograms.
- **[BNO086](bno086/introduction.md)** — 9-axis IMU: orientation and motion.

## Build & link

```bash
cmake -B build -S .
cmake --build build
ctest --test-dir build          # golden-vector suites
```

The build produces a static library exposed as the CMake target
`depz::sensor_sdk_cpp` (the alias `depz::sensor_sdk` is kept for backwards
compatibility). Pull it into your own tree with FetchContent:

```cmake
include(FetchContent)
FetchContent_Declare(depz-sensor-sdk-cpp
  GIT_REPOSITORY https://github.com/depz-ai/depz-sensor-sdk.git
  GIT_TAG        v0.1.2
  SOURCE_SUBDIR  packages/depz-sensor-sdk-cpp)
FetchContent_MakeAvailable(depz-sensor-sdk-cpp)
target_link_libraries(my_app PRIVATE depz::sensor_sdk_cpp)
```

The public headers live under `include/depz/` — include what you use
(`#include "depz/sr04.hpp"`); everything is in namespace `depz` (ToF under
`depz::vl53l8`, IMU under `depz::bno086`, datasets under `depz::dataset`).

## Where to next

- **Your sensor's pages** — [SR04](sr04/introduction.md) ·
  [VL53L8CX](vl53l8cx/introduction.md) · [VL53L8CH](vl53l8ch/introduction.md) ·
  [BNO086](bno086/introduction.md): introduction, hands-on guide, and the
  sensor's own API reference.
- **[Guide](guide.md)** — the SDK-wide walkthrough: build & link, byte types,
  the mental model, framing & CRC, time-sync math, datasets.
- **[API Reference](api.md)** — the whole SDK's public surface, generated from
  the header doc-comments.
- **Docs for LLMs** — the sensor SDK documentation as raw Markdown:
  [/llms-full.txt](/llms-full.txt).
