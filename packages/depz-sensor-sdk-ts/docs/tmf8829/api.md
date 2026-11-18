# TMF8829 (dToF) — API reference

The public API for the TMF8829 multizone direct time-of-flight sensor:
the `Tmf8829` device and its measurement / config types, the frame
parser and half-frame merge, the clock model and capture time, presets
and config-page fields, and the register-bridge wire codecs. Discovery,
device-base, transport and other cross-sensor symbols live in the [top-level API reference](../api.md).

Generated from the TypeScript sources by TypeDoc — run `bun run docs` to regenerate. Edit the doc comments in the source, not this file.

# @depz/sensor-sdk

## Enumerations

- [Tmf8829Cmd](#enumeration-tmf8829cmd)
- [Tmf8829Rpt](#enumeration-tmf8829rpt)
- [Tmf8829EventCode](#enumeration-tmf8829eventcode)

## Classes

- [Tmf8829FrameReassembler](#class-tmf8829framereassembler)
- [Tmf8829MeasurementBuilder](#class-tmf8829measurementbuilder)
- [Tmf8829SensorClock](#class-tmf8829sensorclock)
- [Tmf8829CaptureTimer](#class-tmf8829capturetimer)
- [Tmf8829Error](#class-tmf8829error)
- [Tmf8829](#class-tmf8829)

## Interfaces

- [Tmf8829RegData](#interface-tmf8829regdata)
- [Tmf8829Info](#interface-tmf8829info)
- [Tmf8829FrameChunk](#interface-tmf8829framechunk)
- [Tmf8829Event](#interface-tmf8829event)
- [Tmf8829RawFrame](#interface-tmf8829rawframe)
- [Tmf8829Measurement](#interface-tmf8829measurement)
- [Tmf8829FrameTime](#interface-tmf8829frametime)
- [Tmf8829FrameInfo](#interface-tmf8829frameinfo)
- [Tmf8829PixelResults](#interface-tmf8829pixelresults)
- [Tmf8829HistogramFrame](#interface-tmf8829histogramframe)
- [Tmf8829ConfigureOptions](#interface-tmf8829configureoptions)
- [Tmf8829Config](#interface-tmf8829config)
- [Tmf8829Options](#interface-tmf8829options)

## Type Aliases

- [Tmf8829Preset](#type-alias-tmf8829preset)
- [Tmf8829CfgField](#type-alias-tmf8829cfgfield)

## Variables

- [TMF8829\_READ\_MAX](#variable-tmf8829_read_max)
- [TMF8829\_WRITE\_MAX](#variable-tmf8829_write_max)
- [TMF8829\_CHUNK\_MAX](#variable-tmf8829_chunk_max)
- [TMF8829\_INT\_RESULTS](#variable-tmf8829_int_results)
- [TMF8829\_INT\_MOTION](#variable-tmf8829_int_motion)
- [TMF8829\_INT\_PROXIMITY](#variable-tmf8829_int_proximity)
- [TMF8829\_INT\_HISTOGRAMS](#variable-tmf8829_int_histograms)
- [TMF8829\_INT\_MASK\_ALL](#variable-tmf8829_int_mask_all)
- [TMF8829\_SF\_REF\_FRAME](#variable-tmf8829_sf_ref_frame)
- [TMF8829\_FT\_NOMINAL](#variable-tmf8829_ft_nominal)
- [TMF8829\_FT\_CLOCK\_RESET](#variable-tmf8829_ft_clock_reset)
- [TMF8829\_FT\_EDGE\_LATE](#variable-tmf8829_ft_edge_late)
- [TMF8829\_FT\_STALLED](#variable-tmf8829_ft_stalled)
- [TMF8829\_FT\_T1\_UNSURE](#variable-tmf8829_ft_t1_unsure)
- [TMF8829\_FT\_NAMES](#variable-tmf8829_ft_names)
- [TMF8829\_PRESETS](#variable-tmf8829_presets)
- [TMF8829\_CFG\_FIELDS](#variable-tmf8829_cfg_fields)
- [TMF8829\_FIRMWARE\_LICENSE](#variable-tmf8829_firmware_license)
- [TMF8829\_FP\_GRID](#variable-tmf8829_fp_grid)
- [TMF8829\_FP\_NAMES](#variable-tmf8829_fp_names)
- [TMF8829\_FIRMWARE\_LOAD\_ADDR](#variable-tmf8829_firmware_load_addr)

## Functions

- [packTmf8829ReadReg](#function-packtmf8829readreg)
- [packTmf8829WriteReg](#function-packtmf8829writereg)
- [packTmf8829StartStream](#function-packtmf8829startstream)
- [unpackTmf8829RegData](#function-unpacktmf8829regdata)
- [unpackTmf8829Info](#function-unpacktmf8829info)
- [unpackTmf8829FrameChunk](#function-unpacktmf8829framechunk)
- [unpackTmf8829Event](#function-unpacktmf8829event)
- [tmf8829DistanceMm](#function-tmf8829distancemm)
- [tmf8829RecordValue](#function-tmf8829recordvalue)
- [tmf8829FlagNames](#function-tmf8829flagnames)
- [tmf8829CfgGet](#function-tmf8829cfgget)
- [tmf8829CfgSet](#function-tmf8829cfgset)
- [tmf8829ConfigFields](#function-tmf8829configfields)
- [parseTmf8829Frame](#function-parsetmf8829frame)
- [parseTmf8829Results](#function-parsetmf8829results)
- [mergeTmf8829Halves](#function-mergetmf8829halves)
- [tmf8829FirmwareImage](#function-tmf8829firmwareimage)

# Class: Tmf8829

Defined in: [sensors/tmf8829/tmf8829.ts:165](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L165)

TMF8829 ToF device (8×8 … 48×32 zones, up to 4 peaks per zone, histograms).
`init()` downloads the 14.4 KB sensor firmware (~0.15 s), `configure(preset,
…)` writes the config page, `start()` streams measurements to the
`measurements` stream, `stop()` ends it. The sensor's EN pin is
wired high: there is no sleep, recovery from any state is `init()`.

## Extends

- `DepzDevice`

## Constructors

### Constructor

```ts
new Tmf8829(transport, opts?): Tmf8829;
```

Defined in: [sensors/tmf8829/tmf8829.ts:185](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L185)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | `SerialTransport` |
| `opts?` | [`Tmf8829Options`](#interface-tmf8829options) |

#### Returns

`Tmf8829`

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
readonly measurements: Stream<Tmf8829Measurement>;
```

Defined in: [sensors/tmf8829/tmf8829.ts:173](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L173)

The measurements (contract 07 §3): `on(cb)` subscribes, `subscribe(maxsize)`
iterates, `next(timeoutMs)` waits for one; `droppedCounts` and
`parseErrors` (a FRAME chunk or EVENT shorter than its header — contract
07 §2, 15 §2 — reset by `start()`; `frameParseErrors` adds the builder's)
are its counters.

***

### appVersion

```ts
appVersion: string | null = null;
```

Defined in: [sensors/tmf8829/tmf8829.ts:175](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L175)

***

### sensorSerial

```ts
sensorSerial: number | null = null;
```

Defined in: [sensors/tmf8829/tmf8829.ts:176](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L176)

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

### config

#### Get Signature

```ts
get config(): Tmf8829Config | null;
```

Defined in: [sensors/tmf8829/tmf8829.ts:355](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L355)

The last `configure()` result (null after `init()`).

##### Returns

[`Tmf8829Config`](#interface-tmf8829config) \| `null`

***

### streaming

#### Get Signature

```ts
get streaming(): boolean;
```

Defined in: [sensors/tmf8829/tmf8829.ts:361](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L361)

##### Returns

`boolean`

***

### framesDropped

#### Get Signature

```ts
get framesDropped(): number;
```

Defined in: [sensors/tmf8829/tmf8829.ts:420](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L420)

Frames the bridge dropped (frame_seq gaps) in this stream.

##### Returns

`number`

***

### framesDiscarded

#### Get Signature

```ts
get framesDiscarded(): number;
```

Defined in: [sensors/tmf8829/tmf8829.ts:425](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L425)

Frames lost to reassembly (incomplete, out-of-order chunks) in this stream.

##### Returns

`number`

***

### frameParseErrors

#### Get Signature

```ts
get frameParseErrors(): number;
```

Defined in: [sensors/tmf8829/tmf8829.ts:434](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L434)

Reports of this class dropped because they failed to decode: a FRAME
chunk or EVENT shorter than its header (contract 07 §2), or a frame the
builder could not parse. Reset by `start()`.

##### Returns

`number`

***

### clockRatioQ16

#### Get Signature

```ts
get clockRatioQ16(): number;
```

Defined in: [sensors/tmf8829/tmf8829.ts:439](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L439)

Current clock ratio (Q16), for diagnostics.

##### Returns

`number`

## Methods

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

### readRegisters()

```ts
readRegisters(addr, length): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [sensors/tmf8829/tmf8829.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L196)

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

Defined in: [sensors/tmf8829/tmf8829.ts:201](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L201)

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

Defined in: [sensors/tmf8829/tmf8829.ts:206](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L206)

Raw host-interface registers, for diagnostics.

#### Returns

`Promise`\<`Record`\<`string`, `number`\>\>

***

### getBridgeInfo()

```ts
getBridgeInfo(): Promise<Tmf8829Info>;
```

Defined in: [sensors/tmf8829/tmf8829.ts:226](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L226)

Bridge counters (never touches the sensor, safe mid-stream).

#### Returns

`Promise`\<[`Tmf8829Info`](#interface-tmf8829info)\>

***

### init()

```ts
init(progress?): Promise<[string, number]>;
```

Defined in: [sensors/tmf8829/tmf8829.ts:240](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L240)

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

Defined in: [sensors/tmf8829/tmf8829.ts:280](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L280)

LOAD_CONFIG_PAGE → the 190-byte page 0x22..0xDF now in the sensor.

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### configure()

```ts
configure(preset?, opts?): Promise<Tmf8829Config>;
```

Defined in: [sensors/tmf8829/tmf8829.ts:290](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L290)

Load `preset` and write the config page. Every field a preset does not
reset is written (contract 14 §4), so the result never depends on an
earlier config. Rejects with `Tmf8829Error` when the sensor refuses the
page (e.g. ERR_CONFIG_RESULT_SIZE) or a written field does not read back.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `preset` | [`Tmf8829Preset`](#type-alias-tmf8829preset) | `"8x8"` |
| `opts` | [`Tmf8829ConfigureOptions`](#interface-tmf8829configureoptions) | `{}` |

#### Returns

`Promise`\<[`Tmf8829Config`](#interface-tmf8829config)\>

***

### start()

```ts
start(): Promise<void>;
```

Defined in: [sensors/tmf8829/tmf8829.ts:366](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L366)

MEASURE with the bridge stream armed; configures the 8x8 preset first if needed.

#### Returns

`Promise`\<`void`\>

***

### stop()

```ts
stop(): Promise<void>;
```

Defined in: [sensors/tmf8829/tmf8829.ts:399](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L399)

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

Defined in: [sensors/tmf8829/tmf8829.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L414)

Close the connection. A device that handed its connection over owns none.

#### Returns

`Promise`\<`void`\>

#### Overrides

```ts
DepzDevice.close
```

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [sensors/tmf8829/tmf8829.ts:521](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L521)

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

Defined in: [sensors/tmf8829/clock.ts:148](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L148)

When a results frame was captured: the centre of [t0, t1 + T1], T1 the
stream's shortest (t1 − t0)/(N − 1).

## Constructors

### Constructor

```ts
new Tmf8829CaptureTimer(clock): Tmf8829CaptureTimer;
```

Defined in: [sensors/tmf8829/clock.ts:153](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L153)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `clock` | [`Tmf8829SensorClock`](#class-tmf8829sensorclock) |

#### Returns

`Tmf8829CaptureTimer`

## Properties

### clock

```ts
readonly clock: Tmf8829SensorClock;
```

Defined in: [sensors/tmf8829/clock.ts:153](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L153)

## Methods

### reset()

```ts
reset(): void;
```

Defined in: [sensors/tmf8829/clock.ts:155](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L155)

#### Returns

`void`

***

### histograms()

```ts
histograms(): void;
```

Defined in: [sensors/tmf8829/clock.ts:162](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L162)

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

Defined in: [sensors/tmf8829/clock.ts:166](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L166)

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

[`Tmf8829FrameTime`](#interface-tmf8829frametime)

# Class: Tmf8829Error

Defined in: [sensors/tmf8829/tmf8829.ts:125](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L125)

The sensor refused or did not answer (bootloader / application status).

## Extends

- `DepzError`

## Constructors

### Constructor

```ts
new Tmf8829Error(message?): Tmf8829Error;
```

Defined in: [errors.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L8)

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

# Class: Tmf8829FrameReassembler

Defined in: [protocol/tmf8829.ts:184](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L184)

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

Defined in: [protocol/tmf8829.ts:185](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L185)

***

### discarded

```ts
discarded: number = 0;
```

Defined in: [protocol/tmf8829.ts:186](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L186)

***

### seqGaps

```ts
seqGaps: number = 0;
```

Defined in: [protocol/tmf8829.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L187)

## Methods

### reset()

```ts
reset(): void;
```

Defined in: [protocol/tmf8829.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L194)

Forget everything — call at every stream start (frame_seq restarts at 0).

#### Returns

`void`

***

### feed()

```ts
feed(chunk): Tmf8829RawFrame | null;
```

Defined in: [protocol/tmf8829.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L210)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `chunk` | [`Tmf8829FrameChunk`](#interface-tmf8829framechunk) |

#### Returns

[`Tmf8829RawFrame`](#interface-tmf8829rawframe) \| `null`

# Class: Tmf8829MeasurementBuilder

Defined in: [sensors/tmf8829/builder.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L73)

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

Defined in: [sensors/tmf8829/builder.ts:74](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L74)

***

### timer

```ts
readonly timer: Tmf8829CaptureTimer;
```

Defined in: [sensors/tmf8829/builder.ts:75](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L75)

***

### parseErrors

```ts
parseErrors: number = 0;
```

Defined in: [sensors/tmf8829/builder.ts:76](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L76)

## Methods

### reset()

```ts
reset(): void;
```

Defined in: [sensors/tmf8829/builder.ts:83](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L83)

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

Defined in: [sensors/tmf8829/builder.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L98)

One frame with the MCU time of its header read and its INT edge (0 =
none) → the measurement it completes, or null. `raw` is copied.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `rawIn` | `Uint8Array` | `undefined` |
| `timestampUs` | `number` | `undefined` |
| `edgeUs` | `number` | `0` |

#### Returns

[`Tmf8829Measurement`](#interface-tmf8829measurement) \| `null`

# Class: Tmf8829SensorClock

Defined in: [sensors/tmf8829/clock.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L59)

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

Defined in: [sensors/tmf8829/clock.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L60)

***

### nominal

```ts
nominal: boolean = true;
```

Defined in: [sensors/tmf8829/clock.ts:61](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L61)

## Accessors

### ratio

#### Get Signature

```ts
get ratio(): number;
```

Defined in: [sensors/tmf8829/clock.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L77)

MCU/sensor clock ratio (≈ 1.015): distances are scaled by it.

##### Returns

`number`

## Methods

### reset()

```ts
reset(): void;
```

Defined in: [sensors/tmf8829/clock.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L67)

#### Returns

`void`

***

### add()

```ts
add(us, tick): number;
```

Defined in: [sensors/tmf8829/clock.ts:88](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L88)

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

Defined in: [sensors/tmf8829/clock.ts:121](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L121)

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

Defined in: [protocol/tmf8829.ts:9](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L9)

TMF8829 SPI register-bridge wire codecs (contract 14 §2).
Mirrors the Python reference `depz_sensor_sdk.protocol.tmf8829`.

Same command numbering as the VL53 bridges (0x32/0x33/0x35/0x36/0x37) but
TMF8829 payloads: an 8-bit register address and self-sized frames.

## Enumeration Members

### ReadReg

```ts
ReadReg: 50;
```

Defined in: [protocol/tmf8829.ts:10](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L10)

***

### WriteReg

```ts
WriteReg: 51;
```

Defined in: [protocol/tmf8829.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L11)

***

### StartStream

```ts
StartStream: 53;
```

Defined in: [protocol/tmf8829.ts:12](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L12)

***

### StopStream

```ts
StopStream: 54;
```

Defined in: [protocol/tmf8829.ts:13](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L13)

***

### GetInfo

```ts
GetInfo: 55;
```

Defined in: [protocol/tmf8829.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L14)

# Enumeration: Tmf8829EventCode

Defined in: [protocol/tmf8829.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L25)

RPT_TMF_EVENT.code

## Enumeration Members

### Int

```ts
Int: 0;
```

Defined in: [protocol/tmf8829.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L27)

MOTION/PROXIMITY interrupt, no frame; its bits already cleared.

***

### BadHeader

```ts
BadHeader: 1;
```

Defined in: [protocol/tmf8829.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L29)

Frame header refused, frame not read (expected once on STOP mid histogram dump).

# Enumeration: Tmf8829Rpt

Defined in: [protocol/tmf8829.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L17)

## Enumeration Members

### RegData

```ts
RegData: 145;
```

Defined in: [protocol/tmf8829.ts:18](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L18)

***

### Info

```ts
Info: 146;
```

Defined in: [protocol/tmf8829.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L19)

***

### Frame

```ts
Frame: 147;
```

Defined in: [protocol/tmf8829.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L20)

***

### Event

```ts
Event: 148;
```

Defined in: [protocol/tmf8829.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L21)

# Function: mergeTmf8829Halves()

```ts
function mergeTmf8829Halves(first, second): Tmf8829PixelResults;
```

Defined in: [sensors/tmf8829/frames.ts:212](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L212)

32x32/48x32: the first half-frame holds the even rows, the second the odd ones.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `first` | [`Tmf8829PixelResults`](#interface-tmf8829pixelresults) |
| `second` | [`Tmf8829PixelResults`](#interface-tmf8829pixelresults) |

## Returns

[`Tmf8829PixelResults`](#interface-tmf8829pixelresults)

# Function: packTmf8829ReadReg()

```ts
function packTmf8829ReadReg(addr, length): Uint8Array;
```

Defined in: [protocol/tmf8829.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L44)

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

Defined in: [protocol/tmf8829.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L63)

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

Defined in: [protocol/tmf8829.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L52)

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

Defined in: [sensors/tmf8829/frames.ts:86](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L86)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `raw` | `Uint8Array` |

## Returns

[`Tmf8829FrameInfo`](#interface-tmf8829frameinfo)

# Function: parseTmf8829Results()

```ts
function parseTmf8829Results(raw, info?): Tmf8829PixelResults;
```

Defined in: [sensors/tmf8829/frames.ts:158](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L158)

Pixel results of one results frame.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `raw` | `Uint8Array` |
| `info?` | [`Tmf8829FrameInfo`](#interface-tmf8829frameinfo) |

## Returns

[`Tmf8829PixelResults`](#interface-tmf8829pixelresults)

# Function: tmf8829CfgGet()

```ts
function tmf8829CfgGet(page, name): number;
```

Defined in: [sensors/tmf8829/config.ts:96](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L96)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `page` | `Uint8Array` |
| `name` | [`Tmf8829CfgField`](#type-alias-tmf8829cfgfield) |

## Returns

`number`

# Function: tmf8829CfgSet()

```ts
function tmf8829CfgSet(
   page, 
   name, 
   value): void;
```

Defined in: [sensors/tmf8829/config.ts:103](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L103)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `page` | `Uint8Array` |
| `name` | [`Tmf8829CfgField`](#type-alias-tmf8829cfgfield) |
| `value` | `number` |

## Returns

`void`

# Function: tmf8829ConfigFields()

```ts
function tmf8829ConfigFields(page): Record<Tmf8829CfgField, number>;
```

Defined in: [sensors/tmf8829/config.ts:119](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L119)

Every named field of a config page.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `page` | `Uint8Array` |

## Returns

`Record`\<[`Tmf8829CfgField`](#type-alias-tmf8829cfgfield), `number`\>

# Function: tmf8829DistanceMm()

```ts
function tmf8829DistanceMm(
   results, 
   ratio, 
   inMm): Float64Array<ArrayBufferLike> | null;
```

Defined in: [sensors/tmf8829/builder.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L54)

Raw distances → mm, clock-corrected (×ratio; ÷4 for 0.25 mm units).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `results` | [`Tmf8829PixelResults`](#interface-tmf8829pixelresults) |
| `ratio` | `number` |
| `inMm` | `boolean` |

## Returns

`Float64Array`\<`ArrayBufferLike`\> \| `null`

# Function: tmf8829FirmwareImage()

```ts
function tmf8829FirmwareImage(): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [sensors/tmf8829/tmf8829.ts:117](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L117)

The sensor firmware image `init()` downloads (14 444 B, MIT). Lazily decoded.

## Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

# Function: tmf8829FlagNames()

```ts
function tmf8829FlagNames(flags): string[];
```

Defined in: [sensors/tmf8829/clock.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L38)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `flags` | `number` |

## Returns

`string`[]

# Function: tmf8829RecordValue()

```ts
function tmf8829RecordValue(m): Record<string, unknown>;
```

Defined in: [sensors/tmf8829/builder.ts:216](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L216)

Contract 09 `tmf8829` record value of a measurement (distances rounded to 0.1 mm).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `m` | [`Tmf8829Measurement`](#interface-tmf8829measurement) |

## Returns

`Record`\<`string`, `unknown`\>

# Function: unpackTmf8829Event()

```ts
function unpackTmf8829Event(p): Tmf8829Event;
```

Defined in: [protocol/tmf8829.ts:159](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L159)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | `Uint8Array` |

## Returns

[`Tmf8829Event`](#interface-tmf8829event)

# Function: unpackTmf8829FrameChunk()

```ts
function unpackTmf8829FrameChunk(p): Tmf8829FrameChunk;
```

Defined in: [protocol/tmf8829.ts:135](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L135)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | `Uint8Array` |

## Returns

[`Tmf8829FrameChunk`](#interface-tmf8829framechunk)

# Function: unpackTmf8829Info()

```ts
function unpackTmf8829Info(p): Tmf8829Info;
```

Defined in: [protocol/tmf8829.ts:104](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L104)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | `Uint8Array` |

## Returns

[`Tmf8829Info`](#interface-tmf8829info)

# Function: unpackTmf8829RegData()

```ts
function unpackTmf8829RegData(p): Tmf8829RegData;
```

Defined in: [protocol/tmf8829.ts:81](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L81)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | `Uint8Array` |

## Returns

[`Tmf8829RegData`](#interface-tmf8829regdata)

# Interface: Tmf8829Config

Defined in: [sensors/tmf8829/tmf8829.ts:142](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L142)

The config written by `configure()`, as read back from the sensor.

## Properties

### preset

```ts
preset: Tmf8829Preset;
```

Defined in: [sensors/tmf8829/tmf8829.ts:143](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L143)

***

### page

```ts
page: Uint8Array;
```

Defined in: [sensors/tmf8829/tmf8829.ts:145](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L145)

The 190-byte page 0x22..0xDF.

***

### histograms

```ts
histograms: boolean;
```

Defined in: [sensors/tmf8829/tmf8829.ts:146](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L146)

***

### refFrame

```ts
refFrame: boolean;
```

Defined in: [sensors/tmf8829/tmf8829.ts:147](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L147)

***

### fields

```ts
fields: Record<Tmf8829CfgField, number>;
```

Defined in: [sensors/tmf8829/tmf8829.ts:148](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L148)

***

### grid

```ts
grid: readonly [number, number];
```

Defined in: [sensors/tmf8829/tmf8829.ts:150](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L150)

[rows, cols] of a measurement.

# Interface: Tmf8829ConfigureOptions

Defined in: [sensors/tmf8829/tmf8829.ts:127](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L127)

## Properties

### periodMs?

```ts
optional periodMs?: number;
```

Defined in: [sensors/tmf8829/tmf8829.ts:128](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L128)

***

### kiloIterations?

```ts
optional kiloIterations?: number;
```

Defined in: [sensors/tmf8829/tmf8829.ts:130](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L130)

Defaults to the preset's own value.

***

### nrPeaks?

```ts
optional nrPeaks?: number;
```

Defined in: [sensors/tmf8829/tmf8829.ts:131](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L131)

***

### signalStrength?

```ts
optional signalStrength?: boolean;
```

Defined in: [sensors/tmf8829/tmf8829.ts:132](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L132)

***

### noiseStrength?

```ts
optional noiseStrength?: boolean;
```

Defined in: [sensors/tmf8829/tmf8829.ts:133](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L133)

***

### xtalk?

```ts
optional xtalk?: boolean;
```

Defined in: [sensors/tmf8829/tmf8829.ts:134](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L134)

***

### histograms?

```ts
optional histograms?: boolean;
```

Defined in: [sensors/tmf8829/tmf8829.ts:135](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L135)

***

### refFrame?

```ts
optional refFrame?: boolean;
```

Defined in: [sensors/tmf8829/tmf8829.ts:136](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L136)

***

### fields?

```ts
optional fields?: Partial<Record<Tmf8829CfgField, number>>;
```

Defined in: [sensors/tmf8829/tmf8829.ts:138](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L138)

Any other config-page field by name.

# Interface: Tmf8829Event

Defined in: [protocol/tmf8829.ts:149](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L149)

RPT_TMF_EVENT. `intStatus` is INT_STATUS as read (unmasked).

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [protocol/tmf8829.ts:150](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L150)

***

### intStatus

```ts
intStatus: number;
```

Defined in: [protocol/tmf8829.ts:151](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L151)

***

### code

```ts
code: number;
```

Defined in: [protocol/tmf8829.ts:152](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L152)

***

### frameId

```ts
frameId: number;
```

Defined in: [protocol/tmf8829.ts:154](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L154)

Header id byte (BAD_HEADER), else 0.

***

### fifoStatus

```ts
fifoStatus: number;
```

Defined in: [protocol/tmf8829.ts:156](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L156)

Pre-header byte (BAD_HEADER), else 0.

# Interface: Tmf8829FrameChunk

Defined in: [protocol/tmf8829.ts:126](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L126)

One RPT_TMF_FRAME chunk. `timestampUs` + the frame's pre-header SYSTICK are
a clock pair (not the capture time); `edgeUs` is the INT edge credited to
the frame, 0 = none.

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [protocol/tmf8829.ts:127](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L127)

***

### fullSize

```ts
fullSize: number;
```

Defined in: [protocol/tmf8829.ts:128](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L128)

***

### offset

```ts
offset: number;
```

Defined in: [protocol/tmf8829.ts:129](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L129)

***

### frameSeq

```ts
frameSeq: number;
```

Defined in: [protocol/tmf8829.ts:130](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L130)

***

### edgeUs

```ts
edgeUs: bigint;
```

Defined in: [protocol/tmf8829.ts:131](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L131)

***

### data

```ts
data: Uint8Array;
```

Defined in: [protocol/tmf8829.ts:132](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L132)

# Interface: Tmf8829FrameInfo

Defined in: [sensors/tmf8829/frames.ts:58](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L58)

Pre-header, header and footer of one frame (any kind).

## Properties

### fifoStatus

```ts
fifoStatus: number;
```

Defined in: [sensors/tmf8829/frames.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L59)

***

### systick

```ts
systick: number;
```

Defined in: [sensors/tmf8829/frames.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L60)

***

### id

```ts
id: number;
```

Defined in: [sensors/tmf8829/frames.ts:61](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L61)

***

### kind

```ts
kind: number;
```

Defined in: [sensors/tmf8829/frames.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L63)

TMF8829_FID_RESULTS / _HISTOGRAMS / _REF_SPAD

***

### fpMode

```ts
fpMode: number;
```

Defined in: [sensors/tmf8829/frames.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L64)

***

### layout

```ts
layout: number;
```

Defined in: [sensors/tmf8829/frames.ts:65](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L65)

***

### payload

```ts
payload: number;
```

Defined in: [sensors/tmf8829/frames.ts:66](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L66)

***

### fNumber

```ts
fNumber: number;
```

Defined in: [sensors/tmf8829/frames.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L67)

***

### temperature

```ts
temperature: [number, number, number];
```

Defined in: [sensors/tmf8829/frames.ts:68](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L68)

***

### bdv

```ts
bdv: number;
```

Defined in: [sensors/tmf8829/frames.ts:69](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L69)

***

### refPos

```ts
refPos: [number, number];
```

Defined in: [sensors/tmf8829/frames.ts:70](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L70)

***

### t0Integration

```ts
t0Integration: number;
```

Defined in: [sensors/tmf8829/frames.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L71)

***

### t1Integration

```ts
t1Integration: number;
```

Defined in: [sensors/tmf8829/frames.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L72)

***

### frameStatus

```ts
frameStatus: number;
```

Defined in: [sensors/tmf8829/frames.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L73)

***

### reserved

```ts
reserved: number;
```

Defined in: [sensors/tmf8829/frames.ts:74](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L74)

***

### eof

```ts
eof: number;
```

Defined in: [sensors/tmf8829/frames.ts:75](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L75)

***

### size

```ts
size: number;
```

Defined in: [sensors/tmf8829/frames.ts:76](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L76)

***

### isSecondHalf

```ts
isSecondHalf: boolean;
```

Defined in: [sensors/tmf8829/frames.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L77)

***

### distanceInMm

```ts
distanceInMm: boolean;
```

Defined in: [sensors/tmf8829/frames.ts:78](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L78)

# Interface: Tmf8829FrameTime

Defined in: [sensors/tmf8829/clock.ts:127](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L127)

Capture time of one results frame, MCU µs.

## Properties

### timestampUs

```ts
timestampUs: number;
```

Defined in: [sensors/tmf8829/clock.ts:129](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L129)

Centre of the capture window.

***

### flags

```ts
flags: number;
```

Defined in: [sensors/tmf8829/clock.ts:131](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L131)

TMF8829_FT_*

***

### t1LenUs

```ts
t1LenUs: number;
```

Defined in: [sensors/tmf8829/clock.ts:133](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L133)

Sub-integration length used.

***

### t0Us

```ts
t0Us: number;
```

Defined in: [sensors/tmf8829/clock.ts:134](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L134)

***

### t1Us

```ts
t1Us: number;
```

Defined in: [sensors/tmf8829/clock.ts:135](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L135)

***

### edgeUs

```ts
edgeUs: number;
```

Defined in: [sensors/tmf8829/clock.ts:137](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L137)

INT edge credited to the frame, 0 = none.

***

### pairUs

```ts
pairUs: number;
```

Defined in: [sensors/tmf8829/clock.ts:139](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L139)

MCU time of the header read (the clock pair).

***

### ratioQ16

```ts
ratioQ16: number;
```

Defined in: [sensors/tmf8829/clock.ts:141](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L141)

Clock ratio after this frame.

# Interface: Tmf8829HistogramFrame

Defined in: [sensors/tmf8829/frames.ts:243](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L243)

A raw histogram frame (25 377 B); decoding the bins is left to the caller.

## Properties

### info

```ts
info: Tmf8829FrameInfo;
```

Defined in: [sensors/tmf8829/frames.ts:244](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L244)

***

### raw

```ts
raw: Uint8Array;
```

Defined in: [sensors/tmf8829/frames.ts:245](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L245)

# Interface: Tmf8829Info

Defined in: [protocol/tmf8829.ts:90](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L90)

RPT_TMF_INFO: bridge state only (no echoed command byte).

## Properties

### intEdges

```ts
intEdges: number;
```

Defined in: [protocol/tmf8829.ts:91](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L91)

***

### frames

```ts
frames: number;
```

Defined in: [protocol/tmf8829.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L92)

***

### framesAborted

```ts
framesAborted: number;
```

Defined in: [protocol/tmf8829.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L93)

***

### frameErrors

```ts
frameErrors: number;
```

Defined in: [protocol/tmf8829.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L94)

***

### spiTimeouts

```ts
spiTimeouts: number;
```

Defined in: [protocol/tmf8829.ts:95](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L95)

***

### intLevel

```ts
intLevel: number;
```

Defined in: [protocol/tmf8829.ts:97](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L97)

PA4 level now, 0 = asserted.

***

### streaming

```ts
streaming: boolean;
```

Defined in: [protocol/tmf8829.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L98)

***

### intMask

```ts
intMask: number;
```

Defined in: [protocol/tmf8829.ts:99](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L99)

***

### lastIntStatus

```ts
lastIntStatus: number;
```

Defined in: [protocol/tmf8829.ts:100](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L100)

***

### lastFifoStatus

```ts
lastFifoStatus: number;
```

Defined in: [protocol/tmf8829.ts:101](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L101)

# Interface: Tmf8829Measurement

Defined in: [sensors/tmf8829/builder.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L27)

One measurement: its results frame (8x8, 16x16) or both half-frames (32x32,
48x32) merged, with the histogram / ref-SPAD frames before it.

## Properties

### timestampUs

```ts
timestampUs: number;
```

Defined in: [sensors/tmf8829/builder.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L29)

Capture time, MCU µs (floor mean of the half-frame captures).

***

### flags

```ts
flags: number;
```

Defined in: [sensors/tmf8829/builder.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L31)

TMF8829_FT_* of all its results frames.

***

### fpMode

```ts
fpMode: number;
```

Defined in: [sensors/tmf8829/builder.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L32)

***

### fNumber

```ts
fNumber: number;
```

Defined in: [sensors/tmf8829/builder.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L33)

***

### rows

```ts
rows: number;
```

Defined in: [sensors/tmf8829/builder.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L34)

***

### cols

```ts
cols: number;
```

Defined in: [sensors/tmf8829/builder.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L35)

***

### results

```ts
results: Tmf8829PixelResults;
```

Defined in: [sensors/tmf8829/builder.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L37)

Raw sensor integers.

***

### distanceMm

```ts
distanceMm: Float64Array<ArrayBufferLike> | null;
```

Defined in: [sensors/tmf8829/builder.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L39)

Clock-corrected mm, flat [peak][row][col] (null without peaks).

***

### ratioQ16

```ts
ratioQ16: number;
```

Defined in: [sensors/tmf8829/builder.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L41)

Clock ratio the distances were corrected with (Q16).

***

### frames

```ts
frames: Tmf8829FrameInfo[];
```

Defined in: [sensors/tmf8829/builder.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L42)

***

### times

```ts
times: Tmf8829FrameTime[];
```

Defined in: [sensors/tmf8829/builder.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L43)

***

### histograms

```ts
histograms: Tmf8829HistogramFrame[];
```

Defined in: [sensors/tmf8829/builder.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L44)

***

### refFrames

```ts
refFrames: Uint8Array<ArrayBufferLike>[];
```

Defined in: [sensors/tmf8829/builder.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L46)

65 B each, zero pre-header.

***

### problems

```ts
problems: string[];
```

Defined in: [sensors/tmf8829/builder.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L48)

What went wrong since the previous measurement, human-readable.

***

### temperatureC

```ts
temperatureC: number;
```

Defined in: [sensors/tmf8829/builder.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/builder.ts#L50)

Sensor die temperature (header byte 0 of the last frame), °C.

# Interface: Tmf8829Options

Defined in: [sensors/tmf8829/tmf8829.ts:153](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L153)

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

Defined in: [sensors/tmf8829/tmf8829.ts:155](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L155)

Injectable sleep (tests).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Interface: Tmf8829PixelResults

Defined in: [sensors/tmf8829/frames.ts:139](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L139)

Decoded results of one frame or of a merged measurement. Per-peak arrays
are flat `[peak][row][col]` (row-major); noise / xtalk `[row][col]`. Raw
sensor integers: distance in 0.25 mm (or mm with the footer flag), not yet
clock-corrected. Absent fields are null.

## Properties

### peaks

```ts
peaks: number;
```

Defined in: [sensors/tmf8829/frames.ts:140](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L140)

***

### rows

```ts
rows: number;
```

Defined in: [sensors/tmf8829/frames.ts:141](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L141)

***

### cols

```ts
cols: number;
```

Defined in: [sensors/tmf8829/frames.ts:142](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L142)

***

### distance

```ts
distance: Uint16Array<ArrayBufferLike> | null;
```

Defined in: [sensors/tmf8829/frames.ts:143](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L143)

***

### snr

```ts
snr: Uint8Array<ArrayBufferLike> | null;
```

Defined in: [sensors/tmf8829/frames.ts:144](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L144)

***

### signal

```ts
signal: Uint16Array<ArrayBufferLike> | null;
```

Defined in: [sensors/tmf8829/frames.ts:145](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L145)

***

### noise

```ts
noise: Uint16Array<ArrayBufferLike> | null;
```

Defined in: [sensors/tmf8829/frames.ts:146](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L146)

***

### xtalk

```ts
xtalk: Uint16Array<ArrayBufferLike> | null;
```

Defined in: [sensors/tmf8829/frames.ts:147](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L147)

# Interface: Tmf8829RawFrame

Defined in: [protocol/tmf8829.ts:171](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L171)

A reassembled frame: the sensor bytes from register 0xFA on.

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [protocol/tmf8829.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L172)

***

### edgeUs

```ts
edgeUs: bigint;
```

Defined in: [protocol/tmf8829.ts:173](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L173)

***

### frameSeq

```ts
frameSeq: number;
```

Defined in: [protocol/tmf8829.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L174)

***

### data

```ts
data: Uint8Array;
```

Defined in: [protocol/tmf8829.ts:175](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L175)

# Interface: Tmf8829RegData

Defined in: [protocol/tmf8829.ts:76](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L76)

RPT_TMF_REG_DATA: register bytes and the MCU time the SPI read completed.

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [protocol/tmf8829.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L77)

***

### data

```ts
data: Uint8Array;
```

Defined in: [protocol/tmf8829.ts:78](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L78)

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

Defined in: [sensors/tmf8829/config.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L54)

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

Defined in: [sensors/tmf8829/config.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L11)

# Variable: TMF8829\_CFG\_FIELDS

```ts
const TMF8829_CFG_FIELDS: Readonly<Record<Tmf8829CfgField, readonly [number, number, number]>>;
```

Defined in: [sensors/tmf8829/config.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L71)

Field → [register, bit shift, width in bits]; width 16 = little-endian register pair.

# Variable: TMF8829\_CHUNK\_MAX

```ts
const TMF8829_CHUNK_MAX: 1024 = 1024;
```

Defined in: [protocol/tmf8829.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L34)

# Variable: TMF8829\_FIRMWARE\_LICENSE

```ts
const TMF8829_FIRMWARE_LICENSE: "Copyright (C) 2025 AMS-OSRAM\n\n                        The MIT License (MIT)\n                Copyright © 2025 <copyright holders>\n\nPermission is hereby granted, free of charge, to any person obtaining a copy of this software\nand associated documentation files (the “Software”), to deal in the Software without\nrestriction, including without limitation the rights to use, copy, modify, merge, publish,\ndistribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the\nSoftware is furnished to do so, subject to the following conditions:\n\nThe above copyright notice and this permission notice shall be included in all copies or \nsubstantial portions of the Software.\n\nTHE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING \nBUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND \nNONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, \nDAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING \nFROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE." = "Copyright (C) 2025 AMS-OSRAM\n\n                        The MIT License (MIT)\n                Copyright © 2025 <copyright holders>\n\nPermission is hereby granted, free of charge, to any person obtaining a copy of this software\nand associated documentation files (the “Software”), to deal in the Software without\nrestriction, including without limitation the rights to use, copy, modify, merge, publish,\ndistribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the\nSoftware is furnished to do so, subject to the following conditions:\n\nThe above copyright notice and this permission notice shall be included in all copies or \nsubstantial portions of the Software.\n\nTHE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING \nBUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND \nNONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, \nDAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING \nFROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.";
```

Defined in: [sensors/tmf8829/firmware-license.ts:5](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/firmware-license.ts#L5)

The MIT license (ams-OSRAM) of the TMF8829 sensor firmware the SDK embeds.

# Variable: TMF8829\_FIRMWARE\_LOAD\_ADDR

```ts
const TMF8829_FIRMWARE_LOAD_ADDR: 65536 = 0x00010000;
```

Defined in: [sensors/tmf8829/tmf8829.ts:112](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/tmf8829.ts#L112)

ams tmf8829_application.hex load address (application 2.200.0).

# Variable: TMF8829\_FP\_GRID

```ts
const TMF8829_FP_GRID: Readonly<Record<number, readonly [number, number]>>;
```

Defined in: [sensors/tmf8829/frames.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L40)

fp_mode → [rows, cols] of the full focal plane.

# Variable: TMF8829\_FP\_NAMES

```ts
const TMF8829_FP_NAMES: Readonly<Record<number, string>>;
```

Defined in: [sensors/tmf8829/frames.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/frames.ts#L48)

# Variable: TMF8829\_FT\_CLOCK\_RESET

```ts
const TMF8829_FT_CLOCK_RESET: 2 = 0x02;
```

Defined in: [sensors/tmf8829/clock.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L22)

Clock model restarted at this frame (SYSTICK jump).

# Variable: TMF8829\_FT\_EDGE\_LATE

```ts
const TMF8829_FT_EDGE_LATE: 4 = 0x04;
```

Defined in: [sensors/tmf8829/clock.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L24)

INT edge > 2 ms later after t1 than usual in this stream.

# Variable: TMF8829\_FT\_NAMES

```ts
const TMF8829_FT_NAMES: ReadonlyArray<[number, string]>;
```

Defined in: [sensors/tmf8829/clock.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L30)

# Variable: TMF8829\_FT\_NOMINAL

```ts
const TMF8829_FT_NOMINAL: 1 = 0x01;
```

Defined in: [sensors/tmf8829/clock.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L20)

Clock ratio not measured yet (nominal 8 µs/tick used).

# Variable: TMF8829\_FT\_STALLED

```ts
const TMF8829_FT_STALLED: 8 = 0x08;
```

Defined in: [sensors/tmf8829/clock.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L26)

Sub-integrations not back to back (histogram FIFO wait).

# Variable: TMF8829\_FT\_T1\_UNSURE

```ts
const TMF8829_FT_T1_UNSURE: 16 = 0x10;
```

Defined in: [sensors/tmf8829/clock.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/clock.ts#L28)

Histograms + 32x32/48x32: T1 may be too long, centre late.

# Variable: TMF8829\_INT\_HISTOGRAMS

```ts
const TMF8829_INT_HISTOGRAMS: 8 = 0x08;
```

Defined in: [protocol/tmf8829.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L39)

# Variable: TMF8829\_INT\_MASK\_ALL

```ts
const TMF8829_INT_MASK_ALL: 15 = 0x0f;
```

Defined in: [protocol/tmf8829.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L40)

# Variable: TMF8829\_INT\_MOTION

```ts
const TMF8829_INT_MOTION: 2 = 0x02;
```

Defined in: [protocol/tmf8829.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L37)

# Variable: TMF8829\_INT\_PROXIMITY

```ts
const TMF8829_INT_PROXIMITY: 4 = 0x04;
```

Defined in: [protocol/tmf8829.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L38)

# Variable: TMF8829\_INT\_RESULTS

```ts
const TMF8829_INT_RESULTS: 1 = 0x01;
```

Defined in: [protocol/tmf8829.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L36)

# Variable: TMF8829\_PRESETS

```ts
const TMF8829_PRESETS: Readonly<Record<Tmf8829Preset, number>>;
```

Defined in: [sensors/tmf8829/config.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/tmf8829/config.ts#L24)

Preset → load command (each loads its config page).

# Variable: TMF8829\_READ\_MAX

```ts
const TMF8829_READ_MAX: 1024 = 1024;
```

Defined in: [protocol/tmf8829.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L32)

# Variable: TMF8829\_SF\_REF\_FRAME

```ts
const TMF8829_SF_REF_FRAME: 1 = 0x01;
```

Defined in: [protocol/tmf8829.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L42)

TMF_START_STREAM flags: emit the ref-SPAD frame before each results frame.

# Variable: TMF8829\_WRITE\_MAX

```ts
const TMF8829_WRITE_MAX: 1023 = 1023;
```

Defined in: [protocol/tmf8829.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/tmf8829.ts#L33)

