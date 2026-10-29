<!-- generated from ../bno086/ by scripts/gen_bno085_docs.py — edit those pages -->
# BNO085 (IMU) — API reference

The public API for the BNO085 IMU: the `Bno086` device plus the SHTP /
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
- [FeatureFlag](enumerations/FeatureFlag.md)
- [MotionIntent](enumerations/MotionIntent.md)
- [FrsRecordId](enumerations/FrsRecordId.md)
- [FrsStatus](enumerations/FrsStatus.md)
- [FrsWriteStatus](enumerations/FrsWriteStatus.md)
- [ShtpChannel](enumerations/ShtpChannel.md)
- [ExecutableCommand](enumerations/ExecutableCommand.md)

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
- [MotionRequest](interfaces/MotionRequest.md)
- [FlushCompleted](interfaces/FlushCompleted.md)
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
- [FLUSH\_COMPLETED](variables/FLUSH_COMPLETED.md)
- [Q\_POINTS](variables/Q_POINTS.md)
- [RV\_ACCURACY\_Q](variables/RV_ACCURACY_Q.md)
- [GYRO\_RV\_ANGVEL\_Q](variables/GYRO_RV_ANGVEL_Q.md)
- [REPORT\_LENGTHS](variables/REPORT_LENGTHS.md)
- [MOTION\_REQUEST\_NAMES](variables/MOTION_REQUEST_NAMES.md)
- [MOTION\_INTENT\_NAMES](variables/MOTION_INTENT_NAMES.md)
- [STABILITY\_NAMES](variables/STABILITY_NAMES.md)
- [ACTIVITY\_NAMES](variables/ACTIVITY_NAMES.md)
- [CONTROL\_RESPONSE\_LENGTHS](variables/CONTROL_RESPONSE_LENGTHS.md)
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
- [splitControlCargo](functions/splitControlCargo.md)
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
- [interactiveCalibrationParams](functions/interactiveCalibrationParams.md)
- [buildForceFlush](functions/buildForceFlush.md)
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

Defined in: [sensors/bno086/bno086.ts:150](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L150)

BNO085 device: enable SH-2 sensors, stream typed reports.

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

Defined in: [sensors/bno086/bno086.ts:171](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L171)

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

Defined in: [device/device.ts:214](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L214)

#### Inherited from

```ts
DepzDevice.timeoutMs
```

***

### link

```ts
protected readonly link: DepzLink;
```

Defined in: [device/device.ts:216](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L216)

#### Inherited from

```ts
DepzDevice.link
```

***

### busyRetries

```ts
busyRetries: number = 5;
```

Defined in: [sensors/bno086/bno086.ts:152](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L152)

SEND_SHTP_PACKET attempts before giving up.

***

### busyBackoffMs

```ts
busyBackoffMs: number = BUSY_BACKOFF_MS;
```

Defined in: [sensors/bno086/bno086.ts:154](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L154)

>= 200 ms per the bridge spec (contract 05 §2).

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
DepzDevice.stats
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
DepzDevice.timeSync
```

***

### advertisement

#### Get Signature

```ts
get advertisement(): Uint8Array;
```

Defined in: [sensors/bno086/bno086.ts:481](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L481)

Raw SHTP channel-0 advertisement bytes seen since open/reset.

##### Returns

`Uint8Array`

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
DepzDevice.open
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
DepzDevice.close
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
DepzDevice.registerStream
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
DepzDevice.onEvent
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
DepzDevice.emitEvent
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
DepzDevice.callGuarded
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
DepzDevice.request
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
DepzDevice.expectReport
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
DepzDevice.expectText
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
DepzDevice.getDeviceName
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
DepzDevice.getSoftwareName
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
DepzDevice.getSerialNumber
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
DepzDevice.identify
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
DepzDevice.readMcuTemperature
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
DepzDevice.syncTime
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
DepzDevice.toHostTimeUs
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
DepzDevice.getReportPayloadCrc
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
DepzDevice.setReportPayloadCrc
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
DepzDevice.getSyncPin
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
DepzDevice.setSyncPin
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
DepzDevice.reset
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
DepzDevice.enterBootloaderMode
```

***

### onTeardown()

```ts
protected onTeardown(error): void;
```

Defined in: [sensors/bno086/bno086.ts:177](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L177)

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

Defined in: [sensors/bno086/bno086.ts:190](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L190)

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

Defined in: [sensors/bno086/bno086.ts:422](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L422)

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

Defined in: [sensors/bno086/bno086.ts:449](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L449)

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

Defined in: [sensors/bno086/bno086.ts:464](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L464)

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

Defined in: [sensors/bno086/bno086.ts:476](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L476)

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

Defined in: [sensors/bno086/bno086.ts:495](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L495)

The SH-2 application's Product ID — the first of `productIds()`.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`ProductId`](../interfaces/ProductId.md)\>

***

### productIds()

```ts
productIds(timeoutMs?, settleMs?): Promise<ProductId[]>;
```

Defined in: [sensors/bno086/bno086.ts:506](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L506)

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

`Promise`\<[`ProductId`](../interfaces/ProductId.md)[]\>

***

### enable()

```ts
enable(
   sensor, 
   hz?, 
opts?): Promise<FeatureResponse | null>;
```

Defined in: [sensors/bno086/bno086.ts:535](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L535)

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

Defined in: [sensors/bno086/bno086.ts:594](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L594)

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

Defined in: [sensors/bno086/bno086.ts:600](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L600)

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

Defined in: [sensors/bno086/bno086.ts:611](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L611)

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

Defined in: [sensors/bno086/bno086.ts:615](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L615)

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

Defined in: [sensors/bno086/bno086.ts:619](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L619)

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

Defined in: [sensors/bno086/bno086.ts:623](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L623)

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

Defined in: [sensors/bno086/bno086.ts:627](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L627)

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

Defined in: [sensors/bno086/bno086.ts:631](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L631)

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

Defined in: [sensors/bno086/bno086.ts:635](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L635)

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

Defined in: [sensors/bno086/bno086.ts:639](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L639)

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

Defined in: [sensors/bno086/bno086.ts:649](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L649)

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

Defined in: [sensors/bno086/bno086.ts:662](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L662)

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

Defined in: [sensors/bno086/bno086.ts:675](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L675)

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

Defined in: [sensors/bno086/bno086.ts:680](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L680)

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

Defined in: [sensors/bno086/bno086.ts:694](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L694)

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

Defined in: [sensors/bno086/bno086.ts:699](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L699)

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

Defined in: [sensors/bno086/bno086.ts:715](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L715)

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

Defined in: [sensors/bno086/bno086.ts:730](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L730)

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

Defined in: [sensors/bno086/bno086.ts:738](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L738)

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

Defined in: [sensors/bno086/bno086.ts:751](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L751)

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

Defined in: [sensors/bno086/bno086.ts:760](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L760)

Enable MotionRequest (0x2B) reports. The hub sends one when its request
changes, not at `hz`; it reads the rate back late, so the granted rate is
not checked.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `1` |
| `opts` | [`EnableOptions`](../interfaces/EnableOptions.md) | `{}` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### flush()

