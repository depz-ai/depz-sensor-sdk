# VL53L7CH (ToF + CNH) — API reference

The `Vl53l7ch` class (the `Vl53l7cx` superset that adds
`configureCnh()`) plus the Compact-Network-Histogram (CNH) symbols.
Board commands are in the [VL53L7CX reference](../vl53l7cx/api.md), the
shared ToF surface in the [VL53L8CX reference](../vl53l8cx/api.md);
cross-sensor symbols live in the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Classes

- [Vl53l7ch](classes/Vl53l7ch.md)
- [CnhConfigError](classes/CnhConfigError.md)
- [CnhConfig](classes/CnhConfig.md)

## Interfaces

- [CnhAggregate](interfaces/CnhAggregate.md)
- [CnhDecoded](interfaces/CnhDecoded.md)

## Variables

- [CNH\_BIN\_WIDTH\_MM](variables/CNH_BIN_WIDTH_MM.md)
- [MI\_CFG\_DEV\_IDX](variables/MI_CFG_DEV_IDX.md)
- [CNH\_MAX\_DATA\_BYTES](variables/CNH_MAX_DATA_BYTES.md)

## Functions

- [cnhMaxBins](functions/cnhMaxBins.md)
- [decodeCnh](functions/decodeCnh.md)

# Class: CnhConfig

Defined in: [sensors/vl53l8/cnh.ts:69](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L69)

Mirror of VL53LMZ_Motion_Configuration plus the helpers that fill it.
Build with initConfig()/createAggMap(), check size with requiredMemory(),
then pack() the 156-byte struct for cnh_send_config.

## Constructors

### Constructor

```ts
new CnhConfig(): CnhConfig;
```

#### Returns

`CnhConfig`

## Properties

### refBinOffset

```ts
refBinOffset: number = 0;
```

Defined in: [sensors/vl53l8/cnh.ts:70](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L70)

***

### detectionThreshold

```ts
detectionThreshold: number = 0;
```

Defined in: [sensors/vl53l8/cnh.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L71)

***

### extraNoiseSigma

```ts
extraNoiseSigma: number = 0;
```

Defined in: [sensors/vl53l8/cnh.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L72)

***

### nullDenClipValue

```ts
nullDenClipValue: number = 0;
```

Defined in: [sensors/vl53l8/cnh.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L73)

***

### memUpdateMode

```ts
memUpdateMode: number = 0;
```

Defined in: [sensors/vl53l8/cnh.ts:74](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L74)

***

### memUpdateChoice

```ts
memUpdateChoice: number = 0;
```

Defined in: [sensors/vl53l8/cnh.ts:75](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L75)

***

### sumSpan

```ts
sumSpan: number = 0;
```

Defined in: [sensors/vl53l8/cnh.ts:76](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L76)

***

### featureLength

```ts
featureLength: number = 0;
```

Defined in: [sensors/vl53l8/cnh.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L77)

***

### nbOfAggregates

```ts
nbOfAggregates: number = 0;
```

Defined in: [sensors/vl53l8/cnh.ts:78](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L78)

***

### nbOfTemporalAccumulations

```ts
nbOfTemporalAccumulations: number = 1;
```

Defined in: [sensors/vl53l8/cnh.ts:79](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L79)

***

### minNbForGlobalDetection

```ts
minNbForGlobalDetection: number = 0;
```

Defined in: [sensors/vl53l8/cnh.ts:80](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L80)

***

### globalIndicatorFormat1

```ts
globalIndicatorFormat1: number = 0;
```

Defined in: [sensors/vl53l8/cnh.ts:81](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L81)

***

### globalIndicatorFormat2

```ts
globalIndicatorFormat2: number = 0;
```

Defined in: [sensors/vl53l8/cnh.ts:82](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L82)

***

### cnhCfg

```ts
cnhCfg: number = 0;
```

Defined in: [sensors/vl53l8/cnh.ts:83](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L83)

***

### cnhFlexShift

```ts
cnhFlexShift: number = 0;
```

Defined in: [sensors/vl53l8/cnh.ts:84](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L84)

***

### spare3

```ts
spare3: number = 0;
```

Defined in: [sensors/vl53l8/cnh.ts:85](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L85)

***

### mapId

```ts
mapId: number[];
```

