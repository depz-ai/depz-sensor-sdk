# TMF8829 — introduction

The **TMF8829** is an ams-OSRAM multizone **direct time-of-flight** (dToF)
sensor: it fires a laser and times the returning photons for every zone of a
grid, up to **48×32 zones** and up to **4 distance peaks per zone**, and it can
dump the raw photon **histograms** too. On the DEPZ sensor line the MCU is a
thin SPI **register bridge** (`APP_TMF8829_v0.51`+ firmware, contract 14, USB
PID `0xED4C`, `UsbIds.PID_TMF8829`): the host runs the sensor — soft reset,
sensor-firmware download, presets, start/stop — and the bridge pushes every
frame the sensor produces to the host in chunks.

## What the sensor does

- **Four grids**: 8×8, 16×16, 32×32 and 48×32 (32 rows × 48 columns), chosen
  by one of ten **presets**: `8x8`, `8x8-lr` (long range), `8x8-ha` (high
  accuracy), `8x8-xr` (extended range), `16x16`, `16x16-ha`, `32x32`,
  `32x32-ha`, `48x32`, `48x32-ha`.
- **Per zone**: up to 4 peaks, each a distance plus its SNR and optionally a
  signal strength; optionally a noise and a crosstalk value per zone.
- **Raw histograms** (one 25 377-byte frame per block of zones) and the
  60-byte **ref-SPAD** frame, for calibration work.
- **Rates** at a 1 ms period: about 30 measurements/s on 8×8 and 16×16, 15 on
  32×32, 10 on 48×32. Histograms slow it down (≈10/s on 8×8, ≈1.7/s on
  48×32) — then USB is the limit.

## When to use it

Use the TMF8829 when you need a denser depth image than the 8×8 VL53L5/L7/L8
imagers give — up to 1536 zones — or several targets per zone (glass in
front of a wall, an edge). For a plain 8×8 depth image the
[VL53L8CX](../vl53l8cx/introduction.md) is simpler.

## What this SDK offers

The live device `ai.depz.sensor.devices.Tmf8829Device` runs the sensor: the
firmware download, the config page and the start / stop sequence of contract
14 §4, with the measurements built from the streamed frames. Under it:

- [`Tmf8829`](api.md#tmf8829) (`ai.depz.sensor.protocol`) — the bridge wire
  codecs: `TMF_READ_REG`, `TMF_WRITE_REG`, `TMF_START_STREAM`,
  `TMF_STOP_STREAM`, `TMF_GET_INFO` payloads; the `RPT_TMF_REG_DATA`,
  `RPT_TMF_INFO`, `RPT_TMF_FRAME` chunk and `RPT_TMF_EVENT` reports; the
  `FrameReassembler` that turns chunks back into frames.
- `ai.depz.sensor.sensors.tmf8829`:
  - `Frames` — frame header/footer parse, results-pixel decode, the
    32×32/48×32 half-frame merge, raw histogram frames;
  - `Tmf8829Config` — the presets, each preset's `kilo_iterations`, and
    named get/set of the config-page fields;
  - `SensorClock` + `CaptureTimer` — the sensor clock model and the capture
    time of every results frame;
  - `MeasurementBuilder` — frames in stream order → whole measurements with
    clock-corrected distances in mm;
  - `Tmf8829Firmware` — the embedded sensor-firmware image (ams-OSRAM,
    **MIT**, shipped with its `LICENSES-MIT.TXT` on the classpath).

## Key concepts

- **The host runs the sensor** — the board has no TMF8829 logic of its own;
  init, presets and start/stop are register writes the host sends
  (contract 14 §4 lists the sequence).
- **Firmware download on every init** — the sensor boots into its
  bootloader; the host downloads the 14.4 KB application image
  (`Tmf8829Firmware.image()`) on every init, ~0.1–0.15 s.
- **No sleep** — the sensor's EN pin is wired high: there is no power-mode
  API, and recovery from any state is a fresh init (it starts with a soft
  reset).
- **A preset is not a clean slate** — it keeps the result format, the
  histogram switch and the period from the previous config. A correct host
  writes `period_ms`, `kilo_iterations`, `nr_peaks`, the three strengths,
  `dump_histograms` and `ref_spad_frame` on **every** configure.
- **Units** — raw distances are in 0.25 mm, except preset `8x8-xr` (mm; the
  frame footer says which). They also run on the sensor clock, ~1.5 % slow:
  `distance_mm = raw × ratio / 4` (or `raw × ratio`). `MeasurementBuilder`
  does that for you.
- **Half-frames** — 32×32 and 48×32 arrive as two frames, even rows first,
  then odd rows; `MeasurementBuilder` merges them.
- **Orientation** — the grid is transposed against the scene: for a board
  held USB down and seen from behind (looking where the sensor looks), the
  **row** index runs left → right and the **column** index top → bottom. The
  SDK keeps the sensor's own row-major order; the DEPZ viewer transposes for
  display.
- **Capture time** — computed on the host from the sensor's integration
  timestamps and the clock pair in every frame (the centre of the capture
  window, MCU µs), bit-exact with the other SDKs.

## See also

- [TMF8829 user guide](guide.md) — commands, firmware download, config page,
  stream decode, frames, capture time, bridge info, gotchas.
- [API reference](api.md) — `Tmf8829` and the `sensors.tmf8829` classes.
