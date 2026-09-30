# BNO055 (IMU) — API reference

The public API for the BNO055 IMU: the `Bno055` device and its sample
type, the register map and codecs (units, axis remap, calibration
profile, …) and the register-bridge wire codecs. Discovery, device-base,
transport and other cross-sensor symbols live in the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Enumerations

- [Bno055Cmd](enumerations/Bno055Cmd.md)
- [Bno055Rpt](enumerations/Bno055Rpt.md)
- [Bno055OprMode](enumerations/Bno055OprMode.md)
- [Bno055PwrMode](enumerations/Bno055PwrMode.md)
- [Bno055TempSource](enumerations/Bno055TempSource.md)

## Classes

- [Bno055](classes/Bno055.md)

## Interfaces

- [Bno055RegData](interfaces/Bno055RegData.md)
- [Bno055Info](interfaces/Bno055Info.md)
- [Bno055StreamData](interfaces/Bno055StreamData.md)
- [Bno055Sample](interfaces/Bno055Sample.md)
- [Bno055Options](interfaces/Bno055Options.md)
- [Bno055ConfigureOptions](interfaces/Bno055ConfigureOptions.md)
- [Bno055Units](interfaces/Bno055Units.md)
- [Bno055CalibStatus](interfaces/Bno055CalibStatus.md)
- [Bno055CalibrationProfile](interfaces/Bno055CalibrationProfile.md)
- [Bno055AxisRemap](interfaces/Bno055AxisRemap.md)
- [Bno055AccelConfig](interfaces/Bno055AccelConfig.md)
- [Bno055GyroConfig](interfaces/Bno055GyroConfig.md)
- [Bno055MagConfig](interfaces/Bno055MagConfig.md)
- [Bno055SystemStatus](interfaces/Bno055SystemStatus.md)
- [Bno055RawBlock](interfaces/Bno055RawBlock.md)

## Variables

- [BNO055\_XFER\_MAX](variables/BNO055_XFER_MAX.md)
- [BNO055\_TRIGGER\_TIMER](variables/BNO055_TRIGGER_TIMER.md)
- [BNO055\_TRIGGER\_INT](variables/BNO055_TRIGGER_INT.md)
- [BNO055\_RESET\_TIMEOUT\_MS](variables/BNO055_RESET_TIMEOUT_MS.md)
- [BNO055\_I2C\_ERROR\_NAMES](variables/BNO055_I2C_ERROR_NAMES.md)
- [BNO055\_EXPECTED\_IDS](variables/BNO055_EXPECTED_IDS.md)
- [BNO055\_INFO\_SIZE](variables/BNO055_INFO_SIZE.md)
- [BNO055\_REG\_CHIP\_ID](variables/BNO055_REG_CHIP_ID.md)
- [BNO055\_REG\_PAGE\_ID](variables/BNO055_REG_PAGE_ID.md)
- [BNO055\_REG\_ACC\_DATA](variables/BNO055_REG_ACC_DATA.md)
- [BNO055\_REG\_MAG\_DATA](variables/BNO055_REG_MAG_DATA.md)
- [BNO055\_REG\_GYR\_DATA](variables/BNO055_REG_GYR_DATA.md)
- [BNO055\_REG\_EUL\_DATA](variables/BNO055_REG_EUL_DATA.md)
- [BNO055\_REG\_QUA\_DATA](variables/BNO055_REG_QUA_DATA.md)
- [BNO055\_REG\_LIA\_DATA](variables/BNO055_REG_LIA_DATA.md)
- [BNO055\_REG\_GRV\_DATA](variables/BNO055_REG_GRV_DATA.md)
- [BNO055\_REG\_TEMP](variables/BNO055_REG_TEMP.md)
- [BNO055\_REG\_CALIB\_STAT](variables/BNO055_REG_CALIB_STAT.md)
- [BNO055\_REG\_ST\_RESULT](variables/BNO055_REG_ST_RESULT.md)
- [BNO055\_REG\_INT\_STA](variables/BNO055_REG_INT_STA.md)
- [BNO055\_REG\_SYS\_CLK\_STATUS](variables/BNO055_REG_SYS_CLK_STATUS.md)
- [BNO055\_REG\_SYS\_STATUS](variables/BNO055_REG_SYS_STATUS.md)
- [BNO055\_REG\_SYS\_ERR](variables/BNO055_REG_SYS_ERR.md)
- [BNO055\_REG\_UNIT\_SEL](variables/BNO055_REG_UNIT_SEL.md)
- [BNO055\_REG\_OPR\_MODE](variables/BNO055_REG_OPR_MODE.md)
- [BNO055\_REG\_PWR\_MODE](variables/BNO055_REG_PWR_MODE.md)
- [BNO055\_REG\_SYS\_TRIGGER](variables/BNO055_REG_SYS_TRIGGER.md)
- [BNO055\_REG\_TEMP\_SOURCE](variables/BNO055_REG_TEMP_SOURCE.md)
- [BNO055\_REG\_AXIS\_MAP\_CONFIG](variables/BNO055_REG_AXIS_MAP_CONFIG.md)
- [BNO055\_REG\_AXIS\_MAP\_SIGN](variables/BNO055_REG_AXIS_MAP_SIGN.md)
- [BNO055\_REG\_SIC\_MATRIX](variables/BNO055_REG_SIC_MATRIX.md)
- [BNO055\_REG\_CALIB\_PROFILE](variables/BNO055_REG_CALIB_PROFILE.md)
- [BNO055\_CALIB\_PROFILE\_LEN](variables/BNO055_CALIB_PROFILE_LEN.md)
- [BNO055\_REG1\_ACC\_CONFIG](variables/BNO055_REG1_ACC_CONFIG.md)
- [BNO055\_REG1\_MAG\_CONFIG](variables/BNO055_REG1_MAG_CONFIG.md)
- [BNO055\_REG1\_GYR\_CONFIG\_0](variables/BNO055_REG1_GYR_CONFIG_0.md)
- [BNO055\_REG1\_GYR\_CONFIG\_1](variables/BNO055_REG1_GYR_CONFIG_1.md)
- [BNO055\_REG1\_INT\_MSK](variables/BNO055_REG1_INT_MSK.md)
- [BNO055\_REG1\_INT\_EN](variables/BNO055_REG1_INT_EN.md)
- [BNO055\_REG1\_INT\_SETTINGS\_FIRST](variables/BNO055_REG1_INT_SETTINGS_FIRST.md)
- [BNO055\_REG1\_INT\_SETTINGS\_LAST](variables/BNO055_REG1_INT_SETTINGS_LAST.md)
- [BNO055\_REG1\_UNIQUE\_ID](variables/BNO055_REG1_UNIQUE_ID.md)
- [BNO055\_UNIQUE\_ID\_LEN](variables/BNO055_UNIQUE_ID_LEN.md)
- [BNO055\_FULL\_BLOCK](variables/BNO055_FULL_BLOCK.md)
- [BNO055\_QUAT\_BLOCK](variables/BNO055_QUAT_BLOCK.md)
- [BNO055\_SYS\_TRIGGER\_SELF\_TEST](variables/BNO055_SYS_TRIGGER_SELF_TEST.md)
- [BNO055\_SYS\_TRIGGER\_RST\_SYS](variables/BNO055_SYS_TRIGGER_RST_SYS.md)
- [BNO055\_SYS\_TRIGGER\_RST\_INT](variables/BNO055_SYS_TRIGGER_RST_INT.md)
- [BNO055\_SYS\_TRIGGER\_CLK\_SEL](variables/BNO055_SYS_TRIGGER_CLK_SEL.md)
- [BNO055\_INT](variables/BNO055_INT.md)
- [BNO055\_ST](variables/BNO055_ST.md)
- [BNO055\_EXPECTED\_SELF\_TEST](variables/BNO055_EXPECTED_SELF_TEST.md)
- [BNO055\_MODE\_SWITCH\_FROM\_CONFIG\_MS](variables/BNO055_MODE_SWITCH_FROM_CONFIG_MS.md)
- [BNO055\_MODE\_SWITCH\_TO\_CONFIG\_MS](variables/BNO055_MODE_SWITCH_TO_CONFIG_MS.md)
- [BNO055\_SELF\_TEST\_MS](variables/BNO055_SELF_TEST_MS.md)
- [BNO055\_SYS\_STATUS\_BOOTING](variables/BNO055_SYS_STATUS_BOOTING.md)
- [BNO055\_BOOT\_SETTLE\_TIMEOUT\_MS](variables/BNO055_BOOT_SETTLE_TIMEOUT_MS.md)
- [BNO055\_STUCK\_SELF\_TEST\_POLLS](variables/BNO055_STUCK_SELF_TEST_POLLS.md)
- [BNO055\_FUSION\_START\_TIMEOUT\_MS](variables/BNO055_FUSION_START_TIMEOUT_MS.md)
- [BNO055\_SYS\_STATUS\_NAMES](variables/BNO055_SYS_STATUS_NAMES.md)
- [BNO055\_SYS\_ERR\_NAMES](variables/BNO055_SYS_ERR_NAMES.md)
- [BNO055\_DEFAULT\_UNITS](variables/BNO055_DEFAULT_UNITS.md)
- [BNO055\_MAG\_LSB](variables/BNO055_MAG_LSB.md)
- [BNO055\_QUAT\_LSB](variables/BNO055_QUAT_LSB.md)
- [BNO055\_FUSION\_ACCEL\_LSB](variables/BNO055_FUSION_ACCEL_LSB.md)
- [BNO055\_SIC\_IDENTITY](variables/BNO055_SIC_IDENTITY.md)
- [BNO055\_AXIS\_X](variables/BNO055_AXIS_X.md)
- [BNO055\_AXIS\_Y](variables/BNO055_AXIS_Y.md)
- [BNO055\_AXIS\_Z](variables/BNO055_AXIS_Z.md)
- [BNO055\_DEFAULT\_AXIS\_REMAP](variables/BNO055_DEFAULT_AXIS_REMAP.md)
- [BNO055\_PLACEMENTS](variables/BNO055_PLACEMENTS.md)
- [BNO055\_ACC\_RANGE\_G](variables/BNO055_ACC_RANGE_G.md)
- [BNO055\_ACC\_BANDWIDTH\_HZ](variables/BNO055_ACC_BANDWIDTH_HZ.md)
- [BNO055\_GYR\_RANGE\_DPS](variables/BNO055_GYR_RANGE_DPS.md)
- [BNO055\_GYR\_BANDWIDTH\_HZ](variables/BNO055_GYR_BANDWIDTH_HZ.md)
- [BNO055\_MAG\_RATE\_HZ](variables/BNO055_MAG_RATE_HZ.md)

