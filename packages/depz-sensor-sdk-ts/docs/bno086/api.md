# BNO086 (IMU) — API reference

The public API for the BNO086 IMU: the `Bno086` device plus the SHTP /
SH-2 protocol and report types. Discovery, device-base, transport and
other cross-sensor symbols live in the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Enumerations

- [SensorId](enumerations/SensorId.md)
- [ControlReport](enumerations/ControlReport.md)
- [Sh2Command](enumerations/Sh2Command.md)
- [OscillatorType](enumerations/OscillatorType.md)
- [ErrorSource](enumerations/ErrorSource.md)
- [TareBasis](enumerations/TareBasis.md)
- [TareAxis](enumerations/TareAxis.md)
- [FrsRecordId](enumerations/FrsRecordId.md)
- [FrsStatus](enumerations/FrsStatus.md)
- [FrsWriteStatus](enumerations/FrsWriteStatus.md)
- [ShtpChannel](enumerations/ShtpChannel.md)

## Classes

- [Bno086](classes/Bno086.md)
- [Sh2Error](classes/Sh2Error.md)
- [FrsReadSession](classes/FrsReadSession.md)
- [FrsWriteSession](classes/FrsWriteSession.md)
- [ShtpLayer](classes/ShtpLayer.md)

## Interfaces

- [CalibrationConfig](interfaces/CalibrationConfig.md)
- [Bno086Options](interfaces/Bno086Options.md)
- [EnableOptions](interfaces/EnableOptions.md)
- [ReportBase](interfaces/ReportBase.md)
- [InputReportBase](interfaces/InputReportBase.md)
- [Acceleration](interfaces/Acceleration.md)
- [Gyroscope](interfaces/Gyroscope.md)
- [Magnetometer](interfaces/Magnetometer.md)
- [UncalibratedGyroscope](interfaces/UncalibratedGyroscope.md)
- [UncalibratedMagnetometer](interfaces/UncalibratedMagnetometer.md)
- [RotationVector](interfaces/RotationVector.md)
- [GyroIntegratedRV](interfaces/GyroIntegratedRV.md)
- [ScalarReport](interfaces/ScalarReport.md)
- [TapDetector](interfaces/TapDetector.md)
- [StepCounter](interfaces/StepCounter.md)
- [StepDetector](interfaces/StepDetector.md)
- [SignificantMotion](interfaces/SignificantMotion.md)
- [StabilityClassifier](interfaces/StabilityClassifier.md)
- [ShakeDetector](interfaces/ShakeDetector.md)
- [GenericEvent](interfaces/GenericEvent.md)
- [PersonalActivityClassifier](interfaces/PersonalActivityClassifier.md)
- [RawSensor](interfaces/RawSensor.md)
- [UnknownReport](interfaces/UnknownReport.md)
- [ErrorRecord](interfaces/ErrorRecord.md)
- [Counts](interfaces/Counts.md)
- [FeatureResponse](interfaces/FeatureResponse.md)
- [ProductId](interfaces/ProductId.md)
- [CommandResponse](interfaces/CommandResponse.md)
- [FrsReadResponse](interfaces/FrsReadResponse.md)
- [FrsWriteResponse](interfaces/FrsWriteResponse.md)
- [SensorMetadata](interfaces/SensorMetadata.md)
- [ShtpHeader](interfaces/ShtpHeader.md)
- [ShtpCargo](interfaces/ShtpCargo.md)

## Type Aliases

- [InputReport](type-aliases/InputReport.md)
- [Report](type-aliases/Report.md)

## Variables

- [RATE\_LOW\_FACTOR](variables/RATE_LOW_FACTOR.md)
- [RATE\_HIGH\_FACTOR](variables/RATE_HIGH_FACTOR.md)
- [BASE\_TIMESTAMP\_REF](variables/BASE_TIMESTAMP_REF.md)
- [TIMESTAMP\_REBASE](variables/TIMESTAMP_REBASE.md)
- [Q\_POINTS](variables/Q_POINTS.md)
- [RV\_ACCURACY\_Q](variables/RV_ACCURACY_Q.md)
- [GYRO\_RV\_ANGVEL\_Q](variables/GYRO_RV_ANGVEL_Q.md)
- [REPORT\_LENGTHS](variables/REPORT_LENGTHS.md)
- [STABILITY\_NAMES](variables/STABILITY_NAMES.md)
- [ACTIVITY\_NAMES](variables/ACTIVITY_NAMES.md)
- [COUNTS\_GET](variables/COUNTS_GET.md)
- [COUNTS\_CLEAR](variables/COUNTS_CLEAR.md)
- [ME\_CAL\_GET](variables/ME_CAL_GET.md)
- [METADATA\_RECORDS](variables/METADATA_RECORDS.md)
- [SHTP\_HEADER\_SIZE](variables/SHTP_HEADER_SIZE.md)
- [LENGTH\_MASK](variables/LENGTH_MASK.md)
- [CONTINUATION\_BIT](variables/CONTINUATION_BIT.md)
- [NUM\_CHANNELS](variables/NUM_CHANNELS.md)
- [MAX\_TX\_FRAME](variables/MAX_TX_FRAME.md)

## Functions

- [parseInputCargo](functions/parseInputCargo.md)
- [parseGyroRvCargo](functions/parseGyroRvCargo.md)
- [countsGetParams](functions/countsGetParams.md)
- [countsClearParams](functions/countsClearParams.md)
- [errorsParams](functions/errorsParams.md)
- [errorRecordFromResponse](functions/errorRecordFromResponse.md)
- [buildSetFeature](functions/buildSetFeature.md)
- [buildGetFeatureRequest](functions/buildGetFeatureRequest.md)
- [unpackFeatureResponse](functions/unpackFeatureResponse.md)
- [buildProductIdRequest](functions/buildProductIdRequest.md)
- [unpackProductId](functions/unpackProductId.md)
- [buildCommandRequest](functions/buildCommandRequest.md)
- [unpackCommandResponse](functions/unpackCommandResponse.md)
- [tareNowParams](functions/tareNowParams.md)
- [persistTareParams](functions/persistTareParams.md)
- [setReorientationParams](functions/setReorientationParams.md)
- [meCalibrationParams](functions/meCalibrationParams.md)
- [periodicDcdParams](functions/periodicDcdParams.md)
- [buildFrsReadRequest](functions/buildFrsReadRequest.md)
- [unpackFrsReadResponse](functions/unpackFrsReadResponse.md)
- [buildFrsWriteRequest](functions/buildFrsWriteRequest.md)
- [buildFrsWriteData](functions/buildFrsWriteData.md)
- [unpackFrsWriteResponse](functions/unpackFrsWriteResponse.md)
- [sensorMetadataFromWords](functions/sensorMetadataFromWords.md)
- [packShtpHeader](functions/packShtpHeader.md)
- [unpackShtpHeader](functions/unpackShtpHeader.md)
- [buildFrame](functions/buildFrame.md)
- [fragmentCargo](functions/fragmentCargo.md)

# Class: Bno086

Defined in: [src/sensors/bno086/bno086.ts:127](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L127)

BNO086 device: enable SH-2 sensors, stream typed reports.

Typical use:

    const imu = new Bno086(transport);
    await imu.open();
    await imu.enable(SensorId.RotationVector, 100);
    for await (const r of imu.reports()) { ... }

Callbacks run on the read-pump context — never await blocking device
methods (enable/tare/...) from inside one.

## Extends

- `DepzDevice`

## Constructors

### Constructor

```ts
new Bno086(transport, opts?): Bno086;
```

Defined in: [src/sensors/bno086/bno086.ts:143](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L143)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | [`Bno086Options`](../interfaces/Bno086Options.md) |

#### Returns

`Bno086`

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

### busyRetries

```ts
busyRetries: number = 5;
```

Defined in: [src/sensors/bno086/bno086.ts:129](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L129)

SEND_SHTP_PACKET attempts before giving up.

***

### busyBackoffMs

```ts
busyBackoffMs: number = BUSY_BACKOFF_MS;
```

Defined in: [src/sensors/bno086/bno086.ts:131](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L131)

>= 200 ms per the bridge spec (contract 05 §2).

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

Defined in: [src/device/device.ts:474](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L474)

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

Defined in: [src/sensors/bno086/bno086.ts:402](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L402)

Raw SHTP channel-0 advertisement bytes seen since open/reset.

##### Returns

`Uint8Array`

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
DepzDevice.registerStream
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
DepzDevice.onEvent
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
DepzDevice.request
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
DepzDevice.expectReport
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
DepzDevice.expectText
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
DepzDevice.getDeviceName
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
DepzDevice.getSoftwareName
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
DepzDevice.getSerialNumber
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
DepzDevice.identify
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
DepzDevice.readMcuTemperature
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
DepzDevice.syncTime
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
DepzDevice.toHostTimeUs
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
DepzDevice.getReportPayloadCrc
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
DepzDevice.setReportPayloadCrc
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
DepzDevice.getSyncPin
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
DepzDevice.setSyncPin
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
DepzDevice.reset
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
DepzDevice.enterBootloaderMode
```

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [src/sensors/bno086/bno086.ts:151](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L151)

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

Defined in: [src/sensors/bno086/bno086.ts:375](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L375)

Hard-reset the sensor via nRST (0x32). All SHTP state (seq counters,
partial cargos) and cached features restart from zero.

The hub's RPT_STATUS OK ack for the 0x32 command is treated as the reset
confirmation. The SH-2 executable-channel reset-complete (which the BNO08X
SH-2 spec would emit) is only waited for best-effort: older firmware
(≤ v0.95) did **not** emit it (ERRATA E9 in contracts/ERRATA.md, now fixed
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

Defined in: [src/sensors/bno086/bno086.ts:397](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L397)

Pulse WAKE (PS0): wakes the sensor from sleep, no state loss.

#### Returns

`Promise`\<`void`\>

***

### productId()

```ts
productId(timeoutMs?): Promise<ProductId>;
```

Defined in: [src/sensors/bno086/bno086.ts:416](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L416)

Product ID Request/Response round trip (first responding subsystem).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`ProductId`](../interfaces/ProductId.md)\>

***

### enable()

```ts
enable(
   sensor, 
   hz?, 
opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:435](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L435)

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
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### disable()

