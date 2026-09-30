# VL53L3CX — introduction

The **VL53L3CX** is ST's multi-target Time-of-Flight ranger: up to ~3 m, with
a histogram that can resolve several objects along the beam. It is one of the
six VL53L 1D boards that share one firmware (`APP_VL53L0_4`, contract 12,
protocol v2.00): the MCU is a thin I2C **register bridge** that knows no
sensor, and every ST driver runs **on the host**. Production USB PID
`0xED44`, model id `0xEAAA`.

## What the sensor does

- Single-zone distance up to **~3 m** per frame, with a range status, sigma
  (std-dev estimate), signal and ambient rates.
- INT-driven **push streaming**: on each data-ready edge the MCU reads the
  sensor's result block and ships it with a microsecond timestamp.
- Two ST drivers on this board:

| driver | what it is | streamed block |
|---|---|---|
| `ulp` (default) | ST's Ultra Low Power driver: one target, minimal bus traffic; budgets 10–200 ms | 17-byte die result block at `0x0089` |
| `histogram` | ST's Bare Driver: the die hands the host 24 photon-count bins per frame and the host finds **up to four targets** in them; modes `short` / `medium` / `long`, budgets 2–550 ms | 83-byte histogram block at `0x0088` |

## When to use it

Use it when you need to see through: a glass pane and the object behind it,
or two people at different distances — the histogram driver reports up to
four targets per frame. The ULP driver is the low-power single-target option.

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
  resolution;
- stateless decode of the streamed blocks: the die result block as the ULP
  reads it (`Vl53lxDecode.decodeDieBlock` with
  `DieVariant.L4`) and the histogram block's status bytes and 24 bins
  (`Vl53lxDecode.decodeHistogramRaw`).

What it does **not** do: initialise the sensor, apply budgets or modes, or
turn histogram bins into targets — that is the full driver. Initialising and
streaming the board is done today with the Python or TypeScript SDK; this
SDK decodes what the board streams and builds the command payloads.

## Key concepts

- **The multi-target part is the host's work** — the histogram block carries
  24 bins; finding up to four targets in them is the full driver.
- **The ULP reads the die like the L4CD ULD** — `DieVariant.L4`.
- **No calibrations** on either driver.
- **Plottable, not just valid** — the full drivers draw statuses 0, 6 and 11.

## See also

- [VL53L3CX user guide](guide.md) — stream commands, die and histogram decode,
  the product table, gotchas.
- [API reference](api.md) — `Vl53lx`, `Vl53lxProducts`, `Vl53lxDecode` and the
  contract-10 codecs they forward to.
