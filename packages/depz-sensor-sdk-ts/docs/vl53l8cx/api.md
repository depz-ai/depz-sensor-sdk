# VL53L8CX (ToF) — API reference

The base VL53L8**CX** time-of-flight surface: the `Vl53l8Cx` class (and
its `Vl53l8` alias), the frame type, and the ST ULD constants/helpers.
The CH superset (`Vl53l8Ch` + Compact-Network-Histogram) has its own
[VL53L8CH reference](../vl53l8ch/api.md); cross-sensor symbols shared by
every sensor live in the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Classes

- [Vl53l8cxError](classes/Vl53l8cxError.md)
- [MotionConfig](classes/MotionConfig.md)
- [VL53L8CX](classes/VL53L8CX.md)
- [Vl53l8Cx](classes/Vl53l8Cx-1.md)

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

Defined in: [src/sensors/vl53l8/uld.ts:286](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L286)

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

Defined in: [src/sensors/vl53l8/uld.ts:287](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L287)

***

### detectionThreshold

```ts
detectionThreshold: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:288](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L288)

***

### extraNoiseSigma

```ts
extraNoiseSigma: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:289](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L289)

***

### nullDenClipValue

```ts
nullDenClipValue: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:290](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L290)

***

### memUpdateMode

```ts
memUpdateMode: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:291](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L291)

***

### memUpdateChoice

```ts
memUpdateChoice: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:292](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L292)

***

### sumSpan

```ts
sumSpan: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:293](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L293)

***

### featureLength

```ts
featureLength: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:294](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L294)

***

### nbOfAggregates

```ts
nbOfAggregates: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:295](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L295)

***

### nbOfTemporalAccumulations

```ts
nbOfTemporalAccumulations: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:296](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L296)

***

### minNbForGlobalDetection

```ts
minNbForGlobalDetection: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:297](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L297)

***

### globalIndicatorFormat1

```ts
globalIndicatorFormat1: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:298](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L298)

***

### globalIndicatorFormat2

```ts
globalIndicatorFormat2: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:299](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L299)

***

### spare1

```ts
spare1: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:300](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L300)

***

### spare2

```ts
spare2: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:301](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L301)

***

### spare3

```ts
spare3: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:302](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L302)

***

### mapId

```ts
mapId: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:303](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L303)

***

### indicatorFormat1

```ts
indicatorFormat1: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:304](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L304)

***

### indicatorFormat2

```ts
indicatorFormat2: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:305](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L305)

## Methods

### pack()

```ts
pack(): Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:307](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L307)

#### Returns

`Uint8Array`

# Class: VL53L8CX

Defined in: [src/sensors/vl53l8/uld.ts:410](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L410)

## Constructors

### Constructor

```ts
new VL53L8CX(
   platform, 
   assets, 
   variant?): VL53L8CX;
```

Defined in: [src/sensors/vl53l8/uld.ts:434](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L434)

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

Defined in: [src/sensors/vl53l8/uld.ts:411](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L411)

***

### variant

```ts
readonly variant: Vl53l8Variant;
```

Defined in: [src/sensors/vl53l8/uld.ts:412](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L412)

***

### fwChecksum

```ts
readonly fwChecksum: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:413](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L413)

***

### firmware

```ts
readonly firmware: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L414)

***

### defaultCfg

```ts
readonly defaultCfg: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:415](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L415)

***

### defaultXtalk

```ts
readonly defaultXtalk: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:416](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L416)

***

### getNvmCmd

```ts
readonly getNvmCmd: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:417](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L417)

***

### offsetData

```ts
offsetData: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:419](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L419)

***

### xtalkData

```ts
xtalkData: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:420](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L420)

***

### streamcount

```ts
streamcount: number = 255;
```

Defined in: [src/sensors/vl53l8/uld.ts:421](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L421)

***

### dataReadSize