```ts
flush(sensor, timeoutMs?): Promise<void>;
```

Defined in: [sensors/bno086/bno086.ts:772](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L772)

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

Defined in: [sensors/bno086/bno086.ts:798](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L798)

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

Defined in: [sensors/bno086/bno086.ts:816](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L816)

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

Defined in: [sensors/bno086/bno086.ts:839](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L839)

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

Defined in: [sensors/bno086/bno086.ts:852](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L852)

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

Defined in: [sensors/bno086/bno086.ts:862](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L862)

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

Defined in: [sensors/bno086/bno086.ts:889](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L889)

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

Defined in: [sensors/bno086/bno086.ts:907](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L907)

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

Defined in: [sensors/bno086/bno086.ts:932](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L932)

Clear a sensor's event counts (command 0x02, subcommand 1).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensor` | `number` | `undefined` |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<`void`\>

# Class: FrsReadSession

Defined in: [sensors/bno086/sh2.ts:616](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L616)

Multi-packet FRS read state machine (pure — feed parsed responses).

Usage: send `request()`, then `feed()` every 0xF3 for this record until it
returns true; `words` holds the record. Error statuses throw.

## Constructors

### Constructor

```ts
new FrsReadSession(frsType): FrsReadSession;
```

Defined in: [sensors/bno086/sh2.ts:621](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L621)

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

Defined in: [sensors/bno086/sh2.ts:617](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L617)

***

### words

```ts
words: number[] = [];
```

Defined in: [sensors/bno086/sh2.ts:618](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L618)

***

### done

```ts
done: boolean = false;
```

Defined in: [sensors/bno086/sh2.ts:619](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L619)

## Methods

### request()

```ts
request(): Uint8Array;
```

Defined in: [sensors/bno086/sh2.ts:625](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L625)

#### Returns

`Uint8Array`

***

### feed()

```ts
feed(resp): boolean;
```

Defined in: [sensors/bno086/sh2.ts:629](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L629)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `resp` | [`FrsReadResponse`](../interfaces/FrsReadResponse.md) |

#### Returns

`boolean`

# Class: FrsWriteSession

Defined in: [sensors/bno086/sh2.ts:664](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L664)

Multi-packet FRS write state machine (pure).

Usage: send `request()`; then for every 0xF5 call `feed()` — it returns
the next Write Data payload to send, or null; `done` flips on
WriteCompleted. Error statuses throw.

## Constructors

### Constructor

```ts
new FrsWriteSession(frsType, words): FrsWriteSession;
```

Defined in: [sensors/bno086/sh2.ts:670](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L670)

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

Defined in: [sensors/bno086/sh2.ts:665](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L665)

***

### words

```ts
readonly words: number[];
```

Defined in: [sensors/bno086/sh2.ts:666](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L666)

***

### offset

```ts
offset: number = 0;
```

Defined in: [sensors/bno086/sh2.ts:667](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L667)

***

### done

```ts
done: boolean = false;
```

Defined in: [sensors/bno086/sh2.ts:668](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L668)

## Methods

### request()

```ts
request(): Uint8Array;
```

Defined in: [sensors/bno086/sh2.ts:675](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L675)

#### Returns

`Uint8Array`

***

### feed()

```ts
feed(resp): Uint8Array<ArrayBufferLike> | null;
```

Defined in: [sensors/bno086/sh2.ts:687](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L687)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `resp` | [`FrsWriteResponse`](../interfaces/FrsWriteResponse.md) |

#### Returns

`Uint8Array`\<`ArrayBufferLike`\> \| `null`

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

# Class: ShtpLayer

Defined in: [sensors/bno086/shtp.ts:181](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L181)

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

Defined in: [sensors/bno086/shtp.ts:183](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L183)

Incomplete cargos and junk-channel (≥ 6) frames thrown away.

## Methods

### nextFrame()

```ts
nextFrame(channel, payload): Uint8Array;
```

Defined in: [sensors/bno086/shtp.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L196)

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

Defined in: [sensors/bno086/shtp.ts:208](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L208)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `channel` | `number` |

#### Returns

`number`

***

### feed()

```ts
feed(frame, captureUs?): ShtpCargo | null;
```

Defined in: [sensors/bno086/shtp.ts:225](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L225)

Consume one inbound frame; return the cargo when complete.

Rules (contract 05 §3): a non-continuation fragment starts a new cargo
(discarding any partial one on that channel); a continuation without a
cargo in progress is dropped; the cargo completes when the accumulated
bytes reach the first fragment's advertised total.

`captureUs` is the frame's RPT_DATA capture time; the cargo carries the
FIRST fragment's — the base its 0xFB timestamps refer to.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `frame` | `Uint8Array` | `undefined` |
| `captureUs` | `bigint` | `0n` |

#### Returns

[`ShtpCargo`](../interfaces/ShtpCargo.md) \| `null`

***

### reset()

```ts
reset(): void;
```

Defined in: [sensors/bno086/shtp.ts:273](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L273)

Forget all TX seq counters and partial cargos (sensor reset).

#### Returns

`void`

# Enumeration: ControlReport

Defined in: [sensors/bno086/sh2.ts:18](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L18)

Report IDs on SHTP channel 2 (control).

## Enumeration Members

### CommandResponse

```ts
CommandResponse: 241;
```

Defined in: [sensors/bno086/sh2.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L19)

***

### CommandRequest

```ts
CommandRequest: 242;
```

Defined in: [sensors/bno086/sh2.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L20)

***

### FrsReadResponse

```ts
FrsReadResponse: 243;
```

Defined in: [sensors/bno086/sh2.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L21)

***

### FrsReadRequest

```ts
FrsReadRequest: 244;
```

Defined in: [sensors/bno086/sh2.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L22)

***

### FrsWriteResponse

```ts
FrsWriteResponse: 245;
```

Defined in: [sensors/bno086/sh2.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L23)

***

### FrsWriteData

```ts
FrsWriteData: 246;
```

Defined in: [sensors/bno086/sh2.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L24)

***

### FrsWriteRequest

```ts
FrsWriteRequest: 247;
```

Defined in: [sensors/bno086/sh2.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L25)

***

### ProductIdResponse

```ts
ProductIdResponse: 248;
```

Defined in: [sensors/bno086/sh2.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L26)

***

### ProductIdRequest

```ts
ProductIdRequest: 249;
```

Defined in: [sensors/bno086/sh2.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L27)

***

### GetFeatureResponse

```ts
GetFeatureResponse: 252;
```

Defined in: [sensors/bno086/sh2.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L28)

***

### SetFeatureCommand

```ts
SetFeatureCommand: 253;
```

Defined in: [sensors/bno086/sh2.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L29)

***

### GetFeatureRequest

```ts
GetFeatureRequest: 254;
```

Defined in: [sensors/bno086/sh2.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L30)

***

### ForceSensorFlush

```ts
ForceSensorFlush: 240;
```

Defined in: [sensors/bno086/sh2.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L32)

Host → hub; answered by Flush Completed (0xEF).

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

# Enumeration: FrsStatus

Defined in: [sensors/bno086/sh2.ts:492](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L492)

FRS Read Response status (low nibble of the len/status byte).

