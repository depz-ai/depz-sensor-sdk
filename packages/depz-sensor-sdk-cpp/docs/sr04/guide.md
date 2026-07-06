# SR04 — user guide

Hands-on guide to the SR04 codecs in `depz/sr04.hpp`. For what the sensor is and
its concepts, read the [introduction](introduction.md); for exact signatures see
the [API reference](api.md). This SDK is the **decode layer** — it turns bytes
into values and back; you own the serial port and the request/reply loop (see
the [common guide](../guide.md)).

## Contents

- [Decode a measurement](#decode-a-measurement)
- [Echo time → distance](#echo-time--distance)
- [Building the commands](#building-the-commands)
- [Configuration codecs](#configuration-codecs)
- [Single shot vs the loop](#single-shot-vs-the-loop)
- [Gotchas](#gotchas)

## Decode a measurement

The device sends `RPT_SR04_DATA` (`Sr04Rpt::Data`, id `0x91`). Pull the packet
out of the `PacketParser` stream, then decode its payload:

```cpp
#include "depz/framing.hpp"
#include "depz/sr04.hpp"
#include <variant>

depz::PacketParser parser;
for (const auto& ev : parser.feed(depz::as_bytes(rx))) {
    auto* pkt = std::get_if<depz::Packet>(&ev);
    if (!pkt || pkt->cmd != static_cast<std::uint8_t>(depz::Sr04Rpt::Data)) continue;

    depz::Sr04Data m = depz::Sr04Data::unpack(depz::as_bytes(pkt->payload));
    bool from_loop = (m.source_cmd == 0x37);   // else 0x36 = one-shot / SYNC_IN
    auto dist = depz::distance_mm_from_echo(m.echo_time_us);
    if (dist) std::printf("%.1f mm  (t=%llu us)\n", *dist,
                          (unsigned long long)m.timestamp_us);
    else      std::puts("no echo");
}
```

`Sr04Data` carries the raw wire fields only: `source_cmd`, `timestamp_us` (device
µs), and `echo_time_us`.

## Echo time → distance

`distance_mm_from_echo()` is the one conversion helper. It returns
`std::optional<double>` — `std::nullopt` for the no-echo sentinel — and takes an
optional air temperature (°C):

```cpp
auto d0 = depz::distance_mm_from_echo(m.echo_time_us);          // 343 m/s
auto dT = depz::distance_mm_from_echo(m.echo_time_us, 30.0);    // c = 331.3 + 0.606·T

if (m.echo_time_us == depz::ECHO_TIMEOUT) { /* == !d0.has_value() */ }
```

Both forms return an empty optional for the timeout, so a single `if (d)` guard
covers the no-echo case.

## Building the commands

To *request* the measurements, frame the SR04 command bytes with `build_packet`
(from the [common guide](../guide.md#transport-framing--crc)) and write them to
your port:

```cpp
using depz::Sr04Cmd;
auto start = depz::build_packet(static_cast<std::uint8_t>(Sr04Cmd::StartMeasurementLoop));
auto stop  = depz::build_packet(static_cast<std::uint8_t>(Sr04Cmd::StopMeasurementLoop));
auto once  = depz::build_packet(static_cast<std::uint8_t>(Sr04Cmd::MeasureOnce));
```

The device replies to each with a `StatusReport` (echoing the command byte) and,
for the measurement commands, an `Sr04Rpt::Data` report.

## Configuration codecs

Sample period and echo decay are plain get/set pairs; the SDK gives you the
payload codecs (values are microseconds):

```cpp
// set: pack the value into a command payload
auto p = depz::build_packet(static_cast<std::uint8_t>(depz::Sr04Cmd::SetSamplePeriod),
                            depz::as_bytes(depz::pack_sample_period(20'000)));   // 50 Hz ceiling
auto e = depz::build_packet(static_cast<std::uint8_t>(depz::Sr04Cmd::SetEchoDecay),
                            depz::as_bytes(depz::pack_echo_decay(5'000)));

// get: decode the report payload the device sends back
std::uint32_t period_us = depz::unpack_sample_period(reply_payload);   // Sr04Rpt::SamplePeriod
std::uint16_t decay_us  = depz::unpack_echo_decay(reply_payload);      // Sr04Rpt::EchoDecay
```

`pack_echo_decay` values outside `ECHO_DECAY_MIN_US`..`ECHO_DECAY_MAX_US` are
clamped **by the device**, so the read-back (`unpack_echo_decay`) is the value
actually in effect.

## Single shot vs the loop

Both paths deliver an `Sr04Rpt::Data` report; tell them apart by `source_cmd`:

- **One-shot** (`MeasureOnce`, `0x36`) — the reply arrives only when the echo
  completes (or times out at ~65.5 ms). The device returns `ERR_BUSY`
  (`Status::ErrBusy`) if you request it while the loop is running.
- **Loop** (`StartMeasurementLoop`, `0x37`) — samples stream at the configured
  period until `StopMeasurementLoop`.
- **SYNC_IN** — an unsolicited single shot triggered by an AUX edge; it also
  carries `source_cmd == 0x36`.

## Gotchas

- **Always check the optional.** A no-echo timeout is `echo_time_us == 0xFFFF`
  (`depz::ECHO_TIMEOUT`) and `distance_mm_from_echo()` returns `std::nullopt`.
- **`MeasureOnce` during the loop returns `ERR_BUSY`** — one in-flight
  measurement path at a time; stop the loop first.
- **Configured period is a ceiling, not the realised rate** — the echo window
  throttles it; a read-back returns the stored value.
- **Echo decay is clamped on the device** to 4000–65000 µs; trust the read-back.
  Values above 65535 can't be sent (the field is a `std::uint16_t`).
- **Raw integers are authoritative** — `echo_time_us` is the source of truth;
  distance is derived, and temperature compensation is opt-in.