```ts
disable(sensor): Promise<void>;
```

Defined in: [src/sensors/bno086/bno086.ts:474](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L474)

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

Defined in: [src/sensors/bno086/bno086.ts:480](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L480)

Get Feature Request/Response round trip for `sensor`.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensor` | `number` | `undefined` |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md)\>

***

### enableRotationVector()

```ts
enableRotationVector(hz?, opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:491](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L491)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `100` |
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) | `undefined` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### enableGameRotationVector()

```ts
enableGameRotationVector(hz?, opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:495](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L495)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `100` |
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) | `undefined` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### enableAccelerometer()

```ts
enableAccelerometer(hz?, opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:499](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L499)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `100` |
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) | `undefined` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### enableGyroscope()

```ts
enableGyroscope(hz?, opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:503](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L503)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `100` |
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) | `undefined` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### enableMagnetometer()

```ts
enableMagnetometer(hz?, opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:507](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L507)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `50` |
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) | `undefined` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### enableLinearAcceleration()

```ts
enableLinearAcceleration(hz?, opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:511](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L511)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `100` |
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) | `undefined` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### enableGravity()

```ts
enableGravity(hz?, opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:515](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L515)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `100` |
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) | `undefined` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### enableGyroIntegratedRv()

```ts
enableGyroIntegratedRv(hz?, opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:519](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L519)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `400` |
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) | `undefined` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### onReport()

```ts
onReport(cb, sensors?): () => void;
```

Defined in: [src/sensors/bno086/bno086.ts:529](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L529)

Subscribe to typed sensor reports (read-pump context; do not block).
`sensors` filters by SensorId. Returns an unsubscribe fn.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`r`) => `void` |
| `sensors?` | `number` \| `Iterable`\<`number`, `any`, `any`\> \| `null` |

#### Returns

() => `void`

***

### reports()

```ts
reports(sensors?, maxsize?): StreamQueue<Report>;
```

Defined in: [src/sensors/bno086/bno086.ts:542](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L542)

Async iterator over typed reports — bounded, drop-oldest (contract 07
§3). Subscribes eagerly — reports emitted after this call are never
missed.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensors?` | `number` \| `Iterable`\<`number`, `any`, `any`\> \| `null` | `undefined` |
| `maxsize?` | `number` | `1024` |

#### Returns

`StreamQueue`\<[`Report`](../type-aliases/Report.md)\>

***

### tareNow()

```ts
tareNow(axes?, basis?): Promise<void>;
```

Defined in: [src/sensors/bno086/bno086.ts:555](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L555)

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

Defined in: [src/sensors/bno086/bno086.ts:560](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L560)

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

Defined in: [src/sensors/bno086/bno086.ts:568](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L568)

Set the runtime reorientation quaternion (Q14 on the wire; all zeros
clears). No response per SH-2.

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

Defined in: [src/sensors/bno086/bno086.ts:573](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L573)

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

Defined in: [src/sensors/bno086/bno086.ts:589](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L589)

Read back which ME calibrations are running.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`CalibrationConfig`](../interfaces/CalibrationConfig.md)\>

***

### saveDcd()

```ts
saveDcd(timeoutMs?): Promise<void>;
```

Defined in: [src/sensors/bno086/bno086.ts:604](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L604)

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

Defined in: [src/sensors/bno086/bno086.ts:612](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L612)

Enable/disable the hub's periodic DCD autosave (no response).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `enable` | `boolean` |

#### Returns

`Promise`\<`void`\>

***

### frsRead()

```ts
frsRead(recordId, timeoutMs?): Promise<number[]>;
```

Defined in: [src/sensors/bno086/bno086.ts:619](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L619)

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

Defined in: [src/sensors/bno086/bno086.ts:636](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L636)

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

Defined in: [src/sensors/bno086/bno086.ts:658](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L658)

Read + parse the sensor's FRS metadata record.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensor` | `number` | `undefined` |
| `timeoutMs` | `number` | `FRS_TIMEOUT_MS` |

#### Returns

`Promise`\<[`SensorMetadata`](../interfaces/SensorMetadata.md)\>

***

### getOscillatorType()

```ts
getOscillatorType(timeoutMs?): Promise<OscillatorType>;
```

Defined in: [src/sensors/bno086/bno086.ts:671](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L671)

Get Oscillator Type (command 0x0A). r[0] is the type directly.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`OscillatorType`](../enumerations/OscillatorType.md)\>

***

### clearDcdAndReset()

```ts
clearDcdAndReset(timeoutMs?): Promise<void>;
```

Defined in: [src/sensors/bno086/bno086.ts:681](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L681)

Clear the in-RAM dynamic calibration and reset the sensor (command 0x0B).
There is no command response — the hub resets, so this waits for the
executable reset-complete like hardwareReset().

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

Defined in: [src/sensors/bno086/bno086.ts:708](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L708)

Read the error queue (command 0x01), filtered to `severity` or greater.
Records stream until one with source == 255 (no more).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `severity` | `number` | `0` |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`ErrorRecord`](../interfaces/ErrorRecord.md)[]\>

***

### getCounts()

```ts
getCounts(sensor, timeoutMs?): Promise<Counts>;
```

Defined in: [src/sensors/bno086/bno086.ts:726](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L726)

Read a sensor's event counts (command 0x02). The hub answers with two
responses (responseSeq 0 then 1).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensor` | `number` | `undefined` |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`Counts`](../interfaces/Counts.md)\>

***

### clearCounts()

```ts
clearCounts(sensor, timeoutMs?): Promise<void>;
```

Defined in: [src/sensors/bno086/bno086.ts:751](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L751)

Clear a sensor's event counts (command 0x02, subcommand 1).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensor` | `number` | `undefined` |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<`void`\>

# Class: FrsReadSession

Defined in: [src/sensors/bno086/sh2.ts:518](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L518)

Multi-packet FRS read state machine (pure — feed parsed responses).

Usage: send `request()`, then `feed()` every 0xF3 for this record until it
returns true; `words` holds the record. Error statuses throw.

## Constructors

### Constructor

```ts
new FrsReadSession(frsType): FrsReadSession;
```

Defined in: [src/sensors/bno086/sh2.ts:523](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L523)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `frsType` | `number` |

#### Returns

`FrsReadSession`

## Properties

### frsType

```ts
readonly frsType: number;
```

Defined in: [src/sensors/bno086/sh2.ts:519](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L519)

***

### words

```ts
words: number[] = [];
```

Defined in: [src/sensors/bno086/sh2.ts:520](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L520)

***

### done

```ts
done: boolean = false;
```

Defined in: [src/sensors/bno086/sh2.ts:521](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L521)

## Methods

### request()

```ts
request(): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:527](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L527)

#### Returns

`Uint8Array`

***

### feed()

```ts
feed(resp): boolean;
```

Defined in: [src/sensors/bno086/sh2.ts:531](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L531)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `resp` | [`FrsReadResponse`](../interfaces/FrsReadResponse.md) |

#### Returns

`boolean`

# Class: FrsWriteSession

Defined in: [src/sensors/bno086/sh2.ts:566](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L566)

Multi-packet FRS write state machine (pure).

Usage: send `request()`; then for every 0xF5 call `feed()` — it returns
the next Write Data payload to send, or null; `done` flips on
WriteCompleted. Error statuses throw.

## Constructors

### Constructor

```ts
new FrsWriteSession(frsType, words): FrsWriteSession;
```

Defined in: [src/sensors/bno086/sh2.ts:572](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L572)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `frsType` | `number` |
| `words` | `number`[] |

#### Returns

`FrsWriteSession`

## Properties

### frsType

```ts
readonly frsType: number;
```

Defined in: [src/sensors/bno086/sh2.ts:567](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L567)

***

### words

```ts
readonly words: number[];
```

Defined in: [src/sensors/bno086/sh2.ts:568](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L568)

***

### offset

```ts
offset: number = 0;
```

Defined in: [src/sensors/bno086/sh2.ts:569](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L569)

***

### done

```ts
done: boolean = false;
```

Defined in: [src/sensors/bno086/sh2.ts:570](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L570)

## Methods

### request()

```ts
request(): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:577](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L577)

#### Returns

`Uint8Array`

***

### feed()

```ts
feed(resp): Uint8Array<ArrayBufferLike> | null;
```

Defined in: [src/sensors/bno086/sh2.ts:589](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L589)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `resp` | [`FrsWriteResponse`](../interfaces/FrsWriteResponse.md) |

#### Returns

`Uint8Array`\<`ArrayBufferLike`\> \| `null`

# Class: Sh2Error

Defined in: [src/sensors/bno086/sh2.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L14)

SH-2 level failure (bad response status, FRS error, ...).

## Extends

- `DepzError`

## Constructors

### Constructor

```ts
new Sh2Error(message?): Sh2Error;
```

Defined in: [src/errors.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L8)

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

# Class: ShtpLayer

Defined in: [src/sensors/bno086/shtp.ts:159](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L159)

Per-channel TX sequence counters + RX cargo reassembly.

Feed every inbound frame (the RPT_DATA payload after cmd/timestamp) to
`feed()`; it returns a completed ShtpCargo or null. Build outbound frames
with `nextFrame()` which consumes the channel's TX seq. The device layer
serializes access.

## Constructors

### Constructor

```ts
new ShtpLayer(): ShtpLayer;
```

#### Returns

`ShtpLayer`

## Properties

### discarded

```ts
discarded: number = 0;
```

Defined in: [src/sensors/bno086/shtp.ts:161](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L161)

Incomplete cargos thrown away.

## Methods

### nextFrame()

```ts
nextFrame(channel, payload): Uint8Array;
```

Defined in: [src/sensors/bno086/shtp.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L174)

Build a single-fragment frame, consuming the channel's TX seq.

