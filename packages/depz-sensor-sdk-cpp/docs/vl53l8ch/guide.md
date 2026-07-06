# VL53L8CH — user guide

Hands-on guide to the CH side of the ToF decode layer. For what the sensor is
and why CH exists, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md).

**The CH shares the entire VL53L8CX decode surface.** Reassembly, `decode_frame`,
the `Vl53l8Frame` fields and the zone grid, and every advanced-feature DCI codec
work exactly as in the [VL53L8CX user guide](../vl53l8cx/guide.md) — read that
first. This page covers **only the CH-specific footer offset and the CNH
extension point**.

## Contents

- [Decoding a CH frame](#decoding-a-ch-frame)
- [The CNH extension point](#the-cnh-extension-point)
- [Gotchas](#gotchas)

## Decoding a CH frame

Identical to the CX, except you pass the CH footer-id offset so the decoder's
header/footer id check matches the VL53LMZ 2.0.16 layout:

```cpp
#include "depz/vl53l8.hpp"

using depz::vl53l8::Variant;
// … reassemble chunks exactly as in the CX guide …
auto frame = depz::vl53l8::decode_frame(depz::as_bytes(done->second),
                                        depz::vl53l8::footer_id_off(Variant::CH));
//                                       == depz::vl53l8::FOOTER_ID_OFF_CH (4)
```

Everything else — `FrameChunk`, `FrameReassembler`, the `Vl53l8Frame` fields,
`swap_buffer`, and the xtalk / threshold / motion codecs — is the shared surface
documented in the [CX guide](../vl53l8cx/guide.md).

## The CNH extension point

CNH is the CH's addition over the CX, and its decode is **intentionally not
implemented** in this SDK — a documented stub in `depz/vl53l8.hpp` rather than a
fabricated codec. The reasoning, verbatim from the header: no decoder is
provided rather than a fabricated one; when the CNH wire format is
contract-pinned, the codec is added in that section (namespaced under
`depz::vl53l8`) alongside a golden-vector suite.

Until then:

- The shared results-frame path (`decode_frame` with `Variant::CH`'s footer
  offset) already serves CH **ranging** output.
- Only the CNH **histogram** stream is unimplemented.

If you need CNH today, capture the raw block off the wire and decode it with the
reference Python `depz_sensor_sdk.vl53l8.cnh`; this C++ layer will gain a
byte-exact port when the format is pinned.

## Gotchas

- **Use the CH footer offset** — `FOOTER_ID_OFF_CH` (4), not the CX default
  (12); the wrong offset fails every frame's id check.
- **CNH decode is a stub** — the histogram stream is not decoded here yet; only
  the shared ranging frame is. This is deliberate and documented.
- **Everything else is the CX surface** — reassembly, the frame fields, the DCI
  codecs; all the [CX gotchas](../vl53l8cx/guide.md#gotchas) apply unchanged.
