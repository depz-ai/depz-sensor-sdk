//! The VL53L 1D family on the `APP_VL53L0_4` register bridge (contract 12): VL53L0X, VL53L1CX,
//! VL53L1CB, VL53L3CX, VL53L4CD, VL53L4CX — every ULD on the host.

use std::collections::{BTreeMap, VecDeque};
use std::ops::Deref;
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::{Arc, Mutex, MutexGuard, TryLockError};
use std::time::Duration;

use super::device::{expect_report, DepzDevice, ReportHandler, Requester};
use super::error::{DepzError, Result};
use super::stream::{Fanout, Hook, Subscription};
use crate::framing::Packet;
use crate::vl53lx::bare::HistogramBinData;
use crate::vl53lx::driver::{
    BinData, ExtraValue, Thresholds, Vl53lxDriver, Vl53lxPlatform, Vl53lxRange, Vl53lxTarget,
};
use crate::vl53lx::products::{product as product_row, product_from_board_name, DriverKind};
use crate::vl53lx::uld::{create_driver, driver_kinds};
use crate::vl53lx::{
    pack_set_addr_width, pack_set_i2c_speed, pack_start_stream, pack_xshut, try_pack_read_reg,
    try_pack_write_reg, RegData, StreamData, Vl53lxCmd, Vl53lxInfo, Vl53lxRpt, XFER_MAX,
    XSHUT_RESET,
};

const REG_TIMEOUT: Duration = Duration::from_secs(2);
const XSHUT_TIMEOUT: Duration = Duration::from_secs(1);
const STREAM_MIN_BYTES: usize = 12;
const ERR_INVALID_CMD: u8 = 0x02;

/// Statuses that mean "this distance is real": a histogram product reports 6 on the first frame of a
/// stream and 11 for two merged targets. Prefer [`Vl53lxMeasurement::plottable`] over
/// [`Vl53lxMeasurement::valid`] there.
pub const PLOTTABLE_STATUSES: [i32; 3] = [0, 6, 11];

/// One ranging result of the 1D family, the same shape for every product. On the histogram driver
/// `targets` holds every return, strongest first, the top level repeats the first and `bins` carries
/// the raw 24-bin histogram; the light drivers leave both empty.
#[derive(Debug, Clone, PartialEq)]
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

impl Vl53lxMeasurement {
    pub fn valid(&self) -> bool {
        self.status == 0
    }

    pub fn plottable(&self) -> bool {
        PLOTTABLE_STATUSES.contains(&self.status)
    }

    /// The distances a chart draws: the plottable targets in driver order, or the single distance.
    pub fn plot_distances(&self) -> Vec<i32> {
        if !self.targets.is_empty() {
            return self
                .targets
                .iter()
                .filter(|t| PLOTTABLE_STATUSES.contains(&t.status))
                .map(|t| t.distance_mm)
                .collect();
        }
        if self.plottable() {
            vec![self.distance_mm]
        } else {
            Vec::new()
        }
    }

    /// The one target a single-number readout shows, or `None`.
    pub fn primary_target(&self) -> Option<&Vl53lxTarget> {
        self.targets
            .iter()
            .find(|t| PLOTTABLE_STATUSES.contains(&t.status))
    }

    fn from(timestamp_us: u64, r: Vl53lxRange, bins: Option<BinData>) -> Vl53lxMeasurement {
        Vl53lxMeasurement {
            timestamp_us,
            distance_mm: r.distance_mm,
            status: r.status,
            status_text: r.status_text,
            signal_kcps: r.signal_kcps,
            ambient_kcps: r.ambient_kcps,
            sigma_mm: r.sigma_mm,
            spads: r.spads,
            targets: r.targets,
            extra: r.extra,
            bins: bins
                .and_then(|b| b.downcast::<HistogramBinData>().ok())
                .map(Arc::from),
        }
    }
}

/// Everything about what is connected ([`Vl53lxDevice::identify_product`]).
#[derive(Debug, Clone, PartialEq)]
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

type DriverSlot = Option<Box<dyn Vl53lxDriver>>;

// The driver is shared by the caller's thread (configuration, polling) and the reader thread (stream
// decode), and driver calls go over the bus, whose answers the reader thread delivers. So the reader
// never waits for the driver: a frame that arrives while a call holds it goes to the backlog, and that
// call decodes it on its way out (`release`). Frames keep their order; the measurements go out with
// the backlog held.
struct Streams {
    driver: Mutex<DriverSlot>,
    backlog: Mutex<VecDeque<Vec<u8>>>,
    measurements: Fanout<Vl53lxMeasurement>,
    parse_errors: AtomicU64,
}

