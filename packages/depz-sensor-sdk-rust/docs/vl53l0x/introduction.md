# VL53L0X — introduction

The **VL53L0X** is the first-generation ST single-zone Time-of-Flight ranger:
one laser distance per measurement up to ~2 m. It is one of the six VL53L 1D
boards that share one firmware (`APP_VL53L0_4`, contract 12, protocol v2.00):
the MCU is a thin I2C **register bridge** that knows no sensor, and every ST
driver runs **on the host**. Production USB PID `0xED41`, model id `0x00EE`.

## What the sensor does

- Single-zone distance up to **~2 m** per frame, with a range status, signal
  and ambient rates.
- INT-driven **push streaming**: on each data-ready edge the MCU reads the
  sensor's result block and ships it with a microsecond timestamp.
- One ST driver on this board:

| driver | what it is | streamed block |
|---|---|---|
| `uld` | ST's VL53L0X API 1.0.4 (the PAL), ported to the host; modes `default`, `long-range`, `high-speed`, `high-accuracy`; budgets 20–200 ms | 12-byte result block at `0x14` |

## When to use it

Use it for a cheap, narrow-beam distance up to ~2 m when a single number is
enough: presence, level, proximity. For more reach, targets through a cover
glass or several targets per frame, pick a VL53L1/L3/L4 board.

## What this crate offers

The [`vl53lx`](api.md) module is the family's verifiable **decode and codec
layer**:

- the v2.00 wire codecs — [`pack_set_addr_width`](api.md#pack_set_addr_width)
  (the VL53L0X is the one family member with **1-byte** register addresses),
  [`pack_start_stream`](api.md#pack_start_stream) with its interrupt-release
  list, the 23-byte [`Vl53lxInfo`](api.md#vl53lxinfo) report — plus the
  register read/write/XSHUT codecs and the `StreamData` report shared with the
  VL53L4CD bridge;
- the [product table](api.md#products) and board → product → class
  resolution;
- the stateless decode of the result block's **raw fields**
  ([`decode_l0x_raw`](api.md#decode_l0x_raw)): distance, the raw device status
  byte, signal and ambient rates, effective SPAD count.

What it does **not** do: initialise the sensor (reference SPADs, calibration,
profiles), or compute the PAL range status, sigma and maximum distance — those
need device data the driver caches at init. Initialising and streaming the
board is done today with the Python or TypeScript SDK; this crate decodes what
the board streams and builds the command payloads.

## Key concepts

- **1-byte register addresses, 400 kHz** — set the bridge's address width to
  1 before the first register access; the bus ceiling stays at 400 kHz.
- **Raw fields, not the PAL result** — the block alone gives the distance and
  rates; the status the full driver reports comes from its init state.
- **Calibrate the offset** — uncalibrated boards read several centimetres
  long; the full driver's offset calibration fixes it.

## See also

- [VL53L0X user guide](guide.md) — stream commands, raw-field decode, the
  product table, gotchas.
- [API reference](api.md) — the `vl53lx` module and the codecs it re-exports.
