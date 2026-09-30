# VL53L4CD — introduction

The **VL53L4CD** is ST's single-zone Time-of-Flight ranger (~1 mm resolution
to ~1.3 m). On the DEPZ sensor line the MCU is a **thin I2C register bridge**:
it owns only the I2C bus, the XSHUT and INT pins and one streaming FSM, while
the full ST Ultra-Lite Driver (ULD 2.2.3) runs **on the host**. This C SDK
opens the board, runs that ULD for you — ported register for register — and
hands you timestamped measurements as single shots, callbacks or streams.

## What the SDK covers

The VL53L4CD has a full **sensor class** in the C live layer
(`depz_sensor_io.h`), built on its codecs (`depz_sensor_sdk.h`):

- **Open** — `depz_open_device()` finds, probes and opens it with the class
  attached (firmware `APP_VL53L4_*`; `depz_is_vl53l4cd()` confirms);
  `depz_vl53l4cd_open_link()` puts the class on any link (tests, replay).
- **Initialise** — `depz_vl53l4cd_init()` is the ULD `sensor_init`: the
  91-byte default configuration block, VHV calibration, a 50 ms continuous
  default timing, and the bus re-timed to 1 MHz (~60 ms in all).
  `depz_vl53l4cd_is_alive()` checks the model id (`0xEBAA`).
- **Configure** — range timing (budget + inter-measurement period), offset,
  crosstalk, the distance-window interrupt (detection thresholds), signal and
  sigma quality limits, each with a getter that reads the sensor back;
  `depz_vl53l4cd_start_temperature_update()` re-runs VHV calibration.
- **Calibrate** — `depz_vl53l4cd_calibrate_offset()` and
  `depz_vl53l4cd_calibrate_xtalk()` against a target at a known distance.
- **Measure** — a polled single shot (`depz_vl53l4cd_measure_once`), or
  INT-driven ranging (`depz_vl53l4cd_start_ranging` / `_stop_ranging`, one
  measurement per INT edge) delivered as callbacks
  (`depz_vl53l4cd_on_measurement`), bounded drop-oldest streams
  (`depz_vl53l4cd_stream`) or one at a time (`depz_vl53l4cd_get_measurement`).
- **Power and diagnostics** — the XSHUT pin (`depz_vl53l4cd_xshut`,
  `depz_vl53l4cd_reset_sensor`), the bridge counters
  (`depz_vl53l4cd_bridge_info`), the bus speed, and raw register access
  (`depz_vl53l4cd_read_reg` / `_write_reg`) as an escape hatch.
- **Record / replay** — any session can be recorded to a `.depzrec` file and
  replayed without the board, byte-exact; a capture made by the Python SDK
  replays through this class unchanged.
- **Decode layer** — if you own the transport yourself, the codecs are still
  there: `depz_vl53l4_pack_*` / `depz_vl53l4_unpack_*`, the result-block
  decode (`depz_vl53l4_parse_result_block`), the range-timing register math,
  the tuning word codecs and the init configuration block.

The class was checked on a real VL53L4CD (firmware `APP_VL53L4_v0.83`) — see
[verified on hardware](guide.md#verified-on-hardware).

## When to use it

Reach for the VL53L4CD when you need precise single-point optical distance —
level sensing, presence, close-range positioning — without the
[SR04](../sr04/introduction.md)'s wide ultrasonic cone or the
[VL53L8](../vl53l8cx/introduction.md)'s multizone depth image.

## Key concepts

- **Host-side driver** — the board forwards register reads and writes; every
  ULD step (boot wait, configuration, start/stop, data-ready polling, the
  calibration loops) is a sequence of those, issued by this SDK. Configuration
  therefore lives in the **sensor**, not the board, and is lost when the sensor
  is power-cycled through XSHUT.
- **Measurement** — `depz_vl53l4cd_measurement` is `timestamp_us` (the MCU
  clock at the INT edge, or at the read for a single shot) plus a
  `depz_vl53l4_result`: `range_status` (0 = valid; see
  `depz_vl53l4cd_status_text()`), `distance_mm`, `sigma_mm`, signal / ambient
  rates, `number_of_spad` and the sensor's own `stream_count` (wraps at 255).
- **Range timing** — the *budget* (10–200 ms) is how long one measurement
  integrates; the *inter-measurement period* is 0 for continuous back-to-back
  ranging, or larger than the budget for autonomous low-power mode (the sensor
  sleeps between measurements). Anything in between is refused.
- **Stream vs poll** — while ranging, the board's streaming FSM owns the
  register bank: it reads the 17-byte result block on each INT edge and pushes
  it to the host. Configuration and single shots are refused with
  `DEPZ_E_ARG` until `depz_vl53l4cd_stop_ranging()`.
- **Detection thresholds** — a distance window (below / above / outside /
  inside) that gates INT, and so the stream: only measurements meeting it are
  reported.
- **I2C speed** — `init()` runs the configuration block at 400 kHz (the only
  speed an unconfigured sensor is specified for), then leaves the bus at
  1 MHz (`DEPZ_VL53L4CD_I2C_KHZ_DEFAULT`) unless you ask for another step.

## See also

- [VL53L4CD user guide](guide.md) — open, initialise, configure, calibrate,
  measure, stream, record and replay, the decode layer, gotchas.
- [API reference](api.md) — the VL53L4CD class (`depz_vl53l4cd_*`) and the
  VL53L4 codecs (`depz_vl53l4_*`).
- [Common guide](../guide.md#live-layer) — devices, errors, threading rules,
  streams and discovery shared by every board.