## Functions

- [packBno055ReadReg](functions/packBno055ReadReg.md)
- [packBno055WriteReg](functions/packBno055WriteReg.md)
- [packBno055StartStream](functions/packBno055StartStream.md)
- [unpackBno055RegData](functions/unpackBno055RegData.md)
- [unpackBno055Info](functions/unpackBno055Info.md)
- [bno055IdsOk](functions/bno055IdsOk.md)
- [bno055SwRevText](functions/bno055SwRevText.md)
- [unpackBno055Stream](functions/unpackBno055Stream.md)
- [decodeBno055Sample](functions/decodeBno055Sample.md)
- [bno055IsFusion](functions/bno055IsFusion.md)
- [packBno055Units](functions/packBno055Units.md)
- [unpackBno055Units](functions/unpackBno055Units.md)
- [bno055Lsb](functions/bno055Lsb.md)
- [unpackBno055CalibStatus](functions/unpackBno055CalibStatus.md)
- [packBno055CalibStatus](functions/packBno055CalibStatus.md)
- [bno055FullyCalibrated](functions/bno055FullyCalibrated.md)
- [packBno055CalibrationProfile](functions/packBno055CalibrationProfile.md)
- [unpackBno055CalibrationProfile](functions/unpackBno055CalibrationProfile.md)
- [packBno055SicMatrix](functions/packBno055SicMatrix.md)
- [unpackBno055SicMatrix](functions/unpackBno055SicMatrix.md)
- [packBno055AxisRemap](functions/packBno055AxisRemap.md)
- [unpackBno055AxisRemap](functions/unpackBno055AxisRemap.md)
- [bno055Placement](functions/bno055Placement.md)
- [packBno055AccelConfig](functions/packBno055AccelConfig.md)
- [unpackBno055AccelConfig](functions/unpackBno055AccelConfig.md)
- [packBno055GyroConfig](functions/packBno055GyroConfig.md)
- [unpackBno055GyroConfig](functions/unpackBno055GyroConfig.md)
- [packBno055MagConfig](functions/packBno055MagConfig.md)
- [unpackBno055MagConfig](functions/unpackBno055MagConfig.md)
- [makeBno055SystemStatus](functions/makeBno055SystemStatus.md)
- [decodeBno055Block](functions/decodeBno055Block.md)

# Class: Bno055

Defined in: [sensors/bno055/bno055.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L196)

BNO055 absolute-orientation IMU.

Typical use: `configure()` (CONFIG → units → axis remap → optional
calibration profile → NDOF), then `startStream(10)` and read `samples()` /
`onSample`, or poll `readSample()`. Streaming is timer-driven: the
data-ready interrupt does not exist on the sensor firmware these boards
carry (03.11).

Multi-step register sequences (page switches, CONFIG round trips) are
serialised internally, so concurrent calls cannot interleave mid-sequence.
Page 1 is refused while a stream runs.

## Extends

- `DepzDevice`

## Constructors

### Constructor

```ts
new Bno055(transport, opts?): Bno055;
```

Defined in: [sensors/bno055/bno055.ts:208](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L208)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | [`Bno055Options`](../interfaces/Bno055Options.md) |

#### Returns

`Bno055`

#### Overrides

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

Defined in: [device/device.ts:498](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L498)

##### Returns

`TimeSync` \| `null`

#### Inherited from

```ts
DepzDevice.timeSync
```

***

### streaming

#### Get Signature

```ts
get streaming(): boolean;
```

Defined in: [sensors/bno055/bno055.ts:558](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L558)

##### Returns

`boolean`

***

### streamParseErrors

#### Get Signature

```ts
get streamParseErrors(): number;
```

Defined in: [sensors/bno055/bno055.ts:604](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L604)

Stream reports dropped because they did not decode (short block).

##### Returns

`number`

***

### streamDroppedCounts

#### Get Signature

```ts
get streamDroppedCounts(): number[];
```

Defined in: [sensors/bno055/bno055.ts:608](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L608)

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

### onTeardown()

```ts
protected onTeardown(error): void;
```

Defined in: [device/device.ts:234](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L234)

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

Defined in: [device/device.ts:354](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L354)

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

Defined in: [device/device.ts:367](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L367)

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

Defined in: [device/device.ts:374](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L374)

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

Defined in: [device/device.ts:390](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L390)

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

Defined in: [device/device.ts:433](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L433)

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

Defined in: [device/device.ts:438](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L438)

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

Defined in: [device/device.ts:449](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L449)

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

Defined in: [device/device.ts:455](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L455)

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

Defined in: [device/device.ts:461](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L461)

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

Defined in: [device/device.ts:468](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L468)

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

Defined in: [device/device.ts:473](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L473)

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

Defined in: [device/device.ts:481](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L481)

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

Defined in: [device/device.ts:503](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L503)

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

Defined in: [device/device.ts:508](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L508)

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

Defined in: [device/device.ts:515](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L515)

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

Defined in: [device/device.ts:519](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L519)

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

Defined in: [device/device.ts:525](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L525)

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

Defined in: [device/device.ts:530](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L530)

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

Defined in: [device/device.ts:539](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L539)

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

### bridgeInfo()

```ts
bridgeInfo(): Promise<Bno055Info>;
```

Defined in: [sensors/bno055/bno055.ts:216](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L216)

RPT_BNO_INFO: chip ids, sensor firmware revision and bridge counters.

#### Returns

`Promise`\<[`Bno055Info`](../interfaces/Bno055Info.md)\>

***

### isAlive()

```ts
isAlive(): Promise<boolean>;
```

Defined in: [sensors/bno055/bno055.ts:223](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L223)

True when the bridge passed the chip-ID handshake and the ids match.

#### Returns

`Promise`\<`boolean`\>

***

### resetSensor()

```ts
resetSensor(): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:238](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L238)

Hardware reset via nRESET, then wait out the sensor's own boot tail
(contract 13 §5). Stops any stream; the sensor comes back in CONFIG with
power-on units — call configure() or restoreConfiguration().

#### Returns

`Promise`\<`void`\>

***

### readRegisters()

```ts
readRegisters(
   addr, 
   length, 
page?): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [sensors/bno055/bno055.ts:253](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L253)

Read `length` bytes at `addr` on `page` (page 1 refused while streaming).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `addr` | `number` | `undefined` |
| `length` | `number` | `undefined` |
| `page` | `number` | `0` |

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### writeRegisters()

```ts
writeRegisters(
   addr, 
   data, 
page?): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:258](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L258)

Write `data` at `addr` on `page`. Most config registers need CONFIG mode.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `addr` | `number` | `undefined` |
| `data` | `Uint8Array` | `undefined` |
| `page` | `number` | `0` |

#### Returns

`Promise`\<`void`\>

***

### readRegister()

```ts
readRegister(addr, page?): Promise<number>;
```

Defined in: [sensors/bno055/bno055.ts:262](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L262)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `addr` | `number` | `undefined` |
| `page` | `number` | `0` |

#### Returns

`Promise`\<`number`\>

***

### writeRegister()

```ts
writeRegister(
   addr, 
   value, 
page?): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:266](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L266)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `addr` | `number` | `undefined` |
| `value` | `number` | `undefined` |
| `page` | `number` | `0` |

#### Returns

`Promise`\<`void`\>

***

### getOperationMode()

```ts
getOperationMode(): Promise<Bno055OprMode>;
```

Defined in: [sensors/bno055/bno055.ts:272](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L272)

#### Returns

`Promise`\<[`Bno055OprMode`](../enumerations/Bno055OprMode.md)\>

***

### setOperationMode()

```ts
setOperationMode(mode): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:282](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L282)

Switch OPR_MODE and wait out the switching time; into a fusion mode also
wait (≤ 1 s) for the fusion outputs, zero for ~70 ms after CONFIG. The
sensor ignores a direct write from one operating mode to another
(measured: NDOF → AMG stays NDOF), so such a switch goes through CONFIG.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `mode` | [`Bno055OprMode`](../enumerations/Bno055OprMode.md) |

#### Returns

`Promise`\<`void`\>

***

### getPowerMode()

```ts
getPowerMode(): Promise<Bno055PwrMode>;
```

Defined in: [sensors/bno055/bno055.ts:293](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L293)

#### Returns

`Promise`\<[`Bno055PwrMode`](../enumerations/Bno055PwrMode.md)\>

***

### setPowerMode()

```ts
setPowerMode(mode): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:297](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L297)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `mode` | [`Bno055PwrMode`](../enumerations/Bno055PwrMode.md) |

#### Returns

`Promise`\<`void`\>

***

### getUnits()

```ts
getUnits(): Promise<Bno055Units>;
```

Defined in: [sensors/bno055/bno055.ts:301](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L301)

#### Returns

`Promise`\<[`Bno055Units`](../interfaces/Bno055Units.md)\>

***

### setUnits()

