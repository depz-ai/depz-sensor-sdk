# VL53L8CX (ToF) — API reference

The base VL53L8**CX** time-of-flight surface: the `Vl53l8cx` class (and
its `Vl53l8` alias), the frame type, and the ST ULD constants/helpers.
The CH superset (`Vl53l8ch` + Compact-Network-Histogram) has its own
[VL53L8CH reference](../vl53l8ch/api.md); cross-sensor symbols shared by
every sensor live in the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Classes

- [Vl53l8cxError](classes/Vl53l8cxError.md)
- [MotionConfig](classes/MotionConfig.md)
- [VL53L8CX](classes/VL53L8CX.md)
- [Vl53l8cx](classes/Vl53l8cx-1.md)

## Interfaces

- [Vl53l8Assets](interfaces/Vl53l8Assets.md)
- [Vl53l8Platform](interfaces/Vl53l8Platform.md)
- [DetectionThreshold](interfaces/DetectionThreshold.md)
- [MotionResult](interfaces/MotionResult.md)
- [Vl53l8Results](interfaces/Vl53l8Results.md)
- [Vl53l8Frame](interfaces/Vl53l8Frame.md)
- [Vl53l8Options](interfaces/Vl53l8Options.md)
- [Vl53l8InitOptions](interfaces/Vl53l8InitOptions.md)

## Type Aliases

- [Vl53l8Variant](type-aliases/Vl53l8Variant.md)
- [Vl53l8](type-aliases/Vl53l8.md)

## Variables

- [FW\_CHECKSUM](variables/FW_CHECKSUM.md)
- [RESOLUTION\_4X4](variables/RESOLUTION_4X4.md)
- [RESOLUTION\_8X8](variables/RESOLUTION_8X8.md)
- [RANGING\_MODE\_CONTINUOUS](variables/RANGING_MODE_CONTINUOUS.md)
- [RANGING\_MODE\_AUTONOMOUS](variables/RANGING_MODE_AUTONOMOUS.md)
- [TARGET\_ORDER\_CLOSEST](variables/TARGET_ORDER_CLOSEST.md)
- [TARGET\_ORDER\_STRONGEST](variables/TARGET_ORDER_STRONGEST.md)
- [NB\_THRESHOLDS](variables/NB_THRESHOLDS.md)
- [POWER\_MODE\_SLEEP](variables/POWER_MODE_SLEEP.md)
- [POWER\_MODE\_WAKEUP](variables/POWER_MODE_WAKEUP.md)
- [POWER\_MODE\_DEEP\_SLEEP](variables/POWER_MODE_DEEP_SLEEP.md)
- [THRESH\_IN\_WINDOW](variables/THRESH_IN_WINDOW.md)
- [THRESH\_OUT\_OF\_WINDOW](variables/THRESH_OUT_OF_WINDOW.md)
- [THRESH\_OP\_NONE](variables/THRESH_OP_NONE.md)
- [THRESH\_OP\_OR](variables/THRESH_OP_OR.md)
- [THRESH\_OP\_AND](variables/THRESH_OP_AND.md)
- [GET\_XTALK\_CMD](variables/GET_XTALK_CMD.md)
- [CALIBRATE\_XTALK](variables/CALIBRATE_XTALK.md)
- [MIN\_RANGING\_FREQUENCY\_HZ](variables/MIN_RANGING_FREQUENCY_HZ.md)
- [Vl53l8](variables/Vl53l8.md)

## Functions

- [loadAssets](functions/loadAssets.md)
- [swapBuffer](functions/swapBuffer.md)
- [motionConfigSetResolution](functions/motionConfigSetResolution.md)
- [defaultMotionConfig](functions/defaultMotionConfig.md)
- [xtalkMarginToRaw](functions/xtalkMarginToRaw.md)
- [packDetectionThresholds](functions/packDetectionThresholds.md)
- [zoneGrid](functions/zoneGrid.md)

# Class: MotionConfig

Defined in: [src/sensors/vl53l8/uld.ts:370](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L370)

Mirror of VL53L8CX_Motion_Configuration (156 bytes, plugin source).
`pack()` reproduces the C struct byte layout (`<i3I12B64b32B32B`).

## Constructors

### Constructor

```ts
new MotionConfig(): MotionConfig;
```

#### Returns

`MotionConfig`

## Properties

### refBinOffset

```ts
refBinOffset: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:371](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L371)

***

### detectionThreshold

```ts
detectionThreshold: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:372](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L372)

***

### extraNoiseSigma

```ts
extraNoiseSigma: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:373](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L373)

***

### nullDenClipValue

```ts
nullDenClipValue: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:374](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L374)

***

### memUpdateMode

```ts
memUpdateMode: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:375](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L375)

***

### memUpdateChoice

```ts
memUpdateChoice: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:376](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L376)

***

### sumSpan

```ts
sumSpan: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:377](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L377)

***

### featureLength

```ts
featureLength: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:378](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L378)

***

### nbOfAggregates

```ts
nbOfAggregates: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:379](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L379)

***

### nbOfTemporalAccumulations

```ts
nbOfTemporalAccumulations: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:380](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L380)

***

### minNbForGlobalDetection

```ts
minNbForGlobalDetection: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:381](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L381)

***

### globalIndicatorFormat1

```ts
globalIndicatorFormat1: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:382](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L382)

***

### globalIndicatorFormat2

```ts
globalIndicatorFormat2: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:383](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L383)

***

### spare1

```ts
spare1: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:384](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L384)

***

### spare2

```ts
spare2: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:385](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L385)

***

### spare3

```ts
spare3: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:386](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L386)

