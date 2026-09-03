# VL53L4CD — introduction

The **VL53L4CD** is ST's single-zone time-of-flight ranger (~1 mm resolution to
~1.3 m). On the DEPZ sensor line the MCU is a **thin I2C register bridge**: it
owns only the I2C bus, the XSHUT and INT pins and one streaming FSM, while the
full ST Ultra-Lite Driver (ULD 2.2.3) semantics run **on the host**. The Java
SDK provides both halves of the host side: the wire codecs
(`ai.depz.sensor.protocol.Vl53l4`) and the pure ULD decode/math
(`ai.depz.sensor.sensors.vl53l4.Vl53l4Uld`).

## What it does

- Single-zone ranging: one 17-byte result block per measurement — no zone
  grids, no firmware blob download, no chunked reassembly.
- Every ULD register sequence maps onto two commands: `READ_REG` (0x32) and
  `WRITE_REG` (0x33), with `XSHUT` (0x34) for the one thing a register write
  can't do.
- **Streaming**: `START_STREAM` (0x35) arms an INT-driven push of one
  configured register block (usually the whole result block at 0x0089, 17
  bytes) as `RPT_VL53_STREAM` (0x93) reports.
- **Diagnostics**: `GET_INFO` (0x37) returns bridge counters, the model id
  (expected 0xEBAA) and the programmed I2C speed.

## When to use it

Reach for the VL53L4CD when you need precise single-point optical distance —
level sensing, presence, close-range positioning — without the SR04's wide
ultrasonic cone or the VL53L8's multizone depth image.

## Key concepts

- **Register bridge** — register *contents* are big-endian sensor bytes passed
  through untouched; all wire fields (`addr`, `len`, timestamps) are
  little-endian like every other DEPZ payload. Transfers are capped at
  `Vl53l4.XFER_MAX` (253) bytes.
- **Result block** — `Vl53l4Uld.parseResultBlock` decodes the 17-byte block at
  0x0089 exactly as `VL53L4CD_GetResult()`: status via the `STATUS_RTN` table
  (0 = valid), rates ×8 kcps, sigma ÷4 mm, per-SPAD rates ×256 / raw SPADs.
- **Range timing** — `rangeTimingRegisters` / `decodeRangeTiming` reproduce the
  SetRangeTiming/GetRangeTiming register math bit-exactly (budget 10–200 ms;
  inter-measurement 0 = continuous, > budget = autonomous low power).
- **Init config block** — `Vl53l4Uld.configBlock()` is the 91-byte default
  configuration written at 0x2D, byte 0 forced to 0x12 (I2C Fast Mode Plus).
- **I2C speed** — the bridge boots at 400 kHz; after init the host may re-time
  it to 1 MHz with `SET_I2C_SPEED` (the block read is ~4× faster).

## See also

- [VL53L4CD user guide](guide.md) — build commands, decode reports, stream the
  result block, gotchas.
- [API reference](api.md) — `Vl53l4`, `Vl53l4.Vl53l4Cmd`, `Vl53l4.Vl53l4Rpt`,
  `Vl53l4Uld`.
