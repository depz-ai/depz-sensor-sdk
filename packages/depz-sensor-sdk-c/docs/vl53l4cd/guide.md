# VL53L4CD — user guide

Hands-on guide to the VL53L4CD: the live sensor class of `depz_sensor_io.h`
first, then the codecs underneath for when you own the transport. For what the
sensor is and its concepts, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md). Devices, errors, threading rules and
streams in general are in the [common guide](../guide.md#live-layer).

## Contents

- [Open the sensor](#open-the-sensor)
- [Initialise](#initialise)
- [Configure](#configure)
- [Detection thresholds](#detection-thresholds)
- [Calibrate](#calibrate)
- [Single shot](#single-shot)
- [Ranging: streams and callbacks](#ranging-streams-and-callbacks)
- [The measurement](#the-measurement)
- [XSHUT, reset and bridge diagnostics](#xshut-reset-and-bridge-diagnostics)
- [Disconnects](#disconnects)
- [Record and replay for tests](#record-and-replay-for-tests)
- [Verified on hardware](#verified-on-hardware)
- [Decode layer (your own transport)](#decode-layer-your-own-transport)
- [Gotchas](#gotchas)

## Open the sensor

`depz_open_device()` finds the board, probes it and attaches the VL53L4CD
class when the firmware is `APP_VL53L4_*`. With no options it takes the DEPZ
board with the smallest USB serial; set `port`, `serial` or `index` to pick
one. Check `depz_is_vl53l4cd()` — the same call opens any DEPZ board, and a
VL53L4CD call on another type fails with `DEPZ_E_WRONG_TYPE`.

```c
#include <depz_sensor_io.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    depz_open_options opt = DEPZ_OPEN_OPTIONS_INIT;
    depz_device *dev;
    depz_vl53l4cd_measurement m;
    bool alive = false;
    char fw[64];

    if (argc > 1) opt.port = argv[1];          /* "/dev/ttyACM0", "COM7", ... */
    if (depz_open_device(&opt, &dev) != DEPZ_OK) {
        fprintf(stderr, "open: %s\n", depz_last_error());
        return 1;
    }
    if (!depz_is_vl53l4cd(dev)) {
        fprintf(stderr, "%s is not a VL53L4CD\n", depz_device_port(dev));
        depz_device_close(dev);
        return 1;
    }
    depz_device_get_software_name(dev, fw, sizeof fw);   /* "APP_VL53L4_v0.83" */
    depz_vl53l4cd_is_alive(dev, &alive);                 /* model id 0xEBAA */
    if (!alive || depz_vl53l4cd_init(dev, 0) != DEPZ_OK) {
        fprintf(stderr, "init: %s\n", depz_last_error());
        depz_device_close(dev);
        return 1;
    }
    if (depz_vl53l4cd_measure_once(dev, -1, &m) == DEPZ_OK)
        printf("%s: %d mm (%s)\n", fw, m.r.distance_mm,
               depz_vl53l4cd_status_text(m.r.range_status));
    depz_device_close(dev);
    return 0;
}
```

On a link of your own — a replay, a loopback fake — `depz_vl53l4cd_open_link(link,
&dev)` attaches the class without a probe (see
[record and replay](#record-and-replay-for-tests)). Either way the device owns
the link and `depz_device_close()` frees both.

## Initialise

```c
int rc = depz_vl53l4cd_init(dev, 0);   /* 0 = DEPZ_VL53L4CD_I2C_KHZ_DEFAULT (1 MHz) */
if (rc != DEPZ_OK)
    fprintf(stderr, "%s: %s\n", depz_err_str(rc), depz_last_error());
```

`depz_vl53l4cd_init()` is the ULD `sensor_init`, run from the host:

1. the bus goes to 400 kHz, the only speed an unconfigured sensor is specified
   for, and the SDK waits for the sensor's boot flag;
2. the 91-byte default configuration block is written in one transaction;
3. the bus is re-timed to `bus_khz` (0 = 1 MHz; pass 400 to stay there);
4. VHV calibration runs once, then the timing is set to 50 ms continuous.

There is no firmware download — the VL53L4CD carries its own — so this takes
about 60 ms. Call it before any configuration or measurement, and again after
every XSHUT power-cycle. `depz_vl53l4cd_initialized()` says whether it has run
since the last power-cycle; `depz_vl53l4cd_is_alive()` only reads the model id
and works at any time.

## Configure

All setters need `init()` first and are **refused while ranging** with
`DEPZ_E_ARG` ("stop ranging first"). Each has a getter that reads the value
back from the sensor.

```c
uint32_t budget_ms, inter_ms;
int32_t offset_mm;
uint16_t xtalk_kcps, signal_kcps, sigma_mm;

/* Timing: budget 10..200 ms; inter 0 = continuous, > budget = autonomous. */
depz_vl53l4cd_set_range_timing(dev, 50, 0);       /* back to back, ~20 Hz */
depz_vl53l4cd_set_range_timing(dev, 200, 1000);   /* one 200 ms shot per second */
depz_vl53l4cd_get_range_timing(dev, &budget_ms, &inter_ms);

/* Corrections. */
depz_vl53l4cd_set_offset_mm(dev, -7);             /* signed ranging offset */
depz_vl53l4cd_get_offset_mm(dev, &offset_mm);
depz_vl53l4cd_set_xtalk_kcps(dev, 12);            /* crosstalk; 0 = off */
depz_vl53l4cd_get_xtalk_kcps(dev, &xtalk_kcps);

/* Quality limits: measurements outside them get a non-zero range_status. */
depz_vl53l4cd_set_signal_threshold_kcps(dev, 1024);   /* reject weak returns */
depz_vl53l4cd_get_signal_threshold_kcps(dev, &signal_kcps);
depz_vl53l4cd_set_sigma_threshold_mm(dev, 15);        /* reject noisy ranges */
depz_vl53l4cd_get_sigma_threshold_mm(dev, &sigma_mm);

/* After a > 8 °C ambient change: re-run VHV calibration. */
depz_vl53l4cd_start_temperature_update(dev);
```

- **Timing** — an inter-measurement period between 1 and the budget is refused
  with `DEPZ_E_ARG`, as by the ULD. The getter returns what the registers
  encode, which can be a step off what you set: on the verified board a 33 ms
  budget reads back as 32 ms.
- **Sigma** above 16383 mm cannot be encoded and fails with `DEPZ_E_ARG`
  before anything is written. Signal thresholds are stored in steps of 8 kcps.
- **The bus speed** can be changed later with
  `depz_vl53l4cd_set_i2c_speed_khz()` (not while ranging); the bridge picks the
  nominal step nearest your value — `depz_vl53l4cd_bridge_info()` reads back
  the step in effect.

## Detection thresholds

The detection thresholds are a distance window that gates the sensor's INT
line — and with it the stream: only measurements that meet the condition are
reported.

```c
uint16_t low_mm, high_mm;
uint8_t window;

/* Report only targets between 100 and 300 mm. */
depz_vl53l4cd_set_detection_thresholds(dev, 100, 300, DEPZ_VL53L4CD_WINDOW_IN);
depz_vl53l4cd_get_detection_thresholds(dev, &low_mm, &high_mm, &window);
```

| window | INT fires when |
|---|---|
| `DEPZ_VL53L4CD_WINDOW_BELOW` (0) | distance < `low_mm` |
| `DEPZ_VL53L4CD_WINDOW_ABOVE` (1) | distance > `high_mm` |
| `DEPZ_VL53L4CD_WINDOW_OUT` (2) | distance < `low_mm` or > `high_mm` |
| `DEPZ_VL53L4CD_WINDOW_IN` (3) | `low_mm` ≤ distance ≤ `high_mm` |

**There is no "thresholds off" call.** `init()` leaves the window *disabled*
(`0x20` in `SYSTEM__INTERRUPT`: INT on every measurement); once you program a
window, only another `depz_vl53l4cd_init()` restores that state. In
particular, a 0/0 window with `WINDOW_BELOW` ("closer than 0 mm") never fires:
the stream goes silent and polled single shots time out. The getter reports
the low three bits, so a freshly initialised sensor reads back as
`WINDOW_BELOW` even though no window is active.

## Calibrate

Both calibrations need a target at a known distance, block while they range a
burst of samples (after a 10-sample warm-up), program the sensor, and return
the value now in effect:

```c
int32_t offset_mm;
uint16_t xtalk_kcps;

/* Target at 100 mm (10..1000); 0 samples = the default 20 (5..255). */
if (depz_vl53l4cd_calibrate_offset(dev, 100, 0, &offset_mm) == DEPZ_OK)
    printf("offset %d mm\n", (int)offset_mm);

/* Crosstalk from a cover glass: target at 600 mm (10..5000). */
if (depz_vl53l4cd_calibrate_xtalk(dev, 600, 0, &xtalk_kcps) == DEPZ_OK)
    printf("xtalk %u kcps\n", (unsigned)xtalk_kcps);
```

Crosstalk calibration discards invalid samples and the first frame, and fails
with `DEPZ_E_PROTOCOL` when no sample is valid or the result exceeds 127 kcps.
Neither value survives an XSHUT power-cycle: keep them and re-apply them with
`depz_vl53l4cd_set_offset_mm()` / `_set_xtalk_kcps()` after `init()`.

## Single shot

```c
depz_vl53l4cd_measurement m;
int rc = depz_vl53l4cd_measure_once(dev, -1, &m);   /* -1: the 1000 ms default */

if (rc == DEPZ_OK) {
    if (m.r.range_status == 0)
        printf("%d mm, sigma %d mm\n", m.r.distance_mm, m.r.sigma_mm);
    else
        printf("suspect: %s\n", depz_vl53l4cd_status_text(m.r.range_status));
} else if (rc == DEPZ_E_ARG && depz_vl53l4cd_ranging(dev)) {
    printf("ranging is running: stop it first\n");
} else {
    fprintf(stderr, "%s: %s\n", depz_err_str(rc), depz_last_error());
}
```

`measure_once` is the ULD poll loop: start the sensor, poll data-ready, read
the result block, clear the interrupt, stop. It waits one timing budget plus
the register round trips, so a negative timeout means **1000 ms**, not the
200 ms request default. If anything fails it still stops the sensor and
reports the first failure. `timestamp_us` is the MCU clock when the result
block was read.

## Ranging: streams and callbacks

`depz_vl53l4cd_start_ranging()` starts the sensor and arms the board's stream:
on each INT edge the MCU reads the 17-byte result block and pushes it, so there
is **one measurement per INT edge**, timestamped at the edge.
`depz_vl53l4cd_stop_ranging()` stops both (idempotent; the sensor is stopped
even if the stream stop fails). Measurements come out three ways — use any
mix:

**Streams** — pulled from your own thread, bounded, drop-oldest. The usual
choice:

```c
depz_vl53l4cd_measurement m;
depz_stream *s = depz_vl53l4cd_stream(dev, 64);   /* before start: nothing is missed */
uint64_t end;

depz_vl53l4cd_set_range_timing(dev, 33, 0);       /* ~30 Hz */
depz_vl53l4cd_start_ranging(dev);
end = depz_host_now_us() + 5000000u;              /* 5 s */
while (depz_host_now_us() < end && depz_stream_next(s, &m, 500) == DEPZ_OK) {
    if (m.r.range_status == 0) printf("%5d mm\n", m.r.distance_mm);
    else printf("  %s\n", depz_vl53l4cd_status_text(m.r.range_status));
}
depz_vl53l4cd_stop_ranging(dev);
printf("%llu dropped, %llu undecoded\n",
       (unsigned long long)depz_stream_dropped_count(s),
       (unsigned long long)depz_vl53l4cd_stream_parse_errors(dev));
depz_stream_close(s);
```

If the consumer falls behind, the oldest measurements are discarded and
`depz_stream_dropped_count()` counts them — the device is never stalled. Each
stream is independent; `maxsize == 0` picks the default of 64.
`depz_vl53l4cd_stream_parse_errors()` counts stream reports whose block did
not decode — 0 in a healthy session.

**Callbacks** — run on the device's reader thread for every measurement:

```c
static void on_measurement(const depz_vl53l4cd_measurement *m, void *user)
{
    /* reader thread: short, never block, never a device call here */
    int *nearest = (int *)user;
    if (m->r.range_status == 0 && m->r.distance_mm < *nearest)
        *nearest = m->r.distance_mm;
}

int nearest = 1 << 30;
int token;
depz_vl53l4cd_on_measurement(dev, on_measurement, &nearest, &token);
depz_vl53l4cd_start_ranging(dev);
/* ... */
depz_vl53l4cd_stop_ranging(dev);
depz_vl53l4cd_off_measurement(dev, token);
```

**One at a time** — `depz_vl53l4cd_get_measurement(dev, timeout_ms, &m)` waits
for the next streamed measurement (a negative timeout means 2000 ms). It
subscribes when called, so it only sees measurements that arrive after the
call; for a steady flow use a stream.

While ranging, every configuration call, `measure_once` and
`set_i2c_speed_khz` fail with `DEPZ_E_ARG` — the stream owns the register
bank. `depz_vl53l4cd_bridge_info()` and the getters still work.

## The measurement

`depz_vl53l4cd_measurement` is the MCU `timestamp_us` plus a
`depz_vl53l4_result`, the ULD's `VL53L4CD_ResultsData_t`:

| field | meaning |
|---|---|
| `timestamp_us` | MCU µs at the INT edge (stream) / the result read (single shot) |
| `r.range_status` | 0 = valid; `depz_vl53l4cd_status_text()` names the rest |
| `r.distance_mm` | measured distance, mm |
| `r.sigma_mm` | estimated standard deviation of the distance, mm |
| `r.signal_rate_kcps` / `r.signal_per_spad_kcps` | return-signal rate |
| `r.ambient_rate_kcps` / `r.ambient_per_spad_kcps` | ambient-light rate |
| `r.number_of_spad` | SPADs (light-detecting cells) used |
| `r.stream_count` | the sensor's frame counter, wraps at 255 |

Check `range_status` before trusting a distance: a non-zero status ("signal
below threshold", "wrapped target, phase mismatch", ...) is a legitimate
reading, not a protocol error. To put `timestamp_us` on the host clock, run
`depz_device_sync_time()` once and map it with `depz_device_to_host_time_us()`.

## XSHUT, reset and bridge diagnostics

The board drives the sensor's XSHUT (shutdown) pin:

```c
depz_vl53l4_info info;

depz_vl53l4cd_reset_sensor(dev);                   /* power-cycle */
depz_vl53l4cd_xshut(dev, DEPZ_VL53L4_XSHUT_OFF);   /* sensor off */
depz_vl53l4cd_xshut(dev, DEPZ_VL53L4_XSHUT_ON);    /* back on, in its boot state */
depz_vl53l4cd_init(dev, 0);                        /* required again */

depz_vl53l4cd_bridge_info(dev, &info);             /* safe while ranging */
printf("model 0x%04X, %u kHz, %u INT edges, %u skipped, %u I2C errors\n",
       info.model_id, info.i2c_khz, (unsigned)info.int_edges,
       (unsigned)info.slots_skipped, (unsigned)info.i2c_errors);
```

OFF and RESET stop any stream and wipe everything the ULD programmed —
timing, offset, crosstalk, thresholds: `depz_vl53l4cd_initialized()` and
`depz_vl53l4cd_ranging()` both go `false`. Call `init()` and reconfigure
before measuring again. The bridge counters are free-running and wrap
silently — watch increments, not absolute values.

For anything the class does not wrap, `depz_vl53l4cd_read_reg()` /
`depz_vl53l4cd_write_reg()` reach any register (16-bit address, contents
big-endian as the sensor has them; long transfers are split at the bridge's
253-byte limit). They bypass the class's state checks — don't use them while
ranging.

## Disconnects

When the cable goes, the reader thread notices and:

- requests that are waiting fail with `DEPZ_E_CLOSED`, and so does every
  later call on the device;
- streams **hand out what they already hold, then** return `DEPZ_E_CLOSED`;
- a `DEPZ_DEV_EV_DISCONNECTED` event fires (callback and event streams), with
  the reason in `text`; `depz_device_closed()` becomes `true`.

```c
int rc;
while ((rc = depz_stream_next(s, &m, 500)) != DEPZ_E_CLOSED) {
    if (rc == DEPZ_OK) { /* use m */ }
}
if (depz_device_closed(dev)) fprintf(stderr, "VL53L4CD lost\n");
depz_stream_close(s);
depz_device_close(dev);      /* still needed: frees the device */
```

There is no automatic reconnect: open the device again once it is back, and
`init()` it — the sensor may have lost power with the board.

## Record and replay for tests

Tee a live session into a `.depzrec` file, then replay it with no board
attached. Strict replay (`strict_tx = true`) also checks that your code sends
exactly the bytes it sent when recording, so a changed register sequence fails
loudly with `DEPZ_E_REPLAY_MISMATCH` instead of drifting.

```c
static void session(depz_device *dev, depz_vl53l4cd_measurement *m)
{
    depz_vl53l4cd_init(dev, 0);
    depz_vl53l4cd_set_range_timing(dev, 33, 0);
    depz_vl53l4cd_measure_once(dev, -1, m);
}

int record_and_replay(const char *port)
{
    depz_link *serial, *rec, *rp;
    depz_device *dev;
    depz_vl53l4cd_measurement live, again;

    /* 1. Record against the real board. */
    if (depz_link_open_serial(port, &serial) != DEPZ_OK) return -1;
    if (depz_link_open_recording(serial, "l4.depzrec", "\"note\":\"bench\"", &rec) != DEPZ_OK)
        return -1;                                 /* serial already freed */
    if (depz_vl53l4cd_open_link(rec, &dev) != DEPZ_OK) return -1;
    session(dev, &live);
    depz_device_close(dev);

    /* 2. Replay: same calls, same order, recorded answers. */
    if (depz_link_open_replay("l4.depzrec", true, false, &rp) != DEPZ_OK) return -1;
    if (depz_vl53l4cd_open_link(rp, &dev) != DEPZ_OK) return -1;
    session(dev, &again);                          /* again == live */
    depz_device_close(dev);
    return 0;
}
```

The ULD's poll loops (boot wait, data-ready) issue as many register reads as
the sensor needed on the day, and the replay serves exactly those answers — so
the replayed session takes the same path. `depz_vl53l4cd_open_link()` skips
the identity probe; to record the probe too (and then `depz_device_promote()`
in replay), open with `depz_device_open_link()` instead. That is how the test
suite replays the reference capture
`contracts/vectors/recordings/vl53l4cd_session.depzrec` (`io_l4_session_replay`:
init, timing, offset and crosstalk readback, two single shots, 20 streamed
frames). It was recorded by the Python SDK, and the Python, TypeScript, C and
C++ SDKs all replay it strictly — the four drivers issue the same bytes.

## Verified on hardware

The class was run against a real VL53L4CD (board `TL7TKSLW8Z`, firmware
`APP_VL53L4_v0.83`) facing a wall at about 1.05 m:

- discovery, the common commands and the model id `0xEBAA`;
- `init()` in 60 ms, the bus left at 1 MHz;
- timing, offset and threshold round trips, and the temperature update;
- single shots of ~1043 mm, all valid;
- the stream at 33 frames/s with a 33 ms budget, none dropped or undecoded;
- configuration refused with `DEPZ_E_ARG` while ranging.

The CTest suite covers the same behaviour without hardware: a fake bridge
firmware with a register map over a loopback link (`io_l4_*`) and the strict
replay of the real capture above.

## Decode layer (your own transport)

If you run your own serial code, the VL53L4 codecs of `depz_sensor_sdk.h` work
without the live layer — the class above is built from them. You then drive
the ULD sequences yourself over `READ_REG` / `WRITE_REG`. For the parser loop
these examples build on, see the common guide's
[mental model](../guide.md#mental-model).

### The codec surface

- `depz_vl53l4_cmd` — host→device opcodes (`READ_REG` `0x32`, `WRITE_REG`
  `0x33`, `XSHUT` `0x34`, `START_STREAM` `0x35`, `STOP_STREAM`, `GET_INFO`
  `0x37`, `SET_I2C_SPEED`).
- `depz_vl53l4_rpt` — device→host report ids (`REG_DATA` `0x91`, `INFO`
  `0x92`, `STREAM` `0x93`).
- `depz_vl53l4_pack_*` — payload encoders (they return the payload length
  written; `depz_build_packet()` then frames it).
- `depz_vl53l4_reg_data` / `depz_vl53l4_info` / `depz_vl53l4_stream` — decoded
  reports, filled by the matching `depz_vl53l4_unpack_*` (0 on success,
  negative on a short/bad payload).
- The pure ULD pieces — `depz_vl53l4_parse_result_block()`, the range-timing
  register math, the tuning word codecs and `depz_vl53l4_config_block()`.

### Read and write registers

```c
#include <depz_sensor_sdk.h>

void serial_write(const uint8_t *data, size_t len);   /* your transport */

static unsigned seq;

static void send(uint8_t cmd, const uint8_t *payload, size_t n)
{
    uint8_t frame[DEPZ_MAX_FRAME];
    size_t frame_len;
    if (depz_build_packet(cmd, payload, n, seq++, DEPZ_CRC8,
                          frame, sizeof frame, &frame_len) == 0)
        serial_write(frame, frame_len);
}

static void read_model_id_and_configure(void)
{
    uint8_t payload[2 + DEPZ_VL53L4_XFER_MAX];
    uint8_t cfg[91];
    size_t n;

    /* read the model id word (IDENTIFICATION__MODEL_ID 0x010F; expect 0xEBAA) */
    n = depz_vl53l4_pack_read_reg(0x010F, 2, payload);
    send(DEPZ_VL53L4_CMD_READ_REG, payload, n);

    /* write the 91-byte init configuration block in one transaction */
    depz_vl53l4_config_block(cfg);            /* ST defaults, byte 0 forced to 0x12 */
    n = depz_vl53l4_pack_write_reg(DEPZ_VL53L4_CONFIG_ADDR, cfg, sizeof cfg, payload);
    send(DEPZ_VL53L4_CMD_WRITE_REG, payload, n);
}

/* the reply is RPT_VL53_REG_DATA (0x91) — in your parser callback: */
static void on_event(const depz_event *ev, void *user)
{
    depz_vl53l4_reg_data d;
    (void)user;
    if (ev->type != DEPZ_EV_PACKET || ev->cmd != DEPZ_VL53L4_RPT_REG_DATA) return;
    if (depz_vl53l4_unpack_reg_data(ev->payload, ev->payload_len, &d) == 0 &&
        d.data_len >= 2) {
        /* register contents are big-endian sensor bytes */
        uint16_t model = (uint16_t)((d.data[0] << 8) | d.data[1]);
        (void)model;                           /* == DEPZ_VL53L4_MODEL_ID */
    }
}
```

Reads and writes are capped at `DEPZ_VL53L4_XFER_MAX` (253) bytes and
`addr + len` ≤ 0x10000 — `depz_vl53l4_pack_write_reg()` returns 0 for an
out-of-range `data_len`. Register contents are big-endian; the wire fields
(`addr`, `len`, timestamps) are little-endian.

### Stream the result block

```c
static void arm_stream(void)
{
    /* INT-driven streaming of the 17-byte result block at 0x0089 */
    uint8_t payload[5];
    size_t n = depz_vl53l4_pack_start_stream(DEPZ_VL53L4_RESULT_BLOCK_ADDR,
                                             DEPZ_VL53L4_RESULT_BLOCK_LEN, 0, payload);
    send(DEPZ_VL53L4_CMD_START_STREAM, payload, n);
}

/* each sample arrives as RPT_VL53_STREAM (0x93) */
static void on_stream(const depz_event *ev)
{
    depz_vl53l4_stream s;
    depz_vl53l4_result r;
    if (ev->cmd == DEPZ_VL53L4_RPT_STREAM &&
        depz_vl53l4_unpack_stream(ev->payload, ev->payload_len, &s) == 0 &&
        depz_vl53l4_parse_result_block(s.data, s.len, &r) == 0 &&
        r.range_status == 0)                    /* 0 = valid */
        printf("%d mm, sigma %d mm at %llu us\n", r.distance_mm, r.sigma_mm,
               (unsigned long long)s.timestamp_us);
}
```

The `flags` bit `DEPZ_VL53L4_SF_INT_ACT_HIGH` (`0x02`) selects
INT-active-high; the default (0) matches the ULD init block (INT active low).
`timestamp_us` is MCU uptime at the **INT edge** — the sensor event, not the
I2C completion. Each stream report echoes `addr`/`len`, so it is
self-describing. The streaming FSM only reads; starting the sensor
(`SYSTEM_START`) is a register write of yours, as in the ULD.

`depz_vl53l4_parse_result_block()` decodes the block exactly as
`VL53L4CD_GetResult()`: status via the `STATUS_RTN` table (raw ≥ 24 passes
through unmapped), rates ×8 kcps, sigma ÷4 mm; it returns `-1` when
`len < 15`.

### Range timing

The SetRangeTiming/GetRangeTiming register math is pure and bit-exact:

```c
/* osc_frequency is register 0x0006; clock_pll is RESULT__OSC_CALIBRATE_VAL
 * (0x00DE, only used in autonomous mode) — both read from the sensor. */
static void timing(uint16_t osc_frequency, uint16_t clock_pll)
{
    uint16_t range_config_a, range_config_b;
    uint32_t inter_raw, budget_ms, inter_ms;

    /* SetRangeTiming(50 ms budget, continuous) */
    if (depz_vl53l4_range_timing_registers(50, 0, osc_frequency, clock_pll,
                                           &range_config_a, &range_config_b,
                                           &inter_raw) == 0) {
        /* write range_config_a -> RANGE_CONFIG_A (0x005E),
         *       range_config_b -> RANGE_CONFIG_B (0x0061),
         *       inter_raw      -> INTERMEASUREMENT_MS (0x006C), via WRITE_REG */
    }

    /* GetRangeTiming readback from the raw register reads */
    depz_vl53l4_decode_range_timing(inter_raw, clock_pll, osc_frequency,
                                    range_config_a, &budget_ms, &inter_ms);
}
```

Budget is 10..200 ms. `inter_ms == 0` selects continuous mode; a value
**greater** than the budget selects autonomous low power; anything else (or
`osc_frequency == 0`) returns `-1`.

### Tuning words

Each tuning register is a word codec pair (encode for the write, decode for
the readback):

```c
static void tuning(void)
{
    uint16_t word = depz_vl53l4_offset_raw(-10);      /* RANGE_OFFSET_MM (0x001E) */
    int32_t  mm   = depz_vl53l4_decode_offset(word);  /* -> -10 */
    uint16_t raw;

    depz_vl53l4_xtalk_raw(20);                /* XTALK_PLANE_OFFSET_KCPS (0x0016) */
    depz_vl53l4_signal_threshold_raw(1024);   /* MIN_COUNT_RATE_RTN_LIMIT_MCPS (0x0066) */
    depz_vl53l4_sigma_threshold_raw(15, &raw); /* RANGE_CONFIG__SIGMA_THRESH (0x0064) */
    (void)mm;
}
```

`depz_vl53l4_sigma_threshold_raw()` returns `-1` when `mm > 16383` (the word
overflows); the other encoders are total.

## Gotchas

- **`init()` first, and after every power-cycle.** XSHUT OFF / RESET wipe the
  ULD configuration; `depz_vl53l4cd_initialized()` tracks it.
- **Configuration while ranging fails with `DEPZ_E_ARG`** — so do
  `measure_once` and `set_i2c_speed_khz`. The stream owns the register bank:
  stop, reconfigure, restart.
- **Detection thresholds cannot be switched off** except by `init()`; a 0/0
  `WINDOW_BELOW` window silences INT — and the stream — for good.
- **Always check `range_status`.** Non-zero values are data, not errors.
- **Inter-measurement must be 0 or greater than the budget**; values in
  between are rejected, as by the ULD.
- **The first streamed frame can be stale.** In the reference capture the
  first frame after `start_ranging()`, 3 ms after a single shot, repeats that
  shot's result exactly (`stream_count` 0 both times); `stream_count` then
  counts up. Drop the first frame if that matters.
- **Callbacks run on the reader thread** — keep them short, don't block, don't
  make device calls or close the device from one. Prefer a stream for real
  work.
- **A full stream drops the oldest measurement**, it never blocks the device;
  watch `depz_stream_dropped_count()`, and the bridge's `slots_skipped` for
  INT edges the MCU itself could not service.
- **Two endiannesses on purpose** (decode layer) — wire fields little-endian,
  register contents big-endian. The codecs handle both; don't swap bytes
  yourself.
