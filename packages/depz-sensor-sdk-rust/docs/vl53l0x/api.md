# VL53L0X (ToF) — API reference

The public API for the VL53L0X (ToF) sensor. Transport, framing,
identity/discovery, firmware-container and dataset symbols shared
across sensors live in the [top-level API reference](../api.md).

One module serves the whole 1D family (one `APP_VL53L0_4`
firmware): VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX and VL53L4CX.
This page lists the whole `vl53lx` module and is the same on
every family page; the product table says what differs per board.

Generated from the `///` doc-comments in the source — edit them,
not this file.

## Contents

- **VL53L0X (ToF)**: [`PLOTTABLE_STATUSES`](#plottable_statuses), [`Vl53lxMeasurement`](#vl53lxmeasurement), [`Vl53lxProductInfo`](#vl53lxproductinfo), [`Vl53lxDevice`](#vl53lxdevice), [`Vl53l0xDevice`](#vl53l0xdevice), [`Vl53l1cxDevice`](#vl53l1cxdevice), [`Vl53l1cbDevice`](#vl53l1cbdevice), [`Vl53l3cxDevice`](#vl53l3cxdevice), [`Vl53l4cxDevice`](#vl53l4cxdevice), [`HistogramBinData`](#histogrambindata), [`Vl53lxCmd`](#vl53lxcmd), [`Vl53lxRpt`](#vl53lxrpt), [`CLEAR_STEPS_MAX`](#clear_steps_max), [`INFO_SIZE`](#info_size), [`pack_set_addr_width`](#pack_set_addr_width), [`pack_start_stream`](#pack_start_stream), [`Vl53lxInfo`](#vl53lxinfo), [`DIE_BLOCK_ADDR`](#die_block_addr), [`DIE_BLOCK_LEN`](#die_block_len), [`L0X_BLOCK_ADDR`](#l0x_block_addr), [`L0X_BLOCK_LEN`](#l0x_block_len), [`HISTOGRAM_BLOCK_ADDR`](#histogram_block_addr), [`HISTOGRAM_BLOCK_LEN`](#histogram_block_len), [`HISTOGRAM_BINS`](#histogram_bins), [`DieVariant`](#dievariant), [`ShortBlock`](#shortblock), [`DieResult`](#dieresult), [`decode_die_block`](#decode_die_block), [`L0xRaw`](#l0xraw), [`decode_l0x_raw`](#decode_l0x_raw), [`HistogramRaw`](#histogramraw), [`decode_histogram_raw`](#decode_histogram_raw), [`Vl53lxTarget`](#vl53lxtarget), [`ExtraValue`](#extravalue), [`Vl53lxRange`](#vl53lxrange), [`Thresholds`](#thresholds), [`DriverKind`](#driverkind), [`DRIVER_KINDS`](#driver_kinds), [`BusParams`](#busparams), [`Product`](#product), [`PRODUCTS`](#products), [`product`](#product), [`product_from_board_name`](#product_from_board_name), [`Vl53lxClass`](#vl53lxclass), [`resolve_product`](#resolve_product), [`resolve_class`](#resolve_class)
- **Contract-10 codecs (re-exported from `vl53l4`)**: [`pack_read_reg`](#pack_read_reg), [`pack_write_reg`](#pack_write_reg), [`pack_xshut`](#pack_xshut), [`pack_set_i2c_speed`](#pack_set_i2c_speed), [`RegData`](#regdata), [`StreamData`](#streamdata), [`i2c_error_name`](#i2c_error_name), [`XFER_MAX`](#xfer_max), [`XSHUT_OFF`](#xshut_off), [`XSHUT_ON`](#xshut_on), [`XSHUT_RESET`](#xshut_reset), [`SF_INT_ACT_HIGH`](#sf_int_act_high), [`I2C_KHZ_STEPS`](#i2c_khz_steps)

## VL53L0X (ToF)

### PLOTTABLE_STATUSES

```rust
pub const PLOTTABLE_STATUSES: [i32; 3] = [0, 6, 11];
```

Statuses that mean "this distance is real": a histogram product reports 6 on the first frame of a
stream and 11 for two merged targets. Prefer [`Vl53lxMeasurement::plottable`] over
[`Vl53lxMeasurement::valid`] there.

### Vl53lxMeasurement

```rust
#[non_exhaustive]
pub struct Vl53lxMeasurement {
    pub timestamp_us: u64,
    pub distance_mm: i32,
    pub status: i32,
    pub status_text: String,
    pub signal_kcps: f64,
    pub ambient_kcps: f64,
    pub sigma_mm: f64,
    pub spads: f64,
    pub targets: Vec<Vl53lxTarget>,
    pub extra: BTreeMap<String, ExtraValue>,
    pub bins: Option<Arc<HistogramBinData>>,
}
```

One ranging result of the 1D family, the same shape for every product. On the histogram driver
`targets` holds every return, strongest first, the top level repeats the first and `bins` carries
the raw 24-bin histogram; the light drivers leave both empty.

#### Vl53lxMeasurement::valid

```rust
pub fn valid(&self) -> bool
```

#### Vl53lxMeasurement::plottable

```rust
pub fn plottable(&self) -> bool
```

#### Vl53lxMeasurement::plot_distances

```rust
pub fn plot_distances(&self) -> Vec<i32>
```

The distances a chart draws: the plottable targets in driver order, or the single distance.

#### Vl53lxMeasurement::primary_target

```rust
pub fn primary_target(&self) -> Option<&Vl53lxTarget>
```

The one target a single-number readout shows, or `None`.

### Vl53lxProductInfo

```rust
#[non_exhaustive]
pub struct Vl53lxProductInfo {
    /// The board's device name.
    pub board: String,
    /// The product the device name carries, or `None` on an unstamped board.
    pub detected: Option<&'static str>,
    /// The product [`Vl53lxDevice::init`] bound.
    pub product: &'static str,
    pub driver: &'static str,
    /// The driver's own name ("Vl53l1UldDriver", …).
    pub driver_class: &'static str,
    /// The driver kinds the bound product has.
    pub kinds: Vec<&'static str>,
    pub model_id: u16,
    /// A cross-check only: L1CX/L1CB and L4CD/L4CX share their ids.
    pub model_id_ok: bool,
    pub supports: Vec<&'static str>,
    pub modes: Vec<&'static str>,
    /// The board's rated reach, mm.
    pub reach_mm: Option<u32>,
    /// How far the loaded configuration can measure, mm.
    pub driver_reach_mm: Option<i32>,
    pub budget_ms: (u32, u32),
    pub budget_choices: Vec<u32>,
    pub histogram: bool,
    pub max_khz: u16,
    pub caveat: Option<&'static str>,
}
```

Everything about what is connected ([`Vl53lxDevice::identify_product`]).

### Vl53lxDevice

```rust
pub struct Vl53lxDevice {
    dev: DepzDevice,
    fixed_product: Option<&'static str>,
    streams: Arc<Streams>,
    platform: Arc<BridgePlatform>,
    bound: Arc<Mutex<Option<Bound>>>,
    board_name: Mutex<Option<String>>,
    ranging: AtomicBool,
}
```

A board of the VL53L 1D family on the `APP_VL53L0_4` register bridge (contract 12): VL53L0X,
VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CD, VL53L4CX. Every ULD runs here; two axes pick how a board
is driven — the product (whose parameter set: the board's own, or a sibling's to borrow its
driver) and the driver kind (`uld`, `ulp`, `histogram`). [`Vl53lxDevice::init`] binds the pair,
[`Vl53lxDevice::configure`] re-initialises and applies a ranging configuration (before every
run), then [`Vl53lxDevice::start_ranging`] and read [`Vl53lxDevice::measurements`]. Ask
[`Vl53lxDevice::supports`] before an optional group; a missing one fails with
[`DepzError::Unsupported`]. This generic class reads the product from the board's name; the
per-product classes ([`Vl53l0xDevice`], …) fix it. Configuration while ranging fails with
[`DepzError::InvalidState`] — the stream owns the bus.

#### Vl53lxDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53lxDevice
```

Continue the open connection `dev` as a 1D-family board; the product comes from its name.

#### Vl53lxDevice::board_name

```rust
pub fn board_name(&self) -> Result<String>
```

The board's device name, e.g. `DEPZ ToF Sensor VL53L4CX USB v2.1 TEXAMPLE03` (asked once).

#### Vl53lxDevice::detected

```rust
pub fn detected(&self) -> Result<Option<&'static str>>
```

The product the device name carries, or `None` on an unstamped board.

#### Vl53lxDevice::product

```rust
pub fn product(&self) -> Option<&'static str>
```

The product [`Vl53lxDevice::init`] bound (`None` before).

#### Vl53lxDevice::driver_kind

```rust
pub fn driver_kind(&self) -> Option<&'static str>
```

#### Vl53lxDevice::caveat

```rust
pub fn caveat(&self) -> Option<&'static str>
```

#### Vl53lxDevice::with_driver

```rust
pub fn with_driver<R>(&self, f: impl FnOnce(&mut dyn Vl53lxDriver) -> Result<R>) -> Result<R>
```

The ULD port in use — escape hatch for product-specific calls: `f` runs with the driver (do
not call this device's driver-backed methods from `f`: the driver is held).
[`DepzError::InvalidState`] before [`Vl53lxDevice::init`].

unstable — not covered by the 1.0 freeze.

#### Vl53lxDevice::initialized

```rust
pub fn initialized(&self) -> bool
```

#### Vl53lxDevice::ranging

```rust
pub fn ranging(&self) -> bool
```

#### Vl53lxDevice::driver_kinds

```rust
pub fn driver_kinds(&self, product: Option<&str>) -> Result<Vec<&'static str>>
```

The driver kinds a product has (`None`: this board's; empty when it carries no product).

#### Vl53lxDevice::init

```rust
pub fn init(&self, driver: Option<&str>, product: Option<&str>) -> Result<()>
```

Bind the (product, driver kind) pair and initialise the sensor. `product` `None`: this
class's, else the board name's; `driver` `None`: the product's first kind. A pair the table
lacks fails with [`DepzError::Unsupported`] naming what the product has.

#### Vl53lxDevice::identify_product

```rust
pub fn identify_product(&self) -> Result<Vl53lxProductInfo>
```

Everything about what is connected (needs [`Vl53lxDevice::init`]).

#### Vl53lxDevice::notes

```rust
pub fn notes(&self) -> Result<Vec<String>>
```

The lines a UI shows about the chosen pair: a product named by hand, a borrowed driver, a pair
that reaches less than the board is rated for, the driver's caveat.

#### Vl53lxDevice::supports

```rust
pub fn supports(&self, group: &str) -> Result<bool>
```

Whether this product / driver serves an optional group.

#### Vl53lxDevice::modes

```rust
pub fn modes(&self) -> Result<Vec<&'static str>>
```

Named ranging modes, the first being what init leaves; empty if none.

#### Vl53lxDevice::budget_range

```rust
pub fn budget_range(&self) -> Result<(u32, u32)>
```

The (min, max) budget the driver offers, ms.

#### Vl53lxDevice::driver_reach_mm

```rust
pub fn driver_reach_mm(&self) -> Result<Option<i32>>
```

How far the loaded configuration can measure, mm, or `None`.

#### Vl53lxDevice::xshut

```rust
pub fn xshut(&self, action: u8) -> Result<()>
```

Drive XSHUT: off (0) / on (1) / reset (2). Off and reset stop the stream; the sensor then holds
none of the configuration — [`Vl53lxDevice::init`] again.

#### Vl53lxDevice::bridge_info

```rust
pub fn bridge_info(&self) -> Result<Vl53lxInfo>
```

RPT_VL53_INFO — the bridge's own counters and settings; safe while streaming.

#### Vl53lxDevice::configure

```rust
pub fn configure(
    &self,
    budget_ms: u32,
    inter_ms: u32,
    mode: Option<&str>,
    offset_mm: Option<i32>,
    xtalk_kcps: Option<i32>,
    signal_kcps: Option<u32>,
) -> Result<()>
```

Re-initialise the sensor and apply a ranging configuration. The re-init is deliberate: it is
the only way to know what the configuration registers hold. `mode` goes on before the budget;
`offset_mm` / `xtalk_kcps` re-apply a stored calibration; `signal_kcps` replaces the blob's
signal threshold (each `None`: leave it). `inter_ms` 0 = continuous. An unsupported group is
refused before the re-init.

#### Vl53lxDevice::get_range_timing

```rust
pub fn get_range_timing(&self) -> Result<(u32, u32)>
```

(timing budget ms, inter-measurement ms) read back from the sensor; 0 = continuous.

#### Vl53lxDevice::get_mode

```rust
pub fn get_mode(&self) -> Result<Option<String>>
```

The mode in use, or `None` on a product without modes.

#### Vl53lxDevice::budget_choices

```rust
pub fn budget_choices(&self) -> Result<Vec<u32>>
```

The only budgets accepted right now, or empty when any in [`Vl53lxDevice::budget_range`] will do.

#### Vl53lxDevice::snap_budget

```rust
pub fn snap_budget(&self, budget_ms: u32) -> Result<u32>
```

The nearest budget this product accepts.

#### Vl53lxDevice::get_offset_mm

```rust
pub fn get_offset_mm(&self) -> Result<i32>
```

#### Vl53lxDevice::set_offset_mm

```rust
pub fn set_offset_mm(&self, offset_mm: i32) -> Result<()>
```

#### Vl53lxDevice::get_xtalk_kcps

```rust
pub fn get_xtalk_kcps(&self) -> Result<i32>
```

#### Vl53lxDevice::set_xtalk_kcps

```rust
pub fn set_xtalk_kcps(&self, xtalk_kcps: i32) -> Result<()>
```

#### Vl53lxDevice::calibrate_offset

```rust
pub fn calibrate_offset(&self, target_dist_mm: i32, nb_samples: Option<u32>) -> Result<i32>
```

Offset calibration against a flat target; returns the offset programmed (store it on the host).
`nb_samples` `None`: the driver's default.

#### Vl53lxDevice::calibrate_xtalk

```rust
pub fn calibrate_xtalk(&self, target_dist_mm: i32, nb_samples: Option<u32>) -> Result<i32>
```

Crosstalk calibration; returns the xtalk programmed, kcps (store it on the host).

#### Vl53lxDevice::get_detection_thresholds

```rust
pub fn get_detection_thresholds(&self) -> Result<Thresholds>
```

#### Vl53lxDevice::set_detection_thresholds

```rust
pub fn set_detection_thresholds(
    &self,
    distance_low_mm: u16,
    distance_high_mm: u16,
    window: u8,
) -> Result<()>
```

The distance-window interrupt; stays armed until the next init / configure.

#### Vl53lxDevice::get_signal_threshold_kcps

```rust
pub fn get_signal_threshold_kcps(&self) -> Result<u32>
```

#### Vl53lxDevice::set_signal_threshold_kcps

```rust
pub fn set_signal_threshold_kcps(&self, signal_kcps: u32) -> Result<()>
```

#### Vl53lxDevice::get_sigma_threshold_mm

```rust
pub fn get_sigma_threshold_mm(&self) -> Result<u32>
```

#### Vl53lxDevice::set_sigma_threshold_mm

```rust
pub fn set_sigma_threshold_mm(&self, sigma_mm: u32) -> Result<()>
```

#### Vl53lxDevice::get_roi

```rust
pub fn get_roi(&self) -> Result<(u8, u8)>
```

(x, y) SPADs.

#### Vl53lxDevice::set_roi

```rust
pub fn set_roi(&self, x: u8, y: u8) -> Result<()>
```

#### Vl53lxDevice::get_roi_center

```rust
pub fn get_roi_center(&self) -> Result<u8>
```

#### Vl53lxDevice::set_roi_center

```rust
pub fn set_roi_center(&self, center_spad: u8) -> Result<()>
```

#### Vl53lxDevice::start_temperature_update

```rust
pub fn start_temperature_update(&self) -> Result<()>
```

Re-run VHV after an ambient change over 8 °C.

#### Vl53lxDevice::perform_ref_spad_management

```rust
pub fn perform_ref_spad_management(&self) -> Result<(u32, bool)>
```

VL53L0X: re-measure the reference SPADs → (count, is_aperture).

#### Vl53lxDevice::start_ranging

```rust
pub fn start_ranging(&self) -> Result<()>
```

Start the sensor and arm the board's stream: one report per INT edge, followed on the MCU by
the driver's interrupt-release writes.

#### Vl53lxDevice::stop_ranging

```rust
pub fn stop_ranging(&self) -> Result<()>
```

#### Vl53lxDevice::measure_once

```rust
pub fn measure_once(&self, timeout: Option<Duration>) -> Result<Vl53lxMeasurement>
```

One polled measurement: start, data ready, read, release the interrupt, stop. Not while
streaming. `timeout` `None`: 1 s.

#### Vl53lxDevice::measurements

```rust
pub fn measurements(&self) -> &Stream<Vl53lxMeasurement>
```

The measurement stream (contract 07 §3): `measurements().on(cb)` calls back on the reader
thread (or the thread of a call that held the driver while the frame came in),
`measurements().subscribe(capacity)` is a bounded drop-oldest queue subscribed at the call
— before or after `start_ranging()` —, `measurements().next(timeout)` waits for one;
`dropped_counts()` and `parse_errors()` (a report the driver did not decode, or one
shorter than its header, before `init()` too) are its counters. A callback may take the
driver for what it knows locally (`supports`, `modes`, `budget_range`, …); a bus request
(`get_range_timing`, `identify_product`, …) from a callback on the reader thread times
out — the reader that would deliver its answer is in the callback.

### Vl53l0xDevice

```rust
pub struct Vl53l0xDevice(Vl53lxDevice);
```

VL53L0X (2 m, 1-byte register addresses, 400 kHz): ULD API 1.0.4 with ranging profiles.

#### Vl53l0xDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l0xDevice
```

Continue the open connection `dev` as a VL53L0X.

### Vl53l1cxDevice

```rust
pub struct Vl53l1cxDevice(Vl53lxDevice);
```

VL53L1CX (4 m): the VL53L1X ULD (short / long) or the histogram driver.

#### Vl53l1cxDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l1cxDevice
```

Continue the open connection `dev` as a VL53L1CX.

### Vl53l1cbDevice

```rust
pub struct Vl53l1cbDevice(Vl53lxDevice);
```

VL53L1CB (8 m, cover-glass module): the VL53L1CX die and drivers; usually needs a crosstalk
calibration.

#### Vl53l1cbDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l1cbDevice
```

Continue the open connection `dev` as a VL53L1CB.

### Vl53l3cxDevice

```rust
pub struct Vl53l3cxDevice(Vl53lxDevice);
```

VL53L3CX (3 m): ST's ULP driver (single target) or the histogram driver (several per frame).

#### Vl53l3cxDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l3cxDevice
```

Continue the open connection `dev` as a VL53L3CX.

### Vl53l4cxDevice

```rust
pub struct Vl53l4cxDevice(Vl53lxDevice);
```

VL53L4CX (6 m): the histogram driver only — to run it light, borrow a sibling's with
`init(Some("uld"), Some("VL53L4CD"))`.

#### Vl53l4cxDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l4cxDevice
```

Continue the open connection `dev` as a VL53L4CX.

### HistogramBinData

```rust
#[non_exhaustive]
pub struct HistogramBinData {
    /// `result__interrupt_status`.
    pub result_interrupt_status: i64,
    /// `result__range_status`.
    pub result_range_status: i64,
    /// `result__report_status`.
    pub result_report_status: i64,
    /// `result__stream_count`: the device's frame counter.
    pub result_stream_count: i64,
    /// `result__dss_actual_effective_spads`, 1/256 SPAD.
    pub result_dss_actual_effective_spads: i64,
    /// `phasecal_result__reference_phase`.
    pub phasecal_result_reference_phase: i64,
    /// `phasecal_result__vcsel_start`.
    pub phasecal_result_vcsel_start: i64,
    /// The photon counts (always [`regs::HISTOGRAM_BUFFER_SIZE`] long; `bins_in_data` are in use).
    pub bin_data: Vec<i64>,
    /// `zone_id`.
    pub zone_id: i64,
    /// `VL53LX_p_019`: the first bin.
    pub first_bin: i64,
    /// `VL53LX_p_020`: bins in the buffer.
    pub number_of_bins: i64,
    /// `VL53LX_p_021`: bins in use.
    pub bins_in_data: i64,
    /// `cal_config__vcsel_start`.
    pub cal_config_vcsel_start: i64,
    /// The VCSEL pulse width.
    pub vcsel_width: i64,
    /// `VL53LX_p_015`: the fast oscillator frequency.
    pub fast_osc_frequency: i64,
    /// `VL53LX_p_005`: the VCSEL period register value the frame ran at.
    pub vcsel_period: i64,
    /// The bin-sequence codes.
    pub bin_seq: [i64; 6],
    /// How often each code was sampled.
    pub bin_rep: [i64; 6],
    /// The smallest bin.
    pub min_bin_value: i64,
    /// The largest bin.
    pub max_bin_value: i64,
    /// Ambient-only bins at the front.
    pub number_of_ambient_bins: i64,
    /// Samples the ambient estimate is over.
    pub number_of_ambient_samples: i64,
    /// Their sum.
    pub ambient_events_sum: i64,
    /// `VL53LX_p_028`: ambient events per bin.
    pub ambient_per_bin: i64,
    /// Macro periods the frame ran for.
    pub total_periods_elapsed: i64,
    /// Peak window duration, µs.
    pub peak_duration_us: i64,
    /// Window-of-interest duration, µs.
    pub woi_duration_us: i64,
    /// The phase of zero distance.
    pub zero_distance_phase: i64,
    /// `roi_config__user_roi_centre_spad`.
    pub roi_config_user_roi_centre_spad: i64,
    /// `roi_config__user_roi_requested_global_xy_size`.
    pub roi_config_user_roi_requested_global_xy_size: i64,
}
```

`VL53LX_histogram_bin_data_t`: 24 bins plus everything the post-processing needs to read them —
the VCSEL period they were taken at, the phase of zero distance, the bin sequence and the ambient
estimate. Field names are the reference's (the C names, `__` folded to `_`);
[`HistogramBinData::fields`] gives them under the exact reference keys.

#### HistogramBinData::range_status

```rust
pub fn range_status(&self) -> i64
```

The device's range status (`result__range_status` masked).

#### HistogramBinData::fields

```rust
pub fn fields(&self) -> Vec<(&'static str, Value)>
```

The fields under the C driver's (and the Python SDK's) names, in the reference's order.

#### HistogramBinData::set_field

```rust
pub fn set_field(&mut self, key: &str, value: &Value) -> bool
```

Set one field by its reference key (the inverse of [`HistogramBinData::fields`]); `false` when
the key is unknown or the value has the wrong shape.

### Vl53lxCmd

```rust
#[non_exhaustive]
pub enum Vl53lxCmd {
    ReadReg = 0x32,
    WriteReg = 0x33,
    Xshut = 0x34,
    StartStream = 0x35,
    StopStream = 0x36,
    GetInfo = 0x37,
    SetI2cSpeed = 0x38,
    /// New in v2.00: register-address width, 1 or 2 bytes (sticky, 2 after a
    /// reset). Set it before the first register access of a session and never
    /// under a running stream.
    SetAddrWidth = 0x39,
    /// New in v2.01 (firmware v0.24), no payload: zero `i2c_errors` /
    /// `last_i2c_error`. The host sends it after every sensor init — a
    /// resetting die NACKs for a moment, and that is no bus fault.
    ClearI2cErrors = 0x3A,
}
```

VL53LX host→device command opcodes.

### Vl53lxRpt

```rust
#[non_exhaustive]
pub enum Vl53lxRpt {
    RegData = 0x91,
    Info = 0x92,
    Stream = 0x93,
}
```

VL53LX device→host report opcodes.

### CLEAR_STEPS_MAX

```rust
pub const CLEAR_STEPS_MAX: usize = 4;
```

Interrupt-release steps a stream may carry (`VL53_CLEAR_STEPS_WIRE_MAX`).

### INFO_SIZE

```rust
pub const INFO_SIZE: usize = 23;
```

`RPT_VL53_INFO` payload size (v2.00).

### pack_set_addr_width

```rust
pub fn pack_set_addr_width(width: u8) -> Result<[u8; 1], CodecError>
```

`VL53_SET_ADDR_WIDTH` payload: `width u8` — 1 (VL53L0X) or 2 (the rest).

### pack_start_stream

```rust
pub fn pack_start_stream(
    addr: u16,
    len: u16,
    clear: &[(u16, u8)],
    flags: u8,
) -> Result<Vec<u8>, CodecError>
```

`VL53_START_STREAM` payload (v2.00): `addr u16, len u16, flags u8,
n_clear u8, clear[n] {addr u16, value u8}` — `6 + 3n` bytes. `clear` is the
interrupt-release list the bridge plays after every block read (0..4
steps); more than [`CLEAR_STEPS_MAX`] is refused.

### Vl53lxInfo

```rust
#[non_exhaustive]
pub struct Vl53lxInfo {
    pub int_edges: u32,
    pub slots_skipped: u32,
    pub i2c_errors: u32,
    /// 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR
    /// ([`crate::vl53l4::i2c_error_name`]).
    pub last_i2c_error: u8,
    pub xshut_level: u8,
    pub int_level: u8,
    pub i2c_khz: u16,
    pub addr_width: u8,
    pub n_clear: u8,
    pub frames_dropped: u32,
}
```

A decoded `RPT_VL53_INFO` report (v2.00, 23 bytes) — bridge state only; the
bridge reads no sensor register. Counters are free-running and wrap
silently: watch increments. `slots_skipped` = a slot that never got the
bus, `i2c_errors` = a bus that answered badly, `frames_dropped` = a good
sample the USB TX ring had no room for (since the stream was armed).

#### Vl53lxInfo::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<Vl53lxInfo, CodecError>
```

### DIE_BLOCK_ADDR

```rust
pub const DIE_BLOCK_ADDR: u16 = 0x0089;
```

Die result block address (`RESULT__RANGE_STATUS`).

### DIE_BLOCK_LEN

```rust
pub const DIE_BLOCK_LEN: usize = 17;
```

Die result block length.

### L0X_BLOCK_ADDR

```rust
pub const L0X_BLOCK_ADDR: u16 = 0x14;
```

VL53L0X result block address (8-bit register space).

### L0X_BLOCK_LEN

```rust
pub const L0X_BLOCK_LEN: usize = 12;
```

VL53L0X result block length.

### HISTOGRAM_BLOCK_ADDR

```rust
pub const HISTOGRAM_BLOCK_ADDR: u16 = 0x0088;
```

Histogram block address (`result__interrupt_status`).

### HISTOGRAM_BLOCK_LEN

```rust
pub const HISTOGRAM_BLOCK_LEN: usize = 83;
```

Histogram block length (0x0088..=0x00DA).

### HISTOGRAM_BINS

```rust
pub const HISTOGRAM_BINS: usize = 24;
```

Photon bins per histogram block.

### DieVariant

```rust
pub enum DieVariant {
    /// VL53L4CD ULD — also the L3CX ULP and L4CX-as-L4CD: `(5, 256)`.
    L4,
    /// VL53L1X ULD — crosstalk-corrected peak signal at 0x0098: `(15, 25)`.
    L1,
}
```

Which ULD reads the die block: `(signal-rate byte offset, per-SPAD K)`.

#### DieVariant::as_str

```rust
pub fn as_str(&self) -> &'static str
```

Lowercase name (`"l4"` / `"l1"`, matches the golden vectors).

#### DieVariant::from_name

```rust
pub fn from_name(s: &str) -> Option<DieVariant>
```

#### DieVariant::params

```rust
pub fn params(&self) -> (usize, u32)
```

`(signal-rate byte offset, per-SPAD scale K)`.

### ShortBlock

```rust
#[non_exhaustive]
pub struct ShortBlock {
    pub need: usize,
    pub got: usize,
}
```

A block shorter than its decoder needs.

### DieResult

```rust
#[non_exhaustive]
pub struct DieResult {
    /// ULD status via `STATUS_RTN` (0 = valid).
    pub range_status: u8,
    pub distance_mm: u32,
    pub sigma_mm: u32,
    pub signal_rate_kcps: u32,
    pub ambient_rate_kcps: u32,
    pub signal_per_spad_kcps: u32,
    pub ambient_per_spad_kcps: u32,
    pub number_of_spad: u32,
    pub stream_count: u8,
}
```

The die block decoded as a ULD reads it.

### decode_die_block

```rust
pub fn decode_die_block(raw: &[u8], variant: DieVariant) -> Result<DieResult, ShortBlock>
```

The 17-byte die block (0x0089..0x0099) as the named ULD reads it.

### L0xRaw

```rust
#[non_exhaustive]
pub struct L0xRaw {
    /// mm (quarter-mm when RangeFractionalEnable, off by default).
    pub distance_raw: u32,
    /// Raw byte 0; the PAL status needs the init state.
    pub device_range_status: u8,
    /// FixPoint16.16 Mcps (9.7 on the wire `<< 9`).
    pub signal_rate_mcps_1616: u32,
    pub ambient_rate_mcps_1616: u32,
    /// 8.8.
    pub effective_spad_count_88: u32,
}
```

Raw fields of the VL53L0X result block.

### decode_l0x_raw

```rust
pub fn decode_l0x_raw(raw: &[u8]) -> Result<L0xRaw, ShortBlock>
```

Raw fields of the VL53L0X block at 0x14 (`VL53L0X_GetRangingMeasurementData`
before the PAL status/sigma step).

### HistogramRaw

```rust
#[non_exhaustive]
pub struct HistogramRaw {
    pub interrupt_status: u8,
    pub range_status: u8,
    pub report_status: u8,
    pub stream_count: u8,
    pub dss_actual_effective_spads: u32,
    pub reference_phase: u32,
    pub vcsel_start: u8,
    /// 24 photon counts.
    pub bins: [u32; HISTOGRAM_BINS],
}
```

Status bytes and bins of the histogram block.

### decode_histogram_raw

```rust
pub fn decode_histogram_raw(raw: &[u8]) -> Result<HistogramRaw, ShortBlock>
```

The 83-byte histogram block at 0x0088: status bytes and the 24 bins
(bin 23's low byte is carried in a separate MSB/LSB pair and patched in as
`(MSB << 2) + LSB` before the bins are read).

### Vl53lxTarget

```rust
#[non_exhaustive]
pub struct Vl53lxTarget {
    pub distance_mm: i32,
    pub status: i32,
    pub status_text: String,
    pub signal_kcps: f64,
    pub ambient_kcps: f64,
    pub sigma_mm: f64,
    pub min_range_mm: i32,
    pub max_range_mm: i32,
}
```

One return of a multi-target frame. `min_range_mm` / `max_range_mm` are the edges of the target's
own pulse, which only the histogram driver knows.

### ExtraValue

```rust
#[non_exhaustive]
pub enum ExtraValue {
    Int(i64),
    Float(f64),
}
```

A value of [`Vl53lxRange::extra`] / [`Vl53lxDriver::driver_info`]: driver-specific numbers.

### Vl53lxRange

```rust
#[non_exhaustive]
pub struct Vl53lxRange {
    pub distance_mm: i32,
    pub status: i32,
    pub status_text: String,
    pub signal_kcps: f64,
    pub ambient_kcps: f64,
    pub sigma_mm: f64,
    pub spads: f64,
    pub targets: Vec<Vl53lxTarget>,
    pub extra: BTreeMap<String, ExtraValue>,
}
```

One range result as a driver reports it, the same shape for every product of the family. On the
histogram driver `targets` holds every return and the top level repeats the first; the light
drivers leave it empty. `extra` is driver-specific (keys as the reference SDK spells them) — show
it, don't branch on it.

#### Vl53lxRange::valid

```rust
pub fn valid(&self) -> bool
```

### Thresholds

```rust
#[non_exhaustive]
pub struct Thresholds {
    pub distance_low_mm: u16,
    pub distance_high_mm: u16,
    pub window: u8,
}
```

The distance-window interrupt: `window` is one of the `WINDOW_*` constants of the die.

#### Thresholds::new

```rust
pub fn new(distance_low_mm: u16, distance_high_mm: u16, window: u8) -> Thresholds
```

A window from its three fields.

### DriverKind

```rust
#[non_exhaustive]
pub enum DriverKind {
    /// Ultra Lite Driver — the die computes the distance.
    Uld,
    /// Ultra Low Power (VL53L3CX only).
    Ulp,
    /// ST's Bare Driver — the die hands over 24 photon bins, the host finds
    /// up to four targets.
    Histogram,
}
```

The three kinds of driver ST ships for this family, in UI order.

#### DriverKind::as_str

```rust
pub fn as_str(&self) -> &'static str
```

Lowercase wire-string form (matches the golden vectors).

#### DriverKind::from_name

```rust
pub fn from_name(s: &str) -> Option<DriverKind>
```

### DRIVER_KINDS

```rust
pub const DRIVER_KINDS: [DriverKind; 3] = [DriverKind::Uld, DriverKind::Ulp, DriverKind::Histogram];
```

Every driver kind, in UI order.

### BusParams

```rust
#[non_exhaustive]
pub struct BusParams {
    /// `VL53_SET_ADDR_WIDTH` value (1 on VL53L0X, else 2).
    pub addr_width: u8,
    /// Interrupt-release steps handed to `VL53_START_STREAM`.
    pub clear_steps: &'static [(u16, u8)],
    /// Bus ceiling the driver raises the bridge to after the 400 kHz init.
    pub max_khz: u16,
    /// Streamed block address and length.
    pub block_addr: u16,
    pub block_len: u16,
    /// How the die block is read (`None` for the L0X and histogram blocks).
    pub die_variant: Option<DieVariant>,
}
```

The bridge parameters one product/driver pair runs with (contract 12 §1, §3).

### Product

```rust
#[non_exhaustive]
pub struct Product {
    /// Part number, e.g. `"VL53L4CD"`.
    pub name: &'static str,
    /// Production USB PID.
    pub usb_pid: u16,
    /// Expected model id (cross-check only).
    pub model_id: u16,
    /// Datasheet rated reach, mm.
    pub reach_mm: u32,
    /// The driver kind a board of this product opens with.
    pub default_driver: DriverKind,
    drivers: &'static [(DriverKind, BusParams)],
}
```

One row of the product table.

#### Product::driver_kinds

```rust
pub fn driver_kinds(&self) -> Vec<DriverKind>
```

The driver kinds this product has, in [`DRIVER_KINDS`] order.

#### Product::bus_params

```rust
pub fn bus_params(&self, kind: DriverKind) -> Option<BusParams>
```

Bridge parameters of one product/driver pair; `None` for a pair the
table has no row for (a refusal, never a fallback).

#### Product::default_bus_params

```rust
pub fn default_bus_params(&self) -> BusParams
```

Bridge parameters of [`Product::default_driver`].

#### Product::model_id_ok

```rust
pub fn model_id_ok(&self, value: u16) -> bool
```

Cross-check a model id the sensor answered. `true` only says "not
something else entirely": the pairs that share an id can't be told apart.

### PRODUCTS

```rust
pub const PRODUCTS: [Product; 6] = [
    Product {
        name: "VL53L0X",
        usb_pid: 0xED41,
        model_id: 0x00EE,
        reach_mm: 2000,
        default_driver: DriverKind::Uld,
        drivers: &[(DriverKind::Uld, L0X_ULD)],
    },
    Product {
        name: "VL53L1CX",
        usb_pid: 0xED43,
        model_id: 0xEACC,
        reach_mm: 4000,
        default_driver: DriverKind::Uld,
        drivers: &[
            (DriverKind::Uld, die(DieVariant::L1)),
            (DriverKind::Histogram, HISTOGRAM),
        ],
    },
    Product {
        name: "VL53L1CB",
        usb_pid: 0xED42,
        model_id: 0xEACC,
        reach_mm: 8000,
        default_driver: DriverKind::Uld,
        drivers: &[
            (DriverKind::Uld, die(DieVariant::L1)),
            (DriverKind::Histogram, HISTOGRAM),
        ],
    },
    Product {
        name: "VL53L3CX",
        usb_pid: 0xED44,
        model_id: 0xEAAA,
        reach_mm: 3000,
        default_driver: DriverKind::Ulp,
        drivers: &[
            (DriverKind::Ulp, die(DieVariant::L4)),
            (DriverKind::Histogram, HISTOGRAM),
        ],
    },
    Product {
        name: "VL53L4CD",
        usb_pid: 0xED45,
        model_id: 0xEBAA,
        reach_mm: 1300,
        default_driver: DriverKind::Uld,
        drivers: &[
            (DriverKind::Uld, die(DieVariant::L4)),
            (DriverKind::Histogram, HISTOGRAM),
        ],
    },
    Product {
        name: "VL53L4CX",
        usb_pid: 0xED46,
        model_id: 0xEBAA,
        reach_mm: 6000,
        default_driver: DriverKind::Histogram,
        drivers: &[(DriverKind::Histogram, HISTOGRAM)],
    },
];
```

Every product of the family, in UI order.

### product

```rust
pub fn product(name: &str) -> Option<&'static Product>
```

Table row by part number (case-sensitive, uppercase), `None` if unserved.

### product_from_board_name

```rust
pub fn product_from_board_name(name: &str) -> Option<&'static str>
```

`"ToF Sensor VL53L4CD USB v2.1"` → `"VL53L4CD"`. Matches the first
`VL53L<digit>[A-Z0-9]*` in the upper-cased name; `None` when there is none
or it is not a family product (an unstamped board, or an unknown name).

### Vl53lxClass

```rust
#[non_exhaustive]
pub enum Vl53lxClass {
    Vl53l0x,
    Vl53l1cx,
    Vl53l1cb,
    Vl53l3cx,
    Vl53l4cx,
    /// The generic class: takes the product at init (VL53L4CD boards on this
    /// firmware land here too — the `vl53l4cd` class belongs to `APP_VL53L4`).
    Vl53lx,
}
```

The sensor class an `APP_VL53L0_4` board opens as.

#### Vl53lxClass::as_str

```rust
pub fn as_str(&self) -> &'static str
```

Class name (matches the golden vectors).

### resolve_product

```rust
pub fn resolve_product(usb_model: Option<&str>, device_name: &str) -> Option<&'static str>
```

The product a board carries: the production USB PID model (`usb_model`,
e.g. from [`crate::usb_model_hint`]) when it names a family product, else
the device-name product ([`product_from_board_name`]), else `None`.

### resolve_class

```rust
pub fn resolve_class(usb_model: Option<&str>, device_name: &str) -> Vl53lxClass
```

Resolve the 1D-family class (contract 12 §1, pinned by `vl53lx.json`
`model`): PID model if it is a family product, else the device-name
product, else the generic [`Vl53lxClass::Vl53lx`].

## Contract-10 codecs (re-exported from `vl53l4`)

### pack_read_reg

```rust
pub fn pack_read_reg(addr: u16, len: u16) -> [u8; 4]
```

`VL53_READ_REG` payload: `addr u16, len u16` (little-endian), unchecked —
prefer [`try_pack_read_reg`].

### pack_write_reg

```rust
pub fn pack_write_reg(addr: u16, data: &[u8]) -> Vec<u8>
```

`VL53_WRITE_REG` payload: `addr u16` followed by the raw register bytes,
unchecked — prefer [`try_pack_write_reg`].

### pack_xshut

```rust
pub fn pack_xshut(action: u8) -> [u8; 1]
```

`VL53_XSHUT` payload: `action u8` ([`XSHUT_OFF`] / [`XSHUT_ON`] /
[`XSHUT_RESET`]).

### pack_set_i2c_speed

```rust
pub fn pack_set_i2c_speed(khz: u16) -> [u8; 2]
```

`VL53_SET_I2C_SPEED` payload: `khz u16`.

### RegData

```rust
#[non_exhaustive]
pub struct RegData {
    /// Echoed `VL53_READ_REG` opcode (0x32).
    pub cmd: u8,
    /// MCU uptime at I2C-read completion.
    pub timestamp_us: u64,
    /// Raw register bytes (big-endian sensor contents, passed through).
    pub data: Vec<u8>,
}
```

A decoded `RPT_VL53_REG_DATA` report (one register read).

#### RegData::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<RegData, CodecError>
```

### StreamData

```rust
#[non_exhaustive]
pub struct StreamData {
    /// MCU uptime at the INT edge (the sensor event, not the I2C completion).
    pub timestamp_us: u64,
    pub addr: u16,
    pub len: u16,
    /// Raw register bytes (big-endian sensor contents, passed through).
    pub data: Vec<u8>,
}
```

A decoded `RPT_VL53_STREAM` report — one streamed register block.
`addr`/`len` echo the stream configuration so each report is
self-describing.

#### StreamData::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<StreamData, CodecError>
```

### i2c_error_name

```rust
pub fn i2c_error_name(code: u8) -> &'static str
```

`last_i2c_error` code in [`Vl53l4Info`] → human-readable name.

### XFER_MAX

```rust
pub const XFER_MAX: usize = 253;
```

Largest `len` (read) / data length (write) per transfer. The STM32 I2C
NBYTES field is 8-bit and a write spends two bytes on the register address;
the firmware applies the same 253 to both directions.

### XSHUT_OFF

```rust
pub const XSHUT_OFF: u8 = 0;
```

`VL53_XSHUT` action: drive XSHUT low (sensor powered down).

### XSHUT_ON

```rust
pub const XSHUT_ON: u8 = 1;
```

`VL53_XSHUT` action: drive XSHUT high, no boot handshake.

### XSHUT_RESET

```rust
pub const XSHUT_RESET: u8 = 2;
```

`VL53_XSHUT` action: pulse low then poll the boot handshake (answered after
it completes — allow ≥ 1.5 s).

### SF_INT_ACT_HIGH

```rust
pub const SF_INT_ACT_HIGH: u8 = 0x02;
```

`VL53_START_STREAM` flag: INT active high, mirroring bit 4 of
`GPIO_HV_MUX__CTRL` (0x0030). Clear (default): INT active low.

### I2C_KHZ_STEPS

```rust
pub const I2C_KHZ_STEPS: [u16; 9] = [100, 200, 400, 500, 600, 700, 800, 900, 1000];
```

Nominal SCL steps the firmware carries a TIMINGR for (`VL53_SET_I2C_SPEED`
clamps to the nearest one).
