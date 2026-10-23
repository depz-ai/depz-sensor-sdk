# VL53L4CD (ToF) — API reference

The VL53L4CD single-zone time-of-flight surface: the `Vl53l4cd` class,
the host-side ST ULD 2.2.3 driver (`VL53L4CD` + codecs), and the
register-bridge wire codecs. Discovery, device-base, transport and
other cross-sensor symbols shared by every sensor live in the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Enumerations

- [Vl53l4Cmd](enumerations/Vl53l4Cmd.md)
- [Vl53l4Rpt](enumerations/Vl53l4Rpt.md)

## Classes

- [Vl53l4cdError](classes/Vl53l4cdError.md)
- [VL53L4CD](classes/VL53L4CD.md)
- [Vl53l4cd](classes/Vl53l4cd-1.md)

## Interfaces

- [Vl53l4RegData](interfaces/Vl53l4RegData.md)
- [Vl53l4Info](interfaces/Vl53l4Info.md)
- [Vl53l4StreamData](interfaces/Vl53l4StreamData.md)
- [Vl53l4Platform](interfaces/Vl53l4Platform.md)
- [Vl53l4Results](interfaces/Vl53l4Results.md)
- [RangeTimingRegisters](interfaces/RangeTimingRegisters.md)
- [RangeTiming](interfaces/RangeTiming.md)
- [Vl53l4Measurement](interfaces/Vl53l4Measurement.md)
- [Vl53l4Options](interfaces/Vl53l4Options.md)

## Variables

- [VL53L4\_XFER\_MAX](variables/VL53L4_XFER_MAX.md)
- [VL53L4\_XSHUT\_OFF](variables/VL53L4_XSHUT_OFF.md)
- [VL53L4\_XSHUT\_ON](variables/VL53L4_XSHUT_ON.md)
- [VL53L4\_XSHUT\_RESET](variables/VL53L4_XSHUT_RESET.md)
- [VL53L4\_SF\_INT\_ACT\_HIGH](variables/VL53L4_SF_INT_ACT_HIGH.md)
- [VL53L4\_I2C\_KHZ\_STEPS](variables/VL53L4_I2C_KHZ_STEPS.md)
- [VL53L4\_I2C\_ERROR\_NAMES](variables/VL53L4_I2C_ERROR_NAMES.md)
- [ULD\_VERSION](variables/ULD_VERSION.md)
- [SOFT\_RESET](variables/SOFT_RESET.md)
- [I2C\_SLAVE\_\_DEVICE\_ADDRESS](variables/I2C_SLAVE__DEVICE_ADDRESS.md)
- [OSC\_FREQUENCY](variables/OSC_FREQUENCY.md)
- [VHV\_CONFIG\_\_TIMEOUT\_MACROP\_LOOP\_BOUND](variables/VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND.md)
- [XTALK\_PLANE\_OFFSET\_KCPS](variables/XTALK_PLANE_OFFSET_KCPS.md)
- [XTALK\_X\_PLANE\_GRADIENT\_KCPS](variables/XTALK_X_PLANE_GRADIENT_KCPS.md)
- [XTALK\_Y\_PLANE\_GRADIENT\_KCPS](variables/XTALK_Y_PLANE_GRADIENT_KCPS.md)
- [RANGE\_OFFSET\_MM](variables/RANGE_OFFSET_MM.md)
- [INNER\_OFFSET\_MM](variables/INNER_OFFSET_MM.md)
- [OUTER\_OFFSET\_MM](variables/OUTER_OFFSET_MM.md)
- [GPIO\_HV\_MUX\_\_CTRL](variables/GPIO_HV_MUX__CTRL.md)
- [GPIO\_\_TIO\_HV\_STATUS](variables/GPIO__TIO_HV_STATUS.md)
- [SYSTEM\_\_INTERRUPT](variables/SYSTEM__INTERRUPT.md)
- [RANGE\_CONFIG\_A](variables/RANGE_CONFIG_A.md)
- [RANGE\_CONFIG\_B](variables/RANGE_CONFIG_B.md)
- [RANGE\_CONFIG\_\_SIGMA\_THRESH](variables/RANGE_CONFIG__SIGMA_THRESH.md)
- [MIN\_COUNT\_RATE\_RTN\_LIMIT\_MCPS](variables/MIN_COUNT_RATE_RTN_LIMIT_MCPS.md)
- [INTERMEASUREMENT\_MS](variables/INTERMEASUREMENT_MS.md)
- [THRESH\_HIGH](variables/THRESH_HIGH.md)
- [THRESH\_LOW](variables/THRESH_LOW.md)
- [SYSTEM\_\_INTERRUPT\_CLEAR](variables/SYSTEM__INTERRUPT_CLEAR.md)
- [SYSTEM\_START](variables/SYSTEM_START.md)
- [RESULT\_\_RANGE\_STATUS](variables/RESULT__RANGE_STATUS.md)
- [RESULT\_\_SPAD\_NB](variables/RESULT__SPAD_NB.md)
- [RESULT\_\_SIGNAL\_RATE](variables/RESULT__SIGNAL_RATE.md)
- [RESULT\_\_AMBIENT\_RATE](variables/RESULT__AMBIENT_RATE.md)
- [RESULT\_\_SIGMA](variables/RESULT__SIGMA.md)
- [RESULT\_\_DISTANCE](variables/RESULT__DISTANCE.md)
- [RESULT\_\_OSC\_CALIBRATE\_VAL](variables/RESULT__OSC_CALIBRATE_VAL.md)
- [FIRMWARE\_\_SYSTEM\_STATUS](variables/FIRMWARE__SYSTEM_STATUS.md)
- [IDENTIFICATION\_\_MODEL\_ID](variables/IDENTIFICATION__MODEL_ID.md)
- [MODEL\_ID\_VL53L4CD](variables/MODEL_ID_VL53L4CD.md)
- [WINDOW\_BELOW](variables/WINDOW_BELOW.md)
- [WINDOW\_ABOVE](variables/WINDOW_ABOVE.md)
- [WINDOW\_OUT](variables/WINDOW_OUT.md)
- [WINDOW\_IN](variables/WINDOW_IN.md)
- [CONFIG\_ADDR](variables/CONFIG_ADDR.md)
- [CONFIG\_END](variables/CONFIG_END.md)
- [DEFAULT\_CONFIGURATION](variables/DEFAULT_CONFIGURATION.md)
- [CONFIG\_FMP\_BYTE](variables/CONFIG_FMP_BYTE.md)
- [RESULT\_BLOCK\_ADDR](variables/RESULT_BLOCK_ADDR.md)
- [RESULT\_BLOCK\_LEN](variables/RESULT_BLOCK_LEN.md)
- [I2C\_KHZ\_BOOT](variables/I2C_KHZ_BOOT.md)
- [I2C\_KHZ\_DEFAULT](variables/I2C_KHZ_DEFAULT.md)
- [STATUS\_RTN](variables/STATUS_RTN.md)
- [RANGE\_STATUS\_NAMES](variables/RANGE_STATUS_NAMES.md)

## Functions

- [packVl53l4ReadReg](functions/packVl53l4ReadReg.md)
- [packVl53l4WriteReg](functions/packVl53l4WriteReg.md)
- [packVl53l4Xshut](functions/packVl53l4Xshut.md)
- [packVl53l4StartStream](functions/packVl53l4StartStream.md)
- [packVl53l4SetI2cSpeed](functions/packVl53l4SetI2cSpeed.md)
- [unpackVl53l4RegData](functions/unpackVl53l4RegData.md)
- [unpackVl53l4Info](functions/unpackVl53l4Info.md)
- [unpackVl53l4Stream](functions/unpackVl53l4Stream.md)
- [rangeStatusText](functions/rangeStatusText.md)
- [parseResultBlock](functions/parseResultBlock.md)
- [configBlock](functions/configBlock.md)
- [rangeTimingRegisters](functions/rangeTimingRegisters.md)
- [decodeRangeTiming](functions/decodeRangeTiming.md)
- [offsetRaw](functions/offsetRaw.md)
- [decodeOffset](functions/decodeOffset.md)
- [xtalkRaw](functions/xtalkRaw.md)
- [decodeXtalk](functions/decodeXtalk.md)
- [signalThresholdRaw](functions/signalThresholdRaw.md)
- [decodeSignalThreshold](functions/decodeSignalThreshold.md)
- [sigmaThresholdRaw](functions/sigmaThresholdRaw.md)
- [decodeSigmaThreshold](functions/decodeSigmaThreshold.md)

# Class: VL53L4CD

