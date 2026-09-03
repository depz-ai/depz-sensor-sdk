# VL53L4CD — user guide

Hands-on guide to the [`vl53l4`](api.md) wire codecs and the host-ULD math. For
what the sensor is and its concepts, read the [introduction](introduction.md);
for exact signatures see the [API reference](api.md). This is a decode layer —
the snippets build command payloads and decode reply/report payloads; you
supply the transport that carries them (see the
[common guide](../guide.md#getting-started)).

## Contents

- [The codec surface](#the-codec-surface)
- [Read and write registers](#read-and-write-registers)
- [Stream the result block](#stream-the-result-block)
- [Decode a result block](#decode-a-result-block)
- [Range timing](#range-timing)
- [Tuning words](#tuning-words)
- [Gotchas](#gotchas)

## The codec surface

- [`Vl53l4Cmd`](api.md) — host→device opcodes (`ReadReg`, `WriteReg`, `Xshut`,
  `StartStream`, `StopStream`, `GetInfo`, `SetI2cSpeed`), each a `u8` opcode
  (`as u8`).
- [`Vl53l4Rpt`](api.md) — device→host report ids (`RegData`, `Info`, `Stream`).
- [`RegData`](api.md) / [`Vl53l4Info`](api.md) / [`StreamData`](api.md) —
  decoded reports, each with an `unpack` constructor.
- [`uld_math`](api.md) — the pure ULD pieces: [`parse_result_block`](api.md),
  the range-timing register math, the tuning-word codecs and
  [`config_block`](api.md).

## Read and write registers

```rust
use depz_sensor_sdk::framing::{build_packet, CrcType};
use depz_sensor_sdk::vl53l4::{self, uld_math, RegData, Vl53l4Cmd};

// read the model id word (expect 0xEBAA)
let rd = build_packet(
    Vl53l4Cmd::ReadReg as u8,
    &vl53l4::pack_read_reg(uld_math::IDENTIFICATION__MODEL_ID, 2),
    seq, CrcType::Crc16)?;

// the reply is RPT_VL53_REG_DATA (0x91)
let d = RegData::unpack(&reply_payload)?;

// write the 91-byte init configuration block in one transaction
let wr = build_packet(
    Vl53l4Cmd::WriteReg as u8,
    &vl53l4::pack_write_reg(vl53l4::CONFIG_ADDR, &vl53l4::config_block()),
    seq, CrcType::Crc16)?;
```

Reads and writes are capped at [`XFER_MAX`](api.md) (253) bytes and
`addr + len` ≤ 0x10000. Register contents are big-endian; the wire fields are
little-endian.

## Stream the result block

```rust
use depz_sensor_sdk::vl53l4::{self, StreamData, Vl53l4Cmd};

// arm INT-driven streaming of the 17-byte result block
let start = build_packet(
    Vl53l4Cmd::StartStream as u8,
    &vl53l4::pack_start_stream(vl53l4::RESULT_BLOCK_ADDR, vl53l4::RESULT_BLOCK_LEN, 0),
    seq, CrcType::Crc16)?;

// each sample arrives as RPT_VL53_STREAM (0x93)
let s = StreamData::unpack(&stream_payload)?;
let r = vl53l4::parse_result_block(&s.data)?;
```

`flags` bit [`SF_INT_ACT_HIGH`](api.md) (0x02) selects INT-active-high; the
default (0) matches the ULD init block (INT active low). `timestamp_us` is MCU
uptime at the **INT edge** — the sensor event, not the I2C completion.

## Decode a result block

```rust
use depz_sensor_sdk::vl53l4::parse_result_block;

let r = parse_result_block(&raw17)?;
if r.range_status == 0 {                     // 0 = valid
    println!("{} mm, sigma {} mm", r.distance_mm, r.sigma_mm);
} else {
    println!("suspect: {}", r.status_text());
}
```

[`Vl53l4Results`](api.md) carries `range_status` (via the
[`STATUS_RTN`](api.md) table; raw ≥ 24 passes through unmapped),
`distance_mm`, `signal_rate_kcps`/`ambient_rate_kcps` (×8), per-SPAD rates,
`number_of_spad`, `sigma_mm` and the sensor's own `stream_count` frame counter
(wraps at 255).

## Range timing

The SetRangeTiming/GetRangeTiming register math is pure and bit-exact:

```rust
use depz_sensor_sdk::vl53l4::{decode_range_timing, range_timing_registers};

// SetRangeTiming(50 ms budget, continuous): osc_frequency from reg 0x0006,
// clock_pll from RESULT__OSC_CALIBRATE_VAL (only used in autonomous mode)
let (cfg_a, cfg_b, inter_raw) =
    range_timing_registers(50, 0, osc_frequency, clock_pll)?;
// cfg_a → RANGE_CONFIG_A, cfg_b → RANGE_CONFIG_B, inter_raw → INTERMEASUREMENT_MS

// GetRangeTiming readback
let (budget_ms, inter_ms) =
    decode_range_timing(inter_raw, clock_pll, osc_frequency, cfg_a)?;
```

Budget is 10..200 ms. `inter_measurement_ms == 0` selects continuous mode; a
value **greater** than the budget selects autonomous low power; anything else
returns [`Vl53l4Error::InterMeasurementInvalid`](api.md).

## Tuning words

Each tuning register is a word codec pair (encode for the write, decode for the
readback):

```rust
use depz_sensor_sdk::vl53l4 as v4;

let word = v4::offset_raw(-10);              // RANGE_OFFSET_MM (0x001E)
let mm = v4::decode_offset(word);            // → -10

v4::xtalk_raw(20);                           // XTALK_PLANE_OFFSET_KCPS (0x0016)
v4::signal_threshold_raw(1024);              // MIN_COUNT_RATE_RTN_LIMIT_MCPS (0x0066)
v4::sigma_threshold_raw(15)?;                // RANGE_CONFIG__SIGMA_THRESH (0x0064)
```

`sigma_threshold_raw` is the one fallible encoder
([`Vl53l4Error::SigmaTooLarge`](api.md) above 16383 mm).

## Gotchas

- **Check `range_status` before trusting a distance** — 0 is valid; everything
  else describes why the measurement is suspect
  ([`range_status_name`](api.md); raw ≥ 24 is unmapped).
- **Two endiannesses on purpose** — wire fields little-endian, register
  contents big-endian. The codecs handle both; don't swap bytes yourself.
- **After `XSHUT` OFF/RESET the sensor holds none of the ULD configuration** —
  re-run the init sequence (config block, VHV, timing).
- **Configure the sensor before raising the bus speed** — init at 400 kHz
  ([`I2C_KHZ_BOOT`](api.md)), then `SetI2cSpeed` to 1 MHz; read `i2c_khz` back
  from `GET_INFO` for the programmed step (the command answers OK
  unconditionally).
- **Skipped stream slots produce no per-slot error** — watch the
  [`Vl53l4Info`](api.md) counters (`slots_skipped`, `i2c_errors`) and gaps in
  `timestamp_us`; `stream_count` tells a frame the host never received from
  one the sensor never produced.
- **The live driver is host code you write** — `uld_math` is the pure math
  only; `sensor_init`, VHV calibration and ranging loops are a documented
  extension point (see [What it is not](../guide.md#what-it-is-not)), driven
  over `ReadReg`/`WriteReg`.
