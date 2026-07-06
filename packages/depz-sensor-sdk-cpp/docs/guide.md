# depz-sensor-sdk-cpp — guide

The common guide to the C++ SDK for the DEPZ USB sensor line. It covers what
the SDK is, how to build it, the shared mental model, and the cross-sensor
codecs (framing, CRC, identity, USB discovery ordering, time-sync math,
firmware containers, dataset replay). Each sensor then has its own
**introduction** and **user guide**:

- **SR04** (HC-SR04 ultrasonic) — [introduction](sr04/introduction.md) ·
  [guide](sr04/guide.md) · [api](sr04/api.md)
- **VL53L8CX** (8×8 ToF base) — [introduction](vl53l8cx/introduction.md) ·
  [guide](vl53l8cx/guide.md) · [api](vl53l8cx/api.md)
- **VL53L8CH** (ToF superset + CNH histograms) —
  [introduction](vl53l8ch/introduction.md) · [guide](vl53l8ch/guide.md) ·
  [api](vl53l8ch/api.md)
- **BNO086** (9-axis IMU) — [introduction](bno086/introduction.md) ·
  [guide](bno086/guide.md) · [api](bno086/api.md)

For the exhaustive symbol-by-symbol reference see [api.md](api.md), generated
from the header doc-comments so it never drifts from the code.

## Contents