Host-side cargos always fit one MCU slot (control payloads are <= 21
bytes); larger payloads are a caller bug.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `channel` | `number` |
| `payload` | `Uint8Array` |

#### Returns

`Uint8Array`

***

### txSeq()

```ts
txSeq(channel): number;
```

Defined in: [src/sensors/bno086/shtp.ts:183](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L183)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `channel` | `number` |

#### Returns

`number`

***

### feed()

```ts
feed(frame): ShtpCargo | null;
```

Defined in: [src/sensors/bno086/shtp.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L197)

Consume one inbound frame; return the cargo when complete.

Rules (contract 05 §3): a non-continuation fragment starts a new cargo
(discarding any partial one on that channel); a continuation without a
cargo in progress is dropped; the cargo completes when the accumulated
bytes reach the first fragment's advertised total.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `frame` | `Uint8Array` |

#### Returns

[`ShtpCargo`](../interfaces/ShtpCargo.md) \| `null`

***

### reset()

```ts
reset(): void;
```

Defined in: [src/sensors/bno086/shtp.ts:240](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L240)

Forget all TX seq counters and partial cargos (sensor reset).

#### Returns

`void`

# Enumeration: ControlReport

Defined in: [src/sensors/bno086/sh2.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L17)

Report IDs on SHTP channel 2 (control).

## Enumeration Members

### CommandResponse

```ts
CommandResponse: 241;
```

Defined in: [src/sensors/bno086/sh2.ts:18](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L18)

***

### CommandRequest

```ts
CommandRequest: 242;
```

Defined in: [src/sensors/bno086/sh2.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L19)

***

### FrsReadResponse

```ts
FrsReadResponse: 243;
```

Defined in: [src/sensors/bno086/sh2.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L20)

***

### FrsReadRequest

```ts
FrsReadRequest: 244;
```

Defined in: [src/sensors/bno086/sh2.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L21)

***

### FrsWriteResponse

```ts
FrsWriteResponse: 245;
```

Defined in: [src/sensors/bno086/sh2.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L22)

***

### FrsWriteData

```ts
FrsWriteData: 246;
```

Defined in: [src/sensors/bno086/sh2.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L23)

***

### FrsWriteRequest

```ts
FrsWriteRequest: 247;
```

Defined in: [src/sensors/bno086/sh2.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L24)

***

### ProductIdResponse

```ts
ProductIdResponse: 248;
```

Defined in: [src/sensors/bno086/sh2.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L25)

***

### ProductIdRequest

```ts
ProductIdRequest: 249;
```

Defined in: [src/sensors/bno086/sh2.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L26)

***

### GetFeatureResponse

```ts
GetFeatureResponse: 252;
```

Defined in: [src/sensors/bno086/sh2.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L27)

***

### SetFeatureCommand

```ts
SetFeatureCommand: 253;
```

Defined in: [src/sensors/bno086/sh2.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L28)

***

### GetFeatureRequest

```ts
GetFeatureRequest: 254;
```

Defined in: [src/sensors/bno086/sh2.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L29)

# Enumeration: ErrorSource

Defined in: [src/sensors/bno086/sh2.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L53)

`source` field of an error record (SH-2 §6.4.1).

## Enumeration Members

### MotionEngine

```ts
MotionEngine: 1;
```

Defined in: [src/sensors/bno086/sh2.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L54)

***

### MotionHub

```ts
MotionHub: 2;
```

Defined in: [src/sensors/bno086/sh2.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L55)

***

### SensorHub

```ts
SensorHub: 3;
```

Defined in: [src/sensors/bno086/sh2.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L56)

***

### Chip

```ts
Chip: 4;
```

Defined in: [src/sensors/bno086/sh2.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L57)

***

### NoMoreErrors

```ts
NoMoreErrors: 255;
```

Defined in: [src/sensors/bno086/sh2.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L59)

Sentinel: end of the error queue.

# Enumeration: FrsRecordId

Defined in: [src/sensors/bno086/sh2.ts:348](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L348)

FRS record IDs used by this SDK (SH-2 figure 28; metadata records).

## Enumeration Members

### StaticCalibrationAgm

```ts
StaticCalibrationAgm: 31097;
```

Defined in: [src/sensors/bno086/sh2.ts:349](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L349)

***

### NominalCalibration

```ts
NominalCalibration: 19789;
```

Defined in: [src/sensors/bno086/sh2.ts:350](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L350)

***

### DynamicCalibration

```ts
DynamicCalibration: 7967;
```

Defined in: [src/sensors/bno086/sh2.ts:351](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L351)

***

### MePowerMgmt

```ts
MePowerMgmt: 54242;
```

Defined in: [src/sensors/bno086/sh2.ts:352](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L352)

***

### SystemOrientation

```ts
SystemOrientation: 11582;
```

Defined in: [src/sensors/bno086/sh2.ts:354](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L354)

Mounting quaternion, 4 × Q30 words.

***

### AccelOrientation

```ts
AccelOrientation: 11585;
```

Defined in: [src/sensors/bno086/sh2.ts:355](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L355)

***

### GyroscopeOrientation

```ts
GyroscopeOrientation: 11590;
```

Defined in: [src/sensors/bno086/sh2.ts:356](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L356)

***

### MagnetometerOrientation

```ts
MagnetometerOrientation: 11596;
```

Defined in: [src/sensors/bno086/sh2.ts:357](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L357)

***

### ArvrStabilizationRv

```ts
ArvrStabilizationRv: 15917;
```

Defined in: [src/sensors/bno086/sh2.ts:358](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L358)

***

### ArvrStabilizationGrv

```ts
ArvrStabilizationGrv: 15918;
```

Defined in: [src/sensors/bno086/sh2.ts:359](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L359)

***

### SigMotionDetectConfig

```ts
SigMotionDetectConfig: 49780;
```

Defined in: [src/sensors/bno086/sh2.ts:361](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L361)

***

### ShakeDetectConfig

```ts
ShakeDetectConfig: 32125;
```

Defined in: [src/sensors/bno086/sh2.ts:362](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L362)

***

### StabilityDetectorConfig

```ts
StabilityDetectorConfig: 60805;
```

Defined in: [src/sensors/bno086/sh2.ts:363](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L363)

***

### ActivityTrackerConfig

```ts
ActivityTrackerConfig: 60808;
```

Defined in: [src/sensors/bno086/sh2.ts:365](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L365)

Personal-activity-classifier config.

# Enumeration: FrsStatus

Defined in: [src/sensors/bno086/sh2.ts:396](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L396)

FRS Read Response status (low nibble of the len/status byte).

## Enumeration Members

### NoError

```ts
NoError: 0;
```

Defined in: [src/sensors/bno086/sh2.ts:397](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L397)

***

### UnrecognizedFrsType

```ts
UnrecognizedFrsType: 1;
```

Defined in: [src/sensors/bno086/sh2.ts:398](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L398)

***

### Busy

```ts
Busy: 2;
```

Defined in: [src/sensors/bno086/sh2.ts:399](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L399)

***

### ReadCompleted

```ts
ReadCompleted: 3;
```

Defined in: [src/sensors/bno086/sh2.ts:400](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L400)

***

### OffsetOutOfRange

```ts
OffsetOutOfRange: 4;
```

Defined in: [src/sensors/bno086/sh2.ts:401](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L401)

***

### RecordEmpty

```ts
RecordEmpty: 5;
```

Defined in: [src/sensors/bno086/sh2.ts:402](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L402)

***

### BlockCompleted

```ts
BlockCompleted: 6;
```

Defined in: [src/sensors/bno086/sh2.ts:403](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L403)

***

### BlockAndReadCompleted

```ts
BlockAndReadCompleted: 7;
```

Defined in: [src/sensors/bno086/sh2.ts:404](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L404)

***

### DeviceError

```ts
DeviceError: 8;
```

Defined in: [src/sensors/bno086/sh2.ts:405](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L405)

# Enumeration: FrsWriteStatus

Defined in: [src/sensors/bno086/sh2.ts:408](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L408)

## Enumeration Members

### WordsReceived

```ts
WordsReceived: 0;
```

Defined in: [src/sensors/bno086/sh2.ts:409](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L409)

***

### UnrecognizedFrsType

```ts
UnrecognizedFrsType: 1;
```

Defined in: [src/sensors/bno086/sh2.ts:410](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L410)

***

### Busy

```ts
Busy: 2;
```

Defined in: [src/sensors/bno086/sh2.ts:411](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L411)

***

### WriteCompleted

```ts
WriteCompleted: 3;
```

Defined in: [src/sensors/bno086/sh2.ts:412](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L412)

***

### WriteModeReady

```ts
WriteModeReady: 4;
```

Defined in: [src/sensors/bno086/sh2.ts:413](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L413)

***

### WriteFailed

```ts
WriteFailed: 5;
```

Defined in: [src/sensors/bno086/sh2.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L414)

***

### NotInWriteMode

```ts
NotInWriteMode: 6;
```

Defined in: [src/sensors/bno086/sh2.ts:415](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L415)

***

### InvalidLength

```ts
InvalidLength: 7;
```

Defined in: [src/sensors/bno086/sh2.ts:416](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L416)

***

### RecordValid

```ts
RecordValid: 8;
```

Defined in: [src/sensors/bno086/sh2.ts:417](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L417)

***

### RecordInvalid

```ts
RecordInvalid: 9;
```

Defined in: [src/sensors/bno086/sh2.ts:418](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L418)

# Enumeration: OscillatorType

Defined in: [src/sensors/bno086/sh2.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L46)

Get-Oscillator-Type (command 0x0A) result (r[0]).

## Enumeration Members

### Internal

```ts
Internal: 0;
```

Defined in: [src/sensors/bno086/sh2.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L47)

***

### ExtCrystal

```ts
ExtCrystal: 1;
```

Defined in: [src/sensors/bno086/sh2.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L48)

***

### ExtClock

```ts
ExtClock: 2;
```

Defined in: [src/sensors/bno086/sh2.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L49)

# Enumeration: SensorId

Defined in: [src/sensors/bno086/reports.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L20)

