# VL53L0X — user guide

Hands-on guide to the VL53L0X in C: the 1D-family sensor class
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
the USB PID or the board's device name (`… VL53L0X USB v2.1 …`).
`depz_vl53lx_init()` binds a (product, driver) pair and initialises the
sensor: product `DEPZ_VL53LX_PRODUCT_NONE` means the detected one, driver `0`
the product's first kind — `uld` here.

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

The model id (`0x00EE`) is a cross-check only: several products share
theirs. `depz_vl53lx_caveat()` says what the pair cannot do, "" when nothing.

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
        depz_vl53lx_configure(dev, 33, 0, "long-range", NULL, NULL) != DEPZ_OK) {
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
    depz_vl53lx_configure(dev, 33, 0, "long-range", NULL, NULL);   /* re-init + apply */
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
Budgets here: any 20–200 ms. The budget reads back rounded (a 50 ms request
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

## What each driver supports

| driver | capability groups |
|---|---|
| `uld` | timing, modes, offset, offset calibration, crosstalk, crosstalk calibration, reference-SPAD management |

Ask `depz_vl53lx_supports(dev, DEPZ_VL53LX_CAP_*)` at run time; a call
outside the list returns `DEPZ_E_ARG` naming the missing group.

## Settings and calibration

```c
#include <depz_sensor_io.h>
#include <stdio.h>

/* A flat target at a known distance; store what comes back. */
void calibrate(depz_device *dev, int target_mm)
{
    int32_t offset, xtalk;
    if (depz_vl53lx_calibrate_offset(dev, target_mm, 0, &offset) == DEPZ_OK)
        printf("offset %d mm\n", (int)offset);
    if (depz_vl53lx_calibrate_xtalk(dev, target_mm, 0, &xtalk) == DEPZ_OK)
        printf("crosstalk %d kcps\n", (int)xtalk);
    /* the sensor forgets both on reset: re-apply them with every configure */
    depz_vl53lx_configure(dev, 50, 0, NULL, &offset, &xtalk);
}
```

Calibrations run against a flat target (`nb_samples` 0 = the driver's
default) and return the value now programmed. It lives in sensor RAM: store
it on the host and pass it to `depz_vl53lx_configure()`.

`depz_vl53lx_perform_ref_spad_management(dev, &count, &is_aperture)`
re-measures the reference SPADs (normally read from the part's NVM at init).

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

- `vl53l0x_uld_long-range_33ms` — `io_vlx_l0x_uld_replay`

The C and Python SDKs send the same requests in the same order, so a capture
from either replays in the other.

## Verification status

- **Not run live from C** — no VL53L0X board was at hand. The driver is a
  register-for-register port of the Python SDK's, which was verified on a
  live board; the class around it was run live on a VL53L4CX.
- **Replay** of the captures above, strict, in C.

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

For this board: `VL53L0X`, model id `0x00EE`, 2000 mm, driver `uld` (ST's
VL53L0X API 1.0.4), **1-byte addresses**, two release steps `0x0B ← 0x01`,
`0x0B ← 0x00`, and a **400 kHz** bus ceiling. `depz_vl53lx_resolve_class()`
gives `"vl53l0x"`.

### Switch the bridge to 1-byte addresses

The VL53L0X is the only family member with 8-bit register addresses. The
width is a bridge setting: sticky, back to 2 after a reset, and it must be
set **before the first register access of a session** (the model-id read
included) and never while a stream runs:

```c
void use_1byte_addresses(void)
{
    uint8_t p[4];
    size_t n = depz_vl53lx_pack_set_addr_width(1, p);   /* returns 0 unless 1 or 2 */
    send_packet(DEPZ_VL53LX_CMD_SET_ADDR_WIDTH, p, n);

    /* now read the model id at 0xC0 (expect 0xEE): at width 1 only the low
     * address byte goes on the bus and addr + len must stay <= 0x100 */
    n = depz_vl53l4_pack_read_reg(0xC0, 1, p);
    send_packet(DEPZ_VL53LX_CMD_READ_REG, p, n);
}
```

The reply is `RPT_VL53_REG_DATA` (`0x91`), decoded with
`depz_vl53l4_unpack_reg_data()`.

### Stream the result block

The VL53L0X driver streams the 12-byte block at `0x14` and releases the
interrupt with two writes to `0x0B` — both come from the product row:

```c
void start_l0x_stream(void)
{
    const depz_vl53lx_product_info *pi = depz_vl53lx_product_get(DEPZ_VL53LX_PRODUCT_L0X);
    uint8_t p[DEPZ_VL53LX_START_STREAM_MAX];
    size_t n = depz_vl53lx_pack_start_stream(DEPZ_VL53LX_L0X_BLOCK_ADDR,
                                             DEPZ_VL53LX_L0X_BLOCK_LEN, 0,
                                             pi->clear, pi->n_clear, p);
    send_packet(DEPZ_VL53LX_CMD_START_STREAM, p, n);   /* 6 + 3 x 2 = 12 B */
}
```

### Decode the result block

`depz_vl53lx_decode_l0x_raw()` returns the raw fields of the block — the
distance, the device's range-status byte, the signal and ambient rates as
16.16 fixed point (Mcps) and the effective SPAD count as 8.8 fixed point:

```c
#include <stdio.h>

void on_stream(const depz_event *ev)
{
    if (ev->type != DEPZ_EV_PACKET || ev->cmd != DEPZ_VL53LX_RPT_STREAM)
        return;
    depz_vl53l4_stream s;
    if (depz_vl53l4_unpack_stream(ev->payload, ev->payload_len, &s) != 0
            || s.addr != DEPZ_VL53LX_L0X_BLOCK_ADDR)
        return;
    depz_vl53lx_l0x_raw r;
    if (depz_vl53lx_decode_l0x_raw(s.data, s.len, &r) != 0)
        return;
    printf("t=%llu us  %u mm  device status 0x%02X  signal %.3f Mcps  "
           "ambient %.3f Mcps  %.2f SPADs\n",
           (unsigned long long)s.timestamp_us, (unsigned)r.distance_raw,
           (unsigned)r.device_range_status,
           r.signal_rate_mcps_1616 / 65536.0, r.ambient_rate_mcps_1616 / 65536.0,
           r.effective_spad_count_88 / 256.0);
}
```

`distance_raw` is millimetres (quarter-millimetres if the driver enabled
fractional ranging). The **range status, sigma and dmax** of the ST API need
device data the driver caches at init (SPAD and reference calibration), so
they are the class's — the raw `device_range_status` byte is what the
sensor wrote, not the final status.

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
- **No thresholds, ROI or temperature update** on this part — `depz_vl53lx_supports()` says so; the calls return `DEPZ_E_ARG`.
- **A NACK or two right after a reset is normal** — the die is still booting; the driver waits them out, and the class clears the bridge's I2C error counter once init is through.
- **Stay at 400 kHz** — this die has no Fast Mode Plus; the driver never raises the bus.
- **Uncalibrated it reads long** — the Python SDK measured about +3 cm on the lab board; calibrate the offset against a flat target and re-apply it with `configure()`.
- **Callbacks run on the reader thread** — keep them short and never call a blocking `depz_vl53lx_*` function from one.
- **`XSHUT` reset has no boot handshake on this bridge** — the class polls the boot register itself; on your own transport, do the same before the next access.
