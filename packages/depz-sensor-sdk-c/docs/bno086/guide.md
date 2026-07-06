# BNO086 — user guide

Hands-on guide to the BNO086 codecs — SHTP framing, SH-2 control encoders, and
the input-report parsers. For what the sensor is and its concepts, read the
[introduction](introduction.md); for exact signatures see the
[API reference](api.md).

## Contents

- [The three layers](#the-three-layers)
- [Enable a report](#enable-a-report)
- [Reassemble SHTP and parse reports](#reassemble-shtp-and-parse-reports)
- [Report fields and units](#report-fields-and-units)
- [The gyro-integrated RV channel](#the-gyro-integrated-rv-channel)
- [Gotchas](#gotchas)

## The three layers

The BNO086 speaks SH-2 over SHTP, tunnelled through the DEPZ transport. Stacked
from the wire up:

```
DEPZ transport (A5 C3 framing)     <- depz_build_packet / depz_parser_feed
        │  payload = SHTP frame(s), inbound carries a bridge capture timestamp
        ▼
SHTP framing (6 channels, cargos)  <- depz_shtp_next_frame / depz_shtp_feed
        │  cargo payload = SH-2 message
        ▼
SH-2 (features, reports, commands) <- depz_bno_pack_* / depz_bno_parse_input_cargo
```

The SDK owns the SHTP and SH-2 layers; you own the DEPZ transport around them
(the same `depz_parser` / `depz_build_packet` every sensor uses). Extracting the
SHTP bytes and the capture timestamp from the inbound bridge packet is your
transport glue — pass that capture time into the report parser.

## Enable a report

Encode a Set Feature message (SH-2 §6), wrap it in an SHTP control-channel frame,
then wrap that in a DEPZ packet and send it:

```c
#include <depz_sensor_sdk.h>

depz_shtp_layer shtp;
depz_shtp_init(&shtp);

// SH-2: enable ROTATION_VECTOR (id 5) at 100 Hz (interval 10 000 µs)
uint8_t sh2[17];
size_t sh2n = depz_bno_pack_set_feature(DEPZ_BNO_ROTATION_VECTOR, /*flags*/0,
                                        /*sensitivity*/0, /*interval_us*/10000,
                                        /*batch_us*/0, /*cfg_word*/0, sh2);

// SHTP: single-fragment frame on the control channel (consumes its TX seq)
uint8_t shtp_frame[DEPZ_SHTP_MAX_TX_FRAME];
size_t fn = depz_shtp_next_frame(&shtp, DEPZ_SHTP_CH_CONTROL, sh2, sh2n,
                                 shtp_frame, sizeof shtp_frame);

// DEPZ: wrap and send (opcode = the bridge's SHTP-write command)
uint8_t frame[DEPZ_MAX_FRAME]; size_t flen;
depz_build_packet(/*cmd*/0x33, shtp_frame, fn, seq++, DEPZ_CRC_NONE,
                  frame, sizeof frame, &flen);
serial_write(fd, frame, flen);
```

Note `DEPZ_BNO_ROTATION_VECTOR` here is the report **catalog** index used by the
parsers; the SH-2 *sensor id* you pass to `depz_bno_pack_set_feature` is the SH-2
number (ROTATION_VECTOR = 5). The other encoders follow the same shape:
`depz_bno_pack_get_feature_request`, `depz_bno_pack_product_id_request`,
`depz_bno_pack_command_request`, and the FRS read/write encoders.

## Reassemble SHTP and parse reports

Inbound, feed each SHTP frame (carried in a bridge packet) into the shared
`depz_shtp_layer`. When a channel-3/4 cargo completes, parse it into typed
reports; the capture timestamp comes from the bridge packet that carried it:

```c
static void on_event(const depz_event *ev, void *user) {
    if (ev->type != DEPZ_EV_PACKET) return;

    // ev->payload is the SHTP frame; capture_ts is decoded from the envelope
    uint64_t capture_ts = /* from the bridge RPT_DATA envelope */ 0;

    depz_shtp_cargo cargo;
    if (depz_shtp_feed(&shtp, ev->payload, ev->payload_len, &cargo) != 1)
        return;

    if (cargo.channel == DEPZ_SHTP_CH_INPUT_NORMAL ||
        cargo.channel == DEPZ_SHTP_CH_INPUT_WAKE) {
        depz_bno_report reports[16];
        size_t n = depz_bno_parse_input_cargo(cargo.payload, cargo.payload_len,
                                              capture_ts, reports, 16);
        for (size_t i = 0; i < n; i++) {
            const depz_bno_report *r = &reports[i];
            if (r->type == DEPZ_BNO_ROTATION_VECTOR)
                printf("quat raw i=%d j=%d k=%d w=%d  @%lld us\n",
                       r->i_raw, r->j_raw, r->k_raw, r->real_raw,
                       (long long)r->timestamp_us);
        }
    } else if (cargo.channel == DEPZ_SHTP_CH_GYRO_RV) {
        depz_bno_report r;
        if (depz_bno_parse_gyro_rv(cargo.payload, cargo.payload_len,
                                   capture_ts, &r) == 0)
            printf("gyro-RV vx=%d vy=%d vz=%d\n", r.vx_raw, r.vy_raw, r.vz_raw);
    }
}
```

`depz_bno_parse_input_cargo()` handles the 0xFB base-timestamp and 0xFA rebase,
folds the per-report delay, and returns the number of reports written (an unknown
report id stops the parse with a trailing `DEPZ_BNO_UNKNOWN_REPORT`).

## Report fields and units

Every `depz_bno_report` keeps the **wire integers**; apply the SH-2 Q-point
yourself (`value = raw / 2**Q`). `type` (a `depz_bno_report_type`) says which
fields are meaningful:

| type | fields | scaling |
|---|---|---|
| `DEPZ_BNO_ACCELERATION` | `x_raw/y_raw/z_raw` | m/s², Q8 |
| `DEPZ_BNO_GYROSCOPE` | `x_raw/y_raw/z_raw` | rad/s, Q9 |
| `DEPZ_BNO_MAGNETOMETER` | `x_raw/y_raw/z_raw` | µT, Q4 |
| `DEPZ_BNO_ROTATION_VECTOR` | `i_raw/j_raw/k_raw/real_raw` (+ `accuracy_raw`) | quat Q14; accuracy Q12 (radians) |
| `DEPZ_BNO_GYRO_INTEGRATED_RV` | quat + `vx_raw/vy_raw/vz_raw` | quat Q14; angular velocity Q10 (rad/s) |
| `DEPZ_BNO_STEP_COUNTER` | `steps`, `latency_us` | — |
| `DEPZ_BNO_UNCAL_GYROSCOPE` / `_MAGNETOMETER` | axes + `bias_*_raw` | as the calibrated variant |

Each channel-3/4 report also carries `sensor_id`, `seq` (rolling sample counter
for drop detection), `accuracy` (0 unreliable … 3 high), `delay_us`, and the
folded absolute `timestamp_us`. `depz_bno_report_type_str()` maps the type to its
catalog name.

## The gyro-integrated RV channel

The gyro-integrated rotation vector streams on its own channel (channel 5,
`DEPZ_SHTP_CH_GYRO_RV`) as a dense record with no channel-3/4 header — parse it
with `depz_bno_parse_gyro_rv()` rather than `depz_bno_parse_input_cargo()`, as in
the example above. It carries the quaternion (`i_raw`…`real_raw`, Q14) plus the
angular velocity (`vx_raw/vy_raw/vz_raw`, Q10 rad/s).

## Gotchas

- **Enable before you parse** — no input report arrives until you send a Set
  Feature for it.
- **Everything is raw integers** — apply the Q-point per field; rotation-vector
  `accuracy_raw` is Q12 **radians**, a different thing from the integer
  `accuracy` reliability level (0–3).
- **One TX slot is 64 bytes** (`DEPZ_SHTP_MAX_TX_FRAME`, ERRATA E2) —
  `depz_shtp_next_frame` builds single-fragment frames only and returns `0` if
  the payload won't fit.
- **Correlation is at the SH-2 layer** — the bridge returns everything as
  unsolicited SHTP; reassemble with `depz_shtp_feed` and match responses
  yourself. That's why you think in reports, not request/reply.
- **Gyro-integrated RV uses a different parser** — channel 5 has no standard
  header; use `depz_bno_parse_gyro_rv`, not the input-cargo parser.
- **The bridge SHTP-write opcode and the capture-timestamp envelope are your
  transport glue** — this SDK provides the SHTP and SH-2 layers, not the bridge
  envelope codec.