```ts
setUnits(units): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:307](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L307)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `units` | [`Bno055Units`](../interfaces/Bno055Units.md) |

#### Returns

`Promise`\<`void`\>

***

### getAxisRemap()

```ts
getAxisRemap(): Promise<Bno055AxisRemap>;
```

Defined in: [sensors/bno055/bno055.ts:316](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L316)

#### Returns

`Promise`\<[`Bno055AxisRemap`](../interfaces/Bno055AxisRemap.md)\>

***

### setAxisRemap()

```ts
setAxisRemap(remap): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:322](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L322)

An AxisRemap, or a datasheet placement "P0".."P7" (P1 = default).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `remap` | `string` \| [`Bno055AxisRemap`](../interfaces/Bno055AxisRemap.md) |

#### Returns

`Promise`\<`void`\>

***

### getTemperatureSource()

```ts
getTemperatureSource(): Promise<Bno055TempSource>;
```

Defined in: [sensors/bno055/bno055.ts:328](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L328)

#### Returns

`Promise`\<[`Bno055TempSource`](../enumerations/Bno055TempSource.md)\>

***

### setTemperatureSource()

```ts
setTemperatureSource(source): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:332](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L332)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `source` | [`Bno055TempSource`](../enumerations/Bno055TempSource.md) |

#### Returns

`Promise`\<`void`\>

***

### configure()

```ts
configure(opts?): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:343](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L343)

The usual session setup: CONFIG → units → axis remap → calibration
profile → mode (default NDOF). In a fusion mode it resolves once the
fusion outputs are live. Remembered for restoreConfiguration().

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `opts` | [`Bno055ConfigureOptions`](../interfaces/Bno055ConfigureOptions.md) |

#### Returns

`Promise`\<`void`\>

***

### restoreConfiguration()

```ts
restoreConfiguration(): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:368](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L368)

Re-apply the last configure() — after resetSensor(), or when
bridgeInfo().sensorResets rose (the bridge's bus recovery pulses nRESET).

#### Returns

`Promise`\<`void`\>

***

### systemStatus()

```ts
systemStatus(): Promise<Bno055SystemStatus>;
```

Defined in: [sensors/bno055/bno055.ts:378](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L378)

ST_RESULT, SYS_CLK_STATUS, SYS_STATUS, SYS_ERR (INT_STA skipped).

#### Returns

`Promise`\<[`Bno055SystemStatus`](../interfaces/Bno055SystemStatus.md)\>

***

### selfTest()

```ts
selfTest(): Promise<Bno055SystemStatus>;
```

Defined in: [sensors/bno055/bno055.ts:386](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L386)

Built-in self-test (datasheet §3.9.2): CONFIG, SYS_TRIGGER SELF_TEST,
~400 ms, then read the verdict. The mode is restored. Not while streaming.

#### Returns

`Promise`\<[`Bno055SystemStatus`](../interfaces/Bno055SystemStatus.md)\>

***

### calibrationStatus()

```ts
calibrationStatus(): Promise<Bno055CalibStatus>;
```

Defined in: [sensors/bno055/bno055.ts:405](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L405)

#### Returns

`Promise`\<[`Bno055CalibStatus`](../interfaces/Bno055CalibStatus.md)\>

***

### readCalibrationProfile()

```ts
readCalibrationProfile(): Promise<Bno055CalibrationProfile>;
```

Defined in: [sensors/bno055/bno055.ts:410](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L410)

Offsets and radii (0x55..0x6A) — CONFIG only; the driver switches there and back.

#### Returns

`Promise`\<[`Bno055CalibrationProfile`](../interfaces/Bno055CalibrationProfile.md)\>

***

### writeCalibrationProfile()

```ts
writeCalibrationProfile(profile): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:424](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L424)

Restore a stored profile: CONFIG, all 22 bytes, back to the previous
mode. A starting point — fusion refines it as soon as it resumes.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `profile` | [`Bno055CalibrationProfile`](../interfaces/Bno055CalibrationProfile.md) |

#### Returns

`Promise`\<`void`\>

***

### getSicMatrix()

```ts
getSicMatrix(): Promise<number[]>;
```

Defined in: [sensors/bno055/bno055.ts:430](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L430)

#### Returns

`Promise`\<`number`[]\>

***

### setSicMatrix()

```ts
setSicMatrix(matrix?): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:434](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L434)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `matrix` | readonly `number`[] | `BNO055_SIC_IDENTITY` |

#### Returns

`Promise`\<`void`\>

***

### getAccelConfig()

```ts
getAccelConfig(): Promise<Bno055AccelConfig>;
```

Defined in: [sensors/bno055/bno055.ts:440](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L440)

#### Returns

`Promise`\<[`Bno055AccelConfig`](../interfaces/Bno055AccelConfig.md)\>

***

### setAccelConfig()

```ts
setAccelConfig(c): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:444](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L444)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `c` | [`Bno055AccelConfig`](../interfaces/Bno055AccelConfig.md) |

#### Returns

`Promise`\<`void`\>

***

### getGyroConfig()

```ts
getGyroConfig(): Promise<Bno055GyroConfig>;
```

Defined in: [sensors/bno055/bno055.ts:448](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L448)

#### Returns

`Promise`\<[`Bno055GyroConfig`](../interfaces/Bno055GyroConfig.md)\>

***

### setGyroConfig()

```ts
setGyroConfig(c): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:452](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L452)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `c` | [`Bno055GyroConfig`](../interfaces/Bno055GyroConfig.md) |

#### Returns

`Promise`\<`void`\>

***

### getMagConfig()

```ts
getMagConfig(): Promise<Bno055MagConfig>;
```

Defined in: [sensors/bno055/bno055.ts:456](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L456)

#### Returns

`Promise`\<[`Bno055MagConfig`](../interfaces/Bno055MagConfig.md)\>

***

### setMagConfig()

```ts
setMagConfig(c): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:460](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L460)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `c` | [`Bno055MagConfig`](../interfaces/Bno055MagConfig.md) |

#### Returns

`Promise`\<`void`\>

***

### uniqueId()

```ts
uniqueId(): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [sensors/bno055/bno055.ts:465](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L465)

The chip's 16-byte unique id (page 1, 0x50..0x5F).

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### getInterruptEnable()

```ts
getInterruptEnable(): Promise<number>;
```

Defined in: [sensors/bno055/bno055.ts:471](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L471)

#### Returns

`Promise`\<`number`\>

***

### setInterruptEnable()

```ts
setInterruptEnable(mask): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:475](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L475)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `mask` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getInterruptMask()

```ts
getInterruptMask(): Promise<number>;
```

Defined in: [sensors/bno055/bno055.ts:479](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L479)

#### Returns

`Promise`\<`number`\>

***

### setInterruptMask()

```ts
setInterruptMask(mask): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:483](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L483)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `mask` | `number` |

#### Returns

`Promise`\<`void`\>

***

### setInterruptSetting()

```ts
setInterruptSetting(register, value): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:488](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L488)

One raw motion-interrupt setting (page 1, 0x11..0x1F). Not while streaming.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `register` | `number` |
| `value` | `number` |

#### Returns

`Promise`\<`void`\>

***

### readInterruptStatus()

```ts
readInterruptStatus(): Promise<number>;
```

Defined in: [sensors/bno055/bno055.ts:496](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L496)

INT_STA — which interrupts fired. Clears on read.

#### Returns

`Promise`\<`number`\>

***

### clearInterrupt()

```ts
clearInterrupt(): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:501](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L501)

SYS_TRIGGER RST_INT: reset the interrupt status bits and the INT pin.

#### Returns

`Promise`\<`void`\>

***

### readSample()

```ts
readSample(block?): Promise<Bno055Sample>;
```

Defined in: [sensors/bno055/bno055.ts:511](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L511)

Poll one register block (default the full 46-byte block) and decode it.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `block` | readonly \[`number`, `number`\] | `BNO055_FULL_BLOCK` |

#### Returns

`Promise`\<[`Bno055Sample`](../interfaces/Bno055Sample.md)\>

***

### readQuaternion()

```ts
readQuaternion(): Promise<[number, number, number, number]>;
```

Defined in: [sensors/bno055/bno055.ts:520](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L520)

(w, x, y, z) — the cheapest orientation read.

#### Returns

`Promise`\<\[`number`, `number`, `number`, `number`\]\>

***

### startStream()

```ts
startStream(
   periodMs?, 
   block?, 
trigger?): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:529](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L529)

Arm the bridge: read `block` every `periodMs` and push it. Fusion runs at
100 Hz, so 10 ms is the useful floor. `trigger = BNO055_TRIGGER_INT`
reads on the INT edge with `periodMs` as a watchdog. Replaces a stream.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `periodMs` | `number` | `10` |
| `block` | readonly \[`number`, `number`\] | `BNO055_FULL_BLOCK` |
| `trigger` | `number` | `BNO055_TRIGGER_TIMER` |

#### Returns

`Promise`\<`void`\>

***

### stopStream()

