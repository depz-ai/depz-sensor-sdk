# TMF8829 — user guide

Hands-on guide to the TMF8829 codecs in `Depz.Sensor.Tmf8829`: the bridge wire
codecs ([`Tmf8829Wire`](api.md#tmf8829wire)), the frame decode
([`Tmf8829Frames`](api.md#tmf8829frames)), the measurement builder and clock
model, and the config page. For what the sensor is and its concepts, read the
[introduction](introduction.md); for exact signatures see the
[API reference](api.md).

This SDK does not drive the sensor: there is no `Tmf8829` device class. The
sensor sequence — `init()` with the firmware download, `configure()`,
`start()` / `stop()` — runs in the Python or TypeScript SDK (the C and C++
SDKs have it too). What follows is everything you need to build the commands
and decode what the board sends; the rules the sensor imposes are listed where
they matter.

## Contents

- [Commands](#commands)
- [Hello-world: decode a captured stream](#hello-world-decode-a-captured-stream)
- [The measurement](#the-measurement)
- [Reassembly and frames by hand](#reassembly-and-frames-by-hand)
- [Capture time and the clock model](#capture-time-and-the-clock-model)
- [Presets and the config page](#presets-and-the-config-page)
- [The sensor firmware image](#the-sensor-firmware-image)
- [Events and bridge info](#events-and-bridge-info)
- [Datasets](#datasets)
- [Gotchas](#gotchas)

## Commands

The bridge has five commands ([`Tmf8829Cmd`](api.md#tmf8829cmd)). Register
addresses are one byte; a read is 1..1024 bytes (`ReadMax`), a write
1..1023 (`WriteMax`). The sensor auto-increments the address on every byte
except at `0xFF` (the FIFO port):

```csharp
using Depz.Sensor.Tmf8829;
using Depz.Sensor.Transport;

byte[] readId = Framing.BuildPacket((int)Tmf8829Cmd.ReadReg,
    Tmf8829Wire.PackReadReg(0xE3, 1));                       // ID, expect 0x9E
byte[] softReset = Framing.BuildPacket((int)Tmf8829Cmd.WriteReg,
    Tmf8829Wire.PackWriteReg(0xF7, new byte[] { 0x40 }));

// Results + histograms, with the ref-SPAD frame before each results frame.
byte[] start = Framing.BuildPacket((int)Tmf8829Cmd.StartStream,
    Tmf8829Wire.PackStartStream(Tmf8829Wire.IntResults | Tmf8829Wire.IntHistograms,
                                Tmf8829Wire.SfRefFrame));
byte[] stop = Framing.BuildPacket((int)Tmf8829Cmd.StopStream);
byte[] info = Framing.BuildPacket((int)Tmf8829Cmd.GetInfo);
```

The encoders throw `ArgumentOutOfRangeException` before anything is sent for
an address outside 0..255, a length outside the limits, an empty or
out-of-range `int_mask`, or an unknown flag. A `RPT_TMF_REG_DATA` reply
decodes with `Tmf8829RegData.Unpack(pkt.Payload)` → `TimestampUs`, `Data`.

The sensor sequence itself is register traffic — contract 14 §4 lists it step
by step. In short: `init()` writes ENABLE and a soft reset, checks `ID`
(`0xE3`) = `0x9E`, drives the sensor bootloader to download
`Tmf8829Firmware.ImageSpan` to `Tmf8829Firmware.LoadAddr` in ≤ 1023-byte
writes to `0xFF`, and starts it; `start()` clears the FIFO / INT flags, writes
`INT_ENAB`, sends `TMF_START_STREAM`, then `MEASURE` (`0x10` to `0x08`);
`stop()` writes `STOP` (`0xFF` to `0x08`) and sends `TMF_STOP_STREAM`. Only
one register command may be outstanding: wait for each reply. Use
`Tmf8829Wire.RegisterTimeout` (1 s) while streaming — a 25 KB histogram frame
in flight delays the reply.

## Hello-world: decode a captured stream

Frames arrive as `RPT_TMF_FRAME` chunks. Reassemble them, then feed every
frame in stream order to one `Tmf8829MeasurementBuilder`:

```csharp
using Depz.Sensor.Tmf8829;
using Depz.Sensor.Transport;

var parser = new PacketParser();
var reasm = new Tmf8829FrameReassembler();
var builder = new Tmf8829MeasurementBuilder();     // Reset() both at every stream start

foreach (ParserEvent ev in parser.Feed(rxBytes))
{
    if (ev is not Packet pkt || pkt.Cmd != (int)Tmf8829Rpt.Frame)
        continue;
    Tmf8829RawFrame? raw = reasm.Feed(Tmf8829FrameChunk.Unpack(pkt.Payload));
    if (raw is null)
        continue;                                   // frame not complete yet
    Tmf8829Measurement? m = builder.Add(raw.Data, (long)raw.TimestampUs, (long)raw.EdgeUs);
    if (m is null)
        continue;                                   // histogram / ref / first half-frame
    var (rows, cols) = m.Shape;
    double? centre = m.DistanceMm?[(0 * rows + rows / 2) * cols + cols / 2];   // peak 0
    Console.WriteLine($"{m.TimestampUs} µs {rows}x{cols} centre {centre:F1} mm " +
                      $"flags [{string.Join(",", Tmf8829TimeFlags.FlagNames(m.Flags))}]");
}
```

The builder does the half-frame merge, attaches histogram and ref-SPAD frames,
runs the clock model and computes the capture time. The chunk and frame
timestamps are `ulong`; the builder and clock take `long` — cast as above.

## The measurement

`Tmf8829Measurement` is one complete measurement:

| field | meaning |
|---|---|
| `TimestampUs` | capture time, MCU µs (32×32/48×32: floor mean of the two halves) |
| `Flags` | `Tmf8829TimeFlags` of its results frame(s), OR-ed |
| `FpMode`, `Shape`, `Peaks` | focal-plane mode, `(Rows, Cols)` of the full grid, peaks per zone |
| `DistanceMm` | clock-corrected distances in mm, flat `[peaks][rows][cols]`; null without peaks |
| `Results` | the raw integers: `Distance`, `Snr`, `Signal` (`[peaks][rows][cols]`), `Noise`, `Xtalk` (`[rows][cols]`); absent fields are null |
| `FNumber`, `TemperatureC` | frame number, sensor die temperature (°C) |
| `Frames`, `Times` | the results frame(s) (`Tmf8829FrameInfo`) and their `Tmf8829FrameTime` |
| `Histograms`, `RefFrames` | histogram frames (`Tmf8829HistogramFrame`, raw bins in `Data`) and 65-byte ref-SPAD frames before it |
| `Problems` | what went wrong since the previous measurement (bad status / EOF, dropped halves, parse errors) |

Flat arrays index as `(p * rows + r) * cols + c`;
`Results.DistanceAt(p, r, c)` reads one raw distance. A raw distance of 0
means no target. Raw units are 0.25 mm, or mm with preset `8x8-xr`
(`Tmf8829FrameInfo.DistanceInMm`). The grid is in the sensor's own order:
with the board USB down, seen from behind, `r` runs left → right and `c` top
→ bottom — transpose it to draw the scene.

## Reassembly and frames by hand

`Tmf8829FrameReassembler.Feed` follows the contract rules: a chunk at offset 0
starts a frame, a chunk is appended only with the same `FrameSeq` and
`FullSize` at the expected offset, anything else discards the frame in
progress. Counters: `Completed`, `Discarded` (partial frames lost),
`SeqGaps` (frames the bridge dropped). An incomplete last frame after a stop
is normal — it is simply never returned.

The frame layer is usable on its own:

```csharp
Tmf8829FrameInfo f = Tmf8829Frames.ParseFrame(raw.Data);
if (f.Kind == Tmf8829Frames.FidResults)
{
    Tmf8829PixelResults r = Tmf8829Frames.ParseResults(raw.Data, f);   // one frame
    Console.WriteLine($"fp_mode {Tmf8829Frames.FpNames[f.FpMode]} {r.Rows}x{r.Cols} " +
                      $"peaks {r.Peaks} second half {f.IsSecondHalf} problems {string.Join(",", f.Problems())}");
}
// 32×32 / 48×32: first half (even rows) + second half (odd rows).
Tmf8829PixelResults full = Tmf8829Frames.MergeHalves(firstHalf, secondHalf);
// Raw → mm with the current clock ratio.
double[]? mm = Tmf8829Measurement.ToMm(full, builder.Clock.Ratio, f.DistanceInMm);
```

`ParseResults` throws `Tmf8829FrameException` when the frame is not a results
frame or its data size does not match the grid × pixel size (a corrupt frame).

## Capture time and the clock model

Each chunk's `TimestampUs` is the MCU time right before the header read — a
clock pair with the frame's SYSTICK, **not** the capture time. The builder
feeds every non-ref frame's pair to `Tmf8829SensorClock.Add` and asks
`Tmf8829CaptureTimer` for each results frame; to do it yourself:

```csharp
var clock = new Tmf8829SensorClock();
var timer = new Tmf8829CaptureTimer(clock);

int flags = clock.Add((long)raw.TimestampUs, f.Systick);       // every non-ref frame
if (f.Kind == Tmf8829Frames.FidHistograms)
    timer.Histograms();
else if (f.Kind == Tmf8829Frames.FidResults)
{
    Tmf8829FrameTime t = timer.ResultsFrame(f, (long)raw.TimestampUs, (long)raw.EdgeUs, flags);
    Console.WriteLine($"{t.TimestampUs} µs, T1 {t.T1LenUs} µs, ratio {t.RatioQ16 / 65536.0 / 8:F4}");
}
```

The ratio (`clock.Ratio`, `RatioQ16`) is measured over a sliding 1–2 s window
and is also the distance correction. Flags (`Tmf8829TimeFlags`): `Nominal` —
ratio not measured yet (first 200 ms); `ClockReset` — the model restarted;
`EdgeLate` — the INT edge was later than usual; `Stalled` — sub-integrations
were not back to back (histogram FIFO wait); `T1Unsure` — 32×32/48×32 with
histograms, the time may be up to ~20 ms late. All arithmetic is integer and
pinned bit for bit by `contracts/vectors/tmf8829_frame_time.json`. To map MCU
µs to host time, use SYNC_TIME (see the [common guide](../guide.md)).

## Presets and the config page

`Tmf8829Config.Presets` maps the ten preset names to their load command; the
config page is the 190 bytes at `0x22..0xDF` (`RegCfg`, `CfgSize`), read back
after `LOAD_CONFIG_PAGE`. Edit it by field name:

```csharp
byte[] page = regData.Data;                                    // 190 B read from 0x22
Tmf8829Config.CfgSet(page, "period_ms", Tmf8829Config.DefaultPeriodMs);
Tmf8829Config.CfgSet(page, "kilo_iterations", Tmf8829Config.PresetKiloIterations["32x32"]);
Tmf8829Config.CfgSet(page, "nr_peaks", 2);
Tmf8829Config.CfgSet(page, "signal_strength", 1);
Tmf8829Config.CfgSet(page, "noise_strength", 0);
Tmf8829Config.CfgSet(page, "xtalk", 0);
Tmf8829Config.CfgSet(page, "dump_histograms", 0);
Tmf8829Config.CfgSet(page, "ref_spad_frame", 0);
// then TMF_WRITE_REG 0x22 + page, WRITE_PAGE (0x15 to 0x08), LOAD_CONFIG_PAGE, read back
foreach (var (name, value) in Tmf8829Config.ConfigFields(page))
    Console.WriteLine($"{name} = {value}");
```

**Write all eight of these fields on every configure.** A preset load keeps
what the previous config wrote (result format, histograms, ref frame,
period; `kilo_iterations` is rescaled), so a stale 4-peak format can make the
next `32x32` fail with `ERR_CONFIG_RESULT_SIZE`, and a stale
`dump_histograms` survives even an `init()`. The defaults are `period_ms` 33,
`kilo_iterations` from `PresetKiloIterations`, `nr_peaks` 1, strengths 0, no
histograms, no ref frame. The other named fields (`fp_mode`, `power_modes`,
`ha_kilo_iterations`, `dual_mode`, `fov_correction`, `distance_in_mm`) are
there to read; `CfgSet` throws for a value that does not fit the field.

## The sensor firmware image

The image the host downloads on every `init()` is embedded in the assembly:

```csharp
ReadOnlySpan<byte> image = Tmf8829Firmware.ImageSpan;   // 14 444 B, 3611 words
uint addr = Tmf8829Firmware.LoadAddr;                   // 0x00010000
string license = Tmf8829Firmware.License();             // ams-OSRAM MIT text
```

It is ams-OSRAM's `tmf8829_application` (application 2.200.0), MIT-licensed;
the license text ships with the package. Redistribute it with that text.

## Events and bridge info

`RPT_TMF_EVENT` (`Tmf8829Event.Unpack`): `Code` `Tmf8829EventCode.Int` is a
MOTION/PROXIMITY interrupt; `Tmf8829EventCode.BadHeader` is a refused frame
header. Exactly one `BadHeader` is **expected** when STOP lands in the middle
of a histogram dump — ignore it during a stop; any other means the sensor is
lost and needs a new `init()`.

`RPT_TMF_INFO` (`Tmf8829Info.Unpack`) has no echoed command byte (a reply
matcher keyed on `payload[0] == cmd` must exempt it): `IntEdges`, `Frames`,
`FramesAborted`, `FrameErrors`, `SpiTimeouts`, `IntLevel`, `Streaming`,
`IntMask`, `LastIntStatus`, `LastFifoStatus`. It never touches the sensor and
is safe to poll mid-stream.

## Datasets

A `.depzdata` file recorded by the Python or TypeScript SDK has one
`tmf8829` record per measurement (contract 09): `DatasetRecord.Value` carries
`fp_mode`, `rows`, `cols`, `peaks`, `f_number`, `flags`, `temperature_c`,
`distance_mm[]` (mm, rounded to 0.1, flat `[peak][row][col]`), `snr[]` and,
when present, `signal[]`, `noise[]`, `xtalk[]`:

```csharp
using Depz.Sensor.Dataset;

foreach (DatasetRecord r in new DatasetReader("run.depzdata").Records)
    if (r.Kind == "tmf8829")
        Console.WriteLine($"{r.THostUs} {r.Value.GetProperty("rows")}x{r.Value.GetProperty("cols")}");
```

Histogram and ref-SPAD frames are not recorded.

## Gotchas

- **No live class** — this SDK decodes and encodes; init, configure and
  start/stop are your register traffic (contract 14 §4) or the Python / TS SDK.
- **Reset per stream** — call `Reset()` on the reassembler and the builder at
  every stream start: `frame_seq`, the clock model and T1 are per stream.
- **Never stitch across `frame_seq`** — the reassembler already refuses; do
  not "repair" frames yourself.
- **Half-frames** — on 32×32/48×32 a measurement needs both halves; a second
  half without a first is dropped (listed in `Problems`).
- **Configure writes everything** — see [the config page](#presets-and-the-config-page).
- **Units** — raw distances are 0.25 mm except `8x8-xr` (mm), and need the
  clock ratio; use `DistanceMm` / `ToMm`.
- **Orientation** — the grid is transposed against the scene (row = left →
  right, column = top → bottom for a board USB down, seen from behind).
- **No sleep** — EN is wired high; recovery from any state is `init()`.
- **One register command at a time**, 1 s timeout while streaming.
