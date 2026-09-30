# VL53L0X — user guide

Hands-on guide to the 1D-family codecs, product table and block decode
(`Depz.Sensor.Vl53lx`) for the VL53L0X. For what the sensor is and its
concepts, read the [introduction](introduction.md); for exact signatures see
the [API reference](api.md).

This SDK does not drive the sensor: the ST driver (initialisation,
reference SPADs, profiles, calibration, the PAL status) runs in the Python or
TypeScript SDK. What follows is everything you need to build the bridge
commands and decode what the board streams.

## Contents

- [Which board is it](#which-board-is-it)
- [The product table](#the-product-table)
- [Stream commands](#stream-commands)
- [Decode the result block](#decode-the-result-block)
- [Reset and bridge diagnostics](#reset-and-bridge-diagnostics)
- [Gotchas](#gotchas)

## Which board is it

Every 1D board runs the same firmware (`APP_VL53L0_4_*`, sensor type
`Vl53lx`), so the firmware name cannot tell them apart.
[`Vl53lxProducts.ResolveClass`](api.md#vl53lxproducts) uses the production
USB PID model, else the first `VL53L<digit>…` in the device name:

```csharp
using System.Diagnostics;
using Depz.Sensor.Usb;
using Depz.Sensor.Vl53lx;

Vl53lxClass cls = Vl53lxProducts.ResolveClass(UsbIds.UsbModelHint(vid, pid), deviceName);
Debug.Assert(Vl53lxProducts.ResolveClass("vl53l0x", "") == Vl53lxClass.Vl53l0x);
Debug.Assert(Vl53lxProducts.ModelIdOk("VL53L0X", 0x00EE));   // cross-check the sensor's answer
Console.WriteLine(cls);
```

## The product table

```csharp
using System.Diagnostics;
using Depz.Sensor.Vl53lx;

Vl53lxProduct p = Vl53lxProducts.Find("VL53L0X")!;
Debug.Assert(p.UsbPid == 0xED41 && p.ModelId == 0x00EE && p.ReachMm == 2000);
Debug.Assert(p.DriverKinds.SequenceEqual(new[] { Vl53lxDriverKind.Uld }));
Debug.Assert(p.AddrWidth == 1);                                   // 8-bit register space
Debug.Assert(p.ClearSteps.SequenceEqual(new[] { new Vl53lxClearStep(0x0B, 0x01), new Vl53lxClearStep(0x0B, 0x00) }));
Debug.Assert(p.MaxKhz == 400);                                    // no FM+ on this part
```

The streamed block is the 12-byte result block at `Vl53lxDecode.L0xBlockAddr`
(`0x14`, `L0xBlockLen` = 12).

## Stream commands

Set the register-address width to 1 before the first register access of a
session (the model-id read included) — it is sticky, and 2 after a reset. Then
arm the stream with the block and the two interrupt-release writes:

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53lx;

Vl53lxProduct p = Vl53lxProducts.Find("VL53L0X")!;
byte[] width = Framing.BuildPacket((int)Vl53lxCmd.SetAddrWidth, Vl53lxWire.PackSetAddrWidth(1));
byte[] modelId = Framing.BuildPacket((int)Vl53lxCmd.ReadReg, Vl53lxWire.PackReadReg(0xC0, 1));   // answers 0xEE
byte[] start = Framing.BuildPacket((int)Vl53lxCmd.StartStream,
    Vl53lxWire.PackStartStream(Vl53lxDecode.L0xBlockAddr, (ushort)Vl53lxDecode.L0xBlockLen, p.ClearSteps));
byte[] stop = Framing.BuildPacket((int)Vl53lxCmd.StopStream);
```

At width 1 only the low byte of `addr` goes on the bus, and `addr + len` must
stay within `0x100`.

## Decode the result block

Each `RPT_VL53_STREAM` report carries the 12-byte block at `0x14`.
`Vl53lxDecode.DecodeL0xRaw` extracts its raw fields — what
`VL53L0X_GetRangingMeasurementData` reads before the PAL status step:

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l4;
using Depz.Sensor.Vl53lx;

var parser = new PacketParser();
foreach (ParserEvent ev in parser.Feed(rxBytes))
{
    if (ev is not Packet pkt || pkt.Cmd != (int)Vl53lxRpt.Stream)
        continue;
    Vl53l4StreamData s = Vl53l4StreamData.Unpack(pkt.Payload);
    if (s.Addr != Vl53lxDecode.L0xBlockAddr)
        continue;
    Vl53lxL0xRaw r = Vl53lxDecode.DecodeL0xRaw(s.Data);            // ArgumentException below 12 bytes
    double signalMcps = r.SignalRateMcps1616 / 65536.0;            // FixPoint16.16
    double ambientMcps = r.AmbientRateMcps1616 / 65536.0;
    double spads = r.EffectiveSpadCount88 / 256.0;                 // 8.8
    Console.WriteLine($"{s.TimestampUs} µs  {r.DistanceRaw} mm  device status 0x{r.DeviceRangeStatus:X2}  "
                      + $"signal {signalMcps:F3} Mcps  ambient {ambientMcps:F3} Mcps  {spads:F1} SPADs");
}
```

| field | meaning |
|---|---|
| `DistanceRaw` | distance, mm (quarter-mm if the driver enabled RangeFractional; off by default) |
| `DeviceRangeStatus` | raw status byte 0 — **not** the PAL range status |
| `SignalRateMcps1616` / `AmbientRateMcps1616` | rates, FixPoint16.16 Mcps (the wire's 9.7 shifted left by 9) |
| `EffectiveSpadCount88` | effective SPAD count, 8.8 |

The PAL range status, sigma and maximum distance the full drivers report need
device data cached at init (reference SPADs, calibration), so they are not in
this SDK. The family's plottable rule (statuses 0, 6, 11) applies to that
driver status, not to `DeviceRangeStatus`.

## Reset and bridge diagnostics

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l4;
using Depz.Sensor.Vl53lx;

byte[] reset = Framing.BuildPacket((int)Vl53lxCmd.Xshut, Vl53lxWire.PackXshut(Vl53l4Xshut.Reset));
byte[] getInfo = Framing.BuildPacket((int)Vl53lxCmd.GetInfo);

// RPT_VL53_INFO (0x92), 23 bytes — bridge state only, safe while streaming.
if (infoPkt.Cmd == (int)Vl53lxRpt.Info)
{
    Vl53lxInfo info = Vl53lxInfo.Unpack(infoPkt.Payload);
    Console.WriteLine($"address width {info.AddrWidth} (1 on the L0X), {info.I2cKhz} kHz, "
                      + $"{info.I2cErrors} I2C errors, {info.SlotsSkipped} slots skipped");
}
```

`Vl53l4Xshut.Reset` is 1 ms low plus a fixed 5 ms wait on this firmware —
v2.00 has **no boot handshake**: poll the model id (`0xC0 == 0xEE`) yourself
before configuring. The address width is 2 after a reset: set it to 1 again.

## Gotchas

- **Address width 1, every session** — after a reset the bridge is back at 2;
  a 2-byte access on the L0X reads the wrong registers.
- **Two interrupt-release writes** (`0x0B ← 1`, `0x0B ← 0`) — pass the table's
  `ClearSteps`, not the die parts' single `0x0086 ← 1`.
- **Raw fields only** — `DeviceRangeStatus` is not the PAL status.
- **400 kHz ceiling** — the L0X has no FM+ pad.
- **A NACK or two right after reset is normal** — the part is still booting;
  retry, and don't count those against a stream.