impl Streams {
    // Decoded exactly once: the histogram driver steps its own frame-pair state on every frame.
    fn decode(&self, drv: &mut DriverSlot, payload: &[u8]) -> Option<Vl53lxMeasurement> {
        let drv = drv.as_deref_mut()?;
        // A bad sample must not kill the reader: an error or a panic is a parse error.
        let r = catch_unwind(AssertUnwindSafe(|| -> Result<Vl53lxMeasurement> {
            let sample = StreamData::unpack(payload)?; // shorter than its length field: refused
            decode_frame(drv, sample.timestamp_us, &sample.data)
        }));
        match r {
            Ok(Ok(m)) => Some(m),
            _ => {
                self.parse_errors.fetch_add(1, Ordering::Relaxed);
                None
            }
        }
    }

    /// Give the driver back, decoding what streamed in meanwhile.
    fn release(&self, mut drv: MutexGuard<'_, DriverSlot>) {
        let mut backlog = self.backlog.lock().unwrap();
        let out: Vec<Vl53lxMeasurement> = backlog
            .drain(..)
            .filter_map(|p| self.decode(&mut drv, &p))
            .collect();
        drop(drv);
        for m in out {
            self.measurements.emit(m);
        }
    }
}

impl ReportHandler for Streams {
    fn handle_report(&self, packet: &Packet) -> bool {
        if packet.cmd != Vl53lxRpt::Stream as u8 || packet.payload.len() < STREAM_MIN_BYTES {
            return false;
        }
        let mut backlog = self.backlog.lock().unwrap();
        let mut drv = match self.driver.try_lock() {
            Ok(g) => g,
            Err(TryLockError::Poisoned(p)) => p.into_inner(),
            Err(TryLockError::WouldBlock) => {
                backlog.push_back(packet.payload.clone());
                return true;
            }
        };
        let mut out: Vec<Vl53lxMeasurement> = backlog
            .drain(..)
            .filter_map(|p| self.decode(&mut drv, &p))
            .collect();
        out.extend(self.decode(&mut drv, &packet.payload));
        drop(drv);
        for m in out {
            self.measurements.emit(m);
        }
        true
    }

    fn reader_stopped(&self) {
        self.measurements.close_all();
    }
}

fn decode_frame(
    drv: &mut dyn Vl53lxDriver,
    timestamp_us: u64,
    data: &[u8],
) -> Result<Vl53lxMeasurement> {
    if drv.histogram() {
        let bins = drv.bin_data(Some(data))?;
        let r = drv.to_measurement(&bins)?;
        Ok(Vl53lxMeasurement::from(timestamp_us, r, Some(bins)))
    } else {
        Ok(Vl53lxMeasurement::from(
            timestamp_us,
            drv.decode(data)?,
            None,
        ))
    }
}

// The platform the ULD ports expect, over the register bridge: transfers split at 253 bytes; a non-OK
// status or a missing answer comes back as the DepzError the drivers treat as a bridge error.
struct BridgePlatform {
    dev: Requester,
    last_timestamp_us: AtomicU64,
}

impl BridgePlatform {
    /// Zero the bridge's I2C error counters once an init is through (a resetting die NACKs for a
    /// moment, expectedly). A v2.00 bridge (firmware older than v0.24) lacks the command.
    fn clear_i2c_errors(&self) -> Result<()> {
        match self.dev.request_ok(Vl53lxCmd::ClearI2cErrors as u8, &[], None) {
            Err(DepzError::Status { status, .. }) if status == ERR_INVALID_CMD => Err(DepzError::Sensor(
                "the board's firmware predates APP_VL53L0_4_v0.24 (protocol v2.01) and cannot clear its I2C error \
                 counter - reflash it"
                    .into(),
            )),
            r => r,
        }
    }
}

impl Vl53lxPlatform for BridgePlatform {
    fn read_multi(&self, addr: u16, size: usize) -> Result<Vec<u8>> {
        let mut out = Vec::with_capacity(size);
        let mut addr = addr;
        while out.len() < size {
            let n = (size - out.len()).min(XFER_MAX);
            let rep: RegData = self.dev.request(
                Vl53lxCmd::ReadReg as u8,
                &try_pack_read_reg(addr, n as u16)
                    .map_err(|e| DepzError::InvalidArgument(e.0.to_string()))?,
                expect_report(Vl53lxRpt::RegData as u8, RegData::unpack),
                Some(REG_TIMEOUT),
            )?;
            if rep.data.len() != n {
                return Err(DepzError::Protocol(format!(
                    "READ_REG 0x{addr:04X}: expected {n}, got {}",
                    rep.data.len()
                )));
            }
            self.last_timestamp_us
                .store(rep.timestamp_us, Ordering::Relaxed);
            out.extend_from_slice(&rep.data);
            addr = addr.wrapping_add(n as u16);
        }
        Ok(out)
    }

