# VL53L4CX — introduction

The **VL53L4CX** is ST's extended-range sibling of the VL53L4CD: up to ~6 m, histogram only. It is one of the VL53L 1D boards that share one
firmware (`APP_VL53L0_4`, USB PID `0xED46`): the MCU is a thin I2C
**register bridge** that knows no sensor, and every ST driver runs **on the
host** — in this SDK, the **1D-family sensor class** (`depz_vl53lx_*` in `depz_sensor_io.h`), request for request as the Python SDK does.

| driver | what it is | modes | timing budget |
|---|---|---|---|
| `histogram` | ST's Bare Driver: the die hands over 24 photon-count bins per frame and the host finds **up to four targets** in them | `short`, `medium` (init), `long` | any 2–550 ms |

## What the SDK covers

- **Open** — `depz_open_device()` finds, probes and opens the board with the class attached (`depz_is_vl53lx()`); `depz_vl53lx_open_link()` attaches it to a link without a probe (tests, replay).
- **Initialise** — bind a (product, driver) pair: the product defaults to the
  one the board's name carries, the driver to its first kind (`histogram`).
- **Configure** — one call re-initialises the sensor and applies timing
  budget, mode and a stored calibration; the capability groups each driver
  serves (modes, calibrations, thresholds, ROI…) are asked, not guessed.
- **Measure** — an INT-driven stream (callbacks, a bounded stream, one at a
  time) or poll-mode single shots, one measurement shape for every product, with up to four targets and the raw 24-bin histogram on the histogram driver.
- **Board** — XSHUT, the bridge's counters, raw register access.
- **Record / replay** — captures made by the Python SDK replay strictly through the class.
- **Decode layer** — the bridge codecs, the product table and the stateless
  block decoders, for when you own the transport.

**Status: verified live on the lab VL53L4CX from C and on strict replays of real captures** — see [verification status](guide.md#verification-status).

## Key concepts

- **One firmware, six products** — the bridge cannot tell the parts apart;
  the product comes from the USB PID or the board's device name. The model
  id (`0xEBAA`) is a cross-check only.
- **`configure()` before every run** — it re-initialises the sensor; nothing
  from an earlier session carries over.
- **Borrowing a driver** — naming another product loads its driver on this die: the VL53L4CD light driver (single target, ~1.2 m, offset and crosstalk calibration, thresholds) — or name `VL53L1CX` for its light driver (long / short modes, ROI, calibrations).
- **Status 0 is not the whole story** — `depz_vl53lx_plottable()` also accepts 6 (first
  histogram frame) and 11 (merged target): those are real distances.
- **Initialise at 400 kHz** — every init runs the bus at 400 kHz and the
  driver raises it to the product's ceiling afterwards.

## See also

- [VL53L4CX user guide](guide.md) — open and init, configure, streaming, the
  measurement, drivers and capabilities, calibration, diagnostics, replay,
  the decode layer, gotchas.
- [API reference](api.md) — the codecs and the sensor class.