```ts
dataReadSize: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:422](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L422)

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

Defined in: [src/sensors/vl53l8/uld.ts:424](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L424)

{ fw, host } when the FW disagrees with the api.c formula.

***

### lastBlocks

```ts
lastBlocks: [number, number, number][] = [];
```

Defined in: [src/sensors/vl53l8/uld.ts:426](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L426)

[idx, type, size] of the blocks in the last parsed frame.

***

### motionPresent

```ts
motionPresent: boolean = false;
```

Defined in: [src/sensors/vl53l8/uld.ts:428](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L428)

Whether the motion-indicator output block is configured.

## Methods

### dciReadData()

```ts
dciReadData(index, dataSize): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/sensors/vl53l8/uld.ts:521](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L521)

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

Defined in: [src/sensors/vl53l8/uld.ts:537](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L537)

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

Defined in: [src/sensors/vl53l8/uld.ts:565](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L565)

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

Defined in: [src/sensors/vl53l8/uld.ts:679](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L679)

Returns [deviceId, revisionId]; alive when (0xF0, 0x0C).

#### Returns

`Promise`\<\[`number`, `number`\]\>

***

### init()

```ts
init(progress?): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:688](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L688)

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

Defined in: [src/sensors/vl53l8/uld.ts:834](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L834)

#### Returns

`Promise`\<`number`\>

***

### setResolution()

```ts
setResolution(resolution): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:839](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L839)

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

Defined in: [src/sensors/vl53l8/uld.ts:871](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L871)

#### Returns

`Promise`\<`number`\>

***

### setRangingFrequencyHz()

```ts
setRangingFrequencyHz(hz): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:875](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L875)

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

Defined in: [src/sensors/vl53l8/uld.ts:879](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L879)

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

Defined in: [src/sensors/vl53l8/uld.ts:897](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L897)

#### Returns

`Promise`\<`number`\>

***

### getIntegrationTimeMs()

```ts
getIntegrationTimeMs(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:902](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L902)

#### Returns

`Promise`\<`number`\>

***

### setIntegrationTimeMs()

```ts
setIntegrationTimeMs(timeMs): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:908](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L908)

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

Defined in: [src/sensors/vl53l8/uld.ts:925](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L925)

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

Defined in: [src/sensors/vl53l8/uld.ts:930](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L930)

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

Defined in: [src/sensors/vl53l8/uld.ts:937](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L937)

#### Returns

`Promise`\<`number`\>

***

### setTargetOrder()

```ts
setTargetOrder(order): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:941](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L941)

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

Defined in: [src/sensors/vl53l8/uld.ts:956](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L956)

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

Defined in: [src/sensors/vl53l8/uld.ts:1054](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1054)

#### Returns

`Promise`\<`void`\>

***

### checkDataReady()

```ts
checkDataReady(): Promise<boolean>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1090](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1090)

#### Returns

`Promise`\<`boolean`\>

***

### getRangingData()

```ts
getRangingData(): Promise<Vl53l8Results>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1109](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1109)

Poll-mode read: fetch one results frame from reg 0x00 and parse it.

#### Returns

`Promise`\<[`Vl53l8Results`](../interfaces/Vl53l8Results.md)\>

***

### parseFrame()

```ts
parseFrame(raw): Vl53l8Results;
```

Defined in: [src/sensors/vl53l8/uld.ts:1122](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1122)

Parse one raw results frame (dataReadSize bytes read from reg 0x00).
Used both by poll-mode getRangingData() and by the host when the MCU
pushes frames over the INT-driven stream. Returns per-zone arrays
distanceMm, targetStatus, nbTargetDetected, signalPerSpad (kcps/SPAD),
ambientPerSpad (kcps/SPAD), nbSpadsEnabled, rangeSigmaMm, reflectance
(%), plus the per-frame scalar siliconTempDegc.

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

Defined in: [src/sensors/vl53l8/uld.ts:1269](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1269)

#### Returns

`Promise`\<`number`\>

***

### setPowerMode()

```ts
setPowerMode(powerMode): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1289](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1289)

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

Defined in: [src/sensors/vl53l8/uld.ts:1319](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1319)

Xtalk margin in kcps/spad.

#### Returns

`Promise`\<`number`\>

***

### setXtalkMargin()

```ts
setXtalkMargin(marginKcps): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1324](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1324)

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

Defined in: [src/sensors/vl53l8/uld.ts:1336](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1336)

Read the live 776-byte xtalk calibration blob back from the sensor FW (as
produced by calibrateXtalk). Restores the current resolution afterwards.

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### setCaldataXtalk()

