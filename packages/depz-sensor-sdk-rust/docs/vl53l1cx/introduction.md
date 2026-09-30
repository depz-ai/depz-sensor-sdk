# VL53L1CX — introduction

The **VL53L1CX** is ST's long-distance single-zone Time-of-Flight ranger (up
to ~4 m) with a programmable region of interest. It is one of the six VL53L 1D
boards that share one firmware (`APP_VL53L0_4`, contract 12, protocol v2.00):
the MCU is a thin I2C **register bridge** that knows no sensor, and every ST
driver runs **on the host**. Production USB PID `0xED43`, model id `0xEACC`.

## What the sensor does

- Single-zone distance up to **~4 m** per frame, with a range status, sigma
  (std-dev estimate), signal and ambient rates.
- INT-driven **push streaming**: on each data-ready edge the MCU reads the
  sensor's result block and ships it with a microsecond timestamp.
- Two ST drivers on this board:

| driver | what it is | streamed block |
|---|---|---|
| `uld` (default) | the VL53L1X Ultra Lite Driver: the die computes one distance per frame; modes `long` / `short`, tabulated budgets 20–500 ms | 17-byte die result block at `0x0089` |
| `histogram` | ST's Bare Driver: the die hands the host 24 photon-count bins per frame and the host finds **up to four targets** in them; modes `short` / `medium` / `long`, budgets 2–550 ms | 83-byte histogram block at `0x0088` |

## When to use it

Use it for up to ~4 m of single-point ranging with a narrowable field of view
(ROI), or with the histogram driver to see several targets at once (a glass
pane and the wall behind it). Behind a cover glass prefer the
[VL53L1CB](../vl53l1cb/introduction.md).

## What this crate offers

The [`vl53lx`](api.md) module is the family's verifiable **decode and codec
layer**:

- the v2.00 wire codecs — [`pack_set_addr_width`](api.md#pack_set_addr_width),
  [`pack_start_stream`](api.md#pack_start_stream) with its interrupt-release
  list, the 23-byte [`Vl53lxInfo`](api.md#vl53lxinfo) report — plus the
  register read/write/XSHUT codecs and the `StreamData` report shared with the
  VL53L4CD bridge;
- the [product table](api.md#products): per product and driver the
  register-address width, interrupt-release steps, bus ceiling and streamed
  block, and board → product → class resolution;
- stateless decode of the streamed blocks: the die result block as the
  VL53L1X ULD reads it ([`decode_die_block`](api.md#decode_die_block) with
  `DieVariant::L1`) and the histogram block's status bytes and 24 bins
  ([`decode_histogram_raw`](api.md#decode_histogram_raw)).

What it does **not** do: initialise the sensor, apply modes, budgets, ROI or
calibrations, or turn histogram bins into targets — that is the full driver.
Initialising and streaming the board is done today with the Python or
TypeScript SDK; this crate decodes what the board streams and builds the
command payloads.

## Key concepts

- **Product × driver kind** — the product (normally what is soldered on) and
  the driver kind are two separate choices; the table lists which pairs exist.
  A missing pair is a refusal, never a fallback.
- **The board, not the silicon, names the product** — the L1CX and L1CB share
  model id `0xEACC`; the production USB PID or the device name tells them
  apart.
- **The block says which driver produced it** — a die block (17 bytes at
  `0x0089`) or a histogram block (83 bytes at `0x0088`); `StreamData` echoes
  the address and length.
- **Plottable, not just valid** — the full drivers draw statuses 0, 6 and 11;
  `status == 0` alone drops usable frames.

## See also

- [VL53L1CX user guide](guide.md) — stream commands, die and histogram decode,
  the product table, gotchas.
- [API reference](api.md) — the `vl53lx` module and the codecs it re-exports.
