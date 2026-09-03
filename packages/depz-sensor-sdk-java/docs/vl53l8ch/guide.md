# VL53L8CH — user guide

Hands-on guide to the CH-specific surface of the ToF decoder. For what the
sensor is and why CH exists, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md).

**CH reuses the entire VL53L8CX decode surface.** The frame path
(`FrameReassembler`), `Vl53l8Uld.parseFrame`, the `Results` fields, and every
advanced-DCI codec (xtalk, thresholds, motion) work exactly as in the
[VL53L8CX user guide](../vl53l8cx/guide.md) — read that first. This page covers
**only the CH additions**: the `Variant.CH` footer and the CNH block.

## Contents

- [Parsing a CH frame](#parsing-a-ch-frame)
- [The CNH block](#the-cnh-block)
- [Gotchas](#gotchas)

## Parsing a CH frame

Identical to the CX, except you pass `Variant.CH` so `parseFrame` uses the CH
footer-id offset (4 instead of 12):

```java
import ai.depz.sensor.sensors.vl53l8.*;

FrameReassembler reasm = new FrameReassembler();
// ... feed chunk reports through reasm.feed(...) as in the CX guide ...

FrameReassembler.CompletedFrame done = reasm.feed(chunk);
if (done != null) {
    Vl53l8Uld.Results r = Vl53l8Uld.parseFrame(done.frame(), done.frame().length, Vl53l8Uld.Variant.CH);
    // r.distanceMm, r.targetStatus, ... — the full CX Results surface
    byte[] cnh = r.cnhRaw;      // the CH-only extra; null when the frame carries no CNH block
}
```

Everything on `Results` is the same as CX (see the
[CX frame fields](../vl53l8cx/guide.md#the-results-frame-fields)); the one extra
is `cnhRaw`.

## The CNH block

When a CH frame carries a Compact Network Histogram, its raw bytes ride along on
`Vl53l8Uld.Results.cnhRaw`. This SDK **surfaces them but does not decode them** —
histogram decode is a CH-only extension point:

```java
if (r.cnhRaw != null) {
    // Vl53l8Uld.cnhHistogramDecodeStubbed() == true: decode not implemented here.
    // The raw block is available for downstream tooling or a future decoder.
}
```

`Vl53l8Uld.cnhHistogramDecodeStubbed()` returns `true` to mark that boundary. A
future CH build would add the ST CNH-plugin port at that point; the raw bytes it
would consume are already surfaced.

## Gotchas

- **Pass `Variant.CH`** — using `Variant.CX` on a CH frame trips the
  header/footer id check (offset 12 vs 4) and throws `Vl53l8Uld.Vl53l8Error`.
- **`cnhRaw` may be `null`** — it is `null` on CX and on CH frames without a CNH
  block; null-check before using it.
- **CNH decode is stubbed** — only the raw block is exposed
  (`cnhHistogramDecodeStubbed()` == `true`). Everything else is inherited
  unchanged from the [CX guide](../vl53l8cx/guide.md).
- All the CX gotchas (chunked frames, applied scaling, the stubbed live driver)
  apply here too — see the [CX guide](../vl53l8cx/guide.md#gotchas).
