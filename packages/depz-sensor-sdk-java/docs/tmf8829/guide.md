# TMF8829 — user guide

Hands-on guide to the TMF8829 live device
(`ai.depz.sensor.devices.Tmf8829Device`) and the layers under it: the wire codecs ([`Tmf8829`](api.md#tmf8829)) and
the decode classes in `ai.depz.sensor.sensors.tmf8829`. For what the sensor is
and its concepts, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md).

## Contents

- [Live device](#live-device)
- [Commands](#commands)
- [Sensor firmware](#sensor-firmware)
- [Presets and the config page](#presets-and-the-config-page)
- [Hello-world: decode the stream](#hello-world-decode-the-stream)
- [The measurement](#the-measurement)
- [Frames by hand](#frames-by-hand)
- [Capture time and the sensor clock](#capture-time-and-the-sensor-clock)
- [Bridge info and events](#bridge-info-and-events)
- [Gotchas](#gotchas)

## Live device

```java
import ai.depz.sensor.devices.*;
import ai.depz.sensor.sensors.tmf8829.MeasurementBuilder.Measurement;
import java.time.Duration;

try (Tmf8829Device tof = DepzDevices.openTmf8829()) {
    Tmf8829Device.InitResult id = tof.init();           // soft reset, 14.4 KB firmware download, ~0.15 s
    System.out.println("application " + id.appVersion() + ", sensor serial " + id.sensorSerial());
    Tmf8829AppliedConfig cfg = tof.configure("32x32", 100, null, 2, false, false, false, false, false, null);
    System.out.println(cfg.grid()[0] + "x" + cfg.grid()[1] + ", fp_mode " + cfg.fields().get("fp_mode"));

    try (Subscription<Measurement> frames = tof.measurements()) {
        tof.start();
        Measurement m = frames.take(Duration.ofSeconds(2));
        int rows = m.shape()[0], cols = m.shape()[1];
        System.out.println(rows + "x" + cols + ", " + m.peaks() + " peak(s), centre "
                + m.distanceMm()[rows / 2 * cols + cols / 2] + " mm");
        tof.stop();
    }
}
```

- `init()` brings the sensor from **any** state to its running application
  (it starts with a soft reset), so it is also the recovery: after a
  `Tmf8829InterruptEvent` with code 1 (a refused frame header) run it again.
- `configure(preset, …)` loads a preset (`Tmf8829Config.PRESETS`: 8x8 …
  48x32) and writes **every** field a preset does not reset — the result never
  depends on an earlier config; the `fields` map sets any other page field by
  name. A field that does not read back, or a page the sensor refuses (e.g.
  `ERR_CONFIG_RESULT_SIZE`: too many peaks / strengths for the grid), throws
  `Tmf8829Exception`. It throws `IllegalStateException` while streaming.
- `start()` configures 8x8 when `configure` was not called since `init`;
  `histograms` / `refFrame` add the histogram and ref-SPAD frames to each
  measurement (`m.histograms()`, `m.refFrames()`).
- Frame loss is counted, not raised: `framesDropped()` (bridge gaps),
  `framesDiscarded()` (incomplete reassembly), `frameParseErrors()`;
  `clockRatioQ16()` is the live clock model. `close()` stops a running stream.
- `readRegisters` / `writeRegisters` / `sensorState()` / `getBridgeInfo()`
  are the escape hatches.

The sections below are the layers under the device; the register sequence
itself is in contract 14 §4.

## Commands

The bridge has five commands (`Tmf8829.Tmf8829Cmd`, `0x32`/`0x33`/`0x35`–`0x37`).
Register addresses are one byte; a read is 1..1024 bytes (`READ_MAX`), a write
1..1023 (`WRITE_MAX`). The encoders refuse anything else before it is sent:

```java
import ai.depz.sensor.protocol.Tmf8829;
import ai.depz.sensor.transport.CrcType;
import ai.depz.sensor.transport.Framing;

byte[] readId = Framing.buildPacket(Tmf8829.Tmf8829Cmd.READ_REG.value,
        Tmf8829.packReadReg(0xE3, 1), 0, CrcType.NONE);              // ID: the sensor answers 0x9E
byte[] measure = Framing.buildPacket(Tmf8829.Tmf8829Cmd.WRITE_REG.value,
        Tmf8829.packWriteReg(0x08, new byte[] {0x10}), 0, CrcType.NONE);  // CMD_STAT = MEASURE
byte[] start = Framing.buildPacket(Tmf8829.Tmf8829Cmd.START_STREAM.value,
        Tmf8829.packStartStream(Tmf8829.INT_RESULTS | Tmf8829.INT_HISTOGRAMS, Tmf8829.SF_REF_FRAME),
        0, CrcType.NONE);
byte[] stop = Framing.buildPacket(Tmf8829.Tmf8829Cmd.STOP_STREAM.value);
byte[] info = Framing.buildPacket(Tmf8829.Tmf8829Cmd.GET_INFO.value);
```

`TMF_START_STREAM` takes the `INT_STATUS` bits the bridge serves
(`INT_RESULTS`, plus `INT_HISTOGRAMS` when the config dumps histograms) and
`SF_REF_FRAME` to emit the ref-SPAD frame before each results frame. It does
not touch the sensor: the host writes `INT_ENAB` and sends `MEASURE` itself.
Send one register command at a time and wait for its reply (a second one
queued behind it gets `ERR_BUSY`); allow **1 s** for register commands while
streaming — a 25 KB histogram frame in flight delays the reply.

## Sensor firmware

The sensor boots into its bootloader, and every init downloads the
application into its RAM. The image is embedded on the classpath:

```java
import ai.depz.sensor.sensors.tmf8829.Tmf8829Firmware;

byte[] image = Tmf8829Firmware.image();                 // 14 444 B = 3611 words, MIT
for (int off = 0; off < image.length; off += Tmf8829.WRITE_MAX) {
    byte[] piece = Arrays.copyOfRange(image, off, Math.min(off + Tmf8829.WRITE_MAX, image.length));
    send(Framing.buildPacket(Tmf8829.Tmf8829Cmd.WRITE_REG.value,
            Tmf8829.packWriteReg(0xFF, piece), 0, CrcType.NONE));   // wait for each RPT_STATUS OK
}
String license = Tmf8829Firmware.license();             // ship it with your product
long loadAddr = Tmf8829Firmware.LOAD_ADDR;              // 0x00010000, for BL W_FIFO_BOTH
```

The pieces go to the FIFO port `0xFF` (writes there do not advance the
address), after the bootloader's `W_FIFO_BOTH` command with `LOAD_ADDR` and
the word count; `START_RAM_APP` then starts the application (contract 14 §4).
The image is ams-OSRAM's `tmf8829_application.hex`, application 2.200.0,
under the **MIT** license — keep `LICENSES-MIT.TXT` with it.

## Presets and the config page

A preset is a load command written to `CMD_STAT` (`0x08`); it fills the
190-byte config page (`0x22..0xDF`, `REG_CFG` / `CFG_SIZE`), which the host
reads, edits and writes back. `Tmf8829Config` knows the presets and the named
fields:

```java
import ai.depz.sensor.sensors.tmf8829.Tmf8829Config;

String preset = "32x32";
int load = Tmf8829Config.PRESETS.get(preset);                     // 0x45: write it to 0x08
byte[] page = Tmf8829.RegData.unpack(regDataPayload).data();     // READ 0x22, 190 B
Tmf8829Config.cfgSet(page, "period_ms", Tmf8829Config.DEFAULT_PERIOD_MS);
Tmf8829Config.cfgSet(page, "kilo_iterations", Tmf8829Config.PRESET_KILO_ITERATIONS.get(preset));
Tmf8829Config.cfgSet(page, "nr_peaks", 2);
Tmf8829Config.cfgSet(page, "signal_strength", 1);
Tmf8829Config.cfgSet(page, "noise_strength", 0);
Tmf8829Config.cfgSet(page, "xtalk", 0);
Tmf8829Config.cfgSet(page, "dump_histograms", 0);
Tmf8829Config.cfgSet(page, "ref_spad_frame", 0);
byte[] write = Framing.buildPacket(Tmf8829.Tmf8829Cmd.WRITE_REG.value,
        Tmf8829.packWriteReg(Tmf8829Config.REG_CFG, page), 0, CrcType.NONE);
// then WRITE_PAGE (0x15), LOAD_CONFIG_PAGE (0x16), read the page back and compare:
Map<String, Integer> back = Tmf8829Config.configFields(page);  // every named field
```

| preset | grid (rows × cols) | `kilo_iterations` |
|---|---|---|
| `8x8`, `8x8-lr`, `8x8-ha` | 8 × 8 | 586 |
| `8x8-xr` | 8 × 8, distances in mm | 586 |
| `16x16`, `16x16-ha` | 16 × 16 | 586 |
| `32x32`, `32x32-ha` | 32 × 32 | 2344 |
| `48x32`, `48x32-ha` | 32 × 48 | 3516 |

`Tmf8829Config.CFG_FIELDS` names every field with its register, bit shift and
width: `period_ms`, `kilo_iterations`, `fp_mode`, `nr_peaks`,
`signal_strength`, `noise_strength`, `xtalk`, `dump_histograms`,
`ref_spad_frame`, `power_modes`, `ha_kilo_iterations`, `dual_mode`,
`fov_correction`, `distance_in_mm`. `cfgSet` refuses a value that does not fit
the field's width. **Write all eight fields above on every configure**: a
preset keeps what the previous config wrote (a stale 4-peak format makes the
next `32x32` fail `WRITE_PAGE` with `ERR_CONFIG_RESULT_SIZE`).

## Hello-world: decode the stream

`RPT_TMF_FRAME` reports carry the frames in chunks of up to 1024 bytes. The
`FrameReassembler` puts them back together; the `MeasurementBuilder` turns
frames in stream order into measurements:

```java
import ai.depz.sensor.protocol.Tmf8829;
import ai.depz.sensor.sensors.tmf8829.MeasurementBuilder;
import ai.depz.sensor.sensors.tmf8829.SensorClock;
import ai.depz.sensor.transport.Event;
import ai.depz.sensor.transport.Packet;
import ai.depz.sensor.transport.PacketParser;

PacketParser parser = new PacketParser();
Tmf8829.FrameReassembler reasm = new Tmf8829.FrameReassembler();
MeasurementBuilder builder = new MeasurementBuilder();   // reset() both at every stream start

for (Event ev : parser.feed(rxBytes)) {
    if (!(ev instanceof Packet p) || p.cmd() != Tmf8829.Tmf8829Rpt.FRAME.value) {
        continue;
    }
    Tmf8829.RawFrame raw = reasm.feed(Tmf8829.FrameChunk.unpack(p.payload()));
    if (raw == null) {
        continue;                                    // frame not complete yet
    }
    MeasurementBuilder.Measurement m = builder.add(raw.data(), raw.timestampUs(), raw.edgeUs());
    if (m == null) {
        continue;                                    // histogram / ref-SPAD / first half-frame
    }
    int rows = m.shape()[0], cols = m.shape()[1];
    double centre = m.distanceMm()[(rows / 2) * cols + cols / 2];   // peak 0, middle zone
    System.out.printf("%d µs  %dx%d  centre %.1f mm  %s%n",
            m.timestampUs(), rows, cols, centre, SensorClock.flagNames(m.flags()));
}
System.out.println("dropped " + reasm.seqGaps + ", discarded " + reasm.discarded
        + ", parse errors " + builder.parseErrors);
```

The reassembler never stitches chunks of different `frame_seq`: an
out-of-order or missing chunk discards the frame in progress (`discarded`),
and a jump in `frame_seq` means the bridge dropped whole frames (`seqGaps`) —
count them, do not fail. Call `reset()` on both objects when a stream starts:
`frame_seq` restarts at 0 and the clock model is per stream.

## The measurement

`MeasurementBuilder.Measurement` is one whole measurement: its results frame
(8×8, 16×16) or both half-frames merged (32×32, 48×32), with the histogram and
ref-SPAD frames that came before it.

| field | meaning |
|---|---|
| `timestampUs()` | capture time, MCU µs (centre of the capture window; mean of the two halves) |
| `flags()` | `FT_*` bits of its results frames, OR-ed (`SensorClock.flagNames`) |
| `fpMode()`, `shape()` | focal-plane mode 0..5 and `{rows, cols}` of the full grid |
| `fNumber()` | the sensor's measurement counter |
| `distanceMm()` | `[peaks][rows][cols]` flat, clock-corrected, mm; `null` without peaks |
| `results()` | `Frames.PixelResults`: the raw integers — `distance`, `snr`, `signal` `[peaks][rows][cols]`, `noise`, `xtalk` `[rows][cols]`, `null` when absent |
| `temperatureC()` | sensor die temperature, °C |
| `frames()`, `times()` | the `FrameInfo` and `CaptureTimer.FrameTime` of each results frame |
| `histograms()` | raw `Frames.HistogramFrame`s (25 377 B each; bin decode is left to you) |
| `refFrames()` | ref-SPAD frames, 65 B each (zero pre-header + register `0x20`) |
| `problems()` | what went wrong since the previous measurement, human-readable |

Arrays are row-major in the sensor's order: index `(p * rows + r) * cols + c`.
Remember the orientation — with the board USB down and seen from behind, `r`
runs left → right and `c` top → bottom; transpose for a picture of the scene.

## Frames by hand

The builder is built from public pieces you can use directly:

```java
import ai.depz.sensor.sensors.tmf8829.Frames;
import ai.depz.sensor.sensors.tmf8829.MeasurementBuilder;

Frames.FrameInfo f = Frames.parseFrame(raw);           // pre-header, header, footer
if (f.kind() == Frames.FID_RESULTS) {
    Frames.PixelResults r = Frames.parseResults(raw, f);   // raw integers, [peaks][rows][cols]
    double[] mm = MeasurementBuilder.distanceMm(r, ratio, f.distanceInMm());
    System.out.println(Frames.FP_NAMES[f.fpMode()] + " fN " + f.fNumber()
            + " temp " + f.temperature()[0] + " °C " + f.problems());
}
Frames.PixelResults full = Frames.mergeHalves(Frames.parseResults(rawFirst), Frames.parseResults(rawSecond));
```

- `kind()` is `FID_RESULTS` (`0x10`), `FID_HISTOGRAMS` (`0x20`) or
  `FID_REF_SPAD` (`0x30`); `fpMode()` indexes `FP_GRID` / `FP_NAMES`.
- `parseResults` throws `Frames.FrameError` when the data size does not match
  the pixel format — the frame is corrupt.
- `isSecondHalf()` marks the odd-row half of a 32×32/48×32 measurement;
  `mergeHalves(first, second)` interleaves them (`full[2i] = first[i]`,
  `full[2i+1] = second[i]`).
- `problems()` lists a bad EOF, a not-valid or aborted `frame_status`, the
  HV_CP / VCDRV warnings and the FIFO under/overrun flags.
- `distanceMm(r, ratio, inMm)` scales by the clock ratio (`SensorClock.ratio()`
  at that frame) and divides by 4 unless the frame is in mm.

## Capture time and the sensor clock

The frame time is computed on the host. Each frame's pre-header SYSTICK
(sensor clock, 125 kHz) and the chunk's `timestampUs` (MCU µs) form a clock
pair; `SensorClock` estimates the ratio between the clocks (~1.015) over a
sliding 1–2 s window, and `CaptureTimer` places the capture window of every
results frame:

```java
import ai.depz.sensor.sensors.tmf8829.CaptureTimer;
import ai.depz.sensor.sensors.tmf8829.Frames;
import ai.depz.sensor.sensors.tmf8829.SensorClock;

SensorClock clock = new SensorClock();
CaptureTimer timer = new CaptureTimer(clock);          // reset() both at every stream start

Frames.FrameInfo f = Frames.parseFrame(raw.data());
if (f.kind() != Frames.FID_REF_SPAD) {                 // ref-SPAD: no clock pair
    int flags = clock.add(raw.timestampUs(), f.systick());
    if (f.kind() == Frames.FID_HISTOGRAMS) {
        timer.histograms();
    } else {
        CaptureTimer.FrameTime t = timer.resultsFrame(f, raw.timestampUs(), raw.edgeUs(), flags);
        System.out.println(t.timestampUs() + " µs, T1 " + t.t1LenUs() + " µs, ratio " + clock.ratio());
    }
}
```

Feed the frames in stream order — that is exactly what `MeasurementBuilder`
does. The flags say how far to trust a time:

| flag | meaning |
|---|---|
| `FT_NOMINAL` | ratio not measured yet (first ~0.2 s): nominal 8 µs/tick used |
| `FT_CLOCK_RESET` | the clock model restarted at this frame (power cycle, 9.5 h wrap) |
| `FT_EDGE_LATE` | the INT edge came > 2 ms later than usual — the frame waited |
| `FT_STALLED` | sub-integrations not back to back (histogram FIFO wait) |
| `FT_T1_UNSURE` | 32×32/48×32 with histograms: the centre may be up to ~20 ms late |

All times are MCU microseconds; map them to host time with the time-sync of
the [SDK guide](../guide.md#time-sync-math).

## Bridge info and events

`RPT_TMF_INFO` (25 bytes) is the bridge's own state — safe to poll mid-stream.
Unlike the other replies it has **no echoed command byte**: a host that
matches replies by `payload[0] == cmd` must exempt it.

```java
Tmf8829.Tmf8829Info info = Tmf8829.Tmf8829Info.unpack(infoPayload);   // 25 B, no cmd echo
System.out.printf("frames %d, aborted %d, bad headers %d, SPI timeouts %d, streaming %b%n",
        info.frames(), info.framesAborted(), info.frameErrors(), info.spiTimeouts(), info.streaming());

Tmf8829.Tmf8829Event ev = Tmf8829.Tmf8829Event.unpack(eventPayload);
if (ev.code() == Tmf8829.EventCode.BAD_HEADER.value) {
    // expected once while stopping a histogram stream; otherwise the sensor is lost
}
```

`RPT_TMF_EVENT` is unsolicited: code `INT` is a MOTION/PROXIMITY interrupt,
code `BAD_HEADER` a frame header the bridge refused. A sensor `STOP` issued
in the middle of a histogram dump raises exactly one `BAD_HEADER` — ignore it
while stopping. Any other means the sensor is lost: run init again.

## Gotchas

- **`init()` is the recovery** — the EN pin is wired high, there is no sleep;
  from any state (a refused frame header, a hung sensor) `init()` starts over
  with a soft reset.
- **Write the whole result format on every configure** — a preset keeps the
  previous `nr_peaks`, strengths, histogram and ref-frame switches and period;
  `dump_histograms` even survives init.
- **0.25 mm, except `8x8-xr`** — check `FrameInfo.distanceInMm()` (or use
  `Measurement.distanceMm()`), and scale by the clock ratio.
- **32×32 and 48×32 come in halves** — a lone half is not a measurement;
  `MeasurementBuilder` drops a second half without a first and logs it in
  `problems()`.
- **The grid is transposed** against the scene (row = left → right, column =
  top → bottom, board USB down, seen from behind).
- **No sleep, recovery = init** — EN is wired high; if init fails, only a USB
  re-plug is left.
- **Discard the last frame after `TMF_STOP_STREAM`** — the stop drops a frame
  in flight, so the reassembler may hold an incomplete one.
- **Histograms are slow and big** — 2 to 12 frames of 25 377 B per
  measurement (×2 with `dual_mode`); USB becomes the limit.