    fn write_multi(&self, addr: u16, data: &[u8]) -> Result<()> {
        let mut addr = addr;
        for chunk in data.chunks(XFER_MAX) {
            self.dev.request_ok(
                Vl53lxCmd::WriteReg as u8,
                &try_pack_write_reg(addr, chunk)
                    .map_err(|e| DepzError::InvalidArgument(e.0.to_string()))?,
                Some(REG_TIMEOUT),
            )?;
            addr = addr.wrapping_add(chunk.len() as u16);
        }
        Ok(())
    }

    fn set_i2c_speed(&self, khz: u16) -> Result<()> {
        self.dev
            .request_ok(Vl53lxCmd::SetI2cSpeed as u8, &pack_set_i2c_speed(khz), None)
    }

    fn set_addr_width(&self, width: u8) -> Result<()> {
        self.dev.request_ok(
            Vl53lxCmd::SetAddrWidth as u8,
            &pack_set_addr_width(width)?,
            None,
        )
    }

    fn xshut_reset(&self) -> Result<()> {
        self.dev.request_ok(
            Vl53lxCmd::Xshut as u8,
            &pack_xshut(XSHUT_RESET),
            Some(XSHUT_TIMEOUT),
        )
    }
}

// The pair init bound.
#[derive(Clone, Copy)]
struct Bound {
    product: &'static str,
    kind: &'static str,
    caveat: Option<&'static str>,
}

/// A board of the VL53L 1D family on the `APP_VL53L0_4` register bridge (contract 12): VL53L0X,
/// VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CD, VL53L4CX. Every ULD runs here; two axes pick how a board
/// is driven — the product (whose parameter set: the board's own, or a sibling's to borrow its
/// driver) and the driver kind (`uld`, `ulp`, `histogram`). [`Vl53lxDevice::init`] binds the pair,
/// [`Vl53lxDevice::configure`] re-initialises and applies a ranging configuration (before every
/// run), then [`Vl53lxDevice::start_ranging`] and read [`Vl53lxDevice::measurements`]. Ask
/// [`Vl53lxDevice::supports`] before an optional group; a missing one fails with
/// [`DepzError::Unsupported`]. This generic class reads the product from the board's name; the
/// per-product classes ([`Vl53l0xDevice`], …) fix it. Configuration while ranging fails with
/// [`DepzError::InvalidState`] — the stream owns the bus.
pub struct Vl53lxDevice {
    dev: DepzDevice,
    fixed_product: Option<&'static str>,
    streams: Arc<Streams>,
    platform: Arc<BridgePlatform>,
    bound: Arc<Mutex<Option<Bound>>>,
    board_name: Mutex<Option<String>>,
    ranging: AtomicBool,
}

impl Deref for Vl53lxDevice {
    type Target = DepzDevice;

    fn deref(&self) -> &DepzDevice {
        &self.dev
    }
}

impl Vl53lxDevice {
    /// Continue the open connection `dev` as a 1D-family board; the product comes from its name.
    pub fn attach(dev: DepzDevice) -> Vl53lxDevice {
        Vl53lxDevice::attach_as(dev, None)
    }

    fn attach_as(dev: DepzDevice, fixed_product: Option<&'static str>) -> Vl53lxDevice {
        let streams = Arc::new(Streams {
            driver: Mutex::new(None),
            backlog: Mutex::new(VecDeque::new()),
            measurements: Fanout::new(dev.callback_errors_counter()),
            parse_errors: AtomicU64::new(0),
        });
        dev.set_report_handler(streams.clone());
        let platform = Arc::new(BridgePlatform {
            dev: dev.requester(),
            last_timestamp_us: AtomicU64::new(0),
        });
        Vl53lxDevice {
            dev,
            fixed_product,
            streams,
            platform,
            bound: Arc::new(Mutex::new(None)),
            board_name: Mutex::new(None),
            ranging: AtomicBool::new(false),
        }
    }

    // ── identity ─────────────────────────────────────────────────────────────

    /// The board's device name, e.g. `DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523` (asked once).
    pub fn board_name(&self) -> Result<String> {
        let mut name = self.board_name.lock().unwrap();
        if name.is_none() {
            *name = Some(self.get_device_name()?);
        }
        Ok(name.clone().unwrap_or_default())
    }

