# SR04 — user guide

Hands-on guide to the SR04: the live sensor class of `depz_sensor_io.h` first,
then the codecs underneath for when you own the transport. For what the sensor
is and its concepts, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md). Devices, errors, threading rules and
streams in general are in the [common guide](../guide.md#live-layer).

## Contents

- [Open the sensor](#open-the-sensor)
- [Configure](#configure)
- [Single shot](#single-shot)
- [The measurement loop](#the-measurement-loop)
- [Distance and the no-echo sentinel](#distance-and-the-no-echo-sentinel)
- [Disconnects](#disconnects)
- [Record and replay for tests](#record-and-replay-for-tests)
- [Verified on hardware](#verified-on-hardware)
- [Decode layer (your own transport)](#decode-layer-your-own-transport)
- [Gotchas](#gotchas)

## Open the sensor

`depz_open_device()` finds the board, probes it and attaches the SR04 class.
With no options it takes the DEPZ board with the smallest USB serial; set
`port`, `serial` or `index` to pick one. Check `depz_is_sr04()` — the same call
opens any DEPZ board, and an SR04 call on another type fails with
`DEPZ_E_WRONG_TYPE`.

```c
#include <depz_sensor_io.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    depz_open_options opt = DEPZ_OPEN_OPTIONS_INIT;
    depz_device *dev;
    char fw[64];

    if (argc > 1) opt.port = argv[1];          /* "/dev/ttyACM0", "COM7", ... */
    if (depz_open_device(&opt, &dev) != DEPZ_OK) {
        fprintf(stderr, "open: %s\n", depz_last_error());
        return 1;
    }
    if (!depz_is_sr04(dev)) {
        fprintf(stderr, "%s is not an SR04\n", depz_device_port(dev));
        depz_device_close(dev);
        return 1;
    }
    depz_device_get_software_name(dev, fw, sizeof fw);   /* "APP_usonic_SR04_v0.97" */
    printf("SR04 on %s, %s\n", depz_device_port(dev), fw);
    /* ... */
    depz_device_close(dev);
    return 0;
}
```

On a link of your own — a replay, a loopback fake — `depz_sr04_open_link(link,
&dev)` attaches the class without a probe (see
[record and replay](#record-and-replay-for-tests)). Either way the device owns
the link and `depz_device_close()` frees both.

## Configure

Two settings, both microseconds, both kept by the device:

```c
uint32_t period_us;
uint16_t decay_us;

depz_sr04_set_sample_period_us(dev, 20000);       /* 50 Hz ceiling */
depz_sr04_get_sample_period_us(dev, &period_us);  /* 20000: the stored value */

/* The device clamps echo decay to 4000..65000; the setter re-reads it. */
depz_sr04_set_echo_decay_us(dev, 100, &decay_us);     /* decay_us == 4000  */
depz_sr04_set_echo_decay_us(dev, 65535, &decay_us);   /* decay_us == 65000 */
depz_sr04_get_echo_decay_us(dev, &decay_us);
```

- The **sample period** is a ceiling (default 50000 µs, 20 Hz): the loop never
  starts measurements closer together, and each echo window can stretch it
  further. The read-back is the stored value, not the realised rate.
- **Echo decay** outside 4000–65000 µs is legal and clamped by the device;
  `effective` (optional, may be `NULL`) receives what is actually in effect.
  A value above 65535 cannot travel in the `u16` wire field and fails with
  `DEPZ_E_ARG` without being sent.

## Single shot

```c
depz_sr04_measurement m;
double mm;
int rc = depz_sr04_measure_once(dev, -1, &m);     /* -1: the 1000 ms default */

if (rc == DEPZ_OK) {
    if (depz_sr04_measurement_distance_mm(&m, &mm))
        printf("%.1f mm at device t=%llu us\n", mm, (unsigned long long)m.timestamp_us);
    else
        printf("no echo\n");
} else if (rc == DEPZ_E_BUSY) {
    printf("the loop is running: stop it first\n");
} else {
    fprintf(stderr, "%s: %s\n", depz_err_str(rc), depz_last_error());
}
```

The reply *is* the measurement, and it comes when the echo completes — or when
the device gives up on it after ~65.5 ms — so `measure_once` does not use the
200 ms request default: a negative timeout means **1000 ms**. While the loop
runs the device refuses a single shot with `ERR_BUSY`, reported as
`DEPZ_E_BUSY` (and `depz_last_status()` is `DEPZ_STATUS_ERR_BUSY`).

## The measurement loop

`depz_sr04_start()` starts the free-running loop and `depz_sr04_stop()` stops
it; both are idempotent. Samples come out two ways — use either or both:

**Streams** — pulled from your own thread, bounded, drop-oldest. This is the
usual choice:

```c
depz_sr04_measurement m;
double mm;
depz_stream *s = depz_sr04_stream(dev, 64);   /* before start: nothing is missed */
uint64_t end;

depz_sr04_start(dev);
end = depz_host_now_us() + 5000000u;          /* 5 s */
while (depz_host_now_us() < end && depz_stream_next(s, &m, 500) == DEPZ_OK) {
    if (depz_sr04_measurement_distance_mm(&m, &mm)) printf("%7.1f mm\n", mm);
    else printf("   no echo\n");
}
depz_sr04_stop(dev);
printf("%llu dropped\n", (unsigned long long)depz_stream_dropped_count(s));
depz_stream_close(s);
```

If the consumer falls behind, the oldest samples are discarded and
`depz_stream_dropped_count()` counts them — the device is never stalled. Each
stream is independent: two consumers get two streams, each with every sample.
`maxsize == 0` picks the default of 256.

**Callbacks** — run on the device's reader thread for every sample:

```c
static void on_sample(const depz_sr04_measurement *m, void *user)
{
    /* reader thread: short, never block, never depz_device_close() here */
    unsigned *count = (unsigned *)user;
    (*count)++;
    (void)m;
}

unsigned count = 0;
int token;
depz_sr04_on_measurement(dev, on_sample, &count, &token);
depz_sr04_start(dev);
/* ... */
depz_sr04_stop(dev);
depz_sr04_off_measurement(dev, token);
```

Both paths also see single shots triggered by a **SYNC_IN** edge (`from_loop ==
false`), which the device sends without being asked. The reply to a host
`depz_sr04_measure_once()` is different: that call claims it and returns it,
so it does not reach streams or callbacks.

## Distance and the no-echo sentinel

When nothing returns, the device reports `echo_time_us ==
DEPZ_SR04_ECHO_TIMEOUT` (`0xFFFF`). The helpers refuse to turn that into a
distance:

```c
double mm;

if (!depz_sr04_valid(&m))
    printf("no echo\n");
if (depz_sr04_measurement_distance_mm(&m, &mm))           /* c = 343 m/s */
    printf("%.1f mm\n", mm);
if (depz_sr04_measurement_distance_mm_at(&m, 30.0, &mm))  /* c = 331.3 + 0.606·30 */
    printf("%.1f mm at 30 C\n", mm);
```

`echo_time_us` stays the authoritative value; the distance is derived
(`echo_us · c / 2000`). To put `timestamp_us` on the host clock, run
`depz_device_sync_time()` once and map it with `depz_device_to_host_time_us()`.

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
if (depz_device_closed(dev)) fprintf(stderr, "SR04 lost\n");
depz_stream_close(s);
depz_device_close(dev);      /* still needed: frees the device */
```

There is no automatic reconnect: open the device again once it is back.

## Record and replay for tests

Tee a live session into a `.depzrec` file, then replay it with no board
attached. Strict replay (`strict_tx = true`) also checks that your code sends
exactly the bytes it sent when recording, so a changed request fails loudly
with `DEPZ_E_REPLAY_MISMATCH` instead of drifting.

```c
static void session(depz_device *dev, depz_sr04_measurement *m)
{
    depz_sr04_set_sample_period_us(dev, 20000);
    depz_sr04_measure_once(dev, -1, m);
}

int record_and_replay(const char *port)
{
    depz_link *serial, *rec, *rp;
    depz_device *dev;
    depz_sr04_measurement live, again;

    /* 1. Record against the real board. */
    if (depz_link_open_serial(port, &serial) != DEPZ_OK) return -1;
    if (depz_link_open_recording(serial, "sr04.depzrec", "\"note\":\"bench\"", &rec) != DEPZ_OK)
        return -1;                                 /* serial already freed */
    if (depz_sr04_open_link(rec, &dev) != DEPZ_OK) return -1;
    session(dev, &live);
    depz_device_close(dev);

    /* 2. Replay: same calls, same order, recorded answers. */
    if (depz_link_open_replay("sr04.depzrec", true, false, &rp) != DEPZ_OK) return -1;
    if (depz_sr04_open_link(rp, &dev) != DEPZ_OK) return -1;
    session(dev, &again);                          /* again == live */
    depz_device_close(dev);
    return 0;
}
```

`depz_sr04_open_link()` skips the identity probe, so the recording holds only
the calls you make. To record the probe too (and then
`depz_device_promote()` in replay), open with `depz_device_open_link()` instead
— that is how the test suite replays the lab SR04's capture
(`contracts/vectors/recordings/sr04_session.depzrec`, `io_sr04_session_replay`).
Pass `realtime = true` to replay at the recorded pace instead of as fast as
possible.

## Verified on hardware

The class was run against a real SR04 with firmware `APP_usonic_SR04_v0.97`:

- discovery and the common commands (names, serial, MCU temperature, time sync);
- the echo-decay clamp: 100 → 4000 and 65535 → 65000;
- single shots, and `DEPZ_E_BUSY` for a single shot while the loop runs;
- the loop at 50 samples/s with none dropped.

The CTest suite covers the same behaviour without hardware (a fake SR04 over a
loopback link, and the strict replay of a real capture).

## Decode layer (your own transport)

If you run your own serial code, the SR04 codecs of `depz_sensor_sdk.h` work
without the live layer — the class above is built from them. For the parser
loop these examples build on, see the common guide's
[mental model](../guide.md#mental-model).

### Decode a measurement

An SR04 measurement arrives as a `DEPZ_SR04_RPT_DATA` (`0x91`) packet. In your
parser callback, match on the report id and unpack the payload:

```c
#include <depz_sensor_sdk.h>

static void on_event(const depz_event *ev, void *user) {
    if (ev->type != DEPZ_EV_PACKET) return;
    if (ev->cmd == DEPZ_SR04_RPT_DATA) {
        depz_sr04_data m;
        if (depz_sr04_unpack_data(ev->payload, ev->payload_len, &m) == 0) {
            double mm;
            bool ok = depz_sr04_distance_mm(m.echo_time_us, 0.0, false, &mm);
            if (ok) printf("%8.1f mm  (src 0x%02X)\n", mm, m.source_cmd);
            else    printf("no echo\n");
        }
    }
}
```

`depz_sr04_data` carries the device `timestamp_us`, the raw `echo_time_us`, and
`source_cmd` (`0x36` single shot / `0x37` loop). Feed bytes from your serial
read into `depz_parser_feed(&p, buf, n, on_event, ctx)` — see the
[mental model](../guide.md#mental-model).

### Echo time to distance

`depz_sr04_distance_mm()` is the one conversion you need. It returns `false` for
the no-echo timeout sentinel, and lets you pass an air temperature for a
compensated speed of sound:

```c
double mm;

// default: 343 m/s (have_temp = false)
if (depz_sr04_distance_mm(m.echo_time_us, 0.0, false, &mm))
    printf("%.1f mm\n", mm);

// temperature-compensated: c = 331.3 + 0.606·T
if (depz_sr04_distance_mm(m.echo_time_us, 30.0, true, &mm))
    printf("%.1f mm at 30 C\n", mm);   // c ≈ 349.5 m/s
```

The raw `echo_time_us` is authoritative; the millimetre distance is derived
(`echo_us · c / 2000`).

### Configuration codecs

Each config value is a get/set pair. The setters are commands whose payload you
encode; the read-backs are reports you decode. Values are microseconds.

```c
uint8_t frame[DEPZ_MAX_FRAME], payload[8];
size_t plen, flen;

// set the sample period to 20 ms (50 Hz ceiling)
plen = depz_sr04_pack_sample_period(20000, payload);
depz_build_packet(DEPZ_SR04_SET_SAMPLE_PERIOD, payload, plen,
                  seq++, DEPZ_CRC_NONE, frame, sizeof frame, &flen);
serial_write(fd, frame, flen);

// set the echo decay (clamped to 4000–65000 µs on the device)
plen = depz_sr04_pack_echo_decay(5000, payload);
depz_build_packet(DEPZ_SR04_SET_ECHO_DECAY, payload, plen,
                  seq++, DEPZ_CRC_NONE, frame, sizeof frame, &flen);
serial_write(fd, frame, flen);
```

Read-backs come as `DEPZ_SR04_RPT_SAMPLE_PERIOD` (`0x92`) /
`DEPZ_SR04_RPT_ECHO_DECAY` (`0x93`); decode them in your callback:

```c
if (ev->cmd == DEPZ_SR04_RPT_SAMPLE_PERIOD) {
    uint32_t period_us;
    depz_sr04_unpack_sample_period(ev->payload, ev->payload_len, &period_us);
}
if (ev->cmd == DEPZ_SR04_RPT_ECHO_DECAY) {
    uint16_t decay_us;                 // the value actually in effect (clamped)
    depz_sr04_unpack_echo_decay(ev->payload, ev->payload_len, &decay_us);
}
```

The device throttles the **effective** rate by the echo window, so the read-back
period is the stored ceiling, not the realised rate.

### Single shot vs the loop

There are two ways the device produces measurements; both arrive as
`DEPZ_SR04_RPT_DATA` and are told apart by `source_cmd`:

```c
// one-shot: the reply arrives when the echo completes (or times out ~65.5 ms)
depz_build_packet(DEPZ_SR04_MEASURE_ONCE, NULL, 0, seq++, DEPZ_CRC_NONE,
                  frame, sizeof frame, &flen);

// free-running loop: samples stream at the configured period
depz_build_packet(DEPZ_SR04_START_MEASUREMENT_LOOP, NULL, 0, seq++,
                  DEPZ_CRC_NONE, frame, sizeof frame, &flen);
// ... later ...
depz_build_packet(DEPZ_SR04_STOP_MEASUREMENT_LOOP, NULL, 0, seq++,
                  DEPZ_CRC_NONE, frame, sizeof frame, &flen);
```

A `MEASURE_ONCE` while the loop is running is rejected by the device with an
`ERR_BUSY` status (`DEPZ_RPT_STATUS` echoing `0x36`, status
`DEPZ_STATUS_ERR_BUSY`) — decode it with `depz_unpack_status`.

## Gotchas

- **Always check the distance helpers' return.** A no-echo timeout sets
  `echo_time_us == DEPZ_SR04_ECHO_TIMEOUT` (`0xFFFF`) and
  `depz_sr04_measurement_distance_mm` / `depz_sr04_distance_mm` return `false`.
- **`measure_once` during the loop fails with `DEPZ_E_BUSY`** (`ERR_BUSY` on the
  wire) — one measurement path at a time. Stop the loop first.
- **`measure_once` waits up to 1000 ms by default**, not the 200 ms request
  default — the reply is the echo itself.
- **Callbacks run on the reader thread** — keep them short, don't block, don't
  make requests or close the device from one. Prefer a stream for real work.
- **A full stream drops the oldest sample**, it never blocks the device; watch
  `depz_stream_dropped_count()` if every sample matters.
- **Configured period is a ceiling, not the realised rate** — the echo window
  throttles it; the read-back returns the stored value.
- **Echo decay is clamped to 4000–65000 µs** on the device; the setter's
  `effective` output is the value in effect. Values above 65535 can't be sent
  (the field is a `uint16_t`): `DEPZ_E_ARG`.
- **Loop vs single shot** — `from_loop` (class) / `source_cmd` (codec) tell them
  apart; a SYNC_IN-triggered shot arrives as a single shot (`from_loop ==
  false`, `source_cmd == 0x36`), just like a host `MEASURE_ONCE`.