SH-2 input report IDs (datasheet §1.3.5, sh2 reference driver).

## Enumeration Members

### Accelerometer

```ts
Accelerometer: 1;
```

Defined in: [src/sensors/bno086/reports.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L21)

***

### Gyroscope

```ts
Gyroscope: 2;
```

Defined in: [src/sensors/bno086/reports.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L22)

***

### Magnetometer

```ts
Magnetometer: 3;
```

Defined in: [src/sensors/bno086/reports.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L23)

***

### LinearAcceleration

```ts
LinearAcceleration: 4;
```

Defined in: [src/sensors/bno086/reports.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L24)

***

### RotationVector

```ts
RotationVector: 5;
```

Defined in: [src/sensors/bno086/reports.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L25)

***

### Gravity

```ts
Gravity: 6;
```

Defined in: [src/sensors/bno086/reports.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L26)

***

### UncalibratedGyroscope

```ts
UncalibratedGyroscope: 7;
```

Defined in: [src/sensors/bno086/reports.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L27)

***

### GameRotationVector

```ts
GameRotationVector: 8;
```

Defined in: [src/sensors/bno086/reports.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L28)

***

### GeomagneticRotationVector

```ts
GeomagneticRotationVector: 9;
```

Defined in: [src/sensors/bno086/reports.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L29)

***

### Pressure

```ts
Pressure: 10;
```

Defined in: [src/sensors/bno086/reports.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L30)

***

### AmbientLight

```ts
AmbientLight: 11;
```

Defined in: [src/sensors/bno086/reports.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L31)

***

### Humidity

```ts
Humidity: 12;
```

Defined in: [src/sensors/bno086/reports.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L32)

***

### Proximity

```ts
Proximity: 13;
```

Defined in: [src/sensors/bno086/reports.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L33)

***

### Temperature

```ts
Temperature: 14;
```

Defined in: [src/sensors/bno086/reports.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L34)

***

### UncalibratedMagnetometer

```ts
UncalibratedMagnetometer: 15;
```

Defined in: [src/sensors/bno086/reports.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L35)

***

### TapDetector

```ts
TapDetector: 16;
```

Defined in: [src/sensors/bno086/reports.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L36)

***

### StepCounter

```ts
StepCounter: 17;
```

Defined in: [src/sensors/bno086/reports.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L37)

***

### SignificantMotion

```ts
SignificantMotion: 18;
```

Defined in: [src/sensors/bno086/reports.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L38)

***

### StabilityClassifier

```ts
StabilityClassifier: 19;
```

Defined in: [src/sensors/bno086/reports.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L39)

***

### RawAccelerometer

```ts
RawAccelerometer: 20;
```

Defined in: [src/sensors/bno086/reports.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L40)

***

### RawGyroscope

```ts
RawGyroscope: 21;
```

Defined in: [src/sensors/bno086/reports.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L41)

***

### RawMagnetometer

```ts
RawMagnetometer: 22;
```

Defined in: [src/sensors/bno086/reports.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L42)

***

### StepDetector

```ts
StepDetector: 24;
```

Defined in: [src/sensors/bno086/reports.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L43)

***

### ShakeDetector

```ts
ShakeDetector: 25;
```

Defined in: [src/sensors/bno086/reports.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L44)

***

### FlipDetector

```ts
FlipDetector: 26;
```

Defined in: [src/sensors/bno086/reports.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L45)

***

### PickupDetector

```ts
PickupDetector: 27;
```

Defined in: [src/sensors/bno086/reports.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L46)

***

### StabilityDetector

```ts
StabilityDetector: 28;
```

Defined in: [src/sensors/bno086/reports.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L47)

***

### PersonalActivityClassifier

```ts
PersonalActivityClassifier: 30;
```

Defined in: [src/sensors/bno086/reports.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L48)

***

### SleepDetector

```ts
SleepDetector: 31;
```

Defined in: [src/sensors/bno086/reports.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L49)

***

### TiltDetector

```ts
TiltDetector: 32;
```

Defined in: [src/sensors/bno086/reports.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L50)

***

### PocketDetector

```ts
PocketDetector: 33;
```

Defined in: [src/sensors/bno086/reports.ts:51](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L51)

***

### CircleDetector

```ts
CircleDetector: 34;
```

Defined in: [src/sensors/bno086/reports.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L52)

***

### HeartRateMonitor

```ts
HeartRateMonitor: 35;
```

Defined in: [src/sensors/bno086/reports.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L53)

***

### ArvrStabilizedRv

```ts
ArvrStabilizedRv: 40;
```

Defined in: [src/sensors/bno086/reports.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L54)

***

### ArvrStabilizedGameRv

```ts
ArvrStabilizedGameRv: 41;
```

Defined in: [src/sensors/bno086/reports.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L55)

***

### GyroIntegratedRv

```ts
GyroIntegratedRv: 42;
```

Defined in: [src/sensors/bno086/reports.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L56)

# Enumeration: Sh2Command

Defined in: [src/sensors/bno086/sh2.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L33)

`command` field of Command Request/Response (0xF2/0xF1).

## Enumeration Members

### Errors

```ts
Errors: 1;
```

Defined in: [src/sensors/bno086/sh2.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L34)

***

### Counter

```ts
Counter: 2;
```

Defined in: [src/sensors/bno086/sh2.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L35)

***

### Tare

```ts
Tare: 3;
```

Defined in: [src/sensors/bno086/sh2.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L36)

***

### Initialize

```ts
Initialize: 4;
```

Defined in: [src/sensors/bno086/sh2.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L37)

***

### SaveDcd

```ts
SaveDcd: 6;
```

Defined in: [src/sensors/bno086/sh2.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L38)

***

### MeCalibrate

```ts
MeCalibrate: 7;
```

Defined in: [src/sensors/bno086/sh2.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L39)

***

### PeriodicDcdConfig

```ts
PeriodicDcdConfig: 9;
```

Defined in: [src/sensors/bno086/sh2.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L40)

***

### GetOscillatorType

```ts
GetOscillatorType: 10;
```

Defined in: [src/sensors/bno086/sh2.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L41)

***

### ClearDcdAndReset

```ts
ClearDcdAndReset: 11;
```

Defined in: [src/sensors/bno086/sh2.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L42)

# Enumeration: ShtpChannel

Defined in: [src/sensors/bno086/shtp.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L21)

SHTP framing layer for the BNO086 (contracts/05_SENSOR_BNO086.md §3).

Pure codec — no I/O. A frame is a 4-byte header plus a cargo fragment:

    length u16 LE  — bits 14:0 cargo length *including* the 4-byte header;
                     bit 15 set marks a continuation fragment
    channel u8     — see ShtpChannel
    seq u8         — per-channel, per-direction free-running counter

For a cargo that spans several bridge frames, the first fragment's length
field carries the TOTAL cargo length (header included) even though the
frame itself holds fewer bytes; each continuation fragment carries the
remaining length (its own header included) with bit 15 set. The receiver
trusts the first fragment's total and the actual frame sizes; continuation
length fields are informative only.

Mirrors the Python reference `depz_sensor_sdk.bno086.shtp`.

## Enumeration Members

### Command

```ts
Command: 0;
```

Defined in: [src/sensors/bno086/shtp.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L23)

SHTP command channel (advertisements).

***

### Executable

```ts
Executable: 1;
```

Defined in: [src/sensors/bno086/shtp.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L25)

Device executable: reset/on/sleep; RX 0x01 = reset done.

***

### Control

```ts
Control: 2;
```

Defined in: [src/sensors/bno086/shtp.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L27)

SH-2 control: feature/FRS/command reports.

***

### InputNormal

```ts
InputNormal: 3;
```

Defined in: [src/sensors/bno086/shtp.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L29)

Non-wake input reports (0xFB timebase + sensors).

***

### InputWake

```ts
InputWake: 4;
```

Defined in: [src/sensors/bno086/shtp.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L31)

Wake input reports (same cargo format as channel 3).

***

### GyroRv

```ts
GyroRv: 5;
```

Defined in: [src/sensors/bno086/shtp.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L33)

Gyro-integrated rotation vector, dense format.

# Enumeration: TareAxis

Defined in: [src/sensors/bno086/sh2.ts:116](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L116)

## Enumeration Members

### X

```ts
X: 1;
```

Defined in: [src/sensors/bno086/sh2.ts:117](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L117)

***

### Y

```ts
Y: 2;
```

Defined in: [src/sensors/bno086/sh2.ts:118](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L118)

***

### Z

```ts
Z: 4;
```

Defined in: [src/sensors/bno086/sh2.ts:119](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L119)

***

### All

```ts
All: 7;
```

Defined in: [src/sensors/bno086/sh2.ts:120](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L120)

# Enumeration: TareBasis

Defined in: [src/sensors/bno086/sh2.ts:107](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L107)

Rotation vector used as the tare reference (Tare Now P2).

## Enumeration Members

### RotationVector

```ts
RotationVector: 0;
```

Defined in: [src/sensors/bno086/sh2.ts:108](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L108)

***

### GameRotationVector

```ts
GameRotationVector: 1;
```

Defined in: [src/sensors/bno086/sh2.ts:109](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L109)

***

### GeomagneticRotationVector

```ts
GeomagneticRotationVector: 2;
```

Defined in: [src/sensors/bno086/sh2.ts:110](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L110)

***

### GyroIntegratedRv

```ts
GyroIntegratedRv: 3;
```

Defined in: [src/sensors/bno086/sh2.ts:111](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L111)

***

### ArvrStabilizedRv

```ts
ArvrStabilizedRv: 4;
```

Defined in: [src/sensors/bno086/sh2.ts:112](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L112)

***

### ArvrStabilizedGameRv

```ts
ArvrStabilizedGameRv: 5;
```

Defined in: [src/sensors/bno086/sh2.ts:113](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L113)

# Function: buildCommandRequest()

