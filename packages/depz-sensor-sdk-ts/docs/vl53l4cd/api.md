# VL53L4CD (ToF) — API reference

The VL53L4CD single-zone time-of-flight surface: the `Vl53l4cd` class,
the values its methods take and return, and the register-bridge wire
codecs (the ST ULD 2.2.3 driver underneath is not public — only the
unstable `uld` escape hatch reaches it). Discovery, device-base, transport and
other cross-sensor symbols shared by every sensor live in the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Enumerations

- [Vl53l4Cmd](#enumeration-vl53l4cmd)
- [Vl53l4Rpt](#enumeration-vl53l4rpt)

## Classes

- [Vl53l4cdError](#class-vl53l4cderror)
- [Vl53l4cd](#class-vl53l4cd)

## Interfaces

- [Vl53l4RegData](#interface-vl53l4regdata)
- [Vl53l4Info](#interface-vl53l4info)
- [Vl53l4StreamData](#interface-vl53l4streamdata)
- [RangeTiming](#interface-rangetiming)
- [Vl53l4Measurement](#interface-vl53l4measurement)
- [Vl53l4Options](#interface-vl53l4options)

## Variables

- [VL53L4\_XFER\_MAX](#variable-vl53l4_xfer_max)
- [VL53L4\_XSHUT\_OFF](#variable-vl53l4_xshut_off)
- [VL53L4\_XSHUT\_ON](#variable-vl53l4_xshut_on)
- [VL53L4\_XSHUT\_RESET](#variable-vl53l4_xshut_reset)
- [VL53L4\_SF\_INT\_ACT\_HIGH](#variable-vl53l4_sf_int_act_high)
- [VL53L4\_I2C\_KHZ\_STEPS](#variable-vl53l4_i2c_khz_steps)
- [VL53L4\_I2C\_ERROR\_NAMES](#variable-vl53l4_i2c_error_names)
- [MODEL\_ID\_VL53L4CD](#variable-model_id_vl53l4cd)
- [WINDOW\_BELOW](#variable-window_below)
- [WINDOW\_ABOVE](#variable-window_above)
- [WINDOW\_OUT](#variable-window_out)
- [WINDOW\_IN](#variable-window_in)
- [I2C\_KHZ\_BOOT](#variable-i2c_khz_boot)
- [I2C\_KHZ\_DEFAULT](#variable-i2c_khz_default)
- [RANGE\_STATUS\_NAMES](#variable-range_status_names)

## Functions

- [packVl53l4ReadReg](#function-packvl53l4readreg)
- [packVl53l4WriteReg](#function-packvl53l4writereg)
- [packVl53l4Xshut](#function-packvl53l4xshut)
- [packVl53l4StartStream](#function-packvl53l4startstream)
- [packVl53l4SetI2cSpeed](#function-packvl53l4seti2cspeed)
- [unpackVl53l4RegData](#function-unpackvl53l4regdata)
- [unpackVl53l4Info](#function-unpackvl53l4info)
- [unpackVl53l4Stream](#function-unpackvl53l4stream)
- [rangeStatusText](#function-rangestatustext)

# Class: Vl53l4cd

Defined in: [sensors/vl53l4/vl53l4.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L98)

VL53L4CD single-zone ToF device.

`init()` runs the ULD boot sequence (no firmware blob — the sensor carries
its own), then configure and `startRanging()`. Configuration methods must
not be called while ranging: the INT-driven stream owns the register bank
(contract 10). `measurements` is the stream (contract 07 §3).

## Extends

- `DepzDevice`

## Constructors

### Constructor

```ts
new Vl53l4cd(transport, opts?): Vl53l4cd;
```

Defined in: [sensors/vl53l4/vl53l4.ts:146](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L146)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | [`Vl53l4Options`](#interface-vl53l4options) |

#### Returns

`Vl53l4cd`

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

### measurements

```ts
readonly measurements: Stream<Vl53l4Measurement>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:106](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L106)

The streamed measurements (contract 07 §3): `on(cb)` subscribes,
`subscribe(maxsize)` iterates, `next(timeoutMs)` waits for one;
`droppedCounts` and `parseErrors` (stream reports whose result block did
not decode — a short block from a reconfigured stream, a corrupt read)
are its counters.

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

### uld

#### Get Signature

```ts
get uld(): VL53L4CD;
```

Defined in: [sensors/vl53l4/vl53l4.ts:160](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L160)

The underlying ULD driver (escape hatch for raw register access).
Unstable — not covered by the 1.0 freeze: the driver class and its
methods follow the ST ULD release the SDK ports, and the class is not
exported from the package root.

##### Returns

`VL53L4CD`

***

### initialized

#### Get Signature

```ts
get initialized(): boolean;
```

Defined in: [sensors/vl53l4/vl53l4.ts:168](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L168)

True after a successful init(). Cleared by resetSensor() and xshut() — a
power-cycled sensor holds none of the ULD configuration.

##### Returns

`boolean`

***

### ranging

#### Get Signature

```ts
get ranging(): boolean;
```

Defined in: [sensors/vl53l4/vl53l4.ts:402](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L402)

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

### isAlive()

```ts
isAlive(): Promise<boolean>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:173](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L173)

True when the sensor answers with the VL53L4CD model id (0xEBAA).

#### Returns

`Promise`\<`boolean`\>

***

### init()

```ts
init(busKhz?): Promise<void>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L187)

Initialise the sensor: default configuration block + VHV calibration
(ULD sensorInit). Takes well under a second; the bus is left at `busKhz`
(one of I2C_KHZ_STEPS).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `busKhz` | `number` | `I2C_KHZ_DEFAULT` |

#### Returns

`Promise`\<`void`\>

***

### xshut()

```ts
xshut(action): Promise<void>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:200](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L200)

Drive the XSHUT pin: XSHUT_OFF / XSHUT_ON / XSHUT_RESET. OFF and RESET
stop any active stream on the bridge; a power-cycled sensor needs init()
again.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `action` | `number` |

#### Returns

`Promise`\<`void`\>

***

### resetSensor()

```ts
resetSensor(): Promise<void>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:212](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L212)

Hardware sensor reset via XSHUT (blocks ≤ 20 ms on the MCU, typ. ~2 ms;
the reply is awaited up to 2 s). The ULD
configuration is wiped — call init() again.

#### Returns

`Promise`\<`void`\>

***

### bridgeInfo()

```ts
bridgeInfo(): Promise<Vl53l4Info>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:223](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L223)

RPT_VL53_INFO: sensor identity, pin levels and bridge counters. Counters
wrap silently — watch increments; `i2cErrors` is cleared by a successful
XSHUT reset. Safe to call while streaming.

#### Returns

`Promise`\<[`Vl53l4Info`](#interface-vl53l4info)\>

***

### setI2cSpeedKhz()

```ts
setI2cSpeedKhz(khz): Promise<void>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:234](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L234)

Re-time the bridge's I2C bus to the nominal step nearest `khz`
(I2C_KHZ_STEPS). Not while ranging — re-timing refuses a transfer in
flight (ERR_BUSY). Read back the programmed step via bridgeInfo().

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `khz` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getRangeTiming()

```ts
getRangeTiming(): Promise<RangeTiming>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:245](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L245)

→ { timingBudgetMs, interMeasurementMs }. interMeasurement 0 means
continuous mode.

#### Returns

`Promise`\<[`RangeTiming`](#interface-rangetiming)\>

***

### setRangeTiming()

```ts
setRangeTiming(timingBudgetMs, interMeasurementMs?): Promise<void>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:254](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L254)

Set the timing budget (10–200 ms) and inter-measurement period.
`interMeasurementMs = 0` selects continuous ranging; a value larger than
the budget selects autonomous low-power mode. Not while ranging.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timingBudgetMs` | `number` | `undefined` |
| `interMeasurementMs` | `number` | `0` |

#### Returns

`Promise`\<`void`\>

***

### getOffsetMm()

```ts
getOffsetMm(): Promise<number>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:260](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L260)

Configured ranging offset in mm (signed).

#### Returns

`Promise`\<`number`\>

***

### setOffsetMm()

```ts
setOffsetMm(offsetMm): Promise<void>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:265](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L265)

Set the ranging offset correction in mm. Not while ranging.

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

Defined in: [sensors/vl53l4/vl53l4.ts:271](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L271)

Configured crosstalk compensation in kcps (0 = disabled).

#### Returns

`Promise`\<`number`\>

***

### setXtalkKcps()

```ts
setXtalkKcps(xtalkKcps): Promise<void>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:276](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L276)

Set the crosstalk compensation in kcps. Not while ranging.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `xtalkKcps` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getDetectionThresholds()

```ts
getDetectionThresholds(): Promise<{
  distanceLowMm: number;
  distanceHighMm: number;
  window: number;
}>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:285](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L285)

→ { distanceLowMm, distanceHighMm, window }. Window is one of
WINDOW_BELOW / WINDOW_ABOVE / WINDOW_OUT / WINDOW_IN.

#### Returns

`Promise`\<\{
  `distanceLowMm`: `number`;
  `distanceHighMm`: `number`;
  `window`: `number`;
\}\>

***

### setDetectionThresholds()

```ts
setDetectionThresholds(
   distanceLowMm, 
   distanceHighMm, 
window): Promise<void>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:299](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L299)

Program the distance-window interrupt (INT only fires when the window
condition holds). Not while ranging. The distances are u16 registers, the
window a u8 one: a value outside is refused before anything is written,
never masked (contract 07 §3).

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

Defined in: [sensors/vl53l4/vl53l4.ts:311](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L311)

#### Returns

`Promise`\<`number`\>

***

### setSignalThresholdKcps()

```ts
setSignalThresholdKcps(signalKcps): Promise<void>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:319](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L319)

Discard measurements whose return signal is below `signalKcps`. Not
while ranging.

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

Defined in: [sensors/vl53l4/vl53l4.ts:324](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L324)

#### Returns

`Promise`\<`number`\>

***

### setSigmaThresholdMm()

```ts
setSigmaThresholdMm(sigmaMm): Promise<void>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:332](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L332)

Discard measurements whose sigma exceeds `sigmaMm` (≤ 16383). Not while
ranging.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `sigmaMm` | `number` |

#### Returns

`Promise`\<`void`\>

***

### startTemperatureUpdate()

```ts
startTemperatureUpdate(): Promise<void>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:341](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L341)

Re-run VHV calibration; recommended after a >8 °C ambient change. Not
while ranging (runs a short ranging burst internally).

#### Returns

`Promise`\<`void`\>

***

### calibrateOffset()

```ts
calibrateOffset(targetDistMm, nbSamples?): Promise<number>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:350](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L350)

Offset calibration against a target at `targetDistMm` (10–1000). Blocks
for the sample burst; returns the offset now programmed.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `targetDistMm` | `number` | `undefined` |
| `nbSamples` | `number` | `20` |

#### Returns

`Promise`\<`number`\>

***

### calibrateXtalk()

```ts
calibrateXtalk(targetDistMm, nbSamples?): Promise<number>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:359](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L359)

Crosstalk calibration against a target at `targetDistMm` (10–5000).
Blocks for the sample burst; returns the xtalk now programmed (kcps).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `targetDistMm` | `number` | `undefined` |
| `nbSamples` | `number` | `20` |

#### Returns

`Promise`\<`number`\>

***

### startRanging()

```ts
startRanging(): Promise<void>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:370](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L370)

Start the sensor's ranging loop and arm the MCU stream: one
RPT_VL53_STREAM per INT edge carrying the 17-byte result block.

#### Returns

`Promise`\<`void`\>

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:389](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L389)

#### Returns

`Promise`\<`void`\>

***

### measureOnce()

```ts
measureOnce(timeoutMs?): Promise<Vl53l4Measurement>;
```

Defined in: [sensors/vl53l4/vl53l4.ts:410](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L410)

Single poll-mode measurement: start ranging, wait for data-ready, read
the result block, stop. Rejects while the stream is running.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `1000` |

#### Returns

`Promise`\<[`Vl53l4Measurement`](#interface-vl53l4measurement)\>

***

### requireNotRanging()

```ts
protected requireNotRanging(): void;
```

Defined in: [sensors/vl53l4/vl53l4.ts:426](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L426)

#### Returns

`void`

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [sensors/vl53l4/vl53l4.ts:433](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L433)

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

# Class: Vl53l4cdError

Defined in: [sensors/vl53l4/uld.ts:154](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L154)

What the sensor did, not what the caller asked: a boot or data-ready
timeout, a short result block, an oscillator word of 0, a failed xtalk
calibration. A refused argument is a RangeError (contract 07 §3).

## Extends

- `Error`

## Constructors

### Constructor

```ts
new Vl53l4cdError(message?): Vl53l4cdError;
```

Defined in: [sensors/vl53l4/uld.ts:155](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L155)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `message?` | `string` |

#### Returns

`Vl53l4cdError`

#### Overrides

```ts
Error.constructor
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
Error.stackTraceLimit
```

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: ../node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

```ts
Error.cause
```

***

### name

```ts
name: string;
```

Defined in: ../node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

```ts
Error.name
```

***

### message

```ts
message: string;
```

Defined in: ../node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

```ts
Error.message
```

***

### stack?

```ts
optional stack?: string;
```

Defined in: ../node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

```ts
Error.stack
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
Error.captureStackTrace
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
Error.prepareStackTrace
```

# Enumeration: Vl53l4Cmd

Defined in: [protocol/vl53l4.ts:9](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L9)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
values its methods take and return (the window codes, the range-status
names, `RangeTiming`, `Vl53l4cdError`) and the register-bridge wire codecs
(`protocol/vl53l4`, re-exported under the same `Vl53l4*`-prefixed names
`src/index.ts` uses) — so TypeDoc documents that sensor in isolation. The
ST ULD driver underneath (`VL53L4CD`, the register map) is not public; it
is reachable only through the unstable `uld` escape hatch.

Not part of the shipped package — a doc-generation entry point only.

## Enumeration Members

### ReadReg

```ts
ReadReg: 50;
```

Defined in: [protocol/vl53l4.ts:10](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L10)

***

### WriteReg

```ts
WriteReg: 51;
```

Defined in: [protocol/vl53l4.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L11)

***

### Xshut

```ts
Xshut: 52;
```

Defined in: [protocol/vl53l4.ts:12](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L12)

***

### StartStream

```ts
StartStream: 53;
```

Defined in: [protocol/vl53l4.ts:13](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L13)

***

### StopStream

```ts
StopStream: 54;
```

Defined in: [protocol/vl53l4.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L14)

***

### GetInfo

```ts
GetInfo: 55;
```

Defined in: [protocol/vl53l4.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L15)

***

### SetI2cSpeed

```ts
SetI2cSpeed: 56;
```

Defined in: [protocol/vl53l4.ts:16](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L16)

# Enumeration: Vl53l4Rpt

Defined in: [protocol/vl53l4.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L19)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
values its methods take and return (the window codes, the range-status
names, `RangeTiming`, `Vl53l4cdError`) and the register-bridge wire codecs
(`protocol/vl53l4`, re-exported under the same `Vl53l4*`-prefixed names
`src/index.ts` uses) — so TypeDoc documents that sensor in isolation. The
ST ULD driver underneath (`VL53L4CD`, the register map) is not public; it
is reachable only through the unstable `uld` escape hatch.

Not part of the shipped package — a doc-generation entry point only.

## Enumeration Members

### RegData

```ts
RegData: 145;
```

Defined in: [protocol/vl53l4.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L20)

***

### Info

```ts
Info: 146;
```

Defined in: [protocol/vl53l4.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L21)

***

### Stream

```ts
Stream: 147;
```

Defined in: [protocol/vl53l4.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L22)

# Function: packVl53l4ReadReg()

```ts
function packVl53l4ReadReg(addr, length): Uint8Array;
```

Defined in: [protocol/vl53l4.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L53)

VL53_READ_REG payload `addr u16, len u16` with the transfer limit of
contract 10 §1: `len` 1..253 and `addr + len <= 0x10000`, else RangeError.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `length` | `number` |

## Returns

`Uint8Array`

# Function: packVl53l4SetI2cSpeed()

```ts
function packVl53l4SetI2cSpeed(khz): Uint8Array;
```

Defined in: [protocol/vl53l4.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L92)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
values its methods take and return (the window codes, the range-status
names, `RangeTiming`, `Vl53l4cdError`) and the register-bridge wire codecs
(`protocol/vl53l4`, re-exported under the same `Vl53l4*`-prefixed names
`src/index.ts` uses) — so TypeDoc documents that sensor in isolation. The
ST ULD driver underneath (`VL53L4CD`, the register map) is not public; it
is reachable only through the unstable `uld` escape hatch.

Not part of the shipped package — a doc-generation entry point only.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `khz` | `number` |

## Returns

`Uint8Array`

# Function: packVl53l4StartStream()

```ts
function packVl53l4StartStream(
   addr, 
   length, 
   flags?): Uint8Array;
```

Defined in: [protocol/vl53l4.ts:83](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L83)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
values its methods take and return (the window codes, the range-status
names, `RangeTiming`, `Vl53l4cdError`) and the register-bridge wire codecs
(`protocol/vl53l4`, re-exported under the same `Vl53l4*`-prefixed names
`src/index.ts` uses) — so TypeDoc documents that sensor in isolation. The
ST ULD driver underneath (`VL53L4CD`, the register map) is not public; it
is reachable only through the unstable `uld` escape hatch.

Not part of the shipped package — a doc-generation entry point only.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `addr` | `number` | `undefined` |
| `length` | `number` | `undefined` |
| `flags` | `number` | `0` |

## Returns

`Uint8Array`

# Function: packVl53l4WriteReg()

```ts
function packVl53l4WriteReg(addr, data): Uint8Array;
```

Defined in: [protocol/vl53l4.ts:68](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L68)

VL53_WRITE_REG payload `addr u16, data[N]`: N 1..253 and `addr + N <= 0x10000`, else RangeError.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `data` | `Uint8Array` |

## Returns

`Uint8Array`

# Function: packVl53l4Xshut()

```ts
function packVl53l4Xshut(action): Uint8Array;
```

Defined in: [protocol/vl53l4.ts:79](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L79)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
values its methods take and return (the window codes, the range-status
names, `RangeTiming`, `Vl53l4cdError`) and the register-bridge wire codecs
(`protocol/vl53l4`, re-exported under the same `Vl53l4*`-prefixed names
`src/index.ts` uses) — so TypeDoc documents that sensor in isolation. The
ST ULD driver underneath (`VL53L4CD`, the register map) is not public; it
is reachable only through the unstable `uld` escape hatch.

Not part of the shipped package — a doc-generation entry point only.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `action` | `number` |

## Returns

`Uint8Array`

# Function: rangeStatusText()

```ts
function rangeStatusText(rangeStatus): string;
```

Defined in: [sensors/vl53l4/uld.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L189)

Human-readable text for a decoded range status.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `rangeStatus` | `number` |

## Returns

`string`

# Function: unpackVl53l4Info()

```ts
function unpackVl53l4Info(payload): Vl53l4Info;
```

Defined in: [protocol/vl53l4.ts:145](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L145)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
values its methods take and return (the window codes, the range-status
names, `RangeTiming`, `Vl53l4cdError`) and the register-bridge wire codecs
(`protocol/vl53l4`, re-exported under the same `Vl53l4*`-prefixed names
`src/index.ts` uses) — so TypeDoc documents that sensor in isolation. The
ST ULD driver underneath (`VL53L4CD`, the register map) is not public; it
is reachable only through the unstable `uld` escape hatch.

Not part of the shipped package — a doc-generation entry point only.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Vl53l4Info`](#interface-vl53l4info)

# Function: unpackVl53l4RegData()

```ts
function unpackVl53l4RegData(payload): Vl53l4RegData;
```

Defined in: [protocol/vl53l4.ts:107](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L107)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
values its methods take and return (the window codes, the range-status
names, `RangeTiming`, `Vl53l4cdError`) and the register-bridge wire codecs
(`protocol/vl53l4`, re-exported under the same `Vl53l4*`-prefixed names
`src/index.ts` uses) — so TypeDoc documents that sensor in isolation. The
ST ULD driver underneath (`VL53L4CD`, the register map) is not public; it
is reachable only through the unstable `uld` escape hatch.

Not part of the shipped package — a doc-generation entry point only.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Vl53l4RegData`](#interface-vl53l4regdata)

# Function: unpackVl53l4Stream()

```ts
function unpackVl53l4Stream(payload): Vl53l4StreamData;
```

Defined in: [protocol/vl53l4.ts:173](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L173)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
values its methods take and return (the window codes, the range-status
names, `RangeTiming`, `Vl53l4cdError`) and the register-bridge wire codecs
(`protocol/vl53l4`, re-exported under the same `Vl53l4*`-prefixed names
`src/index.ts` uses) — so TypeDoc documents that sensor in isolation. The
ST ULD driver underneath (`VL53L4CD`, the register map) is not public; it
is reachable only through the unstable `uld` escape hatch.

Not part of the shipped package — a doc-generation entry point only.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Vl53l4StreamData`](#interface-vl53l4streamdata)

# Interface: RangeTiming

Defined in: [sensors/vl53l4/uld.ts:312](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L312)

GetRangeTiming decode result.

## Properties

### timingBudgetMs

```ts
timingBudgetMs: number;
```

Defined in: [sensors/vl53l4/uld.ts:313](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L313)

***

### interMeasurementMs

```ts
interMeasurementMs: number;
```

Defined in: [sensors/vl53l4/uld.ts:314](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L314)

# Interface: Vl53l4Info

Defined in: [protocol/vl53l4.ts:129](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L129)

RPT_VL53_INFO — bridge diagnostics. Counters wrap silently; watch
increments, not absolute values. `i2cErrors` counts since the last
successful XSHUT reset (the bridge clears it there), not since power-up.

## Properties

### intEdges

```ts
intEdges: number;
```

Defined in: [protocol/vl53l4.ts:130](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L130)

***

### slotsSkipped

```ts
slotsSkipped: number;
```

Defined in: [protocol/vl53l4.ts:131](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L131)

***

### i2cErrors

```ts
i2cErrors: number;
```

Defined in: [protocol/vl53l4.ts:132](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L132)

***

### lastI2cError

```ts
lastI2cError: number;
```

Defined in: [protocol/vl53l4.ts:133](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L133)

***

### modelId

```ts
modelId: number;
```

Defined in: [protocol/vl53l4.ts:135](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L135)

0x010F..0x0110 — expected 0xEBAA.

***

### fwStatus

```ts
fwStatus: number;
```

Defined in: [protocol/vl53l4.ts:137](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L137)

0x00E5 — expected 0x03 (booted).

***

### initialized

```ts
initialized: number;
```

Defined in: [protocol/vl53l4.ts:139](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L139)

1 = MODEL_ID read back as 0xEBAA on this GET_INFO.

***

### xshutLevel

```ts
xshutLevel: number;
```

Defined in: [protocol/vl53l4.ts:140](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L140)

***

### intLevel

```ts
intLevel: number;
```

Defined in: [protocol/vl53l4.ts:141](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L141)

***

### i2cKhz

```ts
i2cKhz: number;
```

Defined in: [protocol/vl53l4.ts:142](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L142)

# Interface: Vl53l4Measurement

Defined in: [sensors/vl53l4/vl53l4.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L48)

One decoded ranging result (VL53L4CD_ResultsData_t + MCU timestamp).

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [sensors/vl53l4/vl53l4.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L50)

MCU uptime at the INT edge (stream) / read (poll).

***

### rangeStatus

```ts
rangeStatus: number;
```

Defined in: [sensors/vl53l4/vl53l4.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L52)

0 = valid (RANGE_STATUS_NAMES).

***

### distanceMm

```ts
distanceMm: number;
```

Defined in: [sensors/vl53l4/vl53l4.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L53)

***

### sigmaMm

```ts
sigmaMm: number;
```

Defined in: [sensors/vl53l4/vl53l4.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L54)

***

### signalRateKcps

```ts
signalRateKcps: number;
```

Defined in: [sensors/vl53l4/vl53l4.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L55)

***

### ambientRateKcps

```ts
ambientRateKcps: number;
```

Defined in: [sensors/vl53l4/vl53l4.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L56)

***

### signalPerSpadKcps

```ts
signalPerSpadKcps: number;
```

Defined in: [sensors/vl53l4/vl53l4.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L57)

***

### ambientPerSpadKcps

```ts
ambientPerSpadKcps: number;
```

Defined in: [sensors/vl53l4/vl53l4.ts:58](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L58)

***

### numberOfSpad

```ts
numberOfSpad: number;
```

Defined in: [sensors/vl53l4/vl53l4.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L59)

***

### streamCount

```ts
streamCount: number;
```

Defined in: [sensors/vl53l4/vl53l4.ts:61](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L61)

Sensor frame counter, wraps at 255.

***

### valid

```ts
valid: boolean;
```

Defined in: [sensors/vl53l4/vl53l4.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L63)

rangeStatus === 0.

***

### statusText

```ts
statusText: string;
```

Defined in: [sensors/vl53l4/vl53l4.ts:65](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L65)

Human-readable range status.

# Interface: Vl53l4Options

Defined in: [sensors/vl53l4/vl53l4.ts:85](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L85)

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

Defined in: [sensors/vl53l4/vl53l4.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L87)

ULD sleep implementation (tests inject an instant one).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Interface: Vl53l4RegData

Defined in: [protocol/vl53l4.ts:99](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L99)

RPT_VL53_REG_DATA payload.

## Properties

### cmd

```ts
cmd: number;
```

Defined in: [protocol/vl53l4.ts:101](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L101)

Echoed READ_REG opcode.

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [protocol/vl53l4.ts:103](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L103)

MCU uptime at I2C-read completion.

***

### data

```ts
data: Uint8Array;
```

Defined in: [protocol/vl53l4.ts:104](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L104)

# Interface: Vl53l4StreamData

Defined in: [protocol/vl53l4.ts:165](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L165)

RPT_VL53_STREAM — one streamed register block. `addr`/`length` echo the
stream configuration so each report is self-describing.

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [protocol/vl53l4.ts:167](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L167)

MCU uptime at the INT edge (the sensor event).

***

### addr

```ts
addr: number;
```

Defined in: [protocol/vl53l4.ts:168](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L168)

***

### length

```ts
length: number;
```

Defined in: [protocol/vl53l4.ts:169](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L169)

***

### data

```ts
data: Uint8Array;
```

Defined in: [protocol/vl53l4.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L170)

# Variable: I2C\_KHZ\_BOOT

```ts
const I2C_KHZ_BOOT: 400 = 400;
```

Defined in: [sensors/vl53l4/uld.ts:101](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L101)

# Variable: I2C\_KHZ\_DEFAULT

```ts
const I2C_KHZ_DEFAULT: 1000 = 1000;
```

Defined in: [sensors/vl53l4/uld.ts:102](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L102)

# Variable: MODEL\_ID\_VL53L4CD

```ts
const MODEL_ID_VL53L4CD: 60330 = 0xebaa;
```

Defined in: [sensors/vl53l4/uld.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L56)

# Variable: RANGE\_STATUS\_NAMES

```ts
const RANGE_STATUS_NAMES: Readonly<Record<number, string>>;
```

Defined in: [sensors/vl53l4/uld.ts:113](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L113)

UM2931, "Range status description".

# Variable: VL53L4\_I2C\_ERROR\_NAMES

```ts
const VL53L4_I2C_ERROR_NAMES: Readonly<Record<number, string>>;
```

Defined in: [protocol/vl53l4.ts:117](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L117)

last_i2c_error values in RPT_VL53_INFO.

# Variable: VL53L4\_I2C\_KHZ\_STEPS

```ts
const VL53L4_I2C_KHZ_STEPS: readonly number[];
```

Defined in: [protocol/vl53l4.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L45)

Nominal SCL steps the firmware carries a TIMINGR for (VL53_SET_I2C_SPEED
clamps to the nearest one).

# Variable: VL53L4\_SF\_INT\_ACT\_HIGH

```ts
const VL53L4_SF_INT_ACT_HIGH: 2 = 0x02;
```

Defined in: [protocol/vl53l4.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L39)

VL53_START_STREAM flags: interrupt polarity, mirroring bit 4 of
GPIO_HV_MUX__CTRL (0x0030). Clear (default): INT active low.

# Variable: VL53L4\_XFER\_MAX

```ts
const VL53L4_XFER_MAX: 253 = 253;
```

Defined in: [protocol/vl53l4.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L27)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
values its methods take and return (the window codes, the range-status
names, `RangeTiming`, `Vl53l4cdError`) and the register-bridge wire codecs
(`protocol/vl53l4`, re-exported under the same `Vl53l4*`-prefixed names
`src/index.ts` uses) — so TypeDoc documents that sensor in isolation. The
ST ULD driver underneath (`VL53L4CD`, the register map) is not public; it
is reachable only through the unstable `uld` escape hatch.

Not part of the shipped package — a doc-generation entry point only.

# Variable: VL53L4\_XSHUT\_OFF

```ts
const VL53L4_XSHUT_OFF: 0 = 0;
```

Defined in: [protocol/vl53l4.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L30)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
values its methods take and return (the window codes, the range-status
names, `RangeTiming`, `Vl53l4cdError`) and the register-bridge wire codecs
(`protocol/vl53l4`, re-exported under the same `Vl53l4*`-prefixed names
`src/index.ts` uses) — so TypeDoc documents that sensor in isolation. The
ST ULD driver underneath (`VL53L4CD`, the register map) is not public; it
is reachable only through the unstable `uld` escape hatch.

Not part of the shipped package — a doc-generation entry point only.

# Variable: VL53L4\_XSHUT\_ON

```ts
const VL53L4_XSHUT_ON: 1 = 1;
```

Defined in: [protocol/vl53l4.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L31)

TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.

The VL53L4CD single-zone ToF surface: the `Vl53l4cd` device class, the
values its methods take and return (the window codes, the range-status
names, `RangeTiming`, `Vl53l4cdError`) and the register-bridge wire codecs
(`protocol/vl53l4`, re-exported under the same `Vl53l4*`-prefixed names
`src/index.ts` uses) — so TypeDoc documents that sensor in isolation. The
ST ULD driver underneath (`VL53L4CD`, the register map) is not public; it
is reachable only through the unstable `uld` escape hatch.

Not part of the shipped package — a doc-generation entry point only.

# Variable: VL53L4\_XSHUT\_RESET

```ts
const VL53L4_XSHUT_RESET: 2 = 2;
```

Defined in: [protocol/vl53l4.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L33)

Blocking on the MCU (≤ 20 ms, typ. ~2 ms); answered after the boot handshake.

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

