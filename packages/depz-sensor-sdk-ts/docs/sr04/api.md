# SR04 — API reference

The public API for the SR04 ultrasonic sensor. Discovery, device-base,
transport and other cross-sensor symbols shared by every sensor live in
the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Classes

- [Sr04](classes/Sr04.md)

## Interfaces

- [Sr04Measurement](interfaces/Sr04Measurement.md)

# Class: Sr04

Defined in: [sensors/sr04.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L49)

HC-SR04 ultrasonic ranging device.

Measurements stream via callbacks (`onMeasurement`) and/or the pull
iterator (`measurements()`); both receive loop samples *and* unsolicited
single shots triggered by an AUX SYNC_IN edge (RPT_DATA source cmd 0x36).

## Extends

- `DepzDevice`

## Constructors

### Constructor

```ts
new Sr04(transport, opts?): Sr04;
```

Defined in: [device/device.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L189)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | `DeviceOptions` |

#### Returns

`Sr04`

#### Inherited from

```ts
DepzDevice.constructor
```

## Properties

### timeoutMs

```ts
timeoutMs: number;
```

Defined in: [device/device.ts:178](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L178)

#### Inherited from

```ts
DepzDevice.timeoutMs
```

***

### link

```ts
protected readonly link: DepzLink;
```

Defined in: [device/device.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L180)

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

Defined in: [device/device.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L210)

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

Defined in: [device/device.ts:474](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L474)

##### Returns

`TimeSync` \| `null`

#### Inherited from

```ts
DepzDevice.timeSync
```

***

### streamDroppedCounts

#### Get Signature

```ts
get streamDroppedCounts(): number[];
```

Defined in: [sensors/sr04.ts:141](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L141)

Drop counters of all live measurement queues (diagnostics).

##### Returns

`number`[]

## Methods

### open()

```ts
open(): Promise<void>;
```

Defined in: [device/device.ts:200](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L200)

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

Defined in: [device/device.ts:205](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L205)

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

Defined in: [device/device.ts:330](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L330)

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

Defined in: [device/device.ts:343](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L343)

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

Defined in: [device/device.ts:350](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L350)

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

Defined in: [device/device.ts:366](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L366)

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

Defined in: [device/device.ts:409](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L409)

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

Defined in: [device/device.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L414)

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

Defined in: [device/device.ts:425](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L425)

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

Defined in: [device/device.ts:431](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L431)

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

Defined in: [device/device.ts:437](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L437)

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

Defined in: [device/device.ts:444](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L444)

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

Defined in: [device/device.ts:449](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L449)

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

Defined in: [device/device.ts:457](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L457)

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

Defined in: [device/device.ts:479](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L479)

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

Defined in: [device/device.ts:484](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L484)

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

Defined in: [device/device.ts:491](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L491)

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

Defined in: [device/device.ts:495](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L495)

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

Defined in: [device/device.ts:501](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L501)

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

Defined in: [device/device.ts:506](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L506)

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

Defined in: [device/device.ts:515](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L515)

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

### getSamplePeriodUs()

```ts
getSamplePeriodUs(): Promise<number>;
```

Defined in: [sensors/sr04.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L56)

Minimum interval between measurement starts (default 50000).

#### Returns

`Promise`\<`number`\>

***

### setSamplePeriodUs()

```ts
setSamplePeriodUs(periodUs): Promise<void>;
```

Defined in: [sensors/sr04.ts:66](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L66)

The effective rate is auto-throttled by the echo window (contract 03
§3) — reading back returns the stored value, not the effective one.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `periodUs` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getEchoDecayUs()

```ts
getEchoDecayUs(): Promise<number>;
```

Defined in: [sensors/sr04.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L72)

#### Returns

`Promise`\<`number`\>

***

### setEchoDecayUs()

```ts
setEchoDecayUs(decayUs): Promise<number>;
```

Defined in: [sensors/sr04.ts:82](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L82)

Set the settle pause; the device clamps to 4000–65000 µs silently, so
this re-reads and returns the value actually in effect.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `decayUs` | `number` |

#### Returns

`Promise`\<`number`\>

***

### measureOnce()

```ts
measureOnce(timeoutMs?): Promise<Sr04Measurement>;
```

Defined in: [sensors/sr04.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L94)

Single shot. Rejects with BusyError while the loop is running. The
reply arrives only when the echo completes (or times out at ~65.5 ms),
so the default timeout is generous.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `1000` |

#### Returns

`Promise`\<[`Sr04Measurement`](../interfaces/Sr04Measurement.md)\>

***

### start()

```ts
start(): Promise<void>;
```

Defined in: [sensors/sr04.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L105)

Start the measurement loop (idempotent).

#### Returns

`Promise`\<`void`\>

***

### stop()

```ts
stop(): Promise<void>;
```

Defined in: [sensors/sr04.ts:110](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L110)

Stop the measurement loop (idempotent).

#### Returns

`Promise`\<`void`\>

***

### onMeasurement()

```ts
onMeasurement(cb): () => void;
```

Defined in: [sensors/sr04.ts:118](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L118)

Subscribe to measurements (read-pump context; don't block). Returns an
unsubscribe function.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`m`) => `void` |

#### Returns

() => `void`

***

### measurements()

```ts
measurements(maxsize?): StreamQueue<Sr04Measurement>;
```

Defined in: [sensors/sr04.ts:130](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L130)

Async iterator over measurements — bounded, drop-oldest (contract 07
§3). The returned queue exposes `droppedCount`; it ends when the device
closes or the consumer breaks out of iteration.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `maxsize` | `number` | `256` |

#### Returns

`StreamQueue`\<[`Sr04Measurement`](../interfaces/Sr04Measurement.md)\>

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [sensors/sr04.ts:147](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L147)

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

# Interface: Sr04Measurement

Defined in: [sensors/sr04.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L22)

One ranging result. `valid` is false for the no-echo timeout sentinel.

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/sr04.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L23)

***

### echoTimeUs

```ts
echoTimeUs: number;
```

Defined in: [sensors/sr04.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L24)

***

### source

```ts
source: "once" | "loop";
```

Defined in: [sensors/sr04.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L26)

"once": host command or SYNC_IN edge; "loop": measurement loop sample.

***

### valid

```ts
valid: boolean;
```

Defined in: [sensors/sr04.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L27)

***

### distanceMm

```ts
distanceMm: number | null;
```

Defined in: [sensors/sr04.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L29)

Distance at 343 m/s, or null when no echo was received.

