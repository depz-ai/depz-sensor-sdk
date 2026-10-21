# TMF8829 (dToF) — API reference

The public API for the TMF8829 multizone direct time-of-flight sensor:
the `Tmf8829` device and its measurement / config types, the frame
parser and half-frame merge, the clock model and capture time, presets
and config-page fields, and the register-bridge wire codecs. Discovery,
device-base, transport and other cross-sensor symbols live in the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Enumerations

- [Tmf8829Cmd](enumerations/Tmf8829Cmd.md)
- [Tmf8829Rpt](enumerations/Tmf8829Rpt.md)
- [Tmf8829EventCode](enumerations/Tmf8829EventCode.md)

## Classes

- [Tmf8829FrameReassembler](classes/Tmf8829FrameReassembler.md)
- [Tmf8829MeasurementBuilder](classes/Tmf8829MeasurementBuilder.md)
- [Tmf8829SensorClock](classes/Tmf8829SensorClock.md)
- [Tmf8829CaptureTimer](classes/Tmf8829CaptureTimer.md)
- [Tmf8829Error](classes/Tmf8829Error.md)
- [Tmf8829](classes/Tmf8829.md)

## Interfaces

- [Tmf8829RegData](interfaces/Tmf8829RegData.md)
- [Tmf8829Info](interfaces/Tmf8829Info.md)
- [Tmf8829FrameChunk](interfaces/Tmf8829FrameChunk.md)
- [Tmf8829Event](interfaces/Tmf8829Event.md)
- [Tmf8829RawFrame](interfaces/Tmf8829RawFrame.md)
- [Tmf8829Measurement](interfaces/Tmf8829Measurement.md)
- [Tmf8829FrameTime](interfaces/Tmf8829FrameTime.md)
- [Tmf8829FrameInfo](interfaces/Tmf8829FrameInfo.md)
- [Tmf8829PixelResults](interfaces/Tmf8829PixelResults.md)
- [Tmf8829HistogramFrame](interfaces/Tmf8829HistogramFrame.md)
- [Tmf8829ConfigureOptions](interfaces/Tmf8829ConfigureOptions.md)
- [Tmf8829Config](interfaces/Tmf8829Config.md)
- [Tmf8829Options](interfaces/Tmf8829Options.md)

## Type Aliases

- [Tmf8829Preset](type-aliases/Tmf8829Preset.md)
- [Tmf8829CfgField](type-aliases/Tmf8829CfgField.md)

## Variables

- [TMF8829\_READ\_MAX](variables/TMF8829_READ_MAX.md)
- [TMF8829\_WRITE\_MAX](variables/TMF8829_WRITE_MAX.md)
- [TMF8829\_CHUNK\_MAX](variables/TMF8829_CHUNK_MAX.md)
- [TMF8829\_INT\_RESULTS](variables/TMF8829_INT_RESULTS.md)
- [TMF8829\_INT\_MOTION](variables/TMF8829_INT_MOTION.md)
- [TMF8829\_INT\_PROXIMITY](variables/TMF8829_INT_PROXIMITY.md)
- [TMF8829\_INT\_HISTOGRAMS](variables/TMF8829_INT_HISTOGRAMS.md)
- [TMF8829\_INT\_MASK\_ALL](variables/TMF8829_INT_MASK_ALL.md)
- [TMF8829\_SF\_REF\_FRAME](variables/TMF8829_SF_REF_FRAME.md)
- [TMF8829\_NOMINAL\_Q16](variables/TMF8829_NOMINAL_Q16.md)
- [TMF8829\_FT\_NOMINAL](variables/TMF8829_FT_NOMINAL.md)
- [TMF8829\_FT\_CLOCK\_RESET](variables/TMF8829_FT_CLOCK_RESET.md)
- [TMF8829\_FT\_EDGE\_LATE](variables/TMF8829_FT_EDGE_LATE.md)
- [TMF8829\_FT\_STALLED](variables/TMF8829_FT_STALLED.md)
- [TMF8829\_FT\_T1\_UNSURE](variables/TMF8829_FT_T1_UNSURE.md)
- [TMF8829\_FT\_NAMES](variables/TMF8829_FT_NAMES.md)
- [TMF8829\_REG\_CFG](variables/TMF8829_REG_CFG.md)
- [TMF8829\_CFG\_SIZE](variables/TMF8829_CFG_SIZE.md)
- [TMF8829\_PRESETS](variables/TMF8829_PRESETS.md)
- [TMF8829\_PRESET\_KILO\_ITERATIONS](variables/TMF8829_PRESET_KILO_ITERATIONS.md)
- [TMF8829\_DEFAULT\_PERIOD\_MS](variables/TMF8829_DEFAULT_PERIOD_MS.md)
- [TMF8829\_CFG\_FIELDS](variables/TMF8829_CFG_FIELDS.md)
- [TMF8829\_FIRMWARE\_LICENSE](variables/TMF8829_FIRMWARE_LICENSE.md)
- [TMF8829\_PRE\_HEADER\_SIZE](variables/TMF8829_PRE_HEADER_SIZE.md)
- [TMF8829\_HEADER\_SIZE](variables/TMF8829_HEADER_SIZE.md)
- [TMF8829\_FOOTER\_SIZE](variables/TMF8829_FOOTER_SIZE.md)
- [TMF8829\_FRAME\_HEAD](variables/TMF8829_FRAME_HEAD.md)
- [TMF8829\_FRAME\_EOF](variables/TMF8829_FRAME_EOF.md)
- [TMF8829\_FID\_MASK](variables/TMF8829_FID_MASK.md)
- [TMF8829\_FID\_RESULTS](variables/TMF8829_FID_RESULTS.md)
- [TMF8829\_FID\_HISTOGRAMS](variables/TMF8829_FID_HISTOGRAMS.md)
- [TMF8829\_FID\_REF\_SPAD](variables/TMF8829_FID_REF_SPAD.md)
- [TMF8829\_LAYOUT\_PEAKS](variables/TMF8829_LAYOUT_PEAKS.md)
- [TMF8829\_LAYOUT\_SIGNAL](variables/TMF8829_LAYOUT_SIGNAL.md)
- [TMF8829\_LAYOUT\_NOISE](variables/TMF8829_LAYOUT_NOISE.md)
- [TMF8829\_LAYOUT\_XTALK](variables/TMF8829_LAYOUT_XTALK.md)
- [TMF8829\_LAYOUT\_SUBFRAME](variables/TMF8829_LAYOUT_SUBFRAME.md)
- [TMF8829\_FP\_GRID](variables/TMF8829_FP_GRID.md)
- [TMF8829\_FP\_NAMES](variables/TMF8829_FP_NAMES.md)
- [TMF8829\_FIRMWARE\_LOAD\_ADDR](variables/TMF8829_FIRMWARE_LOAD_ADDR.md)

## Functions

