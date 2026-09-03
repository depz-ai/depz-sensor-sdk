# BNO086 — user guide

Hands-on guide to the BNO086 codecs (`Depz.Sensor.Bno086`). For what the sensor
is and its concepts, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md).

## Contents

- [The decode pipeline](#the-decode-pipeline)
- [Reassembling SHTP cargos](#reassembling-shtp-cargos)
- [Decoding input reports](#decoding-input-reports)
- [Report types and units](#report-types-and-units)
- [The gyro-integrated RV](#the-gyro-integrated-rv)
- [Encoding control reports](#encoding-control-reports)
- [Gotchas](#gotchas)

## The decode pipeline

BNO086 data arrives as SH-2 wrapped in SHTP wrapped in DEPZ packets. Peel the
layers in order:

```
Packet (bridge RPT_DATA)  ─►  ShtpLayer.Feed  ─►  ShtpCargo(Channel, Seq, Payload)
      ├─ channel 3/4  ─►  Sh2Reports.ParseInputCargo   ─►  List<BnoReport>
      └─ channel 5    ─►  Sh2Reports.ParseGyroRvCargo   ─►  GyroIntegratedRV?
```

## Reassembling SHTP cargos

The bridge delivers SHTP frames as the payload of unsolicited data reports. Feed
each frame to a `ShtpLayer`; it returns a complete `ShtpCargo` once all
continuation fragments have arrived (`null` while a cargo is still incomplete):

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Bno086;

var parser = new PacketParser();
var shtp   = new ShtpLayer();

foreach (ParserEvent ev in parser.Feed(chunk))
{
    if (ev is not Packet pkt) continue;
    if (shtp.Feed(pkt.Payload) is not ShtpCargo cargo) continue;   // still assembling

    long captureUs = (long)pkt.Seq;   // use the bridge capture timestamp you track
    if (cargo.Channel == (int)ShtpChannel.InputNormal ||
        cargo.Channel == (int)ShtpChannel.InputWake)
    {
        foreach (BnoReport r in Sh2Reports.ParseInputCargo(cargo.Payload, captureUs))
            Handle(r);
    }
    else if (cargo.Channel == (int)ShtpChannel.GyroRv)
    {
        if (Sh2Reports.ParseGyroRvCargo(cargo.Payload, captureUs) is { } rv)
            Handle(rv);
    }
}
// shtp.Discarded counts cargos dropped on a framing gap/overrun.
```

`ShtpLayer.Reset()` forgets all TX seq counters and partial cargos — call it on a
sensor hardware reset. On the TX side, `ShtpLayer.NextFrame(channel, payload)`
frames a control cargo and consumes that channel's sequence counter.

## Decoding input reports

`Sh2Reports.ParseInputCargo` walks a channel-3/4 cargo and returns a list of
typed `BnoReport`. It handles the 0xFB base-timebase reference and 0xFA rebase
markers, folding them into an absolute `TimestampUs` (in the MCU clock) per
report. Dispatch on the concrete record type or `report.Kind`:

```csharp
void Handle(BnoReport r)
{
    switch (r)
    {
        case RotationVector rv:
            double w = rv.RealRaw / 16384.0;    // Q14
            double? accRad = rv.AccuracyRaw is { } a ? a / 4096.0 : null;  // Q12, radians
            Console.WriteLine($"quat real={w:F3} acc={accRad}");
            break;
        case Acceleration acc:
            Console.WriteLine($"a=({acc.XRaw/256.0:F2},{acc.YRaw/256.0:F2},{acc.ZRaw/256.0:F2}) m/s²"); // Q8
            break;
        case UnknownReport u:
            Console.WriteLine($"unknown report 0x{u.SensorIdValue:X2}: {u.Fields()["data"]}");
            break;
    }
}
```

Every `InputReport` also carries `Seq` (rolling sample counter for drop
detection), `Accuracy` (0 unreliable … 3 high) and `DelayUs`. `BnoReport.Fields()`
returns the flat field→value dictionary that matches the golden vectors — handy
for logging or generic serialization. Unrecognised report IDs surface as
`UnknownReport`.

## Report types and units

Raw wire integers are authoritative; apply the fixed **Q point**
(`value = raw / 2^Q`) yourself. The common ones:

| SensorId | record | fields (units) | Q |
|---|---|---|---|
| `Accelerometer` / `LinearAcceleration` / `Gravity` | `Acceleration` | `XRaw/YRaw/ZRaw` → m/s² | Q8 |
| `Gyroscope` | `Gyroscope` | `XRaw/YRaw/ZRaw` → rad/s | Q9 |
| `Magnetometer` | `Magnetometer` | `XRaw/YRaw/ZRaw` → µT | Q4 |
| `RotationVector` / `Geomagnetic…` / `ArvrStabilizedRv` | `RotationVector` | `IRaw/JRaw/KRaw/RealRaw` + `AccuracyRaw` | Q14 quat, Q12 (rad) accuracy |
| `GameRotationVector` / `ArvrStabilizedGameRv` | `RotationVector` | `IRaw/JRaw/KRaw/RealRaw`; `AccuracyRaw` is `null` | Q14 |
| `GyroIntegratedRv` | `GyroIntegratedRV` | quaternion + `VxRaw/VyRaw/VzRaw` → rad/s | Q14 quat, Q10 velocity |
| `StepCounter` | `StepCounter` | `Steps`, `LatencyUs` | — |
| `TapDetector` | `TapDetector` | `Flags` | — |
| `StabilityClassifier` | `StabilityClassifier` | `Classification` | — |
| `PersonalActivityClassifier` | `PersonalActivityClassifier` | `MostLikelyState`, `Confidences[]` | — |

Pressure/light/humidity/proximity/temperature and other scalar reports decode to
`ScalarReport` (`ValueRaw`); unmapped IDs to `GenericEvent`.

## The gyro-integrated RV

The dense gyro-integrated rotation vector rides its own channel (5) in a compact
format, so it has its own parser (returns `null` when the cargo is too short):

```csharp
GyroIntegratedRV? rv = Sh2Reports.ParseGyroRvCargo(cargo.Payload, captureUs);
if (rv is { } g)
{
    double qw = g.RealRaw / 16384.0;      // Q14
    double wz = g.VzRaw / 1024.0;         // Q10, rad/s
}
```

## Encoding control reports

`Sh2Control` produces the header-less SH-2 cargo payloads for channel 2; frame
them with `ShtpLayer.NextFrame` (or `BuildFrame` / `FragmentCargo` for control):

```csharp
using Depz.Sensor.Bno086;

var shtp = new ShtpLayer();

// Enable rotation vector at 100 Hz (10 000 µs report interval):
byte[] setFeature = Sh2Control.SetFeature((int)SensorId.RotationVector, intervalUs: 10_000);
byte[] frame      = shtp.NextFrame((int)ShtpChannel.Control, setFeature);

byte[] getFeature = Sh2Control.GetFeature((int)SensorId.Gyroscope);
byte[] productId  = Sh2Control.ProductId();
byte[] frsRead    = Sh2Control.FrsRead(frsType: 0x7979);
byte[] command    = Sh2Control.Command(seq: 0, command: 0x84, parameters: stackalloc byte[0]); // e.g. tare
```

`Sh2Control` covers Set/Get Feature, Product ID, the generic Command request, and
FRS read/write/data. Each returns the exact cargo bytes the golden vectors pin.

## Gotchas

- **Reassemble before you decode** — feed frames to `ShtpLayer.Feed` and only
  decode a returned `ShtpCargo`; a partial cargo returns `null`.
- **Dispatch by channel** — channels 3/4 use `ParseInputCargo`; channel 5 uses
  `ParseGyroRvCargo`. They are different formats.
- **Scaling is yours** — reports keep raw integers; apply the Q point. Watch the
  units: rotation-vector accuracy is Q12 **radians**, and game/AR-VR-game vectors
  have `AccuracyRaw == null`.
- **TX cargo fits one MCU slot** — `NextFrame` rejects payloads over
  `ShtpLayer.MaxTxFrame` (64 B incl. header, ERRATA E2); use `FragmentCargo` for
  anything larger.
- **Reset clears SHTP state** — on a sensor hardware reset, call `ShtpLayer.Reset`
  so seq counters and partial cargos restart from zero.
