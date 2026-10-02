# depz-sensor-sdk-java — guide

The common guide to the Java SDK for the DEPZ USB sensor line. It covers what
the SDK is, how to build it, the shared mental model, and the cross-sensor
building blocks (framing, CRCs, USB identity, common commands, time-sync,
firmware container, datasets). Each sensor then has its own **introduction** and
**user guide**:

- **SR04** (HC-SR04 ultrasonic) — [introduction](sr04/introduction.md) ·
  [guide](sr04/guide.md) · [api](sr04/api.md)
- **VL53L8CX** (8×8 ToF base) — [introduction](vl53l8cx/introduction.md) ·
  [guide](vl53l8cx/guide.md) · [api](vl53l8cx/api.md)
- **VL53L8CH** (ToF superset + CNH histograms) —
  [introduction](vl53l8ch/introduction.md) · [guide](vl53l8ch/guide.md) ·
  [api](vl53l8ch/api.md)
- **VL53L4CD** (single-zone ToF) — [introduction](vl53l4cd/introduction.md) ·
  [guide](vl53l4cd/guide.md) · [api](vl53l4cd/api.md)
- **VL53L5CX / VL53L7CX** (8×8 ToF on the I2C board, 63° / 90°) —
  [VL53L5CX](vl53l5cx/introduction.md) ([guide](vl53l5cx/guide.md) ·
  [api](vl53l5cx/api.md)) · [VL53L7CX](vl53l7cx/introduction.md)
  ([guide](vl53l7cx/guide.md) · [api](vl53l7cx/api.md))
- **VL53L7CH** (VL53L7CX + CNH histograms) —
  [introduction](vl53l7ch/introduction.md) · [guide](vl53l7ch/guide.md) ·
  [api](vl53l7ch/api.md)
- **VL53L0X / VL53L1CX / VL53L1CB / VL53L3CX / VL53L4CX** (the 1D ToF family) —
  [VL53L0X](vl53l0x/introduction.md) · [VL53L1CX](vl53l1cx/introduction.md) ·
  [VL53L1CB](vl53l1cb/introduction.md) · [VL53L3CX](vl53l3cx/introduction.md) ·
  [VL53L4CX](vl53l4cx/introduction.md), each with its guide and api
- **BNO086** (9-axis IMU) — [introduction](bno086/introduction.md) ·
  [guide](bno086/guide.md) · [api](bno086/api.md)
- **BNO055** (9-axis IMU, on-chip fusion) — [introduction](bno055/introduction.md) ·
  [guide](bno055/guide.md) · [api](bno055/api.md)

For the exhaustive symbol-by-symbol reference see [api.md](api.md), generated
from the Java sources by `scripts/gen_api_md.py` so it never drifts from the
code.

## Contents

