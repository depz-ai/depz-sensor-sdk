# VL53L8CX — user guide

Hands-on guide to the base ToF codecs (`Depz.Sensor.Vl53l8`). For what the
sensor is, CX vs CH, and the concepts, read the [introduction](introduction.md);
for exact signatures see the [API reference](api.md). The **VL53L8CH** superset
(CNH histograms) has its [own guide](../vl53l8ch/guide.md) — it reuses everything
below.

## Contents

- [The decode pipeline](#the-decode-pipeline)
- [Reassembling a frame](#reassembling-a-frame)
- [Decoding a frame](#decoding-a-frame)
- [The frame: fields and the grid](#the-frame-fields-and-the-grid)
- [Advanced DCI codecs](#advanced-dci-codecs)
- [Gotchas](#gotchas)

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
resolution (`Resolution` = 16 or 64), plus the per-frame silicon temperature:

| field | type | meaning |
|---|---|---|
| `DistanceMm` | `int[]` | per-zone distance in mm (ST ÷4 applied) |
| `TargetStatus` | `byte[]` | 5/9 = valid, 255 = no target |
| `NbTargetDetected` | `byte[]` | targets found in the zone |
| `SignalPerSpad` | `uint[]` | signal rate, kcps/SPAD |
| `AmbientPerSpad` | `uint[]` | ambient rate, kcps/SPAD |
| `NbSpadsEnabled` | `uint[]` | SPADs enabled in the zone |
| `RangeSigmaMm` | `double[]` | range std-dev estimate, mm (ST ÷128 applied) |
| `Reflectance` | `byte[]` | estimated reflectance, % |
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
plugin **encoders** — they produce the DCI payloads. The live DCI read/write
transport that carries them to the sensor is hardware-dependent and out of scope
(`Vl53l8Uld.Init` is a throwing stub, by design).

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

// Xtalk margin: kcps → raw DCI value.
uint xtalk = Vl53l8Advanced.XtalkMarginRaw(50.0);
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
- **Live init/config is out of scope** — `Vl53l8Uld.Init` throws on purpose. This
  SDK decodes the frames the hardware already produces.
- **CNH is CH-only** — histogram decode lives in `Vl53l8Cnh` (a pending
  extension point); see the [VL53L8CH guide](../vl53l8ch/guide.md).
