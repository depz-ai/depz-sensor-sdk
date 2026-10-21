# TMF8829 — user guide

Hands-on guide to the TMF8829: the live sensor class of `depz_sensor_io.h`
first, then the codecs underneath for when you own the transport. For what the
sensor is and its concepts, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md). Devices, errors, threading rules and
streams in general are in the [common guide](../guide.md#live-layer).

The class issues the same register sequences as the Python SDK's `Tmf8829`, so
a capture made by either SDK replays through the other. It is **checked
against the golden vectors and a fake bridge, not yet on a board from C** —
see [verification status](#verification-status).

## Contents

- [Open, init and configure](#open-init-and-configure)
- [Presets and options](#presets-and-options)
- [The measurement](#the-measurement)
- [Streaming: streams and callbacks](#streaming-streams-and-callbacks)
- [Histograms and reference frames](#histograms-and-reference-frames)
- [Capture time and flags](#capture-time-and-flags)
- [Raw registers and the config page](#raw-registers-and-the-config-page)
- [Bridge diagnostics and recovery](#bridge-diagnostics-and-recovery)
- [Record and replay for tests](#record-and-replay-for-tests)
- [Verification status](#verification-status)
- [Decode layer (your own transport)](#decode-layer-your-own-transport)
- [Gotchas](#gotchas)

## Open, init and configure

`depz_open_device()` finds the board, probes it and attaches the TMF8829 class
when the firmware is `APP_TMF8829_*`. Check `depz_is_tmf8829()` — the same call
opens any DEPZ board, and a TMF8829 call on another type fails with
`DEPZ_E_WRONG_TYPE`.

```c
#include <depz_sensor_io.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    depz_open_options opt = DEPZ_OPEN_OPTIONS_INIT;
    depz_device *dev;
    depz_stream *meas;
    depz_tmf8829_measurement *m = malloc(sizeof *m);   /* ~90 KB: not on the stack */
    char ver[16];
    uint32_t serial;
    int i;

    if (argc > 1) opt.port = argv[1];
    if (!m || depz_open_device(&opt, &dev) != DEPZ_OK) {
        fprintf(stderr, "open: %s\n", depz_last_error());
        return 1;
    }
    if (!depz_is_tmf8829(dev)) {                        /* sensor type "tmf8829" */
        fprintf(stderr, "%s is not a TMF8829\n", depz_device_port(dev));
        depz_device_close(dev);
        return 1;
    }
    /* soft reset + sensor firmware download (~0.15 s), then a preset */
    if (depz_tmf8829_init(dev, NULL, NULL, ver, sizeof ver, &serial) != DEPZ_OK ||
        depz_tmf8829_configure(dev, "16x16", NULL, NULL) != DEPZ_OK) {
        fprintf(stderr, "init/configure: %s\n", depz_last_error());
        depz_device_close(dev);
        return 1;
    }
    printf("sensor app %s, serial %08X\n", ver, (unsigned)serial);

    meas = depz_tmf8829_measurements(dev, 4);           /* subscribe before starting */
    if (depz_tmf8829_start(dev) == DEPZ_OK) {
        for (i = 0; i < 100 && depz_stream_next(meas, m, 1000) == DEPZ_OK; i++) {
            unsigned r = m->rows / 2, c = m->cols / 2;  /* the zone in the middle, peak 0 */
            printf("t=%lld us  %ux%u  centre %.1f mm\n", (long long)m->timestamp_us,
                   (unsigned)m->rows, (unsigned)m->cols, m->distance_mm[r * m->cols + c]);
        }
        depz_tmf8829_stop(dev);
    }
    depz_stream_close(meas);
    depz_device_close(dev);
    free(m);
    return 0;
}
```

**`depz_tmf8829_init(dev, progress, user, app_version, cap, &serial)`** brings
the sensor from any state to its running application: soft reset, the sensor
bootloader, the 14 444-byte ams firmware written into the sensor's FIFO in
≤ 1023-byte pieces, then the RAM application. It stops a running stream first
and forgets the last configuration. `progress(done, total, user)` is called
after each piece of the download (15 calls); `app_version` (`"2.200.0"`, give
it at least 12 bytes) and `serial` may be `NULL`. Afterwards
`depz_tmf8829_sensor_id()` returns the same two values again (`DEPZ_E_ARG`
before the first init).

On a bare link — a replay, say — `depz_tmf8829_open_link(link, &dev)` attaches
the class without a probe; `depz_device_open_link()` followed by
`depz_device_promote()` asks the firmware name first and attaches it then.

## Presets and options

**`depz_tmf8829_configure(dev, preset, &opt, &cfg)`** loads a preset, writes
the config page with your options on top and reads it back. `opt` `NULL` means
`DEPZ_TMF8829_OPTIONS_INIT`; `cfg` may be `NULL`. It is refused while
streaming (`DEPZ_E_ARG`: stop first).

| Preset | Grid (rows × cols) | Raw distance unit | Notes |
|---|---|---|---|
| `8x8` | 8 × 8 | 0.25 mm | the default of `start()` without `configure()` |
| `8x8-lr` | 8 × 8 | 0.25 mm | long range |
| `8x8-ha` | 8 × 8 | 0.25 mm | high accuracy |
| `8x8-xr` | 8 × 8 | **mm** | extended range |
| `16x16` / `16x16-ha` | 16 × 16 | 0.25 mm | |
| `32x32` / `32x32-ha` | 32 × 32 | 0.25 mm | two half-frames per measurement |
| `48x32` / `48x32-ha` | **32 × 48** | 0.25 mm | two half-frames per measurement |

`DEPZ_TMF8829_PRESETS[]` lists them (name, load command, the preset's own
`kilo_iterations`); `depz_tmf8829_find_preset(name)` looks one up.

`depz_tmf8829_options` — the fields a preset does **not** reset, so the class
writes every one of them on every `configure()`:

| Field | Default | Meaning |
|---|---|---|
| `period_ms` | 33 | measurement period; 1 = as fast as the sensor goes |
| `kilo_iterations` | −1 = the preset's own (586 / 2344 / 3516) | laser pulses per measurement, in thousands: more = less noise, slower |
| `nr_peaks` | 1 | peaks per zone, 1..4 |
| `signal_strength`, `noise_strength`, `xtalk` | false | add the per-peak signal, the per-zone noise / crosstalk to the result |
| `histograms` | false | also stream the raw histogram frames |
| `ref_frame` | false | also stream the 60-byte reference-SPAD frame before each results frame |
| `fields`, `n_fields` | none | any other config-page field by name (also overrides the ones above) |

```c
depz_tmf8829_options o = DEPZ_TMF8829_OPTIONS_INIT;
depz_tmf8829_field_value extra[1] = {{"power_modes", 5}};
depz_tmf8829_config cfg;

o.period_ms = 1;            /* as fast as it goes */
o.nr_peaks = 2;
o.signal_strength = true;
o.fields = extra;
o.n_fields = 1;
if (depz_tmf8829_configure(dev, "48x32", &o, &cfg) != DEPZ_OK)
    fprintf(stderr, "%s\n", depz_last_error());
else
    printf("%s: %u x %u\n", cfg.preset, cfg.rows, cfg.cols);   /* 48x32: 32 x 48 */
```

The named page fields (`DEPZ_TMF8829_CFG_FIELDS[]`): `period_ms`,
`kilo_iterations`, `fp_mode`, `nr_peaks`, `signal_strength`,
`noise_strength`, `xtalk`, `dump_histograms`, `ref_spad_frame`,
`power_modes`, `ha_kilo_iterations`, `dual_mode`, `fov_correction`,
`distance_in_mm`. `cfg.fields[i]` holds each one as read back, in that order;
`cfg.page` is the whole 190-byte page.

What can go wrong:

- an unknown preset or field name, a value too wide for its field, a
  `nr_peaks` outside its 3 bits → `DEPZ_E_ARG`, nothing sent;
- the sensor refuses the page → `DEPZ_E_PROTOCOL` with its status in
  `depz_last_error()`, e.g. `ERR_CONFIG_RESULT_SIZE`: the result format is too
  big for the grid (the sensor's limit is 7684 data bytes per frame — 4 peaks
  with every strength fits 8×8 but not 32×32);
- a written field does not read back → `DEPZ_E_PROTOCOL` "config fields did
  not stick: …" (`fov_correction` does that on application 2.200.0).

A refused `configure()` keeps the last good configuration for `start()`.

## The measurement

`depz_tmf8829_measurement` is one full grid (both half-frames merged for 32×32
and 48×32):

| Field | Meaning |
|---|---|
| `timestamp_us` | capture time, MCU µs — the middle of the light-collection window ([below](#capture-time-and-flags)) |
| `flags` | `DEPZ_TMF8829_FT_*` of its results frames, OR-ed |
| `fp_mode`, `rows`, `cols` | the sensor's focal-plane mode and the full grid |
| `f_number` | the sensor's frame number |
| `temperature_c` | die temperature, °C |
| `distance_mm[]` | distances in mm, corrected for the sensor clock; valid when `results.peaks > 0` |
| `distance_in_mm`, `ratio_q16` | the raw unit (mm with `8x8-xr`, else 0.25 mm) and the clock ratio used |
| `results` | the raw sensor integers: `distance`, `snr`, `signal` per peak; `noise`, `xtalk` per zone; `has_signal` / `has_noise` / `has_xtalk` |
| `n_frames`, `frames[]`, `times[]` | the 1 or 2 results frames (header / footer) and their capture times |
| `n_histograms`, `n_ref_frames` | how many histogram / ref-SPAD frames belonged to it |
| `n_problems`, `problems[]` | what went wrong since the previous measurement (the first 16 as text) |

The per-peak arrays are `[peaks][rows][cols]`, packed densely: zone (r, c) of
peak p is at index `p * rows * cols + r * cols + c`; `noise` and `xtalk` are
`[rows][cols]`. The order is the sensor's own — for a board held USB down and
seen from behind, **r runs left → right and c top → bottom**; transpose to
draw the scene.

```c
static void print_zone(const depz_tmf8829_measurement *m, unsigned r, unsigned c)
{
    size_t zones = (size_t)m->rows * m->cols, z = (size_t)r * m->cols + c;
    unsigned p;
    for (p = 0; p < m->results.peaks; p++)
        printf("peak %u: %.1f mm  snr %u%s\n", p, m->distance_mm[p * zones + z],
               (unsigned)m->results.snr[p * zones + z],
               m->results.has_signal ? "" : "  (no signal field)");
    if (m->results.has_noise) printf("noise %u\n", (unsigned)m->results.noise[z]);
}
```

A problem never drops a measurement whose frames parsed — read `problems[]`
(bad frame status, EOF, FIFO flags, a dropped half-frame). A corrupt results
frame drops its measurement and counts in the stream stats.

## Streaming: streams and callbacks

`depz_tmf8829_start(dev)` arms the bridge stream (results, plus histograms /
the reference frame as configured) and starts the sensor measuring;
`depz_tmf8829_stop(dev)` stops the sensor, then the bridge stream — the
frames already queued still drain. `start()` is a no-op while streaming;
`depz_tmf8829_streaming()` tells. Without a `configure()` since `init()`,
`start()` configures the default `8x8` first.

Rates at `period_ms = 1` (measured on the Python SDK): about 30
measurements/s at 8×8 and 16×16, 15 at 32×32, 10 at 48×32. With histograms
it drops to ~10 (8×8 / 16×16) and below 2 (48×32) — USB is the limit then.

Three ways to consume, as for every sensor class:

```c
static void on_meas(const depz_tmf8829_measurement *m, void *user)
{
    (void)user;                                        /* the reader thread: keep it short */
    printf("fN=%u t=%lld\n", (unsigned)m->f_number, (long long)m->timestamp_us);
}

void consume(depz_device *dev, depz_tmf8829_measurement *m /* malloc'ed */)
{
    depz_stream *st = depz_tmf8829_measurements(dev, 8);   /* 1. bounded, drop-oldest */
    depz_tmf8829_stream_stats stats;
    int token;

    depz_tmf8829_on_measurement(dev, on_meas, NULL, &token); /* 2. callback */
    depz_tmf8829_start(dev);
    while (depz_stream_next(st, m, 1000) == DEPZ_OK && m->f_number < 300)
        ;                                                  /* ... use *m ... */
    if (depz_tmf8829_get_measurement(dev, 1000, m) == DEPZ_OK)  /* 3. just the next one */
        printf("%u zones\n", (unsigned)m->rows * m->cols);
    depz_tmf8829_stop(dev);

    depz_tmf8829_get_stream_stats(dev, &stats);
    printf("dropped %llu  discarded %llu  unparsed %llu\n",
           (unsigned long long)stats.frames_dropped,
           (unsigned long long)stats.frames_discarded,
           (unsigned long long)stats.frame_parse_errors);
    depz_tmf8829_off_measurement(dev, token);
    depz_stream_close(st);
}
```

- **`depz_tmf8829_measurements(dev, maxsize)`** — a pull stream (`maxsize` 0
  = 16; each item is a whole ~90 KB measurement); when full, the oldest goes
  and `depz_stream_dropped_count()` grows. It receives what is produced after
  the call, so subscribe before `start()`.
- **`depz_tmf8829_on_measurement(dev, cb, user, &token)`** — a callback on
  the reader thread; `depz_tmf8829_off_measurement(dev, token)` removes it.
- **`depz_tmf8829_get_measurement(dev, timeout_ms, m)`** — the next one after
  the call (a negative timeout = 1000 ms), `DEPZ_E_TIMEOUT` on silence.

`depz_tmf8829_get_stream_stats()` counts the current stream (reset by
`start()`): frames completed, **dropped** (the bridge skipped a frame
number), **discarded** (lost in reassembly), frames that did not decode,
short reports, measurements, and the swallowed stop event. All zeros but the
first and `measurements` in a healthy session.

## Histograms and reference frames

`opt.histograms = true` makes the sensor dump its raw photon histograms
before each results frame: 2 frames per measurement at 8×8 / 16×16, 8 at
32×32, 12 at 48×32 (twice that with `dual_mode`), 25 377 bytes each.
`opt.ref_frame = true` adds the 60-byte reference-SPAD frame (behind a zero
5-byte pre-header, 65 bytes) before each results frame. A measurement only
**counts** them (`n_histograms`, `n_ref_frames`); their bytes come through
the frame callback, which sees every reassembled frame in stream order:

```c
static void on_frame(const depz_tmf8829_raw_frame *f, void *user)
{
    depz_tmf8829_frame_info fi;
    (void)user;
    if (depz_tmf8829_parse_frame(f->data, f->len, &fi) != 0) return;
    if (fi.kind == DEPZ_TMF8829_FID_HISTOGRAMS) {
        const uint8_t *bins = f->data + DEPZ_TMF8829_FRAME_HEAD;
        size_t n = f->len - DEPZ_TMF8829_FRAME_HEAD - DEPZ_TMF8829_FOOTER_SIZE;
        printf("histogram frame fN=%u, %zu data bytes\n", (unsigned)fi.f_number, n);
        (void)bins;                    /* the ams histogram layout: decode it yourself */
    }
}
/* ... depz_tmf8829_on_frame(dev, on_frame, NULL, &token); */
```

`f->data` is valid only during the call — copy what you keep. Stopping
mid-dump makes the sensor raise one bad-header event; `stop()` expects it
and swallows it.

## Capture time and flags

The sensor stamps each frame with its own 125 kHz clock (SYSTICK); the bridge
pairs that stamp with the MCU microsecond clock at the moment it reads the
frame header. The SDK keeps a running model of the two clocks (the sensor's
runs ~1.5 % slow, ratio ~1.015, measured over a sliding 1–2 s window) and
puts `timestamp_us` in the **middle** of the window the sensor was actually
collecting light — not at the moment the bytes arrived. The same ratio
corrects the distances. Both are bit-exact with the Python SDK
(`contracts/vectors/tmf8829_frame_time.json`). Map MCU time to host time with
the common time sync ([common guide](../guide.md#common-commands-and-time-sync)).

| Flag | Meaning |
|---|---|
| `DEPZ_TMF8829_FT_NOMINAL` | the clock ratio is not measured yet (first ~200 ms of a stream): the nominal 8 µs per tick is used |
| `DEPZ_TMF8829_FT_CLOCK_RESET` | the clock model restarted at this frame (a jump; only after a power cycle or the 9.5 h wrap) |
| `DEPZ_TMF8829_FT_EDGE_LATE` | the interrupt edge came > 2 ms later than usual — a plausibility warning |
| `DEPZ_TMF8829_FT_STALLED` | the sensor paused between sub-measurements (waiting for the histogram FIFO) |
| `DEPZ_TMF8829_FT_T1_UNSURE` | histograms with 32×32 / 48×32: the time can be up to ~20 ms late |

`times[i]` of a measurement has the details per results frame (`t0_us` /
`t1_us`, the sub-measurement length, the clock pair, the ratio after it).

## Raw registers and the config page

```c
uint8_t page[DEPZ_TMF8829_CFG_SIZE], id;
uint32_t period;

depz_tmf8829_config_page(dev, page);                 /* the page the sensor holds now */
depz_tmf8829_cfg_get(page, "period_ms", &period);
depz_tmf8829_read_registers(dev, DEPZ_TMF8829_REG_ID, &id, 1);   /* 0x9E */
```

`depz_tmf8829_read_registers()` / `_write_registers()` split into
≤ 1024 / 1023-byte requests. The sensor's address auto-increments except at
the FIFO port `0xFF`: a read that reaches `0xFF` keeps reading the FIFO, a
write to `0xFF` stays there. They work while streaming (each waits behind a
frame in flight, up to 1 s). Writing registers behind the class's back —
`ENABLE`, `CMD_STAT`, the interrupt registers — can stall the stream; use
them for diagnostics.

`depz_tmf8829_get_config(dev, &cfg)` returns the last `configure()` result
(`DEPZ_E_ARG` when none since `init()`).

## Bridge diagnostics and recovery

```c
depz_tmf8829_info info;
if (depz_tmf8829_bridge_info(dev, &info) == DEPZ_OK)    /* never touches the sensor */
    printf("streaming %d  INT edges %u  frames %u  aborted %u  errors %u  SPI timeouts %u\n",
           (int)info.streaming, (unsigned)info.int_edges, (unsigned)info.frames,
           (unsigned)info.frames_aborted, (unsigned)info.frame_errors,
           (unsigned)info.spi_timeouts);
```

The frame counters restart with each stream; `int_edges` runs from MCU
power-up. `frames_aborted` grows by one when `stop()` cuts a frame being sent.

**Events.** `depz_tmf8829_on_event(dev, cb, user, &token)` delivers the
bridge's `RPT_TMF_EVENT`: `DEPZ_TMF8829_EV_INT` (a motion / proximity
interrupt) or `DEPZ_TMF8829_EV_BAD_HEADER` — the bridge refused a frame
header. The one a `stop()` raises mid histogram dump is swallowed (counted in
`events_swallowed`); **any other bad header means the sensor is lost**.

**Recovery = init.** There is no sleep, standby or power-down on this board
(the sensor's enable pin is wired high), so there is no power-mode API.
From any state — a lost sensor, a refused command, a stuck stream —
`depz_tmf8829_init()` brings it back, then `configure()` again (init forgets
the configuration; a `start()` without one would run the default `8x8`). If
`init()` itself fails, only re-plugging the USB cable (a power cycle) is left.

**Disconnects** work as for every class: waiting calls and later ones fail
with `DEPZ_E_CLOSED`, streams hand out what they hold and then return
`DEPZ_E_CLOSED`, a `DEPZ_DEV_EV_DISCONNECTED` event fires. Open the device
again and `init()` it — the sensor lost its firmware with the power.

## Record and replay for tests

Tee a live session into a `.depzrec` file, then replay it with no board
attached. Strict replay (`strict_tx = true`) also checks that your code sends
exactly the bytes it sent when recording (`DEPZ_E_REPLAY_MISMATCH`
otherwise) — init's firmware download included.

```c
static void session(depz_device *dev, depz_tmf8829_measurement *m)
{
    depz_stream *st = depz_tmf8829_measurements(dev, 4);
    depz_tmf8829_init(dev, NULL, NULL, NULL, 0, NULL);
    depz_tmf8829_configure(dev, "32x32", NULL, NULL);
    depz_tmf8829_start(dev);
    depz_stream_next(st, m, 2000);
    depz_tmf8829_stop(dev);
    depz_stream_close(st);
}

int record_and_replay(const char *port, depz_tmf8829_measurement *m)
{
    depz_link *serial, *rec, *rp;
    depz_device *dev;

    if (depz_link_open_serial(port, &serial) != DEPZ_OK) return -1;
    if (depz_link_open_recording(serial, "tmf.depzrec", "\"note\":\"bench\"", &rec) != DEPZ_OK)
        return -1;                                     /* serial already freed */
    if (depz_tmf8829_open_link(rec, &dev) != DEPZ_OK) return -1;
    session(dev, m);
    depz_device_close(dev);

    if (depz_link_open_replay("tmf.depzrec", true, false, &rp) != DEPZ_OK) return -1;
    if (depz_tmf8829_open_link(rp, &dev) != DEPZ_OK) return -1;
    session(dev, m);
    depz_device_close(dev);
    return 0;
}
```

The poll loops (bootloader, command status) issue as many reads as the sensor
needed on the day and the replay serves exactly those answers. A dataset
(`.depzdata`, contract 09) with `tmf8829` records — written by the Python or
TypeScript SDK — reads with the generic `depz_dataset_*` reader of the
[common guide](../guide.md#datasets).

## Verification status

**Checked against the golden vectors and a fake bridge; not yet run on a
board from C.**

- `vec_tmf8829` — `contracts/vectors/tmf8829.json`: the command encodings
  and report decodes, chunk reassembly (in order, a frame number changing
  mid-frame, an offset gap), real frames captured on a board (8×8, 8×8 with
  every field, 8×8-xr in mm, 16×16, 32×32 halves, 48×32 with 2 peaks and
  signal, a histogram frame, a ref-SPAD frame) — header, footer, every raw
  integer and the merged grids — and config-field get / set on a captured
  page.
- `vec_tmf8829_frame_time` — 27 recorded streams (every preset, histograms,
  reference frames): the capture time, sub-measurement length, flags and
  clock ratio of all 970 results frames, bit for bit.
- `io_tmf8829_fake_session` — the class against a fake bridge: discovery,
  `init()` with the firmware download, `configure()` over a stale page with
  read-back (and its refusals), a 32×32 stream with ref frames from captured
  frames into the expected measurement, an event, bridge info, raw reads into
  the FIFO port, `stop()` swallowing the bad-header event, the stream counters.

The register sequences are the Python SDK's, which is hardware-verified on
firmware `APP_TMF8829_v0.52` (19.10.2026); the measured behaviour this page
describes (rates, units, what a preset keeps, orientation) comes from that
work.

## Decode layer (your own transport)

If you run your own serial code, the TMF8829 codecs of `depz_sensor_sdk.h`
are pure functions over your buffers. Commands go out in the shared framing
(`depz_build_packet()`, see the [common guide](../guide.md)); the payload
encoders return the length written, or 0 when an argument is out of range:

```c
#include <depz_sensor_sdk.h>

void port_write(const uint8_t *bytes, size_t len);   /* your transport */

static void send_packet(uint8_t cmd, const uint8_t *payload, size_t len)
{
    static unsigned seq;
    static uint8_t frame[DEPZ_HEADER_SIZE + 1 + 1024 + 4];
    size_t n;
    if (depz_build_packet(cmd, payload, len, seq++, DEPZ_CRC8, frame, sizeof frame, &n) == 0)
        port_write(frame, n);
}

void read_id_and_start(void)
{
    uint8_t p[3], s[2];
    size_t n = depz_tmf8829_pack_read_reg(DEPZ_TMF8829_REG_ID, 1, p);   /* 3 */
    send_packet(DEPZ_TMF8829_CMD_READ_REG, p, n);     /* -> RPT_TMF_REG_DATA, 0x9E */
    /* ... wait for the reply ... */
    n = depz_tmf8829_pack_start_stream(DEPZ_TMF8829_INT_RESULTS, 0, s); /* 2 */
    send_packet(DEPZ_TMF8829_CMD_START_STREAM, s, n);
}
```

The replies decode with `depz_tmf8829_unpack_reg_data()` (`0x91`),
`_unpack_info()` (`0x92` — note: no echoed command byte), `_unpack_event()`
(`0x94`). Only one register command may be outstanding: wait for each reply.

**Frames to measurements.** Feed every `RPT_TMF_FRAME` (`0x93`) payload to a
reassembler, and every completed frame to a measurement builder. Both are big
(~64 KB and ~80 KB): allocate them, and reset both at every stream start.

```c
#include <depz_sensor_sdk.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    depz_tmf8829_reassembler *reasm;
    depz_tmf8829_builder *builder;
    depz_tmf8829_measurement *m;
} tmf_rx;

int tmf_rx_init(tmf_rx *rx)
{
    rx->reasm = malloc(sizeof *rx->reasm);
    rx->builder = malloc(sizeof *rx->builder);
    rx->m = malloc(sizeof *rx->m);
    if (!rx->reasm || !rx->builder || !rx->m) return -1;
    depz_tmf8829_reassembler_reset(rx->reasm);    /* again at every TMF_START_STREAM */
    depz_tmf8829_builder_reset(rx->builder);
    return 0;
}

/* one decoded packet of the device */
void tmf_rx_packet(tmf_rx *rx, uint8_t cmd, const uint8_t *payload, size_t len)
{
    depz_tmf8829_frame_chunk ch;
    depz_tmf8829_raw_frame f;

    if (cmd != DEPZ_TMF8829_RPT_FRAME || depz_tmf8829_unpack_frame_chunk(payload, len, &ch) != 0)
        return;
    if (!depz_tmf8829_reassembler_feed(rx->reasm, &ch, &f)) return;   /* frame not complete */
    if (depz_tmf8829_builder_add(rx->builder, f.data, f.len, (int64_t)f.timestamp_us,
                                 (int64_t)f.edge_us, rx->m))
        printf("measurement fN=%u %ux%u at %lld us, %zu problems\n",
               (unsigned)rx->m->f_number, (unsigned)rx->m->rows, (unsigned)rx->m->cols,
               (long long)rx->m->timestamp_us, rx->m->n_problems);
}
```

The reassembler follows the contract's rules (restart on offset 0, never
stitch chunks of different frame numbers) and counts `completed`,
`discarded` and `seq_gaps` (frames the bridge dropped). The builder does the
half-frame merge, attaches histogram / ref frames, runs the clock model and
counts `parse_errors`.

**Lower down**, each step is a function of its own: `depz_tmf8829_parse_frame()`
(pre-header, header, footer; `kind`, `fp_mode`, `second_half`,
`distance_in_mm` derived), `depz_tmf8829_frame_problems_of()`,
`depz_tmf8829_parse_results()` (the pixels of one results frame),
`depz_tmf8829_merge_halves()` (even rows + odd rows), and the clock model —
`depz_tmf8829_clock_add()` for every non-ref frame,
`depz_tmf8829_timer_histograms()` for a histogram frame,
`depz_tmf8829_timer_results()` for a results frame, which fills a
`depz_tmf8829_frame_time`. `depz_tmf8829_fp_grid()` /
`_frame_rows()` give the grid of an `fp_mode` (full and per frame).

**Running the sensor yourself** takes the sequence of contract 14 §4 — soft
reset, the bootloader commands, `depz_tmf8829_firmware` /
`depz_tmf8829_firmware_len` written to `DEPZ_TMF8829_REG_FIFO` with the load
address `DEPZ_TMF8829_FIRMWARE_LOAD_ADDR`, the presets
(`DEPZ_TMF8829_PRESETS`), the page fields (`depz_tmf8829_cfg_get` /
`_cfg_set`), MEASURE / STOP, and `depz_tmf8829_status_name()` for the
statuses. The live class is that sequence; reading
`src/io/tmf8829_device.c` is the shortest way to it.

## Gotchas

- **`depz_tmf8829_measurement` is ~90 KB** — allocate it; a stream of
  `maxsize` N holds N of them.
- **48×32 is 32 rows × 48 columns** — `rows = 32`, `cols = 48`.
- **The grid is transposed against the scene** — row index left → right,
  column index top → bottom (board USB down, seen from behind). Transpose
  to draw.
- **Use `distance_mm`, not `results.distance`** — the raw integers are in
  0.25 mm (mm with `8x8-xr`) and not corrected for the ~1.5 % slow sensor
  clock.
- **`configure()` writes the whole result format every time** — that is on
  purpose: the sensor keeps stale fields across a preset load, and a stale
  4-peak format makes the next 32×32 fail with `ERR_CONFIG_RESULT_SIZE`.
  Set every option you want on each call.
- **`ERR_CONFIG_RESULT_SIZE`** — the result format does not fit the grid:
  fewer peaks or strengths at 32×32 / 48×32.
- **`init()` forgets the configuration** — call `configure()` after it, or
  `start()` runs the default `8x8`.
- **No sleep** — recovery from anything is `init()`; a bad-header event
  outside `stop()` means exactly that.
- **`configure()` is refused while streaming** — `stop()` first.
- **Histograms slow everything down** and make capture times of 32×32 /
  48×32 unsure (`FT_T1_UNSURE`) — a calibration mode, not a streaming mode.
- **Subscribe before `start()`** to see the first measurement.
- **Callbacks run on the reader thread** — keep them short, and never call
  `depz_device_close()` from one; the frame callback's `f->data` is valid
  only during the call.
- **Decode layer:** reset the reassembler and builder at every stream start
  (frame numbers and the clock model restart), feed every frame — ref frames
  included — in stream order, and wait for each register reply before the
  next command.
