# API reference

Auto-generated from the public Java source under
`src/main/java/ai/depz/sensor/` by `scripts/gen_api_md.py` — run
`python3 scripts/gen_api_md.py` to regenerate. Edit the Javadoc in the
source, not this file.

This SDK is decode-layer only: pure, host-verifiable codecs. The live
drivers (ULD init / register-bridge configuration, the 1D family's ST
drivers, the BNO055 session logic) are extension points, surfaced here
as documented stubs. The TMF8829 sensor sequence (init, configure,
start / stop) is not ported: its codecs and decode classes are.

Each sensor also has a focused reference with just its own symbols:
[SR04](sr04/api.md) · [VL53L0X](vl53l0x/api.md) · [VL53L1CX](vl53l1cx/api.md) · [VL53L1CB](vl53l1cb/api.md) · [VL53L3CX](vl53l3cx/api.md) · [VL53L4CD](vl53l4cd/api.md) · [VL53L4CX](vl53l4cx/api.md) · [VL53L5CX](vl53l5cx/api.md) · [VL53L7CX](vl53l7cx/api.md) · [VL53L7CH](vl53l7ch/api.md) · [VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · [TMF8829](tmf8829/api.md) · [BNO055](bno055/api.md) · [BNO085](bno085/api.md) · [BNO086](bno086/api.md).

## Contents

- **Transport**: [`Crc`](#crc), [`CrcError`](#crcerror), [`CrcType`](#crctype), [`Event`](#event), [`Framing`](#framing), [`Packet`](#packet), [`PacketParser`](#packetparser), [`Trash`](#trash)
- **USB identity**: [`Discovery`](#discovery), [`UsbIds`](#usbids), [`UsbIds.PortRef`](#usbidsportref)
- **Protocol codecs (common)**: [`Common`](#common), [`Common.Cmd`](#commoncmd), [`Common.Rpt`](#commonrpt), [`Common.Status`](#commonstatus), [`Common.SyncPinMode`](#commonsyncpinmode), [`Common.SyncPinPolarity`](#commonsyncpinpolarity), [`Common.StatusReport`](#commonstatusreport), [`Common.TextReport`](#commontextreport), [`Common.SyncTimeReport`](#commonsynctimereport), [`Common.TemperatureReport`](#commontemperaturereport), [`Common.SequenceErrorReport`](#commonsequenceerrorreport), [`Common.SyncPinConfig`](#commonsyncpinconfig), [`Identity`](#identity), [`Identity.SensorType`](#identitysensortype), [`Wire`](#wire)
- **Live devices**: [`BootloaderModeException`](#bootloadermodeexception), [`BusyException`](#busyexception), [`DepzDevice`](#depzdevice), [`DepzDevices`](#depzdevices), [`DepzDevices.ProbeResult`](#depzdevicesproberesult), [`DepzException`](#depzexception), [`DepzTimeoutException`](#depztimeoutexception), [`DeviceEvent`](#deviceevent), [`DeviceInfo`](#deviceinfo), [`DeviceLostException`](#devicelostexception), [`InvalidReplyException`](#invalidreplyexception), [`InvalidStateException`](#invalidstateexception), [`Link`](#link), [`LinkClosedException`](#linkclosedexception), [`LinkStats`](#linkstats), [`NoDepzDeviceException`](#nodepzdeviceexception), [`RecordingLink`](#recordinglink), [`ReplayLink`](#replaylink), [`ReplayMismatchException`](#replaymismatchexception), [`SerialPortLink`](#serialportlink), [`SerialPorts`](#serialports), [`SerialPorts.SerialPortInfo`](#serialportsserialportinfo), [`StatusException`](#statusexception), [`Subscription`](#subscription), [`TimeSyncResult`](#timesyncresult), [`WrongTypeException`](#wrongtypeexception)
- **SR04**: [`Sr04Device`](#sr04device), [`Sr04Measurement`](#sr04measurement), [`Sr04`](#sr04), [`Sr04.Sr04Cmd`](#sr04sr04cmd), [`Sr04.Sr04Rpt`](#sr04sr04rpt), [`Sr04.Sr04Data`](#sr04sr04data)
- **Firmware container**: [`FwDepz`](#fwdepz), [`FwDepz.FwDepzError`](#fwdepzfwdepzerror), [`FwDepz.FwDepzImage`](#fwdepzfwdepzimage)
- **VL53L4CD (ToF)**: [`Vl53l4Measurement`](#vl53l4measurement), [`Vl53l4cdDevice`](#vl53l4cddevice), [`Vl53l4`](#vl53l4), [`Vl53l4.Vl53l4Cmd`](#vl53l4vl53l4cmd), [`Vl53l4.Vl53l4Rpt`](#vl53l4vl53l4rpt), [`Vl53l4.RegData`](#vl53l4regdata), [`Vl53l4.Vl53l4Info`](#vl53l4vl53l4info), [`Vl53l4.StreamData`](#vl53l4streamdata), [`DetectionThresholds`](#detectionthresholds), [`RangeTiming`](#rangetiming), [`Vl53l4Platform`](#vl53l4platform), [`Vl53l4Uld`](#vl53l4uld), [`Vl53l4Uld.Vl53l4Error`](#vl53l4uldvl53l4error), [`Vl53l4Uld.Results`](#vl53l4uldresults), [`Vl53l4cdDriver`](#vl53l4cddriver)
- **VL53L8 (ToF)**: [`Vl53l8chDevice`](#vl53l8chdevice), [`Vl53l8cxDevice`](#vl53l8cxdevice), [`Vl53l8`](#vl53l8), [`Vl53l8.Vl53l8Cmd`](#vl53l8vl53l8cmd), [`Vl53l8.Vl53l8Rpt`](#vl53l8vl53l8rpt), [`Vl53l8.RegData`](#vl53l8regdata), [`FrameReassembler`](#framereassembler), [`FrameReassembler.FrameChunk`](#framereassemblerframechunk), [`FrameReassembler.CompletedFrame`](#framereassemblercompletedframe), [`Vl53l8CnhSetup`](#vl53l8cnhsetup), [`Vl53l8Driver`](#vl53l8driver), [`Vl53l8Firmware`](#vl53l8firmware), [`Vl53l8Frame`](#vl53l8frame), [`Vl53l8Motion`](#vl53l8motion), [`Vl53l8Platform`](#vl53l8platform), [`Vl53l8Tables`](#vl53l8tables), [`Vl53l8Uld`](#vl53l8uld), [`Vl53l8Uld.Variant`](#vl53l8uldvariant), [`Vl53l8Uld.CnhAggregate`](#vl53l8uldcnhaggregate), [`Vl53l8Uld.CnhResult`](#vl53l8uldcnhresult), [`Vl53l8Uld.Results`](#vl53l8uldresults), [`Vl53l8Uld.DetectionThreshold`](#vl53l8ulddetectionthreshold), [`Vl53l8Uld.PackedThresholds`](#vl53l8uldpackedthresholds), [`Vl53l8Uld.MotionConfig`](#vl53l8uldmotionconfig), [`Vl53l8Uld.Vl53l8Error`](#vl53l8uldvl53l8error)
- **VL53L5CX / VL53L7CX / VL53L7CH (ToF)**: [`Vl53l5cxDevice`](#vl53l5cxdevice), [`Vl53l7chDevice`](#vl53l7chdevice), [`Vl53l7cxDevice`](#vl53l7cxdevice), [`Vl53l7`](#vl53l7), [`Vl53l7.Vl53l7Cmd`](#vl53l7vl53l7cmd), [`Vl53l7.Vl53l7Rpt`](#vl53l7vl53l7rpt), [`Vl53l7.PinAction`](#vl53l7pinaction), [`Vl53l7.Vl53l7Info`](#vl53l7vl53l7info), [`Vl53l7Uld`](#vl53l7uld), [`Vl53l7Uld.Model`](#vl53l7uldmodel)
- **VL53L0X / L1CX / L1CB / L3CX / L4CX (ToF)**: [`Vl53l0xDevice`](#vl53l0xdevice), [`Vl53l1cbDevice`](#vl53l1cbdevice), [`Vl53l1cxDevice`](#vl53l1cxdevice), [`Vl53l3cxDevice`](#vl53l3cxdevice), [`Vl53l4cxDevice`](#vl53l4cxdevice), [`Vl53lxDevice`](#vl53lxdevice), [`Vl53lxMeasurement`](#vl53lxmeasurement), [`Vl53lx`](#vl53lx), [`Vl53lx.Vl53lxCmd`](#vl53lxvl53lxcmd), [`Vl53lx.Vl53lxRpt`](#vl53lxvl53lxrpt), [`Vl53lx.Vl53lxInfo`](#vl53lxvl53lxinfo), [`Vl53l0xUldDriver`](#vl53l0xulddriver), [`Vl53l0xUldDriver.RefCalibration`](#vl53l0xulddriverrefcalibration), [`Vl53l0xUldDriver.RefSpads`](#vl53l0xulddriverrefspads), [`Vl53l1Die`](#vl53l1die), [`Vl53l1UldDriver`](#vl53l1ulddriver), [`Vl53l3UlpDriver`](#vl53l3ulpdriver), [`Vl53l4UldDriver`](#vl53l4ulddriver), [`Vl53lxDecode`](#vl53lxdecode), [`Vl53lxDecode.DieVariant`](#vl53lxdecodedievariant), [`Vl53lxDecode.L0xRaw`](#vl53lxdecodel0xraw), [`Vl53lxDecode.HistogramRaw`](#vl53lxdecodehistogramraw), [`Vl53lxDriver`](#vl53lxdriver), [`Vl53lxDriver.Thresholds`](#vl53lxdriverthresholds), [`Vl53lxDrivers`](#vl53lxdrivers), [`Vl53lxDrivers.Created`](#vl53lxdriverscreated), [`Vl53lxException`](#vl53lxexception), [`Vl53lxHistogramDriver`](#vl53lxhistogramdriver), [`Vl53lxPlatform`](#vl53lxplatform), [`Vl53lxProducts`](#vl53lxproducts), [`Vl53lxProducts.Bridge`](#vl53lxproductsbridge), [`Vl53lxProducts.Product`](#vl53lxproductsproduct), [`Vl53lxProducts.SensorClass`](#vl53lxproductssensorclass), [`Vl53lxProtocolException`](#vl53lxprotocolexception), [`Vl53lxRange`](#vl53lxrange), [`Vl53lxTarget`](#vl53lxtarget), [`BareDeviceState`](#baredevicestate), [`BareDriver`](#baredriver), [`BareMath`](#baremath), [`BareRegs`](#bareregs), [`BareTables`](#baretables), [`DeviceImage`](#deviceimage), [`FrameHistory`](#framehistory), [`Hist`](#hist), [`HistConfig`](#histconfig), [`HistPostProcessConfig`](#histpostprocessconfig), [`HistogramBinData`](#histogrambindata), [`LlState`](#llstate), [`NvmReader`](#nvmreader), [`NvmReader.OpticalCentre`](#nvmreaderopticalcentre), [`NvmReader.CalPeakRateMap`](#nvmreadercalpeakratemap), [`NvmReader.AdditionalOffsetCalData`](#nvmreaderadditionaloffsetcaldata), [`NvmReader.FmtRangeData`](#nvmreaderfmtrangedata), [`RangeData`](#rangedata), [`RangeResults`](#rangeresults), [`RegBlock`](#regblock), [`RegBlockDef`](#regblockdef), [`RegField`](#regfield)
- **TMF8829 (dToF)**: [`Tmf8829AppliedConfig`](#tmf8829appliedconfig), [`Tmf8829Device`](#tmf8829device), [`Tmf8829Device.InitResult`](#tmf8829deviceinitresult), [`Tmf8829Exception`](#tmf8829exception), [`Tmf8829InterruptEvent`](#tmf8829interruptevent), [`Tmf8829`](#tmf8829), [`Tmf8829.Tmf8829Cmd`](#tmf8829tmf8829cmd), [`Tmf8829.Tmf8829Rpt`](#tmf8829tmf8829rpt), [`Tmf8829.EventCode`](#tmf8829eventcode), [`Tmf8829.RegData`](#tmf8829regdata), [`Tmf8829.Tmf8829Info`](#tmf8829tmf8829info), [`Tmf8829.FrameChunk`](#tmf8829framechunk), [`Tmf8829.Tmf8829Event`](#tmf8829tmf8829event), [`Tmf8829.RawFrame`](#tmf8829rawframe), [`Tmf8829.FrameReassembler`](#tmf8829framereassembler), [`CaptureTimer`](#capturetimer), [`CaptureTimer.FrameTime`](#capturetimerframetime), [`Frames`](#frames), [`Frames.FrameError`](#framesframeerror), [`Frames.FrameInfo`](#framesframeinfo), [`Frames.PixelResults`](#framespixelresults), [`Frames.HistogramFrame`](#frameshistogramframe), [`MeasurementBuilder`](#measurementbuilder), [`MeasurementBuilder.Measurement`](#measurementbuildermeasurement), [`SensorClock`](#sensorclock), [`Tmf8829Config`](#tmf8829config), [`Tmf8829Config.CfgField`](#tmf8829configcfgfield), [`Tmf8829Firmware`](#tmf8829firmware)
- **BNO086 (IMU)**: [`Bno086Device`](#bno086device), [`Bno086`](#bno086), [`Bno086.Bno086Cmd`](#bno086bno086cmd), [`Bno086.Bno086Rpt`](#bno086bno086rpt), [`Bno086.Data`](#bno086data), [`Reports`](#reports), [`Reports.Report`](#reportsreport), [`Sh2`](#sh2), [`Sh2Hub`](#sh2hub), [`Sh2Hub.TareBasis`](#sh2hubtarebasis), [`Sh2Hub.MotionIntent`](#sh2hubmotionintent), [`Sh2Hub.Sh2Exception`](#sh2hubsh2exception), [`Sh2Hub.FeatureResponse`](#sh2hubfeatureresponse), [`Sh2Hub.ProductId`](#sh2hubproductid), [`Sh2Hub.CommandResponse`](#sh2hubcommandresponse), [`Sh2Hub.FrsReadResponse`](#sh2hubfrsreadresponse), [`Sh2Hub.FrsWriteResponse`](#sh2hubfrswriteresponse), [`Sh2Hub.ErrorRecord`](#sh2huberrorrecord), [`Sh2Hub.Counts`](#sh2hubcounts), [`Sh2Hub.CalibrationConfig`](#sh2hubcalibrationconfig), [`Sh2Hub.SensorMetadata`](#sh2hubsensormetadata), [`Sh2Hub.FrsReadSession`](#sh2hubfrsreadsession), [`Sh2Hub.FrsWriteSession`](#sh2hubfrswritesession), [`Shtp`](#shtp), [`Shtp.ShtpHeader`](#shtpshtpheader), [`Shtp.ShtpCargo`](#shtpshtpcargo), [`Shtp.ShtpLayer`](#shtpshtplayer)
- **BNO055 (IMU)**: [`Bno055Device`](#bno055device), [`Bno055Sample`](#bno055sample), [`Bno055`](#bno055), [`Bno055.Bno055Cmd`](#bno055bno055cmd), [`Bno055.Bno055Rpt`](#bno055bno055rpt), [`Bno055.RegData`](#bno055regdata), [`Bno055.StreamData`](#bno055streamdata), [`Bno055.Bno055Info`](#bno055bno055info), [`Bno055Regs`](#bno055regs), [`Bno055Regs.SystemStatus`](#bno055regssystemstatus), [`Bno055Regs.OprMode`](#bno055regsoprmode), [`Bno055Regs.PwrMode`](#bno055regspwrmode), [`Bno055Regs.TempSource`](#bno055regstempsource), [`Bno055Regs.Units`](#bno055regsunits), [`Bno055Regs.CalibStatus`](#bno055regscalibstatus), [`Bno055Regs.CalibrationProfile`](#bno055regscalibrationprofile), [`Bno055Regs.AxisRemap`](#bno055regsaxisremap), [`Bno055Regs.AccelConfig`](#bno055regsaccelconfig), [`Bno055Regs.GyroConfig`](#bno055regsgyroconfig), [`Bno055Regs.MagConfig`](#bno055regsmagconfig), [`Bno055Regs.RawBlock`](#bno055regsrawblock)
- **Datasets (record & replay)**: [`Dataset`](#dataset), [`DatasetPlayer`](#datasetplayer), [`DatasetRecords`](#datasetrecords), [`DatasetWriter`](#datasetwriter), [`PlayerState`](#playerstate), [`SessionRecorder`](#sessionrecorder)
- **JSON**: [`JsonText`](#jsontext)

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
public static byte[] buildPacket(int cmd, byte[] payload, long seq, CrcType crcType)
```

Frame one packet. `crcType` bits are set in the header even for an
empty payload (matching device TX), but CRC bytes are only appended for
non-empty payloads. `seq` is a free-running u32 counter taken modulo
256 on the wire; `cmd` outside 0..255, `seq` outside 0..2^32-1 or
a null `crcType` → IllegalArgumentException.

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

### Discovery

```java
public final class Discovery
```

Class resolution (contracts/02_COMMON_COMMANDS.md §4.4, Python reference
`resolve_sensor_class`): which SDK class a device opens as, from its
probe answers and its port's USB (vid, pid).

This SDK is decode-only (no device layer), so it carries the pure part of
§4.4 only: `resolveSensorClass` and `SENSOR_CLASS_NAMES`.

#### Discovery.SENSOR_CLASS_NAMES *(field)*

```java
public static final List<String> SENSOR_CLASS_NAMES
```

Every name `resolveSensorClass` returns: the contract 07 sensor
classes, `"vl53lx"` (the generic 1D-family class: an unstamped
board, or a VL53L4CD on the APP_VL53L0_4 firmware), `"tmf8829"`,
`"device"` (the base device: an unknown application, or a name that
is neither APP_ nor BOOTDEPZ) and `"bootloader"` (use the contract
06 client). BNO085 boards run APP_BNO086 and resolve to `"bno086"`.

#### Discovery.resolveSensorClass

```java
public static String resolveSensorClass(String softwareName, String deviceName, Integer usbVid, Integer usbPid)
```

The canonical class name (one of `SENSOR_CLASS_NAMES`) for a
device's probe answers and its port's USB id — contract 02 §4.4, pure.
- BOOTDEPZ_* → `"bootloader"`;
- vl53l8 → `"vl53l8ch"` if the USB model is VL53L8CH, else `"vl53l8cx"`;
- vl53l7 → `Vl53l7Uld#resolveModel` (USB model, else device name, else CX);
- vl53lx → `Vl53lxProducts#resolveClass` (USB model, else device name, else
the generic `"vl53lx"`; VL53L4CD on this firmware is generic too);
- any other known token → its class; anything else → `"device"`.

- `softwareName` — the GET_NAME_ACTIVE_SOFTWARE answer (already stripped)
- `deviceName` — the GET_DEVICE_NAME answer; `""` or `null` when none
- `usbVid` — the port's USB vendor id; `null` when unknown (a replay, a bridge)
- `usbPid` — the port's USB product id; `null` when unknown

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

#### UsbIds.PID_TMF8829 *(field)*

```java
public static final int PID_TMF8829 = 0xED4C
```

TMF8829 dToF production USB PID (catalog p_TM88).

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

`pcTimestampUs` is a u64 in Java's two's-complement convention: every
`long` is a valid bit pattern (a value ≥ 2^63 arrives negative).

#### Common.PAYLOAD_CRC_TYPE_MAX *(field)*

```java
public static final int PAYLOAD_CRC_TYPE_MAX = 3
```

SET_PAYLOAD_CRC_TYPE takes 0..3 (contract 02 §2).

#### Common.packSetPayloadCrcType

```java
public static byte[] packSetPayloadCrcType(int crcType)
```

SET_PAYLOAD_CRC_TYPE payload `crc_type u8`, 0..3 (contract 02 §2), else
IllegalArgumentException — checked before anything is written (contract 07 §3).

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

`BOOTLOADER`(0x01), `DEVICE_RESET`(0x02), `GET_DEVICE_NAME`(0x03), `GET_NAME_ACTIVE_SOFTWARE`(0x04), `GET_SERIAL`(0x05), `SYNC_TIME`(0x06), `GET_MCU_TEMPERATURE`(0x07), `GET_PAYLOAD_CRC_TYPE`(0x08), `SET_PAYLOAD_CRC_TYPE`(0x09), `USB_TEST_TX_START`(0x1C), `USB_TEST_TX_STOP`(0x1D), `USB_TEST_RX_DATA`(0x1E), `USB_TEST_STATS`(0x1F), `GET_SYNC_PIN_CONFIG`(0x30), `SET_SYNC_PIN_CONFIG`(0x31)

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

`STATUS`(0x80), `TEXT`(0x81), `SYNC_TIME`(0x82), `TEMPERATURE`(0x83), `SEQUENCE_ERROR`(0x84), `PAYLOAD_CRC_TYPE`(0x87), `USB_TEST_DATA`(0x88), `USB_TEST_STATS`(0x8F), `SYNC_PIN_CONFIG`(0x90)

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

`pin`: a logical AUX sync pin 1..5 (contract 02 §6), else IllegalArgumentException.

#### Common.SyncPinConfig.PIN_MIN *(field)*

```java
public static final int PIN_MIN = 1
```

#### Common.SyncPinConfig.PIN_MAX *(field)*

```java
public static final int PIN_MAX = 5
```

#### Common.SyncPinConfig.pack

```java
public byte[] pack()
```

#### Common.SyncPinConfig.checkPin

```java
public static int checkPin(int pin)
```

`pin` as a sync pin 1..5, else IllegalArgumentException.

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

`SR04`, `VL53L4`, `VL53L8`, `VL53L7`, `VL53LX`, `BNO086`, `BNO055`, `TMF8829`, `UNKNOWN`

#### Identity.SensorType.label *(field)*

```java
public final String label
```

### Wire

```java
public final class Wire
```

Wire-field checks shared by the codecs (contract 07 §3).

Encoding: every argument is checked against its wire width before a byte
is written — never truncated, wrapped or masked (`IllegalArgumentException`,
the SDK's `invalid_argument`). Decoding: a payload shorter than the
fields the contract defines is rejected (`IllegalArgumentException`);
trailing bytes are ignored by the callers.

#### Wire.checkUint

```java
public static long checkUint(String name, long value, int bits)
```

`value` as an unsigned integer that fits `bits` (≤ 63) bits, else IllegalArgumentException.

#### Wire.checkUintI

```java
public static int checkUintI(String name, long value, int bits)
```

`checkUint` for a field of at most 31 bits, as an `int`.

#### Wire.checkInt

```java
public static int checkInt(String name, long value, int bits)
```

`value` as a signed integer that fits `bits` (≤ 32) bits, else IllegalArgumentException.

#### Wire.needBytes

```java
public static void needBytes(String what, byte[] payload, int n)
```

Reject a device payload shorter than the `n` bytes its fields need.

## Live devices

### BootloaderModeException

```java
public final class BootloaderModeException extends DepzException
```

The device is in bootloader mode: use the contract 06 client, not a sensor class.

### BusyException

```java
public final class BusyException extends StatusException
```

ERR_BUSY from the device, or a request whose opcode is already in flight.

### DepzDevice

```java
public class DepzDevice implements AutoCloseable
```

A connection to one DEPZ device in application mode (contracts 02, 07): a reader thread,
replies correlated to requests by echoed opcode (one request in flight per opcode), events,
and the common commands. Sensor classes extend it. Requests block the calling thread up to
`timeout()` (default 200 ms). Callbacks (`on…`) run on the reader thread: keep
them short, never block, never close the device from one.

#### DepzDevice.DEFAULT_TIMEOUT *(field)*

```java
public static final Duration DEFAULT_TIMEOUT
```

The default request timeout, 200 ms (contract 07 §3).

#### DepzDevice.port

```java
public String port()
```

The link's name: the port path, or what the link wraps.

#### DepzDevice.isClosed

```java
public boolean isClosed()
```

#### DepzDevice.timeout

```java
public Duration timeout()
```

The timeout of requests that take none of their own.

#### DepzDevice.setTimeout

```java
public void setTimeout(Duration timeout)
```

#### DepzDevice.txCrcType

```java
public CrcType txCrcType()
```

The payload CRC the host puts on its packets (contract 01).

#### DepzDevice.setTxCrcType

```java
public void setTxCrcType(CrcType crcType)
```

#### DepzDevice.stats

```java
public LinkStats stats()
```

#### DepzDevice.handleReport

```java
protected boolean handleReport(Packet packet)
```

Route a streaming report; return true when the packet was consumed. Runs on the reader thread
— which starts in this class's constructor, before a subclass's fields are initialized: a
report arriving in that window must find them null and return false (it becomes an event).

#### DepzDevice.onClosing

```java
protected void onClosing()
```

The link is about to close: a class whose sensor keeps running on its own stops it here.
Exceptions are the class's to swallow; the link closes regardless.

#### DepzDevice.onReaderStopped

```java
protected void onReaderStopped()
```

The reader stopped: end the class's streams (null-check them, as in `handleReport`). Runs on the reader thread.

#### DepzDevice.emitEvent

```java
protected void emitEvent(DeviceEvent event)
```

Hand an event to the event subscribers (a sensor class's own events).

#### DepzDevice.onEvent

```java
public AutoCloseable onEvent(Consumer<DeviceEvent> callback)
```

Call `callback` for every event (reader thread). Close the result to stop.

#### DepzDevice.events

```java
public Subscription<DeviceEvent> events(int capacity)
```

A pull queue of events (bounded, drop-oldest); subscribed when this returns.

#### DepzDevice.events

```java
public Subscription<DeviceEvent> events()
```

#### DepzDevice.send

```java
public void send(int cmd, byte[] payload)
```

Send a packet without waiting for anything (escape hatch; prefer a request).

#### DepzDevice.send

```java
public void send(int cmd, byte[] payload, CrcType crcType)
```

#### DepzDevice.request

```java
public <T> T request(int cmd, byte[] payload, ReplyMatcher<T> matcher)
```

#### DepzDevice.requestOk

```java
public void requestOk(int cmd, byte[] payload, Duration timeout)
```

Send `cmd` and wait for RPT_STATUS OK echoing it.

#### DepzDevice.requestOk

```java
public void requestOk(int cmd, byte[] payload)
```

#### DepzDevice.requestOk

```java
public void requestOk(int cmd)
```

#### DepzDevice.expectReport

```java
public static <T> ReplyMatcher<T> expectReport(int reportId, Function<byte[], T> unpack)
```

A matcher for a report identified by its report id alone.

#### DepzDevice.expectText

```java
public static ReplyMatcher<String> expectText(int requestCmd)
```

A matcher for RPT_TEXT echoing `requestCmd`.

#### DepzDevice.getDeviceName

```java
public String getDeviceName()
```

#### DepzDevice.getSoftwareName

```java
public String getSoftwareName()
```

#### DepzDevice.getSerialNumber

```java
public String getSerialNumber()
```

#### DepzDevice.identify

```java
public Identity identify()
```

The running firmware: the software name parsed into mode, sensor type and version (contract 02 §4).

#### DepzDevice.readMcuTemperature

```java
public double readMcuTemperature()
```

The MCU temperature the device last cached, °C (it refreshes about twice a second).

#### DepzDevice.syncTime

```java
public TimeSyncResult syncTime(int samples)
```

NTP-style time sync keeping the lowest-RTT of `samples` rounds (contract 02 §5).

#### DepzDevice.syncTime

```java
public TimeSyncResult syncTime()
```

#### DepzDevice.lastTimeSync

```java
public TimeSyncResult lastTimeSync()
```

The last `syncTime` result, or null.

#### DepzDevice.toHostTimeUs

```java
public long toHostTimeUs(long deviceTimestampUs)
```

Device µs → host monotonic µs (`hostNowUs`); needs a `syncTime` first.

#### DepzDevice.getReportPayloadCrc

```java
public CrcType getReportPayloadCrc()
```

The device → host payload CRC mode.

#### DepzDevice.setReportPayloadCrc

```java
public void setReportPayloadCrc(CrcType crcType)
```

Set the device → host payload CRC mode (host → device is per packet: `setTxCrcType`).

#### DepzDevice.getSyncPin

```java
public Common.SyncPinConfig getSyncPin(int pin)
```

#### DepzDevice.setSyncPin

```java
public void setSyncPin(Common.SyncPinConfig config)
```

#### DepzDevice.reset

```java
public void reset()
```

DEVICE_RESET: the device acknowledges, then reboots; the link drops.

#### DepzDevice.enterBootloaderMode

```java
public void enterBootloaderMode()
```

Reboot into the resident bootloader and close this connection (flashing: contract 06).

#### DepzDevice.hostNowUs

```java
public static long hostNowUs()
```

The host monotonic clock in µs — the host side of all time-sync math.

### DepzDevices

```java
public final class DepzDevices
```

Opening devices (contract 02 §4, contract 07 §2): the probe, class resolution and continuing a
connection as its sensor class.

#### DepzDevices.openOnLink

```java
public static DepzDevice openOnLink(Link link, Integer usbVid, Integer usbPid, Duration timeout)
```

Open a device on a connection that cannot be reopened — a replay or recording link, a link
the host opened (contract 02 §4.4). Runs the probe on `link`, resolves the class from
the answers and the port's USB id (null = unknown; a link knows none) and continues as that
class on the same connection. The device owns the link; on failure it is closed.

#### DepzDevices.openOnLink

```java
public static DepzDevice openOnLink(Link link)
```

#### DepzDevices.attach

```java
public static DepzDevice attach(DepzDevice dev, String className)
```

Continue the open plain device `dev` as `className` (a name
`Discovery#resolveSensorClass` returns) on the same connection — no reopen; the reader
and the host sequence numbers run on (contract 02 §4.4). Use the result from here on.
"device", and a class this SDK has no device layer for, return `dev` itself.

#### DepzDevices.probe

```java
public static ProbeResult probe(DepzDevice dev)
```

The contract 02 §4.4 probe on an open connection: GET_NAME_ACTIVE_SOFTWARE (required: its
failure throws), then GET_DEVICE_NAME and GET_SERIAL (a timeout or error leaves "").

#### DepzDevices.probePort

```java
public static DeviceInfo probePort(String port, Duration timeout)
```

Probe one port: open it, run `probe`, close. Null when nothing DEPZ-shaped answers.

#### DepzDevices.listDepzDevices

```java
public static List<DeviceInfo> listDepzDevices(List<String> ports, boolean matchUsb, Duration timeout)
```

Probe the candidate ports and return every DEPZ device found, ordered by USB serial. With
`matchUsb` only ports with a known DEPZ USB id are probed; `ports` (null:
enumerate) replaces enumeration with an explicit list.

#### DepzDevices.listDepzDevices

```java
public static List<DeviceInfo> listDepzDevices()
```

#### DepzDevices.openDevice

```java
public static DepzDevice openDevice(String serial, Duration timeout)
```

Open the DEPZ device with the smallest USB serial, or the one whose USB serial is
`serial`, as its sensor class (contract 02 §4.3).

#### DepzDevices.openDevice

```java
public static DepzDevice openDevice()
```

#### DepzDevices.openDevice

```java
public static DepzDevice openDevice(int index, Duration timeout)
```

Open the `index`-th DEPZ candidate, candidates sorted by USB serial.

#### DepzDevices.openPort

```java
public static DepzDevice openPort(String port, Duration timeout)
```

Open exactly `port` as its sensor class (also a port whose USB id is not a DEPZ one). The
USB id is that of the enumerated port the path names under any alias (`/dev/serial/by-id/…`;
contract 02 §4.3).

#### DepzDevices.openDevice

```java
public static DepzDevice openDevice(DeviceInfo info, Duration timeout)
```

Open the port of a `listDepzDevices` result as the class its probe answers resolve to.

#### DepzDevices.openSr04

```java
public static Sr04Device openSr04(String serial, Duration timeout)
```

`openDevice(String, Duration)`, and `WrongTypeException` unless it is an SR04.

#### DepzDevices.openSr04

```java
public static Sr04Device openSr04()
```

#### DepzDevices.openVl53l4cd

```java
public static Vl53l4cdDevice openVl53l4cd(String serial, Duration timeout)
```

`openDevice(String, Duration)`, and `WrongTypeException` unless it is a VL53L4CD.

#### DepzDevices.openVl53l4cd

```java
public static Vl53l4cdDevice openVl53l4cd()
```

#### DepzDevices.openVl53l8

```java
public static Vl53l8cxDevice openVl53l8(String serial, Duration timeout)
```

`openDevice(String, Duration)`, and `WrongTypeException` unless it is a VL53L8CX or
VL53L8CH (`Vl53l8chDevice` extends `Vl53l8cxDevice`).

#### DepzDevices.openVl53l8

```java
public static Vl53l8cxDevice openVl53l8()
```

#### DepzDevices.openVl53l7

```java
public static Vl53l7cxDevice openVl53l7(String serial, Duration timeout)
```

`openDevice(String, Duration)`, and `WrongTypeException` unless it is a VL53L5CX,
VL53L7CX or VL53L7CH (all extend `Vl53l7cxDevice`).

#### DepzDevices.openVl53l7

```java
public static Vl53l7cxDevice openVl53l7()
```

#### DepzDevices.openBno055

```java
public static Bno055Device openBno055(String serial, Duration timeout)
```

`openDevice(String, Duration)`, and `WrongTypeException` unless it is a BNO055.

#### DepzDevices.openBno055

```java
public static Bno055Device openBno055()
```

#### DepzDevices.openBno086

```java
public static Bno086Device openBno086(String serial, Duration timeout)
```

`openDevice(String, Duration)`, and `WrongTypeException` unless it is a BNO085 / BNO086.

#### DepzDevices.openBno086

```java
public static Bno086Device openBno086()
```

#### DepzDevices.openVl53lx

```java
public static Vl53lxDevice openVl53lx(String serial, Duration timeout)
```

`openDevice(String, Duration)`, and `WrongTypeException` unless it is a board of the
VL53L 1D family (every class extends `Vl53lxDevice`).

#### DepzDevices.openVl53lx

```java
public static Vl53lxDevice openVl53lx()
```

#### DepzDevices.openTmf8829

```java
public static Tmf8829Device openTmf8829(String serial, Duration timeout)
```

`openDevice(String, Duration)`, and `WrongTypeException` unless it is a TMF8829.

#### DepzDevices.openTmf8829

```java
public static Tmf8829Device openTmf8829()
```

### DepzDevices.ProbeResult

```java
public record ProbeResult(String softwareName, String deviceName, String serialNumber)
```

The probe's answers: software name, device name, serial number.

### DepzException

```java
public class DepzException extends RuntimeException
```

Base of every live-layer error (contract 07). Unchecked. Arguments the SDK refuses before
sending anything throw `IllegalArgumentException` instead.

### DepzTimeoutException

```java
public class DepzTimeoutException extends DepzException
```

No reply to a request within its timeout (default 200 ms).

#### DepzTimeoutException.cmd

```java
public int cmd()
```

The request opcode that went unanswered (-1 when not a request).

### DeviceEvent

```java
public interface DeviceEvent
```

The unsolicited / diagnostic events a device emits (contract 07 §2). The common ones are the
nested records; a sensor class adds its own.

### DeviceInfo

```java
public record DeviceInfo(String port, String mode, Identity.SensorType sensorType, String softwareName, String fwVersion, String deviceName, String serialNumber, Integer usbVid, Integer usbPid, String usbSerial)
```

What the contract 02 §4.4 probe learned about one port.

#### DeviceInfo.usbModelHint

```java
public String usbModelHint()
```

The model the USB-id table gives the port (contract 02 §4.2), or null.

### DeviceLostException

```java
public class DeviceLostException extends DepzException
```

The connection went away: unplugged, closed, or the OS refused I/O.

### InvalidReplyException

```java
public class InvalidReplyException extends DepzException
```

The reply to a request does not decode (contract 07 §3): shorter than the fields its contract
defines, or a value the decoder refuses. Only that request fails; the reader goes on and the
device stays usable.

#### InvalidReplyException.cmd

```java
public int cmd()
```

The request opcode whose reply was refused.

### InvalidStateException

```java
public final class InvalidStateException extends DepzException
```

The call is not valid in the current state of the device or object (contract 07 §3):
`toHostTimeUs` before `syncTime`, a sensor call before `init()` /
`configure()` or while streaming, `add()` after recording started, a closed
dataset writer or player, `await` without the player thread, a write to a read-only
register block, a VL53Lx board name without a product number, a packaged firmware blob of
the wrong size or missing from the classpath.

### Link

```java
public interface Link
```

A blocking byte pipe to one device: a serial port, a `.depzrec` record/replay wrapper,
or anything a host supplies. Device classes accept any link, which keeps every layer above the
port testable without hardware.

### LinkClosedException

```java
public final class LinkClosedException extends DeviceLostException
```

I/O on a link that is already closed.

### LinkStats

```java
public final class LinkStats
```

Link counters (contract 07 §2 `stats`); updated by the reader thread and the sender.

#### LinkStats.txPackets

```java
public long txPackets()
```

#### LinkStats.rxPackets

```java
public long rxPackets()
```

#### LinkStats.txBytes

```java
public long txBytes()
```

#### LinkStats.rxBytes

```java
public long rxBytes()
```

#### LinkStats.crcErrors

```java
public long crcErrors()
```

#### LinkStats.headerErrors

```java
public long headerErrors()
```

#### LinkStats.trashBytes

```java
public long trashBytes()
```

#### LinkStats.seqGaps

```java
public long seqGaps()
```

Gaps in the device → host sequence numbers, seen by the host.

#### LinkStats.deviceSeqErrors

```java
public long deviceSeqErrors()
```

RPT_SEQUENCE_ERROR reports: gaps the device saw in host → device packets.

#### LinkStats.callbackErrors

```java
public long callbackErrors()
```

Callbacks that threw on the reader thread (contract 07 §3).

### NoDepzDeviceException

```java
public final class NoDepzDeviceException extends DepzException
```

Discovery found no DEPZ device to open.

### RecordingLink

```java
public final class RecordingLink implements Link
```

Wraps any link and tees both directions into a `.depzrec` file (contract 08). TX is
journaled before the physical write: a fast device reply could otherwise reach the reader and
be journaled ahead of its own request, which breaks the causal rx gating replay relies on.

#### RecordingLink.SCHEMA *(field)*

```java
public static final String SCHEMA
```

### ReplayLink

```java
public final class ReplayLink implements Link
```

Replays the rx side of a `.depzrec` capture causally (contract 08): an rx chunk is
served only after the host has written as many tx bytes as preceded it in the capture. With
`strictTx` every written byte must also equal the recorded tx stream
(`ReplayMismatchException`).

#### ReplayLink.header

```java
public Map<String, Object> header()
```

The capture's header object (contract 08).

#### ReplayLink.rxExhausted

```java
public synchronized boolean rxExhausted()
```

Every rx chunk was delivered.

#### ReplayLink.rxRemaining

```java
public synchronized int rxRemaining()
```

Recorded rx chunks not delivered yet.

#### ReplayLink.txRemaining

```java
public synchronized long txRemaining()
```

Recorded tx bytes the host has not written yet.

### ReplayMismatchException

```java
public final class ReplayMismatchException extends DepzException
```

A `ReplayLink` in strict mode saw the SDK write bytes the capture does not hold.

### SerialPortLink

```java
public final class SerialPortLink implements Link
```

A serial port as a `Link` (jSerialComm: Windows, macOS, Linux). 115200 8N1 with DTR and
RTS raised; the USB CDC boards ignore the baud rate.

### SerialPorts

```java
public final class SerialPorts
```

Serial port enumeration with USB ids (jSerialComm).

#### SerialPorts.list

```java
public static List<SerialPortInfo> list()
```

Every serial port the OS lists.

#### SerialPorts.find

```java
public static SerialPortInfo find(List<SerialPortInfo> ports, String port)
```

The enumerated port `port` names, under any alias (contract 02 §4.3, ERRATA A5): the one
whose path is the same string, else — on POSIX — the one whose resolved path is the same
(`/dev/serial/by-id/…` is a symlink to the node), else the one that is the same character
device under another name. On Windows a COM name is matched by string only: a COM port is not
a file, and resolving one (toRealPath, readAttributes) opens it — a DTR pulse on every board of
the bench. Null when the OS does not list the port.

#### SerialPorts.bySerial

```java
public static List<SerialPortInfo> bySerial(List<SerialPortInfo> ports)
```

Ports by USB iSerial ascending, those without one last, ties by path (contract 02 §4).

### SerialPorts.SerialPortInfo

```java
public record SerialPortInfo(String port, Integer vid, Integer pid, String usbSerial)
```

One serial port: its path, and its USB ids and iSerial when it is a USB device (else null).

### StatusException

```java
public class StatusException extends DepzException
```

The device answered a request with a non-OK RPT_STATUS (contract 02 §3).

#### StatusException.cmd

```java
public int cmd()
```

#### StatusException.status

```java
public int status()
```

#### StatusException.statusName

```java
public String statusName()
```

The contract 02 §3 name of `status()`, e.g. `ERR_INVALID_PARAM`.

### Subscription

```java
public final class Subscription<T> implements AutoCloseable
```

A pull subscription to one stream (contract 07 §3): a bounded queue that drops the oldest
item when full and counts the drops. It is registered when created, so nothing produced after
that is missed. Poll it with `tryTake` (never blocks) or wait with `take`;
`close` unsubscribes.

#### Subscription.droppedCount

```java
public synchronized long droppedCount()
```

Items dropped because the queue was full (monotonic).

#### Subscription.size

```java
public synchronized int size()
```

Items waiting.

#### Subscription.isClosed

```java
public synchronized boolean isClosed()
```

The device closed (or this was closed): no more items will arrive.

#### Subscription.tryTake

```java
public synchronized Optional<T> tryTake()
```

The oldest waiting item, without blocking.

#### Subscription.take

```java
public synchronized T take(Duration timeout)
```

The oldest item, waiting up to `timeout` for one.

#### Subscription.drain

```java
public synchronized List<T> drain()
```

Everything waiting now, oldest first, without blocking.

### TimeSyncResult

```java
public record TimeSyncResult(long offsetUs, long rttUs, long syncedAtHostUs)
```

Result of `DepzDevice#syncTime`: device clock − host clock, the round trip of the kept
sample, and when (host µs) it was taken.

### WrongTypeException

```java
public final class WrongTypeException extends DepzException
```

The opened device is not the sensor class the caller asked for.

## SR04

### Sr04Device

```java
public final class Sr04Device extends DepzDevice
```

HC-SR04 ultrasonic ranger (contract 03). Measurements stream to `onMeasurement`
callbacks and `measurements` queues: loop samples and the single shots an AUX SYNC_IN
edge triggers.

#### Sr04Device.getSamplePeriodUs

```java
public long getSamplePeriodUs()
```

The stored minimum interval between measurement starts, µs (default 50000) — not the
effective rate, which the echo window throttles (contract 03 §3).

#### Sr04Device.setSamplePeriodUs

```java
public void setSamplePeriodUs(long periodUs)
```

Set the minimum interval between measurement starts, µs (u32 wire field).

#### Sr04Device.getEchoDecayUs

```java
public int getEchoDecayUs()
```

The post-echo settle pause, µs (4000–65000).

#### Sr04Device.setEchoDecayUs

```java
public int setEchoDecayUs(int decayUs)
```

Set the settle pause. The device clamps it to 4000–65000 µs silently, so this reads it back
and returns the value in effect. Must fit the u16 wire field.

#### Sr04Device.measureOnce

```java
public Sr04Measurement measureOnce(Duration timeout)
```

One shot. The reply comes when the echo completes (or times out at ~65.5 ms), hence the 1 s
default. `BusyException` while the loop runs.

#### Sr04Device.measureOnce

```java
public Sr04Measurement measureOnce()
```

#### Sr04Device.start

```java
public void start()
```

Start the measurement loop (idempotent).

#### Sr04Device.stop

```java
public void stop()
```

Stop the measurement loop (idempotent).

#### Sr04Device.onMeasurement

```java
public AutoCloseable onMeasurement(Consumer<Sr04Measurement> callback)
```

Call `callback` for every measurement (reader thread). Close the result to stop.

#### Sr04Device.measurements

```java
public Subscription<Sr04Measurement> measurements(int capacity)
```

A pull queue of measurements (bounded, drop-oldest); subscribed when this returns.

#### Sr04Device.measurements

```java
public Subscription<Sr04Measurement> measurements()
```

#### Sr04Device.getMeasurement

```java
public Sr04Measurement getMeasurement(Duration timeout)
```

The next measurement (contract 07 §3).

#### Sr04Device.streamDroppedCounts

```java
public List<Long> streamDroppedCounts()
```

The drop counter of each open `measurements` queue.

### Sr04Measurement

```java
public record Sr04Measurement(long timestampUs, int echoTimeUs, String source)
```

One SR04 ranging result. `source` is `"once"` (a host command or a SYNC_IN edge) or
`"loop"`; `valid()` is false for the no-echo timeout.

#### Sr04Measurement.valid

```java
public boolean valid()
```

#### Sr04Measurement.distanceMm

```java
public Double distanceMm()
```

Distance at 343 m/s, or null when no echo came back.

#### Sr04Measurement.distanceMmAt

```java
public Double distanceMmAt(double airTempC)
```

Distance with the speed of sound for `airTempC` °C.

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

`periodUs`: u32; outside 0..2^32-1 → IllegalArgumentException.

#### Sr04.unpackSamplePeriod

```java
public static long unpackSamplePeriod(byte[] p)
```

Reads the u32 sample period as an unsigned value in a long.

#### Sr04.packEchoDecay

```java
public static byte[] packEchoDecay(int decayUs)
```

`decayUs`: u16; outside 0..65535 → IllegalArgumentException.

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
then `fw_size == payload length`, then section 0 of 1 (sections
are 0-based; multi-section files are refused).

## VL53L4CD (ToF)

### Vl53l4Measurement

```java
public record Vl53l4Measurement(long timestampUs, int rangeStatus, int distanceMm, int sigmaMm, int signalRateKcps, int ambientRateKcps, int signalPerSpadKcps, int ambientPerSpadKcps, int numberOfSpad, int streamCount)
```

One VL53L4CD ranging result (`VL53L4CD_ResultsData_t` plus the MCU timestamp: of the INT
edge when streamed, of the result read when polled).

#### Vl53l4Measurement.valid

```java
public boolean valid()
```

#### Vl53l4Measurement.statusText

```java
public String statusText()
```

What `rangeStatus()` means (`Vl53l4Uld#RANGE_STATUS_NAMES`).

### Vl53l4cdDevice

```java
public final class Vl53l4cdDevice extends DepzDevice
```

VL53L4CD single-zone ToF (contract 10). The board is a thin I2C bridge; the ST ULD 2.2.3 runs
here (`uld()`). `init()` first, then configure and `startRanging()`: one
result block per INT edge streams to `measurements`. Configuration while ranging throws
`InvalidStateException` — the stream owns the register bank.

#### Vl53l4cdDevice.uld

```java
public Vl53l4cdDriver uld()
```

The ULD driver (escape hatch for raw register access).

#### Vl53l4cdDevice.initialized

```java
public boolean initialized()
```

True after a successful `init()`; cleared by `xshut` — a power-cycled sensor holds none of it.

#### Vl53l4cdDevice.ranging

```java
public boolean ranging()
```

#### Vl53l4cdDevice.isAlive

```java
public boolean isAlive()
```

True when the sensor answers with the VL53L4CD model id (0xEBAA).

#### Vl53l4cdDevice.init

```java
public void init(int busKhz)
```

Default configuration block + VHV calibration (ULD sensor_init); the bus is left at `busKhz`.

#### Vl53l4cdDevice.init

```java
public void init()
```

#### Vl53l4cdDevice.xshut

```java
public void xshut(int action)
```

Drive XSHUT: `Vl53l4#XSHUT_OFF` / `Vl53l4#XSHUT_ON` / `Vl53l4#XSHUT_RESET`.
Off and Reset stop a stream; `init()` again after.

#### Vl53l4cdDevice.resetSensor

```java
public void resetSensor()
```

Hardware reset through XSHUT (≤ 20 ms on the MCU, typ. ~2 ms); call `init()` again.

#### Vl53l4cdDevice.bridgeInfo

```java
public Vl53l4.Vl53l4Info bridgeInfo()
```

RPT_VL53_INFO: sensor identity, pin levels and bridge counters (`i2cErrors` is cleared by a successful XSHUT reset). Safe while streaming.

#### Vl53l4cdDevice.setI2cSpeedKhz

```java
public void setI2cSpeedKhz(int khz)
```

Re-time the bridge's I2C bus to the step nearest `khz`. Not while ranging.

#### Vl53l4cdDevice.getRangeTiming

```java
public RangeTiming getRangeTiming()
```

(timing budget ms, inter-measurement ms; 0 = continuous).

#### Vl53l4cdDevice.setRangeTiming

```java
public void setRangeTiming(int timingBudgetMs, long interMeasurementMs)
```

Timing budget 10–200 ms; inter-measurement 0 = continuous, larger than the budget = autonomous low power.

#### Vl53l4cdDevice.setRangeTiming

```java
public void setRangeTiming(int timingBudgetMs)
```

#### Vl53l4cdDevice.getOffsetMm

```java
public int getOffsetMm()
```

#### Vl53l4cdDevice.setOffsetMm

```java
public void setOffsetMm(int offsetMm)
```

#### Vl53l4cdDevice.getXtalkKcps

```java
public int getXtalkKcps()
```

Crosstalk compensation, kcps (0 = off).

#### Vl53l4cdDevice.setXtalkKcps

```java
public void setXtalkKcps(int xtalkKcps)
```

#### Vl53l4cdDevice.getDetectionThresholds

```java
public DetectionThresholds getDetectionThresholds()
```

#### Vl53l4cdDevice.setDetectionThresholds

```java
public void setDetectionThresholds(int distanceLowMm, int distanceHighMm, int window)
```

The distance-window interrupt: INT only fires while the window holds.

#### Vl53l4cdDevice.getSignalThresholdKcps

```java
public int getSignalThresholdKcps()
```

#### Vl53l4cdDevice.setSignalThresholdKcps

```java
public void setSignalThresholdKcps(int signalKcps)
```

Drop measurements whose return signal is below `signalKcps`.

#### Vl53l4cdDevice.getSigmaThresholdMm

```java
public int getSigmaThresholdMm()
```

#### Vl53l4cdDevice.setSigmaThresholdMm

```java
public void setSigmaThresholdMm(int sigmaMm)
```

Drop measurements whose sigma exceeds `sigmaMm` (≤ 16383).

#### Vl53l4cdDevice.startTemperatureUpdate

```java
public void startTemperatureUpdate()
```

Re-run VHV calibration, recommended after a >8 °C ambient change.

#### Vl53l4cdDevice.calibrateOffset

```java
public int calibrateOffset(int targetDistMm, int nbSamples)
```

Offset calibration against a target at `targetDistMm` (10–1000 mm); returns the offset now programmed.

#### Vl53l4cdDevice.calibrateOffset

```java
public int calibrateOffset(int targetDistMm)
```

#### Vl53l4cdDevice.calibrateXtalk

```java
public int calibrateXtalk(int targetDistMm, int nbSamples)
```

Crosstalk calibration against a target at `targetDistMm` (10–5000 mm); returns the xtalk now programmed, kcps.

#### Vl53l4cdDevice.calibrateXtalk

```java
public int calibrateXtalk(int targetDistMm)
```

#### Vl53l4cdDevice.startRanging

```java
public void startRanging()
```

Start the sensor and arm the MCU stream: one RPT_VL53_STREAM per INT edge.

#### Vl53l4cdDevice.stopRanging

```java
public void stopRanging()
```

#### Vl53l4cdDevice.measureOnce

```java
public Vl53l4Measurement measureOnce(Duration timeout)
```

One polled measurement: start, wait for data ready, read, stop. Not while streaming.

#### Vl53l4cdDevice.measureOnce

```java
public Vl53l4Measurement measureOnce()
```

#### Vl53l4cdDevice.onMeasurement

```java
public AutoCloseable onMeasurement(Consumer<Vl53l4Measurement> callback)
```

Call `callback` for every streamed measurement (reader thread).

#### Vl53l4cdDevice.measurements

```java
public Subscription<Vl53l4Measurement> measurements(int capacity)
```

A pull queue of streamed measurements (bounded, drop-oldest).

#### Vl53l4cdDevice.measurements

```java
public Subscription<Vl53l4Measurement> measurements()
```

#### Vl53l4cdDevice.getMeasurement

```java
public Vl53l4Measurement getMeasurement(Duration timeout)
```

The next streamed measurement (contract 07 §3).

#### Vl53l4cdDevice.streamParseErrors

```java
public long streamParseErrors()
```

Stream reports dropped because their result block did not decode.

#### Vl53l4cdDevice.streamDroppedCounts

```java
public List<Long> streamDroppedCounts()
```

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

VL53_XSHUT action: pulse low then wait for the boot handshake (≤ 20 ms, typ. ~2 ms).

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

VL53_READ_REG payload `addr u16, len u16` with the transfer limit of contract 10 §1:
`len` 1..253 and `addr + len <= 0x10000`, else IllegalArgumentException.

#### Vl53l4.packWriteReg

```java
public static byte[] packWriteReg(int addr, byte[] data)
```

VL53_WRITE_REG payload: `addr u16, data[1..253]` with `addr + N <= 0x10000`, else IllegalArgumentException.

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

RPT_VL53_INFO — bridge diagnostics (21-byte LE payload). Counters wrap
silently; watch increments, not absolute values. `i2cErrors` counts
since the last successful XSHUT reset (the bridge clears it there), not
since power-up; `initialized` = 1 when MODEL_ID read back as 0xEBAA
on this GET_INFO.
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

Rejects a payload shorter than its header or than the `len` data bytes it announces.

### DetectionThresholds

```java
public record DetectionThresholds(int distanceLowMm, int distanceHighMm, int window)
```

The distance-window interrupt: low and high distance, mm, and the window
(`Vl53l4Uld#WINDOW_BELOW` … `Vl53l4Uld#WINDOW_IN`).

### RangeTiming

```java
public record RangeTiming(int timingBudgetMs, int interMeasurementMs)
```

A ranging timing: the budget of one measurement and the period between two (0 = continuous), ms.

### Vl53l4Platform

```java
public interface Vl53l4Platform
```

What the ULD needs from the board: the register bridge (16-bit addresses, auto-increment), a
delay, and the bus speed. `Vl53l4cdDevice` maps it onto the firmware bridge; a test can
script it.

### Vl53l4Uld

```java
public final class Vl53l4Uld
```

VL53L4CD ULD (ST STSW-IMG026 2.2.3) — the register math under the driver.

Port of the pure codec/math pieces of `VL53L4CD_api.c` that are
hardware-independent and covered by golden vectors: the 17-byte result-block
decode (`parseResultBlock`), the SetRangeTiming/GetRangeTiming
register math (`rangeTimingRegisters`, `decodeRangeTiming`),
the tuning-register word codecs and the 91-byte init configuration block
(`configBlock`). Semantics mirror the Python reference
(`depz_sensor_sdk/vl53l4/uld.py`) 1:1 — integer widths and 32-bit
truncations included; do not "simplify" the math.

The live driver over a register bridge (sensor_init, VHV calibration,
offset / xtalk calibration loops) is `Vl53l4cdDriver`.

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

#### Vl53l4Uld.I2C_KHZ_BOOT *(field)*

```java
public static final int I2C_KHZ_BOOT = 400
```

The bus speed an unconfigured sensor is specified for (sensor_init writes its block at it).

#### Vl53l4Uld.I2C_KHZ_DEFAULT *(field)*

```java
public static final int I2C_KHZ_DEFAULT = 1000
```

Recommended post-init bus speed (the result-block read is ~4× faster).

#### Vl53l4Uld.RANGE_STATUS_NAMES *(field)*

```java
public static final java.util.Map<Integer, String> RANGE_STATUS_NAMES
```

Range status (after the ST mapping) → its meaning; 0 is a valid measurement.

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
`Vl53l4Error` when `raw` is shorter than `RESULT_BLOCK_LEN` (17) bytes
(contract 10 §5); a longer buffer is decoded and the tail ignored.

#### Vl53l4Uld.checkRangeTiming

```java
public static void checkRangeTiming(int timingBudgetMs, long interMeasurementMs)
```

SetRangeTiming's own argument checks (ST's VL53L4CD_ERROR_INVALID_ARGUMENT),
which need no register: budget 10..200 ms, period 0 (continuous) or above
the budget (autonomous) and a u32 — `IllegalArgumentException`
otherwise (contract 10 §5).

#### Vl53l4Uld.rangeTimingRegisters

```java
public static int[] rangeTimingRegisters(int timingBudgetMs, long interMeasurementMs, int oscFrequency, int clockPll)
```

SetRangeTiming register math → `{RANGE_CONFIG_A, RANGE_CONFIG_B,
INTERMEASUREMENT_MS raw dword}`.

`oscFrequency` is the word read from 0x0006; `clockPll`
is the word read from RESULT__OSC_CALIBRATE_VAL (used only in autonomous
mode, i.e. when `interMeasurementMs > 0`). Budget 10..200 ms;
`interMeasurementMs` 0 (continuous) or greater than the budget
(autonomous low power) — anything else throws
`IllegalArgumentException` (`checkRangeTiming`).
Register words are refused out of range, never wrapped: `oscFrequency`
1..65535 (0 reads as `Vl53l4Error`), `clockPll` 0..65535, and
an INTERMEASUREMENT_MS dword that would overflow u32 throws
`IllegalArgumentException` too.
The third element (the dword) is a u32 in Java's `int` convention
(`Integer#toUnsignedLong`).

#### Vl53l4Uld.decodeRangeTiming

```java
public static int[] decodeRangeTiming(long intermeasurementRaw, int clockPll, int oscFrequency, int rangeConfigA)
```

GetRangeTiming register math → `{timing_budget_ms,
inter_measurement_ms}`.

Inputs are the raw register reads: INTERMEASUREMENT_MS dword, the
RESULT__OSC_CALIBRATE_VAL word, the 0x0006 word and the RANGE_CONFIG_A
word. An oscillator word of 0 throws `Vl53l4Error`; a value wider
than its register (dword u32, words u16) `IllegalArgumentException`.

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

#### Vl53l4Uld.roundHalfAway

```java
public static int roundHalfAway(double x)
```

C's round(): an exact .5 goes away from zero. ST's ULD rounds xtalk this way; Java's
`Math.round` sends a negative half up instead.

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

### Vl53l4Uld.Vl53l4Error

```java
public static final class Vl53l4Error extends RuntimeException
```

What the sensor did, not what the caller asked: a boot or data-ready
timeout, a short result block, an oscillator word of 0, a failed xtalk
calibration. A refused argument is an `IllegalArgumentException`
(contract 07 §3).

### Vl53l4Uld.Results

```java
public record Results(int rangeStatus, int distanceMm, int ambientRateKcps, int ambientPerSpadKcps, int signalRateKcps, int signalPerSpadKcps, int numberOfSpad, int sigmaMm, int streamCount)
```

VL53L4CD_ResultsData_t plus the sensor's own frame counter.

### Vl53l4cdDriver

```java
public final class Vl53l4cdDriver
```

The ST VL53L4CD ULD 2.2.3 (`VL53L4CD_api.c` + `VL53L4CD_calibration.c`) on the host,
ported from the reference Python driver: init, ranging, timing, offset, crosstalk, thresholds,
temperature update, calibration. Every request sequence is the Python one, so captures of either
replay in both. Register contents are big-endian.

#### Vl53l4cdDriver.readByte

```java
public int readByte(int addr)
```

#### Vl53l4cdDriver.readWord

```java
public int readWord(int addr)
```

#### Vl53l4cdDriver.readDword

```java
public long readDword(int addr)
```

#### Vl53l4cdDriver.writeByte

```java
public void writeByte(int addr, int value)
```

#### Vl53l4cdDriver.writeWord

```java
public void writeWord(int addr, int value)
```

#### Vl53l4cdDriver.writeDword

```java
public void writeDword(int addr, long value)
```

#### Vl53l4cdDriver.getSensorId

```java
public int getSensorId()
```

#### Vl53l4cdDriver.isAlive

```java
public boolean isAlive()
```

#### Vl53l4cdDriver.waitBoot

```java
public void waitBoot(int timeoutMs)
```

Polls FIRMWARE__SYSTEM_STATUS until the sensor has booted, once per millisecond.

#### Vl53l4cdDriver.sensorInit

```java
public void sensorInit(int busKhz)
```

Initialise the sensor and leave the bus at `busKhz`. The configuration block goes at
400 kHz, the only speed an unconfigured sensor is specified for; the bridge is re-timed after.

#### Vl53l4cdDriver.sensorInit

```java
public void sensorInit()
```

#### Vl53l4cdDriver.clearInterrupt

```java
public void clearInterrupt()
```

#### Vl53l4cdDriver.startRanging

```java
public void startRanging()
```

#### Vl53l4cdDriver.stopRanging

```java
public void stopRanging()
```

#### Vl53l4cdDriver.checkForDataReady

```java
public boolean checkForDataReady()
```

#### Vl53l4cdDriver.waitDataReady

```java
public void waitDataReady(int timeoutMs)
```

Polls the data-ready flag, once per millisecond.

#### Vl53l4cdDriver.getResult

```java
public Vl53l4Uld.Results getResult()
```

One block read instead of the C driver's six register reads; the decoding is identical.

#### Vl53l4cdDriver.setRangeTiming

```java
public void setRangeTiming(int timingBudgetMs, long interMeasurementMs)
```

#### Vl53l4cdDriver.getRangeTiming

```java
public RangeTiming getRangeTiming()
```

(timing budget ms, inter-measurement ms; 0 = continuous).

#### Vl53l4cdDriver.setOffset

```java
public void setOffset(int offsetMm)
```

#### Vl53l4cdDriver.getOffset

```java
public int getOffset()
```

#### Vl53l4cdDriver.setXtalk

```java
public void setXtalk(int xtalkKcps)
```

#### Vl53l4cdDriver.getXtalk

```java
public int getXtalk()
```

#### Vl53l4cdDriver.setDetectionThresholds

```java
public void setDetectionThresholds(int distanceLowMm, int distanceHighMm, int window)
```

The distance-window interrupt; `window` is one of `Vl53l4Uld#WINDOW_BELOW` … `Vl53l4Uld#WINDOW_IN`.

#### Vl53l4cdDriver.getDetectionThresholds

```java
public DetectionThresholds getDetectionThresholds()
```

#### Vl53l4cdDriver.setSignalThreshold

```java
public void setSignalThreshold(int signalKcps)
```

#### Vl53l4cdDriver.getSignalThreshold

```java
public int getSignalThreshold()
```

#### Vl53l4cdDriver.setSigmaThreshold

```java
public void setSigmaThreshold(int sigmaMm)
```

#### Vl53l4cdDriver.getSigmaThreshold

```java
public int getSigmaThreshold()
```

#### Vl53l4cdDriver.startTemperatureUpdate

```java
public void startTemperatureUpdate()
```

Re-run VHV; recommended after a >8 °C ambient change (ST Example_3).

#### Vl53l4cdDriver.calibrateOffset

```java
public int calibrateOffset(int targetDistMm, int nbSamples)
```

Offset calibration against a target at `targetDistMm` (10–1000); returns the offset now programmed, mm.

#### Vl53l4cdDriver.calibrateXtalk

```java
public int calibrateXtalk(int targetDistMm, int nbSamples)
```

Crosstalk calibration against a target at `targetDistMm` (10–5000); returns the xtalk now programmed, kcps (C rounding).

## VL53L8 (ToF)

### Vl53l8chDevice

```java
public class Vl53l8chDevice extends Vl53l8cxDevice
```

VL53L8CH: the VL53L8CX superset (every CX method) plus CNH, compact network histograms — each
frame can carry a per-aggregate distance histogram. `init()` downloads the CH firmware
(VL53LMZ ULD 2.0.16).

#### Vl53l8chDevice.cnhSetup

```java
public Vl53l8CnhSetup cnhSetup()
```

The CNH setup armed by `configureCnh`, or null; recorders keep it next to the raw blocks to decode them later.

#### Vl53l8chDevice.configureCnh

```java
public void configureCnh(Vl53l8CnhSetup setup)
```

Arm CNH for the next `startRanging()`: every frame then carries
`Vl53l8Frame#cnhRaw()`, decoded with `Vl53l8Uld.decodeCnh`. CNH and the motion
indicator share one DCI struct (0xBFAC): this disarms the motion indicator (frames carry no
`Vl53l8Frame#motion()`).

### Vl53l8cxDevice

```java
public class Vl53l8cxDevice extends DepzDevice
```

VL53L8CX multizone ToF (contract 04). The board is a thin SPI bridge; the ST ULD runs here
(`uld()`). `init()` downloads the ~84 KB sensor firmware (about a second), then
configure and `startRanging()`: the board pushes one frame per INT edge in chunks,
reassembled and parsed into `frames`. Configuration needs `init()` and throws
`InvalidStateException` while ranging — the stream owns the register bank. The VL53L8CH
superset is `Vl53l8chDevice`.

#### Vl53l8cxDevice.MIN_RANGING_FREQUENCY_HZ *(field)*

```java
public static final int MIN_RANGING_FREQUENCY_HZ = 2
```

Below this the sensor never enters its ranging loop and streams nothing (contract 04).

#### Vl53l8cxDevice.maxRangingFrequencyHz

```java
public static int maxRangingFrequencyHz(int resolution)
```

Highest ranging frequency at `resolution` zones (contract 11 §3; the same silicon
limit on VL53L8): 60 Hz at 4×4, 15 Hz at 8×8.

#### Vl53l8cxDevice.firmwareSet

```java
protected Vl53l8Firmware firmwareSet()
```

The sensor firmware `init()` downloads.

#### Vl53l8cxDevice.readChunk

```java
protected int readChunk()
```

Register bytes per READ_REG (the I2C L5/L7 board reads less per call).

#### Vl53l8cxDevice.writeChunk

```java
protected int writeChunk()
```

Register bytes per WRITE_REG.

#### Vl53l8cxDevice.streamChunk

```java
protected int streamChunk()
```

Largest RPT_VL53_FRAME chunk the board emits (contract 04 §4 rule 4).

#### Vl53l8cxDevice.minRangingHz

```java
protected int minRangingHz()
```

The lowest ranging frequency that streams on this sensor.

#### Vl53l8cxDevice.uld

```java
public Vl53l8Driver uld()
```

The ULD driver (escape hatch for DCI access). @throws InvalidStateException before `init()`

#### Vl53l8cxDevice.uldOrNull

```java
protected Vl53l8Driver uldOrNull()
```

The driver after `init()`, or null.

#### Vl53l8cxDevice.firmware

```java
public Vl53l8Firmware firmware()
```

The firmware set in use after `init()` (cx / ch / l7cx / l7ch).

#### Vl53l8cxDevice.ranging

```java
public boolean ranging()
```

#### Vl53l8cxDevice.isAlive

```java
public boolean isAlive()
```

True when the sensor answers with its own silicon ids (a bus error or a foreign id: false).

#### Vl53l8cxDevice.init

```java
public void init(Consumer<String> progress, BiConsumer<Integer, Integer> writeProgress)
```

Download the sensor firmware and the default configuration. `progress` gets phase
texts, `writeProgress` (done, total) the big firmware writes; either may be null.

#### Vl53l8cxDevice.init

```java
public void init()
```

#### Vl53l8cxDevice.getResolution

```java
public int getResolution()
```

Zones in use: 16 (4×4) or 64 (8×8).

#### Vl53l8cxDevice.setResolution

```java
public void setResolution(int zones)
```

16 (4×4) or 64 (8×8) zones.

#### Vl53l8cxDevice.getRangingFrequencyHz

```java
public int getRangingFrequencyHz()
```

#### Vl53l8cxDevice.setRangingFrequencyHz

```java
public void setRangingFrequencyHz(int hz)
```

At least 2 Hz (1 Hz on L5/L7); at most `maxRangingFrequencyHz` of the resolution last
set or read through this class (4×4 after init) — 60 Hz at 4×4, 15 Hz at 8×8 (contract 11
§3). Refused host-side, nothing reaches the bus.

#### Vl53l8cxDevice.getRangingMode

```java
public int getRangingMode()
```

`Vl53l8Driver#RANGING_MODE_CONTINUOUS` or `Vl53l8Driver#RANGING_MODE_AUTONOMOUS`.

#### Vl53l8cxDevice.setRangingMode

```java
public void setRangingMode(int mode)
```

#### Vl53l8cxDevice.getIntegrationTimeMs

```java
public int getIntegrationTimeMs()
```

#### Vl53l8cxDevice.setIntegrationTimeMs

```java
public void setIntegrationTimeMs(int ms)
```

2–1000 ms; autonomous mode only.

#### Vl53l8cxDevice.getSharpenerPercent

```java
public int getSharpenerPercent()
```

#### Vl53l8cxDevice.setSharpenerPercent

```java
public void setSharpenerPercent(int pct)
```

Edge sharpener, 0–99 % (0 = off).

#### Vl53l8cxDevice.getTargetOrder

```java
public int getTargetOrder()
```

`Vl53l8Driver#TARGET_ORDER_CLOSEST` or `Vl53l8Driver#TARGET_ORDER_STRONGEST`.

#### Vl53l8cxDevice.setTargetOrder

```java
public void setTargetOrder(int order)
```

#### Vl53l8cxDevice.getPowerMode

```java
public int getPowerMode()
```

`Vl53l8Driver#POWER_MODE_SLEEP`, `Vl53l8Driver#POWER_MODE_WAKEUP` or `Vl53l8Driver#POWER_MODE_DEEP_SLEEP`.

#### Vl53l8cxDevice.setPowerMode

```java
public void setPowerMode(int mode)
```

Waking from deep sleep downloads the firmware again.

#### Vl53l8cxDevice.getXtalkMargin

```java
public int getXtalkMargin()
```

Xtalk margin, whole kcps/SPAD (as ST).

#### Vl53l8cxDevice.setXtalkMargin

```java
public void setXtalkMargin(int marginKcps)
```

Xtalk margin, a whole number of kcps/SPAD, 0..10000 (as ST); anything else is refused.

#### Vl53l8cxDevice.calibrateXtalk

```java
public void calibrateXtalk(int reflectancePercent, int nbSamples, int distanceMm)
```

On-device crosstalk calibration against a flat target (`Vl53l8Driver#calibrateXtalk`); read the result with `getCaldataXtalk()`.

#### Vl53l8cxDevice.getCaldataXtalk

```java
public byte[] getCaldataXtalk()
```

The 776-byte xtalk calibration buffer (save / restore).

#### Vl53l8cxDevice.setCaldataXtalk

```java
public void setCaldataXtalk(byte[] blob)
```

#### Vl53l8cxDevice.getDetectionThresholdsEnable

```java
public int getDetectionThresholdsEnable()
```

#### Vl53l8cxDevice.setDetectionThresholdsEnable

```java
public void setDetectionThresholdsEnable(boolean enabled)
```

#### Vl53l8cxDevice.getDetectionThresholds

```java
public List<Vl53l8Uld.DetectionThreshold> getDetectionThresholds()
```

#### Vl53l8cxDevice.setDetectionThresholds

```java
public void setDetectionThresholds(List<Vl53l8Uld.DetectionThreshold> thresholds)
```

The 64 detection thresholds (interrupt on threshold).

#### Vl53l8cxDevice.setDetectionThresholdsAutoStop

```java
public void setDetectionThresholdsAutoStop(boolean autoStop)
```

#### Vl53l8cxDevice.configureMotionIndicator

```java
public Vl53l8Uld.MotionConfig configureMotionIndicator(int distanceMinMm, int distanceMaxMm)
```

Enable the motion indicator over [min, max] mm; every frame then carries
`Vl53l8Frame#motion()`. Returns the configuration for further tuning.

On CH firmware the motion indicator and CNH share one DCI struct (0xBFAC): this disarms
CNH (`cnhSetup()` null, no `Vl53l8Frame#cnhRaw()`).

#### Vl53l8cxDevice.configureMotionIndicator

```java
public Vl53l8Uld.MotionConfig configureMotionIndicator()
```

#### Vl53l8cxDevice.armedCnh

```java
protected Vl53l8CnhSetup armedCnh()
```

The CNH setup the CH classes armed, or null.

#### Vl53l8cxDevice.armCnh

```java
protected void armCnh(Vl53l8CnhSetup setup)
```

#### Vl53l8cxDevice.forgetSensor

```java
protected void forgetSensor()
```

#### Vl53l8cxDevice.startRanging

```java
public void startRanging()
```

Program the output list, start the sensor and the board's stream.

#### Vl53l8cxDevice.stopRanging

```java
public void stopRanging()
```

#### Vl53l8cxDevice.onFrame

```java
public AutoCloseable onFrame(Consumer<Vl53l8Frame> callback)
```

Call `callback` for every parsed frame (reader thread).

#### Vl53l8cxDevice.frames

```java
public Subscription<Vl53l8Frame> frames(int capacity)
```

A pull queue of parsed frames (bounded, drop-oldest).

#### Vl53l8cxDevice.frames

```java
public Subscription<Vl53l8Frame> frames()
```

#### Vl53l8cxDevice.getFrame

```java
public Vl53l8Frame getFrame(Duration timeout)
```

The next frame (contract 07 §3).

#### Vl53l8cxDevice.frameParseErrors

```java
public long frameParseErrors()
```

Frames dropped because they did not parse (corrupt frame, bad size) — not reassembly gaps.

#### Vl53l8cxDevice.reassemblerDiscards

```java
public int reassemblerDiscards()
```

Chunked frames the reassembler discarded (gaps, offset errors).

#### Vl53l8cxDevice.streamDroppedCounts

```java
public List<Long> streamDroppedCounts()
```

#### Vl53l8cxDevice.requireNotRanging

```java
protected void requireNotRanging()
```

### Vl53l8

```java
public final class Vl53l8
```

VL53L8 register-bridge command codecs (contracts/04_SENSOR_VL53L8.md): the SPI bridge of the
VL53L8CX / CH boards, which the L5 / L7 I2C boards speak too (contract 11). RPT_VL53_FRAME
chunks are reassembled by `sensors.vl53l8.FrameReassembler`.

#### Vl53l8.CHUNK_SIZE *(field)*

```java
public static final int CHUNK_SIZE = 2048
```

Register bytes one READ_REG / WRITE_REG moves at most (the bridge's limit).

#### Vl53l8.STREAM_CHUNK_MAX *(field)*

```java
public static final int STREAM_CHUNK_MAX = 1528
```

Bytes of frame data per RPT_VL53_FRAME chunk.

#### Vl53l8.STREAM_TOTAL_MAX *(field)*

```java
public static final int STREAM_TOTAL_MAX = 8192
```

Max frame_size accepted by START_STREAM.

#### Vl53l8.packReadReg

```java
public static byte[] packReadReg(int addr, int length)
```

VL53_READ_REG payload: addr u16 LE, length u16 LE.

#### Vl53l8.packWriteReg

```java
public static byte[] packWriteReg(int addr, byte[] data, int offset, int length)
```

VL53_WRITE_REG payload: addr u16 LE, then at least one data byte (contract 04 §2).

#### Vl53l8.packWriteReg

```java
public static byte[] packWriteReg(int addr, byte[] data)
```

#### Vl53l8.packStartStream

```java
public static byte[] packStartStream(int frameSize)
```

VL53_START_STREAM payload: the frame size the board reads per INT, u16 LE.

### Vl53l8.Vl53l8Cmd

```java
public enum Vl53l8Cmd
```

Host→device opcodes. 0x34 is intentionally unused (a gap in the firmware's sequence).

#### Vl53l8.Vl53l8Cmd — constants

`READ_REG`(0x32), `WRITE_REG`(0x33), `START_STREAM`(0x35), `STOP_STREAM`(0x36)

#### Vl53l8.Vl53l8Cmd.value *(field)*

```java
public final int value
```

### Vl53l8.Vl53l8Rpt

```java
public enum Vl53l8Rpt
```

Device→host report ids.

#### Vl53l8.Vl53l8Rpt — constants

`REG_DATA`(0x91), `VL53_FRAME`(0x93)

#### Vl53l8.Vl53l8Rpt.value *(field)*

```java
public final int value
```

### Vl53l8.RegData

```java
public record RegData(int cmd, long timestampUs, byte[] data)
```

RPT_VL53_REG_DATA: the echoed READ_REG opcode, the MCU timestamp and the register bytes.

#### Vl53l8.RegData.unpack

```java
public static RegData unpack(byte[] p)
```

### FrameReassembler

```java
public final class FrameReassembler
```

Rebuilds full VL53L8 sensor frames from chunked RPT_VL53_FRAME reports
(contracts/04_SENSOR_VL53L8.md). Shared by both VL53L8CX and VL53L8CH — the
chunk transport is identical across the two parts.

Rules: reset on `offset == 0`; chunks must be contiguous — a gap
discards the frame in progress; a frame completes when the accumulated bytes
equal `fullSize`. A chunk out of the rule-4 bounds (more than
`chunkMax` bytes, `fullSize` over `STREAM_TOTAL_MAX`, or
running past `fullSize`) is damaged: dropped and counted as a lost
frame like the other damaged input, never thrown. Mirror of the TS/Python
`FrameReassembler`.

#### FrameReassembler.STREAM_CHUNK_MAX *(field)*

```java
public static final int STREAM_CHUNK_MAX = 1528
```

Bytes of frame data per RPT_VL53_FRAME chunk.

#### FrameReassembler.STREAM_TOTAL_MAX *(field)*

```java
public static final int STREAM_TOTAL_MAX = 8192
```

Largest frame_size START_STREAM accepts (contract 04 §4 rule 4).

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

### Vl53l8CnhSetup

```java
public final class Vl53l8CnhSetup
```

The CNH (compact network histogram) configuration of a VL53L8CH / VL53L7CH: a mirror of
`VL53LMZ_Motion_Configuration` with the plugin helpers that fill it
(`vl53lmz_plugin_cnh.c`, ULD 2.0.16), ported from the reference Python `CnhConfig`.
Build it with `initConfig` and `createAggMap`, check `requiredMemory()`,
then arm it with the device's `configureCnh`. The flags are fixed to ping-pong and variance
off (`Example_12_cnh_data.c`), which `Vl53l8Uld#decodeCnh` assumes. Arguments outside
their fields throw `IllegalArgumentException`, never wrap.

#### Vl53l8CnhSetup.MI_CFG_DEV_IDX *(field)*

```java
public static final int MI_CFG_DEV_IDX = 0xBFAC
```

#### Vl53l8CnhSetup.CNH_MAX_DATA_BYTES *(field)*

```java
public static final int CNH_MAX_DATA_BYTES
```

#### Vl53l8CnhSetup.PULSE_WIDTH_BIN *(field)*

```java
public static final int PULSE_WIDTH_BIN = 10
```

#### Vl53l8CnhSetup.BIN_WIDTH_MM *(field)*

```java
public static final double BIN_WIDTH_MM
```

#### Vl53l8CnhSetup.refBinOffset

```java
public int refBinOffset()
```

#### Vl53l8CnhSetup.sumSpan

```java
public int sumSpan()
```

#### Vl53l8CnhSetup.featureLength

```java
public int featureLength()
```

#### Vl53l8CnhSetup.nbOfAggregates

```java
public int nbOfAggregates()
```

#### Vl53l8CnhSetup.cnhCfg

```java
public int cnhCfg()
```

#### Vl53l8CnhSetup.cnhFlexShift

```java
public int cnhFlexShift()
```

#### Vl53l8CnhSetup.mapId

```java
public int[] mapId()
```

Device zone → CNH aggregate, −1 for a zone in none (64 entries).

#### Vl53l8CnhSetup.initConfig

```java
public void initConfig(int startBin, int numBins, int subSample)
```

vl53lmz_cnh_init_config. `startBin`: first device histogram bin; `numBins`: CNH
bins; `subSample`: device bins summed per CNH bin.

#### Vl53l8CnhSetup.createAggMap

```java
public void createAggMap(int resolution, int startX, int startY, int mergeX, int mergeY, int cols, int rows)
```

vl53lmz_cnh_create_agg_map: aggregates of `mergeX`×`mergeY` zones, `cols`×
`rows` of them from (`startX`, `startY`). `resolution` (16 or 64) must
be the sensor's.

#### Vl53l8CnhSetup.requiredMemory

```java
public int requiredMemory()
```

vl53lmz_cnh_calc_required_memory: the on-device CNH buffer, bytes.

#### Vl53l8CnhSetup.minMaxDistanceMm

```java
public int[] minMaxDistanceMm()
```

{min, max} target distance, mm, the histogram captures whole.

#### Vl53l8CnhSetup.binCenterMm

```java
public double binCenterMm(int binIdx)
```

Distance, mm, at the centre of CNH bin `binIdx`.

#### Vl53l8CnhSetup.pack

```java
public byte[] pack()
```

The 156-byte VL53LMZ_Motion_Configuration struct for the device (DCI 0xBFAC).

### Vl53l8Driver

```java
public final class Vl53l8Driver
```

The ST VL53L8CX ULD 2.1.0 / VL53LMZ ULD 2.0.16 / VL53L5CX-L7CX ULD 2.0.1 on the host, ported from
the reference Python driver: init (sensor firmware download), resolution, frequency, mode,
integration time, sharpener, target order, ranging, frame parsing, power modes, xtalk margin and
calibration, detection thresholds, motion indicator. Register sequences are a 1:1 port — every
request matches the Python one, so captures replay in both.

#### Vl53l8Driver.RESOLUTION_4X4 *(field)*

```java
public static final int RESOLUTION_4X4 = 16
```

#### Vl53l8Driver.RESOLUTION_8X8 *(field)*

```java
public static final int RESOLUTION_8X8 = 64
```

#### Vl53l8Driver.RANGING_MODE_CONTINUOUS *(field)*

```java
public static final int RANGING_MODE_CONTINUOUS = 1
```

#### Vl53l8Driver.RANGING_MODE_AUTONOMOUS *(field)*

```java
public static final int RANGING_MODE_AUTONOMOUS = 3
```

#### Vl53l8Driver.TARGET_ORDER_CLOSEST *(field)*

```java
public static final int TARGET_ORDER_CLOSEST = 1
```

#### Vl53l8Driver.TARGET_ORDER_STRONGEST *(field)*

```java
public static final int TARGET_ORDER_STRONGEST = 2
```

#### Vl53l8Driver.POWER_MODE_SLEEP *(field)*

```java
public static final int POWER_MODE_SLEEP = 0
```

#### Vl53l8Driver.POWER_MODE_WAKEUP *(field)*

```java
public static final int POWER_MODE_WAKEUP = 1
```

#### Vl53l8Driver.POWER_MODE_DEEP_SLEEP *(field)*

```java
public static final int POWER_MODE_DEEP_SLEEP = 2
```

#### Vl53l8Driver.MODULE_TYPE_MZ *(field)*

```java
public static final int MODULE_TYPE_MZ = 0
```

#### Vl53l8Driver.MODULE_TYPE_MZEVO *(field)*

```java
public static final int MODULE_TYPE_MZEVO = 1
```

#### Vl53l8Driver.MODULE_TYPE_MZPLUS *(field)*

```java
public static final int MODULE_TYPE_MZPLUS = 2
```

#### Vl53l8Driver.NVM_DATA_SIZE *(field)*

```java
public static final int NVM_DATA_SIZE = 492
```

#### Vl53l8Driver.OFFSET_BUFFER_SIZE *(field)*

```java
public static final int OFFSET_BUFFER_SIZE = 488
```

#### Vl53l8Driver.XTALK_BUFFER_SIZE *(field)*

```java
public static final int XTALK_BUFFER_SIZE = 776
```

#### Vl53l8Driver.firmware

```java
public Vl53l8Firmware firmware()
```

#### Vl53l8Driver.isI2c

```java
public boolean isI2c()
```

The L5/L7 boards: the sensor sits on I2C (other boot sequence, per-target blocks keep 64 entries in 4×4).

#### Vl53l8Driver.moduleType

```java
public Integer moduleType()
```

L5/L7: MZ (VL53L5CX) or MZEVO (VL53L7CX/CH), read during `init`; else null.

#### Vl53l8Driver.dataReadSize

```java
public int dataReadSize()
```

The frame size the firmware streams (bytes), set by `startRanging`.

#### Vl53l8Driver.frameSizeMismatch

```java
public int[] frameSizeMismatch()
```

{firmware, host} frame sizes when the firmware reported another size than the ULD formula; else null.

#### Vl53l8Driver.resolution

```java
public Integer resolution()
```

Zones the last `startRanging` used, or null.

#### Vl53l8Driver.xtalkCalibrationFailed

```java
public boolean xtalkCalibrationFailed()
```

The last `calibrateXtalk` ran into "coverglass too good": the default xtalk buffer is kept.

#### Vl53l8Driver.xtalkData

```java
public byte[] xtalkData()
```

The 776-byte xtalk buffer the driver uploads (default, calibrated or restored).

#### Vl53l8Driver.siliconIdMatches

```java
public static boolean siliconIdMatches(int deviceId, int revisionId, boolean i2c)
```

Whether the silicon ids are the sensor's: (0xF0, 0x0C) on L8 over SPI; revision 0x02, or 0x01
with device id 0xF0, on L5/L7 over I2C.

#### Vl53l8Driver.dciReadData

```java
public byte[] dciReadData(int index, int dataSize)
```

#### Vl53l8Driver.dciWriteData

```java
public void dciWriteData(int index, byte[] data)
```

#### Vl53l8Driver.dciReplaceData

```java
public void dciReplaceData(int index, int dataSize, byte[] newData, int newDataPos)
```

#### Vl53l8Driver.isAlive

```java
public int[] isAlive()
```

{device id, revision id} of the silicon; `siliconIdMatches` tells whether they are the sensor's.

#### Vl53l8Driver.init

```java
public void init(Consumer<String> progress)
```

vl53l8cx_init: boot the sensor MCU, download the 84 KB sensor firmware, upload the NVM
offsets, the default xtalk and the default configuration. `progress` (may be null)
gets phase texts.

#### Vl53l8Driver.init

```java
public void init()
```

#### Vl53l8Driver.getResolution

```java
public int getResolution()
```

#### Vl53l8Driver.setResolution

```java
public void setResolution(int res)
```

#### Vl53l8Driver.getRangingFrequencyHz

```java
public int getRangingFrequencyHz()
```

#### Vl53l8Driver.setRangingFrequencyHz

```java
public void setRangingFrequencyHz(int hz)
```

#### Vl53l8Driver.setRangingMode

```java
public void setRangingMode(int mode)
```

#### Vl53l8Driver.getRangingMode

```java
public int getRangingMode()
```

#### Vl53l8Driver.getIntegrationTimeMs

```java
public int getIntegrationTimeMs()
```

#### Vl53l8Driver.setIntegrationTimeMs

```java
public void setIntegrationTimeMs(int timeMs)
```

2..1000 ms; no effect in continuous mode.

#### Vl53l8Driver.getSharpenerPercent

```java
public int getSharpenerPercent()
```

0..99 %, rounded to nearest on the way back (ST truncates and loses a count for 95 of the 100
legal values; the stored byte is the same).

#### Vl53l8Driver.setSharpenerPercent

```java
public void setSharpenerPercent(int pct)
```

#### Vl53l8Driver.getTargetOrder

```java
public int getTargetOrder()
```

#### Vl53l8Driver.setTargetOrder

```java
public void setTargetOrder(int order)
```

#### Vl53l8Driver.startRanging

```java
public void startRanging(Integer cnhDataSize)
```

Program the output list and start. `cnhDataSize` (bytes, from
`Vl53l8CnhSetup#requiredMemory()`; null for none) appends the CNH block to every frame.

#### Vl53l8Driver.startRanging

```java
public void startRanging()
```

#### Vl53l8Driver.stopRanging

```java
public void stopRanging()
```

#### Vl53l8Driver.checkDataReady

```java
public boolean checkDataReady()
```

Poll mode: a new frame is ready.

#### Vl53l8Driver.getRangingData

```java
public Vl53l8Frame getRangingData(long timestampUs)
```

Poll mode: read one frame from register 0x00 and parse it.

#### Vl53l8Driver.parseFrame

```java
public Vl53l8Frame parseFrame(long timestampUs, byte[] raw)
```

Parse one raw frame (`dataReadSize()` bytes) — polled, or pushed by the board's stream.
L5/L7 frames are trimmed to the zones in use.

#### Vl53l8Driver.getPowerMode

```java
public int getPowerMode()
```

#### Vl53l8Driver.setPowerMode

```java
public void setPowerMode(int powerMode)
```

Wakeup, sleep or deep sleep (not on L5CX/L7CX). Waking from deep sleep re-runs `init`:
the sensor firmware is lost.

#### Vl53l8Driver.getXtalkMargin

```java
public int getXtalkMargin()
```

Xtalk margin, whole kcps/SPAD (raw / 2048 rounded down, as ST).

#### Vl53l8Driver.setXtalkMargin

```java
public void setXtalkMargin(int marginKcps)
```

Xtalk margin, a whole number of kcps/SPAD, 0..10000 (as ST); anything else is INVALID_PARAM, nothing written.

#### Vl53l8Driver.getCaldataXtalk

```java
public byte[] getCaldataXtalk()
```

The live 776-byte xtalk calibration buffer (save / restore); the resolution is restored afterwards.

#### Vl53l8Driver.setCaldataXtalk

```java
public void setCaldataXtalk(byte[] blob)
```

Restore a saved 776-byte xtalk buffer; it is uploaded again by the next resolution change and at once here.

#### Vl53l8Driver.getDetectionThresholdsEnable

```java
public int getDetectionThresholdsEnable()
```

#### Vl53l8Driver.setDetectionThresholdsEnable

```java
public void setDetectionThresholdsEnable(boolean enabled)
```

#### Vl53l8Driver.getDetectionThresholds

```java
public List<Vl53l8Uld.DetectionThreshold> getDetectionThresholds()
```

The 64 thresholds, low / high rescaled to the units of their measurement.

#### Vl53l8Driver.setDetectionThresholds

```java
public void setDetectionThresholds(List<Vl53l8Uld.DetectionThreshold> thresholds)
```

Program the 64 thresholds (missing entries are zeros; more than 64 is refused).

#### Vl53l8Driver.setDetectionThresholdsAutoStop

```java
public void setDetectionThresholdsAutoStop(boolean autoStop)
```

#### Vl53l8Driver.motionIndicatorInit

```java
public Vl53l8Uld.MotionConfig motionIndicatorInit(int res)
```

A default motion-indicator configuration for `res`, written to the sensor; frames carry motion output from here on.

#### Vl53l8Driver.motionIndicatorDisarm

```java
public void motionIndicatorDisarm()
```

Stop surfacing the motion block: frames parse with `motion` null. Called when the
VL53LMZ CNH setup overwrites the shared DCI 0xBFAC struct (contract 04 §7, ERRATA B4).

#### Vl53l8Driver.motionIndicatorSetResolution

```java
public void motionIndicatorSetResolution(Vl53l8Uld.MotionConfig cfg, int res)
```

#### Vl53l8Driver.motionIndicatorSetDistanceMotion

```java
public void motionIndicatorSetDistanceMotion(Vl53l8Uld.MotionConfig cfg, int distanceMinMm, int distanceMaxMm)
```

#### Vl53l8Driver.calibrateXtalk

```java
public void calibrateXtalk(int reflectancePercent, int nbSamples, int distanceMm)
```

vl53l8cx_calibrate_xtalk: on-device crosstalk calibration against a flat target at
`distanceMm` (600..3000) of `reflectancePercent` (1..99), averaging
`nbSamples` (1..16). Takes seconds; the configuration is saved and restored around it.
Without cover glass the firmware answers "coverglass too good": `xtalkCalibrationFailed()`,
and the default buffer stays.

### Vl53l8Firmware

```java
public enum Vl53l8Firmware
```

The sensor firmware a `Vl53l8Driver` downloads.

#### Vl53l8Firmware — constants

`CX`, `CH`, `L7CX`, `L7CH`

#### Vl53l8Firmware.setName *(field)*

```java
public final String setName
```

The blob set name under `vl53l8/data`.

### Vl53l8Frame

```java
public record Vl53l8Frame(long timestampUs, int resolution, int siliconTempDegc, int[] distanceMm, int[] targetStatus, int[] nbTargetDetected, double[] signalPerSpad, double[] ambientPerSpad, long[] nbSpadsEnabled, double[] rangeSigmaMm, double[] reflectance, byte[] cnhRaw, Vl53l8Motion motion)
```

One decoded ranging frame. Per-zone arrays are sized to the active resolution (16 for 4×4, 64
for 8×8); zone index runs row-major. Values are in the real units ST's GetRangingData gives
(contract 04 §5); ST divides integers, here the scaled values keep their fraction:
`distanceMm` mm (raw ÷4 floored, a negative distance clamped to 0, as ST),
`signalPerSpad` / `ambientPerSpad` kcps/SPAD (raw ÷2048), `rangeSigmaMm` mm
(raw ÷128), `reflectance` % (raw ÷2). `targetStatus` 5 or 9 = valid, 255 = no
target. `cnhRaw` is the raw CNH data block (VL53L8CH / VL53L7CH with CNH armed), else
null; `motion` the motion-indicator output when it is configured, else null.

#### Vl53l8Frame.withResolution

```java
public Vl53l8Frame withResolution(int zones)
```

This frame with `resolution` replaced (the device's own count of zones in use).

### Vl53l8Motion

```java
public record Vl53l8Motion(long globalIndicator1, long globalIndicator2, int status, int nbOfDetectedAggregates, int nbOfAggregates, long[] motion)
```

The motion-indicator block of a frame (`VL53L8CX_ResultsData MotionIndicator`).

### Vl53l8Platform

```java
public interface Vl53l8Platform
```

What the ULD needs from the board: register access through the bridge (16-bit addresses,
auto-increment) and a delay. `Vl53l8cxDevice` maps it onto the firmware bridge; a test can
script it.

### Vl53l8Tables

```java
public final class Vl53l8Tables
```

The ST command tables the VL53L8 / L5 / L7 ULD writes verbatim. Each accessor returns a copy.

#### Vl53l8Tables.getXtalkCmd

```java
public static byte[] getXtalkCmd()
```

vl53l8cx_plugin_xtalk.h: the command that reads the xtalk buffer back; 72 bytes.

#### Vl53l8Tables.calibrateXtalkCx

```java
public static byte[] calibrateXtalkCx()
```

VL53L8CX_CALIBRATE_XTALK (ULD 2.1.0); 1000 bytes.

#### Vl53l8Tables.calibrateXtalkLmz

```java
public static byte[] calibrateXtalkLmz()
```

VL53LMZ_CALIBRATE_XTALK (ULD 2.0.16: L8CH, L7CH); 984 bytes.

#### Vl53l8Tables.calibrateXtalkL7cx

```java
public static byte[] calibrateXtalkL7cx()
```

VL53L7CX_CALIBRATE_XTALK (ULD 2.0.1: L5CX, L7CX); 984 bytes.

#### Vl53l8Tables.xtalkCalOutput

```java
public static int[] xtalkCalOutput()
```

the 17-block output list of the xtalk calibration run (u32 block headers as Java ints).

#### Vl53l8Tables.xtalkCalOutputEnable

```java
public static int[] xtalkCalOutputEnable()
```

the output enables of xtalkCalOutput (u32 block headers as Java ints).

### Vl53l8Uld

```java
public final class Vl53l8Uld
```

VL53L8CX/CH ULD — the frame decode and DCI codecs under the driver.

The DEPZ ToF is two sensors: VL53L8CX (the base multizone
part, also the dev/unprogrammed default) and VL53L8CH (CX plus CNH
compact histograms and its own production USB PID `0xED40`). Their
results-frame wire layout is identical, so this decoder serves both:
`parseFrame` is shared, and the advanced-feature DCI codecs (motion
configuration, detection thresholds, xtalk margin) apply equally to CX and CH.
The only variant difference in the decode path is the footer-id offset —
see `Variant`. Semantics mirror the TS/Python references 1:1.

These are the pure parts of the ST ULD (`vl53l8cx_api.c`, ULD 2.1.0)
that golden vectors pin; the live driver over the register bridge (firmware
download, DCI access, ranging, power modes, calibration) is `Vl53l8Driver`.

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

#### Vl53l8Uld.XTALK_MARGIN_MAX_KCPS *(field)*

```java
public static final int XTALK_MARGIN_MAX_KCPS = 10000
```

Largest xtalk margin `setXtalkMargin` accepts, whole kcps/SPAD (as ST).

#### Vl53l8Uld.xtalkMarginToRaw

```java
public static long xtalkMarginToRaw(int marginKcps)
```

Xtalk margin, a whole number of kcps/SPAD (as ST) -> raw DCI value
(kcps * 2048). The margin is 0..`XTALK_MARGIN_MAX_KCPS`; a negative
or a bigger value throws `Vl53l8Error` (INVALID_PARAM) — never
wrapped (contract 07 §3).

#### Vl53l8Uld.xtalkMarginFromRaw

```java
public static int xtalkMarginFromRaw(long raw)
```

Inverse: raw DCI value -> whole kcps/SPAD (raw / 2048, integer division as ST).

#### Vl53l8Uld.packDetectionThresholds

```java
public static PackedThresholds packDetectionThresholds(List<DetectionThreshold> thresholds)
```

Pack up to 64 detection thresholds into the DCI_DET_THRESH_START payload
plus the 8-byte valid-status block. Each threshold's low/high are scaled
by its measurement selector. Mirror of set_detection_thresholds.
Anything out of its field's range (measurement/type/zone/operation u8,
scaled low/high i32) or more than 64 entries throws
`IllegalArgumentException`, never wrapped or dropped.

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

Parsed results frame, in the real units ST's GetRangingData gives (contract
04 §5). ST divides integers; the scaled values here keep their fraction.

#### Vl53l8Uld.Results.distanceMm *(field)*

```java
public int[] distanceMm
```

mm: raw / 4 floored, a negative distance clamped to 0 (as ST).

#### Vl53l8Uld.Results.targetStatus *(field)*

```java
public int[] targetStatus
```

5 or 9 = valid measurement; 255 = no target detected.

#### Vl53l8Uld.Results.nbTargetDetected *(field)*

```java
public int[] nbTargetDetected
```

Targets found per zone.

#### Vl53l8Uld.Results.signalPerSpad *(field)*

```java
public double[] signalPerSpad
```

Signal rate, kcps/SPAD (raw / 2048).

#### Vl53l8Uld.Results.ambientPerSpad *(field)*

```java
public double[] ambientPerSpad
```

Ambient rate, kcps/SPAD (raw / 2048), one per zone.

#### Vl53l8Uld.Results.nbSpadsEnabled *(field)*

```java
public long[] nbSpadsEnabled
```

Active SPAD count per zone.

#### Vl53l8Uld.Results.rangeSigmaMm *(field)*

```java
public double[] rangeSigmaMm
```

Measurement noise, mm (raw / 128).

#### Vl53l8Uld.Results.reflectance *(field)*

```java
public double[] reflectance
```

Estimated target reflectance, % (raw / 2).

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

A VL53L8 / L5 / L7 ULD status (vl53l8cx_api.h codes) with where it happened.

#### Vl53l8Uld.Vl53l8Error.STATUS_TIMEOUT_ERROR *(field)*

```java
public static final int STATUS_TIMEOUT_ERROR = 1
```

#### Vl53l8Uld.Vl53l8Error.STATUS_CORRUPTED_FRAME *(field)*

```java
public static final int STATUS_CORRUPTED_FRAME = 2
```

#### Vl53l8Uld.Vl53l8Error.STATUS_LASER_SAFETY *(field)*

```java
public static final int STATUS_LASER_SAFETY = 3
```

#### Vl53l8Uld.Vl53l8Error.STATUS_FW_CHECKSUM_FAIL *(field)*

```java
public static final int STATUS_FW_CHECKSUM_FAIL = 5
```

#### Vl53l8Uld.Vl53l8Error.MCU_ERROR *(field)*

```java
public static final int MCU_ERROR = 66
```

#### Vl53l8Uld.Vl53l8Error.STATUS_INVALID_PARAM *(field)*

```java
public static final int STATUS_INVALID_PARAM = 127
```

#### Vl53l8Uld.Vl53l8Error.STATUS_ERROR *(field)*

```java
public static final int STATUS_ERROR = 255
```

#### Vl53l8Uld.Vl53l8Error.code *(field)*

```java
public final int code
```

## VL53L5CX / VL53L7CX / VL53L7CH (ToF)

### Vl53l5cxDevice

```java
public class Vl53l5cxDevice extends Vl53l7cxDevice
```

VL53L5CX ToF (8×8 zones, 63° field of view): the board, firmware and API of `Vl53l7cxDevice`; the sensor reports module type MZ.

### Vl53l7chDevice

```java
public class Vl53l7chDevice extends Vl53l7cxDevice
```

VL53L7CH: the VL53L7CX superset with CNH histograms. `init()` downloads the CH firmware
(VL53LMZ ULD 2.0.16, the VL53L8CH one); CNH frames up to ~7.6 KB stream in chunks.

#### Vl53l7chDevice.cnhSetup

```java
public Vl53l8CnhSetup cnhSetup()
```

The CNH setup armed by `configureCnh`, or null.

#### Vl53l7chDevice.configureCnh

```java
public void configureCnh(Vl53l8CnhSetup setup)
```

Arm CNH for the next start (as `Vl53l8chDevice#configureCnh`); disarms the motion indicator.

### Vl53l7cxDevice

```java
public class Vl53l7cxDevice extends Vl53l8cxDevice
```

VL53L7CX multizone ToF (8×8 zones, 90° field of view) on the I2C bridge board (contract 11). The
board speaks the VL53L8 protocol plus three commands, and the sensor runs the same ST ULD family,
so this is a `Vl53l8cxDevice` with the L5/L7 firmware set (ULD 2.0.1), smaller register
reads, ranging from 1 Hz, and the board commands `getBridgeInfo()`,
`setI2cSpeedKhz`, `pinCtrl`. L5CX / L7CX have no deep sleep and no threshold
auto-stop.

#### Vl53l7cxDevice.expectedModuleType

```java
protected int expectedModuleType()
```

The module type this class expects after `init()` (L5 and L7 share a firmware).

#### Vl53l7cxDevice.moduleType

```java
public Integer moduleType()
```

The sensor's module type read at `init()`: `Vl53l8Driver#MODULE_TYPE_MZ` (VL53L5CX)
or `Vl53l8Driver#MODULE_TYPE_MZEVO` (VL53L7CX / CH); null before.

#### Vl53l7cxDevice.xtalkCalibrationFailed

```java
public boolean xtalkCalibrationFailed()
```

The last `calibrateXtalk` found nothing to calibrate ("coverglass too good").

#### Vl53l7cxDevice.getBridgeInfo

```java
public Vl53l7.Vl53l7Info getBridgeInfo()
```

Bridge counters and pin levels (the sensor is not touched). Read it before and after a run:
during one each call takes the bus from the stream and can cost a frame.

#### Vl53l7cxDevice.setI2cSpeedKhz

```java
public int setI2cSpeedKhz(int khz)
```

Sensor bus SCL; the board takes the nearest of 100, 200, 400, 500 … 1000 kHz. Returns the value
in effect. `BusyException` mid-transfer: stop ranging first.

#### Vl53l7cxDevice.pinCtrl

```java
public void pinCtrl(Vl53l7.PinAction action)
```

Drive the sensor's LPn / I2C_RST pins. LPN_OFF and SOFT_CYCLE drop the sensor's state: `init()` again afterwards.

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

VL53_READ_REG payload: `addr u16, len u16` little-endian, with
`len` 1..`READ_MAX_LEN` and `addr+len <= 0x10000`
(contract 11 §2), else IllegalArgumentException.

#### Vl53l7.packWriteReg

```java
public static byte[] packWriteReg(int addr, byte[] data)
```

VL53_WRITE_REG payload: `addr u16, data[1..2048]` with
`addr+N <= 0x10000` (contract 11 §2), else IllegalArgumentException.

#### Vl53l7.packStartStream

```java
public static byte[] packStartStream(int frameSize)
```

VL53_START_STREAM payload: `frame_size u16`, 1..8192 (the VL53L8 command, contract 04 §2).

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

The live driver over the I2C bridge is `Vl53l8Driver` with the L5/L7
firmware sets (`Vl53l7cxDevice`). Power modes, xtalk calibration,
detection thresholds and the motion indicator are the VL53L8 features with the
per-part deltas of contract 11 §4 (L5CX / L7CX: no threshold auto-stop, no
DEEP_SLEEP), checked against the ST sources and verified on hardware (L5CX and
L7CH, 2026-09-24); they run through that live driver.

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

### Vl53l0xDevice

```java
public final class Vl53l0xDevice extends Vl53lxDevice
```

VL53L0X (2 m, 1-byte register addresses, 400 kHz): ULD API 1.0.4 with ranging profiles.

### Vl53l1cbDevice

```java
public final class Vl53l1cbDevice extends Vl53lxDevice
```

VL53L1CB (8 m, cover-glass module): the VL53L1CX die and drivers; usually needs a crosstalk calibration.

### Vl53l1cxDevice

```java
public final class Vl53l1cxDevice extends Vl53lxDevice
```

VL53L1CX (4 m): the VL53L1X ULD (short / long) or the histogram driver.

### Vl53l3cxDevice

```java
public final class Vl53l3cxDevice extends Vl53lxDevice
```

VL53L3CX (3 m): ST's ULP driver (single target) or the histogram driver (several per frame).

### Vl53l4cxDevice

```java
public final class Vl53l4cxDevice extends Vl53lxDevice
```

VL53L4CX (6 m): the histogram driver only — to run it light, borrow a sibling's with `init("uld", "VL53L4CD")`.

### Vl53lxDevice

```java
public class Vl53lxDevice extends DepzDevice
```

A board of the VL53L 1D family on the `APP_VL53L0_4` register bridge (contract 12): VL53L0X,
VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CD, VL53L4CX. Every ULD runs here; two axes pick how a board is
driven — the product (whose parameter set: the board's own, or a sibling's to borrow its driver) and
the driver kind (`uld`, `ulp`, `histogram`). `init` binds the pair,
`configure` re-initialises and applies a ranging configuration (before every run), then
`startRanging()` and read `measurements`. Ask `supports` before an optional group;
a missing one throws `UnsupportedOperationException`. This generic class reads the product
from the board's name; the per-product classes fix it.

#### Vl53lxDevice.fixedProduct

```java
protected String fixedProduct()
```

The product this class drives; null = read it from the device name.

#### Vl53lxDevice.boardName

```java
public synchronized String boardName()
```

The board's device name, e.g. `DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523`.

#### Vl53lxDevice.detected

```java
public String detected()
```

The product the device name carries, or null on an unstamped board.

#### Vl53lxDevice.product

```java
public String product()
```

The product `init` bound (null before).

#### Vl53lxDevice.driverKind

```java
public String driverKind()
```

#### Vl53lxDevice.caveat

```java
public String caveat()
```

#### Vl53lxDevice.driver

```java
public Vl53lxDriver driver()
```

The ULD port in use — escape hatch for product-specific calls.

#### Vl53lxDevice.initialized

```java
public boolean initialized()
```

#### Vl53lxDevice.ranging

```java
public boolean ranging()
```

#### Vl53lxDevice.driverKinds

```java
public List<String> driverKinds(String product)
```

The driver kinds a product has (null: this board's).

#### Vl53lxDevice.driverKinds

```java
public List<String> driverKinds()
```

#### Vl53lxDevice.init

```java
public void init(String driver, String product)
```

Bind the (product, driver kind) pair and initialise the sensor. `product` null: this class's,
else the board name's; `driver` null: the product's first kind. A pair the table lacks throws
`UnsupportedOperationException` naming what the product has.

#### Vl53lxDevice.init

```java
public void init(String driver)
```

#### Vl53lxDevice.init

```java
public void init()
```

#### Vl53lxDevice.identifyProduct

```java
public Map<String, Object> identifyProduct()
```

Everything about what is connected (needs `init`). `model_id_ok` is a cross-check
only: L1CX/L1CB and L4CD/L4CX share their ids.

#### Vl53lxDevice.notes

```java
public List<String> notes()
```

The lines a UI shows about the chosen pair: a product named by hand, a borrowed driver, a pair that
reaches less than the board is rated for, the driver's caveat.

#### Vl53lxDevice.supports

```java
public boolean supports(String group)
```

Whether this product / driver serves an optional group.

#### Vl53lxDevice.modes

```java
public List<String> modes()
```

Named ranging modes, the first being what init leaves; empty if none.

#### Vl53lxDevice.budgetRange

```java
public int[] budgetRange()
```

The {min, max} budget the driver offers, ms.

#### Vl53lxDevice.driverReachMm

```java
public Integer driverReachMm()
```

How far the loaded configuration can measure, mm, or null.

#### Vl53lxDevice.xshut

```java
public void xshut(int action)
```

Drive XSHUT (`Vl53lx#XSHUT_OFF` / `Vl53lx#XSHUT_ON` / `Vl53lx#XSHUT_RESET`). Off and
Reset stop the stream; the sensor then holds none of the configuration — `init` again.

#### Vl53lxDevice.bridgeInfo

```java
public Vl53lx.Vl53lxInfo bridgeInfo()
```

RPT_VL53_INFO — the bridge's own counters and settings; safe while streaming.

#### Vl53lxDevice.configure

```java
public void configure(int budgetMs, int interMs, String mode, Integer offsetMm, Integer xtalkKcps, Integer signalKcps)
```

Re-initialise the sensor and apply a ranging configuration. The re-init is deliberate: it is the
only way to know what the configuration registers hold. `mode` goes on before the budget;
`offsetMm` / `xtalkKcps` re-apply a stored calibration; `signalKcps` replaces the
blob's signal threshold (each null: leave it). `interMs` 0 = continuous. An unsupported group
is refused before the re-init.

#### Vl53lxDevice.configure

```java
public void configure(int budgetMs, int interMs, String mode)
```

#### Vl53lxDevice.configure

```java
public void configure(int budgetMs, int interMs)
```

#### Vl53lxDevice.configure

```java
public void configure()
```

50 ms, continuous.

#### Vl53lxDevice.getRangeTiming

```java
public RangeTiming getRangeTiming()
```

(timing budget ms, inter-measurement ms) read back from the sensor; 0 = continuous.

#### Vl53lxDevice.getMode

```java
public String getMode()
```

The mode in use, or null on a product without modes.

#### Vl53lxDevice.budgetChoices

```java
public List<Integer> budgetChoices()
```

The only budgets accepted right now, or empty when any in `budgetRange()` will do.

#### Vl53lxDevice.snapBudget

```java
public int snapBudget(int budgetMs)
```

The nearest budget this product accepts.

#### Vl53lxDevice.getOffsetMm

```java
public int getOffsetMm()
```

#### Vl53lxDevice.setOffsetMm

```java
public void setOffsetMm(int offsetMm)
```

#### Vl53lxDevice.getXtalkKcps

```java
public int getXtalkKcps()
```

#### Vl53lxDevice.setXtalkKcps

```java
public void setXtalkKcps(int xtalkKcps)
```

#### Vl53lxDevice.calibrateOffset

```java
public int calibrateOffset(int targetDistMm, Integer nbSamples)
```

Offset calibration against a flat target; returns the offset programmed (store it on the host).

#### Vl53lxDevice.calibrateOffset

```java
public int calibrateOffset(int targetDistMm)
```

#### Vl53lxDevice.calibrateXtalk

```java
public int calibrateXtalk(int targetDistMm, Integer nbSamples)
```

Crosstalk calibration; returns the xtalk programmed, kcps (store it on the host).

#### Vl53lxDevice.calibrateXtalk

```java
public int calibrateXtalk(int targetDistMm)
```

#### Vl53lxDevice.getDetectionThresholds

```java
public Vl53lxDriver.Thresholds getDetectionThresholds()
```

#### Vl53lxDevice.setDetectionThresholds

```java
public void setDetectionThresholds(int distanceLowMm, int distanceHighMm, int window)
```

The distance-window interrupt; stays armed until the next init / configure.

#### Vl53lxDevice.getSignalThresholdKcps

```java
public int getSignalThresholdKcps()
```

#### Vl53lxDevice.setSignalThresholdKcps

```java
public void setSignalThresholdKcps(int signalKcps)
```

#### Vl53lxDevice.getSigmaThresholdMm

```java
public int getSigmaThresholdMm()
```

#### Vl53lxDevice.setSigmaThresholdMm

```java
public void setSigmaThresholdMm(int sigmaMm)
```

#### Vl53lxDevice.getRoi

```java
public int[] getRoi()
```

{x, y} SPADs.

#### Vl53lxDevice.setRoi

```java
public void setRoi(int x, int y)
```

#### Vl53lxDevice.getRoiCenter

```java
public int getRoiCenter()
```

#### Vl53lxDevice.setRoiCenter

```java
public void setRoiCenter(int centerSpad)
```

#### Vl53lxDevice.startTemperatureUpdate

```java
public void startTemperatureUpdate()
```

Re-run VHV after an ambient change over 8 °C.

#### Vl53lxDevice.performRefSpadManagement

```java
public Object performRefSpadManagement()
```

VL53L0X: re-measure the reference SPADs.

#### Vl53lxDevice.startRanging

```java
public void startRanging()
```

Start the sensor and arm the board's stream: one report per INT edge, followed on the MCU by the
driver's interrupt-release writes.

#### Vl53lxDevice.stopRanging

```java
public void stopRanging()
```

#### Vl53lxDevice.measureOnce

```java
public Vl53lxMeasurement measureOnce(Duration timeout)
```

One polled measurement: start, data ready, read, release the interrupt, stop. Not while streaming.

#### Vl53lxDevice.measureOnce

```java
public Vl53lxMeasurement measureOnce()
```

#### Vl53lxDevice.onMeasurement

```java
public AutoCloseable onMeasurement(Consumer<Vl53lxMeasurement> callback)
```

Call `callback` for every streamed measurement (reader thread).

#### Vl53lxDevice.measurements

```java
public Subscription<Vl53lxMeasurement> measurements(int capacity)
```

A pull queue of streamed measurements (bounded, drop-oldest).

#### Vl53lxDevice.measurements

```java
public Subscription<Vl53lxMeasurement> measurements()
```

#### Vl53lxDevice.getMeasurement

```java
public Vl53lxMeasurement getMeasurement(Duration timeout)
```

The next streamed measurement (contract 07 §3).

#### Vl53lxDevice.streamParseErrors

```java
public long streamParseErrors()
```

Stream reports dropped because the driver did not decode them.

#### Vl53lxDevice.streamDroppedCounts

```java
public List<Long> streamDroppedCounts()
```

### Vl53lxMeasurement

```java
public record Vl53lxMeasurement(long timestampUs, int distanceMm, int status, String statusText, double signalKcps, double ambientKcps, double sigmaMm, double spads, List<Vl53lxTarget> targets, Map<String, Object> extra, Object bins)
```

One ranging result of the 1D family, the same shape for every product. On the histogram driver
`targets` holds every return, strongest first, the top level repeats the first and `bins`
carries the raw 24-bin histogram; the light drivers leave both empty.

#### Vl53lxMeasurement.PLOTTABLE_STATUSES *(field)*

```java
public static final List<Integer> PLOTTABLE_STATUSES
```

Statuses that mean "this distance is real": a histogram product reports 6 on the first frame of a
stream and 11 for two merged targets. Prefer `plottable()` over `valid()` there.

#### Vl53lxMeasurement.valid

```java
public boolean valid()
```

#### Vl53lxMeasurement.plottable

```java
public boolean plottable()
```

#### Vl53lxMeasurement.plotDistances

```java
public List<Integer> plotDistances()
```

The distances a chart draws: the plottable targets in driver order, or the single distance.

#### Vl53lxMeasurement.primaryTarget

```java
public Vl53lxTarget primaryTarget()
```

The one target a single-number readout shows, or null.

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

### Vl53l0xUldDriver

```java
public final class Vl53l0xUldDriver extends Vl53lxDriver
```

The VL53L0X API 1.0.4 ranging path (vl53l0x_api.c / _api_core.c / _api_calibration.c), ported from
the reference Python driver: DataInit, StaticInit (reference SPADs from NVM, tuning settings), the
timing-budget arithmetic, start / stop, GetRangingMeasurementData with its PAL range status (sigma
estimate included), the reference calibration, PerformRefSpadManagement, offset and crosstalk
calibration and the four ST ranging profiles. Register sequences, integer widths and the deliberate
uint32 overflows are the C driver's. There is no ROI on this die.

#### Vl53l0xUldDriver.MODEL_ID_VL53L0X *(field)*

```java
public static final int MODEL_ID_VL53L0X = 0xEE
```

#### Vl53l0xUldDriver.refSpadsFromNvm

```java
public boolean refSpadsFromNvm()
```

False when the NVM reference-SPAD record was out of range and was measured instead.

#### Vl53l0xUldDriver.moduleId

```java
public int moduleId()
```

#### Vl53l0xUldDriver.revision

```java
public int revision()
```

#### Vl53l0xUldDriver.productIdString

```java
public String productIdString()
```

#### Vl53l0xUldDriver.partUidUpper

```java
public long partUidUpper()
```

#### Vl53l0xUldDriver.partUidLower

```java
public long partUidLower()
```

#### Vl53l0xUldDriver.signalRateMeasFixed400mm

```java
public long signalRateMeasFixed400mm()
```

#### Vl53l0xUldDriver.part2PartOffsetAdjustmentNvmUm

```java
public long part2PartOffsetAdjustmentNvmUm()
```

#### Vl53l0xUldDriver.targetRefRate

```java
public int targetRefRate()
```

The reference-SPAD target rate (9.7 Mcps) a tuning buffer's {0xFF, 3, msb, lsb} record sets;
the SPAD management converges on it.

#### Vl53l0xUldDriver.sigmaEstRefArray

```java
public int sigmaEstRefArray()
```

The three sigma-estimate constants a tuning buffer's {0xFF, 0..2, msb, lsb} records set
(stored only: the 1.0.4 sigma estimate has them as constants).

#### Vl53l0xUldDriver.sigmaEstEffPulseWidth

```java
public int sigmaEstEffPulseWidth()
```

#### Vl53l0xUldDriver.sigmaEstEffAmbWidth

```java
public int sigmaEstEffAmbWidth()
```

#### Vl53l0xUldDriver.oscFrequencyMHz

```java
public long oscFrequencyMHz()
```

#### Vl53l0xUldDriver.isqrt

```java
public static long isqrt(long num)
```

VL53L0X_isqrt(): the bit-by-bit integer square root.

#### Vl53l0xUldDriver.waitBoot

```java
public void waitBoot()
```

No firmware-status register: the model id answering says the die is up (1 s).

#### Vl53l0xUldDriver.waitBoot

```java
public void waitBoot(long timeoutMs)
```

No firmware-status register: the model id answering says the die is up.

#### Vl53l0xUldDriver.resetDevice

```java
public void resetDevice()
```

VL53L0X_ResetDevice() (1 s): a sensor left configured by a previous session ranges nothing without it.

#### Vl53l0xUldDriver.resetDevice

```java
public void resetDevice(long timeoutMs)
```

VL53L0X_ResetDevice(): a sensor left configured by a previous session ranges nothing without it.

#### Vl53l0xUldDriver.performRefCalibration

```java
public RefCalibration performRefCalibration()
```

VL53L0X_PerformRefCalibration(): VHV, then phase → (vhv, phase). Not optional in practice.

#### Vl53l0xUldDriver.getInfoFromDevice

```java
public void getInfoFromDevice(int option)
```

VL53L0X_get_info_from_device(): the reference SPAD record (option 1), the product
identification (2), the part UID and factory offset (4).

#### Vl53l0xUldDriver.loadTuningSettings

```java
public void loadTuningSettings(int[] buffer)
```

VL53L0X_load_tuning_settings(): {count, address, bytes...} records, with count 0xFF carrying a
host-side parameter instead; a zero count ends the buffer.

#### Vl53l0xUldDriver.setOffsetUm

```java
public void setOffsetUm(long offsetUm)
```

The register is 10.2 format in mm: steps of 250 µm.

#### Vl53l0xUldDriver.getOffsetUm

```java
public long getOffsetUm()
```

#### Vl53l0xUldDriver.performOffsetCalibration

```java
public long performOffsetCalibration(int calDistanceMm)
```

VL53L0X_perform_offset_calibration() with 50 samples → µm.

#### Vl53l0xUldDriver.performOffsetCalibration

```java
public long performOffsetCalibration(int calDistanceMm, int nbSamples)
```

VL53L0X_perform_offset_calibration() → µm. TCC and the range-ignore threshold are off for the run.

#### Vl53l0xUldDriver.performXtalkCalibration

```java
public long performXtalkCalibration(int calDistanceMm)
```

VL53L0X_perform_xtalk_calibration() with 50 samples → the rate, Mcps 16.16.

#### Vl53l0xUldDriver.performXtalkCalibration

```java
public long performXtalkCalibration(int calDistanceMm, int nbSamples)
```

VL53L0X_perform_xtalk_calibration() → the rate, Mcps 16.16.

#### Vl53l0xUldDriver.getReferenceSpads

```java
public RefSpads getReferenceSpads()
```

### Vl53l0xUldDriver.RefCalibration

```java
public record RefCalibration(int vhv, int phase)
```

The VHV and phase settings the reference calibration found.

### Vl53l0xUldDriver.RefSpads

```java
public record RefSpads(int count, int isAperture)
```

The reference SPAD set: how many and whether they are aperture SPADs (1) or not (0).

### Vl53l1Die

```java
public abstract class Vl53l1Die extends Vl53lxDriver
```

The VL53L1 die (`uld/vl53l1_die.py`): register map, the 17-byte result block every part of
it shares, and the body of the VL53L4CD ULD that `Vl53l4UldDriver` and `Vl53l3UlpDriver`
fill in; `Vl53l1UldDriver` (ST's VL53L1X ULD) overrides where that driver differs. A faithful
port: register sequences and integer widths as in the C drivers.

#### Vl53l1Die.SOFT_RESET *(field)*

```java
public static final int SOFT_RESET = 0x0000
```

#### Vl53l1Die.I2C_SLAVE_DEVICE_ADDRESS *(field)*

```java
public static final int I2C_SLAVE_DEVICE_ADDRESS = 0x0001
```

#### Vl53l1Die.OSC_FREQUENCY *(field)*

```java
public static final int OSC_FREQUENCY = 0x0006
```

#### Vl53l1Die.VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND *(field)*

```java
public static final int VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND = 0x0008
```

#### Vl53l1Die.XTALK_PLANE_OFFSET_KCPS *(field)*

```java
public static final int XTALK_PLANE_OFFSET_KCPS = 0x0016
```

#### Vl53l1Die.XTALK_X_PLANE_GRADIENT_KCPS *(field)*

```java
public static final int XTALK_X_PLANE_GRADIENT_KCPS = 0x0018
```

#### Vl53l1Die.XTALK_Y_PLANE_GRADIENT_KCPS *(field)*

```java
public static final int XTALK_Y_PLANE_GRADIENT_KCPS = 0x001A
```

#### Vl53l1Die.RANGE_OFFSET_MM *(field)*

```java
public static final int RANGE_OFFSET_MM = 0x001E
```

#### Vl53l1Die.INNER_OFFSET_MM *(field)*

```java
public static final int INNER_OFFSET_MM = 0x0020
```

#### Vl53l1Die.OUTER_OFFSET_MM *(field)*

```java
public static final int OUTER_OFFSET_MM = 0x0022
```

#### Vl53l1Die.GPIO_HV_MUX_CTRL *(field)*

```java
public static final int GPIO_HV_MUX_CTRL = 0x0030
```

#### Vl53l1Die.GPIO_TIO_HV_STATUS *(field)*

```java
public static final int GPIO_TIO_HV_STATUS = 0x0031
```

#### Vl53l1Die.SYSTEM_INTERRUPT_CONFIG_GPIO *(field)*

```java
public static final int SYSTEM_INTERRUPT_CONFIG_GPIO = 0x0046
```

#### Vl53l1Die.PHASECAL_CONFIG_TIMEOUT_MACROP *(field)*

```java
public static final int PHASECAL_CONFIG_TIMEOUT_MACROP = 0x004B
```

#### Vl53l1Die.RANGE_CONFIG_TIMEOUT_MACROP_A_HI *(field)*

```java
public static final int RANGE_CONFIG_TIMEOUT_MACROP_A_HI = 0x005E
```

#### Vl53l1Die.RANGE_CONFIG_VCSEL_PERIOD_A *(field)*

```java
public static final int RANGE_CONFIG_VCSEL_PERIOD_A = 0x0060
```

#### Vl53l1Die.RANGE_CONFIG_TIMEOUT_MACROP_B_HI *(field)*

```java
public static final int RANGE_CONFIG_TIMEOUT_MACROP_B_HI = 0x0061
```

#### Vl53l1Die.RANGE_CONFIG_VCSEL_PERIOD_B *(field)*

```java
public static final int RANGE_CONFIG_VCSEL_PERIOD_B = 0x0063
```

#### Vl53l1Die.RANGE_CONFIG_SIGMA_THRESH *(field)*

```java
public static final int RANGE_CONFIG_SIGMA_THRESH = 0x0064
```

#### Vl53l1Die.MIN_COUNT_RATE_RTN_LIMIT_MCPS *(field)*

```java
public static final int MIN_COUNT_RATE_RTN_LIMIT_MCPS = 0x0066
```

#### Vl53l1Die.RANGE_CONFIG_VALID_PHASE_HIGH *(field)*

```java
public static final int RANGE_CONFIG_VALID_PHASE_HIGH = 0x0069
```

#### Vl53l1Die.INTERMEASUREMENT_MS *(field)*

```java
public static final int INTERMEASUREMENT_MS = 0x006C
```

#### Vl53l1Die.THRESH_HIGH *(field)*

```java
public static final int THRESH_HIGH = 0x0072
```

#### Vl53l1Die.THRESH_LOW *(field)*

```java
public static final int THRESH_LOW = 0x0074
```

#### Vl53l1Die.SD_CONFIG_WOI_SD0 *(field)*

```java
public static final int SD_CONFIG_WOI_SD0 = 0x0078
```

#### Vl53l1Die.SD_CONFIG_INITIAL_PHASE_SD0 *(field)*

```java
public static final int SD_CONFIG_INITIAL_PHASE_SD0 = 0x007A
```

#### Vl53l1Die.ROI_CONFIG_USER_ROI_CENTRE_SPAD *(field)*

```java
public static final int ROI_CONFIG_USER_ROI_CENTRE_SPAD = 0x007F
```

#### Vl53l1Die.ROI_CONFIG_USER_ROI_XY_SIZE *(field)*

```java
public static final int ROI_CONFIG_USER_ROI_XY_SIZE = 0x0080
```

#### Vl53l1Die.SYSTEM_INTERRUPT_CLEAR *(field)*

```java
public static final int SYSTEM_INTERRUPT_CLEAR = 0x0086
```

#### Vl53l1Die.SYSTEM_MODE_START *(field)*

```java
public static final int SYSTEM_MODE_START = 0x0087
```

#### Vl53l1Die.RESULT_RANGE_STATUS *(field)*

```java
public static final int RESULT_RANGE_STATUS = 0x0089
```

#### Vl53l1Die.RESULT_OSC_CALIBRATE_VAL *(field)*

```java
public static final int RESULT_OSC_CALIBRATE_VAL = 0x00DE
```

#### Vl53l1Die.FIRMWARE_SYSTEM_STATUS *(field)*

```java
public static final int FIRMWARE_SYSTEM_STATUS = 0x00E5
```

#### Vl53l1Die.IDENTIFICATION_MODEL_ID *(field)*

```java
public static final int IDENTIFICATION_MODEL_ID = 0x010F
```

#### Vl53l1Die.ROI_CONFIG_MODE_ROI_CENTRE_SPAD *(field)*

```java
public static final int ROI_CONFIG_MODE_ROI_CENTRE_SPAD = 0x013E
```

#### Vl53l1Die.WINDOW_BELOW *(field)*

```java
public static final int WINDOW_BELOW = 0
```

#### Vl53l1Die.WINDOW_ABOVE *(field)*

```java
public static final int WINDOW_ABOVE = 1
```

#### Vl53l1Die.WINDOW_OUT *(field)*

```java
public static final int WINDOW_OUT = 2
```

#### Vl53l1Die.WINDOW_IN *(field)*

```java
public static final int WINDOW_IN = 3
```

#### Vl53l1Die.CONFIG_ADDR *(field)*

```java
public static final int CONFIG_ADDR = 0x002D
```

The configuration blob spans 0x2D..0x87 on every part; byte 0 is always the Fast Mode Plus byte.

#### Vl53l1Die.CONFIG_END *(field)*

```java
public static final int CONFIG_END = 0x0087
```

#### Vl53l1Die.CONFIG_FMP_BYTE *(field)*

```java
public static final int CONFIG_FMP_BYTE = 0x12
```

#### Vl53l1Die.RESULT_BLOCK_ADDR *(field)*

```java
public static final int RESULT_BLOCK_ADDR
```

#### Vl53l1Die.RESULT_BLOCK_LEN *(field)*

```java
public static final int RESULT_BLOCK_LEN = 17
```

#### Vl53l1Die.stopMode

```java
protected int stopMode()
```

What stopRanging writes: the L4CD ULD 0x80, the L3CX ULP and the L1X ULD 0x00.

#### Vl53l1Die.signalAt

```java
protected int signalAt()
```

Where the signal rate comes from: 0x008E (offset 5) on the L4CD ULD, 0x0098 (15) on the L1X ULD.

#### Vl53l1Die.perSpadK

```java
protected int perSpadK()
```

Per-SPAD scale: 256 on the L4CD ULD, 25 on the L1X ULD (ST's own difference, kept).

#### Vl53l1Die.phaseWindowMm

```java
public static long phaseWindowMm(long fastOscFrequency, long validPhaseHigh)
```

How far a target can be before the die fails it with status 4, mm.

#### Vl53l1Die.statusText

```java
public static String statusText(int status)
```

#### Vl53l1Die.getResult

```java
public Vl53lxRange getResult()
```

One block read instead of the C driver's six register reads; the decoding is identical.

#### Vl53l1Die.bootState

```java
public int bootState()
```

#### Vl53l1Die.waitBoot

```java
public void waitBoot(long timeoutMs)
```

#### Vl53l1Die.waitBoot

```java
public void waitBoot()
```

#### Vl53l1Die.resetDevice

```java
public void resetDevice(long timeoutMs)
```

The die's soft reset; a part that no longer acknowledges writes is reset through XSHUT.

#### Vl53l1Die.resetDevice

```java
public void resetDevice()
```

#### Vl53l1Die.initBoot

```java
protected void initBoot()
```

How a part gets to a booted die (the L1X ULD resets first).

#### Vl53l1Die.initAfterConfig

```java
protected void initAfterConfig()
```

Writes right after the configuration blob, before VHV.

#### Vl53l1Die.initExtra

```java
protected void initExtra()
```

Writes at the end of init, before the timing is set.

#### Vl53l1Die.getInterruptPolarity

```java
public int getInterruptPolarity()
```

#### Vl53l1Die.setInterruptPolarity

```java
public void setInterruptPolarity(int polarity)
```

#### Vl53l1Die.collect

```java
protected void collect(int nbSamples, BiConsumer<Integer, Vl53lxRange> onSample)
```

#### Vl53l1Die.roundHalfAway

```java
protected static long roundHalfAway(double v)
```

C's round(): half away from zero.

### Vl53l1UldDriver

```java
public final class Vl53l1UldDriver extends Vl53l1Die
```

VL53L1X ULD 3.5.5 (`VL53L1X_api.c` + `VL53L1X_calibration.c`) for VL53L1CX and VL53L1CB: a
tabulated timing budget, short / long distance modes, an init that resets the die first. Crosstalk is
in kcps, the register's own unit (ST's wrapper converts to cps).

#### Vl53l1UldDriver.setDetectionThresholds

```java
public void setDetectionThresholds(int distanceLowMm, int distanceHighMm, int window, boolean intOnNoTarget)
```

### Vl53l3UlpDriver

```java
public final class Vl53l3UlpDriver extends Vl53l1Die
```

VL53L3CX ULP 1.0.0: plain single-target ranging on the histogram part.

### Vl53l4UldDriver

```java
public final class Vl53l4UldDriver extends Vl53l1Die
```

VL53L4CD ULD 2.2.3 (`VL53L4CD_api.c` + `VL53L4CD_calibration.c`) for VL53L4CD and VL53L4CX alike.

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

The live ULD / ULP / Bare-Driver host drivers over the bridge are
`Vl53lxDrivers` (`ai.depz.sensor.devices.Vl53lxDevice` runs them).

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

### Vl53lxDriver

```java
public abstract class Vl53lxDriver
```

The contract every driver of the 1D family implements (`uld/base.py`). The properties say what
the product is as far as the bridge is concerned and which optional groups it serves
(`supports()`): mode, timing, offset, xtalk, calib_offset, calib_xtalk, thresholds,
signal_thresh, sigma_thresh, roi, temp_update, refspad. A group a driver lacks throws
`UnsupportedOperationException`.

#### Vl53lxDriver.p *(field)*

```java
protected final Vl53lxPlatform p
```

#### Vl53lxDriver.product

```java
public String product()
```

The product this driver was bound for (one driver serves several).

#### Vl53lxDriver.addrWidth

```java
public int addrWidth()
```

Bytes of register address on the wire.

#### Vl53lxDriver.clearSteps

```java
public int[][] clearSteps()
```

The interrupt-release writes handed to START_STREAM, `{addr, value}` each.

#### Vl53lxDriver.maxKhz

```java
public int maxKhz()
```

The bus ceiling; the driver runs the bus there itself.

#### Vl53lxDriver.supports

```java
public Collection<String> supports()
```

#### Vl53lxDriver.modes

```java
public List<String> modes()
```

The named ranging modes, the first being what init leaves; empty when there are none.

#### Vl53lxDriver.budgetMs

```java
public int[] budgetMs()
```

The {min, max} budget a UI should offer, ms; see `budgetChoices()`.

#### Vl53lxDriver.histogram

```java
public boolean histogram()
```

The frame is a raw histogram (the histogram driver).

#### Vl53lxDriver.waitDataReady

```java
public void waitDataReady(long timeoutMs)
```

#### Vl53lxDriver.readMeasurement

```java
public Vl53lxRange readMeasurement()
```

One polled result: the stream block read and decoded.

#### Vl53lxDriver.budgetChoices

```java
public List<Integer> budgetChoices()
```

The only budgets accepted right now (ascending), or empty when any integer of `budgetMs()` will do.

#### Vl53lxDriver.setMode

```java
public Integer setMode(String name)
```

Apply one of `modes()`. Returns the timing budget in effect after the change, ms, on a driver whose
mode change can move it (VL53L1: contract 12 §5), else null.

#### Vl53lxDriver.getMode

```java
public String getMode()
```

#### Vl53lxDriver.reachMm

```java
public Integer reachMm()
```

How far the loaded configuration can measure, mm, or null when not characterised.

#### Vl53lxDriver.driverInfo

```java
public Map<String, Object> driverInfo()
```

Numbers this driver shows that the contract has no field for.

#### Vl53lxDriver.getOffset

```java
public int getOffset()
```

#### Vl53lxDriver.setOffset

```java
public void setOffset(int offsetMm)
```

#### Vl53lxDriver.getXtalk

```java
public int getXtalk()
```

#### Vl53lxDriver.setXtalk

```java
public void setXtalk(int xtalkKcps)
```

#### Vl53lxDriver.calibrateOffset

```java
public int calibrateOffset(int targetDistMm, Integer nbSamples)
```

`nbSamples` null: the driver's default.

#### Vl53lxDriver.calibrateXtalk

```java
public int calibrateXtalk(int targetDistMm, Integer nbSamples)
```

#### Vl53lxDriver.getDetectionThresholds

```java
public Thresholds getDetectionThresholds()
```

#### Vl53lxDriver.setDetectionThresholds

```java
public void setDetectionThresholds(int distanceLowMm, int distanceHighMm, int window)
```

#### Vl53lxDriver.getSignalThreshold

```java
public int getSignalThreshold()
```

#### Vl53lxDriver.setSignalThreshold

```java
public void setSignalThreshold(int signalKcps)
```

#### Vl53lxDriver.getSigmaThreshold

```java
public int getSigmaThreshold()
```

#### Vl53lxDriver.setSigmaThreshold

```java
public void setSigmaThreshold(int sigmaMm)
```

#### Vl53lxDriver.getRoi

```java
public int[] getRoi()
```

{x, y} SPADs.

#### Vl53lxDriver.setRoi

```java
public void setRoi(int x, int y)
```

#### Vl53lxDriver.getRoiCenter

```java
public int getRoiCenter()
```

#### Vl53lxDriver.setRoiCenter

```java
public void setRoiCenter(int centerSpad)
```

#### Vl53lxDriver.startTemperatureUpdate

```java
public void startTemperatureUpdate()
```

#### Vl53lxDriver.performRefSpadManagement

```java
public Object performRefSpadManagement()
```

#### Vl53lxDriver.binData

```java
public Object binData(byte[] raw)
```

The raw histogram of a frame (read from the sensor when `raw` is null).

#### Vl53lxDriver.toMeasurement

```java
public Vl53lxRange toMeasurement(Object bins)
```

A histogram frame → a range (the host-side ranging of the histogram driver).

#### Vl53lxDriver.unsupported

```java
protected UnsupportedOperationException unsupported(String group)
```

### Vl53lxDriver.Thresholds

```java
public record Thresholds(int distanceLowMm, int distanceHighMm, int window)
```

The distance-window interrupt: `window` is one of `Vl53l1Die.WINDOW_*`.

### Vl53lxDrivers

```java
public final class Vl53lxDrivers
```

The drivers behind the product table (`uld/registry.py`): which (product, kind) pairs exist,
the driver each one runs and what to tell the user about it. A missing pair is absent on purpose.

#### Vl53lxDrivers.driverKinds

```java
public static List<String> driverKinds(String product)
```

The driver kinds `product` has, in `Vl53lxProducts#DRIVER_KINDS` order.

#### Vl53lxDrivers.createDriver

```java
public static Created createDriver(String product, String kind, Vl53lxPlatform platform)
```

The driver for one (product, kind) pair, and its caveat or null.

### Vl53lxDrivers.Created

```java
public record Created(Vl53lxDriver driver, String caveat)
```

A driver bound to a product, and the caveat a UI shows about the pair (or null).

### Vl53lxException

```java
public class Vl53lxException extends DepzException
```

A ULD-level failure: the sensor did not do what the driver needed (a timeout waiting for data
ready or boot, a calibration that failed, a mode the driver lacks…).

### Vl53lxHistogramDriver

```java
public final class Vl53lxHistogramDriver extends Vl53lxDriver
```

ST's VL53LX BareDriver on the host — the histogram driver of VL53L1CX / L1CB / L3CX / L4CD / L4CX:
the die hands over 24 raw photon-count bins per frame and `Hist` turns them into up to four
targets. What it costs against the light drivers: no calibrations and no detection thresholds, and
the first frame of a stream reports status 6 (no wrap check done) because the check needs the frame
before.

#### Vl53lxHistogramDriver.bare

```java
public BareDriver bare()
```

The BareDriver underneath: device image, factory data, preset and post-processing config.

#### Vl53lxHistogramDriver.waitBoot

```java
public void waitBoot()
```

#### Vl53lxHistogramDriver.waitBoot

```java
public void waitBoot(long timeoutMs)
```

### Vl53lxPlatform

```java
public abstract class Vl53lxPlatform
```

ST's platform layer (RdByte / WrWord / WaitMs …) for the 1D family, plus what the bridge owns: bus
speed, register-address width and the XSHUT line. Register values are big-endian, as the C drivers
assume. `Vl53lxDevice` implements it over the board; a test can script it.

#### Vl53lxPlatform.sleepMs

```java
public void sleepMs(int ms)
```

#### Vl53lxPlatform.readByte

```java
public int readByte(int addr)
```

#### Vl53lxPlatform.readWord

```java
public int readWord(int addr)
```

#### Vl53lxPlatform.readDword

```java
public long readDword(int addr)
```

#### Vl53lxPlatform.writeByte

```java
public void writeByte(int addr, int value)
```

#### Vl53lxPlatform.writeWord

```java
public void writeWord(int addr, int value)
```

#### Vl53lxPlatform.writeDword

```java
public void writeDword(int addr, long value)
```

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

Each pair is described by the bridge parameters its driver hands the
firmware (`Bridge`); the drivers themselves are `Vl53lxDrivers`,
the live device `ai.depz.sensor.devices.Vl53lxDevice`.

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

### Vl53lxProtocolException

```java
public final class Vl53lxProtocolException extends DepzException
```

The bridge refused a register command or did not answer it. The drivers catch it where a sensor
legitimately NACKs for a while (right after a soft reset).

### Vl53lxRange

```java
public record Vl53lxRange(int distanceMm, int status, String statusText, double signalKcps, double ambientKcps, double sigmaMm, double spads, List<Vl53lxTarget> targets, Map<String, Object> extra)
```

One range result as a driver reports it, the same shape for every product of the family. On the
histogram driver `targets` holds every return and the top level repeats the first; the light
drivers leave it empty. `extra` is driver-specific — show it, don't branch on it.

#### Vl53lxRange.valid

```java
public boolean valid()
```

### Vl53lxTarget

```java
public record Vl53lxTarget(int distanceMm, int status, String statusText, double signalKcps, double ambientKcps, double sigmaMm, int minRangeMm, int maxRangeMm)
```

One return of a multi-target frame. `minRangeMm` / `maxRangeMm` are the edges of the
target's own pulse, which only the histogram driver knows.

### BareDeviceState

```java
public enum BareDeviceState
```

The ll-driver device states, single-zone case.

#### BareDeviceState — constants

`SW_STANDBY`, `RANGING_WAIT_GPH_SYNC`, `RANGING_OUTPUT_DATA`, `RANGING_DSS_AUTO`

### BareDriver

```java
public final class BareDriver
```

ST's VL53LX BareDriver ranging path, on top of `DeviceImage`: `VL53LX_read_p2p_data()`,
the histogram preset modes, `VL53LX_set_timeouts_us()`, `VL53LX_init_and_start_range()`
and `VL53LX_get_histogram_bin_data()`. Nothing here writes a register on its own: the preset
builders edit the host-side image and the whole span 0x0001..0x0087 goes out as one transfer when
ranging starts.

#### BareDriver.img

```java
public DeviceImage img()
```

#### BareDriver.nvm

```java
public NvmReader nvm()
```

#### BareDriver.tuning

```java
public Map<String, Long> tuning()
```

The tuning parameters, a mutable copy of `BareTables#TUNING`.

#### BareDriver.histCfg

```java
public HistConfig histCfg()
```

#### BareDriver.hpp

```java
public HistPostProcessConfig hpp()
```

#### BareDriver.state

```java
public LlState state()
```

#### BareDriver.rtnGoodSpadsMap

```java
public byte[] rtnGoodSpadsMap()
```

#### BareDriver.opticalCentre

```java
public NvmReader.OpticalCentre opticalCentre()
```

#### BareDriver.calPeakRateMap

```java
public NvmReader.CalPeakRateMap calPeakRateMap()
```

#### BareDriver.addOffCalData

```java
public NvmReader.AdditionalOffsetCalData addOffCalData()
```

#### BareDriver.fmtDmaxCal

```java
public Map<String, Long> fmtDmaxCal()
```

#### BareDriver.mmRoi

```java
public Map<String, Long> mmRoi()
```

#### BareDriver.resultOscCalibrateVal

```java
public long resultOscCalibrateVal()
```

#### BareDriver.presetMode

```java
public String presetMode()
```

#### BareDriver.phasecalConfigTimeoutUs

```java
public long phasecalConfigTimeoutUs()
```

#### BareDriver.mmConfigTimeoutUs

```java
public long mmConfigTimeoutUs()
```

#### BareDriver.rangeConfigTimeoutUs

```java
public long rangeConfigTimeoutUs()
```

#### BareDriver.interMeasurementPeriodMs

```java
public long interMeasurementPeriodMs()
```

#### BareDriver.measurementMode

```java
public long measurementMode()
```

#### BareDriver.readP2pData

```java
public void readP2pData()
```

VL53LX_read_p2p_data(): three register blocks and four NVM regions (~0.1 s over USB).

#### BareDriver.dataInit

```java
public void dataInit()
```

VL53LX_data_init + VL53LX_DataInit: the factory data, the default preset mode and timing
budget. Leaves the sensor untouched apart from the reads.

#### BareDriver.setPresetMode

```java
public void setPresetMode(String mode)
```

VL53LX_set_preset_mode() for the three histogram ranging modes, by distance mode.

#### BareDriver.setPresetMode

```java
public void setPresetMode(String mode, Long dssTargetMcps, Long interMeasurementPeriodMs)
```

VL53LX_set_preset_mode() for the three histogram ranging modes, by distance mode; a null DSS
target or period keeps the tuning default / the current period.

#### BareDriver.isL4

```java
public boolean isL4()
```

`IsL4()` of the L4CX BareDriver: the die, read from NVM, not the product name. 0xEC is the L4ED.

#### BareDriver.distanceModesForDie

```java
public List<String> distanceModesForDie()
```

#### BareDriver.fdaMaxTimingBudgetUs

```java
public long fdaMaxTimingBudgetUs()
```

#### BareDriver.setDistanceMode

```java
public void setDistanceMode(String mode)
```

VL53LX_SetDistanceMode(): the preset mode with the caller's timeouts put back. Short is refused
on an L4 die, as the L4CX BareDriver does (the A frame of the pair ranges on the wrong side of
the wrap).

#### BareDriver.setTimeoutsUs

```java
public void setTimeoutsUs(long phasecalUs, long mmUs, long rangeUs)
```

#### BareDriver.setInterMeasurementPeriodMs

```java
public void setInterMeasurementPeriodMs(long periodMs)
```

#### BareDriver.setMeasurementTimingBudgetUs

```java
public void setMeasurementTimingBudgetUs(long budgetUs)
```

VL53LX_SetMeasurementTimingBudgetMicroSeconds(): the range timeout is one sixth of the budget
after a fixed guard comes off.

#### BareDriver.loadPatch

```java
public void loadPatch()
```

VL53LX_load_patch(): a six-instruction phasecal patch, written with the firmware stopped.

#### BareDriver.unloadPatch

```java
public void unloadPatch()
```

#### BareDriver.initAndStartRange

```java
public void initAndStartRange()
```

VL53LX_init_and_start_range() with every block: write the whole configuration and start.

#### BareDriver.initAndStartRange

```java
public void initAndStartRange(List<String> blocks)
```

VL53LX_init_and_start_range(): write the configuration and start. `blocks` is ST's config
level — all of them to start, general_config onwards to let the next frame run.

#### BareDriver.clearInterruptAndStartNextRange

```java
public void clearInterruptAndStartNextRange()
```

VL53LX_clear_interrupt_and_enable_next_range(): in histogram mode the interrupt is cleared by
rewriting general_config onwards.

#### BareDriver.stopRange

```java
public void stopRange()
```

#### BareDriver.getHistogramBinData

```java
public HistogramBinData getHistogramBinData(byte[] raw)
```

VL53LX_get_histogram_bin_data(): one frame, 83 bytes, decoded into bins plus the metadata the
post-processing needs. `raw` is bytes that arrived some other way (a stream report);
null reads them here.

#### BareDriver.getHistogramBinData

```java
public HistogramBinData getHistogramBinData()
```

`getHistogramBinData(byte[])` reading the frame off the sensor.

### BareMath

```java
public final class BareMath
```

SPAD geometry and timing maths (vl53lx_core.c, vl53lx_core_support.c).

#### BareMath.encodeRowCol

```java
public static int encodeRowCol(int row, int col)
```

#### BareMath.decodeRowCol

```java
public static int[] decodeRowCol(int spadNumber)
```

→ {row, col}.

#### BareMath.decodeZoneSize

```java
public static int[] decodeZoneSize(int encodedXySize)
```

→ {width, height}.

#### BareMath.encodeZoneSize

```java
public static int encodeZoneSize(int width, int height)
```

#### BareMath.decodeZoneLimits

```java
public static int[] decodeZoneLimits(int encodedXyCentre, int encodedXySize)
```

→ {x_ll, y_ll, x_ur, y_ur}, clipped to the SPAD array.

#### BareMath.isApertureLocation

```java
public static boolean isApertureLocation(int row, int col)
```

#### BareMath.calcMmEffectiveSpads

```java
public static long[] calcMmEffectiveSpads(int encodedMmRoiCentre, int encodedMmRoiSize, int encodedZoneCentre, int encodedZoneSize, byte[] goodSpads, long apertureAttenuation)
```

→ {mm inner, mm outer} effective SPADs, in 1/256 of a SPAD. Needed because our boards ship
without the FMT offset calibration: the driver works out the SPAD counts the nominal rates
apply to.

#### BareMath.combineMm1Mm2Offsets

```java
public static long combineMm1Mm2Offsets(long mm1OffsetMm, long mm2OffsetMm, int encodedMmRoiCentre, int encodedMmRoiSize, int encodedZoneCentre, int encodedZoneSize, NvmReader.AdditionalOffsetCalData cal, byte[] goodSpads, long apertureAttenuation)
```

`VL53LX_hist_combine_mm1_mm2_offsets()` → `range_offset_mm`, in quarter
millimetres: the MM1 (inner) and MM2 (outer) offsets weighted by the share of each region the
ROI covers.

#### BareMath.rtnGoodSpads

```java
public static byte[] rtnGoodSpads(RegBlock nvmCopyData)
```

The 32 `global_config__spad_enables_rtn_*` bytes as one bitmap.

#### BareMath.calcPllPeriodUs

```java
public static long calcPllPeriodUs(long fastOscFrequency)
```

#### BareMath.decodeVcselPeriod

```java
public static long decodeVcselPeriod(long vcselPeriodReg)
```

#### BareMath.calcMacroPeriodUs

```java
public static long calcMacroPeriodUs(long fastOscFrequency, long vcselPeriodReg)
```

#### BareMath.calcTimeoutMclks

```java
public static long calcTimeoutMclks(long timeoutUs, long macroPeriodUs)
```

#### BareMath.encodeTimeout

```java
public static long encodeTimeout(long timeoutMclks)
```

#### BareMath.decodeTimeout

```java
public static long decodeTimeout(long encoded)
```

#### BareMath.calcEncodedTimeout

```java
public static long calcEncodedTimeout(long timeoutUs, long macroPeriodUs)
```

#### BareMath.durationMaths

```java
public static long durationMaths(long pllPeriodUs, long vcselParmPclks, long windowVclks, long elapsedMclks)
```

#### BareMath.calcTimeoutRegisterValues

```java
public static void calcTimeoutRegisterValues(long phasecalUs, long mmUs, long rangeUs, long fastOscFrequency, RegBlock gen, RegBlock tim)
```

Fill the six timeout registers, for VCSEL period A and then B.

### BareRegs

```java
public final class BareRegs
```

Register addresses and settings of the BareDriver (vl53lx_register_map.h, vl53lx_ll_device.h).

#### BareRegs.POWER_MANAGEMENT_GO1_POWER_FORCE *(field)*

```java
public static final int POWER_MANAGEMENT_GO1_POWER_FORCE = 0x0083
```

#### BareRegs.FIRMWARE_ENABLE *(field)*

```java
public static final int FIRMWARE_ENABLE = 0x0085
```

#### BareRegs.RESULT_OSC_CALIBRATE_VAL *(field)*

```java
public static final int RESULT_OSC_CALIBRATE_VAL = 0x00DE
```

#### BareRegs.PATCH_CTRL *(field)*

```java
public static final int PATCH_CTRL = 0x0470
```

#### BareRegs.PATCH_JMP_ENABLES *(field)*

```java
public static final int PATCH_JMP_ENABLES = 0x0472
```

#### BareRegs.PATCH_DATA_ENABLES *(field)*

```java
public static final int PATCH_DATA_ENABLES = 0x0474
```

#### BareRegs.PATCH_OFFSET_0 *(field)*

```java
public static final int PATCH_OFFSET_0 = 0x0476
```

#### BareRegs.PATCH_ADDRESS_0 *(field)*

```java
public static final int PATCH_ADDRESS_0 = 0x0496
```

#### BareRegs.HISTOGRAM_BIN_DATA_I2C_INDEX *(field)*

```java
public static final int HISTOGRAM_BIN_DATA_I2C_INDEX = 0x0088
```

#### BareRegs.RESULT_HISTOGRAM_BIN_0_2 *(field)*

```java
public static final int RESULT_HISTOGRAM_BIN_0_2 = 0x008E
```

#### BareRegs.RESULT_HISTOGRAM_BIN_23_0 *(field)*

```java
public static final int RESULT_HISTOGRAM_BIN_23_0 = 0x00D5
```

#### BareRegs.PHASECAL_RESULT_REFERENCE_PHASE *(field)*

```java
public static final int PHASECAL_RESULT_REFERENCE_PHASE = 0x00D6
```

#### BareRegs.PHASECAL_RESULT_VCSEL_START *(field)*

```java
public static final int PHASECAL_RESULT_VCSEL_START = 0x00D8
```

#### BareRegs.RESULT_HISTOGRAM_BIN_23_0_MSB *(field)*

```java
public static final int RESULT_HISTOGRAM_BIN_23_0_MSB = 0x00D9
```

#### BareRegs.RESULT_HISTOGRAM_BIN_23_0_LSB *(field)*

```java
public static final int RESULT_HISTOGRAM_BIN_23_0_LSB = 0x00DA
```

#### BareRegs.HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES *(field)*

```java
public static final int HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES
```

#### BareRegs.DEVICE_MEASUREMENT_MODE_BACK_TO_BACK *(field)*

```java
public static final int DEVICE_MEASUREMENT_MODE_BACK_TO_BACK = 0x20
```

#### BareRegs.DEVICE_MEASUREMENT_MODE_ABORT *(field)*

```java
public static final int DEVICE_MEASUREMENT_MODE_ABORT = 0x80
```

#### BareRegs.DEVICE_MEASUREMENT_MODE_STOP_MASK *(field)*

```java
public static final int DEVICE_MEASUREMENT_MODE_STOP_MASK = 0x0F
```

#### BareRegs.DEVICE_MEASUREMENT_MODE_MODE_MASK *(field)*

```java
public static final int DEVICE_MEASUREMENT_MODE_MODE_MASK = 0xF0
```

#### BareRegs.DEVICE_SCHEDULER_MODE_STREAMING *(field)*

```java
public static final int DEVICE_SCHEDULER_MODE_STREAMING = 0x01
```

#### BareRegs.DEVICE_SCHEDULER_MODE_HISTOGRAM *(field)*

```java
public static final int DEVICE_SCHEDULER_MODE_HISTOGRAM = 0x02
```

#### BareRegs.DEVICE_READOUT_MODE_SINGLE_SD *(field)*

```java
public static final int DEVICE_READOUT_MODE_SINGLE_SD
```

#### BareRegs.DEVICE_READOUT_MODE_DUAL_SD *(field)*

```java
public static final int DEVICE_READOUT_MODE_DUAL_SD
```

#### BareRegs.GROUPED_PARAMETER_HOLD_ID_MASK *(field)*

```java
public static final int GROUPED_PARAMETER_HOLD_ID_MASK = 0x02
```

#### BareRegs.INTERRUPT_CONFIG_NEW_SAMPLE_READY *(field)*

```java
public static final int INTERRUPT_CONFIG_NEW_SAMPLE_READY = 0x20
```

#### BareRegs.CLEAR_RANGE_INT *(field)*

```java
public static final int CLEAR_RANGE_INT = 0x01
```

#### BareRegs.RANGE_STATUS_MASK *(field)*

```java
public static final int RANGE_STATUS_MASK = 0x1F
```

#### BareRegs.DEVICE_INTERRUPT_POLARITY_ACTIVE_LOW *(field)*

```java
public static final int DEVICE_INTERRUPT_POLARITY_ACTIVE_LOW = 0x10
```

#### BareRegs.DEVICE_GPIO_MODE_OUTPUT_RANGE_AND_ERROR_INTERRUPTS *(field)*

```java
public static final int DEVICE_GPIO_MODE_OUTPUT_RANGE_AND_ERROR_INTERRUPTS = 0x01
```

#### BareRegs.DEVICE_DSS_MODE_TARGET_RATE *(field)*

```java
public static final int DEVICE_DSS_MODE_TARGET_RATE = 1
```

#### BareRegs.SEQUENCE_VHV_EN *(field)*

```java
public static final int SEQUENCE_VHV_EN = 0x01
```

#### BareRegs.SEQUENCE_PHASECAL_EN *(field)*

```java
public static final int SEQUENCE_PHASECAL_EN = 0x02
```

#### BareRegs.SEQUENCE_DSS1_EN *(field)*

```java
public static final int SEQUENCE_DSS1_EN = 0x08
```

#### BareRegs.SEQUENCE_DSS2_EN *(field)*

```java
public static final int SEQUENCE_DSS2_EN = 0x10
```

#### BareRegs.SEQUENCE_MM1_EN *(field)*

```java
public static final int SEQUENCE_MM1_EN = 0x20
```

#### BareRegs.SEQUENCE_MM2_EN *(field)*

```java
public static final int SEQUENCE_MM2_EN = 0x40
```

#### BareRegs.SEQUENCE_RANGE_EN *(field)*

```java
public static final int SEQUENCE_RANGE_EN = 0x80
```

#### BareRegs.SPAD_ARRAY_WIDTH *(field)*

```java
public static final int SPAD_ARRAY_WIDTH = 16
```

#### BareRegs.SPAD_ARRAY_HEIGHT *(field)*

```java
public static final int SPAD_ARRAY_HEIGHT = 16
```

#### BareRegs.RTN_SPAD_APERTURE_TRANSMISSION *(field)*

```java
public static final int RTN_SPAD_APERTURE_TRANSMISSION = 0x0038
```

#### BareRegs.RTN_SPAD_UNITY_TRANSMISSION *(field)*

```java
public static final int RTN_SPAD_UNITY_TRANSMISSION = 0x0100
```

#### BareRegs.AMBIENT_WINDOW_VCSEL_PERIODS *(field)*

```java
public static final int AMBIENT_WINDOW_VCSEL_PERIODS = 256
```

#### BareRegs.RANGING_WINDOW_VCSEL_PERIODS *(field)*

```java
public static final int RANGING_WINDOW_VCSEL_PERIODS = 2048
```

#### BareRegs.MACRO_PERIOD_VCSEL_PERIODS *(field)*

```java
public static final int MACRO_PERIOD_VCSEL_PERIODS
```

#### BareRegs.HISTOGRAM_BUFFER_SIZE *(field)*

```java
public static final int HISTOGRAM_BUFFER_SIZE = 24
```

#### BareRegs.DISTANCE_MODES *(field)*

```java
public static final List<String> DISTANCE_MODES
```

The three preset modes VL53L3CX/VL53L4CX expose, by the distance mode that picks them.

#### BareRegs.L4_DISTANCE_MODES *(field)*

```java
public static final List<String> L4_DISTANCE_MODES
```

The L4CX BareDriver (STSW-IMG029) has no short mode on an L4 die.

#### BareRegs.TIMING_GUARD_US *(field)*

```java
public static final int TIMING_GUARD_US = 1700
```

#### BareRegs.TIMING_DIVISOR *(field)*

```java
public static final int TIMING_DIVISOR = 6
```

#### BareRegs.FDA_MAX_TIMING_BUDGET_US *(field)*

```java
public static final int FDA_MAX_TIMING_BUDGET_US = 550000
```

#### BareRegs.L4_FDA_MAX_TIMING_BUDGET_US *(field)*

```java
public static final int L4_FDA_MAX_TIMING_BUDGET_US = 200000
```

### BareTables

```java
public final class BareTables
```

The register-block layout and the tuning parameters of the VL53LX BareDriver (generated data).

#### BareTables.BLOCKS *(field)*

```java
public static final Map<String, RegBlockDef> BLOCKS
```

The register blocks of the device image: base, size, writable and the fields with offset,
width, signedness and the mask the C codec applies (null: none).

#### BareTables.TUNING *(field)*

```java
public static final Map<String, Long> TUNING
```

`VL53LX_tuning_parm_storage_t` after initialisation, keyed by the C field name.

#### BareTables.DEFAULTS *(field)*

```java
public static final Map<String, Long> DEFAULTS
```

The raw tuning header, for the parameters the post-processing reads without a `tp_` field.

### DeviceImage

```java
public final class DeviceImage
```

The device's register image, block by block. The C driver never reads a configuration register
on its own: it edits the structs, then pushes whole blocks — ranging start writes 0x0001..0x0087
in a single 135-byte transfer. Every transfer costs a USB round trip, so block moves keep the
count where the C driver has it. Editing a field only touches the host copy; `push`
reaches the sensor.

#### DeviceImage.RANGE_START_BLOCKS *(field)*

```java
public static final List<String> RANGE_START_BLOCKS
```

The blocks `init_and_start_range` pushes, in address order: one transfer.

#### DeviceImage.get

```java
public RegBlock get(String name)
```

#### DeviceImage.staticNvm

```java
public RegBlock staticNvm()
```

#### DeviceImage.customerNvm

```java
public RegBlock customerNvm()
```

#### DeviceImage.staticConfig

```java
public RegBlock staticConfig()
```

#### DeviceImage.generalConfig

```java
public RegBlock generalConfig()
```

#### DeviceImage.timingConfig

```java
public RegBlock timingConfig()
```

#### DeviceImage.dynamicConfig

```java
public RegBlock dynamicConfig()
```

#### DeviceImage.systemControl

```java
public RegBlock systemControl()
```

#### DeviceImage.nvmCopyData

```java
public RegBlock nvmCopyData()
```

#### DeviceImage.pull

```java
public RegBlock pull(String name)
```

Read one block off the sensor into the image.

#### DeviceImage.push

```java
public void push(String name)
```

Write one block from the image to the sensor.

#### DeviceImage.pullRange

```java
public void pullRange(List<String> names)
```

#### DeviceImage.pushRange

```java
public void pushRange(List<String> names)
```

The named blocks as one transfer; a hole between them goes out as 0x00, as in the C driver.

#### DeviceImage.dump

```java
public String dump(Iterable<String> names)
```

Every field of the named blocks, one per line.

#### DeviceImage.dump

```java
public String dump()
```

Every field of every block, one per line.

### FrameHistory

```java
public final class FrameHistory
```

The frame before this one, reduced to what the consistency checks read
(`VL53LX_zone_hist_info_t` and `VL53LX_zone_objects_t` for one zone). The engine
alternates two VCSEL periods, so one frame cannot say whether a target is inside the unambiguous
range or an echo wrapped into it; the API compares consecutive frames. The BareDriver keeps no
history, so the driver owns one of these and hands it every frame.

#### FrameHistory.reset

```java
public void reset()
```

Forget the stream so far: the next `apply` only remembers.

#### FrameHistory.apply

```java
public void apply(RangeResults results, HistogramBinData bins, BareDeviceState rdDeviceState, HistPostProcessConfig hpp)
```

Check `results` against the remembered frame, then remember it — C's order: both checks
run before hist_data and range_results are copied into the zone history.

### Hist

```java
public final class Hist
```

Histogram post-processing: 24 bins → targets, on the host alone, as `VL53LX_hist_process_data()`
does in C. Fold repeated bin codes, estimate the ambient floor, drop the ambient-only bins, threshold
each bin, take the runs of marked bins as pulses; each pulse is isolated, filtered with a three-tap
window and its zero crossing interpolated to a sub-bin phase, which converts to millimetres. ST's
source names its functions `VL53LX_f_NNN`; the names here are ours, the C name is in the
comment. Crosstalk compensation and the ambient dmax are not ported (dmax needs FMT data our boards
ship blank, so it would be zero anyway). Arithmetic is the C driver's: truncating division, and
saturation where it caps an intermediate.

#### Hist.MAX_BIN_SEQUENCE_LENGTH *(field)*

```java
public static final int MAX_BIN_SEQUENCE_LENGTH = 6
```

#### Hist.MAX_BIN_SEQUENCE_CODE *(field)*

```java
public static final int MAX_BIN_SEQUENCE_CODE = 15
```

#### Hist.MAX_PULSES *(field)*

```java
public static final int MAX_PULSES = 8
```

#### Hist.MAX_RANGE_RESULTS *(field)*

```java
public static final int MAX_RANGE_RESULTS = 4
```

#### Hist.MAX_ALLOWED_PHASE *(field)*

```java
public static final long MAX_ALLOWED_PHASE = 0xFFFF
```

#### Hist.SPAD_TOTAL_COUNT_MAX *(field)*

```java
public static final long SPAD_TOTAL_COUNT_MAX
```

#### Hist.SPAD_TOTAL_COUNT_RES_THRES *(field)*

```java
public static final long SPAD_TOTAL_COUNT_RES_THRES
```

#### Hist.SPEED_OF_LIGHT_IN_AIR_DIV8 *(field)*

```java
public static final long SPEED_OF_LIGHT_IN_AIR_DIV8
```

#### Hist.SIGMA_INVALID *(field)*

```java
public static final long SIGMA_INVALID = 0xFFFF
```

#### Hist.DEVICE_ERROR_NO_UPDATE *(field)*

```java
public static final long DEVICE_ERROR_NO_UPDATE = 0
```

#### Hist.DEVICE_ERROR_RANGE_PHASE_CHECK *(field)*

```java
public static final long DEVICE_ERROR_RANGE_PHASE_CHECK = 5
```

#### Hist.DEVICE_ERROR_SIGMA_THRESHOLD_CHECK *(field)*

```java
public static final long DEVICE_ERROR_SIGMA_THRESHOLD_CHECK = 6
```

#### Hist.DEVICE_ERROR_PHASE_CONSISTENCY *(field)*

```java
public static final long DEVICE_ERROR_PHASE_CONSISTENCY = 7
```

#### Hist.DEVICE_ERROR_RANGE_COMPLETE *(field)*

```java
public static final long DEVICE_ERROR_RANGE_COMPLETE = 9
```

#### Hist.DEVICE_ERROR_RANGE_COMPLETE_NO_WRAP_CHECK *(field)*

```java
public static final long DEVICE_ERROR_RANGE_COMPLETE_NO_WRAP_CHECK = 19
```

#### Hist.DEVICE_ERROR_EVENT_CONSISTENCY *(field)*

```java
public static final long DEVICE_ERROR_EVENT_CONSISTENCY = 20
```

#### Hist.DEVICE_ERROR_RANGE_COMPLETE_MERGED_PULSE *(field)*

```java
public static final long DEVICE_ERROR_RANGE_COMPLETE_MERGED_PULSE = 22
```

#### Hist.DEVICE_ERROR_PREV_RANGE_NO_TARGETS *(field)*

```java
public static final long DEVICE_ERROR_PREV_RANGE_NO_TARGETS = 23
```

#### Hist.HIST_TARGET_ORDER_STRONGEST_FIRST *(field)*

```java
public static final long HIST_TARGET_ORDER_STRONGEST_FIRST = 1
```

#### Hist.HIST_TARGET_ORDER_CLOSEST_FIRST *(field)*

```java
public static final long HIST_TARGET_ORDER_CLOSEST_FIRST = 2
```

#### Hist.RANGE_STATUS_RANGE_VALID *(field)*

```java
public static final int RANGE_STATUS_RANGE_VALID = 0
```

#### Hist.RANGE_STATUS_SIGMA_FAIL *(field)*

```java
public static final int RANGE_STATUS_SIGMA_FAIL = 1
```

#### Hist.RANGE_STATUS_OUT_OF_BOUNDS_FAIL *(field)*

```java
public static final int RANGE_STATUS_OUT_OF_BOUNDS_FAIL = 4
```

#### Hist.RANGE_STATUS_RANGE_VALID_NO_WRAP_CHECK *(field)*

```java
public static final int RANGE_STATUS_RANGE_VALID_NO_WRAP_CHECK = 6
```

#### Hist.RANGE_STATUS_WRAP_TARGET_FAIL *(field)*

```java
public static final int RANGE_STATUS_WRAP_TARGET_FAIL = 7
```

#### Hist.RANGE_STATUS_RANGE_VALID_MERGED_PULSE *(field)*

```java
public static final int RANGE_STATUS_RANGE_VALID_MERGED_PULSE = 11
```

#### Hist.RANGE_STATUS_TARGET_PRESENT_LACK_OF_SIGNAL *(field)*

```java
public static final int RANGE_STATUS_TARGET_PRESENT_LACK_OF_SIGNAL = 12
```

#### Hist.RANGE_STATUS_NONE *(field)*

```java
public static final int RANGE_STATUS_NONE = 255
```

#### Hist.STATUS_NAMES *(field)*

```java
public static final Map<Integer, String> STATUS_NAMES
```

UM2931 range status names.

#### Hist.pyMod

```java
public static long pyMod(long a, long b)
```

The modulo of the reference (sign of the divisor).

#### Hist.isqrt

```java
public static long isqrt(long num)
```

VL53LX_isqrt(): floor of the square root of a uint32 argument.

#### Hist.calcPllPeriodMm

```java
public static long calcPllPeriodMm(long fastOscFrequency)
```

VL53LX_calc_pll_period_mm().

#### Hist.rateMaths

```java
public static long rateMaths(long events, long timeUs)
```

VL53LX_rate_maths(): events over a duration → count rate in 9.7 Mcps.

#### Hist.ratePerSpadMaths

```java
public static long ratePerSpadMaths(int fracBits, long peakCountRate, long numSpads, long maxOutputValue)
```

VL53LX_rate_per_spad_maths().

#### Hist.eventsPerSpadMaths

```java
public static long eventsPerSpadMaths(long events, long numSpads, long duration)
```

VL53LX_events_per_spad_maths() → kcps per SPAD.

#### Hist.rangeMaths

```java
public static long rangeMaths(long fastOscFrequency, long phase, long zeroDistancePhase, int fractionalBits, long gainFactor, long rangeOffsetMm)
```

VL53LX_range_maths(): phase → millimetres.

#### Hist.averageRepeatedBins

```java
public static HistogramBinData averageRepeatedBins(HistogramBinData src)
```

VL53LX_f_031: fold the six bin-sequence codes together. Repeats of a code land on the same four
output bins and are averaged, so what comes out is one bin per distinct code.

#### Hist.calcZeroDistancePhase

```java
public static void calcZeroDistancePhase(HistogramBinData bins)
```

VL53LX_hist_calc_zero_distance_phase.

#### Hist.estimateAmbientFromThresholdedBins

```java
public static void estimateAmbientFromThresholdedBins(long ambientThresholdSigma, HistogramBinData bins)
```

VL53LX_hist_estimate_ambient_from_thresholded_bins(): the floor from every bin near the quietest.

#### Hist.estimateAmbientFromAmbientBins

```java
public static void estimateAmbientFromAmbientBins(HistogramBinData bins)
```

VL53LX_hist_estimate_ambient_from_ambient_bins(): the better estimate, from the VCSEL-off bins.

#### Hist.removeAmbientBins

```java
public static void removeAmbientBins(HistogramBinData bins)
```

VL53LX_hist_remove_ambient_bins: drop the ambient-only bins off the front, so bin 0 is phase zero.

#### Hist.processData

```java
public static RangeResults processData(HistogramBinData binsInput, HistPostProcessConfig hpp)
```

`VL53LX_hist_process_data()` + `VL53LX_f_025`, crosstalk off, one merged frame.

#### Hist.processData

```java
public static RangeResults processData(HistogramBinData binsInput, HistPostProcessConfig hpp, long mergeNb)
```

`VL53LX_hist_process_data()` + `VL53LX_f_025`, crosstalk off.

#### Hist.convertStatus

```java
public static int convertStatus(long deviceError)
```

ConvertStatusHisto() (vl53lx_api.c), the cases the histogram path produces here.

#### Hist.toRange

```java
public static Vl53lxRange toRange(RangeResults results, HistogramBinData bins)
```

The frame's results as the family-wide range: every return in `targets`, the first one
also the range itself. Effective SPADs come in 1/256 and are reported whole, as the rest of the
family does.

### HistConfig

```java
public final class HistConfig
```

`VL53LX_histogram_config_t`. It has no registers of its own: the bin sequence is smuggled
into static_config and timing_config fields that mean something else in lite mode
(`copyToStaticCfg`).

#### HistConfig.lowAmbEvenBin01 *(field)*

```java
public long lowAmbEvenBin01
```

#### HistConfig.lowAmbEvenBin23 *(field)*

```java
public long lowAmbEvenBin23
```

#### HistConfig.lowAmbEvenBin45 *(field)*

```java
public long lowAmbEvenBin45
```

#### HistConfig.lowAmbOddBin01 *(field)*

```java
public long lowAmbOddBin01
```

#### HistConfig.lowAmbOddBin23 *(field)*

```java
public long lowAmbOddBin23
```

#### HistConfig.lowAmbOddBin45 *(field)*

```java
public long lowAmbOddBin45
```

#### HistConfig.midAmbEvenBin01 *(field)*

```java
public long midAmbEvenBin01
```

#### HistConfig.midAmbEvenBin23 *(field)*

```java
public long midAmbEvenBin23
```

#### HistConfig.midAmbEvenBin45 *(field)*

```java
public long midAmbEvenBin45
```

#### HistConfig.midAmbOddBin01 *(field)*

```java
public long midAmbOddBin01
```

#### HistConfig.midAmbOddBin2 *(field)*

```java
public long midAmbOddBin2
```

#### HistConfig.midAmbOddBin34 *(field)*

```java
public long midAmbOddBin34
```

#### HistConfig.midAmbOddBin5 *(field)*

```java
public long midAmbOddBin5
```

#### HistConfig.userBinOffset *(field)*

```java
public long userBinOffset
```

#### HistConfig.highAmbEvenBin01 *(field)*

```java
public long highAmbEvenBin01
```

#### HistConfig.highAmbEvenBin23 *(field)*

```java
public long highAmbEvenBin23
```

#### HistConfig.highAmbEvenBin45 *(field)*

```java
public long highAmbEvenBin45
```

#### HistConfig.highAmbOddBin01 *(field)*

```java
public long highAmbOddBin01
```

#### HistConfig.highAmbOddBin23 *(field)*

```java
public long highAmbOddBin23
```

#### HistConfig.highAmbOddBin45 *(field)*

```java
public long highAmbOddBin45
```

#### HistConfig.ambThreshLow *(field)*

```java
public long ambThreshLow
```

#### HistConfig.ambThreshHigh *(field)*

```java
public long ambThreshHigh
```

#### HistConfig.spadArraySelection *(field)*

```java
public long spadArraySelection
```

#### HistConfig.setBinSequence

```java
public void setBinSequence(int[] even, int[] odd)
```

VL53LX_init_histogram_config_structure(): six even and six odd bin codes, packed two to a
byte, repeated across the three ambient levels.

#### HistConfig.copyToStaticCfg

```java
public void copyToStaticCfg(RegBlock stat, RegBlock timing, RegBlock dynamic)
```

VL53LX_copy_hist_cfg_to_static_cfg(): lite-mode fields the firmware reads as the bin sequence.

#### HistConfig.binSequence

```java
public int[] binSequence(long streamCount, long ambientEventsSum)
```

VL53LX_hist_get_bin_sequence_config(): which sequence the device used for this frame, even or
odd by the stream count.

### HistPostProcessConfig

```java
public final class HistPostProcessConfig
```

`VL53LX_hist_post_process_config_t`, as `VL53LX_init_hist_post_process_config_struct()`
leaves it; the preset modes set the valid phase window here.

#### HistPostProcessConfig.histAlgoSelect *(field)*

```java
public long histAlgoSelect
```

#### HistPostProcessConfig.histTargetOrder *(field)*

```java
public long histTargetOrder
```

#### HistPostProcessConfig.filterWoi0 *(field)*

```java
public long filterWoi0
```

#### HistPostProcessConfig.filterWoi1 *(field)*

```java
public long filterWoi1
```

#### HistPostProcessConfig.histAmbEstMethod *(field)*

```java
public long histAmbEstMethod
```

#### HistPostProcessConfig.ambientThreshSigma0 *(field)*

```java
public long ambientThreshSigma0
```

#### HistPostProcessConfig.ambientThreshSigma1 *(field)*

```java
public long ambientThreshSigma1
```

#### HistPostProcessConfig.ambientThreshEventsScaler *(field)*

```java
public long ambientThreshEventsScaler
```

#### HistPostProcessConfig.minAmbientThreshEvents *(field)*

```java
public long minAmbientThreshEvents
```

#### HistPostProcessConfig.noiseThreshold *(field)*

```java
public long noiseThreshold
```

#### HistPostProcessConfig.signalTotalEventsLimit *(field)*

```java
public long signalTotalEventsLimit
```

#### HistPostProcessConfig.sigmaEstimatorSigmaRefMm *(field)*

```java
public long sigmaEstimatorSigmaRefMm
```

#### HistPostProcessConfig.sigmaThresh *(field)*

```java
public long sigmaThresh
```

#### HistPostProcessConfig.rangeOffsetMm *(field)*

```java
public long rangeOffsetMm
```

#### HistPostProcessConfig.gainFactor *(field)*

```java
public long gainFactor
```

#### HistPostProcessConfig.validPhaseLow *(field)*

```java
public long validPhaseLow = 0x08
```

#### HistPostProcessConfig.validPhaseHigh *(field)*

```java
public long validPhaseHigh = 0x88
```

#### HistPostProcessConfig.consistencyPhaseTolerance *(field)*

```java
public long consistencyPhaseTolerance
```

#### HistPostProcessConfig.consistencyEventSigma *(field)*

```java
public long consistencyEventSigma
```

#### HistPostProcessConfig.consistencyEventMinSpadCount *(field)*

```java
public long consistencyEventMinSpadCount
```

#### HistPostProcessConfig.consistencyMinMaxTolerance *(field)*

```java
public long consistencyMinMaxTolerance
```

#### HistPostProcessConfig.crosstalkCompensationEnable *(field)*

```java
public long crosstalkCompensationEnable
```

#### HistPostProcessConfig.crosstalkDetectMinValidRangeMm *(field)*

```java
public long crosstalkDetectMinValidRangeMm
```

#### HistPostProcessConfig.crosstalkDetectMaxValidRangeMm *(field)*

```java
public long crosstalkDetectMaxValidRangeMm
```

#### HistPostProcessConfig.crosstalkDetectMaxValidRateKcps *(field)*

```java
public long crosstalkDetectMaxValidRateKcps
```

#### HistPostProcessConfig.crosstalkDetectMaxSigmaMm *(field)*

```java
public long crosstalkDetectMaxSigmaMm
```

#### HistPostProcessConfig.crosstalkDetectEventSigma *(field)*

```java
public long crosstalkDetectEventSigma
```

#### HistPostProcessConfig.crosstalkDetectMinMaxTolerance *(field)*

```java
public long crosstalkDetectMinMaxTolerance
```

#### HistPostProcessConfig.crosstalkCompensationPlaneOffsetKcps *(field)*

```java
public long crosstalkCompensationPlaneOffsetKcps
```

#### HistPostProcessConfig.crosstalkCompensationXPlaneGradientKcps *(field)*

```java
public long crosstalkCompensationXPlaneGradientKcps
```

#### HistPostProcessConfig.crosstalkCompensationYPlaneGradientKcps *(field)*

```java
public long crosstalkCompensationYPlaneGradientKcps
```

### HistogramBinData

```java
public final class HistogramBinData implements Cloneable
```

`VL53LX_histogram_bin_data_t`: 24 bins plus everything the post-processing needs to read
them — the VCSEL period they were taken at, the phase of zero distance, the bin sequence and the
ambient estimate.

#### HistogramBinData.resultInterruptStatus *(field)*

```java
public long resultInterruptStatus
```

#### HistogramBinData.resultRangeStatus *(field)*

```java
public long resultRangeStatus
```

#### HistogramBinData.resultReportStatus *(field)*

```java
public long resultReportStatus
```

#### HistogramBinData.resultStreamCount *(field)*

```java
public long resultStreamCount
```

#### HistogramBinData.resultDssActualEffectiveSpads *(field)*

```java
public long resultDssActualEffectiveSpads
```

#### HistogramBinData.phasecalResultReferencePhase *(field)*

```java
public long phasecalResultReferencePhase
```

#### HistogramBinData.phasecalResultVcselStart *(field)*

```java
public long phasecalResultVcselStart
```

#### HistogramBinData.binData *(field)*

```java
public long[] binData
```

#### HistogramBinData.zoneId *(field)*

```java
public long zoneId
```

#### HistogramBinData.firstBin *(field)*

```java
public long firstBin
```

#### HistogramBinData.numberOfBins *(field)*

```java
public long numberOfBins
```

#### HistogramBinData.binsInData *(field)*

```java
public long binsInData
```

#### HistogramBinData.calConfigVcselStart *(field)*

```java
public long calConfigVcselStart
```

#### HistogramBinData.vcselWidth *(field)*

```java
public long vcselWidth
```

#### HistogramBinData.fastOscFrequency *(field)*

```java
public long fastOscFrequency
```

#### HistogramBinData.vcselPeriod *(field)*

```java
public long vcselPeriod
```

#### HistogramBinData.binSeq *(field)*

```java
public int[] binSeq
```

#### HistogramBinData.binRep *(field)*

```java
public long[] binRep
```

#### HistogramBinData.minBinValue *(field)*

```java
public long minBinValue
```

#### HistogramBinData.maxBinValue *(field)*

```java
public long maxBinValue
```

#### HistogramBinData.numberOfAmbientBins *(field)*

```java
public long numberOfAmbientBins
```

#### HistogramBinData.numberOfAmbientSamples *(field)*

```java
public long numberOfAmbientSamples
```

#### HistogramBinData.ambientEventsSum *(field)*

```java
public long ambientEventsSum
```

#### HistogramBinData.ambientPerBin *(field)*

```java
public long ambientPerBin
```

#### HistogramBinData.totalPeriodsElapsed *(field)*

```java
public long totalPeriodsElapsed
```

#### HistogramBinData.peakDurationUs *(field)*

```java
public long peakDurationUs
```

#### HistogramBinData.woiDurationUs *(field)*

```java
public long woiDurationUs
```

#### HistogramBinData.zeroDistancePhase *(field)*

```java
public long zeroDistancePhase
```

#### HistogramBinData.roiConfigUserRoiCentreSpad *(field)*

```java
public long roiConfigUserRoiCentreSpad
```

#### HistogramBinData.roiConfigUserRoiRequestedGlobalXySize *(field)*

```java
public long roiConfigUserRoiRequestedGlobalXySize
```

#### HistogramBinData.rangeStatus

```java
public long rangeStatus()
```

#### HistogramBinData.fields

```java
public Map<String, Object> fields()
```

The fields under the C driver's (and the Python SDK's) names.

### LlState

```java
public final class LlState
```

`VL53LX_update_ll_driver_{rd,cfg}_state()`, reduced to one zone: the GPH handshake (the id
stamped into the dynamic config tells the device a configuration is new) and the timing-status
flip (whether a frame ran on VCSEL period A or B, which the post-processing has to know).

#### LlState.cfgDeviceState *(field)*

```java
public BareDeviceState cfgDeviceState
```

#### LlState.cfgStreamCount *(field)*

```java
public long cfgStreamCount
```

#### LlState.cfgGphId *(field)*

```java
public long cfgGphId
```

#### LlState.cfgTimingStatus *(field)*

```java
public long cfgTimingStatus
```

#### LlState.rdDeviceState *(field)*

```java
public BareDeviceState rdDeviceState
```

#### LlState.rdStreamCount *(field)*

```java
public long rdStreamCount
```

#### LlState.rdGphId *(field)*

```java
public long rdGphId
```

#### LlState.rdTimingStatus *(field)*

```java
public long rdTimingStatus
```

#### LlState.reset

```java
public void reset()
```

#### LlState.updateRd

```java
public void updateRd(long modeStart, long groupedParameterHold)
```

#### LlState.updateCfg

```java
public void updateCfg(long modeStart)
```

### NvmReader

```java
public final class NvmReader
```

The raw NVM, which the register blocks do not cover: the optical centre, the 5x5 peak-rate map,
the mode-mitigation offset calibration and the FMT range results. Each word costs four writes
and a read, so only the regions the driver wants are read, each inside its own enable/disable
pair so the sensor is never left with its firmware stopped (vl53lx_nvm.c).

#### NvmReader.FMT_OPTICAL_CENTRE *(field)*

```java
public static final int[] FMT_OPTICAL_CENTRE
```

NVM regions, {byte index, byte size}.

#### NvmReader.FMT_CAL_PEAK_RATE_MAP *(field)*

```java
public static final int[] FMT_CAL_PEAK_RATE_MAP
```

#### NvmReader.FMT_ADDITIONAL_OFFSET_CAL *(field)*

```java
public static final int[] FMT_ADDITIONAL_OFFSET_CAL
```

#### NvmReader.FMT_RANGE_RESULTS_140MM_DARK *(field)*

```java
public static final int[] FMT_RANGE_RESULTS_140MM_DARK
```

#### NvmReader.readRegion

```java
public byte[] readRegion(int[] region)
```

VL53LX_read_nvm_raw_data(): {byte index, byte size}, both word-aligned → the bytes.

#### NvmReader.readOpticalCentre

```java
public OpticalCentre readOpticalCentre()
```

#### NvmReader.readCalPeakRateMap

```java
public CalPeakRateMap readCalPeakRateMap()
```

#### NvmReader.readAdditionalOffsetCalData

```java
public AdditionalOffsetCalData readAdditionalOffsetCalData()
```

#### NvmReader.readFmtRangeResults

```java
public FmtRangeData readFmtRangeResults()
```

#### NvmReader.readFmtRangeResults

```java
public FmtRangeData readFmtRangeResults(int[] region)
```

### NvmReader.OpticalCentre

```java
public record OpticalCentre(int xCentre, int yCentre)
```

`VL53LX_optical_centre_t`.

### NvmReader.CalPeakRateMap

```java
public record CalPeakRateMap(int calDistanceMm, int calReflectancePc, int maxSamples, int width, int height, List<Integer> peakRateMcps)
```

`VL53LX_cal_peak_rate_map_t`.

### NvmReader.AdditionalOffsetCalData

```java
public record AdditionalOffsetCalData(long mmInnerActualEffectiveSpads, long mmOuterActualEffectiveSpads, long mmInnerPeakSignalCountRtnMcps, long mmOuterPeakSignalCountRtnMcps)
```

`VL53LX_additional_offset_cal_data_t`.

### NvmReader.FmtRangeData

```java
public record FmtRangeData(int actualEffectiveRtnSpads, int numRequestedRefSpads, int refLocation, int peakSignalCountRateRtnMcps, int ambientCountRateRtnMcps, int peakSignalCountRateRefMcps, int ambientCountRateRefMcps, int measuredDistanceMm, int measuredDistanceStdevMm)
```

`VL53LX_decoded_nvm_fmt_range_data_t`.

### RangeData

```java
public final class RangeData
```

`VL53LX_range_data_t`: one target. The event sums and phases are what the algorithm
produced; the `*RangeMm` fields and the rates are what they mean.

#### RangeData.rangeId *(field)*

```java
public long rangeId
```

#### RangeData.startBin *(field)*

```java
public long startBin
```

#### RangeData.firstBin *(field)*

```java
public long firstBin
```

#### RangeData.peakBin *(field)*

```java
public long peakBin
```

#### RangeData.lastBin *(field)*

```java
public long lastBin
```

#### RangeData.endBin *(field)*

```java
public long endBin
```

#### RangeData.widthBins *(field)*

```java
public long widthBins
```

#### RangeData.windowBins *(field)*

```java
public long windowBins
```

#### RangeData.vcselWidth *(field)*

```java
public long vcselWidth
```

#### RangeData.fastOscFrequency *(field)*

```java
public long fastOscFrequency
```

#### RangeData.zeroDistancePhase *(field)*

```java
public long zeroDistancePhase
```

#### RangeData.spads *(field)*

```java
public long spads
```

#### RangeData.totalPeriodsElapsed *(field)*

```java
public long totalPeriodsElapsed
```

#### RangeData.peakDurationUs *(field)*

```java
public long peakDurationUs
```

#### RangeData.woiDurationUs *(field)*

```java
public long woiDurationUs
```

#### RangeData.ambientEvents *(field)*

```java
public long ambientEvents
```

#### RangeData.totalEvents *(field)*

```java
public long totalEvents
```

#### RangeData.signalEvents *(field)*

```java
public long signalEvents
```

#### RangeData.peakSignalCountRateMcps *(field)*

```java
public long peakSignalCountRateMcps
```

#### RangeData.avgSignalCountRateMcps *(field)*

```java
public long avgSignalCountRateMcps
```

#### RangeData.ambientCountRateMcps *(field)*

```java
public long ambientCountRateMcps
```

#### RangeData.totalRatePerSpadMcps *(field)*

```java
public long totalRatePerSpadMcps
```

#### RangeData.signalEventsPerSpadKcps *(field)*

```java
public long signalEventsPerSpadKcps
```

#### RangeData.sigma *(field)*

```java
public long sigma
```

#### RangeData.phaseStart *(field)*

```java
public long phaseStart
```

#### RangeData.phaseMean *(field)*

```java
public long phaseMean
```

#### RangeData.phaseEnd *(field)*

```java
public long phaseEnd
```

#### RangeData.minRangeMm *(field)*

```java
public long minRangeMm
```

#### RangeData.medianRangeMm *(field)*

```java
public long medianRangeMm
```

#### RangeData.maxRangeMm *(field)*

```java
public long maxRangeMm
```

#### RangeData.rangeStatus *(field)*

```java
public long rangeStatus
```

#### RangeData.sigmaMm

```java
public long sigmaMm()
```

#### RangeData.signalKcps

```java
public long signalKcps()
```

The peak signal rate is 9.7 Mcps.

#### RangeData.ambientKcps

```java
public long ambientKcps()
```

### RangeResults

```java
public final class RangeResults
```

`VL53LX_range_results_t`, reduced to the single-zone case.

#### RangeResults.streamCount *(field)*

```java
public long streamCount
```

#### RangeResults.targets *(field)*

```java
public final List<RangeData> targets
```

#### RangeResults.ambientDmaxMm *(field)*

```java
public long[] ambientDmaxMm
```

#### RangeResults.wrapDmaxMm *(field)*

```java
public long wrapDmaxMm
```

### RegBlock

```java
public final class RegBlock
```

One register block's worth of device image, addressed by the C field name:
`img.staticConfig().get("dss_config__target_total_rate_mcps")`. Bytes no field names (the
reserved holes) keep whatever was read, so a read-modify-write round trip is byte-exact.

#### RegBlock.name

```java
public String name()
```

#### RegBlock.def

```java
public RegBlockDef def()
```

#### RegBlock.base

```java
public int base()
```

#### RegBlock.size

```java
public int size()
```

#### RegBlock.get

```java
public long get(String field)
```

The field by its C name; `NoSuchElementException` when the block has none.

#### RegBlock.set

```java
public void set(String field, long value)
```

#### RegBlock.items

```java
public List<Map.Entry<String, Long>> items()
```

Every field in block order, {name, value}.

#### RegBlock.decode

```java
public RegBlock decode(byte[] raw)
```

#### RegBlock.encode

```java
public byte[] encode()
```

### RegBlockDef

```java
public record RegBlockDef(int base, int size, boolean writable, List<RegField> fields)
```

One contiguous I2C block of the device image (a `VL53LX_*_t` struct of the C driver).

### RegField

```java
public record RegField(String name, int off, int width, boolean signed, Long mask)
```

One field of a register block: byte offset, width in bytes, signedness and the mask the C codec
applies (null: none).

## TMF8829 (dToF)

### Tmf8829AppliedConfig

```java
public record Tmf8829AppliedConfig(String preset, byte[] page, boolean histograms, boolean refFrame, Map<String, Integer> fields)
```

The config `Tmf8829Device#configure` wrote, as read back from the sensor: the preset name, the
190-byte page 0x22..0xDF, whether histogram frames are dumped and a ref-SPAD frame comes with each
measurement, and every named field of the page.

#### Tmf8829AppliedConfig.grid

```java
public int[] grid()
```

{rows, cols} of a measurement.

### Tmf8829Device

```java
public final class Tmf8829Device extends DepzDevice
```

TMF8829 multizone dToF (8×8 … 48×32 zones, up to 4 peaks per zone, histograms) over the SPI register
bridge (contract 14). The board is a thin bridge: this class runs the sensor. `init` downloads
the 14.4 KB sensor firmware (~0.15 s), `configure` writes the config page, `start`
streams measurements to `onMeasurement` / `measurements`, `stop` ends it. The
sensor's EN pin is wired high: there is no sleep, and recovery from any state is `init` (it
starts with a soft reset).

#### Tmf8829Device.appVersion

```java
public String appVersion()
```

The RAM application version `init` started ("2.200.0"), or null.

#### Tmf8829Device.sensorSerial

```java
public Long sensorSerial()
```

The sensor's serial number, read by `init`, or null.

#### Tmf8829Device.config

```java
public Tmf8829AppliedConfig config()
```

The last `configure` result (null after `init`).

#### Tmf8829Device.streaming

```java
public boolean streaming()
```

#### Tmf8829Device.readRegisters

```java
public byte[] readRegisters(int addr, int length)
```

Read `length` bytes from `addr` in ≤1024-byte requests; reads that reach the FIFO port
0xFF stay there (the sensor does not auto-increment it).

#### Tmf8829Device.readRegister

```java
public int readRegister(int addr)
```

#### Tmf8829Device.writeRegisters

```java
public void writeRegisters(int addr, byte[] data, BiConsumer<Integer, Integer> progress)
```

Write `data` from `addr` in ≤1023-byte requests (writes to 0xFF stay at 0xFF);
`progress` (may be null) gets (done, total) after each.

#### Tmf8829Device.writeRegisters

```java
public void writeRegisters(int addr, byte[] data)
```

#### Tmf8829Device.writeRegister

```java
public void writeRegister(int addr, int value)
```

Write one register; the low byte of `value`.

#### Tmf8829Device.sensorState

```java
public Map<String, Integer> sensorState()
```

Raw host-interface registers, for diagnostics.

#### Tmf8829Device.getBridgeInfo

```java
public Tmf8829.Tmf8829Info getBridgeInfo()
```

Bridge counters (never touches the sensor, safe mid-stream).

#### Tmf8829Device.init

```java
public InitResult init(BiConsumer<Integer, Integer> progress)
```

Bring the sensor from any state to its running RAM application: soft reset, bootloader, firmware
download, start. Stops a running stream first. `progress` (done, total; may be null) tracks
the image download.

#### Tmf8829Device.init

```java
public InitResult init()
```

#### Tmf8829Device.configPage

```java
public byte[] configPage()
```

LOAD_CONFIG_PAGE → the 190-byte page 0x22..0xDF now in the sensor.

#### Tmf8829Device.configure

```java
public Tmf8829AppliedConfig configure(String preset, int periodMs, Integer kiloIterations, int nrPeaks, boolean signalStrength, boolean noiseStrength, boolean xtalk, boolean histograms, boolean refFrame, Map<String, Integer> fields)
```

Load `preset` and write the config page. Every field a preset does not reset is written
(contract 14 §4), so the result never depends on an earlier config; `fields` (may be null)
sets any other page field by name. `kiloIterations` null: the preset's. Not while streaming.

is too big for the grid) or a written field does not read back

#### Tmf8829Device.configure

```java
public Tmf8829AppliedConfig configure(String preset, int periodMs)
```

#### Tmf8829Device.configure

```java
public Tmf8829AppliedConfig configure(String preset)
```

#### Tmf8829Device.configure

```java
public Tmf8829AppliedConfig configure()
```

The 8x8 preset at the default period.

#### Tmf8829Device.start

```java
public void start()
```

MEASURE with the bridge stream armed. Configures the default 8x8 preset first when
`configure` was not called since `init`.

#### Tmf8829Device.stop

```java
public void stop()
```

Sensor STOP, then the bridge stream (the frames already queued drain). The bad-header event a STOP
raises mid histogram dump is expected and swallowed.

#### Tmf8829Device.onMeasurement

```java
public AutoCloseable onMeasurement(Consumer<MeasurementBuilder.Measurement> callback)
```

Call `callback` for every measurement (reader thread; don't block).

#### Tmf8829Device.measurements

```java
public Subscription<MeasurementBuilder.Measurement> measurements(int capacity)
```

A pull queue of measurements (bounded, drop-oldest); subscribed when this returns.

#### Tmf8829Device.measurements

```java
public Subscription<MeasurementBuilder.Measurement> measurements()
```

#### Tmf8829Device.getMeasurement

```java
public MeasurementBuilder.Measurement getMeasurement(Duration timeout)
```

The next measurement (contract 07 §3).

#### Tmf8829Device.streamDroppedCounts

```java
public List<Long> streamDroppedCounts()
```

#### Tmf8829Device.framesDropped

```java
public int framesDropped()
```

Frames the bridge dropped (frame_seq gaps) in this stream.

#### Tmf8829Device.framesDiscarded

```java
public int framesDiscarded()
```

Frames lost to reassembly (incomplete, out-of-order chunks) in this stream.

#### Tmf8829Device.frameParseErrors

```java
public int frameParseErrors()
```

#### Tmf8829Device.clockRatioQ16

```java
public long clockRatioQ16()
```

Current sensor-vs-board clock ratio in Q16 (contract 14 §5; nominal 8 << 16 = 8 µs per SYSTICK),
for diagnostics.

### Tmf8829Device.InitResult

```java
public record InitResult(String appVersion, long sensorSerial)
```

What `init` reports: the RAM application version and the sensor's serial number.

### Tmf8829Exception

```java
public final class Tmf8829Exception extends DepzException
```

The TMF8829 refused or did not answer (bootloader / application status).

### Tmf8829InterruptEvent

```java
public record Tmf8829InterruptEvent(Tmf8829.Tmf8829Event event) implements DeviceEvent
```

RPT_TMF_EVENT that is not an expected stop artefact: a MOTION / PROXIMITY interrupt (code 0) or a
refused frame header (code 1 — the sensor is lost, run `Tmf8829Device#init` again).

### Tmf8829

```java
public final class Tmf8829
```

TMF8829 SPI register-bridge wire codecs, protocol v1.00
(contracts/14_SENSOR_TMF8829.md §2).

Same command numbering as the VL53 bridges (0x32/0x33/0x35/0x36/0x37) but
TMF8829 payloads: an 8-bit register address and self-sized frames, so the
host never tells the bridge a frame size. The MCU is a thin bridge; the host
runs the sensor (`ai.depz.sensor.sensors.tmf8829`). Mirrors
`depz_sensor_sdk/protocol/tmf8829.py` 1:1.

#### Tmf8829.READ_MAX *(field)*

```java
public static final int READ_MAX = 1024
```

TMF_READ_REG `len` 1..1024.

#### Tmf8829.WRITE_MAX *(field)*

```java
public static final int WRITE_MAX = 1023
```

TMF_WRITE_REG N 1..1023 (1024 B buffer minus the address).

#### Tmf8829.CHUNK_MAX *(field)*

```java
public static final int CHUNK_MAX = 1024
```

Frame bytes per RPT_TMF_FRAME.

#### Tmf8829.INT_RESULTS *(field)*

```java
public static final int INT_RESULTS = 0x01
```

#### Tmf8829.INT_MOTION *(field)*

```java
public static final int INT_MOTION = 0x02
```

#### Tmf8829.INT_PROXIMITY *(field)*

```java
public static final int INT_PROXIMITY = 0x04
```

#### Tmf8829.INT_HISTOGRAMS *(field)*

```java
public static final int INT_HISTOGRAMS = 0x08
```

#### Tmf8829.INT_MASK_ALL *(field)*

```java
public static final int INT_MASK_ALL = 0x0F
```

#### Tmf8829.SF_REF_FRAME *(field)*

```java
public static final int SF_REF_FRAME = 0x01
```

TMF_START_STREAM flags: emit the ref-SPAD frame before each results frame.

#### Tmf8829.REG_DATA_HEADER_SIZE *(field)*

```java
public static final int REG_DATA_HEADER_SIZE = 9
```

#### Tmf8829.FRAME_CHUNK_HEADER_SIZE *(field)*

```java
public static final int FRAME_CHUNK_HEADER_SIZE = 21
```

#### Tmf8829.EVENT_SIZE *(field)*

```java
public static final int EVENT_SIZE = 12
```

#### Tmf8829.INFO_SIZE *(field)*

```java
public static final int INFO_SIZE = 25
```

#### Tmf8829.packReadReg

```java
public static byte[] packReadReg(int addr, int len)
```

TMF_READ_REG payload: `addr u8, len u16 LE`; `len` 1..`READ_MAX`.

#### Tmf8829.packWriteReg

```java
public static byte[] packWriteReg(int addr, byte[] data)
```

TMF_WRITE_REG payload: `addr u8, data[1..1023]`.

#### Tmf8829.packStartStream

```java
public static byte[] packStartStream(int intMask, int flags)
```

TMF_START_STREAM payload: `int_mask u8, flags u8`. `intMask` a
non-zero subset of 0x0F; `flags` only `SF_REF_FRAME`.

### Tmf8829.Tmf8829Cmd

```java
public enum Tmf8829Cmd
```

#### Tmf8829.Tmf8829Cmd — constants

`READ_REG`(0x32), `WRITE_REG`(0x33), `START_STREAM`(0x35), `STOP_STREAM`(0x36), `GET_INFO`(0x37)

#### Tmf8829.Tmf8829Cmd.value *(field)*

```java
public final int value
```

### Tmf8829.Tmf8829Rpt

```java
public enum Tmf8829Rpt
```

#### Tmf8829.Tmf8829Rpt — constants

`REG_DATA`(0x91), `INFO`(0x92), `FRAME`(0x93), `EVENT`(0x94)

#### Tmf8829.Tmf8829Rpt.value *(field)*

```java
public final int value
```

### Tmf8829.EventCode

```java
public enum EventCode
```

RPT_TMF_EVENT `code`.

#### Tmf8829.EventCode — constants

`INT`(0), `BAD_HEADER`(1)

#### Tmf8829.EventCode.value *(field)*

```java
public final int value
```

### Tmf8829.RegData

```java
public record RegData(long timestampUs, byte[] data)
```

RPT_TMF_REG_DATA: register bytes and the MCU time the SPI read completed.

#### Tmf8829.RegData.unpack

```java
public static RegData unpack(byte[] p)
```

### Tmf8829.Tmf8829Info

```java
public record Tmf8829Info(long intEdges, long frames, long framesAborted, long frameErrors, long spiTimeouts, int intLevel, boolean streaming, int intMask, int lastIntStatus, int lastFifoStatus)
```

RPT_TMF_INFO (25 B `<IIIIIBBBBB`): bridge state only, the sensor is
never touched. Carries no echoed command byte. `intEdges` runs from
MCU power-up; the frame counters restart at TMF_START_STREAM.
`intLevel` is the PA4 level now, 0 = asserted.

#### Tmf8829.Tmf8829Info.unpack

```java
public static Tmf8829Info unpack(byte[] p)
```

### Tmf8829.FrameChunk

```java
public record FrameChunk(long timestampUs, int fullSize, int offset, int frameSeq, long edgeUs, byte[] data)
```

One RPT_TMF_FRAME chunk (`<QHHBQ` + data). `timestampUs` plus
the frame's pre-header SYSTICK are a clock pair (not the capture time);
`edgeUs` is the INT falling edge credited to the frame, 0 = none.

#### Tmf8829.FrameChunk.unpack

```java
public static FrameChunk unpack(byte[] p)
```

### Tmf8829.Tmf8829Event

```java
public record Tmf8829Event(long timestampUs, int intStatus, int code, int frameId, int fifoStatus)
```

RPT_TMF_EVENT (`<QBBBB`). `intStatus` is INT_STATUS as read
(unmasked); `frameId` / `fifoStatus` are the header id byte and
the pre-header byte for BAD_HEADER, else 0.

#### Tmf8829.Tmf8829Event.unpack

```java
public static Tmf8829Event unpack(byte[] p)
```

### Tmf8829.RawFrame

```java
public record RawFrame(long timestampUs, long edgeUs, int frameSeq, byte[] data)
```

A reassembled frame: the sensor bytes from register 0xFA on.

### Tmf8829.FrameReassembler

```java
public static final class FrameReassembler
```

Rebuilds frames from RPT_TMF_FRAME chunks (contract 14 §2): reset on
offset 0; append only a chunk of the same frame_seq at the expected
offset; complete at full_size. Never stitches chunks of different
frame_seq. `seqGaps` counts frames the bridge dropped (frame_seq jumps).

#### Tmf8829.FrameReassembler.completed *(field)*

```java
public int completed
```

#### Tmf8829.FrameReassembler.discarded *(field)*

```java
public int discarded
```

#### Tmf8829.FrameReassembler.seqGaps *(field)*

```java
public int seqGaps
```

#### Tmf8829.FrameReassembler.reset

```java
public void reset()
```

Forget everything — call at every stream start (frame_seq restarts at 0).

#### Tmf8829.FrameReassembler.feed

```java
public RawFrame feed(FrameChunk chunk)
```

One chunk → the frame it completes, or `null`.

### CaptureTimer

```java
public final class CaptureTimer
```

When a results frame was captured (contracts/14_SENSOR_TMF8829.md §5). A
results frame is N equal sub-integrations back to back; the footer has the
SYSTICK of the start of the first (t0) and of the last (t1). The window is
`[t0, t1 + T1]`, T1 the stream's shortest `(t1 − t0)/(N − 1)`;
its centre is the frame time. Call `reset()` at every stream start.

#### CaptureTimer.clock *(field)*

```java
public final SensorClock clock
```

#### CaptureTimer.reset

```java
public void reset()
```

#### CaptureTimer.histograms

```java
public void histograms()
```

A histogram frame was seen in the stream.

#### CaptureTimer.results

```java
public FrameTime results(int fpMode, long systick, long t0Tick, long t1Tick, long pairUs, long edgeUs, int flags)
```

#### CaptureTimer.resultsFrame

```java
public FrameTime resultsFrame(Frames.FrameInfo f, long pairUs, long edgeUs, int flags)
```

### CaptureTimer.FrameTime

```java
public record FrameTime(long timestampUs, int flags, long t1LenUs, long t0Us, long t1Us, long edgeUs, long pairUs, long ratioQ16)
```

Capture time of one results frame, MCU µs: `timestampUs` the centre
of the capture window, `flags` FT_*, `t1LenUs` the
sub-integration length used, `t0Us`/`t1Us` the starts of the
first/last sub-integration, `edgeUs` the INT edge credited (0 =
none), `pairUs` the MCU time of the header read, `ratioQ16`
the clock ratio after this frame.

### Frames

```java
public final class Frames
```

TMF8829 frame layout and pixel decoding (contracts/14_SENSOR_TMF8829.md §3).

A frame is the sensor bytes from register 0xFA on: 5-byte pre-header
(FIFOSTATUS, SYSTICK), 16-byte header, data (pixel results or histograms),
12-byte footer. Mirrors `depz_sensor_sdk/tmf8829/frames.py`.

#### Frames.PRE_HEADER_SIZE *(field)*

```java
public static final int PRE_HEADER_SIZE = 5
```

#### Frames.HEADER_SIZE *(field)*

```java
public static final int HEADER_SIZE = 16
```

#### Frames.FOOTER_SIZE *(field)*

```java
public static final int FOOTER_SIZE = 12
```

#### Frames.FRAME_HEAD *(field)*

```java
public static final int FRAME_HEAD
```

21 B the bridge reads @0xFA.

#### Frames.FRAME_EOF *(field)*

```java
public static final int FRAME_EOF = 0xE0F7
```

#### Frames.FID_MASK *(field)*

```java
public static final int FID_MASK = 0xF0
```

#### Frames.FID_RESULTS *(field)*

```java
public static final int FID_RESULTS = 0x10
```

#### Frames.FID_HISTOGRAMS *(field)*

```java
public static final int FID_HISTOGRAMS = 0x20
```

#### Frames.FID_REF_SPAD *(field)*

```java
public static final int FID_REF_SPAD = 0x30
```

#### Frames.LAYOUT_PEAKS *(field)*

```java
public static final int LAYOUT_PEAKS = 0x07
```

#### Frames.LAYOUT_SIGNAL *(field)*

```java
public static final int LAYOUT_SIGNAL = 0x08
```

#### Frames.LAYOUT_NOISE *(field)*

```java
public static final int LAYOUT_NOISE = 0x10
```

#### Frames.LAYOUT_XTALK *(field)*

```java
public static final int LAYOUT_XTALK = 0x20
```

#### Frames.LAYOUT_SUBFRAME *(field)*

```java
public static final int LAYOUT_SUBFRAME = 0x40
```

32x32/48x32: set on the second (odd-row) half-frame.

#### Frames.FRAME_VALID *(field)*

```java
public static final int FRAME_VALID = 0x01
```

#### Frames.FRAME_ABORTED *(field)*

```java
public static final int FRAME_ABORTED = 0xC0
```

#### Frames.FRAME_WARNING_BITS *(field)*

```java
public static final int[] FRAME_WARNING_BITS
```

frame_status warning bits, in report order.

#### Frames.FRAME_WARNING_NAMES *(field)*

```java
public static final String[] FRAME_WARNING_NAMES
```

#### Frames.FOOTER_DIST_IN_MM *(field)*

```java
public static final int FOOTER_DIST_IN_MM = 0x01
```

footer.reserved b0: distances in mm, else 0.25 mm.

#### Frames.FIFO_TX_UNDERRUN *(field)*

```java
public static final int FIFO_TX_UNDERRUN = 0x10
```

#### Frames.FIFO_RX_OVERRUN *(field)*

```java
public static final int FIFO_RX_OVERRUN = 0x20
```

#### Frames.FP_GRID *(field)*

```java
public static final int[][] FP_GRID
```

fp_mode → {rows, cols} of the full focal plane (index = fp_mode 0..5).

#### Frames.FP_NAMES *(field)*

```java
public static final String[] FP_NAMES
```

#### Frames.knownFpMode

```java
public static boolean knownFpMode(int fpMode)
```

True when `fpMode` is one of the six known focal-plane modes.

#### Frames.frameSize

```java
public static int frameSize(byte[] head)
```

Full frame size (pre-header included) from its first 21 bytes.

#### Frames.parseFrame

```java
public static FrameInfo parseFrame(byte[] raw)
```

#### Frames.pixelSize

```java
public static int pixelSize(int layout)
```

Size in bytes of one results pixel for this layout.

#### Frames.frameRows

```java
public static int[] frameRows(int fpMode)
```

{rows, cols} one results frame carries: half the rows for 32x32/48x32.

#### Frames.parseResults

```java
public static PixelResults parseResults(byte[] raw)
```

#### Frames.parseResults

```java
public static PixelResults parseResults(byte[] raw, FrameInfo f)
```

Pixel results of one results frame, row-major.

#### Frames.mergeHalves

```java
public static PixelResults mergeHalves(PixelResults first, PixelResults second)
```

32x32/48x32: the first half-frame holds the even rows, the second the odd ones.

### Frames.FrameError

```java
public static final class FrameError extends IllegalArgumentException
```

A frame whose bytes do not match its own header.

### Frames.FrameInfo

```java
public record FrameInfo(int fifoStatus, long systick, int id, int layout, int payload, long fNumber, int[] temperature, int bdv, int[] refPos, long t0Integration, long t1Integration, int frameStatus, int reserved, int eof, int size)
```

Pre-header, header and footer of one frame (any kind). `temperature`
is three signed bytes, `refPos` two u16; u32 fields are `long`.

#### Frames.FrameInfo.kind

```java
public int kind()
```

FID_RESULTS, FID_HISTOGRAMS or FID_REF_SPAD.

#### Frames.FrameInfo.fpMode

```java
public int fpMode()
```

#### Frames.FrameInfo.isSecondHalf

```java
public boolean isSecondHalf()
```

#### Frames.FrameInfo.distanceInMm

```java
public boolean distanceInMm()
```

#### Frames.FrameInfo.problems

```java
public List<String> problems()
```

What is wrong with the frame, human-readable; empty when fine.

### Frames.PixelResults

```java
public record PixelResults(int peaks, int rows, int cols, int[] distance, int[] snr, int[] signal, int[] noise, int[] xtalk)
```

Decoded results of one frame or of a merged measurement, flat row-major
arrays. `distance`, `snr`, `signal` are
`[peaks][rows][cols]` (index `(p * rows + r) * cols + c`);
`noise`, `xtalk` `[rows][cols]`. Raw sensor integers:
distance in 0.25 mm (or mm with the footer flag), not yet clock-corrected.
Absent fields are `null`.

#### Frames.PixelResults.shape

```java
public int[] shape()
```

{rows, cols} of the arrays present; {0, 0} when the format carries none (as Python).

### Frames.HistogramFrame

```java
public record HistogramFrame(FrameInfo info, byte[] raw)
```

A raw histogram frame (25 377 B); decoding the bins is left to the caller
(ams histogram layout).

#### Frames.HistogramFrame.data

```java
public byte[] data()
```

### MeasurementBuilder

```java
public final class MeasurementBuilder
```

Frames in stream order → measurements (contracts/14_SENSOR_TMF8829.md §6).

Histogram frames between two results frames belong to the second; a
ref-SPAD frame comes right before its results frame (each half-frame has its
own). 32x32/48x32: the first half (layout 0x40 clear) holds the even rows; a
second half without a first is dropped, two first halves in a row keep the
newer one (both logged as problems). Call `reset()` at the start of
every stream: the clock model and T1 are per stream. Mirrors
`depz_sensor_sdk/tmf8829/builder.py`.

#### MeasurementBuilder.distanceMm

```java
public static double[] distanceMm(PixelResults results, double ratio, boolean inMm)
```

Raw distances → mm, clock-corrected (×ratio; ÷4 for 0.25 mm units).

#### MeasurementBuilder.clock *(field)*

```java
public final SensorClock clock
```

#### MeasurementBuilder.timer *(field)*

```java
public final CaptureTimer timer
```

#### MeasurementBuilder.parseErrors *(field)*

```java
public int parseErrors
```

Frames that failed to parse since the last `reset()`.

#### MeasurementBuilder.reset

```java
public void reset()
```

#### MeasurementBuilder.add

```java
public Measurement add(byte[] raw, long timestampUs, long edgeUs)
```

One frame with the MCU time of its header read and its INT edge (0 =
none) → the measurement it completes, or `null`.

### MeasurementBuilder.Measurement

```java
public record Measurement(long timestampUs, int flags, int fpMode, long fNumber, PixelResults results, double[] distanceMm, List<FrameInfo> frames, List<FrameTime> times, List<HistogramFrame> histograms, List<byte[]> refFrames, List<String> problems)
```

One measurement: its results frame (8x8, 16x16) or both half-frames
(32x32, 48x32) merged, with the histogram / ref-SPAD frames before it.
`timestampUs` is the capture time, MCU µs (floor mean of the
half-frame captures); `flags` the OR of their FT_*.
`distanceMm` is `[peaks][rows][cols]` flat, clock-corrected,
in mm (`null` when the format has no peaks); `results` keeps
the raw sensor integers. `refFrames` are 65 B each (zero
pre-header). `problems` lists what went wrong since the previous
measurement.

#### MeasurementBuilder.Measurement.shape

```java
public int[] shape()
```

{rows, cols} of the full focal plane.

#### MeasurementBuilder.Measurement.peaks

```java
public int peaks()
```

#### MeasurementBuilder.Measurement.temperatureC

```java
public int temperatureC()
```

Sensor die temperature (header byte 0), °C.

### SensorClock

```java
public final class SensorClock
```

Sensor SYSTICK (125 kHz) → MCU µs (contracts/14_SENSOR_TMF8829.md §5), fed
one clock pair per frame (the MCU time of the header read and the
pre-header SYSTICK sampled by it). The ratio is taken over a sliding 1..2 s
window, first after 200 ms; it is also the distance correction (ams
`tmf8829ClockCorrection`).

Integer arithmetic exactly as the contract; `vectors/tmf8829_frame_time.json`
pins it bit for bit. µs values are 64-bit `long`; SYSTICK values are u32
held in `long` (0..2^32−1). Mirrors `depz_sensor_sdk/tmf8829/clock.py`.

#### SensorClock.NOMINAL_Q16 *(field)*

```java
public static final long NOMINAL_Q16
```

SYSTICK 125 kHz = 8 MCU µs per tick, Q16.

#### SensorClock.CLOCK_WINDOW_MIN_US *(field)*

```java
public static final long CLOCK_WINDOW_MIN_US = 200_000L
```

First ratio estimate.

#### SensorClock.CLOCK_WINDOW_SLIDE_US *(field)*

```java
public static final long CLOCK_WINDOW_SLIDE_US = 1_000_000L
```

Then a 1..2 s window.

#### SensorClock.EDGE_LATE_US *(field)*

```java
public static final long EDGE_LATE_US = 2000L
```

#### SensorClock.FT_NOMINAL *(field)*

```java
public static final int FT_NOMINAL = 0x01
```

Clock ratio not measured yet (nominal 8 µs/tick used).

#### SensorClock.FT_CLOCK_RESET *(field)*

```java
public static final int FT_CLOCK_RESET = 0x02
```

Clock model restarted at this frame (SYSTICK jump).

#### SensorClock.FT_EDGE_LATE *(field)*

```java
public static final int FT_EDGE_LATE = 0x04
```

INT edge > 2 ms later after t1 than usual in this stream.

#### SensorClock.FT_STALLED *(field)*

```java
public static final int FT_STALLED = 0x08
```

Sub-integrations not back to back (histogram FIFO wait).

#### SensorClock.FT_T1_UNSURE *(field)*

```java
public static final int FT_T1_UNSURE = 0x10
```

Histograms + 32x32/48x32: T1 may be too long, centre late.

#### SensorClock.FP_SUBINTS *(field)*

```java
public static final int[] FP_SUBINTS
```

Sub-integrations per results frame, index = fp_mode 0..5.

#### SensorClock.flagNames

```java
public static List<String> flagNames(int flags)
```

Names of the FT_* bits set in `flags`.

#### SensorClock.ratioQ16 *(field)*

```java
public long ratioQ16
```

Current ratio, Q16 (u32); `NOMINAL_Q16` until measured.

#### SensorClock.nominal *(field)*

```java
public boolean nominal
```

True while the ratio is still the nominal one.

#### SensorClock.reset

```java
public void reset()
```

#### SensorClock.ratio

```java
public double ratio()
```

MCU/sensor clock ratio (≈ 1.015): distances are scaled by it.

#### SensorClock.add

```java
public int add(long us, long tick)
```

Add a pair → `FT_CLOCK_RESET` if it does not fit the model (which
then restarts from it), else 0. `us` is a u64 in Java's
two's-complement convention; `tick` a u32 SYSTICK (outside
0..2^32-1 → IllegalArgumentException, contract 07 §3).

#### SensorClock.toUs

```java
public long toUs(long pairUs, long pairTick, long tick)
```

SYSTICK `tick` → MCU µs through the pair (`pairUs`, `pairTick`).

### Tmf8829Config

```java
public final class Tmf8829Config
```

TMF8829 presets and config-page fields (contracts/14_SENSOR_TMF8829.md §4).
Mirrors `depz_sensor_sdk/tmf8829/config.py`.

#### Tmf8829Config.REG_CFG *(field)*

```java
public static final int REG_CFG = 0x22
```

Config page 0x22..0xDF.

#### Tmf8829Config.CFG_SIZE *(field)*

```java
public static final int CFG_SIZE
```

190 B.

#### Tmf8829Config.DEFAULT_PERIOD_MS *(field)*

```java
public static final int DEFAULT_PERIOD_MS = 33
```

The sensor's own period after `init()`.

#### Tmf8829Config.PRESETS *(field)*

```java
public static final Map<String, Integer> PRESETS
```

Preset name → load command (each loads its config page).

#### Tmf8829Config.PRESET_KILO_ITERATIONS *(field)*

```java
public static final Map<String, Integer> PRESET_KILO_ITERATIONS
```

The preset's own kilo_iterations (a preset rescales a stale value, it does not reset it).

#### Tmf8829Config.CFG_FIELDS *(field)*

```java
public static final Map<String, CfgField> CFG_FIELDS
```

Field name → location, in contract order.

#### Tmf8829Config.cfgGet

```java
public static int cfgGet(byte[] page, String name)
```

Read a named field from a config page (190 B starting at 0x22).

#### Tmf8829Config.cfgSet

```java
public static void cfgSet(byte[] page, String name, int value)
```

Write a named field into a config page in place; out-of-range values are refused.

#### Tmf8829Config.configFields

```java
public static Map<String, Integer> configFields(byte[] page)
```

Every named field of a config page.

### Tmf8829Config.CfgField

```java
public record CfgField(int reg, int shift, int width)
```

A config-page field: register, bit shift, width in bits (16 = little-endian register pair).

### Tmf8829Firmware

```java
public final class Tmf8829Firmware
```

The embedded TMF8829 sensor firmware (contracts/14_SENSOR_TMF8829.md §4):
ams `tmf8829_application.hex`, application 2.200.0, MIT (see
`LICENSES-MIT.TXT` next to the image on the classpath). One contiguous
segment, 14 444 B (already a multiple of 4: 3611 words), downloaded by the
host through the sensor bootloader to `LOAD_ADDR`.

#### Tmf8829Firmware.LOAD_ADDR *(field)*

```java
public static final long LOAD_ADDR = 0x00010000L
```

Load address of the image in sensor RAM.

#### Tmf8829Firmware.RESOURCE *(field)*

```java
public static final String RESOURCE
```

Classpath resource of the image.

#### Tmf8829Firmware.LICENSE_RESOURCE *(field)*

```java
public static final String LICENSE_RESOURCE
```

Classpath resource of the image's MIT license.

#### Tmf8829Firmware.image

```java
public static synchronized byte[] image()
```

The sensor firmware image (14 444 B); a fresh copy on every call.

#### Tmf8829Firmware.license

```java
public static String license()
```

The MIT license text that ships with the image.

## BNO086 (IMU)

### Bno086Device

```java
public final class Bno086Device extends DepzDevice
```

BNO085 / BNO086 9-axis IMU (contract 05). The board only shuttles raw SHTP frames
(SEND_SHTP_PACKET / RPT_DATA); the SH-2 hub protocol runs here, ported from the reference SDK.
Every send is acknowledged by the bridge at once and every inbound frame arrives as
RPT_DATA(cmd 0x00), so replies are correlated at the SH-2 layer only.
<pre>`imu.hardwareReset();
try (Subscription<Reports.Report> reports = imu.reports()) {
imu.enable(Reports.ROTATION_VECTOR, 100);
Reports.Report r = reports.take(Duration.ofSeconds(1));
}`</pre>
Never call a blocking device method (`enable`, `tareNow`, …) from an
`onReport` callback.

#### Bno086Device.RATE_LOW_FACTOR *(field)*

```java
public static final double RATE_LOW_FACTOR
```

Granted rates outside [0.9, 2.1] × the requested one are worth a warning, never an error (contract 05 §7).

#### Bno086Device.RATE_HIGH_FACTOR *(field)*

```java
public static final double RATE_HIGH_FACTOR
```

#### Bno086Device.busyRetries

```java
public int busyRetries()
```

SEND_SHTP_PACKET attempts on ERR_BUSY before giving up.

#### Bno086Device.setBusyRetries

```java
public void setBusyRetries(int retries)
```

#### Bno086Device.busyBackoff

```java
public Duration busyBackoff()
```

The back-off between busy retries (at least 200 ms per the bridge spec).

#### Bno086Device.setBusyBackoff

```java
public void setBusyBackoff(Duration backoff)
```

#### Bno086Device.advertisement

```java
public byte[] advertisement()
```

The raw SHTP channel-0 advertisement seen since open or reset.

#### Bno086Device.hardwareReset

```java
public void hardwareReset(Duration timeout)
```

Hard-reset the sensor through nRST. The bridge's OK confirms it; the SH-2 executable
reset-complete is waited for best-effort (firmware ≤ 0.95 never sent it, ERRATA E9). SHTP
state and cached features restart from zero.

#### Bno086Device.hardwareReset

```java
public void hardwareReset()
```

#### Bno086Device.wake

```java
public void wake()
```

Pulse WAKE (PS0) — the board's pin, not the SH-2 "On" that ends `sleep()` (that is `powerOn()`).

#### Bno086Device.sleep

```java
public void sleep()
```

SH-2 "Sleep": every report stops except those enabled with
`Sh2.FEATURE_FLAG_WAKEUP | Sh2.FEATURE_FLAG_ALWAYS_ON`; the configuration is kept. No response.

#### Bno086Device.powerOn

```java
public void powerOn()
```

SH-2 "On": ends `sleep()`; every enabled report comes back. No response.

#### Bno086Device.productId

```java
public ProductId productId(Duration timeout)
```

The SH-2 application's Product ID — the first of `productIds`.

#### Bno086Device.productId

```java
public ProductId productId()
```

#### Bno086Device.productIds

```java
public List<ProductId> productIds(Duration timeout, Duration settle)
```

Every component's Product ID Response, the SH-2 application first. Collects until no response
came for `settle` (default 150 ms), so late ones cannot answer the next request.

#### Bno086Device.productIds

```java
public List<ProductId> productIds()
```

#### Bno086Device.enableInterval

```java
public FeatureResponse enableInterval(int sensor, long intervalUs, long batchUs, int sensitivity, int flags, long cfgWord, boolean verify, Duration timeout)
```

Enable `sensor` through Set Feature (0xFD) every `intervalUs`. The hub rounds to
its grid; with `verify` the granted rate is read back through Get Feature and a rate
outside 0.9–2.1 × the request is logged as a warning (contract 05 §7). Returns that read-back
(null without verify).

#### Bno086Device.enableInterval

```java
public FeatureResponse enableInterval(int sensor, long intervalUs)
```

#### Bno086Device.enable

```java
public FeatureResponse enable(int sensor, double hz, long batchUs, int sensitivity, int flags, long cfgWord, boolean verify, Duration timeout)
```

`enableInterval` at `hz`.

#### Bno086Device.enable

```java
public FeatureResponse enable(int sensor, double hz)
```

#### Bno086Device.disable

```java
public void disable(int sensor)
```

Disable `sensor` (Set Feature with interval 0).

#### Bno086Device.getFeature

```java
public FeatureResponse getFeature(int sensor, Duration timeout)
```

Get Feature round trip: the rates in effect for `sensor`.

#### Bno086Device.getFeature

```java
public FeatureResponse getFeature(int sensor)
```

#### Bno086Device.enableRotationVector

```java
public FeatureResponse enableRotationVector(double hz)
```

#### Bno086Device.enableGameRotationVector

```java
public FeatureResponse enableGameRotationVector(double hz)
```

#### Bno086Device.enableAccelerometer

```java
public FeatureResponse enableAccelerometer(double hz)
```

#### Bno086Device.enableGyroscope

```java
public FeatureResponse enableGyroscope(double hz)
```

#### Bno086Device.enableMagnetometer

```java
public FeatureResponse enableMagnetometer(double hz)
```

#### Bno086Device.enableLinearAcceleration

```java
public FeatureResponse enableLinearAcceleration(double hz)
```

#### Bno086Device.enableGravity

```java
public FeatureResponse enableGravity(double hz)
```

#### Bno086Device.enableGyroIntegratedRv

```java
public FeatureResponse enableGyroIntegratedRv(double hz)
```

#### Bno086Device.onReport

```java
public AutoCloseable onReport(Consumer<Reports.Report> callback, Collection<Integer> sensors)
```

Call `callback` for every report (of `sensors` only, when not null) on the reader
thread. Close the result to stop.

#### Bno086Device.onReport

```java
public AutoCloseable onReport(Consumer<Reports.Report> callback)
```

#### Bno086Device.reports

```java
public Subscription<Reports.Report> reports(Collection<Integer> sensors, int capacity)
```

A pull queue of reports (of `sensors` only, when not null), bounded, drop-oldest;
subscribed when this returns.

#### Bno086Device.reports

```java
public Subscription<Reports.Report> reports(int capacity)
```

#### Bno086Device.reports

```java
public Subscription<Reports.Report> reports()
```

#### Bno086Device.streamDroppedCounts

```java
public List<Long> streamDroppedCounts()
```

#### Bno086Device.tareNow

```java
public void tareNow(int axes, TareBasis basis)
```

Tare `axes` (a `Sh2Hub.TARE_*` bitmap) against `basis` (no response).

#### Bno086Device.tareNow

```java
public void tareNow()
```

All axes against the rotation vector.

#### Bno086Device.persistTare

```java
public void persistTare()
```

Persist the current tare into FRS (no response).

#### Bno086Device.setReorientation

```java
public void setReorientation(double x, double y, double z, double w)
```

The runtime reorientation quaternion (Q14 on the wire; all zeros clears; no response). It is
applied on the hub's side, in the hub's own axes: x and y swapped and z negated relative to the
reports.

#### Bno086Device.setCalibration

```java
public void setCalibration(boolean accel, boolean gyro, boolean mag, boolean planar, Duration timeout)
```

Configure the motion-engine calibration; `Sh2Exception` on a non-zero status.

#### Bno086Device.setCalibration

```java
public void setCalibration(boolean accel, boolean gyro, boolean mag)
```

#### Bno086Device.getCalibration

```java
public CalibrationConfig getCalibration(Duration timeout)
```

Which motion-engine calibrations run.

#### Bno086Device.getCalibration

```java
public CalibrationConfig getCalibration()
```

#### Bno086Device.saveDcd

```java
public void saveDcd(Duration timeout)
```

Save the dynamic calibration data to flash (DCD Save Now).

#### Bno086Device.saveDcd

```java
public void saveDcd()
```

#### Bno086Device.configurePeriodicDcd

```java
public void configurePeriodicDcd(boolean enable)
```

The hub's periodic DCD autosave (no response).

#### Bno086Device.setMotionIntent

```java
public void setMotionIntent(MotionIntent intent)
```

Interactive Calibration (0x0E, BNO086 only): how the device is about to move, so the hub can
refine the gyro zero-rate offset while it is still. No response; a BNO085 ignores it. The
hub's answer, if it wants the device to stop, is a MotionRequest report.

#### Bno086Device.enableMotionRequest

```java
public FeatureResponse enableMotionRequest(double hz)
```

Enable MotionRequest (0x2B) reports; the hub sends one when its request changes, and reads
the rate back late, so the granted rate is not checked.

#### Bno086Device.flush

```java
public void flush(int sensor, Duration timeout)
```

Force Sensor Flush (0xF0): the hub sends `sensor`'s batched reports now; returns once its
Flush Completed (0xEF) arrives.

#### Bno086Device.flush

```java
public void flush(int sensor)
```

#### Bno086Device.frsRead

```java
public long[] frsRead(int recordId, Duration timeout)
```

Read a whole FRS record: its 32-bit words.

#### Bno086Device.frsRead

```java
public long[] frsRead(int recordId)
```

#### Bno086Device.frsWrite

```java
public void frsWrite(int recordId, long[] words, Duration timeout)
```

Write a whole FRS record; `Sh2Exception` on failure.

#### Bno086Device.frsWrite

```java
public void frsWrite(int recordId, long[] words)
```

#### Bno086Device.getMetadata

```java
public SensorMetadata getMetadata(int sensor, Duration timeout)
```

Read and parse `sensor`'s metadata FRS record.

#### Bno086Device.getMetadata

```java
public SensorMetadata getMetadata(int sensor)
```

#### Bno086Device.getOscillatorType

```java
public int getOscillatorType(Duration timeout)
```

Get Oscillator Type (0x0A): R0, one of `Sh2Hub.OSC_*` (an unknown value comes back as is).

#### Bno086Device.getOscillatorType

```java
public int getOscillatorType()
```

#### Bno086Device.clearDcdAndReset

```java
public void clearDcdAndReset(Duration timeout)
```

Clear the in-RAM dynamic calibration and reset (0x0B). The hub resets instead of answering,
so this waits for the executable reset-complete.

#### Bno086Device.clearDcdAndReset

```java
public void clearDcdAndReset()
```

#### Bno086Device.getErrors

```java
public List<ErrorRecord> getErrors(int severity, Duration timeout)
```

The error queue (0x01), `severity` or greater; records stream until one with source 255 (no more).

#### Bno086Device.getErrors

```java
public List<ErrorRecord> getErrors()
```

#### Bno086Device.getCounts

```java
public Counts getCounts(int sensor, Duration timeout)
```

A sensor's event counts (0x02): two responses, response_seq 0 then 1.

#### Bno086Device.getCounts

```java
public Counts getCounts(int sensor)
```

#### Bno086Device.clearCounts

```java
public void clearCounts(int sensor, Duration timeout)
```

Clear a sensor's event counts (0x02, subcommand 1).

#### Bno086Device.clearCounts

```java
public void clearCounts(int sensor)
```

### Bno086

```java
public final class Bno086
```

BNO085 / BNO086 bridge wire codecs (contracts/05_SENSOR_BNO086.md §2): the MCU only shuttles
raw SHTP frames; the SH-2 hub protocol runs on the host
(`ai.depz.sensor.sensors.bno086.Sh2Hub`, `ai.depz.sensor.devices.Bno086Device`).

#### Bno086.BUSY_BACKOFF_MS *(field)*

```java
public static final int BUSY_BACKOFF_MS = 200
```

Two 64-byte SHTP transmit slots on the MCU (ERRATA E2); a third send gets ERR_BUSY, to back
off at least this long.

### Bno086.Bno086Cmd

```java
public enum Bno086Cmd
```

#### Bno086.Bno086Cmd — constants

`SENSOR_RESET`(0x32), `SENSOR_WAKE_UP`(0x33), `SEND_SHTP_PACKET`(0x34)

#### Bno086.Bno086Cmd.value *(field)*

```java
public final int value
```

### Bno086.Bno086Rpt

```java
public enum Bno086Rpt
```

#### Bno086.Bno086Rpt — constants

`DATA`(0x91)

#### Bno086.Bno086Rpt.value *(field)*

```java
public final int value
```

### Bno086.Data

```java
public record Data(int cmd, long timestampUs, byte[] shtp)
```

One RPT_DATA report: an SHTP frame captured from the sensor bus, with the MCU uptime.

#### Bno086.Data.unpack

```java
public static Data unpack(byte[] p)
```

### Reports

```java
public final class Reports
```

SH-2 input-report catalog and parsers (contracts/05_SENSOR_BNO086.md §5).

Raw wire integers are authoritative (this decode layer surfaces them as
`*_raw` fields); `scaled` derives the physical values with the
Q points of `Q_POINTS`. Timestamps:
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

#### Reports.MOTION_REQUEST *(field)*

```java
public static final int MOTION_REQUEST = 0x2b
```

Interactive calibration, BNO086 only.

#### Reports.BASE_TIMESTAMP_REF *(field)*

```java
public static final int BASE_TIMESTAMP_REF = 0xfb
```

#### Reports.TIMESTAMP_REBASE *(field)*

```java
public static final int TIMESTAMP_REBASE = 0xfa
```

#### Reports.FLUSH_COMPLETED *(field)*

```java
public static final int FLUSH_COMPLETED = 0xef
```

+ u8 sensor ID, no SH-2 header; the answer to Force Sensor Flush (0xF0).

#### Reports.Q_POINTS *(field)*

```java
public static final Map<Integer, Integer> Q_POINTS
```

The Q points of the primary fields by sensor id (value = raw / 2^Q), SH-2 reference driver.

#### Reports.RV_ACCURACY_Q *(field)*

```java
public static final int RV_ACCURACY_Q = 12
```

The rotation-vector accuracy estimate, radians.

#### Reports.GYRO_RV_ANGVEL_Q *(field)*

```java
public static final int GYRO_RV_ANGVEL_Q = 10
```

The gyro-integrated rotation vector's angular velocity, rad/s.

#### Reports.scale

```java
public static double scale(long raw, int q)
```

raw / 2^q (exact in a double).

#### Reports.scaled

```java
public static Map<String, Object> scaled(Report r)
```

The physical values of a report, under the reference SDK's property names: `x/y/z`
(m/s², rad/s, µT), `i/j/k/real` (unit quaternion), `accuracy_rad` (null for the game
variants), `bias` and `angular_velocity` (3 doubles), `value` (hPa, lux, %, cm, °C)
and `double_tap` — only those the report's type has.

#### Reports.parseInputCargo

```java
public static List<Report> parseInputCargo(byte[] payload, long captureTimestampUs)
```

Parse a channel-3/4 cargo into typed reports. `captureTimestampUs`
is the bridge RPT_DATA capture time (MCU uptime, a u64: a negative one
is an IllegalArgumentException).

#### Reports.parseGyroRvCargo

```java
public static Report parseGyroRvCargo(byte[] payload, long captureTimestampUs)
```

Parse a channel-5 cargo (gyro-integrated RV, dense format). Two shapes:
7×i16 bare, or prefixed with 0xFB + i32 base delta + u16 delay. Returns
`null` on a too-short buffer; a negative `captureTimestampUs`
is an IllegalArgumentException.

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
codecs; mirror of the TS/Python `sh2` references. The response
decoders and the hub session helpers are `Sh2Hub`.

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

#### Sh2.FEATURE_FLAG_WAKEUP *(field)*

```java
public static final int FEATURE_FLAG_WAKEUP = 0x04
```

Set Feature `flags` bit 2 (SH-2 §6.5.4): wake-up enabled — the
sensor reports on SHTP channel 4 (wake input).

#### Sh2.FEATURE_FLAG_ALWAYS_ON *(field)*

```java
public static final int FEATURE_FLAG_ALWAYS_ON = 0x08
```

Set Feature `flags` bit 3 (SH-2 §6.5.4): always-on — the sensor
keeps running while the hub sleeps. With `FEATURE_FLAG_WAKEUP` a
report kept its full rate through `buildSleep()` on a DEPZ BNO086
(SH-2 3.12.6, 2026-10-16), on channel 4, while every other report stopped.

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

#### Sh2.buildSleep

```java
public static byte[] buildSleep()
```

SH-2 "Sleep" for the executable channel (channel 1, not control):
`[0x03]`. Frame it with
`nextFrame(Shtp.CHANNEL_EXECUTABLE, Sh2.buildSleep())` and send it
through the bridge's SEND_SHTP_PACKET (0x34); the hub sends no response.
Every report stops, except those enabled with
`FEATURE_FLAG_WAKEUP` | `FEATURE_FLAG_ALWAYS_ON`, which keep
their rate; the configuration (enabled sensors, rates) is kept.
`buildPowerOn()` ends it. On a DEPZ BNO086 (SH-2 3.12.6,
2026-10-16) all reports stopped within one sample; a WAKEUP|ALWAYS_ON
report kept streaming at full rate on channel 4.

#### Sh2.buildPowerOn

```java
public static byte[] buildPowerOn()
```

SH-2 "On" for the executable channel (channel 1): `[0x02]` —
ends `buildSleep()`'s sleep. Every report that was enabled comes
back at its rate, without re-enabling; the hub sends no response. Not the
bridge's SENSOR_WAKE_UP (0x33), which pulses the WAKE/PS0 pin — a hardware
signal, not an SH-2 command.

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

### Sh2Hub

```java
public final class Sh2Hub
```

SH-2 hub protocol (contract 05 §6): the control-response decoders, the request parameters, the
FRS sessions and the control-cargo walk the device layer needs on top of `Sh2`. Ported from
the reference `sh2.py`.

#### Sh2Hub.FORCE_SENSOR_FLUSH *(field)*

```java
public static final int FORCE_SENSOR_FLUSH = 0xF0
```

Host → hub on the control channel; answered by Flush Completed (0xEF) on the input channel.

#### Sh2Hub.CHANNEL_COMMAND *(field)*

```java
public static final int CHANNEL_COMMAND = 0
```

#### Sh2Hub.CHANNEL_CONTROL *(field)*

```java
public static final int CHANNEL_CONTROL = 2
```

#### Sh2Hub.CHANNEL_INPUT_NORMAL *(field)*

```java
public static final int CHANNEL_INPUT_NORMAL = 3
```

#### Sh2Hub.CHANNEL_INPUT_WAKE *(field)*

```java
public static final int CHANNEL_INPUT_WAKE = 4
```

#### Sh2Hub.CHANNEL_GYRO_RV *(field)*

```java
public static final int CHANNEL_GYRO_RV = 5
```

#### Sh2Hub.EXECUTABLE_RESET_COMPLETE *(field)*

```java
public static final int EXECUTABLE_RESET_COMPLETE = 0x01
```

Inbound on the executable channel: the hub finished a reset.

#### Sh2Hub.CMD_ERRORS *(field)*

```java
public static final int CMD_ERRORS = 0x01
```

#### Sh2Hub.CMD_COUNTER *(field)*

```java
public static final int CMD_COUNTER = 0x02
```

#### Sh2Hub.CMD_TARE *(field)*

```java
public static final int CMD_TARE = 0x03
```

#### Sh2Hub.CMD_INITIALIZE *(field)*

```java
public static final int CMD_INITIALIZE = 0x04
```

#### Sh2Hub.CMD_SAVE_DCD *(field)*

```java
public static final int CMD_SAVE_DCD = 0x06
```

#### Sh2Hub.CMD_ME_CALIBRATE *(field)*

```java
public static final int CMD_ME_CALIBRATE = 0x07
```

#### Sh2Hub.CMD_PERIODIC_DCD_CONFIG *(field)*

```java
public static final int CMD_PERIODIC_DCD_CONFIG = 0x09
```

#### Sh2Hub.CMD_GET_OSCILLATOR_TYPE *(field)*

```java
public static final int CMD_GET_OSCILLATOR_TYPE = 0x0A
```

#### Sh2Hub.CMD_CLEAR_DCD_AND_RESET *(field)*

```java
public static final int CMD_CLEAR_DCD_AND_RESET = 0x0B
```

#### Sh2Hub.CMD_INTERACTIVE_CALIBRATION *(field)*

```java
public static final int CMD_INTERACTIVE_CALIBRATION = 0x0E
```

BNO086 only; no response.

#### Sh2Hub.OSC_INTERNAL *(field)*

```java
public static final int OSC_INTERNAL = 0
```

#### Sh2Hub.OSC_EXT_CRYSTAL *(field)*

```java
public static final int OSC_EXT_CRYSTAL = 1
```

#### Sh2Hub.OSC_EXT_CLOCK *(field)*

```java
public static final int OSC_EXT_CLOCK = 2
```

#### Sh2Hub.ERROR_SOURCE_MOTION_ENGINE *(field)*

```java
public static final int ERROR_SOURCE_MOTION_ENGINE = 1
```

#### Sh2Hub.ERROR_SOURCE_MOTION_HUB *(field)*

```java
public static final int ERROR_SOURCE_MOTION_HUB = 2
```

#### Sh2Hub.ERROR_SOURCE_SENSOR_HUB *(field)*

```java
public static final int ERROR_SOURCE_SENSOR_HUB = 3
```

#### Sh2Hub.ERROR_SOURCE_CHIP *(field)*

```java
public static final int ERROR_SOURCE_CHIP = 4
```

#### Sh2Hub.ERROR_SOURCE_NO_MORE_ERRORS *(field)*

```java
public static final int ERROR_SOURCE_NO_MORE_ERRORS = 255
```

The end of the error queue.

#### Sh2Hub.TARE_X *(field)*

```java
public static final int TARE_X = 1
```

#### Sh2Hub.TARE_Y *(field)*

```java
public static final int TARE_Y = 2
```

#### Sh2Hub.TARE_Z *(field)*

```java
public static final int TARE_Z = 4
```

#### Sh2Hub.TARE_ALL *(field)*

```java
public static final int TARE_ALL = 7
```

#### Sh2Hub.FRS_STATIC_CALIBRATION_AGM *(field)*

```java
public static final int FRS_STATIC_CALIBRATION_AGM = 0x7979
```

FRS record ids (SH-2 figure 28).

#### Sh2Hub.FRS_NOMINAL_CALIBRATION *(field)*

```java
public static final int FRS_NOMINAL_CALIBRATION = 0x4D4D
```

#### Sh2Hub.FRS_DYNAMIC_CALIBRATION *(field)*

```java
public static final int FRS_DYNAMIC_CALIBRATION = 0x1F1F
```

#### Sh2Hub.FRS_ME_POWER_MGMT *(field)*

```java
public static final int FRS_ME_POWER_MGMT = 0xD3E2
```

#### Sh2Hub.FRS_SYSTEM_ORIENTATION *(field)*

```java
public static final int FRS_SYSTEM_ORIENTATION = 0x2D3E
```

Mounting quaternion, 4 × Q30 words.

#### Sh2Hub.FRS_ACCEL_ORIENTATION *(field)*

```java
public static final int FRS_ACCEL_ORIENTATION = 0x2D41
```

#### Sh2Hub.FRS_GYROSCOPE_ORIENTATION *(field)*

```java
public static final int FRS_GYROSCOPE_ORIENTATION = 0x2D46
```

#### Sh2Hub.FRS_MAGNETOMETER_ORIENTATION *(field)*

```java
public static final int FRS_MAGNETOMETER_ORIENTATION = 0x2D4C
```

#### Sh2Hub.FRS_ARVR_STABILIZATION_RV *(field)*

```java
public static final int FRS_ARVR_STABILIZATION_RV = 0x3E2D
```

#### Sh2Hub.FRS_ARVR_STABILIZATION_GRV *(field)*

```java
public static final int FRS_ARVR_STABILIZATION_GRV = 0x3E2E
```

#### Sh2Hub.FRS_SERIAL_NUMBER *(field)*

```java
public static final int FRS_SERIAL_NUMBER = 0x4B4B
```

One 32-bit word; empty unless programmed.

#### Sh2Hub.FRS_USER_RECORD *(field)*

```java
public static final int FRS_USER_RECORD = 0x74B4
```

Free for the user, up to 64 words.

#### Sh2Hub.FRS_SIG_MOTION_DETECT_CONFIG *(field)*

```java
public static final int FRS_SIG_MOTION_DETECT_CONFIG = 0xC274
```

#### Sh2Hub.FRS_SHAKE_DETECT_CONFIG *(field)*

```java
public static final int FRS_SHAKE_DETECT_CONFIG = 0x7D7D
```

#### Sh2Hub.FRS_STABILITY_DETECTOR_CONFIG *(field)*

```java
public static final int FRS_STABILITY_DETECTOR_CONFIG = 0xED85
```

#### Sh2Hub.FRS_ACTIVITY_TRACKER_CONFIG *(field)*

```java
public static final int FRS_ACTIVITY_TRACKER_CONFIG = 0xED88
```

#### Sh2Hub.FRS_GYRO_INTEGRATED_RV_CONFIG *(field)*

```java
public static final int FRS_GYRO_INTEGRATED_RV_CONFIG = 0xA1A2
```

Reference, sync interval, max error, prediction (SH-2 §4.3.24).

#### Sh2Hub.METADATA_RECORDS *(field)*

```java
public static final Map<Integer, Integer> METADATA_RECORDS
```

Sensor id → its metadata FRS record.

#### Sh2Hub.FRS_NO_ERROR *(field)*

```java
public static final int FRS_NO_ERROR = 0
```

#### Sh2Hub.FRS_UNRECOGNIZED_FRS_TYPE *(field)*

```java
public static final int FRS_UNRECOGNIZED_FRS_TYPE = 1
```

#### Sh2Hub.FRS_BUSY *(field)*

```java
public static final int FRS_BUSY = 2
```

#### Sh2Hub.FRS_READ_COMPLETED *(field)*

```java
public static final int FRS_READ_COMPLETED = 3
```

#### Sh2Hub.FRS_OFFSET_OUT_OF_RANGE *(field)*

```java
public static final int FRS_OFFSET_OUT_OF_RANGE = 4
```

#### Sh2Hub.FRS_RECORD_EMPTY *(field)*

```java
public static final int FRS_RECORD_EMPTY = 5
```

#### Sh2Hub.FRS_BLOCK_COMPLETED *(field)*

```java
public static final int FRS_BLOCK_COMPLETED = 6
```

#### Sh2Hub.FRS_BLOCK_AND_READ_COMPLETED *(field)*

```java
public static final int FRS_BLOCK_AND_READ_COMPLETED = 7
```

#### Sh2Hub.FRS_DEVICE_ERROR *(field)*

```java
public static final int FRS_DEVICE_ERROR = 8
```

#### Sh2Hub.FRS_WRITE_WORDS_RECEIVED *(field)*

```java
public static final int FRS_WRITE_WORDS_RECEIVED = 0
```

#### Sh2Hub.FRS_WRITE_UNRECOGNIZED_FRS_TYPE *(field)*

```java
public static final int FRS_WRITE_UNRECOGNIZED_FRS_TYPE = 1
```

#### Sh2Hub.FRS_WRITE_BUSY *(field)*

```java
public static final int FRS_WRITE_BUSY = 2
```

#### Sh2Hub.FRS_WRITE_COMPLETED *(field)*

```java
public static final int FRS_WRITE_COMPLETED = 3
```

#### Sh2Hub.FRS_WRITE_MODE_READY *(field)*

```java
public static final int FRS_WRITE_MODE_READY = 4
```

#### Sh2Hub.FRS_WRITE_FAILED *(field)*

```java
public static final int FRS_WRITE_FAILED = 5
```

#### Sh2Hub.FRS_WRITE_NOT_IN_WRITE_MODE *(field)*

```java
public static final int FRS_WRITE_NOT_IN_WRITE_MODE = 6
```

#### Sh2Hub.FRS_WRITE_INVALID_LENGTH *(field)*

```java
public static final int FRS_WRITE_INVALID_LENGTH = 7
```

#### Sh2Hub.FRS_WRITE_RECORD_VALID *(field)*

```java
public static final int FRS_WRITE_RECORD_VALID = 8
```

#### Sh2Hub.FRS_WRITE_RECORD_INVALID *(field)*

```java
public static final int FRS_WRITE_RECORD_INVALID = 9
```

#### Sh2Hub.COUNTS_GET *(field)*

```java
public static final int COUNTS_GET = 0
```

#### Sh2Hub.COUNTS_CLEAR *(field)*

```java
public static final int COUNTS_CLEAR = 1
```

#### Sh2Hub.ME_CAL_GET *(field)*

```java
public static final int ME_CAL_GET = 0x01
```

ME Calibration subcommand: report the current configuration.

#### Sh2Hub.splitControlCargo

```java
public static List<byte[]> splitControlCargo(byte[] payload)
```

Split a control cargo into its reports; an unknown id or a short tail ends the walk (the rest
is one piece: its length is unknowable).

#### Sh2Hub.countsGetParams

```java
public static byte[] countsGetParams(int sensorId)
```

#### Sh2Hub.countsClearParams

```java
public static byte[] countsClearParams(int sensorId)
```

#### Sh2Hub.errorsParams

```java
public static byte[] errorsParams(int severity)
```

Errors of `severity` or greater (0 = all).

#### Sh2Hub.tareNowParams

```java
public static byte[] tareNowParams(int axes, TareBasis basis)
```

`axes`: a `TARE_*` bitmap.

#### Sh2Hub.persistTareParams

```java
public static byte[] persistTareParams()
```

#### Sh2Hub.setReorientationParams

```java
public static byte[] setReorientationParams(double x, double y, double z, double w)
```

Tare subcommand 2: the reorientation quaternion as four Q14 int16 (all zero clears).

#### Sh2Hub.meCalibrationParams

```java
public static byte[] meCalibrationParams(boolean accel, boolean gyro, boolean mag, boolean planar, int subcommand)
```

ME Calibration (0x07): subcommand 0 configures, 1 (`ME_CAL_GET`) reads back.

#### Sh2Hub.interactiveCalibrationParams

```java
public static byte[] interactiveCalibrationParams(int intent)
```

Interactive Calibration (0x0E, BNO086 only): P0 = motion intent; no response.

#### Sh2Hub.forceFlush

```java
public static byte[] forceFlush(int sensorId)
```

Force Sensor Flush (0xF0, SH-2 §6.5.6); the hub answers with Flush Completed (0xEF).

#### Sh2Hub.periodicDcdParams

```java
public static byte[] periodicDcdParams(boolean enable)
```

Periodic DCD save (0x09): P0 0 = enable, 1 = disable; no response.

### Sh2Hub.TareBasis

```java
public enum TareBasis
```

The rotation vector a tare refers to (Tare Now P2).

#### Sh2Hub.TareBasis — constants

`ROTATION_VECTOR`(0), `GAME_ROTATION_VECTOR`(1), `GEOMAGNETIC_ROTATION_VECTOR`(2), `GYRO_INTEGRATED_RV`(3), `ARVR_STABILIZED_RV`(4), `ARVR_STABILIZED_GAME_RV`(5)

#### Sh2Hub.TareBasis.value *(field)*

```java
public final int value
```

### Sh2Hub.MotionIntent

```java
public enum MotionIntent
```

P0 of Interactive Calibration (0x0E, SH-2 §6.4.12.1): what the host knows about the device's
motion. UNKNOWN is the hub's initial state.

#### Sh2Hub.MotionIntent — constants

`UNKNOWN`(0), `STATIONARY_NO_VIBRATION`(1), `STATIONARY_WITH_VIBRATION`(2), `IN_MOTION`(3), `IN_MOTION_ACCELERATING`(4)

#### Sh2Hub.MotionIntent.value *(field)*

```java
public final int value
```

### Sh2Hub.Sh2Exception

```java
public static final class Sh2Exception extends RuntimeException
```

A SH-2 level failure: a non-zero command status, an FRS error.

### Sh2Hub.FeatureResponse

```java
public record FeatureResponse(int sensorId, int flags, int sensitivity, long intervalUs, long batchUs, long cfgWord)
```

Get Feature Response (0xFC), 17 bytes: the rates in effect.

#### Sh2Hub.FeatureResponse.unpack

```java
public static FeatureResponse unpack(byte[] p)
```

### Sh2Hub.ProductId

```java
public record ProductId(int resetCause, int swVersionMajor, int swVersionMinor, long swPartNumber, long swBuildNumber, int swVersionPatch)
```

Product ID Response (0xF8), 16 bytes, one per software component (the SH-2 application
first); the reset cause (SH-2 §6.4.5.2) is set on the first only.

#### Sh2Hub.ProductId.version

```java
public String version()
```

#### Sh2Hub.ProductId.unpack

```java
public static ProductId unpack(byte[] p)
```

### Sh2Hub.CommandResponse

```java
public record CommandResponse(int seq, int command, int commandSeq, int responseSeq, byte[] r)
```

Command Response (0xF1), 16 bytes. `commandSeq` echoes the request (correlate on it plus
`command`); `responseSeq` counts responses to one request; R0 is the status of most
commands (0 = success).

#### Sh2Hub.CommandResponse.status

```java
public int status()
```

#### Sh2Hub.CommandResponse.r

```java
public int r(int i)
```

R`i` as an unsigned byte.

#### Sh2Hub.CommandResponse.unpack

```java
public static CommandResponse unpack(byte[] p)
```

### Sh2Hub.FrsReadResponse

```java
public record FrsReadResponse(int status, int dataLength, int offsetWords, long data0, long data1, int frsType)
```

FRS Read Response (0xF3), 16 bytes, up to two data words.

#### Sh2Hub.FrsReadResponse.unpack

```java
public static FrsReadResponse unpack(byte[] p)
```

### Sh2Hub.FrsWriteResponse

```java
public record FrsWriteResponse(int status, int offsetWords)
```

FRS Write Response (0xF5), 4 bytes.

#### Sh2Hub.FrsWriteResponse.unpack

```java
public static FrsWriteResponse unpack(byte[] p)
```

### Sh2Hub.ErrorRecord

```java
public record ErrorRecord(int severity, int seq, int source, int error, int module, int code)
```

One error-queue entry (command 0x01 response, R0..R5).

#### Sh2Hub.ErrorRecord.from

```java
public static ErrorRecord from(CommandResponse r)
```

### Sh2Hub.Counts

```java
public record Counts(int sensorId, long offered, long accepted, long on, long attempted)
```

Per-sensor event counts (command 0x02, two responses).

### Sh2Hub.CalibrationConfig

```java
public record CalibrationConfig(boolean accel, boolean gyro, boolean mag, boolean planar)
```

ME calibration enables as the sensor reports them.

### Sh2Hub.SensorMetadata

```java
public record SensorMetadata(int meVersion, int mhVersion, int shVersion, long rangeRaw, long resolutionRaw, int revision, int powerMaQ10, long minPeriodUs, long maxPeriodUs, int fifoMax, int fifoReserved, int batchBufferBytes, int qPoint1, int qPoint2, int qPoint3, long[] rawWords)
```

A sensor's metadata FRS record (revision-gated fields are 0 on older records);
`rawWords` is authoritative.

#### Sh2Hub.SensorMetadata.fromWords

```java
public static SensorMetadata fromWords(long[] words)
```

#### Sh2Hub.SensorMetadata.fields

```java
public Map<String, Object> fields()
```

The fields under the reference SDK's names.

### Sh2Hub.FrsReadSession

```java
public static final class FrsReadSession
```

A multi-packet FRS read: send `request()`, `feed` every 0xF3 of the record until
it returns true; `words()` holds the record. Error statuses throw `Sh2Exception`.

#### Sh2Hub.FrsReadSession.frsType

```java
public int frsType()
```

#### Sh2Hub.FrsReadSession.done

```java
public boolean done()
```

#### Sh2Hub.FrsReadSession.words

```java
public long[] words()
```

#### Sh2Hub.FrsReadSession.request

```java
public byte[] request()
```

#### Sh2Hub.FrsReadSession.feed

```java
public boolean feed(FrsReadResponse r)
```

### Sh2Hub.FrsWriteSession

```java
public static final class FrsWriteSession
```

A multi-packet FRS write: send `request()`; for every 0xF5 `feed` returns the
next Write Data payload, or null; `done()` on WRITE_COMPLETED. Error statuses throw
`Sh2Exception`.

#### Sh2Hub.FrsWriteSession.frsType

```java
public int frsType()
```

#### Sh2Hub.FrsWriteSession.done

```java
public boolean done()
```

#### Sh2Hub.FrsWriteSession.request

```java
public byte[] request()
```

#### Sh2Hub.FrsWriteSession.feed

```java
public byte[] feed(FrsWriteResponse r)
```

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

#### Shtp.CHANNEL_EXECUTABLE *(field)*

```java
public static final int CHANNEL_EXECUTABLE = 1
```

SHTP channel 1: device executable (reset / on / sleep; RX 0x01 = reset done).

#### Shtp.EXECUTABLE_RESET *(field)*

```java
public static final int EXECUTABLE_RESET = 0x01
```

One-byte host writes on the executable channel (contract 05 §6a); the hub
answers none of them. RESET restarts the hub (the 0x01 reset-complete
follows on the same channel); SLEEP stops every report except those
enabled with `Sh2#FEATURE_FLAG_WAKEUP` | `Sh2#FEATURE_FLAG_ALWAYS_ON`,
keeping the configuration; ON ends SLEEP and every report resumes at its
rate. See `Sh2#buildSleep()` / `Sh2#buildPowerOn()`.

#### Shtp.EXECUTABLE_ON *(field)*

```java
public static final int EXECUTABLE_ON = 0x02
```

#### Shtp.EXECUTABLE_SLEEP *(field)*

```java
public static final int EXECUTABLE_SLEEP = 0x03
```

#### Shtp.packShtpHeader

```java
public static byte[] packShtpHeader(ShtpHeader hdr)
```

Fields out of their widths (length u15, channel u8, seq u8) → IllegalArgumentException.

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
public record ShtpCargo(int channel, int seq, byte[] payload, long captureUs)
```

One reassembled cargo: `payload` excludes all SHTP headers; `seq`
and `captureUs` are the FIRST fragment's — `captureUs` is its
`RPT_DATA.timestamp_us`, the capture the cargo's 0xFB base delta
refers to (contract 05 §3).

### Shtp.ShtpLayer

```java
public static final class ShtpLayer
```

Per-channel TX sequence counters + RX cargo reassembly.

#### Shtp.ShtpLayer.discarded *(field)*

```java
public int discarded = 0
```

Incomplete cargos and junk-channel (≥ 6) frames thrown away.

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

`feed(byte[], long)` with capture 0 (cargo `captureUs` = 0).

#### Shtp.ShtpLayer.feed

```java
public ShtpCargo feed(byte[] frame, long captureUs)
```

Consume one inbound frame; return the cargo when complete, else null.
`captureUs` is the frame's `RPT_DATA.timestamp_us`; the cargo
carries the FIRST fragment's — the base its 0xFB timestamps refer to
(contract 05 §3), not the fragment that completed it.

#### Shtp.ShtpLayer.reset

```java
public void reset()
```

Forget all TX seq counters and partial cargos (sensor reset).

## BNO055 (IMU)

### Bno055Device

```java
public final class Bno055Device extends DepzDevice
```

BNO055 absolute-orientation IMU (contract 13). The board is a thin I2C register bridge: mode,
units, axis remap and calibration are register access here; the sensor fuses on chip at 100 Hz.
Typical use: `configure`, then `startStream` and read `samples`, or poll
`readSample`. Page 1 (sensor configs, interrupts, unique id) is reached through PAGE_ID
and refused while streaming.

#### Bno055Device.bridgeInfo

```java
public Bno055.Bno055Info bridgeInfo()
```

RPT_BNO_INFO: chip ids, sensor firmware revision, bridge counters. Safe while streaming.

#### Bno055Device.isAlive

```java
public boolean isAlive()
```

True when the bridge passed the chip-id handshake and the sensor answers with the BNO055 ids.

#### Bno055Device.resetSensor

```java
public void resetSensor()
```

Hardware reset through nRESET (the bridge answers after the ~0.5 s boot handshake). Stops a
stream; the sensor returns in CONFIG with power-on units and no calibration —
`configure` again, or `restoreConfiguration()`.

#### Bno055Device.readRegisters

```java
public byte[] readRegisters(int addr, int length, int page)
```

Read `length` bytes at `addr` on `page` (128-byte transfers). Page 1 is refused while streaming.

#### Bno055Device.readRegisters

```java
public byte[] readRegisters(int addr, int length)
```

#### Bno055Device.writeRegisters

```java
public void writeRegisters(int addr, byte[] data, int page)
```

Write at `addr` on `page`. Most configuration registers only take writes in CONFIG
mode (the sensor ignores the rest); the typed setters handle that.

#### Bno055Device.writeRegisters

```java
public void writeRegisters(int addr, byte[] data)
```

#### Bno055Device.readRegister

```java
public int readRegister(int addr, int page)
```

#### Bno055Device.readRegister

```java
public int readRegister(int addr)
```

#### Bno055Device.writeRegister

```java
public void writeRegister(int addr, int value, int page)
```

#### Bno055Device.writeRegister

```java
public void writeRegister(int addr, int value)
```

#### Bno055Device.getOperationMode

```java
public OprMode getOperationMode()
```

#### Bno055Device.setOperationMode

```java
public void setOperationMode(OprMode mode)
```

Switch OPR_MODE and wait out the switching time; into a fusion mode also wait (up to 1 s) for
the fusion outputs. The sensor only switches between CONFIG and another mode, so a switch
between two operating modes goes through CONFIG.

#### Bno055Device.getPowerMode

```java
public PwrMode getPowerMode()
```

#### Bno055Device.setPowerMode

```java
public void setPowerMode(PwrMode mode)
```

Normal / low power / suspend, written in CONFIG with the operating mode restored after. A
suspended chip ignores OPR_MODE writes, so the mode is kept here and put back on wake-up (NDOF
when the chip was already asleep at connect, contract 13 §4.6).

#### Bno055Device.getUnits

```java
public Units getUnits()
```

#### Bno055Device.setUnits

```java
public void setUnits(Units units)
```

#### Bno055Device.getAxisRemap

```java
public AxisRemap getAxisRemap()
```

#### Bno055Device.setAxisRemap

```java
public void setAxisRemap(AxisRemap remap)
```

Remap the output axes for the board's mounting (`AxisRemap#placement` gives the datasheet
placements P0..P7; P1 is the default).

#### Bno055Device.getTemperatureSource

```java
public TempSource getTemperatureSource()
```

#### Bno055Device.setTemperatureSource

```java
public void setTemperatureSource(TempSource source)
```

#### Bno055Device.configure

```java
public void configure(OprMode mode, Units units, AxisRemap axisRemap, CalibrationProfile calibration)
```

The usual session setup: CONFIG → units → axis remap → calibration profile → `mode`; in a
fusion mode it returns once the fusion outputs are live (~70 ms). Remembered for
`restoreConfiguration()`. `units` null means the power-on units; `axisRemap`
and `calibration` null leave the sensor's.

#### Bno055Device.configure

```java
public void configure(OprMode mode, Units units)
```

#### Bno055Device.configure

```java
public void configure(OprMode mode)
```

#### Bno055Device.configure

```java
public void configure()
```

NDOF with the power-on units.

#### Bno055Device.restoreConfiguration

```java
public void restoreConfiguration()
```

Re-apply the last `configure` — after `resetSensor()`, or when
`bridgeInfo().sensorResets()` rose (bus recovery pulses nRESET). A running stream keeps
running.

#### Bno055Device.systemStatus

```java
public SystemStatus systemStatus()
```

ST_RESULT, SYS_CLK_STATUS, SYS_STATUS, SYS_ERR.

#### Bno055Device.selfTest

```java
public SystemStatus selfTest()
```

The built-in self-test (datasheet §3.9.2), in CONFIG, ~400 ms; the operating mode is restored
after. Not while streaming.

#### Bno055Device.calibrationStatus

```java
public CalibStatus calibrationStatus()
```

CALIB_STAT: 0..3 per sensor; in NDOF the sensor calibrates in the background.

#### Bno055Device.readCalibrationProfile

```java
public CalibrationProfile readCalibrationProfile()
```

Offsets and radii (0x55..0x6A), only readable in CONFIG: the driver switches there and back.

#### Bno055Device.writeCalibrationProfile

```java
public void writeCalibrationProfile(CalibrationProfile profile)
```

Restore a stored profile (datasheet §3.11.5). A starting point, not a lock: the background
calibration refines it once fusion resumes.

#### Bno055Device.getSicMatrix

```java
public int[] getSicMatrix()
```

Soft-iron compensation matrix, 9 × i16 row-major, 1.0 = 16384.

#### Bno055Device.setSicMatrix

```java
public void setSicMatrix(int[] matrix)
```

`matrix` null writes the identity.

#### Bno055Device.getAccelConfig

```java
public AccelConfig getAccelConfig()
```

#### Bno055Device.setAccelConfig

```java
public void setAccelConfig(AccelConfig config)
```

Accelerometer range / bandwidth / power; fusion modes override it.

#### Bno055Device.getGyroConfig

```java
public GyroConfig getGyroConfig()
```

#### Bno055Device.setGyroConfig

```java
public void setGyroConfig(GyroConfig config)
```

#### Bno055Device.getMagConfig

```java
public MagConfig getMagConfig()
```

#### Bno055Device.setMagConfig

```java
public void setMagConfig(MagConfig config)
```

#### Bno055Device.uniqueId

```java
public byte[] uniqueId()
```

The chip's 16-byte unique id (page 1, 0x50..0x5F).

#### Bno055Device.getInterruptEnable

```java
public int getInterruptEnable()
```

#### Bno055Device.setInterruptEnable

```java
public void setInterruptEnable(int mask)
```

INT_EN: which interrupt engines run.

#### Bno055Device.getInterruptMask

```java
public int getInterruptMask()
```

#### Bno055Device.setInterruptMask

```java
public void setInterruptMask(int mask)
```

INT_MSK: which enabled interrupts drive the INT pin.

#### Bno055Device.setInterruptSetting

```java
public void setInterruptSetting(int register, int value)
```

One motion-interrupt setting register (page 1, 0x11..0x1F), as a raw byte. Not while streaming.

#### Bno055Device.readInterruptStatus

```java
public int readInterruptStatus()
```

INT_STA — which interrupts fired. Clears on read.

#### Bno055Device.clearInterrupt

```java
public void clearInterrupt()
```

SYS_TRIGGER RST_INT: reset the interrupt status bits and the INT pin.

#### Bno055Device.readSample

```java
public Bno055Sample readSample(int[] block)
```

Poll one register block `{addr, length}` (null: the full 46 bytes 0x08..0x35) and decode
it. Works alongside a stream.

#### Bno055Device.readSample

```java
public Bno055Sample readSample()
```

#### Bno055Device.readQuaternion

```java
public double[] readQuaternion()
```

(w, x, y, z) — the cheapest orientation read (8 bytes).

#### Bno055Device.startStream

```java
public void startStream(int periodMs, int[] block, int trigger)
```

Arm the bridge to read `block` (`{addr, length}`, null: the full block) every
`periodMs` and push it. Fusion runs at 100 Hz, so 10 ms is the useful floor.
`Bno055#TRIGGER_INT` reads on the INT edge, `periodMs` then a missed-edge watchdog
(0 = none). Replaces a running stream.

#### Bno055Device.startStream

```java
public void startStream(int periodMs, int[] block)
```

#### Bno055Device.startStream

```java
public void startStream(int periodMs)
```

#### Bno055Device.startStream

```java
public void startStream()
```

The full block every 10 ms.

#### Bno055Device.stopStream

```java
public void stopStream()
```

#### Bno055Device.streaming

```java
public boolean streaming()
```

#### Bno055Device.onSample

```java
public AutoCloseable onSample(Consumer<Bno055Sample> callback)
```

Call `callback` for every streamed sample (reader thread).

#### Bno055Device.samples

```java
public Subscription<Bno055Sample> samples(int capacity)
```

A pull queue of streamed samples (bounded, drop-oldest).

#### Bno055Device.samples

```java
public Subscription<Bno055Sample> samples()
```

#### Bno055Device.getSample

```java
public Bno055Sample getSample(Duration timeout)
```

The next streamed sample (contract 07 §3).

#### Bno055Device.streamParseErrors

```java
public long streamParseErrors()
```

Stream reports dropped because they did not decode (a short block).

#### Bno055Device.streamDroppedCounts

```java
public List<Long> streamDroppedCounts()
```

### Bno055Sample

```java
public record Bno055Sample(long timestampUs, int addr, byte[] raw, Bno055Regs.Units units, double[] accel, double[] mag, double[] gyro, double[] euler, double[] quaternion, double[] linearAccel, double[] gravity, Double temperature, Bno055Regs.CalibStatus calibration)
```

One decoded BNO055 register block (streamed or polled). A channel is null when the block did not
cover it; values are scaled by `units`, the units in effect when the stream started. Euler
is (heading, roll, pitch), the quaternion (w, x, y, z); linear acceleration and gravity are always
m/s². In non-fusion modes the fusion outputs read zero; in CONFIG everything does.

#### Bno055Sample.decode

```java
public static Bno055Sample decode(long timestampUs, int addr, byte[] data, Bno055Regs.Units units)
```

Decode `data` read at `addr` with `units`.

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

#### Bno055.TIMER_PERIOD_MIN_MS *(field)*

```java
public static final int TIMER_PERIOD_MIN_MS = 1
```

BNO_START_STREAM TIMER period limits, ms (contract 13 §2).

#### Bno055.TIMER_PERIOD_MAX_MS *(field)*

```java
public static final int TIMER_PERIOD_MAX_MS = 60000
```

#### Bno055.packReadReg

```java
public static byte[] packReadReg(int addr, int len)
```

BNO_READ_REG payload `addr u8, len u8` with the transfer limit of contract 13 §1:
`len` 1..128 and `addr + len <= 0x100`, else IllegalArgumentException.

#### Bno055.packWriteReg

```java
public static byte[] packWriteReg(int addr, byte[] data)
```

BNO_WRITE_REG payload: `addr u8, data[1..128]` with `addr + N <= 0x100`, else IllegalArgumentException.

#### Bno055.packStartStream

```java
public static byte[] packStartStream(int trigger, int addr, int len, int periodMs)
```

BNO_START_STREAM payload `trigger u8, addr u8, len u8, period_ms u16`. With the TIMER
trigger the period is 1..60000 ms (contract 13 §2), else IllegalArgumentException; with INT it
is the missed-edge watchdog and 0 disables it. Replaces any running stream.

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

Rejects a payload shorter than its header or than the `len` data bytes it announces.

### Bno055.Bno055Info

```java
public record Bno055Info(int i2cAddr, int chipId, int accId, int magId, int gyrId, int swRev, int blRev, int initialized, int intLevel, long intEdges, int readMinUs, int readMaxUs, int readAvgUs, long txDropped, long i2cErrors, long slotsSkipped, int busRecoveries, int lastI2cError, int sensorResets, int loopMaxUs)
```

RPT_BNO_INFO (38-byte LE payload `<BBBBBHBBBIHHHIIIHBBH`) — sensor
identity (registers 0x00..0x06) plus bridge diagnostics. Counters wrap
silently; watch increments. `read*Us`, `slotsSkipped` and
`loopMaxUs` reset at START_STREAM, `i2cErrors` at every
successful reset handshake. A rising `sensorResets` means the bridge
pulsed nRESET to recover the bus: the sensor is back in CONFIG and the host
must restore its configuration.
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

The live driver over the bridge (mode switching, boot / fusion-start polls,
page discipline, self-test) is `ai.depz.sensor.devices.Bno055Device`.

#### Bno055Regs.MODE_SWITCH_FROM_CONFIG_MS *(field)*

```java
public static final int MODE_SWITCH_FROM_CONFIG_MS = 10
```

Settle time after an OPR_MODE write out of CONFIG (datasheet: 7 ms).

#### Bno055Regs.MODE_SWITCH_TO_CONFIG_MS *(field)*

```java
public static final int MODE_SWITCH_TO_CONFIG_MS = 25
```

Settle time after an OPR_MODE write into CONFIG (datasheet: 19 ms).

#### Bno055Regs.SELF_TEST_TIME_MS *(field)*

```java
public static final int SELF_TEST_TIME_MS = 450
```

The built-in self-test runs ~400 ms (datasheet §3.9.2).

#### Bno055Regs.SYS_STATUS_BOOTING *(field)*

```java
public static final java.util.List<Integer> SYS_STATUS_BOOTING
```

SYS_STATUS values of a sensor still booting (initializing, self-test).

#### Bno055Regs.BOOT_SETTLE_TIMEOUT_MS *(field)*

```java
public static final int BOOT_SETTLE_TIMEOUT_MS = 1000
```

#### Bno055Regs.STUCK_SELF_TEST_POLLS *(field)*

```java
public static final int STUCK_SELF_TEST_POLLS = 20
```

SYS_STATUS 4 polls in a row taken as the leftover of a self-test run in CONFIG, not POST.

#### Bno055Regs.FUSION_START_TIMEOUT_MS *(field)*

```java
public static final int FUSION_START_TIMEOUT_MS = 1000
```

After CONFIG → a fusion mode the fusion outputs read zero for ~70 ms.

#### Bno055Regs.SYS_STATUS_NAMES *(field)*

```java
public static final Map<Integer, String> SYS_STATUS_NAMES
```

#### Bno055Regs.SYS_ERR_NAMES *(field)*

```java
public static final Map<Integer, String> SYS_ERR_NAMES
```

#### Bno055Regs.isInterruptSettingReg

```java
public static boolean isInterruptSettingReg(int register)
```

Page-1 motion-interrupt setting registers (thresholds, durations, axis selects).

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

Unpack whatever channels the register window starting at `addr`
holds. `addr` is a register address (0..255); anything else is
rejected with `IllegalArgumentException`.

### Bno055Regs.SystemStatus

```java
public record SystemStatus(int selfTest, int clkStatus, int status, int error)
```

ST_RESULT (0x36) + SYS_CLK_STATUS / SYS_STATUS / SYS_ERR (0x38..0x3A); INT_STA (0x37) sits
in between and clears on read, so it is not read.

#### Bno055Regs.SystemStatus.statusText

```java
public String statusText()
```

#### Bno055Regs.SystemStatus.errorText

```java
public String errorText()
```

Meaningful when `status()` is 1 (system error).

#### Bno055Regs.SystemStatus.selfTestPassed

```java
public boolean selfTestPassed()
```

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

`value`: the UNIT_SEL register byte (outside 0..255 → IllegalArgumentException).

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

`value`: the CALIB_STAT register byte (outside 0..255 → IllegalArgumentException).

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

The two register bytes (each outside 0..255 → IllegalArgumentException).

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

`value`: the ACC_CONFIG register byte (outside 0..255 → IllegalArgumentException).

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

`value`: the MAG_CONFIG register byte (outside 0..255 → IllegalArgumentException).

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

`read` opens a file (plain or gzip, `.depzdata.gz`) with the
SDK's own JSON; `parse` takes the content and a caller-supplied line
parser.

#### Dataset.DATASET_SCHEMA_PREFIX *(field)*

```java
public static final String DATASET_SCHEMA_PREFIX
```

#### Dataset.read

```java
public static Dataset read(Path path)
```

Read the `.depzdata` file at `path`; gzip content (by its magic bytes) is inflated.

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

#### Dataset.note

```java
public String note()
```

The header's free-text note, or null.

#### Dataset.records

```java
public List<Record> records()
```

#### Dataset.durationUs

```java
public long durationUs()
```

#### Dataset.play

```java
public void play(Consumer<Record> callback, double speed, Long startTUs, BooleanSupplier cancelled)
```

Deliver the records paced by their timestamps (`speed` 2 = twice as fast, 0 = as fast as
possible), blocking until the end or until `cancelled` returns true (polled before each record
and during waits). Records before `startTUs` (null: none) are skipped. For pause / seek / speed
changes use `DatasetPlayer`.

#### Dataset.play

```java
public void play(Consumer<Record> callback, double speed)
```

`play(Consumer, double, Long, BooleanSupplier)` from the first record, not cancellable.

#### Dataset.play

```java
public void play(Consumer<Record> callback)
```

`play(Consumer, double, Long, BooleanSupplier)` in real time from the first record.

### DatasetPlayer

```java
public final class DatasetPlayer implements AutoCloseable
```

Timeline player over a dataset: paced delivery with play / pause / seek / speed — the engine behind a
viewer's playback mode (contract 09 §Player; reference: the Python `DatasetPlayer`).

Timing: a media clock anchored at play / resume / seek to (now, t of the last delivered record — of
the first when none was). Record i is due at `anchor_wall + (t_i − anchor_t) / speed`: the first
record goes out at once, each later one after its gap to the previous one. Pacing is against the clock,
so a slow consumer gets overdue records back to back and playback does not drift. Speed 0 delivers as
fast as possible.

Threads: records and the `PlayerState#DONE` state are delivered on a player thread (started by
the first `play()`); a state change a call makes is reported on the calling thread before it
returns. Callbacks run under the player's lock — they see a consistent player and may call its methods,
and once `pause()` returns no record callback is running or will start. They must not block. With
`threaded = false` no thread starts and the owner calls `tick()` (a GUI timer, or a test
with a fake clock).

#### DatasetPlayer.THREAD_NAME *(field)*

```java
public static final String THREAD_NAME
```

The player thread's name.

#### DatasetPlayer.reader

```java
public Dataset reader()
```

#### DatasetPlayer.state

```java
public PlayerState state()
```

#### DatasetPlayer.durationUs

```java
public long durationUs()
```

Span of the timeline: the last record's t minus the first's.

#### DatasetPlayer.positionUs

```java
public long positionUs()
```

µs from the first record to the last delivered one (after a seek: to the record just before the
target); 0 before anything was delivered.

#### DatasetPlayer.speed

```java
public double speed()
```

Playback speed (finite, ≥ 0; 0 = as fast as possible).

#### DatasetPlayer.setSpeed

```java
public void setSpeed(double value)
```

Set the playback speed (finite, ≥ 0, else `IllegalArgumentException`). A change while playing
applies at once: the rest of the current wait is rescaled.

#### DatasetPlayer.onRecord

```java
public AutoCloseable onRecord(Consumer<Dataset.Record> callback)
```

Subscribe to delivered records; close the result to unsubscribe (idempotent).

#### DatasetPlayer.onState

```java
public AutoCloseable onState(Consumer<PlayerState> callback)
```

Subscribe to state changes, called with the new state on every transition.

#### DatasetPlayer.onDeliveryError

```java
public AutoCloseable onDeliveryError(Consumer<Exception> callback)
```

Subscribe to errors of callbacks on the player thread: playback paused there (the record counts as
delivered).

#### DatasetPlayer.play

```java
public void play()
```

Start, or resume after `pause()`; no-op while playing. From `PlayerState#DONE` (or a
position past the end) playback restarts at the first record. An empty dataset goes to done at once.

#### DatasetPlayer.pause

```java
public void pause()
```

Stop delivering; `play()` resumes from the next record, which then waits its full gap again. No-op unless playing.

#### DatasetPlayer.seekUs

```java
public void seekUs(double offsetUs)
```

Move to `offsetUs` µs from the first record: the next record delivered is the first with t ≥
first t + offset (negative: the start). Records in between are skipped. The state does not change,
except that a seek past the end while playing ends playback.

#### DatasetPlayer.tick

```java
public Double tick()
```

Deliver every record due now, on the calling thread. Returns the seconds until the next one is due,
or null when not playing. For `threaded = false`; harmless with a player thread. A callback's
exception pauses playback and propagates.

#### DatasetPlayer.await

```java
public boolean await(Duration timeout)
```

Block until the player is not playing (done, or paused by someone else): true, or false on timeout
(null: no timeout). Needs the player thread, else `InvalidStateException`. (Python's
`wait`; `wait` is taken by `Object` in Java.)

#### DatasetPlayer.await

```java
public boolean await()
```

`await(Duration)` without a timeout.

### DatasetRecords

```java
public final class DatasetRecords
```

The contract 09 record value (`v`) of each kind, from the device classes' results — what
`SessionRecorder` writes, public for writers of their own. Key for key the reference Python
SDK's values (integers as `Long`, reals as `Double`, arrays as lists).

#### DatasetRecords.sr04

```java
public static Map<String, Object> sr04(Sr04Measurement m)
```

`sr04`: `{echo_us, source}`.

#### DatasetRecords.vl53l4

```java
public static Map<String, Object> vl53l4(Vl53l4Measurement m)
```

`vl53l4`: the result fields, distance raw whatever the status.

#### DatasetRecords.vl53l8

```java
public static Map<String, Object> vl53l8(Vl53l8Frame f, boolean layers)
```

`vl53l8`: per-zone arrays cut to the resolution; `layers` adds signal and ambient
(kcps/SPAD), sigma (mm) and reflectance (%) as reals; `motion` when the indicator is configured.

#### DatasetRecords.vl53l8

```java
public static Map<String, Object> vl53l8(Vl53l8Frame f)
```

#### DatasetRecords.vl53l8Cnh

```java
public static Map<String, Object> vl53l8Cnh(byte[] cnhRaw, Vl53l8CnhSetup setup)
```

`vl53l8_cnh`: the raw CNH block of a frame with the armed setup's decode parameters.

#### DatasetRecords.vl53lx

```java
public static Map<String, Object> vl53lx(Vl53lxMeasurement m, String product, String driver)
```

`vl53lx`: the family-wide result, plus `targets` when the driver reported several.

#### DatasetRecords.bno055

```java
public static Map<String, Object> bno055(Bno055Sample s)
```

`bno055`: `unit_sel` and every channel the streamed block covered, scaled.

#### DatasetRecords.tmf8829

```java
public static Map<String, Object> tmf8829(MeasurementBuilder.Measurement m)
```

`tmf8829`: one measurement — distances in mm rounded to 0.1, the raw per-zone arrays.

#### DatasetRecords.bno086

```java
public static Map<String, Object> bno086(Reports.Report r)
```

`bno086`: the report in snake_case — type and sensor id, then only the fields the report
carries (scaled), the integer counts of the raw ADC streams, and the device timestamp.

#### DatasetRecords.round1

```java
public static double round1(double x)
```

Python's `round(x, 1)`: the nearest tenth of the exact binary value, ties to even, as the
double closest to it.

### DatasetWriter

```java
public final class DatasetWriter implements AutoCloseable
```

Low-level thread-safe `.depzdata` record writer (gzip when the path ends in `.gz`).
Prefer `SessionRecorder`.

#### DatasetWriter.SCHEMA *(field)*

```java
public static final String SCHEMA
```

The schema id this SDK writes.

#### DatasetWriter.recordsWritten

```java
public long recordsWritten()
```

#### DatasetWriter.write

```java
public void write(String deviceId, long tHostUs, String kind, Map<String, Object> value)
```

One record: device id, host µs, kind, value.

### PlayerState

```java
public enum PlayerState
```

`DatasetPlayer` state (contract 09 §Player).

#### PlayerState — constants

`IDLE`, `PLAYING`, `PAUSED`, `DONE`

### SessionRecorder

```java
public final class SessionRecorder implements AutoCloseable
```

Record decoded data from several devices onto one host timeline (contract 09). `add` each open
device (it runs `DepzDevice#syncTime`), then `start()`: the recorder hooks every device's
result stream and writes a record per result at device time − offset. It never reconfigures a device —
start and stop streaming yourself. `stop()` (or closing) unhooks and closes the file.

#### SessionRecorder.add

```java
public synchronized String add(DepzDevice device, String deviceId, int syncSamples)
```

Register a device before recording starts; runs `DepzDevice#syncTime` so its timestamps land
on the shared host timeline. Returns its device id (`d0`, `d1`, … unless given).

#### SessionRecorder.add

```java
public String add(DepzDevice device)
```

#### SessionRecorder.recordsWritten

```java
public long recordsWritten()
```

#### SessionRecorder.start

```java
public synchronized void start()
```

Open the file and hook every device's stream.

#### SessionRecorder.stop

```java
public synchronized void stop()
```

Unhook every device and close the file.

## JSON

### JsonText

```java
public final class JsonText
```

The SDK's own minimal JSON, for `.depzrec` captures and `.depzdata`
datasets (no external dependency). `parse`: objects → `Map<String,Object>`
(insertion-ordered), arrays → `List<Object>`, strings → `String`,
integers → `Long` (`BigInteger` beyond the long range), reals →
`Double`, `true/false` → `Boolean`, `null` → `null`.
`serialize` writes those back, plus any `Number`, primitive arrays and
`byte[]` (as a lowercase hex string). Malformed or truncated text is refused with
`IllegalArgumentException` naming the index — never an index error of the runtime.

#### JsonText.parse

```java
public static Object parse(String text)
```

#### JsonText.isBlankLine

```java
public static boolean isBlankLine(String line)
```

Contract 08 «Line rules»: a line of nothing but JSON whitespace (space, tab, CR) is blank.

#### JsonText.serialize

```java
public static String serialize(Object value)
```

One value as compact JSON.

#### JsonText.hex

```java
public static String hex(byte[] b)
```

Lowercase hex of `b`.

#### JsonText.unhex

```java
public static byte[] unhex(String s)
```

Bytes of a hex string: even length, hex digits only, else `IllegalArgumentException`.

#### JsonText.base64

```java
public static String base64(byte[] b)
```

Standard base64 of `b`.
