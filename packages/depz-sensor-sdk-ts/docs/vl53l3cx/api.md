# VL53L3CX (ToF) — API reference

`Vl53l3cx` fixes the product on the 1D-family class `Vl53lx`; the family
surface (`Vl53lx`, `Vl53lxMeasurement`, the helpers and the bridge
codecs) follows it and is identical on every VL53L0X / L1CX / L1CB /
L3CX / L4CX page. Discovery, device-base, transport and other
cross-sensor symbols shared by every sensor live in the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Enumerations

- [Vl53lxCmd](enumerations/Vl53lxCmd.md)
- [Vl53lxRpt](enumerations/Vl53lxRpt.md)

## Classes

- [Vl53lxSensorDriver](classes/Vl53lxSensorDriver.md)
- [Vl53lxError](classes/Vl53lxError.md)
- [Vl53lxProtocolError](classes/Vl53lxProtocolError.md)
- [Vl53lx](classes/Vl53lx.md)
- [Vl53l3cx](classes/Vl53l3cx.md)

## Interfaces

- [Vl53lxInfo](interfaces/Vl53lxInfo.md)
- [Vl53lxTarget](interfaces/Vl53lxTarget.md)
- [Vl53lxDriverMeasurement](interfaces/Vl53lxDriverMeasurement.md)
- [Vl53lxMeasurement](interfaces/Vl53lxMeasurement.md)
- [HistogramDriverApi](interfaces/HistogramDriverApi.md)
- [Vl53lxProductInfo](interfaces/Vl53lxProductInfo.md)
- [Vl53lxOptions](interfaces/Vl53lxOptions.md)
- [Vl53lxConfigureOptions](interfaces/Vl53lxConfigureOptions.md)

## Type Aliases

- [Vl53lxClearStep](type-aliases/Vl53lxClearStep.md)

## Variables

- [VL53L4\_XSHUT\_OFF](variables/VL53L4_XSHUT_OFF.md)
- [VL53L4\_XSHUT\_ON](variables/VL53L4_XSHUT_ON.md)
- [VL53L4\_XSHUT\_RESET](variables/VL53L4_XSHUT_RESET.md)
- [VL53LX\_CLEAR\_STEPS\_MAX](variables/VL53LX_CLEAR_STEPS_MAX.md)
- [VL53LX\_INFO\_SIZE](variables/VL53LX_INFO_SIZE.md)
- [WINDOW\_BELOW](variables/WINDOW_BELOW.md)
- [WINDOW\_ABOVE](variables/WINDOW_ABOVE.md)
- [WINDOW\_OUT](variables/WINDOW_OUT.md)
- [WINDOW\_IN](variables/WINDOW_IN.md)
- [PLOTTABLE\_STATUSES](variables/PLOTTABLE_STATUSES.md)
- [VL53LX\_CLASS\_BY\_PRODUCT](variables/VL53LX_CLASS_BY_PRODUCT.md)
- [VL53LX\_PRODUCTS](variables/VL53LX_PRODUCTS.md)
- [VL53LX\_DRIVER\_KINDS](variables/VL53LX_DRIVER_KINDS.md)

## Functions

- [packVl53lxStartStream](functions/packVl53lxStartStream.md)
- [packVl53lxSetAddrWidth](functions/packVl53lxSetAddrWidth.md)
- [unpackVl53lxInfo](functions/unpackVl53lxInfo.md)
- [plotDistances](functions/plotDistances.md)
- [primaryTarget](functions/primaryTarget.md)
- [resolveVl53lxClass](functions/resolveVl53lxClass.md)

# Class: Vl53l3cx

Defined in: [src/sensors/vl53lx/vl53lx.ts:831](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L831)

VL53L3CX (3 m): ST's ULP driver (single target) or the histogram driver.

## Extends

- [`Vl53lx`](Vl53lx.md)

## Constructors

### Constructor

```ts
new Vl53l3cx(transport, opts?): Vl53l3cx;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:322](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L322)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | [`Vl53lxOptions`](../interfaces/Vl53lxOptions.md) |

#### Returns

`Vl53l3cx`

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`constructor`](Vl53lx.md#constructor)

## Properties

### timeoutMs

```ts
timeoutMs: number;
```

Defined in: [src/device/device.ts:178](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L178)

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`timeoutMs`](Vl53lx.md#timeoutms)

***

### link

```ts
protected readonly link: DepzLink;
```

Defined in: [src/device/device.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L180)

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`link`](Vl53lx.md#link)

***

### driverInst

```ts
protected driverInst: Vl53lxSensorDriver | null = null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:202](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L202)

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`driverInst`](Vl53lx.md#driverinst)

***

### productName

```ts
protected productName: string | null = null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:203](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L203)

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`productName`](Vl53lx.md#productname)

***

### driverKindName

```ts
protected driverKindName: string | null = null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:204](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L204)

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`driverKindName`](Vl53lx.md#driverkindname)

***

### caveat

```ts
protected caveat: string | null = null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:205](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L205)

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`caveat`](Vl53lx.md#caveat)

***

### rangingFlag

```ts
protected rangingFlag: boolean = false;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:209](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L209)

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`rangingFlag`](Vl53lx.md#rangingflag)

***

### bridge

```ts
readonly bridge: BridgeDevice;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:223](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L223)

The `BridgeDevice` the ULD ports talk to: register transfers split at the
bridge's 253-byte limit; a non-OK status or a missing answer surfaces as
`ProtocolError`, exactly what the ports catch.

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`bridge`](Vl53lx.md#bridge)

***

### PRODUCT

```ts
readonly static PRODUCT: string | null = "VL53L3CX";
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:832](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L832)

The product this class drives; null = read it from the device name.

#### Overrides

[`Vl53lx`](Vl53lx.md).[`PRODUCT`](Vl53lx.md#product)

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

[`Vl53lx`](Vl53lx.md).[`stats`](Vl53lx.md#stats)

***

### timeSync

#### Get Signature

```ts
get timeSync(): TimeSync | null;
```

Defined in: [src/device/device.ts:498](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L498)

##### Returns

`TimeSync` \| `null`

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`timeSync`](Vl53lx.md#timesync)

***

### product

#### Get Signature

```ts
get product(): string | null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:349](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L349)

The product init() bound (null before init).

##### Returns

`string` \| `null`

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`product`](Vl53lx.md#product-1)

***

### driverKind

#### Get Signature

```ts
get driverKind(): string | null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:353](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L353)

##### Returns

`string` \| `null`

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`driverKind`](Vl53lx.md#driverkind)

***

### driver

#### Get Signature

```ts
get driver(): Vl53lxSensorDriver;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:358](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L358)

The ULD port in use — escape hatch for product-specific calls.

##### Returns

[`Vl53lxSensorDriver`](Vl53lxSensorDriver.md)

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`driver`](Vl53lx.md#driver)

***

### initialized

#### Get Signature

```ts
get initialized(): boolean;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:363](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L363)

##### Returns

`boolean`

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`initialized`](Vl53lx.md#initialized)

***

### modes

#### Get Signature

```ts
get modes(): readonly string[];
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:470](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L470)

Named ranging modes, first = what init leaves; [] if none.

##### Returns

readonly `string`[]

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`modes`](Vl53lx.md#modes)

***

### ranging

#### Get Signature

```ts
get ranging(): boolean;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:670](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L670)

##### Returns

`boolean`

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`ranging`](Vl53lx.md#ranging)

***

### streamParseErrors

#### Get Signature

```ts
get streamParseErrors(): number;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:746](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L746)

Stream reports dropped because the driver failed to decode them.

##### Returns

`number`

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`streamParseErrors`](Vl53lx.md#streamparseerrors)

***

### streamDroppedCounts

#### Get Signature

```ts
get streamDroppedCounts(): number[];
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:751](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L751)

Drop counters of all live measurement queues (diagnostics).

##### Returns