Defined in: [src/sensors/vl53l4/uld.ts:399](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L399)

Port of VL53L4CD_api.c + VL53L4CD_calibration.c (ULD 2.2.3).

## Constructors

### Constructor

```ts
new VL53L4CD(p): VL53L4CD;
```

Defined in: [src/sensors/vl53l4/uld.ts:400](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L400)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | [`Vl53l4Platform`](../interfaces/Vl53l4Platform.md) |

#### Returns

`VL53L4CD`

## Properties

### p

```ts
readonly p: Vl53l4Platform;
```

Defined in: [src/sensors/vl53l4/uld.ts:400](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L400)

## Methods

### rdByte()

```ts
rdByte(addr): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:404](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L404)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |

#### Returns

`Promise`\<`number`\>

***

### rdWord()

```ts
rdWord(addr): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:408](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L408)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |

#### Returns

`Promise`\<`number`\>

***

### rdDword()

```ts
rdDword(addr): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:413](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L413)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |

#### Returns

`Promise`\<`number`\>

***

### wrByte()

```ts
wrByte(addr, value): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:418](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L418)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `value` | `number` |

#### Returns

`Promise`\<`void`\>

***

### wrWord()

```ts
wrWord(addr, value): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:422](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L422)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `value` | `number` |

#### Returns

`Promise`\<`void`\>

***

### wrDword()

```ts
wrDword(addr, value): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:426](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L426)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `value` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getSensorId()

```ts
getSensorId(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:435](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L435)

#### Returns

`Promise`\<`number`\>

***

### isAlive()

```ts
isAlive(): Promise<boolean>;
```

Defined in: [src/sensors/vl53l4/uld.ts:439](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L439)

#### Returns

`Promise`\<`boolean`\>

***

### waitBoot()

```ts
waitBoot(timeoutMs?): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:445](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L445)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `1000` |

#### Returns

`Promise`\<`void`\>

***

### sensorInit()

```ts
sensorInit(busKhz?): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:461](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L461)

Initialise the sensor and leave the bus at `busKhz`.

The configuration block is written at I2C_KHZ_BOOT (400 kHz) because that
is the only speed an unconfigured sensor is specified for; the bridge is
re-timed to `busKhz` after it. A sensor reset just re-runs this whole
sequence.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `busKhz` | `number` | `I2C_KHZ_DEFAULT` |

#### Returns

`Promise`\<`void`\>

***

### clearInterrupt()

```ts
clearInterrupt(): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:485](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L485)

#### Returns

`Promise`\<`void`\>

***

### startRanging()

```ts
startRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:489](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L489)

#### Returns

`Promise`\<`void`\>

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:495](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L495)

#### Returns

`Promise`\<`void`\>

***

### checkForDataReady()

```ts
checkForDataReady(): Promise<boolean>;
```

Defined in: [src/sensors/vl53l4/uld.ts:499](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L499)

#### Returns

`Promise`\<`boolean`\>

***

### waitDataReady()

```ts
waitDataReady(timeoutMs?): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:504](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L504)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `1000` |

#### Returns

`Promise`\<`void`\>

***

### getResult()

```ts
getResult(): Promise<Vl53l4Results>;
```

Defined in: [src/sensors/vl53l4/uld.ts:516](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L516)

One block read instead of the C driver's six register reads — the sensor
auto-increments and the decoding is identical.

#### Returns

`Promise`\<[`Vl53l4Results`](../interfaces/Vl53l4Results.md)\>

***

### setRangeTiming()

```ts
setRangeTiming(timingBudgetMs, interMeasurementMs): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:522](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L522)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `timingBudgetMs` | `number` |
| `interMeasurementMs` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getRangeTiming()

```ts
getRangeTiming(): Promise<RangeTiming>;
```

Defined in: [src/sensors/vl53l4/uld.ts:535](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L535)

#### Returns

`Promise`\<[`RangeTiming`](../interfaces/RangeTiming.md)\>

***

### setOffset()

```ts
setOffset(offsetMm): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:546](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L546)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `offsetMm` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getOffset()

```ts
getOffset(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:552](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L552)

#### Returns

`Promise`\<`number`\>

***

### setXtalk()

```ts
setXtalk(xtalkKcps): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:558](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L558)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `xtalkKcps` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getXtalk()

```ts
getXtalk(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:564](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L564)

#### Returns

`Promise`\<`number`\>

***

### setDetectionThresholds()

```ts
setDetectionThresholds(
   distanceLowMm, 
   distanceHighMm, 
window): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:570](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L570)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `distanceLowMm` | `number` |
| `distanceHighMm` | `number` |
| `window` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getDetectionThresholds()

```ts
getDetectionThresholds(): Promise<{
  distanceLowMm: number;
  distanceHighMm: number;
  window: number;
}>;
```

Defined in: [src/sensors/vl53l4/uld.ts:580](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L580)

#### Returns

`Promise`\<\{
  `distanceLowMm`: `number`;
  `distanceHighMm`: `number`;
  `window`: `number`;
\}\>

***

### setSignalThreshold()

```ts
setSignalThreshold(signalKcps): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:591](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L591)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `signalKcps` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getSignalThreshold()

```ts
getSignalThreshold(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:595](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L595)

#### Returns

`Promise`\<`number`\>

***

### setSigmaThreshold()

```ts
setSigmaThreshold(sigmaMm): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:599](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L599)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `sigmaMm` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getSigmaThreshold()

```ts
getSigmaThreshold(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:603](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L603)

#### Returns

`Promise`\<`number`\>

***

### startTemperatureUpdate()

```ts
startTemperatureUpdate(): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:610](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L610)

Recommended after a >8 °C ambient change (ST Example_3).

#### Returns

`Promise`\<`void`\>

***

### calibrateOffset()

```ts
calibrateOffset(targetDistMm, nbSamples?): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:642](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L642)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `targetDistMm` | `number` | `undefined` |
| `nbSamples` | `number` | `20` |

#### Returns

`Promise`\<`number`\>

***

### calibrateXtalk()

```ts
calibrateXtalk(targetDistMm, nbSamples?): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:662](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L662)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `targetDistMm` | `number` | `undefined` |
| `nbSamples` | `number` | `20` |

#### Returns

`Promise`\<`number`\>

# Class: Vl53l4cd

Defined in: [src/sensors/vl53l4/vl53l4.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L98)

VL53L4CD single-zone ToF device.

`init()` runs the ULD boot sequence (no firmware blob — the sensor carries
its own), then configure and `startRanging()`. Configuration methods must
not be called while ranging: the INT-driven stream owns the register bank
(contract 10). Measurements stream via callbacks (`onMeasurement`) and/or
the pull iterator (`measurements()`).

## Extends

- `DepzDevice`

## Constructors

### Constructor

```ts
new Vl53l4cd(transport, opts?): Vl53l4cd;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:140](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L140)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | [`Vl53l4Options`](../interfaces/Vl53l4Options.md) |

#### Returns

`Vl53l4cd`

#### Overrides

```ts
DepzDevice.constructor
```

## Properties

### timeoutMs

```ts
timeoutMs: number;
```

Defined in: [src/device/device.ts:201](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L201)

#### Inherited from

```ts
DepzDevice.timeoutMs
```

***

### link

```ts
protected readonly link: DepzLink;
```

Defined in: [src/device/device.ts:203](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L203)

#### Inherited from

```ts
DepzDevice.link
```

## Accessors

### stats

#### Get Signature

```ts
get stats(): LinkStats;
```

Defined in: [src/device/device.ts:264](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L264)

##### Returns

`LinkStats`

#### Inherited from

```ts
DepzDevice.stats
```

***

### timeSync

#### Get Signature

```ts
get timeSync(): TimeSync | null;
```

Defined in: [src/device/device.ts:553](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L553)

##### Returns

`TimeSync` \| `null`

#### Inherited from

```ts
DepzDevice.timeSync
```

***

### uld

#### Get Signature

```ts
get uld(): VL53L4CD;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:149](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L149)

The underlying ULD driver (escape hatch for raw register access).

##### Returns

[`VL53L4CD`](VL53L4CD.md)

***

### initialized

#### Get Signature

```ts
get initialized(): boolean;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:157](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L157)

True after a successful init(). Cleared by resetSensor() and xshut() — a
power-cycled sensor holds none of the ULD configuration.

##### Returns

`boolean`

***

### ranging

#### Get Signature

```ts
get ranging(): boolean;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:377](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L377)

##### Returns

`boolean`

***

### streamParseErrors

#### Get Signature

```ts
get streamParseErrors(): number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:456](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L456)

