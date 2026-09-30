# VL53L7CH — user guide

The VL53L7CH is the [VL53L7CX](../vl53l7cx/guide.md) plus CNH histograms:
class resolution, board commands, the 1536-byte ceilings and the frame decode
with `Vl53l7Frames.CreateDecoder()` all apply unchanged. This page covers the
CNH block. For signatures see the [API reference](api.md).

This SDK does not drive the board: arming a CNH configuration and starting
the stream is done with the Python or TypeScript SDK. Keep the configuration's
aggregate count and bins per aggregate — the decode needs them.

## Contents

- [CNH frames arrive in chunks](#cnh-frames-arrive-in-chunks)
- [Decode the histograms](#decode-the-histograms)
- [Gotchas](#gotchas)

## CNH frames arrive in chunks

A frame with CNH is larger than one 1536-byte chunk — 3156 bytes for 16
aggregates × 20 bins, up to ~7.6 KB — so it arrives as several
`RPT_VL53_FRAME` chunks. The shared reassembler rebuilds it; nothing
CH-specific is needed:

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l7;
using Depz.Sensor.Vl53l8;

var parser = new PacketParser();
var reasm = new FrameReassembler();
Vl53l8FrameDecoder decoder = Vl53l7Frames.CreateDecoder();
var frames = new List<Vl53l8Frame>();

foreach (ParserEvent ev in parser.Feed(rxBytes))
{
    if (ev is Packet pkt && pkt.Cmd == (int)Vl53l8Rpt.Vl53Frame
        && reasm.Feed(FrameChunk.Unpack(pkt.Payload)) is (ulong ts, byte[] frameBytes))
    {
        frames.Add(decoder.ParseFrame(ts, frameBytes));
    }
}
```

## Decode the histograms

The decoder captures the CNH output block (index `0xC048`) verbatim in
`CnhRaw`; [`Vl53l8Cnh.DecodeHistogram`](api.md#vl53l8cnh) unpacks it with the
configuration the sensor was armed with:

```csharp
using Depz.Sensor.Vl53l8;

// The armed CNH configuration: 16 aggregates of 20 bins each.
var config = new Vl53l8CnhConfig(NbOfAggregates: 16, FeatureLength: 20);
foreach (Vl53l8Frame f in frames)
{
    if (f.CnhRaw is null)
        continue;                                            // no CNH block armed
    Vl53l8CnhResult cnh = Vl53l8Cnh.DecodeHistogram(config, f.CnhRaw);
    for (int a = 0; a < cnh.Aggregates.Count; a++)
    {
        Vl53l8CnhAggregate agg = cnh.Aggregates[a];
        double[] hist = agg.HistRaw.Select((v, i) => v / Math.Pow(2, agg.HistScaler[i])).ToArray();   // raw / 2^scaler
        Console.WriteLine($"aggregate {a}: {string.Join(", ", hist)}");
    }
    double refResidual = cnh.RefResidualWord / 2048.0;       // 11 fractional bits
    Console.WriteLine($"ref residual {refResidual}");
}
```

The per-zone depth arrays in the same `Vl53l8Frame` are fully decoded and
trimmed as on the VL53L7CX.

## Gotchas

- **Pass the armed configuration** — `DecodeHistogram` needs the same
  aggregate count and bins per aggregate the CNH configuration used; the block
  does not say.
- **`CnhRaw` is null** on frames without a CNH block (CNH not armed).
- **Use `Vl53l7Frames.CreateDecoder()`** — the L7CH carries the VL53LMZ blob,
  and its footer sits at `size − 4` like the other L5/L7 parts.
- All the [VL53L7CX gotchas](../vl53l7cx/guide.md#gotchas) apply.