***

### mapId

```ts
mapId: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:387](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L387)

***

### indicatorFormat1

```ts
indicatorFormat1: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:388](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L388)

***

### indicatorFormat2

```ts
indicatorFormat2: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:389](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L389)

## Methods

### pack()

```ts
pack(): Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:391](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L391)

#### Returns

`Uint8Array`

# Class: VL53L8CX

Defined in: [src/sensors/vl53l8/uld.ts:508](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L508)

## Constructors

### Constructor

```ts
new VL53L8CX(
   platform, 
   assets, 
   variant?): VL53L8CX;
```

Defined in: [src/sensors/vl53l8/uld.ts:543](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L543)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `platform` | [`Vl53l8Platform`](../interfaces/Vl53l8Platform.md) | `undefined` |
| `assets` | [`Vl53l8Assets`](../interfaces/Vl53l8Assets.md) | `undefined` |
| `variant` | [`Vl53l8Variant`](../type-aliases/Vl53l8Variant.md) | `"cx"` |

#### Returns

`VL53L8CX`

## Properties

### p

```ts
readonly p: Vl53l8Platform;
```

Defined in: [src/sensors/vl53l8/uld.ts:509](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L509)

***

### variant

```ts
readonly variant: Vl53l8Variant;
```

Defined in: [src/sensors/vl53l8/uld.ts:510](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L510)

***

### i2c

```ts
readonly i2c: boolean;
```

Defined in: [src/sensors/vl53l8/uld.ts:512](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L512)

True for the I2C L5/L7 variants (the non-L8 branch of vl53lmz_init).

***

### fwChecksum

```ts
readonly fwChecksum: number | null;
```

Defined in: [src/sensors/vl53l8/uld.ts:513](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L513)

***

### firmware

```ts
readonly firmware: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:514](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L514)

***

### defaultCfg

```ts
readonly defaultCfg: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:515](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L515)

***

### defaultXtalk

```ts
readonly defaultXtalk: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:516](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L516)

***

### getNvmCmd

```ts
readonly getNvmCmd: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:517](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L517)

***

### offsetData

```ts
offsetData: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:519](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L519)

***

### xtalkData

```ts
xtalkData: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:520](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L520)

***

### streamcount

```ts
streamcount: number = 255;
```

Defined in: [src/sensors/vl53l8/uld.ts:521](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L521)

***

### dataReadSize

```ts
dataReadSize: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:522](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L522)

***

### frameSizeMismatch

```ts
frameSizeMismatch: 
  | {
  fw: number;
  host: number;
}
  | null = null;
```

Defined in: [src/sensors/vl53l8/uld.ts:524](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L524)

{ fw, host } when the FW disagrees with the api.c formula.

***

### xtalkCalibrationFailed

```ts
xtalkCalibrationFailed: boolean = false;
```

Defined in: [src/sensors/vl53l8/uld.ts:526](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L526)

Last calibrateXtalk(): nothing to calibrate (ST XTALK_FAILED).

***

### lastBlocks

```ts
lastBlocks: [number, number, number][] = [];
```

Defined in: [src/sensors/vl53l8/uld.ts:528](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L528)

[idx, type, size] of the blocks in the last parsed frame.

***

### motionPresent

```ts
motionPresent: boolean = false;
```

Defined in: [src/sensors/vl53l8/uld.ts:530](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L530)

Whether the motion-indicator output block is configured.

***

### deviceId

```ts
deviceId: number | null = null;
```

Defined in: [src/sensors/vl53l8/uld.ts:532](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L532)

Silicon ids cached by isAlive().

***

### revisionId

```ts
revisionId: number | null = null;
```

Defined in: [src/sensors/vl53l8/uld.ts:533](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L533)

***

### moduleType

```ts
moduleType: number | null = null;
```

Defined in: [src/sensors/vl53l8/uld.ts:535](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L535)

I2C variants: 0 MZ (L5) / 1 MZEVO (L7), read after init().

***

### resolution

```ts
resolution: number | null = null;
```

Defined in: [src/sensors/vl53l8/uld.ts:537](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L537)

Zones the last startRanging() used (the L5/L7 parser trims to it).

## Methods

### dciReadData()

```ts
dciReadData(index, dataSize): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/sensors/vl53l8/uld.ts:633](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L633)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `index` | `number` |
| `dataSize` | `number` |

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### dciWriteData()

```ts
dciWriteData(index, data): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:649](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L649)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `index` | `number` |
| `data` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

***

### dciReplaceData()

```ts
dciReplaceData(
   index, 
   dataSize, 
   newData, 
newDataPos): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:677](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L677)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `index` | `number` |
| `dataSize` | `number` |
| `newData` | `Uint8Array` |
| `newDataPos` | `number` |

#### Returns

`Promise`\<`void`\>

***

### isAlive()

```ts
isAlive(): Promise<[number, number]>;
```

Defined in: [src/sensors/vl53l8/uld.ts:794](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L794)

Returns [deviceId, revisionId] and caches them; alive when (0xF0, 0x0C)
on L8, revision 0x02 (or 0x01 with deviceId 0xF0) on L5/L7.

#### Returns

`Promise`\<\[`number`, `number`\]\>

***

### init()

```ts
init(progress?): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:819](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L819)

vl53l8cx_init(): boot the sensor MCU, download the 84 KB sensor
firmware, upload NVM offset / xtalk / default configuration.
`progress(text)` is an optional UI callback.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `progress?` | (`text`) => `void` |

#### Returns

`Promise`\<`void`\>

***

### getResolution()

```ts
getResolution(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:999](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L999)

