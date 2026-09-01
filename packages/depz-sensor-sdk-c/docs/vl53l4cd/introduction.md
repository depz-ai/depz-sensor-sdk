# VL53L4CD — introduction

The **VL53L4CD** is ST's single-zone Time-of-Flight ranger (~1 mm resolution
to ~1.3 m). On the DEPZ sensor line the MCU is a **thin I2C register bridge**:
it owns only the I2C bus, the XSHUT and INT pins and one streaming FSM, while
the full ST Ultra-Lite Driver (ULD 2.2.3) semantics run **on the host**. This
C SDK provides both halves of the host side: the wire codecs
(`depz_vl53l4_pack_*` / `depz_vl53l4_unpack_*`) and the pure ULD decode/math
(`depz_vl53l4_parse_result_block`, the range-timing register math, the tuning
word codecs and the init configuration block).

## What the SDK covers

- **Single-zone ranging** — one 17-byte result block per measurement. No zone
  grids, no firmware blob download, no chunked reassembly:
  `depz_vl53l4_parse_result_block()` decodes the block straight into a
  `depz_vl53l4_result`.
- **Register access** — every ULD register sequence maps onto two commands:
  `DEPZ_VL53L4_CMD_READ_REG` (`0x32`) and `DEPZ_VL53L4_CMD_WRITE_REG`
  (`0x33`), with `DEPZ_VL53L4_CMD_XSHUT` (`0x34`) for the one thing a register
  write can't do. Replies decode with `depz_vl53l4_unpack_reg_data()`.
- **Streaming** — `DEPZ_VL53L4_CMD_START_STREAM` (`0x35`) arms an INT-driven
  push of one configured register block (usually the whole result block at
  `DEPZ_VL53L4_RESULT_BLOCK_ADDR`, 17 bytes) as `DEPZ_VL53L4_RPT_STREAM`
  (`0x93`) reports, decoded with `depz_vl53l4_unpack_stream()`.
- **Diagnostics** — `DEPZ_VL53L4_CMD_GET_INFO` (`0x37`) returns bridge
  counters, the model id (expected `DEPZ_VL53L4_MODEL_ID`, `0xEBAA`) and the
  programmed I2C speed, decoded with `depz_vl53l4_unpack_info()`.
- **Host-ULD math** — the SetRangeTiming/GetRangeTiming register math
  (`depz_vl53l4_range_timing_registers` / `depz_vl53l4_decode_range_timing`),
  the tuning word codecs (offset, xtalk, signal/sigma thresholds) and the
  91-byte init configuration block (`depz_vl53l4_config_block`).

## When to use it

Reach for the VL53L4CD when you need precise single-point optical distance —
level sensing, presence, close-range positioning — without the
[SR04](../sr04/introduction.md)'s wide ultrasonic cone or the
[VL53L8](../vl53l8cx/introduction.md)'s multizone depth image.

## Key concepts

- **Register bridge** — register *contents* are big-endian sensor bytes passed
  through untouched; all wire fields (`addr`, `len`, timestamps) are
  little-endian like every other DEPZ payload. Transfers are capped at
  `DEPZ_VL53L4_XFER_MAX` (253) bytes.
- **Result block** — `depz_vl53l4_parse_result_block()` decodes the 17-byte
  block at `0x0089` exactly as `VL53L4CD_GetResult()`: status via the
  `STATUS_RTN` table (0 = valid), rates ×8 kcps, sigma ÷4 mm, per-SPAD rates
  ×256 / raw SPADs.
- **Range timing** — `depz_vl53l4_range_timing_registers()` /
  `depz_vl53l4_decode_range_timing()` reproduce the
  SetRangeTiming/GetRangeTiming register math bit-exactly (budget 10–200 ms;
  inter-measurement 0 = continuous, > budget = autonomous low power).
- **Init config block** — `depz_vl53l4_config_block()` writes the 91-byte
  default configuration for registers `0x2D..0x87`, byte 0 forced to
  `DEPZ_VL53L4_CONFIG_FMP_BYTE` (`0x12`, I2C Fast Mode Plus).
- **I2C speed** — the bridge boots at 400 kHz; after init the host may re-time
  it to 1 MHz with `DEPZ_VL53L4_CMD_SET_I2C_SPEED` (the block read is ~4×
  faster).

## See also

- [VL53L4CD user guide](guide.md) — build commands, decode reports, stream the
  result block, gotchas.
- [API reference](api.md) — `depz_vl53l4_cmd`, `depz_vl53l4_rpt`, the
  pack/unpack codecs and the host-ULD math.
- [Common guide](../guide.md) — the transport/decode mental model every sensor
  shares.