Stream reports dropped because the result block failed to decode (short
block from a reconfigured stream, corrupt read).

##### Returns

`number`

***

### streamDroppedCounts

#### Get Signature

```ts
get streamDroppedCounts(): number[];
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:461](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L461)

Drop counters of all live measurement queues (diagnostics).

##### Returns

`number`[]

## Methods

### open()

```ts
open(): Promise<void>;
```

Defined in: [src/device/device.ts:230](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L230)

Open the transport and start the read pump. A no-op on a device that
continues an adopted connection (`attachSensorClass`) — already open.

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
DepzDevice.open
```

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/device/device.ts:237](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L237)

Close the connection. A device that handed its connection over owns none.

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
DepzDevice.close
```

***

### onTeardown()

```ts
protected onTeardown(error): void;
```

Defined in: [src/device/device.ts:288](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L288)

Hook for sensor subclasses: reject any subclass-managed in-flight requests
and clear per-connection state when the link closes. Runs once, after the
base `pending` map is failed. Default: no-op.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `error` | `Error` |

#### Returns

`void`

#### Inherited from

```ts
DepzDevice.onTeardown
```

***

### registerStream()

```ts
protected registerStream(stream): () => void;
```

Defined in: [src/device/device.ts:408](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L408)

Register a stream to be closed on device teardown.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `stream` | \{ `close`: `void`; \} |
| `stream.close` |

#### Returns

() => `void`

#### Inherited from

```ts
DepzDevice.registerStream
```

***

### onEvent()

```ts
onEvent(cb): () => void;
```

Defined in: [src/device/device.ts:421](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L421)

Subscribe to unsolicited/diagnostic events (read-pump context; do not
block). Returns an unsubscribe function.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`ev`) => `void` |

#### Returns

() => `void`

#### Inherited from

```ts
DepzDevice.onEvent
```

***

### emitEvent()

```ts
protected emitEvent(event): void;
```

Defined in: [src/device/device.ts:428](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L428)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `event` | `DeviceEvent` |

#### Returns

`void`

#### Inherited from

```ts
DepzDevice.emitEvent
```

***

### request()

```ts
request<T>(
   cmd, 
   payload?, 
opts?): Promise<T>;
```

Defined in: [src/device/device.ts:444](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L444)

Send `cmd` and wait for its correlated completion (contract 02 §1).

Exactly one of the completion paths must be configured:
- `okCompletes` — RPT_STATUS(cmd, OK) resolves with undefined;
- `matcher` — first packet for which the matcher returns non-`NO_MATCH`
  resolves with the matcher's return value.
Non-OK RPT_STATUS echoing `cmd` always rejects (BusyError for ERR_BUSY,
StatusError otherwise). One in-flight request per opcode.

#### Type Parameters

| Type Parameter | Default type |
| ------ | ------ |
| `T` | `void` |

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cmd` | `number` |
| `payload` | `Uint8Array` |
| `opts` | `RequestOptions`\<`T`\> |

#### Returns

`Promise`\<`T`\>

#### Inherited from

```ts
DepzDevice.request
```

***

### expectReport()

```ts
static expectReport<T>(reportId, unpack): Matcher<T>;
```

Defined in: [src/device/device.ts:488](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L488)

Matcher for a typed report identified by its report ID alone.

#### Type Parameters

| Type Parameter |
| ------ |
| `T` |

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `reportId` | `number` |
| `unpack` | (`payload`) => `T` |

#### Returns

`Matcher`\<`T`\>

#### Inherited from

```ts
DepzDevice.expectReport
```

***

### expectText()

```ts
static expectText(requestCmd): Matcher<string>;
```

Defined in: [src/device/device.ts:493](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L493)

Matcher for RPT_TEXT echoing `requestCmd`.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `requestCmd` | `number` |

#### Returns

`Matcher`\<`string`\>

#### Inherited from

```ts
DepzDevice.expectText
```

***

### getDeviceName()

```ts
getDeviceName(): Promise<string>;
```

Defined in: [src/device/device.ts:504](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L504)

#### Returns

`Promise`\<`string`\>

#### Inherited from

```ts
DepzDevice.getDeviceName
```

***

### getSoftwareName()

```ts
getSoftwareName(): Promise<string>;
```

Defined in: [src/device/device.ts:510](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L510)

#### Returns

`Promise`\<`string`\>

#### Inherited from

```ts
DepzDevice.getSoftwareName
```

***

### getSerialNumber()

```ts
getSerialNumber(): Promise<string>;
```

Defined in: [src/device/device.ts:516](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L516)

#### Returns

`Promise`\<`string`\>

#### Inherited from

```ts
DepzDevice.getSerialNumber
```

***

### identify()

```ts
identify(): Promise<Identity>;
```

Defined in: [src/device/device.ts:523](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L523)

Classify the running firmware (contract 02 §4).

#### Returns

`Promise`\<`Identity`\>

#### Inherited from

```ts
DepzDevice.identify
```

***

### readMcuTemperature()

```ts
readMcuTemperature(): Promise<number>;
```

Defined in: [src/device/device.ts:528](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L528)

Last cached MCU temperature in °C (device refreshes ~2 Hz).

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
DepzDevice.readMcuTemperature
```

***

### syncTime()

```ts
syncTime(samples?): Promise<TimeSync>;
```

Defined in: [src/device/device.ts:536](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L536)

NTP-style sync; keeps the lowest-RTT sample (contract 02 §5).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `samples` | `number` | `5` |

#### Returns

`Promise`\<`TimeSync`\>

#### Inherited from

```ts
DepzDevice.syncTime
```

***

### toHostTimeUs()

```ts
toHostTimeUs(deviceTsUs): bigint;
```

Defined in: [src/device/device.ts:558](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L558)

Device µs → host monotonic µs (requires a prior `syncTime`).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `deviceTsUs` | `bigint` |

#### Returns

`bigint`

#### Inherited from

```ts
DepzDevice.toHostTimeUs
```

***

### getReportPayloadCrc()

```ts
getReportPayloadCrc(): Promise<CrcType>;
```

Defined in: [src/device/device.ts:563](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L563)

#### Returns

`Promise`\<`CrcType`\>

#### Inherited from

```ts
DepzDevice.getReportPayloadCrc
```

***

### setReportPayloadCrc()

```ts
setReportPayloadCrc(crcType): Promise<void>;
```

Defined in: [src/device/device.ts:570](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L570)

Set the device→host payload CRC mode (host→device is per-packet).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `crcType` | `CrcType` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
DepzDevice.setReportPayloadCrc
```

***

### getSyncPin()

```ts
getSyncPin(pin): Promise<SyncPinConfig>;
```

Defined in: [src/device/device.ts:574](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L574)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pin` | `number` |

#### Returns

`Promise`\<`SyncPinConfig`\>

#### Inherited from

```ts
DepzDevice.getSyncPin
```

***

### setSyncPin()

```ts
setSyncPin(config): Promise<void>;
```

Defined in: [src/device/device.ts:580](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L580)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `config` | `SyncPinConfig` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
DepzDevice.setSyncPin
```

***

### reset()

```ts
reset(): Promise<void>;
```

Defined in: [src/device/device.ts:585](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L585)

DEVICE_RESET: device ACKs then reboots; the link will drop.

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
DepzDevice.reset
```

***

### enterBootloaderMode()

```ts
enterBootloaderMode(): Promise<void>;
```

Defined in: [src/device/device.ts:594](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L594)

Ask the device to reboot into the resident bootloader and close this
connection. Re-discovery/flash flow lives in the bootloader module
(contract 06).

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
DepzDevice.enterBootloaderMode
```

***

### isAlive()

```ts
isAlive(): Promise<boolean>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:162](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L162)

True when the sensor answers with the VL53L4CD model id (0xEBAA).

#### Returns

`Promise`\<`boolean`\>

***

### init()

```ts
init(busKhz?): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:176](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L176)

Initialise the sensor: default configuration block + VHV calibration
(ULD sensorInit). Takes well under a second; the bus is left at `busKhz`
(one of I2C_KHZ_STEPS).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `busKhz` | `number` | `I2C_KHZ_DEFAULT` |

#### Returns

`Promise`\<`void`\>

***

### xshut()

```ts
xshut(action): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L189)

Drive the XSHUT pin: XSHUT_OFF / XSHUT_ON / XSHUT_RESET. OFF and RESET
stop any active stream on the bridge; a power-cycled sensor needs init()
again.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `action` | `number` |

#### Returns

`Promise`\<`void`\>

***

### resetSensor()

```ts
resetSensor(): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:200](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L200)

