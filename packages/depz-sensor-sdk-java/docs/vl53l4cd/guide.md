# VL53L4CD — user guide

Hands-on guide to the `Vl53l4` wire codecs and the `Vl53l4Uld` host-ULD math.
For what the sensor is and its concepts, read the
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

- `Vl53l4.Vl53l4Cmd` — host→device opcodes (`READ_REG`, `WRITE_REG`, `XSHUT`,
  `START_STREAM`, `STOP_STREAM`, `GET_INFO`, `SET_I2C_SPEED`), each with a
  `.value` byte.
- `Vl53l4.Vl53l4Rpt` — device→host report ids (`REG_DATA`, `INFO`, `STREAM`).
- `Vl53l4.RegData` / `Vl53l4.Vl53l4Info` / `Vl53l4.StreamData` — decoded
  reports.
- `Vl53l4Uld` — the pure ULD pieces: `parseResultBlock`, the range-timing
  register math, the tuning-word codecs and `configBlock()`.

## Read and write registers

```java
import ai.depz.sensor.protocol.Vl53l4;
import ai.depz.sensor.sensors.vl53l4.Vl53l4Uld;
import ai.depz.sensor.transport.Framing;

// read the model id word (expect 0xEBAA)
byte[] rd = Framing.buildPacket(
        Vl53l4.Vl53l4Cmd.READ_REG.value,
        Vl53l4.packReadReg(Vl53l4Uld.IDENTIFICATION_MODEL_ID, 2), seq, crc);

// the reply is RPT_VL53_REG_DATA (0x91)
Vl53l4.RegData d = Vl53l4.RegData.unpack(replyPayload);

// write the 91-byte init configuration block in one transaction
byte[] wr = Framing.buildPacket(
        Vl53l4.Vl53l4Cmd.WRITE_REG.value,
        Vl53l4.packWriteReg(Vl53l4Uld.CONFIG_ADDR, Vl53l4Uld.configBlock()), seq, crc);
```

Reads and writes are capped at `Vl53l4.XFER_MAX` (253) bytes and
`addr + len` ≤ 0x10000. Register contents are big-endian; the wire fields are
little-endian.

## Stream the result block

```java
// arm INT-driven streaming of the 17-byte result block
byte[] start = Framing.buildPacket(
        Vl53l4.Vl53l4Cmd.START_STREAM.value,
        Vl53l4.packStartStream(Vl53l4Uld.RESULT_BLOCK_ADDR, Vl53l4Uld.RESULT_BLOCK_LEN, 0),
        seq, crc);

// each sample arrives as RPT_VL53_STREAM (0x93)
Vl53l4.StreamData s = Vl53l4.StreamData.unpack(streamPayload);
Vl53l4Uld.Results r = Vl53l4Uld.parseResultBlock(s.data());
```

`flags` bit `Vl53l4.SF_INT_ACT_HIGH` (0x02) selects INT-active-high; the
default (0) matches the ULD init block (INT active low). `timestampUs` is MCU
uptime at the **INT edge** — the sensor event, not the I2C completion.

## Decode a result block

```java
Vl53l4Uld.Results r = Vl53l4Uld.parseResultBlock(raw17);
if (r.rangeStatus() == 0) {                 // 0 = valid
    System.out.println(r.distanceMm() + " mm, sigma " + r.sigmaMm() + " mm");
}
```

`Results` carries `rangeStatus` (via the `STATUS_RTN` table; raw ≥ 24 passes
through unmapped), `distanceMm`, `signalRateKcps`/`ambientRateKcps` (×8),
per-SPAD rates, `numberOfSpad`, `sigmaMm` and the sensor's own `streamCount`
frame counter (wraps at 255).

## Range timing

The SetRangeTiming/GetRangeTiming register math is pure and bit-exact:

```java
// SetRangeTiming(50 ms budget, continuous): oscFrequency from reg 0x0006,
// clockPll from RESULT__OSC_CALIBRATE_VAL (only used in autonomous mode)
int[] regs = Vl53l4Uld.rangeTimingRegisters(50, 0, oscFrequency, clockPll);
// regs = {RANGE_CONFIG_A, RANGE_CONFIG_B, INTERMEASUREMENT_MS raw dword}

// GetRangeTiming readback
int[] timing = Vl53l4Uld.decodeRangeTiming(interRaw, clockPll, oscFrequency, regs[0]);
// timing = {timing_budget_ms, inter_measurement_ms}
```

Budget is 10..200 ms. `inter_measurement_ms == 0` selects continuous mode; a
value **greater** than the budget selects autonomous low power; anything else
throws `Vl53l4Uld.Vl53l4Error`.

## Tuning words

Each tuning register is a word codec pair (encode for the write, decode for the
readback):

```java
int word = Vl53l4Uld.offsetRaw(-10);            // RANGE_OFFSET_MM (0x001E)
int mm   = Vl53l4Uld.decodeOffset(word);        // → -10

Vl53l4Uld.xtalkRaw(20);                         // XTALK_PLANE_OFFSET_KCPS (0x0016)
Vl53l4Uld.signalThresholdRaw(1024);             // MIN_COUNT_RATE_RTN_LIMIT_MCPS (0x0066)
Vl53l4Uld.sigmaThresholdRaw(15);                // RANGE_CONFIG__SIGMA_THRESH (0x0064)
```

## Gotchas

- **Check `rangeStatus` before trusting a distance** — 0 is valid; everything
  else describes why the measurement is suspect (raw ≥ 24 is unmapped).
- **Two endiannesses on purpose** — wire fields little-endian, register
  contents big-endian. The codecs handle both; don't swap bytes yourself.
- **After `XSHUT` OFF/RESET the sensor holds none of the ULD configuration** —
  re-run the init sequence (config block, VHV, timing).
- **Configure the sensor before raising the bus speed** — init at 400 kHz,
  then `SET_I2C_SPEED` to 1 MHz; read `i2cKhz` back from `GET_INFO` for the
  programmed step (the command answers OK unconditionally).
- **Skipped stream slots produce no per-slot error** — watch the
  `Vl53l4Info` counters (`slotsSkipped`, `i2cErrors`) and gaps in
  `timestampUs`; `streamCount` tells a frame the host never received from one
  the sensor never produced.
- **The live driver is host code you write** — `Vl53l4Uld` is the pure math
  only; `sensor_init`, calibration loops and waits are an extension point
  (`liveDriverStubbed()`), driven over `READ_REG`/`WRITE_REG`.