## Enumeration Members

### NoError

```ts
NoError: 0;
```

Defined in: [sensors/bno086/sh2.ts:493](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L493)

***

### UnrecognizedFrsType

```ts
UnrecognizedFrsType: 1;
```

Defined in: [sensors/bno086/sh2.ts:494](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L494)

***

### Busy

```ts
Busy: 2;
```

Defined in: [sensors/bno086/sh2.ts:495](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L495)

***

### ReadCompleted

```ts
ReadCompleted: 3;
```

Defined in: [sensors/bno086/sh2.ts:496](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L496)

***

### OffsetOutOfRange

```ts
OffsetOutOfRange: 4;
```

Defined in: [sensors/bno086/sh2.ts:497](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L497)

***

### RecordEmpty

```ts
RecordEmpty: 5;
```

Defined in: [sensors/bno086/sh2.ts:498](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L498)

***

### BlockCompleted

```ts
BlockCompleted: 6;
```

Defined in: [sensors/bno086/sh2.ts:499](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L499)

***

### BlockAndReadCompleted

```ts
BlockAndReadCompleted: 7;
```

Defined in: [sensors/bno086/sh2.ts:500](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L500)

***

### DeviceError

```ts
DeviceError: 8;
```

Defined in: [sensors/bno086/sh2.ts:501](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L501)

# Enumeration: FrsWriteStatus

Defined in: [sensors/bno086/sh2.ts:504](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L504)

## Enumeration Members

### WordsReceived

```ts
WordsReceived: 0;
```

Defined in: [sensors/bno086/sh2.ts:505](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L505)

***

### UnrecognizedFrsType

```ts
UnrecognizedFrsType: 1;
```

Defined in: [sensors/bno086/sh2.ts:506](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L506)

***

### Busy

```ts
Busy: 2;
```

Defined in: [sensors/bno086/sh2.ts:507](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L507)

***

### WriteCompleted

```ts
WriteCompleted: 3;
```

Defined in: [sensors/bno086/sh2.ts:508](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L508)

***

### WriteModeReady

```ts
WriteModeReady: 4;
```

Defined in: [sensors/bno086/sh2.ts:509](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L509)

***

### WriteFailed

```ts
WriteFailed: 5;
```

Defined in: [sensors/bno086/sh2.ts:510](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L510)

***

### NotInWriteMode

```ts
NotInWriteMode: 6;
```

Defined in: [sensors/bno086/sh2.ts:511](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L511)

***

### InvalidLength

```ts
InvalidLength: 7;
```

Defined in: [sensors/bno086/sh2.ts:512](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L512)

***

### RecordValid

```ts
RecordValid: 8;
```

Defined in: [sensors/bno086/sh2.ts:513](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L513)

***

### RecordInvalid

```ts
RecordInvalid: 9;
```

Defined in: [sensors/bno086/sh2.ts:514](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L514)

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

Defined in: [sensors/bno086/reports.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L22)

SH-2 input report IDs (datasheet §1.3.5, sh2 reference driver).

## Enumeration Members

### Accelerometer

```ts
Accelerometer: 1;
```

Defined in: [sensors/bno086/reports.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L23)

***

### Gyroscope

```ts
Gyroscope: 2;
```

Defined in: [sensors/bno086/reports.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L24)

***

### Magnetometer

```ts
Magnetometer: 3;
```

Defined in: [sensors/bno086/reports.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L25)

***

### LinearAcceleration

```ts
LinearAcceleration: 4;
```

Defined in: [sensors/bno086/reports.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L26)

***

### RotationVector

```ts
RotationVector: 5;
```

Defined in: [sensors/bno086/reports.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L27)

***

### Gravity

```ts
Gravity: 6;
```

Defined in: [sensors/bno086/reports.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L28)

***

### UncalibratedGyroscope

```ts
UncalibratedGyroscope: 7;
```

Defined in: [sensors/bno086/reports.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L29)

***

### GameRotationVector

```ts
GameRotationVector: 8;
```

Defined in: [sensors/bno086/reports.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L30)

***

### GeomagneticRotationVector

```ts
GeomagneticRotationVector: 9;
```

Defined in: [sensors/bno086/reports.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L31)

***

### Pressure

```ts
Pressure: 10;
```

Defined in: [sensors/bno086/reports.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L32)

***

### AmbientLight

```ts
AmbientLight: 11;
```

Defined in: [sensors/bno086/reports.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L33)

***

### Humidity

```ts
Humidity: 12;
```

Defined in: [sensors/bno086/reports.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L34)

***

### Proximity

```ts
Proximity: 13;
```

Defined in: [sensors/bno086/reports.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L35)

***

### Temperature

```ts
Temperature: 14;
```

Defined in: [sensors/bno086/reports.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L36)

***

### UncalibratedMagnetometer

```ts
UncalibratedMagnetometer: 15;
```

Defined in: [sensors/bno086/reports.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L37)

***

### TapDetector

```ts
TapDetector: 16;
```

Defined in: [sensors/bno086/reports.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L38)

***

### StepCounter

```ts
StepCounter: 17;
```

Defined in: [sensors/bno086/reports.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L39)

***

### SignificantMotion

```ts
SignificantMotion: 18;
```

Defined in: [sensors/bno086/reports.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L40)

***

### StabilityClassifier

```ts
StabilityClassifier: 19;
```

Defined in: [sensors/bno086/reports.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L41)

***

### RawAccelerometer

```ts
RawAccelerometer: 20;
```

Defined in: [sensors/bno086/reports.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L42)

***

### RawGyroscope

```ts
RawGyroscope: 21;
```

Defined in: [sensors/bno086/reports.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L43)

***

### RawMagnetometer

```ts
RawMagnetometer: 22;
```

Defined in: [sensors/bno086/reports.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L44)

***

### StepDetector

```ts
StepDetector: 24;
```

Defined in: [sensors/bno086/reports.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L45)

***

### ShakeDetector

```ts
ShakeDetector: 25;
```

Defined in: [sensors/bno086/reports.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L46)

***

### FlipDetector

```ts
FlipDetector: 26;
```

Defined in: [sensors/bno086/reports.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L47)

***

### PickupDetector

```ts
PickupDetector: 27;
```

Defined in: [sensors/bno086/reports.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L48)

***

### StabilityDetector

```ts
StabilityDetector: 28;
```

Defined in: [sensors/bno086/reports.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L49)

***

### PersonalActivityClassifier

```ts
PersonalActivityClassifier: 30;
```

Defined in: [sensors/bno086/reports.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L50)

***

### SleepDetector

```ts
SleepDetector: 31;
```

Defined in: [sensors/bno086/reports.ts:51](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L51)

***

### TiltDetector

```ts
TiltDetector: 32;
```

Defined in: [sensors/bno086/reports.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L52)

***

### PocketDetector

```ts
PocketDetector: 33;
```

Defined in: [sensors/bno086/reports.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L53)

***

### CircleDetector

```ts
CircleDetector: 34;
```

Defined in: [sensors/bno086/reports.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L54)

***

### HeartRateMonitor

```ts
HeartRateMonitor: 35;
```

