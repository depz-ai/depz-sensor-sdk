# depz-sensor-sdk-java — guide

The common guide to the Java SDK for the DEPZ USB sensor line. It covers what
the SDK is, how to build it, the shared mental model, and the cross-sensor
building blocks (framing, CRCs, USB identity, common commands, time-sync,
firmware container, datasets). Each sensor then has its own **introduction** and
**user guide**:

- **SR04** (HC-SR04 ultrasonic) — [introduction](sr04/introduction.md) ·
  [guide](sr04/guide.md) · [api](sr04/api.md)
- **VL53L0X / VL53L1CX / VL53L1CB / VL53L3CX / VL53L4CX** (the 1D ToF family) —
  [VL53L0X](vl53l0x/introduction.md) · [VL53L1CX](vl53l1cx/introduction.md) ·
  [VL53L1CB](vl53l1cb/introduction.md) · [VL53L3CX](vl53l3cx/introduction.md) ·
  [VL53L4CX](vl53l4cx/introduction.md), each with its guide and api
- **VL53L4CD** (single-zone ToF) — [introduction](vl53l4cd/introduction.md) ·
  [guide](vl53l4cd/guide.md) · [api](vl53l4cd/api.md)
- **VL53L5CX / VL53L7CX** (8×8 ToF on the I2C board, 63° / 90°) —
  [VL53L5CX](vl53l5cx/introduction.md) ([guide](vl53l5cx/guide.md) ·
  [api](vl53l5cx/api.md)) · [VL53L7CX](vl53l7cx/introduction.md)
  ([guide](vl53l7cx/guide.md) · [api](vl53l7cx/api.md))
- **VL53L7CH** (VL53L7CX + CNH histograms) —
  [introduction](vl53l7ch/introduction.md) · [guide](vl53l7ch/guide.md) ·
  [api](vl53l7ch/api.md)
- **VL53L8CX** (8×8 ToF base) — [introduction](vl53l8cx/introduction.md) ·
  [guide](vl53l8cx/guide.md) · [api](vl53l8cx/api.md)
- **VL53L8CH** (ToF superset + CNH histograms) —
  [introduction](vl53l8ch/introduction.md) · [guide](vl53l8ch/guide.md) ·
  [api](vl53l8ch/api.md)
- **TMF8829** (dToF, 8×8 … 48×32 zones, up to 4 peaks) —
  [introduction](tmf8829/introduction.md) · [guide](tmf8829/guide.md) ·
  [api](tmf8829/api.md)
- **BNO055** (9-axis IMU, on-chip fusion) — [introduction](bno055/introduction.md) ·
  [guide](bno055/guide.md) · [api](bno055/api.md)
- **BNO085** (9-axis IMU) — [introduction](bno085/introduction.md) ·
  [guide](bno085/guide.md) · [api](bno085/api.md)
- **BNO086** (9-axis IMU) — [introduction](bno086/introduction.md) ·
  [guide](bno086/guide.md) · [api](bno086/api.md)

For the exhaustive symbol-by-symbol reference see [api.md](api.md), generated
from the Java sources by `scripts/gen_api_md.py` so it never drifts from the
code.

## Contents

