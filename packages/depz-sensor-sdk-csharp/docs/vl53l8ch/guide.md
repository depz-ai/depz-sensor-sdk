# VL53L8CH — user guide

Hands-on guide to the CH additions on top of the base ToF codecs. For what the
sensor is and why CH exists, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md).

**CH reuses the entire VL53L8CX decode surface.** Packet handling, chunk
reassembly (`FrameReassembler`), results-frame decode (`Vl53l8FrameDecoder`), the
frame fields, and the advanced DCI codecs (`Vl53l8Advanced`, `MotionConfig`) work
exactly as in the [VL53L8CX user guide](../vl53l8cx/guide.md) — read that first.
This page covers **only the CH-specific bits**: selecting the CH decoder and the
CNH extension point.

## Contents

- [Decoding CH frames](#decoding-ch-frames)
- [CNH histograms: the extension point](#cnh-histograms-the-extension-point)
- [Gotchas](#gotchas)

## Decoding CH frames

Identical to the CX pipeline, except you select the **CH** variant so the decoder
uses the CH frame-id footer offset (4 bytes from the end, vs 12 for CX):

```csharp
using Depz.Sensor.Vl53l8;

var decoder = Vl53l8FrameDecoder.ForVariant(Vl53l8Variant.Ch);   // CH footer offset
// everything else — FrameReassembler, ParseFrame, Vl53l8Frame — is the same:
Vl53l8Frame frame = decoder.ParseFrame(timestampUs, frameBytes);
```

The per-zone frame fields, the row-major grid, the DCI codecs and the corrupt-
frame guard are all inherited unchanged from the
[CX guide](../vl53l8cx/guide.md).

## CNH histograms: the extension point

CNH is the CH-only addition over the shared frame decode. Its decoder lives in
`Vl53l8Cnh` and is **not yet implemented** — it throws by design rather than
return fabricated data:

```csharp
using Depz.Sensor.Vl53l8;

Vl53l8Variant v = Vl53l8Cnh.Variant;   // always Vl53l8Variant.Ch

try
{
    Vl53l8Cnh.DecodeHistogram(rawBlock);
}
catch (NotSupportedException ex)
{
    // Expected today: CNH histogram decode is a pending extension point.
    // There is no golden CNH vector to verify a port against yet, so it is
    // stubbed, not faked. Wire the real parse into Vl53l8Cnh when one exists.
    Console.Error.WriteLine(ex.Message);
}
```

Note that CH results frames also use the CH footer-id offset
(`Vl53l8FrameDecoder.FooterIdOffsetCh`) — so a full CH capture is decoded with
`ForVariant(Vl53l8Variant.Ch)` for the depth image, and the CNH block would be
decoded separately here once `Vl53l8Cnh` is implemented.

## Gotchas

- **Select the CH variant for decoding** — `ForVariant(Vl53l8Variant.Ch)`. The
  CX footer offset on a CH frame yields a spurious `CorruptedFrameException`.
- **CNH decode is a stub today** — `Vl53l8Cnh.DecodeHistogram` throws
  `NotSupportedException`. Handle it, or gate on it; do not expect histogram
  output yet.
- **Everything else is the CX surface** — chunk gaps, `Resolution`-sized arrays,
  `TargetStatus == 255`, out-of-scope live init — all apply here too; see the
  [CX gotchas](../vl53l8cx/guide.md#gotchas).
