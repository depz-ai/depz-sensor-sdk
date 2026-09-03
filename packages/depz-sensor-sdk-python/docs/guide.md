# depz-sensor-sdk — guide

The common guide to the Python SDK for the DEPZ USB sensor line. It covers what
the SDK is, installation, discovery, the shared mental model, and the
cross-sensor features (multi-device time-sync, recording & replay, firmware
update). Each sensor then has its own **introduction** and **user guide**:

- **SR04** (HC-SR04 ultrasonic) — [introduction](sr04/introduction.md) ·
  [guide](sr04/guide.md) · [api](sr04/api.md)
- **VL53L4CD** (single-zone ToF) — [introduction](vl53l4cd/introduction.md) ·
  [guide](vl53l4cd/guide.md) · [api](vl53l4cd/api.md)
- **VL53L8CX** (8×8 ToF base) — [introduction](vl53l8cx/introduction.md) ·
  [guide](vl53l8cx/guide.md) · [api](vl53l8cx/api.md)
- **VL53L8CH** (ToF superset + CNH histograms) —
  [introduction](vl53l8ch/introduction.md) · [guide](vl53l8ch/guide.md) ·
  [api](vl53l8ch/api.md)
- **BNO086** (9-axis IMU) — [introduction](bno086/introduction.md) ·
  [guide](bno086/guide.md) · [api](bno086/api.md)

For the exhaustive symbol-by-symbol reference see [api.md](api.md), generated
from the source docstrings so it never drifts from the code.

## Contents

