# BNO055 — user guide

Hands-on guide to the BNO055 wire codecs ([`Bno055Wire`](api.md#bno055wire))
and register codecs ([`Bno055Regs`](api.md#bno055regs)) in
`Depz.Sensor.Bno055`. For what the sensor is and its concepts, read the
[introduction](introduction.md); for exact signatures see the
[API reference](api.md).

This SDK does not drive the sensor. The session logic — going through CONFIG
for every mode change, waiting for the boot and the fusion start, page
discipline — runs in the Python or TypeScript SDK. What follows is everything
you need to build the commands and decode what the board sends; the rules the
sensor imposes are listed where they matter.

## Contents

- [Commands](#commands)
- [Hello-world: decode the stream](#hello-world-decode-the-stream)
- [Units and scaling](#units-and-scaling)
- [Operating modes](#operating-modes)
- [Calibration status and profile](#calibration-status-and-profile)
- [Axis remap and placements](#axis-remap-and-placements)
- [Page 1: sensor configs](#page-1-sensor-configs)
- [Bridge info](#bridge-info)
- [Gotchas](#gotchas)

## Commands

The bridge has six commands ([`Bno055Cmd`](api.md#bno055cmd), `0x32`–`0x37`).
Register addresses and lengths are one byte; a transfer is 1..128 bytes and
`addr + len` stays within `0x100`:

```csharp
using Depz.Sensor.Bno055;
using Depz.Sensor.Transport;

// A session as register writes: CONFIG → units → NDOF (wait ~70 ms for the fusion).
byte[] toConfig = Framing.BuildPacket((int)Bno055Cmd.WriteReg,
    Bno055Wire.PackWriteReg(Bno055Regs.OprMode, new[] { (byte)Bno055OprMode.Config }));
byte[] units = Framing.BuildPacket((int)Bno055Cmd.WriteReg,
    Bno055Wire.PackWriteReg(Bno055Regs.UnitSel, new[] { Bno055Units.Default.Pack() }));
byte[] toNdof = Framing.BuildPacket((int)Bno055Cmd.WriteReg,
    Bno055Wire.PackWriteReg(Bno055Regs.OprMode, new[] { (byte)Bno055OprMode.Ndof }));

// Stream the full 46-byte block (0x08..0x35) every 10 ms.
byte[] start = Framing.BuildPacket((int)Bno055Cmd.StartStream,
    Bno055Wire.PackStartStream(Bno055Trigger.Timer, Bno055Regs.FullBlockAddr, Bno055Regs.FullBlockLen, 10));
byte[] stop = Framing.BuildPacket((int)Bno055Cmd.StopStream);
byte[] readCalibStat = Framing.BuildPacket((int)Bno055Cmd.ReadReg, Bno055Wire.PackReadReg(Bno055Regs.CalibStat, 1));
byte[] reset = Framing.BuildPacket((int)Bno055Cmd.Reset);     // answered after the chip-ID handshake (~0.5 s)
byte[] getInfo = Framing.BuildPacket((int)Bno055Cmd.GetInfo);
```

`BNO_START_STREAM` replaces any running stream. `Bno055Trigger.Timer` reads
every `periodMs` (1..60000) and is the only data trigger on sensor firmware
03.11; `Bno055Trigger.Int` reads on each INT edge and is for the motion
interrupts (`periodMs` is then a missed-edge watchdog, 0 disables it). The
46-byte block costs ≈3.2 ms of bus in NDOF, the 8-byte quaternion block
(`QuatBlockAddr` / `QuatBlockLen`) ≈1.2 ms.

## Hello-world: decode the stream

Every `RPT_BNO_REG_STREAM` report echoes its window (`Addr`, `Length`), so
`Bno055Regs.DecodeBlock` needs nothing else. A channel is null when the window
does not cover all of its bytes:

```csharp
using Depz.Sensor.Bno055;
using Depz.Sensor.Transport;

Bno055Units units = Bno055Units.Default;       // the UNIT_SEL written before START_STREAM
var parser = new PacketParser();
foreach (ParserEvent ev in parser.Feed(rxBytes))
{
    if (ev is not Packet pkt || pkt.Cmd != (int)Bno055Rpt.Stream)
        continue;
    Bno055StreamData s = Bno055StreamData.Unpack(pkt.Payload);   // TimestampUs, Addr, Length, Data
    Bno055RawBlock raw = Bno055Regs.DecodeBlock(s.Addr, s.Data);
    if (raw.Euler is Bno055Euler e)
        Console.WriteLine($"{s.TimestampUs} µs heading {e.Heading / units.EulerLsb,6:F1} "
                          + $"roll {e.Roll / units.EulerLsb,6:F1} pitch {e.Pitch / units.EulerLsb,6:F1}");
    if (raw.Quaternion is Bno055Quat q)
        Console.WriteLine($"q = ({q.W / Bno055Regs.QuatLsb:+0.0000;-0.0000}, {q.X / Bno055Regs.QuatLsb:+0.0000;-0.0000}, "
                          + $"{q.Y / Bno055Regs.QuatLsb:+0.0000;-0.0000}, {q.Z / Bno055Regs.QuatLsb:+0.0000;-0.0000})");
    if (raw.CalibStat is byte c)
    {
        Bno055CalibStatus st = Bno055CalibStatus.Unpack(c);
        Console.WriteLine($"calib sys {st.System} gyro {st.Gyro} accel {st.Accel} mag {st.Mag}");
    }
}
```

The timestamp is the MCU time of the timer tick, not the I2C completion; a
sample the USB link could not take is dropped, so gaps show as jumps in
`TimestampUs`. A quaternion of all zeros means the fusion is not running:
CONFIG mode, a non-fusion mode, or the first ~70 ms after a switch out of
CONFIG.

## Units and scaling

`DecodeBlock` returns register integers; `value = raw / LSB`:

| channel | field | LSB | unit |
|---|---|---|---|
| acceleration | `Accel` | `units.AccelLsb`: 100 / 1 | m/s² / mg |
| linear acceleration | `LinearAccel` | `Bno055Regs.FusionAccelLsb` = 100 | **always m/s²** |
| gravity | `Gravity` | `Bno055Regs.FusionAccelLsb` = 100 | **always m/s²** |
| angular rate | `Gyro` | `units.GyroLsb`: 16 / 900 | deg/s / rad/s |
| Euler angles | `Euler` | `units.EulerLsb`: 16 / 900 | degrees / radians |
| quaternion | `Quaternion` | `Bno055Regs.QuatLsb` = 16384 | — |
| magnetic field | `Mag` | `Bno055Regs.MagLsb` = 16 | µT |
| temperature | `Temperature` | `units.TempLsb`: 1 / 0.5 | °C / °F (1 LSB = 2 °F) |

```csharp
using System.Diagnostics;
using Depz.Sensor.Bno055;

var units = new Bno055Units(AccelMg: true);
Debug.Assert(units.Pack() == 0x01);
Bno055RawBlock raw = Bno055Regs.DecodeBlock(s.Addr, s.Data);
if (raw.Accel is Bno055Vec3 a && raw.Gravity is Bno055Vec3 g)
{
    double accelMg = a.Z / units.AccelLsb;                    // mg: UNIT_SEL bit 0 is set
    double gravityMs2 = g.Z / Bno055Regs.FusionAccelLsb;      // still m/s² — bit 0 is ignored here
    Console.WriteLine($"{accelMg:F0} mg, gravity {gravityMs2:F2} m/s²");
}
if (raw.Mag is Bno055Vec3 m)
    Console.WriteLine($"Bx = {m.X / Bno055Regs.MagLsb:F1} µT");
```

**Linear acceleration and gravity ignore the ACC unit bit**: they stay in m/s²
at 100 LSB even with `AccelMg` set (measured on sensor firmware 03.11; the
datasheet tables promise mg). Only `Accel` switches.

UNIT_SEL (`0x3B`) bits as the silicon implements them — the datasheet's
§4.3.60 bit table is off by one:

| bit | constant | set means |
|---|---|---|
| 0 | `Bno055Units.AccMg` | acceleration in mg |
| 1 | `Bno055Units.GyrRps` | angular rate in rad/s |
| 2 | `Bno055Units.EulRad` | Euler angles in radians |
| 4 | `Bno055Units.TempFahrenheit` | temperature in °F |
| 7 | `Bno055Units.OriAndroid` | Android orientation (flips the pitch sign) |

`Bno055Units.Default` is UNIT_SEL `0x00` — m/s², deg/s, degrees, °C, Windows
orientation — the SDK default. The sensor's own power-on value is `0x80`
(Android), so write UNIT_SEL explicitly in CONFIG, and scale a stream by the
units written **before** `BNO_START_STREAM`: the first sample can arrive
before the command's own reply.

## Operating modes

```csharp
using System.Diagnostics;
using Depz.Sensor.Bno055;

var mode = (Bno055OprMode)(0x1C & 0x0F);   // OPR_MODE reads back with bit 4 set after reset: mask
Debug.Assert(mode == Bno055OprMode.Ndof);
Debug.Assert(Bno055Regs.IsFusion(mode));
Debug.Assert(!Bno055Regs.IsFusion(Bno055OprMode.Amg));      // raw sensors only
```

| mode | sensors | output |
|---|---|---|
| `Config` | — | configuration only, all outputs zero |
| `AccOnly` … `Amg` | as named | raw data only; fusion registers read zero |
| `Imu` | accel + gyro | relative orientation, 100 Hz |
| `Compass` | accel + mag | absolute heading, 20 Hz |
| `M4g` | accel + mag | relative orientation from the magnetometer, 50 Hz |
| `NdofFmcOff` | all three | absolute orientation, 100 Hz, slow mag calibration |
| `Ndof` | all three | absolute orientation, 100 Hz |

The sensor only switches **between CONFIG and an operating mode**: a write of
OPR_MODE from one operating mode straight to another (NDOF → AMG) is silently
ignored. Go through CONFIG (any → CONFIG takes 19 ms, CONFIG → any 7 ms).
After `BNO_RESET` the reply comes at the chip-ID handshake, but the sensor is
still booting: poll SYS_STATUS (`Bno055Regs.SysStatus`, `0x39`) until it
leaves 2, 3 and 4 before configuring, or the mode you write is lost.

## Calibration status and profile

```csharp
using System.Diagnostics;
using Depz.Sensor.Bno055;

Debug.Assert(Bno055CalibStatus.Unpack(0xFF).FullyCalibrated);    // sys/gyro/accel/mag all 3

// Read the 22-byte profile (CONFIG mode only), keep it, write it back later in one transfer.
byte[] read = Bno055Wire.PackReadReg(Bno055Regs.CalibProfile, (byte)Bno055Regs.CalibProfileLen);
Bno055RegData reply = Bno055RegData.Unpack(regDataPayload);       // the RPT_BNO_REG_DATA answer
var profile = Bno055CalibrationProfile.Unpack(reply.Data);        // exactly 22 bytes
if (profile.MagRadius is >= 144 and <= 1280)
{
    byte[] restore = Bno055Wire.PackWriteReg(Bno055Regs.CalibProfile, profile.Pack());
}
```

What each sensor needs to calibrate (datasheet §3.11): **gyro** — hold still
for a few seconds; **accel** — six still poses, each axis up and down;
**magnetometer** — slow figure-eights in the air. The fusion calibrates
continuously in the background and cannot be told not to.

A restored profile is a starting point, not a lock: once the fusion runs it
keeps refining the offsets. **Don't store the profile of an uncalibrated
sensor**: its `MagRadius` is 0, outside the legal 144..1280, and restoring it
makes the sensor report a fusion configuration error (SYS_ERR 9). Store a
profile only once CALIB_STAT reads 3/3/3/3; verify a write by reading it back
without leaving CONFIG. The soft-iron matrix at `0x43` has its own codec:
`Bno055Regs.PackSicMatrix` / `UnpackSicMatrix` (9 × i16, 1.0 = 16384,
identity `Bno055Regs.SicIdentity`).

## Axis remap and placements

```csharp
using System.Diagnostics;
using Depz.Sensor.Bno055;

// One of the datasheet mountings P0..P7 (P1 is the default: chip axes as printed).
(byte config, byte sign) = Bno055AxisRemap.Placement("P2").Pack();
Debug.Assert((config, sign) == (0x24, 0x06));
Debug.Assert(Bno055AxisRemap.Placements[2] == ("P2", (byte)0x24, (byte)0x06));

// Or any permutation: output X = −chip Y, output Y = chip X.
var custom = new Bno055AxisRemap(X: Bno055Axis.Y, Y: Bno055Axis.X, XNegative: true);
var (cfg, sgn) = custom.Pack();
byte[] write = Bno055Wire.PackWriteReg(Bno055Regs.AxisMapConfig, new[] { cfg, sgn });   // 0x41 and 0x42 in one write, CONFIG only

// A mapping that uses one axis twice is refused: the sensor would silently keep the old one.
try
{
    new Bno055AxisRemap(X: Bno055Axis.X, Y: Bno055Axis.X).Pack();
    throw new InvalidOperationException("not refused");
}
catch (ArgumentException)
{
}
```

`AXIS_MAP_CONFIG` is `z<5:4> y<3:2> x<1:0>` (the source axis of each output
axis), `AXIS_MAP_SIGN` is `x 2, y 1, z 0` (1 = negative).

## Page 1: sensor configs

The accel / gyro / mag configs, the interrupt setup and the unique id live on
register page 1. The host owns the page select: write `PAGE_ID ← 1`, the
access, then `PAGE_ID ← 0` — and never while a stream runs (the bridge would
stream page-1 registers):

```csharp
using System.Diagnostics;
using Depz.Sensor.Bno055;

var accel = new Bno055AccelConfig(Range: 2, Bandwidth: 3, Power: 0);   // ±8 g, 62.5 Hz, normal
Debug.Assert(Bno055Regs.AccRangeG[accel.Range] == 8);
byte[][] writes =
{
    Bno055Wire.PackWriteReg(Bno055Regs.PageId, new byte[] { 1 }),
    Bno055Wire.PackWriteReg(Bno055Regs.P1AccConfig, new[] { accel.Pack() }),
    Bno055Wire.PackWriteReg(Bno055Regs.PageId, new byte[] { 0 }),
};

var gyro = Bno055GyroConfig.Unpack(new byte[] { 0x38, 0x00 });        // power-on GYR_CONFIG_0/1
Debug.Assert(Bno055Regs.GyrRangeDps[gyro.Range] == 2000);
var mag = Bno055MagConfig.Unpack(0x8B);                               // bit 7 is not a field
Debug.Assert(Bno055Regs.MagRateHz[mag.Rate] == 10 && mag.Pack() == 0x0B);
```

These configs take effect in the **non-fusion** modes only; the fusion modes
override them. On these boards only the motion interrupts work
(any/no-motion, high-g, high-rate): the data-ready bits exist but never fire
on sensor firmware 03.11.

## Bridge info

`RPT_BNO_INFO` (38 bytes) carries the sensor identity and the bridge
counters:

```csharp
using Depz.Sensor.Bno055;

Bno055Info info = Bno055Info.Unpack(infoPayload);      // ArgumentException below 38 bytes
Console.WriteLine($"ids ok: {info.IdsOk}, sensor firmware {info.SwRevText}, block read avg {info.ReadAvgUs} µs, "
                  + $"{info.SlotsSkipped} slots skipped, {info.SensorResets} sensor resets, "
                  + $"last I2C error {info.LastI2cError} (0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR)");
```

The counters are free-running and wrap silently — watch increments. A rising
`SensorResets` means the bridge recovered a stuck bus by pulsing nRESET: the
stream keeps running, but the sensor is back in CONFIG with power-on settings
(every output zero) and must be configured again.

## Gotchas

- **Linear acceleration and gravity are always m/s²** — scale them with
  `Bno055Regs.FusionAccelLsb`, not `units.AccelLsb`.
- **UNIT_SEL is not the datasheet's §4.3.60 table** — use the `Bno055Units`
  constants / `Pack()`.
- **Scale a stream by the units written before `BNO_START_STREAM`.**
- **Zero quaternion = not fusing** — CONFIG, a non-fusion mode, or the first
  ~70 ms after leaving CONFIG. Every CONFIG round trip (reading the profile,
  writing units) restarts that gap.
- **No direct mode-to-mode switch** — go through CONFIG.
- **Settings are silently ignored outside CONFIG** — units, axis remap,
  calibration profile, power mode.
- **Don't store a profile with `MagRadius` 0** — restoring it causes a fusion
  configuration error.
- **Never switch pages under a running stream**, and always return to page 0.
- **INT_STA (`0x37`) clears on read** — keep it out of routine block reads
  (the full block stops at `0x35`).
- **Don't benchmark in CONFIG** — the sensor stretches the bus 3–5× harder
  there (a 46-byte read ≈10 ms instead of ≈3 ms).
