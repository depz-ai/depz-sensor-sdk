# VL53L8CX — introduction

The **VL53L8CX** is ST's multizone Time-of-Flight sensor: a tiny laser depth
**imager** that returns a 4×4 or 8×8 grid of distances up to ~4 m. On the DEPZ
sensor line the MCU is a **thin SPI register bridge**: it owns the SPI bus,
the INT line and one frame-streaming FSM, while the full ST Ultra-Lite Driver
(ULD) runs **on the host**. This C SDK opens the board, runs that ULD for you —
the firmware download included, ported register for register from the Python
SDK — and hands you decoded depth frames as callbacks or streams.

This is the base sensor. Its superset, the **VL53L8CH**, adds Compact Network
Histograms and uses the same class with one more feature — see its
[introduction](../vl53l8ch/introduction.md) and [guide](../vl53l8ch/guide.md).

## What the SDK covers

The VL53L8CX and VL53L8CH share one **sensor class** in the C live layer
(`depz_sensor_io.h`), built on the VL53L8 codecs (`depz_sensor_sdk.h`) — the
same class also drives the I2C [VL53L5CX / VL53L7CX /
VL53L7CH](../vl53l7cx/introduction.md) boards:

- **Open** — `depz_open_device()` finds, probes and opens it with the class
  attached (firmware `APP_VL53L8_*`; `depz_is_vl53l8()` confirms,
  `depz_vl53l8_get_model()` says CX or CH); `depz_vl53l8_open_link(link,
  model, &dev)` puts the class on any link (tests, replay).
- **Initialise** — `depz_vl53l8_init()` is the ULD `init`: boot the sensor
  MCU, download its ~84 KB firmware in three banks, check the firmware
  checksum, upload the NVM offset data, the default crosstalk and the default
  configuration (0.76 s on the verified board), with an optional progress
  callback. `depz_vl53l8_is_alive()` checks the device id (`0xF0` / `0x0C`).
- **Configure** — resolution (4×4 / 8×8), ranging frequency, ranging mode
  (continuous / autonomous), integration time, sharpener, target order, each
  with a getter that reads the sensor back.
- **Advanced features** — power modes (sleep, wake-up, deep sleep), the
  crosstalk margin and calibration, the 776-byte crosstalk calibration blob
  (save / restore), per-zone detection thresholds, and the motion indicator.
- **Ranging** — `depz_vl53l8_start_ranging()` / `_stop_ranging()`; the board
  pushes each frame on the sensor's data-ready INT, the SDK reassembles and
  decodes it, and delivers `depz_vl53l8_live_frame`s as callbacks
  (`depz_vl53l8_on_frame`), bounded drop-oldest streams
  (`depz_vl53l8_frames`) or one at a time (`depz_vl53l8_get_frame`).
- **Escape hatches** — raw registers (`depz_vl53l8_read_reg` / `_write_reg`)
  and DCI indices (`depz_vl53l8_dci_read` / `_dci_write`).
- **Record / replay** — any session, the firmware download included, can be
  recorded to a `.depzrec` file and replayed without the board, byte-exact; a
  capture made by the Python SDK replays through this class unchanged.
- **Decode layer** — if you own the transport yourself, the codecs are still
  there: chunk parse, frame reassembly, the frame decoder and the pure halves of
  the advanced DCI features (crosstalk margin, thresholds, motion
  configuration).

The class was checked on a real VL53L8CH board (firmware `APP_VL53L8_v0.92`),
which runs the same code with the CH firmware blob — see
[verified on hardware](guide.md#verified-on-hardware). The CX path is covered
by the strict replay of a real VL53L8CX capture.

## When to use it

Reach for the VL53L8 when you need a **depth image** — zone occupancy,
gestures, small obstacle maps, people counting — in a package far smaller and
cheaper than a stereo camera, indoors or in the dark. For one precise distance
the [VL53L4CD](../vl53l4cd/introduction.md) is simpler; for orientation use
the [BNO086](../bno086/introduction.md). Reach for the
[VL53L8CH](../vl53l8ch/introduction.md) only when you need the raw return
histograms.

## Key concepts

- **Host-side driver** — the board forwards register reads and writes; every
  ULD step (the boot sequence, the firmware download, configuration, start /
  stop, calibration) is a sequence of those, issued by this SDK. Configuration
  therefore lives in the **sensor** and is lost with its power (or in deep
  sleep).
- **DCI** — the sensor's Device Configuration Interface: numbered
  configuration blocks inside the sensor firmware that the ULD reads, patches
  and writes back through the register bridge. Every setter here is one or more
  DCI writes.
- **Model** — `DEPZ_VL53L8_MODEL_L8CX` or `DEPZ_VL53L8_MODEL_L8CH` (the
  `_L5CX` / `_L7CX` / `_L7CH` models are the I2C boards). Both VL53L8 boards
  run the same bridge firmware and report the same `VL53L8` identity; the model
  only picks the sensor-firmware blob `init()` downloads (CX = ULD 2.1.0, CH =
  VL53LMZ ULD 2.0.16) and whether CNH is available.
- **Resolution** — 16 zones (4×4) or 64 (8×8). A frame's arrays are valid for
  `[0, resolution)`, row-major.
- **Ranging frequency** — at least **2 Hz**: below that a VL53L8 never enters
  its ranging loop and streams nothing, so the SDK refuses it. The sensor's
  maximum is 60 Hz at 4×4 and 15 Hz at 8×8.
- **Stream vs configuration** — while ranging, the board's streaming FSM owns
  the sensor's register bank. Configuration is refused with `DEPZ_E_ARG` until
  `depz_vl53l8_stop_ranging()`.
- **Raw fixed-point frame values** — the frame arrays hold the wire integers
  (`range_sigma_mm = raw/128`, signal and ambient in raw kcps/SPAD); only
  `distance_mm` is pre-scaled to millimetres. `target_status` 5 or 9 means a
  valid range, 255 no target.

## See also

- [VL53L8CX user guide](guide.md) — open, initialise, configure, stream, the
  frame, thresholds, motion, crosstalk, power modes, record and replay, the
  decode layer, gotchas.
- [VL53L8CH docs](../vl53l8ch/introduction.md) — the CNH superset.
- [API reference](api.md) — the VL53L8 class (`depz_vl53l8_*`) and the VL53L8
  codecs.
- [Common guide](../guide.md#live-layer) — devices, errors, threading rules,
  streams and discovery shared by every board.
