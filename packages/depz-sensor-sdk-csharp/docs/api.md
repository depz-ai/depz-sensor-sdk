# API reference

Auto-generated from the public C# surface under `src/Depz.Sensor/` by
`scripts/gen_api_md.py` — run it to regenerate (see below). Edit the
`///` XML-doc summaries in the source, not this file.

Each sensor also has a focused reference with just its own symbols:
[SR04](sr04/api.md) · [VL53L4CD](vl53l4cd/api.md) · [VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · [VL53L5CX](vl53l5cx/api.md) · [VL53L7CX](vl53l7cx/api.md) · [VL53L7CH](vl53l7ch/api.md) · [VL53L0X](vl53l0x/api.md) · [VL53L1CX](vl53l1cx/api.md) · [VL53L1CB](vl53l1cb/api.md) · [VL53L3CX](vl53l3cx/api.md) · [VL53L4CX](vl53l4cx/api.md) · [BNO086](bno086/api.md) · [BNO055](bno055/api.md).

Regenerate: `python3 scripts/gen_api_md.py` (from the package root).

## Contents

- **USB identity**: [`UsbIds`](#usbids)
- **SR04**: [`Sr04Cmd`](#sr04cmd), [`Sr04Rpt`](#sr04rpt), [`Sr04Data`](#sr04data), [`Sr04`](#sr04)
- **VL53L4CD (ToF)**: [`Vl53l4Cmd`](#vl53l4cmd), [`Vl53l4Rpt`](#vl53l4rpt), [`Vl53l4Xshut`](#vl53l4xshut), [`Vl53l4Wire`](#vl53l4wire), [`Vl53l4RegData`](#vl53l4regdata), [`Vl53l4Info`](#vl53l4info), [`Vl53l4StreamData`](#vl53l4streamdata), [`Vl53l4Exception`](#vl53l4exception), [`Vl53l4Results`](#vl53l4results), [`Vl53l4Uld`](#vl53l4uld)
- **VL53L8 (ToF)**: [`Vl53l8Variant`](#vl53l8variant), [`Vl53l8Frame`](#vl53l8frame), [`Vl53l8FrameDecoder`](#vl53l8framedecoder), [`Vl53l8Cmd`](#vl53l8cmd), [`Vl53l8Rpt`](#vl53l8rpt), [`Vl53l8Wire`](#vl53l8wire), [`FrameChunk`](#framechunk), [`FrameReassembler`](#framereassembler), [`Vl53l8Advanced`](#vl53l8advanced), [`MotionConfig`](#motionconfig), [`Vl53l8CnhConfig`](#vl53l8cnhconfig), [`Vl53l8CnhAggregate`](#vl53l8cnhaggregate), [`Vl53l8CnhResult`](#vl53l8cnhresult), [`Vl53l8Cnh`](#vl53l8cnh), [`Vl53l8Uld`](#vl53l8uld)
- **VL53L5CX / VL53L7CX / VL53L7CH (ToF)**: [`Vl53l7Cmd`](#vl53l7cmd), [`Vl53l7Rpt`](#vl53l7rpt), [`Vl53l7PinAction`](#vl53l7pinaction), [`Vl53l7I2cError`](#vl53l7i2cerror), [`Vl53l7Wire`](#vl53l7wire), [`Vl53l7Info`](#vl53l7info), [`Vl53l7Model`](#vl53l7model), [`Vl53l7Discovery`](#vl53l7discovery), [`Vl53l7Frames`](#vl53l7frames)
- **VL53L0X / L1CX / L1CB / L3CX / L4CX (ToF)**: [`Vl53lxCmd`](#vl53lxcmd), [`Vl53lxRpt`](#vl53lxrpt), [`Vl53lxClearStep`](#vl53lxclearstep), [`Vl53lxWire`](#vl53lxwire), [`Vl53lxInfo`](#vl53lxinfo), [`Vl53lxDriverKind`](#vl53lxdriverkind), [`Vl53lxClass`](#vl53lxclass), [`Vl53lxProduct`](#vl53lxproduct), [`Vl53lxProducts`](#vl53lxproducts), [`Vl53lxDieVariant`](#vl53lxdievariant), [`Vl53lxDieResult`](#vl53lxdieresult), [`Vl53lxL0xRaw`](#vl53lxl0xraw), [`Vl53lxHistogramRaw`](#vl53lxhistogramraw), [`Vl53lxDecode`](#vl53lxdecode)
- **BNO086 (IMU)**: [`SensorId`](#sensorid), [`BnoReport`](#bnoreport), [`InputReport`](#inputreport), [`Acceleration`](#acceleration), [`Gyroscope`](#gyroscope), [`Magnetometer`](#magnetometer), [`UncalibratedGyroscope`](#uncalibratedgyroscope), [`UncalibratedMagnetometer`](#uncalibratedmagnetometer), [`RotationVector`](#rotationvector), [`ScalarReport`](#scalarreport), [`TapDetector`](#tapdetector), [`StepCounter`](#stepcounter), [`StepDetector`](#stepdetector), [`SignificantMotion`](#significantmotion), [`StabilityClassifier`](#stabilityclassifier), [`ShakeDetector`](#shakedetector), [`GenericEvent`](#genericevent), [`PersonalActivityClassifier`](#personalactivityclassifier), [`RawSensor`](#rawsensor), [`GyroIntegratedRV`](#gyrointegratedrv), [`UnknownReport`](#unknownreport), [`Sh2Reports`](#sh2reports), [`Sh2Control`](#sh2control), [`ShtpChannel`](#shtpchannel), [`ShtpHeader`](#shtpheader), [`ShtpCargo`](#shtpcargo), [`ShtpLayer`](#shtplayer)
- **BNO055 (IMU)**: [`Bno055Cmd`](#bno055cmd), [`Bno055Rpt`](#bno055rpt), [`Bno055Trigger`](#bno055trigger), [`Bno055Wire`](#bno055wire), [`Bno055RegData`](#bno055regdata), [`Bno055StreamData`](#bno055streamdata), [`Bno055Info`](#bno055info), [`Bno055OprMode`](#bno055oprmode), [`Bno055PwrMode`](#bno055pwrmode), [`Bno055TempSource`](#bno055tempsource), [`Bno055Vec3`](#bno055vec3), [`Bno055Euler`](#bno055euler), [`Bno055Quat`](#bno055quat), [`Bno055Units`](#bno055units), [`Bno055CalibStatus`](#bno055calibstatus), [`Bno055CalibrationProfile`](#bno055calibrationprofile), [`Bno055Axis`](#bno055axis), [`Bno055AxisRemap`](#bno055axisremap), [`Bno055AccelConfig`](#bno055accelconfig), [`Bno055GyroConfig`](#bno055gyroconfig), [`Bno055MagConfig`](#bno055magconfig), [`Bno055RawBlock`](#bno055rawblock), [`Bno055Regs`](#bno055regs)
- **Firmware update**: [`FwDepz`](#fwdepz)
- **Datasets (record & replay)**: [`TimeSync`](#timesync), [`DatasetDevice`](#datasetdevice), [`DatasetRecord`](#datasetrecord), [`DatasetReader`](#datasetreader)
- **Transport**: [`Framing`](#framing), [`PacketParser`](#packetparser), [`ParserEvent`](#parserevent), [`Packet`](#packet), [`Trash`](#trash), [`CrcError`](#crcerror), [`Crc`](#crc), [`CrcType`](#crctype), [`CrcTypeExtensions`](#crctypeextensions)
- **Protocol codecs**: [`Cmd`](#cmd), [`Rpt`](#rpt), [`Status`](#status), [`SyncPinMode`](#syncpinmode), [`SyncPinPolarity`](#syncpinpolarity), [`Reports`](#reports), [`StatusReport`](#statusreport), [`TextReport`](#textreport), [`SyncTimeReport`](#synctimereport), [`TemperatureReport`](#temperaturereport), [`SequenceErrorReport`](#sequenceerrorreport), [`SyncPinConfig`](#syncpinconfig), [`Common`](#common), [`SensorType`](#sensortype), [`Identity`](#identity), [`IdentityParser`](#identityparser)

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

## SR04

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

VL53_WRITE_REG payload: addr u16 LE, then the raw register data.

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

The live register-sequence driver (init/calibration over the CDC link) is hardware-dependent and out of scope for the decode SDK, mirroring `Vl53l8Uld` (`LiveDriverStubbed`).

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

#### Vl53l4Uld.LiveDriverStubbed *(property)*

```csharp
public static bool LiveDriverStubbed
```

The live ULD register-sequence driver (sensor_init / calibration over the CDC link) is deliberately stubbed: it only means anything against real silicon and cannot be verified from golden vectors. The verifiable codec/math layer below is what this SDK ships.

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
public static (ushort RangeConfigA, ushort RangeConfigB, uint IntermeasurementRaw) RangeTimingRegisters(int timingBudgetMs, int interMeasurementMs, int oscFrequency, int clockPll = 0)
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

## VL53L8 (ToF)

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
public sealed record Vl53l8Frame(ulong TimestampUs, int Resolution, int SiliconTempDegc, int[] DistanceMm, byte[] TargetStatus, byte[] NbTargetDetected, uint[] SignalPerSpad, uint[] AmbientPerSpad, uint[] NbSpadsEnabled, double[] RangeSigmaMm, byte[] Reflectance)
```

One decoded ranging frame. Per-zone arrays are sized to the active resolution (16 for 4×4, 64 for 8×8); zone index runs row-major. Raw wire integers are preserved; `DistanceMm` and `RangeSigmaMm` carry the ST GetRangingData fixed-point scaling applied (÷4 and ÷128).

#### Vl53l8Frame.CnhRaw *(property)*

```csharp
public byte[]? CnhRaw
```

Raw CNH data block (output index 0xC048), byte-swapped like every other block — decode with `DecodeHistogram`. Null when the frame carries no CNH block (CX variants, or CNH not configured).

### Vl53l8FrameDecoder

```csharp
public sealed class Vl53l8FrameDecoder
```

VL53L8 results-frame decoder (contracts/04_SENSOR_VL53L8.md). Verbatim port of the verifiable parse path of the ST ULD's GetRangingData / parse_frame.

Serves both ToF variants — VL53L8CX and VL53L8CH stream the identical results-frame layout; only the frame-id footer offset differs per variant (see `Vl53l8Variant` / `ForVariant`). The CH-only CNH histogram block is copied out raw (`CnhRaw`) and decoded by `DecodeHistogram`; the live register-bridge init/config that produces these frames is hardware-dependent and out of scope (`Vl53l8Uld`).

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

### FrameChunk

```csharp
public sealed record FrameChunk(ulong TimestampUs, int FullSize, int Offset, byte[] Data)
```

One RPT_VL53_FRAME chunk: device timestamp, the full frame size, this chunk's byte offset into the frame, and the chunk payload.

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

VL53L8 advanced-feature DCI codecs — pure, verifiable ports of the ST ULD (BSD-3-Clause) plugin encoders: motion-indicator configuration, detection-threshold block, and xtalk-margin scaling. Shared across both ToF variants (VL53L8CX and VL53L8CH; see `Vl53l8Variant`). The live DCI read/write transport that carries these to the sensor is hardware-dependent and out of scope (see `Vl53l8Uld`).

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

Xtalk margin raw DCI value: round(kcps × 2048).

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

Encode the 64×12-byte DCI_DET_THRESH_START block. Each entry is packed as (i32 low, i32 high, u8 measurement, u8 type, u8 zone, u8 op) with low/high multiplied by the measurement's scale factor; missing entries are zeros.

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
public sealed record Vl53l8CnhAggregate(int[] HistRaw, sbyte[] HistScaler)
```

One decoded CNH aggregate histogram. The float value of bin `i` is `HistRaw[i] / 2^HistScaler[i]` (a per-bin block-floating-point mantissa + shift). Both arrays are `FeatureLength` long.

### Vl53l8CnhResult

```csharp
public sealed record Vl53l8CnhResult(uint RefResidualWord, IReadOnlyList<Vl53l8CnhAggregate> Aggregates)
```

A decoded CNH data block: the reference-residual word plus one histogram per aggregate (in aggregate-id order).

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

### Vl53l8Uld

```csharp
public static class Vl53l8Uld
```

Placeholder for the live VL53L8CX ULD init/config driver (sensor-firmware download + DCI register-bridge configuration).

OUT OF SCOPE for this SDK layer: that driver is a 1:1 register-sequence port that only means anything against real silicon over the CDC link, and cannot be verified from golden vectors. It is deliberately stubbed. The verifiable decode layer lives in `Vl53l8FrameDecoder` (results frames) and `Vl53l8Advanced` / `MotionConfig` (DCI payload codecs), which are covered by the golden vectors.

#### Vl53l8Uld.Init

```csharp
public static void Init()
```

Not implemented: requires live hardware (see class remarks).

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

## BNO086 (IMU)

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
}
```

SH-2 input report IDs (contracts/05_SENSOR_BNO086.md §5).

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

#### GyroIntegratedRV.Fields

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

SH-2 input-report parsers (contracts/05_SENSOR_BNO086.md §5). Byte-exact with the reference Python `reports.py`. Raw wire integers preserved; scaling (Q points) is the caller's concern.

#### Sh2Reports.GyroIntegratedRvId *(constant)*

```csharp
public const int GyroIntegratedRvId = 0x2A
```

#### Sh2Reports.ParseInputCargo

```csharp
public static List<BnoReport> ParseInputCargo(byte[] payload, long captureTimestampUs)
```

Parse a channel-3/4 cargo into typed reports. `captureTimestampUs` is the bridge RPT_DATA capture time; 0xFB base and 0xFA rebase adjust it.

#### Sh2Reports.ParseGyroRvCargo

```csharp
public static GyroIntegratedRV? ParseGyroRvCargo(byte[] payload, long captureTimestampUs)
```

Parse a channel-5 cargo (gyro-integrated RV, dense). Two shapes: bare 7×i16, or prefixed with 0xFB + i32 base delta + u16 delay. Returns null when the cargo is too short.

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

0xF6 FRS Write Data: reserved, u16 word offset, then the u32 data words.

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
public sealed record ShtpCargo(int Channel, int Seq, byte[] Payload)
```

One reassembled cargo: `Payload` excludes all SHTP headers.

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
public ShtpCargo? Feed(byte[] frame)
```

Consume one inbound frame; return the cargo when complete, else null.

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

BNO_WRITE_REG payload: addr u8, then data[1..128].

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

### Bno055RawBlock

```csharp
public sealed record Bno055RawBlock(Bno055Vec3? Accel, Bno055Vec3? Mag, Bno055Vec3? Gyro, Bno055Euler? Euler, Bno055Quat? Quaternion, Bno055Vec3? LinearAccel, Bno055Vec3? Gravity, sbyte? Temperature, byte? CalibStat)
```

Raw register values found in one block read. A channel is null when the window addr..addr+len does not cover all of its bytes.

### Bno055Regs

```csharp
public static class Bno055Regs
```

BNO055 register map and the pure codecs every SDK shares (contracts/13_SENSOR_BNO055.md §4, Bosch BST-BNO055-DS000 rev 1.8) — the "base" layer. Mirrors `depz_sensor_sdk/bno055/regs.py` 1:1. Nothing here touches the wire; `vectors/bno055.json` pins it. Scaling is raw / LSB with `Bno055Units` and the fixed LSB constants below. OUT OF SCOPE (as for every C# sensor): the live driver over the bridge (mode switching, boot / fusion-start polls, page discipline, self-test).

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
public sealed record DatasetRecord(string DeviceId, long THostUs, string Kind, JsonElement Value)
```

One decoded record on the shared host timeline. `Value` is the kind-specific payload object (players use the fields present for the kind).

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

#### DatasetReader.DatasetReader *(constructor)*

```csharp
public DatasetReader(string path) : this(ReadAllLines(path))
```

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
public static byte[] BuildPacket(int cmd, ReadOnlySpan<byte> payload = default, int seq = 0, CrcType crcType = CrcType.None)
```

Frame one packet. The `crcType` bits are set in the header even for an empty payload (matching device TX), but CRC bytes are only appended for non-empty payloads (E6). `seq` is taken modulo 256.

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
    ThroughputTxStart = 0x1C,
    ThroughputTxStop = 0x1D,
    ThroughputRxData = 0x1E,
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
    ThroughputData = 0x88,
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

#### SyncPinConfig.Pack

```csharp
public byte[] Pack()
```

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

Decode an ASCII device string, dropping trailing NUL/0xFF filler.

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