```ts
function buildCommandRequest(
   seq, 
   command, 
   params?): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:240](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L240)

Command Request (0xF2), 12 bytes: id, seq, command, P0..P8.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `seq` | `number` |
| `command` | `number` |
| `params` | `Uint8Array` |

## Returns

`Uint8Array`

# Function: buildFrame()

```ts
function buildFrame(
   channel, 
   payload, 
   seq): Uint8Array;
```

Defined in: [src/sensors/bno086/shtp.ts:82](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L82)

Single-fragment frame: length = header + payload.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `channel` | `number` |
| `payload` | `Uint8Array` |
| `seq` | `number` |

## Returns

`Uint8Array`

# Function: buildFrsReadRequest()

```ts
function buildFrsReadRequest(
   frsType, 
   offsetWords?, 
   blockWords?): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:422](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L422)

FRS Read Request (0xF4), 8 bytes. blockWords = 0 reads the record.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `frsType` | `number` | `undefined` |
| `offsetWords` | `number` | `0` |
| `blockWords` | `number` | `0` |

## Returns

`Uint8Array`

# Function: buildFrsWriteData()

```ts
function buildFrsWriteData(offsetWords, words): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:474](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L474)

FRS Write Data (0xF6), 12 bytes; 1 or 2 words per packet.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `offsetWords` | `number` |
| `words` | `number`[] |

## Returns

`Uint8Array`

# Function: buildFrsWriteRequest()

```ts
function buildFrsWriteRequest(frsType, lengthWords): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:463](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L463)

FRS Write Request (0xF7), 6 bytes. lengthWords = 0 erases the record.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `frsType` | `number` |
| `lengthWords` | `number` |

## Returns

`Uint8Array`

# Function: buildGetFeatureRequest()

```ts
function buildGetFeatureRequest(sensorId): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:165](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L165)

Get Feature Request (0xFE), 2 bytes.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `sensorId` | `number` |

## Returns

`Uint8Array`

# Function: buildProductIdRequest()

```ts
function buildProductIdRequest(): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L198)

## Returns

`Uint8Array`

# Function: buildSetFeature()

```ts
function buildSetFeature(
   sensorId, 
   intervalUs, 
   batchUs?, 
   sensitivity?, 
   flags?, 
   cfgWord?): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:144](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L144)

Set Feature Command (0xFD), 17 bytes.

`intervalUs` = 0 disables the sensor. `sensitivity` units are
sensor-dependent (change sensitivity, u16); `flags` bit meanings per SH-2
§6.5.4; `cfgWord` is the sensor-specific configuration u32.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensorId` | `number` | `undefined` |
| `intervalUs` | `number` | `undefined` |
| `batchUs` | `number` | `0` |
| `sensitivity` | `number` | `0` |
| `flags` | `number` | `0` |
| `cfgWord` | `number` | `0` |

## Returns

`Uint8Array`

# Function: countsClearParams()

```ts
function countsClearParams(sensorId): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L72)

Counter command 0x02: clear event counts for `sensorId`.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `sensorId` | `number` |

## Returns

`Uint8Array`

# Function: countsGetParams()

```ts
function countsGetParams(sensorId): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L67)

Counter command 0x02: get event counts for `sensorId`.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `sensorId` | `number` |

## Returns

`Uint8Array`

# Function: errorRecordFromResponse()

```ts
function errorRecordFromResponse(resp): ErrorRecord;
```

Defined in: [src/sensors/bno086/sh2.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L92)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `resp` | [`CommandResponse`](../interfaces/CommandResponse.md) |

## Returns

[`ErrorRecord`](../interfaces/ErrorRecord.md)

# Function: errorsParams()

```ts
function errorsParams(severity?): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L77)

Errors command 0x01: return errors of `severity` or greater (0 = all).

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `severity` | `number` | `0` |

## Returns

`Uint8Array`

# Function: fragmentCargo()

```ts
function fragmentCargo(
   channel, 
   payload, 
   seqStart, 
   maxFrame?): Uint8Array<ArrayBufferLike>[];
```

Defined in: [src/sensors/bno086/shtp.ts:101](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L101)

Split a cargo into wire frames of at most `maxFrame` bytes.

First fragment advertises the TOTAL cargo length; continuations carry the
remaining length with the continuation bit set. seq increments per frame.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `channel` | `number` | `undefined` |
| `payload` | `Uint8Array` | `undefined` |
| `seqStart` | `number` | `undefined` |
| `maxFrame` | `number` | `MAX_TX_FRAME` |

## Returns

`Uint8Array`\<`ArrayBufferLike`\>[]

# Function: meCalibrationParams()

```ts
function meCalibrationParams(
   accel, 
   gyro, 
   mag, 
   planar?, 
   subcommand?): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:324](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L324)

ME Calibration (command 0x07). subcommand 0 = configure, 1 = get.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `accel` | `boolean` | `undefined` |
| `gyro` | `boolean` | `undefined` |
| `mag` | `boolean` | `undefined` |
| `planar` | `boolean` | `false` |
| `subcommand` | `number` | `0` |

## Returns

`Uint8Array`

# Function: packShtpHeader()

```ts
function packShtpHeader(hdr): Uint8Array;
```

Defined in: [src/sensors/bno086/shtp.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L52)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `hdr` | [`ShtpHeader`](../interfaces/ShtpHeader.md) |

## Returns

`Uint8Array`

# Function: parseGyroRvCargo()

```ts
function parseGyroRvCargo(payload, captureTimestampUs): GyroIntegratedRV | null;
```

Defined in: [src/sensors/bno086/reports.ts:619](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L619)

Parse a channel-5 cargo (gyro-integrated RV, dense format).

Two shapes seen on hardware (vendor tool): 7×i16 bare, or prefixed with
0xFB + i32 base delta + u16 delay (both 100 µs ticks).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |
| `captureTimestampUs` | `bigint` |

## Returns

[`GyroIntegratedRV`](../interfaces/GyroIntegratedRV.md) \| `null`

# Function: parseInputCargo()

```ts
function parseInputCargo(payload, captureTimestampUs): Report[];
```

Defined in: [src/sensors/bno086/reports.ts:413](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L413)

Parse a channel-3/4 cargo into typed reports.

`captureTimestampUs` is the bridge RPT_DATA capture time (MCU uptime).
Handles 0xFB base timestamp references and 0xFA rebases; every report's
timestamp is `base + delay` where base = capture − baseDelta·100 µs.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |
| `captureTimestampUs` | `bigint` |

## Returns

[`Report`](../type-aliases/Report.md)[]

# Function: periodicDcdParams()

```ts
function periodicDcdParams(enable): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:341](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L341)

Periodic DCD save config (command 0x09). P0: 0 = enable, 1 = disable.
No command response is generated.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `enable` | `boolean` |

## Returns

`Uint8Array`

# Function: persistTareParams()

```ts
function persistTareParams(): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:298](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L298)

Tare subcommand 1 — persist current tare into FRS.

## Returns

`Uint8Array`

# Function: sensorMetadataFromWords()

```ts
function sensorMetadataFromWords(words): SensorMetadata;
```

Defined in: [src/sensors/bno086/sh2.ts:637](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L637)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `words` | `number`[] |

## Returns

[`SensorMetadata`](../interfaces/SensorMetadata.md)

# Function: setReorientationParams()

```ts
function setReorientationParams(
   x, 
   y, 
   z, 
   w): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:309](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L309)

Tare subcommand 2 — set reorientation quaternion.

P1..P8 are four int16 Q14 components (the 8 available parameter bytes only
fit Q14 halves; the *FRS* System Orientation record is the one that stores
Q30 words). All-zero clears the reorientation.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `x` | `number` |
| `y` | `number` |
| `z` | `number` |
| `w` | `number` |

## Returns

`Uint8Array`

# Function: tareNowParams()

```ts
function tareNowParams(axes?, basis?): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:290](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L290)

Tare subcommand 0 — tare `axes` (bitmap X=1,Y=2,Z=4) using `basis`.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `axes` | `number` | `TareAxis.All` |
| `basis` | `number` | `TareBasis.RotationVector` |

## Returns

`Uint8Array`

# Function: unpackCommandResponse()

```ts
function unpackCommandResponse(payload): CommandResponse;
```

Defined in: [src/sensors/bno086/sh2.ts:274](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L274)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`CommandResponse`](../interfaces/CommandResponse.md)

# Function: unpackFeatureResponse()

```ts
function unpackFeatureResponse(payload): FeatureResponse;
```

Defined in: [src/sensors/bno086/sh2.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L180)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`FeatureResponse`](../interfaces/FeatureResponse.md)

# Function: unpackFrsReadResponse()

```ts
function unpackFrsReadResponse(payload): FrsReadResponse;
```

Defined in: [src/sensors/bno086/sh2.ts:445](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L445)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`FrsReadResponse`](../interfaces/FrsReadResponse.md)

# Function: unpackFrsWriteResponse()

```ts
function unpackFrsWriteResponse(payload): FrsWriteResponse;
```

Defined in: [src/sensors/bno086/sh2.ts:495](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L495)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`FrsWriteResponse`](../interfaces/FrsWriteResponse.md)

# Function: unpackProductId()

```ts
function unpackProductId(payload): ProductId;
```

Defined in: [src/sensors/bno086/sh2.ts:217](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L217)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`ProductId`](../interfaces/ProductId.md)

# Function: unpackShtpHeader()

```ts
function unpackShtpHeader(data): ShtpHeader;
```

Defined in: [src/sensors/bno086/shtp.ts:62](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L62)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

## Returns

[`ShtpHeader`](../interfaces/ShtpHeader.md)

# Interface: Acceleration

Defined in: [src/sensors/bno086/reports.ts:178](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L178)

