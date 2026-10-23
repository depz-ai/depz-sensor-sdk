# depz-sensor-sdk (Rust) — guide

The common guide to the Rust SDK for the DEPZ USB sensor line. It covers what
the crate is, how to depend on it, the shared mental model, and the
cross-sensor building blocks (framing & CRC, USB-id discovery, firmware-name
identity, the `.fwdepz` container, and the `.depzdata` datasets). Each
sensor then has its own **introduction** and **user guide**:

- **SR04** (HC-SR04 ultrasonic) — [introduction](sr04/introduction.md) ·
  [guide](sr04/guide.md) · [api](sr04/api.md)
- **VL53L0X / VL53L1CX / VL53L1CB / VL53L3CX / VL53L4CX** (the 1D ToF family) —
  [VL53L0X](vl53l0x/introduction.md) · [VL53L1CX](vl53l1cx/introduction.md) ·
  [VL53L1CB](vl53l1cb/introduction.md) · [VL53L3CX](vl53l3cx/introduction.md) ·
  [VL53L4CX](vl53l4cx/introduction.md), each with its guide and api
- **VL53L4CD** (single-zone ToF) — [introduction](vl53l4cd/introduction.md) ·
  [guide](vl53l4cd/guide.md) · [api](vl53l4cd/api.md)
- **VL53L5CX / VL53L7CX** (8×8 ToF on the I2C board, 63° / 90°) —
  [VL53L5CX](vl53l5cx/introduction.md) ([guide](vl53l5cx/guide.md) ·
  [api](vl53l5cx/api.md)) · [VL53L7CX](vl53l7cx/introduction.md)
  ([guide](vl53l7cx/guide.md) · [api](vl53l7cx/api.md))
- **VL53L7CH** (VL53L7CX + CNH histograms) —
  [introduction](vl53l7ch/introduction.md) · [guide](vl53l7ch/guide.md) ·
  [api](vl53l7ch/api.md)
- **VL53L8CX** (8×8 ToF base) — [introduction](vl53l8cx/introduction.md) ·
  [guide](vl53l8cx/guide.md) · [api](vl53l8cx/api.md)
- **VL53L8CH** (ToF superset + CNH histograms) —
  [introduction](vl53l8ch/introduction.md) · [guide](vl53l8ch/guide.md) ·
  [api](vl53l8ch/api.md)
- **TMF8829** (dToF, 8×8 … 48×32 zones, up to 4 peaks) —
  [introduction](tmf8829/introduction.md) · [guide](tmf8829/guide.md) ·
  [api](tmf8829/api.md)
- **BNO055** (9-axis IMU, on-chip fusion) — [introduction](bno055/introduction.md) ·
  [guide](bno055/guide.md) · [api](bno055/api.md)
- **BNO085** (9-axis IMU) — [introduction](bno085/introduction.md) ·
  [guide](bno085/guide.md) · [api](bno085/api.md)
- **BNO086** (9-axis IMU) — [introduction](bno086/introduction.md) ·
  [guide](bno086/guide.md) · [api](bno086/api.md)

For the exhaustive symbol-by-symbol reference see [api.md](api.md), generated
from the source `///` doc-comments so it never drifts from the code.

## Contents

