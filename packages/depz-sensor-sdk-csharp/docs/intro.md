# C# SDK

`Depz.Sensor` is a **contract-first decode/encode SDK** for the DEPZ USB
sensor line, targeting **.NET 8**. It is the codec layer: feed it bytes (from
a serial port, a capture, a test harness) and it gives you typed, decoded
results — and encodes the command payloads to send back. Every codec is a
byte-exact port of the reference Python SDK, pinned to the shared golden
vectors.

**Working with one specific sensor?** Each has its own introduction, guide
and API reference:

- **[SR04](sr04/introduction.md)** — ultrasonic ranging: one distance per ping.
- **[VL53L4CD](vl53l4cd/introduction.md)** — single-zone ToF: precise
  one-point distance.
- **[VL53L8CX](vl53l8cx/introduction.md)** — 8×8 ToF depth frames.
- **[VL53L8CH](vl53l8ch/introduction.md)** — the CX superset with CNH histograms.
- **[BNO086](bno086/introduction.md)** — 9-axis IMU: orientation and motion.

## Install

Published on NuGet as `Depz.Sensor` (0.1.4):

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

## Where to next

- **Your sensor's pages** — [SR04](sr04/introduction.md) ·
  [VL53L4CD](vl53l4cd/introduction.md) ·
  [VL53L8CX](vl53l8cx/introduction.md) · [VL53L8CH](vl53l8ch/introduction.md) ·
  [BNO086](bno086/introduction.md): introduction, hands-on guide, and the
  sensor's own API reference.
- **[Guide](guide.md)** — the SDK-wide walkthrough: installation, getting
  started, discovery, the mental model, transport & CRC, dataset replay.
- **[API Reference](api.md)** — the whole public surface, generated from the
  XML-doc summaries.
- **Docs for LLMs** — the sensor SDK documentation as raw Markdown:
  [/llms-full.txt](/llms-full.txt).
- **Source & license** — the SDKs are open source (MIT):
  [github.com/depz-ai/depz-sensor-sdk](https://github.com/depz-ai/depz-sensor-sdk).
