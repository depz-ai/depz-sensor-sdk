# TMF8829 — introduction

The **TMF8829** is an ams-OSRAM multizone **direct time-of-flight** (dToF)
sensor: it fires a laser and times the returning photons for every zone of a
grid of up to 48×32 zones, with up to four distances (peaks) per zone and,
optionally, the raw photon-count histograms behind them. On the DEPZ sensor
line the MCU is a thin SPI **register bridge** (`APP_TMF8829_v0.51`+ firmware,
contract 14, USB PID `0xED4C`): the host runs the sensor — soft reset, sensor
firmware download, config page, start/stop — and the bridge pushes the
sensor's self-sized frames to the host in chunks.

## What the sensor does

- **Four grids** (rows × columns): 8×8, 16×16, 32×32 and 48×32 (32 rows ×
  48 columns), chosen by one of ten **presets**: `8x8`, `8x8-lr` (long
  range), `8x8-ha` (high accuracy), `8x8-xr` (extended range), `16x16`,
  `16x16-ha`, `32x32`, `32x32-ha`, `48x32`, `48x32-ha`.
- **Up to 4 peaks per zone** — distance and SNR for each, plus optional
  signal strength per peak, and noise and crosstalk per zone.
- **Raw histograms** (25 377-byte frames) and **reference-SPAD frames** for
  calibration work.
- **Rates** at a 1 ms period: about 30 measurements/s on 8×8 and 16×16,
  ~15/s on 32×32, ~10/s on 48×32. Histograms slow it down (~10/s on
  8×8/16×16, under 2/s on 48×32 — then USB is the limit).

## When to use it

Use the TMF8829 when you need a denser depth image than the 8×8 VL53L5/L7/L8
imagers give — up to 1 536 zones — or several returns per zone (glass,
partial occlusion, edges). For a single distance, a single-zone ToF is
simpler; for 8×8 depth at low cost, the [VL53L8CX](../vl53l8cx/introduction.md).

## What this SDK offers

The live device `Depz.Sensor.Devices.Tmf8829Device` runs the sensor from the
host — `Init()` with the firmware download, `Configure()`, `Start()` /
`Stop()`, measurements as a stream — see the [user guide](guide.md#live-device).
Under it, the `Depz.Sensor.Tmf8829` namespace is the verifiable **codec and
decode layer**:

- the bridge wire codecs — `TMF_READ_REG`, `TMF_WRITE_REG`,
  `TMF_START_STREAM` / `TMF_STOP_STREAM`, `TMF_GET_INFO`
  ([`Tmf8829Wire`](api.md#tmf8829wire)) and the reports
  `RPT_TMF_REG_DATA`, `RPT_TMF_INFO`, `RPT_TMF_FRAME`, `RPT_TMF_EVENT`;
- the chunk reassembler ([`Tmf8829FrameReassembler`](api.md#tmf8829framereassembler));
- frame header/footer parse, results-pixel decode and the 32×32/48×32
  half-frame merge ([`Tmf8829Frames`](api.md#tmf8829frames));
- the clock model and the frame capture time
  ([`Tmf8829SensorClock`](api.md#tmf8829sensorclock),
  [`Tmf8829CaptureTimer`](api.md#tmf8829capturetimer)), and the
  [`Tmf8829MeasurementBuilder`](api.md#tmf8829measurementbuilder) that turns
  frames in stream order into measurements;
- presets and config-page fields ([`Tmf8829Config`](api.md#tmf8829config));
- the embedded sensor firmware image ([`Tmf8829Firmware`](api.md#tmf8829firmware)).

## Key concepts

- **The host runs the sensor** — the bridge only moves register bytes and
  frames. Every `init()` (a soft reset and the 14 444-byte firmware download,
  ~0.11–0.16 s) starts the sensor from scratch.
- **Measurement = frames** — 8×8 and 16×16 give one results frame per
  measurement; 32×32 and 48×32 give **two half-frames** (even rows, then odd
  rows) that are merged. Histogram and ref-SPAD frames come before the
  results frame they belong to.
- **Distances need the clock** — raw distances are in **0.25 mm** (in **mm**
  for preset `8x8-xr`) of the sensor's own clock, which runs ~1.5 % slow.
  `distance_mm = raw × ratio / 4` (or `raw × ratio`), the ratio estimated
  from the stream; `Tmf8829MeasurementBuilder` does it for you.
- **Capture time is computed** — the device sends clock pairs, not capture
  times; the host places each frame in the middle of its integration window
  and flags anything uncertain (`Tmf8829TimeFlags`).
- **A preset is not a clean slate** — loading one keeps the result format,
  histogram dump and period of the previous config, so a host writes all of
  them on every configure (contract 14 §4).
- **Grid orientation** — for a board held USB down and seen from behind (in
  the direction the sensor looks), the **row** index runs left → right and the
  **column** index top → bottom. The SDK keeps the sensor's own row-major
  order; viewers transpose to show the scene.
- **No sleep** — the sensor's EN pin is wired high; recovery from any state is
  a new `init()`, and only a USB re-plug is left if that fails.
- **Sensor firmware** — the image is ams-OSRAM's `tmf8829_application`
  (application 2.200.0, **MIT**), embedded in the assembly with its license
  text (`Tmf8829Firmware.License()`).

## See also

- [TMF8829 user guide](guide.md) — commands, stream decode, measurements,
  capture time, config page, firmware image, gotchas.
- [API reference](api.md) — `Depz.Sensor.Tmf8829`.
