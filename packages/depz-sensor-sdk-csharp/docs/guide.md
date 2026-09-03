# Depz.Sensor — guide

The common guide to the C# SDK for the DEPZ USB sensor line. It covers what the
SDK is, installation, discovery, the shared mental model, and the cross-sensor
building blocks (transport framing, common protocol codecs, dataset replay,
firmware-image parse). Each sensor then has its own **introduction** and **user
guide**:

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
from the `///` XML-doc summaries so it never drifts from the code.

## Contents

- [What it is](#what-it-is)
- [Installation](#installation)
- [Getting started](#getting-started)
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
payloads to send back. It does not own a serial port or a reader thread; that
transport is the host application's concern.

Each sensor is a USB CDC-ACM device speaking one shared framed protocol
(`A5 C3` header + CRC). Five sensors across three firmware philosophies —
mirrored from the firmware:

- **SR04** — the device does the ranging; you decode `EchoTimeUs` → distance
  (`Depz.Sensor.Protocol.Sr04`).
- **VL53L4CD** — the device is a thin I2C register bridge; the full ST ULD
  runs on the host. This SDK ports the **verifiable codec/math layer**: the
  wire codecs, result-block decode and the range-timing / tuning-word math
  (`Depz.Sensor.Vl53l4`).
- **VL53L8CX / VL53L8CH** — the device is a thin SPI bridge; the full ST ULD
  runs on the host. This SDK ports the **verifiable decode path**: results-frame
  decode and the advanced DCI payload codecs (`Depz.Sensor.Vl53l8`). CX is the
  base ToF imager; CH is its superset, adding Compact-Network-Histogram output.
- **BNO086** — the device is an SHTP pass-through; the SH-2 stack runs on the
  host. This SDK ports SHTP framing/reassembly, the SH-2 control encoders and
  the input-report decoders (`Depz.Sensor.Bno086`).

The public surface is every `public` type under `src/Depz.Sensor/`; the API
reference is generated from it, so it never drifts from the code.

## Installation

Everything targets **.NET 8** (`net8.0`). It is published on NuGet as
`Depz.Sensor` (0.1.4):

```bash
dotnet add package Depz.Sensor
```

Or reference the project directly from a source checkout:

```bash
dotnet add reference path/to/src/Depz.Sensor/Depz.Sensor.csproj
```

```csharp
using Depz.Sensor.Transport;   // framing, CRC, PacketParser
using Depz.Sensor.Protocol;    // Cmd/Rpt/Status, common reports, SR04, identity
using Depz.Sensor.Vl53l4;      // single-zone ToF register bridge + ULD math
using Depz.Sensor.Vl53l8;      // ToF frame decode + DCI codecs
using Depz.Sensor.Bno086;      // SHTP + SH-2
using Depz.Sensor.Usb;         // USB identity table + discovery ordering
using Depz.Sensor.Dataset;     // .depzdata reader
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

`DatasetReader` reads the decoded, multi-device, time-synced `.depzdata` format
(contract 09; plain or gzip). Records from every device — including two of the
same model — are merged strictly by host time onto one timeline:

```csharp
using Depz.Sensor.Dataset;

var reader = new DatasetReader("run.depzdata");
foreach (DatasetRecord r in reader.Records)   // merged by ascending host time
    Console.WriteLine($"{r.DeviceId} {r.Kind} @ {r.THostUs}µs");

foreach (var (id, dev) in reader.Devices)      // per-device header metadata
    Console.WriteLine($"{id}: {dev.SensorType} serial={dev.Serial} sync={dev.TimeSync}");
```

Each `DatasetRecord.Value` is a `JsonElement` carrying the kind-specific payload;
read the fields present for the kind. The raw-byte capture layer (`.depzrec`)
is what the VL53L8 replay test feeds through the full codec stack byte-for-byte.

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

## Extension points

A few things are deliberately **stubbed, not faked**, because they cannot be
verified from golden vectors alone (rather than return fabricated data, they
throw a clearly-named exception or are declared as an explicit stub):

- **CNH histogram decode (VL53L8CH-specific)** — `Vl53l8Cnh.DecodeHistogram`.
  The shared frame decode already serves both CX and CH; CNH is the CH-only
  addition and has no golden vector yet. See the
  [VL53L8CH guide](vl53l8ch/guide.md).
- **Live VL53L8 ULD init/config** — `Vl53l8Uld.Init`. The firmware-download +
  register-bridge init sequence only means anything against real silicon over
  the CDC link, so it is out of scope for a decode SDK. See the
  [VL53L8CX guide](vl53l8cx/guide.md#advanced-dci-codecs).
- **Live VL53L4CD ULD driver** — the `sensor_init`/calibration register
  sequences over `ReadReg`/`WriteReg` are host code you write
  (`Vl53l4Uld.LiveDriverStubbed`); the codec/math layer itself is complete.
  See the [VL53L4CD guide](vl53l4cd/guide.md).
