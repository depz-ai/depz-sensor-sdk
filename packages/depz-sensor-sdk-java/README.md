# DEPZ Sensor SDK — Java

Contract-first Java 17 port of the DEPZ USB sensor transport + protocol
**foundation**. Byte-exact with `contracts/*.md` and the golden vectors in
`contracts/vectors/`, cross-checked against the Python reference SDK.

Scope: CRCs, packet framing + incremental parser, USB id table, identity
parsing, common command codecs, SR04 codecs, `.fwdepz` header parsing, the
VL53L0X / L1CX / L1CB / L3CX / L4CX codecs + product table + block decode, the
VL53L4CD register-bridge codecs + host-ULD math, the VL53L5CX / L7CX / L7CH
board codecs and frame decode, the shared VL53L8 results-frame decoder +
advanced-DCI codecs + CNH histogram decode, the TMF8829 register-bridge codecs +
frame / measurement decode + capture time, the BNO055 register-bridge and
register codecs, and the BNO086 SH-2 / SHTP layer — pure, host-verifiable codecs
covered by golden vectors. On top of them, the **live device layer** (contract
07, `ai.depz.sensor.devices`: reader thread, requests, streams, events, time
sync, `.depzrec` record / replay, port discovery over jSerialComm) with the
SR04, VL53L4CD (the ST ULD 2.2.3 on the host), VL53L8CX / CH, VL53L5CX,
VL53L7CX / CH (the ST ULD with its firmware download on the host), the VL53L
1D family (VL53L0X / L1CX / L1CB / L3CX / L4CX: ST's ULD, ULP and histogram
drivers on the host), TMF8829 (firmware download and the sensor sequence),
BNO055 and BNO085 / BNO086 (the SH-2 host stack) classes.

## Documentation

Full docs live under [`docs/`](docs/):

- [Common guide](docs/guide.md) — build, mental model, framing/CRCs, USB
  identity, common commands, time-sync, firmware container, datasets, testing.
- Per-sensor **introduction · guide · api**:
  [SR04](docs/sr04/introduction.md) ·
  [VL53L0X](docs/vl53l0x/introduction.md) · [VL53L1CX](docs/vl53l1cx/introduction.md) ·
  [VL53L1CB](docs/vl53l1cb/introduction.md) · [VL53L3CX](docs/vl53l3cx/introduction.md) ·
  [VL53L4CD](docs/vl53l4cd/introduction.md) · [VL53L4CX](docs/vl53l4cx/introduction.md) ·
  [VL53L5CX](docs/vl53l5cx/introduction.md) · [VL53L7CX](docs/vl53l7cx/introduction.md) ·
  [VL53L7CH](docs/vl53l7ch/introduction.md) · [VL53L8CX](docs/vl53l8cx/introduction.md) ·
  [VL53L8CH](docs/vl53l8ch/introduction.md) · [TMF8829](docs/tmf8829/introduction.md) ·
  [BNO055](docs/bno055/introduction.md) ·
  [BNO085](docs/bno085/introduction.md) · [BNO086](docs/bno086/introduction.md)
- [Full API reference](docs/api.md) — generated from the Java sources by
  `scripts/gen_api_md.py`; regenerate with `python3 scripts/gen_api_md.py`.

## Sensors

The DEPZ family's sensor surfaces. The VL53L8 ToF is *two* sensors that share
a wire frame but ship as different parts:

| Sensor       | What it is                                                      | Coverage today |
| ------------ | -------------------------------------------------------------- | -------------- |
| **SR04**     | Ultrasonic range finder.                                       | Full codecs (encode + decode). |
| **VL53L0X / L1CX / L1CB / L3CX / L4CX** | The 1D ToF family on one I2C register-bridge firmware (protocol v2.00). | Live devices `Vl53lxDevice` (+ one class per product) running ST's ULD / ULP / histogram drivers; v2.00 wire codecs, the product table + class resolution, die-block / VL53L0X raw / histogram-block decode. |
| **VL53L4CD** | Single-zone ToF (~1.3 m) behind a thin I2C register bridge; production USB PID **0xED45**. | Full wire codecs + host-ULD math (result block, range timing, tuning words, init config block). |
| **VL53L5CX / VL53L7CX / VL53L7CH** | 8×8 multizone ToF on one I2C board firmware (63° / 90° / 90° + CNH); PIDs **0xED48 / 0xED49 / 0xED4A**. | Live devices `Vl53l5cxDevice` / `Vl53l7cxDevice` / `Vl53l7chDevice` (the L5/L7 ULD on the host); board codecs (pin control, bus speed, info), class resolution, frame decode through the VL53L8 decoder with the L5/L7 geometry, CNH decode on the L7CH. |
| **VL53L8CX** | Base multizone ToF. Dev-default; enumerates on the ST dev USB id (VID 0x0483 / PID 0x56DC). | Live device `Vl53l8cxDevice` (the ST ULD on the host); shared VL53L8 frame decode + advanced-DCI codecs. |
| **VL53L8CH** | CX **plus** CNH (compact histograms) and its own production USB PID **0xED40**. | Live device `Vl53l8chDevice` (CNH armed with `Vl53l8CnhSetup`); shares the VL53L8 frame decode + advanced-DCI with CX, plus the CNH histogram decode (`Vl53l8Uld.decodeCnh`). |
| **TMF8829**  | ams-OSRAM multizone dToF (8×8 … 48×32 zones, up to 4 peaks, histograms) behind a thin SPI register bridge; PID **0xED4C**. | Live device `Tmf8829Device` (firmware download, config page, the sensor sequence); wire codecs, chunk reassembly, frame / pixel decode + half-frame merge, config-page fields, clock model + capture time, measurement builder, embedded sensor firmware (MIT). |
| **BNO055**   | 9-axis IMU with on-chip fusion behind an I2C register bridge; PID **0xEE0A**. | Live device `Bno055Device` (session logic, scaled samples); wire codecs, units / calibration / axis-remap / page-1 codecs, register-window decode. |
| **BNO085 / BNO086** | 9-axis IMU (SH-2 over SHTP).                                   | Live device `Bno086Device` (the SH-2 host stack); SHTP framing/reassembly, SH-2 control encoders and response decoders, input-report decode + Q-point scaling. |