Defined in: [sensors/bno086/reports.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L55)

***

### ArvrStabilizedRv

```ts
ArvrStabilizedRv: 40;
```

Defined in: [sensors/bno086/reports.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L56)

***

### ArvrStabilizedGameRv

```ts
ArvrStabilizedGameRv: 41;
```

Defined in: [sensors/bno086/reports.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L57)

***

### GyroIntegratedRv

```ts
GyroIntegratedRv: 42;
```

Defined in: [sensors/bno086/reports.ts:58](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L58)

***

### MotionRequest

```ts
MotionRequest: 43;
```

Defined in: [sensors/bno086/reports.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L59)

# Enumeration: Sh2Command

Defined in: [sensors/bno086/sh2.ts:69](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L69)

`command` field of Command Request/Response (0xF2/0xF1).

## Enumeration Members

### Errors

```ts
Errors: 1;
```

Defined in: [sensors/bno086/sh2.ts:70](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L70)

***

### Counter

```ts
Counter: 2;
```

Defined in: [sensors/bno086/sh2.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L71)

***

### Tare

```ts
Tare: 3;
```

Defined in: [sensors/bno086/sh2.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L72)

***

### Initialize

```ts
Initialize: 4;
```

Defined in: [sensors/bno086/sh2.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L73)

***

### SaveDcd

```ts
SaveDcd: 6;
```

Defined in: [sensors/bno086/sh2.ts:74](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L74)

***

### MeCalibrate

```ts
MeCalibrate: 7;
```

Defined in: [sensors/bno086/sh2.ts:75](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L75)

***

### PeriodicDcdConfig

```ts
PeriodicDcdConfig: 9;
```

Defined in: [sensors/bno086/sh2.ts:76](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L76)

***

### GetOscillatorType

```ts
GetOscillatorType: 10;
```

Defined in: [sensors/bno086/sh2.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L77)

***

### ClearDcdAndReset

```ts
ClearDcdAndReset: 11;
```

Defined in: [sensors/bno086/sh2.ts:78](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L78)

***

### InteractiveCalibration

```ts
InteractiveCalibration: 14;
```

Defined in: [sensors/bno086/sh2.ts:80](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L80)

BNO085 only; no response.

# Enumeration: ShtpChannel

Defined in: [sensors/bno086/shtp.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L23)

## Enumeration Members

### Command

```ts
Command: 0;
```

Defined in: [sensors/bno086/shtp.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L25)

SHTP command channel (advertisements).

***

### Executable

```ts
Executable: 1;
```

Defined in: [sensors/bno086/shtp.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L27)

Device executable: reset/on/sleep; RX 0x01 = reset done.

***

### Control

```ts
Control: 2;
```

Defined in: [sensors/bno086/shtp.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L29)

SH-2 control: feature/FRS/command reports.

***

### InputNormal

```ts
InputNormal: 3;
```

Defined in: [sensors/bno086/shtp.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L31)

Non-wake input reports (0xFB timebase + sensors).

***

### InputWake

```ts
InputWake: 4;
```

Defined in: [sensors/bno086/shtp.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L33)

Wake input reports (same cargo format as channel 3).

***

### GyroRv

```ts
GyroRv: 5;
```

Defined in: [sensors/bno086/shtp.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L35)

Gyro-integrated rotation vector, dense format.

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

# Function: buildCommandRequest()

```ts
function buildCommandRequest(
   seq, 
   command, 
   params?): Uint8Array;
```

Defined in: [sensors/bno086/sh2.ts:297](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L297)

Command Request (0xF2), 12 bytes: id, seq, command, P0..P8.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `seq` | `number` |
| `command` | `number` |
| `params` | `Uint8Array` |

## Returns

`Uint8Array`

# Function: buildForceFlush()

```ts
function buildForceFlush(sensorId): Uint8Array;
```

Defined in: [sensors/bno086/sh2.ts:429](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L429)

Force Sensor Flush (0xF0, SH-2 §6.5.6): flush `sensorId`'s batched data
now. The hub answers with Flush Completed (0xEF) — on the input channel, as
the lab BNO085 does (SH-2 3.12.6, 2026-10-22).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `sensorId` | `number` |

## Returns

`Uint8Array`

# Function: buildFrame()

```ts
function buildFrame(
   channel, 
   payload, 
   seq): Uint8Array;
```

Defined in: [sensors/bno086/shtp.ts:102](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L102)

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

Defined in: [sensors/bno086/sh2.ts:518](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L518)

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

Defined in: [sensors/bno086/sh2.ts:571](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L571)

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

Defined in: [sensors/bno086/sh2.ts:560](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L560)

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

Defined in: [sensors/bno086/sh2.ts:218](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L218)

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

Defined in: [sensors/bno086/sh2.ts:252](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L252)

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

Defined in: [sensors/bno086/sh2.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L197)

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

Defined in: [sensors/bno086/sh2.ts:110](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L110)

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

Defined in: [sensors/bno086/sh2.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L105)

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

Defined in: [sensors/bno086/sh2.ts:130](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L130)

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

Defined in: [sensors/bno086/sh2.ts:115](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L115)

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

Defined in: [sensors/bno086/shtp.ts:121](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L121)

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

# Function: interactiveCalibrationParams()

```ts
function interactiveCalibrationParams(intent): Uint8Array;
```

Defined in: [sensors/bno086/sh2.ts:419](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L419)

Interactive Calibration (command 0x0E, BNO085 only): P0 = motion intent,
P1..P8 reserved. No command response is generated.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `intent` | `number` |

## Returns

`Uint8Array`

# Function: meCalibrationParams()

```ts
function meCalibrationParams(
   accel, 
   gyro, 
   mag, 
   planar?, 
   subcommand?): Uint8Array;
```

Defined in: [sensors/bno086/sh2.ts:382](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L382)

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

Defined in: [sensors/bno086/shtp.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L67)

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

Defined in: [sensors/bno086/reports.ts:688](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L688)

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

Defined in: [sensors/bno086/reports.ts:462](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L462)

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

Defined in: [sensors/bno086/sh2.ts:399](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L399)

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

Defined in: [sensors/bno086/sh2.ts:356](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L356)

Tare subcommand 1 — persist current tare into FRS.

## Returns

`Uint8Array`

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

[`SensorMetadata`](../interfaces/SensorMetadata.md)

# Function: setReorientationParams()

```ts
function setReorientationParams(
   x, 
   y, 
   z, 
   w): Uint8Array;
```

Defined in: [sensors/bno086/sh2.ts:367](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L367)

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

# Function: splitControlCargo()

```ts
function splitControlCargo(payload): Uint8Array<ArrayBufferLike>[];
```

Defined in: [sensors/bno086/sh2.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L53)

Split a control-channel cargo into its reports. An unknown report ID (or a
short tail) ends the walk: the rest is returned as one piece, since its
length is unknowable.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

`Uint8Array`\<`ArrayBufferLike`\>[]

# Function: tareNowParams()

```ts
function tareNowParams(axes?, basis?): Uint8Array;
```

Defined in: [sensors/bno086/sh2.ts:348](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L348)

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