- [packTmf8829ReadReg](functions/packTmf8829ReadReg.md)
- [packTmf8829WriteReg](functions/packTmf8829WriteReg.md)
- [packTmf8829StartStream](functions/packTmf8829StartStream.md)
- [unpackTmf8829RegData](functions/unpackTmf8829RegData.md)
- [unpackTmf8829Info](functions/unpackTmf8829Info.md)
- [unpackTmf8829FrameChunk](functions/unpackTmf8829FrameChunk.md)
- [unpackTmf8829Event](functions/unpackTmf8829Event.md)
- [tmf8829DistanceMm](functions/tmf8829DistanceMm.md)
- [tmf8829RecordValue](functions/tmf8829RecordValue.md)
- [tmf8829FlagNames](functions/tmf8829FlagNames.md)
- [tmf8829CfgGet](functions/tmf8829CfgGet.md)
- [tmf8829CfgSet](functions/tmf8829CfgSet.md)
- [tmf8829ConfigFields](functions/tmf8829ConfigFields.md)
- [tmf8829FrameSize](functions/tmf8829FrameSize.md)
- [parseTmf8829Frame](functions/parseTmf8829Frame.md)
- [tmf8829FrameProblems](functions/tmf8829FrameProblems.md)
- [tmf8829FrameRows](functions/tmf8829FrameRows.md)
- [parseTmf8829Results](functions/parseTmf8829Results.md)
- [mergeTmf8829Halves](functions/mergeTmf8829Halves.md)
- [tmf8829FirmwareImage](functions/tmf8829FirmwareImage.md)

# Class: Tmf8829

Defined in: [src/sensors/tmf8829/tmf8829.ts:160](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L160)

TMF8829 ToF device (8×8 … 48×32 zones, up to 4 peaks per zone, histograms).
`init()` downloads the 14.4 KB sensor firmware (~0.15 s), `configure(preset,
…)` writes the config page, `start()` streams measurements to
`onMeasurement` / `measurements()`, `stop()` ends it. The sensor's EN pin is
wired high: there is no sleep, recovery from any state is `init()`.

## Extends

- `DepzDevice`

## Constructors

### Constructor

```ts
new Tmf8829(transport, opts?): Tmf8829;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L174)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | [`Tmf8829Options`](../interfaces/Tmf8829Options.md) |

#### Returns

`Tmf8829`

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

### appVersion

```ts
appVersion: string | null = null;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:161](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L161)

***

### sensorSerial

```ts
sensorSerial: number | null = null;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:162](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L162)

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

### config

#### Get Signature

```ts
get config(): Tmf8829Config | null;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:325](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L325)

The last `configure()` result (null after `init()`).

##### Returns

[`Tmf8829Config`](../interfaces/Tmf8829Config.md) \| `null`

***

### streaming

#### Get Signature

```ts
get streaming(): boolean;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:331](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L331)

##### Returns

`boolean`

***

### framesDropped

#### Get Signature

```ts
get framesDropped(): number;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:441](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L441)

Frames the bridge dropped (frame_seq gaps) in this stream.

##### Returns

`number`

***

### framesDiscarded

#### Get Signature

```ts
get framesDiscarded(): number;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:446](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L446)

Frames lost to reassembly (incomplete, out-of-order chunks) in this stream.

##### Returns

`number`

***

### frameParseErrors

#### Get Signature

```ts
get frameParseErrors(): number;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:450](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L450)

##### Returns

`number`

***

### clockRatioQ16

#### Get Signature

```ts
get clockRatioQ16(): number;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:455](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L455)

Current clock ratio (Q16), for diagnostics.

##### Returns

`number`

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

### readRegisters()

```ts
readRegisters(addr, length): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:185](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L185)

Read `length` bytes from `addr` in ≤1024-byte requests; reads that reach the
FIFO port 0xFF stay there (the sensor does not auto-increment it).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `length` | `number` |

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### writeRegisters()

```ts
writeRegisters(addr, data): Promise<void>;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:190](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L190)

Write `data` from `addr` in ≤1023-byte requests (writes to 0xFF stay at 0xFF).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `data` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

***

### sensorState()

```ts
sensorState(): Promise<Record<string, number>>;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:195](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L195)

Raw host-interface registers, for diagnostics.

#### Returns

`Promise`\<`Record`\<`string`, `number`\>\>

***

### getBridgeInfo()

```ts
getBridgeInfo(): Promise<Tmf8829Info>;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:215](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L215)

Bridge counters (never touches the sensor, safe mid-stream).

#### Returns

`Promise`\<[`Tmf8829Info`](../interfaces/Tmf8829Info.md)\>

***

### init()

```ts
init(progress?): Promise<[string, number]>;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:229](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L229)

Bring the sensor from any state to its running RAM application: soft
reset, bootloader, firmware download, start. Resolves [application
version, sensor serial]. Stops a running stream first. `progress(done,
total)` tracks the image download.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `progress?` | (`done`, `total`) => `void` |

#### Returns

`Promise`\<\[`string`, `number`\]\>

***

### configPage()

```ts
configPage(): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:269](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L269)

LOAD_CONFIG_PAGE → the 190-byte page 0x22..0xDF now in the sensor.

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### configure()

```ts
configure(preset?, opts?): Promise<Tmf8829Config>;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:279](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L279)

Load `preset` and write the config page. Every field a preset does not
reset is written (contract 14 §4), so the result never depends on an
earlier config. Rejects with `Tmf8829Error` when the sensor refuses the
page (e.g. ERR_CONFIG_RESULT_SIZE) or a written field does not read back.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `preset` | [`Tmf8829Preset`](../type-aliases/Tmf8829Preset.md) | `"8x8"` |
| `opts` | [`Tmf8829ConfigureOptions`](../interfaces/Tmf8829ConfigureOptions.md) | `{}` |

#### Returns

`Promise`\<[`Tmf8829Config`](../interfaces/Tmf8829Config.md)\>

***

### start()

```ts
start(): Promise<void>;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:336](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L336)

MEASURE with the bridge stream armed; configures the 8x8 preset first if needed.

#### Returns

`Promise`\<`void`\>

***

### stop()

```ts
stop(): Promise<void>;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:367](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L367)

Sensor STOP, then the bridge stream (frames already queued drain). The
bad-header event a STOP raises mid histogram dump is expected and
swallowed.

#### Returns

`Promise`\<`void`\>

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:382](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L382)

#### Returns

`Promise`\<`void`\>

#### Overrides

```ts
DepzDevice.close
```

***

### onMeasurement()

```ts
onMeasurement(cb): () => void;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:388](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L388)

Subscribe to measurements (read-pump context; don't block).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`m`) => `void` |

#### Returns

() => `void`

***

### measurements()

```ts
measurements(maxsize?): StreamQueue<Tmf8829Measurement>;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:396](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L396)

Async iterator over measurements — bounded, drop-oldest.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `maxsize` | `number` | `16` |

#### Returns

`StreamQueue`\<[`Tmf8829Measurement`](../interfaces/Tmf8829Measurement.md)\>

***

### getMeasurement()

```ts
getMeasurement(timeoutMs?): Promise<Tmf8829Measurement>;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:407](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L407)

Wait for the next measurement.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `2000` |

#### Returns

`Promise`\<[`Tmf8829Measurement`](../interfaces/Tmf8829Measurement.md)\>

***

### onInterrupt()

```ts
onInterrupt(cb): () => void;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:433](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L433)

