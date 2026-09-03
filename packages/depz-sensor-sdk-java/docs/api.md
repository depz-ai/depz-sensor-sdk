# API reference

Auto-generated from the public Java source under
`src/main/java/ai/depz/sensor/` by `scripts/gen_api_md.py` — run
`python3 scripts/gen_api_md.py` to regenerate. Edit the Javadoc in the
source, not this file.

This SDK is decode-layer only: pure, host-verifiable codecs. The live
ULD init / register-bridge driver and the VL53L8CH CNH histogram decode
are extension points, surfaced here as documented stubs.

Each sensor also has a focused reference with just its own symbols:
[SR04](sr04/api.md) · [VL53L4CD](vl53l4cd/api.md) · [VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · [BNO086](bno086/api.md).

## Contents

- **Transport**: [`Crc`](#crc), [`CrcError`](#crcerror), [`CrcType`](#crctype), [`Event`](#event), [`Framing`](#framing), [`Packet`](#packet), [`PacketParser`](#packetparser), [`Trash`](#trash)
- **USB identity**: [`UsbIds`](#usbids), [`UsbIds.PortRef`](#usbidsportref)
- **Protocol codecs (common)**: [`Common`](#common), [`Common.Cmd`](#commoncmd), [`Common.Rpt`](#commonrpt), [`Common.Status`](#commonstatus), [`Common.SyncPinMode`](#commonsyncpinmode), [`Common.SyncPinPolarity`](#commonsyncpinpolarity), [`Common.StatusReport`](#commonstatusreport), [`Common.TextReport`](#commontextreport), [`Common.SyncTimeReport`](#commonsynctimereport), [`Common.TemperatureReport`](#commontemperaturereport), [`Common.SequenceErrorReport`](#commonsequenceerrorreport), [`Common.SyncPinConfig`](#commonsyncpinconfig), [`Identity`](#identity), [`Identity.SensorType`](#identitysensortype)
- **SR04**: [`Sr04`](#sr04), [`Sr04.Sr04Cmd`](#sr04sr04cmd), [`Sr04.Sr04Rpt`](#sr04sr04rpt), [`Sr04.Sr04Data`](#sr04sr04data)
- **Firmware container**: [`FwDepz`](#fwdepz), [`FwDepz.FwDepzError`](#fwdepzfwdepzerror), [`FwDepz.FwDepzImage`](#fwdepzfwdepzimage)
- **VL53L4CD (ToF)**: [`Vl53l4`](#vl53l4), [`Vl53l4.Vl53l4Cmd`](#vl53l4vl53l4cmd), [`Vl53l4.Vl53l4Rpt`](#vl53l4vl53l4rpt), [`Vl53l4.RegData`](#vl53l4regdata), [`Vl53l4.Vl53l4Info`](#vl53l4vl53l4info), [`Vl53l4.StreamData`](#vl53l4streamdata), [`Vl53l4Uld`](#vl53l4uld), [`Vl53l4Uld.Vl53l4Error`](#vl53l4uldvl53l4error), [`Vl53l4Uld.Results`](#vl53l4uldresults)
- **VL53L8 (ToF)**: [`FrameReassembler`](#framereassembler), [`FrameReassembler.FrameChunk`](#framereassemblerframechunk), [`FrameReassembler.CompletedFrame`](#framereassemblercompletedframe), [`Vl53l8Uld`](#vl53l8uld), [`Vl53l8Uld.Variant`](#vl53l8uldvariant), [`Vl53l8Uld.CnhAggregate`](#vl53l8uldcnhaggregate), [`Vl53l8Uld.CnhResult`](#vl53l8uldcnhresult), [`Vl53l8Uld.Results`](#vl53l8uldresults), [`Vl53l8Uld.DetectionThreshold`](#vl53l8ulddetectionthreshold), [`Vl53l8Uld.PackedThresholds`](#vl53l8uldpackedthresholds), [`Vl53l8Uld.MotionConfig`](#vl53l8uldmotionconfig), [`Vl53l8Uld.Vl53l8Error`](#vl53l8uldvl53l8error)
- **BNO086 (IMU)**: [`Reports`](#reports), [`Reports.Report`](#reportsreport), [`Sh2`](#sh2), [`Shtp`](#shtp), [`Shtp.ShtpHeader`](#shtpshtpheader), [`Shtp.ShtpCargo`](#shtpshtpcargo), [`Shtp.ShtpLayer`](#shtpshtplayer)
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

`SR04`, `VL53L4`, `VL53L8`, `BNO086`, `UNKNOWN`

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

#### Vl53l8Uld.decodeCnh

```java
public static CnhResult decodeCnh(int nbOfAggregates, int featureLength, byte[] raw)
```

Decode a captured CNH data block (`raw`, byte-swapped exactly like the
standard ranging blocks) into per-aggregate histograms, for the fixed
cnh_cfg (ping-pong + variance disabled). Faithful port of the ST CNH plugin
/ Python `cnh.decode`. Replaces the former `cnhHistogramDecodeStubbed`.

- `nbOfAggregates` — number of CNH aggregates (from the CNH config)
- `featureLength` — CNH bins per aggregate (from the CNH config)
- `raw` — captured CNH block bytes

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

Raw CNH (compact-histogram) block bytes, VL53L8CH only; `null`
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