- [What it is](#what-it-is)
- [Live devices](#live-devices)
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

The sensors fall into a few firmware philosophies:

- **SR04** — the device does the ranging; you decode `echoTimeUs` → distance
  (`ai.depz.sensor.protocol.Sr04`).
- **The 1D family (VL53L0X / L1CX / L1CB / L3CX / L4CX), VL53L4CD, VL53L5CX /
  L7CX / L7CH and BNO055** — the device is a thin I2C register bridge. This
  SDK ports the wire codecs and the stateless decode: `Vl53lx` /
  `Vl53lxProducts` / `Vl53lxDecode`, `Vl53l4` / `Vl53l4Uld`, `Vl53l7` /
  `Vl53l7Uld` (frames through the VL53L8 decoder with the L5/L7 geometry, CNH
  through `Vl53l8Uld.decodeCnh`), `Bno055` / `Bno055Regs`.
- **VL53L8CX / VL53L8CH** — the device is a thin SPI bridge; the host owns the
  ST ULD. This SDK ports the pure decode half — results-frame parsing and the
  advanced-DCI codecs (`ai.depz.sensor.sensors.vl53l8`). CX is the base imager;
  CH is its superset (Compact Network Histograms).
- **TMF8829** — the device is a thin SPI register bridge; the host runs the
  sensor (firmware download, presets, start/stop). This SDK ports the wire
  codecs, the frame reassembly and decode, the config-page fields, the clock
  model / capture time and the embedded sensor firmware
  (`ai.depz.sensor.protocol.Tmf8829`, `ai.depz.sensor.sensors.tmf8829`).
- **BNO085 / BNO086** — the device is an SHTP pass-through; the host owns the
  SH-2 stack. This SDK ports the SHTP framing/reassembly, the SH-2 control
  encoders and the input-report decoders (`ai.depz.sensor.sensors.bno086`).

## Live devices

On top of the codecs sits the **device layer** (contract 07,
`ai.depz.sensor.devices`): a connection with its reader thread, requests and
replies, streams, events, time sync, `.depzrec` record / replay and port
discovery. Serial ports go through [jSerialComm](https://fazecast.github.io/jSerialComm/)
(Windows, macOS, Linux) — the one dependency, pulled in by Maven / Gradle, or
fetched and checksum-checked by `./build.sh`. The sensor classes are ported one
class per sensor family — the **SR04**, the **VL53L4CD**, the **VL53L8 / L5 / L7**
boards, the **VL53L 1D family**, the **TMF8829**, the **BNO055** and the
**BNO085 / BNO086**; a board this SDK has no class for opens as a plain
`DepzDevice` (the common commands).

```java
import ai.depz.sensor.devices.*;
import java.time.Duration;

try (Sr04Device dev = DepzDevices.openSr04()) {           // smallest USB serial; or openSr04("SN…", null)
    System.out.println(dev.getSoftwareName());             // APP_usonic_SR04_v0.97
    Sr04Measurement once = dev.measureOnce();

    try (Subscription<Sr04Measurement> loop = dev.measurements()) {   // subscribe before start(): nothing is missed
        dev.start();
        for (int i = 0; i < 20; i++) {
            System.out.println(loop.take(Duration.ofSeconds(1)).distanceMm());
        }
        dev.stop();
    }
}
```

- **Requests** block the calling thread until the reply, at most
  `dev.timeout()` (200 ms by default). A board error throws `StatusException`
  (`BusyException` for ERR_BUSY), no reply `DepzTimeoutException`, a lost
  connection `DeviceLostException` — all unchecked, all `DepzException`; an
  argument outside its wire field throws `IllegalArgumentException` before
  anything is sent.
- **Streams** come two ways: `on…(callback)` runs on the reader thread (keep it
  short, never block; close the returned handle to stop), and `measurements()`
  (`events()`, …) returns a `Subscription<T>` — a bounded queue that drops the
  oldest item when full and counts it in `droppedCount()`. `take(timeout)`
  waits, `tryTake()` and `drain()` never block.
- **Events** (`onEvent`, `events()`): unsolicited status (hardware fault),
  sequence gaps, CRC errors, text, temperature, disconnect — the nested records
  of `DeviceEvent`.
- **Time**: `syncTime()` measures device clock − host clock;
  `toHostTimeUs(deviceUs)` then puts device timestamps on the host clock
  (`DepzDevice.hostNowUs()`).
- **Record / replay**: wrap any link in `RecordingLink` to write a `.depzrec`
  capture (contract 08); `ReplayLink` plays one back, and
  `DepzDevices.openOnLink(link)` opens whatever the capture holds. The SDK's
  tests run the shared device scenarios (contract 15) this way.

## Build

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

Note the Maven group is `io.github.depz-ai`, while the Java package namespace you
import is `ai.depz.sensor.*` — they are deliberately different.

To build from source instead, the repo uses no build system — plain `javac`
(Java 17+; verified on JDK 21):

```sh
./build.sh          # fetch jSerialComm into ./lib (once), compile main + test harness into ./out
./test.sh           # build, then run every golden vector and the device tests
```

The package is a flat source tree under `src/main/java/ai/depz/sensor/`; add
`out/` and `lib/jSerialComm-*.jar` to your classpath and import `ai.depz.sensor.*`.

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

A `.depzdata` file (contract 09) holds **decoded** results of one or more
devices on one host timeline: JSON Lines, plain or gzip (`.depzdata.gz`).

**Record.** `SessionRecorder` hooks the result stream of every device class and
writes one record per measurement, frame or report, stamped with the device
time mapped onto the host clock (`t = device µs − offset`; `add` runs
`syncTime` for that offset). It never configures a device — start and stop
streaming yourself:

```java
import ai.depz.sensor.dataset.*;
import ai.depz.sensor.devices.*;
import java.nio.file.Path;

try (Sr04Device sr04 = DepzDevices.openSr04(); Vl53l8cxDevice tof = DepzDevices.openVl53l8()) {
    tof.init();
    try (SessionRecorder rec = new SessionRecorder(Path.of("run.depzdata"), "desk test", false, 1.0)) {
        rec.add(sr04);                  // d0
        rec.add(tof);                   // d1
        rec.start();
        sr04.start();
        tof.startRanging();
        Thread.sleep(10_000);
        sr04.stop();
        tof.stopRanging();
    }                                   // close: unhook, close the file
}
```

Every class has its kind — `sr04`, `vl53l4`, `vl53l8` (+ `vl53l8_cnh` for an
armed CH), `vl53lx`, `bno055`, `bno086`, `tmf8829` — plus about one
`temperature` record per second and device (`temperatureHz`; the MCU, and the
VL53L8 die). `DatasetRecords` builds each record value (BNO086 values scaled
with the Q points of `Reports.Q_POINTS`), for a writer of your own over
`DatasetWriter`.

**Read.** `Dataset.read(path)` (plain or gzip) — or `Dataset.parse(content,
jsonLine)` with a line parser of your own — merges the records of every
device, two of the same model included, stably by host time:

```java
Dataset ds = Dataset.read(Path.of("run.depzdata"));
for (Dataset.Record rec : ds.records()) {   // merged by ascending host time
    System.out.println(rec.deviceId() + " " + rec.kind() + " @ " + rec.tHostUs());
}
System.out.println(ds.devices());           // per-device header metadata
System.out.println(ds.durationUs() / 1e6 + " s");
long echoUs = (Long) ds.records().get(0).value().get("echo_us");   // an "sr04" record
```

Values are plain objects — `Long`, `Double`, `String`, `Boolean`, `null`,
lists and nested maps — as `ai.depz.sensor.json.JsonText.parse` returns them.

**Replay.** `ds.play(callback, speed)` delivers the records paced by their
timestamps (2 = twice as fast, 0 = at once) and blocks. For a viewer timeline,
`DatasetPlayer` adds play / pause / seek / speed (contract 09 §Player):

```java
try (DatasetPlayer player = new DatasetPlayer(ds, 2.0)) {
    player.onRecord(r -> System.out.println(r.kind() + " " + r.tHostUs()));   // player thread; don't block
    player.onState(st -> System.out.println(st));
    player.play();
    player.seekUs(5_000_000);   // the next record is the first at >= 5 s from the start
    player.await();             // until done (or paused)
}
```

With `threaded` false (`new DatasetPlayer(ds, speed, null, false)`) no thread
starts: call `tick()` from your own loop; it returns the seconds until the next
record is due.

The raw-byte capture layer (`.depzrec`, contract 08) is a different thing: one
device's link, byte for byte, which `ReplayLink` plays back through the full
device stack.

## Testing with golden vectors

The `RunVectors` harness (no JUnit) replays every case in `contracts/vectors/`
through these codecs and prints `TOTAL: N/N vector cases passed`; `RunDeviceTests`
then runs the device layer over a fake board and every contract 15 scenario of
a ported class against its real-hardware capture (replayed strict: each byte
the SDK writes must equal the recorded one). That is how the whole SDK is
verified without hardware — every codec here is pinned to the same vectors as
the Python and TypeScript ports.

## Extension points

Every sensor of the line has its live class. A board whose sensor class this
SDK does not know (a newer firmware) opens as a plain `DepzDevice`: the common
commands work, and the decode surface — complete and vector-tested — is there
to build a driver on, the way the classes here are built.