0x01 accelerometer / 0x04 linear acceleration / 0x06 gravity (Q8).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "Acceleration";
```

Defined in: [src/sensors/bno086/reports.ts:179](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L179)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L180)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:181](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L181)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:182](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L182)

***

### x

```ts
x: number;
```

Defined in: [src/sensors/bno086/reports.ts:184](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L184)

m/s².

***

### y

```ts
y: number;
```

Defined in: [src/sensors/bno086/reports.ts:185](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L185)

***

### z

```ts
z: number;
```

Defined in: [src/sensors/bno086/reports.ts:186](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L186)

# Interface: Bno086Options

Defined in: [src/sensors/bno086/bno086.ts:83](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L83)

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

### busyRetries?

```ts
optional busyRetries?: number;
```

Defined in: [src/sensors/bno086/bno086.ts:85](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L85)

SEND_SHTP_PACKET attempts before giving up (default 5).

***

### busyBackoffMs?

```ts
optional busyBackoffMs?: number;
```

Defined in: [src/sensors/bno086/bno086.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L87)

ERR_BUSY backoff; >= 200 ms per the bridge spec (tests inject less).

# Interface: CalibrationConfig

Defined in: [src/sensors/bno086/bno086.ts:76](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L76)

ME calibration enables as reported by the sensor.

## Properties

### accel

```ts
accel: boolean;
```

Defined in: [src/sensors/bno086/bno086.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L77)

***

### gyro

```ts
gyro: boolean;
```

Defined in: [src/sensors/bno086/bno086.ts:78](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L78)

***

### mag

```ts
mag: boolean;
```

Defined in: [src/sensors/bno086/bno086.ts:79](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L79)

***

### planar

```ts
planar: boolean;
```

Defined in: [src/sensors/bno086/bno086.ts:80](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L80)

# Interface: CommandResponse

Defined in: [src/sensors/bno086/sh2.ts:263](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L263)

Command Response (0xF1), 16 bytes.

`commandSeq` echoes the request's sequence number (correlate on it plus
`command`); `responseSeq` counts multiple responses to one request.
R0 is the status word for most commands (0 = success).

## Properties

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/sh2.ts:264](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L264)

***

### command

```ts
command: number;
```

Defined in: [src/sensors/bno086/sh2.ts:265](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L265)

***

### commandSeq

```ts
commandSeq: number;
```

Defined in: [src/sensors/bno086/sh2.ts:266](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L266)

***

### responseSeq

```ts
responseSeq: number;
```

Defined in: [src/sensors/bno086/sh2.ts:267](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L267)

***

### r

```ts
r: number[];
```

Defined in: [src/sensors/bno086/sh2.ts:269](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L269)

R0..R10.

***

### status

```ts
status: number;
```

Defined in: [src/sensors/bno086/sh2.ts:271](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L271)

R0.

# Interface: Counts

Defined in: [src/sensors/bno086/sh2.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L98)

Per-sensor event counts (command 0x02 get response, 2 messages).

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/sh2.ts:99](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L99)

***

### offered

```ts
offered: number;
```

Defined in: [src/sensors/bno086/sh2.ts:100](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L100)

***

### accepted

```ts
accepted: number;
```

Defined in: [src/sensors/bno086/sh2.ts:101](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L101)

***

### on

```ts
on: number;
```

Defined in: [src/sensors/bno086/sh2.ts:102](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L102)

***

### attempted

```ts
attempted: number;
```

Defined in: [src/sensors/bno086/sh2.ts:103](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L103)

# Interface: EnableOptions

Defined in: [src/sensors/bno086/bno086.ts:90](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L90)

## Properties

### intervalUs?

```ts
optional intervalUs?: number;
```

Defined in: [src/sensors/bno086/bno086.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L92)

Alternative to `hz`: exact report interval in µs.

***

### batchUs?

```ts
optional batchUs?: number;
```

Defined in: [src/sensors/bno086/bno086.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L93)

***

### sensitivity?

```ts
optional sensitivity?: number;
```

Defined in: [src/sensors/bno086/bno086.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L94)

***

### flags?

```ts
optional flags?: number;
```

Defined in: [src/sensors/bno086/bno086.ts:95](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L95)

***

### cfgWord?

```ts
optional cfgWord?: number;
```

Defined in: [src/sensors/bno086/bno086.ts:96](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L96)

***

### verify?

```ts
optional verify?: boolean;
```

Defined in: [src/sensors/bno086/bno086.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L98)

Read back the granted rate via Get Feature (default true).

***

### timeoutMs?

```ts
optional timeoutMs?: number;
```

Defined in: [src/sensors/bno086/bno086.ts:99](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L99)

# Interface: ErrorRecord

Defined in: [src/sensors/bno086/sh2.ts:82](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L82)

One error queue entry (command 0x01 response, r[0..5]).

## Properties

### severity

```ts
severity: number;
```

Defined in: [src/sensors/bno086/sh2.ts:83](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L83)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/sh2.ts:84](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L84)

***

### source

```ts
source: number;
```

Defined in: [src/sensors/bno086/sh2.ts:86](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L86)

ErrorSource.

***

### error

```ts
error: number;
```

Defined in: [src/sensors/bno086/sh2.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L87)

***

### module

```ts
module: number;
```

Defined in: [src/sensors/bno086/sh2.ts:88](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L88)

***

### code

```ts
code: number;
```

Defined in: [src/sensors/bno086/sh2.ts:89](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L89)

# Interface: FeatureResponse

Defined in: [src/sensors/bno086/sh2.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L170)

Get Feature Response (0xFC), 17 bytes — the rates in effect.

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/sh2.ts:171](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L171)

***

### flags

```ts
flags: number;
```

Defined in: [src/sensors/bno086/sh2.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L172)

***

### sensitivity

```ts
sensitivity: number;
```

Defined in: [src/sensors/bno086/sh2.ts:173](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L173)

***

### intervalUs

```ts
intervalUs: number;
```

Defined in: [src/sensors/bno086/sh2.ts:175](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L175)

Actual report interval granted by the hub.

***

### batchUs

```ts
batchUs: number;
```

Defined in: [src/sensors/bno086/sh2.ts:176](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L176)

***

### cfgWord

```ts
cfgWord: number;
```

Defined in: [src/sensors/bno086/sh2.ts:177](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L177)

# Interface: FrsReadResponse

Defined in: [src/sensors/bno086/sh2.ts:434](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L434)

FRS Read Response (0xF3), 16 bytes; up to two data words per packet.

## Properties

### status

```ts
status: number;
```

Defined in: [src/sensors/bno086/sh2.ts:436](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L436)

FrsStatus.

***

### dataLength

```ts
dataLength: number;
```

Defined in: [src/sensors/bno086/sh2.ts:438](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L438)

Valid words in data0/data1 (0–2).

***

### offsetWords

```ts
offsetWords: number;
```

Defined in: [src/sensors/bno086/sh2.ts:439](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L439)

***

### data0

```ts
data0: number;
```

Defined in: [src/sensors/bno086/sh2.ts:440](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L440)

***

### data1

```ts
data1: number;
```

Defined in: [src/sensors/bno086/sh2.ts:441](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L441)

***

### frsType

```ts
frsType: number;
```

Defined in: [src/sensors/bno086/sh2.ts:442](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L442)

# Interface: FrsWriteResponse

Defined in: [src/sensors/bno086/sh2.ts:489](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L489)

FRS Write Response (0xF5), 4 bytes.

## Properties

### status

```ts
status: number;
```

Defined in: [src/sensors/bno086/sh2.ts:491](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L491)

FrsWriteStatus.

***

### offsetWords

```ts
offsetWords: number;
```

Defined in: [src/sensors/bno086/sh2.ts:492](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L492)

# Interface: GenericEvent

Defined in: [src/sensors/bno086/reports.ts:338](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L338)

Simple u16-value detectors: 0x1A flip, 0x1B pickup, 0x1C stability
detector, 0x1F sleep, 0x20 tilt, 0x21 pocket, 0x22 circle, 0x23 HR.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "GenericEvent";
```

Defined in: [src/sensors/bno086/reports.ts:339](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L339)

***

### valueRaw

```ts
valueRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:340](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L340)

# Interface: GyroIntegratedRV

Defined in: [src/sensors/bno086/reports.ts:268](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L268)

0x2A gyro-integrated rotation vector (channel 5, dense — no SH-2 header).
Quaternion Q14, angular velocity Q10 rad/s.

## Extends

- [`ReportBase`](ReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`ReportBase`](ReportBase.md).[`sensorId`](ReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`ReportBase`](ReportBase.md).[`timestampUs`](ReportBase.md#timestampus)

***

### type

```ts
type: "GyroIntegratedRV";
```

Defined in: [src/sensors/bno086/reports.ts:269](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L269)

***

### iRaw

```ts
iRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:270](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L270)

***

### jRaw

```ts
jRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:271](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L271)

***

### kRaw

```ts
kRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:272](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L272)

***

### realRaw

```ts
realRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:273](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L273)

***

### vxRaw

```ts
vxRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:274](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L274)

***

### vyRaw

```ts
vyRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:275](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L275)

***

### vzRaw

```ts
vzRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:276](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L276)

***

### i

```ts
i: number;
```

Defined in: [src/sensors/bno086/reports.ts:277](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L277)

***

### j

```ts
j: number;
```

Defined in: [src/sensors/bno086/reports.ts:278](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L278)

***

### k

```ts
k: number;
```

Defined in: [src/sensors/bno086/reports.ts:279](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L279)

***

### real

```ts
real: number;
```

Defined in: [src/sensors/bno086/reports.ts:280](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L280)

***

### angularVelocity

```ts
angularVelocity: [number, number, number];
```

Defined in: [src/sensors/bno086/reports.ts:282](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L282)

rad/s.

# Interface: Gyroscope

Defined in: [src/sensors/bno086/reports.ts:190](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L190)