Defined in: [sensors/bno086/sh2.ts:331](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L331)

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

Defined in: [sensors/bno086/sh2.ts:233](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L233)

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

Defined in: [sensors/bno086/sh2.ts:541](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L541)

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

Defined in: [sensors/bno086/sh2.ts:592](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L592)

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

Defined in: [sensors/bno086/sh2.ts:273](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L273)

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

Defined in: [sensors/bno086/shtp.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L77)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

## Returns

[`ShtpHeader`](../interfaces/ShtpHeader.md)

# Interface: Acceleration

Defined in: [sensors/bno086/reports.ts:202](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L202)

0x01 accelerometer / 0x04 linear acceleration / 0x06 gravity (Q8).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "Acceleration";
```

Defined in: [sensors/bno086/reports.ts:203](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L203)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [sensors/bno086/reports.ts:204](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L204)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [sensors/bno086/reports.ts:205](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L205)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [sensors/bno086/reports.ts:206](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L206)

***

### x

```ts
x: number;
```

Defined in: [sensors/bno086/reports.ts:208](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L208)

m/s².

***

### y

```ts
y: number;
```

Defined in: [sensors/bno086/reports.ts:209](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L209)

***

### z

```ts
z: number;
```

Defined in: [sensors/bno086/reports.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L210)

# Interface: Bno086Options

Defined in: [sensors/bno086/bno086.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L93)

## Extends

- `DeviceOptions`

## Properties

### timeoutMs?

```ts
optional timeoutMs?: number;
```

Defined in: [device/device.ts:183](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L183)

#### Inherited from

```ts
DeviceOptions.timeoutMs
```

***

### txCrcType?

```ts
optional txCrcType?: CrcType;
```

Defined in: [device/device.ts:184](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L184)

#### Inherited from

```ts
DeviceOptions.txCrcType
```

***

### busyRetries?

```ts
optional busyRetries?: number;
```

Defined in: [sensors/bno086/bno086.ts:95](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L95)

SEND_SHTP_PACKET attempts before giving up (default 5).

***

### busyBackoffMs?

```ts
optional busyBackoffMs?: number;
```

Defined in: [sensors/bno086/bno086.ts:97](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L97)

ERR_BUSY backoff; >= 200 ms per the bridge spec (tests inject less).

# Interface: CalibrationConfig

Defined in: [sensors/bno086/bno086.ts:86](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L86)

ME calibration enables as reported by the sensor.

## Properties

### accel

```ts
accel: boolean;
```

Defined in: [sensors/bno086/bno086.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L87)

***

### gyro

```ts
gyro: boolean;
```

Defined in: [sensors/bno086/bno086.ts:88](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L88)

***

### mag

```ts
mag: boolean;
```

Defined in: [sensors/bno086/bno086.ts:89](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L89)

***

### planar

```ts
planar: boolean;
```

Defined in: [sensors/bno086/bno086.ts:90](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L90)

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

Defined in: [sensors/bno086/bno086.ts:100](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L100)

## Properties

### intervalUs?

```ts
optional intervalUs?: number;
```

Defined in: [sensors/bno086/bno086.ts:102](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L102)

Alternative to `hz`: exact report interval in µs.

***

### batchUs?

```ts
optional batchUs?: number;
```

Defined in: [sensors/bno086/bno086.ts:103](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L103)

***

### sensitivity?

```ts
optional sensitivity?: number;
```

Defined in: [sensors/bno086/bno086.ts:104](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L104)

***

### flags?

```ts
optional flags?: number;
```

Defined in: [sensors/bno086/bno086.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L105)

***

### cfgWord?

```ts
optional cfgWord?: number;
```

Defined in: [sensors/bno086/bno086.ts:106](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L106)

***

### verify?

```ts
optional verify?: boolean;
```

Defined in: [sensors/bno086/bno086.ts:108](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L108)

Read back the granted rate via Get Feature (default true).

***

### timeoutMs?

```ts
optional timeoutMs?: number;
```

Defined in: [sensors/bno086/bno086.ts:109](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L109)

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

Defined in: [sensors/bno086/reports.ts:412](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L412)

0xEF Flush Completed: every batched report of `flushedSensor` as of the
matching Force Sensor Flush has been sent. `sensorId` is 0xEF.

## Extends

- [`ReportBase`](ReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`ReportBase`](ReportBase.md).[`sensorId`](ReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`ReportBase`](ReportBase.md).[`timestampUs`](ReportBase.md#timestampus)

***

### type

```ts
type: "FlushCompleted";
```

Defined in: [sensors/bno086/reports.ts:413](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L413)

***

### flushedSensor

```ts
flushedSensor: number;
```

Defined in: [sensors/bno086/reports.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L414)

# Interface: FrsReadResponse

Defined in: [sensors/bno086/sh2.ts:530](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L530)

FRS Read Response (0xF3), 16 bytes; up to two data words per packet.

## Properties

### status

```ts
status: number;
```

Defined in: [sensors/bno086/sh2.ts:532](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L532)

FrsStatus.

***

### dataLength

```ts
dataLength: number;
```

Defined in: [sensors/bno086/sh2.ts:534](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L534)

Valid words in data0/data1 (0–2).

***

### offsetWords

```ts
offsetWords: number;
```

Defined in: [sensors/bno086/sh2.ts:535](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L535)

***

### data0

```ts
data0: number;
```

Defined in: [sensors/bno086/sh2.ts:536](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L536)

***

### data1

```ts
data1: number;
```

Defined in: [sensors/bno086/sh2.ts:537](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L537)

***

### frsType

```ts
frsType: number;
```

Defined in: [sensors/bno086/sh2.ts:538](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L538)

# Interface: FrsWriteResponse

Defined in: [sensors/bno086/sh2.ts:586](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L586)

FRS Write Response (0xF5), 4 bytes.

## Properties

### status

```ts
status: number;
```

Defined in: [sensors/bno086/sh2.ts:588](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L588)

FrsWriteStatus.

***

### offsetWords

```ts
offsetWords: number;
```

Defined in: [sensors/bno086/sh2.ts:589](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L589)

# Interface: GenericEvent

Defined in: [sensors/bno086/reports.ts:362](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L362)

Simple u16-value detectors: 0x1A flip, 0x1B pickup, 0x1C stability
detector, 0x1F sleep, 0x20 tilt, 0x21 pocket, 0x22 circle, 0x23 HR.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "GenericEvent";
```

Defined in: [sensors/bno086/reports.ts:363](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L363)

***

### valueRaw

```ts
valueRaw: number;
```

Defined in: [sensors/bno086/reports.ts:364](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L364)

# Interface: GyroIntegratedRV

Defined in: [sensors/bno086/reports.ts:292](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L292)

0x2A gyro-integrated rotation vector (channel 5, dense — no SH-2 header).
Quaternion Q14, angular velocity Q10 rad/s.

## Extends