    /// The product the device name carries, or `None` on an unstamped board.
    pub fn detected(&self) -> Result<Option<&'static str>> {
        Ok(product_from_board_name(&self.board_name()?))
    }

    /// The product [`Vl53lxDevice::init`] bound (`None` before).
    pub fn product(&self) -> Option<&'static str> {
        self.bound().map(|b| b.product)
    }

    pub fn driver_kind(&self) -> Option<&'static str> {
        self.bound().map(|b| b.kind)
    }

    // (product, driver kind) as init binds them, read where the device is not at hand (a recorder's
    // measurement hook).
    pub(crate) fn bound_pair(
        &self,
    ) -> impl Fn() -> Option<(&'static str, &'static str)> + Send + Sync + 'static {
        let bound = self.bound.clone();
        move || bound.lock().unwrap().map(|b| (b.product, b.kind))
    }

    pub fn caveat(&self) -> Option<&'static str> {
        self.bound().and_then(|b| b.caveat)
    }

    /// The ULD port in use — escape hatch for product-specific calls: `f` runs with the driver (do
    /// not call this device's driver-backed methods from `f`: the driver is held).
    /// [`DepzError::InvalidState`] before [`Vl53lxDevice::init`].
    pub fn with_driver<R>(&self, f: impl FnOnce(&mut dyn Vl53lxDriver) -> Result<R>) -> Result<R> {
        let mut g = self
            .streams
            .driver
            .lock()
            .unwrap_or_else(|p| p.into_inner());
        let r = match g.as_deref_mut() {
            Some(d) => f(d),
            None => Err(DepzError::InvalidState("call init() first".into())),
        };
        self.streams.release(g);
        r
    }

    pub fn initialized(&self) -> bool {
        self.streams
            .driver
            .lock()
            .unwrap_or_else(|p| p.into_inner())
            .is_some()
    }

    pub fn ranging(&self) -> bool {
        self.ranging.load(Ordering::SeqCst)
    }

    /// The driver kinds a product has (`None`: this board's; empty when it carries no product).
    pub fn driver_kinds(&self, product: Option<&str>) -> Result<Vec<&'static str>> {
        match self.product_name(product)? {
            Some(name) => driver_kinds(name),
            None => Ok(Vec::new()),
        }
    }

    // ── lifecycle ────────────────────────────────────────────────────────────

    /// Bind the (product, driver kind) pair and initialise the sensor. `product` `None`: this
    /// class's, else the board name's; `driver` `None`: the product's first kind. A pair the table
    /// lacks fails with [`DepzError::Unsupported`] naming what the product has.
    pub fn init(&self, driver: Option<&str>, product: Option<&str>) -> Result<()> {
        self.require_not_ranging()?;
        let Some(name) = self.product_name(product)? else {
            return Err(DepzError::InvalidState(format!(
                "board '{}' carries no product number - pass product explicitly",
                self.board_name()?
            )));
        };
        let kind = match driver {
            Some(k) => k,
            None => *driver_kinds(name)?.first().ok_or_else(|| {
                DepzError::Unsupported(format!("{name} has no driver this SDK runs yet"))
            })?,
        };
        let (mut drv, caveat) = create_driver(name, kind, self.platform.clone())?;
        // Sticky on the bridge and 2 after a reset: set before the first register access.
        self.platform.set_addr_width(drv.addr_width())?;
        self.sensor_init(drv.as_mut())?;
        let product = product_row(name).map_or("", |p| p.name);
        let kind = DriverKind::from_name(kind).map_or("", |k| k.as_str());
        let mut g = self
            .streams
            .driver
            .lock()
            .unwrap_or_else(|p| p.into_inner());
        *g = Some(drv);
        *self.bound.lock().unwrap() = Some(Bound {
            product,
            kind,
            caveat,
        });
        self.streams.release(g);
        Ok(())
    }

    // A reset makes the die NACK for a moment; the driver waits those out, and they are no bus fault.
    fn sensor_init(&self, drv: &mut dyn Vl53lxDriver) -> Result<()> {
        drv.sensor_init()?;
        self.platform.clear_i2c_errors()
    }

    /// Everything about what is connected (needs [`Vl53lxDevice::init`]).
    pub fn identify_product(&self) -> Result<Vl53lxProductInfo> {
        let b = self.bound_or_err()?;
        let board = self.board_name()?;
        let detected = product_from_board_name(&board);
        let rated = product_row(detected.unwrap_or(b.product));
        let kinds = driver_kinds(b.product)?;
        let model_id_row = product_row(b.product);
        self.with_driver(|d| {
            let model_id = d.model_id()?;
            Ok(Vl53lxProductInfo {
                board,
                detected,
                product: b.product,
                driver: b.kind,
                driver_class: d.driver_name(),
                kinds,
                model_id,
                model_id_ok: model_id_row.is_some_and(|p| p.model_id_ok(model_id)),
                supports: d.supports().to_vec(),
                modes: d.modes(),
                reach_mm: rated.map(|p| p.reach_mm),
                driver_reach_mm: d.reach_mm()?,
                budget_ms: d.budget_ms(),
                budget_choices: d.budget_choices(),
                histogram: d.histogram(),
                max_khz: d.max_khz(),
                caveat: b.caveat,
            })
        })
    }

    /// The lines a UI shows about the chosen pair: a product named by hand, a borrowed driver, a pair
    /// that reaches less than the board is rated for, the driver's caveat.
    pub fn notes(&self) -> Result<Vec<String>> {
        let b = self.bound_or_err()?;
        let reach = self.with_driver(|d| d.reach_mm())?;
        let det = self.detected()?;
        let (product, kind) = (b.product, b.kind);
        let mut out = Vec::new();
        match det {
            None => out.push(format!(
                "{product}: board name carries no product number, named by hand"
            )),
            Some(d) if d != product => out.push(format!(
                "{product}: borrowing this driver - the board says it is a {d}"
            )),
            _ => {}
        }
        if let (Some(row), Some(reach)) = (product_row(det.unwrap_or(product)), reach) {
            if (reach as i64) < row.reach_mm as i64 {
                out.push(format!(
                    "{product}/{kind}: this pair reaches {reach} mm, the board is rated {} mm - past {reach} mm the \
                     phase is out of the valid window and frames come back with status 4",
                    row.reach_mm
                ));
            }
        }
        if let Some(c) = b.caveat {
            out.push(format!("{product}/{kind}: {c}"));
        }
        Ok(out)
    }

    /// Whether this product / driver serves an optional group.
    pub fn supports(&self, group: &str) -> Result<bool> {
        self.with_driver(|d| Ok(d.supports().contains(&group)))
    }

    /// Named ranging modes, the first being what init leaves; empty if none.
    pub fn modes(&self) -> Result<Vec<&'static str>> {
        self.with_driver(|d| Ok(d.modes()))
    }

    /// The (min, max) budget the driver offers, ms.
    pub fn budget_range(&self) -> Result<(u32, u32)> {
        self.with_driver(|d| Ok(d.budget_ms()))
    }

    /// How far the loaded configuration can measure, mm, or `None`.
    pub fn driver_reach_mm(&self) -> Result<Option<i32>> {
        self.with_driver(|d| d.reach_mm())
    }

    // ── sensor power and the bridge ──────────────────────────────────────────

    /// Drive XSHUT: off (0) / on (1) / reset (2). Off and reset stop the stream; the sensor then holds
    /// none of the configuration — [`Vl53lxDevice::init`] again.
    pub fn xshut(&self, action: u8) -> Result<()> {
        if action > XSHUT_RESET {
            return Err(DepzError::InvalidArgument(format!(
                "XSHUT action must be 0 (off), 1 (on) or 2 (reset), got {action}"
            )));
        }
        self.request_ok(
            Vl53lxCmd::Xshut as u8,
            &pack_xshut(action),
            Some(XSHUT_TIMEOUT),
        )?;
        self.ranging.store(false, Ordering::SeqCst);
        let mut g = self
            .streams
            .driver
            .lock()
            .unwrap_or_else(|p| p.into_inner());
        *g = None;
        self.streams.release(g);
        Ok(())
    }

    /// RPT_VL53_INFO — the bridge's own counters and settings; safe while streaming.
    pub fn bridge_info(&self) -> Result<Vl53lxInfo> {
        self.request(
            Vl53lxCmd::GetInfo as u8,
            &[],
            expect_report(Vl53lxRpt::Info as u8, Vl53lxInfo::unpack),
            None,
        )
    }

    // ── configuration (init first; not while ranging) ────────────────────────

    /// Re-initialise the sensor and apply a ranging configuration. The re-init is deliberate: it is
    /// the only way to know what the configuration registers hold. `mode` goes on before the budget;
    /// `offset_mm` / `xtalk_kcps` re-apply a stored calibration; `signal_kcps` replaces the blob's
    /// signal threshold (each `None`: leave it). `inter_ms` 0 = continuous. An unsupported group is
    /// refused before the re-init.
    pub fn configure(
        &self,
        budget_ms: u32,
        inter_ms: u32,
        mode: Option<&str>,
        offset_mm: Option<i32>,
        xtalk_kcps: Option<i32>,
        signal_kcps: Option<u32>,
    ) -> Result<()> {
        self.require_not_ranging()?;
        let b = self.bound_or_err()?;
        self.with_driver(|d| {
            let groups = [
                ("mode", mode.is_some()),
                ("offset", offset_mm.is_some()),
                ("xtalk", xtalk_kcps.is_some()),
                ("signal_thresh", signal_kcps.is_some()),
            ];
            for (group, wanted) in groups {
                if wanted {
                    need(d, b, group)?;
                }
            }
            self.sensor_init(d)?;
            if let Some(m) = mode {
                d.set_mode(m)?;
            }
            d.set_range_timing(budget_ms, inter_ms)?;
            if let Some(o) = offset_mm {
                d.set_offset(o)?;
            }
            if let Some(x) = xtalk_kcps {
                d.set_xtalk(x)?;
            }
            if let Some(s) = signal_kcps {
                d.set_signal_threshold(s)?;
            }
            Ok(())
        })
    }

    /// (timing budget ms, inter-measurement ms) read back from the sensor; 0 = continuous.
    pub fn get_range_timing(&self) -> Result<(u32, u32)> {
        self.with_driver(|d| d.get_range_timing())
    }

    /// The mode in use, or `None` on a product without modes.
    pub fn get_mode(&self) -> Result<Option<String>> {
        self.with_driver(|d| {
            if d.supports().contains(&"mode") {
                d.get_mode().map(Some)
            } else {
                Ok(None)
            }
        })
    }

    /// The only budgets accepted right now, or empty when any in [`Vl53lxDevice::budget_range`] will do.
    pub fn budget_choices(&self) -> Result<Vec<u32>> {
        self.with_driver(|d| Ok(d.budget_choices()))
    }

    /// The nearest budget this product accepts.
    pub fn snap_budget(&self, budget_ms: u32) -> Result<u32> {
        self.with_driver(|d| {
            let choices = d.budget_choices();
            if let Some(&first) = choices.first() {
                let dist = |c: u32| (c as i64 - budget_ms as i64).abs();
                // The first of equally near choices wins.
                return Ok(choices.iter().copied().fold(first, |best, c| {
                    if dist(c) < dist(best) {
                        c
                    } else {
                        best
                    }
                }));
            }
            let (lo, hi) = d.budget_ms();
            Ok(budget_ms.clamp(lo, hi))
        })
    }

    pub fn get_offset_mm(&self) -> Result<i32> {
        self.with_need("offset", |d| d.get_offset())
    }

    pub fn set_offset_mm(&self, offset_mm: i32) -> Result<()> {
        self.require_not_ranging()?;
        self.with_need("offset", |d| d.set_offset(offset_mm))
    }

    pub fn get_xtalk_kcps(&self) -> Result<i32> {
        self.with_need("xtalk", |d| d.get_xtalk())
    }

    pub fn set_xtalk_kcps(&self, xtalk_kcps: i32) -> Result<()> {
        self.require_not_ranging()?;
        self.with_need("xtalk", |d| d.set_xtalk(xtalk_kcps))
    }

    /// Offset calibration against a flat target; returns the offset programmed (store it on the host).
    /// `nb_samples` `None`: the driver's default.
    pub fn calibrate_offset(&self, target_dist_mm: i32, nb_samples: Option<u32>) -> Result<i32> {
        self.require_not_ranging()?;
        self.with_need("calib_offset", |d| {
            d.calibrate_offset(target_dist_mm, nb_samples)
        })
    }

    /// Crosstalk calibration; returns the xtalk programmed, kcps (store it on the host).
    pub fn calibrate_xtalk(&self, target_dist_mm: i32, nb_samples: Option<u32>) -> Result<i32> {
        self.require_not_ranging()?;
        self.with_need("calib_xtalk", |d| {
            d.calibrate_xtalk(target_dist_mm, nb_samples)
        })
    }

    pub fn get_detection_thresholds(&self) -> Result<Thresholds> {
        self.with_need("thresholds", |d| d.get_detection_thresholds())
    }

    /// The distance-window interrupt; stays armed until the next init / configure.
    pub fn set_detection_thresholds(
        &self,
        distance_low_mm: u16,
        distance_high_mm: u16,
        window: u8,
    ) -> Result<()> {
        self.require_not_ranging()?;
        self.with_need("thresholds", |d| {
            d.set_detection_thresholds(distance_low_mm, distance_high_mm, window)
        })
    }

    pub fn get_signal_threshold_kcps(&self) -> Result<u32> {
        self.with_need("signal_thresh", |d| d.get_signal_threshold())
    }

    pub fn set_signal_threshold_kcps(&self, signal_kcps: u32) -> Result<()> {
        self.require_not_ranging()?;
        self.with_need("signal_thresh", |d| d.set_signal_threshold(signal_kcps))
    }

    pub fn get_sigma_threshold_mm(&self) -> Result<u32> {
        self.with_need("sigma_thresh", |d| d.get_sigma_threshold())
    }

    pub fn set_sigma_threshold_mm(&self, sigma_mm: u32) -> Result<()> {
        self.require_not_ranging()?;
        self.with_need("sigma_thresh", |d| d.set_sigma_threshold(sigma_mm))
    }

    /// (x, y) SPADs.
    pub fn get_roi(&self) -> Result<(u8, u8)> {
        self.with_need("roi", |d| d.get_roi())
    }

    pub fn set_roi(&self, x: u8, y: u8) -> Result<()> {
        self.require_not_ranging()?;
        self.with_need("roi", |d| d.set_roi(x, y))
    }

    pub fn get_roi_center(&self) -> Result<u8> {
        self.with_need("roi", |d| d.get_roi_center())
    }

    pub fn set_roi_center(&self, center_spad: u8) -> Result<()> {
        self.require_not_ranging()?;
        self.with_need("roi", |d| d.set_roi_center(center_spad))
    }

    /// Re-run VHV after an ambient change over 8 °C.
    pub fn start_temperature_update(&self) -> Result<()> {
        self.require_not_ranging()?;
        self.with_need("temp_update", |d| d.start_temperature_update())
    }

    /// VL53L0X: re-measure the reference SPADs → (count, is_aperture).
    pub fn perform_ref_spad_management(&self) -> Result<(u32, bool)> {
        self.require_not_ranging()?;
        self.with_need("refspad", |d| d.perform_ref_spad_management())
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    /// Start the sensor and arm the board's stream: one report per INT edge, followed on the MCU by
    /// the driver's interrupt-release writes.
    pub fn start_ranging(&self) -> Result<()> {
        self.require_not_ranging()?;
        // The stream is armed once the driver is given back: its first frame must not wait for it.
        let (addr, len, clear) = self.with_driver(|d| {
            d.start_ranging()?;
            let (addr, len) = d.stream_block();
            Ok((addr, len, d.clear_steps()))
        })?;
        let started = pack_start_stream(addr, len, &clear, 0)
            .map_err(DepzError::from)
            .and_then(|payload| self.request_ok(Vl53lxCmd::StartStream as u8, &payload, None));
        if let Err(e) = started {
            // The sensor ranges but the host will say it does not: stop it again, or
            // stop_ranging() could never reach it. The START_STREAM error is the one the caller
            // gets, even if this stop fails too.
            let _ = self.with_driver(|d| d.stop_ranging());
            return Err(e);
        }
        self.ranging.store(true, Ordering::SeqCst);
        Ok(())
    }

    pub fn stop_ranging(&self) -> Result<()> {
        if !self.ranging() {
            return Ok(());
        }
        // Leave the host state "not ranging" and stop the sensor even when the STOP_STREAM ack fails,
        // or no reconfiguration would ever be possible again.
        let r = self.request_ok(Vl53lxCmd::StopStream as u8, &[], None);
        self.ranging.store(false, Ordering::SeqCst);
        let s = self.with_driver(|d| d.stop_ranging());
        r.and(s)
    }

    /// One polled measurement: start, data ready, read, release the interrupt, stop. Not while
    /// streaming. `timeout` `None`: 1 s.
    pub fn measure_once(&self, timeout: Option<Duration>) -> Result<Vl53lxMeasurement> {
        self.require_not_ranging()?;
        let (r, bins) = self.with_driver(|d| {
            d.start_ranging()?;
            let got = (|| -> Result<(Vl53lxRange, Option<BinData>)> {
                d.wait_data_ready(timeout.unwrap_or(Duration::from_secs(1)))?;
                let got = if d.histogram() {
                    let bins = d.bin_data(None)?;
                    (d.to_measurement(&bins)?, Some(bins))
                } else {
                    (d.read_measurement()?, None)
                };
                d.clear_interrupt()?;
                Ok(got)
            })();
            let stop = d.stop_ranging();
            let got = got?;
            stop?;
            Ok(got)
        })?;
        Ok(Vl53lxMeasurement::from(
            self.platform.last_timestamp_us.load(Ordering::Relaxed),
            r,
            bins,
        ))
    }

    /// Call `callback` for every streamed measurement (reader thread, or the thread of a call that
    /// held the driver while the frame came in).
    pub fn on_measurement(
        &self,
        callback: impl Fn(&Vl53lxMeasurement) + Send + Sync + 'static,
    ) -> Hook {
        self.streams.measurements.on_item(callback)
    }

    /// A pull queue of streamed measurements (bounded, drop-oldest).
    pub fn measurements(&self, capacity: usize) -> Subscription<Vl53lxMeasurement> {
        self.streams.measurements.subscribe(capacity)
    }

    /// The next streamed measurement (contract 07 §3); `timeout` `None`: 2 s.
    pub fn get_measurement(&self, timeout: Option<Duration>) -> Result<Vl53lxMeasurement> {
        self.streams.measurements.next(
            timeout.unwrap_or(Duration::from_secs(2)),
            Vl53lxRpt::Stream as u8,
            self.is_closed(),
        )
    }

    /// Stream reports dropped because the driver did not decode them.
    pub fn stream_parse_errors(&self) -> u64 {
        self.streams.parse_errors.load(Ordering::Relaxed)
    }

    pub fn stream_dropped_counts(&self) -> Vec<u64> {
        self.streams.measurements.dropped_counts()
    }

    // ── internal ─────────────────────────────────────────────────────────────

    fn bound(&self) -> Option<Bound> {
        *self.bound.lock().unwrap()
    }

    fn bound_or_err(&self) -> Result<Bound> {
        if !self.initialized() {
            return Err(DepzError::InvalidState("call init() first".into()));
        }
        self.bound()
            .ok_or_else(|| DepzError::InvalidState("call init() first".into()))
    }

    // `product`, else this class's, else the board name's.
    fn product_name<'a>(&self, product: Option<&'a str>) -> Result<Option<&'a str>> {
        Ok(match (product, self.fixed_product) {
            (Some(p), _) => Some(p),
            (None, Some(f)) => Some(f),
            (None, None) => self.detected()?,
        })
    }

    fn with_need<R>(
        &self,
        group: &str,
        f: impl FnOnce(&mut dyn Vl53lxDriver) -> Result<R>,
    ) -> Result<R> {
        let b = self.bound_or_err()?;
        self.with_driver(|d| {
            need(d, b, group)?;
            f(d)
        })
    }

    fn require_not_ranging(&self) -> Result<()> {
        if self.ranging() {
            return Err(DepzError::InvalidState(
                "stop_ranging() first — the stream owns the bus".into(),
            ));
        }
        Ok(())
    }
}

