# depz-sensor-sdk

Public distribution mirror of the **C and C++** SDKs for the DEPZ USB sensor
line (SR04, VL53L8CX/CH ToF, BNO086 IMU). Development happens in the DEPZ
monorepo; this repo mirrors the C/C++ packages + golden test vectors so they
can be consumed by CMake FetchContent, Conan and vcpkg (which fetch source at
build time and therefore need a public repository).

- **C**:   `packages/depz-sensor-sdk-c`   — see its `README.md` (FetchContent / find_package / Conan / vcpkg).
- **C++**: `packages/depz-sensor-sdk-cpp` — same.

Contract-first: `contracts/vectors/` holds the golden vectors both SDKs verify against.
This repository is generated — do not edit here; changes go to the monorepo.