Defined in: [sensors/vl53l8/cnh.ts:86](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L86)

***

### indicatorFormat1

```ts
indicatorFormat1: number[];
```

Defined in: [sensors/vl53l8/cnh.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L87)

***

### indicatorFormat2

```ts
indicatorFormat2: number[];
```

Defined in: [sensors/vl53l8/cnh.ts:88](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L88)

## Methods

### initConfig()

```ts
initConfig(
   startBin, 
   numBins, 
   subSample): void;
```

Defined in: [sensors/vl53l8/cnh.ts:95](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L95)

startBin: first device-histogram bin; numBins: CNH bins;
subSample: bins of the device histogram summed per CNH bin.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `startBin` | `number` |
| `numBins` | `number` |
| `subSample` | `number` |

#### Returns

`void`

***

### createAggMap()

```ts
createAggMap(
   resolution, 
   startX, 
   startY, 
   mergeX, 
   mergeY, 
   cols, 
   rows): void;
```

Defined in: [sensors/vl53l8/cnh.ts:132](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L132)

Map device zones to CNH aggregates. resolution: 16 (4x4) or 64 (8x8)
— must match the value passed to setResolution().

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `resolution` | `number` |
| `startX` | `number` |
| `startY` | `number` |
| `mergeX` | `number` |
| `mergeY` | `number` |
| `cols` | `number` |
| `rows` | `number` |

#### Returns

`void`

***

### requiredMemory()

```ts
requiredMemory(): number;
```

Defined in: [sensors/vl53l8/cnh.ts:173](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L173)

On-device CNH buffer size in bytes for this config. Throws if the
config is blank or the size exceeds CNH_MAX_DATA_BYTES.

#### Returns

`number`

***

### minMaxDistanceMm()

```ts
minMaxDistanceMm(): [number, number];
```

Defined in: [sensors/vl53l8/cnh.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L188)

[min, max] target distance, in mm, fully captured by the histogram.

#### Returns

\[`number`, `number`\]

***

### binCenterMm()

```ts
binCenterMm(binIdx): number;
```

Defined in: [sensors/vl53l8/cnh.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L198)

Distance (mm) at the centre of CNH histogram bin `binIdx`.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `binIdx` | `number` |

#### Returns

`number`

***

### pack()

```ts
pack(): Uint8Array;
```

Defined in: [sensors/vl53l8/cnh.ts:204](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L204)

#### Returns

`Uint8Array`

# Class: CnhConfigError

Defined in: [sensors/vl53l8/cnh.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L57)

## Extends

- `Error`

## Constructors

### Constructor

```ts
new CnhConfigError(message?): CnhConfigError;
```

Defined in: [sensors/vl53l8/cnh.ts:58](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L58)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `message?` | `string` |

#### Returns

`CnhConfigError`

#### Overrides

```ts
Error.constructor
```

## Properties

### stackTraceLimit

```ts
static stackTraceLimit: number;
```

Defined in: ../node\_modules/@types/node/globals.d.ts:68

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

Defined in: ../node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

```ts
Error.cause
```

***

### name

```ts
name: string;
```

Defined in: ../node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

```ts
Error.name
```

***

### message

```ts
message: string;
```

Defined in: ../node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

```ts
Error.message
```

***

### stack?

```ts
optional stack?: string;
```

Defined in: ../node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

```ts
Error.stack
```

## Methods

### captureStackTrace()

```ts
static captureStackTrace(targetObject, constructorOpt?): void;
```

Defined in: ../node\_modules/@types/node/globals.d.ts:52

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

Defined in: ../node\_modules/@types/node/globals.d.ts:56

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

# Class: Vl53l7ch

Defined in: [sensors/vl53l7/vl53l7.ts:156](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L156)

VL53L7CH ToF device: the VL53L7CX superset. `init()` downloads the CH
firmware blob (VL53LMZ ULD 2.0.16, the same blob as VL53L8CH) and adds
Compact-Network-Histogram output (`configureCnh`). CNH frames up to ~7.6 KB
stream in chunks (≤ 8192 B total).

## Extends

- `Vl53l7cx`

## Constructors

### Constructor

```ts
new Vl53l7ch(transport, opts?): Vl53l7ch;
```

Defined in: [sensors/vl53l8/vl53l8.ts:193](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L193)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | `Vl53l8Options` |