`number`[]

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`streamDroppedCounts`](Vl53lx.md#streamdroppedcounts)

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

[`Vl53lx`](Vl53lx.md).[`open`](Vl53lx.md#open)

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/device/device.ts:205](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L205)

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`close`](Vl53lx.md#close)

***

### onTeardown()

```ts
protected onTeardown(error): void;
```

Defined in: [src/device/device.ts:234](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L234)

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

[`Vl53lx`](Vl53lx.md).[`onTeardown`](Vl53lx.md#onteardown)

***

### registerStream()

```ts
protected registerStream(stream): () => void;
```

Defined in: [src/device/device.ts:354](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L354)

Register a stream to be closed on device teardown.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `stream` | \{ `close`: `void`; \} |
| `stream.close` |

#### Returns

() => `void`

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`registerStream`](Vl53lx.md#registerstream)

***

### onEvent()

```ts
onEvent(cb): () => void;
```

Defined in: [src/device/device.ts:367](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L367)

Subscribe to unsolicited/diagnostic events (read-pump context; do not
block). Returns an unsubscribe function.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`ev`) => `void` |

#### Returns

() => `void`

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`onEvent`](Vl53lx.md#onevent)

***

### emitEvent()

```ts
protected emitEvent(event): void;
```

Defined in: [src/device/device.ts:374](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L374)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `event` | `DeviceEvent` |

#### Returns

`void`

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`emitEvent`](Vl53lx.md#emitevent)

***

### request()

```ts
request<T>(
   cmd, 
   payload?, 
opts?): Promise<T>;
```

Defined in: [src/device/device.ts:390](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L390)

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

[`Vl53lx`](Vl53lx.md).[`request`](Vl53lx.md#request)

***

### expectReport()

```ts
static expectReport<T>(reportId, unpack): Matcher<T>;
```

Defined in: [src/device/device.ts:433](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L433)

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

[`Vl53lx`](Vl53lx.md).[`expectReport`](Vl53lx.md#expectreport)

***

### expectText()

```ts
static expectText(requestCmd): Matcher<string>;
```

Defined in: [src/device/device.ts:438](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L438)

Matcher for RPT_TEXT echoing `requestCmd`.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `requestCmd` | `number` |

#### Returns

`Matcher`\<`string`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`expectText`](Vl53lx.md#expecttext)

***

### getDeviceName()

```ts
getDeviceName(): Promise<string>;
```

Defined in: [src/device/device.ts:449](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L449)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`getDeviceName`](Vl53lx.md#getdevicename)

***

### getSoftwareName()

```ts
getSoftwareName(): Promise<string>;
```

Defined in: [src/device/device.ts:455](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L455)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`getSoftwareName`](Vl53lx.md#getsoftwarename)

***

### getSerialNumber()

```ts
getSerialNumber(): Promise<string>;
```

Defined in: [src/device/device.ts:461](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L461)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`getSerialNumber`](Vl53lx.md#getserialnumber)

***

### identify()

```ts
identify(): Promise<Identity>;
```

Defined in: [src/device/device.ts:468](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L468)

Classify the running firmware (contract 02 §4).

#### Returns

`Promise`\<`Identity`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`identify`](Vl53lx.md#identify)

***

### readMcuTemperature()

```ts
readMcuTemperature(): Promise<number>;
```

Defined in: [src/device/device.ts:473](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L473)

Last cached MCU temperature in °C (device refreshes ~2 Hz).

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`readMcuTemperature`](Vl53lx.md#readmcutemperature)

***

### syncTime()

```ts
syncTime(samples?): Promise<TimeSync>;
```

Defined in: [src/device/device.ts:481](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L481)

NTP-style sync; keeps the lowest-RTT sample (contract 02 §5).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `samples` | `number` | `5` |

#### Returns

`Promise`\<`TimeSync`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`syncTime`](Vl53lx.md#synctime)

***

### toHostTimeUs()

```ts
toHostTimeUs(deviceTsUs): bigint;
```

Defined in: [src/device/device.ts:503](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L503)

Device µs → host monotonic µs (requires a prior `syncTime`).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `deviceTsUs` | `bigint` |

#### Returns

`bigint`

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`toHostTimeUs`](Vl53lx.md#tohosttimeus)

***

### getReportPayloadCrc()

```ts
getReportPayloadCrc(): Promise<CrcType>;
```

Defined in: [src/device/device.ts:508](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L508)

#### Returns

`Promise`\<`CrcType`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`getReportPayloadCrc`](Vl53lx.md#getreportpayloadcrc)

***

### setReportPayloadCrc()

```ts
setReportPayloadCrc(crcType): Promise<void>;
```

Defined in: [src/device/device.ts:515](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L515)

Set the device→host payload CRC mode (host→device is per-packet).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `crcType` | `CrcType` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`setReportPayloadCrc`](Vl53lx.md#setreportpayloadcrc)

***

### getSyncPin()

```ts
getSyncPin(pin): Promise<SyncPinConfig>;
```

Defined in: [src/device/device.ts:519](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L519)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pin` | `number` |

#### Returns

`Promise`\<`SyncPinConfig`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`getSyncPin`](Vl53lx.md#getsyncpin)

***

### setSyncPin()

```ts
setSyncPin(config): Promise<void>;
```

Defined in: [src/device/device.ts:525](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L525)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `config` | `SyncPinConfig` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`setSyncPin`](Vl53lx.md#setsyncpin)

***

### reset()

```ts
reset(): Promise<void>;
```

Defined in: [src/device/device.ts:530](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L530)

DEVICE_RESET: device ACKs then reboots; the link will drop.

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`reset`](Vl53lx.md#reset)

***

### enterBootloaderMode()

```ts
enterBootloaderMode(): Promise<void>;
```

Defined in: [src/device/device.ts:539](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L539)

Ask the device to reboot into the resident bootloader and close this
connection. Re-discovery/flash flow lives in the bootloader module
(contract 06).

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`enterBootloaderMode`](Vl53lx.md#enterbootloadermode)

***

### boardName()

```ts
boardName(): Promise<string>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:338](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L338)

The board's device name (bootloader metablock), e.g.
`DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523`. Read once, then cached.

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`boardName`](Vl53lx.md#boardname)

***

### detected()

```ts
detected(): Promise<string | null>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:344](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L344)

The product the device name carries, or null on an unstamped board.

#### Returns

`Promise`\<`string` \| `null`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`detected`](Vl53lx.md#detected)

***

### driverKinds()

```ts
driverKinds(product?): Promise<string[]>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:368](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L368)

The driver kinds a product has (default: this board's).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `product?` | `string` |

#### Returns

`Promise`\<`string`[]\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`driverKinds`](Vl53lx.md#driverkinds)

***

### init()

```ts
init(driver?, opts?): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:383](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L383)

Bind the (product, driver kind) pair and initialise the sensor.

`product` defaults to this class's product, else the board's device name;
`driver` defaults to the product's first kind (`uld`, else `ulp`, else
`histogram`). A pair the table has no row for throws
`NotImplementedError` naming what the product has.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `driver?` | `string` \| `null` |
| `opts?` | \{ `product?`: `string` \| `null`; \} |
| `opts.product?` | `string` \| `null` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`init`](Vl53lx.md#init)

***

### identifyProduct()

```ts
identifyProduct(): Promise<Vl53lxProductInfo>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:408](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L408)

Everything about what is connected (needs init()).

#### Returns

`Promise`\<[`Vl53lxProductInfo`](../interfaces/Vl53lxProductInfo.md)\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`identifyProduct`](Vl53lx.md#identifyproduct)

***

### notes()

```ts
notes(): Promise<string[]>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:438](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L438)

The lines a UI should show about the chosen pair: product named by hand,
a borrowed driver, a pair that reaches less than the board is rated for,
the driver's caveat.

#### Returns

`Promise`\<`string`[]\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`notes`](Vl53lx.md#notes)

***

### supports()

```ts
supports(group): boolean;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:465](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L465)

Whether this product/driver serves an optional capability group: mode,
timing, offset, calib_offset, xtalk, calib_xtalk, thresholds,
signal_thresh, sigma_thresh, roi, temp_update, refspad.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `group` | `string` |

#### Returns

`boolean`

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`supports`](Vl53lx.md#supports)

***

### xshut()

```ts
xshut(action): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:481](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L481)

Drive XSHUT: XSHUT_OFF / XSHUT_ON / XSHUT_RESET (1 ms pulse + 5 ms wait;
the host confirms the boot). OFF and RESET stop the stream; the sensor
then holds none of the configuration — init() again.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `action` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`xshut`](Vl53lx.md#xshut)

***

### bridgeInfo()

```ts
bridgeInfo(): Promise<Vl53lxInfo>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:488](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L488)

RPT_VL53_INFO — the bridge's own counters; safe while streaming.

#### Returns