```ts
setCaldataXtalk(xtalkData): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1355](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1355)

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

Defined in: [src/sensors/vl53l8/uld.ts:1366](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1366)

#### Returns

`Promise`\<`number`\>

***

### setDetectionThresholdsEnable()

```ts
setDetectionThresholdsEnable(enabled): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1370](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1370)

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

Defined in: [src/sensors/vl53l8/uld.ts:1385](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1385)

Return the 64 detection thresholds; low/high rescaled to real units.

#### Returns

`Promise`\<[`DetectionThreshold`](../interfaces/DetectionThreshold.md)[]\>

***

### setDetectionThresholds()

```ts
setDetectionThresholds(thresholds): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1410](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1410)

Program the 64 detection thresholds (missing entries default to zeros).

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

Defined in: [src/sensors/vl53l8/uld.ts:1416](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1416)

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

Defined in: [src/sensors/vl53l8/uld.ts:1427](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1427)

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

Defined in: [src/sensors/vl53l8/uld.ts:1434](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1434)

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

Defined in: [src/sensors/vl53l8/uld.ts:1439](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1439)

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

Defined in: [src/sensors/vl53l8/uld.ts:1523](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1523)

vl53l8cx_calibrate_xtalk: run on-device crosstalk calibration.

NOTE: ported from ST ULD source but NOT verified against live hardware in
this SDK — the get/set caldata-xtalk buffer path IS the tested save/restore
route. Saves & restores resolution/frequency/int-time/sharpener/target-
order/xtalk-margin/ranging-mode around the run.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `reflectancePercent` | `number` |
| `nbSamples` | `number` |
| `distanceMm` | `number` |

#### Returns

`Promise`\<`void`\>

# Class: Vl53l8Cx

Defined in: [src/sensors/vl53l8/vl53l8.ts:106](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L106)

VL53L8CX ToF device: `init()` downloads the ~84 KB sensor firmware (takes a
few seconds over CDC), then configure and `startRanging()`.

This is the base class for both silicon variants. The VL53L8CH superset
(compact-network-histogram output) lives in `Vl53l8Ch`, which inherits every
method here. All configuration methods require `init()` first and must not
be called while ranging (the ULD talks to the current register bank; the
stream owns it — contract 04).

## Extends

- `DepzDevice`

## Constructors

### Constructor

```ts
new Vl53l8Cx(transport, opts?): Vl53l8Cx;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:162](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L162)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | [`Vl53l8Options`](../interfaces/Vl53l8Options.md) |

#### Returns

`Vl53l8Cx`

#### Overrides

```ts
DepzDevice.constructor
```

## Properties

### timeoutMs

```ts
timeoutMs: number;
```

Defined in: [src/device/device.ts:178](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L178)

#### Inherited from

```ts
DepzDevice.timeoutMs
```

***

### link

```ts
protected readonly link: DepzLink;
```

Defined in: [src/device/device.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L180)

#### Inherited from

```ts
DepzDevice.link
```

***

### variantId

```ts
protected readonly variantId: Vl53l8Variant = "cx";
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:108](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L108)

Sensor-firmware blob variant this class loads.

***

### cnhConfig

```ts
protected cnhConfig: CnhConfig | null = null;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:115](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L115)

## Accessors

### stats

#### Get Signature

```ts
get stats(): LinkStats;
```

Defined in: [src/device/device.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L210)

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

Defined in: [src/device/device.ts:474](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L474)

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
get uld(): VL53L8CX;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L170)

The underlying ULD driver (escape hatch for advanced DCI access).

##### Returns

[`VL53L8CX`](VL53L8CX.md)

***

### variant

#### Get Signature

```ts
get variant(): Vl53l8Variant;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:176](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L176)

'cx' | 'ch' (valid after init()).

##### Returns

[`Vl53l8Variant`](../type-aliases/Vl53l8Variant.md)

***

### ranging

#### Get Signature

```ts
get ranging(): boolean;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:395](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L395)

##### Returns

`boolean`

## Methods

### open()

```ts
open(): Promise<void>;
```

Defined in: [src/device/device.ts:200](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L200)