```ts
stopStream(): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:549](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L549)

#### Returns

`Promise`\<`void`\>

***

### onSample()

```ts
onSample(cb): () => void;
```

Defined in: [sensors/bno055/bno055.ts:563](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L563)

Subscribe to streamed samples (read-pump context; don't block).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`s`) => `void` |

#### Returns

() => `void`

***

### samples()

```ts
samples(maxsize?): StreamQueue<Bno055Sample>;
```

Defined in: [sensors/bno055/bno055.ts:571](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L571)

Async iterator over streamed samples — bounded, drop-oldest.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `maxsize` | `number` | `256` |

#### Returns

`StreamQueue`\<[`Bno055Sample`](../interfaces/Bno055Sample.md)\>

***

### getSample()

```ts
getSample(timeoutMs?): Promise<Bno055Sample>;
```

Defined in: [sensors/bno055/bno055.ts:582](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L582)

Wait for the next streamed sample.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `1000` |

#### Returns

`Promise`\<[`Bno055Sample`](../interfaces/Bno055Sample.md)\>

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [sensors/bno055/bno055.ts:739](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L739)

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

# Enumeration: Bno055Cmd

Defined in: [protocol/bno055.ts:6](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L6)

BNO055 register-bridge wire codecs (contracts/13_SENSOR_BNO055.md).
Mirrors the Python reference `depz_sensor_sdk.protocol.bno055`.

## Enumeration Members

### ReadReg

```ts
ReadReg: 50;
```

Defined in: [protocol/bno055.ts:7](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L7)

***

### WriteReg

```ts
WriteReg: 51;
```

Defined in: [protocol/bno055.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L8)

***

### Reset

```ts
Reset: 52;
```

Defined in: [protocol/bno055.ts:9](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L9)

***

### StartStream

```ts
StartStream: 53;
```

Defined in: [protocol/bno055.ts:10](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L10)

***

### StopStream

```ts
StopStream: 54;
```

Defined in: [protocol/bno055.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L11)

***

### GetInfo

```ts
GetInfo: 55;
```

Defined in: [protocol/bno055.ts:12](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L12)

# Enumeration: Bno055OprMode

Defined in: [sensors/bno055/regs.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L87)

OPR_MODE (0x3D) bits 3:0.

## Enumeration Members

### Config

```ts
Config: 0;
```

Defined in: [sensors/bno055/regs.ts:88](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L88)

***

### AccOnly

```ts
AccOnly: 1;
```

Defined in: [sensors/bno055/regs.ts:89](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L89)

***

### MagOnly

```ts
MagOnly: 2;
```

Defined in: [sensors/bno055/regs.ts:90](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L90)

***

### GyroOnly

```ts
GyroOnly: 3;
```

Defined in: [sensors/bno055/regs.ts:91](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L91)

***

### AccMag

```ts
AccMag: 4;
```

Defined in: [sensors/bno055/regs.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L92)

***

### AccGyro

```ts
AccGyro: 5;
```

Defined in: [sensors/bno055/regs.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L93)

***

### MagGyro

```ts
MagGyro: 6;
```

Defined in: [sensors/bno055/regs.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L94)

***

### Amg

```ts
Amg: 7;
```

Defined in: [sensors/bno055/regs.ts:95](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L95)

***

### Imu

```ts
Imu: 8;
```

Defined in: [sensors/bno055/regs.ts:96](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L96)

***

### Compass

```ts
Compass: 9;
```

Defined in: [sensors/bno055/regs.ts:97](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L97)

***

### M4g

```ts
M4g: 10;
```

Defined in: [sensors/bno055/regs.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L98)

***

### NdofFmcOff

```ts
NdofFmcOff: 11;
```

Defined in: [sensors/bno055/regs.ts:99](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L99)

***

### Ndof

```ts
Ndof: 12;
```

Defined in: [sensors/bno055/regs.ts:100](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L100)

# Enumeration: Bno055PwrMode

Defined in: [sensors/bno055/regs.ts:108](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L108)

PWR_MODE (0x3E) bits 1:0.

## Enumeration Members

### Normal

```ts
Normal: 0;
```

Defined in: [sensors/bno055/regs.ts:109](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L109)

***

### LowPower

```ts
LowPower: 1;
```

Defined in: [sensors/bno055/regs.ts:110](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L110)

***

### Suspend

```ts
Suspend: 2;
```

Defined in: [sensors/bno055/regs.ts:111](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L111)

# Enumeration: Bno055Rpt

Defined in: [protocol/bno055.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L15)

## Enumeration Members

### RegData

```ts
RegData: 145;
```

Defined in: [protocol/bno055.ts:16](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L16)

***

### Info

```ts
Info: 146;
```

Defined in: [protocol/bno055.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L17)

***

### Stream

```ts
Stream: 147;
```

Defined in: [protocol/bno055.ts:18](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L18)

# Enumeration: Bno055TempSource

Defined in: [sensors/bno055/regs.ts:115](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L115)

TEMP_SOURCE (0x40) bits 1:0.

## Enumeration Members

### Accel

```ts
Accel: 0;
```

Defined in: [sensors/bno055/regs.ts:116](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L116)

***

### Gyro

```ts
Gyro: 1;
```

Defined in: [sensors/bno055/regs.ts:117](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L117)

# Function: bno055FullyCalibrated()

```ts
function bno055FullyCalibrated(s): boolean;
```

Defined in: [sensors/bno055/regs.ts:250](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L250)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `s` | [`Bno055CalibStatus`](../interfaces/Bno055CalibStatus.md) |

## Returns

`boolean`

# Function: bno055IdsOk()

```ts
function bno055IdsOk(info): boolean;
```

Defined in: [protocol/bno055.ts:149](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L149)

True when the four identity registers carry the BNO055 values.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `info` | [`Bno055Info`](../interfaces/Bno055Info.md) |

## Returns

`boolean`

# Function: bno055IsFusion()

```ts
function bno055IsFusion(mode): boolean;
```

Defined in: [sensors/bno055/regs.ts:103](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L103)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `mode` | [`Bno055OprMode`](../enumerations/Bno055OprMode.md) |

## Returns

`boolean`

# Function: bno055Lsb()

```ts
function bno055Lsb(u): {
  accel: number;
  gyro: number;
  euler: number;
  temp: number;
};
```

Defined in: [sensors/bno055/regs.ts:218](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L218)

LSB per unit (contract 13 §4.2).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `u` | [`Bno055Units`](../interfaces/Bno055Units.md) |

## Returns

```ts
{
  accel: number;
  gyro: number;
  euler: number;
  temp: number;
}
```

### accel

```ts
accel: number;
```

### gyro

```ts
gyro: number;
```

### euler

```ts
euler: number;
```

### temp

```ts
temp: number;
```

# Function: bno055Placement()

```ts
function bno055Placement(name): Bno055AxisRemap;
```

Defined in: [sensors/bno055/regs.ts:375](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L375)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `name` | `string` |

## Returns

[`Bno055AxisRemap`](../interfaces/Bno055AxisRemap.md)

# Function: bno055SwRevText()

```ts
function bno055SwRevText(swRev): string;
```

Defined in: [protocol/bno055.ts:157](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L157)

Sensor firmware revision as Bosch writes it: 0x0311 → "03.11".

## Parameters

| Parameter | Type |
| ------ | ------ |
| `swRev` | `number` |

## Returns

`string`

# Function: decodeBno055Block()

```ts
function decodeBno055Block(addr, data): Bno055RawBlock;
```

Defined in: [sensors/bno055/regs.ts:493](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L493)

Unpack whatever channels the register window starting at `addr` holds.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `data` | `Uint8Array` |

## Returns

[`Bno055RawBlock`](../interfaces/Bno055RawBlock.md)

# Function: decodeBno055Sample()

```ts
function decodeBno055Sample(
   timestampUs, 
   addr, 
   data, 
   units): Bno055Sample;
```

Defined in: [sensors/bno055/bno055.ts:143](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L143)

Decode one register window into a scaled sample.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `timestampUs` | `bigint` |
| `addr` | `number` |
| `data` | `Uint8Array` |
| `units` | [`Bno055Units`](../interfaces/Bno055Units.md) |

## Returns

[`Bno055Sample`](../interfaces/Bno055Sample.md)

# Function: makeBno055SystemStatus()

```ts
function makeBno055SystemStatus(
   selfTest, 
   clkStatus, 
   status, 
   error): Bno055SystemStatus;
```

Defined in: [sensors/bno055/regs.ts:445](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L445)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `selfTest` | `number` |
| `clkStatus` | `number` |
| `status` | `number` |
| `error` | `number` |

## Returns

[`Bno055SystemStatus`](../interfaces/Bno055SystemStatus.md)

# Function: packBno055AccelConfig()

```ts
function packBno055AccelConfig(c): number;
```

Defined in: [sensors/bno055/regs.ts:396](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L396)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `c` | [`Bno055AccelConfig`](../interfaces/Bno055AccelConfig.md) |

## Returns

`number`

# Function: packBno055AxisRemap()

```ts
function packBno055AxisRemap(a): [number, number];
```

Defined in: [sensors/bno055/regs.ts:342](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L342)

→ [AXIS_MAP_CONFIG, AXIS_MAP_SIGN]. The sensor keeps the old mapping when
one axis is used twice, so this refuses a non-permutation up front.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `a` | [`Bno055AxisRemap`](../interfaces/Bno055AxisRemap.md) |

## Returns

\[`number`, `number`\]

# Function: packBno055CalibStatus()

```ts
function packBno055CalibStatus(s): number;
```

Defined in: [sensors/bno055/regs.ts:246](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L246)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `s` | [`Bno055CalibStatus`](../interfaces/Bno055CalibStatus.md) |

## Returns

`number`

# Function: packBno055CalibrationProfile()

```ts
function packBno055CalibrationProfile(p): Uint8Array;
```

Defined in: [sensors/bno055/regs.ts:270](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L270)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | [`Bno055CalibrationProfile`](../interfaces/Bno055CalibrationProfile.md) |

## Returns

`Uint8Array`

# Function: packBno055GyroConfig()

```ts
function packBno055GyroConfig(c): Uint8Array;
```

Defined in: [sensors/bno055/regs.ts:410](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L410)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `c` | [`Bno055GyroConfig`](../interfaces/Bno055GyroConfig.md) |

## Returns

`Uint8Array`

# Function: packBno055MagConfig()

```ts
function packBno055MagConfig(c): number;
```

Defined in: [sensors/bno055/regs.ts:424](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L424)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `c` | [`Bno055MagConfig`](../interfaces/Bno055MagConfig.md) |

## Returns

`number`

# Function: packBno055ReadReg()

```ts
function packBno055ReadReg(addr, length): Uint8Array;
```

Defined in: [protocol/bno055.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L32)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `length` | `number` |

## Returns

`Uint8Array`

# Function: packBno055SicMatrix()

```ts
function packBno055SicMatrix(m): Uint8Array;
```

Defined in: [sensors/bno055/regs.ts:297](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L297)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `m` | readonly `number`[] |

## Returns

`Uint8Array`

# Function: packBno055StartStream()

```ts
function packBno055StartStream(
   trigger, 
   addr, 
   length, 
   periodMs): Uint8Array;