RPT_TMF_EVENT that is not an expected stop artefact: a MOTION/PROXIMITY
interrupt (code 0) or a refused frame header (code 1 — the sensor is lost,
run `init()` again).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`e`) => `void` |

#### Returns

() => `void`

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:537](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L537)

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

# Class: Tmf8829CaptureTimer

Defined in: [src/sensors/tmf8829/clock.ts:142](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L142)

When a results frame was captured: the centre of [t0, t1 + T1], T1 the
stream's shortest (t1 − t0)/(N − 1).

## Constructors

### Constructor

```ts
new Tmf8829CaptureTimer(clock): Tmf8829CaptureTimer;
```

Defined in: [src/sensors/tmf8829/clock.ts:147](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L147)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `clock` | [`Tmf8829SensorClock`](Tmf8829SensorClock.md) |

#### Returns

`Tmf8829CaptureTimer`

## Properties

### clock

```ts
readonly clock: Tmf8829SensorClock;
```

Defined in: [src/sensors/tmf8829/clock.ts:147](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L147)

## Methods

### reset()

```ts
reset(): void;
```

Defined in: [src/sensors/tmf8829/clock.ts:149](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L149)

#### Returns

`void`

***

### histograms()

```ts
histograms(): void;
```

Defined in: [src/sensors/tmf8829/clock.ts:156](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L156)

A histogram frame was seen in the stream.

#### Returns

`void`

***

### results()

```ts
results(
   fpMode, 
   systick, 
   t0Tick, 
   t1Tick, 
   pairUs, 
   edgeUs, 
   flagsIn): Tmf8829FrameTime;
```

Defined in: [src/sensors/tmf8829/clock.ts:160](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L160)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `fpMode` | `number` |
| `systick` | `number` |
| `t0Tick` | `number` |
| `t1Tick` | `number` |
| `pairUs` | `number` |
| `edgeUs` | `number` |
| `flagsIn` | `number` |

#### Returns

[`Tmf8829FrameTime`](../interfaces/Tmf8829FrameTime.md)

# Class: Tmf8829Error

Defined in: [src/sensors/tmf8829/tmf8829.ts:120](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L120)

The sensor refused or did not answer (bootloader / application status).

## Extends

- `DepzError`

## Constructors

### Constructor

```ts
new Tmf8829Error(message?): Tmf8829Error;
```

Defined in: [src/errors.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L8)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `message?` | `string` |

#### Returns

`Tmf8829Error`

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

# Class: Tmf8829FrameReassembler

Defined in: [src/protocol/tmf8829.ts:184](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L184)

Rebuilds frames from RPT_TMF_FRAME chunks (contract 14 §2): reset on offset
0; append only a chunk of the same frame_seq at the expected offset;
complete at full_size. Never stitches chunks of different frame_seq.
`seqGaps` counts frames the bridge dropped (frame_seq jumps).

## Constructors

### Constructor

```ts
new Tmf8829FrameReassembler(): Tmf8829FrameReassembler;
```

#### Returns

`Tmf8829FrameReassembler`

## Properties

### completed

```ts
completed: number = 0;
```

Defined in: [src/protocol/tmf8829.ts:185](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L185)

***

### discarded

```ts
discarded: number = 0;
```

Defined in: [src/protocol/tmf8829.ts:186](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L186)

***

### seqGaps

```ts
seqGaps: number = 0;
```

Defined in: [src/protocol/tmf8829.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L187)

## Methods

### reset()

```ts
reset(): void;
```

Defined in: [src/protocol/tmf8829.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L194)

Forget everything — call at every stream start (frame_seq restarts at 0).

#### Returns

`void`

***

### feed()

```ts
feed(chunk): Tmf8829RawFrame | null;
```

Defined in: [src/protocol/tmf8829.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L210)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `chunk` | [`Tmf8829FrameChunk`](../interfaces/Tmf8829FrameChunk.md) |

#### Returns

[`Tmf8829RawFrame`](../interfaces/Tmf8829RawFrame.md) \| `null`

# Class: Tmf8829MeasurementBuilder

Defined in: [src/sensors/tmf8829/builder.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L72)

Histogram frames between two results frames belong to the second; a
ref-SPAD frame comes right before its results frame. Call `reset()` at
every stream start: the clock model and T1 are per stream.

## Constructors

### Constructor

```ts
new Tmf8829MeasurementBuilder(): Tmf8829MeasurementBuilder;
```

#### Returns

`Tmf8829MeasurementBuilder`

## Properties

### clock

```ts
readonly clock: Tmf8829SensorClock;
```

Defined in: [src/sensors/tmf8829/builder.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L73)

***

### timer

```ts
readonly timer: Tmf8829CaptureTimer;
```

Defined in: [src/sensors/tmf8829/builder.ts:74](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L74)

***

### parseErrors

```ts
parseErrors: number = 0;
```

Defined in: [src/sensors/tmf8829/builder.ts:75](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L75)

## Methods

### reset()

```ts
reset(): void;
```

Defined in: [src/sensors/tmf8829/builder.ts:82](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L82)

#### Returns

`void`

***

### add()

```ts
add(
   rawIn, 
   timestampUs, 
   edgeUs?): Tmf8829Measurement | null;
```

Defined in: [src/sensors/tmf8829/builder.ts:97](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L97)

One frame with the MCU time of its header read and its INT edge (0 =
none) → the measurement it completes, or null. `raw` is copied.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `rawIn` | `Uint8Array` | `undefined` |
| `timestampUs` | `number` | `undefined` |
| `edgeUs` | `number` | `0` |

#### Returns

[`Tmf8829Measurement`](../interfaces/Tmf8829Measurement.md) \| `null`

# Class: Tmf8829SensorClock

Defined in: [src/sensors/tmf8829/clock.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L56)

Sensor SYSTICK (125 kHz) → MCU µs, fed one clock pair per frame. The ratio
is taken over a sliding 1..2 s window, first after 200 ms; it is also the
distance correction.

## Constructors

### Constructor

```ts
new Tmf8829SensorClock(): Tmf8829SensorClock;
```

#### Returns

`Tmf8829SensorClock`

## Properties

### ratioQ16

```ts
ratioQ16: number = TMF8829_NOMINAL_Q16;
```

Defined in: [src/sensors/tmf8829/clock.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L57)

***

### nominal

```ts
nominal: boolean = true;
```

Defined in: [src/sensors/tmf8829/clock.ts:58](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L58)

## Accessors

### ratio

#### Get Signature

```ts
get ratio(): number;
```

Defined in: [src/sensors/tmf8829/clock.ts:74](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L74)

MCU/sensor clock ratio (≈ 1.015): distances are scaled by it.

##### Returns

`number`

## Methods

### reset()

```ts
reset(): void;
```

Defined in: [src/sensors/tmf8829/clock.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L64)

#### Returns

`void`

***

### add()

```ts
add(us, tick): number;
```

Defined in: [src/sensors/tmf8829/clock.ts:85](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L85)

Add a pair → TMF8829_FT_CLOCK_RESET if it does not fit the model, else 0.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `us` | `number` |
| `tick` | `number` |

#### Returns

`number`

***

### toUs()