0x02 calibrated gyroscope (Q9).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "Gyroscope";
```

Defined in: [src/sensors/bno086/reports.ts:191](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L191)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:192](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L192)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:193](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L193)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

***

### x

```ts
x: number;
```

Defined in: [src/sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

rad/s.

***

### y

```ts
y: number;
```

Defined in: [src/sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

***

### z

```ts
z: number;
```

Defined in: [src/sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

# Interface: InputReportBase

Defined in: [src/sensors/bno086/reports.ts:168](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L168)

Channel-3/4 report with the common SH-2 header fields.

## Extends

- [`ReportBase`](ReportBase.md)

## Extended by

- [`Acceleration`](Acceleration.md)
- [`Gyroscope`](Gyroscope.md)
- [`Magnetometer`](Magnetometer.md)
- [`UncalibratedGyroscope`](UncalibratedGyroscope.md)
- [`UncalibratedMagnetometer`](UncalibratedMagnetometer.md)
- [`RotationVector`](RotationVector.md)
- [`ScalarReport`](ScalarReport.md)
- [`TapDetector`](TapDetector.md)
- [`StepCounter`](StepCounter.md)
- [`StepDetector`](StepDetector.md)
- [`SignificantMotion`](SignificantMotion.md)
- [`StabilityClassifier`](StabilityClassifier.md)
- [`ShakeDetector`](ShakeDetector.md)
- [`GenericEvent`](GenericEvent.md)
- [`PersonalActivityClassifier`](PersonalActivityClassifier.md)
- [`RawSensor`](RawSensor.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`ReportBase`](ReportBase.md).[`sensorId`](ReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`ReportBase`](ReportBase.md).[`timestampUs`](ReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

# Interface: Magnetometer

Defined in: [src/sensors/bno086/reports.ts:202](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L202)

0x03 calibrated magnetic field (Q4).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "Magnetometer";
```

Defined in: [src/sensors/bno086/reports.ts:203](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L203)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:204](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L204)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:205](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L205)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:206](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L206)

***

### x

```ts
x: number;
```

Defined in: [src/sensors/bno086/reports.ts:208](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L208)

µT.

***

### y

```ts
y: number;
```

Defined in: [src/sensors/bno086/reports.ts:209](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L209)

***

### z

```ts
z: number;
```

Defined in: [src/sensors/bno086/reports.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L210)

# Interface: PersonalActivityClassifier

Defined in: [src/sensors/bno086/reports.ts:347](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L347)

0x1E personal activity classifier (see ACTIVITY_NAMES).
`confidences` are 0–100 per state, states 0–9 of the current page.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "PersonalActivityClassifier";
```

Defined in: [src/sensors/bno086/reports.ts:348](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L348)

***

### pageNumber

```ts
pageNumber: number;
```

Defined in: [src/sensors/bno086/reports.ts:349](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L349)

***

### endOfSequence

```ts
endOfSequence: boolean;
```

Defined in: [src/sensors/bno086/reports.ts:350](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L350)

***

### mostLikelyState

```ts
mostLikelyState: number;
```

Defined in: [src/sensors/bno086/reports.ts:351](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L351)

***

### confidences

```ts
confidences: number[];
```

Defined in: [src/sensors/bno086/reports.ts:352](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L352)

***

### mostLikelyName

```ts
mostLikelyName: string;
```

Defined in: [src/sensors/bno086/reports.ts:353](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L353)

# Interface: ProductId

Defined in: [src/sensors/bno086/sh2.ts:206](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L206)

Product ID Response (0xF8), 16 bytes. The sensor sends one response per
subsystem (typically 2); resetCause per SH-2 §6.4.5.2.

## Properties

### resetCause

```ts
resetCause: number;
```

Defined in: [src/sensors/bno086/sh2.ts:207](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L207)

***

### swVersionMajor

```ts
swVersionMajor: number;
```

Defined in: [src/sensors/bno086/sh2.ts:208](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L208)

***

### swVersionMinor

```ts
swVersionMinor: number;
```

Defined in: [src/sensors/bno086/sh2.ts:209](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L209)

***

### swPartNumber

```ts
swPartNumber: number;
```

Defined in: [src/sensors/bno086/sh2.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L210)

***

### swBuildNumber

```ts
swBuildNumber: number;
```

Defined in: [src/sensors/bno086/sh2.ts:211](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L211)

***

### swVersionPatch

```ts
swVersionPatch: number;
```

Defined in: [src/sensors/bno086/sh2.ts:212](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L212)

***

### version

```ts
version: string;
```

Defined in: [src/sensors/bno086/sh2.ts:214](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L214)

"major.minor.patch".

# Interface: RawSensor

Defined in: [src/sensors/bno086/reports.ts:360](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L360)

0x14/0x15/0x16 raw ADC samples + sensor-clock timestamp (u32 µs).
`temperatureRaw` is populated only for the raw gyroscope.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "RawSensor";
```

Defined in: [src/sensors/bno086/reports.ts:361](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L361)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:362](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L362)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:363](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L363)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:364](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L364)

***

### sensorTimestampUs

```ts
sensorTimestampUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:365](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L365)

***

### temperatureRaw

```ts
temperatureRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:366](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L366)

# Interface: ReportBase

Defined in: [src/sensors/bno086/reports.ts:162](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L162)

Base for anything the sensor pushes; `timestampUs` is absolute in the MCU
clock domain (bridge capture time corrected by timebase + delay).

## Extended by

- [`InputReportBase`](InputReportBase.md)
- [`GyroIntegratedRV`](GyroIntegratedRV.md)
- [`UnknownReport`](UnknownReport.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

# Interface: RotationVector

Defined in: [src/sensors/bno086/reports.ts:249](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L249)

Quaternion reports 0x05/0x08/0x09/0x28/0x29 (unit quaternion, Q14).
`accuracyRaw` (Q12, radians) is present only for 0x05/0x09/0x28.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "RotationVector";
```

Defined in: [src/sensors/bno086/reports.ts:250](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L250)

***

### iRaw

```ts
iRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:251](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L251)

***

### jRaw

```ts
jRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:252](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L252)

***

### kRaw

```ts
kRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:253](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L253)

***

### realRaw

```ts
realRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:254](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L254)

***

### accuracyRaw

```ts
accuracyRaw: number | null;
```

Defined in: [src/sensors/bno086/reports.ts:255](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L255)

***

### i

```ts
i: number;
```

Defined in: [src/sensors/bno086/reports.ts:256](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L256)

***

### j

```ts
j: number;
```

Defined in: [src/sensors/bno086/reports.ts:257](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L257)

***

### k

```ts
k: number;
```

Defined in: [src/sensors/bno086/reports.ts:258](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L258)

***

### real

```ts
real: number;
```

Defined in: [src/sensors/bno086/reports.ts:259](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L259)

***

### accuracyRad

```ts
accuracyRad: number | null;
```

Defined in: [src/sensors/bno086/reports.ts:261](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L261)

Estimated heading accuracy in radians (null for game variants).

# Interface: ScalarReport

Defined in: [src/sensors/bno086/reports.ts:286](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L286)

Environment reports 0x0A–0x0E: single value, Q from Q_POINTS.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "ScalarReport";
```

Defined in: [src/sensors/bno086/reports.ts:287](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L287)

***

### valueRaw

```ts
valueRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:288](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L288)

***

### value

```ts
value: number;
```

Defined in: [src/sensors/bno086/reports.ts:289](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L289)

# Interface: SensorMetadata

Defined in: [src/sensors/bno086/sh2.ts:614](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L614)

Parsed sensor metadata FRS record; `rawWords` is authoritative.

Field packing follows the sh2 reference driver (revision-gated fields are
0 when the record predates them).

## Properties

### meVersion

```ts
meVersion: number;
```

Defined in: [src/sensors/bno086/sh2.ts:615](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L615)

***

### mhVersion

```ts
mhVersion: number;
```

Defined in: [src/sensors/bno086/sh2.ts:616](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L616)

***

### shVersion

```ts
shVersion: number;
```

Defined in: [src/sensors/bno086/sh2.ts:617](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L617)

***

### rangeRaw

```ts
rangeRaw: number;
```

Defined in: [src/sensors/bno086/sh2.ts:619](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L619)

Same units & Q point as the sensor's reports.

***

### resolutionRaw

```ts
resolutionRaw: number;
```

Defined in: [src/sensors/bno086/sh2.ts:620](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L620)

***

### revision

```ts
revision: number;
```

Defined in: [src/sensors/bno086/sh2.ts:621](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L621)

***

### powerMaQ10

```ts
powerMaQ10: number;
```

Defined in: [src/sensors/bno086/sh2.ts:623](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L623)

mA in Q10.

***

### minPeriodUs

```ts
minPeriodUs: number;
```

Defined in: [src/sensors/bno086/sh2.ts:624](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L624)

***

### maxPeriodUs

```ts
maxPeriodUs: number;
```

Defined in: [src/sensors/bno086/sh2.ts:626](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L626)

Revision >= 4 only.

***

### fifoMax

```ts
fifoMax: number;
```

Defined in: [src/sensors/bno086/sh2.ts:627](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L627)

***

### fifoReserved

```ts
fifoReserved: number;
```

Defined in: [src/sensors/bno086/sh2.ts:628](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L628)

***

### batchBufferBytes

```ts
batchBufferBytes: number;
```

Defined in: [src/sensors/bno086/sh2.ts:629](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L629)

***

### qPoint1

```ts
qPoint1: number;
```

Defined in: [src/sensors/bno086/sh2.ts:630](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L630)

***

### qPoint2

```ts
qPoint2: number;
```

Defined in: [src/sensors/bno086/sh2.ts:631](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L631)

***

### qPoint3

```ts
qPoint3: number;
```

Defined in: [src/sensors/bno086/sh2.ts:633](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L633)

Revision >= 3 only.

***

### rawWords

```ts
rawWords: number[];
```

Defined in: [src/sensors/bno086/sh2.ts:634](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L634)

# Interface: ShakeDetector

Defined in: [src/sensors/bno086/reports.ts:329](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L329)

0x19 shake detector; bits 0/1/2 = X/Y/Z shake.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "ShakeDetector";
```

Defined in: [src/sensors/bno086/reports.ts:330](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L330)

***

### flags

```ts
flags: number;
```

Defined in: [src/sensors/bno086/reports.ts:331](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L331)

# Interface: ShtpCargo

Defined in: [src/sensors/bno086/shtp.ts:74](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L74)

One reassembled cargo: `payload` excludes all SHTP headers.

## Properties

### channel

```ts
channel: number;
```

Defined in: [src/sensors/bno086/shtp.ts:75](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L75)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/shtp.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L77)

