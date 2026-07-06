# SR04 — user guide

Hands-on guide to the SR04 codecs. For what the sensor is and its concepts, read
the [introduction](introduction.md); for exact signatures see the
[API reference](api.md). For the transport and the parser loop these examples
build on, see the [common guide](../guide.md).

## Contents

- [Decode a measurement](#decode-a-measurement)
- [Echo time to distance](#echo-time-to-distance)
- [Configuration codecs](#configuration-codecs)
- [Single shot vs the loop](#single-shot-vs-the-loop)
- [Gotchas](#gotchas)

## Decode a measurement

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

## Echo time to distance

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

## Configuration codecs

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

## Single shot vs the loop

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

- **Always check the `depz_sr04_distance_mm` return.** A no-echo timeout sets
  `echo_time_us == DEPZ_SR04_ECHO_TIMEOUT` (`0xFFFF`) and the function returns
  `false`.
- **`MEASURE_ONCE` during the loop returns `ERR_BUSY`** — one measurement path at
  a time.
- **Configured period is a ceiling, not the realised rate** — the echo window
  throttles it; the read-back returns the stored value.
- **Echo decay is clamped to 4000–65000 µs** on the device; the read-back is the
  effective value. Values above 65535 can't be sent (the field is a `uint16_t`).
- **`source_cmd` distinguishes loop vs single-shot** — a SYNC_IN-triggered shot
  arrives as `0x36`, just like a host `MEASURE_ONCE`.
