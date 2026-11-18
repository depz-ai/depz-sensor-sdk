# VL53L7CX (ToF) — API reference

What the I2C L5/L7 board adds: the `Vl53l7cx` class, its wire codecs
(`PinAction`, `Vl53l7Info`, …) and the module-type constants.
`Vl53l7cx` subclasses `Vl53l8cx`: init, resolution, frequency, the
advanced ULD features and the frame are in the
[VL53L8CX reference](../vl53l8cx/api.md); cross-sensor symbols live in
the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Enumerations

- [Vl53l7Cmd](#enumeration-vl53l7cmd)
- [Vl53l7Rpt](#enumeration-vl53l7rpt)
- [PinAction](#enumeration-pinaction)
- [Vl53l7I2cError](#enumeration-vl53l7i2cerror)

## Classes

- [Vl53l7cx](#class-vl53l7cx)

## Interfaces

- [Vl53l7Info](#interface-vl53l7info)

## Type Aliases

- [Vl53l7Model](#type-alias-vl53l7model)
- [Vl53l7Frame](#type-alias-vl53l7frame)

## Variables

- [VL53L7\_READ\_MAX\_LEN](#variable-vl53l7_read_max_len)
- [VL53L7\_WRITE\_MAX\_LEN](#variable-vl53l7_write_max_len)
- [VL53L7\_STREAM\_CHUNK\_MAX](#variable-vl53l7_stream_chunk_max)
- [VL53L7\_INFO\_SIZE](#variable-vl53l7_info_size)
- [VL53L7\_I2C\_SPEED\_STEPS\_KHZ](#variable-vl53l7_i2c_speed_steps_khz)
- [VL53L7\_MIN\_RANGING\_FREQUENCY\_HZ](#variable-vl53l7_min_ranging_frequency_hz)
- [MODULE\_TYPE\_MZ](#variable-module_type_mz)
- [MODULE\_TYPE\_MZEVO](#variable-module_type_mzevo)
- [MODULE\_TYPE\_MZPLUS](#variable-module_type_mzplus)
- [MODULE\_TYPE\_NAMES](#variable-module_type_names)

## Functions

- [packVl53l7ReadReg](#function-packvl53l7readreg)
- [packVl53l7WriteReg](#function-packvl53l7writereg)
- [packPinCtrl](#function-packpinctrl)
- [packVl53l7SetI2cSpeed](#function-packvl53l7seti2cspeed)
- [unpackVl53l7Info](#function-unpackvl53l7info)
- [resolveVl53l7Model](#function-resolvevl53l7model)

# Class: Vl53l7cx

Defined in: [sensors/vl53l7/vl53l7.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L52)

VL53L7CX ToF device (8×8 zones, 90° field of view). `init()` downloads the
~84 KB L5/L7 sensor firmware (~1.4 s at the board's default 1 MHz I2C),
then configure and `startRanging()` exactly as on `Vl53l8cx`.

Base class of the I2C family: `Vl53l5cx` (same blob, MZ module) and
`Vl53l7ch` (CH blob, adds CNH) inherit it.

## Extends

- `Vl53l8cx`

## Constructors

### Constructor

```ts
new Vl53l7cx(transport, opts?): Vl53l7cx;
```

Defined in: [sensors/vl53l8/vl53l8.ts:200](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L200)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | `Vl53l8Options` |

#### Returns

`Vl53l7cx`

#### Inherited from

```ts
Vl53l8cx.constructor
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
Vl53l8cx.NO_MATCH
```

***

### timeoutMs

```ts
timeoutMs: number;
```

Defined in: [device/device.ts:179](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L179)

#### Inherited from

```ts
Vl53l8cx.timeoutMs
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
Vl53l8cx.events
```

***

### variantId

```ts
protected readonly variantId: Vl53l8Variant = "l7cx";
```

Defined in: [sensors/vl53l7/vl53l7.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L53)

Sensor-firmware blob variant this class loads.

#### Overrides

```ts
Vl53l8cx.variantId
```

***

### readChunk

```ts
protected readonly readChunk: number = VL53L7_READ_MAX_LEN;
```

Defined in: [sensors/vl53l7/vl53l7.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L54)

Register-bridge transfer limits (the I2C L5/L7 board reads less per call).

#### Overrides

```ts
Vl53l8cx.readChunk
```

***

### writeChunk

```ts
protected readonly writeChunk: number = VL53L7_WRITE_MAX_LEN;
```

Defined in: [sensors/vl53l7/vl53l7.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L55)

#### Overrides

```ts
Vl53l8cx.writeChunk
```

***

### streamChunk

```ts
protected readonly streamChunk: number = VL53L7_STREAM_CHUNK_MAX;
```

Defined in: [sensors/vl53l7/vl53l7.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L56)

Largest RPT_VL53_FRAME chunk the board emits (contract 04 §4 rule 4).

#### Overrides

```ts
Vl53l8cx.streamChunk
```

***

### minRangingHz

```ts
protected readonly minRangingHz: number = VL53L7_MIN_RANGING_FREQUENCY_HZ;
```

Defined in: [sensors/vl53l7/vl53l7.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L57)

Lowest ranging frequency that actually streams on this sensor.

#### Overrides

```ts
Vl53l8cx.minRangingHz
```

***

### expectedModuleType

```ts
protected readonly expectedModuleType: number = MODULE_TYPE_MZEVO;
```

Defined in: [sensors/vl53l7/vl53l7.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L59)

module_type this class expects after init() (L5 and L7 share a blob).

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

#### Inherited from

```ts
Vl53l8cx.frames
```

***

### uldDriver

```ts
protected uldDriver: VL53L8CX | null = null;
```

Defined in: [sensors/vl53l8/vl53l8.ts:141](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L141)

#### Inherited from

```ts
Vl53l8cx.uldDriver
```

***

### rangingFlag

```ts
protected rangingFlag: boolean = false;
```

Defined in: [sensors/vl53l8/vl53l8.ts:143](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L143)

#### Inherited from

```ts
Vl53l8cx.rangingFlag
```

***

### cnhConfig

```ts
protected cnhConfig: CnhConfig | null = null;
```

Defined in: [sensors/vl53l8/vl53l8.ts:144](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L144)

#### Inherited from

```ts
Vl53l8cx.cnhConfig
```

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
Vl53l8cx.stats
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
Vl53l8cx.lost
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
Vl53l8cx.timeSync
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

#### Inherited from

```ts
Vl53l8cx.activeCnhConfig
```

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

#### Inherited from

```ts
Vl53l8cx.uld
```

***

### variant

#### Get Signature

```ts
get variant(): Vl53l8Variant;
```

Defined in: [sensors/vl53l8/vl53l8.ts:219](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L219)

'cx' | 'ch' | 'l7cx' | 'l7ch' (valid after init()).

##### Returns

`Vl53l8Variant`

#### Inherited from

```ts
Vl53l8cx.variant
```

***

### ranging

#### Get Signature

```ts
get ranging(): boolean;
```

Defined in: [sensors/vl53l8/vl53l8.ts:478](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L478)

##### Returns

`boolean`

#### Inherited from

```ts
Vl53l8cx.ranging
```

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
Vl53l8cx.close
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
Vl53l8cx.onTeardown
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
Vl53l8cx.requireOpen
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
Vl53l8cx.request
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
Vl53l8cx.expectReport
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
Vl53l8cx.expectText
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
Vl53l8cx.getDeviceName
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
Vl53l8cx.getSoftwareName
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
Vl53l8cx.getSerialNumber
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
Vl53l8cx.identify
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
Vl53l8cx.readMcuTemperature
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
Vl53l8cx.syncTime
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
Vl53l8cx.toHostTimeUs
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
Vl53l8cx.getReportPayloadCrc
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
Vl53l8cx.setReportPayloadCrc
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
Vl53l8cx.getSyncPin
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
Vl53l8cx.setSyncPin
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
Vl53l8cx.reset
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
Vl53l8cx.enterBootloaderMode
```

***

### init()

```ts
init(opts?): Promise<void>;
```

Defined in: [sensors/vl53l7/vl53l7.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L67)

Initialize the sensor: firmware blob download + default config. Refuses
non-L5/L7 silicon. The sensor's `moduleType` then tells L5 (MZ) from L7
(MZEVO); a mismatch with this class is reported as a warning — ranging
still works, since the blob is shared.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `opts?` | `Vl53l8InitOptions` |

#### Returns

`Promise`\<`void`\>

#### Overrides

```ts
Vl53l8cx.init
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

`Promise`\<[`Vl53l7Info`](#interface-vl53l7info)\>

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
| `action` | [`PinAction`](#enumeration-pinaction) |

#### Returns

`Promise`\<`void`\>

***

### isAlive()

```ts
isAlive(): Promise<boolean>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:224](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L224)

True when the sensor answers with its own silicon ids (a bus error or a foreign id: false).

#### Returns

`Promise`\<`boolean`\>

#### Inherited from

```ts
Vl53l8cx.isAlive
```

***

### getResolution()

```ts
getResolution(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:264](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L264)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8cx.getResolution
```

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

#### Inherited from

```ts
Vl53l8cx.setResolution
```

***

### getRangingFrequencyHz()

```ts
getRangingFrequencyHz(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:278](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L278)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8cx.getRangingFrequencyHz
```

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

#### Inherited from

```ts
Vl53l8cx.setRangingFrequencyHz
```

***

### getRangingMode()

```ts
getRangingMode(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:306](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L306)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8cx.getRangingMode
```

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

#### Inherited from

```ts
Vl53l8cx.setRangingMode
```

***

### getIntegrationTimeMs()

```ts
getIntegrationTimeMs(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:315](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L315)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8cx.getIntegrationTimeMs
```

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

#### Inherited from

```ts
Vl53l8cx.setIntegrationTimeMs
```

***

### getSharpenerPercent()

```ts
getSharpenerPercent(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:324](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L324)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8cx.getSharpenerPercent
```

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

#### Inherited from

```ts
Vl53l8cx.setSharpenerPercent
```

***

### getTargetOrder()

```ts
getTargetOrder(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:333](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L333)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8cx.getTargetOrder
```

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

#### Inherited from

```ts
Vl53l8cx.setTargetOrder
```

***

### getPowerMode()

```ts
getPowerMode(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:345](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L345)

POWER_MODE_SLEEP/WAKEUP/DEEP_SLEEP (uld constants).

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8cx.getPowerMode
```

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

#### Inherited from

```ts
Vl53l8cx.setPowerMode
```

***

### getXtalkMargin()

```ts
getXtalkMargin(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:359](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L359)

Xtalk margin, whole kcps/SPAD (as ST).

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8cx.getXtalkMargin
```

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

#### Inherited from

```ts
Vl53l8cx.setXtalkMargin
```

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

#### Inherited from

```ts
Vl53l8cx.calibrateXtalk
```

***

### getCaldataXtalk()

```ts
getCaldataXtalk(): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:385](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L385)

Read back the 776-byte xtalk calibration blob (save/restore).

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

#### Inherited from

```ts
Vl53l8cx.getCaldataXtalk
```

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

#### Inherited from

```ts
Vl53l8cx.setCaldataXtalk
```

***

### getDetectionThresholdsEnable()

```ts
getDetectionThresholdsEnable(): Promise<number>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:396](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L396)

#### Returns

`Promise`\<`number`\>

#### Inherited from

```ts
Vl53l8cx.getDetectionThresholdsEnable
```

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

#### Inherited from

```ts
Vl53l8cx.setDetectionThresholdsEnable
```

***

### getDetectionThresholds()

```ts
getDetectionThresholds(): Promise<DetectionThreshold[]>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:405](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L405)

#### Returns

`Promise`\<`DetectionThreshold`[]\>

#### Inherited from

```ts
Vl53l8cx.getDetectionThresholds
```

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
| `thresholds` | `Partial`\<`DetectionThreshold`\>[] |

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l8cx.setDetectionThresholds
```

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

#### Inherited from

```ts
Vl53l8cx.setDetectionThresholdsAutoStop
```

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

`Promise`\<`MotionConfig`\>

#### Inherited from

```ts
Vl53l8cx.configureMotionIndicator
```

***

### startRanging()

```ts
startRanging(): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:444](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L444)

Configure the output list, start the sensor and the MCU stream.

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l8cx.startRanging
```

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [sensors/vl53l8/vl53l8.ts:465](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L465)

#### Returns

`Promise`\<`void`\>

#### Inherited from

```ts
Vl53l8cx.stopRanging
```

***

### requireNotRanging()

```ts
protected requireNotRanging(): void;
```

Defined in: [sensors/vl53l8/vl53l8.ts:484](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L484)

#### Returns

`void`

#### Inherited from

```ts
Vl53l8cx.requireNotRanging
```

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

#### Inherited from

```ts
Vl53l8cx.handleReport
```

# Enumeration: PinAction

Defined in: [protocol/vl53l7.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L28)

VL53_PIN_CTRL actions. None is a true sensor reset (the board has no power
GPIO): after LpnOff or SoftCycle the host must re-run init().

## Enumeration Members

### LpnOff

```ts
LpnOff: 0;
```

Defined in: [protocol/vl53l7.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L30)

Stop streaming, drive LPn low: sensor I2C interface off.

***

### LpnOn

```ts
LpnOn: 1;
```

Defined in: [protocol/vl53l7.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L32)

Drive LPn high: interface on (power-up default).

***

### I2cRst

```ts
I2cRst: 2;
```

Defined in: [protocol/vl53l7.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L34)

Pulse I2C_RST.

***

### SoftCycle

```ts
SoftCycle: 3;
```

Defined in: [protocol/vl53l7.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L36)

Stop streaming, LPn low 1 ms, high, I2C_RST pulse; clears I2C counters.

# Enumeration: Vl53l7Cmd

Defined in: [protocol/vl53l7.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L14)

## Enumeration Members

### PinCtrl

```ts
PinCtrl: 52;
```

Defined in: [protocol/vl53l7.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L15)

***

### GetInfo

```ts
GetInfo: 55;
```

Defined in: [protocol/vl53l7.ts:16](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L16)

***

### SetI2cSpeed

```ts
SetI2cSpeed: 56;
```

Defined in: [protocol/vl53l7.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L17)

# Enumeration: Vl53l7I2cError

Defined in: [protocol/vl53l7.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L40)

RPT_VL53_INFO.lastI2cError.

## Enumeration Members

### Ok

```ts
Ok: 0;
```

Defined in: [protocol/vl53l7.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L41)

***

### Nack

```ts
Nack: 1;
```

Defined in: [protocol/vl53l7.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L42)

***

### Timeout

```ts
Timeout: 2;
```

Defined in: [protocol/vl53l7.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L43)

***

### BusError

```ts
BusError: 3;
```

Defined in: [protocol/vl53l7.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L44)

# Enumeration: Vl53l7Rpt

Defined in: [protocol/vl53l7.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L20)

## Enumeration Members

### Vl53Info

```ts
Vl53Info: 146;
```

Defined in: [protocol/vl53l7.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L21)

# Function: packPinCtrl()

```ts
function packPinCtrl(action): Uint8Array;
```

Defined in: [protocol/vl53l7.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L94)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `action` | `number` |

## Returns

`Uint8Array`

# Function: packVl53l7ReadReg()

```ts
function packVl53l7ReadReg(addr, length): Uint8Array;
```

Defined in: [protocol/vl53l7.ts:66](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L66)

VL53_READ_REG payload `addr u16, len u16` with the L5/L7 limits (contract
11 §2): `len` 1..1536 and `addr + len ≤ 0x10000`, else RangeError.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `length` | `number` |

## Returns

`Uint8Array`

# Function: packVl53l7SetI2cSpeed()

```ts
function packVl53l7SetI2cSpeed(khz): Uint8Array;
```

Defined in: [protocol/vl53l7.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L98)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `khz` | `number` |

## Returns

`Uint8Array`

# Function: packVl53l7WriteReg()

```ts
function packVl53l7WriteReg(addr, data): Uint8Array;
```

Defined in: [protocol/vl53l7.ts:83](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L83)

VL53_WRITE_REG payload `addr u16, data[N]` with the L5/L7 limits (contract
11 §2): N 1..2048 and `addr + N ≤ 0x10000`, else RangeError.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `data` | `Uint8Array` |

## Returns

`Uint8Array`

# Function: resolveVl53l7Model()

```ts
function resolveVl53l7Model(usbModel, deviceName): Vl53l7Model;
```

Defined in: [protocol/vl53l7.ts:153](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L153)

Pick the L5/L7 class (contract 11 §1). All three boards run APP_VL53L7 and
the silicon only tells L5 from L7 after init() (module_type), never CX from
CH — so the production USB PID model decides, then the device name the
bootloader was stamped with (`… VL53L7CH USB v2.1 …`), then the CX base
(safe: its blob runs on every L5/L7 part).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `usbModel` | `string` \| `null` \| `undefined` |
| `deviceName` | `string` \| `null` \| `undefined` |

## Returns

[`Vl53l7Model`](#type-alias-vl53l7model)

# Function: unpackVl53l7Info()

```ts
function unpackVl53l7Info(payload): Vl53l7Info;
```

Defined in: [protocol/vl53l7.ts:121](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L121)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Vl53l7Info`](#interface-vl53l7info)

# Interface: Vl53l7Info

Defined in: [protocol/vl53l7.ts:109](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L109)

RPT_VL53_INFO: bridge state only (the sensor is never probed). Counters run
from power-up / DEVICE_RESET; SoftCycle clears the I2C ones. The report
carries no echoed command byte.

## Properties

### intEdges

```ts
intEdges: number;
```

Defined in: [protocol/vl53l7.ts:110](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L110)

***

### framesDropped

```ts
framesDropped: number;
```

Defined in: [protocol/vl53l7.ts:111](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L111)

***

### i2cErrors

```ts
i2cErrors: number;
```

Defined in: [protocol/vl53l7.ts:112](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L112)

***

### lastI2cError

```ts
lastI2cError: number;
```

Defined in: [protocol/vl53l7.ts:113](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L113)

***

### lpnLevel

```ts
lpnLevel: number;
```

Defined in: [protocol/vl53l7.ts:114](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L114)

***

### intLevel

```ts
intLevel: number;
```

Defined in: [protocol/vl53l7.ts:115](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L115)

***

### i2cKhz

```ts
i2cKhz: number;
```

Defined in: [protocol/vl53l7.ts:116](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L116)

***

### frameSize

```ts
frameSize: number;
```

Defined in: [protocol/vl53l7.ts:117](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L117)

***

### streaming

```ts
streaming: boolean;
```

Defined in: [protocol/vl53l7.ts:118](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L118)

# Type Alias: Vl53l7Frame

```ts
type Vl53l7Frame = Vl53l8Frame;
```

Defined in: [sensors/vl53l7/vl53l7.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L42)

Frames are the VL53L8 frame type.

# Type Alias: Vl53l7Model

```ts
type Vl53l7Model = "vl53l5cx" | "vl53l7cx" | "vl53l7ch";
```

Defined in: [protocol/vl53l7.ts:142](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L142)

A sensor class an APP_VL53L7 board opens as.

# Variable: MODULE\_TYPE\_MZ

```ts
const MODULE_TYPE_MZ: 0 = 0;
```

Defined in: [sensors/vl53l8/uld.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L64)

# Variable: MODULE\_TYPE\_MZEVO

```ts
const MODULE_TYPE_MZEVO: 1 = 1;
```

Defined in: [sensors/vl53l8/uld.ts:65](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L65)

# Variable: MODULE\_TYPE\_MZPLUS

```ts
const MODULE_TYPE_MZPLUS: 2 = 2;
```

Defined in: [sensors/vl53l8/uld.ts:66](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L66)

# Variable: MODULE\_TYPE\_NAMES

```ts
const MODULE_TYPE_NAMES: Readonly<Record<number, string>>;
```

Defined in: [sensors/vl53l8/uld.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L67)

# Variable: VL53L7\_I2C\_SPEED\_STEPS\_KHZ

```ts
const VL53L7_I2C_SPEED_STEPS_KHZ: readonly number[];
```

Defined in: [protocol/vl53l7.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L55)

Nominal SCL steps the firmware carries a timing for; others snap to nearest.

# Variable: VL53L7\_INFO\_SIZE

```ts
const VL53L7_INFO_SIZE: 20 = 20;
```

Defined in: [protocol/vl53l7.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L53)

# Variable: VL53L7\_MIN\_RANGING\_FREQUENCY\_HZ

```ts
const VL53L7_MIN_RANGING_FREQUENCY_HZ: 1 = 1;
```

Defined in: [sensors/vl53l7/vl53l7.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L39)

L5/L7 range and stream at 1 Hz (contract 11; the L8 needs ≥ 2 Hz).

# Variable: VL53L7\_READ\_MAX\_LEN

```ts
const VL53L7_READ_MAX_LEN: 1536 = 1536;
```

Defined in: [protocol/vl53l7.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L48)

VL53LMZ_READ_MAX: READ_REG len 1..1536.

# Variable: VL53L7\_STREAM\_CHUNK\_MAX

```ts
const VL53L7_STREAM_CHUNK_MAX: 1536 = 1536;
```

Defined in: [protocol/vl53l7.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L52)

Bytes of frame data per RPT_VL53_FRAME chunk.

# Variable: VL53L7\_WRITE\_MAX\_LEN

```ts
const VL53L7_WRITE_MAX_LEN: 2048 = 2048;
```

Defined in: [protocol/vl53l7.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l7.ts#L50)

VL53LMZ_XFER_MAX: WRITE_REG N 1..2048.

