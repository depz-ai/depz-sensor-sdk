# BNO055 — user guide

Hands-on guide to the [`bno055`](api.md) codec and decode layer. For what the
sensor is and its concepts, read the [introduction](introduction.md); for
exact signatures see the [API reference](api.md).

This crate does not drive the sensor. The session logic — going through
CONFIG for every mode change, waiting for the boot and the fusion start, page
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

```rust
use depz_sensor_sdk::bno055::regs::{REG_OPR_MODE, REG_UNIT_SEL};
use depz_sensor_sdk::bno055::{
    pack_read_reg, pack_start_stream, pack_write_reg, Bno055Cmd, OprMode, Units, FULL_BLOCK,
    TRIGGER_TIMER,
};
use depz_sensor_sdk::framing::{build_packet, CrcType};

let frame = |cmd: Bno055Cmd, payload: &[u8]| build_packet(cmd as u8, payload, 0, CrcType::None).unwrap();

// A session as register writes: CONFIG → units → NDOF (wait ~70 ms for the fusion).
let to_config = frame(Bno055Cmd::WriteReg, &pack_write_reg(REG_OPR_MODE, &[OprMode::Config as u8]));
let units = frame(Bno055Cmd::WriteReg, &pack_write_reg(REG_UNIT_SEL, &[Units::default().pack()]));
let to_ndof = frame(Bno055Cmd::WriteReg, &pack_write_reg(REG_OPR_MODE, &[OprMode::Ndof as u8]));

// Stream the full 46-byte block (0x08..0x35) every 10 ms.
let (addr, len) = FULL_BLOCK;
let start = frame(Bno055Cmd::StartStream, &pack_start_stream(TRIGGER_TIMER, addr, len, 10));
let stop = frame(Bno055Cmd::StopStream, &[]);
let read_calib_stat = frame(Bno055Cmd::ReadReg, &pack_read_reg(0x35, 1));
let reset = frame(Bno055Cmd::Reset, &[]);          // answered after the chip-ID handshake (~0.5 s)
let get_info = frame(Bno055Cmd::GetInfo, &[]);
```

