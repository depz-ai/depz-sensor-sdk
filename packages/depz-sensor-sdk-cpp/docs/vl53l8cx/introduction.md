# VL53L8CX — introduction

The **VL53L8CX** is a STMicroelectronics multizone Time-of-Flight ranging
sensor: a tiny laser depth **imager** that returns a 4×4 or 8×8 grid of
distances up to ~4 m. On the DEPZ sensor line the MCU is a **thin SPI register
bridge**: it owns the SPI bus, the INT pin and one streaming loop, while the
ST Ultra-Lite Driver (ULD) runs **on the host** — in the C SDK, register for
register. The C++ SDK opens the board, downloads the sensor firmware,
configures the sensor, streams frames and hands each one over decoded.

This is the base ToF sensor. Its superset — the **VL53L8CH**, which adds
Compact Network Histograms — has its own [introduction](../vl53l8ch/introduction.md)
and [guide](../vl53l8ch/guide.md); everything on this page applies to it
unchanged.

The VL53L8CX is covered by both layers of the SDK:

- **The live class** `depz::Vl53l8` (`depz/device.hpp`) — one class for the
  VL53L8CX and the VL53L8CH (and, on the I2C board, the
  [VL53L5CX / L7CX / L7CH](../vl53l7cx/introduction.md)): open the board, `init()` the sensor (the firmware
  download included), set resolution, frequency, ranging mode, sharpener,
  target order, power mode, crosstalk and detection thresholds, arm the
  motion indicator, `start_ranging()` / `stop_ranging()`, and receive
  `Vl53l8LiveFrame`s as callbacks or from a pull stream. It builds on the C
  SDK; see the [common guide](../guide.md#live-hardware) for the build and the
  rules every device shares.
- **The decode layer** (`depz::vl53l8`, `depz/vl53l8.hpp`) — pure codecs:
  frame-chunk reassembly, the shared results-frame decoder (raw per-zone
  arrays) and the advanced-feature DCI codecs (xtalk margin, detection
  thresholds, motion indicator), for bytes you read yourself (your own port
  loop, a recording). Byte-exact with the other SDKs via the shared golden
  vectors.

```cpp
#include <cstdio>
#include "depz/device.hpp"

int main() {
    auto tof = depz::open_vl53l8();            // a VL53L8CX or VL53L8CH
    tof->init();                               // firmware download + config, ~0.8 s
    tof->set_resolution(depz::vl53l8::RESOLUTION_8X8);
    tof->set_ranging_frequency_hz(15);
    tof->start_ranging();
    depz::Vl53l8LiveFrame f = tof->get_frame();
    std::printf("zone 27: %d mm, %d °C\n", f.frame.distance_mm[27], f.frame.silicon_temp_degc);
    tof->stop_ranging();
}
```

## What it does

- Per-zone distance over a **16-zone (4×4)** (`RESOLUTION_4X4`) or **64-zone
  (8×8)** (`RESOLUTION_8X8`) grid, at 2..60 Hz (4×4) / 2..15 Hz (8×8).
- Each frame carries far more than distance: per-zone target status, number of
  targets, signal and ambient rate, range sigma, reflectance, plus a per-frame
  silicon temperature — all decoded into `vl53l8::Vl53l8Frame`.
- INT-driven **push streaming**: on each data-ready the MCU reads the results
  frame and ships it to the host in ≤`STREAM_CHUNK_MAX` (1528-byte) chunks;
  the live class reassembles and decodes them and delivers them to
  `on_frame()` callbacks and `frames()` streams.
- **The ULD's configuration surface**: continuous or autonomous ranging,
  integration time, sharpener, target order, sleep / wake / deep sleep,
  crosstalk margin, crosstalk calibration and the 776-byte calibration blob,
  per-zone detection thresholds, the motion indicator. `read_reg()` /
  `write_reg()` / `dci_read()` / `dci_write()` reach anything else.

## CX vs CH

Two silicon/firmware variants share the same board firmware (`APP_VL53L8`),
the same register protocol and the **same results-frame layout**:

- **VL53L8CX** — the base ranging sensor (this page). ST ULD 2.1.0 sensor
  firmware; ships as the dev default and carries no dedicated production USB
  PID.
- **VL53L8CH** — a superset that additionally emits **Compact Network
  Histograms (CNH)**, with the VL53LMZ 2.0.16 sensor firmware and its own
  production USB PID `0xED40`. See the
  [VL53L8CH docs](../vl53l8ch/introduction.md).

The live class tells them apart by `depz::Vl53l8Model` (`L8CX` / `L8CH`),
which fixes the sensor firmware `init()` downloads. `open_vl53l8()` and
`open_device()` pick `L8CH` for the production PID `0xED40` and `L8CX`
otherwise (safe: the CH is a superset); `Vl53l8::open(Link, model)` takes it by
hand. In the decode layer the only wire difference is the **footer-id
offset** (`FOOTER_ID_OFF_CX` = 12, `FOOTER_ID_OFF_CH` = 4), selected via
`depz::vl53l8::Variant` / `footer_id_off(Variant)`.

## When to use it

Use the VL53L8 when you need a **depth image** — gesture/zone occupancy, small
obstacle maps, people counting, hand tracking — in a package far smaller and
cheaper than a stereo camera, indoors or in the dark. For one precise distance
the [VL53L4CD](../vl53l4cd/introduction.md) or the
[SR04](../sr04/introduction.md) is simpler; for orientation use the
[BNO086](../bno086/introduction.md). Reach for **CH/CNH** only when you need
raw return histograms.

## Key concepts

- **Register bridge, ULD on the host** — every ULD call becomes SPI register
  reads and writes over USB; the sensor does nothing until the host has
  initialised it.
- **`init()` first, every power-up** — it boots the sensor's own MCU,
  downloads the ~84 KB sensor firmware, then uploads the NVM offset data, the
  default crosstalk data and the default configuration. ~0.8 s on a real
  board; pass a progress callback to follow it.
- **Ranging frequency ≥ 2 Hz** — below that the sensor never enters its
  ranging loop and streams nothing, so `set_ranging_frequency_hz(1)` throws
  `ArgumentError`. The maximum is 60 Hz at 4×4 and 15 Hz at 8×8.
- **The stream owns the register bank** — while ranging, every setter throws
  `ArgumentError` ("stop ranging first"). Stop, reconfigure, restart.
- **`Vl53l8Frame`** — one decoded frame. Every per-zone field is a
  `std::vector<…>` sized to the active resolution (16 or 64), row-major.
  Values are raw wire integers: `distance_mm` is in mm (the ST /4 floor
  scaling already applied); `range_sigma_mm_raw` is the raw u16 (real value =
  raw / 128). `target_status` 5/9 = valid, 255 = no target.

The live class has been checked on a real board (a VL53L8CH, board
`TMNQ8E3PRR`, which runs the same ranging path): model `L8CH` detected from
the USB PID, `init()` in 0.76 s, 4×4 at 30 Hz delivering 30 frames/s at
~596 mm with none dropped, sleep and wake, and 1 Hz refused.

## See also

- [VL53L8CX user guide](guide.md) — open and init, configuration, ranging,
  callbacks vs streams, the frame, power modes, crosstalk, detection
  thresholds, the motion indicator, raw registers, record / replay, and the
  decode layer.
- [VL53L8CH docs](../vl53l8ch/introduction.md) — the CNH superset.
- [API reference](api.md) — `Vl53l8`, `Vl53l8LiveFrame`, `open_vl53l8`,
  `Vl53l8Frame`, `FrameReassembler`, `decode_frame` and the advanced-feature
  codecs.
