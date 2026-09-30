# VL53L7CH — user guide

The VL53L7CH is the [VL53L7CX](../vl53l7cx/guide.md) plus CNH histograms:
class resolution, board commands, the 1536-byte ceilings and the frame decode
with `Vl53l7Uld.parseFrame` all apply unchanged. This page covers the CNH
block. For signatures see the [API reference](api.md).

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

```java
import java.util.ArrayList;
import java.util.List;
import ai.depz.sensor.protocol.Vl53l7;
import ai.depz.sensor.sensors.vl53l7.Vl53l7Uld;
import ai.depz.sensor.sensors.vl53l8.FrameReassembler;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Uld;
import ai.depz.sensor.transport.Event;
import ai.depz.sensor.transport.Packet;
import ai.depz.sensor.transport.PacketParser;

PacketParser parser = new PacketParser();
FrameReassembler reasm = new FrameReassembler();
List<Vl53l8Uld.Results> frames = new ArrayList<>();
for (Event ev : parser.feed(rxBytes)) {
    if (ev instanceof Packet p && p.cmd() == Vl53l7.Vl53l7Rpt.VL53_FRAME.value) {
        FrameReassembler.CompletedFrame done = reasm.feed(FrameReassembler.unpackFrameChunk(p.payload()));
        if (done != null) {
            frames.add(Vl53l7Uld.parseFrame(done.frame(), done.frame().length));
        }
    }
}
```

## Decode the histograms

The frame decode captures the CNH output block (block id
`Vl53l8Uld.CNH_DATA_IDX` = `0xC048`) verbatim in `cnhRaw`;
[`Vl53l8Uld.decodeCnh`](api.md#vl53l8ulddecodecnh) unpacks it with the
configuration the sensor was armed with:

```java
import ai.depz.sensor.sensors.vl53l8.Vl53l8Uld;

// The armed CNH configuration: 16 aggregates of 20 bins each.
int nbOfAggregates = 16, featureLength = 20;
for (Vl53l8Uld.Results r : frames) {
    if (r.cnhRaw == null) {
        continue;                                   // no CNH block armed
    }
    Vl53l8Uld.CnhResult cnh = Vl53l8Uld.decodeCnh(nbOfAggregates, featureLength, r.cnhRaw);
    for (int a = 0; a < cnh.aggregates.length; a++) {
        Vl53l8Uld.CnhAggregate agg = cnh.aggregates[a];
        double[] hist = new double[agg.histRaw.length];
        for (int i = 0; i < hist.length; i++) {
            hist[i] = agg.histRaw[i] / Math.pow(2, agg.histScaler[i]);   // raw / 2^scaler
        }
        System.out.println("aggregate " + a + ": " + java.util.Arrays.toString(hist));
    }
    double refResidual = cnh.refResidualWord / 2048.0;                  // 11 fractional bits
    System.out.println("ref residual " + refResidual);
}
```

The per-zone depth arrays in the same `Results` are fully decoded and trimmed
as on the VL53L7CX.

## Gotchas

- **Pass the armed configuration** — `decodeCnh` needs the same aggregate
  count and bins per aggregate the CNH configuration used; the block does not
  say.
- **`cnhRaw` is `null`** on frames without a CNH block (CNH not armed).
- **Use `Vl53l7Uld.parseFrame`** — the L7CH carries the VL53LMZ blob, and its
  footer sits at `size-4` like the other L5/L7 parts.
- All the [VL53L7CX gotchas](../vl53l7cx/guide.md#gotchas) apply.