#### Returns

`Promise`\<`number`\>

***

### setResolution()

```ts
setResolution(resolution): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1004](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1004)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `resolution` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getRangingFrequencyHz()

```ts
getRangingFrequencyHz(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1036](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1036)

#### Returns

`Promise`\<`number`\>

***

### setRangingFrequencyHz()

```ts
setRangingFrequencyHz(hz): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1040](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1040)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `hz` | `number` |

#### Returns

`Promise`\<`void`\>

***

### setRangingMode()

```ts
setRangingMode(mode): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1044](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1044)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `mode` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getRangingMode()

```ts
getRangingMode(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1062](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1062)

#### Returns

`Promise`\<`number`\>

***

### getIntegrationTimeMs()

```ts
getIntegrationTimeMs(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1067](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1067)

#### Returns

`Promise`\<`number`\>

***

### setIntegrationTimeMs()

```ts
setIntegrationTimeMs(timeMs): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1073](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1073)

Integration time 2..1000 ms. No effect in continuous ranging mode.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `timeMs` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getSharpenerPercent()

```ts
getSharpenerPercent(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1090](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1090)

Sharpener 0..99 %. Rounds to nearest: the register holds pct scaled to
0..255, and truncating the way back (as ST's C ULD does) loses a count for
95 of the 100 legal values — set(25) read back as 24. That also made
calibrateXtalk's save/restore decay the setting by 1 % on every run
(25 -> 24 -> 23 -> ...). The stored byte is unchanged; only this host-side
interpretation is. Kept in lockstep with the Python SDK's uld.py.

#### Returns

`Promise`\<`number`\>

***

### setSharpenerPercent()

```ts
setSharpenerPercent(pct): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1095](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1095)

Sharpener 0..99 % (0 = disabled).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pct` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getTargetOrder()

```ts
getTargetOrder(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1102](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1102)

#### Returns

`Promise`\<`number`\>

***

### setTargetOrder()

```ts
setTargetOrder(order): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1106](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1106)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `order` | `number` |

#### Returns

`Promise`\<`void`\>

***

### startRanging()

```ts
startRanging(cnhDataSize?): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1121](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1121)

Start ranging. When `cnhDataSize` is given (bytes, from
CnhConfig.requiredMemory), a VL53L8CH CNH data block is appended to the
output list so each frame also carries the compact-network-histogram
buffer. The CNH frame is far larger than the MCU stream cap, so the
caller must read it in poll-mode (checkDataReady + getRangingData),
not via the MCU INT stream.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `cnhDataSize` | `number` \| `null` | `null` |

#### Returns

`Promise`\<`void`\>

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1229](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1229)

#### Returns

`Promise`\<`void`\>

***

### checkDataReady()

```ts
checkDataReady(): Promise<boolean>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1265](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1265)

#### Returns

`Promise`\<`boolean`\>

***

### getRangingData()

```ts
getRangingData(): Promise<Vl53l8Results>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1284](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1284)

Poll-mode read: fetch one results frame from reg 0x00 and parse it.

#### Returns

`Promise`\<[`Vl53l8Results`](../interfaces/Vl53l8Results.md)\>

***

### parseFrame()

```ts
parseFrame(raw): Vl53l8Results;
```

Defined in: [src/sensors/vl53l8/uld.ts:1298](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1298)

Parse one raw results frame (dataReadSize bytes read from reg 0x00).
Used both by poll-mode getRangingData() and by the host when the MCU
pushes frames over the INT-driven stream. Returns per-zone arrays
distanceMm (integer mm, >= 0), targetStatus, nbTargetDetected,
signalPerSpad (float kcps/SPAD), ambientPerSpad (float kcps/SPAD),
nbSpadsEnabled, rangeSigmaMm (float mm), reflectance (float %), plus the
per-frame scalar siliconTempDegc.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `raw` | `Uint8Array` |

#### Returns

[`Vl53l8Results`](../interfaces/Vl53l8Results.md)

***

### getPowerMode()

```ts
getPowerMode(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1467](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1467)

#### Returns

`Promise`\<`number`\>

***

### setPowerMode()

```ts
setPowerMode(powerMode): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1492](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1492)

WAKEUP (1), SLEEP (0) or DEEP_SLEEP (2). Not allowed while ranging. Wake
from DEEP_SLEEP re-runs init() (the FW blob is lost).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `powerMode` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getXtalkMargin()

```ts
getXtalkMargin(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1535](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1535)

Xtalk margin in kcps/spad.

#### Returns

`Promise`\<`number`\>

***

### setXtalkMargin()

```ts
setXtalkMargin(marginKcps): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1540](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1540)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `marginKcps` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getCaldataXtalk()

```ts
getCaldataXtalk(): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1555](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1555)

Read the live 776-byte xtalk calibration blob back from the sensor FW (as
produced by calibrateXtalk). Restores the current resolution afterwards.

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### setCaldataXtalk()

```ts
setCaldataXtalk(xtalkData): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1574](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1574)

Restore a previously saved 776-byte xtalk calibration blob. The blob is
re-uploaded to the FW by the next setResolution()/startRanging().

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `xtalkData` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

***

### getDetectionThresholdsEnable()

```ts
getDetectionThresholdsEnable(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1585](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1585)

#### Returns

`Promise`\<`number`\>

***

### setDetectionThresholdsEnable()

```ts
setDetectionThresholdsEnable(enabled): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1589](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1589)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `enabled` | `boolean` |

#### Returns

`Promise`\<`void`\>

***

### getDetectionThresholds()

```ts
getDetectionThresholds(): Promise<DetectionThreshold[]>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1604](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1604)