Hardware sensor reset via XSHUT (blocks ~3 ms on the MCU). The ULD
configuration is wiped — call init() again.

#### Returns

`Promise`\<`void`\>

***

### bridgeInfo()

```ts
bridgeInfo(): Promise<Vl53l4Info>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:211](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L211)

RPT_VL53_INFO: sensor identity, pin levels and bridge counters. Counters
are free-running (wrap silently) — watch increments. Safe to call while
streaming.

#### Returns

`Promise`\<[`Vl53l4Info`](../interfaces/Vl53l4Info.md)\>

***

### setI2cSpeedKhz()

```ts
setI2cSpeedKhz(khz): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:222](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L222)

Re-time the bridge's I2C bus to the nominal step nearest `khz`
(I2C_KHZ_STEPS). Not while ranging — re-timing refuses a transfer in
flight (ERR_BUSY). Read back the programmed step via bridgeInfo().

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `khz` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getRangeTiming()

```ts
getRangeTiming(): Promise<RangeTiming>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:233](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L233)

→ { timingBudgetMs, interMeasurementMs }. interMeasurement 0 means
continuous mode.

#### Returns

`Promise`\<[`RangeTiming`](../interfaces/RangeTiming.md)\>

***

### setRangeTiming()

```ts
setRangeTiming(timingBudgetMs, interMeasurementMs?): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:242](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L242)

Set the timing budget (10–200 ms) and inter-measurement period.
`interMeasurementMs = 0` selects continuous ranging; a value larger than
the budget selects autonomous low-power mode. Not while ranging.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timingBudgetMs` | `number` | `undefined` |
| `interMeasurementMs` | `number` | `0` |

#### Returns

`Promise`\<`void`\>

***

### getOffsetMm()

```ts
getOffsetMm(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:248](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L248)

Configured ranging offset in mm (signed).

#### Returns

`Promise`\<`number`\>

***

### setOffsetMm()

```ts
setOffsetMm(offsetMm): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:253](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L253)

Set the ranging offset correction in mm. Not while ranging.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `offsetMm` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getXtalkKcps()

```ts
getXtalkKcps(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:259](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L259)

Configured crosstalk compensation in kcps (0 = disabled).

#### Returns

`Promise`\<`number`\>

***

### setXtalkKcps()

```ts
setXtalkKcps(xtalkKcps): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:264](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L264)

Set the crosstalk compensation in kcps. Not while ranging.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `xtalkKcps` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getDetectionThresholds()

```ts
getDetectionThresholds(): Promise<{
  distanceLowMm: number;
  distanceHighMm: number;
  window: number;
}>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:273](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L273)

→ { distanceLowMm, distanceHighMm, window }. Window is one of
WINDOW_BELOW / WINDOW_ABOVE / WINDOW_OUT / WINDOW_IN.

#### Returns

`Promise`\<\{
  `distanceLowMm`: `number`;
  `distanceHighMm`: `number`;
  `window`: `number`;
\}\>

***

### setDetectionThresholds()

```ts
setDetectionThresholds(
   distanceLowMm, 
   distanceHighMm, 
window): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:285](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L285)

Program the distance-window interrupt (INT only fires when the window
condition holds). Not while ranging.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `distanceLowMm` | `number` |
| `distanceHighMm` | `number` |
| `window` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getSignalThresholdKcps()

```ts
getSignalThresholdKcps(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:294](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L294)

#### Returns

`Promise`\<`number`\>

***

### setSignalThresholdKcps()

```ts
setSignalThresholdKcps(signalKcps): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:302](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L302)

Discard measurements whose return signal is below `signalKcps`. Not
while ranging.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `signalKcps` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getSigmaThresholdMm()

```ts
getSigmaThresholdMm(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:307](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L307)

#### Returns

`Promise`\<`number`\>

***

### setSigmaThresholdMm()

```ts
setSigmaThresholdMm(sigmaMm): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:315](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L315)

Discard measurements whose sigma exceeds `sigmaMm` (≤ 16383). Not while
ranging.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `sigmaMm` | `number` |

#### Returns

`Promise`\<`void`\>

***

### startTemperatureUpdate()

```ts
startTemperatureUpdate(): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:324](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L324)

Re-run VHV calibration; recommended after a >8 °C ambient change. Not
while ranging (runs a short ranging burst internally).

#### Returns

`Promise`\<`void`\>

***

### calibrateOffset()

```ts
calibrateOffset(targetDistMm, nbSamples?): Promise<number>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:333](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L333)

Offset calibration against a target at `targetDistMm` (10–1000). Blocks
for the sample burst; returns the offset now programmed.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `targetDistMm` | `number` | `undefined` |
| `nbSamples` | `number` | `20` |

#### Returns

`Promise`\<`number`\>

***

### calibrateXtalk()

```ts
calibrateXtalk(targetDistMm, nbSamples?): Promise<number>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:342](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L342)

Crosstalk calibration against a target at `targetDistMm` (10–5000).
Blocks for the sample burst; returns the xtalk now programmed (kcps).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `targetDistMm` | `number` | `undefined` |
| `nbSamples` | `number` | `20` |

#### Returns

`Promise`\<`number`\>

***

### startRanging()

```ts
startRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:353](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L353)

Start the sensor's ranging loop and arm the MCU stream: one
RPT_VL53_STREAM per INT edge carrying the 17-byte result block.

#### Returns

`Promise`\<`void`\>

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:364](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L364)

#### Returns

`Promise`\<`void`\>

***

### measureOnce()

```ts
measureOnce(timeoutMs?): Promise<Vl53l4Measurement>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:385](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L385)

Single poll-mode measurement: start ranging, wait for data-ready, read
the result block, stop. Rejects while the stream is running.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `1000` |

#### Returns

`Promise`\<[`Vl53l4Measurement`](../interfaces/Vl53l4Measurement.md)\>

***

### onMeasurement()

```ts
onMeasurement(cb): () => void;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:403](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L403)

Subscribe to streamed measurements (read-pump context; don't block).
Returns an unsubscribe function.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`m`) => `void` |

#### Returns

() => `void`

***

### measurements()

```ts
measurements(maxsize?): StreamQueue<Vl53l4Measurement>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:415](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L415)

Async iterator over measurements — bounded, drop-oldest (contract 07
§3). The returned queue exposes `droppedCount`; it ends when the device
closes or the consumer breaks out of iteration.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `maxsize` | `number` | `64` |

#### Returns

`StreamQueue`\<[`Vl53l4Measurement`](../interfaces/Vl53l4Measurement.md)\>

***

### getMeasurement()

```ts
getMeasurement(timeoutMs?): Promise<Vl53l4Measurement>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:426](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L426)

Convenience: wait for the next streamed measurement.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `2000` |

#### Returns

`Promise`\<[`Vl53l4Measurement`](../interfaces/Vl53l4Measurement.md)\>

***

### requireNotRanging()

```ts
protected requireNotRanging(): void;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:467](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L467)

#### Returns

`void`

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:473](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L473)

Hook for sensor subclasses: route streaming reports here. Return true
when the packet was consumed. Runs after status/matcher/common-report
routing, on the read-pump context.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pkt` | `PacketEvent` |

#### Returns

`boolean`

#### Overrides

```ts
DepzDevice.handleReport
```

# Class: Vl53l4cdError

Defined in: [src/sensors/vl53l4/uld.ts:135](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L135)

## Extends

- `Error`

## Constructors

### Constructor

```ts
new Vl53l4cdError(message?): Vl53l4cdError;
```

Defined in: [src/sensors/vl53l4/uld.ts:136](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L136)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `message?` | `string` |

#### Returns

`Vl53l4cdError`

#### Overrides

```ts
Error.constructor
```

## Properties

### stackTraceLimit

```ts
static stackTraceLimit: number;
```

Defined in: node\_modules/@types/node/globals.d.ts:68

The `Error.stackTraceLimit` property specifies the number of stack frames
collected by a stack trace (whether generated by `new Error().stack` or
`Error.captureStackTrace(obj)`).

The default value is `10` but may be set to any valid JavaScript number. Changes
will affect any stack trace captured _after_ the value has been changed.

If set to a non-number value, or set to a negative number, stack traces will
not capture any frames.

#### Inherited from

```ts
Error.stackTraceLimit
```

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

```ts
Error.cause
```

***

### name

```ts
name: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

```ts
Error.name
```

***

### message

```ts
message: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

```ts
Error.message
```

***

### stack?

```ts
optional stack?: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

```ts
Error.stack
```

## Methods

### captureStackTrace()

```ts
static captureStackTrace(targetObject, constructorOpt?): void;
```

