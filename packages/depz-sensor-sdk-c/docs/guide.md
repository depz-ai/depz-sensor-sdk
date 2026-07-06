# depz-sensor-sdk-c — guide

The common guide to the **C** SDK for the DEPZ USB sensor line. It covers what
the SDK is, how to build it with CMake, the transport/decode mental model, and
what is covered versus the hardware/CNH extension points. Each sensor then has
its own **introduction** and **user guide**:

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
from the header's doc-comments so it never drifts from the code.

## Contents

- [What it is](#what-it-is)
- [What it is not](#what-it-is-not)
- [Build & install](#build--install)
- [Mental model](#mental-model)
- [Transport: framing in, packets out](#transport-framing-in-packets-out)
- [Identity & discovery](#identity--discovery)
- [Common command & report codecs](#common-command--report-codecs)
- [Per-sensor decode](#per-sensor-decode)
- [Firmware container & bootloader](#firmware-container--bootloader)
- [Datasets](#datasets)
- [Extension points](#extension-points)
- [Testing](#testing)

## What it is

`depz-sensor-sdk-c` is a **contract-first, dependency-free C11 decode/codec
layer** for the DEPZ USB sensor line. Every DEPZ sensor is a USB CDC-ACM device
speaking one shared framed protocol (`A5 C3` header + optional payload CRC). This
library turns that byte stream into typed C values and packs the command/config
payloads that go the other way — byte-exact with the `contracts/` docs and the
golden vectors in `contracts/vectors/`.

It is the C counterpart of the Python SDK's protocol core. The entire public
surface is one header, [`include/depz_sensor_sdk.h`](../include/depz_sensor_sdk.h),
and the [API reference](api.md) is generated straight from its doc-comments.

Four sensors across three firmware philosophies — mirrored from the firmware:

- **SR04** — the device does the ranging; you decode `echo_time_us` → distance.
- **VL53L8CX / VL53L8CH** — the device is a thin SPI register bridge; the ranging
  frames are reassembled and decoded **on the host**. CX is the base ToF imager;
  CH is its superset, adding Compact-Network-Histogram (CNH) output.
- **BNO086** — the device is an SHTP pass-through; the SH-2 framing, control
  encoders and input-report parsers run **on the host**.

## What it is not

This package is a **decode / codec layer only**:

- It has **no serial-port I/O, no threads, and no device objects.** You own the
  transport (open the port, read/write bytes); the SDK is pure functions and
  small POD structs over buffers you supply.
- It does **not drive live hardware.** The hardware-dependent pieces — the live
  VL53L8 ULD init/config register sequences, and CNH histogram decode — are
  deliberately left as named [extension points](#extension-points) rather than
  faked.

Everything here is verifiable against golden vectors, so what ships is exactly
what the tests exercise.

## Build & install

C11, no dependencies beyond libm (`round()` in the dataset/advanced codecs).
Build the static library and run the vector suite with CMake:

```sh
cmake -B build -S .
cmake --build build
ctest --test-dir build          # 13/13: transport+protocol foundation + decode
```

To consume it from your own CMake project, add the package and link the
`depz_sensor_sdk` target — it exports its `include/` directory:

```cmake
add_subdirectory(path/to/depz-sensor-sdk-c)
target_link_libraries(my_app PRIVATE depz_sensor_sdk)
```

```c
#include <depz_sensor_sdk.h>
```

There is no `install` step wired up; vendor the source or `add_subdirectory` it.
The `docs/` reference is regenerated with `make docs` (see [below](#testing)).

## Mental model

There are no callbacks or streams — you drive a small state machine and call
decoders. The shape of every integration is the same:

```
your serial read()  ──►  depz_parser_feed(&parser, bytes, n, on_event, ctx)
                                   │  (emits one depz_event per complete frame)
                                   ▼
                         on_event(ev): switch (ev->cmd)
                                   │
              ┌────────────────────┼─────────────────────┐
        common report          SR04 data            VL53L8 chunk / BNO SHTP
    depz_unpack_status(…)   depz_sr04_unpack_data   depz_vl53l8_reasm_feed(…)
    depz_unpack_sync_time   (…)                      → depz_vl53l8_decode_frame
    …                                                depz_shtp_feed → depz_bno_*

your serial write()  ◄──  depz_build_packet(cmd, payload, …)
                                   ▲
                       depz_*_pack_*(…)  (encode the payload first)
```

- **One parser drains bytes.** `depz_parser_feed()` is fed whatever your serial
  read returns (any chunking) and invokes a callback once per complete frame,
  emitting `DEPZ_EV_PACKET`, `DEPZ_EV_TRASH`, or `DEPZ_EV_CRC_ERROR`. Event
  ordering is invariant to how the byte stream is chunked (contract 01 §5).
- **You correlate replies yourself.** There is no request/reply machinery: a
  solicited reply is just a `DEPZ_EV_PACKET` whose `cmd` echoes the command byte
  you sent. Match on `ev->cmd` (RPT ids are ≥ 0x80).
- **Decoders are pure.** Each `depz_*_unpack_*` / `depz_*_decode_*` takes the
  payload pointer + length and fills a caller-owned struct, returning 0 on
  success or negative on a length/format error. Encoders (`depz_*_pack_*`) write
  a payload into your buffer and return its length.
- **Timestamps are device microseconds.** The reports carry `timestamp_us` in the
  device clock; compute the host offset with `depz_sync_time_offset_rtt()` from a
  SYNC_TIME round trip.

## Transport: framing in, packets out

The framing layer (contract 01) is the foundation every sensor sits on:

```c
uint8_t frame[DEPZ_MAX_FRAME];
size_t n;
depz_build_packet(DEPZ_CMD_GET_SERIAL, NULL, 0, seq++, DEPZ_CRC_NONE,
                  frame, sizeof frame, &n);
serial_write(fd, frame, n);           // -> device

depz_parser p;
depz_parser_init(&p);
for (;;) {
    uint8_t buf[512];
    ssize_t got = serial_read(fd, buf, sizeof buf);
    depz_parser_feed(&p, buf, (size_t)got, on_event, &ctx);   // -> callbacks
}
depz_parser_free(&p);
```

The header sets the CRC-type bits even for an empty payload, but appends a CRC
trailer only for a non-empty payload (ERRATA E6) — `depz_build_packet` handles
that for you. The four CRCs (`depz_crc8_maxim`, `depz_crc16_modbus`,
`depz_crc32_iso_hdlc`, and the file-only `depz_crc16_ccitt_false`) are exposed
directly if you need them.

## Identity & discovery

The SDK does not enumerate USB ports (you own the transport), but it gives you
the identity primitives to select and classify a device:

- `depz_is_known_depz_usb(vid, pid)` — true for a recognized DEPZ (or
  dev-default) USB id; `depz_usb_model_hint(vid, pid)` returns a best-guess model
  name. Use these against the (vid, pid) your OS reports for a candidate port.
- `depz_parse_software_name()` classifies a `GET_NAME_ACTIVE_SOFTWARE` string
  (first stripped with `depz_strip_device_string`) into a `depz_identity`
  (`app`/`bootloader`/`unknown` mode + `depz_sensor_type`). Both VL53L8 variants
  report the same `VL53L8` firmware identity, so the CX/CH split is named
  separately by `depz_vl53l8_variant`.

## Common command & report codecs

Every sensor shares the command/report set in contract 02: device name, serial,
active-software name, MCU temperature, time-sync, payload-CRC mode, and the AUX
sync-pin config. The opcode/report/status enums (`depz_cmd`, `depz_rpt`,
`depz_status`) plus the pack/unpack helpers (`depz_pack_sync_time`,
`depz_unpack_temperature`, `depz_unpack_status`, …) live in the
[Common protocol](api.md#common-protocol) section. Time-sync math is
`depz_sync_time_offset_rtt()` (NTP-style offset + RTT, contract 02 §5).

## Per-sensor decode

Each sensor adds its own opcodes and decoders on top of the common set — follow
the per-sensor guide:

- **[SR04](sr04/guide.md)** — sample-period / echo-decay config codecs, the
  `depz_sr04_data` measurement, and `depz_sr04_distance_mm()` (echo → mm).
- **[VL53L8CX](vl53l8cx/guide.md)** — the register-bridge streaming path: chunk
  parse (`depz_vl53l8_unpack_chunk`), frame reassembly
  (`depz_vl53l8_reassembler`), the ranging-frame decoder
  (`depz_vl53l8_decode_frame`), and the advanced DCI codecs.
- **[VL53L8CH](vl53l8ch/guide.md)** — the CX superset; shares the entire CX
  decode surface (CNH decode is an extension point).
- **[BNO086](bno086/guide.md)** — SHTP framing (`depz_shtp_*`), SH-2 control
  encoders (`depz_bno_pack_*`), and the input-report parsers
  (`depz_bno_parse_input_cargo`, `depz_bno_parse_gyro_rv`).

## Firmware container & bootloader

`depz_fwdepz_parse()` parses and validates a `.fwdepz` application-firmware
container (magic → CRC-16/CCITT-FALSE header CRC → `fw_size` == payload), and the
bootloader opcode space (`depz_bl_cmd`) plus its page-write/read and flash-info
codecs are provided (contract 06). Driving an actual flash flow (open the
bootloader port, erase, write pages, verify) is left to the transport you own.

## Datasets

`depz_dataset_parse()` reads a `.depzdata` recording (JSONL: a header plus
time-merged records, contract 09) into an in-memory model you walk with
`depz_dataset_get_device()` / `depz_dataset_get_record()` and read scalars from
with `depz_dataset_value_int` / `depz_dataset_value_str`. Records are exposed in
stable merge order (by `t_host_us`, then file order).

## Extension points

Two pieces are intentionally **not** implemented, and are documented as gaps
rather than faked:

- **Live VL53L8 ULD init/config** — the firmware-download + DCI register-write
  sequences that bring the ToF sensor up are hardware-dependent. The SDK ships
  the pure *codec* halves (e.g. `depz_vl53l8_motion_cfg_default_pack`,
  `depz_vl53l8_pack_thresholds`) and the frame decode, but not the live driver.
  See the TS/Python `uld` reference for the register sequences.
- **CNH (compact-network-histogram) decode** — the CH-only histogram frame is a
  distinct on-wire layout and is left as a named gap in `src/vl53l8_decode.c`.
  The shared ranging-frame decode (`depz_vl53l8_decode_frame`) serves both CX and
  CH; CNH is the one CH-exclusive feature not yet decoded.

## Testing

The suite is one CTest per consumed vector / replay target — the
transport+protocol foundation plus the sensor-decode layer across the four
sensors (13 tests). A real VL53L8CX capture is replayed end-to-end through the
shared decode path (`vec_vl53l8_replay`). No hardware required:

```sh
ctest --test-dir build            # 13/13
```

Regenerate this documentation's API reference after changing the header's
doc-comments:

```sh
make docs                         # == python3 scripts/gen_api_md.py
```
