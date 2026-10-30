# Depz.Sensor — guide

The common guide to the C# SDK for the DEPZ USB sensor line. It covers what the
SDK is, installation, discovery, the shared mental model, and the cross-sensor
building blocks (transport framing, common protocol codecs, dataset replay,
firmware-image parse). Each sensor then has its own **introduction** and **user
guide**:

- **SR04** (HC-SR04 ultrasonic) — [introduction](sr04/introduction.md) ·
  [guide](sr04/guide.md) · [api](sr04/api.md)
- **VL53L0X / VL53L1CX / VL53L1CB / VL53L3CX / VL53L4CX** (the 1D ToF family) —
  [VL53L0X](vl53l0x/introduction.md) · [VL53L1CX](vl53l1cx/introduction.md) ·
  [VL53L1CB](vl53l1cb/introduction.md) · [VL53L3CX](vl53l3cx/introduction.md) ·
  [VL53L4CX](vl53l4cx/introduction.md), each with its guide and api
- **VL53L4CD** (single-zone ToF) — [introduction](vl53l4cd/introduction.md) ·
  [guide](vl53l4cd/guide.md) · [api](vl53l4cd/api.md)
- **VL53L5CX / VL53L7CX** (8×8 ToF on the I2C board, 63° / 90°) —
  [VL53L5CX](vl53l5cx/introduction.md) ([guide](vl53l5cx/guide.md) ·
  [api](vl53l5cx/api.md)) · [VL53L7CX](vl53l7cx/introduction.md)
  ([guide](vl53l7cx/guide.md) · [api](vl53l7cx/api.md))
- **VL53L7CH** (VL53L7CX + CNH histograms) —
  [introduction](vl53l7ch/introduction.md) · [guide](vl53l7ch/guide.md) ·
  [api](vl53l7ch/api.md)
- **VL53L8CX** (8×8 ToF base) — [introduction](vl53l8cx/introduction.md) ·
  [guide](vl53l8cx/guide.md) · [api](vl53l8cx/api.md)
- **VL53L8CH** (ToF superset + CNH histograms) —
  [introduction](vl53l8ch/introduction.md) · [guide](vl53l8ch/guide.md) ·
  [api](vl53l8ch/api.md)
- **TMF8829** (dToF, 8×8 … 48×32 zones, up to 4 peaks) —
  [introduction](tmf8829/introduction.md) · [guide](tmf8829/guide.md) ·
  [api](tmf8829/api.md)
- **BNO055** (9-axis IMU, on-chip fusion) — [introduction](bno055/introduction.md) ·
  [guide](bno055/guide.md) · [api](bno055/api.md)
- **BNO085** (9-axis IMU) — [introduction](bno085/introduction.md) ·
  [guide](bno085/guide.md) · [api](bno085/api.md)
- **BNO086** (9-axis IMU) — [introduction](bno086/introduction.md) ·
  [guide](bno086/guide.md) · [api](bno086/api.md)

For the exhaustive symbol-by-symbol reference see [api.md](api.md), generated
from the `///` XML-doc summaries so it never drifts from the code.

## Contents