seq of the first fragment.

***

### payload

```ts
payload: Uint8Array;
```

Defined in: [src/sensors/bno086/shtp.ts:78](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L78)

# Interface: ShtpHeader

Defined in: [src/sensors/bno086/shtp.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L44)

## Properties

### length

```ts
length: number;
```

Defined in: [src/sensors/bno086/shtp.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L46)

Bits 14:0 — cargo length incl. this 4-byte header.

***

### channel

```ts
channel: number;
```

Defined in: [src/sensors/bno086/shtp.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L47)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/shtp.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L48)

***

### continuation

```ts
continuation: boolean;
```

Defined in: [src/sensors/bno086/shtp.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L49)

# Interface: SignificantMotion

Defined in: [src/sensors/bno086/reports.ts:316](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L316)

0x12 significant motion (1 = motion detected; sensor auto-disables).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "SignificantMotion";
```

Defined in: [src/sensors/bno086/reports.ts:317](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L317)

***

### motion

```ts
motion: number;
```

Defined in: [src/sensors/bno086/reports.ts:318](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L318)

# Interface: StabilityClassifier

Defined in: [src/sensors/bno086/reports.ts:322](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L322)

0x13 stability classification (see STABILITY_NAMES).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "StabilityClassifier";
```

Defined in: [src/sensors/bno086/reports.ts:323](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L323)

***

### classification

```ts
classification: number;
```

Defined in: [src/sensors/bno086/reports.ts:324](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L324)

***

### name

```ts
name: string;
```

Defined in: [src/sensors/bno086/reports.ts:325](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L325)

# Interface: StepCounter

Defined in: [src/sensors/bno086/reports.ts:303](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L303)

0x11 step counter. NOTE: latency u32 µs at bytes 4–7, steps u16 at bytes
8–9 per SH-2; the vendor tool's `latency(2)+steps(2)` comment is wrong.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "StepCounter";
```

Defined in: [src/sensors/bno086/reports.ts:304](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L304)

***

### latencyUs

```ts
latencyUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:305](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L305)

***

### steps

```ts
steps: number;
```

Defined in: [src/sensors/bno086/reports.ts:306](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L306)

# Interface: StepDetector

Defined in: [src/sensors/bno086/reports.ts:310](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L310)

0x18 step detector; latency from step event to report, µs.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "StepDetector";
```

Defined in: [src/sensors/bno086/reports.ts:311](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L311)

***

### latencyUs

```ts
latencyUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:312](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L312)

# Interface: TapDetector

Defined in: [src/sensors/bno086/reports.ts:293](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L293)

0x10 tap detector; `flags` bit 6 = double tap, bits 0–5 axis/sign.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "TapDetector";
```

Defined in: [src/sensors/bno086/reports.ts:294](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L294)

***

### flags

```ts
flags: number;
```

Defined in: [src/sensors/bno086/reports.ts:295](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L295)

***

### doubleTap

```ts
doubleTap: boolean;
```

Defined in: [src/sensors/bno086/reports.ts:296](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L296)

# Interface: UncalibratedGyroscope

Defined in: [src/sensors/bno086/reports.ts:214](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L214)

0x07 uncalibrated gyroscope + bias estimate (all Q9, rad/s).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "UncalibratedGyroscope";
```

Defined in: [src/sensors/bno086/reports.ts:215](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L215)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:216](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L216)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:217](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L217)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:218](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L218)

***

### biasXRaw

```ts
biasXRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:219](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L219)

***

### biasYRaw

```ts
biasYRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:220](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L220)

***

### biasZRaw

```ts
biasZRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:221](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L221)

***

### x

```ts
x: number;
```

Defined in: [src/sensors/bno086/reports.ts:222](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L222)

***

### y

```ts
y: number;
```

Defined in: [src/sensors/bno086/reports.ts:223](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L223)

***

### z

```ts
z: number;
```

Defined in: [src/sensors/bno086/reports.ts:224](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L224)

***

### bias

```ts
bias: [number, number, number];
```

Defined in: [src/sensors/bno086/reports.ts:226](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L226)

rad/s.

# Interface: UncalibratedMagnetometer

Defined in: [src/sensors/bno086/reports.ts:230](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L230)

0x0F uncalibrated magnetic field + hard-iron bias (all Q4, µT).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "UncalibratedMagnetometer";
```

Defined in: [src/sensors/bno086/reports.ts:231](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L231)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:232](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L232)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:233](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L233)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:234](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L234)

***

### biasXRaw

```ts
biasXRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:235](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L235)

***

### biasYRaw

```ts
biasYRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:236](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L236)

***

### biasZRaw

```ts
biasZRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:237](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L237)

***

### x

```ts
x: number;
```

Defined in: [src/sensors/bno086/reports.ts:238](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L238)

***

### y

```ts
y: number;
```

Defined in: [src/sensors/bno086/reports.ts:239](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L239)

***

### z

```ts
z: number;
```

Defined in: [src/sensors/bno086/reports.ts:240](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L240)

***

### bias

```ts
bias: [number, number, number];
```

Defined in: [src/sensors/bno086/reports.ts:242](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L242)

µT.

# Interface: UnknownReport

Defined in: [src/sensors/bno086/reports.ts:373](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L373)

Unrecognized report ID: raw bytes from the ID to end of cargo (the length
is unknowable, so parsing stops here).

## Extends

- [`ReportBase`](ReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`ReportBase`](ReportBase.md).[`sensorId`](ReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`ReportBase`](ReportBase.md).[`timestampUs`](ReportBase.md#timestampus)

***

### type

```ts
type: "UnknownReport";
```

Defined in: [src/sensors/bno086/reports.ts:374](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L374)

***

### data

```ts
data: Uint8Array;
```

Defined in: [src/sensors/bno086/reports.ts:375](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L375)

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
  | RawSensor;
```

Defined in: [src/sensors/bno086/reports.ts:379](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L379)

Channel-3/4 typed reports (share the SH-2 input header fields).

# Type Alias: Report

```ts
type Report = 
  | InputReport
  | GyroIntegratedRV
  | UnknownReport;
```

Defined in: [src/sensors/bno086/reports.ts:398](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L398)

Anything the sensor pushes.

# Variable: ACTIVITY\_NAMES

```ts
const ACTIVITY_NAMES: Record<number, string>;
```

Defined in: [src/sensors/bno086/reports.ts:140](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L140)

# Variable: BASE\_TIMESTAMP\_REF

```ts
const BASE_TIMESTAMP_REF: 251 = 0xfb;
```

Defined in: [src/sensors/bno086/reports.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L60)

# Variable: CONTINUATION\_BIT

```ts
const CONTINUATION_BIT: 32768 = 0x8000;
```

Defined in: [src/sensors/bno086/shtp.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L38)

# Variable: COUNTS\_CLEAR

```ts
const COUNTS_CLEAR: 1 = 1;
```

Defined in: [src/sensors/bno086/sh2.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L64)

# Variable: COUNTS\_GET

```ts
const COUNTS_GET: 0 = 0;
```

Defined in: [src/sensors/bno086/sh2.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L63)

Counter subcommands (command 0x02, P0).

# Variable: GYRO\_RV\_ANGVEL\_Q

```ts
const GYRO_RV_ANGVEL_Q: 10 = 10;
```

Defined in: [src/sensors/bno086/reports.ts:88](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L88)

# Variable: LENGTH\_MASK

```ts
const LENGTH_MASK: 32767 = 0x7fff;
```

Defined in: [src/sensors/bno086/shtp.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L37)

# Variable: MAX\_TX\_FRAME

```ts
const MAX_TX_FRAME: 64 = 64;
```

Defined in: [src/sensors/bno086/shtp.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L42)

Host->sensor frames must fit one MCU transmit slot (ERRATA E2: 2 x 64 B).

# Variable: METADATA\_RECORDS

```ts
const METADATA_RECORDS: Record<number, number>;
```

Defined in: [src/sensors/bno086/sh2.ts:369](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L369)

Per-sensor metadata FRS record IDs (subset used by getMetadata()).

# Variable: ME\_CAL\_GET

```ts
const ME_CAL_GET: 1 = 0x01;
```

Defined in: [src/sensors/bno086/sh2.ts:335](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L335)

Subcommand: report current ME calibration config.

# Variable: NUM\_CHANNELS

```ts
const NUM_CHANNELS: 6 = 6;
```

Defined in: [src/sensors/bno086/shtp.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L39)

# Variable: Q\_POINTS

```ts
const Q_POINTS: Record<number, number>;
```

Defined in: [src/sensors/bno086/reports.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L64)

Q point of the primary fields (value = raw / 2**Q); see module docs.

# Variable: RATE\_HIGH\_FACTOR

```ts
const RATE_HIGH_FACTOR: 2.1 = 2.1;
```

Defined in: [src/sensors/bno086/bno086.ts:68](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L68)

# Variable: RATE\_LOW\_FACTOR

```ts
const RATE_LOW_FACTOR: 0.9 = 0.9;
```

Defined in: [src/sensors/bno086/bno086.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L67)

# Variable: REPORT\_LENGTHS

```ts
const REPORT_LENGTHS: Record<number, number>;
```

Defined in: [src/sensors/bno086/reports.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L94)

Total report length on the wire, 4-byte SH-2 header included
(sh2 reference driver report-length table).

# Variable: RV\_ACCURACY\_Q

```ts
const RV_ACCURACY_Q: 12 = 12;
```

Defined in: [src/sensors/bno086/reports.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L87)

# Variable: SHTP\_HEADER\_SIZE

```ts
const SHTP_HEADER_SIZE: 4 = 4;
```

Defined in: [src/sensors/bno086/shtp.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L36)

# Variable: STABILITY\_NAMES

```ts
const STABILITY_NAMES: Record<number, string>;
```

Defined in: [src/sensors/bno086/reports.ts:133](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L133)

# Variable: TIMESTAMP\_REBASE

```ts
const TIMESTAMP_REBASE: 250 = 0xfa;
```

Defined in: [src/sensors/bno086/reports.ts:61](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L61)

