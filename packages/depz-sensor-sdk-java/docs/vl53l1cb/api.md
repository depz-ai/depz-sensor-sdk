# VL53L1CB (ToF) — API reference

The public API for the VL53L1CB (ToF) sensor. Transport, USB identity, the common
protocol codecs and other cross-sensor symbols shared by every sensor
live in the [top-level API reference](../api.md).

One package serves the whole 1D family (one `APP_VL53L0_4`
firmware): VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX and VL53L4CX. This
page lists the whole family surface and is the same on every family
page; the product table says what differs per board.

Generated from the Javadoc of the Java sources.

## Contents

- **VL53L1CB (ToF)**: [`Vl53l0xDevice`](#vl53l0xdevice), [`Vl53l1cbDevice`](#vl53l1cbdevice), [`Vl53l1cxDevice`](#vl53l1cxdevice), [`Vl53l3cxDevice`](#vl53l3cxdevice), [`Vl53l4cxDevice`](#vl53l4cxdevice), [`Vl53lxDevice`](#vl53lxdevice), [`Vl53lxMeasurement`](#vl53lxmeasurement), [`Vl53lx`](#vl53lx), [`Vl53lx.Vl53lxCmd`](#vl53lxvl53lxcmd), [`Vl53lx.Vl53lxRpt`](#vl53lxvl53lxrpt), [`Vl53lx.Vl53lxInfo`](#vl53lxvl53lxinfo), [`HistogramBinData`](#histogrambindata), [`Vl53l0xUldDriver`](#vl53l0xulddriver), [`Vl53l0xUldDriver.RefCalibration`](#vl53l0xulddriverrefcalibration), [`Vl53l0xUldDriver.RefSpads`](#vl53l0xulddriverrefspads), [`Vl53l1Die`](#vl53l1die), [`Vl53l1UldDriver`](#vl53l1ulddriver), [`Vl53l4UldDriver`](#vl53l4ulddriver), [`Vl53lxDecode`](#vl53lxdecode), [`Vl53lxDecode.DieVariant`](#vl53lxdecodedievariant), [`Vl53lxDecode.L0xRaw`](#vl53lxdecodel0xraw), [`Vl53lxDecode.HistogramRaw`](#vl53lxdecodehistogramraw), [`Vl53lxDriver`](#vl53lxdriver), [`Vl53lxDriver.Thresholds`](#vl53lxdriverthresholds), [`Vl53lxDrivers`](#vl53lxdrivers), [`Vl53lxDrivers.Created`](#vl53lxdriverscreated), [`Vl53lxException`](#vl53lxexception), [`Vl53lxHistogramDriver`](#vl53lxhistogramdriver), [`Vl53lxPlatform`](#vl53lxplatform), [`Vl53lxProducts`](#vl53lxproducts), [`Vl53lxProducts.Bridge`](#vl53lxproductsbridge), [`Vl53lxProducts.Product`](#vl53lxproductsproduct), [`Vl53lxProducts.SensorClass`](#vl53lxproductssensorclass), [`Vl53lxProtocolException`](#vl53lxprotocolexception), [`Vl53lxRange`](#vl53lxrange), [`Vl53lxTarget`](#vl53lxtarget)
- **Contract-10 codecs and result shape (from `Vl53l4` / `Vl53l4Uld`)**: [`Vl53l4.RegData`](#vl53l4regdata), [`Vl53l4.StreamData`](#vl53l4streamdata), [`Vl53l4Uld.Results`](#vl53l4uldresults)

## VL53L1CB (ToF)

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
`startRanging()` and read `measurements()`. Ask `supports` before an optional group;
a missing one throws `UnsupportedOperationException`. This generic class reads the product
from the board's name; the per-product classes fix it.

#### Vl53lxDevice.fixedProduct

```java
protected @Nullable String fixedProduct()
```

The product this class drives; null = read it from the device name.

#### Vl53lxDevice.boardName

```java
public synchronized String boardName()
```

The board's device name, e.g. `DEPZ ToF Sensor VL53L4CX USB v2.1 TEXAMPLE03`.

#### Vl53lxDevice.detected

```java
public @Nullable String detected()
```

The product the device name carries, or null on an unstamped board.

#### Vl53lxDevice.product

```java
public @Nullable String product()
```

The product `init` bound (null before).

#### Vl53lxDevice.driverKind

```java
public @Nullable String driverKind()
```

#### Vl53lxDevice.caveat

```java
public @Nullable String caveat()
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
public List<String> driverKinds(@Nullable String product)
```

The driver kinds a product has (null: this board's).

#### Vl53lxDevice.driverKinds

```java
public List<String> driverKinds()
```

#### Vl53lxDevice.init

```java
public void init(@Nullable String driver, @Nullable String product)
```

Bind the (product, driver kind) pair and initialise the sensor. `product` null: this class's,
else the board name's; `driver` null: the product's first kind. A pair the table lacks throws
`UnsupportedOperationException` naming what the product has.

#### Vl53lxDevice.init

```java
public void init(@Nullable String driver)
```

#### Vl53lxDevice.init

```java
public void init()
```

#### Vl53lxDevice.identifyProduct

```java
public Map<String, @Nullable Object> identifyProduct()
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
public @Nullable Integer driverReachMm()
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
public void configure(int budgetMs, int interMs, @Nullable String mode, @Nullable Integer offsetMm, @Nullable Integer xtalkKcps, @Nullable Integer signalKcps)
```

Re-initialise the sensor and apply a ranging configuration. The re-init is deliberate: it is the
only way to know what the configuration registers hold. `mode` goes on before the budget;
`offsetMm` / `xtalkKcps` re-apply a stored calibration; `signalKcps` replaces the
blob's signal threshold (each null: leave it). `interMs` 0 = continuous. An unsupported group
is refused before the re-init.

#### Vl53lxDevice.configure

```java
public void configure(int budgetMs, int interMs, @Nullable String mode)
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
public @Nullable String getMode()
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
public int calibrateOffset(int targetDistMm, @Nullable Integer nbSamples)
```

Offset calibration against a flat target; returns the offset programmed (store it on the host).

#### Vl53lxDevice.calibrateOffset

```java
public int calibrateOffset(int targetDistMm)
```

#### Vl53lxDevice.calibrateXtalk

```java
public int calibrateXtalk(int targetDistMm, @Nullable Integer nbSamples)
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

The distance-window interrupt; stays armed until the next init / configure. The distances are u16 registers,
the window a u8 one: a value outside is refused before anything is written, never masked (contract 07 §3).

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

The ROI centre SPAD, a u8 register: a value outside 0..255 is refused before anything is written (contract 07 §3).

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
public Vl53lxMeasurement measureOnce(@Nullable Duration timeout)
```

One polled measurement: start, data ready, read, release the interrupt, stop. Not while streaming.

#### Vl53lxDevice.measureOnce

```java
public Vl53lxMeasurement measureOnce()
```

#### Vl53lxDevice.measurements

```java
public SensorStream<Vl53lxMeasurement> measurements()
```

The stream of ranging measurements (contract 07 §3): `measurements().on(cb)` subscribes
(reader thread; don't block), `measurements().subscribe(capacity)` queues (bounded,
drop-oldest, subscribed when it returns), `measurements().next(timeout)` waits for one;
`parseErrors()` counts the stream reports the driver did not decode — and, before
`init()`, those shorter than their header (contract 07 §2).

### Vl53lxMeasurement

```java
public record Vl53lxMeasurement(long timestampUs, int distanceMm, int status, String statusText, double signalKcps, double ambientKcps, double sigmaMm, double spads, List<Vl53lxTarget> targets, Map<String, Object> extra, @Nullable Object bins)
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
public @Nullable Vl53lxTarget primaryTarget()
```

The one target a single-number readout shows, or null.

### Vl53lx

```java
public final class Vl53lx
```

VL53L 1D-family register-bridge wire codecs, protocol v2.00
(contract 12).

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
public static byte[] packStartStream(int addr, int len, @Nullable List<List<Integer>> clear, int flags)
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

ST driver port: unstable — not covered by the 1.0 freeze.

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

ST driver port: unstable — not covered by the 1.0 freeze.

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

ST driver port: unstable — not covered by the 1.0 freeze.

#### Vl53l1UldDriver.setDetectionThresholds

```java
public void setDetectionThresholds(int distanceLowMm, int distanceHighMm, int window, boolean intOnNoTarget)
```

### Vl53l4UldDriver

```java
public final class Vl53l4UldDriver extends Vl53l1Die
```

VL53L4CD ULD 2.2.3 (`VL53L4CD_api.c` + `VL53L4CD_calibration.c`) for VL53L4CD and VL53L4CX alike.

ST driver port: unstable — not covered by the 1.0 freeze.

### Vl53lxDecode

```java
public final class Vl53lxDecode
```

Stateless decoders of the blocks the 1D-family bridge streams
(contract 12 §4) — the "base" every SDK implements. Mirrors
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

ST driver port: unstable — not covered by the 1.0 freeze.

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

ST driver port: unstable — not covered by the 1.0 freeze.

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
public List<List<Integer>> clearSteps()
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
public @Nullable Integer setMode(String name)
```

Apply one of `modes()`. Returns the timing budget in effect after the change, ms, on a driver whose
mode change can move it (VL53L1: contract 12 §5), else null.

#### Vl53lxDriver.getMode

```java
public String getMode()
```

#### Vl53lxDriver.reachMm

```java
public @Nullable Integer reachMm()
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
public int calibrateOffset(int targetDistMm, @Nullable Integer nbSamples)
```

`nbSamples` null: the driver's default.

#### Vl53lxDriver.calibrateXtalk

```java
public int calibrateXtalk(int targetDistMm, @Nullable Integer nbSamples)
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
public Object binData(byte @Nullable [] raw)
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

#### Vl53lxDriver.Thresholds.of

```java
public static Thresholds of(int distanceLowMm, int distanceHighMm, int window)
```

Build one: the path that survives a component appended after 1.0 (contract 07 §5).

### Vl53lxDrivers

```java
public final class Vl53lxDrivers
```

The drivers behind the product table (`uld/registry.py`): which (product, kind) pairs exist,
the driver each one runs and what to tell the user about it. A missing pair is absent on purpose.

ST driver port: unstable — not covered by the 1.0 freeze.

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
public record Created(Vl53lxDriver driver, @Nullable String caveat)
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

ST driver port: unstable — not covered by the 1.0 freeze.

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

ST driver port: unstable — not covered by the 1.0 freeze.

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
(contract 12 §1, §3; Python reference
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
public static @Nullable String productFromBoardName(@Nullable String name)
```

`"ToF Sensor VL53L4CD USB v2.1"` → `"VL53L4CD"`: the first
`VL53L<digit><part>` match, case-insensitive. `null` if the name
carries no product this table serves (an unstamped board, an unknown part).

#### Vl53lxProducts.resolveClass

```java
public static SensorClass resolveClass(@Nullable String usbModel, @Nullable String deviceName)
```

Class resolution (contract 12 §1, pinned by `vl53lx.json model`): the
production PID model (`usbModel`, e.g. `"vl53l0x"`; may be
`null`) if it is a family product, else the product the device name
carries, else the generic class.

### Vl53lxProducts.Bridge

```java
public record Bridge(int addrWidth, List<List<Integer>> clearSteps, int maxKhz, int blockAddr, int blockLen)
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
public final @Nullable String product
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

## Contract-10 codecs and result shape (from `Vl53l4` / `Vl53l4Uld`)

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

### Vl53l4Uld.Results

```java
public record Results(int rangeStatus, int distanceMm, int ambientRateKcps, int ambientPerSpadKcps, int signalRateKcps, int signalPerSpadKcps, int numberOfSpad, int sigmaMm, int streamCount)
```

VL53L4CD_ResultsData_t plus the sensor's own frame counter.