Return the 64 detection thresholds; low/high rescaled to real units.

#### Returns

`Promise`\<[`DetectionThreshold`](../interfaces/DetectionThreshold.md)[]\>

***

### setDetectionThresholds()

```ts
setDetectionThresholds(thresholds): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1630](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1630)

Program the 64 detection thresholds (missing entries default to zeros;
more than 64 is a RangeError).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `thresholds` | `Partial`\<[`DetectionThreshold`](../interfaces/DetectionThreshold.md)\>[] |

#### Returns

`Promise`\<`void`\>

***

### setDetectionThresholdsAutoStop()

```ts
setDetectionThresholdsAutoStop(autoStop): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1636](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1636)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `autoStop` | `boolean` |

#### Returns

`Promise`\<`void`\>

***

### motionIndicatorInit()

```ts
motionIndicatorInit(resolution): Promise<MotionConfig>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1654](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1654)

Initialize a motion-indicator configuration (default distance window) and
write it to the sensor. Enables the motion output block so subsequent
frames carry motion data.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `resolution` | `number` |

#### Returns

`Promise`\<[`MotionConfig`](MotionConfig.md)\>

***

### motionIndicatorSetResolution()

```ts
motionIndicatorSetResolution(cfg, resolution): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1661](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1661)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cfg` | [`MotionConfig`](MotionConfig.md) |
| `resolution` | `number` |

#### Returns

`Promise`\<`void`\>

***

### motionIndicatorSetDistanceMotion()

```ts
motionIndicatorSetDistanceMotion(
   cfg, 
   distanceMinMm, 
distanceMaxMm): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1666](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1666)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cfg` | [`MotionConfig`](MotionConfig.md) |
| `distanceMinMm` | `number` |
| `distanceMaxMm` | `number` |

#### Returns

`Promise`\<`void`\>

***

### calibrateXtalk()

```ts
calibrateXtalk(
   reflectancePercent, 
   nbSamples, 
distanceMm): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1725](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1725)

vl53l8cx_calibrate_xtalk: run on-device crosstalk calibration. As every ST
version: the firmware's own calibration table, then the dedicated 17-block
calibration output list. Verified on a VL53L8CH (and L5CX / L7CH) without
cover glass, where the firmware correctly answers XTALK_FAILED
("coverglass too good"); a successful run needs glass. Saves & restores
resolution/frequency/int-time/sharpener/target-order/xtalk-margin/
ranging-mode around the run.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `reflectancePercent` | `number` |
| `nbSamples` | `number` |
| `distanceMm` | `number` |

#### Returns

`Promise`\<`void`\>

# Class: Vl53l8cx

Defined in: [src/sensors/vl53l8/vl53l8.ts:111](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L111)

VL53L8CX ToF device: `init()` downloads the ~84 KB sensor firmware (takes a
few seconds over CDC), then configure and `startRanging()`.

This is the base class for both silicon variants. The VL53L8CH superset
(compact-network-histogram output) lives in `Vl53l8ch`, which inherits every
method here. All configuration methods require `init()` first and must not
be called while ranging (the ULD talks to the current register bank; the
stream owns it — contract 04).

## Extends

- `DepzDevice`

## Constructors

### Constructor

```ts
new Vl53l8cx(transport, opts?): Vl53l8cx;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:182](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L182)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | [`Vl53l8Options`](../interfaces/Vl53l8Options.md) |

#### Returns

`Vl53l8cx`

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

***

### variantId

```ts
protected readonly variantId: Vl53l8Variant = "cx";
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:113](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L113)

Sensor-firmware blob variant this class loads.

***

### readChunk

```ts
protected readonly readChunk: number = CHUNK_SIZE;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:115](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L115)

Register-bridge transfer limits (the I2C L5/L7 board reads less per call).

***

### writeChunk

```ts
protected readonly writeChunk: number = CHUNK_SIZE;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:116](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L116)

***

### minRangingHz

```ts
protected readonly minRangingHz: number = MIN_RANGING_FREQUENCY_HZ;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:118](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L118)

Lowest ranging frequency that actually streams on this sensor.

***

### uldDriver

```ts
protected uldDriver: VL53L8CX | null = null;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:120](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L120)

***

### rangingFlag

```ts
protected rangingFlag: boolean = false;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:124](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L124)

***

### cnhConfig

```ts
protected cnhConfig: CnhConfig | null = null;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:125](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L125)

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

### activeCnhConfig

#### Get Signature

```ts
get activeCnhConfig(): CnhConfig | null;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:130](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L130)

The CNH config armed by `configureCnh()` (CH), or null. Recorders read
 this to persist the decode parameters next to raw CNH blocks.

##### Returns

`CnhConfig` \| `null`

***

### uld

#### Get Signature

```ts
get uld(): VL53L8CX;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:190](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L190)

The underlying ULD driver (escape hatch for advanced DCI access).

##### Returns

[`VL53L8CX`](VL53L8CX.md)

***

### variant

#### Get Signature

```ts
get variant(): Vl53l8Variant;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L196)

'cx' | 'ch' | 'l7cx' | 'l7ch' (valid after init()).

##### Returns

[`Vl53l8Variant`](../type-aliases/Vl53l8Variant.md)

***

### ranging

#### Get Signature

```ts
get ranging(): boolean;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:431](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L431)

##### Returns

`boolean`

***

### frameParseErrors

#### Get Signature

