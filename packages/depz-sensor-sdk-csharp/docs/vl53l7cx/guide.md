# VL53L7CX — user guide

Hands-on guide to the L5/L7 codecs and decode (`Depz.Sensor.Vl53l7`). It
applies unchanged to the [VL53L5CX](../vl53l5cx/guide.md) and, plus CNH, to
the [VL53L7CH](../vl53l7ch/guide.md). The results frame itself — fields,
scaling, the grid, the advanced DCI codecs — is the VL53L8 one: see the
[VL53L8CX guide](../vl53l8cx/guide.md). For concepts read the
[introduction](introduction.md); for signatures the [API reference](api.md).

This SDK does not drive the board: initialising the sensor (firmware
download, default configuration) and starting a ranging session is done with
the Python or TypeScript SDK. What follows is everything you need to build the
board's commands and decode what it sends.

## Contents

- [Which board is it](#which-board-is-it)
- [Board commands](#board-commands)
- [The bridge info report](#the-bridge-info-report)
- [Register reads: split at 1536 bytes](#register-reads-split-at-1536-bytes)
- [Reassemble and decode a frame](#reassemble-and-decode-a-frame)
- [What differs from the VL53L8 decode](#what-differs-from-the-vl53l8-decode)
- [Gotchas](#gotchas)

## Which board is it

All three L5/L7 boards run the same firmware (`APP_VL53L7_*`, sensor type
`Vl53l7`), so the firmware name cannot tell them apart.
[`Vl53l7Discovery.ResolveModel`](api.md#vl53l7discovery) applies the contract
order: the production USB PID model, else the first `VL53L([57])(CX|CH)` in
the device name, else VL53L7CX:

```csharp
using System.Diagnostics;
using Depz.Sensor.Usb;
using Depz.Sensor.Vl53l7;

// vid/pid from the USB enumeration, deviceName from GET_DEVICE_NAME.
Vl53l7Model model = Vl53l7Discovery.ResolveModel(UsbIds.UsbModelHint(vid, pid), deviceName);
Debug.Assert(Vl53l7Discovery.ResolveModel(null, "DEPZ ToF Sensor VL53L7CH USB v2.1") == Vl53l7Model.Vl53l7ch);
Console.WriteLine($"{model} (CNH: {Vl53l7Discovery.HasCnh(model)})");
```

## Board commands

The L5/L7 bridge adds three commands to the VL53L8 register bridge
([`Vl53l7Cmd`](api.md#vl53l7cmd)). Frame each payload with
`Framing.BuildPacket`:

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l7;

// LPn low 1 ms, high, I2C_RST pulse — the sensor firmware is lost afterwards.
byte[] softCycle = Framing.BuildPacket((int)Vl53l7Cmd.PinCtrl, Vl53l7Wire.PackPinCtrl(Vl53l7PinAction.SoftCycle));
// Re-time the sensor bus; the device snaps to the nearest Vl53l7Wire.I2cKhzSteps entry.
byte[] bus400k = Framing.BuildPacket((int)Vl53l7Cmd.SetI2cSpeed, Vl53l7Wire.PackSetI2cSpeed(400));
// Ask for the bridge counters (answered with RPT_VL53_INFO).
byte[] getInfo = Framing.BuildPacket((int)Vl53l7Cmd.GetInfo);
```

| `Vl53l7PinAction` | effect |
|---|---|
| `LpnOff` | stop streaming, LPn low: sensor I2C interface off (state lost) |
| `LpnOn` | LPn high: interface on (the power-up default) |
| `I2cRst` | pulse the sensor's I2C_RST |
| `SoftCycle` | stop streaming, LPn low 1 ms, high, I2C_RST pulse, clear the I2C counters (state lost) |

`VL53_SET_I2C_SPEED` is refused with `ERR_BUSY` while a transfer is in
flight — stop the stream first, and read the effective speed back from the
info report. The register commands (`Vl53l8Cmd.ReadReg` 0x32, `WriteReg`
0x33, `StartStream` 0x35, `StopStream` 0x36) are the VL53L8 ones; their
payload encoders with the L5/L7 limits are on `Vl53l7Wire`.

## The bridge info report

`RPT_VL53_INFO` (`Vl53l7Rpt.Vl53Info`, `0x92`) is bridge state only — the
sensor is never probed — and carries **no** echoed command byte:

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l7;

var parser = new PacketParser();
foreach (ParserEvent ev in parser.Feed(rxBytes))
{
    if (ev is Packet pkt && pkt.Cmd == (int)Vl53l7Rpt.Vl53Info)
    {
        Vl53l7Info info = Vl53l7Info.Unpack(pkt.Payload);   // ArgumentException below 20 bytes
        Console.WriteLine($"{info.I2cKhz} kHz, {info.I2cErrors} I2C errors (last code {info.LastI2cError}), "
                          + $"{info.FramesDropped} frames dropped, streaming {info.Streaming}");
    }
}
```

`LastI2cError` is one of the [`Vl53l7I2cError`](api.md#vl53l7i2cerror)
codes (`Ok`, `Nack`, `Timeout`, `BusError`). Every `VL53_GET_INFO` takes the
bus from the stream and can drop the frame in flight (counted in
`FramesDropped`): read it before and after a run, not in a loop during one.

## Register reads: split at 1536 bytes

The L5/L7 bridge accepts at most `Vl53l7Wire.ReadMaxLen` = 1536 bytes per
`VL53_READ_REG` (the VL53L8 host splits at 2048 — reusing that here fails
every large read with `ERR_INVALID_PARAM`) and `Vl53l7Wire.WriteMaxLen` =
2048 per `VL53_WRITE_REG`. `Vl53l7Wire.PackReadReg` refuses a longer read:

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l7;
using Depz.Sensor.Vl53l8;

// Read `total` bytes from register `addr` in L5/L7-sized pieces.
var requests = new List<byte[]>();
for (int off = 0; off < total; off += Vl53l7Wire.ReadMaxLen)
{
    int n = Math.Min(total - off, Vl53l7Wire.ReadMaxLen);
    requests.Add(Framing.BuildPacket((int)Vl53l8Cmd.ReadReg, Vl53l7Wire.PackReadReg((ushort)(addr + off), n)));
}
```

Each reply is an `RPT_VL53_REG_DATA` (`Vl53l8Rpt.RegData`, `0x91`): `cmd u8`
(the echoed opcode), `timestamp_us u64`, then the register bytes.

## Reassemble and decode a frame

Frames arrive as `RPT_VL53_FRAME` chunks (up to 1536 data bytes each): a 4×4
frame is 1060 bytes and an 8×8 frame 1444 bytes, one chunk each; a CNH frame
on the VL53L7CH spans several. Reassemble with the shared `FrameReassembler`
and decode with the L5/L7 decoder from
[`Vl53l7Frames.CreateDecoder`](api.md#vl53l7frames):

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l7;
using Depz.Sensor.Vl53l8;

var parser = new PacketParser();
var reasm = new FrameReassembler();
Vl53l8FrameDecoder decoder = Vl53l7Frames.CreateDecoder();   // footer at size − 4, per-zone trim

foreach (ParserEvent ev in parser.Feed(rxBytes))
{
    if (ev is not Packet pkt || pkt.Cmd != (int)Vl53l8Rpt.Vl53Frame)
        continue;
    if (reasm.Feed(FrameChunk.Unpack(pkt.Payload)) is not (ulong ts, byte[] frameBytes))
        continue;
    // Resolution read from the frame; CorruptedFrameException on a header/footer mismatch.
    Vl53l8Frame frame = decoder.ParseFrame(ts, frameBytes);
    int side = frame.Resolution == 64 ? 8 : 4;
    int centre = side / 2 * side + side / 2;                  // one of the four centre zones
    if (frame.TargetStatus[centre] is 5 or 9)
        Console.WriteLine($"{ts} µs: centre {frame.DistanceMm[centre]} mm, {frame.SiliconTempDegc} °C");
}
// reasm.Completed / reasm.Discarded count rebuilt and dropped frames
```

`decoder.ParseFrame(ts, raw, resolution)` takes the resolution explicitly (16
or 64) when you know what the session was started with. The result is a
`Vl53l8Frame` with the same fields as on the VL53L8CX — see
[the frame fields](../vl53l8cx/guide.md#the-frame-fields-and-the-grid).

## What differs from the VL53L8 decode

- **Footer id at `size − 4`** for every L5/L7 part
  (`Vl53l7Frames.FooterIdOffset`) — both the ULD 2.0.1 blob of the L5CX /
  L7CX and the VL53LMZ 2.0.16 blob of the L7CH. The VL53L8CX decoder
  (`ForVariant(Vl53l8Variant.Cx)`, `size − 12`) fails these frames.
- **Per-zone arrays are trimmed to the frame's resolution.** On L5/L7 the
  per-target blocks (index ≥ `0x6C90`) keep their 64 entries even in 4×4: the
  sensor fills the first 16 and zero-pads the rest. The L5/L7 decoder reads
  the zone count from the ambient-rate block (one entry per zone) and cuts
  every array to it, so `frame.DistanceMm.Length == frame.Resolution` in both
  resolutions.
- **Bigger chunks** — `Vl53l7Wire.StreamChunkMax` is 1536 (VL53L8: 1528). The
  reassembler does not depend on the chunk size.
- **Ranging frequency** — `Vl53l7Frames.MinRangingFrequencyHz` = 1 (VL53L8:
  2); maximum 60 Hz at 4×4, 15 Hz at 8×8.

The advanced DCI codecs in `Depz.Sensor.Vl53l8` (motion indicator, detection
thresholds, crosstalk margin) apply as on the VL53L8, within the L5CX / L7CX
firmware's limits: no deep sleep and no threshold auto-stop.

## Gotchas

- **Decode with `Vl53l7Frames.CreateDecoder()`**, not
  `Vl53l8FrameDecoder.ForVariant(...)` — only it uses the `size − 4` footer
  and trims the 4×4 per-target blocks.
- **Split register reads at 1536 bytes**, not the VL53L8's 2048.
- **`RPT_VL53_INFO` has no echoed command byte** — don't match it to its
  request by `Payload[0]`.
- **Pin control wipes the sensor** — after `LpnOff` / `SoftCycle` the sensor
  must be initialised again (firmware download included).
- **Bridge info during ranging costs a frame** — read the counters around a
  run.
- **Peel the protective film off the lens** before measuring — with the film
  on, weak-signal zones read 0.
