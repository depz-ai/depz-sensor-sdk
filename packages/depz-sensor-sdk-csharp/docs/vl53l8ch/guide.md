# VL53L8CH — user guide

Hands-on guide to the CH additions on top of the base ToF codecs. For what the
sensor is and why CH exists, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md).

**CH reuses the entire VL53L8CX decode surface.** Packet handling, chunk
reassembly (`FrameReassembler`), results-frame decode (`Vl53l8FrameDecoder`), the
frame fields, and the advanced DCI codecs (`Vl53l8Advanced`, `MotionConfig`) work
exactly as in the [VL53L8CX user guide](../vl53l8cx/guide.md) — read that first.
This page covers **only the CH-specific bits**: selecting the CH decoder and
decoding the CNH histograms.

`Vl53l8chDevice` is a `Vl53l8cxDevice` with one more method, `ConfigureCnh`,
and downloads the CH firmware (VL53LMZ ULD 2.0.16) at `Init()`.

## Contents

- [Arm CNH on the live device](#arm-cnh-on-the-live-device)
- [Decoding CH frames](#decoding-ch-frames)
- [Decode the CNH histograms](#decode-the-cnh-histograms)
- [Gotchas](#gotchas)

## Arm CNH on the live device

```csharp
using Depz.Sensor.Devices;
using Depz.Sensor.Vl53l8;

var ch = (Vl53l8chDevice)DepzDevices.OpenVl53l8();
ch.Init();
ch.SetResolution(64);
ch.SetRangingFrequencyHz(15);

var setup = new Vl53l8CnhSetup();
setup.InitConfig(startBin: 10, numBins: 20, subSample: 2);                // bins 10.. of the device histogram, 2 per CNH bin
setup.CreateAggMap(64, startX: 0, startY: 0, mergeX: 2, mergeY: 2, cols: 4, rows: 4); // 16 aggregates of 2x2 zones
ch.ConfigureCnh(setup);                                                   // refuses a setup over 6160 B

using var frames = ch.Frames();
ch.StartRanging();                                                        // every frame now carries CnhRaw
Vl53l8CnhResult cnh = Vl53l8Cnh.DecodeHistogram(setup.ToDecodeConfig(), frames.Take(TimeSpan.FromSeconds(1)).CnhRaw);
ch.StopRanging();
```

`setup.MinMaxDistanceMm()` tells the distance range the histogram captures and
`setup.BinCenterMm(i)` the distance of bin `i`.

## Decoding CH frames

Identical to the CX pipeline, except you select the **CH** variant: the VL53L8CH
firmware (VL53LMZ 2.0.16) echoes the frame id 4 bytes from the end, where the
VL53L8CX firmware (ULD 2.1.0) echoes it 12 bytes from the end:

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l8;

var parser = new PacketParser();
var reasm = new FrameReassembler();
var decoder = Vl53l8FrameDecoder.ForVariant(Vl53l8Variant.Ch);   // CH footer offset
var frames = new List<Vl53l8Frame>();

foreach (ParserEvent ev in parser.Feed(rxBytes))
{
    if (ev is Packet pkt && pkt.Cmd == (int)Vl53l8Rpt.Vl53Frame
        && reasm.Feed(FrameChunk.Unpack(pkt.Payload)) is (ulong ts, byte[] frameBytes))
    {
        frames.Add(decoder.ParseFrame(ts, frameBytes));   // CnhRaw: the CNH block, or null
    }
}
```

The per-zone frame fields, the row-major grid, the DCI codecs and the corrupt-
frame guard are all inherited unchanged from the
[CX guide](../vl53l8cx/guide.md). The one CH-specific field is `CnhRaw`: the raw
bytes of the CNH output block (index `0xC048`), or `null` when the frame has no
CNH block.

## Decode the CNH histograms

[`Vl53l8Cnh.DecodeHistogram`](api.md#vl53l8cnhdecodehistogram) turns the raw
CNH block — `CnhRaw` from a decoded frame, or the bytes of the board's CNH
read — into one histogram per aggregate. It needs the aggregate count and bins
per aggregate of the CNH configuration the sensor runs:

```csharp
using Depz.Sensor.Vl53l8;

static void PrintCnh(Vl53l8Frame f)
{
    // The configuration the sensor runs: 16 aggregates of 20 bins each.
    var config = new Vl53l8CnhConfig(NbOfAggregates: 16, FeatureLength: 20);
    if (f.CnhRaw is null)
        return;                                              // no CNH block
    Vl53l8CnhResult cnh = Vl53l8Cnh.DecodeHistogram(config, f.CnhRaw);
    for (int a = 0; a < cnh.Aggregates.Count; a++)
    {
        Vl53l8CnhAggregate agg = cnh.Aggregates[a];
        Console.WriteLine($"aggregate {a}: {string.Join(", ", agg.Hist)} (ambient {agg.Ambient})");
    }
    Console.WriteLine($"ref residual {cnh.RefResidual}");    // 11 fractional bits
}
```

Each `Vl53l8CnhAggregate` holds `FeatureLength` bins as an integer `HistRaw`
plus a power-of-two `HistScaler`; the bin value is `HistRaw / 2^HistScaler`
(`Hist`). The aggregate's ambient level is `AmbientRaw / 2^AmbientScaler`
(`Ambient`). `DecodeHistogram` throws `ArgumentOutOfRangeException` for a
negative aggregate or bin count, and `ArgumentException` when the block is
shorter than the layout the config implies.

## Gotchas

- **Select the CH variant for decoding** — `ForVariant(Vl53l8Variant.Ch)`. The
  CX footer offset on a CH frame yields a spurious `CorruptedFrameException`.
- **Pass the configuration the sensor runs** — `DecodeHistogram` needs the same
  aggregate count and bins per aggregate; the block does not record them, and a
  wrong pair decodes to wrong bins or throws `ArgumentException`.
- **`CnhRaw` is null** on CX frames and on CH frames without a CNH block.
- **Everything else is the CX surface** — chunk gaps, `Resolution`-sized arrays,
  `TargetStatus == 255`, the 2 Hz floor — all apply here too; see the
  [CX gotchas](../vl53l8cx/guide.md#gotchas).
