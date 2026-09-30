# SR04 — user guide

Hands-on guide to the SR04 in C++: the live class `depz::Sr04`
(`depz/device.hpp`) first, then the codecs of `depz/sr04.hpp` underneath it.
For what the sensor is and its concepts, read the
[introduction](introduction.md); for exact signatures see the
[API reference](api.md). The rules every live device shares — the build and
the C SDK dependency, errors, threads and callbacks — are in the
[common guide](../guide.md#live-hardware).

## Contents

- [Open the sensor](#open-the-sensor)
- [Configure](#configure)
- [One measurement](#one-measurement)
- [The measurement loop](#the-measurement-loop)
- [Callbacks vs streams](#callbacks-vs-streams)
- [Device time → host time](#device-time--host-time)
- [Disconnects](#disconnects)
- [Record and replay in tests](#record-and-replay-in-tests)
- [Decode layer](#decode-layer)
- [Gotchas](#gotchas)

## Open the sensor

`open_sr04()` finds the DEPZ boards by USB id, orders them by USB serial,
probes the chosen one and returns a `std::unique_ptr<depz::Sr04>` — or throws
`NoDeviceError` (nothing found) or `WrongTypeError` (the board is not an SR04):

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main() {
    try {
        auto sr04 = depz::open_sr04();                    // the first SR04 by USB serial
        std::printf("%s on %s, MCU %.1f °C\n", sr04->software_name().c_str(),
                    sr04->port().c_str(), sr04->read_mcu_temperature());
    } catch (const depz::NoDeviceError&) {
        std::puts("no DEPZ device plugged in");
    } catch (const depz::WrongTypeError& e) {
        std::printf("%s\n", e.what());
    }
}   // the unique_ptr closes the port
```

Pick a board with `OpenOptions` — a port, a USB serial or an index:

```cpp
#include "depz/device.hpp"

std::unique_ptr<depz::Sr04> open_on(const std::string& port) {
    depz::OpenOptions opt;
    opt.port = port;                                  // "/dev/ttyACM0", "COM7"
    opt.timeout = std::chrono::milliseconds(500);     // request timeout (default 200 ms)
    return depz::open_sr04(opt);
}
```

Opened with `open_device()` instead, an SR04 is the `depz::Sr04` subclass of
the returned `depz::Device` — `dynamic_cast<depz::Sr04*>(dev.get())` finds it.
`depz::Sr04::open(Link)` opens an SR04 on a link with no probe at all, for
replays and fakes (see [below](#record-and-replay-in-tests)).

`Sr04` is a `Device`, so the common commands work on it too: `device_name()`,
`software_name()`, `serial_number()`, `read_mcu_temperature()`, `sync_time()`,
`reset()`, the events stream.

## Configure

Two settings, both microseconds:

```cpp
#include <cstdio>
#include "depz/device.hpp"

void configure(depz::Sr04& sr04) {
    sr04.set_sample_period_us(20'000);                     // at most 50 starts/s
    std::printf("period %u us\n", static_cast<unsigned>(sr04.sample_period_us()));

    std::uint16_t decay = sr04.set_echo_decay_us(1'000);   // below the 4000 minimum...
    std::printf("decay %u us\n", static_cast<unsigned>(decay));   // ...so 4000 is in effect
}
```

- **Sample period** — the minimum interval between measurement starts
  (default 50000 µs). The echo window throttles it further, so the read-back
  is the stored value, not the realised rate.
- **Echo decay** — the settle pause after an echo. The device clamps it to
  4000..65000 µs; `set_echo_decay_us()` reads the value back and returns what
  is in effect. Above 65535 it throws `ArgumentError` without sending (the
  wire field is 16 bits).

## One measurement

```cpp
#include <cstdio>
#include "depz/device.hpp"

void once(depz::Sr04& sr04) {
    depz::Sr04Measurement m = sr04.measure_once();     // waits up to 1 s
    if (!m.valid()) {                                   // echo_time_us == ECHO_TIMEOUT
        std::puts("no echo");
        return;
    }
    std::printf("%.0f mm at 343 m/s, %.0f mm at 30 °C (echo %u us, t=%llu us)\n",
                *m.distance_mm(), *m.distance_mm_at(30.0),
                static_cast<unsigned>(m.echo_time_us),
                static_cast<unsigned long long>(m.timestamp_us));
}
```

The reply arrives when the echo completes, or when the device gives up
(~65.5 ms); the `timeout` argument (default 1000 ms) bounds the wait, then
`TimeoutError`. **No echo is not an error**: it is a measurement with
`echo_time_us == depz::ECHO_TIMEOUT`, `valid() == false`, and both
`distance_mm()` and `distance_mm_at()` return `std::nullopt` — check before
dereferencing. The result of `measure_once()` is returned to the caller only;
it does not go to callbacks or streams.

On a real SR04 facing a wall about a metre away this prints ~985 mm.

## The measurement loop

`start()` runs the device's loop at the configured period; every sample goes
to the `on_measurement()` callbacks and the `stream()` queues. `start()` is
idempotent; `stop()` ends the loop. While it runs, `measure_once()` throws
`BusyError` — one measurement path at a time:

```cpp
#include <cstdio>
#include "depz/device.hpp"

void loop_for_a_second(depz::Sr04& sr04) {
    sr04.set_sample_period_us(20'000);
    auto samples = sr04.stream();             // subscribe before start(): nothing is missed
    sr04.start();
    try {
        sr04.measure_once();
    } catch (const depz::BusyError&) {
        std::puts("single shot refused while the loop runs");
    }
    int n = 0, echoes = 0;
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < until) {
        auto m = samples.next(std::chrono::milliseconds(200));
        if (!m) continue;                     // timeout (or the device closed)
        n++;
        if (m->distance_mm()) echoes++;
    }
    sr04.stop();
    std::printf("%d samples, %d with an echo, %llu dropped\n", n, echoes,
                static_cast<unsigned long long>(samples.dropped_count()));
}
```

At a 20 ms period and a near target a real SR04 delivers 50 samples/s. A far
target or no echo stretches each window and the rate drops.

## Callbacks vs streams

Both receive the same samples — loop samples (`from_loop == true`) and
SYNC_IN single shots (`from_loop == false`). Use either, or both at once.

- **`stream(maxsize)`** — a bounded queue (default 256) that your thread
  pulls with `next(timeout)`. When it is full the **oldest** sample goes and
  `dropped_count()` grows. It is the easy choice: your code runs on your
  thread, where it may block, log or call the device.
- **`on_measurement(fn)`** — `fn` runs **on the device's reader thread** for
  every sample. Keep it short: never block, never call a method that waits
  for a reply (`measure_once()`, `sample_period_us()`, …), never close or
  destroy the device from it. An exception thrown from it is swallowed. It
  returns an `Unsubscribe` function; call it while the device is open.

```cpp
#include <atomic>
#include <cstdio>
#include <memory>
#include <thread>
#include "depz/device.hpp"

void count_with_a_callback(depz::Sr04& sr04) {
    // Shared, so the callback's copy stays valid however late its last call runs.
    auto seen = std::make_shared<std::atomic<int>>(0);
    auto unsubscribe = sr04.on_measurement([seen](const depz::Sr04Measurement& m) {
        if (m.from_loop) (*seen)++;           // quick, non-blocking
    });
    sr04.start();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    sr04.stop();
    unsubscribe();
    std::printf("%d samples\n", seen->load());
}
```

A callback may still be running right after `unsubscribe()` returns, and the
SDK keeps the callback object itself until the device is destroyed — so
capture by value (or a `shared_ptr`) rather than a reference to a local.

## Device time → host time

`timestamp_us` is the device's clock. To line samples up with other devices
or host events, sync once and convert:

```cpp
#include <cstdint>
#include "depz/device.hpp"

std::int64_t host_time_of(depz::Sr04& sr04, const depz::Sr04Measurement& m) {
    if (!sr04.time_sync()) sr04.sync_time();         // 5 round trips, lowest RTT kept
    return sr04.to_host_time_us(m.timestamp_us);     // on depz::host_now_us()'s clock
}
```

## Disconnects

When the board is unplugged the reader thread notices, emits a
`DeviceEvent::Type::Disconnected` event (its `text` is the reason), and every
stream hands out what it still holds, then returns `std::nullopt` with
`closed() == true`. From then on `closed()` is `true` on the device and every
call throws `DeviceLostError`:

```cpp
#include <cstdio>
#include "depz/device.hpp"

void run_until_unplugged(depz::Sr04& sr04) {
    auto samples = sr04.stream();
    sr04.start();
    while (!samples.closed()) {
        if (auto m = samples.next(std::chrono::milliseconds(500)))
            std::printf("%.0f\n", m->distance_mm().value_or(-1.0));
    }
    std::puts("device lost");
    try {
        sr04.stop();
    } catch (const depz::DeviceLostError&) {
        // expected: the board is gone
    }
}   // destroy the Sr04 as usual; open_sr04() again to reconnect
```

There is no automatic reconnect: open the board again when it is back.

## Record and replay in tests

Record a live session once into a `.depzrec` capture, then replay it — in CI,
with no SR04 attached. Replay is causal (each recorded reply is served after
the request that preceded it), so the test must make the same calls in the
same order; with `strict_tx` any byte your code sends differently throws
`ReplayMismatchError`.

```cpp
#include "depz/device.hpp"

void record(const std::string& port) {
    auto link = depz::Link::recording(depz::Link::serial(port), "sr04.depzrec",
                                      "\"port\": \"live\"");
    auto sr04 = depz::Sr04::open(std::move(link));
    sr04->set_sample_period_us(20'000);
    sr04->sample_period_us();
    sr04->measure_once();
}

bool replay_matches() {
    auto sr04 = depz::Sr04::open(depz::Link::replay("sr04.depzrec", /*strict_tx=*/true));
    sr04->set_sample_period_us(20'000);
    return sr04->sample_period_us() == 20'000 && sr04->measure_once().valid();
}
```

`Sr04::open(Link)` does not probe the board, so it records nothing extra; with
`Device::promote(Device::open(link))` the identity probe becomes part of the
capture (and of the replay). `Link::replay(path, strict, /*realtime=*/true)`
paces the replies by the recorded times, which a loop test needs when it
checks timing. The SDK's own tests replay a real SR04 session this way
(`contracts/vectors/recordings/sr04_session.depzrec`), and run against the C
SDK's fake SR04 over `Link::loopback_pair()`.

## Decode layer

The SR04 codecs, for bytes you read yourself — your own port loop (the
[common guide](../guide.md#mental-model) shows the pattern) or a capture. They
describe the same wire format the live class speaks (through the C SDK), and
match the other language SDKs byte for byte via the golden vectors.

### Decode a measurement

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

### Echo time → distance

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

### Building the commands

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

### Configuration codecs

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

### Single shot vs the loop

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
  (`depz::ECHO_TIMEOUT`); `valid()` is `false` and `distance_mm()` /
  `distance_mm_from_echo()` return `std::nullopt`. It is a normal measurement,
  not an exception.
- **`measure_once()` during the loop throws `BusyError`** (`ERR_BUSY` on the
  wire) — one in-flight measurement path at a time; stop the loop first.
- **Callbacks run on the reader thread** — no blocking, no requests, no
  closing the device from one; use `stream()` when in doubt.
- **Streams drop the oldest** when full — watch `dropped_count()` and pull
  faster or raise `maxsize`.
- **Configured period is a ceiling, not the realised rate** — the echo window
  throttles it; a read-back returns the stored value.
- **Echo decay is clamped on the device** to 4000–65000 µs;
  `set_echo_decay_us()` returns the value in effect. Values above 65535 can't
  be sent (the field is a `std::uint16_t`): `ArgumentError`.
- **Timestamps are device µs** — convert with `to_host_time_us()` after a
  `sync_time()`.
- **Raw integers are authoritative** — `echo_time_us` is the source of truth;
  distance is derived, and temperature compensation is opt-in.