`Promise`\<[`Vl53lxInfo`](../interfaces/Vl53lxInfo.md)\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`bridgeInfo`](Vl53lx.md#bridgeinfo)

***

### configure()

```ts
configure(opts?): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:502](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L502)

Re-initialise the sensor and apply a ranging configuration. The re-init
is deliberate (the only way to know what the configuration registers
hold). `mode` goes on before the budget; `offsetMm` / `xtalkKcps`
re-apply a stored calibration.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `opts` | [`Vl53lxConfigureOptions`](../interfaces/Vl53lxConfigureOptions.md) |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`configure`](Vl53lx.md#configure)

***

### getRangeTiming()

```ts
getRangeTiming(): Promise<[number, number]>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:513](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L513)

→ [timingBudgetMs, interMeasurementMs]; 0 for the period = continuous.

#### Returns

`Promise`\<\[`number`, `number`\]\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`getRangeTiming`](Vl53lx.md#getrangetiming)

***

### getMode()

```ts
getMode(): Promise<string | null>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:518](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L518)

The ranging mode in use, or null on a product without modes.

#### Returns

`Promise`\<`string` \| `null`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`getMode`](Vl53lx.md#getmode)

***

### budgetChoices()

```ts
budgetChoices(): number[];
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:523](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L523)

The only budgets accepted right now (ascending), or [] for any in range.

#### Returns

`number`[]

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`budgetChoices`](Vl53lx.md#budgetchoices)

***

### snapBudget()

```ts
snapBudget(budgetMs): number;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:528](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L528)

The nearest budget this product will actually accept.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `budgetMs` | `number` |

#### Returns

`number`

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`snapBudget`](Vl53lx.md#snapbudget)

***

### getOffsetMm()

```ts
getOffsetMm(): Promise<number>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:542](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L542)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`getOffsetMm`](Vl53lx.md#getoffsetmm)

***

### setOffsetMm()

```ts
setOffsetMm(offsetMm): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:546](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L546)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `offsetMm` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`setOffsetMm`](Vl53lx.md#setoffsetmm)

***

### getXtalkKcps()

```ts
getXtalkKcps(): Promise<number>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:551](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L551)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`getXtalkKcps`](Vl53lx.md#getxtalkkcps)

***

### setXtalkKcps()

```ts
setXtalkKcps(xtalkKcps): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:555](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L555)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `xtalkKcps` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`setXtalkKcps`](Vl53lx.md#setxtalkkcps)

***

### calibrateOffset()

```ts
calibrateOffset(targetDistMm, nbSamples?): Promise<number>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:564](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L564)

Offset calibration against a flat target at `targetDistMm`; returns the
offset now programmed. Store it on the host (sensor RAM, lost on reset).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetDistMm` | `number` |
| `nbSamples?` | `number` |

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`calibrateOffset`](Vl53lx.md#calibrateoffset)

***

### calibrateXtalk()

```ts
calibrateXtalk(targetDistMm, nbSamples?): Promise<number>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:570](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L570)

Crosstalk calibration; returns the xtalk now programmed (kcps).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetDistMm` | `number` |
| `nbSamples?` | `number` |

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`calibrateXtalk`](Vl53lx.md#calibratextalk)

***

### getDetectionThresholds()

```ts
getDetectionThresholds(): Promise<[number, number, number]>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:576](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L576)

→ [distanceLowMm, distanceHighMm, window].

#### Returns

`Promise`\<\[`number`, `number`, `number`\]\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`getDetectionThresholds`](Vl53lx.md#getdetectionthresholds)

***

### setDetectionThresholds()

```ts
setDetectionThresholds(
   distanceLowMm, 
   distanceHighMm, 
window): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:581](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L581)

Arm the distance-window interrupt; stays armed until the next init/configure.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `distanceLowMm` | `number` |
| `distanceHighMm` | `number` |
| `window` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`setDetectionThresholds`](Vl53lx.md#setdetectionthresholds)

***

### getSignalThresholdKcps()

```ts
getSignalThresholdKcps(): Promise<number>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:590](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L590)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`getSignalThresholdKcps`](Vl53lx.md#getsignalthresholdkcps)

***

### setSignalThresholdKcps()

```ts
setSignalThresholdKcps(signalKcps): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:594](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L594)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `signalKcps` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`setSignalThresholdKcps`](Vl53lx.md#setsignalthresholdkcps)

***

### getSigmaThresholdMm()

```ts
getSigmaThresholdMm(): Promise<number>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:599](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L599)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`getSigmaThresholdMm`](Vl53lx.md#getsigmathresholdmm)

***

### setSigmaThresholdMm()

```ts
setSigmaThresholdMm(sigmaMm): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:603](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L603)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `sigmaMm` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`setSigmaThresholdMm`](Vl53lx.md#setsigmathresholdmm)

***

### getRoi()

```ts
getRoi(): Promise<[number, number]>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:609](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L609)

→ [x, y] SPAD window size.

#### Returns

`Promise`\<\[`number`, `number`\]\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`getRoi`](Vl53lx.md#getroi)

***

### setRoi()

```ts
setRoi(x, y): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:613](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L613)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `x` | `number` |
| `y` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`setRoi`](Vl53lx.md#setroi)

***

### getRoiCenter()

```ts
getRoiCenter(): Promise<number>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:618](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L618)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`getRoiCenter`](Vl53lx.md#getroicenter)

***

### setRoiCenter()

```ts
setRoiCenter(centerSpad): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:622](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L622)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `centerSpad` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`setRoiCenter`](Vl53lx.md#setroicenter)

***

### startTemperatureUpdate()

```ts
startTemperatureUpdate(): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:628](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L628)

Re-run VHV after an ambient change over 8 °C.

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`startTemperatureUpdate`](Vl53lx.md#starttemperatureupdate)

***

### performRefSpadManagement()

```ts
performRefSpadManagement(): Promise<[number, number]>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:634](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L634)

VL53L0X: re-measure the reference SPADs → [count, isAperture].

#### Returns

`Promise`\<\[`number`, `number`\]\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`performRefSpadManagement`](Vl53lx.md#performrefspadmanagement)

***

### startRanging()

```ts
startRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:646](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L646)

Start the sensor's ranging loop and arm the MCU stream: one
RPT_VL53_STREAM per INT edge, followed on the MCU by the driver's
interrupt-release writes.

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`startRanging`](Vl53lx.md#startranging)

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:659](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L659)

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`stopRanging`](Vl53lx.md#stopranging)

***

### measureOnce()

```ts
measureOnce(timeoutMs?): Promise<Vl53lxMeasurement>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:678](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L678)

Single poll-mode measurement: start ranging, wait for data-ready, read,
release the interrupt, stop. Rejects while the stream runs.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `1000` |

#### Returns

`Promise`\<[`Vl53lxMeasurement`](../interfaces/Vl53lxMeasurement.md)\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`measureOnce`](Vl53lx.md#measureonce)

***

### onMeasurement()

```ts
onMeasurement(cb): () => void;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:702](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L702)

Subscribe to streamed measurements. Returns an unsubscribe function.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`m`) => `void` |

#### Returns

() => `void`

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`onMeasurement`](Vl53lx.md#onmeasurement)

***

### measurements()

```ts
measurements(maxsize?): StreamQueue<Vl53lxMeasurement>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:710](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L710)

Async iterator over measurements — bounded, drop-oldest (contract 07 §3).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `maxsize` | `number` | `64` |

#### Returns

`StreamQueue`\<[`Vl53lxMeasurement`](../interfaces/Vl53lxMeasurement.md)\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`measurements`](Vl53lx.md#measurements)

***

### getMeasurement()

```ts
getMeasurement(timeoutMs?): Promise<Vl53lxMeasurement>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:721](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L721)

Wait for the next streamed measurement.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `2000` |

#### Returns

`Promise`\<[`Vl53lxMeasurement`](../interfaces/Vl53lxMeasurement.md)\>

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`getMeasurement`](Vl53lx.md#getmeasurement)

***

### need()

```ts
protected need(group): Vl53lxSensorDriver & DriverExtras;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:757](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L757)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `group` | `string` |

#### Returns

[`Vl53lxSensorDriver`](Vl53lxSensorDriver.md) & `DriverExtras`

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`need`](Vl53lx.md#need)

***

### requireNotRanging()

```ts
protected requireNotRanging(): void;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:768](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L768)

#### Returns

`void`

#### Inherited from

[`Vl53lx`](Vl53lx.md).[`requireNotRanging`](Vl53lx.md#requirenotranging)

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:777](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L777)

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