```

Defined in: [protocol/bno055.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L43)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `trigger` | `number` |
| `addr` | `number` |
| `length` | `number` |
| `periodMs` | `number` |

## Returns

`Uint8Array`

# Function: packBno055Units()

```ts
function packBno055Units(u): number;
```

Defined in: [sensors/bno055/regs.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L197)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `u` | [`Bno055Units`](../interfaces/Bno055Units.md) |

## Returns

`number`

# Function: packBno055WriteReg()

```ts
function packBno055WriteReg(addr, data): Uint8Array;
```

Defined in: [protocol/bno055.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L36)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `data` | `Uint8Array` |

## Returns

`Uint8Array`

# Function: unpackBno055AccelConfig()

```ts
function unpackBno055AccelConfig(v): Bno055AccelConfig;
```

Defined in: [sensors/bno055/regs.ts:400](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L400)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `v` | `number` |

## Returns

[`Bno055AccelConfig`](../interfaces/Bno055AccelConfig.md)

# Function: unpackBno055AxisRemap()

```ts
function unpackBno055AxisRemap(config, sign): Bno055AxisRemap;
```

Defined in: [sensors/bno055/regs.ts:352](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L352)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `config` | `number` |
| `sign` | `number` |

## Returns

[`Bno055AxisRemap`](../interfaces/Bno055AxisRemap.md)

# Function: unpackBno055CalibStatus()

```ts
function unpackBno055CalibStatus(v): Bno055CalibStatus;
```

Defined in: [sensors/bno055/regs.ts:242](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L242)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `v` | `number` |

## Returns

[`Bno055CalibStatus`](../interfaces/Bno055CalibStatus.md)

# Function: unpackBno055CalibrationProfile()

```ts
function unpackBno055CalibrationProfile(data): Bno055CalibrationProfile;
```

Defined in: [sensors/bno055/regs.ts:279](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L279)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

## Returns

[`Bno055CalibrationProfile`](../interfaces/Bno055CalibrationProfile.md)

# Function: unpackBno055GyroConfig()

```ts
function unpackBno055GyroConfig(b): Bno055GyroConfig;
```

Defined in: [sensors/bno055/regs.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L414)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `b` | `Uint8Array` |

## Returns

[`Bno055GyroConfig`](../interfaces/Bno055GyroConfig.md)

# Function: unpackBno055Info()

```ts
function unpackBno055Info(payload): Bno055Info;
```

Defined in: [protocol/bno055.ts:122](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L122)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Bno055Info`](../interfaces/Bno055Info.md)

# Function: unpackBno055MagConfig()

```ts
function unpackBno055MagConfig(v): Bno055MagConfig;
```

Defined in: [sensors/bno055/regs.ts:428](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L428)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `v` | `number` |

## Returns

[`Bno055MagConfig`](../interfaces/Bno055MagConfig.md)

# Function: unpackBno055RegData()

```ts
function unpackBno055RegData(payload): Bno055RegData;
```

Defined in: [protocol/bno055.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L67)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Bno055RegData`](../interfaces/Bno055RegData.md)

# Function: unpackBno055SicMatrix()

```ts
function unpackBno055SicMatrix(data): number[];
```

Defined in: [sensors/bno055/regs.ts:305](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L305)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

## Returns

`number`[]

# Function: unpackBno055Stream()

```ts
function unpackBno055Stream(payload): Bno055StreamData;
```

Defined in: [protocol/bno055.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L174)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Bno055StreamData`](../interfaces/Bno055StreamData.md)

# Function: unpackBno055Units()

```ts
function unpackBno055Units(value): Bno055Units;
```

Defined in: [sensors/bno055/regs.ts:207](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L207)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `value` | `number` |

## Returns

[`Bno055Units`](../interfaces/Bno055Units.md)

# Interface: Bno055AccelConfig

Defined in: [sensors/bno055/regs.ts:390](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L390)

Register codes: `range`/`bandwidth` index the tables above, `power` the datasheet mode list.

## Properties

### range

```ts
range: number;
```

Defined in: [sensors/bno055/regs.ts:391](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L391)

***

### bandwidth

```ts
bandwidth: number;
```

Defined in: [sensors/bno055/regs.ts:392](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L392)

***

### power

```ts
power: number;
```

Defined in: [sensors/bno055/regs.ts:393](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L393)

# Interface: Bno055AxisRemap

Defined in: [sensors/bno055/regs.ts:320](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L320)

Which physical axis feeds each output axis, and its sign: `x = AXIS_Y`
means "output X is the chip's Y axis".

## Properties

### x

```ts
x: number;
```

Defined in: [sensors/bno055/regs.ts:321](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L321)

***

### y

```ts
y: number;
```

Defined in: [sensors/bno055/regs.ts:322](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L322)

***

### z

```ts
z: number;
```

Defined in: [sensors/bno055/regs.ts:323](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L323)

***

### xNegative

```ts
xNegative: boolean;
```

Defined in: [sensors/bno055/regs.ts:324](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L324)

***

### yNegative

```ts
yNegative: boolean;
```

Defined in: [sensors/bno055/regs.ts:325](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L325)

***

### zNegative

```ts
zNegative: boolean;
```

Defined in: [sensors/bno055/regs.ts:326](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L326)

# Interface: Bno055CalibStatus

Defined in: [sensors/bno055/regs.ts:235](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L235)

CALIB_STAT (0x35): 0 = not calibrated … 3 = fully calibrated.

## Properties

### system

```ts
system: number;
```

Defined in: [sensors/bno055/regs.ts:236](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L236)

***

### gyro

```ts
gyro: number;
```

Defined in: [sensors/bno055/regs.ts:237](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L237)

***

### accel

```ts
accel: number;
```

Defined in: [sensors/bno055/regs.ts:238](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L238)

***

### mag

```ts
mag: number;
```

Defined in: [sensors/bno055/regs.ts:239](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L239)

# Interface: Bno055CalibrationProfile

Defined in: [sensors/bno055/regs.ts:262](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L262)

Sensor offsets and radii, registers 0x55..0x6A (22 bytes, all i16 LE).
Read after a full calibration (CONFIG mode), store, write back after
every power-on reset. A written profile is a starting point: fusion
refines it as soon as it resumes.

## Properties

### accelOffset

```ts
accelOffset: Vec3i;
```

Defined in: [sensors/bno055/regs.ts:263](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L263)

***

### magOffset

```ts
magOffset: Vec3i;
```

Defined in: [sensors/bno055/regs.ts:264](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L264)

***

### gyroOffset

```ts
gyroOffset: Vec3i;
```

Defined in: [sensors/bno055/regs.ts:265](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L265)

***

### accelRadius

```ts
accelRadius: number;
```

Defined in: [sensors/bno055/regs.ts:266](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L266)

***

### magRadius

```ts
magRadius: number;
```

Defined in: [sensors/bno055/regs.ts:267](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L267)

# Interface: Bno055ConfigureOptions

Defined in: [sensors/bno055/bno055.ts:175](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L175)

## Properties

### mode?

```ts
optional mode?: Bno055OprMode;
```

Defined in: [sensors/bno055/bno055.ts:176](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L176)

***

### units?

```ts
optional units?: Bno055Units;
```

Defined in: [sensors/bno055/bno055.ts:177](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L177)

***

### axisRemap?

```ts
optional axisRemap?: string | Bno055AxisRemap;
```

Defined in: [sensors/bno055/bno055.ts:179](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L179)

An axis remap, or a datasheet placement name "P0".."P7".

***

### calibration?

```ts
optional calibration?: Bno055CalibrationProfile;
```

Defined in: [sensors/bno055/bno055.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L180)

# Interface: Bno055GyroConfig

Defined in: [sensors/bno055/regs.ts:404](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L404)

## Properties

### range

```ts
range: number;
```

Defined in: [sensors/bno055/regs.ts:405](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L405)

***

### bandwidth

```ts
bandwidth: number;
```

Defined in: [sensors/bno055/regs.ts:406](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L406)

***

### power

```ts
power: number;
```

Defined in: [sensors/bno055/regs.ts:407](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L407)

# Interface: Bno055Info

Defined in: [protocol/bno055.ts:89](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L89)

RPT_BNO_INFO — sensor identity (registers 0x00..0x06) plus bridge
diagnostics. Counters are free-running and wrap silently. A rising
`sensorResets` means the bridge pulsed nRESET to recover the bus: the
sensor is back in CONFIG mode and the host must restore its configuration.

## Properties

### i2cAddr

```ts
i2cAddr: number;
```

Defined in: [protocol/bno055.ts:91](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L91)

7-bit sensor address in use (0x28).

***

### chipId

```ts
chipId: number;
```

Defined in: [protocol/bno055.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L92)

***

### accId

```ts
accId: number;
```

Defined in: [protocol/bno055.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L93)

***

### magId

```ts
magId: number;
```

Defined in: [protocol/bno055.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L94)

***

### gyrId

```ts
gyrId: number;
```

Defined in: [protocol/bno055.ts:95](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L95)

***

### swRev

```ts
swRev: number;
```

Defined in: [protocol/bno055.ts:97](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L97)

Sensor firmware, BCD: 0x0311 = 03.11.

***

### blRev

```ts
blRev: number;
```

Defined in: [protocol/bno055.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L98)

***

### initialized

```ts
initialized: number;
```

Defined in: [protocol/bno055.ts:100](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L100)

1 = chip-ID handshake passed.

