# DEPZ Sensor SDK — Java

Contract-first Java 17 port of the DEPZ USB sensor transport + protocol
**foundation**. Byte-exact with `contracts/*.md` and the golden vectors in
`contracts/vectors/`, cross-checked against the Python reference SDK.

Scope: CRCs, packet framing + incremental parser, USB id table, identity
parsing, common command codecs, SR04 codecs, `.fwdepz` header parsing, the
VL53L4CD register-bridge codecs + host-ULD math, the shared VL53L8
results-frame decoder + advanced-DCI codecs, and the BNO086 SH-2 /
SHTP layer. This SDK is **decode-layer only** — pure, host-verifiable codecs
covered by golden vectors; live hardware bring-up (register bridges, ULD init)
is out of scope.

## Documentation

Full docs live under [`docs/`](docs/):

- [Common guide](docs/guide.md) — build, mental model, framing/CRCs, USB
  identity, common commands, time-sync, firmware container, datasets, testing.
- Per-sensor **introduction · guide · api**:
  [SR04](docs/sr04/introduction.md) · [VL53L4CD](docs/vl53l4cd/introduction.md) ·
  [VL53L8CX](docs/vl53l8cx/introduction.md) ·
  [VL53L8CH](docs/vl53l8ch/introduction.md) · [BNO086](docs/bno086/introduction.md)
- [Full API reference](docs/api.md) — generated from the Java sources by
  `scripts/gen_api_md.py`; regenerate with `python3 scripts/gen_api_md.py`.

## Sensors

The DEPZ family exposes **five** distinct sensor surfaces. The VL53L8 ToF is
*two* sensors that share a wire frame but ship as different parts:

| Sensor       | What it is                                                      | Coverage today |
| ------------ | -------------------------------------------------------------- | -------------- |
| **SR04**     | Ultrasonic range finder.                                       | Full codecs (encode + decode). |
| **VL53L4CD** | Single-zone ToF (~1.3 m) behind a thin I2C register bridge; production USB PID **0xED45**. | Full wire codecs + host-ULD math (result block, range timing, tuning words, init config block). |
| **VL53L8CX** | Base multizone ToF. Dev-default; enumerates on the ST dev USB id (VID 0x0483 / PID 0x56DC). | Shared VL53L8 frame decode + advanced-DCI codecs. |
| **VL53L8CH** | CX **plus** CNH (compact histograms) and its own production USB PID **0xED40**. | Shares the VL53L8 frame decode + advanced-DCI with CX. CNH histogram decode is a CH-specific **extension point** (not yet implemented). |
| **BNO086**   | 9-axis IMU (SH-2 over SHTP).                                   | SHTP framing/reassembly, SH-2 control encoders, input-report decode. |

`VL53L8CX` and `VL53L8CH` are byte-identical in the results-frame path this SDK
decodes, so `Vl53l8Uld.parseFrame` serves both — the only wire difference is the
footer-id offset (see `Vl53l8Uld.Variant`). Two things remain CH/hardware
extension points and are intentionally **not** implemented here: CNH histogram
decode (CH-only) and the live ULD init / register-bridge driver (both variants).

## Layout

- `src/main/java/ai/depz/sensor/transport/` — `Crc`, `CrcType`, `Framing`
  (`buildPacket`), `PacketParser` + `Event`/`Packet`/`Trash`/`CrcError`.
- `src/main/java/ai/depz/sensor/usb/` — `UsbIds` (`isKnownDepzUsb`,
  `usbModelHint`, serial ordering).
- `src/main/java/ai/depz/sensor/protocol/` — `Common`, `Identity`, `Sr04`,
  `Vl53l4`, `FwDepz`.
- `src/main/java/ai/depz/sensor/sensors/vl53l4/` — `Vl53l4Uld` (VL53L4CD
  result-block decode, range-timing math, tuning codecs, init config block).
- `src/main/java/ai/depz/sensor/sensors/vl53l8/` — `FrameReassembler` and
  `Vl53l8Uld` (shared VL53L8CX/CH frame decode + advanced-DCI codecs).
- `src/main/java/ai/depz/sensor/sensors/bno086/` — `Shtp`, `Sh2`, `Reports`.
- `src/test/java/ai/depz/sensor/test/` — hand-written `Json` parser and the
  `RunVectors` golden-vector harness (no JUnit).

## Install

Published on Maven Central as `io.github.depz-ai:depz-sensor-sdk` (0.1.4).

Gradle:

```kotlin
implementation("io.github.depz-ai:depz-sensor-sdk:0.1.4")
```

Maven:

```xml
<dependency>
  <groupId>io.github.depz-ai</groupId>
  <artifactId>depz-sensor-sdk</artifactId>
  <version>0.1.4</version>
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

MIT. Open source — source, byte-level protocol contracts and issue tracker:
<https://github.com/depz-ai/depz-sensor-sdk>.
