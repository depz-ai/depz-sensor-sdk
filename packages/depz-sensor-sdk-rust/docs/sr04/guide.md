# SR04 — user guide

Hands-on guide to the SR04 codecs in [`protocol::sr04`](api.md). For what the
sensor is and its concepts, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md). Framing, CRC and discovery are
shared across sensors — see the [common guide](../guide.md).

## Contents

- [The command / report set](#the-command--report-set)
- [Hello-world: decode a distance](#hello-world-decode-a-distance)
- [Framing configuration commands](#framing-configuration-commands)
- [Single shot vs the loop](#single-shot-vs-the-loop)
- [Distance and temperature compensation](#distance-and-temperature-compensation)
- [Gotchas](#gotchas)

## The command / report set

Host→device opcodes are [`Sr04Cmd`](api.md); device→host report opcodes are
[`Sr04Rpt`](api.md). You frame a command with the shared
[`build_packet`](../guide.md#transport-framing--crc) and match replies by the
echoed command byte:

```rust
use depz_sensor_sdk::framing::{build_packet, CrcType};
use depz_sensor_sdk::protocol::sr04::Sr04Cmd;

let tx = build_packet(Sr04Cmd::StartMeasurementLoop as u8, &[], seq, CrcType::None).unwrap();
// ... write `tx` to your transport ...
```

## Hello-world: decode a distance

Feed inbound bytes to a `PacketParser`, then decode any `RPT_DATA`
(`Sr04Rpt::Data`, `0x91`) payload into an [`Sr04Data`](api.md):

```rust
use depz_sensor_sdk::framing::{Event, PacketParser};
use depz_sensor_sdk::protocol::sr04::{Sr04Data, Sr04Rpt, distance_mm_from_echo};

let mut parser = PacketParser::new();
for ev in parser.feed(&rx_bytes) {
    if let Event::Packet(pkt) = ev {
        if pkt.cmd == Sr04Rpt::Data as u8 {
            let d = Sr04Data::unpack(&pkt.payload).unwrap();
            match distance_mm_from_echo(d.echo_time_us, None) {
                Some(mm) => println!("{:7.1} mm  (src 0x{:02x})", mm, d.source_cmd),
                None => println!("no echo"),
            }
        }
    }
}
```

`Sr04Data` carries the raw wire fields: `source_cmd` (`0x36` shot / `0x37`
loop), `timestamp_us` (device µs) and `echo_time_us`.

## Framing configuration commands

Configuration is get/set pairs; values are microseconds, encoded little-endian
by the pack helpers and decoded by the unpack helpers:

```rust
use depz_sensor_sdk::protocol::sr04::{
    Sr04Cmd, pack_sample_period, unpack_sample_period,
    pack_echo_decay, unpack_echo_decay,
};
use depz_sensor_sdk::framing::{build_packet, CrcType};

// Set the sample period to 20 000 µs (50 Hz ceiling).
let tx = build_packet(
    Sr04Cmd::SetSamplePeriod as u8,
    &pack_sample_period(20_000),
    seq,
    CrcType::None,
).unwrap();

// Decode the RPT_SAMPLE_PERIOD reply payload:
let stored_us = unpack_sample_period(&reply_payload).unwrap();   // the stored value
```

Echo decay works the same way with `pack_echo_decay` / `unpack_echo_decay`
(a `u16`). The device clamps decay to [`ECHO_DECAY_MIN_US`](api.md)
–[`ECHO_DECAY_MAX_US`](api.md), so read the value back to see what took effect.

## Single shot vs the loop

- **Single shot** — frame `Sr04Cmd::MeasureOnce`; the `RPT_DATA` reply arrives
  only when the echo completes (or times out at ~65.5 ms). The device returns
  `ERR_BUSY` (see [`Status`](../api.md#protocol-codecs)) if the loop is running.
- **Loop** — frame `StartMeasurementLoop`; samples stream as `RPT_DATA` until
  you frame `StopMeasurementLoop`.

Both paths deliver the same `Sr04Data`; tell them apart by `source_cmd`. An AUX
SYNC_IN edge also produces a `0x36` single shot on the stream.

## Distance and temperature compensation

The raw `echo_time_us` is authoritative; distance is derived at your chosen
speed of sound:

```rust
use depz_sensor_sdk::protocol::sr04::distance_mm_from_echo;

distance_mm_from_echo(echo_us, None);        // at 343 m/s
distance_mm_from_echo(echo_us, Some(30.0));  // c = 331.3 + 0.606·T ≈ 349.5 m/s at 30 °C
```

Both return `Option<f64>` — `None` for the no-echo timeout sentinel.

## Gotchas

- **Always check for the timeout.** A no-echo result is `echo_time_us == 0xFFFF`
  ([`ECHO_TIMEOUT`](api.md)); `Sr04Data::is_timeout()` is `true` and
  `distance_mm_from_echo` returns `None`.
- **`unpack` is length-checked.** `Sr04Data::unpack` needs exactly 11 bytes and
  returns [`CodecError`](../api.md#protocol-codecs) otherwise; the period/decay
  unpackers need 4 / 2 bytes.
- **Configured period is a ceiling, not the realised rate** — the echo window
  throttles it (ERRATA E3); the read-back returns the stored value.
- **Echo decay is a `u16`** — values above 65535 can't be encoded, and the
  device clamps to 4000–65000 µs.
- **This crate does not own a port** — you supply the bytes and write the framed
  commands; see the [common guide](../guide.md#getting-started).
