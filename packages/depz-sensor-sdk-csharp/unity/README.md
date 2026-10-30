# DEPZ Sensor SDK for Unity (`ai.depz.sensor`)

The DEPZ sensor line in Unity on Windows. Two parts, one package:

- **The shared library `Depz.Sensor`** (`netstandard2.1`) — protocol, every
  sensor class, datasets, record/replay: the same build .NET users get from
  NuGet. It opens no port by itself.
- **The Windows communication module** — this package's `Runtime/`: the one
  part that touches a serial port, replaceable as a whole (anything that
  implements `ILink` can stand in).

| Type | What it is |
|------|------------|
| `WindowsSerialLink` | `ILink` over `System.IO.Ports.SerialPort`: 115200 8N1, DTR and RTS raised on open |
| `WindowsRegistry` | The registry reader for the shared library's `WindowsSerialPorts` / `IRegistry` (`Depz.Sensor.Devices`): COM ports with USB VID / PID / serial (`SERIALCOMM` + `Enum\USB`) |
| `DepzDevicesUnity` | `OpenDevice`, `OpenSr04` … `OpenTmf8829`, `OpenPort`, `ListDepzDevices`: the `DepzDevices` calls, for Unity |
| `DepzSensorBehaviour` | Sample `MonoBehaviour`: open in `Start()`, poll in `Update()`, close in `OnDestroy()` |

Everything else — requests, streams, events, datasets — is the shared library's,
documented in the [C# SDK guide](../docs/guide.md) and
[API reference](../docs/api.md) (section *Unity (Windows communication module)*).

## Requirements

- **Unity 6 LTS** (6000.0 or newer). The sources are C# 9 / .NET Standard 2.1,
  so 2021.2+ compiles them too, but 6000.0 is the checked target.
- **Project Settings › Player › Api Compatibility Level = .NET Framework.**
  `System.IO.Ports` and `Microsoft.Win32.Registry` are in that profile and not
  in *.NET Standard 2.1*.
- **Windows** editor or standalone player (limits below).

## Install

### By git URL

Window › Package Manager › **+** › *Add package from git URL…*:

```
https://github.com/depz-ai/depz-sensor-sdk.git?path=packages/depz-sensor-sdk-csharp/unity#v0.9.1
```

Then bring the shared library into the project, in either way:

- [NuGetForUnity](https://github.com/GlitchEnzo/NuGetForUnity): install
  `Depz.Sensor` 0.9.1; or
- take `lib/netstandard2.1/Depz.Sensor.dll` out of the `Depz.Sensor.0.9.1.nupkg`
  (a zip file) and drop it under `Assets/Plugins/`.

The module's assembly definition references `Depz.Sensor.dll` **by name**,
wherever it sits in the project. The git repository does not carry the DLL.

### By copying

Copy this folder into your project as `Packages/ai.depz.sensor/` (or anywhere
under `Assets/`) and put the shared library next to the sources:

```bash
cd packages/depz-sensor-sdk-csharp
dotnet build -c Release src/Depz.Sensor
cp src/Depz.Sensor/bin/Release/netstandard2.1/Depz.Sensor.dll unity/Runtime/Plugins/
```

`Runtime/Plugins/.gitignore` keeps the DLL out of this repository; a copy of the
folder carries it along.

## Use

Discovery is by USB id (contract 02 §4.3): only ports whose VID/PID is a known
DEPZ id count, ordered by USB serial; the board's own answer to the probe
decides what it is. `DepzDevicesUnity` mirrors `DepzDevices`:

```csharp
using Depz.Sensor.Devices;
using Depz.Sensor.Unity;

Sr04Device dev = DepzDevicesUnity.OpenSr04();               // smallest USB serial; or OpenSr04("SN000042")
DepzDevice any  = DepzDevicesUnity.OpenDevice();            // whatever is there, as its class
DepzDevice com5 = DepzDevicesUnity.OpenPort("COM5");        // exactly this port (also an unprogrammed unit)
foreach (DeviceInfo d in DepzDevicesUnity.ListDepzDevices())
    Debug.Log($"{d.Port} {d.SoftwareName} {d.SerialNumber} vid={d.UsbVid:X4} pid={d.UsbPid:X4}");
```

The device runs a reader thread; **never touch Unity objects from it**. Use the
`Subscription<T>` queues and poll them from `Update()` — `TryTake` never blocks.
`DepzSensorBehaviour` is the pattern; derive from it for a sensor:

```csharp
using Depz.Sensor.Devices;
using Depz.Sensor.Unity;
using UnityEngine;

public sealed class RangeFinder : DepzSensorBehaviour
{
    Subscription<Sr04Measurement>? _loop;

    protected override void OnOpened(DepzDevice device)
    {
        var sr04 = (Sr04Device)device;         // or DepzDevices.Expect<Sr04Device>(device, "an SR04")
        _loop = sr04.Measurements();           // subscribe before Start(): nothing is missed
        sr04.Start();
    }

    protected override void Update()
    {
        base.Update();                         // device events (disconnect, faults)
        if (_loop is null) return;
        while (_loop.TryTake(out var m))
            Debug.Log($"{m.DistanceMm} mm");
    }

    protected override void OnDestroy()
    {
        _loop?.Dispose();
        base.OnDestroy();
    }
}
```

Errors are the shared library's: a port that cannot be opened or disappears is a
`DeviceLostException`, no DEPZ candidate a `NoDepzDeviceException`, a board of
another class a `WrongTypeException`, a board error a `StatusException`.

## Limits (honestly)

- **Windows only, for now.** Port discovery reads the Windows registry; off
  Windows it finds nothing and `OpenDevice` says so. `WindowsSerialLink` itself
  is plain `System.IO.Ports`, so on a macOS or Linux editor `OpenPort("/dev/tty…")`
  may work where Mono has ports — untested.
- **macOS / Linux .NET** users (outside Unity) keep the .NET 8 build's
  `SerialPortLink` and `DepzDevices`.
- **Android, iOS, WebGL**: not covered (no serial ports in those players).
- **Unity Editor checks**: this repository's .NET suite runs on Linux. The module's
  sources (minus the `MonoBehaviour`) compile and are unit-tested here as
  `netstandard2.1` (`src/Depz.Sensor.Unity`, `tests/Depz.Sensor.Unity.Tests`:
  registry parser on a fake registry, candidate ordering, error mapping). In the
  Unity editor itself (6000.0.85f1, Windows 11, 2026-10-28, ten bench boards on
  `COM*`): the package imports and compiles with the DLL under
  `Runtime/Plugins/`; `ListDepzPorts` reads them from the registry in USB-serial
  order and every board answers the probe; an SR04 streams; and `Close()` ends a
  read blocked in Mono's `SerialPort` at once (a `DeviceLostException`, not the
  read's own timeout). In Play mode a `DepzSensorBehaviour` subclass streams
  the SR04 into a scene (distance label and bar, polled from `Update()`).
  These are EditMode and PlayMode tests in a project outside this repository,
  run with `-batchmode -runTests`.

## Replacing the communication module

Every sensor class takes an `ILink` (`Read(timeout)`, `Write`, `Close`,
`IsClosed`, `Name`). Implement it over any transport — a Bluetooth SPP stream,
a TCP bridge, a platform serial plugin — and open the device on it:
`DepzDevices.OpenOnLink(link, usbVid, usbPid)`; `DepzDevices.ProbeLink(link)` runs
the probe alone. The USB ids are optional and only break the VL53L8CX / CH and
VL53L5CX / L7CX / L7CH ties.
