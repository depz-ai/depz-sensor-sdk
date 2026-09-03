# API reference

Auto-generated from the public C# surface under `src/Depz.Sensor/` by
`scripts/gen_api_md.py` — run it to regenerate (see below). Edit the
`///` XML-doc summaries in the source, not this file.

Each sensor also has a focused reference with just its own symbols:
[SR04](sr04/api.md) · [VL53L4CD](vl53l4cd/api.md) · [VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · [BNO086](bno086/api.md).

Regenerate: `python3 scripts/gen_api_md.py` (from the package root).

## Contents

- **USB identity**: [`UsbIds`](#usbids)
- **SR04**: [`Sr04Cmd`](#sr04cmd), [`Sr04Rpt`](#sr04rpt), [`Sr04Data`](#sr04data), [`Sr04`](#sr04)
- **VL53L4CD (ToF)**: [`Vl53l4Cmd`](#vl53l4cmd), [`Vl53l4Rpt`](#vl53l4rpt), [`Vl53l4Xshut`](#vl53l4xshut), [`Vl53l4Wire`](#vl53l4wire), [`Vl53l4RegData`](#vl53l4regdata), [`Vl53l4Info`](#vl53l4info), [`Vl53l4StreamData`](#vl53l4streamdata), [`Vl53l4Exception`](#vl53l4exception), [`Vl53l4Results`](#vl53l4results), [`Vl53l4Uld`](#vl53l4uld)
- **VL53L8 (ToF)**: [`Vl53l8Variant`](#vl53l8variant), [`Vl53l8Frame`](#vl53l8frame), [`Vl53l8FrameDecoder`](#vl53l8framedecoder), [`Vl53l8Cmd`](#vl53l8cmd), [`Vl53l8Rpt`](#vl53l8rpt), [`Vl53l8Wire`](#vl53l8wire), [`FrameChunk`](#framechunk), [`FrameReassembler`](#framereassembler), [`Vl53l8Advanced`](#vl53l8advanced), [`MotionConfig`](#motionconfig), [`Vl53l8CnhConfig`](#vl53l8cnhconfig), [`Vl53l8CnhAggregate`](#vl53l8cnhaggregate), [`Vl53l8CnhResult`](#vl53l8cnhresult), [`Vl53l8Cnh`](#vl53l8cnh), [`Vl53l8Uld`](#vl53l8uld)
- **BNO086 (IMU)**: [`SensorId`](#sensorid), [`BnoReport`](#bnoreport), [`InputReport`](#inputreport), [`Acceleration`](#acceleration), [`Gyroscope`](#gyroscope), [`Magnetometer`](#magnetometer), [`UncalibratedGyroscope`](#uncalibratedgyroscope), [`UncalibratedMagnetometer`](#uncalibratedmagnetometer), [`RotationVector`](#rotationvector), [`ScalarReport`](#scalarreport), [`TapDetector`](#tapdetector), [`StepCounter`](#stepcounter), [`StepDetector`](#stepdetector), [`SignificantMotion`](#significantmotion), [`StabilityClassifier`](#stabilityclassifier), [`ShakeDetector`](#shakedetector), [`GenericEvent`](#genericevent), [`PersonalActivityClassifier`](#personalactivityclassifier), [`RawSensor`](#rawsensor), [`GyroIntegratedRV`](#gyrointegratedrv), [`UnknownReport`](#unknownreport), [`Sh2Reports`](#sh2reports), [`Sh2Control`](#sh2control), [`ShtpChannel`](#shtpchannel), [`ShtpHeader`](#shtpheader), [`ShtpCargo`](#shtpcargo), [`ShtpLayer`](#shtplayer)
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

#### UsbIds.PidVl53l8Ch *(constant)*

```csharp
public const int PidVl53l8Ch = 0xED40
```

#### UsbIds.PidVl53l8Cx *(constant)*

```csharp
public const int PidVl53l8Cx = 0xED4B
```

#### UsbIds.PidVl53l4Cd *(constant)*

```csharp
public const int PidVl53l4Cd = 0xED45
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

VL53_XSHUT actions. `Reset` is blocking on the MCU and is answered only after the sensor's boot handshake (allow ≥ 1.5 s).

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

USB product id — `Ch` ships as production PID 0xED40; `Cx` is the development default and enumerates under the raw ST VID/PID. Results-frame footer-id offset — CX FW (ULD 2.1.0) echoes the frame id 12 bytes before the end, CH FW (VL53LMZ 2.0.16) 4 bytes (`ForVariant`). CNH histograms — CH-only, not yet decoded (`Vl53l8Cnh`).

### Vl53l8Frame

```csharp
public sealed record Vl53l8Frame(ulong TimestampUs, int Resolution, int SiliconTempDegc, int[] DistanceMm, byte[] TargetStatus, byte[] NbTargetDetected, uint[] SignalPerSpad, uint[] AmbientPerSpad, uint[] NbSpadsEnabled, double[] RangeSigmaMm, byte[] Reflectance)
```

One decoded ranging frame. Per-zone arrays are sized to the active resolution (16 for 4×4, 64 for 8×8); zone index runs row-major. Raw wire integers are preserved; `DistanceMm` and `RangeSigmaMm` carry the ST GetRangingData fixed-point scaling applied (÷4 and ÷128).

### Vl53l8FrameDecoder

```csharp
public sealed class Vl53l8FrameDecoder
```

VL53L8 results-frame decoder (contracts/04_SENSOR_VL53L8.md). Verbatim port of the verifiable parse path of the ST ULD's GetRangingData / parse_frame.

Serves both ToF variants — VL53L8CX and VL53L8CH stream the identical results-frame layout; only the frame-id footer offset differs per variant (see `Vl53l8Variant` / `ForVariant`). The CH-only CNH histogram block is a separate, not-yet-decoded extension point (`Vl53l8Cnh`); the live register-bridge init/config that produces these frames is hardware-dependent and out of scope (`Vl53l8Uld`).

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
public Vl53l8FrameDecoder(int footerIdOff = FooterIdOffsetCx)
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

CNH is what the VL53L8CH firmware adds on top of the CX base: a per-aggregate distance histogram captured in poll-mode alongside the normal ranging frame. The shared results-frame decode (`Vl53l8FrameDecoder`) and the advanced DCI codecs (`Vl53l8Advanced` / `MotionConfig`) already serve both CX and CH; this is the CH-only histogram parse.

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
    Bno086,
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