- [What it is](#what-it-is)
- [Installation](#installation)
- [Getting started](#getting-started)
- [Discovery](#discovery)
- [Mental model](#mental-model)
- [Common device features](#common-device-features)
- [Streaming: callbacks vs iterators](#streaming-callbacks-vs-iterators)
- [Multi-sensor: one shared timeline](#multi-sensor-one-shared-timeline)
- [Recording & replay](#recording--replay)
- [Firmware update](#firmware-update)
- [Testing without hardware](#testing-without-hardware)

## What it is

Each sensor is a USB CDC-ACM device speaking one shared framed protocol
(`A5 C3` header + CRC). The SDK gives you a typed Python object per sensor with
a background reader thread; you configure it and consume a stream of decoded,
timestamped results. Five sensors across three firmware philosophies — mirrored
from the firmware:

- **SR04** — the device does the ranging; you get `echo_time_us` → distance.
- **VL53L4CD** — the device is a thin I2C bridge; the full ST ULD 2.2.3 runs
  **on the host** (`depz_sensor_sdk.vl53l4`). Single-zone ToF distance.
- **VL53L8CX / VL53L8CH** — the device is a thin SPI bridge; the full ST ULD
  driver runs **on the host** (`depz_sensor_sdk.vl53l8`). CX is the base ToF
  imager; CH is its superset, adding Compact-Network-Histogram output.
- **BNO086** — the device is an SHTP pass-through; the full SH-2 stack runs
  **on the host** (`depz_sensor_sdk.bno086`).

The public surface is the union of `__all__` across the package; the API
reference is generated from it, so it never drifts from the code.

## Installation

```bash
pip install depz-sensor-sdk        # once published
# from a source checkout, for development:
uv sync                            # installs the workspace incl. the SDK
```

Linux note: your user must be able to open the port (group `dialout`), and
`ModemManager` can grab CDC-ACM devices — disable it or add a udev rule if a
device is present but every open times out.

## Getting started

```python
from depz_sensor_sdk import open_device, Sr04, Bno086
from depz_sensor_sdk.vl53l8 import Vl53l8

dev = open_device("/dev/ttyACM0")   # factory: returns the right subclass
if isinstance(dev, Sr04):
    dev.start()
    print(next(dev.stream()).distance_mm)
```

`open_device()` probes the port and returns `Sr04`, `Vl53l8Cx`/`Vl53l8Ch`, or
`Bno086` — each a subclass of `DeviceBase`. From there, follow the per-sensor
guide linked above. Every device is a context manager (`with open_device(...)
as dev:`) or you call `dev.close()`.

## Discovery

Selection is by **USB identity**: only ports whose USB (vid, pid) is a known
DEPZ id are considered, ordered by USB iSerial — never "first port the OS
enumerated". The protocol probe (`GET_NAME_ACTIVE_SOFTWARE`) then decides what
each device actually *is*.

```python
from depz_sensor_sdk import list_depz_devices, open_device

for info in list_depz_devices():       # probes DEPZ-id ports only
    print(info.port, info.sensor_type, info.serial_number, info.usb_model_hint)

dev = open_device()                    # the DEPZ port with the smallest USB serial
dev = open_device(0)                   # by index (candidates sorted by serial)
dev = open_device(serial="SN000042")   # by exact USB serial
dev = open_device("/dev/ttyACM0")      # an explicit port path
```

Two identical sensors (say, two SR04) are told apart by their **USB serial**:
they sort deterministically, so `open_device(0)` / `open_device(1)` and
`serial=` are stable. Pass a bare port path to open an unprogrammed unit
(you'll get a warning if its USB id isn't a known DEPZ one).

## Mental model

```
open_device(port)  ──►  Sr04 | Vl53l8Cx/Ch | Bno086   (subclass of DeviceBase)
                          │
             background reader thread
                          │
        ┌─────────────────┼─────────────────┐
   request/reply      callbacks         bounded iterators
  (correlated by     (on_measurement/    (stream()/frames()/
   echoed cmd)        on_frame/           reports(); drop-oldest)
                      on_report)
```

- **One reader thread per device** drains the port, parses frames, dispatches.
  Solicited replies are correlated by the **echoed command byte** (not by
  sequence number); the default timeout is 200 ms.
- **Streams** come two ways: register a callback (fires on the reader thread —
  don't block it) *or* pull from a bounded, drop-oldest iterator.
- **Timestamps** are device microseconds. Call `sync_time()` once, then
  `to_host_time_us()` maps them onto the host clock — this is also what lets
  multi-device recordings share one timeline.

## Common device features

Available on every sensor (they all subclass `DeviceBase`):

```python
dev.read_mcu_temperature()             # °C
ts = dev.sync_time(samples=5)          # NTP-style; ts.offset_us / ts.rtt_us
dev.to_host_time_us(frame.timestamp_us)
dev.get_sync_pin(1); dev.set_sync_pin(cfg)   # AUX sync pins
dev.on_event(lambda ev: print(ev))     # seq errors, hw faults, text, temperature…
dev.stats                              # LinkStats: tx/rx, crc/seq errors, trash
dev.reset()                            # reboots the device
```

## Streaming: callbacks vs iterators

```python
# callback — fires on the reader thread; keep it quick
unsub = dev.on_frame(lambda f: queue.put(f))
unsub()                                # stop

# iterator — bounded, drop-oldest; subscribes immediately
it = dev.frames(maxsize=8)
frame = next(it)
print("dropped so far:", it.dropped_count)
```

The stream method is named per sensor: `stream()` (SR04), `frames()`
(VL53L8CX/CH), `reports()` (BNO086). Each drops the oldest item when full and
counts drops.

## Multi-sensor: one shared timeline

`sync_time_all()` runs `sync_time()` on several devices at once, so every
device's `to_host_time_us()` maps its own timestamps onto **one shared host
timeline** — the basis for correlating live reads across sensors (and the same
alignment `SessionRecorder` uses):

```python
from depz_sensor_sdk import sync_time_all

syncs = sync_time_all([sr04_a, sr04_b, imu])   # {device: TimeSync}
```

This works across mixed sensor types and across two of the same type.

## Recording & replay

Two layers (see contracts 08 and 09):

```python
# decoded, multi-device, time-synced dataset
from depz_sensor_sdk import SessionRecorder, DatasetReader

with SessionRecorder("run.depzdata", note="lab run") as rec:
    rec.add(sr04_a, device_id="left")    # runs sync_time; add several devices
    rec.add(sr04_b, device_id="right")   # two-of-a-kind is fine
    rec.add(vl53l8)
    rec.start()
    sr04_a.start(); sr04_b.start(); vl53l8.start_ranging()
    time.sleep(10)

reader = DatasetReader("run.depzdata")
for record in reader:                  # merged by host time across all devices
    print(record.device_id, record.kind, record.t_host_us)
```

Records from every device — including two of the same model — land on one
timeline, merged strictly by host time. CLI equivalents:

```bash
depz-sensor record-data /dev/ttyACM0 /dev/ttyACM1 --out two.depzdata
depz-sensor play-data two.depzdata --speed 2
```

The raw-byte layer (`.depzrec`, `RecordingLink` / `ReplayLink`) is for protocol
regression and hardware-free tests — the same file replays the full SDK stack
byte-for-byte.

## Firmware update

Updates the **application** firmware (not the bootloader):

```python
from depz_sensor_sdk import update_firmware
update_firmware("/dev/ttyACM0", "app_tof_vl53l8.fwdepz", progress=print)
# or: depz-sensor flash /dev/ttyACM0 app_tof_vl53l8.fwdepz
```

It enters the bootloader, re-finds the device by serial, erases, writes pages
with per-page CRC verification, checks the app CRC, and boots.

## Testing without hardware

Every layer takes a `Link`, so you can drive full device classes off a loopback
or a recorded session — no serial port required. That's how the test suite
runs, including a real VL53L8 capture replayed end-to-end.
