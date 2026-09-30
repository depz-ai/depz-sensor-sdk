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
- **[VL53L5CX](vl53l5cx/introduction.md)** / **[VL53L7CX](vl53l7cx/introduction.md)**
  — 8×8 ToF depth frames on the I2C board, 63° / 90° field of view.
- **[VL53L7CH](vl53l7ch/introduction.md)** — the VL53L7CX with CNH histograms.
- **[VL53L0X](vl53l0x/introduction.md)** · **[VL53L1CX](vl53l1cx/introduction.md)** ·
  **[VL53L1CB](vl53l1cb/introduction.md)** · **[VL53L3CX](vl53l3cx/introduction.md)** ·
  **[VL53L4CX](vl53l4cx/introduction.md)** — the 1D ToF family: single-zone
  ranging from ~2 m to ~8 m, with histograms on the L1/L3/L4.
- **[BNO086](bno086/introduction.md)** — 9-axis IMU: orientation and motion.
- **[BNO055](bno055/introduction.md)** — 9-axis IMU with on-chip fusion:
  orientation from one register block.

The VL53L5CX / L7CX / L7CH, the 1D family and the BNO055 have the decode and
codec layer in this SDK; initialising and streaming them is done with the
Python or TypeScript SDK.

## Install

Published on NuGet as `Depz.Sensor` (0.3.0):

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
using Depz.Sensor.Vl53l7;      // VL53L5CX / L7CX / L7CH board codecs + frame decoder
using Depz.Sensor.Vl53lx;      // VL53L0X / L1CX / L1CB / L3CX / L4CX codecs + block decode
using Depz.Sensor.Bno086;      // SHTP + SH-2
using Depz.Sensor.Bno055;      // BNO055 register-bridge + register codecs
using Depz.Sensor.Usb;         // USB identity table + discovery ordering
using Depz.Sensor.Dataset;     // .depzdata reader
```

## Where to next

- **Your sensor's pages** — [SR04](sr04/introduction.md) ·
  [VL53L4CD](vl53l4cd/introduction.md) ·
  [VL53L8CX](vl53l8cx/introduction.md) · [VL53L8CH](vl53l8ch/introduction.md) ·
  [VL53L5CX](vl53l5cx/introduction.md) · [VL53L7CX](vl53l7cx/introduction.md) ·
  [VL53L7CH](vl53l7ch/introduction.md) · [VL53L0X](vl53l0x/introduction.md) ·
  [VL53L1CX](vl53l1cx/introduction.md) · [VL53L1CB](vl53l1cb/introduction.md) ·
  [VL53L3CX](vl53l3cx/introduction.md) · [VL53L4CX](vl53l4cx/introduction.md) ·
  [BNO086](bno086/introduction.md) · [BNO055](bno055/introduction.md):
  introduction, hands-on guide, and the
  sensor's own API reference.
- **[Guide](guide.md)** — the SDK-wide walkthrough: installation, getting
  started, discovery, the mental model, transport & CRC, dataset replay.
- **[API Reference](api.md)** — the whole public surface, generated from the
  XML-doc summaries.
- **Docs for LLMs** — the sensor SDK documentation as raw Markdown:
  [/llms-full.txt](/llms-full.txt).
- **Source & license** — the SDKs are open source (MIT):
  [github.com/depz-ai/depz-sensor-sdk](https://github.com/depz-ai/depz-sensor-sdk).
