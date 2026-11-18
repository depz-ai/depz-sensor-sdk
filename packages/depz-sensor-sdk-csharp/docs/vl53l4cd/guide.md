# VL53L4CD — user guide

Hands-on guide to the VL53L4CD: the live device
(`Depz.Sensor.Devices.Vl53l4cdDevice`, which runs the ST ULD 2.2.3 on the host
as `Vl53l4cdDriver`) and the layer under it — the `Vl53l4Wire` wire codecs
(`Depz.Sensor.Vl53l4`). For what the sensor is and its concepts, read the
[introduction](introduction.md); for exact signatures see the
[API reference](api.md).

## Contents

- [Live device](#live-device)
- [The codec surface](#the-codec-surface)
- [Read and write registers](#read-and-write-registers)
- [Stream the result block](#stream-the-result-block)
- [The measurement](#the-measurement)
- [Range timing](#range-timing)
- [Tuning words](#tuning-words)
- [Gotchas](#gotchas)

## Live device

```csharp
using Depz.Sensor.Devices;

using var tof = (Vl53l4cdDevice)DepzDevices.OpenDevice();
tof.Init();                                   // config block at 400 kHz, VHV calibration, then 1 MHz
tof.SetRangeTiming(33);                       // 33 ms budget, continuous
Vl53l4Measurement m = tof.MeasureOnce();      // polled: start, data ready, read, stop
Console.WriteLine($"{m.DistanceMm} mm ({m.StatusText})");

using var stream = tof.Measurements.Subscribe();
tof.StartRanging();                           // one result per INT edge, pushed by the board
for (int i = 0; i < 20; i++)
    Console.WriteLine(stream.Take(TimeSpan.FromSeconds(1)).DistanceMm);
tof.StopRanging();
```

- Configuration (`SetRangeTiming`, `SetOffsetMm`, thresholds, calibration)
  needs `Init()` and throws `InvalidOperationException` while ranging: the
  stream owns the register bank.
- `CalibrateOffset(targetMm)` / `CalibrateXtalk(targetMm)` run ST's
  calibration against a target at a known distance and return what they
  programmed.
- `Xshut(...)` / `ResetSensor()` power-cycle the sensor; `Init()` again after.
- A stream report whose result block does not decode is dropped and counted
  in `Measurements.ParseErrors`; streaming goes on.
- `tof.Uld` is the driver itself, for raw register access — unstable, not
  covered by the 1.0 freeze.

## The codec surface

- `Vl53l4Cmd` — host→device opcodes (`ReadReg`, `WriteReg`, `Xshut`,
  `StartStream`, `StopStream`, `GetInfo`, `SetI2cSpeed`).
- `Vl53l4Rpt` — device→host report ids (`RegData`, `Info`, `Stream`).
- `Vl53l4Wire` — wire limits (`XferMax`, `FlagIntActHigh`, `I2cKhzSteps`) and
  the command payload packers (`PackReadReg`, `PackWriteReg`, `PackXshut`,
  `PackStartStream`, `PackSetI2cSpeed`).
- `Vl53l4RegData` / `Vl53l4Info` / `Vl53l4StreamData` — decoded reports, each
  with a static `Unpack`.
- `Vl53l4cdDriver` — the ST ULD itself (init, ranging, timing, tuning words,
  calibration) over the register bridge; `tof.Uld` hands it out. Unstable —
  not covered by the 1.0 freeze. Its register-level pieces (the result-block
  decode, the range-timing register math, the tuning-word codecs, the init
  block) are internal: the device class and the driver are the way to them.

## Read and write registers

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l4;

// read the model id word: IDENTIFICATION__MODEL_ID (0x010F), expect Vl53l4cdDevice.ModelId (0xEBAA)
byte[] rd = Framing.BuildPacket(
    (int)Vl53l4Cmd.ReadReg,
    Vl53l4Wire.PackReadReg(0x010F, 2));

// the reply is RPT_VL53_REG_DATA (Vl53l4Rpt.RegData, 0x91)
Vl53l4RegData d = Vl53l4RegData.Unpack(replyPayload);

// write a register block in one transaction: SYSTEM__START (0x0087) = 0x80, stop ranging
byte[] wr = Framing.BuildPacket(
    (int)Vl53l4Cmd.WriteReg,
    Vl53l4Wire.PackWriteReg(0x0087, new byte[] { 0x80 }));
```

On an opened device the same reads and writes are `tof.Uld.ReadWord(0x010F)`
and `tof.Uld.WriteByte(0x0087, 0x80)`; the ULD's init block is `tof.Init()`.

Reads and writes are capped at `Vl53l4Wire.XferMax` (253) bytes and
`addr + len` ≤ 0x10000. Register contents are big-endian; the wire fields are
little-endian.

`Vl53l4Wire.PackXshut` takes a `Vl53l4Xshut` action (`Off` / `On` / `Reset`);
`Reset` is blocking on the MCU and is answered only after the sensor's boot
handshake (allow ≥ 1.5 s).

## Stream the result block

```csharp
// arm INT-driven streaming of the 17-byte result block at RESULT__RANGE_STATUS (0x0089)
byte[] start = Framing.BuildPacket(
    (int)Vl53l4Cmd.StartStream,
    Vl53l4Wire.PackStartStream(0x0089, 17));

// each sample arrives as RPT_VL53_STREAM (Vl53l4Rpt.Stream, 0x93)
Vl53l4StreamData s = Vl53l4StreamData.Unpack(streamPayload);
```

This is what `tof.StartRanging()` arms; the live device decodes every sample
into a `Vl53l4Measurement` on `Measurements`.

The `flags` byte's `Vl53l4Wire.FlagIntActHigh` bit (0x02) selects
INT-active-high; the default (0) matches the ULD init block (INT active low).
`Vl53l4StreamData.TimestampUs` is MCU uptime at the **INT edge** — the sensor
event, not the I2C completion — and `Addr`/`Len` echo the stream configuration
so each report is self-describing.

## The measurement

```csharp
Vl53l4Measurement m = tof.MeasureOnce();      // or stream.Take(...) while ranging
if (m.Valid)                                  // RangeStatus == 0
    Console.WriteLine($"{m.DistanceMm} mm, sigma {m.SigmaMm} mm");
```

`Vl53l4Measurement` is the 17-byte result block decoded exactly as
`VL53L4CD_GetResult()` decodes the same registers one by one: `RangeStatus`
through ST's status table (0 valid, the names are
`Vl53l4cdDevice.RangeStatusNames`; raw ≥ 24 passes through unmapped),
`DistanceMm`, `SignalRateKcps` / `AmbientRateKcps` (×8), the per-SPAD rates,
`NumberOfSpad`, `SigmaMm`, the sensor's own `StreamCount` frame counter (wraps
at 255) and the MCU `TimestampUs` of the INT edge. `tof.Uld.GetResult()` is the
polled read of the same block as a `Vl53l4Results` (no timestamp). A stream
report whose block is shorter than 17 bytes is dropped and counted in
`Measurements.ParseErrors`.

## Range timing

```csharp
tof.SetRangeTiming(50);                       // 50 ms budget, continuous
tof.SetRangeTiming(50, 100);                  // 50 ms budget every 100 ms: autonomous low power
var (budgetMs, interMs) = tof.GetRangeTiming();
```

Budget is 10..200 ms. An inter-measurement period of 0 selects continuous
mode; a value **greater** than the budget selects autonomous low power;
anything else is refused with an `ArgumentOutOfRangeException` before any
register is written (contract 10 §5). The register math under it
(RANGE_CONFIG_A / B and INTERMEASUREMENT_MS from the oscillator and PLL words)
is a bit-exact port of the C ULD, pinned by the golden vectors.

## Tuning words

```csharp
tof.SetOffsetMm(-10);                         // RANGE_OFFSET_MM (0x001E); INNER / OUTER zeroed alongside
int mm = tof.GetOffsetMm();                   // → -10
tof.SetXtalkKcps(20);                         // XTALK_PLANE_OFFSET_KCPS (0x0016)
tof.SetSignalThresholdKcps(1024);             // MIN_COUNT_RATE_RTN_LIMIT_MCPS (0x0066)
tof.SetSigmaThresholdMm(15);                  // RANGE_CONFIG__SIGMA_THRESH (0x0064)
tof.SetDetectionThresholds(100, 300, Vl53l4cdDevice.WindowIn);   // SYSTEM__INTERRUPT: WindowBelow / Above / Out / In
```

Each is a word codec pair in the ULD (encode for the write, decode for the
readback); an argument outside the register's range is an
`ArgumentOutOfRangeException`, never wrapped.

## Gotchas

- **Check `RangeStatus` before trusting a distance** — 0 is valid; everything
  else describes why the measurement is suspect (raw ≥ 24 is unmapped).
- **Two endiannesses on purpose** — wire fields little-endian, register
  contents big-endian. The codecs handle both; don't swap bytes yourself.
- **After `Xshut(...)` / `ResetSensor()` the sensor holds none of the ULD
  configuration** — `Init()` again (config block, VHV, timing).
- **Configure the sensor before raising the bus speed** — `Init()` writes the
  configuration block at 400 kHz (`Vl53l4cdDevice.I2cKhzBoot`) and only then
  re-times the bus to its `busKhz` (1 MHz, `Vl53l4cdDevice.I2cKhzDefault`, by
  default); `SetI2cSpeedKhz` re-times it later. Read `I2cKhz` back from
  `BridgeInfo()` for the programmed step (the command answers OK
  unconditionally).
- **Skipped stream slots produce no per-slot error** — watch the `Vl53l4Info`
  counters (`SlotsSkipped`, `I2cErrors`) and gaps in `TimestampUs`;
  `StreamCount` tells a frame the host never received from one the sensor
  never produced.
- **The driver is host code** — `Vl53l4cdDriver` (`tof.Uld`) runs the register
  sequences (`SensorInit`, calibration loops, waits) over the board bridge; it
  is the escape hatch for what the device class does not wrap — unstable, not
  covered by the 1.0 freeze.
