# VL53L8CH — user guide

Hands-on guide to the VL53L8CH, the CNH superset of the ToF sensor. For what the
sensor is and why CH exists, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md).

**The VL53L8CH shares the entire VL53L8CX decode surface.** Chunk parse, frame
reassembly, the ranging-frame decoder and every advanced DCI codec work exactly
as in the [VL53L8CX user guide](../vl53l8cx/guide.md) — read that first. This page
covers **only the CNH story**, which today is a documented extension point.

## Contents

- [Decoding CH ranging frames](#decoding-ch-ranging-frames)
- [CNH: the extension point](#cnh-the-extension-point)
- [Gotchas](#gotchas)

## Decoding CH ranging frames

A CH device's ordinary ranging frames are **byte-identical** to CX frames, so you
decode them with the same pipeline — nothing changes:

```c
#include <depz_sensor_sdk.h>

depz_vl53l8_reassembler reasm;
depz_vl53l8_reasm_init(&reasm);

// in your RPT_FRAME handler (identical to the CX guide):
depz_vl53l8_chunk chunk;
depz_vl53l8_unpack_chunk(ev->payload, ev->payload_len, &chunk);

const uint8_t *raw; size_t raw_len; uint64_t ts;
if (depz_vl53l8_reasm_feed(&reasm, &chunk, &raw, &raw_len, &ts) == 1) {
    depz_vl53l8_frame f;
    depz_vl53l8_decode_frame(raw, raw_len, ts, &f);   // same decoder as CX
}
```

`depz_vl53l8_variant` exists to name which silicon you are talking to
(`DEPZ_VL53L8_VARIANT_CH`); the decoder is variant-agnostic and needs no variant
argument.

## CNH: the extension point

CNH (Compact Network Histograms) is the one thing CH does that CX does not — and
it is **not yet implemented** in this SDK. The CNH histogram frame is a distinct
on-wire layout from the ranging frame, so `depz_vl53l8_decode_frame()` does **not**
handle it; the gap is marked, not faked, in `src/vl53l8_decode.c`.

Consequently there is no `depz_vl53l8_cnh_*` API to call yet. If you need CNH
today, capture the raw CH histogram block off the wire and decode it against your
config out-of-band (see the TS/Python `cnh` reference for the layout). When CNH
decode lands in this SDK it will be a new decoder alongside
`depz_vl53l8_decode_frame`, documented on this page.

## Gotchas

- **CH ranging frames use the CX decoder** — there is no separate CH frame path;
  everything in the [CX guide](../vl53l8cx/guide.md) applies unchanged.
- **CNH is not decoded here** — it is a documented gap, not a silent one. Don't
  expect `depz_vl53l8_decode_frame` to parse a CNH histogram frame.
- **The variant enum is just a label** — `DEPZ_VL53L8_VARIANT_CH` names the
  silicon; it does not switch any decode behaviour (the ranging layout is
  shared).
- All the CX gotchas (raw fixed-point values, check the decode return, gaps
  discard the in-progress frame, row-major zones) apply here too — see the
  [CX guide](../vl53l8cx/guide.md#gotchas).