Open the transport and start the read pump.

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

Defined in: [src/device/device.ts:205](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L205)

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
DepzDevice.close
```

***

### registerStream()

```ts
protected registerStream(stream): () => void;
```

Defined in: [src/device/device.ts:330](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L330)

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

Defined in: [src/device/device.ts:343](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L343)

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

Defined in: [src/device/device.ts:350](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L350)

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

Defined in: [src/device/device.ts:366](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L366)

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

Defined in: [src/device/device.ts:409](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L409)

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

Defined in: [src/device/device.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L414)

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

Defined in: [src/device/device.ts:425](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L425)

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

Defined in: [src/device/device.ts:431](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L431)

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

Defined in: [src/device/device.ts:437](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L437)

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

Defined in: [src/device/device.ts:444](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L444)

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

Defined in: [src/device/device.ts:449](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L449)

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

Defined in: [src/device/device.ts:457](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L457)

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

Defined in: [src/device/device.ts:479](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L479)

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

Defined in: [src/device/device.ts:484](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L484)

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

Defined in: [src/device/device.ts:491](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L491)

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

Defined in: [src/device/device.ts:495](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L495)

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

Defined in: [src/device/device.ts:501](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L501)

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

Defined in: [src/device/device.ts:506](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L506)

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

Defined in: [src/device/device.ts:515](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L515)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L180)

#### Returns

`Promise`\<`boolean`\>

***

### init()

```ts
init(variant?, opts?): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L198)

Initialize the sensor: firmware blob download + default config.

The blob variant is fixed by the class (`Vl53l8Cx` → 'cx', `Vl53l8Ch` →
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

Defined in: [src/sensors/vl53l8/vl53l8.ts:218](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L218)

#### Returns

`Promise`\<`number`\>

***

### setResolution()

```ts
setResolution(zones): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:223](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L223)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:232](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L232)

#### Returns

`Promise`\<`number`\>

***

### setRangingFrequencyHz()

```ts
setRangingFrequencyHz(hz): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:236](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L236)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:247](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L247)

#### Returns

`Promise`\<`number`\>

***

### setRangingMode()

```ts
setRangingMode(mode): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:251](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L251)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:256](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L256)

#### Returns

`Promise`\<`number`\>

***

### setIntegrationTimeMs()

```ts
setIntegrationTimeMs(ms): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:260](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L260)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:265](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L265)

#### Returns

`Promise`\<`number`\>

***

### setSharpenerPercent()

```ts
setSharpenerPercent(pct): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:269](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L269)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:274](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L274)

#### Returns

`Promise`\<`number`\>

***

### setTargetOrder()

```ts
setTargetOrder(order): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:278](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L278)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:286](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L286)

POWER_MODE_SLEEP/WAKEUP/DEEP_SLEEP (uld constants).

#### Returns

`Promise`\<`number`\>

***

### setPowerMode()

```ts
setPowerMode(mode): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:294](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L294)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:299](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L299)

#### Returns

`Promise`\<`number`\>

***

### setXtalkMargin()

```ts
setXtalkMargin(marginKcps): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:303](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L303)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:314](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L314)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:320](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L320)

Read back the 776-byte xtalk calibration blob (save/restore).

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### setCaldataXtalk()

```ts
setCaldataXtalk(blob): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:326](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L326)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:331](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L331)

#### Returns

`Promise`\<`number`\>

***

### setDetectionThresholdsEnable()

```ts
setDetectionThresholdsEnable(enabled): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:335](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L335)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:340](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L340)

#### Returns

`Promise`\<[`DetectionThreshold`](../interfaces/DetectionThreshold.md)[]\>

***

### setDetectionThresholds()

```ts
setDetectionThresholds(thresholds): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:349](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L349)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:354](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L354)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:364](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L364)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:375](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L375)

Configure the output list, start the sensor and the MCU stream.

#### Returns

`Promise`\<`void`\>

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:388](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L388)

#### Returns

`Promise`\<`void`\>

***

### onFrame()

```ts
onFrame(cb): () => void;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:400](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L400)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:413](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L413)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:424](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L424)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:452](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L452)

#### Returns

`void`

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:458](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L458)

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

