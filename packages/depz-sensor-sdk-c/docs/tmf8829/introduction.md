# TMF8829 — introduction

The **TMF8829** is an ams-OSRAM multizone **direct time-of-flight** (dToF)
sensor: it fires a laser and times the photons that come back, per zone, on a
grid of 8×8, 16×16, 32×32 or 48×32 zones, with up to **4 peaks** (objects at
different distances) per zone and the raw photon **histograms** on request. On
the DEPZ sensor line (`APP_TMF8829_v0.51`+ firmware, USB PID `0xED4C`) the MCU
is a thin SPI **register bridge** (contract 14): it reads the sensor's frames
off its FIFO on each interrupt and pushes them to the host in chunks.
Everything else — the soft reset, the sensor-firmware download, the presets,
start / stop, putting the frames together, the clock correction — runs on the
host, and this C SDK does it in the **TMF8829 sensor class**
(`depz_tmf8829_*`).

## What the SDK covers

The TMF8829 has a **sensor class** in the C live layer (`depz_sensor_io.h`),
with the same request sequences as the Python SDK's `Tmf8829`:

- **Open** — `depz_open_device()` finds, probes and opens the board with the
  class attached; the device reports sensor type `tmf8829`
  (`DEPZ_SENSOR_TMF8829`) and `depz_is_tmf8829()` is true for it.
  `depz_tmf8829_open_link(link, &dev)` attaches it to a link without a probe
  (tests, replay).
- **Init** — `depz_tmf8829_init()`: soft reset, the sensor's bootloader, the
  14.4 KB sensor firmware downloaded into it, then its application; returns
  the application version (`"2.200.0"`) and the sensor serial. About
  0.1–0.15 s.
- **Configure** — `depz_tmf8829_configure(dev, preset, &opt, &cfg)`: one of
  ten presets (grid and mode), the measurement period, the result format
  (peaks, signal / noise / crosstalk), histograms and the reference frame,
  any other config-page field by name — all checked by reading the page back.
- **Measurements** — `depz_tmf8829_start()` / `_stop()`, then measurements as
  callbacks, a bounded stream or one at a time. Each
  `depz_tmf8829_measurement` is one full grid: the raw sensor integers,
  distances in mm corrected for the sensor clock, the capture time in MCU
  microseconds and quality flags. 32×32 and 48×32 arrive as two half-frames;
  the class merges them.
- **Raw frames and events** — every reassembled frame (histogram frames and
  reference frames included) through a frame callback; the bridge's
  interrupt / bad-header events through an event callback.
- **Board** — raw sensor registers, the config page as the sensor holds it,
  the bridge counters, per-stream counters (dropped, discarded, unparsed
  frames).
- **Record / replay** — any session records to a `.depzrec` file and replays
  byte-exact.
- **Decode layer** — if you own the transport (`depz_sensor_sdk.h`): the
  bridge command encoders and report decoders, the chunk reassembler, frame /
  pixel decode with the half-frame merge, the sensor-clock model and capture
  time, the measurement builder, presets and config-page fields, and the
  embedded sensor firmware.

**Status: checked against the golden vectors and a fake bridge, not yet on a
board from C.** The decode layer reproduces `contracts/vectors/tmf8829.json`
(real frames captured on a board) and the capture times of all 970 results
frames of `tmf8829_frame_time.json` bit for bit; the class runs a whole
session against a fake bridge playing captured frames — see
[verification status](guide.md#verification-status). The Python SDK it is
ported from is hardware-verified (firmware v0.52).

## When to use it

Use the TMF8829 when you want a depth image denser than the VL53L
multizone sensors' 8×8 — up to 48×32 = 1536 zones — or several objects per
zone (glass in front of a wall, an edge), or the raw histograms for your own
processing. The price is the rate: about 30 measurements/s at 8×8 and 16×16,
15 at 32×32 and 10 at 48×32.

## Key concepts

- **Zones and the grid** — each zone is one "pixel" of the depth image.
  Grids (rows × columns): 8×8, 16×16, 32×32 and 48×32, which is **32 rows ×
  48 columns** (`rows = 32`, `cols = 48`).
- **Peaks** — the sensor can report up to 4 distances per zone, one per
  return it found (`nr_peaks` 1..4), each with its signal-to-noise ratio and,
  optionally, its signal strength.
- **Presets** — ten ready configurations, named by grid: `8x8`, `8x8-lr`
  (long range), `8x8-ha` (high accuracy), `8x8-xr` (extended range),
  `16x16`, `16x16-ha`, `32x32`, `32x32-ha`, `48x32`, `48x32-ha`.
- **A preset is not a clean slate** — the sensor keeps the result format,
  the period and the histogram switch from the previous configuration across
  a preset load, so `configure()` writes all of them **every time** (defaults
  unless you set them). A configuration never depends on the one before.
- **Distance units** — the raw integers are in 0.25 mm, except with preset
  `8x8-xr` (whole mm). `distance_mm` is already converted and corrected for
  the sensor clock (which runs ~1.5 % slow); use it rather than the raw value.
- **Half-frames** — at 32×32 and 48×32 the sensor sends each measurement as
  two frames, the even rows then the odd rows. The class glues them back
  together; the measurement's time is the mean of the two.
- **Capture time** — the sensor stamps its frames with its own 125 kHz clock;
  the SDK maps that onto the MCU's microsecond clock and puts the measurement
  time in the middle of the actual light-collection window, not at the moment
  the bytes arrived.
- **Grid orientation** — the grid is turned against the scene: for a board
  held USB down and seen from behind (looking where the sensor looks), the
  **row** index runs left → right and the **column** index top → bottom. The
  SDK keeps the sensor's own order; transpose it to show the scene (the DEPZ
  viewer does).
- **No sleep, recovery is init** — the sensor's enable pin is wired high on
  this board, so there is no power-down or standby; whatever state the sensor
  is in, `depz_tmf8829_init()` brings it back (it starts with a soft reset).
  If init fails, only re-plugging the USB cable is left.
- **Sensor firmware** — the image `init()` downloads is ams-OSRAM's
  `tmf8829_application` 2.200.0, MIT-licensed; it is compiled into the
  library (`depz_tmf8829_firmware`), and its license ships with the SDK as
  `licenses/tmf8829/LICENSES-MIT.TXT`.

## See also

- [TMF8829 user guide](guide.md) — open and init, presets and options, the
  measurement, streaming, histograms, recovery, record / replay, the decode
  layer, gotchas.
- [API reference](api.md) — the class (`depz_tmf8829_*` in
  `depz_sensor_io.h`) and the codecs (`depz_sensor_sdk.h`).
- [VL53L8CX](../vl53l8cx/introduction.md) — the other multizone ToF family of
  the line (4×4 / 8×8).