- [`ReportBase`](ReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`ReportBase`](ReportBase.md).[`sensorId`](ReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`ReportBase`](ReportBase.md).[`timestampUs`](ReportBase.md#timestampus)

***

### type

```ts
type: "GyroIntegratedRV";
```

Defined in: [sensors/bno086/reports.ts:293](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L293)

***

### iRaw

```ts
iRaw: number;
```

Defined in: [sensors/bno086/reports.ts:294](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L294)

***

### jRaw

```ts
jRaw: number;
```

Defined in: [sensors/bno086/reports.ts:295](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L295)

***

### kRaw

```ts
kRaw: number;
```

Defined in: [sensors/bno086/reports.ts:296](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L296)

***

### realRaw

```ts
realRaw: number;
```

Defined in: [sensors/bno086/reports.ts:297](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L297)

***

### vxRaw

```ts
vxRaw: number;
```

Defined in: [sensors/bno086/reports.ts:298](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L298)

***

### vyRaw

```ts
vyRaw: number;
```

Defined in: [sensors/bno086/reports.ts:299](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L299)

***

### vzRaw

```ts
vzRaw: number;
```

Defined in: [sensors/bno086/reports.ts:300](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L300)

***

### i

```ts
i: number;
```

Defined in: [sensors/bno086/reports.ts:301](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L301)

***

### j

```ts
j: number;
```

Defined in: [sensors/bno086/reports.ts:302](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L302)

***

### k

```ts
k: number;
```

Defined in: [sensors/bno086/reports.ts:303](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L303)

***

### real

```ts
real: number;
```

Defined in: [sensors/bno086/reports.ts:304](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L304)

***

### angularVelocity

```ts
angularVelocity: [number, number, number];
```

Defined in: [sensors/bno086/reports.ts:306](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L306)

rad/s.

# Interface: Gyroscope

Defined in: [sensors/bno086/reports.ts:214](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L214)

0x02 calibrated gyroscope (Q9).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "Gyroscope";
```

Defined in: [sensors/bno086/reports.ts:215](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L215)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [sensors/bno086/reports.ts:216](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L216)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [sensors/bno086/reports.ts:217](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L217)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [sensors/bno086/reports.ts:218](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L218)

***

### x

```ts
x: number;
```

Defined in: [sensors/bno086/reports.ts:220](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L220)

rad/s.

***

### y

```ts
y: number;
```

Defined in: [sensors/bno086/reports.ts:221](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L221)

***

### z

```ts
z: number;
```

Defined in: [sensors/bno086/reports.ts:222](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L222)

# Interface: InputReportBase

Defined in: [sensors/bno086/reports.ts:192](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L192)

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
- [`MotionRequest`](MotionRequest.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`ReportBase`](ReportBase.md).[`sensorId`](ReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`ReportBase`](ReportBase.md).[`timestampUs`](ReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

# Interface: Magnetometer

Defined in: [sensors/bno086/reports.ts:226](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L226)

0x03 calibrated magnetic field (Q4).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "Magnetometer";
```

Defined in: [sensors/bno086/reports.ts:227](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L227)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [sensors/bno086/reports.ts:228](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L228)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [sensors/bno086/reports.ts:229](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L229)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [sensors/bno086/reports.ts:230](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L230)

***

### x

```ts
x: number;
```

Defined in: [sensors/bno086/reports.ts:232](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L232)

µT.

***

### y

```ts
y: number;
```

Defined in: [sensors/bno086/reports.ts:233](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L233)

***

### z

```ts
z: number;
```

Defined in: [sensors/bno086/reports.ts:234](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L234)

# Interface: MotionRequest

Defined in: [sensors/bno086/reports.ts:398](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L398)

0x2B motion request (BNO085 interactive calibration): the motion intent the
hub was last told, and what it asks of the device. Sent when the request
changes, not periodically, whatever the granted rate.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "MotionRequest";
```

Defined in: [sensors/bno086/reports.ts:399](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L399)

***

### intent

```ts
intent: number;
```

Defined in: [sensors/bno086/reports.ts:400](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L400)

***

### request

```ts
request: number;
```

Defined in: [sensors/bno086/reports.ts:401](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L401)

***

### intentName

```ts
intentName: string;
```

Defined in: [sensors/bno086/reports.ts:402](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L402)

***

### requestName

```ts
requestName: string;
```

Defined in: [sensors/bno086/reports.ts:403](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L403)

***

### wantsStillness

```ts
wantsStillness: boolean;
```

Defined in: [sensors/bno086/reports.ts:405](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L405)

The hub asks the device to stop (the deprecated optional request 2 does not count).

# Interface: PersonalActivityClassifier

Defined in: [sensors/bno086/reports.ts:371](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L371)

0x1E personal activity classifier (see ACTIVITY_NAMES).
`confidences` are 0–100 per state, states 0–9 of the current page.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "PersonalActivityClassifier";
```

Defined in: [sensors/bno086/reports.ts:372](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L372)

***

### pageNumber

```ts
pageNumber: number;
```

Defined in: [sensors/bno086/reports.ts:373](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L373)

***

### endOfSequence

```ts
endOfSequence: boolean;
```

Defined in: [sensors/bno086/reports.ts:374](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L374)

***

### mostLikelyState

```ts
mostLikelyState: number;
```

Defined in: [sensors/bno086/reports.ts:375](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L375)

***

### confidences

```ts
confidences: number[];
```

Defined in: [sensors/bno086/reports.ts:376](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L376)

***

### mostLikelyName

```ts
mostLikelyName: string;
```

Defined in: [sensors/bno086/reports.ts:377](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L377)

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

Defined in: [sensors/bno086/reports.ts:384](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L384)

0x14/0x15/0x16 raw ADC samples + sensor-clock timestamp (u32 µs).
`temperatureRaw` is populated only for the raw gyroscope.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "RawSensor";
```

Defined in: [sensors/bno086/reports.ts:385](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L385)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [sensors/bno086/reports.ts:386](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L386)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [sensors/bno086/reports.ts:387](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L387)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [sensors/bno086/reports.ts:388](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L388)

***

### sensorTimestampUs

```ts
sensorTimestampUs: number;
```

Defined in: [sensors/bno086/reports.ts:389](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L389)

***

### temperatureRaw

```ts
temperatureRaw: number;
```

Defined in: [sensors/bno086/reports.ts:390](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L390)

# Interface: ReportBase

Defined in: [sensors/bno086/reports.ts:186](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L186)

Base for anything the sensor pushes; `timestampUs` is absolute in the MCU
clock domain (bridge capture time corrected by timebase + delay).

## Extended by

- [`InputReportBase`](InputReportBase.md)
- [`GyroIntegratedRV`](GyroIntegratedRV.md)
- [`FlushCompleted`](FlushCompleted.md)
- [`UnknownReport`](UnknownReport.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

# Interface: RotationVector

Defined in: [sensors/bno086/reports.ts:273](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L273)

Quaternion reports 0x05/0x08/0x09/0x28/0x29 (unit quaternion, Q14).
`accuracyRaw` (Q12, radians) is present only for 0x05/0x09/0x28.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "RotationVector";
```

Defined in: [sensors/bno086/reports.ts:274](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L274)

***

### iRaw

```ts
iRaw: number;
```

Defined in: [sensors/bno086/reports.ts:275](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L275)

***

### jRaw

```ts
jRaw: number;
```

Defined in: [sensors/bno086/reports.ts:276](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L276)

***

### kRaw

```ts
kRaw: number;
```

Defined in: [sensors/bno086/reports.ts:277](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L277)

***

### realRaw

```ts
realRaw: number;
```

Defined in: [sensors/bno086/reports.ts:278](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L278)

***

### accuracyRaw

```ts
accuracyRaw: number | null;
```

Defined in: [sensors/bno086/reports.ts:279](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L279)

***

### i

```ts
i: number;
```

Defined in: [sensors/bno086/reports.ts:280](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L280)

***

### j

```ts
j: number;
```

Defined in: [sensors/bno086/reports.ts:281](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L281)

***

### k

```ts
k: number;
```

Defined in: [sensors/bno086/reports.ts:282](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L282)

***

### real

```ts
real: number;
```

Defined in: [sensors/bno086/reports.ts:283](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L283)

***

### accuracyRad

```ts
accuracyRad: number | null;
```

Defined in: [sensors/bno086/reports.ts:285](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L285)

Estimated heading accuracy in radians (null for game variants).

# Interface: ScalarReport

Defined in: [sensors/bno086/reports.ts:310](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L310)

Environment reports 0x0A–0x0E: single value, Q from Q_POINTS.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "ScalarReport";
```

Defined in: [sensors/bno086/reports.ts:311](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L311)

***

### valueRaw

```ts
valueRaw: number;
```

Defined in: [sensors/bno086/reports.ts:312](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L312)

***

### value

```ts
value: number;
```

Defined in: [sensors/bno086/reports.ts:313](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L313)

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

Defined in: [sensors/bno086/reports.ts:353](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L353)

0x19 shake detector; bits 0/1/2 = X/Y/Z shake.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "ShakeDetector";
```

Defined in: [sensors/bno086/reports.ts:354](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L354)

***

### flags

```ts
flags: number;
```

Defined in: [sensors/bno086/reports.ts:355](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L355)

# Interface: ShtpCargo

Defined in: [sensors/bno086/shtp.ts:89](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L89)

One reassembled cargo: `payload` excludes all SHTP headers.

## Properties

### channel

```ts
channel: number;
```

Defined in: [sensors/bno086/shtp.ts:90](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L90)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/shtp.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L92)