***

### intLevel

```ts
intLevel: number;
```

Defined in: [protocol/bno055.ts:101](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L101)

***

### intEdges

```ts
intEdges: number;
```

Defined in: [protocol/bno055.ts:103](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L103)

EXTI rising edges (counted only while an INT stream is armed).

***

### readMinUs

```ts
readMinUs: number;
```

Defined in: [protocol/bno055.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L105)

Streamed block read timing since the last START_STREAM.

***

### readMaxUs

```ts
readMaxUs: number;
```

Defined in: [protocol/bno055.ts:106](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L106)

***

### readAvgUs

```ts
readAvgUs: number;
```

Defined in: [protocol/bno055.ts:107](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L107)

***

### txDropped

```ts
txDropped: number;
```

Defined in: [protocol/bno055.ts:109](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L109)

Packets refused by a full USB TX ring.

***

### i2cErrors

```ts
i2cErrors: number;
```

Defined in: [protocol/bno055.ts:110](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L110)

***

### slotsSkipped

```ts
slotsSkipped: number;
```

Defined in: [protocol/bno055.ts:112](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L112)

Stream slots dropped: bus still busy.

***

### busRecoveries

```ts
busRecoveries: number;
```

Defined in: [protocol/bno055.ts:113](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L113)

***

### lastI2cError

```ts
lastI2cError: number;
```

Defined in: [protocol/bno055.ts:114](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L114)

***

### sensorResets

```ts
sensorResets: number;
```

Defined in: [protocol/bno055.ts:116](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L116)

Rung-3 nRESET recoveries (sensor back in CONFIG).

***

### loopMaxUs

```ts
loopMaxUs: number;
```

Defined in: [protocol/bno055.ts:117](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L117)

# Interface: Bno055MagConfig

Defined in: [sensors/bno055/regs.ts:418](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L418)

## Properties

### rate

```ts
rate: number;
```

Defined in: [sensors/bno055/regs.ts:419](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L419)

***

### mode

```ts
mode: number;
```

Defined in: [sensors/bno055/regs.ts:420](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L420)

***

### power

```ts
power: number;
```

Defined in: [sensors/bno055/regs.ts:421](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L421)

# Interface: Bno055Options

Defined in: [sensors/bno055/bno055.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L170)

## Extends

- `DeviceOptions`

## Properties

### timeoutMs?

```ts
optional timeoutMs?: number;
```

Defined in: [device/device.ts:169](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L169)

#### Inherited from

```ts
DeviceOptions.timeoutMs
```

***

### txCrcType?

```ts
optional txCrcType?: CrcType;
```

Defined in: [device/device.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L170)

#### Inherited from

```ts
DeviceOptions.txCrcType
```

***

### sleepImpl?

