# BNO086 — user guide

Hands-on guide to the BNO085 / BNO086 in C: the sensor class
(`depz_bno086_*` in `depz_sensor_io.h`) first, then the codecs underneath it
for when you own the transport. For what the sensor is and its concepts, read
the [introduction](introduction.md); for exact signatures see the
[API reference](api.md).

## Contents

- [Open and identify](#open-and-identify)
- [Enable outputs and read reports](#enable-outputs-and-read-reports)
- [The report and its units](#the-report-and-its-units)
- [Callbacks](#callbacks)
- [Choosing outputs and rates](#choosing-outputs-and-rates)
- [Calibration, tare and reorientation](#calibration-tare-and-reorientation)
- [FRS records and sensor metadata](#frs-records-and-sensor-metadata)
- [Diagnostics](#diagnostics)
- [Errors, busy and timeouts](#errors-busy-and-timeouts)
- [Record and replay for tests](#record-and-replay-for-tests)
- [Verification status](#verification-status)
- [Decode layer (your own transport)](#decode-layer-your-own-transport)
- [Gotchas](#gotchas)

## Open and identify

`depz_open_device()` finds the board, asks its firmware name
(`APP_BNO086_*`) and attaches the class: `depz_is_bno086()` is then true. The
same class serves the BNO085 and the BNO086; the chip's product id tells
which one is on the board.

```c
#include <depz_sensor_io.h>
#include <stdio.h>

int main(void)
{
    depz_device *dev;
    depz_bno_product_id pid;
    if (depz_open_device(NULL, &dev) != DEPZ_OK) {
        fprintf(stderr, "open: %s\n", depz_last_error());
        return 1;
    }
    if (!depz_is_bno086(dev)) {
        fprintf(stderr, "%s is not a BNO085 / BNO086\n", depz_device_port(dev));
        depz_device_close(dev);
        return 1;
    }
    depz_bno086_hardware_reset(dev, -1);  /* a clean start: nothing enabled */
    depz_bno086_product_id(dev, &pid);
    printf("%s, SH-2 %u.%u.%u build %u\n",
           pid.sw_part_number == 10004148u ? "BNO085" : "BNO086",
           pid.sw_version_major, pid.sw_version_minor, pid.sw_version_patch,
           pid.sw_build_number);
    depz_device_close(dev);
    return 0;
}
```

`depz_bno086_hardware_reset()` pulses the sensor's reset pin: every output
is off afterwards and the SHTP sequence counters restart. It is optional — a
board you just plugged in is already fresh — but it makes a session start
from a known state. It waits up to 0.5 s for the chip's "reset complete"
message and does not fail without it (firmware up to v0.95 never sent one,
ERRATA E9).

## Enable outputs and read reports

Nothing streams until you enable an output with a rate. `enable()` sends the
request, then asks the hub which rate it actually granted:

```c
#include <depz_sensor_io.h>
#include <stdio.h>

static void stream_orientation(depz_device *dev)
{
    depz_bno_feature granted;
    depz_bno_report r;
    depz_stream *s = depz_bno086_reports(dev, 256);  /* subscribe first */
    int i;
    depz_bno086_enable(dev, DEPZ_BNO_SENSOR_ROTATION_VECTOR, 100, &granted);
    depz_bno086_enable(dev, DEPZ_BNO_SENSOR_ACCELEROMETER, 50, NULL);
    printf("rotation vector every %u us\n", granted.interval_us);
    for (i = 0; i < 500 && depz_stream_next(s, &r, 1000) == DEPZ_OK; i++) {
        if (r.sensor_id == DEPZ_BNO_SENSOR_ROTATION_VECTOR) {
            double q[4];
            depz_bno_report_quaternion(&r, q);
            printf("i %+.3f j %+.3f k %+.3f real %+.3f  accuracy %d\n",
                   q[0], q[1], q[2], q[3], r.accuracy);
        } else if (r.sensor_id == DEPZ_BNO_SENSOR_ACCELEROMETER) {
            double a[3];
            depz_bno_report_xyz(&r, a);
            printf("accel %+.2f %+.2f %+.2f m/s2\n", a[0], a[1], a[2]);
        }
    }
    depz_bno086_disable(dev, DEPZ_BNO_SENSOR_ROTATION_VECTOR);
    depz_bno086_disable(dev, DEPZ_BNO_SENSOR_ACCELEROMETER);
    depz_stream_close(s);
}
```

- **One stream carries every output** — reports of all enabled sensors
  arrive interleaved; tell them apart by `r.sensor_id`.
- **Subscribe before you enable**: a stream only sees reports produced after
  it was created. `depz_bno086_get_report(dev, timeout, &r)` returns the next
  report without a stream of your own, but it subscribes when called, so it
  can miss one that was already on its way.
- **`granted` is what the hub does**, not what you asked: it runs on a
  1000 / 2^n Hz grid and rounds (50 Hz becomes 62.5 Hz, interval 16000 µs).
  `depz_bno086_rate_ok(requested_interval_us, &granted)` tells whether that
  is within the normal 0.9–2.1× band. Pass NULL when you do not care.
- **The hub answers "0" at first.** Right after enabling, the lab BNO085
  answers the rate question with "interval 0" once or twice before it reports
  the real rate; `enable()` asks again (up to five times) while the answer is
  0, so `granted` holds the real rate.
- `depz_bno086_enable_ex()` takes every Set Feature field
  (`depz_bno086_feature_request`: interval, batch interval, change
  sensitivity, flags, sensor-specific word); pass `granted` NULL there to skip
  the read-back. `depz_bno086_get_feature()` asks the rate of any sensor.

The example program `examples/bno086_minimal.c` turns the quaternion into
heading / pitch / roll and prints the rate each output really arrived at.

## The report and its units

A `depz_bno_report` keeps the **wire integers** (`x_raw`, `i_raw`, ...) — they
are authoritative and never rounded. The helpers scale them (`value = raw /
2^Q`, the Q point is fixed per sensor):

| Sensor (`sensor_id`) | Report `type` | Helper | Units |
|---|---|---|---|
| Accelerometer (0x01), linear acceleration (0x04), gravity (0x06) | `DEPZ_BNO_ACCELERATION` | `depz_bno_report_xyz` | m/s² (Q8) |
| Gyroscope (0x02) | `DEPZ_BNO_GYROSCOPE` | `depz_bno_report_xyz` | rad/s (Q9) |
| Magnetometer (0x03) | `DEPZ_BNO_MAGNETOMETER` | `depz_bno_report_xyz` | µT (Q4) |
| Uncalibrated gyroscope / magnetometer (0x07 / 0x0F) | `DEPZ_BNO_UNCAL_*` | `_xyz`, `_bias` | as calibrated |
| Rotation vector (0x05), geomagnetic RV (0x09), AR/VR RV (0x28) | `DEPZ_BNO_ROTATION_VECTOR` | `_quaternion`, `_accuracy_rad` | unit quaternion (Q14), accuracy rad (Q12) |
| Game RV (0x08), AR/VR game RV (0x29) | `DEPZ_BNO_ROTATION_VECTOR` | `_quaternion` | unit quaternion (Q14), no accuracy |
| Gyro-integrated RV (0x2A) | `DEPZ_BNO_GYRO_INTEGRATED_RV` | `_quaternion`, `_angular_velocity` | Q14, rad/s (Q10) |
| Raw accelerometer / gyroscope / magnetometer (0x14–0x16) | `DEPZ_BNO_RAW_SENSOR` | `depz_bno_report_xyz` | ADC counts |
| Step counter, tap, shake, stability, activity ... | detector types | — | fields as named |

The quaternion order is **i, j, k, real** (x, y, z, w). Every report also
carries `timestamp_us` (MCU microseconds: the bridge's capture time corrected
by the report's own delay), `seq` (a rolling counter — a gap means a lost
report) and `accuracy` (0 unreliable … 3 high; for the rotation vector it
follows the magnetometer calibration). `accuracy` and `accuracy_raw` are
different things: the first is a 0–3 level, the second an angle.

Heading, pitch and roll from the quaternion:

```c
#include <math.h>
#include <depz_sensor_io.h>

static void euler_deg(const depz_bno_report *r, double *heading, double *pitch, double *roll)
{
    double q[4], x, y, z, w, sp;
    depz_bno_report_quaternion(r, q);
    x = q[0]; y = q[1]; z = q[2]; w = q[3];
    sp = 2 * (w * y - z * x);
    *heading = atan2(2 * (w * z + x * y), 1 - 2 * (y * y + z * z)) * 57.29577951;
    *pitch = fabs(sp) >= 1 ? copysign(90.0, sp) : asin(sp) * 57.29577951;
    *roll = atan2(2 * (w * x + y * z), 1 - 2 * (x * x + y * y)) * 57.29577951;
}
```

## Callbacks

Instead of pulling a stream, register a callback; it runs on the device's
reader thread for every report:

```c
#include <depz_sensor_io.h>
#include <stdio.h>

static void on_report(const depz_bno_report *r, void *user)
{
    (void)user;
    if (r->sensor_id == DEPZ_BNO_SENSOR_GAME_ROTATION_VECTOR)
        printf("seq %u at %lld us\n", r->seq, (long long)r->timestamp_us);
}

static void with_callback(depz_device *dev)
{
    int token;
    depz_bno086_on_report(dev, on_report, NULL, &token);
    depz_bno086_enable(dev, DEPZ_BNO_SENSOR_GAME_ROTATION_VECTOR, 200, NULL);
    /* ... */
    depz_bno086_off_report(dev, token);
}
```

Keep callbacks short and never call a blocking `depz_bno086_*` function from
one: the answer it would wait for arrives on the same thread.

## Choosing outputs and rates

- **Rotation vector** (0x05) — absolute orientation (magnetic north), up to
  400 Hz. Needs the magnetometer: heading accuracy starts poor (±180°) and
  improves as the magnetometer calibrates — move the board around.
- **Game rotation vector** (0x08) — accelerometer + gyroscope only: no
  magnetic disturbance, but the heading drifts slowly. Best for fast relative
  motion.
- **Geomagnetic rotation vector** (0x09) — accelerometer + magnetometer, low
  power, up to 90 Hz.
- **Gyro-integrated RV** (0x2A) — up to 1 kHz on its own SHTP channel, with
  angular velocity; for low-latency head tracking.
- **Gravity** (0x06) and **linear acceleration** (0x04) — the accelerometer
  split into the pull of gravity and your own motion.
- Raw sensors, detectors (tap, shake, step, stability, activity) — the full
  list with rates and units is in contract 05 §4
  (`contracts/05_SENSOR_BNO086.md`).

Disable what you do not read: the bridge forwards every report over USB, one
packet each — a rotation vector at 400 Hz is about 17 kB/s.

## Calibration, tare and reorientation

The hub calibrates the accelerometer, gyroscope and magnetometer by itself
while it runs ("motion engine calibration"). You choose which ones may adapt,
save the result to the chip's flash, and reset it:

```c
#include <depz_sensor_io.h>
#include <stdio.h>

static void calibration(depz_device *dev)
{
    depz_bno086_calibration c;
    depz_bno086_set_calibration(dev, true, true, true, false);  /* accel, gyro, mag, planar */
    depz_bno086_get_calibration(dev, &c);
    printf("adapting: accel %d gyro %d mag %d\n", c.accel, c.gyro, c.mag);
    /* ... move the board: figure eights for the magnetometer ... */
    depz_bno086_save_dcd(dev);                   /* keep it across power cycles */
    depz_bno086_configure_periodic_dcd(dev, true);  /* or let the hub save it */
}
```

"DCD" is the dynamic calibration data. `depz_bno086_clear_dcd_and_reset()`
throws it away and resets the sensor (waits for the reset to finish).

**Tare** makes the current orientation the zero: `depz_bno086_tare_now(dev,
DEPZ_BNO_TARE_ALL, DEPZ_BNO_TARE_BASIS_RV)` (or only `DEPZ_BNO_TARE_Z` for
heading). It lasts until reset unless you call `depz_bno086_persist_tare()`.
`depz_bno086_set_reorientation(dev, x, y, z, w)` sets a fixed mounting
rotation (a unit quaternion; all zeros clears it) — for a board mounted
rotated in your device. None of these three has an SH-2 answer: they return
once the bridge took the frame.

## FRS records and sensor metadata

The FRS ("flash record system") is the chip's small store of configuration
records, 32-bit words each. `depz_bno086_frs_read()` / `_frs_write()` access
any record by id (`DEPZ_BNO_FRS_*`); writing zero words erases one.

```c
#include <depz_sensor_io.h>
#include <stdio.h>

static void metadata(depz_device *dev)
{
    depz_bno_metadata m;
    uint32_t q[4];
    size_t n;
    depz_bno086_get_metadata(dev, DEPZ_BNO_SENSOR_ACCELEROMETER, &m);
    printf("accelerometer: %.2f mA, fastest every %u us, Q%u\n",
           m.power_ma_q10 / 1024.0, m.min_period_us, m.q_point_1);
    if (depz_bno086_frs_read(dev, DEPZ_BNO_FRS_SYSTEM_ORIENTATION, q, 4, &n) == DEPZ_OK)
        printf("system orientation: %zu words\n", n);  /* empty record: DEPZ_E_STATUS */
}
```

`depz_bno086_get_metadata()` reads a sensor's metadata record: supply
current, fastest and slowest period, range, resolution, Q points.
`frs_read()` sets `*n` to the record's full length and fails with
`DEPZ_E_ARG` when it did not fit `cap`; an empty or unknown record is an SH-2
refusal (`DEPZ_E_STATUS`).

## Diagnostics

- `depz_bno086_get_errors(dev, severity, out, cap, &n)` — the hub's error
  queue (`severity` 0 = all).
- `depz_bno086_get_counts(dev, sensor, &c)` — per-sensor event counts:
  offered, accepted, on, attempted. `depz_bno086_clear_counts()` zeroes them.
- `depz_bno086_get_oscillator_type()` — internal, crystal or external clock.
- `depz_bno086_wake()` — pulse the WAKE pin (no state lost).
- `depz_bno086_advertisement()` — the SHTP channel-0 advertisement seen since
  open / reset (empty on current firmware).
- `depz_bno086_shtp_discarded()` — incomplete SHTP messages thrown away; it
  stays 0 on a healthy link.
- Escape hatches: `depz_bno086_command()` sends any SH-2 command (optionally
  waiting for its answer), `depz_bno086_send_shtp()` any SHTP message.

## Errors, busy and timeouts

Functions return `DEPZ_OK` or a negative `depz_err`; `depz_last_error()` has
the text.

- **Busy.** The bridge holds two outgoing SHTP messages at a time; a third
  gets `ERR_BUSY`. The class then waits 200 ms and sends the same frame again,
  up to five times (`DEPZ_BNO086_BUSY_RETRIES`), and only then returns
  `DEPZ_E_BUSY`. It happens in normal use — the lab capture has one, on the
  third of three back-to-back disables.
- **SH-2 refusals** (a non-zero status in a command or FRS answer) return
  `DEPZ_E_STATUS`; the SH-2 status is in the message.
- **Timeouts** — answers are waited for at least 1 s (FRS 2 s), or the
  device timeout when that is longer: `DEPZ_E_TIMEOUT`.
- `DEPZ_E_WRONG_TYPE` — a `depz_bno086_*` call on a device of another type.

## Record and replay for tests

Record a live session and replay it without the board; with strict replay
every byte the SDK sends is compared with the recording:

```c
#include <depz_sensor_io.h>

static int replay(const char *path)
{
    depz_link *link;
    depz_device *dev;
    depz_bno_product_id pid;
    int rc = depz_link_open_replay(path, true, false, &link);
    if (rc) return rc;
    rc = depz_device_open_link(link, &dev);  /* no probe: promote below */
    if (rc) return rc;
    rc = depz_device_promote(dev);
    if (!rc) rc = depz_bno086_hardware_reset(dev, -1);
    if (!rc) rc = depz_bno086_product_id(dev, &pid);
    depz_device_close(dev);
    return rc;
}
```

`contracts/vectors/recordings/bno086_session.depzrec` was captured by the
Python SDK on the lab BNO085; `tests/test_io.c` (`bno086_session_replay`)
replays the whole session strictly — reset, product id, three enables with
their read-backs (the early zeros included), 150 reports, the disables (one
answered busy and resent), calibration, metadata, counts, errors. The C and
Python SDKs send the same requests in the same order, so a capture from
either replays in the other. `tests/fake_bno086.c` is an in-process fake hub
for the paths a capture does not cover.

## Verification status

- **Live** on the lab BNO085 (I5MFL1ONMCD, `APP_BNO086_v0.99`, SH-2 3.2.13),
  from C and C++: rotation vector 100.0 Hz, game rotation vector 201 Hz,
  gravity 50 Hz with |g| = 9.85 m/s², unit quaternions, gyroscope at rest
  below 0.02 rad/s, no reports dropped, product id, metadata, errors.
- **Replay** of the Python capture, strict, in C and C++ (and in Python and
  TS).
- A BNO086 (part 10004563) was not run from C: it answers the same SH-2
  protocol — the Python SDK drives both with one class.

## Decode layer (your own transport)

Everything above is built from codecs in `depz_sensor_sdk.h`; use them
directly if you run the DEPZ transport yourself. The stack, from the wire up:

```
DEPZ transport (A5 C3 framing)       <- depz_build_packet / depz_parser_feed
        │  0x34 SEND_SHTP_PACKET out; 0x91 RPT_DATA in (cmd, capture µs, frame)
        ▼
SHTP framing (6 channels, cargos)    <- depz_shtp_next_frame / depz_shtp_feed
        │  cargo = SH-2 message(s)
        ▼
SH-2 (features, reports, commands)   <- depz_bno_pack_* / depz_bno_unpack_* /
                                        depz_bno_parse_input_cargo
```

### Send a request

```c
#include <depz_sensor_sdk.h>

/* Enable the rotation vector at 100 Hz: SH-2 message -> SHTP frame -> packet. */
static size_t enable_rv(depz_shtp_layer *shtp, uint8_t seq, uint8_t *out, size_t cap)
{
    uint8_t sh2[17], frame[DEPZ_SHTP_MAX_TX_FRAME];
    size_t n, fn, pn = 0;
    n = depz_bno_pack_set_feature(DEPZ_BNO_SENSOR_ROTATION_VECTOR, 0, 0, 10000, 0, 0, sh2);
    fn = depz_shtp_next_frame(shtp, DEPZ_SHTP_CH_CONTROL, sh2, n, frame, sizeof frame);
    if (!fn || depz_build_packet(DEPZ_BNO086_CMD_SEND_SHTP_PACKET, frame, fn, seq,
                                 DEPZ_CRC_NONE, out, cap, &pn) != 0)
        return 0;
    return pn;  /* write out[0..pn) to the port */
}
```

The bridge answers every `SEND_SHTP_PACKET` at once with `RPT_STATUS`: OK,
or `ERR_BUSY` — then the frame was dropped; send the same frame again after
at least 200 ms. The SH-2 answer comes later, as ordinary inbound data.

### Receive

Every inbound SHTP frame arrives as `RPT_DATA`. Unpack it, feed the frame to
the SHTP layer, and act on complete cargos by channel:

```c
#include <depz_sensor_sdk.h>
#include <stdio.h>

static depz_shtp_layer g_shtp;  /* depz_shtp_init(&g_shtp) once */

static void on_event(const depz_event *ev, void *user)
{
    const uint8_t *frame;
    size_t frame_len, n, i;
    uint64_t capture;
    depz_shtp_cargo cargo;
    depz_bno_report reps[32];
    depz_bno_feature f;
    (void)user;
    if (ev->type != DEPZ_EV_PACKET || ev->cmd != DEPZ_BNO086_RPT_DATA) return;
    if (depz_bno086_unpack_data(ev->payload, ev->payload_len, &capture, &frame, &frame_len)) return;
    if (depz_shtp_feed(&g_shtp, frame, frame_len, &cargo) != 1) return;
    switch (cargo.channel) {
    case DEPZ_SHTP_CH_CONTROL:  /* answers: match them on their content */
        if (depz_bno_unpack_feature_response(cargo.payload, cargo.payload_len, &f) == 0)
            printf("sensor 0x%02X every %u us\n", f.sensor_id, f.interval_us);
        break;
    case DEPZ_SHTP_CH_INPUT_NORMAL:
    case DEPZ_SHTP_CH_INPUT_WAKE:
        n = depz_bno_parse_input_cargo(cargo.payload, cargo.payload_len, capture, reps, 32);
        for (i = 0; i < n; i++) printf("%s\n", depz_bno_report_type_str(reps[i].type));
        break;
    case DEPZ_SHTP_CH_GYRO_RV:
        if (depz_bno_parse_gyro_rv(cargo.payload, cargo.payload_len, capture, &reps[0]) == 0)
            printf("gyro-integrated RV\n");
        break;
    default:
        break;
    }
}
```

- `depz_bno_parse_input_cargo()` handles the 0xFB base timestamp and the 0xFA
  rebase and folds each report's delay into `timestamp_us`; an unknown report
  id ends the cargo with a `DEPZ_BNO_UNKNOWN_REPORT`. Size `reps` for the
  cargo: at most `payload_len / 5 + 1` reports.
- The control answers have parsers too: `depz_bno_unpack_product_id`,
  `_command_response`, `_frs_read_response`, `_frs_write_response`, and
  `depz_bno_metadata_from_words()` for a metadata record.
- There is no request id anywhere: a Get Feature answer carries only the
  sensor id, a command answer the command and its sequence number. The class
  matches answers exactly this way.
- `DEPZ_BNO_SENSOR_*` are the SH-2 sensor ids (what Set Feature takes and
  `sensor_id` holds); `depz_bno_report_type` (`DEPZ_BNO_ROTATION_VECTOR`, ...)
  is the parser's catalog of report shapes — a different numbering.

## Gotchas

- **Nothing streams until you enable it**, and a reset turns everything off.
- **Subscribe before you enable** — a stream misses what came before it.
- **The granted rate is rounded** to the hub's 1000 / 2^n Hz grid, and the
  first read-backs after enabling may say 0 — `enable()` handles it; code on
  the decode layer must ask again itself.
- **One stream, many sensors** — filter on `sensor_id`.
- **Rotation-vector heading needs a calibrated magnetometer**: until then
  `accuracy_rad` is large (the lab board started at ±180°, then ±46°). Use
  the game rotation vector when magnetic north does not matter.
- **Never block in a callback** — answers arrive on the reader thread.
- **Two outgoing messages at a time** — busy is normal under bursts; the
  class retries, your own transport must too.
- **Tare, persist tare, reorientation and periodic DCD have no answer** from
  the hub; success means the bridge took the frame.