seq of the first fragment.

***

### payload

```ts
payload: Uint8Array;
```

Defined in: [sensors/bno086/shtp.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L93)

***

### captureUs

```ts
captureUs: bigint;
```

Defined in: [sensors/bno086/shtp.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L98)

Capture time (µs, MCU clock) of the FIRST fragment — the base the cargo's
0xFB timestamps refer to (contract 05 §3). 0 when the caller fed none.

# Interface: ShtpHeader

Defined in: [sensors/bno086/shtp.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L59)

## Properties

### length

```ts
length: number;
```

Defined in: [sensors/bno086/shtp.ts:61](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L61)

Bits 14:0 — cargo length incl. this 4-byte header.

***

### channel

```ts
channel: number;
```

Defined in: [sensors/bno086/shtp.ts:62](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L62)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/shtp.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L63)

***

### continuation

```ts
continuation: boolean;
```

Defined in: [sensors/bno086/shtp.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L64)

# Interface: SignificantMotion

Defined in: [sensors/bno086/reports.ts:340](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L340)

0x12 significant motion (1 = motion detected; sensor auto-disables).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "SignificantMotion";
```

Defined in: [sensors/bno086/reports.ts:341](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L341)

***

### motion

```ts
motion: number;
```

Defined in: [sensors/bno086/reports.ts:342](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L342)

# Interface: StabilityClassifier

Defined in: [sensors/bno086/reports.ts:346](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L346)

0x13 stability classification (see STABILITY_NAMES).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "StabilityClassifier";
```

Defined in: [sensors/bno086/reports.ts:347](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L347)

***

### classification

```ts
classification: number;
```

Defined in: [sensors/bno086/reports.ts:348](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L348)

***

### name

```ts
name: string;
```

Defined in: [sensors/bno086/reports.ts:349](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L349)

# Interface: StepCounter

Defined in: [sensors/bno086/reports.ts:327](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L327)

0x11 step counter. NOTE: latency u32 µs at bytes 4–7, steps u16 at bytes
8–9 per SH-2; the vendor tool's `latency(2)+steps(2)` comment is wrong.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "StepCounter";
```

Defined in: [sensors/bno086/reports.ts:328](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L328)

***

### latencyUs

```ts
latencyUs: number;
```

Defined in: [sensors/bno086/reports.ts:329](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L329)

***

### steps

```ts
steps: number;
```

Defined in: [sensors/bno086/reports.ts:330](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L330)

# Interface: StepDetector

Defined in: [sensors/bno086/reports.ts:334](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L334)

0x18 step detector; latency from step event to report, µs.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "StepDetector";
```

Defined in: [sensors/bno086/reports.ts:335](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L335)

***

### latencyUs

```ts
latencyUs: number;
```

Defined in: [sensors/bno086/reports.ts:336](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L336)

# Interface: TapDetector

Defined in: [sensors/bno086/reports.ts:317](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L317)

0x10 tap detector; `flags` bit 6 = double tap, bits 0–5 axis/sign.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "TapDetector";
```

Defined in: [sensors/bno086/reports.ts:318](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L318)

***

### flags

```ts
flags: number;
```

Defined in: [sensors/bno086/reports.ts:319](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L319)

***

### doubleTap

```ts
doubleTap: boolean;
```

Defined in: [sensors/bno086/reports.ts:320](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L320)

# Interface: UncalibratedGyroscope

Defined in: [sensors/bno086/reports.ts:238](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L238)

0x07 uncalibrated gyroscope + bias estimate (all Q9, rad/s).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "UncalibratedGyroscope";
```

Defined in: [sensors/bno086/reports.ts:239](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L239)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [sensors/bno086/reports.ts:240](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L240)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [sensors/bno086/reports.ts:241](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L241)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [sensors/bno086/reports.ts:242](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L242)

***

### biasXRaw

```ts
biasXRaw: number;
```

Defined in: [sensors/bno086/reports.ts:243](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L243)

***

### biasYRaw

```ts
biasYRaw: number;
```

Defined in: [sensors/bno086/reports.ts:244](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L244)

***

### biasZRaw

```ts
biasZRaw: number;
```

Defined in: [sensors/bno086/reports.ts:245](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L245)

***

### x

```ts
x: number;
```

Defined in: [sensors/bno086/reports.ts:246](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L246)

***

### y

```ts
y: number;
```

Defined in: [sensors/bno086/reports.ts:247](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L247)

***

### z

```ts
z: number;
```

Defined in: [sensors/bno086/reports.ts:248](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L248)

***

### bias

```ts
bias: [number, number, number];
```

Defined in: [sensors/bno086/reports.ts:250](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L250)

rad/s.

# Interface: UncalibratedMagnetometer

Defined in: [sensors/bno086/reports.ts:254](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L254)