#### Returns

`Vl53l7ch`

#### Inherited from

```ts
Vl53l7cx.constructor
```

## Properties

### timeoutMs

```ts
timeoutMs: number;
```

Defined in: [device/device.ts:219](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L219)

#### Inherited from

```ts
Vl53l7cx.timeoutMs
```

***

### link

```ts
protected readonly link: DepzLink;
```

Defined in: [device/device.ts:221](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L221)

#### Inherited from

```ts
Vl53l7cx.link
```

***

### readChunk

```ts
protected readonly readChunk: number = VL53L7_READ_MAX_LEN;
```

Defined in: [sensors/vl53l7/vl53l7.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L54)

Register-bridge transfer limits (the I2C L5/L7 board reads less per call).

#### Inherited from

```ts
Vl53l7cx.readChunk
```

***

### writeChunk

```ts
protected readonly writeChunk: number = VL53L7_WRITE_MAX_LEN;
```

Defined in: [sensors/vl53l7/vl53l7.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L55)

#### Inherited from

```ts
Vl53l7cx.writeChunk
```

***

### streamChunk

```ts
protected readonly streamChunk: number = VL53L7_STREAM_CHUNK_MAX;
```

Defined in: [sensors/vl53l7/vl53l7.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L56)

Largest RPT_VL53_FRAME chunk the board emits (contract 04 §4 rule 4).

#### Inherited from

```ts
Vl53l7cx.streamChunk
```

***

### minRangingHz

```ts
protected readonly minRangingHz: number = VL53L7_MIN_RANGING_FREQUENCY_HZ;
```

Defined in: [sensors/vl53l7/vl53l7.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L57)

Lowest ranging frequency that actually streams on this sensor.

#### Inherited from

```ts
Vl53l7cx.minRangingHz
```

***

### expectedModuleType

```ts
protected readonly expectedModuleType: number = MODULE_TYPE_MZEVO;
```

Defined in: [sensors/vl53l7/vl53l7.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L59)

module_type this class expects after init() (L5 and L7 share a blob).

#### Inherited from

```ts
Vl53l7cx.expectedModuleType
```

***

### variantId

```ts
protected readonly variantId: Vl53l8Variant = "l7ch";
```

Defined in: [sensors/vl53l7/vl53l7.ts:157](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L157)

#### Overrides

```ts
Vl53l7cx.variantId
```

***

### uldDriver

```ts
protected uldDriver: VL53L8CX | null = null;
```

Defined in: [sensors/vl53l8/vl53l8.ts:131](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L131)

#### Inherited from

```ts
Vl53l7cx.uldDriver
```

***

### rangingFlag

```ts
protected rangingFlag: boolean = false;
```

Defined in: [sensors/vl53l8/vl53l8.ts:135](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L135)

#### Inherited from

```ts
Vl53l7cx.rangingFlag
```

***

### cnhConfig

```ts
protected cnhConfig: CnhConfig | null = null;
```

Defined in: [sensors/vl53l8/vl53l8.ts:136](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L136)

#### Inherited from

```ts
Vl53l7cx.cnhConfig
```

## Accessors

### stats

#### Get Signature

```ts
get stats(): LinkStats;
```

Defined in: [device/device.ts:285](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L285)

##### Returns

`LinkStats`

#### Inherited from

```ts
Vl53l7cx.stats
```

***

### timeSync

#### Get Signature

```ts
get timeSync(): TimeSync | null;
```

Defined in: [device/device.ts:628](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L628)

##### Returns

`TimeSync` \| `null`

#### Inherited from

```ts
Vl53l7cx.timeSync
```

***

### moduleType

#### Get Signature

```ts
get moduleType(): number | null;
```

Defined in: [sensors/vl53l7/vl53l7.ts:83](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L83)

Sensor module type read at init(): MODULE_TYPE_MZ (0) = VL53L5CX,
MODULE_TYPE_MZEVO (1) = VL53L7CX/CH. null before init().

##### Returns

`number` \| `null`

#### Inherited from

```ts
Vl53l7cx.moduleType
```

***

### xtalkCalibrationFailed

#### Get Signature

```ts
get xtalkCalibrationFailed(): boolean;
```

Defined in: [sensors/vl53l7/vl53l7.ts:137](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L137)

