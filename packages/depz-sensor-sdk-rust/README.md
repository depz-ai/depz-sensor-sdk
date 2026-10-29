# depz-sensor-sdk (Rust)

Contract-first Rust SDK for the **DEPZ USB sensor line** — USB CDC-ACM sensors
that speak one shared framed protocol. The first four sensors of the line:

- **HC-SR04** — ultrasonic distance.
- **VL53L8CX** — 8×8 multizone Time-of-Flight (base ToF; host runs the ST ULD).
  This is the dev/unprogrammed default.
- **VL53L8CH** — the CX plus compact-network-histogram (CNH) output, and its own
  production USB PID `0xED40`. Everything CX does, plus CNH.
- **BNO086** — 9-axis IMU (host runs the SH-2 stack).

Further boards, each with its decode and codec layer here and its live device
class in `devices` (`Sr04Device`, `Vl53l4cdDevice`, `Vl53l8cxDevice` /
`Vl53l8chDevice`, `Vl53l5cxDevice` / `Vl53l7cxDevice` / `Vl53l7chDevice`,
`Bno055Device`, `Bno086Device`, `Tmf8829Device`, and the 1D family's
`Vl53lxDevice` with `Vl53l0xDevice`, `Vl53l1cxDevice`, `Vl53l1cbDevice`,
`Vl53l3cxDevice`, `Vl53l4cxDevice`):

- **VL53L0X / VL53L1CX / VL53L1CB / VL53L3CX / VL53L4CX** — the 1D ToF family
  on one I2C register-bridge firmware.
- **VL53L4CD** — single-zone ToF behind an I2C register bridge (host runs the
  ST ULD math).
- **VL53L5CX / VL53L7CX / VL53L7CH** — 8×8 multizone ToF on one I2C board
  firmware; the VL53L8 frame layout, and CNH histograms on the L7CH.
- **TMF8829** — ams-OSRAM multizone dToF (8×8 … 48×32 zones, up to 4 peaks,
  raw histograms) behind an SPI register bridge; the host runs the sensor,
  the crate embeds its MIT firmware image.
- **BNO055** — 9-axis IMU with Bosch's fusion on the chip, behind an I2C
  register bridge.

The two ToF parts (CX and CH) share one results-frame layout, so the SDK exposes
them through a single decode module with a `vl53l8::Variant` selector rather
than duplicated code — see coverage below. This crate mirrors the Python / TS /
Java / C / C++ reference SDKs byte-for-byte via the shared golden vectors in
`contracts/vectors/`.

## Install

Published on crates.io as `depz-sensor-sdk` (0.9.0):

```bash
cargo add depz-sensor-sdk
```

