# VL53L4CX — user guide

Hands-on guide to the VL53L4CX in C: the 1D-family sensor class
(`depz_vl53lx_*` in `depz_sensor_io.h`) first, then the codecs underneath it
for when you own the transport. For what the sensor is and its concepts, read
the [introduction](introduction.md); for exact signatures see the
[API reference](api.md).

## Contents

- [Open and initialize](#open-and-initialize)
- [Hello world: live distance](#hello-world-live-distance)
- [Configure](#configure)
- [Streaming and single shot](#streaming-and-single-shot)
- [The measurement](#the-measurement)
- [Several targets (histogram driver)](#several-targets-histogram-driver)
- [What each driver supports](#what-each-driver-supports)
- [Settings and calibration](#settings-and-calibration)
- [Reset and bridge diagnostics](#reset-and-bridge-diagnostics)
- [Record and replay for tests](#record-and-replay-for-tests)
- [Verification status](#verification-status)
- [Decode layer (your own transport)](#decode-layer-your-own-transport)
- [Gotchas](#gotchas)

## Open and initialize

`depz_open_device()` finds the board, reads its firmware name
(`APP_VL53L0_4_*`) and attaches the 1D-family class: `depz_is_vl53lx()` is
then true. Every 1D board runs the same firmware, so the product comes from
the USB PID or the board's device name (`… VL53L4CX USB v2.1 …`).
`depz_vl53lx_init()` binds a (product, driver) pair and initialises the
sensor: product `DEPZ_VL53LX_PRODUCT_NONE` means the detected one, driver `0`
the product's first kind — `histogram` here.

```c
#include <depz_sensor_io.h>
#include <stdio.h>

int main(void)
{
    depz_device *dev;
    uint16_t model;
    if (depz_open_device(NULL, &dev) != DEPZ_OK) {
        fprintf(stderr, "open: %s\n", depz_last_error());
        return 1;
    }
    if (!depz_is_vl53lx(dev) || depz_vl53lx_init(dev, 0, DEPZ_VL53LX_PRODUCT_NONE) != DEPZ_OK) {
        fprintf(stderr, "init: %s\n", depz_last_error());
        depz_device_close(dev);
        return 1;
    }
    depz_vl53lx_model_id(dev, &model);
    printf("%s / %s, model id 0x%04X\n",
           depz_vl53lx_product_get(depz_vl53lx_product_bound(dev))->name,
           depz_vl53lx_driver_str(depz_vl53lx_driver_bound(dev)), model);
    printf("caveat: %s\n", depz_vl53lx_caveat(dev));
    depz_device_close(dev);
    return 0;
}
```

The model id (`0xEBAA`) is a cross-check only: several products share
theirs. `depz_vl53lx_caveat()` says what the pair cannot do, "" when nothing.

To run the die on another product's driver, name that product — here
the VL53L4CD light driver (single target, ~1.2 m, offset and crosstalk calibration, thresholds) — or name `VL53L1CX` for its light driver (long / short modes, ROI, calibrations). `depz_vl53lx_product_bound()` then answers the borrowed product:

```c
#include <depz_sensor_io.h>

int borrow_light_driver(depz_device *dev)
{
    return depz_vl53lx_init(dev, DEPZ_VL53LX_DRIVER_ULD, DEPZ_VL53LX_PRODUCT_L4CD);
}
```

## Hello world: live distance

```c
#include <depz_sensor_io.h>
#include <stdio.h>

int main(void)
{
    depz_device *dev;
    depz_stream *s;
    depz_vl53lx_measurement m;
    int i;
    if (depz_open_device(NULL, &dev) != DEPZ_OK) return 1;
    if (depz_vl53lx_init(dev, 0, DEPZ_VL53LX_PRODUCT_NONE) != DEPZ_OK ||
        depz_vl53lx_configure(dev, 50, 0, "medium", NULL, NULL) != DEPZ_OK) {
        fprintf(stderr, "%s\n", depz_last_error());
        depz_device_close(dev);
        return 1;
    }
    s = depz_vl53lx_measurements(dev, 64);  /* subscribe before starting */
    depz_vl53lx_start_ranging(dev);
    for (i = 0; i < 100 && depz_stream_next(s, &m, 1000) == DEPZ_OK; i++) {
        if (depz_vl53lx_plottable(&m))
            printf("%5d mm  sigma %.1f mm\n", (int)m.distance_mm, m.sigma_mm);
        else
            printf("status %d (%s)\n", m.status, m.status_text);
    }
    depz_vl53lx_stop_ranging(dev);
    depz_stream_close(s);
    depz_device_close(dev);
    return 0;
}
```

The example program `examples/vl53lx_minimal.c` does the same for any board
of the family and prints the frame rate at the end.

## Configure

```c
#include <depz_sensor_io.h>
#include <stdio.h>

void show_timing(depz_device *dev)
{
    const char *modes[DEPZ_VL53LX_MAX_MODES], *mode = NULL;
    int budget, inter, lo, hi, choices[16];
    size_t n, k, nc = 0;
    depz_vl53lx_configure(dev, 50, 0, "medium", NULL, NULL);   /* re-init + apply */
    depz_vl53lx_get_range_timing(dev, &budget, &inter);     /* read back */
    depz_vl53lx_get_mode(dev, &mode);                       /* NULL: no modes */
    depz_vl53lx_budget_range(dev, &lo, &hi);
    depz_vl53lx_budget_choices(dev, choices, 16, &nc);      /* 0: any in range */
    n = depz_vl53lx_modes(dev, modes, DEPZ_VL53LX_MAX_MODES);
    printf("budget %d ms, period %d ms, mode %s, budgets %d..%d (%zu choices)\n",
           budget, inter, mode ? mode : "-", lo, hi, nc);
    for (k = 0; k < n; k++) printf("  mode %s\n", modes[k]);
}
```

`depz_vl53lx_configure()` re-initialises the sensor, then applies the mode,
the timing budget and any stored calibration (the `offset_mm` / `xtalk_kcps`
pointers, NULL = leave). Call it before every run: it is the only way to know
what the configuration registers hold. `inter_ms` 0 ranges back-to-back; a
larger value is the period between measurements and must exceed the budget.
Budgets here: any 2–550 ms. The budget reads back rounded (a 50 ms request
may read 49).

## Streaming and single shot

```c
#include <depz_sensor_io.h>
#include <stdio.h>

static void on_measurement(const depz_vl53lx_measurement *m, void *user)
{
    (void)user;  /* the reader thread: keep it short, never block */
    printf("%llu us: %d mm\n", (unsigned long long)m->timestamp_us, (int)m->distance_mm);
}

void shots(depz_device *dev)
{
    depz_vl53lx_measurement m;
    depz_stream *s;
    int token;
    depz_vl53lx_measure_once(dev, 1000, &m);   /* start, wait, read, stop */
    s = depz_vl53lx_measurements(dev, 64);     /* bounded, drop-oldest */
    depz_vl53lx_on_measurement(dev, on_measurement, NULL, &token);
    depz_vl53lx_start_ranging(dev);            /* one frame per INT edge */
    depz_stream_next(s, &m, 2000);
    depz_vl53lx_get_measurement(dev, 2000, &m); /* the next one, no stream needed */
    depz_vl53lx_stop_ranging(dev);
    depz_vl53lx_off_measurement(dev, token);
    depz_stream_close(s);
    printf("parse errors: %llu\n", (unsigned long long)depz_vl53lx_stream_parse_errors(dev));
}
```

The stream is INT-driven: the bridge reads the result block on every
data-ready edge and plays the driver's interrupt-release writes itself.
Configuration is refused while ranging (`DEPZ_E_ARG` "stop ranging first");
`depz_vl53lx_measure_once()` too. `depz_vl53lx_stream_parse_errors()` counts
streamed blocks that failed to decode — 0 in a healthy session.

## The measurement

`depz_vl53lx_measurement` has one shape for every product:

| field | meaning |
|---|---|
| `timestamp_us` | MCU µs at the INT edge (stream) / the read (poll) |
| `distance_mm` | distance, mm |
| `status`, `status_text` | 0 = valid; the text names the failure otherwise |
| `sigma_mm` | range standard-deviation estimate |
| `signal_kcps`, `ambient_kcps` | return-signal and ambient rates |
| `spads` | active SPADs |
| `n_targets`, `targets[]` | every return of a histogram frame (else 0) |
| `has_bins`, `bins` | the histogram frame (histogram driver) |
| `stream_count` | the sensor's frame counter (die ULDs, histogram; -1 otherwise) |
| `signal_per_spad_kcps`, `ambient_per_spad_kcps` | die ULDs |
| `dmax_mm`, `device_range_status` | VL53L0X (-1 otherwise) |
| `min_range_mm`, `max_range_mm`, `peak_bin` | histogram, targets[0] (-1 otherwise) |

`depz_vl53lx_plottable(&m)` accepts the statuses that mean "this distance is
real": 0, 6 (first histogram frame, no wrap check yet) and 11 (merged
target) — prefer it to `status == 0`.

## Several targets (histogram driver)

```c
#include <depz_sensor_io.h>
#include <stdio.h>

void targets(depz_device *dev)
{
    depz_vl53lx_measurement m;
    int32_t primary;
    size_t k;
    depz_vl53lx_init(dev, DEPZ_VL53LX_DRIVER_HISTOGRAM, DEPZ_VL53LX_PRODUCT_NONE);
    depz_vl53lx_configure(dev, 33, 0, "medium", NULL, NULL);
    depz_vl53lx_start_ranging(dev);
    if (depz_vl53lx_get_measurement(dev, 2000, &m) == DEPZ_OK) {
        for (k = 0; k < m.n_targets; k++)    /* up to four */
            printf("%d mm  status %d (%s)  signal %.0f kcps\n", (int)m.targets[k].distance_mm,
                   m.targets[k].status, m.targets[k].status_text, m.targets[k].signal_kcps);
        if (depz_vl53lx_primary_distance(&m, &primary))
            printf("readout: %d mm\n", (int)primary);
        if (m.has_bins)
            printf("%u bins, VCSEL period %u\n", m.bins.number_of_bins, m.bins.vcsel_period);
    }
    depz_vl53lx_stop_ranging(dev);
}
```

The histogram driver's presets pick how far the 24 bins reach before the
phase wraps: `short` 1.6 m, `medium` 2.4 m (what init leaves), `long` 4 m
(`depz_vl53lx_driver_reach_mm()`). The bin is 199 mm wide in all three.
On this driver `status == 0` is not the test — use `depz_vl53lx_plottable()`
or `depz_vl53lx_primary_distance()` (the first plottable target). The driver
steps an A/B frame-pair state and a phase-consistency history on every frame,
so each streamed frame is decoded exactly once, in order.

## What each driver supports

| driver | capability groups |
|---|---|
| `histogram` | timing, modes |

Ask `depz_vl53lx_supports(dev, DEPZ_VL53LX_CAP_*)` at run time; a call
outside the list returns `DEPZ_E_ARG` naming the missing group.

## Settings and calibration

The VL53L4CX's own driver (histogram) has no settings beyond timing and
mode: no calibrations and no thresholds. The borrowed light drivers have
them — see [Open and initialize](#open-and-initialize). On the lab board a
VL53L4CD-driver offset calibration at 600 mm from C brought the readout from
615 to 601 mm, and `configure()` re-applied it.

## Reset and bridge diagnostics

```c
#include <depz_sensor_io.h>
#include <stdio.h>

void diagnostics(depz_device *dev)
{
    depz_vl53lx_info info;
    if (depz_vl53lx_bridge_info(dev, &info) == DEPZ_OK)  /* safe while streaming */
        printf("%u edges, %u frames dropped, %u I2C errors, %u kHz\n", (unsigned)info.int_edges,
               (unsigned)info.frames_dropped, (unsigned)info.i2c_errors, (unsigned)info.i2c_khz);
    depz_vl53lx_xshut(dev, DEPZ_VL53L4_XSHUT_RESET);     /* power-cycle: init() again */
}
```

A power-cycled sensor holds none of the configuration or calibration.
`i2c_errors` counts from the end of the last init: a resetting die NACKs its
own address for a moment, so the class clears the counter once init /
configure is through (`CLEAR_I2C_ERRORS`, firmware `APP_VL53L0_4_v0.24` or
newer — on an older board init fails with a message saying to reflash it).
`depz_vl53lx_read_reg()` / `_write_reg()` are the raw escape hatch.

## Record and replay for tests

```c
#include <depz_sensor_io.h>

int replay(const char *path)
{
    depz_link *link;
    depz_device *dev;
    int rc = depz_link_open_replay(path, true, false, &link);  /* strict */
    if (rc) return rc;
    rc = depz_device_open_link(link, &dev);
    if (rc) return rc;
    rc = depz_device_promote(dev);          /* reads the device name, as live */
    if (!rc) rc = depz_vl53lx_init(dev, 0, DEPZ_VL53LX_PRODUCT_NONE);
    depz_device_close(dev);
    return rc;
}
```

The committed captures of this board (recorded by the Python SDK on
`APP_VL53L0_4_v0.24`, `contracts/vectors/recordings/`) replay strictly
through the class in `tests/test_io.c` — every byte the SDK sends is
compared with the recording, every frame's distance, status, targets and
bins with the sidecar:

- `vl53l4cx_histogram_medium_50ms` — `io_vlx_l4cx_hist_medium_replay`
- `vl53l4cx_histogram_short_33ms` — `io_vlx_l4cx_hist_short_replay`
- `vl53l4cx_uld_as_l4cd_50ms` — `io_vlx_l4cd_uld_replay`

The C and Python SDKs send the same requests in the same order, so a capture
from either replays in the other.

## Verification status

- **Live** on the lab VL53L4CX (`TOVJALN523`, `APP_VL53L0_4_v0.24`) from C,
  against a flat wall at 0.600 m: histogram `medium` 606 mm (20.4 Hz),
  `long` 603 mm (10.4 Hz), a single shot 599 mm; `short`: status 7 on every frame after the first
  (see [Gotchas](#gotchas)). Borrowed light drivers, uncalibrated:
  VL53L4CD ULD 615 mm, VL53L1CX ULD `long` 620 mm / `short` 614 mm; an
  offset calibration at 600 mm from C brought both to 601 mm, re-applied by
  `configure()`. No I2C errors, no dropped frames.
- **Replay** of the three captures above, strict, in C.

## Decode layer (your own transport)

Everything above is built from the codecs in `depz_sensor_sdk.h`; use them
directly if you run the DEPZ transport yourself. The snippets below build
command payloads and decode reply / report payloads; you own the serial port.

### The codec surface

| direction | id | what | codec |
|---|---|---|---|
| host → device | `0x32` / `0x33` | read / write registers | `depz_vl53l4_pack_read_reg` / `_write_reg` |
| host → device | `0x34` | XSHUT pin | `depz_vl53l4_pack_xshut` |
| host → device | `0x35` | start streaming | `depz_vl53lx_pack_start_stream` |
| host → device | `0x36` / `0x37` | stop / bridge info | empty payload |
| host → device | `0x38` | bus speed | `depz_vl53l4_pack_set_i2c_speed` |
| host → device | `0x39` | register-address width | `depz_vl53lx_pack_set_addr_width` |
| device → host | `0x91` | register data | `depz_vl53l4_unpack_reg_data` |
| device → host | `0x92` | bridge info (23 B) | `depz_vl53lx_unpack_info` |
| device → host | `0x93` | streamed block | `depz_vl53l4_unpack_stream` → a block decoder |

The opcodes are `DEPZ_VL53LX_CMD_*` / `DEPZ_VL53LX_RPT_*` (the unchanged ones
equal their `DEPZ_VL53L4_*` twins).

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
    static uint8_t frame[DEPZ_HEADER_SIZE + 2 + DEPZ_VL53L4_XFER_MAX + 4];
    size_t n;
    if (depz_build_packet(cmd, payload, len, seq++, DEPZ_CRC8,
                          frame, sizeof frame, &n) == 0)
        port_write(frame, n);
}
```

### Which product is this

Every 1D board runs the same firmware, so the product comes from the board:
the production USB PID, else the first `VL53L<digit>…` in the device name.
The product row then carries everything the bridge needs:

```c
#include <stdio.h>

/* vid/pid from your port enumeration; device_name from GET_DEVICE_NAME */
void print_product(int vid, int pid, const char *device_name)
{
    const char *usb_model = depz_usb_model_hint(vid, pid);          /* may be NULL */
    depz_vl53lx_product prod = depz_vl53lx_resolve_product(usb_model, device_name);
    const depz_vl53lx_product_info *pi = depz_vl53lx_product_get(prod);
    if (!pi) {
        printf("unknown / unstamped board\n");
        return;
    }
    printf("%s: model id 0x%04X, %u mm, default driver %s, "
           "%u-byte addresses, bus up to %u kHz\n",
           pi->name, (unsigned)pi->model_id, (unsigned)pi->reach_mm,
           depz_vl53lx_driver_str(pi->default_driver),
           (unsigned)pi->addr_width, (unsigned)pi->max_khz);
    if (pi->driver_kinds & DEPZ_VL53LX_DRIVER_HISTOGRAM)
        printf("  also runs the histogram driver\n");
}
```

For this board: `VL53L4CX`, model id `0xEBAA` (the same as the VL53L4CD —
the device name tells them apart), 6000 mm, driver `histogram`, 2-byte
addresses, one release step `0x0086 ← 0x01`, 1000 kHz after init.
`depz_vl53lx_resolve_class()` gives `"vl53l4cx"`. Borrowing a neighbour's
driver is explicit: `depz_vl53lx_product_get(DEPZ_VL53LX_PRODUCT_L4CD)` is the
row the borrowed light driver runs with (reach 1200 mm).

### Stream a block

`VL53_START_STREAM` names the block to read on every data-ready edge and the
interrupt-release writes the bridge plays after each read (up to
`DEPZ_VL53LX_CLEAR_STEPS_MAX`). Take them from the product row:

```c
/* after the driver has initialised and started the sensor */
void start_block_stream(depz_vl53lx_product prod, uint16_t addr, uint16_t len)
{
    const depz_vl53lx_product_info *pi = depz_vl53lx_product_get(prod);
    uint8_t p[DEPZ_VL53LX_START_STREAM_MAX];
    size_t n = depz_vl53lx_pack_start_stream(addr, len, 0 /* INT active low */,
                                             pi->clear, pi->n_clear, p);
    if (n)
        send_packet(DEPZ_VL53LX_CMD_START_STREAM, p, n);
}
```

Call it with `DEPZ_VL53LX_DIE_BLOCK_ADDR` / `DEPZ_VL53LX_DIE_BLOCK_LEN` for
the borrowed light driver or `DEPZ_VL53LX_HISTOGRAM_BLOCK_ADDR` /
`DEPZ_VL53LX_HISTOGRAM_BLOCK_LEN` for the histogram driver (both release with
`0x0086 ← 0x01`). The encoder returns `0` for more than four release steps.
Stop with `DEPZ_VL53LX_CMD_STOP_STREAM` and an empty payload.

### Decode the histogram block

The histogram driver — this part's own and default driver — streams the
83-byte block at `0x0088`:

```c
#include <stdio.h>

/* s: a depz_vl53l4_stream whose s.addr == DEPZ_VL53LX_HISTOGRAM_BLOCK_ADDR */
void print_bins(const depz_vl53l4_stream *s)
{
    depz_vl53lx_histogram_raw h;
    if (depz_vl53lx_decode_histogram_raw(s->data, s->len, &h) != 0)
        return;
    printf("frame %u  range status 0x%02X  %u SPADs  vcsel start %u\n",
           (unsigned)h.stream_count, (unsigned)h.range_status,
           (unsigned)h.dss_actual_effective_spads, (unsigned)h.vcsel_start);
    for (unsigned i = 0; i < DEPZ_VL53LX_HISTOGRAM_BINS; i++)
        printf("%u%c", (unsigned)h.bins[i],
               i + 1 < DEPZ_VL53LX_HISTOGRAM_BINS ? ' ' : '\n');
}
```

The decoder hands you the device's status bytes, the reference phase, the
VCSEL start and the 24 photon counts (24-bit each). Bin 23's low byte comes
from a separate MSB/LSB pair; the decoder rebuilds it before reading the bins
and leaves your buffer untouched. **Finding targets in the bins** — the
preset, the VCSEL period, the A/B frame pairs, the phase-consistency history
— is the class's job (the sections above).

### Decode the die block (borrowed VL53L4CD driver)

The VL53L4CX has one driver of its own — the histogram driver. The class can also run the die on the **VL53L4CD's light driver** (single
target, ~1.2 m, with calibrations and thresholds); it then streams the
17-byte die block at `0x0089`, read the VL53L4CD way — pass
`DEPZ_VL53LX_DIE_L4`:

```c
#include <stdio.h>

void on_stream(const depz_event *ev)
{
    if (ev->type != DEPZ_EV_PACKET || ev->cmd != DEPZ_VL53LX_RPT_STREAM)
        return;
    depz_vl53l4_stream s;
    if (depz_vl53l4_unpack_stream(ev->payload, ev->payload_len, &s) != 0
            || s.addr != DEPZ_VL53LX_DIE_BLOCK_ADDR)
        return;
    depz_vl53l4_result r;
    if (depz_vl53lx_decode_die_block(s.data, s.len, DEPZ_VL53LX_DIE_L4, &r) != 0)
        return;
    if (r.range_status == 0)
        printf("t=%llu us  %d mm  sigma %d mm  signal %d kcps  frame %d\n",
               (unsigned long long)s.timestamp_us, r.distance_mm, r.sigma_mm,
               r.signal_rate_kcps, r.stream_count);
    else
        printf("status %d\n", r.range_status);
}
```

The result is the VL53L4CD result struct: `range_status` through ST's status
table (0 = valid), `distance_mm`, `sigma_mm`, signal and ambient rates in
kcps, the per-SPAD rates, `number_of_spad` and the sensor's own frame
counter `stream_count` (wraps at 255). The `L4` variant is exactly
`depz_vl53l4_parse_result_block()`.
`timestamp_us` is MCU uptime at the INT edge.

### Valid or not

- **Die block** — `range_status == 0` is a valid distance; other values name
  why the measurement is suspect.
- **Histogram driver** — the family-wide rule the class applies to the
  targets they find: statuses **0, 6 and 11 are all plottable** (6 = first
  frame, no predecessor for the wrap check; 11 = merged pulse). Testing
  `status == 0` alone throws good frames away. The raw `range_status` byte in
  `depz_vl53lx_histogram_raw` is the device's own, not that status.
- **Decode each histogram frame exactly once, in order** — the target search
  steps an A/B frame-pair state per frame. `depz_vl53lx_decode_histogram_raw()`
  itself is stateless, but anything you build on the bins must see every
  frame once, in stream order.

### Bridge diagnostics

```c
#include <stdio.h>

/* answer to DEPZ_VL53LX_CMD_GET_INFO (empty payload); safe while streaming */
void on_info(const depz_event *ev)
{
    depz_vl53lx_info info;
    if (ev->type == DEPZ_EV_PACKET && ev->cmd == DEPZ_VL53LX_RPT_INFO
            && depz_vl53lx_unpack_info(ev->payload, ev->payload_len, &info) == 0)
        printf("%u edges, %u slots skipped, %u frames dropped, %u I2C errors, "
               "%u kHz, width %u, %u release steps\n",
               (unsigned)info.int_edges, (unsigned)info.slots_skipped,
               (unsigned)info.frames_dropped, (unsigned)info.i2c_errors,
               (unsigned)info.i2c_khz, (unsigned)info.addr_width,
               (unsigned)info.n_clear);
}
```

`slots_skipped` (a read slot that never got the bus), `frames_dropped` (a good
sample the USB link had no room for) and the fault latch reset at
`START_STREAM`; `i2c_errors` runs free and wraps — watch increments.

## Gotchas

- **`configure()` before every run** — it re-initialises the sensor, so nothing from an earlier session carries over (an armed detection window included).
- **Configuration while ranging is refused** — stop, reconfigure, restart.
- **Prefer `depz_vl53lx_plottable()` over `status == 0`** — non-zero statuses are data, not errors.
- **Don't use the `short` preset on the L4CX** — at any distance its frames alternate: one carries the true distance but status 7, the next a wrong one (flat wall, 2026-09-28: −341 mm at 0.15 and 0.3 m, −156 mm at 0.6 m; at 1.0 m the true 1008 mm comes flagged status 4 and the other frame reads 238 mm). A plottable filter drops them all; from C on 2026-09-29 at 0.6 m, every short frame after the first came back status 7. Same on firmware v0.23 and v0.24, in the Python SDK and with the firmware repo's own tool; the cause is not known yet. `medium` and `long` are clean — use them.
- **Same model id as the VL53L4CD** (`0xEBAA`) — the device name tells them apart.
- **The borrowed light drivers read long uncalibrated** — +15..20 mm at 0.6 m on the lab board; calibrate the offset (they have it) and re-apply it with `configure()`. The histogram driver has no calibration and needed none there (+3..6 mm).
- **Callbacks run on the reader thread** — keep them short and never call a blocking `depz_vl53lx_*` function from one.
- **`XSHUT` reset has no boot handshake on this bridge** — the class polls the boot register itself; on your own transport, do the same before the next access.
