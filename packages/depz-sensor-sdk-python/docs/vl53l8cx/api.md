# VL53L8CX (ToF) — API reference

The public API for the VL53L8CX (ToF) sensor. Discovery, device-base,
transport and other cross-sensor symbols shared by every sensor
live in the [top-level API reference](../api.md).

Generated from the docstrings — edit them in the source, not
this file.

## Contents

- **VL53L8CX (ToF)**: [`Vl53l8cx`](#vl53l8cx), [`Vl53l8Frame`](#vl53l8frame), [`Vl53l8cxError`](#vl53l8cxerror), [`RESOLUTION_4X4`](#resolution_4x4), [`RESOLUTION_8X8`](#resolution_8x8)

## VL53L8CX (ToF)

### Vl53l8cx

```python
class Vl53l8cx(port_or_link: str | depz_sensor_sdk.transport.link.Link, *, timeout: float = 0.2, tx_crc_type: depz_sensor_sdk.transport.framing.CrcType = <CrcType.NONE: 0>) -> None
```

VL53L8CX ToF device: `init()` downloads the ~84 KB sensor firmware
(~1 s over the CDC link), then configure and `start_ranging()`.

This is the base class for both silicon variants. The VL53L8CH superset
(compact-network-histogram output) lives in `Vl53l8ch`, which inherits
every method here. All configuration methods require `init()` first and
must not be called while ranging (the ULD talks to the current register
bank; the stream owns it — contract 04).

#### Vl53l8cx.frames *(attribute)*

```python
frames: Stream[Vl53l8Frame]
```

The parsed-frame stream: `frames.on(cb)` subscribes (reader-thread
context; don't block), `frames(maxsize)` iterates (bounded, drop-oldest,
subscribes immediately — call before or after `start_ranging()`),
`frames.next(timeout)` waits for one; `dropped_counts` and `parse_errors`
(frames ULD parsing refused: corrupt frame, bad size — distinct from
`reassembler_discards`) are its counters.

#### Vl53l8cx.events *(attribute)*

```python
events: Stream[DeviceEvent]
```

The unsolicited / diagnostic events (contract 07 §2): `events.on(cb)`
subscribes (reader-thread context; do not block) and returns an
unsubscribe function, `events(maxsize)` is a bounded drop-oldest iterator
that subscribes immediately, `events.dropped_counts` its queues' drops.

#### Vl53l8cx.uld *(property)*

The underlying ULD driver (escape hatch for advanced DCI access).

unstable — not covered by the 1.0 freeze.

#### Vl53l8cx.variant *(property)*

'cx' | 'ch' (valid after init()).

#### Vl53l8cx.is_alive

```python
is_alive(self) -> bool
```

True when the sensor answers with its own silicon ids (a bus
error or a foreign id: False).

#### Vl53l8cx.init

```python
init(self, *, progress: Optional[Callable[[str], NoneType]] = None, write_progress: Optional[Callable[[int, int], NoneType]] = None) -> None
```

Initialize the sensor: firmware blob download + default config.

The blob variant is fixed by the class (`Vl53l8cx` → 'cx',
`Vl53l8ch` → 'ch'). `progress` receives phase strings;
`write_progress(done, total)` tracks the big blob writes.

#### Vl53l8cx.get_resolution

```python
get_resolution(self) -> int
```

Active zone count: 16 (4×4) or 64 (8×8).

#### Vl53l8cx.set_resolution

```python
set_resolution(self, zones: int) -> None
```

Select the zone grid: RESOLUTION_4X4 (16) or RESOLUTION_8X8 (64).
Not while ranging.

#### Vl53l8cx.get_ranging_frequency_hz

```python
get_ranging_frequency_hz(self) -> int
```

Configured ranging frequency in Hz.

#### Vl53l8cx.set_ranging_frequency_hz

```python
set_ranging_frequency_hz(self, hz: int) -> None
```

Set the ranging frequency in Hz. Not while ranging.

At least 2 Hz on the VL53L8 (below that the sensor never enters its
ranging loop and streams nothing, contract 04), 1 Hz on L5/L7; at
most 60 Hz at 4×4 and 15 Hz at 8×8 (contract 11 §3) — judged by the
resolution last set or read through this class (4×4 after init).
Refused host-side, nothing reaches the bus.

#### Vl53l8cx.get_ranging_mode

```python
get_ranging_mode(self) -> int
```

RANGING_MODE_CONTINUOUS or RANGING_MODE_AUTONOMOUS.

#### Vl53l8cx.set_ranging_mode

```python
set_ranging_mode(self, mode: int) -> None
```

Set CONTINUOUS (free-running) or AUTONOMOUS (integrate-then-idle)
ranging. Not while ranging.

#### Vl53l8cx.get_integration_time_ms

```python
get_integration_time_ms(self) -> int
```

Configured integration time in ms.

#### Vl53l8cx.set_integration_time_ms

```python
set_integration_time_ms(self, ms: int) -> None
```

Set the integration time, 2–1000 ms. Autonomous mode only (no
effect in continuous ranging). Not while ranging.

#### Vl53l8cx.get_sharpener_percent

```python
get_sharpener_percent(self) -> int
```

Configured edge-sharpener strength, 0–99 %.

#### Vl53l8cx.set_sharpener_percent

```python
set_sharpener_percent(self, pct: int) -> None
```

Set the edge sharpener, 0–99 % (0 disables). Not while ranging.

#### Vl53l8cx.get_target_order

```python
get_target_order(self) -> int
```

TARGET_ORDER_CLOSEST or TARGET_ORDER_STRONGEST.

#### Vl53l8cx.set_target_order

```python
set_target_order(self, order: int) -> None
```

Order multi-target zones by CLOSEST or STRONGEST return. Not while
ranging.

#### Vl53l8cx.get_power_mode

```python
get_power_mode(self) -> int
```

POWER_MODE_SLEEP/WAKEUP/DEEP_SLEEP (uld constants).

#### Vl53l8cx.set_power_mode

```python
set_power_mode(self, mode: int) -> None
```

Enter sleep / wake / deep-sleep. Not while ranging. Waking from
DEEP_SLEEP re-downloads the firmware blob (init()).

#### Vl53l8cx.get_xtalk_margin

```python
get_xtalk_margin(self) -> int
```

Xtalk margin, whole kcps/SPAD (as ST).

#### Vl53l8cx.set_xtalk_margin

```python
set_xtalk_margin(self, margin_kcps: int) -> None
```

Xtalk margin, a whole number of kcps/SPAD, 0..10000 (as ST).

#### Vl53l8cx.calibrate_xtalk

```python
calibrate_xtalk(self, reflectance_percent: int, nb_samples: int, distance_mm: int) -> None
```

Run on-device crosstalk calibration against a flat target at
`distance_mm` with the given `reflectance_percent` (1..99) averaging
`nb_samples` (1..16). The result is captured into the xtalk buffer;
read it back with get_caldata_xtalk(). Blocks several seconds.

#### Vl53l8cx.get_caldata_xtalk

```python
get_caldata_xtalk(self) -> bytes
```

Read back the 776-byte xtalk calibration blob (save/restore).

#### Vl53l8cx.set_caldata_xtalk

```python
set_caldata_xtalk(self, blob: bytes) -> None
```

Restore a previously saved 776-byte xtalk calibration blob.

#### Vl53l8cx.get_detection_thresholds_enable

```python
get_detection_thresholds_enable(self) -> int
```

#### Vl53l8cx.set_detection_thresholds_enable

```python
set_detection_thresholds_enable(self, enabled: bool) -> None
```

#### Vl53l8cx.get_detection_thresholds

```python
get_detection_thresholds(self) -> list[depz_sensor_sdk.vl53l8.uld.DetectionThreshold]
```

#### Vl53l8cx.set_detection_thresholds

```python
set_detection_thresholds(self, thresholds: list[depz_sensor_sdk.vl53l8.uld.DetectionThreshold]) -> None
```

Program the 64 detection thresholds (interrupt-on-threshold). Each
entry is a dict: low_thresh, high_thresh, measurement, type, zone_num,
operation (see uld THRESH_* constants).

#### Vl53l8cx.set_detection_thresholds_auto_stop

```python
set_detection_thresholds_auto_stop(self, auto_stop: bool) -> None
```

#### Vl53l8cx.configure_motion_indicator

```python
configure_motion_indicator(self, distance_min_mm: int = 400, distance_max_mm: int = 1500) -> depz_sensor_sdk.vl53l8.uld.MotionConfig
```

Enable the motion indicator over [distance_min_mm, distance_max_mm]
and surface motion output in each frame's `.motion`. Returns the
underlying uld MotionConfig for advanced tuning.

On CH firmware the motion indicator and CNH share one DCI struct
(0xBFAC): this disarms CNH (`cnh_config` → None, no `cnh_raw`).

#### Vl53l8cx.start_ranging

```python
start_ranging(self) -> None
```

Configure the output list, start the sensor and the MCU stream.

#### Vl53l8cx.stop_ranging

```python
stop_ranging(self) -> None
```

#### Vl53l8cx.ranging *(property)*

#### Vl53l8cx.reassembler_discards *(property)*

Chunked frames discarded by the reassembler (gaps / offset errors).
Distinct from `frames.parse_errors` (ULD parsing failed).

#### Vl53l8cx.close

```python
close(self) -> None
```

#### Vl53l8cx.port *(property)*

#### Vl53l8cx.closed *(property)*

#### Vl53l8cx.lost *(property)*

The loss that ended this device (its `__cause__` is what the link
raised), or None: still open, or ended by the program's own `close()`.
Set before `closed` shows the loss, so a pull iterator that has just
ended quietly tells a loss from its own close here (contract 07 §3).

#### Vl53l8cx.send

```python
send(self, cmd: int, payload: bytes = b'', *, crc_type: depz_sensor_sdk.transport.framing.CrcType | None = None) -> None
```

Fire-and-forget packet (escape hatch; prefer `request`).

#### Vl53l8cx.request

```python
request(self, cmd: int, payload: bytes = b'', *, matcher: Optional[Callable[[depz_sensor_sdk.transport.framing.Packet], Union[Any, depz_sensor_sdk.device._NoMatch]]] = None, ok_completes: bool = False, timeout: float | None = None) -> Any
```

Send `cmd` and wait for its correlated completion.

Exactly one of the completion paths must be configured:
- `ok_completes=True` — RPT_STATUS(cmd, OK) finishes with None;
- `matcher` — first packet for which `matcher(pkt) is not
  request.NO_MATCH` finishes with the matcher's return value.
Non-OK RPT_STATUS echoing `cmd` always raises (BusyError for
ERR_BUSY, StatusError otherwise); a matcher that raises on the packet
it claims (a reply shorter than its fields) raises InvalidReplyError
here, and the device stays usable. One in-flight request per opcode.

#### Vl53l8cx.expect_report

```python
expect_report(report_id: int, unpack: Callable[[bytes], ~T]) -> Callable[[depz_sensor_sdk.transport.framing.Packet], Union[~T, depz_sensor_sdk.device._NoMatch]]
```

Matcher for a typed report identified by its report ID alone.

#### Vl53l8cx.expect_text

```python
expect_text(request_cmd: int) -> Callable[[depz_sensor_sdk.transport.framing.Packet], Union[str, depz_sensor_sdk.device._NoMatch]]
```

Matcher for RPT_TEXT echoing `request_cmd`.

#### Vl53l8cx.get_device_name

```python
get_device_name(self) -> str
```

#### Vl53l8cx.get_software_name

```python
get_software_name(self) -> str
```

#### Vl53l8cx.get_serial_number

```python
get_serial_number(self) -> str
```

#### Vl53l8cx.identify

```python
identify(self) -> depz_sensor_sdk.protocol.identity.Identity
```

Classify the running firmware (contract 02 §4): the software name
parsed into mode, sensor type and version.

#### Vl53l8cx.read_mcu_temperature

```python
read_mcu_temperature(self) -> float
```

Last cached MCU temperature in °C (device refreshes ~2 Hz).

#### Vl53l8cx.sync_time

```python
sync_time(self, samples: int = 5) -> depz_sensor_sdk.device.TimeSync
```

NTP-style sync; keeps the lowest-RTT sample (contract 02 §5).

#### Vl53l8cx.time_sync *(property)*

#### Vl53l8cx.to_host_time_us

```python
to_host_time_us(self, device_timestamp_us: int) -> int
```

Device µs → host monotonic µs (requires a prior `sync_time`).

#### Vl53l8cx.get_report_payload_crc

```python
get_report_payload_crc(self) -> depz_sensor_sdk.transport.framing.CrcType
```

#### Vl53l8cx.set_report_payload_crc

```python
set_report_payload_crc(self, crc_type: depz_sensor_sdk.transport.framing.CrcType) -> None
```

Set the device→host payload CRC mode (host→device is per-packet).

#### Vl53l8cx.get_sync_pin

```python
get_sync_pin(self, pin: int) -> depz_sensor_sdk.protocol.common.SyncPinConfig
```

#### Vl53l8cx.set_sync_pin

```python
set_sync_pin(self, config: depz_sensor_sdk.protocol.common.SyncPinConfig) -> None
```

#### Vl53l8cx.reset

```python
reset(self) -> None
```

DEVICE_RESET: device ACKs then reboots; the link will drop.

#### Vl53l8cx.enter_bootloader_mode

```python
enter_bootloader_mode(self) -> None
```

Ask the device to reboot into the resident bootloader and close
this connection. Re-discovery/flash flow lives in the bootloader
module (contract 06).

### Vl53l8Frame

```python
class Vl53l8Frame(*, timestamp_us: int, resolution: int, distance_mm: NDArray[numpy.int32], target_status: NDArray[numpy.uint8], nb_target_detected: NDArray[numpy.uint8], signal_per_spad: NDArray[numpy.float64], ambient_per_spad: NDArray[numpy.float64], nb_spads_enabled: NDArray[numpy.int32], range_sigma_mm: NDArray[numpy.float64], reflectance: NDArray[numpy.float64], silicon_temp_degc: int, cnh_raw: bytes | None = None, motion: depz_sensor_sdk.vl53l8.uld.MotionIndicator | None = None) -> None
```

One parsed ranging frame. Arrays are sized to the active resolution
(16 or 64 zones); zone index runs row-major (see datasheet zone maps).

#### Vl53l8Frame.grid

```python
grid(self, field: str = 'distance_mm') -> NDArray[typing.Any]
```

Zone array reshaped to (4,4) or (8,8).

### Vl53l8cxError

```python
class Vl53l8cxError(code: int, where: str = '') -> None
```

#### Vl53l8cxError.with_traceback

```python
with_traceback
```

Exception.with_traceback(tb) --
set self.__traceback__ to tb and return self.

#### Vl53l8cxError.add_note

```python
add_note
```

Exception.add_note(note) --
add a note to the exception

### RESOLUTION_4X4

`int` constant.

int(x, base=10) -> integer

Convert a number or string to an integer, or return 0 if no arguments
are given.  If x is a number, return x.__int__().  For floating-point
numbers, this truncates towards zero.

If x is not a number or if base is given, then x must be a string,
bytes, or bytearray instance representing an integer literal in the
given base.  The literal can be preceded by '+' or '-' and be surrounded
by whitespace.  The base defaults to 10.  Valid bases are 0 and 2-36.
Base 0 means to interpret the base from the string as an integer literal.
>>> int('0b100', base=0)
4

### RESOLUTION_8X8

`int` constant.

int(x, base=10) -> integer

Convert a number or string to an integer, or return 0 if no arguments
are given.  If x is a number, return x.__int__().  For floating-point
numbers, this truncates towards zero.

If x is not a number or if base is given, then x must be a string,
bytes, or bytearray instance representing an integer literal in the
given base.  The literal can be preceded by '+' or '-' and be surrounded
by whitespace.  The base defaults to 10.  Valid bases are 0 and 2-36.
Base 0 means to interpret the base from the string as an integer literal.
>>> int('0b100', base=0)
4