The crate name is `depz-sensor-sdk`; the library imports as `depz_sensor_sdk`.
It targets stable Rust (edition 2021). Its one runtime dependency is the
[`serialport`](https://crates.io/crates/serialport) crate, which opens the
board's USB serial port on Windows, macOS and Linux; on Linux it builds against
libudev (`sudo apt-get install libudev-dev` on Debian / Ubuntu).

## Documentation

Full docs live under [`docs/`](docs/):

- [Common guide](docs/guide.md) — what the crate is, framing & CRC, discovery,
  the `.fwdepz` container, live devices, datasets, testing.
- Per sensor — **SR04** ([intro](docs/sr04/introduction.md) ·
  [guide](docs/sr04/guide.md) · [api](docs/sr04/api.md)), **VL53L8CX**
  ([intro](docs/vl53l8cx/introduction.md) · [guide](docs/vl53l8cx/guide.md) ·
  [api](docs/vl53l8cx/api.md)), **VL53L8CH**
  ([intro](docs/vl53l8ch/introduction.md) · [guide](docs/vl53l8ch/guide.md) ·
  [api](docs/vl53l8ch/api.md)), **TMF8829**
  ([intro](docs/tmf8829/introduction.md) · [guide](docs/tmf8829/guide.md) ·
  [api](docs/tmf8829/api.md)), **BNO086**
  ([intro](docs/bno086/introduction.md) · [guide](docs/bno086/guide.md) ·
  [api](docs/bno086/api.md)), **VL53L4CD**
  ([intro](docs/vl53l4cd/introduction.md) · [guide](docs/vl53l4cd/guide.md) ·
  [api](docs/vl53l4cd/api.md)), **VL53L5CX**
  ([intro](docs/vl53l5cx/introduction.md) · [guide](docs/vl53l5cx/guide.md) ·
  [api](docs/vl53l5cx/api.md)), **VL53L7CX**
  ([intro](docs/vl53l7cx/introduction.md) · [guide](docs/vl53l7cx/guide.md) ·
  [api](docs/vl53l7cx/api.md)), **VL53L7CH**
  ([intro](docs/vl53l7ch/introduction.md) · [guide](docs/vl53l7ch/guide.md) ·
  [api](docs/vl53l7ch/api.md)), **VL53L0X**
  ([intro](docs/vl53l0x/introduction.md) · [guide](docs/vl53l0x/guide.md) ·
  [api](docs/vl53l0x/api.md)), **VL53L1CX**
  ([intro](docs/vl53l1cx/introduction.md) · [guide](docs/vl53l1cx/guide.md) ·
  [api](docs/vl53l1cx/api.md)), **VL53L1CB**
  ([intro](docs/vl53l1cb/introduction.md) · [guide](docs/vl53l1cb/guide.md) ·
  [api](docs/vl53l1cb/api.md)), **VL53L3CX**
  ([intro](docs/vl53l3cx/introduction.md) · [guide](docs/vl53l3cx/guide.md) ·
  [api](docs/vl53l3cx/api.md)), **VL53L4CX**
  ([intro](docs/vl53l4cx/introduction.md) · [guide](docs/vl53l4cx/guide.md) ·
  [api](docs/vl53l4cx/api.md)), **BNO055**
  ([intro](docs/bno055/introduction.md) · [guide](docs/bno055/guide.md) ·
  [api](docs/bno055/api.md)).
- [Full API reference](docs/api.md) — every public symbol, generated from the
  source `///` doc-comments by `scripts/gen_api_md.py`
  (`python3 scripts/gen_api_md.py`).

## Coverage today

The live layer (`devices`) opens the board's USB serial port itself —
`open_device("/dev/ttyACM0", None)`, `open_first_device(None)`,
`list_depz_devices(None, true, None)` — or runs on any `Link` you hand it (a
`.depzrec` replay, a socket). Under it, the **verifiable decode + protocol
foundation**:

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
- **CNH histogram decode is CH-only.** When a CH frame carries a CNH block its
  raw bytes are surfaced as `Vl53l8Results::cnh_raw`; `vl53l8::decode_cnh`
  unpacks them into per-aggregate histograms, given the aggregate count and bins
  per aggregate the sensor runs (verified against
  `contracts/vectors/vl53l8_cnh.json`).
- **Live ULD init/config (firmware download + DCI register bridge over the
  wire)** is hardware-dependent and likewise a documented extension point, out
  of scope for this decode-layer crate.

The register-bridge boards:

| Area | VL53L0X / L1CX / L1CB / L3CX / L4CX | VL53L5CX / L7CX / L7CH | BNO055 | TMF8829 |
|------|-------------------------------------|------------------------|--------|---------|
| Identity / class resolution | ✅ (`vl53lx`, `resolve_class`, product table) | ✅ (`vl53l7`, `resolve_model`) | ✅ (`bno055`) | ✅ (`tmf8829`) |
| Command encoders / report decoders | ✅ (v2.00: address width, stream with clear list, info) | ✅ (pin, bus speed, info + the VL53L8 register codecs) | ✅ (read/write/reset/stream/info) | ✅ (read/write/stream/info, frame chunks, events, reassembler) |
| Stateless decode | ✅ die block, VL53L0X raw fields, histogram bins | ✅ frames via `vl53l8::parse_frame(Variant::L7)`; CNH via `decode_cnh` (L7CH) | ✅ units, calibration, axis remap, page-1 configs, register windows | ✅ frames, results pixels, half-frame merge, clock model + capture time, measurements, config fields, firmware image |
| Live driver (init, configuration, ranging) | — (Python / TypeScript SDK) | — (Python / TypeScript SDK) | — (Python / TypeScript SDK) | ✅ (`devices::Tmf8829Device`) |

## Layout

- `vl53lx` — VL53L0X / L1CX / L1CB / L3CX / L4CX codecs, product table, block
  decode.
- `vl53l4` — VL53L4CD register-bridge codecs, result-block decode, ULD math.
- `vl53l7` — VL53L5CX / L7CX / L7CH board codecs and class resolution.
- `vl53l8` — frame reassembly, raw-frame decode (`Variant` CX/CH/L7), advanced
  DCI, CNH histogram decode.
- `bno055` — BNO055 register-bridge codecs, register codecs, window decode.
- `tmf8829` — TMF8829 bridge codecs + reassembler, frame / pixel decode,
  `SensorClock` / `CaptureTimer`, `MeasurementBuilder`, config fields, the
  embedded sensor firmware.
- `bno086` — SHTP framing, SH-2 report parsers, control request builders.
- `protocol`, `framing`, `crc`, `fwdepz`, `usb_ids` — shared transport/protocol.
- `dataset` — `.depzdata` decoded multi-device datasets: reader, writer, session recorder, player (contract 09).

## Test

```bash
cargo test
cargo fmt --check && cargo clippy --all-targets -- -D warnings   # the lint gate
scripts/coverage.sh       # line coverage with cargo-llvm-cov
```

`scripts/coverage.sh` (needs `cargo install cargo-llvm-cov --locked`) lists the
least-covered files of `src/` and fails below the floor in the script
(`FLOOR`, the measured total rounded down — raise it when the tests grow,
never lower it to make a change pass). The serial-port link
(`src/devices/serial.rs`) needs a board and is listed apart.

**On real boards:** `cargo run --example live_check -- [seconds] [port ...]` opens every DEPZ board on the machine, streams 10 s and
prints its rate and a plausibility value — the centre distance of a ToF sensor,
|gravity| of a BNO055. The Python, Rust, Java and C# SDKs carry the same check;
on 2026-10-26 all four agreed on ten boards.

## License

MIT. Bundled VL53L8 sensor-firmware blobs are © STMicroelectronics
(BSD-3-Clause).

Open source — source and issue tracker:
<https://github.com/depz-ai/depz-sensor-sdk>.
