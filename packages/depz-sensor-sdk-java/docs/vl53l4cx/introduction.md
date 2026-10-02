# VL53L4CX — introduction

The **VL53L4CX** is ST's extended-range sibling of the
[VL53L4CD](../vl53l4cd/introduction.md): up to ~6 m, histogram only. It is one
of the six VL53L 1D boards that share one firmware (`APP_VL53L0_4`, contract
12, protocol v2.00): the MCU is a thin I2C **register bridge** that knows no
sensor, and every ST driver runs **on the host**. Production USB PID `0xED46`,
model id `0xEBAA` (the same as the VL53L4CD).

## What the sensor does

- Single-zone distance up to **~6 m** per frame, with a range status, sigma
  (std-dev estimate), signal and ambient rates — and **several targets per
  frame**.
- INT-driven **push streaming**: on each data-ready edge the MCU reads the
  sensor's result block and ships it with a microsecond timestamp.
- One ST driver on this board:

| driver | what it is | streamed block |
|---|---|---|
| `histogram` | ST's Bare Driver: the die hands the host 24 photon-count bins per frame and the host finds **up to four targets** in them; modes `medium` / `long` (no `short` on an L4 die), budgets 2–200 ms | 83-byte histogram block at `0x0088` |

## When to use it

Use it for the widest range on a small board (up to ~6 m), with several
targets per frame. If you need offset/crosstalk calibration or thresholds, the
full drivers can run it on the VL53L4CD's light driver instead (reach drops to
~1.4 m); the board then streams the VL53L4CD's die block.

## What this SDK offers

[`Vl53lx`](api.md#vl53lx) (wire codecs), [`Vl53lxProducts`](api.md#vl53lxproducts)
(product table) and [`Vl53lxDecode`](api.md#vl53lxdecode) (block decode) are
the family's verifiable **decode and codec layer**:

- the v2.00 wire codecs — `Vl53lx.packSetAddrWidth`,
  `Vl53lx.packStartStream` with its interrupt-release
  list, the 23-byte [`Vl53lx.Vl53lxInfo`](api.md#vl53lxvl53lxinfo) report — plus the
  register read/write/XSHUT encoders and the `Vl53l4.StreamData` report
  shared with the VL53L4CD bridge;
- the product table ([`Vl53lxProducts.TABLE`](api.md#vl53lxproducts)) and board → product → class
  resolution, including the borrowed VL53L4CD driver;
- stateless decode of the streamed blocks: the histogram block's status bytes
  and 24 bins (`Vl53lxDecode.decodeHistogramRaw`), and the
  die block (`Vl53lxDecode.decodeDieBlock` with
  `DieVariant.L4`) when running as a VL53L4CD.

What it does **not** do: initialise the sensor, apply presets and budgets, or
turn the 24 bins into targets — that is the full driver. Initialising and
streaming the board is done today with the Python or TypeScript SDK; this
SDK decodes what the board streams and builds the command payloads.

## Key concepts

- **Histogram only** — the product table has no `uld` pair for the L4CX;
  asking for one is a refusal, not a fallback. Naming the VL53L4CD borrows its
  driver.
- **Same silicon id as the VL53L4CD** — `0xEBAA`; the board names the
  product.
- **Plottable, not just valid** — the full drivers draw statuses 0, 6 and 11.

## See also

- [VL53L4CX user guide](guide.md) — stream commands, histogram decode, the
  borrowed driver, gotchas.
- [API reference](api.md) — `Vl53lx`, `Vl53lxProducts`, `Vl53lxDecode` and the
  contract-10 codecs they forward to.