```ts
toUs(
   pairUs, 
   pairTick, 
   tick): number;
```

Defined in: [src/sensors/tmf8829/clock.ts:115](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L115)

SYSTICK `tick` → MCU µs through the pair (pairUs, pairTick).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pairUs` | `number` |
| `pairTick` | `number` |
| `tick` | `number` |

#### Returns

`number`

# Enumeration: Tmf8829Cmd

Defined in: [src/protocol/tmf8829.ts:9](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L9)

TMF8829 SPI register-bridge wire codecs (contracts/14_SENSOR_TMF8829.md §2).
Mirrors the Python reference `depz_sensor_sdk.protocol.tmf8829`.

Same command numbering as the VL53 bridges (0x32/0x33/0x35/0x36/0x37) but
TMF8829 payloads: an 8-bit register address and self-sized frames.

## Enumeration Members

### ReadReg

```ts
ReadReg: 50;
```

Defined in: [src/protocol/tmf8829.ts:10](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L10)

***

### WriteReg

```ts
WriteReg: 51;
```

Defined in: [src/protocol/tmf8829.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L11)

***

### StartStream

```ts
StartStream: 53;
```

Defined in: [src/protocol/tmf8829.ts:12](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L12)

***

### StopStream

```ts
StopStream: 54;
```

Defined in: [src/protocol/tmf8829.ts:13](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L13)

***

### GetInfo

```ts
GetInfo: 55;
```

Defined in: [src/protocol/tmf8829.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L14)

# Enumeration: Tmf8829EventCode

Defined in: [src/protocol/tmf8829.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L25)

RPT_TMF_EVENT.code

## Enumeration Members

### Int

```ts
Int: 0;
```

Defined in: [src/protocol/tmf8829.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L27)

MOTION/PROXIMITY interrupt, no frame; its bits already cleared.

***

### BadHeader

```ts
BadHeader: 1;
```

Defined in: [src/protocol/tmf8829.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L29)

Frame header refused, frame not read (expected once on STOP mid histogram dump).

# Enumeration: Tmf8829Rpt

Defined in: [src/protocol/tmf8829.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L17)

## Enumeration Members

### RegData

```ts
RegData: 145;
```

Defined in: [src/protocol/tmf8829.ts:18](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L18)

***

### Info

```ts
Info: 146;
```

Defined in: [src/protocol/tmf8829.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L19)

***

### Frame

```ts
Frame: 147;
```

Defined in: [src/protocol/tmf8829.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L20)

***

### Event

```ts
Event: 148;
```

Defined in: [src/protocol/tmf8829.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L21)

# Function: mergeTmf8829Halves()

```ts
function mergeTmf8829Halves(first, second): Tmf8829PixelResults;
```

Defined in: [src/sensors/tmf8829/frames.ts:211](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L211)

32x32/48x32: the first half-frame holds the even rows, the second the odd ones.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `first` | [`Tmf8829PixelResults`](../interfaces/Tmf8829PixelResults.md) |
| `second` | [`Tmf8829PixelResults`](../interfaces/Tmf8829PixelResults.md) |

## Returns

[`Tmf8829PixelResults`](../interfaces/Tmf8829PixelResults.md)

# Function: packTmf8829ReadReg()

```ts
function packTmf8829ReadReg(addr, length): Uint8Array;
```

Defined in: [src/protocol/tmf8829.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L44)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `length` | `number` |

## Returns

`Uint8Array`

# Function: packTmf8829StartStream()

```ts
function packTmf8829StartStream(intMask, flags?): Uint8Array;
```

Defined in: [src/protocol/tmf8829.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L63)

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `intMask` | `number` | `undefined` |
| `flags` | `number` | `0` |

## Returns

`Uint8Array`

# Function: packTmf8829WriteReg()

```ts
function packTmf8829WriteReg(addr, data): Uint8Array;
```

Defined in: [src/protocol/tmf8829.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L52)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `data` | `Uint8Array` |

## Returns

`Uint8Array`

# Function: parseTmf8829Frame()

```ts
function parseTmf8829Frame(raw): Tmf8829FrameInfo;
```

Defined in: [src/sensors/tmf8829/frames.ts:85](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L85)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `raw` | `Uint8Array` |

## Returns

[`Tmf8829FrameInfo`](../interfaces/Tmf8829FrameInfo.md)

# Function: parseTmf8829Results()

```ts
function parseTmf8829Results(raw, info?): Tmf8829PixelResults;
```

Defined in: [src/sensors/tmf8829/frames.ts:157](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L157)

Pixel results of one results frame.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `raw` | `Uint8Array` |
| `info?` | [`Tmf8829FrameInfo`](../interfaces/Tmf8829FrameInfo.md) |

## Returns

[`Tmf8829PixelResults`](../interfaces/Tmf8829PixelResults.md)

# Function: tmf8829CfgGet()

```ts
function tmf8829CfgGet(page, name): number;
```

Defined in: [src/sensors/tmf8829/config.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L87)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `page` | `Uint8Array` |
| `name` | [`Tmf8829CfgField`](../type-aliases/Tmf8829CfgField.md) |

## Returns

`number`

# Function: tmf8829CfgSet()

```ts
function tmf8829CfgSet(
   page, 
   name, 
   value): void;
```

Defined in: [src/sensors/tmf8829/config.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L94)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `page` | `Uint8Array` |
| `name` | [`Tmf8829CfgField`](../type-aliases/Tmf8829CfgField.md) |
| `value` | `number` |

## Returns

`void`

# Function: tmf8829ConfigFields()

```ts
function tmf8829ConfigFields(page): Record<Tmf8829CfgField, number>;
```

Defined in: [src/sensors/tmf8829/config.ts:110](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L110)

Every named field of a config page.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `page` | `Uint8Array` |

## Returns

`Record`\<[`Tmf8829CfgField`](../type-aliases/Tmf8829CfgField.md), `number`\>

# Function: tmf8829DistanceMm()

```ts
function tmf8829DistanceMm(
   results, 
   ratio, 
   inMm): Float64Array<ArrayBufferLike> | null;
```

Defined in: [src/sensors/tmf8829/builder.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L53)

Raw distances → mm, clock-corrected (×ratio; ÷4 for 0.25 mm units).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `results` | [`Tmf8829PixelResults`](../interfaces/Tmf8829PixelResults.md) |
| `ratio` | `number` |
| `inMm` | `boolean` |

## Returns

`Float64Array`\<`ArrayBufferLike`\> \| `null`

# Function: tmf8829FirmwareImage()

```ts
function tmf8829FirmwareImage(): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:112](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L112)

The sensor firmware image `init()` downloads (14 444 B, MIT). Lazily decoded.

## Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

# Function: tmf8829FlagNames()

```ts
function tmf8829FlagNames(flags): string[];
```

Defined in: [src/sensors/tmf8829/clock.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L35)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `flags` | `number` |

## Returns

`string`[]

# Function: tmf8829FrameProblems()

```ts
function tmf8829FrameProblems(f): string[];
```

Defined in: [src/sensors/tmf8829/frames.ts:119](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L119)

