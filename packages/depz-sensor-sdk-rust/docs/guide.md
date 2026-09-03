# depz-sensor-sdk (Rust) — guide

The common guide to the Rust SDK for the DEPZ USB sensor line. It covers what
the crate is, how to depend on it, the shared mental model, and the
cross-sensor building blocks (framing & CRC, USB-id discovery, firmware-name
identity, the `.fwdepz` container, and the `.depzdata` dataset reader). Each
sensor then has its own **introduction** and **user guide**:

- **SR04** (HC-SR04 ultrasonic) — [introduction](sr04/introduction.md) ·
  [guide](sr04/guide.md) · [api](sr04/api.md)
- **VL53L4CD** (single-zone ToF) — [introduction](vl53l4cd/introduction.md) ·
  [guide](vl53l4cd/guide.md) · [api](vl53l4cd/api.md)
- **VL53L8CX** (8×8 ToF base) — [introduction](vl53l8cx/introduction.md) ·
  [guide](vl53l8cx/guide.md) · [api](vl53l8cx/api.md)
- **VL53L8CH** (ToF superset + CNH histograms) —
  [introduction](vl53l8ch/introduction.md) · [guide](vl53l8ch/guide.md) ·
  [api](vl53l8ch/api.md)
- **BNO086** (9-axis IMU) — [introduction](bno086/introduction.md) ·
  [guide](bno086/guide.md) · [api](bno086/api.md)

For the exhaustive symbol-by-symbol reference see [api.md](api.md), generated
from the source `///` doc-comments so it never drifts from the code.

## Contents

