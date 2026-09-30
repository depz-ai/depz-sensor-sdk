# VL53L4CX — user guide

Hands-on guide to the 1D-family codecs, product table and block decode
(`Depz.Sensor.Vl53lx`) for the VL53L4CX. For what the sensor is and its
concepts, read the [introduction](introduction.md); for exact signatures see
the [API reference](api.md).

This SDK does not drive the sensor: the ST driver (initialisation, modes,
budgets, histogram target extraction) runs in the Python or
TypeScript SDK. What follows is everything you need to build the bridge
commands and decode what the board streams.

## Contents

- [Which board is it](#which-board-is-it)
- [The product table](#the-product-table)
- [Stream commands](#stream-commands)
- [Decode the histogram block](#decode-the-histogram-block)
- [Running as a VL53L4CD](#running-as-a-vl53l4cd)
- [Which statuses to plot](#which-statuses-to-plot)
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
Debug.Assert(Vl53lxProducts.ResolveClass(null, "DEPZ ToF Sensor VL53L4CX USB v2.1") == Vl53lxClass.Vl53l4cx);
Debug.Assert(Vl53lxProducts.ProductFromBoardName("ToF Sensor VL53L4CX USB v2.1") == "VL53L4CX");
Console.WriteLine(cls);
```

The model id the sensor answers (`0xEBAA`) is a cross-check only
(`Vl53lxProducts.ModelIdOk`): the VL53L4CD answers the same.

## The product table

[`Vl53lxProducts.All`](api.md#vl53lxproducts) holds one
[`Vl53lxProduct`](api.md#vl53lxproduct) per family member, with the bridge
parameters of its default driver:

```csharp
using System.Diagnostics;
using Depz.Sensor.Vl53lx;

Vl53lxProduct p = Vl53lxProducts.Find("VL53L4CX")!;        // null for an unserved name
Debug.Assert(p.UsbPid == 0xED46 && p.ModelId == 0xEBAA && p.ReachMm == 6000);
Debug.Assert(p.DriverKinds.SequenceEqual(new[] { Vl53lxDriverKind.Histogram }));   // histogram only
Debug.Assert(p.DefaultDriver == Vl53lxDriverKind.Histogram);
// Naming the neighbour borrows its driver: the L4CD's light ULD, 1.2 m reach.
Debug.Assert(Vl53lxProducts.Find("VL53L4CD")!.DriverKinds.Contains(Vl53lxDriverKind.Uld));
Debug.Assert(p.AddrWidth == 2 && p.MaxKhz == 1000);
Debug.Assert(p.ClearSteps.Single() == new Vl53lxClearStep(0x0086, 0x01));   // interrupt release
```

The `histogram` driver streams the 83-byte block at
`Vl53lxDecode.HistogramBlockAddr` (`0x0088`). Running as a VL53L4CD, the board
streams the 17-byte die block at `Vl53lxDecode.DieBlockAddr` (`0x0089`)
instead, read with `Vl53lxDieVariant.L4`; both use the same interrupt release
and bus ceiling.

## Stream commands

A stream is armed with `VL53_START_STREAM`: the block to read on every
data-ready edge and the interrupt-release writes the bridge plays after each
read (v2.00 carries them in the command). Set the register-address width
first — it is sticky, and 2 after a reset:

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53lx;

Vl53lxProduct p = Vl53lxProducts.Find("VL53L4CX")!;
byte[] width = Framing.BuildPacket((int)Vl53lxCmd.SetAddrWidth, Vl53lxWire.PackSetAddrWidth(p.AddrWidth));
byte[] start = Framing.BuildPacket((int)Vl53lxCmd.StartStream,
    Vl53lxWire.PackStartStream(Vl53lxDecode.HistogramBlockAddr, (ushort)Vl53lxDecode.HistogramBlockLen, p.ClearSteps));   // ≤ 4 steps
byte[] stop = Framing.BuildPacket((int)Vl53lxCmd.StopStream);
```

The register commands (`PackReadReg`, `PackWriteReg`, `PackXshut`,
`PackSetI2cSpeed`) are the VL53L4CD codecs, forwarded by `Vl53lxWire`. Every
init runs at 400 kHz; the driver raises the bus to `MaxKhz` afterwards.

## Decode the histogram block

The VL53L4CX's only driver is `histogram`: each `RPT_VL53_STREAM` report
carries the 83-byte block at `0x0088`. `Vl53lxDecode.DecodeHistogramRaw` extracts the status bytes and the
24 photon-count bins (bin 23's low byte, carried separately, is patched in
first):

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l4;
using Depz.Sensor.Vl53lx;

var parser = new PacketParser();
foreach (ParserEvent ev in parser.Feed(rxBytes))
{
    if (ev is not Packet pkt || pkt.Cmd != (int)Vl53lxRpt.Stream)
        continue;
    Vl53l4StreamData s = Vl53l4StreamData.Unpack(pkt.Payload);    // TimestampUs, Addr, Len, Data
    if (s.Addr != Vl53lxDecode.HistogramBlockAddr)
        continue;
    Vl53lxHistogramRaw h = Vl53lxDecode.DecodeHistogramRaw(s.Data);   // ArgumentException below 83 bytes
    int peakBin = Enumerable.Range(0, Vl53lxDecode.HistogramBins).MaxBy(i => h.Bins[i]);
    Console.WriteLine($"{s.TimestampUs} µs #{h.StreamCount} range status 0x{h.RangeStatus:X2}, {h.DssActualEffectiveSpads} effective SPADs, "
                      + $"VCSEL start {h.VcselStart}, peak bin {peakBin} = {h.Bins[peakBin]}");
}
```

Turning bins into up to four targets (preset, VCSEL period, the A/B frame
pairs, phase history) is the full driver's job and is not in this SDK. The
driver keeps state from frame to frame, so a stream fed to it must be decoded
**exactly once, in order** — replaying or skipping frames changes its output.

## Running as a VL53L4CD

For offset/crosstalk calibration or thresholds, the full drivers can run the
VL53L4CX on the VL53L4CD's light ULD (reach drops to ~1.2 m). The board then
streams the 17-byte die block, read with `Vl53lxDieVariant.L4`:

```csharp
using Depz.Sensor.Vl53lx;

if (s.Addr == Vl53lxDecode.DieBlockAddr)
{
    Vl53lxDieResult r = Vl53lxDecode.DecodeDieBlock(s.Data, Vl53lxDieVariant.L4);
    Console.WriteLine($"{r.DistanceMm} mm, status {r.RangeStatus}, sigma {r.SigmaMm} mm");
}
```

[`Vl53lxDieResult`](api.md#vl53lxdieresult) holds integers as the ULD
computes them: `DistanceMm`, `SigmaMm`, signal and ambient rates in kcps,
per-SPAD rates (rate × 256 ÷ SPADs), the active SPAD count and the rolling
`StreamCount`; `RangeStatus` is mapped through the ULD status table (0 =
valid).

## Which statuses to plot

The full drivers report one family-wide status. Statuses **0, 6 and 11** are
usable: 6 is the first frame (no predecessor for the wrap check), 11 a merged
pulse. `status == 0` alone drops those frames. The die block's `RangeStatus`
(when running as a VL53L4CD) uses the same numbering, so the same rule applies
to it:

```csharp
using Depz.Sensor.Vl53lx;

// Statuses a chart should draw (0 valid, 6 first frame, 11 merged pulse).
int[] plottable = { 0, 6, 11 };

Vl53lxDieResult r = Vl53lxDecode.DecodeDieBlock(s.Data, Vl53lxDieVariant.L4);
if (plottable.Contains(r.RangeStatus))
    Console.WriteLine($"{r.DistanceMm} mm");
```

The histogram block's `RangeStatus` byte is the raw device register, not the
driver's target status — the rule does not apply to it.

The histogram driver's `short` preset is unusable on the L4CX at any distance:
its frames alternate between the true distance flagged status 7 and a wrong one
(flat wall, 2026-09-28: −341 mm at 0.15 and 0.3 m, −156 mm at 0.6 m; at 1.0 m
the true 1008 mm comes flagged status 4 and the other frame reads 238 mm), so a
plottable filter drops them all. Same on firmware v0.23 and v0.24 and with the
firmware repo's own `vl53_tool.py`; the cause is not known yet. `medium` and
`long` are clean at all four distances.

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
    Console.WriteLine($"{info.IntEdges} edges, {info.SlotsSkipped} slots skipped, {info.FramesDropped} frames dropped, "
                      + $"{info.I2cErrors} I2C errors, {info.I2cKhz} kHz, width {info.AddrWidth}, {info.NClear} clear steps");
}
```

`Vl53l4Xshut.Reset` is 1 ms low plus a fixed 5 ms wait on this firmware —
v2.00 has **no boot handshake**: poll the boot register (`0x00E5 == 0x03`)
yourself before the first access. A power-cycled sensor holds none of its
configuration or calibration. The counters are free-running and wrap
silently: watch increments.

## Gotchas

- **Plot 0, 6 and 11**, not only 0 — and don't use the `short` preset on the
  L4CX at all (see above); `medium` and `long` are clean.
- **Decode every histogram frame exactly once, in order** if you feed a
  target-extraction driver.
- **Set the address width before the first register access** of a session,
  and never under a running stream.
- **Same model id as the VL53L4CD** (`0xEBAA`) — the board's USB PID or device
  name, not the silicon, tells them apart.
- **Running as a VL53L4CD reads the die block with `Vl53lxDieVariant.L4`**.
- **At most 4 interrupt-release steps** — `PackStartStream` throws beyond.
