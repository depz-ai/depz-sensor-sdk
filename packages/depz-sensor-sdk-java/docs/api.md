# API reference

Auto-generated from the public Java source under
`src/main/java/ai/depz/sensor/` by `scripts/gen_api_md.py` — run
`python3 scripts/gen_api_md.py` to regenerate. Edit the Javadoc in the
source, not this file.

This SDK is decode-layer only: pure, host-verifiable codecs. The live
drivers (ULD init / register-bridge configuration, the 1D family's ST
drivers, the BNO055 session logic) are extension points, surfaced here
as documented stubs.

Each sensor also has a focused reference with just its own symbols:
[SR04](sr04/api.md) · [VL53L4CD](vl53l4cd/api.md) · [VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · [VL53L5CX](vl53l5cx/api.md) · [VL53L7CX](vl53l7cx/api.md) · [VL53L7CH](vl53l7ch/api.md) · [VL53L0X](vl53l0x/api.md) · [VL53L1CX](vl53l1cx/api.md) · [VL53L1CB](vl53l1cb/api.md) · [VL53L3CX](vl53l3cx/api.md) · [VL53L4CX](vl53l4cx/api.md) · [BNO086](bno086/api.md) · [BNO055](bno055/api.md).

## Contents

- **Transport**: [`Crc`](#crc), [`CrcError`](#crcerror), [`CrcType`](#crctype), [`Event`](#event), [`Framing`](#framing), [`Packet`](#packet), [`PacketParser`](#packetparser), [`Trash`](#trash)
- **USB identity**: [`UsbIds`](#usbids), [`UsbIds.PortRef`](#usbidsportref)
- **Protocol codecs (common)**: [`Common`](#common), [`Common.Cmd`](#commoncmd), [`Common.Rpt`](#commonrpt), [`Common.Status`](#commonstatus), [`Common.SyncPinMode`](#commonsyncpinmode), [`Common.SyncPinPolarity`](#commonsyncpinpolarity), [`Common.StatusReport`](#commonstatusreport), [`Common.TextReport`](#commontextreport), [`Common.SyncTimeReport`](#commonsynctimereport), [`Common.TemperatureReport`](#commontemperaturereport), [`Common.SequenceErrorReport`](#commonsequenceerrorreport), [`Common.SyncPinConfig`](#commonsyncpinconfig), [`Identity`](#identity), [`Identity.SensorType`](#identitysensortype)
- **SR04**: [`Sr04`](#sr04), [`Sr04.Sr04Cmd`](#sr04sr04cmd), [`Sr04.Sr04Rpt`](#sr04sr04rpt), [`Sr04.Sr04Data`](#sr04sr04data)
- **Firmware container**: [`FwDepz`](#fwdepz), [`FwDepz.FwDepzError`](#fwdepzfwdepzerror), [`FwDepz.FwDepzImage`](#fwdepzfwdepzimage)
- **VL53L4CD (ToF)**: [`Vl53l4`](#vl53l4), [`Vl53l4.Vl53l4Cmd`](#vl53l4vl53l4cmd), [`Vl53l4.Vl53l4Rpt`](#vl53l4vl53l4rpt), [`Vl53l4.RegData`](#vl53l4regdata), [`Vl53l4.Vl53l4Info`](#vl53l4vl53l4info), [`Vl53l4.StreamData`](#vl53l4streamdata), [`Vl53l4Uld`](#vl53l4uld), [`Vl53l4Uld.Vl53l4Error`](#vl53l4uldvl53l4error), [`Vl53l4Uld.Results`](#vl53l4uldresults)
- **VL53L8 (ToF)**: [`FrameReassembler`](#framereassembler), [`FrameReassembler.FrameChunk`](#framereassemblerframechunk), [`FrameReassembler.CompletedFrame`](#framereassemblercompletedframe), [`Vl53l8Uld`](#vl53l8uld), [`Vl53l8Uld.Variant`](#vl53l8uldvariant), [`Vl53l8Uld.CnhAggregate`](#vl53l8uldcnhaggregate), [`Vl53l8Uld.CnhResult`](#vl53l8uldcnhresult), [`Vl53l8Uld.Results`](#vl53l8uldresults), [`Vl53l8Uld.DetectionThreshold`](#vl53l8ulddetectionthreshold), [`Vl53l8Uld.PackedThresholds`](#vl53l8uldpackedthresholds), [`Vl53l8Uld.MotionConfig`](#vl53l8uldmotionconfig), [`Vl53l8Uld.Vl53l8Error`](#vl53l8uldvl53l8error)
- **VL53L5CX / VL53L7CX / VL53L7CH (ToF)**: [`Vl53l7`](#vl53l7), [`Vl53l7.Vl53l7Cmd`](#vl53l7vl53l7cmd), [`Vl53l7.Vl53l7Rpt`](#vl53l7vl53l7rpt), [`Vl53l7.PinAction`](#vl53l7pinaction), [`Vl53l7.Vl53l7Info`](#vl53l7vl53l7info), [`Vl53l7Uld`](#vl53l7uld), [`Vl53l7Uld.Model`](#vl53l7uldmodel)
- **VL53L0X / L1CX / L1CB / L3CX / L4CX (ToF)**: [`Vl53lx`](#vl53lx), [`Vl53lx.Vl53lxCmd`](#vl53lxvl53lxcmd), [`Vl53lx.Vl53lxRpt`](#vl53lxvl53lxrpt), [`Vl53lx.Vl53lxInfo`](#vl53lxvl53lxinfo), [`Vl53lxDecode`](#vl53lxdecode), [`Vl53lxDecode.DieVariant`](#vl53lxdecodedievariant), [`Vl53lxDecode.L0xRaw`](#vl53lxdecodel0xraw), [`Vl53lxDecode.HistogramRaw`](#vl53lxdecodehistogramraw), [`Vl53lxProducts`](#vl53lxproducts), [`Vl53lxProducts.Bridge`](#vl53lxproductsbridge), [`Vl53lxProducts.Product`](#vl53lxproductsproduct), [`Vl53lxProducts.SensorClass`](#vl53lxproductssensorclass)
- **BNO086 (IMU)**: [`Reports`](#reports), [`Reports.Report`](#reportsreport), [`Sh2`](#sh2), [`Shtp`](#shtp), [`Shtp.ShtpHeader`](#shtpshtpheader), [`Shtp.ShtpCargo`](#shtpshtpcargo), [`Shtp.ShtpLayer`](#shtpshtplayer)
- **BNO055 (IMU)**: [`Bno055`](#bno055), [`Bno055.Bno055Cmd`](#bno055bno055cmd), [`Bno055.Bno055Rpt`](#bno055bno055rpt), [`Bno055.RegData`](#bno055regdata), [`Bno055.StreamData`](#bno055streamdata), [`Bno055.Bno055Info`](#bno055bno055info), [`Bno055Regs`](#bno055regs), [`Bno055Regs.OprMode`](#bno055regsoprmode), [`Bno055Regs.PwrMode`](#bno055regspwrmode), [`Bno055Regs.TempSource`](#bno055regstempsource), [`Bno055Regs.Units`](#bno055regsunits), [`Bno055Regs.CalibStatus`](#bno055regscalibstatus), [`Bno055Regs.CalibrationProfile`](#bno055regscalibrationprofile), [`Bno055Regs.AxisRemap`](#bno055regsaxisremap), [`Bno055Regs.AccelConfig`](#bno055regsaccelconfig), [`Bno055Regs.GyroConfig`](#bno055regsgyroconfig), [`Bno055Regs.MagConfig`](#bno055regsmagconfig), [`Bno055Regs.RawBlock`](#bno055regsrawblock)
- **Datasets (record & replay)**: [`Dataset`](#dataset), [`Dataset.Record`](#datasetrecord)

## Transport

### Crc

```java
public final class Crc
```

CRC algorithms of the DEPZ transport (contracts/01_TRANSPORT_FRAMING.md §3).

All wire CRCs are reflected table implementations, byte-exact with the
firmware and the reference host tool. CRC-8 init is 0x00 for every device
(contracts/ERRATA.md E1). `crc16CcittFalse` is used only for the
`.fwdepz` file header (contract 06), never on the wire.

Returned values are unsigned, held in wider signed Java types: crc8 in an
`int` 0..255, crc16 in an `int` 0..65535, crc32 in a `long`
0..0xFFFFFFFF.

#### Crc.crc8Maxim

```java
public static int crc8Maxim(byte[] data)
```

CRC-8/MAXIM: poly 0x31 reflected, init 0x00, xorout 0x00.

#### Crc.crc16Modbus

```java
public static int crc16Modbus(byte[] data)
```

CRC-16/MODBUS: poly 0x8005 reflected, init 0xFFFF, xorout 0x0000.

#### Crc.crc32IsoHdlc

```java
public static long crc32IsoHdlc(byte[] data)
```

CRC-32/ISO-HDLC: poly 0x04C11DB7 reflected, init/xorout 0xFFFFFFFF.

#### Crc.crc16CcittFalse

```java
public static int crc16CcittFalse(byte[] data)
```

CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, not reflected.
Used only for the `.fwdepz` file header (contract 06).

### CrcError

```java
public record CrcError(int cmd, int seq) implements Event
```

A frame with a valid header whose payload CRC failed; dropped.

### CrcType

```java
public enum CrcType
```

Payload CRC type carried in the two top bits of the frame data-size field.

#### CrcType — constants

`NONE`(0, 0), `CRC8`(1, 1), `CRC16`(2, 2), `CRC32`(3, 4)

#### CrcType.value *(field)*

```java
public final int value
```

#### CrcType.size *(field)*

```java
public final int size
```

#### CrcType.fromValue

```java
public static CrcType fromValue(int value)
```

### Event

```java
public sealed interface Event permits Packet, Trash, CrcError
```

Base type for `PacketParser` events: `Packet`, `Trash`, `CrcError`.

### Framing

```java
public final class Framing
```

Packet framing (contracts/01_TRANSPORT_FRAMING.md). Byte-exact with the
firmware transport. Per contracts/ERRATA.md E6, a packet whose header
advertises a payload CRC type but carries an empty payload has no CRC
bytes on the wire.

#### Framing.MAGIC *(field)*

```java
public static final byte[] MAGIC
```

#### Framing.HEADER_SIZE *(field)*

```java
public static final int HEADER_SIZE = 7
```

#### Framing.MAX_PAYLOAD *(field)*

```java
public static final int MAX_PAYLOAD = 0x3FFF
```

#### Framing.payloadCrcBytes

```java
public static byte[] payloadCrcBytes(CrcType crcType, byte[] payload)
```

CRC trailer for a payload; empty payloads never carry CRC bytes (E6).

#### Framing.buildPacket

```java
public static byte[] buildPacket(int cmd, byte[] payload, int seq, CrcType crcType)
```

Frame one packet. `crcType` bits are set in the header even for an
empty payload (matching device TX), but CRC bytes are only appended for
non-empty payloads. `seq` is taken modulo 256.

#### Framing.buildPacket

```java
public static byte[] buildPacket(int cmd)
```

### Packet

```java
public record Packet(int cmd, int seq, byte[] payload) implements Event
```

A successfully decoded frame. `cmd`/`seq` are unsigned bytes 0..255.

### PacketParser

```java
public final class PacketParser
```

Incremental frame parser. Feed arbitrary byte chunks; get events.

Event order is invariant to chunking (contract 01 §5) except Trash event
boundaries — concatenate Trash data when comparing streams. Byte-exact with
the reference Python `PacketParser`, including the empty-payload-no-CRC
rule (ERRATA E6) and single-byte resync on a corrupt header.

#### PacketParser.packets *(field)*

```java
public int packets = 0
```

#### PacketParser.crcErrors *(field)*

```java
public int crcErrors = 0
```

#### PacketParser.headerErrors *(field)*

```java
public int headerErrors = 0
```

#### PacketParser.trashBytes *(field)*

```java
public int trashBytes = 0
```

#### PacketParser.residue

```java
public byte[] residue()
```

Current unconsumed bytes (residue).

#### PacketParser.feed

```java
public List<Event> feed(byte[] data)
```

### Trash

```java
public record Trash(byte[] data) implements Event
```

Bytes discarded while hunting for a valid frame. Boundaries between
consecutive Trash events depend on read chunking; only the concatenated
byte stream is deterministic (contract 01 §5).

## USB identity

### UsbIds

```java
public final class UsbIds
```

DEPZ USB identity table (contracts/02_COMMON_COMMANDS.md §4).

Used to pick the right serial port without poking unrelated devices; the
protocol probe (GET_NAME_ACTIVE_SOFTWARE) remains the source of truth for
what a device actually is. The PID→model map is an informational hint only.

#### UsbIds.DEPZ_USB_VID *(field)*

```java
public static final int DEPZ_USB_VID = 0x1BCF
```

Production VID shared by every DEPZ sensor.

#### UsbIds.PID_SR04 *(field)*

```java
public static final int PID_SR04 = 0xEC78
```

#### UsbIds.PID_VL53L8 *(field)*

```java
public static final int PID_VL53L8 = 0xED40
```

VL53L8CH production USB PID (the CH variant).

#### UsbIds.PID_VL53L8CX *(field)*

```java
public static final int PID_VL53L8CX = 0xED4B
```

VL53L8CX production USB PID (hw-verified).

#### UsbIds.PID_VL53L4CD *(field)*

```java
public static final int PID_VL53L4CD = 0xED45
```

VL53L4CD production USB PID.

#### UsbIds.PID_BNO086 *(field)*

```java
public static final int PID_BNO086 = 0xEE08
```

#### UsbIds.DEPZ_PID_MODEL *(field)*

```java
public static final Map<Integer, String> DEPZ_PID_MODEL
```

PID → sensor-model hint. Informational: the protocol probe is authoritative.

#### UsbIds.DEPZ_PID_RANGE_LO *(field)*

```java
public static final int DEPZ_PID_RANGE_LO = 60536
```

Whole reserved sensor block (60536..65535 inclusive).

#### UsbIds.DEPZ_PID_RANGE_HI *(field)*

```java
public static final int DEPZ_PID_RANGE_HI = 65535
```

#### UsbIds.DEV_USB_VID *(field)*

```java
public static final int DEV_USB_VID = 0x0483
```

Dev / unprogrammed default: STMicroelectronics VID/PID.

#### UsbIds.DEV_USB_PID *(field)*

```java
public static final int DEV_USB_PID = 0x56DC
```

#### UsbIds.isKnownDepzUsb

```java
public static boolean isKnownDepzUsb(Integer vid, Integer pid)
```

True when (vid, pid) is a recognized DEPZ (or dev-default) USB id.

#### UsbIds.usbModelHint

```java
public static String usbModelHint(Integer vid, Integer pid)
```

Best-guess model name for a (vid, pid), or `null`. Informational only.

#### UsbIds.orderBySerial

```java
public static List<PortRef> orderBySerial(List<PortRef> ports)
```

Order ports by USB iSerial ascending; null/empty serials sort last,
tie-broken by port path (contract 02 §4).

### UsbIds.PortRef

```java
public record PortRef(String port, String usbSerial)
```

One enumerated serial port with its USB iSerial (may be null/empty).

## Protocol codecs (common)

### Common

```java
public final class Common
```

Common command/report IDs and payload codecs (contracts/02_COMMON_COMMANDS.md).

Payload codecs return raw integers exactly as on the wire; unit
conversions (0.1 °C, µs) happen in the device layer.

#### Common.UNSOLICITED *(field)*

```java
public static final int UNSOLICITED = 0x00
```

Value of the echoed-cmd byte in unsolicited reports.

#### Common.packSyncTime

```java
public static byte[] packSyncTime(long pcTimestampUs)
```

#### Common.syncTimeOffsetRtt

```java
public static long[] syncTimeOffsetRtt(long t1, long t2, long t3, long t4)
```

NTP-style clock math, all µs (contract 02 §5). Returns
`{offset_us, rtt_us}` where offset = device_clock - host_clock,
computed as `((T2-T1)+(T3-T4)) / 2` truncated toward zero (Java
`long` division truncates toward zero, matching all SDKs), and
`rtt = (T4-T1)-(T3-T2)`.

#### Common.stripDeviceString

```java
public static String stripDeviceString(byte[] raw)
```

Decode an ASCII device string, dropping trailing NUL/0xFF filler.

### Common.Cmd

```java
public enum Cmd
```

Host→device command opcodes.

#### Common.Cmd — constants

`BOOTLOADER`(0x01), `DEVICE_RESET`(0x02), `GET_DEVICE_NAME`(0x03), `GET_NAME_ACTIVE_SOFTWARE`(0x04), `GET_SERIAL`(0x05), `SYNC_TIME`(0x06), `GET_MCU_TEMPERATURE`(0x07), `GET_PAYLOAD_CRC_TYPE`(0x08), `SET_PAYLOAD_CRC_TYPE`(0x09), `THROUGHPUT_TX_START`(0x1C), `THROUGHPUT_TX_STOP`(0x1D), `THROUGHPUT_RX_DATA`(0x1E), `GET_SYNC_PIN_CONFIG`(0x30), `SET_SYNC_PIN_CONFIG`(0x31)

#### Common.Cmd.value *(field)*

```java
public final int value
```

### Common.Rpt

```java
public enum Rpt
```

Device→host report IDs.

#### Common.Rpt — constants

`STATUS`(0x80), `TEXT`(0x81), `SYNC_TIME`(0x82), `TEMPERATURE`(0x83), `SEQUENCE_ERROR`(0x84), `PAYLOAD_CRC_TYPE`(0x87), `THROUGHPUT_DATA`(0x88), `SYNC_PIN_CONFIG`(0x90)

#### Common.Rpt.value *(field)*

```java
public final int value
```

### Common.Status

```java
public enum Status
```

#### Common.Status — constants

`OK`(0x00), `ERROR`(0x01), `ERR_INVALID_CMD`(0x02), `ERR_PAYLOAD_FORMAT`(0x03), `ERR_INVALID_PARAM`(0x04), `ERR_PAYLOAD_CRC`(0x05), `ERR_BUSY`(0x06), `ERR_CMD_NOT_SUPPORTED`(0x07), `ERR_NOT_INITIALIZED`(0x08), `ERR_HARDWARE_FAULT`(0x09)

#### Common.Status.value *(field)*

```java
public final int value
```

### Common.SyncPinMode

```java
public enum SyncPinMode
```

#### Common.SyncPinMode — constants

`DISABLE`(0x00), `IN`(0x01), `OUT_START`(0x02), `OUT_END`(0x03), `OUT_BOTH`(0x04)

#### Common.SyncPinMode.value *(field)*

```java
public final int value
```

#### Common.SyncPinMode.fromValue

```java
public static SyncPinMode fromValue(int v)
```

### Common.SyncPinPolarity

```java
public enum SyncPinPolarity
```

#### Common.SyncPinPolarity — constants

`IDLE_LOW`(0x00), `IDLE_HIGH`(0x01)

#### Common.SyncPinPolarity.value *(field)*

```java
public final int value
```

#### Common.SyncPinPolarity.fromValue

```java
public static SyncPinPolarity fromValue(int v)
```

### Common.StatusReport

```java
public record StatusReport(int cmd, int status)
```

`cmd`: echoed request opcode; 0x00 = unsolicited.

#### Common.StatusReport.unpack

```java
public static StatusReport unpack(byte[] p)
```

### Common.TextReport

```java
public record TextReport(int cmd, String text)
```

#### Common.TextReport.unpack

```java
public static TextReport unpack(byte[] p)
```

### Common.SyncTimeReport

```java
public record SyncTimeReport(long pcTimestampUs, long mcuRxUs, long mcuTxUs)
```

`pcTimestampUs`=T1 echoed, `mcuRxUs`=T2, `mcuTxUs`=T3.

#### Common.SyncTimeReport.unpack

```java
public static SyncTimeReport unpack(byte[] p)
```

### Common.TemperatureReport

```java
public record TemperatureReport(long timestampUs, int rawDecidegrees)
```

`rawDecidegrees`: int16, units of 0.1 °C.

#### Common.TemperatureReport.unpack

```java
public static TemperatureReport unpack(byte[] p)
```

#### Common.TemperatureReport.celsius

```java
public double celsius()
```

### Common.SequenceErrorReport

```java
public record SequenceErrorReport(int expectedSeq, int receivedSeq)
```

#### Common.SequenceErrorReport.unpack

```java
public static SequenceErrorReport unpack(byte[] p)
```

### Common.SyncPinConfig

```java
public record SyncPinConfig(int pin, SyncPinMode mode, SyncPinPolarity polarity)
```

`pin`: 1..5.

#### Common.SyncPinConfig.pack

```java
public byte[] pack()
```

#### Common.SyncPinConfig.unpack

```java
public static SyncPinConfig unpack(byte[] p)
```

### Identity

```java
public record Identity(String mode, SensorType sensorType, String softwareName, String version)
```

Firmware-name parsing (contracts/02_COMMON_COMMANDS.md §4).

- `mode` — "app" | "bootloader" | "unknown"
- `sensorType` — `null` in bootloader/unknown mode
- `softwareName` — the classified name
- `version` — "" when not parseable

#### Identity.parseSoftwareName

```java
public static Identity parseSoftwareName(String name)
```

Classify a GET_NAME_ACTIVE_SOFTWARE string. The string must already be
stripped of trailing NUL/0xFF (`Common#stripDeviceString`).

### Identity.SensorType

```java
public enum SensorType
```

#### Identity.SensorType — constants

`SR04`, `VL53L4`, `VL53L8`, `VL53L7`, `VL53LX`, `BNO086`, `BNO055`, `UNKNOWN`

#### Identity.SensorType.label *(field)*

```java
public final String label
```

## SR04

### Sr04

```java
public final class Sr04
```

SR04 wire codecs (contracts/03_SENSOR_SR04.md).

#### Sr04.ECHO_TIMEOUT *(field)*

```java
public static final int ECHO_TIMEOUT = 0xFFFF
```

echo_time_us sentinel: no echo received.

#### Sr04.SAMPLE_PERIOD_DEFAULT_US *(field)*

```java
public static final long SAMPLE_PERIOD_DEFAULT_US = 50_000L
```

#### Sr04.ECHO_DECAY_DEFAULT_US *(field)*

```java
public static final int ECHO_DECAY_DEFAULT_US = 5_000
```

#### Sr04.ECHO_DECAY_MIN_US *(field)*

```java
public static final int ECHO_DECAY_MIN_US = 4_000
```

#### Sr04.ECHO_DECAY_MAX_US *(field)*

```java
public static final int ECHO_DECAY_MAX_US = 65_000
```

#### Sr04.packSamplePeriod

```java
public static byte[] packSamplePeriod(long periodUs)
```

#### Sr04.unpackSamplePeriod

```java
public static long unpackSamplePeriod(byte[] p)
```

Reads the u32 sample period as an unsigned value in a long.

#### Sr04.packEchoDecay

```java
public static byte[] packEchoDecay(int decayUs)
```

#### Sr04.unpackEchoDecay

```java
public static int unpackEchoDecay(byte[] p)
```

#### Sr04.distanceMmFromEcho

```java
public static Double distanceMmFromEcho(int echoTimeUs, Double airTempC)
```

Round-trip echo time → distance in mm; `null` for the timeout
sentinel. Default speed of sound 343 m/s; with `airTempC` uses
c = 331.3 + 0.606·T (m/s).

### Sr04.Sr04Cmd

```java
public enum Sr04Cmd
```

#### Sr04.Sr04Cmd — constants

`GET_SAMPLE_PERIOD`(0x32), `SET_SAMPLE_PERIOD`(0x33), `GET_ECHO_DECAY`(0x34), `SET_ECHO_DECAY`(0x35), `MEASURE_ONCE`(0x36), `START_MEASUREMENT_LOOP`(0x37), `STOP_MEASUREMENT_LOOP`(0x38)

#### Sr04.Sr04Cmd.value *(field)*

```java
public final int value
```

### Sr04.Sr04Rpt

```java
public enum Sr04Rpt
```

#### Sr04.Sr04Rpt — constants

`DATA`(0x91), `SAMPLE_PERIOD`(0x92), `ECHO_DECAY`(0x93)

#### Sr04.Sr04Rpt.value *(field)*

```java
public final int value
```

### Sr04.Sr04Data

```java
public record Sr04Data(int sourceCmd, long timestampUs, int echoTimeUs)
```

`sourceCmd`: 0x36 single shot (host or SYNC_IN), 0x37 loop sample.

#### Sr04.Sr04Data.unpack

```java
public static Sr04Data unpack(byte[] p)
```

## Firmware container

### FwDepz

```java
public final class FwDepz
```

`.fwdepz` bootloader container parse/validate (contracts/06 §2).

#### FwDepz.FWDEPZ_MAGIC *(field)*

```java
public static final byte[] FWDEPZ_MAGIC
```

#### FwDepz.FWDEPZ_HEADER_SIZE *(field)*

```java
public static final int FWDEPZ_HEADER_SIZE = 64
```

### FwDepz.FwDepzError

```java
public static final class FwDepzError extends RuntimeException
```

Reason codes match the vector `error` strings.

#### FwDepz.FwDepzError.code *(field)*

```java
public final String code
```

### FwDepz.FwDepzImage

```java
public record FwDepzImage(long loadAddr, long fwSize, long fwCrc32, int curSec, int totSec, byte[] payload)
```

Parsed and validated `.fwdepz` firmware container.

#### FwDepz.FwDepzImage.payloadCrcOk

```java
public boolean payloadCrcOk()
```

#### FwDepz.FwDepzImage.parse

```java
public static FwDepzImage parse(byte[] blob)
```

Parse and validate. Validation order (contract 06 §2): length, magic,
then CRC-16/CCITT-FALSE over bytes [0..61] vs the u16 LE at offset 62,
then `fw_size == payload length`.

## VL53L4CD (ToF)

### Vl53l4

```java
public final class Vl53l4
```

VL53L4CD register-bridge wire codecs (contracts/10_SENSOR_VL53L4.md).

#### Vl53l4.XFER_MAX *(field)*

```java
public static final int XFER_MAX = 253
```

Max read/write length per transfer. The STM32 I2C NBYTES field is 8 bit
and a write spends two bytes on the register address; the firmware
applies the same 253 to both directions.

#### Vl53l4.XSHUT_OFF *(field)*

```java
public static final int XSHUT_OFF = 0
```

VL53_XSHUT action: drive XSHUT low (sensor powered down).

#### Vl53l4.XSHUT_ON *(field)*

```java
public static final int XSHUT_ON = 1
```

VL53_XSHUT action: drive XSHUT high, no boot handshake.

#### Vl53l4.XSHUT_RESET *(field)*

```java
public static final int XSHUT_RESET = 2
```

VL53_XSHUT action: pulse low then wait for the boot handshake (~3 ms).

#### Vl53l4.SF_INT_ACT_HIGH *(field)*

```java
public static final int SF_INT_ACT_HIGH = 0x02
```

VL53_START_STREAM flags bit 1: interrupt polarity, mirroring bit 4 of
GPIO_HV_MUX__CTRL (0x0030). Clear (default): INT active low.

#### Vl53l4.packReadReg

```java
public static byte[] packReadReg(int addr, int len)
```

VL53_READ_REG payload: `addr u16, len u16` little-endian.

#### Vl53l4.packWriteReg

```java
public static byte[] packWriteReg(int addr, byte[] data)
```

VL53_WRITE_REG payload: `addr u16, data[1..253]`.

#### Vl53l4.packXshut

```java
public static byte[] packXshut(int action)
```

VL53_XSHUT payload: `action u8` (XSHUT_OFF/ON/RESET).

#### Vl53l4.packStartStream

```java
public static byte[] packStartStream(int addr, int len, int flags)
```

VL53_START_STREAM payload: `addr u16, len u16, flags u8`.

#### Vl53l4.packSetI2cSpeed

```java
public static byte[] packSetI2cSpeed(int khz)
```

VL53_SET_I2C_SPEED payload: `khz u16`.

### Vl53l4.Vl53l4Cmd

```java
public enum Vl53l4Cmd
```

#### Vl53l4.Vl53l4Cmd — constants

`READ_REG`(0x32), `WRITE_REG`(0x33), `XSHUT`(0x34), `START_STREAM`(0x35), `STOP_STREAM`(0x36), `GET_INFO`(0x37), `SET_I2C_SPEED`(0x38)

#### Vl53l4.Vl53l4Cmd.value *(field)*

```java
public final int value
```

### Vl53l4.Vl53l4Rpt

```java
public enum Vl53l4Rpt
```

#### Vl53l4.Vl53l4Rpt — constants

`REG_DATA`(0x91), `INFO`(0x92), `STREAM`(0x93)

#### Vl53l4.Vl53l4Rpt.value *(field)*

```java
public final int value
```

### Vl53l4.RegData

```java
public record RegData(int cmd, long timestampUs, byte[] data)
```

RPT_VL53_REG_DATA — one register read. `cmd` echoes the READ_REG
opcode; `timestampUs` is MCU uptime at I2C-read completion.

#### Vl53l4.RegData.unpack

```java
public static RegData unpack(byte[] p)
```

### Vl53l4.Vl53l4Info

```java
public record Vl53l4Info(long intEdges, long slotsSkipped, long i2cErrors, int lastI2cError, int modelId, int fwStatus, int initialized, int xshutLevel, int intLevel, int i2cKhz)
```

RPT_VL53_INFO — bridge diagnostics (21-byte LE payload). Counters are
free-running and wrap silently; watch increments, not absolute values.
`modelId` expected 0xEBAA, `fwStatus` expected 0x03 (booted);
`lastI2cError`: 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR.

#### Vl53l4.Vl53l4Info.unpack

```java
public static Vl53l4Info unpack(byte[] p)
```

### Vl53l4.StreamData

```java
public record StreamData(long timestampUs, int addr, int len, byte[] data)
```

RPT_VL53_STREAM — one streamed register block. `addr`/`len`
echo the stream configuration so each report is self-describing;
`timestampUs` is MCU uptime at the INT edge (the sensor event).

#### Vl53l4.StreamData.unpack

```java
public static StreamData unpack(byte[] p)
```

### Vl53l4Uld

```java
public final class Vl53l4Uld
```

VL53L4CD ULD (ST STSW-IMG026 2.2.3) — the host-verifiable decode layer only.

Port of the pure codec/math pieces of `VL53L4CD_api.c` that are
hardware-independent and covered by golden vectors: the 17-byte result-block
decode (`parseResultBlock`), the SetRangeTiming/GetRangeTiming
register math (`rangeTimingRegisters`, `decodeRangeTiming`),
the tuning-register word codecs and the 91-byte init configuration block
(`configBlock`). Semantics mirror the Python reference
(`depz_sensor_sdk/vl53l4/uld.py`) 1:1 — integer widths and 32-bit
truncations included; do not "simplify" the math.

OUT OF SCOPE (extension point, intentionally not ported): the live ULD
init / register-bridge driver (sensor_init, VHV calibration, offset/xtalk
calibration loops). Those depend on a live I2C bridge and cannot be replayed
deterministically; see `liveDriverStubbed()`.

#### Vl53l4Uld.SOFT_RESET *(field)*

```java
public static final int SOFT_RESET = 0x0000
```

#### Vl53l4Uld.I2C_SLAVE_DEVICE_ADDRESS *(field)*

```java
public static final int I2C_SLAVE_DEVICE_ADDRESS = 0x0001
```

#### Vl53l4Uld.OSC_FREQUENCY *(field)*

```java
public static final int OSC_FREQUENCY = 0x0006
```

Unnamed in the C driver; the oscillator-frequency word.

#### Vl53l4Uld.VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND *(field)*

```java
public static final int VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND = 0x0008
```

#### Vl53l4Uld.XTALK_PLANE_OFFSET_KCPS *(field)*

```java
public static final int XTALK_PLANE_OFFSET_KCPS = 0x0016
```

#### Vl53l4Uld.XTALK_X_PLANE_GRADIENT_KCPS *(field)*

```java
public static final int XTALK_X_PLANE_GRADIENT_KCPS = 0x0018
```

#### Vl53l4Uld.XTALK_Y_PLANE_GRADIENT_KCPS *(field)*

```java
public static final int XTALK_Y_PLANE_GRADIENT_KCPS = 0x001A
```

#### Vl53l4Uld.RANGE_OFFSET_MM *(field)*

```java
public static final int RANGE_OFFSET_MM = 0x001E
```

#### Vl53l4Uld.INNER_OFFSET_MM *(field)*

```java
public static final int INNER_OFFSET_MM = 0x0020
```

#### Vl53l4Uld.OUTER_OFFSET_MM *(field)*

```java
public static final int OUTER_OFFSET_MM = 0x0022
```

#### Vl53l4Uld.GPIO_HV_MUX_CTRL *(field)*

```java
public static final int GPIO_HV_MUX_CTRL = 0x0030
```

#### Vl53l4Uld.GPIO_TIO_HV_STATUS *(field)*

```java
public static final int GPIO_TIO_HV_STATUS = 0x0031
```

#### Vl53l4Uld.SYSTEM_INTERRUPT *(field)*

```java
public static final int SYSTEM_INTERRUPT = 0x0046
```

#### Vl53l4Uld.RANGE_CONFIG_A *(field)*

```java
public static final int RANGE_CONFIG_A = 0x005E
```

#### Vl53l4Uld.RANGE_CONFIG_B *(field)*

```java
public static final int RANGE_CONFIG_B = 0x0061
```

#### Vl53l4Uld.RANGE_CONFIG_SIGMA_THRESH *(field)*

```java
public static final int RANGE_CONFIG_SIGMA_THRESH = 0x0064
```

#### Vl53l4Uld.MIN_COUNT_RATE_RTN_LIMIT_MCPS *(field)*

```java
public static final int MIN_COUNT_RATE_RTN_LIMIT_MCPS = 0x0066
```

#### Vl53l4Uld.INTERMEASUREMENT_MS *(field)*

```java
public static final int INTERMEASUREMENT_MS = 0x006C
```

#### Vl53l4Uld.THRESH_HIGH *(field)*

```java
public static final int THRESH_HIGH = 0x0072
```

#### Vl53l4Uld.THRESH_LOW *(field)*

```java
public static final int THRESH_LOW = 0x0074
```

#### Vl53l4Uld.SYSTEM_INTERRUPT_CLEAR *(field)*

```java
public static final int SYSTEM_INTERRUPT_CLEAR = 0x0086
```

#### Vl53l4Uld.SYSTEM_START *(field)*

```java
public static final int SYSTEM_START = 0x0087
```

#### Vl53l4Uld.RESULT_RANGE_STATUS *(field)*

```java
public static final int RESULT_RANGE_STATUS = 0x0089
```

#### Vl53l4Uld.RESULT_SPAD_NB *(field)*

```java
public static final int RESULT_SPAD_NB = 0x008C
```

#### Vl53l4Uld.RESULT_SIGNAL_RATE *(field)*

```java
public static final int RESULT_SIGNAL_RATE = 0x008E
```

#### Vl53l4Uld.RESULT_AMBIENT_RATE *(field)*

```java
public static final int RESULT_AMBIENT_RATE = 0x0090
```

#### Vl53l4Uld.RESULT_SIGMA *(field)*

```java
public static final int RESULT_SIGMA = 0x0092
```

#### Vl53l4Uld.RESULT_DISTANCE *(field)*

```java
public static final int RESULT_DISTANCE = 0x0096
```

#### Vl53l4Uld.RESULT_OSC_CALIBRATE_VAL *(field)*

```java
public static final int RESULT_OSC_CALIBRATE_VAL = 0x00DE
```

#### Vl53l4Uld.FIRMWARE_SYSTEM_STATUS *(field)*

```java
public static final int FIRMWARE_SYSTEM_STATUS = 0x00E5
```

#### Vl53l4Uld.IDENTIFICATION_MODEL_ID *(field)*

```java
public static final int IDENTIFICATION_MODEL_ID = 0x010F
```

#### Vl53l4Uld.MODEL_ID *(field)*

```java
public static final int MODEL_ID = 0xEBAA
```

IDENTIFICATION__MODEL_ID word expected from a live VL53L4CD.

#### Vl53l4Uld.WINDOW_BELOW *(field)*

```java
public static final int WINDOW_BELOW = 0
```

Detection-threshold window modes (SYSTEM__INTERRUPT).

#### Vl53l4Uld.WINDOW_ABOVE *(field)*

```java
public static final int WINDOW_ABOVE = 1
```

#### Vl53l4Uld.WINDOW_OUT *(field)*

```java
public static final int WINDOW_OUT = 2
```

#### Vl53l4Uld.WINDOW_IN *(field)*

```java
public static final int WINDOW_IN = 3
```

#### Vl53l4Uld.CONFIG_ADDR *(field)*

```java
public static final int CONFIG_ADDR = 0x2D
```

First register of the init configuration block (0x2D..0x87).

#### Vl53l4Uld.DEFAULT_CONFIGURATION *(field)*

```java
public static final byte[] DEFAULT_CONFIGURATION
```

VL53L4CD_DEFAULT_CONFIGURATION[] — 91 bytes, registers 0x2D..0x87.
`configBlock()` always overrides byte 0 (register 0x2D) with
`CONFIG_FMP_BYTE` (0x12) to put the sensor's I2C pad in Fast Mode
Plus — exactly what VL53L4CD_I2C_FAST_MODE_PLUS does in the C ULD.

#### Vl53l4Uld.CONFIG_FMP_BYTE *(field)*

```java
public static final int CONFIG_FMP_BYTE = 0x12
```

Byte forced at register 0x2D: I2C Fast Mode Plus pad, never cleared.

#### Vl53l4Uld.RESULT_BLOCK_ADDR *(field)*

```java
public static final int RESULT_BLOCK_ADDR
```

The block the MCU streams: RESULT__RANGE_STATUS .. 0x0099.

#### Vl53l4Uld.RESULT_BLOCK_LEN *(field)*

```java
public static final int RESULT_BLOCK_LEN = 17
```

#### Vl53l4Uld.STATUS_RTN *(field)*

```java
public static final int[] STATUS_RTN
```

GetResult() raw status → ULD status (status_rtn[24] in VL53L4CD_api.c).

#### Vl53l4Uld.configBlock

```java
public static byte[] configBlock()
```

The 91-byte block sensor_init() writes at `CONFIG_ADDR`: the ST
default configuration with byte 0 forced to `CONFIG_FMP_BYTE`
(Fast Mode Plus).

#### Vl53l4Uld.parseResultBlock

```java
public static Results parseResultBlock(byte[] raw)
```

Decode the streamed 0x0089..0x0099 block exactly as VL53L4CD_GetResult()
decodes the same registers read one by one. Register contents are
big-endian words (the bridge passes them through untouched). Throws
`Vl53l4Error` when `raw` is shorter than 15 bytes.

#### Vl53l4Uld.rangeTimingRegisters

```java
public static int[] rangeTimingRegisters(int timingBudgetMs, int interMeasurementMs, int oscFrequency, int clockPll)
```

SetRangeTiming register math → `{RANGE_CONFIG_A, RANGE_CONFIG_B,
INTERMEASUREMENT_MS raw dword}`.

`oscFrequency` is the word read from 0x0006; `clockPll`
is the word read from RESULT__OSC_CALIBRATE_VAL (used only in autonomous
mode, i.e. when `interMeasurementMs > 0`). Budget 10..200 ms;
`interMeasurementMs` 0 (continuous) or greater than the budget
(autonomous low power) — anything else throws `Vl53l4Error`.

#### Vl53l4Uld.decodeRangeTiming

```java
public static int[] decodeRangeTiming(long intermeasurementRaw, int clockPll, int oscFrequency, int rangeConfigA)
```

GetRangeTiming register math → `{timing_budget_ms,
inter_measurement_ms}`.

Inputs are the raw register reads: INTERMEASUREMENT_MS dword, the
RESULT__OSC_CALIBRATE_VAL word, the 0x0006 word and the RANGE_CONFIG_A
word.

#### Vl53l4Uld.offsetRaw

```java
public static int offsetRaw(int offsetMm)
```

RANGE_OFFSET_MM word for SetOffset (INNER/OUTER are zeroed alongside).

#### Vl53l4Uld.decodeOffset

```java
public static int decodeOffset(int rawWord)
```

GetOffset: RANGE_OFFSET_MM word → signed millimetres.

#### Vl53l4Uld.xtalkRaw

```java
public static int xtalkRaw(int xtalkKcps)
```

XTALK_PLANE_OFFSET_KCPS word for SetXtalk.

#### Vl53l4Uld.decodeXtalk

```java
public static int decodeXtalk(int rawWord)
```

GetXtalk: XTALK_PLANE_OFFSET_KCPS word → kcps.

#### Vl53l4Uld.signalThresholdRaw

```java
public static int signalThresholdRaw(int signalKcps)
```

MIN_COUNT_RATE_RTN_LIMIT_MCPS word for SetSignalThreshold.

#### Vl53l4Uld.decodeSignalThreshold

```java
public static int decodeSignalThreshold(int rawWord)
```

GetSignalThreshold: word → kcps.

#### Vl53l4Uld.sigmaThresholdRaw

```java
public static int sigmaThresholdRaw(int sigmaMm)
```

RANGE_CONFIG__SIGMA_THRESH word for SetSigmaThreshold (mm ≤ 16383).

#### Vl53l4Uld.decodeSigmaThreshold

```java
public static int decodeSigmaThreshold(int rawWord)
```

GetSigmaThreshold: word → mm.

#### Vl53l4Uld.liveDriverStubbed

```java
public static boolean liveDriverStubbed()
```

The live ULD init / register-bridge driver (sensor_init, VHV/offset/xtalk
calibration) is intentionally not ported to Java — it is
hardware-dependent and out of the decode scope.

### Vl53l4Uld.Vl53l4Error

```java
public static final class Vl53l4Error extends RuntimeException
```

ULD error.

### Vl53l4Uld.Results

```java
public record Results(int rangeStatus, int distanceMm, int ambientRateKcps, int ambientPerSpadKcps, int signalRateKcps, int signalPerSpadKcps, int numberOfSpad, int sigmaMm, int streamCount)
```

VL53L4CD_ResultsData_t plus the sensor's own frame counter.

## VL53L8 (ToF)

### FrameReassembler

```java
public final class FrameReassembler
```

Rebuilds full VL53L8 sensor frames from chunked RPT_VL53_FRAME reports
(contracts/04_SENSOR_VL53L8.md). Shared by both VL53L8CX and VL53L8CH — the
chunk transport is identical across the two parts.

Rules: reset on `offset == 0`; chunks must be contiguous — a gap
discards the frame in progress; a frame completes when the accumulated bytes
equal `fullSize`. Mirror of the TS/Python `FrameReassembler`.

#### FrameReassembler.STREAM_CHUNK_MAX *(field)*

```java
public static final int STREAM_CHUNK_MAX = 1528
```

Bytes of frame data per RPT_VL53_FRAME chunk.

#### FrameReassembler.completed *(field)*

```java
public int completed = 0
```

#### FrameReassembler.discarded *(field)*

```java
public int discarded = 0
```

#### FrameReassembler.unpackFrameChunk

```java
public static FrameChunk unpackFrameChunk(byte[] payload)
```

Decode a RPT_VL53_FRAME payload: ts u64 LE, fullSize u16, offset u16, data.

#### FrameReassembler.feed

```java
public CompletedFrame feed(FrameChunk chunk)
```

Returns a completed frame or `null`.

### FrameReassembler.FrameChunk

```java
public record FrameChunk(long timestampUs, int fullSize, int offset, byte[] data)
```

One RPT_VL53_FRAME chunk (payload of a cmd-0x93 packet).

### FrameReassembler.CompletedFrame

```java
public record CompletedFrame(long timestampUs, byte[] frame)
```

Result of a completed frame.

### Vl53l8Uld

```java
public final class Vl53l8Uld
```

VL53L8CX/CH ULD — the host-verifiable decode layer only.

The DEPZ ToF is two sensors: VL53L8CX (the base multizone
part, also the dev/unprogrammed default) and VL53L8CH (CX plus CNH
compact histograms and its own production USB PID `0xED40`). Their
results-frame wire layout is identical, so this decoder serves both:
`parseFrame` is shared, and the advanced-feature DCI codecs (motion
configuration, detection thresholds, xtalk margin) apply equally to CX and CH.
The only variant difference in the decode path is the footer-id offset —
see `Variant`. Semantics mirror the TS/Python references 1:1.

This is a port of the parts of the ST ULD (`vl53l8cx_api.c`, ULD
2.1.0) that are pure, hardware-independent and covered by golden vectors.

OUT OF SCOPE (extension points, intentionally not ported):
- Live ULD init / config register-bridge driver (both variants):
sensor-firmware download, `dciReadData`/`dciWriteData`,
`setResolution`, `startRanging`, power-mode transitions,
xtalk calibration. Those depend on a live SPI bridge and cannot be
replayed deterministically; see `liveDriverStubbed()`.

#### Vl53l8Uld.RESOLUTION_4X4 *(field)*

```java
public static final int RESOLUTION_4X4 = 16
```

#### Vl53l8Uld.RESOLUTION_8X8 *(field)*

```java
public static final int RESOLUTION_8X8 = 64
```

#### Vl53l8Uld.NB_TARGET_PER_ZONE *(field)*

```java
public static final int NB_TARGET_PER_ZONE = 1
```

#### Vl53l8Uld.FOOTER_ID_OFF_CX *(field)*

```java
public static final int FOOTER_ID_OFF_CX = 12
```

Footer-id offset from frame end: cx (ULD 2.1.0) = 12, ch (2.0.16) = 4.

#### Vl53l8Uld.FOOTER_ID_OFF_CH *(field)*

```java
public static final int FOOTER_ID_OFF_CH = 4
```

#### Vl53l8Uld.STATUS_INVALID_PARAM *(field)*

```java
public static final int STATUS_INVALID_PARAM = 127
```

#### Vl53l8Uld.STATUS_CORRUPTED_FRAME *(field)*

```java
public static final int STATUS_CORRUPTED_FRAME = 2
```

#### Vl53l8Uld.CNH_DATA_IDX *(field)*

```java
public static final int CNH_DATA_IDX = 0xc048
```

VL53LMZ CNH data output block (VL53L8CH / VL53L7CH).

#### Vl53l8Uld.RESOLUTION_FROM_FRAME *(field)*

```java
public static final int RESOLUTION_FROM_FRAME = -1
```

`resolution` argument of `parseFrame(byte[], int, int, int)`:
take the zone count from the zone-scaled ambient block (index 0x54D0,
always sized to the resolution) and trim to it.

#### Vl53l8Uld.DIST_MM *(field)*

```java
public static final int DIST_MM = 1
```

#### Vl53l8Uld.SIGNAL_PER_SPAD_KCPS *(field)*

```java
public static final int SIGNAL_PER_SPAD_KCPS = 2
```

#### Vl53l8Uld.RANGE_SIGMA_MM *(field)*

```java
public static final int RANGE_SIGMA_MM = 4
```

#### Vl53l8Uld.AMBIENT_PER_SPAD_KCPS *(field)*

```java
public static final int AMBIENT_PER_SPAD_KCPS = 8
```

#### Vl53l8Uld.NB_TARGET_DETECTED *(field)*

```java
public static final int NB_TARGET_DETECTED = 9
```

#### Vl53l8Uld.TAR_STATUS *(field)*

```java
public static final int TAR_STATUS = 12
```

#### Vl53l8Uld.NB_SPADS_ENABLED *(field)*

```java
public static final int NB_SPADS_ENABLED = 13
```

#### Vl53l8Uld.MOTION_INDICATOR *(field)*

```java
public static final int MOTION_INDICATOR = 19
```

#### Vl53l8Uld.NB_THRESHOLDS *(field)*

```java
public static final int NB_THRESHOLDS = 64
```

#### Vl53l8Uld.LAST_THRESHOLD *(field)*

```java
public static final int LAST_THRESHOLD = 128
```

#### Vl53l8Uld.POWER_MODE_SLEEP *(field)*

```java
public static final int POWER_MODE_SLEEP = 0
```

#### Vl53l8Uld.POWER_MODE_WAKEUP *(field)*

```java
public static final int POWER_MODE_WAKEUP = 1
```

#### Vl53l8Uld.POWER_MODE_DEEP_SLEEP *(field)*

```java
public static final int POWER_MODE_DEEP_SLEEP = 2
```

#### Vl53l8Uld.liveDriverStubbed

```java
public static boolean liveDriverStubbed()
```

The live ULD init/config register-bridge driver is intentionally not
ported to Java — it is hardware-dependent and out of the decode scope.
Applies to both VL53L8CX and VL53L8CH.

#### Vl53l8Uld.CNH_MAX_AGGREGATES *(field)*

```java
public static final int CNH_MAX_AGGREGATES = 64
```

decodeCnh() bounds — the same limits as the other SDKs.

#### Vl53l8Uld.CNH_MAX_FEATURE_LENGTH *(field)*

```java
public static final int CNH_MAX_FEATURE_LENGTH = 255
```

#### Vl53l8Uld.decodeCnh

```java
public static CnhResult decodeCnh(int nbOfAggregates, int featureLength, byte[] raw)
```

Decode a captured CNH data block (`raw`, byte-swapped exactly like the
standard ranging blocks) into per-aggregate histograms, for the fixed
cnh_cfg (ping-pong + variance disabled). Faithful port of the ST CNH plugin
/ Python `cnh.decode`.

- `nbOfAggregates` — number of CNH aggregates (from the CNH config)
- `featureLength` — CNH bins per aggregate (from the CNH config)
- `raw` — captured CNH block bytes
shorter than those counts imply (as the C / C++ / Rust / C# decoders)

#### Vl53l8Uld.swapBuffer

```java
public static byte[] swapBuffer(byte[] data)
```

VL53L8CX_SwapBuffer: byte-reverse every 32-bit word (tail untouched).

#### Vl53l8Uld.bhFields

```java
public static int[] bhFields(long bh)
```

union Block_header: type[3:0], size[15:4], idx[31:16].

#### Vl53l8Uld.parseFrame

```java
public static Results parseFrame(byte[] raw, int dataReadSize, Variant variant)
```

Parse one raw results frame (`dataReadSize` bytes) for the given
variant (CX or CH). Convenience overload of
`parseFrame(byte[], int, int)` that picks the variant-specific
footer-id offset. The decode body is shared across CX and CH.

#### Vl53l8Uld.parseFrame

```java
public static Results parseFrame(byte[] raw, int dataReadSize, int footerIdOff)
```

Parse one raw results frame (`dataReadSize` bytes). `footerIdOff`
is variant-specific (cx = 12, ch = 4); prefer the
`parseFrame(byte[], int, Variant)` overload. Shared by VL53L8CX and
VL53L8CH. Throws on a header/footer id mismatch.

#### Vl53l8Uld.parseFrame

```java
public static Results parseFrame(byte[] raw, int dataReadSize, int footerIdOff, int resolution)
```

Parse one raw results frame and trim every per-zone array to
`resolution` zones (x `NB_TARGET_PER_ZONE` for per-target
arrays). Needed on VL53L5/L7 (contract 11 §3): blocks above index 0x6C90
keep their declared 64-entry size even in 4x4, the sensor fills the first
`resolution` entries and zero-pads the rest. `resolution`:
`0` = no trim (VL53L8 behaviour), `16`/`64` = explicit,
`RESOLUTION_FROM_FRAME` = the size of the zone-scaled ambient block.

#### Vl53l8Uld.xtalkMarginToRaw

```java
public static long xtalkMarginToRaw(double marginKcps)
```

Xtalk margin (kcps/spad) -> raw DCI value (round(kcps * 2048)).

#### Vl53l8Uld.packDetectionThresholds

```java
public static PackedThresholds packDetectionThresholds(List<DetectionThreshold> thresholds)
```

Pack up to 64 detection thresholds into the DCI_DET_THRESH_START payload
plus the 8-byte valid-status block. Each threshold's low/high are scaled
by its measurement selector. Mirror of set_detection_thresholds.

#### Vl53l8Uld.motionConfigSetResolution

```java
public static void motionConfigSetResolution(MotionConfig cfg, int resolution)
```

Set MotionConfig.mapId for the given resolution (pure — no I/O).

#### Vl53l8Uld.defaultMotionConfig

```java
public static MotionConfig defaultMotionConfig(int resolution)
```

Default motion-indicator configuration for a resolution (pure).

### Vl53l8Uld.Variant

```java
public enum Variant
```

Which VL53L8 part produced a frame. The frame decode is shared; the only
per-variant difference is the footer-id offset. CX is the dev-default base
part; CH adds CNH histograms and its own production USB PID (0xED40).

#### Vl53l8Uld.Variant — constants

`CX`(FOOTER_ID_OFF_CX), `CH`(FOOTER_ID_OFF_CH)

#### Vl53l8Uld.Variant.footerIdOff *(field)*

```java
public final int footerIdOff
```

Footer-id offset from the frame end for this variant.

### Vl53l8Uld.CnhAggregate

```java
public static final class CnhAggregate
```

One decoded CNH aggregate: raw histogram ints + per-bin int8 scalers.

#### Vl53l8Uld.CnhAggregate.histRaw *(field)*

```java
public final int[] histRaw
```

Signed int32 histogram value per CNH bin (len == featureLength).

#### Vl53l8Uld.CnhAggregate.histScaler *(field)*

```java
public final int[] histScaler
```

int8 scaler per CNH bin (len == featureLength); value = histRaw / 2^scaler.

### Vl53l8Uld.CnhResult

```java
public static final class CnhResult
```

Decoded CNH data block: reference residual word + per-aggregate histograms.

#### Vl53l8Uld.CnhResult.refResidualWord *(field)*

```java
public final long refResidualWord
```

Raw uint32 ref-residual word (words[2]); float residual = value / 2048.

#### Vl53l8Uld.CnhResult.aggregates *(field)*

```java
public final CnhAggregate[] aggregates
```

Per-aggregate histograms (len == nbOfAggregates).

### Vl53l8Uld.Results

```java
public static final class Results
```

Parsed raw results frame (raw-integer per-zone arrays).

#### Vl53l8Uld.Results.distanceMm *(field)*

```java
public int[] distanceMm
```

#### Vl53l8Uld.Results.targetStatus *(field)*

```java
public int[] targetStatus
```

#### Vl53l8Uld.Results.nbTargetDetected *(field)*

```java
public int[] nbTargetDetected
```

#### Vl53l8Uld.Results.signalPerSpad *(field)*

```java
public long[] signalPerSpad
```

#### Vl53l8Uld.Results.ambientPerSpad *(field)*

```java
public long[] ambientPerSpad
```

#### Vl53l8Uld.Results.nbSpadsEnabled *(field)*

```java
public long[] nbSpadsEnabled
```

#### Vl53l8Uld.Results.rangeSigmaMm *(field)*

```java
public double[] rangeSigmaMm
```

#### Vl53l8Uld.Results.reflectance *(field)*

```java
public int[] reflectance
```

#### Vl53l8Uld.Results.siliconTempDegc *(field)*

```java
public int siliconTempDegc = 0
```

#### Vl53l8Uld.Results.cnhRaw *(field)*

```java
public byte[] cnhRaw
```

Raw CNH (compact-histogram) block bytes (VL53L8CH / VL53L7CH, block
`CNH_DATA_IDX`, word-swapped like every block); `null`
when absent (always on CX). Decode with `decodeCnh`.

#### Vl53l8Uld.Results.resolution

```java
public int resolution()
```

Zones actually present this frame (16 for 4x4, 64 for 8x8).

### Vl53l8Uld.DetectionThreshold

```java
public record DetectionThreshold(int lowThresh, int highThresh, int measurement, int type, int zoneNum, int operation)
```

One detection-threshold entry (real units).

### Vl53l8Uld.PackedThresholds

```java
public record PackedThresholds(byte[] start, byte[] valid)
```

`DCI_DET_THRESH_START` payload (64x12 B) + 8-B valid-status block.

### Vl53l8Uld.MotionConfig

```java
public static final class MotionConfig
```

Mirror of VL53L8CX_Motion_Configuration (156 bytes, `<i3I12B64b32B32B`).

#### Vl53l8Uld.MotionConfig.refBinOffset *(field)*

```java
public int refBinOffset = 0
```

#### Vl53l8Uld.MotionConfig.detectionThreshold *(field)*

```java
public long detectionThreshold = 0
```

#### Vl53l8Uld.MotionConfig.extraNoiseSigma *(field)*

```java
public long extraNoiseSigma = 0
```

#### Vl53l8Uld.MotionConfig.nullDenClipValue *(field)*

```java
public long nullDenClipValue = 0
```

#### Vl53l8Uld.MotionConfig.memUpdateMode *(field)*

```java
public int memUpdateMode = 0
```

#### Vl53l8Uld.MotionConfig.memUpdateChoice *(field)*

```java
public int memUpdateChoice = 0
```

#### Vl53l8Uld.MotionConfig.sumSpan *(field)*

```java
public int sumSpan = 0
```

#### Vl53l8Uld.MotionConfig.featureLength *(field)*

```java
public int featureLength = 0
```

#### Vl53l8Uld.MotionConfig.nbOfAggregates *(field)*

```java
public int nbOfAggregates = 0
```

#### Vl53l8Uld.MotionConfig.nbOfTemporalAccumulations *(field)*

```java
public int nbOfTemporalAccumulations = 0
```

#### Vl53l8Uld.MotionConfig.minNbForGlobalDetection *(field)*

```java
public int minNbForGlobalDetection = 0
```

#### Vl53l8Uld.MotionConfig.globalIndicatorFormat1 *(field)*

```java
public int globalIndicatorFormat1 = 0
```

#### Vl53l8Uld.MotionConfig.globalIndicatorFormat2 *(field)*

```java
public int globalIndicatorFormat2 = 0
```

#### Vl53l8Uld.MotionConfig.spare1 *(field)*

```java
public int spare1 = 0
```

#### Vl53l8Uld.MotionConfig.spare2 *(field)*

```java
public int spare2 = 0
```

#### Vl53l8Uld.MotionConfig.spare3 *(field)*

```java
public int spare3 = 0
```

#### Vl53l8Uld.MotionConfig.mapId *(field)*

```java
public final int[] mapId
```

#### Vl53l8Uld.MotionConfig.indicatorFormat1 *(field)*

```java
public final int[] indicatorFormat1
```

#### Vl53l8Uld.MotionConfig.indicatorFormat2 *(field)*

```java
public final int[] indicatorFormat2
```

#### Vl53l8Uld.MotionConfig.pack

```java
public byte[] pack()
```

### Vl53l8Uld.Vl53l8Error

```java
public static final class Vl53l8Error extends RuntimeException
```

ULD status error.

#### Vl53l8Uld.Vl53l8Error.code *(field)*

```java
public final int code
```

## VL53L5CX / VL53L7CX / VL53L7CH (ToF)

### Vl53l7

```java
public final class Vl53l7
```

VL53L5CX / VL53L7CX / VL53L7CH I2C register-bridge wire codecs
(contracts/11_SENSOR_VL53L7.md, a delta against contracts/04_SENSOR_VL53L8.md).

Commands 0x32/0x33/0x35/0x36 and reports 0x91/0x93 are bit-for-bit the
VL53L8 bridge; RPT_VL53_FRAME (0x93) chunks are decoded and reassembled by
`ai.depz.sensor.sensors.vl53l8.FrameReassembler`. This class adds what
the I2C board brings: PIN_CTRL, GET_INFO, SET_I2C_SPEED, RPT_VL53_INFO and its
tighter transfer limits. The register encoders are included here because the
Java SDK has no separate VL53L8 protocol class (its VL53L8 support is
decode-only).

#### Vl53l7.I2C_ERR_OK *(field)*

```java
public static final int I2C_ERR_OK = 0
```

RPT_VL53_INFO.lastI2cError values.

#### Vl53l7.I2C_ERR_NACK *(field)*

```java
public static final int I2C_ERR_NACK = 1
```

#### Vl53l7.I2C_ERR_TIMEOUT *(field)*

```java
public static final int I2C_ERR_TIMEOUT = 2
```

#### Vl53l7.I2C_ERR_BUS_ERROR *(field)*

```java
public static final int I2C_ERR_BUS_ERROR = 3
```

#### Vl53l7.READ_MAX_LEN *(field)*

```java
public static final int READ_MAX_LEN = 1536
```

VL53LMZ_READ_MAX: READ_REG len 1..1536 (the VL53L8 host's 2048 fails here).

#### Vl53l7.WRITE_MAX_LEN *(field)*

```java
public static final int WRITE_MAX_LEN = 2048
```

VL53LMZ_XFER_MAX: WRITE_REG N 1..2048.

#### Vl53l7.STREAM_CHUNK_MAX *(field)*

```java
public static final int STREAM_CHUNK_MAX = 1536
```

Bytes of frame data per RPT_VL53_FRAME chunk (VL53L8: 1528).

#### Vl53l7.INFO_SIZE *(field)*

```java
public static final int INFO_SIZE = 20
```

RPT_VL53_INFO payload size.

#### Vl53l7.I2C_SPEED_STEPS_KHZ *(field)*

```java
public static final int[] I2C_SPEED_STEPS_KHZ
```

Nominal SCL steps the firmware carries a timing for; others snap to nearest.

#### Vl53l7.packReadReg

```java
public static byte[] packReadReg(int addr, int len)
```

VL53_READ_REG payload: `addr u16, len u16` little-endian.

#### Vl53l7.packWriteReg

```java
public static byte[] packWriteReg(int addr, byte[] data)
```

VL53_WRITE_REG payload: `addr u16, data[1..2048]`.

#### Vl53l7.packStartStream

```java
public static byte[] packStartStream(int frameSize)
```

VL53_START_STREAM payload: `frame_size u16`.

#### Vl53l7.packPinCtrl

```java
public static byte[] packPinCtrl(int action)
```

VL53_PIN_CTRL payload: `action u8` (see `PinAction`).

#### Vl53l7.packPinCtrl

```java
public static byte[] packPinCtrl(PinAction action)
```

VL53_PIN_CTRL payload for a `PinAction`.

#### Vl53l7.packSetI2cSpeed

```java
public static byte[] packSetI2cSpeed(int khz)
```

VL53_SET_I2C_SPEED payload: `khz u16`.

### Vl53l7.Vl53l7Cmd

```java
public enum Vl53l7Cmd
```

#### Vl53l7.Vl53l7Cmd — constants

`READ_REG`(0x32), `WRITE_REG`(0x33), `PIN_CTRL`(0x34), `START_STREAM`(0x35), `STOP_STREAM`(0x36), `GET_INFO`(0x37), `SET_I2C_SPEED`(0x38)

#### Vl53l7.Vl53l7Cmd.value *(field)*

```java
public final int value
```

### Vl53l7.Vl53l7Rpt

```java
public enum Vl53l7Rpt
```

#### Vl53l7.Vl53l7Rpt — constants

`REG_DATA`(0x91), `VL53_INFO`(0x92), `VL53_FRAME`(0x93)

#### Vl53l7.Vl53l7Rpt.value *(field)*

```java
public final int value
```

### Vl53l7.PinAction

```java
public enum PinAction
```

VL53_PIN_CTRL actions. None is a true sensor reset (the board has no power
GPIO): after LPN_OFF or SOFT_CYCLE the host must re-run init().

#### Vl53l7.PinAction — constants

`LPN_OFF`(0), `LPN_ON`(1), `I2C_RST`(2), `SOFT_CYCLE`(3)

#### Vl53l7.PinAction.value *(field)*

```java
public final int value
```

### Vl53l7.Vl53l7Info

```java
public record Vl53l7Info(long intEdges, long framesDropped, long i2cErrors, int lastI2cError, int lpnLevel, int intLevel, int i2cKhz, int frameSize, boolean streaming)
```

RPT_VL53_INFO — bridge state only (the sensor is never probed), 20-byte LE
payload `<IIIBBBHHB`. All counters run from power-up / DEVICE_RESET;
SOFT_CYCLE clears the I2C ones. The report carries no echoed command byte.

#### Vl53l7.Vl53l7Info.unpack

```java
public static Vl53l7Info unpack(byte[] p)
```

### Vl53l7Uld

```java
public final class Vl53l7Uld
```

VL53L5CX / VL53L7CX / VL53L7CH ULD — the host-verifiable decode layer only
(contracts/11_SENSOR_VL53L7.md, a delta against contracts/04_SENSOR_VL53L8.md).

One firmware app (`APP_VL53L7_*`) serves three boards: VL53L7CX
(base), VL53L5CX (same API, different optics) and VL53L7CH (CX plus
CNH, exactly as VL53L8CH). Frames, CNH decode and the chunk transport are the
VL53L8 ones (`Vl53l8Uld`, `ai.depz.sensor.sensors.vl53l8.FrameReassembler`);
the L5/L7 differences in the decode path are:
- footer id at `size - 4` for every L5/L7 part (like L8 CH);
- per-target blocks (and CNH) keep their 64-entry size even in 4x4, so
every per-zone array is trimmed to the frame's resolution.

OUT OF SCOPE, as for VL53L8: the live ULD init/config driver over the I2C
bridge; see `liveDriverStubbed()`. Power modes, xtalk calibration,
detection thresholds and the motion indicator are the VL53L8 features with the
per-part deltas of contract 11 §4 (L5CX / L7CX: no threshold auto-stop, no
DEEP_SLEEP), checked against the ST sources and verified on hardware (L5CX and
L7CH, 2026-09-24); they run through that live driver (Python / TypeScript SDKs).

#### Vl53l7Uld.RESOLUTION_4X4 *(field)*

```java
public static final int RESOLUTION_4X4
```

#### Vl53l7Uld.RESOLUTION_8X8 *(field)*

```java
public static final int RESOLUTION_8X8
```

#### Vl53l7Uld.FOOTER_ID_OFF *(field)*

```java
public static final int FOOTER_ID_OFF = 4
```

Footer-id offset from the frame end: `size - 4` for L5CX, L7CX and L7CH.

#### Vl53l7Uld.MIN_RANGING_FREQUENCY_HZ *(field)*

```java
public static final int MIN_RANGING_FREQUENCY_HZ = 1
```

L5/L7 range and stream at 1 Hz (VL53L8: 2 Hz).

#### Vl53l7Uld.resolveModel

```java
public static Model resolveModel(String usbModel, String deviceName)
```

Resolve the sensor class of an `APP_VL53L7` board (contract 11 §1,
normative): the production USB PID model first (`usbModel`, e.g.
from `ai.depz.sensor.usb.UsbIds#usbModelHint`; may be `null`),
else the first `VL53L<5|7><CX|CH>` in `GET_DEVICE_NAME`, else
`Model#VL53L7CX` (its blob runs on every L5/L7 part).

#### Vl53l7Uld.liveDriverStubbed

```java
public static boolean liveDriverStubbed()
```

The live ULD init/config I2C-bridge driver is intentionally not ported to
Java — hardware-dependent and out of the decode scope (as for VL53L8).

#### Vl53l7Uld.parseFrame

```java
public static Vl53l8Uld.Results parseFrame(byte[] raw, int dataReadSize, int resolution)
```

Parse one raw L5/L7 results frame for a known resolution (the one
`start_ranging` used: 16 or 64). Footer at `size - 4`;
per-zone arrays trimmed to `resolution`. CNH (L7CH) lands in
`Vl53l8Uld.Results#cnhRaw`. Throws on a header/footer id mismatch.

#### Vl53l7Uld.parseFrame

```java
public static Vl53l8Uld.Results parseFrame(byte[] raw, int dataReadSize)
```

Parse one raw L5/L7 results frame, taking the resolution from the frame
itself: the zone-scaled ambient block (index 0x54D0) is always sized to the
resolution (contract 11 §3), so its entry count is the zone count.

### Vl53l7Uld.Model

```java
public enum Model
```

The three sensor classes an `APP_VL53L7` board opens as.

#### Vl53l7Uld.Model — constants

`VL53L5CX`, `VL53L7CX`, `VL53L7CH`

#### Vl53l7Uld.Model.label *(field)*

```java
public final String label
```

#### Vl53l7Uld.Model.fromLabel

```java
public static Model fromLabel(String label)
```

Model for a lowercase label (`"vl53l7ch"`), or `null`.

## VL53L0X / L1CX / L1CB / L3CX / L4CX (ToF)

### Vl53lx

```java
public final class Vl53lx
```

VL53L 1D-family register-bridge wire codecs, protocol v2.00
(contracts/12_SENSOR_VL53LX.md).

One firmware (`APP_VL53L0_4`) serves VL53L0X, VL53L1CX, VL53L1CB,
VL53L3CX, VL53L4CD and VL53L4CX. It is the VL53L4CD bridge of contract 10 with
the three sensor-specific facts moved to the host: the register-address width
(VL53_SET_ADDR_WIDTH, new), the interrupt-release writes (carried by
VL53_START_STREAM) and the boot handshake (no longer inside VL53_XSHUT).
READ_REG, WRITE_REG, XSHUT, STOP_STREAM, SET_I2C_SPEED and the REG_DATA /
STREAM reports are the contract-10 codecs of `Vl53l4`, re-exposed here.

#### Vl53lx.CLEAR_STEPS_MAX *(field)*

```java
public static final int CLEAR_STEPS_MAX = 4
```

Interrupt-release steps a stream may carry (VL53_CLEAR_STEPS_WIRE_MAX).

#### Vl53lx.INFO_SIZE *(field)*

```java
public static final int INFO_SIZE = 23
```

RPT_VL53_INFO payload size (v2.00).

#### Vl53lx.XFER_MAX *(field)*

```java
public static final int XFER_MAX
```

Same as contract 10.

#### Vl53lx.XSHUT_OFF *(field)*

```java
public static final int XSHUT_OFF
```

#### Vl53lx.XSHUT_ON *(field)*

```java
public static final int XSHUT_ON
```

#### Vl53lx.XSHUT_RESET *(field)*

```java
public static final int XSHUT_RESET
```

1 ms low + a fixed 5 ms wait; v2.00 has no boot handshake — the host polls.

#### Vl53lx.SF_INT_ACT_HIGH *(field)*

```java
public static final int SF_INT_ACT_HIGH
```

#### Vl53lx.packReadReg

```java
public static byte[] packReadReg(int addr, int len)
```

VL53_READ_REG payload (contract 10). At width 1 only the low byte of addr goes on the bus.

#### Vl53lx.packWriteReg

```java
public static byte[] packWriteReg(int addr, byte[] data)
```

VL53_WRITE_REG payload (contract 10).

#### Vl53lx.packXshut

```java
public static byte[] packXshut(int action)
```

VL53_XSHUT payload (contract 10 codec; RESET semantics changed, see `XSHUT_RESET`).

#### Vl53lx.packSetI2cSpeed

```java
public static byte[] packSetI2cSpeed(int khz)
```

VL53_SET_I2C_SPEED payload (contract 10).

#### Vl53lx.packStartStream

```java
public static byte[] packStartStream(int addr, int len, int[][] clear, int flags)
```

VL53_START_STREAM payload, `6 + 3n` bytes: `addr u16, len u16,
flags u8, n_clear u8, clear[n] {addr u16, value u8}`. `clear` holds
`{addr, value}` pairs the bridge writes after every block read
(0..{@value #CLEAR_STEPS_MAX} steps); more throws
`IllegalArgumentException`.

#### Vl53lx.packSetAddrWidth

```java
public static byte[] packSetAddrWidth(int width)
```

VL53_SET_ADDR_WIDTH payload: `width u8`, 1 or 2 (else `IllegalArgumentException`).

#### Vl53lx.unpackRegData

```java
public static Vl53l4.RegData unpackRegData(byte[] p)
```

RPT_VL53_REG_DATA — contract 10 unchanged.

#### Vl53lx.unpackStreamData

```java
public static Vl53l4.StreamData unpackStreamData(byte[] p)
```

RPT_VL53_STREAM — contract 10 unchanged.

### Vl53lx.Vl53lxCmd

```java
public enum Vl53lxCmd
```

#### Vl53lx.Vl53lxCmd — constants

`READ_REG`(0x32), `WRITE_REG`(0x33), `XSHUT`(0x34), `START_STREAM`(0x35), `STOP_STREAM`(0x36), `GET_INFO`(0x37), `SET_I2C_SPEED`(0x38), `SET_ADDR_WIDTH`(0x39), `CLEAR_I2C_ERRORS`(0x3A)

#### Vl53lx.Vl53lxCmd.value *(field)*

```java
public final int value
```

### Vl53lx.Vl53lxRpt

```java
public enum Vl53lxRpt
```

#### Vl53lx.Vl53lxRpt — constants

`REG_DATA`(0x91), `INFO`(0x92), `STREAM`(0x93)

#### Vl53lx.Vl53lxRpt.value *(field)*

```java
public final int value
```

### Vl53lx.Vl53lxInfo

```java
public record Vl53lxInfo(long intEdges, long slotsSkipped, long i2cErrors, int lastI2cError, int xshutLevel, int intLevel, int i2cKhz, int addrWidth, int nClear, long framesDropped)
```

RPT_VL53_INFO (v2.00, 23-byte LE payload `<IIIBBBHBBI`) — bridge
state only; the bridge reads no sensor register. Counters are free-running
(wrap silently): watch increments. `slotsSkipped`, `framesDropped`
and the fault latch reset at START_STREAM; `i2cErrors` is free-running.
`lastI2cError`: 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR.

#### Vl53lx.Vl53lxInfo.unpack

```java
public static Vl53lxInfo unpack(byte[] p)
```

### Vl53lxDecode

```java
public final class Vl53lxDecode
```

Stateless decoders of the blocks the 1D-family bridge streams
(contracts/12_SENSOR_VL53LX.md §4) — the "base" every SDK implements. Mirrors
`depz_sensor_sdk/vl53lx/decode.py` 1:1.
- `decodeDieBlock` — the 17-byte VL53L1-die result block at 0x0089
(L1CX, L1CB, L3CX, L4CD, L4CX light drivers), fully decoded. It is
contract 10 §4's decode (`Vl53l4Uld#parseResultBlock`) parameterised
by the ULD that reads it (`DieVariant`).
- `decodeL0xRaw` — raw fields of the VL53L0X 12-byte block at 0x14.
The PAL range status, sigma and dmax need the device data cached by init
(full driver, not base).
- `decodeHistogramRaw` — status bytes and the 24 photon bins of the
83-byte histogram block at 0x0088. Bins → targets is the full driver.

OUT OF SCOPE (as for every Java sensor): the live ULD / Bare-Driver host
drivers over the bridge.

#### Vl53lxDecode.DIE_BLOCK_ADDR *(field)*

```java
public static final int DIE_BLOCK_ADDR = 0x0089
```

#### Vl53lxDecode.DIE_BLOCK_LEN *(field)*

```java
public static final int DIE_BLOCK_LEN = 17
```

#### Vl53lxDecode.L0X_BLOCK_ADDR *(field)*

```java
public static final int L0X_BLOCK_ADDR = 0x14
```

#### Vl53lxDecode.L0X_BLOCK_LEN *(field)*

```java
public static final int L0X_BLOCK_LEN = 12
```

#### Vl53lxDecode.HISTOGRAM_BLOCK_ADDR *(field)*

```java
public static final int HISTOGRAM_BLOCK_ADDR = 0x0088
```

#### Vl53lxDecode.RESULT_HISTOGRAM_BIN_0_2 *(field)*

```java
public static final int RESULT_HISTOGRAM_BIN_0_2 = 0x008E
```

#### Vl53lxDecode.RESULT_HISTOGRAM_BIN_23_0 *(field)*

```java
public static final int RESULT_HISTOGRAM_BIN_23_0 = 0x00D5
```

#### Vl53lxDecode.PHASECAL_RESULT_REFERENCE_PHASE *(field)*

```java
public static final int PHASECAL_RESULT_REFERENCE_PHASE = 0x00D6
```

#### Vl53lxDecode.PHASECAL_RESULT_VCSEL_START *(field)*

```java
public static final int PHASECAL_RESULT_VCSEL_START = 0x00D8
```

#### Vl53lxDecode.RESULT_HISTOGRAM_BIN_23_0_MSB *(field)*

```java
public static final int RESULT_HISTOGRAM_BIN_23_0_MSB = 0x00D9
```

#### Vl53lxDecode.RESULT_HISTOGRAM_BIN_23_0_LSB *(field)*

```java
public static final int RESULT_HISTOGRAM_BIN_23_0_LSB = 0x00DA
```

#### Vl53lxDecode.HISTOGRAM_BLOCK_LEN *(field)*

```java
public static final int HISTOGRAM_BLOCK_LEN
```

#### Vl53lxDecode.HISTOGRAM_BINS *(field)*

```java
public static final int HISTOGRAM_BINS = 24
```

#### Vl53lxDecode.decodeDieBlock

```java
public static Vl53l4Uld.Results decodeDieBlock(byte[] raw, DieVariant variant)
```

The 17-byte die block (0x0089..0x0099) as the named ULD reads it. The result
shape is contract 10's `Vl53l4Uld.Results`; the status goes through the
same `Vl53l4Uld#STATUS_RTN`. Throws `IllegalArgumentException`
when `raw` is shorter than {@value #DIE_BLOCK_LEN} bytes.

#### Vl53lxDecode.decodeDieBlock

```java
public static Vl53l4Uld.Results decodeDieBlock(byte[] raw, String variant)
```

`decodeDieBlock(byte[], DieVariant)` with the variant by label (`"l4"`/`"l1"`).

#### Vl53lxDecode.decodeL0xRaw

```java
public static L0xRaw decodeL0xRaw(byte[] raw)
```

VL53L0X_GetRangingMeasurementData before the PAL status/sigma step.

#### Vl53lxDecode.decodeHistogramRaw

```java
public static HistogramRaw decodeHistogramRaw(byte[] raw)
```

The 83-byte histogram block at 0x0088: status bytes and the 24 bins of 3
big-endian bytes each. Bin 23's low byte is carried in a separate MSB/LSB
pair — `(MSB << 2) + LSB`, truncated to 8 bits — and is patched in
before the bins are read. `raw` is not modified.

### Vl53lxDecode.DieVariant

```java
public enum DieVariant
```

The two ULDs that read the die block: signal-rate byte offset and per-SPAD
scale K. `l4`: VL53L4CD ULD — also the L3CX ULP and L4CX-as-L4CD;
`l1`: VL53L1X ULD (crosstalk-corrected peak signal at 0x0098, K = 25).

#### Vl53lxDecode.DieVariant — constants

`L4`(, 5, 256), `L1`(, 15, 25)

#### Vl53lxDecode.DieVariant.label *(field)*

```java
public final String label
```

#### Vl53lxDecode.DieVariant.signalOffset *(field)*

```java
public final int signalOffset
```

#### Vl53lxDecode.DieVariant.perSpadK *(field)*

```java
public final int perSpadK
```

#### Vl53lxDecode.DieVariant.fromLabel

```java
public static DieVariant fromLabel(String label)
```

Variant for `"l4"` / `"l1"`; throws `IllegalArgumentException` otherwise.

### Vl53lxDecode.L0xRaw

```java
public record L0xRaw(int distanceRaw, int deviceRangeStatus, int signalRateMcps1616, int ambientRateMcps1616, int effectiveSpadCount88)
```

Raw fields of the VL53L0X block at 0x14.

- `distanceRaw` — mm (quarter-mm when RangeFractionalEnable, off by default)
- `deviceRangeStatus` — raw byte 0; the PAL status needs the init state
- `signalRateMcps1616` — FixPoint16.16 Mcps (9.7 on the wire `<< 9`)
- `ambientRateMcps1616` — FixPoint16.16 Mcps
- `effectiveSpadCount88` — 8.8

### Vl53lxDecode.HistogramRaw

```java
public record HistogramRaw(int interruptStatus, int rangeStatus, int reportStatus, int streamCount, int dssActualEffectiveSpads, int referencePhase, int vcselStart, int[] bins)
```

Status bytes and the 24 photon counts of the histogram block.

### Vl53lxProducts

```java
public final class Vl53lxProducts
```

The VL53L 1D-family product table and class resolution
(contracts/12_SENSOR_VL53LX.md §1, §3; Python reference
`depz_sensor_sdk/vl53lx/uld/registry.py` and
`discovery.py::_vl53lx_class`).

Two independent axes: the product (whose parameter set to load —
normally what is soldered on, but naming a neighbour borrows its driver) and
the driver kind (`uld`, `ulp`, `histogram`). A missing
pair is a refusal, never a fallback. The model id is a cross-check only:
L1CX/L1CB share `0xEACC`, L4CD/L4CX share `0xEBAA`.

Java carries no live driver (as for every other sensor); each pair is
described by the bridge parameters its driver hands the firmware
(`Bridge`).

#### Vl53lxProducts.DRIVER_KINDS *(field)*

```java
public static final List<String> DRIVER_KINDS
```

Driver kinds, in the order a UI should list them.

#### Vl53lxProducts.PRODUCTS *(field)*

```java
public static final List<String> PRODUCTS
```

Every product of the family, in the order a UI should list them.

#### Vl53lxProducts.L0X_ULD *(field)*

```java
public static final Bridge L0X_ULD
```

VL53L0X ULD: 12 B at 0x14, width 1, 0x0B←1 then 0x0B←0, 400 kHz (no FM+ pad).

#### Vl53lxProducts.DIE_ULD *(field)*

```java
public static final Bridge DIE_ULD
```

Die ULD/ULP (L1CX, L1CB, L3CX, L4CD): 17 B at 0x0089, 0x0086←1, 1 MHz.

#### Vl53lxProducts.HISTOGRAM *(field)*

```java
public static final Bridge HISTOGRAM
```

Histogram (ST Bare Driver): 83 B at 0x0088, 0x0086←1, 1 MHz.

#### Vl53lxProducts.TABLE *(field)*

```java
public static final Map<String, Product> TABLE
```

The product table (contract 12 §1), keyed by product name, in `PRODUCTS` order.

#### Vl53lxProducts.product

```java
public static Product product(String name)
```

The table row; throws `UnsupportedOperationException` for a product nobody serves.

#### Vl53lxProducts.productFromBoardName

```java
public static String productFromBoardName(String name)
```

`"ToF Sensor VL53L4CD USB v2.1"` → `"VL53L4CD"`: the first
`VL53L<digit><part>` match, case-insensitive. `null` if the name
carries no product this table serves (an unstamped board, an unknown part).

#### Vl53lxProducts.resolveClass

```java
public static SensorClass resolveClass(String usbModel, String deviceName)
```

Class resolution (contract 12 §1, pinned by `vl53lx.json model`): the
production PID model (`usbModel`, e.g. `"vl53l0x"`; may be
`null`) if it is a family product, else the product the device name
carries, else the generic class.

### Vl53lxProducts.Bridge

```java
public record Bridge(int addrWidth, int[][] clearSteps, int maxKhz, int blockAddr, int blockLen)
```

What one driver tells the bridge (contract 12 §1, §3): register-address
width, the interrupt-release writes `{addr, value}` played after each
block read, the bus ceiling after init, and the streamed block.

### Vl53lxProducts.Product

```java
public record Product(String name, int modelId, int reachMm, String defaultDriver, Map<String, Bridge> drivers)
```

One row of the table. `drivers` maps a kind from `DRIVER_KINDS`
to its bridge parameters; an absent kind is absent on purpose.
`reachMm` is the datasheet rating of the module.

#### Vl53lxProducts.Product.driverKinds

```java
public List<String> driverKinds()
```

Driver kinds this product has, in `DRIVER_KINDS` order.

#### Vl53lxProducts.Product.bridge

```java
public Bridge bridge(String kind)
```

Bridge parameters of one pair; throws `UnsupportedOperationException` for a missing pair.

#### Vl53lxProducts.Product.defaultBridge

```java
public Bridge defaultBridge()
```

Bridge parameters of the default driver.

#### Vl53lxProducts.Product.modelIdOk

```java
public boolean modelIdOk(int value)
```

"Not something else entirely" — pairs sharing an id cannot be told apart.

### Vl53lxProducts.SensorClass

```java
public enum SensorClass
```

The sensor class an `APP_VL53L0_4` board opens as (the Python/TS
class names). `VL53LX` is the generic class that takes the product
at init — also what a VL53L4CD board on this firmware uses.

#### Vl53lxProducts.SensorClass — constants

`VL53L0X`(,), `VL53L1CX`(,), `VL53L1CB`(,), `VL53L3CX`(,), `VL53L4CX`(,), `VL53LX`(, null)

#### Vl53lxProducts.SensorClass.className *(field)*

```java
public final String className
```

#### Vl53lxProducts.SensorClass.product *(field)*

```java
public final String product
```

The product the class is fixed to, `null` for the generic class.

## BNO086 (IMU)

### Reports

```java
public final class Reports
```

SH-2 input-report catalog and parsers (contracts/05_SENSOR_BNO086.md §5).

Raw wire integers are authoritative (this decode layer surfaces them as
`*_raw` fields). Q-point scaling is derived downstream. Timestamps:
channel-3/4 cargos start with a Base Timestamp Reference (0xFB, i32 base
delta in 100 µs ticks, subtracted from the bridge capture time), 0xFA
rebases add; each report adds its own 14-bit delay (status bits 7:2 upper,
byte 3 lower; 100 µs). timestampUs = capture − baseDelta·100 + delay·100.

Mirror of the TS/Python `reports` references.

#### Reports.ACCELEROMETER *(field)*

```java
public static final int ACCELEROMETER = 0x01
```

#### Reports.GYROSCOPE *(field)*

```java
public static final int GYROSCOPE = 0x02
```

#### Reports.MAGNETOMETER *(field)*

```java
public static final int MAGNETOMETER = 0x03
```

#### Reports.LINEAR_ACCELERATION *(field)*

```java
public static final int LINEAR_ACCELERATION = 0x04
```

#### Reports.ROTATION_VECTOR *(field)*

```java
public static final int ROTATION_VECTOR = 0x05
```

#### Reports.GRAVITY *(field)*

```java
public static final int GRAVITY = 0x06
```

#### Reports.UNCAL_GYROSCOPE *(field)*

```java
public static final int UNCAL_GYROSCOPE = 0x07
```

#### Reports.GAME_ROTATION_VECTOR *(field)*

```java
public static final int GAME_ROTATION_VECTOR = 0x08
```

#### Reports.GEOMAG_ROTATION_VECTOR *(field)*

```java
public static final int GEOMAG_ROTATION_VECTOR = 0x09
```

#### Reports.PRESSURE *(field)*

```java
public static final int PRESSURE = 0x0a
```

#### Reports.AMBIENT_LIGHT *(field)*

```java
public static final int AMBIENT_LIGHT = 0x0b
```

#### Reports.HUMIDITY *(field)*

```java
public static final int HUMIDITY = 0x0c
```

#### Reports.PROXIMITY *(field)*

```java
public static final int PROXIMITY = 0x0d
```

#### Reports.TEMPERATURE *(field)*

```java
public static final int TEMPERATURE = 0x0e
```

#### Reports.UNCAL_MAGNETOMETER *(field)*

```java
public static final int UNCAL_MAGNETOMETER = 0x0f
```

#### Reports.TAP_DETECTOR *(field)*

```java
public static final int TAP_DETECTOR = 0x10
```

#### Reports.STEP_COUNTER *(field)*

```java
public static final int STEP_COUNTER = 0x11
```

#### Reports.SIGNIFICANT_MOTION *(field)*

```java
public static final int SIGNIFICANT_MOTION = 0x12
```

#### Reports.STABILITY_CLASSIFIER *(field)*

```java
public static final int STABILITY_CLASSIFIER = 0x13
```

#### Reports.RAW_ACCELEROMETER *(field)*

```java
public static final int RAW_ACCELEROMETER = 0x14
```

#### Reports.RAW_GYROSCOPE *(field)*

```java
public static final int RAW_GYROSCOPE = 0x15
```

#### Reports.RAW_MAGNETOMETER *(field)*

```java
public static final int RAW_MAGNETOMETER = 0x16
```

#### Reports.STEP_DETECTOR *(field)*

```java
public static final int STEP_DETECTOR = 0x18
```

#### Reports.SHAKE_DETECTOR *(field)*

```java
public static final int SHAKE_DETECTOR = 0x19
```

#### Reports.PERSONAL_ACTIVITY_CLASSIFIER *(field)*

```java
public static final int PERSONAL_ACTIVITY_CLASSIFIER = 0x1e
```

#### Reports.ARVR_STABILIZED_RV *(field)*

```java
public static final int ARVR_STABILIZED_RV = 0x28
```

#### Reports.ARVR_STABILIZED_GAME_RV *(field)*

```java
public static final int ARVR_STABILIZED_GAME_RV = 0x29
```

#### Reports.GYRO_INTEGRATED_RV *(field)*

```java
public static final int GYRO_INTEGRATED_RV = 0x2a
```

#### Reports.BASE_TIMESTAMP_REF *(field)*

```java
public static final int BASE_TIMESTAMP_REF = 0xfb
```

#### Reports.TIMESTAMP_REBASE *(field)*

```java
public static final int TIMESTAMP_REBASE = 0xfa
```

#### Reports.parseInputCargo

```java
public static List<Report> parseInputCargo(byte[] payload, long captureTimestampUs)
```

Parse a channel-3/4 cargo into typed reports. `captureTimestampUs`
is the bridge RPT_DATA capture time (MCU uptime).

#### Reports.parseGyroRvCargo

```java
public static Report parseGyroRvCargo(byte[] payload, long captureTimestampUs)
```

Parse a channel-5 cargo (gyro-integrated RV, dense format). Two shapes:
7×i16 bare, or prefixed with 0xFB + i32 base delta + u16 delay. Returns
`null` on a too-short buffer.

### Reports.Report

```java
public record Report(String type, Map<String, Object> fields)
```

One decoded report: a `type` tag plus the field map (raw-integer).

### Sh2

```java
public final class Sh2
```

SH-2 control-channel encoders (contracts/05_SENSOR_BNO086.md §6). Pure
codecs; mirror of the TS/Python `sh2` references. Report/response
decoders that require a live hub are out of the verifiable-vector scope.

#### Sh2.SET_FEATURE_COMMAND *(field)*

```java
public static final int SET_FEATURE_COMMAND = 0xfd
```

#### Sh2.GET_FEATURE_REQUEST *(field)*

```java
public static final int GET_FEATURE_REQUEST = 0xfe
```

#### Sh2.GET_FEATURE_RESPONSE *(field)*

```java
public static final int GET_FEATURE_RESPONSE = 0xfc
```

#### Sh2.PRODUCT_ID_REQUEST *(field)*

```java
public static final int PRODUCT_ID_REQUEST = 0xf9
```

#### Sh2.PRODUCT_ID_RESPONSE *(field)*

```java
public static final int PRODUCT_ID_RESPONSE = 0xf8
```

#### Sh2.COMMAND_REQUEST *(field)*

```java
public static final int COMMAND_REQUEST = 0xf2
```

#### Sh2.COMMAND_RESPONSE *(field)*

```java
public static final int COMMAND_RESPONSE = 0xf1
```

#### Sh2.FRS_READ_REQUEST *(field)*

```java
public static final int FRS_READ_REQUEST = 0xf4
```

#### Sh2.FRS_READ_RESPONSE *(field)*

```java
public static final int FRS_READ_RESPONSE = 0xf3
```

#### Sh2.FRS_WRITE_REQUEST *(field)*

```java
public static final int FRS_WRITE_REQUEST = 0xf7
```

#### Sh2.FRS_WRITE_DATA *(field)*

```java
public static final int FRS_WRITE_DATA = 0xf6
```

#### Sh2.FRS_WRITE_RESPONSE *(field)*

```java
public static final int FRS_WRITE_RESPONSE = 0xf5
```

#### Sh2.buildSetFeature

```java
public static byte[] buildSetFeature(int sensorId, long intervalUs, long batchUs, int sensitivity, int flags, long cfgWord)
```

Set Feature Command (0xFD), 17 bytes. intervalUs = 0 disables.

#### Sh2.buildGetFeatureRequest

```java
public static byte[] buildGetFeatureRequest(int sensorId)
```

Get Feature Request (0xFE), 2 bytes.

#### Sh2.buildProductIdRequest

```java
public static byte[] buildProductIdRequest()
```

Product ID Request (0xF9), 2 bytes.

#### Sh2.buildCommandRequest

```java
public static byte[] buildCommandRequest(int seq, int command, byte[] params)
```

Command Request (0xF2), 12 bytes: id, seq, command, P0..P8.

#### Sh2.buildFrsReadRequest

```java
public static byte[] buildFrsReadRequest(int frsType, int offsetWords, int blockWords)
```

FRS Read Request (0xF4), 8 bytes. blockWords = 0 reads the record.

#### Sh2.buildFrsWriteRequest

```java
public static byte[] buildFrsWriteRequest(int frsType, int lengthWords)
```

FRS Write Request (0xF7), 6 bytes. lengthWords = 0 erases the record.

#### Sh2.buildFrsWriteData

```java
public static byte[] buildFrsWriteData(int offsetWords, long[] words)
```

FRS Write Data (0xF6), 12 bytes; 1 or 2 words per packet.

### Shtp

```java
public final class Shtp
```

SHTP framing layer for the BNO086 (contracts/05_SENSOR_BNO086.md §3).

Header: length u16 LE (bits 14:0 = cargo length incl. the 4-byte header;
bit 15 = continuation), channel u8, seq u8. A first fragment advertises the
TOTAL cargo length; continuations carry the remaining length with bit 15 set.
TX seq counters are per channel. Mirror of the TS/Python references.

#### Shtp.SHTP_HEADER_SIZE *(field)*

```java
public static final int SHTP_HEADER_SIZE = 4
```

#### Shtp.LENGTH_MASK *(field)*

```java
public static final int LENGTH_MASK = 0x7fff
```

#### Shtp.CONTINUATION_BIT *(field)*

```java
public static final int CONTINUATION_BIT = 0x8000
```

#### Shtp.NUM_CHANNELS *(field)*

```java
public static final int NUM_CHANNELS = 6
```

#### Shtp.MAX_TX_FRAME *(field)*

```java
public static final int MAX_TX_FRAME = 64
```

Host->sensor frames must fit one MCU transmit slot (ERRATA E2).

#### Shtp.packShtpHeader

```java
public static byte[] packShtpHeader(ShtpHeader hdr)
```

#### Shtp.unpackShtpHeader

```java
public static ShtpHeader unpackShtpHeader(byte[] data)
```

#### Shtp.buildFrame

```java
public static byte[] buildFrame(int channel, byte[] payload, int seq)
```

Single-fragment frame: length = header + payload.

#### Shtp.reassemble

```java
public static List<ShtpCargo> reassemble(List<byte[]> frames, int[] discardedOut)
```

Convenience for tests/consumers: reassemble a list of frames.

### Shtp.ShtpHeader

```java
public record ShtpHeader(int length, int channel, int seq, boolean continuation)
```

### Shtp.ShtpCargo

```java
public record ShtpCargo(int channel, int seq, byte[] payload)
```

One reassembled cargo: `payload` excludes all SHTP headers.

### Shtp.ShtpLayer

```java
public static final class ShtpLayer
```

Per-channel TX sequence counters + RX cargo reassembly.

#### Shtp.ShtpLayer.discarded *(field)*

```java
public int discarded = 0
```

Incomplete cargos thrown away.

#### Shtp.ShtpLayer.nextFrame

```java
public byte[] nextFrame(int channel, byte[] payload)
```

Build a single-fragment frame, consuming the channel's TX seq.

#### Shtp.ShtpLayer.txSeq

```java
public int txSeq(int channel)
```

#### Shtp.ShtpLayer.feed

```java
public ShtpCargo feed(byte[] frame)
```

Consume one inbound frame; return the cargo when complete, else null.

#### Shtp.ShtpLayer.reset

```java
public void reset()
```

Forget all TX seq counters and partial cargos (sensor reset).

## BNO055 (IMU)

### Bno055

```java
public final class Bno055
```

BNO055 register-bridge wire codecs, protocol v0.10
(contracts/13_SENSOR_BNO055.md §2–§3).

The firmware (`APP_BNO055`) is a thin register bridge: the MCU owns
the I2C bus (sensor at 7-bit 0x28, 400 kHz fixed), the nRESET/INT pins and one
streaming loop. Everything else — operating mode, units, axis remap,
calibration, decoding — is host logic expressed as register access
(`ai.depz.sensor.sensors.bno055.Bno055Regs`). Mirrors
`depz_sensor_sdk/protocol/bno055.py` 1:1.

#### Bno055.XFER_MAX *(field)*

```java
public static final int XFER_MAX = 128
```

Max bytes per READ_REG / WRITE_REG / streamed block; `addr + len ≤ 0x100`.

#### Bno055.INFO_SIZE *(field)*

```java
public static final int INFO_SIZE = 38
```

RPT_BNO_INFO payload size.

#### Bno055.TRIGGER_TIMER *(field)*

```java
public static final int TRIGGER_TIMER = 0
```

BNO_START_STREAM trigger: read every `period_ms` (the only data trigger on SW rev 03.11).

#### Bno055.TRIGGER_INT *(field)*

```java
public static final int TRIGGER_INT = 1
```

BNO_START_STREAM trigger: read on the INT rising edge; `period_ms` is a missed-edge watchdog.

#### Bno055.RESET_TIMEOUT_S *(field)*

```java
public static final double RESET_TIMEOUT_S
```

BNO_RESET answers after the sensor's ~0.5 s boot handshake.

#### Bno055.EXPECTED_CHIP_ID *(field)*

```java
public static final int EXPECTED_CHIP_ID = 0xA0
```

Identity registers 0x00..0x03 of a healthy BNO055.

#### Bno055.EXPECTED_ACC_ID *(field)*

```java
public static final int EXPECTED_ACC_ID = 0xFB
```

#### Bno055.EXPECTED_MAG_ID *(field)*

```java
public static final int EXPECTED_MAG_ID = 0x32
```

#### Bno055.EXPECTED_GYR_ID *(field)*

```java
public static final int EXPECTED_GYR_ID = 0x0F
```

#### Bno055.I2C_ERROR_NAMES *(field)*

```java
public static final String[] I2C_ERROR_NAMES
```

`last_i2c_error` values in RPT_BNO_INFO, indexed by code.

#### Bno055.packReadReg

```java
public static byte[] packReadReg(int addr, int len)
```

BNO_READ_REG payload: `addr u8, len u8`.

#### Bno055.packWriteReg

```java
public static byte[] packWriteReg(int addr, byte[] data)
```

BNO_WRITE_REG payload: `addr u8, data[1..128]`.

#### Bno055.packStartStream

```java
public static byte[] packStartStream(int trigger, int addr, int len, int periodMs)
```

BNO_START_STREAM payload, 5 bytes: `trigger u8, addr u8, len u8,
period_ms u16 LE`. Replaces any running stream.

### Bno055.Bno055Cmd

```java
public enum Bno055Cmd
```

#### Bno055.Bno055Cmd — constants

`READ_REG`(0x32), `WRITE_REG`(0x33), `RESET`(0x34), `START_STREAM`(0x35), `STOP_STREAM`(0x36), `GET_INFO`(0x37)

#### Bno055.Bno055Cmd.value *(field)*

```java
public final int value
```

### Bno055.Bno055Rpt

```java
public enum Bno055Rpt
```

#### Bno055.Bno055Rpt — constants

`REG_DATA`(0x91), `INFO`(0x92), `STREAM`(0x93)

#### Bno055.Bno055Rpt.value *(field)*

```java
public final int value
```

### Bno055.RegData

```java
public record RegData(int cmd, long timestampUs, byte[] data)
```

RPT_BNO_REG_DATA: `cmd u8` (echoed READ_REG opcode), `timestamp_us u64`, data.

#### Bno055.RegData.unpack

```java
public static RegData unpack(byte[] p)
```

### Bno055.StreamData

```java
public record StreamData(long timestampUs, int addr, int len, byte[] data)
```

RPT_BNO_REG_STREAM — one streamed register block. `addr`/`len`
echo the stream configuration so each report is self-describing;
`timestampUs` is the trigger time (timer expiry or INT edge).

#### Bno055.StreamData.unpack

```java
public static StreamData unpack(byte[] p)
```

### Bno055.Bno055Info

```java
public record Bno055Info(int i2cAddr, int chipId, int accId, int magId, int gyrId, int swRev, int blRev, int initialized, int intLevel, long intEdges, int readMinUs, int readMaxUs, int readAvgUs, long txDropped, long i2cErrors, long slotsSkipped, int busRecoveries, int lastI2cError, int sensorResets, int loopMaxUs)
```

RPT_BNO_INFO (38-byte LE payload `<BBBBBHBBBIHHHIIIHBBH`) — sensor
identity (registers 0x00..0x06) plus bridge diagnostics. Counters are
free-running and wrap silently; watch increments. `read*Us`,
`slotsSkipped` and `loopMaxUs` reset at START_STREAM. A rising
`sensorResets` means the bridge pulsed nRESET to recover the bus: the
sensor is back in CONFIG and the host must restore its configuration.
`swRev` is BCD (`0x0311` = 03.11).

#### Bno055.Bno055Info.unpack

```java
public static Bno055Info unpack(byte[] p)
```

#### Bno055.Bno055Info.idsOk

```java
public boolean idsOk()
```

All four identity registers hold the healthy BNO055 values.

#### Bno055.Bno055Info.swRevText

```java
public String swRevText()
```

Sensor firmware revision as Bosch writes it: 0x0311 → "03.11".

### Bno055Regs

```java
public final class Bno055Regs
```

BNO055 register map and the pure codecs every SDK shares
(contracts/13_SENSOR_BNO055.md §4, Bosch BST-BNO055-DS000 rev 1.8) — the
"base" every SDK implements. Mirrors `depz_sensor_sdk/bno055/regs.py` 1:1.

Nothing here touches the wire: these functions turn register bytes into
values and back, so they are what `vectors/bno055.json` pins. Scaling
raw integers to physical units is `raw / LSB` with the LSB constants of
`Units` and the fixed `MAG_LSB` / `QUAT_LSB` /
`FUSION_ACCEL_LSB`.

OUT OF SCOPE (as for every Java sensor): the live driver over the bridge
(mode switching, boot / fusion-start polls, page discipline, self-test).

#### Bno055Regs.REG_CHIP_ID *(field)*

```java
public static final int REG_CHIP_ID = 0x00
```

#### Bno055Regs.REG_PAGE_ID *(field)*

```java
public static final int REG_PAGE_ID = 0x07
```

#### Bno055Regs.REG_ACC_DATA *(field)*

```java
public static final int REG_ACC_DATA = 0x08
```

#### Bno055Regs.REG_MAG_DATA *(field)*

```java
public static final int REG_MAG_DATA = 0x0E
```

#### Bno055Regs.REG_GYR_DATA *(field)*

```java
public static final int REG_GYR_DATA = 0x14
```

#### Bno055Regs.REG_EUL_DATA *(field)*

```java
public static final int REG_EUL_DATA = 0x1A
```

#### Bno055Regs.REG_QUA_DATA *(field)*

```java
public static final int REG_QUA_DATA = 0x20
```

#### Bno055Regs.REG_LIA_DATA *(field)*

```java
public static final int REG_LIA_DATA = 0x28
```

#### Bno055Regs.REG_GRV_DATA *(field)*

```java
public static final int REG_GRV_DATA = 0x2E
```

#### Bno055Regs.REG_TEMP *(field)*

```java
public static final int REG_TEMP = 0x34
```

#### Bno055Regs.REG_CALIB_STAT *(field)*

```java
public static final int REG_CALIB_STAT = 0x35
```

#### Bno055Regs.REG_ST_RESULT *(field)*

```java
public static final int REG_ST_RESULT = 0x36
```

#### Bno055Regs.REG_INT_STA *(field)*

```java
public static final int REG_INT_STA = 0x37
```

Clear-on-read — never part of a routine block read.

#### Bno055Regs.REG_SYS_CLK_STATUS *(field)*

```java
public static final int REG_SYS_CLK_STATUS = 0x38
```

#### Bno055Regs.REG_SYS_STATUS *(field)*

```java
public static final int REG_SYS_STATUS = 0x39
```

#### Bno055Regs.REG_SYS_ERR *(field)*

```java
public static final int REG_SYS_ERR = 0x3A
```

#### Bno055Regs.REG_UNIT_SEL *(field)*

```java
public static final int REG_UNIT_SEL = 0x3B
```

#### Bno055Regs.REG_OPR_MODE *(field)*

```java
public static final int REG_OPR_MODE = 0x3D
```

#### Bno055Regs.REG_PWR_MODE *(field)*

```java
public static final int REG_PWR_MODE = 0x3E
```

#### Bno055Regs.REG_SYS_TRIGGER *(field)*

```java
public static final int REG_SYS_TRIGGER = 0x3F
```

#### Bno055Regs.REG_TEMP_SOURCE *(field)*

```java
public static final int REG_TEMP_SOURCE = 0x40
```

#### Bno055Regs.REG_AXIS_MAP_CONFIG *(field)*

```java
public static final int REG_AXIS_MAP_CONFIG = 0x41
```

#### Bno055Regs.REG_AXIS_MAP_SIGN *(field)*

```java
public static final int REG_AXIS_MAP_SIGN = 0x42
```

#### Bno055Regs.REG_SIC_MATRIX *(field)*

```java
public static final int REG_SIC_MATRIX = 0x43
```

9 × i16, row-major, 1.0 = 16384.

#### Bno055Regs.REG_CALIB_PROFILE *(field)*

```java
public static final int REG_CALIB_PROFILE = 0x55
```

acc/mag/gyr offsets + acc/mag radius, {@value #CALIB_PROFILE_LEN} bytes.

#### Bno055Regs.CALIB_PROFILE_LEN *(field)*

```java
public static final int CALIB_PROFILE_LEN = 22
```

#### Bno055Regs.REG1_ACC_CONFIG *(field)*

```java
public static final int REG1_ACC_CONFIG = 0x08
```

#### Bno055Regs.REG1_MAG_CONFIG *(field)*

```java
public static final int REG1_MAG_CONFIG = 0x09
```

#### Bno055Regs.REG1_GYR_CONFIG_0 *(field)*

```java
public static final int REG1_GYR_CONFIG_0 = 0x0A
```

#### Bno055Regs.REG1_GYR_CONFIG_1 *(field)*

```java
public static final int REG1_GYR_CONFIG_1 = 0x0B
```

#### Bno055Regs.REG1_ACC_SLEEP_CONFIG *(field)*

```java
public static final int REG1_ACC_SLEEP_CONFIG = 0x0C
```

#### Bno055Regs.REG1_GYR_SLEEP_CONFIG *(field)*

```java
public static final int REG1_GYR_SLEEP_CONFIG = 0x0D
```

#### Bno055Regs.REG1_INT_MSK *(field)*

```java
public static final int REG1_INT_MSK = 0x0F
```

#### Bno055Regs.REG1_INT_EN *(field)*

```java
public static final int REG1_INT_EN = 0x10
```

#### Bno055Regs.REG1_ACC_AM_THRES *(field)*

```java
public static final int REG1_ACC_AM_THRES = 0x11
```

Page-1 0x11..0x1F are the motion-interrupt settings, written raw.

#### Bno055Regs.REG1_GYR_AM_SET *(field)*

```java
public static final int REG1_GYR_AM_SET = 0x1F
```

#### Bno055Regs.REG1_UNIQUE_ID *(field)*

```java
public static final int REG1_UNIQUE_ID = 0x50
```

#### Bno055Regs.UNIQUE_ID_LEN *(field)*

```java
public static final int UNIQUE_ID_LEN = 16
```

#### Bno055Regs.FULL_BLOCK_ADDR *(field)*

```java
public static final int FULL_BLOCK_ADDR
```

The block that carries every output channel: 0x08 (ACC_DATA_X_LSB) … 0x35 (CALIB_STAT).

#### Bno055Regs.FULL_BLOCK_LEN *(field)*

```java
public static final int FULL_BLOCK_LEN
```

#### Bno055Regs.QUAT_BLOCK_ADDR *(field)*

```java
public static final int QUAT_BLOCK_ADDR
```

Quaternion only — the cheapest orientation read (8 bytes, ~1.2 ms of bus).

#### Bno055Regs.QUAT_BLOCK_LEN *(field)*

```java
public static final int QUAT_BLOCK_LEN = 8
```

#### Bno055Regs.SYS_TRIGGER_SELF_TEST *(field)*

```java
public static final int SYS_TRIGGER_SELF_TEST = 0x01
```

#### Bno055Regs.SYS_TRIGGER_RST_SYS *(field)*

```java
public static final int SYS_TRIGGER_RST_SYS = 0x20
```

#### Bno055Regs.SYS_TRIGGER_RST_INT *(field)*

```java
public static final int SYS_TRIGGER_RST_INT = 0x40
```

#### Bno055Regs.SYS_TRIGGER_CLK_SEL *(field)*

```java
public static final int SYS_TRIGGER_CLK_SEL = 0x80
```

#### Bno055Regs.INT_ACC_BSX_DRDY *(field)*

```java
public static final int INT_ACC_BSX_DRDY = 0x01
```

#### Bno055Regs.INT_MAG_DRDY *(field)*

```java
public static final int INT_MAG_DRDY = 0x02
```

#### Bno055Regs.INT_GYR_AM *(field)*

```java
public static final int INT_GYR_AM = 0x04
```

#### Bno055Regs.INT_GYR_HIGH_RATE *(field)*

```java
public static final int INT_GYR_HIGH_RATE = 0x08
```

#### Bno055Regs.INT_GYR_DRDY *(field)*

```java
public static final int INT_GYR_DRDY = 0x10
```

#### Bno055Regs.INT_ACC_HIGH_G *(field)*

```java
public static final int INT_ACC_HIGH_G = 0x20
```

#### Bno055Regs.INT_ACC_AM *(field)*

```java
public static final int INT_ACC_AM = 0x40
```

#### Bno055Regs.INT_ACC_NM *(field)*

```java
public static final int INT_ACC_NM = 0x80
```

#### Bno055Regs.ST_ACC *(field)*

```java
public static final int ST_ACC = 0x01
```

#### Bno055Regs.ST_MAG *(field)*

```java
public static final int ST_MAG = 0x02
```

#### Bno055Regs.ST_GYR *(field)*

```java
public static final int ST_GYR = 0x04
```

#### Bno055Regs.ST_MCU *(field)*

```java
public static final int ST_MCU = 0x08
```

#### Bno055Regs.EXPECTED_SELF_TEST *(field)*

```java
public static final int EXPECTED_SELF_TEST
```

#### Bno055Regs.UNIT_ACC_MG *(field)*

```java
public static final int UNIT_ACC_MG = 0x01
```

#### Bno055Regs.UNIT_GYR_RPS *(field)*

```java
public static final int UNIT_GYR_RPS = 0x02
```

#### Bno055Regs.UNIT_EUL_RAD *(field)*

```java
public static final int UNIT_EUL_RAD = 0x04
```

#### Bno055Regs.UNIT_TEMP_F *(field)*

```java
public static final int UNIT_TEMP_F = 0x10
```

#### Bno055Regs.UNIT_ORI_ANDROID *(field)*

```java
public static final int UNIT_ORI_ANDROID = 0x80
```

#### Bno055Regs.MAG_LSB *(field)*

```java
public static final double MAG_LSB
```

µT, not selectable.

#### Bno055Regs.QUAT_LSB *(field)*

```java
public static final double QUAT_LSB
```

2^14, unit-less.

#### Bno055Regs.FUSION_ACCEL_LSB *(field)*

```java
public static final double FUSION_ACCEL_LSB
```

Linear acceleration and gravity ignore the ACC_Unit bit: always m/s² at
100 LSB — measured on SW rev 03.11 (datasheet Tables 3-33/3-35 claim mg).

#### Bno055Regs.SIC_IDENTITY *(field)*

```java
public static final int[] SIC_IDENTITY
```

Soft-iron matrix identity, 1.0 = 16384.

#### Bno055Regs.packSicMatrix

```java
public static byte[] packSicMatrix(int[] m)
```

Soft-iron matrix, 9 × i16 row-major, 1.0 = 16384 (datasheet §3.11.4).

#### Bno055Regs.unpackSicMatrix

```java
public static int[] unpackSicMatrix(byte[] data)
```

#### Bno055Regs.AXIS_X *(field)*

```java
public static final int AXIS_X = 0
```

#### Bno055Regs.AXIS_Y *(field)*

```java
public static final int AXIS_Y = 1
```

#### Bno055Regs.AXIS_Z *(field)*

```java
public static final int AXIS_Z = 2
```

#### Bno055Regs.PLACEMENTS *(field)*

```java
public static final Map<String, int[]> PLACEMENTS
```

Datasheet §3.4: placement → `{AXIS_MAP_CONFIG, AXIS_MAP_SIGN}`, P0..P7 in order.

#### Bno055Regs.ACC_RANGE_G *(field)*

```java
public static final int[] ACC_RANGE_G
```

#### Bno055Regs.ACC_BANDWIDTH_HZ *(field)*

```java
public static final double[] ACC_BANDWIDTH_HZ
```

#### Bno055Regs.ACC_POWER_NAMES *(field)*

```java
public static final String[] ACC_POWER_NAMES
```

#### Bno055Regs.GYR_RANGE_DPS *(field)*

```java
public static final int[] GYR_RANGE_DPS
```

#### Bno055Regs.GYR_BANDWIDTH_HZ *(field)*

```java
public static final int[] GYR_BANDWIDTH_HZ
```

#### Bno055Regs.GYR_POWER_NAMES *(field)*

```java
public static final String[] GYR_POWER_NAMES
```

#### Bno055Regs.MAG_RATE_HZ *(field)*

```java
public static final int[] MAG_RATE_HZ
```

#### Bno055Regs.MAG_OPR_NAMES *(field)*

```java
public static final String[] MAG_OPR_NAMES
```

#### Bno055Regs.MAG_POWER_NAMES *(field)*

```java
public static final String[] MAG_POWER_NAMES
```

#### Bno055Regs.decodeBlock

```java
public static RawBlock decodeBlock(int addr, byte[] data)
```

Unpack whatever channels the register window starting at `addr` holds.

### Bno055Regs.OprMode

```java
public enum OprMode
```

OPR_MODE (0x3D) bits 3:0 (the register reads back 0x10 after reset — mask).

#### Bno055Regs.OprMode — constants

`CONFIG`(0x00), `ACCONLY`(0x01), `MAGONLY`(0x02), `GYROONLY`(0x03), `ACCMAG`(0x04), `ACCGYRO`(0x05), `MAGGYRO`(0x06), `AMG`(0x07), `IMU`(0x08), `COMPASS`(0x09), `M4G`(0x0A), `NDOF_FMC_OFF`(0x0B), `NDOF`(0x0C)

#### Bno055Regs.OprMode.value *(field)*

```java
public final int value
```

#### Bno055Regs.OprMode.isFusion

```java
public boolean isFusion()
```

### Bno055Regs.PwrMode

```java
public enum PwrMode
```

PWR_MODE (0x3E) bits 1:0.

#### Bno055Regs.PwrMode — constants

`NORMAL`(0x00), `LOW_POWER`(0x01), `SUSPEND`(0x02)

#### Bno055Regs.PwrMode.value *(field)*

```java
public final int value
```

### Bno055Regs.TempSource

```java
public enum TempSource
```

TEMP_SOURCE (0x40) bits 1:0.

#### Bno055Regs.TempSource — constants

`ACCEL`(0x00), `GYRO`(0x01)

#### Bno055Regs.TempSource.value *(field)*

```java
public final int value
```

### Bno055Regs.Units

```java
public record Units(boolean accelMg, boolean gyroRps, boolean eulerRad, boolean tempF, boolean android)
```

Output units. The SDK default (`DEFAULT`, UNIT_SEL = 0x00) is m/s²,
dps, degrees, °C, Windows orientation. The sensor's own power-on value is
0x80 (Android), so a fresh sensor must be told. Unknown bits are dropped.

#### Bno055Regs.Units.DEFAULT *(field)*

```java
public static final Units DEFAULT
```

#### Bno055Regs.Units.pack

```java
public int pack()
```

#### Bno055Regs.Units.unpack

```java
public static Units unpack(int value)
```

#### Bno055Regs.Units.accelLsb

```java
public double accelLsb()
```

ACC_DATA LSB: 1 per mg, else 100 per m/s² (LIA/GRV use `FUSION_ACCEL_LSB`).

#### Bno055Regs.Units.gyroLsb

```java
public double gyroLsb()
```

Angular-rate LSB: 900 per rad/s, else 16 per dps.

#### Bno055Regs.Units.eulerLsb

```java
public double eulerLsb()
```

Euler LSB: 900 per radian, else 16 per degree.

#### Bno055Regs.Units.tempLsb

```java
public double tempLsb()
```

Temperature LSB: 1 LSB = 2 °F (0.5 LSB/°F), else 1 LSB = 1 °C.

### Bno055Regs.CalibStatus

```java
public record CalibStatus(int system, int gyro, int accel, int mag)
```

CALIB_STAT (0x35) `sys<7:6> gyr<5:4> acc<3:2> mag<1:0>`: 0 = not calibrated … 3 = fully.

#### Bno055Regs.CalibStatus.unpack

```java
public static CalibStatus unpack(int value)
```

#### Bno055Regs.CalibStatus.pack

```java
public int pack()
```

#### Bno055Regs.CalibStatus.fullyCalibrated

```java
public boolean fullyCalibrated()
```

### Bno055Regs.CalibrationProfile

```java
public record CalibrationProfile(int[] accelOffset, int[] magOffset, int[] gyroOffset, int accelRadius, int magRadius)
```

Sensor offsets and radii, registers 0x55..0x6A (22 bytes, 11 × i16 LE:
acc_offset xyz, mag_offset xyz, gyr_offset xyz, acc_radius, mag_radius).
Readable and writable only in CONFIG; write all 22 bytes in one transfer
(the sensor latches each group on its MSB). Offsets are in sensor LSB and do
not depend on UNIT_SEL.

#### Bno055Regs.CalibrationProfile.pack

```java
public byte[] pack()
```

#### Bno055Regs.CalibrationProfile.unpack

```java
public static CalibrationProfile unpack(byte[] data)
```

Throws `IllegalArgumentException` unless `data` is exactly 22 bytes.

### Bno055Regs.AxisRemap

```java
public record AxisRemap(int x, int y, int z, boolean xNegative, boolean yNegative, boolean zNegative)
```

Which chip axis feeds each output axis, and its sign. `x = AXIS_Y`
means "output X is the chip's Y axis". `AXIS_MAP_CONFIG = z<5:4>
y<3:2> x<1:0>`, `AXIS_MAP_SIGN = x 2, y 1, z 0` (1 = negative).

#### Bno055Regs.AxisRemap.DEFAULT *(field)*

```java
public static final AxisRemap DEFAULT
```

P1, the power-on mapping.

#### Bno055Regs.AxisRemap.pack

```java
public int[] pack()
```

→ `{AXIS_MAP_CONFIG, AXIS_MAP_SIGN}`. The sensor keeps its old
mapping when given one that uses an axis twice, so this refuses it up
front with `IllegalArgumentException`.

#### Bno055Regs.AxisRemap.unpack

```java
public static AxisRemap unpack(int config, int sign)
```

#### Bno055Regs.AxisRemap.placement

```java
public static AxisRemap placement(String name)
```

Datasheet §3.4 mounting presets P0..P7 (case-insensitive; P1 is the
default); throws `IllegalArgumentException` for another name.

### Bno055Regs.AccelConfig

```java
public record AccelConfig(int range, int bandwidth, int power)
```

ACC_CONFIG (page 1, 0x08) as register codes `range<1:0>
bandwidth<4:2> power<7:5>`: `range` indexes `ACC_RANGE_G`,
`bandwidth` `ACC_BANDWIDTH_HZ`, `power`
`ACC_POWER_NAMES`. Power-on value 0x0D = ±4 g, 62.5 Hz, normal.

#### Bno055Regs.AccelConfig.DEFAULT *(field)*

```java
public static final AccelConfig DEFAULT
```

#### Bno055Regs.AccelConfig.pack

```java
public int pack()
```

#### Bno055Regs.AccelConfig.unpack

```java
public static AccelConfig unpack(int value)
```

### Bno055Regs.GyroConfig

```java
public record GyroConfig(int range, int bandwidth, int power)
```

GYR_CONFIG_0/1 (page 1, 0x0A/0x0B), 2 bytes: `byte0 = range<2:0>
bandwidth<5:3>`, `byte1 = power<2:0>`. Indexes `GYR_RANGE_DPS`,
`GYR_BANDWIDTH_HZ`, `GYR_POWER_NAMES`. Power-on 0x38/0x00 =
2000 dps, 32 Hz, normal.

#### Bno055Regs.GyroConfig.DEFAULT *(field)*

```java
public static final GyroConfig DEFAULT
```

#### Bno055Regs.GyroConfig.pack

```java
public byte[] pack()
```

#### Bno055Regs.GyroConfig.unpack

```java
public static GyroConfig unpack(byte[] data)
```

### Bno055Regs.MagConfig

```java
public record MagConfig(int rate, int mode, int power)
```

MAG_CONFIG (page 1, 0x09) `rate<2:0> mode<4:3> power<6:5>`: indexes
`MAG_RATE_HZ`, `MAG_OPR_NAMES`, `MAG_POWER_NAMES`.
Bit 7 is not a field: `pack(unpack(v)) == v & 0x7F`. Power-on 0x0B =
10 Hz, regular, normal.

#### Bno055Regs.MagConfig.DEFAULT *(field)*

```java
public static final MagConfig DEFAULT
```

#### Bno055Regs.MagConfig.pack

```java
public int pack()
```

#### Bno055Regs.MagConfig.unpack

```java
public static MagConfig unpack(int value)
```

### Bno055Regs.RawBlock

```java
public record RawBlock(int[] accel, int[] mag, int[] gyro, int[] euler, int[] quaternion, int[] linearAccel, int[] gravity, Integer temperature, Integer calibStat)
```

Raw register values found in one block read. A channel is `null` when
the window `addr..addr+len` does not cover all of its bytes. Euler is
(heading, roll, pitch), quaternion (w, x, y, z); vectors are (x, y, z).

## Datasets (record & replay)

### Dataset

```java
public final class Dataset
```

`.depzdata` dataset reader (contracts/09_DATASET_FORMAT.md): a JSONL
file whose first line is the header (`schema`, `devices`, ...)
and whose remaining lines are records `{"d","t","k","v"}`. Records are
stably merged onto one host timeline by `t` (µs). Mirror of the
TS/Python `DatasetReader`.

JSON parsing is delegated to a caller-supplied line parser so this module
has no dependency on the test harness's JSON code; see `parse`.

#### Dataset.DATASET_SCHEMA_PREFIX *(field)*

```java
public static final String DATASET_SCHEMA_PREFIX
```

#### Dataset.parse

```java
public static Dataset parse(String content, java.util.function.Function<String, Object> jsonLine)
```

Parse dataset content. `jsonLine` converts one JSON line to a
`Map<String,Object>` (objects → Map, arrays → List, ints → Long).

#### Dataset.header

```java
public Map<String, Object> header()
```

#### Dataset.schema

```java
public String schema()
```

#### Dataset.records

```java
public List<Record> records()
```

#### Dataset.durationUs

```java
public long durationUs()
```

### Dataset.Record

```java
public record Record(String deviceId, long tHostUs, String kind, Map<String, Object> value)
```

One merged record.