True when the last calibrateXtalk() found nothing to calibrate (ST
XTALK_FAILED: "coverglass too good") — the sensor keeps its default xtalk.

##### Returns

`boolean`

#### Inherited from

```ts
Vl53l7cx.xtalkCalibrationFailed
```

***

### activeCnhConfig

#### Get Signature

```ts
get activeCnhConfig(): CnhConfig | null;
```

Defined in: [sensors/vl53l8/vl53l8.ts:141](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L141)

The CNH config armed by `configureCnh()` (CH), or null. Recorders read
 this to persist the decode parameters next to raw CNH blocks.

##### Returns

[`CnhConfig`](CnhConfig.md) \| `null`

#### Inherited from

```ts
Vl53l7cx.activeCnhConfig
```

***

### uld

#### Get Signature

```ts
get uld(): VL53L8CX;
```

Defined in: [sensors/vl53l8/vl53l8.ts:201](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L201)

The underlying ULD driver (escape hatch for advanced DCI access).

##### Returns

`VL53L8CX`

#### Inherited from

```ts
Vl53l7cx.uld
```

***

### variant

#### Get Signature

```ts
get variant(): Vl53l8Variant;
```

Defined in: [sensors/vl53l8/vl53l8.ts:207](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L207)

'cx' | 'ch' | 'l7cx' | 'l7ch' (valid after init()).

##### Returns

`Vl53l8Variant`

#### Inherited from

```ts
Vl53l7cx.variant
```

***

### ranging

#### Get Signature

```ts
get ranging(): boolean;
```

Defined in: [sensors/vl53l8/vl53l8.ts:472](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L472)

##### Returns

`boolean`

#### Inherited from

```ts
Vl53l7cx.ranging
```

***

### frameParseErrors

#### Get Signature

```ts
get frameParseErrors(): number;
```

Defined in: [sensors/vl53l8/vl53l8.ts:480](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L480)

Frames dropped because the ULD could not parse them (corrupt frame, bad
size). Distinct from reassembly gaps. Mirrors Python `frame_parse_errors`.

##### Returns

`number`

#### Inherited from

```ts
Vl53l7cx.frameParseErrors
```

## Methods

### open()

```ts
open(): Promise<void>;
```

Defined in: [device/device.ts:251](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L251)

Open the transport and start the read pump. A no-op on a device that
continues an adopted connection (`attachSensorClass`) — already open.

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.open
```

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [device/device.ts:258](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L258)

Close the connection. A device that handed its connection over owns none.

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.close
```

***

### onTeardown()

```ts
protected onTeardown(error): void;
```

Defined in: [device/device.ts:317](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L317)

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
Vl53l7cx.onTeardown
```

***

### registerStream()

```ts
protected registerStream(stream): () => void;
```

Defined in: [device/device.ts:464](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L464)

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
Vl53l7cx.registerStream
```

***

### onEvent()

```ts
onEvent(cb): () => void;
```

Defined in: [device/device.ts:477](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L477)

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
Vl53l7cx.onEvent
```

***

### emitEvent()

```ts
protected emitEvent(event): void;
```

Defined in: [device/device.ts:484](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L484)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `event` | `DeviceEvent` |

#### Returns

`void`

#### Inherited from

```ts
Vl53l7cx.emitEvent
```

***

### callGuarded()

```ts
protected callGuarded<T>(cb, item): void;
```

Defined in: [device/device.ts:493](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L493)

Run a user callback on the read pump: one that throws is counted in
`stats.callbackErrors`, logged and dropped — it never stops the pump, the
other callbacks or the queues (contract 07 §3).

#### Type Parameters

| Type Parameter |
| ------ |
| `T` |

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`item`) => `void` |
| `item` | `T` |

#### Returns

`void`

#### Inherited from

```ts
Vl53l7cx.callGuarded
```

***

### request()

```ts
request<T>(
   cmd, 
   payload?, 
opts?): Promise<T>;
```

Defined in: [device/device.ts:509](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L509)

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
Vl53l7cx.request
```

***

### expectReport()

```ts
static expectReport<T>(reportId, unpack): Matcher<T>;
```

