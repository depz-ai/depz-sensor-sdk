# depz-sensor-sdk (Rust)

Contract-first Rust SDK for the **DEPZ USB sensor line** — USB CDC-ACM sensors
that speak one shared framed protocol. The line is **four distinct sensors**:

- **HC-SR04** — ultrasonic distance.
- **VL53L8CX** — 8×8 multizone Time-of-Flight (base ToF; host runs the ST ULD).
  This is the dev/unprogrammed default.
- **VL53L8CH** — the CX plus compact-network-histogram (CNH) output, and its own
  production USB PID `0xED40`. Everything CX does, plus CNH.
- **BNO086** — 9-axis IMU (host runs the SH-2 stack).

The two ToF parts (CX and CH) share one results-frame layout, so the SDK exposes
them through a single decode module with a `vl53l8::Variant` selector rather
than duplicated code — see coverage below. This crate mirrors the Python / TS /
Java / C / C++ reference SDKs byte-for-byte via the shared golden vectors in
`contracts/vectors/`.

## Install

Published on crates.io as `depz-sensor-sdk` (0.1.4):

```bash
cargo add depz-sensor-sdk
```

The crate name is `depz-sensor-sdk`; the library imports as `depz_sensor_sdk`.
It targets stable Rust (edition 2021) with no runtime dependencies.

## Documentation

Full docs live under [`docs/`](docs/):

- [Common guide](docs/guide.md) — what the crate is, framing & CRC, discovery,
  the `.fwdepz` container, datasets, extension points, testing.
- Per sensor — **SR04** ([intro](docs/sr04/introduction.md) ·
  [guide](docs/sr04/guide.md) · [api](docs/sr04/api.md)), **VL53L8CX**
  ([intro](docs/vl53l8cx/introduction.md) · [guide](docs/vl53l8cx/guide.md) ·
  [api](docs/vl53l8cx/api.md)), **VL53L8CH**
  ([intro](docs/vl53l8ch/introduction.md) · [guide](docs/vl53l8ch/guide.md) ·
  [api](docs/vl53l8ch/api.md)), **BNO086**
  ([intro](docs/bno086/introduction.md) · [guide](docs/bno086/guide.md) ·
  [api](docs/bno086/api.md)).
- [Full API reference](docs/api.md) — every public symbol, generated from the
  source `///` doc-comments by `scripts/gen_api_md.py`
  (`python3 scripts/gen_api_md.py`).

## Coverage today

This crate is the **verifiable decode + protocol foundation** (transport-
agnostic; no live serial I/O layer yet):

| Area | SR04 | VL53L8CX | VL53L8CH | BNO086 |
|------|------|----------|----------|--------|
| USB-id discovery / model hint | ✅ | ✅ | ✅ (PID `0xED40`) | ✅ |
| Identity (firmware-name) parse | ✅ | ✅ (`vl53l8`) | ✅ (`vl53l8`) | ✅ |
| Packet framing / CRC / `.fwdepz` | shared across all four | | | |
| Frame decode | ✅ | ✅ | ✅ (shared frame path) | ✅ (SHTP + SH-2) |
| Advanced DCI codecs | — | ✅ | ✅ (shared) | control builders |

Notes on the ToF pair:

- **Shared VL53L8 frame decode serves both CX and CH.** The results-frame block
  layout is identical; only the frame-tail footer-id offset differs and is
  selected by `Variant::{Cx, Ch}` (`size-12` for ULD 2.1.0 vs `size-4` for ULD
  2.0.16). The advanced DCI codecs (motion, xtalk margin, detection thresholds)
  are likewise shared.
- **CNH histogram decode is a CH-only extension point that is not yet
  implemented.** When a CH frame carries a CNH block its raw bytes are surfaced
  as `Vl53l8Results::cnh_raw`, but they are not unpacked into per-zone
  histograms. The SDK does not fabricate a CH-specific decode it cannot verify.
- **Live ULD init/config (firmware download + DCI register bridge over the
  wire)** is hardware-dependent and likewise a documented extension point, out
  of scope for this decode-layer crate.

## Layout

- `vl53l8` — frame reassembly, raw-frame decode (`Variant` CX/CH), advanced DCI.
- `bno086` — SHTP framing, SH-2 report parsers, control request builders.
- `protocol`, `framing`, `crc`, `fwdepz`, `usb_ids` — shared transport/protocol.
- `dataset` — `.depzdata` decoded multi-device dataset reader (contract 09).

## Test

```bash
cargo test
```

## License

MIT. Bundled VL53L8 sensor-firmware blobs are © STMicroelectronics
(BSD-3-Clause).

Open source — source and issue tracker:
<https://github.com/depz-ai/depz-sensor-sdk>.
