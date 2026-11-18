# VL53L8CX (ToF) — API reference

The public API for the VL53L8CX (ToF) sensor. Transport, framing,
identity/discovery, firmware-container and dataset symbols shared
across sensors live in the [top-level API reference](../api.md).

Generated from the `///` doc-comments in the source — edit them,
not this file.

## Contents

- **VL53L8CX (ToF)**: [`MIN_RANGING_FREQUENCY_HZ`](#min_ranging_frequency_hz), [`max_ranging_frequency_hz`](#max_ranging_frequency_hz), [`BridgeUld`](#bridgeuld), [`Vl53l8cxDevice`](#vl53l8cxdevice), [`Vl53l8chDevice`](#vl53l8chdevice), [`DIST_MM`](#dist_mm), [`SIGNAL_PER_SPAD_KCPS`](#signal_per_spad_kcps), [`RANGE_SIGMA_MM`](#range_sigma_mm), [`AMBIENT_PER_SPAD_KCPS`](#ambient_per_spad_kcps), [`NB_TARGET_DETECTED`](#nb_target_detected), [`TAR_STATUS`](#tar_status), [`NB_SPADS_ENABLED`](#nb_spads_enabled), [`MOTION_INDICATOR`](#motion_indicator), [`NB_THRESHOLDS`](#nb_thresholds), [`POWER_MODE_SLEEP`](#power_mode_sleep), [`POWER_MODE_WAKEUP`](#power_mode_wakeup), [`POWER_MODE_DEEP_SLEEP`](#power_mode_deep_sleep), [`AdvancedError`](#advancederror), [`XTALK_MARGIN_MAX_KCPS`](#xtalk_margin_max_kcps), [`try_xtalk_margin_to_raw`](#try_xtalk_margin_to_raw), [`xtalk_margin_from_raw`](#xtalk_margin_from_raw), [`MotionConfig`](#motionconfig), [`try_default_motion_config`](#try_default_motion_config), [`DetectionThreshold`](#detectionthreshold), [`DetectionThresholdBlocks`](#detectionthresholdblocks), [`try_pack_detection_thresholds`](#try_pack_detection_thresholds), [`Vl53l8Cmd`](#vl53l8cmd), [`Vl53l8Rpt`](#vl53l8rpt), [`READ_MAX_LEN`](#read_max_len), [`CHUNK_SIZE`](#chunk_size), [`pack_read_reg`](#pack_read_reg), [`pack_write_reg`](#pack_write_reg), [`try_pack_write_reg`](#try_pack_write_reg), [`pack_start_stream`](#pack_start_stream), [`RegData`](#regdata), [`RESOLUTION_4X4`](#resolution_4x4), [`RESOLUTION_8X8`](#resolution_8x8), [`NB_TARGET_PER_ZONE`](#nb_target_per_zone), [`Variant`](#variant), [`Vl53l8Error`](#vl53l8error), [`Vl53l8Results`](#vl53l8results), [`swap_buffer`](#swap_buffer), [`parse_frame`](#parse_frame), [`Vl53l8CnhSetup`](#vl53l8cnhsetup), [`Vl53l8Motion`](#vl53l8motion), [`Vl53l8Frame`](#vl53l8frame), [`RANGING_MODE_CONTINUOUS`](#ranging_mode_continuous), [`RANGING_MODE_AUTONOMOUS`](#ranging_mode_autonomous), [`TARGET_ORDER_CLOSEST`](#target_order_closest), [`TARGET_ORDER_STRONGEST`](#target_order_strongest), [`STREAM_CHUNK_MAX`](#stream_chunk_max), [`STREAM_TOTAL_MAX`](#stream_total_max), [`FrameChunk`](#framechunk), [`unpack_frame_chunk`](#unpack_frame_chunk), [`CompletedFrame`](#completedframe), [`FrameReassembler`](#framereassembler)

## VL53L8CX (ToF)

### MIN_RANGING_FREQUENCY_HZ

```rust
pub const MIN_RANGING_FREQUENCY_HZ: i32 = 2;
```

Below this the VL53L8 never enters its ranging loop and streams nothing (contract 04).

### max_ranging_frequency_hz

```rust
pub fn max_ranging_frequency_hz(resolution: usize) -> i32
```

Highest ranging frequency at `resolution` zones (contract 11 §3; the same
silicon limit on VL53L8): 60 Hz at 4×4, 15 Hz at 8×8.

### BridgeUld

```rust
pub type BridgeUld = Vl53l8Driver<Arc<dyn Vl53l8Platform>>;
```

The ULD driver a VL53L8 / L5 / L7 device runs ([`Vl53l8cxDevice::with_uld`]):
unstable — not covered by the 1.0 freeze.

### Vl53l8cxDevice

```rust
pub struct Vl53l8cxDevice {
    dev: DepzDevice,
    shared: Arc<Shared>,
    platform: Arc<BridgePlatform>,
    profile: &'static Profile,
    ranging: AtomicBool,
    armed_cnh: Arc<Mutex<Option<Vl53l8CnhSetup>>>,
}
```

VL53L8CX multizone ToF (contract 04). The board is a thin SPI bridge; the ST ULD runs here
([`Vl53l8cxDevice::with_uld`]). [`Vl53l8cxDevice::init`] downloads the ~84 KB sensor firmware
(about a second), then configure and [`Vl53l8cxDevice::start_ranging`]: the board pushes one
frame per INT edge in chunks, reassembled and parsed into [`Vl53l8cxDevice::frames`].
Configuration needs `init` and fails with [`DepzError::InvalidState`] while ranging — the stream
owns the register bank. The VL53L8CH superset is [`Vl53l8chDevice`]; the L5/L7 classes deref to
this one too.

#### Vl53l8cxDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l8cxDevice
```

Continue the open connection `dev` as a VL53L8CX.

#### Vl53l8cxDevice::with_uld

```rust
pub fn with_uld<R>(&self, f: impl FnOnce(&mut BridgeUld) -> UldResult<R>) -> Result<R>
```

Run `f` on the ULD driver (escape hatch for DCI access). [`DepzError::InvalidState`] before
[`Vl53l8cxDevice::init`].

unstable — not covered by the 1.0 freeze.

#### Vl53l8cxDevice::initialized

```rust
pub fn initialized(&self) -> bool
```

True after a successful [`Vl53l8cxDevice::init`], until the sensor loses its state.

#### Vl53l8cxDevice::ranging

```rust
pub fn ranging(&self) -> bool
```

#### Vl53l8cxDevice::is_alive

```rust
pub fn is_alive(&self) -> bool
```

True when the sensor answers with its own silicon ids (a bus error or a foreign id: false).

#### Vl53l8cxDevice::init

```rust
pub fn init(&self) -> Result<()>
```

Download the sensor firmware and the default configuration.

#### Vl53l8cxDevice::init_with_progress

```rust
pub fn init_with_progress(
    &self,
    progress: &mut dyn FnMut(&str),
    write_progress: Option<WriteProgress>,
) -> Result<()>
```

[`Vl53l8cxDevice::init`] reporting its phases to `progress` and the big firmware writes to
`write_progress` (done, total bytes). On L5/L7, a module type other than the class expects
(a board labelled L7 carrying an L5, or the reverse) is a warning on stderr: ranging still
works, the firmware is shared.

#### Vl53l8cxDevice::get_resolution

```rust
pub fn get_resolution(&self) -> Result<usize>
```

Zones in use: 16 (4×4) or 64 (8×8).

#### Vl53l8cxDevice::set_resolution

```rust
pub fn set_resolution(&self, zones: usize) -> Result<()>
```

16 (4×4) or 64 (8×8) zones.

#### Vl53l8cxDevice::get_ranging_frequency_hz

```rust
pub fn get_ranging_frequency_hz(&self) -> Result<u8>
```

#### Vl53l8cxDevice::set_ranging_frequency_hz

```rust
pub fn set_ranging_frequency_hz(&self, hz: i32) -> Result<()>
```

At least 2 Hz (1 Hz on L5/L7); at most 60 Hz at 4×4 and 15 Hz at 8×8.

#### Vl53l8cxDevice::get_ranging_mode

```rust
pub fn get_ranging_mode(&self) -> Result<u8>
```

[`crate::vl53l8::RANGING_MODE_CONTINUOUS`] or [`crate::vl53l8::RANGING_MODE_AUTONOMOUS`].

#### Vl53l8cxDevice::set_ranging_mode

```rust
pub fn set_ranging_mode(&self, mode: u8) -> Result<()>
```

#### Vl53l8cxDevice::get_integration_time_ms

```rust
pub fn get_integration_time_ms(&self) -> Result<u32>
```

#### Vl53l8cxDevice::set_integration_time_ms

```rust
pub fn set_integration_time_ms(&self, ms: i32) -> Result<()>
```

2–1000 ms; autonomous mode only.

#### Vl53l8cxDevice::get_sharpener_percent

```rust
pub fn get_sharpener_percent(&self) -> Result<u8>
```

#### Vl53l8cxDevice::set_sharpener_percent

```rust
pub fn set_sharpener_percent(&self, pct: i32) -> Result<()>
```

Edge sharpener, 0–99 % (0 = off).

#### Vl53l8cxDevice::get_target_order

```rust
pub fn get_target_order(&self) -> Result<u8>
```

[`crate::vl53l8::TARGET_ORDER_CLOSEST`] or [`crate::vl53l8::TARGET_ORDER_STRONGEST`].

#### Vl53l8cxDevice::set_target_order

```rust
pub fn set_target_order(&self, order: u8) -> Result<()>
```

#### Vl53l8cxDevice::get_power_mode

```rust
pub fn get_power_mode(&self) -> Result<u8>
```

[`crate::vl53l8::advanced::POWER_MODE_SLEEP`], `POWER_MODE_WAKEUP` or `POWER_MODE_DEEP_SLEEP`.

#### Vl53l8cxDevice::set_power_mode

```rust
pub fn set_power_mode(&self, mode: u8) -> Result<()>
```

Waking from deep sleep downloads the firmware again.

#### Vl53l8cxDevice::get_xtalk_margin

```rust
pub fn get_xtalk_margin(&self) -> Result<u32>
```

Xtalk margin, whole kcps/SPAD (as ST).

#### Vl53l8cxDevice::set_xtalk_margin

```rust
pub fn set_xtalk_margin(&self, margin_kcps: u32) -> Result<()>
```

Xtalk margin, a whole number of kcps/SPAD, 0..10000 (as ST); above is refused.

#### Vl53l8cxDevice::calibrate_xtalk

```rust
pub fn calibrate_xtalk(
    &self,
    reflectance_percent: i32,
    nb_samples: i32,
    distance_mm: i32,
) -> Result<()>
```

On-device crosstalk calibration against a flat target; read the result with
[`Vl53l8cxDevice::get_caldata_xtalk`].

#### Vl53l8cxDevice::get_caldata_xtalk

```rust
pub fn get_caldata_xtalk(&self) -> Result<Vec<u8>>
```

The 776-byte xtalk calibration buffer (save / restore).

#### Vl53l8cxDevice::set_caldata_xtalk

```rust
pub fn set_caldata_xtalk(&self, blob: &[u8]) -> Result<()>
```

#### Vl53l8cxDevice::get_detection_thresholds_enable

```rust
pub fn get_detection_thresholds_enable(&self) -> Result<u8>
```

#### Vl53l8cxDevice::set_detection_thresholds_enable

```rust
pub fn set_detection_thresholds_enable(&self, enabled: bool) -> Result<()>
```

#### Vl53l8cxDevice::get_detection_thresholds

```rust
pub fn get_detection_thresholds(&self) -> Result<Vec<DetectionThreshold>>
```

#### Vl53l8cxDevice::set_detection_thresholds

```rust
pub fn set_detection_thresholds(&self, thresholds: &[DetectionThreshold]) -> Result<()>
```

The 64 detection thresholds (interrupt on threshold).

#### Vl53l8cxDevice::set_detection_thresholds_auto_stop

```rust
pub fn set_detection_thresholds_auto_stop(&self, auto_stop: bool) -> Result<()>
```

#### Vl53l8cxDevice::configure_motion_indicator

```rust
pub fn configure_motion_indicator(
    &self,
    distance_mm: Option<(i32, i32)>,
) -> Result<MotionConfig>
```

Enable the motion indicator over [min, max] mm (`None`: 400–1500); every frame then carries
[`Vl53l8Frame::motion`]. Returns the configuration for further tuning.

On CH firmware the motion indicator and CNH share one DCI struct (0xBFAC): this disarms CNH
(`cnh_setup()` → `None`, no [`Vl53l8Frame::cnh_raw`]).

#### Vl53l8cxDevice::start_ranging

```rust
pub fn start_ranging(&self) -> Result<()>
```

Program the output list, start the sensor and the board's stream.

#### Vl53l8cxDevice::stop_ranging

```rust
pub fn stop_ranging(&self) -> Result<()>
```

#### Vl53l8cxDevice::frames

```rust
pub fn frames(&self) -> &Stream<Vl53l8Frame>
```

The frame stream (contract 07 §3): `frames().on(cb)` calls back on the reader thread (or
the thread of a call that held the driver while the frame came in),
`frames().subscribe(capacity)` is a bounded drop-oldest queue subscribed at the call,
`frames().next(timeout)` waits for one; `dropped_counts()` and `parse_errors()` (a chunk
shorter than its header, a frame that did not parse — not reassembly gaps) are its counters.
A callback may take the ULD (`with_uld`); on the reader thread a bus request
(`get_resolution`, `get_ranging_mode`, …) times out — the reader that would deliver its
answer is in the callback — while on the thread of a call that held the ULD it goes through.

#### Vl53l8cxDevice::reassembler_discards

```rust
pub fn reassembler_discards(&self) -> u64
```

Chunked frames the reassembler discarded (gaps, offset errors).

### Vl53l8chDevice

```rust
pub struct Vl53l8chDevice(Vl53l8cxDevice);
```

VL53L8CH: the VL53L8CX superset (every CX method, through `Deref`) plus CNH, compact network
histograms — each frame can carry a per-aggregate distance histogram. `init` downloads the CH
firmware (VL53LMZ ULD 2.0.16).

#### Vl53l8chDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l8chDevice
```

Continue the open connection `dev` as a VL53L8CH.

#### Vl53l8chDevice::cnh_setup

```rust
pub fn cnh_setup(&self) -> Option<Vl53l8CnhSetup>
```

The CNH setup armed by [`Vl53l8chDevice::configure_cnh`], or `None`; recorders keep it next
to the raw blocks to decode them later.

#### Vl53l8chDevice::configure_cnh

```rust
pub fn configure_cnh(&self, setup: &Vl53l8CnhSetup) -> Result<()>
```

Arm CNH for the next [`Vl53l8cxDevice::start_ranging`]: every frame then carries
[`Vl53l8Frame::cnh_raw`], decoded with [`crate::vl53l8::decode_cnh`] and
[`Vl53l8CnhSetup::decode_config`]. CNH and the motion indicator share one DCI struct
(0xBFAC): this disarms the motion indicator (frames carry no [`Vl53l8Frame::motion`]).

### DIST_MM

```rust
pub const DIST_MM: u8 = 1;
```

Detection-threshold measurement selectors (`measurement` field).

### SIGNAL_PER_SPAD_KCPS

```rust
pub const SIGNAL_PER_SPAD_KCPS: u8 = 2;
```

### RANGE_SIGMA_MM

```rust
pub const RANGE_SIGMA_MM: u8 = 4;
```

### AMBIENT_PER_SPAD_KCPS

```rust
pub const AMBIENT_PER_SPAD_KCPS: u8 = 8;
```

### NB_TARGET_DETECTED

```rust
pub const NB_TARGET_DETECTED: u8 = 9;
```

### TAR_STATUS

```rust
pub const TAR_STATUS: u8 = 12;
```

### NB_SPADS_ENABLED

```rust
pub const NB_SPADS_ENABLED: u8 = 13;
```

### MOTION_INDICATOR

```rust
pub const MOTION_INDICATOR: u8 = 19;
```

### NB_THRESHOLDS

```rust
pub const NB_THRESHOLDS: usize = 64;
```

### POWER_MODE_SLEEP

```rust
pub const POWER_MODE_SLEEP: u8 = 0;
```

Power modes (`vl53l8cx_api.h`).

### POWER_MODE_WAKEUP

```rust
pub const POWER_MODE_WAKEUP: u8 = 1;
```

### POWER_MODE_DEEP_SLEEP

```rust
pub const POWER_MODE_DEEP_SLEEP: u8 = 2;
```

### AdvancedError

```rust
#[non_exhaustive]
pub enum AdvancedError {
    /// Motion indicator resolution other than 16 (4×4) or 64 (8×8).
    InvalidResolution(usize),
    /// Xtalk margin above 10000 kcps.
    XtalkMarginOutOfRange,
    /// Threshold `index`: low/high × its measurement scale does not fit i32.
    ThresholdOverflow(usize),
    /// More than [`NB_THRESHOLDS`] thresholds.
    TooManyThresholds(usize),
}
```

Advanced-codec argument error (contract 07 §3: an argument the driver
would refuse, or one that does not fit its DCI field, is rejected before
anything is written — the ULD's `VL53L8CX_STATUS_INVALID_PARAM`).

### XTALK_MARGIN_MAX_KCPS

```rust
pub const XTALK_MARGIN_MAX_KCPS: u32 = 10000;
```

Largest xtalk margin `set_xtalk_margin` accepts, whole kcps/SPAD (as ST).

### try_xtalk_margin_to_raw

```rust
pub fn try_xtalk_margin_to_raw(margin_kcps: u32) -> Result<u32, AdvancedError>
```

Xtalk margin, a whole number of kcps/SPAD (as ST) → raw DCI value
`kcps * 2048`, as `set_xtalk_margin` writes it: 0..=[`XTALK_MARGIN_MAX_KCPS`],
else [`AdvancedError::XtalkMarginOutOfRange`].

### xtalk_margin_from_raw

```rust
pub fn xtalk_margin_from_raw(raw: u32) -> u32
```

Inverse: raw DCI value → whole kcps/SPAD (`raw / 2048`, integer division as ST).

### MotionConfig

```rust
#[non_exhaustive]
pub struct MotionConfig {
    pub ref_bin_offset: i32,
    pub detection_threshold: u32,
    pub extra_noise_sigma: u32,
    pub null_den_clip_value: u32,
    pub mem_update_mode: u8,
    pub mem_update_choice: u8,
    pub sum_span: u8,
    pub feature_length: u8,
    pub nb_of_aggregates: u8,
    pub nb_of_temporal_accumulations: u8,
    pub min_nb_for_global_detection: u8,
    pub global_indicator_format_1: u8,
    pub global_indicator_format_2: u8,
    pub spare1: u8,
    pub spare2: u8,
    pub spare3: u8,
    pub map_id: [i8; 64],
    pub indicator_format_1: [u8; 32],
    pub indicator_format_2: [u8; 32],
}
```

Mirror of `VL53L8CX_Motion_Configuration` (156 bytes, plugin source).
`pack()` reproduces the C struct byte layout (`<i3I12B64b32B32B`).

#### MotionConfig::pack

```rust
pub fn pack(&self) -> Vec<u8>
```

Serialize to the 156-byte DCI payload.

#### MotionConfig::try_set_resolution

```rust
pub fn try_set_resolution(&mut self, resolution: usize) -> Result<(), AdvancedError>
```

Set `map_id` for the resolution (pure; mirrors `_set_resolution`):
16 or 64 zones, else [`AdvancedError::InvalidResolution`] with
`map_id` untouched.

### try_default_motion_config

```rust
pub fn try_default_motion_config(resolution: usize) -> Result<MotionConfig, AdvancedError>
```

The default motion-indicator configuration `motion_indicator_init` programs
for a resolution (the exact bytes written to the sensor); a resolution
other than 16 or 64 is [`AdvancedError::InvalidResolution`].

### DetectionThreshold

```rust
#[non_exhaustive]
pub struct DetectionThreshold {
    pub low_thresh: i32,
    pub high_thresh: i32,
    pub measurement: u8,
    pub type_: u8,
    pub zone_num: u8,
    pub operation: u8,
}
```

One detection-threshold entry (real units; scaled on pack).

### DetectionThresholdBlocks

```rust
#[non_exhaustive]
pub struct DetectionThresholdBlocks {
    /// `DCI_DET_THRESH_START` payload: 64 × 12 bytes.
    pub start: Vec<u8>,
    /// `DCI_DET_THRESH_VALID_STATUS`: 8 bytes, all `0x05`.
    pub valid: Vec<u8>,
}
```

The two DCI blocks written by `set_detection_thresholds`.

### try_pack_detection_thresholds

```rust
pub fn try_pack_detection_thresholds(
    thresholds: &[DetectionThreshold],
) -> Result<DetectionThresholdBlocks, AdvancedError>
```

Pack up to 64 detection thresholds into their DCI blocks. Entries beyond
the supplied list are zero-filled. Each entry's low/high are multiplied by
its measurement's fixed-point scale; a product that does not fit the i32
field is [`AdvancedError::ThresholdOverflow`] (never wrapped), more than
64 entries [`AdvancedError::TooManyThresholds`].

### Vl53l8Cmd

```rust
#[non_exhaustive]
pub enum Vl53l8Cmd {
    ReadReg = 0x32,
    WriteReg = 0x33,
    StartStream = 0x35,
    StopStream = 0x36,
}
```

VL53L8 host→device command opcodes (0x34 is unused on VL53L8).

### Vl53l8Rpt

```rust
#[non_exhaustive]
pub enum Vl53l8Rpt {
    RegData = 0x91,
    Vl53Frame = 0x93,
}
```

VL53L8 device→host report opcodes.

### READ_MAX_LEN

```rust
pub const READ_MAX_LEN: usize = 2295;
```

Largest `VL53_READ_REG` `len` on VL53L8: the MCU transport buffer (2304 B)
minus the 9-byte `RPT_VL53_REG_DATA` header.

### CHUNK_SIZE

```rust
pub const CHUNK_SIZE: usize = 2048;
```

Transfer size the VL53L8 host splits reads/writes at.

### pack_read_reg

```rust
pub fn pack_read_reg(addr: u16, len: u16) -> [u8; 4]
```

`VL53_READ_REG` payload: `addr u16, len u16` (little-endian).

### pack_write_reg

```rust
pub fn pack_write_reg(addr: u16, data: &[u8]) -> Vec<u8>
```

`VL53_WRITE_REG` payload: `addr u16` followed by the raw register bytes.

### try_pack_write_reg

```rust
pub fn try_pack_write_reg(addr: u16, data: &[u8]) -> Result<Vec<u8>, CodecError>
```

[`pack_write_reg`] that refuses empty data (contract 04 §2: at least one
data byte).

### pack_start_stream

```rust
pub fn pack_start_stream(frame_size: u16) -> Result<[u8; 2], CodecError>
```

`VL53_START_STREAM` payload: `frame_size u16`, which must be
1..=[`STREAM_TOTAL_MAX`] (contract 04 §4 rule 4 — the firmware refuses
anything else too).

### RegData

```rust
#[non_exhaustive]
pub struct RegData {
    /// Echoed `VL53_READ_REG` opcode (0x32).
    pub cmd: u8,
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

### RESOLUTION_4X4

```rust
pub const RESOLUTION_4X4: usize = 16;
```

### RESOLUTION_8X8

```rust
pub const RESOLUTION_8X8: usize = 64;
```

### NB_TARGET_PER_ZONE

```rust
pub const NB_TARGET_PER_ZONE: usize = 1;
```

### Variant

```rust
#[non_exhaustive]
pub enum Variant {
    /// Base VL53L8CX (dev default), or any device on ULD 2.1.0 footer geometry.
    Cx,
    /// VL53L8CH (VL53LMZ 2.0.16 firmware): footer id at `size-4`.
    Ch,
    /// VL53L5CX / VL53L7CX / VL53L7CH (contract 11): footer id at `size-4`,
    /// and every per-zone array trimmed to the frame's resolution. On these
    /// parts the per-target blocks (index ≥ 0x6C90) keep 64 entries even in
    /// 4×4 — the sensor fills the first `resolution` and zero-pads the rest.
    /// The resolution is read from the frame itself: the ambient-rate block
    /// (0x54D0, index range sized to the resolution by the vl53lmz 2.0.16
    /// output-list rule) holds exactly one entry per zone.
    L7,
}
```

ToF silicon/firmware variant. Both the base **VL53L8CX** and the
**VL53L8CH** (CX + CNH + production PID 0xED40) share one results-frame
layout; only the frame-tail geometry differs for decoding, and that is all
this enum selects. The footer-id offset is `size-12` (ULD 2.1.0, selected by
[`Variant::Cx`]) or `size-4` (VL53LMZ 2.0.16, selected by [`Variant::Ch`]).

The geometry follows the firmware each part runs: the VL53L8CX firmware
embeds ULD 2.1.0 (`size-12`), the VL53L8CH firmware VL53LMZ 2.0.16
(`size-4`), so decode CH frames with [`Variant::Ch`].

### Vl53l8Error

```rust
#[non_exhaustive]
pub enum Vl53l8Error {
    /// Header/footer id mismatch (`STATUS_CORRUPTED_FRAME`).
    CorruptedFrame,
    /// Frame shorter than the fixed 16-byte prologue.
    ShortFrame,
}
```

Frame-decode failure.

### Vl53l8Results

```rust
#[non_exhaustive]
pub struct Vl53l8Results {
    /// mm: `floor(raw/4)`, a negative distance clamped to 0, as ST `GetRangingData`.
    pub distance_mm: Vec<i32>,
    /// 5/9 = valid; 255 = no target detected in the zone.
    pub target_status: Vec<u8>,
    pub nb_target_detected: Vec<u8>,
    /// Signal rate, kcps/SPAD (`raw / 2048`).
    pub signal_per_spad: Vec<f64>,
    /// Ambient rate, kcps/SPAD (`raw / 2048`).
    pub ambient_per_spad: Vec<f64>,
    pub nb_spads_enabled: Vec<u32>,
    /// Measurement noise, mm (`raw / 128`).
    pub range_sigma_mm: Vec<f64>,
    /// Estimated target reflectance, % (`raw / 2`).
    pub reflectance: Vec<f64>,
    pub silicon_temp_degc: i8,
    /// Raw compact-network-histogram block bytes, present only on **VL53L8CH**
    /// frames that carry a CNH output block. Pass this to
    /// [`super::cnh::decode_cnh`] to unpack it into per-aggregate histograms.
    /// `None` on CX frames and on CH frames without a CNH block.
    pub cnh_raw: Option<Vec<u8>>,
}
```

One decoded results frame. Per-zone arrays are sized to the resolution
actually present in the frame (`resolution` = `nb_target_detected.len()`).

#### Vl53l8Results::resolution

```rust
pub fn resolution(&self) -> usize
```

Active resolution (16 or 64), from the number of zones decoded.

### swap_buffer

```rust
pub fn swap_buffer(data: &[u8]) -> Vec<u8>
```

VL53L8CX `SwapBuffer`: byte-reverse every complete 32-bit word (a <4-byte
tail is left untouched).

### parse_frame

```rust
pub fn parse_frame(raw: &[u8], variant: Variant) -> Result<Vl53l8Results, Vl53l8Error>
```

Decode one raw results frame (`raw` = `full_size` bytes read from reg 0x00).

The frame's own length is authoritative (`data_read_size = raw.len()`); the
FW streams exactly the size advertised in each `RPT_VL53_FRAME` chunk.

### Vl53l8CnhSetup

```rust
pub struct Vl53l8CnhSetup {
    ref_bin_offset: i32,
    sum_span: i32,
    feature_length: i32,
    nb_of_aggregates: i32,
    cnh_cfg: u8,
    cnh_flex_shift: u8,
    map_id: [i8; 64],
}
```

The CNH (compact network histogram) configuration of a VL53L8CH / VL53L7CH.

A mirror of `VL53LMZ_Motion_Configuration` with the plugin helpers that fill it
(`vl53lmz_plugin_cnh.c`, ULD 2.0.16), ported from the reference Python `CnhConfig`. Build it
with [`Vl53l8CnhSetup::init_config`] and [`Vl53l8CnhSetup::create_agg_map`], check
[`Vl53l8CnhSetup::required_memory`], then arm it on the device (the CNH block size goes to
[`super::Vl53l8Driver::start_ranging`], the struct to DCI [`Vl53l8CnhSetup::MI_CFG_DEV_IDX`]).
The flags are fixed to ping-pong and variance off (`Example_12_cnh_data.c`), which
[`crate::vl53l8::decode_cnh`] assumes. Arguments outside their fields are
[`DepzError::InvalidArgument`], never wrapped.

#### Vl53l8CnhSetup::new

```rust
pub fn new() -> Self
```

An empty configuration: no bins, no aggregates, every zone in none.

#### Vl53l8CnhSetup::ref_bin_offset

```rust
pub fn ref_bin_offset(&self) -> i32
```

The first device bin, ×2048 (`ref_bin_offset`).

#### Vl53l8CnhSetup::sum_span

```rust
pub fn sum_span(&self) -> i32
```

Device bins summed per CNH bin.

#### Vl53l8CnhSetup::feature_length

```rust
pub fn feature_length(&self) -> i32
```

CNH bins per aggregate.

#### Vl53l8CnhSetup::nb_of_aggregates

```rust
pub fn nb_of_aggregates(&self) -> i32
```

Aggregates in the map.

#### Vl53l8CnhSetup::cnh_cfg

```rust
pub fn cnh_cfg(&self) -> u8
```

The CNH flags byte.

#### Vl53l8CnhSetup::cnh_flex_shift

```rust
pub fn cnh_flex_shift(&self) -> u8
```

The CNH flex shift.

#### Vl53l8CnhSetup::map_id

```rust
pub fn map_id(&self) -> [i8; 64]
```

Device zone → CNH aggregate, −1 for a zone in none (64 entries).

#### Vl53l8CnhSetup::decode_config

```rust
pub fn decode_config(&self) -> CnhDecodeConfig
```

The counts [`crate::vl53l8::decode_cnh`] needs to unpack this configuration's blocks.

#### Vl53l8CnhSetup::init_config

```rust
pub fn init_config(&mut self, start_bin: i32, num_bins: i32, sub_sample: i32) -> Result<()>
```

`vl53lmz_cnh_init_config`. `start_bin`: first device histogram bin; `num_bins`: CNH bins;
`sub_sample`: device bins summed per CNH bin.

#### Vl53l8CnhSetup::create_agg_map

```rust
pub fn create_agg_map(
    &mut self,
    resolution: usize,
    start_x: i32,
    start_y: i32,
    merge_x: i32,
    merge_y: i32,
    cols: i32,
    rows: i32,
) -> Result<()>
```

`vl53lmz_cnh_create_agg_map`: aggregates of `merge_x`×`merge_y` zones, `cols`×`rows` of
them from (`start_x`, `start_y`). `resolution` (16 or 64) must be the sensor's.

#### Vl53l8CnhSetup::required_memory

```rust
pub fn required_memory(&self) -> Result<u32>
```

`vl53lmz_cnh_calc_required_memory`: the on-device CNH buffer, bytes. No aggregate map yet,
or more than [`Vl53l8CnhSetup::CNH_MAX_DATA_BYTES`], is [`DepzError::InvalidArgument`].

#### Vl53l8CnhSetup::min_max_distance_mm

```rust
pub fn min_max_distance_mm(&self) -> (i32, i32)
```

(min, max) target distance, mm, the histogram captures whole.

#### Vl53l8CnhSetup::bin_center_mm

```rust
pub fn bin_center_mm(&self, bin_idx: i32) -> f64
```

Distance, mm, at the centre of CNH bin `bin_idx`.

#### Vl53l8CnhSetup::pack

```rust
pub fn pack(&self) -> Vec<u8>
```

The 156-byte `VL53LMZ_Motion_Configuration` struct for the device (DCI 0xBFAC).

### Vl53l8Motion

```rust
#[non_exhaustive]
pub struct Vl53l8Motion {
    /// Global indicator 1.
    pub global_indicator_1: u32,
    /// Global indicator 2.
    pub global_indicator_2: u32,
    /// Motion-indicator status.
    pub status: u8,
    /// Aggregates with motion detected.
    pub nb_of_detected_aggregates: u8,
    /// Aggregates configured.
    pub nb_of_aggregates: u8,
    /// Per-aggregate motion value (32 entries).
    pub motion: Vec<u32>,
}
```

The motion-indicator block of a frame (`VL53L8CX_ResultsData MotionIndicator`).

### Vl53l8Frame

```rust
#[non_exhaustive]
pub struct Vl53l8Frame {
    /// Host timestamp the caller gave the frame, µs.
    pub timestamp_us: u64,
    /// Zones in this frame (16 or 64).
    pub resolution: usize,
    /// Silicon temperature, °C.
    pub silicon_temp_degc: i8,
    /// Distance per zone and target, mm (≥ 0).
    pub distance_mm: Vec<i32>,
    /// Target status per zone and target: 5 / 9 valid, 255 no target detected.
    pub target_status: Vec<u8>,
    /// Targets detected per zone.
    pub nb_target_detected: Vec<u8>,
    /// Signal rate per zone and target, kcps/SPAD.
    pub signal_per_spad: Vec<f64>,
    /// Ambient rate per zone, kcps/SPAD.
    pub ambient_per_spad: Vec<f64>,
    /// SPADs enabled per zone (raw, ÷256).
    pub nb_spads_enabled: Vec<u32>,
    /// Range sigma per zone and target, mm.
    pub range_sigma_mm: Vec<f64>,
    /// Reflectance estimate per zone and target, %.
    pub reflectance: Vec<f64>,
    /// The raw CNH block (word-swapped like every block), or `None`; decode with
    /// [`crate::vl53l8::decode_cnh`].
    pub cnh_raw: Option<Vec<u8>>,
    /// The motion-indicator output, or `None` when the indicator is not configured.
    pub motion: Option<Vl53l8Motion>,
}
```

One decoded ranging frame.

Per-zone vectors are sized to the active resolution (16 for 4×4, 64 for 8×8); zone index runs
row-major. Values are in ST GetRangingData's real units (contract 04 §5): a negative distance
is clamped to 0, the rates and the reflectance keep their fraction. `cnh_raw` is the raw CNH data block (VL53L8CH / VL53L7CH with CNH armed),
else `None`; `motion` the motion-indicator output when it is configured, else `None`.

#### Vl53l8Frame::with_resolution

```rust
pub fn with_resolution(mut self, zones: usize) -> Self
```

This frame with `resolution` replaced (the device's own count of zones in use).

### RANGING_MODE_CONTINUOUS

```rust
pub const RANGING_MODE_CONTINUOUS: u8 = 1;
```

`set_ranging_mode`: continuous ranging (the integration time has no effect).

### RANGING_MODE_AUTONOMOUS

```rust
pub const RANGING_MODE_AUTONOMOUS: u8 = 3;
```

`set_ranging_mode`: autonomous ranging (frames at the ranging frequency, set integration time).

### TARGET_ORDER_CLOSEST

```rust
pub const TARGET_ORDER_CLOSEST: u8 = 1;
```

`set_target_order`: closest target first.

### TARGET_ORDER_STRONGEST

```rust
pub const TARGET_ORDER_STRONGEST: u8 = 2;
```

`set_target_order`: strongest target first.

### STREAM_CHUNK_MAX

```rust
pub const STREAM_CHUNK_MAX: usize = 1528;
```

Bytes of frame data carried per `RPT_VL53_FRAME` chunk.

### STREAM_TOTAL_MAX

```rust
pub const STREAM_TOTAL_MAX: usize = 8192;
```

Largest `frame_size` accepted by `START_STREAM`.

### FrameChunk

```rust
#[non_exhaustive]
pub struct FrameChunk {
    pub timestamp_us: u64,
    pub full_size: u16,
    pub offset: u16,
    pub data: Vec<u8>,
}
```

One `RPT_VL53_FRAME` chunk (header + a slice of the frame).

### unpack_frame_chunk

```rust
pub fn unpack_frame_chunk(payload: &[u8]) -> Option<FrameChunk>
```

Decode an `RPT_VL53_FRAME` payload: `timestamp_us` u64 LE, `full_size` u16
LE, `offset` u16 LE, then the chunk data. Returns `None` if the payload is
shorter than the 12-byte header.

### CompletedFrame

```rust
#[non_exhaustive]
pub struct CompletedFrame {
    pub timestamp_us: u64,
    pub frame: Vec<u8>,
}
```

A completed sensor frame with its capture timestamp.

### FrameReassembler

```rust
pub struct FrameReassembler {
    pub completed: u64,
    pub discarded: u64,
    chunk_max: usize,
    buf: Vec<u8>,
    full_size: usize,
    timestamp_us: u64,
}
```

Rebuilds full sensor frames from chunked `RPT_VL53_FRAME` reports.

#### FrameReassembler::new

```rust
pub fn new() -> Self
```

A reassembler for the VL53L8 board's [`STREAM_CHUNK_MAX`] chunks.

#### FrameReassembler::with_chunk_max

```rust
pub fn with_chunk_max(chunk_max: usize) -> Self
```

A reassembler for a board whose `VL53_STREAM_FRAME_MAX` is `chunk_max`
(`vl53l7::STREAM_CHUNK_MAX` = 1536 on the I2C board, contract 11 §3).

#### FrameReassembler::feed

```rust
pub fn feed(&mut self, chunk: FrameChunk) -> Option<CompletedFrame>
```

Feed one chunk; returns a `CompletedFrame` when a frame finishes.
