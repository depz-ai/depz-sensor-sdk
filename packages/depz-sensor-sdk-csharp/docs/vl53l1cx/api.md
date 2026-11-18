# VL53L1CX (ToF) — API reference

The public API for the VL53L1CX (ToF) sensor. The shared foundation —
transport framing, common protocol codecs, firmware image parse,
dataset reader and USB identity — lives in the
[top-level API reference](../api.md).

One namespace serves the whole 1D family (one `APP_VL53L0_4`
firmware): VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX and VL53L4CX. This
page lists the whole `Depz.Sensor.Vl53lx` surface and is the same on
every family page; the product table says what differs per board.

Auto-generated from the source — edit the `///` summaries
in the source, not this file.

## Contents

- **VL53L1CX (ToF)**: [`Vl53lxCmd`](#vl53lxcmd), [`Vl53lxRpt`](#vl53lxrpt), [`Vl53lxClearStep`](#vl53lxclearstep), [`Vl53lxWire`](#vl53lxwire), [`Vl53lxInfo`](#vl53lxinfo), [`Vl53lxDriverKind`](#vl53lxdriverkind), [`Vl53lxClass`](#vl53lxclass), [`Vl53lxProduct`](#vl53lxproduct), [`Vl53lxProducts`](#vl53lxproducts), [`Vl53lxDieVariant`](#vl53lxdievariant), [`Vl53lxDieResult`](#vl53lxdieresult), [`Vl53lxL0xRaw`](#vl53lxl0xraw), [`Vl53lxHistogramRaw`](#vl53lxhistogramraw), [`Vl53lxDecode`](#vl53lxdecode), [`Vl53lxMeasurement`](#vl53lxmeasurement), [`Vl53lxDevice`](#vl53lxdevice), [`Vl53l0xDevice`](#vl53l0xdevice), [`Vl53l1cxDevice`](#vl53l1cxdevice), [`Vl53l1cbDevice`](#vl53l1cbdevice), [`Vl53l3cxDevice`](#vl53l3cxdevice), [`Vl53l4cxDevice`](#vl53l4cxdevice), [`HistogramBinData`](#histogrambindata), [`Vl53lxException`](#vl53lxexception), [`Vl53lxProtocolException`](#vl53lxprotocolexception), [`Vl53lxTarget`](#vl53lxtarget), [`Vl53lxRange`](#vl53lxrange), [`Vl53lxDriver`](#vl53lxdriver)
- **Contract-10 records (from `Depz.Sensor.Vl53l4`)**: [`Vl53l4RegData`](#vl53l4regdata), [`Vl53l4StreamData`](#vl53l4streamdata), [`Vl53l4Xshut`](#vl53l4xshut)

## VL53L1CX (ToF)

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

VL53L 1D-family register-bridge command opcodes, protocol v2.00 (contract 12 §2). The contract-10 opcodes plus `SetAddrWidth`.

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
public static readonly IReadOnlyList<string> DriverKindNames = Array.AsReadOnly(new[] { "uld", "ulp", "histogram" })
```

The driver kinds in the order a UI lists them (uld/registry.py DRIVER_KINDS).

#### Vl53lxProducts.DriverKinds

```csharp
public static IReadOnlyList<string> DriverKinds(string product)
```

The driver kinds `product` has, in `DriverKindNames` order.

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

#### Vl53lxDecode.DieVariantParams

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

One ranging result of the 1D family, the same shape for every product. On the histogram driver `Targets` holds every return, strongest first, the top level repeats the first and `Bins` carries the raw 24-bin histogram, a `HistogramBinData`; the light drivers leave both empty.

#### Vl53lxMeasurement.PlottableStatuses *(field)*

```csharp
public static readonly IReadOnlyList<int> PlottableStatuses = Array.AsReadOnly(new[] { 0, 6, 11 })
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

#### Vl53lxDevice.Measurements *(property)*

```csharp
public SensorStream<Vl53lxMeasurement> Measurements
```

The streamed measurements (contract 07 §3): `Measurements.On(cb)`, `Measurements.Subscribe()` (64 by default), `Measurements.Next()`; `ParseErrors` counts stream reports dropped because they were shorter than their header (before `Init` too) or the driver did not decode them (contract 07 §2).

#### Vl53lxDevice.BoardName *(property)*

```csharp
public string BoardName
```

The board's device name, e.g. `DEPZ ToF Sensor VL53L4CX USB v2.1 TEXAMPLE03`.

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

The ULD port in use — escape hatch for product-specific calls; unstable — not covered by the 1.0 freeze.

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

#### Vl53lxDevice.BudgetRange

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

#### Vl53lxDevice.GetRangeTiming

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

#### Vl53lxDevice.GetDetectionThresholds

```csharp
public (int DistanceLowMm, int DistanceHighMm, int Window) GetDetectionThresholds()
```

#### Vl53lxDevice.SetDetectionThresholds

```csharp
public void SetDetectionThresholds(int distanceLowMm, int distanceHighMm, int window)
```

The distance-window interrupt; stays armed until the next init / configure. The distances are u16 registers, the window a u8 one: a value outside is refused before anything is written, never masked (contract 07 §3).

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

#### Vl53lxDevice.GetRoi

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

The ROI centre SPAD, a u8 register: a value outside 0..255 is refused before anything is written, never masked (contract 07 §3).

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

### Vl53l0xDevice

```csharp
public sealed class Vl53l0xDevice : Vl53lxDevice
```

VL53L0X (2 m, 1-byte register addresses, 400 kHz): ULD API 1.0.4 with ranging profiles.

#### Vl53l0xDevice.Vl53l0xDevice *(constructor)*

```csharp
public Vl53l0xDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout)
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

### Vl53l1cbDevice

```csharp
public sealed class Vl53l1cbDevice : Vl53lxDevice
```

VL53L1CB (8 m, cover-glass module): the VL53L1CX die and drivers; usually needs a crosstalk calibration.

#### Vl53l1cbDevice.Vl53l1cbDevice *(constructor)*

```csharp
public Vl53l1cbDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout)
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

### Vl53l4cxDevice

```csharp
public sealed class Vl53l4cxDevice : Vl53lxDevice
```

VL53L4CX (6 m): the histogram driver only — to run it light, borrow a sibling's with `Init(product: "VL53L4CD")`.

#### Vl53l4cxDevice.Vl53l4cxDevice *(constructor)*

```csharp
public Vl53l4cxDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout)
```

### HistogramBinData

```csharp
public sealed class HistogramBinData
```

`VL53LX_histogram_bin_data_t`: 24 bins plus everything the post-processing needs to read them — the VCSEL period they were taken at, the phase of zero distance, the bin sequence and the ambient estimate. What `Vl53lxMeasurement.Bins` carries on the histogram products (contract 07 §5 produced type: read it, the SDK fills it; `Fields` has it under the C driver's and the Python SDK's names).

#### HistogramBinData.ResultInterruptStatus *(property)*

```csharp
public long ResultInterruptStatus
```

#### HistogramBinData.ResultRangeStatus *(property)*

```csharp
public long ResultRangeStatus
```

#### HistogramBinData.ResultReportStatus *(property)*

```csharp
public long ResultReportStatus
```

#### HistogramBinData.ResultStreamCount *(property)*

```csharp
public long ResultStreamCount
```

#### HistogramBinData.ResultDssActualEffectiveSpads *(property)*

```csharp
public long ResultDssActualEffectiveSpads
```

#### HistogramBinData.PhasecalResultReferencePhase *(property)*

```csharp
public long PhasecalResultReferencePhase
```

#### HistogramBinData.PhasecalResultVcselStart *(property)*

```csharp
public long PhasecalResultVcselStart
```

#### HistogramBinData.BinData *(property)*

```csharp
public long[] BinData
```

The photon counts (always `HistogramBufferSize` long; `BinsInData` are in use).

#### HistogramBinData.ZoneId *(property)*

```csharp
public long ZoneId
```

#### HistogramBinData.FirstBin *(property)*

```csharp
public long FirstBin
```

#### HistogramBinData.NumberOfBins *(property)*

```csharp
public long NumberOfBins
```

#### HistogramBinData.BinsInData *(property)*

```csharp
public long BinsInData
```

#### HistogramBinData.CalConfigVcselStart *(property)*

```csharp
public long CalConfigVcselStart
```

#### HistogramBinData.VcselWidth *(property)*

```csharp
public long VcselWidth
```

#### HistogramBinData.FastOscFrequency *(property)*

```csharp
public long FastOscFrequency
```

#### HistogramBinData.VcselPeriod *(property)*

```csharp
public long VcselPeriod
```

#### HistogramBinData.BinSeq *(property)*

```csharp
public int[] BinSeq
```

The bin-sequence codes.

#### HistogramBinData.BinRep *(property)*

```csharp
public long[] BinRep
```

How often each code was sampled.

#### HistogramBinData.MinBinValue *(property)*

```csharp
public long MinBinValue
```

#### HistogramBinData.MaxBinValue *(property)*

```csharp
public long MaxBinValue
```

#### HistogramBinData.NumberOfAmbientBins *(property)*

```csharp
public long NumberOfAmbientBins
```

#### HistogramBinData.NumberOfAmbientSamples *(property)*

```csharp
public long NumberOfAmbientSamples
```

#### HistogramBinData.AmbientEventsSum *(property)*

```csharp
public long AmbientEventsSum
```

#### HistogramBinData.AmbientPerBin *(property)*

```csharp
public long AmbientPerBin
```

#### HistogramBinData.TotalPeriodsElapsed *(property)*

```csharp
public long TotalPeriodsElapsed
```

#### HistogramBinData.PeakDurationUs *(property)*

```csharp
public long PeakDurationUs
```

#### HistogramBinData.WoiDurationUs *(property)*

```csharp
public long WoiDurationUs
```

#### HistogramBinData.ZeroDistancePhase *(property)*

```csharp
public long ZeroDistancePhase
```

#### HistogramBinData.RoiConfigUserRoiCentreSpad *(property)*

```csharp
public long RoiConfigUserRoiCentreSpad
```

#### HistogramBinData.RoiConfigUserRoiRequestedGlobalXySize *(property)*

```csharp
public long RoiConfigUserRoiRequestedGlobalXySize
```

#### HistogramBinData.RangeStatus *(property)*

```csharp
public long RangeStatus
```

`ResultRangeStatus` without its flag bits.

#### HistogramBinData.Fields

```csharp
public IReadOnlyDictionary<string, object?> Fields()
```

The fields under the C driver's (and the Python SDK's) names.

#### HistogramBinData.ToString

```csharp
public override string ToString()
```

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
public Vl53lxProtocolException(string message) : base(message)
```

#### Vl53lxProtocolException.Vl53lxProtocolException *(constructor)*

```csharp
public Vl53lxProtocolException(string message, Exception inner) : base(message, inner)
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

#### Vl53lxDriver.BudgetMs

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

#### Vl53lxDriver.StreamBlock

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

#### Vl53lxDriver.GetRangeTiming

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
public virtual int? SetMode(string name)
```

Apply one of the modes. Returns the timing budget in effect after the change, ms, on a driver whose mode change can move it (VL53L1: contract 12 §5), else null.

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

#### Vl53lxDriver.GetDetectionThresholds

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

#### Vl53lxDriver.GetRoi

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

## Contract-10 records (from `Depz.Sensor.Vl53l4`)

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