- [What it is](#what-it-is)
- [Installation](#installation)
- [Getting started](#getting-started)
- [Live devices](#live-devices)
- [Discovery](#discovery)
- [Mental model](#mental-model)
- [Common protocol codecs](#common-protocol-codecs)
- [Transport and CRC](#transport-and-crc)
- [Datasets: record and replay](#datasets-record-and-replay)
- [Firmware update](#firmware-update)
- [Golden vectors and testing](#golden-vectors-and-testing)
- [Extension points](#extension-points)

## What it is

`Depz.Sensor` is a **contract-first decode/encode SDK** for the DEPZ sensor
family. Every codec is a byte-exact port of the reference Python SDK, pinned to
the shared golden vectors under `contracts/vectors` (`dotnet test`). It is the
**codec layer**: you feed it bytes (from a serial port, a capture, a test
harness) and it gives you typed, decoded results — and it encodes the command
payloads to send back. On top of it sits the **device layer** (contract 07):
a connection with its reader thread, requests and replies, streams, events,
`.depzrec` record/replay and port discovery — see [Live devices](#live-devices).
The device layer has a class for every sensor — the **SR04**, the
**VL53L4CD**, the **VL53L 1D family** (VL53L0X / L1CX / L1CB / L3CX / L4CX),
the **VL53L8CX / VL53L8CH**, the **VL53L5CX / VL53L7CX / VL53L7CH**, the
**BNO055**, the **BNO085 / BNO086** and the **TMF8829**; a board it does not
recognise opens as a plain `DepzDevice` (common commands).

Each sensor is a USB CDC-ACM device speaking one shared framed protocol
(`A5 C3` header + CRC). The sensors fall into a few firmware philosophies:

- **SR04** — the device does the ranging; you decode `EchoTimeUs` → distance
  (`Depz.Sensor.Protocol.Sr04`).
- **VL53L0X / L1CX / L1CB / L3CX / L4CX** — one I2C register-bridge firmware
  for the 1D family; every ST driver runs on the host. `Vl53lxDevice` (and
  `Vl53l0xDevice` … `Vl53l4cxDevice`) binds a product and a driver kind: the
  VL53L0X PAL (`Vl53l0xUldDriver`), the VL53L1X / L4CD ULDs and the L3CX ULP
  (`Vl53l1Die`), and ST's histogram BareDriver with its multi-target
  post-processing (`Vl53lxHistogramDriver`, `Depz.Sensor.Vl53lx.Bare`) — over
  the v2.00 codecs, the product table and the block decode (`Depz.Sensor.Vl53lx`).
- **VL53L4CD** — the device is a thin I2C register bridge; the full ST ULD
  runs on the host: `Vl53l4cdDevice` with the ULD 2.2.3 port
  (`Vl53l4cdDriver`), over the wire codecs, result-block decode and the
  range-timing / tuning-word math (`Depz.Sensor.Vl53l4`).
- **VL53L5CX / VL53L7CX / VL53L7CH** — one I2C register-bridge firmware for
  three boards; frames have the VL53L8 layout. `Vl53l5cxDevice` /
  `Vl53l7cxDevice` / `Vl53l7chDevice` run the VL53L8 ULD port on its L5/L7
  branch, over the board codecs, class resolution and the L5/L7 frame decoder
  (`Depz.Sensor.Vl53l7`), with CNH through `Vl53l8CnhSetup` / `Vl53l8Cnh`.
- **VL53L8CX / VL53L8CH** — the device is a thin SPI bridge; the full ST ULD
  runs on the host: `Vl53l8cxDevice` / `Vl53l8chDevice` with the ULD port
  (`Vl53l8Driver`, which downloads the sensor firmware embedded in the
  assembly), over the results-frame decode and the advanced DCI payload codecs
  (`Depz.Sensor.Vl53l8`). CX is the
  base ToF imager; CH is its superset, adding Compact-Network-Histogram output.
- **TMF8829** — a thin SPI register bridge; the host runs the sensor (firmware
  download, config page, presets): `Tmf8829Device`, over the wire codecs, chunk
  reassembly, frame and pixel decode with the half-frame merge, the clock
  model and capture time, the config-page fields and the embedded ams
  firmware image (`Depz.Sensor.Tmf8829`).
- **BNO055** — an I2C register bridge; Bosch's fusion runs on the chip.
  `Bno055Device` runs the session logic over the wire codecs, the register
  codecs and the register-window decode (`Depz.Sensor.Bno055`).
- **BNO085 / BNO086** — the device is an SHTP pass-through; the SH-2 stack runs on the
  host: `Bno086Device`, over SHTP framing/reassembly, the SH-2 control
  encoders and response decoders and the input-report decoders
  (`Depz.Sensor.Bno086`).

The public surface is every `public` type under `src/Depz.Sensor/`; the API
reference is generated from it, so it never drifts from the code.

## Installation

The package carries two builds: **.NET 8** (`net8.0`) and **.NET Standard 2.1**
(`netstandard2.1`, for Unity 2021.2 and newer). It is published on NuGet as
`Depz.Sensor`:

```bash
dotnet add package Depz.Sensor
```

Or reference the project directly from a source checkout:

```bash
dotnet add reference path/to/src/Depz.Sensor/Depz.Sensor.csproj
```

### Unity

Unity reads the `netstandard2.1` build: the **shared library**, which opens no
port by itself. Ports are the job of a **communication module**, replaceable as
a whole; the one shipped is for **Windows**, as the UPM package
[`unity/`](../unity/README.md) (`ai.depz.sensor`): `WindowsSerialLink`,
registry-based port discovery and `DepzDevicesUnity`, with install by git URL
or by copying, and a sample `MonoBehaviour`. Current Unity LTS (6000.0+), API
compatibility level **.NET Framework**.

The shared library alone (any platform, your own `ILink`): install `Depz.Sensor`
with [NuGetForUnity](https://github.com/GlitchEnzo/NuGetForUnity), or take
`lib/netstandard2.1/Depz.Sensor.dll` out of the `.nupkg` (a zip file) and put it
under `Assets/Plugins/`. It has no dependency outside the .NET Standard 2.1
library, so nothing else needs to be copied.

```csharp
using Depz.Sensor.Transport;   // framing, CRC, PacketParser
using Depz.Sensor.Protocol;    // Cmd/Rpt/Status, common reports, SR04, identity
using Depz.Sensor.Vl53l4;      // single-zone ToF register bridge + ULD math
using Depz.Sensor.Vl53l8;      // ToF frame decode + DCI codecs
using Depz.Sensor.Bno086;      // SHTP + SH-2
using Depz.Sensor.Usb;         // USB identity table + discovery ordering
using Depz.Sensor.Dataset;     // .depzdata record, read, replay
```

Toolchain (this repo is developed with a user-local dotnet):

```bash
export PATH="$HOME/.dotnet:$PATH"; export DOTNET_ROOT="$HOME/.dotnet"
dotnet test packages/depz-sensor-sdk-csharp   # 33/33 golden-vector tests
```

## Getting started

The entry point is the framing parser: feed it arbitrary byte chunks off the
wire, get back decoded packets, then hand each packet's payload to the matching
sensor codec.

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Protocol;

var parser = new PacketParser();
foreach (ParserEvent ev in parser.Feed(bytesFromSerialPort))
{
    if (ev is Packet pkt && pkt.Cmd == (int)Sr04Rpt.Data)
    {
        Sr04Data d = Sr04Data.Unpack(pkt.Payload);
        double? mm = Sr04.DistanceMmFromEcho(d.EchoTimeUs);
        Console.WriteLine(mm is null ? "no echo" : $"{mm:F1} mm");
    }
}
```

`PacketParser.Feed` is incremental — chunk boundaries don't matter — and returns
a mix of `Packet`, `Trash` (resync bytes) and `CrcError` events. From there,
follow the per-sensor guide linked above.

## Live devices

`DepzDevices` finds and opens a board as its sensor class; the class talks to
it over a background reader thread (`Depz.Sensor.Devices`):

```csharp
using Depz.Sensor.Devices;

using var dev = DepzDevices.OpenSr04();          // smallest USB serial; or OpenSr04(serial: "…")
Console.WriteLine(dev.GetSoftwareName());        // APP_usonic_SR04_v0.97
Sr04Measurement once = dev.MeasureOnce();

using var loop = dev.Measurements();             // subscribe before Start(): nothing is missed
dev.Start();
for (int i = 0; i < 20; i++)
    Console.WriteLine(loop.Take(TimeSpan.FromSeconds(1)).DistanceMm);
dev.Stop();
```

- **Requests** block the calling thread until the reply, at most `dev.Timeout`
  (200 ms by default). A board error throws `StatusException` (`BusyException`
  for ERR_BUSY), no reply `DepzTimeoutException`, a lost connection
  `DeviceLostException`; an argument outside its wire field throws
  `ArgumentOutOfRangeException` before anything is sent.
- **Streams** come two ways: `On…(callback)` runs on the reader thread (keep it
  short, never block), and `Measurements()` (`Events()`, …) returns a
  `Subscription<T>` — a bounded queue that drops the oldest item when full and
  counts it in `DroppedCount`. `Take(timeout)` waits, `TryTake(out item)` and
  `Drain()` never block. Dispose a subscription to stop it.
- **Events** (`OnEvent`, `Events()`): unsolicited status (hardware fault),
  sequence gaps, CRC errors, text, temperature, disconnect.
- **Time**: `SyncTime()` measures device clock − host clock;
  `ToHostTimeUs(deviceUs)` then puts device timestamps on the host clock
  (`DepzDevice.HostNowUs()`).
- **Record / replay**: wrap any link in `RecordingLink` to write a `.depzrec`
  capture (contract 08); `ReplayLink` plays one back, and
  `DepzDevices.OpenOnLink(link)` opens whatever the capture holds. The SDK's
  tests run the shared device scenarios (contract 15) this way.

### Unity

The shared library opens no port in Unity; the **Windows communication module**
([`unity/`](../unity/README.md), UPM package `ai.depz.sensor`) does.
`DepzDevicesUnity` mirrors `DepzDevices` — `OpenDevice`, `OpenSr04` …
`OpenTmf8829`, `OpenPort`, `ListDepzDevices`, `ProbePort` — over
`WindowsSerialPorts` (COM ports with USB VID / PID / serial from the registry,
behind `IRegistry` so it is testable with a fake — the shared library's code,
which `SerialPorts.List()` runs on .NET 8 too; the module brings the registry
reader, `WindowsRegistry`) and `WindowsSerialLink` (an
`ILink` over `System.IO.Ports.SerialPort`: 115200 8N1, DTR / RTS raised, short
reads so `Close()` never waits on Mono's port). Selection is contract 02 §4.3
(DEPZ USB ids, ordered by USB serial); the class comes from the probe, run on
the one connection the device keeps (`DepzDevices.OpenOnLink`). Errors are the
same types: `DeviceLostException`, `NoDepzDeviceException`, `WrongTypeException`.

```csharp
using Depz.Sensor.Devices;
using Depz.Sensor.Unity;

public sealed class RangeFinder : DepzSensorBehaviour     // the package's sample: open / poll / close
{
    Subscription<Sr04Measurement>? _loop;

    protected override void OnOpened(DepzDevice device)   // Start(): DepzDevicesUnity.OpenDevice() succeeded
    {
        var sr04 = DepzDevices.Expect<Sr04Device>(device, "an SR04");
        _loop = sr04.Measurements();
        sr04.Start();
    }

    protected override void Update()
    {
        base.Update();                                    // device events: disconnect, faults
        while (_loop != null && _loop.TryTake(out var m))
            UnityEngine.Debug.Log($"{m.DistanceMm} mm");
    }
}
```

Poll the queues from `Update()`: the reader thread must never touch Unity
objects. Limits, honestly: discovery is **Windows only** for now (off Windows it
finds nothing; `OpenPort("/dev/tty…")` may work where Mono has ports, untested);
macOS / Linux .NET users keep the .NET 8 build's `SerialPortLink`; Android,
iOS and WebGL are not covered. Another transport is another module: implement
`ILink` and call `DepzDevices.OpenOnLink(link, usbVid, usbPid)` — the USB ids
are optional and only break the VL53L8CX / CH and VL53L5CX / L7CX / L7CH ties
(`DepzDevices.ProbeLink(link)` runs the probe alone).

## Discovery

Selection is by **USB identity**: only ports whose USB (vid, pid) is a known
DEPZ id are considered, ordered by USB iSerial — never "first port the OS
enumerated". The protocol probe (`GET_NAME_ACTIVE_SOFTWARE`, `Cmd.GetNameActiveSoftware`)
then decides what each device actually *is*.

```csharp
using Depz.Sensor.Usb;
using Depz.Sensor.Protocol;

bool known = UsbIds.IsKnownDepzUsb(vid: 0x1BCF, pid: 0xEC78);   // true → SR04
string? hint = UsbIds.UsbModelHint(0x1BCF, 0xEC78);            // "sr04" (informational)

// Deterministic candidate order: by USB serial ascending, no-serial last.
var ports = new[]
{
    new UsbIds.PortCandidate("/dev/ttyACM1", "SN000042"),
    new UsbIds.PortCandidate("/dev/ttyACM0", "SN000041"),
};
IReadOnlyList<UsbIds.PortCandidate> ordered = UsbIds.SerialOrdering(ports);

// Classify the probe reply (already stripped of NUL/0xFF filler):
Identity id = IdentityParser.ParseSoftwareName("APP_SR04_v1.2.3");
// id.Mode == "app", id.SensorType == SensorType.Sr04, id.Version == "1.2.3"
```

On .NET 8, `DepzDevices.ListDepzDevices()` probes every port with a known
DEPZ USB id and returns a `DeviceInfo` per board; `OpenDevice()` /
`OpenDevice(index)` / `OpenDevice(serial: …)` / `OpenPort(path)` open one as its
class. Ports and their USB ids come from sysfs on Linux and the registry on
Windows; on macOS the USB ids are not read, so open a port path explicitly.

Two identical sensors are told apart by their **USB serial**: they sort
deterministically, so index 0/1 and serial lookups are stable. The PID→model map
(`UsbIds.DepzPidModel`) is a hint only; `IdentityParser` is authoritative.

## Mental model

```
raw bytes  ──►  PacketParser  ──►  Packet(Cmd, Seq, Payload)
                                        │
        ┌───────────────────┬───────────┴────────────┬────────────────────┐
     SR04            VL53L4CD (ToF)            VL53L8 (ToF)            BNO086
 Sr04Data.Unpack   Vl53l4StreamData          FrameReassembler ──►   ShtpLayer.Feed ──►
 Sr04.Distance…      .Unpack ──►             Vl53l8FrameDecoder     Sh2Reports.Parse…
                   Vl53l4Uld                   (CX/CH shared           (SHTP → SH-2)
                     .ParseResultBlock          decode)
```

- **`PacketParser`** drains the transport layer: it hunts the `A5 C3` magic,
  validates the 7-byte header CRC-8, checks the optional payload CRC, and emits
  `Packet` / `Trash` / `CrcError`. Byte-exact with the firmware (including the
  empty-payload-never-carries-CRC rule, ERRATA E6).
- **Correlation** is by the **echoed command byte**: a report's `Cmd` echoes the
  request opcode (or `Reports.Unsolicited` = `0x00` for spontaneous reports).
- **Two sensors add a reassembly layer above packets**: VL53L8 ships each frame
  in ≤1528-byte chunks (`FrameReassembler`); BNO086 wraps SH-2 in SHTP cargos
  that can span frames (`ShtpLayer`). The VL53L4CD's 17-byte result block fits
  in a single packet — no reassembly.
- **Timestamps** are device microseconds. `Common.SyncTimeOffsetRtt` does the
  NTP-style clock math; a dataset stores the resulting `TimeSync` per device so
  every record lands on one shared host timeline.

## Common protocol codecs

Shared across every sensor (`Depz.Sensor.Protocol`, contract 02):

```csharp
// Command / report / status opcode enums:
byte[] frame = Framing.BuildPacket((int)Cmd.SyncTime, Common.PackSyncTime(hostUs));

// Typed common reports (decode a Packet.Payload):
var temp = TemperatureReport.Unpack(payload);   // temp.Celsius (°C)
var sync = SyncTimeReport.Unpack(payload);       // T1/T2/T3 µs
var st   = StatusReport.Unpack(payload);         // echoed Cmd + Status code
var txt  = TextReport.Unpack(payload);           // echoed Cmd + ASCII text

// NTP-style time-sync math (device_clock − host_clock, and RTT):
(long offsetUs, long rttUs) = Common.SyncTimeOffsetRtt(t1, t2, t3, t4);

// AUX sync pins (pin 1..5, mode, polarity):
byte[] cfg = new SyncPinConfig(1, SyncPinMode.In, SyncPinPolarity.IdleLow).Pack();
```

`Status` (Ok, ErrBusy, ErrHardwareFault, …) is the shared result enum; a report
echoing your request opcode with a non-`Ok` status is the device rejecting it.

## Transport and CRC

The framing layer is a small, byte-exact set of codecs (contract 01):

```csharp
using Depz.Sensor.Transport;

byte[] frame = Framing.BuildPacket(cmd, payload, seq: 0, crcType: CrcType.Crc16);
byte crc8   = Crc.Crc8Maxim(header4);     // header CRC (init 0x00, ERRATA E1)
ushort m    = Crc.Crc16Modbus(payload);   // optional payload CRC
uint iso     = Crc.Crc32IsoHdlc(payload);

var parser = new PacketParser();
IReadOnlyList<ParserEvent> events = parser.Feed(chunk);
// parser.Packets / .CrcErrors / .HeaderErrors / .TrashBytes are live counters.
```

`CrcType` (None/Crc8/Crc16/Crc32) is carried in the two high bits of the
data-size field; `CrcTypeExtensions.Size()` gives its trailer length. Empty
payloads never carry a CRC trailer (ERRATA E6) — `Framing.PayloadCrcBytes`
enforces it.

## Datasets: record and replay

A `.depzdata` file (contract 09) holds **decoded** results of one or more
devices on one host timeline: JSON Lines, plain or gzip (`.depzdata.gz`).

**Record.** `SessionRecorder` hooks the result stream of every device class and
writes one record per measurement, frame or report, stamped with the device
time mapped onto the host clock (`t = device µs − offset`; `Add` runs
`SyncTime` for that offset). It never configures a device — start and stop
streaming yourself:

```csharp
using Depz.Sensor.Dataset;
using Depz.Sensor.Devices;

using var sr04 = DepzDevices.OpenSr04();
using var tof = DepzDevices.OpenVl53l8();
tof.Init();

using (var rec = new SessionRecorder("run.depzdata", note: "desk test"))
{
    rec.Add(sr04);                  // d0
    rec.Add(tof);                   // d1
    rec.Start();
    sr04.Start();
    tof.StartRanging();
    Thread.Sleep(10_000);
    sr04.Stop();
    tof.StopRanging();
}                                   // Dispose: unhook, close the file
```

Every class has its kind — `sr04`, `vl53l4`, `vl53l8` (+ `vl53l8_cnh` for an
armed CH), `vl53lx`, `bno055`, `bno086`, `tmf8829` — plus about one
`temperature` record per second and device (`temperatureHz`; the MCU, and the
VL53L8 die). `DatasetRecords` builds each record value, for a writer of your
own over `DatasetWriter`.

**Read.** `DatasetReader` merges the records of every device — two of the same
model included — stably by host time:

```csharp
using Depz.Sensor.Dataset;

var reader = new DatasetReader("run.depzdata");
foreach (DatasetRecord r in reader.Records)   // merged by ascending host time
    Console.WriteLine($"{r.DeviceId} {r.Kind} @ {r.THostUs}µs");

foreach (var (id, dev) in reader.Devices)      // per-device header metadata
    Console.WriteLine($"{id}: {dev.SensorType} serial={dev.Serial} sync={dev.TimeSync}");
Console.WriteLine($"{reader.DurationUs / 1e6:F1} s");
```

Each `DatasetRecord.Value` is the kind-specific payload as a dictionary keyed by
the contract 09 field names; read the fields present for the kind. Values are
plain objects — `long`, `double`, `string`, `bool`, `null`, `List<object?>` and
nested dictionaries — as `Depz.Sensor.Json.JsonText.Parse` returns them
(the SDK does not use System.Text.Json, which Unity lacks):

```csharp
long echoUs = (long)r.Value["echo_us"]!;   // an "sr04" record
```

**Replay.** `reader.Play(callback, speed)` delivers the records paced by their
timestamps (2 = twice as fast, 0 = at once) and blocks. For a viewer timeline,
`DatasetPlayer` adds play / pause / seek / speed (contract 09 §Player):

```csharp
using var player = new DatasetPlayer(reader, speed: 2.0);
player.OnRecord(r => Console.WriteLine($"{r.Kind} {r.THostUs}"));   // player thread; don't block
player.OnState(s => Console.WriteLine(s));
player.Play();
player.SeekUs(5_000_000);       // the next record is the first at >= 5 s from the start
player.Wait();                  // until done (or paused)
```

`threaded: false` starts no thread: call `Tick()` from your own loop (a Unity
`Update()`); it returns the seconds until the next record is due.

The raw-byte capture layer (`.depzrec`, contract 08) is a different thing: one
device's link, byte for byte, which `ReplayLink` plays back through the full
device stack.

## Firmware update

`.fwdepz` is the application-firmware container (not the bootloader). This SDK
parses and validates it (contract 06 §2); the flashing transport itself is the
host application's job:

```csharp
using Depz.Sensor.Protocol;

FwDepz.FwDepzImage img = FwDepz.FwDepzImage.Parse(File.ReadAllBytes("app.fwdepz"));
Console.WriteLine($"load=0x{img.LoadAddr:X} size={img.FwSize} crcOk={img.PayloadCrcOk}");
// Parse throws FwDepz.FwDepzException (Code: too_short / magic / header_crc / size).
```

Validation order is fixed: length, magic (`FWDEPZ00`), header CRC-16/CCITT-FALSE
over bytes [0..61], then `fw_size == payload length`, then a CRC-32 over the
payload (`PayloadCrcOk`).

## Golden vectors and testing

Every codec is pinned to the shared golden vectors in `contracts/vectors`, so
the C# output is byte-identical to the reference Python SDK and the firmware.
The suite runs with no hardware:

```bash
dotnet test packages/depz-sensor-sdk-csharp     # 23/23
```

It covers CRC KATs, framing encode/decode, USB identity + discovery ordering,
the common commands, SR04 / VL53L4CD / VL53L8 / BNO086 codecs, the `.fwdepz` parse, the
`.depzdata` read, and a recorded `.depzrec` VL53L8 replay driven end-to-end
through `PacketParser` → `FrameReassembler` → `Vl53l8FrameDecoder`.

Line coverage of the same suite, per class, with no tool beyond
`coverlet.collector` (in the test project) and Python 3:

```bash
scripts/coverage.sh           # from packages/depz-sensor-sdk-csharp; --all lists every class
```

The Cobertura XML lands in `tests/Depz.Sensor.Tests/TestResults/coverage/`.
Only what needs a serial port (`SerialPortLink`, `SerialPorts`) has no
hardware-free test.

## Extension points

A few things are deliberately **stubbed, not faked**, because they cannot be
verified from golden vectors alone (rather than return fabricated data, they
throw a clearly-named exception or are declared as an explicit stub):

- **Live drivers of the 1D family and the TMF8829** — the ST drivers (1D
  family) and the sensor sequence (TMF8829: soft reset, firmware download,
  configure, start/stop). The codecs and the stateless decode are here; see
  the per-sensor guides.

The CNH histogram block of the VL53L7CH / VL53L8CH is decoded:
`Vl53l8Cnh.DecodeHistogram` unpacks `Vl53l8Frame.CnhRaw` into per-aggregate
histograms.