fn need(d: &dyn Vl53lxDriver, b: Bound, group: &str) -> Result<()> {
    if d.supports().contains(&group) {
        return Ok(());
    }
    Err(DepzError::Unsupported(format!(
        "{}/{} does not support '{group}'{}",
        b.product,
        b.kind,
        b.caveat.map_or(String::new(), |c| format!(" ({c})"))
    )))
}

// The per-product classes: the family class with the product fixed (Java's subclasses).
macro_rules! deref_to_family {
    ($($name:ident),*) => {
        $(
            impl Deref for $name {
                type Target = Vl53lxDevice;

                fn deref(&self) -> &Vl53lxDevice {
                    &self.0
                }
            }
        )*
    };
}

deref_to_family!(
    Vl53l0xDevice,
    Vl53l1cxDevice,
    Vl53l1cbDevice,
    Vl53l3cxDevice,
    Vl53l4cxDevice
);

/// VL53L0X (2 m, 1-byte register addresses, 400 kHz): ULD API 1.0.4 with ranging profiles.
pub struct Vl53l0xDevice(Vl53lxDevice);

impl Vl53l0xDevice {
    /// Continue the open connection `dev` as a VL53L0X.
    pub fn attach(dev: DepzDevice) -> Vl53l0xDevice {
        Vl53l0xDevice(Vl53lxDevice::attach_as(dev, Some("VL53L0X")))
    }

