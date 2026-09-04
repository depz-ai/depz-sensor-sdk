# DEPZ Sensor SDK

Open-source (MIT) SDKs for the [DEPZ](https://depz.ai) USB sensor line, in
seven languages: **Python, TypeScript/JavaScript, C, C++, Rust, Java and C#**.

Five sensors across four firmware apps:

| Sensor | What it is |
|--------|------------|
| **HC-SR04** | ultrasonic ranging (smart device: config + measurement on board) |
| **VL53L4CD** | single-zone ToF ranger to ~1.3 m (host runs the ST ULD 2.2.3 driver over an I2C register bridge) |
| **VL53L8CX** | 8×8 ToF matrix (host runs the ST ULD driver over a register bridge) |
| **VL53L8CH** | VL53L8CX plus CNH histogram output (same firmware app, its own USB PID) |
| **BNO086** | 9-axis IMU (host runs the full SHTP/SH-2 stack over a pass-through bridge) |

## Install

```bash
pip install depz-sensor-sdk                 # Python (PyPI)
npm install @depz/sensor-sdk                # TypeScript/JS (npm)
cargo add depz-sensor-sdk                   # Rust (crates.io)
dotnet add package Depz.Sensor              # C# (NuGet)
```

Java (Maven Central): `io.github.depz-ai:depz-sensor-sdk`.

C / C++ — CMake FetchContent straight from this repository (also Conan and
vcpkg overlay ports; see the package READMEs):

```cmake
include(FetchContent)
FetchContent_Declare(depz_sensor_sdk
  GIT_REPOSITORY https://github.com/depz-ai/depz-sensor-sdk.git
  GIT_TAG        v0.1.4
  SOURCE_SUBDIR  packages/depz-sensor-sdk-c)   # or packages/depz-sensor-sdk-cpp
FetchContent_MakeAvailable(depz_sensor_sdk)
```

## Layout

| Path | What |
|------|------|
| `packages/depz-sensor-sdk-python/` | Python SDK (`depz_sensor_sdk`) — sync API + background reader thread, discovery, CLI, recording/datasets |
| `packages/depz-sensor-sdk-ts/` | TypeScript SDK (`@depz/sensor-sdk`) — WebSerial (browser) + serialport (Node), pure ESM |
| `packages/depz-sensor-sdk-c/` | C SDK — decode/encode, CMake / Conan / vcpkg |
| `packages/depz-sensor-sdk-cpp/` | C++ SDK — decode/encode, CMake / Conan / vcpkg |
| `packages/depz-sensor-sdk-rust/` | Rust SDK (`depz-sensor-sdk`) |
| `packages/depz-sensor-sdk-java/` | Java SDK (`io.github.depz-ai:depz-sensor-sdk`) |
| `packages/depz-sensor-sdk-csharp/` | C# SDK (`Depz.Sensor`) |

## Documentation

Full guides and API references for every language and sensor:
**[depz.ai/developers/sensors/docs](https://depz.ai/developers/sensors/docs)**

The same documentation ships in-repo under each package's `docs/` directory.

Zero-install web viewer (WebSerial, no setup):
**[sensor-viewer.depz.ai](https://sensor-viewer.depz.ai)** — and a standalone
local viewer: `pipx install depz-sensor-viewer`.

## License

MIT — see [LICENSE](LICENSE). The VL53L4CD/VL53L8 drivers port ST
Microelectronics ULD sources under their permissive license terms; see the
per-package license notes.
