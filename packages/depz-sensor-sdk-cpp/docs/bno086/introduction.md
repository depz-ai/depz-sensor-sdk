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

In C++ that is the live class **`depz::Bno086`** (`depz/device.hpp`), a
wrapper over the C SDK's sensor class, which sends the same requests in the
same order as the Python SDK. One class serves both chips. Under it,
`depz/bno086.hpp` holds the pure codecs.

## What the SDK covers

- **Open** — `depz::open_bno086()` finds, probes and opens the board;
  `depz::open_device()` returns a `Bno086` too (`dynamic_cast` tells);
  `Bno086::open(link)` attaches the class to a link without a probe (tests,
  replay).
- **Identify and reset** — the product id (BNO085 or BNO086, SH-2 version),
  the reset pin, the wake pin.
- **Outputs** — enable any of the ~30 SH-2 outputs at a rate and get the rate
  the hub granted; reports arrive as callbacks, a bounded `Stream`, or one at
  a time, as `Bno086Report` with methods that scale the raw integers to m/s²,
  rad/s, µT and unit quaternions.
- **Calibration** — which sensors the hub may calibrate, saving and clearing
  the calibration, tare and a fixed mounting rotation.
- **FRS records** — read and write the chip's configuration records; sensor
  metadata (supply current, fastest rate, range, resolution).
- **Diagnostics** — the error queue, per-sensor event counts, the oscillator
  type; raw SH-2 command and SHTP escape hatches.
- **Record / replay** — sessions record to `.depzrec` and replay byte-exact;
  the Python SDK's BNO085 capture replays through this class.
- **Decode layer** — `depz::bno086`: SHTP reassembly (`ShtpLayer`), SH-2
  request encoders, input-report decoders.

**Status: verified live on a lab BNO085 and on a strict replay of a real
capture** — see [verification status](guide.md#verification-status).

## When to use it

Use the BNO085 / BNO086 whenever you need **orientation or motion** at a
rate you choose: heading and attitude for a robot or a handheld, head
tracking (the gyro-integrated rotation vector runs up to 1 kHz), step and
activity detection, image stabilisation. It offers more outputs and higher
rates than the [BNO055](../bno055/introduction.md), which is simpler to drive
(one register block, fixed 100 Hz fusion).

## Key concepts

- **Outputs are subscriptions.** Nothing streams until you enable an output
  with a rate; each enabled output then reports on its own. A hardware reset
  turns every output off.
- **The hub rounds the rate** to its 1000 / 2^n Hz grid (50 Hz becomes
  62.5 Hz); `enable()` returns what was granted. Right after enabling the hub
  may answer "0" once or twice — `enable()` asks again.
- **Answers are matched by content.** The bridge acknowledges each outgoing
  message at once and delivers the chip's answers later, mixed with the
  reports; the class matches them by report id, sensor id and command
  sequence number.
- **Raw integers are authoritative.** A report keeps the wire integers;
  `xyz()`, `quaternion()` and friends scale them (`raw / 2^Q`, fixed Q per
  sensor).
- **Timestamps are the MCU's** — microseconds on the board's clock;
  `sync_time()` maps them to host time.
- **Two messages in flight.** The bridge buffers two outgoing messages; a
  third is refused as busy, and the class waits 200 ms and resends.
- **The heading needs the magnetometer.** The rotation vector points to
  magnetic north and improves as the magnetometer calibrates; the game
  rotation vector leaves the magnetometer out.

## See also

- [BNO086 user guide](guide.md) — open, enable and read, units, callbacks,
  outputs, calibration and tare, FRS, diagnostics, errors, record and replay,
  the decode layer, gotchas.
- [API reference](api.md) — the codecs and the live class.
