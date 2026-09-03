# SR04 — user guide

Hands-on guide to the SR04 codecs (`Depz.Sensor.Protocol.Sr04`). For what the
sensor is and its concepts, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md).

## Contents

- [Decode a measurement](#decode-a-measurement)
- [Echo time to distance](#echo-time-to-distance)
- [Configuration payloads](#configuration-payloads)
- [Encoding commands](#encoding-commands)
- [Temperature-compensated distance](#temperature-compensated-distance)
- [Gotchas](#gotchas)

## Decode a measurement

An SR04 data report arrives as a `Packet` with `Cmd == Sr04Rpt.Data` (`0x91`).
Unpack its payload into an `Sr04Data` record:

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Protocol;

var parser = new PacketParser();
foreach (ParserEvent ev in parser.Feed(chunk))
{
    if (ev is Packet pkt && pkt.Cmd == (int)Sr04Rpt.Data)
    {
        Sr04Data d = Sr04Data.Unpack(pkt.Payload);
        double? mm = Sr04.DistanceMmFromEcho(d.EchoTimeUs);
        string src = d.SourceCmd == (int)Sr04Cmd.MeasureOnce ? "once" : "loop";
        Console.WriteLine(mm is null ? $"no echo ({src})" : $"{mm,7:F1} mm ({src})");
    }
}
```

`Sr04Data` carries `SourceCmd` (0x36 one-shot / 0x37 loop), `TimestampUs` (device
µs) and `EchoTimeUs` (the raw round-trip time, or `Sr04.EchoTimeout` = `0xFFFF`
for the no-echo timeout).

## Echo time to distance

`Sr04.DistanceMmFromEcho` is the single source of truth for the conversion:

```csharp
double? mm = Sr04.DistanceMmFromEcho(echoTimeUs: d.EchoTimeUs);   // 343 m/s
// distance_mm = echoUs · c / 2000, c in m/s. null for the 0xFFFF sentinel.
```

The echo time is authoritative; distance is derived. Always handle the `null`
(no-echo) case before using the value.

## Configuration payloads

The device stores two settings; the SDK packs the request payloads and unpacks
the read-back reports.

```csharp
byte[] setPeriod = Sr04.PackSamplePeriod(20_000);   // u32 LE µs → 50 Hz ceiling
uint   period    = Sr04.UnpackSamplePeriod(payload); // from a SamplePeriod report

byte[] setDecay  = Sr04.PackEchoDecay(5_000);        // u16 LE µs
ushort decay     = Sr04.UnpackEchoDecay(payload);    // from an EchoDecay report
```

Useful constants: `Sr04.SamplePeriodDefaultUs` (50000), `Sr04.EchoDecayDefaultUs`
(5000), and the device clamp range `Sr04.EchoDecayMinUs`..`Sr04.EchoDecayMaxUs`
(4000..65000). The echo-decay field is a `u16`, so values above 65535 cannot be
sent; the device also clamps silently, so read back the effective value.

## Encoding commands

To drive the device, frame a command opcode (`Sr04Cmd`) with `Framing.BuildPacket`:

```csharp
using Depz.Sensor.Transport;

byte[] start = Framing.BuildPacket((int)Sr04Cmd.StartMeasurementLoop);
byte[] stop  = Framing.BuildPacket((int)Sr04Cmd.StopMeasurementLoop);
byte[] once  = Framing.BuildPacket((int)Sr04Cmd.MeasureOnce);
byte[] cfg   = Framing.BuildPacket((int)Sr04Cmd.SetSamplePeriod, Sr04.PackSamplePeriod(20_000));
```

`GetSamplePeriod` / `GetEchoDecay` request the current stored values, which come
back as `Sr04Rpt.SamplePeriod` / `Sr04Rpt.EchoDecay` reports.

## Temperature-compensated distance

The default distance assumes 343 m/s. For better accuracy across temperature,
pass the air temperature (°C):

```csharp
double? at30 = Sr04.DistanceMmFromEcho(d.EchoTimeUs, airTempC: 30.0);
// c = 331.3 + 0.606·T ≈ 349.5 m/s at 30 °C
```

Both forms return `null` for a no-echo timeout.

## Gotchas

- **Always handle `null`.** A no-echo timeout is `EchoTimeUs == Sr04.EchoTimeout`
  (`0xFFFF`); `DistanceMmFromEcho` returns `null`.
- **Configured period is a ceiling, not the realised rate** — the echo window
  throttles it on the device; a read-back returns the stored value.
- **Echo decay is clamped to 4000..65000 µs on the device** and the field is a
  `u16`; read back the effective value with `UnpackEchoDecay`.
- **`SourceCmd` distinguishes one-shot from loop** — an unsolicited SYNC_IN edge
  arrives tagged as a one-shot (`0x36`).
