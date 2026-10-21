# TMF8829 — introduction

The **TMF8829** is an ams-OSRAM multizone **direct** Time-of-Flight sensor: it
fires laser pulses and times each photon's return per zone. One measurement
is a depth grid of **8×8, 16×16, 32×32 or 48×32 zones** (48×32 = 32 rows × 48
columns), with **up to 4 peaks per zone** (four objects at different
distances seen through one zone — glass and what is behind it, an edge and
the wall), a signal-to-noise ratio per peak, optional signal / noise /
crosstalk values, and optional **raw histograms**. On the DEPZ sensor line
(`APP_TMF8829_v0.51`+ firmware, USB PID `0xED4C`) the MCU is a thin SPI
**register bridge**: the host runs the sensor — soft reset, sensor firmware
download, presets, start / stop, frame parsing and clock correction — and the
bridge only reads finished frames off the sensor and pushes them to the host
in chunks.

## What this C++ SDK offers today

The TMF8829 is covered by both layers of the SDK:

- **The live class** `depz::Tmf8829` (`depz/device.hpp`) — open the board
  (`open_tmf8829()`), `init()` it (soft reset + download of the 14.4 KB ams
  sensor firmware, ~0.1–0.15 s, with a progress callback), `configure()` a
  preset with `Tmf8829Options` (period, iterations, peaks, the optional
  per-zone fields, histograms, the ref-SPAD frame), `start()` / `stop()`, and
  read `Tmf8829Measurement`s as callbacks or from a pull stream — the frames
  reassembled, the 32×32 / 48×32 half-frames merged, distances in
  clock-corrected millimetres, a computed capture time with flags. Around
  that: the raw config page, raw register access, the bridge's counters,
  sensor events, every raw frame (the way to the histogram and ref-SPAD
  bytes) and per-stream counters. It builds on the C SDK; see the
  [common guide](../guide.md#live-hardware) for the build and the rules every
  device shares.
- **The decode layer** (`depz::tmf8829`, `depz/tmf8829.hpp`) — pure codecs for
  bytes you read yourself: the bridge command encoders and report decoders,
  the chunk reassembler (`FrameReassembler`), frame header / footer parse,
  the results-pixel decode and half-frame merge, the clock model and capture
  time (`SensorClock`, `CaptureTimer`), measurement assembly
  (`MeasurementBuilder`), the presets and config-page fields, and the
  embedded sensor firmware image. The live class uses the same types.

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main() {
    auto tof = depz::open_tmf8829();             // the first TMF8829 board
    auto id = tof->init();                       // soft reset + firmware download
    std::printf("sensor app %s, serial %u\n", id.app_version.c_str(), unsigned(id.serial));
    tof->configure("32x32");                     // preset + every survivor field
    auto stream = tof->measurements(4);          // subscribe before start()
    tof->start();
    for (int i = 0; i < 30; ++i) {
        auto m = stream.next(std::chrono::milliseconds(1000));
        if (!m) break;
        auto [rows, cols] = m->shape();          // 32 x 32
        unsigned centre = (rows / 2) * cols + cols / 2;   // peak 0, middle zone
        std::printf("t=%lld us  centre %.1f mm\n",
                    static_cast<long long>(m->timestamp_us), m->distance_mm[centre]);
    }
    tof->stop();
}
```

The class does what the Python SDK's `Tmf8829` does, request for request, so
a session captured by either SDK replays in the other. It is verified
against the C SDK's fake bridge fed with frames captured on a real board
(`contracts/vectors/tmf8829.json`), and its capture-time math bit-exactly
against 27 recorded streams (`tmf8829_frame_time.json`).

## When to use it

Use the TMF8829 when you want a **denser** depth image than the 8×8 VL53L
imagers (up to 1536 zones), several objects per zone, or the raw histograms
for your own processing. Rates at a 1 ms period: ~30 measurements/s on 8×8
and 16×16, ~15 on 32×32, ~10 on 48×32; histograms slow it down (~10/s on
8×8 / 16×16, under 2/s on 48×32 — then USB is the limit). For one distance
per measurement a single-zone sensor (VL53L4CD) is simpler.

## Key concepts

- **Presets** — ten sensor configurations, each a name the SDK maps to the
  sensor's load command: `8x8`, `8x8-lr` (long range), `8x8-ha` (high
  accuracy), `8x8-xr` (extended range), `16x16`, `16x16-ha`, `32x32`,
  `32x32-ha`, `48x32`, `48x32-ha`. `configure()` loads one and writes its
  options on top.
- **A preset is not a clean slate** — the sensor keeps the result format,
  period, histogram switch and more from the previous config across a
  preset load (and some even across `init()`). So `configure()` writes all
  of them **every time**, with defaults when you leave them out — the result
  never depends on an earlier config.
- **Distances** — the sensor reports 0.25 mm units (mm on `8x8-xr`) counted
  by its own clock, which runs ~1.5 % slow. `distance_mm` is already scaled
  and clock-corrected; `results.distance` keeps the raw integers.
- **Half-frames** — 32×32 and 48×32 arrive as two frames, the even rows and
  then the odd rows; the measurement merges them. On 8×8 / 16×16 one frame
  is one measurement.
- **Capture time is computed** — the board timestamps when it read a frame,
  not when the sensor measured it. From the sensor's clock values in the
  frame, the SDK computes the middle of the measurement window in MCU
  microseconds (`timestamp_us`) and flags anything doubtful (`flags`).
- **Grid orientation** — the arrays keep the sensor's own row-major order,
  which is transposed against the scene: for a board held USB down and seen
  from behind (looking where the sensor looks), the **row** index runs left
  → right and the **column** index top → bottom. Transpose to draw it (the
  DEPZ viewer does).
- **No sleep, recovery is `init()`** — the sensor's EN pin is wired high, so
  there is no power-mode API. `init()` starts with a soft reset and works
  from any sensor state; if even it fails, only a USB re-plug is left.
- **The sensor firmware** is ams' `tmf8829_application` image (application
  2.200.0, MIT licence), embedded in the SDK and shipped with its licence
  (`licenses/tmf8829/LICENSES-MIT.TXT`).

## See also

- [TMF8829 user guide](guide.md) — open and init, configure, measurements,
  histograms and ref-SPAD frames, diagnostics, recovery, and the decode layer.
- [API reference](api.md) — `depz/tmf8829.hpp` and the live class from
  `depz/device.hpp`.