```ts
get frameParseErrors(): number;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:439](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L439)

Frames dropped because the ULD could not parse them (corrupt frame, bad
size). Distinct from reassembly gaps. Mirrors Python `frame_parse_errors`.

##### Returns

`number`

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:201](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L201)

True when the sensor answers with its own silicon ids (a bus error or a foreign id: false).

#### Returns

`Promise`\<`boolean`\>

***

### init()

```ts
init(variant?, opts?): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:219](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L219)

Initialize the sensor: firmware blob download + default config.

The blob variant is fixed by the class (`Vl53l8cx` → 'cx', `Vl53l8ch` →
'ch'); `variant` is accepted only for backward compatibility and must
match the class variant when given. Assets are loaded lazily (dynamic
import) so the blobs stay out of bundles that never init the ToF.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `variant?` | [`Vl53l8Variant`](../type-aliases/Vl53l8Variant.md) |
| `opts?` | [`Vl53l8InitOptions`](../interfaces/Vl53l8InitOptions.md) |

#### Returns

`Promise`\<`void`\>

***

### getResolution()

```ts
getResolution(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:244](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L244)

#### Returns

`Promise`\<`number`\>

***

### setResolution()

```ts
setResolution(zones): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:249](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L249)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `zones` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getRangingFrequencyHz()

```ts
getRangingFrequencyHz(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:258](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L258)

#### Returns

`Promise`\<`number`\>

***

### setRangingFrequencyHz()

```ts
setRangingFrequencyHz(hz): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:262](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L262)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `hz` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getRangingMode()

```ts
getRangingMode(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:273](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L273)

#### Returns

`Promise`\<`number`\>

***

### setRangingMode()

```ts
setRangingMode(mode): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:277](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L277)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `mode` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getIntegrationTimeMs()

```ts
getIntegrationTimeMs(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:282](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L282)

#### Returns

`Promise`\<`number`\>

***

### setIntegrationTimeMs()

```ts
setIntegrationTimeMs(ms): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:286](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L286)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getSharpenerPercent()

```ts
getSharpenerPercent(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:291](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L291)

#### Returns

`Promise`\<`number`\>

***

### setSharpenerPercent()

```ts
setSharpenerPercent(pct): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:295](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L295)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pct` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getTargetOrder()

```ts
getTargetOrder(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:300](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L300)

#### Returns

`Promise`\<`number`\>

***

### setTargetOrder()

```ts
setTargetOrder(order): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:304](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L304)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `order` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getPowerMode()

```ts
getPowerMode(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:312](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L312)

POWER_MODE_SLEEP/WAKEUP/DEEP_SLEEP (uld constants).

#### Returns

`Promise`\<`number`\>

***

### setPowerMode()

```ts
setPowerMode(mode): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:320](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L320)

Enter sleep / wake / deep-sleep. Not while ranging. Waking from
DEEP_SLEEP re-downloads the firmware blob (init()).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `mode` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getXtalkMargin()

```ts
getXtalkMargin(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:325](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L325)

#### Returns

`Promise`\<`number`\>

***

### setXtalkMargin()

```ts
setXtalkMargin(marginKcps): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:329](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L329)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `marginKcps` | `number` |

#### Returns

`Promise`\<`void`\>

***

### calibrateXtalk()

```ts
calibrateXtalk(
   reflectancePercent, 
   nbSamples, 
distanceMm): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:340](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L340)

Run on-device crosstalk calibration against a flat target at
`distanceMm` with the given `reflectancePercent` (1..99) averaging
`nbSamples` (1..16). The result is captured into the xtalk buffer; read
it back with getCaldataXtalk(). Blocks several seconds.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `reflectancePercent` | `number` |
| `nbSamples` | `number` |
| `distanceMm` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getCaldataXtalk()

```ts
getCaldataXtalk(): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:350](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L350)

Read back the 776-byte xtalk calibration blob (save/restore).

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### setCaldataXtalk()

```ts
setCaldataXtalk(blob): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:356](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L356)

Restore a previously saved 776-byte xtalk calibration blob.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `blob` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

***

### getDetectionThresholdsEnable()

```ts
getDetectionThresholdsEnable(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:361](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L361)

#### Returns

`Promise`\<`number`\>

***

### setDetectionThresholdsEnable()

```ts
setDetectionThresholdsEnable(enabled): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:365](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L365)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `enabled` | `boolean` |

#### Returns

`Promise`\<`void`\>

***

### getDetectionThresholds()

```ts
getDetectionThresholds(): Promise<DetectionThreshold[]>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:370](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L370)

#### Returns

`Promise`\<[`DetectionThreshold`](../interfaces/DetectionThreshold.md)[]\>

***

### setDetectionThresholds()

```ts
setDetectionThresholds(thresholds): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:379](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L379)

Program the 64 detection thresholds (interrupt-on-threshold). Each entry
carries lowThresh, highThresh, measurement, type, zoneNum, operation (see
uld THRESH_* constants).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `thresholds` | `Partial`\<[`DetectionThreshold`](../interfaces/DetectionThreshold.md)\>[] |

#### Returns

`Promise`\<`void`\>

***

### setDetectionThresholdsAutoStop()

```ts
setDetectionThresholdsAutoStop(autoStop): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:384](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L384)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `autoStop` | `boolean` |

#### Returns

`Promise`\<`void`\>

***

### configureMotionIndicator()

```ts
configureMotionIndicator(distanceMinMm?, distanceMaxMm?): Promise<MotionConfig>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:394](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L394)

