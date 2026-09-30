# VL53L7CX — user guide

Hands-on guide to the [`vl53l7`](api.md) decode layer. It applies unchanged to
the [VL53L5CX](../vl53l5cx/guide.md) and, plus CNH, to the
[VL53L7CH](../vl53l7ch/guide.md). The results frame itself — fields, scaling,
the grid, the advanced DCI codecs — is the VL53L8 one: see the
[VL53L8CX guide](../vl53l8cx/guide.md). For concepts read the
[introduction](introduction.md); for signatures the [API reference](api.md).

This crate does not drive the board: initialising the sensor (firmware
download, default configuration) and starting a ranging session is done with
the Python or TypeScript SDK. What follows is everything you need to build the
board's commands and decode what it sends.

## Contents

- [Which board is it](#which-board-is-it)
- [Board commands](#board-commands)
- [The bridge info report](#the-bridge-info-report)
- [Register reads: split at 1536 bytes](#register-reads-split-at-1536-bytes)
- [Reassemble and decode a frame](#reassemble-and-decode-a-frame)
- [What differs from the VL53L8 decode](#what-differs-from-the-vl53l8-decode)
- [Gotchas](#gotchas)

## Which board is it

All three L5/L7 boards run the same firmware (`APP_VL53L7_*`, sensor type
`vl53l7`), so the firmware name cannot tell them apart.
[`resolve_model`](api.md#resolve_model) applies the contract order: the
production USB PID model, else the first `VL53L([57])(CX|CH)` in the device
name, else VL53L7CX:

```rust
use depz_sensor_sdk::usb_model_hint;
use depz_sensor_sdk::vl53l7::{resolve_model, Vl53l7Model};

// `vid`/`pid` from the USB enumeration, `device_name` from GET_DEVICE_NAME.
let model = resolve_model(usb_model_hint(Some(vid), Some(pid)), &device_name);
assert_eq!(resolve_model(None, "DEPZ ToF Sensor VL53L7CH USB v2.1"), Vl53l7Model::Vl53l7ch);
println!("{} (CNH: {})", model.as_str(), model.has_cnh());
```

## Board commands

The L5/L7 bridge adds three commands to the VL53L8 register bridge
([`Vl53l7Cmd`](api.md#vl53l7cmd)). Frame each payload with `build_packet`:

```rust
use depz_sensor_sdk::framing::{build_packet, CrcType};
use depz_sensor_sdk::vl53l7::{pack_pin_ctrl, pack_set_i2c_speed, Vl53l7Cmd, PIN_SOFT_CYCLE};

// LPn low 1 ms, high, I2C_RST pulse — the sensor firmware is lost afterwards.
let soft_cycle = build_packet(Vl53l7Cmd::PinCtrl as u8, &pack_pin_ctrl(PIN_SOFT_CYCLE), 0, CrcType::None)?;
// Re-time the sensor bus; the device snaps to the nearest I2C_SPEED_STEPS_KHZ entry.
let bus_400k = build_packet(Vl53l7Cmd::SetI2cSpeed as u8, &pack_set_i2c_speed(400), 0, CrcType::None)?;
// Ask for the bridge counters (answered with RPT_VL53_INFO).
let get_info = build_packet(Vl53l7Cmd::GetInfo as u8, &[], 0, CrcType::None)?;
```

| action | effect |
|---|---|
| [`PIN_LPN_OFF`](api.md#pin_lpn_off) | stop streaming, LPn low: sensor I2C interface off (state lost) |
| [`PIN_LPN_ON`](api.md#pin_lpn_on) | LPn high: interface on (the power-up default) |
| [`PIN_I2C_RST`](api.md#pin_i2c_rst) | pulse the sensor's I2C_RST |
| [`PIN_SOFT_CYCLE`](api.md#pin_soft_cycle) | stop streaming, LPn low 1 ms, high, I2C_RST pulse, clear the I2C counters (state lost) |

`VL53_SET_I2C_SPEED` is refused with `ERR_BUSY` while a transfer is in
flight — stop the stream first, and read the effective speed back from the
info report. The register commands (`VL53_READ_REG` 0x32, `VL53_WRITE_REG`
0x33, `VL53_START_STREAM` 0x35, `VL53_STOP_STREAM` 0x36) are the VL53L8 ones
(`vl53l8::Vl53l8Cmd`); their payload packers are re-exported from `vl53l7`.

## The bridge info report

`RPT_VL53_INFO` ([`Vl53l7Rpt::Info`](api.md#vl53l7rpt), `0x92`) is bridge state
only — the sensor is never probed — and carries **no** echoed command byte:

```rust
use depz_sensor_sdk::framing::{Event, PacketParser};
use depz_sensor_sdk::vl53l7::{i2c_error_name, Vl53l7Info, Vl53l7Rpt};

let mut parser = PacketParser::new();
for ev in parser.feed(&rx_bytes) {
    if let Event::Packet(pkt) = ev {
        if pkt.cmd == Vl53l7Rpt::Info as u8 {
            let info = Vl53l7Info::unpack(&pkt.payload)?;   // CodecError below 20 bytes
            println!(
                "{} kHz, {} I2C errors (last: {}), {} frames dropped, streaming: {}",
                info.i2c_khz,
                info.i2c_errors,
                i2c_error_name(info.last_i2c_error),
                info.frames_dropped,
                info.streaming
            );
        }
    }
}
```

Every `VL53_GET_INFO` takes the bus from the stream and can drop the frame in
flight (counted in `frames_dropped`): read it before and after a run, not in a
loop during one.

## Register reads: split at 1536 bytes

The L5/L7 bridge accepts at most [`READ_MAX_LEN`](api.md#read_max_len) = 1536
bytes per `VL53_READ_REG` (the VL53L8 host splits at 2048 — reusing that here
fails every large read with `ERR_INVALID_PARAM`) and
[`WRITE_MAX_LEN`](api.md#write_max_len) = 2048 per `VL53_WRITE_REG`:

```rust
use depz_sensor_sdk::framing::{build_packet, CrcType};
use depz_sensor_sdk::vl53l7::{pack_read_reg, RegData, READ_MAX_LEN};
use depz_sensor_sdk::vl53l8::Vl53l8Cmd;

// Read `total` bytes from register `addr` in L5/L7-sized pieces.
let mut requests = Vec::new();
let mut off = 0usize;
while off < total {
    let n = (total - off).min(READ_MAX_LEN);
    let payload = pack_read_reg(addr + off as u16, n as u16);
    requests.push(build_packet(Vl53l8Cmd::ReadReg as u8, &payload, 0, CrcType::None)?);
    off += n;
}

// Each reply is an RPT_VL53_REG_DATA (0x91): echoed opcode, timestamp, the bytes.
let reply = RegData::unpack(&reg_data_payload)?;
assert_eq!(reply.cmd, Vl53l8Cmd::ReadReg as u8);
```

## Reassemble and decode a frame

Frames arrive as `RPT_VL53_FRAME` chunks (up to 1536 data bytes each): a 4×4
frame is 1060 bytes and an 8×8 frame 1444 bytes, one chunk each; a CNH frame
on the VL53L7CH spans several. Reassemble with the shared
`vl53l8::FrameReassembler` and decode with `parse_frame` and
[`Variant::L7`](api.md#variant):

```rust
use depz_sensor_sdk::framing::{Event, PacketParser};
use depz_sensor_sdk::vl53l8::{parse_frame, unpack_frame_chunk, FrameReassembler, Variant, Vl53l8Rpt};

let mut parser = PacketParser::new();
let mut asm = FrameReassembler::new();
for ev in parser.feed(&rx_bytes) {
    let Event::Packet(pkt) = ev else { continue };
    if pkt.cmd != Vl53l8Rpt::Vl53Frame as u8 {
        continue;
    }
    let Some(chunk) = unpack_frame_chunk(&pkt.payload) else { continue };
    if let Some(done) = asm.feed(chunk) {
        let res = parse_frame(&done.frame, Variant::L7)?;   // Vl53l8Error on a corrupt frame
        let n = res.resolution();                           // 16 or 64, from the frame
        let side = if n == 64 { 8 } else { 4 };
        // The centre of an 8×8 grid is zones 27, 28, 35, 36.
        let centre = side / 2 * side + side / 2;
        if res.target_status[centre] == 5 || res.target_status[centre] == 9 {
            println!("{} µs: centre {} mm, {} °C", done.timestamp_us,
                     res.distance_mm[centre], res.silicon_temp_degc);
        }
    }
}
// asm.completed / asm.discarded count rebuilt and dropped frames
```

The result is a `Vl53l8Results` with the same fields and raw fixed-point as on
the VL53L8CX — see [the results table](../vl53l8cx/guide.md#the-results-fields-and-scaling).

## What differs from the VL53L8 decode

- **Footer id at `size-4`** for every L5/L7 part — both the ULD 2.0.1 blob of
  the L5CX / L7CX and the VL53LMZ 2.0.16 blob of the L7CH. `Variant::Cx`
  (footer at `size-12`) fails these frames with `CorruptedFrame`.
- **Per-zone arrays are trimmed to the frame's resolution.** On L5/L7 the
  per-target blocks (index ≥ `0x6C90`) keep their 64 entries even in 4×4: the
  sensor fills the first 16 and zero-pads the rest. `Variant::L7` reads the
  zone count from the ambient-rate block (one entry per zone) and cuts every
  array to it, so `res.distance_mm.len() == res.resolution()` in both
  resolutions.
- **Bigger chunks** — [`STREAM_CHUNK_MAX`](api.md#stream_chunk_max) is 1536
  (VL53L8: 1528). The reassembler does not depend on the chunk size.
- **Ranging frequency** — [`MIN_RANGING_FREQUENCY_HZ`](api.md#min_ranging_frequency_hz)
  = 1 (VL53L8: 2); maximum 60 Hz at 4×4, 15 Hz at 8×8.

The advanced DCI codecs in `vl53l8::advanced` (motion indicator, detection
thresholds, crosstalk margin) apply as on the VL53L8, within the L5CX / L7CX
firmware's limits: no `POWER_MODE_DEEP_SLEEP` and no threshold auto-stop.

## Gotchas

- **Decode with `Variant::L7`**, not `Cx` or `Ch` — only it trims the 4×4
  per-target blocks.
- **Split register reads at 1536 bytes**, not the VL53L8's 2048.
- **`RPT_VL53_INFO` has no echoed command byte** — don't match it to its
  request by `payload[0]`.
- **Pin control wipes the sensor** — after `PIN_LPN_OFF` / `PIN_SOFT_CYCLE`
  the sensor must be initialised again (firmware download included).
- **Bridge info during ranging costs a frame** — read the counters around a
  run.
- **Peel the protective film off the lens** before measuring — with the film
  on, weak-signal zones read 0.
