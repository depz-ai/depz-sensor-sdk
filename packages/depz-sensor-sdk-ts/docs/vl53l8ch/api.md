# VL53L8CH (ToF) — API reference

CH-specific surface: the `Vl53l8Ch` class (the CX superset that adds
`configureCnh()`) plus the Compact-Network-Histogram (CNH) symbols.
Every base ToF method CH inherits is documented in the
[VL53L8CX reference](../vl53l8cx/api.md); cross-sensor symbols live in
the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Classes

- [CnhConfigError](classes/CnhConfigError.md)
- [CnhConfig](classes/CnhConfig.md)
- [Vl53l8Ch](classes/Vl53l8Ch.md)

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

Defined in: [src/sensors/vl53l8/cnh.ts:68](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L68)

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

Defined in: [src/sensors/vl53l8/cnh.ts:69](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L69)

***

### detectionThreshold

```ts
detectionThreshold: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:70](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L70)

***

### extraNoiseSigma

```ts
extraNoiseSigma: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L71)

***

### nullDenClipValue

```ts
nullDenClipValue: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L72)

***

### memUpdateMode

```ts
memUpdateMode: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L73)

***

### memUpdateChoice

```ts
memUpdateChoice: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:74](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L74)

***

### sumSpan

```ts
sumSpan: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:75](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L75)

***

### featureLength

```ts
featureLength: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:76](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L76)

***

### nbOfAggregates

```ts
nbOfAggregates: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L77)

***

### nbOfTemporalAccumulations

```ts
nbOfTemporalAccumulations: number = 1;
```

Defined in: [src/sensors/vl53l8/cnh.ts:78](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L78)

***

### minNbForGlobalDetection

```ts
minNbForGlobalDetection: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:79](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L79)

***

### globalIndicatorFormat1

```ts
globalIndicatorFormat1: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:80](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L80)

***

### globalIndicatorFormat2

```ts
globalIndicatorFormat2: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:81](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L81)

***

### cnhCfg

```ts
cnhCfg: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:82](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L82)

***

### cnhFlexShift

```ts
cnhFlexShift: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:83](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L83)

***

### spare3

```ts
spare3: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:84](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L84)

***

### mapId

```ts
mapId: number[];
```

Defined in: [src/sensors/vl53l8/cnh.ts:85](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L85)

***

### indicatorFormat1

```ts
indicatorFormat1: number[];
```

Defined in: [src/sensors/vl53l8/cnh.ts:86](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L86)

***

### indicatorFormat2

```ts
indicatorFormat2: number[];
```

Defined in: [src/sensors/vl53l8/cnh.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L87)

## Methods

### initConfig()

```ts
initConfig(
   startBin, 
   numBins, 
   subSample): void;
```

Defined in: [src/sensors/vl53l8/cnh.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L94)

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

Defined in: [src/sensors/vl53l8/cnh.ts:123](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L123)

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

Defined in: [src/sensors/vl53l8/cnh.ts:157](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L157)

On-device CNH buffer size in bytes for this config. Throws if the
config is blank or the size exceeds CNH_MAX_DATA_BYTES.

#### Returns

`number`

***

### minMaxDistanceMm()

```ts
minMaxDistanceMm(): [number, number];
```

Defined in: [src/sensors/vl53l8/cnh.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L172)

[min, max] target distance, in mm, fully captured by the histogram.

#### Returns

\[`number`, `number`\]

***

### binCenterMm()

```ts
binCenterMm(binIdx): number;
```

Defined in: [src/sensors/vl53l8/cnh.ts:182](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L182)

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

Defined in: [src/sensors/vl53l8/cnh.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L188)

#### Returns

`Uint8Array`

# Class: CnhConfigError

Defined in: [src/sensors/vl53l8/cnh.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L56)

## Extends

- `Error`

## Constructors

### Constructor

```ts
new CnhConfigError(message?): CnhConfigError;
```

Defined in: [src/sensors/vl53l8/cnh.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L57)

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

# Class: Vl53l8Ch

Defined in: [src/sensors/vl53l8/vl53l8.ts:503](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L503)

VL53L8CH device: the VL53L8CX superset. Inherits every CX method and adds
Compact-Network-Histogram (CNH) output. `init()` downloads the CH firmware
blob (VL53LMZ ULD 2.0.16). CNH is the reason to run CH firmware: each frame
can additionally carry a per-aggregate distance histogram.

## Extends

- `Vl53l8Cx`

## Constructors

### Constructor