Enable the motion indicator over [distanceMinMm, distanceMaxMm] and
surface motion output in each frame's `.motion`. Returns the underlying
uld MotionConfig for advanced tuning.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `distanceMinMm` | `number` | `400` |
| `distanceMaxMm` | `number` | `1500` |

#### Returns

`Promise`\<[`MotionConfig`](MotionConfig.md)\>

***

### startRanging()

```ts
startRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:405](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L405)

Configure the output list, start the sensor and the MCU stream.

#### Returns

`Promise`\<`void`\>

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:418](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L418)

#### Returns

`Promise`\<`void`\>

***

### onFrame()

```ts
onFrame(cb): () => void;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:444](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L444)

Subscribe to parsed frames (read-pump context; don't block).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`frame`) => `void` |

#### Returns

() => `void`

***

### frames()

```ts
frames(maxsize?): StreamQueue<Vl53l8Frame>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:457](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L457)

Async iterator over parsed frames — bounded, drop-oldest (contract 07
§3). Subscribes eagerly at call time — call before or after
startRanging(); it ends when the device closes or the consumer breaks
out of iteration.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `maxsize` | `number` | `8` |

#### Returns

`StreamQueue`\<[`Vl53l8Frame`](../interfaces/Vl53l8Frame.md)\>

***

### getFrame()

```ts
getFrame(timeoutMs?): Promise<Vl53l8Frame>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:468](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L468)

Convenience: wait for the next frame.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `2000` |

#### Returns

`Promise`\<[`Vl53l8Frame`](../interfaces/Vl53l8Frame.md)\>

***

### requireNotRanging()

```ts
protected requireNotRanging(): void;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:496](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L496)

#### Returns

`void`

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:502](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L502)

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

# Class: Vl53l8cxError

Defined in: [src/sensors/vl53l8/uld.ts:250](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L250)

## Extends

- `Error`

## Constructors

### Constructor

```ts
new Vl53l8cxError(code, where?): Vl53l8cxError;
```

Defined in: [src/sensors/vl53l8/uld.ts:254](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L254)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `code` | `number` | `undefined` |
| `where` | `string` | `""` |

#### Returns

`Vl53l8cxError`

#### Overrides

```ts
Error.constructor
```

## Properties

### code

```ts
readonly code: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:251](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L251)

***

### where

```ts
readonly where: string;
```

Defined in: [src/sensors/vl53l8/uld.ts:252](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L252)

***

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

# Function: defaultMotionConfig()

```ts
function defaultMotionConfig(resolution): MotionConfig;
```

Defined in: [src/sensors/vl53l8/uld.ts:441](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L441)

The default motion-indicator configuration used by `motionIndicatorInit`
for a resolution (pure — same bytes the sensor is programmed with).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `resolution` | `number` |

## Returns

[`MotionConfig`](../classes/MotionConfig.md)

# Function: loadAssets()

```ts
function loadAssets(variant): Promise<Vl53l8Assets>;
```

Defined in: [src/sensors/vl53l8/assets/index.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L24)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `variant` | [`Vl53l8Variant`](../type-aliases/Vl53l8Variant.md) |

## Returns

`Promise`\<[`Vl53l8Assets`](../interfaces/Vl53l8Assets.md)\>

# Function: motionConfigSetResolution()

```ts
function motionConfigSetResolution(cfg, resolution): void;
```

Defined in: [src/sensors/vl53l8/uld.ts:426](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L426)

Set MotionConfig.map_id for the given resolution (pure — no I/O).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `cfg` | [`MotionConfig`](../classes/MotionConfig.md) |
| `resolution` | `number` |

## Returns

`void`

# Function: packDetectionThresholds()

```ts
function packDetectionThresholds(thresholds): {
  start: Uint8Array;
  valid: Uint8Array;
};
```

Defined in: [src/sensors/vl53l8/uld.ts:477](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L477)

Pack up to 64 detection thresholds into the DCI_DET_THRESH_START payload plus
the 8-byte valid-status block (pure — mirror of set_detection_thresholds).
Each threshold's low/high are scaled by its measurement selector. More than
64 entries is a RangeError (never silently dropped).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `thresholds` | `Partial`\<[`DetectionThreshold`](../interfaces/DetectionThreshold.md)\>[] |

## Returns

```ts
{
  start: Uint8Array;
  valid: Uint8Array;
}
```

### start

```ts
start: Uint8Array;
```

### valid

```ts
valid: Uint8Array;
```

# Function: swapBuffer()

```ts
function swapBuffer(data): Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:264](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L264)

VL53L8CX_SwapBuffer: byte-reverse every 32-bit word (tail untouched).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

## Returns

`Uint8Array`

# Function: xtalkMarginToRaw()

```ts
function xtalkMarginToRaw(marginKcps): number;
```

Defined in: [src/sensors/vl53l8/uld.ts:464](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L464)

Xtalk margin (kcps/spad) → raw DCI value (round(kcps * 2048)). The margin is
0..10000 kcps; anything else is a RangeError, never wrapped (contract 07 §3).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `marginKcps` | `number` |

## Returns

`number`

# Function: zoneGrid()

```ts
function zoneGrid(arr, resolution): number[][];
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:78](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L78)

Zone array reshaped to (4,4) or (8,8) — mirror of Vl53l8Frame.grid().

## Parameters

| Parameter | Type |
| ------ | ------ |
| `arr` | `ArrayLike`\<`number`\> |
| `resolution` | `number` |

## Returns

`number`[][]

# Interface: DetectionThreshold

Defined in: [src/sensors/vl53l8/uld.ts:322](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L322)

One detection-threshold entry (real units; mirror of the Python dict).