`VL53L8CX` and `VL53L8CH` are byte-identical in the results-frame path this SDK
decodes, so `Vl53l8Uld.parseFrame` serves both — the only wire difference is the
footer-id offset (see `Vl53l8Uld.Variant`). The live ULD init /
register-bridge driver (both variants) is an extension point and intentionally
**not** implemented here.

## Layout

- `src/main/java/ai/depz/sensor/transport/` — `Crc`, `CrcType`, `Framing`
  (`buildPacket`), `PacketParser` + `Event`/`Packet`/`Trash`/`CrcError`.
- `src/main/java/ai/depz/sensor/usb/` — `UsbIds` (`isKnownDepzUsb`,
  `usbModelHint`, serial ordering).
- `src/main/java/ai/depz/sensor/protocol/` — `Common`, `Identity`, `Sr04`,
  `Vl53l4`, `Vl53l7`, `Vl53lx`, `Bno055`, `Tmf8829`, `FwDepz`.
- `src/main/java/ai/depz/sensor/sensors/vl53l4/` — `Vl53l4Uld` (VL53L4CD
  result-block decode, range-timing math, tuning codecs, init config block).
- `src/main/java/ai/depz/sensor/sensors/vl53l8/` — `FrameReassembler` and
  `Vl53l8Uld` (shared VL53L8CX/CH frame decode + advanced-DCI codecs).
- `src/main/java/ai/depz/sensor/sensors/vl53l7/` — `Vl53l7Uld` (L5/L7 class
  resolution and frame decode).
- `src/main/java/ai/depz/sensor/sensors/vl53lx/` — `Vl53lxProducts` (product
  table, class resolution) and `Vl53lxDecode` (die / VL53L0X / histogram
  blocks).
- `src/main/java/ai/depz/sensor/sensors/tmf8829/` — `Frames`, `Tmf8829Config`,
  `SensorClock`, `CaptureTimer`, `MeasurementBuilder`, `Tmf8829Firmware`
  (frame decode, config fields, capture time, the embedded sensor firmware).
- `src/main/java/ai/depz/sensor/sensors/bno055/` — `Bno055Regs` (register map,
  units, calibration, axis remap, page-1 configs, window decode).
- `src/main/java/ai/depz/sensor/sensors/bno086/` — `Shtp`, `Sh2`, `Reports`.
- `src/test/java/ai/depz/sensor/test/` — hand-written `Json` parser and the
  `RunVectors` golden-vector harness (no JUnit).

## Install

Published on Maven Central as `io.github.depz-ai:depz-sensor-sdk` (0.7.0).

Gradle:

```kotlin
implementation("io.github.depz-ai:depz-sensor-sdk:0.7.0")
```

Maven:

```xml
<dependency>
  <groupId>io.github.depz-ai</groupId>
  <artifactId>depz-sensor-sdk</artifactId>
  <version>0.7.0</version>
</dependency>
```

The Maven group is `io.github.depz-ai`; the Java package namespace you import is
`ai.depz.sensor.*` (they are deliberately different).

## Build & test

The repo itself uses no build system — plain `javac` (Java 17+; verified on
JDK 21):

```sh
./build.sh          # compile main + test into ./out
./test.sh           # build, then run all vectors (nonzero exit on failure)
./test.sh /path/to/contracts/vectors   # explicit vectors dir
```

The harness prints `TOTAL: N/N vector cases passed` and `ALL VECTORS PASSED`.

## Notes on parity

- CRC-8/MAXIM init is `0x00` for every device (ERRATA E1).
- Empty payloads never carry CRC trailer bytes even when the header advertises a
  CRC type (ERRATA E6); the parser mirrors this and resyncs one byte past a
  corrupt header.
- `syncTimeOffsetRtt` relies on Java `long` division truncating toward zero,
  matching the shared integer rule across SDKs.
- Unsigned wire fields are widened into signed Java types (`int` for u8/u16,
  `long` for u32/u64) and masked on read/write.

## License

MIT. Open source — source and issue tracker:
<https://github.com/depz-ai/depz-sensor-sdk>.
