# TMF8829 — introduction

The **TMF8829** is an ams-OSRAM multizone **direct** Time-of-Flight sensor
(dToF): it times single photons of its own laser and returns a depth image of
**8×8, 16×16, 32×32 or 48×32 zones**, with up to **4 peaks** (targets) per
zone and, on request, the raw per-zone photon histograms. On the DEPZ sensor
line the MCU is a thin SPI **register bridge** (`APP_TMF8829_v0.51`+
firmware, contract 14, USB PID `0xED4C`): the host runs the sensor — soft
reset, sensor-firmware download, presets, start/stop — and the bridge only
pushes the sensor's self-sized frames to the host in chunks.

## What the sensor does

- **Depth grids** — eight presets for the four grids, plus long-range
  (`8x8-lr`), high-accuracy (`*-ha`) and extended-range (`8x8-xr`) variants:
  ten presets in all.
- **Up to 4 peaks per zone** — distance and SNR for every peak, optionally
  the signal strength per peak, the noise and the crosstalk per zone.
- **Raw histograms** — the photon return counts per zone, for calibration
  and multi-target analysis (they slow the stream down a lot).
- **Rates** at a 1 ms period: ~30 measurements/s on 8×8 and 16×16, ~15 on
  32×32, ~10 on 48×32; with histograms ~10 (8×8/16×16) and under 2 (48×32).

## When to use it

Use the TMF8829 when you need a denser depth image than the 8×8 VL53L5/L7/L8
family — up to 1536 zones — or several targets per zone (glass in front of a
wall, a person in front of furniture). For a single distance a 1D ToF sensor
is simpler; for 8×8 with CNH histograms see the
[VL53L8CH](../vl53l8ch/introduction.md).

## What this crate offers

The [`tmf8829`](api.md) module is the verifiable **codec and decode layer** —
everything except talking to a port:

- the bridge wire codecs — `TMF_READ_REG`, `TMF_WRITE_REG`,
  `TMF_START_STREAM` / `TMF_STOP_STREAM`, `TMF_GET_INFO` payloads
  (range-checked), and the `RPT_TMF_REG_DATA`, `RPT_TMF_INFO`,
  `RPT_TMF_FRAME` and `RPT_TMF_EVENT` reports;
- [`FrameReassembler`](api.md#framereassembler) — chunks back into frames,
  counting dropped and discarded ones;
- [`parse_frame`](api.md#parse_frame) / [`parse_results`](api.md#parse_results)
  / [`merge_halves`](api.md#merge_halves) — header, footer, per-zone results,
  and the 32×32/48×32 half-frame merge;
- [`SensorClock`](api.md#sensorclock) + [`CaptureTimer`](api.md#capturetimer)
  — the integer clock model and the capture time of every frame, bit-exact
  with the shared vectors;
- [`MeasurementBuilder`](api.md#measurementbuilder) — frames in stream order
  → [`Tmf8829Measurement`](api.md#tmf8829measurement) with clock-corrected
  distances in mm;
- the presets, their `kilo_iterations` and the config-page fields
  ([`cfg_get`](api.md#cfg_get) / [`cfg_set`](api.md#cfg_set));
- the embedded **sensor firmware image** ([`FIRMWARE_IMAGE`](api.md#firmware_image),
  14 444 B) with its MIT license text.

What it does **not** do: open a port or run the sensor. There is **no live
device class** in this crate — no `init()`, `configure()`, `start()`. The init
sequence (soft reset, bootloader, firmware download), configuration and
start/stop are register writes you send yourself (contract 14 §4 lists them);
the Python, TypeScript, C and C++ SDKs ship them as a ready driver.

## Key concepts

- **The host runs the sensor** — after every power-up or soft reset the
  sensor sits in its bootloader; the host downloads the 14.4 KB firmware
  image (~0.1–0.15 s) and starts it. The image is ams-OSRAM's, MIT-licensed,
  and embedded in this crate together with its license.
- **No sleep, recovery = init** — the sensor's enable pin is wired high on
  this board, so there is no power mode to switch. Any trouble (a refused
  frame header, a stuck sensor) is cured by running the init sequence again;
  it starts with a soft reset.
- **A preset is not a clean slate** — loading a preset keeps what the
  previous config wrote to the result format, histogram dump, ref frame and
  period. Write all of them on every configuration (the guide shows how).
- **Frames, not measurements** — 8×8 and 16×16 send one results frame per
  measurement; 32×32 and 48×32 send **two half-frames** (even rows, then odd
  rows). Histogram and ref-SPAD frames come before the results frame they
  belong to. `MeasurementBuilder` does the grouping.
- **Distances are in sensor-clock units** — 0.25 mm per step (mm on preset
  `8x8-xr`), measured with a sensor clock ~1.5 % slow; `distance_mm` corrects
  both with the measured clock ratio.
- **Grid orientation** — the arrays keep the sensor's own row-major order.
  For a board held **USB down** and seen from behind (looking where the sensor
  looks), the **row** index runs left → right and the **column** index top →
  bottom: transpose to show the scene, as the DEPZ viewer does.

## See also

- [TMF8829 user guide](guide.md) — commands, reassembly, frame decode,
  measurements, capture time, config page, firmware image, gotchas.
- [API reference](api.md) — the `tmf8829` module.
