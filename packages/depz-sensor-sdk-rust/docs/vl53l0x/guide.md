# VL53L0X — user guide

Hands-on guide to the [`vl53lx`](api.md) device class and decode layer for the VL53L0X. For
what the sensor is and its concepts, read the [introduction](introduction.md);
for exact signatures see the [API reference](api.md).

The device class runs the ST driver (VL53L0X API 1.0.4) on the host; below it, the bridge
commands and the stateless decode of what the board streams.

## Contents

- [Live device](#live-device)
- [Which board is it](#which-board-is-it)
- [The product table](#the-product-table)
- [Stream commands](#stream-commands)
- [Decode the result block](#decode-the-result-block)
- [Reset and bridge diagnostics](#reset-and-bridge-diagnostics)
- [Gotchas](#gotchas)

## Live device

```rust
use std::time::Duration;
use depz_sensor_sdk::devices::open_on_link;

let tof = open_on_link(link, None, None, None)?.into_vl53lx()?;   // a Vl53l0xDevice underneath
tof.init(None, None)?;                                   // VL53L0X / uld
tof.configure(33, 0, Some("long-range"), None, None, None)?;
let frames = tof.measurements(64);
tof.start_ranging()?;
let m = frames.take(Duration::from_secs(1))?;
println!("{} mm, status {} ({}), {} targets", m.distance_mm, m.status, m.status_text, m.targets.len());
tof.stop_ranging()?;
# Ok::<(), depz_sensor_sdk::devices::DepzError>(())
```

- `init(driver, product)` binds the (product, driver kind) pair — `None`, `None`:
  the class's product, its first kind (`uld`, the only one). A pair the table lacks is refused with what the product has
  (`driver_kinds(None)`); `notes()` and `identify_product()` say what was bound,
  borrowed, and how far it reaches.
- `configure(budget_ms, inter_ms, mode, offset_mm, xtalk_kcps, signal_kcps)`
  re-initialises and applies a ranging configuration before every run (an
  unsupported group is refused before the re-init); ask `supports(group)` first.
  The L0X has the ranging profiles as modes, offset / crosstalk
  calibration and `perform_ref_spad_management()`; no ROI, no thresholds.
- Measurements stream to `on_measurement(callback)` and `measurements(capacity)`
  (`Vl53lxMeasurement`: distance, status, rates, `targets` and `bins` on the
  histogram driver); `measure_once(None)` polls one; `plottable()` and
  `plot_distances()` apply the family's 0 / 6 / 11 rule. A frame the driver
  cannot decode is dropped and counted in `stream_parse_errors()`.
- Configuration while ranging is refused (the stream owns the bus); `xshut(0)`
  drops the driver — `init` again; `bridge_info()` is safe while streaming.

The sections below are the layers under the device.

## Which board is it

Every 1D board runs the same firmware (`APP_VL53L0_4_*`, sensor type
`vl53lx`), so the firmware name cannot tell them apart.
[`resolve_class`](api.md#resolve_class) uses the production USB PID model,
else the first `VL53L<digit>…` in the device name:

```rust
use depz_sensor_sdk::usb_model_hint;
use depz_sensor_sdk::vl53lx::{product, resolve_class, Vl53lxClass};

let class = resolve_class(usb_model_hint(Some(vid), Some(pid)), &device_name);
assert_eq!(resolve_class(Some("vl53l0x"), ""), Vl53lxClass::Vl53l0x);
assert!(product("VL53L0X").unwrap().model_id_ok(0x00EE));   // cross-check the sensor's answer
println!("{}", class.as_str());
```

## The product table

```rust
use depz_sensor_sdk::vl53lx::{product, DriverKind, L0X_BLOCK_ADDR, L0X_BLOCK_LEN};

let p = product("VL53L0X").expect("a family product");
assert_eq!((p.usb_pid, p.model_id, p.reach_mm), (0xED41, 0x00EE, 2000));
assert_eq!(p.driver_kinds(), vec![DriverKind::Uld]);

let uld = p.default_bus_params();
assert_eq!(uld.addr_width, 1);                                    // 8-bit register space
assert_eq!((uld.block_addr, uld.block_len as usize), (L0X_BLOCK_ADDR, L0X_BLOCK_LEN));   // 0x14, 12 B
assert_eq!(uld.clear_steps, &[(0x0B, 0x01), (0x0B, 0x00)]);      // interrupt release, two writes
assert_eq!(uld.max_khz, 400);                                     // no FM+ on this part
assert_eq!(uld.die_variant, None);                                // not a die block
```

## Stream commands

Set the register-address width to 1 before the first register access of a
session (the model-id read included) — it is sticky, and 2 after a reset. Then
arm the stream with the block and the two interrupt-release writes:

```rust
use depz_sensor_sdk::framing::{build_packet, CrcType};
use depz_sensor_sdk::vl53lx::{pack_read_reg, pack_set_addr_width, pack_start_stream, product, Vl53lxCmd};

let bus = product("VL53L0X").unwrap().default_bus_params();
let width = build_packet(Vl53lxCmd::SetAddrWidth as u8, &pack_set_addr_width(1)?, 0, CrcType::None)?;
let model_id = build_packet(Vl53lxCmd::ReadReg as u8, &pack_read_reg(0xC0, 1), 0, CrcType::None)?;   // 0xEE
let start = build_packet(
    Vl53lxCmd::StartStream as u8,
    &pack_start_stream(bus.block_addr, bus.block_len, bus.clear_steps, 0)?,
    0,
    CrcType::None,
)?;
let stop = build_packet(Vl53lxCmd::StopStream as u8, &[], 0, CrcType::None)?;
```

At width 1 only the low byte of `addr` goes on the bus, and `addr + len` must
stay within `0x100`.

## Decode the result block

Each `RPT_VL53_STREAM` report carries the 12-byte block at `0x14`.
[`decode_l0x_raw`](api.md#decode_l0x_raw) extracts its raw fields — what
`VL53L0X_GetRangingMeasurementData` reads before the PAL status step:

```rust
use depz_sensor_sdk::framing::{Event, PacketParser};
use depz_sensor_sdk::vl53lx::{decode_l0x_raw, StreamData, Vl53lxRpt, L0X_BLOCK_ADDR};

let mut parser = PacketParser::new();
for ev in parser.feed(&rx_bytes) {
    let Event::Packet(pkt) = ev else { continue };
    if pkt.cmd != Vl53lxRpt::Stream as u8 {
        continue;
    }
    let s = StreamData::unpack(&pkt.payload)?;
    if s.addr != L0X_BLOCK_ADDR {
        continue;
    }
    let r = decode_l0x_raw(&s.data)?;                      // ShortBlock below 12 bytes
    let signal_mcps = r.signal_rate_mcps_1616 as f64 / 65536.0;     // FixPoint16.16
    let ambient_mcps = r.ambient_rate_mcps_1616 as f64 / 65536.0;
    let spads = r.effective_spad_count_88 as f64 / 256.0;           // 8.8
    println!(
        "{} µs  {} mm  device status 0x{:02X}  signal {:.3} Mcps  ambient {:.3} Mcps  {:.1} SPADs",
        s.timestamp_us, r.distance_raw, r.device_range_status, signal_mcps, ambient_mcps, spads
    );
}
```

| field | meaning |
|---|---|
| `distance_raw` | distance, mm (quarter-mm if the driver enabled RangeFractional; off by default) |
| `device_range_status` | raw status byte 0 — **not** the PAL range status |
| `signal_rate_mcps_1616` / `ambient_rate_mcps_1616` | rates, FixPoint16.16 Mcps (the wire's 9.7 shifted left by 9) |
| `effective_spad_count_88` | effective SPAD count, 8.8 |

The PAL range status, sigma and maximum distance the driver reports need
device data cached at init (reference SPADs, calibration): the
[live device](#live-device) gives them. The family's plottable rule (statuses 0, 6, 11) applies to that
driver status, not to `device_range_status`.

## Reset and bridge diagnostics

```rust
use depz_sensor_sdk::vl53lx::{pack_xshut, Vl53lxCmd, Vl53lxInfo, Vl53lxRpt, XSHUT_RESET};

let reset = build_packet(Vl53lxCmd::Xshut as u8, &pack_xshut(XSHUT_RESET), 0, CrcType::None)?;
let get_info = build_packet(Vl53lxCmd::GetInfo as u8, &[], 0, CrcType::None)?;

// RPT_VL53_INFO (0x92), 23 bytes — bridge state only, safe while streaming.
if info_pkt.cmd == Vl53lxRpt::Info as u8 {
    let info = Vl53lxInfo::unpack(&info_pkt.payload)?;
    println!(
        "address width {} (1 on the L0X), {} kHz, {} I2C errors, {} slots skipped",
        info.addr_width, info.i2c_khz, info.i2c_errors, info.slots_skipped
    );
}
```

`XSHUT_RESET` is 1 ms low plus a fixed 5 ms wait — v2.00 has **no boot
handshake**: poll the model id (`0xC0 == 0xEE`) yourself before configuring.
The address width is 2 after a reset: set it to 1 again.

## Gotchas

- **Address width 1, every session** — after a reset the bridge is back at 2;
  a 2-byte access on the L0X reads the wrong registers.
- **Two interrupt-release writes** (`0x0B ← 1`, `0x0B ← 0`) — pass the table's
  `clear_steps`, not the die parts' single `0x0086 ← 1`.
- **Raw fields only** — `device_range_status` is not the PAL status.
- **400 kHz ceiling** — the L0X has no FM+ pad.
- **A NACK or two right after reset is normal** — the part is still booting;
  retry, and don't count those against a stream.
