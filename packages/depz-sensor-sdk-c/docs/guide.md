# depz-sensor-sdk-c — guide

The common guide to the **C** SDK for the DEPZ USB sensor line. It covers what
the SDK is, how to build it with CMake, the live-hardware layer (devices,
errors, threading, streams, discovery, record/replay), the transport/decode
mental model and the per-sensor decode layers. Each sensor then has
its own **introduction** and **user guide**:

- **SR04** (HC-SR04 ultrasonic) — [introduction](sr04/introduction.md) ·
  [guide](sr04/guide.md) · [api](sr04/api.md)
- **VL53L4CD** (single-zone ToF) — [introduction](vl53l4cd/introduction.md) ·
  [guide](vl53l4cd/guide.md) · [api](vl53l4cd/api.md)
- **VL53L8CX** (8×8 ToF base) — [introduction](vl53l8cx/introduction.md) ·
  [guide](vl53l8cx/guide.md) · [api](vl53l8cx/api.md)
- **VL53L8CH** (ToF superset + CNH histograms) —
  [introduction](vl53l8ch/introduction.md) · [guide](vl53l8ch/guide.md) ·
  [api](vl53l8ch/api.md)
- **VL53L5CX** / **VL53L7CX** / **VL53L7CH** (8×8 ToF, I2C board) —
  [L5CX](vl53l5cx/introduction.md) ([guide](vl53l5cx/guide.md) ·
  [api](vl53l5cx/api.md)) · [L7CX](vl53l7cx/introduction.md)
  ([guide](vl53l7cx/guide.md) · [api](vl53l7cx/api.md)) ·
  [L7CH](vl53l7ch/introduction.md) ([guide](vl53l7ch/guide.md) ·
  [api](vl53l7ch/api.md))
- **VL53L 1D family** (single-zone / multi-target ToF) —
  [L0X](vl53l0x/introduction.md) ([guide](vl53l0x/guide.md) ·
  [api](vl53l0x/api.md)) · [L1CX](vl53l1cx/introduction.md)
  ([guide](vl53l1cx/guide.md) · [api](vl53l1cx/api.md)) ·
  [L1CB](vl53l1cb/introduction.md) ([guide](vl53l1cb/guide.md) ·
  [api](vl53l1cb/api.md)) · [L3CX](vl53l3cx/introduction.md)
  ([guide](vl53l3cx/guide.md) · [api](vl53l3cx/api.md)) ·
  [L4CX](vl53l4cx/introduction.md) ([guide](vl53l4cx/guide.md) ·
  [api](vl53l4cx/api.md))
- **BNO086** (9-axis IMU) — [introduction](bno086/introduction.md) ·
  [guide](bno086/guide.md) · [api](bno086/api.md)
- **BNO055** (9-axis IMU, on-chip fusion) —
  [introduction](bno055/introduction.md) · [guide](bno055/guide.md) ·
  [api](bno055/api.md)

For the exhaustive symbol-by-symbol reference see [api.md](api.md), generated
from the two headers' doc-comments so it never drifts from the code.

## Contents

