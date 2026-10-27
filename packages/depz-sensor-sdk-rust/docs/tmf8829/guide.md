# TMF8829 — user guide

Hands-on guide to the TMF8829: the live device (`devices::Tmf8829Device`, which
runs the sensor) and the [`tmf8829`](api.md) codec and decode layer under it.
For what the sensor is and its concepts, read the
[introduction](introduction.md); for exact signatures see the
[API reference](api.md).

The sections after [Live device](#live-device) are the layers under the device:
the bridge commands, the sensor sequences as plain register writes, and the
chunked frame stream turned into measurements — what the device does for you,
the rules the sensor imposes listed where they matter.

## Contents

- [Live device](#live-device)
- [Commands](#commands)
- [Running the sensor: init, configure, start, stop](#running-the-sensor-init-configure-start-stop)
- [Hello-world: decode the stream](#hello-world-decode-the-stream)
- [The measurement](#the-measurement)
- [Frames by hand](#frames-by-hand)
- [Capture time and the clock model](#capture-time-and-the-clock-model)
- [Config page](#config-page)
- [Sensor firmware image](#sensor-firmware-image)
- [Events and bridge info](#events-and-bridge-info)
- [Gotchas](#gotchas)

## Live device

```rust
use std::time::Duration;
use depz_sensor_sdk::devices::{open_first_device, Tmf8829Options};

let tof = open_first_device(None)?.into_tmf8829()?;
let init = tof.init(None)?;                             // firmware download, ~0.15 s
println!("app {} serial {:08X}", init.app_version, init.sensor_serial);
let cfg = tof.configure("16x16", &Tmf8829Options { nr_peaks: 2, ..Default::default() })?;
println!("{:?} zones", cfg.grid());                    // (16, 16)
let depth = tof.measurements(16);
tof.start()?;
let m = depth.take(Duration::from_secs(1))?;            // a Tmf8829Measurement
println!("fN {} at {} µs, {:?} mm", m.f_number, m.timestamp_us, m.distance_mm.as_ref().map(|d| d[0]));
tof.stop()?;
# Ok::<(), depz_sensor_sdk::devices::DepzError>(())
```

- `init(progress)` — soft reset, bootloader, firmware download, start; works
  from any state (recovery = init) and stops a running stream first.
  `app_version()`, `sensor_serial()`.
- `configure(preset, &opts)` — one of the ten [`PRESETS`](api.md#presets);
  writes every field a preset does not reset (period, `kilo_iterations`,
  peaks, signal / noise / crosstalk, histograms, ref frame, plus
  `opts.fields` by name) and checks it reads back. `config()` is the last
  `Tmf8829AppliedConfig` (`grid()`, `field(name)`, the raw `page`). Not
  while streaming.
- `start()` (the default 8x8 preset when nothing was configured) / `stop()`;
  measurements stream to `on_measurement(callback)`, `measurements(capacity)`
  and `get_measurement(timeout)`. `frames_dropped()`, `frames_discarded()`,
  `frame_parse_errors()` and `clock_ratio_q16()` restart with each stream.
- Events: a MOTION / PROXIMITY interrupt or a refused frame header arrives as
  `DeviceEvent::Sensor` of kind `Tmf8829Device::INTERRUPT_EVENT`
  (`Tmf8829Device::interrupt_event(&e)` decodes it); the bad header a stop
  raises mid histogram dump is swallowed.
- Sensor refusals and timeouts are `DepzError::Sensor` with the sensor's status
  name (`"sensor cmd 0x15 -> ERR_CONFIG_RESULT_SIZE"`).
- Diagnostics: `read_registers` / `write_registers` (chunked, FIFO-aware),
  `sensor_state()`, `config_page()`, `get_bridge_info()` (safe mid-stream).
- `close()` (and dropping the device) stops a running stream first.

## Commands

The bridge has five commands ([`Tmf8829Cmd`](api.md#tmf8829cmd), `0x32`,
`0x33`, `0x35`–`0x37`). The register address is one byte; a read is 1..1024
bytes ([`READ_MAX`](api.md#read_max)), a write 1..1023
([`WRITE_MAX`](api.md#write_max)). The encoders refuse anything else with a
`CodecError` — the bridge would drop an oversized write without a reply:

```rust
use depz_sensor_sdk::framing::{build_packet, CrcType};
use depz_sensor_sdk::tmf8829::{
    pack_get_info, pack_read_reg, pack_start_stream, pack_stop_stream, pack_write_reg,
    Tmf8829Cmd, INT_HISTOGRAMS, INT_RESULTS, SF_REF_FRAME,
};

let frame = |cmd: Tmf8829Cmd, payload: &[u8]| build_packet(cmd as u8, payload, 0, CrcType::None).unwrap();

let read_id = frame(Tmf8829Cmd::ReadReg, &pack_read_reg(0xE3, 1)?);        // ID, reads 0x9E
let soft_reset = frame(Tmf8829Cmd::WriteReg, &pack_write_reg(0xF7, &[0x40])?);
let start = frame(Tmf8829Cmd::StartStream, &pack_start_stream(INT_RESULTS, 0)?);
let start_all = frame(
    Tmf8829Cmd::StartStream,
    &pack_start_stream(INT_RESULTS | INT_HISTOGRAMS, SF_REF_FRAME)?,  // + histograms + ref frames
);
let stop = frame(Tmf8829Cmd::StopStream, &pack_stop_stream());
let info = frame(Tmf8829Cmd::GetInfo, &pack_get_info());
```

Replies: a register read answers `RPT_TMF_REG_DATA`
([`RegData::unpack`](api.md#regdata) → `timestamp_us`, `data`), everything
else `RPT_STATUS` OK. Only one register command may be outstanding — wait for
each reply. Use a 1 s timeout for register commands while streaming (a 25 KB
histogram frame in flight delays the reply), 200 ms otherwise.

## Running the sensor: init, configure, start, stop

These are register writes and reads over `TMF_WRITE_REG` / `TMF_READ_REG`;
contract 14 §4 is the authority. In short:

- **init** (from any state, ~0.1–0.15 s): `ENABLE 0xF8 = 0x14`, soft reset
  `0xF7 = 0x40`, check `ID 0xE3 == 0x9E`, wait for the bootloader
  (`0x00 == 0x80`), bootloader `I2C_OFF`, wait for `cpu_ready`, then
  `W_FIFO_BOTH` at [`FIRMWARE_LOAD_ADDR`](api.md#firmware_load_addr) with
  [`firmware_words()`](api.md#firmware_words) words, write
  [`firmware_image()`](api.md#firmware_image) to `0xFF` in ≤ 1023-byte pieces,
  `START_RAM_APP`, wait for `0x00 == 0x01`. Read the app version (`0x01..0x03`)
  and the serial (`0x1C`, u32).
- **configure**: write the preset command ([`preset_cmd`](api.md#preset_cmd))
  to `0x08`, `LOAD_CONFIG_PAGE` (`0x16`), read the 190-byte page at
  [`REG_CFG`](api.md#reg_cfg), set the fields (see [Config page](#config-page)),
  write the page back, `WRITE_PAGE` (`0x15`), load it again and check it.
- **start**: clear `FIFOSTATUS 0xFA = 0x30`, `INT_STATUS 0xE1 = 0xFF`,
  `INT_ENAB 0xE2 = INT_RESULTS [| INT_HISTOGRAMS]`, `TMF_START_STREAM`, then
  `MEASURE` (`0x08 = 0x10`, expect `0x01` ACCEPTED).
- **stop**: `STOP` (`0x08 = 0xFF`, wait for 0), `INT_ENAB = 0`,
  `INT_STATUS = 0xFF`, `TMF_STOP_STREAM`.

Application commands are written to `0x08`; poll it until it reads below
`0x10` — 0 OK, 1 ACCEPTED, anything else an error (4 =
`ERR_CONFIG_RESULT_SIZE`: the result format is too big for the grid).

## Hello-world: decode the stream

Frames arrive as unsolicited `RPT_TMF_FRAME` chunks of ≤ 1024 bytes.
[`FrameReassembler`](api.md#framereassembler) rebuilds the frames;
[`MeasurementBuilder`](api.md#measurementbuilder) turns them into
measurements. Reset both at every stream start:

```rust
use depz_sensor_sdk::framing::{Event, PacketParser};
use depz_sensor_sdk::tmf8829::{FrameChunk, FrameReassembler, MeasurementBuilder, Tmf8829Rpt};

let mut parser = PacketParser::new();
let mut reasm = FrameReassembler::new();
let mut builder = MeasurementBuilder::new();       // reset() both at every TMF_START_STREAM

for ev in parser.feed(&rx_bytes) {
    let Event::Packet(pkt) = ev else { continue };
    if pkt.cmd != Tmf8829Rpt::Frame as u8 {
        continue;
    }
    let chunk = FrameChunk::unpack(&pkt.payload)?;
    let Some(raw) = reasm.feed(chunk) else { continue };          // frame not complete yet
    if let Some(m) = builder.add(&raw.data, raw.timestamp_us, raw.edge_us) {
        let (rows, cols) = m.shape();
        if let Some(d) = &m.distance_mm {
            // d is [peaks][rows][cols], row-major: peak 0, centre zone
            println!("{} µs {rows}x{cols}: {:.1} mm", m.timestamp_us, d[(rows / 2) * cols + cols / 2]);
        }
    }
}
println!("dropped {} discarded {}", reasm.seq_gaps, reasm.discarded);
```

`seq_gaps` counts frames the bridge dropped (a gap in `frame_seq`),
`discarded` frames lost to reassembly (an incomplete last frame after a stop,
out-of-order chunks). Neither is fatal; the measurement after a loss carries
it in `problems`.

## The measurement

[`Tmf8829Measurement`](api.md#tmf8829measurement) is one results frame
(8×8, 16×16) or both halves merged (32×32, 48×32):

| field | meaning |
|---|---|
| `timestamp_us` | capture time, MCU µs (centre of the window; mean of the two halves) |
| `flags` | `FT_*` bits of its results frame(s), see [below](#capture-time-and-the-clock-model) |
| `fp_mode`, `shape()` | focal-plane mode; (rows, cols) — 8×8, 16×16, 32×32 or **32×48** |
| `peaks()` | peaks per zone, 0..4 |
| `distance_mm` | `Option<Vec<f64>>`, `[peaks][rows][cols]`, clock-corrected, mm |
| `results` | [`PixelResults`](api.md#pixelresults): the raw sensor integers |
| `f_number`, `temperature_c()` | frame number, die temperature (°C) |
| `frames`, `times` | the header/footer of each results frame and its [`FrameTime`](api.md#frametime) |
| `histograms` | the raw histogram frames before it (25 377 B each) |
| `ref_frames` | ref-SPAD frames (65 B: zero pre-header + register `0x20`) |
| `problems` | what went wrong since the previous measurement, human-readable |

`PixelResults` holds `distance`, `snr`, `signal` as `[peaks][rows][cols]` and
`noise`, `xtalk` as `[rows][cols]` — each `None` when the result format does
not carry it. Raw distances are 0.25 mm steps (mm with preset `8x8-xr`) **in
sensor-clock units**; `distance_mm` multiplies them by the measured clock
ratio. To convert by hand:

```rust
use depz_sensor_sdk::tmf8829::distance_mm;

let r = &m.results;
let raw = r.distance_at(0, 3, 5);                 // peak 0, row 3, col 5
let in_mm = m.frames[0].distance_in_mm();         // footer flag: preset 8x8-xr
let mm = distance_mm(r, builder.clock.ratio(), in_mm);
```

The arrays keep the sensor's order: for a board **USB down**, seen from behind,
the row index runs left → right and the column index top → bottom. Transpose
to draw the scene.

## Frames by hand

Without the builder, decode each frame yourself:

```rust
use depz_sensor_sdk::tmf8829::{
    fp_grid, merge_halves, parse_frame, parse_results_info, FID_HISTOGRAMS, FID_REF_SPAD,
    FID_RESULTS,
};

let f = parse_frame(&raw.data)?;                  // pre-header, header, footer
match f.kind() {
    FID_RESULTS => {
        let res = parse_results_info(&raw.data, &f)?;   // FrameError on a size mismatch
        println!("{:?} grid, {} peaks, problems {:?}", fp_grid(f.fp_mode()), res.peaks, f.problems());
    }
    FID_HISTOGRAMS => { /* raw bins, ams layout */ }
    FID_REF_SPAD => { /* 60 B of register 0x20 behind a zero pre-header */ }
    _ => {}
}
```

32×32 and 48×32 frames carry **half the rows** ([`frame_rows`](api.md#frame_rows)):
the first (`f.is_second_half() == false`) the even rows, the second the odd
ones. `merge_halves(&first, &second)?` interleaves them
(`full[2i] = first[i]`, `full[2i+1] = second[i]`) and refuses halves of
different formats. `f.problems()` lists a bad EOF, an invalid or aborted
frame status, the HV/VCDRV warnings and FIFO under/overruns — empty when fine.

## Capture time and the clock model

The frame's MCU timestamp is the moment the bridge read its header, **not**
when the sensor measured. The capture time comes from the sensor's own clock
(SYSTICK, 125 kHz nominal, ~1.5 % slow): [`SensorClock`](api.md#sensorclock)
learns the ratio from (MCU µs, SYSTICK) pairs, [`CaptureTimer`](api.md#capturetimer)
places each results frame's integration window on the MCU timeline. Both are
integer and bit-exact with `contracts/vectors/tmf8829_frame_time.json`.
`MeasurementBuilder` runs them for you; by hand, per frame in stream order:

```rust
use depz_sensor_sdk::tmf8829::{flag_names, parse_frame, CaptureTimer, SensorClock, FID_HISTOGRAMS, FID_REF_SPAD, FID_RESULTS};

let mut clock = SensorClock::new();               // reset both at every stream start
let mut timer = CaptureTimer::new();

let f = parse_frame(&raw.data)?;
if f.kind() != FID_REF_SPAD {                     // ref-SPAD frames have no clock pair
    let flags = clock.add(raw.timestamp_us, f.systick);
    if f.kind() == FID_HISTOGRAMS {
        timer.histograms();
    } else if f.kind() == FID_RESULTS {
        let t = timer.results_frame(&clock, &f, raw.timestamp_us, raw.edge_us, flags);
        println!("captured at {} µs (T1 {} µs, ratio {:.4}) {:?}",
                 t.timestamp_us, t.t1_len_us, clock.ratio(), flag_names(t.flags));
    }
}
```

Flags ([`FT_*`](api.md#ft_nominal)): `FT_NOMINAL` — ratio not measured yet
(the first 200 ms); `FT_CLOCK_RESET` — the model restarted (SYSTICK jump);
`FT_EDGE_LATE` — the INT edge came later than usual; `FT_STALLED` — the
sub-integrations were not back to back (histogram mode); `FT_T1_UNSURE` —
32×32/48×32 with histograms, the time may be up to ~20 ms late. Map MCU time
to host time with SYNC_TIME, as for every board.

## Config page

The page is the 190 bytes `0x22..0xDF` ([`CFG_SIZE`](api.md#cfg_size)), index 0
= register `0x22`. [`cfg_get`](api.md#cfg_get) / [`cfg_set`](api.md#cfg_set)
read and write a field by name ([`CFG_FIELDS`](api.md#cfg_fields): `period_ms`,
`kilo_iterations`, `fp_mode`, `nr_peaks`, `signal_strength`, `noise_strength`,
`xtalk`, `dump_histograms`, `ref_spad_frame`, `power_modes`,
`ha_kilo_iterations`, `dual_mode`, `fov_correction`, `distance_in_mm`).

**A preset load does not reset the result format, the histogram dump, the ref
frame or the period** — they keep what the previous config wrote (and a stale
4-peak format makes `32x32` fail with `ERR_CONFIG_RESULT_SIZE`). So write all
of them every time, with these defaults unless you want otherwise:

```rust
use depz_sensor_sdk::tmf8829::{cfg_get, cfg_set, config_fields, preset_cmd, preset_kilo_iterations, DEFAULT_PERIOD_MS};

let preset = "32x32";
let load = preset_cmd(preset).unwrap();                  // 0x45 → write to 0x08, then LOAD_CONFIG_PAGE
let mut page = page_read_back.clone();                   // 190 B read at 0x22
cfg_set(&mut page, "period_ms", DEFAULT_PERIOD_MS as u32)?;    // 33
cfg_set(&mut page, "kilo_iterations", preset_kilo_iterations(preset).unwrap() as u32)?;  // 2344
cfg_set(&mut page, "nr_peaks", 2)?;                      // 1..4
for name in ["signal_strength", "noise_strength", "xtalk", "dump_histograms", "ref_spad_frame"] {
    cfg_set(&mut page, name, 0)?;
}
// write `page` at 0x22, WRITE_PAGE, LOAD_CONFIG_PAGE, read back and compare:
assert_eq!(cfg_get(&back, "nr_peaks")?, 2);
println!("{:?}", config_fields(&back));
```

The ten presets ([`PRESETS`](api.md#presets)): `8x8`, `8x8-lr` (long range),
`8x8-ha` (high accuracy), `16x16`, `16x16-ha`, `32x32`, `32x32-ha`, `48x32`,
`48x32-ha`, `8x8-xr` (extended range, distances in mm). `cfg_set` refuses a
value that does not fit the field; `fov_correction` does not stick on
application 2.200.0 (written 3, reads 5).

## Sensor firmware image

```rust
use depz_sensor_sdk::tmf8829::{firmware_image, firmware_words, FIRMWARE_LICENSE, FIRMWARE_LOAD_ADDR};

assert_eq!(firmware_image().len(), 14_444);      // ams application 2.200.0
assert_eq!(firmware_words(), 3611);              // W_FIFO_BOTH word count
assert_eq!(FIRMWARE_LOAD_ADDR, 0x0001_0000);
```

The image is ams-OSRAM's `tmf8829_application.hex`, **MIT-licensed**. Any
binary you ship that embeds it must carry the license text —
[`FIRMWARE_LICENSE`](api.md#firmware_license) holds it.

## Events and bridge info

`RPT_TMF_EVENT` ([`Tmf8829Event`](api.md#tmf8829event)) reports a
MOTION/PROXIMITY interrupt (`code == EV_INT`) or a refused frame header
(`EV_BAD_HEADER`). One bad header is **expected** when you stop a stream mid
histogram dump — ignore it during stop; any other means the sensor is lost:
run init again.

`RPT_TMF_INFO` ([`Tmf8829Info`](api.md#tmf8829info), 25 B) has the bridge
counters; it never touches the sensor and is safe to poll mid-stream. It has
**no echoed command byte** — a reply matcher keyed on `payload[0] == cmd` must
exempt it:

```rust
use depz_sensor_sdk::tmf8829::{Tmf8829Event, Tmf8829Info, EV_BAD_HEADER};

let info = Tmf8829Info::unpack(&info_payload)?;
println!("streaming {} frames {} aborted {} errors {} spi timeouts {}",
         info.streaming, info.frames, info.frames_aborted, info.frame_errors, info.spi_timeouts);

let ev = Tmf8829Event::unpack(&event_payload)?;
if ev.code == EV_BAD_HEADER && !stopping {
    // the sensor is lost: re-run the init sequence
}
```

## Gotchas

- **Doing it by hand** — the sequences above are what `Tmf8829Device` sends;
  only one register command may be outstanding at a time.
- **No sleep, recovery = init** — the enable pin is wired high; init starts
  with a soft reset and works from any state. If it fails, only a USB re-plug
  (power cycle) is left.
- **Write every result-format field on each configuration** — a preset load
  keeps the old values.
- **Reset the reassembler, the builder (or clock + timer) at every stream
  start** — `frame_seq` and the clock model are per stream.
- **Never stitch chunks of different `frame_seq`** — `FrameReassembler` does
  it right; don't roll your own without the same checks.
- **32×32 / 48×32 are two half-frames** — a measurement needs both;
  48×32 is **32 rows × 48 columns**.
- **Distances need the clock ratio** — the raw integers are in sensor-clock
  units; use `distance_mm` (or `Tmf8829Measurement::distance_mm`), and the
  footer flag for mm vs 0.25 mm.
- **Histograms slow the stream down** — to ~10 measurements/s (8×8/16×16) and
  under 2 (48×32); USB is then the limit.
- **Expect one bad-header event on stop** with histograms on.
