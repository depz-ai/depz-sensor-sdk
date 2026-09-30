# BNO055 — user guide

Hands-on guide to the BNO055: the live sensor class of `depz_sensor_io.h`
first, then the codecs underneath for when you own the transport. For what the
sensor is and its concepts, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md). Devices, errors, threading rules and
streams in general are in the [common guide](../guide.md#live-layer).

The class issues the same register sequences as the Python SDK's `Bno055`, so
a capture made by either SDK replays through the other byte for byte. It is
**verified on replays of real captures and live on a lab board** — see [verification status](#verification-status).

## Contents

- [Open and configure](#open-and-configure)
- [The sample](#the-sample)
- [Polling](#polling)
- [Streaming: streams and callbacks](#streaming-streams-and-callbacks)
- [Modes and power](#modes-and-power)
- [Units](#units)
- [Axis remap](#axis-remap)
- [Calibration](#calibration)
- [Status and self-test](#status-and-self-test)
- [Page 1: sensor configs, interrupts, unique id](#page-1-sensor-configs-interrupts-unique-id)
- [Raw registers](#raw-registers)
- [Reset and bridge diagnostics](#reset-and-bridge-diagnostics)
- [Disconnects](#disconnects)
- [Record and replay for tests](#record-and-replay-for-tests)
- [Verification status](#verification-status)
- [Decode layer (your own transport)](#decode-layer-your-own-transport)
- [Gotchas](#gotchas)

## Open and configure

`depz_open_device()` finds the board, probes it and attaches the BNO055 class
when the firmware is `APP_BNO055_*`. With no options it takes the DEPZ board
with the smallest USB serial; set `port`, `serial` or `index` to pick one.
Check `depz_is_bno055()` — the same call opens any DEPZ board, and a BNO055
call on another type fails with `DEPZ_E_WRONG_TYPE`.

```c
#include <depz_sensor_io.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    depz_open_options opt = DEPZ_OPEN_OPTIONS_INIT;
    depz_device *dev;
    depz_stream *samples;
    depz_bno055_sample s;
    int i;

    if (argc > 1) opt.port = argv[1];
    if (depz_open_device(&opt, &dev) != DEPZ_OK) {
        fprintf(stderr, "open: %s\n", depz_last_error());
        return 1;
    }
    if (!depz_is_bno055(dev)) {                     /* sensor type "bno055" */
        fprintf(stderr, "%s is not a BNO055\n", depz_device_port(dev));
        depz_device_close(dev);
        return 1;
    }
    /* CONFIG -> default units -> NDOF; returns once the fusion runs */
    if (depz_bno055_configure(dev, DEPZ_BNO055_MODE_NDOF, NULL, NULL, NULL) != DEPZ_OK) {
        fprintf(stderr, "configure: %s\n", depz_last_error());
        depz_device_close(dev);
        return 1;
    }
    samples = depz_bno055_samples(dev, 256);        /* subscribe before starting */
    if (depz_bno055_start_stream(dev, 10, DEPZ_BNO055_FULL_BLOCK_ADDR,   /* 100 Hz */
                                 DEPZ_BNO055_FULL_BLOCK_LEN,
                                 DEPZ_BNO055_TRIGGER_TIMER) == DEPZ_OK) {
        for (i = 0; i < 300 && depz_stream_next(samples, &s, 1000) == DEPZ_OK; i++)
            printf("heading %6.1f  roll %6.1f  pitch %6.1f  calib %u/%u/%u/%u\n",
                   s.euler[0], s.euler[1], s.euler[2],
                   (unsigned)s.calibration.system, (unsigned)s.calibration.gyro,
                   (unsigned)s.calibration.accel, (unsigned)s.calibration.mag);
        depz_bno055_stop_stream(dev);
    }
    depz_stream_close(samples);
    depz_device_close(dev);
    return 0;
}
```

**`depz_bno055_configure(dev, mode, units, remap, calibration)`** is the usual
session setup in one call:

1. wait until the sensor has finished booting (after a power-up or reset it
   is still initialising for a moment, and a mode written then is lost);
2. switch to CONFIG;
3. write the units — `NULL` means the SDK default: m/s², degrees per second,
   degrees, °C, Windows orientation;
4. write the axis remap, if `remap` is not `NULL` (else leave it as it is);
5. write a stored calibration profile, if `calibration` is not `NULL`;
6. switch to `mode`. In a fusion mode it returns only once the fusion outputs
   are live (a non-zero quaternion) — they read zero for ~70 ms after every
   switch out of CONFIG. If the fusion does not start within 1 s it fails
   with `DEPZ_E_TIMEOUT`.

The class remembers the arguments: `depz_bno055_restore_configuration(dev)`
applies them again (after a reset, or when the bridge reset the sensor on its
own — see [reset](#reset-and-bridge-diagnostics)). It fails with `DEPZ_E_ARG`
before the first `configure()`.

```c
depz_bno055_units units = { .euler_rad = true };        /* Euler angles in radians */
depz_bno055_axis_remap p2;

depz_bno055_placement("P2", &p2);                        /* datasheet mounting P2 */
depz_bno055_configure(dev, DEPZ_BNO055_MODE_IMU,          /* accel + gyro, no magnetometer */
                      &units, &p2, NULL);
```

On a bare link — a replay, say — `depz_bno055_open_link(link, &dev)` attaches
the class without a probe; `depz_device_open_link()` followed by
`depz_device_promote()` asks the firmware name first and attaches it then.

## The sample

`depz_bno055_sample` is one decoded register block, already divided by the
right scale factor. The `present` bit mask (`DEPZ_BNO055_HAS_*`) says which
channels the block covered in full; the others are zero.

| field | `present` bit | meaning | unit (default) |
|---|---|---|---|
| `timestamp_us` | — | MCU time of the timer tick (stream) or of the read's completion (poll) | µs |
| `quaternion[4]` | `HAS_QUATERNION` | `w, x, y, z`, unit length | — |
| `euler[3]` | `HAS_EULER` | heading, roll, pitch | degrees |
| `accel[3]` | `HAS_ACCEL` | acceleration including gravity | m/s² |
| `linear_accel[3]` | `HAS_LINEAR_ACCEL` | acceleration with gravity removed | m/s² (always) |
| `gravity[3]` | `HAS_GRAVITY` | the gravity vector | m/s² (always) |
| `gyro[3]` | `HAS_GYRO` | angular rate | degrees/s |
| `mag[3]` | `HAS_MAG` | magnetic field | µT |
| `temperature` | `HAS_TEMPERATURE` | chip temperature | °C |
| `calibration` | `HAS_CALIBRATION` | `system`, `gyro`, `accel`, `mag`, each 0..3 | — |
| `units` | — | the units the values were scaled by | — |
| `addr`, `len`, `raw[]` | — | the block's start register, length and bytes | — |

The full block (`DEPZ_BNO055_FULL_BLOCK_ADDR` `0x08`, 46 bytes) carries every
channel; the quaternion block (`DEPZ_BNO055_QUAT_BLOCK_ADDR` `0x20`, 8 bytes)
only the quaternion. Any other window works too — a channel is present when
the window holds all of its bytes. In non-fusion modes the fusion fields
(quaternion, Euler, gravity, linear acceleration) read zero; in CONFIG mode
everything does. An all-zero quaternion therefore means "not fusing (yet)".

## Polling

Without a stream, read a block when you want one:

```c
depz_bno055_sample s;
double q[4];

if (depz_bno055_read_sample(dev, DEPZ_BNO055_FULL_BLOCK_ADDR,
                            DEPZ_BNO055_FULL_BLOCK_LEN, &s) == DEPZ_OK
        && (s.present & DEPZ_BNO055_HAS_GRAVITY))
    printf("gravity %.2f %.2f %.2f m/s^2\n", s.gravity[0], s.gravity[1], s.gravity[2]);

if (depz_bno055_read_quaternion(dev, q) == DEPZ_OK)      /* 8 bytes: the cheapest read */
    printf("q = %.4f %.4f %.4f %.4f\n", q[0], q[1], q[2], q[3]);
```

A poll is a request / reply round trip over USB, so a stream is the way to
get 100 Hz. Polling works alongside a running stream. The units come from the
class's memory of the last units it wrote or read (read once from the sensor
when it knows none).

## Streaming: streams and callbacks

`depz_bno055_start_stream(dev, period_ms, addr, len, trigger)` makes the
bridge read the block on its own timer and push it with the MCU timestamp of
the tick — so gaps show up as jumps in `timestamp_us`. The fusion updates at
100 Hz, so 10 ms is the useful floor. The full block takes ≈3.2 ms of bus
time, the quaternion block ≈1.2 ms; a period shorter than the read skips
slots (counted in the bridge's `slots_skipped`). A new `start_stream()`
replaces the running one; `depz_bno055_stop_stream()` is a no-op when nothing
streams, and `depz_bno055_streaming()` tells.

Three ways to consume, as for every sensor class:

```c
#include <stdio.h>

static void on_sample(const depz_bno055_sample *s, void *user)
{
    (void)user;                                          /* the reader thread: keep it short */
    if (s->present & DEPZ_BNO055_HAS_QUATERNION)
        printf("t=%llu us  w=%.4f\n", (unsigned long long)s->timestamp_us, s->quaternion[0]);
}

void consume(depz_device *dev)
{
    depz_stream *st = depz_bno055_samples(dev, 256);     /* 1. bounded, drop-oldest */
    depz_bno055_sample s;
    int token;

    depz_bno055_on_sample(dev, on_sample, NULL, &token); /* 2. callback */
    depz_bno055_start_stream(dev, 20, DEPZ_BNO055_QUAT_BLOCK_ADDR,   /* 50 Hz */
                             DEPZ_BNO055_QUAT_BLOCK_LEN, DEPZ_BNO055_TRIGGER_TIMER);

    while (depz_stream_next(st, &s, 1000) == DEPZ_OK && s.timestamp_us < 5000000u)
        ;                                                /* ... use s ... */
    printf("dropped %llu\n", (unsigned long long)depz_stream_dropped_count(st));

    if (depz_bno055_get_sample(dev, 1000, &s) == DEPZ_OK)  /* 3. just the next one */
        printf("w=%.4f\n", s.quaternion[0]);

    depz_bno055_stop_stream(dev);
    depz_bno055_off_sample(dev, token);
    depz_stream_close(st);
}
```

- **`depz_bno055_samples(dev, maxsize)`** — a pull stream (`maxsize` 0 =
  256); when full, the oldest sample goes and `depz_stream_dropped_count()`
  grows. It receives everything produced after the call, so subscribe before
  `start_stream()`.
- **`depz_bno055_on_sample(dev, cb, user, &token)`** — a callback on the
  reader thread; `depz_bno055_off_sample(dev, token)` removes it.
- **`depz_bno055_get_sample(dev, timeout_ms, &s)`** — the next sample after
  the call (a negative timeout = 1000 ms), `DEPZ_E_TIMEOUT` on silence.
  Samples between two calls are not kept — use a stream for a steady flow.

A stream's samples are scaled by the units in effect **when the stream
started** — change the units with the stream stopped, or restart it
afterwards. `depz_bno055_stream_parse_errors(dev)` counts stream reports whose
length did not match the block; 0 in a healthy session.

While a stream runs, page-0 calls keep working — `read_sample()`,
`calibration_status()`, mode switches, units — but page-1 calls and
`self_test()` are refused with `DEPZ_E_ARG` (the bridge reads page 0 for the
stream; switching pages under it would stream the wrong registers).

`DEPZ_BNO055_TRIGGER_INT` reads the block on each rising edge of the
sensor's INT pin instead of on a timer, with `period_ms` as a watchdog for a
missed edge (0 turns it off). On sensor firmware 03.11 only the **motion**
interrupts fire (see [page 1](#page-1-sensor-configs-interrupts-unique-id)) —
there is no data-ready interrupt — so this is a "read on motion" stream, not
a faster one.

## Modes and power

```c
uint8_t mode;

depz_bno055_get_operation_mode(dev, &mode);              /* DEPZ_BNO055_MODE_NDOF */
depz_bno055_set_operation_mode(dev, DEPZ_BNO055_MODE_AMG);   /* raw accel + mag + gyro */
depz_bno055_set_operation_mode(dev, DEPZ_BNO055_MODE_NDOF);  /* waits for the fusion */

depz_bno055_set_power_mode(dev, DEPZ_BNO055_POWER_LOW);  /* accel only until motion */
depz_bno055_set_temperature_source(dev, DEPZ_BNO055_TEMP_FROM_GYRO);
```

| mode | sensors | output |
|---|---|---|
| `DEPZ_BNO055_MODE_CONFIG` | — | configuration only, all outputs zero |
| `ACCONLY`, `MAGONLY`, `GYROONLY`, `ACCMAG`, `ACCGYRO`, `MAGGYRO`, `AMG` | as named | raw data only |
| `IMU` | accel + gyro | relative orientation, 100 Hz |
| `COMPASS` | accel + mag | absolute heading, 20 Hz |
| `M4G` | accel + mag | relative orientation from the magnetometer, 50 Hz |
| `NDOF_FMC_OFF` | all three | absolute orientation, 100 Hz, slow mag calibration |
| `NDOF` | all three | absolute orientation, 100 Hz |

"Relative" orientation starts from wherever the sensor was; "absolute" uses
the magnetometer to point heading at magnetic north.

The sensor only switches **between CONFIG and an operating mode** — a write
from one operating mode straight to another (NDOF → AMG) is silently
ignored. `depz_bno055_set_operation_mode()` goes through CONFIG for you and
waits the datasheet times (≥ 19 ms into CONFIG, ≥ 7 ms out of it); into a
fusion mode it also waits for the first non-zero quaternion (up to 1 s). A
mode above `NDOF` fails with `DEPZ_E_ARG`; setting the mode it is already in
does nothing.

The power mode (`DEPZ_BNO055_POWER_NORMAL` / `_LOW` / `_SUSPEND`) and the
temperature source (`DEPZ_BNO055_TEMP_FROM_ACCEL` / `_GYRO`) are written in
CONFIG and the operating mode is restored afterwards. A suspended sensor
stops updating its registers, so the fusion never "starts" there:
`set_operation_mode()` gives up waiting after 1 s, `configure()` fails with
`DEPZ_E_TIMEOUT`.

## Units

```c
depz_bno055_units u = { .accel_mg = true, .gyro_rps = true,
                        .euler_rad = true, .temp_f = true };

depz_bno055_set_units(dev, &u);        /* through CONFIG and back */
depz_bno055_get_units(dev, &u);        /* read back from UNIT_SEL */
```

| field | set means | clear (the SDK default) |
|---|---|---|
| `accel_mg` | acceleration in mg | m/s² |
| `gyro_rps` | angular rate in rad/s | degrees/s |
| `euler_rad` | Euler angles in radians | degrees |
| `temp_f` | temperature in °F | °C |
| `android` | Android orientation (flips the pitch sign) | Windows orientation |

An all-false `depz_bno055_units` is the SDK default, which `configure()`
writes when `units` is `NULL`. The sensor's own power-on value is different
(Android orientation), so `configure()` always writes the units.

**Linear acceleration and gravity stay in m/s² even with `accel_mg`** — only
the raw acceleration switches to mg. That is what the sensor does (measured on
firmware 03.11), whatever some datasheet tables say; the class scales
accordingly. Samples carry the units they were scaled by (`s.units`).

## Axis remap

The sensor reports along its own chip axes. If the board is mounted rotated,
tell it which chip axis feeds each output axis, and with which sign:

```c
depz_bno055_axis_remap r = { .x = DEPZ_BNO055_AXIS_Y, .y = DEPZ_BNO055_AXIS_X,
                             .z = DEPZ_BNO055_AXIS_Z, .x_negative = true };

depz_bno055_set_axis_placement(dev, "P0");   /* one of the datasheet placements P0..P7 */
depz_bno055_set_axis_remap(dev, &r);         /* or by hand: output X = minus chip Y */
depz_bno055_get_axis_remap(dev, &r);
```

P1 is the power-on default (chip axes as printed). A mapping that uses one
axis twice fails with `DEPZ_E_ARG` — the sensor would silently keep the old
one; an unknown placement name fails the same way. Both are written in CONFIG
and the operating mode is restored.

## Calibration

The fusion calibrates each sensor continuously in the background; you cannot
switch that off. The calibration status says how far it got, 0 (not
calibrated) to 3 (fully calibrated), for the system and each sensor:

```c
depz_bno055_calib_status cs;

if (depz_bno055_calibration_status(dev, &cs) == DEPZ_OK)
    printf("sys %u gyro %u accel %u mag %u%s\n", (unsigned)cs.system,
           (unsigned)cs.gyro, (unsigned)cs.accel, (unsigned)cs.mag,
           depz_bno055_fully_calibrated(&cs) ? " — fully calibrated" : "");
```

What each sensor needs (datasheet §3.11): **gyroscope** — hold still for a
few seconds; **accelerometer** — six still poses, each axis pointing up and
then down; **magnetometer** — slow figure-eights in the air. The full block
carries the status in every sample too (`s.calibration`).

Once all four read 3, save the **calibration profile** — the offsets and
radii the fusion found, 22 bytes — and write it back after every power cycle
so the sensor starts calibrated:

```c
#include <stdio.h>

/* Save: read in CONFIG (the class goes there and back). */
int save_profile(depz_device *dev, const char *path)
{
    depz_bno055_calib_profile p;
    FILE *f;
    if (depz_bno055_read_calibration_profile(dev, &p) != DEPZ_OK) return -1;
    if (p.mag_radius < 144 || p.mag_radius > 1280) return -1;   /* not calibrated yet */
    if (!(f = fopen(path, "wb"))) return -1;
    fwrite(&p, sizeof p, 1, f);
    fclose(f);
    return 0;
}

/* Restore at the next session: as part of configure()... */
int restore_profile(depz_device *dev, const char *path)
{
    depz_bno055_calib_profile p;
    FILE *f = fopen(path, "rb");
    size_t n;
    if (!f) return -1;
    n = fread(&p, sizeof p, 1, f);
    fclose(f);
    if (n != 1) return -1;
    return depz_bno055_configure(dev, DEPZ_BNO055_MODE_NDOF, NULL, NULL, &p);
    /* ...or on its own: depz_bno055_write_calibration_profile(dev, &p) */
}
```

(Storing the struct as raw bytes is fine on one machine; for a portable file
store the 22 register bytes of `depz_bno055_pack_calib_profile()` instead.)

A restored profile is a starting point, not a lock: as soon as the fusion
runs it keeps refining the offsets (with an uncalibrated magnetometer it
rewrites the magnetometer radius straight away). Don't store the profile of an
uncalibrated sensor: its magnetometer radius is 0, outside the legal
144..1280, and restoring it makes the sensor report a fusion configuration
error (`SYS_ERR` = 9) while the fusion keeps running. Reading the profile is
a CONFIG round trip, so it costs another ~70 ms of zero fusion outputs.

The **soft-iron matrix** — a 3 × 3 correction for magnetic distortion from
nearby metal — is there too: `depz_bno055_get_sic_matrix(dev, m)` /
`depz_bno055_set_sic_matrix(dev, m)`, 9 × `int16_t` row by row, 1.0 = 16384
(the identity matrix is the default).

## Status and self-test

```c
depz_bno055_status_regs st;

if (depz_bno055_system_status(dev, &st) == DEPZ_OK)
    printf("status %u, error %u, power-on self-test %s\n", (unsigned)st.status,
           (unsigned)st.error, (st.self_test & 0x0F) == 0x0F ? "passed" : "FAILED");

if (depz_bno055_self_test(dev, &st) == DEPZ_OK)   /* ~0.45 s; the mode is restored */
    printf("self-test result 0x%02X\n", (unsigned)st.self_test);
```

`depz_bno055_status_regs` holds four registers: `self_test` (`ST_RESULT`: bit
0 accelerometer, 1 magnetometer, 2 gyroscope, 3 the sensor's MCU — 1 =
passed, so `0x0F` is all good), `clk_status`, `status` (`SYS_STATUS`) and
`error` (`SYS_ERR`):

| `status` | meaning | | `error` | meaning |
|---|---|---|---|---|
| 0 | idle | | 0 | no error |
| 1 | system error — see `error` | | 1 / 2 | peripheral / system initialisation error |
| 2 | initialising peripherals | | 3 | self-test failed |
| 3 | system initialisation | | 4 / 5 / 6 | register map value / address out of range, write error |
| 4 | executing self-test | | 7 | low-power mode not available for this operating mode |
| 5 | fusion algorithm running | | 8 | accelerometer power mode not available |
| 6 | running without fusion | | 9 / 10 | fusion / sensor configuration error |

`error` only means something while `status` is 1 — at other times the
register may still hold an old value. `depz_bno055_system_status()` skips
the interrupt-status register that sits among these, because reading it
clears it. `depz_bno055_self_test()` runs the chip's built-in self-test: it
goes to CONFIG, triggers the test, waits ~0.45 s, reads the result and
restores the operating mode; it is refused while a stream runs.
A self-test run in CONFIG leaves `SYS_STATUS` at 4 ("executing self-test")
until the mode leaves CONFIG, so when the sensor was in CONFIG the call ends
with a step into ACCONLY and back; the returned `status` is the 4 read during
the test.

## Page 1: sensor configs, interrupts, unique id

The sensor's registers come in two pages; everything above lives on page 1.
Each call below switches to page 1, does its access and always switches back
to page 0 — and each is refused with `DEPZ_E_ARG` while a stream runs.

```c
uint8_t uid[16];
depz_bno055_accel_config acc;

depz_bno055_unique_id(dev, uid);                  /* 16-byte chip id */

depz_bno055_get_accel_config(dev, &acc);          /* power-on: range 1 (4 g), bandwidth 3 */
acc.range = 2;                                    /* 8 g — effective in non-fusion modes only */
depz_bno055_set_accel_config(dev, &acc);

depz_bno055_set_interrupt_setting(dev, 0x11, 0x14);  /* ACC_AM_THRES: raw threshold byte */
depz_bno055_set_interrupt_enable(dev, 0x40);         /* ACC_AM: the any-motion engine on */
depz_bno055_set_interrupt_mask(dev, 0x40);           /* ...and routed to the INT pin */

uint8_t sta;
depz_bno055_read_interrupt_status(dev, &sta);     /* INT_STA — clears on read */
depz_bno055_clear_interrupt(dev);                 /* reset the status bits and the INT pin */
```

- **Sensor configs** — `depz_bno055_get/set_accel_config`, `_gyro_config`,
  `_mag_config` (range, bandwidth, power per sensor; the fields are the
  datasheet's codes — see the [codecs](#page-1-config-codecs)). The setters
  write in CONFIG and restore the mode. The fusion modes **override** these
  configs; they only take effect in the non-fusion modes.
- **Interrupts** — `depz_bno055_set_interrupt_setting(dev, reg, value)`
  writes one raw motion-interrupt setting (page-1 registers `0x11..0x1F`:
  thresholds, durations, axes; others fail with `DEPZ_E_ARG`).
  `_set_interrupt_enable` turns interrupt engines on, `_set_interrupt_mask`
  routes them to the INT pin (bits: `0x04` gyro any-motion, `0x08` gyro
  high-rate, `0x20` accel high-g, `0x40` accel any-motion, `0x80` accel
  no-motion). On these boards only the **motion** interrupts work; the
  data-ready bits (`0x01`, `0x02`, `0x10`) exist but never fire on sensor
  firmware 03.11. `depz_bno055_read_interrupt_status()` (a page-0 register)
  clears the bits it returns.

## Raw registers

For anything the class does not wrap:

```c
uint8_t id[7], page1[2];
uint8_t unit_sel = 0x00;

depz_bno055_read_registers(dev, DEPZ_BNO055_REG_CHIP_ID, id, sizeof id, 0);   /* page 0 */
depz_bno055_read_registers(dev, DEPZ_BNO055_REG1_GYR_CONFIG_0, page1, 2, 1);  /* page 1 */
depz_bno055_write_registers(dev, DEPZ_BNO055_REG_UNIT_SEL, &unit_sel, 1, 0);
```

Transfers longer than 128 bytes are split for you; page 1 is selected around
the access and page 0 restored, and page 1 is refused while streaming. A raw
write does **not** go through CONFIG: most configuration registers only take
writes in CONFIG mode, and a raw `OPR_MODE` write neither goes through CONFIG
nor waits for the fusion — use the typed calls for those. A raw `UNIT_SEL`
write also leaves the class's idea of the units stale: call
`depz_bno055_get_units()` afterwards so samples are scaled right.

## Reset and bridge diagnostics

```c
depz_bno055_info info;
bool alive;

depz_bno055_reset_sensor(dev);          /* nRESET pulse; waits until the sensor has booted */
depz_bno055_restore_configuration(dev); /* re-apply the last configure() */

if (depz_bno055_is_alive(dev, &alive) == DEPZ_OK && !alive)
    printf("sensor missing or ids wrong\n");

if (depz_bno055_bridge_info(dev, &info) == DEPZ_OK)   /* safe while streaming */
    printf("sensor firmware %02X.%02X, block read %u us avg, %u slots skipped, "
           "%u I2C errors, %u sensor resets\n",
           (unsigned)(info.sw_rev >> 8), (unsigned)(info.sw_rev & 0xFF),
           (unsigned)info.read_avg_us, (unsigned)info.slots_skipped,
           (unsigned)info.i2c_errors, (unsigned)info.sensor_resets);
```

- **`depz_bno055_reset_sensor()`** pulses the sensor's reset pin. The bridge
  answers after the chip-id handshake (~0.5 s); the class then polls
  `SYS_STATUS` until the sensor has finished booting, since a mode written
  before that is lost. Any stream stops. Afterwards the sensor is in CONFIG
  with power-on settings — every output reads zero until you configure again.
- **`depz_bno055_is_alive()`** — the bridge passed its chip-id handshake and
  the four ids are the BNO055's (`0xA0`, `0xFB`, `0x32`, `0x0F`). A failed
  info request reads as "not alive", not as an error.
- **`depz_bno055_bridge_info()`** — the ids, the sensor firmware revision
  (`sw_rev`, BCD: `0x0311` = 03.11) and the bridge counters: block-read times
  (`read_min_us` / `_max_us` / `_avg_us`), `slots_skipped` (timer ticks lost
  because the bus was busy), `i2c_errors` / `last_i2c_error`,
  `bus_recoveries`, `tx_dropped` and `sensor_resets`. The read-time counters
  and `slots_skipped` restart with each stream.
- **`sensor_resets` rising** during a long run means the bridge recovered a
  stuck bus by resetting the sensor: the stream keeps running, but the sensor
  is back in CONFIG with power-on settings. Call
  `depz_bno055_restore_configuration()`.

## Disconnects

When the cable goes, the reader thread notices and:

- requests that are waiting fail with `DEPZ_E_CLOSED`, and so does every
  later call on the device;
- streams **hand out what they already hold, then** return `DEPZ_E_CLOSED`;
- a `DEPZ_DEV_EV_DISCONNECTED` event fires (callback and event streams), with
  the reason in `text`; `depz_device_closed()` becomes `true`.

There is no automatic reconnect: open the device again once it is back, and
`configure()` it — the sensor lost power with the board, and with it the
configuration and the calibration it had reached (restore a saved profile).

## Record and replay for tests

Tee a live session into a `.depzrec` file, then replay it with no board
attached. Strict replay (`strict_tx = true`) also checks that your code sends
exactly the bytes it sent when recording, so a changed register sequence fails
loudly with `DEPZ_E_REPLAY_MISMATCH` instead of drifting.

```c
static void session(depz_device *dev)
{
    depz_bno055_sample s;
    depz_bno055_reset_sensor(dev);
    depz_bno055_configure(dev, DEPZ_BNO055_MODE_NDOF, NULL, NULL, NULL);
    depz_bno055_read_sample(dev, DEPZ_BNO055_FULL_BLOCK_ADDR, DEPZ_BNO055_FULL_BLOCK_LEN, &s);
}

int record_and_replay(const char *port)
{
    depz_link *serial, *rec, *rp;
    depz_device *dev;

    /* 1. Record against the real board. */
    if (depz_link_open_serial(port, &serial) != DEPZ_OK) return -1;
    if (depz_link_open_recording(serial, "bno.depzrec", "\"note\":\"bench\"", &rec) != DEPZ_OK)
        return -1;                                 /* serial already freed */
    if (depz_bno055_open_link(rec, &dev) != DEPZ_OK) return -1;
    session(dev);
    depz_device_close(dev);

    /* 2. Replay: same calls, same order, recorded answers. */
    if (depz_link_open_replay("bno.depzrec", true, false, &rp) != DEPZ_OK) return -1;
    if (depz_bno055_open_link(rp, &dev) != DEPZ_OK) return -1;
    session(dev);
    depz_device_close(dev);
    return 0;
}
```

The poll loops — the boot wait after a reset, the fusion-start wait after a
mode switch — issue as many register reads as the sensor needed on the day,
and the replay serves exactly those answers, so the replayed session takes the
same path. The driver's fixed waits (mode-switch settle times, the self-test)
are skipped on a replay link, so a replay runs as fast as the bytes flow.

The test suite replays the two committed BNO055 captures strictly this way,
both recorded by the Python SDK on `APP_BNO055_v0.12` (sensor firmware 03.11)
— so the C and Python drivers issue the same bytes:

- `io_bno_ndof_replay` — `bno055_ndof_full_100hz.depzrec`: identity probe,
  `depz_device_promote()`, bridge info, `reset_sensor()` with the boot wait,
  `configure()` into NDOF with the default units and the fusion-start wait,
  the calibration profile read through CONFIG and back, system status, then
  the full block streamed at 100 Hz — 100 samples, every channel present,
  timestamps and bytes as recorded;
- `io_bno_imu_replay` — `bno055_imu_quat_units_50hz.depzrec`: the same
  session into IMU mode with every unit switched (mg, rad/s, radians, °F,
  Android orientation), then the quaternion block streamed at 50 Hz — 50
  samples, scaled by those units.

## Verification status

**Verified on replays of real captures and live on a lab board.** The two
captures above were recorded on a real BNO055 board by the Python SDK, and
they replay strictly through this class, so the C class issues the same
register sequences and decodes the same values. Live, from C (board I0MG1KQN8DW, 28.09.2026):
bridge info, self-test (0x0F, all pass), unique id, `configure()` into NDOF,
`read_sample()`, and the 10 ms timer stream — 6002 samples in 60 s (100.0 Hz),
none dropped, every quaternion of unit norm; turning and tilting the board by
hand moved heading over the full circle, pitch to ±80° with gravity following
(9.57 m/s² on Y at 80°, 6.9 / 6.9 on X / Z at 45° roll), shaking read up to
~5 g of linear acceleration; gyro and magnetometer reached calibration 3.

**A sensor quirk found live and fixed:** a self-test run in CONFIG mode (as
on a freshly powered sensor) left SYS_STATUS at 4 ("executing self-test")
for good — rewriting CONFIG did not clear it, leaving CONFIG did — and the
next `configure()` timed out with "BNO055 did not finish booting" (the Python
and TypeScript SDKs the same). Now `depz_bno055_self_test()` steps through
ACCONLY back to CONFIG when it stays there, and the boot wait of
`configure()` / `reset_sensor()` takes a 4 lasting 20 polls — POST shows it
for ~35 ms, a handful — as that leftover and clears it the same way. Both
cases checked live from C, Python and TypeScript. The measured sensor behaviour
this page describes (unit quirks, timings, the zero-quaternion gap) comes
from the Python SDK's bench work on the same boards.

## Decode layer (your own transport)

If you run your own serial code, the BNO055 codecs of `depz_sensor_sdk.h`
work without the live layer — the class above is built from them. You then do
the register sequencing yourself (the class's order and waits are described
above); the snippets below build command payloads and decode reply / report
payloads (see the common guide's [mental model](../guide.md#mental-model)).

### The codec surface

| direction | id | what | codec |
|---|---|---|---|
| host → device | `0x32` | read registers (1..128 B) | `depz_bno055_pack_read_reg` |
| host → device | `0x33` | write registers (1..128 B) | `depz_bno055_pack_write_reg` |
| host → device | `0x34` | reset the sensor | empty payload; reply after the boot handshake |
| host → device | `0x35` / `0x36` | start / stop streaming | `depz_bno055_pack_start_stream` / empty |
| host → device | `0x37` | bridge info | empty payload |
| device → host | `0x91` | register data | `depz_bno055_unpack_reg_data` |
| device → host | `0x92` | bridge info (38 B) | `depz_bno055_unpack_info` |
| device → host | `0x93` | streamed register block | `depz_bno055_unpack_stream` → `depz_bno055_decode_block` |

Register addresses are 8-bit; `addr + len` must stay within `0x100`.

### Send a command

The snippets call one small helper that frames a payload and hands it to your
transport:

```c
#include <depz_sensor_sdk.h>

/* Your transport: write bytes to the board's serial port. */
void port_write(const uint8_t *bytes, size_t len);

static unsigned seq;

void send_packet(uint8_t cmd, const uint8_t *payload, size_t len)
{
    static uint8_t frame[DEPZ_HEADER_SIZE + 1 + DEPZ_BNO055_XFER_MAX + 4];
    size_t n;
    if (depz_build_packet(cmd, payload, len, seq++, DEPZ_CRC8,
                          frame, sizeof frame, &n) == 0)
        port_write(frame, n);
}
```

### Registers, modes and pages

Everything the sensor does is a register write. The rules the class follows,
and a C host with its own transport must follow too:

```c
uint8_t p[1 + DEPZ_BNO055_XFER_MAX];
size_t n;
uint8_t v;

/* 1. settings take only in CONFIG: switch there first (then wait >= 19 ms) */
v = DEPZ_BNO055_MODE_CONFIG;
n = depz_bno055_pack_write_reg(DEPZ_BNO055_REG_OPR_MODE, &v, 1, p);
send_packet(DEPZ_BNO055_CMD_WRITE_REG, p, n);

/* 2. write the units (here the SDK default: m/s^2, dps, degrees, C, Windows) */
depz_bno055_units units = {0};
v = depz_bno055_pack_units(&units);
n = depz_bno055_pack_write_reg(DEPZ_BNO055_REG_UNIT_SEL, &v, 1, p);
send_packet(DEPZ_BNO055_CMD_WRITE_REG, p, n);

/* 3. page 1 (sensor configs, interrupts, unique id): select, access, back to 0 */
v = 1;
n = depz_bno055_pack_write_reg(DEPZ_BNO055_REG_PAGE_ID, &v, 1, p);
send_packet(DEPZ_BNO055_CMD_WRITE_REG, p, n);
n = depz_bno055_pack_read_reg(DEPZ_BNO055_REG1_UNIQUE_ID, 16, p);
send_packet(DEPZ_BNO055_CMD_READ_REG, p, n);
v = 0;
n = depz_bno055_pack_write_reg(DEPZ_BNO055_REG_PAGE_ID, &v, 1, p);
send_packet(DEPZ_BNO055_CMD_WRITE_REG, p, n);

/* 4. into a fusion mode (wait >= 7 ms; fusion outputs read zero ~70 ms) */
v = DEPZ_BNO055_MODE_NDOF;
n = depz_bno055_pack_write_reg(DEPZ_BNO055_REG_OPR_MODE, &v, 1, p);
send_packet(DEPZ_BNO055_CMD_WRITE_REG, p, n);
```

Wait for each command's `RPT_STATUS` before sending the next. The sensor only
switches **between CONFIG and an operating mode**: a write from one operating
mode straight to another (NDOF → AMG) is silently ignored, so go through
CONFIG. `OPR_MODE` reads back with extra bits after a reset — mask it with
`& 0x0F`. `depz_bno055_pack_write_reg()` returns `0` for a length outside
`1..DEPZ_BNO055_XFER_MAX`.

| mode | sensors | output |
|---|---|---|
| `DEPZ_BNO055_MODE_CONFIG` | — | configuration only, all outputs zero |
| `ACCONLY`, `MAGONLY`, `GYROONLY`, `ACCMAG`, `ACCGYRO`, `MAGGYRO`, `AMG` | as named | raw data only |
| `IMU` | accel + gyro | relative orientation, 100 Hz |
| `COMPASS` | accel + mag | absolute heading, 20 Hz |
| `M4G` | accel + mag | relative orientation from the magnetometer, 50 Hz |
| `NDOF_FMC_OFF` | all three | absolute orientation, 100 Hz, slow mag calibration |
| `NDOF` | all three | absolute orientation, 100 Hz |

### Stream a block

`BNO_START_STREAM` reads one register window on a timer. The full block
(`0x08`, 46 bytes) carries every output channel; the quaternion alone
(`0x20`, 8 bytes) is the cheapest orientation read:

```c
/* the units a stream is scaled by: latch them BEFORE arming the stream */
depz_bno055_units stream_units;

void start_full_stream(const depz_bno055_units *units_in_force)
{
    stream_units = *units_in_force;
    uint8_t p[5];
    size_t n = depz_bno055_pack_start_stream(DEPZ_BNO055_TRIGGER_TIMER,
                                             DEPZ_BNO055_FULL_BLOCK_ADDR,
                                             DEPZ_BNO055_FULL_BLOCK_LEN,
                                             10 /* ms: 100 Hz */, p);
    send_packet(DEPZ_BNO055_CMD_START_STREAM, p, n);
}
```

The first sample can arrive before the command's own reply is processed,
which is why the units are latched first. At 10 ms the full block streams at
100 Hz (≈3.2 ms of bus per read); a period shorter than the read time skips
slots and counts them in `slots_skipped`. `DEPZ_BNO055_TRIGGER_INT` reads on
each INT rising edge instead — for **motion** interrupts only; `period_ms` is
then a missed-edge watchdog (0 disables it).

### Decode a window

`depz_bno055_decode_block()` takes the window's start address and bytes and
fills every channel the window covers **completely** (`has_*` flags); the
rest are zero. Scale with the latched units:

```c
#include <stdio.h>

extern depz_bno055_units stream_units;   /* latched at START_STREAM */

void on_stream(const depz_event *ev)
{
    if (ev->type != DEPZ_EV_PACKET || ev->cmd != DEPZ_BNO055_RPT_STREAM)
        return;
    depz_bno055_stream s;
    if (depz_bno055_unpack_stream(ev->payload, ev->payload_len, &s) != 0)
        return;
    depz_bno055_block b;
    depz_bno055_decode_block(s.addr, s.data, s.len, &b);

    const depz_bno055_units *u = &stream_units;
    if (b.has_quaternion) {
        double w = b.quaternion[0] / DEPZ_BNO055_QUAT_LSB;
        double x = b.quaternion[1] / DEPZ_BNO055_QUAT_LSB;
        double y = b.quaternion[2] / DEPZ_BNO055_QUAT_LSB;
        double z = b.quaternion[3] / DEPZ_BNO055_QUAT_LSB;
        if (w == 0 && x == 0 && y == 0 && z == 0)
            printf("not fusing yet\n");        /* CONFIG, non-fusion, or ~70 ms after a switch */
        else
            printf("q = (%.4f, %.4f, %.4f, %.4f)\n", w, x, y, z);
    }
    if (b.has_euler)                               /* heading, roll, pitch */
        printf("heading %.2f\n", b.euler[0] / depz_bno055_euler_lsb(u));
    if (b.has_accel)                               /* m/s^2 or mg, per the units */
        printf("accel z %.2f\n", b.accel[2] / depz_bno055_accel_lsb(u));
    if (b.has_linear_accel)                        /* ALWAYS m/s^2 */
        printf("linear z %.2f m/s^2\n", b.linear_accel[2] / DEPZ_BNO055_FUSION_ACCEL_LSB);
    if (b.has_gravity)                             /* ALWAYS m/s^2 */
        printf("gravity z %.2f m/s^2\n", b.gravity[2] / DEPZ_BNO055_FUSION_ACCEL_LSB);
    if (b.has_gyro)
        printf("gyro x %.2f\n", b.gyro[0] / depz_bno055_gyro_lsb(u));
    if (b.has_mag)
        printf("mag x %.2f uT\n", b.mag[0] / DEPZ_BNO055_MAG_LSB);
    if (b.has_temperature)
        printf("%.0f deg\n", b.temperature / depz_bno055_temp_lsb(u));
    if (b.has_calib_stat) {
        depz_bno055_calib_status cs;
        depz_bno055_unpack_calib_status(b.calib_stat, &cs);
        printf("calib sys %u gyr %u acc %u mag %u\n", (unsigned)cs.system,
               (unsigned)cs.gyro, (unsigned)cs.accel, (unsigned)cs.mag);
    }
}
```

`timestamp_us` is the timer tick (or the INT edge) that triggered the read,
not the I2C completion; gaps show up as jumps in it. The same decoder works
on a polled read — pass the address you asked for and the bytes of the
`RPT_BNO_REG_DATA` reply.

### Unit codecs

`UNIT_SEL` (`0x3B`) bits **as the silicon implements them** (sensor firmware
03.11; the datasheet's own bit table in §4.3.60 is off by one — its Table
3-11 and Bosch's driver agree with this):

| bit | constant | set means | LSB (value = raw / LSB) |
|---|---|---|---|
| 0 | `DEPZ_BNO055_UNIT_ACC_MG` | acceleration in **mg** | m/s²: 100 · mg: 1 |
| 1 | `DEPZ_BNO055_UNIT_GYR_RPS` | angular rate in **rad/s** | dps: 16 · rad/s: 900 |
| 2 | `DEPZ_BNO055_UNIT_EUL_RAD` | Euler angles in **radians** | degrees: 16 · radians: 900 |
| 4 | `DEPZ_BNO055_UNIT_TEMP_F` | temperature in **°F** | °C: 1 · °F: 0.5 (1 LSB = 2 °F) |
| 7 | `DEPZ_BNO055_UNIT_ORI_ANDROID` | Android orientation | Windows = 0 (flips the pitch sign) |

Fixed scales: magnetometer 16 LSB/µT (`DEPZ_BNO055_MAG_LSB`), quaternion
2¹⁴ (`DEPZ_BNO055_QUAT_LSB`). **Linear acceleration and gravity ignore bit
0**: they stay m/s² at 100 LSB (`DEPZ_BNO055_FUSION_ACCEL_LSB`) — measured;
the datasheet's Tables 3-33 / 3-35 promise mg. The power-on value is `0x80`
(Android orientation), so write `UNIT_SEL` explicitly:

```c
#include <stdio.h>

depz_bno055_units u;
depz_bno055_unpack_units(0x80, &u);          /* the power-on value */
printf("android=%d accel LSB %.0f\n", u.android, depz_bno055_accel_lsb(&u));

depz_bno055_units want = { .accel_mg = true, .euler_rad = true };
uint8_t unit_sel = depz_bno055_pack_units(&want);   /* 0x05 */
printf("UNIT_SEL = 0x%02X\n", (unsigned)unit_sel);
```

Bits the sensor does not implement are ignored on unpack and never set on
pack.

### Calibration codecs

`CALIB_STAT` (`0x35`, also the last byte of the full block) holds four
0..3 fields; 3/3/3/3 is fully calibrated. The fusion calibrates continuously
in the background — gyro: hold still for a few seconds; accel: six still
poses, each axis up and down; magnetometer: slow figure-eights.

The calibration **profile** — offsets and radii, 22 bytes at `0x55` — is
read and written **in CONFIG only**, all 22 bytes in one transfer:

```c
#include <stdio.h>

/* data / len: the RPT_BNO_REG_DATA reply to READ_REG 0x55, 22 bytes (in CONFIG) */
bool profile_worth_storing(const uint8_t *data, size_t len, uint8_t calib_stat)
{
    depz_bno055_calib_status st;
    depz_bno055_unpack_calib_status(calib_stat, &st);
    depz_bno055_calib_profile prof;
    if (depz_bno055_unpack_calib_profile(data, len, &prof) != 0)
        return false;                     /* not exactly 22 bytes */
    printf("accel offset %d %d %d, radius %d; mag radius %d\n",
           prof.accel_offset[0], prof.accel_offset[1], prof.accel_offset[2],
           prof.accel_radius, prof.mag_radius);
    /* an uncalibrated magnetometer reports radius 0 — outside 144..1280 */
    return depz_bno055_fully_calibrated(&st)
        && prof.mag_radius >= 144 && prof.mag_radius <= 1280;
}

/* restore: one WRITE_REG of all 22 bytes at 0x55, in CONFIG */
void restore_profile(const depz_bno055_calib_profile *prof)
{
    uint8_t bytes[DEPZ_BNO055_CALIB_PROFILE_LEN];
    uint8_t p[1 + DEPZ_BNO055_CALIB_PROFILE_LEN];
    depz_bno055_pack_calib_profile(prof, bytes);
    size_t n = depz_bno055_pack_write_reg(DEPZ_BNO055_REG_CALIB_PROFILE, bytes,
                                          sizeof bytes, p);
    send_packet(DEPZ_BNO055_CMD_WRITE_REG, p, n);
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

```c
depz_bno055_axis_remap a;
uint8_t cfg_sign[2];
uint8_t p[3];

depz_bno055_placement("P2", &a);                      /* datasheet placement P2 */
depz_bno055_pack_axis_remap(&a, &cfg_sign[0], &cfg_sign[1]);   /* 0x24, 0x06 */

/* or by hand: output X = minus chip Y, output Y = chip X, Z unchanged */
depz_bno055_axis_remap custom = { .x = DEPZ_BNO055_AXIS_Y, .y = DEPZ_BNO055_AXIS_X,
                                  .z = DEPZ_BNO055_AXIS_Z, .x_negative = true };
if (depz_bno055_pack_axis_remap(&custom, &cfg_sign[0], &cfg_sign[1]) == 0) {
    /* both registers in one write, in CONFIG */
    size_t n = depz_bno055_pack_write_reg(DEPZ_BNO055_REG_AXIS_MAP_CONFIG,
                                          cfg_sign, 2, p);
    send_packet(DEPZ_BNO055_CMD_WRITE_REG, p, n);
}
```

P1 (`0x24` / `0x00`) is the power-on default; `DEPZ_BNO055_PLACEMENTS[i]`
holds the raw pairs. A mapping that uses one axis twice is refused
(`depz_bno055_pack_axis_remap` returns `-1`, nothing written) — the sensor
would silently keep its old mapping.

### Page-1 config codecs

The accelerometer, gyroscope and magnetometer configs (page 1, `0x08..0x0B`)
only take effect in the **non-fusion** modes — the fusion modes override
them:

```c
#include <stdio.h>

depz_bno055_accel_config acc;
depz_bno055_unpack_accel_config(0x0D, &acc);  /* power-on: range 1 (4 g), bandwidth 3 (62.5 Hz) */
acc.range = 2;                                /* 8 g */
uint8_t acc_byte = depz_bno055_pack_accel_config(&acc);   /* 0x0E */

const uint8_t gyr_bytes[2] = { 0x38, 0x00 };  /* power-on: 2000 dps, 32 Hz */
depz_bno055_gyro_config gyr;
depz_bno055_unpack_gyro_config(gyr_bytes, &gyr);

depz_bno055_mag_config mag;
depz_bno055_unpack_mag_config(0x8B, &mag);    /* bit 7 is not a field ... */
uint8_t mag_byte = depz_bno055_pack_mag_config(&mag);     /* ... so this is 0x0B */

printf("acc 0x%02X  gyro range %u  mag 0x%02X\n", (unsigned)acc_byte,
       (unsigned)gyr.range, (unsigned)mag_byte);
```

Write them with the page-1 sequence from
[Registers, modes and pages](#registers-modes-and-pages): `PAGE_ID ← 1`, the
write, `PAGE_ID ← 0`. On these boards only the **motion** interrupts work
(any / no-motion, high-g, high-rate); the data-ready interrupt bits exist but
never fire on sensor firmware 03.11.

### Bridge info and reset by hand

```c
#include <stdio.h>

/* the answer to DEPZ_BNO055_CMD_GET_INFO (empty payload) */
void on_info(const depz_event *ev)
{
    depz_bno055_info info;
    if (ev->type != DEPZ_EV_PACKET || ev->cmd != DEPZ_BNO055_RPT_INFO
            || depz_bno055_unpack_info(ev->payload, ev->payload_len, &info) != 0)
        return;
    bool ids_ok = info.chip_id == 0xA0 && info.acc_id == 0xFB
               && info.mag_id == 0x32 && info.gyr_id == 0x0F;
    printf("ids %s, sensor firmware %02X.%02X, block read %u us avg, "
           "%u slots skipped, %u sensor resets\n",
           ids_ok ? "ok" : "WRONG", (unsigned)(info.sw_rev >> 8),
           (unsigned)(info.sw_rev & 0xFF), (unsigned)info.read_avg_us,
           (unsigned)info.slots_skipped, (unsigned)info.sensor_resets);
}
```

`BNO_RESET` pulses the sensor's reset pin and answers only after the chip-id
handshake (allow ≥ 1.5 s). The reply comes **before the sensor has finished
booting**: poll `SYS_STATUS` (`0x39`) until it leaves 2, 3 and 4
(initialising / self-test) before writing a mode, or the mode is lost. After
a reset the sensor is in CONFIG with power-on settings (`UNIT_SEL = 0x80`).
If `sensor_resets` grows during a long run, the bridge recovered a stuck bus
by resetting the sensor: the stream keeps running, but the configuration is
gone — write it again.

## Gotchas

- **Zero quaternion = not fusing** — CONFIG, a non-fusion mode, or the first
  ~70 ms after a switch out of CONFIG. `configure()` and
  `set_operation_mode()` wait that out; a raw `OPR_MODE` write does not.
- **Every CONFIG round trip restarts the fusion** — reading the calibration
  profile, changing units, remap, power mode or a page-1 config each cost
  another ~70 ms of zero fusion outputs.
- **No direct mode-to-mode switch** — the sensor ignores NDOF → AMG; the class
  goes through CONFIG. A raw register write does not.
- **Settings are silently ignored outside CONFIG** — use the typed setters
  (they go to CONFIG and back); a raw `UNIT_SEL` write in NDOF does nothing.
- **A stream keeps the units it started with** — change units with the stream
  stopped, or restart it.
- **Linear acceleration and gravity are always m/s²**, whatever `accel_mg`
  says.
- **Page 1 and `self_test()` are refused while streaming** (`DEPZ_E_ARG`) —
  stop the stream first.
- **After a reset — yours or the bridge's (`sensor_resets` rising) — the
  sensor is in CONFIG with power-on settings**: `restore_configuration()`.
- **Don't store an uncalibrated profile** — a magnetometer radius of 0 is
  outside the legal 144..1280 and makes the sensor report a fusion
  configuration error.
- **Page-1 sensor configs only matter in non-fusion modes** — the fusion
  overrides them.
- **Only motion interrupts fire** on sensor firmware 03.11; there is no
  data-ready interrupt, so the timer is the only way to stream data.
- **Don't benchmark in CONFIG** — the sensor stretches the bus 3–5× harder
  there; a 46-byte read takes ~10 ms instead of ~3 ms.
- **`INT_STA` (`0x37`) clears on read** — keep it out of your own block reads
  (the full block stops just before it).
- **Callbacks run on the reader thread** — keep them short, and never call
  `depz_device_close()` from one.
- **Decode layer:** latch the units before `START_STREAM` (the first sample
  can beat the command's reply), never switch pages while streaming, and scale
  linear acceleration and gravity with `DEPZ_BNO055_FUSION_ACCEL_LSB`, never
  with `depz_bno055_accel_lsb()`.