- [What it is](#what-it-is)
- [What it is not](#what-it-is-not)
- [Live devices](#live-devices)
- [Add the crate](#add-the-crate)
- [Getting started](#getting-started)
- [Mental model](#mental-model)
- [Transport: framing & CRC](#transport-framing--crc)
- [Discovery: USB ids & identity](#discovery-usb-ids--identity)
- [Per-sensor decode](#per-sensor-decode)
- [Firmware container (`.fwdepz`)](#firmware-container-fwdepz)
- [Datasets (record & replay)](#datasets-record--replay)
- [Testing & golden vectors](#testing--golden-vectors)

## What it is

Every DEPZ sensor is a USB CDC-ACM device speaking one shared framed protocol
(`A5 C3` header + CRC). This crate is the **contract-first, transport-agnostic
foundation**: the byte-exact codecs, framing/parsing, CRCs, USB-id table,
firmware-name identity, per-sensor frame/report decoders, the `.fwdepz`
firmware-container parser, and the `.depzdata` datasets — everything you
need to turn bytes on the wire into typed values, verified against the shared
golden vectors in `contracts/vectors/` and byte-for-byte identical to the
Python / TypeScript / Java / C / C++ reference SDKs.

The sensors fall into a few firmware philosophies:

- **SR04** — the device does the ranging; you decode `echo_time_us` → distance
  ([`protocol::sr04`](sr04/api.md)).
- **VL53L0X / L1CX / L1CB / L3CX / L4CX** — one I2C register-bridge firmware
  for the 1D family; the v2.00 codecs, the product table and the stateless
  decode of the streamed blocks are in [`vl53lx`](vl53l1cx/api.md), and ST's
  drivers (light ULDs, the L0X API, the histogram driver) **run on the host**
  under the live class `devices::Vl53lxDevice`.
- **VL53L4CD** — the device is a thin I2C register bridge; the ST ULD 2.2.3
  math (result block, range timing, tuning words) runs **on the host**
  ([`vl53l4`](vl53l4cd/api.md)).
- **VL53L5CX / VL53L7CX / VL53L7CH** — one I2C register-bridge firmware for
  three boards; frames have the VL53L8 layout and decode through `vl53l8` with
  `Variant::L7`; the board codecs and class resolution are in
  [`vl53l7`](vl53l7cx/api.md).
- **VL53L8CX / VL53L8CH** — the device is a thin SPI bridge; the ST ULD frame
  layout is decoded **on the host** ([`vl53l8`](vl53l8cx/api.md)). CX is the
  base ToF imager; CH is its superset, adding Compact-Network-Histogram output.
- **TMF8829** — the device is a thin SPI register bridge; the host runs the
  sensor (firmware download, presets, start/stop) and the frame decode, clock
  model and capture time are in [`tmf8829`](tmf8829/api.md), with the embedded
  MIT sensor-firmware image.
- **BNO055** — the device is an I2C register bridge; Bosch's fusion runs on the
  chip, and the register codecs and block decode are in
  [`bno055`](bno055/api.md).
- **BNO085 / BNO086** — the device is an SHTP pass-through; the SH-2 stack is
  decoded **on the host** ([`bno086`](bno086/api.md)).

Everything is pure computation over `&[u8]` — no threads, no alloc(beyond the
returned `Vec`s), no runtime dependencies.

## What it is not

The crate has **no serial-port dependency**: the live layer
([`devices`](#live-devices)) runs on any byte pipe you hand it — a port opened
with the crate of your choice, a socket, a `.depzrec` replay. It has no
dependency at all: JSON, gzip and base64 are its own.

The crate never fabricates a decode it cannot verify against a golden vector.

## Live devices

`depz_sensor_sdk::devices` is the contract 07 device layer: a [`DepzDevice`] owns
a [`Link`] and a reader thread, correlates replies to requests by echoed opcode
(one request in flight per opcode), raises events and runs the common commands
(name, serial, MCU temperature, time sync, payload CRC, sync pins). Each sensor
class wraps it (and derefs to it) — the **SR04**, the **VL53L4CD**, the **VL53L8CX / CH**, the **VL53L5CX /
L7CX / L7CH**, the **BNO055**, the **BNO085 / BNO086**, the **TMF8829** and the **VL53L 1D family**
(VL53L0X / L1CX / L1CB / L3CX / L4CX).

```rust
use std::time::Duration;
use depz_sensor_sdk::devices::{open_on_link, AnyDevice, ReplayLink};

// Any `Link` works: here a capture, replayed; on hardware, a serial port (below).
let link = ReplayLink::open("sr04.depzrec", true, false)?;
let AnyDevice::Sr04(sr04) = open_on_link(link, None, None, None)? else { panic!("not an SR04") };
println!("{}", sr04.get_device_name()?);
let loop_ = sr04.measurements(256);         // a bounded, drop-oldest pull queue
sr04.start()?;
let m = loop_.take(Duration::from_secs(1))?;
println!("{:?} mm", m.distance_mm());
sr04.stop()?;
# Ok::<(), depz_sensor_sdk::devices::DepzError>(())
```

`open_on_link` runs the contract 02 §4.4 probe (software name, device name,
serial), resolves the class and continues the same connection as it. A link is
four methods — `read` with a timeout, `write`, `close`, `is_closed` (plus a
`name`) — so a serial port is a few lines over the port crate you already use,
e.g. `serialport`:

```rust,ignore
struct Port(std::sync::Mutex<Box<dyn serialport::SerialPort>>, std::sync::atomic::AtomicBool);

impl depz_sensor_sdk::devices::Link for Port {
    fn read(&self, timeout: Duration) -> depz_sensor_sdk::devices::Result<Vec<u8>> {
        let mut p = self.0.lock().unwrap();
        p.set_timeout(timeout)?;
        let mut buf = [0u8; 4096];
        match p.read(&mut buf) {
            Ok(n) => Ok(buf[..n].to_vec()),
            Err(e) if e.kind() == std::io::ErrorKind::TimedOut => Ok(Vec::new()),
            Err(e) => Err(e.into()),
        }
    }
    fn write(&self, data: &[u8]) -> depz_sensor_sdk::devices::Result<()> {
        Ok(self.0.lock().unwrap().write_all(data)?)
    }
    fn close(&self) { self.1.store(true, std::sync::atomic::Ordering::SeqCst) }
    fn is_closed(&self) -> bool { self.1.load(std::sync::atomic::Ordering::SeqCst) }
    fn name(&self) -> String { self.0.lock().unwrap().name().unwrap_or_default() }
}
```

`RecordingLink` wraps any link and writes a `.depzrec` capture (contract 08);
`ReplayLink` plays one back causally, with `strict_tx` failing any byte the host
writes that the capture does not hold — how the contract 15 scenarios run.
Callbacks (`on_measurement`, `on_event`) run on the reader thread and return a
`Hook` that unregisters when dropped; `measurements(capacity)` /
`events(capacity)` are pull queues. Errors are one enum, `DepzError`
(`Timeout`, `Status`, `DeviceLost`, `InvalidArgument`, …).

## Add the crate

Published on crates.io as `depz-sensor-sdk` (0.7.0):

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
- **VL53L0X / L1CX / L1CB / L3CX / L4CX** — [`vl53lx`](vl53l1cx/api.md): the
  v2.00 stream / address-width / info codecs, the product table
  (`PRODUCTS`, `resolve_class`) and `decode_die_block` / `decode_l0x_raw` /
  `decode_histogram_raw`.
- **VL53L4CD** — [`vl53l4`](vl53l4cd/api.md): the register-bridge command
  packers and report unpackers (`RegData` / `Vl53l4Info` / `StreamData`) plus
  the pure ULD math — `parse_result_block`, range timing, tuning words.
- **VL53L5CX / VL53L7CX / VL53L7CH** — [`vl53l7`](vl53l7cx/api.md): the
  board's pin / bus-speed / info codecs and `resolve_model`; frames decode with
  `vl53l8::parse_frame(frame, Variant::L7)`, the VL53L7CH's CNH block with
  `vl53l8::decode_cnh`.
- **VL53L8CX / CH** — [`vl53l8`](vl53l8cx/api.md): `FrameReassembler` rebuilds a
  frame from `RPT_VL53_FRAME` chunks, `parse_frame` decodes it into per-zone
  arrays, `advanced` holds the DCI codecs (motion, xtalk, thresholds).
- **BNO055** — [`bno055`](bno055/api.md): the register-bridge codecs, `Units`,
  `CalibStatus`, `CalibrationProfile`, `AxisRemap` + placements, the page-1
  configs and `decode_block`.
- **TMF8829** — [`tmf8829`](tmf8829/api.md): the bridge codecs,
  `FrameReassembler` rebuilds frames from `RPT_TMF_FRAME` chunks,
  `parse_frame` / `parse_results` / `merge_halves` decode them,
  `SensorClock` + `CaptureTimer` give the capture time, `MeasurementBuilder`
  assembles measurements, `cfg_get` / `cfg_set` edit the config page, and
  `firmware_image()` is the embedded sensor firmware.
- **BNO085 / BNO086** — [`bno086`](bno086/api.md): `ShtpLayer` reassembles SHTP
  cargos, `parse_input_cargo` / `parse_gyro_rv_cargo` decode reports, `sh2`
  builds control requests.

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

[`dataset::Dataset`](api.md#datasets-record--replay) reads a `.depzdata`
document — UTF-8 JSON Lines, a header describing devices on a shared host
timeline followed by one decoded record per line (`.depzdata.gz`: the same,
gzipped) — using the crate's own dependency-free [`json`](api.md#json) parser:

```rust
use depz_sensor_sdk::dataset::{Dataset, RecordValue};

let ds = Dataset::read("session.depzdata")?;   // or Dataset::parse(&text)
for rec in &ds.records {                       // sorted by host time
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

Recording: [`SessionRecorder`](api.md#datasets-record--replay) takes open
devices (any class, or an `AnyDevice`), syncs their clocks and writes a record
per result; it never reconfigures a device — start and stop streaming yourself.

```rust
use depz_sensor_sdk::dataset::SessionRecorder;

let mut rec = SessionRecorder::new("session.depzdata.gz", Some("bench run"), false, Some(1.0));
rec.add(&tof, None, 5)?;   // "d0"; 5 time-sync rounds
rec.add(&imu, None, 5)?;   // "d1"
rec.start()?;
// … stream …
rec.stop()?;
```

A `.gz` file is written as a gzip container of stored (uncompressed) blocks —
every gzip reader opens it; the crate has no compression dependency.
`DatasetWriter` writes records of your own; `dataset::records` builds each
kind's value.

Playback: [`DatasetPlayer`](api.md#datasets-record--replay) delivers the
records paced by their times, with play / pause / seek / speed (contract 09
§Player) — on a player thread, or from your own timer through `tick()`.

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