Defined in: node\_modules/@types/node/globals.d.ts:52

Creates a `.stack` property on `targetObject`, which when accessed returns
a string representing the location in the code at which
`Error.captureStackTrace()` was called.

```js
const myObject = {};
Error.captureStackTrace(myObject);
myObject.stack;  // Similar to `new Error().stack`
```

The first line of the trace will be prefixed with
`${myObject.name}: ${myObject.message}`.

The optional `constructorOpt` argument accepts a function. If given, all frames
above `constructorOpt`, including `constructorOpt`, will be omitted from the
generated stack trace.

The `constructorOpt` argument is useful for hiding implementation
details of error generation from the user. For instance:

```js
function a() {
  b();
}

function b() {
  c();
}

function c() {
  // Create an error without stack trace to avoid calculating the stack trace twice.
  const { stackTraceLimit } = Error;
  Error.stackTraceLimit = 0;
  const error = new Error();
  Error.stackTraceLimit = stackTraceLimit;

  // Capture the stack trace above function b
  Error.captureStackTrace(error, b); // Neither function c, nor b is included in the stack trace
  throw error;
}

a();
```

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetObject` | `object` |
| `constructorOpt?` | `Function` |

#### Returns

`void`

#### Inherited from

```ts
Error.captureStackTrace
```

***

### prepareStackTrace()

```ts
static prepareStackTrace(err, stackTraces): any;
```

Defined in: node\_modules/@types/node/globals.d.ts:56

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `err` | `Error` |
| `stackTraces` | `CallSite`[] |

#### Returns

`any`

#### See

https://v8.dev/docs/stack-trace-api#customizing-stack-traces

#### Inherited from

```ts
Error.prepareStackTrace
```

# Enumeration: Vl53l4Cmd

Defined in: [src/protocol/vl53l4.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L8)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
host-side ULD driver (`VL53L4CD` + codecs from `sensors/vl53l4/uld`), and
the register-bridge wire codecs (`protocol/vl53l4`, re-exported under the
same `Vl53l4*`-prefixed names `src/index.ts` uses) — so TypeDoc documents
that sensor in isolation.

Not part of the shipped package — a doc-generation entry point only.

## Enumeration Members

### ReadReg

```ts
ReadReg: 50;
```

Defined in: [src/protocol/vl53l4.ts:9](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L9)

***

### WriteReg

```ts
WriteReg: 51;
```

Defined in: [src/protocol/vl53l4.ts:10](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L10)

***

### Xshut

```ts
Xshut: 52;
```

Defined in: [src/protocol/vl53l4.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L11)

***

### StartStream

```ts
StartStream: 53;
```

Defined in: [src/protocol/vl53l4.ts:12](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L12)

***

### StopStream

```ts
StopStream: 54;
```

Defined in: [src/protocol/vl53l4.ts:13](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L13)

***

### GetInfo

```ts
GetInfo: 55;
```

Defined in: [src/protocol/vl53l4.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L14)

***

### SetI2cSpeed

```ts
SetI2cSpeed: 56;
```

Defined in: [src/protocol/vl53l4.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L15)

# Enumeration: Vl53l4Rpt

Defined in: [src/protocol/vl53l4.ts:18](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L18)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
host-side ULD driver (`VL53L4CD` + codecs from `sensors/vl53l4/uld`), and
the register-bridge wire codecs (`protocol/vl53l4`, re-exported under the
same `Vl53l4*`-prefixed names `src/index.ts` uses) — so TypeDoc documents
that sensor in isolation.

Not part of the shipped package — a doc-generation entry point only.

## Enumeration Members

### RegData

```ts
RegData: 145;
```

Defined in: [src/protocol/vl53l4.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L19)

***

### Info

```ts
Info: 146;
```

Defined in: [src/protocol/vl53l4.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L20)

***

### Stream

```ts
Stream: 147;
```

Defined in: [src/protocol/vl53l4.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L21)

# Function: configBlock()

```ts
function configBlock(): Uint8Array;
```

Defined in: [src/sensors/vl53l4/uld.ts:219](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L219)

The 91-byte block sensorInit() writes at CONFIG_ADDR: the ST default
configuration with byte 0 forced to CONFIG_FMP_BYTE (Fast Mode Plus).

## Returns

`Uint8Array`

# Function: decodeOffset()

```ts
function decodeOffset(rawWord): number;
```

Defined in: [src/sensors/vl53l4/uld.ts:348](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L348)

getOffset: RANGE_OFFSET_MM word → signed millimetres.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `rawWord` | `number` |

## Returns

`number`

# Function: decodeRangeTiming()

```ts
function decodeRangeTiming(
   intermeasurementRaw, 
   clockPll, 
   oscFrequency, 
   rangeConfigA): RangeTiming;
```

Defined in: [src/sensors/vl53l4/uld.ts:309](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L309)

GetRangeTiming register math → timing budget and inter-measurement period.

Inputs are the raw register reads: INTERMEASUREMENT_MS dword, the
RESULT__OSC_CALIBRATE_VAL word, the 0x0006 word and the RANGE_CONFIG_A
word.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `intermeasurementRaw` | `number` |
| `clockPll` | `number` |
| `oscFrequency` | `number` |
| `rangeConfigA` | `number` |

## Returns

[`RangeTiming`](../interfaces/RangeTiming.md)

# Function: decodeSigmaThreshold()

```ts
function decodeSigmaThreshold(rawWord): number;
```

Defined in: [src/sensors/vl53l4/uld.ts:382](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L382)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `rawWord` | `number` |

## Returns

`number`

# Function: decodeSignalThreshold()

```ts
function decodeSignalThreshold(rawWord): number;
```

Defined in: [src/sensors/vl53l4/uld.ts:369](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L369)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `rawWord` | `number` |

## Returns

`number`

# Function: decodeXtalk()

```ts
function decodeXtalk(rawWord): number;
```

Defined in: [src/sensors/vl53l4/uld.ts:360](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L360)

getXtalk: XTALK_PLANE_OFFSET_KCPS word → kcps.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `rawWord` | `number` |

## Returns

`number`

# Function: offsetRaw()

```ts
function offsetRaw(offsetMm): number;
```

Defined in: [src/sensors/vl53l4/uld.ts:342](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L342)

RANGE_OFFSET_MM word for setOffset (INNER/OUTER are zeroed alongside).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `offsetMm` | `number` |

## Returns

`number`

# Function: packVl53l4ReadReg()

```ts
function packVl53l4ReadReg(addr, length): Uint8Array;
```

Defined in: [src/protocol/vl53l4.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L46)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
host-side ULD driver (`VL53L4CD` + codecs from `sensors/vl53l4/uld`), and
the register-bridge wire codecs (`protocol/vl53l4`, re-exported under the
same `Vl53l4*`-prefixed names `src/index.ts` uses) — so TypeDoc documents
that sensor in isolation.

Not part of the shipped package — a doc-generation entry point only.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `length` | `number` |

## Returns

`Uint8Array`

# Function: packVl53l4SetI2cSpeed()

```ts
function packVl53l4SetI2cSpeed(khz): Uint8Array;
```

Defined in: [src/protocol/vl53l4.ts:81](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L81)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
host-side ULD driver (`VL53L4CD` + codecs from `sensors/vl53l4/uld`), and
the register-bridge wire codecs (`protocol/vl53l4`, re-exported under the
same `Vl53l4*`-prefixed names `src/index.ts` uses) — so TypeDoc documents
that sensor in isolation.

Not part of the shipped package — a doc-generation entry point only.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `khz` | `number` |

## Returns

`Uint8Array`

# Function: packVl53l4StartStream()

```ts
function packVl53l4StartStream(
   addr, 
   length, 
   flags?): Uint8Array;