Defined in: [device/device.ts:563](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L563)

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
Vl53l7cx.expectReport
```

***

### expectText()

```ts
static expectText(requestCmd): Matcher<string>;
```

Defined in: [device/device.ts:568](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L568)

Matcher for RPT_TEXT echoing `requestCmd`.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `requestCmd` | `number` |

#### Returns

`Matcher`\<`string`\>

#### Inherited from

```ts
Vl53l7cx.expectText
```

***

### getDeviceName()

```ts
getDeviceName(): Promise<string>;
```

Defined in: [device/device.ts:579](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L579)

#### Returns

`Promise`\<`string`\>

#### Inherited from

```ts
Vl53l7cx.getDeviceName
```

***

### getSoftwareName()

```ts
getSoftwareName(): Promise<string>;
```

Defined in: [device/device.ts:585](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L585)

#### Returns

`Promise`\<`string`\>

#### Inherited from

```ts
Vl53l7cx.getSoftwareName
```

***

### getSerialNumber()

```ts
getSerialNumber(): Promise<string>;
```

Defined in: [device/device.ts:591](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L591)

#### Returns

`Promise`\<`string`\>

#### Inherited from

```ts
Vl53l7cx.getSerialNumber
```

***

### identify()

```ts
identify(): Promise<Identity>;
```

Defined in: [device/device.ts:598](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L598)

Classify the running firmware (contract 02 §4).

#### Returns

`Promise`\<`Identity`\>

#### Inherited from

```ts
Vl53l7cx.identify
```

***

### readMcuTemperature()

```ts
readMcuTemperature(): Promise<number>;
```

Defined in: [device/device.ts:603](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L603)

Last cached MCU temperature in °C (device refreshes ~2 Hz).

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l7cx.readMcuTemperature
```

***

### syncTime()

```ts
syncTime(samples?): Promise<TimeSync>;
```

Defined in: [device/device.ts:611](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L611)

NTP-style sync; keeps the lowest-RTT sample (contract 02 §5).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `samples` | `number` | `5` |

#### Returns

`Promise`\<`TimeSync`\>

#### Inherited from

```ts
Vl53l7cx.syncTime
```

***

### toHostTimeUs()

```ts
toHostTimeUs(deviceTsUs): bigint;
```

Defined in: [device/device.ts:633](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L633)

Device µs → host monotonic µs (requires a prior `syncTime`).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `deviceTsUs` | `bigint` |

#### Returns

`bigint`

#### Inherited from

```ts
Vl53l7cx.toHostTimeUs
```

***

### getReportPayloadCrc()

```ts
getReportPayloadCrc(): Promise<CrcType>;
```

Defined in: [device/device.ts:638](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L638)

#### Returns

`Promise`\<`CrcType`\>

#### Inherited from

```ts
Vl53l7cx.getReportPayloadCrc
```

***

### setReportPayloadCrc()

```ts
setReportPayloadCrc(crcType): Promise<void>;
```

Defined in: [device/device.ts:645](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L645)

Set the device→host payload CRC mode (host→device is per-packet).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `crcType` | `CrcType` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.setReportPayloadCrc
```

***

### getSyncPin()

```ts
getSyncPin(pin): Promise<SyncPinConfig>;
```

Defined in: [device/device.ts:649](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L649)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pin` | `number` |

#### Returns

`Promise`\<`SyncPinConfig`\>

#### Inherited from

```ts
Vl53l7cx.getSyncPin
```

***

### setSyncPin()

```ts
setSyncPin(config): Promise<void>;
```

Defined in: [device/device.ts:655](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L655)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `config` | `SyncPinConfig` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.setSyncPin
```

***

### reset()

```ts
reset(): Promise<void>;
```

Defined in: [device/device.ts:660](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L660)

DEVICE_RESET: device ACKs then reboots; the link will drop.

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.reset
```

***

### enterBootloaderMode()

```ts
enterBootloaderMode(): Promise<void>;
```

Defined in: [device/device.ts:669](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L669)

Ask the device to reboot into the resident bootloader and close this
connection. Re-discovery/flash flow lives in the bootloader module
(contract 06).

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.enterBootloaderMode
```

***

### init()

```ts
init(variant?, opts?): Promise<void>;
```

Defined in: [sensors/vl53l7/vl53l7.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L67)

Initialize the sensor: firmware blob download + default config. Refuses
non-L5/L7 silicon. The sensor's `moduleType` then tells L5 (MZ) from L7
(MZEVO); a mismatch with this class is reported as a warning — ranging
still works, since the blob is shared.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `variant?` | `Vl53l8Variant` |
| `opts?` | `Vl53l8InitOptions` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.init
```