`BNO_START_STREAM` replaces any running stream. `TRIGGER_TIMER` reads every
`period_ms` (1..60000) and is the only data trigger on sensor firmware 03.11;
`TRIGGER_INT` reads on each INT edge and is for the motion interrupts
(`period_ms` is then a missed-edge watchdog, 0 disables it). The 46-byte
block costs ≈3.2 ms of bus in NDOF, the 8-byte [`QUAT_BLOCK`](api.md#quat_block)
≈1.2 ms.

## Hello-world: decode the stream

Every `RPT_BNO_REG_STREAM` report echoes its window (`addr`, `len`), so
[`decode_block`](api.md#decode_block) needs nothing else. A channel is `None`
when the window does not cover all of its bytes:

```rust
use depz_sensor_sdk::bno055::{decode_block, Bno055Rpt, CalibStatus, StreamData, Units, QUAT_LSB};
use depz_sensor_sdk::framing::{Event, PacketParser};

let units = Units::default();          // the UNIT_SEL written before START_STREAM
let mut parser = PacketParser::new();
for ev in parser.feed(&rx_bytes) {
    let Event::Packet(pkt) = ev else { continue };
    if pkt.cmd != Bno055Rpt::Stream as u8 {
        continue;
    }
    let s = StreamData::unpack(&pkt.payload)?;       // timestamp_us, addr, len, data
    let raw = decode_block(s.addr, &s.data);
    if let Some([h, r, p]) = raw.euler {
        let deg = |v: i16| v as f64 / units.euler_lsb();
        println!("{} µs heading {:6.1} roll {:6.1} pitch {:6.1}", s.timestamp_us, deg(h), deg(r), deg(p));
    }
    if let Some([w, x, y, z]) = raw.quaternion {
        let q = |v: i16| v as f64 / QUAT_LSB;
        println!("q = ({:+.4}, {:+.4}, {:+.4}, {:+.4})", q(w), q(x), q(y), q(z));
    }
    if let Some(c) = raw.calib_stat {
        let st = CalibStatus::unpack(c);
        println!("calib sys {} gyro {} accel {} mag {}", st.system, st.gyro, st.accel, st.mag);
    }
}
```

The timestamp is the MCU time of the timer tick, not the I2C completion; a
sample the USB link could not take is dropped, so gaps show as jumps in
`timestamp_us`. A quaternion of all zeros means the fusion is not running:
CONFIG mode, a non-fusion mode, or the first ~70 ms after a switch out of
CONFIG.

## Units and scaling

`decode_block` returns register integers; `value = raw / LSB`:

| channel | field | LSB | unit |
|---|---|---|---|
| acceleration | `accel` | `units.accel_lsb()`: 100 / 1 | m/s² / mg |
| linear acceleration | `linear_accel` | [`FUSION_ACCEL_LSB`](api.md#fusion_accel_lsb) = 100 | **always m/s²** |
| gravity | `gravity` | `FUSION_ACCEL_LSB` = 100 | **always m/s²** |
| angular rate | `gyro` | `units.gyro_lsb()`: 16 / 900 | deg/s / rad/s |
| Euler angles | `euler` | `units.euler_lsb()`: 16 / 900 | degrees / radians |
| quaternion | `quaternion` | [`QUAT_LSB`](api.md#quat_lsb) = 16384 | — |
| magnetic field | `mag` | [`MAG_LSB`](api.md#mag_lsb) = 16 | µT |
| temperature | `temperature` | `units.temp_lsb()`: 1 / 0.5 | °C / °F (1 LSB = 2 °F) |

```rust
use depz_sensor_sdk::bno055::{decode_block, Units, FUSION_ACCEL_LSB, MAG_LSB};

let units = Units { accel_mg: true, ..Units::default() };
assert_eq!(units.pack(), 0x01);
let raw = decode_block(s.addr, &s.data);
if let (Some(a), Some(g)) = (raw.accel, raw.gravity) {
    let accel_mg = a[2] as f64 / units.accel_lsb();       // mg: UNIT_SEL bit 0 is set
    let gravity_ms2 = g[2] as f64 / FUSION_ACCEL_LSB;     // still m/s² — bit 0 is ignored here
    println!("{accel_mg:.0} mg, gravity {gravity_ms2:.2} m/s²");
}
if let Some(m) = raw.mag {
    println!("|B|x = {:.1} µT", m[0] as f64 / MAG_LSB);
}
```

**Linear acceleration and gravity ignore the ACC unit bit**: they stay in m/s²
at 100 LSB even with `accel_mg` set (measured on sensor firmware 03.11; the
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

`Units::default()` is UNIT_SEL `0x00` — m/s², deg/s, degrees, °C, Windows
orientation — the SDK default. The sensor's own power-on value is `0x80`
(Android), so write UNIT_SEL explicitly in CONFIG, and scale a stream by the
units written **before** `BNO_START_STREAM`: the first sample can arrive
before the command's own reply.

## Operating modes

```rust
use depz_sensor_sdk::bno055::OprMode;

let mode = OprMode::from_reg(0x1C).unwrap();       // OPR_MODE reads back with bit 4 set after reset
assert_eq!(mode, OprMode::Ndof);
assert!(mode.is_fusion());
assert!(!OprMode::Amg.is_fusion());                 // raw sensors only
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
still booting: poll SYS_STATUS (`0x39`) until it leaves
`regs::SYS_STATUS_BOOTING` (2, 3, 4) before configuring, or the mode you write
is lost.

## Calibration status and profile

```rust
use depz_sensor_sdk::bno055::regs::REG_CALIB_PROFILE;
use depz_sensor_sdk::bno055::{
    pack_read_reg, pack_write_reg, CalibStatus, CalibrationProfile, RegData, CALIB_PROFILE_LEN,
};

let st = CalibStatus::unpack(0xFF);
assert!(st.fully_calibrated());                     // sys/gyro/accel/mag all 3

// Read the 22-byte profile (CONFIG mode only), keep it, write it back later in one transfer.
let read = pack_read_reg(REG_CALIB_PROFILE, CALIB_PROFILE_LEN as u8);
let reply = RegData::unpack(&reg_data_payload)?;    // the RPT_BNO_REG_DATA answer
let profile = CalibrationProfile::unpack(&reply.data)?;   // exactly 22 bytes
if (144..=1280).contains(&profile.mag_radius) {
    let restore = pack_write_reg(REG_CALIB_PROFILE, &profile.pack());
}
```

What each sensor needs to calibrate (datasheet §3.11): **gyro** — hold still
for a few seconds; **accel** — six still poses, each axis up and down;
**magnetometer** — slow figure-eights in the air. The fusion calibrates
continuously in the background and cannot be told not to.

A restored profile is a starting point, not a lock: once the fusion runs it
keeps refining the offsets. **Don't store the profile of an uncalibrated
sensor**: its `mag_radius` is 0, outside the legal 144..1280, and restoring it
makes the sensor report a fusion configuration error (SYS_ERR 9). Store a
profile only once CALIB_STAT reads 3/3/3/3; verify a write by reading it back
without leaving CONFIG. The soft-iron matrix at `0x43` has its own codec:
[`pack_sic_matrix`](api.md#pack_sic_matrix) / `unpack_sic_matrix` (9 × i16,
1.0 = 16384, identity [`SIC_IDENTITY`](api.md#sic_identity)).

## Axis remap and placements

```rust
use depz_sensor_sdk::bno055::regs::{AXIS_X, AXIS_Y, AXIS_Z, REG_AXIS_MAP_CONFIG};
use depz_sensor_sdk::bno055::{pack_write_reg, AxisRemap, PLACEMENTS};

// One of the datasheet mountings P0..P7 (P1 is the default: chip axes as printed).
let p2 = AxisRemap::placement("P2").expect("P0..P7");
let (config, sign) = p2.pack()?;
assert_eq!((config, sign), (0x24, 0x06));
assert_eq!(PLACEMENTS[2], ("P2", 0x24, 0x06));

// Or any permutation: output X = −chip Y, output Y = chip X.
let custom = AxisRemap { x: AXIS_Y, y: AXIS_X, z: AXIS_Z, x_negative: true, ..AxisRemap::default() };
let (config, sign) = custom.pack()?;
let write = pack_write_reg(REG_AXIS_MAP_CONFIG, &[config, sign]);   // 0x41 and 0x42 in one write, CONFIG only

// A mapping that uses one axis twice is refused: the sensor would silently keep the old one.
assert!(AxisRemap { x: AXIS_X, y: AXIS_X, ..AxisRemap::default() }.pack().is_err());
```

`AXIS_MAP_CONFIG` is `z<5:4> y<3:2> x<1:0>` (the source axis of each output
axis), `AXIS_MAP_SIGN` is `x 2, y 1, z 0` (1 = negative).

## Page 1: sensor configs

The accel / gyro / mag configs, the interrupt setup and the unique id live on
register page 1. The host owns the page select: write `PAGE_ID ← 1`, the
access, then `PAGE_ID ← 0` — and never while a stream runs (the bridge would
stream page-1 registers):

```rust
use depz_sensor_sdk::bno055::regs::{
    ACC_RANGE_G, GYR_RANGE_DPS, MAG_RATE_HZ, REG1_ACC_CONFIG, REG_PAGE_ID,
};
use depz_sensor_sdk::bno055::{pack_write_reg, AccelConfig, GyroConfig, MagConfig};

let accel = AccelConfig { range: 2, ..AccelConfig::default() };   // ±8 g, 62.5 Hz, normal
assert_eq!(ACC_RANGE_G[accel.range as usize], 8);
let writes = [
    pack_write_reg(REG_PAGE_ID, &[1]),
    pack_write_reg(REG1_ACC_CONFIG, &[accel.pack()]),
    pack_write_reg(REG_PAGE_ID, &[0]),
];

let gyro = GyroConfig::unpack(&[0x38, 0x00])?;      // power-on GYR_CONFIG_0/1
assert_eq!(GYR_RANGE_DPS[gyro.range as usize], 2000);
let mag = MagConfig::unpack(0x8B);                  // bit 7 is not a field
assert_eq!((MAG_RATE_HZ[mag.rate as usize], mag.pack()), (10, 0x0B));
```

These configs take effect in the **non-fusion** modes only; the fusion modes
override them. On these boards only the motion interrupts work
(any/no-motion, high-g, high-rate): the data-ready bits exist but never fire
on sensor firmware 03.11.

## Bridge info

`RPT_BNO_INFO` (38 bytes) carries the sensor identity and the bridge
counters:

```rust
use depz_sensor_sdk::bno055::{i2c_error_name, Bno055Info};

let info = Bno055Info::unpack(&info_payload)?;      // CodecError below 38 bytes
println!(
    "ids ok: {}, sensor firmware {}, block read avg {} µs, {} slots skipped, {} sensor resets, last I2C error {}",
    info.ids_ok(), info.sw_rev_text(), info.read_avg_us, info.slots_skipped,
    info.sensor_resets, i2c_error_name(info.last_i2c_error)
);
```

The counters are free-running and wrap silently — watch increments. A rising
`sensor_resets` means the bridge recovered a stuck bus by pulsing nRESET: the
stream keeps running, but the sensor is back in CONFIG with power-on settings
(every output zero) and must be configured again.

## Gotchas

- **Linear acceleration and gravity are always m/s²** — scale them with
  `FUSION_ACCEL_LSB`, not `units.accel_lsb()`.
- **UNIT_SEL is not the datasheet's §4.3.60 table** — use the `UNIT_*`
  constants / `Units::pack`.
- **Scale a stream by the units written before `BNO_START_STREAM`.**
- **Zero quaternion = not fusing** — CONFIG, a non-fusion mode, or the first
  ~70 ms after leaving CONFIG. Every CONFIG round trip (reading the profile,
  writing units) restarts that gap.
- **No direct mode-to-mode switch** — go through CONFIG.
- **Settings are silently ignored outside CONFIG** — units, axis remap,
  calibration profile, power mode.
- **Don't store a profile with `mag_radius` 0** — restoring it causes a
  fusion configuration error.
- **Never switch pages under a running stream**, and always return to page 0.
- **INT_STA (`0x37`) clears on read** — keep it out of routine block reads
  (`FULL_BLOCK` stops at `0x35`).
- **Don't benchmark in CONFIG** — the sensor stretches the bus 3–5× harder
  there (a 46-byte read ≈10 ms instead of ≈3 ms).