[`Vl53lx`](Vl53lx.md).[`handleReport`](Vl53lx.md#handlereport)

# Class: Vl53lx

Defined in: [src/sensors/vl53lx/vl53lx.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L198)

A board of the VL53L 1D family (firmware `APP_VL53L0_4`).

`init()` binds the (product, driver kind) pair and runs the ULD's
sensorInit; `configure()` re-initialises and applies the ranging
configuration — call it before every run. Then `startRanging()` arms the MCU
stream and measurements arrive via `onMeasurement` / `measurements()` /
`getMeasurement()`. Configuration must not change while ranging.

This generic class serves any product (read from the board's device name,
or `product` at init); the per-product subclasses (`Vl53l0x`...) fix it.

## Extends

- `DepzDevice`

## Extended by

- [`Vl53l3cx`](Vl53l3cx.md)

## Constructors

### Constructor

```ts
new Vl53lx(transport, opts?): Vl53lx;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:322](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L322)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | [`Vl53lxOptions`](../interfaces/Vl53lxOptions.md) |

#### Returns

`Vl53lx`

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

### PRODUCT

```ts
readonly static PRODUCT: string | null = null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:200](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L200)

The product this class drives; null = read it from the device name.

***

### driverInst

```ts
protected driverInst: Vl53lxSensorDriver | null = null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:202](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L202)

***

### productName

```ts
protected productName: string | null = null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:203](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L203)

***

### driverKindName

```ts
protected driverKindName: string | null = null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:204](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L204)

***

### caveat

```ts
protected caveat: string | null = null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:205](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L205)

***

### rangingFlag

```ts
protected rangingFlag: boolean = false;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:209](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L209)

***

### bridge

```ts
readonly bridge: BridgeDevice;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:223](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L223)

The `BridgeDevice` the ULD ports talk to: register transfers split at the
bridge's 253-byte limit; a non-OK status or a missing answer surfaces as
`ProtocolError`, exactly what the ports catch.

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

Defined in: [src/device/device.ts:498](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L498)

##### Returns

`TimeSync` \| `null`

#### Inherited from

```ts
DepzDevice.timeSync
```

***

### product

#### Get Signature

```ts
get product(): string | null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:349](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L349)

The product init() bound (null before init).

##### Returns

`string` \| `null`

***

### driverKind

#### Get Signature

```ts
get driverKind(): string | null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:353](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L353)

##### Returns

`string` \| `null`

***

### driver

#### Get Signature

```ts
get driver(): Vl53lxSensorDriver;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:358](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L358)

The ULD port in use — escape hatch for product-specific calls.

##### Returns

[`Vl53lxSensorDriver`](Vl53lxSensorDriver.md)

***

### initialized

#### Get Signature

```ts
get initialized(): boolean;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:363](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L363)

##### Returns

`boolean`

***

### modes

#### Get Signature

```ts
get modes(): readonly string[];
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:470](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L470)

Named ranging modes, first = what init leaves; [] if none.

##### Returns

readonly `string`[]

***

### ranging

#### Get Signature

```ts
get ranging(): boolean;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:670](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L670)

##### Returns

`boolean`

***

### streamParseErrors

#### Get Signature

```ts
get streamParseErrors(): number;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:746](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L746)

Stream reports dropped because the driver failed to decode them.

##### Returns

`number`

***

### streamDroppedCounts

#### Get Signature

```ts
get streamDroppedCounts(): number[];
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:751](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L751)

Drop counters of all live measurement queues (diagnostics).

##### Returns

`number`[]

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

### onTeardown()

```ts
protected onTeardown(error): void;
```

Defined in: [src/device/device.ts:234](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L234)

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

Defined in: [src/device/device.ts:354](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L354)

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

Defined in: [src/device/device.ts:367](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L367)

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

Defined in: [src/device/device.ts:374](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L374)

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

Defined in: [src/device/device.ts:390](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L390)

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

Defined in: [src/device/device.ts:433](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L433)

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

Defined in: [src/device/device.ts:438](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L438)

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

Defined in: [src/device/device.ts:449](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L449)

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

Defined in: [src/device/device.ts:455](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L455)

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

Defined in: [src/device/device.ts:461](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L461)

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

Defined in: [src/device/device.ts:468](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L468)

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

Defined in: [src/device/device.ts:473](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L473)

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

Defined in: [src/device/device.ts:481](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L481)

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

Defined in: [src/device/device.ts:503](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L503)

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

Defined in: [src/device/device.ts:508](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L508)

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

Defined in: [src/device/device.ts:515](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L515)

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

Defined in: [src/device/device.ts:519](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L519)

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

Defined in: [src/device/device.ts:525](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L525)

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

Defined in: [src/device/device.ts:530](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L530)

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

Defined in: [src/device/device.ts:539](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L539)

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

### boardName()

```ts
boardName(): Promise<string>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:338](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L338)

The board's device name (bootloader metablock), e.g.
`DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523`. Read once, then cached.

#### Returns

`Promise`\<`string`\>

***

### detected()

```ts
detected(): Promise<string | null>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:344](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L344)

The product the device name carries, or null on an unstamped board.

#### Returns

`Promise`\<`string` \| `null`\>

***

### driverKinds()

```ts
driverKinds(product?): Promise<string[]>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:368](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L368)

The driver kinds a product has (default: this board's).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `product?` | `string` |

#### Returns

`Promise`\<`string`[]\>

***

### init()

```ts
init(driver?, opts?): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:383](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L383)

Bind the (product, driver kind) pair and initialise the sensor.

`product` defaults to this class's product, else the board's device name;
`driver` defaults to the product's first kind (`uld`, else `ulp`, else
`histogram`). A pair the table has no row for throws
`NotImplementedError` naming what the product has.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `driver?` | `string` \| `null` |
| `opts?` | \{ `product?`: `string` \| `null`; \} |
| `opts.product?` | `string` \| `null` |

#### Returns

`Promise`\<`void`\>

***

### identifyProduct()

```ts
identifyProduct(): Promise<Vl53lxProductInfo>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:408](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L408)

Everything about what is connected (needs init()).

#### Returns

`Promise`\<[`Vl53lxProductInfo`](../interfaces/Vl53lxProductInfo.md)\>

***

### notes()

```ts
notes(): Promise<string[]>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:438](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L438)

The lines a UI should show about the chosen pair: product named by hand,
a borrowed driver, a pair that reaches less than the board is rated for,
the driver's caveat.

#### Returns

`Promise`\<`string`[]\>

***

### supports()

```ts
supports(group): boolean;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:465](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L465)

Whether this product/driver serves an optional capability group: mode,
timing, offset, calib_offset, xtalk, calib_xtalk, thresholds,
signal_thresh, sigma_thresh, roi, temp_update, refspad.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `group` | `string` |

#### Returns

`boolean`

***

### xshut()

```ts
xshut(action): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:481](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L481)

Drive XSHUT: XSHUT_OFF / XSHUT_ON / XSHUT_RESET (1 ms pulse + 5 ms wait;
the host confirms the boot). OFF and RESET stop the stream; the sensor
then holds none of the configuration — init() again.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `action` | `number` |

#### Returns

`Promise`\<`void`\>

***

### bridgeInfo()

```ts
bridgeInfo(): Promise<Vl53lxInfo>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:488](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L488)

RPT_VL53_INFO — the bridge's own counters; safe while streaming.

#### Returns

`Promise`\<[`Vl53lxInfo`](../interfaces/Vl53lxInfo.md)\>

***

### configure()

```ts
configure(opts?): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:502](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L502)

Re-initialise the sensor and apply a ranging configuration. The re-init
is deliberate (the only way to know what the configuration registers
hold). `mode` goes on before the budget; `offsetMm` / `xtalkKcps`
re-apply a stored calibration.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `opts` | [`Vl53lxConfigureOptions`](../interfaces/Vl53lxConfigureOptions.md) |

#### Returns

`Promise`\<`void`\>

***

### getRangeTiming()

```ts
getRangeTiming(): Promise<[number, number]>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:513](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L513)

→ [timingBudgetMs, interMeasurementMs]; 0 for the period = continuous.

#### Returns

`Promise`\<\[`number`, `number`\]\>

***

### getMode()

```ts
getMode(): Promise<string | null>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:518](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L518)

The ranging mode in use, or null on a product without modes.

#### Returns

`Promise`\<`string` \| `null`\>

***

### budgetChoices()

```ts
budgetChoices(): number[];
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:523](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L523)

The only budgets accepted right now (ascending), or [] for any in range.

#### Returns

`number`[]

***

### snapBudget()

```ts
snapBudget(budgetMs): number;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:528](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L528)

The nearest budget this product will actually accept.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `budgetMs` | `number` |

#### Returns

`number`

***