    /// The family class underneath (the product stays fixed).
    pub fn into_inner(self) -> Vl53lxDevice {
        self.0
    }
}

/// VL53L1CX (4 m): the VL53L1X ULD (short / long) or the histogram driver.
pub struct Vl53l1cxDevice(Vl53lxDevice);

impl Vl53l1cxDevice {
    /// Continue the open connection `dev` as a VL53L1CX.
    pub fn attach(dev: DepzDevice) -> Vl53l1cxDevice {
        Vl53l1cxDevice(Vl53lxDevice::attach_as(dev, Some("VL53L1CX")))
    }

    /// The family class underneath (the product stays fixed).
    pub fn into_inner(self) -> Vl53lxDevice {
        self.0
    }
}

/// VL53L1CB (8 m, cover-glass module): the VL53L1CX die and drivers; usually needs a crosstalk
/// calibration.
pub struct Vl53l1cbDevice(Vl53lxDevice);

impl Vl53l1cbDevice {
    /// Continue the open connection `dev` as a VL53L1CB.
    pub fn attach(dev: DepzDevice) -> Vl53l1cbDevice {
        Vl53l1cbDevice(Vl53lxDevice::attach_as(dev, Some("VL53L1CB")))
    }

    /// The family class underneath (the product stays fixed).
    pub fn into_inner(self) -> Vl53lxDevice {
        self.0
    }
}

/// VL53L3CX (3 m): ST's ULP driver (single target) or the histogram driver (several per frame).
pub struct Vl53l3cxDevice(Vl53lxDevice);

impl Vl53l3cxDevice {
    /// Continue the open connection `dev` as a VL53L3CX.
    pub fn attach(dev: DepzDevice) -> Vl53l3cxDevice {
        Vl53l3cxDevice(Vl53lxDevice::attach_as(dev, Some("VL53L3CX")))
    }

    /// The family class underneath (the product stays fixed).
    pub fn into_inner(self) -> Vl53lxDevice {
        self.0
    }
}

/// VL53L4CX (6 m): the histogram driver only — to run it light, borrow a sibling's with
/// `init(Some("uld"), Some("VL53L4CD"))`.
pub struct Vl53l4cxDevice(Vl53lxDevice);

impl Vl53l4cxDevice {
    /// Continue the open connection `dev` as a VL53L4CX.
    pub fn attach(dev: DepzDevice) -> Vl53l4cxDevice {
        Vl53l4cxDevice(Vl53lxDevice::attach_as(dev, Some("VL53L4CX")))
    }

    /// The family class underneath (the product stays fixed).
    pub fn into_inner(self) -> Vl53lxDevice {
        self.0
    }
}
