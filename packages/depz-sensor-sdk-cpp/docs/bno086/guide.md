# BNO086 — user guide

Hands-on guide to the BNO085 / BNO086 in C++: the live class `depz::Bno086`
(`depz/device.hpp`, a wrapper over the C SDK's sensor class) first, then the
codecs in `depz/bno086.hpp` for when you own the transport. For what the
sensor is and its concepts, read the [introduction](introduction.md); for
exact signatures see the [API reference](api.md).

## Contents

- [Open and identify](#open-and-identify)
- [Enable outputs and read reports](#enable-outputs-and-read-reports)
- [The report and its units](#the-report-and-its-units)
- [Callbacks](#callbacks)
- [Choosing outputs and rates](#choosing-outputs-and-rates)
- [Calibration, tare and reorientation](#calibration-tare-and-reorientation)
- [FRS records and sensor metadata](#frs-records-and-sensor-metadata)
- [Diagnostics](#diagnostics)
- [Errors](#errors)
- [Record and replay for tests](#record-and-replay-for-tests)
- [Verification status](#verification-status)
- [Decode layer (your own transport)](#decode-layer-your-own-transport)
- [Gotchas](#gotchas)

## Open and identify

`depz::open_bno086()` finds the board, checks its firmware (`APP_BNO086_*`)
and returns the class — or throws `WrongTypeError` for another board. The
same class serves the BNO085 and the BNO086; the product id tells which chip
is on the board.

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main()
{
    auto imu = depz::open_bno086();
    imu->hardware_reset();  // a clean start: nothing enabled
    const auto pid = imu->product_id();
    std::printf("%s, SH-2 %u.%u.%u\n", pid.sw_part_number == 10004148u ? "BNO085" : "BNO086",
                pid.sw_version_major, pid.sw_version_minor, pid.sw_version_patch);
}
```

`hardware_reset()` pulses the sensor's reset pin: every output is off
afterwards. It is optional — a freshly plugged board is already clean — and
waits up to 0.5 s for the chip's "reset complete" (not an error without it:
firmware up to v0.95 never sent one, ERRATA E9). `depz::open_device()` works
too; `dynamic_cast<depz::Bno086*>` tells.

## Enable outputs and read reports

Nothing streams until you enable an output with a rate. `enable()` returns
the rate the hub actually granted:

```cpp
#include <chrono>
#include <cstdio>
#include "depz/device.hpp"

using namespace std::chrono_literals;
using depz::bno086::SensorId;

void stream_orientation(depz::Bno086& imu)
{
    auto reports = imu.reports(256);  // subscribe first
    const auto granted = imu.enable(SensorId::RotationVector, 100);
    imu.enable(SensorId::Accelerometer, 50);
    std::printf("rotation vector every %u us\n", granted.interval_us);
    for (int i = 0; i < 500; i++) {
        auto r = reports.next(1000ms);
        if (!r) break;
        if (r->sensor_id == static_cast<int>(SensorId::RotationVector)) {
            const auto q = r->quaternion();  // i, j, k, real
            std::printf("i %+.3f j %+.3f k %+.3f real %+.3f  accuracy %d\n", q[0], q[1], q[2], q[3],
                        r->accuracy);
        } else if (r->sensor_id == static_cast<int>(SensorId::Accelerometer)) {
            const auto a = r->xyz();
            std::printf("accel %+.2f %+.2f %+.2f m/s2\n", a[0], a[1], a[2]);
        }
    }
    imu.disable(SensorId::RotationVector);
    imu.disable(SensorId::Accelerometer);
}
```

- **One stream carries every output** — tell reports apart by `sensor_id`.
- **Subscribe before you enable**: a stream only sees reports produced after
  it was created. `get_report(timeout)` returns the next report without a
  stream of your own, but subscribes when called.
- **The granted rate is rounded** to the hub's 1000 / 2^n Hz grid (50 Hz
  becomes 62.5 Hz). `Bno086::rate_ok(requested_interval_us, granted)` says
  whether it is within the normal 0.9–2.1× band.
- **The hub answers "0" at first**: right after enabling, the lab BNO085
  answers the rate question with "interval 0" once or twice; `enable()` asks
  again (up to five times), so the returned rate is the real one.
- `enable(sensor, FeatureRequest{...}, verify)` sets every Set Feature field
  (interval, batch interval, change sensitivity, flags, sensor-specific
  word); with `verify` false it skips the read-back and returns nullopt.
  `feature(sensor)` asks the rate of any sensor.

## The report and its units

A `depz::Bno086Report` is the codec `bno086::Report` — the **wire integers**
(`x_raw`, `i_raw`, ...), authoritative and never rounded — plus methods that
scale them (`value = raw / 2^Q`, the Q point fixed per sensor):

| Sensor (`sensor_id`) | `type` | Method | Units |
|---|---|---|---|
| Accelerometer, linear acceleration, gravity | `Acceleration` | `xyz()` | m/s² (Q8) |
| Gyroscope | `Gyroscope` | `xyz()` | rad/s (Q9) |
| Magnetometer | `Magnetometer` | `xyz()` | µT (Q4) |
| Uncalibrated gyroscope / magnetometer | `Uncalibrated*` | `xyz()`, `bias()` | as calibrated |
| Rotation vector, geomagnetic RV, AR/VR RV | `RotationVector` | `quaternion()`, `accuracy_rad()` | unit quaternion (Q14), rad (Q12) |
| Game RV, AR/VR game RV | `RotationVector` | `quaternion()` (`accuracy_rad()` nullopt) | unit quaternion (Q14) |
| Gyro-integrated RV | `GyroIntegratedRV` | `quaternion()`, `angular_velocity()` | Q14, rad/s (Q10) |
| Raw accelerometer / gyroscope / magnetometer | `RawSensor` | `xyz()` | ADC counts |
| Environment (pressure, light, ...) | `ScalarReport` | `scalar()` | hPa, lux, ... |

The quaternion order is **i, j, k, real** (x, y, z, w). Every report also
carries `timestamp_us` (MCU microseconds), `seq` (a rolling counter — a gap
means a lost report) and `accuracy` (0 unreliable … 3 high).
`bno086::q_point(sensor_id)` gives the Q point of any sensor.

## Callbacks

```cpp
#include <cstdio>
#include "depz/device.hpp"

void with_callback(depz::Bno086& imu)
{
    auto off = imu.on_report([](const depz::Bno086Report& r) {
        std::printf("sensor 0x%02X seq %d\n", r.sensor_id, r.seq);
    });
    imu.enable(depz::bno086::SensorId::GameRotationVector, 200);
    // ...
    off();  // unsubscribe
}
```

Callbacks run on the device's reader thread: keep them short, and never call
a blocking `Bno086` method from one — its answer arrives on that thread.

## Choosing outputs and rates

- **Rotation vector** — absolute orientation (magnetic north), up to 400 Hz.
  Its heading accuracy starts poor (±180°) and improves as the magnetometer
  calibrates — move the board around.
- **Game rotation vector** — accelerometer + gyroscope only: no magnetic
  disturbance, the heading drifts slowly.
- **Geomagnetic rotation vector** — accelerometer + magnetometer, low power,
  up to 90 Hz.
- **Gyro-integrated RV** — up to 1 kHz, with angular velocity: low-latency
  head tracking.
- **Gravity** and **linear acceleration** — the accelerometer split into the
  pull of gravity and your own motion.
- Raw sensors and detectors — contract 05 §4
  (`contracts/05_SENSOR_BNO086.md`) lists all of them with rates and units.

Disable what you do not read: every report crosses USB as its own packet.

## Calibration, tare and reorientation

```cpp
#include <cstdio>
#include "depz/device.hpp"

void calibrate(depz::Bno086& imu)
{
    imu.set_calibration(true, true, true);  // accel, gyro, mag may adapt
    const auto c = imu.calibration();
    std::printf("adapting: accel %d gyro %d mag %d\n", c.accel, c.gyro, c.mag);
    // ... move the board: figure eights for the magnetometer ...
    imu.save_dcd();                   // keep the calibration across power cycles
    imu.tare_now(depz::bno086::TARE_Z);  // the current heading becomes zero
}
```

- The hub calibrates by itself while it runs; `set_calibration()` chooses
  which sensors may adapt, `save_dcd()` stores the result ("dynamic
  calibration data") in the chip's flash, `configure_periodic_dcd(true)` lets
  the hub save it itself, `clear_dcd_and_reset()` throws it away.
- `tare_now(axes, basis)` makes the current orientation the zero until reset;
  `persist_tare()` keeps it. `set_reorientation(x, y, z, w)` sets a fixed
  mounting rotation (all zeros clears it). These three have no SH-2 answer.

## FRS records and sensor metadata

```cpp
#include <cstdio>
#include "depz/device.hpp"

void metadata(depz::Bno086& imu)
{
    const auto m = imu.metadata(depz::bno086::SensorId::Accelerometer);
    std::printf("accelerometer: %.2f mA, fastest every %u us, Q%u\n", m.power_ma(), m.min_period_us,
                m.q_point_1);
    const auto raw = imu.frs_read(0xE302);  // any record by id: 32-bit words
    std::printf("accelerometer metadata: %zu words\n", raw.size());
}
```

The FRS ("flash record system") is the chip's store of configuration
records. Reading an empty or unknown record is an SH-2 refusal
(`StatusError`). `frs_write(record, words)` replaces a record in the chip's
flash — it survives power cycles, so write only what you read back or know
(writing no words erases the record).

## Diagnostics

- `errors(severity)` — the hub's error queue; `counts(sensor)` /
  `clear_counts(sensor)` — per-sensor event counts; `oscillator_type()`.
- `wake()`, `advertisement()`, `shtp_discarded()` (stays 0 on a healthy
  link).
- Escape hatches: `command(id, params, wait_response)` sends any SH-2
  command, `send_shtp(channel, payload)` any SHTP message.

## Errors

- `BusyError` — the bridge holds two outgoing messages at a time; the class
  already waited 200 ms and resent five times before throwing. Busy answers
  happen in normal use (the lab capture has one); the retry absorbs them.
- `StatusError` — an SH-2 refusal (non-zero status in a command or FRS
  answer); the message holds the SH-2 status.
- `TimeoutError` — no answer within 1 s (FRS 2 s) or the device timeout.
- `ArgumentError` — e.g. a sensor without a metadata record, a quaternion
  outside Q14 range.

## Record and replay for tests

```cpp
#include <string>
#include "depz/device.hpp"

void replay(const std::string& path)
{
    auto plain = depz::Device::open(depz::Link::replay(path, /*strict_tx=*/true));
    auto dev = depz::Device::promote(std::move(plain));
    auto* imu = dynamic_cast<depz::Bno086*>(dev.get());
    if (!imu) return;
    imu->hardware_reset();
    imu->product_id();
}
```

`contracts/vectors/recordings/bno086_session.depzrec` was captured by the
Python SDK on the lab BNO085; `tests/test_device.cpp`
(`bno086_session_replay`) replays it strictly — every byte the SDK sends is
compared with the recording. The fake hub `tests/fake_bno086.c` (C SDK)
covers busy retries, refusals and the rarer report types
(`bno086_fake_errors_and_reports`).

## Verification status

Live on the lab BNO085 (I5MFL1ONMCD, `APP_BNO086_v0.99`, SH-2 3.2.13) from
C++: game rotation vector 201 Hz, gravity 50 Hz with |g| = 9.85 m/s²,
gyroscope 100 Hz (below 0.02 rad/s at rest), unit quaternions, no reports
dropped, metadata and error queue read. Strict replay of the Python capture
in C, C++, Python and TS. A BNO086 (part 10004563) was not run from C++; it
speaks the same protocol.

## Decode layer (your own transport)

`depz/bno086.hpp` holds the codecs the class is built on — pure functions, no
I/O. The bridge sends every inbound SHTP frame as `RPT_DATA` (0x91: cmd u8,
capture time u64 µs, frame); outgoing frames go in `SEND_SHTP_PACKET` (0x34).

### SHTP reassembly and requests

```cpp
#include "depz/bno086.hpp"

using namespace depz::bno086;

// One inbound SHTP frame (the RPT_DATA payload after its 9-byte head).
void on_frame(ShtpLayer& shtp, depz::byte_span frame, std::int64_t capture_us)
{
    auto cargo = shtp.feed(frame);
    if (!cargo) return;  // a fragment: wait for the rest
    switch (static_cast<ShtpChannel>(cargo->channel)) {
    case ShtpChannel::InputNormal:
    case ShtpChannel::InputWake:
        for (const auto& r : parse_input_cargo(cargo->payload, capture_us)) (void)r;
        break;
    case ShtpChannel::GyroRv:
        if (auto r = parse_gyro_rv_cargo(cargo->payload, capture_us)) (void)r;
        break;
    default:  // Control: SH-2 answers, matched by their content
        break;
    }
}

// Enable the rotation vector at 100 Hz: the frame to send in SEND_SHTP_PACKET.
depz::bytes enable_rv(ShtpLayer& shtp)
{
    const depz::bytes cmd = sh2_build_set_feature(0x05, 10'000);
    return shtp.next_frame(static_cast<int>(ShtpChannel::Control), cmd);
}
```

- `ShtpLayer` reassembles fragments per channel and numbers outgoing frames;
  call `reset()` after a sensor reset. `discarded` counts dropped fragments.
- The bridge answers each `SEND_SHTP_PACKET` at once with `RPT_STATUS`: OK,
  or busy — resend the same frame after at least 200 ms.
- Encoders: `sh2_build_set_feature`, `_get_feature_request`,
  `_product_id_request`, `_command_request`, `_frs_read_request`,
  `_frs_write_request`, `_frs_write_data`.
- `parse_input_cargo()` folds the 0xFB / 0xFA timebase records and each
  report's delay into `timestamp_us`; scale the raw integers with the Q points
  in the table above (`RV_ACCURACY_Q` = 12, `GYRO_RV_ANGVEL_Q` = 10).
- There is no request id: a Get Feature answer carries only the sensor id, a
  command answer the command and its sequence number — match on those. And
  expect "interval 0" for a read-back or two right after enabling.

## Gotchas

- **Nothing streams until you enable it**, and a reset turns everything off.
- **Subscribe before you enable** — a stream misses what came before it.
- **The granted rate is rounded** to the 1000 / 2^n Hz grid.
- **One stream, many sensors** — filter on `sensor_id`.
- **Rotation-vector heading needs a calibrated magnetometer** (the lab board
  started at ±180°); use the game rotation vector when north does not matter.
- **Never block in a callback.**
- **Tare, persist tare, reorientation and periodic DCD have no answer** from
  the hub; success means the bridge took the frame.