- [What it is](#what-it-is)
- [Build & link](#build--link)
- [Byte types: `bytes`, `byte_span`, `span<T>`](#byte-types-bytes-byte_span-spant)
- [Mental model](#mental-model)
- [Transport: framing & CRC](#transport-framing--crc)
- [Common commands & reports](#common-commands--reports)
- [Time-sync math](#time-sync-math)
- [Discovery: USB identity](#discovery-usb-identity)
- [Firmware containers](#firmware-containers)
- [Datasets: record & replay](#datasets-record--replay)
- [What is not here (extension points)](#what-is-not-here-extension-points)
- [Testing without hardware](#testing-without-hardware)

## What it is

This is the **decode layer** — pure codecs, **no I/O and no transport**. You
own the serial port (or a recording, or a socket); the SDK turns bytes into
typed values and typed values back into bytes. Each of the four sensors speaks
one shared framed protocol (`A5 C3` header + optional payload CRC), and this
library gives you:

- a byte-exact frame parser and packet builder (`depz/framing.hpp`,
  `depz/crc.hpp`), and
- one header of wire codecs per sensor — `depz/sr04.hpp`, `depz/vl53l8.hpp`
  (CX **and** CH), `depz/bno086.hpp` — plus the cross-sensor pieces
  (`depz/common.hpp`, `depz/identity.hpp`, `depz/usb_ids.hpp`,
  `depz/fwdepz.hpp`, `depz/dataset.hpp`).

Everything lives in namespace `depz` (ToF under `depz::vl53l8`, IMU under
`depz::bno086`, datasets under `depz::dataset`). It targets **C++17** and is
byte-for-byte identical to the Python / TypeScript / Java reference SDKs via the
shared golden vectors in `contracts/vectors`.

The three firmware philosophies mirror the firmware itself:

- **SR04** — the device does the ranging; you decode `echo_time_us` → distance.
- **VL53L8CX / VL53L8CH** — the device is a thin SPI bridge; the ST results
  frame is reassembled and decoded on the host (`depz::vl53l8`). CX is the base
  ToF imager; CH is its superset, adding Compact-Network-Histogram output.
- **BNO086** — the device is an SHTP pass-through; the SH-2 framing and report
  decode run on the host (`depz::bno086`).

## Build & link

```bash
cmake -B build -S .
cmake --build build
ctest --test-dir build          # 6 golden-vector suites
```

The build produces the static library `depz_sensor_sdk` and the CMake alias
`depz::sensor_sdk`. In your own tree:

```cmake
add_subdirectory(depz-sensor-sdk-cpp)
target_link_libraries(my_app PRIVATE depz::sensor_sdk)
```

The public headers are under `include/depz/`; include what you use
(`#include "depz/sr04.hpp"`).

To regenerate the API reference after editing header doc-comments:

```bash
cmake --build build --target docs      # or: python3 scripts/gen_api_md.py
```

## Byte types: `bytes`, `byte_span`, `span<T>`

The SDK avoids allocating on the read path: **inputs are non-owning views,
outputs are owned buffers** (`depz/span.hpp`).

- `depz::bytes` — `std::vector<std::byte>`, the owned buffer every `pack`/
  `build` returns.
- `depz::byte_span` — `depz::span<const std::byte>`, a non-owning `(ptr, len)`
  view; every `unpack`/`decode`/`feed` takes one. It implicitly converts from a
  `std::vector<std::byte>` or `std::array<std::byte, N>`.
- `depz::as_bytes(vec)` — wrap a `std::vector<std::byte>` as a `byte_span`.
- `depz::span<T>` is a tiny C++17 stand-in for `std::span` (this library
  predates C++20); you rarely name it directly.

```cpp
#include "depz/span.hpp"

depz::bytes buf = /* bytes you read off the wire */;
depz::byte_span view = depz::as_bytes(buf);   // no copy
```

## Mental model

```
  serial port / recording / socket   (you own this)
            │   read bytes            │   write bytes
            ▼                         ▲
     PacketParser::feed()       build_packet()
            │                         │
      ParserEvent variant       framed A5C3 packet
   (Packet | Trash | CrcError)
            │
   std::visit / std::get_if
            │  Packet{cmd,seq,payload}
            ▼
   per-sensor payload codec  ──►  typed value
   (Sr04Data::unpack, decode_frame, parse_input_cargo, …)
```

- **You drive the loop.** There is no reader thread and no callback registry;
  you read bytes on your own schedule and feed them to a `PacketParser`.
- **Correlation is yours to do.** Solicited replies echo the request's command
  byte in `Packet::cmd` (unsolicited reports use `cmd == 0x00`,
  `depz::UNSOLICITED`); match replies to requests on that byte, exactly as the
  reference SDKs do.
- **Raw integers are authoritative.** Codecs return the wire integers; unit
  conversions (0.1 °C, µs → mm, Q-point floats) are thin helpers on top.

## Transport: framing & CRC

`PacketParser` is incremental: feed arbitrary byte chunks, get an ordered
`std::vector<ParserEvent>`. Event order is invariant to how the stream is
chunked (only `Trash` byte-run boundaries depend on chunking).

```cpp
#include "depz/framing.hpp"
#include <variant>

depz::PacketParser parser;
for (const auto& ev : parser.feed(depz::as_bytes(rx))) {
    if (auto* p = std::get_if<depz::Packet>(&ev)) {
        // p->cmd, p->seq, p->payload   (payload is CRC-stripped)
    } else if (std::get_if<depz::CrcError>(&ev)) {
        // a well-framed packet whose payload CRC failed (dropped)
    } else if (auto* t = std::get_if<depz::Trash>(&ev)) {
        // bytes discarded while hunting for a frame
    }
}
// running counters: parser.packets / crc_errors / header_errors / trash_bytes
```

Build a packet to send (per firmware TX, the header advertises the CRC type
even for an empty payload, but ERRATA E6 means empty payloads carry no CRC
bytes):

```cpp
depz::bytes frame = depz::build_packet(
    static_cast<std::uint8_t>(depz::Cmd::SyncTime),
    depz::as_bytes(payload), /*seq=*/0, depz::CrcType::Crc16);
```

The four wire CRCs (`depz/crc.hpp`) are standalone reflected-table functions —
`crc8_maxim`, `crc16_modbus`, `crc32_iso_hdlc`, and `crc16_ccitt_false` (used
only for the `.fwdepz` header, never on the wire).

## Common commands & reports

`depz/common.hpp` holds the shared command/report IDs (`Cmd`, `Rpt`, `Status`)
and the payload codecs every sensor uses: `StatusReport`, `TextReport`,
`SyncTimeReport`, `TemperatureReport`, `SequenceErrorReport`, and the AUX
`SyncPinConfig`. Each is a plain struct with a `static … unpack(byte_span)` (and
`pack()` where the host sends it):

```cpp
auto st = depz::StatusReport::unpack(pkt.payload);
if (st.cmd == static_cast<std::uint8_t>(depz::Cmd::SetSyncPinConfig) &&
    st.status == static_cast<std::uint8_t>(depz::Status::Ok)) { /* … */ }

auto temp = depz::TemperatureReport::unpack(pkt.payload);
double c = temp.celsius();     // raw 0.1 °C → °C
```

## Time-sync math

Multi-device correlation is built on the same NTP-style clock math the firmware
uses. `sync_time_offset_rtt(t1, t2, t3, t4)` returns `{offset_us, rtt_us}` where
`offset = device_clock − host_clock`; drive it from a `SyncTimeReport`:

```cpp
auto r = depz::SyncTimeReport::unpack(pkt.payload);   // T1(echoed), T2, T3
std::int64_t t4 = host_now_us();                      // your host clock
auto [offset_us, rtt_us] = depz::sync_time_offset_rtt(
    r.pc_timestamp_us, r.mcu_rx_us, r.mcu_tx_us, t4);
// host_us = device_timestamp_us − offset_us
```

Keep the lowest-RTT sample across a few rounds (contract 02 §5). Applying the
same offset per device is what lets several sensors — including two of the same
model — share one host timeline.

## Discovery: USB identity

Selection is by **USB identity**, never "first port the OS enumerated"
(`depz/usb_ids.hpp`). Filter candidate ports to known DEPZ (vid, pid) pairs,
then order them deterministically by USB iSerial:

```cpp
#include "depz/usb_ids.hpp"

if (depz::is_known_depz_usb(vid, pid)) { /* a DEPZ (or dev-default) unit */ }
auto hint = depz::usb_model_hint(vid, pid);     // std::optional<std::string>

std::vector<depz::PortInfo> ordered =
    depz::order_by_serial(std::move(enumerated));  // ascending; empty serial last
```

The PID→model map is an informational **hint**; the protocol probe
(`GET_NAME_ACTIVE_SOFTWARE`, decoded by `depz::parse_software_name` in
`depz/identity.hpp`) is the source of truth for what a device actually is.

```cpp
std::string name = depz::strip_device_string(pkt.payload);   // drop NUL/0xFF filler
depz::Identity id = depz::parse_software_name(name);
// id.mode (App/Bootloader), id.sensor_type (Sr04/Vl53l8/Bno086), id.version
```

## Firmware containers

`depz/fwdepz.hpp` parses and validates the `.fwdepz` application-firmware
container and holds the resident-bootloader wire codecs (`BlCmd`, `BlRpt`,
`FlashInfo`, `pack_write_page`, …):

```cpp
depz::FwDepzImage img = depz::FwDepzImage::parse(depz::as_bytes(blob));
// throws depz::FwDepzError on a bad length / magic / header CRC / size
bool ok = img.payload_crc_ok();      // CRC-32 over the payload
// img.load_addr, img.fw_size, img.cur_sec / tot_sec, img.payload
```

Driving the actual erase/write/verify handshake is host-application work; this
SDK supplies the codecs it is built from.

## Datasets: record & replay

`depz::dataset::Reader` reads a decoded, multi-device, time-synced
`.depzdata` file (JSON-lines: a header line + one record per line). Records come
back **merged by host time** across every device, including two of the same
model:

```cpp
#include "depz/dataset.hpp"

depz::dataset::Reader ds(file_contents);
for (const auto& rec : ds.records) {         // stable-sorted by t_host_us
    // rec.device_id, rec.t_host_us, rec.kind
    // rec.ints / rec.strings / rec.arrays   (value split by JSON type)
}
// ds.schema, ds.devices (per-device DeviceMeta incl. TimeSync), ds.duration_us()
```

Writing datasets and the raw-byte recording layer are host-side concerns; the
SDK provides the reader used for replay and regression.

## What is not here (extension points)

Truthfully scoped — two things are hardware-dependent or CH-specific and
intentionally **not** implemented, rather than faked:

- **CNH histogram decode** — the VL53L8CH-only compact-network-histogram
  stream. The shared results-frame path already serves CH ranging; only the CNH
  codec is a documented stub in `depz/vl53l8.hpp`.
- **Live ULD init / register-bridge driver** — the firmware download and DCI
  read/write handshakes that bring a ToF part up on real hardware. See the
  reference Python `vl53l8/uld.py`.

## Testing without hardware

Because every entry point takes bytes, the whole SDK is exercised off recorded
captures — no serial port required. That is exactly how the golden-vector suite
runs (`ctest --test-dir build`), including a real VL53L8 capture replayed
through `PacketParser` → `FrameReassembler` → `decode_frame`.