## Properties

### lowThresh

```ts
lowThresh: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:323](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L323)

***

### highThresh

```ts
highThresh: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:324](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L324)

***

### measurement

```ts
measurement: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:325](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L325)

***

### type

```ts
type: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:326](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L326)

***

### zoneNum

```ts
zoneNum: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:327](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L327)

***

### operation

```ts
operation: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:328](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L328)

# Interface: MotionResult

Defined in: [src/sensors/vl53l8/uld.ts:332](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L332)

Motion-indicator results block (mirror of the Python `motion_indicator` dict).

## Properties

### globalIndicator1

```ts
globalIndicator1: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:333](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L333)

***

### globalIndicator2

```ts
globalIndicator2: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:334](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L334)

***

### status

```ts
status: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:335](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L335)

***

### nbOfDetectedAggregates

```ts
nbOfDetectedAggregates: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:336](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L336)

***

### nbOfAggregates

```ts
nbOfAggregates: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:337](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L337)

***

### motion

```ts
motion: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:338](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L338)

# Interface: Vl53l8Assets

Defined in: [src/sensors/vl53l8/assets/index.ts:13](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L13)

## Properties

### firmware

```ts
firmware: Uint8Array;
```

Defined in: [src/sensors/vl53l8/assets/index.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L15)

firmware.bin — 86016 bytes (0x15000).

***

### defaultCfg

```ts
defaultCfg: Uint8Array;
```

Defined in: [src/sensors/vl53l8/assets/index.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L17)

default_configuration.bin — 972 bytes.

***

### defaultXtalk

```ts
defaultXtalk: Uint8Array;
```

Defined in: [src/sensors/vl53l8/assets/index.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L19)

default_xtalk.bin — 776 bytes.

***

### getNvmCmd

```ts
getNvmCmd: Uint8Array;
```

Defined in: [src/sensors/vl53l8/assets/index.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L21)

get_nvm_cmd.bin — 40 bytes.

# Interface: Vl53l8Frame

Defined in: [src/sensors/vl53l8/vl53l8.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L52)

One parsed ranging frame. Arrays are sized to the active resolution
(16 or 64 zones); zone index runs row-major (see datasheet zone maps).

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L53)

***

### resolution

```ts
resolution: number;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L55)

16 | 64

***

### distanceMm

```ts
distanceMm: Int32Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L57)

Integer mm, as ST: a negative distance is clamped to 0.

***

### targetStatus

```ts
targetStatus: Uint8Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L59)

5/9 = valid, 255 = no target.

***

### nbTargetDetected

```ts
nbTargetDetected: Uint8Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L60)

***

### signalPerSpad

```ts
signalPerSpad: Float64Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:62](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L62)

kcps/SPAD (raw / 2048, fraction kept).

***

### ambientPerSpad

```ts
ambientPerSpad: Float64Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L64)

kcps/SPAD (raw / 2048, fraction kept).

***

### nbSpadsEnabled

```ts
nbSpadsEnabled: Int32Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:65](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L65)

***

### rangeSigmaMm

```ts
rangeSigmaMm: Float64Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L67)

Measurement noise, mm (raw / 128).

***

### reflectance

```ts
reflectance: Float64Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:69](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L69)

Estimated target reflectance, % (raw / 2, fraction kept).

***

### siliconTempDegc

```ts
siliconTempDegc: number;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:70](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L70)

***

### cnhRaw

```ts
cnhRaw: Uint8Array<ArrayBufferLike> | null;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L72)

CH variant: raw CNH block (decode via cnh).

***

### motion

```ts
motion: MotionResult | null;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:74](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L74)

Motion-indicator output when `configureMotionIndicator()` is active.

# Interface: Vl53l8InitOptions

Defined in: [src/sensors/vl53l8/vl53l8.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L94)

## Properties

### progress?

```ts
optional progress?: (text) => void;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:96](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L96)

Receives phase strings.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `text` | `string` |

#### Returns

`void`

***

### writeProgress?

```ts
optional writeProgress?: (done, total) => void;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L98)

Tracks the big blob writes: (done, total) bytes.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `done` | `number` |
| `total` | `number` |

#### Returns

`void`

# Interface: Vl53l8Options

Defined in: [src/sensors/vl53l8/vl53l8.ts:89](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L89)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:91](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L91)

ULD sleep implementation (tests inject an instant one).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Interface: Vl53l8Platform

Defined in: [src/sensors/vl53l8/uld.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L20)

ULD platform: chunking/bridging is the caller's concern.

## Methods

### rdMulti()

```ts
rdMulti(addr, size): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/sensors/vl53l8/uld.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L21)

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

Defined in: [src/sensors/vl53l8/uld.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L22)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `data` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

***

### sleepMs()

```ts
sleepMs(ms): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L23)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Interface: Vl53l8Results

Defined in: [src/sensors/vl53l8/uld.ts:345](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L345)

One parsed results frame (plain arrays, mirroring the Python dict), in the
real units ST's GetRangingData gives (contract 04 §5).

## Properties

### distanceMm

```ts
distanceMm: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:347](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L347)

Integer mm (raw / 4 floored); a negative distance is clamped to 0, as ST.

***

### targetStatus

```ts
targetStatus: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:349](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L349)

5/9 = valid, 255 = no target.

***

### nbTargetDetected

```ts
nbTargetDetected: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:350](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L350)

***

### signalPerSpad

```ts
signalPerSpad: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:352](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L352)

kcps/SPAD (raw / 2048, fraction kept).

***

### ambientPerSpad