What is wrong with a frame, human-readable; empty when fine.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `f` | [`Tmf8829FrameInfo`](../interfaces/Tmf8829FrameInfo.md) |

## Returns

`string`[]

# Function: tmf8829FrameRows()

```ts
function tmf8829FrameRows(fpMode): [number, number];
```

Defined in: [src/sensors/tmf8829/frames.ts:150](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L150)

[rows, cols] one results frame carries: half the rows for 32x32/48x32.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `fpMode` | `number` |

## Returns

\[`number`, `number`\]

# Function: tmf8829FrameSize()

```ts
function tmf8829FrameSize(head): number;
```

Defined in: [src/sensors/tmf8829/frames.ts:81](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L81)

Full frame size (pre-header included) from its first 21 bytes.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `head` | `Uint8Array` |

## Returns

`number`

# Function: tmf8829RecordValue()

```ts
function tmf8829RecordValue(m): Record<string, unknown>;
```

Defined in: [src/sensors/tmf8829/builder.ts:206](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L206)

Contract 09 `tmf8829` record value of a measurement (distances rounded to 0.1 mm).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `m` | [`Tmf8829Measurement`](../interfaces/Tmf8829Measurement.md) |

## Returns

`Record`\<`string`, `unknown`\>

# Function: unpackTmf8829Event()

```ts
function unpackTmf8829Event(p): Tmf8829Event;
```

Defined in: [src/protocol/tmf8829.ts:159](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L159)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | `Uint8Array` |

## Returns

[`Tmf8829Event`](../interfaces/Tmf8829Event.md)

# Function: unpackTmf8829FrameChunk()

```ts
function unpackTmf8829FrameChunk(p): Tmf8829FrameChunk;
```

Defined in: [src/protocol/tmf8829.ts:135](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L135)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | `Uint8Array` |

## Returns

[`Tmf8829FrameChunk`](../interfaces/Tmf8829FrameChunk.md)

# Function: unpackTmf8829Info()

```ts
function unpackTmf8829Info(p): Tmf8829Info;
```

Defined in: [src/protocol/tmf8829.ts:104](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L104)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | `Uint8Array` |

## Returns

[`Tmf8829Info`](../interfaces/Tmf8829Info.md)

# Function: unpackTmf8829RegData()

```ts
function unpackTmf8829RegData(p): Tmf8829RegData;
```

Defined in: [src/protocol/tmf8829.ts:81](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L81)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | `Uint8Array` |

## Returns

[`Tmf8829RegData`](../interfaces/Tmf8829RegData.md)

# Interface: Tmf8829Config

Defined in: [src/sensors/tmf8829/tmf8829.ts:137](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L137)

The config written by `configure()`, as read back from the sensor.

## Properties

### preset

```ts
preset: Tmf8829Preset;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:138](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L138)

***

### page

```ts
page: Uint8Array;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:140](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L140)

The 190-byte page 0x22..0xDF.

***

### histograms

```ts
histograms: boolean;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:141](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L141)

***

### refFrame

```ts
refFrame: boolean;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:142](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L142)

***

### fields

```ts
fields: Record<Tmf8829CfgField, number>;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:143](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L143)

***

### grid

```ts
grid: readonly [number, number];
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:145](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L145)

[rows, cols] of a measurement.

# Interface: Tmf8829ConfigureOptions

Defined in: [src/sensors/tmf8829/tmf8829.ts:122](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L122)

## Properties

### periodMs?

```ts
optional periodMs?: number;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:123](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L123)

***

### kiloIterations?

```ts
optional kiloIterations?: number;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:125](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L125)

Defaults to the preset's own value.

***

### nrPeaks?

```ts
optional nrPeaks?: number;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:126](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L126)

***

### signalStrength?

```ts
optional signalStrength?: boolean;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:127](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L127)

***

### noiseStrength?

```ts
optional noiseStrength?: boolean;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:128](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L128)

***

### xtalk?

```ts
optional xtalk?: boolean;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:129](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L129)

***

### histograms?

```ts
optional histograms?: boolean;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:130](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L130)

***

### refFrame?

```ts
optional refFrame?: boolean;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:131](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L131)

***

### fields?

```ts
optional fields?: Partial<Record<Tmf8829CfgField, number>>;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:133](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L133)

Any other config-page field by name.

# Interface: Tmf8829Event

Defined in: [src/protocol/tmf8829.ts:149](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L149)

RPT_TMF_EVENT. `intStatus` is INT_STATUS as read (unmasked).

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/protocol/tmf8829.ts:150](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L150)

***

### intStatus

```ts
intStatus: number;
```

Defined in: [src/protocol/tmf8829.ts:151](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L151)

***

### code

```ts
code: number;
```

Defined in: [src/protocol/tmf8829.ts:152](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L152)

***

### frameId

```ts
frameId: number;
```

Defined in: [src/protocol/tmf8829.ts:154](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L154)

Header id byte (BAD_HEADER), else 0.

***

### fifoStatus

```ts
fifoStatus: number;
```

Defined in: [src/protocol/tmf8829.ts:156](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L156)

Pre-header byte (BAD_HEADER), else 0.

# Interface: Tmf8829FrameChunk

Defined in: [src/protocol/tmf8829.ts:126](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L126)

One RPT_TMF_FRAME chunk. `timestampUs` + the frame's pre-header SYSTICK are
a clock pair (not the capture time); `edgeUs` is the INT edge credited to
the frame, 0 = none.

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/protocol/tmf8829.ts:127](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L127)

***

### fullSize

```ts
fullSize: number;
```

Defined in: [src/protocol/tmf8829.ts:128](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L128)

***

### offset

```ts
offset: number;
```

Defined in: [src/protocol/tmf8829.ts:129](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L129)

***

### frameSeq

```ts
frameSeq: number;
```

Defined in: [src/protocol/tmf8829.ts:130](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L130)

***

### edgeUs

```ts
edgeUs: bigint;
```

Defined in: [src/protocol/tmf8829.ts:131](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L131)

***

### data

```ts
data: Uint8Array;
```

Defined in: [src/protocol/tmf8829.ts:132](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L132)

# Interface: Tmf8829FrameInfo

Defined in: [src/sensors/tmf8829/frames.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L57)

Pre-header, header and footer of one frame (any kind).

## Properties

### fifoStatus

```ts
fifoStatus: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:58](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L58)

***

### systick

```ts
systick: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L59)

***

### id

```ts
id: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L60)

***

### kind

```ts
kind: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:62](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L62)

TMF8829_FID_RESULTS / _HISTOGRAMS / _REF_SPAD

***

### fpMode

```ts
fpMode: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L63)

***

### layout

```ts
layout: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L64)

***

### payload

```ts
payload: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:65](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L65)

***

### fNumber

```ts
fNumber: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:66](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L66)

***

### temperature

```ts
temperature: [number, number, number];
```

Defined in: [src/sensors/tmf8829/frames.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L67)

***

### bdv

```ts
bdv: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:68](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L68)

***

### refPos

```ts
refPos: [number, number];
```

Defined in: [src/sensors/tmf8829/frames.ts:69](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L69)

***

### t0Integration

```ts
t0Integration: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:70](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L70)

***

### t1Integration

```ts
t1Integration: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L71)

