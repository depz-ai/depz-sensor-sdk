# TMF8829 — user guide

Hands-on guide to the `Tmf8829` device class. For what the sensor is and its
concepts, read the [introduction](introduction.md); for exact signatures see
the [API reference](api.md).

## Contents

- [Open and initialize](#open-and-initialize)
- [Hello-world: a depth image](#hello-world-a-depth-image)
- [Presets and configuration](#presets-and-configuration)
- [The measurement](#the-measurement)
- [Streaming](#streaming)
- [Histograms and ref-SPAD frames](#histograms-and-ref-spad-frames)
- [Capture time and flags](#capture-time-and-flags)
- [Recording](#recording)
- [Bridge diagnostics and recovery](#bridge-diagnostics-and-recovery)
- [Decoding without a device](#decoding-without-a-device)
- [Gotchas](#gotchas)

## Open and initialize

```python
from depz_sensor_sdk import Tmf8829, open_device

dev = open_device("/dev/ttyACM0")      # returns a Tmf8829 for USB PID 0xED4C
assert isinstance(dev, Tmf8829)
version, serial = dev.init()           # soft reset + 14.4 KB firmware download, ~0.15 s
print(version, hex(serial))            # e.g. "2.200.0" — the sensor application
```

`init()` brings the sensor from any state to its running application: soft
reset, the sensor bootloader, the firmware download, start. It returns the
application version and the sensor's serial number (also kept as
`dev.app_version` / `dev.sensor_serial`). `progress=lambda done, total: …`
tracks the download. A running stream is stopped first, and the last
configuration is forgotten (`dev.config` is `None` again).

You can also open the class directly: `Tmf8829("/dev/ttyACM0")`.

## Hello-world: a depth image

```python
from depz_sensor_sdk import Tmf8829, open_device

with open_device("/dev/ttyACM0") as dev:
    assert isinstance(dev, Tmf8829)
    dev.init()
    dev.configure("16x16", period_ms=1)        # as fast as the sensor goes
    dev.start()
    for m in dev.measurements():               # blocks; Ctrl+C to stop
        d = m.distance_mm[0]                   # first peak, 16×16 numpy array, mm
        print(f"{m.timestamp_us} us  centre {d[8, 8]:.0f} mm  flags {m.flags:#x}")
```

Leaving the `with` block stops the stream and closes the port.

## Presets and configuration

```python
cfg = dev.configure("32x32", nr_peaks=2, signal_strength=True, period_ms=1)
print(cfg.grid)                  # (32, 32) — rows, cols
print(cfg.fields["kilo_iterations"], cfg.fields["period_ms"])
```

`configure(preset, …)` loads the preset, sets the fields, writes the page and
reads it back. It returns a `Tmf8829Config` (`preset`, the 190-byte `page`,
`histograms`, `ref_frame`, every named field in `fields`, and `grid`). Not
while streaming: `stop()` first.

| preset | grid (rows × cols) | note |
|---|---|---|
| `8x8`, `8x8-lr`, `8x8-ha` | 8 × 8 | default, long range, high accuracy |
| `8x8-xr` | 8 × 8 | extended range; the sensor reports whole mm |
| `16x16`, `16x16-ha` | 16 × 16 | |
| `32x32`, `32x32-ha` | 32 × 32 | two half-frames per measurement |
| `48x32`, `48x32-ha` | 32 × 48 | two half-frames per measurement |

Keyword arguments and their defaults — **all of them are written on every
call**, because a preset load keeps what the previous config left in these
fields:

| argument | default | meaning |
|---|---|---|
| `period_ms` | 33 | time between measurements; 1 = back to back |
| `kilo_iterations` | the preset's (586 / 2344 / 3516) | integration length, thousands of laser pulses |
| `nr_peaks` | 1 | targets per zone, 0..4 |
| `signal_strength` | `False` | add per-peak signal strength |
| `noise_strength` | `False` | add per-zone noise |
| `xtalk` | `False` | add per-zone crosstalk |
| `histograms` | `False` | dump raw histograms (`dump_histograms`) |
| `ref_frame` | `False` | emit a ref-SPAD frame before each results frame |
| `fields` | `None` | any other page field by name, e.g. `{"dual_mode": 1}` |

The named fields are in `CFG_FIELDS` (`power_modes`, `ha_kilo_iterations`,
`dual_mode`, `fov_correction`, `distance_in_mm`, …). The sensor refuses a
result format too large for the grid (`Tmf8829Error` … `ERR_CONFIG_RESULT_SIZE`:
4 peaks with every strength on 48×32, for instance), and `configure()` raises
if a written field does not read back.

`dev.config` is the last result; `dev.config_page()` reads the page now in the
sensor. Raw register access is there for anything else:
`read_registers(addr, n)`, `write_registers(addr, data)`,
`read_register(addr)`, `write_register(addr, value)`, and `sensor_state()`
for a diagnostic dump.

## The measurement

`Tmf8829Measurement` is one full measurement — one results frame (8×8,
16×16) or both half-frames merged (32×32, 48×32) — with the frames that
belong to it.

| field | meaning |
|---|---|
| `timestamp_us` | capture time, MCU µs (mean of the half-frame times) |
| `flags` | capture-time flags of its results frames, OR-ed |
| `distance_mm` | `[peaks, rows, cols]` float, mm, clock-corrected; `None` with 0 peaks |
| `results` | `PixelResults`: raw `distance`, `snr`, `signal` `[peaks, rows, cols]`, `noise`, `xtalk` `[rows, cols]` (`None` when absent) |
| `shape`, `peaks` | `(rows, cols)` and peaks per zone |
| `fp_mode`, `f_number` | focal-plane mode (grid) and the sensor's frame counter |
| `temperature_c` | sensor die temperature, °C |
| `frames`, `times` | `FrameInfo` (header/footer) and `FrameTime` of each results frame |
| `histograms`, `ref_frames` | the histogram and ref-SPAD frames before it |
| `problems` | what went wrong since the previous measurement (strings) |

A distance of 0 means no target in that zone/peak. `results.distance` is the
raw integer (0.25 mm, or mm with `8x8-xr`) **before** the clock correction —
use `distance_mm` unless you need the sensor's own numbers.

The arrays keep the sensor's row-major order: for a board held USB down and
seen from behind, the row index runs left → right and the column index
top → bottom. To get the scene as you see it, transpose:

```python
scene = m.distance_mm[0].T       # rows = top → bottom, cols = left → right
```

## Streaming

```python
dev.start()                      # configures the default 8x8 preset if needed
it = dev.measurements(maxsize=64)   # iterator — bounded, drop-oldest
m = next(it)
print("dropped so far:", it.dropped_count)

unsub = dev.on_measurement(lambda m: q.put(m))   # callback on the reader thread
dev.stop()
```

`measurements()` subscribes immediately — call it before or after `start()`.
Callbacks run on the reader thread: keep them short. Per-stream counters:
`dev.frames_dropped` (frames the bridge dropped), `dev.frames_discarded`
(lost to reassembly), `dev.frame_parse_errors` — all 0 in a healthy session.
`dev.streaming` tells whether a stream runs.

## Histograms and ref-SPAD frames

```python
dev.configure("8x8", histograms=True, ref_frame=True)
dev.start()
m = next(dev.measurements())
for h in m.histograms:            # HistogramFrame, 25 377 B each
    print(h.info.id, len(h.data)) # header/footer + the raw bins
print(len(m.ref_frames))          # 65 B each: zero pre-header + register 0x20
```

8×8 and 16×16 dump 2 histogram frames per measurement, 32×32 8, 48×32 12
(twice that with `dual_mode`). Decoding the bins is left to you (ams
histogram layout). Histograms are a calibration mode: the stream drops to
~10 measurements/s on 8×8 and under 2/s on 48×32, and the capture time gets
less precise (see below). Turn them off with the next
`configure(…)` — the default `histograms=False` does that.

## Capture time and flags

`timestamp_us` is the centre of the measurement's capture window, in MCU
microseconds — not the time the frame reached the USB. Map it onto the host
clock with `dev.sync_time()` + `dev.to_host_time_us(m.timestamp_us)`.

```python
from depz_sensor_sdk.tmf8829 import flag_names

print(flag_names(m.flags))        # e.g. ['nominal'] for the first ~0.2 s
t = m.times[0]                    # FrameTime of the first (half-)frame
print(t.t0_us, t.t1_us, t.t1_len_us, t.ratio_q16 / (8 << 16))
```

The constants live in `depz_sensor_sdk.tmf8829.clock`; `flag_names()` gives
the lowercase names.

| flag (constant, value, name) | meaning |
|---|---|
| `FT_NOMINAL` 0x01 `nominal` | clock ratio not measured yet (first ~0.2 s of a stream) |
| `FT_CLOCK_RESET` 0x02 `clock_reset` | the clock model restarted at this frame |
| `FT_EDGE_LATE` 0x04 `edge_late` | the interrupt came > 2 ms later than usual |
| `FT_STALLED` 0x08 `stalled` | the sub-integrations were not back to back (histogram wait) |
| `FT_T1_UNSURE` 0x10 `t1_unsure` | 32×32 / 48×32 with histograms: time up to ~20 ms late |

## Recording

`SessionRecorder` records TMF8829 measurements like any other sensor
(contract 09 record kind `tmf8829`: one record per measurement, flat
`distance_mm` in mm rounded to 0.1, `snr`, and `signal` / `noise` / `xtalk`
when present; histograms and ref-SPAD frames are not recorded):

```python
import time
from depz_sensor_sdk import SessionRecorder

dev.configure("32x32", period_ms=1)
with SessionRecorder("tof.depzdata") as rec:
    rec.add(dev, device_id="tmf")
    rec.start()
    dev.start()
    time.sleep(5)
    dev.stop()
```

## Bridge diagnostics and recovery

```python
info = dev.get_bridge_info()      # Tmf8829Info, safe mid-stream
print(info.frames, info.frames_aborted, info.frame_errors, info.streaming)
```

`Tmf8829InterruptEvent`s arrive through `dev.on_event(...)`: a motion /
proximity interrupt, or a refused frame header. The single bad header a
`stop()` causes during a histogram dump is expected and swallowed; any other
means the sensor is lost — run `dev.init()` and `configure()` again. If
`init()` itself fails, only re-plugging the USB cable (a power cycle) helps.
There is no sleep / standby: the sensor's enable pin is wired on.

## Decoding without a device

The frame layer is pure and works on captured bytes:

```python
from depz_sensor_sdk.protocol.tmf8829 import FrameChunk, FrameReassembler
from depz_sensor_sdk.tmf8829 import MeasurementBuilder, firmware_image

reasm, builder = FrameReassembler(), MeasurementBuilder()
for payload in rpt_tmf_frame_payloads:          # RPT_TMF_FRAME (0x93) payloads
    raw = reasm.feed(FrameChunk.unpack(payload))
    if raw is not None:                         # a whole frame
        m = builder.add(raw.data, raw.timestamp_us, raw.edge_us)
        if m is not None:
            print(m.shape, m.timestamp_us)

print(len(firmware_image()))                    # 14 444 B, ams MIT image
```

`parse_frame(raw)` gives the header/footer, `parse_results(raw, info)` the
pixels of one results frame, `merge_halves(first, second)` joins two halves;
`cfg_get(page, name)` / `cfg_set(page, name, value)` read and write config
page fields.

## Gotchas

- **`configure()` writes everything every time** — a preset load keeps the
  previous result format and histogram setting; a stale 4-peak format can
  make the next preset fail with `ERR_CONFIG_RESULT_SIZE`. Always pass what
  you want; the defaults reset the rest.
- **Configure while stopped** — `configure()` raises while streaming.
- **Row index = left → right** — the arrays are transposed against the
  scene; transpose for display.
- **`results.distance` is raw** — 0.25 mm units (mm with `8x8-xr`) of the
  sensor's slow clock; `distance_mm` is the corrected value.
- **Histograms are slow** — and make 32×32 / 48×32 capture times
  `t1_unsure`. Use them for calibration, not for live depth.
- **No sleep** — recovery is `init()`; there is no power-mode API.
