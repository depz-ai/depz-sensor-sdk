# API reference

Auto-generated from the public C# surface under `src/Depz.Sensor/` by
`scripts/gen_api_md.py` — run it to regenerate (see below). Edit the
`///` XML-doc summaries in the source, not this file.

Each sensor also has a focused reference with just its own symbols:
[SR04](sr04/api.md) · [VL53L0X](vl53l0x/api.md) · [VL53L1CX](vl53l1cx/api.md) · [VL53L1CB](vl53l1cb/api.md) · [VL53L3CX](vl53l3cx/api.md) · [VL53L4CD](vl53l4cd/api.md) · [VL53L4CX](vl53l4cx/api.md) · [VL53L5CX](vl53l5cx/api.md) · [VL53L7CX](vl53l7cx/api.md) · [VL53L7CH](vl53l7ch/api.md) · [VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · [TMF8829](tmf8829/api.md) · [BNO055](bno055/api.md) · [BNO085](bno085/api.md) · [BNO086](bno086/api.md).

Regenerate: `python3 scripts/gen_api_md.py` (from the package root).

## Contents

- **Live devices (connection, streams, discovery)**: [`DeviceInfo`](#deviceinfo), [`DepzDevices`](#depzdevices), [`DeviceEvent`](#deviceevent), [`SequenceErrorEvent`](#sequenceerrorevent), [`LinkCrcErrorEvent`](#linkcrcerrorevent), [`TrashEvent`](#trashevent), [`UnsolicitedStatusEvent`](#unsolicitedstatusevent), [`TextEvent`](#textevent), [`TemperatureEvent`](#temperatureevent), [`DisconnectedEvent`](#disconnectedevent), [`TimeSyncResult`](#timesyncresult), [`LinkStats`](#linkstats), [`ReplyMatcher`](#replymatcher), [`DepzDevice`](#depzdevice), [`Subscription`](#subscription), [`ILink`](#ilink), [`RecordingLink`](#recordinglink), [`ReplayLink`](#replaylink), [`SerialPortInfo`](#serialportinfo), [`SerialPorts`](#serialports), [`SerialPortLink`](#serialportlink), [`DepzException`](#depzexception), [`DepzTimeoutException`](#depztimeoutexception), [`StatusException`](#statusexception), [`BusyException`](#busyexception), [`DeviceLostException`](#devicelostexception), [`LinkClosedException`](#linkclosedexception), [`NoDepzDeviceException`](#nodepzdeviceexception), [`BootloaderModeException`](#bootloadermodeexception), [`WrongTypeException`](#wrongtypeexception), [`ReplayMismatchException`](#replaymismatchexception)
- **USB identity**: [`UsbIds`](#usbids), [`Discovery`](#discovery)
- **SR04**: [`Sr04Measurement`](#sr04measurement), [`Sr04Device`](#sr04device), [`Sr04Cmd`](#sr04cmd), [`Sr04Rpt`](#sr04rpt), [`Sr04Data`](#sr04data), [`Sr04`](#sr04)
- **VL53L4CD (ToF)**: [`Vl53l4Measurement`](#vl53l4measurement), [`Vl53l4cdDevice`](#vl53l4cddevice), [`Vl53l4Cmd`](#vl53l4cmd), [`Vl53l4Rpt`](#vl53l4rpt), [`Vl53l4Xshut`](#vl53l4xshut), [`Vl53l4Wire`](#vl53l4wire), [`Vl53l4RegData`](#vl53l4regdata), [`Vl53l4Info`](#vl53l4info), [`Vl53l4StreamData`](#vl53l4streamdata), [`Vl53l4Exception`](#vl53l4exception), [`Vl53l4Results`](#vl53l4results), [`Vl53l4Uld`](#vl53l4uld), [`IVl53l4Platform`](#ivl53l4platform), [`Vl53l4cdDriver`](#vl53l4cddriver)
- **VL53L8 (ToF)**: [`Vl53l8cxDevice`](#vl53l8cxdevice), [`Vl53l8chDevice`](#vl53l8chdevice), [`IVl53l8Platform`](#ivl53l8platform), [`Vl53l8Firmware`](#vl53l8firmware), [`Vl53l8Exception`](#vl53l8exception), [`Vl53l8Motion`](#vl53l8motion), [`Vl53l8Driver`](#vl53l8driver), [`Vl53l8Variant`](#vl53l8variant), [`Vl53l8Frame`](#vl53l8frame), [`Vl53l8FrameDecoder`](#vl53l8framedecoder), [`Vl53l8Cmd`](#vl53l8cmd), [`Vl53l8Rpt`](#vl53l8rpt), [`Vl53l8Wire`](#vl53l8wire), [`Vl53l8RegData`](#vl53l8regdata), [`FrameChunk`](#framechunk), [`FrameReassembler`](#framereassembler), [`Vl53l8Advanced`](#vl53l8advanced), [`MotionConfig`](#motionconfig), [`Vl53l8CnhConfig`](#vl53l8cnhconfig), [`Vl53l8CnhAggregate`](#vl53l8cnhaggregate), [`Vl53l8CnhResult`](#vl53l8cnhresult), [`Vl53l8Cnh`](#vl53l8cnh), [`Vl53l8CnhSetup`](#vl53l8cnhsetup), [`Vl53l8Tables`](#vl53l8tables)
- **VL53L5CX / VL53L7CX / VL53L7CH (ToF)**: [`Vl53l7Cmd`](#vl53l7cmd), [`Vl53l7Rpt`](#vl53l7rpt), [`Vl53l7PinAction`](#vl53l7pinaction), [`Vl53l7I2cError`](#vl53l7i2cerror), [`Vl53l7Wire`](#vl53l7wire), [`Vl53l7Info`](#vl53l7info), [`Vl53l7Model`](#vl53l7model), [`Vl53l7Discovery`](#vl53l7discovery), [`Vl53l7Frames`](#vl53l7frames), [`Vl53l7cxDevice`](#vl53l7cxdevice), [`Vl53l5cxDevice`](#vl53l5cxdevice), [`Vl53l7chDevice`](#vl53l7chdevice)
- **VL53L0X / L1CX / L1CB / L3CX / L4CX (ToF)**: [`Vl53lxCmd`](#vl53lxcmd), [`Vl53lxRpt`](#vl53lxrpt), [`Vl53lxClearStep`](#vl53lxclearstep), [`Vl53lxWire`](#vl53lxwire), [`Vl53lxInfo`](#vl53lxinfo), [`Vl53lxDriverKind`](#vl53lxdriverkind), [`Vl53lxClass`](#vl53lxclass), [`Vl53lxProduct`](#vl53lxproduct), [`Vl53lxProducts`](#vl53lxproducts), [`Vl53lxDieVariant`](#vl53lxdievariant), [`Vl53lxDieResult`](#vl53lxdieresult), [`Vl53lxL0xRaw`](#vl53lxl0xraw), [`Vl53lxHistogramRaw`](#vl53lxhistogramraw), [`Vl53lxDecode`](#vl53lxdecode), [`Vl53lxMeasurement`](#vl53lxmeasurement), [`Vl53lxDevice`](#vl53lxdevice), [`Vl53l0xDevice`](#vl53l0xdevice), [`Vl53l1cxDevice`](#vl53l1cxdevice), [`Vl53l1cbDevice`](#vl53l1cbdevice), [`Vl53l3cxDevice`](#vl53l3cxdevice), [`Vl53l4cxDevice`](#vl53l4cxdevice), [`BareRegs`](#bareregs), [`BareMath`](#baremath), [`HistConfig`](#histconfig), [`HistPostProcessConfig`](#histpostprocessconfig), [`HistogramBinData`](#histogrambindata), [`BareDeviceState`](#baredevicestate), [`LlState`](#llstate), [`BareDriver`](#baredriver), [`BareTables`](#baretables), [`RegField`](#regfield), [`RegBlockDef`](#regblockdef), [`RegBlock`](#regblock), [`DeviceImage`](#deviceimage), [`OpticalCentre`](#opticalcentre), [`CalPeakRateMap`](#calpeakratemap), [`AdditionalOffsetCalData`](#additionaloffsetcaldata), [`FmtRangeData`](#fmtrangedata), [`NvmReader`](#nvmreader), [`RangeData`](#rangedata), [`RangeResults`](#rangeresults), [`FrameHistory`](#framehistory), [`Hist`](#hist), [`Vl53l0xUldDriver`](#vl53l0xulddriver), [`Vl53l1Die`](#vl53l1die), [`Vl53l4UldDriver`](#vl53l4ulddriver), [`Vl53l3UlpDriver`](#vl53l3ulpdriver), [`Vl53l1UldDriver`](#vl53l1ulddriver), [`Vl53lxException`](#vl53lxexception), [`Vl53lxProtocolException`](#vl53lxprotocolexception), [`Vl53lxPlatform`](#vl53lxplatform), [`Vl53lxTarget`](#vl53lxtarget), [`Vl53lxRange`](#vl53lxrange), [`Vl53lxDriver`](#vl53lxdriver), [`Vl53lxHistogramDriver`](#vl53lxhistogramdriver)
- **BNO086 (IMU)**: [`Bno086Device`](#bno086device), [`SensorId`](#sensorid), [`Bno086Q`](#bno086q), [`BnoReport`](#bnoreport), [`InputReport`](#inputreport), [`Acceleration`](#acceleration), [`Gyroscope`](#gyroscope), [`Magnetometer`](#magnetometer), [`UncalibratedGyroscope`](#uncalibratedgyroscope), [`UncalibratedMagnetometer`](#uncalibratedmagnetometer), [`RotationVector`](#rotationvector), [`ScalarReport`](#scalarreport), [`TapDetector`](#tapdetector), [`StepCounter`](#stepcounter), [`StepDetector`](#stepdetector), [`SignificantMotion`](#significantmotion), [`StabilityClassifier`](#stabilityclassifier), [`MotionRequest`](#motionrequest), [`ShakeDetector`](#shakedetector), [`GenericEvent`](#genericevent), [`PersonalActivityClassifier`](#personalactivityclassifier), [`RawSensor`](#rawsensor), [`GyroIntegratedRV`](#gyrointegratedrv), [`FlushCompleted`](#flushcompleted), [`UnknownReport`](#unknownreport), [`Sh2Reports`](#sh2reports), [`FeatureFlags`](#featureflags), [`Sh2Control`](#sh2control), [`Bno086Cmd`](#bno086cmd), [`Bno086Rpt`](#bno086rpt), [`Bno086Data`](#bno086data), [`Sh2Exception`](#sh2exception), [`ControlReport`](#controlreport), [`Sh2Command`](#sh2command), [`OscillatorType`](#oscillatortype), [`ErrorSource`](#errorsource), [`TareBasis`](#tarebasis), [`TareAxis`](#tareaxis), [`MotionIntent`](#motionintent), [`FrsRecordId`](#frsrecordid), [`FrsStatus`](#frsstatus), [`FrsWriteStatus`](#frswritestatus), [`FeatureResponse`](#featureresponse), [`ProductId`](#productid), [`CommandResponse`](#commandresponse), [`FrsReadResponse`](#frsreadresponse), [`FrsWriteResponse`](#frswriteresponse), [`ErrorRecord`](#errorrecord), [`Counts`](#counts), [`CalibrationConfig`](#calibrationconfig), [`SensorMetadata`](#sensormetadata), [`Sh2Hub`](#sh2hub), [`FrsReadSession`](#frsreadsession), [`FrsWriteSession`](#frswritesession), [`ShtpChannel`](#shtpchannel), [`ExecutableCommand`](#executablecommand), [`ShtpHeader`](#shtpheader), [`ShtpCargo`](#shtpcargo), [`ShtpLayer`](#shtplayer)
- **BNO055 (IMU)**: [`Bno055Cmd`](#bno055cmd), [`Bno055Rpt`](#bno055rpt), [`Bno055Trigger`](#bno055trigger), [`Bno055Wire`](#bno055wire), [`Bno055RegData`](#bno055regdata), [`Bno055StreamData`](#bno055streamdata), [`Bno055Info`](#bno055info), [`Bno055OprMode`](#bno055oprmode), [`Bno055PwrMode`](#bno055pwrmode), [`Bno055TempSource`](#bno055tempsource), [`Bno055Vec3`](#bno055vec3), [`Bno055Euler`](#bno055euler), [`Bno055Quat`](#bno055quat), [`Bno055Units`](#bno055units), [`Bno055CalibStatus`](#bno055calibstatus), [`Bno055CalibrationProfile`](#bno055calibrationprofile), [`Bno055Axis`](#bno055axis), [`Bno055AxisRemap`](#bno055axisremap), [`Bno055AccelConfig`](#bno055accelconfig), [`Bno055GyroConfig`](#bno055gyroconfig), [`Bno055MagConfig`](#bno055magconfig), [`Bno055SystemStatus`](#bno055systemstatus), [`Bno055RawBlock`](#bno055rawblock), [`Bno055Regs`](#bno055regs), [`Bno055Sample`](#bno055sample), [`Bno055Device`](#bno055device)
- **TMF8829 (dToF)**: [`Tmf8829Exception`](#tmf8829exception), [`Tmf8829InterruptEvent`](#tmf8829interruptevent), [`Tmf8829AppliedConfig`](#tmf8829appliedconfig), [`Tmf8829Device`](#tmf8829device), [`Tmf8829Cmd`](#tmf8829cmd), [`Tmf8829Rpt`](#tmf8829rpt), [`Tmf8829EventCode`](#tmf8829eventcode), [`Tmf8829Wire`](#tmf8829wire), [`Tmf8829RegData`](#tmf8829regdata), [`Tmf8829Info`](#tmf8829info), [`Tmf8829FrameChunk`](#tmf8829framechunk), [`Tmf8829Event`](#tmf8829event), [`Tmf8829RawFrame`](#tmf8829rawframe), [`Tmf8829FrameReassembler`](#tmf8829framereassembler), [`Tmf8829FrameException`](#tmf8829frameexception), [`Tmf8829FrameInfo`](#tmf8829frameinfo), [`Tmf8829PixelResults`](#tmf8829pixelresults), [`Tmf8829HistogramFrame`](#tmf8829histogramframe), [`Tmf8829Frames`](#tmf8829frames), [`Tmf8829Measurement`](#tmf8829measurement), [`Tmf8829MeasurementBuilder`](#tmf8829measurementbuilder), [`Tmf8829TimeFlags`](#tmf8829timeflags), [`Tmf8829SensorClock`](#tmf8829sensorclock), [`Tmf8829FrameTime`](#tmf8829frametime), [`Tmf8829CaptureTimer`](#tmf8829capturetimer), [`Tmf8829Config`](#tmf8829config), [`Tmf8829Firmware`](#tmf8829firmware)
- **Firmware update**: [`FwDepz`](#fwdepz)
- **Datasets (record & replay)**: [`TimeSync`](#timesync), [`DatasetDevice`](#datasetdevice), [`DatasetRecord`](#datasetrecord), [`DatasetReader`](#datasetreader), [`DatasetWriter`](#datasetwriter), [`SessionRecorder`](#sessionrecorder), [`PlayerState`](#playerstate), [`DatasetPlayer`](#datasetplayer), [`DatasetRecords`](#datasetrecords), [`JsonText`](#jsontext)
- **Transport**: [`Framing`](#framing), [`PacketParser`](#packetparser), [`ParserEvent`](#parserevent), [`Packet`](#packet), [`Trash`](#trash), [`CrcError`](#crcerror), [`Crc`](#crc), [`CrcType`](#crctype), [`CrcTypeExtensions`](#crctypeextensions)
- **Protocol codecs**: [`Cmd`](#cmd), [`Rpt`](#rpt), [`Status`](#status), [`SyncPinMode`](#syncpinmode), [`SyncPinPolarity`](#syncpinpolarity), [`Reports`](#reports), [`StatusReport`](#statusreport), [`TextReport`](#textreport), [`SyncTimeReport`](#synctimereport), [`TemperatureReport`](#temperaturereport), [`SequenceErrorReport`](#sequenceerrorreport), [`SyncPinConfig`](#syncpinconfig), [`Common`](#common), [`SensorType`](#sensortype), [`Identity`](#identity), [`IdentityParser`](#identityparser), [`Wire`](#wire)

## Live devices (connection, streams, discovery)

### DeviceInfo

```csharp
public sealed record DeviceInfo(string Port, string Mode, SensorType? SensorType, string SoftwareName, string FwVersion, string DeviceName, string SerialNumber, int? UsbVid = null, int? UsbPid = null, string? UsbSerial = null)
```

What the contract 02 §4.4 probe learned about one port.

#### DeviceInfo.UsbModelHint *(property)*

```csharp
public string? UsbModelHint
```

The model the USB-id table gives the port (contract 02 §4.2), or null.

### DepzDevices

```csharp
public static class DepzDevices
```

Opening devices (contract 02 §4, contract 07 §2): the probe, class resolution and continuing a connection as its sensor class. Port discovery (`OpenDevice`, `ListDepzDevices`) needs .NET 8; in Unity open your own `ILink` and call `OpenOnLink`.

#### DepzDevices.OpenOnLink

```csharp
public static DepzDevice OpenOnLink(ILink link, int? usbVid = null, int? usbPid = null, TimeSpan? timeout = null)
```

Open a device on a connection that cannot be reopened — a replay or recording link, a link the host opened (contract 02 §4.4). Runs the probe (GET_NAME_ACTIVE_SOFTWARE, GET_DEVICE_NAME, GET_SERIAL) on `link`, resolves the class from the answers and the port's USB id (null = unknown; a link knows none) and continues as that class on the same connection. The device owns the link; on failure it is closed.

#### DepzDevices.Attach

```csharp
public static DepzDevice Attach(DepzDevice dev, string className)
```

Continue the open plain device `dev` as `className` (a name `ResolveSensorClass` returns) on the same connection — no reopen; the reader and the host sequence numbers run on (contract 02 §4.4). Use the result from here on. "device", and a class this SDK has no device layer for yet, return `dev` itself.

#### DepzDevices.static

```csharp
public static (string SoftwareName, string DeviceName, string SerialNumber) Probe(DepzDevice dev)
```

The contract 02 §4.4 probe on an open connection: GET_NAME_ACTIVE_SOFTWARE (required: its failure throws), then GET_DEVICE_NAME and GET_SERIAL (a timeout or error leaves ""). Returns (software name, device name, serial number).

#### DepzDevices.cref

```csharp
#if NET /// <summary>Probe one port: open it, run <see cref="Probe"/>, close. Null when nothing DEPZ-shaped answers.</summary> public static DeviceInfo? ProbePort(string port, TimeSpan? timeout = null)
```

#### DepzDevices.ListDepzDevices

```csharp
public static IReadOnlyList<DeviceInfo> ListDepzDevices(IEnumerable<string>? ports = null, bool matchUsb = true, TimeSpan? timeout = null)
```

Probe the candidate ports and return every DEPZ device found, ordered by USB serial. With `matchUsb` (default) only ports with a known DEPZ USB id are probed; `ports` replaces enumeration with an explicit list.

#### DepzDevices.OpenDevice

```csharp
public static DepzDevice OpenDevice(string? serial = null, TimeSpan? timeout = null)
```

Open the DEPZ device with the smallest USB serial, or the one whose USB serial is `serial`, as its sensor class (contract 02 §4.3).

#### DepzDevices.OpenDevice

```csharp
public static DepzDevice OpenDevice(int index, TimeSpan? timeout = null)
```

Open the `index`-th DEPZ candidate, candidates sorted by USB serial.

#### DepzDevices.OpenPort

```csharp
public static DepzDevice OpenPort(string port, TimeSpan? timeout = null)
```

Open exactly `port` as its sensor class (also a port whose USB id is not a DEPZ one: an unprogrammed unit).

#### DepzDevices.OpenDevice

```csharp
public static DepzDevice OpenDevice(DeviceInfo info, TimeSpan? timeout = null)
```

Open the port of a `ListDepzDevices` result as the class its probe answers resolve to.

#### DepzDevices.OpenSr04

```csharp
public static Sr04Device OpenSr04(string? serial = null, TimeSpan? timeout = null)
```

`OpenDevice(string?, TimeSpan?)`, and `WrongTypeException` unless it is an SR04.

#### DepzDevices.OpenVl53l4cd

```csharp
public static Vl53l4cdDevice OpenVl53l4cd(string? serial = null, TimeSpan? timeout = null)
```

`OpenDevice(string?, TimeSpan?)`, and `WrongTypeException` unless it is a VL53L4CD.

#### DepzDevices.OpenVl53l8

```csharp
public static Vl53l8cxDevice OpenVl53l8(string? serial = null, TimeSpan? timeout = null)
```

`OpenDevice(string?, TimeSpan?)`, and `WrongTypeException` unless it is a VL53L8CX or VL53L8CH (`Vl53l8chDevice` derives from `Vl53l8cxDevice`).

#### DepzDevices.OpenVl53l7

```csharp
public static Vl53l7cxDevice OpenVl53l7(string? serial = null, TimeSpan? timeout = null)
```

`OpenDevice(string?, TimeSpan?)`, and `WrongTypeException` unless it is a VL53L5CX, VL53L7CX or VL53L7CH (all derive from `Vl53l7cxDevice`).

#### DepzDevices.OpenBno055

```csharp
public static Bno055Device OpenBno055(string? serial = null, TimeSpan? timeout = null)
```

`OpenDevice(string?, TimeSpan?)`, and `WrongTypeException` unless it is a BNO055.

#### DepzDevices.OpenBno086

```csharp
public static Bno086Device OpenBno086(string? serial = null, TimeSpan? timeout = null)
```

`OpenDevice(string?, TimeSpan?)`, and `WrongTypeException` unless it is a BNO085 / BNO086.

#### DepzDevices.OpenVl53lx

```csharp
public static Vl53lxDevice OpenVl53lx(string? serial = null, TimeSpan? timeout = null)
```

`OpenDevice(string?, TimeSpan?)`, and `WrongTypeException` unless it is a board of the VL53L 1D family (every class derives from `Vl53lxDevice`).

#### DepzDevices.OpenTmf8829

```csharp
public static Tmf8829Device OpenTmf8829(string? serial = null, TimeSpan? timeout = null)
```

`OpenDevice(string?, TimeSpan?)`, and `WrongTypeException` unless it is a TMF8829.

### DeviceEvent

```csharp
public abstract record DeviceEvent
```

Base of the unsolicited / diagnostic events a device emits.

### SequenceErrorEvent

```csharp
public sealed record SequenceErrorEvent(int ExpectedSeq, int ReceivedSeq, bool ReportedByDevice) : DeviceEvent
```

A gap in packet sequence numbers; `ReportedByDevice`: the board saw a gap in host TX (RPT_SEQUENCE_ERROR), else the host saw one in the board's packets.

### LinkCrcErrorEvent

```csharp
public sealed record LinkCrcErrorEvent(int Cmd, int Seq) : DeviceEvent
```

A packet whose payload CRC did not check.

### TrashEvent

```csharp
public sealed record TrashEvent(byte[] Data) : DeviceEvent
```

Bytes the parser skipped while looking for a packet header.

### UnsolicitedStatusEvent

```csharp
public sealed record UnsolicitedStatusEvent(int Status) : DeviceEvent
```

An RPT_STATUS no request was waiting for (echoed cmd 0x00, or nobody in flight).

#### UnsolicitedStatusEvent.IsHardwareFault *(property)*

```csharp
public bool IsHardwareFault
```

### TextEvent

```csharp
public sealed record TextEvent(int Cmd, string Text) : DeviceEvent
```

RPT_TEXT nobody asked for, or a report no class routes (its payload as hex).

### TemperatureEvent

```csharp
public sealed record TemperatureEvent(ulong TimestampUs, double Celsius) : DeviceEvent
```

An unsolicited MCU temperature report.

### DisconnectedEvent

```csharp
public sealed record DisconnectedEvent(string Reason) : DeviceEvent
```

The connection dropped (unplug, I/O error); the device is unusable from here on.

### TimeSyncResult

```csharp
public sealed record TimeSyncResult(long OffsetUs, long RttUs, long SyncedAtHostUs)
```

Result of `SyncTime`: device clock − host clock, the round trip of the kept sample, and when (host µs) it was taken.

### LinkStats

```csharp
public sealed class LinkStats
```

Link counters (contract 07 §2 `stats`); updated by the reader thread.

#### LinkStats.TxPackets *(property)*

```csharp
public long TxPackets
```

#### LinkStats.RxPackets *(property)*

```csharp
public long RxPackets
```

#### LinkStats.TxBytes *(property)*

```csharp
public long TxBytes
```

#### LinkStats.RxBytes *(property)*

```csharp
public long RxBytes
```

#### LinkStats.CrcErrors *(property)*

```csharp
public long CrcErrors
```

#### LinkStats.HeaderErrors *(property)*

```csharp
public long HeaderErrors
```

#### LinkStats.TrashBytes *(property)*

```csharp
public long TrashBytes
```

#### LinkStats.SeqGaps *(property)*

```csharp
public long SeqGaps
```

Gaps in the device → host sequence numbers, seen by the host.

#### LinkStats.DeviceSeqErrors *(property)*

```csharp
public long DeviceSeqErrors
```

RPT_SEQUENCE_ERROR reports: gaps the device saw in host → device packets.

### ReplyMatcher

```csharp
public delegate bool ReplyMatcher<T>(Packet packet, out T result)
```

Claims a packet as the reply to a request: return true and set `result`.

### DepzDevice

```csharp
public class DepzDevice : IDisposable
```

A connection to one DEPZ device in application mode (contracts 02, 07): a reader thread, replies correlated to requests by echoed opcode (one request in flight per opcode), events, and the common commands. Sensor classes derive from it. Requests block the calling thread up to `Timeout` (default 200 ms). Callbacks (`On…`) run on the reader thread: keep them short, never block, never close the device from one; in Unity use the `Subscription{T}` queues and poll them from `Update()`.

#### DepzDevice.DefaultTimeout *(field)*

```csharp
public static readonly TimeSpan DefaultTimeout = TimeSpan.FromMilliseconds(200)
```

The default request timeout, 200 ms (contract 07 §3).

#### DepzDevice.DepzDevice *(constructor)*

```csharp
public DepzDevice(ILink link, TimeSpan? timeout = null)
```

Open a connection on any link; the device owns it and closes it.

#### DepzDevice.name

```csharp
#if NET /// <summary>Open the serial port at <paramref name="port"/> (e.g. <c>/dev/ttyACM0</c>, <c>COM5</c>).</summary> public DepzDevice(string port, TimeSpan? timeout = null) : this(new SerialPortLink(port), timeout)
```

#### DepzDevice.Port *(property)*

```csharp
public string Port
```

The link's name: the port path, or what the link wraps.

#### DepzDevice.IsClosed *(property)*

```csharp
public bool IsClosed
```

#### DepzDevice.Timeout *(property)*

```csharp
public TimeSpan Timeout
```

The timeout of requests that take none of their own.

#### DepzDevice.TxCrcType *(property)*

```csharp
public CrcType TxCrcType
```

The payload CRC the host puts on its packets (contract 01).

#### DepzDevice.Stats *(property)*

```csharp
public LinkStats Stats
```

#### DepzDevice.Close

```csharp
public void Close()
```

Close the link and stop the reader; pending requests and streams end.

#### DepzDevice.Dispose

```csharp
public void Dispose()
```

#### DepzDevice.OnEvent

```csharp
public IDisposable OnEvent(Action<DeviceEvent> callback)
```

Call `callback` for every event (reader thread). Dispose the result to stop.

#### DepzDevice.Events

```csharp
public Subscription<DeviceEvent> Events(int capacity = 256)
```

A pull queue of events (bounded, drop-oldest); subscribed when this returns.

#### DepzDevice.Send

```csharp
public void Send(int cmd, byte[]? payload = null, CrcType? crcType = null)
```

Send a packet without waiting for anything (escape hatch; prefer a request).

#### DepzDevice.Request

```csharp
public T Request<T>(int cmd, byte[]? payload, ReplyMatcher<T> matcher, TimeSpan? timeout = null)
```

Send `cmd` and wait for the first packet `matcher` claims (escape hatch, contract 07 §2). A non-OK RPT_STATUS echoing `cmd` throws `StatusException` (`BusyException` for ERR_BUSY); one request per opcode may be in flight.

#### DepzDevice.RequestOk

```csharp
public void RequestOk(int cmd, byte[]? payload = null, TimeSpan? timeout = null)
```

Send `cmd` and wait for RPT_STATUS OK echoing it.

#### DepzDevice.ExpectReport

```csharp
public static ReplyMatcher<T> ExpectReport<T>(int reportId, Func<byte[], T> unpack)
```

A matcher for a report identified by its report id alone.

#### DepzDevice.ExpectText

```csharp
public static ReplyMatcher<string> ExpectText(int requestCmd)
```

A matcher for RPT_TEXT echoing `requestCmd`.

#### DepzDevice.GetDeviceName

```csharp
public string GetDeviceName()
```

#### DepzDevice.GetSoftwareName

```csharp
public string GetSoftwareName()
```

#### DepzDevice.GetSerialNumber

```csharp
public string GetSerialNumber()
```

#### DepzDevice.Identify

```csharp
public Identity Identify()
```

The running firmware: the software name parsed into mode, sensor type and version (contract 02 §4).

#### DepzDevice.ReadMcuTemperature

```csharp
public double ReadMcuTemperature()
```

The MCU temperature the device last cached, °C (it refreshes about twice a second).

#### DepzDevice.SyncTime

```csharp
public TimeSyncResult SyncTime(int samples = 5)
```

NTP-style time sync keeping the lowest-RTT of `samples` rounds (contract 02 §5).

#### DepzDevice.LastTimeSync *(property)*

```csharp
public TimeSyncResult? LastTimeSync
```

The last `SyncTime` result, or null.

#### DepzDevice.ToHostTimeUs

```csharp
public long ToHostTimeUs(long deviceTimestampUs)
```

Device µs → host monotonic µs (`HostNowUs`); needs a `SyncTime` first.

#### DepzDevice.GetReportPayloadCrc

```csharp
public CrcType GetReportPayloadCrc()
```

The device → host payload CRC mode.

### Subscription

```csharp
public sealed class Subscription<T> : IDisposable
```

A pull subscription to one stream (contract 07 §3): a bounded queue that drops the oldest item when full and counts the drops. It is registered when created, so nothing produced after that is missed. Poll it with `TryTake` (Unity: from `Update()`, never blocks) or wait with `Take`; `Dispose` unsubscribes.

#### Subscription.DroppedCount *(property)*

```csharp
public long DroppedCount
```

Items dropped because the queue was full (monotonic).

#### Subscription.Count *(property)*

```csharp
public int Count
```

Items waiting.

#### Subscription.IsClosed *(property)*

```csharp
public bool IsClosed
```

The device closed (or this was disposed): no more items will arrive.

#### Subscription.TryTake

```csharp
public bool TryTake(out T item)
```

The oldest waiting item, without blocking.

#### Subscription.Take

```csharp
public T Take(TimeSpan timeout)
```

The oldest item, waiting up to `timeout` for one.

#### Subscription.Drain

```csharp
public List<T> Drain()
```

Everything waiting now, oldest first, without blocking.

#### Subscription.Dispose

```csharp
public void Dispose()
```

### ILink

```csharp
public interface ILink
```

A blocking byte pipe to one device: a serial port, a `.depzrec` record/replay wrapper, or anything a host supplies (in Unity, a port opened with Unity's own System.IO.Ports). Device classes accept any link, which keeps every layer above the port testable without hardware.

### RecordingLink

```csharp
public sealed class RecordingLink : ILink
```

Wraps any link and tees both directions into a `.depzrec` file (contract 08). TX is journaled before the physical write: a fast device reply could otherwise reach the reader and be journaled ahead of its own request, which breaks the causal rx gating replay relies on.

#### RecordingLink.Schema *(constant)*

```csharp
public const string Schema = "depz.rec/1"
```

#### RecordingLink.RecordingLink *(constructor)*

```csharp
public RecordingLink(ILink inner, string path, IDictionary<string, object?>? headerExtra = null)
```

#### RecordingLink.Name *(property)*

```csharp
public string Name
```

#### RecordingLink.IsClosed *(property)*

```csharp
public bool IsClosed
```

#### RecordingLink.Read

```csharp
public byte[] Read(TimeSpan timeout)
```

#### RecordingLink.Write

```csharp
public void Write(byte[] data)
```

#### RecordingLink.Close

```csharp
public void Close()
```

### ReplayLink

```csharp
public sealed class ReplayLink : ILink
```

Replays the rx side of a `.depzrec` capture causally (contract 08): an rx chunk is served only after the host has written as many tx bytes as preceded it in the capture. With `strictTx` every written byte must also equal the recorded tx stream (`ReplayMismatchException`).

#### ReplayLink.Header *(property)*

```csharp
public IReadOnlyDictionary<string, object?> Header
```

The capture's header object (contract 08).

#### ReplayLink.ReplayLink *(constructor)*

```csharp
public ReplayLink(string path, bool strictTx = false, bool realtime = false)
```

#### ReplayLink.Name *(property)*

```csharp
public string Name
```

#### ReplayLink.IsClosed *(property)*

```csharp
public bool IsClosed
```

#### ReplayLink.RxExhausted *(property)*

```csharp
public bool RxExhausted
```

Every rx chunk was delivered.

#### ReplayLink.RxRemaining *(property)*

```csharp
public int RxRemaining
```

Recorded rx chunks not delivered yet.

#### ReplayLink.TxRemaining *(property)*

```csharp
public long TxRemaining
```

Recorded tx bytes the host has not written yet.

#### ReplayLink.Read

```csharp
public byte[] Read(TimeSpan timeout)
```

#### ReplayLink.Write

```csharp
public void Write(byte[] data)
```

#### ReplayLink.Close

```csharp
public void Close()
```

### SerialPortInfo

```csharp
public sealed record SerialPortInfo(string Port, int? Vid, int? Pid, string? UsbSerial)
```

One serial port and its USB identity, where the OS tells it.

### SerialPorts

```csharp
public static class SerialPorts
```

Serial port enumeration with USB ids: Linux (sysfs) and Windows (the registry). On macOS the ports are listed without USB ids, so discovery finds no DEPZ candidate there: open a port path explicitly.

#### SerialPorts.List

```csharp
public static IReadOnlyList<SerialPortInfo> List()
```

Every serial port the OS lists.

#### SerialPorts.BySerial

```csharp
public static IEnumerable<SerialPortInfo> BySerial(IEnumerable<SerialPortInfo> ports)
```

Sorted by USB serial (none last), then by port path (contract 02 §4.3).

### SerialPortLink

```csharp
public sealed class SerialPortLink : ILink
```

A serial port as an `ILink` (System.IO.Ports). DTR and RTS are raised on open, as the reference SDK's pyserial does: a CDC device may hold its output until the host asserts DTR.

#### SerialPortLink.SerialPortLink *(constructor)*

```csharp
public SerialPortLink(string port, int baudRate = 115200)
```

#### SerialPortLink.Name *(property)*

```csharp
public string Name
```

#### SerialPortLink.IsClosed *(property)*

```csharp
public bool IsClosed
```

#### SerialPortLink.Read

```csharp
public byte[] Read(TimeSpan timeout)
```

#### SerialPortLink.Write

```csharp
public void Write(byte[] data)
```

#### SerialPortLink.Close

```csharp
public void Close()
```

### DepzException

```csharp
public class DepzException : Exception
```

Base of every live-layer error (contract 07). Arguments the SDK refuses before sending anything throw `ArgumentException` instead.

#### DepzException.DepzException *(constructor)*

```csharp
public DepzException(string message) : base(message)
```

#### DepzException.DepzException *(constructor)*

```csharp
public DepzException(string message, Exception inner) : base(message, inner)
```

### DepzTimeoutException

```csharp
public class DepzTimeoutException : DepzException
```

No reply to a request within its timeout (default 200 ms).

#### DepzTimeoutException.Cmd *(property)*

```csharp
public int Cmd
```

The request opcode that went unanswered.

#### DepzTimeoutException.DepzTimeoutException *(constructor)*

```csharp
public DepzTimeoutException(int cmd, TimeSpan timeout) : base($"no reply to cmd 0x{cmd:X2} within {timeout.TotalSeconds:0.000}s")
```

#### DepzTimeoutException.DepzTimeoutException *(constructor)*

```csharp
public DepzTimeoutException(string message) : base(message)
```

### StatusException

```csharp
public class StatusException : DepzException
```

The device answered a request with a non-OK RPT_STATUS (contract 02 §3).

#### StatusException.Cmd *(property)*

```csharp
public int Cmd
```

#### StatusException.Status *(property)*

```csharp
public int Status
```

#### StatusException.StatusName *(property)*

```csharp
public string StatusName
```

The contract 02 §3 name of `Status`, e.g. `ERR_INVALID_PARAM`.

#### StatusException.StatusException *(constructor)*

```csharp
public StatusException(int cmd, int status) : this(cmd, status, $"cmd 0x{cmd:X2} failed: {NameOf(status)} (0x{status:X2})")
```

### BusyException

```csharp
public sealed class BusyException : StatusException
```

ERR_BUSY from the device, or a request whose opcode is already in flight.

#### BusyException.BusyException *(constructor)*

```csharp
public BusyException(int cmd) : base(cmd, (int)Protocol.Status.ErrBusy, $"cmd 0x{cmd:X2}: busy (ERR_BUSY or already in flight)")
```

### DeviceLostException

```csharp
public class DeviceLostException : DepzException
```

The connection went away: unplugged, closed, or the OS refused I/O.

#### DeviceLostException.DeviceLostException *(constructor)*

```csharp
public DeviceLostException(string message) : base(message)
```

#### DeviceLostException.DeviceLostException *(constructor)*

```csharp
public DeviceLostException(string message, Exception inner) : base(message, inner)
```

### LinkClosedException

```csharp
public sealed class LinkClosedException : DeviceLostException
```

I/O on a link that is already closed.

#### LinkClosedException.LinkClosedException *(constructor)*

```csharp
public LinkClosedException(string message) : base(message)
```

### NoDepzDeviceException

```csharp
public sealed class NoDepzDeviceException : DepzException
```

Discovery found no DEPZ device to open.

#### NoDepzDeviceException.NoDepzDeviceException *(constructor)*

```csharp
public NoDepzDeviceException(string message) : base(message)
```

#### NoDepzDeviceException.NoDepzDeviceException *(constructor)*

```csharp
public NoDepzDeviceException(string message, Exception inner) : base(message, inner)
```

### BootloaderModeException

```csharp
public sealed class BootloaderModeException : DepzException
```

The device is in bootloader mode: use the contract 06 client, not a sensor class.

#### BootloaderModeException.BootloaderModeException *(constructor)*

```csharp
public BootloaderModeException() : base("device is in bootloader mode; use the bootloader client (contract 06)")
```

### WrongTypeException

```csharp
public sealed class WrongTypeException : DepzException
```

The opened device is not the sensor class the caller asked for.

#### WrongTypeException.WrongTypeException *(constructor)*

```csharp
public WrongTypeException(string message) : base(message)
```

### ReplayMismatchException

```csharp
public sealed class ReplayMismatchException : DepzException
```

A `ReplayLink` in strict mode saw the SDK write bytes the capture does not hold.

#### ReplayMismatchException.ReplayMismatchException *(constructor)*

```csharp
public ReplayMismatchException(string message) : base(message)
```

## USB identity

### UsbIds

```csharp
public static class UsbIds
```

DEPZ USB identity table (contracts/02_COMMON_COMMANDS.md §4).

Used by discovery to pick the right serial port without poking unrelated devices; the protocol probe (GET_NAME_ACTIVE_SOFTWARE) remains the source of truth for what a device actually is. The PID→model map is an informational hint only.

#### UsbIds.DepzUsbVid *(constant)*

```csharp
public const int DepzUsbVid = 0x1BCF
```

Production VID shared by every DEPZ sensor.

#### UsbIds.PidSr04 *(constant)*

```csharp
public const int PidSr04 = 0xEC78
```

#### UsbIds.PidVl53l8ch *(constant)*

```csharp
public const int PidVl53l8ch = 0xED40
```

#### UsbIds.PidVl53l8cx *(constant)*

```csharp
public const int PidVl53l8cx = 0xED4B
```

#### UsbIds.PidVl53l4cd *(constant)*

```csharp
public const int PidVl53l4cd = 0xED45
```

#### UsbIds.PidTmf8829 *(constant)*

```csharp
public const int PidTmf8829 = 0xED4C
```

#### UsbIds.PidBno086 *(constant)*

```csharp
public const int PidBno086 = 0xEE08
```

#### UsbIds.DepzPidModel *(field)*

```csharp
public static readonly IReadOnlyDictionary<int, string> DepzPidModel = …
```

PID → sensor-model hint. Informational: the protocol probe is authoritative.

#### UsbIds.DepzPidRangeLo *(constant)*

```csharp
public const int DepzPidRangeLo = 60536
```

Inclusive PID range: any PID here under `DepzUsbVid` is a candidate DEPZ sensor even if not individually mapped (the whole reserved sensor block 60536..65535).

#### UsbIds.DepzPidRangeHi *(constant)*

```csharp
public const int DepzPidRangeHi = 65535
```

#### UsbIds.DevUsbVid *(constant)*

```csharp
public const int DevUsbVid = 0x0483
```

Dev / unprogrammed default: STMicroelectronics VID/PID.

#### UsbIds.DevUsbPid *(constant)*

```csharp
public const int DevUsbPid = 0x56DC
```

#### UsbIds.IsKnownDepzUsb

```csharp
public static bool IsKnownDepzUsb(int? vid, int? pid)
```

True when (vid, pid) is a recognized DEPZ (or dev-default) USB id.

#### UsbIds.UsbModelHint

```csharp
public static string? UsbModelHint(int? vid, int? pid)
```

Best-guess model name for a (vid, pid), or null. Informational only.

#### UsbIds.PortCandidate

```csharp
public sealed record PortCandidate(string Port, string? Serial)
```

A discovered serial port with its USB iSerial (null/empty when absent).

#### UsbIds.SerialOrdering

```csharp
public static IReadOnlyList<PortCandidate> SerialOrdering(IEnumerable<PortCandidate> ports)
```

Deterministic discovery order (contract 02 §4): sort candidate ports by USB iSerial ascending (ordinal), ports with no serial (null or empty) last, tie-broken by port path (ordinal). Stable, non-mutating.

### Discovery

```csharp
public static class Discovery
```

Class resolution (contracts/02_COMMON_COMMANDS.md §4.4, Python reference `resolve_sensor_class`): which SDK class a device opens as, from its probe answers and its port's USB (vid, pid). This SDK is decode-only (no device layer), so it carries the pure part of §4.4 only.

#### Discovery.SensorClassNames *(field)*

```csharp
public static readonly IReadOnlyList<string> SensorClassNames = …
```

Every name `ResolveSensorClass` returns: the contract 07 sensor classes, `"vl53lx"` (the generic 1D-family class: an unstamped board, or a VL53L4CD on the APP_VL53L0_4 firmware), `"tmf8829"`, `"device"` (the base device: an unknown application, or a name that is neither APP_ nor BOOTDEPZ) and `"bootloader"` (use the contract 06 client). BNO085 boards run APP_BNO086 and resolve to `"bno086"`.

#### Discovery.ResolveSensorClass

```csharp
public static string ResolveSensorClass(string softwareName, string? deviceName, int? usbVid, int? usbPid)
```

The canonical class name (one of `SensorClassNames`) for a device's probe answers and its port's USB id — contract 02 §4.4, pure. `softwareName` is the GET_NAME_ACTIVE_SOFTWARE answer, `deviceName` the GET_DEVICE_NAME answer (`""`/null when none); the USB id is null when unknown (a replay, a bridge). BOOTDEPZ_* → `"bootloader"`; vl53l8 → `"vl53l8ch"` if the USB model is VL53L8CH, else `"vl53l8cx"`; vl53l7 → `ResolveModel`; vl53lx → `ResolveClass` (VL53L4CD on that firmware is the generic `"vl53lx"`); any other known token → its class; anything else → `"device"`.

## SR04

### Sr04Measurement

```csharp
public sealed record Sr04Measurement(ulong TimestampUs, int EchoTimeUs, string Source)
```

One SR04 ranging result. `Source` is `"once"` (a host command or a SYNC_IN edge) or `"loop"`; `Valid` is false for the no-echo timeout.

#### Sr04Measurement.Valid *(property)*

```csharp
public bool Valid
```

#### Sr04Measurement.DistanceMm *(property)*

```csharp
public double? DistanceMm
```

Distance at 343 m/s, or null when no echo came back.

#### Sr04Measurement.DistanceMmAt

```csharp
public double? DistanceMmAt(double airTempC)
```

Distance with the speed of sound for `airTempC` °C.

### Sr04Device

```csharp
public sealed class Sr04Device : DepzDevice
```

HC-SR04 ultrasonic ranger (contract 03). Measurements stream to `OnMeasurement` callbacks and `Measurements` queues: loop samples and the single shots an AUX SYNC_IN edge triggers.

#### Sr04Device.Sr04Device *(constructor)*

```csharp
public Sr04Device(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

#### Sr04Device.Sr04Device *(constructor)*

```csharp
#if NET public Sr04Device(string port, TimeSpan? timeout = null) : base(port, timeout)
```

#### Sr04Device.GetSamplePeriodUs

```csharp
public long GetSamplePeriodUs()
```

The stored minimum interval between measurement starts, µs (default 50000) — not the effective rate, which the echo window throttles (contract 03 §3).

#### Sr04Device.SetSamplePeriodUs

```csharp
public void SetSamplePeriodUs(long periodUs)
```

Set the minimum interval between measurement starts, µs (u32 wire field).

#### Sr04Device.GetEchoDecayUs

```csharp
public int GetEchoDecayUs()
```

The post-echo settle pause, µs (4000–65000).

#### Sr04Device.SetEchoDecayUs

```csharp
public int SetEchoDecayUs(int decayUs)
```

Set the settle pause. The device clamps it to 4000–65000 µs silently, so this reads it back and returns the value in effect. Must fit the u16 wire field.

#### Sr04Device.MeasureOnce

```csharp
public Sr04Measurement MeasureOnce(TimeSpan? timeout = null)
```

One shot. The reply comes when the echo completes (or times out at ~65.5 ms), hence the 1 s default. `BusyException` while the loop runs.

#### Sr04Device.Start

```csharp
public void Start()
```

Start the measurement loop (idempotent).

#### Sr04Device.Stop

```csharp
public void Stop()
```

Stop the measurement loop (idempotent).

#### Sr04Device.OnMeasurement

```csharp
public IDisposable OnMeasurement(Action<Sr04Measurement> callback)
```

Call `callback` for every measurement (reader thread). Dispose the result to stop.

#### Sr04Device.Measurements

```csharp
public Subscription<Sr04Measurement> Measurements(int capacity = 256)
```

A pull queue of measurements (bounded, drop-oldest); subscribed when this returns.

#### Sr04Device.StreamDroppedCounts *(property)*

```csharp
public IReadOnlyList<long> StreamDroppedCounts
```

The drop counter of each open `Measurements` queue.

### Sr04Cmd

```csharp
public enum Sr04Cmd
{
    GetSamplePeriod = 0x32,
    SetSamplePeriod = 0x33,
    GetEchoDecay = 0x34,
    SetEchoDecay = 0x35,
    MeasureOnce = 0x36,
    StartMeasurementLoop = 0x37,
    StopMeasurementLoop = 0x38,
}
```

SR04 command opcodes (contracts/03_SENSOR_SR04.md).

### Sr04Rpt

```csharp
public enum Sr04Rpt
{
    Data = 0x91,
    SamplePeriod = 0x92,
    EchoDecay = 0x93,
}
```

SR04 report opcodes.

### Sr04Data

```csharp
public sealed record Sr04Data(int SourceCmd, ulong TimestampUs, int EchoTimeUs)
```

SR04 measurement sample: source cmd (0x36 single / 0x37 loop), timestamp, echo time.

#### Sr04Data.Unpack

```csharp
public static Sr04Data Unpack(ReadOnlySpan<byte> payload)
```

### Sr04

```csharp
public static class Sr04
```

#### Sr04.EchoTimeout *(constant)*

```csharp
public const int EchoTimeout = 0xFFFF
```

echo_time_us sentinel: no echo received.

#### Sr04.SamplePeriodDefaultUs *(constant)*

```csharp
public const int SamplePeriodDefaultUs = 50_000
```

#### Sr04.EchoDecayDefaultUs *(constant)*

```csharp
public const int EchoDecayDefaultUs = 5_000
```

#### Sr04.EchoDecayMinUs *(constant)*

```csharp
public const int EchoDecayMinUs = 4_000
```

#### Sr04.EchoDecayMaxUs *(constant)*

```csharp
public const int EchoDecayMaxUs = 65_000
```

#### Sr04.PackSamplePeriod

```csharp
public static byte[] PackSamplePeriod(uint periodUs)
```

#### Sr04.UnpackSamplePeriod

```csharp
public static uint UnpackSamplePeriod(ReadOnlySpan<byte> payload)
```

#### Sr04.PackEchoDecay

```csharp
public static byte[] PackEchoDecay(ushort decayUs)
```

#### Sr04.UnpackEchoDecay

```csharp
public static ushort UnpackEchoDecay(ReadOnlySpan<byte> payload)
```

#### Sr04.DistanceMmFromEcho

```csharp
public static double? DistanceMmFromEcho(int echoTimeUs, double? airTempC = null)
```

Round-trip echo time → distance in mm; null for the timeout sentinel. Default speed of sound 343 m/s; with `airTempC` uses c = 331.3 + 0.606·T (m/s).

## VL53L4CD (ToF)

### Vl53l4Measurement

```csharp
public sealed record Vl53l4Measurement(ulong TimestampUs, int RangeStatus, int DistanceMm, int SigmaMm, int SignalRateKcps, int AmbientRateKcps, int SignalPerSpadKcps, int AmbientPerSpadKcps, int NumberOfSpad, int StreamCount)
```

One VL53L4CD ranging result (VL53L4CD_ResultsData_t plus the MCU timestamp: of the INT edge when streamed, of the result read when polled).

#### Vl53l4Measurement.Valid *(property)*

```csharp
public bool Valid
```

#### Vl53l4Measurement.StatusText *(property)*

```csharp
public string StatusText
```

What `RangeStatus` means (`RangeStatusNames`).

### Vl53l4cdDevice

```csharp
public sealed class Vl53l4cdDevice : DepzDevice
```

VL53L4CD single-zone ToF (contract 10). The board is a thin I2C bridge; the ST ULD 2.2.3 runs here (`Uld`). `Init` first, then configure and `StartRanging`: one result block per INT edge streams to `Measurements`. Configuration while ranging throws `InvalidOperationException` — the stream owns the register bank.

#### Vl53l4cdDevice.Vl53l4cdDevice *(constructor)*

```csharp
public Vl53l4cdDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

#### Vl53l4cdDevice.Vl53l4cdDevice *(constructor)*

```csharp
#if NET public Vl53l4cdDevice(string port, TimeSpan? timeout = null) : base(port, timeout)
```

#### Vl53l4cdDevice.Uld *(property)*

```csharp
public Vl53l4cdDriver Uld
```

The ULD driver (escape hatch for raw register access).

#### Vl53l4cdDevice.Initialized *(property)*

```csharp
public bool Initialized
```

True after a successful `Init`; cleared by `Xshut` — a power-cycled sensor holds none of the ULD configuration.

#### Vl53l4cdDevice.Ranging *(property)*

```csharp
public bool Ranging
```

#### Vl53l4cdDevice.IsAlive

```csharp
public bool IsAlive()
```

True when the sensor answers with the VL53L4CD model id (0xEBAA).

#### Vl53l4cdDevice.Init

```csharp
public void Init(int busKhz = Vl53l4Uld.I2cKhzDefault)
```

Default configuration block + VHV calibration (ULD sensor_init); the bus is left at `busKhz` (one of `I2cKhzSteps`).

#### Vl53l4cdDevice.Xshut

```csharp
public void Xshut(byte action)
```

Drive XSHUT: `Off` / `On` / `Reset`. Off and Reset stop a stream; `Init` again after.

#### Vl53l4cdDevice.ResetSensor

```csharp
public void ResetSensor()
```

Hardware reset through XSHUT (~3 ms on the MCU); call `Init` again.

#### Vl53l4cdDevice.BridgeInfo

```csharp
public Vl53l4Info BridgeInfo()
```

RPT_VL53_INFO: sensor identity, pin levels and free-running bridge counters. Safe while streaming.

#### Vl53l4cdDevice.SetI2cSpeedKhz

```csharp
public void SetI2cSpeedKhz(int khz)
```

Re-time the bridge's I2C bus to the step nearest `khz`. Not while ranging.

#### Vl53l4cdDevice.public

```csharp
public (int TimingBudgetMs, int InterMeasurementMs) GetRangeTiming()
```

(timing budget ms, inter-measurement ms; 0 = continuous).

#### Vl53l4cdDevice.SetRangeTiming

```csharp
public void SetRangeTiming(int timingBudgetMs, long interMeasurementMs = 0)
```

Timing budget 10–200 ms; inter-measurement 0 = continuous, larger than the budget = autonomous low power.

#### Vl53l4cdDevice.GetOffsetMm

```csharp
public int GetOffsetMm()
```

#### Vl53l4cdDevice.SetOffsetMm

```csharp
public void SetOffsetMm(int offsetMm)
```

#### Vl53l4cdDevice.GetXtalkKcps

```csharp
public int GetXtalkKcps()
```

Crosstalk compensation, kcps (0 = off).

#### Vl53l4cdDevice.SetXtalkKcps

```csharp
public void SetXtalkKcps(int xtalkKcps)
```

#### Vl53l4cdDevice.public

```csharp
public (int DistanceLowMm, int DistanceHighMm, int Window) GetDetectionThresholds()
```

(distance low mm, distance high mm, window: `WindowBelow` …).

#### Vl53l4cdDevice.SetDetectionThresholds

```csharp
public void SetDetectionThresholds(int distanceLowMm, int distanceHighMm, int window)
```

The distance-window interrupt: INT only fires while the window holds.

#### Vl53l4cdDevice.GetSignalThresholdKcps

```csharp
public int GetSignalThresholdKcps()
```

#### Vl53l4cdDevice.SetSignalThresholdKcps

```csharp
public void SetSignalThresholdKcps(int signalKcps)
```

Drop measurements whose return signal is below `signalKcps`.

#### Vl53l4cdDevice.GetSigmaThresholdMm

```csharp
public int GetSigmaThresholdMm()
```

#### Vl53l4cdDevice.SetSigmaThresholdMm

```csharp
public void SetSigmaThresholdMm(int sigmaMm)
```

Drop measurements whose sigma exceeds `sigmaMm` (≤ 16383).

#### Vl53l4cdDevice.StartTemperatureUpdate

```csharp
public void StartTemperatureUpdate()
```

Re-run VHV calibration, recommended after a >8 °C ambient change.

#### Vl53l4cdDevice.CalibrateOffset

```csharp
public int CalibrateOffset(int targetDistMm, int nbSamples = 20)
```

Offset calibration against a target at `targetDistMm` (10–1000 mm); returns the offset now programmed.

#### Vl53l4cdDevice.CalibrateXtalk

```csharp
public int CalibrateXtalk(int targetDistMm, int nbSamples = 20)
```

Crosstalk calibration against a target at `targetDistMm` (10–5000 mm); returns the xtalk now programmed, kcps.

#### Vl53l4cdDevice.StartRanging

```csharp
public void StartRanging()
```

Start the sensor and arm the MCU stream: one RPT_VL53_STREAM per INT edge.

#### Vl53l4cdDevice.StopRanging

```csharp
public void StopRanging()
```

#### Vl53l4cdDevice.MeasureOnce

```csharp
public Vl53l4Measurement MeasureOnce(TimeSpan? timeout = null)
```

One polled measurement: start, wait for data ready, read, stop. Not while streaming.

#### Vl53l4cdDevice.OnMeasurement

```csharp
public IDisposable OnMeasurement(Action<Vl53l4Measurement> callback)
```

Call `callback` for every streamed measurement (reader thread).

#### Vl53l4cdDevice.Measurements

```csharp
public Subscription<Vl53l4Measurement> Measurements(int capacity = 64)
```

A pull queue of streamed measurements (bounded, drop-oldest).

#### Vl53l4cdDevice.GetMeasurement

```csharp
public Vl53l4Measurement GetMeasurement(TimeSpan? timeout = null)
```

The next streamed measurement (contract 07 §3).

#### Vl53l4cdDevice.StreamParseErrors *(property)*

```csharp
public long StreamParseErrors
```

Stream reports dropped because their result block did not decode.

#### Vl53l4cdDevice.StreamDroppedCounts *(property)*

```csharp
public IReadOnlyList<long> StreamDroppedCounts
```

### Vl53l4Cmd

```csharp
public enum Vl53l4Cmd
{
    ReadReg = 0x32,
    WriteReg = 0x33,
    Xshut = 0x34,
    StartStream = 0x35,
    StopStream = 0x36,
    GetInfo = 0x37,
    SetI2cSpeed = 0x38,
}
```

VL53L4CD register-bridge command opcodes (contracts/10_SENSOR_VL53L4.md).

### Vl53l4Rpt

```csharp
public enum Vl53l4Rpt
{
    RegData = 0x91,
    Info = 0x92,
    Stream = 0x93,
}
```

VL53L4CD report opcodes.

### Vl53l4Xshut

```csharp
public static class Vl53l4Xshut
```

VL53_XSHUT actions. What `Reset` does depends on the bridge: on the VL53L4CD bridge (contract 10) it is blocking on the MCU and is answered only after the sensor's boot handshake (allow ≥ 1.5 s); on the 1D-family bridge v2.00 (contract 12 §2, `PackXshut`) it is 1 ms low plus a fixed 5 ms wait with no handshake, and the host polls the boot register itself.

#### Vl53l4Xshut.Off *(constant)*

```csharp
public const byte Off = 0
```

#### Vl53l4Xshut.On *(constant)*

```csharp
public const byte On = 1
```

#### Vl53l4Xshut.Reset *(constant)*

```csharp
public const byte Reset = 2
```

### Vl53l4Wire

```csharp
public static class Vl53l4Wire
```

VL53L4CD wire limits and command payload codecs (contract 10 §1–§3). All wire fields (`addr`, `len`, …) are little-endian; register contents are big-endian and pass through the bridge untouched.

#### Vl53l4Wire.XferMax *(constant)*

```csharp
public const int XferMax = 253
```

Max read length / write data length per transfer (the STM32 I2C NBYTES field is 8-bit and a write spends two bytes on the register address; the firmware applies the same limit to both directions).

#### Vl53l4Wire.FlagIntActHigh *(constant)*

```csharp
public const byte FlagIntActHigh = 0x02
```

VL53_START_STREAM flags bit 1: INT active high, mirroring bit 4 of GPIO_HV_MUX__CTRL (0x0030). Clear (default): INT active low.

#### Vl53l4Wire.I2cKhzSteps *(property)*

```csharp
public static ReadOnlySpan<int> I2cKhzSteps
```

Nominal SCL steps the firmware carries a TIMINGR for (VL53_SET_I2C_SPEED clamps to the nearest one).

#### Vl53l4Wire.PackReadReg

```csharp
public static byte[] PackReadReg(ushort addr, ushort len)
```

VL53_READ_REG payload: addr u16 LE, len u16 LE.

#### Vl53l4Wire.PackWriteReg

```csharp
public static byte[] PackWriteReg(ushort addr, ReadOnlySpan<byte> data)
```

VL53_WRITE_REG payload: addr u16 LE, then the raw register data (1..`XferMax` bytes).

#### Vl53l4Wire.PackXshut

```csharp
public static byte[] PackXshut(byte action)
```

VL53_XSHUT payload: action u8 (`Vl53l4Xshut`).

#### Vl53l4Wire.PackStartStream

```csharp
public static byte[] PackStartStream(ushort addr, ushort len, byte flags = 0)
```

VL53_START_STREAM payload: addr u16 LE, len u16 LE, flags u8.

#### Vl53l4Wire.PackSetI2cSpeed

```csharp
public static byte[] PackSetI2cSpeed(ushort khz)
```

VL53_SET_I2C_SPEED payload: khz u16 LE.

### Vl53l4RegData

```csharp
public sealed record Vl53l4RegData(byte Cmd, ulong TimestampUs, byte[] Data)
```

RPT_VL53_REG_DATA — one register read. `Cmd` echoes the READ_REG opcode (0x32); `TimestampUs` is MCU uptime at I2C-read completion.

#### Vl53l4RegData.Unpack

```csharp
public static Vl53l4RegData Unpack(ReadOnlySpan<byte> payload)
```

Parse a RPT_VL53_REG_DATA payload: cmd u8, timestamp u64 LE, then data.

### Vl53l4Info

```csharp
public sealed record Vl53l4Info(uint IntEdges, uint SlotsSkipped, uint I2cErrors, byte LastI2cError, ushort ModelId, byte FwStatus, byte Initialized, byte XshutLevel, byte IntLevel, ushort I2cKhz)
```

RPT_VL53_INFO — bridge diagnostics (21-byte payload). Counters are free-running and wrap silently; watch increments, not absolute values. `ModelId` expected 0xEBAA, `FwStatus` expected 0x03 (booted). `LastI2cError`: 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR.

#### Vl53l4Info.Unpack

```csharp
public static Vl53l4Info Unpack(ReadOnlySpan<byte> payload)
```

Parse a RPT_VL53_INFO payload (u32 u32 u32 u8 u16 u8 u8 u8 u8 u16, all LE).

### Vl53l4StreamData

```csharp
public sealed record Vl53l4StreamData(ulong TimestampUs, ushort Addr, ushort Len, byte[] Data)
```

RPT_VL53_STREAM — one streamed register block. `TimestampUs` is MCU uptime at the INT edge (the sensor event, not the I2C completion); `Addr`/`Len` echo the stream configuration so each report is self-describing.

#### Vl53l4StreamData.Unpack

```csharp
public static Vl53l4StreamData Unpack(ReadOnlySpan<byte> payload)
```

Parse a RPT_VL53_STREAM payload: ts u64 LE, addr u16 LE, len u16 LE, data[len].

### Vl53l4Exception

```csharp
public sealed class Vl53l4Exception : Exception
```

Raised on invalid VL53L4CD ULD inputs (short result block, out-of-range timing…).

#### Vl53l4Exception.Vl53l4Exception *(constructor)*

```csharp
public Vl53l4Exception(string message) : base(message)
```

### Vl53l4Results

```csharp
public sealed record Vl53l4Results(int RangeStatus, int DistanceMm, int AmbientRateKcps, int AmbientPerSpadKcps, int SignalRateKcps, int SignalPerSpadKcps, int NumberOfSpad, int SigmaMm, int StreamCount)
```

VL53L4CD_ResultsData_t plus the sensor's own frame counter. `StreamCount` (RESULT__STREAM_COUNT, wraps at 255) tells a frame the host never received from one the sensor never produced.

### Vl53l4Uld

```csharp
public static class Vl53l4Uld
```

VL53L4CD ULD codec/math layer (ST STSW-IMG026 2.2.3, contract 10 §5): the pure register-word codecs of `VL53L4CD_api.c` — result-block decode, SetRangeTiming/GetRangeTiming register math, tuning-word codecs and the init configuration block — ported byte-exact from the reference Python `vl53l4/uld.py` and pinned to the golden vectors.

The register sequences on top of it (init, ranging, calibration) are `Vl53l4cdDriver`; `Vl53l4cdDevice` runs them over the board.

#### Vl53l4Uld.SoftReset *(constant)*

```csharp
public const ushort SoftReset = 0x0000
```

#### Vl53l4Uld.I2cSlaveDeviceAddress *(constant)*

```csharp
public const ushort I2cSlaveDeviceAddress = 0x0001
```

#### Vl53l4Uld.OscFrequency *(constant)*

```csharp
public const ushort OscFrequency = 0x0006
```

Oscillator-frequency word (unnamed register 0x0006 in the C driver).

#### Vl53l4Uld.VhvConfigTimeoutMacropLoopBound *(constant)*

```csharp
public const ushort VhvConfigTimeoutMacropLoopBound = 0x0008
```

#### Vl53l4Uld.XtalkPlaneOffsetKcps *(constant)*

```csharp
public const ushort XtalkPlaneOffsetKcps = 0x0016
```

#### Vl53l4Uld.XtalkXPlaneGradientKcps *(constant)*

```csharp
public const ushort XtalkXPlaneGradientKcps = 0x0018
```

#### Vl53l4Uld.XtalkYPlaneGradientKcps *(constant)*

```csharp
public const ushort XtalkYPlaneGradientKcps = 0x001A
```

#### Vl53l4Uld.RangeOffsetMm *(constant)*

```csharp
public const ushort RangeOffsetMm = 0x001E
```

#### Vl53l4Uld.InnerOffsetMm *(constant)*

```csharp
public const ushort InnerOffsetMm = 0x0020
```

#### Vl53l4Uld.OuterOffsetMm *(constant)*

```csharp
public const ushort OuterOffsetMm = 0x0022
```

#### Vl53l4Uld.GpioHvMuxCtrl *(constant)*

```csharp
public const ushort GpioHvMuxCtrl = 0x0030
```

#### Vl53l4Uld.GpioTioHvStatus *(constant)*

```csharp
public const ushort GpioTioHvStatus = 0x0031
```

#### Vl53l4Uld.SystemInterrupt *(constant)*

```csharp
public const ushort SystemInterrupt = 0x0046
```

#### Vl53l4Uld.RangeConfigA *(constant)*

```csharp
public const ushort RangeConfigA = 0x005E
```

#### Vl53l4Uld.RangeConfigB *(constant)*

```csharp
public const ushort RangeConfigB = 0x0061
```

#### Vl53l4Uld.RangeConfigSigmaThresh *(constant)*

```csharp
public const ushort RangeConfigSigmaThresh = 0x0064
```

#### Vl53l4Uld.MinCountRateRtnLimitMcps *(constant)*

```csharp
public const ushort MinCountRateRtnLimitMcps = 0x0066
```

#### Vl53l4Uld.IntermeasurementMs *(constant)*

```csharp
public const ushort IntermeasurementMs = 0x006C
```

#### Vl53l4Uld.ThreshHigh *(constant)*

```csharp
public const ushort ThreshHigh = 0x0072
```

#### Vl53l4Uld.ThreshLow *(constant)*

```csharp
public const ushort ThreshLow = 0x0074
```

#### Vl53l4Uld.SystemInterruptClear *(constant)*

```csharp
public const ushort SystemInterruptClear = 0x0086
```

#### Vl53l4Uld.SystemStart *(constant)*

```csharp
public const ushort SystemStart = 0x0087
```

#### Vl53l4Uld.ResultRangeStatus *(constant)*

```csharp
public const ushort ResultRangeStatus = 0x0089
```

#### Vl53l4Uld.ResultSpadNb *(constant)*

```csharp
public const ushort ResultSpadNb = 0x008C
```

#### Vl53l4Uld.ResultSignalRate *(constant)*

```csharp
public const ushort ResultSignalRate = 0x008E
```

#### Vl53l4Uld.ResultAmbientRate *(constant)*

```csharp
public const ushort ResultAmbientRate = 0x0090
```

#### Vl53l4Uld.ResultSigma *(constant)*

```csharp
public const ushort ResultSigma = 0x0092
```

#### Vl53l4Uld.ResultDistance *(constant)*

```csharp
public const ushort ResultDistance = 0x0096
```

#### Vl53l4Uld.ResultOscCalibrateVal *(constant)*

```csharp
public const ushort ResultOscCalibrateVal = 0x00DE
```

#### Vl53l4Uld.FirmwareSystemStatus *(constant)*

```csharp
public const ushort FirmwareSystemStatus = 0x00E5
```

#### Vl53l4Uld.IdentificationModelId *(constant)*

```csharp
public const ushort IdentificationModelId = 0x010F
```

#### Vl53l4Uld.ModelId *(constant)*

```csharp
public const ushort ModelId = 0xEBAA
```

IDENTIFICATION__MODEL_ID word expected for VL53L4CD silicon.

#### Vl53l4Uld.WindowBelow *(constant)*

```csharp
public const int WindowBelow = 0
```

#### Vl53l4Uld.WindowAbove *(constant)*

```csharp
public const int WindowAbove = 1
```

#### Vl53l4Uld.WindowOut *(constant)*

```csharp
public const int WindowOut = 2
```

#### Vl53l4Uld.WindowIn *(constant)*

```csharp
public const int WindowIn = 3
```

#### Vl53l4Uld.ConfigAddr *(constant)*

```csharp
public const ushort ConfigAddr = 0x002D
```

First register of the init configuration block (0x2D..0x87).

#### Vl53l4Uld.ConfigEnd *(constant)*

```csharp
public const ushort ConfigEnd = 0x0087
```

Last register of the init configuration block.

#### Vl53l4Uld.ConfigFmpByte *(constant)*

```csharp
public const byte ConfigFmpByte = 0x12
```

Byte 0 of `ConfigBlock` (register 0x2D): I2C pad in Fast Mode Plus — set unconditionally and never cleared (FM+ pads work at every bus step down to 100 kHz).

#### Vl53l4Uld.ResultBlockAddr *(constant)*

```csharp
public const ushort ResultBlockAddr = ResultRangeStatus
```

The block the MCU streams: RESULT__RANGE_STATUS .. 0x0099.

#### Vl53l4Uld.ResultBlockLen *(constant)*

```csharp
public const int ResultBlockLen = 17
```

Length of the streamed result block (every VL53L4CD_ResultsData_t field).

#### Vl53l4Uld.I2cKhzBoot *(constant)*

```csharp
public const int I2cKhzBoot = 400
```

The bridge boots at 400 kHz; init must run its configuration block there.

#### Vl53l4Uld.I2cKhzDefault *(constant)*

```csharp
public const int I2cKhzDefault = 1000
```

Recommended post-init bus speed (the result-block read is ~4× faster).

#### Vl53l4Uld.DefaultConfiguration *(property)*

```csharp
public static ReadOnlySpan<byte> DefaultConfiguration
```

VL53L4CD_DEFAULT_CONFIGURATION[] — 91 bytes, registers 0x2D..0x87. `ConfigBlock` always overrides byte 0 with `ConfigFmpByte`.

#### Vl53l4Uld.StatusRtn *(property)*

```csharp
public static ReadOnlySpan<byte> StatusRtn
```

GetResult() raw status → ULD status (status_rtn[24] in VL53L4CD_api.c; raw 9 → 0 valid).

#### Vl53l4Uld.RangeStatusNames *(field)*

```csharp
public static readonly IReadOnlyDictionary<int, string> RangeStatusNames = …
```

Range status (after the ST mapping) → its meaning; 0 is a valid measurement.

#### Vl53l4Uld.ConfigBlock

```csharp
public static byte[] ConfigBlock()
```

The 91-byte block sensor_init() writes at `ConfigAddr`: the ST default configuration with byte 0 forced to `ConfigFmpByte` (Fast Mode Plus).

#### Vl53l4Uld.ParseResultBlock

```csharp
public static Vl53l4Results ParseResultBlock(ReadOnlySpan<byte> raw)
```

Decode the streamed 0x0089..0x0099 block exactly as VL53L4CD_GetResult() decodes the same registers read one by one. Register contents are big-endian words (the bridge passes them through untouched). Raw status ≥ 24 passes through unmapped.

#### Vl53l4Uld.static

```csharp
public static (ushort RangeConfigA, ushort RangeConfigB, uint IntermeasurementRaw) RangeTimingRegisters(int timingBudgetMs, long interMeasurementMs, int oscFrequency, int clockPll = 0)
```

SetRangeTiming register math → (RANGE_CONFIG_A, RANGE_CONFIG_B, INTERMEASUREMENT_MS raw dword). `oscFrequency` is the word read from 0x0006; `clockPll` is the word read from RESULT__OSC_CALIBRATE_VAL (used only in autonomous mode, i.e. when `interMeasurementMs` > 0). Bit-exact port of the C ULD, 32-bit truncations and the 1.055 PLL factor included.

#### Vl53l4Uld.static

```csharp
public static (int TimingBudgetMs, int InterMeasurementMs) DecodeRangeTiming(uint intermeasurementRaw, int clockPll, int oscFrequency, int rangeConfigA)
```

GetRangeTiming register math → (timing_budget_ms, inter_measurement_ms). Inputs are the raw register reads: the INTERMEASUREMENT_MS dword, the RESULT__OSC_CALIBRATE_VAL word, the 0x0006 word and the RANGE_CONFIG_A word. Bit-exact port (1.065 PLL factor, 32-bit wrap of the ms-byte).

#### Vl53l4Uld.OffsetRaw

```csharp
public static ushort OffsetRaw(int offsetMm)
```

RANGE_OFFSET_MM word for SetOffset (INNER/OUTER are zeroed alongside).

#### Vl53l4Uld.DecodeOffset

```csharp
public static int DecodeOffset(ushort raw)
```

GetOffset: RANGE_OFFSET_MM word → signed millimetres.

#### Vl53l4Uld.XtalkRaw

```csharp
public static ushort XtalkRaw(int xtalkKcps)
```

XTALK_PLANE_OFFSET_KCPS word for SetXtalk.

#### Vl53l4Uld.DecodeXtalk

```csharp
public static int DecodeXtalk(ushort raw)
```

GetXtalk: XTALK_PLANE_OFFSET_KCPS word → kcps.

#### Vl53l4Uld.SignalThresholdRaw

```csharp
public static int SignalThresholdRaw(int signalKcps)
```

MIN_COUNT_RATE_RTN_LIMIT_MCPS word for SetSignalThreshold.

#### Vl53l4Uld.DecodeSignalThreshold

```csharp
public static int DecodeSignalThreshold(int raw)
```

GetSignalThreshold: MIN_COUNT_RATE_RTN_LIMIT_MCPS word → kcps.

#### Vl53l4Uld.SigmaThresholdRaw

```csharp
public static int SigmaThresholdRaw(int sigmaMm)
```

RANGE_CONFIG__SIGMA_THRESH word for SetSigmaThreshold.

#### Vl53l4Uld.DecodeSigmaThreshold

```csharp
public static int DecodeSigmaThreshold(int raw)
```

GetSigmaThreshold: RANGE_CONFIG__SIGMA_THRESH word → mm.

### IVl53l4Platform

```csharp
public interface IVl53l4Platform
```

What the ULD needs from the board: the register bridge (16-bit addresses, auto-increment), a delay, and the bus speed. `Vl53l4cdDevice` maps it onto the firmware bridge; a test can script it.

### Vl53l4cdDriver

```csharp
public sealed class Vl53l4cdDriver
```

The ST VL53L4CD ULD 2.2.3 (VL53L4CD_api.c + VL53L4CD_calibration.c) on the host, ported from the reference Python driver: init, ranging, timing, offset, crosstalk, thresholds, temperature update, calibration. Every request sequence is the Python one, so captures of either replay in both. Register contents are big-endian.

#### Vl53l4cdDriver.Vl53l4cdDriver *(constructor)*

```csharp
public Vl53l4cdDriver(IVl53l4Platform platform)
```

#### Vl53l4cdDriver.ReadByte

```csharp
public int ReadByte(ushort addr)
```

#### Vl53l4cdDriver.ReadWord

```csharp
public int ReadWord(ushort addr)
```

#### Vl53l4cdDriver.ReadDword

```csharp
public uint ReadDword(ushort addr)
```

#### Vl53l4cdDriver.WriteByte

```csharp
public void WriteByte(ushort addr, int value)
```

#### Vl53l4cdDriver.WriteWord

```csharp
public void WriteWord(ushort addr, int value)
```

#### Vl53l4cdDriver.WriteDword

```csharp
public void WriteDword(ushort addr, uint value)
```

#### Vl53l4cdDriver.GetSensorId

```csharp
public int GetSensorId()
```

#### Vl53l4cdDriver.IsAlive

```csharp
public bool IsAlive()
```

#### Vl53l4cdDriver.WaitBoot

```csharp
public void WaitBoot(int timeoutMs = 1000)
```

#### Vl53l4cdDriver.SensorInit

```csharp
public void SensorInit(int busKhz = Vl53l4Uld.I2cKhzDefault)
```

Initialise the sensor and leave the bus at `busKhz`. The configuration block goes at 400 kHz, the only speed an unconfigured sensor is specified for; the bridge is re-timed after it.

#### Vl53l4cdDriver.ClearInterrupt

```csharp
public void ClearInterrupt()
```

#### Vl53l4cdDriver.StartRanging

```csharp
public void StartRanging()
```

#### Vl53l4cdDriver.StopRanging

```csharp
public void StopRanging()
```

#### Vl53l4cdDriver.CheckForDataReady

```csharp
public bool CheckForDataReady()
```

#### Vl53l4cdDriver.WaitDataReady

```csharp
public void WaitDataReady(int timeoutMs = 1000)
```

#### Vl53l4cdDriver.GetResult

```csharp
public Vl53l4Results GetResult()
```

One block read instead of the C driver's six register reads; the decoding is identical.

#### Vl53l4cdDriver.SetRangeTiming

```csharp
public void SetRangeTiming(int timingBudgetMs, long interMeasurementMs)
```

#### Vl53l4cdDriver.public

```csharp
public (int TimingBudgetMs, int InterMeasurementMs) GetRangeTiming()
```

(timing budget ms, inter-measurement ms; 0 = continuous).

#### Vl53l4cdDriver.SetOffset

```csharp
public void SetOffset(int offsetMm)
```

#### Vl53l4cdDriver.GetOffset

```csharp
public int GetOffset()
```

#### Vl53l4cdDriver.SetXtalk

```csharp
public void SetXtalk(int xtalkKcps)
```

#### Vl53l4cdDriver.GetXtalk

```csharp
public int GetXtalk()
```

#### Vl53l4cdDriver.SetDetectionThresholds

```csharp
public void SetDetectionThresholds(int distanceLowMm, int distanceHighMm, int window)
```

The distance-window interrupt; `window` is one of `WindowBelow` … `WindowIn`.

#### Vl53l4cdDriver.public

```csharp
public (int DistanceLowMm, int DistanceHighMm, int Window) GetDetectionThresholds()
```

(distance low mm, distance high mm, window).

#### Vl53l4cdDriver.SetSignalThreshold

```csharp
public void SetSignalThreshold(int signalKcps)
```

#### Vl53l4cdDriver.GetSignalThreshold

```csharp
public int GetSignalThreshold()
```

#### Vl53l4cdDriver.SetSigmaThreshold

```csharp
public void SetSigmaThreshold(int sigmaMm)
```

#### Vl53l4cdDriver.GetSigmaThreshold

```csharp
public int GetSigmaThreshold()
```

#### Vl53l4cdDriver.StartTemperatureUpdate

```csharp
public void StartTemperatureUpdate()
```

Re-run VHV; recommended after a >8 °C ambient change (ST Example_3).

#### Vl53l4cdDriver.CalibrateOffset

```csharp
public int CalibrateOffset(int targetDistMm, int nbSamples = 20)
```

Offset calibration against a target at `targetDistMm` (10–1000); returns the offset now programmed, mm.

#### Vl53l4cdDriver.CalibrateXtalk

```csharp
public int CalibrateXtalk(int targetDistMm, int nbSamples = 20)
```

Crosstalk calibration against a target at `targetDistMm` (10–5000); returns the xtalk now programmed, kcps (C rounding).

## VL53L8 (ToF)

### Vl53l8cxDevice

```csharp
public class Vl53l8cxDevice : DepzDevice
```

VL53L8CX multizone ToF (contract 04). The board is a thin SPI bridge; the ST ULD runs here (`Uld`). `Init` downloads the ~84 KB sensor firmware (about a second), then configure and `StartRanging`: the board pushes one frame per INT edge in chunks, reassembled and parsed into `Frames`. Configuration needs `Init` and throws `InvalidOperationException` while ranging — the stream owns the register bank. The VL53L8CH superset is `Vl53l8chDevice`.

#### Vl53l8cxDevice.MinRangingFrequencyHz *(constant)*

```csharp
public const int MinRangingFrequencyHz = 2
```

Below this the sensor never enters its ranging loop and streams nothing (contract 04).

#### Vl53l8cxDevice.Vl53l8cxDevice *(constructor)*

```csharp
public Vl53l8cxDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

#### Vl53l8cxDevice.Vl53l8cxDevice *(constructor)*

```csharp
#if NET public Vl53l8cxDevice(string port, TimeSpan? timeout = null) : base(port, timeout)
```

#### Vl53l8cxDevice.Uld *(property)*

```csharp
public Vl53l8Driver Uld
```

The ULD driver (escape hatch for DCI access).

#### Vl53l8cxDevice.Firmware *(property)*

```csharp
public Vl53l8Firmware Firmware
```

The firmware set in use after `Init` (cx / ch / l7cx / l7ch).

#### Vl53l8cxDevice.Ranging *(property)*

```csharp
public bool Ranging
```

#### Vl53l8cxDevice.IsAlive

```csharp
public bool IsAlive()
```

True when the sensor answers with its own silicon ids (a bus error or a foreign id: false).

#### Vl53l8cxDevice.Init

```csharp
public virtual void Init(Action<string>? progress = null, Action<int, int>? writeProgress = null)
```

Download the sensor firmware and the default configuration. `progress` gets phase texts, `writeProgress` (done, total) the big firmware writes.

#### Vl53l8cxDevice.GetResolution

```csharp
public int GetResolution()
```

Zones in use: 16 (4×4) or 64 (8×8).

#### Vl53l8cxDevice.SetResolution

```csharp
public void SetResolution(int zones)
```

16 (4×4) or 64 (8×8) zones.

#### Vl53l8cxDevice.GetRangingFrequencyHz

```csharp
public int GetRangingFrequencyHz()
```

#### Vl53l8cxDevice.SetRangingFrequencyHz

```csharp
public void SetRangingFrequencyHz(int hz)
```

At least 2 Hz; at most 60 Hz at 4×4 and 15 Hz at 8×8.

#### Vl53l8cxDevice.GetRangingMode

```csharp
public int GetRangingMode()
```

`RangingModeContinuous` or `RangingModeAutonomous`.

#### Vl53l8cxDevice.SetRangingMode

```csharp
public void SetRangingMode(int mode)
```

#### Vl53l8cxDevice.GetIntegrationTimeMs

```csharp
public int GetIntegrationTimeMs()
```

#### Vl53l8cxDevice.SetIntegrationTimeMs

```csharp
public void SetIntegrationTimeMs(int ms)
```

2–1000 ms; autonomous mode only.

#### Vl53l8cxDevice.GetSharpenerPercent

```csharp
public int GetSharpenerPercent()
```

#### Vl53l8cxDevice.SetSharpenerPercent

```csharp
public void SetSharpenerPercent(int pct)
```

Edge sharpener, 0–99 % (0 = off).

#### Vl53l8cxDevice.GetTargetOrder

```csharp
public int GetTargetOrder()
```

`TargetOrderClosest` or `TargetOrderStrongest`.

#### Vl53l8cxDevice.SetTargetOrder

```csharp
public void SetTargetOrder(int order)
```

#### Vl53l8cxDevice.GetPowerMode

```csharp
public int GetPowerMode()
```

`PowerModeSleep`, `PowerModeWakeup` or `PowerModeDeepSleep`.

#### Vl53l8cxDevice.SetPowerMode

```csharp
public void SetPowerMode(int mode)
```

Waking from deep sleep downloads the firmware again.

#### Vl53l8cxDevice.GetXtalkMargin

```csharp
public double GetXtalkMargin()
```

#### Vl53l8cxDevice.SetXtalkMargin

```csharp
public void SetXtalkMargin(double marginKcps)
```

#### Vl53l8cxDevice.CalibrateXtalk

```csharp
public void CalibrateXtalk(int reflectancePercent, int nbSamples, int distanceMm)
```

On-device crosstalk calibration against a flat target (see `CalibrateXtalk`); read the result with `GetCaldataXtalk`.

#### Vl53l8cxDevice.GetCaldataXtalk

```csharp
public byte[] GetCaldataXtalk()
```

The 776-byte xtalk calibration buffer (save / restore).

#### Vl53l8cxDevice.SetCaldataXtalk

```csharp
public void SetCaldataXtalk(byte[] blob)
```

#### Vl53l8cxDevice.GetDetectionThresholdsEnable

```csharp
public int GetDetectionThresholdsEnable()
```

#### Vl53l8cxDevice.SetDetectionThresholdsEnable

```csharp
public void SetDetectionThresholdsEnable(bool enabled)
```

#### Vl53l8cxDevice.GetDetectionThresholds

```csharp
public IReadOnlyList<Vl53l8Advanced.DetectionThreshold> GetDetectionThresholds()
```

#### Vl53l8cxDevice.SetDetectionThresholds

```csharp
public void SetDetectionThresholds(IReadOnlyList<Vl53l8Advanced.DetectionThreshold> thresholds)
```

The 64 detection thresholds (interrupt on threshold).

#### Vl53l8cxDevice.SetDetectionThresholdsAutoStop

```csharp
public void SetDetectionThresholdsAutoStop(bool autoStop)
```

#### Vl53l8cxDevice.ConfigureMotionIndicator

```csharp
public MotionConfig ConfigureMotionIndicator(int distanceMinMm = 400, int distanceMaxMm = 1500)
```

Enable the motion indicator over [min, max] mm; every frame then carries `Motion`. Returns the configuration for further tuning.

#### Vl53l8cxDevice.StartRanging

```csharp
public void StartRanging()
```

Program the output list, start the sensor and the board's stream.

#### Vl53l8cxDevice.StopRanging

```csharp
public void StopRanging()
```

#### Vl53l8cxDevice.OnFrame

```csharp
public IDisposable OnFrame(Action<Vl53l8Frame> callback)
```

Call `callback` for every parsed frame (reader thread).

#### Vl53l8cxDevice.Frames

```csharp
public Subscription<Vl53l8Frame> Frames(int capacity = 8)
```

A pull queue of parsed frames (bounded, drop-oldest).

#### Vl53l8cxDevice.GetFrame

```csharp
public Vl53l8Frame GetFrame(TimeSpan? timeout = null)
```

The next frame (contract 07 §3).

#### Vl53l8cxDevice.FrameParseErrors *(property)*

```csharp
public long FrameParseErrors
```

Frames dropped because they did not parse (corrupt frame, bad size) — not reassembly gaps.

#### Vl53l8cxDevice.ReassemblerDiscards *(property)*

```csharp
public int ReassemblerDiscards
```

Chunked frames the reassembler discarded (gaps, offset errors).

#### Vl53l8cxDevice.StreamDroppedCounts *(property)*

```csharp
public IReadOnlyList<long> StreamDroppedCounts
```

### Vl53l8chDevice

```csharp
public class Vl53l8chDevice : Vl53l8cxDevice
```

VL53L8CH: the VL53L8CX superset (every CX method) plus CNH, compact network histograms — each frame can carry a per-aggregate distance histogram. `Init` downloads the CH firmware (VL53LMZ ULD 2.0.16).

#### Vl53l8chDevice.Vl53l8chDevice *(constructor)*

```csharp
public Vl53l8chDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

#### Vl53l8chDevice.Vl53l8chDevice *(constructor)*

```csharp
#if NET public Vl53l8chDevice(string port, TimeSpan? timeout = null) : base(port, timeout)
```

#### Vl53l8chDevice.CnhSetup *(property)*

```csharp
public Vl53l8CnhSetup? CnhSetup
```

The CNH setup armed by `ConfigureCnh`, or null; recorders keep it next to the raw blocks to decode them later.

#### Vl53l8chDevice.ConfigureCnh

```csharp
public void ConfigureCnh(Vl53l8CnhSetup setup)
```

Arm CNH for the next `StartRanging`: every frame then carries `CnhRaw`, decoded with `DecodeHistogram` and `ToDecodeConfig`.

### IVl53l8Platform

```csharp
public interface IVl53l8Platform
```

What the ULD needs from the board: register access through the bridge (16-bit addresses, auto-increment) and a delay. `Vl53l8cxDevice` maps it onto the firmware bridge; a test can script it.

### Vl53l8Firmware

```csharp
public enum Vl53l8Firmware
{
    Cx,
    Ch,
    L7cx,
    L7ch,
}
```

The sensor firmware a `Vl53l8Driver` downloads.

### Vl53l8Exception

```csharp
public sealed class Vl53l8Exception : Exception
```

A VL53L8 / L5 / L7 ULD status (vl53l8cx_api.h codes) with where it happened.

#### Vl53l8Exception.StatusTimeoutError *(constant)*

```csharp
public const int StatusTimeoutError = 1
```

#### Vl53l8Exception.StatusCorruptedFrame *(constant)*

```csharp
public const int StatusCorruptedFrame = 2
```

#### Vl53l8Exception.StatusLaserSafety *(constant)*

```csharp
public const int StatusLaserSafety = 3
```

#### Vl53l8Exception.StatusFwChecksumFail *(constant)*

```csharp
public const int StatusFwChecksumFail = 5
```

#### Vl53l8Exception.McuError *(constant)*

```csharp
public const int McuError = 66
```

#### Vl53l8Exception.StatusInvalidParam *(constant)*

```csharp
public const int StatusInvalidParam = 127
```

#### Vl53l8Exception.StatusError *(constant)*

```csharp
public const int StatusError = 255
```

#### Vl53l8Exception.Code *(property)*

```csharp
public int Code
```

#### Vl53l8Exception.Vl53l8Exception *(constructor)*

```csharp
public Vl53l8Exception(int code, string where = "") : base($"VL53L8CX {NameOf(code)} (code {code}) {where}".TrimEnd())
```

### Vl53l8Motion

```csharp
public sealed record Vl53l8Motion(uint GlobalIndicator1, uint GlobalIndicator2, int Status, int NbOfDetectedAggregates, int NbOfAggregates, uint[] Motion)
```

The motion-indicator block of a frame (VL53L8CX_ResultsData MotionIndicator).

### Vl53l8Driver

```csharp
public sealed class Vl53l8Driver
```

The ST VL53L8CX ULD 2.1.0 / VL53LMZ ULD 2.0.16 / VL53L5CX-L7CX ULD 2.0.1 on the host, ported from the reference Python driver: init (sensor firmware download), resolution, frequency, mode, integration time, sharpener, target order, ranging, frame parsing, power modes, xtalk margin and calibration, detection thresholds, motion indicator. Register sequences are a 1:1 port — every request matches the Python one, so captures replay in both.

#### Vl53l8Driver.Resolution4x4 *(constant)*

```csharp
public const int Resolution4x4 = 16
```

#### Vl53l8Driver.Resolution8x8 *(constant)*

```csharp
public const int Resolution8x8 = 64
```

#### Vl53l8Driver.RangingModeContinuous *(constant)*

```csharp
public const int RangingModeContinuous = 1
```

#### Vl53l8Driver.RangingModeAutonomous *(constant)*

```csharp
public const int RangingModeAutonomous = 3
```

#### Vl53l8Driver.TargetOrderClosest *(constant)*

```csharp
public const int TargetOrderClosest = 1
```

#### Vl53l8Driver.TargetOrderStrongest *(constant)*

```csharp
public const int TargetOrderStrongest = 2
```

#### Vl53l8Driver.PowerModeSleep *(constant)*

```csharp
public const int PowerModeSleep = 0
```

#### Vl53l8Driver.PowerModeWakeup *(constant)*

```csharp
public const int PowerModeWakeup = 1
```

#### Vl53l8Driver.PowerModeDeepSleep *(constant)*

```csharp
public const int PowerModeDeepSleep = 2
```

#### Vl53l8Driver.ModuleTypeMz *(constant)*

```csharp
public const int ModuleTypeMz = 0
```

#### Vl53l8Driver.ModuleTypeMzevo *(constant)*

```csharp
public const int ModuleTypeMzevo = 1
```

#### Vl53l8Driver.ModuleTypeMzplus *(constant)*

```csharp
public const int ModuleTypeMzplus = 2
```

#### Vl53l8Driver.NvmDataSize *(constant)*

```csharp
public const int NvmDataSize = 492
```

#### Vl53l8Driver.OffsetBufferSize *(constant)*

```csharp
public const int OffsetBufferSize = 488
```

#### Vl53l8Driver.XtalkBufferSize *(constant)*

```csharp
public const int XtalkBufferSize = 776
```

#### Vl53l8Driver.Vl53l8Driver *(constructor)*

```csharp
public Vl53l8Driver(IVl53l8Platform platform, Vl53l8Firmware firmware = Vl53l8Firmware.Cx)
```

#### Vl53l8Driver.Firmware *(property)*

```csharp
public Vl53l8Firmware Firmware
```

#### Vl53l8Driver.IsI2c *(property)*

```csharp
public bool IsI2c
```

The L5/L7 boards: the sensor sits on I2C (other boot sequence, per-target blocks keep 64 entries in 4×4).

#### Vl53l8Driver.ModuleType *(property)*

```csharp
public int? ModuleType
```

L5/L7: MZ (VL53L5CX) or MZEVO (VL53L7CX/CH), read during `Init`; else null.

#### Vl53l8Driver.DataReadSize *(property)*

```csharp
public int DataReadSize
```

The frame size the firmware streams (bytes), set by `StartRanging`.

#### Vl53l8Driver.public

```csharp
public (int Firmware, int Host)? FrameSizeMismatch
```

(firmware, host) frame sizes when the firmware reported another size than the ULD formula; the firmware value is used.

#### Vl53l8Driver.Resolution *(property)*

```csharp
public int? Resolution
```

Zones the last `StartRanging` used.

#### Vl53l8Driver.XtalkCalibrationFailed *(property)*

```csharp
public bool XtalkCalibrationFailed
```

The last `CalibrateXtalk` ran into "coverglass too good": the default xtalk buffer is kept.

#### Vl53l8Driver.XtalkData *(property)*

```csharp
public byte[] XtalkData
```

The 776-byte xtalk buffer the driver uploads (default, calibrated or restored).

#### Vl53l8Driver.SiliconIdMatches

```csharp
public static bool SiliconIdMatches(int deviceId, int revisionId, bool i2c)
```

Whether the silicon ids are the sensor's: (0xF0, 0x0C) on L8 over SPI; revision 0x02, or 0x01 with device id 0xF0, on L5/L7 over I2C.

#### Vl53l8Driver.DciReadData

```csharp
public byte[] DciReadData(int index, int dataSize)
```

#### Vl53l8Driver.DciWriteData

```csharp
public void DciWriteData(int index, byte[] data)
```

#### Vl53l8Driver.DciReplaceData

```csharp
public void DciReplaceData(int index, int dataSize, byte[] newData, int newDataPos)
```

#### Vl53l8Driver.public

```csharp
public (int DeviceId, int RevisionId) IsAlive()
```

(device id, revision id) of the silicon; `SiliconIdMatches` tells whether they are the sensor's.

#### Vl53l8Driver.Init

```csharp
public void Init(Action<string>? progress = null)
```

vl53l8cx_init: boot the sensor MCU, download the 84 KB sensor firmware, upload the NVM offsets, the default xtalk and the default configuration. `progress` gets phase texts.

#### Vl53l8Driver.GetResolution

```csharp
public int GetResolution()
```

#### Vl53l8Driver.SetResolution

```csharp
public void SetResolution(int resolution)
```

#### Vl53l8Driver.GetRangingFrequencyHz

```csharp
public int GetRangingFrequencyHz()
```

#### Vl53l8Driver.SetRangingFrequencyHz

```csharp
public void SetRangingFrequencyHz(int hz)
```

#### Vl53l8Driver.SetRangingMode

```csharp
public void SetRangingMode(int mode)
```

#### Vl53l8Driver.GetRangingMode

```csharp
public int GetRangingMode()
```

#### Vl53l8Driver.GetIntegrationTimeMs

```csharp
public int GetIntegrationTimeMs()
```

#### Vl53l8Driver.SetIntegrationTimeMs

```csharp
public void SetIntegrationTimeMs(int timeMs)
```

2..1000 ms; no effect in continuous mode.

#### Vl53l8Driver.GetSharpenerPercent

```csharp
public int GetSharpenerPercent()
```

0..99 %, rounded to nearest on the way back (ST truncates and loses a count for 95 of the 100 legal values; the stored byte is the same).

#### Vl53l8Driver.SetSharpenerPercent

```csharp
public void SetSharpenerPercent(int pct)
```

#### Vl53l8Driver.GetTargetOrder

```csharp
public int GetTargetOrder()
```

#### Vl53l8Driver.SetTargetOrder

```csharp
public void SetTargetOrder(int order)
```

#### Vl53l8Driver.StartRanging

```csharp
public void StartRanging(int? cnhDataSize = null)
```

Program the output list and start. `cnhDataSize` (bytes, from `RequiredMemory`) appends the CNH block to every frame.

#### Vl53l8Driver.StopRanging

```csharp
public void StopRanging()
```

#### Vl53l8Driver.CheckDataReady

```csharp
public bool CheckDataReady()
```

Poll mode: a new frame is ready.

#### Vl53l8Driver.GetRangingData

```csharp
public Vl53l8Frame GetRangingData(ulong timestampUs = 0)
```

Poll mode: read one frame from register 0x00 and parse it.

#### Vl53l8Driver.ParseFrame

```csharp
public Vl53l8Frame ParseFrame(ulong timestampUs, byte[] raw)
```

Parse one raw frame (`DataReadSize` bytes) — polled, or pushed by the board's stream. L5/L7 frames are trimmed to the zones in use.

#### Vl53l8Driver.GetPowerMode

```csharp
public int GetPowerMode()
```

#### Vl53l8Driver.SetPowerMode

```csharp
public void SetPowerMode(int powerMode)
```

Wakeup, sleep or deep sleep (not on L5CX/L7CX). Waking from deep sleep re-runs `Init`: the sensor firmware is lost.

#### Vl53l8Driver.GetXtalkMargin

```csharp
public double GetXtalkMargin()
```

Xtalk margin, kcps/SPAD.

#### Vl53l8Driver.SetXtalkMargin

```csharp
public void SetXtalkMargin(double marginKcps)
```

#### Vl53l8Driver.GetCaldataXtalk

```csharp
public byte[] GetCaldataXtalk()
```

The live 776-byte xtalk calibration buffer (save / restore); the resolution is restored afterwards.

#### Vl53l8Driver.SetCaldataXtalk

```csharp
public void SetCaldataXtalk(byte[] xtalkData)
```

Restore a saved 776-byte xtalk buffer; it is uploaded again by the next resolution change and at once here.

#### Vl53l8Driver.GetDetectionThresholdsEnable

```csharp
public int GetDetectionThresholdsEnable()
```

#### Vl53l8Driver.SetDetectionThresholdsEnable

```csharp
public void SetDetectionThresholdsEnable(bool enabled)
```

#### Vl53l8Driver.GetDetectionThresholds

```csharp
public IReadOnlyList<Vl53l8Advanced.DetectionThreshold> GetDetectionThresholds()
```

The 64 thresholds, low / high rescaled to the units of their measurement.

#### Vl53l8Driver.SetDetectionThresholds

```csharp
public void SetDetectionThresholds(IReadOnlyList<Vl53l8Advanced.DetectionThreshold> thresholds)
```

Program the 64 thresholds (missing entries are zeros; more than 64 is refused).

#### Vl53l8Driver.SetDetectionThresholdsAutoStop

```csharp
public void SetDetectionThresholdsAutoStop(bool autoStop)
```

#### Vl53l8Driver.MotionIndicatorInit

```csharp
public MotionConfig MotionIndicatorInit(int resolution)
```

A default motion-indicator configuration for `resolution`, written to the sensor; frames carry motion output from here on.

#### Vl53l8Driver.MotionIndicatorSetResolution

```csharp
public void MotionIndicatorSetResolution(MotionConfig cfg, int resolution)
```

#### Vl53l8Driver.MotionIndicatorSetDistanceMotion

```csharp
public void MotionIndicatorSetDistanceMotion(MotionConfig cfg, int distanceMinMm, int distanceMaxMm)
```

#### Vl53l8Driver.CalibrateXtalk

```csharp
public void CalibrateXtalk(int reflectancePercent, int nbSamples, int distanceMm)
```

vl53l8cx_calibrate_xtalk: on-device crosstalk calibration against a flat target at `distanceMm` (600..3000) of `reflectancePercent` (1..99), averaging `nbSamples` (1..16). Takes seconds; the configuration is saved and restored around it. Without cover glass the firmware answers "coverglass too good": `XtalkCalibrationFailed`, and the default buffer stays.

### Vl53l8Variant

```csharp
public enum Vl53l8Variant
{
    Cx,
    Ch,
}
```

The two VL53L8 ToF silicon variants in the DEPZ family. Both stream the same ULD results-frame layout, so `Vl53l8FrameDecoder` and the `Vl53l8Advanced` DCI codecs serve both; the differences are:

USB product id — `Ch` ships as production PID 0xED40; `Cx` is the development default and enumerates under the raw ST VID/PID. Results-frame footer-id offset — CX FW (ULD 2.1.0) echoes the frame id 12 bytes before the end, CH FW (VL53LMZ 2.0.16) 4 bytes (`ForVariant`). CNH histograms — CH-only; the frame decoder copies the raw block into `CnhRaw` and `DecodeHistogram` decodes it.

### Vl53l8Frame

```csharp
public sealed record Vl53l8Frame(ulong TimestampUs, int Resolution, int SiliconTempDegc, int[] DistanceMm, byte[] TargetStatus, byte[] NbTargetDetected, double[] SignalPerSpad, double[] AmbientPerSpad, uint[] NbSpadsEnabled, double[] RangeSigmaMm, double[] Reflectance)
```

One decoded ranging frame. Per-zone arrays are sized to the active resolution (16 for 4×4, 64 for 8×8); zone index runs row-major. Values are in the real units ST's GetRangingData gives (contract 04 §5); ST divides integers, here the scaled values keep their fraction.

#### Vl53l8Frame.CnhRaw *(property)*

```csharp
public byte[]? CnhRaw
```

Raw CNH data block (output index 0xC048), byte-swapped like every other block — decode with `DecodeHistogram`. Null when the frame carries no CNH block (CX variants, or CNH not configured).

#### Vl53l8Frame.Motion *(property)*

```csharp
public Vl53l8Motion? Motion
```

The motion-indicator output, when the motion indicator is configured; else null.

### Vl53l8FrameDecoder

```csharp
public sealed class Vl53l8FrameDecoder
```

VL53L8 results-frame decoder (contracts/04_SENSOR_VL53L8.md). Verbatim port of the verifiable parse path of the ST ULD's GetRangingData / parse_frame.

Serves both ToF variants — VL53L8CX and VL53L8CH stream the identical results-frame layout; only the frame-id footer offset differs per variant (see `Vl53l8Variant` / `ForVariant`). The CH-only CNH histogram block is copied out raw (`CnhRaw`) and decoded by `DecodeHistogram`; the register-bridge init/config that produces these frames is `Vl53l8Driver`.

#### Vl53l8FrameDecoder.CorruptedFrameException

```csharp
public sealed class CorruptedFrameException : Exception
```

Raised when a frame's header/footer id words disagree (corrupt frame).

##### Vl53l8FrameDecoder.CorruptedFrameException.CorruptedFrameException *(constructor)*

```csharp
public CorruptedFrameException() : base("VL53L8CX CORRUPTED_FRAME (header/footer id mismatch)")
```

#### Vl53l8FrameDecoder.FooterIdOffsetCx *(constant)*

```csharp
public const int FooterIdOffsetCx = 12
```

Frame-id footer offset for VL53L8CX FW (ULD 2.1.0): 12 bytes from end.

#### Vl53l8FrameDecoder.FooterIdOffsetCh *(constant)*

```csharp
public const int FooterIdOffsetCh = 4
```

Frame-id footer offset for VL53L8CH FW (VL53LMZ 2.0.16): 4 bytes from end.

#### Vl53l8FrameDecoder.Vl53l8FrameDecoder *(constructor)*

```csharp
public Vl53l8FrameDecoder(int footerIdOff = FooterIdOffsetCx, bool trimToZones = false)
```

#### Vl53l8FrameDecoder.ForVariant

```csharp
public static Vl53l8FrameDecoder ForVariant(Vl53l8Variant variant)
```

Decoder tuned for a specific ToF variant (selects the frame-id footer offset). The decode body is shared — CX and CH stream the same frame.

#### Vl53l8FrameDecoder.SwapBuffer

```csharp
public static byte[] SwapBuffer(ReadOnlySpan<byte> data)
```

VL53L8CX_SwapBuffer: byte-reverse every 32-bit word; trailing bytes unchanged.

#### Vl53l8FrameDecoder.ParseFrame

```csharp
public Vl53l8Frame ParseFrame(ulong timestampUs, byte[] raw)
```

Parse one raw results frame (the reassembled bytes read from reg 0x00). Resolution is derived from the number-of-targets block length.

#### Vl53l8FrameDecoder.ParseFrame

```csharp
public Vl53l8Frame ParseFrame(ulong timestampUs, byte[] raw, int resolution)
```

Parse one raw results frame, trimming every per-zone array to `resolution` entries (16 or 64) — the resolution the host configured in `start_ranging`. Needed for VL53L5/L7 frames, whose per-target blocks carry 64 entries even in 4×4 (contract 11 §3).

### Vl53l8Cmd

```csharp
public enum Vl53l8Cmd
{
    ReadReg = 0x32,
    WriteReg = 0x33,
    StartStream = 0x35,
    StopStream = 0x36,
}
```

VL53L8 register-bridge command opcodes (contracts/04_SENSOR_VL53L8.md).

### Vl53l8Rpt

```csharp
public enum Vl53l8Rpt
{
    RegData = 0x91,
    Vl53Frame = 0x93,
}
```

VL53L8 report opcodes.

### Vl53l8Wire

```csharp
public static class Vl53l8Wire
```

Wire limits (contract 04).

#### Vl53l8Wire.ChunkSize *(constant)*

```csharp
public const int ChunkSize = 2048
```

Register bytes one READ_REG / WRITE_REG moves at most (the bridge's limit).

#### Vl53l8Wire.PackReadReg

```csharp
public static byte[] PackReadReg(int addr, int length)
```

VL53_READ_REG payload: addr u16 LE, length u16 LE.

#### Vl53l8Wire.PackWriteReg

```csharp
public static byte[] PackWriteReg(int addr, ReadOnlySpan<byte> data)
```

VL53_WRITE_REG payload: addr u16 LE, then at least one data byte (contract 04 §2).

#### Vl53l8Wire.PackStartStream

```csharp
public static byte[] PackStartStream(int frameSize)
```

VL53_START_STREAM payload: the frame size the board reads per INT, u16 LE.

#### Vl53l8Wire.StreamChunkMax *(constant)*

```csharp
public const int StreamChunkMax = 1528
```

Bytes of frame data per RPT_VL53_FRAME chunk.

#### Vl53l8Wire.StreamTotalMax *(constant)*

```csharp
public const int StreamTotalMax = 8192
```

Max frame_size accepted by START_STREAM.

### Vl53l8RegData

```csharp
public sealed record Vl53l8RegData(int Cmd, ulong TimestampUs, byte[] Data)
```

One RPT_VL53_FRAME chunk: device timestamp, the full frame size, this chunk's byte offset into the frame, and the chunk payload.

#### Vl53l8RegData.Unpack

```csharp
public static Vl53l8RegData Unpack(ReadOnlySpan<byte> payload)
```

### FrameChunk

```csharp
public sealed record FrameChunk(ulong TimestampUs, int FullSize, int Offset, byte[] Data)
```

#### FrameChunk.Unpack

```csharp
public static FrameChunk Unpack(ReadOnlySpan<byte> payload)
```

Parse a RPT_VL53_FRAME payload: ts u64 LE, full u16 LE, off u16 LE, then data.

### FrameReassembler

```csharp
public sealed class FrameReassembler
```

Rebuilds full sensor frames from chunked RPT_VL53_FRAME reports.

Rules (contract 04): reset on offset==0; chunks must be contiguous — a gap discards the frame in progress; a frame completes when the accumulated bytes equal `FullSize`. Byte-exact with the reference Python `FrameReassembler`.

#### FrameReassembler.Completed *(property)*

```csharp
public int Completed
```

#### FrameReassembler.Discarded *(property)*

```csharp
public int Discarded
```

#### FrameReassembler.public

```csharp
public (ulong TimestampUs, byte[] Frame)? Feed(FrameChunk chunk)
```

Feed one chunk; returns (timestampUs, frameBytes) when a frame completes, else null.

### Vl53l8Advanced

```csharp
public static class Vl53l8Advanced
```

VL53L8 advanced-feature DCI codecs — pure, verifiable ports of the ST ULD (BSD-3-Clause) plugin encoders: motion-indicator configuration, detection-threshold block, and xtalk-margin scaling. Shared across both ToF variants (VL53L8CX and VL53L8CH; see `Vl53l8Variant`). The DCI read/write that carries these to the sensor is `Vl53l8Driver`.

#### Vl53l8Advanced.Resolution4x4 *(constant)*

```csharp
public const int Resolution4x4 = 16
```

#### Vl53l8Advanced.Resolution8x8 *(constant)*

```csharp
public const int Resolution8x8 = 64
```

#### Vl53l8Advanced.DistMm *(constant)*

```csharp
public const int DistMm = 1
```

#### Vl53l8Advanced.SignalPerSpadKcps *(constant)*

```csharp
public const int SignalPerSpadKcps = 2
```

#### Vl53l8Advanced.RangeSigmaMm *(constant)*

```csharp
public const int RangeSigmaMm = 4
```

#### Vl53l8Advanced.AmbientPerSpadKcps *(constant)*

```csharp
public const int AmbientPerSpadKcps = 8
```

#### Vl53l8Advanced.NbSpadsEnabled *(constant)*

```csharp
public const int NbSpadsEnabled = 13
```

#### Vl53l8Advanced.MotionIndicator *(constant)*

```csharp
public const int MotionIndicator = 19
```

#### Vl53l8Advanced.NbThresholds *(constant)*

```csharp
public const int NbThresholds = 64
```

#### Vl53l8Advanced.XtalkMarginRaw

```csharp
public static uint XtalkMarginRaw(double marginKcps)
```

Xtalk margin raw DCI value: round(kcps × 2048). The margin is 0..10000 kcps (set_xtalk_margin's own limit); anything else, NaN included, is refused — never wrapped (contract 07 §3).

#### Vl53l8Advanced.DetectionThreshold

```csharp
public sealed record DetectionThreshold(int LowThresh, int HighThresh, int Measurement, int Type, int ZoneNum, int Operation)
```

One detection threshold (raw real-unit low/high, before scaling).

#### Vl53l8Advanced.ThresholdValidStatus

```csharp
public static byte[] ThresholdValidStatus()
```

The 8-byte DCI_DET_THRESH_VALID_STATUS payload written before the block.

#### Vl53l8Advanced.PackDetectionThresholds

```csharp
public static byte[] PackDetectionThresholds(IReadOnlyList<DetectionThreshold> thresholds)
```

Encode the 64×12-byte DCI_DET_THRESH_START block. Each entry is packed as (i32 low, i32 high, u8 measurement, u8 type, u8 zone, u8 op) with low/high multiplied by the measurement's scale factor; missing entries are zeros. Anything out of its field's range (measurement/type/zone/operation u8, scaled low/high i32) or more than 64 entries throws, never wrapped or dropped (contract 07 §3).

### MotionConfig

```csharp
public sealed class MotionConfig
```

Mirror of VL53L8CX_Motion_Configuration (156 bytes, ST ULD motion plugin). `Pack` is byte-exact with the C struct `<i 3I 12B 64b 32B 32B>`.

#### MotionConfig.RefBinOffset *(field)*

```csharp
public int RefBinOffset
```

#### MotionConfig.DetectionThreshold *(field)*

```csharp
public uint DetectionThreshold
```

#### MotionConfig.ExtraNoiseSigma *(field)*

```csharp
public uint ExtraNoiseSigma
```

#### MotionConfig.NullDenClipValue *(field)*

```csharp
public uint NullDenClipValue
```

#### MotionConfig.MemUpdateMode *(field)*

```csharp
public byte MemUpdateMode
```

#### MotionConfig.MemUpdateChoice *(field)*

```csharp
public byte MemUpdateChoice
```

#### MotionConfig.SumSpan *(field)*

```csharp
public byte SumSpan
```

#### MotionConfig.FeatureLength *(field)*

```csharp
public byte FeatureLength
```

#### MotionConfig.NbOfAggregates *(field)*

```csharp
public byte NbOfAggregates
```

#### MotionConfig.NbOfTemporalAccumulations *(field)*

```csharp
public byte NbOfTemporalAccumulations
```

#### MotionConfig.MinNbForGlobalDetection *(field)*

```csharp
public byte MinNbForGlobalDetection
```

#### MotionConfig.GlobalIndicatorFormat1 *(field)*

```csharp
public byte GlobalIndicatorFormat1
```

#### MotionConfig.GlobalIndicatorFormat2 *(field)*

```csharp
public byte GlobalIndicatorFormat2
```

#### MotionConfig.Spare1 *(field)*

```csharp
public byte Spare1
```

#### MotionConfig.Spare2 *(field)*

```csharp
public byte Spare2
```

#### MotionConfig.Spare3 *(field)*

```csharp
public byte Spare3
```

#### MotionConfig.MapId *(field)*

```csharp
public readonly sbyte[] MapId = new sbyte[64]
```

#### MotionConfig.IndicatorFormat1 *(field)*

```csharp
public readonly byte[] IndicatorFormat1 = new byte[32]
```

#### MotionConfig.IndicatorFormat2 *(field)*

```csharp
public readonly byte[] IndicatorFormat2 = new byte[32]
```

#### MotionConfig.Pack

```csharp
public byte[] Pack()
```

Serialize to the 156-byte VL53L8CX_Motion_Configuration wire form.

#### MotionConfig.InitDefault

```csharp
public static MotionConfig InitDefault(int resolution)
```

The default motion configuration produced by vl53l8cx_motion_indicator_init for the given resolution (distance-window preset + zone map).

#### MotionConfig.SetResolution

```csharp
public void SetResolution(int resolution)
```

vl53l8cx_motion_indicator_set_resolution: fill the per-zone map id.

### Vl53l8CnhConfig

```csharp
public sealed record Vl53l8CnhConfig(int NbOfAggregates, int FeatureLength)
```

The `Vl53l8Cnh` input: the two aggregate/histogram dimensions of the on-device CNH buffer (`VL53LMZ_Motion_Configuration`). These fix the block's internal offsets, so the decode needs them alongside the raw bytes.

### Vl53l8CnhAggregate

```csharp
public sealed record Vl53l8CnhAggregate(int[] HistRaw, sbyte[] HistScaler, int AmbientRaw, sbyte AmbientScaler)
```

One decoded CNH aggregate histogram. The float value of bin `i` is `HistRaw[i] / 2^HistScaler[i]` (a per-bin block-floating-point mantissa + shift); both arrays are `FeatureLength` long. The ambient level of the aggregate is `AmbientRaw / 2^AmbientScaler` (the value ST's Example_12 prints).

#### Vl53l8CnhAggregate.Hist *(property)*

```csharp
public double[] Hist
```

The histogram as floating-point values.

#### Vl53l8CnhAggregate.Ambient *(property)*

```csharp
public double Ambient
```

The ambient level as a floating-point value.

### Vl53l8CnhResult

```csharp
public sealed record Vl53l8CnhResult(uint RefResidualWord, IReadOnlyList<Vl53l8CnhAggregate> Aggregates)
```

A decoded CNH data block: the reference-residual word plus one histogram per aggregate (in aggregate-id order).

#### Vl53l8CnhResult.RefResidual *(property)*

```csharp
public double RefResidual
```

The ref residual (11 fractional bits).

### Vl53l8Cnh

```csharp
public static class Vl53l8Cnh
```

VL53L8CH-specific CNH (Compact Network Histogram) decode.

CNH is what the VL53L8CH firmware adds on top of the CX base: a per-aggregate distance histogram carried alongside the normal ranging frame. The shared results-frame decode (`Vl53l8FrameDecoder`) and the advanced DCI codecs (`Vl53l8Advanced` / `MotionConfig`) already serve both CX and CH; this is the CH-only histogram parse.

A 1:1 port of the decode path of `vl53lmz_plugin_cnh.c` (`vl53lmz_cnh_get_block_addresses` / `_cnh_get_mem_block_addresses`, VL53LMZ ULD 2.0.16) for the fixed `cnh_cfg` used by the DEPZ firmware (DISABLE_PING_PONG | DISABLE_VARIANCE | AMBIENT | XTALK | ZERO_INVALID | STORE_REF_RESIDUAL). Verified byte-exact against the live-hardware golden vector `contracts/vectors/vl53l8_cnh.json`. The offset arithmetic mirrors the C plugin verbatim; do not "simplify" it.

#### Vl53l8Cnh.Variant *(constant)*

```csharp
public const Vl53l8Variant Variant = Vl53l8Variant.Ch
```

The variant this histogram block belongs to (always CH).

#### Vl53l8Cnh.DecodeHistogram

```csharp
public static Vl53l8CnhResult DecodeHistogram(Vl53l8CnhConfig config, ReadOnlySpan<byte> block)
```

Decode a captured CNH data block (`block`, byte-swapped exactly like the standard ranging blocks) into per-aggregate histograms.

Layout (little-endian words) — a faithful port of `_cnh_get_mem_block_addresses` for the fixed cnh_cfg (ping-pong + variance disabled):

`ref_residual_word = word[2]` (byte offset 8). Per-buffer base = `CNH_PER_HEADER_WORDS (5)` + the ping-pong buffer size (from the header buffer-info word); with ping-pong disabled the device reports one buffer and this resolves to the single buffer. Data starts after the 2-word buffer header, then: FEAT_INT `int32[nb_agg*feat]`, FEAT_FRAC `sbyte[nb_agg*feat]` (4-byte padded), AMBIENT_INT `int32[nb_agg]`, AMBIENT_FRAC `sbyte[nb_agg]`. Aggregate `a`'s slice starts at `a*feat`.

### Vl53l8CnhSetup

```csharp
public sealed class Vl53l8CnhSetup
```

The CNH (compact network histogram) configuration of a VL53L8CH / VL53L7CH: a mirror of VL53LMZ_Motion_Configuration with the plugin helpers that fill it (vl53lmz_plugin_cnh.c, ULD 2.0.16), ported from the reference Python `CnhConfig`. Build it with `InitConfig` and `CreateAggMap`, check `RequiredMemory`, then arm it with the device's `ConfigureCnh`. The flags are fixed to ping-pong and variance off (Example_12_cnh_data.c), which `DecodeHistogram` assumes. Arguments outside their fields throw `ArgumentException`, never wrap.

#### Vl53l8CnhSetup.MiCfgDevIdx *(constant)*

```csharp
public const int MiCfgDevIdx = 0xBFAC
```

#### Vl53l8CnhSetup.CnhMaxDataBytes *(constant)*

```csharp
public const int CnhMaxDataBytes = 1540 * 4
```

#### Vl53l8CnhSetup.PulseWidthBin *(constant)*

```csharp
public const int PulseWidthBin = 10
```

#### Vl53l8CnhSetup.BinWidthMm *(constant)*

```csharp
public const double BinWidthMm = 37.5348
```

#### Vl53l8CnhSetup.RefBinOffset *(property)*

```csharp
public int RefBinOffset
```

#### Vl53l8CnhSetup.SumSpan *(property)*

```csharp
public int SumSpan
```

#### Vl53l8CnhSetup.FeatureLength *(property)*

```csharp
public int FeatureLength
```

#### Vl53l8CnhSetup.NbOfAggregates *(property)*

```csharp
public int NbOfAggregates
```

#### Vl53l8CnhSetup.CnhCfg *(property)*

```csharp
public int CnhCfg
```

#### Vl53l8CnhSetup.CnhFlexShift *(property)*

```csharp
public int CnhFlexShift
```

#### Vl53l8CnhSetup.MapId *(property)*

```csharp
public sbyte[] MapId
```

Device zone → CNH aggregate, −1 for a zone in none (64 entries).

#### Vl53l8CnhSetup.InitConfig

```csharp
public void InitConfig(int startBin, int numBins, int subSample)
```

vl53lmz_cnh_init_config. `startBin`: first device histogram bin; `numBins`: CNH bins; `subSample`: device bins summed per CNH bin.

#### Vl53l8CnhSetup.CreateAggMap

```csharp
public void CreateAggMap(int resolution, int startX, int startY, int mergeX, int mergeY, int cols, int rows)
```

vl53lmz_cnh_create_agg_map: aggregates of `mergeX`×`mergeY` zones, `cols`×`rows` of them from (`startX`, `startY`). `resolution` (16 or 64) must be the sensor's.

#### Vl53l8CnhSetup.RequiredMemory

```csharp
public int RequiredMemory()
```

vl53lmz_cnh_calc_required_memory: the on-device CNH buffer, bytes.

#### Vl53l8CnhSetup.public

```csharp
public (int MinMm, int MaxMm) MinMaxDistanceMm()
```

(min, max) target distance, mm, the histogram captures whole.

#### Vl53l8CnhSetup.BinCenterMm

```csharp
public double BinCenterMm(int binIdx)
```

Distance, mm, at the centre of CNH bin `binIdx`.

#### Vl53l8CnhSetup.ToDecodeConfig

```csharp
public Vl53l8CnhConfig ToDecodeConfig()
```

What `DecodeHistogram` needs to decode this setup's blocks.

#### Vl53l8CnhSetup.Pack

```csharp
public byte[] Pack()
```

The 156-byte VL53LMZ_Motion_Configuration struct for the device (DCI 0xBFAC).

### Vl53l8Tables

```csharp
public static class Vl53l8Tables
```

The ST command tables the VL53L8 / L5 / L7 ULD writes verbatim.

#### Vl53l8Tables.GetXtalkCmd *(property)*

```csharp
public static ReadOnlySpan<byte> GetXtalkCmd
```

vl53l8cx_plugin_xtalk.h: the command that reads the xtalk buffer back; 72 bytes.

#### Vl53l8Tables.CalibrateXtalkCx *(property)*

```csharp
public static ReadOnlySpan<byte> CalibrateXtalkCx
```

VL53L8CX_CALIBRATE_XTALK (ULD 2.1.0); 1000 bytes.

#### Vl53l8Tables.CalibrateXtalkLmz *(property)*

```csharp
public static ReadOnlySpan<byte> CalibrateXtalkLmz
```

VL53LMZ_CALIBRATE_XTALK (ULD 2.0.16: L8CH, L7CH); 984 bytes.

#### Vl53l8Tables.CalibrateXtalkL7cx *(property)*

```csharp
public static ReadOnlySpan<byte> CalibrateXtalkL7cx
```

VL53L7CX_CALIBRATE_XTALK (ULD 2.0.1: L5CX, L7CX); 984 bytes.

#### Vl53l8Tables.XtalkCalOutput *(property)*

```csharp
public static ReadOnlySpan<uint> XtalkCalOutput
```

the 17-block output list of the xtalk calibration run.

#### Vl53l8Tables.XtalkCalOutputEnable *(property)*

```csharp
public static ReadOnlySpan<uint> XtalkCalOutputEnable
```

the output enables of XtalkCalOutput.

## VL53L5CX / VL53L7CX / VL53L7CH (ToF)

### Vl53l7Cmd

```csharp
public enum Vl53l7Cmd
{
    PinCtrl = 0x34,
    GetInfo = 0x37,
    SetI2cSpeed = 0x38,
}
```

Commands the VL53L5CX / VL53L7CX / VL53L7CH I2C bridge adds on top of the VL53L8 register bridge (contracts/11_SENSOR_VL53L7.md §2). READ_REG 0x32, WRITE_REG 0x33, START_STREAM 0x35 and STOP_STREAM 0x36 are the VL53L8 ones (`Vl53l8Cmd`), with the tighter I2C limits of `Vl53l7Wire`.

### Vl53l7Rpt

```csharp
public enum Vl53l7Rpt
{
    Vl53Info = 0x92,
}
```

Reports the L5/L7 bridge adds. RPT_REG_DATA 0x91 / RPT_VL53_FRAME 0x93 are the VL53L8 ones (`Vl53l8Rpt`).

### Vl53l7PinAction

```csharp
public static class Vl53l7PinAction
```

VL53_PIN_CTRL actions. None is a true sensor reset (the board has no power GPIO): after `LpnOff` or `SoftCycle` the host must re-run init() (firmware download included).

#### Vl53l7PinAction.LpnOff *(constant)*

```csharp
public const byte LpnOff = 0
```

Stop streaming, drive LPn low: sensor I2C interface off (reads NACK).

#### Vl53l7PinAction.LpnOn *(constant)*

```csharp
public const byte LpnOn = 1
```

Drive LPn high: interface on (power-up default).

#### Vl53l7PinAction.I2cRst *(constant)*

```csharp
public const byte I2cRst = 2
```

Pulse I2C_RST.

#### Vl53l7PinAction.SoftCycle *(constant)*

```csharp
public const byte SoftCycle = 3
```

Stop streaming, LPn low 1 ms, high, I2C_RST pulse; clears the I2C error counters.

### Vl53l7I2cError

```csharp
public static class Vl53l7I2cError
```

`LastI2cError` values.

#### Vl53l7I2cError.Ok *(constant)*

```csharp
public const byte Ok = 0
```

#### Vl53l7I2cError.Nack *(constant)*

```csharp
public const byte Nack = 1
```

#### Vl53l7I2cError.Timeout *(constant)*

```csharp
public const byte Timeout = 2
```

#### Vl53l7I2cError.BusError *(constant)*

```csharp
public const byte BusError = 3
```

### Vl53l7Wire

```csharp
public static class Vl53l7Wire
```

VL53L5/L7 I2C-bridge wire limits and command payload codecs (contract 11 §2). All wire fields are little-endian.

#### Vl53l7Wire.ReadMaxLen *(constant)*

```csharp
public const int ReadMaxLen = 1536
```

VL53LMZ_READ_MAX: READ_REG `len` 1..1536 (hosts MUST split larger reads here, not at 2048).

#### Vl53l7Wire.WriteMaxLen *(constant)*

```csharp
public const int WriteMaxLen = 2048
```

VL53LMZ_XFER_MAX: WRITE_REG data length 1..2048.

#### Vl53l7Wire.StreamChunkMax *(constant)*

```csharp
public const int StreamChunkMax = 1536
```

Bytes of frame data per RPT_VL53_FRAME chunk (VL53L8: 1528).

#### Vl53l7Wire.StreamTotalMax *(constant)*

```csharp
public const int StreamTotalMax = 8192
```

Max frame_size accepted by START_STREAM (as VL53L8).

#### Vl53l7Wire.InfoSize *(constant)*

```csharp
public const int InfoSize = 20
```

RPT_VL53_INFO payload size.

#### Vl53l7Wire.I2cKhzSteps *(property)*

```csharp
public static ReadOnlySpan<int> I2cKhzSteps
```

Nominal SCL steps the firmware carries a timing for; others snap to the nearest.

#### Vl53l7Wire.PackReadReg

```csharp
public static byte[] PackReadReg(ushort addr, int len)
```

VL53_READ_REG payload: addr u16 LE, len u16 LE (len 1..`ReadMaxLen`, addr+len ≤ 0x10000).

#### Vl53l7Wire.PackWriteReg

```csharp
public static byte[] PackWriteReg(ushort addr, ReadOnlySpan<byte> data)
```

VL53_WRITE_REG payload: addr u16 LE, then the raw register data (1..`WriteMaxLen` bytes).

#### Vl53l7Wire.PackStartStream

```csharp
public static byte[] PackStartStream(int frameSize)
```

VL53_START_STREAM payload: frame_size u16 LE (1..`StreamTotalMax`).

#### Vl53l7Wire.PackPinCtrl

```csharp
public static byte[] PackPinCtrl(byte action)
```

VL53_PIN_CTRL payload: action u8 (`Vl53l7PinAction`).

#### Vl53l7Wire.PackSetI2cSpeed

```csharp
public static byte[] PackSetI2cSpeed(ushort khz)
```

VL53_SET_I2C_SPEED payload: khz u16 LE (the device snaps to `I2cKhzSteps`).

### Vl53l7Info

```csharp
public sealed record Vl53l7Info(uint IntEdges, uint FramesDropped, uint I2cErrors, byte LastI2cError, byte LpnLevel, byte IntLevel, ushort I2cKhz, ushort FrameSize, bool Streaming)
```

RPT_VL53_INFO (0x92) — bridge state only (the sensor is never probed). All counters run from power-up / DEVICE_RESET; only SOFT_CYCLE clears the I2C ones. The report carries no echoed command byte. Each GET_INFO takes the bus from the stream and can drop the frame in flight — read it before and after a run, never in a polling loop during one.

#### Vl53l7Info.Unpack

```csharp
public static Vl53l7Info Unpack(ReadOnlySpan<byte> payload)
```

Parse a RPT_VL53_INFO payload (`<IIIBBBHHB`, 20 bytes LE).

### Vl53l7Model

```csharp
public enum Vl53l7Model
{
    Vl53l5cx,
    Vl53l7cx,
    Vl53l7ch,
}
```

The three sensors one `APP_VL53L7` firmware serves (contract 11). They stream the VL53L8 results-frame layout with the L5/L7 geometry (footer id at size − 4, per-zone trim — see `CreateDecoder`); only `Vl53l7ch` adds CNH (decoded by `Vl53l8Cnh`).

### Vl53l7Discovery

```csharp
public static class Vl53l7Discovery
```

Class resolution for an `APP_VL53L7` board (contract 11 §1).

#### Vl53l7Discovery.ResolveModel

```csharp
public static Vl53l7Model ResolveModel(string? usbModel, string? deviceName)
```

Pick the sensor class. The firmware name cannot tell the three apart and the silicon never tells CX from CH, so (normative order): the production USB PID model (`UsbModelHint`), else the first `VL53L([57])(CX|CH)` match in GET_DEVICE_NAME, else `Vl53l7cx` (its blob runs on every L5/L7 part).

#### Vl53l7Discovery.HasCnh

```csharp
public static bool HasCnh(Vl53l7Model model)
```

True for the model that carries CNH histograms (VL53L7CH).

### Vl53l7Frames

```csharp
public static class Vl53l7Frames
```

L5/L7 results-frame decoding (contract 11 §3). The frame layout, scaling and CNH block are VL53L8's (`Vl53l8FrameDecoder`); two things differ:

the frame-id footer sits at size − 4 for every L5/L7 class (both ULD 2.0.1 and VL53LMZ 2.0.16); per-target blocks keep 64 entries even in 4×4 — the decoder trims every per-zone array to the frame's resolution, inferred from the zone-scaled ambient block (index 0x54D0).

Chunks are reassembled with the shared `FrameReassembler` (chunk size `StreamChunkMax` does not affect it).

#### Vl53l7Frames.FooterIdOffset *(constant)*

```csharp
public const int FooterIdOffset = 4
```

Frame-id footer offset from the frame end, for every L5/L7 class.

#### Vl53l7Frames.MinRangingFrequencyHz *(constant)*

```csharp
public const int MinRangingFrequencyHz = 1
```

Minimum ranging frequency: L5/L7 range at 1 Hz (VL53L8: 2 Hz).

#### Vl53l7Frames.CreateDecoder

```csharp
public static Vl53l8FrameDecoder CreateDecoder()
```

A decoder for L5/L7 frames (any of the three classes).

### Vl53l7cxDevice

```csharp
public class Vl53l7cxDevice : Vl53l8cxDevice
```

VL53L7CX multizone ToF (8×8 zones, 90° field of view) on the I2C bridge board (contract 11). The board speaks the VL53L8 protocol plus three commands, and the sensor runs the same ST ULD family, so this is a `Vl53l8cxDevice` with the L5/L7 firmware set (ULD 2.0.1), smaller register reads, ranging from 1 Hz, and the board commands `GetBridgeInfo`, `SetI2cSpeedKhz`, `PinCtrl`. L5CX / L7CX have no deep sleep and no threshold auto-stop.

#### Vl53l7cxDevice.Vl53l7cxDevice *(constructor)*

```csharp
public Vl53l7cxDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

#### Vl53l7cxDevice.Vl53l7cxDevice *(constructor)*

```csharp
#if NET public Vl53l7cxDevice(string port, TimeSpan? timeout = null) : base(port, timeout)
```

#### Vl53l7cxDevice.ModuleType *(property)*

```csharp
public int? ModuleType
```

The sensor's module type read at `Init`: `ModuleTypeMz` (VL53L5CX) or `ModuleTypeMzevo` (VL53L7CX / CH); null before.

#### Vl53l7cxDevice.XtalkCalibrationFailed *(property)*

```csharp
public bool XtalkCalibrationFailed
```

The last `CalibrateXtalk` found nothing to calibrate ("coverglass too good"): the sensor keeps its default xtalk data.

#### Vl53l7cxDevice.Init

```csharp
public override void Init(Action<string>? progress = null, Action<int, int>? writeProgress = null)
```

Init, refusing non-L5/L7 silicon. A module type other than this class expects (a board labelled L7 carrying an L5, or the reverse) is traced as a warning: ranging still works, the firmware is shared.

#### Vl53l7cxDevice.GetBridgeInfo

```csharp
public Vl53l7Info GetBridgeInfo()
```

Bridge counters and pin levels (the sensor is not touched). Read it before and after a run: during one each call takes the bus from the stream and can cost a frame.

#### Vl53l7cxDevice.SetI2cSpeedKhz

```csharp
public int SetI2cSpeedKhz(int khz)
```

Sensor bus SCL; the board takes the nearest of 100, 200, 400, 500 … 1000 kHz. Returns the value in effect. `BusyException` mid-transfer: stop ranging first.

#### Vl53l7cxDevice.PinCtrl

```csharp
public void PinCtrl(byte action)
```

Drive the sensor's LPn / I2C_RST pins (`Vl53l7PinAction`). LpnOff and SoftCycle drop the sensor's state: `Init` again afterwards.

### Vl53l5cxDevice

```csharp
public class Vl53l5cxDevice : Vl53l7cxDevice
```

VL53L5CX ToF (8×8 zones, 63° field of view): the board, firmware and API of `Vl53l7cxDevice`; the sensor reports module type MZ.

#### Vl53l5cxDevice.Vl53l5cxDevice *(constructor)*

```csharp
public Vl53l5cxDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

#### Vl53l5cxDevice.Vl53l5cxDevice *(constructor)*

```csharp
#if NET public Vl53l5cxDevice(string port, TimeSpan? timeout = null) : base(port, timeout)
```

### Vl53l7chDevice

```csharp
public class Vl53l7chDevice : Vl53l7cxDevice
```

VL53L7CH: the VL53L7CX superset with CNH histograms. `Init` downloads the CH firmware (VL53LMZ ULD 2.0.16, the VL53L8CH one); CNH frames up to ~7.6 KB stream in chunks.

#### Vl53l7chDevice.Vl53l7chDevice *(constructor)*

```csharp
public Vl53l7chDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

#### Vl53l7chDevice.Vl53l7chDevice *(constructor)*

```csharp
#if NET public Vl53l7chDevice(string port, TimeSpan? timeout = null) : base(port, timeout)
```

#### Vl53l7chDevice.CnhSetup *(property)*

```csharp
public Vl53l8CnhSetup? CnhSetup
```

The CNH setup armed by `ConfigureCnh`, or null.

#### Vl53l7chDevice.ConfigureCnh

```csharp
public void ConfigureCnh(Vl53l8CnhSetup setup)
```

Arm CNH for the next start (as `ConfigureCnh`).

## VL53L0X / L1CX / L1CB / L3CX / L4CX (ToF)

### Vl53lxCmd

```csharp
public enum Vl53lxCmd
{
    ReadReg = 0x32,
    WriteReg = 0x33,
    Xshut = 0x34,
    StartStream = 0x35,
    StopStream = 0x36,
    GetInfo = 0x37,
    SetI2cSpeed = 0x38,
    SetAddrWidth = 0x39,
    ClearI2cErrors = 0x3A,
}
```

VL53L 1D-family register-bridge command opcodes, protocol v2.00 (contracts/12_SENSOR_VL53LX.md §2). The contract-10 opcodes plus `SetAddrWidth`.

### Vl53lxRpt

```csharp
public enum Vl53lxRpt
{
    RegData = 0x91,
    Info = 0x92,
    Stream = 0x93,
}
```

VL53L 1D-family report opcodes (same ids as contract 10).

### Vl53lxClearStep

```csharp
public readonly record struct Vl53lxClearStep(ushort Addr, byte Value)
```

One interrupt-release step the bridge plays after every streamed block read.

### Vl53lxWire

```csharp
public static class Vl53lxWire
```

v2.00 wire codecs. One firmware (`APP_VL53L0_4`) serves six products; it is the contract-10 bridge with the sensor-specific facts moved to the host: the register-address width (`PackSetAddrWidth`, new), the interrupt-release writes (carried by `PackStartStream`) and the boot handshake (no longer inside XSHUT). READ_REG, WRITE_REG, XSHUT, SET_I2C_SPEED and the REG_DATA / STREAM reports are identical on the wire to contract 10 and are forwarded to `Vl53l4Wire` (`Vl53l4RegData`, `Vl53l4StreamData`).

#### Vl53lxWire.ClearStepsMax *(constant)*

```csharp
public const int ClearStepsMax = 4
```

Most interrupt-release steps a stream may carry (VL53_CLEAR_STEPS_WIRE_MAX).

#### Vl53lxWire.InfoSize *(constant)*

```csharp
public const int InfoSize = 23
```

RPT_VL53_INFO payload size in v2.00.

#### Vl53lxWire.XferMax *(constant)*

```csharp
public const int XferMax = Vl53l4Wire.XferMax
```

Max read length / write data length per transfer (contract 10).

#### Vl53lxWire.FlagIntActHigh *(constant)*

```csharp
public const byte FlagIntActHigh = Vl53l4Wire.FlagIntActHigh
```

START_STREAM flags bit 1: INT active high (contract 10).

#### Vl53lxWire.PackReadReg

```csharp
public static byte[] PackReadReg(ushort addr, ushort len)
```

VL53_READ_REG payload (contract 10): addr u16 LE, len u16 LE.

#### Vl53lxWire.PackWriteReg

```csharp
public static byte[] PackWriteReg(ushort addr, ReadOnlySpan<byte> data)
```

VL53_WRITE_REG payload (contract 10): addr u16 LE, then the raw register data.

#### Vl53lxWire.PackXshut

```csharp
public static byte[] PackXshut(byte action)
```

VL53_XSHUT payload: action u8 (`Vl53l4Xshut`). In v2.00 RESET has no boot handshake — the host polls the boot register itself.

#### Vl53lxWire.PackSetI2cSpeed

```csharp
public static byte[] PackSetI2cSpeed(ushort khz)
```

VL53_SET_I2C_SPEED payload (contract 10): khz u16 LE.

#### Vl53lxWire.PackSetAddrWidth

```csharp
public static byte[] PackSetAddrWidth(int width)
```

VL53_SET_ADDR_WIDTH payload: width u8, 1 (VL53L0X) or 2. Sticky, 2 after a reset; set it before the first register access of a session.

#### Vl53lxWire.PackStartStream

```csharp
public static byte[] PackStartStream(ushort addr, ushort len, IReadOnlyList<Vl53lxClearStep>? clear = null, byte flags = 0)
```

VL53_START_STREAM payload (v2.00, 6 + 3n bytes): addr u16 LE, len u16 LE, flags u8, n_clear u8, then n × {addr u16 LE, value u8} — the interrupt-release steps the bridge plays after every block read (0..4).

### Vl53lxInfo

```csharp
public sealed record Vl53lxInfo(uint IntEdges, uint SlotsSkipped, uint I2cErrors, byte LastI2cError, byte XshutLevel, byte IntLevel, ushort I2cKhz, byte AddrWidth, byte NClear, uint FramesDropped)
```

RPT_VL53_INFO (v2.00, 23 bytes) — bridge state only; the bridge reads no sensor register. Counters are free-running and wrap silently: watch increments. `SlotsSkipped` = a slot that never got the bus, `I2cErrors` = a bus that answered badly, `FramesDropped` = a good sample the USB TX ring had no room for (since the stream was armed). `LastI2cError`: 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR.

#### Vl53lxInfo.Unpack

```csharp
public static Vl53lxInfo Unpack(ReadOnlySpan<byte> payload)
```

Parse a RPT_VL53_INFO payload (`<IIIBBBHBBI`, 23 bytes LE).

### Vl53lxDriverKind

```csharp
public enum Vl53lxDriverKind
{
    Uld,
    Ulp,
    Histogram,
}
```

The three kinds of driver ST ships for the 1D family, in the order a UI should list them (contract 12 §1). What separates them is where the ranging arithmetic runs.

### Vl53lxClass

```csharp
public enum Vl53lxClass
{
    Vl53lx,
    Vl53l0x,
    Vl53l1cx,
    Vl53l1cb,
    Vl53l3cx,
    Vl53l4cx,
}
```

Sensor class an `APP_VL53L0_4` board resolves to (contract 12 §1). `Vl53lx` is the generic class that takes the product at init — also what a VL53L4CD board on this firmware gets.

### Vl53lxProduct

```csharp
public sealed record Vl53lxProduct(string Name, int UsbPid, ushort ModelId, int ReachMm, IReadOnlyList<Vl53lxDriverKind> DriverKinds, Vl53lxDriverKind DefaultDriver, int AddrWidth, IReadOnlyList<Vl53lxClearStep> ClearSteps, int MaxKhz)
```

One row of the product table: what is true of the part itself, plus the bridge parameters its default driver runs with (register-address width, interrupt-release steps, bus ceiling). The model id is a cross-check only: L1CX/L1CB share 0xEACC, L4CD/L4CX share 0xEBAA.

### Vl53lxProducts

```csharp
public static class Vl53lxProducts
```

Product table and board-name / class resolution (contract 12 §1).

#### Vl53lxProducts.All *(field)*

```csharp
public static readonly IReadOnlyList<Vl53lxProduct> All = …
```

Every product of the family, in the order a UI should list them.

#### Vl53lxProducts.Find

```csharp
public static Vl53lxProduct? Find(string? name)
```

The table row for a product name (exact, upper case), or null.

#### Vl53lxProducts.ProductFromBoardName

```csharp
public static string? ProductFromBoardName(string? name)
```

`"ToF Sensor VL53L4CD USB v2.1"` → `"VL53L4CD"`: the first `VL53L<digit><part>` match, case-insensitive. Null if the name carries no family product (an unstamped board, an unknown part).

#### Vl53lxProducts.ResolveClass

```csharp
public static Vl53lxClass ResolveClass(string? usbModel, string? deviceName)
```

Pick the class (normative order): the production USB PID model (`UsbModelHint`) if it is a family product, else the product the device name carries, else the generic `Vl53lx`. VL53L4CD maps to the generic class on this firmware (the `vl53l4cd` class belongs to `APP_VL53L4`).

#### Vl53lxProducts.ModelIdOk

```csharp
public static bool ModelIdOk(string product, int value)
```

Cross-check the id the sensor answered against the product asked for. True only says "not something else entirely".

#### Vl53lxProducts.DriverKindNames *(field)*

```csharp
public static readonly IReadOnlyList<string> DriverKindNames = new[] { "uld", "ulp", "histogram" }
```

The driver kinds in the order a UI lists them (uld/registry.py DRIVER_KINDS).

#### Vl53lxProducts.DriverKinds

```csharp
public static IReadOnlyList<string> DriverKinds(string product)
```

The driver kinds `product` has, in `DriverKindNames` order.

#### Vl53lxProducts.static

```csharp
public static (Vl53lxDriver Driver, string? Caveat) CreateDriver(string product, string kind, Vl53lxPlatform platform)
```

The driver for one (product, kind) pair, and its caveat or null.

### Vl53lxDieVariant

```csharp
public enum Vl53lxDieVariant
{
    L4,
    L1,
}
```

Which ULD reads the die block (contract 12 §4). They differ only in the signal-rate byte offset S and the per-SPAD scale K.

### Vl53lxDieResult

```csharp
public sealed record Vl53lxDieResult(int RangeStatus, int DistanceMm, int SigmaMm, int SignalRateKcps, int AmbientRateKcps, int SignalPerSpadKcps, int AmbientPerSpadKcps, int NumberOfSpad, int StreamCount)
```

The 17-byte die block decoded as one ULD reads it.

### Vl53lxL0xRaw

```csharp
public sealed record Vl53lxL0xRaw(int DistanceRaw, int DeviceRangeStatus, long SignalRateMcps1616, long AmbientRateMcps1616, int EffectiveSpadCount88)
```

Raw fields of the VL53L0X 12-byte block at 0x14. The PAL range status, sigma and dmax need the device data cached by init — full driver, not base.

### Vl53lxHistogramRaw

```csharp
public sealed record Vl53lxHistogramRaw(int InterruptStatus, int RangeStatus, int ReportStatus, int StreamCount, int DssActualEffectiveSpads, int ReferencePhase, int VcselStart, IReadOnlyList<int> Bins)
```

Status bytes and the 24 photon bins of the 83-byte histogram block at 0x0088. Bins → targets is the full driver's job.

### Vl53lxDecode

```csharp
public static class Vl53lxDecode
```

Stateless decoders of the blocks the 1D-family bridge streams (contract 12 §4) — the "base" every SDK implements. What a block says on its own, without the driver state an init leaves behind.

#### Vl53lxDecode.DieBlockAddr *(constant)*

```csharp
public const ushort DieBlockAddr = 0x0089
```

Die result block (L1CX, L1CB, L3CX, L4CD, L4CX light drivers).

#### Vl53lxDecode.DieBlockLen *(constant)*

```csharp
public const int DieBlockLen = 17
```

#### Vl53lxDecode.L0xBlockAddr *(constant)*

```csharp
public const ushort L0xBlockAddr = 0x14
```

VL53L0X result block (register-address width 1).

#### Vl53lxDecode.L0xBlockLen *(constant)*

```csharp
public const int L0xBlockLen = 12
```

#### Vl53lxDecode.HistogramBlockAddr *(constant)*

```csharp
public const ushort HistogramBlockAddr = 0x0088
```

Histogram block, RESULT__INTERRUPT_STATUS .. RESULT__HISTOGRAM_BIN_23_0_LSB.

#### Vl53lxDecode.HistogramBlockLen *(constant)*

```csharp
public const int HistogramBlockLen = 0x00DA - 0x0088 + 1
```

#### Vl53lxDecode.HistogramBins *(constant)*

```csharp
public const int HistogramBins = 24
```

#### Vl53lxDecode.static

```csharp
public static (int SignalAt, int K) DieVariantParams(Vl53lxDieVariant variant)
```

(signal-rate byte offset, per-SPAD scale K) of a die variant.

#### Vl53lxDecode.ParseDieVariant

```csharp
public static Vl53lxDieVariant ParseDieVariant(string name)
```

Parse the vector/wire name of a die variant (`"l4"` / `"l1"`).

#### Vl53lxDecode.DecodeDieBlock

```csharp
public static Vl53lxDieResult DecodeDieBlock(ReadOnlySpan<byte> raw, Vl53lxDieVariant variant = Vl53lxDieVariant.L4)
```

The 17-byte die block (0x0089..0x0099) as the named ULD reads it. The status maps through the contract-10 table (`StatusRtn`); raw status ≥ 24 passes through unmapped.

#### Vl53lxDecode.DecodeL0xRaw

```csharp
public static Vl53lxL0xRaw DecodeL0xRaw(ReadOnlySpan<byte> raw)
```

Raw fields of the VL53L0X block at 0x14 (VL53L0X_GetRangingMeasurementData before the PAL status/sigma step). Distance is mm (quarter-mm when RangeFractionalEnable); rates are FixPoint16.16 Mcps (9.7 on the wire << 9).

#### Vl53lxDecode.DecodeHistogramRaw

```csharp
public static Vl53lxHistogramRaw DecodeHistogramRaw(ReadOnlySpan<byte> raw)
```

The 83-byte histogram block at 0x0088: status bytes and the 24 bins of 3 big-endian bytes. Bin 23's low byte is carried separately as an MSB/LSB pair — `((MSB << 2) + LSB) & 0xFF` — and patched in (on a copy; the input is not modified) before the bins are read.

### Vl53lxMeasurement

```csharp
public sealed record Vl53lxMeasurement(ulong TimestampUs, int DistanceMm, int Status, string StatusText, double SignalKcps, double AmbientKcps, double SigmaMm, double Spads, IReadOnlyList<Vl53lxTarget> Targets, IReadOnlyDictionary<string, object?> Extra, object? Bins)
```

One ranging result of the 1D family, the same shape for every product. On the histogram driver `Targets` holds every return, strongest first, the top level repeats the first and `Bins` carries the raw 24-bin histogram; the light drivers leave both empty.

#### Vl53lxMeasurement.PlottableStatuses *(field)*

```csharp
public static readonly IReadOnlyList<int> PlottableStatuses = new[] { 0, 6, 11 }
```

Statuses that mean "this distance is real": a histogram product reports 6 on the first frame of a stream and 11 for two merged targets. Prefer `Plottable` over `Valid` there.

#### Vl53lxMeasurement.Valid *(property)*

```csharp
public bool Valid
```

#### Vl53lxMeasurement.Plottable *(property)*

```csharp
public bool Plottable
```

#### Vl53lxMeasurement.PlotDistances

```csharp
public IReadOnlyList<int> PlotDistances()
```

The distances a chart draws: the plottable targets in driver order, or the single distance.

#### Vl53lxMeasurement.PrimaryTarget

```csharp
public Vl53lxTarget? PrimaryTarget()
```

The one target a single-number readout shows, or null.

### Vl53lxDevice

```csharp
public class Vl53lxDevice : DepzDevice
```

A board of the VL53L 1D family on the `APP_VL53L0_4` register bridge (contract 12): VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CD, VL53L4CX. Every ULD runs here; two axes pick how a board is driven — the product (whose parameter set: the board's own, or a sibling's to borrow its driver) and the driver kind (`uld`, `ulp`, `histogram`). `Init` binds the pair, `Configure` re-initialises and applies a ranging configuration (before every run), then `StartRanging` and read `Measurements`. Ask `Supports` before an optional group; a missing one throws `NotSupportedException`. This generic class reads the product from the board's name; the per-product classes fix it.

#### Vl53lxDevice.Vl53lxDevice *(constructor)*

```csharp
public Vl53lxDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

#### Vl53lxDevice.Vl53lxDevice *(constructor)*

```csharp
#if NET public Vl53lxDevice(string port, TimeSpan? timeout = null) : base(port, timeout)
```

#### Vl53lxDevice.BoardName *(property)*

```csharp
public string BoardName
```

The board's device name, e.g. `DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523`.

#### Vl53lxDevice.Detected *(property)*

```csharp
public string? Detected
```

The product the device name carries, or null on an unstamped board.

#### Vl53lxDevice.Product *(property)*

```csharp
public string? Product
```

The product `Init` bound (null before).

#### Vl53lxDevice.DriverKind *(property)*

```csharp
public string? DriverKind
```

#### Vl53lxDevice.Caveat *(property)*

```csharp
public string? Caveat
```

#### Vl53lxDevice.Driver *(property)*

```csharp
public Vl53lxDriver Driver
```

The ULD port in use — escape hatch for product-specific calls.

#### Vl53lxDevice.Initialized *(property)*

```csharp
public bool Initialized
```

#### Vl53lxDevice.Ranging *(property)*

```csharp
public bool Ranging
```

#### Vl53lxDevice.DriverKinds

```csharp
public IReadOnlyList<string> DriverKinds(string? product = null)
```

The driver kinds a product has (default: this board's).

#### Vl53lxDevice.Init

```csharp
public void Init(string? driver = null, string? product = null)
```

Bind the (product, driver kind) pair and initialise the sensor. `product` defaults to this class's, else the board name's; `driver` to the product's first kind. A pair the table lacks throws `NotSupportedException` naming what the product has.

#### Vl53lxDevice.IdentifyProduct

```csharp
public IReadOnlyDictionary<string, object?> IdentifyProduct()
```

Everything about what is connected (needs `Init`). `model_id_ok` is a cross-check only: L1CX/L1CB and L4CD/L4CX share their ids.

#### Vl53lxDevice.Notes

```csharp
public IReadOnlyList<string> Notes()
```

The lines a UI shows about the chosen pair: a product named by hand, a borrowed driver, a pair that reaches less than the board is rated for, the driver's caveat.

#### Vl53lxDevice.Supports

```csharp
public bool Supports(string group)
```

Whether this product / driver serves an optional group.

#### Vl53lxDevice.Modes *(property)*

```csharp
public IReadOnlyList<string> Modes
```

Named ranging modes, the first being what init leaves; empty if none.

#### Vl53lxDevice.public

```csharp
public (int Min, int Max) BudgetRange
```

The (min, max) budget the driver offers, ms.

#### Vl53lxDevice.DriverReachMm

```csharp
public int? DriverReachMm()
```

How far the loaded configuration can measure, mm, or null.

#### Vl53lxDevice.Xshut

```csharp
public void Xshut(byte action)
```

Drive XSHUT (`Vl53l4Xshut`). Off and Reset stop the stream; the sensor then holds none of the configuration — `Init` again.

#### Vl53lxDevice.BridgeInfo

```csharp
public Vl53lxInfo BridgeInfo()
```

RPT_VL53_INFO — the bridge's own counters and settings; safe while streaming.

#### Vl53lxDevice.Configure

```csharp
public void Configure(int budgetMs = 50, int interMs = 0, string? mode = null, int? offsetMm = null, int? xtalkKcps = null, int? signalKcps = null)
```

Re-initialise the sensor and apply a ranging configuration. The re-init is deliberate: it is the only way to know what the configuration registers hold. `mode` goes on before the budget; `offsetMm` / `xtalkKcps` re-apply a stored calibration; `signalKcps` replaces the blob's signal threshold. `interMs` 0 = continuous. An unsupported group is refused before the re-init.

#### Vl53lxDevice.public

```csharp
public (int TimingBudgetMs, int InterMeasurementMs) GetRangeTiming()
```

(timing budget ms, inter-measurement ms) read back from the sensor; 0 = continuous.

#### Vl53lxDevice.GetMode

```csharp
public string? GetMode()
```

The mode in use, or null on a product without modes.

#### Vl53lxDevice.BudgetChoices

```csharp
public IReadOnlyList<int> BudgetChoices()
```

The only budgets accepted right now, or empty when any in `BudgetRange` will do.

#### Vl53lxDevice.SnapBudget

```csharp
public int SnapBudget(int budgetMs)
```

The nearest budget this product accepts.

#### Vl53lxDevice.GetOffsetMm

```csharp
public int GetOffsetMm()
```

#### Vl53lxDevice.SetOffsetMm

```csharp
public void SetOffsetMm(int offsetMm)
```

#### Vl53lxDevice.GetXtalkKcps

```csharp
public int GetXtalkKcps()
```

#### Vl53lxDevice.SetXtalkKcps

```csharp
public void SetXtalkKcps(int xtalkKcps)
```

#### Vl53lxDevice.CalibrateOffset

```csharp
public int CalibrateOffset(int targetDistMm, int? nbSamples = null)
```

Offset calibration against a flat target; returns the offset programmed (store it on the host).

#### Vl53lxDevice.CalibrateXtalk

```csharp
public int CalibrateXtalk(int targetDistMm, int? nbSamples = null)
```

Crosstalk calibration; returns the xtalk programmed, kcps (store it on the host).

#### Vl53lxDevice.public

```csharp
public (int DistanceLowMm, int DistanceHighMm, int Window) GetDetectionThresholds()
```

#### Vl53lxDevice.SetDetectionThresholds

```csharp
public void SetDetectionThresholds(int distanceLowMm, int distanceHighMm, int window)
```

The distance-window interrupt; stays armed until the next init / configure.

#### Vl53lxDevice.GetSignalThresholdKcps

```csharp
public int GetSignalThresholdKcps()
```

#### Vl53lxDevice.SetSignalThresholdKcps

```csharp
public void SetSignalThresholdKcps(int signalKcps)
```

#### Vl53lxDevice.GetSigmaThresholdMm

```csharp
public int GetSigmaThresholdMm()
```

#### Vl53lxDevice.SetSigmaThresholdMm

```csharp
public void SetSigmaThresholdMm(int sigmaMm)
```

#### Vl53lxDevice.public

```csharp
public (int X, int Y) GetRoi()
```

#### Vl53lxDevice.SetRoi

```csharp
public void SetRoi(int x, int y)
```

#### Vl53lxDevice.GetRoiCenter

```csharp
public int GetRoiCenter()
```

#### Vl53lxDevice.SetRoiCenter

```csharp
public void SetRoiCenter(int centerSpad)
```

#### Vl53lxDevice.StartTemperatureUpdate

```csharp
public void StartTemperatureUpdate()
```

Re-run VHV after an ambient change over 8 °C.

#### Vl53lxDevice.PerformRefSpadManagement

```csharp
public object PerformRefSpadManagement()
```

VL53L0X: re-measure the reference SPADs.

#### Vl53lxDevice.StartRanging

```csharp
public void StartRanging()
```

Start the sensor and arm the board's stream: one report per INT edge, followed on the MCU by the driver's interrupt-release writes.

#### Vl53lxDevice.StopRanging

```csharp
public void StopRanging()
```

#### Vl53lxDevice.MeasureOnce

```csharp
public Vl53lxMeasurement MeasureOnce(TimeSpan? timeout = null)
```

One polled measurement: start, data ready, read, release the interrupt, stop. Not while streaming.

#### Vl53lxDevice.OnMeasurement

```csharp
public IDisposable OnMeasurement(Action<Vl53lxMeasurement> callback)
```

#### Vl53lxDevice.Measurements

```csharp
public Subscription<Vl53lxMeasurement> Measurements(int capacity = 64)
```

#### Vl53lxDevice.GetMeasurement

```csharp
public Vl53lxMeasurement GetMeasurement(TimeSpan? timeout = null)
```

#### Vl53lxDevice.StreamParseErrors *(property)*

```csharp
public long StreamParseErrors
```

Stream reports dropped because the driver did not decode them.

#### Vl53lxDevice.StreamDroppedCounts *(property)*

```csharp
public IReadOnlyList<long> StreamDroppedCounts
```

### Vl53l0xDevice

```csharp
public sealed class Vl53l0xDevice : Vl53lxDevice
```

VL53L0X (2 m, 1-byte register addresses, 400 kHz): ULD API 1.0.4 with ranging profiles.

#### Vl53l0xDevice.Vl53l0xDevice *(constructor)*

```csharp
public Vl53l0xDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

#### Vl53l0xDevice.Vl53l0xDevice *(constructor)*

```csharp
#if NET public Vl53l0xDevice(string port, TimeSpan? timeout = null) : base(port, timeout)
```

### Vl53l1cxDevice

```csharp
public sealed class Vl53l1cxDevice : Vl53lxDevice
```

VL53L1CX (4 m): the VL53L1X ULD (short / long) or the histogram driver.

#### Vl53l1cxDevice.Vl53l1cxDevice *(constructor)*

```csharp
public Vl53l1cxDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

#### Vl53l1cxDevice.Vl53l1cxDevice *(constructor)*

```csharp
#if NET public Vl53l1cxDevice(string port, TimeSpan? timeout = null) : base(port, timeout)
```

### Vl53l1cbDevice

```csharp
public sealed class Vl53l1cbDevice : Vl53lxDevice
```

VL53L1CB (8 m, cover-glass module): the VL53L1CX die and drivers; usually needs a crosstalk calibration.

#### Vl53l1cbDevice.Vl53l1cbDevice *(constructor)*

```csharp
public Vl53l1cbDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

#### Vl53l1cbDevice.Vl53l1cbDevice *(constructor)*

```csharp
#if NET public Vl53l1cbDevice(string port, TimeSpan? timeout = null) : base(port, timeout)
```

### Vl53l3cxDevice

```csharp
public sealed class Vl53l3cxDevice : Vl53lxDevice
```

VL53L3CX (3 m): ST's ULP driver (single target) or the histogram driver (several per frame).

#### Vl53l3cxDevice.Vl53l3cxDevice *(constructor)*

```csharp
public Vl53l3cxDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

#### Vl53l3cxDevice.Vl53l3cxDevice *(constructor)*

```csharp
#if NET public Vl53l3cxDevice(string port, TimeSpan? timeout = null) : base(port, timeout)
```

### Vl53l4cxDevice

```csharp
public sealed class Vl53l4cxDevice : Vl53lxDevice
```

VL53L4CX (6 m): the histogram driver only — to run it light, borrow a sibling's with `Init(product: "VL53L4CD")`.

#### Vl53l4cxDevice.Vl53l4cxDevice *(constructor)*

```csharp
public Vl53l4cxDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

#### Vl53l4cxDevice.Vl53l4cxDevice *(constructor)*

```csharp
#if NET public Vl53l4cxDevice(string port, TimeSpan? timeout = null) : base(port, timeout)
```

### BareRegs

```csharp
public static class BareRegs
```

Register addresses and settings of the BareDriver (vl53lx_register_map.h, vl53lx_ll_device.h).

#### BareRegs.PowerManagementGo1PowerForce *(constant)*

```csharp
public const int PowerManagementGo1PowerForce = 0x0083
```

#### BareRegs.FirmwareEnable *(constant)*

```csharp
public const int FirmwareEnable = 0x0085
```

#### BareRegs.ResultOscCalibrateVal *(constant)*

```csharp
public const int ResultOscCalibrateVal = 0x00DE
```

#### BareRegs.PatchCtrl *(constant)*

```csharp
public const int PatchCtrl = 0x0470
```

#### BareRegs.PatchJmpEnables *(constant)*

```csharp
public const int PatchJmpEnables = 0x0472
```

#### BareRegs.PatchDataEnables *(constant)*

```csharp
public const int PatchDataEnables = 0x0474
```

#### BareRegs.PatchOffset0 *(constant)*

```csharp
public const int PatchOffset0 = 0x0476
```

#### BareRegs.PatchAddress0 *(constant)*

```csharp
public const int PatchAddress0 = 0x0496
```

#### BareRegs.HistogramBinDataI2cIndex *(constant)*

```csharp
public const int HistogramBinDataI2cIndex = 0x0088
```

#### BareRegs.ResultHistogramBin0_2 *(constant)*

```csharp
public const int ResultHistogramBin0_2 = 0x008E
```

#### BareRegs.ResultHistogramBin23_0 *(constant)*

```csharp
public const int ResultHistogramBin23_0 = 0x00D5
```

#### BareRegs.PhasecalResultReferencePhase *(constant)*

```csharp
public const int PhasecalResultReferencePhase = 0x00D6
```

#### BareRegs.PhasecalResultVcselStart *(constant)*

```csharp
public const int PhasecalResultVcselStart = 0x00D8
```

#### BareRegs.ResultHistogramBin23_0Msb *(constant)*

```csharp
public const int ResultHistogramBin23_0Msb = 0x00D9
```

#### BareRegs.ResultHistogramBin23_0Lsb *(constant)*

```csharp
public const int ResultHistogramBin23_0Lsb = 0x00DA
```

#### BareRegs.HistogramBinDataI2cSizeBytes *(constant)*

```csharp
public const int HistogramBinDataI2cSizeBytes = ResultHistogramBin23_0Lsb - HistogramBinDataI2cIndex + 1
```

#### BareRegs.DeviceMeasurementModeBackToBack *(constant)*

```csharp
public const int DeviceMeasurementModeBackToBack = 0x20
```

#### BareRegs.DeviceMeasurementModeAbort *(constant)*

```csharp
public const int DeviceMeasurementModeAbort = 0x80
```

#### BareRegs.DeviceMeasurementModeStopMask *(constant)*

```csharp
public const int DeviceMeasurementModeStopMask = 0x0F
```

#### BareRegs.DeviceMeasurementModeModeMask *(constant)*

```csharp
public const int DeviceMeasurementModeModeMask = 0xF0
```

#### BareRegs.DeviceSchedulerModeStreaming *(constant)*

```csharp
public const int DeviceSchedulerModeStreaming = 0x01
```

#### BareRegs.DeviceSchedulerModeHistogram *(constant)*

```csharp
public const int DeviceSchedulerModeHistogram = 0x02
```

#### BareRegs.DeviceReadoutModeSingleSd *(constant)*

```csharp
public const int DeviceReadoutModeSingleSd = 0x00 << 2
```

#### BareRegs.DeviceReadoutModeDualSd *(constant)*

```csharp
public const int DeviceReadoutModeDualSd = 0x01 << 2
```

#### BareRegs.GroupedParameterHoldIdMask *(constant)*

```csharp
public const int GroupedParameterHoldIdMask = 0x02
```

#### BareRegs.InterruptConfigNewSampleReady *(constant)*

```csharp
public const int InterruptConfigNewSampleReady = 0x20
```

#### BareRegs.ClearRangeInt *(constant)*

```csharp
public const int ClearRangeInt = 0x01
```

#### BareRegs.RangeStatusMask *(constant)*

```csharp
public const int RangeStatusMask = 0x1F
```

#### BareRegs.DeviceInterruptPolarityActiveLow *(constant)*

```csharp
public const int DeviceInterruptPolarityActiveLow = 0x10
```

#### BareRegs.DeviceGpioModeOutputRangeAndErrorInterrupts *(constant)*

```csharp
public const int DeviceGpioModeOutputRangeAndErrorInterrupts = 0x01
```

#### BareRegs.DeviceDssModeTargetRate *(constant)*

```csharp
public const int DeviceDssModeTargetRate = 1
```

#### BareRegs.SequenceVhvEn *(constant)*

```csharp
public const int SequenceVhvEn = 0x01
```

#### BareRegs.SequencePhasecalEn *(constant)*

```csharp
public const int SequencePhasecalEn = 0x02
```

#### BareRegs.SequenceDss1En *(constant)*

```csharp
public const int SequenceDss1En = 0x08
```

#### BareRegs.SequenceDss2En *(constant)*

```csharp
public const int SequenceDss2En = 0x10
```

#### BareRegs.SequenceMm1En *(constant)*

```csharp
public const int SequenceMm1En = 0x20
```

#### BareRegs.SequenceMm2En *(constant)*

```csharp
public const int SequenceMm2En = 0x40
```

#### BareRegs.SequenceRangeEn *(constant)*

```csharp
public const int SequenceRangeEn = 0x80
```

#### BareRegs.SpadArrayWidth *(constant)*

```csharp
public const int SpadArrayWidth = 16
```

#### BareRegs.SpadArrayHeight *(constant)*

```csharp
public const int SpadArrayHeight = 16
```

#### BareRegs.RtnSpadApertureTransmission *(constant)*

```csharp
public const int RtnSpadApertureTransmission = 0x0038
```

#### BareRegs.RtnSpadUnityTransmission *(constant)*

```csharp
public const int RtnSpadUnityTransmission = 0x0100
```

#### BareRegs.AmbientWindowVcselPeriods *(constant)*

```csharp
public const int AmbientWindowVcselPeriods = 256
```

#### BareRegs.RangingWindowVcselPeriods *(constant)*

```csharp
public const int RangingWindowVcselPeriods = 2048
```

#### BareRegs.MacroPeriodVcselPeriods *(constant)*

```csharp
public const int MacroPeriodVcselPeriods = AmbientWindowVcselPeriods + RangingWindowVcselPeriods
```

#### BareRegs.HistogramBufferSize *(constant)*

```csharp
public const int HistogramBufferSize = 24
```

#### BareRegs.DistanceModes *(field)*

```csharp
public static readonly IReadOnlyList<string> DistanceModes = new[] { "short", "medium", "long" }
```

The three preset modes VL53L3CX/VL53L4CX expose, by the distance mode that picks them.

#### BareRegs.L4DistanceModes *(field)*

```csharp
public static readonly IReadOnlyList<string> L4DistanceModes = new[] { "medium", "long" }
```

The L4CX BareDriver (STSW-IMG029) has no short mode on an L4 die.

#### BareRegs.TimingGuardUs *(constant)*

```csharp
public const int TimingGuardUs = 1700
```

#### BareRegs.TimingDivisor *(constant)*

```csharp
public const int TimingDivisor = 6
```

#### BareRegs.FdaMaxTimingBudgetUs *(constant)*

```csharp
public const int FdaMaxTimingBudgetUs = 550000
```

#### BareRegs.L4FdaMaxTimingBudgetUs *(constant)*

```csharp
public const int L4FdaMaxTimingBudgetUs = 200000
```

### BareMath

```csharp
public static class BareMath
```

SPAD geometry and timing maths (vl53lx_core.c, vl53lx_core_support.c).

#### BareMath.EncodeRowCol

```csharp
public static int EncodeRowCol(int row, int col)
```

#### BareMath.static

```csharp
public static (int Row, int Col) DecodeRowCol(int spadNumber)
```

→ (row, col).

#### BareMath.static

```csharp
public static (int Width, int Height) DecodeZoneSize(int encodedXySize)
```

→ (width, height).

#### BareMath.EncodeZoneSize

```csharp
public static int EncodeZoneSize(int width, int height)
```

#### BareMath.static

```csharp
public static (int XLl, int YLl, int XUr, int YUr) DecodeZoneLimits(int encodedXyCentre, int encodedXySize)
```

→ (x_ll, y_ll, x_ur, y_ur), clipped to the SPAD array.

#### BareMath.IsApertureLocation

```csharp
public static bool IsApertureLocation(int row, int col)
```

#### BareMath.static

```csharp
public static (long Inner, long Outer) CalcMmEffectiveSpads(int encodedMmRoiCentre, int encodedMmRoiSize, int encodedZoneCentre, int encodedZoneSize, byte[] goodSpads, long apertureAttenuation)
```

→ (mm inner, mm outer) effective SPADs, in 1/256 of a SPAD. Needed because our boards ship without the FMT offset calibration: the driver works out the SPAD counts the nominal rates apply to.

#### BareMath.CombineMm1Mm2Offsets

```csharp
public static long CombineMm1Mm2Offsets(long mm1OffsetMm, long mm2OffsetMm, int encodedMmRoiCentre, int encodedMmRoiSize, int encodedZoneCentre, int encodedZoneSize, AdditionalOffsetCalData cal, byte[] goodSpads, long apertureAttenuation)
```

`VL53LX_hist_combine_mm1_mm2_offsets()` → `range_offset_mm`, in quarter millimetres: the MM1 (inner) and MM2 (outer) offsets weighted by the share of each region the ROI covers.

#### BareMath.RtnGoodSpads

```csharp
public static byte[] RtnGoodSpads(RegBlock nvmCopyData)
```

The 32 `global_config__spad_enables_rtn_*` bytes as one bitmap.

#### BareMath.CalcPllPeriodUs

```csharp
public static long CalcPllPeriodUs(long fastOscFrequency)
```

#### BareMath.DecodeVcselPeriod

```csharp
public static long DecodeVcselPeriod(long vcselPeriodReg)
```

#### BareMath.CalcMacroPeriodUs

```csharp
public static long CalcMacroPeriodUs(long fastOscFrequency, long vcselPeriodReg)
```

#### BareMath.CalcTimeoutMclks

```csharp
public static long CalcTimeoutMclks(long timeoutUs, long macroPeriodUs)
```

#### BareMath.EncodeTimeout

```csharp
public static long EncodeTimeout(long timeoutMclks)
```

#### BareMath.DecodeTimeout

```csharp
public static long DecodeTimeout(long encoded)
```

#### BareMath.CalcEncodedTimeout

```csharp
public static long CalcEncodedTimeout(long timeoutUs, long macroPeriodUs)
```

#### BareMath.DurationMaths

```csharp
public static long DurationMaths(long pllPeriodUs, long vcselParmPclks, long windowVclks, long elapsedMclks)
```

#### BareMath.CalcTimeoutRegisterValues

```csharp
public static void CalcTimeoutRegisterValues(long phasecalUs, long mmUs, long rangeUs, long fastOscFrequency, RegBlock gen, RegBlock tim)
```

Fill the six timeout registers, for VCSEL period A and then B.

### HistConfig

```csharp
public sealed class HistConfig
```

`VL53LX_histogram_config_t`. It has no registers of its own: the bin sequence is smuggled into static_config and timing_config fields that mean something else in lite mode (`CopyToStaticCfg`).

#### HistConfig.LowAmbEvenBin01 *(field)*

```csharp
public long LowAmbEvenBin01
```

#### HistConfig.LowAmbEvenBin23 *(field)*

```csharp
public long LowAmbEvenBin23
```

#### HistConfig.LowAmbEvenBin45 *(field)*

```csharp
public long LowAmbEvenBin45
```

#### HistConfig.LowAmbOddBin01 *(field)*

```csharp
public long LowAmbOddBin01
```

#### HistConfig.LowAmbOddBin23 *(field)*

```csharp
public long LowAmbOddBin23
```

#### HistConfig.LowAmbOddBin45 *(field)*

```csharp
public long LowAmbOddBin45
```

#### HistConfig.MidAmbEvenBin01 *(field)*

```csharp
public long MidAmbEvenBin01
```

#### HistConfig.MidAmbEvenBin23 *(field)*

```csharp
public long MidAmbEvenBin23
```

#### HistConfig.MidAmbEvenBin45 *(field)*

```csharp
public long MidAmbEvenBin45
```

#### HistConfig.MidAmbOddBin01 *(field)*

```csharp
public long MidAmbOddBin01
```

#### HistConfig.MidAmbOddBin2 *(field)*

```csharp
public long MidAmbOddBin2
```

#### HistConfig.MidAmbOddBin34 *(field)*

```csharp
public long MidAmbOddBin34
```

#### HistConfig.MidAmbOddBin5 *(field)*

```csharp
public long MidAmbOddBin5
```

#### HistConfig.UserBinOffset *(field)*

```csharp
public long UserBinOffset
```

#### HistConfig.HighAmbEvenBin01 *(field)*

```csharp
public long HighAmbEvenBin01
```

#### HistConfig.HighAmbEvenBin23 *(field)*

```csharp
public long HighAmbEvenBin23
```

#### HistConfig.HighAmbEvenBin45 *(field)*

```csharp
public long HighAmbEvenBin45
```

#### HistConfig.HighAmbOddBin01 *(field)*

```csharp
public long HighAmbOddBin01
```

#### HistConfig.HighAmbOddBin23 *(field)*

```csharp
public long HighAmbOddBin23
```

#### HistConfig.HighAmbOddBin45 *(field)*

```csharp
public long HighAmbOddBin45
```

#### HistConfig.AmbThreshLow *(field)*

```csharp
public long AmbThreshLow
```

#### HistConfig.AmbThreshHigh *(field)*

```csharp
public long AmbThreshHigh
```

#### HistConfig.SpadArraySelection *(field)*

```csharp
public long SpadArraySelection
```

#### HistConfig.SetBinSequence

```csharp
public void SetBinSequence(IReadOnlyList<int> even, IReadOnlyList<int> odd)
```

VL53LX_init_histogram_config_structure(): six even and six odd bin codes, packed two to a byte, repeated across the three ambient levels.

#### HistConfig.CopyToStaticCfg

```csharp
public void CopyToStaticCfg(RegBlock stat, RegBlock timing, RegBlock dynamic)
```

VL53LX_copy_hist_cfg_to_static_cfg(): lite-mode fields the firmware reads as the bin sequence.

#### HistConfig.BinSequence

```csharp
public int[] BinSequence(long streamCount, long ambientEventsSum)
```

VL53LX_hist_get_bin_sequence_config(): which sequence the device used for this frame, even or odd by the stream count.

### HistPostProcessConfig

```csharp
public sealed class HistPostProcessConfig
```

`VL53LX_hist_post_process_config_t`, as `VL53LX_init_hist_post_process_config_struct()` leaves it; the preset modes set the valid phase window here.

#### HistPostProcessConfig.HistAlgoSelect *(field)*

```csharp
public long HistAlgoSelect = D("TUNINGPARM_HIST_ALGO_SELECT_DEFAULT")
```

#### HistPostProcessConfig.HistTargetOrder *(field)*

```csharp
public long HistTargetOrder = D("TUNINGPARM_HIST_TARGET_ORDER_DEFAULT")
```

#### HistPostProcessConfig.FilterWoi0 *(field)*

```csharp
public long FilterWoi0 = D("TUNINGPARM_HIST_FILTER_WOI_0_DEFAULT")
```

#### HistPostProcessConfig.FilterWoi1 *(field)*

```csharp
public long FilterWoi1 = D("TUNINGPARM_HIST_FILTER_WOI_1_DEFAULT")
```

#### HistPostProcessConfig.HistAmbEstMethod *(field)*

```csharp
public long HistAmbEstMethod = D("TUNINGPARM_HIST_AMB_EST_METHOD_DEFAULT")
```

#### HistPostProcessConfig.AmbientThreshSigma0 *(field)*

```csharp
public long AmbientThreshSigma0 = D("TUNINGPARM_HIST_AMB_THRESH_SIGMA_0_DEFAULT")
```

#### HistPostProcessConfig.AmbientThreshSigma1 *(field)*

```csharp
public long AmbientThreshSigma1 = D("TUNINGPARM_HIST_AMB_THRESH_SIGMA_1_DEFAULT")
```

#### HistPostProcessConfig.AmbientThreshEventsScaler *(field)*

```csharp
public long AmbientThreshEventsScaler = D("TUNINGPARM_HIST_AMB_EVENTS_SCALER_DEFAULT")
```

#### HistPostProcessConfig.MinAmbientThreshEvents *(field)*

```csharp
public long MinAmbientThreshEvents = D("TUNINGPARM_HIST_MIN_AMB_THRESH_EVENTS_DEFAULT")
```

#### HistPostProcessConfig.NoiseThreshold *(field)*

```csharp
public long NoiseThreshold = D("TUNINGPARM_HIST_NOISE_THRESHOLD_DEFAULT")
```

#### HistPostProcessConfig.SignalTotalEventsLimit *(field)*

```csharp
public long SignalTotalEventsLimit = D("TUNINGPARM_HIST_SIGNAL_TOTAL_EVENTS_LIMIT_DEFAULT")
```

#### HistPostProcessConfig.SigmaEstimatorSigmaRefMm *(field)*

```csharp
public long SigmaEstimatorSigmaRefMm = D("TUNINGPARM_HIST_SIGMA_EST_REF_MM_DEFAULT")
```

#### HistPostProcessConfig.SigmaThresh *(field)*

```csharp
public long SigmaThresh = D("TUNINGPARM_HIST_SIGMA_THRESH_MM_DEFAULT")
```

#### HistPostProcessConfig.RangeOffsetMm *(field)*

```csharp
public long RangeOffsetMm
```

#### HistPostProcessConfig.GainFactor *(field)*

```csharp
public long GainFactor = D("TUNINGPARM_HIST_GAIN_FACTOR_DEFAULT")
```

#### HistPostProcessConfig.ValidPhaseLow *(field)*

```csharp
public long ValidPhaseLow = 0x08
```

#### HistPostProcessConfig.ValidPhaseHigh *(field)*

```csharp
public long ValidPhaseHigh = 0x88
```

#### HistPostProcessConfig.ConsistencyPhaseTolerance *(field)*

```csharp
public long ConsistencyPhaseTolerance = D("TUNINGPARM_CONSISTENCY_HIST_PHASE_TOLERANCE_DEFAULT")
```

#### HistPostProcessConfig.ConsistencyEventSigma *(field)*

```csharp
public long ConsistencyEventSigma = D("TUNINGPARM_CONSISTENCY_HIST_EVENT_SIGMA_DEFAULT")
```

#### HistPostProcessConfig.ConsistencyEventMinSpadCount *(field)*

```csharp
public long ConsistencyEventMinSpadCount = …
```

#### HistPostProcessConfig.ConsistencyMinMaxTolerance *(field)*

```csharp
public long ConsistencyMinMaxTolerance = …
```

#### HistPostProcessConfig.CrosstalkCompensationEnable *(field)*

```csharp
public long CrosstalkCompensationEnable
```

#### HistPostProcessConfig.CrosstalkDetectMinValidRangeMm *(field)*

```csharp
public long CrosstalkDetectMinValidRangeMm = D("TUNINGPARM_XTALK_DETECT_MIN_VALID_RANGE_MM_DEFAULT")
```

#### HistPostProcessConfig.CrosstalkDetectMaxValidRangeMm *(field)*

```csharp
public long CrosstalkDetectMaxValidRangeMm = D("TUNINGPARM_XTALK_DETECT_MAX_VALID_RANGE_MM_DEFAULT")
```

#### HistPostProcessConfig.CrosstalkDetectMaxValidRateKcps *(field)*

```csharp
public long CrosstalkDetectMaxValidRateKcps = D("TUNINGPARM_XTALK_DETECT_MAX_VALID_RATE_KCPS_DEFAULT")
```

#### HistPostProcessConfig.CrosstalkDetectMaxSigmaMm *(field)*

```csharp
public long CrosstalkDetectMaxSigmaMm = D("TUNINGPARM_XTALK_DETECT_MAX_SIGMA_MM_DEFAULT")
```

#### HistPostProcessConfig.CrosstalkDetectEventSigma *(field)*

```csharp
public long CrosstalkDetectEventSigma = D("TUNINGPARM_XTALK_DETECT_EVENT_SIGMA_DEFAULT")
```

#### HistPostProcessConfig.CrosstalkDetectMinMaxTolerance *(field)*

```csharp
public long CrosstalkDetectMinMaxTolerance = D("TUNINGPARM_XTALK_DETECT_MIN_MAX_TOLERANCE_DEFAULT")
```

#### HistPostProcessConfig.CrosstalkCompensationPlaneOffsetKcps *(field)*

```csharp
public long CrosstalkCompensationPlaneOffsetKcps
```

#### HistPostProcessConfig.CrosstalkCompensationXPlaneGradientKcps *(field)*

```csharp
public long CrosstalkCompensationXPlaneGradientKcps
```

#### HistPostProcessConfig.CrosstalkCompensationYPlaneGradientKcps *(field)*

```csharp
public long CrosstalkCompensationYPlaneGradientKcps
```

### HistogramBinData

```csharp
public sealed class HistogramBinData
```

`VL53LX_histogram_bin_data_t`: 24 bins plus everything the post-processing needs to read them — the VCSEL period they were taken at, the phase of zero distance, the bin sequence and the ambient estimate.

#### HistogramBinData.ResultInterruptStatus *(field)*

```csharp
public long ResultInterruptStatus
```

#### HistogramBinData.ResultRangeStatus *(field)*

```csharp
public long ResultRangeStatus
```

#### HistogramBinData.ResultReportStatus *(field)*

```csharp
public long ResultReportStatus
```

#### HistogramBinData.ResultStreamCount *(field)*

```csharp
public long ResultStreamCount
```

#### HistogramBinData.ResultDssActualEffectiveSpads *(field)*

```csharp
public long ResultDssActualEffectiveSpads
```

#### HistogramBinData.PhasecalResultReferencePhase *(field)*

```csharp
public long PhasecalResultReferencePhase
```

#### HistogramBinData.PhasecalResultVcselStart *(field)*

```csharp
public long PhasecalResultVcselStart
```

#### HistogramBinData.BinData *(field)*

```csharp
public long[] BinData = new long[BareRegs.HistogramBufferSize]
```

#### HistogramBinData.ZoneId *(field)*

```csharp
public long ZoneId
```

#### HistogramBinData.VL53LX_p_019 *(field)*

```csharp
public long FirstBin
```

#### HistogramBinData.NumberOfBins *(field)*

```csharp
public long NumberOfBins = BareRegs.HistogramBufferSize
```

#### HistogramBinData.BinsInData *(field)*

```csharp
public long BinsInData = BareRegs.HistogramBufferSize
```

#### HistogramBinData.CalConfigVcselStart *(field)*

```csharp
public long CalConfigVcselStart
```

#### HistogramBinData.VcselWidth *(field)*

```csharp
public long VcselWidth
```

#### HistogramBinData.VL53LX_p_015 *(field)*

```csharp
public long FastOscFrequency
```

#### HistogramBinData.value *(field)*

```csharp
public long VcselPeriod
```

#### HistogramBinData.BinSeq *(field)*

```csharp
public int[] BinSeq = new int[6]
```

#### HistogramBinData.BinRep *(field)*

```csharp
public long[] BinRep = new long[6]
```

#### HistogramBinData.MinBinValue *(field)*

```csharp
public long MinBinValue
```

#### HistogramBinData.MaxBinValue *(field)*

```csharp
public long MaxBinValue
```

#### HistogramBinData.NumberOfAmbientBins *(field)*

```csharp
public long NumberOfAmbientBins
```

#### HistogramBinData.NumberOfAmbientSamples *(field)*

```csharp
public long NumberOfAmbientSamples
```

#### HistogramBinData.AmbientEventsSum *(field)*

```csharp
public long AmbientEventsSum
```

#### HistogramBinData.VL53LX_p_028 *(field)*

```csharp
public long AmbientPerBin
```

#### HistogramBinData.TotalPeriodsElapsed *(field)*

```csharp
public long TotalPeriodsElapsed
```

#### HistogramBinData.PeakDurationUs *(field)*

```csharp
public long PeakDurationUs
```

#### HistogramBinData.WoiDurationUs *(field)*

```csharp
public long WoiDurationUs
```

#### HistogramBinData.ZeroDistancePhase *(field)*

```csharp
public long ZeroDistancePhase
```

#### HistogramBinData.RoiConfigUserRoiCentreSpad *(field)*

```csharp
public long RoiConfigUserRoiCentreSpad
```

#### HistogramBinData.RoiConfigUserRoiRequestedGlobalXySize *(field)*

```csharp
public long RoiConfigUserRoiRequestedGlobalXySize
```

#### HistogramBinData.RangeStatus *(property)*

```csharp
public long RangeStatus
```

#### HistogramBinData.Clone

```csharp
public HistogramBinData Clone()
```

#### HistogramBinData.Fields

```csharp
public IReadOnlyDictionary<string, object?> Fields()
```

The fields under the C driver's (and the Python SDK's) names.

#### HistogramBinData.ToString

```csharp
public override string ToString()
```

### BareDeviceState

```csharp
public enum BareDeviceState
{
    SwStandby,
    RangingWaitGphSync,
    RangingOutputData,
    RangingDssAuto,
}
```

The ll-driver device states, single-zone case.

### LlState

```csharp
public sealed class LlState
```

`VL53LX_update_ll_driver_{rd,cfg}_state()`, reduced to one zone: the GPH handshake (the id stamped into the dynamic config tells the device a configuration is new) and the timing-status flip (whether a frame ran on VCSEL period A or B, which the post-processing has to know).

#### LlState.CfgDeviceState *(field)*

```csharp
public BareDeviceState CfgDeviceState
```

#### LlState.CfgStreamCount *(field)*

```csharp
public long CfgStreamCount
```

#### LlState.CfgGphId *(field)*

```csharp
public long CfgGphId
```

#### LlState.CfgTimingStatus *(field)*

```csharp
public long CfgTimingStatus
```

#### LlState.RdDeviceState *(field)*

```csharp
public BareDeviceState RdDeviceState
```

#### LlState.RdStreamCount *(field)*

```csharp
public long RdStreamCount
```

#### LlState.RdGphId *(field)*

```csharp
public long RdGphId
```

#### LlState.RdTimingStatus *(field)*

```csharp
public long RdTimingStatus
```

#### LlState.LlState *(constructor)*

```csharp
public LlState()
```

#### LlState.Reset

```csharp
public void Reset()
```

#### LlState.UpdateRd

```csharp
public void UpdateRd(long modeStart, long groupedParameterHold)
```

#### LlState.UpdateCfg

```csharp
public void UpdateCfg(long modeStart)
```

### BareDriver

```csharp
public sealed class BareDriver
```

ST's VL53LX BareDriver ranging path, on top of `DeviceImage`: `VL53LX_read_p2p_data()`, the histogram preset modes, `VL53LX_set_timeouts_us()`, `VL53LX_init_and_start_range()` and `VL53LX_get_histogram_bin_data()`. Nothing here writes a register on its own: the preset builders edit the host-side image and the whole span 0x0001..0x0087 goes out as one transfer when ranging starts.

#### BareDriver.BareDriver *(constructor)*

```csharp
public BareDriver(Vl53lxPlatform platform)
```

#### BareDriver.Img *(property)*

```csharp
public DeviceImage Img
```

#### BareDriver.Nvm *(property)*

```csharp
public NvmReader Nvm
```

#### BareDriver.Tuning *(property)*

```csharp
public Dictionary<string, long> Tuning
```

#### BareDriver.HistCfg *(property)*

```csharp
public HistConfig HistCfg
```

#### BareDriver.Hpp *(property)*

```csharp
public HistPostProcessConfig Hpp
```

#### BareDriver.RtnGoodSpadsMap *(property)*

```csharp
public byte[] RtnGoodSpadsMap
```

#### BareDriver.OpticalCentre *(property)*

```csharp
public OpticalCentre? OpticalCentre
```

#### BareDriver.CalPeakRateMap *(property)*

```csharp
public CalPeakRateMap? CalPeakRateMap
```

#### BareDriver.AddOffCalData *(property)*

```csharp
public AdditionalOffsetCalData? AddOffCalData
```

#### BareDriver.FmtDmaxCal *(property)*

```csharp
public IReadOnlyDictionary<string, long>? FmtDmaxCal
```

#### BareDriver.MmRoi *(property)*

```csharp
public IReadOnlyDictionary<string, long>? MmRoi
```

#### BareDriver.ResultOscCalibrateVal *(property)*

```csharp
public long ResultOscCalibrateVal
```

#### BareDriver.PresetMode *(property)*

```csharp
public string PresetMode
```

#### BareDriver.PhasecalConfigTimeoutUs *(property)*

```csharp
public long PhasecalConfigTimeoutUs
```

#### BareDriver.MmConfigTimeoutUs *(property)*

```csharp
public long MmConfigTimeoutUs
```

#### BareDriver.RangeConfigTimeoutUs *(property)*

```csharp
public long RangeConfigTimeoutUs
```

#### BareDriver.InterMeasurementPeriodMs *(property)*

```csharp
public long InterMeasurementPeriodMs
```

#### BareDriver.MeasurementMode *(property)*

```csharp
public long MeasurementMode
```

#### BareDriver.State *(property)*

```csharp
public LlState State
```

#### BareDriver.ReadP2pData

```csharp
public void ReadP2pData()
```

VL53LX_read_p2p_data(): three register blocks and four NVM regions (~0.1 s over USB).

#### BareDriver.DataInit

```csharp
public void DataInit()
```

VL53LX_data_init + VL53LX_DataInit: the factory data, the default preset mode and timing budget. Leaves the sensor untouched apart from the reads.

#### BareDriver.SetPresetMode

```csharp
public void SetPresetMode(string mode = "medium", long? dssTargetMcps = null, long? interMeasurementPeriodMs = null)
```

VL53LX_set_preset_mode() for the three histogram ranging modes, by distance mode.

#### BareDriver.IsL4

```csharp
public bool IsL4()
```

`IsL4()` of the L4CX BareDriver: the die, read from NVM, not the product name. 0xEC is the L4ED.

#### BareDriver.DistanceModesForDie

```csharp
public IReadOnlyList<string> DistanceModesForDie()
```

#### BareDriver.FdaMaxTimingBudgetUs

```csharp
public long FdaMaxTimingBudgetUs()
```

#### BareDriver.SetDistanceMode

```csharp
public void SetDistanceMode(string mode)
```

VL53LX_SetDistanceMode(): the preset mode with the caller's timeouts put back. Short is refused on an L4 die, as the L4CX BareDriver does (the A frame of the pair ranges on the wrong side of the wrap).

#### BareDriver.SetTimeoutsUs

```csharp
public void SetTimeoutsUs(long phasecalUs, long mmUs, long rangeUs)
```

#### BareDriver.SetInterMeasurementPeriodMs

```csharp
public void SetInterMeasurementPeriodMs(long periodMs)
```

#### BareDriver.SetMeasurementTimingBudgetUs

```csharp
public void SetMeasurementTimingBudgetUs(long budgetUs)
```

VL53LX_SetMeasurementTimingBudgetMicroSeconds(): the range timeout is one sixth of the budget after a fixed guard comes off.

#### BareDriver.LoadPatch

```csharp
public void LoadPatch()
```

VL53LX_load_patch(): a six-instruction phasecal patch, written with the firmware stopped.

#### BareDriver.UnloadPatch

```csharp
public void UnloadPatch()
```

#### BareDriver.InitAndStartRange

```csharp
public void InitAndStartRange(IReadOnlyList<string>? blocks = null)
```

VL53LX_init_and_start_range(): write the configuration and start. `blocks` is ST's config level — all of them to start, general_config onwards to let the next frame run.

#### BareDriver.ClearInterruptAndStartNextRange

```csharp
public void ClearInterruptAndStartNextRange()
```

VL53LX_clear_interrupt_and_enable_next_range(): in histogram mode the interrupt is cleared by rewriting general_config onwards.

#### BareDriver.StopRange

```csharp
public void StopRange()
```

#### BareDriver.GetHistogramBinData

```csharp
public HistogramBinData GetHistogramBinData(byte[]? raw = null)
```

VL53LX_get_histogram_bin_data(): one frame, 83 bytes, decoded into bins plus the metadata the post-processing needs. `raw` is bytes that arrived some other way (a stream report).

### BareTables

```csharp
public static partial class BareTables
```

#### BareTables.Blocks *(field)*

```csharp
public static readonly IReadOnlyDictionary<string, RegBlockDef> Blocks = …
```

The register blocks of the device image: base, size, writable and the fields with offset, width, signedness and the mask the C codec applies (null: none).

#### BareTables.Tuning *(field)*

```csharp
public static readonly IReadOnlyDictionary<string, long> Tuning = …
```

`VL53LX_tuning_parm_storage_t` after initialisation, keyed by the C field name.

#### BareTables.Defaults *(field)*

```csharp
public static readonly IReadOnlyDictionary<string, long> Defaults = …
```

The raw tuning header, for the parameters the post-processing reads without a `tp_` field.

### RegField

```csharp
public sealed record RegField(string Name, int Off, int Width, bool Signed, long? Mask)
```

One field of a register block: byte offset, width in bytes, signedness and the mask the C codec applies (null: none).

### RegBlockDef

```csharp
public sealed record RegBlockDef(int Base, int Size, bool Writable, IReadOnlyList<RegField> Fields)
```

One contiguous I2C block of the device image (a `VL53LX_*_t` struct of the C driver).

### RegBlock

```csharp
public sealed class RegBlock
```

One register block's worth of device image, addressed by the C field name: `img.StaticConfig["dss_config__target_total_rate_mcps"]`. Bytes no field names (the reserved holes) keep whatever was read, so a read-modify-write round trip is byte-exact.

#### RegBlock.RegBlock *(constructor)*

```csharp
public RegBlock(string name)
```

#### RegBlock.Name *(property)*

```csharp
public string Name
```

#### RegBlock.Def *(property)*

```csharp
public RegBlockDef Def
```

#### RegBlock.Base *(property)*

```csharp
public int Base
```

#### RegBlock.Size *(property)*

```csharp
public int Size
```

#### RegBlock.field *(property)*

```csharp
public long this[string field]
```

#### RegBlock.Items

```csharp
public IEnumerable<KeyValuePair<string, long>> Items()
```

#### RegBlock.Decode

```csharp
public RegBlock Decode(byte[] raw)
```

#### RegBlock.Encode

```csharp
public byte[] Encode()
```

#### RegBlock.ToString

```csharp
public override string ToString()
```

### DeviceImage

```csharp
public sealed class DeviceImage
```

The device's register image, block by block. The C driver never reads a configuration register on its own: it edits the structs, then pushes whole blocks — ranging start writes 0x0001..0x0087 in a single 135-byte transfer. Every transfer costs a USB round trip, so block moves keep the count where the C driver has it. Editing a field only touches the host copy; `Push` reaches the sensor.

#### DeviceImage.RangeStartBlocks *(field)*

```csharp
public static readonly IReadOnlyList<string> RangeStartBlocks = …
```

The blocks `init_and_start_range` pushes, in address order: one transfer.

#### DeviceImage.DeviceImage *(constructor)*

```csharp
public DeviceImage(Vl53lxPlatform platform)
```

#### DeviceImage.name *(property)*

```csharp
public RegBlock this[string name]
```

#### DeviceImage.StaticNvm *(property)*

```csharp
public RegBlock StaticNvm
```

#### DeviceImage.CustomerNvm *(property)*

```csharp
public RegBlock CustomerNvm
```

#### DeviceImage.StaticConfig *(property)*

```csharp
public RegBlock StaticConfig
```

#### DeviceImage.GeneralConfig *(property)*

```csharp
public RegBlock GeneralConfig
```

#### DeviceImage.TimingConfig *(property)*

```csharp
public RegBlock TimingConfig
```

#### DeviceImage.DynamicConfig *(property)*

```csharp
public RegBlock DynamicConfig
```

#### DeviceImage.SystemControl *(property)*

```csharp
public RegBlock SystemControl
```

#### DeviceImage.NvmCopyData *(property)*

```csharp
public RegBlock NvmCopyData
```

#### DeviceImage.Pull

```csharp
public RegBlock Pull(string name)
```

Read one block off the sensor into the image.

#### DeviceImage.Push

```csharp
public void Push(string name)
```

Write one block from the image to the sensor.

#### DeviceImage.PullRange

```csharp
public void PullRange(IReadOnlyList<string> names)
```

#### DeviceImage.PushRange

```csharp
public void PushRange(IReadOnlyList<string> names)
```

The named blocks as one transfer; a hole between them goes out as 0x00, as in the C driver.

#### DeviceImage.Dump

```csharp
public string Dump(IEnumerable<string>? names = null)
```

Every field of the named blocks (default: all), one per line.

### OpticalCentre

```csharp
public sealed record OpticalCentre(int XCentre, int YCentre)
```

`VL53LX_optical_centre_t`.

### CalPeakRateMap

```csharp
public sealed record CalPeakRateMap(int CalDistanceMm, int CalReflectancePc, int MaxSamples, int Width, int Height, IReadOnlyList<int> PeakRateMcps)
```

`VL53LX_cal_peak_rate_map_t`.

### AdditionalOffsetCalData

```csharp
public sealed record AdditionalOffsetCalData(long MmInnerActualEffectiveSpads, long MmOuterActualEffectiveSpads, long MmInnerPeakSignalCountRtnMcps, long MmOuterPeakSignalCountRtnMcps)
```

`VL53LX_additional_offset_cal_data_t`.

### FmtRangeData

```csharp
public sealed record FmtRangeData(int ActualEffectiveRtnSpads, int NumRequestedRefSpads, int RefLocation, int PeakSignalCountRateRtnMcps, int AmbientCountRateRtnMcps, int PeakSignalCountRateRefMcps, int AmbientCountRateRefMcps, int MeasuredDistanceMm, int MeasuredDistanceStdevMm)
```

`VL53LX_decoded_nvm_fmt_range_data_t`.

### NvmReader

```csharp
public sealed class NvmReader
```

The raw NVM, which the register blocks do not cover: the optical centre, the 5x5 peak-rate map, the mode-mitigation offset calibration and the FMT range results. Each word costs four writes and a read, so only the regions the driver wants are read, each inside its own enable/disable pair so the sensor is never left with its firmware stopped (vl53lx_nvm.c).

#### NvmReader.readonly

```csharp
public static readonly (int Index, int Size) FmtOpticalCentre = (0x00B8, 4)
```

#### NvmReader.readonly

```csharp
public static readonly (int Index, int Size) FmtCalPeakRateMap = (0x015C, 56)
```

#### NvmReader.readonly

```csharp
public static readonly (int Index, int Size) FmtAdditionalOffsetCal = (0x0194, 8)
```

#### NvmReader.readonly

```csharp
public static readonly (int Index, int Size) FmtRangeResults140mmDark = (0x01AC, 16)
```

#### NvmReader.NvmReader *(constructor)*

```csharp
public NvmReader(Vl53lxPlatform platform, DeviceImage image)
```

#### NvmReader.ReadRegion

```csharp
public byte[] ReadRegion((int Index, int Size) region)
```

VL53LX_read_nvm_raw_data(): (byte index, byte size), both word-aligned → the bytes.

#### NvmReader.ReadOpticalCentre

```csharp
public OpticalCentre ReadOpticalCentre()
```

#### NvmReader.ReadCalPeakRateMap

```csharp
public CalPeakRateMap ReadCalPeakRateMap()
```

#### NvmReader.ReadAdditionalOffsetCalData

```csharp
public AdditionalOffsetCalData ReadAdditionalOffsetCalData()
```

#### NvmReader.ReadFmtRangeResults

```csharp
public FmtRangeData ReadFmtRangeResults((int Index, int Size)? region = null)
```

### RangeData

```csharp
public sealed class RangeData
```

`VL53LX_range_data_t`: one target. The event sums and phases are what the algorithm produced; the `*RangeMm` fields and the rates are what they mean.

#### RangeData.RangeId *(field)*

```csharp
public long RangeId
```

#### RangeData.start *(field)*

```csharp
public long StartBin
```

#### RangeData.threshold *(field)*

```csharp
public long FirstBin
```

#### RangeData.VL53LX_p_023 *(field)*

```csharp
public long PeakBin
```

#### RangeData.VL53LX_p_024 *(field)*

```csharp
public long LastBin
```

#### RangeData.VL53LX_p_013 *(field)*

```csharp
public long EndBin
```

#### RangeData.threshold *(field)*

```csharp
public long WidthBins
```

#### RangeData.window *(field)*

```csharp
public long WindowBins
```

#### RangeData.VcselWidth *(field)*

```csharp
public long VcselWidth
```

#### RangeData.FastOscFrequency *(field)*

```csharp
public long FastOscFrequency
```

#### RangeData.ZeroDistancePhase *(field)*

```csharp
public long ZeroDistancePhase
```

#### RangeData.VL53LX_p_004 *(field)*

```csharp
public long Spads
```

#### RangeData.TotalPeriodsElapsed *(field)*

```csharp
public long TotalPeriodsElapsed
```

#### RangeData.PeakDurationUs *(field)*

```csharp
public long PeakDurationUs
```

#### RangeData.WoiDurationUs *(field)*

```csharp
public long WoiDurationUs
```

#### RangeData.VL53LX_p_016 *(field)*

```csharp
public long AmbientEvents
```

#### RangeData.VL53LX_p_017 *(field)*

```csharp
public long TotalEvents
```

#### RangeData.VL53LX_p_010 *(field)*

```csharp
public long SignalEvents
```

#### RangeData.PeakSignalCountRateMcps *(field)*

```csharp
public long PeakSignalCountRateMcps
```

#### RangeData.AvgSignalCountRateMcps *(field)*

```csharp
public long AvgSignalCountRateMcps
```

#### RangeData.AmbientCountRateMcps *(field)*

```csharp
public long AmbientCountRateMcps
```

#### RangeData.TotalRatePerSpadMcps *(field)*

```csharp
public long TotalRatePerSpadMcps
```

#### RangeData.VL53LX_p_009 *(field)*

```csharp
public long SignalEventsPerSpadKcps
```

#### RangeData.mm *(field)*

```csharp
public long Sigma
```

#### RangeData.VL53LX_p_026 *(field)*

```csharp
public long PhaseStart
```

#### RangeData.VL53LX_p_011 *(field)*

```csharp
public long PhaseMean
```

#### RangeData.VL53LX_p_027 *(field)*

```csharp
public long PhaseEnd
```

#### RangeData.MinRangeMm *(field)*

```csharp
public long MinRangeMm
```

#### RangeData.MedianRangeMm *(field)*

```csharp
public long MedianRangeMm
```

#### RangeData.MaxRangeMm *(field)*

```csharp
public long MaxRangeMm
```

#### RangeData.RangeStatus *(field)*

```csharp
public long RangeStatus = Hist.DeviceErrorNoUpdate
```

#### RangeData.SigmaMm *(property)*

```csharp
public long SigmaMm
```

#### RangeData.SignalKcps *(property)*

```csharp
public long SignalKcps
```

The peak signal rate is 9.7 Mcps.

#### RangeData.AmbientKcps *(property)*

```csharp
public long AmbientKcps
```

#### RangeData.ToString

```csharp
public override string ToString()
```

### RangeResults

```csharp
public sealed class RangeResults
```

`VL53LX_range_results_t`, reduced to the single-zone case.

#### RangeResults.StreamCount *(field)*

```csharp
public long StreamCount
```

#### RangeResults.Targets *(property)*

```csharp
public List<RangeData> Targets
```

#### RangeResults.AmbientDmaxMm *(field)*

```csharp
public long[] AmbientDmaxMm = new long[5]
```

#### RangeResults.WrapDmaxMm *(field)*

```csharp
public long WrapDmaxMm
```

### FrameHistory

```csharp
public sealed class FrameHistory
```

The frame before this one, reduced to what the consistency checks read (`VL53LX_zone_hist_info_t` and `VL53LX_zone_objects_t` for one zone). The engine alternates two VCSEL periods, so one frame cannot say whether a target is inside the unambiguous range or an echo wrapped into it; the API compares consecutive frames. The BareDriver keeps no history, so the driver owns one of these and hands it every frame.

#### FrameHistory.Reset

```csharp
public void Reset()
```

Forget the stream so far: the next `Apply` only remembers.

#### FrameHistory.Apply

```csharp
public void Apply(RangeResults results, HistogramBinData bins, BareDeviceState rdDeviceState, HistPostProcessConfig hpp)
```

Check `results` against the remembered frame, then remember it — C's order: both checks run before hist_data and range_results are copied into the zone history.

### Hist

```csharp
public static class Hist
```

Histogram post-processing: 24 bins → targets, on the host alone, as `VL53LX_hist_process_data()` does in C. Fold repeated bin codes, estimate the ambient floor, drop the ambient-only bins, threshold each bin, take the runs of marked bins as pulses; each pulse is isolated, filtered with a three-tap window and its zero crossing interpolated to a sub-bin phase, which converts to millimetres. ST's source names its functions `VL53LX_f_NNN`; the names here are ours, the C name is in the comment. Crosstalk compensation and the ambient dmax are not ported (dmax needs FMT data our boards ship blank, so it would be zero anyway). Arithmetic is the C driver's: truncating division, and saturation where it caps an intermediate.

#### Hist.MaxBinSequenceLength *(constant)*

```csharp
public const int MaxBinSequenceLength = 6
```

#### Hist.MaxBinSequenceCode *(constant)*

```csharp
public const int MaxBinSequenceCode = 15
```

#### Hist.MaxPulses *(constant)*

```csharp
public const int MaxPulses = 8
```

#### Hist.MaxRangeResults *(constant)*

```csharp
public const int MaxRangeResults = 4
```

#### Hist.MaxAllowedPhase *(constant)*

```csharp
public const long MaxAllowedPhase = 0xFFFF
```

#### Hist.SpadTotalCountMax *(constant)*

```csharp
public const long SpadTotalCountMax = (1L << 29) - 1
```

#### Hist.SpadTotalCountResThres *(constant)*

```csharp
public const long SpadTotalCountResThres = 1L << 24
```

#### Hist.SpeedOfLightInAirDiv8 *(constant)*

```csharp
public const long SpeedOfLightInAirDiv8 = 299704 >> 3
```

#### Hist.SigmaInvalid *(constant)*

```csharp
public const long SigmaInvalid = 0xFFFF
```

#### Hist.DeviceErrorNoUpdate *(constant)*

```csharp
public const long DeviceErrorNoUpdate = 0
```

#### Hist.DeviceErrorRangePhaseCheck *(constant)*

```csharp
public const long DeviceErrorRangePhaseCheck = 5
```

#### Hist.DeviceErrorSigmaThresholdCheck *(constant)*

```csharp
public const long DeviceErrorSigmaThresholdCheck = 6
```

#### Hist.DeviceErrorPhaseConsistency *(constant)*

```csharp
public const long DeviceErrorPhaseConsistency = 7
```

#### Hist.DeviceErrorRangeComplete *(constant)*

```csharp
public const long DeviceErrorRangeComplete = 9
```

#### Hist.DeviceErrorRangeCompleteNoWrapCheck *(constant)*

```csharp
public const long DeviceErrorRangeCompleteNoWrapCheck = 19
```

#### Hist.DeviceErrorEventConsistency *(constant)*

```csharp
public const long DeviceErrorEventConsistency = 20
```

#### Hist.DeviceErrorRangeCompleteMergedPulse *(constant)*

```csharp
public const long DeviceErrorRangeCompleteMergedPulse = 22
```

#### Hist.DeviceErrorPrevRangeNoTargets *(constant)*

```csharp
public const long DeviceErrorPrevRangeNoTargets = 23
```

#### Hist.HistTargetOrderStrongestFirst *(constant)*

```csharp
public const long HistTargetOrderStrongestFirst = 1
```

#### Hist.HistTargetOrderClosestFirst *(constant)*

```csharp
public const long HistTargetOrderClosestFirst = 2
```

#### Hist.RangeStatusRangeValid *(constant)*

```csharp
public const int RangeStatusRangeValid = 0
```

#### Hist.RangeStatusSigmaFail *(constant)*

```csharp
public const int RangeStatusSigmaFail = 1
```

#### Hist.RangeStatusOutOfBoundsFail *(constant)*

```csharp
public const int RangeStatusOutOfBoundsFail = 4
```

#### Hist.RangeStatusRangeValidNoWrapCheck *(constant)*

```csharp
public const int RangeStatusRangeValidNoWrapCheck = 6
```

#### Hist.RangeStatusWrapTargetFail *(constant)*

```csharp
public const int RangeStatusWrapTargetFail = 7
```

#### Hist.RangeStatusRangeValidMergedPulse *(constant)*

```csharp
public const int RangeStatusRangeValidMergedPulse = 11
```

#### Hist.RangeStatusTargetPresentLackOfSignal *(constant)*

```csharp
public const int RangeStatusTargetPresentLackOfSignal = 12
```

#### Hist.RangeStatusNone *(constant)*

```csharp
public const int RangeStatusNone = 255
```

#### Hist.StatusNames *(field)*

```csharp
public static readonly IReadOnlyDictionary<int, string> StatusNames = …
```

UM2931 range status names.

#### Hist.PyMod

```csharp
public static long PyMod(long a, long b)
```

The modulo of the reference (sign of the divisor).

#### Hist.Isqrt

```csharp
public static long Isqrt(long num)
```

VL53LX_isqrt(): floor of the square root of a uint32 argument.

#### Hist.CalcPllPeriodMm

```csharp
public static long CalcPllPeriodMm(long fastOscFrequency)
```

VL53LX_calc_pll_period_mm().

#### Hist.RateMaths

```csharp
public static long RateMaths(long events, long timeUs)
```

VL53LX_rate_maths(): events over a duration → count rate in 9.7 Mcps.

#### Hist.RatePerSpadMaths

```csharp
public static long RatePerSpadMaths(int fracBits, long peakCountRate, long numSpads, long maxOutputValue)
```

VL53LX_rate_per_spad_maths().

#### Hist.EventsPerSpadMaths

```csharp
public static long EventsPerSpadMaths(long events, long numSpads, long duration)
```

VL53LX_events_per_spad_maths() → kcps per SPAD.

#### Hist.RangeMaths

```csharp
public static long RangeMaths(long fastOscFrequency, long phase, long zeroDistancePhase, int fractionalBits, long gainFactor, long rangeOffsetMm)
```

VL53LX_range_maths(): phase → millimetres.

#### Hist.AverageRepeatedBins

```csharp
public static HistogramBinData AverageRepeatedBins(HistogramBinData src)
```

VL53LX_f_031: fold the six bin-sequence codes together. Repeats of a code land on the same four output bins and are averaged, so what comes out is one bin per distinct code.

#### Hist.CalcZeroDistancePhase

```csharp
public static void CalcZeroDistancePhase(HistogramBinData bins)
```

VL53LX_hist_calc_zero_distance_phase.

#### Hist.EstimateAmbientFromThresholdedBins

```csharp
public static void EstimateAmbientFromThresholdedBins(long ambientThresholdSigma, HistogramBinData bins)
```

VL53LX_hist_estimate_ambient_from_thresholded_bins(): the floor from every bin near the quietest.

#### Hist.EstimateAmbientFromAmbientBins

```csharp
public static void EstimateAmbientFromAmbientBins(HistogramBinData bins)
```

VL53LX_hist_estimate_ambient_from_ambient_bins(): the better estimate, from the VCSEL-off bins.

#### Hist.RemoveAmbientBins

```csharp
public static void RemoveAmbientBins(HistogramBinData bins)
```

VL53LX_hist_remove_ambient_bins: drop the ambient-only bins off the front, so bin 0 is phase zero.

#### Hist.ProcessData

```csharp
public static RangeResults ProcessData(HistogramBinData binsInput, HistPostProcessConfig hpp, long mergeNb = 1)
```

`VL53LX_hist_process_data()` + `VL53LX_f_025`, crosstalk off.

#### Hist.ConvertStatus

```csharp
public static int ConvertStatus(long deviceError)
```

ConvertStatusHisto() (vl53lx_api.c), the cases the histogram path produces here.

#### Hist.ToRange

```csharp
public static Vl53lxRange ToRange(RangeResults results, HistogramBinData bins)
```

The frame's results as the family-wide range: every return in `Targets`, the first one also the range itself. Effective SPADs come in 1/256 and are reported whole, as the rest of the family does.

### Vl53l0xUldDriver

```csharp
public sealed class Vl53l0xUldDriver : Vl53lxDriver
```

The VL53L0X API 1.0.4 ranging path (vl53l0x_api.c / _api_core.c / _api_calibration.c), ported from the reference Python driver: DataInit, StaticInit (reference SPADs from NVM, tuning settings), the timing-budget arithmetic, start / stop, GetRangingMeasurementData with its PAL range status (sigma estimate included), the reference calibration, PerformRefSpadManagement, offset and crosstalk calibration and the four ST ranging profiles. Register sequences, integer widths and the deliberate uint32 overflows are the C driver's. There is no ROI on this die.

#### Vl53l0xUldDriver.ModelIdVl53l0x *(constant)*

```csharp
public const int ModelIdVl53l0x = 0xEE
```

#### Vl53l0xUldDriver.Vl53l0xUldDriver *(constructor)*

```csharp
public Vl53l0xUldDriver(Vl53lxPlatform platform, string product) : base(platform, product)
```

#### Vl53l0xUldDriver.AddrWidth *(property)*

```csharp
public override int AddrWidth
```

#### Vl53l0xUldDriver.ClearSteps *(property)*

```csharp
public override IReadOnlyList<Vl53lxClearStep> ClearSteps
```

#### Vl53l0xUldDriver.MaxKhz *(property)*

```csharp
public override int MaxKhz
```

#### Vl53l0xUldDriver.Supports *(property)*

```csharp
public override IReadOnlyCollection<string> Supports
```

#### Vl53l0xUldDriver.Modes *(property)*

```csharp
public override IReadOnlyList<string> Modes
```

#### Vl53l0xUldDriver.override

```csharp
public override (int Min, int Max) BudgetMs
```

#### Vl53l0xUldDriver.RefSpadsFromNvm *(property)*

```csharp
public bool RefSpadsFromNvm
```

False when the NVM reference-SPAD record was out of range and was measured instead.

#### Vl53l0xUldDriver.ModuleId *(property)*

```csharp
public int ModuleId
```

#### Vl53l0xUldDriver.Revision *(property)*

```csharp
public int Revision
```

#### Vl53l0xUldDriver.ProductIdString *(property)*

```csharp
public string ProductIdString
```

#### Vl53l0xUldDriver.PartUidUpper *(property)*

```csharp
public long PartUidUpper
```

#### Vl53l0xUldDriver.PartUidLower *(property)*

```csharp
public long PartUidLower
```

#### Vl53l0xUldDriver.SignalRateMeasFixed400mm *(property)*

```csharp
public long SignalRateMeasFixed400mm
```

#### Vl53l0xUldDriver.Part2PartOffsetAdjustmentNvmUm *(property)*

```csharp
public long Part2PartOffsetAdjustmentNvmUm
```

#### Vl53l0xUldDriver.OscFrequencyMHz *(property)*

```csharp
public long OscFrequencyMHz
```

#### Vl53l0xUldDriver.Isqrt

```csharp
public static long Isqrt(long num)
```

VL53L0X_isqrt(): the bit-by-bit integer square root.

#### Vl53l0xUldDriver.override

```csharp
public override (int Addr, int Len) StreamBlock()
```

#### Vl53l0xUldDriver.Decode

```csharp
public override Vl53lxRange Decode(byte[] raw)
```

#### Vl53l0xUldDriver.ReadMeasurement

```csharp
public override Vl53lxRange ReadMeasurement()
```

#### Vl53l0xUldDriver.ModelId

```csharp
public override int ModelId()
```

#### Vl53l0xUldDriver.WaitBoot

```csharp
public void WaitBoot(TimeSpan? timeout = null)
```

No firmware-status register: the model id answering says the die is up.

#### Vl53l0xUldDriver.SensorInit

```csharp
public override void SensorInit()
```

DataInit + StaticInit, the reference calibration, continuous mode; the bus stays at 400 kHz.

#### Vl53l0xUldDriver.ResetDevice

```csharp
public void ResetDevice(TimeSpan? timeout = null)
```

VL53L0X_ResetDevice(): a sensor left configured by a previous session ranges nothing without it.

#### Vl53l0xUldDriver.public

```csharp
public (int Vhv, int Phase) PerformRefCalibration()
```

VL53L0X_PerformRefCalibration(): VHV, then phase → (vhv, phase). Not optional in practice.

#### Vl53l0xUldDriver.GetInfoFromDevice

```csharp
public void GetInfoFromDevice(int option)
```

VL53L0X_get_info_from_device(): the reference SPAD record (option 1), the product identification (2), the part UID and factory offset (4).

#### Vl53l0xUldDriver.ClearInterrupt

```csharp
public override void ClearInterrupt()
```

VL53L0X_ClearInterruptMask(): two writes, then confirm — up to three rounds.

#### Vl53l0xUldDriver.CheckForDataReady

```csharp
public override bool CheckForDataReady()
```

#### Vl53l0xUldDriver.SetRangeTiming

```csharp
public override void SetRangeTiming(int timingBudgetMs, int interMeasurementMs)
```

Budget and period as the family asks for them: a period of 0 means back-to-back continuous, anything else the timed continuous mode.

#### Vl53l0xUldDriver.override

```csharp
public override (int TimingBudgetMs, int InterMeasurementMs) GetRangeTiming()
```

#### Vl53l0xUldDriver.StartRanging

```csharp
public override void StartRanging()
```

VL53L0X_StartMeasurement() for the continuous modes; a single shot in between is resumed from.

#### Vl53l0xUldDriver.StopRanging

```csharp
public override void StopRanging()
```

VL53L0X_StopMeasurement().

#### Vl53l0xUldDriver.SetMode

```csharp
public override void SetMode(string name)
```

Apply one of `Modes`: the sigma and signal limits, the budget and both VCSEL periods.

#### Vl53l0xUldDriver.GetMode

```csharp
public override string GetMode()
```

#### Vl53l0xUldDriver.ReachMm

```csharp
public override int? ReachMm()
```

By the final-range VCSEL period read from the sensor; null for a period no profile sets.

#### Vl53l0xUldDriver.DriverInfo

```csharp
public override IReadOnlyDictionary<string, object?> DriverInfo()
```

#### Vl53l0xUldDriver.SetOffsetUm

```csharp
public void SetOffsetUm(long offsetUm)
```

The register is 10.2 format in mm: steps of 250 µm.

#### Vl53l0xUldDriver.GetOffsetUm

```csharp
public long GetOffsetUm()
```

#### Vl53l0xUldDriver.SetOffset

```csharp
public override void SetOffset(int offsetMm)
```

#### Vl53l0xUldDriver.GetOffset

```csharp
public override int GetOffset()
```

#### Vl53l0xUldDriver.PerformOffsetCalibration

```csharp
public long PerformOffsetCalibration(int calDistanceMm, int nbSamples = 50)
```

VL53L0X_perform_offset_calibration() → µm. TCC and the range-ignore threshold are off for the run.

#### Vl53l0xUldDriver.CalibrateOffset

```csharp
public override int CalibrateOffset(int targetDistMm, int? nbSamples = null)
```

#### Vl53l0xUldDriver.SetXtalk

```csharp
public override void SetXtalk(int xtalkKcps)
```

#### Vl53l0xUldDriver.GetXtalk

```csharp
public override int GetXtalk()
```

#### Vl53l0xUldDriver.PerformXtalkCalibration

```csharp
public long PerformXtalkCalibration(int calDistanceMm, int nbSamples = 50)
```

VL53L0X_perform_xtalk_calibration() → the rate, Mcps 16.16.

#### Vl53l0xUldDriver.CalibrateXtalk

```csharp
public override int CalibrateXtalk(int targetDistMm, int? nbSamples = null)
```

#### Vl53l0xUldDriver.public

```csharp
public (int Count, int IsAperture) GetReferenceSpads()
```

#### Vl53l0xUldDriver.PerformRefSpadManagement

```csharp
public override object PerformRefSpadManagement()
```

Re-measure the reference SPADs on a live session → (count, is aperture). The stop variable goes back first (a stopped session leaves register 0x91 at 0, and the rate then reads far too high).

### Vl53l1Die

```csharp
public abstract class Vl53l1Die : Vl53lxDriver
```

The VL53L1 die (uld/vl53l1_die.py): register map, the 17-byte result block every part of it shares, and the body of the VL53L4CD ULD that `Vl53l4UldDriver` and `Vl53l3UlpDriver` fill in; `Vl53l1UldDriver` (ST's VL53L1X ULD) overrides where that driver differs. A faithful port: register sequences and integer widths as in the C drivers.

#### Vl53l1Die.SoftReset *(constant)*

```csharp
public const int SoftReset = 0x0000
```

#### Vl53l1Die.I2cSlaveDeviceAddress *(constant)*

```csharp
public const int I2cSlaveDeviceAddress = 0x0001
```

#### Vl53l1Die.OscFrequency *(constant)*

```csharp
public const int OscFrequency = 0x0006
```

#### Vl53l1Die.VhvConfigTimeoutMacropLoopBound *(constant)*

```csharp
public const int VhvConfigTimeoutMacropLoopBound = 0x0008
```

#### Vl53l1Die.XtalkPlaneOffsetKcps *(constant)*

```csharp
public const int XtalkPlaneOffsetKcps = 0x0016
```

#### Vl53l1Die.XtalkXPlaneGradientKcps *(constant)*

```csharp
public const int XtalkXPlaneGradientKcps = 0x0018
```

#### Vl53l1Die.XtalkYPlaneGradientKcps *(constant)*

```csharp
public const int XtalkYPlaneGradientKcps = 0x001A
```

#### Vl53l1Die.RangeOffsetMm *(constant)*

```csharp
public const int RangeOffsetMm = 0x001E
```

#### Vl53l1Die.InnerOffsetMm *(constant)*

```csharp
public const int InnerOffsetMm = 0x0020
```

#### Vl53l1Die.OuterOffsetMm *(constant)*

```csharp
public const int OuterOffsetMm = 0x0022
```

#### Vl53l1Die.GpioHvMuxCtrl *(constant)*

```csharp
public const int GpioHvMuxCtrl = 0x0030
```

#### Vl53l1Die.GpioTioHvStatus *(constant)*

```csharp
public const int GpioTioHvStatus = 0x0031
```

#### Vl53l1Die.SystemInterruptConfigGpio *(constant)*

```csharp
public const int SystemInterruptConfigGpio = 0x0046
```

#### Vl53l1Die.PhasecalConfigTimeoutMacrop *(constant)*

```csharp
public const int PhasecalConfigTimeoutMacrop = 0x004B
```

#### Vl53l1Die.RangeConfigTimeoutMacropAHi *(constant)*

```csharp
public const int RangeConfigTimeoutMacropAHi = 0x005E
```

#### Vl53l1Die.RangeConfigVcselPeriodA *(constant)*

```csharp
public const int RangeConfigVcselPeriodA = 0x0060
```

#### Vl53l1Die.RangeConfigTimeoutMacropBHi *(constant)*

```csharp
public const int RangeConfigTimeoutMacropBHi = 0x0061
```

#### Vl53l1Die.RangeConfigVcselPeriodB *(constant)*

```csharp
public const int RangeConfigVcselPeriodB = 0x0063
```

#### Vl53l1Die.RangeConfigSigmaThresh *(constant)*

```csharp
public const int RangeConfigSigmaThresh = 0x0064
```

#### Vl53l1Die.MinCountRateRtnLimitMcps *(constant)*

```csharp
public const int MinCountRateRtnLimitMcps = 0x0066
```

#### Vl53l1Die.RangeConfigValidPhaseHigh *(constant)*

```csharp
public const int RangeConfigValidPhaseHigh = 0x0069
```

#### Vl53l1Die.IntermeasurementMs *(constant)*

```csharp
public const int IntermeasurementMs = 0x006C
```

#### Vl53l1Die.ThreshHigh *(constant)*

```csharp
public const int ThreshHigh = 0x0072
```

#### Vl53l1Die.ThreshLow *(constant)*

```csharp
public const int ThreshLow = 0x0074
```

#### Vl53l1Die.SdConfigWoiSd0 *(constant)*

```csharp
public const int SdConfigWoiSd0 = 0x0078
```

#### Vl53l1Die.SdConfigInitialPhaseSd0 *(constant)*

```csharp
public const int SdConfigInitialPhaseSd0 = 0x007A
```

#### Vl53l1Die.RoiConfigUserRoiCentreSpad *(constant)*

```csharp
public const int RoiConfigUserRoiCentreSpad = 0x007F
```

#### Vl53l1Die.RoiConfigUserRoiXySize *(constant)*

```csharp
public const int RoiConfigUserRoiXySize = 0x0080
```

#### Vl53l1Die.SystemInterruptClear *(constant)*

```csharp
public const int SystemInterruptClear = 0x0086
```

#### Vl53l1Die.SystemModeStart *(constant)*

```csharp
public const int SystemModeStart = 0x0087
```

#### Vl53l1Die.ResultRangeStatus *(constant)*

```csharp
public const int ResultRangeStatus = 0x0089
```

#### Vl53l1Die.ResultOscCalibrateVal *(constant)*

```csharp
public const int ResultOscCalibrateVal = 0x00DE
```

#### Vl53l1Die.FirmwareSystemStatus *(constant)*

```csharp
public const int FirmwareSystemStatus = 0x00E5
```

#### Vl53l1Die.IdentificationModelId *(constant)*

```csharp
public const int IdentificationModelId = 0x010F
```

#### Vl53l1Die.RoiConfigModeRoiCentreSpad *(constant)*

```csharp
public const int RoiConfigModeRoiCentreSpad = 0x013E
```

#### Vl53l1Die.WindowBelow *(constant)*

```csharp
public const int WindowBelow = 0, WindowAbove = 1, WindowOut = 2, WindowIn = 3
```

#### Vl53l1Die.ConfigAddr *(constant)*

```csharp
public const int ConfigAddr = 0x002D
```

The configuration blob spans 0x2D..0x87 on every part; byte 0 is always the Fast Mode Plus byte.

#### Vl53l1Die.ConfigEnd *(constant)*

```csharp
public const int ConfigEnd = 0x0087
```

#### Vl53l1Die.ConfigFmpByte *(constant)*

```csharp
public const byte ConfigFmpByte = 0x12
```

#### Vl53l1Die.ResultBlockAddr *(constant)*

```csharp
public const int ResultBlockAddr = ResultRangeStatus
```

#### Vl53l1Die.ResultBlockLen *(constant)*

```csharp
public const int ResultBlockLen = 17
```

#### Vl53l1Die.AddrWidth *(property)*

```csharp
public override int AddrWidth
```

#### Vl53l1Die.ClearSteps *(property)*

```csharp
public override IReadOnlyList<Vl53lxClearStep> ClearSteps
```

#### Vl53l1Die.MaxKhz *(property)*

```csharp
public override int MaxKhz
```

#### Vl53l1Die.PhaseWindowMm

```csharp
public static long PhaseWindowMm(long fastOscFrequency, long validPhaseHigh)
```

How far a target can be before the die fails it with status 4, mm.

#### Vl53l1Die.override

```csharp
public override (int Addr, int Len) StreamBlock()
```

#### Vl53l1Die.Decode

```csharp
public override Vl53lxRange Decode(byte[] raw)
```

#### Vl53l1Die.StatusText

```csharp
public static string StatusText(int status)
```

#### Vl53l1Die.GetResult

```csharp
public Vl53lxRange GetResult()
```

One block read instead of the C driver's six register reads; the decoding is identical.

#### Vl53l1Die.ReadMeasurement

```csharp
public override Vl53lxRange ReadMeasurement()
```

#### Vl53l1Die.ModelId

```csharp
public override int ModelId()
```

#### Vl53l1Die.BootState

```csharp
public int BootState()
```

#### Vl53l1Die.WaitBoot

```csharp
public void WaitBoot(TimeSpan? timeout = null)
```

#### Vl53l1Die.ResetDevice

```csharp
public void ResetDevice(TimeSpan? timeout = null)
```

The die's soft reset; a part that no longer acknowledges writes is reset through XSHUT.

#### Vl53l1Die.SensorInit

```csharp
public override void SensorInit()
```

Configuration at 400 kHz (the only speed an unconfigured sensor is specified for), Fast Mode Plus byte first, then the bus at `MaxKhz`, VHV, and 50 ms continuous.

#### Vl53l1Die.ClearInterrupt

```csharp
public override void ClearInterrupt()
```

#### Vl53l1Die.StartRanging

```csharp
public override void StartRanging()
```

#### Vl53l1Die.StopRanging

```csharp
public override void StopRanging()
```

#### Vl53l1Die.GetInterruptPolarity

```csharp
public int GetInterruptPolarity()
```

#### Vl53l1Die.SetInterruptPolarity

```csharp
public void SetInterruptPolarity(int polarity)
```

#### Vl53l1Die.CheckForDataReady

```csharp
public override bool CheckForDataReady()
```

#### Vl53l1Die.SetRangeTiming

```csharp
public override void SetRangeTiming(int timingBudgetMs, int interMeasurementMs)
```

#### Vl53l1Die.override

```csharp
public override (int TimingBudgetMs, int InterMeasurementMs) GetRangeTiming()
```

#### Vl53l1Die.ReachMm

```csharp
public override int? ReachMm()
```

Read off the loaded configuration: 1388 mm for the 0x38 window, 4563 mm for 0xB8.

#### Vl53l1Die.SetDetectionThresholds

```csharp
public override void SetDetectionThresholds(int distanceLowMm, int distanceHighMm, int window)
```

#### Vl53l1Die.override

```csharp
public override (int DistanceLowMm, int DistanceHighMm, int Window) GetDetectionThresholds()
```

#### Vl53l1Die.SetSignalThreshold

```csharp
public override void SetSignalThreshold(int signalKcps)
```

#### Vl53l1Die.GetSignalThreshold

```csharp
public override int GetSignalThreshold()
```

#### Vl53l1Die.SetSigmaThreshold

```csharp
public override void SetSigmaThreshold(int sigmaMm)
```

#### Vl53l1Die.GetSigmaThreshold

```csharp
public override int GetSigmaThreshold()
```

#### Vl53l1Die.SetOffset

```csharp
public override void SetOffset(int offsetMm)
```

#### Vl53l1Die.GetOffset

```csharp
public override int GetOffset()
```

#### Vl53l1Die.SetXtalk

```csharp
public override void SetXtalk(int xtalkKcps)
```

#### Vl53l1Die.GetXtalk

```csharp
public override int GetXtalk()
```

#### Vl53l1Die.SetRoi

```csharp
public override void SetRoi(int x, int y)
```

An X by Y window of SPADs, 4..16 each way.

#### Vl53l1Die.override

```csharp
public override (int X, int Y) GetRoi()
```

#### Vl53l1Die.SetRoiCenter

```csharp
public override void SetRoiCenter(int centerSpad)
```

#### Vl53l1Die.GetRoiCenter

```csharp
public override int GetRoiCenter()
```

#### Vl53l1Die.StartTemperatureUpdate

```csharp
public override void StartTemperatureUpdate()
```

Recommended after a >8 °C ambient change.

### Vl53l4UldDriver

```csharp
public sealed class Vl53l4UldDriver : Vl53l1Die
```

VL53L4CD ULD 2.2.3 (VL53L4CD_api.c + VL53L4CD_calibration.c) for VL53L4CD and VL53L4CX alike.

#### Vl53l4UldDriver.Vl53l4UldDriver *(constructor)*

```csharp
public Vl53l4UldDriver(Vl53lxPlatform platform, string product) : base(platform, product)
```

#### Vl53l4UldDriver.Supports *(property)*

```csharp
public override IReadOnlyCollection<string> Supports
```

#### Vl53l4UldDriver.CalibrateOffset

```csharp
public override int CalibrateOffset(int targetDistMm, int? nbSamples = null)
```

#### Vl53l4UldDriver.CalibrateXtalk

```csharp
public override int CalibrateXtalk(int targetDistMm, int? nbSamples = null)
```

### Vl53l3UlpDriver

```csharp
public sealed class Vl53l3UlpDriver : Vl53l1Die
```

VL53L3CX ULP 1.0.0: plain single-target ranging on the histogram part.

#### Vl53l3UlpDriver.Vl53l3UlpDriver *(constructor)*

```csharp
public Vl53l3UlpDriver(Vl53lxPlatform platform, string product) : base(platform, product)
```

#### Vl53l3UlpDriver.Supports *(property)*

```csharp
public override IReadOnlyCollection<string> Supports
```

### Vl53l1UldDriver

```csharp
public sealed class Vl53l1UldDriver : Vl53l1Die
```

VL53L1X ULD 3.5.5 (VL53L1X_api.c + VL53L1X_calibration.c) for VL53L1CX and VL53L1CB: a tabulated timing budget, short / long distance modes, an init that resets the die first. Crosstalk is in kcps, the register's own unit (ST's wrapper converts to cps).

#### Vl53l1UldDriver.Vl53l1UldDriver *(constructor)*

```csharp
public Vl53l1UldDriver(Vl53lxPlatform platform, string product) : base(platform, product)
```

#### Vl53l1UldDriver.Supports *(property)*

```csharp
public override IReadOnlyCollection<string> Supports
```

#### Vl53l1UldDriver.Modes *(property)*

```csharp
public override IReadOnlyList<string> Modes
```

#### Vl53l1UldDriver.override

```csharp
public override (int Min, int Max) BudgetMs
```

#### Vl53l1UldDriver.StartRanging

```csharp
public override void StartRanging()
```

Timed mode only: this part has no free-running mode under the ULD configuration.

#### Vl53l1UldDriver.SetMode

```csharp
public override void SetMode(string name)
```

"short" or "long"; the budget is re-applied (the VCSEL periods change what it means).

#### Vl53l1UldDriver.GetMode

```csharp
public override string GetMode()
```

#### Vl53l1UldDriver.SetRangeTiming

```csharp
public override void SetRangeTiming(int timingBudgetMs, int interMeasurementMs)
```

Only the tabulated budgets of the mode exist; a period of 0 is written as the budget itself (this part has no free-running mode).

#### Vl53l1UldDriver.BudgetChoices

```csharp
public override IReadOnlyList<int> BudgetChoices()
```

#### Vl53l1UldDriver.override

```csharp
public override (int TimingBudgetMs, int InterMeasurementMs) GetRangeTiming()
```

#### Vl53l1UldDriver.SetDetectionThresholds

```csharp
public override void SetDetectionThresholds(int distanceLowMm, int distanceHighMm, int window)
```

VL53L1X_SetDistanceThreshold(): a read-modify-write keeping the bits outside 0x6F.

#### Vl53l1UldDriver.SetDetectionThresholds

```csharp
public void SetDetectionThresholds(int distanceLowMm, int distanceHighMm, int window, bool intOnNoTarget)
```

#### Vl53l1UldDriver.CalibrateOffset

```csharp
public override int CalibrateOffset(int targetDistMm, int? nbSamples = null)
```

#### Vl53l1UldDriver.CalibrateXtalk

```csharp
public override int CalibrateXtalk(int targetDistMm, int? nbSamples = null)
```

VL53L1X_CalibrateXtalk(); returns kcps, not the C driver's cps.

### Vl53lxException

```csharp
public class Vl53lxException : DepzException
```

A ULD-level failure: the sensor did not do what the driver needed (a timeout waiting for data ready or boot, a calibration that failed, a mode the driver lacks…).

#### Vl53lxException.Vl53lxException *(constructor)*

```csharp
public Vl53lxException(string message) : base(message)
```

### Vl53lxProtocolException

```csharp
public sealed class Vl53lxProtocolException : DepzException
```

The bridge refused a register command or did not answer it. The drivers catch it where a sensor legitimately NACKs for a while (right after a soft reset).

#### Vl53lxProtocolException.Vl53lxProtocolException *(constructor)*

```csharp
public Vl53lxProtocolException(string message, Exception? inner = null) : base(message, inner ?? new Exception(message))
```

### Vl53lxPlatform

```csharp
public abstract class Vl53lxPlatform
```

ST's platform layer (RdByte / WrWord / WaitMs …) for the 1D family, plus what the bridge owns: bus speed, register-address width and the XSHUT line. Register values are big-endian, as the C drivers assume. `Vl53lxDevice` implements it over the board; a test can script it.

#### Vl53lxPlatform.ReadMulti

```csharp
public abstract byte[] ReadMulti(int addr, int size)
```

#### Vl53lxPlatform.WriteMulti

```csharp
public abstract void WriteMulti(int addr, byte[] data)
```

#### Vl53lxPlatform.SetI2cSpeed

```csharp
public abstract void SetI2cSpeed(int khz)
```

The bridge's bus timing (not part of the C platform layer).

#### Vl53lxPlatform.SetAddrWidth

```csharp
public abstract void SetAddrWidth(int width)
```

How wide a register address the bridge puts on the wire; sticky.

#### Vl53lxPlatform.XshutReset

```csharp
public abstract void XshutReset()
```

Reset through XSHUT, for a sensor that no longer acknowledges writes.

#### Vl53lxPlatform.SleepMs

```csharp
public virtual void SleepMs(int ms)
```

#### Vl53lxPlatform.ReadByte

```csharp
public int ReadByte(int addr)
```

#### Vl53lxPlatform.ReadWord

```csharp
public int ReadWord(int addr)
```

#### Vl53lxPlatform.ReadDword

```csharp
public uint ReadDword(int addr)
```

#### Vl53lxPlatform.WriteByte

```csharp
public void WriteByte(int addr, int value)
```

#### Vl53lxPlatform.WriteWord

```csharp
public void WriteWord(int addr, int value)
```

#### Vl53lxPlatform.WriteDword

```csharp
public void WriteDword(int addr, long value)
```

### Vl53lxTarget

```csharp
public sealed record Vl53lxTarget(int DistanceMm, int Status, string StatusText, double SignalKcps, double AmbientKcps, double SigmaMm, int MinRangeMm, int MaxRangeMm)
```

One return of a multi-target frame. `MinRangeMm` / `MaxRangeMm` are the edges of the target's own pulse, which only the histogram driver knows.

### Vl53lxRange

```csharp
public sealed record Vl53lxRange(int DistanceMm, int Status, string StatusText, double SignalKcps, double AmbientKcps, double SigmaMm, double Spads, IReadOnlyList<Vl53lxTarget> Targets, IReadOnlyDictionary<string, object?> Extra)
```

One range result as a driver reports it, the same shape for every product of the family. On the histogram driver `Targets` holds every return and the top level repeats the first; the light drivers leave it empty. `Extra` is driver-specific — show it, don't branch on it.

#### Vl53lxRange.Valid *(property)*

```csharp
public bool Valid
```

### Vl53lxDriver

```csharp
public abstract class Vl53lxDriver
```

The contract every driver of the 1D family implements (uld/base.py). The class properties say what the product is as far as the bridge is concerned and which optional groups it serves (`Supports`): mode, timing, offset, xtalk, calib_offset, calib_xtalk, thresholds, signal_thresh, sigma_thresh, roi, temp_update, refspad. A group a driver lacks throws `NotSupportedException`.

#### Vl53lxDriver.Product *(property)*

```csharp
public string Product
```

The product this driver was bound for (one driver serves several).

#### Vl53lxDriver.AddrWidth *(property)*

```csharp
public virtual int AddrWidth
```

Bytes of register address on the wire.

#### Vl53lxDriver.ClearSteps *(property)*

```csharp
public virtual IReadOnlyList<Vl53lxClearStep> ClearSteps
```

The interrupt-release writes handed to START_STREAM.

#### Vl53lxDriver.MaxKhz *(property)*

```csharp
public virtual int MaxKhz
```

The bus ceiling; the driver runs the bus there itself.

#### Vl53lxDriver.Supports *(property)*

```csharp
public virtual IReadOnlyCollection<string> Supports
```

#### Vl53lxDriver.Modes *(property)*

```csharp
public virtual IReadOnlyList<string> Modes
```

The named ranging modes, the first being what init leaves; empty when there are none.

#### Vl53lxDriver.virtual

```csharp
public virtual (int Min, int Max) BudgetMs
```

The (min, max) budget a UI should offer, ms; see `BudgetChoices`.

#### Vl53lxDriver.Histogram *(property)*

```csharp
public virtual bool Histogram
```

The frame is a raw histogram (the histogram driver).

#### Vl53lxDriver.ModelId

```csharp
public abstract int ModelId()
```

#### Vl53lxDriver.SensorInit

```csharp
public abstract void SensorInit()
```

Reset and configure the sensor; leaves the bus at `MaxKhz`. Safe to re-run.

#### Vl53lxDriver.StartRanging

```csharp
public abstract void StartRanging()
```

#### Vl53lxDriver.StopRanging

```csharp
public abstract void StopRanging()
```

#### Vl53lxDriver.CheckForDataReady

```csharp
public abstract bool CheckForDataReady()
```

#### Vl53lxDriver.WaitDataReady

```csharp
public virtual void WaitDataReady(TimeSpan timeout)
```

#### Vl53lxDriver.ClearInterrupt

```csharp
public abstract void ClearInterrupt()
```

#### Vl53lxDriver.abstract

```csharp
public abstract (int Addr, int Len) StreamBlock()
```

(addr, len) — the register block the bridge streams on each INT.

#### Vl53lxDriver.Decode

```csharp
public abstract Vl53lxRange Decode(byte[] raw)
```

Streamed block bytes → a range.

#### Vl53lxDriver.ReadMeasurement

```csharp
public virtual Vl53lxRange ReadMeasurement()
```

One polled result: the stream block read and decoded.

#### Vl53lxDriver.SetRangeTiming

```csharp
public abstract void SetRangeTiming(int timingBudgetMs, int interMeasurementMs)
```

Budget of one measurement and the period between two; a period of 0 means continuous.

#### Vl53lxDriver.abstract

```csharp
public abstract (int TimingBudgetMs, int InterMeasurementMs) GetRangeTiming()
```

#### Vl53lxDriver.BudgetChoices

```csharp
public virtual IReadOnlyList<int> BudgetChoices()
```

The only budgets accepted right now (ascending), or empty when any integer of `BudgetMs` will do.

#### Vl53lxDriver.SetMode

```csharp
public virtual void SetMode(string name)
```

#### Vl53lxDriver.GetMode

```csharp
public virtual string GetMode()
```

#### Vl53lxDriver.ReachMm

```csharp
public virtual int? ReachMm()
```

How far the loaded configuration can measure, mm, or null when not characterised.

#### Vl53lxDriver.DriverInfo

```csharp
public virtual IReadOnlyDictionary<string, object?> DriverInfo()
```

Numbers this driver shows that the contract has no field for.

#### Vl53lxDriver.GetOffset

```csharp
public virtual int GetOffset()
```

#### Vl53lxDriver.SetOffset

```csharp
public virtual void SetOffset(int offsetMm)
```

#### Vl53lxDriver.GetXtalk

```csharp
public virtual int GetXtalk()
```

#### Vl53lxDriver.SetXtalk

```csharp
public virtual void SetXtalk(int xtalkKcps)
```

#### Vl53lxDriver.CalibrateOffset

```csharp
public virtual int CalibrateOffset(int targetDistMm, int? nbSamples = null)
```

#### Vl53lxDriver.CalibrateXtalk

```csharp
public virtual int CalibrateXtalk(int targetDistMm, int? nbSamples = null)
```

#### Vl53lxDriver.virtual

```csharp
public virtual (int DistanceLowMm, int DistanceHighMm, int Window) GetDetectionThresholds()
```

#### Vl53lxDriver.SetDetectionThresholds

```csharp
public virtual void SetDetectionThresholds(int distanceLowMm, int distanceHighMm, int window)
```

#### Vl53lxDriver.GetSignalThreshold

```csharp
public virtual int GetSignalThreshold()
```

#### Vl53lxDriver.SetSignalThreshold

```csharp
public virtual void SetSignalThreshold(int signalKcps)
```

#### Vl53lxDriver.GetSigmaThreshold

```csharp
public virtual int GetSigmaThreshold()
```

#### Vl53lxDriver.SetSigmaThreshold

```csharp
public virtual void SetSigmaThreshold(int sigmaMm)
```

#### Vl53lxDriver.virtual

```csharp
public virtual (int X, int Y) GetRoi()
```

#### Vl53lxDriver.SetRoi

```csharp
public virtual void SetRoi(int x, int y)
```

#### Vl53lxDriver.GetRoiCenter

```csharp
public virtual int GetRoiCenter()
```

#### Vl53lxDriver.SetRoiCenter

```csharp
public virtual void SetRoiCenter(int centerSpad)
```

#### Vl53lxDriver.StartTemperatureUpdate

```csharp
public virtual void StartTemperatureUpdate()
```

#### Vl53lxDriver.PerformRefSpadManagement

```csharp
public virtual object PerformRefSpadManagement()
```

#### Vl53lxDriver.BinData

```csharp
public virtual object BinData(byte[]? raw = null)
```

The raw histogram of a frame (read from the sensor when `raw` is null).

#### Vl53lxDriver.ToMeasurement

```csharp
public virtual Vl53lxRange ToMeasurement(object bins)
```

A histogram frame → a range (the host-side ranging of the histogram driver).

### Vl53lxHistogramDriver

```csharp
public sealed class Vl53lxHistogramDriver : Vl53lxDriver
```

ST's VL53LX BareDriver on the host — the histogram driver of VL53L1CX / L1CB / L3CX / L4CD / L4CX: the die hands over 24 raw photon-count bins per frame and `Hist` turns them into up to four targets. What it costs against the light drivers: no calibrations and no detection thresholds, and the first frame of a stream reports status 6 (no wrap check done) because the check needs the frame before.

#### Vl53lxHistogramDriver.Vl53lxHistogramDriver *(constructor)*

```csharp
public Vl53lxHistogramDriver(Vl53lxPlatform platform, string product) : base(platform, product)
```

#### Vl53lxHistogramDriver.Bare *(property)*

```csharp
public BareDriver Bare
```

The BareDriver underneath: device image, factory data, preset and post-processing config.

#### Vl53lxHistogramDriver.AddrWidth *(property)*

```csharp
public override int AddrWidth
```

#### Vl53lxHistogramDriver.ClearSteps *(property)*

```csharp
public override IReadOnlyList<Vl53lxClearStep> ClearSteps
```

#### Vl53lxHistogramDriver.MaxKhz *(property)*

```csharp
public override int MaxKhz
```

#### Vl53lxHistogramDriver.Supports *(property)*

```csharp
public override IReadOnlyCollection<string> Supports
```

#### Vl53lxHistogramDriver.Modes *(property)*

```csharp
public override IReadOnlyList<string> Modes
```

short / medium / long, narrowed per die by `SensorInit` (an L4 has no short).

#### Vl53lxHistogramDriver.override

```csharp
public override (int Min, int Max) BudgetMs
```

The guard comes off the budget before it is divided and the range timeout is capped: 2..550 ms, 2..200 on an L4 die.

#### Vl53lxHistogramDriver.Histogram *(property)*

```csharp
public override bool Histogram
```

#### Vl53lxHistogramDriver.ModelId

```csharp
public override int ModelId()
```

#### Vl53lxHistogramDriver.WaitBoot

```csharp
public void WaitBoot(TimeSpan? timeout = null)
```

#### Vl53lxHistogramDriver.SensorInit

```csharp
public override void SensorInit()
```

Reset the part (whatever ran before left it ranging under another configuration), configure it for histogram ranging and raise the bus only after the FM+ pad byte has reached the device.

#### Vl53lxHistogramDriver.SetMode

```csharp
public override void SetMode(string name)
```

The device preset mode: the VCSEL period, which sets where the phase wraps (1.6 / 2.4 / 4.0 m), and the valid phase window inside it (`ReachMm`). The bin is 199 mm wide in all three.

#### Vl53lxHistogramDriver.GetMode

```csharp
public override string GetMode()
```

#### Vl53lxHistogramDriver.ReachMm

```csharp
public override int? ReachMm()
```

The valid phase window the post-processing applies: 992 / 1785 / 3373 mm for short / medium / long at the usual oscillator. Past it the target is status 4.

#### Vl53lxHistogramDriver.DriverInfo

```csharp
public override IReadOnlyDictionary<string, object?> DriverInfo()
```

How long one range actually runs, and the MM1/MM2 offset the post-processing applies.

#### Vl53lxHistogramDriver.StartRanging

```csharp
public override void StartRanging()
```

#### Vl53lxHistogramDriver.StopRanging

```csharp
public override void StopRanging()
```

#### Vl53lxHistogramDriver.override

```csharp
public override (int Addr, int Len) StreamBlock()
```

#### Vl53lxHistogramDriver.Decode

```csharp
public override Vl53lxRange Decode(byte[] raw)
```

#### Vl53lxHistogramDriver.ReadMeasurement

```csharp
public override Vl53lxRange ReadMeasurement()
```

#### Vl53lxHistogramDriver.BinData

```csharp
public override object BinData(byte[]? raw = null)
```

The raw frame as a `HistogramBinData`; read here when `raw` is null.

#### Vl53lxHistogramDriver.ToMeasurement

```csharp
public override Vl53lxRange ToMeasurement(object bins)
```

A frame → its range. Once per frame: the history is a state machine and would step twice.

#### Vl53lxHistogramDriver.CheckForDataReady

```csharp
public override bool CheckForDataReady()
```

#### Vl53lxHistogramDriver.ClearInterrupt

```csharp
public override void ClearInterrupt()
```

#### Vl53lxHistogramDriver.SetRangeTiming

```csharp
public override void SetRangeTiming(int timingBudgetMs, int interMeasurementMs)
```

The budget covers the whole measurement; the range timeout is one sixth of it after a fixed guard.

#### Vl53lxHistogramDriver.override

```csharp
public override (int TimingBudgetMs, int InterMeasurementMs) GetRangeTiming()
```

## BNO086 (IMU)

### Bno086Device

```csharp
public sealed class Bno086Device : DepzDevice
```

BNO085 / BNO086 9-axis IMU (contract 05). The board only shuttles raw SHTP frames (SEND_SHTP_PACKET / RPT_DATA); the SH-2 hub protocol runs here, ported from the reference SDK. Every send is acknowledged by the bridge at once and every inbound frame arrives as RPT_DATA(cmd 0x00), so replies are correlated at the SH-2 layer only.

imu.HardwareReset(); using var reports = imu.Reports(); imu.Enable(SensorId.RotationVector, hz: 100);

Never call a blocking device method (Enable, Tare, …) from an `OnReport` callback.

#### Bno086Device.RateLowFactor *(constant)*

```csharp
public const double RateLowFactor = 0.9
```

Granted rates outside [0.9, 2.1] × the requested one are worth a warning, never an error (contract 05 §7).

#### Bno086Device.RateHighFactor *(constant)*

```csharp
public const double RateHighFactor = 2.1
```

#### Bno086Device.Bno086Device *(constructor)*

```csharp
public Bno086Device(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

#### Bno086Device.Bno086Device *(constructor)*

```csharp
#if NET public Bno086Device(string port, TimeSpan? timeout = null) : base(port, timeout)
```

#### Bno086Device.BusyRetries *(property)*

```csharp
public int BusyRetries
```

SEND_SHTP_PACKET attempts on ERR_BUSY before giving up.

#### Bno086Device.BusyBackoff *(property)*

```csharp
public TimeSpan BusyBackoff
```

The back-off between busy retries (at least 200 ms per the bridge spec).

#### Bno086Device.Advertisement *(property)*

```csharp
public byte[] Advertisement
```

The raw SHTP channel-0 advertisement seen since open or reset.

#### Bno086Device.HardwareReset

```csharp
public void HardwareReset(TimeSpan? timeout = null)
```

Hard-reset the sensor through nRST. The bridge's OK confirms it; the SH-2 executable reset-complete is waited for best-effort (firmware ≤ 0.95 never sent it, ERRATA E9). SHTP state and cached features restart from zero.

#### Bno086Device.Wake

```csharp
public void Wake()
```

Pulse WAKE (PS0) — the board's pin, not the SH-2 "On" that ends `Sleep` (that is `PowerOn`).

#### Bno086Device.Sleep

```csharp
public void Sleep()
```

SH-2 "Sleep": every report stops except those enabled with `FeatureFlags.Wakeup | FeatureFlags.AlwaysOn`; the configuration is kept. No response.

#### Bno086Device.PowerOn

```csharp
public void PowerOn()
```

SH-2 "On": ends `Sleep`; every enabled report comes back. No response.

#### Bno086Device.ProductId

```csharp
public ProductId ProductId(TimeSpan? timeout = null)
```

The SH-2 application's Product ID — the first of `ProductIds`.

#### Bno086Device.ProductIds

```csharp
public List<ProductId> ProductIds(TimeSpan? timeout = null, TimeSpan? settle = null)
```

Every component's Product ID Response, the SH-2 application first. Collects until no response came for `settle` (default 150 ms), so late ones cannot answer the next request.

#### Bno086Device.Enable

```csharp
public FeatureResponse? Enable(int sensor, double? hz = null, uint? intervalUs = null, uint batchUs = 0, int sensitivity = 0, int flags = 0, uint cfgWord = 0, bool verify = true, TimeSpan? timeout = null)
```

Enable `sensor` through Set Feature (0xFD), at `hz` or every `intervalUs` (give exactly one). The hub rounds to its grid; with `verify` the granted rate is read back through Get Feature and a rate outside 0.9–2.1 × the request is traced as a warning (contract 05 §7). Returns that read-back (null without verify).

#### Bno086Device.Disable

```csharp
public void Disable(int sensor)
```

Disable `sensor` (Set Feature with interval 0).

#### Bno086Device.GetFeature

```csharp
public FeatureResponse GetFeature(int sensor, TimeSpan? timeout = null)
```

Get Feature round trip: the rates in effect for `sensor`.

#### Bno086Device.EnableRotationVector

```csharp
public FeatureResponse? EnableRotationVector(double hz = 100)
```

#### Bno086Device.EnableGameRotationVector

```csharp
public FeatureResponse? EnableGameRotationVector(double hz = 100)
```

#### Bno086Device.EnableAccelerometer

```csharp
public FeatureResponse? EnableAccelerometer(double hz = 100)
```

#### Bno086Device.EnableGyroscope

```csharp
public FeatureResponse? EnableGyroscope(double hz = 100)
```

#### Bno086Device.EnableMagnetometer

```csharp
public FeatureResponse? EnableMagnetometer(double hz = 50)
```

#### Bno086Device.EnableLinearAcceleration

```csharp
public FeatureResponse? EnableLinearAcceleration(double hz = 100)
```

#### Bno086Device.EnableGravity

```csharp
public FeatureResponse? EnableGravity(double hz = 100)
```

#### Bno086Device.EnableGyroIntegratedRv

```csharp
public FeatureResponse? EnableGyroIntegratedRv(double hz = 400)
```

#### Bno086Device.OnReport

```csharp
public IDisposable OnReport(Action<BnoReport> callback, IEnumerable<int>? sensors = null)
```

Call `callback` for every report (of `sensors` only, when given) on the reader thread. Dispose the result to stop.

#### Bno086Device.Reports

```csharp
public Subscription<BnoReport> Reports(IEnumerable<int>? sensors = null, int capacity = 1024)
```

A pull queue of reports (of `sensors` only, when given), bounded, drop-oldest; subscribed when this returns.

#### Bno086Device.StreamDroppedCounts *(property)*

```csharp
public IReadOnlyList<long> StreamDroppedCounts
```

#### Bno086Device.TareNow

```csharp
public void TareNow(TareAxis axes = TareAxis.All, TareBasis basis = TareBasis.RotationVector)
```

Tare `axes` against `basis` (no response).

#### Bno086Device.PersistTare

```csharp
public void PersistTare()
```

Persist the current tare into FRS (no response).

#### Bno086Device.SetReorientation

```csharp
public void SetReorientation(double x, double y, double z, double w)
```

The runtime reorientation quaternion (Q14 on the wire; all zeros clears; no response). It is applied on the hub's side, in the hub's own axes: x and y swapped and z negated relative to the reports.

#### Bno086Device.SetCalibration

```csharp
public void SetCalibration(bool accel = true, bool gyro = true, bool mag = true, bool planar = false, TimeSpan? timeout = null)
```

Configure the motion-engine calibration; `Sh2Exception` on a non-zero status.

#### Bno086Device.GetCalibration

```csharp
public CalibrationConfig GetCalibration(TimeSpan? timeout = null)
```

Which motion-engine calibrations run.

#### Bno086Device.SaveDcd

```csharp
public void SaveDcd(TimeSpan? timeout = null)
```

Save the dynamic calibration data to flash (DCD Save Now).

#### Bno086Device.ConfigurePeriodicDcd

```csharp
public void ConfigurePeriodicDcd(bool enable)
```

The hub's periodic DCD autosave (no response).

#### Bno086Device.SetMotionIntent

```csharp
public void SetMotionIntent(MotionIntent intent)
```

Interactive Calibration (0x0E, BNO086 only): how the device is about to move, so the hub can refine the gyro zero-rate offset while it is still. No response; a BNO085 ignores it. The hub's answer, if it wants the device to stop, is a `MotionRequest` report.

#### Bno086Device.EnableMotionRequest

```csharp
public FeatureResponse? EnableMotionRequest(double hz = 1)
```

Enable MotionRequest (0x2B) reports; the hub sends one when its request changes, and reads the rate back late, so the granted rate is not checked.

#### Bno086Device.Flush

```csharp
public void Flush(int sensor, TimeSpan? timeout = null)
```

Force Sensor Flush (0xF0): the hub sends `sensor`'s batched reports now; returns once its Flush Completed (0xEF) arrives.

#### Bno086Device.FrsRead

```csharp
public uint[] FrsRead(int recordId, TimeSpan? timeout = null)
```

Read a whole FRS record: its 32-bit words.

#### Bno086Device.FrsWrite

```csharp
public void FrsWrite(int recordId, IReadOnlyList<uint> words, TimeSpan? timeout = null)
```

Write a whole FRS record; `Sh2Exception` on failure.

#### Bno086Device.GetMetadata

```csharp
public SensorMetadata GetMetadata(int sensor, TimeSpan? timeout = null)
```

Read and parse `sensor`'s metadata FRS record.

#### Bno086Device.GetOscillatorType

```csharp
public OscillatorType GetOscillatorType(TimeSpan? timeout = null)
```

Get Oscillator Type (0x0A): R0 is the type (an unknown value comes back as its number).

#### Bno086Device.ClearDcdAndReset

```csharp
public void ClearDcdAndReset(TimeSpan? timeout = null)
```

Clear the in-RAM dynamic calibration and reset (0x0B). The hub resets instead of answering, so this waits for the executable reset-complete.

#### Bno086Device.GetErrors

```csharp
public List<ErrorRecord> GetErrors(int severity = 0, TimeSpan? timeout = null)
```

The error queue (0x01), `severity` or greater; records stream until one with source 255 (no more).

### SensorId

```csharp
public enum SensorId
{
    Accelerometer = 0x01,
    Gyroscope = 0x02,
    Magnetometer = 0x03,
    LinearAcceleration = 0x04,
    RotationVector = 0x05,
    Gravity = 0x06,
    UncalibratedGyroscope = 0x07,
    GameRotationVector = 0x08,
    GeomagneticRotationVector = 0x09,
    Pressure = 0x0A,
    AmbientLight = 0x0B,
    Humidity = 0x0C,
    Proximity = 0x0D,
    Temperature = 0x0E,
    UncalibratedMagnetometer = 0x0F,
    TapDetector = 0x10,
    StepCounter = 0x11,
    SignificantMotion = 0x12,
    StabilityClassifier = 0x13,
    RawAccelerometer = 0x14,
    RawGyroscope = 0x15,
    RawMagnetometer = 0x16,
    StepDetector = 0x18,
    ShakeDetector = 0x19,
    FlipDetector = 0x1A,
    PickupDetector = 0x1B,
    StabilityDetector = 0x1C,
    PersonalActivityClassifier = 0x1E,
    SleepDetector = 0x1F,
    TiltDetector = 0x20,
    PocketDetector = 0x21,
    CircleDetector = 0x22,
    HeartRateMonitor = 0x23,
    ArvrStabilizedRv = 0x28,
    ArvrStabilizedGameRv = 0x29,
    GyroIntegratedRv = 0x2A,
    MotionRequest = 0x2B,
}
```

SH-2 input report IDs (contracts/05_SENSOR_BNO086.md §5).

### Bno086Q

```csharp
public static class Bno086Q
```

The Q points of the report values (value = raw / 2^Q), SH-2 reference driver.

#### Bno086Q.Points *(field)*

```csharp
public static readonly IReadOnlyDictionary<int, int> Points = …
```

Q of the primary fields, by sensor id.

#### Bno086Q.RvAccuracyQ *(constant)*

```csharp
public const int RvAccuracyQ = 12
```

The rotation-vector accuracy estimate, radians.

#### Bno086Q.GyroRvAngvelQ *(constant)*

```csharp
public const int GyroRvAngvelQ = 10
```

The gyro-integrated rotation vector's angular velocity, rad/s.

#### Bno086Q.Scale

```csharp
public static double Scale(long raw, int q)
```

raw / 2^`q` (exact in a double).

### BnoReport

```csharp
public abstract record BnoReport(int SensorIdValue, long TimestampUs)
```

Base for a decoded report. Raw wire integers are authoritative.

#### BnoReport.Kind *(property)*

```csharp
public abstract string Kind
```

Vector "type" tag (matches the concrete record name).

#### BnoReport.Fields

```csharp
public abstract IReadOnlyDictionary<string, object?> Fields()
```

Field name → value (long, null, int[] or string), matching the golden vector fields.

### InputReport

```csharp
public abstract record InputReport(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs) : BnoReport(SensorIdValue, TimestampUs)
```

Channel-3/4 report with the common SH-2 header fields.

### Acceleration

```csharp
public sealed record Acceleration(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, int XRaw, int YRaw, int ZRaw) : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
```

#### Acceleration.Kind *(property)*

```csharp
public override string Kind
```

#### Acceleration.X *(property)*

```csharp
public double X
```

m/s², Q8 (also linear acceleration and gravity). Scaled values (raw / 2^8).

#### Acceleration.Y *(property)*

```csharp
public double Y
```

#### Acceleration.Z *(property)*

```csharp
public double Z
```

#### Acceleration.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### Gyroscope

```csharp
public sealed record Gyroscope(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, int XRaw, int YRaw, int ZRaw) : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
```

#### Gyroscope.Kind *(property)*

```csharp
public override string Kind
```

#### Gyroscope.X *(property)*

```csharp
public double X
```

rad/s, Q9. Scaled values (raw / 2^9).

#### Gyroscope.Y *(property)*

```csharp
public double Y
```

#### Gyroscope.Z *(property)*

```csharp
public double Z
```

#### Gyroscope.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### Magnetometer

```csharp
public sealed record Magnetometer(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, int XRaw, int YRaw, int ZRaw) : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
```

#### Magnetometer.Kind *(property)*

```csharp
public override string Kind
```

#### Magnetometer.X *(property)*

```csharp
public double X
```

µT, Q4. Scaled values (raw / 2^4).

#### Magnetometer.Y *(property)*

```csharp
public double Y
```

#### Magnetometer.Z *(property)*

```csharp
public double Z
```

#### Magnetometer.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### UncalibratedGyroscope

```csharp
public sealed record UncalibratedGyroscope(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, int XRaw, int YRaw, int ZRaw, int BiasXRaw, int BiasYRaw, int BiasZRaw) : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
```

#### UncalibratedGyroscope.Kind *(property)*

```csharp
public override string Kind
```

#### UncalibratedGyroscope.X *(property)*

```csharp
public double X
```

Scaled values (raw / 2^9).

#### UncalibratedGyroscope.Y *(property)*

```csharp
public double Y
```

#### UncalibratedGyroscope.Z *(property)*

```csharp
public double Z
```

#### UncalibratedGyroscope.Bias *(property)*

```csharp
public double[] Bias
```

The bias estimate, rad/s.

#### UncalibratedGyroscope.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### UncalibratedMagnetometer

```csharp
public sealed record UncalibratedMagnetometer(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, int XRaw, int YRaw, int ZRaw, int BiasXRaw, int BiasYRaw, int BiasZRaw) : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
```

#### UncalibratedMagnetometer.Kind *(property)*

```csharp
public override string Kind
```

#### UncalibratedMagnetometer.X *(property)*

```csharp
public double X
```

Scaled values (raw / 2^4).

#### UncalibratedMagnetometer.Y *(property)*

```csharp
public double Y
```

#### UncalibratedMagnetometer.Z *(property)*

```csharp
public double Z
```

#### UncalibratedMagnetometer.Bias *(property)*

```csharp
public double[] Bias
```

The hard-iron bias, µT.

#### UncalibratedMagnetometer.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### RotationVector

```csharp
public sealed record RotationVector(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, int IRaw, int JRaw, int KRaw, int RealRaw, int? AccuracyRaw) : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
```

#### RotationVector.Kind *(property)*

```csharp
public override string Kind
```

#### RotationVector.I *(property)*

```csharp
public double I
```

The unit quaternion, Q14.

#### RotationVector.J *(property)*

```csharp
public double J
```

#### RotationVector.K *(property)*

```csharp
public double K
```

#### RotationVector.Real *(property)*

```csharp
public double Real
```

#### RotationVector.AccuracyRad *(property)*

```csharp
public double? AccuracyRad
```

Estimated heading accuracy, radians (null for the game variants).

#### RotationVector.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### ScalarReport

```csharp
public sealed record ScalarReport(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, long ValueRaw) : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
```

#### ScalarReport.Kind *(property)*

```csharp
public override string Kind
```

#### ScalarReport.Value *(property)*

```csharp
public double Value
```

The value, Q from `Points` by sensor (hPa, lux, %, cm, °C).

#### ScalarReport.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### TapDetector

```csharp
public sealed record TapDetector(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, int Flags) : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
```

#### TapDetector.Kind *(property)*

```csharp
public override string Kind
```

#### TapDetector.DoubleTap *(property)*

```csharp
public bool DoubleTap
```

Bit 6: a double tap.

#### TapDetector.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### StepCounter

```csharp
public sealed record StepCounter(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, long LatencyUs, long Steps) : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
```

#### StepCounter.Kind *(property)*

```csharp
public override string Kind
```

#### StepCounter.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### StepDetector

```csharp
public sealed record StepDetector(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, long LatencyUs) : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
```

#### StepDetector.Kind *(property)*

```csharp
public override string Kind
```

#### StepDetector.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### SignificantMotion

```csharp
public sealed record SignificantMotion(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, int Motion) : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
```

#### SignificantMotion.Kind *(property)*

```csharp
public override string Kind
```

#### SignificantMotion.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### StabilityClassifier

```csharp
public sealed record StabilityClassifier(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, int Classification) : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
```

#### StabilityClassifier.Kind *(property)*

```csharp
public override string Kind
```

#### StabilityClassifier.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### MotionRequest

```csharp
public sealed record MotionRequest(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, int Intent, int Request) : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
```

0x2B motion request (BNO086 interactive calibration): the motion intent the hub was last told, and what it asks of the device (SH-2 §6.5.45.2; 1, 3, 4 = stay still). Sent when the request changes, not periodically.

#### MotionRequest.Kind *(property)*

```csharp
public override string Kind
```

#### MotionRequest.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### ShakeDetector

```csharp
public sealed record ShakeDetector(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, int Flags) : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
```

#### ShakeDetector.Kind *(property)*

```csharp
public override string Kind
```

#### ShakeDetector.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### GenericEvent

```csharp
public sealed record GenericEvent(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, long ValueRaw) : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
```

#### GenericEvent.Kind *(property)*

```csharp
public override string Kind
```

#### GenericEvent.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### PersonalActivityClassifier

```csharp
public sealed record PersonalActivityClassifier(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, int PageNumber, int EndOfSequence, int MostLikelyState, int[] Confidences) : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
```

#### PersonalActivityClassifier.Kind *(property)*

```csharp
public override string Kind
```

#### PersonalActivityClassifier.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### RawSensor

```csharp
public sealed record RawSensor(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, int XRaw, int YRaw, int ZRaw, long SensorTimestampUs, int TemperatureRaw) : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
```

#### RawSensor.Kind *(property)*

```csharp
public override string Kind
```

#### RawSensor.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### GyroIntegratedRV

```csharp
public sealed record GyroIntegratedRV(int SensorIdValue, long TimestampUs, int IRaw, int JRaw, int KRaw, int RealRaw, int VxRaw, int VyRaw, int VzRaw) : BnoReport(SensorIdValue, TimestampUs)
```

#### GyroIntegratedRV.Kind *(property)*

```csharp
public override string Kind
```

#### GyroIntegratedRV.I *(property)*

```csharp
public double I
```

The unit quaternion, Q14.

#### GyroIntegratedRV.J *(property)*

```csharp
public double J
```

#### GyroIntegratedRV.K *(property)*

```csharp
public double K
```

#### GyroIntegratedRV.Real *(property)*

```csharp
public double Real
```

#### GyroIntegratedRV.AngularVelocity *(property)*

```csharp
public double[] AngularVelocity
```

Angular velocity, rad/s (Q10).

#### GyroIntegratedRV.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### FlushCompleted

```csharp
public sealed record FlushCompleted(int SensorIdValue, long TimestampUs, int FlushedSensor) : BnoReport(SensorIdValue, TimestampUs)
```

0xEF Flush Completed: every batched report of `FlushedSensor` as of the matching Force Sensor Flush has been sent. `SensorIdValue` is 0xEF; no SH-2 header, so the timestamp is the cargo's current base.

#### FlushCompleted.Kind *(property)*

```csharp
public override string Kind
```

#### FlushCompleted.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### UnknownReport

```csharp
public sealed record UnknownReport(int SensorIdValue, long TimestampUs, byte[] Data) : BnoReport(SensorIdValue, TimestampUs)
```

#### UnknownReport.Kind *(property)*

```csharp
public override string Kind
```

#### UnknownReport.Fields

```csharp
public override IReadOnlyDictionary<string, object?> Fields()
```

### Sh2Reports

```csharp
public static class Sh2Reports
```

SH-2 input-report parsers (contracts/05_SENSOR_BNO086.md §5). Byte-exact with the reference Python `reports.py`. Raw wire integers preserved; the report records scale them (`Bno086Q`).

#### Sh2Reports.FlushCompletedId *(constant)*

```csharp
public const int FlushCompletedId = 0xEF
```

Flush Completed: + u8 sensor ID, no SH-2 header; the answer to Force Sensor Flush (0xF0).

#### Sh2Reports.GyroIntegratedRvId *(constant)*

```csharp
public const int GyroIntegratedRvId = 0x2A
```

#### Sh2Reports.ParseInputCargo

```csharp
public static List<BnoReport> ParseInputCargo(byte[] payload, long captureTimestampUs)
```

Parse a channel-3/4 cargo into typed reports. `captureTimestampUs` is the bridge RPT_DATA capture time (MCU uptime, never negative); 0xFB base and 0xFA rebase adjust it.

#### Sh2Reports.ParseGyroRvCargo

```csharp
public static GyroIntegratedRV? ParseGyroRvCargo(byte[] payload, long captureTimestampUs)
```

Parse a channel-5 cargo (gyro-integrated RV, dense). Two shapes: bare 7×i16, or prefixed with 0xFB + i32 base delta + u16 delay. Returns null when the cargo is too short.

### FeatureFlags

```csharp
public static class FeatureFlags
```

Set Feature `flags` bits (SH-2 §6.5.4) for `SetFeature`. `Wakeup | AlwaysOn` keeps a sensor reporting through `Sleep`: on a DEPZ BNO086 (SH-2 3.12.6, 2026-10-16) such a report kept its full rate during sleep, delivered on SHTP channel 4 (wake input), while every other report stopped.

#### FeatureFlags.Wakeup *(constant)*

```csharp
public const byte Wakeup = 0x04
```

Bit 2: wake-up enabled — the sensor reports on channel 4.

#### FeatureFlags.AlwaysOn *(constant)*

```csharp
public const byte AlwaysOn = 0x08
```

Bit 3: always-on — the sensor keeps running while the hub sleeps.

### Sh2Control

```csharp
public static class Sh2Control
```

SH-2 control-report encoders (contracts/05_SENSOR_BNO086.md §6). Each method returns the header-less cargo payload for the control channel (channel 2); wrap it with `NextFrame` to frame it. Byte-exact with the golden vectors.

#### Sh2Control.SetFeatureCommand *(constant)*

```csharp
public const byte SetFeatureCommand = 0xFD
```

#### Sh2Control.GetFeatureRequest *(constant)*

```csharp
public const byte GetFeatureRequest = 0xFE
```

#### Sh2Control.ProductIdRequest *(constant)*

```csharp
public const byte ProductIdRequest = 0xF9
```

#### Sh2Control.CommandRequest *(constant)*

```csharp
public const byte CommandRequest = 0xF2
```

#### Sh2Control.FrsReadRequest *(constant)*

```csharp
public const byte FrsReadRequest = 0xF4
```

#### Sh2Control.FrsWriteRequest *(constant)*

```csharp
public const byte FrsWriteRequest = 0xF7
```

#### Sh2Control.FrsWriteData *(constant)*

```csharp
public const byte FrsWriteData = 0xF6
```

#### Sh2Control.SetFeature

```csharp
public static byte[] SetFeature(int sensorId, uint intervalUs, uint batchUs = 0, int sensitivity = 0, int flags = 0, uint cfgWord = 0)
```

0xFD Set Feature Command: enable/configure a sensor. 17-byte payload — id, flags, u16 change-sensitivity, u32 report interval µs, u32 batch interval µs, u32 sensor-specific config.

#### Sh2Control.GetFeature

```csharp
public static byte[] GetFeature(int sensorId)
```

0xFE Get Feature Request.

#### Sh2Control.ProductId

```csharp
public static byte[] ProductId()
```

0xF9 Product ID Request.

#### Sh2Control.Sleep

```csharp
public static byte[] Sleep()
```

SH-2 "Sleep" for the executable channel (channel 1, not control): `[0x03]`. Frame it with `NextFrame((int)ShtpChannel.Executable, Sh2Control.Sleep())` and send it through the bridge's SEND_SHTP_PACKET (0x34); the hub sends no response. Every report stops, except those enabled with `Wakeup` | `AlwaysOn`, which keep their rate; the configuration (enabled sensors, rates) is kept. `PowerOn` ends it. On a DEPZ BNO086 (SH-2 3.12.6, 2026-10-16) all reports stopped within one sample; a Wakeup|AlwaysOn report kept streaming at full rate on channel 4.

#### Sh2Control.PowerOn

```csharp
public static byte[] PowerOn()
```

SH-2 "On" for the executable channel (channel 1): `[0x02]` — ends `Sleep`. Every report that was enabled comes back at its rate, without re-enabling; the hub sends no response. Not the bridge's SENSOR_WAKE_UP (0x33), which pulses the WAKE/PS0 pin — a hardware signal, not an SH-2 command.

#### Sh2Control.Command

```csharp
public static byte[] Command(int seq, int command, ReadOnlySpan<byte> parameters)
```

0xF2 Command Request: [id, seq, command] + params, zero-padded to 12 bytes (9 param slots).

#### Sh2Control.FrsRead

```csharp
public static byte[] FrsRead(int frsType, int offsetWords = 0, int blockWords = 0)
```

0xF4 FRS Read Request: reserved, u16 read offset (words), u16 FRS type, u16 block size (words). 8-byte payload.

#### Sh2Control.FrsWrite

```csharp
public static byte[] FrsWrite(int frsType, int lengthWords)
```

0xF7 FRS Write Request: reserved, u16 length (words), u16 FRS type. 6-byte payload.

#### Sh2Control.FrsWriteDataReport

```csharp
public static byte[] FrsWriteDataReport(int offsetWords, IReadOnlyList<uint> words)
```

0xF6 FRS Write Data: reserved, u16 word offset, then two u32 data words — always 12 bytes (contract 05 §6); one word leaves the second slot zero.

### Bno086Cmd

```csharp
public enum Bno086Cmd
{
    SensorReset = 0x32,
    SensorWakeUp = 0x33,
    SendShtpPacket = 0x34,
}
```

Bridge commands of the BNO086 board (contract 05 §2): the MCU only shuttles raw SHTP frames.

### Bno086Rpt

```csharp
public enum Bno086Rpt
{
    Data = 0x91,
}
```

### Bno086Data

```csharp
public sealed record Bno086Data(int Cmd, ulong TimestampUs, byte[] Shtp)
```

One RPT_DATA report: an SHTP frame captured from the sensor bus, with the MCU uptime.

#### Bno086Data.BusyBackoff *(field)*

```csharp
public static readonly TimeSpan BusyBackoff = TimeSpan.FromMilliseconds(200)
```

Two 64-byte SHTP transmit slots on the MCU (ERRATA E2); a third send gets ERR_BUSY, to back off at least this long.

#### Bno086Data.Unpack

```csharp
public static Bno086Data Unpack(ReadOnlySpan<byte> payload)
```

### Sh2Exception

```csharp
public sealed class Sh2Exception : Exception
```

A SH-2 level failure: a non-zero command status, an FRS error.

#### Sh2Exception.Sh2Exception *(constructor)*

```csharp
public Sh2Exception(string message) : base(message)
```

### ControlReport

```csharp
public enum ControlReport
{
    CommandResponse = 0xF1,
    CommandRequest = 0xF2,
    FrsReadResponse = 0xF3,
    FrsReadRequest = 0xF4,
    FrsWriteResponse = 0xF5,
    FrsWriteData = 0xF6,
    FrsWriteRequest = 0xF7,
    ProductIdResponse = 0xF8,
    ProductIdRequest = 0xF9,
    GetFeatureResponse = 0xFC,
    SetFeatureCommand = 0xFD,
    GetFeatureRequest = 0xFE,
    ForceSensorFlush = 0xF0,
}
```

Report ids on SHTP channel 2 (control).

### Sh2Command

```csharp
public enum Sh2Command
{
    Errors = 0x01,
    Counter = 0x02,
    Tare = 0x03,
    Initialize = 0x04,
    SaveDcd = 0x06,
    MeCalibrate = 0x07,
    PeriodicDcdConfig = 0x09,
    GetOscillatorType = 0x0A,
    ClearDcdAndReset = 0x0B,
    InteractiveCalibration = 0x0E,
}
```

The `command` of a Command Request / Response (0xF2 / 0xF1).

### OscillatorType

```csharp
public enum OscillatorType
{
    Internal = 0,
    ExtCrystal = 1,
    ExtClock = 2,
}
```

### ErrorSource

```csharp
public enum ErrorSource
{
    MotionEngine = 1,
    MotionHub = 2,
    SensorHub = 3,
    Chip = 4,
    NoMoreErrors = 255,
}
```

The `source` of an error record (SH-2 §6.4.1).

### TareBasis

```csharp
public enum TareBasis
{
    RotationVector = 0,
    GameRotationVector = 1,
    GeomagneticRotationVector = 2,
    GyroIntegratedRv = 3,
    ArvrStabilizedRv = 4,
    ArvrStabilizedGameRv = 5,
}
```

The rotation vector a tare refers to (Tare Now P2).

### TareAxis

```csharp
public enum TareAxis
{
    X = 1,
    Y = 2,
    Z = 4,
    All = 7,
}
```

### MotionIntent

```csharp
public enum MotionIntent
{
    Unknown = 0,
    StationaryNoVibration = 1,
    StationaryWithVibration = 2,
    InMotion = 3,
    InMotionAccelerating = 4,
}
```

P0 of Interactive Calibration (0x0E, SH-2 §6.4.12.1): what the host knows about the device's motion. Unknown is the hub's initial state.

### FrsRecordId

```csharp
public static class FrsRecordId
```

FRS record ids (SH-2 figure 28).

#### FrsRecordId.StaticCalibrationAgm *(constant)*

```csharp
public const int StaticCalibrationAgm = 0x7979
```

#### FrsRecordId.NominalCalibration *(constant)*

```csharp
public const int NominalCalibration = 0x4D4D
```

#### FrsRecordId.DynamicCalibration *(constant)*

```csharp
public const int DynamicCalibration = 0x1F1F
```

#### FrsRecordId.MePowerMgmt *(constant)*

```csharp
public const int MePowerMgmt = 0xD3E2
```

#### FrsRecordId.SystemOrientation *(constant)*

```csharp
public const int SystemOrientation = 0x2D3E
```

Mounting quaternion, 4 × Q30 words.

#### FrsRecordId.AccelOrientation *(constant)*

```csharp
public const int AccelOrientation = 0x2D41
```

#### FrsRecordId.GyroscopeOrientation *(constant)*

```csharp
public const int GyroscopeOrientation = 0x2D46
```

#### FrsRecordId.MagnetometerOrientation *(constant)*

```csharp
public const int MagnetometerOrientation = 0x2D4C
```

#### FrsRecordId.ArvrStabilizationRv *(constant)*

```csharp
public const int ArvrStabilizationRv = 0x3E2D
```

#### FrsRecordId.ArvrStabilizationGrv *(constant)*

```csharp
public const int ArvrStabilizationGrv = 0x3E2E
```

#### FrsRecordId.SerialNumber *(constant)*

```csharp
public const int SerialNumber = 0x4B4B
```

One 32-bit word; empty unless programmed.

#### FrsRecordId.UserRecord *(constant)*

```csharp
public const int UserRecord = 0x74B4
```

Free for the user, up to 64 words.

#### FrsRecordId.SigMotionDetectConfig *(constant)*

```csharp
public const int SigMotionDetectConfig = 0xC274
```

#### FrsRecordId.ShakeDetectConfig *(constant)*

```csharp
public const int ShakeDetectConfig = 0x7D7D
```

#### FrsRecordId.StabilityDetectorConfig *(constant)*

```csharp
public const int StabilityDetectorConfig = 0xED85
```

#### FrsRecordId.ActivityTrackerConfig *(constant)*

```csharp
public const int ActivityTrackerConfig = 0xED88
```

#### FrsRecordId.MetadataRecords *(field)*

```csharp
public static readonly IReadOnlyDictionary<int, int> MetadataRecords = …
```

Sensor id → its metadata FRS record.

### FrsStatus

```csharp
public enum FrsStatus
{
    NoError = 0,
    UnrecognizedFrsType = 1,
    Busy = 2,
    ReadCompleted = 3,
    OffsetOutOfRange = 4,
    RecordEmpty = 5,
    BlockCompleted = 6,
    BlockAndReadCompleted = 7,
    DeviceError = 8,
}
```

### FrsWriteStatus

```csharp
public enum FrsWriteStatus
{
    WordsReceived = 0,
    UnrecognizedFrsType = 1,
    Busy = 2,
    WriteCompleted = 3,
    WriteModeReady = 4,
    WriteFailed = 5,
    NotInWriteMode = 6,
    InvalidLength = 7,
    RecordValid = 8,
    RecordInvalid = 9,
}
```

### FeatureResponse

```csharp
public sealed record FeatureResponse(int SensorId, int Flags, int Sensitivity, uint IntervalUs, uint BatchUs, uint CfgWord)
```

Get Feature Response (0xFC), 17 bytes: the rates in effect.

#### FeatureResponse.Unpack

```csharp
public static FeatureResponse Unpack(ReadOnlySpan<byte> p)
```

### ProductId

```csharp
public sealed record ProductId(int ResetCause, int SwVersionMajor, int SwVersionMinor, uint SwPartNumber, uint SwBuildNumber, int SwVersionPatch)
```

Product ID Response (0xF8), 16 bytes, one per software component (the SH-2 application first); the reset cause (SH-2 §6.4.5.2) is set on the first only.

#### ProductId.Version *(property)*

```csharp
public string Version
```

#### ProductId.Unpack

```csharp
public static ProductId Unpack(ReadOnlySpan<byte> p)
```

### CommandResponse

```csharp
public sealed record CommandResponse(int Seq, int Command, int CommandSeq, int ResponseSeq, byte[] R)
```

Command Response (0xF1), 16 bytes. `CommandSeq` echoes the request (correlate on it plus `Command`); `ResponseSeq` counts responses to one request; R0 is the status of most commands (0 = success).

#### CommandResponse.Status *(property)*

```csharp
public int Status
```

#### CommandResponse.Unpack

```csharp
public static CommandResponse Unpack(ReadOnlySpan<byte> p)
```

### FrsReadResponse

```csharp
public sealed record FrsReadResponse(int Status, int DataLength, int OffsetWords, uint Data0, uint Data1, int FrsType)
```

FRS Read Response (0xF3), 16 bytes, up to two data words.

#### FrsReadResponse.Unpack

```csharp
public static FrsReadResponse Unpack(ReadOnlySpan<byte> p)
```

### FrsWriteResponse

```csharp
public sealed record FrsWriteResponse(int Status, int OffsetWords)
```

FRS Write Response (0xF5), 4 bytes.

#### FrsWriteResponse.Unpack

```csharp
public static FrsWriteResponse Unpack(ReadOnlySpan<byte> p)
```

### ErrorRecord

```csharp
public sealed record ErrorRecord(int Severity, int Seq, int Source, int Error, int Module, int Code)
```

One error-queue entry (command 0x01 response, R0..R5).

#### ErrorRecord.From

```csharp
public static ErrorRecord From(CommandResponse r)
```

### Counts

```csharp
public sealed record Counts(int SensorId, uint Offered, uint Accepted, uint On, uint Attempted)
```

Per-sensor event counts (command 0x02, two responses).

### CalibrationConfig

```csharp
public sealed record CalibrationConfig(bool Accel, bool Gyro, bool Mag, bool Planar)
```

ME calibration enables as the sensor reports them.

### SensorMetadata

```csharp
public sealed record SensorMetadata(int MeVersion, int MhVersion, int ShVersion, uint RangeRaw, uint ResolutionRaw, int Revision, int PowerMaQ10, uint MinPeriodUs, uint MaxPeriodUs, int FifoMax, int FifoReserved, int BatchBufferBytes, int QPoint1, int QPoint2, int QPoint3, uint[] RawWords)
```

A sensor's metadata FRS record (revision-gated fields are 0 on older records); `RawWords` is authoritative.

#### SensorMetadata.FromWords

```csharp
public static SensorMetadata FromWords(IReadOnlyList<uint> words)
```

### Sh2Hub

```csharp
public static class Sh2Hub
```

SH-2 hub protocol: the request parameters and the control-cargo walk the device layer needs on top of `Sh2Control` (contract 05 §6). Ported from the reference sh2.py.

#### Sh2Hub.CountsGet *(constant)*

```csharp
public const int CountsGet = 0
```

#### Sh2Hub.CountsClear *(constant)*

```csharp
public const int CountsClear = 1
```

#### Sh2Hub.MeCalGet *(constant)*

```csharp
public const int MeCalGet = 0x01
```

ME Calibration subcommand: report the current configuration.

#### Sh2Hub.SplitControlCargo

```csharp
public static List<byte[]> SplitControlCargo(byte[] payload)
```

Split a control cargo into its reports; an unknown id or a short tail ends the walk (the rest is one piece: its length is unknowable).

#### Sh2Hub.CountsGetParams

```csharp
public static byte[] CountsGetParams(int sensorId)
```

#### Sh2Hub.CountsClearParams

```csharp
public static byte[] CountsClearParams(int sensorId)
```

#### Sh2Hub.ErrorsParams

```csharp
public static byte[] ErrorsParams(int severity = 0)
```

Errors of `severity` or greater (0 = all).

#### Sh2Hub.TareNowParams

```csharp
public static byte[] TareNowParams(TareAxis axes = TareAxis.All, TareBasis basis = TareBasis.RotationVector)
```

#### Sh2Hub.PersistTareParams

```csharp
public static byte[] PersistTareParams()
```

#### Sh2Hub.SetReorientationParams

```csharp
public static byte[] SetReorientationParams(double x, double y, double z, double w)
```

Tare subcommand 2: the reorientation quaternion as four Q14 int16 (all zero clears).

#### Sh2Hub.MeCalibrationParams

```csharp
public static byte[] MeCalibrationParams(bool accel, bool gyro, bool mag, bool planar = false, int subcommand = 0)
```

ME Calibration (0x07): subcommand 0 configures, 1 (`MeCalGet`) reads back.

#### Sh2Hub.InteractiveCalibrationParams

```csharp
public static byte[] InteractiveCalibrationParams(int intent)
```

Interactive Calibration (0x0E, BNO086 only): P0 = motion intent; no response.

#### Sh2Hub.ForceFlush

```csharp
public static byte[] ForceFlush(int sensorId)
```

Force Sensor Flush (0xF0, SH-2 §6.5.6); the hub answers with Flush Completed (0xEF).

#### Sh2Hub.PeriodicDcdParams

```csharp
public static byte[] PeriodicDcdParams(bool enable)
```

Periodic DCD save (0x09): P0 0 = enable, 1 = disable; no response.

### FrsReadSession

```csharp
public sealed class FrsReadSession
```

A multi-packet FRS read: send `Request`, `Feed` every 0xF3 of the record until it returns true; `Words` holds the record. Error statuses throw.

#### FrsReadSession.FrsReadSession *(constructor)*

```csharp
public FrsReadSession(int frsType)
```

#### FrsReadSession.FrsType *(property)*

```csharp
public int FrsType
```

#### FrsReadSession.Words *(property)*

```csharp
public List<uint> Words
```

#### FrsReadSession.Done *(property)*

```csharp
public bool Done
```

#### FrsReadSession.Request

```csharp
public byte[] Request()
```

#### FrsReadSession.Feed

```csharp
public bool Feed(FrsReadResponse r)
```

### FrsWriteSession

```csharp
public sealed class FrsWriteSession
```

A multi-packet FRS write: send `Request`; for every 0xF5 `Feed` returns the next Write Data payload, or null; `Done` on WRITE_COMPLETED. Error statuses throw.

#### FrsWriteSession.FrsWriteSession *(constructor)*

```csharp
public FrsWriteSession(int frsType, IReadOnlyList<uint> words)
```

#### FrsWriteSession.FrsType *(property)*

```csharp
public int FrsType
```

#### FrsWriteSession.Words *(property)*

```csharp
public uint[] Words
```

#### FrsWriteSession.Done *(property)*

```csharp
public bool Done
```

#### FrsWriteSession.Request

```csharp
public byte[] Request()
```

#### FrsWriteSession.Feed

```csharp
public byte[]? Feed(FrsWriteResponse r)
```

### ShtpChannel

```csharp
public enum ShtpChannel
{
    Command = 0,
    Executable = 1,
    Control = 2,
    InputNormal = 3,
    InputWake = 4,
    GyroRv = 5,
}
```

SHTP channels (contracts/05_SENSOR_BNO086.md §3).

### ExecutableCommand

```csharp
public enum ExecutableCommand : byte
{
    Reset = 1,
    On = 2,
    Sleep = 3,
}
```

One-byte host writes on the executable channel (SHTP channel 1, contracts/05_SENSOR_BNO086.md §6a). The hub answers none of them. `Reset` restarts the hub (answered later by the 0x01 reset-complete on the same channel); `Sleep` stops every report except those enabled with `Wakeup` | `AlwaysOn`, keeping the configuration; `On` ends `Sleep` and every report resumes at its rate.

### ShtpHeader

```csharp
public sealed record ShtpHeader(int Length, int Channel, int Seq, bool Continuation = false)
```

SHTP frame header: length (bits 14:0 = cargo length incl. this 4-byte header; bit 15 = continuation), channel, per-channel/per-direction seq.

#### ShtpHeader.Size *(constant)*

```csharp
public const int Size = 4
```

#### ShtpHeader.LengthMask *(constant)*

```csharp
public const int LengthMask = 0x7FFF
```

#### ShtpHeader.ContinuationBit *(constant)*

```csharp
public const int ContinuationBit = 0x8000
```

#### ShtpHeader.Pack

```csharp
public byte[] Pack()
```

#### ShtpHeader.Unpack

```csharp
public static ShtpHeader Unpack(ReadOnlySpan<byte> data)
```

### ShtpCargo

```csharp
public sealed record ShtpCargo(int Channel, int Seq, byte[] Payload, long CaptureUs = 0)
```

One reassembled cargo: `Payload` excludes all SHTP headers; `Seq` and `CaptureUs` are the FIRST fragment's — `CaptureUs` is its `RPT_DATA.timestamp_us`, the capture the cargo's 0xFB base delta refers to (contract 05 §3).

### ShtpLayer

```csharp
public sealed class ShtpLayer
```

SHTP codec: per-channel TX sequence counters and RX cargo reassembly (continuation-bit fragments). Byte-exact with the reference Python `ShtpLayer`. Not thread-safe.

#### ShtpLayer.NumChannels *(constant)*

```csharp
public const int NumChannels = 6
```

#### ShtpLayer.MaxTxFrame *(constant)*

```csharp
public const int MaxTxFrame = 64
```

#### ShtpLayer.Discarded *(property)*

```csharp
public int Discarded
```

#### ShtpLayer.ShtpLayer *(constructor)*

```csharp
public ShtpLayer()
```

#### ShtpLayer.BuildFrame

```csharp
public static byte[] BuildFrame(int channel, ReadOnlySpan<byte> payload, int seq)
```

Single-fragment frame: length = header + payload.

#### ShtpLayer.FragmentCargo

```csharp
public static List<byte[]> FragmentCargo(int channel, byte[] payload, int seqStart, int maxFrame = MaxTxFrame)
```

Split a cargo into wire frames of at most `maxFrame` bytes. The first fragment advertises the TOTAL cargo length; each continuation carries the remaining length with the continuation bit set. seq increments per frame.

#### ShtpLayer.NextFrame

```csharp
public byte[] NextFrame(int channel, ReadOnlySpan<byte> payload)
```

Build a single-fragment frame, consuming the channel's TX seq.

#### ShtpLayer.TxSeq

```csharp
public int TxSeq(int channel)
```

#### ShtpLayer.Feed

```csharp
public ShtpCargo? Feed(byte[] frame, long captureUs = 0)
```

Consume one inbound frame; return the cargo when complete, else null. `captureUs` is the frame's `RPT_DATA.timestamp_us`; the cargo carries the FIRST fragment's — the base its 0xFB timestamps refer to (contract 05 §3), not the fragment that completed it. It is the MCU uptime (a u64 µs): a negative one throws `ArgumentOutOfRangeException`.

#### ShtpLayer.Reset

```csharp
public void Reset()
```

Forget all TX seq counters and partial cargos (sensor reset).

## BNO055 (IMU)

### Bno055Cmd

```csharp
public enum Bno055Cmd
{
    ReadReg = 0x32,
    WriteReg = 0x33,
    Reset = 0x34,
    StartStream = 0x35,
    StopStream = 0x36,
    GetInfo = 0x37,
}
```

BNO055 register-bridge command opcodes, protocol v0.10 (contracts/13_SENSOR_BNO055.md §2). 0x30/0x31 are the common sync pins.

### Bno055Rpt

```csharp
public enum Bno055Rpt
{
    RegData = 0x91,
    Info = 0x92,
    Stream = 0x93,
}
```

BNO055 report opcodes (contract 13 §3).

### Bno055Trigger

```csharp
public enum Bno055Trigger : byte
{
    Timer = 0,
    Int = 1,
}
```

BNO_START_STREAM trigger.

### Bno055Wire

```csharp
public static class Bno055Wire
```

Wire codecs of the BNO055 bridge (`APP_BNO055`). The firmware is a thin register bridge: the MCU owns the I2C bus (sensor at 7-bit 0x28, 400 kHz fixed), the nRESET/INT pins and one streaming loop; operating mode, units, axis remap, calibration and decoding are host logic expressed as register access (`Bno055Regs`). Mirrors `depz_sensor_sdk/protocol/bno055.py` 1:1. BNO_RESET, BNO_STOP_STREAM and BNO_GET_INFO carry an empty payload.

#### Bno055Wire.XferMax *(constant)*

```csharp
public const int XferMax = 128
```

Max bytes per READ_REG / WRITE_REG / streamed block; addr + len ≤ 0x100.

#### Bno055Wire.InfoSize *(constant)*

```csharp
public const int InfoSize = 38
```

RPT_BNO_INFO payload size.

#### Bno055Wire.ResetTimeout *(field)*

```csharp
public static readonly TimeSpan ResetTimeout = TimeSpan.FromSeconds(3)
```

BNO_RESET answers after the sensor's ~0.5 s boot handshake.

#### Bno055Wire.ExpectedChipId *(constant)*

```csharp
public const byte ExpectedChipId = 0xA0
```

Identity registers 0x00..0x03 of a healthy BNO055.

#### Bno055Wire.ExpectedAccId *(constant)*

```csharp
public const byte ExpectedAccId = 0xFB
```

#### Bno055Wire.ExpectedMagId *(constant)*

```csharp
public const byte ExpectedMagId = 0x32
```

#### Bno055Wire.ExpectedGyrId *(constant)*

```csharp
public const byte ExpectedGyrId = 0x0F
```

#### Bno055Wire.PackReadReg

```csharp
public static byte[] PackReadReg(byte addr, byte len)
```

BNO_READ_REG payload: addr u8, len u8.

#### Bno055Wire.PackWriteReg

```csharp
public static byte[] PackWriteReg(byte addr, ReadOnlySpan<byte> data)
```

BNO_WRITE_REG payload: addr u8, then data[1..`XferMax`].

#### Bno055Wire.PackStartStream

```csharp
public static byte[] PackStartStream(Bno055Trigger trigger, byte addr, byte len, ushort periodMs)
```

BNO_START_STREAM payload (5 bytes): trigger u8, addr u8, len u8, period_ms u16 LE. Replaces any running stream.

### Bno055RegData

```csharp
public sealed record Bno055RegData(byte Cmd, ulong TimestampUs, byte[] Data)
```

RPT_BNO_REG_DATA: echoed READ_REG opcode, MCU uptime at I2C-read completion, register bytes.

#### Bno055RegData.Unpack

```csharp
public static Bno055RegData Unpack(ReadOnlySpan<byte> payload)
```

Parse a RPT_BNO_REG_DATA payload: cmd u8, timestamp u64 LE, then data.

### Bno055StreamData

```csharp
public sealed record Bno055StreamData(ulong TimestampUs, byte Addr, byte Length, byte[] Data)
```

RPT_BNO_REG_STREAM — one streamed register block. `Addr` / `Length` echo the stream configuration so each report is self-describing; `TimestampUs` is the trigger time (timer expiry or INT edge), not the I2C completion.

#### Bno055StreamData.Unpack

```csharp
public static Bno055StreamData Unpack(ReadOnlySpan<byte> payload)
```

Parse a RPT_BNO_REG_STREAM payload: timestamp u64 LE, addr u8, len u8, data[len].

### Bno055Info

```csharp
public sealed record Bno055Info(byte I2cAddr, byte ChipId, byte AccId, byte MagId, byte GyrId, ushort SwRev, byte BlRev, byte Initialized, byte IntLevel, uint IntEdges, ushort ReadMinUs, ushort ReadMaxUs, ushort ReadAvgUs, uint TxDropped, uint I2cErrors, uint SlotsSkipped, ushort BusRecoveries, byte LastI2cError, byte SensorResets, ushort LoopMaxUs)
```

RPT_BNO_INFO (38 bytes) — sensor identity (registers 0x00..0x06) plus bridge diagnostics. Counters are free-running and wrap silently: watch increments. Read*Us, `SlotsSkipped` and `LoopMaxUs` reset at START_STREAM. A rising `SensorResets` means the bridge pulsed nRESET to recover the bus: the sensor is back in CONFIG and the host must restore its configuration. `SwRev` is BCD (0x0311 = 03.11). `LastI2cError`: 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR.

#### Bno055Info.Unpack

```csharp
public static Bno055Info Unpack(ReadOnlySpan<byte> p)
```

Parse a RPT_BNO_INFO payload (`<BBBBBHBBBIHHHIIIHBBH`, 38 bytes LE).

#### Bno055Info.IdsOk *(property)*

```csharp
public bool IdsOk
```

All four identity registers hold the healthy BNO055 values.

#### Bno055Info.SwRevText *(property)*

```csharp
public string SwRevText
```

Sensor firmware revision as Bosch writes it: 0x0311 → "03.11".

### Bno055OprMode

```csharp
public enum Bno055OprMode : byte
{
    Config = 0x00,
    AccOnly = 0x01,
    MagOnly = 0x02,
    GyroOnly = 0x03,
    AccMag = 0x04,
    AccGyro = 0x05,
    MagGyro = 0x06,
    Amg = 0x07,
    Imu = 0x08,
    Compass = 0x09,
    M4g = 0x0A,
    NdofFmcOff = 0x0B,
    Ndof = 0x0C,
}
```

OPR_MODE (0x3D) bits 3:0 (the register reads back 0x10 after reset — mask).

### Bno055PwrMode

```csharp
public enum Bno055PwrMode : byte
{
    Normal = 0x00,
    LowPower = 0x01,
    Suspend = 0x02,
}
```

PWR_MODE (0x3E) bits 1:0.

### Bno055TempSource

```csharp
public enum Bno055TempSource : byte
{
    Accel = 0x00,
    Gyro = 0x01,
}
```

TEMP_SOURCE (0x40) bits 1:0.

### Bno055Vec3

```csharp
public readonly record struct Bno055Vec3(short X, short Y, short Z)
```

Three raw i16 words in register order (x, y, z).

### Bno055Euler

```csharp
public readonly record struct Bno055Euler(short Heading, short Roll, short Pitch)
```

Raw Euler angles in register order: heading, roll, pitch.

### Bno055Quat

```csharp
public readonly record struct Bno055Quat(short W, short X, short Y, short Z)
```

Raw quaternion in register order: w, x, y, z (1.0 = 2^14).

### Bno055Units

```csharp
public sealed record Bno055Units(bool AccelMg = false, bool GyroRps = false, bool EulerRad = false, bool TempF = false, bool Android = false)
```

Output units (UNIT_SEL 0x3B). The SDK default (`Default`, UNIT_SEL = 0x00) is m/s², dps, degrees, °C, Windows orientation. The sensor's own power-on value is 0x80 (Android), so a fresh sensor must be told. Bits as the silicon implements them (contract 13 §4.2; the datasheet §4.3.60 bit table is off by one); unknown bits are dropped on unpack. Physical value = raw / LSB.

#### Bno055Units.AccMg *(constant)*

```csharp
public const byte AccMg = 0x01
```

#### Bno055Units.GyrRps *(constant)*

```csharp
public const byte GyrRps = 0x02
```

#### Bno055Units.EulRad *(constant)*

```csharp
public const byte EulRad = 0x04
```

#### Bno055Units.TempFahrenheit *(constant)*

```csharp
public const byte TempFahrenheit = 0x10
```

#### Bno055Units.OriAndroid *(constant)*

```csharp
public const byte OriAndroid = 0x80
```

#### Bno055Units.Default *(field)*

```csharp
public static readonly Bno055Units Default = new()
```

#### Bno055Units.Pack

```csharp
public byte Pack()
```

#### Bno055Units.Unpack

```csharp
public static Bno055Units Unpack(byte value)
```

#### Bno055Units.AccelLsb *(property)*

```csharp
public double AccelLsb
```

ACC_DATA LSB: 1 per mg, else 100 per m/s² (LIA/GRV: `FusionAccelLsb`).

#### Bno055Units.GyroLsb *(property)*

```csharp
public double GyroLsb
```

Angular-rate LSB: 900 per rad/s, else 16 per dps.

#### Bno055Units.EulerLsb *(property)*

```csharp
public double EulerLsb
```

Euler LSB: 900 per radian, else 16 per degree.

#### Bno055Units.TempLsb *(property)*

```csharp
public double TempLsb
```

Temperature LSB: 1 LSB = 2 °F (0.5 LSB/°F), else 1 LSB = 1 °C.

### Bno055CalibStatus

```csharp
public readonly record struct Bno055CalibStatus(int System, int Gyro, int Accel, int Mag)
```

CALIB_STAT (0x35) `sys<7:6> gyr<5:4> acc<3:2> mag<1:0>`: 0 = not calibrated … 3 = fully.

#### Bno055CalibStatus.Unpack

```csharp
public static Bno055CalibStatus Unpack(byte value)
```

#### Bno055CalibStatus.Pack

```csharp
public byte Pack()
```

#### Bno055CalibStatus.FullyCalibrated *(property)*

```csharp
public bool FullyCalibrated
```

### Bno055CalibrationProfile

```csharp
public sealed record Bno055CalibrationProfile(Bno055Vec3 AccelOffset, Bno055Vec3 MagOffset, Bno055Vec3 GyroOffset, short AccelRadius, short MagRadius)
```

Sensor offsets and radii, registers 0x55..0x6A (22 bytes, 11 × i16 LE: acc_offset xyz, mag_offset xyz, gyr_offset xyz, acc_radius, mag_radius). Readable and writable only in CONFIG; write all 22 bytes in one transfer (the sensor latches each group on its MSB). Offsets are in sensor LSB and do not depend on UNIT_SEL.

#### Bno055CalibrationProfile.Pack

```csharp
public byte[] Pack()
```

#### Bno055CalibrationProfile.Unpack

```csharp
public static Bno055CalibrationProfile Unpack(ReadOnlySpan<byte> data)
```

### Bno055Axis

```csharp
public enum Bno055Axis : byte
{
    X = 0,
    Y = 1,
    Z = 2,
}
```

Source axis codes of `Bno055AxisRemap`.

### Bno055AxisRemap

```csharp
public sealed record Bno055AxisRemap(Bno055Axis X = Bno055Axis.X, Bno055Axis Y = Bno055Axis.Y, Bno055Axis Z = Bno055Axis.Z, bool XNegative = false, bool YNegative = false, bool ZNegative = false)
```

Which chip axis feeds each output axis, and its sign (AXIS_MAP_CONFIG 0x41 = `z<5:4> y<3:2> x<1:0>`, AXIS_MAP_SIGN 0x42 = `x 2, y 1, z 0`, 1 = negative). `X = Bno055Axis.Y` means "output X is the chip's Y axis". The default is P1.

#### Bno055AxisRemap.public

```csharp
public (byte Config, byte Sign) Pack()
```

→ (AXIS_MAP_CONFIG, AXIS_MAP_SIGN). The sensor keeps its old mapping when given one that uses an axis twice, so this refuses it up front.

#### Bno055AxisRemap.Unpack

```csharp
public static Bno055AxisRemap Unpack(byte config, byte sign)
```

#### Bno055AxisRemap.IReadOnlyList

```csharp
public static readonly IReadOnlyList<(string Name, byte Config, byte Sign)> Placements = new[]
```

Datasheet §3.4: placement → (AXIS_MAP_CONFIG, AXIS_MAP_SIGN), P0..P7 in order.

#### Bno055AxisRemap.Placement

```csharp
public static Bno055AxisRemap Placement(string name)
```

Datasheet §3.4 mounting preset P0..P7 (case-insensitive; P1 is the default).

### Bno055AccelConfig

```csharp
public readonly record struct Bno055AccelConfig(int Range, int Bandwidth, int Power)
```

ACC_CONFIG (page 1, 0x08) as register codes `range<1:0> bandwidth<4:2> power<7:5>`: indexes into `AccRangeG`, `AccBandwidthHz`, `AccPowerNames`. Power-on value 0x0D = ±4 g, 62.5 Hz, normal. Effective in non-fusion modes only.

#### Bno055AccelConfig.Default *(field)*

```csharp
public static readonly Bno055AccelConfig Default = new(1, 3, 0)
```

#### Bno055AccelConfig.Pack

```csharp
public byte Pack()
```

#### Bno055AccelConfig.Unpack

```csharp
public static Bno055AccelConfig Unpack(byte value)
```

### Bno055GyroConfig

```csharp
public readonly record struct Bno055GyroConfig(int Range, int Bandwidth, int Power)
```

GYR_CONFIG_0/1 (page 1, 0x0A/0x0B), 2 bytes: byte0 = `range<2:0> bandwidth<5:3>`, byte1 = `power<2:0>`. Indexes into `GyrRangeDps`, `GyrBandwidthHz`, `GyrPowerNames`. Power-on 0x38/0x00 = 2000 dps, 32 Hz, normal.

#### Bno055GyroConfig.Default *(field)*

```csharp
public static readonly Bno055GyroConfig Default = new(0, 7, 0)
```

#### Bno055GyroConfig.Pack

```csharp
public byte[] Pack()
```

#### Bno055GyroConfig.Unpack

```csharp
public static Bno055GyroConfig Unpack(ReadOnlySpan<byte> data)
```

### Bno055MagConfig

```csharp
public readonly record struct Bno055MagConfig(int Rate, int Mode, int Power)
```

MAG_CONFIG (page 1, 0x09) `rate<2:0> mode<4:3> power<6:5>`: indexes into `MagRateHz`, `MagOprNames`, `MagPowerNames`. Bit 7 is not a field: `Unpack(v).Pack() == (v & 0x7F)`. Power-on 0x0B = 10 Hz, regular, normal.

#### Bno055MagConfig.Default *(field)*

```csharp
public static readonly Bno055MagConfig Default = new(3, 1, 0)
```

#### Bno055MagConfig.Pack

```csharp
public byte Pack()
```

#### Bno055MagConfig.Unpack

```csharp
public static Bno055MagConfig Unpack(byte value)
```

### Bno055SystemStatus

```csharp
public sealed record Bno055SystemStatus(int SelfTest, int ClkStatus, int Status, int Error)
```

ST_RESULT (0x36) + SYS_CLK_STATUS / SYS_STATUS / SYS_ERR (0x38..0x3A); INT_STA (0x37) sits in between and clears on read, so it is not read.

#### Bno055SystemStatus.StatusText *(property)*

```csharp
public string StatusText
```

#### Bno055SystemStatus.ErrorText *(property)*

```csharp
public string ErrorText
```

Meaningful when `Status` is 1 (system error).

#### Bno055SystemStatus.SelfTestPassed *(property)*

```csharp
public bool SelfTestPassed
```

### Bno055RawBlock

```csharp
public sealed record Bno055RawBlock(Bno055Vec3? Accel, Bno055Vec3? Mag, Bno055Vec3? Gyro, Bno055Euler? Euler, Bno055Quat? Quaternion, Bno055Vec3? LinearAccel, Bno055Vec3? Gravity, sbyte? Temperature, byte? CalibStat)
```

Raw register values found in one block read. A channel is null when the window addr..addr+len does not cover all of its bytes.

### Bno055Regs

```csharp
public static class Bno055Regs
```

BNO055 register map and the pure codecs every SDK shares (contracts/13_SENSOR_BNO055.md §4, Bosch BST-BNO055-DS000 rev 1.8) — the "base" layer. Mirrors `depz_sensor_sdk/bno055/regs.py` 1:1. Nothing here touches the wire; `vectors/bno055.json` pins it. Scaling is raw / LSB with `Bno055Units` and the fixed LSB constants below. The live driver over the bridge (mode switching, boot / fusion-start polls, page discipline, self-test) is `Bno055Device`.

#### Bno055Regs.ModeSwitchFromConfig *(field)*

```csharp
public static readonly TimeSpan ModeSwitchFromConfig = TimeSpan.FromMilliseconds(10)
```

Settle time after an OPR_MODE write out of CONFIG (datasheet: 7 ms).

#### Bno055Regs.ModeSwitchToConfig *(field)*

```csharp
public static readonly TimeSpan ModeSwitchToConfig = TimeSpan.FromMilliseconds(25)
```

Settle time after an OPR_MODE write into CONFIG (datasheet: 19 ms).

#### Bno055Regs.SelfTestTime *(field)*

```csharp
public static readonly TimeSpan SelfTestTime = TimeSpan.FromMilliseconds(450)
```

The built-in self-test runs ~400 ms (datasheet §3.9.2).

#### Bno055Regs.SysStatusBooting *(field)*

```csharp
public static readonly IReadOnlyList<int> SysStatusBooting = new[] { 2, 3, 4 }
```

SYS_STATUS values of a sensor still booting (initializing, self-test).

#### Bno055Regs.BootSettleTimeout *(field)*

```csharp
public static readonly TimeSpan BootSettleTimeout = TimeSpan.FromSeconds(1)
```

#### Bno055Regs.StuckSelfTestPolls *(constant)*

```csharp
public const int StuckSelfTestPolls = 20
```

SYS_STATUS 4 polls in a row taken as the leftover of a self-test run in CONFIG, not POST.

#### Bno055Regs.FusionStartTimeout *(field)*

```csharp
public static readonly TimeSpan FusionStartTimeout = TimeSpan.FromSeconds(1)
```

After CONFIG → a fusion mode the fusion outputs read zero for ~70 ms.

#### Bno055Regs.IsInterruptSettingReg

```csharp
public static bool IsInterruptSettingReg(int register)
```

Page-1 motion-interrupt setting registers (thresholds, durations, axis selects).

#### Bno055Regs.SysStatusNames *(field)*

```csharp
public static readonly IReadOnlyDictionary<int, string> SysStatusNames = …
```

#### Bno055Regs.SysErrNames *(field)*

```csharp
public static readonly IReadOnlyDictionary<int, string> SysErrNames = …
```

#### Bno055Regs.ChipId *(constant)*

```csharp
public const byte ChipId = 0x00
```

#### Bno055Regs.PageId *(constant)*

```csharp
public const byte PageId = 0x07
```

#### Bno055Regs.AccData *(constant)*

```csharp
public const byte AccData = 0x08
```

#### Bno055Regs.MagData *(constant)*

```csharp
public const byte MagData = 0x0E
```

#### Bno055Regs.GyrData *(constant)*

```csharp
public const byte GyrData = 0x14
```

#### Bno055Regs.EulData *(constant)*

```csharp
public const byte EulData = 0x1A
```

#### Bno055Regs.QuaData *(constant)*

```csharp
public const byte QuaData = 0x20
```

#### Bno055Regs.LiaData *(constant)*

```csharp
public const byte LiaData = 0x28
```

#### Bno055Regs.GrvData *(constant)*

```csharp
public const byte GrvData = 0x2E
```

#### Bno055Regs.Temp *(constant)*

```csharp
public const byte Temp = 0x34
```

#### Bno055Regs.CalibStat *(constant)*

```csharp
public const byte CalibStat = 0x35
```

#### Bno055Regs.StResult *(constant)*

```csharp
public const byte StResult = 0x36
```

#### Bno055Regs.IntSta *(constant)*

```csharp
public const byte IntSta = 0x37
```

Clear-on-read — never part of a routine block read.

#### Bno055Regs.SysClkStatus *(constant)*

```csharp
public const byte SysClkStatus = 0x38
```

#### Bno055Regs.SysStatus *(constant)*

```csharp
public const byte SysStatus = 0x39
```

#### Bno055Regs.SysErr *(constant)*

```csharp
public const byte SysErr = 0x3A
```

#### Bno055Regs.UnitSel *(constant)*

```csharp
public const byte UnitSel = 0x3B
```

#### Bno055Regs.OprMode *(constant)*

```csharp
public const byte OprMode = 0x3D
```

#### Bno055Regs.PwrMode *(constant)*

```csharp
public const byte PwrMode = 0x3E
```

#### Bno055Regs.SysTrigger *(constant)*

```csharp
public const byte SysTrigger = 0x3F
```

#### Bno055Regs.TempSource *(constant)*

```csharp
public const byte TempSource = 0x40
```

#### Bno055Regs.AxisMapConfig *(constant)*

```csharp
public const byte AxisMapConfig = 0x41
```

#### Bno055Regs.AxisMapSign *(constant)*

```csharp
public const byte AxisMapSign = 0x42
```

#### Bno055Regs.SicMatrix *(constant)*

```csharp
public const byte SicMatrix = 0x43
```

9 × i16, row-major, 1.0 = 16384.

#### Bno055Regs.CalibProfile *(constant)*

```csharp
public const byte CalibProfile = 0x55
```

acc/mag/gyr offsets + acc/mag radius, `CalibProfileLen` bytes.

#### Bno055Regs.CalibProfileLen *(constant)*

```csharp
public const int CalibProfileLen = 22
```

#### Bno055Regs.P1AccConfig *(constant)*

```csharp
public const byte P1AccConfig = 0x08
```

#### Bno055Regs.P1MagConfig *(constant)*

```csharp
public const byte P1MagConfig = 0x09
```

#### Bno055Regs.P1GyrConfig0 *(constant)*

```csharp
public const byte P1GyrConfig0 = 0x0A
```

#### Bno055Regs.P1GyrConfig1 *(constant)*

```csharp
public const byte P1GyrConfig1 = 0x0B
```

#### Bno055Regs.P1AccSleepConfig *(constant)*

```csharp
public const byte P1AccSleepConfig = 0x0C
```

#### Bno055Regs.P1GyrSleepConfig *(constant)*

```csharp
public const byte P1GyrSleepConfig = 0x0D
```

#### Bno055Regs.P1IntMsk *(constant)*

```csharp
public const byte P1IntMsk = 0x0F
```

#### Bno055Regs.P1IntEn *(constant)*

```csharp
public const byte P1IntEn = 0x10
```

#### Bno055Regs.P1AccAmThres *(constant)*

```csharp
public const byte P1AccAmThres = 0x11
```

Page-1 0x11..0x1F are the motion-interrupt settings, written raw.

#### Bno055Regs.P1GyrAmSet *(constant)*

```csharp
public const byte P1GyrAmSet = 0x1F
```

#### Bno055Regs.P1UniqueId *(constant)*

```csharp
public const byte P1UniqueId = 0x50
```

#### Bno055Regs.UniqueIdLen *(constant)*

```csharp
public const int UniqueIdLen = 16
```

#### Bno055Regs.FullBlockAddr *(constant)*

```csharp
public const byte FullBlockAddr = AccData
```

The block that carries every output channel: 0x08 (ACC_DATA_X_LSB) … 0x35 (CALIB_STAT).

#### Bno055Regs.FullBlockLen *(constant)*

```csharp
public const byte FullBlockLen = CalibStat - AccData + 1
```

#### Bno055Regs.QuatBlockAddr *(constant)*

```csharp
public const byte QuatBlockAddr = QuaData
```

Quaternion only — the cheapest orientation read (8 bytes, ~1.2 ms of bus).

#### Bno055Regs.QuatBlockLen *(constant)*

```csharp
public const byte QuatBlockLen = 8
```

#### Bno055Regs.SysTriggerSelfTest *(constant)*

```csharp
public const byte SysTriggerSelfTest = 0x01
```

#### Bno055Regs.SysTriggerRstSys *(constant)*

```csharp
public const byte SysTriggerRstSys = 0x20
```

#### Bno055Regs.SysTriggerRstInt *(constant)*

```csharp
public const byte SysTriggerRstInt = 0x40
```

#### Bno055Regs.SysTriggerClkSel *(constant)*

```csharp
public const byte SysTriggerClkSel = 0x80
```

#### Bno055Regs.IntAccBsxDrdy *(constant)*

```csharp
public const byte IntAccBsxDrdy = 0x01
```

#### Bno055Regs.IntMagDrdy *(constant)*

```csharp
public const byte IntMagDrdy = 0x02
```

#### Bno055Regs.IntGyrAm *(constant)*

```csharp
public const byte IntGyrAm = 0x04
```

#### Bno055Regs.IntGyrHighRate *(constant)*

```csharp
public const byte IntGyrHighRate = 0x08
```

#### Bno055Regs.IntGyrDrdy *(constant)*

```csharp
public const byte IntGyrDrdy = 0x10
```

#### Bno055Regs.IntAccHighG *(constant)*

```csharp
public const byte IntAccHighG = 0x20
```

#### Bno055Regs.IntAccAm *(constant)*

```csharp
public const byte IntAccAm = 0x40
```

#### Bno055Regs.IntAccNm *(constant)*

```csharp
public const byte IntAccNm = 0x80
```

#### Bno055Regs.StAcc *(constant)*

```csharp
public const byte StAcc = 0x01
```

#### Bno055Regs.StMag *(constant)*

```csharp
public const byte StMag = 0x02
```

#### Bno055Regs.StGyr *(constant)*

```csharp
public const byte StGyr = 0x04
```

#### Bno055Regs.StMcu *(constant)*

```csharp
public const byte StMcu = 0x08
```

#### Bno055Regs.ExpectedSelfTest *(constant)*

```csharp
public const byte ExpectedSelfTest = StAcc | StMag | StGyr | StMcu
```

#### Bno055Regs.MagLsb *(constant)*

```csharp
public const double MagLsb = 16.0
```

MAG_DATA LSB per µT, not selectable.

#### Bno055Regs.QuatLsb *(constant)*

```csharp
public const double QuatLsb = 16384.0
```

Quaternion LSB, 2^14, unit-less.

#### Bno055Regs.FusionAccelLsb *(constant)*

```csharp
public const double FusionAccelLsb = 100.0
```

Linear acceleration and gravity ignore the ACC_Unit bit: always m/s² at 100 LSB — measured on SW rev 03.11 (datasheet Tables 3-33/3-35 claim mg).

#### Bno055Regs.IsFusion

```csharp
public static bool IsFusion(Bno055OprMode mode)
```

True for the fusion operating modes (IMU and above).

#### Bno055Regs.AccRangeG *(field)*

```csharp
public static readonly IReadOnlyList<int> AccRangeG = new[] { 2, 4, 8, 16 }
```

#### Bno055Regs.AccBandwidthHz *(field)*

```csharp
public static readonly IReadOnlyList<double> AccBandwidthHz = …
```

#### Bno055Regs.AccPowerNames *(field)*

```csharp
public static readonly IReadOnlyList<string> AccPowerNames = …
```

#### Bno055Regs.GyrRangeDps *(field)*

```csharp
public static readonly IReadOnlyList<int> GyrRangeDps = new[] { 2000, 1000, 500, 250, 125 }
```

#### Bno055Regs.GyrBandwidthHz *(field)*

```csharp
public static readonly IReadOnlyList<int> GyrBandwidthHz = new[] { 523, 230, 116, 47, 23, 12, 64, 32 }
```

#### Bno055Regs.GyrPowerNames *(field)*

```csharp
public static readonly IReadOnlyList<string> GyrPowerNames = …
```

#### Bno055Regs.MagRateHz *(field)*

```csharp
public static readonly IReadOnlyList<int> MagRateHz = new[] { 2, 6, 8, 10, 15, 20, 25, 30 }
```

#### Bno055Regs.MagOprNames *(field)*

```csharp
public static readonly IReadOnlyList<string> MagOprNames = …
```

#### Bno055Regs.MagPowerNames *(field)*

```csharp
public static readonly IReadOnlyList<string> MagPowerNames = new[] { "normal", "sleep", "suspend", "force" }
```

#### Bno055Regs.SicIdentity *(field)*

```csharp
public static readonly IReadOnlyList<short> SicIdentity = new short[] { 16384, 0, 0, 0, 16384, 0, 0, 0, 16384 }
```

Soft-iron matrix identity, 1.0 = 16384.

#### Bno055Regs.PackSicMatrix

```csharp
public static byte[] PackSicMatrix(IReadOnlyList<short> m)
```

Soft-iron matrix, 9 × i16 LE row-major, 1.0 = 16384 (datasheet §3.11.4).

#### Bno055Regs.UnpackSicMatrix

```csharp
public static short[] UnpackSicMatrix(ReadOnlySpan<byte> data)
```

#### Bno055Regs.DecodeBlock

```csharp
public static Bno055RawBlock DecodeBlock(byte addr, ReadOnlySpan<byte> data)
```

Unpack whatever channels the register window starting at `addr` holds.

### Bno055Sample

```csharp
public sealed record Bno055Sample(ulong TimestampUs, int Addr, byte[] Raw, Bno055Units Units, double[]? Accel, double[]? Mag, double[]? Gyro, double[]? Euler, double[]? Quaternion, double[]? LinearAccel, double[]? Gravity, double? Temperature, Bno055CalibStatus? Calibration)
```

One decoded BNO055 register block (streamed or polled). A channel is null when the block did not cover it; values are scaled by `Units`, the units in effect when the stream started. Euler is (heading, roll, pitch), the quaternion (w, x, y, z); linear acceleration and gravity are always m/s². In non-fusion modes the fusion outputs read zero; in CONFIG everything does.

#### Bno055Sample.Decode

```csharp
public static Bno055Sample Decode(ulong timestampUs, int addr, byte[] data, Bno055Units units)
```

Decode `data` read at `addr` with `units`.

### Bno055Device

```csharp
public sealed class Bno055Device : DepzDevice
```

BNO055 absolute-orientation IMU (contract 13). The board is a thin I2C register bridge: mode, units, axis remap and calibration are register access here; the sensor fuses on chip at 100 Hz. Typical use: `Configure`, then `StartStream` and read `Samples`, or poll `ReadSample`. Page 1 (sensor configs, interrupts, unique id) is reached through PAGE_ID and refused while streaming.

#### Bno055Device.Bno055Device *(constructor)*

```csharp
public Bno055Device(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

#### Bno055Device.Bno055Device *(constructor)*

```csharp
#if NET public Bno055Device(string port, TimeSpan? timeout = null) : base(port, timeout)
```

#### Bno055Device.BridgeInfo

```csharp
public Bno055Info BridgeInfo()
```

RPT_BNO_INFO: chip ids, sensor firmware revision, bridge counters. Safe while streaming.

#### Bno055Device.IsAlive

```csharp
public bool IsAlive()
```

True when the bridge passed the chip-id handshake and the sensor answers with the BNO055 ids.

#### Bno055Device.ResetSensor

```csharp
public void ResetSensor()
```

Hardware reset through nRESET (the bridge answers after the ~0.5 s boot handshake). Stops a stream; the sensor returns in CONFIG with power-on units and no calibration — `Configure` again, or `RestoreConfiguration`.

#### Bno055Device.ReadRegisters

```csharp
public byte[] ReadRegisters(int addr, int length, int page = 0)
```

Read `length` bytes at `addr` on `page` (128-byte transfers). Page 1 is refused while streaming.

#### Bno055Device.WriteRegisters

```csharp
public void WriteRegisters(int addr, byte[] data, int page = 0)
```

Write at `addr` on `page`. Most configuration registers only take writes in CONFIG mode (the sensor ignores the rest); the typed setters handle that.

#### Bno055Device.ReadRegister

```csharp
public int ReadRegister(int addr, int page = 0)
```

#### Bno055Device.WriteRegister

```csharp
public void WriteRegister(int addr, int value, int page = 0)
```

#### Bno055Device.GetOperationMode

```csharp
public Bno055OprMode GetOperationMode()
```

#### Bno055Device.SetOperationMode

```csharp
public void SetOperationMode(Bno055OprMode mode)
```

Switch OPR_MODE and wait out the switching time; into a fusion mode also wait (up to 1 s) for the fusion outputs. The sensor only switches between CONFIG and another mode, so a switch between two operating modes goes through CONFIG.

#### Bno055Device.GetPowerMode

```csharp
public Bno055PwrMode GetPowerMode()
```

#### Bno055Device.SetPowerMode

```csharp
public void SetPowerMode(Bno055PwrMode mode)
```

Normal / low power / suspend, written in CONFIG with the operating mode restored after. A suspended chip ignores OPR_MODE writes, so the mode is kept here and put back on wake-up (NDOF when the chip was already asleep at connect, contract 13 §4.6).

#### Bno055Device.GetUnits

```csharp
public Bno055Units GetUnits()
```

#### Bno055Device.SetUnits

```csharp
public void SetUnits(Bno055Units units)
```

#### Bno055Device.GetAxisRemap

```csharp
public Bno055AxisRemap GetAxisRemap()
```

#### Bno055Device.SetAxisRemap

```csharp
public void SetAxisRemap(Bno055AxisRemap remap)
```

Remap the output axes for the board's mounting (`Placement` gives the datasheet placements P0..P7; P1 is the default).

#### Bno055Device.GetTemperatureSource

```csharp
public Bno055TempSource GetTemperatureSource()
```

#### Bno055Device.SetTemperatureSource

```csharp
public void SetTemperatureSource(Bno055TempSource source)
```

#### Bno055Device.Configure

```csharp
public void Configure(Bno055OprMode mode = Bno055OprMode.Ndof, Bno055Units? units = null, Bno055AxisRemap? axisRemap = null, Bno055CalibrationProfile? calibration = null)
```

The usual session setup: CONFIG → units → axis remap → calibration profile → `mode`; in a fusion mode it returns once the fusion outputs are live (~70 ms). Remembered for `RestoreConfiguration`.

#### Bno055Device.RestoreConfiguration

```csharp
public void RestoreConfiguration()
```

Re-apply the last `Configure` — after `ResetSensor`, or when `BridgeInfo().SensorResets` rose (bus recovery pulses nRESET). A running stream keeps running.

#### Bno055Device.SystemStatus

```csharp
public Bno055SystemStatus SystemStatus()
```

ST_RESULT, SYS_CLK_STATUS, SYS_STATUS, SYS_ERR.

#### Bno055Device.SelfTest

```csharp
public Bno055SystemStatus SelfTest()
```

The built-in self-test (datasheet §3.9.2), in CONFIG, ~400 ms; the operating mode is restored after. Not while streaming.

#### Bno055Device.CalibrationStatus

```csharp
public Bno055CalibStatus CalibrationStatus()
```

CALIB_STAT: 0..3 per sensor; in NDOF the sensor calibrates in the background.

#### Bno055Device.ReadCalibrationProfile

```csharp
public Bno055CalibrationProfile ReadCalibrationProfile()
```

Offsets and radii (0x55..0x6A), only readable in CONFIG: the driver switches there and back.

#### Bno055Device.WriteCalibrationProfile

```csharp
public void WriteCalibrationProfile(Bno055CalibrationProfile profile)
```

Restore a stored profile (datasheet §3.11.5). A starting point, not a lock: the background calibration refines it once fusion resumes.

#### Bno055Device.GetSicMatrix

```csharp
public short[] GetSicMatrix()
```

Soft-iron compensation matrix, 9 × i16 row-major, 1.0 = 16384.

#### Bno055Device.SetSicMatrix

```csharp
public void SetSicMatrix(IReadOnlyList<short>? matrix = null)
```

#### Bno055Device.GetAccelConfig

```csharp
public Bno055AccelConfig GetAccelConfig()
```

#### Bno055Device.SetAccelConfig

```csharp
public void SetAccelConfig(Bno055AccelConfig config)
```

Accelerometer range / bandwidth / power; fusion modes override it.

#### Bno055Device.GetGyroConfig

```csharp
public Bno055GyroConfig GetGyroConfig()
```

#### Bno055Device.SetGyroConfig

```csharp
public void SetGyroConfig(Bno055GyroConfig config)
```

#### Bno055Device.GetMagConfig

```csharp
public Bno055MagConfig GetMagConfig()
```

#### Bno055Device.SetMagConfig

```csharp
public void SetMagConfig(Bno055MagConfig config)
```

#### Bno055Device.UniqueId

```csharp
public byte[] UniqueId()
```

The chip's 16-byte unique id (page 1, 0x50..0x5F).

#### Bno055Device.GetInterruptEnable

```csharp
public int GetInterruptEnable()
```

#### Bno055Device.SetInterruptEnable

```csharp
public void SetInterruptEnable(int mask)
```

INT_EN: which interrupt engines run.

#### Bno055Device.GetInterruptMask

```csharp
public int GetInterruptMask()
```

#### Bno055Device.SetInterruptMask

```csharp
public void SetInterruptMask(int mask)
```

INT_MSK: which enabled interrupts drive the INT pin.

#### Bno055Device.SetInterruptSetting

```csharp
public void SetInterruptSetting(int register, int value)
```

One motion-interrupt setting register (page 1, 0x11..0x1F), as a raw byte. Not while streaming.

#### Bno055Device.ReadInterruptStatus

```csharp
public int ReadInterruptStatus()
```

INT_STA — which interrupts fired. Clears on read.

#### Bno055Device.ClearInterrupt

```csharp
public void ClearInterrupt()
```

SYS_TRIGGER RST_INT: reset the interrupt status bits and the INT pin.

#### Bno055Device.ReadSample

```csharp
public Bno055Sample ReadSample((byte Addr, byte Length)? block = null)
```

Poll one register block (default the full 46 bytes 0x08..0x35) and decode it. Works alongside a stream.

#### Bno055Device.ReadQuaternion

```csharp
public double[] ReadQuaternion()
```

(w, x, y, z) — the cheapest orientation read (8 bytes).

#### Bno055Device.StartStream

```csharp
public void StartStream(int periodMs = 10, (byte Addr, byte Length)? block = null, Bno055Trigger trigger = Bno055Trigger.Timer)
```

Arm the bridge to read `block` (default the full block) every `periodMs` and push it. Fusion runs at 100 Hz, so 10 ms is the useful floor. `Int` reads on the INT edge, `periodMs` then a missed-edge watchdog (0 = none). Replaces a running stream.

#### Bno055Device.StopStream

```csharp
public void StopStream()
```

#### Bno055Device.Streaming *(property)*

```csharp
public bool Streaming
```

#### Bno055Device.OnSample

```csharp
public IDisposable OnSample(Action<Bno055Sample> callback)
```

Call `callback` for every streamed sample (reader thread).

#### Bno055Device.Samples

```csharp
public Subscription<Bno055Sample> Samples(int capacity = 256)
```

A pull queue of streamed samples (bounded, drop-oldest).

#### Bno055Device.GetSample

```csharp
public Bno055Sample GetSample(TimeSpan? timeout = null)
```

The next streamed sample (contract 07 §3).

#### Bno055Device.StreamParseErrors *(property)*

```csharp
public long StreamParseErrors
```

Stream reports dropped because they did not decode (a short block).

#### Bno055Device.StreamDroppedCounts *(property)*

```csharp
public IReadOnlyList<long> StreamDroppedCounts
```

## TMF8829 (dToF)

### Tmf8829Exception

```csharp
public sealed class Tmf8829Exception : DepzException
```

The sensor refused or did not answer (bootloader / application status).

#### Tmf8829Exception.Tmf8829Exception *(constructor)*

```csharp
public Tmf8829Exception(string message) : base(message)
```

### Tmf8829InterruptEvent

```csharp
public sealed record Tmf8829InterruptEvent(Tmf8829Event Event) : DeviceEvent
```

RPT_TMF_EVENT that is not an expected stop artefact: a MOTION / PROXIMITY interrupt (code 0) or a refused frame header (code 1 — the sensor is lost, run `Init` again).

### Tmf8829AppliedConfig

```csharp
public sealed record Tmf8829AppliedConfig(string Preset, byte[] Page, bool Histograms, bool RefFrame, IReadOnlyDictionary<string, int> Fields)
```

The config `Configure` wrote, as read back from the sensor.

#### Tmf8829AppliedConfig.public

```csharp
public (int Rows, int Cols) Grid
```

(rows, cols) of a measurement.

### Tmf8829Device

```csharp
public sealed class Tmf8829Device : DepzDevice
```

TMF8829 multizone dToF (8×8 … 48×32 zones, up to 4 peaks per zone, histograms) over the SPI register bridge (contract 14). The board is a thin bridge: this class runs the sensor. `Init` downloads the 14.4 KB sensor firmware (~0.15 s), `Configure` writes the config page, `Start` streams measurements to `OnMeasurement` / `Measurements`, `Stop` ends it. The sensor's EN pin is wired high: there is no sleep, and recovery from any state is `Init` (it starts with a soft reset).

#### Tmf8829Device.Tmf8829Device *(constructor)*

```csharp
public Tmf8829Device(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

#### Tmf8829Device.Tmf8829Device *(constructor)*

```csharp
#if NET public Tmf8829Device(string port, TimeSpan? timeout = null) : base(port, timeout)
```

#### Tmf8829Device.AppVersion *(property)*

```csharp
public string? AppVersion
```

The RAM application version `Init` started ("2.200.0"), or null.

#### Tmf8829Device.SensorSerial *(property)*

```csharp
public uint? SensorSerial
```

The sensor's serial number, read by `Init`, or null.

#### Tmf8829Device.Config *(property)*

```csharp
public Tmf8829AppliedConfig? Config
```

The last `Configure` result (null after `Init`).

#### Tmf8829Device.Streaming *(property)*

```csharp
public bool Streaming
```

#### Tmf8829Device.ReadRegisters

```csharp
public byte[] ReadRegisters(int addr, int length)
```

Read `length` bytes from `addr` in ≤1024-byte requests; reads that reach the FIFO port 0xFF stay there (the sensor does not auto-increment it).

#### Tmf8829Device.ReadRegister

```csharp
public int ReadRegister(int addr)
```

#### Tmf8829Device.WriteRegisters

```csharp
public void WriteRegisters(int addr, byte[] data, Action<int, int>? progress = null)
```

Write `data` from `addr` in ≤1023-byte requests (writes to 0xFF stay at 0xFF); `progress` gets (done, total) after each.

#### Tmf8829Device.WriteRegister

```csharp
public void WriteRegister(int addr, int value)
```

Write one register; the low byte of `value`.

#### Tmf8829Device.SensorState

```csharp
public IReadOnlyDictionary<string, int> SensorState()
```

Raw host-interface registers, for diagnostics.

#### Tmf8829Device.GetBridgeInfo

```csharp
public Tmf8829Info GetBridgeInfo()
```

Bridge counters (never touches the sensor, safe mid-stream).

#### Tmf8829Device.public

```csharp
public (string AppVersion, uint SensorSerial) Init(Action<int, int>? progress = null)
```

Bring the sensor from any state to its running RAM application: soft reset, bootloader, firmware download, start. Returns (application version, sensor serial). Stops a running stream first. `progress` (done, total) tracks the image download.

#### Tmf8829Device.ConfigPage

```csharp
public byte[] ConfigPage()
```

LOAD_CONFIG_PAGE → the 190-byte page 0x22..0xDF now in the sensor.

#### Tmf8829Device.Configure

```csharp
public Tmf8829AppliedConfig Configure(string preset = "8x8", int periodMs = Tmf8829Config.DefaultPeriodMs, int? kiloIterations = null, int nrPeaks = 1, bool signalStrength = false, bool noiseStrength = false, bool xtalk = false, bool histograms = false, bool refFrame = false, IReadOnlyDictionary<string, int>? fields = null)
```

Load `preset` and write the config page. Every field a preset does not reset is written (contract 14 §4), so the result never depends on an earlier config; `fields` sets any other page field by name. Not while streaming.

#### Tmf8829Device.Start

```csharp
public void Start()
```

MEASURE with the bridge stream armed. Configures the default 8x8 preset first when `Configure` was not called since `Init`.

#### Tmf8829Device.Stop

```csharp
public void Stop()
```

Sensor STOP, then the bridge stream (the frames already queued drain). The bad-header event a STOP raises mid histogram dump is expected and swallowed.

#### Tmf8829Device.OnMeasurement

```csharp
public IDisposable OnMeasurement(Action<Tmf8829Measurement> callback)
```

Call `callback` for every measurement (reader thread; don't block).

#### Tmf8829Device.Measurements

```csharp
public Subscription<Tmf8829Measurement> Measurements(int capacity = 16)
```

A pull queue of measurements (bounded, drop-oldest); subscribed when this returns.

#### Tmf8829Device.GetMeasurement

```csharp
public Tmf8829Measurement GetMeasurement(TimeSpan? timeout = null)
```

The next measurement (contract 07 §3).

#### Tmf8829Device.StreamDroppedCounts *(property)*

```csharp
public IReadOnlyList<long> StreamDroppedCounts
```

#### Tmf8829Device.FramesDropped *(property)*

```csharp
public int FramesDropped
```

Frames the bridge dropped (frame_seq gaps) in this stream.

#### Tmf8829Device.FramesDiscarded *(property)*

```csharp
public int FramesDiscarded
```

Frames lost to reassembly (incomplete, out-of-order chunks) in this stream.

#### Tmf8829Device.FrameParseErrors *(property)*

```csharp
public int FrameParseErrors
```

#### Tmf8829Device.ClockRatioQ16 *(property)*

```csharp
public uint ClockRatioQ16
```

Current sensor-vs-board clock ratio in Q16 (contract 14 §5; nominal 8 << 16 = 8 µs per SYSTICK), for diagnostics.

### Tmf8829Cmd

```csharp
public enum Tmf8829Cmd
{
    ReadReg = 0x32,
    WriteReg = 0x33,
    StartStream = 0x35,
    StopStream = 0x36,
    GetInfo = 0x37,
}
```

TMF8829 SPI register-bridge command opcodes, protocol v1.00 (contracts/14_SENSOR_TMF8829.md §2). Same numbering as the VL53 bridges but TMF8829 payloads: an 8-bit register address and self-sized frames. 0x34 is reserved; 0x30/0x31 (AUX sync) answer ERR_CMD_NOT_SUPPORTED.

### Tmf8829Rpt

```csharp
public enum Tmf8829Rpt
{
    RegData = 0x91,
    Info = 0x92,
    Frame = 0x93,
    Event = 0x94,
}
```

TMF8829 report opcodes (contract 14 §2).

### Tmf8829EventCode

```csharp
public static class Tmf8829EventCode
```

`Code` values.

#### Tmf8829EventCode.Int *(constant)*

```csharp
public const byte Int = 0
```

MOTION/PROXIMITY interrupt, no frame; its bits already cleared.

#### Tmf8829EventCode.BadHeader *(constant)*

```csharp
public const byte BadHeader = 1
```

Frame header refused, frame not read (expected once on STOP mid histogram dump).

### Tmf8829Wire

```csharp
public static class Tmf8829Wire
```

Wire limits and command payload codecs of the TMF8829 bridge (`APP_TMF8829`). The MCU is a thin SPI register bridge: the host runs the sensor (firmware download, config page, presets, frame parsing, clock correction). Mirrors `depz_sensor_sdk/protocol/tmf8829.py` 1:1. TMF_STOP_STREAM and TMF_GET_INFO carry an empty payload.

#### Tmf8829Wire.ReadMax *(constant)*

```csharp
public const int ReadMax = 1024
```

TMF_READ_REG `len` 1..1024.

#### Tmf8829Wire.WriteMax *(constant)*

```csharp
public const int WriteMax = 1023
```

TMF_WRITE_REG data 1..1023 bytes (1024 B buffer minus the address).

#### Tmf8829Wire.ChunkMax *(constant)*

```csharp
public const int ChunkMax = 1024
```

Frame bytes per RPT_TMF_FRAME.

#### Tmf8829Wire.IntResults *(constant)*

```csharp
public const byte IntResults = 0x01
```

INT_STATUS bits the bridge serves (TMF_START_STREAM `int_mask`).

#### Tmf8829Wire.IntMotion *(constant)*

```csharp
public const byte IntMotion = 0x02
```

#### Tmf8829Wire.IntProximity *(constant)*

```csharp
public const byte IntProximity = 0x04
```

#### Tmf8829Wire.IntHistograms *(constant)*

```csharp
public const byte IntHistograms = 0x08
```

#### Tmf8829Wire.IntMaskAll *(constant)*

```csharp
public const byte IntMaskAll = 0x0F
```

#### Tmf8829Wire.SfRefFrame *(constant)*

```csharp
public const byte SfRefFrame = 0x01
```

TMF_START_STREAM flags: emit the ref-SPAD frame before each results frame.

#### Tmf8829Wire.RegDataHeaderSize *(constant)*

```csharp
public const int RegDataHeaderSize = 9
```

#### Tmf8829Wire.FrameChunkHeaderSize *(constant)*

```csharp
public const int FrameChunkHeaderSize = 21
```

#### Tmf8829Wire.EventSize *(constant)*

```csharp
public const int EventSize = 12
```

#### Tmf8829Wire.InfoSize *(constant)*

```csharp
public const int InfoSize = 25
```

#### Tmf8829Wire.RegisterTimeout *(field)*

```csharp
public static readonly TimeSpan RegisterTimeout = TimeSpan.FromSeconds(1)
```

Command timeout while streaming: a 25 KB histogram frame in flight delays READ_REG / WRITE_REG replies (SDKs may use it for all commands; 200 ms otherwise).

#### Tmf8829Wire.PackReadReg

```csharp
public static byte[] PackReadReg(int addr, int len)
```

TMF_READ_REG payload: addr u8, len u16 LE (len 1..`ReadMax`; no addr+len limit — the sensor stays at 0xFF, the FIFO).

#### Tmf8829Wire.PackWriteReg

```csharp
public static byte[] PackWriteReg(int addr, ReadOnlySpan<byte> data)
```

TMF_WRITE_REG payload: addr u8, then data[1..`WriteMax`].

#### Tmf8829Wire.PackStartStream

```csharp
public static byte[] PackStartStream(int intMask, int flags = 0)
```

TMF_START_STREAM payload: int_mask u8 (non-zero subset of `IntMaskAll`), flags u8 (only `SfRefFrame`). Implies STOP first and restarts `frame_seq` at 0.

### Tmf8829RegData

```csharp
public sealed record Tmf8829RegData(ulong TimestampUs, byte[] Data)
```

RPT_TMF_REG_DATA: register bytes and the MCU time the SPI read completed.

#### Tmf8829RegData.Unpack

```csharp
public static Tmf8829RegData Unpack(ReadOnlySpan<byte> p)
```

Parse a RPT_TMF_REG_DATA payload: cmd u8 (= 0x32), timestamp u64 LE, then data.

### Tmf8829Info

```csharp
public sealed record Tmf8829Info(uint IntEdges, uint Frames, uint FramesAborted, uint FrameErrors, uint SpiTimeouts, byte IntLevel, bool Streaming, byte IntMask, byte LastIntStatus, byte LastFifoStatus)
```

RPT_TMF_INFO (25 bytes): bridge state only, the sensor is never touched. Carries no echoed command byte. `IntEdges` runs from MCU power-up; the frame counters restart at TMF_START_STREAM. `IntLevel`: PA4 level now, 0 = asserted.

#### Tmf8829Info.Unpack

```csharp
public static Tmf8829Info Unpack(ReadOnlySpan<byte> p)
```

Parse a RPT_TMF_INFO payload (`<IIIIIBBBBB`, 25 bytes LE).

### Tmf8829FrameChunk

```csharp
public sealed record Tmf8829FrameChunk(ulong TimestampUs, ushort FullSize, ushort Offset, byte FrameSeq, ulong EdgeUs, byte[] Data)
```

One RPT_TMF_FRAME chunk. `TimestampUs` + the frame's pre-header SYSTICK are a clock pair (not the capture time); `EdgeUs` is the INT falling edge credited to the frame, 0 = none.

#### Tmf8829FrameChunk.Unpack

```csharp
public static Tmf8829FrameChunk Unpack(ReadOnlySpan<byte> p)
```

Parse: timestamp u64, full_size u16, offset u16, frame_seq u8, edge_us u64, then ≤ 1024 frame bytes.

### Tmf8829Event

```csharp
public sealed record Tmf8829Event(ulong TimestampUs, byte IntStatus, byte Code, byte FrameId, byte FifoStatus)
```

RPT_TMF_EVENT (12 bytes). `IntStatus` is INT_STATUS as read (unmasked); `Code` is a `Tmf8829EventCode`; `FrameId` / `FifoStatus` are the refused header's id byte and pre-header byte (BAD_HEADER), else 0.

#### Tmf8829Event.Unpack

```csharp
public static Tmf8829Event Unpack(ReadOnlySpan<byte> p)
```

### Tmf8829RawFrame

```csharp
public sealed record Tmf8829RawFrame(ulong TimestampUs, ulong EdgeUs, byte FrameSeq, byte[] Data)
```

A reassembled frame: the sensor bytes from register 0xFA on.

### Tmf8829FrameReassembler

```csharp
public sealed class Tmf8829FrameReassembler
```

Rebuilds frames from RPT_TMF_FRAME chunks (contract 14 §2): reset on offset 0; append only a chunk of the same frame_seq (and full_size) at the expected offset; complete at full_size. Never stitches chunks of different frame_seq. `SeqGaps` counts frames the bridge dropped (frame_seq jumps, u8 wrap).

#### Tmf8829FrameReassembler.Completed *(property)*

```csharp
public int Completed
```

Frames returned complete.

#### Tmf8829FrameReassembler.Discarded *(property)*

```csharp
public int Discarded
```

Partial frames thrown away (restarted, out of order, overrun).

#### Tmf8829FrameReassembler.SeqGaps *(property)*

```csharp
public int SeqGaps
```

Frames the bridge dropped, from frame_seq jumps.

#### Tmf8829FrameReassembler.Reset

```csharp
public void Reset()
```

Forget everything — call at every stream start (frame_seq restarts at 0).

#### Tmf8829FrameReassembler.Feed

```csharp
public Tmf8829RawFrame? Feed(Tmf8829FrameChunk chunk)
```

Feed one chunk → the frame it completes, or null.

### Tmf8829FrameException

```csharp
public sealed class Tmf8829FrameException : FormatException
```

A frame whose bytes do not match its own header (contract 14 §3).

#### Tmf8829FrameException.Tmf8829FrameException *(constructor)*

```csharp
public Tmf8829FrameException(string message) : base(message)
```

### Tmf8829FrameInfo

```csharp
public sealed record Tmf8829FrameInfo(byte FifoStatus, uint Systick, byte Id, byte Layout, ushort Payload, uint FNumber, sbyte[] Temperature, byte Bdv, ushort[] RefPos, uint T0Integration, uint T1Integration, byte FrameStatus, byte Reserved, ushort Eof, int Size)
```

Pre-header, header and footer of one frame of any kind (contracts/14_SENSOR_TMF8829.md §3). A frame is the sensor bytes from register 0xFA on: 5-byte pre-header (FIFOSTATUS, SYSTICK), 16-byte header, data, 12-byte footer. `Systick`, `T0Integration` and `T1Integration` are sensor SYSTICK (125 kHz) values. `Temperature` holds 3 signed bytes, `RefPos` 2 u16.

#### Tmf8829FrameInfo.Kind *(property)*

```csharp
public int Kind
```

`FidResults`, `FidHistograms` or `FidRefSpad`.

#### Tmf8829FrameInfo.FpMode *(property)*

```csharp
public int FpMode
```

Focal-plane mode: 0 8×8A, 1 8×8B, 2 16×16, 3 32×32, 4 32×32s, 5 48×32.

#### Tmf8829FrameInfo.IsSecondHalf *(property)*

```csharp
public bool IsSecondHalf
```

32×32/48×32: the second (odd-row) half-frame.

#### Tmf8829FrameInfo.DistanceInMm *(property)*

```csharp
public bool DistanceInMm
```

Distances in mm (footer.reserved b0, preset 8x8-xr), else 0.25 mm.

#### Tmf8829FrameInfo.Problems

```csharp
public List<string> Problems()
```

What is wrong with the frame, human-readable; empty when fine.

### Tmf8829PixelResults

```csharp
public sealed record Tmf8829PixelResults(int Peaks, int GridRows, int GridCols, ushort[]? Distance, byte[]? Snr, ushort[]? Signal, ushort[]? Noise, ushort[]? Xtalk)
```

Decoded results of one frame or of a merged measurement. Raw sensor integers: distance in 0.25 mm (or mm with the footer flag), not yet clock-corrected. Per-peak arrays (`Distance`, `Snr`, `Signal`) are flat `[peaks][rows][cols]`, index `(p * rows + r) * cols + c`; `Noise` / `Xtalk` are flat `[rows][cols]`. Absent fields are null.

#### Tmf8829PixelResults.Rows *(property)*

```csharp
public int Rows
```

Rows of the arrays (0 when the result format carries none).

#### Tmf8829PixelResults.Cols *(property)*

```csharp
public int Cols
```

Columns of the arrays (0 when the result format carries none).

#### Tmf8829PixelResults.DistanceAt

```csharp
public ushort DistanceAt(int p, int r, int c)
```

Raw distance of peak `p` at (`r`, `c`).

### Tmf8829HistogramFrame

```csharp
public sealed record Tmf8829HistogramFrame(Tmf8829FrameInfo Info, byte[] Raw)
```

A raw histogram frame (25 377 B); decoding the bins is left to the caller (ams histogram layout).

#### Tmf8829HistogramFrame.Data *(property)*

```csharp
public ReadOnlySpan<byte> Data
```

The bytes between header and footer.

### Tmf8829Frames

```csharp
public static class Tmf8829Frames
```

TMF8829 frame layout and pixel decoding (contract 14 §3). Mirrors `depz_sensor_sdk/tmf8829/frames.py`.

#### Tmf8829Frames.PreHeaderSize *(constant)*

```csharp
public const int PreHeaderSize = 5
```

#### Tmf8829Frames.HeaderSize *(constant)*

```csharp
public const int HeaderSize = 16
```

#### Tmf8829Frames.FooterSize *(constant)*

```csharp
public const int FooterSize = 12
```

#### Tmf8829Frames.FrameHead *(constant)*

```csharp
public const int FrameHead = PreHeaderSize + HeaderSize
```

21 B the bridge reads at 0xFA.

#### Tmf8829Frames.FrameEof *(constant)*

```csharp
public const ushort FrameEof = 0xE0F7
```

#### Tmf8829Frames.FidMask *(constant)*

```csharp
public const int FidMask = 0xF0
```

#### Tmf8829Frames.FidResults *(constant)*

```csharp
public const int FidResults = 0x10
```

#### Tmf8829Frames.FidHistograms *(constant)*

```csharp
public const int FidHistograms = 0x20
```

#### Tmf8829Frames.FidRefSpad *(constant)*

```csharp
public const int FidRefSpad = 0x30
```

#### Tmf8829Frames.LayoutPeaks *(constant)*

```csharp
public const int LayoutPeaks = 0x07
```

#### Tmf8829Frames.LayoutSignal *(constant)*

```csharp
public const int LayoutSignal = 0x08
```

#### Tmf8829Frames.LayoutNoise *(constant)*

```csharp
public const int LayoutNoise = 0x10
```

#### Tmf8829Frames.LayoutXtalk *(constant)*

```csharp
public const int LayoutXtalk = 0x20
```

#### Tmf8829Frames.LayoutSubframe *(constant)*

```csharp
public const int LayoutSubframe = 0x40
```

32×32/48×32: set on the second (odd-row) half-frame.

#### Tmf8829Frames.FrameValid *(constant)*

```csharp
public const int FrameValid = 0x01
```

#### Tmf8829Frames.FrameAborted *(constant)*

```csharp
public const int FrameAborted = 0xC0
```

#### Tmf8829Frames.IReadOnlyList

```csharp
public static readonly IReadOnlyList<(int Bit, string Name)> FrameWarnings = new[]
```

frame_status warning bits, in report order.

#### Tmf8829Frames.FooterDistInMm *(constant)*

```csharp
public const int FooterDistInMm = 0x01
```

footer.reserved b0: distances in mm, else 0.25 mm.

#### Tmf8829Frames.FifoTxUnderrun *(constant)*

```csharp
public const int FifoTxUnderrun = 0x10
```

#### Tmf8829Frames.FifoRxOverrun *(constant)*

```csharp
public const int FifoRxOverrun = 0x20
```

#### Tmf8829Frames.int

```csharp
public static readonly IReadOnlyDictionary<int, (int Rows, int Cols)> FpGrid = new Dictionary<int, (int, int)>
```

fp_mode → (rows, cols) of the full focal plane.

#### Tmf8829Frames.FpNames *(field)*

```csharp
public static readonly IReadOnlyDictionary<int, string> FpNames = …
```

#### Tmf8829Frames.FrameSize

```csharp
public static int FrameSize(ReadOnlySpan<byte> head)
```

Full frame size (pre-header included) from its first 21 bytes: 5 + 4 + payload.

#### Tmf8829Frames.ParseFrame

```csharp
public static Tmf8829FrameInfo ParseFrame(ReadOnlySpan<byte> raw)
```

Parse pre-header, header and footer.

#### Tmf8829Frames.static

```csharp
public static (int Rows, int Cols) FrameRows(int fpMode)
```

(rows, cols) one results frame carries: half the rows for 32×32/48×32.

#### Tmf8829Frames.PixelSize

```csharp
public static int PixelSize(int layout)
```

Bytes per results pixel for a layout byte.

#### Tmf8829Frames.ParseResults

```csharp
public static Tmf8829PixelResults ParseResults(ReadOnlySpan<byte> raw, Tmf8829FrameInfo? info = null)
```

Pixel results of one results frame, row-major: `[noise u16]`, `[xtalk u16]`, then peaks × (distance u16, snr u8 [, signal u16]).

#### Tmf8829Frames.MergeHalves

```csharp
public static Tmf8829PixelResults MergeHalves(Tmf8829PixelResults first, Tmf8829PixelResults second)
```

32×32/48×32: the first half-frame holds the even rows, the second the odd ones.

### Tmf8829Measurement

```csharp
public sealed record Tmf8829Measurement(long TimestampUs, int Flags, int FpMode, uint FNumber, Tmf8829PixelResults Results, double[]? DistanceMm, IReadOnlyList<Tmf8829FrameInfo> Frames, IReadOnlyList<Tmf8829FrameTime> Times, IReadOnlyList<Tmf8829HistogramFrame> Histograms, IReadOnlyList<byte[]> RefFrames, IReadOnlyList<string> Problems)
```

One measurement (contract 14 §6): its results frame (8×8, 16×16) or both half-frames (32×32, 48×32) merged, with the histogram / ref-SPAD frames before it. `DistanceMm` is flat `[peaks][rows][cols]`, clock-corrected, in mm (null when the result format has no peaks); `Results` keeps the raw sensor integers. `Problems` lists what went wrong since the previous measurement.

#### Tmf8829Measurement.public

```csharp
public (int Rows, int Cols) Shape
```

(rows, cols) of the full focal plane.

#### Tmf8829Measurement.Peaks *(property)*

```csharp
public int Peaks
```

#### Tmf8829Measurement.TemperatureC *(property)*

```csharp
public int TemperatureC
```

Sensor die temperature (header byte 0), °C.

#### Tmf8829Measurement.ToMm

```csharp
public static double[]? ToMm(Tmf8829PixelResults results, double ratio, bool inMm)
```

Raw distances → mm, clock-corrected (× ratio; ÷ 4 for 0.25 mm units).

### Tmf8829MeasurementBuilder

```csharp
public sealed class Tmf8829MeasurementBuilder
```

Frames in stream order → measurements (contract 14 §6). Histogram frames between two results frames belong to the second; a ref-SPAD frame comes right before its results frame (each half-frame has its own). 32×32/48×32: the first half (layout 0x40 clear) holds the even rows; a second half without a first is dropped, two first halves in a row keep the newer one (both logged as problems). Call `Reset` at the start of every stream: the clock model and T1 are per stream. Mirrors `depz_sensor_sdk/tmf8829/builder.py`.

#### Tmf8829MeasurementBuilder.Tmf8829MeasurementBuilder *(constructor)*

```csharp
public Tmf8829MeasurementBuilder()
```

#### Tmf8829MeasurementBuilder.Clock *(property)*

```csharp
public Tmf8829SensorClock Clock
```

#### Tmf8829MeasurementBuilder.Timer *(property)*

```csharp
public Tmf8829CaptureTimer Timer
```

#### Tmf8829MeasurementBuilder.ParseErrors *(property)*

```csharp
public int ParseErrors
```

Frames that could not be parsed (also listed as problems).

#### Tmf8829MeasurementBuilder.Reset

```csharp
public void Reset()
```

#### Tmf8829MeasurementBuilder.Add

```csharp
public Tmf8829Measurement? Add(byte[] raw, long timestampUs, long edgeUs = 0)
```

One frame with the MCU time of its header read and its INT edge (0 = none) → the measurement it completes, or null.

### Tmf8829TimeFlags

```csharp
public static class Tmf8829TimeFlags
```

`Flags` bits (contract 14 §5).

#### Tmf8829TimeFlags.Nominal *(constant)*

```csharp
public const int Nominal = 0x01
```

Clock ratio not measured yet (nominal 8 µs/tick used).

#### Tmf8829TimeFlags.ClockReset *(constant)*

```csharp
public const int ClockReset = 0x02
```

Clock model restarted at this frame (SYSTICK jump).

#### Tmf8829TimeFlags.EdgeLate *(constant)*

```csharp
public const int EdgeLate = 0x04
```

INT edge > 2 ms later after t1 than usual in this stream.

#### Tmf8829TimeFlags.Stalled *(constant)*

```csharp
public const int Stalled = 0x08
```

Sub-integrations not back to back (histogram FIFO wait).

#### Tmf8829TimeFlags.T1Unsure *(constant)*

```csharp
public const int T1Unsure = 0x10
```

Histograms + 32×32/48×32: T1 may be too long, centre late.

#### Tmf8829TimeFlags.FlagNames

```csharp
public static List<string> FlagNames(int flags)
```

Names of the bits set in `flags`.

### Tmf8829SensorClock

```csharp
public sealed class Tmf8829SensorClock
```

Sensor SYSTICK (125 kHz) → MCU µs, fed one clock pair per frame (the MCU time of the header read and the pre-header SYSTICK sampled by it). The ratio is taken over a sliding 1..2 s window, first after 200 ms; it is also the distance correction (ams tmf8829ClockCorrection). Integer arithmetic exactly as contract 14 §5 — `tmf8829_frame_time.json` pins it bit for bit: u32 tick differences, signed 32-bit tick deltas, 64-bit µs, floor division, arithmetic shift.

#### Tmf8829SensorClock.NominalQ16 *(constant)*

```csharp
public const uint NominalQ16 = 8u << 16
```

SYSTICK 125 kHz = 8 MCU µs per tick, Q16.

#### Tmf8829SensorClock.WindowMinUs *(constant)*

```csharp
public const long WindowMinUs = 200_000
```

First ratio estimate after this span.

#### Tmf8829SensorClock.WindowSlideUs *(constant)*

```csharp
public const long WindowSlideUs = 1_000_000
```

Then a 1..2 s sliding window.

#### Tmf8829SensorClock.Tmf8829SensorClock *(constructor)*

```csharp
public Tmf8829SensorClock()
```

#### Tmf8829SensorClock.RatioQ16 *(property)*

```csharp
public uint RatioQ16
```

MCU/sensor clock ratio, Q16 × 8 (`NominalQ16` until measured).

#### Tmf8829SensorClock.Nominal *(property)*

```csharp
public bool Nominal
```

True while the ratio is not measured yet.

#### Tmf8829SensorClock.Ratio *(property)*

```csharp
public double Ratio
```

MCU/sensor clock ratio (≈ 1.015): distances are scaled by it.

#### Tmf8829SensorClock.Reset

```csharp
public void Reset()
```

Restart the model — at every stream start.

#### Tmf8829SensorClock.Add

```csharp
public int Add(long us, uint tick)
```

Add a pair → `ClockReset` if it does not fit the model (which then restarts from it), else 0.

#### Tmf8829SensorClock.ToUs

```csharp
public long ToUs(long pairUs, uint pairTick, uint tick)
```

SYSTICK `tick` → MCU µs through the pair (`pairUs`, `pairTick`).

### Tmf8829FrameTime

```csharp
public sealed record Tmf8829FrameTime(long TimestampUs, int Flags, uint T1LenUs, long T0Us, long T1Us, long EdgeUs, long PairUs, uint RatioQ16)
```

Capture time of one results frame, MCU µs.

### Tmf8829CaptureTimer

```csharp
public sealed class Tmf8829CaptureTimer
```

When a results frame was captured. A results frame is N equal sub-integrations back to back; the footer has the SYSTICK of the start of the first (t0) and of the last (t1). The window is [t0, t1 + T1], T1 the stream's shortest (t1 − t0)/(N − 1); its centre is the frame time.

#### Tmf8829CaptureTimer.EdgeLateUs *(constant)*

```csharp
public const long EdgeLateUs = 2000
```

EDGE_LATE threshold over the stream's smallest edge lag.

#### Tmf8829CaptureTimer.FpSubints *(field)*

```csharp
public static readonly IReadOnlyDictionary<int, int> FpSubints = …
```

Sub-integrations per results frame by fp_mode.

#### Tmf8829CaptureTimer.Tmf8829CaptureTimer *(constructor)*

```csharp
public Tmf8829CaptureTimer(Tmf8829SensorClock clock)
```

#### Tmf8829CaptureTimer.Clock *(property)*

```csharp
public Tmf8829SensorClock Clock
```

#### Tmf8829CaptureTimer.Reset

```csharp
public void Reset()
```

Forget the per-stream state (lag, T1, histogram mode).

#### Tmf8829CaptureTimer.Histograms

```csharp
public void Histograms()
```

A histogram frame was seen in the stream.

#### Tmf8829CaptureTimer.Results

```csharp
public Tmf8829FrameTime Results(int fpMode, uint systick, uint t0Tick, uint t1Tick, long pairUs, long edgeUs, int flags)
```

Capture time of a results frame. Call `Add` with the same pair first and pass its flags in `flags`.

#### Tmf8829CaptureTimer.ResultsFrame

```csharp
public Tmf8829FrameTime ResultsFrame(Tmf8829FrameInfo f, long pairUs, long edgeUs, int flags)
```

`Results` with the times taken from a parsed frame.

### Tmf8829Config

```csharp
public static class Tmf8829Config
```

TMF8829 presets and config-page fields (contracts/14_SENSOR_TMF8829.md §4). Mirrors `depz_sensor_sdk/tmf8829/config.py`. A preset load is not a clean slate: hosts write `period_ms`, `kilo_iterations`, `nr_peaks`, the three strengths, `dump_histograms` and `ref_spad_frame` on every configure.

#### Tmf8829Config.RegCfg *(constant)*

```csharp
public const int RegCfg = 0x22
```

Config page register 0x22..0xDF.

#### Tmf8829Config.CfgSize *(constant)*

```csharp
public const int CfgSize = 0xDF - RegCfg + 1
```

190 B.

#### Tmf8829Config.DefaultPeriodMs *(constant)*

```csharp
public const int DefaultPeriodMs = 33
```

The sensor's own period after init().

#### Tmf8829Config.Presets *(field)*

```csharp
public static readonly IReadOnlyDictionary<string, byte> Presets = …
```

Preset name → load command (each loads its config page).

#### Tmf8829Config.PresetKiloIterations *(field)*

```csharp
public static readonly IReadOnlyDictionary<string, int> PresetKiloIterations = …
```

The preset's own kilo_iterations (a preset rescales a stale value, it does not reset it).

#### Tmf8829Config.string

```csharp
public static readonly IReadOnlyDictionary<string, (int Reg, int Shift, int Width)> CfgFields = new Dictionary<string, (int, int, int)>
```

Field → (register, bit shift, width in bits); width 16 = little-endian register pair.

#### Tmf8829Config.CfgGet

```csharp
public static int CfgGet(ReadOnlySpan<byte> page, string name)
```

Read a named field from a config page (page byte 0 = register 0x22).

#### Tmf8829Config.CfgSet

```csharp
public static void CfgSet(Span<byte> page, string name, int value)
```

Write a named field into a config page in place.

#### Tmf8829Config.ConfigFields

```csharp
public static Dictionary<string, int> ConfigFields(ReadOnlySpan<byte> page)
```

Every named field of a config page.

### Tmf8829Firmware

```csharp
public static class Tmf8829Firmware
```

The TMF8829 sensor firmware the host downloads through the sensor bootloader on every init (contract 14 §4): ams `tmf8829_application.hex`, application 2.200.0, one contiguous segment, converted to a flat image at build time. MIT-licensed — the license text ships with the package (`LICENSES-MIT.TXT`, also `License`).

#### Tmf8829Firmware.LoadAddr *(constant)*

```csharp
public const uint LoadAddr = 0x00010000
```

Load address of the image (BL W_FIFO_BOTH `addr`).

#### Tmf8829Firmware.Size *(constant)*

```csharp
public const int Size = 14444
```

Image size in bytes (already a multiple of 4: 3611 words).

#### Tmf8829Firmware.Image

```csharp
public static byte[] Image()
```

A copy of the firmware image (14 444 B).

#### Tmf8829Firmware.ImageSpan *(property)*

```csharp
public static ReadOnlySpan<byte> ImageSpan
```

The image without copying.

#### Tmf8829Firmware.License

```csharp
public static string License()
```

The ams MIT license text of the image.

## Firmware update

### FwDepz

```csharp
public static class FwDepz
```

`.fwdepz` bootloader container parse/validate (contracts/06 §2).

#### FwDepz.FwDepzMagic *(field)*

```csharp
public static readonly byte[] FwDepzMagic = Encoding.ASCII.GetBytes("FWDEPZ00")
```

#### FwDepz.FwDepzHeaderSize *(constant)*

```csharp
public const int FwDepzHeaderSize = 64
```

#### FwDepz.FwDepzException

```csharp
public sealed class FwDepzException : Exception
```

Parse/validate failure. `Code` matches the vector `error` strings.

##### FwDepz.FwDepzException.Code *(property)*

```csharp
public string Code
```

##### FwDepz.FwDepzException.FwDepzException *(constructor)*

```csharp
public FwDepzException(string code, string message) : base(message)
```

#### FwDepz.FwDepzImage

```csharp
public sealed record FwDepzImage(uint LoadAddr, uint FwSize, uint FwCrc32, int CurSec, int TotSec, byte[] Payload)
```

Parsed and validated `.fwdepz` firmware container.

##### FwDepz.FwDepzImage.PayloadCrcOk *(property)*

```csharp
public bool PayloadCrcOk
```

##### FwDepz.FwDepzImage.Parse

```csharp
public static FwDepzImage Parse(byte[] blob)
```

Parse and validate. Validation order (contract 06 §2): length, magic, then CRC-16/CCITT-FALSE over bytes [0..61] vs the u16 LE at offset 62, then fw_size == payload length.

## Datasets (record & replay)

### TimeSync

```csharp
public sealed record TimeSync(long OffsetUs, long RttUs)
```

Per-device time-sync (contract 02 §5): device_clock − host_clock, and RTT.

### DatasetDevice

```csharp
public sealed record DatasetDevice(string Id, string? Serial, string? SoftwareName, string? SensorType, TimeSync? TimeSync)
```

Header metadata for one device in a dataset (contract 09).

### DatasetRecord

```csharp
public sealed record DatasetRecord(string DeviceId, long THostUs, string Kind, IReadOnlyDictionary<string, object?> Value)
```

One decoded record on the shared host timeline. `Value` is the kind-specific payload object (players use the fields present for the kind), as `Parse` returns it: nested values are `long`, `double`, `string`, `bool`, `null`, lists and dictionaries.

### DatasetReader

```csharp
public sealed class DatasetReader
```

Reader for the `.depzdata` decoded, multi-device, time-synced dataset format (contracts/09_DATASET_FORMAT.md). Plain or gzip (`.depzdata.gz`). Records are exposed merged by host time `t`; unknown kinds are kept.

#### DatasetReader.Devices *(property)*

```csharp
public IReadOnlyDictionary<string, DatasetDevice> Devices
```

Device metadata from the header, keyed by device id (d0, d1, …).

#### DatasetReader.Records *(property)*

```csharp
public IReadOnlyList<DatasetRecord> Records
```

All records, stably merged by ascending host time.

#### DatasetReader.Note *(property)*

```csharp
public string? Note
```

#### DatasetReader.Header *(property)*

```csharp
public IReadOnlyDictionary<string, object?> Header
```

The header line as parsed (unknown fields included).

#### DatasetReader.DurationUs *(property)*

```csharp
public long DurationUs
```

Host-time span from the first record to the last (0 with fewer than two records).

#### DatasetReader.DatasetReader *(constructor)*

```csharp
public DatasetReader(string path) : this(ReadAllLines(path))
```

#### DatasetReader.Play

```csharp
public void Play(Action<DatasetRecord> callback, double speed = 1.0, long? startTUs = null, CancellationToken cancel = default)
```

Deliver the records paced by their timestamps (`speed` 2 = twice as fast, 0 = as fast as possible), blocking until the end or `cancel`. Records before `startTUs` are skipped. For pause / seek / speed changes use `DatasetPlayer`.

### DatasetWriter

```csharp
public sealed class DatasetWriter : IDisposable
```

Low-level thread-safe `.depzdata` record writer (gzip when the path ends in `.gz`). Prefer `SessionRecorder`.

#### DatasetWriter.Schema *(constant)*

```csharp
public const string Schema = "depz.dataset/2"
```

The schema id this SDK writes.

#### DatasetWriter.DatasetWriter *(constructor)*

```csharp
public DatasetWriter(string path, IReadOnlyDictionary<string, IReadOnlyDictionary<string, object?>> devices, string note = "")
```

#### DatasetWriter.RecordsWritten *(property)*

```csharp
public long RecordsWritten
```

#### DatasetWriter.Write

```csharp
public void Write(string deviceId, long tHostUs, string kind, IReadOnlyDictionary<string, object?> value)
```

One record: device id, host µs, kind, value.

#### DatasetWriter.Close

```csharp
public void Close()
```

#### DatasetWriter.Dispose

```csharp
public void Dispose()
```

### SessionRecorder

```csharp
public sealed class SessionRecorder : IDisposable
```

Record decoded data from several devices onto one host timeline (contract 09). `Add` each open device (it runs `SyncTime`), then `Start`: the recorder hooks every device's result stream and writes a record per result at device time − offset. It never reconfigures a device — start and stop streaming yourself. `Stop` (or disposing) unhooks and closes the file.

#### SessionRecorder.SessionRecorder *(constructor)*

```csharp
public SessionRecorder(string path, string note = "", bool vl53l8Layers = false, double? temperatureHz = 1.0)
```

#### SessionRecorder.Add

```csharp
public string Add(DepzDevice device, string? deviceId = null, int syncSamples = 5)
```

Register a device before recording starts; runs `SyncTime` so its timestamps land on the shared host timeline. Returns its device id (`d0`, `d1`, …).

#### SessionRecorder.RecordsWritten *(property)*

```csharp
public long RecordsWritten
```

#### SessionRecorder.Start

```csharp
public void Start()
```

Open the file and hook every device's stream.

#### SessionRecorder.Stop

```csharp
public void Stop()
```

Unhook every device and close the file.

#### SessionRecorder.Dispose

```csharp
public void Dispose()
```

### PlayerState

```csharp
public enum PlayerState
{
    Idle,
    Playing,
    Paused,
    Done,
}
```

`DatasetPlayer` state (contract 09 §Player).

### DatasetPlayer

```csharp
public sealed class DatasetPlayer : IDisposable
```

Timeline player over a dataset: paced delivery with play / pause / seek / speed — the engine behind a viewer's playback mode (contract 09 §Player; reference: the Python `DatasetPlayer`).

Timing: a media clock anchored at play / resume / seek to (now, t of the last delivered record — of the first when none was). Record i is due at `anchor_wall + (t_i − anchor_t) / speed`: the first record goes out at once, each later one after its gap to the previous one. Pacing is against the clock, so a slow consumer gets overdue records back to back and playback does not drift. Speed 0 delivers as fast as possible.

Threads: records and the `Done` state are delivered on a player thread (started by the first `Play`); a state change a call makes is reported on the calling thread before it returns. Callbacks run under the player's lock — they see a consistent player and may call its methods, and once `Pause` returns no record callback is running or will start. They must not block. With `threaded: false` no thread starts and the owner calls `Tick` (a GUI timer, or a test with a fake clock).

#### DatasetPlayer.DatasetPlayer *(constructor)*

```csharp
public DatasetPlayer(DatasetReader reader, double speed = 1.0, Func<double>? clock = null, bool threaded = true)
```

#### DatasetPlayer.DatasetPlayer *(constructor)*

```csharp
public DatasetPlayer(string path, double speed = 1.0, Func<double>? clock = null, bool threaded = true) : this(new DatasetReader(path), speed, clock, threaded)
```

A player over the `.depzdata` file at `path`.

#### DatasetPlayer.Reader *(property)*

```csharp
public DatasetReader Reader
```

#### DatasetPlayer.State *(property)*

```csharp
public PlayerState State
```

#### DatasetPlayer.DeliveryError *(field)*

```csharp
public event Action<Exception>? DeliveryError
```

A callback on the player thread threw: playback paused there (the record counts as delivered).

#### DatasetPlayer.DurationUs *(property)*

```csharp
public long DurationUs
```

Span of the timeline: the last record's t minus the first's.

#### DatasetPlayer.PositionUs *(property)*

```csharp
public long PositionUs
```

µs from the first record to the last delivered one (after a seek: to the record just before the target); 0 before anything was delivered.

#### DatasetPlayer.Speed *(property)*

```csharp
public double Speed
```

Playback speed (finite, ≥ 0; 0 = as fast as possible). A change while playing applies at once: the rest of the current wait is rescaled.

#### DatasetPlayer.OnRecord

```csharp
public IDisposable OnRecord(Action<DatasetRecord> callback)
```

Subscribe to delivered records; dispose the result to unsubscribe (idempotent).

#### DatasetPlayer.OnState

```csharp
public IDisposable OnState(Action<PlayerState> callback)
```

Subscribe to state changes, called with the new state on every transition.

#### DatasetPlayer.Play

```csharp
public void Play()
```

Start, or resume after `Pause`; no-op while playing. From `Done` (or a position past the end) playback restarts at the first record. An empty dataset goes to done at once.

#### DatasetPlayer.Pause

```csharp
public void Pause()
```

Stop delivering; `Play` resumes from the next record, which then waits its full gap again. No-op unless playing.

#### DatasetPlayer.SeekUs

```csharp
public void SeekUs(double offsetUs)
```

Move to `offsetUs` µs from the first record: the next record delivered is the first with t ≥ first t + offset (negative: the start). Records in between are skipped. The state does not change, except that a seek past the end while playing ends playback.

#### DatasetPlayer.Tick

```csharp
public double? Tick()
```

Deliver every record due now, on the calling thread. Returns the seconds until the next one is due, or null when not playing. For `threaded: false`; harmless with a player thread.

#### DatasetPlayer.Wait

```csharp
public bool Wait(TimeSpan? timeout = null)
```

Block until the player is not playing (done, or paused by someone else): true, or false on timeout. Needs the player thread.

#### DatasetPlayer.Close

```csharp
public void Close()
```

Stop playback and the player thread. Idempotent.

#### DatasetPlayer.Dispose

```csharp
public void Dispose()
```

### DatasetRecords

```csharp
public static class DatasetRecords
```

The contract 09 record value (`v`) of each kind, from the device classes' results — what `SessionRecorder` writes, public for writers of their own. Key for key the reference Python SDK's values.

#### DatasetRecords.Sr04

```csharp
public static Dictionary<string, object?> Sr04(Sr04Measurement m)
```

`sr04`: `{echo_us, source}`.

#### DatasetRecords.Vl53l4

```csharp
public static Dictionary<string, object?> Vl53l4(Vl53l4Measurement m)
```

`vl53l4`: the result fields, distance raw whatever the status.

#### DatasetRecords.Vl53l8

```csharp
public static Dictionary<string, object?> Vl53l8(Vl53l8Frame f, bool layers = false)
```

`vl53l8`: per-zone arrays cut to the resolution; `layers` adds signal and ambient (kcps/SPAD), sigma (mm) and reflectance (%) as reals; `motion` when the indicator is configured.

#### DatasetRecords.Vl53l8Cnh

```csharp
public static Dictionary<string, object?> Vl53l8Cnh(byte[] cnhRaw, Vl53l8CnhSetup setup)
```

`vl53l8_cnh`: the raw CNH block of a frame with the armed setup's decode parameters.

#### DatasetRecords.Vl53lx

```csharp
public static Dictionary<string, object?> Vl53lx(Vl53lxMeasurement m, string? product, string? driver)
```

`vl53lx`: the family-wide result, plus `targets` when the driver reported several.

#### DatasetRecords.Bno055

```csharp
public static Dictionary<string, object?> Bno055(Bno055Sample s)
```

`bno055`: `unit_sel` and every channel the streamed block covered, scaled.

#### DatasetRecords.Tmf8829

```csharp
public static Dictionary<string, object?> Tmf8829(Tmf8829Measurement m)
```

`tmf8829`: one measurement — distances in mm rounded to 0.1, the raw per-zone arrays.

#### DatasetRecords.Bno086

```csharp
public static Dictionary<string, object?> Bno086(BnoReport r)
```

`bno086`: the report in snake_case — type and sensor id, then only the fields the report carries (scaled), the integer counts of the raw ADC streams, and the device timestamp.

#### DatasetRecords.Round1

```csharp
public static double Round1(double x)
```

Python's `round(x, 1)`: the nearest tenth of the exact binary value, ties to even, as the double closest to it.

### JsonText

```csharp
public static class JsonText
```

Minimal JSON reader/writer for the JSON Lines formats (contracts 08, 09). The SDK targets netstandard2.1 for Unity, which has no System.Text.Json, so values are plain objects: `null`, `bool`, `string`, `long` (integers that fit), `ulong` (larger positive integers), `double`, `List{T}` of `object?` and `Dictionary{TKey,TValue}` of `string` → `object?` (key order kept as in the text).

#### JsonText.Parse

```csharp
public static object? Parse(string text)
```

Parse one JSON value; trailing whitespace is allowed, anything else is not.

#### JsonText.Serialize

```csharp
public static string Serialize(object? value)
```

Compact JSON (no spaces) of a value tree built from the types `Parse` returns, plus any integer type, `float`, `byte[]` (lowercase hex string) and any `IDictionary` / `IEnumerable`.

## Transport

### Framing

```csharp
public static class Framing
```

Packet framing (contracts/01_TRANSPORT_FRAMING.md). Byte-exact with the firmware and the reference Python SDK, including the empty-payload-never-carries-CRC rule (contracts/ERRATA.md E6).

#### Framing.Magic *(field)*

```csharp
public static readonly byte[] Magic = { 0xA5, 0xC3 }
```

#### Framing.HeaderSize *(constant)*

```csharp
public const int HeaderSize = 7
```

#### Framing.MaxPayload *(constant)*

```csharp
public const int MaxPayload = 0x3FFF
```

#### Framing.PayloadCrcBytes

```csharp
public static byte[] PayloadCrcBytes(CrcType crcType, ReadOnlySpan<byte> payload)
```

CRC trailer for a payload; empty payloads never carry CRC bytes (E6).

#### Framing.BuildPacket

```csharp
public static byte[] BuildPacket(int cmd, ReadOnlySpan<byte> payload = default, long seq = 0, CrcType crcType = CrcType.None)
```

Frame one packet. The `crcType` bits are set in the header even for an empty payload (matching device TX), but CRC bytes are only appended for non-empty payloads (E6). `seq` is a free-running u32 counter, taken modulo 256 on the wire.

### PacketParser

```csharp
public sealed class PacketParser
```

Incremental frame parser. Feed arbitrary byte chunks; get events.

Event order is invariant to chunking (contract 01 §5) except `Trash` event boundaries — concatenate Trash data when comparing streams. Byte-exact with the reference Python `PacketParser`, including the empty-payload-no-CRC rule (ERRATA E6) and single-byte resync on a corrupt header.

#### PacketParser.Packets *(property)*

```csharp
public int Packets
```

#### PacketParser.CrcErrors *(property)*

```csharp
public int CrcErrors
```

#### PacketParser.HeaderErrors *(property)*

```csharp
public int HeaderErrors
```

#### PacketParser.TrashBytes *(property)*

```csharp
public int TrashBytes
```

#### PacketParser.Residue

```csharp
public byte[] Residue()
```

Current unconsumed bytes (residue).

#### PacketParser.Feed

```csharp
public IReadOnlyList<ParserEvent> Feed(ReadOnlySpan<byte> data)
```

### ParserEvent

```csharp
public abstract record ParserEvent
```

Base type for events produced by `PacketParser`.

### Packet

```csharp
public sealed record Packet(int Cmd, int Seq, byte[] Payload) : ParserEvent
```

A fully decoded frame with a valid header (and payload CRC, if any).

### Trash

```csharp
public sealed record Trash(byte[] Data) : ParserEvent
```

Bytes discarded while hunting for a valid frame. Boundaries between consecutive `Trash` events depend on read chunking; only the concatenated byte stream is deterministic.

### CrcError

```csharp
public sealed record CrcError(int Cmd, int Seq) : ParserEvent
```

A frame with a valid header whose payload CRC failed; dropped.

### Crc

```csharp
public static class Crc
```

CRC algorithms of the DEPZ transport (contracts/01_TRANSPORT_FRAMING.md §3).

All wire CRCs are reflected table implementations, byte-exact with the firmware and the reference Python SDK. CRC-8 init is 0x00 for every device (contracts/ERRATA.md E1). `Crc16CcittFalse` is the `.fwdepz`-header-only CRC (contract 06), never on the wire.

#### Crc.Crc8Maxim

```csharp
public static byte Crc8Maxim(ReadOnlySpan<byte> data)
```

CRC-8/MAXIM: poly 0x31 reflected, init 0x00, xorout 0x00.

#### Crc.Crc16Modbus

```csharp
public static ushort Crc16Modbus(ReadOnlySpan<byte> data)
```

CRC-16/MODBUS: poly 0x8005 reflected, init 0xFFFF, xorout 0x0000.

#### Crc.Crc32IsoHdlc

```csharp
public static uint Crc32IsoHdlc(ReadOnlySpan<byte> data)
```

CRC-32/ISO-HDLC: poly 0x04C11DB7 reflected, init/xorout 0xFFFFFFFF.

#### Crc.Crc16CcittFalse

```csharp
public static ushort Crc16CcittFalse(ReadOnlySpan<byte> data)
```

CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, not reflected. Used only for the `.fwdepz` file header (contract 06), never on the wire.

### CrcType

```csharp
public enum CrcType
{
    None = 0,
    Crc8 = 1,
    Crc16 = 2,
    Crc32 = 3,
}
```

Payload CRC type carried in the two high bits of the data-size field.

### CrcTypeExtensions

```csharp
public static class CrcTypeExtensions
```

#### CrcTypeExtensions.Size

```csharp
public static int Size(this CrcType crcType)
```

Size in bytes of the CRC trailer for a non-empty payload.

## Protocol codecs

### Cmd

```csharp
public enum Cmd
{
    Bootloader = 0x01,
    DeviceReset = 0x02,
    GetDeviceName = 0x03,
    GetNameActiveSoftware = 0x04,
    GetSerial = 0x05,
    SyncTime = 0x06,
    GetMcuTemperature = 0x07,
    GetPayloadCrcType = 0x08,
    SetPayloadCrcType = 0x09,
    UsbTestTxStart = 0x1C,
    UsbTestTxStop = 0x1D,
    UsbTestRxData = 0x1E,
    UsbTestStats = 0x1F,
    GetSyncPinConfig = 0x30,
    SetSyncPinConfig = 0x31,
}
```

Common command opcodes (contracts/02_COMMON_COMMANDS.md).

### Rpt

```csharp
public enum Rpt
{
    Status = 0x80,
    Text = 0x81,
    SyncTime = 0x82,
    Temperature = 0x83,
    SequenceError = 0x84,
    PayloadCrcType = 0x87,
    UsbTestData = 0x88,
    UsbTestStats = 0x8F,
    SyncPinConfig = 0x90,
}
```

Common report opcodes.

### Status

```csharp
public enum Status
{
    Ok = 0x00,
    Error = 0x01,
    ErrInvalidCmd = 0x02,
    ErrPayloadFormat = 0x03,
    ErrInvalidParam = 0x04,
    ErrPayloadCrc = 0x05,
    ErrBusy = 0x06,
    ErrCmdNotSupported = 0x07,
    ErrNotInitialized = 0x08,
    ErrHardwareFault = 0x09,
}
```

### SyncPinMode

```csharp
public enum SyncPinMode
{
    Disable = 0x00,
    In = 0x01,
    OutStart = 0x02,
    OutEnd = 0x03,
    OutBoth = 0x04,
}
```

### SyncPinPolarity

```csharp
public enum SyncPinPolarity
{
    IdleLow = 0x00,
    IdleHigh = 0x01,
}
```

### Reports

```csharp
public static class Reports
```

Echoed-cmd byte value for an unsolicited report.

#### Reports.Unsolicited *(constant)*

```csharp
public const int Unsolicited = 0x00
```

### StatusReport

```csharp
public sealed record StatusReport(int Cmd, int Status)
```

Status report: echoed request opcode (0x00 = unsolicited) + status code.

#### StatusReport.Unpack

```csharp
public static StatusReport Unpack(ReadOnlySpan<byte> payload)
```

### TextReport

```csharp
public sealed record TextReport(int Cmd, string Text)
```

Text report: echoed opcode + ASCII string (trailing 0x00/0xFF stripped).

#### TextReport.Unpack

```csharp
public static TextReport Unpack(ReadOnlySpan<byte> payload)
```

### SyncTimeReport

```csharp
public sealed record SyncTimeReport(ulong PcTimestampUs, ulong McuRxUs, ulong McuTxUs)
```

Sync-time report: T1 (echoed), T2 (mcu rx), T3 (mcu tx), all µs.

#### SyncTimeReport.Unpack

```csharp
public static SyncTimeReport Unpack(ReadOnlySpan<byte> payload)
```

### TemperatureReport

```csharp
public sealed record TemperatureReport(ulong TimestampUs, short RawDecidegrees)
```

Temperature report: timestamp µs + raw int16 in units of 0.1 °C.

#### TemperatureReport.Celsius *(property)*

```csharp
public double Celsius
```

#### TemperatureReport.Unpack

```csharp
public static TemperatureReport Unpack(ReadOnlySpan<byte> payload)
```

### SequenceErrorReport

```csharp
public sealed record SequenceErrorReport(int ExpectedSeq, int ReceivedSeq)
```

Sequence-error report: expected vs received seq bytes.

#### SequenceErrorReport.Unpack

```csharp
public static SequenceErrorReport Unpack(ReadOnlySpan<byte> payload)
```

### SyncPinConfig

```csharp
public sealed record SyncPinConfig(int Pin, SyncPinMode Mode, SyncPinPolarity Polarity)
```

Sync-pin configuration (pin 1..5, mode, polarity).

#### SyncPinConfig.PinMin *(constant)*

```csharp
public const int PinMin = 1, PinMax = 5
```

First / last logical AUX sync pin (contract 02 §6).

#### SyncPinConfig.Pack

```csharp
public byte[] Pack()
```

SET_SYNC_PIN_CONFIG payload. Contract 02 §6: pin 1..5, mode a `SyncPinMode` (0..4), polarity a `SyncPinPolarity` (0..1).

#### SyncPinConfig.Unpack

```csharp
public static SyncPinConfig Unpack(ReadOnlySpan<byte> payload)
```

### Common

```csharp
public static class Common
```

Common command/report payload codecs (contract 02).

#### Common.PackSyncTime

```csharp
public static byte[] PackSyncTime(ulong pcTimestampUs)
```

Encode a SYNC_TIME request payload (u64 LE µs).

#### Common.static

```csharp
public static (long OffsetUs, long RttUs) SyncTimeOffsetRtt(long t1, long t2, long t3, long t4)
```

NTP-style clock math, all µs (contract 02 §5). Returns (offsetUs, rttUs) where offset = device_clock - host_clock, computed with truncation toward zero on the sum (C# long division already truncates toward zero).

#### Common.StripDeviceString

```csharp
public static string StripDeviceString(ReadOnlySpan<byte> raw)
```

Decode an ASCII device string, dropping trailing NUL/0xFF filler. Every byte ≥ 0x80 becomes U+FFFD (contract 02 §4.1: never '?', never UTF-8 or Latin-1); an embedded 0x00 before the filler is kept as a character.

### SensorType

```csharp
public enum SensorType
{
    Sr04,
    Vl53l8,
    Vl53l4,
    Vl53l7,
    Vl53lx,
    Bno086,
    Bno055,
    Tmf8829,
    Unknown,
}
```

### Identity

```csharp
public sealed record Identity(string Mode, SensorType? SensorType, string SoftwareName, string Version)
```

Classified GET_NAME_ACTIVE_SOFTWARE string (contracts/02 §4). `SensorType` is null in bootloader mode.

### IdentityParser

```csharp
public static class IdentityParser
```

#### IdentityParser.ParseSoftwareName

```csharp
public static Identity ParseSoftwareName(string name)
```

Classify a GET_NAME_ACTIVE_SOFTWARE string. The string must already be stripped of trailing NUL/0xFF (`StripDeviceString`).

### Wire

```csharp
public static class Wire
```

Wire-field checks shared by the codecs (contract 07 §3). Encoding: every argument is checked against its wire width before a byte is written — never truncated, wrapped or masked (`ArgumentOutOfRangeException`, the SDK's `invalid_argument`). Decoding: a payload shorter than the fields the contract defines is rejected (`ArgumentException`); trailing bytes are ignored by the callers.

#### Wire.CheckUint

```csharp
public static long CheckUint(string name, long value, int bits)
```

`value` as an unsigned integer that fits `bits` (≤ 63) bits.

#### Wire.CheckInt

```csharp
public static int CheckInt(string name, long value, int bits)
```

`value` as a signed integer that fits `bits` (≤ 32) bits.

#### Wire.NeedBytes

```csharp
public static void NeedBytes(string what, ReadOnlySpan<byte> payload, int n)
```

Reject a device payload shorter than the `n` bytes its fields need.