```

Defined in: [src/protocol/vl53l4.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L72)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
host-side ULD driver (`VL53L4CD` + codecs from `sensors/vl53l4/uld`), and
the register-bridge wire codecs (`protocol/vl53l4`, re-exported under the
same `Vl53l4*`-prefixed names `src/index.ts` uses) — so TypeDoc documents
that sensor in isolation.

Not part of the shipped package — a doc-generation entry point only.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `addr` | `number` | `undefined` |
| `length` | `number` | `undefined` |
| `flags` | `number` | `0` |

## Returns

`Uint8Array`

# Function: packVl53l4WriteReg()

```ts
function packVl53l4WriteReg(addr, data): Uint8Array;
```

Defined in: [src/protocol/vl53l4.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L57)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
host-side ULD driver (`VL53L4CD` + codecs from `sensors/vl53l4/uld`), and
the register-bridge wire codecs (`protocol/vl53l4`, re-exported under the
same `Vl53l4*`-prefixed names `src/index.ts` uses) — so TypeDoc documents
that sensor in isolation.

Not part of the shipped package — a doc-generation entry point only.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `data` | `Uint8Array` |

## Returns

`Uint8Array`

# Function: packVl53l4Xshut()

```ts
function packVl53l4Xshut(action): Uint8Array;
```

Defined in: [src/protocol/vl53l4.ts:68](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L68)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
host-side ULD driver (`VL53L4CD` + codecs from `sensors/vl53l4/uld`), and
the register-bridge wire codecs (`protocol/vl53l4`, re-exported under the
same `Vl53l4*`-prefixed names `src/index.ts` uses) — so TypeDoc documents
that sensor in isolation.

Not part of the shipped package — a doc-generation entry point only.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `action` | `number` |

## Returns

`Uint8Array`

# Function: parseResultBlock()

```ts
function parseResultBlock(raw): Vl53l4Results;
```

Defined in: [src/sensors/vl53l4/uld.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L188)

Decode the streamed 0x0089..0x0099 block exactly as VL53L4CD_GetResult()
decodes the same registers read one by one. Register contents are
big-endian words (the bridge passes them through untouched).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `raw` | `Uint8Array` |

## Returns

[`Vl53l4Results`](../interfaces/Vl53l4Results.md)

# Function: rangeStatusText()

```ts
function rangeStatusText(rangeStatus): string;
```

Defined in: [src/sensors/vl53l4/uld.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L170)

Human-readable text for a decoded range status.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `rangeStatus` | `number` |

## Returns

`string`

# Function: rangeTimingRegisters()

```ts
function rangeTimingRegisters(
   timingBudgetMs, 
   interMeasurementMs, 
   oscFrequency, 
   clockPll?): RangeTimingRegisters;
```

Defined in: [src/sensors/vl53l4/uld.ts:241](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L241)

SetRangeTiming register math → RANGE_CONFIG_A, RANGE_CONFIG_B and the
INTERMEASUREMENT_MS raw dword.

`oscFrequency` is the word read from 0x0006; `clockPll` is the word read
from RESULT__OSC_CALIBRATE_VAL (used only in autonomous mode, i.e. when
`interMeasurementMs > 0`).

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timingBudgetMs` | `number` | `undefined` |
| `interMeasurementMs` | `number` | `undefined` |
| `oscFrequency` | `number` | `undefined` |
| `clockPll` | `number` | `0` |

## Returns

[`RangeTimingRegisters`](../interfaces/RangeTimingRegisters.md)

# Function: sigmaThresholdRaw()

```ts
function sigmaThresholdRaw(sigmaMm): number;
```

Defined in: [src/sensors/vl53l4/uld.ts:374](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L374)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `sigmaMm` | `number` |

## Returns

`number`

# Function: signalThresholdRaw()

```ts
function signalThresholdRaw(signalKcps): number;
```

Defined in: [src/sensors/vl53l4/uld.ts:365](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L365)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `signalKcps` | `number` |

## Returns

`number`

# Function: unpackVl53l4Info()

```ts
function unpackVl53l4Info(payload): Vl53l4Info;
```

Defined in: [src/protocol/vl53l4.ts:133](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L133)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
host-side ULD driver (`VL53L4CD` + codecs from `sensors/vl53l4/uld`), and
the register-bridge wire codecs (`protocol/vl53l4`, re-exported under the
same `Vl53l4*`-prefixed names `src/index.ts` uses) — so TypeDoc documents
that sensor in isolation.

Not part of the shipped package — a doc-generation entry point only.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Vl53l4Info`](../interfaces/Vl53l4Info.md)

# Function: unpackVl53l4RegData()

```ts
function unpackVl53l4RegData(payload): Vl53l4RegData;
```

Defined in: [src/protocol/vl53l4.ts:96](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L96)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
host-side ULD driver (`VL53L4CD` + codecs from `sensors/vl53l4/uld`), and
the register-bridge wire codecs (`protocol/vl53l4`, re-exported under the
same `Vl53l4*`-prefixed names `src/index.ts` uses) — so TypeDoc documents
that sensor in isolation.

Not part of the shipped package — a doc-generation entry point only.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Vl53l4RegData`](../interfaces/Vl53l4RegData.md)

# Function: unpackVl53l4Stream()

```ts
function unpackVl53l4Stream(payload): Vl53l4StreamData;
```

Defined in: [src/protocol/vl53l4.ts:161](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L161)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
host-side ULD driver (`VL53L4CD` + codecs from `sensors/vl53l4/uld`), and
the register-bridge wire codecs (`protocol/vl53l4`, re-exported under the
same `Vl53l4*`-prefixed names `src/index.ts` uses) — so TypeDoc documents
that sensor in isolation.

Not part of the shipped package — a doc-generation entry point only.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Vl53l4StreamData`](../interfaces/Vl53l4StreamData.md)

# Function: xtalkRaw()

```ts
function xtalkRaw(xtalkKcps): number;
```

Defined in: [src/sensors/vl53l4/uld.ts:355](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L355)

XTALK_PLANE_OFFSET_KCPS word for setXtalk.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `xtalkKcps` | `number` |

## Returns

`number`

# Interface: RangeTiming

Defined in: [src/sensors/vl53l4/uld.ts:297](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L297)

GetRangeTiming decode result.

## Properties

### timingBudgetMs

```ts
timingBudgetMs: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:298](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L298)

***

### interMeasurementMs

```ts
interMeasurementMs: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:299](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L299)

# Interface: RangeTimingRegisters

Defined in: [src/sensors/vl53l4/uld.ts:226](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L226)

SetRangeTiming register words.

## Properties

### rangeConfigA

```ts
rangeConfigA: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:227](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L227)

***

### rangeConfigB

```ts
rangeConfigB: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:228](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L228)

***

### intermeasurementRaw

```ts
intermeasurementRaw: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:230](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L230)

INTERMEASUREMENT_MS raw dword.

# Interface: Vl53l4Info

Defined in: [src/protocol/vl53l4.ts:117](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L117)

RPT_VL53_INFO — bridge diagnostics. Counters are free-running and wrap
silently; watch increments, not absolute values.

## Properties

### intEdges

```ts
intEdges: number;
```

Defined in: [src/protocol/vl53l4.ts:118](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L118)

***

### slotsSkipped

```ts
slotsSkipped: number;
```

Defined in: [src/protocol/vl53l4.ts:119](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L119)

***

### i2cErrors

```ts
i2cErrors: number;
```

Defined in: [src/protocol/vl53l4.ts:120](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L120)

***

### lastI2cError

```ts
lastI2cError: number;
```

Defined in: [src/protocol/vl53l4.ts:121](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L121)

***

### modelId

```ts
modelId: number;
```

Defined in: [src/protocol/vl53l4.ts:123](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L123)

0x010F..0x0110 — expected 0xEBAA.

***

### fwStatus

```ts
fwStatus: number;
```

Defined in: [src/protocol/vl53l4.ts:125](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L125)

0x00E5 — expected 0x03 (booted).

***

### initialized

```ts
initialized: number;
```

Defined in: [src/protocol/vl53l4.ts:127](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L127)

1 = MODEL_ID matched on this read.

***

### xshutLevel

```ts
xshutLevel: number;
```

Defined in: [src/protocol/vl53l4.ts:128](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L128)

***

### intLevel

```ts
intLevel: number;
```

Defined in: [src/protocol/vl53l4.ts:129](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L129)

***

### i2cKhz

```ts
i2cKhz: number;
```

Defined in: [src/protocol/vl53l4.ts:130](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L130)

# Interface: Vl53l4Measurement

Defined in: [src/sensors/vl53l4/vl53l4.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L47)

One decoded ranging result (VL53L4CD_ResultsData_t + MCU timestamp).

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L49)

MCU uptime at the INT edge (stream) / read (poll).

***

### rangeStatus

```ts
rangeStatus: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:51](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L51)

0 = valid (RANGE_STATUS_NAMES).

***

### distanceMm

```ts
distanceMm: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L52)

***

### sigmaMm

```ts
sigmaMm: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L53)

***

### signalRateKcps

```ts
signalRateKcps: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L54)

***

### ambientRateKcps

```ts
ambientRateKcps: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L55)

***

### signalPerSpadKcps

```ts
signalPerSpadKcps: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L56)

***

### ambientPerSpadKcps

```ts
ambientPerSpadKcps: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L57)

***

### numberOfSpad

```ts
numberOfSpad: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:58](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L58)

