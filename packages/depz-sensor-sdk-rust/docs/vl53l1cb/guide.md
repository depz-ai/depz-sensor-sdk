# VL53L1CB — user guide

Hands-on guide to the [`vl53lx`](api.md) decode layer for the VL53L1CB. For
what the sensor is and its concepts, read the [introduction](introduction.md);
for exact signatures see the [API reference](api.md).

This crate does not drive the sensor: the ST driver (initialisation, modes,
budgets, ROI, calibration, histogram target extraction) runs in the Python or
TypeScript SDK. What follows is everything you need to build the bridge
commands and decode what the board streams.

## Contents

- [Which board is it](#which-board-is-it)
- [The product table](#the-product-table)
- [Stream commands](#stream-commands)
- [Decode the die block (ULD)](#decode-the-die-block-uld)
- [Decode the histogram block](#decode-the-histogram-block)
- [Which statuses to plot](#which-statuses-to-plot)
- [Reset and bridge diagnostics](#reset-and-bridge-diagnostics)
- [Behind a cover glass](#behind-a-cover-glass)
- [Gotchas](#gotchas)

## Which board is it

Every 1D board runs the same firmware (`APP_VL53L0_4_*`, sensor type
`vl53lx`), so the firmware name cannot tell them apart.
[`resolve_class`](api.md#resolve_class) uses the production USB PID model,
else the first `VL53L<digit>…` in the device name:

```rust
use depz_sensor_sdk::usb_model_hint;
use depz_sensor_sdk::vl53lx::{product_from_board_name, resolve_class, resolve_product, Vl53lxClass};

let class = resolve_class(usb_model_hint(Some(vid), Some(pid)), &device_name);
assert_eq!(resolve_class(None, "DEPZ ToF Sensor VL53L1CB USB v2.1"), Vl53lxClass::Vl53l1cb);
assert_eq!(product_from_board_name("ToF Sensor VL53L1CB USB v2.1"), Some("VL53L1CB"));
let product = resolve_product(usb_model_hint(Some(vid), Some(pid)), &device_name);   // Some("VL53L1CB")
println!("{} / {:?}", class.as_str(), product);
```

The model id the sensor answers (`0xEACC`) is a cross-check only: the
VL53L1CX answers the same.

## The product table

[`PRODUCTS`](api.md#products) holds one [`Product`](api.md#product) per family
member; [`BusParams`](api.md#busparams) says what one product/driver pair
tells the bridge:

```rust
use depz_sensor_sdk::vl53lx::{product, DieVariant, DriverKind};

let p = product("VL53L1CB").expect("a family product");
assert_eq!((p.usb_pid, p.model_id, p.reach_mm), (0xED42, 0xEACC, 8000));
assert_eq!(p.driver_kinds(), vec![DriverKind::Uld, DriverKind::Histogram]);
assert_eq!(p.default_driver, DriverKind::Uld);

let uld = p.bus_params(DriverKind::Uld).unwrap();
assert_eq!((uld.block_addr, uld.block_len), (0x0089, 17));      // die result block
assert_eq!(uld.die_variant, Some(DieVariant::L1));              // read as the VL53L1X ULD does
assert_eq!(uld.clear_steps, &[(0x0086, 0x01)]);                 // interrupt release
assert_eq!((uld.addr_width, uld.max_khz), (2, 1000));

let hist = p.bus_params(DriverKind::Histogram).unwrap();
assert_eq!((hist.block_addr, hist.block_len), (0x0088, 83));    // histogram block
assert!(p.bus_params(DriverKind::Ulp).is_none());               // no such pair: refuse
```

## Stream commands

A stream is armed with `VL53_START_STREAM`: the block to read on every
data-ready edge and the interrupt-release writes the bridge plays after each
read (v2.00 carries them in the command). Set the register-address width
first — it is sticky, and 2 after a reset:

```rust
use depz_sensor_sdk::framing::{build_packet, CrcType};
use depz_sensor_sdk::vl53lx::{pack_set_addr_width, pack_start_stream, product, DriverKind, Vl53lxCmd};

let bus = product("VL53L1CB").unwrap().bus_params(DriverKind::Uld).unwrap();
let width = build_packet(Vl53lxCmd::SetAddrWidth as u8, &pack_set_addr_width(bus.addr_width)?, 0, CrcType::None)?;
let start = build_packet(
    Vl53lxCmd::StartStream as u8,
    &pack_start_stream(bus.block_addr, bus.block_len, bus.clear_steps, 0)?,   // ≤ 4 clear steps
    0,
    CrcType::None,
)?;
let stop = build_packet(Vl53lxCmd::StopStream as u8, &[], 0, CrcType::None)?;
```

The register commands (`pack_read_reg`, `pack_write_reg`, `pack_xshut`,
`pack_set_i2c_speed`) are the VL53L4CD codecs, re-exported from `vl53lx`.
Every init runs at 400 kHz; the driver raises the bus to `max_khz` afterwards.

## Decode the die block (ULD)

With the `uld` driver every `RPT_VL53_STREAM` report carries the 17-byte die
result block. [`decode_die_block`](api.md#decode_die_block) reads it as the
VL53L1X ULD does ([`DieVariant::L1`](api.md#dievariant): crosstalk-corrected
peak signal at `0x0098`, per-SPAD scale 25):

```rust
use depz_sensor_sdk::framing::{Event, PacketParser};
use depz_sensor_sdk::vl53lx::{decode_die_block, DieVariant, StreamData, Vl53lxRpt, DIE_BLOCK_ADDR};

let mut parser = PacketParser::new();
for ev in parser.feed(&rx_bytes) {
    let Event::Packet(pkt) = ev else { continue };
    if pkt.cmd != Vl53lxRpt::Stream as u8 {
        continue;
    }
    let s = StreamData::unpack(&pkt.payload)?;          // timestamp_us, addr, len, data
    if s.addr != DIE_BLOCK_ADDR {
        continue;
    }
    let r = decode_die_block(&s.data, DieVariant::L1)?;  // ShortBlock below 17 bytes
    println!(
        "{} µs  #{:3}  {:5} mm  status {}  sigma {} mm  signal {} kcps  ambient {} kcps  {} SPADs",
        s.timestamp_us, r.stream_count, r.distance_mm, r.range_status,
        r.sigma_mm, r.signal_rate_kcps, r.ambient_rate_kcps, r.number_of_spad
    );
}
```

[`DieResult`](api.md#dieresult) holds integers as the ULD computes them:
`distance_mm`, `sigma_mm` (raw ÷ 4), signal and ambient rates in kcps (raw ×
8), per-SPAD rates (rate × 25 ÷ SPADs), the active SPAD count and the rolling
`stream_count`. `range_status` is already mapped through the ULD status table
(0 = valid).

## Decode the histogram block

With the `histogram` driver each report carries the 83-byte block at
`0x0088`. [`decode_histogram_raw`](api.md#decode_histogram_raw) extracts the
status bytes and the 24 photon-count bins (bin 23's low byte, carried
separately, is patched in first):

```rust
use depz_sensor_sdk::vl53lx::{decode_histogram_raw, HISTOGRAM_BLOCK_ADDR, HISTOGRAM_BINS};

if s.addr == HISTOGRAM_BLOCK_ADDR {
    let h = decode_histogram_raw(&s.data)?;
    assert_eq!(h.bins.len(), HISTOGRAM_BINS);
    let (peak_bin, peak) = h.bins.iter().enumerate().max_by_key(|(_, c)| **c).unwrap();
    println!(
        "#{} range status 0x{:02X}, {} effective SPADs, VCSEL start {}, peak bin {} = {}",
        h.stream_count, h.range_status, h.dss_actual_effective_spads, h.vcsel_start, peak_bin, peak
    );
}
```

Turning bins into up to four targets (preset, VCSEL period, the A/B frame
pairs, phase history) is the full driver's job and is not in this crate. The
driver keeps state from frame to frame, so a stream fed to it must be decoded
**exactly once, in order** — replaying or skipping frames changes its output.

## Which statuses to plot

The full drivers report one family-wide status. Statuses **0, 6 and 11** are
usable: 6 is the first frame (no predecessor for the wrap check), 11 a merged
pulse. `status == 0` alone drops those frames. The die block's `range_status`
uses the same numbering, so the same rule applies to it:

```rust
/// Statuses a chart should draw (0 valid, 6 first frame, 11 merged pulse).
const PLOTTABLE: [u8; 3] = [0, 6, 11];

let r = decode_die_block(&s.data, DieVariant::L1)?;
if PLOTTABLE.contains(&r.range_status) {
    println!("{} mm", r.distance_mm);
}
```

The histogram block's `range_status` byte is the raw device register, not the
driver's target status — the rule does not apply to it.

## Reset and bridge diagnostics

```rust
use depz_sensor_sdk::vl53lx::{pack_xshut, Vl53lxCmd, Vl53lxInfo, Vl53lxRpt, XSHUT_RESET};

let reset = build_packet(Vl53lxCmd::Xshut as u8, &pack_xshut(XSHUT_RESET), 0, CrcType::None)?;
let get_info = build_packet(Vl53lxCmd::GetInfo as u8, &[], 0, CrcType::None)?;

// RPT_VL53_INFO (0x92), 23 bytes — bridge state only, safe while streaming.
if info_pkt.cmd == Vl53lxRpt::Info as u8 {
    let info = Vl53lxInfo::unpack(&info_pkt.payload)?;
    println!(
        "{} edges, {} slots skipped, {} frames dropped, {} I2C errors, {} kHz, width {}, {} clear steps",
        info.int_edges, info.slots_skipped, info.frames_dropped, info.i2c_errors,
        info.i2c_khz, info.addr_width, info.n_clear
    );
}
```

`XSHUT_RESET` is 1 ms low plus a fixed 5 ms wait — v2.00 has **no boot
handshake**: poll the boot register (`0x00E5 == 0x03`) yourself before the
first access. A power-cycled sensor holds none of its configuration or
calibration. The counters are free-running and wrap silently: watch
increments.

## Behind a cover glass

The L1CB is the module meant for a window or an enclosure, and it is the board
that usually needs a crosstalk calibration. Calibration values live in sensor
RAM and are lost on reset: the full driver measures them once, and the
application stores them and re-applies them on every run.

## Gotchas

- **Use `DieVariant::L1`** for the L1CB light driver — the `L4` variant reads
  the signal rate from a different register and scales per SPAD by 256.
- **Plot 0, 6 and 11**, not only 0.
- **Decode every histogram frame exactly once, in order** if you feed a
  target-extraction driver.
- **Set the address width before the first register access** of a session,
  and never under a running stream.
- **Same model id as the VL53L1CX** (`0xEACC`) — the board's USB PID or device
  name, not the silicon, tells them apart.
- **At most 4 interrupt-release steps** — `pack_start_stream` refuses more.
