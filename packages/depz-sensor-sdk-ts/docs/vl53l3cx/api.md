# VL53L3CX (ToF) — API reference

`Vl53l3cx` fixes the product on the 1D-family class `Vl53lx`; the family
surface (`Vl53lx`, `Vl53lxMeasurement`, the helpers and the bridge
codecs) follows it and is identical on every VL53L0X / L1CX / L1CB /
L3CX / L4CX page. Discovery, device-base, transport and other
cross-sensor symbols shared by every sensor live in the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Enumerations

- [Vl53lxCmd](#enumeration-vl53lxcmd)
- [Vl53lxRpt](#enumeration-vl53lxrpt)

## Classes

- [Vl53lxError](#class-vl53lxerror)
- [Vl53lxProtocolError](#class-vl53lxprotocolerror)
- [Vl53lx](#class-vl53lx)
- [Vl53l3cx](#class-vl53l3cx)

## Interfaces

- [Vl53lxInfo](#interface-vl53lxinfo)
- [Vl53lxTarget](#interface-vl53lxtarget)
- [Vl53lxDriverMeasurement](#interface-vl53lxdrivermeasurement)
- [Vl53lxMeasurement](#interface-vl53lxmeasurement)
- [HistogramDriverApi](#interface-histogramdriverapi)
- [Vl53lxProductInfo](#interface-vl53lxproductinfo)
- [Vl53lxOptions](#interface-vl53lxoptions)
- [Vl53lxConfigureOptions](#interface-vl53lxconfigureoptions)

## Type Aliases

- [Vl53lxClearStep](#type-alias-vl53lxclearstep)

## Variables

- [VL53L4\_XSHUT\_OFF](#variable-vl53l4_xshut_off)
- [VL53L4\_XSHUT\_ON](#variable-vl53l4_xshut_on)
- [VL53L4\_XSHUT\_RESET](#variable-vl53l4_xshut_reset)
- [VL53LX\_CLEAR\_STEPS\_MAX](#variable-vl53lx_clear_steps_max)
- [VL53LX\_INFO\_SIZE](#variable-vl53lx_info_size)
- [WINDOW\_BELOW](#variable-window_below)
- [WINDOW\_ABOVE](#variable-window_above)
- [WINDOW\_OUT](#variable-window_out)
- [WINDOW\_IN](#variable-window_in)
- [PLOTTABLE\_STATUSES](#variable-plottable_statuses)
- [VL53LX\_CLASS\_BY\_PRODUCT](#variable-vl53lx_class_by_product)
- [VL53LX\_PRODUCTS](#variable-vl53lx_products)
- [VL53LX\_DRIVER\_KINDS](#variable-vl53lx_driver_kinds)

## Functions

- [packVl53lxStartStream](#function-packvl53lxstartstream)
- [packVl53lxSetAddrWidth](#function-packvl53lxsetaddrwidth)
- [unpackVl53lxInfo](#function-unpackvl53lxinfo)
- [plotDistances](#function-plotdistances)
- [primaryTarget](#function-primarytarget)
- [resolveVl53lxClass](#function-resolvevl53lxclass)

# Class: Vl53l3cx

Defined in: [sensors/vl53lx/vl53lx.ts:824](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L824)

VL53L3CX (3 m): ST's ULP driver (single target) or the histogram driver.

## Extends

- [`Vl53lx`](#class-vl53lx)

## Constructors

### Constructor

```ts
new Vl53l3cx(transport, opts?): Vl53l3cx;
```

Defined in: [sensors/vl53lx/vl53lx.ts:340](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L340)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | [`Vl53lxOptions`](#interface-vl53lxoptions) |

#### Returns

`Vl53l3cx`

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`constructor`](#constructor-1)

## Properties

### NO\_MATCH

```ts
readonly static NO_MATCH: typeof NO_MATCH;
```

Defined in: [device/device.ts:177](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L177)

What a `Matcher` returns to decline a packet (the Python reference's
`DepzDevice.NO_MATCH`).

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`NO_MATCH`](#no_match-1)

***

### timeoutMs

```ts
timeoutMs: number;
```

Defined in: [device/device.ts:179](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L179)

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`timeoutMs`](#timeoutms-1)

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

[`Vl53lx`](#class-vl53lx).[`events`](#events-1)

***

### measurements

```ts
readonly measurements: Stream<Vl53lxMeasurement>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:219](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L219)

The streamed measurements (contract 07 §3): `on(cb)` subscribes,
`subscribe(maxsize)` iterates, `next(timeoutMs)` waits for one;
`droppedCounts` and `parseErrors` (stream reports the driver could not
decode, or shorter than their header — counted before `init()` too) are
its counters.

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`measurements`](#measurements-1)

***

### driverInst

```ts
protected driverInst: SensorDriver | null = null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:221](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L221)

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`driverInst`](#driverinst-1)

***

### productName

```ts
protected productName: string | null = null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:222](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L222)

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`productName`](#productname-1)

***

### driverKindName

```ts
protected driverKindName: string | null = null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:223](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L223)

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`driverKindName`](#driverkindname-1)

***

### caveat

```ts
protected caveat: string | null = null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:224](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L224)

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`caveat`](#caveat-1)

***

### rangingFlag

```ts
protected rangingFlag: boolean = false;
```

Defined in: [sensors/vl53lx/vl53lx.ts:226](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L226)

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`rangingFlag`](#rangingflag-1)

***

### PRODUCT

```ts
readonly static PRODUCT: string | null = "VL53L3CX";
```

Defined in: [sensors/vl53lx/vl53lx.ts:825](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L825)

The product this class drives; null = read it from the device name.

#### Overrides

[`Vl53lx`](#class-vl53lx).[`PRODUCT`](#product-2)

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

[`Vl53lx`](#class-vl53lx).[`stats`](#stats-1)

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

[`Vl53lx`](#class-vl53lx).[`lost`](#lost-1)

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

[`Vl53lx`](#class-vl53lx).[`timeSync`](#timesync-1)

***

### product

#### Get Signature

```ts
get product(): string | null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:367](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L367)

The product init() bound (null before init).

##### Returns

`string` \| `null`

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`product`](#product-3)

***

### driverKind

#### Get Signature

```ts
get driverKind(): string | null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:371](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L371)

##### Returns

`string` \| `null`

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`driverKind`](#driverkind-1)

***

### driver

#### Get Signature

```ts
get driver(): SensorDriver;
```

Defined in: [sensors/vl53lx/vl53lx.ts:381](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L381)

The ULD port in use — escape hatch for product-specific calls.
Unstable — not covered by the 1.0 freeze: the driver class and its
methods follow the ST ULD release the SDK ports, and the class is not
exported from the package root.

##### Returns

`SensorDriver`

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`driver`](#driver-1)

***

### initialized

#### Get Signature

```ts
get initialized(): boolean;
```

Defined in: [sensors/vl53lx/vl53lx.ts:386](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L386)

##### Returns

`boolean`

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`initialized`](#initialized-1)

***

### modes

#### Get Signature

```ts
get modes(): readonly string[];
```

Defined in: [sensors/vl53lx/vl53lx.ts:493](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L493)

Named ranging modes, first = what init leaves; [] if none.

##### Returns

readonly `string`[]

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`modes`](#modes-1)

***

### ranging

#### Get Signature

```ts
get ranging(): boolean;
```

Defined in: [sensors/vl53lx/vl53lx.ts:719](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L719)

##### Returns

`boolean`

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`ranging`](#ranging-1)

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

[`Vl53lx`](#class-vl53lx).[`close`](#close-1)

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

[`Vl53lx`](#class-vl53lx).[`onTeardown`](#onteardown-1)

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

[`Vl53lx`](#class-vl53lx).[`requireOpen`](#requireopen-1)

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

[`Vl53lx`](#class-vl53lx).[`request`](#request-1)

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

[`Vl53lx`](#class-vl53lx).[`expectReport`](#expectreport-1)

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

[`Vl53lx`](#class-vl53lx).[`expectText`](#expecttext-1)

***

### getDeviceName()

```ts
getDeviceName(): Promise<string>;
```

Defined in: [device/device.ts:567](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L567)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`getDeviceName`](#getdevicename-1)

***

### getSoftwareName()

```ts
getSoftwareName(): Promise<string>;
```

Defined in: [device/device.ts:573](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L573)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`getSoftwareName`](#getsoftwarename-1)

***

### getSerialNumber()

```ts
getSerialNumber(): Promise<string>;
```

Defined in: [device/device.ts:579](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L579)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`getSerialNumber`](#getserialnumber-1)

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

[`Vl53lx`](#class-vl53lx).[`identify`](#identify-1)

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

[`Vl53lx`](#class-vl53lx).[`readMcuTemperature`](#readmcutemperature-1)

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

[`Vl53lx`](#class-vl53lx).[`syncTime`](#synctime-1)

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

[`Vl53lx`](#class-vl53lx).[`toHostTimeUs`](#tohosttimeus-1)

***

### getReportPayloadCrc()

```ts
getReportPayloadCrc(): Promise<CrcType>;
```

Defined in: [device/device.ts:626](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L626)

#### Returns

`Promise`\<`CrcType`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`getReportPayloadCrc`](#getreportpayloadcrc-1)

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

[`Vl53lx`](#class-vl53lx).[`setReportPayloadCrc`](#setreportpayloadcrc-1)

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

[`Vl53lx`](#class-vl53lx).[`getSyncPin`](#getsyncpin-1)

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

[`Vl53lx`](#class-vl53lx).[`setSyncPin`](#setsyncpin-1)

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

[`Vl53lx`](#class-vl53lx).[`reset`](#reset-1)

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

[`Vl53lx`](#class-vl53lx).[`enterBootloaderMode`](#enterbootloadermode-1)

***

### boardName()

```ts
boardName(): Promise<string>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:356](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L356)

The board's device name (bootloader metablock), e.g.
`DEPZ ToF Sensor VL53L4CX USB v2.1 TEXAMPLE03`. Read once, then cached.

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`boardName`](#boardname-1)

***

### detected()

```ts
detected(): Promise<string | null>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:362](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L362)

The product the device name carries, or null on an unstamped board.

#### Returns

`Promise`\<`string` \| `null`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`detected`](#detected-1)

***

### driverKinds()

```ts
driverKinds(product?): Promise<string[]>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:391](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L391)

The driver kinds a product has (default: this board's).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `product?` | `string` |

#### Returns

`Promise`\<`string`[]\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`driverKinds`](#driverkinds-1)

***

### init()

```ts
init(driver?, opts?): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:406](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L406)

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

[`Vl53lx`](#class-vl53lx).[`init`](#init-1)

***

### identifyProduct()

```ts
identifyProduct(): Promise<Vl53lxProductInfo>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:431](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L431)

Everything about what is connected (needs init()).

#### Returns

`Promise`\<[`Vl53lxProductInfo`](#interface-vl53lxproductinfo)\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`identifyProduct`](#identifyproduct-1)

***

### notes()

```ts
notes(): Promise<string[]>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:461](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L461)

The lines a UI should show about the chosen pair: product named by hand,
a borrowed driver, a pair that reaches less than the board is rated for,
the driver's caveat.

#### Returns

`Promise`\<`string`[]\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`notes`](#notes-1)

***

### supports()

```ts
supports(group): boolean;
```

Defined in: [sensors/vl53lx/vl53lx.ts:488](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L488)

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

[`Vl53lx`](#class-vl53lx).[`supports`](#supports-1)

***

### xshut()

```ts
xshut(action): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:504](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L504)

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

[`Vl53lx`](#class-vl53lx).[`xshut`](#xshut-1)

***

### bridgeInfo()

```ts
bridgeInfo(): Promise<Vl53lxInfo>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:511](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L511)

RPT_VL53_INFO — the bridge's own counters; safe while streaming.

#### Returns

`Promise`\<[`Vl53lxInfo`](#interface-vl53lxinfo)\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`bridgeInfo`](#bridgeinfo-1)

***

### configure()

```ts
configure(opts?): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:526](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L526)

Re-initialise the sensor and apply a ranging configuration. The re-init
is deliberate (the only way to know what the configuration registers
hold). `mode` goes on before the budget; `offsetMm` / `xtalkKcps`
re-apply a stored calibration; `signalKcps` goes on last. An unsupported
group is refused before the re-init.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `opts` | [`Vl53lxConfigureOptions`](#interface-vl53lxconfigureoptions) |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`configure`](#configure-1)

***

### getRangeTiming()

```ts
getRangeTiming(): Promise<[number, number]>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:542](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L542)

→ [timingBudgetMs, interMeasurementMs]; 0 for the period = continuous.

#### Returns

`Promise`\<\[`number`, `number`\]\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`getRangeTiming`](#getrangetiming-1)

***

### getMode()

```ts
getMode(): Promise<string | null>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:547](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L547)

The ranging mode in use, or null on a product without modes.

#### Returns

`Promise`\<`string` \| `null`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`getMode`](#getmode-1)

***

### budgetChoices()

```ts
budgetChoices(): number[];
```

Defined in: [sensors/vl53lx/vl53lx.ts:552](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L552)

The only budgets accepted right now (ascending), or [] for any in range.

#### Returns

`number`[]

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`budgetChoices`](#budgetchoices-1)

***

### snapBudget()

```ts
snapBudget(budgetMs): number;
```

Defined in: [sensors/vl53lx/vl53lx.ts:557](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L557)

The nearest budget this product will actually accept.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `budgetMs` | `number` |

#### Returns

`number`

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`snapBudget`](#snapbudget-1)

***

### getOffsetMm()

```ts
getOffsetMm(): Promise<number>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:571](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L571)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`getOffsetMm`](#getoffsetmm-1)

***

### setOffsetMm()

```ts
setOffsetMm(offsetMm): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:575](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L575)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `offsetMm` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`setOffsetMm`](#setoffsetmm-1)

***

### getXtalkKcps()

```ts
getXtalkKcps(): Promise<number>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:580](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L580)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`getXtalkKcps`](#getxtalkkcps-1)

***

### setXtalkKcps()

```ts
setXtalkKcps(xtalkKcps): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:584](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L584)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `xtalkKcps` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`setXtalkKcps`](#setxtalkkcps-1)

***

### calibrateOffset()

```ts
calibrateOffset(targetDistMm, nbSamples?): Promise<number>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:593](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L593)

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

[`Vl53lx`](#class-vl53lx).[`calibrateOffset`](#calibrateoffset-1)

***

### calibrateXtalk()

```ts
calibrateXtalk(targetDistMm, nbSamples?): Promise<number>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:599](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L599)

Crosstalk calibration; returns the xtalk now programmed (kcps).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetDistMm` | `number` |
| `nbSamples?` | `number` |

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`calibrateXtalk`](#calibratextalk-1)

***

### getDetectionThresholds()

```ts
getDetectionThresholds(): Promise<[number, number, number]>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:605](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L605)

→ [distanceLowMm, distanceHighMm, window].

#### Returns

`Promise`\<\[`number`, `number`, `number`\]\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`getDetectionThresholds`](#getdetectionthresholds-1)

***

### setDetectionThresholds()

```ts
setDetectionThresholds(
   distanceLowMm, 
   distanceHighMm, 
window): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:614](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L614)

Arm the distance-window interrupt; stays armed until the next init/configure.
The distances are u16 registers, the window a u8 one: a value outside is
refused before anything is written, never masked (contract 07 §3).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `distanceLowMm` | `number` |
| `distanceHighMm` | `number` |
| `window` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`setDetectionThresholds`](#setdetectionthresholds-1)

***

### getSignalThresholdKcps()

```ts
getSignalThresholdKcps(): Promise<number>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:626](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L626)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`getSignalThresholdKcps`](#getsignalthresholdkcps-1)

***

### setSignalThresholdKcps()

```ts
setSignalThresholdKcps(signalKcps): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:630](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L630)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `signalKcps` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`setSignalThresholdKcps`](#setsignalthresholdkcps-1)

***

### getSigmaThresholdMm()

```ts
getSigmaThresholdMm(): Promise<number>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:635](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L635)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`getSigmaThresholdMm`](#getsigmathresholdmm-1)

***

### setSigmaThresholdMm()

```ts
setSigmaThresholdMm(sigmaMm): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:639](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L639)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `sigmaMm` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`setSigmaThresholdMm`](#setsigmathresholdmm-1)

***

### getRoi()

```ts
getRoi(): Promise<[number, number]>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:645](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L645)

→ [x, y] SPAD window size.

#### Returns

`Promise`\<\[`number`, `number`\]\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`getRoi`](#getroi-1)

***

### setRoi()

```ts
setRoi(x, y): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:649](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L649)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `x` | `number` |
| `y` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`setRoi`](#setroi-1)

***

### getRoiCenter()

```ts
getRoiCenter(): Promise<number>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:654](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L654)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`getRoiCenter`](#getroicenter-1)

***

### setRoiCenter()

```ts
setRoiCenter(centerSpad): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:662](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L662)

The ROI centre SPAD, a u8 register: a value outside 0..255 is refused
before anything is written, never masked (contract 07 §3).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `centerSpad` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`setRoiCenter`](#setroicenter-1)

***

### startTemperatureUpdate()

```ts
startTemperatureUpdate(): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:669](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L669)

Re-run VHV after an ambient change over 8 °C.

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`startTemperatureUpdate`](#starttemperatureupdate-1)

***

### performRefSpadManagement()

```ts
performRefSpadManagement(): Promise<[number, number]>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:675](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L675)

VL53L0X: re-measure the reference SPADs → [count, isAperture].

#### Returns

`Promise`\<\[`number`, `number`\]\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`performRefSpadManagement`](#performrefspadmanagement-1)

***

### startRanging()

```ts
startRanging(): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:687](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L687)

Start the sensor's ranging loop and arm the MCU stream: one
RPT_VL53_STREAM per INT edge, followed on the MCU by the driver's
interrupt-release writes.

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`startRanging`](#startranging-1)

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:708](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L708)

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`stopRanging`](#stopranging-1)

***

### measureOnce()

```ts
measureOnce(timeoutMs?): Promise<Vl53lxMeasurement>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:727](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L727)

Single poll-mode measurement: start ranging, wait for data-ready, read,
release the interrupt, stop. Rejects while the stream runs.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `1000` |

#### Returns

`Promise`\<[`Vl53lxMeasurement`](#interface-vl53lxmeasurement)\>

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`measureOnce`](#measureonce-1)

***

### need()

```ts
protected need(group): SensorDriver & DriverExtras;
```

Defined in: [sensors/vl53lx/vl53lx.ts:752](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L752)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `group` | `string` |

#### Returns

`SensorDriver` & `DriverExtras`

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`need`](#need-1)

***

### requireNotRanging()

```ts
protected requireNotRanging(): void;
```

Defined in: [sensors/vl53lx/vl53lx.ts:763](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L763)

#### Returns

`void`

#### Inherited from

[`Vl53lx`](#class-vl53lx).[`requireNotRanging`](#requirenotranging-1)

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [sensors/vl53lx/vl53lx.ts:768](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L768)

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

[`Vl53lx`](#class-vl53lx).[`handleReport`](#handlereport-1)

# Class: Vl53lx

Defined in: [sensors/vl53lx/vl53lx.ts:208](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L208)

A board of the VL53L 1D family (firmware `APP_VL53L0_4`).

`init()` binds the (product, driver kind) pair and runs the ULD's
sensorInit; `configure()` re-initialises and applies the ranging
configuration — call it before every run. Then `startRanging()` arms the MCU
stream and measurements arrive on `measurements` (contract 07 §3).
Configuration must not change while ranging.

This generic class serves any product (read from the board's device name,
or `product` at init); the per-product subclasses (`Vl53l0x`...) fix it.

## Extends

- `DepzDevice`

## Extended by

- [`Vl53l3cx`](#class-vl53l3cx)

## Constructors

### Constructor

```ts
new Vl53lx(transport, opts?): Vl53lx;
```

Defined in: [sensors/vl53lx/vl53lx.ts:340](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L340)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | [`Vl53lxOptions`](#interface-vl53lxoptions) |

#### Returns

`Vl53lx`

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

### PRODUCT

```ts
readonly static PRODUCT: string | null = null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L210)

The product this class drives; null = read it from the device name.

***

### measurements

```ts
readonly measurements: Stream<Vl53lxMeasurement>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:219](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L219)

The streamed measurements (contract 07 §3): `on(cb)` subscribes,
`subscribe(maxsize)` iterates, `next(timeoutMs)` waits for one;
`droppedCounts` and `parseErrors` (stream reports the driver could not
decode, or shorter than their header — counted before `init()` too) are
its counters.

***

### driverInst

```ts
protected driverInst: SensorDriver | null = null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:221](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L221)

***

### productName

```ts
protected productName: string | null = null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:222](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L222)

***

### driverKindName

```ts
protected driverKindName: string | null = null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:223](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L223)

***

### caveat

```ts
protected caveat: string | null = null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:224](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L224)

***

### rangingFlag

```ts
protected rangingFlag: boolean = false;
```

Defined in: [sensors/vl53lx/vl53lx.ts:226](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L226)

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

### product

#### Get Signature

```ts
get product(): string | null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:367](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L367)

The product init() bound (null before init).

##### Returns

`string` \| `null`

***

### driverKind

#### Get Signature

```ts
get driverKind(): string | null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:371](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L371)

##### Returns

`string` \| `null`

***

### driver

#### Get Signature

```ts
get driver(): SensorDriver;
```

Defined in: [sensors/vl53lx/vl53lx.ts:381](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L381)

The ULD port in use — escape hatch for product-specific calls.
Unstable — not covered by the 1.0 freeze: the driver class and its
methods follow the ST ULD release the SDK ports, and the class is not
exported from the package root.

##### Returns

`SensorDriver`

***

### initialized

#### Get Signature

```ts
get initialized(): boolean;
```

Defined in: [sensors/vl53lx/vl53lx.ts:386](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L386)

##### Returns

`boolean`

***

### modes

#### Get Signature

```ts
get modes(): readonly string[];
```

Defined in: [sensors/vl53lx/vl53lx.ts:493](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L493)

Named ranging modes, first = what init leaves; [] if none.

##### Returns

readonly `string`[]

***

### ranging

#### Get Signature

```ts
get ranging(): boolean;
```

Defined in: [sensors/vl53lx/vl53lx.ts:719](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L719)

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

### boardName()

```ts
boardName(): Promise<string>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:356](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L356)

The board's device name (bootloader metablock), e.g.
`DEPZ ToF Sensor VL53L4CX USB v2.1 TEXAMPLE03`. Read once, then cached.

#### Returns

`Promise`\<`string`\>

***

### detected()

```ts
detected(): Promise<string | null>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:362](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L362)

The product the device name carries, or null on an unstamped board.

#### Returns

`Promise`\<`string` \| `null`\>

***

### driverKinds()

```ts
driverKinds(product?): Promise<string[]>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:391](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L391)

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

Defined in: [sensors/vl53lx/vl53lx.ts:406](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L406)

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

Defined in: [sensors/vl53lx/vl53lx.ts:431](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L431)

Everything about what is connected (needs init()).

#### Returns

`Promise`\<[`Vl53lxProductInfo`](#interface-vl53lxproductinfo)\>

***

### notes()

```ts
notes(): Promise<string[]>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:461](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L461)

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

Defined in: [sensors/vl53lx/vl53lx.ts:488](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L488)

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

Defined in: [sensors/vl53lx/vl53lx.ts:504](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L504)

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

Defined in: [sensors/vl53lx/vl53lx.ts:511](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L511)

RPT_VL53_INFO — the bridge's own counters; safe while streaming.

#### Returns

`Promise`\<[`Vl53lxInfo`](#interface-vl53lxinfo)\>

***

### configure()

```ts
configure(opts?): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:526](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L526)

Re-initialise the sensor and apply a ranging configuration. The re-init
is deliberate (the only way to know what the configuration registers
hold). `mode` goes on before the budget; `offsetMm` / `xtalkKcps`
re-apply a stored calibration; `signalKcps` goes on last. An unsupported
group is refused before the re-init.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `opts` | [`Vl53lxConfigureOptions`](#interface-vl53lxconfigureoptions) |

#### Returns

`Promise`\<`void`\>

***

### getRangeTiming()

```ts
getRangeTiming(): Promise<[number, number]>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:542](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L542)

→ [timingBudgetMs, interMeasurementMs]; 0 for the period = continuous.

#### Returns

`Promise`\<\[`number`, `number`\]\>

***

### getMode()

```ts
getMode(): Promise<string | null>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:547](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L547)

The ranging mode in use, or null on a product without modes.

#### Returns

`Promise`\<`string` \| `null`\>

***

### budgetChoices()

```ts
budgetChoices(): number[];
```

Defined in: [sensors/vl53lx/vl53lx.ts:552](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L552)

The only budgets accepted right now (ascending), or [] for any in range.

#### Returns

`number`[]

***

### snapBudget()

```ts
snapBudget(budgetMs): number;
```

Defined in: [sensors/vl53lx/vl53lx.ts:557](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L557)

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

Defined in: [sensors/vl53lx/vl53lx.ts:571](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L571)

#### Returns

`Promise`\<`number`\>

***

### setOffsetMm()

```ts
setOffsetMm(offsetMm): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:575](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L575)

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

Defined in: [sensors/vl53lx/vl53lx.ts:580](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L580)

#### Returns

`Promise`\<`number`\>

***

### setXtalkKcps()

```ts
setXtalkKcps(xtalkKcps): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:584](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L584)

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

Defined in: [sensors/vl53lx/vl53lx.ts:593](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L593)

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

Defined in: [sensors/vl53lx/vl53lx.ts:599](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L599)

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

Defined in: [sensors/vl53lx/vl53lx.ts:605](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L605)

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

Defined in: [sensors/vl53lx/vl53lx.ts:614](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L614)

Arm the distance-window interrupt; stays armed until the next init/configure.
The distances are u16 registers, the window a u8 one: a value outside is
refused before anything is written, never masked (contract 07 §3).

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

Defined in: [sensors/vl53lx/vl53lx.ts:626](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L626)

#### Returns

`Promise`\<`number`\>

***

### setSignalThresholdKcps()

```ts
setSignalThresholdKcps(signalKcps): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:630](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L630)

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

Defined in: [sensors/vl53lx/vl53lx.ts:635](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L635)

#### Returns

`Promise`\<`number`\>

***

### setSigmaThresholdMm()

```ts
setSigmaThresholdMm(sigmaMm): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:639](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L639)

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

Defined in: [sensors/vl53lx/vl53lx.ts:645](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L645)

→ [x, y] SPAD window size.

#### Returns

`Promise`\<\[`number`, `number`\]\>

***

### setRoi()

```ts
setRoi(x, y): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:649](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L649)

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

Defined in: [sensors/vl53lx/vl53lx.ts:654](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L654)

#### Returns

`Promise`\<`number`\>

***

### setRoiCenter()

```ts
setRoiCenter(centerSpad): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:662](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L662)

The ROI centre SPAD, a u8 register: a value outside 0..255 is refused
before anything is written, never masked (contract 07 §3).

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

Defined in: [sensors/vl53lx/vl53lx.ts:669](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L669)

Re-run VHV after an ambient change over 8 °C.

#### Returns

`Promise`\<`void`\>

***

### performRefSpadManagement()

```ts
performRefSpadManagement(): Promise<[number, number]>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:675](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L675)

VL53L0X: re-measure the reference SPADs → [count, isAperture].

#### Returns

`Promise`\<\[`number`, `number`\]\>

***

### startRanging()

```ts
startRanging(): Promise<void>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:687](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L687)

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

Defined in: [sensors/vl53lx/vl53lx.ts:708](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L708)

#### Returns

`Promise`\<`void`\>

***

### measureOnce()

```ts
measureOnce(timeoutMs?): Promise<Vl53lxMeasurement>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:727](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L727)

Single poll-mode measurement: start ranging, wait for data-ready, read,
release the interrupt, stop. Rejects while the stream runs.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `1000` |

#### Returns

`Promise`\<[`Vl53lxMeasurement`](#interface-vl53lxmeasurement)\>

***

### need()

```ts
protected need(group): SensorDriver & DriverExtras;
```

Defined in: [sensors/vl53lx/vl53lx.ts:752](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L752)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `group` | `string` |

#### Returns

`SensorDriver` & `DriverExtras`

***

### requireNotRanging()

```ts
protected requireNotRanging(): void;
```

Defined in: [sensors/vl53lx/vl53lx.ts:763](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L763)

#### Returns

`void`

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [sensors/vl53lx/vl53lx.ts:768](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L768)

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

Defined in: [sensors/vl53lx/uld/link.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/link.ts#L23)

A ULD-level failure: the sensor did not do what the driver needed (timeout
waiting for data-ready or boot, a calibration that failed...).

## Extends

- `DepzError`

## Constructors

### Constructor

```ts
new Vl53lxError(message?): Vl53Error;
```

Defined in: [errors.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L8)

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
DepzError.stackTraceLimit
```

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: ../node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

```ts
DepzError.cause
```

***

### name

```ts
name: string;
```

Defined in: ../node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

```ts
DepzError.name
```

***

### message

```ts
message: string;
```

Defined in: ../node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

```ts
DepzError.message
```

***

### stack?

```ts
optional stack?: string;
```

Defined in: ../node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

```ts
DepzError.stack
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
DepzError.captureStackTrace
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
DepzError.prepareStackTrace
```

# Class: Vl53lxProtocolError

Defined in: [sensors/vl53lx/uld/link.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/link.ts#L30)

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

Defined in: [errors.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L8)

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
DepzError.stackTraceLimit
```

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: ../node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

```ts
DepzError.cause
```

***

### name

```ts
name: string;
```

Defined in: ../node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

```ts
DepzError.name
```

***

### message

```ts
message: string;
```

Defined in: ../node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

```ts
DepzError.message
```

***

### stack?

```ts
optional stack?: string;
```

Defined in: ../node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

```ts
DepzError.stack
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
DepzError.captureStackTrace
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
DepzError.prepareStackTrace
```

# Enumeration: Vl53lxCmd

Defined in: [protocol/vl53lx.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L22)

## Enumeration Members

### ReadReg

```ts
ReadReg: 50;
```

Defined in: [protocol/vl53lx.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L23)

***

### WriteReg

```ts
WriteReg: 51;
```

Defined in: [protocol/vl53lx.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L24)

***

### Xshut

```ts
Xshut: 52;
```

Defined in: [protocol/vl53lx.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L25)

***

### StartStream

```ts
StartStream: 53;
```

Defined in: [protocol/vl53lx.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L26)

***

### StopStream

```ts
StopStream: 54;
```

Defined in: [protocol/vl53lx.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L27)

***

### GetInfo

```ts
GetInfo: 55;
```

Defined in: [protocol/vl53lx.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L28)

***

### SetI2cSpeed

```ts
SetI2cSpeed: 56;
```

Defined in: [protocol/vl53lx.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L29)

***

### SetAddrWidth

```ts
SetAddrWidth: 57;
```

Defined in: [protocol/vl53lx.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L30)

***

### ClearI2cErrors

```ts
ClearI2cErrors: 58;
```

Defined in: [protocol/vl53lx.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L31)

# Enumeration: Vl53lxRpt

Defined in: [protocol/vl53lx.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L34)

## Enumeration Members

### RegData

```ts
RegData: 145;
```

Defined in: [protocol/vl53lx.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L35)

***

### Info

```ts
Info: 146;
```

Defined in: [protocol/vl53lx.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L36)

***

### Stream

```ts
Stream: 147;
```

Defined in: [protocol/vl53lx.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L37)

# Function: packVl53lxSetAddrWidth()

```ts
function packVl53lxSetAddrWidth(width): Uint8Array;
```

Defined in: [protocol/vl53lx.ts:75](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L75)

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

Defined in: [protocol/vl53lx.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L52)

vl53_start_stream_t: block, flags, then `clear` — the (addr, value) writes
the bridge plays after every block read (0..4 steps). 6+3n bytes.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `addr` | `number` | `undefined` |
| `length` | `number` | `undefined` |
| `clear` | readonly [`Vl53lxClearStep`](#type-alias-vl53lxclearstep)[] | `[]` |
| `flags` | `number` | `0` |

## Returns

`Uint8Array`

# Function: plotDistances()

```ts
function plotDistances(m): number[];
```

Defined in: [sensors/vl53lx/vl53lx.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L98)

The distances a chart should draw for one measurement: the plottable
targets in driver order, or the single distance on a light driver.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `m` | [`Vl53lxDriverMeasurement`](#interface-vl53lxdrivermeasurement) |

## Returns

`number`[]

# Function: primaryTarget()

```ts
function primaryTarget(m): Vl53lxTarget | null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:106](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L106)

The first plottable target, or null when the frame produced nothing usable.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `m` | [`Vl53lxDriverMeasurement`](#interface-vl53lxdrivermeasurement) |

## Returns

[`Vl53lxTarget`](#interface-vl53lxtarget) \| `null`

# Function: resolveVl53lxClass()

```ts
function resolveVl53lxClass(usbModel, deviceName): typeof Vl53lx;
```

Defined in: [sensors/vl53lx/vl53lx.ts:849](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L849)

Pick the 1D-family class (contract 12 §1): the production PID model, then
the product the device name carries, then the generic class.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `usbModel` | `string` \| `null` \| `undefined` |
| `deviceName` | `string` \| `null` \| `undefined` |

## Returns

*typeof* [`Vl53lx`](#class-vl53lx)

# Function: unpackVl53lxInfo()

```ts
function unpackVl53lxInfo(payload): Vl53lxInfo;
```

Defined in: [protocol/vl53lx.ts:106](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L106)

`<IIIBBBHBBI`; rejects a payload shorter than 23 bytes.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Vl53lxInfo`](#interface-vl53lxinfo)

# Interface: HistogramDriverApi

Defined in: [sensors/vl53lx/vl53lx.ts:117](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L117)

What the device class needs from the histogram (Bare) driver beyond
`SensorDriver`: `binData(raw)` steps the frame-pair state and returns the
bin object, `toMeasurement(bins)` finds the targets. Both run exactly once
per frame.

## Methods

### binData()

```ts
binData(raw): unknown;
```

Defined in: [sensors/vl53lx/vl53lx.ts:118](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L118)

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

Defined in: [sensors/vl53lx/vl53lx.ts:119](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L119)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `bins` | `unknown` |

#### Returns

[`Vl53lxDriverMeasurement`](#interface-vl53lxdrivermeasurement)

# Interface: Vl53lxConfigureOptions

Defined in: [sensors/vl53lx/vl53lx.ts:171](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L171)

## Properties

### budgetMs?

```ts
optional budgetMs?: number;
```

Defined in: [sensors/vl53lx/vl53lx.ts:173](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L173)

Timing budget, ms (see budgetChoices()). Default 50.

***

### interMs?

```ts
optional interMs?: number;
```

Defined in: [sensors/vl53lx/vl53lx.ts:175](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L175)

0 = continuous; otherwise the period between measurements (> budget).

***

### mode?

```ts
optional mode?: string | null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:177](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L177)

One of `modes`, applied before the budget.

***

### offsetMm?

```ts
optional offsetMm?: number | null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:179](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L179)

Re-apply a stored offset calibration.

***

### xtalkKcps?

```ts
optional xtalkKcps?: number | null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:181](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L181)

Re-apply a stored crosstalk calibration.

***

### signalKcps?

```ts
optional signalKcps?: number | null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L189)

Replace the blob's signal threshold, kcps. The re-init puts it back to
the default, so a lowered one goes here, not in a setSignalThresholdKcps()
before configure: frames past the default threshold come back status 2
with the distance right (L1 long at ~4 m, a light driver borrowed onto a
die without the lens it was tuned for).

# Interface: Vl53lxDriverMeasurement

Defined in: [sensors/vl53lx/uld/base.ts:103](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L103)

One range result, the same shape for every sensor of the family. On the
histogram driver `targets` holds every return strongest-first and the
top-level fields repeat `targets[0]`; on the light drivers it stays empty.

## Extended by

- [`Vl53lxMeasurement`](#interface-vl53lxmeasurement)

## Properties

### distanceMm

```ts
distanceMm: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:104](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L104)

***

### status

```ts
status: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L105)

***

### statusText

```ts
statusText: string;
```

Defined in: [sensors/vl53lx/uld/base.ts:106](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L106)

***

### signalKcps

```ts
signalKcps: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:107](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L107)

***

### ambientKcps

```ts
ambientKcps: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:108](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L108)

***

### sigmaMm

```ts
sigmaMm: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:109](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L109)

***

### spads

```ts
spads: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:110](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L110)

***

### targets

```ts
targets: Vl53lxTarget[];
```

Defined in: [sensors/vl53lx/uld/base.ts:111](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L111)

***

### extra

```ts
extra: Record<string, unknown>;
```

Defined in: [sensors/vl53lx/uld/base.ts:112](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L112)

# Interface: Vl53lxInfo

Defined in: [protocol/vl53lx.ts:91](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L91)

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

Defined in: [protocol/vl53lx.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L92)

***

### slotsSkipped

```ts
slotsSkipped: number;
```

Defined in: [protocol/vl53lx.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L93)

***

### i2cErrors

```ts
i2cErrors: number;
```

Defined in: [protocol/vl53lx.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L94)

***

### lastI2cError

```ts
lastI2cError: number;
```

Defined in: [protocol/vl53lx.ts:96](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L96)

0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR.

***

### xshutLevel

```ts
xshutLevel: number;
```

Defined in: [protocol/vl53lx.ts:97](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L97)

***

### intLevel

```ts
intLevel: number;
```

Defined in: [protocol/vl53lx.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L98)

***

### i2cKhz

```ts
i2cKhz: number;
```

Defined in: [protocol/vl53lx.ts:99](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L99)

***

### addrWidth

```ts
addrWidth: number;
```

Defined in: [protocol/vl53lx.ts:100](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L100)

***

### nClear

```ts
nClear: number;
```

Defined in: [protocol/vl53lx.ts:101](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L101)

***

### framesDropped

```ts
framesDropped: number;
```

Defined in: [protocol/vl53lx.ts:102](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L102)

# Interface: Vl53lxMeasurement

Defined in: [sensors/vl53lx/vl53lx.ts:66](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L66)

One ranging result, the same shape for every product of the family. On
the histogram driver `targets` holds every return strongest-first and the
top-level fields repeat `targets[0]`; `bins` carries the raw histogram
(the driver's bin object). On the light drivers `targets` is empty and
`bins` is null. `extra` is driver-specific — show it, don't branch on it.

## Extends

- [`Vl53lxDriverMeasurement`](#interface-vl53lxdrivermeasurement)

## Properties

### distanceMm

```ts
distanceMm: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:104](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L104)

#### Inherited from

[`Vl53lxDriverMeasurement`](#interface-vl53lxdrivermeasurement).[`distanceMm`](#distancemm)

***

### status

```ts
status: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L105)

#### Inherited from

[`Vl53lxDriverMeasurement`](#interface-vl53lxdrivermeasurement).[`status`](#status)

***

### statusText

```ts
statusText: string;
```

Defined in: [sensors/vl53lx/uld/base.ts:106](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L106)

#### Inherited from

[`Vl53lxDriverMeasurement`](#interface-vl53lxdrivermeasurement).[`statusText`](#statustext)

***

### signalKcps

```ts
signalKcps: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:107](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L107)

#### Inherited from

[`Vl53lxDriverMeasurement`](#interface-vl53lxdrivermeasurement).[`signalKcps`](#signalkcps-1)

***

### ambientKcps

```ts
ambientKcps: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:108](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L108)

#### Inherited from

[`Vl53lxDriverMeasurement`](#interface-vl53lxdrivermeasurement).[`ambientKcps`](#ambientkcps)

***

### sigmaMm

```ts
sigmaMm: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:109](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L109)

#### Inherited from

[`Vl53lxDriverMeasurement`](#interface-vl53lxdrivermeasurement).[`sigmaMm`](#sigmamm)

***

### spads

```ts
spads: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:110](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L110)

#### Inherited from

[`Vl53lxDriverMeasurement`](#interface-vl53lxdrivermeasurement).[`spads`](#spads)

***

### targets

```ts
targets: Vl53lxTarget[];
```

Defined in: [sensors/vl53lx/uld/base.ts:111](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L111)

#### Inherited from

[`Vl53lxDriverMeasurement`](#interface-vl53lxdrivermeasurement).[`targets`](#targets)

***

### extra

```ts
extra: Record<string, unknown>;
```

Defined in: [sensors/vl53lx/uld/base.ts:112](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L112)

#### Inherited from

[`Vl53lxDriverMeasurement`](#interface-vl53lxdrivermeasurement).[`extra`](#extra)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/vl53lx/vl53lx.ts:68](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L68)

MCU uptime at the INT edge (stream) / read (poll).

***

### bins

```ts
bins: unknown;
```

Defined in: [sensors/vl53lx/vl53lx.ts:69](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L69)

***

### valid

```ts
valid: boolean;
```

Defined in: [sensors/vl53lx/vl53lx.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L71)

`status === 0`. On histogram products prefer `plottable`.

***

### plottable

```ts
plottable: boolean;
```

Defined in: [sensors/vl53lx/vl53lx.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L73)

The frame has a usable range (PLOTTABLE_STATUSES).

# Interface: Vl53lxOptions

Defined in: [sensors/vl53lx/vl53lx.ts:166](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L166)

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

Defined in: [sensors/vl53lx/vl53lx.ts:168](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L168)

ULD sleep implementation (tests inject an instant one).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Interface: Vl53lxProductInfo

Defined in: [sensors/vl53lx/vl53lx.ts:145](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L145)

Everything about what is connected (`identifyProduct()`).

## Properties

### board

```ts
board: string;
```

Defined in: [sensors/vl53lx/vl53lx.ts:146](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L146)

***

### detected

```ts
detected: string | null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:147](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L147)

***

### product

```ts
product: string | null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:148](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L148)

***

### driver

```ts
driver: string | null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:149](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L149)

***

### driverClass

```ts
driverClass: string;
```

Defined in: [sensors/vl53lx/vl53lx.ts:150](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L150)

***

### kinds

```ts
kinds: string[];
```

Defined in: [sensors/vl53lx/vl53lx.ts:151](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L151)

***

### modelId

```ts
modelId: number;
```

Defined in: [sensors/vl53lx/vl53lx.ts:152](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L152)

***

### modelIdOk

```ts
modelIdOk: boolean;
```

Defined in: [sensors/vl53lx/vl53lx.ts:154](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L154)

Cross-check only: L1CX/L1CB and L4CD/L4CX share their ids.

***

### supports

```ts
supports: ReadonlySet<string>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:155](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L155)

***

### modes

```ts
modes: readonly string[];
```

Defined in: [sensors/vl53lx/vl53lx.ts:156](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L156)

***

### reachMm

```ts
reachMm: number | null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:157](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L157)

***

### driverReachMm

```ts
driverReachMm: number | null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:158](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L158)

***

### budgetMs

```ts
budgetMs: readonly [number, number];
```

Defined in: [sensors/vl53lx/vl53lx.ts:159](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L159)

***

### budgetChoices

```ts
budgetChoices: number[];
```

Defined in: [sensors/vl53lx/vl53lx.ts:160](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L160)

***

### histogram

```ts
histogram: boolean;
```

Defined in: [sensors/vl53lx/vl53lx.ts:161](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L161)

***

### maxKhz

```ts
maxKhz: number;
```

Defined in: [sensors/vl53lx/vl53lx.ts:162](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L162)

***

### caveat

```ts
caveat: string | null;
```

Defined in: [sensors/vl53lx/vl53lx.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L163)

# Interface: Vl53lxTarget

Defined in: [sensors/vl53lx/uld/base.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L87)

One return of a multi-target frame. `minRangeMm`/`maxRangeMm` are the edges
of the target's own pulse (histogram parts); the others repeat `distanceMm`.

## Properties

### distanceMm

```ts
distanceMm: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:88](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L88)

***

### status

```ts
status: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:89](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L89)

***

### statusText

```ts
statusText: string;
```

Defined in: [sensors/vl53lx/uld/base.ts:90](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L90)

***

### signalKcps

```ts
signalKcps: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:91](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L91)

***

### ambientKcps

```ts
ambientKcps: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L92)

***

### sigmaMm

```ts
sigmaMm: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L93)

***

### minRangeMm

```ts
minRangeMm: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L94)

***

### maxRangeMm

```ts
maxRangeMm: number;
```

Defined in: [sensors/vl53lx/uld/base.ts:95](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/uld/base.ts#L95)

# Type Alias: Vl53lxClearStep

```ts
type Vl53lxClearStep = readonly [number, number];
```

Defined in: [protocol/vl53lx.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L46)

One interrupt-release step: write `value` to register `addr`.

# Variable: PLOTTABLE\_STATUSES

```ts
const PLOTTABLE_STATUSES: readonly number[];
```

Defined in: [sensors/vl53lx/vl53lx.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L57)

Statuses that mean "this distance is real". A histogram product reports 6
on the first frame of a stream and 11 on a merged pulse: use this, not
`status === 0`, as the validity test on histogram products.

# Variable: VL53L4\_XSHUT\_OFF

```ts
const VL53L4_XSHUT_OFF: 0 = 0;
```

Defined in: [protocol/vl53l4.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L30)

# Variable: VL53L4\_XSHUT\_ON

```ts
const VL53L4_XSHUT_ON: 1 = 1;
```

Defined in: [protocol/vl53l4.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L31)

# Variable: VL53L4\_XSHUT\_RESET

```ts
const VL53L4_XSHUT_RESET: 2 = 2;
```

Defined in: [protocol/vl53l4.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L33)

Blocking on the MCU (≤ 20 ms, typ. ~2 ms); answered after the boot handshake.

# Variable: VL53LX\_CLASS\_BY\_PRODUCT

```ts
const VL53LX_CLASS_BY_PRODUCT: Readonly<Record<string, typeof Vl53lx>>;
```

Defined in: [sensors/vl53lx/vl53lx.ts:837](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L837)

Class per product (VL53L4CD on this firmware uses the generic class).

# Variable: VL53LX\_CLEAR\_STEPS\_MAX

```ts
const VL53LX_CLEAR_STEPS_MAX: 4 = 4;
```

Defined in: [protocol/vl53lx.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L41)

Interrupt-release steps a stream may carry (VL53_CLEAR_STEPS_WIRE_MAX).

# Variable: VL53LX\_DRIVER\_KINDS

```ts
const VL53LX_DRIVER_KINDS: readonly ["uld", "ulp", "histogram"] = registry.DRIVER_KINDS;
```

Defined in: [sensors/vl53lx/vl53lx.ts:861](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L861)

Shared barrel for the VL53L 1D-family TypeDoc entry points
(`entry-vl53l0x.ts` … `entry-vl53l4cx.ts`): the family class `Vl53lx`, the
measurement shape, the helpers and the bridge codecs, under the names
`src/index.ts` exports them with. The ULD ports underneath (`VL53L0X`,
`VL53L1`, …) are not public; the unstable `driver` escape hatch reaches
them. Every product page carries this same
family surface after its own class.

Not part of the shipped package — a doc-generation helper only.

# Variable: VL53LX\_INFO\_SIZE

```ts
const VL53LX_INFO_SIZE: 23 = 23;
```

Defined in: [protocol/vl53lx.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53lx.ts#L43)

RPT_VL53_INFO payload size (v2.00).

# Variable: VL53LX\_PRODUCTS

```ts
const VL53LX_PRODUCTS: readonly ["VL53L0X", "VL53L1CX", "VL53L1CB", "VL53L3CX", "VL53L4CD", "VL53L4CX"] = registry.PRODUCTS;
```

Defined in: [sensors/vl53lx/vl53lx.ts:860](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53lx/vl53lx.ts#L860)

Shared barrel for the VL53L 1D-family TypeDoc entry points
(`entry-vl53l0x.ts` … `entry-vl53l4cx.ts`): the family class `Vl53lx`, the
measurement shape, the helpers and the bridge codecs, under the names
`src/index.ts` exports them with. The ULD ports underneath (`VL53L0X`,
`VL53L1`, …) are not public; the unstable `driver` escape hatch reaches
them. Every product page carries this same
family surface after its own class.

Not part of the shipped package — a doc-generation helper only.

# Variable: WINDOW\_ABOVE

```ts
const WINDOW_ABOVE: 1 = 1;
```

Defined in: [sensors/vl53l4/uld.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L60)

# Variable: WINDOW\_BELOW

```ts
const WINDOW_BELOW: 0 = 0;
```

Defined in: [sensors/vl53l4/uld.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L59)

# Variable: WINDOW\_IN

```ts
const WINDOW_IN: 3 = 3;
```

Defined in: [sensors/vl53l4/uld.ts:62](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L62)

# Variable: WINDOW\_OUT

```ts
const WINDOW_OUT: 2 = 2;
```

Defined in: [sensors/vl53l4/uld.ts:61](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L61)