***

### frameStatus

```ts
frameStatus: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L72)

***

### reserved

```ts
reserved: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L73)

***

### eof

```ts
eof: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:74](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L74)

***

### size

```ts
size: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:75](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L75)

***

### isSecondHalf

```ts
isSecondHalf: boolean;
```

Defined in: [src/sensors/tmf8829/frames.ts:76](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L76)

***

### distanceInMm

```ts
distanceInMm: boolean;
```

Defined in: [src/sensors/tmf8829/frames.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L77)

# Interface: Tmf8829FrameTime

Defined in: [src/sensors/tmf8829/clock.ts:121](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L121)

Capture time of one results frame, MCU µs.

## Properties

### timestampUs

```ts
timestampUs: number;
```

Defined in: [src/sensors/tmf8829/clock.ts:123](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L123)

Centre of the capture window.

***

### flags

```ts
flags: number;
```

Defined in: [src/sensors/tmf8829/clock.ts:125](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L125)

TMF8829_FT_*

***

### t1LenUs

```ts
t1LenUs: number;
```

Defined in: [src/sensors/tmf8829/clock.ts:127](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L127)

Sub-integration length used.

***

### t0Us

```ts
t0Us: number;
```

Defined in: [src/sensors/tmf8829/clock.ts:128](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L128)

***

### t1Us

```ts
t1Us: number;
```

Defined in: [src/sensors/tmf8829/clock.ts:129](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L129)

***

### edgeUs

```ts
edgeUs: number;
```

Defined in: [src/sensors/tmf8829/clock.ts:131](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L131)

INT edge credited to the frame, 0 = none.

***

### pairUs

```ts
pairUs: number;
```

Defined in: [src/sensors/tmf8829/clock.ts:133](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L133)

MCU time of the header read (the clock pair).

***

### ratioQ16

```ts
ratioQ16: number;
```

Defined in: [src/sensors/tmf8829/clock.ts:135](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L135)

Clock ratio after this frame.

# Interface: Tmf8829HistogramFrame

Defined in: [src/sensors/tmf8829/frames.ts:242](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L242)

A raw histogram frame (25 377 B); decoding the bins is left to the caller.

## Properties

### info

```ts
info: Tmf8829FrameInfo;
```

Defined in: [src/sensors/tmf8829/frames.ts:243](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L243)

***

### raw

```ts
raw: Uint8Array;
```

Defined in: [src/sensors/tmf8829/frames.ts:244](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L244)

# Interface: Tmf8829Info

Defined in: [src/protocol/tmf8829.ts:90](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L90)

RPT_TMF_INFO: bridge state only (no echoed command byte).

## Properties

### intEdges

```ts
intEdges: number;
```

Defined in: [src/protocol/tmf8829.ts:91](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L91)

***

### frames

```ts
frames: number;
```

Defined in: [src/protocol/tmf8829.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L92)

***

### framesAborted

```ts
framesAborted: number;
```

Defined in: [src/protocol/tmf8829.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L93)

***

### frameErrors

```ts
frameErrors: number;
```

Defined in: [src/protocol/tmf8829.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L94)

***

### spiTimeouts

```ts
spiTimeouts: number;
```

Defined in: [src/protocol/tmf8829.ts:95](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L95)

***

### intLevel

```ts
intLevel: number;
```

Defined in: [src/protocol/tmf8829.ts:97](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L97)

PA4 level now, 0 = asserted.

***

### streaming

```ts
streaming: boolean;
```

Defined in: [src/protocol/tmf8829.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L98)

***

### intMask

```ts
intMask: number;
```

Defined in: [src/protocol/tmf8829.ts:99](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L99)

***

### lastIntStatus

```ts
lastIntStatus: number;
```

Defined in: [src/protocol/tmf8829.ts:100](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L100)

***

### lastFifoStatus

```ts
lastFifoStatus: number;
```

Defined in: [src/protocol/tmf8829.ts:101](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L101)

# Interface: Tmf8829Measurement

Defined in: [src/sensors/tmf8829/builder.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L26)

One measurement: its results frame (8x8, 16x16) or both half-frames (32x32,
48x32) merged, with the histogram / ref-SPAD frames before it.

## Properties

### timestampUs

```ts
timestampUs: number;
```

Defined in: [src/sensors/tmf8829/builder.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L28)

Capture time, MCU µs (floor mean of the half-frame captures).

***

### flags

```ts
flags: number;
```

Defined in: [src/sensors/tmf8829/builder.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L30)

TMF8829_FT_* of all its results frames.

***

### fpMode

```ts
fpMode: number;
```

Defined in: [src/sensors/tmf8829/builder.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L31)

***

### fNumber

```ts
fNumber: number;
```

Defined in: [src/sensors/tmf8829/builder.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L32)

***

### rows

```ts
rows: number;
```

Defined in: [src/sensors/tmf8829/builder.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L33)

***

### cols

```ts
cols: number;
```

Defined in: [src/sensors/tmf8829/builder.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L34)

***

### results

```ts
results: Tmf8829PixelResults;
```

Defined in: [src/sensors/tmf8829/builder.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L36)

Raw sensor integers.

***

### distanceMm

```ts
distanceMm: Float64Array<ArrayBufferLike> | null;
```

Defined in: [src/sensors/tmf8829/builder.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L38)

Clock-corrected mm, flat [peak][row][col] (null without peaks).

***

### ratioQ16

```ts
ratioQ16: number;
```

Defined in: [src/sensors/tmf8829/builder.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L40)

Clock ratio the distances were corrected with (Q16).

***

### frames

```ts
frames: Tmf8829FrameInfo[];
```

Defined in: [src/sensors/tmf8829/builder.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L41)

***

### times

```ts
times: Tmf8829FrameTime[];
```

Defined in: [src/sensors/tmf8829/builder.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L42)

***

### histograms

```ts
histograms: Tmf8829HistogramFrame[];
```

Defined in: [src/sensors/tmf8829/builder.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L43)

***

### refFrames

```ts
refFrames: Uint8Array<ArrayBufferLike>[];
```

Defined in: [src/sensors/tmf8829/builder.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L45)

65 B each, zero pre-header.

***

### problems

```ts
problems: string[];
```

Defined in: [src/sensors/tmf8829/builder.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L47)

What went wrong since the previous measurement, human-readable.

***

### temperatureC

```ts
temperatureC: number;
```

Defined in: [src/sensors/tmf8829/builder.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L49)

Sensor die temperature (header byte 0 of the last frame), °C.

# Interface: Tmf8829Options

Defined in: [src/sensors/tmf8829/tmf8829.ts:148](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L148)

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

Defined in: [src/sensors/tmf8829/tmf8829.ts:150](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L150)

Injectable sleep (tests).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Interface: Tmf8829PixelResults

Defined in: [src/sensors/tmf8829/frames.ts:138](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L138)

Decoded results of one frame or of a merged measurement. Per-peak arrays
are flat `[peak][row][col]` (row-major); noise / xtalk `[row][col]`. Raw
sensor integers: distance in 0.25 mm (or mm with the footer flag), not yet
clock-corrected. Absent fields are null.

