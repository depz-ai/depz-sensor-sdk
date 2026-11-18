# VL53L8CX (ToF) — API reference

The base VL53L8**CX** time-of-flight surface: the `Vl53l8cx` class, the
frame type, and the ULD values its methods take and return (the ST ULD
driver itself is behind the unstable `uld` escape hatch).
The CH superset (`Vl53l8ch` + Compact-Network-Histogram) has its own
[VL53L8CH reference](../vl53l8ch/api.md); cross-sensor symbols shared by
every sensor live in the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Classes

- [Vl53l8cxError](#class-vl53l8cxerror)
- [MotionConfig](#class-motionconfig)
- [Vl53l8cx](#class-vl53l8cx)

## Interfaces

- [Vl53l8Assets](#interface-vl53l8assets)
- [DetectionThreshold](#interface-detectionthreshold)
- [MotionResult](#interface-motionresult)
- [Vl53l8Frame](#interface-vl53l8frame)
- [Vl53l8Options](#interface-vl53l8options)
- [Vl53l8InitOptions](#interface-vl53l8initoptions)

## Type Aliases

- [Vl53l8Variant](#type-alias-vl53l8variant)

## Variables

- [RESOLUTION\_4X4](#variable-resolution_4x4)
- [RESOLUTION\_8X8](#variable-resolution_8x8)
- [RANGING\_MODE\_CONTINUOUS](#variable-ranging_mode_continuous)
- [RANGING\_MODE\_AUTONOMOUS](#variable-ranging_mode_autonomous)
- [TARGET\_ORDER\_CLOSEST](#variable-target_order_closest)
- [TARGET\_ORDER\_STRONGEST](#variable-target_order_strongest)
- [NB\_THRESHOLDS](#variable-nb_thresholds)
- [POWER\_MODE\_SLEEP](#variable-power_mode_sleep)
- [POWER\_MODE\_WAKEUP](#variable-power_mode_wakeup)
- [POWER\_MODE\_DEEP\_SLEEP](#variable-power_mode_deep_sleep)
- [DIST\_MM](#variable-dist_mm)
- [SIGNAL\_PER\_SPAD\_KCPS](#variable-signal_per_spad_kcps)
- [RANGE\_SIGMA\_MM](#variable-range_sigma_mm)
- [AMBIENT\_PER\_SPAD\_KCPS](#variable-ambient_per_spad_kcps)
- [NB\_TARGET\_DETECTED](#variable-nb_target_detected)
- [TAR\_STATUS](#variable-tar_status)
- [NB\_SPADS\_ENABLED](#variable-nb_spads_enabled)
- [MOTION\_INDICATOR](#variable-motion_indicator)
- [THRESH\_IN\_WINDOW](#variable-thresh_in_window)
- [THRESH\_OUT\_OF\_WINDOW](#variable-thresh_out_of_window)
- [THRESH\_LESS\_THAN\_EQUAL\_MIN](#variable-thresh_less_than_equal_min)
- [THRESH\_GREATER\_THAN\_MAX](#variable-thresh_greater_than_max)
- [THRESH\_EQUAL\_MIN](#variable-thresh_equal_min)
- [THRESH\_NOT\_EQUAL\_MIN](#variable-thresh_not_equal_min)
- [THRESH\_OP\_NONE](#variable-thresh_op_none)
- [THRESH\_OP\_OR](#variable-thresh_op_or)
- [THRESH\_OP\_AND](#variable-thresh_op_and)
- [XTALK\_MARGIN\_MAX\_KCPS](#variable-xtalk_margin_max_kcps)
- [MIN\_RANGING\_FREQUENCY\_HZ](#variable-min_ranging_frequency_hz)
- [MAX\_RANGING\_FREQUENCY\_HZ](#variable-max_ranging_frequency_hz)

## Functions

- [loadAssets](#function-loadassets)
- [zoneGrid](#function-zonegrid)

# Class: MotionConfig

Defined in: [sensors/vl53l8/uld.ts:372](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L372)

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

Defined in: [sensors/vl53l8/uld.ts:373](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L373)

***

### detectionThreshold

```ts
detectionThreshold: number = 0;
```

Defined in: [sensors/vl53l8/uld.ts:374](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L374)

***

### extraNoiseSigma

```ts
extraNoiseSigma: number = 0;
```

Defined in: [sensors/vl53l8/uld.ts:375](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L375)

***

### nullDenClipValue

```ts
nullDenClipValue: number = 0;
```

Defined in: [sensors/vl53l8/uld.ts:376](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L376)

***

### memUpdateMode

```ts
memUpdateMode: number = 0;
```

Defined in: [sensors/vl53l8/uld.ts:377](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L377)

***

### memUpdateChoice

```ts
memUpdateChoice: number = 0;
```

Defined in: [sensors/vl53l8/uld.ts:378](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L378)

***

### sumSpan

```ts
sumSpan: number = 0;
```

Defined in: [sensors/vl53l8/uld.ts:379](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L379)

***

### featureLength

```ts
featureLength: number = 0;
```

Defined in: [sensors/vl53l8/uld.ts:380](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L380)

***

### nbOfAggregates

```ts
nbOfAggregates: number = 0;
```

Defined in: [sensors/vl53l8/uld.ts:381](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L381)

***

### nbOfTemporalAccumulations

```ts
nbOfTemporalAccumulations: number = 0;
```

Defined in: [sensors/vl53l8/uld.ts:382](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L382)

***

### minNbForGlobalDetection

```ts
minNbForGlobalDetection: number = 0;
```

Defined in: [sensors/vl53l8/uld.ts:383](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L383)

***

### globalIndicatorFormat1

```ts
globalIndicatorFormat1: number = 0;
```

Defined in: [sensors/vl53l8/uld.ts:384](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L384)

***

### globalIndicatorFormat2

```ts
globalIndicatorFormat2: number = 0;
```

Defined in: [sensors/vl53l8/uld.ts:385](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L385)

***

### spare1

```ts
spare1: number = 0;
```

Defined in: [sensors/vl53l8/uld.ts:386](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L386)

***

### spare2

```ts
spare2: number = 0;
```

Defined in: [sensors/vl53l8/uld.ts:387](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L387)

***

### spare3

```ts
spare3: number = 0;
```

Defined in: [sensors/vl53l8/uld.ts:388](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L388)

***

### mapId

```ts
mapId: number[];
```

Defined in: [sensors/vl53l8/uld.ts:389](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L389)

***

### indicatorFormat1

```ts
indicatorFormat1: number[];
```

Defined in: [sensors/vl53l8/uld.ts:390](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L390)

***

### indicatorFormat2

```ts
indicatorFormat2: number[];
```

Defined in: [sensors/vl53l8/uld.ts:391](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L391)

## Methods

### pack()

```ts
pack(): Uint8Array;
```

Defined in: [sensors/vl53l8/uld.ts:393](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L393)

#### Returns

`Uint8Array`

# Class: Vl53l8cx

Defined in: [sensors/vl53l8/vl53l8.ts:121](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L121)

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

Defined in: [sensors/vl53l8/vl53l8.ts:200](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L200)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | [`Vl53l8Options`](#interface-vl53l8options) |

#### Returns

`Vl53l8cx`

#### Overrides

```ts
DepzDevice.constructor
```

## Properties

### NO\_MATCH

```ts
readonly static NO_MATCH: typeof NO_MATCH;
```

Defined in: [device/device.ts:177](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L177)

What a `Matcher` returns to decline a packet (the Python reference's
`DepzDevice.NO_MATCH`).

#### Inherited from

```ts
DepzDevice.NO_MATCH
```

***

### timeoutMs

```ts
timeoutMs: number;
```

Defined in: [device/device.ts:179](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L179)

#### Inherited from

```ts
DepzDevice.timeoutMs
```

***

### events

```ts
readonly events: Stream<DeviceEvent>;
```

Defined in: [device/device.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L187)

The unsolicited / diagnostic events of the device (contract 07 §2):
`events.on(cb)` subscribes (read-pump context; do not block),
`events.subscribe()` iterates. There is no `events.next()` — no single
report to wait for (§3).

#### Inherited from

```ts
DepzDevice.events
```

***

### variantId

```ts
protected readonly variantId: Vl53l8Variant = "cx";
```

Defined in: [sensors/vl53l8/vl53l8.ts:123](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L123)

Sensor-firmware blob variant this class loads.

***

### readChunk

```ts
protected readonly readChunk: number = CHUNK_SIZE;
```

Defined in: [sensors/vl53l8/vl53l8.ts:125](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L125)

Register-bridge transfer limits (the I2C L5/L7 board reads less per call).

***

### writeChunk

```ts
protected readonly writeChunk: number = CHUNK_SIZE;
```

Defined in: [sensors/vl53l8/vl53l8.ts:126](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L126)

***

### streamChunk

```ts
protected readonly streamChunk: number = STREAM_CHUNK_MAX;
```

Defined in: [sensors/vl53l8/vl53l8.ts:128](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L128)

Largest RPT_VL53_FRAME chunk the board emits (contract 04 §4 rule 4).

***

### minRangingHz

```ts
protected readonly minRangingHz: number = MIN_RANGING_FREQUENCY_HZ;
```

Defined in: [sensors/vl53l8/vl53l8.ts:130](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L130)

Lowest ranging frequency that actually streams on this sensor.

***

### frames

```ts
readonly frames: Stream<Vl53l8Frame>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:139](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L139)

The parsed frames (contract 07 §3): `on(cb)` subscribes, `subscribe(maxsize)`
iterates (call before or after `startRanging()`), `next(timeoutMs)` waits
for one; `droppedCounts` and `parseErrors` (frames the ULD could not
parse — corrupt frame, bad size — or chunks shorter than their header;
distinct from reassembly gaps) are its counters.

***

### uldDriver

```ts
protected uldDriver: VL53L8CX | null = null;
```

Defined in: [sensors/vl53l8/vl53l8.ts:141](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L141)

***

### rangingFlag

```ts
protected rangingFlag: boolean = false;
```

Defined in: [sensors/vl53l8/vl53l8.ts:143](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L143)

***

### cnhConfig

```ts
protected cnhConfig: CnhConfig | null = null;
```

Defined in: [sensors/vl53l8/vl53l8.ts:144](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L144)

## Accessors

### stats

#### Get Signature

```ts
get stats(): LinkStats;
```

Defined in: [device/device.ts:255](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L255)

##### Returns

`LinkStats`

#### Inherited from

```ts
DepzDevice.stats
```

***

### lost

#### Get Signature

```ts
get lost(): DeviceLostError | undefined;
```

Defined in: [device/device.ts:265](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L265)

The loss that ended this device (its `cause` is what the transport
raised), or undefined: still open, or ended by the program's own
`close()`. A pull iterator ends quietly either way and this tells which
(contract 07 §3).

##### Returns

`DeviceLostError` \| `undefined`

#### Inherited from

```ts
DepzDevice.lost
```

***

### timeSync

#### Get Signature

```ts
get timeSync(): TimeSync | null;
```

Defined in: [device/device.ts:616](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L616)

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

Defined in: [sensors/vl53l8/vl53l8.ts:148](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L148)

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

Defined in: [sensors/vl53l8/vl53l8.ts:213](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L213)

The underlying ULD driver (escape hatch for advanced DCI access).
Unstable — not covered by the 1.0 freeze: the driver class and its
methods follow the ST ULD release the SDK ports, and the class is not
exported from the package root.

##### Returns

`VL53L8CX`

***

### variant

#### Get Signature

```ts
get variant(): Vl53l8Variant;
```

Defined in: [sensors/vl53l8/vl53l8.ts:219](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L219)

'cx' | 'ch' | 'l7cx' | 'l7ch' (valid after init()).

##### Returns

[`Vl53l8Variant`](#type-alias-vl53l8variant)

***

### ranging

#### Get Signature

```ts
get ranging(): boolean;
```

Defined in: [sensors/vl53l8/vl53l8.ts:478](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L478)

##### Returns

`boolean`

## Methods

### close()

```ts
close(): Promise<void>;
```

Defined in: [device/device.ts:228](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L228)

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

Defined in: [device/device.ts:298](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L298)

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

### requireOpen()

```ts
protected requireOpen(): void;
```

Defined in: [device/device.ts:477](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L477)

Throws what every request of a handed-over, lost or closed device throws (contract 07 §3): lost
(whatever came after it), or closed by this side. A sensor class calls it before a guard of its
own state — "stop the stream first" — so that a board unplugged while it streamed says it is
lost, not that it still streams.

#### Returns

`void`

#### Inherited from

```ts
DepzDevice.requireOpen
```

***

### request()

```ts
request<T>(
   cmd, 
   payload?, 
opts?): Promise<T>;
```

Defined in: [device/device.ts:487](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L487)

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

Defined in: [device/device.ts:551](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L551)

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

Defined in: [device/device.ts:556](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L556)

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

Defined in: [device/device.ts:567](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L567)

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

Defined in: [device/device.ts:573](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L573)

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

Defined in: [device/device.ts:579](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L579)

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

Defined in: [device/device.ts:586](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L586)

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

Defined in: [device/device.ts:591](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L591)

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

Defined in: [device/device.ts:599](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L599)

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

Defined in: [device/device.ts:621](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L621)

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

Defined in: [device/device.ts:626](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L626)

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

Defined in: [device/device.ts:638](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L638)

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

Defined in: [device/device.ts:642](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L642)

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

Defined in: [device/device.ts:648](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L648)

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

Defined in: [device/device.ts:653](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L653)

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

Defined in: [device/device.ts:662](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L662)

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

Defined in: [sensors/vl53l8/vl53l8.ts:224](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L224)

True when the sensor answers with its own silicon ids (a bus error or a foreign id: false).

#### Returns

`Promise`\<`boolean`\>

***

### init()

```ts
init(opts?): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:241](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L241)

Initialize the sensor: firmware blob download + default config.

The blob is the class's (`Vl53l8cx` → 'cx', `Vl53l8ch` → 'ch', the L5/L7
classes theirs). Assets are loaded lazily (dynamic import) so the blobs
stay out of bundles that never init the ToF.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `opts?` | [`Vl53l8InitOptions`](#interface-vl53l8initoptions) |

#### Returns

`Promise`\<`void`\>

***

### getResolution()

```ts
getResolution(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:264](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L264)

#### Returns

`Promise`\<`number`\>

***

### setResolution()

```ts
setResolution(zones): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:269](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L269)

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

Defined in: [sensors/vl53l8/vl53l8.ts:278](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L278)

#### Returns

`Promise`\<`number`\>

***

### setRangingFrequencyHz()

```ts
setRangingFrequencyHz(hz): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:288](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L288)

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

***

### getRangingMode()

```ts
getRangingMode(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:306](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L306)

#### Returns

`Promise`\<`number`\>

***

### setRangingMode()

```ts
setRangingMode(mode): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:310](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L310)

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

Defined in: [sensors/vl53l8/vl53l8.ts:315](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L315)

#### Returns

`Promise`\<`number`\>

***

### setIntegrationTimeMs()

```ts
setIntegrationTimeMs(ms): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:319](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L319)

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

Defined in: [sensors/vl53l8/vl53l8.ts:324](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L324)

#### Returns

`Promise`\<`number`\>

***

### setSharpenerPercent()

```ts
setSharpenerPercent(pct): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:328](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L328)

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

Defined in: [sensors/vl53l8/vl53l8.ts:333](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L333)

#### Returns

`Promise`\<`number`\>

***

### setTargetOrder()

```ts
setTargetOrder(order): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:337](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L337)

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

Defined in: [sensors/vl53l8/vl53l8.ts:345](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L345)

POWER_MODE_SLEEP/WAKEUP/DEEP_SLEEP (uld constants).

#### Returns

`Promise`\<`number`\>

***

### setPowerMode()

```ts
setPowerMode(mode): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:353](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L353)

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

Defined in: [sensors/vl53l8/vl53l8.ts:359](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L359)

Xtalk margin, whole kcps/SPAD (as ST).

#### Returns

`Promise`\<`number`\>

***

### setXtalkMargin()

```ts
setXtalkMargin(marginKcps): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:364](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L364)

Xtalk margin, a whole number of kcps/SPAD, 0..10000 (as ST); anything else is refused.

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

Defined in: [sensors/vl53l8/vl53l8.ts:375](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L375)

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

Defined in: [sensors/vl53l8/vl53l8.ts:385](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L385)

Read back the 776-byte xtalk calibration blob (save/restore).

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### setCaldataXtalk()

```ts
setCaldataXtalk(blob): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:391](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L391)

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

Defined in: [sensors/vl53l8/vl53l8.ts:396](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L396)

#### Returns

`Promise`\<`number`\>

***

### setDetectionThresholdsEnable()

```ts
setDetectionThresholdsEnable(enabled): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:400](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L400)

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

Defined in: [sensors/vl53l8/vl53l8.ts:405](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L405)

#### Returns

`Promise`\<[`DetectionThreshold`](#interface-detectionthreshold)[]\>

***

### setDetectionThresholds()

```ts
setDetectionThresholds(thresholds): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L414)

Program the 64 detection thresholds (interrupt-on-threshold). Each entry
carries lowThresh, highThresh, measurement, type, zoneNum, operation (see
uld THRESH_* constants).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `thresholds` | `Partial`\<[`DetectionThreshold`](#interface-detectionthreshold)\>[] |

#### Returns

`Promise`\<`void`\>

***

### setDetectionThresholdsAutoStop()

```ts
setDetectionThresholdsAutoStop(autoStop): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:419](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L419)

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

Defined in: [sensors/vl53l8/vl53l8.ts:432](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L432)

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

`Promise`\<[`MotionConfig`](#class-motionconfig)\>

***

### startRanging()

```ts
startRanging(): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:444](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L444)

Configure the output list, start the sensor and the MCU stream.

#### Returns

`Promise`\<`void`\>

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:465](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L465)

#### Returns

`Promise`\<`void`\>

***

### requireNotRanging()

```ts
protected requireNotRanging(): void;
```

Defined in: [sensors/vl53l8/vl53l8.ts:484](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L484)

#### Returns

`void`

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [sensors/vl53l8/vl53l8.ts:491](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L491)

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

Defined in: [sensors/vl53l8/uld.ts:252](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L252)

## Extends

- `Error`

## Constructors

### Constructor

```ts
new Vl53l8cxError(code, where?): Vl53l8cxError;
```

Defined in: [sensors/vl53l8/uld.ts:256](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L256)

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

Defined in: [sensors/vl53l8/uld.ts:253](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L253)

***

### where

```ts
readonly where: string;
```

Defined in: [sensors/vl53l8/uld.ts:254](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L254)

***

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

# Function: loadAssets()

```ts
function loadAssets(variant): Promise<Vl53l8Assets>;
```

Defined in: [sensors/vl53l8/assets/index.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L24)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `variant` | [`Vl53l8Variant`](#type-alias-vl53l8variant) |

## Returns

`Promise`\<[`Vl53l8Assets`](#interface-vl53l8assets)\>

# Function: zoneGrid()

```ts
function zoneGrid(arr, resolution): number[][];
```

Defined in: [sensors/vl53l8/vl53l8.ts:88](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L88)

Zone array reshaped to (4,4) or (8,8) — mirror of Vl53l8Frame.grid().

## Parameters

| Parameter | Type |
| ------ | ------ |
| `arr` | `ArrayLike`\<`number`\> |
| `resolution` | `number` |

## Returns

`number`[][]

# Interface: DetectionThreshold

Defined in: [sensors/vl53l8/uld.ts:324](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L324)

One detection-threshold entry (real units; mirror of the Python dict).

## Properties

### lowThresh

```ts
lowThresh: number;
```

Defined in: [sensors/vl53l8/uld.ts:325](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L325)

***

### highThresh

```ts
highThresh: number;
```

Defined in: [sensors/vl53l8/uld.ts:326](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L326)

***

### measurement

```ts
measurement: number;
```

Defined in: [sensors/vl53l8/uld.ts:327](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L327)

***

### type

```ts
type: number;
```

Defined in: [sensors/vl53l8/uld.ts:328](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L328)

***

### zoneNum

```ts
zoneNum: number;
```

Defined in: [sensors/vl53l8/uld.ts:329](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L329)

***

### operation

```ts
operation: number;
```

Defined in: [sensors/vl53l8/uld.ts:330](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L330)

# Interface: MotionResult

Defined in: [sensors/vl53l8/uld.ts:334](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L334)

Motion-indicator results block (mirror of the Python `motion_indicator` dict).

## Properties

### globalIndicator1

```ts
globalIndicator1: number;
```

Defined in: [sensors/vl53l8/uld.ts:335](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L335)

***

### globalIndicator2

```ts
globalIndicator2: number;
```

Defined in: [sensors/vl53l8/uld.ts:336](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L336)

***

### status

```ts
status: number;
```

Defined in: [sensors/vl53l8/uld.ts:337](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L337)

***

### nbOfDetectedAggregates

```ts
nbOfDetectedAggregates: number;
```

Defined in: [sensors/vl53l8/uld.ts:338](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L338)

***

### nbOfAggregates

```ts
nbOfAggregates: number;
```

Defined in: [sensors/vl53l8/uld.ts:339](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L339)

***

### motion

```ts
motion: number[];
```

Defined in: [sensors/vl53l8/uld.ts:340](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L340)

# Interface: Vl53l8Assets

Defined in: [sensors/vl53l8/assets/index.ts:13](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L13)

## Properties

### firmware

```ts
firmware: Uint8Array;
```

Defined in: [sensors/vl53l8/assets/index.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L15)

firmware.bin — 86016 bytes (0x15000).

***

### defaultCfg

```ts
defaultCfg: Uint8Array;
```

Defined in: [sensors/vl53l8/assets/index.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L17)

default_configuration.bin — 972 bytes.

***

### defaultXtalk

```ts
defaultXtalk: Uint8Array;
```

Defined in: [sensors/vl53l8/assets/index.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L19)

default_xtalk.bin — 776 bytes.

***

### getNvmCmd

```ts
getNvmCmd: Uint8Array;
```

Defined in: [sensors/vl53l8/assets/index.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L21)

get_nvm_cmd.bin — 40 bytes.

# Interface: Vl53l8Frame

Defined in: [sensors/vl53l8/vl53l8.ts:62](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L62)

One parsed ranging frame. Arrays are sized to the active resolution
(16 or 64 zones); zone index runs row-major (see datasheet zone maps).

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/vl53l8/vl53l8.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L63)

***

### resolution

```ts
resolution: number;
```

Defined in: [sensors/vl53l8/vl53l8.ts:65](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L65)

16 | 64

***

### distanceMm

```ts
distanceMm: Int32Array;
```

Defined in: [sensors/vl53l8/vl53l8.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L67)

Integer mm, as ST: a negative distance is clamped to 0.

***

### targetStatus

```ts
targetStatus: Uint8Array;
```

Defined in: [sensors/vl53l8/vl53l8.ts:69](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L69)

5/9 = valid, 255 = no target.

***

### nbTargetDetected

```ts
nbTargetDetected: Uint8Array;
```

Defined in: [sensors/vl53l8/vl53l8.ts:70](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L70)

***

### signalPerSpad

```ts
signalPerSpad: Float64Array;
```

Defined in: [sensors/vl53l8/vl53l8.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L72)

kcps/SPAD (raw / 2048, fraction kept).

***

### ambientPerSpad

```ts
ambientPerSpad: Float64Array;
```

Defined in: [sensors/vl53l8/vl53l8.ts:74](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L74)

kcps/SPAD (raw / 2048, fraction kept).

***

### nbSpadsEnabled

```ts
nbSpadsEnabled: Int32Array;
```

Defined in: [sensors/vl53l8/vl53l8.ts:75](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L75)

***

### rangeSigmaMm

```ts
rangeSigmaMm: Float64Array;
```

Defined in: [sensors/vl53l8/vl53l8.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L77)

Measurement noise, mm (raw / 128).

***

### reflectance

```ts
reflectance: Float64Array;
```

Defined in: [sensors/vl53l8/vl53l8.ts:79](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L79)

Estimated target reflectance, % (raw / 2, fraction kept).

***

### siliconTempDegc

```ts
siliconTempDegc: number;
```

Defined in: [sensors/vl53l8/vl53l8.ts:80](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L80)

***

### cnhRaw

```ts
cnhRaw: Uint8Array<ArrayBufferLike> | null;
```

Defined in: [sensors/vl53l8/vl53l8.ts:82](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L82)

CH variant: raw CNH block (decode via cnh).

***

### motion

```ts
motion: MotionResult | null;
```

Defined in: [sensors/vl53l8/vl53l8.ts:84](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L84)

Motion-indicator output when `configureMotionIndicator()` is active.

# Interface: Vl53l8InitOptions

Defined in: [sensors/vl53l8/vl53l8.ts:104](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L104)

## Properties

### progress?

```ts
optional progress?: (text) => void;
```

Defined in: [sensors/vl53l8/vl53l8.ts:106](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L106)

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

Defined in: [sensors/vl53l8/vl53l8.ts:108](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L108)

Tracks the big blob writes: (done, total) bytes.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `done` | `number` |
| `total` | `number` |

#### Returns

`void`

# Interface: Vl53l8Options

Defined in: [sensors/vl53l8/vl53l8.ts:99](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L99)

## Extends

- `DeviceOptions`

## Properties

### timeoutMs?

```ts
optional timeoutMs?: number;
```

Defined in: [device/device.ts:136](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L136)

#### Inherited from

```ts
DeviceOptions.timeoutMs
```

***

### txCrcType?

```ts
optional txCrcType?: CrcType;
```

Defined in: [device/device.ts:142](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L142)

CRC type of the frames the device sends (default `CrcType.None`); on an
adopted connection (`attachSensorClass`, `openOnTransport`) it replaces
the link's, left as it was when not given.

#### Inherited from

```ts
DeviceOptions.txCrcType
```

***

### sleepImpl?

```ts
optional sleepImpl?: (ms) => Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:101](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L101)

ULD sleep implementation (tests inject an instant one).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Type Alias: Vl53l8Variant

```ts
type Vl53l8Variant = "cx" | "ch" | "l7cx" | "l7ch";
```

Defined in: [sensors/vl53l8/assets/index.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L11)

Blob set: 'cx' / 'ch' = VL53L8CX / VL53L8CH; 'l7cx' = VL53L5CX/VL53L7CX
(ULD 2.0.1); 'l7ch' = VL53L7CH (VL53LMZ 2.0.16, L7 configuration).

# Variable: AMBIENT\_PER\_SPAD\_KCPS

```ts
const AMBIENT_PER_SPAD_KCPS: 8 = 8;
```

Defined in: [sensors/vl53l8/uld.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L164)

# Variable: DIST\_MM

```ts
const DIST_MM: 1 = 1;
```

Defined in: [sensors/vl53l8/uld.ts:161](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L161)

# Variable: MAX\_RANGING\_FREQUENCY\_HZ

```ts
const MAX_RANGING_FREQUENCY_HZ: Readonly<Record<number, number>>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L53)

Highest ranging frequency per resolution (contract 11 §3; the same silicon
limit on VL53L8): 60 Hz at 4×4, 15 Hz at 8×8.

# Variable: MIN\_RANGING\_FREQUENCY\_HZ

```ts
const MIN_RANGING_FREQUENCY_HZ: 2 = 2;
```

Defined in: [sensors/vl53l8/vl53l8.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L48)

Below this the sensor never streams (contract 04).

# Variable: MOTION\_INDICATOR

```ts
const MOTION_INDICATOR: 19 = 19;
```

Defined in: [sensors/vl53l8/uld.ts:168](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L168)

# Variable: NB\_SPADS\_ENABLED

```ts
const NB_SPADS_ENABLED: 13 = 13;
```

Defined in: [sensors/vl53l8/uld.ts:167](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L167)

# Variable: NB\_TARGET\_DETECTED

```ts
const NB_TARGET_DETECTED: 9 = 9;
```

Defined in: [sensors/vl53l8/uld.ts:165](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L165)

# Variable: NB\_THRESHOLDS

```ts
const NB_THRESHOLDS: 64 = 64;
```

Defined in: [sensors/vl53l8/uld.ts:151](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L151)

# Variable: POWER\_MODE\_DEEP\_SLEEP

```ts
const POWER_MODE_DEEP_SLEEP: 2 = 2;
```

Defined in: [sensors/vl53l8/uld.ts:157](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L157)

# Variable: POWER\_MODE\_SLEEP

```ts
const POWER_MODE_SLEEP: 0 = 0;
```

Defined in: [sensors/vl53l8/uld.ts:155](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L155)

# Variable: POWER\_MODE\_WAKEUP

```ts
const POWER_MODE_WAKEUP: 1 = 1;
```

Defined in: [sensors/vl53l8/uld.ts:156](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L156)

# Variable: RANGE\_SIGMA\_MM

```ts
const RANGE_SIGMA_MM: 4 = 4;
```

Defined in: [sensors/vl53l8/uld.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L163)

# Variable: RANGING\_MODE\_AUTONOMOUS

```ts
const RANGING_MODE_AUTONOMOUS: 3 = 3;
```

Defined in: [sensors/vl53l8/uld.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L77)

# Variable: RANGING\_MODE\_CONTINUOUS

```ts
const RANGING_MODE_CONTINUOUS: 1 = 1;
```

Defined in: [sensors/vl53l8/uld.ts:76](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L76)

# Variable: RESOLUTION\_4X4

```ts
const RESOLUTION_4X4: 16 = 16;
```

Defined in: [sensors/vl53l8/uld.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L73)

# Variable: RESOLUTION\_8X8

```ts
const RESOLUTION_8X8: 64 = 64;
```

Defined in: [sensors/vl53l8/uld.ts:74](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L74)

# Variable: SIGNAL\_PER\_SPAD\_KCPS

```ts
const SIGNAL_PER_SPAD_KCPS: 2 = 2;
```

Defined in: [sensors/vl53l8/uld.ts:162](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L162)

# Variable: TARGET\_ORDER\_CLOSEST

```ts
const TARGET_ORDER_CLOSEST: 1 = 1;
```

Defined in: [sensors/vl53l8/uld.ts:79](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L79)

# Variable: TARGET\_ORDER\_STRONGEST

```ts
const TARGET_ORDER_STRONGEST: 2 = 2;
```

Defined in: [sensors/vl53l8/uld.ts:80](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L80)

# Variable: TAR\_STATUS

```ts
const TAR_STATUS: 12 = 12;
```

Defined in: [sensors/vl53l8/uld.ts:166](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L166)

# Variable: THRESH\_EQUAL\_MIN

```ts
const THRESH_EQUAL_MIN: 4 = 4;
```

Defined in: [sensors/vl53l8/uld.ts:183](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L183)

# Variable: THRESH\_GREATER\_THAN\_MAX

```ts
const THRESH_GREATER_THAN_MAX: 3 = 3;
```

Defined in: [sensors/vl53l8/uld.ts:182](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L182)

# Variable: THRESH\_IN\_WINDOW

```ts
const THRESH_IN_WINDOW: 0 = 0;
```

Defined in: [sensors/vl53l8/uld.ts:179](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L179)

# Variable: THRESH\_LESS\_THAN\_EQUAL\_MIN

```ts
const THRESH_LESS_THAN_EQUAL_MIN: 2 = 2;
```

Defined in: [sensors/vl53l8/uld.ts:181](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L181)

# Variable: THRESH\_NOT\_EQUAL\_MIN

```ts
const THRESH_NOT_EQUAL_MIN: 5 = 5;
```

Defined in: [sensors/vl53l8/uld.ts:184](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L184)

# Variable: THRESH\_OP\_AND

```ts
const THRESH_OP_AND: 2 = 2;
```

Defined in: [sensors/vl53l8/uld.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L188)

# Variable: THRESH\_OP\_NONE

```ts
const THRESH_OP_NONE: 0 = 0;
```

Defined in: [sensors/vl53l8/uld.ts:186](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L186)

# Variable: THRESH\_OP\_OR

```ts
const THRESH_OP_OR: 0 = 0;
```

Defined in: [sensors/vl53l8/uld.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L187)

# Variable: THRESH\_OUT\_OF\_WINDOW

```ts
const THRESH_OUT_OF_WINDOW: 1 = 1;
```

Defined in: [sensors/vl53l8/uld.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L180)

# Variable: XTALK\_MARGIN\_MAX\_KCPS

```ts
const XTALK_MARGIN_MAX_KCPS: 10000 = 10000;
```

Defined in: [sensors/vl53l8/uld.ts:463](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L463)

Largest xtalk margin setXtalkMargin accepts, kcps/SPAD (as ST).

