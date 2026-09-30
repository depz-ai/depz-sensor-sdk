# VL53L5CX — user guide

The VL53L5CX is decoded exactly like the [VL53L7CX](../vl53l7cx/guide.md):
same board commands, same 1536-byte transfer ceilings, same frame decode with
`Vl53l7Frames.CreateDecoder()`. Everything in the VL53L7CX guide applies
unchanged; this page shows the two L5 specifics. For signatures see the
[API reference](api.md).

This SDK does not drive the board: initialising and streaming is done with
the Python or TypeScript SDK.

## Resolve the class

```csharp
using System.Diagnostics;
using Depz.Sensor.Vl53l7;

// The production PID model wins; the device name is the fallback.
Debug.Assert(Vl53l7Discovery.ResolveModel("vl53l5cx", "") == Vl53l7Model.Vl53l5cx);
Debug.Assert(Vl53l7Discovery.ResolveModel(null, "DEPZ ToF Sensor VL53L5CX USB v2.1") == Vl53l7Model.Vl53l5cx);
Debug.Assert(!Vl53l7Discovery.HasCnh(Vl53l7Model.Vl53l5cx));
```

A board stamped as an L7 but carrying an L5 (or the reverse) still ranges —
the sensor firmware is shared — and the module type the sensor reports after
initialisation (0 = MZ for the L5) is the only silicon-side tell.

## Decode a frame

```csharp
using Depz.Sensor.Vl53l7;
using Depz.Sensor.Vl53l8;

Vl53l8Frame frame = Vl53l7Frames.CreateDecoder().ParseFrame(timestampUs, frameBytes);
if (frame.Resolution == 16)
{
    // 4×4 frames carry 64-entry per-target blocks on L5/L7; the L5/L7 decoder trims them to 16.
    Debug.Assert(frame.DistanceMm.Length == 16);
}
```

## Gotchas

- **The class is chosen from the board's USB id / device name**, never from
  the silicon.
- **No deep sleep, no threshold auto-stop** on this firmware (ULD 2.0.1).
- **Peel the protective film off the lens** before measuring — with the film
  on, weak-signal zones read 0.
- All the [VL53L7CX gotchas](../vl53l7cx/guide.md#gotchas) apply.