***

### streamCount

```ts
streamCount: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L60)

Sensor frame counter, wraps at 255.

***

### valid

```ts
valid: boolean;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:62](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L62)

rangeStatus === 0.

***

### statusText

```ts
statusText: string;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L64)

Human-readable range status.

# Interface: Vl53l4Options

Defined in: [src/sensors/vl53l4/vl53l4.ts:84](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L84)

## Extends

- `DeviceOptions`

## Properties

### timeoutMs?

```ts
optional timeoutMs?: number;
```

Defined in: [src/device/device.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L170)

#### Inherited from

```ts
DeviceOptions.timeoutMs
```

***

### txCrcType?

```ts
optional txCrcType?: CrcType;
```

Defined in: [src/device/device.ts:171](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L171)

#### Inherited from

```ts
DeviceOptions.txCrcType
```

***

### sleepImpl?

```ts
optional sleepImpl?: (ms) => Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:86](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L86)

ULD sleep implementation (tests inject an instant one).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Interface: Vl53l4Platform

Defined in: [src/sensors/vl53l4/uld.ts:143](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L143)

What the ULD needs from the register bridge.

## Methods

### rdMulti()

```ts
rdMulti(addr, size): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/sensors/vl53l4/uld.ts:144](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L144)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `size` | `number` |

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### wrMulti()

```ts
wrMulti(addr, data): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:145](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L145)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `data` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

***

### setI2cSpeed()

```ts
setI2cSpeed(khz): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:146](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L146)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `khz` | `number` |

#### Returns

`Promise`\<`void`\>

***

### sleepMs()

```ts
sleepMs(ms): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:147](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L147)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Interface: Vl53l4RegData

Defined in: [src/protocol/vl53l4.ts:88](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L88)

RPT_VL53_REG_DATA payload.

## Properties

### cmd

```ts
cmd: number;
```

Defined in: [src/protocol/vl53l4.ts:90](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L90)

Echoed READ_REG opcode.

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/protocol/vl53l4.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L92)

MCU uptime at I2C-read completion.

***

### data

```ts
data: Uint8Array;
```

Defined in: [src/protocol/vl53l4.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L93)

# Interface: Vl53l4Results

Defined in: [src/sensors/vl53l4/uld.ts:151](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L151)

VL53L4CD_ResultsData_t plus the sensor's own frame counter.

## Properties

### rangeStatus

```ts
rangeStatus: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:153](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L153)

0 = valid (RANGE_STATUS_NAMES).

***

### distanceMm

```ts
distanceMm: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:154](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L154)

***

### ambientRateKcps

```ts
ambientRateKcps: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:155](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L155)

***

### ambientPerSpadKcps

```ts
ambientPerSpadKcps: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:156](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L156)

***

### signalRateKcps

```ts
signalRateKcps: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:157](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L157)

***

### signalPerSpadKcps

```ts
signalPerSpadKcps: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:158](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L158)

***

### numberOfSpad

```ts
numberOfSpad: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:159](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L159)

***

### sigmaMm

```ts
sigmaMm: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:160](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L160)

***

### streamCount

