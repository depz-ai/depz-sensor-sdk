# VL53L4CD — user guide

Hands-on guide to the `Vl53l4Wire` wire codecs and the `Vl53l4Uld` host-ULD
math (`Depz.Sensor.Vl53l4`). For what the sensor is and its concepts, read the
[introduction](introduction.md); for exact signatures see the
[API reference](api.md). This is a decode layer — the snippets build request
payloads and decode reply/report payloads; you supply the transport that
carries them (see the [common guide](../guide.md#getting-started)).

## Contents

- [The codec surface](#the-codec-surface)
- [Read and write registers](#read-and-write-registers)
- [Stream the result block](#stream-the-result-block)
- [Decode a result block](#decode-a-result-block)
- [Range timing](#range-timing)
- [Tuning words](#tuning-words)
- [Gotchas](#gotchas)

## The codec surface

- `Vl53l4Cmd` — host→device opcodes (`ReadReg`, `WriteReg`, `Xshut`,
  `StartStream`, `StopStream`, `GetInfo`, `SetI2cSpeed`).
- `Vl53l4Rpt` — device→host report ids (`RegData`, `Info`, `Stream`).
- `Vl53l4Wire` — wire limits (`XferMax`, `FlagIntActHigh`, `I2cKhzSteps`) and
  the command payload packers (`PackReadReg`, `PackWriteReg`, `PackXshut`,
  `PackStartStream`, `PackSetI2cSpeed`).
- `Vl53l4RegData` / `Vl53l4Info` / `Vl53l4StreamData` — decoded reports, each
  with a static `Unpack`.
- `Vl53l4Uld` — the pure ULD pieces: `ParseResultBlock`, the range-timing
  register math, the tuning-word codecs and `ConfigBlock()`.

## Read and write registers

```csharp
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l4;

// read the model id word (expect Vl53l4Uld.ModelId == 0xEBAA)
byte[] rd = Framing.BuildPacket(
    (int)Vl53l4Cmd.ReadReg,
    Vl53l4Wire.PackReadReg(Vl53l4Uld.IdentificationModelId, 2));

// the reply is RPT_VL53_REG_DATA (Vl53l4Rpt.RegData, 0x91)
Vl53l4RegData d = Vl53l4RegData.Unpack(replyPayload);

// write the 91-byte init configuration block in one transaction
byte[] wr = Framing.BuildPacket(
    (int)Vl53l4Cmd.WriteReg,
    Vl53l4Wire.PackWriteReg(Vl53l4Uld.ConfigAddr, Vl53l4Uld.ConfigBlock()));
```

Reads and writes are capped at `Vl53l4Wire.XferMax` (253) bytes and
`addr + len` ≤ 0x10000. Register contents are big-endian; the wire fields are
little-endian.

`Vl53l4Wire.PackXshut` takes a `Vl53l4Xshut` action (`Off` / `On` / `Reset`);
`Reset` is blocking on the MCU and is answered only after the sensor's boot
handshake (allow ≥ 1.5 s).

## Stream the result block

```csharp
// arm INT-driven streaming of the 17-byte result block
byte[] start = Framing.BuildPacket(
    (int)Vl53l4Cmd.StartStream,
    Vl53l4Wire.PackStartStream(Vl53l4Uld.ResultBlockAddr,
                               (ushort)Vl53l4Uld.ResultBlockLen));

// each sample arrives as RPT_VL53_STREAM (Vl53l4Rpt.Stream, 0x93)
Vl53l4StreamData s = Vl53l4StreamData.Unpack(streamPayload);
Vl53l4Results r = Vl53l4Uld.ParseResultBlock(s.Data);
```

The `flags` byte's `Vl53l4Wire.FlagIntActHigh` bit (0x02) selects
INT-active-high; the default (0) matches the ULD init block (INT active low).
`Vl53l4StreamData.TimestampUs` is MCU uptime at the **INT edge** — the sensor
event, not the I2C completion — and `Addr`/`Len` echo the stream configuration
so each report is self-describing.

## Decode a result block

```csharp
Vl53l4Results r = Vl53l4Uld.ParseResultBlock(raw17);
if (r.RangeStatus == 0)                       // 0 = valid
    Console.WriteLine($"{r.DistanceMm} mm, sigma {r.SigmaMm} mm");
```

`Vl53l4Results` carries `RangeStatus` (via the `Vl53l4Uld.StatusRtn` table;
raw ≥ 24 passes through unmapped), `DistanceMm`, `SignalRateKcps` /
`AmbientRateKcps` (×8), the per-SPAD rates, `NumberOfSpad`, `SigmaMm` and the
sensor's own `StreamCount` frame counter (wraps at 255). `ParseResultBlock`
throws `Vl53l4Exception` on a block shorter than 15 bytes.

## Range timing

The SetRangeTiming/GetRangeTiming register math is pure and bit-exact:

```csharp
// SetRangeTiming(50 ms budget, continuous): oscFrequency from reg 0x0006
// (Vl53l4Uld.OscFrequency), clockPll from RESULT__OSC_CALIBRATE_VAL
// (Vl53l4Uld.ResultOscCalibrateVal, only used in autonomous mode)
var regs = Vl53l4Uld.RangeTimingRegisters(
    timingBudgetMs: 50, interMeasurementMs: 0, oscFrequency, clockPll);
// regs.RangeConfigA, regs.RangeConfigB, regs.IntermeasurementRaw

// GetRangeTiming readback
var timing = Vl53l4Uld.DecodeRangeTiming(interRaw, clockPll, oscFrequency, regs.RangeConfigA);
// timing.TimingBudgetMs, timing.InterMeasurementMs
```

Budget is 10..200 ms. `interMeasurementMs == 0` selects continuous mode; a
value **greater** than the budget selects autonomous low power; anything else
throws `Vl53l4Exception`.

## Tuning words

Each tuning register is a word codec pair (encode for the write, decode for the
readback):

```csharp
ushort word = Vl53l4Uld.OffsetRaw(-10);          // RANGE_OFFSET_MM (0x001E)
int mm      = Vl53l4Uld.DecodeOffset(word);      // → -10

Vl53l4Uld.XtalkRaw(20);                          // XTALK_PLANE_OFFSET_KCPS (0x0016)
Vl53l4Uld.SignalThresholdRaw(1024);              // MIN_COUNT_RATE_RTN_LIMIT_MCPS (0x0066)
Vl53l4Uld.SigmaThresholdRaw(15);                 // RANGE_CONFIG__SIGMA_THRESH (0x0064)
```

## Gotchas

- **Check `RangeStatus` before trusting a distance** — 0 is valid; everything
  else describes why the measurement is suspect (raw ≥ 24 is unmapped).
- **Two endiannesses on purpose** — wire fields little-endian, register
  contents big-endian. The codecs handle both; don't swap bytes yourself.
- **After `Vl53l4Xshut.Off`/`Reset` the sensor holds none of the ULD
  configuration** — re-run the init sequence (config block, VHV, timing).
- **Configure the sensor before raising the bus speed** — init at 400 kHz
  (`Vl53l4Uld.I2cKhzBoot`), then `Vl53l4Cmd.SetI2cSpeed` to 1 MHz
  (`Vl53l4Uld.I2cKhzDefault`); read `I2cKhz` back from `Vl53l4Info` for the
  programmed step (the command answers OK unconditionally).
- **Skipped stream slots produce no per-slot error** — watch the `Vl53l4Info`
  counters (`SlotsSkipped`, `I2cErrors`) and gaps in `TimestampUs`;
  `StreamCount` tells a frame the host never received from one the sensor
  never produced.
- **The live driver is host code you write** — `Vl53l4Uld` is the pure math
  only; `sensor_init`, calibration loops and waits are an extension point
  (`Vl53l4Uld.LiveDriverStubbed`), driven over `ReadReg`/`WriteReg`.
