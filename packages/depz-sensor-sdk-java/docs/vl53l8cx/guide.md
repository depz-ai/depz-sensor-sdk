# VL53L8CX — user guide

Hands-on guide to the VL53L8CX: the live device
(`ai.depz.sensor.devices.Vl53l8cxDevice`, which runs the ST ULD on the host as
`Vl53l8Driver`) and the shared decode layers under it (`Vl53l8Uld`,
`FrameReassembler`). For what the sensor is, CX vs CH,
and the concepts, read the [introduction](introduction.md); for exact signatures
see the [API reference](api.md). The **VL53L8CH** superset (CNH histograms) has
its [own guide](../vl53l8ch/guide.md) — it reuses everything below.

## Contents

- [Live device](#live-device)
- [The frame path](#the-frame-path)
- [Reassemble and parse a frame](#reassemble-and-parse-a-frame)
- [The Results frame: fields](#the-results-frame-fields)
- [Advanced-DCI codecs](#advanced-dci-codecs)
- [Gotchas](#gotchas)

## Live device

```java
import ai.depz.sensor.devices.*;
import ai.depz.sensor.sensors.vl53l8.*;
import java.time.Duration;

try (Vl53l8cxDevice tof = DepzDevices.openVl53l8()) { // a Vl53l8chDevice for a CH board
    tof.init(System.out::println, null);          // ~84 KB sensor firmware over the bridge, about a second
    tof.setResolution(Vl53l8Driver.RESOLUTION_8X8);
    tof.setRangingFrequencyHz(15);                // 2..15 Hz at 8x8, up to 60 at 4x4

    try (Subscription<Vl53l8Frame> frames = tof.frames()) {
        tof.startRanging();
        Vl53l8Frame f = frames.take(Duration.ofSeconds(1));
        System.out.println("centre zone: " + f.distanceMm()[27] + " mm, " + f.siliconTempDegc() + " °C");
        tof.stopRanging();
    }
}
```

- Configuration needs `init()` and throws `IllegalStateException` while
  ranging: the stream owns the register bank.
- The rest of ST's API is there: ranging mode, integration time, sharpener,
  target order, power modes (`setPowerMode`), xtalk margin and calibration
  (`calibrateXtalk`, `getCaldataXtalk` / `setCaldataXtalk` to save and restore
  the 776-byte buffer), detection thresholds, the motion indicator
  (`configureMotionIndicator`, then `Vl53l8Frame.motion()`).
- A frame that does not parse is dropped and counted in `frameParseErrors()`;
  chunk gaps in `reassemblerDiscards()`.
- `uld()` is the driver itself (DCI access, poll mode: `checkDataReady` /
  `getRangingData`), over any `Vl53l8Platform`.

The sections below are the layers under the device, for replaying captures or
writing your own transport.

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

`Vl53l8Uld.Results` (and the device's `Vl53l8Frame`) carries every per-zone
output as an array sized to the active resolution, plus the per-frame silicon
temperature. Values come in the real units ST's `GetRangingData` gives
(contract 04 §5); ST divides integers, the SDK keeps the fraction:

| field | type | meaning |
|---|---|---|
| `distanceMm` | `int[]` | per-zone distance in mm (raw /4; a negative distance is clamped to 0, as ST) |
| `targetStatus` | `int[]` | 5/9 = valid, 255 = no target |
| `nbTargetDetected` | `int[]` | targets found in the zone |
| `signalPerSpad` | `double[]` | signal rate, kcps/SPAD (raw /2048) |
| `ambientPerSpad` | `double[]` | ambient rate, kcps/SPAD (raw /2048) |
| `nbSpadsEnabled` | `long[]` | enabled SPADs per zone |
| `rangeSigmaMm` | `double[]` | range std-dev estimate, mm (raw /128) |
| `reflectance` | `double[]` | estimated reflectance, % (raw /2) |
| `siliconTempDegc` | `int` | per-frame sensor temperature, °C |
| `cnhRaw` | `byte[]` | CNH block bytes (CH only; `null` on CX) |

`resolution()` returns the zone count. Zone index runs row-major; reshape to
`(4,4)` / `(8,8)` yourself for a heatmap. Mask invalid zones on
`targetStatus == 255`.

## Advanced-DCI codecs

These are the pure **codecs** for the ST ULD plugins — the payloads
`Vl53l8Driver` writes over DCI (`setXtalkMargin`, `setDetectionThresholds`,
`motionIndicatorInit`), usable on their own for another transport.

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
- **Real units, as ST** — `distanceMm` is mm (a negative distance comes out as
  0), `signalPerSpad` / `ambientPerSpad` kcps/SPAD, `rangeSigmaMm` mm,
  `reflectance` %; no raw fixed-point values remain.
- **`cnhRaw` is CH-only** — it is `null` on CX; on CH `Vl53l8Uld.decodeCnh`
  unpacks it (see the [VL53L8CH guide](../vl53l8ch/guide.md)).
- **One driver, any transport** — `Vl53l8Driver` runs over the
  `Vl53l8Platform` interface; the device class maps it onto the register
  bridge, a test or another bridge can script it.