```ts
streamCount: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:166](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L166)

0x008B RESULT__STREAM_COUNT: wraps at 255. The C ULD ignores it; it is
what tells a frame the host never received from one the sensor never
produced.

# Interface: Vl53l4StreamData

Defined in: [src/protocol/vl53l4.ts:153](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L153)

RPT_VL53_STREAM — one streamed register block. `addr`/`length` echo the
stream configuration so each report is self-describing.

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/protocol/vl53l4.ts:155](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L155)

MCU uptime at the INT edge (the sensor event).

***

### addr

```ts
addr: number;
```

Defined in: [src/protocol/vl53l4.ts:156](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L156)

***

### length

```ts
length: number;
```

Defined in: [src/protocol/vl53l4.ts:157](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L157)

***

### data

```ts
data: Uint8Array;
```

Defined in: [src/protocol/vl53l4.ts:158](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L158)

# Variable: CONFIG\_ADDR

```ts
const CONFIG_ADDR: 45 = 0x002d;
```

Defined in: [src/sensors/vl53l4/uld.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L63)

# Variable: CONFIG\_END

```ts
const CONFIG_END: 135 = 0x0087;
```

Defined in: [src/sensors/vl53l4/uld.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L64)

# Variable: CONFIG\_FMP\_BYTE

```ts
const CONFIG_FMP_BYTE: 18 = 0x12;
```

Defined in: [src/sensors/vl53l4/uld.ts:90](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L90)

# Variable: DEFAULT\_CONFIGURATION

```ts
const DEFAULT_CONFIGURATION: Uint8Array;
```

Defined in: [src/sensors/vl53l4/uld.ts:75](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L75)

VL53L4CD_DEFAULT_CONFIGURATION[] — 91 bytes, registers 0x2D..0x87.
`configBlock()` always overrides byte 0 (register 0x2D) with
CONFIG_FMP_BYTE (0x12) to put the sensor's I2C pad in Fast Mode Plus —
exactly what VL53L4CD_I2C_FAST_MODE_PLUS does in the C ULD. FM+ pads work
at every bus step down to 100 kHz, so it is set unconditionally and never
cleared (clearing it mid-block NACKs and truncates the write).

# Variable: FIRMWARE\_\_SYSTEM\_STATUS

```ts
const FIRMWARE__SYSTEM_STATUS: 229 = 0x00e5;
```

Defined in: [src/sensors/vl53l4/uld.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L52)

# Variable: GPIO\_HV\_MUX\_\_CTRL

```ts
const GPIO_HV_MUX__CTRL: 48 = 0x0030;
```

Defined in: [src/sensors/vl53l4/uld.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L33)

# Variable: GPIO\_\_TIO\_HV\_STATUS

```ts
const GPIO__TIO_HV_STATUS: 49 = 0x0031;
```

Defined in: [src/sensors/vl53l4/uld.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L34)

# Variable: I2C\_KHZ\_BOOT

```ts
const I2C_KHZ_BOOT: 400 = 400;
```

Defined in: [src/sensors/vl53l4/uld.ts:100](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L100)

# Variable: I2C\_KHZ\_DEFAULT

```ts
const I2C_KHZ_DEFAULT: 1000 = 1000;
```

Defined in: [src/sensors/vl53l4/uld.ts:101](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L101)

# Variable: I2C\_SLAVE\_\_DEVICE\_ADDRESS

```ts
const I2C_SLAVE__DEVICE_ADDRESS: 1 = 0x0001;
```

Defined in: [src/sensors/vl53l4/uld.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L23)

# Variable: IDENTIFICATION\_\_MODEL\_ID

```ts
const IDENTIFICATION__MODEL_ID: 271 = 0x010f;
```

Defined in: [src/sensors/vl53l4/uld.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L53)

# Variable: INNER\_OFFSET\_MM

```ts
const INNER_OFFSET_MM: 32 = 0x0020;
```

Defined in: [src/sensors/vl53l4/uld.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L31)

# Variable: INTERMEASUREMENT\_MS

```ts
const INTERMEASUREMENT_MS: 108 = 0x006c;
```

Defined in: [src/sensors/vl53l4/uld.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L40)

# Variable: MIN\_COUNT\_RATE\_RTN\_LIMIT\_MCPS

```ts
const MIN_COUNT_RATE_RTN_LIMIT_MCPS: 102 = 0x0066;
```

Defined in: [src/sensors/vl53l4/uld.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L39)

# Variable: MODEL\_ID\_VL53L4CD

```ts
const MODEL_ID_VL53L4CD: 60330 = 0xebaa;
```

Defined in: [src/sensors/vl53l4/uld.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L55)

# Variable: OSC\_FREQUENCY

```ts
const OSC_FREQUENCY: 6 = 0x0006;
```

Defined in: [src/sensors/vl53l4/uld.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L25)

Unnamed in the C driver.

# Variable: OUTER\_OFFSET\_MM

```ts
const OUTER_OFFSET_MM: 34 = 0x0022;
```

Defined in: [src/sensors/vl53l4/uld.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L32)

# Variable: RANGE\_CONFIG\_A

```ts
const RANGE_CONFIG_A: 94 = 0x005e;
```

Defined in: [src/sensors/vl53l4/uld.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L36)

# Variable: RANGE\_CONFIG\_B

```ts
const RANGE_CONFIG_B: 97 = 0x0061;
```

Defined in: [src/sensors/vl53l4/uld.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L37)

# Variable: RANGE\_CONFIG\_\_SIGMA\_THRESH

```ts
const RANGE_CONFIG__SIGMA_THRESH: 100 = 0x0064;
```

Defined in: [src/sensors/vl53l4/uld.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L38)

# Variable: RANGE\_OFFSET\_MM

```ts
const RANGE_OFFSET_MM: 30 = 0x001e;
```

Defined in: [src/sensors/vl53l4/uld.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L30)

# Variable: RANGE\_STATUS\_NAMES

```ts
const RANGE_STATUS_NAMES: Readonly<Record<number, string>>;
```

Defined in: [src/sensors/vl53l4/uld.ts:112](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L112)

UM2931, "Range status description".

# Variable: RESULT\_BLOCK\_ADDR

```ts
const RESULT_BLOCK_ADDR: 137 = RESULT__RANGE_STATUS;
```

Defined in: [src/sensors/vl53l4/uld.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L94)

# Variable: RESULT\_BLOCK\_LEN

```ts
const RESULT_BLOCK_LEN: 17 = 17;
```

Defined in: [src/sensors/vl53l4/uld.ts:95](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L95)

# Variable: RESULT\_\_AMBIENT\_RATE

```ts
const RESULT__AMBIENT_RATE: 144 = 0x0090;
```

Defined in: [src/sensors/vl53l4/uld.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L48)

# Variable: RESULT\_\_DISTANCE

```ts
const RESULT__DISTANCE: 150 = 0x0096;
```

Defined in: [src/sensors/vl53l4/uld.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L50)

# Variable: RESULT\_\_OSC\_CALIBRATE\_VAL

```ts
const RESULT__OSC_CALIBRATE_VAL: 222 = 0x00de;
```

Defined in: [src/sensors/vl53l4/uld.ts:51](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L51)

# Variable: RESULT\_\_RANGE\_STATUS

```ts
const RESULT__RANGE_STATUS: 137 = 0x0089;
```

Defined in: [src/sensors/vl53l4/uld.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L45)

# Variable: RESULT\_\_SIGMA

```ts
const RESULT__SIGMA: 146 = 0x0092;
```

Defined in: [src/sensors/vl53l4/uld.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L49)

# Variable: RESULT\_\_SIGNAL\_RATE

```ts
const RESULT__SIGNAL_RATE: 142 = 0x008e;
```

Defined in: [src/sensors/vl53l4/uld.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L47)

# Variable: RESULT\_\_SPAD\_NB

```ts
const RESULT__SPAD_NB: 140 = 0x008c;
```

Defined in: [src/sensors/vl53l4/uld.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L46)

# Variable: SOFT\_RESET

```ts
const SOFT_RESET: 0 = 0x0000;
```

Defined in: [src/sensors/vl53l4/uld.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L22)

# Variable: STATUS\_RTN

```ts
const STATUS_RTN: readonly number[];
```

Defined in: [src/sensors/vl53l4/uld.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L105)

GetResult() raw status → ULD status (status_rtn[24] in VL53L4CD_api.c).

# Variable: SYSTEM\_START

```ts
const SYSTEM_START: 135 = 0x0087;
```

Defined in: [src/sensors/vl53l4/uld.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L44)

# Variable: SYSTEM\_\_INTERRUPT

```ts
const SYSTEM__INTERRUPT: 70 = 0x0046;
```

Defined in: [src/sensors/vl53l4/uld.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L35)

# Variable: SYSTEM\_\_INTERRUPT\_CLEAR

```ts
const SYSTEM__INTERRUPT_CLEAR: 134 = 0x0086;
```

Defined in: [src/sensors/vl53l4/uld.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L43)

# Variable: THRESH\_HIGH

```ts
const THRESH_HIGH: 114 = 0x0072;
```

Defined in: [src/sensors/vl53l4/uld.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L41)

# Variable: THRESH\_LOW

```ts
const THRESH_LOW: 116 = 0x0074;
```

Defined in: [src/sensors/vl53l4/uld.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L42)

# Variable: ULD\_VERSION

```ts
const ULD_VERSION: readonly [number, number, number, number];
```

Defined in: [src/sensors/vl53l4/uld.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L19)

# Variable: VHV\_CONFIG\_\_TIMEOUT\_MACROP\_LOOP\_BOUND

```ts
const VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND: 8 = 0x0008;
```

Defined in: [src/sensors/vl53l4/uld.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L26)

# Variable: VL53L4\_I2C\_ERROR\_NAMES

```ts
const VL53L4_I2C_ERROR_NAMES: Readonly<Record<number, string>>;
```

Defined in: [src/protocol/vl53l4.ts:106](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L106)

last_i2c_error values in RPT_VL53_INFO.

# Variable: VL53L4\_I2C\_KHZ\_STEPS

```ts
const VL53L4_I2C_KHZ_STEPS: readonly number[];
```

Defined in: [src/protocol/vl53l4.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L44)

Nominal SCL steps the firmware carries a TIMINGR for (VL53_SET_I2C_SPEED
clamps to the nearest one).

# Variable: VL53L4\_SF\_INT\_ACT\_HIGH

```ts
const VL53L4_SF_INT_ACT_HIGH: 2 = 0x02;
```

Defined in: [src/protocol/vl53l4.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L38)

VL53_START_STREAM flags: interrupt polarity, mirroring bit 4 of
GPIO_HV_MUX__CTRL (0x0030). Clear (default): INT active low.

# Variable: VL53L4\_XFER\_MAX

```ts
const VL53L4_XFER_MAX: 253 = 253;
```

Defined in: [src/protocol/vl53l4.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L26)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
host-side ULD driver (`VL53L4CD` + codecs from `sensors/vl53l4/uld`), and
the register-bridge wire codecs (`protocol/vl53l4`, re-exported under the
same `Vl53l4*`-prefixed names `src/index.ts` uses) — so TypeDoc documents
that sensor in isolation.

Not part of the shipped package — a doc-generation entry point only.

# Variable: VL53L4\_XSHUT\_OFF

```ts
const VL53L4_XSHUT_OFF: 0 = 0;
```

Defined in: [src/protocol/vl53l4.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L29)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
host-side ULD driver (`VL53L4CD` + codecs from `sensors/vl53l4/uld`), and
the register-bridge wire codecs (`protocol/vl53l4`, re-exported under the
same `Vl53l4*`-prefixed names `src/index.ts` uses) — so TypeDoc documents
that sensor in isolation.

Not part of the shipped package — a doc-generation entry point only.

# Variable: VL53L4\_XSHUT\_ON

```ts
const VL53L4_XSHUT_ON: 1 = 1;
```

Defined in: [src/protocol/vl53l4.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L30)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
host-side ULD driver (`VL53L4CD` + codecs from `sensors/vl53l4/uld`), and
the register-bridge wire codecs (`protocol/vl53l4`, re-exported under the
same `Vl53l4*`-prefixed names `src/index.ts` uses) — so TypeDoc documents
that sensor in isolation.

Not part of the shipped package — a doc-generation entry point only.

# Variable: VL53L4\_XSHUT\_RESET

```ts
const VL53L4_XSHUT_RESET: 2 = 2;
```

Defined in: [src/protocol/vl53l4.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L32)

Blocking on the MCU (~3 ms); answered after the boot handshake.

# Variable: WINDOW\_ABOVE

```ts
const WINDOW_ABOVE: 1 = 1;
```

Defined in: [src/sensors/vl53l4/uld.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L59)

# Variable: WINDOW\_BELOW

```ts
const WINDOW_BELOW: 0 = 0;
```

Defined in: [src/sensors/vl53l4/uld.ts:58](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L58)

# Variable: WINDOW\_IN

```ts
const WINDOW_IN: 3 = 3;
```

Defined in: [src/sensors/vl53l4/uld.ts:61](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L61)

# Variable: WINDOW\_OUT

```ts
const WINDOW_OUT: 2 = 2;
```

Defined in: [src/sensors/vl53l4/uld.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L60)

# Variable: XTALK\_PLANE\_OFFSET\_KCPS

```ts
const XTALK_PLANE_OFFSET_KCPS: 22 = 0x0016;
```

Defined in: [src/sensors/vl53l4/uld.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L27)

# Variable: XTALK\_X\_PLANE\_GRADIENT\_KCPS

```ts
const XTALK_X_PLANE_GRADIENT_KCPS: 24 = 0x0018;
```

Defined in: [src/sensors/vl53l4/uld.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L28)

# Variable: XTALK\_Y\_PLANE\_GRADIENT\_KCPS

```ts
const XTALK_Y_PLANE_GRADIENT_KCPS: 26 = 0x001a;
```

Defined in: [src/sensors/vl53l4/uld.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L29)