Defined in: [src/sensors/vl53l8/uld.ts:179](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L179)

## Extends

- `Error`

## Constructors

### Constructor

```ts
new Vl53l8cxError(code, where?): Vl53l8cxError;
```

Defined in: [src/sensors/vl53l8/uld.ts:183](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L183)

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

Defined in: [src/sensors/vl53l8/uld.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L180)

***

### where

```ts
readonly where: string;
```

Defined in: [src/sensors/vl53l8/uld.ts:181](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L181)

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

Defined in: [src/sensors/vl53l8/uld.ts:357](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L357)

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

Defined in: [src/sensors/vl53l8/assets/index.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L20)

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

Defined in: [src/sensors/vl53l8/uld.ts:342](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L342)

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

Defined in: [src/sensors/vl53l8/uld.ts:386](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L386)

Pack 64 detection thresholds into the DCI_DET_THRESH_START payload plus the
8-byte valid-status block (pure — mirror of set_detection_thresholds). Each
threshold's low/high are scaled by its measurement selector.

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

Defined in: [src/sensors/vl53l8/uld.ts:193](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L193)

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

Defined in: [src/sensors/vl53l8/uld.ts:377](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L377)

Xtalk margin (kcps/spad) → raw DCI value (round(kcps * 2048)).

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L73)

Zone array reshaped to (4,4) or (8,8) — mirror of Vl53l8Frame.grid().

## Parameters

| Parameter | Type |
| ------ | ------ |
| `arr` | `ArrayLike`\<`number`\> |
| `resolution` | `number` |

## Returns

`number`[][]

# Interface: DetectionThreshold

Defined in: [src/sensors/vl53l8/uld.ts:247](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L247)

One detection-threshold entry (real units; mirror of the Python dict).

## Properties

### lowThresh

```ts
lowThresh: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:248](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L248)

***

### highThresh

```ts
highThresh: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:249](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L249)

***

### measurement

```ts
measurement: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:250](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L250)

***

### type

```ts
type: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:251](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L251)

***

### zoneNum

```ts
zoneNum: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:252](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L252)

***

### operation

```ts
operation: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:253](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L253)

# Interface: MotionResult

Defined in: [src/sensors/vl53l8/uld.ts:257](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L257)

Motion-indicator results block (mirror of the Python `motion_indicator` dict).

## Properties

### globalIndicator1

```ts
globalIndicator1: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:258](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L258)

***

### globalIndicator2

```ts
globalIndicator2: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:259](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L259)

***

### status

```ts
status: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:260](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L260)

***

### nbOfDetectedAggregates

```ts
nbOfDetectedAggregates: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:261](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L261)

***

### nbOfAggregates

```ts
nbOfAggregates: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:262](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L262)

***

### motion

```ts
motion: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:263](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L263)

# Interface: Vl53l8Assets

Defined in: [src/sensors/vl53l8/assets/index.ts:9](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L9)

## Properties

### firmware

```ts
firmware: Uint8Array;
```

Defined in: [src/sensors/vl53l8/assets/index.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L11)

firmware.bin — 86016 bytes (0x15000).

***

### defaultCfg

```ts
defaultCfg: Uint8Array;
```

Defined in: [src/sensors/vl53l8/assets/index.ts:13](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L13)

default_configuration.bin — 972 bytes.

***

### defaultXtalk

```ts
defaultXtalk: Uint8Array;
```

Defined in: [src/sensors/vl53l8/assets/index.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L15)

default_xtalk.bin — 776 bytes.

***

### getNvmCmd

```ts
getNvmCmd: Uint8Array;
```

Defined in: [src/sensors/vl53l8/assets/index.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L17)

get_nvm_cmd.bin — 40 bytes.

# Interface: Vl53l8Frame

Defined in: [src/sensors/vl53l8/vl53l8.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L49)

One parsed ranging frame. Arrays are sized to the active resolution
(16 or 64 zones); zone index runs row-major (see datasheet zone maps).

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L50)

***

### resolution

```ts
resolution: number;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L52)

16 | 64

***

### distanceMm

```ts
distanceMm: Int32Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L53)

***

### targetStatus