```ts
new Vl53l8Ch(transport, opts?): Vl53l8Ch;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:162](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L162)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | `Vl53l8Options` |

#### Returns

`Vl53l8Ch`

#### Inherited from

```ts
Vl53l8Cx.constructor
```

## Properties

### timeoutMs

```ts
timeoutMs: number;
```

Defined in: [src/device/device.ts:178](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L178)

#### Inherited from

```ts
Vl53l8Cx.timeoutMs
```

***

### link

```ts
protected readonly link: DepzLink;
```

Defined in: [src/device/device.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L180)

#### Inherited from

```ts
Vl53l8Cx.link
```

***

### cnhConfig

```ts
protected cnhConfig: CnhConfig | null = null;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:115](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L115)

#### Inherited from

```ts
Vl53l8Cx.cnhConfig
```

***

### variantId

```ts
protected readonly variantId: Vl53l8Variant = "ch";
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:504](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L504)

Sensor-firmware blob variant this class loads.

#### Overrides

```ts
Vl53l8Cx.variantId
```

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
Vl53l8Cx.stats
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
Vl53l8Cx.timeSync
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

`VL53L8CX`

#### Inherited from

```ts
Vl53l8Cx.uld
```

***

### variant

#### Get Signature

```ts
get variant(): Vl53l8Variant;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:176](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L176)

'cx' | 'ch' (valid after init()).

##### Returns

`Vl53l8Variant`

#### Inherited from

```ts
Vl53l8Cx.variant
```

***

### ranging

#### Get Signature

```ts
get ranging(): boolean;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:395](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L395)

##### Returns

`boolean`

#### Inherited from

```ts
Vl53l8Cx.ranging
```

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
Vl53l8Cx.open
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
Vl53l8Cx.close
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
Vl53l8Cx.registerStream
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
Vl53l8Cx.onEvent
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
Vl53l8Cx.emitEvent
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
Vl53l8Cx.request
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
Vl53l8Cx.expectReport
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
Vl53l8Cx.expectText
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
Vl53l8Cx.getDeviceName
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
Vl53l8Cx.getSoftwareName
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
Vl53l8Cx.getSerialNumber
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
Vl53l8Cx.identify
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
Vl53l8Cx.readMcuTemperature
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
Vl53l8Cx.syncTime
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
Vl53l8Cx.toHostTimeUs
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
Vl53l8Cx.getReportPayloadCrc
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
Vl53l8Cx.setReportPayloadCrc
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
Vl53l8Cx.getSyncPin
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
Vl53l8Cx.setSyncPin
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
Vl53l8Cx.reset
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
Vl53l8Cx.enterBootloaderMode
```

***

### isAlive()

```ts
isAlive(): Promise<boolean>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L180)

#### Returns

`Promise`\<`boolean`\>

#### Inherited from

```ts
Vl53l8Cx.isAlive
```

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
| `variant?` | `Vl53l8Variant` |
| `opts?` | `Vl53l8InitOptions` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l8Cx.init
```

***

### getResolution()

```ts
getResolution(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:218](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L218)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8Cx.getResolution
```

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

#### Inherited from

```ts
Vl53l8Cx.setResolution
```

***

### getRangingFrequencyHz()

```ts
getRangingFrequencyHz(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:232](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L232)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8Cx.getRangingFrequencyHz
```

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

#### Inherited from

```ts
Vl53l8Cx.setRangingFrequencyHz
```

***

### getRangingMode()

```ts
getRangingMode(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:247](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L247)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8Cx.getRangingMode
```

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

#### Inherited from

```ts
Vl53l8Cx.setRangingMode
```

***

### getIntegrationTimeMs()

```ts
getIntegrationTimeMs(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:256](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L256)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8Cx.getIntegrationTimeMs
```

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

#### Inherited from

```ts
Vl53l8Cx.setIntegrationTimeMs
```

***

### getSharpenerPercent()

```ts
getSharpenerPercent(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:265](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L265)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8Cx.getSharpenerPercent
```

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

#### Inherited from

```ts
Vl53l8Cx.setSharpenerPercent
```

***

### getTargetOrder()

```ts
getTargetOrder(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:274](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L274)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8Cx.getTargetOrder
```

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

#### Inherited from

```ts
Vl53l8Cx.setTargetOrder
```

***

### getPowerMode()

```ts
getPowerMode(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:286](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L286)

POWER_MODE_SLEEP/WAKEUP/DEEP_SLEEP (uld constants).

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8Cx.getPowerMode
```

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

#### Inherited from

```ts
Vl53l8Cx.setPowerMode
```

***

### getXtalkMargin()

```ts
getXtalkMargin(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:299](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L299)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8Cx.getXtalkMargin
```

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

#### Inherited from

```ts
Vl53l8Cx.setXtalkMargin
```

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