***

### getBridgeInfo()

```ts
getBridgeInfo(): Promise<Vl53l7Info>;
```

Defined in: [sensors/vl53l7/vl53l7.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L94)

Bridge counters and pin levels (never touches the sensor). Read it before
and after a run, not during one: each call takes the bus from the stream
and can itself cost a frame.

#### Returns

`Promise`\<`Vl53l7Info`\>

#### Inherited from

```ts
Vl53l7cx.getBridgeInfo
```

***

### setI2cSpeedKhz()

```ts
setI2cSpeedKhz(khz): Promise<number>;
```

Defined in: [sensors/vl53l7/vl53l7.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L105)

Set the sensor-bus SCL frequency; the board snaps to the nearest of 100,
200, 400, 500 … 1000 kHz. Resolves to the value now in effect. Rejects
with a busy error mid-transfer — stop ranging first.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `khz` | `number` |

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l7cx.setI2cSpeedKhz
```

***

### pinCtrl()

```ts
pinCtrl(action): Promise<void>;
```

Defined in: [sensors/vl53l7/vl53l7.ts:117](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L117)

Drive the sensor's LPn / I2C_RST pins (`PinAction`). LpnOff and SoftCycle
drop the sensor's state: run `init()` again afterwards.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `action` | `PinAction` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.pinCtrl
```

***

### configureCnh()

```ts
configureCnh(config): Promise<void>;
```

Defined in: [sensors/vl53l7/vl53l7.ts:166](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L166)

Arm the CNH histogram block for the next startRanging(). CH only — this
method does not exist on Vl53l7cx / Vl53l5cx.

CNH and the motion indicator share one DCI struct (0xBFAC): this disarms
the motion indicator (frames carry `motion` null).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `config` | [`CnhConfig`](CnhConfig.md) |

#### Returns

`Promise`\<`void`\>

***

### isAlive()

```ts
isAlive(): Promise<boolean>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:212](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L212)

True when the sensor answers with its own silicon ids (a bus error or a foreign id: false).

#### Returns

`Promise`\<`boolean`\>

#### Inherited from

```ts
Vl53l7cx.isAlive
```

***

### getResolution()

```ts
getResolution(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:258](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L258)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l7cx.getResolution
```

***

### setResolution()

```ts
setResolution(zones): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:263](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L263)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `zones` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.setResolution
```

***

### getRangingFrequencyHz()

```ts
getRangingFrequencyHz(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:272](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L272)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l7cx.getRangingFrequencyHz
```

***

### setRangingFrequencyHz()

```ts
setRangingFrequencyHz(hz): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:282](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L282)

Set the ranging frequency in Hz. Not while ranging. At least 2 Hz on the
VL53L8 (1 Hz on L5/L7); at most 60 Hz at 4×4 and 15 Hz at 8×8 (contract
11 §3) — judged by the resolution last set or read through this class
(4×4 after init). Refused host-side, nothing reaches the bus.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `hz` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.setRangingFrequencyHz
```

***

### getRangingMode()

```ts
getRangingMode(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:300](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L300)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l7cx.getRangingMode
```

***

### setRangingMode()

```ts
setRangingMode(mode): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:304](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L304)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `mode` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.setRangingMode
```

***

### getIntegrationTimeMs()

```ts
getIntegrationTimeMs(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:309](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L309)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l7cx.getIntegrationTimeMs
```

***

### setIntegrationTimeMs()

```ts
setIntegrationTimeMs(ms): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:313](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L313)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.setIntegrationTimeMs
```

***

### getSharpenerPercent()

```ts
getSharpenerPercent(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:318](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L318)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l7cx.getSharpenerPercent
```

***

### setSharpenerPercent()

```ts
setSharpenerPercent(pct): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:322](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L322)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pct` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.setSharpenerPercent
```

***

### getTargetOrder()

```ts
getTargetOrder(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:327](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L327)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l7cx.getTargetOrder
```

***

### setTargetOrder()

```ts
setTargetOrder(order): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:331](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L331)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `order` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.setTargetOrder
```

***

### getPowerMode()