```ts
targetStatus: Uint8Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L55)

5/9 = valid, 255 = no target.

***

### nbTargetDetected

```ts
nbTargetDetected: Uint8Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L56)

***

### signalPerSpad

```ts
signalPerSpad: Float64Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:58](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L58)

kcps/SPAD.

***

### ambientPerSpad

```ts
ambientPerSpad: Float64Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L60)

kcps/SPAD.

***

### nbSpadsEnabled

```ts
nbSpadsEnabled: Int32Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:61](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L61)

***

### rangeSigmaMm

```ts
rangeSigmaMm: Float64Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:62](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L62)

***

### reflectance

```ts
reflectance: Uint8Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L64)

%.

***

### siliconTempDegc

```ts
siliconTempDegc: number;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:65](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L65)

***

### cnhRaw

```ts
cnhRaw: Uint8Array<ArrayBufferLike> | null;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L67)

CH variant: raw CNH block (decode via cnh).

***

### motion

```ts
motion: MotionResult | null;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:69](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L69)

Motion-indicator output when `configureMotionIndicator()` is active.

# Interface: Vl53l8InitOptions

Defined in: [src/sensors/vl53l8/vl53l8.ts:89](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L89)

## Properties

### progress?

```ts
optional progress?: (text) => void;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:91](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L91)

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

Defined in: [src/sensors/vl53l8/vl53l8.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L93)

Tracks the big blob writes: (done, total) bytes.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `done` | `number` |
| `total` | `number` |

#### Returns

`void`

# Interface: Vl53l8Options

Defined in: [src/sensors/vl53l8/vl53l8.ts:84](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L84)

## Extends

- `DeviceOptions`

## Properties

### timeoutMs?

```ts
optional timeoutMs?: number;
```

Defined in: [src/device/device.ts:169](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L169)

#### Inherited from

```ts
DeviceOptions.timeoutMs
```

***

### txCrcType?

```ts
optional txCrcType?: CrcType;
```

Defined in: [src/device/device.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L170)

#### Inherited from

```ts
DeviceOptions.txCrcType
```

***

### sleepImpl?

```ts
optional sleepImpl?: (ms) => Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:86](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L86)

ULD sleep implementation (tests inject an instant one).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Interface: Vl53l8Platform

Defined in: [src/sensors/vl53l8/uld.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L19)

ULD platform: chunking/bridging is the caller's concern.

## Methods

### rdMulti()

```ts
rdMulti(addr, size): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/sensors/vl53l8/uld.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L20)

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

Defined in: [src/sensors/vl53l8/uld.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L21)

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

Defined in: [src/sensors/vl53l8/uld.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L22)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Interface: Vl53l8Results

Defined in: [src/sensors/vl53l8/uld.ts:267](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L267)

One parsed raw results frame (plain arrays, mirroring the Python dict).

## Properties

### distanceMm

```ts
distanceMm: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:268](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L268)

***

### targetStatus

```ts
targetStatus: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:269](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L269)

***

### nbTargetDetected

```ts
nbTargetDetected: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:270](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L270)

***

### signalPerSpad

```ts
signalPerSpad: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:271](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L271)

***

### ambientPerSpad

```ts
ambientPerSpad: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:272](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L272)

***

### nbSpadsEnabled

```ts
nbSpadsEnabled: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:273](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L273)

***

### rangeSigmaMm

```ts
rangeSigmaMm: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:274](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L274)

***

### reflectance

```ts
reflectance: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:275](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L275)

***

### siliconTempDegc

```ts
siliconTempDegc: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:276](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L276)

***

### cnhRaw

```ts
cnhRaw: Uint8Array<ArrayBufferLike> | null;
```

Defined in: [src/sensors/vl53l8/uld.ts:277](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L277)

***

### motion

```ts
motion: MotionResult | null;
```

Defined in: [src/sensors/vl53l8/uld.ts:279](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L279)

Motion-indicator output when the motion detector is configured.

# Type Alias: Vl53l8

