# VL53L8CX — user guide

Hands-on guide to the shared VL53L8 decode codecs (`Vl53l8Uld`,
`FrameReassembler`) for the base ToF sensor. For what the sensor is, CX vs CH,
and the concepts, read the [introduction](introduction.md); for exact signatures
see the [API reference](api.md). The **VL53L8CH** superset (CNH histograms) has
its [own guide](../vl53l8ch/guide.md) — it reuses everything below.

## Contents

- [The frame path](#the-frame-path)
- [Reassemble and parse a frame](#reassemble-and-parse-a-frame)
- [The Results frame: fields](#the-results-frame-fields)
- [Advanced-DCI codecs](#advanced-dci-codecs)
- [Gotchas](#gotchas)

## The frame path

A full ranging frame arrives as a run of chunk reports and is decoded in three
steps, all pure and vector-tested:

1. `FrameReassembler.unpackFrameChunk(payload)` → a `FrameChunk`
   (`timestampUs`, `fullSize`, `offset`, `data`).
2. `reassembler.feed(chunk)` → a `CompletedFrame` once the bytes add up, or
   `null` while still accumulating.
3. `Vl53l8Uld.parseFrame(frame, dataReadSize, Variant.CX)` → a `Results`.

## Reassemble and parse a frame

```java
import ai.depz.sensor.transport.*;
import ai.depz.sensor.sensors.vl53l8.*;

PacketParser parser = new PacketParser();
FrameReassembler reasm = new FrameReassembler();

for (Event ev : parser.feed(bytesFromPort)) {
    if (ev instanceof Packet p && p.cmd() == 0x93) {   // RPT_VL53_FRAME chunk
        FrameReassembler.FrameChunk chunk = FrameReassembler.unpackFrameChunk(p.payload());
        FrameReassembler.CompletedFrame done = reasm.feed(chunk);
        if (done != null) {
            Vl53l8Uld.Results r = Vl53l8Uld.parseFrame(done.frame(), done.frame().length, Vl53l8Uld.Variant.CX);
            int zones = r.resolution();                 // 16 (4×4) or 64 (8×8)
            System.out.println("center-ish mm: " + r.distanceMm[zones / 2]
                    + "  °C: " + r.siliconTempDegc);
        }
    }
}
```

Link health is observable on the reassembler: `reasm.completed` and
`reasm.discarded` (chunked frames dropped on a gap / offset error). A
header/footer id mismatch in `parseFrame` throws `Vl53l8Uld.Vl53l8Error`.

## The Results frame: fields

`Vl53l8Uld.Results` carries every per-zone output as a raw-integer array sized to
the active resolution, plus the per-frame silicon temperature:

| field | type | meaning |
|---|---|---|
| `distanceMm` | `int[]` | per-zone distance in mm (already scaled /4) |
| `targetStatus` | `int[]` | 5/9 = valid, 255 = no target |
| `nbTargetDetected` | `int[]` | targets found in the zone |
| `signalPerSpad` | `long[]` | signal rate, kcps/SPAD (raw) |
| `ambientPerSpad` | `long[]` | ambient rate, kcps/SPAD (raw) |
| `nbSpadsEnabled` | `long[]` | enabled SPADs per zone (raw) |
| `rangeSigmaMm` | `double[]` | range std-dev estimate, mm (scaled /128) |
| `reflectance` | `int[]` | estimated reflectance, % |
| `siliconTempDegc` | `int` | per-frame sensor temperature, °C |
| `cnhRaw` | `byte[]` | CNH block bytes (CH only; `null` on CX) |

`resolution()` returns the zone count. Zone index runs row-major; reshape to
`(4,4)` / `(8,8)` yourself for a heatmap. Mask invalid zones on
`targetStatus == 255`.

## Advanced-DCI codecs

These are the pure **codecs** for the ST ULD plugins — they build the payloads a
live driver would write over DCI. The live write path is the
[extension point](introduction.md#scope-decode-only); the packing is complete.

```java
// crosstalk margin (kcps/SPAD) → raw DCI value
long raw = Vl53l8Uld.xtalkMarginToRaw(50.0);

// per-zone detection thresholds (interrupt-on-threshold)
var thr = List.of(new Vl53l8Uld.DetectionThreshold(
        200, 600, Vl53l8Uld.DIST_MM, /*type*/0, /*zoneNum*/128, /*operation*/0));
Vl53l8Uld.PackedThresholds packed = Vl53l8Uld.packDetectionThresholds(thr);
// packed.start()  → DCI_DET_THRESH_START payload (64×12 B)
// packed.valid()  → 8-byte valid-status block

// motion indicator
Vl53l8Uld.MotionConfig cfg = Vl53l8Uld.defaultMotionConfig(Vl53l8Uld.RESOLUTION_8X8);
byte[] motionPayload = cfg.pack();       // 156-byte VL53L8CX_Motion_Configuration
```

The threshold `measurement` selectors (`DIST_MM`, `SIGNAL_PER_SPAD_KCPS`,
`RANGE_SIGMA_MM`, `AMBIENT_PER_SPAD_KCPS`, `MOTION_INDICATOR`, …) and the power
modes (`POWER_MODE_SLEEP`/`WAKEUP`/`DEEP_SLEEP`) are constants on `Vl53l8Uld`.
`motionConfigSetResolution` re-maps a `MotionConfig` for a given resolution.

## Gotchas

- **Frames are chunked** — always route chunk reports through `FrameReassembler`;
  `parseFrame` expects a complete frame buffer.
- **Pass the right `Variant`** — CX uses footer-id offset 12, CH uses 4; the
  wrong one trips the header/footer id check and throws `Vl53l8Error`.
- **Scaling is applied for you** — `distanceMm` is already /4 and `rangeSigmaMm`
  /128; the other rate arrays stay raw integers.
- **`cnhRaw` is CH-only** — it is `null` on CX, and even on CH this SDK does not
  decode it (see the [VL53L8CH guide](../vl53l8ch/guide.md)).
- **The live driver is a stub** — the advanced-DCI codecs pack payloads but do
  not talk to hardware here; `liveDriverStubbed()` marks that boundary.