- [What it is](#what-it-is)
- [What it is not](#what-it-is-not)
- [Add the crate](#add-the-crate)
- [Getting started](#getting-started)
- [Mental model](#mental-model)
- [Transport: framing & CRC](#transport-framing--crc)
- [Discovery: USB ids & identity](#discovery-usb-ids--identity)
- [Per-sensor decode](#per-sensor-decode)
- [Firmware container (`.fwdepz`)](#firmware-container-fwdepz)
- [Datasets (record & replay)](#datasets-record--replay)
- [Extension points](#extension-points)
- [Testing & golden vectors](#testing--golden-vectors)

## What it is

Every DEPZ sensor is a USB CDC-ACM device speaking one shared framed protocol
(`A5 C3` header + CRC). This crate is the **contract-first, transport-agnostic
foundation**: the byte-exact codecs, framing/parsing, CRCs, USB-id table,
firmware-name identity, per-sensor frame/report decoders, the `.fwdepz`
firmware-container parser, and the `.depzdata` dataset reader — everything you
need to turn bytes on the wire into typed values, verified against the shared
golden vectors in `contracts/vectors/` and byte-for-byte identical to the
Python / TypeScript / Java / C / C++ reference SDKs.

Five sensors across three firmware philosophies — mirrored from the firmware:

- **SR04** — the device does the ranging; you decode `echo_time_us` → distance
  ([`protocol::sr04`](sr04/api.md)).
- **VL53L4CD** — the device is a thin I2C register bridge; the ST ULD 2.2.3
  math (result block, range timing, tuning words) runs **on the host**
  ([`vl53l4`](vl53l4cd/api.md)).
- **VL53L8CX / VL53L8CH** — the device is a thin SPI bridge; the ST ULD frame
  layout is decoded **on the host** ([`vl53l8`](vl53l8cx/api.md)). CX is the
  base ToF imager; CH is its superset, adding Compact-Network-Histogram output.
- **BNO086** — the device is an SHTP pass-through; the SH-2 stack is decoded
  **on the host** ([`bno086`](bno086/api.md)).

Everything is pure computation over `&[u8]` — no threads, no alloc(beyond the
returned `Vec`s), no runtime dependencies.

## What it is not

This crate is the **verifiable decode + protocol layer only**. It deliberately
carries **no live serial-I/O layer**: no port enumeration, no reader thread, no
`open_device()`. You bring the bytes (from a `serialport` crate, a socket, a
capture file, a test vector) and feed them to the parsers here; you take the
frames these builders produce and write them to your transport. Two things are
documented **extension points**, present in shape but not yet decoded:

- **Live ULD init/config** (VL53L8 firmware download + the DCI register bridge
  over the wire) — hardware-dependent.
- **CNH histogram decode** (VL53L8CH) — the raw CNH block is surfaced as
  [`Vl53l8Results::cnh_raw`](vl53l8ch/api.md), not unpacked into per-zone
  histograms.

The crate never fabricates a decode it cannot verify against a golden vector.

## Add the crate

Published on crates.io as `depz-sensor-sdk` (0.1.4):

```bash
cargo add depz-sensor-sdk
```

Or, from a source checkout, depend on it by path:

```toml
[dependencies]
depz-sensor-sdk = { path = "../depz-sensor-sdk-rust" }
```

The crate name on disk is `depz-sensor-sdk`; the library imports as
`depz_sensor_sdk`. It targets stable Rust (edition 2021) and pulls in no
runtime dependencies.

## Getting started

Frame a command, parse a reply, decode it — the whole round trip is synchronous
and pure:

```rust
use depz_sensor_sdk::framing::{build_packet, CrcType, Event, PacketParser};
use depz_sensor_sdk::protocol::sr04::{Sr04Cmd, Sr04Data, distance_mm_from_echo};

// Host → device: frame the "start measurement loop" command.
let tx = build_packet(Sr04Cmd::StartMeasurementLoop as u8, &[], 0, CrcType::None).unwrap();
// ... write `tx` to your serial port ...

// Device → host: feed whatever bytes arrive; drain decoded packets.
let mut parser = PacketParser::new();
for ev in parser.feed(&rx_bytes) {
    if let Event::Packet(pkt) = ev {
        if pkt.cmd == 0x91 {                       // RPT_DATA
            let d = Sr04Data::unpack(&pkt.payload).unwrap();
            println!("{:?} mm", distance_mm_from_echo(d.echo_time_us, None));
        }
    }
}
```

From there, follow the per-sensor guide linked above.

## Mental model

```
your transport (serialport / socket / file)
        │  bytes in                       bytes out  ▲
        ▼                                            │
  PacketParser.feed(&[u8]) ──► [Event]        build_packet(cmd, payload, seq, crc)
        │                                            │
        │ Event::Packet{cmd, seq, payload}           │
        ▼                                            │
  per-sensor codec / decoder                   per-sensor command builder
  (Sr04Data, vl53l8::parse_frame,              (Sr04Cmd, sh2::build_*,
   bno086::parse_input_cargo, …)                common::pack_*)
```

- **Framing is incremental.** `PacketParser` takes arbitrary byte chunks and
  yields `Event::Packet` / `Event::Trash` / `Event::CrcError`; event order is
  invariant to how the stream was chunked (contract 01 §5).
- **Correlation is by echoed command byte.** Solicited replies echo the request
  opcode; there is no sequence-number handshake (that logic lives in a device
  layer you build on top).
- **Timestamps are device microseconds.** `common::sync_time_offset_rtt`
  gives you the NTP-style `(offset_us, rtt_us)` to map device µs onto a host
  clock — the same math a multi-device `.depzdata` recording is aligned by.
- **Raw integers are authoritative.** Codecs return values exactly as on the
  wire (raw fixed-point, sentinels intact); unit conversion is a separate,
  explicit step (`distance_mm_from_echo`, `TemperatureReport::celsius`, the Q
  points in `bno086::reports`).

## Transport: framing & CRC

[`framing`](api.md#transport) is the shared packet layer, byte-exact with the
firmware `transport.c`:

```rust
use depz_sensor_sdk::framing::{build_packet, CrcType, Event, PacketParser, MAGIC};

let bytes = build_packet(0x06, &payload, seq, CrcType::Crc16).unwrap();

let mut p = PacketParser::new();
let events = p.feed(&chunk);           // feed any sized chunk, repeatedly
// p.packets / p.crc_errors / p.header_errors / p.trash_bytes are live counters
```

`CrcType` selects the payload trailer (`None`/`Crc8`/`Crc16`/`Crc32`), encoded
in the top two bits of the size field. Per firmware **ERRATA E6**, a header can
advertise a CRC type while an *empty* payload carries no CRC bytes — both
`build_packet` and the parser honour that. The four wire CRCs live in
[`crc`](api.md#transport) (`crc8_maxim`, `crc16_modbus`, `crc32_iso_hdlc`, and
`crc16_ccitt_false` for the `.fwdepz` header only).

## Discovery: USB ids & identity

There is no port enumeration here, but the two decisions discovery needs are:

```rust
use depz_sensor_sdk::usb_ids::{is_known_depz_usb, usb_model_hint, order_ports, PortEntry};
use depz_sensor_sdk::protocol::identity::{parse_software_name, SensorType};

is_known_depz_usb(Some(0x1BCF), Some(0xED40));   // true — a DEPZ VL53L8CH
usb_model_hint(Some(0x1BCF), Some(0xEC78));      // Some("sr04") — informational only

// Order candidate ports deterministically by USB iSerial (empty serials last).
let ports = order_ports(vec![PortEntry { port: "/dev/ttyACM0".into(), serial: Some("SN2".into()) }]);
```

The `(vid, pid)` table is an **informational hint**; the protocol probe
(`GET_NAME_ACTIVE_SOFTWARE`) is authoritative. Feed its stripped reply to
[`parse_software_name`](api.md#discovery) to classify the device into an
[`Identity`](api.md#discovery) — `mode` (app / bootloader / unknown),
`sensor_type` ([`SensorType`](api.md#discovery)) and firmware `version`.

## Per-sensor decode

The typed layer per sensor — see each sensor's own guide and API reference:

- **SR04** — [`protocol::sr04`](sr04/api.md): `Sr04Data::unpack`,
  `distance_mm_from_echo`, the sample-period / echo-decay codecs.
- **VL53L4CD** — [`vl53l4`](vl53l4cd/api.md): the register-bridge command
  packers and report unpackers (`RegData` / `Vl53l4Info` / `StreamData`) plus
  the pure ULD math — `parse_result_block`, range timing, tuning words.
- **VL53L8CX / CH** — [`vl53l8`](vl53l8cx/api.md): `FrameReassembler` rebuilds a
  frame from `RPT_VL53_FRAME` chunks, `parse_frame` decodes it into per-zone
  arrays, `advanced` holds the DCI codecs (motion, xtalk, thresholds).
- **BNO086** — [`bno086`](bno086/api.md): `ShtpLayer` reassembles SHTP cargos,
  `parse_input_cargo` / `parse_gyro_rv_cargo` decode reports, `sh2` builds
  control requests.

Common command/report codecs shared by all sensors (temperature, sync-time,
status, text, sync-pins) live in [`protocol::common`](api.md#protocol-codecs).

## Firmware container (`.fwdepz`)

[`fwdepz::parse`](api.md#bootloader--firmware-update) validates an application
firmware image and reports the header plus a payload-CRC verdict — magic →
header CRC-16/CCITT-FALSE → `fw_size == len(payload)`, then a CRC-32 check of
the body:

```rust
use depz_sensor_sdk::fwdepz;

let hdr = fwdepz::parse(&blob)?;      // FwdepzError on bad magic / header CRC / size
assert!(hdr.payload_crc_ok);          // body CRC-32 matches fw_crc32
// hdr.load_addr / hdr.fw_size / hdr.tot_sec drive the (host-side) flash flow
```

Driving the bootloader over the wire is a device-layer concern; this crate
verifies the container.

## Datasets (record & replay)

[`dataset::Dataset::parse`](api.md#datasets-record--replay) reads a `.depzdata`
document — UTF-8 JSON Lines, a header describing devices on a shared host
timeline followed by one decoded record per line — using the crate's own
dependency-free [`json`](api.md#json) parser:

```rust
use depz_sensor_sdk::dataset::{Dataset, RecordValue};

let ds = Dataset::parse(&text)?;
for rec in &ds.records {                       // file order (already merged by host time)
    if let RecordValue::Sr04 { echo_us, source } = &rec.value {
        println!("{} {} echo={}", rec.device, source, echo_us);
    }
}
let left = ds.records_for("d0");               // one device's records
```

Record times (`t`) are host-monotonic µs — the device timestamp mapped through
that device's `time_sync.offset_us`, so records from every device (including
two of the same model) share one timeline. Unknown record kinds keep their raw
JSON as `RecordValue::Other`.

## Extension points

Truthfully, two layers are present in shape but not decoded (see
[What it is not](#what-it-is-not)):

- **Live ULD init/config** — the VL53L8 firmware download and the DCI register
  bridge that performs writes over the wire. The pure DCI byte layouts are here
  (`vl53l8::advanced`); the bridge that ships them is not.
- **CNH histogram decode** — a VL53L8CH-only feature. `parse_frame` surfaces the
  raw CNH block as `Vl53l8Results::cnh_raw`; unpacking it into per-aggregate,
  per-bin histograms is not yet implemented. See the
  [VL53L8CH guide](vl53l8ch/guide.md).

## Testing & golden vectors

Everything here is exercised against the shared golden vectors in
`contracts/vectors/` (the same fixtures the other reference SDKs verify
against), plus a real VL53L8 capture replayed end-to-end and the SH-2 report
catalog:

```bash
cargo test
```

Because every codec is pure `&[u8]` → value, you can unit-test the full stack
with no hardware — feed a captured buffer to `PacketParser` / `FrameReassembler`
/ `ShtpLayer` and assert on the decoded result.
