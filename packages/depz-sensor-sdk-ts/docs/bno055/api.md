# BNO055 (IMU) — API reference

The public API for the BNO055 IMU: the `Bno055` device and its sample
type, the register map and codecs (units, axis remap, calibration
profile, …) and the register-bridge wire codecs. Discovery, device-base,
transport and other cross-sensor symbols live in the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Enumerations

- [Bno055Cmd](#enumeration-bno055cmd)
- [Bno055Rpt](#enumeration-bno055rpt)
- [Bno055OprMode](#enumeration-bno055oprmode)
- [Bno055PwrMode](#enumeration-bno055pwrmode)
- [Bno055TempSource](#enumeration-bno055tempsource)

## Classes

- [Bno055](#class-bno055)

## Interfaces

- [Bno055RegData](#interface-bno055regdata)
- [Bno055Info](#interface-bno055info)
- [Bno055StreamData](#interface-bno055streamdata)
- [Bno055Sample](#interface-bno055sample)
- [Bno055Options](#interface-bno055options)
- [Bno055ConfigureOptions](#interface-bno055configureoptions)
- [Bno055Units](#interface-bno055units)
- [Bno055CalibStatus](#interface-bno055calibstatus)
- [Bno055CalibrationProfile](#interface-bno055calibrationprofile)
- [Bno055AxisRemap](#interface-bno055axisremap)
- [Bno055AccelConfig](#interface-bno055accelconfig)
- [Bno055GyroConfig](#interface-bno055gyroconfig)
- [Bno055MagConfig](#interface-bno055magconfig)
- [Bno055SystemStatus](#interface-bno055systemstatus)
- [Bno055RawBlock](#interface-bno055rawblock)

## Variables

- [BNO055\_XFER\_MAX](#variable-bno055_xfer_max)
- [BNO055\_TRIGGER\_TIMER](#variable-bno055_trigger_timer)
- [BNO055\_TRIGGER\_INT](#variable-bno055_trigger_int)
- [BNO055\_RESET\_TIMEOUT\_MS](#variable-bno055_reset_timeout_ms)
- [BNO055\_TIMER\_PERIOD\_MIN\_MS](#variable-bno055_timer_period_min_ms)
- [BNO055\_TIMER\_PERIOD\_MAX\_MS](#variable-bno055_timer_period_max_ms)
- [BNO055\_WRITE\_MAX](#variable-bno055_write_max)
- [BNO055\_I2C\_ERROR\_NAMES](#variable-bno055_i2c_error_names)
- [BNO055\_EXPECTED\_IDS](#variable-bno055_expected_ids)
- [BNO055\_INFO\_SIZE](#variable-bno055_info_size)
- [BNO055\_REG\_CHIP\_ID](#variable-bno055_reg_chip_id)
- [BNO055\_REG\_PAGE\_ID](#variable-bno055_reg_page_id)
- [BNO055\_REG\_ACC\_DATA](#variable-bno055_reg_acc_data)
- [BNO055\_REG\_MAG\_DATA](#variable-bno055_reg_mag_data)
- [BNO055\_REG\_GYR\_DATA](#variable-bno055_reg_gyr_data)
- [BNO055\_REG\_EUL\_DATA](#variable-bno055_reg_eul_data)
- [BNO055\_REG\_QUA\_DATA](#variable-bno055_reg_qua_data)
- [BNO055\_REG\_LIA\_DATA](#variable-bno055_reg_lia_data)
- [BNO055\_REG\_GRV\_DATA](#variable-bno055_reg_grv_data)
- [BNO055\_REG\_TEMP](#variable-bno055_reg_temp)
- [BNO055\_REG\_CALIB\_STAT](#variable-bno055_reg_calib_stat)
- [BNO055\_REG\_ST\_RESULT](#variable-bno055_reg_st_result)
- [BNO055\_REG\_INT\_STA](#variable-bno055_reg_int_sta)
- [BNO055\_REG\_SYS\_CLK\_STATUS](#variable-bno055_reg_sys_clk_status)
- [BNO055\_REG\_SYS\_STATUS](#variable-bno055_reg_sys_status)
- [BNO055\_REG\_SYS\_ERR](#variable-bno055_reg_sys_err)
- [BNO055\_REG\_UNIT\_SEL](#variable-bno055_reg_unit_sel)
- [BNO055\_REG\_OPR\_MODE](#variable-bno055_reg_opr_mode)
- [BNO055\_REG\_PWR\_MODE](#variable-bno055_reg_pwr_mode)
- [BNO055\_REG\_SYS\_TRIGGER](#variable-bno055_reg_sys_trigger)
- [BNO055\_REG\_TEMP\_SOURCE](#variable-bno055_reg_temp_source)
- [BNO055\_REG\_AXIS\_MAP\_CONFIG](#variable-bno055_reg_axis_map_config)
- [BNO055\_REG\_AXIS\_MAP\_SIGN](#variable-bno055_reg_axis_map_sign)
- [BNO055\_REG\_SIC\_MATRIX](#variable-bno055_reg_sic_matrix)
- [BNO055\_REG\_CALIB\_PROFILE](#variable-bno055_reg_calib_profile)
- [BNO055\_CALIB\_PROFILE\_LEN](#variable-bno055_calib_profile_len)
- [BNO055\_REG1\_ACC\_CONFIG](#variable-bno055_reg1_acc_config)
- [BNO055\_REG1\_MAG\_CONFIG](#variable-bno055_reg1_mag_config)
- [BNO055\_REG1\_GYR\_CONFIG\_0](#variable-bno055_reg1_gyr_config_0)
- [BNO055\_REG1\_GYR\_CONFIG\_1](#variable-bno055_reg1_gyr_config_1)
- [BNO055\_REG1\_INT\_MSK](#variable-bno055_reg1_int_msk)
- [BNO055\_REG1\_INT\_EN](#variable-bno055_reg1_int_en)
- [BNO055\_REG1\_INT\_SETTINGS\_FIRST](#variable-bno055_reg1_int_settings_first)
- [BNO055\_REG1\_INT\_SETTINGS\_LAST](#variable-bno055_reg1_int_settings_last)
- [BNO055\_REG1\_UNIQUE\_ID](#variable-bno055_reg1_unique_id)
- [BNO055\_UNIQUE\_ID\_LEN](#variable-bno055_unique_id_len)
- [BNO055\_FULL\_BLOCK](#variable-bno055_full_block)
- [BNO055\_QUAT\_BLOCK](#variable-bno055_quat_block)
- [BNO055\_INT](#variable-bno055_int)
- [BNO055\_ST](#variable-bno055_st)
- [BNO055\_SYS\_STATUS\_NAMES](#variable-bno055_sys_status_names)
- [BNO055\_SYS\_ERR\_NAMES](#variable-bno055_sys_err_names)
- [BNO055\_DEFAULT\_UNITS](#variable-bno055_default_units)
- [BNO055\_SIC\_IDENTITY](#variable-bno055_sic_identity)
- [BNO055\_AXIS\_X](#variable-bno055_axis_x)
- [BNO055\_AXIS\_Y](#variable-bno055_axis_y)
- [BNO055\_AXIS\_Z](#variable-bno055_axis_z)
- [BNO055\_DEFAULT\_AXIS\_REMAP](#variable-bno055_default_axis_remap)
- [BNO055\_PLACEMENTS](#variable-bno055_placements)
- [BNO055\_ACC\_RANGE\_G](#variable-bno055_acc_range_g)
- [BNO055\_ACC\_BANDWIDTH\_HZ](#variable-bno055_acc_bandwidth_hz)
- [BNO055\_GYR\_RANGE\_DPS](#variable-bno055_gyr_range_dps)
- [BNO055\_GYR\_BANDWIDTH\_HZ](#variable-bno055_gyr_bandwidth_hz)
- [BNO055\_MAG\_RATE\_HZ](#variable-bno055_mag_rate_hz)

## Functions

- [packBno055ReadReg](#function-packbno055readreg)
- [packBno055WriteReg](#function-packbno055writereg)
- [packBno055StartStream](#function-packbno055startstream)
- [unpackBno055RegData](#function-unpackbno055regdata)
- [unpackBno055Info](#function-unpackbno055info)
- [bno055IdsOk](#function-bno055idsok)
- [bno055SwRevText](#function-bno055swrevtext)
- [unpackBno055Stream](#function-unpackbno055stream)
- [bno055IsFusion](#function-bno055isfusion)
- [packBno055Units](#function-packbno055units)
- [unpackBno055Units](#function-unpackbno055units)
- [unpackBno055CalibStatus](#function-unpackbno055calibstatus)
- [packBno055CalibStatus](#function-packbno055calibstatus)
- [bno055FullyCalibrated](#function-bno055fullycalibrated)
- [packBno055CalibrationProfile](#function-packbno055calibrationprofile)
- [unpackBno055CalibrationProfile](#function-unpackbno055calibrationprofile)
- [packBno055SicMatrix](#function-packbno055sicmatrix)
- [unpackBno055SicMatrix](#function-unpackbno055sicmatrix)
- [packBno055AxisRemap](#function-packbno055axisremap)
- [unpackBno055AxisRemap](#function-unpackbno055axisremap)
- [bno055Placement](#function-bno055placement)
- [packBno055AccelConfig](#function-packbno055accelconfig)
- [unpackBno055AccelConfig](#function-unpackbno055accelconfig)
- [packBno055GyroConfig](#function-packbno055gyroconfig)
- [unpackBno055GyroConfig](#function-unpackbno055gyroconfig)
- [packBno055MagConfig](#function-packbno055magconfig)
- [unpackBno055MagConfig](#function-unpackbno055magconfig)
- [makeBno055SystemStatus](#function-makebno055systemstatus)
- [decodeBno055Block](#function-decodebno055block)

# Class: Bno055

Defined in: [sensors/bno055/bno055.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L198)

BNO055 absolute-orientation IMU.

Typical use: `configure()` (CONFIG → units → axis remap → optional
calibration profile → NDOF), then `startStream(10)` and read the `samples`
stream, or poll `readSample()`. Streaming is timer-driven: the
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

Defined in: [sensors/bno055/bno055.ts:217](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L217)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | [`Bno055Options`](#interface-bno055options) |

#### Returns

`Bno055`

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

### samples

```ts
readonly samples: Stream<Bno055Sample>;
```

Defined in: [sensors/bno055/bno055.ts:205](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L205)

The streamed samples (contract 07 §3): `on(cb)` subscribes,
`subscribe(maxsize)` iterates, `next(timeoutMs)` waits for one;
`droppedCounts` and `parseErrors` (stream reports that did not decode — a
short block) are its counters.

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

### streaming

#### Get Signature

```ts
get streaming(): boolean;
```

Defined in: [sensors/bno055/bno055.ts:616](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L616)

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

### bridgeInfo()

```ts
bridgeInfo(): Promise<Bno055Info>;
```

Defined in: [sensors/bno055/bno055.ts:225](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L225)

RPT_BNO_INFO: chip ids, sensor firmware revision and bridge counters.

#### Returns

`Promise`\<[`Bno055Info`](#interface-bno055info)\>

***

### isAlive()

```ts
isAlive(): Promise<boolean>;
```

Defined in: [sensors/bno055/bno055.ts:232](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L232)

True when the bridge passed the chip-ID handshake and the ids match.

#### Returns

`Promise`\<`boolean`\>

***

### resetSensor()

```ts
resetSensor(): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:247](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L247)

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

Defined in: [sensors/bno055/bno055.ts:262](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L262)

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

Defined in: [sensors/bno055/bno055.ts:267](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L267)

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

Defined in: [sensors/bno055/bno055.ts:271](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L271)

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

Defined in: [sensors/bno055/bno055.ts:276](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L276)

Write one register; a `value` outside 0..255 is a `RangeError`.

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

Defined in: [sensors/bno055/bno055.ts:282](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L282)

#### Returns

`Promise`\<[`Bno055OprMode`](#enumeration-bno055oprmode)\>

***

### setOperationMode()

```ts
setOperationMode(mode): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:292](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L292)

Switch OPR_MODE and wait out the switching time; into a fusion mode also
wait (≤ 1 s) for the fusion outputs, zero for ~70 ms after CONFIG. The
sensor ignores a direct write from one operating mode to another
(measured: NDOF → AMG stays NDOF), so such a switch goes through CONFIG.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `mode` | [`Bno055OprMode`](#enumeration-bno055oprmode) |

#### Returns

`Promise`\<`void`\>

***

### getPowerMode()

```ts
getPowerMode(): Promise<Bno055PwrMode>;
```

Defined in: [sensors/bno055/bno055.ts:303](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L303)

#### Returns

`Promise`\<[`Bno055PwrMode`](#enumeration-bno055pwrmode)\>

***

### setPowerMode()

```ts
setPowerMode(mode): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:314](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L314)

Written in CONFIG mode; the operating mode is restored after. A suspended
chip ignores OPR_MODE writes and stays in CONFIG (measured 19.10.2026), so
the operating mode is kept here and put back on wake-up — NDOF if the chip
was already asleep when we connected (contract 13 §4.6). A value
`Bno055PwrMode` does not list is a RangeError before any traffic (contract 07 §3).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `mode` | [`Bno055PwrMode`](#enumeration-bno055pwrmode) |

#### Returns

`Promise`\<`void`\>

***

### getUnits()

```ts
getUnits(): Promise<Bno055Units>;
```

Defined in: [sensors/bno055/bno055.ts:340](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L340)

#### Returns

`Promise`\<[`Bno055Units`](#interface-bno055units)\>

***

### setUnits()

```ts
setUnits(units): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:347](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L347)

A flag left out is false.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `units` | `Partial`\<[`Bno055Units`](#interface-bno055units)\> |

#### Returns

`Promise`\<`void`\>

***

### getAxisRemap()

```ts
getAxisRemap(): Promise<Bno055AxisRemap>;
```

Defined in: [sensors/bno055/bno055.ts:357](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L357)

#### Returns

`Promise`\<[`Bno055AxisRemap`](#interface-bno055axisremap)\>

***

### setAxisRemap()

```ts
setAxisRemap(remap): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:366](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L366)

An AxisRemap (a field left out is its P1 value), or a datasheet placement
"P0".."P7" (P1 = default).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `remap` | \| `string` \| `Partial`\<[`Bno055AxisRemap`](#interface-bno055axisremap)\> |

#### Returns

`Promise`\<`void`\>

***

### getTemperatureSource()

```ts
getTemperatureSource(): Promise<Bno055TempSource>;
```

Defined in: [sensors/bno055/bno055.ts:372](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L372)

#### Returns

`Promise`\<[`Bno055TempSource`](#enumeration-bno055tempsource)\>

***

### setTemperatureSource()

```ts
setTemperatureSource(source): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:377](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L377)

A value `Bno055TempSource` does not list is a RangeError before any traffic (contract 07 §3).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `source` | [`Bno055TempSource`](#enumeration-bno055tempsource) |

#### Returns

`Promise`\<`void`\>

***

### configure()

```ts
configure(opts?): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:391](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L391)

The usual session setup: CONFIG → units → axis remap → calibration
profile → mode (default NDOF). In a fusion mode it resolves once the
fusion outputs are live. Remembered for restoreConfiguration().

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `opts` | [`Bno055ConfigureOptions`](#interface-bno055configureoptions) |

#### Returns

`Promise`\<`void`\>

***

### restoreConfiguration()

```ts
restoreConfiguration(): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:417](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L417)

Re-apply the last configure() — after resetSensor(), or when
bridgeInfo().sensorResets rose (the bridge's bus recovery pulses nRESET).

#### Returns

`Promise`\<`void`\>

***

### systemStatus()

```ts
systemStatus(): Promise<Bno055SystemStatus>;
```

Defined in: [sensors/bno055/bno055.ts:427](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L427)

ST_RESULT, SYS_CLK_STATUS, SYS_STATUS, SYS_ERR (INT_STA skipped).

#### Returns

`Promise`\<[`Bno055SystemStatus`](#interface-bno055systemstatus)\>

***

### selfTest()

```ts
selfTest(): Promise<Bno055SystemStatus>;
```

Defined in: [sensors/bno055/bno055.ts:435](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L435)

Built-in self-test (datasheet §3.9.2): CONFIG, SYS_TRIGGER SELF_TEST,
~400 ms, then read the verdict. The mode is restored. Not while streaming.

#### Returns

`Promise`\<[`Bno055SystemStatus`](#interface-bno055systemstatus)\>

***

### calibrationStatus()

```ts
calibrationStatus(): Promise<Bno055CalibStatus>;
```

Defined in: [sensors/bno055/bno055.ts:454](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L454)

#### Returns

`Promise`\<[`Bno055CalibStatus`](#interface-bno055calibstatus)\>

***

### readCalibrationProfile()

```ts
readCalibrationProfile(): Promise<Bno055CalibrationProfile>;
```

Defined in: [sensors/bno055/bno055.ts:459](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L459)

Offsets and radii (0x55..0x6A) — CONFIG only; the driver switches there and back.

#### Returns

`Promise`\<[`Bno055CalibrationProfile`](#interface-bno055calibrationprofile)\>

***

### writeCalibrationProfile()

```ts
writeCalibrationProfile(profile): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:473](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L473)

Restore a stored profile: CONFIG, all 22 bytes, back to the previous
mode. A starting point — fusion refines it as soon as it resumes.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `profile` | [`Bno055CalibrationProfile`](#interface-bno055calibrationprofile) |

#### Returns

`Promise`\<`void`\>

***

### getSicMatrix()

```ts
getSicMatrix(): Promise<number[]>;
```

Defined in: [sensors/bno055/bno055.ts:479](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L479)

#### Returns

`Promise`\<`number`[]\>

***

### setSicMatrix()

```ts
setSicMatrix(matrix?): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:483](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L483)

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

Defined in: [sensors/bno055/bno055.ts:489](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L489)

#### Returns

`Promise`\<[`Bno055AccelConfig`](#interface-bno055accelconfig)\>

***

### setAccelConfig()

```ts
setAccelConfig(c): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:494](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L494)

A field left out is its power-on value, not the current one.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `c` | `Partial`\<[`Bno055AccelConfig`](#interface-bno055accelconfig)\> |

#### Returns

`Promise`\<`void`\>

***

### getGyroConfig()

```ts
getGyroConfig(): Promise<Bno055GyroConfig>;
```

Defined in: [sensors/bno055/bno055.ts:498](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L498)

#### Returns

`Promise`\<[`Bno055GyroConfig`](#interface-bno055gyroconfig)\>

***

### setGyroConfig()

```ts
setGyroConfig(c): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:503](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L503)

A field left out is its power-on value, not the current one.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `c` | `Partial`\<[`Bno055GyroConfig`](#interface-bno055gyroconfig)\> |

#### Returns

`Promise`\<`void`\>

***

### getMagConfig()

```ts
getMagConfig(): Promise<Bno055MagConfig>;
```

Defined in: [sensors/bno055/bno055.ts:507](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L507)

#### Returns

`Promise`\<[`Bno055MagConfig`](#interface-bno055magconfig)\>

***

### setMagConfig()

```ts
setMagConfig(c): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:512](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L512)

A field left out is its power-on value, not the current one.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `c` | `Partial`\<[`Bno055MagConfig`](#interface-bno055magconfig)\> |

#### Returns

`Promise`\<`void`\>

***

### uniqueId()

```ts
uniqueId(): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [sensors/bno055/bno055.ts:517](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L517)

The chip's 16-byte unique id (page 1, 0x50..0x5F).

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### getInterruptEnable()

```ts
getInterruptEnable(): Promise<number>;
```

Defined in: [sensors/bno055/bno055.ts:523](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L523)

#### Returns

`Promise`\<`number`\>

***

### setInterruptEnable()

```ts
setInterruptEnable(mask): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:527](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L527)

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

Defined in: [sensors/bno055/bno055.ts:531](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L531)

#### Returns

`Promise`\<`number`\>

***

### setInterruptMask()

```ts
setInterruptMask(mask): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:535](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L535)

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

Defined in: [sensors/bno055/bno055.ts:540](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L540)

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

Defined in: [sensors/bno055/bno055.ts:548](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L548)

INT_STA — which interrupts fired. Clears on read.

#### Returns

`Promise`\<`number`\>

***

### clearInterrupt()

```ts
clearInterrupt(): Promise<void>;
```

Defined in: [sensors/bno055/bno055.ts:553](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L553)

SYS_TRIGGER RST_INT: reset the interrupt status bits and the INT pin.

#### Returns

`Promise`\<`void`\>

***

### readSample()

```ts
readSample(block?): Promise<Bno055Sample>;
```

Defined in: [sensors/bno055/bno055.ts:563](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L563)

Poll one register block (default the full 46-byte block) and decode it.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `block` | readonly \[`number`, `number`\] | `BNO055_FULL_BLOCK` |

#### Returns

`Promise`\<[`Bno055Sample`](#interface-bno055sample)\>

***

### readQuaternion()

```ts
readQuaternion(): Promise<[number, number, number, number]>;
```

Defined in: [sensors/bno055/bno055.ts:572](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L572)

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

Defined in: [sensors/bno055/bno055.ts:581](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L581)

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

Defined in: [sensors/bno055/bno055.ts:607](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L607)

#### Returns

`Promise`\<`void`\>

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [sensors/bno055/bno055.ts:748](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L748)

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

Defined in: [protocol/bno055.ts:9](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L9)

## Enumeration Members

### ReadReg

```ts
ReadReg: 50;
```

Defined in: [protocol/bno055.ts:10](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L10)

***

### WriteReg

```ts
WriteReg: 51;
```

Defined in: [protocol/bno055.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L11)

***

### Reset

```ts
Reset: 52;
```

Defined in: [protocol/bno055.ts:12](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L12)

***

### StartStream

```ts
StartStream: 53;
```

Defined in: [protocol/bno055.ts:13](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L13)

***

### StopStream

```ts
StopStream: 54;
```

Defined in: [protocol/bno055.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L14)

***

### GetInfo

```ts
GetInfo: 55;
```

Defined in: [protocol/bno055.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L15)

# Enumeration: Bno055OprMode

Defined in: [sensors/bno055/regs.ts:90](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L90)

OPR_MODE (0x3D) bits 3:0.

## Enumeration Members

### Config

```ts
Config: 0;
```

Defined in: [sensors/bno055/regs.ts:91](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L91)

***

### AccOnly

```ts
AccOnly: 1;
```

Defined in: [sensors/bno055/regs.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L92)

***

### MagOnly

```ts
MagOnly: 2;
```

Defined in: [sensors/bno055/regs.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L93)

***

### GyroOnly

```ts
GyroOnly: 3;
```

Defined in: [sensors/bno055/regs.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L94)

***

### AccMag

```ts
AccMag: 4;
```

Defined in: [sensors/bno055/regs.ts:95](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L95)

***

### AccGyro

```ts
AccGyro: 5;
```

Defined in: [sensors/bno055/regs.ts:96](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L96)

***

### MagGyro

```ts
MagGyro: 6;
```

Defined in: [sensors/bno055/regs.ts:97](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L97)

***

### Amg

```ts
Amg: 7;
```

Defined in: [sensors/bno055/regs.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L98)

***

### Imu

```ts
Imu: 8;
```

Defined in: [sensors/bno055/regs.ts:99](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L99)

***

### Compass

```ts
Compass: 9;
```

Defined in: [sensors/bno055/regs.ts:100](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L100)

***

### M4g

```ts
M4g: 10;
```

Defined in: [sensors/bno055/regs.ts:101](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L101)

***

### NdofFmcOff

```ts
NdofFmcOff: 11;
```

Defined in: [sensors/bno055/regs.ts:102](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L102)

***

### Ndof

```ts
Ndof: 12;
```

Defined in: [sensors/bno055/regs.ts:103](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L103)

# Enumeration: Bno055PwrMode

Defined in: [sensors/bno055/regs.ts:111](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L111)

PWR_MODE (0x3E) bits 1:0.

## Enumeration Members

### Normal

```ts
Normal: 0;
```

Defined in: [sensors/bno055/regs.ts:112](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L112)

***

### LowPower

```ts
LowPower: 1;
```

Defined in: [sensors/bno055/regs.ts:113](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L113)

***

### Suspend

```ts
Suspend: 2;
```

Defined in: [sensors/bno055/regs.ts:114](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L114)

# Enumeration: Bno055Rpt

Defined in: [protocol/bno055.ts:18](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L18)

## Enumeration Members

### RegData

```ts
RegData: 145;
```

Defined in: [protocol/bno055.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L19)

***

### Info

```ts
Info: 146;
```

Defined in: [protocol/bno055.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L20)

***

### Stream

```ts
Stream: 147;
```

Defined in: [protocol/bno055.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L21)

# Enumeration: Bno055TempSource

Defined in: [sensors/bno055/regs.ts:118](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L118)

TEMP_SOURCE (0x40) bits 1:0.

## Enumeration Members

### Accel

```ts
Accel: 0;
```

Defined in: [sensors/bno055/regs.ts:119](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L119)

***

### Gyro

```ts
Gyro: 1;
```

Defined in: [sensors/bno055/regs.ts:120](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L120)

# Function: bno055FullyCalibrated()

```ts
function bno055FullyCalibrated(s): boolean;
```

Defined in: [sensors/bno055/regs.ts:256](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L256)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `s` | [`Bno055CalibStatus`](#interface-bno055calibstatus) |

## Returns

`boolean`

# Function: bno055IdsOk()

```ts
function bno055IdsOk(info): boolean;
```

Defined in: [protocol/bno055.ts:190](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L190)

True when the four identity registers carry the BNO055 values.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `info` | [`Bno055Info`](#interface-bno055info) |

## Returns

`boolean`

# Function: bno055IsFusion()

```ts
function bno055IsFusion(mode): boolean;
```

Defined in: [sensors/bno055/regs.ts:106](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L106)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `mode` | [`Bno055OprMode`](#enumeration-bno055oprmode) |

## Returns

`boolean`

# Function: bno055Placement()

```ts
function bno055Placement(name): Bno055AxisRemap;
```

Defined in: [sensors/bno055/regs.ts:387](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L387)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `name` | `string` |

## Returns

[`Bno055AxisRemap`](#interface-bno055axisremap)

# Function: bno055SwRevText()

```ts
function bno055SwRevText(swRev): string;
```

Defined in: [protocol/bno055.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L198)

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

Defined in: [sensors/bno055/regs.ts:516](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L516)

Unpack whatever channels the register window starting at `addr` holds.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `data` | `Uint8Array` |

## Returns

[`Bno055RawBlock`](#interface-bno055rawblock)

# Function: makeBno055SystemStatus()

```ts
function makeBno055SystemStatus(
   selfTest, 
   clkStatus, 
   status, 
   error): Bno055SystemStatus;
```

Defined in: [sensors/bno055/regs.ts:468](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L468)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `selfTest` | `number` |
| `clkStatus` | `number` |
| `status` | `number` |
| `error` | `number` |

## Returns

[`Bno055SystemStatus`](#interface-bno055systemstatus)

# Function: packBno055AccelConfig()

```ts
function packBno055AccelConfig(c): number;
```

Defined in: [sensors/bno055/regs.ts:412](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L412)

A field left out is its power-on value (range 1, bandwidth 3, power 0).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `c` | `Partial`\<[`Bno055AccelConfig`](#interface-bno055accelconfig)\> |

## Returns

`number`

# Function: packBno055AxisRemap()

```ts
function packBno055AxisRemap(a): [number, number];
```

Defined in: [sensors/bno055/regs.ts:349](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L349)

→ [AXIS_MAP_CONFIG, AXIS_MAP_SIGN]. A field left out is its P1 value
(`BNO055_DEFAULT_AXIS_REMAP`). The sensor keeps the old mapping when one
axis is used twice, so this refuses a non-permutation up front.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `a` | `Partial`\<[`Bno055AxisRemap`](#interface-bno055axisremap)\> |

## Returns

\[`number`, `number`\]

# Function: packBno055CalibStatus()

```ts
function packBno055CalibStatus(s): number;
```

Defined in: [sensors/bno055/regs.ts:252](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L252)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `s` | [`Bno055CalibStatus`](#interface-bno055calibstatus) |

## Returns

`number`

# Function: packBno055CalibrationProfile()

```ts
function packBno055CalibrationProfile(p): Uint8Array;
```

Defined in: [sensors/bno055/regs.ts:276](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L276)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | [`Bno055CalibrationProfile`](#interface-bno055calibrationprofile) |

## Returns

`Uint8Array`

# Function: packBno055GyroConfig()

```ts
function packBno055GyroConfig(c): Uint8Array;
```

Defined in: [sensors/bno055/regs.ts:429](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L429)

A field left out is its power-on value (range 0, bandwidth 7, power 0).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `c` | `Partial`\<[`Bno055GyroConfig`](#interface-bno055gyroconfig)\> |

## Returns

`Uint8Array`

# Function: packBno055MagConfig()

```ts
function packBno055MagConfig(c): number;
```

Defined in: [sensors/bno055/regs.ts:446](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L446)

A field left out is its power-on value (rate 3, mode 1, power 0).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `c` | `Partial`\<[`Bno055MagConfig`](#interface-bno055magconfig)\> |

## Returns

`number`

# Function: packBno055ReadReg()

```ts
function packBno055ReadReg(addr, length): Uint8Array;
```

Defined in: [protocol/bno055.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L45)

BNO_READ_REG payload `addr u8, len u8` with the transfer limit of
contract 13 §1: `len` 1..128 and `addr + len <= 0x100`, else RangeError.

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

Defined in: [sensors/bno055/regs.ts:303](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L303)

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

Defined in: [protocol/bno055.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L73)

BNO_START_STREAM payload `trigger u8, addr u8, len u8, period_ms u16`. With
the TIMER trigger the period is 1..60000 ms (contract 13 §2), else
RangeError; with INT it is the missed-edge watchdog and 0 disables it.

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

Defined in: [sensors/bno055/regs.ts:201](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L201)

A flag left out is false (the SI default).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `u` | `Partial`\<[`Bno055Units`](#interface-bno055units)\> |

## Returns

`number`

# Function: packBno055WriteReg()

```ts
function packBno055WriteReg(addr, data): Uint8Array;
```

Defined in: [protocol/bno055.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L57)

BNO_WRITE_REG payload `addr u8, data[N]`: N 1..128 and `addr + N <= 0x100`, else RangeError.

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

Defined in: [sensors/bno055/regs.ts:416](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L416)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `v` | `number` |

## Returns

[`Bno055AccelConfig`](#interface-bno055accelconfig)

# Function: unpackBno055AxisRemap()

```ts
function unpackBno055AxisRemap(config, sign): Bno055AxisRemap;
```

Defined in: [sensors/bno055/regs.ts:362](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L362)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `config` | `number` |
| `sign` | `number` |

## Returns

[`Bno055AxisRemap`](#interface-bno055axisremap)

# Function: unpackBno055CalibStatus()

```ts
function unpackBno055CalibStatus(v): Bno055CalibStatus;
```

Defined in: [sensors/bno055/regs.ts:247](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L247)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `v` | `number` |

## Returns

[`Bno055CalibStatus`](#interface-bno055calibstatus)

# Function: unpackBno055CalibrationProfile()

```ts
function unpackBno055CalibrationProfile(data): Bno055CalibrationProfile;
```

Defined in: [sensors/bno055/regs.ts:285](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L285)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

## Returns

[`Bno055CalibrationProfile`](#interface-bno055calibrationprofile)

# Function: unpackBno055GyroConfig()

```ts
function unpackBno055GyroConfig(b): Bno055GyroConfig;
```

Defined in: [sensors/bno055/regs.ts:433](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L433)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `b` | `Uint8Array` |

## Returns

[`Bno055GyroConfig`](#interface-bno055gyroconfig)

# Function: unpackBno055Info()

```ts
function unpackBno055Info(payload): Bno055Info;
```

Defined in: [protocol/bno055.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L163)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Bno055Info`](#interface-bno055info)

# Function: unpackBno055MagConfig()

```ts
function unpackBno055MagConfig(v): Bno055MagConfig;
```

Defined in: [sensors/bno055/regs.ts:450](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L450)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `v` | `number` |

## Returns

[`Bno055MagConfig`](#interface-bno055magconfig)

# Function: unpackBno055RegData()

```ts
function unpackBno055RegData(payload): Bno055RegData;
```

Defined in: [protocol/bno055.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L105)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Bno055RegData`](#interface-bno055regdata)

# Function: unpackBno055SicMatrix()

```ts
function unpackBno055SicMatrix(data): number[];
```

Defined in: [sensors/bno055/regs.ts:311](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L311)

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

Defined in: [protocol/bno055.ts:215](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L215)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Bno055StreamData`](#interface-bno055streamdata)

# Function: unpackBno055Units()

```ts
function unpackBno055Units(value): Bno055Units;
```

Defined in: [sensors/bno055/regs.ts:211](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L211)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `value` | `number` |

## Returns

[`Bno055Units`](#interface-bno055units)

# Interface: Bno055AccelConfig

Defined in: [sensors/bno055/regs.ts:405](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L405)

Register codes: `range`/`bandwidth` index the tables above, `power` the
datasheet mode list. Power-on value 0x0D = ±4 g, 62.5 Hz, normal.

## Properties

### range

```ts
range: number;
```

Defined in: [sensors/bno055/regs.ts:406](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L406)

***

### bandwidth

```ts
bandwidth: number;
```

Defined in: [sensors/bno055/regs.ts:407](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L407)

***

### power

```ts
power: number;
```

Defined in: [sensors/bno055/regs.ts:408](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L408)

# Interface: Bno055AxisRemap

Defined in: [sensors/bno055/regs.ts:326](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L326)

Which physical axis feeds each output axis, and its sign: `x = AXIS_Y`
means "output X is the chip's Y axis".

## Properties

### x

```ts
x: number;
```

Defined in: [sensors/bno055/regs.ts:327](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L327)

***

### y

```ts
y: number;
```

Defined in: [sensors/bno055/regs.ts:328](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L328)

***

### z

```ts
z: number;
```

Defined in: [sensors/bno055/regs.ts:329](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L329)

***

### xNegative

```ts
xNegative: boolean;
```

Defined in: [sensors/bno055/regs.ts:330](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L330)

***

### yNegative

```ts
yNegative: boolean;
```

Defined in: [sensors/bno055/regs.ts:331](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L331)

***

### zNegative

```ts
zNegative: boolean;
```

Defined in: [sensors/bno055/regs.ts:332](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L332)

# Interface: Bno055CalibStatus

Defined in: [sensors/bno055/regs.ts:240](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L240)

CALIB_STAT (0x35): 0 = not calibrated … 3 = fully calibrated.

## Properties

### system

```ts
system: number;
```

Defined in: [sensors/bno055/regs.ts:241](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L241)

***

### gyro

```ts
gyro: number;
```

Defined in: [sensors/bno055/regs.ts:242](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L242)

***

### accel

```ts
accel: number;
```

Defined in: [sensors/bno055/regs.ts:243](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L243)

***

### mag

```ts
mag: number;
```

Defined in: [sensors/bno055/regs.ts:244](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L244)

# Interface: Bno055CalibrationProfile

Defined in: [sensors/bno055/regs.ts:268](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L268)

Sensor offsets and radii, registers 0x55..0x6A (22 bytes, all i16 LE).
Read after a full calibration (CONFIG mode), store, write back after
every power-on reset. A written profile is a starting point: fusion
refines it as soon as it resumes.

## Properties

### accelOffset

```ts
accelOffset: Vec3i;
```

Defined in: [sensors/bno055/regs.ts:269](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L269)

***

### magOffset

```ts
magOffset: Vec3i;
```

Defined in: [sensors/bno055/regs.ts:270](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L270)

***

### gyroOffset

```ts
gyroOffset: Vec3i;
```

Defined in: [sensors/bno055/regs.ts:271](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L271)

***

### accelRadius

```ts
accelRadius: number;
```

Defined in: [sensors/bno055/regs.ts:272](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L272)

***

### magRadius

```ts
magRadius: number;
```

Defined in: [sensors/bno055/regs.ts:273](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L273)

# Interface: Bno055ConfigureOptions

Defined in: [sensors/bno055/bno055.ts:177](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L177)

A units flag left out is false; an axis-remap field left out is its P1 value.

## Properties

### mode?

```ts
optional mode?: Bno055OprMode;
```

Defined in: [sensors/bno055/bno055.ts:178](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L178)

***

### units?

```ts
optional units?: Partial<Bno055Units>;
```

Defined in: [sensors/bno055/bno055.ts:179](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L179)

***

### axisRemap?

```ts
optional axisRemap?: string | Partial<Bno055AxisRemap>;
```

Defined in: [sensors/bno055/bno055.ts:181](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L181)

An axis remap, or a datasheet placement name "P0".."P7".

***

### calibration?

```ts
optional calibration?: Bno055CalibrationProfile;
```

Defined in: [sensors/bno055/bno055.ts:182](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L182)

# Interface: Bno055GyroConfig

Defined in: [sensors/bno055/regs.ts:422](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L422)

GYR_CONFIG_0/1 as register codes. Power-on value 0x38/0x00 = 2000 dps, 32 Hz, normal.

## Properties

### range

```ts
range: number;
```

Defined in: [sensors/bno055/regs.ts:423](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L423)

***

### bandwidth

```ts
bandwidth: number;
```

Defined in: [sensors/bno055/regs.ts:424](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L424)

***

### power

```ts
power: number;
```

Defined in: [sensors/bno055/regs.ts:425](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L425)

# Interface: Bno055Info

Defined in: [protocol/bno055.ts:129](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L129)

RPT_BNO_INFO — sensor identity (registers 0x00..0x06) plus bridge
diagnostics. Counters wrap silently; `i2cErrors` is cleared by every
successful reset handshake, the read timings and `slotsSkipped` at
START_STREAM. A rising
`sensorResets` means the bridge pulsed nRESET to recover the bus: the
sensor is back in CONFIG mode and the host must restore its configuration.

## Properties

### i2cAddr

```ts
i2cAddr: number;
```

Defined in: [protocol/bno055.ts:131](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L131)

7-bit sensor address in use (0x28).

***

### chipId

```ts
chipId: number;
```

Defined in: [protocol/bno055.ts:132](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L132)

***

### accId

```ts
accId: number;
```

Defined in: [protocol/bno055.ts:133](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L133)

***

### magId

```ts
magId: number;
```

Defined in: [protocol/bno055.ts:134](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L134)

***

### gyrId

```ts
gyrId: number;
```

Defined in: [protocol/bno055.ts:135](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L135)

***

### swRev

```ts
swRev: number;
```

Defined in: [protocol/bno055.ts:137](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L137)

Sensor firmware, BCD: 0x0311 = 03.11.

***

### blRev

```ts
blRev: number;
```

Defined in: [protocol/bno055.ts:138](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L138)

***

### initialized

```ts
initialized: number;
```

Defined in: [protocol/bno055.ts:140](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L140)

1 = chip-ID handshake passed.

***

### intLevel

```ts
intLevel: number;
```

Defined in: [protocol/bno055.ts:141](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L141)

***

### intEdges

```ts
intEdges: number;
```

Defined in: [protocol/bno055.ts:143](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L143)

EXTI rising edges (counted only while an INT stream is armed).

***

### readMinUs

```ts
readMinUs: number;
```

Defined in: [protocol/bno055.ts:145](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L145)

Streamed block read timing since the last START_STREAM.

***

### readMaxUs

```ts
readMaxUs: number;
```

Defined in: [protocol/bno055.ts:146](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L146)

***

### readAvgUs

```ts
readAvgUs: number;
```

Defined in: [protocol/bno055.ts:147](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L147)

***

### txDropped

```ts
txDropped: number;
```

Defined in: [protocol/bno055.ts:149](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L149)

Packets refused by a full USB TX ring.

***

### i2cErrors

```ts
i2cErrors: number;
```

Defined in: [protocol/bno055.ts:151](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L151)

Failed transfers since the last successful reset handshake.

***

### slotsSkipped

```ts
slotsSkipped: number;
```

Defined in: [protocol/bno055.ts:153](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L153)

Stream slots dropped: bus still busy.

***

### busRecoveries

```ts
busRecoveries: number;
```

Defined in: [protocol/bno055.ts:154](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L154)

***

### lastI2cError

```ts
lastI2cError: number;
```

Defined in: [protocol/bno055.ts:155](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L155)

***

### sensorResets

```ts
sensorResets: number;
```

Defined in: [protocol/bno055.ts:157](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L157)

Rung-3 nRESET recoveries (sensor back in CONFIG).

***

### loopMaxUs

```ts
loopMaxUs: number;
```

Defined in: [protocol/bno055.ts:158](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L158)

# Interface: Bno055MagConfig

Defined in: [sensors/bno055/regs.ts:439](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L439)

MAG_CONFIG as register codes. Power-on value 0x0B = 10 Hz, regular, normal.

## Properties

### rate

```ts
rate: number;
```

Defined in: [sensors/bno055/regs.ts:440](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L440)

***

### mode

```ts
mode: number;
```

Defined in: [sensors/bno055/regs.ts:441](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L441)

***

### power

```ts
power: number;
```

Defined in: [sensors/bno055/regs.ts:442](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L442)

# Interface: Bno055Options

Defined in: [sensors/bno055/bno055.ts:171](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L171)

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

Defined in: [sensors/bno055/bno055.ts:173](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L173)

Sleep implementation (tests inject an instant one).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Interface: Bno055RawBlock

Defined in: [sensors/bno055/regs.ts:491](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L491)

Raw register values found in one block read. A channel is null when the
window `addr..addr+len` does not cover it completely.

## Properties

### accel

```ts
accel: Vec3i | null;
```

Defined in: [sensors/bno055/regs.ts:492](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L492)

***

### mag

```ts
mag: Vec3i | null;
```

Defined in: [sensors/bno055/regs.ts:493](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L493)

***

### gyro

```ts
gyro: Vec3i | null;
```

Defined in: [sensors/bno055/regs.ts:494](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L494)

***

### euler

```ts
euler: Vec3i | null;
```

Defined in: [sensors/bno055/regs.ts:496](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L496)

heading, roll, pitch

***

### quaternion

```ts
quaternion: [number, number, number, number] | null;
```

Defined in: [sensors/bno055/regs.ts:498](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L498)

w, x, y, z

***

### linearAccel

```ts
linearAccel: Vec3i | null;
```

Defined in: [sensors/bno055/regs.ts:499](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L499)

***

### gravity

```ts
gravity: Vec3i | null;
```

Defined in: [sensors/bno055/regs.ts:500](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L500)

***

### temperature

```ts
temperature: number | null;
```

Defined in: [sensors/bno055/regs.ts:501](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L501)

***

### calibStat

```ts
calibStat: number | null;
```

Defined in: [sensors/bno055/regs.ts:502](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L502)

# Interface: Bno055RegData

Defined in: [protocol/bno055.ts:97](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L97)

RPT_BNO_REG_DATA payload.

## Properties

### cmd

```ts
cmd: number;
```

Defined in: [protocol/bno055.ts:99](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L99)

Echoed READ_REG opcode.

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [protocol/bno055.ts:101](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L101)

MCU uptime at I2C-read completion.

***

### data

```ts
data: Uint8Array;
```

Defined in: [protocol/bno055.ts:102](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L102)

# Interface: Bno055Sample

Defined in: [sensors/bno055/bno055.ts:118](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L118)

One decoded register block (streamed or polled). Channels are null when
the block did not cover them; values are scaled by `units` — the units in
force when the stream started (contract 13 §4.2).

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/bno055/bno055.ts:120](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L120)

MCU uptime: trigger time (stream) / read completion (poll).

***

### addr

```ts
addr: number;
```

Defined in: [sensors/bno055/bno055.ts:121](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L121)

***

### raw

```ts
raw: Uint8Array;
```

Defined in: [sensors/bno055/bno055.ts:122](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L122)

***

### units

```ts
units: Bno055Units;
```

Defined in: [sensors/bno055/bno055.ts:123](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L123)

***

### accel

```ts
accel: Vec3 | null;
```

Defined in: [sensors/bno055/bno055.ts:125](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L125)

m/s² or mg

***

### mag

```ts
mag: Vec3 | null;
```

Defined in: [sensors/bno055/bno055.ts:127](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L127)

µT

***

### gyro

```ts
gyro: Vec3 | null;
```

Defined in: [sensors/bno055/bno055.ts:129](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L129)

dps or rps

***

### euler

```ts
euler: Vec3 | null;
```

Defined in: [sensors/bno055/bno055.ts:131](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L131)

heading, roll, pitch — degrees or radians

***

### quaternion

```ts
quaternion: [number, number, number, number] | null;
```

Defined in: [sensors/bno055/bno055.ts:133](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L133)

w, x, y, z (unit quaternion)

***

### linearAccel

```ts
linearAccel: Vec3 | null;
```

Defined in: [sensors/bno055/bno055.ts:135](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L135)

Acceleration minus gravity, always m/s².

***

### gravity

```ts
gravity: Vec3 | null;
```

Defined in: [sensors/bno055/bno055.ts:137](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L137)

Always m/s².

***

### temperature

```ts
temperature: number | null;
```

Defined in: [sensors/bno055/bno055.ts:139](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L139)

°C or °F

***

### calibration

```ts
calibration: Bno055CalibStatus | null;
```

Defined in: [sensors/bno055/bno055.ts:140](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/bno055.ts#L140)

# Interface: Bno055StreamData

Defined in: [protocol/bno055.ts:207](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L207)

RPT_BNO_REG_STREAM — one streamed register block. `addr`/`length` echo the
stream configuration so each report is self-describing.

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [protocol/bno055.ts:209](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L209)

MCU uptime at the trigger (timer expiry or INT edge).

***

### addr

```ts
addr: number;
```

Defined in: [protocol/bno055.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L210)

***

### length

```ts
length: number;
```

Defined in: [protocol/bno055.ts:211](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L211)

***

### data

```ts
data: Uint8Array;
```

Defined in: [protocol/bno055.ts:212](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L212)

# Interface: Bno055SystemStatus

Defined in: [sensors/bno055/regs.ts:458](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L458)

ST_RESULT (0x36) + SYS_CLK_STATUS/SYS_STATUS/SYS_ERR (0x38..0x3A).

## Properties

### selfTest

```ts
selfTest: number;
```

Defined in: [sensors/bno055/regs.ts:459](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L459)

***

### clkStatus

```ts
clkStatus: number;
```

Defined in: [sensors/bno055/regs.ts:460](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L460)

***

### status

```ts
status: number;
```

Defined in: [sensors/bno055/regs.ts:461](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L461)

***

### error

```ts
error: number;
```

Defined in: [sensors/bno055/regs.ts:462](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L462)

***

### statusText

```ts
statusText: string;
```

Defined in: [sensors/bno055/regs.ts:463](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L463)

***

### errorText

```ts
errorText: string;
```

Defined in: [sensors/bno055/regs.ts:464](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L464)

***

### selfTestPassed

```ts
selfTestPassed: boolean;
```

Defined in: [sensors/bno055/regs.ts:465](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L465)

# Interface: Bno055Units

Defined in: [sensors/bno055/regs.ts:183](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L183)

Output units. Default (all false) = m/s², dps, degrees, °C, Windows
orientation (UNIT_SEL 0x00). The sensor's power-on value is 0x80.

## Properties

### accelMg

```ts
accelMg: boolean;
```

Defined in: [sensors/bno055/regs.ts:185](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L185)

ACC_DATA in mg, else m/s² (linear accel / gravity stay m/s²).

***

### gyroRps

```ts
gyroRps: boolean;
```

Defined in: [sensors/bno055/regs.ts:186](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L186)

***

### eulerRad

```ts
eulerRad: boolean;
```

Defined in: [sensors/bno055/regs.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L187)

***

### tempF

```ts
tempF: boolean;
```

Defined in: [sensors/bno055/regs.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L188)

***

### android

```ts
android: boolean;
```

Defined in: [sensors/bno055/regs.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L189)

# Variable: BNO055\_ACC\_BANDWIDTH\_HZ

```ts
const BNO055_ACC_BANDWIDTH_HZ: readonly number[];
```

Defined in: [sensors/bno055/regs.ts:396](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L396)

# Variable: BNO055\_ACC\_RANGE\_G

```ts
const BNO055_ACC_RANGE_G: readonly number[];
```

Defined in: [sensors/bno055/regs.ts:395](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L395)

# Variable: BNO055\_AXIS\_X

```ts
const BNO055_AXIS_X: 0 = 0;
```

Defined in: [sensors/bno055/regs.ts:318](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L318)

# Variable: BNO055\_AXIS\_Y

```ts
const BNO055_AXIS_Y: 1 = 1;
```

Defined in: [sensors/bno055/regs.ts:319](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L319)

# Variable: BNO055\_AXIS\_Z

```ts
const BNO055_AXIS_Z: 2 = 2;
```

Defined in: [sensors/bno055/regs.ts:320](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L320)

# Variable: BNO055\_CALIB\_PROFILE\_LEN

```ts
const BNO055_CALIB_PROFILE_LEN: 22 = 22;
```

Defined in: [sensors/bno055/regs.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L44)

# Variable: BNO055\_DEFAULT\_AXIS\_REMAP

```ts
const BNO055_DEFAULT_AXIS_REMAP: Readonly<Bno055AxisRemap>;
```

Defined in: [sensors/bno055/regs.ts:335](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L335)

# Variable: BNO055\_DEFAULT\_UNITS

```ts
const BNO055_DEFAULT_UNITS: Readonly<Bno055Units>;
```

Defined in: [sensors/bno055/regs.ts:192](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L192)

# Variable: BNO055\_EXPECTED\_IDS

```ts
const BNO055_EXPECTED_IDS: {
  chipId: 160;
  accId: 251;
  magId: 50;
  gyrId: 15;
};
```

Defined in: [protocol/bno055.ts:119](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L119)

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

# Variable: BNO055\_FULL\_BLOCK

```ts
const BNO055_FULL_BLOCK: readonly [number, number];
```

Defined in: [sensors/bno055/regs.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L60)

[addr, len] of the block carrying every output channel (0x08..0x35).

# Variable: BNO055\_GYR\_BANDWIDTH\_HZ

```ts
const BNO055_GYR_BANDWIDTH_HZ: readonly number[];
```

Defined in: [sensors/bno055/regs.ts:398](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L398)

# Variable: BNO055\_GYR\_RANGE\_DPS

```ts
const BNO055_GYR_RANGE_DPS: readonly number[];
```

Defined in: [sensors/bno055/regs.ts:397](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L397)

# Variable: BNO055\_I2C\_ERROR\_NAMES

```ts
const BNO055_I2C_ERROR_NAMES: Readonly<Record<number, string>>;
```

Defined in: [protocol/bno055.ts:111](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L111)

last_i2c_error values in RPT_BNO_INFO.

# Variable: BNO055\_INFO\_SIZE

```ts
const BNO055_INFO_SIZE: 38 = 38;
```

Defined in: [protocol/bno055.ts:161](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L161)

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

Defined in: [sensors/bno055/regs.ts:74](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L74)

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

# Variable: BNO055\_MAG\_RATE\_HZ

```ts
const BNO055_MAG_RATE_HZ: readonly number[];
```

Defined in: [sensors/bno055/regs.ts:399](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L399)

# Variable: BNO055\_PLACEMENTS

```ts
const BNO055_PLACEMENTS: Readonly<Record<string, readonly [number, number]>>;
```

Defined in: [sensors/bno055/regs.ts:376](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L376)

Datasheet §3.4: placement → [AXIS_MAP_CONFIG, AXIS_MAP_SIGN]; P1 is the default.

# Variable: BNO055\_QUAT\_BLOCK

```ts
const BNO055_QUAT_BLOCK: readonly [number, number];
```

Defined in: [sensors/bno055/regs.ts:62](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L62)

[addr, len] of the quaternion alone — the cheapest orientation read.

# Variable: BNO055\_REG1\_ACC\_CONFIG

```ts
const BNO055_REG1_ACC_CONFIG: 8 = 0x08;
```

Defined in: [sensors/bno055/regs.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L47)

# Variable: BNO055\_REG1\_GYR\_CONFIG\_0

```ts
const BNO055_REG1_GYR_CONFIG_0: 10 = 0x0a;
```

Defined in: [sensors/bno055/regs.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L49)

# Variable: BNO055\_REG1\_GYR\_CONFIG\_1

```ts
const BNO055_REG1_GYR_CONFIG_1: 11 = 0x0b;
```

Defined in: [sensors/bno055/regs.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L50)

# Variable: BNO055\_REG1\_INT\_EN

```ts
const BNO055_REG1_INT_EN: 16 = 0x10;
```

Defined in: [sensors/bno055/regs.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L52)

# Variable: BNO055\_REG1\_INT\_MSK

```ts
const BNO055_REG1_INT_MSK: 15 = 0x0f;
```

Defined in: [sensors/bno055/regs.ts:51](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L51)

# Variable: BNO055\_REG1\_INT\_SETTINGS\_FIRST

```ts
const BNO055_REG1_INT_SETTINGS_FIRST: 17 = 0x11;
```

Defined in: [sensors/bno055/regs.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L54)

First / last page-1 motion-interrupt setting register (written raw).

# Variable: BNO055\_REG1\_INT\_SETTINGS\_LAST

```ts
const BNO055_REG1_INT_SETTINGS_LAST: 31 = 0x1f;
```

Defined in: [sensors/bno055/regs.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L55)

# Variable: BNO055\_REG1\_MAG\_CONFIG

```ts
const BNO055_REG1_MAG_CONFIG: 9 = 0x09;
```

Defined in: [sensors/bno055/regs.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L48)

# Variable: BNO055\_REG1\_UNIQUE\_ID

```ts
const BNO055_REG1_UNIQUE_ID: 80 = 0x50;
```

Defined in: [sensors/bno055/regs.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L56)

# Variable: BNO055\_REG\_ACC\_DATA

```ts
const BNO055_REG_ACC_DATA: 8 = 0x08;
```

Defined in: [sensors/bno055/regs.ts:16](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L16)

# Variable: BNO055\_REG\_AXIS\_MAP\_CONFIG

```ts
const BNO055_REG_AXIS_MAP_CONFIG: 65 = 0x41;
```

Defined in: [sensors/bno055/regs.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L38)

# Variable: BNO055\_REG\_AXIS\_MAP\_SIGN

```ts
const BNO055_REG_AXIS_MAP_SIGN: 66 = 0x42;
```

Defined in: [sensors/bno055/regs.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L39)

# Variable: BNO055\_REG\_CALIB\_PROFILE

```ts
const BNO055_REG_CALIB_PROFILE: 85 = 0x55;
```

Defined in: [sensors/bno055/regs.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L43)

acc/mag/gyr offsets + acc/mag radius.

# Variable: BNO055\_REG\_CALIB\_STAT

```ts
const BNO055_REG_CALIB_STAT: 53 = 0x35;
```

Defined in: [sensors/bno055/regs.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L26)

# Variable: BNO055\_REG\_CHIP\_ID

```ts
const BNO055_REG_CHIP_ID: 0 = 0x00;
```

Defined in: [sensors/bno055/regs.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L14)

# Variable: BNO055\_REG\_EUL\_DATA

```ts
const BNO055_REG_EUL_DATA: 26 = 0x1a;
```

Defined in: [sensors/bno055/regs.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L20)

heading, roll, pitch

# Variable: BNO055\_REG\_GRV\_DATA

```ts
const BNO055_REG_GRV_DATA: 46 = 0x2e;
```

Defined in: [sensors/bno055/regs.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L24)

# Variable: BNO055\_REG\_GYR\_DATA

```ts
const BNO055_REG_GYR_DATA: 20 = 0x14;
```

Defined in: [sensors/bno055/regs.ts:18](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L18)

# Variable: BNO055\_REG\_INT\_STA

```ts
const BNO055_REG_INT_STA: 55 = 0x37;
```

Defined in: [sensors/bno055/regs.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L29)

Clear-on-read — never part of a routine block read.

# Variable: BNO055\_REG\_LIA\_DATA

```ts
const BNO055_REG_LIA_DATA: 40 = 0x28;
```

Defined in: [sensors/bno055/regs.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L23)

# Variable: BNO055\_REG\_MAG\_DATA

```ts
const BNO055_REG_MAG_DATA: 14 = 0x0e;
```

Defined in: [sensors/bno055/regs.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L17)

# Variable: BNO055\_REG\_OPR\_MODE

```ts
const BNO055_REG_OPR_MODE: 61 = 0x3d;
```

Defined in: [sensors/bno055/regs.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L34)

# Variable: BNO055\_REG\_PAGE\_ID

```ts
const BNO055_REG_PAGE_ID: 7 = 0x07;
```

Defined in: [sensors/bno055/regs.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L15)

# Variable: BNO055\_REG\_PWR\_MODE

```ts
const BNO055_REG_PWR_MODE: 62 = 0x3e;
```

Defined in: [sensors/bno055/regs.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L35)

# Variable: BNO055\_REG\_QUA\_DATA

```ts
const BNO055_REG_QUA_DATA: 32 = 0x20;
```

Defined in: [sensors/bno055/regs.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L22)

w, x, y, z

# Variable: BNO055\_REG\_SIC\_MATRIX

```ts
const BNO055_REG_SIC_MATRIX: 67 = 0x43;
```

Defined in: [sensors/bno055/regs.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L41)

9 × i16, row-major.

# Variable: BNO055\_REG\_ST\_RESULT

```ts
const BNO055_REG_ST_RESULT: 54 = 0x36;
```

Defined in: [sensors/bno055/regs.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L27)

# Variable: BNO055\_REG\_SYS\_CLK\_STATUS

```ts
const BNO055_REG_SYS_CLK_STATUS: 56 = 0x38;
```

Defined in: [sensors/bno055/regs.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L30)

# Variable: BNO055\_REG\_SYS\_ERR

```ts
const BNO055_REG_SYS_ERR: 58 = 0x3a;
```

Defined in: [sensors/bno055/regs.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L32)

# Variable: BNO055\_REG\_SYS\_STATUS

```ts
const BNO055_REG_SYS_STATUS: 57 = 0x39;
```

Defined in: [sensors/bno055/regs.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L31)

# Variable: BNO055\_REG\_SYS\_TRIGGER

```ts
const BNO055_REG_SYS_TRIGGER: 63 = 0x3f;
```

Defined in: [sensors/bno055/regs.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L36)

# Variable: BNO055\_REG\_TEMP

```ts
const BNO055_REG_TEMP: 52 = 0x34;
```

Defined in: [sensors/bno055/regs.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L25)

# Variable: BNO055\_REG\_TEMP\_SOURCE

```ts
const BNO055_REG_TEMP_SOURCE: 64 = 0x40;
```

Defined in: [sensors/bno055/regs.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L37)

# Variable: BNO055\_REG\_UNIT\_SEL

```ts
const BNO055_REG_UNIT_SEL: 59 = 0x3b;
```

Defined in: [sensors/bno055/regs.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L33)

# Variable: BNO055\_RESET\_TIMEOUT\_MS

```ts
const BNO055_RESET_TIMEOUT_MS: 3000 = 3000;
```

Defined in: [protocol/bno055.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L33)

BNO_RESET answers after the sensor's ~0.5 s boot handshake.

# Variable: BNO055\_SIC\_IDENTITY

```ts
const BNO055_SIC_IDENTITY: readonly number[];
```

Defined in: [sensors/bno055/regs.ts:301](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L301)

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

Defined in: [sensors/bno055/regs.ts:86](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L86)

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

# Variable: BNO055\_SYS\_ERR\_NAMES

```ts
const BNO055_SYS_ERR_NAMES: Readonly<Record<number, string>>;
```

Defined in: [sensors/bno055/regs.ts:155](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L155)

# Variable: BNO055\_SYS\_STATUS\_NAMES

```ts
const BNO055_SYS_STATUS_NAMES: Readonly<Record<number, string>>;
```

Defined in: [sensors/bno055/regs.ts:145](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L145)

# Variable: BNO055\_TIMER\_PERIOD\_MAX\_MS

```ts
const BNO055_TIMER_PERIOD_MAX_MS: 60000 = 60000;
```

Defined in: [protocol/bno055.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L39)

# Variable: BNO055\_TIMER\_PERIOD\_MIN\_MS

```ts
const BNO055_TIMER_PERIOD_MIN_MS: 1 = 1;
```

Defined in: [protocol/bno055.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L38)

BNO_START_STREAM TIMER period limits, ms (contract 13 §2).

# Variable: BNO055\_TRIGGER\_INT

```ts
const BNO055_TRIGGER_INT: 1 = 1;
```

Defined in: [protocol/bno055.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L30)

BNO_START_STREAM trigger: read on the INT edge; period_ms is a missed-edge watchdog.

# Variable: BNO055\_TRIGGER\_TIMER

```ts
const BNO055_TRIGGER_TIMER: 0 = 0;
```

Defined in: [protocol/bno055.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L28)

BNO_START_STREAM trigger: read every period_ms (the only data trigger on SW 03.11).

# Variable: BNO055\_UNIQUE\_ID\_LEN

```ts
const BNO055_UNIQUE_ID_LEN: 16 = 16;
```

Defined in: [sensors/bno055/regs.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno055/regs.ts#L57)

# Variable: BNO055\_WRITE\_MAX

```ts
const BNO055_WRITE_MAX: 128 = BNO055_XFER_MAX;
```

Defined in: [protocol/bno055.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L54)

BNO_WRITE_REG data length limit (contract 13 §2: 1..128 bytes).

# Variable: BNO055\_XFER\_MAX

```ts
const BNO055_XFER_MAX: 128 = 128;
```

Defined in: [protocol/bno055.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno055.ts#L25)

Max bytes per READ_REG / WRITE_REG / streamed block; `addr + len` ≤ 0x100.

