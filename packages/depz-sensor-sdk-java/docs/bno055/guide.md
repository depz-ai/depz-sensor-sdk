# BNO055 — user guide

Hands-on guide to the BNO055 wire codecs ([`Bno055`](api.md#bno055)) and
register codecs ([`Bno055Regs`](api.md#bno055regs)). For what the sensor is
and its concepts, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md).

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

The bridge has six commands (`Bno055.Bno055Cmd`, `0x32`–`0x37`). Register
addresses and lengths are one byte; a transfer is 1..128 bytes and
`addr + len` stays within `0x100`:

```java
import ai.depz.sensor.protocol.Bno055;
import ai.depz.sensor.sensors.bno055.Bno055Regs;
import ai.depz.sensor.transport.CrcType;
import ai.depz.sensor.transport.Framing;

// A session as register writes: CONFIG → units → NDOF (wait ~70 ms for the fusion).
byte[] toConfig = Framing.buildPacket(Bno055.Bno055Cmd.WRITE_REG.value,
        Bno055.packWriteReg(Bno055Regs.REG_OPR_MODE, new byte[] {(byte) Bno055Regs.OprMode.CONFIG.value}),
        0, CrcType.NONE);
byte[] units = Framing.buildPacket(Bno055.Bno055Cmd.WRITE_REG.value,
        Bno055.packWriteReg(Bno055Regs.REG_UNIT_SEL, new byte[] {(byte) Bno055Regs.Units.DEFAULT.pack()}),
        0, CrcType.NONE);
byte[] toNdof = Framing.buildPacket(Bno055.Bno055Cmd.WRITE_REG.value,
        Bno055.packWriteReg(Bno055Regs.REG_OPR_MODE, new byte[] {(byte) Bno055Regs.OprMode.NDOF.value}),
        0, CrcType.NONE);

// Stream the full 46-byte block (0x08..0x35) every 10 ms.
byte[] start = Framing.buildPacket(Bno055.Bno055Cmd.START_STREAM.value,
        Bno055.packStartStream(Bno055.TRIGGER_TIMER, Bno055Regs.FULL_BLOCK_ADDR, Bno055Regs.FULL_BLOCK_LEN, 10),
        0, CrcType.NONE);
byte[] stop = Framing.buildPacket(Bno055.Bno055Cmd.STOP_STREAM.value);
byte[] readCalibStat = Framing.buildPacket(Bno055.Bno055Cmd.READ_REG.value,
        Bno055.packReadReg(Bno055Regs.REG_CALIB_STAT, 1), 0, CrcType.NONE);
byte[] reset = Framing.buildPacket(Bno055.Bno055Cmd.RESET.value);    // answered after the chip-ID handshake (~0.5 s)
byte[] getInfo = Framing.buildPacket(Bno055.Bno055Cmd.GET_INFO.value);
```

`BNO_START_STREAM` replaces any running stream. `TRIGGER_TIMER` reads every
`periodMs` (1..60000) and is the only data trigger on sensor firmware 03.11;
`TRIGGER_INT` reads on each INT edge and is for the motion interrupts
(`periodMs` is then a missed-edge watchdog, 0 disables it). The 46-byte block
costs ≈3.2 ms of bus in NDOF, the 8-byte quaternion block
(`QUAT_BLOCK_ADDR` / `QUAT_BLOCK_LEN`) ≈1.2 ms.

## Hello-world: decode the stream

Every `RPT_BNO_REG_STREAM` report echoes its window (`addr`, `len`), so
`Bno055Regs.decodeBlock` needs nothing else. A channel is `null` when the
window does not cover all of its bytes:

```java
import ai.depz.sensor.protocol.Bno055;
import ai.depz.sensor.sensors.bno055.Bno055Regs;
import ai.depz.sensor.transport.Event;
import ai.depz.sensor.transport.Packet;
import ai.depz.sensor.transport.PacketParser;

Bno055Regs.Units units = Bno055Regs.Units.DEFAULT;    // the UNIT_SEL written before START_STREAM
PacketParser parser = new PacketParser();
for (Event ev : parser.feed(rxBytes)) {
    if (!(ev instanceof Packet p) || p.cmd() != Bno055.Bno055Rpt.STREAM.value) {
        continue;
    }
    Bno055.StreamData s = Bno055.StreamData.unpack(p.payload());   // timestampUs, addr, len, data
    Bno055Regs.RawBlock raw = Bno055Regs.decodeBlock(s.addr(), s.data());
    if (raw.euler() != null) {
        int[] e = raw.euler();                                        // heading, roll, pitch
        System.out.printf("%d µs heading %6.1f roll %6.1f pitch %6.1f%n", s.timestampUs(),
                e[0] / units.eulerLsb(), e[1] / units.eulerLsb(), e[2] / units.eulerLsb());
    }
    if (raw.quaternion() != null) {
        int[] q = raw.quaternion();                                   // w, x, y, z
        System.out.printf("q = (%+.4f, %+.4f, %+.4f, %+.4f)%n", q[0] / Bno055Regs.QUAT_LSB,
                q[1] / Bno055Regs.QUAT_LSB, q[2] / Bno055Regs.QUAT_LSB, q[3] / Bno055Regs.QUAT_LSB);
    }
    if (raw.calibStat() != null) {
        Bno055Regs.CalibStatus st = Bno055Regs.CalibStatus.unpack(raw.calibStat());
        System.out.println("calib sys " + st.system() + " gyro " + st.gyro()
                + " accel " + st.accel() + " mag " + st.mag());
    }
}
```

The timestamp is the MCU time of the timer tick, not the I2C completion; a
sample the USB link could not take is dropped, so gaps show as jumps in
`timestampUs`. A quaternion of all zeros means the fusion is not running:
CONFIG mode, a non-fusion mode, or the first ~70 ms after a switch out of
CONFIG.

## Units and scaling

`decodeBlock` returns register integers; `value = raw / LSB`:

| channel | field | LSB | unit |
|---|---|---|---|
| acceleration | `accel` | `units.accelLsb()`: 100 / 1 | m/s² / mg |
| linear acceleration | `linearAccel` | `FUSION_ACCEL_LSB` = 100 | **always m/s²** |
| gravity | `gravity` | `FUSION_ACCEL_LSB` = 100 | **always m/s²** |
| angular rate | `gyro` | `units.gyroLsb()`: 16 / 900 | deg/s / rad/s |
| Euler angles | `euler` | `units.eulerLsb()`: 16 / 900 | degrees / radians |
| quaternion | `quaternion` | `QUAT_LSB` = 16384 | — |
| magnetic field | `mag` | `MAG_LSB` = 16 | µT |
| temperature | `temperature` | `units.tempLsb()`: 1 / 0.5 | °C / °F (1 LSB = 2 °F) |

```java
import ai.depz.sensor.sensors.bno055.Bno055Regs;

Bno055Regs.Units units = new Bno055Regs.Units(true, false, false, false, false);   // accel in mg
assert units.pack() == 0x01;
Bno055Regs.RawBlock raw = Bno055Regs.decodeBlock(s.addr(), s.data());
if (raw.accel() != null && raw.gravity() != null) {
    double accelMg = raw.accel()[2] / units.accelLsb();                 // mg: UNIT_SEL bit 0 is set
    double gravityMs2 = raw.gravity()[2] / Bno055Regs.FUSION_ACCEL_LSB; // still m/s² — bit 0 is ignored here
    System.out.printf("%.0f mg, gravity %.2f m/s²%n", accelMg, gravityMs2);
}
if (raw.mag() != null) {
    System.out.printf("Bx = %.1f µT%n", raw.mag()[0] / Bno055Regs.MAG_LSB);
}
```

**Linear acceleration and gravity ignore the ACC unit bit**: they stay in m/s²
at 100 LSB even with `accelMg` set (measured on sensor firmware 03.11; the
datasheet tables promise mg). Only `accel` switches.

UNIT_SEL (`0x3B`) bits as the silicon implements them — the datasheet's
§4.3.60 bit table is off by one:

| bit | constant | set means |
|---|---|---|
| 0 | `UNIT_ACC_MG` | acceleration in mg |
| 1 | `UNIT_GYR_RPS` | angular rate in rad/s |
| 2 | `UNIT_EUL_RAD` | Euler angles in radians |
| 4 | `UNIT_TEMP_F` | temperature in °F |
| 7 | `UNIT_ORI_ANDROID` | Android orientation (flips the pitch sign) |

`Units.DEFAULT` is UNIT_SEL `0x00` — m/s², deg/s, degrees, °C, Windows
orientation — the SDK default. The sensor's own power-on value is `0x80`
(Android), so write UNIT_SEL explicitly in CONFIG, and scale a stream by the
units written **before** `BNO_START_STREAM`: the first sample can arrive
before the command's own reply.

## Operating modes

```java
import ai.depz.sensor.sensors.bno055.Bno055Regs;

int opr = 0x1C & 0x0F;                   // OPR_MODE reads back with bit 4 set after reset: mask
assert opr == Bno055Regs.OprMode.NDOF.value;
assert Bno055Regs.OprMode.NDOF.isFusion();
assert !Bno055Regs.OprMode.AMG.isFusion();         // raw sensors only
```

| mode | sensors | output |
|---|---|---|
| `CONFIG` | — | configuration only, all outputs zero |
| `ACCONLY` … `AMG` | as named | raw data only; fusion registers read zero |
| `IMU` | accel + gyro | relative orientation, 100 Hz |
| `COMPASS` | accel + mag | absolute heading, 20 Hz |
| `M4G` | accel + mag | relative orientation from the magnetometer, 50 Hz |
| `NDOF_FMC_OFF` | all three | absolute orientation, 100 Hz, slow mag calibration |
| `NDOF` | all three | absolute orientation, 100 Hz |

The sensor only switches **between CONFIG and an operating mode**: a write of
OPR_MODE from one operating mode straight to another (NDOF → AMG) is silently
ignored. Go through CONFIG (any → CONFIG takes 19 ms, CONFIG → any 7 ms).
After `BNO_RESET` the reply comes at the chip-ID handshake, but the sensor is
still booting: poll SYS_STATUS (`REG_SYS_STATUS`, `0x39`) until it leaves 2,
3 and 4 before configuring, or the mode you write is lost.

## Calibration status and profile

```java
import ai.depz.sensor.protocol.Bno055;
import ai.depz.sensor.sensors.bno055.Bno055Regs;

assert Bno055Regs.CalibStatus.unpack(0xFF).fullyCalibrated();     // sys/gyro/accel/mag all 3

// Read the 22-byte profile (CONFIG mode only), keep it, write it back later in one transfer.
byte[] read = Bno055.packReadReg(Bno055Regs.REG_CALIB_PROFILE, Bno055Regs.CALIB_PROFILE_LEN);
Bno055.RegData reply = Bno055.RegData.unpack(regDataPayload);       // the RPT_BNO_REG_DATA answer
Bno055Regs.CalibrationProfile profile = Bno055Regs.CalibrationProfile.unpack(reply.data());   // exactly 22 bytes
if (profile.magRadius() >= 144 && profile.magRadius() <= 1280) {
    byte[] restore = Bno055.packWriteReg(Bno055Regs.REG_CALIB_PROFILE, profile.pack());
}
```

What each sensor needs to calibrate (datasheet §3.11): **gyro** — hold still
for a few seconds; **accel** — six still poses, each axis up and down;
**magnetometer** — slow figure-eights in the air. The fusion calibrates
continuously in the background and cannot be told not to.

A restored profile is a starting point, not a lock: once the fusion runs it
keeps refining the offsets. **Don't store the profile of an uncalibrated
sensor**: its `magRadius` is 0, outside the legal 144..1280, and restoring it
makes the sensor report a fusion configuration error (SYS_ERR 9). Store a
profile only once CALIB_STAT reads 3/3/3/3; verify a write by reading it back
without leaving CONFIG. The soft-iron matrix at `0x43` has its own codec:
`packSicMatrix` / `unpackSicMatrix` (9 × i16, 1.0 = 16384, identity
`SIC_IDENTITY`).

## Axis remap and placements

```java
import ai.depz.sensor.protocol.Bno055;
import ai.depz.sensor.sensors.bno055.Bno055Regs;

// One of the datasheet mountings P0..P7 (P1 is the default: chip axes as printed).
int[] p2 = Bno055Regs.AxisRemap.placement("P2").pack();          // {AXIS_MAP_CONFIG, AXIS_MAP_SIGN}
assert p2[0] == 0x24 && p2[1] == 0x06;
assert java.util.Arrays.equals(Bno055Regs.PLACEMENTS.get("P2"), new int[] {0x24, 0x06});

// Or any permutation: output X = −chip Y, output Y = chip X.
Bno055Regs.AxisRemap custom = new Bno055Regs.AxisRemap(
        Bno055Regs.AXIS_Y, Bno055Regs.AXIS_X, Bno055Regs.AXIS_Z, true, false, false);
int[] cs = custom.pack();
byte[] write = Bno055.packWriteReg(Bno055Regs.REG_AXIS_MAP_CONFIG,
        new byte[] {(byte) cs[0], (byte) cs[1]});                 // 0x41 and 0x42 in one write, CONFIG only

// A mapping that uses one axis twice is refused: the sensor would silently keep the old one.
try {
    new Bno055Regs.AxisRemap(Bno055Regs.AXIS_X, Bno055Regs.AXIS_X, Bno055Regs.AXIS_Z, false, false, false).pack();
    throw new AssertionError("not refused");
} catch (IllegalArgumentException expected) {
}
```

`AXIS_MAP_CONFIG` is `z<5:4> y<3:2> x<1:0>` (the source axis of each output
axis), `AXIS_MAP_SIGN` is `x 2, y 1, z 0` (1 = negative).

## Page 1: sensor configs

The accel / gyro / mag configs, the interrupt setup and the unique id live on
register page 1. The host owns the page select: write `PAGE_ID ← 1`, the
access, then `PAGE_ID ← 0` — and never while a stream runs (the bridge would
stream page-1 registers):

```java
import ai.depz.sensor.protocol.Bno055;
import ai.depz.sensor.sensors.bno055.Bno055Regs;

Bno055Regs.AccelConfig accel = new Bno055Regs.AccelConfig(2, 3, 0);    // ±8 g, 62.5 Hz, normal
assert Bno055Regs.ACC_RANGE_G[accel.range()] == 8;
byte[][] writes = {
    Bno055.packWriteReg(Bno055Regs.REG_PAGE_ID, new byte[] {1}),
    Bno055.packWriteReg(Bno055Regs.REG1_ACC_CONFIG, new byte[] {(byte) accel.pack()}),
    Bno055.packWriteReg(Bno055Regs.REG_PAGE_ID, new byte[] {0}),
};

Bno055Regs.GyroConfig gyro = Bno055Regs.GyroConfig.unpack(new byte[] {0x38, 0x00});   // power-on GYR_CONFIG_0/1
assert Bno055Regs.GYR_RANGE_DPS[gyro.range()] == 2000;
Bno055Regs.MagConfig mag = Bno055Regs.MagConfig.unpack(0x8B);                          // bit 7 is not a field
assert Bno055Regs.MAG_RATE_HZ[mag.rate()] == 10 && mag.pack() == 0x0B;
```

These configs take effect in the **non-fusion** modes only; the fusion modes
override them. On these boards only the motion interrupts work
(any/no-motion, high-g, high-rate): the data-ready bits exist but never fire
on sensor firmware 03.11.

## Bridge info

`RPT_BNO_INFO` (38 bytes) carries the sensor identity and the bridge
counters:

```java
import ai.depz.sensor.protocol.Bno055;

Bno055.Bno055Info info = Bno055.Bno055Info.unpack(infoPayload);   // throws below 38 bytes
System.out.printf("ids ok: %b, sensor firmware %s, block read avg %d µs, %d slots skipped, %d sensor resets, last I2C error %s%n",
        info.idsOk(), info.swRevText(), info.readAvgUs(), info.slotsSkipped(),
        info.sensorResets(), Bno055.I2C_ERROR_NAMES[info.lastI2cError()]);
```

The counters are free-running and wrap silently — watch increments. A rising
`sensorResets` means the bridge recovered a stuck bus by pulsing nRESET: the
stream keeps running, but the sensor is back in CONFIG with power-on settings
(every output zero) and must be configured again.

## Gotchas

- **Linear acceleration and gravity are always m/s²** — scale them with
  `FUSION_ACCEL_LSB`, not `units.accelLsb()`.
- **UNIT_SEL is not the datasheet's §4.3.60 table** — use the `UNIT_*`
  constants / `Units.pack()`.
- **Scale a stream by the units written before `BNO_START_STREAM`.**
- **Zero quaternion = not fusing** — CONFIG, a non-fusion mode, or the first
  ~70 ms after leaving CONFIG. Every CONFIG round trip (reading the profile,
  writing units) restarts that gap.
- **No direct mode-to-mode switch** — go through CONFIG.
- **Settings are silently ignored outside CONFIG** — units, axis remap,
  calibration profile, power mode.
- **Don't store a profile with `magRadius` 0** — restoring it causes a fusion
  configuration error.
- **Never switch pages under a running stream**, and always return to page 0.
- **INT_STA (`0x37`) clears on read** — keep it out of routine block reads
  (the full block stops at `0x35`).
- **Don't benchmark in CONFIG** — the sensor stretches the bus 3–5× harder
  there (a 46-byte read ≈10 ms instead of ≈3 ms).
