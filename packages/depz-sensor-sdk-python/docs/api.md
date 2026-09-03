# API reference

Auto-generated from the package's public surface (the union of
`__all__` across `depz_sensor_sdk` and its subpackages) by
`scripts/gen_api_md.py` — run `make docs` to regenerate. Edit the
docstrings in the source, not this file.

Each sensor also has a focused reference with just its own symbols:
[SR04](sr04/api.md) · [VL53L4CD](vl53l4cd/api.md) · [VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · [BNO086](bno086/api.md).

## Contents

- **Discovery**: [`DeviceInfo`](#deviceinfo), [`list_depz_devices`](#list_depz_devices), [`probe_port`](#probe_port), [`open_device`](#open_device)
- **Device core**: [`DeviceBase`](#devicebase), [`TimeSync`](#timesync), [`LinkStats`](#linkstats), [`StreamQueue`](#streamqueue), [`host_now_us`](#host_now_us), [`sync_time_all`](#sync_time_all), [`DeviceEvent`](#deviceevent), [`SequenceErrorEvent`](#sequenceerrorevent), [`LinkCrcErrorEvent`](#linkcrcerrorevent), [`TrashEvent`](#trashevent), [`UnsolicitedStatusEvent`](#unsolicitedstatusevent), [`TextEvent`](#textevent), [`TemperatureEvent`](#temperatureevent), [`DisconnectedEvent`](#disconnectedevent), [`StreamIterator`](#streamiterator)
- **SR04**: [`Sr04`](#sr04), [`Sr04Measurement`](#sr04measurement)
- **VL53L4CD (ToF)**: [`Vl53l4Cd`](#vl53l4cd), [`Vl53l4Measurement`](#vl53l4measurement), [`Vl53l4cdError`](#vl53l4cderror), [`MODEL_ID_VL53L4CD`](#model_id_vl53l4cd), [`RANGE_STATUS_NAMES`](#range_status_names), [`WINDOW_ABOVE`](#window_above), [`WINDOW_OUT`](#window_out), [`WINDOW_IN`](#window_in), [`I2C_KHZ_BOOT`](#i2c_khz_boot), [`I2C_KHZ_DEFAULT`](#i2c_khz_default), [`I2C_KHZ_STEPS`](#i2c_khz_steps)
- **VL53L8 (ToF)**: [`Vl53l8`](#vl53l8), [`Vl53l8Ch`](#vl53l8ch), [`Vl53l8Frame`](#vl53l8frame), [`CnhConfig`](#cnhconfig), [`Vl53l8cxError`](#vl53l8cxerror), [`RESOLUTION_4X4`](#resolution_4x4), [`RESOLUTION_8X8`](#resolution_8x8)
- **BNO086 (IMU)**: [`Bno086`](#bno086), [`SensorId`](#sensorid), [`RotationVector`](#rotationvector), [`GyroIntegratedRV`](#gyrointegratedrv), [`Report`](#report), [`InputReport`](#inputreport), [`Acceleration`](#acceleration), [`Gyroscope`](#gyroscope), [`Magnetometer`](#magnetometer), [`FeatureResponse`](#featureresponse), [`ProductId`](#productid), [`CommandResponse`](#commandresponse), [`SensorMetadata`](#sensormetadata), [`TareAxis`](#tareaxis), [`TareBasis`](#tarebasis), [`Sh2Error`](#sh2error), [`CalibrationConfig`](#calibrationconfig), [`OscillatorType`](#oscillatortype), [`ErrorRecord`](#errorrecord), [`ErrorSource`](#errorsource), [`Counts`](#counts)
- **Bootloader / firmware update**: [`BootloaderDevice`](#bootloaderdevice), [`update_firmware`](#update_firmware), [`find_bootloader_port`](#find_bootloader_port)
- **Datasets (record & replay)**: [`SessionRecorder`](#sessionrecorder), [`DatasetReader`](#datasetreader), [`DatasetRecord`](#datasetrecord)
- **Transport**: [`MAGIC`](#magic), [`HEADER_SIZE`](#header_size), [`MAX_PAYLOAD`](#max_payload), [`CrcType`](#crctype), [`Packet`](#packet), [`Trash`](#trash), [`CrcError`](#crcerror), [`ParserEvent`](#parserevent), [`PacketParser`](#packetparser), [`build_packet`](#build_packet), [`payload_crc_bytes`](#payload_crc_bytes), [`crc8_maxim`](#crc8_maxim), [`crc16_modbus`](#crc16_modbus), [`crc32_iso_hdlc`](#crc32_iso_hdlc), [`crc16_ccitt_false`](#crc16_ccitt_false), [`Link`](#link), [`LoopbackLink`](#loopbacklink)
- **Protocol codecs**: [`Vl53l4Info`](#vl53l4info), [`FwDepzImage`](#fwdepzimage), [`Cmd`](#cmd), [`Rpt`](#rpt), [`Status`](#status), [`SyncPinMode`](#syncpinmode), [`SyncPinPolarity`](#syncpinpolarity), [`SyncPinConfig`](#syncpinconfig), [`StatusReport`](#statusreport), [`TextReport`](#textreport), [`SyncTimeReport`](#synctimereport), [`TemperatureReport`](#temperaturereport), [`SequenceErrorReport`](#sequenceerrorreport), [`UNSOLICITED`](#unsolicited), [`pack_sync_time`](#pack_sync_time), [`sync_time_offset_rtt`](#sync_time_offset_rtt), [`strip_device_string`](#strip_device_string), [`Identity`](#identity), [`SensorType`](#sensortype), [`parse_software_name`](#parse_software_name)
- **Errors**: [`DepzError`](#depzerror), [`DepzTimeoutError`](#depztimeouterror), [`StatusError`](#statuserror), [`BusyError`](#busyerror), [`DeviceLostError`](#devicelosterror), [`LinkClosedError`](#linkclosederror), [`NoDepzDeviceError`](#nodepzdeviceerror)
- **Top level**: [`DEPZ_USB_VID`](#depz_usb_vid), [`DEPZ_PID_MODEL`](#depz_pid_model), [`__version__`](#__version__)
- **Usb_Ids**: [`is_known_depz_usb`](#is_known_depz_usb), [`usb_model_hint`](#usb_model_hint)

## Discovery

### DeviceInfo

```python
class DeviceInfo(port: str, mode: Literal['app', 'bootloader', 'unknown'], sensor_type: depz_sensor_sdk.protocol.identity.SensorType | None, software_name: str, fw_version: str, device_name: str, serial_number: str, usb_vid: int | None = None, usb_pid: int | None = None, usb_serial: str | None = None) -> None
```

#### DeviceInfo.usb_model_hint *(property)*

### list_depz_devices

```python
list_depz_devices(*, ports: list[str] | None = None, timeout: float = 0.2, match_usb: bool = True) -> list[depz_sensor_sdk.discovery.DeviceInfo]
```

Probe candidate serial ports and return every DEPZ device found.

With `match_usb` (default) only ports whose USB (vid, pid) is a known DEPZ
identity are probed — fast, and it avoids poking unrelated devices. Pass
`match_usb=False` for the legacy probe-every-port behavior. `ports`
overrides enumeration with an explicit list (still USB-annotated when the
OS knows the port). Results are ordered by USB iSerial.

### probe_port

```python
probe_port(port: str, *, timeout: float = 0.2) -> depz_sensor_sdk.discovery.DeviceInfo | None
```

Open `port`, ask GET_NAME_ACTIVE_SOFTWARE (+ name/serial), close.

Returns None when nothing DEPZ-shaped answers. Note: probing opens the
port — skip ports owned by other software via the `ports=` argument of
`list_depz_devices`.

### open_device

```python
open_device(target: str | int | depz_sensor_sdk.discovery.DeviceInfo | depz_sensor_sdk.transport.link.Link | None = None, *, serial: str | None = None, timeout: float = 0.2)
```

Open the right sensor class for `target`.

- ``None`` (default): the DEPZ candidate port with the alphabetically
  smallest USB serial (index 0); with ``serial=`` the candidate whose USB
  serial matches. No candidate → `NoDepzDeviceError`.
- ``int N``: the Nth DEPZ candidate, candidates sorted by USB serial
  (0-based). Out of range → `NoDepzDeviceError`.
- ``str`` (a port path): open exactly that port; if its USB (vid, pid) is
  not a known DEPZ id, warn but proceed.
- ``DeviceInfo``: its ``.port``.
- ``Link``/transport: opened directly (loopback/replay).

Bootloader-mode devices raise until the bootloader client lands
(contract 06).

## Device core

### DeviceBase

```python
class DeviceBase(port_or_link: str | depz_sensor_sdk.transport.link.Link, *, timeout: float = 0.2, tx_crc_type: depz_sensor_sdk.transport.framing.CrcType = <CrcType.NONE: 0>)
```

Connection to one DEPZ device in application mode.

Accepts a port name or any `Link` (loopback/replay for tests). Starts a
reader thread on construction; use as a context manager or call
`close()`.

#### DeviceBase.close

```python
close(self) -> None
```

#### DeviceBase.port *(property)*

#### DeviceBase.closed *(property)*

#### DeviceBase.on_event

```python
on_event(self, cb: Callable[[depz_sensor_sdk.device.DeviceEvent], NoneType]) -> Callable[[], NoneType]
```

Subscribe to unsolicited/diagnostic events (reader-thread context;
do not block). Returns an unsubscribe function.

#### DeviceBase.events

```python
events(self, maxsize: int = 256) -> depz_sensor_sdk.device.StreamIterator
```

Pull-style event stream (bounded, drop-oldest). Subscribes
immediately — events emitted after this call are never missed.

#### DeviceBase.send

```python
send(self, cmd: int, payload: bytes = b'', *, crc_type: depz_sensor_sdk.transport.framing.CrcType | None = None) -> None
```

Fire-and-forget packet (escape hatch; prefer `request`).

#### DeviceBase.request

```python
request(self, cmd: int, payload: bytes = b'', *, matcher: Optional[Callable[[depz_sensor_sdk.transport.framing.Packet], Any]] = None, ok_completes: bool = False, timeout: float | None = None) -> Any
```

Send `cmd` and wait for its correlated completion.

Exactly one of the completion paths must be configured:
- `ok_completes=True` — RPT_STATUS(cmd, OK) finishes with None;
- `matcher` — first packet for which `matcher(pkt) is not
  request.NO_MATCH` finishes with the matcher's return value.
Non-OK RPT_STATUS echoing `cmd` always raises (BusyError for
ERR_BUSY, StatusError otherwise). One in-flight request per opcode.

#### DeviceBase.expect_report

```python
expect_report(report_id: int, unpack: Callable[[bytes], Any]) -> Callable[[depz_sensor_sdk.transport.framing.Packet], Any]
```

Matcher for a typed report identified by its report ID alone.

#### DeviceBase.expect_text

```python
expect_text(request_cmd: int) -> Callable[[depz_sensor_sdk.transport.framing.Packet], Any]
```

Matcher for RPT_TEXT echoing `request_cmd`.

#### DeviceBase.get_device_name

```python
get_device_name(self) -> str
```

#### DeviceBase.get_software_name

```python
get_software_name(self) -> str
```

#### DeviceBase.get_serial_number

```python
get_serial_number(self) -> str
```

#### DeviceBase.read_mcu_temperature

```python
read_mcu_temperature(self) -> float
```

Last cached MCU temperature in °C (device refreshes ~2 Hz).

#### DeviceBase.sync_time

```python
sync_time(self, samples: int = 5) -> depz_sensor_sdk.device.TimeSync
```

NTP-style sync; keeps the lowest-RTT sample (contract 02 §5).

#### DeviceBase.time_sync *(property)*

#### DeviceBase.to_host_time_us

```python
to_host_time_us(self, device_timestamp_us: int) -> int
```

Device µs → host monotonic µs (requires a prior `sync_time`).

#### DeviceBase.get_report_payload_crc

```python
get_report_payload_crc(self) -> depz_sensor_sdk.transport.framing.CrcType
```

#### DeviceBase.set_report_payload_crc

```python
set_report_payload_crc(self, crc_type: depz_sensor_sdk.transport.framing.CrcType) -> None
```

Set the device→host payload CRC mode (host→device is per-packet).

#### DeviceBase.get_sync_pin

```python
get_sync_pin(self, pin: int) -> depz_sensor_sdk.protocol.common.SyncPinConfig
```

#### DeviceBase.set_sync_pin

```python
set_sync_pin(self, config: depz_sensor_sdk.protocol.common.SyncPinConfig) -> None
```

#### DeviceBase.reset

```python
reset(self) -> None
```

DEVICE_RESET: device ACKs then reboots; the link will drop.

#### DeviceBase.enter_bootloader_mode

```python
enter_bootloader_mode(self) -> None
```

Ask the device to reboot into the resident bootloader and close
this connection. Re-discovery/flash flow lives in the bootloader
module (contract 06).

### TimeSync

```python
class TimeSync(offset_us: int, rtt_us: int, synced_at_host_us: int) -> None
```

### LinkStats

```python
class LinkStats(tx_packets: int = 0, rx_packets: int = 0, tx_bytes: int = 0, rx_bytes: int = 0, crc_errors: int = 0, header_errors: int = 0, trash_bytes: int = 0, seq_gaps: int = 0, device_seq_errors: int = 0) -> None
```

### StreamQueue

```python
class StreamQueue(maxsize: int)
```

Bounded drop-oldest queue with a drop counter (contract 07 §3).

#### StreamQueue.put

```python
put(self, item: Any) -> None
```

#### StreamQueue.get

```python
get(self, timeout: float | None = None) -> Any
```

### host_now_us

```python
host_now_us() -> int
```

Monotonic host clock in µs — the host side of all time-sync math.

### sync_time_all

```python
sync_time_all(devices: 'Iterable[DeviceBase]', samples: int = 5) -> 'dict[DeviceBase, TimeSync]'
```

Software-sync several devices to the common host monotonic clock.

Runs `sync_time()` on each device so that every device's
`to_host_time_us()` maps its own timestamps onto ONE shared host timeline —
the basis for correlating reports from multiple sensors live (the same
alignment SessionRecorder uses for recordings). Returns {device: TimeSync}.

### DeviceEvent

```python
class DeviceEvent() -> None
```

Base for unsolicited/diagnostic events (contract 07 §2).

### SequenceErrorEvent

```python
class SequenceErrorEvent(expected_seq: int, received_seq: int, reported_by_device: bool) -> None
```

### LinkCrcErrorEvent

```python
class LinkCrcErrorEvent(cmd: int, seq: int) -> None
```

### TrashEvent

```python
class TrashEvent(data: bytes) -> None
```

### UnsolicitedStatusEvent

```python
class UnsolicitedStatusEvent(status: int) -> None
```

#### UnsolicitedStatusEvent.is_hardware_fault *(property)*

### TextEvent

```python
class TextEvent(cmd: int, text: str) -> None
```

### TemperatureEvent

```python
class TemperatureEvent(timestamp_us: int, celsius: float) -> None
```

### DisconnectedEvent

```python
class DisconnectedEvent(reason: str) -> None
```

### StreamIterator

```python
class StreamIterator(registry: list['StreamQueue'], maxsize: int, closed_fn)
```

Iterator over a StreamQueue that registers **eagerly** at construction.

A lazy generator would subscribe only on the first `next()`, losing
everything emitted in between (a real race on instant replay links).
Ends when the device closes; deregisters on GC/close().

#### StreamIterator.dropped_count *(property)*

#### StreamIterator.close

```python
close(self) -> None
```

## SR04

### Sr04

```python
class Sr04(port_or_link: str | depz_sensor_sdk.transport.link.Link, *, timeout: float = 0.2, tx_crc_type: depz_sensor_sdk.transport.framing.CrcType = <CrcType.NONE: 0>)
```

HC-SR04 ultrasonic ranging device.

Measurements stream via callbacks (`on_measurement`) and/or the pull
iterator (`stream()`); both receive loop samples *and* unsolicited
single shots triggered by an AUX SYNC_IN edge.

#### Sr04.get_sample_period_us

```python
get_sample_period_us(self) -> int
```

Configured minimum interval between measurement starts, in µs
(default 50000). This is the stored value, not the effective rate.

#### Sr04.set_sample_period_us

```python
set_sample_period_us(self, period_us: int) -> None
```

Set the minimum interval between measurement starts, in µs.

The effective rate is auto-throttled by the echo window (contract 03
§3) — reading back returns the stored value, not the effective one.

Raises `ValueError` when `period_us` does not fit the u32 wire field.

#### Sr04.get_echo_decay_us

```python
get_echo_decay_us(self) -> int
```

Configured post-echo settle pause, in µs (4000–65000).

#### Sr04.set_echo_decay_us

```python
set_echo_decay_us(self, decay_us: int) -> int
```

Set the settle pause; the device clamps to 4000–65000 µs silently,
so this re-reads and returns the value actually in effect.

Values inside the u16 wire field but outside the clamp window are
legal to send — the device clamps them, which is why this re-reads.
A value that does not fit the wire field at all is a caller bug and
raises `ValueError` (rather than letting `struct.error` escape from
the codec).

#### Sr04.measure_once

```python
measure_once(self, timeout: float = 1.0) -> depz_sensor_sdk.sr04.Sr04Measurement
```

Single shot. Raises BusyError while the loop is running. The reply
arrives only when the echo completes (or times out at ~65.5 ms), so
the default timeout is generous.

#### Sr04.start

```python
start(self) -> None
```

Start the measurement loop (idempotent).

#### Sr04.stop

```python
stop(self) -> None
```

Stop the measurement loop (idempotent).

#### Sr04.on_measurement

```python
on_measurement(self, cb: Callable[[depz_sensor_sdk.sr04.Sr04Measurement], NoneType]) -> Callable[[], NoneType]
```

Subscribe to measurements (reader-thread context; don't block).
Returns an unsubscribe function.

#### Sr04.stream

```python
stream(self, maxsize: int = 256) -> depz_sensor_sdk.device.StreamIterator
```

Blocking iterator over measurements (bounded, drop-oldest;
`dropped_count` on the returned iterator). Subscribes immediately.

#### Sr04.stream_dropped_counts *(property)*

#### Sr04.close

```python
close(self) -> None
```

#### Sr04.port *(property)*

#### Sr04.closed *(property)*

#### Sr04.on_event

```python
on_event(self, cb: Callable[[depz_sensor_sdk.device.DeviceEvent], NoneType]) -> Callable[[], NoneType]
```

Subscribe to unsolicited/diagnostic events (reader-thread context;
do not block). Returns an unsubscribe function.

#### Sr04.events

```python
events(self, maxsize: int = 256) -> depz_sensor_sdk.device.StreamIterator
```

Pull-style event stream (bounded, drop-oldest). Subscribes
immediately — events emitted after this call are never missed.

#### Sr04.send

```python
send(self, cmd: int, payload: bytes = b'', *, crc_type: depz_sensor_sdk.transport.framing.CrcType | None = None) -> None
```

Fire-and-forget packet (escape hatch; prefer `request`).

#### Sr04.request

```python
request(self, cmd: int, payload: bytes = b'', *, matcher: Optional[Callable[[depz_sensor_sdk.transport.framing.Packet], Any]] = None, ok_completes: bool = False, timeout: float | None = None) -> Any
```

Send `cmd` and wait for its correlated completion.

Exactly one of the completion paths must be configured:
- `ok_completes=True` — RPT_STATUS(cmd, OK) finishes with None;
- `matcher` — first packet for which `matcher(pkt) is not
  request.NO_MATCH` finishes with the matcher's return value.
Non-OK RPT_STATUS echoing `cmd` always raises (BusyError for
ERR_BUSY, StatusError otherwise). One in-flight request per opcode.

#### Sr04.expect_report

```python
expect_report(report_id: int, unpack: Callable[[bytes], Any]) -> Callable[[depz_sensor_sdk.transport.framing.Packet], Any]
```

Matcher for a typed report identified by its report ID alone.

#### Sr04.expect_text

```python
expect_text(request_cmd: int) -> Callable[[depz_sensor_sdk.transport.framing.Packet], Any]
```

Matcher for RPT_TEXT echoing `request_cmd`.

#### Sr04.get_device_name

```python
get_device_name(self) -> str
```

#### Sr04.get_software_name

```python
get_software_name(self) -> str
```

#### Sr04.get_serial_number

```python
get_serial_number(self) -> str
```

#### Sr04.read_mcu_temperature

```python
read_mcu_temperature(self) -> float
```

Last cached MCU temperature in °C (device refreshes ~2 Hz).

#### Sr04.sync_time

```python
sync_time(self, samples: int = 5) -> depz_sensor_sdk.device.TimeSync
```

NTP-style sync; keeps the lowest-RTT sample (contract 02 §5).

#### Sr04.time_sync *(property)*

#### Sr04.to_host_time_us

```python
to_host_time_us(self, device_timestamp_us: int) -> int
```

Device µs → host monotonic µs (requires a prior `sync_time`).

#### Sr04.get_report_payload_crc

```python
get_report_payload_crc(self) -> depz_sensor_sdk.transport.framing.CrcType
```

#### Sr04.set_report_payload_crc

```python
set_report_payload_crc(self, crc_type: depz_sensor_sdk.transport.framing.CrcType) -> None
```

Set the device→host payload CRC mode (host→device is per-packet).

#### Sr04.get_sync_pin

```python
get_sync_pin(self, pin: int) -> depz_sensor_sdk.protocol.common.SyncPinConfig
```

#### Sr04.set_sync_pin

```python
set_sync_pin(self, config: depz_sensor_sdk.protocol.common.SyncPinConfig) -> None
```

#### Sr04.reset

```python
reset(self) -> None
```

DEVICE_RESET: device ACKs then reboots; the link will drop.

#### Sr04.enter_bootloader_mode

```python
enter_bootloader_mode(self) -> None
```

Ask the device to reboot into the resident bootloader and close
this connection. Re-discovery/flash flow lives in the bootloader
module (contract 06).

### Sr04Measurement

```python
class Sr04Measurement(timestamp_us: int, echo_time_us: int, source: str) -> None
```

One ranging result. `valid` is False for the no-echo timeout.

#### Sr04Measurement.valid *(property)*

#### Sr04Measurement.distance_mm *(property)*

Distance at 343 m/s, or None when no echo was received.

#### Sr04Measurement.distance_mm_at

```python
distance_mm_at(self, air_temp_c: float) -> float | None
```

Distance with temperature-compensated speed of sound.

## VL53L4CD (ToF)

### Vl53l4Cd

```python
class Vl53l4Cd(port_or_link: str | depz_sensor_sdk.transport.link.Link, *, timeout: float = 0.2, tx_crc_type: depz_sensor_sdk.transport.framing.CrcType = <CrcType.NONE: 0>)
```

VL53L4CD single-zone ToF device.

`init()` runs the ULD boot sequence (no firmware blob — the sensor carries
its own), then configure and `start_ranging()`. Configuration methods must
not be called while ranging: the INT-driven stream owns the register bank
(contract 10). Measurements stream via callbacks (`on_measurement`) and/or
the pull iterator (`measurements()`).

#### Vl53l4Cd.uld *(property)*

The underlying ULD driver (escape hatch for raw register access).

#### Vl53l4Cd.initialized *(property)*

True after a successful init(). Cleared by reset_sensor() and
xshut() — a power-cycled sensor holds none of the ULD configuration.

#### Vl53l4Cd.is_alive

```python
is_alive(self) -> bool
```

True when the sensor answers with the VL53L4CD model id (0xEBAA).

#### Vl53l4Cd.init

```python
init(self, bus_khz: int = 1000) -> None
```

Initialise the sensor: default configuration block + VHV calibration
(ULD sensor_init). Takes well under a second; the bus is left at
`bus_khz` (one of I2C_KHZ_STEPS).

#### Vl53l4Cd.xshut

```python
xshut(self, action: int) -> None
```

Drive the XSHUT pin: XSHUT_OFF / XSHUT_ON / XSHUT_RESET. OFF and
RESET stop any active stream on the bridge; a power-cycled sensor
needs init() again.

#### Vl53l4Cd.reset_sensor

```python
reset_sensor(self) -> None
```

Hardware sensor reset via XSHUT (blocks ~3 ms on the MCU). The ULD
configuration is wiped — call init() again.

#### Vl53l4Cd.bridge_info

```python
bridge_info(self) -> depz_sensor_sdk.protocol.vl53l4.Vl53l4Info
```

RPT_VL53_INFO: sensor identity, pin levels and bridge counters.
Counters are free-running (wrap silently) — watch increments. Safe to
call while streaming.

#### Vl53l4Cd.set_i2c_speed_khz

```python
set_i2c_speed_khz(self, khz: int) -> None
```

Re-time the bridge's I2C bus to the nominal step nearest `khz`
(I2C_KHZ_STEPS). Not while ranging — re-timing refuses a transfer in
flight (ERR_BUSY). Read back the programmed step via bridge_info().

#### Vl53l4Cd.get_range_timing

```python
get_range_timing(self) -> tuple[int, int]
```

→ (timing_budget_ms, inter_measurement_ms). inter_measurement 0
means continuous mode.

#### Vl53l4Cd.set_range_timing

```python
set_range_timing(self, timing_budget_ms: int, inter_measurement_ms: int = 0) -> None
```

Set the timing budget (10–200 ms) and inter-measurement period.
`inter_measurement_ms=0` selects continuous ranging; a value larger
than the budget selects autonomous low-power mode. Not while ranging.

#### Vl53l4Cd.get_offset_mm

```python
get_offset_mm(self) -> int
```

Configured ranging offset in mm (signed).

#### Vl53l4Cd.set_offset_mm

```python
set_offset_mm(self, offset_mm: int) -> None
```

Set the ranging offset correction in mm. Not while ranging.

#### Vl53l4Cd.get_xtalk_kcps

```python
get_xtalk_kcps(self) -> int
```

Configured crosstalk compensation in kcps (0 = disabled).

#### Vl53l4Cd.set_xtalk_kcps

```python
set_xtalk_kcps(self, xtalk_kcps: int) -> None
```

Set the crosstalk compensation in kcps. Not while ranging.

#### Vl53l4Cd.get_detection_thresholds

```python
get_detection_thresholds(self) -> tuple[int, int, int]
```

→ (distance_low_mm, distance_high_mm, window). Window is one of
WINDOW_BELOW / WINDOW_ABOVE / WINDOW_OUT / WINDOW_IN.

#### Vl53l4Cd.set_detection_thresholds

```python
set_detection_thresholds(self, distance_low_mm: int, distance_high_mm: int, window: int) -> None
```

Program the distance-window interrupt (INT only fires when the
window condition holds). Not while ranging.

#### Vl53l4Cd.get_signal_threshold_kcps

```python
get_signal_threshold_kcps(self) -> int
```

#### Vl53l4Cd.set_signal_threshold_kcps

```python
set_signal_threshold_kcps(self, signal_kcps: int) -> None
```

Discard measurements whose return signal is below `signal_kcps`.
Not while ranging.

#### Vl53l4Cd.get_sigma_threshold_mm

```python
get_sigma_threshold_mm(self) -> int
```

#### Vl53l4Cd.set_sigma_threshold_mm

```python
set_sigma_threshold_mm(self, sigma_mm: int) -> None
```

Discard measurements whose sigma exceeds `sigma_mm` (≤ 16383).
Not while ranging.

#### Vl53l4Cd.start_temperature_update

```python
start_temperature_update(self) -> None
```

Re-run VHV calibration; recommended after a >8 °C ambient change.
Not while ranging (runs a short ranging burst internally).

#### Vl53l4Cd.calibrate_offset

```python
calibrate_offset(self, target_dist_mm: int, nb_samples: int = 20) -> int
```

Offset calibration against a target at `target_dist_mm` (10–1000).
Blocks for the sample burst; returns the offset now programmed.

#### Vl53l4Cd.calibrate_xtalk

```python
calibrate_xtalk(self, target_dist_mm: int, nb_samples: int = 20) -> int
```

Crosstalk calibration against a target at `target_dist_mm` (10–5000).
Blocks for the sample burst; returns the xtalk now programmed (kcps).

#### Vl53l4Cd.start_ranging

```python
start_ranging(self) -> None
```

Start the sensor's ranging loop and arm the MCU stream: one
RPT_VL53_STREAM per INT edge carrying the 17-byte result block.

#### Vl53l4Cd.stop_ranging

```python
stop_ranging(self) -> None
```

#### Vl53l4Cd.ranging *(property)*

#### Vl53l4Cd.measure_once

```python
measure_once(self, timeout: float = 1.0) -> depz_sensor_sdk.vl53l4.Vl53l4Measurement
```

Single poll-mode measurement: start ranging, wait for data-ready,
read the result block, stop. Raises while the stream is running.

#### Vl53l4Cd.on_measurement

```python
on_measurement(self, cb: Callable[[depz_sensor_sdk.vl53l4.Vl53l4Measurement], NoneType]) -> Callable[[], NoneType]
```

Subscribe to streamed measurements (reader-thread context; don't
block). Returns an unsubscribe function.

#### Vl53l4Cd.measurements

```python
measurements(self, maxsize: int = 64) -> depz_sensor_sdk.device.StreamIterator
```

Blocking iterator over measurements (bounded, drop-oldest;
`dropped_count` on the returned iterator). Subscribes immediately.

#### Vl53l4Cd.get_measurement

```python
get_measurement(self, timeout: float = 2.0) -> depz_sensor_sdk.vl53l4.Vl53l4Measurement
```

Convenience: wait for the next streamed measurement.

Raises `DepzTimeoutError` when nothing arrives within `timeout`, and
`LinkClosedError` as soon as the device is closed while waiting.

#### Vl53l4Cd.stream_parse_errors *(property)*

Stream reports dropped because the result block failed to decode
(short block from a reconfigured stream, corrupt read).

#### Vl53l4Cd.stream_dropped_counts *(property)*

#### Vl53l4Cd.close

```python
close(self) -> None
```

#### Vl53l4Cd.port *(property)*

#### Vl53l4Cd.closed *(property)*

#### Vl53l4Cd.on_event

```python
on_event(self, cb: Callable[[depz_sensor_sdk.device.DeviceEvent], NoneType]) -> Callable[[], NoneType]
```

Subscribe to unsolicited/diagnostic events (reader-thread context;
do not block). Returns an unsubscribe function.

#### Vl53l4Cd.events

```python
events(self, maxsize: int = 256) -> depz_sensor_sdk.device.StreamIterator
```

Pull-style event stream (bounded, drop-oldest). Subscribes
immediately — events emitted after this call are never missed.

#### Vl53l4Cd.send

```python
send(self, cmd: int, payload: bytes = b'', *, crc_type: depz_sensor_sdk.transport.framing.CrcType | None = None) -> None
```

Fire-and-forget packet (escape hatch; prefer `request`).

#### Vl53l4Cd.request

```python
request(self, cmd: int, payload: bytes = b'', *, matcher: Optional[Callable[[depz_sensor_sdk.transport.framing.Packet], Any]] = None, ok_completes: bool = False, timeout: float | None = None) -> Any
```

Send `cmd` and wait for its correlated completion.

Exactly one of the completion paths must be configured:
- `ok_completes=True` — RPT_STATUS(cmd, OK) finishes with None;
- `matcher` — first packet for which `matcher(pkt) is not
  request.NO_MATCH` finishes with the matcher's return value.
Non-OK RPT_STATUS echoing `cmd` always raises (BusyError for
ERR_BUSY, StatusError otherwise). One in-flight request per opcode.

#### Vl53l4Cd.expect_report

```python
expect_report(report_id: int, unpack: Callable[[bytes], Any]) -> Callable[[depz_sensor_sdk.transport.framing.Packet], Any]
```

Matcher for a typed report identified by its report ID alone.

#### Vl53l4Cd.expect_text

```python
expect_text(request_cmd: int) -> Callable[[depz_sensor_sdk.transport.framing.Packet], Any]
```

Matcher for RPT_TEXT echoing `request_cmd`.

#### Vl53l4Cd.get_device_name

```python
get_device_name(self) -> str
```

#### Vl53l4Cd.get_software_name

```python
get_software_name(self) -> str
```

#### Vl53l4Cd.get_serial_number

```python
get_serial_number(self) -> str
```

#### Vl53l4Cd.read_mcu_temperature

```python
read_mcu_temperature(self) -> float
```

Last cached MCU temperature in °C (device refreshes ~2 Hz).

#### Vl53l4Cd.sync_time

```python
sync_time(self, samples: int = 5) -> depz_sensor_sdk.device.TimeSync
```

NTP-style sync; keeps the lowest-RTT sample (contract 02 §5).

#### Vl53l4Cd.time_sync *(property)*

#### Vl53l4Cd.to_host_time_us

```python
to_host_time_us(self, device_timestamp_us: int) -> int
```

Device µs → host monotonic µs (requires a prior `sync_time`).

#### Vl53l4Cd.get_report_payload_crc

```python
get_report_payload_crc(self) -> depz_sensor_sdk.transport.framing.CrcType
```

#### Vl53l4Cd.set_report_payload_crc

```python
set_report_payload_crc(self, crc_type: depz_sensor_sdk.transport.framing.CrcType) -> None
```

Set the device→host payload CRC mode (host→device is per-packet).

#### Vl53l4Cd.get_sync_pin

```python
get_sync_pin(self, pin: int) -> depz_sensor_sdk.protocol.common.SyncPinConfig
```

#### Vl53l4Cd.set_sync_pin

```python
set_sync_pin(self, config: depz_sensor_sdk.protocol.common.SyncPinConfig) -> None
```

#### Vl53l4Cd.reset

```python
reset(self) -> None
```

DEVICE_RESET: device ACKs then reboots; the link will drop.

#### Vl53l4Cd.enter_bootloader_mode

```python
enter_bootloader_mode(self) -> None
```

Ask the device to reboot into the resident bootloader and close
this connection. Re-discovery/flash flow lives in the bootloader
module (contract 06).

### Vl53l4Measurement

```python
class Vl53l4Measurement(timestamp_us: int, range_status: int, distance_mm: int, sigma_mm: int, signal_rate_kcps: int, ambient_rate_kcps: int, signal_per_spad_kcps: int, ambient_per_spad_kcps: int, number_of_spad: int, stream_count: int) -> None
```

One decoded ranging result (VL53L4CD_ResultsData_t + MCU timestamp).

#### Vl53l4Measurement.valid *(property)*

#### Vl53l4Measurement.status_text *(property)*

### Vl53l4cdError

```python
class Vl53l4cdError
```

#### Vl53l4cdError.with_traceback

```python
with_traceback
```

Exception.with_traceback(tb) --
set self.__traceback__ to tb and return self.

#### Vl53l4cdError.add_note

```python
add_note
```

Exception.add_note(note) --
add a note to the exception

### MODEL_ID_VL53L4CD

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

### RANGE_STATUS_NAMES

`dict` constant.

dict(mapping) -> new dictionary initialized from a mapping object's
    (key, value) pairs
dict(iterable) -> new dictionary initialized as if via:
    d = {}
    for k, v in iterable:
        d[k] = v
dict(**kwargs) -> new dictionary initialized with the name=value pairs
    in the keyword argument list.  For example:  dict(one=1, two=2)

### WINDOW_ABOVE

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

### WINDOW_OUT

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

### WINDOW_IN

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

### I2C_KHZ_BOOT

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

### I2C_KHZ_DEFAULT

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

### I2C_KHZ_STEPS

`tuple` constant.

Built-in immutable sequence.

If no argument is given, the constructor returns an empty tuple.
If iterable is specified the tuple is initialized from iterable's items.

If the argument is a tuple, the return value is the same object.

## VL53L8 (ToF)

### Vl53l8

```python
class Vl53l8(port_or_link: str | depz_sensor_sdk.transport.link.Link, *, timeout: float = 0.2, tx_crc_type: depz_sensor_sdk.transport.framing.CrcType = <CrcType.NONE: 0>)
```

VL53L8CX ToF device: `init()` downloads the ~84 KB sensor firmware
(~1 s over the CDC link), then configure and `start_ranging()`.

This is the base class for both silicon variants. The VL53L8CH superset
(compact-network-histogram output) lives in `Vl53l8Ch`, which inherits
every method here. All configuration methods require `init()` first and
must not be called while ranging (the ULD talks to the current register
bank; the stream owns it — contract 04).

#### Vl53l8.uld *(property)*

The underlying ULD driver (escape hatch for advanced DCI access).

#### Vl53l8.variant *(property)*

'cx' | 'ch' (valid after init()).

#### Vl53l8.is_alive

```python
is_alive(self) -> bool
```

#### Vl53l8.init

```python
init(self, variant: str | None = None, *, progress: Optional[Callable[[str], NoneType]] = None, write_progress: Optional[Callable[[int, int], NoneType]] = None) -> None
```

Initialize the sensor: firmware blob download + default config.

The blob variant is fixed by the class (`Vl53l8Cx` → 'cx',
`Vl53l8Ch` → 'ch'); `variant` is accepted only for backward
compatibility and must match the class variant when given. `progress`
receives phase strings; `write_progress(done, total)` tracks the big
blob writes.

#### Vl53l8.get_resolution

```python
get_resolution(self) -> int
```

Active zone count: 16 (4×4) or 64 (8×8).

#### Vl53l8.set_resolution

```python
set_resolution(self, zones: int) -> None
```

Select the zone grid: RESOLUTION_4X4 (16) or RESOLUTION_8X8 (64).
Not while ranging.

#### Vl53l8.get_ranging_frequency_hz

```python
get_ranging_frequency_hz(self) -> int
```

Configured ranging frequency in Hz.

#### Vl53l8.set_ranging_frequency_hz

```python
set_ranging_frequency_hz(self, hz: int) -> None
```

Set the ranging frequency in Hz (must be ≥ 2). Not while ranging.

Max is 60 Hz at 4×4 and 15 Hz at 8×8; below 2 Hz the sensor never
enters its ranging loop and streams nothing (contract 04).

#### Vl53l8.get_ranging_mode

```python
get_ranging_mode(self) -> int
```

RANGING_MODE_CONTINUOUS or RANGING_MODE_AUTONOMOUS.

#### Vl53l8.set_ranging_mode

```python
set_ranging_mode(self, mode: int) -> None
```

Set CONTINUOUS (free-running) or AUTONOMOUS (integrate-then-idle)
ranging. Not while ranging.

#### Vl53l8.get_integration_time_ms

```python
get_integration_time_ms(self) -> int
```

Configured integration time in ms.

#### Vl53l8.set_integration_time_ms

```python
set_integration_time_ms(self, ms: int) -> None
```

Set the integration time, 2–1000 ms. Autonomous mode only (no
effect in continuous ranging). Not while ranging.

#### Vl53l8.get_sharpener_percent

```python
get_sharpener_percent(self) -> int
```

Configured edge-sharpener strength, 0–99 %.

#### Vl53l8.set_sharpener_percent

```python
set_sharpener_percent(self, pct: int) -> None
```

Set the edge sharpener, 0–99 % (0 disables). Not while ranging.

#### Vl53l8.get_target_order

```python
get_target_order(self) -> int
```

TARGET_ORDER_CLOSEST or TARGET_ORDER_STRONGEST.

#### Vl53l8.set_target_order

```python
set_target_order(self, order: int) -> None
```

Order multi-target zones by CLOSEST or STRONGEST return. Not while
ranging.

#### Vl53l8.get_power_mode

```python
get_power_mode(self) -> int
```

POWER_MODE_SLEEP/WAKEUP/DEEP_SLEEP (uld constants).

#### Vl53l8.set_power_mode

```python
set_power_mode(self, mode: int) -> None
```

Enter sleep / wake / deep-sleep. Not while ranging. Waking from
DEEP_SLEEP re-downloads the firmware blob (init()).

#### Vl53l8.get_xtalk_margin

```python
get_xtalk_margin(self) -> float
```

#### Vl53l8.set_xtalk_margin

```python
set_xtalk_margin(self, margin_kcps: float) -> None
```

#### Vl53l8.calibrate_xtalk

```python
calibrate_xtalk(self, reflectance_percent: int, nb_samples: int, distance_mm: int) -> None
```

Run on-device crosstalk calibration against a flat target at
`distance_mm` with the given `reflectance_percent` (1..99) averaging
`nb_samples` (1..16). The result is captured into the xtalk buffer;
read it back with get_caldata_xtalk(). Blocks several seconds.

#### Vl53l8.get_caldata_xtalk

```python
get_caldata_xtalk(self) -> bytes
```

Read back the 776-byte xtalk calibration blob (save/restore).

#### Vl53l8.set_caldata_xtalk

```python
set_caldata_xtalk(self, blob: bytes) -> None
```

Restore a previously saved 776-byte xtalk calibration blob.

#### Vl53l8.get_detection_thresholds_enable

```python
get_detection_thresholds_enable(self) -> int
```

#### Vl53l8.set_detection_thresholds_enable

```python
set_detection_thresholds_enable(self, enabled: bool) -> None
```

#### Vl53l8.get_detection_thresholds

```python
get_detection_thresholds(self) -> list[dict]
```

#### Vl53l8.set_detection_thresholds

```python
set_detection_thresholds(self, thresholds: list[dict]) -> None
```

Program the 64 detection thresholds (interrupt-on-threshold). Each
entry is a dict: low_thresh, high_thresh, measurement, type, zone_num,
operation (see uld THRESH_* constants).

#### Vl53l8.set_detection_thresholds_auto_stop

```python
set_detection_thresholds_auto_stop(self, auto_stop: bool) -> None
```

#### Vl53l8.configure_motion_indicator

```python
configure_motion_indicator(self, distance_min_mm: int = 400, distance_max_mm: int = 1500)
```

Enable the motion indicator over [distance_min_mm, distance_max_mm]
and surface motion output in each frame's `.motion`. Returns the
underlying uld MotionConfig for advanced tuning.

#### Vl53l8.start_ranging

```python
start_ranging(self) -> None
```

Configure the output list, start the sensor and the MCU stream.

#### Vl53l8.stop_ranging

```python
stop_ranging(self) -> None
```

#### Vl53l8.ranging *(property)*

#### Vl53l8.on_frame

```python
on_frame(self, cb: Callable[[depz_sensor_sdk.vl53l8.Vl53l8Frame], NoneType]) -> Callable[[], NoneType]
```

Subscribe to parsed frames (reader-thread context; don't block).

#### Vl53l8.frames

```python
frames(self, maxsize: int = 8) -> depz_sensor_sdk.device.StreamIterator
```

Blocking iterator over parsed frames (bounded, drop-oldest).
Subscribes immediately — call before or after start_ranging().

#### Vl53l8.frame_parse_errors *(property)*

Frames dropped because ULD parsing failed (corrupt frame, bad size).
Distinct from reassembler gap discards (`reassembler_discards`).

#### Vl53l8.reassembler_discards *(property)*

Chunked frames discarded by the reassembler (gaps / offset errors).

#### Vl53l8.get_frame

```python
get_frame(self, timeout: float = 2.0) -> depz_sensor_sdk.vl53l8.Vl53l8Frame
```

Convenience: wait for the next frame.

Raises `DepzTimeoutError` when no frame arrives within `timeout`, and
`LinkClosedError` as soon as the device is closed while waiting —
a caller blocked here is released by `close()` instead of sitting out
the full timeout on a link that can never deliver again.

#### Vl53l8.close

```python
close(self) -> None
```

#### Vl53l8.port *(property)*

#### Vl53l8.closed *(property)*

#### Vl53l8.on_event

```python
on_event(self, cb: Callable[[depz_sensor_sdk.device.DeviceEvent], NoneType]) -> Callable[[], NoneType]
```

Subscribe to unsolicited/diagnostic events (reader-thread context;
do not block). Returns an unsubscribe function.

#### Vl53l8.events

```python
events(self, maxsize: int = 256) -> depz_sensor_sdk.device.StreamIterator
```

Pull-style event stream (bounded, drop-oldest). Subscribes
immediately — events emitted after this call are never missed.

#### Vl53l8.send

```python
send(self, cmd: int, payload: bytes = b'', *, crc_type: depz_sensor_sdk.transport.framing.CrcType | None = None) -> None
```

Fire-and-forget packet (escape hatch; prefer `request`).

#### Vl53l8.request

```python
request(self, cmd: int, payload: bytes = b'', *, matcher: Optional[Callable[[depz_sensor_sdk.transport.framing.Packet], Any]] = None, ok_completes: bool = False, timeout: float | None = None) -> Any
```

Send `cmd` and wait for its correlated completion.

Exactly one of the completion paths must be configured:
- `ok_completes=True` — RPT_STATUS(cmd, OK) finishes with None;
- `matcher` — first packet for which `matcher(pkt) is not
  request.NO_MATCH` finishes with the matcher's return value.
Non-OK RPT_STATUS echoing `cmd` always raises (BusyError for
ERR_BUSY, StatusError otherwise). One in-flight request per opcode.

#### Vl53l8.expect_report

```python
expect_report(report_id: int, unpack: Callable[[bytes], Any]) -> Callable[[depz_sensor_sdk.transport.framing.Packet], Any]
```

Matcher for a typed report identified by its report ID alone.

#### Vl53l8.expect_text

```python
expect_text(request_cmd: int) -> Callable[[depz_sensor_sdk.transport.framing.Packet], Any]
```

Matcher for RPT_TEXT echoing `request_cmd`.

#### Vl53l8.get_device_name

```python
get_device_name(self) -> str
```

#### Vl53l8.get_software_name

```python
get_software_name(self) -> str
```

#### Vl53l8.get_serial_number

```python
get_serial_number(self) -> str
```

#### Vl53l8.read_mcu_temperature

```python
read_mcu_temperature(self) -> float
```

Last cached MCU temperature in °C (device refreshes ~2 Hz).

#### Vl53l8.sync_time

```python
sync_time(self, samples: int = 5) -> depz_sensor_sdk.device.TimeSync
```

NTP-style sync; keeps the lowest-RTT sample (contract 02 §5).

#### Vl53l8.time_sync *(property)*

#### Vl53l8.to_host_time_us

```python
to_host_time_us(self, device_timestamp_us: int) -> int
```

Device µs → host monotonic µs (requires a prior `sync_time`).

#### Vl53l8.get_report_payload_crc

```python
get_report_payload_crc(self) -> depz_sensor_sdk.transport.framing.CrcType
```

#### Vl53l8.set_report_payload_crc

```python
set_report_payload_crc(self, crc_type: depz_sensor_sdk.transport.framing.CrcType) -> None
```

Set the device→host payload CRC mode (host→device is per-packet).

#### Vl53l8.get_sync_pin

```python
get_sync_pin(self, pin: int) -> depz_sensor_sdk.protocol.common.SyncPinConfig
```

#### Vl53l8.set_sync_pin

```python
set_sync_pin(self, config: depz_sensor_sdk.protocol.common.SyncPinConfig) -> None
```

#### Vl53l8.reset

```python
reset(self) -> None
```

DEVICE_RESET: device ACKs then reboots; the link will drop.

#### Vl53l8.enter_bootloader_mode

```python
enter_bootloader_mode(self) -> None
```

Ask the device to reboot into the resident bootloader and close
this connection. Re-discovery/flash flow lives in the bootloader
module (contract 06).

### Vl53l8Ch

```python
class Vl53l8Ch(port_or_link: str | depz_sensor_sdk.transport.link.Link, *, timeout: float = 0.2, tx_crc_type: depz_sensor_sdk.transport.framing.CrcType = <CrcType.NONE: 0>)
```

VL53L8CH device: the VL53L8CX superset. Inherits every CX method and
adds Compact-Network-Histogram (CNH) output. `init()` downloads the CH
firmware blob (VL53LMZ ULD 2.0.16). CNH is the reason to run CH firmware:
each frame can additionally carry a per-aggregate distance histogram.

#### Vl53l8Ch.configure_cnh

```python
configure_cnh(self, config: depz_sensor_sdk.vl53l8.cnh.CnhConfig) -> None
```

Arm the CNH histogram block for the next start_ranging(). CH only —
this method does not exist on Vl53l8Cx.

#### Vl53l8Ch.uld *(property)*

The underlying ULD driver (escape hatch for advanced DCI access).

#### Vl53l8Ch.variant *(property)*

'cx' | 'ch' (valid after init()).

#### Vl53l8Ch.is_alive

```python
is_alive(self) -> bool
```

#### Vl53l8Ch.init

```python
init(self, variant: str | None = None, *, progress: Optional[Callable[[str], NoneType]] = None, write_progress: Optional[Callable[[int, int], NoneType]] = None) -> None
```

Initialize the sensor: firmware blob download + default config.

The blob variant is fixed by the class (`Vl53l8Cx` → 'cx',
`Vl53l8Ch` → 'ch'); `variant` is accepted only for backward
compatibility and must match the class variant when given. `progress`
receives phase strings; `write_progress(done, total)` tracks the big
blob writes.

#### Vl53l8Ch.get_resolution

```python
get_resolution(self) -> int
```

Active zone count: 16 (4×4) or 64 (8×8).

#### Vl53l8Ch.set_resolution

```python
set_resolution(self, zones: int) -> None
```

Select the zone grid: RESOLUTION_4X4 (16) or RESOLUTION_8X8 (64).
Not while ranging.

#### Vl53l8Ch.get_ranging_frequency_hz

```python
get_ranging_frequency_hz(self) -> int
```

Configured ranging frequency in Hz.

#### Vl53l8Ch.set_ranging_frequency_hz

```python
set_ranging_frequency_hz(self, hz: int) -> None
```

Set the ranging frequency in Hz (must be ≥ 2). Not while ranging.

Max is 60 Hz at 4×4 and 15 Hz at 8×8; below 2 Hz the sensor never
enters its ranging loop and streams nothing (contract 04).

#### Vl53l8Ch.get_ranging_mode

```python
get_ranging_mode(self) -> int
```

RANGING_MODE_CONTINUOUS or RANGING_MODE_AUTONOMOUS.

#### Vl53l8Ch.set_ranging_mode

```python
set_ranging_mode(self, mode: int) -> None
```

Set CONTINUOUS (free-running) or AUTONOMOUS (integrate-then-idle)
ranging. Not while ranging.

#### Vl53l8Ch.get_integration_time_ms

```python
get_integration_time_ms(self) -> int
```

Configured integration time in ms.

#### Vl53l8Ch.set_integration_time_ms

```python
set_integration_time_ms(self, ms: int) -> None
```

Set the integration time, 2–1000 ms. Autonomous mode only (no
effect in continuous ranging). Not while ranging.

#### Vl53l8Ch.get_sharpener_percent

```python
get_sharpener_percent(self) -> int
```

Configured edge-sharpener strength, 0–99 %.

#### Vl53l8Ch.set_sharpener_percent

```python
set_sharpener_percent(self, pct: int) -> None
```

Set the edge sharpener, 0–99 % (0 disables). Not while ranging.

#### Vl53l8Ch.get_target_order

```python
get_target_order(self) -> int
```

TARGET_ORDER_CLOSEST or TARGET_ORDER_STRONGEST.

#### Vl53l8Ch.set_target_order

```python
set_target_order(self, order: int) -> None
```

Order multi-target zones by CLOSEST or STRONGEST return. Not while
ranging.

#### Vl53l8Ch.get_power_mode

```python
get_power_mode(self) -> int
```

POWER_MODE_SLEEP/WAKEUP/DEEP_SLEEP (uld constants).

#### Vl53l8Ch.set_power_mode

```python
set_power_mode(self, mode: int) -> None
```

Enter sleep / wake / deep-sleep. Not while ranging. Waking from
DEEP_SLEEP re-downloads the firmware blob (init()).

#### Vl53l8Ch.get_xtalk_margin

```python
get_xtalk_margin(self) -> float
```

#### Vl53l8Ch.set_xtalk_margin

```python
set_xtalk_margin(self, margin_kcps: float) -> None
```

#### Vl53l8Ch.calibrate_xtalk

```python
calibrate_xtalk(self, reflectance_percent: int, nb_samples: int, distance_mm: int) -> None
```

Run on-device crosstalk calibration against a flat target at
`distance_mm` with the given `reflectance_percent` (1..99) averaging
`nb_samples` (1..16). The result is captured into the xtalk buffer;
read it back with get_caldata_xtalk(). Blocks several seconds.

#### Vl53l8Ch.get_caldata_xtalk

```python
get_caldata_xtalk(self) -> bytes
```

Read back the 776-byte xtalk calibration blob (save/restore).

#### Vl53l8Ch.set_caldata_xtalk

```python
set_caldata_xtalk(self, blob: bytes) -> None
```

Restore a previously saved 776-byte xtalk calibration blob.

#### Vl53l8Ch.get_detection_thresholds_enable

```python
get_detection_thresholds_enable(self) -> int
```

#### Vl53l8Ch.set_detection_thresholds_enable

```python
set_detection_thresholds_enable(self, enabled: bool) -> None
```

#### Vl53l8Ch.get_detection_thresholds

```python
get_detection_thresholds(self) -> list[dict]
```

#### Vl53l8Ch.set_detection_thresholds

```python
set_detection_thresholds(self, thresholds: list[dict]) -> None
```

Program the 64 detection thresholds (interrupt-on-threshold). Each
entry is a dict: low_thresh, high_thresh, measurement, type, zone_num,
operation (see uld THRESH_* constants).

#### Vl53l8Ch.set_detection_thresholds_auto_stop

```python
set_detection_thresholds_auto_stop(self, auto_stop: bool) -> None
```

#### Vl53l8Ch.configure_motion_indicator

```python
configure_motion_indicator(self, distance_min_mm: int = 400, distance_max_mm: int = 1500)
```

Enable the motion indicator over [distance_min_mm, distance_max_mm]
and surface motion output in each frame's `.motion`. Returns the
underlying uld MotionConfig for advanced tuning.

#### Vl53l8Ch.start_ranging

```python
start_ranging(self) -> None
```

Configure the output list, start the sensor and the MCU stream.

#### Vl53l8Ch.stop_ranging

```python
stop_ranging(self) -> None
```

#### Vl53l8Ch.ranging *(property)*

#### Vl53l8Ch.on_frame

```python
on_frame(self, cb: Callable[[depz_sensor_sdk.vl53l8.Vl53l8Frame], NoneType]) -> Callable[[], NoneType]
```

Subscribe to parsed frames (reader-thread context; don't block).

#### Vl53l8Ch.frames

```python
frames(self, maxsize: int = 8) -> depz_sensor_sdk.device.StreamIterator
```

Blocking iterator over parsed frames (bounded, drop-oldest).
Subscribes immediately — call before or after start_ranging().

#### Vl53l8Ch.frame_parse_errors *(property)*

Frames dropped because ULD parsing failed (corrupt frame, bad size).
Distinct from reassembler gap discards (`reassembler_discards`).

#### Vl53l8Ch.reassembler_discards *(property)*

Chunked frames discarded by the reassembler (gaps / offset errors).

#### Vl53l8Ch.get_frame

```python
get_frame(self, timeout: float = 2.0) -> depz_sensor_sdk.vl53l8.Vl53l8Frame
```

Convenience: wait for the next frame.

Raises `DepzTimeoutError` when no frame arrives within `timeout`, and
`LinkClosedError` as soon as the device is closed while waiting —
a caller blocked here is released by `close()` instead of sitting out
the full timeout on a link that can never deliver again.

#### Vl53l8Ch.close

```python
close(self) -> None
```

#### Vl53l8Ch.port *(property)*

#### Vl53l8Ch.closed *(property)*

#### Vl53l8Ch.on_event

```python
on_event(self, cb: Callable[[depz_sensor_sdk.device.DeviceEvent], NoneType]) -> Callable[[], NoneType]
```

Subscribe to unsolicited/diagnostic events (reader-thread context;
do not block). Returns an unsubscribe function.

#### Vl53l8Ch.events

```python
events(self, maxsize: int = 256) -> depz_sensor_sdk.device.StreamIterator
```

Pull-style event stream (bounded, drop-oldest). Subscribes
immediately — events emitted after this call are never missed.

#### Vl53l8Ch.send

```python
send(self, cmd: int, payload: bytes = b'', *, crc_type: depz_sensor_sdk.transport.framing.CrcType | None = None) -> None
```

Fire-and-forget packet (escape hatch; prefer `request`).

#### Vl53l8Ch.request

```python
request(self, cmd: int, payload: bytes = b'', *, matcher: Optional[Callable[[depz_sensor_sdk.transport.framing.Packet], Any]] = None, ok_completes: bool = False, timeout: float | None = None) -> Any
```

Send `cmd` and wait for its correlated completion.

Exactly one of the completion paths must be configured:
- `ok_completes=True` — RPT_STATUS(cmd, OK) finishes with None;
- `matcher` — first packet for which `matcher(pkt) is not
  request.NO_MATCH` finishes with the matcher's return value.
Non-OK RPT_STATUS echoing `cmd` always raises (BusyError for
ERR_BUSY, StatusError otherwise). One in-flight request per opcode.

#### Vl53l8Ch.expect_report

```python
expect_report(report_id: int, unpack: Callable[[bytes], Any]) -> Callable[[depz_sensor_sdk.transport.framing.Packet], Any]
```

Matcher for a typed report identified by its report ID alone.

#### Vl53l8Ch.expect_text

```python
expect_text(request_cmd: int) -> Callable[[depz_sensor_sdk.transport.framing.Packet], Any]
```

Matcher for RPT_TEXT echoing `request_cmd`.

#### Vl53l8Ch.get_device_name

```python
get_device_name(self) -> str
```

#### Vl53l8Ch.get_software_name

```python
get_software_name(self) -> str
```

#### Vl53l8Ch.get_serial_number

```python
get_serial_number(self) -> str
```

#### Vl53l8Ch.read_mcu_temperature

```python
read_mcu_temperature(self) -> float
```

Last cached MCU temperature in °C (device refreshes ~2 Hz).

#### Vl53l8Ch.sync_time

```python
sync_time(self, samples: int = 5) -> depz_sensor_sdk.device.TimeSync
```

NTP-style sync; keeps the lowest-RTT sample (contract 02 §5).

#### Vl53l8Ch.time_sync *(property)*

#### Vl53l8Ch.to_host_time_us

```python
to_host_time_us(self, device_timestamp_us: int) -> int
```

Device µs → host monotonic µs (requires a prior `sync_time`).

#### Vl53l8Ch.get_report_payload_crc

```python
get_report_payload_crc(self) -> depz_sensor_sdk.transport.framing.CrcType
```

#### Vl53l8Ch.set_report_payload_crc

```python
set_report_payload_crc(self, crc_type: depz_sensor_sdk.transport.framing.CrcType) -> None
```

Set the device→host payload CRC mode (host→device is per-packet).

#### Vl53l8Ch.get_sync_pin

```python
get_sync_pin(self, pin: int) -> depz_sensor_sdk.protocol.common.SyncPinConfig
```

#### Vl53l8Ch.set_sync_pin

```python
set_sync_pin(self, config: depz_sensor_sdk.protocol.common.SyncPinConfig) -> None
```

#### Vl53l8Ch.reset

```python
reset(self) -> None
```

DEVICE_RESET: device ACKs then reboots; the link will drop.

#### Vl53l8Ch.enter_bootloader_mode

```python
enter_bootloader_mode(self) -> None
```

Ask the device to reboot into the resident bootloader and close
this connection. Re-discovery/flash flow lives in the bootloader
module (contract 06).

### Vl53l8Frame

```python
class Vl53l8Frame(timestamp_us: int, resolution: int, distance_mm: numpy.ndarray, target_status: numpy.ndarray, nb_target_detected: numpy.ndarray, signal_per_spad: numpy.ndarray, ambient_per_spad: numpy.ndarray, nb_spads_enabled: numpy.ndarray, range_sigma_mm: numpy.ndarray, reflectance: numpy.ndarray, silicon_temp_degc: int, cnh_raw: bytes | None = None, motion: dict | None = None) -> None
```

One parsed ranging frame. Arrays are sized to the active resolution
(16 or 64 zones); zone index runs row-major (see datasheet zone maps).

#### Vl53l8Frame.grid

```python
grid(self, field: str = 'distance_mm') -> numpy.ndarray
```

Zone array reshaped to (4,4) or (8,8).

### CnhConfig

```python
class CnhConfig()
```

Mirror of VL53LMZ_Motion_Configuration plus the helpers that fill it.
Build with init_config()/create_agg_map(), check size with
required_memory(), then pack() the 156-byte struct for cnh_send_config.

#### CnhConfig.init_config

```python
init_config(self, start_bin, num_bins, sub_sample)
```

start_bin: first device-histogram bin; num_bins: CNH bins;
sub_sample: bins of the device histogram summed per CNH bin.

#### CnhConfig.create_agg_map

```python
create_agg_map(self, resolution, start_x, start_y, merge_x, merge_y, cols, rows)
```

Map device zones to CNH aggregates. resolution: 16 (4x4) or 64 (8x8)
— must match the value passed to set_resolution().

#### CnhConfig.required_memory

```python
required_memory(self)
```

On-device CNH buffer size in bytes for this config. Raises if the
config is blank or the size exceeds CNH_MAX_DATA_BYTES.

#### CnhConfig.min_max_distance_mm

```python
min_max_distance_mm(self)
```

(min, max) target distance, in mm, fully captured by the histogram.

#### CnhConfig.bin_center_mm

```python
bin_center_mm(self, bin_idx)
```

Distance (mm) at the centre of CNH histogram bin `bin_idx`.

#### CnhConfig.pack

```python
pack(self)
```

### Vl53l8cxError

```python
class Vl53l8cxError(code, where='')
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

## BNO086 (IMU)

### Bno086

```python
class Bno086(port_or_link: str | depz_sensor_sdk.transport.link.Link, *, timeout: float = 0.2, tx_crc_type: depz_sensor_sdk.transport.framing.CrcType = <CrcType.NONE: 0>)
```

BNO086 device: enable SH-2 sensors, stream typed reports.

Typical use::

    with Bno086(port) as imu:
        imu.enable(SensorId.ROTATION_VECTOR, hz=100)
        for r in imu.reports():
            print(r.i, r.j, r.k, r.real)

Callbacks run on the reader thread — never call blocking device methods
(enable/tare/...) from inside one.

#### Bno086.hardware_reset

```python
hardware_reset(self, timeout: float = 2.0) -> None
```

Hard-reset the sensor via nRST (0x32). All SHTP state (seq
counters, partial cargos) and cached features restart from zero.

The hub's RPT_STATUS OK ack for the 0x32 command is treated as the
reset confirmation. The SH-2 executable-channel reset-complete (which
the BNO08X SH-2 spec would emit) is only waited for best-effort:
older firmware (≤ v0.95) did **not** emit it (ERRATA E9 in
contracts/ERRATA.md, now fixed in newer firmware); the best-effort wait
handles both, so its absence is *not* an error — the sensor is fully
usable without it.

#### Bno086.wake

```python
wake(self) -> None
```

Pulse WAKE (PS0): wakes the sensor from sleep, no state loss.

#### Bno086.advertisement *(property)*

Raw SHTP channel-0 advertisement bytes seen since open/reset.

#### Bno086.product_id

```python
product_id(self, timeout: float = 1.0) -> depz_sensor_sdk.bno086.sh2.ProductId
```

Product ID Request/Response round trip (first responding
subsystem).

#### Bno086.enable

```python
enable(self, sensor: int, hz: float | None = None, *, interval_us: int | None = None, batch_us: int = 0, sensitivity: int = 0, flags: int = 0, cfg_word: int = 0, verify: bool = True, timeout: float = 1.0) -> depz_sensor_sdk.bno086.sh2.FeatureResponse | None
```

Enable `sensor` at the requested rate via Set Feature (0xFD).

Give either `hz` or `interval_us`. The hub rounds to its 1 kHz/2^n
grid; with `verify` the granted rate is read back via Get Feature and
a result outside 0.9–2.1× the request emits a UserWarning (contract
05 §7 — warn, never raise). Returns the FeatureResponse (None when
`verify=False`).

#### Bno086.disable

```python
disable(self, sensor: int) -> None
```

Disable `sensor` (Set Feature with interval 0).

#### Bno086.get_feature

```python
get_feature(self, sensor: int, timeout: float = 1.0) -> depz_sensor_sdk.bno086.sh2.FeatureResponse
```

Get Feature Request/Response round trip for `sensor`.

#### Bno086.enable_rotation_vector

```python
enable_rotation_vector(self, hz: float = 100, **kw) -> depz_sensor_sdk.bno086.sh2.FeatureResponse | None
```

#### Bno086.enable_game_rotation_vector

```python
enable_game_rotation_vector(self, hz: float = 100, **kw) -> depz_sensor_sdk.bno086.sh2.FeatureResponse | None
```

#### Bno086.enable_accelerometer

```python
enable_accelerometer(self, hz: float = 100, **kw) -> depz_sensor_sdk.bno086.sh2.FeatureResponse | None
```

#### Bno086.enable_gyroscope

```python
enable_gyroscope(self, hz: float = 100, **kw) -> depz_sensor_sdk.bno086.sh2.FeatureResponse | None
```

#### Bno086.enable_magnetometer

```python
enable_magnetometer(self, hz: float = 50, **kw) -> depz_sensor_sdk.bno086.sh2.FeatureResponse | None
```

#### Bno086.enable_linear_acceleration

```python
enable_linear_acceleration(self, hz: float = 100, **kw) -> depz_sensor_sdk.bno086.sh2.FeatureResponse | None
```

#### Bno086.enable_gravity

```python
enable_gravity(self, hz: float = 100, **kw) -> depz_sensor_sdk.bno086.sh2.FeatureResponse | None
```

#### Bno086.enable_gyro_integrated_rv

```python
enable_gyro_integrated_rv(self, hz: float = 400, **kw) -> depz_sensor_sdk.bno086.sh2.FeatureResponse | None
```

#### Bno086.on_report

```python
on_report(self, cb: Callable[[depz_sensor_sdk.bno086.reports.Report], NoneType], sensors: Union[Iterable[int], int, NoneType] = None) -> Callable[[], NoneType]
```

Subscribe to typed sensor reports (reader-thread context; do not
block). `sensors` filters by SensorId. Returns an unsubscribe fn.

#### Bno086.reports

```python
reports(self, sensors: Union[Iterable[int], int, NoneType] = None, maxsize: int = 1024) -> depz_sensor_sdk.device.StreamIterator
```

Blocking iterator over typed reports (bounded, drop-oldest).
Subscribes eagerly — reports emitted after this call are never
missed.

#### Bno086.tare_now

```python
tare_now(self, axes: int = <TareAxis.ALL: 7>, basis: int = <TareBasis.ROTATION_VECTOR: 0>) -> None
```

Tare the selected axes against `basis` (no response per SH-2).

#### Bno086.persist_tare

```python
persist_tare(self) -> None
```

Persist the current tare into FRS (no response per SH-2).

#### Bno086.set_reorientation

```python
set_reorientation(self, x: float, y: float, z: float, w: float) -> None
```

Set the runtime reorientation quaternion (Q14 on the wire; all
zeros clears). No response per SH-2.

#### Bno086.set_calibration

```python
set_calibration(self, accel: bool = True, gyro: bool = True, mag: bool = True, planar: bool = False, timeout: float = 1.0) -> None
```

Configure ME calibration; raises Sh2Error on non-zero status.

#### Bno086.get_calibration

```python
get_calibration(self, timeout: float = 1.0) -> depz_sensor_sdk.bno086.CalibrationConfig
```

Read back which ME calibrations are running.

#### Bno086.save_dcd

```python
save_dcd(self, timeout: float = 1.0) -> None
```

Save the dynamic calibration data to flash (DCD Save Now).

#### Bno086.configure_periodic_dcd

```python
configure_periodic_dcd(self, enable: bool) -> None
```

Enable/disable the hub's periodic DCD autosave (no response).

#### Bno086.frs_read

```python
frs_read(self, record_id: int, timeout: float = 2.0) -> tuple[int, ...]
```

Read a whole FRS record; returns its 32-bit words.

#### Bno086.frs_write

```python
frs_write(self, record_id: int, words: Iterable[int], timeout: float = 2.0) -> None
```

Write a whole FRS record (word list); raises Sh2Error on failure.

#### Bno086.get_metadata

```python
get_metadata(self, sensor: int, timeout: float = 2.0) -> depz_sensor_sdk.bno086.sh2.SensorMetadata
```

Read + parse the sensor's FRS metadata record.

#### Bno086.get_oscillator_type

```python
get_oscillator_type(self, timeout: float = 1.0) -> depz_sensor_sdk.bno086.sh2.OscillatorType
```

Get Oscillator Type (command 0x0A). r[0] is the type directly.

#### Bno086.clear_dcd_and_reset

```python
clear_dcd_and_reset(self, timeout: float = 2.0) -> None
```

Clear the in-RAM dynamic calibration and reset the sensor
(command 0x0B). There is no command response — the hub resets, so this
waits for the executable reset-complete like hardware_reset().

#### Bno086.get_errors

```python
get_errors(self, severity: int = 0, timeout: float = 1.0) -> list[depz_sensor_sdk.bno086.sh2.ErrorRecord]
```

Read the error queue (command 0x01), filtered to `severity` or
greater. Records stream until one with source == 255 (no more).

#### Bno086.get_counts

```python
get_counts(self, sensor: int, timeout: float = 1.0) -> depz_sensor_sdk.bno086.sh2.Counts
```

Read a sensor's event counts (command 0x02). The hub answers with
two responses (response_seq 0 then 1).

#### Bno086.clear_counts

```python
clear_counts(self, sensor: int, timeout: float = 1.0) -> None
```

Clear a sensor's event counts (command 0x02, subcommand 1).

#### Bno086.close

```python
close(self) -> None
```

#### Bno086.port *(property)*

#### Bno086.closed *(property)*

#### Bno086.on_event

```python
on_event(self, cb: Callable[[depz_sensor_sdk.device.DeviceEvent], NoneType]) -> Callable[[], NoneType]
```

Subscribe to unsolicited/diagnostic events (reader-thread context;
do not block). Returns an unsubscribe function.

#### Bno086.events

```python
events(self, maxsize: int = 256) -> depz_sensor_sdk.device.StreamIterator
```

Pull-style event stream (bounded, drop-oldest). Subscribes
immediately — events emitted after this call are never missed.

#### Bno086.send

```python
send(self, cmd: int, payload: bytes = b'', *, crc_type: depz_sensor_sdk.transport.framing.CrcType | None = None) -> None
```

Fire-and-forget packet (escape hatch; prefer `request`).

#### Bno086.request

```python
request(self, cmd: int, payload: bytes = b'', *, matcher: Optional[Callable[[depz_sensor_sdk.transport.framing.Packet], Any]] = None, ok_completes: bool = False, timeout: float | None = None) -> Any
```

Send `cmd` and wait for its correlated completion.

Exactly one of the completion paths must be configured:
- `ok_completes=True` — RPT_STATUS(cmd, OK) finishes with None;
- `matcher` — first packet for which `matcher(pkt) is not
  request.NO_MATCH` finishes with the matcher's return value.
Non-OK RPT_STATUS echoing `cmd` always raises (BusyError for
ERR_BUSY, StatusError otherwise). One in-flight request per opcode.

#### Bno086.expect_report

```python
expect_report(report_id: int, unpack: Callable[[bytes], Any]) -> Callable[[depz_sensor_sdk.transport.framing.Packet], Any]
```

Matcher for a typed report identified by its report ID alone.

#### Bno086.expect_text

```python
expect_text(request_cmd: int) -> Callable[[depz_sensor_sdk.transport.framing.Packet], Any]
```

Matcher for RPT_TEXT echoing `request_cmd`.

#### Bno086.get_device_name

```python
get_device_name(self) -> str
```

#### Bno086.get_software_name

```python
get_software_name(self) -> str
```

#### Bno086.get_serial_number

```python
get_serial_number(self) -> str
```

#### Bno086.read_mcu_temperature

```python
read_mcu_temperature(self) -> float
```

Last cached MCU temperature in °C (device refreshes ~2 Hz).

#### Bno086.sync_time

```python
sync_time(self, samples: int = 5) -> depz_sensor_sdk.device.TimeSync
```

NTP-style sync; keeps the lowest-RTT sample (contract 02 §5).

#### Bno086.time_sync *(property)*

#### Bno086.to_host_time_us

```python
to_host_time_us(self, device_timestamp_us: int) -> int
```

Device µs → host monotonic µs (requires a prior `sync_time`).

#### Bno086.get_report_payload_crc

```python
get_report_payload_crc(self) -> depz_sensor_sdk.transport.framing.CrcType
```

#### Bno086.set_report_payload_crc

```python
set_report_payload_crc(self, crc_type: depz_sensor_sdk.transport.framing.CrcType) -> None
```

Set the device→host payload CRC mode (host→device is per-packet).

#### Bno086.get_sync_pin

```python
get_sync_pin(self, pin: int) -> depz_sensor_sdk.protocol.common.SyncPinConfig
```

#### Bno086.set_sync_pin

```python
set_sync_pin(self, config: depz_sensor_sdk.protocol.common.SyncPinConfig) -> None
```

#### Bno086.reset

```python
reset(self) -> None
```

DEVICE_RESET: device ACKs then reboots; the link will drop.

#### Bno086.enter_bootloader_mode

```python
enter_bootloader_mode(self) -> None
```

Ask the device to reboot into the resident bootloader and close
this connection. Re-discovery/flash flow lives in the bootloader
module (contract 06).

### SensorId

```python
class SensorId(*values)
```

SH-2 input report IDs (datasheet §1.3.5, sh2 reference driver).

#### SensorId.conjugate

```python
conjugate
```

Returns self, the complex conjugate of any int.

#### SensorId.bit_length

```python
bit_length(self, /)
```

Number of bits necessary to represent self in binary.

>>> bin(37)
'0b100101'
>>> (37).bit_length()
6

#### SensorId.bit_count

```python
bit_count(self, /)
```

Number of ones in the binary representation of the absolute value of self.

Also known as the population count.

>>> bin(13)
'0b1101'
>>> (13).bit_count()
3

#### SensorId.to_bytes

```python
to_bytes(self, /, length=1, byteorder='big', *, signed=False)
```

Return an array of bytes representing an integer.

length
  Length of bytes object to use.  An OverflowError is raised if the
  integer is not representable with the given number of bytes.  Default
  is length 1.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Determines whether two's complement is used to represent the integer.
  If signed is False and a negative integer is given, an OverflowError
  is raised.

#### SensorId.from_bytes

```python
from_bytes(type, /, bytes, byteorder='big', *, signed=False)
```

Return the integer represented by the given array of bytes.

bytes
  Holds the array of bytes to convert.  The argument must either
  support the buffer protocol or be an iterable object producing bytes.
  Bytes and bytearray are examples of built-in objects that support the
  buffer protocol.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Indicates whether two's complement is used to represent the integer.

#### SensorId.as_integer_ratio

```python
as_integer_ratio(self, /)
```

Return a pair of integers, whose ratio is equal to the original int.

The ratio is in lowest terms and has a positive denominator.

>>> (10).as_integer_ratio()
(10, 1)
>>> (-10).as_integer_ratio()
(-10, 1)
>>> (0).as_integer_ratio()
(0, 1)

#### SensorId.is_integer

```python
is_integer(self, /)
```

Returns True. Exists for duck type compatibility with float.is_integer.

### RotationVector

```python
class RotationVector(sensor_id: int, timestamp_us: int, seq: int, accuracy: int, delay_us: int, i_raw: int, j_raw: int, k_raw: int, real_raw: int, accuracy_raw: int | None = None) -> None
```

Quaternion reports 0x05/0x08/0x09/0x28/0x29 (unit quaternion, Q14).

`accuracy_raw` (Q12, radians) is present only for 0x05/0x09/0x28.

#### RotationVector.i *(property)*

#### RotationVector.j *(property)*

#### RotationVector.k *(property)*

#### RotationVector.real *(property)*

#### RotationVector.accuracy_rad *(property)*

Estimated heading accuracy in radians (None for game variants).

### GyroIntegratedRV

```python
class GyroIntegratedRV(sensor_id: int, timestamp_us: int, i_raw: int, j_raw: int, k_raw: int, real_raw: int, vx_raw: int, vy_raw: int, vz_raw: int) -> None
```

0x2A gyro-integrated rotation vector (channel 5, dense — no SH-2
header). Quaternion Q14, angular velocity Q10 rad/s.

#### GyroIntegratedRV.i *(property)*

#### GyroIntegratedRV.j *(property)*

#### GyroIntegratedRV.k *(property)*

#### GyroIntegratedRV.real *(property)*

#### GyroIntegratedRV.angular_velocity *(property)*

### Report

```python
class Report(sensor_id: int, timestamp_us: int) -> None
```

Base for anything the sensor pushes; `timestamp_us` is absolute in the
MCU clock domain (bridge capture time corrected by timebase + delay).

### InputReport

```python
class InputReport(sensor_id: int, timestamp_us: int, seq: int, accuracy: int, delay_us: int) -> None
```

Channel-3/4 report with the common SH-2 header fields.

### Acceleration

```python
class Acceleration(sensor_id: int, timestamp_us: int, seq: int, accuracy: int, delay_us: int, x_raw: int, y_raw: int, z_raw: int) -> None
```

0x01 accelerometer / 0x04 linear acceleration / 0x06 gravity (Q8).

#### Acceleration.x *(property)*

#### Acceleration.y *(property)*

#### Acceleration.z *(property)*

### Gyroscope

```python
class Gyroscope(sensor_id: int, timestamp_us: int, seq: int, accuracy: int, delay_us: int, x_raw: int, y_raw: int, z_raw: int) -> None
```

0x02 calibrated gyroscope (Q9).

#### Gyroscope.x *(property)*

#### Gyroscope.y *(property)*

#### Gyroscope.z *(property)*

### Magnetometer

```python
class Magnetometer(sensor_id: int, timestamp_us: int, seq: int, accuracy: int, delay_us: int, x_raw: int, y_raw: int, z_raw: int) -> None
```

0x03 calibrated magnetic field (Q4).

#### Magnetometer.x *(property)*

#### Magnetometer.y *(property)*

#### Magnetometer.z *(property)*

### FeatureResponse

```python
class FeatureResponse(sensor_id: int, flags: int, sensitivity: int, interval_us: int, batch_us: int, cfg_word: int) -> None
```

Get Feature Response (0xFC), 17 bytes — the rates in effect.

#### FeatureResponse.unpack *(classmethod)*

```python
unpack(cls, payload: bytes) -> 'FeatureResponse'
```

### ProductId

```python
class ProductId(reset_cause: int, sw_version_major: int, sw_version_minor: int, sw_part_number: int, sw_build_number: int, sw_version_patch: int) -> None
```

Product ID Response (0xF8), 16 bytes. The sensor sends one response
per subsystem (typically 2); reset_cause per SH-2 §6.4.5.2.

#### ProductId.unpack *(classmethod)*

```python
unpack(cls, payload: bytes) -> 'ProductId'
```

#### ProductId.version *(property)*

### CommandResponse

```python
class CommandResponse(seq: int, command: int, command_seq: int, response_seq: int, r: tuple[int, ...]) -> None
```

Command Response (0xF1), 16 bytes.

`command_seq` echoes the request's sequence number (correlate on it plus
`command`); `response_seq` counts multiple responses to one request.
R0 is the status word for most commands (0 = success).

#### CommandResponse.unpack *(classmethod)*

```python
unpack(cls, payload: bytes) -> 'CommandResponse'
```

#### CommandResponse.status *(property)*

### SensorMetadata

```python
class SensorMetadata(me_version: int, mh_version: int, sh_version: int, range_raw: int, resolution_raw: int, revision: int, power_ma_q10: int, min_period_us: int, max_period_us: int, fifo_max: int, fifo_reserved: int, batch_buffer_bytes: int, q_point_1: int, q_point_2: int, q_point_3: int, raw_words: tuple[int, ...]) -> None
```

Parsed sensor metadata FRS record; `raw_words` is authoritative.

Field packing follows the sh2 reference driver (revision-gated fields
are 0 when the record predates them).

#### SensorMetadata.from_words *(classmethod)*

```python
from_words(cls, words: list[int] | tuple[int, ...]) -> 'SensorMetadata'
```

### TareAxis

```python
class TareAxis(*values)
```

#### TareAxis.conjugate

```python
conjugate
```

Returns self, the complex conjugate of any int.

#### TareAxis.bit_length

```python
bit_length(self, /)
```

Number of bits necessary to represent self in binary.

>>> bin(37)
'0b100101'
>>> (37).bit_length()
6

#### TareAxis.bit_count

```python
bit_count(self, /)
```

Number of ones in the binary representation of the absolute value of self.

Also known as the population count.

>>> bin(13)
'0b1101'
>>> (13).bit_count()
3

#### TareAxis.to_bytes

```python
to_bytes(self, /, length=1, byteorder='big', *, signed=False)
```

Return an array of bytes representing an integer.

length
  Length of bytes object to use.  An OverflowError is raised if the
  integer is not representable with the given number of bytes.  Default
  is length 1.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Determines whether two's complement is used to represent the integer.
  If signed is False and a negative integer is given, an OverflowError
  is raised.

#### TareAxis.from_bytes

```python
from_bytes(type, /, bytes, byteorder='big', *, signed=False)
```

Return the integer represented by the given array of bytes.

bytes
  Holds the array of bytes to convert.  The argument must either
  support the buffer protocol or be an iterable object producing bytes.
  Bytes and bytearray are examples of built-in objects that support the
  buffer protocol.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Indicates whether two's complement is used to represent the integer.

#### TareAxis.as_integer_ratio

```python
as_integer_ratio(self, /)
```

Return a pair of integers, whose ratio is equal to the original int.

The ratio is in lowest terms and has a positive denominator.

>>> (10).as_integer_ratio()
(10, 1)
>>> (-10).as_integer_ratio()
(-10, 1)
>>> (0).as_integer_ratio()
(0, 1)

#### TareAxis.is_integer

```python
is_integer(self, /)
```

Returns True. Exists for duck type compatibility with float.is_integer.

### TareBasis

```python
class TareBasis(*values)
```

Rotation vector used as the tare reference (Tare Now P2).

#### TareBasis.conjugate

```python
conjugate
```

Returns self, the complex conjugate of any int.

#### TareBasis.bit_length

```python
bit_length(self, /)
```

Number of bits necessary to represent self in binary.

>>> bin(37)
'0b100101'
>>> (37).bit_length()
6

#### TareBasis.bit_count

```python
bit_count(self, /)
```

Number of ones in the binary representation of the absolute value of self.

Also known as the population count.

>>> bin(13)
'0b1101'
>>> (13).bit_count()
3

#### TareBasis.to_bytes

```python
to_bytes(self, /, length=1, byteorder='big', *, signed=False)
```

Return an array of bytes representing an integer.

length
  Length of bytes object to use.  An OverflowError is raised if the
  integer is not representable with the given number of bytes.  Default
  is length 1.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Determines whether two's complement is used to represent the integer.
  If signed is False and a negative integer is given, an OverflowError
  is raised.

#### TareBasis.from_bytes

```python
from_bytes(type, /, bytes, byteorder='big', *, signed=False)
```

Return the integer represented by the given array of bytes.

bytes
  Holds the array of bytes to convert.  The argument must either
  support the buffer protocol or be an iterable object producing bytes.
  Bytes and bytearray are examples of built-in objects that support the
  buffer protocol.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Indicates whether two's complement is used to represent the integer.

#### TareBasis.as_integer_ratio

```python
as_integer_ratio(self, /)
```

Return a pair of integers, whose ratio is equal to the original int.

The ratio is in lowest terms and has a positive denominator.

>>> (10).as_integer_ratio()
(10, 1)
>>> (-10).as_integer_ratio()
(-10, 1)
>>> (0).as_integer_ratio()
(0, 1)

#### TareBasis.is_integer

```python
is_integer(self, /)
```

Returns True. Exists for duck type compatibility with float.is_integer.

### Sh2Error

```python
class Sh2Error
```

SH-2 level failure (bad response status, FRS error, ...).

#### Sh2Error.with_traceback

```python
with_traceback
```

Exception.with_traceback(tb) --
set self.__traceback__ to tb and return self.

#### Sh2Error.add_note

```python
add_note
```

Exception.add_note(note) --
add a note to the exception

### CalibrationConfig

```python
class CalibrationConfig(accel: bool, gyro: bool, mag: bool, planar: bool) -> None
```

ME calibration enables as reported by the sensor.

### OscillatorType

```python
class OscillatorType(*values)
```

Get-Oscillator-Type (command 0x0A) result (r[0]).

#### OscillatorType.conjugate

```python
conjugate
```

Returns self, the complex conjugate of any int.

#### OscillatorType.bit_length

```python
bit_length(self, /)
```

Number of bits necessary to represent self in binary.

>>> bin(37)
'0b100101'
>>> (37).bit_length()
6

#### OscillatorType.bit_count

```python
bit_count(self, /)
```

Number of ones in the binary representation of the absolute value of self.

Also known as the population count.

>>> bin(13)
'0b1101'
>>> (13).bit_count()
3

#### OscillatorType.to_bytes

```python
to_bytes(self, /, length=1, byteorder='big', *, signed=False)
```

Return an array of bytes representing an integer.

length
  Length of bytes object to use.  An OverflowError is raised if the
  integer is not representable with the given number of bytes.  Default
  is length 1.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Determines whether two's complement is used to represent the integer.
  If signed is False and a negative integer is given, an OverflowError
  is raised.

#### OscillatorType.from_bytes

```python
from_bytes(type, /, bytes, byteorder='big', *, signed=False)
```

Return the integer represented by the given array of bytes.

bytes
  Holds the array of bytes to convert.  The argument must either
  support the buffer protocol or be an iterable object producing bytes.
  Bytes and bytearray are examples of built-in objects that support the
  buffer protocol.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Indicates whether two's complement is used to represent the integer.

#### OscillatorType.as_integer_ratio

```python
as_integer_ratio(self, /)
```

Return a pair of integers, whose ratio is equal to the original int.

The ratio is in lowest terms and has a positive denominator.

>>> (10).as_integer_ratio()
(10, 1)
>>> (-10).as_integer_ratio()
(-10, 1)
>>> (0).as_integer_ratio()
(0, 1)

#### OscillatorType.is_integer

```python
is_integer(self, /)
```

Returns True. Exists for duck type compatibility with float.is_integer.

### ErrorRecord

```python
class ErrorRecord(severity: int, seq: int, source: int, error: int, module: int, code: int) -> None
```

One error queue entry (command 0x01 response, r[0..5]).

#### ErrorRecord.from_response *(classmethod)*

```python
from_response(cls, resp: 'CommandResponse') -> 'ErrorRecord'
```

### ErrorSource

```python
class ErrorSource(*values)
```

`source` field of an error record (SH-2 §6.4.1).

#### ErrorSource.conjugate

```python
conjugate
```

Returns self, the complex conjugate of any int.

#### ErrorSource.bit_length

```python
bit_length(self, /)
```

Number of bits necessary to represent self in binary.

>>> bin(37)
'0b100101'
>>> (37).bit_length()
6

#### ErrorSource.bit_count

```python
bit_count(self, /)
```

Number of ones in the binary representation of the absolute value of self.

Also known as the population count.

>>> bin(13)
'0b1101'
>>> (13).bit_count()
3

#### ErrorSource.to_bytes

```python
to_bytes(self, /, length=1, byteorder='big', *, signed=False)
```

Return an array of bytes representing an integer.

length
  Length of bytes object to use.  An OverflowError is raised if the
  integer is not representable with the given number of bytes.  Default
  is length 1.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Determines whether two's complement is used to represent the integer.
  If signed is False and a negative integer is given, an OverflowError
  is raised.

#### ErrorSource.from_bytes

```python
from_bytes(type, /, bytes, byteorder='big', *, signed=False)
```

Return the integer represented by the given array of bytes.

bytes
  Holds the array of bytes to convert.  The argument must either
  support the buffer protocol or be an iterable object producing bytes.
  Bytes and bytearray are examples of built-in objects that support the
  buffer protocol.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Indicates whether two's complement is used to represent the integer.

#### ErrorSource.as_integer_ratio

```python
as_integer_ratio(self, /)
```

Return a pair of integers, whose ratio is equal to the original int.

The ratio is in lowest terms and has a positive denominator.

>>> (10).as_integer_ratio()
(10, 1)
>>> (-10).as_integer_ratio()
(-10, 1)
>>> (0).as_integer_ratio()
(0, 1)

#### ErrorSource.is_integer

```python
is_integer(self, /)
```

Returns True. Exists for duck type compatibility with float.is_integer.

### Counts

```python
class Counts(sensor_id: int, offered: int, accepted: int, on: int, attempted: int) -> None
```

Per-sensor event counts (command 0x02 get response, 2 messages).

## Bootloader / firmware update

### BootloaderDevice

```python
class BootloaderDevice(port_or_link: str | depz_sensor_sdk.transport.link.Link, *, timeout: float = 5.0)
```

Synchronous client for a device already in bootloader mode.

The bootloader protocol is strict request/response — no reader thread.

#### BootloaderDevice.close

```python
close(self) -> None
```

#### BootloaderDevice.transact

```python
transact(self, cmd: int, payload: bytes = b'', *, crc_type: depz_sensor_sdk.transport.framing.CrcType = <CrcType.NONE: 0>, timeout: float | None = None, min_len: int = 2) -> depz_sensor_sdk.transport.framing.Packet
```

Send and wait for the reply echoing `cmd` in payload[0].

#### BootloaderDevice.get_firmware_name

```python
get_firmware_name(self) -> str
```

#### BootloaderDevice.get_device_name

```python
get_device_name(self) -> str
```

#### BootloaderDevice.get_serial_number

```python
get_serial_number(self) -> str
```

#### BootloaderDevice.get_flash_info

```python
get_flash_info(self) -> depz_sensor_sdk.protocol.bootloader.FlashInfo
```

#### BootloaderDevice.erase_app

```python
erase_app(self) -> None
```

#### BootloaderDevice.write_page

```python
write_page(self, addr: int, data: bytes) -> int
```

Write one page (payload CRC16 framing); returns the device-computed
page CRC32.

#### BootloaderDevice.verify_app_crc

```python
verify_app_crc(self) -> int
```

#### BootloaderDevice.boot_application

```python
boot_application(self) -> None
```

Fire-and-forget: the device jumps to the app and re-enumerates.

#### BootloaderDevice.flash

```python
flash(self, image: depz_sensor_sdk.protocol.bootloader.FwDepzImage, *, progress: Optional[Callable[[float, str], NoneType]] = None, page_retries: int = 3) -> None
```

ERASE → WRITE_PAGE loop (per-page CRC32 check, retries) → VERIFY.

Does not boot the app — call `boot_application()` after.

### update_firmware

```python
update_firmware(port: str, fwdepz_path: str, *, progress: Optional[Callable[[float, str], NoneType]] = None) -> None
```

Full app-mode → bootloader → flash → boot-app flow (contract 06 §3).

### find_bootloader_port

```python
find_bootloader_port(serial_number: str, *, timeout: float = 10.0) -> str
```

Poll serial ports until a BOOTDEPZ device with `serial_number` appears
(after an app→bootloader reboot the USB serial string is preserved).

## Datasets (record & replay)

### SessionRecorder

```python
class SessionRecorder(path: str | pathlib.Path, *, note: str = '', vl53l8_layers: bool = False)
```

Record decoded data from several devices onto one host timeline.

Usage::

    with SessionRecorder("run.depzdata") as rec:
        rec.add(sr04)      # runs sync_time, hooks the measurement stream
        rec.add(vl53l8)    # hooks the frame stream
        sr04.start(); vl53l8.start_ranging()
        time.sleep(10)
    # exit unhooks and closes the file

Devices must already be open; the recorder never reconfigures them —
start/stop streaming yourself.

#### SessionRecorder.add

```python
add(self, device: depz_sensor_sdk.device.DeviceBase, *, device_id: str | None = None, sync_samples: int = 5) -> str
```

Register a device (before the first record is written). Runs
`sync_time` so its timestamps land on the shared host timeline.

#### SessionRecorder.start

```python
start(self) -> None
```

#### SessionRecorder.stop

```python
stop(self) -> None
```

#### SessionRecorder.records_written *(property)*

### DatasetReader

```python
class DatasetReader(path: str | pathlib.Path)
```

Read a `.depzdata` file; iterate records merged by host time.

#### DatasetReader.devices *(property)*

#### DatasetReader.play

```python
play(self, callback: Callable[[depz_sensor_sdk.dataset.DatasetRecord], NoneType], *, speed: float = 1.0, start_t_us: int | None = None, stop: threading.Event | None = None) -> None
```

Deliver records paced by their timestamps (speed=2.0 → twice as
fast; speed=0 → as fast as possible).

### DatasetRecord

```python
class DatasetRecord(device_id: str, t_host_us: int, kind: str, value: dict[str, typing.Any]) -> None
```

## Transport

### MAGIC

`bytes` constant.

bytes(string, encoding[, errors]) -> bytes
bytes(bytes_or_buffer) -> immutable copy of bytes_or_buffer
bytes(int) -> bytes object of size given by the parameter initialized with null bytes
bytes() -> empty bytes object

Construct an immutable array of bytes from:
  - an iterable yielding integers in range(256)
  - a text string encoded using the specified encoding
  - any object implementing the buffer API.
  - an integer

### HEADER_SIZE

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

### MAX_PAYLOAD

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

### CrcType

```python
class CrcType(*values)
```

#### CrcType.conjugate

```python
conjugate
```

Returns self, the complex conjugate of any int.

#### CrcType.bit_length

```python
bit_length(self, /)
```

Number of bits necessary to represent self in binary.

>>> bin(37)
'0b100101'
>>> (37).bit_length()
6

#### CrcType.bit_count

```python
bit_count(self, /)
```

Number of ones in the binary representation of the absolute value of self.

Also known as the population count.

>>> bin(13)
'0b1101'
>>> (13).bit_count()
3

#### CrcType.to_bytes

```python
to_bytes(self, /, length=1, byteorder='big', *, signed=False)
```

Return an array of bytes representing an integer.

length
  Length of bytes object to use.  An OverflowError is raised if the
  integer is not representable with the given number of bytes.  Default
  is length 1.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Determines whether two's complement is used to represent the integer.
  If signed is False and a negative integer is given, an OverflowError
  is raised.

#### CrcType.from_bytes

```python
from_bytes(type, /, bytes, byteorder='big', *, signed=False)
```

Return the integer represented by the given array of bytes.

bytes
  Holds the array of bytes to convert.  The argument must either
  support the buffer protocol or be an iterable object producing bytes.
  Bytes and bytearray are examples of built-in objects that support the
  buffer protocol.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Indicates whether two's complement is used to represent the integer.

#### CrcType.as_integer_ratio

```python
as_integer_ratio(self, /)
```

Return a pair of integers, whose ratio is equal to the original int.

The ratio is in lowest terms and has a positive denominator.

>>> (10).as_integer_ratio()
(10, 1)
>>> (-10).as_integer_ratio()
(-10, 1)
>>> (0).as_integer_ratio()
(0, 1)

#### CrcType.is_integer

```python
is_integer(self, /)
```

Returns True. Exists for duck type compatibility with float.is_integer.

### Packet

```python
class Packet(cmd: int, seq: int, payload: bytes) -> None
```

### Trash

```python
class Trash(data: bytes) -> None
```

Bytes discarded while hunting for a valid frame. Boundaries between
consecutive Trash events depend on read chunking; only the concatenated
byte stream is deterministic.

### CrcError

```python
class CrcError(cmd: int, seq: int) -> None
```

A frame with a valid header whose payload CRC failed; dropped.

### ParserEvent

`UnionType` constant.

Represent a PEP 604 union type

E.g. for int | str

### PacketParser

```python
class PacketParser() -> None
```

Incremental frame parser. Feed arbitrary byte chunks; get events.

Event order is invariant to chunking (contract 01 §5) except Trash event
boundaries — concatenate Trash data when comparing streams.

#### PacketParser.feed

```python
feed(self, data: bytes) -> list[depz_sensor_sdk.transport.framing.Packet | depz_sensor_sdk.transport.framing.Trash | depz_sensor_sdk.transport.framing.CrcError]
```

### build_packet

```python
build_packet(cmd: int, payload: bytes = b'', seq: int = 0, crc_type: depz_sensor_sdk.transport.framing.CrcType = <CrcType.NONE: 0>) -> bytes
```

Frame one packet. `crc_type` bits are set in the header even for an
empty payload (matching device TX), but CRC bytes are only appended for
non-empty payloads.

### payload_crc_bytes

```python
payload_crc_bytes(crc_type: depz_sensor_sdk.transport.framing.CrcType, payload: bytes) -> bytes
```

CRC trailer for a payload; empty payloads never carry CRC bytes.

### crc8_maxim

```python
crc8_maxim(data: bytes) -> int
```

CRC-8/MAXIM: poly 0x31 reflected, init 0x00, xorout 0x00.

### crc16_modbus

```python
crc16_modbus(data: bytes) -> int
```

CRC-16/MODBUS: poly 0x8005 reflected, init 0xFFFF, xorout 0x0000.

### crc32_iso_hdlc

```python
crc32_iso_hdlc(data: bytes) -> int
```

CRC-32/ISO-HDLC: poly 0x04C11DB7 reflected, init/xorout 0xFFFFFFFF.

### crc16_ccitt_false

```python
crc16_ccitt_false(data: bytes) -> int
```

CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, not reflected.

Used only for the `.fwdepz` file header (contract 06), never on the wire.

### Link

```python
class Link()
```

Blocking byte pipe.

#### Link.read

```python
read(self, timeout: float | None = None) -> bytes
```

Return the next available chunk (any size ≥1), or b"" on timeout.

#### Link.write

```python
write(self, data: bytes) -> None
```

#### Link.close

```python
close(self) -> None
```

#### Link.closed *(property)*

### LoopbackLink

```python
class LoopbackLink() -> None
```

In-memory link; `peer` sees what we write and vice versa.

Create with `LoopbackLink.pair()`.

#### LoopbackLink.pair *(classmethod)*

```python
pair(cls) -> tuple['LoopbackLink', 'LoopbackLink']
```

#### LoopbackLink.read

```python
read(self, timeout: float | None = None) -> bytes
```

#### LoopbackLink.write

```python
write(self, data: bytes) -> None
```

#### LoopbackLink.close

```python
close(self) -> None
```

#### LoopbackLink.closed *(property)*

## Protocol codecs

### Vl53l4Info

```python
class Vl53l4Info(int_edges: int, slots_skipped: int, i2c_errors: int, last_i2c_error: int, model_id: int, fw_status: int, initialized: int, xshut_level: int, int_level: int, i2c_khz: int) -> None
```

RPT_VL53_INFO — bridge diagnostics. Counters are free-running and wrap
silently; watch increments, not absolute values.

#### Vl53l4Info.unpack *(classmethod)*

```python
unpack(cls, payload: bytes) -> 'Vl53l4Info'
```

### FwDepzImage

```python
class FwDepzImage(load_addr: int, fw_size: int, fw_crc32: int, cur_sec: int, tot_sec: int, payload: bytes) -> None
```

Parsed and validated `.fwdepz` firmware container.

#### FwDepzImage.parse *(classmethod)*

```python
parse(cls, blob: bytes) -> 'FwDepzImage'
```

#### FwDepzImage.load *(classmethod)*

```python
load(cls, path: str | pathlib.Path) -> 'FwDepzImage'
```

#### FwDepzImage.build *(classmethod)*

```python
build(cls, load_addr: int, payload: bytes, *, cur_sec: int = 1, tot_sec: int = 1) -> bytes
```

Assemble a container (test/tooling helper; the fw_crc32 covers the
payload exactly as flashed).

#### FwDepzImage.payload_crc_ok *(property)*

### Cmd

```python
class Cmd(*values)
```

#### Cmd.conjugate

```python
conjugate
```

Returns self, the complex conjugate of any int.

#### Cmd.bit_length

```python
bit_length(self, /)
```

Number of bits necessary to represent self in binary.

>>> bin(37)
'0b100101'
>>> (37).bit_length()
6

#### Cmd.bit_count

```python
bit_count(self, /)
```

Number of ones in the binary representation of the absolute value of self.

Also known as the population count.

>>> bin(13)
'0b1101'
>>> (13).bit_count()
3

#### Cmd.to_bytes

```python
to_bytes(self, /, length=1, byteorder='big', *, signed=False)
```

Return an array of bytes representing an integer.

length
  Length of bytes object to use.  An OverflowError is raised if the
  integer is not representable with the given number of bytes.  Default
  is length 1.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Determines whether two's complement is used to represent the integer.
  If signed is False and a negative integer is given, an OverflowError
  is raised.

#### Cmd.from_bytes

```python
from_bytes(type, /, bytes, byteorder='big', *, signed=False)
```

Return the integer represented by the given array of bytes.

bytes
  Holds the array of bytes to convert.  The argument must either
  support the buffer protocol or be an iterable object producing bytes.
  Bytes and bytearray are examples of built-in objects that support the
  buffer protocol.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Indicates whether two's complement is used to represent the integer.

#### Cmd.as_integer_ratio

```python
as_integer_ratio(self, /)
```

Return a pair of integers, whose ratio is equal to the original int.

The ratio is in lowest terms and has a positive denominator.

>>> (10).as_integer_ratio()
(10, 1)
>>> (-10).as_integer_ratio()
(-10, 1)
>>> (0).as_integer_ratio()
(0, 1)

#### Cmd.is_integer

```python
is_integer(self, /)
```

Returns True. Exists for duck type compatibility with float.is_integer.

### Rpt

```python
class Rpt(*values)
```

#### Rpt.conjugate

```python
conjugate
```

Returns self, the complex conjugate of any int.

#### Rpt.bit_length

```python
bit_length(self, /)
```

Number of bits necessary to represent self in binary.

>>> bin(37)
'0b100101'
>>> (37).bit_length()
6

#### Rpt.bit_count

```python
bit_count(self, /)
```

Number of ones in the binary representation of the absolute value of self.

Also known as the population count.

>>> bin(13)
'0b1101'
>>> (13).bit_count()
3

#### Rpt.to_bytes

```python
to_bytes(self, /, length=1, byteorder='big', *, signed=False)
```

Return an array of bytes representing an integer.

length
  Length of bytes object to use.  An OverflowError is raised if the
  integer is not representable with the given number of bytes.  Default
  is length 1.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Determines whether two's complement is used to represent the integer.
  If signed is False and a negative integer is given, an OverflowError
  is raised.

#### Rpt.from_bytes

```python
from_bytes(type, /, bytes, byteorder='big', *, signed=False)
```

Return the integer represented by the given array of bytes.

bytes
  Holds the array of bytes to convert.  The argument must either
  support the buffer protocol or be an iterable object producing bytes.
  Bytes and bytearray are examples of built-in objects that support the
  buffer protocol.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Indicates whether two's complement is used to represent the integer.

#### Rpt.as_integer_ratio

```python
as_integer_ratio(self, /)
```

Return a pair of integers, whose ratio is equal to the original int.

The ratio is in lowest terms and has a positive denominator.

>>> (10).as_integer_ratio()
(10, 1)
>>> (-10).as_integer_ratio()
(-10, 1)
>>> (0).as_integer_ratio()
(0, 1)

#### Rpt.is_integer

```python
is_integer(self, /)
```

Returns True. Exists for duck type compatibility with float.is_integer.

### Status

```python
class Status(*values)
```

#### Status.conjugate

```python
conjugate
```

Returns self, the complex conjugate of any int.

#### Status.bit_length

```python
bit_length(self, /)
```

Number of bits necessary to represent self in binary.

>>> bin(37)
'0b100101'
>>> (37).bit_length()
6

#### Status.bit_count

```python
bit_count(self, /)
```

Number of ones in the binary representation of the absolute value of self.

Also known as the population count.

>>> bin(13)
'0b1101'
>>> (13).bit_count()
3

#### Status.to_bytes

```python
to_bytes(self, /, length=1, byteorder='big', *, signed=False)
```

Return an array of bytes representing an integer.

length
  Length of bytes object to use.  An OverflowError is raised if the
  integer is not representable with the given number of bytes.  Default
  is length 1.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Determines whether two's complement is used to represent the integer.
  If signed is False and a negative integer is given, an OverflowError
  is raised.

#### Status.from_bytes

```python
from_bytes(type, /, bytes, byteorder='big', *, signed=False)
```

Return the integer represented by the given array of bytes.

bytes
  Holds the array of bytes to convert.  The argument must either
  support the buffer protocol or be an iterable object producing bytes.
  Bytes and bytearray are examples of built-in objects that support the
  buffer protocol.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Indicates whether two's complement is used to represent the integer.

#### Status.as_integer_ratio

```python
as_integer_ratio(self, /)
```

Return a pair of integers, whose ratio is equal to the original int.

The ratio is in lowest terms and has a positive denominator.

>>> (10).as_integer_ratio()
(10, 1)
>>> (-10).as_integer_ratio()
(-10, 1)
>>> (0).as_integer_ratio()
(0, 1)

#### Status.is_integer

```python
is_integer(self, /)
```

Returns True. Exists for duck type compatibility with float.is_integer.

### SyncPinMode

```python
class SyncPinMode(*values)
```

#### SyncPinMode.conjugate

```python
conjugate
```

Returns self, the complex conjugate of any int.

#### SyncPinMode.bit_length

```python
bit_length(self, /)
```

Number of bits necessary to represent self in binary.

>>> bin(37)
'0b100101'
>>> (37).bit_length()
6

#### SyncPinMode.bit_count

```python
bit_count(self, /)
```

Number of ones in the binary representation of the absolute value of self.

Also known as the population count.

>>> bin(13)
'0b1101'
>>> (13).bit_count()
3

#### SyncPinMode.to_bytes

```python
to_bytes(self, /, length=1, byteorder='big', *, signed=False)
```

Return an array of bytes representing an integer.

length
  Length of bytes object to use.  An OverflowError is raised if the
  integer is not representable with the given number of bytes.  Default
  is length 1.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Determines whether two's complement is used to represent the integer.
  If signed is False and a negative integer is given, an OverflowError
  is raised.

#### SyncPinMode.from_bytes

```python
from_bytes(type, /, bytes, byteorder='big', *, signed=False)
```

Return the integer represented by the given array of bytes.

bytes
  Holds the array of bytes to convert.  The argument must either
  support the buffer protocol or be an iterable object producing bytes.
  Bytes and bytearray are examples of built-in objects that support the
  buffer protocol.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Indicates whether two's complement is used to represent the integer.

#### SyncPinMode.as_integer_ratio

```python
as_integer_ratio(self, /)
```

Return a pair of integers, whose ratio is equal to the original int.

The ratio is in lowest terms and has a positive denominator.

>>> (10).as_integer_ratio()
(10, 1)
>>> (-10).as_integer_ratio()
(-10, 1)
>>> (0).as_integer_ratio()
(0, 1)

#### SyncPinMode.is_integer

```python
is_integer(self, /)
```

Returns True. Exists for duck type compatibility with float.is_integer.

### SyncPinPolarity

```python
class SyncPinPolarity(*values)
```

#### SyncPinPolarity.conjugate

```python
conjugate
```

Returns self, the complex conjugate of any int.

#### SyncPinPolarity.bit_length

```python
bit_length(self, /)
```

Number of bits necessary to represent self in binary.

>>> bin(37)
'0b100101'
>>> (37).bit_length()
6

#### SyncPinPolarity.bit_count

```python
bit_count(self, /)
```

Number of ones in the binary representation of the absolute value of self.

Also known as the population count.

>>> bin(13)
'0b1101'
>>> (13).bit_count()
3

#### SyncPinPolarity.to_bytes

```python
to_bytes(self, /, length=1, byteorder='big', *, signed=False)
```

Return an array of bytes representing an integer.

length
  Length of bytes object to use.  An OverflowError is raised if the
  integer is not representable with the given number of bytes.  Default
  is length 1.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Determines whether two's complement is used to represent the integer.
  If signed is False and a negative integer is given, an OverflowError
  is raised.

#### SyncPinPolarity.from_bytes

```python
from_bytes(type, /, bytes, byteorder='big', *, signed=False)
```

Return the integer represented by the given array of bytes.

bytes
  Holds the array of bytes to convert.  The argument must either
  support the buffer protocol or be an iterable object producing bytes.
  Bytes and bytearray are examples of built-in objects that support the
  buffer protocol.
byteorder
  The byte order used to represent the integer.  If byteorder is 'big',
  the most significant byte is at the beginning of the byte array.  If
  byteorder is 'little', the most significant byte is at the end of the
  byte array.  To request the native byte order of the host system, use
  `sys.byteorder' as the byte order value.  Default is to use 'big'.
signed
  Indicates whether two's complement is used to represent the integer.

#### SyncPinPolarity.as_integer_ratio

```python
as_integer_ratio(self, /)
```

Return a pair of integers, whose ratio is equal to the original int.

The ratio is in lowest terms and has a positive denominator.

>>> (10).as_integer_ratio()
(10, 1)
>>> (-10).as_integer_ratio()
(-10, 1)
>>> (0).as_integer_ratio()
(0, 1)

#### SyncPinPolarity.is_integer

```python
is_integer(self, /)
```

Returns True. Exists for duck type compatibility with float.is_integer.

### SyncPinConfig

```python
class SyncPinConfig(pin: int, mode: depz_sensor_sdk.protocol.common.SyncPinMode, polarity: depz_sensor_sdk.protocol.common.SyncPinPolarity) -> None
```

#### SyncPinConfig.pack

```python
pack(self) -> bytes
```

#### SyncPinConfig.unpack *(classmethod)*

```python
unpack(cls, payload: bytes) -> 'SyncPinConfig'
```

### StatusReport

```python
class StatusReport(cmd: int, status: int) -> None
```

#### StatusReport.unpack *(classmethod)*

```python
unpack(cls, payload: bytes) -> 'StatusReport'
```

### TextReport

```python
class TextReport(cmd: int, text: str) -> None
```

#### TextReport.unpack *(classmethod)*

```python
unpack(cls, payload: bytes) -> 'TextReport'
```

### SyncTimeReport

```python
class SyncTimeReport(pc_timestamp_us: int, mcu_rx_us: int, mcu_tx_us: int) -> None
```

#### SyncTimeReport.unpack *(classmethod)*

```python
unpack(cls, payload: bytes) -> 'SyncTimeReport'
```

### TemperatureReport

```python
class TemperatureReport(timestamp_us: int, raw_decidegrees: int) -> None
```

#### TemperatureReport.unpack *(classmethod)*

```python
unpack(cls, payload: bytes) -> 'TemperatureReport'
```

#### TemperatureReport.celsius *(property)*

### SequenceErrorReport

```python
class SequenceErrorReport(expected_seq: int, received_seq: int) -> None
```

#### SequenceErrorReport.unpack *(classmethod)*

```python
unpack(cls, payload: bytes) -> 'SequenceErrorReport'
```

### UNSOLICITED

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

### pack_sync_time

```python
pack_sync_time(pc_timestamp_us: int) -> bytes
```

### sync_time_offset_rtt

```python
sync_time_offset_rtt(t1: int, t2: int, t3: int, t4: int) -> tuple[int, int]
```

NTP-style clock math, all µs (contract 02 §5).

Returns (offset_us, rtt_us) where offset = device_clock - host_clock,
computed with floor division toward zero on the sum (integer parity rule
shared by all SDKs: offset = ((T2-T1)+(T3-T4)) // 2 with truncation).

### strip_device_string

```python
strip_device_string(raw: bytes) -> str
```

Decode an ASCII device string, dropping trailing NUL/0xFF filler.

### Identity

```python
class Identity(mode: str, sensor_type: depz_sensor_sdk.protocol.identity.SensorType | None, software_name: str, version: str) -> None
```

### SensorType

```python
class SensorType(*values)
```

#### SensorType.encode

```python
encode(self, /, encoding='utf-8', errors='strict')
```

Encode the string using the codec registered for encoding.

encoding
  The encoding in which to encode the string.
errors
  The error handling scheme to use for encoding errors.
  The default is 'strict' meaning that encoding errors raise a
  UnicodeEncodeError.  Other possible values are 'ignore', 'replace' and
  'xmlcharrefreplace' as well as any other name registered with
  codecs.register_error that can handle UnicodeEncodeErrors.

#### SensorType.replace

```python
replace(self, old, new, count=-1, /)
```

Return a copy with all occurrences of substring old replaced by new.

  count
    Maximum number of occurrences to replace.
    -1 (the default value) means replace all occurrences.

If the optional argument count is given, only the first count occurrences are
replaced.

#### SensorType.split

```python
split(self, /, sep=None, maxsplit=-1)
```

Return a list of the substrings in the string, using sep as the separator string.

  sep
    The separator used to split the string.

    When set to None (the default value), will split on any whitespace
    character (including \n \r \t \f and spaces) and will discard
    empty strings from the result.
  maxsplit
    Maximum number of splits.
    -1 (the default value) means no limit.

Splitting starts at the front of the string and works to the end.

Note, str.split() is mainly useful for data that has been intentionally
delimited.  With natural text that includes punctuation, consider using
the regular expression module.

#### SensorType.rsplit

```python
rsplit(self, /, sep=None, maxsplit=-1)
```

Return a list of the substrings in the string, using sep as the separator string.

  sep
    The separator used to split the string.

    When set to None (the default value), will split on any whitespace
    character (including \n \r \t \f and spaces) and will discard
    empty strings from the result.
  maxsplit
    Maximum number of splits.
    -1 (the default value) means no limit.

Splitting starts at the end of the string and works to the front.

#### SensorType.join

```python
join(self, iterable, /)
```

Concatenate any number of strings.

The string whose method is called is inserted in between each given string.
The result is returned as a new string.

Example: '.'.join(['ab', 'pq', 'rs']) -> 'ab.pq.rs'

#### SensorType.capitalize

```python
capitalize(self, /)
```

Return a capitalized version of the string.

More specifically, make the first character have upper case and the rest lower
case.

#### SensorType.casefold

```python
casefold(self, /)
```

Return a version of the string suitable for caseless comparisons.

#### SensorType.title

```python
title(self, /)
```

Return a version of the string where each word is titlecased.

More specifically, words start with uppercased characters and all remaining
cased characters have lower case.

#### SensorType.center

```python
center(self, width, fillchar=' ', /)
```

Return a centered string of length width.

Padding is done using the specified fill character (default is a space).

#### SensorType.count

```python
count
```

Return the number of non-overlapping occurrences of substring sub in
string S[start:end].  Optional arguments start and end are
interpreted as in slice notation.

#### SensorType.expandtabs

```python
expandtabs(self, /, tabsize=8)
```

Return a copy where all tab characters are expanded using spaces.

If tabsize is not given, a tab size of 8 characters is assumed.

#### SensorType.find

```python
find
```

Return the lowest index in S where substring sub is found,
such that sub is contained within S[start:end].  Optional
arguments start and end are interpreted as in slice notation.

Return -1 on failure.

#### SensorType.partition

```python
partition(self, sep, /)
```

Partition the string into three parts using the given separator.

This will search for the separator in the string.  If the separator is found,
returns a 3-tuple containing the part before the separator, the separator
itself, and the part after it.

If the separator is not found, returns a 3-tuple containing the original string
and two empty strings.

#### SensorType.index

```python
index
```

Return the lowest index in S where substring sub is found,
such that sub is contained within S[start:end].  Optional
arguments start and end are interpreted as in slice notation.

Raises ValueError when the substring is not found.

#### SensorType.ljust

```python
ljust(self, width, fillchar=' ', /)
```

Return a left-justified string of length width.

Padding is done using the specified fill character (default is a space).

#### SensorType.lower

```python
lower(self, /)
```

Return a copy of the string converted to lowercase.

#### SensorType.lstrip

```python
lstrip(self, chars=None, /)
```

Return a copy of the string with leading whitespace removed.

If chars is given and not None, remove characters in chars instead.

#### SensorType.rfind

```python
rfind
```

Return the highest index in S where substring sub is found,
such that sub is contained within S[start:end].  Optional
arguments start and end are interpreted as in slice notation.

Return -1 on failure.

#### SensorType.rindex

```python
rindex
```

Return the highest index in S where substring sub is found,
such that sub is contained within S[start:end].  Optional
arguments start and end are interpreted as in slice notation.

Raises ValueError when the substring is not found.

#### SensorType.rjust

```python
rjust(self, width, fillchar=' ', /)
```

Return a right-justified string of length width.

Padding is done using the specified fill character (default is a space).

#### SensorType.rstrip

```python
rstrip(self, chars=None, /)
```

Return a copy of the string with trailing whitespace removed.

If chars is given and not None, remove characters in chars instead.

#### SensorType.rpartition

```python
rpartition(self, sep, /)
```

Partition the string into three parts using the given separator.

This will search for the separator in the string, starting at the end. If
the separator is found, returns a 3-tuple containing the part before the
separator, the separator itself, and the part after it.

If the separator is not found, returns a 3-tuple containing two empty strings
and the original string.

#### SensorType.splitlines

```python
splitlines(self, /, keepends=False)
```

Return a list of the lines in the string, breaking at line boundaries.

Line breaks are not included in the resulting list unless keepends is given and
true.

#### SensorType.strip

```python
strip(self, chars=None, /)
```

Return a copy of the string with leading and trailing whitespace removed.

If chars is given and not None, remove characters in chars instead.

#### SensorType.swapcase

```python
swapcase(self, /)
```

Convert uppercase characters to lowercase and lowercase characters to uppercase.

#### SensorType.translate

```python
translate(self, table, /)
```

Replace each character in the string using the given translation table.

  table
    Translation table, which must be a mapping of Unicode ordinals to
    Unicode ordinals, strings, or None.

The table must implement lookup/indexing via __getitem__, for instance a
dictionary or list.  If this operation raises LookupError, the character is
left untouched.  Characters mapped to None are deleted.

#### SensorType.upper

```python
upper(self, /)
```

Return a copy of the string converted to uppercase.

#### SensorType.startswith

```python
startswith
```

Return True if S starts with the specified prefix, False otherwise.
With optional start, test S beginning at that position.
With optional end, stop comparing S at that position.
prefix can also be a tuple of strings to try.

#### SensorType.endswith

```python
endswith
```

Return True if S ends with the specified suffix, False otherwise.
With optional start, test S beginning at that position.
With optional end, stop comparing S at that position.
suffix can also be a tuple of strings to try.

#### SensorType.removeprefix

```python
removeprefix(self, prefix, /)
```

Return a str with the given prefix string removed if present.

If the string starts with the prefix string, return string[len(prefix):].
Otherwise, return a copy of the original string.

#### SensorType.removesuffix

```python
removesuffix(self, suffix, /)
```

Return a str with the given suffix string removed if present.

If the string ends with the suffix string and that suffix is not empty,
return string[:-len(suffix)]. Otherwise, return a copy of the original
string.

#### SensorType.isascii

```python
isascii(self, /)
```

Return True if all characters in the string are ASCII, False otherwise.

ASCII characters have code points in the range U+0000-U+007F.
Empty string is ASCII too.

#### SensorType.islower

```python
islower(self, /)
```

Return True if the string is a lowercase string, False otherwise.

A string is lowercase if all cased characters in the string are lowercase and
there is at least one cased character in the string.

#### SensorType.isupper

```python
isupper(self, /)
```

Return True if the string is an uppercase string, False otherwise.

A string is uppercase if all cased characters in the string are uppercase and
there is at least one cased character in the string.

#### SensorType.istitle

```python
istitle(self, /)
```

Return True if the string is a title-cased string, False otherwise.

In a title-cased string, upper- and title-case characters may only
follow uncased characters and lowercase characters only cased ones.

#### SensorType.isspace

```python
isspace(self, /)
```

Return True if the string is a whitespace string, False otherwise.

A string is whitespace if all characters in the string are whitespace and there
is at least one character in the string.

#### SensorType.isdecimal

```python
isdecimal(self, /)
```

Return True if the string is a decimal string, False otherwise.

A string is a decimal string if all characters in the string are decimal and
there is at least one character in the string.

#### SensorType.isdigit

```python
isdigit(self, /)
```

Return True if the string is a digit string, False otherwise.

A string is a digit string if all characters in the string are digits and there
is at least one character in the string.

#### SensorType.isnumeric

```python
isnumeric(self, /)
```

Return True if the string is a numeric string, False otherwise.

A string is numeric if all characters in the string are numeric and there is at
least one character in the string.

#### SensorType.isalpha

```python
isalpha(self, /)
```

Return True if the string is an alphabetic string, False otherwise.

A string is alphabetic if all characters in the string are alphabetic and there
is at least one character in the string.

#### SensorType.isalnum

```python
isalnum(self, /)
```

Return True if the string is an alpha-numeric string, False otherwise.

A string is alpha-numeric if all characters in the string are alpha-numeric and
there is at least one character in the string.

#### SensorType.isidentifier

```python
isidentifier(self, /)
```

Return True if the string is a valid Python identifier, False otherwise.

Call keyword.iskeyword(s) to test whether string s is a reserved identifier,
such as "def" or "class".

#### SensorType.isprintable

```python
isprintable(self, /)
```

Return True if all characters in the string are printable, False otherwise.

A character is printable if repr() may use it in its output.

#### SensorType.zfill

```python
zfill(self, width, /)
```

Pad a numeric string with zeros on the left, to fill a field of the given width.

The string is never truncated.

#### SensorType.format

```python
format
```

Return a formatted version of S, using substitutions from args and kwargs.
The substitutions are identified by braces ('{' and '}').

#### SensorType.format_map

```python
format_map
```

Return a formatted version of S, using substitutions from mapping.
The substitutions are identified by braces ('{' and '}').

#### SensorType.maketrans

```python
maketrans
```

Return a translation table usable for str.translate().

If there is only one argument, it must be a dictionary mapping Unicode
ordinals (integers) or characters to Unicode ordinals, strings or None.
Character keys will be then converted to ordinals.
If there are two arguments, they must be strings of equal length, and
in the resulting dictionary, each character in x will be mapped to the
character at the same position in y. If there is a third argument, it
must be a string, whose characters will be mapped to None in the result.

### parse_software_name

```python
parse_software_name(name: str) -> depz_sensor_sdk.protocol.identity.Identity
```

Classify a GET_NAME_ACTIVE_SOFTWARE string.

The string must already be stripped of trailing NUL/0xFF
(`strip_device_string`).

## Errors

### DepzError

```python
class DepzError
```

Base class for all SDK errors.

#### DepzError.with_traceback

```python
with_traceback
```

Exception.with_traceback(tb) --
set self.__traceback__ to tb and return self.

#### DepzError.add_note

```python
add_note
```

Exception.add_note(note) --
add a note to the exception

### DepzTimeoutError

```python
class DepzTimeoutError(cmd: int, timeout: float)
```

A request got no matching reply within the timeout.

#### DepzTimeoutError.with_traceback

```python
with_traceback
```

Exception.with_traceback(tb) --
set self.__traceback__ to tb and return self.

#### DepzTimeoutError.add_note

```python
add_note
```

Exception.add_note(note) --
add a note to the exception

### StatusError

```python
class StatusError(cmd: int, status: int)
```

Device answered a request with a non-OK RPT_STATUS.

#### StatusError.with_traceback

```python
with_traceback
```

Exception.with_traceback(tb) --
set self.__traceback__ to tb and return self.

#### StatusError.add_note

```python
add_note
```

Exception.add_note(note) --
add a note to the exception

### BusyError

```python
class BusyError(cmd: int)
```

Device answered ERR_BUSY; the operation may be retried later.

#### BusyError.with_traceback

```python
with_traceback
```

Exception.with_traceback(tb) --
set self.__traceback__ to tb and return self.

#### BusyError.add_note

```python
add_note
```

Exception.add_note(note) --
add a note to the exception

### DeviceLostError

```python
class DeviceLostError
```

The serial link dropped (unplug, reboot) while in use.

#### DeviceLostError.with_traceback

```python
with_traceback
```

Exception.with_traceback(tb) --
set self.__traceback__ to tb and return self.

#### DeviceLostError.add_note

```python
add_note
```

Exception.add_note(note) --
add a note to the exception

### LinkClosedError

```python
class LinkClosedError
```

Operation attempted on a closed link/device.

#### LinkClosedError.with_traceback

```python
with_traceback
```

Exception.with_traceback(tb) --
set self.__traceback__ to tb and return self.

#### LinkClosedError.add_note

```python
add_note
```

Exception.add_note(note) --
add a note to the exception

### NoDepzDeviceError

```python
class NoDepzDeviceError
```

Discovery found no DEPZ device matching the request.

Raised by `open_device()`/discovery when no candidate serial port has a
known DEPZ USB identity (or none matches a requested serial/index).

#### NoDepzDeviceError.with_traceback

```python
with_traceback
```

Exception.with_traceback(tb) --
set self.__traceback__ to tb and return self.

#### NoDepzDeviceError.add_note

```python
add_note
```

Exception.add_note(note) --
add a note to the exception

## Top level

### DEPZ_USB_VID

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

### DEPZ_PID_MODEL

`dict` constant.

dict(mapping) -> new dictionary initialized from a mapping object's
    (key, value) pairs
dict(iterable) -> new dictionary initialized as if via:
    d = {}
    for k, v in iterable:
        d[k] = v
dict(**kwargs) -> new dictionary initialized with the name=value pairs
    in the keyword argument list.  For example:  dict(one=1, two=2)

### __version__

`str` constant.

str(bytes_or_buffer[, encoding[, errors]]) -> str

Create a new string object from the given object. If encoding or
errors is specified, then the object must expose a data buffer
that will be decoded using the given encoding and error handler.
Otherwise, returns the result of object.__str__() (if defined)
or repr(object).
encoding defaults to sys.getdefaultencoding().
errors defaults to 'strict'.

## Usb_Ids

### is_known_depz_usb

```python
is_known_depz_usb(vid: int | None, pid: int | None) -> bool
```

True when (vid, pid) is a recognized DEPZ (or dev-default) USB id.

### usb_model_hint

```python
usb_model_hint(vid: int | None, pid: int | None) -> str | None
```

Best-guess model name for a (vid, pid), or None.

Informational only — never used to decide how to decode a device; the
firmware-name probe does that.