- [What it is](#what-it-is)
- [What it is not](#what-it-is-not)
- [Build & install](#build--install)
- [Live layer](#live-layer)
  - [Open and close a device](#open-and-close-a-device)
  - [Errors and timeouts](#errors-and-timeouts)
  - [Threads and callbacks](#threads-and-callbacks)
  - [Common commands and time sync](#common-commands-and-time-sync)
  - [Events and link stats](#events-and-link-stats)
  - [Streams](#streams)
  - [Discovery](#discovery)
  - [Links: record, replay, loopback](#links-record-replay-loopback)
- [Mental model](#mental-model)
- [Transport: framing in, packets out](#transport-framing-in-packets-out)
- [Identity & discovery](#identity--discovery)
- [Common command & report codecs](#common-command--report-codecs)
- [Per-sensor decode](#per-sensor-decode)
- [Firmware container & bootloader](#firmware-container--bootloader)
- [Datasets](#datasets)
- [Testing](#testing)

## What it is

`depz-sensor-sdk-c` is a **contract-first C11 SDK** for the DEPZ USB sensor
line. Every DEPZ sensor is a USB CDC-ACM device speaking one shared framed
protocol (`A5 C3` header + optional payload CRC). The SDK is byte-exact with the
`contracts/` docs and the golden vectors in `contracts/vectors/`, and comes in
two layers, one public header each:

- **Codecs — [`include/depz_sensor_sdk.h`](../include/depz_sensor_sdk.h).**
  Turns the byte stream into typed C values and packs the command/config
  payloads that go the other way: framing, identity, the common codecs and the
  per-sensor decode layer. Pure functions over buffers you supply; no
  dependencies beyond libm. This is the C counterpart of the Python SDK's
  protocol core.
- **Live hardware — [`include/depz_sensor_io.h`](../include/depz_sensor_io.h).**
  Everything that talks to a board: byte links (serial port, loopback,
  `.depzrec` record/replay, your own), the device core (reader thread,
  request/response correlation, events, bounded streams, the common commands,
  time sync), discovery, and the sensor classes. Linux, macOS and Windows. It
  includes `depz_sensor_sdk.h`, so one `#include` gives you both.

The live layer covers the **common commands on every board**; full **sensor
classes arrive one sensor at a time — the SR04, the VL53L4CD, the
multizone ToF boards (VL53L8CX / CH, VL53L5CX / L7CX / L7CH) and the BNO055
have one today**. The
[API reference](api.md) is generated straight from both headers' doc-comments.

The sensors follow three firmware philosophies:

- **SR04** — the device does the ranging; you decode `echo_time_us` → distance.
- **VL53L4CD** — the device is a thin I2C register bridge; the ST ULD 2.2.3
  (init, range timing, tuning, calibration, result-block decode) runs **on the
  host** over plain register reads/writes — the live class ports it register
  for register.
- **VL53L8CX / VL53L8CH** — the device is a thin SPI register bridge; the ST
  ULD runs **on the host**, the ~84 KB sensor-firmware download included, and
  the ranging frames are reassembled and decoded there — one live class for
  both. CX is the base ToF imager; CH is its superset, adding
  Compact-Network-Histogram (CNH) output.
- **VL53L5CX / VL53L7CX / VL53L7CH** — one I2C register-bridge firmware for
  three 8×8 ToF boards; the ST ULD runs **on the host** in the same live
  class as the VL53L8 (its L5/L7 branch), and the frames are reassembled and
  decoded there (L7CH adds CNH histograms).
- **VL53L0X / L1CX / L1CB / L3CX / L4CX** — one I2C register-bridge firmware
  for the whole 1D family; every ST driver of the family runs **on the host**
  in one live class (`depz_vl53lx_*`), with the same register traffic as the
  Python SDK — the VL53L0X API, the VL53L1X / VL53L4CD ULDs, the VL53L3CX
  ULP, and the histogram driver that finds up to four targets in 24 bins.
- **BNO085 / BNO086** — the device is an SHTP pass-through; the whole SH-2
  sensor-hub protocol runs **on the host** — the live class enables outputs,
  matches the hub's answers by their content and parses the reports, with the
  same requests as the Python SDK.
- **BNO055** — the device is a thin register bridge; Bosch's fusion runs on
  the chip, and the mode / unit / calibration / page logic, the register
  codecs and the window decode run **on the host** — the live class does the
  sequencing (mode switches through CONFIG, the boot and fusion-start waits,
  page bookkeeping) with the same register traffic as the Python SDK.

## What it is not

- **Not a firmware updater.** Every board of the line has a sensor class —
  the **SR04**, the **VL53L4CD**, the multizone ToF boards (**VL53L8CX /
  VL53L8CH**, **VL53L5CX / VL53L7CX / VL53L7CH**), the **VL53L 1D family**
  (**VL53L0X / L1CX / L1CB / L3CX / L4CX**), the **BNO055** and the **BNO085 /
  BNO086** — but flashing firmware is left to the transport you own (see
  [Firmware container & bootloader](#firmware-container--bootloader)). A
  board of an unknown firmware opens as a *plain device*: discovery, the
  common commands and time sync work, and `depz_device_request()` with a
  matcher can drive its request/reply commands using the codecs of
  `depz_sensor_sdk.h`; its unsolicited reports only surface as truncated hex
  `DEPZ_DEV_EV_TEXT` events.
- **The codec header is not tied to the live layer.** `depz_sensor_sdk.h` has
  no I/O, no threads and no device objects, so you can own the transport
  yourself (your own serial code, an RTOS, a network bridge) and use only the
  codecs — build with `-DDEPZ_SENSOR_SDK_C_BUILD_IO=OFF` to drop the OS
  dependencies altogether.

Everything here is verifiable against golden vectors or a replayed capture, so
what ships is exactly what the tests exercise.

## Build & install

C11. The codec layer needs only libm (`round()` in the dataset/advanced
codecs); the live layer adds threads and the OS port-enumeration API. Build the
static library (`libdepz_sensor_sdk_c.a`), the examples and the tests with
CMake:

```sh
cmake -B build -S .
cmake --build build
ctest --test-dir build          # 83/83: 24 vector/replay + 59 live-layer
```

| Option | Default | Effect |
|--------|---------|--------|
| `DEPZ_SENSOR_SDK_C_BUILD_IO` | `ON` | The live layer (`depz_sensor_io.h`, `src/io/`). `OFF` = codecs only, no OS dependencies. |
| `DEPZ_SENSOR_SDK_C_BUILD_EXAMPLES` | `ON` standalone | `depz_list` and `sr04_minimal` from `examples/`. |
| `DEPZ_SENSOR_SDK_C_BUILD_TESTS` | `ON` standalone | The CTest suite (needs `contracts/vectors/`). |

With the live layer the target links, as `PUBLIC` dependencies that reach your
executable through `depz::sensor_sdk_c`: `Threads::Threads` on Linux and
macOS, `-framework IOKit -framework CoreFoundation` on macOS, and `setupapi` +
`cfgmgr32` on Windows. CI builds and tests all three.

To consume it from your own CMake project, pull it in with FetchContent and link
the namespaced target `depz::sensor_sdk_c` — this builds the library only, never
the test suite:

```cmake
include(FetchContent)
FetchContent_Declare(
  depz_sensor_sdk_c
  GIT_REPOSITORY https://github.com/depz-ai/depz-sensor-sdk.git
  GIT_TAG        v0.3.0
  SOURCE_SUBDIR  packages/depz-sensor-sdk-c
)
FetchContent_MakeAvailable(depz_sensor_sdk_c)
target_link_libraries(my_app PRIVATE depz::sensor_sdk_c)
```

```c
#include <depz_sensor_sdk.h>   /* codecs only */
#include <depz_sensor_io.h>    /* live layer (includes the codecs) */
```

The library also ships an installable CMake package
(`find_package(depz-sensor-sdk-c CONFIG)`) plus Conan and vcpkg recipes — see
the [README](../README.md#install) for those. The `docs/` reference is
regenerated with `make docs` (see [below](#testing)).

## Live layer

`depz_sensor_io.h` stacks three pieces:

```
depz_link      bytes in / out: serial port, loopback pair, .depzrec recording
    │          or replay, or your own vtable
    ▼
depz_device    reader thread + depz_parser, request/response correlation,
    │          common commands, time sync, events, link stats
    ▼
sensor class   the sensor's own commands and measurements (depz_sr04_*,
               depz_vl53l4cd_*, depz_vl53l8_*, depz_bno055_*)
```

A two-minute tour is in `examples/`: [`depz_list.c`](../examples/depz_list.c)
lists every serial port and every DEPZ board it finds, and
[`sr04_minimal.c`](../examples/sr04_minimal.c) streams live SR04 distance.

### Open and close a device

- **`depz_open_device(opt, &dev)`** — find, probe and open a board with its
  sensor class attached. `opt == NULL` takes the candidate with the smallest
  USB serial; otherwise start from `DEPZ_OPEN_OPTIONS_INIT` and set `port`
  (exactly this port), `serial` (this USB serial), `index` (the Nth candidate
  by USB serial) and/or `timeout_ms` (the request timeout).
- **`depz_device_open(port, &dev)`** — a plain device, no identity probe: the
  sensor type stays `DEPZ_SENSOR_NONE` until `depz_device_promote(dev)` asks
  the software name and attaches the matching class.
- **`depz_device_open_link(link, &dev)`** / **`depz_sr04_open_link(link, &dev)`**
  / **`depz_vl53l4cd_open_link(link, &dev)`** /
  **`depz_vl53l8_open_link(link, model, &dev)`** /
  **`depz_bno055_open_link(link, &dev)`** — the same on any
  [link](#links-record-replay-loopback) (tests, replay).

A device **owns its link**: `depz_device_close()` stops the reader thread,
closes the link and frees both (`NULL` is a no-op). The `_open_link` calls take
ownership of the link even when they fail.

```c
#include <depz_sensor_io.h>
#include <stdio.h>

int main(void)
{
    depz_open_options opt = DEPZ_OPEN_OPTIONS_INIT;
    depz_device *dev;
    const char *type;

    opt.port = "/dev/ttyACM0";            /* or opt.serial = "..." / opt.index = 1 */
    if (depz_open_device(&opt, &dev) != DEPZ_OK) {
        fprintf(stderr, "open: %s\n", depz_last_error());
        return 1;
    }
    type = depz_sensor_type_str(depz_device_sensor_type(dev));
    printf("%s on %s\n", type ? type : "unknown", depz_device_port(dev));
    depz_device_close(dev);
    return 0;
}
```

A board in bootloader mode fails the open with `DEPZ_E_BOOTLOADER`.

### Errors and timeouts

Every call returns `0` (`DEPZ_OK`) or a negative `depz_err`. After a failure the
**calling thread** can ask for details:

- `depz_err_str(rc)` — the constant's name (`"DEPZ_E_TIMEOUT"`);
- `depz_last_error()` — a human-readable message;
- `depz_last_status()` / `depz_last_status_cmd()` — for `DEPZ_E_STATUS` and
  `DEPZ_E_BUSY`, the device's `depz_status` code and the opcode it answered
  (`-1` when the last failure was something else).

The codes that matter day to day: `DEPZ_E_TIMEOUT` (no reply in time),
`DEPZ_E_STATUS` (the device answered a non-OK status), `DEPZ_E_BUSY`
(`ERR_BUSY`, or the same opcode is already in flight), `DEPZ_E_CLOSED` (link
closed or device lost), `DEPZ_E_WRONG_TYPE` (a sensor call on a device of
another type), `DEPZ_E_NO_DEVICE` (discovery found nothing), and
`DEPZ_E_REPLAY_MISMATCH` (strict replay saw different bytes).

```c
static void report(int rc)
{
    fprintf(stderr, "%s: %s", depz_err_str(rc), depz_last_error());
    if (rc == DEPZ_E_STATUS || rc == DEPZ_E_BUSY)
        fprintf(stderr, " (status %d for cmd 0x%02X)", depz_last_status(),
                depz_last_status_cmd());
    fputc('\n', stderr);
}
```

Timeouts are milliseconds. A **negative timeout means "the default"**: the
device's request timeout, `DEPZ_DEFAULT_TIMEOUT_MS` (200 ms) unless changed with
`depz_device_set_timeout_ms()` or `depz_open_options.timeout_ms`. Calls whose
reply naturally takes longer pick their own default (an SR04 or VL53L4CD
single shot waits 1000 ms, the VL53L4CD's and VL53L8's register transfers
2000 ms, a VL53L8 `get_frame` 2000 ms).

### Threads and callbacks

- Each open device runs **one reader thread**. It parses the byte stream,
  completes waiting requests, and delivers events and measurements.
- **Callbacks run on that reader thread.** Keep them short, never block in
  them (a request blocks — it waits for a reply only this thread can deliver),
  and **never call `depz_device_close()` from one**. Copy the data out, or use
  a [stream](#streams) and pull from your own thread instead.
- A **matcher** (`depz_device_request`) runs on the reader thread *under the
  device lock*: copy what you need into its `ctx` and return — nothing else.
- Requests may come from several threads at once, but **one request per
  opcode is in flight**: a second request with the same opcode fails at once
  with `DEPZ_E_BUSY`; a different opcode proceeds.
- Error details (`depz_last_error()` and friends) are **per thread**.

### Common commands and time sync

Every board answers the contract-02 common set, with or without a sensor class:

```c
char sw[64], name[128], serial[64];
double mcu_c;
depz_time_sync ts;

depz_device_get_software_name(dev, sw, sizeof sw);        /* "APP_usonic_SR04_v0.97" */
depz_device_get_device_name(dev, name, sizeof name);
depz_device_get_serial_number(dev, serial, sizeof serial);
depz_device_read_mcu_temperature(dev, &mcu_c);            /* cached, refreshed ~2 Hz */

if (depz_device_sync_time(dev, 8, &ts) == DEPZ_OK)        /* 8 round trips, lowest RTT kept */
    printf("offset %lld us, rtt %lld us\n", (long long)ts.offset_us, (long long)ts.rtt_us);
```

After a sync, `depz_device_to_host_time_us(dev, device_us, &host_us)` maps a
report's device timestamp onto the host monotonic clock (`depz_host_now_us()`);
before the first sync it fails with `DEPZ_E_ARG`. The rest of the set:
`depz_device_get_payload_crc_type` / `_set_payload_crc_type`,
`depz_device_get_sync_pin` / `_set_sync_pin`, `depz_device_reset` (the link
drops as the device reboots) and `depz_device_enter_bootloader`.

For a command the SDK has no wrapper for, `depz_device_request()` sends any
opcode and waits for a matcher to claim the reply (or, with `ok_completes`, for
`RPT_STATUS(cmd, OK)`); `depz_device_send()` is fire-and-forget.

```c
typedef struct { uint8_t buf[64]; size_t len; } reply;

static bool match_period(uint8_t cmd, const uint8_t *p, size_t len, void *ctx)
{
    reply *r = (reply *)ctx;
    if (cmd != DEPZ_SR04_RPT_SAMPLE_PERIOD) return false;
    r->len = len < sizeof r->buf ? len : sizeof r->buf;
    memcpy(r->buf, p, r->len);                 /* copy only: we hold the lock */
    return true;
}

static int read_period(depz_device *dev, uint32_t *period_us)
{
    reply r = {{0}, 0};
    int rc = depz_device_request(dev, DEPZ_SR04_GET_SAMPLE_PERIOD, NULL, 0,
                                 match_period, &r, false, -1);
    if (rc) return rc;
    return depz_sr04_unpack_sample_period(r.buf, r.len, period_us) ? DEPZ_E_PROTOCOL : DEPZ_OK;
}
```

### Events and link stats

Everything the device says that is not a reply or a sensor measurement becomes
a `depz_device_event`: `SEQUENCE_ERROR`, `CRC_ERROR`, `TRASH` (bytes that were
not a packet), `UNSOLICITED_STATUS` (`ERR_HARDWARE_FAULT` is a fault), `TEXT`
(`RPT_TEXT`, and unrouted reports as hex text), `TEMPERATURE`, and
`DISCONNECTED` (`text` = the reason). Take them as a callback or as a stream:

```c
static void on_event(const depz_device_event *ev, void *user)
{
    (void)user;
    if (ev->type == DEPZ_DEV_EV_DISCONNECTED)
        fprintf(stderr, "lost: %s\n", ev->text);  /* reader thread: no close here */
}

int token;
depz_device_on_event(dev, on_event, NULL, &token);
/* ... */
depz_device_off_event(dev, token);

depz_stream *evs = depz_device_events(dev, 64);   /* or pull them */
```

`depz_device_stats()` fills a `depz_link_stats` with packet / byte counters,
CRC and header errors, trash bytes, host-observed sequence gaps and the
device-reported sequence errors.

### Streams

A `depz_stream` is a **bounded, drop-oldest** pull queue: the reader thread
pushes, you pull. When it is full the oldest item goes and
`depz_stream_dropped_count()` grows, so a slow consumer never stalls the
device and never grows memory. A stream is registered when it is created —
nothing produced after the call is missed — and any number of independent
streams can hang off one device. `maxsize == 0` picks the default (256).

```c
depz_sr04_measurement m;
depz_stream *s = depz_sr04_stream(dev, 64);        /* item type fixed by the stream */
int rc;

while ((rc = depz_stream_next(s, &m, 500)) != DEPZ_E_CLOSED) {
    if (rc == DEPZ_E_TIMEOUT) continue;            /* nothing in 500 ms */
    /* use m */
}
printf("%llu dropped\n", (unsigned long long)depz_stream_dropped_count(s));
depz_stream_close(s);
```

`depz_stream_next()` returns `DEPZ_OK`, `DEPZ_E_TIMEOUT`, or `DEPZ_E_CLOSED`
once the device is closed or lost **and** the queue is drained — items that
arrived before a disconnect are still handed out. `depz_stream_close()` may be
called before or after closing the device.

### Discovery

```c
depz_device_info *devs;
size_t n, i;

if (depz_list_depz_devices(true, 300, &devs, &n) == DEPZ_OK) {
    for (i = 0; i < n; i++)
        printf("%s  %s  fw %s  serial %s\n", devs[i].port, devs[i].mode,
               devs[i].fw_version, devs[i].serial_number);
    depz_free_device_list(devs);
}
```

- `depz_list_serial_ports()` — every serial port the OS lists, with its USB
  vid/pid and iSerial where known (`-1` / `""` otherwise); free with
  `depz_free_port_list()`.
- `depz_probe_port(port, timeout_ms, &info)` — open one port, ask the software
  name (+ device name and serial), close. `DEPZ_E_NO_DEVICE` when nothing
  DEPZ-shaped answers.
- `depz_list_depz_devices(match_usb, …)` — probe the candidates (with
  `match_usb`, only known DEPZ USB ids — see `depz_is_known_depz_usb()`),
  ordered by USB serial. `mode` is `"app"`, `"bootloader"` or `"unknown"`.
- `depz_open_device()` — the same search, then open with the class attached.

Serial ports open **exclusively**: a second opener fails instead of silently
sharing the byte stream. On POSIX the tty is left in a sane state on close, so
the next program (a browser's Web Serial, say) can use it.

### Links: record, replay, loopback

A `depz_link` is the byte pipe under a device. Besides
`depz_link_open_serial(port, &link)`:

- **Recording** — `depz_link_open_recording(inner, path, extra_json, &out)`
  tees `inner` into a `.depzrec` file (contract 08). It takes ownership of
  `inner`, also on failure.
- **Replay** — `depz_link_open_replay(path, strict_tx, realtime, &out)` serves
  the recorded rx side causally: a chunk is released only once the host has
  written as much as preceded it. `strict_tx` fails any write that differs
  from the recorded one (`DEPZ_E_REPLAY_MISMATCH`); `realtime` paces rx by
  the recorded times. `depz_link_replay_exhausted()` says when it is all
  served.
- **Loopback** — `depz_link_loopback_pair(&a, &b)`: what one writes the other
  reads; the test suite runs a fake SR04 (or a fake VL53L4CD bridge) on the
  far end.
- **Your own** — fill a `depz_link_vtable` (`read`, `write`, `close`,
  `destroy`) and wrap it with `depz_link_new()`.

```c
depz_link *serial, *rec, *rp;
depz_device *dev;

/* Record a session on the bench... */
if (depz_link_open_serial("/dev/ttyACM0", &serial) == DEPZ_OK &&
    depz_link_open_recording(serial, "bench.depzrec", "\"note\":\"bench\"", &rec) == DEPZ_OK &&
    depz_device_open_link(rec, &dev) == DEPZ_OK) {
    depz_device_promote(dev);                 /* attach the sensor class */
    /* ... the calls under test ... */
    depz_device_close(dev);                   /* closes rec and serial */
}

/* ...and replay it anywhere, byte-exact, with no device attached. */
if (depz_link_open_replay("bench.depzrec", true, false, &rp) == DEPZ_OK &&
    depz_device_open_link(rp, &dev) == DEPZ_OK) {
    depz_device_promote(dev);
    /* ... the same calls, in the same order, get the recorded answers ... */
    depz_device_close(dev);
}
```

## Mental model

This is the **decode layer**'s model — what you use when you own the transport
yourself, and what the live layer runs inside its reader thread. There are no
callbacks or streams here: you drive a small state machine and call decoders.
The shape of every integration is the same:

```
your serial read()  ──►  depz_parser_feed(&parser, bytes, n, on_event, ctx)
                                   │  (emits one depz_event per complete frame)
                                   ▼
                         on_event(ev): switch (ev->cmd)
                                   │
              ┌────────────────────┼─────────────────────┐
        common report        SR04 / VL53L4 data     VL53L8 chunk / BNO SHTP
    depz_unpack_status(…)   depz_sr04_unpack_data   depz_vl53l8_reasm_feed(…)
    depz_unpack_sync_time   depz_vl53l4_unpack_      → depz_vl53l8_decode_frame
    …                       stream(…)               depz_shtp_feed → depz_bno_*

your serial write()  ◄──  depz_build_packet(cmd, payload, …)
                                   ▲
                       depz_*_pack_*(…)  (encode the payload first)
```

- **One parser drains bytes.** `depz_parser_feed()` is fed whatever your serial
  read returns (any chunking) and invokes a callback once per complete frame,
  emitting `DEPZ_EV_PACKET`, `DEPZ_EV_TRASH`, or `DEPZ_EV_CRC_ERROR`. Event
  ordering is invariant to how the byte stream is chunked (contract 01 §5).
- **You correlate replies yourself.** The codec layer has no request/reply
  machinery (the live layer's `depz_device_request()` does it for you): a
  solicited reply is just a `DEPZ_EV_PACKET` whose `cmd` echoes the command byte
  you sent. Match on `ev->cmd` (RPT ids are ≥ 0x80).
- **Decoders are pure.** Each `depz_*_unpack_*` / `depz_*_decode_*` takes the
  payload pointer + length and fills a caller-owned struct, returning 0 on
  success or negative on a length/format error. Encoders (`depz_*_pack_*`) write
  a payload into your buffer and return its length.
- **Timestamps are device microseconds.** The reports carry `timestamp_us` in the
  device clock; compute the host offset with `depz_sync_time_offset_rtt()` from a
  SYNC_TIME round trip.

## Transport: framing in, packets out

The framing layer (contract 01) is the foundation every sensor sits on:

```c
uint8_t frame[DEPZ_MAX_FRAME];
size_t n;
depz_build_packet(DEPZ_CMD_GET_SERIAL, NULL, 0, seq++, DEPZ_CRC_NONE,
                  frame, sizeof frame, &n);
serial_write(fd, frame, n);           // -> device

depz_parser p;
depz_parser_init(&p);
for (;;) {
    uint8_t buf[512];
    ssize_t got = serial_read(fd, buf, sizeof buf);
    depz_parser_feed(&p, buf, (size_t)got, on_event, &ctx);   // -> callbacks
}
depz_parser_free(&p);
```

The header sets the CRC-type bits even for an empty payload, but appends a CRC
trailer only for a non-empty payload (ERRATA E6) — `depz_build_packet` handles
that for you. The four CRCs (`depz_crc8_maxim`, `depz_crc16_modbus`,
`depz_crc32_iso_hdlc`, and the file-only `depz_crc16_ccitt_false`) are exposed
directly if you need them.

## Identity & discovery

The live layer [enumerates and probes ports](#discovery) for you. Underneath,
the codec layer gives you the identity primitives to select and classify a
device on a transport of your own:

- `depz_is_known_depz_usb(vid, pid)` — true for a recognized DEPZ (or
  dev-default) USB id; `depz_usb_model_hint(vid, pid)` returns a best-guess model
  name. Use these against the (vid, pid) your OS reports for a candidate port.
- `depz_parse_software_name()` classifies a `GET_NAME_ACTIVE_SOFTWARE` string
  (first stripped with `depz_strip_device_string`) into a `depz_identity`
  (`app`/`bootloader`/`unknown` mode + `depz_sensor_type`). Both VL53L8 variants
  report the same `VL53L8` firmware identity, so the CX/CH split is named
  separately by `depz_vl53l8_variant`; the live layer takes it from the USB PID
  (`0xED40` = VL53L8CH). The L5/L7 boards (resolved by the live layer the same
  way, falling back to the device name) and the 1D family likewise
  share one firmware each (`DEPZ_SENSOR_VL53L7`, `DEPZ_SENSOR_VL53LX`), so the
  board is resolved from the USB PID or the device name:
  `depz_vl53l7_resolve_model()`, `depz_vl53lx_resolve_product()` /
  `depz_vl53lx_resolve_class()`.

## Common command & report codecs

Every sensor shares the command/report set in contract 02: device name, serial,
active-software name, MCU temperature, time-sync, payload-CRC mode, and the AUX
sync-pin config. The opcode/report/status enums (`depz_cmd`, `depz_rpt`,
`depz_status`) plus the pack/unpack helpers (`depz_pack_sync_time`,
`depz_unpack_temperature`, `depz_unpack_status`, …) live in the
[Common protocol](api.md#common-protocol) section. Time-sync math is
`depz_sync_time_offset_rtt()` (NTP-style offset + RTT, contract 02 §5).

## Per-sensor decode

Each sensor adds its own opcodes and decoders on top of the common set — follow
the per-sensor guide:

- **[SR04](sr04/guide.md)** — the live sensor class (`depz_sr04_*`: config,
  single shots, the loop as callbacks or streams) and, underneath, the
  sample-period / echo-decay config codecs, the `depz_sr04_data` measurement,
  and `depz_sr04_distance_mm()` (echo → mm).
- **[VL53L4CD](vl53l4cd/guide.md)** — the live sensor class
  (`depz_vl53l4cd_*`: the host ULD — init, timing, offset/xtalk, thresholds,
  calibration, single shots, the INT-driven stream) and, underneath, the I2C
  register-bridge codecs (`depz_vl53l4_pack_read_reg` / `_write_reg`, stream +
  info decode) and the host-ULD math (`depz_vl53l4_parse_result_block`, range
  timing, tuning words).
- **[VL53L8CX](vl53l8cx/guide.md)** — the live sensor class (`depz_vl53l8_*`:
  the host ULD — init with the firmware download, configuration, power modes,
  crosstalk, thresholds, motion, the INT-driven frame stream) and, underneath,
  the register-bridge streaming path: chunk parse (`depz_vl53l8_unpack_chunk`),
  frame reassembly (`depz_vl53l8_reassembler`), the ranging-frame decoder
  (`depz_vl53l8_decode_frame`), and the advanced DCI codecs.
- **[VL53L8CH](vl53l8ch/guide.md)** — the CX superset; the same class with the
  CH firmware, plus CNH setup (`depz_vl53l8_configure_cnh`) and the CNH
  histogram decode (`depz_vl53l8ch_decode_cnh`).
- **[VL53L5CX](vl53l5cx/guide.md) / [VL53L7CX](vl53l7cx/guide.md) /
  [VL53L7CH](vl53l7ch/guide.md)** — the same live class with the L5/L7 models
  and the board commands (`depz_vl53l7_bridge_info`, `_set_i2c_speed_khz`,
  `_pin_ctrl`) and, underneath, the I2C board codecs (`depz_vl53l7_pack_*`,
  `depz_vl53l7_unpack_info`), class resolution (`depz_vl53l7_resolve_model`)
  and the L5/L7 frame decode (`depz_vl53l7_decode_frame`, footer at
  `size − 4`, CNH block out on the L7CH).
- **[VL53L0X](vl53l0x/guide.md) / [VL53L1CX](vl53l1cx/guide.md) /
  [VL53L1CB](vl53l1cb/guide.md) / [VL53L3CX](vl53l3cx/guide.md) /
  [VL53L4CX](vl53l4cx/guide.md)** — the live 1D-family class
  (`depz_vl53lx_*`: init with a (product, driver) pair, configure, modes and
  budgets, calibrations, thresholds, ROI, streamed and single-shot
  measurements with targets and bins) and, underneath, the bridge codecs
  (`depz_vl53lx_pack_start_stream`, `_set_addr_width`, `_unpack_info`), the
  product table and class resolution, and the die / VL53L0X / histogram block
  decoders.
- **[BNO086](bno086/guide.md)** — the live sensor class (`depz_bno086_*`:
  reset and product id, enable with the granted-rate read-back, reports via
  callbacks / streams, calibration / DCD / tare / reorientation, FRS records
  and sensor metadata, error queue and counts) and, underneath, the bridge
  codecs (`depz_bno086_unpack_data`), SHTP framing (`depz_shtp_*`), SH-2
  encoders and answer parsers (`depz_bno_pack_*` / `depz_bno_unpack_*`), the
  input-report parsers and the scaling helpers (`depz_bno_report_*`).
- **[BNO055](bno055/guide.md)** — the live sensor class (`depz_bno055_*`:
  configure through CONFIG, operating / power mode, units, axis remap,
  calibration status and profile, status and self-test, page-1 configs and
  motion interrupts, scaled samples polled or streamed via callbacks /
  streams) and, underneath, the register-bridge codecs
  (`depz_bno055_pack_*` / `_unpack_*`), units, calibration status and profile,
  axis remap and placements, page-1 sensor configs, and the register-window
  decode (`depz_bno055_decode_block`).

## Firmware container & bootloader

`depz_fwdepz_parse()` parses and validates a `.fwdepz` application-firmware
container (magic → CRC-16/CCITT-FALSE header CRC → `fw_size` == payload), and the
bootloader opcode space (`depz_bl_cmd`) plus its page-write/read and flash-info
codecs are provided (contract 06). The live layer can reboot a board into the
bootloader (`depz_device_enter_bootloader()`) and reports bootloader mode from
discovery, but driving the actual flash flow (open the bootloader port, erase,
write pages, verify) is left to the transport you own.

## Datasets

`depz_dataset_parse()` reads a `.depzdata` recording (JSONL: a header plus
time-merged records, contract 09) into an in-memory model you walk with
`depz_dataset_get_device()` / `depz_dataset_get_record()` and read scalars from
with `depz_dataset_value_int` / `depz_dataset_value_str`. Records are exposed in
stable merge order (by `t_host_us`, then file order).

## Testing

The suite is 83 CTests, no hardware required:

- **Vectors (24)** — one CTest per consumed vector / replay target: the
  transport+protocol foundation plus the sensor-decode layer. A real VL53L8CX
  capture is replayed end-to-end through the shared decode path
  (`vec_vl53l8_replay`), and live L5CX / L7CH / L8CH captures through the L5/L7
  decode (`vec_vl53l7_replay_*`, including CNH frames).
- **Live layer (59, `io_*`)** — the device core and the SR04 class against a
  fake SR04 on the far end of a loopback link (identity, time sync, status
  errors, timeouts, the echo-decay clamp, single shot vs loop, `ERR_BUSY`
  during the loop, drop-oldest streams, disconnect, record → strict replay);
  the VL53L4CD class against a fake register-bridge firmware with a register
  map (`io_l4_*`: init at boot speed then re-timed, the config block and VHV,
  timing / tuning round trips and bounds, the model id, configuration refused
  while ranging, stream decode, single-shot polling); the VL53L8 state guards
  and CNH setup math (`io_l8_guards_and_cnh_math`); and five real captures
  replayed strictly — `io_sr04_session_replay`
  (`contracts/vectors/recordings/sr04_session.depzrec`),
  `io_l4_session_replay` (`vl53l4cd_session.depzrec`), and `io_l8cx_replay`,
  `io_l8ch_replay`, `io_l8ch_cnh_replay` (a VL53L8CX and a VL53L8CH at 8×8,
  15 Hz, the last with CNH — the whole firmware download included; the
  VL53L4CD and VL53L8 captures recorded by the Python SDK), plus the four
  L5/L7 captures through the same class (`io_l5cx_replay`,
  `io_l5cx_4x4_replay`, `io_l7ch_replay`, `io_l7ch_cnh_replay`: a VL53L5CX at
  8×8 and 4×4, a VL53L7CH without and with CNH, model resolved by
  `depz_device_promote()` from the device name), and the two BNO055 captures
  through the BNO055 class (`io_bno_ndof_replay`: reset, configure into NDOF,
  the calibration profile through CONFIG, a 100 Hz full-block stream;
  `io_bno_imu_replay`: IMU mode with every unit switched, a 50 Hz quaternion
  stream), and the BNO085 capture through the BNO085 / BNO086 class
  (`io_bno086_session_replay`: reset, product id, three enables with their
  read-backs, 150 mixed reports, a busy retry, calibration, metadata, counts)
  — so the C SDK must re-issue every request byte for byte and decode the
  same values. The BNO085 / BNO086 class also runs against a fake SH-2 hub
  (`io_bno086_*`: busy retries, stale read-backs, reports and scaling,
  commands, FRS, refusals). The ten VL53L 1D-family captures run through the
  1D class (`io_vlx_*_replay`: `init(driver, product)`, `configure()`, the
  timing read-back and every streamed frame — distance, status, targets,
  histogram bins — for the VL53L0X API, the VL53L1X ULD on a VL53L1CX and a
  VL53L1CB, the VL53L3CX ULP, the VL53L4CD ULD borrowed by a VL53L4CX, and
  the histogram driver on the VL53L1CX, VL53L1CB, VL53L3CX and VL53L4CX,
  the short-preset artefact frames included).

```sh
ctest --test-dir build            # 83/83
```

CI runs the suite on Linux, macOS and Windows, and the live layer again under
AddressSanitizer and ThreadSanitizer. The live layer was also checked by hand on
a real SR04 (firmware `APP_usonic_SR04_v0.97`), a real VL53L4CD
(`APP_VL53L4_v0.83`), a real VL53L8CH (`APP_VL53L8_v0.92`), a real
VL53L7CH (`APP_VL53L7_v0.53`), a real VL53L4CX (`APP_VL53L0_4_v0.24`), a real
BNO055 and a real BNO085 — see the
[SR04](sr04/guide.md#verified-on-hardware),
[VL53L4CD](vl53l4cd/guide.md#verified-on-hardware),
[VL53L8CX](vl53l8cx/guide.md#verified-on-hardware),
[VL53L7CX](vl53l7cx/guide.md#verified-on-hardware),
[VL53L4CX](vl53l4cx/guide.md#verification-status),
[BNO055](bno055/guide.md#verification-status) and
[BNO086](bno086/guide.md#verification-status) guides.

Regenerate this documentation's API reference after changing either header's
doc-comments:

```sh
make docs                         # == python3 scripts/gen_api_md.py
```