```ts
type Vl53l8 = Vl53l8Cx;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:521](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L521)

Backward-compatible alias: the old flat `Vl53l8` name maps to the CX base
class (the historic default). New code should pick Vl53l8Cx / Vl53l8Ch.

# Type Alias: Vl53l8Variant

```ts
type Vl53l8Variant = "cx" | "ch";
```

Defined in: [src/sensors/vl53l8/assets/index.ts:7](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L7)

Lazy loader for the VL53L8 blob assets (sensor firmware + NVM/config
blobs). `loadAssets()` uses dynamic `import()` so the ~115 KB base64
modules stay out of bundles that never init the ToF sensor.

# Variable: CALIBRATE\_XTALK

```ts
const CALIBRATE_XTALK: Uint8Array<ArrayBufferLike>;
```

Defined in: [src/sensors/vl53l8/uld.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L163)

# Variable: FW\_CHECKSUM

```ts
const FW_CHECKSUM: Record<Vl53l8Variant, number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L29)

# Variable: GET\_XTALK\_CMD

```ts
const GET_XTALK_CMD: Uint8Array<ArrayBufferLike>;
```

Defined in: [src/sensors/vl53l8/uld.ts:158](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L158)

# Variable: MIN\_RANGING\_FREQUENCY\_HZ

```ts
const MIN_RANGING_FREQUENCY_HZ: 2 = 2;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L43)

Below this the sensor never streams (contract 04).

# Variable: NB\_THRESHOLDS

```ts
const NB_THRESHOLDS: 64 = 64;
```

Defined in: [src/sensors/vl53l8/uld.ts:112](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L112)

# Variable: POWER\_MODE\_DEEP\_SLEEP

```ts
const POWER_MODE_DEEP_SLEEP: 2 = 2;
```

Defined in: [src/sensors/vl53l8/uld.ts:118](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L118)

# Variable: POWER\_MODE\_SLEEP

```ts
const POWER_MODE_SLEEP: 0 = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:116](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L116)

# Variable: POWER\_MODE\_WAKEUP

```ts
const POWER_MODE_WAKEUP: 1 = 1;
```

Defined in: [src/sensors/vl53l8/uld.ts:117](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L117)

# Variable: RANGING\_MODE\_AUTONOMOUS

```ts
const RANGING_MODE_AUTONOMOUS: 3 = 3;
```

Defined in: [src/sensors/vl53l8/uld.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L38)

# Variable: RANGING\_MODE\_CONTINUOUS

```ts
const RANGING_MODE_CONTINUOUS: 1 = 1;
```

Defined in: [src/sensors/vl53l8/uld.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L37)

# Variable: RESOLUTION\_4X4

```ts
const RESOLUTION_4X4: 16 = 16;
```

Defined in: [src/sensors/vl53l8/uld.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L34)

# Variable: RESOLUTION\_8X8

```ts
const RESOLUTION_8X8: 64 = 64;
```

Defined in: [src/sensors/vl53l8/uld.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L35)

# Variable: TARGET\_ORDER\_CLOSEST

```ts
const TARGET_ORDER_CLOSEST: 1 = 1;
```

Defined in: [src/sensors/vl53l8/uld.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L40)

# Variable: TARGET\_ORDER\_STRONGEST

```ts
const TARGET_ORDER_STRONGEST: 2 = 2;
```

Defined in: [src/sensors/vl53l8/uld.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L41)

# Variable: THRESH\_IN\_WINDOW

```ts
const THRESH_IN_WINDOW: 0 = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:140](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L140)

# Variable: THRESH\_OP\_AND

```ts
const THRESH_OP_AND: 2 = 2;
```

Defined in: [src/sensors/vl53l8/uld.ts:149](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L149)

# Variable: THRESH\_OP\_NONE

```ts
const THRESH_OP_NONE: 0 = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:147](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L147)

# Variable: THRESH\_OP\_OR

```ts
const THRESH_OP_OR: 0 = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:148](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L148)

# Variable: THRESH\_OUT\_OF\_WINDOW

```ts
const THRESH_OUT_OF_WINDOW: 1 = 1;
```

Defined in: [src/sensors/vl53l8/uld.ts:141](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L141)

# Variable: Vl53l8

```ts
const Vl53l8: typeof Vl53l8Cx = Vl53l8Cx;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:521](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L521)

Backward-compatible alias: the old flat `Vl53l8` name maps to the CX base
class (the historic default). New code should pick Vl53l8Cx / Vl53l8Ch.