### getOffsetMm()

```ts
getOffsetMm(): Promise<number>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:542](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L542)

#### Returns

`Promise`\<`number`\>

***

### setOffsetMm()

```ts
setOffsetMm(offsetMm): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:546](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L546)

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

Defined in: [src/sensors/vl53lx/vl53lx.ts:551](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L551)

#### Returns

`Promise`\<`number`\>

***

### setXtalkKcps()

```ts
setXtalkKcps(xtalkKcps): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:555](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L555)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `xtalkKcps` | `number` |

#### Returns

`Promise`\<`void`\>

***

### calibrateOffset()

```ts
calibrateOffset(targetDistMm, nbSamples?): Promise<number>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:564](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L564)

Offset calibration against a flat target at `targetDistMm`; returns the
offset now programmed. Store it on the host (sensor RAM, lost on reset).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetDistMm` | `number` |
| `nbSamples?` | `number` |

#### Returns

`Promise`\<`number`\>

***

### calibrateXtalk()

```ts
calibrateXtalk(targetDistMm, nbSamples?): Promise<number>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:570](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L570)

Crosstalk calibration; returns the xtalk now programmed (kcps).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetDistMm` | `number` |
| `nbSamples?` | `number` |

#### Returns

`Promise`\<`number`\>

***

### getDetectionThresholds()

```ts
getDetectionThresholds(): Promise<[number, number, number]>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:576](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L576)

→ [distanceLowMm, distanceHighMm, window].

#### Returns

`Promise`\<\[`number`, `number`, `number`\]\>

***

### setDetectionThresholds()

```ts
setDetectionThresholds(
   distanceLowMm, 
   distanceHighMm, 
window): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:581](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L581)

Arm the distance-window interrupt; stays armed until the next init/configure.

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

Defined in: [src/sensors/vl53lx/vl53lx.ts:590](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L590)

#### Returns

`Promise`\<`number`\>

***

### setSignalThresholdKcps()

```ts
setSignalThresholdKcps(signalKcps): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:594](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L594)

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

Defined in: [src/sensors/vl53lx/vl53lx.ts:599](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L599)

#### Returns

`Promise`\<`number`\>

***

### setSigmaThresholdMm()

```ts
setSigmaThresholdMm(sigmaMm): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:603](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L603)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `sigmaMm` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getRoi()

```ts
getRoi(): Promise<[number, number]>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:609](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L609)

→ [x, y] SPAD window size.

#### Returns

`Promise`\<\[`number`, `number`\]\>

***

### setRoi()

```ts
setRoi(x, y): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:613](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L613)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `x` | `number` |
| `y` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getRoiCenter()

```ts
getRoiCenter(): Promise<number>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:618](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L618)

#### Returns

`Promise`\<`number`\>

***

### setRoiCenter()

```ts
setRoiCenter(centerSpad): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:622](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L622)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `centerSpad` | `number` |

#### Returns

`Promise`\<`void`\>

***

### startTemperatureUpdate()

```ts
startTemperatureUpdate(): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:628](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L628)

Re-run VHV after an ambient change over 8 °C.

#### Returns

`Promise`\<`void`\>

***

### performRefSpadManagement()

```ts
performRefSpadManagement(): Promise<[number, number]>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:634](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L634)

VL53L0X: re-measure the reference SPADs → [count, isAperture].

#### Returns

`Promise`\<\[`number`, `number`\]\>

***

### startRanging()

```ts
startRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:646](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L646)

Start the sensor's ranging loop and arm the MCU stream: one
RPT_VL53_STREAM per INT edge, followed on the MCU by the driver's
interrupt-release writes.

#### Returns

`Promise`\<`void`\>

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:659](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L659)

#### Returns

`Promise`\<`void`\>

***

### measureOnce()

```ts
measureOnce(timeoutMs?): Promise<Vl53lxMeasurement>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:678](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L678)

Single poll-mode measurement: start ranging, wait for data-ready, read,
release the interrupt, stop. Rejects while the stream runs.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `1000` |

#### Returns

`Promise`\<[`Vl53lxMeasurement`](../interfaces/Vl53lxMeasurement.md)\>

***

### onMeasurement()

```ts
onMeasurement(cb): () => void;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:702](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L702)

Subscribe to streamed measurements. Returns an unsubscribe function.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`m`) => `void` |

#### Returns

() => `void`

***

### measurements()

```ts
measurements(maxsize?): StreamQueue<Vl53lxMeasurement>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:710](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L710)

Async iterator over measurements — bounded, drop-oldest (contract 07 §3).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `maxsize` | `number` | `64` |

#### Returns

`StreamQueue`\<[`Vl53lxMeasurement`](../interfaces/Vl53lxMeasurement.md)\>

***

### getMeasurement()

```ts
getMeasurement(timeoutMs?): Promise<Vl53lxMeasurement>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:721](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L721)

Wait for the next streamed measurement.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `2000` |

#### Returns

`Promise`\<[`Vl53lxMeasurement`](../interfaces/Vl53lxMeasurement.md)\>

***

### need()

```ts
protected need(group): Vl53lxSensorDriver & DriverExtras;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:757](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L757)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `group` | `string` |

#### Returns

[`Vl53lxSensorDriver`](Vl53lxSensorDriver.md) & `DriverExtras`

***

### requireNotRanging()

```ts
protected requireNotRanging(): void;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:768](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L768)

#### Returns

`void`

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:777](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L777)

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

# Class: Vl53lxError

Defined in: [src/sensors/vl53lx/uld/link.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/link.ts#L23)

A ULD-level failure: the sensor did not do what the driver needed (timeout
waiting for data-ready or boot, a calibration that failed...).

## Extends

- `DepzError`

## Constructors

### Constructor

```ts
new Vl53lxError(message?): Vl53Error;
```

Defined in: [src/errors.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L8)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `message?` | `string` |

#### Returns

`Vl53Error`

#### Inherited from

```ts
DepzError.constructor
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
DepzError.stackTraceLimit
```

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

```ts
DepzError.cause
```

***

### name

```ts
name: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

```ts
DepzError.name
```

***

### message

```ts
message: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

```ts
DepzError.message
```

***

### stack?

```ts
optional stack?: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

```ts
DepzError.stack
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
DepzError.captureStackTrace
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
DepzError.prepareStackTrace
```

# Class: Vl53lxProtocolError

Defined in: [src/sensors/vl53lx/uld/link.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/link.ts#L30)

The bridge refused a register command or did not answer it — the firmware
repo's single error for both. The ports catch it where the sensor
legitimately NACKs for a while (e.g. right after a soft reset).

## Extends

- `DepzError`

## Constructors

### Constructor

```ts
new Vl53lxProtocolError(message?): ProtocolError;
```

Defined in: [src/errors.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L8)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `message?` | `string` |

#### Returns

`ProtocolError`

#### Inherited from

```ts
DepzError.constructor
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
DepzError.stackTraceLimit
```

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

```ts
DepzError.cause
```

***

### name

```ts
name: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

```ts
DepzError.name
```

***

### message

```ts
message: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

```ts
DepzError.message
```

***

### stack?

```ts
optional stack?: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

```ts
DepzError.stack
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
DepzError.captureStackTrace
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
DepzError.prepareStackTrace
```

# Abstract Class: Vl53lxSensorDriver

Defined in: [src/sensors/vl53lx/uld/base.ts:142](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L142)

Everything a caller may use on a sensor driver (mirror of the Python
`SensorDriver`). Anything else a driver defines belongs to that product and
is reached off the concrete class. `SUPPORTS` says which optional groups
the product actually has: mode, timing, offset, xtalk, calib_offset,
calib_xtalk, thresholds, roi, signal_thresh, sigma_thresh, temp_update,
refspad.

Static facts (`ADDR_WIDTH`, `CLEAR_STEPS`, `MAX_KHZ`, `SUPPORTS`, `MODES`,
`BUDGET_MS`, `HISTOGRAM`) are exposed both as static members of the driver
class (the registry reads them before construction) and as instance getters.

## Constructors

### Constructor

```ts
new Vl53lxSensorDriver(p, product): SensorDriver;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:151](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L151)

#### Parameters

| Parameter | Type | Description |
| ------ | ------ | ------ |
| `p` | `BridgePlatform` | - |
| `product` | `string` | The product the board reports, e.g. 'VL53L4CX' (one driver serves several). |

#### Returns

`SensorDriver`

## Properties

### ADDR\_WIDTH

```ts
readonly static ADDR_WIDTH: number = 2;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:143](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L143)