- [What it is](#what-it-is)
- [Scope: a decode layer](#scope-a-decode-layer)
- [Build](#build)
- [Getting started](#getting-started)
- [Mental model](#mental-model)
- [Framing and CRCs](#framing-and-crcs)
- [USB identity and discovery](#usb-identity-and-discovery)
- [Identity parsing](#identity-parsing)
- [Common commands and reports](#common-commands-and-reports)
- [Time-sync math](#time-sync-math)
- [Firmware container](#firmware-container)
- [Datasets](#datasets)
- [Testing with golden vectors](#testing-with-golden-vectors)
- [Extension points](#extension-points)

## What it is

Each DEPZ sensor is a USB CDC-ACM device speaking one shared framed protocol
(`A5 C3` header + CRC). This SDK is the **contract-first Java port of that
protocol foundation**: the pure, host-verifiable codecs that turn wire bytes
into typed Java records and back. It is byte-exact with `contracts/*.md` and the
golden vectors in `contracts/vectors/`, cross-checked against the Python and
TypeScript reference SDKs.

The sensors fall into three firmware philosophies:

- **SR04** — the device does the ranging; you decode `echoTimeUs` → distance
  (`ai.depz.sensor.protocol.Sr04`).
- **VL53L8CX / VL53L8CH** — the device is a thin SPI bridge; the host owns the
  ST ULD. This SDK ports the pure decode half — results-frame parsing and the
  advanced-DCI codecs (`ai.depz.sensor.sensors.vl53l8`). CX is the base imager;
  CH is its superset (Compact Network Histograms).
- **BNO086** — the device is an SHTP pass-through; the host owns the SH-2 stack.
  This SDK ports the SHTP framing/reassembly, the SH-2 control encoders and the
  input-report decoders (`ai.depz.sensor.sensors.bno086`).
- **VL53L4CD, VL53L5CX / L7CX / L7CH, the 1D family (VL53L0X / L1CX / L1CB /
  L3CX / L4CX) and BNO055** — the device is a thin I2C register bridge. This
  SDK ports the wire codecs and the stateless decode: `Vl53l4` / `Vl53l4Uld`,
  `Vl53l7` / `Vl53l7Uld` (frames through the VL53L8 decoder with the L5/L7
  geometry, CNH through `Vl53l8Uld.decodeCnh`), `Vl53lx` / `Vl53lxProducts` /
  `Vl53lxDecode`, `Bno055` / `Bno055Regs`.

## Scope: a decode layer

This SDK is **decode-layer only** — pure codecs covered by golden vectors. It
does **not** open serial ports, run reader threads or drive live hardware; that
live layer (register-bridge bring-up, ULD firmware download, the 1D family's
ST drivers, the BNO055 session logic) lives in the Python and TypeScript SDKs.
The live drivers are intentionally left as documented **extension points**:

- **Live ULD init / register-bridge driver** — `Vl53l8Uld.liveDriverStubbed()`
  (VL53L8CX / CH), `Vl53l7Uld.liveDriverStubbed()` (VL53L5CX / L7CX / L7CH),
  `Vl53l4Uld.liveDriverStubbed()` (VL53L4CD).
- **The 1D family's drivers and the BNO055 session logic** — the codecs, the
  product table and the block / register-window decode are here; initialising
  and streaming those boards is done with the Python or TypeScript SDK.

Everything else — CRCs, framing, the incremental parser, USB ids, identity,
common/SR04 codecs, the shared VL53L8 frame decode, the CNH histogram decode
(`Vl53l8Uld.decodeCnh`) and advanced-DCI codecs, the register-bridge codecs of
the VL53L4CD, L5/L7, 1D family and BNO055, and the full BNO086 SHTP/SH-2
layer — is complete and vector-tested.

## Build

Published on Maven Central as `io.github.depz-ai:depz-sensor-sdk` (0.4.0).

Gradle:

```kotlin
implementation("io.github.depz-ai:depz-sensor-sdk:0.4.0")
```

Maven:

```xml
<dependency>
  <groupId>io.github.depz-ai</groupId>
  <artifactId>depz-sensor-sdk</artifactId>
  <version>0.4.0</version>
</dependency>
```

Note the Maven group is `io.github.depz-ai`, while the Java package namespace you
import is `ai.depz.sensor.*` — they are deliberately different.

To build from source instead, the repo uses no build system — plain `javac`
(Java 17+; verified on JDK 21):

```sh
./build.sh          # compile main + test harness into ./out
./test.sh           # build, then run every golden vector
```

The package is a flat source tree under `src/main/java/ai/depz/sensor/`; add
`out/` (or the sources) to your classpath and import `ai.depz.sensor.*`.

## Getting started

Decode is the whole game: feed the bytes you read from a port into the
incremental parser, then hand typed payloads to the per-sensor codecs.

```java
import ai.depz.sensor.transport.*;
import ai.depz.sensor.protocol.Sr04;

PacketParser parser = new PacketParser();
for (Event ev : parser.feed(bytesFromPort)) {          // arbitrary chunking is fine
    if (ev instanceof Packet p && p.cmd() == Sr04.Sr04Rpt.DATA.value) {
        Sr04.Sr04Data d = Sr04.Sr04Data.unpack(p.payload());
        Double mm = Sr04.distanceMmFromEcho(d.echoTimeUs(), null);
        System.out.println(mm == null ? "no echo" : mm + " mm");
    }
}
```

To send, build a framed packet with `Framing.buildPacket`:

```java
byte[] frame = Framing.buildPacket(Sr04.Sr04Cmd.START_MEASUREMENT_LOOP.value);
// write `frame` to your transport of choice
```

From there, follow the per-sensor guide linked above.

## Mental model

```
raw bytes ──► PacketParser.feed() ──► List<Event>
                                        │
                    ┌───────────────────┼───────────────────┐
                 Packet(cmd,          Trash(bytes)        CrcError(cmd,seq)
                  seq, payload)       (resync noise)      (bad payload CRC)
                     │
        dispatch by cmd/report id
                     │
   ┌─────────────────┼──────────────────┬──────────────────────┐
 Common/           Sr04.*            Vl53l8Uld /            Shtp / Sh2 /
 Identity          codecs            FrameReassembler       Reports (BNO086)
 codecs                              (VL53L8)
```

- **`PacketParser` is incremental.** Feed it whatever bytes you have; event
  order is invariant to chunking (contract 01 §5). It counts `packets`,
  `crcErrors`, `headerErrors` and `trashBytes`.
- **Correlation is by the echoed command byte**, not a sequence number. A reply
  is a `Packet` whose report id / echoed `cmd` matches your request.
- **Codecs are static and pure.** `unpack`/`parse` decode a payload into a
  record; `pack`/`build` produce bytes. Nothing holds a connection.
- **Unsigned wire fields widen into signed Java types** (`int` for u8/u16,
  `long` for u32/u64) and are masked on read/write.

## Framing and CRCs

`Framing` (contract 01) frames and the two `Framing.buildPacket` overloads
produce the `A5 C3` + header + payload + optional CRC trailer. `CrcType` selects
the payload CRC (`NONE`/`CRC8`/`CRC16`/`CRC32`); the `Crc` class holds the four
algorithms (`crc8Maxim`, `crc16Modbus`, `crc32IsoHdlc`, plus `crc16CcittFalse`
for the `.fwdepz` header only).

```java
byte[] tx = Framing.buildPacket(cmd, payload, seq, CrcType.CRC8);
```

Two firmware errata are baked in and vector-checked: CRC-8/MAXIM init is `0x00`
for every device (E1), and an **empty payload never carries CRC bytes** even when
the header advertises a CRC type (E6) — the parser resyncs one byte past a
corrupt header.

## USB identity and discovery

`UsbIds` is the identity table used to pick the right port without poking
unrelated devices; the protocol probe (`GET_NAME_ACTIVE_SOFTWARE`) stays the
source of truth for what a device actually is.

```java
boolean mine = UsbIds.isKnownDepzUsb(vid, pid);     // recognized DEPZ / dev id?
String hint  = UsbIds.usbModelHint(vid, pid);       // "sr04" | "vl53l8ch" | "bno086" | "dev" | null

List<UsbIds.PortRef> ordered = UsbIds.orderBySerial(ports);   // by USB iSerial, empties last
```

Two identical sensors are told apart by their **USB iSerial**: `orderBySerial`
sorts deterministically (null/empty serials last, tie-broken by port path), so
"device 0 / device 1" is stable.

## Identity parsing

`Identity.parseSoftwareName` classifies a `GET_NAME_ACTIVE_SOFTWARE` string
(strip it first with `Common.stripDeviceString`):

```java
String name = Common.stripDeviceString(payloadBytes);
Identity id = Identity.parseSoftwareName(name);
id.mode();         // "app" | "bootloader" | "unknown"
id.sensorType();   // SensorType.SR04 / VL53L8 / BNO086 / UNKNOWN (null in bootloader)
id.version();      // "1.2.3" or "" when absent
```

## Common commands and reports

`Common` holds the shared opcode/report enums (`Cmd`, `Rpt`, `Status`) and the
report codecs used by every sensor: `StatusReport`, `TextReport`,
`SyncTimeReport`, `TemperatureReport`, `SequenceErrorReport`, plus the
`SyncPinConfig` codec for the AUX sync pins.

```java
Common.StatusReport st = Common.StatusReport.unpack(p.payload());
if (st.status() != Common.Status.OK.value) { /* handle error status */ }

Common.TemperatureReport t = Common.TemperatureReport.unpack(p.payload());
double celsius = t.celsius();          // rawDecidegrees / 10.0
```

The echoed-cmd byte is `Common.UNSOLICITED` (`0x00`) for unsolicited reports.

## Time-sync math

The device timestamps are MCU microseconds. `Common.packSyncTime(t1)` builds the
`SYNC_TIME` payload; `Common.syncTimeOffsetRtt(t1, t2, t3, t4)` runs the NTP-style
clock math and returns `{offsetUs, rttUs}` (offset = device − host):

```java
long[] r = Common.syncTimeOffsetRtt(t1, t2, t3, t4);
long offsetUs = r[0], rttUs = r[1];    // keep the lowest-RTT sample across N rounds
```

Integer division truncates toward zero (Java `long`), matching the shared rule
across all SDKs — so offsets are byte-identical to the Python/TS references.

## Firmware container

`FwDepz.FwDepzImage.parse` validates a `.fwdepz` application-firmware container
(length, magic, header CRC-16/CCITT-FALSE, then `fwSize == payload length`) and
exposes the fields plus a payload CRC-32 check:

```java
FwDepz.FwDepzImage img = FwDepz.FwDepzImage.parse(blob);   // throws FwDepzError on any mismatch
boolean ok = img.payloadCrcOk();                            // crc32IsoHdlc(payload) == fwCrc32
```

`FwDepzError.code` matches the vector `error` strings (`too_short`, `magic`,
`header_crc`, `size`).

## Datasets

`Dataset.parse` reads a `.depzdata` JSONL recording (contract 09) — a header
line plus record lines — and stably merges the records onto one host timeline by
`t` (µs). JSON parsing is delegated to a caller-supplied line parser so the
module has no dependency on any JSON library:

```java
Dataset ds = Dataset.parse(content, line -> myJson.parse(line));
for (Dataset.Record rec : ds.records()) {
    System.out.println(rec.deviceId() + " " + rec.kind() + " @ " + rec.tHostUs());
}
```

Records from every device — including two of the same model — land on one
timeline, merged strictly by host time.

## Testing with golden vectors

The `RunVectors` harness (no JUnit) replays every case in `contracts/vectors/`
through these codecs and prints `TOTAL: N/N vector cases passed`. That is how
the whole SDK is verified without hardware — every codec here is pinned to the
same vectors as the Python and TypeScript ports.

## Extension points

The live drivers are out of the decode scope and stubbed truthfully (see
[Scope](#scope-a-decode-layer)): the **live ULD init / register-bridge driver**
of the ToF boards (single, documented `liveDriverStubbed()` methods that
return `true` today), the 1D family's ST drivers and the BNO055 session logic.
The surrounding decode surface they would plug into is already complete and
vector-tested.