## Properties

### peaks

```ts
peaks: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:139](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L139)

***

### rows

```ts
rows: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:140](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L140)

***

### cols

```ts
cols: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:141](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L141)

***

### distance

```ts
distance: Uint16Array<ArrayBufferLike> | null;
```

Defined in: [src/sensors/tmf8829/frames.ts:142](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L142)

***

### snr

```ts
snr: Uint8Array<ArrayBufferLike> | null;
```

Defined in: [src/sensors/tmf8829/frames.ts:143](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L143)

***

### signal

```ts
signal: Uint16Array<ArrayBufferLike> | null;
```

Defined in: [src/sensors/tmf8829/frames.ts:144](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L144)

***

### noise

```ts
noise: Uint16Array<ArrayBufferLike> | null;
```

Defined in: [src/sensors/tmf8829/frames.ts:145](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L145)

***

### xtalk

```ts
xtalk: Uint16Array<ArrayBufferLike> | null;
```

Defined in: [src/sensors/tmf8829/frames.ts:146](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L146)

# Interface: Tmf8829RawFrame

Defined in: [src/protocol/tmf8829.ts:171](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L171)

A reassembled frame: the sensor bytes from register 0xFA on.

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/protocol/tmf8829.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L172)

***

### edgeUs

```ts
edgeUs: bigint;
```

Defined in: [src/protocol/tmf8829.ts:173](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L173)

***

### frameSeq

```ts
frameSeq: number;
```

Defined in: [src/protocol/tmf8829.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L174)

***

### data

```ts
data: Uint8Array;
```

Defined in: [src/protocol/tmf8829.ts:175](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L175)

# Interface: Tmf8829RegData

Defined in: [src/protocol/tmf8829.ts:76](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L76)

RPT_TMF_REG_DATA: register bytes and the MCU time the SPI read completed.

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/protocol/tmf8829.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L77)

***

### data

```ts
data: Uint8Array;
```

Defined in: [src/protocol/tmf8829.ts:78](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L78)

# Type Alias: Tmf8829CfgField

```ts
type Tmf8829CfgField = 
  | "period_ms"
  | "kilo_iterations"
  | "fp_mode"
  | "nr_peaks"
  | "signal_strength"
  | "noise_strength"
  | "xtalk"
  | "dump_histograms"
  | "ref_spad_frame"
  | "power_modes"
  | "ha_kilo_iterations"
  | "dual_mode"
  | "fov_correction"
  | "distance_in_mm";
```

Defined in: [src/sensors/tmf8829/config.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L53)

# Type Alias: Tmf8829Preset

```ts
type Tmf8829Preset = 
  | "8x8"
  | "8x8-lr"
  | "8x8-ha"
  | "16x16"
  | "16x16-ha"
  | "32x32"
  | "32x32-ha"
  | "48x32"
  | "48x32-ha"
  | "8x8-xr";