***

### CLEAR\_STEPS

```ts
readonly static CLEAR_STEPS: readonly readonly [number, number][] = [];
```

Defined in: [src/sensors/vl53lx/uld/base.ts:144](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L144)

***

### MAX\_KHZ

```ts
readonly static MAX_KHZ: number = 400;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:145](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L145)

***

### SUPPORTS

```ts
readonly static SUPPORTS: ReadonlySet<string>;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:146](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L146)

***

### MODES

```ts
readonly static MODES: readonly string[] = [];
```

Defined in: [src/sensors/vl53lx/uld/base.ts:147](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L147)

***

### BUDGET\_MS

```ts
readonly static BUDGET_MS: readonly [number, number];
```

Defined in: [src/sensors/vl53lx/uld/base.ts:148](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L148)

***

### HISTOGRAM

```ts
readonly static HISTOGRAM: boolean = false;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:149](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L149)

***

### p

```ts
readonly p: BridgePlatform;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:152](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L152)

***

### product

```ts
readonly product: string;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:154](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L154)

The product the board reports, e.g. 'VL53L4CX' (one driver serves several).

## Accessors

### ADDR\_WIDTH

#### Get Signature

```ts
get ADDR_WIDTH(): number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:160](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L160)

##### Returns

`number`

***

### CLEAR\_STEPS

#### Get Signature

```ts
get CLEAR_STEPS(): readonly readonly [number, number][];
```

Defined in: [src/sensors/vl53lx/uld/base.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L163)

##### Returns

readonly readonly \[`number`, `number`\][]

***

### MAX\_KHZ

#### Get Signature

```ts
get MAX_KHZ(): number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:166](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L166)

##### Returns

`number`

***

### SUPPORTS

#### Get Signature

```ts
get SUPPORTS(): ReadonlySet<string>;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:169](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L169)

##### Returns

`ReadonlySet`\<`string`\>

***

### MODES

#### Get Signature

```ts
get MODES(): readonly string[];
```

Defined in: [src/sensors/vl53lx/uld/base.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L172)

##### Returns

readonly `string`[]

***

### BUDGET\_MS

#### Get Signature

```ts
get BUDGET_MS(): readonly [number, number];
```

Defined in: [src/sensors/vl53lx/uld/base.ts:175](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L175)

##### Returns

readonly \[`number`, `number`\]

***

### HISTOGRAM

#### Get Signature

```ts
get HISTOGRAM(): boolean;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:178](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L178)

##### Returns

`boolean`

## Methods

### modelId()

```ts
abstract modelId(): Promise<number>;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:183](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L183)

#### Returns

`Promise`\<`number`\>

***

### sensorInit()

```ts
abstract sensorInit(): Promise<void>;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:185](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L185)

Reset and configure; leaves the bus at MAX_KHZ. Safe to re-run.

#### Returns

`Promise`\<`void`\>

***

### startRanging()

```ts
abstract startRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:186](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L186)

#### Returns

`Promise`\<`void`\>

***

### stopRanging()

```ts
abstract stopRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L187)

#### Returns

`Promise`\<`void`\>

***

### checkForDataReady()

```ts
abstract checkForDataReady(): Promise<boolean>;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:190](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L190)

#### Returns

`Promise`\<`boolean`\>

***

### waitDataReady()

```ts
waitDataReady(timeoutS?): Promise<void>;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:192](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L192)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutS` | `number` | `1.0` |

#### Returns

`Promise`\<`void`\>

***

### clearInterrupt()

```ts
abstract clearInterrupt(): Promise<void>;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:200](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L200)

#### Returns

`Promise`\<`void`\>

***

### streamBlock()

```ts
abstract streamBlock(): [number, number];
```

Defined in: [src/sensors/vl53lx/uld/base.ts:204](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L204)

[addr, len] — the register block the bridge streams on each INT.

#### Returns

\[`number`, `number`\]

***

### decode()

```ts
abstract decode(raw): Vl53lxDriverMeasurement;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:206](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L206)

Streamed block bytes → Measurement (pure; stateful on histogram parts).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `raw` | `Uint8Array` |

#### Returns

[`Vl53lxDriverMeasurement`](../interfaces/Vl53lxDriverMeasurement.md)

***

### readMeasurement()

```ts
readMeasurement(): Promise<Vl53lxDriverMeasurement>;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:209](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L209)

One polled result. Default: read the stream block and decode it.

#### Returns

`Promise`\<[`Vl53lxDriverMeasurement`](../interfaces/Vl53lxDriverMeasurement.md)\>

***

### setRangeTiming()

```ts
abstract setRangeTiming(timingBudgetMs, interMeasurementMs): Promise<void>;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:215](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L215)

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
abstract getRangeTiming(): Promise<[number, number]>;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:217](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L217)

→ [timingBudgetMs, interMeasurementMs].

#### Returns

`Promise`\<\[`number`, `number`\]\>

***

### budgetChoices()

```ts
budgetChoices(): number[];
```

Defined in: [src/sensors/vl53lx/uld/base.ts:220](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L220)

The only budgets accepted right now (ascending), or [] for any in BUDGET_MS.

#### Returns

`number`[]

***

### setMode()

```ts
setMode(_name): Promise<void>;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:225](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L225)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `_name` | `string` |

#### Returns

`Promise`\<`void`\>

***

### getMode()

```ts
getMode(): Promise<string>;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:228](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L228)

#### Returns

`Promise`\<`string`\>

***

### reachMm()

```ts
reachMm(): number | null;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:233](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L233)

How far this driver's configuration can measure (mm), or null.

#### Returns

`number` \| `null`

***

### driverInfo()

```ts
driverInfo(): Promise<Record<string, unknown>>;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:238](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L238)

Numbers beyond the contract, for display only.

#### Returns

`Promise`\<`Record`\<`string`, `unknown`\>\>

# Enumeration: Vl53lxCmd

Defined in: [src/protocol/vl53lx.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L20)

VL53L 1D-family register-bridge wire codecs, protocol v2.01
(contracts/12_SENSOR_VL53LX.md). Mirrors the Python reference
`depz_sensor_sdk.protocol.vl53lx`.

One firmware (`APP_VL53L0_4`) serves VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX,
VL53L4CD and VL53L4CX. It is the VL53L4CD bridge of contract 10 with the
three sensor-specific facts moved to the host: the register-address width
(VL53_SET_ADDR_WIDTH, new), the interrupt-release writes (carried by
VL53_START_STREAM) and the boot handshake (no longer inside VL53_XSHUT).
READ_REG, WRITE_REG, XSHUT, STOP_STREAM, SET_I2C_SPEED and the REG_DATA /
STREAM reports are the contract-10 codecs of `protocol/vl53l4.ts`, identical
on the wire — import them from there. v2.01 (firmware v0.24) adds
VL53_CLEAR_I2C_ERRORS; a v2.00 bridge refuses it with ERR_INVALID_CMD.

Every export here carries a `Vl53lx`/`VL53LX_` name so the root index can
re-export this module with `export *` next to the other VL53 protocols.

## Enumeration Members

### ReadReg

```ts
ReadReg: 50;
```

Defined in: [src/protocol/vl53lx.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L21)

***

### WriteReg

```ts
WriteReg: 51;
```

Defined in: [src/protocol/vl53lx.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L22)

***

### Xshut

```ts
Xshut: 52;
```

Defined in: [src/protocol/vl53lx.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L23)

***

### StartStream

```ts
StartStream: 53;
```

Defined in: [src/protocol/vl53lx.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L24)

***

### StopStream

```ts
StopStream: 54;
```

Defined in: [src/protocol/vl53lx.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L25)

***

### GetInfo

```ts
GetInfo: 55;
```

Defined in: [src/protocol/vl53lx.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L26)

***

### SetI2cSpeed

```ts
SetI2cSpeed: 56;
```

Defined in: [src/protocol/vl53lx.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L27)

***

### SetAddrWidth

```ts
SetAddrWidth: 57;
```

Defined in: [src/protocol/vl53lx.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L28)

***

### ClearI2cErrors

```ts
ClearI2cErrors: 58;
```

Defined in: [src/protocol/vl53lx.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L29)

# Enumeration: Vl53lxRpt

Defined in: [src/protocol/vl53lx.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L32)

## Enumeration Members

### RegData

```ts
RegData: 145;
```

Defined in: [src/protocol/vl53lx.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L33)

***

### Info

```ts
Info: 146;
```

Defined in: [src/protocol/vl53lx.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L34)

