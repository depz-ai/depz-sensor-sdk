<!-- generated from ../bno086/ — edit those pages -->
# BNO085 (IMU) — API reference

The public API for the BNO085 IMU: the `Bno086` device plus the SHTP /
SH-2 protocol and report types. Discovery, device-base, transport and
other cross-sensor symbols live in the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Enumerations

- [Bno086Cmd](#enumeration-bno086cmd)
- [Bno086Rpt](#enumeration-bno086rpt)
- [SensorId](#enumeration-sensorid)
- [OscillatorType](#enumeration-oscillatortype)
- [ErrorSource](#enumeration-errorsource)
- [TareBasis](#enumeration-tarebasis)
- [TareAxis](#enumeration-tareaxis)
- [FeatureFlag](#enumeration-featureflag)
- [MotionIntent](#enumeration-motionintent)
- [FrsRecordId](#enumeration-frsrecordid)
- [ExecutableCommand](#enumeration-executablecommand)

## Classes

- [Bno086](#class-bno086)
- [Sh2Error](#class-sh2error)

## Interfaces

- [Bno086Data](#interface-bno086data)
- [CalibrationConfig](#interface-calibrationconfig)
- [Bno086Options](#interface-bno086options)
- [EnableOptions](#interface-enableoptions)
- [ReportBase](#interface-reportbase)
- [InputReportBase](#interface-inputreportbase)
- [Acceleration](#interface-acceleration)
- [Gyroscope](#interface-gyroscope)
- [Magnetometer](#interface-magnetometer)
- [UncalibratedGyroscope](#interface-uncalibratedgyroscope)
- [UncalibratedMagnetometer](#interface-uncalibratedmagnetometer)
- [RotationVector](#interface-rotationvector)
- [GyroIntegratedRV](#interface-gyrointegratedrv)
- [ScalarReport](#interface-scalarreport)
- [TapDetector](#interface-tapdetector)
- [StepCounter](#interface-stepcounter)
- [StepDetector](#interface-stepdetector)
- [SignificantMotion](#interface-significantmotion)
- [StabilityClassifier](#interface-stabilityclassifier)
- [ShakeDetector](#interface-shakedetector)
- [GenericEvent](#interface-genericevent)
- [PersonalActivityClassifier](#interface-personalactivityclassifier)
- [RawSensor](#interface-rawsensor)
- [MotionRequest](#interface-motionrequest)
- [FlushCompleted](#interface-flushcompleted)
- [UnknownReport](#interface-unknownreport)
- [ErrorRecord](#interface-errorrecord)
- [Counts](#interface-counts)
- [FeatureResponse](#interface-featureresponse)
- [ProductId](#interface-productid)
- [CommandResponse](#interface-commandresponse)
- [SensorMetadata](#interface-sensormetadata)

## Type Aliases

- [InputReport](#type-alias-inputreport)
- [Report](#type-alias-report)

## Variables

- [TX\_SLOTS](#variable-tx_slots)
- [TX\_SLOT\_SIZE](#variable-tx_slot_size)
- [BUSY\_BACKOFF\_MS](#variable-busy_backoff_ms)
- [MOTION\_REQUEST\_NAMES](#variable-motion_request_names)
- [MOTION\_INTENT\_NAMES](#variable-motion_intent_names)
- [STABILITY\_NAMES](#variable-stability_names)
- [ACTIVITY\_NAMES](#variable-activity_names)

## Functions

- [unpackBno086Data](#function-unpackbno086data)
- [errorRecordFromResponse](#function-errorrecordfromresponse)
- [unpackFeatureResponse](#function-unpackfeatureresponse)
- [unpackProductId](#function-unpackproductid)
- [unpackCommandResponse](#function-unpackcommandresponse)
- [sensorMetadataFromWords](#function-sensormetadatafromwords)

# Class: Bno086

Defined in: [sensors/bno086/bno086.ts:156](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L156)

BNO085 device: enable SH-2 sensors, stream typed reports.

Typical use:

    const imu = await openOnTransport(transport);   // or openDeviceFrom(...)
    if (!(imu instanceof Bno086)) throw new Error("not a BNO085");
    await imu.enable(SensorId.RotationVector, 100);
    for await (const r of imu.reports.subscribe(64)) { ... }

Callbacks run on the read-pump context — never await blocking device
methods (enable/tare/...) from inside one.

## Extends

- `DepzDevice`

## Constructors

### Constructor

```ts
new Bno086(transport, opts?): Bno086;
```

Defined in: [sensors/bno086/bno086.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L188)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | [`Bno086Options`](#interface-bno086options) |

#### Returns

`Bno086`

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

### busyRetries

```ts
busyRetries: number = 5;
```

Defined in: [sensors/bno086/bno086.ts:158](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L158)

SEND_SHTP_PACKET attempts before giving up.

***

### busyBackoffMs

```ts
busyBackoffMs: number = BUSY_BACKOFF_MS;
```

Defined in: [sensors/bno086/bno086.ts:160](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L160)

>= 200 ms per the bridge spec (contract 05 §2).

***

### reports

```ts
readonly reports: Stream<Report>;
```

Defined in: [sensors/bno086/bno086.ts:169](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L169)

The typed sensor reports (contract 07 §3): `on(cb, { where })` subscribes
(`where` keeps the reports it accepts — a SensorId filter),
`subscribe(maxsize, { where })` iterates, `next(timeoutMs)` waits for one;
`droppedCounts` and `parseErrors` (RPT_DATA reports shorter than their
header, contract 07 §2) are its counters.

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

### advertisement

#### Get Signature

```ts
get advertisement(): Uint8Array;
```

Defined in: [sensors/bno086/bno086.ts:532](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L532)

Raw SHTP channel-0 advertisement bytes seen since open/reset.

##### Returns

`Uint8Array`

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

### onTeardown()

```ts
protected onTeardown(error): void;
```

Defined in: [sensors/bno086/bno086.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L194)

Hook for sensor subclasses: reject any subclass-managed in-flight requests
and clear per-connection state when the link closes. Runs once, after the
base `pending` map is failed. Default: no-op.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `error` | `Error` |

#### Returns

`void`

#### Overrides

```ts
DepzDevice.onTeardown
```

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [sensors/bno086/bno086.ts:230](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L230)

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

***

### hardwareReset()

```ts
hardwareReset(timeoutMs?): Promise<void>;
```

Defined in: [sensors/bno086/bno086.ts:467](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L467)

Hard-reset the sensor via nRST (0x32). All SHTP state (seq counters,
partial cargos) and cached features restart from zero.

The hub's RPT_STATUS OK ack for the 0x32 command is treated as the reset
confirmation. The SH-2 executable-channel reset-complete (which the BNO08X
SH-2 spec would emit) is only waited for best-effort: older firmware
(≤ v0.95) did **not** emit it (ERRATA E9, now fixed
in newer firmware); the best-effort wait handles both, so its absence is
*not* an error — the sensor is fully usable without it.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `2000` |

#### Returns

`Promise`\<`void`\>

***

### wake()

```ts
wake(): Promise<void>;
```

Defined in: [sensors/bno086/bno086.ts:500](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L500)

Pulse WAKE (PS0): wakes the sensor from sleep, no state loss.

This is the board's hardware pin pulse (board command 0x33), not an SH-2
command; the SH-2 "On" command that ends [sleep](#sleep) is [powerOn](#poweron).

#### Returns

`Promise`\<`void`\>

***

### sleep()

```ts
sleep(): Promise<void>;
```

Defined in: [sensors/bno086/bno086.ts:515](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L515)

Put the hub to sleep: SH-2 "Sleep" (0x03 on SHTP channel 1).

Every report stops, except those enabled with
`flags: FeatureFlag.Wakeup | FeatureFlag.AlwaysOn`, which keep their rate;
the configuration (enabled sensors, rates) is kept. The hub sends no
response. [powerOn](#poweron) ends it.

On a DEPZ BNO085 (SH-2 3.12.6, 2026-10-16): all reports stopped within one
sample; a Wakeup|AlwaysOn report kept streaming at full rate on channel 4.

#### Returns

`Promise`\<`void`\>

***

### powerOn()

```ts
powerOn(): Promise<void>;
```

Defined in: [sensors/bno086/bno086.ts:527](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L527)

End [sleep](#sleep): SH-2 "On" (0x02 on SHTP channel 1).

Every report that was enabled comes back at its rate — no need to
re-enable. The hub sends no response. Not to be confused with
[wake](#wake), which pulses the WAKE pin (board command 0x33) instead of
sending an SH-2 command.

#### Returns

`Promise`\<`void`\>

***

### productId()

```ts
productId(timeoutMs?): Promise<ProductId>;
```

Defined in: [sensors/bno086/bno086.ts:546](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L546)

The SH-2 application's Product ID — the first of `productIds()`.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`ProductId`](#interface-productid)\>

***

### productIds()

```ts
productIds(timeoutMs?, settleMs?): Promise<ProductId[]>;
```

Defined in: [sensors/bno086/bno086.ts:557](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L557)

Product ID Request → every component's response, in the sensor's order
(the SH-2 application first). Collects until no response came for
`settleMs`, so the late ones can't answer the next request (they arrive in
a second cargo; taking only the first once returned the bootloader's
version on a back-to-back call).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |
| `settleMs` | `number` | `150` |

#### Returns

`Promise`\<[`ProductId`](#interface-productid)[]\>

***

### enable()

```ts
enable(
   sensor, 
   hz?, 
opts?): Promise<FeatureResponse | null>;
```

Defined in: [sensors/bno086/bno086.ts:586](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L586)

Enable `sensor` at the requested rate via Set Feature (0xFD).

Give either `hz` or `opts.intervalUs`. The hub rounds to its 1 kHz/2^n
grid; with `verify` (default) the granted rate is read back via Get
Feature and a result outside 0.9–2.1× the request emits a console
warning (contract 05 §7 — warn, never throw). Resolves with the
FeatureResponse (null when `verify: false`).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `sensor` | `number` |
| `hz?` | `number` |
| `opts?` | [`EnableOptions`](#interface-enableoptions) |

#### Returns

`Promise`\<[`FeatureResponse`](#interface-featureresponse) \| `null`\>

***

### disable()

```ts
disable(sensor): Promise<void>;
```

Defined in: [sensors/bno086/bno086.ts:646](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L646)

Disable `sensor` (Set Feature with interval 0).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `sensor` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getFeature()

```ts
getFeature(sensor, timeoutMs?): Promise<FeatureResponse>;
```

Defined in: [sensors/bno086/bno086.ts:652](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L652)

Get Feature Request/Response round trip for `sensor`.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensor` | `number` | `undefined` |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`FeatureResponse`](#interface-featureresponse)\>

***

### tareNow()

```ts
tareNow(axes?, basis?): Promise<void>;
```

Defined in: [sensors/bno086/bno086.ts:665](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L665)

Tare the selected axes against `basis` (no response per SH-2).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `axes` | `number` | `TareAxis.All` |
| `basis` | `number` | `TareBasis.RotationVector` |

#### Returns

`Promise`\<`void`\>

***

### persistTare()

```ts
persistTare(): Promise<void>;
```

Defined in: [sensors/bno086/bno086.ts:670](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L670)

Persist the current tare into FRS (no response per SH-2).

#### Returns

`Promise`\<`void`\>

***

### setReorientation()

```ts
setReorientation(
   x, 
   y, 
   z, 
w): Promise<void>;
```

Defined in: [sensors/bno086/bno086.ts:684](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L684)

Set the runtime reorientation quaternion (Q14 on the wire; all zeros
clears). No response per SH-2.

The rotation is applied on the board's side (new = report * r) and is
given in the hub's own axes: x and y swapped and z negated relative to
the axes of the reports — a rotation about x here turns the reports
about y (bench, 2026-10-22, SH-2 3.12.6). The System Orientation record
(FRS 0x2D3E) uses the same axes and is read only when the hub starts.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `x` | `number` |
| `y` | `number` |
| `z` | `number` |
| `w` | `number` |

#### Returns

`Promise`\<`void`\>

***

### setCalibration()

```ts
setCalibration(config?, timeoutMs?): Promise<void>;
```

Defined in: [sensors/bno086/bno086.ts:689](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L689)

Configure ME calibration; rejects with Sh2Error on non-zero status.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `config` | \{ `accel?`: `boolean`; `gyro?`: `boolean`; `mag?`: `boolean`; `planar?`: `boolean`; \} | `{}` |
| `config.accel?` | `boolean` | `undefined` |
| `config.gyro?` | `boolean` | `undefined` |
| `config.mag?` | `boolean` | `undefined` |
| `config.planar?` | `boolean` | `undefined` |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<`void`\>

***

### getCalibration()

```ts
getCalibration(timeoutMs?): Promise<CalibrationConfig>;
```

Defined in: [sensors/bno086/bno086.ts:705](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L705)

Read back which ME calibrations are running.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`CalibrationConfig`](#interface-calibrationconfig)\>

***

### saveDcd()

```ts
saveDcd(timeoutMs?): Promise<void>;
```

Defined in: [sensors/bno086/bno086.ts:720](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L720)

Save the dynamic calibration data to flash (DCD Save Now).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<`void`\>

***

### configurePeriodicDcd()

```ts
configurePeriodicDcd(enable): Promise<void>;
```

Defined in: [sensors/bno086/bno086.ts:728](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L728)

Enable/disable the hub's periodic DCD autosave (no response).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `enable` | `boolean` |

#### Returns

`Promise`\<`void`\>

***

### setMotionIntent()

```ts
setMotionIntent(intent): Promise<void>;
```

Defined in: [sensors/bno086/bno086.ts:741](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L741)

Tell the hub how the device is about to move (Interactive Calibration,
command 0x0E, see MotionIntent), so it can refine the gyro zero-rate
offset while the device is still. No response per SH-2; a BNO085 ignores
it. The hub's answer, if it wants the device to stop, arrives as a
MotionRequest report (enable SensorId.MotionRequest).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `intent` | `number` |

#### Returns

`Promise`\<`void`\>

***

### enableMotionRequest()

```ts
enableMotionRequest(hz?, opts?): Promise<FeatureResponse | null>;
```

Defined in: [sensors/bno086/bno086.ts:750](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L750)

Enable MotionRequest (0x2B) reports. The hub sends one when its request
changes, not at `hz`; it reads the rate back late, so the granted rate is
not checked.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `1` |
| `opts` | [`EnableOptions`](#interface-enableoptions) | `{}` |

#### Returns

`Promise`\<[`FeatureResponse`](#interface-featureresponse) \| `null`\>

***

### flush()

```ts
flush(sensor, timeoutMs?): Promise<void>;
```

Defined in: [sensors/bno086/bno086.ts:762](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L762)

Force Sensor Flush (0xF0): have the hub send `sensor`'s batched reports
now instead of when its batch interval runs out. Resolves once the hub's
Flush Completed (0xEF) for `sensor` arrives — every report batched before
the call has been emitted by then.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensor` | `number` | `undefined` |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<`void`\>

***

### frsRead()

```ts
frsRead(recordId, timeoutMs?): Promise<number[]>;
```

Defined in: [sensors/bno086/bno086.ts:794](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L794)

Read a whole FRS record; resolves with its 32-bit words.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `recordId` | `number` | `undefined` |
| `timeoutMs` | `number` | `FRS_TIMEOUT_MS` |

#### Returns

`Promise`\<`number`[]\>

***

### frsWrite()

```ts
frsWrite(
   recordId, 
   words, 
timeoutMs?): Promise<void>;
```

Defined in: [sensors/bno086/bno086.ts:812](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L812)

Write a whole FRS record (word list); rejects with Sh2Error on failure.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `recordId` | `number` | `undefined` |
| `words` | `Iterable`\<`number`\> | `undefined` |
| `timeoutMs` | `number` | `FRS_TIMEOUT_MS` |

#### Returns

`Promise`\<`void`\>

***

### getMetadata()

```ts
getMetadata(sensor, timeoutMs?): Promise<SensorMetadata>;
```

Defined in: [sensors/bno086/bno086.ts:835](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L835)

Read + parse the sensor's FRS metadata record.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensor` | `number` | `undefined` |
| `timeoutMs` | `number` | `FRS_TIMEOUT_MS` |

#### Returns

`Promise`\<[`SensorMetadata`](#interface-sensormetadata)\>

***

### getOscillatorType()

```ts
getOscillatorType(timeoutMs?): Promise<OscillatorType>;
```

Defined in: [sensors/bno086/bno086.ts:848](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L848)

Get Oscillator Type (command 0x0A). r[0] is the type directly.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`OscillatorType`](#enumeration-oscillatortype)\>

***

### clearDcdAndReset()

```ts
clearDcdAndReset(timeoutMs?): Promise<void>;
```

Defined in: [sensors/bno086/bno086.ts:860](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L860)

Clear the in-RAM dynamic calibration and reset the sensor (command 0x0B).
There is no command response — the hub resets, so this waits for the
executable reset-complete like hardwareReset(): `DepzTimeoutError` when
it does not come in `timeoutMs` (`Infinity` for no limit), the device's
own error as soon as the device is closed or lost while waiting.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `2000` |

#### Returns

`Promise`\<`void`\>

***

### getErrors()

```ts
getErrors(severity?, timeoutMs?): Promise<ErrorRecord[]>;
```

Defined in: [sensors/bno086/bno086.ts:890](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L890)

Read the error queue (command 0x01), filtered to `severity` or greater.
Records stream until one with source == 255 (no more).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `severity` | `number` | `0` |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`ErrorRecord`](#interface-errorrecord)[]\>

***

### getCounts()

```ts
getCounts(sensor, timeoutMs?): Promise<Counts>;
```

Defined in: [sensors/bno086/bno086.ts:908](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L908)

Read a sensor's event counts (command 0x02). The hub answers with two
responses (responseSeq 0 then 1).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensor` | `number` | `undefined` |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`Counts`](#interface-counts)\>

***

### clearCounts()

```ts
clearCounts(sensor, timeoutMs?): Promise<void>;
```

Defined in: [sensors/bno086/bno086.ts:935](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L935)

Clear a sensor's event counts (command 0x02, subcommand 1).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensor` | `number` | `undefined` |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<`void`\>

# Class: Sh2Error

Defined in: [sensors/bno086/sh2.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L15)

SH-2 level failure (bad response status, FRS error, ...).

## Extends

- `DepzError`

## Constructors

### Constructor

```ts
new Sh2Error(message?): Sh2Error;
```

Defined in: [errors.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L8)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `message?` | `string` |

#### Returns

`Sh2Error`

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

# Enumeration: Bno086Cmd

Defined in: [protocol/bno086.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L14)

## Enumeration Members

### SensorReset

```ts
SensorReset: 50;
```

Defined in: [protocol/bno086.ts:16](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L16)

Hardware reset via nRST; RPT_STATUS OK.

***

### SensorWakeUp

```ts
SensorWakeUp: 51;
```

Defined in: [protocol/bno086.ts:18](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L18)

1 ms WAKE (PS0) pulse; RPT_STATUS OK.

***

### SendShtpPacket

```ts
SendShtpPacket: 52;
```

Defined in: [protocol/bno086.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L20)

Payload = raw SHTP frame; RPT_STATUS OK/ERR_BUSY.

# Enumeration: Bno086Rpt

Defined in: [protocol/bno086.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L23)

## Enumeration Members

### Data

```ts
Data: 145;
```

Defined in: [protocol/bno086.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L25)

rpt_data_t: cmd u8, timestamp_us u64, raw SHTP frame.

# Enumeration: ErrorSource

Defined in: [sensors/bno086/sh2.ts:91](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L91)

`source` field of an error record (SH-2 §6.4.1).

## Enumeration Members

### MotionEngine

```ts
MotionEngine: 1;
```

Defined in: [sensors/bno086/sh2.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L92)

***

### MotionHub

```ts
MotionHub: 2;
```

Defined in: [sensors/bno086/sh2.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L93)

***

### SensorHub

```ts
SensorHub: 3;
```

Defined in: [sensors/bno086/sh2.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L94)

***

### Chip

```ts
Chip: 4;
```

Defined in: [sensors/bno086/sh2.ts:95](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L95)

***

### NoMoreErrors

```ts
NoMoreErrors: 255;
```

Defined in: [sensors/bno086/sh2.ts:97](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L97)

Sentinel: end of the error queue.

# Enumeration: ExecutableCommand

Defined in: [sensors/bno086/shtp.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L45)

One-byte host writes on the executable channel (SHTP channel 1). The hub
answers none of them. `Reset` restarts the hub (answered later by the 0x01
reset-complete on the same channel); `Sleep` stops every report except those
enabled with `FeatureFlag.Wakeup | FeatureFlag.AlwaysOn`, keeping the
configuration; `On` ends `Sleep` and every report resumes at its rate.

## Enumeration Members

### Reset

```ts
Reset: 1;
```

Defined in: [sensors/bno086/shtp.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L46)

***

### On

```ts
On: 2;
```

Defined in: [sensors/bno086/shtp.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L47)

***

### Sleep

```ts
Sleep: 3;
```

Defined in: [sensors/bno086/shtp.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L48)

# Enumeration: FeatureFlag

Defined in: [sensors/bno086/sh2.ts:183](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L183)

Set Feature `flags` bits (SH-2 §6.5.4) for `Bno086.enable({ flags })`.

`Wakeup | AlwaysOn` keeps a sensor reporting through `Bno086.sleep()`: on a
DEPZ BNO085 (SH-2 3.12.6, 2026-10-16) such a report kept its full rate
during sleep, delivered on SHTP channel 4 (wake input), while every other
report stopped.

## Enumeration Members

### Wakeup

```ts
Wakeup: 4;
```

Defined in: [sensors/bno086/sh2.ts:185](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L185)

Bit 2: wake-up enabled — reports on channel 4.

***

### AlwaysOn

```ts
AlwaysOn: 8;
```

Defined in: [sensors/bno086/sh2.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L187)

Bit 3: always-on — keeps running while the hub sleeps.

# Enumeration: FrsRecordId

Defined in: [sensors/bno086/sh2.ts:437](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L437)

FRS record IDs used by this SDK (SH-2 figure 28; metadata records).

## Enumeration Members

### StaticCalibrationAgm

```ts
StaticCalibrationAgm: 31097;
```

Defined in: [sensors/bno086/sh2.ts:438](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L438)

***

### NominalCalibration

```ts
NominalCalibration: 19789;
```

Defined in: [sensors/bno086/sh2.ts:439](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L439)

***

### DynamicCalibration

```ts
DynamicCalibration: 7967;
```

Defined in: [sensors/bno086/sh2.ts:440](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L440)

***

### MePowerMgmt

```ts
MePowerMgmt: 54242;
```

Defined in: [sensors/bno086/sh2.ts:441](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L441)

***

### SystemOrientation

```ts
SystemOrientation: 11582;
```

Defined in: [sensors/bno086/sh2.ts:443](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L443)

Mounting quaternion, 4 × Q30 words.

***

### AccelOrientation

```ts
AccelOrientation: 11585;
```

Defined in: [sensors/bno086/sh2.ts:444](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L444)

***

### GyroscopeOrientation

```ts
GyroscopeOrientation: 11590;
```

Defined in: [sensors/bno086/sh2.ts:445](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L445)

***

### MagnetometerOrientation

```ts
MagnetometerOrientation: 11596;
```

Defined in: [sensors/bno086/sh2.ts:446](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L446)

***

### ArvrStabilizationRv

```ts
ArvrStabilizationRv: 15917;
```

Defined in: [sensors/bno086/sh2.ts:447](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L447)

***

### ArvrStabilizationGrv

```ts
ArvrStabilizationGrv: 15918;
```

Defined in: [sensors/bno086/sh2.ts:448](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L448)

***

### SerialNumber

```ts
SerialNumber: 19275;
```

Defined in: [sensors/bno086/sh2.ts:450](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L450)

One 32-bit word; empty unless programmed.

***

### UserRecord

```ts
UserRecord: 29876;
```

Defined in: [sensors/bno086/sh2.ts:452](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L452)

Free for the user, up to 64 words.

***

### SigMotionDetectConfig

```ts
SigMotionDetectConfig: 49780;
```

Defined in: [sensors/bno086/sh2.ts:454](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L454)

***

### ShakeDetectConfig

```ts
ShakeDetectConfig: 32125;
```

Defined in: [sensors/bno086/sh2.ts:455](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L455)

***

### StabilityDetectorConfig

```ts
StabilityDetectorConfig: 60805;
```

Defined in: [sensors/bno086/sh2.ts:456](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L456)

***

### ActivityTrackerConfig

```ts
ActivityTrackerConfig: 60808;
```

Defined in: [sensors/bno086/sh2.ts:458](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L458)

Personal-activity-classifier config.

***

### GyroIntegratedRvConfig

```ts
GyroIntegratedRvConfig: 41378;
```

Defined in: [sensors/bno086/sh2.ts:460](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L460)

Reference, sync interval, max error, prediction (SH-2 §4.3.24).

# Enumeration: MotionIntent

Defined in: [sensors/bno086/sh2.ts:407](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L407)

P0 of the Interactive Calibration command (0x0E), SH-2 §6.4.12.1 — what the
host knows about the device's motion. Unknown is the hub's initial state.

## Enumeration Members

### Unknown

```ts
Unknown: 0;
```

Defined in: [sensors/bno086/sh2.ts:408](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L408)

***

### StationaryNoVibration

```ts
StationaryNoVibration: 1;
```

Defined in: [sensors/bno086/sh2.ts:409](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L409)

***

### StationaryWithVibration

```ts
StationaryWithVibration: 2;
```

Defined in: [sensors/bno086/sh2.ts:410](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L410)

***

### InMotion

```ts
InMotion: 3;
```

Defined in: [sensors/bno086/sh2.ts:411](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L411)

***

### InMotionAccelerating

```ts
InMotionAccelerating: 4;
```

Defined in: [sensors/bno086/sh2.ts:412](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L412)

# Enumeration: OscillatorType

Defined in: [sensors/bno086/sh2.ts:84](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L84)

Get-Oscillator-Type (command 0x0A) result (r[0]).

## Enumeration Members

### Internal

```ts
Internal: 0;
```

Defined in: [sensors/bno086/sh2.ts:85](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L85)

***

### ExtCrystal

```ts
ExtCrystal: 1;
```

Defined in: [sensors/bno086/sh2.ts:86](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L86)

***

### ExtClock

```ts
ExtClock: 2;
```

Defined in: [sensors/bno086/sh2.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L87)

# Enumeration: SensorId

Defined in: [sensors/bno086/reports.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L23)

SH-2 input report IDs (datasheet §1.3.5, sh2 reference driver).

## Enumeration Members

### Accelerometer

```ts
Accelerometer: 1;
```

Defined in: [sensors/bno086/reports.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L24)

***

### Gyroscope

```ts
Gyroscope: 2;
```

Defined in: [sensors/bno086/reports.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L25)

***

### Magnetometer

```ts
Magnetometer: 3;
```

Defined in: [sensors/bno086/reports.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L26)

***

### LinearAcceleration

```ts
LinearAcceleration: 4;
```

Defined in: [sensors/bno086/reports.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L27)

***

### RotationVector

```ts
RotationVector: 5;
```

Defined in: [sensors/bno086/reports.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L28)

***

### Gravity

```ts
Gravity: 6;
```

Defined in: [sensors/bno086/reports.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L29)

***

### UncalibratedGyroscope

```ts
UncalibratedGyroscope: 7;
```

Defined in: [sensors/bno086/reports.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L30)

***

### GameRotationVector

```ts
GameRotationVector: 8;
```

Defined in: [sensors/bno086/reports.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L31)

***

### GeomagneticRotationVector

```ts
GeomagneticRotationVector: 9;
```

Defined in: [sensors/bno086/reports.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L32)

***

### Pressure

```ts
Pressure: 10;
```

Defined in: [sensors/bno086/reports.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L33)

***

### AmbientLight

```ts
AmbientLight: 11;
```

Defined in: [sensors/bno086/reports.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L34)

***

### Humidity

```ts
Humidity: 12;
```

Defined in: [sensors/bno086/reports.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L35)

***

### Proximity

```ts
Proximity: 13;
```

Defined in: [sensors/bno086/reports.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L36)

***

### Temperature

```ts
Temperature: 14;
```

Defined in: [sensors/bno086/reports.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L37)

***

### UncalibratedMagnetometer

```ts
UncalibratedMagnetometer: 15;
```

Defined in: [sensors/bno086/reports.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L38)

***

### TapDetector

```ts
TapDetector: 16;
```

Defined in: [sensors/bno086/reports.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L39)

***

### StepCounter

```ts
StepCounter: 17;
```

Defined in: [sensors/bno086/reports.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L40)

***

### SignificantMotion

```ts
SignificantMotion: 18;
```

Defined in: [sensors/bno086/reports.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L41)

***

### StabilityClassifier

```ts
StabilityClassifier: 19;
```

Defined in: [sensors/bno086/reports.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L42)

***

### RawAccelerometer

```ts
RawAccelerometer: 20;
```

Defined in: [sensors/bno086/reports.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L43)

***

### RawGyroscope

```ts
RawGyroscope: 21;
```

Defined in: [sensors/bno086/reports.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L44)

***

### RawMagnetometer

```ts
RawMagnetometer: 22;
```

Defined in: [sensors/bno086/reports.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L45)

***

### StepDetector

```ts
StepDetector: 24;
```

Defined in: [sensors/bno086/reports.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L46)

***

### ShakeDetector

```ts
ShakeDetector: 25;
```

Defined in: [sensors/bno086/reports.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L47)

***

### FlipDetector

```ts
FlipDetector: 26;
```

Defined in: [sensors/bno086/reports.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L48)

***

### PickupDetector

```ts
PickupDetector: 27;
```

Defined in: [sensors/bno086/reports.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L49)

***

### StabilityDetector

```ts
StabilityDetector: 28;
```

Defined in: [sensors/bno086/reports.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L50)

***

### PersonalActivityClassifier

```ts
PersonalActivityClassifier: 30;
```

Defined in: [sensors/bno086/reports.ts:51](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L51)

***

### SleepDetector

```ts
SleepDetector: 31;
```

Defined in: [sensors/bno086/reports.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L52)

***

### TiltDetector

```ts
TiltDetector: 32;
```

Defined in: [sensors/bno086/reports.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L53)

***

### PocketDetector

```ts
PocketDetector: 33;
```

Defined in: [sensors/bno086/reports.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L54)

***

### CircleDetector

```ts
CircleDetector: 34;
```

Defined in: [sensors/bno086/reports.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L55)

***

### HeartRateMonitor

```ts
HeartRateMonitor: 35;
```

Defined in: [sensors/bno086/reports.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L56)

***

### ArvrStabilizedRv

```ts
ArvrStabilizedRv: 40;
```

Defined in: [sensors/bno086/reports.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L57)

***

### ArvrStabilizedGameRv

```ts
ArvrStabilizedGameRv: 41;
```

Defined in: [sensors/bno086/reports.ts:58](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L58)

***

### GyroIntegratedRv

```ts
GyroIntegratedRv: 42;
```

Defined in: [sensors/bno086/reports.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L59)

***

### MotionRequest

```ts
MotionRequest: 43;
```

Defined in: [sensors/bno086/reports.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L60)

# Enumeration: TareAxis

Defined in: [sensors/bno086/sh2.ts:154](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L154)

## Enumeration Members

### X

```ts
X: 1;
```

Defined in: [sensors/bno086/sh2.ts:155](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L155)

***

### Y

```ts
Y: 2;
```

Defined in: [sensors/bno086/sh2.ts:156](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L156)

***

### Z

```ts
Z: 4;
```

Defined in: [sensors/bno086/sh2.ts:157](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L157)

***

### All

```ts
All: 7;
```

Defined in: [sensors/bno086/sh2.ts:158](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L158)

# Enumeration: TareBasis

Defined in: [sensors/bno086/sh2.ts:145](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L145)

Rotation vector used as the tare reference (Tare Now P2).

## Enumeration Members

### RotationVector

```ts
RotationVector: 0;
```

Defined in: [sensors/bno086/sh2.ts:146](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L146)

***

### GameRotationVector

```ts
GameRotationVector: 1;
```

Defined in: [sensors/bno086/sh2.ts:147](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L147)

***

### GeomagneticRotationVector

```ts
GeomagneticRotationVector: 2;
```

Defined in: [sensors/bno086/sh2.ts:148](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L148)

***

### GyroIntegratedRv

```ts
GyroIntegratedRv: 3;
```

Defined in: [sensors/bno086/sh2.ts:149](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L149)

***

### ArvrStabilizedRv

```ts
ArvrStabilizedRv: 4;
```

Defined in: [sensors/bno086/sh2.ts:150](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L150)

***

### ArvrStabilizedGameRv

```ts
ArvrStabilizedGameRv: 5;
```

Defined in: [sensors/bno086/sh2.ts:151](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L151)

# Function: errorRecordFromResponse()

```ts
function errorRecordFromResponse(resp): ErrorRecord;
```

Defined in: [sensors/bno086/sh2.ts:130](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L130)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `resp` | [`CommandResponse`](#interface-commandresponse) |

## Returns

[`ErrorRecord`](#interface-errorrecord)

# Function: sensorMetadataFromWords()

```ts
function sensorMetadataFromWords(words): SensorMetadata;
```

Defined in: [sensors/bno086/sh2.ts:735](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L735)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `words` | `number`[] |

## Returns

[`SensorMetadata`](#interface-sensormetadata)

# Function: unpackBno086Data()

```ts
function unpackBno086Data(payload): Bno086Data;
```

Defined in: [protocol/bno086.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L44)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Bno086Data`](#interface-bno086data)

# Function: unpackCommandResponse()

```ts
function unpackCommandResponse(payload): CommandResponse;
```

Defined in: [sensors/bno086/sh2.ts:331](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L331)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`CommandResponse`](#interface-commandresponse)

# Function: unpackFeatureResponse()

```ts
function unpackFeatureResponse(payload): FeatureResponse;
```

Defined in: [sensors/bno086/sh2.ts:233](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L233)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`FeatureResponse`](#interface-featureresponse)

# Function: unpackProductId()

```ts
function unpackProductId(payload): ProductId;
```

Defined in: [sensors/bno086/sh2.ts:273](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L273)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`ProductId`](#interface-productid)

# Interface: Acceleration

Defined in: [sensors/bno086/reports.ts:203](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L203)

0x01 accelerometer / 0x04 linear acceleration / 0x06 gravity (Q8).

## Extends

- [`InputReportBase`](#interface-inputreportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`sensorId`](#sensorid-7)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`timestampUs`](#timestampus-6)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`seq`](#seq-5)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`accuracy`](#accuracy-3)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`delayUs`](#delayus-3)

***

### type

```ts
type: "Acceleration";
```

Defined in: [sensors/bno086/reports.ts:204](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L204)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [sensors/bno086/reports.ts:205](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L205)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [sensors/bno086/reports.ts:206](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L206)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [sensors/bno086/reports.ts:207](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L207)

***

### x

```ts
x: number;
```

Defined in: [sensors/bno086/reports.ts:209](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L209)

m/s².

***

### y

```ts
y: number;
```

Defined in: [sensors/bno086/reports.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L210)

***

### z

```ts
z: number;
```

Defined in: [sensors/bno086/reports.ts:211](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L211)

# Interface: Bno086Data

Defined in: [protocol/bno086.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L35)

One RPT_DATA report: an SHTP frame captured from the sensor bus.

## Properties

### cmd

```ts
cmd: number;
```

Defined in: [protocol/bno086.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L37)

Always 0x00 in practice (ERRATA E2) — never correlate on it.

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [protocol/bno086.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L39)

MCU uptime at frame capture (µs).

***

### shtp

```ts
shtp: Uint8Array;
```

Defined in: [protocol/bno086.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L41)

Raw SHTP frame (4-byte header + cargo fragment).

# Interface: Bno086Options

Defined in: [sensors/bno086/bno086.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L105)

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

### busyRetries?

```ts
optional busyRetries?: number;
```

Defined in: [sensors/bno086/bno086.ts:107](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L107)

SEND_SHTP_PACKET attempts before giving up (default 5).

***

### busyBackoffMs?

```ts
optional busyBackoffMs?: number;
```

Defined in: [sensors/bno086/bno086.ts:109](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L109)

ERR_BUSY backoff; >= 200 ms per the bridge spec (tests inject less).

# Interface: CalibrationConfig

Defined in: [sensors/bno086/bno086.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L98)

ME calibration enables as reported by the sensor.

## Properties

### accel

```ts
accel: boolean;
```

Defined in: [sensors/bno086/bno086.ts:99](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L99)

***

### gyro

```ts
gyro: boolean;
```

Defined in: [sensors/bno086/bno086.ts:100](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L100)

***

### mag

```ts
mag: boolean;
```

Defined in: [sensors/bno086/bno086.ts:101](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L101)

***

### planar

```ts
planar: boolean;
```

Defined in: [sensors/bno086/bno086.ts:102](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L102)

# Interface: CommandResponse

Defined in: [sensors/bno086/sh2.ts:320](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L320)

Command Response (0xF1), 16 bytes.

`commandSeq` echoes the request's sequence number (correlate on it plus
`command`); `responseSeq` counts multiple responses to one request.
R0 is the status word for most commands (0 = success).

## Properties

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/sh2.ts:321](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L321)

***

### command

```ts
command: number;
```

Defined in: [sensors/bno086/sh2.ts:322](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L322)

***

### commandSeq

```ts
commandSeq: number;
```

Defined in: [sensors/bno086/sh2.ts:323](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L323)

***

### responseSeq

```ts
responseSeq: number;
```

Defined in: [sensors/bno086/sh2.ts:324](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L324)

***

### r

```ts
r: number[];
```

Defined in: [sensors/bno086/sh2.ts:326](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L326)

R0..R10.

***

### status

```ts
status: number;
```

Defined in: [sensors/bno086/sh2.ts:328](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L328)

R0.

# Interface: Counts

Defined in: [sensors/bno086/sh2.ts:136](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L136)

Per-sensor event counts (command 0x02 get response, 2 messages).

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/sh2.ts:137](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L137)

***

### offered

```ts
offered: number;
```

Defined in: [sensors/bno086/sh2.ts:138](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L138)

***

### accepted

```ts
accepted: number;
```

Defined in: [sensors/bno086/sh2.ts:139](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L139)

***

### on

```ts
on: number;
```

Defined in: [sensors/bno086/sh2.ts:140](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L140)

***

### attempted

```ts
attempted: number;
```

Defined in: [sensors/bno086/sh2.ts:141](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L141)

# Interface: EnableOptions

Defined in: [sensors/bno086/bno086.ts:112](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L112)

## Properties

### intervalUs?

```ts
optional intervalUs?: number;
```

Defined in: [sensors/bno086/bno086.ts:114](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L114)

Alternative to `hz`: exact report interval in µs.

***

### batchUs?

```ts
optional batchUs?: number;
```

Defined in: [sensors/bno086/bno086.ts:115](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L115)

***

### sensitivity?

```ts
optional sensitivity?: number;
```

Defined in: [sensors/bno086/bno086.ts:116](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L116)

***

### flags?

```ts
optional flags?: number;
```

Defined in: [sensors/bno086/bno086.ts:117](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L117)

***

### cfgWord?

```ts
optional cfgWord?: number;
```

Defined in: [sensors/bno086/bno086.ts:118](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L118)

***

### verify?

```ts
optional verify?: boolean;
```

Defined in: [sensors/bno086/bno086.ts:120](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L120)

Read back the granted rate via Get Feature (default true).

***

### timeoutMs?

```ts
optional timeoutMs?: number;
```

Defined in: [sensors/bno086/bno086.ts:121](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L121)

# Interface: ErrorRecord

Defined in: [sensors/bno086/sh2.ts:120](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L120)

One error queue entry (command 0x01 response, r[0..5]).

## Properties

### severity

```ts
severity: number;
```

Defined in: [sensors/bno086/sh2.ts:121](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L121)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/sh2.ts:122](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L122)

***

### source

```ts
source: number;
```

Defined in: [sensors/bno086/sh2.ts:124](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L124)

ErrorSource.

***

### error

```ts
error: number;
```

Defined in: [sensors/bno086/sh2.ts:125](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L125)

***

### module

```ts
module: number;
```

Defined in: [sensors/bno086/sh2.ts:126](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L126)

***

### code

```ts
code: number;
```

Defined in: [sensors/bno086/sh2.ts:127](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L127)

# Interface: FeatureResponse

Defined in: [sensors/bno086/sh2.ts:223](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L223)

Get Feature Response (0xFC), 17 bytes — the rates in effect.

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/sh2.ts:224](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L224)

***

### flags

```ts
flags: number;
```

Defined in: [sensors/bno086/sh2.ts:225](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L225)

***

### sensitivity

```ts
sensitivity: number;
```

Defined in: [sensors/bno086/sh2.ts:226](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L226)

***

### intervalUs

```ts
intervalUs: number;
```

Defined in: [sensors/bno086/sh2.ts:228](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L228)

Actual report interval granted by the hub.

***

### batchUs

```ts
batchUs: number;
```

Defined in: [sensors/bno086/sh2.ts:229](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L229)

***

### cfgWord

```ts
cfgWord: number;
```

Defined in: [sensors/bno086/sh2.ts:230](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L230)

# Interface: FlushCompleted

Defined in: [sensors/bno086/reports.ts:413](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L413)

0xEF Flush Completed: every batched report of `flushedSensor` as of the
matching Force Sensor Flush has been sent. `sensorId` is 0xEF.

## Extends

- [`ReportBase`](#interface-reportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`ReportBase`](#interface-reportbase).[`sensorId`](#sensorid-12)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`ReportBase`](#interface-reportbase).[`timestampUs`](#timestampus-11)

***

### type

```ts
type: "FlushCompleted";
```

Defined in: [sensors/bno086/reports.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L414)

***

### flushedSensor

```ts
flushedSensor: number;
```

Defined in: [sensors/bno086/reports.ts:415](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L415)

# Interface: GenericEvent

Defined in: [sensors/bno086/reports.ts:363](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L363)

Simple u16-value detectors: 0x1A flip, 0x1B pickup, 0x1C stability
detector, 0x1F sleep, 0x20 tilt, 0x21 pocket, 0x22 circle, 0x23 HR.

## Extends

- [`InputReportBase`](#interface-inputreportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`sensorId`](#sensorid-7)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`timestampUs`](#timestampus-6)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`seq`](#seq-5)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`accuracy`](#accuracy-3)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`delayUs`](#delayus-3)

***

### type

```ts
type: "GenericEvent";
```

Defined in: [sensors/bno086/reports.ts:364](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L364)

***

### valueRaw

```ts
valueRaw: number;
```

Defined in: [sensors/bno086/reports.ts:365](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L365)

# Interface: GyroIntegratedRV

Defined in: [sensors/bno086/reports.ts:293](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L293)

0x2A gyro-integrated rotation vector (channel 5, dense — no SH-2 header).
Quaternion Q14, angular velocity Q10 rad/s.

## Extends

- [`ReportBase`](#interface-reportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`ReportBase`](#interface-reportbase).[`sensorId`](#sensorid-12)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`ReportBase`](#interface-reportbase).[`timestampUs`](#timestampus-11)

***

### type

```ts
type: "GyroIntegratedRV";
```

Defined in: [sensors/bno086/reports.ts:294](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L294)

***

### iRaw

```ts
iRaw: number;
```

Defined in: [sensors/bno086/reports.ts:295](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L295)

***

### jRaw

```ts
jRaw: number;
```

Defined in: [sensors/bno086/reports.ts:296](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L296)

***

### kRaw

```ts
kRaw: number;
```

Defined in: [sensors/bno086/reports.ts:297](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L297)

***

### realRaw

```ts
realRaw: number;
```

Defined in: [sensors/bno086/reports.ts:298](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L298)

***

### vxRaw

```ts
vxRaw: number;
```

Defined in: [sensors/bno086/reports.ts:299](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L299)

***

### vyRaw

```ts
vyRaw: number;
```

Defined in: [sensors/bno086/reports.ts:300](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L300)

***

### vzRaw

```ts
vzRaw: number;
```

Defined in: [sensors/bno086/reports.ts:301](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L301)

***

### i

```ts
i: number;
```

Defined in: [sensors/bno086/reports.ts:302](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L302)

***

### j

```ts
j: number;
```

Defined in: [sensors/bno086/reports.ts:303](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L303)

***

### k

```ts
k: number;
```

Defined in: [sensors/bno086/reports.ts:304](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L304)

***

### real

```ts
real: number;
```

Defined in: [sensors/bno086/reports.ts:305](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L305)

***

### angularVelocity

```ts
angularVelocity: [number, number, number];
```

Defined in: [sensors/bno086/reports.ts:307](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L307)

rad/s.

# Interface: Gyroscope

Defined in: [sensors/bno086/reports.ts:215](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L215)

0x02 calibrated gyroscope (Q9).

## Extends

- [`InputReportBase`](#interface-inputreportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`sensorId`](#sensorid-7)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`timestampUs`](#timestampus-6)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`seq`](#seq-5)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`accuracy`](#accuracy-3)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`delayUs`](#delayus-3)

***

### type

```ts
type: "Gyroscope";
```

Defined in: [sensors/bno086/reports.ts:216](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L216)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [sensors/bno086/reports.ts:217](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L217)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [sensors/bno086/reports.ts:218](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L218)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [sensors/bno086/reports.ts:219](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L219)

***

### x

```ts
x: number;
```

Defined in: [sensors/bno086/reports.ts:221](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L221)

rad/s.

***

### y

```ts
y: number;
```

Defined in: [sensors/bno086/reports.ts:222](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L222)

***

### z

```ts
z: number;
```

Defined in: [sensors/bno086/reports.ts:223](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L223)

# Interface: InputReportBase

Defined in: [sensors/bno086/reports.ts:193](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L193)

Channel-3/4 report with the common SH-2 header fields.

## Extends

- [`ReportBase`](#interface-reportbase)

## Extended by

- [`Acceleration`](#interface-acceleration)
- [`GenericEvent`](#interface-genericevent)
- [`Gyroscope`](#interface-gyroscope)
- [`Magnetometer`](#interface-magnetometer)
- [`MotionRequest`](#interface-motionrequest)
- [`PersonalActivityClassifier`](#interface-personalactivityclassifier)
- [`RawSensor`](#interface-rawsensor)
- [`RotationVector`](#interface-rotationvector)
- [`ScalarReport`](#interface-scalarreport)
- [`ShakeDetector`](#interface-shakedetector)
- [`SignificantMotion`](#interface-significantmotion)
- [`StabilityClassifier`](#interface-stabilityclassifier)
- [`StepCounter`](#interface-stepcounter)
- [`StepDetector`](#interface-stepdetector)
- [`TapDetector`](#interface-tapdetector)
- [`UncalibratedGyroscope`](#interface-uncalibratedgyroscope)
- [`UncalibratedMagnetometer`](#interface-uncalibratedmagnetometer)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`ReportBase`](#interface-reportbase).[`sensorId`](#sensorid-12)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`ReportBase`](#interface-reportbase).[`timestampUs`](#timestampus-11)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

# Interface: Magnetometer

Defined in: [sensors/bno086/reports.ts:227](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L227)

0x03 calibrated magnetic field (Q4).

## Extends

- [`InputReportBase`](#interface-inputreportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`sensorId`](#sensorid-7)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`timestampUs`](#timestampus-6)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`seq`](#seq-5)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`accuracy`](#accuracy-3)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`delayUs`](#delayus-3)

***

### type

```ts
type: "Magnetometer";
```

Defined in: [sensors/bno086/reports.ts:228](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L228)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [sensors/bno086/reports.ts:229](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L229)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [sensors/bno086/reports.ts:230](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L230)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [sensors/bno086/reports.ts:231](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L231)

***

### x

```ts
x: number;
```

Defined in: [sensors/bno086/reports.ts:233](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L233)

µT.

***

### y

```ts
y: number;
```

Defined in: [sensors/bno086/reports.ts:234](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L234)

***

### z

```ts
z: number;
```

Defined in: [sensors/bno086/reports.ts:235](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L235)

# Interface: MotionRequest

Defined in: [sensors/bno086/reports.ts:399](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L399)

0x2B motion request (BNO085 interactive calibration): the motion intent the
hub was last told, and what it asks of the device. Sent when the request
changes, not periodically, whatever the granted rate.

## Extends

- [`InputReportBase`](#interface-inputreportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`sensorId`](#sensorid-7)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`timestampUs`](#timestampus-6)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`seq`](#seq-5)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`accuracy`](#accuracy-3)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`delayUs`](#delayus-3)

***

### type

```ts
type: "MotionRequest";
```

Defined in: [sensors/bno086/reports.ts:400](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L400)

***

### intent

```ts
intent: number;
```

Defined in: [sensors/bno086/reports.ts:401](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L401)

***

### request

```ts
request: number;
```

Defined in: [sensors/bno086/reports.ts:402](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L402)

***

### intentName

```ts
intentName: string;
```

Defined in: [sensors/bno086/reports.ts:403](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L403)

***

### requestName

```ts
requestName: string;
```

Defined in: [sensors/bno086/reports.ts:404](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L404)

***

### wantsStillness

```ts
wantsStillness: boolean;
```

Defined in: [sensors/bno086/reports.ts:406](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L406)

The hub asks the device to stop (the deprecated optional request 2 does not count).

# Interface: PersonalActivityClassifier

Defined in: [sensors/bno086/reports.ts:372](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L372)

0x1E personal activity classifier (see ACTIVITY_NAMES).
`confidences` are 0–100 per state, states 0–9 of the current page.

## Extends

- [`InputReportBase`](#interface-inputreportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`sensorId`](#sensorid-7)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`timestampUs`](#timestampus-6)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`seq`](#seq-5)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`accuracy`](#accuracy-3)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`delayUs`](#delayus-3)

***

### type

```ts
type: "PersonalActivityClassifier";
```

Defined in: [sensors/bno086/reports.ts:373](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L373)

***

### pageNumber

```ts
pageNumber: number;
```

Defined in: [sensors/bno086/reports.ts:374](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L374)

***

### endOfSequence

```ts
endOfSequence: boolean;
```

Defined in: [sensors/bno086/reports.ts:375](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L375)

***

### mostLikelyState

```ts
mostLikelyState: number;
```

Defined in: [sensors/bno086/reports.ts:376](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L376)

***

### confidences

```ts
confidences: number[];
```

Defined in: [sensors/bno086/reports.ts:377](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L377)

***

### mostLikelyName

```ts
mostLikelyName: string;
```

Defined in: [sensors/bno086/reports.ts:378](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L378)

# Interface: ProductId

Defined in: [sensors/bno086/sh2.ts:262](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L262)

Product ID Response (0xF8), 16 bytes. The sensor sends one response per
software component (a BNO085 sends four: the SH-2 application first, then
the bootloader and two support components); resetCause per SH-2 §6.4.5.2,
set on the first only.

## Properties

### resetCause

```ts
resetCause: number;
```

Defined in: [sensors/bno086/sh2.ts:263](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L263)

***

### swVersionMajor

```ts
swVersionMajor: number;
```

Defined in: [sensors/bno086/sh2.ts:264](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L264)

***

### swVersionMinor

```ts
swVersionMinor: number;
```

Defined in: [sensors/bno086/sh2.ts:265](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L265)

***

### swPartNumber

```ts
swPartNumber: number;
```

Defined in: [sensors/bno086/sh2.ts:266](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L266)

***

### swBuildNumber

```ts
swBuildNumber: number;
```

Defined in: [sensors/bno086/sh2.ts:267](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L267)

***

### swVersionPatch

```ts
swVersionPatch: number;
```

Defined in: [sensors/bno086/sh2.ts:268](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L268)

***

### version

```ts
version: string;
```

Defined in: [sensors/bno086/sh2.ts:270](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L270)

"major.minor.patch".

# Interface: RawSensor

Defined in: [sensors/bno086/reports.ts:385](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L385)

0x14/0x15/0x16 raw ADC samples + sensor-clock timestamp (u32 µs).
`temperatureRaw` is populated only for the raw gyroscope.

## Extends

- [`InputReportBase`](#interface-inputreportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`sensorId`](#sensorid-7)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`timestampUs`](#timestampus-6)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`seq`](#seq-5)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`accuracy`](#accuracy-3)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`delayUs`](#delayus-3)

***

### type

```ts
type: "RawSensor";
```

Defined in: [sensors/bno086/reports.ts:386](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L386)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [sensors/bno086/reports.ts:387](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L387)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [sensors/bno086/reports.ts:388](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L388)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [sensors/bno086/reports.ts:389](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L389)

***

### sensorTimestampUs

```ts
sensorTimestampUs: number;
```

Defined in: [sensors/bno086/reports.ts:390](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L390)

***

### temperatureRaw

```ts
temperatureRaw: number;
```

Defined in: [sensors/bno086/reports.ts:391](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L391)

# Interface: ReportBase

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

Base for anything the sensor pushes; `timestampUs` is absolute in the MCU
clock domain (bridge capture time corrected by timebase + delay).

## Extended by

- [`FlushCompleted`](#interface-flushcompleted)
- [`GyroIntegratedRV`](#interface-gyrointegratedrv)
- [`InputReportBase`](#interface-inputreportbase)
- [`UnknownReport`](#interface-unknownreport)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

# Interface: RotationVector

Defined in: [sensors/bno086/reports.ts:274](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L274)

Quaternion reports 0x05/0x08/0x09/0x28/0x29 (unit quaternion, Q14).
`accuracyRaw` (Q12, radians) is present only for 0x05/0x09/0x28.

## Extends

- [`InputReportBase`](#interface-inputreportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`sensorId`](#sensorid-7)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`timestampUs`](#timestampus-6)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`seq`](#seq-5)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`accuracy`](#accuracy-3)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`delayUs`](#delayus-3)

***

### type

```ts
type: "RotationVector";
```

Defined in: [sensors/bno086/reports.ts:275](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L275)

***

### iRaw

```ts
iRaw: number;
```

Defined in: [sensors/bno086/reports.ts:276](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L276)

***

### jRaw

```ts
jRaw: number;
```

Defined in: [sensors/bno086/reports.ts:277](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L277)

***

### kRaw

```ts
kRaw: number;
```

Defined in: [sensors/bno086/reports.ts:278](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L278)

***

### realRaw

```ts
realRaw: number;
```

Defined in: [sensors/bno086/reports.ts:279](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L279)

***

### accuracyRaw

```ts
accuracyRaw: number | null;
```

Defined in: [sensors/bno086/reports.ts:280](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L280)

***

### i

```ts
i: number;
```

Defined in: [sensors/bno086/reports.ts:281](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L281)

***

### j

```ts
j: number;
```

Defined in: [sensors/bno086/reports.ts:282](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L282)

***

### k

```ts
k: number;
```

Defined in: [sensors/bno086/reports.ts:283](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L283)

***

### real

```ts
real: number;
```

Defined in: [sensors/bno086/reports.ts:284](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L284)

***

### accuracyRad

```ts
accuracyRad: number | null;
```

Defined in: [sensors/bno086/reports.ts:286](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L286)

Estimated heading accuracy in radians (null for game variants).

# Interface: ScalarReport

Defined in: [sensors/bno086/reports.ts:311](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L311)

Environment reports 0x0A–0x0E: single value, Q from Q_POINTS.

## Extends

- [`InputReportBase`](#interface-inputreportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`sensorId`](#sensorid-7)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`timestampUs`](#timestampus-6)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`seq`](#seq-5)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`accuracy`](#accuracy-3)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`delayUs`](#delayus-3)

***

### type

```ts
type: "ScalarReport";
```

Defined in: [sensors/bno086/reports.ts:312](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L312)

***

### valueRaw

```ts
valueRaw: number;
```

Defined in: [sensors/bno086/reports.ts:313](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L313)

***

### value

```ts
value: number;
```

Defined in: [sensors/bno086/reports.ts:314](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L314)

# Interface: SensorMetadata

Defined in: [sensors/bno086/sh2.ts:712](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L712)

Parsed sensor metadata FRS record; `rawWords` is authoritative.

Field packing follows the sh2 reference driver (revision-gated fields are
0 when the record predates them).

## Properties

### meVersion

```ts
meVersion: number;
```

Defined in: [sensors/bno086/sh2.ts:713](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L713)

***

### mhVersion

```ts
mhVersion: number;
```

Defined in: [sensors/bno086/sh2.ts:714](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L714)

***

### shVersion

```ts
shVersion: number;
```

Defined in: [sensors/bno086/sh2.ts:715](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L715)

***

### rangeRaw

```ts
rangeRaw: number;
```

Defined in: [sensors/bno086/sh2.ts:717](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L717)

Same units & Q point as the sensor's reports.

***

### resolutionRaw

```ts
resolutionRaw: number;
```

Defined in: [sensors/bno086/sh2.ts:718](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L718)

***

### revision

```ts
revision: number;
```

Defined in: [sensors/bno086/sh2.ts:719](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L719)

***

### powerMaQ10

```ts
powerMaQ10: number;
```

Defined in: [sensors/bno086/sh2.ts:721](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L721)

mA in Q10.

***

### minPeriodUs

```ts
minPeriodUs: number;
```

Defined in: [sensors/bno086/sh2.ts:722](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L722)

***

### maxPeriodUs

```ts
maxPeriodUs: number;
```

Defined in: [sensors/bno086/sh2.ts:724](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L724)

Revision >= 4 only.

***

### fifoMax

```ts
fifoMax: number;
```

Defined in: [sensors/bno086/sh2.ts:725](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L725)

***

### fifoReserved

```ts
fifoReserved: number;
```

Defined in: [sensors/bno086/sh2.ts:726](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L726)

***

### batchBufferBytes

```ts
batchBufferBytes: number;
```

Defined in: [sensors/bno086/sh2.ts:727](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L727)

***

### qPoint1

```ts
qPoint1: number;
```

Defined in: [sensors/bno086/sh2.ts:728](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L728)

***

### qPoint2

```ts
qPoint2: number;
```

Defined in: [sensors/bno086/sh2.ts:729](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L729)

***

### qPoint3

```ts
qPoint3: number;
```

Defined in: [sensors/bno086/sh2.ts:731](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L731)

Revision >= 3 only.

***

### rawWords

```ts
rawWords: number[];
```

Defined in: [sensors/bno086/sh2.ts:732](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L732)

# Interface: ShakeDetector

Defined in: [sensors/bno086/reports.ts:354](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L354)

0x19 shake detector; bits 0/1/2 = X/Y/Z shake.

## Extends

- [`InputReportBase`](#interface-inputreportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`sensorId`](#sensorid-7)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`timestampUs`](#timestampus-6)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`seq`](#seq-5)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`accuracy`](#accuracy-3)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`delayUs`](#delayus-3)

***

### type

```ts
type: "ShakeDetector";
```

Defined in: [sensors/bno086/reports.ts:355](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L355)

***

### flags

```ts
flags: number;
```

Defined in: [sensors/bno086/reports.ts:356](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L356)

# Interface: SignificantMotion

Defined in: [sensors/bno086/reports.ts:341](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L341)

0x12 significant motion (1 = motion detected; sensor auto-disables).

## Extends

- [`InputReportBase`](#interface-inputreportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`sensorId`](#sensorid-7)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`timestampUs`](#timestampus-6)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`seq`](#seq-5)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`accuracy`](#accuracy-3)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`delayUs`](#delayus-3)

***

### type

```ts
type: "SignificantMotion";
```

Defined in: [sensors/bno086/reports.ts:342](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L342)

***

### motion

```ts
motion: number;
```

Defined in: [sensors/bno086/reports.ts:343](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L343)

# Interface: StabilityClassifier

Defined in: [sensors/bno086/reports.ts:347](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L347)

0x13 stability classification (see STABILITY_NAMES).

## Extends

- [`InputReportBase`](#interface-inputreportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`sensorId`](#sensorid-7)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`timestampUs`](#timestampus-6)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`seq`](#seq-5)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`accuracy`](#accuracy-3)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`delayUs`](#delayus-3)

***

### type

```ts
type: "StabilityClassifier";
```

Defined in: [sensors/bno086/reports.ts:348](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L348)

***

### classification

```ts
classification: number;
```

Defined in: [sensors/bno086/reports.ts:349](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L349)

***

### name

```ts
name: string;
```

Defined in: [sensors/bno086/reports.ts:350](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L350)

# Interface: StepCounter

Defined in: [sensors/bno086/reports.ts:328](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L328)

0x11 step counter. NOTE: latency u32 µs at bytes 4–7, steps u16 at bytes
8–9 per SH-2; the vendor tool's `latency(2)+steps(2)` comment is wrong.

## Extends

- [`InputReportBase`](#interface-inputreportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`sensorId`](#sensorid-7)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`timestampUs`](#timestampus-6)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`seq`](#seq-5)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`accuracy`](#accuracy-3)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`delayUs`](#delayus-3)

***

### type

```ts
type: "StepCounter";
```

Defined in: [sensors/bno086/reports.ts:329](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L329)

***

### latencyUs

```ts
latencyUs: number;
```

Defined in: [sensors/bno086/reports.ts:330](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L330)

***

### steps

```ts
steps: number;
```

Defined in: [sensors/bno086/reports.ts:331](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L331)

# Interface: StepDetector

Defined in: [sensors/bno086/reports.ts:335](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L335)

0x18 step detector; latency from step event to report, µs.

## Extends

- [`InputReportBase`](#interface-inputreportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`sensorId`](#sensorid-7)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`timestampUs`](#timestampus-6)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`seq`](#seq-5)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`accuracy`](#accuracy-3)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`delayUs`](#delayus-3)

***

### type

```ts
type: "StepDetector";
```

Defined in: [sensors/bno086/reports.ts:336](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L336)

***

### latencyUs

```ts
latencyUs: number;
```

Defined in: [sensors/bno086/reports.ts:337](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L337)

# Interface: TapDetector

Defined in: [sensors/bno086/reports.ts:318](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L318)

0x10 tap detector; `flags` bit 6 = double tap, bits 0–5 axis/sign.

## Extends

- [`InputReportBase`](#interface-inputreportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`sensorId`](#sensorid-7)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`timestampUs`](#timestampus-6)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`seq`](#seq-5)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`accuracy`](#accuracy-3)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`delayUs`](#delayus-3)

***

### type

```ts
type: "TapDetector";
```

Defined in: [sensors/bno086/reports.ts:319](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L319)

***

### flags

```ts
flags: number;
```

Defined in: [sensors/bno086/reports.ts:320](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L320)

***

### doubleTap

```ts
doubleTap: boolean;
```

Defined in: [sensors/bno086/reports.ts:321](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L321)

# Interface: UncalibratedGyroscope

Defined in: [sensors/bno086/reports.ts:239](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L239)

0x07 uncalibrated gyroscope + bias estimate (all Q9, rad/s).

## Extends

- [`InputReportBase`](#interface-inputreportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`sensorId`](#sensorid-7)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`timestampUs`](#timestampus-6)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`seq`](#seq-5)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`accuracy`](#accuracy-3)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`delayUs`](#delayus-3)

***

### type

```ts
type: "UncalibratedGyroscope";
```

Defined in: [sensors/bno086/reports.ts:240](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L240)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [sensors/bno086/reports.ts:241](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L241)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [sensors/bno086/reports.ts:242](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L242)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [sensors/bno086/reports.ts:243](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L243)

***

### biasXRaw

```ts
biasXRaw: number;
```

Defined in: [sensors/bno086/reports.ts:244](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L244)

***

### biasYRaw

```ts
biasYRaw: number;
```

Defined in: [sensors/bno086/reports.ts:245](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L245)

***

### biasZRaw

```ts
biasZRaw: number;
```

Defined in: [sensors/bno086/reports.ts:246](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L246)

***

### x

```ts
x: number;
```

Defined in: [sensors/bno086/reports.ts:247](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L247)

***

### y

```ts
y: number;
```

Defined in: [sensors/bno086/reports.ts:248](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L248)

***

### z

```ts
z: number;
```

Defined in: [sensors/bno086/reports.ts:249](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L249)

***

### bias

```ts
bias: [number, number, number];
```

Defined in: [sensors/bno086/reports.ts:251](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L251)

rad/s.

# Interface: UncalibratedMagnetometer

Defined in: [sensors/bno086/reports.ts:255](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L255)

0x0F uncalibrated magnetic field + hard-iron bias (all Q4, µT).

## Extends

- [`InputReportBase`](#interface-inputreportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`sensorId`](#sensorid-7)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`timestampUs`](#timestampus-6)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L195)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`seq`](#seq-5)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`accuracy`](#accuracy-3)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:199](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L199)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](#interface-inputreportbase).[`delayUs`](#delayus-3)

***

### type

```ts
type: "UncalibratedMagnetometer";
```

Defined in: [sensors/bno086/reports.ts:256](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L256)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [sensors/bno086/reports.ts:257](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L257)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [sensors/bno086/reports.ts:258](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L258)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [sensors/bno086/reports.ts:259](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L259)

***

### biasXRaw

```ts
biasXRaw: number;
```

Defined in: [sensors/bno086/reports.ts:260](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L260)

***

### biasYRaw

```ts
biasYRaw: number;
```

Defined in: [sensors/bno086/reports.ts:261](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L261)

***

### biasZRaw

```ts
biasZRaw: number;
```

Defined in: [sensors/bno086/reports.ts:262](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L262)

***

### x

```ts
x: number;
```

Defined in: [sensors/bno086/reports.ts:263](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L263)

***

### y

```ts
y: number;
```

Defined in: [sensors/bno086/reports.ts:264](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L264)

***

### z

```ts
z: number;
```

Defined in: [sensors/bno086/reports.ts:265](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L265)

***

### bias

```ts
bias: [number, number, number];
```

Defined in: [sensors/bno086/reports.ts:267](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L267)

µT.

# Interface: UnknownReport

Defined in: [sensors/bno086/reports.ts:422](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L422)

Unrecognized report ID: raw bytes from the ID to end of cargo (the length
is unknowable, so parsing stops here).

## Extends

- [`ReportBase`](#interface-reportbase)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`ReportBase`](#interface-reportbase).[`sensorId`](#sensorid-12)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L189)

#### Inherited from

[`ReportBase`](#interface-reportbase).[`timestampUs`](#timestampus-11)

***

### type

```ts
type: "UnknownReport";
```

Defined in: [sensors/bno086/reports.ts:423](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L423)

***

### data

```ts
data: Uint8Array;
```

Defined in: [sensors/bno086/reports.ts:424](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L424)

# Type Alias: InputReport

```ts
type InputReport = 
  | Acceleration
  | Gyroscope
  | Magnetometer
  | UncalibratedGyroscope
  | UncalibratedMagnetometer
  | RotationVector
  | ScalarReport
  | TapDetector
  | StepCounter
  | StepDetector
  | SignificantMotion
  | StabilityClassifier
  | ShakeDetector
  | GenericEvent
  | PersonalActivityClassifier
  | RawSensor
  | MotionRequest;
```

Defined in: [sensors/bno086/reports.ts:428](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L428)

Channel-3/4 typed reports (share the SH-2 input header fields).

# Type Alias: Report

```ts
type Report = 
  | InputReport
  | GyroIntegratedRV
  | FlushCompleted
  | UnknownReport;
```

Defined in: [sensors/bno086/reports.ts:448](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L448)

Anything the sensor pushes.

# Variable: ACTIVITY\_NAMES

```ts
const ACTIVITY_NAMES: Readonly<Record<number, string>>;
```

Defined in: [sensors/bno086/reports.ts:165](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L165)

# Variable: BUSY\_BACKOFF\_MS

```ts
const BUSY_BACKOFF_MS: 200 = 200;
```

Defined in: [protocol/bno086.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L32)

# Variable: MOTION\_INTENT\_NAMES

```ts
const MOTION_INTENT_NAMES: Readonly<Record<number, string>>;
```

Defined in: [sensors/bno086/reports.ts:150](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L150)

# Variable: MOTION\_REQUEST\_NAMES

```ts
const MOTION_REQUEST_NAMES: Readonly<Record<number, string>>;
```

Defined in: [sensors/bno086/reports.ts:143](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L143)

SH-2 §6.5.45.2. Value 2 (stay stationary, optional) is deprecated by CEVA:
"ignore this request".

# Variable: STABILITY\_NAMES

```ts
const STABILITY_NAMES: Readonly<Record<number, string>>;
```

Defined in: [sensors/bno086/reports.ts:158](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L158)

# Variable: TX\_SLOTS

```ts
const TX_SLOTS: 2 = 2;
```

Defined in: [protocol/bno086.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L30)

# Variable: TX\_SLOT\_SIZE

```ts
const TX_SLOT_SIZE: 64 = 64;
```

Defined in: [protocol/bno086.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L31)