0x0F uncalibrated magnetic field + hard-iron bias (all Q4, µT).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "UncalibratedMagnetometer";
```

Defined in: [sensors/bno086/reports.ts:255](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L255)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [sensors/bno086/reports.ts:256](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L256)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [sensors/bno086/reports.ts:257](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L257)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [sensors/bno086/reports.ts:258](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L258)

***

### biasXRaw

```ts
biasXRaw: number;
```

Defined in: [sensors/bno086/reports.ts:259](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L259)

***

### biasYRaw

```ts
biasYRaw: number;
```

Defined in: [sensors/bno086/reports.ts:260](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L260)

***

### biasZRaw

```ts
biasZRaw: number;
```

Defined in: [sensors/bno086/reports.ts:261](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L261)

***

### x

```ts
x: number;
```

Defined in: [sensors/bno086/reports.ts:262](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L262)

***

### y

```ts
y: number;
```

Defined in: [sensors/bno086/reports.ts:263](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L263)

***

### z

```ts
z: number;
```

Defined in: [sensors/bno086/reports.ts:264](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L264)

***

### bias

```ts
bias: [number, number, number];
```

Defined in: [sensors/bno086/reports.ts:266](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L266)

µT.

# Interface: UnknownReport

Defined in: [sensors/bno086/reports.ts:421](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L421)

Unrecognized report ID: raw bytes from the ID to end of cargo (the length
is unknowable, so parsing stops here).

## Extends

- [`ReportBase`](ReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [sensors/bno086/reports.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L187)

#### Inherited from

[`ReportBase`](ReportBase.md).[`sensorId`](ReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno086/reports.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L188)

#### Inherited from

[`ReportBase`](ReportBase.md).[`timestampUs`](ReportBase.md#timestampus)

***

### type

```ts
type: "UnknownReport";
```

Defined in: [sensors/bno086/reports.ts:422](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L422)

***

### data

```ts
data: Uint8Array;
```

Defined in: [sensors/bno086/reports.ts:423](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L423)

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

Defined in: [sensors/bno086/reports.ts:427](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L427)

Channel-3/4 typed reports (share the SH-2 input header fields).

# Type Alias: Report

```ts
type Report = 
  | InputReport
  | GyroIntegratedRV
  | FlushCompleted
  | UnknownReport;
```

Defined in: [sensors/bno086/reports.ts:447](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L447)

Anything the sensor pushes.

# Variable: ACTIVITY\_NAMES

```ts
const ACTIVITY_NAMES: Record<number, string>;
```

Defined in: [sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

# Variable: BASE\_TIMESTAMP\_REF

```ts
const BASE_TIMESTAMP_REF: 251 = 0xfb;
```

Defined in: [sensors/bno086/reports.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L63)

# Variable: CONTINUATION\_BIT

```ts
const CONTINUATION_BIT: 32768 = 0x8000;
```

Defined in: [sensors/bno086/shtp.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L53)

# Variable: CONTROL\_RESPONSE\_LENGTHS

```ts
const CONTROL_RESPONSE_LENGTHS: ReadonlyMap<number, number>;
```

Defined in: [sensors/bno086/sh2.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L40)

Fixed lengths of the sensor→host control reports (contract 05 §6). One
control cargo may carry several back to back — the Product ID Response comes
once per software component (a BNO085 sends four, split over two cargos).

# Variable: COUNTS\_CLEAR

```ts
const COUNTS_CLEAR: 1 = 1;
```

Defined in: [sensors/bno086/sh2.ts:102](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L102)

# Variable: COUNTS\_GET

```ts
const COUNTS_GET: 0 = 0;
```

Defined in: [sensors/bno086/sh2.ts:101](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L101)

Counter subcommands (command 0x02, P0).

# Variable: FLUSH\_COMPLETED

```ts
const FLUSH_COMPLETED: 239 = 0xef;
```

Defined in: [sensors/bno086/reports.ts:65](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L65)

# Variable: GYRO\_RV\_ANGVEL\_Q

```ts
const GYRO_RV_ANGVEL_Q: 10 = 10;
```

Defined in: [sensors/bno086/reports.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L92)

# Variable: LENGTH\_MASK

```ts
const LENGTH_MASK: 32767 = 0x7fff;
```

Defined in: [sensors/bno086/shtp.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L52)

# Variable: MAX\_TX\_FRAME

```ts
const MAX_TX_FRAME: 64 = 64;
```

Defined in: [sensors/bno086/shtp.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L57)

Host->sensor frames must fit one MCU transmit slot (ERRATA E2: 2 x 64 B).

# Variable: METADATA\_RECORDS

```ts
const METADATA_RECORDS: Record<number, number>;
```

Defined in: [sensors/bno086/sh2.ts:464](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L464)

Per-sensor metadata FRS record IDs (subset used by getMetadata()).

# Variable: ME\_CAL\_GET

```ts
const ME_CAL_GET: 1 = 0x01;
```

Defined in: [sensors/bno086/sh2.ts:393](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L393)

Subcommand: report current ME calibration config.

# Variable: MOTION\_INTENT\_NAMES

```ts
const MOTION_INTENT_NAMES: Record<number, string>;
```

Defined in: [sensors/bno086/reports.ts:149](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L149)

# Variable: MOTION\_REQUEST\_NAMES

```ts
const MOTION_REQUEST_NAMES: Record<number, string>;
```

Defined in: [sensors/bno086/reports.ts:142](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L142)

SH-2 §6.5.45.2. Value 2 (stay stationary, optional) is deprecated by CEVA:
"ignore this request".

# Variable: NUM\_CHANNELS

```ts
const NUM_CHANNELS: 6 = 6;
```

Defined in: [sensors/bno086/shtp.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L54)

# Variable: Q\_POINTS

```ts
const Q_POINTS: Record<number, number>;
```

Defined in: [sensors/bno086/reports.ts:68](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L68)

Q point of the primary fields (value = raw / 2**Q); see module docs.

# Variable: RATE\_HIGH\_FACTOR

```ts
const RATE_HIGH_FACTOR: 2.1 = 2.1;
```

Defined in: [sensors/bno086/bno086.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L71)

# Variable: RATE\_LOW\_FACTOR

```ts
const RATE_LOW_FACTOR: 0.9 = 0.9;
```

Defined in: [sensors/bno086/bno086.ts:70](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L70)

# Variable: REPORT\_LENGTHS

```ts
const REPORT_LENGTHS: Record<number, number>;
```

Defined in: [sensors/bno086/reports.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L98)

Total report length on the wire, 4-byte SH-2 header included
(sh2 reference driver report-length table).

# Variable: RV\_ACCURACY\_Q

```ts
const RV_ACCURACY_Q: 12 = 12;
```

Defined in: [sensors/bno086/reports.ts:91](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L91)

# Variable: SHTP\_HEADER\_SIZE

```ts
const SHTP_HEADER_SIZE: 4 = 4;
```

Defined in: [sensors/bno086/shtp.ts:51](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L51)

# Variable: STABILITY\_NAMES

```ts
const STABILITY_NAMES: Record<number, string>;
```

Defined in: [sensors/bno086/reports.ts:157](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L157)

# Variable: TIMESTAMP\_REBASE

```ts
const TIMESTAMP_REBASE: 250 = 0xfa;
```

Defined in: [sensors/bno086/reports.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L64)

