# DEPZ Sensor SDK

Open-source (MIT) SDKs for the [DEPZ](https://depz.ai) USB sensor line, in
seven languages: **Python, TypeScript/JavaScript, C, C++, Rust, Java and C#**.

The sensors, each a USB CDC-ACM board speaking one framed protocol:

| Sensor | What it is |
|--------|------------|
| **HC-SR04** | ultrasonic ranging (smart device: config + measurement on board) |
| **VL53L0X / VL53L1CX / VL53L1CB / VL53L3CX / VL53L4CX** | single-zone ToF family, 2–8 m (host runs ST's light and histogram, multi-target, drivers) |
| **VL53L4CD** | single-zone ToF ranger to ~1.3 m (host runs the ST ULD driver over an I2C register bridge) |
| **VL53L5CX / VL53L7CX / VL53L7CH** | 8×8 ToF on an I2C board, 63° / 90° view (VL53L7CH adds streamed CNH histograms) |
| **VL53L8CX** | 8×8 ToF matrix (host runs the ST ULD driver over a register bridge) |
| **VL53L8CH** | VL53L8CX plus CNH histogram output (same firmware app, its own USB PID) |
| **TMF8829** | multizone direct ToF, 8×8 … 48×32 zones, up to 4 targets per zone, raw histograms (host downloads the ams sensor firmware) |
| **BNO055** | 9-axis IMU with on-chip fusion (orientation, gravity, calibration) |
| **BNO085 / BNO086** | 9-axis IMU (host runs the full SHTP/SH-2 stack over a pass-through bridge) |

## Install

```bash
pip install depz-sensor-sdk                 # Python (PyPI)
npm install @depz/sensor-sdk                # TypeScript/JS (npm)
cargo add depz-sensor-sdk                   # Rust (crates.io)
dotnet add package Depz.Sensor              # C# (NuGet)
```

Java (Maven Central): `io.github.depz-ai:depz-sensor-sdk`. Android (Maven
Central): `io.github.depz-ai:depz-sensor-sdk-android`.

C / C++ — CMake FetchContent straight from this repository (also Conan and
vcpkg overlay ports; see the package READMEs):

```cmake
include(FetchContent)
FetchContent_Declare(depz_sensor_sdk
  GIT_REPOSITORY https://github.com/depz-ai/depz-sensor-sdk.git
  GIT_TAG        v1.0.0
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
| `packages/depz-sensor-sdk-android/` | Android SDK (`io.github.depz-ai:depz-sensor-sdk-android`, Android 12+) — Kotlin (coroutines, `Flow`) and Java (callbacks) over the Java SDK, USB host transport, discovery and permission; a sample app |
| `packages/depz-sensor-ros2/` | ROS 2 package `depz_sensors` (Jazzy, Lyrical) — a node over the C++ SDK that publishes the boards as standard `sensor_msgs` topics; built from source with `colcon` |

Each package is what using it needs: the sources, the build files, the docs and
the examples. The test suites and the golden vectors they run on are kept in
DEPZ's development repository; every release here is one tested snapshot.

Source comments and pages cite "contract NN §x" and "ERRATA Ex": sections of
DEPZ's firmware protocol specification and its list of firmware errata, which
the SDKs implement and which are not part of this repository.

## Documentation

Full guides and API references for every language and sensor:
**[depz.ai/developers/sensors/docs](https://depz.ai/developers/sensors/docs)**

The same documentation ships in-repo under each package's `docs/` directory.

Zero-install web viewer (WebSerial, no setup):
**[sensor-viewer.depz.ai](https://sensor-viewer.depz.ai)** — and a standalone
local viewer: `pipx install depz-sensor-viewer`.

## License

MIT — see [LICENSE](LICENSE). The VL53L5 / VL53L7 / VL53L8 sensor firmware,
configuration data and the driver code ported from ST's Ultra Lite Driver are
© STMicroelectronics under the BSD-3-Clause license:
[licenses/vl53l8/LICENSE-BSD-3-Clause.txt](licenses/vl53l8/LICENSE-BSD-3-Clause.txt),
carried by every package too. The TMF8829 sensor firmware is © ams-OSRAM (MIT;
its text ships next to it in each package).