```

Defined in: [src/sensors/tmf8829/config.ts:10](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L10)

# Variable: TMF8829\_CFG\_FIELDS

```ts
const TMF8829_CFG_FIELDS: Readonly<Record<Tmf8829CfgField, readonly [number, number, number]>>;
```

Defined in: [src/sensors/tmf8829/config.ts:70](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L70)

Field → [register, bit shift, width in bits]; width 16 = little-endian register pair.

# Variable: TMF8829\_CFG\_SIZE

```ts
const TMF8829_CFG_SIZE: number;
```

Defined in: [src/sensors/tmf8829/config.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L8)

# Variable: TMF8829\_CHUNK\_MAX

```ts
const TMF8829_CHUNK_MAX: 1024 = 1024;
```

Defined in: [src/protocol/tmf8829.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L34)

# Variable: TMF8829\_DEFAULT\_PERIOD\_MS

```ts
const TMF8829_DEFAULT_PERIOD_MS: 33 = 33;
```

Defined in: [src/sensors/tmf8829/config.ts:51](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L51)

The sensor's own period after init().

# Variable: TMF8829\_FID\_HISTOGRAMS

```ts
const TMF8829_FID_HISTOGRAMS: 32 = 0x20;
```

Defined in: [src/sensors/tmf8829/frames.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L17)

# Variable: TMF8829\_FID\_MASK

```ts
const TMF8829_FID_MASK: 240 = 0xf0;
```

Defined in: [src/sensors/tmf8829/frames.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L15)

# Variable: TMF8829\_FID\_REF\_SPAD

```ts
const TMF8829_FID_REF_SPAD: 48 = 0x30;
```

Defined in: [src/sensors/tmf8829/frames.ts:18](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L18)

# Variable: TMF8829\_FID\_RESULTS

```ts
const TMF8829_FID_RESULTS: 16 = 0x10;
```

Defined in: [src/sensors/tmf8829/frames.ts:16](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L16)

# Variable: TMF8829\_FIRMWARE\_LICENSE

```ts
const TMF8829_FIRMWARE_LICENSE: "Copyright (C) 2025 AMS-OSRAM\n\n                        The MIT License (MIT)\n                Copyright © 2025 <copyright holders>\n\nPermission is hereby granted, free of charge, to any person obtaining a copy of this software\nand associated documentation files (the “Software”), to deal in the Software without\nrestriction, including without limitation the rights to use, copy, modify, merge, publish,\ndistribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the\nSoftware is furnished to do so, subject to the following conditions:\n\nThe above copyright notice and this permission notice shall be included in all copies or \nsubstantial portions of the Software.\n\nTHE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING \nBUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND \nNONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, \nDAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING \nFROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE." = "Copyright (C) 2025 AMS-OSRAM\n\n                        The MIT License (MIT)\n                Copyright © 2025 <copyright holders>\n\nPermission is hereby granted, free of charge, to any person obtaining a copy of this software\nand associated documentation files (the “Software”), to deal in the Software without\nrestriction, including without limitation the rights to use, copy, modify, merge, publish,\ndistribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the\nSoftware is furnished to do so, subject to the following conditions:\n\nThe above copyright notice and this permission notice shall be included in all copies or \nsubstantial portions of the Software.\n\nTHE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING \nBUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND \nNONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, \nDAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING \nFROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.";
```

Defined in: [src/sensors/tmf8829/firmware-license.ts:5](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/firmware-license.ts#L5)

The MIT license (ams-OSRAM) of the TMF8829 sensor firmware the SDK embeds.

# Variable: TMF8829\_FIRMWARE\_LOAD\_ADDR

```ts
const TMF8829_FIRMWARE_LOAD_ADDR: 65536 = 0x00010000;
```

Defined in: [src/sensors/tmf8829/tmf8829.ts:107](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L107)

ams tmf8829_application.hex load address (application 2.200.0).

# Variable: TMF8829\_FOOTER\_SIZE

```ts
const TMF8829_FOOTER_SIZE: 12 = 12;
```

Defined in: [src/sensors/tmf8829/frames.ts:10](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L10)

# Variable: TMF8829\_FP\_GRID

```ts
const TMF8829_FP_GRID: Readonly<Record<number, readonly [number, number]>>;
```

Defined in: [src/sensors/tmf8829/frames.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L39)

fp_mode → [rows, cols] of the full focal plane.

# Variable: TMF8829\_FP\_NAMES

```ts
const TMF8829_FP_NAMES: Readonly<Record<number, string>>;
```

Defined in: [src/sensors/tmf8829/frames.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L47)

# Variable: TMF8829\_FRAME\_EOF

```ts
const TMF8829_FRAME_EOF: 57591 = 0xe0f7;
```

Defined in: [src/sensors/tmf8829/frames.ts:13](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L13)

# Variable: TMF8829\_FRAME\_HEAD

```ts
const TMF8829_FRAME_HEAD: number;
```

Defined in: [src/sensors/tmf8829/frames.ts:12](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L12)

The 21 bytes the bridge reads at 0xFA.

# Variable: TMF8829\_FT\_CLOCK\_RESET

```ts
const TMF8829_FT_CLOCK_RESET: 2 = 0x02;
```

Defined in: [src/sensors/tmf8829/clock.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L19)

Clock model restarted at this frame (SYSTICK jump).

# Variable: TMF8829\_FT\_EDGE\_LATE

```ts
const TMF8829_FT_EDGE_LATE: 4 = 0x04;
```

Defined in: [src/sensors/tmf8829/clock.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L21)

INT edge > 2 ms later after t1 than usual in this stream.

# Variable: TMF8829\_FT\_NAMES

```ts
const TMF8829_FT_NAMES: ReadonlyArray<[number, string]>;
```

Defined in: [src/sensors/tmf8829/clock.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L27)

# Variable: TMF8829\_FT\_NOMINAL

```ts
const TMF8829_FT_NOMINAL: 1 = 0x01;
```

Defined in: [src/sensors/tmf8829/clock.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L17)

Clock ratio not measured yet (nominal 8 µs/tick used).

# Variable: TMF8829\_FT\_STALLED

```ts
const TMF8829_FT_STALLED: 8 = 0x08;
```

Defined in: [src/sensors/tmf8829/clock.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L23)

Sub-integrations not back to back (histogram FIFO wait).

# Variable: TMF8829\_FT\_T1\_UNSURE

```ts
const TMF8829_FT_T1_UNSURE: 16 = 0x10;
```

Defined in: [src/sensors/tmf8829/clock.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L25)

Histograms + 32x32/48x32: T1 may be too long, centre late.

# Variable: TMF8829\_HEADER\_SIZE

```ts
const TMF8829_HEADER_SIZE: 16 = 16;
```

Defined in: [src/sensors/tmf8829/frames.ts:9](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L9)

# Variable: TMF8829\_INT\_HISTOGRAMS

```ts
const TMF8829_INT_HISTOGRAMS: 8 = 0x08;
```

Defined in: [src/protocol/tmf8829.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L39)

# Variable: TMF8829\_INT\_MASK\_ALL

```ts
const TMF8829_INT_MASK_ALL: 15 = 0x0f;
```

Defined in: [src/protocol/tmf8829.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L40)

# Variable: TMF8829\_INT\_MOTION

```ts
const TMF8829_INT_MOTION: 2 = 0x02;
```

Defined in: [src/protocol/tmf8829.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L37)

# Variable: TMF8829\_INT\_PROXIMITY

```ts
const TMF8829_INT_PROXIMITY: 4 = 0x04;
```

Defined in: [src/protocol/tmf8829.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L38)

# Variable: TMF8829\_INT\_RESULTS

```ts
const TMF8829_INT_RESULTS: 1 = 0x01;
```

Defined in: [src/protocol/tmf8829.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L36)

# Variable: TMF8829\_LAYOUT\_NOISE

```ts
const TMF8829_LAYOUT_NOISE: 16 = 0x10;
```

Defined in: [src/sensors/tmf8829/frames.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L22)

# Variable: TMF8829\_LAYOUT\_PEAKS

```ts
const TMF8829_LAYOUT_PEAKS: 7 = 0x07;
```

Defined in: [src/sensors/tmf8829/frames.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L20)

# Variable: TMF8829\_LAYOUT\_SIGNAL

```ts
const TMF8829_LAYOUT_SIGNAL: 8 = 0x08;
```

Defined in: [src/sensors/tmf8829/frames.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L21)

# Variable: TMF8829\_LAYOUT\_SUBFRAME

```ts
const TMF8829_LAYOUT_SUBFRAME: 64 = 0x40;
```

Defined in: [src/sensors/tmf8829/frames.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L25)

32x32/48x32: set on the second (odd-row) half-frame.

# Variable: TMF8829\_LAYOUT\_XTALK

```ts
const TMF8829_LAYOUT_XTALK: 32 = 0x20;
```

Defined in: [src/sensors/tmf8829/frames.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L23)

# Variable: TMF8829\_NOMINAL\_Q16

```ts
const TMF8829_NOMINAL_Q16: number;
```

Defined in: [src/sensors/tmf8829/clock.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L11)

Sensor clock model and frame capture time (contracts/14_SENSOR_TMF8829.md §5).
Mirrors the Python reference `depz_sensor_sdk.tmf8829.clock`; the vectors in
`contracts/vectors/tmf8829_frame_time.json` pin it bit for bit.

Plain numbers, not bigint: every intermediate stays below 2^53 (µs values
~2^31, `span << 16` ~2^37, tick × ratio ~2^51), so `Math.floor(a / b)` is
exact. `>>` in JS is 32-bit, so the Q16 shifts are divisions by 65536.

# Variable: TMF8829\_PRESETS

```ts
const TMF8829_PRESETS: Readonly<Record<Tmf8829Preset, number>>;
```

Defined in: [src/sensors/tmf8829/config.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L23)

Preset → load command (each loads its config page).

# Variable: TMF8829\_PRESET\_KILO\_ITERATIONS

```ts
const TMF8829_PRESET_KILO_ITERATIONS: Readonly<Record<Tmf8829Preset, number>>;
```

Defined in: [src/sensors/tmf8829/config.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L37)

The preset's own kilo_iterations (a preset rescales a stale value, it does not reset it).

# Variable: TMF8829\_PRE\_HEADER\_SIZE

```ts
const TMF8829_PRE_HEADER_SIZE: 5 = 5;
```

Defined in: [src/sensors/tmf8829/frames.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L8)

# Variable: TMF8829\_READ\_MAX

```ts
const TMF8829_READ_MAX: 1024 = 1024;
```

Defined in: [src/protocol/tmf8829.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L32)

# Variable: TMF8829\_REG\_CFG

```ts
const TMF8829_REG_CFG: 34 = 0x22;
```

Defined in: [src/sensors/tmf8829/config.ts:7](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L7)

Config page 0x22..0xDF.

# Variable: TMF8829\_SF\_REF\_FRAME

```ts
const TMF8829_SF_REF_FRAME: 1 = 0x01;
```

Defined in: [src/protocol/tmf8829.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L42)

TMF_START_STREAM flags: emit the ref-SPAD frame before each results frame.

# Variable: TMF8829\_WRITE\_MAX

```ts
const TMF8829_WRITE_MAX: 1023 = 1023;
```

Defined in: [src/protocol/tmf8829.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L33)

