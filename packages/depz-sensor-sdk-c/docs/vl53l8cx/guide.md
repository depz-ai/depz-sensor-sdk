# VL53L8CX — user guide

Hands-on guide to the VL53L8CX: the live sensor class of `depz_sensor_io.h`
first, then the codecs underneath for when you own the transport. For what the
sensor is and its concepts, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md). Devices, errors, threading rules and
streams in general are in the [common guide](../guide.md#live-layer). The
**VL53L8CH** runs the same class and adds CNH histograms — its
[own guide](../vl53l8ch/guide.md) covers only that addition.

## Contents

- [Open the sensor](#open-the-sensor)
- [Initialise](#initialise)
- [Configure](#configure)
- [Ranging: streams and callbacks](#ranging-streams-and-callbacks)
- [The frame](#the-frame)
- [Detection thresholds](#detection-thresholds)
- [Motion indicator](#motion-indicator)
- [Crosstalk: margin, calibration, save and restore](#crosstalk-margin-calibration-save-and-restore)
- [Power modes](#power-modes)
- [Registers and DCI](#registers-and-dci)
- [Disconnects](#disconnects)
- [Record and replay for tests](#record-and-replay-for-tests)
- [Verified on hardware](#verified-on-hardware)
- [Decode layer (your own transport)](#decode-layer-your-own-transport)
- [Gotchas](#gotchas)

## Open the sensor

`depz_open_device()` finds the board, probes it and attaches the VL53L8 class
when the firmware is `APP_VL53L8_*`. With no options it takes the DEPZ board
with the smallest USB serial; set `port`, `serial` or `index` to pick one.
Check `depz_is_vl53l8()` — the same call opens any DEPZ board, and a VL53L8
call on another type fails with `DEPZ_E_WRONG_TYPE`.

```c
#include <depz_sensor_io.h>
#include <stdio.h>

static void on_progress(const char *phase, size_t done, size_t total, void *user)
{
    (void)user;
    if (total) fprintf(stderr, "\r  %s %zu / %zu bytes", phase, done, total);
    else fprintf(stderr, "\n%s", phase);
}

int main(int argc, char **argv)
{
    static depz_vl53l8_live_frame f;               /* ~7.5 KB: not on the stack */
    depz_open_options opt = DEPZ_OPEN_OPTIONS_INIT;
    depz_device *dev;
    bool alive = false;

    if (argc > 1) opt.port = argv[1];              /* "/dev/ttyACM0", "COM7", ... */
    if (depz_open_device(&opt, &dev) != DEPZ_OK) {
        fprintf(stderr, "open: %s\n", depz_last_error());
        return 1;
    }
    if (!depz_is_vl53l8(dev)) {
        fprintf(stderr, "%s is not a VL53L8\n", depz_device_port(dev));
        depz_device_close(dev);
        return 1;
    }
    printf("model %s\n", depz_vl53l8_get_model(dev) == DEPZ_VL53L8_MODEL_L8CH ? "L8CH" : "L8CX");
    depz_vl53l8_is_alive(dev, &alive, NULL, NULL);            /* 0xF0 / 0x0C */
    if (!alive || depz_vl53l8_init(dev, on_progress, NULL) != DEPZ_OK ||
        depz_vl53l8_set_resolution(dev, DEPZ_VL53L8_RES_8X8) != DEPZ_OK ||
        depz_vl53l8_set_ranging_frequency_hz(dev, 15) != DEPZ_OK ||
        depz_vl53l8_start_ranging(dev) != DEPZ_OK) {
        fprintf(stderr, "\n%s\n", depz_last_error());
        depz_device_close(dev);
        return 1;
    }
    if (depz_vl53l8_get_frame(dev, -1, &f) == DEPZ_OK)        /* -1: 2000 ms */
        printf("\nzone 27: %d mm (status %u), %d C\n", f.f.distance_mm[27],
               f.f.target_status[27], f.f.silicon_temp_degc);
    depz_vl53l8_stop_ranging(dev);
    depz_device_close(dev);
    return 0;
}
```

**Which model you get.** The VL53L8CX and VL53L8CH run the same bridge
firmware and report the same `VL53L8` identity; only the USB PID tells them
apart. `depz_open_device()` picks `DEPZ_VL53L8_MODEL_L8CH` for the production
PID `0x0483:0xED40` and `DEPZ_VL53L8_MODEL_L8CX` otherwise (the VL53L8CX has
no production PID yet and enumerates on the dev default `0x56DC`). A device
opened without a USB PID — `depz_device_open_link()` + `depz_device_promote()`
on a replay, say — also gets the CX model. To choose by hand,
`depz_vl53l8_open_link(link, model, &dev)` attaches the class with the model
you name and skips the identity probe. Either way the device owns the link and
`depz_device_close()` frees both.

## Initialise

```c
int rc = depz_vl53l8_init(dev, NULL, NULL);    /* or a progress callback */
if (rc != DEPZ_OK)
    fprintf(stderr, "%s: %s\n", depz_err_str(rc), depz_last_error());
```

`depz_vl53l8_init()` is the ULD `init`, run from the host:

1. a software reboot of the sensor, then a wait for its boot flag;
2. the **sensor firmware download** — the ~84 KB blob of the model, in three
   register banks;
3. the sensor MCU is booted and the firmware checksum checked (a mismatch
   fails with `DEPZ_E_PROTOCOL` "FW_CHECKSUM_FAIL");
4. the NVM offset calibration is read from the sensor and uploaded back, then
   the default crosstalk data and the default configuration.

On the verified board it takes **0.76 s**. The optional progress callback runs
on the calling thread: once per phase with a text (`"SW reboot..."`,
`"Downloading sensor FW (84 KB)... bank 1/3"`, `"Sensor FW checksum OK"`, ...)
and `done` / `total` both 0, and during the big writes with the phase
`"download"` and the bytes written so far. Call `init()` before any
configuration, and again whenever the sensor lost power.
`depz_vl53l8_initialized()` says whether it has run; `depz_vl53l8_is_alive()`
only reads the device id and revision (`0xF0` / `0x0C` on a VL53L8) and needs
no `init()`. `init()` is refused while ranging.

## Configure

All setters need `init()` first and are **refused while ranging** with
`DEPZ_E_ARG` ("stop ranging first"). Each has a getter that reads the value
back from the sensor. Getters need `init()` and are not blocked while ranging,
but they are register transactions on the bank the stream reads — read your
configuration back before `start_ranging()`.

```c
int zones;
uint8_t hz, mode, pct, order;
uint32_t ms;

depz_vl53l8_set_resolution(dev, DEPZ_VL53L8_RES_8X8);   /* 16 (4x4) or 64 (8x8) */
depz_vl53l8_get_resolution(dev, &zones);
depz_vl53l8_set_ranging_frequency_hz(dev, 15);          /* >= 2; max 60 at 4x4, 15 at 8x8 */
depz_vl53l8_get_ranging_frequency_hz(dev, &hz);

depz_vl53l8_set_ranging_mode(dev, DEPZ_VL53L8_RANGING_MODE_AUTONOMOUS);
depz_vl53l8_get_ranging_mode(dev, &mode);
depz_vl53l8_set_integration_time_ms(dev, 20);           /* 2..1000, autonomous only */
depz_vl53l8_get_integration_time_ms(dev, &ms);

depz_vl53l8_set_sharpener_percent(dev, 20);             /* 0..99, 0 = off */
depz_vl53l8_get_sharpener_percent(dev, &pct);
depz_vl53l8_set_target_order(dev, DEPZ_VL53L8_TARGET_ORDER_CLOSEST);
depz_vl53l8_get_target_order(dev, &order);
```

- **Resolution** — changing it also re-uploads the offset and crosstalk data
  for the new grid, as the ULD does.
- **Frequency** — anything below 2 Hz fails with `DEPZ_E_ARG` before anything
  is written: at 1 Hz a VL53L8 never starts its ranging loop and the stream
  stays silent. Set the frequency explicitly after `init()`. The upper limits
  are the sensor's.
- **Ranging mode** — *continuous* ranges back to back at the ranging
  frequency; *autonomous* integrates for the integration time once per period
  and idles in between, saving power. The integration time has no effect in
  continuous mode.
- **Sharpener** — reduces the blur between zones, where a close bright target
  spills signal into its neighbours (0 = off). It is stored as a byte, so a percentage can
  read back one off.
- **Target order** — which target a zone reports when it sees several:
  the closest or the strongest.

Out-of-range values fail with `DEPZ_E_ARG`.

## Ranging: streams and callbacks

`depz_vl53l8_start_ranging()` writes the output configuration for the current
resolution, starts the sensor and arms the board's stream: on each data-ready
INT the MCU reads the whole result frame and pushes it in chunks; the SDK
reassembles and decodes it on the reader thread. `depz_vl53l8_stop_ranging()`
stops both (idempotent; the sensor is stopped even if the stream stop fails).
Frames come out three ways — use any mix:

**Streams** — pulled from your own thread, bounded, drop-oldest. The usual
choice:

```c
static depz_vl53l8_live_frame f;
depz_stream *s = depz_vl53l8_frames(dev, 4);   /* before start: nothing is missed */
uint64_t end;

depz_vl53l8_start_ranging(dev);
end = depz_host_now_us() + 5000000u;           /* 5 s */
while (depz_host_now_us() < end && depz_stream_next(s, &f, 500) == DEPZ_OK)
    printf("%llu us: zone 0 = %d mm\n",
           (unsigned long long)f.f.timestamp_us, f.f.distance_mm[0]);
depz_vl53l8_stop_ranging(dev);
printf("%llu dropped, %llu undecoded, %llu discarded\n",
       (unsigned long long)depz_stream_dropped_count(s),
       (unsigned long long)depz_vl53l8_frame_parse_errors(dev),
       (unsigned long long)depz_vl53l8_reassembler_discards(dev));
depz_stream_close(s);
```

A `depz_vl53l8_live_frame` is about 7.5 KB (it has room for the largest CNH
block), and a stream holds `maxsize` of them: keep `maxsize` small —
`maxsize == 0` picks 8. If the consumer falls behind, the oldest frames are
discarded and `depz_stream_dropped_count()` counts them; the device is never
stalled. Two link-health counters stay separate so both keep their meaning:
`depz_vl53l8_reassembler_discards()` counts frames lost to a gap between their
chunks, `depz_vl53l8_frame_parse_errors()` frames that arrived whole but did
not decode. Both are 0 in a healthy session.

**Callbacks** — run on the device's reader thread for every frame:

```c
static void on_frame(const depz_vl53l8_live_frame *lf, void *user)
{
    /* reader thread: short, never block, never a device call here */
    int *nearest = (int *)user;
    int z;
    for (z = 0; z < lf->f.resolution; z++)
        if ((lf->f.target_status[z] == 5 || lf->f.target_status[z] == 9) &&
            lf->f.distance_mm[z] < *nearest)
            *nearest = lf->f.distance_mm[z];
}

int nearest = 1 << 30;
int token;
depz_vl53l8_on_frame(dev, on_frame, &nearest, &token);
depz_vl53l8_start_ranging(dev);
/* ... */
depz_vl53l8_stop_ranging(dev);
depz_vl53l8_off_frame(dev, token);
```

**One at a time** — `depz_vl53l8_get_frame(dev, timeout_ms, &f)` waits for the
next frame (a negative timeout means 2000 ms). It subscribes when called, so it
only sees frames that arrive after the call; for a steady flow use a stream.

While ranging, every setter, `init()` and the advanced features fail with
`DEPZ_E_ARG` — the stream owns the register bank. There is no polled single
shot: a VL53L8 measures by ranging.

## The frame

`depz_vl53l8_live_frame` is the decoded frame `f` plus what the optional
features add:

| field | meaning |
|---|---|
| `f` | the `depz_vl53l8_frame`: zone arrays, timestamp, temperature (below) |
| `has_motion` / `motion` | the motion-indicator output, once [configured](#motion-indicator) |
| `cnh_len` / `cnh[]` | the raw CNH block — VL53L8CH with CNH armed only, see the [CH guide](../vl53l8ch/guide.md) |

`depz_vl53l8_frame` carries every per-zone output as a flat array valid for
`[0, resolution)`, plus the per-frame data. Values are the **raw wire
integers** — apply the scaling yourself:

| field | type | meaning |
|---|---|---|
| `timestamp_us` | uint64 | MCU µs at the data-ready INT |
| `resolution` | int | 16 or 64 zones |
| `silicon_temp_degc` | int8 | sensor temperature, °C |
| `distance_mm[z]` | int32 | distance, mm (already `raw/4`, floored) |
| `target_status[z]` | uint8 | 5 / 9 = valid, 255 = no target |
| `nb_target_detected[z]` | uint8 | targets found in the zone |
| `signal_per_spad[z]` | uint32 | return-signal rate, kcps/SPAD (raw) |
| `ambient_per_spad[z]` | uint32 | ambient-light rate, kcps/SPAD (raw) |
| `nb_spads_enabled[z]` | uint32 | SPADs (light-detecting cells) enabled |
| `range_sigma_mm_raw[z]` | uint16 | distance standard deviation; mm = `raw/128` |
| `reflectance[z]` | uint8 | estimated reflectance, % |

Zones run **row-major**, so index `z = row*cols + col` where `cols` is 4 (16
zones) or 8 (64 zones):

```c
int cols = (f.f.resolution == DEPZ_VL53L8_RES_8X8) ? 8 : 4;
int row, col;
for (row = 0; row < cols; row++) {
    for (col = 0; col < cols; col++) {
        int z = row * cols + col;
        int mm = (f.f.target_status[z] == 255) ? -1 : f.f.distance_mm[z];
        printf("%6d", mm);                         /* -1 = no target */
    }
    printf("\n");
}
```

Check `target_status` before trusting a distance. To put `timestamp_us` on
the host clock, run `depz_device_sync_time()` once and map it with
`depz_device_to_host_time_us()`.

## Detection thresholds

Detection thresholds gate the sensor's data-ready INT — and with it the stream:
armed, the sensor only reports frames that match the rules. There are 64
entries; each tests one measurement of one zone against a window, and entries
combine with OR / AND.

```c
depz_vl53l8_threshold th[64];
int z;

/* Report only frames where some zone sees a target between 200 and 600 mm. */
for (z = 0; z < 64; z++) {
    th[z].low_thresh = 200;
    th[z].high_thresh = 600;
    th[z].measurement = DEPZ_VL53L8_DIST_MM;        /* in real units: mm */
    th[z].type = DEPZ_VL53L8_THRESH_IN_WINDOW;
    th[z].zone_num = (uint8_t)z;
    th[z].operation = DEPZ_VL53L8_THRESH_OP_OR;
}
th[63].zone_num |= DEPZ_VL53L8_THRESH_LAST;         /* marks the last entry */
depz_vl53l8_set_detection_thresholds(dev, th, 64);
depz_vl53l8_set_detection_thresholds_enable(dev, true);
```

- `measurement` is one of `DEPZ_VL53L8_DIST_MM`, `_SIGNAL_PER_SPAD_KCPS`,
  `_RANGE_SIGMA_MM`, `_AMBIENT_PER_SPAD_KCPS`, `_NB_TARGET_DETECTED`,
  `_TAR_STATUS`, `_NB_SPADS_ENABLED`, `_MOTION_INDICATOR`; `low` / `high` are
  in its real units, scaled for the sensor by the SDK.
- `type` is the window: `DEPZ_VL53L8_THRESH_IN_WINDOW`, `_OUT_OF_WINDOW`,
  `_LESS_THAN_EQUAL_MIN`, `_GREATER_THAN_MAX`, `_EQUAL_MIN`, `_NOT_EQUAL_MIN`.
- The **last** entry in use must carry `DEPZ_VL53L8_THRESH_LAST` (128, ST's
  `VL53L8CX_LAST_THRESHOLD`) OR-ed into its `zone_num`; missing entries are
  sent as zeros.
- `depz_vl53l8_get_detection_thresholds(dev, out)` reads all 64 back in real
  units; `_get_detection_thresholds_enable()` reads the switch;
  `_set_detection_thresholds_enable(dev, false)` turns the gating off again.
- `depz_vl53l8_set_detection_thresholds_auto_stop(dev, true)` sets the ULD's
  auto-stop flag for threshold detection.

The thresholds belong in the configuration phase: set them, then start ranging.

## Motion indicator

The motion indicator compares consecutive frames inside a distance window and
reports how much each group of zones moved:

```c
depz_vl53l8_set_resolution(dev, DEPZ_VL53L8_RES_8X8);  /* first: the config follows it */
depz_vl53l8_configure_motion_indicator(dev, 400, 1500); /* 400..4000 mm, span <= 1500 */
depz_vl53l8_start_ranging(dev);
if (depz_vl53l8_get_frame(dev, -1, &f) == DEPZ_OK && f.has_motion)
    printf("global %u, %u of %u aggregates moved, aggregate 0: %u\n",
           (unsigned)f.motion.global_indicator_1, f.motion.nb_of_detected_aggregates,
           f.motion.nb_of_aggregates, (unsigned)f.motion.motion[0]);
```

`configure_motion_indicator()` programs ST's default motion configuration for
the **current** resolution, then the window; change the resolution afterwards
and you must configure it again. From then on every frame carries `has_motion
= true` and `motion`: two global indicators, the count of aggregates (groups of
zones) that detected motion, and a value per aggregate in `motion[0 ..
nb_of_aggregates)`. A window outside 400..4000 mm or wider than 1500 mm fails
with `DEPZ_E_ARG`. `init()` switches the motion output off again.

## Crosstalk: margin, calibration, save and restore

Crosstalk is light the cover glass in front of the sensor reflects straight
back; the ULD subtracts a per-zone crosstalk estimate. `init()` uploads ST's
default estimate.

```c
double margin;
bool failed = false;
uint8_t cal[DEPZ_VL53L8_XTALK_BUFFER_SIZE];            /* 776 bytes */

depz_vl53l8_set_xtalk_margin(dev, 50.0);                /* kcps/SPAD, <= 10000 */
depz_vl53l8_get_xtalk_margin(dev, &margin);

/* A flat target of known reflectance at a known distance, behind the glass. */
if (depz_vl53l8_calibrate_xtalk(dev, 3, 4, 600, &failed) == DEPZ_OK && failed)
    printf("nothing to calibrate: default crosstalk kept\n");

depz_vl53l8_get_caldata_xtalk(dev, cal);                /* save this somewhere */
/* ... on a later boot, after init(): */
depz_vl53l8_set_caldata_xtalk(dev, cal);
```

- **Margin** — the crosstalk-detection margin, in kcps/SPAD.
- **Calibration** — `calibrate_xtalk(reflectance_percent 1..99, nb_samples
  1..16, distance_mm 600..3000)` saves the configuration, runs ST's
  calibration session (several seconds), reads the new crosstalk data back and
  restores the configuration. Out-of-range arguments fail with `DEPZ_E_ARG`.
  With no cover glass there is nothing to calibrate: the firmware says so, the
  call still returns `DEPZ_OK`, `*failed` becomes `true` and the default
  crosstalk stays — so check `failed`, not only the return code.
- **Save / restore** — `get_caldata_xtalk()` reads the 776-byte crosstalk blob
  (the result of a calibration); `set_caldata_xtalk()` uploads a saved one, so
  a fresh `init()` can skip the calibration. Both switch through 8×8 and back
  to your resolution internally.

The SDK writes the calibration table of the model's own firmware. It ran on
the verified VL53L8CH; on a VL53L8CX board it is not yet verified live.

## Power modes

```c
uint8_t mode;

depz_vl53l8_set_power_mode(dev, DEPZ_VL53L8_POWER_MODE_SLEEP);   /* keeps its state */
depz_vl53l8_get_power_mode(dev, &mode);
depz_vl53l8_set_power_mode(dev, DEPZ_VL53L8_POWER_MODE_WAKEUP);
```

`SLEEP` keeps the firmware and the configuration; `WAKEUP` resumes. In
`DEEP_SLEEP` the sensor loses its firmware: waking from it **re-runs the whole
`init()`** (the download included, without a progress callback), so the
configuration is back to the defaults — set it again. Setting the mode the
sensor is already in does nothing. Not while ranging.

## Registers and DCI

For anything the class does not wrap:

```c
uint8_t zone_cfg[8];

/* DCI_ZONE_CONFIG (0x5450): bytes 0 and 1 are the grid width and height. */
if (depz_vl53l8_dci_read(dev, 0x5450, zone_cfg, sizeof zone_cfg) == DEPZ_OK)
    printf("%u x %u zones\n", zone_cfg[0], zone_cfg[1]);
```

`depz_vl53l8_dci_read()` / `_dci_write()` read or write a DCI configuration
block by index (after `init()`), exactly as the ULD's `dci_read_data` /
`dci_write_data`. `depz_vl53l8_read_reg()` / `_write_reg()` reach raw sensor
registers (16-bit address; the sensor's page register `0x7FFF` selects the
bank, as in the ULD). All four bypass the class's state checks — don't use
them while ranging.

## Disconnects

When the cable goes, the reader thread notices and:

- requests that are waiting fail with `DEPZ_E_CLOSED`, and so does every
  later call on the device;
- streams **hand out what they already hold, then** return `DEPZ_E_CLOSED`;
- a `DEPZ_DEV_EV_DISCONNECTED` event fires (callback and event streams), with
  the reason in `text`; `depz_device_closed()` becomes `true`.

There is no automatic reconnect: open the device again once it is back, and
`init()` it — the sensor lost power with the board.

## Record and replay for tests

Tee a live session into a `.depzrec` file, then replay it with no board
attached. Strict replay (`strict_tx = true`) also checks that your code sends
exactly the bytes it sent when recording — the whole firmware download
included — so a changed register sequence fails loudly with
`DEPZ_E_REPLAY_MISMATCH` instead of drifting.

```c
static depz_vl53l8_live_frame f;

static void session(depz_device *dev)
{
    depz_vl53l8_init(dev, NULL, NULL);
    depz_vl53l8_set_resolution(dev, DEPZ_VL53L8_RES_8X8);
    depz_vl53l8_set_ranging_frequency_hz(dev, 15);
    depz_vl53l8_start_ranging(dev);
    depz_vl53l8_get_frame(dev, -1, &f);
    depz_vl53l8_stop_ranging(dev);
}

int record_and_replay(const char *port)
{
    depz_link *serial, *rec, *rp;
    depz_device *dev;

    /* 1. Record against the real board. */
    if (depz_link_open_serial(port, &serial) != DEPZ_OK) return -1;
    if (depz_link_open_recording(serial, "l8.depzrec", "\"note\":\"bench\"", &rec) != DEPZ_OK)
        return -1;                                 /* serial already freed */
    if (depz_vl53l8_open_link(rec, DEPZ_VL53L8_MODEL_L8CX, &dev) != DEPZ_OK) return -1;
    session(dev);
    depz_device_close(dev);

    /* 2. Replay: same calls, same order, recorded answers. */
    if (depz_link_open_replay("l8.depzrec", true, false, &rp) != DEPZ_OK) return -1;
    if (depz_vl53l8_open_link(rp, DEPZ_VL53L8_MODEL_L8CX, &dev) != DEPZ_OK) return -1;
    session(dev);
    depz_device_close(dev);
    return 0;
}
```

The ULD's poll loops (boot wait, command status) issue as many register reads
as the sensor needed on the day, and the replay serves exactly those answers —
so the replayed session takes the same path. The driver's fixed waits are
skipped on a replay link, so a replayed `init()` runs as fast as the bytes
flow. Replay with the model you recorded with: the model decides which
firmware blob is written.

The test suite replays three real captures strictly this way, all recorded by
the Python SDK — so the C and Python drivers issue the same bytes, the firmware
download included:

- `io_l8cx_replay` — `vl53l8_8x8_15hz_3s.depzrec` (a VL53L8CX, `APP_VL53L8_v0.9`):
  identity probe, `depz_device_promote()` (no USB PID, so the CX model),
  `init()`, 8×8 at 15 Hz, 45 frames;
- `io_l8ch_replay` and `io_l8ch_cnh_replay` — the VL53L8CH captures, see the
  [CH guide](../vl53l8ch/guide.md#record-and-replay-for-tests).

`io_l8_guards_and_cnh_math` covers the state checks without any device
(`init()` first, `start_ranging()` before `init()`, 1 Hz refused, an idempotent
stop, the wrong-type guard) and the CNH setup math.

## Verified on hardware

The class was run against a real VL53L8CH board (`TMNQ8E3PRR`, firmware
`APP_VL53L8_v0.92`) facing a wall at about 0.6 m — the CX code path with the
CH firmware blob:

- `init()` in 0.76 s, the firmware download and checksum included;
- 8×8 at 15 Hz: 30 frames in 2 s, none dropped, distances ~599–602 mm;
- 4×4 at 30 Hz: 30 frames/s;
- CNH armed: a 1708-byte block in every frame, decoding fine;
- sleep and wake-up;
- configuration refused with `DEPZ_E_ARG` while ranging, and 1 Hz refused.

A VL53L8CX board has not been run live with this class yet; the strict replay
of the real VL53L8CX capture above covers its firmware download and frame
decode byte for byte.

## Decode layer (your own transport)

If you run your own serial code, the VL53L8 codecs of `depz_sensor_sdk.h` work
without the live layer — the class above is built from them. You then drive
the ULD sequences yourself over `READ_REG` / `WRITE_REG`. For the parser loop
these examples build on, see the common guide's
[mental model](../guide.md#mental-model).

### The streaming pipeline

A ranging frame is larger than one bridge packet, so the device ships it as a
run of `RPT_FRAME` (`0x93`) chunks that you reassemble, then decode:

```
RPT_FRAME packet  ──►  depz_vl53l8_unpack_chunk()  ──►  depz_vl53l8_chunk
                                                              │
                       depz_vl53l8_reasm_feed()  ◄───────────┘
                             │  (returns 1 when a whole frame completes)
                             ▼
                       depz_vl53l8_decode_frame()  ──►  depz_vl53l8_frame
```

Register reads (`RPT_REG_DATA`, `0x91`) decode separately with
`depz_vl53l8_unpack_reg_data()`.

### Reassemble and decode a frame

Keep one `depz_vl53l8_reassembler` for the stream and feed each chunk into it:

```c
#include <depz_sensor_sdk.h>
#include <stdio.h>

static depz_vl53l8_reassembler reasm;   /* init once with depz_vl53l8_reasm_init */

static void on_event(const depz_event *ev, void *user)
{
    depz_vl53l8_chunk chunk;
    const uint8_t *raw;
    size_t raw_len;
    uint64_t ts;
    (void)user;
    if (ev->type != DEPZ_EV_PACKET || ev->cmd != DEPZ_VL53L8_RPT_FRAME) return;
    if (depz_vl53l8_unpack_chunk(ev->payload, ev->payload_len, &chunk) != 0) return;

    if (depz_vl53l8_reasm_feed(&reasm, &chunk, &raw, &raw_len, &ts) == 1) {
        depz_vl53l8_frame f;
        if (depz_vl53l8_decode_frame(raw, raw_len, ts, &f) == 0)
            printf("res=%d  temp=%d C  zone0=%d mm\n",
                   f.resolution, f.silicon_temp_degc, f.distance_mm[0]);
    }
}
```

`depz_vl53l8_reasm_feed()` returns `1` only when a frame completes (a gap
discards the frame in progress and bumps `reasm.discarded`).
`depz_vl53l8_decode_frame()` returns `0` on success, `-1` on a corrupt frame
(header/footer id mismatch), `-2` on a bad length. A VL53L8CH frame needs
`depz_vl53l8ch_decode_frame()` instead (its footer sits elsewhere) — see the
[CH guide](../vl53l8ch/guide.md#decode-layer-your-own-transport). Start the
stream on the device with `depz_vl53l8_pack_start_stream()` +
`DEPZ_VL53L8_CMD_START_STREAM`, after starting the sensor through the ULD.

### Advanced DCI codecs

These are the pure encode/decode halves of the ST ULD advanced features
(UM3109), shared by CX and CH — the class above uses them for its own DCI
writes:

```c
static void advanced_codecs(void)
{
    /* crosstalk margin (kcps/SPAD) <-> raw DCI value (raw = round(kcps * 2048)) */
    uint32_t raw = depz_vl53l8_xtalk_margin_to_raw(50.0);
    double kcps = depz_vl53l8_xtalk_margin_from_raw(raw);

    /* detection thresholds -> the 768-byte DCI_DET_THRESH_START block */
    depz_vl53l8_threshold th = {
        .low_thresh = 200, .high_thresh = 600,
        .measurement = DEPZ_VL53L8_DIST_MM, .type = 0,
        .zone_num = 128, .operation = 0,         /* zone 0, the last entry */
    };
    uint8_t start[DEPZ_VL53L8_THRESH_START_SIZE], valid[8];
    uint8_t motion[DEPZ_VL53L8_MOTION_CFG_SIZE];
    depz_vl53l8_pack_thresholds(&th, 1, start, valid);

    /* the default 156-byte motion configuration for a resolution */
    depz_vl53l8_motion_cfg_default_pack(DEPZ_VL53L8_RES_8X8, motion);
    (void)kcps;
}
```

`low` / `high` are scaled by the entry's `measurement` selector inside
`depz_vl53l8_pack_thresholds`.

## Gotchas

- **`init()` first, and after every power loss** (and after a deep-sleep
  wake, which re-runs it for you). It downloads the sensor firmware; 0.76 s on
  the verified board.
- **Ranging frequency ≥ 2 Hz.** Below that the sensor never ranges and the
  stream stays silent; the SDK refuses it. Set the frequency explicitly after
  `init()`.
- **Configuration while ranging fails with `DEPZ_E_ARG`.** The stream owns the
  register bank: stop, reconfigure, restart.
- **Frames are big.** A `depz_vl53l8_live_frame` is ~7.5 KB: keep stream
  `maxsize` small and don't put frames on a small thread stack.
- **Check `target_status`** — 255 means no target; only 5 and 9 are valid
  ranges. Values are raw fixed-point except `distance_mm`.
- **Crosstalk calibration can "succeed" without calibrating.** With no cover
  glass `calibrate_xtalk()` returns `DEPZ_OK` with `failed == true` and keeps
  the default data.
- **Integration time only matters in autonomous mode.**
- **Callbacks run on the reader thread** — keep them short, don't block, don't
  make device calls or close the device from one. Prefer a stream for real
  work.
- **Replay with the recorded model.** A CX session replayed with the CH model
  writes a different firmware blob and fails strict replay.
