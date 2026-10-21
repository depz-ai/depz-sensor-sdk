# TMF8829 — introduction

The **TMF8829** is an ams-OSRAM multizone **direct Time-of-Flight** (dToF)
sensor: it fires infrared light and times the photons that come back, per
zone, and reports a depth image from 8×8 up to 48×32 zones with up to four
targets ("peaks") per zone. On the DEPZ sensor line (USB PID `0xED4C`,
firmware `APP_TMF8829_v0.51`+) the MCU is a thin **SPI register bridge**
(contract 14): the host runs the sensor — firmware download, presets, the
config page, start/stop, frame parsing and the clock correction. All of that
lives in `depz_sensor_sdk.tmf8829` (`Tmf8829`).

## What it does

- **Depth images** in four grids: 8×8, 16×16, 32×32 and 48×32 (32 rows ×
  48 columns), picked by one of ten **presets** (`8x8`, `8x8-lr` long range,
  `8x8-ha` high accuracy, `8x8-xr` extended range, `16x16`, `16x16-ha`,
  `32x32`, `32x32-ha`, `48x32`, `48x32-ha`).
- **Up to 4 peaks per zone** — the distance, SNR and (optionally) signal
  strength of each return, plus per-zone noise and crosstalk if you ask for
  them. Glass in front of a wall gives two peaks in one zone.
- **Distances in mm, clock-corrected** — `distance_mm` is ready to use; the
  raw sensor integers stay available.
- **Raw histograms** and **ref-SPAD frames** for calibration work (they slow
  the stream down a lot).
- **A capture time per measurement** in MCU microseconds, computed on the
  host from the sensor's own clock, with quality flags.

Rates at a 1 ms period: ~30 measurements/s for 8×8 and 16×16, ~15 for
32×32, ~10 for 48×32. With histograms the USB link becomes the limit (~10/s
for 8×8, under 2/s for 48×32).

## When to use it

Use the TMF8829 when you need a denser depth image than the 8×8 VL53L5/L7/L8
matrices give, or several targets per zone. For a single distance a
single-zone ToF sensor is simpler; for 8×8 with ST's on-chip processing see
the [VL53L8CX](../vl53l8cx/introduction.md).

## Key concepts

- **The host downloads the sensor firmware** — `init()` soft-resets the
  sensor and loads a 14.4 KB application image into it through its
  bootloader. It takes only ~0.1–0.15 s. The image is ams-OSRAM's
  `tmf8829_application` 2.200.0, **MIT**-licensed, embedded in the package
  (`firmware_image()`) and shipped with its `LICENSES-MIT.TXT`.
- **Presets + a config page** — a preset loads a 190-byte config page;
  `configure()` then sets the fields you care about. A preset load does
  **not** reset the result format, histogram dump and similar fields from the
  previous config, so `configure()` writes all of them every time (with
  defaults for those you do not pass). The result never depends on an earlier
  config.
- **Half-frames** — 32×32 and 48×32 arrive as two frames, the even rows and
  the odd rows. The SDK merges them; you always get one full measurement.
- **Distance units** — the sensor counts in 0.25 mm (preset `8x8-xr`: whole
  mm) of *its own* clock, which runs ~1.5 % slow. The SDK measures that
  clock against the MCU and corrects both distances and times.
- **Grid orientation** — the SDK keeps the sensor's own row-major order. For
  a board held USB down and seen from behind (looking where the sensor
  looks), the **row** index runs left → right and the **column** index top →
  bottom: the image is transposed against the scene. Viewers transpose it
  back (the DEPZ viewer does).
- **No sleep** — the sensor's enable pin is wired on, so there is no power
  mode API. Recovery from any state is `init()` again (it starts with a soft
  reset); if that fails, re-plug the USB cable.

## See also

- [TMF8829 user guide](guide.md) — init, presets, streaming, the
  measurement, histograms, capture time, recording, gotchas.
- [API reference](api.md) — `Tmf8829`, `Tmf8829Measurement`,
  `Tmf8829Config`, `PixelResults`, the frame and clock codecs.