***

### Stream

```ts
Stream: 147;
```

Defined in: [src/protocol/vl53lx.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L35)

# Function: packVl53lxSetAddrWidth()

```ts
function packVl53lxSetAddrWidth(width): Uint8Array;
```

Defined in: [src/protocol/vl53lx.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L73)

VL53_SET_ADDR_WIDTH payload: register-address width, 1 or 2 bytes.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `width` | `number` |

## Returns

`Uint8Array`

# Function: packVl53lxStartStream()

```ts
function packVl53lxStartStream(
   addr, 
   length, 
   clear?, 
   flags?): Uint8Array;
```

Defined in: [src/protocol/vl53lx.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L50)

vl53_start_stream_t: block, flags, then `clear` — the (addr, value) writes
the bridge plays after every block read (0..4 steps). 6+3n bytes.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `addr` | `number` | `undefined` |
| `length` | `number` | `undefined` |
| `clear` | readonly [`Vl53lxClearStep`](../type-aliases/Vl53lxClearStep.md)[] | `[]` |
| `flags` | `number` | `0` |

## Returns

`Uint8Array`

# Function: plotDistances()

```ts
function plotDistances(m): number[];
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:96](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L96)

The distances a chart should draw for one measurement: the plottable
targets in driver order, or the single distance on a light driver.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `m` | [`Vl53lxDriverMeasurement`](../interfaces/Vl53lxDriverMeasurement.md) |

## Returns

`number`[]

# Function: primaryTarget()

```ts
function primaryTarget(m): Vl53lxTarget | null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:104](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L104)

The first plottable target, or null when the frame produced nothing usable.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `m` | [`Vl53lxDriverMeasurement`](../interfaces/Vl53lxDriverMeasurement.md) |

## Returns

[`Vl53lxTarget`](../interfaces/Vl53lxTarget.md) \| `null`

# Function: resolveVl53lxClass()

```ts
function resolveVl53lxClass(usbModel, deviceName): typeof Vl53lx;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:856](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L856)

Pick the 1D-family class (contract 12 §1): the production PID model, then
the product the device name carries, then the generic class.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `usbModel` | `string` \| `null` \| `undefined` |
| `deviceName` | `string` \| `null` \| `undefined` |

## Returns

*typeof* [`Vl53lx`](../classes/Vl53lx.md)

# Function: unpackVl53lxInfo()

```ts
function unpackVl53lxInfo(payload): Vl53lxInfo;
```

Defined in: [src/protocol/vl53lx.ts:104](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L104)

`<IIIBBBHBBI`; rejects a payload shorter than 23 bytes.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Vl53lxInfo`](../interfaces/Vl53lxInfo.md)

# Interface: HistogramDriverApi

Defined in: [src/sensors/vl53lx/vl53lx.ts:115](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L115)

What the device class needs from the histogram (Bare) driver beyond
`SensorDriver`: `binData(raw)` steps the frame-pair state and returns the
bin object, `toMeasurement(bins)` finds the targets. Both run exactly once
per frame.

## Methods

### binData()

```ts
binData(raw): unknown;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:116](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L116)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `raw` | `Uint8Array` |

#### Returns

`unknown`

***

### toMeasurement()

```ts
toMeasurement(bins): Vl53lxDriverMeasurement;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:117](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L117)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `bins` | `unknown` |

#### Returns

[`Vl53lxDriverMeasurement`](Vl53lxDriverMeasurement.md)

# Interface: Vl53lxConfigureOptions

Defined in: [src/sensors/vl53lx/vl53lx.ts:169](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L169)

## Properties

### budgetMs?

```ts
optional budgetMs?: number;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:171](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L171)

Timing budget, ms (see budgetChoices()). Default 50.

***

### interMs?

```ts
optional interMs?: number;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:173](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L173)

0 = continuous; otherwise the period between measurements (> budget).

***

### mode?

```ts
optional mode?: string | null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:175](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L175)

One of `modes`, applied before the budget.

***

### offsetMm?

```ts
optional offsetMm?: number | null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:177](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L177)

Re-apply a stored offset calibration.

***

### xtalkKcps?

```ts
optional xtalkKcps?: number | null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:179](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L179)

Re-apply a stored crosstalk calibration.

# Interface: Vl53lxDriverMeasurement

Defined in: [src/sensors/vl53lx/uld/base.ts:103](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L103)

One range result, the same shape for every sensor of the family. On the
histogram driver `targets` holds every return strongest-first and the
top-level fields repeat `targets[0]`; on the light drivers it stays empty.

## Extended by

- [`Vl53lxMeasurement`](Vl53lxMeasurement.md)

## Properties

### distanceMm

```ts
distanceMm: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:104](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L104)

***

### status

```ts
status: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L105)

***

### statusText

```ts
statusText: string;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:106](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L106)

***

### signalKcps

```ts
signalKcps: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:107](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L107)

***

### ambientKcps

```ts
ambientKcps: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:108](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L108)

***

### sigmaMm

```ts
sigmaMm: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:109](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L109)

***

### spads

```ts
spads: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:110](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L110)

***

### targets

```ts
targets: Vl53lxTarget[];
```

Defined in: [src/sensors/vl53lx/uld/base.ts:111](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L111)

***

### extra

```ts
extra: Record<string, unknown>;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:112](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L112)

# Interface: Vl53lxInfo

Defined in: [src/protocol/vl53lx.ts:89](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L89)

RPT_VL53_INFO (v2.00, 23 bytes) — bridge state only; the bridge reads no
sensor register. Counters are free-running (wrap silently): watch
increments. `slotsSkipped` = a slot that never got the bus, `i2cErrors` =
a bus that answered badly (since power-up, the last XSHUT reset or
VL53_CLEAR_I2C_ERRORS — the SDK sends that at the end of every sensor init,
so the NACKs of a resetting die are not counted), `framesDropped` = a good
sample the USB TX ring had no room for (since the stream was armed).

## Properties

### intEdges

```ts
intEdges: number;
```

Defined in: [src/protocol/vl53lx.ts:90](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L90)

***

### slotsSkipped

```ts
slotsSkipped: number;
```

Defined in: [src/protocol/vl53lx.ts:91](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L91)

***

### i2cErrors

```ts
i2cErrors: number;
```

Defined in: [src/protocol/vl53lx.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L92)

***

### lastI2cError

```ts
lastI2cError: number;
```

Defined in: [src/protocol/vl53lx.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L94)

0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR.

***

### xshutLevel

```ts
xshutLevel: number;
```

Defined in: [src/protocol/vl53lx.ts:95](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L95)

***

### intLevel

```ts
intLevel: number;
```

Defined in: [src/protocol/vl53lx.ts:96](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L96)

***

### i2cKhz

```ts
i2cKhz: number;
```

Defined in: [src/protocol/vl53lx.ts:97](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L97)

***

### addrWidth

```ts
addrWidth: number;
```

Defined in: [src/protocol/vl53lx.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L98)

***

### nClear

```ts
nClear: number;
```

Defined in: [src/protocol/vl53lx.ts:99](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L99)

***

### framesDropped

```ts
framesDropped: number;
```

Defined in: [src/protocol/vl53lx.ts:100](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L100)

# Interface: Vl53lxMeasurement

Defined in: [src/sensors/vl53lx/vl53lx.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L64)

One ranging result, the same shape for every product of the family. On
the histogram driver `targets` holds every return strongest-first and the
top-level fields repeat `targets[0]`; `bins` carries the raw histogram
(the driver's bin object). On the light drivers `targets` is empty and
`bins` is null. `extra` is driver-specific — show it, don't branch on it.

## Extends

- [`Vl53lxDriverMeasurement`](Vl53lxDriverMeasurement.md)

## Properties

### distanceMm

```ts
distanceMm: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:104](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L104)

#### Inherited from

[`Vl53lxDriverMeasurement`](Vl53lxDriverMeasurement.md).[`distanceMm`](Vl53lxDriverMeasurement.md#distancemm)

***

### status

```ts
status: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L105)

#### Inherited from

[`Vl53lxDriverMeasurement`](Vl53lxDriverMeasurement.md).[`status`](Vl53lxDriverMeasurement.md#status)

***

### statusText

```ts
statusText: string;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:106](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L106)

#### Inherited from

[`Vl53lxDriverMeasurement`](Vl53lxDriverMeasurement.md).[`statusText`](Vl53lxDriverMeasurement.md#statustext)

***

### signalKcps

```ts
signalKcps: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:107](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L107)

#### Inherited from

[`Vl53lxDriverMeasurement`](Vl53lxDriverMeasurement.md).[`signalKcps`](Vl53lxDriverMeasurement.md#signalkcps)

***

### ambientKcps

```ts
ambientKcps: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:108](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L108)