#### Inherited from

```ts
Vl53l8Cx.calibrateXtalk
```

***

### getCaldataXtalk()

```ts
getCaldataXtalk(): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:320](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L320)

Read back the 776-byte xtalk calibration blob (save/restore).

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

#### Inherited from

```ts
Vl53l8Cx.getCaldataXtalk
```

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

#### Inherited from

```ts
Vl53l8Cx.setCaldataXtalk
```

***

### getDetectionThresholdsEnable()

```ts
getDetectionThresholdsEnable(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:331](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L331)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8Cx.getDetectionThresholdsEnable
```

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

#### Inherited from

```ts
Vl53l8Cx.setDetectionThresholdsEnable
```

***

### getDetectionThresholds()

```ts
getDetectionThresholds(): Promise<DetectionThreshold[]>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:340](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L340)

#### Returns

`Promise`\<`DetectionThreshold`[]\>

#### Inherited from

```ts
Vl53l8Cx.getDetectionThresholds
```

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
| `thresholds` | `Partial`\<`DetectionThreshold`\>[] |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l8Cx.setDetectionThresholds
```

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

#### Inherited from

```ts
Vl53l8Cx.setDetectionThresholdsAutoStop
```

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

`Promise`\<`MotionConfig`\>

#### Inherited from

```ts
Vl53l8Cx.configureMotionIndicator
```

***

### startRanging()

```ts
startRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:375](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L375)

Configure the output list, start the sensor and the MCU stream.

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l8Cx.startRanging
```

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:388](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L388)

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l8Cx.stopRanging
```

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

#### Inherited from

```ts
Vl53l8Cx.onFrame
```

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

`StreamQueue`\<`Vl53l8Frame`\>

#### Inherited from

```ts
Vl53l8Cx.frames
```

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

`Promise`\<`Vl53l8Frame`\>

#### Inherited from

```ts
Vl53l8Cx.getFrame
```

***

### requireNotRanging()

```ts
protected requireNotRanging(): void;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:452](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L452)

#### Returns

`void`

#### Inherited from

```ts
Vl53l8Cx.requireNotRanging
```

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

#### Inherited from

```ts
Vl53l8Cx.handleReport
```

***

### configureCnh()

```ts
configureCnh(config): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:510](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L510)

Arm the CNH histogram block for the next startRanging(). CH only — this
method does not exist on Vl53l8Cx.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `config` | [`CnhConfig`](CnhConfig.md) |

#### Returns

`Promise`\<`void`\>

# Function: cnhMaxBins()

```ts
function cnhMaxBins(nbAggregates, optionFlags?): number;
```

Defined in: [src/sensors/vl53l8/cnh.ts:253](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L253)

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

Defined in: [src/sensors/vl53l8/cnh.ts:284](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L284)

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

Defined in: [src/sensors/vl53l8/cnh.ts:262](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L262)

Decoded CNH aggregate.

## Properties

### hist

```ts
hist: number[];
```

Defined in: [src/sensors/vl53l8/cnh.ts:264](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L264)

value = raw / 2**scaler, length == cfg.featureLength.

***

### histRaw

```ts
histRaw: number[];
```

Defined in: [src/sensors/vl53l8/cnh.ts:265](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L265)

***

### histScaler

```ts
histScaler: number[];
```

Defined in: [src/sensors/vl53l8/cnh.ts:266](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L266)

***

### ambient

```ts
ambient: number;
```

Defined in: [src/sensors/vl53l8/cnh.ts:267](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L267)

# Interface: CnhDecoded

Defined in: [src/sensors/vl53l8/cnh.ts:270](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L270)

## Properties

### refResidual

```ts
refResidual: number;
```

Defined in: [src/sensors/vl53l8/cnh.ts:272](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L272)

11 fractional bits.

***

### aggregates

```ts
aggregates: CnhAggregate[];
```

Defined in: [src/sensors/vl53l8/cnh.ts:273](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L273)

# Variable: CNH\_BIN\_WIDTH\_MM

```ts
const CNH_BIN_WIDTH_MM: 37.5348 = 37.5348;
```

Defined in: [src/sensors/vl53l8/cnh.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L24)

# Variable: CNH\_MAX\_DATA\_BYTES

```ts
const CNH_MAX_DATA_BYTES: number;
```

Defined in: [src/sensors/vl53l8/cnh.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L34)

# Variable: MI\_CFG\_DEV\_IDX

```ts
const MI_CFG_DEV_IDX: 49068 = 0xbfac;
```

Defined in: [src/sensors/vl53l8/cnh.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L28)

VL53LMZ_MI_CFG_DEV_IDX (cnh_send_config target).

