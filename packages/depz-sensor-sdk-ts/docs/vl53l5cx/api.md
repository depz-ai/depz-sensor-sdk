# VL53L5CX (ToF) — API reference

The `Vl53l5cx` class. It subclasses `Vl53l7cx` and inherits its whole
surface — the board commands and codecs are in the
[VL53L7CX reference](../vl53l7cx/api.md), everything shared with the
VL53L8 in the [VL53L8CX reference](../vl53l8cx/api.md); cross-sensor
symbols live in the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Classes

- [Vl53l5cx](classes/Vl53l5cx.md)

# Class: Vl53l5cx

Defined in: [sensors/vl53l7/vl53l7.ts:146](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L146)

VL53L5CX ToF device (8×8 zones, 63° field of view). Same board, blob and
API as `Vl53l7cx`; the sensor reports module type MZ.

## Extends

- `Vl53l7cx`

## Constructors

### Constructor

```ts
new Vl53l5cx(transport, opts?): Vl53l5cx;
```

Defined in: [sensors/vl53l8/vl53l8.ts:193](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L193)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | `Vl53l8Options` |

#### Returns

`Vl53l5cx`

#### Inherited from

```ts
Vl53l7cx.constructor
```

## Properties

### timeoutMs

```ts
timeoutMs: number;
```

Defined in: [device/device.ts:214](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L214)

#### Inherited from

```ts
Vl53l7cx.timeoutMs
```

***

### link

```ts
protected readonly link: DepzLink;
```

Defined in: [device/device.ts:216](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L216)

#### Inherited from

```ts
Vl53l7cx.link
```

***

### variantId

```ts
protected readonly variantId: Vl53l8Variant = "l7cx";
```

Defined in: [sensors/vl53l7/vl53l7.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L53)

Sensor-firmware blob variant this class loads.

#### Inherited from

```ts
Vl53l7cx.variantId
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
protected readonly expectedModuleType: number = MODULE_TYPE_MZ;
```

Defined in: [sensors/vl53l7/vl53l7.ts:147](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l7/vl53l7.ts#L147)

module_type this class expects after init() (L5 and L7 share a blob).

#### Overrides

```ts
Vl53l7cx.expectedModuleType
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

Defined in: [device/device.ts:278](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L278)

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

Defined in: [device/device.ts:591](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L591)

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

`CnhConfig` \| `null`

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

Defined in: [sensors/vl53l8/vl53l8.ts:464](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L464)

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

Defined in: [sensors/vl53l8/vl53l8.ts:472](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L472)

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

Defined in: [device/device.ts:244](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L244)

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

Defined in: [device/device.ts:251](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L251)

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

Defined in: [device/device.ts:302](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L302)

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

Defined in: [device/device.ts:437](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L437)

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

Defined in: [device/device.ts:450](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L450)

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

Defined in: [device/device.ts:457](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L457)

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

Defined in: [device/device.ts:466](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L466)

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

Defined in: [device/device.ts:482](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L482)

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

Defined in: [device/device.ts:526](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L526)

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

Defined in: [device/device.ts:531](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L531)

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

Defined in: [device/device.ts:542](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L542)

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

Defined in: [device/device.ts:548](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L548)

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

Defined in: [device/device.ts:554](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L554)

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

Defined in: [device/device.ts:561](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L561)

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

Defined in: [device/device.ts:566](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L566)

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

Defined in: [device/device.ts:574](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L574)

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

Defined in: [device/device.ts:596](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L596)

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

Defined in: [device/device.ts:601](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L601)

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

Defined in: [device/device.ts:608](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L608)

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

Defined in: [device/device.ts:612](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L612)

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

Defined in: [device/device.ts:618](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L618)

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

Defined in: [device/device.ts:623](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L623)

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

Defined in: [device/device.ts:632](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L632)

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

Defined in: [sensors/vl53l8/vl53l8.ts:451](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L451)

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

Defined in: [sensors/vl53l8/vl53l8.ts:477](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L477)

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

Defined in: [sensors/vl53l8/vl53l8.ts:490](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L490)

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

Defined in: [sensors/vl53l8/vl53l8.ts:501](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L501)

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

Defined in: [sensors/vl53l8/vl53l8.ts:529](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L529)

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

Defined in: [sensors/vl53l8/vl53l8.ts:535](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L535)

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