#### Inherited from

[`Vl53lxDriverMeasurement`](Vl53lxDriverMeasurement.md).[`ambientKcps`](Vl53lxDriverMeasurement.md#ambientkcps)

***

### sigmaMm

```ts
sigmaMm: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:109](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L109)

#### Inherited from

[`Vl53lxDriverMeasurement`](Vl53lxDriverMeasurement.md).[`sigmaMm`](Vl53lxDriverMeasurement.md#sigmamm)

***

### spads

```ts
spads: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:110](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L110)

#### Inherited from

[`Vl53lxDriverMeasurement`](Vl53lxDriverMeasurement.md).[`spads`](Vl53lxDriverMeasurement.md#spads)

***

### targets

```ts
targets: Vl53lxTarget[];
```

Defined in: [src/sensors/vl53lx/uld/base.ts:111](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L111)

#### Inherited from

[`Vl53lxDriverMeasurement`](Vl53lxDriverMeasurement.md).[`targets`](Vl53lxDriverMeasurement.md#targets)

***

### extra

```ts
extra: Record<string, unknown>;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:112](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L112)

#### Inherited from

[`Vl53lxDriverMeasurement`](Vl53lxDriverMeasurement.md).[`extra`](Vl53lxDriverMeasurement.md#extra)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:66](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L66)

MCU uptime at the INT edge (stream) / read (poll).

***

### bins

```ts
bins: unknown;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L67)

***

### valid

```ts
valid: boolean;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:69](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L69)

`status === 0`. On histogram products prefer `plottable`.

***

### plottable

```ts
plottable: boolean;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L71)

The frame has a usable range (PLOTTABLE_STATUSES).

# Interface: Vl53lxOptions

Defined in: [src/sensors/vl53lx/vl53lx.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L164)

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

Defined in: [src/sensors/vl53lx/vl53lx.ts:166](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L166)

ULD sleep implementation (tests inject an instant one).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Interface: Vl53lxProductInfo

Defined in: [src/sensors/vl53lx/vl53lx.ts:143](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L143)

Everything about what is connected (`identifyProduct()`).

## Properties

### board

```ts
board: string;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:144](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L144)

***

### detected

```ts
detected: string | null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:145](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L145)

***

### product

```ts
product: string | null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:146](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L146)

***

### driver

```ts
driver: string | null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:147](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L147)

***

### driverClass

```ts
driverClass: string;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:148](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L148)

***

### kinds

```ts
kinds: string[];
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:149](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L149)

***

### modelId

```ts
modelId: number;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:150](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L150)

***

### modelIdOk

```ts
modelIdOk: boolean;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:152](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L152)

Cross-check only: L1CX/L1CB and L4CD/L4CX share their ids.

***

### supports

```ts
supports: ReadonlySet<string>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:153](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L153)

***

### modes

```ts
modes: readonly string[];
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:154](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L154)

***

### reachMm

```ts
reachMm: number | null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:155](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L155)

***

### driverReachMm

```ts
driverReachMm: number | null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:156](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L156)

***

### budgetMs

```ts
budgetMs: readonly [number, number];
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:157](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L157)

***

### budgetChoices

```ts
budgetChoices: number[];
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:158](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L158)

***

### histogram

```ts
histogram: boolean;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:159](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L159)

***

### maxKhz

```ts
maxKhz: number;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:160](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L160)

***

### caveat

```ts
caveat: string | null;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:161](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L161)

# Interface: Vl53lxTarget

Defined in: [src/sensors/vl53lx/uld/base.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L87)

One return of a multi-target frame. `minRangeMm`/`maxRangeMm` are the edges
of the target's own pulse (histogram parts); the others repeat `distanceMm`.

## Properties

### distanceMm

```ts
distanceMm: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:88](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L88)

***

### status

```ts
status: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:89](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L89)

***

### statusText

```ts
statusText: string;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:90](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L90)

***

### signalKcps

```ts
signalKcps: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:91](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L91)

***

### ambientKcps

```ts
ambientKcps: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L92)

***

### sigmaMm

```ts
sigmaMm: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L93)

***

### minRangeMm

```ts
minRangeMm: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L94)

***

### maxRangeMm

```ts
maxRangeMm: number;
```

Defined in: [src/sensors/vl53lx/uld/base.ts:95](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L95)

# Type Alias: Vl53lxClearStep

```ts
type Vl53lxClearStep = readonly [number, number];
```

Defined in: [src/protocol/vl53lx.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L44)

One interrupt-release step: write `value` to register `addr`.

# Variable: PLOTTABLE\_STATUSES

```ts
const PLOTTABLE_STATUSES: readonly number[];
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L55)

Statuses that mean "this distance is real". A histogram product reports 6
on the first frame of a stream and 11 on a merged pulse: use this, not
`status === 0`, as the validity test on histogram products.

# Variable: VL53L4\_XSHUT\_OFF

```ts
const VL53L4_XSHUT_OFF: 0 = 0;
```

Defined in: [src/protocol/vl53l4.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L27)

# Variable: VL53L4\_XSHUT\_ON

```ts
const VL53L4_XSHUT_ON: 1 = 1;
```

Defined in: [src/protocol/vl53l4.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L28)

# Variable: VL53L4\_XSHUT\_RESET

```ts
const VL53L4_XSHUT_RESET: 2 = 2;
```

Defined in: [src/protocol/vl53l4.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L30)

Blocking on the MCU (~3 ms); answered after the boot handshake.

# Variable: VL53LX\_CLASS\_BY\_PRODUCT

```ts
const VL53LX_CLASS_BY_PRODUCT: Readonly<Record<string, typeof Vl53lx>>;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:844](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L844)

Class per product (VL53L4CD on this firmware uses the generic class).

# Variable: VL53LX\_CLEAR\_STEPS\_MAX

```ts
const VL53LX_CLEAR_STEPS_MAX: 4 = 4;
```

Defined in: [src/protocol/vl53lx.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L39)

Interrupt-release steps a stream may carry (VL53_CLEAR_STEPS_WIRE_MAX).

# Variable: VL53LX\_DRIVER\_KINDS

```ts
const VL53LX_DRIVER_KINDS: readonly ["uld", "ulp", "histogram"] = registry.DRIVER_KINDS;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:868](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L868)

Shared barrel for the VL53L 1D-family TypeDoc entry points
(`entry-vl53l0x.ts` … `entry-vl53l4cx.ts`): the family class `Vl53lx`, the
measurement shape, the helpers and the bridge codecs, under the names
`src/index.ts` exports them with. Every product page carries this same
family surface after its own class.

Not part of the shipped package — a doc-generation helper only.

# Variable: VL53LX\_INFO\_SIZE

```ts
const VL53LX_INFO_SIZE: 23 = 23;
```

Defined in: [src/protocol/vl53lx.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L41)

RPT_VL53_INFO payload size (v2.00).

# Variable: VL53LX\_PRODUCTS

```ts
const VL53LX_PRODUCTS: readonly ["VL53L0X", "VL53L1CX", "VL53L1CB", "VL53L3CX", "VL53L4CD", "VL53L4CX"] = registry.PRODUCTS;
```

Defined in: [src/sensors/vl53lx/vl53lx.ts:867](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L867)

Shared barrel for the VL53L 1D-family TypeDoc entry points
(`entry-vl53l0x.ts` … `entry-vl53l4cx.ts`): the family class `Vl53lx`, the
measurement shape, the helpers and the bridge codecs, under the names
`src/index.ts` exports them with. Every product page carries this same
family surface after its own class.

Not part of the shipped package — a doc-generation helper only.

# Variable: WINDOW\_ABOVE

```ts
const WINDOW_ABOVE: 1 = 1;
```

Defined in: [src/sensors/vl53l4/uld.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L57)

# Variable: WINDOW\_BELOW

```ts
const WINDOW_BELOW: 0 = 0;
```

Defined in: [src/sensors/vl53l4/uld.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L56)

# Variable: WINDOW\_IN

```ts
const WINDOW_IN: 3 = 3;
```

Defined in: [src/sensors/vl53l4/uld.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L59)

# Variable: WINDOW\_OUT

```ts
const WINDOW_OUT: 2 = 2;
```

Defined in: [src/sensors/vl53l4/uld.ts:58](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L58)