```ts
getPowerMode(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:339](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L339)

POWER_MODE_SLEEP/WAKEUP/DEEP_SLEEP (uld constants).

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l7cx.getPowerMode
```

***

### setPowerMode()

```ts
setPowerMode(mode): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:347](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L347)

Enter sleep / wake / deep-sleep. Not while ranging. Waking from
DEEP_SLEEP re-downloads the firmware blob (init()).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `mode` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.setPowerMode
```

***

### getXtalkMargin()

```ts
getXtalkMargin(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:353](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L353)

Xtalk margin, whole kcps/SPAD (as ST).

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l7cx.getXtalkMargin
```

***

### setXtalkMargin()

```ts
setXtalkMargin(marginKcps): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:358](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L358)

Xtalk margin, a whole number of kcps/SPAD, 0..10000 (as ST); anything else is refused.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `marginKcps` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.setXtalkMargin
```

***

### calibrateXtalk()

```ts
calibrateXtalk(
   reflectancePercent, 
   nbSamples, 
distanceMm): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:369](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L369)

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

#### Inherited from

```ts
Vl53l7cx.calibrateXtalk
```

***

### getCaldataXtalk()

```ts
getCaldataXtalk(): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:379](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L379)

Read back the 776-byte xtalk calibration blob (save/restore).

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

#### Inherited from

```ts
Vl53l7cx.getCaldataXtalk
```

***

### setCaldataXtalk()

```ts
setCaldataXtalk(blob): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:385](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L385)

Restore a previously saved 776-byte xtalk calibration blob.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `blob` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.setCaldataXtalk
```

***

### getDetectionThresholdsEnable()

```ts
getDetectionThresholdsEnable(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:390](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L390)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l7cx.getDetectionThresholdsEnable
```

***

### setDetectionThresholdsEnable()

```ts
setDetectionThresholdsEnable(enabled): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:394](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L394)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `enabled` | `boolean` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.setDetectionThresholdsEnable
```

***

### getDetectionThresholds()

```ts
getDetectionThresholds(): Promise<DetectionThreshold[]>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:399](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L399)

#### Returns

`Promise`\<`DetectionThreshold`[]\>

#### Inherited from

```ts
Vl53l7cx.getDetectionThresholds
```

***

### setDetectionThresholds()

```ts
setDetectionThresholds(thresholds): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:408](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L408)

Program the 64 detection thresholds (interrupt-on-threshold). Each entry
carries lowThresh, highThresh, measurement, type, zoneNum, operation (see
uld THRESH_* constants).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `thresholds` | `Partial`\<`DetectionThreshold`\>[] |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.setDetectionThresholds
```

***

### setDetectionThresholdsAutoStop()

```ts
setDetectionThresholdsAutoStop(autoStop): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:413](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L413)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `autoStop` | `boolean` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.setDetectionThresholdsAutoStop
```

***

### configureMotionIndicator()

```ts
configureMotionIndicator(distanceMinMm?, distanceMaxMm?): Promise<MotionConfig>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:426](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L426)

Enable the motion indicator over [distanceMinMm, distanceMaxMm] and
surface motion output in each frame's `.motion`. Returns the underlying
uld MotionConfig for advanced tuning.

On CH firmware the motion indicator and CNH share one DCI struct
(0xBFAC): this disarms CNH (`activeCnhConfig` → null, no `cnhRaw`).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `distanceMinMm` | `number` | `400` |
| `distanceMaxMm` | `number` | `1500` |

#### Returns

`Promise`\<`MotionConfig`\>

#### Inherited from

```ts
Vl53l7cx.configureMotionIndicator
```

***

### startRanging()

```ts
startRanging(): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:438](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L438)

Configure the output list, start the sensor and the MCU stream.

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.startRanging
```

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:459](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L459)

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l7cx.stopRanging
```

***

### onFrame()

```ts
onFrame(cb): () => void;
```

Defined in: [sensors/vl53l8/vl53l8.ts:485](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L485)

Subscribe to parsed frames (read-pump context; don't block).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`frame`) => `void` |

#### Returns

() => `void`

#### Inherited from

```ts
Vl53l7cx.onFrame
```

***

### frames()

```ts
frames(maxsize?): StreamQueue<Vl53l8Frame>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:498](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L498)

