# VL53L7CX — user guide

Hands-on guide to the L5/L7 boards on the live multizone class
(`depz_vl53l8_*`), then the board codecs underneath for when you own the
transport. For what the sensor is, read the [introduction](introduction.md);
for exact signatures see the [API reference](api.md) and, for the class, the
[VL53L8CX reference](../vl53l8cx/api.md). The same page serves the
[VL53L5CX](../vl53l5cx/guide.md) and the [VL53L7CH](../vl53l7ch/guide.md),
which run the same board firmware.

**The class is the VL53L8 one.** Initialising, configuring, streaming, the
frame, thresholds, motion, crosstalk and record / replay work exactly as in
the [VL53L8CX user guide](../vl53l8cx/guide.md) — read that for the details.
This page covers what is L5/L7-specific.

## Contents

- [Open the sensor](#open-the-sensor)
- [Initialise and read the module type](#initialise-and-read-the-module-type)
- [What differs from the VL53L8](#what-differs-from-the-vl53l8)
- [Board commands](#board-commands)
- [Record and replay for tests](#record-and-replay-for-tests)
- [Verified on hardware](#verified-on-hardware)
- [Decode layer (your own transport)](#decode-layer-your-own-transport)
- [Gotchas](#gotchas)

## Open the sensor

`depz_open_device()` attaches the multizone class to an `APP_VL53L7` board
and resolves which of the three sensors it carries. All three report the same
firmware name, so the rule is the one every DEPZ SDK uses: the production USB
PID first (`0xED48` L5CX, `0xED49` L7CX, `0xED4A` L7CH), then the first
`VL53L5CX` / `VL53L7CX` / `VL53L7CH` in the device name, else VL53L7CX.

```c
#include <depz_sensor_io.h>
#include <stdio.h>

static const char *model_name(depz_vl53l8_model m)
{
    switch (m) {
    case DEPZ_VL53L8_MODEL_L5CX: return "VL53L5CX";
    case DEPZ_VL53L8_MODEL_L7CX: return "VL53L7CX";
    case DEPZ_VL53L8_MODEL_L7CH: return "VL53L7CH";
    default: return "VL53L8";
    }
}

int main(int argc, char **argv)
{
    static depz_vl53l8_live_frame f;               /* ~7.5 KB: not on the stack */
    depz_open_options opt = DEPZ_OPEN_OPTIONS_INIT;
    depz_device *dev;

    if (argc > 1) opt.port = argv[1];
    if (depz_open_device(&opt, &dev) != DEPZ_OK) {
        fprintf(stderr, "open: %s\n", depz_last_error());
        return 1;
    }
    if (depz_device_sensor_type(dev) != DEPZ_SENSOR_VL53L7) {  /* "vl53l7" */
        fprintf(stderr, "%s is not an L5/L7 board\n", depz_device_port(dev));
        depz_device_close(dev);
        return 1;
    }
    printf("%s\n", model_name(depz_vl53l8_get_model(dev)));
    if (depz_vl53l8_init(dev, NULL, NULL) != DEPZ_OK ||              /* ~1.3 s */
        depz_vl53l8_set_resolution(dev, DEPZ_VL53L8_RES_8X8) != DEPZ_OK ||
        depz_vl53l8_set_ranging_frequency_hz(dev, 15) != DEPZ_OK ||
        depz_vl53l8_start_ranging(dev) != DEPZ_OK) {
        fprintf(stderr, "%s\n", depz_last_error());
        depz_device_close(dev);
        return 1;
    }
    printf("module type %d\n", depz_vl53l8_module_type(dev));      /* 0 L5, 1 L7 */
    if (depz_vl53l8_get_frame(dev, -1, &f) == DEPZ_OK)
        printf("zone 27: %d mm\n", f.f.distance_mm[27]);
    depz_vl53l8_stop_ranging(dev);
    depz_device_close(dev);
    return 0;
}
```

The device reports sensor type `vl53l7` (`DEPZ_SENSOR_VL53L7`), while
`depz_is_vl53l8()` is true — it is the VL53L8 class. `depz_device_promote()`
on a bare link (a replay, say) reads the device name to resolve the model, as
the Python SDK does, so captures recorded with a probe replay through it. To
choose the model yourself, `depz_vl53l8_open_link(link,
DEPZ_VL53L8_MODEL_L7CX, &dev)` attaches the class without a probe.

## Initialise and read the module type

`depz_vl53l8_init()` runs the L5/L7 branch of the ULD: it checks the silicon
(device id / revision — an unknown part fails with `DEPZ_E_PROTOCOL`),
downloads the sensor firmware of the model (the L5/L7 ULD 2.0.1 blob for
L5CX / L7CX, which publishes no checksum; the VL53LMZ blob with the VL53L7
default configuration for L7CH), and uploads the offset, crosstalk and
default configuration. It takes about **1.3 s** over the I2C bus (1.29 s on
the verified board). The progress callback works as on the VL53L8.

Afterwards `depz_vl53l8_module_type(dev)` returns what the sensor reports:
**0 = MZ** (a VL53L5CX), **1 = MZEVO** (a VL53L7CX or VL53L7CH), −1 before
`init()`. L5 and L7 share a board and a sensor firmware, so this is how the
silicon tells them apart — it never distinguishes CX from CH. Use it as a
check (a board stamped L7 that carries an L5 still ranges), not as the model
choice.

## What differs from the VL53L8

Everything else in the [VL53L8CX guide](../vl53l8cx/guide.md) holds; these
are the L5/L7 differences:

| | VL53L8CX / CH | VL53L5CX / L7CX | VL53L7CH |
|---|---|---|---|
| bus, firmware download | SPI | I2C, ~1.3 s | I2C, ~1.3 s |
| lowest ranging frequency | 2 Hz | **1 Hz** | **1 Hz** |
| deep sleep | yes | **no** (`DEPZ_E_ARG`) | yes |
| threshold auto-stop | yes | **no** (`DEPZ_E_ARG`) | yes |
| CNH | CH only | no (`DEPZ_E_WRONG_TYPE`) | yes, as the VL53L8CH |
| sensor type | `vl53l8` | `vl53l7` | `vl53l7` |
| `depz_vl53l8_module_type()` | −1 | 0 (L5) / 1 (L7) | 1 |
| board commands | — | `depz_vl53l7_*` | `depz_vl53l7_*` |

The upper frequency limits are the same (60 Hz at 4×4, 15 Hz at 8×8).
`depz_vl53l8_get_power_mode()` on an L5CX / L7CX reports sleep or wake-up
only. Register reads are split at 1536 bytes (`DEPZ_VL53L7_READ_MAX_LEN`)
instead of 2048 — the class does it, including in `depz_vl53l8_read_reg()`.
Crosstalk calibration writes the calibration table of the model's own
firmware, as on the VL53L8.

## Board commands

The I2C bridge has three commands of its own. They fail with
`DEPZ_E_WRONG_TYPE` on a VL53L8.

```c
depz_vl53l7_info info;
uint16_t khz;

if (depz_vl53l7_bridge_info(dev, &info) == DEPZ_OK)   /* before / after a run */
    printf("bus %u kHz, %u I2C errors (last %u), %u frames dropped, %s\n",
           (unsigned)info.i2c_khz, (unsigned)info.i2c_errors,
           (unsigned)info.last_i2c_error, (unsigned)info.frames_dropped,
           info.streaming ? "streaming" : "idle");

depz_vl53l7_set_i2c_speed_khz(dev, 450, &khz);         /* not while ranging */
printf("bus now %u kHz\n", (unsigned)khz);             /* 500: snapped */

depz_vl53l7_pin_ctrl(dev, DEPZ_VL53L7_PIN_SOFT_CYCLE); /* the sensor loses its firmware */
depz_vl53l8_init(dev, NULL, NULL);                     /* so init() again */
```

- **`depz_vl53l7_bridge_info()`** — the bridge's state; the sensor is never
  probed. The counters run from power-up or `DEVICE_RESET`; `SOFT_CYCLE`
  clears the I2C ones. Each call takes the bus from the stream and can cost a
  frame (counted in `frames_dropped`): read it before and after a run, not in
  a polling loop.
- **`depz_vl53l7_set_i2c_speed_khz(dev, khz, &effective)`** — the sensor bus
  speed. The firmware snaps to 100, 200, 400, 500 ... 1000 kHz;
  `effective` (optional) is the value now in effect, read back from the
  bridge. Mid-transfer the board answers `ERR_BUSY` (`DEPZ_E_BUSY`): stop
  ranging first.
- **`depz_vl53l7_pin_ctrl(dev, action)`** — the sensor's pins:

| action | effect |
|---|---|
| `DEPZ_VL53L7_PIN_LPN_OFF` | stop streaming, LPn low: sensor I2C interface off (state lost) |
| `DEPZ_VL53L7_PIN_LPN_ON` | LPn high: interface on (the power-up default) |
| `DEPZ_VL53L7_PIN_I2C_RST` | pulse the sensor's I2C_RST |
| `DEPZ_VL53L7_PIN_SOFT_CYCLE` | stop streaming, LPn low 1 ms, high, I2C_RST pulse (state lost; clears the I2C counters) |

After `LPN_OFF` or `SOFT_CYCLE` the class forgets its state —
`depz_vl53l8_initialized()` and `depz_vl53l8_ranging()` go `false` — and the
sensor needs `init()` again, the firmware download included (after `LPN_OFF`,
switch the interface back on with `LPN_ON` first).

## Record and replay for tests

Recording and strict replay work as in the
[VL53L8CX guide](../vl53l8cx/guide.md#record-and-replay-for-tests). Replay
with the model you recorded — `depz_vl53l8_open_link(link, model, &dev)`,
or `depz_device_open_link()` + `depz_device_promote()` when the capture
starts with the identity probe (promote then reads the device name, which the
capture answers). The test suite replays the four committed L5/L7 captures
this way, strictly, all recorded by the Python SDK on `APP_VL53L7_v0.53`:

- `io_l5cx_replay` — `vl53l5cx_8x8_15hz_3s.depzrec`: model resolved to L5CX,
  `init()` with the firmware download, module type 0, 8×8 at 15 Hz, 45 frames;
- `io_l5cx_4x4_replay` — `vl53l5cx_4x4_15hz.depzrec`: the same at 4×4, 20
  frames;
- `io_l7ch_replay` and `io_l7ch_cnh_replay` — the VL53L7CH captures, see the
  [CH guide](../vl53l7ch/guide.md#record-and-replay-for-tests).

The same captures also run through the decode layer alone
(`vec_vl53l7_replay_*`).

## Verified on hardware

The class was run against a real VL53L7CH (`TXK5KAX6X4`, firmware
`APP_VL53L7_v0.53`) facing a wall at about 0.6 m:

- the model resolved to L7CH, module type 1 (MZEVO);
- `init()` in 1.29 s, the firmware download included;
- 8×8 at 15 Hz, distances ~602 mm; 4×4 at 1 Hz;
- CNH armed: a 1708-byte block in every frame;
- the bus speed: 450 kHz asked, 500 kHz in effect;
- a soft cycle, then `init()` again;
- no I2C errors in the bridge counters.

A VL53L5CX or VL53L7CX board has not been run live with this class; they use
the same code with the L5/L7 firmware blob, and the two real VL53L5CX captures
replay through it byte for byte.

## Decode layer (your own transport)

If you run your own serial code, the L5/L7 codecs of `depz_sensor_sdk.h` work
without the live layer — the class above is built from them. You then drive
the ULD sequences yourself; the snippets below build command payloads and
decode reply / report payloads (see the common guide's
[mental model](../guide.md#mental-model)).

### The codec surface

| direction | id | what | codec |
|---|---|---|---|
| host → device | `0x32` | read registers | `depz_vl53l7_pack_read_reg` (len 1..1536) |
| host → device | `0x33` | write registers | `depz_vl53l7_pack_write_reg` (len 1..2048) |
| host → device | `0x34` | pin control | `depz_vl53l7_pack_pin_ctrl` |
| host → device | `0x35` / `0x36` | start / stop streaming | `depz_vl53l8_pack_start_stream` / empty |
| host → device | `0x37` | bridge info | empty payload |
| host → device | `0x38` | sensor bus speed | `depz_vl53l7_pack_set_i2c_speed` |
| device → host | `0x91` | register data | `depz_vl53l8_unpack_reg_data` |
| device → host | `0x92` | bridge info | `depz_vl53l7_unpack_info` |
| device → host | `0x93` | frame chunk | `depz_vl53l8_unpack_chunk` → reassembler → `depz_vl53l7_decode_frame` |

The opcodes the board shares with the VL53L8 keep their `DEPZ_VL53L8_CMD_*` /
`DEPZ_VL53L8_RPT_*` names; the three new ones are `DEPZ_VL53L7_CMD_PIN_CTRL`,
`DEPZ_VL53L7_CMD_GET_INFO` and `DEPZ_VL53L7_CMD_SET_I2C_SPEED`.

### Send a command

The snippets below call one small helper that frames a payload with
`depz_build_packet()` and hands it to your transport:

```c
#include <depz_sensor_sdk.h>

/* Your transport: write bytes to the board's serial port. */
void port_write(const uint8_t *bytes, size_t len);

static unsigned seq;

void send_packet(uint8_t cmd, const uint8_t *payload, size_t len)
{
    /* large enough for the biggest L5/L7 payload: WRITE_REG 2 + 2048 B */
    static uint8_t frame[DEPZ_HEADER_SIZE + 2 + DEPZ_VL53L7_WRITE_MAX_LEN + 4];
    size_t n;
    if (depz_build_packet(cmd, payload, len, seq++, DEPZ_CRC8,
                          frame, sizeof frame, &n) == 0)
        port_write(frame, n);
}
```

### Board command payloads

```c
uint8_t p[2 + DEPZ_VL53L7_WRITE_MAX_LEN];
size_t n;

/* a 3000-byte read, split at the L5/L7 ceiling (1536, not the VL53L8's 2048) */
for (uint32_t off = 0; off < 3000; off += DEPZ_VL53L7_READ_MAX_LEN) {
    uint32_t left = 3000 - off;
    uint16_t len = (uint16_t)(left < DEPZ_VL53L7_READ_MAX_LEN
                              ? left : DEPZ_VL53L7_READ_MAX_LEN);
    n = depz_vl53l7_pack_read_reg((uint16_t)(0x2C00 + off), len, p);
    send_packet(DEPZ_VL53L8_CMD_READ_REG, p, n);
}

/* pulse LPn low 1 ms, high, then I2C_RST — the sensor loses its firmware */
n = depz_vl53l7_pack_pin_ctrl(DEPZ_VL53L7_PIN_SOFT_CYCLE, p);
send_packet(DEPZ_VL53L7_CMD_PIN_CTRL, p, n);

/* re-time the sensor bus; the firmware snaps to 100, 200, 400, 500 ... 1000 kHz */
n = depz_vl53l7_pack_set_i2c_speed(400, p);
send_packet(DEPZ_VL53L7_CMD_SET_I2C_SPEED, p, n);

/* bridge counters (answered by RPT_VL53_INFO, 0x92) */
send_packet(DEPZ_VL53L7_CMD_GET_INFO, NULL, 0);

/* stream frames of the size the sensor reports (8x8 = 1444 B, 4x4 = 1060 B) */
n = depz_vl53l8_pack_start_stream(1444, p);
send_packet(DEPZ_VL53L8_CMD_START_STREAM, p, n);
send_packet(DEPZ_VL53L8_CMD_STOP_STREAM, NULL, 0);
```

The read / write encoders refuse what the firmware would refuse: they return
`0` for a length outside `1..DEPZ_VL53L7_READ_MAX_LEN` (read) or
`1..DEPZ_VL53L7_WRITE_MAX_LEN` (write), or when `addr + len` passes `0x10000`.

| pin action | effect |
|---|---|
| `DEPZ_VL53L7_PIN_LPN_OFF` | stop streaming, LPn low: sensor I2C interface off (state lost) |
| `DEPZ_VL53L7_PIN_LPN_ON` | LPn high: interface on (the power-up default) |
| `DEPZ_VL53L7_PIN_I2C_RST` | pulse the sensor's I2C_RST |
| `DEPZ_VL53L7_PIN_SOFT_CYCLE` | stop streaming, LPn low 1 ms, high, I2C_RST pulse (state lost; clears the I2C counters) |

### Decode the replies

```c
#include <stdio.h>

void on_reply(const depz_event *ev)
{
    if (ev->type != DEPZ_EV_PACKET)
        return;
    if (ev->cmd == DEPZ_VL53L8_RPT_REG_DATA) {          /* 0x91: READ_REG answer */
        depz_vl53l8_reg_data r;
        if (depz_vl53l8_unpack_reg_data(ev->payload, ev->payload_len, &r) == 0)
            printf("%zu register bytes\n", r.data_len);   /* r.data -> the bytes */
    } else if (ev->cmd == DEPZ_VL53L7_RPT_INFO) {        /* 0x92: no echoed cmd */
        depz_vl53l7_info info;
        if (depz_vl53l7_unpack_info(ev->payload, ev->payload_len, &info) == 0)
            printf("bus %u kHz, %u I2C errors (last %u), %u frames dropped, %s\n",
                   (unsigned)info.i2c_khz, (unsigned)info.i2c_errors,
                   (unsigned)info.last_i2c_error, (unsigned)info.frames_dropped,
                   info.streaming ? "streaming" : "idle");
    }
}
```

`depz_vl53l7_info` is bridge state only — the sensor is never probed. The
counters run from power-up or `DEVICE_RESET`; `SOFT_CYCLE` clears the I2C
ones. `last_i2c_error` is one of `DEPZ_VL53L7_I2C_OK` / `_NACK` /
`_TIMEOUT` (250 ms deadline) / `_BUS_ERROR`; `i2c_khz` is the speed actually
in effect after snapping.

### Which board is this

All three L5/L7 boards report the same firmware name, so the firmware cannot
say which sensor is fitted. `depz_vl53l7_resolve_model()` applies the rule the
other SDKs use: the production USB PID first, then the first
`VL53L5CX` / `VL53L7CX` / `VL53L7CH` in the device name, else VL53L7CX (its
sensor firmware runs on every L5/L7 part).

```c
#include <stdio.h>

/* vid/pid from your port enumeration; device_name from GET_DEVICE_NAME */
void print_board(int vid, int pid, const char *device_name)
{
    const char *usb_model = depz_usb_model_hint(vid, pid);   /* may be NULL */
    depz_vl53l7_model m = depz_vl53l7_resolve_model(usb_model, device_name);
    printf("%s\n", depz_vl53l7_model_str(m));  /* "vl53l5cx" | "vl53l7cx" | "vl53l7ch" */
}
```

A device name such as `DEPZ ToF Sensor VL53L7CH USB v2.1 …` resolves to
`DEPZ_VL53L7_MODEL_L7CH`; a VL53L5CH match (no such board) falls back to
VL53L7CX. What the silicon itself says — L5 or L7 (`module_type`) — is only
readable after the sensor firmware is loaded, and is a diagnostic, never the
class choice.

### Reassemble and decode frames

Frames arrive as `RPT_VL53_FRAME` (`0x93`) chunks of up to
`DEPZ_VL53L7_STREAM_CHUNK_MAX` (1536) bytes. Chunk parse and reassembly are
the VL53L8 ones; only the decoder differs:

```c
#include <stdio.h>

static depz_vl53l8_reassembler reasm;   /* depz_vl53l8_reasm_init(&reasm) once */

void on_frame_chunk(const depz_event *ev)
{
    if (ev->type != DEPZ_EV_PACKET || ev->cmd != DEPZ_VL53L8_RPT_FRAME)
        return;
    depz_vl53l8_chunk chunk;
    if (depz_vl53l8_unpack_chunk(ev->payload, ev->payload_len, &chunk) != 0)
        return;

    const uint8_t *raw;
    size_t raw_len;
    uint64_t ts;
    if (depz_vl53l8_reasm_feed(&reasm, &chunk, &raw, &raw_len, &ts) != 1)
        return;                                  /* frame not complete yet */

    depz_vl53l8_frame f;
    if (depz_vl53l7_decode_frame(raw, raw_len, ts, &f, NULL, 0, NULL) != 0)
        return;                                  /* corrupt or short frame */

    if (f.resolution == DEPZ_VL53L8_RES_8X8) {
        /* the centre of an 8x8 grid is zones 27, 28, 35, 36 (row-major) */
        int32_t centre = (f.distance_mm[27] + f.distance_mm[28] +
                          f.distance_mm[35] + f.distance_mm[36]) / 4;
        printf("t=%llu us  centre %d mm  status %u  %d C\n",
               (unsigned long long)f.timestamp_us, (int)centre,
               (unsigned)f.target_status[27], (int)f.silicon_temp_degc);
    }
}
```

`depz_vl53l7_decode_frame()` returns `0` on success, `-1` on a header/footer
id mismatch (corrupt frame), `-2` on a bad length and `-3` when a CNH block
does not fit the buffer you passed (VL53L7CH only — see its
[guide](../vl53l7ch/guide.md)). The frame fields, their raw fixed-point
scaling and the status values are exactly the VL53L8 ones — see the
[VL53L8CX guide](../vl53l8cx/guide.md#the-frame).

### What differs from the VL53L8 decode

| | VL53L8CX (`depz_vl53l8_decode_frame`) | L5/L7 (`depz_vl53l7_decode_frame`) |
|---|---|---|
| footer id | at `size − 12` | at `size − 4` (`DEPZ_VL53L7_FOOTER_ID_OFFSET`) |
| per-target blocks in 4×4 | 16 entries | **64 entries**, first 16 filled — trimmed by the decoder |
| resolution | from the block layout | from the zone-sized ambient block (`0x54D0`) |
| CNH block | — | copied out on request (VL53L7CH) |
| chunk size | up to 1528 B | up to 1536 B |
| frame size | 8×8 ≈ 1.4 KB | 4×4 = 1060 B, 8×8 = 1444 B; up to ~7.6 KB with CNH |

`out->resolution` is 16 or 64 and every per-zone array past it is zero. Using
the VL53L8 decoder on an L5/L7 frame fails the footer check — use
`depz_vl53l7_decode_frame()` for all three L5/L7 boards.

## Gotchas

- **The model comes from the board**, not the silicon: USB PID, then device
  name, then VL53L7CX. `module_type` tells L5 from L7 after `init()`, never CX
  from CH.
- **No deep sleep and no threshold auto-stop on the L5CX / L7CX** (ULD 2.0.1);
  both fail with `DEPZ_E_ARG`. The L7CH has both.
- **Re-initialise after `LPN_OFF` / `SOFT_CYCLE`** — both wipe the sensor
  firmware; frames stop until `init()` runs again.
- **Bridge info during a stream costs a frame** — read it before and after a
  run, not in a polling loop.
- **Set the bus speed with ranging stopped** — mid-transfer the board answers
  `ERR_BUSY`.
- **1 Hz is fine here** — unlike the VL53L8, whose floor is 2 Hz.
- **Decode layer:** split register reads at 1536 bytes (a 2048-byte read is
  refused with `ERR_INVALID_PARAM`; `depz_vl53l7_pack_read_reg()` returns 0
  for it), decode frames with `depz_vl53l7_decode_frame()` (the VL53L8 decoder
  fails the footer check), and do not match `RPT_VL53_INFO` on `payload[0]` —
  it echoes no command byte.
- All the [VL53L8CX gotchas](../vl53l8cx/guide.md#gotchas) about the class
  apply too.
