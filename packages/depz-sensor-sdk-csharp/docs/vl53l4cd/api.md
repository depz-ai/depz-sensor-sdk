# VL53L4CD (ToF, single-zone) — API reference

The public API for the VL53L4CD (ToF, single-zone) sensor. The shared foundation —
transport framing, common protocol codecs, firmware image parse,
dataset reader and USB identity — lives in the
[top-level API reference](../api.md).

Auto-generated from the source — edit the `///` summaries
in the source, not this file.

## Contents

- **VL53L4CD (ToF, single-zone)**: [`Vl53l4Measurement`](#vl53l4measurement), [`Vl53l4cdDevice`](#vl53l4cddevice), [`Vl53l4Cmd`](#vl53l4cmd), [`Vl53l4Rpt`](#vl53l4rpt), [`Vl53l4Xshut`](#vl53l4xshut), [`Vl53l4Wire`](#vl53l4wire), [`Vl53l4RegData`](#vl53l4regdata), [`Vl53l4Info`](#vl53l4info), [`Vl53l4StreamData`](#vl53l4streamdata), [`Vl53l4Exception`](#vl53l4exception), [`Vl53l4Results`](#vl53l4results), [`Vl53l4cdDriver`](#vl53l4cddriver)

## VL53L4CD (ToF, single-zone)

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

VL53L4CD single-zone ToF (contract 10). The board is a thin I2C bridge; the ST ULD 2.2.3 runs here (`Uld`). `Init` first, then configure and `StartRanging`: one result block per INT edge streams to `Measurements`. Configuration while ranging throws `InvalidStateException` — the stream owns the register bank.

#### Vl53l4cdDevice.ModelId *(constant)*

```csharp
public const int ModelId = Vl53l4Uld.ModelId
```

IDENTIFICATION__MODEL_ID word of VL53L4CD silicon, what `IsAlive` expects.

#### Vl53l4cdDevice.I2cKhzBoot *(constant)*

```csharp
public const int I2cKhzBoot = Vl53l4Uld.I2cKhzBoot
```

The bus speed the bridge boots at; `Init` writes the configuration block there.

#### Vl53l4cdDevice.I2cKhzDefault *(constant)*

```csharp
public const int I2cKhzDefault = Vl53l4Uld.I2cKhzDefault
```

The bus speed `Init` leaves by default (the result-block read is ~4× faster than at boot).

#### Vl53l4cdDevice.WindowBelow *(constant)*

```csharp
public const int WindowBelow = Vl53l4Uld.WindowBelow
```

Detection-threshold window modes for `SetDetectionThresholds` (SYSTEM__INTERRUPT).

#### Vl53l4cdDevice.WindowAbove *(constant)*

```csharp
public const int WindowAbove = Vl53l4Uld.WindowAbove
```

#### Vl53l4cdDevice.WindowOut *(constant)*

```csharp
public const int WindowOut = Vl53l4Uld.WindowOut
```

#### Vl53l4cdDevice.WindowIn *(constant)*

```csharp
public const int WindowIn = Vl53l4Uld.WindowIn
```

#### Vl53l4cdDevice.RangeStatusNames *(property)*

```csharp
public static IReadOnlyDictionary<int, string> RangeStatusNames
```

Range status (after the ST mapping) → its meaning; 0 is a valid measurement.

#### Vl53l4cdDevice.Vl53l4cdDevice *(constructor)*

```csharp
public Vl53l4cdDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

#### Vl53l4cdDevice.Measurements *(property)*

```csharp
public SensorStream<Vl53l4Measurement> Measurements
```

The streamed measurements (contract 07 §3): `Measurements.On(cb)`, `Measurements.Subscribe()` (64 by default), `Measurements.Next()`; `ParseErrors` counts stream reports whose result block did not decode, dropped (contract 07 §2).

#### Vl53l4cdDevice.Uld *(property)*

```csharp
public Vl53l4cdDriver Uld
```

The ULD driver (escape hatch for raw register access); unstable — not covered by the 1.0 freeze.

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
public void Init(int busKhz = I2cKhzDefault)
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

Hardware reset through XSHUT (≤ 20 ms on the MCU, typ. ~2 ms); call `Init` again.

#### Vl53l4cdDevice.BridgeInfo

```csharp
public Vl53l4Info BridgeInfo()
```

RPT_VL53_INFO: sensor identity, pin levels and bridge counters (I2cErrors is cleared by a successful XSHUT reset). Safe while streaming.

#### Vl53l4cdDevice.SetI2cSpeedKhz

```csharp
public void SetI2cSpeedKhz(int khz)
```

Re-time the bridge's I2C bus to the step nearest `khz`. Not while ranging.

#### Vl53l4cdDevice.GetRangeTiming

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

#### Vl53l4cdDevice.GetDetectionThresholds

```csharp
public (int DistanceLowMm, int DistanceHighMm, int Window) GetDetectionThresholds()
```

(distance low mm, distance high mm, window: `WindowBelow` …).

#### Vl53l4cdDevice.SetDetectionThresholds

```csharp
public void SetDetectionThresholds(int distanceLowMm, int distanceHighMm, int window)
```

The distance-window interrupt: INT only fires while the window holds. The distances are u16 registers, the window a u8 one: a value outside is refused before anything is written, never masked (contract 07 §3).

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

VL53L4CD register-bridge command opcodes (contract 10).

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

VL53_READ_REG payload: addr u16 LE, len u16 LE, with the transfer limit of contract 10 §1 — len 1..`XferMax` and addr + len ≤ 0x10000.

#### Vl53l4Wire.PackWriteReg

```csharp
public static byte[] PackWriteReg(ushort addr, ReadOnlySpan<byte> data)
```

VL53_WRITE_REG payload: addr u16 LE, then the raw register data (1..`XferMax` bytes, addr + N ≤ 0x10000).

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

RPT_VL53_INFO — bridge diagnostics (21-byte payload). Counters wrap silently; watch increments, not absolute values. `I2cErrors` counts since the last successful XSHUT reset (the bridge clears it there), not since power-up. `Initialized` = 1 when MODEL_ID read back as 0xEBAA on this GET_INFO. `ModelId` expected 0xEBAA, `FwStatus` expected 0x03 (booted). `LastI2cError`: 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR.

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

What the sensor did, not what the caller asked: a boot or data-ready timeout, a short result block, an oscillator word of 0, a failed xtalk calibration. A refused argument is an `ArgumentException` (contract 07 §3).

#### Vl53l4Exception.Vl53l4Exception *(constructor)*

```csharp
public Vl53l4Exception(string message) : base(message)
```

### Vl53l4Results

```csharp
public sealed record Vl53l4Results(int RangeStatus, int DistanceMm, int AmbientRateKcps, int AmbientPerSpadKcps, int SignalRateKcps, int SignalPerSpadKcps, int NumberOfSpad, int SigmaMm, int StreamCount)
```

VL53L4CD_ResultsData_t plus the sensor's own frame counter. `StreamCount` (RESULT__STREAM_COUNT, wraps at 255) tells a frame the host never received from one the sensor never produced.

### Vl53l4cdDriver

```csharp
public sealed class Vl53l4cdDriver
```

The ST VL53L4CD ULD 2.2.3 (VL53L4CD_api.c + VL53L4CD_calibration.c) on the host, ported from the reference Python driver: init, ranging, timing, offset, crosstalk, thresholds, temperature update, calibration. Every request sequence is the Python one, so captures of either replay in both. Register contents are big-endian.

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

#### Vl53l4cdDriver.GetRangeTiming

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

#### Vl53l4cdDriver.GetDetectionThresholds

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
