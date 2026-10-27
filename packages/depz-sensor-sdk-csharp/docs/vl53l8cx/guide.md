# VL53L8CX — user guide

Hands-on guide to the VL53L8CX: the live device
(`Depz.Sensor.Devices.Vl53l8cxDevice`, which runs the ST ULD on the host as
`Vl53l8Driver`) and the codecs under it (`Depz.Sensor.Vl53l8`). For what the
sensor is, CX vs CH, and the concepts, read the [introduction](introduction.md);
for exact signatures see the [API reference](api.md). The **VL53L8CH** superset
(CNH histograms) has its [own guide](../vl53l8ch/guide.md) — it reuses everything
below.

## Contents

- [Live device](#live-device)
- [The decode pipeline](#the-decode-pipeline)
- [Reassembling a frame](#reassembling-a-frame)
- [Decoding a frame](#decoding-a-frame)
- [The frame: fields and the grid](#the-frame-fields-and-the-grid)
- [Advanced DCI codecs](#advanced-dci-codecs)
- [Gotchas](#gotchas)

## Live device

```csharp
using Depz.Sensor.Devices;
using Depz.Sensor.Vl53l8;

using var tof = DepzDevices.OpenVl53l8();         // a Vl53l8cxDevice, or a Vl53l8chDevice for a CH board
tof.Init(progress: Console.WriteLine);           // ~84 KB sensor firmware over the bridge, about a second
tof.SetResolution(Vl53l8Driver.Resolution8x8);
tof.SetRangingFrequencyHz(15);                   // 2..15 Hz at 8x8, up to 60 at 4x4

using var frames = tof.Frames();
tof.StartRanging();
Vl53l8Frame f = frames.Take(TimeSpan.FromSeconds(1));
Console.WriteLine($"centre zone: {f.DistanceMm[27]} mm, {f.SiliconTempDegc} °C");
tof.StopRanging();
```

- Configuration needs `Init()` and throws `InvalidOperationException` while
  ranging: the stream owns the register bank.
- The rest of ST's API is there: ranging mode, integration time, sharpener,
  target order, power modes (`SetPowerMode`), xtalk margin and calibration
  (`CalibrateXtalk`, `GetCaldataXtalk` / `SetCaldataXtalk` to save and restore
  the 776-byte buffer), detection thresholds, the motion indicator
  (`ConfigureMotionIndicator`, then `Vl53l8Frame.Motion`).
- A frame that does not parse is dropped and counted in `FrameParseErrors`;
  chunk gaps in `ReassemblerDiscards`.
- `tof.Uld` is the driver itself (DCI access, poll mode: `CheckDataReady` /
  `GetRangingData`).

The sections below are the layers under the device, for replaying captures or
writing your own transport.

## The decode pipeline

A VL53L8 frame reaches you in three layers: transport packets, chunk
reassembly, then results-frame decode.

```
Packet (Cmd == Vl53l8Rpt.Vl53Frame)  ─►  FrameChunk.Unpack
      ─►  FrameReassembler.Feed  ─►  (timestampUs, frameBytes)
      ─►  Vl53l8FrameDecoder.ParseFrame  ─►  Vl53l8Frame
```

## Reassembling a frame

Each `RPT_VL53_FRAME` packet (`Vl53l8Rpt.Vl53Frame`, `0x93`) is one chunk of a
larger frame. Unpack it and feed it to a `FrameReassembler`, which returns the
completed frame bytes once all chunks have arrived:

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l8;

var parser = new PacketParser();
var reasm  = new FrameReassembler();
var decoder = Vl53l8FrameDecoder.ForVariant(Vl53l8Variant.Cx);

foreach (ParserEvent ev in parser.Feed(chunk))
{
    if (ev is not Packet pkt || pkt.Cmd != (int)Vl53l8Rpt.Vl53Frame)
        continue;
    FrameChunk fc = FrameChunk.Unpack(pkt.Payload);   // ts, FullSize, Offset, Data
    if (reasm.Feed(fc) is (ulong ts, byte[] frameBytes))
    {
        Vl53l8Frame frame = decoder.ParseFrame(ts, frameBytes);
        // ... use the frame ...
    }
}
// reasm.Completed / reasm.Discarded track frames rebuilt vs dropped on a gap.
```

`FrameReassembler` resets on `Offset == 0`, requires contiguous chunks, and
discards the frame in progress on any gap or overrun (`Discarded++`).

## Decoding a frame

`Vl53l8FrameDecoder` is a verbatim port of the verifiable ST ULD parse path.
Pick the decoder for your silicon variant — the only difference is the frame-id
footer offset:

```csharp
var cx = Vl53l8FrameDecoder.ForVariant(Vl53l8Variant.Cx);   // footer id 12 bytes from end
var ch = Vl53l8FrameDecoder.ForVariant(Vl53l8Variant.Ch);   // footer id 4 bytes from end
// or new Vl53l8FrameDecoder(footerIdOff: Vl53l8FrameDecoder.FooterIdOffsetCx);

Vl53l8Frame frame = cx.ParseFrame(timestampUs, frameBytes);
```

Resolution is derived from the frame (16 or 64 zones). `ParseFrame` throws
`Vl53l8FrameDecoder.CorruptedFrameException` if the header/footer frame-id words
disagree.

## The frame: fields and the grid

`Vl53l8Frame` carries every per-zone output as an array sized to the active
resolution (`Resolution` = 16 or 64), plus the per-frame silicon temperature.
Values come in the real units ST's `GetRangingData` gives (contract 04 §5); ST
divides integers, the SDK keeps the fraction:

| field | type | meaning |
|---|---|---|
| `DistanceMm` | `int[]` | per-zone distance in mm (raw ÷4; a negative distance is clamped to 0, as ST) |
| `TargetStatus` | `byte[]` | 5/9 = valid, 255 = no target |
| `NbTargetDetected` | `byte[]` | targets found in the zone |
| `SignalPerSpad` | `double[]` | signal rate, kcps/SPAD (raw ÷2048) |
| `AmbientPerSpad` | `double[]` | ambient rate, kcps/SPAD (raw ÷2048) |
| `NbSpadsEnabled` | `uint[]` | SPADs enabled in the zone |
| `RangeSigmaMm` | `double[]` | range std-dev estimate, mm (raw ÷128) |
| `Reflectance` | `double[]` | estimated reflectance, % (raw ÷2) |
| `SiliconTempDegc` | `int` | per-frame sensor temperature, °C |

Zone index runs row-major, so a 4×4 grid is `zone = row*4 + col` and an 8×8 grid
`zone = row*8 + col`. Mask invalid zones on `TargetStatus == 255`:

```csharp
for (int z = 0; z < frame.Resolution; z++)
{
    if (frame.TargetStatus[z] == 255) continue;   // no target in this zone
    Console.WriteLine($"zone {z}: {frame.DistanceMm[z]} mm");
}
```

## Advanced DCI codecs

`Vl53l8Advanced` and `MotionConfig` are pure, verifiable ports of the ST ULD
plugin **encoders** — they produce the DCI payloads; `Vl53l8Driver` writes them
to the sensor.

```csharp
using Depz.Sensor.Vl53l8;

// Motion indicator: default config for a resolution, then serialize (156 bytes).
MotionConfig mi = MotionConfig.InitDefault(Vl53l8Advanced.Resolution8x8);
byte[] miPayload = mi.Pack();

// Detection thresholds: 64×12-byte block (real-unit low/high, scaled internally).
var thresholds = new[]
{
    new Vl53l8Advanced.DetectionThreshold(
        LowThresh: 200, HighThresh: 600, Measurement: Vl53l8Advanced.DistMm,
        Type: 0, ZoneNum: 128, Operation: 0),
};
byte[] status = Vl53l8Advanced.ThresholdValidStatus();          // 8-byte valid-status
byte[] block  = Vl53l8Advanced.PackDetectionThresholds(thresholds);

// Xtalk margin: a whole number of kcps (0..10000, as ST) → raw DCI value (kcps * 2048).
uint xtalk = Vl53l8Advanced.XtalkMarginRaw(50);
int kcps   = Vl53l8Advanced.XtalkMarginKcps(xtalk);   // raw / 2048
```

Measurement selectors (`DistMm`, `SignalPerSpadKcps`, `RangeSigmaMm`,
`AmbientPerSpadKcps`, `NbSpadsEnabled`, `MotionIndicator`) carry the scale
factors the encoder applies.

## Gotchas

- **Pick the right variant for the decoder** — the CX and CH footers place the
  frame-id echo at different offsets. Wrong offset → spurious
  `CorruptedFrameException`. Use `ForVariant`.
- **Chunk gaps discard the whole frame** — `FrameReassembler` needs contiguous
  chunks; watch `Discarded`.
- **Arrays are sized to `Resolution`** — don't assume 64; a 4×4 frame has 16.
- **`TargetStatus == 255` means no target** — the decoder sets it per zone when
  `NbTargetDetected == 0`; skip those zones.
- **Below 2 Hz nothing streams** — the sensor never enters its ranging loop;
  `SetRangingFrequencyHz` refuses it.
- **CNH is CH-only** — histogram decode lives in `Vl53l8Cnh.DecodeHistogram`;
  see the [VL53L8CH guide](../vl53l8ch/guide.md).
