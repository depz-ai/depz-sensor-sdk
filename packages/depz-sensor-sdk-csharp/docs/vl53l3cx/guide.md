# VL53L3CX — user guide

Hands-on guide to the 1D-family codecs, product table and block decode
(`Depz.Sensor.Vl53lx`) for the VL53L3CX. For what the sensor is and its
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
- [Decode the die block (ULP)](#decode-the-die-block-ulp)
- [Decode the histogram block](#decode-the-histogram-block)
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
Debug.Assert(Vl53lxProducts.ResolveClass(null, "DEPZ ToF Sensor VL53L3CX USB v2.1") == Vl53lxClass.Vl53l3cx);
Debug.Assert(Vl53lxProducts.ProductFromBoardName("ToF Sensor VL53L3CX USB v2.1") == "VL53L3CX");
Console.WriteLine(cls);
```

The model id the sensor answers (`0xEAAA`) is a cross-check only
(`Vl53lxProducts.ModelIdOk`).

## The product table

[`Vl53lxProducts.All`](api.md#vl53lxproducts) holds one
[`Vl53lxProduct`](api.md#vl53lxproduct) per family member, with the bridge
parameters of its default driver:

```csharp
using System.Diagnostics;
using Depz.Sensor.Vl53lx;

Vl53lxProduct p = Vl53lxProducts.Find("VL53L3CX")!;        // null for an unserved name
Debug.Assert(p.UsbPid == 0xED44 && p.ModelId == 0xEAAA && p.ReachMm == 3000);
Debug.Assert(p.DriverKinds.SequenceEqual(new[] { Vl53lxDriverKind.Ulp, Vl53lxDriverKind.Histogram }));
Debug.Assert(p.DefaultDriver == Vl53lxDriverKind.Ulp);
Debug.Assert(p.AddrWidth == 2 && p.MaxKhz == 1000);
Debug.Assert(p.ClearSteps.Single() == new Vl53lxClearStep(0x0086, 0x01));   // interrupt release
```

Which block the board streams follows from the driver: the `ulp` driver
streams the 17-byte die block at `Vl53lxDecode.DieBlockAddr` (`0x0089`), read
the way the VL53L4CD ULD reads it (`Vl53lxDieVariant.L4`); the `histogram`
driver the
83-byte block at `Vl53lxDecode.HistogramBlockAddr` (`0x0088`). Both use the
same interrupt release and bus ceiling.

## Stream commands

A stream is armed with `VL53_START_STREAM`: the block to read on every
data-ready edge and the interrupt-release writes the bridge plays after each
read (v2.00 carries them in the command). Set the register-address width
first — it is sticky, and 2 after a reset:

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53lx;

Vl53lxProduct p = Vl53lxProducts.Find("VL53L3CX")!;
byte[] width = Framing.BuildPacket((int)Vl53lxCmd.SetAddrWidth, Vl53lxWire.PackSetAddrWidth(p.AddrWidth));
byte[] start = Framing.BuildPacket((int)Vl53lxCmd.StartStream,
    Vl53lxWire.PackStartStream(Vl53lxDecode.DieBlockAddr, (ushort)Vl53lxDecode.DieBlockLen, p.ClearSteps));   // ≤ 4 steps
byte[] stop = Framing.BuildPacket((int)Vl53lxCmd.StopStream);
```

The register commands (`PackReadReg`, `PackWriteReg`, `PackXshut`,
`PackSetI2cSpeed`) are the VL53L4CD codecs, forwarded by `Vl53lxWire`. Every
init runs at 400 kHz; the driver raises the bus to `MaxKhz` afterwards.

## Decode the die block (ULP)

With the `ulp` driver every `RPT_VL53_STREAM` report carries the 17-byte die
result block. `Vl53lxDecode.DecodeDieBlock` reads it the way the ULP does —
the VL53L4CD ULD layout (`Vl53lxDieVariant.L4`: signal rate at `0x008E`,
per-SPAD scale 256):

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
    if (s.Addr != Vl53lxDecode.DieBlockAddr)
        continue;
    Vl53lxDieResult r = Vl53lxDecode.DecodeDieBlock(s.Data, Vl53lxDieVariant.L4);
    Console.WriteLine($"{s.TimestampUs} µs  #{r.StreamCount,3}  {r.DistanceMm,5} mm  status {r.RangeStatus}  "
                      + $"sigma {r.SigmaMm} mm  signal {r.SignalRateKcps} kcps  ambient {r.AmbientRateKcps} kcps  "
                      + $"{r.NumberOfSpad} SPADs");
}
```

[`Vl53lxDieResult`](api.md#vl53lxdieresult) holds integers as the ULD
computes them: `DistanceMm`, `SigmaMm` (raw ÷ 4), signal and ambient rates in
kcps (raw × 8), per-SPAD rates (rate × 256 ÷ SPADs), the active SPAD count and
the rolling `StreamCount`. `RangeStatus` is already mapped through the ULD
status table (0 = valid). A block shorter than 17 bytes throws
`ArgumentException`.

## Decode the histogram block

With the `histogram` driver each report carries the 83-byte block at
`0x0088`. `Vl53lxDecode.DecodeHistogramRaw` extracts the status bytes and the
24 photon-count bins (bin 23's low byte, carried separately, is patched in
first):

```csharp
using Depz.Sensor.Vl53lx;

if (s.Addr == Vl53lxDecode.HistogramBlockAddr)
{
    Vl53lxHistogramRaw h = Vl53lxDecode.DecodeHistogramRaw(s.Data);
    int peakBin = Enumerable.Range(0, Vl53lxDecode.HistogramBins).MaxBy(i => h.Bins[i]);
    Console.WriteLine($"#{h.StreamCount} range status 0x{h.RangeStatus:X2}, {h.DssActualEffectiveSpads} effective SPADs, "
                      + $"VCSEL start {h.VcselStart}, peak bin {peakBin} = {h.Bins[peakBin]}");
}
```

The histogram driver is what makes the L3CX a multi-target ranger: a glass
pane and the object behind it, or two people at different distances. Turning
bins into up to four targets (preset, VCSEL period, the A/B frame
pairs, phase history) is the full driver's job and is not in this SDK. The
driver keeps state from frame to frame, so a stream fed to it must be decoded
**exactly once, in order** — replaying or skipping frames changes its output.

## Which statuses to plot

The full drivers report one family-wide status. Statuses **0, 6 and 11** are
usable: 6 is the first frame (no predecessor for the wrap check), 11 a merged
pulse. `status == 0` alone drops those frames. The die block's `RangeStatus`
uses the same numbering, so the same rule applies to it:

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

- **Use `Vl53lxDieVariant.L4`** for the ULP (the default of
  `DecodeDieBlock`) — the `L1` variant reads the signal rate from a different
  register and scales per SPAD by 25.
- **Plot 0, 6 and 11**, not only 0.
- **Decode every histogram frame exactly once, in order** if you feed a
  target-extraction driver.
- **Set the address width before the first register access** of a session,
  and never under a running stream.
- **No calibrations on either driver** — offset and crosstalk calibration
  belong to the light drivers of the other boards.
- **At most 4 interrupt-release steps** — `PackStartStream` throws beyond.
