# BNO055 — user guide

Hands-on guide to the BNO055 in C++: the live class `depz::Bno055`
(`depz/device.hpp`) first, then the codecs of `depz/bno055.hpp` underneath
it. For what the sensor is and its concepts, read the
[introduction](introduction.md); for exact signatures see the
[API reference](api.md). The rules every live device shares — errors as
exceptions, callbacks on the reader thread, bounded drop-oldest streams —
are in the [common guide](../guide.md#live-hardware).

## Contents

- [Open and configure](#open-and-configure)
- [Hello world: orientation](#hello-world-orientation)
- [The sample](#the-sample)
- [Streaming](#streaming)
- [Modes and power](#modes-and-power)
- [Units](#units)
- [Axis remap](#axis-remap)
- [Calibration](#calibration)
- [Status and self-test](#status-and-self-test)
- [Page 1: sensor configs, interrupts, unique id](#page-1-sensor-configs-interrupts-unique-id)
- [Raw registers](#raw-registers)
- [Reset and bridge diagnostics](#reset-and-bridge-diagnostics)
- [Record and replay](#record-and-replay)
- [Decode layer (your own transport)](#decode-layer-your-own-transport)
- [Gotchas](#gotchas)

## Open and configure

`open_bno055()` finds the DEPZ boards by USB id, probes the chosen one and
returns a `std::unique_ptr<depz::Bno055>` (`WrongTypeError` if the board is
something else). `open_device()` does the same for any board —
`dynamic_cast<depz::Bno055*>` tells — and `Device::promote()` turns a plain
device on a bare link into one. `Bno055::open(Link)` attaches the class to
a link with no identity probe (tests, replay).

```cpp
#include "depz/device.hpp"

namespace bno = depz::bno055;

std::unique_ptr<depz::Bno055> open_imu() {
    depz::OpenOptions opt;
    opt.port = "/dev/ttyACM0";                   // or opt.serial / opt.index
    auto imu = depz::open_bno055(opt);
    imu->configure();                            // CONFIG -> units -> NDOF, fusion live on return
    return imu;
}

void imu_mode(depz::Bno055& imu) {
    bno::Units units;
    units.euler_rad = true;                      // Euler angles in radians
    imu.configure(bno::OprMode::Imu,             // accel + gyro, no magnetometer
                  units, bno::placement("P2"));  // datasheet mounting P2
}
```

`configure(mode, units, remap, calibration)` is the usual session setup in
one call. It waits until the sensor has finished booting, switches to
CONFIG, writes the units (the default `Units{}` is m/s², dps, degrees, °C,
Windows orientation), the axis remap and a stored calibration profile when
given (`std::nullopt` leaves them), then switches to `mode` — `OprMode::Ndof`
by default. In a fusion mode it returns only once the fusion outputs are
live: they read zero for ~70 ms after every switch out of CONFIG, so it
polls the quaternion until it is non-zero and throws `TimeoutError` if that
takes over a second. The class remembers the arguments for
`restore_configuration()`.

## Hello world: orientation

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main() {
    auto imu = depz::open_bno055();
    imu->configure();
    auto samples = imu->samples();               // subscribe before starting
    imu->start_stream(10);                       // the full 46-byte block, 100 Hz
    for (int i = 0; i < 500; ++i) {              // ~5 s
        auto s = samples.next(std::chrono::milliseconds(500));
        if (!s) break;                           // timeout, or the device went away
        const auto& e = *s->euler;               // the full block carries every channel
        const auto& c = *s->calibration;
        std::printf("heading %6.1f  roll %6.1f  pitch %6.1f  calib %u/%u/%u/%u\n",
                    e[0], e[1], e[2], unsigned(c.system), unsigned(c.gyro),
                    unsigned(c.accel), unsigned(c.mag));
    }
    imu->stop_stream();
}
```

Without a stream, poll: `read_sample()` reads the full block once,
`read_quaternion()` just the 8 quaternion bytes (the cheapest read) and
returns `{w, x, y, z}`.

## The sample

`Bno055Sample` is one decoded register block. A channel is `std::nullopt`
when the block did not cover all of its bytes; values are scaled by the
units in force when the stream started (`s.units`) or, for a polled read,
when it was read.

| field | meaning | unit (default `Units{}`) |
|---|---|---|
| `timestamp_us` | MCU µs at the timer tick (stream) / read completion (poll) | µs |
| `quaternion` | `{w, x, y, z}`, unit length | — |
| `euler` | `{heading, roll, pitch}` | degrees (radians with `euler_rad`) |
| `accel` | acceleration including gravity | m/s² (mg with `accel_mg`) |
| `linear_accel` | acceleration with gravity removed | m/s² (always) |
| `gravity` | gravity vector | m/s² (always) |
| `gyro` | angular rate | dps (rad/s with `gyro_rps`) |
| `mag` | magnetic field | µT |
| `temperature` | chip temperature | °C (°F with `temp_f`) |
| `calibration` | `bno055::CalibStatus{system, gyro, accel, mag}`, each 0..3 | — |
| `addr`, `raw` | the block's start register and its bytes | — |

The full block (`FULL_BLOCK_ADDR` `0x08`, `FULL_BLOCK_LEN` 46) covers every
channel; the quaternion block (`QUAT_BLOCK_ADDR` `0x20`, 8 bytes) only
`quaternion`. In non-fusion modes the fusion channels (quaternion, Euler,
gravity, linear acceleration) are present but read zero; in CONFIG mode
everything does.

## Streaming

```cpp
#include <cstdio>
#include "depz/device.hpp"

namespace bno = depz::bno055;

void stream_ways(depz::Bno055& imu) {
    auto stream = imu.samples(256);              // pull: bounded, drop-oldest
    auto unsub = imu.on_sample([](const depz::Bno055Sample& s) {
        if (s.quaternion) { /* reader thread: copy it out, return fast */ }
    });

    imu.start_stream(10);                        // full block every 10 ms (~3.2 ms of bus)
    if (auto s = stream.next(std::chrono::milliseconds(200)))
        std::printf("t=%llu us\n", static_cast<unsigned long long>(s->timestamp_us));
    depz::Bno055Sample one = imu.get_sample();   // the next one; TimeoutError after 1 s
    (void)one;
    std::printf("dropped %llu, parse errors %llu\n",
                static_cast<unsigned long long>(stream.dropped_count()),
                static_cast<unsigned long long>(imu.stream_parse_errors()));

    imu.start_stream(20, bno::QUAT_BLOCK_ADDR, bno::QUAT_BLOCK_LEN);  // replaces it: quaternion, 50 Hz
    imu.stop_stream();
    unsub();
}
```

`start_stream(period_ms, addr, len, trigger)` has the bridge read the block
on its own timer and push it with the MCU timestamp of the tick, so gaps
show up as jumps in `timestamp_us`. The fusion runs at 100 Hz, so 10 ms is
the useful floor; a period shorter than the block's read time skips slots
(counted in `bridge_info().slots_skipped`). A new `start_stream()` replaces
the running one; `stop_stream()` is a no-op when nothing streams, and
`streaming()` says which. The block must be 1..128 bytes within the page
(`ArgumentError` otherwise). The units a stream scales by are latched when
it starts — change units, then restart the stream.

`trigger = bno055::TRIGGER_INT` reads the block on each INT rising edge
instead, with `period_ms` as a missed-edge watchdog (0 = none). On these
boards that means the **motion** interrupts only — see
[page 1](#page-1-sensor-configs-interrupts-unique-id).

Commands stay usable while streaming — `calibration_status()`,
`read_sample()`, mode switches, `bridge_info()`; page-1 access and
`self_test()` do not. `stream_parse_errors()` counts stream reports that
failed to decode — 0 in a healthy session.

## Modes and power

```cpp
#include "depz/device.hpp"

namespace bno = depz::bno055;

void modes(depz::Bno055& imu) {
    bno::OprMode m = imu.operation_mode();       // e.g. OprMode::Ndof
    imu.set_operation_mode(bno::OprMode::Amg);   // raw accel + mag + gyro, no fusion
    imu.set_operation_mode(m);                   // back; waits for the fusion to run

    imu.set_power_mode(1);                       // 0 normal, 1 low power, 2 suspend
    imu.set_power_mode(0);
    imu.set_temperature_source(1);               // 0 accelerometer, 1 gyroscope
}
```

| `OprMode` | sensors | output |
|---|---|---|
| `Config` | — | configuration only, all outputs zero |
| `AccOnly`, `MagOnly`, `GyroOnly`, `AccMag`, `AccGyro`, `MagGyro`, `Amg` | as named | raw data only |
| `Imu` | accel + gyro | relative orientation, 100 Hz |
| `Compass` | accel + mag | absolute heading, 20 Hz |
| `M4g` | accel + mag | relative orientation from the magnetometer, 50 Hz |
| `NdofFmcOff` | all three | absolute orientation, 100 Hz, slow mag calibration |
| `Ndof` | all three | absolute orientation, 100 Hz (the default) |

The sensor only switches **between CONFIG and an operating mode** — a write
from one operating mode straight to another is silently ignored.
`set_operation_mode()` goes through CONFIG for you, with the datasheet
settle times (25 ms into CONFIG, 10 ms out of it), so `Ndof` → `Amg` just
works and costs the CONFIG round trip; into a fusion mode it also waits for
the first non-zero quaternion (up to ~1 s; a suspended sensor never gets
there, and the call returns anyway). `set_power_mode()` and
`set_temperature_source()` are CONFIG-only registers too; the class goes
there and back.

## Units

```cpp
#include "depz/device.hpp"

namespace bno = depz::bno055;

void units(depz::Bno055& imu) {
    bno::Units u;
    u.accel_mg = true;                           // ACC_DATA in mg
    u.gyro_rps = true;                           // rad/s
    u.euler_rad = true;                          // radians
    u.temp_f = true;                             // deg F
    imu.set_units(u);                            // through CONFIG
    bno::Units back = imu.units();               // read back from UNIT_SEL
    (void)back;
}
```

`Units{}` (all flags off) is m/s², dps, degrees, °C and Windows orientation —
the SDK default. The sensor's own power-on value is different (Android
orientation, `UNIT_SEL = 0x80`), so `configure()` always writes it.
`android = true` flips the pitch sign convention.

Linear acceleration and gravity stay in m/s² even with `accel_mg` — only
the raw acceleration switches to mg. That is what the sensor does
(measured), whatever some datasheet tables say; the class scales
accordingly. The scale factors are in the
[unit codecs](#unit-codecs) below.

## Axis remap

```cpp
#include "depz/device.hpp"

namespace bno = depz::bno055;

void axes(depz::Bno055& imu) {
    imu.set_axis_placement("P0");                // one of the datasheet placements P0..P7

    bno::AxisRemap r;                            // output X = minus chip Y, Y = chip X
    r.x = bno::AXIS_Y;
    r.y = bno::AXIS_X;
    r.x_negative = true;
    imu.set_axis_remap(r);

    bno::AxisRemap now = imu.axis_remap();
    (void)now;
}
```

P1 is the power-on default (chip axes as printed). A mapping that uses one
axis twice throws `ArgumentError` — the sensor would silently keep the old
one — and so does a placement name other than `"P0"`..`"P7"`. The remap is a
CONFIG-mode register; the class goes there and back. To set it once per
session, pass it to `configure()`.

## Calibration

```cpp
#include <cstdio>
#include "depz/device.hpp"

void calib(depz::Bno055& imu) {
    depz::bno055::CalibStatus st = imu.calibration_status();   // 0..3 each
    std::printf("sys %u gyr %u acc %u mag %u%s\n", unsigned(st.system),
                unsigned(st.gyro), unsigned(st.accel), unsigned(st.mag),
                st.fully_calibrated() ? "  (all 3)" : "");
}
```

What each sensor needs (datasheet §3.11): **gyro** — hold still for a few
seconds; **accel** — six still poses, each axis up and down; **magnetometer**
— slow figure-eights in the air. The fusion calibrates continuously in the
background; you cannot switch it off.

Save the result once `fully_calibrated()` is true, restore it after every
power cycle:

```cpp
#include "depz/device.hpp"

namespace bno = depz::bno055;

bno::CalibrationProfile save_profile(depz::Bno055& imu) {
    return imu.calibration_profile();            // 22 bytes, read through CONFIG
}

void restore_profile(depz::Bno055& imu, const bno::CalibrationProfile& saved) {
    imu.configure(bno::OprMode::Ndof, {}, std::nullopt, saved);
    // or, on a configured sensor: imu.write_calibration_profile(saved);
}
```

The sensor exposes the profile in CONFIG mode only, so both calls go there
and back — and that restarts the fusion (another ~70 ms of zeros). Store it
as you like: `CalibrationProfile::pack()` gives the 22 register bytes and
`CalibrationProfile::unpack()` takes them back.

A restored profile is a starting point, not a lock: as soon as the fusion
runs it keeps refining the offsets (with an uncalibrated magnetometer it
rewrites the magnetometer radius straight away). To check that a write
landed, read it back without leaving CONFIG (`read_registers()` in CONFIG,
see [raw registers](#raw-registers)). Don't store the profile of an
uncalibrated sensor: its magnetometer radius is 0, outside the legal
144..1280, and restoring it makes the sensor report a fusion configuration
error (`SYS_ERR = 9`) while fusion keeps running.

The soft-iron matrix is there too: `sic_matrix()` / `set_sic_matrix()`
(9 × `int16_t`, row-major, 1.0 = 16384; written through CONFIG).

## Status and self-test

```cpp
#include <cstdio>
#include "depz/device.hpp"

void status(depz::Bno055& imu) {
    depz::Bno055Status s = imu.system_status();
    std::printf("SYS_STATUS %u, SYS_ERR %u, ST_RESULT 0x%02X\n",
                unsigned(s.status), unsigned(s.error), unsigned(s.self_test));

    depz::Bno055Status t = imu.self_test();      // ~0.45 s in CONFIG, mode restored
    bool passed = (t.self_test & 0x0F) == 0x0F;  // accel, mag, gyro, MCU
    std::printf("self-test %s\n", passed ? "passed" : "FAILED");
}
```

`Bno055Status` holds four registers: `self_test` (`ST_RESULT`: bit 0 accel,
1 magnetometer, 2 gyroscope, 3 MCU — 1 = passed; after power-on it holds the
power-on self-test), `clk_status` (`SYS_CLK_STATUS`), `status`
(`SYS_STATUS`: 0 idle, 1 system error, 2 initialising peripherals, 3 system
initialisation, 4 running the self-test, 5 fusion running, 6 running without
fusion) and `error` (`SYS_ERR`). `error` only means something when `status`
is 1 — at other times the register may still hold an old value.
`system_status()` skips `INT_STA`, which clears on read.

`self_test()` triggers the built-in self-test: it goes to CONFIG, runs it
(~0.45 s), reads the status and restores the operating mode. It throws
`ArgumentError` while a stream runs. A self-test run in CONFIG leaves
`SYS_STATUS` at 4 until the mode leaves CONFIG, so when the sensor was in
CONFIG the call ends with a step into ACCONLY and back; the returned `status`
is the 4 read during the test.

## Page 1: sensor configs, interrupts, unique id

```cpp
#include <cstdio>
#include "depz/device.hpp"

namespace bno = depz::bno055;

void page1(depz::Bno055& imu) {
    depz::bytes id = imu.unique_id();            // 16 bytes
    std::printf("unique id: %zu bytes\n", id.size());

    bno::AccelConfig acc = imu.accel_config();   // power-on: range 1 (4 g), bandwidth 3
    acc.range = 2;                               // 8 g — effective in non-fusion modes only
    imu.set_accel_config(acc);

    imu.set_interrupt_setting(0x11, 0x14);       // ACC_AM_THRES (page 1), raw byte
    imu.set_interrupt_enable(0x40);              // ACC_AM: the any-motion engine on
    imu.set_interrupt_mask(0x40);                // ... and routed to the INT pin
    std::uint8_t sta = imu.read_interrupt_status();   // INT_STA — clears on read
    (void)sta;
    imu.clear_interrupt();                       // reset the status bits and the INT pin
}
```

Page 1 holds the raw sensor configs (`accel_config`, `gyro_config`,
`mag_config` and their setters — the fusion modes override them), the
unique id and the interrupt setup (`interrupt_enable` / `interrupt_mask`,
and `set_interrupt_setting(reg, value)` for the raw thresholds and
durations at `0x11..0x1F`; other registers throw `ArgumentError`). The class
selects page 1 around each call and always returns to page 0; the config
and interrupt-setting writes go through CONFIG as well.

**Every page-1 call throws `ArgumentError` while a stream runs** — the
bridge reads its block from whatever page is selected. `read_interrupt_status()`
and `clear_interrupt()` are page-0 registers and work any time.

On these boards only the **motion** interrupts work (any- / no-motion,
high-g, high-rate); the data-ready bits exist but never fire on sensor
firmware 03.11. An INT-triggered stream
(`start_stream(1000, addr, len, bno055::TRIGGER_INT)`) then reads the block
on each motion event, with `period_ms` as the watchdog.

## Raw registers

```cpp
#include "depz/device.hpp"

namespace bno = depz::bno055;

void raw(depz::Bno055& imu) {
    depz::bytes q = imu.read_registers(bno::REG_QUA_DATA, 8);          // page 0
    depz::bytes acc = imu.read_registers(bno::REG1_ACC_CONFIG, 1, 1);  // page 1, back to 0
    depz::bytes v{std::byte{0x00}};
    imu.write_registers(bno::REG_TEMP_SOURCE, depz::as_bytes(v));      // as is: no CONFIG trip
    (void)q; (void)acc;
}
```

`read_registers(addr, len, page)` / `write_registers(addr, data, page)` are
the escape hatch: any register on page 0 or 1, split into 128-byte
transfers, page 1 selected and deselected around the access (and refused
while streaming). They do **not** go through CONFIG: most configuration
registers ignore writes outside CONFIG, and a raw `REG_OPR_MODE` write
neither passes through CONFIG nor waits for the fusion — use the typed
setters unless you mean it.

## Reset and bridge diagnostics

```cpp
#include <cstdio>
#include "depz/device.hpp"

void diagnostics(depz::Bno055& imu) {
    depz::bno055::Bno055Info info = imu.bridge_info();   // safe while streaming
    std::printf("ids %s, sensor firmware %02X.%02X, block read %u us avg, "
                "%u slots skipped, %u sensor resets\n",
                imu.is_alive() ? "ok" : "WRONG", unsigned(info.sw_rev >> 8),
                unsigned(info.sw_rev & 0xFF), unsigned(info.read_avg_us),
                unsigned(info.slots_skipped), unsigned(info.sensor_resets));

    imu.reset_sensor();              // nRESET pulse; waits until the sensor has booted
    imu.restore_configuration();     // the last configure() again
}
```

`bridge_info()` returns the sensor ids, its firmware revision (`sw_rev`,
BCD: `0x0311` = 03.11) and the bridge's counters — block read times,
`slots_skipped`, I2C errors and bus recoveries, `sensor_resets`; the read
times and `slots_skipped` restart at each `start_stream()`. `is_alive()` is
true when the bridge passed its chip-id handshake and the four ids are the
BNO055's (`0xA0`, `0xFB`, `0x32`, `0x0F`).

`reset_sensor()` stops any stream, pulses the sensor's reset pin and waits:
the bridge answers after its chip-id handshake (~0.5 s), then the class
polls `SYS_STATUS` until the sensor has left its boot and self-test states
(a mode written earlier would be lost). After it the sensor is in CONFIG
with power-on settings — every output reads zero until you configure again,
or call `restore_configuration()`, which repeats the last `configure()`
(`ArgumentError` if there was none).

If `sensor_resets` grows during a long run, the bridge recovered a stuck bus
by resetting the sensor: the stream keeps running, but the configuration is
gone — call `restore_configuration()`.

## Record and replay

A session records to a `.depzrec` file through a recording link and
replays byte-exact; `Device::promote()` attaches the class from the
recorded identity probe, or `Bno055::open(Link)` attaches it directly:

```cpp
#include "depz/device.hpp"

bool replay_ndof(const std::string& path) {
    auto plain = depz::Device::open(depz::Link::replay(path, /*strict_tx=*/true));
    plain->set_timeout(std::chrono::milliseconds(2000));
    auto dev = depz::Device::promote(std::move(plain));
    auto* imu = dynamic_cast<depz::Bno055*>(dev.get());
    if (!imu) return false;
    imu->reset_sensor();
    imu->configure();                            // NDOF
    auto samples = imu->samples(128);
    imu->start_stream(10);
    auto s = samples.next(std::chrono::milliseconds(2000));
    imu->stop_stream();
    return s && s->quaternion.has_value();
}
```

The class issues the same register sequences as the Python SDK's `Bno055`
— the boot-settle poll, the CONFIG round trips, the fusion-start poll — so
the committed captures, recorded by the Python SDK on a real board
(`APP_BNO055_v0.12`, sensor firmware 03.11), replay strictly through it.
The C++ suite runs `device_bno_ndof_replay`
(`contracts/vectors/recordings/bno055_ndof_full_100hz.depzrec`): promote
from the recorded probe, bridge info, reset with the boot-settle poll,
`configure()` into NDOF with the fusion-start poll, the calibration profile
through CONFIG, system status, and 100 samples of the full block at 100 Hz,
each with the recorded timestamp and every channel present. The C SDK's
suite, the same code underneath, also replays the second capture
(`bno055_imu_quat_units_50hz`: IMU mode, every unit flag set, the
quaternion block at 50 Hz).

**Status:** the class is verified on replays of these real captures and live
on a lab board (board I0MG1KQN8DW, 28.09.2026): `configure()` into NDOF, a 3 s stream at 100 Hz,
page-1 access refused while streaming. The C layer underneath was also
turned and shaken by hand for 60 s — see the C SDK's BNO055 guide.

`configure()` and `reset_sensor()` also clear a SYS_STATUS 4 left behind by a
self-test run in CONFIG (by this or another program), which used to make
them time out with "BNO055 did not finish booting" — found live, see the C
SDK's BNO055 guide.

## Decode layer (your own transport)

The `depz::bno055` codecs, for bytes you read yourself — your own port loop
(see the [common guide](../guide.md#mental-model)), a capture, or a plain
`depz::Device` carrying the request / reply part
([`request` / `send`](../guide.md#escape-hatch-raw-requests)). They are the
wire format the live class speaks (through the C SDK). Doing it by hand, you
own the order of register writes and the waits the class does for you.

### The codec surface

| direction | `Bno055Cmd` / `Bno055Rpt` | what | codec |
|---|---|---|---|
| host → device | `ReadReg` (0x32) | read registers (1..128 B) | `pack_read_reg` |
| host → device | `WriteReg` (0x33) | write registers (1..128 B) | `pack_write_reg` |
| host → device | `Reset` (0x34) | reset the sensor | empty; reply after the boot handshake |
| host → device | `StartStream` / `StopStream` (0x35 / 0x36) | streaming | `pack_start_stream` / empty |
| host → device | `GetInfo` (0x37) | bridge info | empty payload |
| device → host | `RegData` (0x91) | register data | `RegData::unpack` |
| device → host | `Info` (0x92) | bridge info (38 B) | `Bno055Info::unpack` |
| device → host | `Stream` (0x93) | streamed register block | `StreamData::unpack` → `decode_block` |

Register addresses are 8-bit; `addr + len` must stay within `0x100` and a
transfer within `XFER_MAX` (128) bytes — the encoders do not police it, the
firmware answers `ERR_INVALID_PARAM`.

### Send a command

The snippets call one small helper that frames a payload and hands it to your
transport:

```cpp
#include "depz/framing.hpp"

// Your transport: write bytes to the board's serial port.
void port_write(const depz::bytes& frame);

void send_command(std::uint8_t cmd, const depz::bytes& payload) {
    static std::uint8_t seq = 0;
    port_write(depz::build_packet(cmd, depz::as_bytes(payload), seq++,
                                  depz::CrcType::Crc8));
}
```

### Registers, modes and pages

Everything the sensor does is a register write. The rules the live class
follows, and a C++ host doing it by hand must follow too:

```cpp
#include "depz/bno055.hpp"

namespace bno = depz::bno055;

auto write1 = [](std::uint8_t reg, std::uint8_t value) {
    depz::bytes v{std::byte{value}};
    send_command(static_cast<std::uint8_t>(bno::Bno055Cmd::WriteReg),
                 bno::pack_write_reg(reg, depz::as_bytes(v)));
};
auto mode = [](bno::OprMode m) { return static_cast<std::uint8_t>(m); };

// 1. settings take only in CONFIG: switch there first (then wait >= 19 ms)
write1(bno::REG_OPR_MODE, mode(bno::OprMode::Config));

// 2. write the units (here the SDK default: m/s^2, dps, degrees, C, Windows)
write1(bno::REG_UNIT_SEL, bno::Units{}.pack());

// 3. page 1 (sensor configs, interrupts, unique id): select, access, back to 0
write1(bno::REG_PAGE_ID, 1);
send_command(static_cast<std::uint8_t>(bno::Bno055Cmd::ReadReg),
             bno::pack_read_reg(bno::REG1_UNIQUE_ID, 16));
write1(bno::REG_PAGE_ID, 0);

// 4. into a fusion mode (wait >= 7 ms; fusion outputs read zero ~70 ms)
write1(bno::REG_OPR_MODE, mode(bno::OprMode::Ndof));
```

Wait for each command's `RPT_STATUS` before sending the next. The sensor only
switches **between CONFIG and an operating mode**: a write from one operating
mode straight to another (NDOF → AMG) is silently ignored, so go through
CONFIG. `OPR_MODE` reads back with extra bits after a reset — mask it with
`& 0x0F`.

| `OprMode` | sensors | output |
|---|---|---|
| `Config` | — | configuration only, all outputs zero |
| `AccOnly`, `MagOnly`, `GyroOnly`, `AccMag`, `AccGyro`, `MagGyro`, `Amg` | as named | raw data only |
| `Imu` | accel + gyro | relative orientation, 100 Hz |
| `Compass` | accel + mag | absolute heading, 20 Hz |
| `M4g` | accel + mag | relative orientation from the magnetometer, 50 Hz |
| `NdofFmcOff` | all three | absolute orientation, 100 Hz, slow mag calibration |
| `Ndof` | all three | absolute orientation, 100 Hz |

### Stream a block

`BNO_START_STREAM` reads one register window on a timer. The full block
(`FULL_BLOCK_ADDR`, 46 bytes) carries every output channel; the quaternion
alone (`QUAT_BLOCK_ADDR`, 8 bytes) is the cheapest orientation read:

```cpp
#include "depz/bno055.hpp"

namespace bno = depz::bno055;

// the units a stream is scaled by: latch them BEFORE arming the stream
bno::Units stream_units;

void start_full_stream(const bno::Units& units_in_force) {
    stream_units = units_in_force;
    send_command(static_cast<std::uint8_t>(bno::Bno055Cmd::StartStream),
                 bno::pack_start_stream(bno::TRIGGER_TIMER, bno::FULL_BLOCK_ADDR,
                                        bno::FULL_BLOCK_LEN, /*period_ms=*/10));
}
```

The first sample can arrive before the command's own reply is processed,
which is why the units are latched first. At 10 ms the full block streams at
100 Hz (≈3.2 ms of bus per read); a period shorter than the read time skips
slots and counts them in `slots_skipped`. `TRIGGER_INT` reads on each INT
rising edge instead — for **motion** interrupts only; `period_ms` is then a
missed-edge watchdog (0 disables it).

### Decode a window

`decode_block()` takes the window's start address and bytes and fills every
channel the window covers **completely**; the rest stay `nullopt`. Scale
with the latched units:

```cpp
#include <cstdio>
#include "depz/bno055.hpp"

namespace bno = depz::bno055;

extern bno::Units stream_units;   // latched at START_STREAM

void on_stream(const depz::Packet& pkt) {
    if (pkt.cmd != static_cast<std::uint8_t>(bno::Bno055Rpt::Stream)) return;
    auto s = bno::StreamData::unpack(depz::as_bytes(pkt.payload));
    if (!s) return;
    bno::RawBlock b = bno::decode_block(s->addr, depz::as_bytes(s->data));
    const bno::Units& u = stream_units;

    if (b.quaternion) {
        const auto& q = *b.quaternion;                     // w, x, y, z
        if (q[0] == 0 && q[1] == 0 && q[2] == 0 && q[3] == 0)
            std::printf("not fusing yet\n");  // CONFIG, non-fusion, or ~70 ms after a switch
        else
            std::printf("q = (%.4f, %.4f, %.4f, %.4f)\n", q[0] / bno::QUAT_LSB,
                        q[1] / bno::QUAT_LSB, q[2] / bno::QUAT_LSB, q[3] / bno::QUAT_LSB);
    }
    if (b.euler)                                           // heading, roll, pitch
        std::printf("heading %.2f\n", (*b.euler)[0] / u.euler_lsb());
    if (b.accel)                                           // m/s^2 or mg, per the units
        std::printf("accel z %.2f\n", (*b.accel)[2] / u.accel_lsb());
    if (b.linear_accel)                                    // ALWAYS m/s^2
        std::printf("linear z %.2f m/s^2\n", (*b.linear_accel)[2] / bno::FUSION_ACCEL_LSB);
    if (b.gravity)                                         // ALWAYS m/s^2
        std::printf("gravity z %.2f m/s^2\n", (*b.gravity)[2] / bno::FUSION_ACCEL_LSB);
    if (b.gyro)
        std::printf("gyro x %.2f\n", (*b.gyro)[0] / u.gyro_lsb());
    if (b.mag)
        std::printf("mag x %.2f uT\n", (*b.mag)[0] / bno::MAG_LSB);
    if (b.temperature)
        std::printf("%.0f deg\n", *b.temperature / u.temp_lsb());
    if (b.calib_stat) {
        bno::CalibStatus cs = bno::CalibStatus::unpack(*b.calib_stat);
        std::printf("calib sys %u gyr %u acc %u mag %u\n", unsigned(cs.system),
                    unsigned(cs.gyro), unsigned(cs.accel), unsigned(cs.mag));
    }
}
```

`timestamp_us` is the timer tick (or the INT edge) that triggered the read,
not the I2C completion; gaps show up as jumps in it. The same decoder works
on a polled read — pass the address you asked for and the bytes of the
`RegData` reply.

### Unit codecs

`UNIT_SEL` (`0x3B`) bits **as the silicon implements them** (sensor firmware
03.11; the datasheet's own bit table in §4.3.60 is off by one — its Table
3-11 and Bosch's driver agree with this):

| bit | constant / `Units` field | set means | LSB (value = raw / LSB) |
|---|---|---|---|
| 0 | `UNIT_ACC_MG` / `accel_mg` | acceleration in **mg** | m/s²: 100 · mg: 1 |
| 1 | `UNIT_GYR_RPS` / `gyro_rps` | angular rate in **rad/s** | dps: 16 · rad/s: 900 |
| 2 | `UNIT_EUL_RAD` / `euler_rad` | Euler angles in **radians** | degrees: 16 · radians: 900 |
| 4 | `UNIT_TEMP_F` / `temp_f` | temperature in **°F** | °C: 1 · °F: 0.5 (1 LSB = 2 °F) |
| 7 | `UNIT_ORI_ANDROID` / `android` | Android orientation | Windows = 0 (flips the pitch sign) |

Fixed scales: magnetometer 16 LSB/µT (`MAG_LSB`), quaternion 2¹⁴
(`QUAT_LSB`). **Linear acceleration and gravity ignore bit 0**: they stay
m/s² at 100 LSB (`FUSION_ACCEL_LSB`) — measured; the datasheet's Tables
3-33 / 3-35 promise mg. The power-on value is `0x80` (Android orientation),
so write `UNIT_SEL` explicitly:

```cpp
#include <cstdio>
#include "depz/bno055.hpp"

namespace bno = depz::bno055;

bno::Units power_on = bno::Units::unpack(0x80);
std::printf("android=%d accel LSB %.0f\n", power_on.android, power_on.accel_lsb());

bno::Units want;
want.accel_mg = true;
want.euler_rad = true;
std::printf("UNIT_SEL = 0x%02X\n", unsigned(want.pack()));   // 0x05
```

Bits the sensor does not implement are ignored on unpack and never set on
pack.

### Calibration codecs

`CALIB_STAT` (`0x35`, also the last byte of the full block) holds four
0..3 fields; 3/3/3/3 is fully calibrated. The fusion calibrates continuously
in the background — gyro: hold still for a few seconds; accel: six still
poses, each axis up and down; magnetometer: slow figure-eights.

The calibration **profile** — offsets and radii, 22 bytes at
`REG_CALIB_PROFILE` (`0x55`) — is read and written **in CONFIG only**, all 22
bytes in one transfer:

```cpp
#include <cstdio>
#include "depz/bno055.hpp"

namespace bno = depz::bno055;

// data: the RegData reply to READ_REG 0x55, 22 bytes (read in CONFIG)
bool profile_worth_storing(depz::byte_span data, std::uint8_t calib_stat) {
    auto prof = bno::CalibrationProfile::unpack(data);
    if (!prof) return false;                        // not exactly 22 bytes
    std::printf("accel offset %d %d %d, radius %d; mag radius %d\n",
                prof->accel_offset[0], prof->accel_offset[1], prof->accel_offset[2],
                prof->accel_radius, prof->mag_radius);
    // an uncalibrated magnetometer reports radius 0 — outside 144..1280
    return bno::CalibStatus::unpack(calib_stat).fully_calibrated() &&
           prof->mag_radius >= 144 && prof->mag_radius <= 1280;
}

// restore: one WRITE_REG of all 22 bytes at 0x55, in CONFIG
void restore_profile(const bno::CalibrationProfile& prof) {
    depz::bytes bytes = prof.pack();
    send_command(static_cast<std::uint8_t>(bno::Bno055Cmd::WriteReg),
                 bno::pack_write_reg(bno::REG_CALIB_PROFILE, depz::as_bytes(bytes)));
}
```

A restored profile is a starting point, not a lock: once the fusion runs it
keeps refining the offsets (with an uncalibrated magnetometer it rewrites the
magnetometer radius straight away). To check that a write landed, read it
back **without leaving CONFIG**. Restoring a profile whose magnetometer radius
is 0 makes the sensor report a fusion configuration error (`SYS_ERR = 9`)
while fusion keeps running — store a profile only once `CALIB_STAT` reads
3/3/3/3.

### Axis-remap codecs

`AXIS_MAP_CONFIG` / `AXIS_MAP_SIGN` (`0x41` / `0x42`) say which chip axis
feeds each output axis and its sign. The eight datasheet mounting placements
are ready-made:

```cpp
#include <cstdio>
#include "depz/bno055.hpp"

namespace bno = depz::bno055;

auto p2 = bno::placement("P2");               // datasheet placement P2
auto regs = p2->pack();                       // {0x24, 0x06}
std::printf("P2 = 0x%02X / 0x%02X\n", unsigned(regs->first), unsigned(regs->second));

// or by hand: output X = minus chip Y, output Y = chip X, Z unchanged
bno::AxisRemap custom;
custom.x = bno::AXIS_Y;
custom.y = bno::AXIS_X;
custom.x_negative = true;
if (auto cs = custom.pack()) {                // nullopt: not a permutation
    depz::bytes both{std::byte{cs->first}, std::byte{cs->second}};
    // both registers in one write, in CONFIG
    send_command(static_cast<std::uint8_t>(bno::Bno055Cmd::WriteReg),
                 bno::pack_write_reg(bno::REG_AXIS_MAP_CONFIG, depz::as_bytes(both)));
}
```

P1 (`0x24` / `0x00`) is the power-on default; `PLACEMENTS[i]` holds the raw
pairs. A mapping that uses one axis twice is refused (`pack()` returns
`nullopt`) — the sensor would silently keep its old mapping.

### Page-1 config codecs

The accelerometer, gyroscope and magnetometer configs (page 1, `0x08..0x0B`)
only take effect in the **non-fusion** modes — the fusion modes override
them:

```cpp
#include <cstdio>
#include "depz/bno055.hpp"

namespace bno = depz::bno055;

bno::AccelConfig acc = bno::AccelConfig::unpack(0x0D);  // power-on: range 1 (4 g), bandwidth 3 (62.5 Hz)
acc.range = 2;                                          // 8 g
std::uint8_t acc_byte = acc.pack();                     // 0x0E

depz::bytes gyr_bytes{std::byte{0x38}, std::byte{0x00}};  // power-on: 2000 dps, 32 Hz
auto gyr = bno::GyroConfig::unpack(depz::as_bytes(gyr_bytes));

bno::MagConfig mag = bno::MagConfig::unpack(0x8B);      // bit 7 is not a field ...
std::uint8_t mag_byte = mag.pack();                     // ... so this is 0x0B

std::printf("acc 0x%02X  gyro range %u  mag 0x%02X\n", unsigned(acc_byte),
            unsigned(gyr->range), unsigned(mag_byte));
```

Write them with the page-1 sequence from
[Registers, modes and pages](#registers-modes-and-pages): `PAGE_ID ← 1`, the
write, `PAGE_ID ← 0`. On these boards only the **motion** interrupts work
(any / no-motion, high-g, high-rate); the data-ready interrupt bits exist but
never fire on sensor firmware 03.11.

### Reset and bridge info by hand

```cpp
#include <cstdio>
#include "depz/bno055.hpp"

namespace bno = depz::bno055;

// the answer to Bno055Cmd::GetInfo (empty payload)
void on_info(const depz::Packet& pkt) {
    if (pkt.cmd != static_cast<std::uint8_t>(bno::Bno055Rpt::Info)) return;
    auto info = bno::Bno055Info::unpack(depz::as_bytes(pkt.payload));
    if (!info) return;
    bool ids_ok = info->chip_id == 0xA0 && info->acc_id == 0xFB &&
                  info->mag_id == 0x32 && info->gyr_id == 0x0F;
    std::printf("ids %s, sensor firmware %02X.%02X, block read %u us avg, "
                "%u slots skipped, %u sensor resets\n",
                ids_ok ? "ok" : "WRONG", unsigned(info->sw_rev >> 8),
                unsigned(info->sw_rev & 0xFF), unsigned(info->read_avg_us),
                unsigned(info->slots_skipped), unsigned(info->sensor_resets));
}
```

`Bno055Cmd::Reset` pulses the sensor's reset pin and answers only after the
chip-id handshake (allow ≥ 1.5 s). The reply comes **before the sensor has
finished booting**: poll `SYS_STATUS` (`0x39`) until it leaves 2, 3 and 4
(initialising / self-test) before writing a mode, or the mode is lost. After
a reset the sensor is in CONFIG with power-on settings (`UNIT_SEL = 0x80`).
If `sensor_resets` grows during a long run, the bridge recovered a stuck bus
by resetting the sensor: the stream keeps running, but the configuration is
gone — write it again.

## Gotchas

- **Zero quaternion = not fusing** — CONFIG, a non-fusion mode, or the first
  ~70 ms after any switch out of CONFIG. `configure()` and
  `set_operation_mode()` wait that out; a raw `write_registers()` to
  `REG_OPR_MODE` does not.
- **Every CONFIG round trip restarts the fusion** — reading the calibration
  profile, changing units, remap, power mode or a page-1 config, the
  self-test: each costs another ~70 ms of zeros.
- **No direct mode-to-mode switch** — the sensor ignores it; the class goes
  through CONFIG. A raw `REG_OPR_MODE` write from NDOF to AMG does nothing.
- **Settings silently ignored outside CONFIG** — use the typed setters (they
  go to CONFIG and back); a raw `UNIT_SEL` write in NDOF does nothing.
- **Linear acceleration and gravity are always m/s²** — `accel_mg` only
  changes `accel`. By hand: scale them with `FUSION_ACCEL_LSB`, never with
  `Units::accel_lsb()`.
- **A stream keeps the units it started with** — restart it after
  `set_units()`.
- **Page 1 and `self_test()` throw `ArgumentError` while streaming** — stop
  the stream first.
- **Don't store an uncalibrated profile** — magnetometer radius 0 is outside
  the legal 144..1280, and restoring it gives `SYS_ERR = 9`.
- **Re-configure after a reset** — `reset_sensor()`, or a rising
  `sensor_resets` in `bridge_info()`, leaves the sensor in CONFIG with
  power-on settings: `restore_configuration()`.
- **Don't benchmark in CONFIG** — the sensor stretches the bus 3–5× harder
  there; a 46-byte read takes ~10 ms instead of ~3 ms.
- **`INT_STA` (`0x37`) clears on read** — keep it out of routine block reads
  (the full block stops at `CALIB_STAT`, `0x35`).
- **Callbacks run on the reader thread** — keep them short and don't call
  blocking device methods from inside one.