```ts
ambientPerSpad: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:354](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L354)

kcps/SPAD (raw / 2048, fraction kept).

***

### nbSpadsEnabled

```ts
nbSpadsEnabled: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:355](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L355)

***

### rangeSigmaMm

```ts
rangeSigmaMm: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:357](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L357)

Measurement noise, mm (raw / 128).

***

### reflectance

```ts
reflectance: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:359](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L359)

Estimated target reflectance, % (raw / 2, fraction kept).

***

### siliconTempDegc

```ts
siliconTempDegc: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:360](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L360)

***

### cnhRaw

```ts
cnhRaw: Uint8Array<ArrayBufferLike> | null;
```

Defined in: [src/sensors/vl53l8/uld.ts:361](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L361)

***

### motion

```ts
motion: MotionResult | null;
```

Defined in: [src/sensors/vl53l8/uld.ts:363](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L363)

Motion-indicator output when the motion detector is configured.

# Type Alias: Vl53l8

```ts
type Vl53l8 = Vl53l8cx;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:565](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L565)

Backward-compatible alias: the old flat `Vl53l8` name maps to the CX base
class (the historic default). New code should pick Vl53l8cx / Vl53l8ch.

# Type Alias: Vl53l8Variant

```ts
type Vl53l8Variant = "cx" | "ch" | "l7cx" | "l7ch";
```

Defined in: [src/sensors/vl53l8/assets/index.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L11)

Blob set: 'cx' / 'ch' = VL53L8CX / VL53L8CH; 'l7cx' = VL53L5CX/VL53L7CX
(ULD 2.0.1); 'l7ch' = VL53L7CH (VL53LMZ 2.0.16, L7 configuration).

# Variable: CALIBRATE\_XTALK

```ts
const CALIBRATE_XTALK: Uint8Array<ArrayBufferLike>;
```

Defined in: [src/sensors/vl53l8/uld.ts:200](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L200)

# Variable: FW\_CHECKSUM

```ts
const FW_CHECKSUM: Record<Vl53l8Variant, number | null>;
```

Defined in: [src/sensors/vl53l8/uld.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L35)

# Variable: GET\_XTALK\_CMD

```ts
const GET_XTALK_CMD: Uint8Array<ArrayBufferLike>;
```

Defined in: [src/sensors/vl53l8/uld.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L195)

# Variable: MIN\_RANGING\_FREQUENCY\_HZ

```ts
const MIN_RANGING_FREQUENCY_HZ: 2 = 2;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L46)

Below this the sensor never streams (contract 04).

# Variable: NB\_THRESHOLDS

```ts
const NB_THRESHOLDS: 64 = 64;
```

Defined in: [src/sensors/vl53l8/uld.ts:149](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L149)

# Variable: POWER\_MODE\_DEEP\_SLEEP

```ts
const POWER_MODE_DEEP_SLEEP: 2 = 2;
```

Defined in: [src/sensors/vl53l8/uld.ts:155](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L155)

# Variable: POWER\_MODE\_SLEEP

```ts
const POWER_MODE_SLEEP: 0 = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:153](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L153)

# Variable: POWER\_MODE\_WAKEUP

```ts
const POWER_MODE_WAKEUP: 1 = 1;
```

Defined in: [src/sensors/vl53l8/uld.ts:154](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L154)

# Variable: RANGING\_MODE\_AUTONOMOUS

```ts
const RANGING_MODE_AUTONOMOUS: 3 = 3;
```

Defined in: [src/sensors/vl53l8/uld.ts:75](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L75)

# Variable: RANGING\_MODE\_CONTINUOUS

```ts
const RANGING_MODE_CONTINUOUS: 1 = 1;
```

Defined in: [src/sensors/vl53l8/uld.ts:74](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L74)

# Variable: RESOLUTION\_4X4

```ts
const RESOLUTION_4X4: 16 = 16;
```

Defined in: [src/sensors/vl53l8/uld.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L71)

# Variable: RESOLUTION\_8X8

```ts
const RESOLUTION_8X8: 64 = 64;
```

Defined in: [src/sensors/vl53l8/uld.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L72)

# Variable: TARGET\_ORDER\_CLOSEST

```ts
const TARGET_ORDER_CLOSEST: 1 = 1;
```

Defined in: [src/sensors/vl53l8/uld.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L77)

# Variable: TARGET\_ORDER\_STRONGEST

```ts
const TARGET_ORDER_STRONGEST: 2 = 2;
```

Defined in: [src/sensors/vl53l8/uld.ts:78](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L78)

# Variable: THRESH\_IN\_WINDOW

```ts
const THRESH_IN_WINDOW: 0 = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:177](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L177)

# Variable: THRESH\_OP\_AND

```ts
const THRESH_OP_AND: 2 = 2;
```

Defined in: [src/sensors/vl53l8/uld.ts:186](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L186)

# Variable: THRESH\_OP\_NONE

```ts
const THRESH_OP_NONE: 0 = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:184](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L184)

# Variable: THRESH\_OP\_OR

```ts
const THRESH_OP_OR: 0 = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:185](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L185)

# Variable: THRESH\_OUT\_OF\_WINDOW

```ts
const THRESH_OUT_OF_WINDOW: 1 = 1;
```

Defined in: [src/sensors/vl53l8/uld.ts:178](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L178)

# Variable: Vl53l8

```ts
const Vl53l8: typeof Vl53l8cx = Vl53l8cx;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:565](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L565)

Backward-compatible alias: the old flat `Vl53l8` name maps to the CX base
class (the historic default). New code should pick Vl53l8cx / Vl53l8ch.