Async iterator over parsed frames — bounded, drop-oldest (contract 07
§3). Subscribes eagerly at call time — call before or after
startRanging(); it ends when the device closes or the consumer breaks
out of iteration.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `maxsize` | `number` | `8` |

#### Returns

`StreamQueue`\<`Vl53l8Frame`\>

#### Inherited from

```ts
Vl53l7cx.frames
```

***

### getFrame()

```ts
getFrame(timeoutMs?): Promise<Vl53l8Frame>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:509](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L509)

Convenience: wait for the next frame.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `2000` |

#### Returns

`Promise`\<`Vl53l8Frame`\>

#### Inherited from

```ts
Vl53l7cx.getFrame
```

***

### requireNotRanging()

```ts
protected requireNotRanging(): void;
```

Defined in: [sensors/vl53l8/vl53l8.ts:537](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L537)

#### Returns

`void`

#### Inherited from

```ts
Vl53l7cx.requireNotRanging
```

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [sensors/vl53l8/vl53l8.ts:543](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L543)

Hook for sensor subclasses: route streaming reports here. Return true
when the packet was consumed. Runs after status/matcher/common-report
routing, on the read-pump context.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pkt` | `PacketEvent` |

#### Returns

`boolean`

#### Inherited from

```ts
Vl53l7cx.handleReport
```

# Function: cnhMaxBins()

```ts
function cnhMaxBins(nbAggregates, optionFlags?): number;
```

Defined in: [sensors/vl53l8/cnh.ts:269](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L269)

Largest CNH bins-per-aggregate count whose on-device buffer still fits
CNH_MAX_DATA_BYTES for the given aggregate count.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `nbAggregates` | `number` | `undefined` |
| `optionFlags` | `number` | `DEFAULT_CNH_CFG` |

## Returns

`number`

# Function: decodeCnh()

```ts
function decodeCnh(cfg, raw): CnhDecoded;
```

Defined in: [sensors/vl53l8/cnh.ts:300](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L300)

Decode a captured CNH data block (`raw` bytes, byte-swapped exactly like
the standard ranging blocks) into per-aggregate histograms.

Faithful port of vl53lmz_cnh_get_block_addresses /
_cnh_get_mem_block_addresses for the fixed cnh_cfg (ping-pong + variance
disabled).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `cfg` | [`CnhConfig`](../classes/CnhConfig.md) |
| `raw` | `Uint8Array` |

## Returns

[`CnhDecoded`](../interfaces/CnhDecoded.md)

# Interface: CnhAggregate

Defined in: [sensors/vl53l8/cnh.ts:278](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L278)

Decoded CNH aggregate.

## Properties

### hist

```ts
hist: number[];
```

Defined in: [sensors/vl53l8/cnh.ts:280](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L280)

value = raw / 2**scaler, length == cfg.featureLength.

***

### histRaw

```ts
histRaw: number[];
```

Defined in: [sensors/vl53l8/cnh.ts:281](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L281)

***

### histScaler

```ts
histScaler: number[];
```

Defined in: [sensors/vl53l8/cnh.ts:282](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L282)

***

### ambient

```ts
ambient: number;
```

Defined in: [sensors/vl53l8/cnh.ts:283](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L283)

# Interface: CnhDecoded

Defined in: [sensors/vl53l8/cnh.ts:286](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L286)

## Properties

### refResidual

```ts
refResidual: number;
```

Defined in: [sensors/vl53l8/cnh.ts:288](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L288)

11 fractional bits.

***

### aggregates

```ts
aggregates: CnhAggregate[];
```

Defined in: [sensors/vl53l8/cnh.ts:289](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L289)

# Variable: CNH\_BIN\_WIDTH\_MM

```ts
const CNH_BIN_WIDTH_MM: 37.5348 = 37.5348;
```

Defined in: [sensors/vl53l8/cnh.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L25)

# Variable: CNH\_MAX\_DATA\_BYTES

```ts
const CNH_MAX_DATA_BYTES: number;
```

Defined in: [sensors/vl53l8/cnh.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L35)

# Variable: MI\_CFG\_DEV\_IDX

```ts
const MI_CFG_DEV_IDX: 49068 = 0xbfac;
```

Defined in: [sensors/vl53l8/cnh.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L29)

VL53LMZ_MI_CFG_DEV_IDX (cnh_send_config target).