```ts
optional sleepImpl?: (ms) => Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L172)

Sleep implementation (tests inject an instant one).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Interface: Bno055RawBlock

Defined in: [sensors/bno055/regs.ts:468](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L468)

Raw register values found in one block read. A channel is null when the
window `addr..addr+len` does not cover it completely.

## Properties

### accel

```ts
accel: Vec3i | null;
```

Defined in: [sensors/bno055/regs.ts:469](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L469)

***

### mag

```ts
mag: Vec3i | null;
```

Defined in: [sensors/bno055/regs.ts:470](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L470)

***

### gyro

```ts
gyro: Vec3i | null;
```

Defined in: [sensors/bno055/regs.ts:471](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L471)

***

### euler

```ts
euler: Vec3i | null;
```

Defined in: [sensors/bno055/regs.ts:473](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L473)

heading, roll, pitch

***

### quaternion

```ts
quaternion: [number, number, number, number] | null;
```

Defined in: [sensors/bno055/regs.ts:475](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L475)

w, x, y, z

***

### linearAccel

```ts
linearAccel: Vec3i | null;
```

Defined in: [sensors/bno055/regs.ts:476](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L476)

***

### gravity

```ts
gravity: Vec3i | null;
```

Defined in: [sensors/bno055/regs.ts:477](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L477)

***

### temperature

```ts
temperature: number | null;
```

Defined in: [sensors/bno055/regs.ts:478](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L478)

***

### calibStat

```ts
calibStat: number | null;
```

Defined in: [sensors/bno055/regs.ts:479](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L479)

# Interface: Bno055RegData

Defined in: [protocol/bno055.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L59)

RPT_BNO_REG_DATA payload.

## Properties

### cmd

```ts
cmd: number;
```

Defined in: [protocol/bno055.ts:61](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L61)

Echoed READ_REG opcode.

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [protocol/bno055.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L63)

MCU uptime at I2C-read completion.

***

### data

```ts
data: Uint8Array;
```

Defined in: [protocol/bno055.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L64)

# Interface: Bno055Sample

Defined in: [sensors/bno055/bno055.ts:117](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L117)

One decoded register block (streamed or polled). Channels are null when
the block did not cover them; values are scaled by `units` — the units in
force when the stream started (contract 13 §4.2).

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno055/bno055.ts:119](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L119)

MCU uptime: trigger time (stream) / read completion (poll).

***

### addr

```ts
addr: number;
```

Defined in: [sensors/bno055/bno055.ts:120](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L120)

***

### raw

```ts
raw: Uint8Array;
```

Defined in: [sensors/bno055/bno055.ts:121](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L121)

***

### units

```ts
units: Bno055Units;
```

Defined in: [sensors/bno055/bno055.ts:122](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L122)

***

### accel

```ts
accel: Vec3 | null;
```

Defined in: [sensors/bno055/bno055.ts:124](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L124)

m/s² or mg

***

### mag

```ts
mag: Vec3 | null;
```

Defined in: [sensors/bno055/bno055.ts:126](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L126)

µT

***

### gyro

```ts
gyro: Vec3 | null;
```

Defined in: [sensors/bno055/bno055.ts:128](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L128)

dps or rps

***

### euler

```ts
euler: Vec3 | null;
```

Defined in: [sensors/bno055/bno055.ts:130](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L130)

heading, roll, pitch — degrees or radians

***

### quaternion

```ts
quaternion: [number, number, number, number] | null;
```

Defined in: [sensors/bno055/bno055.ts:132](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L132)

w, x, y, z (unit quaternion)

***

### linearAccel

```ts
linearAccel: Vec3 | null;
```

Defined in: [sensors/bno055/bno055.ts:134](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L134)

Acceleration minus gravity, always m/s².

***

### gravity

```ts
gravity: Vec3 | null;
```

Defined in: [sensors/bno055/bno055.ts:136](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L136)

Always m/s².

***

### temperature

```ts
temperature: number | null;
```

Defined in: [sensors/bno055/bno055.ts:138](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L138)

°C or °F

***

### calibration

```ts
calibration: Bno055CalibStatus | null;
```

Defined in: [sensors/bno055/bno055.ts:139](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L139)

# Interface: Bno055StreamData

Defined in: [protocol/bno055.ts:166](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L166)

RPT_BNO_REG_STREAM — one streamed register block. `addr`/`length` echo the
stream configuration so each report is self-describing.

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [protocol/bno055.ts:168](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L168)

MCU uptime at the trigger (timer expiry or INT edge).

***

### addr

```ts
addr: number;
```

Defined in: [protocol/bno055.ts:169](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L169)

***

### length

```ts
length: number;
```

Defined in: [protocol/bno055.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L170)

***

### data

```ts
data: Uint8Array;
```

Defined in: [protocol/bno055.ts:171](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L171)

# Interface: Bno055SystemStatus

Defined in: [sensors/bno055/regs.ts:435](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L435)

ST_RESULT (0x36) + SYS_CLK_STATUS/SYS_STATUS/SYS_ERR (0x38..0x3A).

## Properties

### selfTest

```ts
selfTest: number;
```

Defined in: [sensors/bno055/regs.ts:436](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L436)

***

### clkStatus

```ts
clkStatus: number;
```

Defined in: [sensors/bno055/regs.ts:437](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L437)

***

### status

```ts
status: number;
```

Defined in: [sensors/bno055/regs.ts:438](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L438)

***

### error

```ts
error: number;
```

Defined in: [sensors/bno055/regs.ts:439](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L439)

***

### statusText

```ts
statusText: string;
```

Defined in: [sensors/bno055/regs.ts:440](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L440)

***

### errorText

```ts
errorText: string;
```

Defined in: [sensors/bno055/regs.ts:441](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L441)

***

### selfTestPassed

```ts
selfTestPassed: boolean;
```

Defined in: [sensors/bno055/regs.ts:442](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L442)

# Interface: Bno055Units

Defined in: [sensors/bno055/regs.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L180)

Output units. Default (all false) = m/s², dps, degrees, °C, Windows
orientation (UNIT_SEL 0x00). The sensor's power-on value is 0x80.

## Properties

### accelMg

```ts
accelMg: boolean;
```

Defined in: [sensors/bno055/regs.ts:182](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L182)

ACC_DATA in mg, else m/s² (linear accel / gravity stay m/s²).

***

### gyroRps

```ts
gyroRps: boolean;
```

Defined in: [sensors/bno055/regs.ts:183](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L183)

***

### eulerRad

```ts
eulerRad: boolean;
```

Defined in: [sensors/bno055/regs.ts:184](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L184)

***

### tempF

```ts
tempF: boolean;
```

Defined in: [sensors/bno055/regs.ts:185](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L185)

***

### android

```ts
android: boolean;
```

Defined in: [sensors/bno055/regs.ts:186](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L186)

# Variable: BNO055\_ACC\_BANDWIDTH\_HZ

```ts
const BNO055_ACC_BANDWIDTH_HZ: readonly number[];
```

Defined in: [sensors/bno055/regs.ts:384](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L384)

# Variable: BNO055\_ACC\_RANGE\_G

```ts
const BNO055_ACC_RANGE_G: readonly number[];
```

Defined in: [sensors/bno055/regs.ts:383](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L383)

# Variable: BNO055\_AXIS\_X

```ts
const BNO055_AXIS_X: 0 = 0;
```

Defined in: [sensors/bno055/regs.ts:312](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L312)

# Variable: BNO055\_AXIS\_Y

```ts
const BNO055_AXIS_Y: 1 = 1;
```

Defined in: [sensors/bno055/regs.ts:313](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L313)

# Variable: BNO055\_AXIS\_Z

```ts
const BNO055_AXIS_Z: 2 = 2;
```

Defined in: [sensors/bno055/regs.ts:314](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L314)

# Variable: BNO055\_BOOT\_SETTLE\_TIMEOUT\_MS

```ts
const BNO055_BOOT_SETTLE_TIMEOUT_MS: 1000 = 1000;
```

Defined in: [sensors/bno055/regs.ts:131](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L131)

# Variable: BNO055\_CALIB\_PROFILE\_LEN

```ts
const BNO055_CALIB_PROFILE_LEN: 22 = 22;
```

Defined in: [sensors/bno055/regs.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L41)

# Variable: BNO055\_DEFAULT\_AXIS\_REMAP

```ts
const BNO055_DEFAULT_AXIS_REMAP: Readonly<Bno055AxisRemap>;
```

Defined in: [sensors/bno055/regs.ts:329](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L329)

# Variable: BNO055\_DEFAULT\_UNITS

```ts
const BNO055_DEFAULT_UNITS: Readonly<Bno055Units>;
```

Defined in: [sensors/bno055/regs.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L189)

# Variable: BNO055\_EXPECTED\_IDS

```ts
const BNO055_EXPECTED_IDS: {
  chipId: 160;
  accId: 251;
  magId: 50;
  gyrId: 15;
};
```

Defined in: [protocol/bno055.ts:81](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L81)

Identity registers 0x00..0x03 of a healthy BNO055.

## Type Declaration

### chipId

```ts
readonly chipId: 160 = 0xa0;
```

### accId

```ts
readonly accId: 251 = 0xfb;
```

### magId

```ts
readonly magId: 50 = 0x32;
```

### gyrId

```ts
readonly gyrId: 15 = 0x0f;
```

# Variable: BNO055\_EXPECTED\_SELF\_TEST

```ts
const BNO055_EXPECTED_SELF_TEST: 15 = 0x0f;
```

Defined in: [sensors/bno055/regs.ts:84](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L84)

# Variable: BNO055\_FULL\_BLOCK

```ts
const BNO055_FULL_BLOCK: readonly [number, number];
```

Defined in: [sensors/bno055/regs.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L57)

[addr, len] of the block carrying every output channel (0x08..0x35).

# Variable: BNO055\_FUSION\_ACCEL\_LSB

```ts
const BNO055_FUSION_ACCEL_LSB: 100 = 100;
```

Defined in: [sensors/bno055/regs.ts:230](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L230)

LIA / GRV ignore the ACC_Unit bit: always m/s² at 100 LSB (measured).

# Variable: BNO055\_FUSION\_START\_TIMEOUT\_MS

```ts
const BNO055_FUSION_START_TIMEOUT_MS: 1000 = 1000;
```

Defined in: [sensors/bno055/regs.ts:140](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L140)

Fusion outputs read zero for ~70 ms after CONFIG → a fusion mode.

# Variable: BNO055\_GYR\_BANDWIDTH\_HZ

```ts
const BNO055_GYR_BANDWIDTH_HZ: readonly number[];
```

Defined in: [sensors/bno055/regs.ts:386](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L386)

# Variable: BNO055\_GYR\_RANGE\_DPS

```ts
const BNO055_GYR_RANGE_DPS: readonly number[];
```

Defined in: [sensors/bno055/regs.ts:385](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L385)

# Variable: BNO055\_I2C\_ERROR\_NAMES

```ts
const BNO055_I2C_ERROR_NAMES: Readonly<Record<number, string>>;
```

Defined in: [protocol/bno055.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L73)

last_i2c_error values in RPT_BNO_INFO.

# Variable: BNO055\_INFO\_SIZE

```ts
const BNO055_INFO_SIZE: 38 = 38;
```

Defined in: [protocol/bno055.ts:120](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L120)

# Variable: BNO055\_INT

```ts
const BNO055_INT: {
  accBsxDrdy: 1;
  magDrdy: 2;
  gyrAm: 4;
  gyrHighRate: 8;
  gyrDrdy: 16;
  accHighG: 32;
  accAm: 64;
  accNm: 128;
};
```

Defined in: [sensors/bno055/regs.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L71)

INT_EN / INT_MSK / INT_STA bits. The DRDY bits exist only on sensor
firmware 03.14+; the boards in this line carry 03.11.

## Type Declaration

### accBsxDrdy

```ts
readonly accBsxDrdy: 1 = 0x01;
```

### magDrdy

```ts
readonly magDrdy: 2 = 0x02;
```

### gyrAm

```ts
readonly gyrAm: 4 = 0x04;
```

### gyrHighRate

```ts
readonly gyrHighRate: 8 = 0x08;
```

### gyrDrdy

```ts
readonly gyrDrdy: 16 = 0x10;
```

### accHighG

```ts
readonly accHighG: 32 = 0x20;
```

### accAm

```ts
readonly accAm: 64 = 0x40;
```

### accNm

```ts
readonly accNm: 128 = 0x80;
```

# Variable: BNO055\_MAG\_LSB

```ts
const BNO055_MAG_LSB: 16 = 16;
```

Defined in: [sensors/bno055/regs.ts:227](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L227)

# Variable: BNO055\_MAG\_RATE\_HZ

```ts
const BNO055_MAG_RATE_HZ: readonly number[];
```

Defined in: [sensors/bno055/regs.ts:387](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L387)

# Variable: BNO055\_MODE\_SWITCH\_FROM\_CONFIG\_MS

```ts
const BNO055_MODE_SWITCH_FROM_CONFIG_MS: 10 = 10;
```

Defined in: [sensors/bno055/regs.ts:121](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L121)

Datasheet Table 3-6 plus margin: CONFIG → any 7 ms, any → CONFIG 19 ms.

# Variable: BNO055\_MODE\_SWITCH\_TO\_CONFIG\_MS

```ts
const BNO055_MODE_SWITCH_TO_CONFIG_MS: 25 = 25;
```

Defined in: [sensors/bno055/regs.ts:122](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L122)

# Variable: BNO055\_PLACEMENTS

```ts
const BNO055_PLACEMENTS: Readonly<Record<string, readonly [number, number]>>;
```

Defined in: [sensors/bno055/regs.ts:364](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L364)

Datasheet §3.4: placement → [AXIS_MAP_CONFIG, AXIS_MAP_SIGN]; P1 is the default.

# Variable: BNO055\_QUAT\_BLOCK

```ts
const BNO055_QUAT_BLOCK: readonly [number, number];
```

Defined in: [sensors/bno055/regs.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L59)

[addr, len] of the quaternion alone — the cheapest orientation read.

# Variable: BNO055\_QUAT\_LSB

```ts
const BNO055_QUAT_LSB: 16384 = 16384;
```

Defined in: [sensors/bno055/regs.ts:228](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L228)

# Variable: BNO055\_REG1\_ACC\_CONFIG

```ts
const BNO055_REG1_ACC_CONFIG: 8 = 0x08;
```

Defined in: [sensors/bno055/regs.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L44)

# Variable: BNO055\_REG1\_GYR\_CONFIG\_0

```ts
const BNO055_REG1_GYR_CONFIG_0: 10 = 0x0a;
```

Defined in: [sensors/bno055/regs.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L46)

# Variable: BNO055\_REG1\_GYR\_CONFIG\_1

```ts
const BNO055_REG1_GYR_CONFIG_1: 11 = 0x0b;
```

Defined in: [sensors/bno055/regs.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L47)

# Variable: BNO055\_REG1\_INT\_EN

```ts
const BNO055_REG1_INT_EN: 16 = 0x10;
```

Defined in: [sensors/bno055/regs.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L49)

# Variable: BNO055\_REG1\_INT\_MSK

```ts
const BNO055_REG1_INT_MSK: 15 = 0x0f;
```

Defined in: [sensors/bno055/regs.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L48)

# Variable: BNO055\_REG1\_INT\_SETTINGS\_FIRST

```ts
const BNO055_REG1_INT_SETTINGS_FIRST: 17 = 0x11;
```

Defined in: [sensors/bno055/regs.ts:51](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L51)

First / last page-1 motion-interrupt setting register (written raw).

# Variable: BNO055\_REG1\_INT\_SETTINGS\_LAST

```ts
const BNO055_REG1_INT_SETTINGS_LAST: 31 = 0x1f;
```

Defined in: [sensors/bno055/regs.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L52)

# Variable: BNO055\_REG1\_MAG\_CONFIG

```ts
const BNO055_REG1_MAG_CONFIG: 9 = 0x09;
```

Defined in: [sensors/bno055/regs.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L45)

# Variable: BNO055\_REG1\_UNIQUE\_ID

```ts
const BNO055_REG1_UNIQUE_ID: 80 = 0x50;
```

Defined in: [sensors/bno055/regs.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L53)

# Variable: BNO055\_REG\_ACC\_DATA

```ts
const BNO055_REG_ACC_DATA: 8 = 0x08;
```

Defined in: [sensors/bno055/regs.ts:13](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L13)

# Variable: BNO055\_REG\_AXIS\_MAP\_CONFIG

```ts
const BNO055_REG_AXIS_MAP_CONFIG: 65 = 0x41;
```

Defined in: [sensors/bno055/regs.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L35)

# Variable: BNO055\_REG\_AXIS\_MAP\_SIGN

```ts
const BNO055_REG_AXIS_MAP_SIGN: 66 = 0x42;
```

Defined in: [sensors/bno055/regs.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L36)

# Variable: BNO055\_REG\_CALIB\_PROFILE

```ts
const BNO055_REG_CALIB_PROFILE: 85 = 0x55;
```

Defined in: [sensors/bno055/regs.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L40)

acc/mag/gyr offsets + acc/mag radius.

# Variable: BNO055\_REG\_CALIB\_STAT

```ts
const BNO055_REG_CALIB_STAT: 53 = 0x35;
```

Defined in: [sensors/bno055/regs.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L23)

# Variable: BNO055\_REG\_CHIP\_ID

```ts
const BNO055_REG_CHIP_ID: 0 = 0x00;
```

Defined in: [sensors/bno055/regs.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L11)

BNO055 register map and the pure codecs every SDK shares
(contracts/13_SENSOR_BNO055.md §4, Bosch BST-BNO055-DS000 rev 1.8).
Mirrors the Python reference `depz_sensor_sdk.bno055.regs`.

Nothing here touches the wire: these functions turn register bytes into
values and back, so they are what `vectors/bno055.json` pins.

# Variable: BNO055\_REG\_EUL\_DATA

```ts
const BNO055_REG_EUL_DATA: 26 = 0x1a;
```

Defined in: [sensors/bno055/regs.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L17)

heading, roll, pitch

# Variable: BNO055\_REG\_GRV\_DATA

```ts
const BNO055_REG_GRV_DATA: 46 = 0x2e;
```

Defined in: [sensors/bno055/regs.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L21)

# Variable: BNO055\_REG\_GYR\_DATA

```ts
const BNO055_REG_GYR_DATA: 20 = 0x14;
```

Defined in: [sensors/bno055/regs.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L15)

# Variable: BNO055\_REG\_INT\_STA

```ts
const BNO055_REG_INT_STA: 55 = 0x37;
```

Defined in: [sensors/bno055/regs.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L26)

Clear-on-read — never part of a routine block read.

# Variable: BNO055\_REG\_LIA\_DATA

```ts
const BNO055_REG_LIA_DATA: 40 = 0x28;
```

Defined in: [sensors/bno055/regs.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L20)

# Variable: BNO055\_REG\_MAG\_DATA

```ts
const BNO055_REG_MAG_DATA: 14 = 0x0e;
```

Defined in: [sensors/bno055/regs.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L14)

# Variable: BNO055\_REG\_OPR\_MODE

```ts
const BNO055_REG_OPR_MODE: 61 = 0x3d;
```

Defined in: [sensors/bno055/regs.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L31)

# Variable: BNO055\_REG\_PAGE\_ID

```ts
const BNO055_REG_PAGE_ID: 7 = 0x07;
```

Defined in: [sensors/bno055/regs.ts:12](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L12)

# Variable: BNO055\_REG\_PWR\_MODE

```ts
const BNO055_REG_PWR_MODE: 62 = 0x3e;
```

Defined in: [sensors/bno055/regs.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L32)

# Variable: BNO055\_REG\_QUA\_DATA

```ts
const BNO055_REG_QUA_DATA: 32 = 0x20;
```

Defined in: [sensors/bno055/regs.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L19)

w, x, y, z

# Variable: BNO055\_REG\_SIC\_MATRIX

```ts
const BNO055_REG_SIC_MATRIX: 67 = 0x43;
```

Defined in: [sensors/bno055/regs.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L38)

9 × i16, row-major.

# Variable: BNO055\_REG\_ST\_RESULT

```ts
const BNO055_REG_ST_RESULT: 54 = 0x36;
```

Defined in: [sensors/bno055/regs.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L24)

# Variable: BNO055\_REG\_SYS\_CLK\_STATUS

```ts
const BNO055_REG_SYS_CLK_STATUS: 56 = 0x38;
```

Defined in: [sensors/bno055/regs.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L27)

# Variable: BNO055\_REG\_SYS\_ERR

```ts
const BNO055_REG_SYS_ERR: 58 = 0x3a;
```

Defined in: [sensors/bno055/regs.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L29)

# Variable: BNO055\_REG\_SYS\_STATUS

```ts
const BNO055_REG_SYS_STATUS: 57 = 0x39;
```

Defined in: [sensors/bno055/regs.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L28)

# Variable: BNO055\_REG\_SYS\_TRIGGER

```ts
const BNO055_REG_SYS_TRIGGER: 63 = 0x3f;
```

Defined in: [sensors/bno055/regs.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L33)

# Variable: BNO055\_REG\_TEMP

```ts
const BNO055_REG_TEMP: 52 = 0x34;
```

Defined in: [sensors/bno055/regs.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L22)

# Variable: BNO055\_REG\_TEMP\_SOURCE

```ts
const BNO055_REG_TEMP_SOURCE: 64 = 0x40;
```

Defined in: [sensors/bno055/regs.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L34)

# Variable: BNO055\_REG\_UNIT\_SEL

```ts
const BNO055_REG_UNIT_SEL: 59 = 0x3b;
```

Defined in: [sensors/bno055/regs.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L30)

# Variable: BNO055\_RESET\_TIMEOUT\_MS

```ts
const BNO055_RESET_TIMEOUT_MS: 3000 = 3000;
```

Defined in: [protocol/bno055.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L30)

BNO_RESET answers after the sensor's ~0.5 s boot handshake.

# Variable: BNO055\_SELF\_TEST\_MS

```ts
const BNO055_SELF_TEST_MS: 450 = 450;
```

Defined in: [sensors/bno055/regs.ts:124](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L124)

BIST runs ~400 ms (datasheet §3.9.2).

# Variable: BNO055\_SIC\_IDENTITY

```ts
const BNO055_SIC_IDENTITY: readonly number[];
```

Defined in: [sensors/bno055/regs.ts:295](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L295)

Soft-iron matrix, 9 × i16 row-major, 1.0 = 16384.

# Variable: BNO055\_ST

```ts
const BNO055_ST: {
  acc: 1;
  mag: 2;
  gyr: 4;
  mcu: 8;
};
```

Defined in: [sensors/bno055/regs.ts:83](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L83)

ST_RESULT bits (1 = passed).

## Type Declaration

### acc

```ts
readonly acc: 1 = 0x01;
```

### mag

```ts
readonly mag: 2 = 0x02;
```

### gyr

```ts
readonly gyr: 4 = 0x04;
```

### mcu

```ts
readonly mcu: 8 = 0x08;
```

# Variable: BNO055\_STUCK\_SELF\_TEST\_POLLS

```ts
const BNO055_STUCK_SELF_TEST_POLLS: 20 = 20;
```

Defined in: [sensors/bno055/regs.ts:138](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L138)

SYS_STATUS 4 polls in a row taken as the leftover of a self-test run in
CONFIG mode, not POST: POST shows 4 for ~35 ms (a handful of polls), the
leftover stays until the mode leaves CONFIG (measured on SW 03.11). Counted,
not timed, so a replay makes the same decision.

# Variable: BNO055\_SYS\_ERR\_NAMES

```ts
const BNO055_SYS_ERR_NAMES: Readonly<Record<number, string>>;
```

Defined in: [sensors/bno055/regs.ts:152](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L152)

# Variable: BNO055\_SYS\_STATUS\_BOOTING

```ts
const BNO055_SYS_STATUS_BOOTING: readonly number[];
```

Defined in: [sensors/bno055/regs.ts:130](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L130)

SYS_STATUS values while the sensor is still booting after BNO_RESET —
the bridge answers at the chip-ID handshake, ~15 ms before the sensor's
boot ends and it reverts OPR_MODE (contract 13 §5, ERRATA E13).

# Variable: BNO055\_SYS\_STATUS\_NAMES

```ts
const BNO055_SYS_STATUS_NAMES: Readonly<Record<number, string>>;
```

Defined in: [sensors/bno055/regs.ts:142](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L142)

# Variable: BNO055\_SYS\_TRIGGER\_CLK\_SEL

```ts
const BNO055_SYS_TRIGGER_CLK_SEL: 128 = 0x80;
```

Defined in: [sensors/bno055/regs.ts:65](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L65)

# Variable: BNO055\_SYS\_TRIGGER\_RST\_INT

```ts
const BNO055_SYS_TRIGGER_RST_INT: 64 = 0x40;
```

Defined in: [sensors/bno055/regs.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L64)

# Variable: BNO055\_SYS\_TRIGGER\_RST\_SYS

```ts
const BNO055_SYS_TRIGGER_RST_SYS: 32 = 0x20;
```

Defined in: [sensors/bno055/regs.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L63)

# Variable: BNO055\_SYS\_TRIGGER\_SELF\_TEST

```ts
const BNO055_SYS_TRIGGER_SELF_TEST: 1 = 0x01;
```

Defined in: [sensors/bno055/regs.ts:62](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L62)

# Variable: BNO055\_TRIGGER\_INT

```ts
const BNO055_TRIGGER_INT: 1 = 1;
```

Defined in: [protocol/bno055.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L27)

BNO_START_STREAM trigger: read on the INT edge; period_ms is a missed-edge watchdog.

# Variable: BNO055\_TRIGGER\_TIMER

```ts
const BNO055_TRIGGER_TIMER: 0 = 0;
```

Defined in: [protocol/bno055.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L25)

BNO_START_STREAM trigger: read every period_ms (the only data trigger on SW 03.11).

# Variable: BNO055\_UNIQUE\_ID\_LEN

```ts
const BNO055_UNIQUE_ID_LEN: 16 = 16;
```

Defined in: [sensors/bno055/regs.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L54)

# Variable: BNO055\_XFER\_MAX

```ts
const BNO055_XFER_MAX: 128 = 128;
```

Defined in: [protocol/bno055.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L22)

Max bytes per READ_REG / WRITE_REG / streamed block; `addr + len` ≤ 0x100.

