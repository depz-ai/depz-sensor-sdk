# SR04 — API reference

The public API for the SR04 ultrasonic sensor. Discovery, device-base,
transport and other cross-sensor symbols shared by every sensor live in
the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Classes

- [Sr04](#class-sr04)

## Interfaces

- [Sr04Measurement](#interface-sr04measurement)

# Class: Sr04

Defined in: [sensors/sr04.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L59)

HC-SR04 ultrasonic ranging device.

`measurements` is the stream (contract 07 §3): loop samples *and* the
unsolicited single shots an AUX SYNC_IN edge triggers (RPT_DATA source cmd
0x36).

## Extends

- `DepzDevice`

## Constructors

### Constructor

```ts
new Sr04(transport, opts?): Sr04;
```

Defined in: [device/device.ts:200](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L200)

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

### measurements

```ts
readonly measurements: Stream<Sr04Measurement>;
```

Defined in: [sensors/sr04.ts:66](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L66)

The measurements (contract 07 §3): `on(cb)` subscribes, `subscribe(maxsize)`
iterates, `next(timeoutMs)` waits for one; `droppedCounts` and
`parseErrors` (RPT_DATA reports shorter than a measurement, contract 07
§2) are its counters.

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

### getSamplePeriodUs()

```ts
getSamplePeriodUs(): Promise<number>;
```

Defined in: [sensors/sr04.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L71)

Minimum interval between measurement starts (default 50000).

#### Returns

`Promise`\<`number`\>

***

### setSamplePeriodUs()

```ts
setSamplePeriodUs(periodUs): Promise<void>;
```

Defined in: [sensors/sr04.ts:81](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L81)

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

Defined in: [sensors/sr04.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L87)

#### Returns

`Promise`\<`number`\>

***

### setEchoDecayUs()

```ts
setEchoDecayUs(decayUs): Promise<number>;
```

Defined in: [sensors/sr04.ts:97](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L97)

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

Defined in: [sensors/sr04.ts:109](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L109)

Single shot. Rejects with BusyError while the loop is running. The
reply arrives only when the echo completes (or times out at ~65.5 ms),
so the default timeout is generous.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `1000` |

#### Returns

`Promise`\<[`Sr04Measurement`](#interface-sr04measurement)\>

***

### start()

```ts
start(): Promise<void>;
```

Defined in: [sensors/sr04.ts:120](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L120)

Start the measurement loop (idempotent).

#### Returns

`Promise`\<`void`\>

***

### stop()

```ts
stop(): Promise<void>;
```

Defined in: [sensors/sr04.ts:125](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L125)

Stop the measurement loop (idempotent).

#### Returns

`Promise`\<`void`\>

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [sensors/sr04.ts:131](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L131)

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

