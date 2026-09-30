# BNO086 — introduction

The **BNO085** and **BNO086** are CEVA 9-axis IMUs: accelerometer, gyroscope
and magnetometer with a sensor hub on the chip that fuses them into
ready-to-use outputs — orientation as a quaternion (four numbers describing a
rotation), gravity, linear acceleration, step counting, tap and activity
detection — each at its own rate. On the DEPZ sensor line (`APP_BNO086`
firmware, USB PIDs `0xEE08` BNO086 and `0xEE09` BNO085) the MCU is a thin
**pass-through**: it forwards the chip's SHTP messages (its transport
protocol) both ways, and the SH-2 protocol that runs the hub (enable an
output at a rate, calibrate, read settings) runs on the host.

This C SDK runs SH-2 for you in the **BNO086 sensor class** (`depz_bno086_*`),
request for request as the Python SDK does. One class serves both chips.

## What the SDK covers

- **Open** — `depz_open_device()` finds, probes and opens the board with the
  class attached; the device reports sensor type `bno086`
  (`DEPZ_SENSOR_BNO086`) and `depz_is_bno086()` is true for it.
  `depz_bno086_open_link(link, &dev)` attaches it to a link without a probe
  (tests, replay).
- **Identify and reset** — the chip's product id (part number and SH-2
  version: BNO085 or BNO086), the reset pin, the wake pin.
- **Outputs** — enable any of the ~30 SH-2 outputs at a rate, read back the
  rate the hub granted, disable; reports arrive as callbacks, a bounded
  stream, or one at a time, with helpers that scale the raw integers to
  m/s², rad/s, µT and unit quaternions.
- **Calibration** — which sensors the hub may calibrate, saving the
  calibration to the chip's flash, clearing it; tare (make the current
  orientation the zero) and a fixed mounting rotation.
- **FRS records** — read and write the chip's configuration records; sensor
  metadata (supply current, fastest rate, range, resolution).
- **Diagnostics** — the hub's error queue, per-sensor event counts, the
  oscillator type; raw SH-2 command and SHTP escape hatches.
- **Record / replay** — any session records to a `.depzrec` file and replays
  byte-exact; the Python SDK's BNO085 capture replays through this class
  unchanged.
- **Decode layer** — if you own the transport: the bridge command and
  `RPT_DATA` codecs, SHTP framing and reassembly, the SH-2 request encoders
  and answer parsers, the input-report parsers and the scaling helpers.

**Status: verified live on a lab BNO085** (rotation vector 100 Hz, game
rotation vector 200 Hz, gravity 9.85 m/s², no drops) **and on a strict replay
of a real capture** — see [verification status](guide.md#verification-status).

## When to use it

Use the BNO085 / BNO086 whenever you need **orientation or motion** at a
rate you choose: heading and attitude for a robot or a handheld, head
tracking (the gyro-integrated rotation vector runs up to 1 kHz), step and
activity detection, image stabilisation. It offers more outputs and higher
rates than the [BNO055](../bno055/introduction.md), which is simpler to drive
(one register block, fixed 100 Hz fusion).

## Key concepts

- **Outputs are subscriptions.** Nothing streams until you enable an output
  with a rate ("Set Feature"); each enabled output then reports on its own at
  that rate. A hardware reset turns every output off.
- **The hub rounds the rate** to its 1000 / 2^n Hz grid (50 Hz becomes
  62.5 Hz). `enable()` reads back what was granted; right after enabling the
  hub may answer "0" once or twice, and `enable()` asks again.
- **Answers are matched by content.** The bridge acknowledges each outgoing
  message at once and delivers the chip's answers later, mixed with the
  report stream. The class matches them the SH-2 way: by report id, sensor
  id and command sequence number.
- **Raw integers are authoritative.** A report keeps the wire integers; the
  scaled value is `raw / 2^Q` with a fixed Q per sensor (accelerometer Q8,
  gyroscope Q9, magnetometer Q4, quaternions Q14). The helpers do it.
- **Timestamps are the MCU's.** Each report's `timestamp_us` is the bridge's
  capture time corrected by the delay the chip reports — microseconds on the
  board's clock; `depz_device_sync_time()` maps it to host time.
- **Two messages in flight.** The bridge buffers two outgoing messages; a
  third is refused as busy. The class waits 200 ms and resends.
- **The heading needs the magnetometer.** The rotation vector points to
  magnetic north and its accuracy improves as the magnetometer calibrates;
  the game rotation vector leaves the magnetometer out.

## See also

- [BNO086 user guide](guide.md) — open, enable and read, units, callbacks,
  outputs, calibration and tare, FRS, diagnostics, errors, record and replay,
  the decode layer, gotchas.
- [API reference](api.md) — the codecs and the sensor class.
