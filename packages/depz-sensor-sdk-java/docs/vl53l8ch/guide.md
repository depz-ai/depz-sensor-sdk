# VL53L8CH — user guide

Hands-on guide to the CH-specific surface of the ToF decoder. For what the
sensor is and why CH exists, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md).

**CH reuses the entire VL53L8CX decode surface.** The frame path
(`FrameReassembler`), `Vl53l8Uld.parseFrame`, the `Results` fields, and every
advanced-DCI codec (xtalk, thresholds, motion) work exactly as in the
[VL53L8CX user guide](../vl53l8cx/guide.md) — read that first. This page covers
**only the CH additions**: the `Variant.CH` footer and the CNH decode.

`Vl53l8chDevice` is a `Vl53l8cxDevice` with one more method, `configureCnh`,
and downloads the CH firmware (VL53LMZ ULD 2.0.16) at `init()`.
Keep the configuration's aggregate count and bins per aggregate — the decode
needs them.

## Contents

- [Arm CNH on the live device](#arm-cnh-on-the-live-device)
- [Parsing a CH frame](#parsing-a-ch-frame)
- [Decode the CNH histograms](#decode-the-cnh-histograms)
- [Gotchas](#gotchas)

## Arm CNH on the live device

```java
import ai.depz.sensor.devices.*;
import ai.depz.sensor.sensors.vl53l8.*;
import java.time.Duration;

try (Vl53l8chDevice ch = (Vl53l8chDevice) DepzDevices.openVl53l8()) {
    ch.init();
    ch.setResolution(64);
    ch.setRangingFrequencyHz(15);

    Vl53l8CnhSetup setup = new Vl53l8CnhSetup();
    setup.initConfig(10, 20, 2);              // bins 10.. of the device histogram, 2 per CNH bin
    setup.createAggMap(64, 0, 0, 2, 2, 4, 4); // 16 aggregates of 2x2 zones
    ch.configureCnh(setup);                   // refuses a setup over 6160 B

    try (Subscription<Vl53l8Frame> frames = ch.frames()) {
        ch.startRanging();                    // every frame now carries cnhRaw
        Vl53l8Frame f = frames.take(Duration.ofSeconds(1));
        Vl53l8Uld.CnhResult cnh = Vl53l8Uld.decodeCnh(setup.nbOfAggregates(), setup.featureLength(), f.cnhRaw());
        ch.stopRanging();
    }
}
```

`setup.minMaxDistanceMm()` tells the distance range the histogram captures and
`setup.binCenterMm(i)` the distance of bin `i`.

## Parsing a CH frame

Identical to the CX, except you pass `Variant.CH`: the VL53L8CH firmware
(VL53LMZ 2.0.16) keeps the frame-id footer 4 bytes from the end, where the
VL53L8CX firmware (ULD 2.1.0) keeps it 12 bytes from the end:

```java
import ai.depz.sensor.transport.*;
import ai.depz.sensor.sensors.vl53l8.*;

PacketParser parser = new PacketParser();
FrameReassembler reasm = new FrameReassembler();

for (Event ev : parser.feed(bytesFromPort)) {
    if (ev instanceof Packet p && p.cmd() == 0x93) {   // RPT_VL53_FRAME chunk
        FrameReassembler.CompletedFrame done = reasm.feed(FrameReassembler.unpackFrameChunk(p.payload()));
        if (done != null) {
            Vl53l8Uld.Results r = Vl53l8Uld.parseFrame(done.frame(), done.frame().length, Vl53l8Uld.Variant.CH);
            // r.distanceMm, r.targetStatus, ... — the full CX Results surface
            byte[] cnh = r.cnhRaw;   // the CNH block; null when the frame carries none
        }
    }
}
```

Everything on `Results` is the same as CX (see the
[CX frame fields](../vl53l8cx/guide.md#the-results-frame-fields)); the one extra
is `cnhRaw`, the raw bytes of the CNH output block (`Vl53l8Uld.CNH_DATA_IDX` =
`0xC048`).

## Decode the CNH histograms

[`Vl53l8Uld.decodeCnh`](api.md#vl53l8ulddecodecnh) turns the raw CNH block —
`cnhRaw` from a parsed frame, or the bytes of the board's CNH read — into one
histogram per aggregate. It needs the aggregate count and bins per aggregate of
the CNH configuration the sensor runs:

```java
import ai.depz.sensor.sensors.vl53l8.Vl53l8Uld;

static void printCnh(Vl53l8Uld.Results r) {
    // The configuration the sensor runs: 16 aggregates of 20 bins each.
    int nbOfAggregates = 16, featureLength = 20;
    if (r.cnhRaw == null) {
        return;                                        // no CNH block
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

Each `CnhAggregate` holds `featureLength` bins as an integer `histRaw` plus a
power-of-two `histScaler`; the bin value is `histRaw / 2^histScaler`.

## Gotchas

- **Pass `Variant.CH`** — using `Variant.CX` on a CH frame trips the
  header/footer id check (offset 12 vs 4) and throws `Vl53l8Uld.Vl53l8Error`.
- **Pass the configuration the sensor runs** — `decodeCnh` needs the same
  aggregate count and bins per aggregate; the block does not record them, and a
  wrong pair decodes to wrong bins.
- **`decodeCnh` checks its inputs** — counts outside 1..64 aggregates /
  1..255 bins, or a block shorter than the config implies, throw
  `IllegalArgumentException`.
- **`cnhRaw` may be `null`** — it is `null` on CX and on CH frames without a CNH
  block; null-check before using it.
- All the CX gotchas (chunked frames, applied scaling, one driver over any transport)
  apply here too — see the [CX guide](../vl53l8cx/guide.md#gotchas).
