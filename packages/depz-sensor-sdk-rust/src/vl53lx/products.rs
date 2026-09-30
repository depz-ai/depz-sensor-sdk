//! The 1D-family product table and class resolution (contract 12 §1, pinned by
//! `vectors/vl53lx.json` `products` and `model`).
//!
//! Two independent axes: the **product** (whose parameter set to load — normally
//! what is soldered on the board, but naming a neighbour borrows its driver) and
//! the **driver kind** (`uld`, `ulp`, `histogram`). A product/driver pair the
//! table does not list is a refusal, never a fallback. The model id is a
//! cross-check only: L1CX/L1CB share `0xEACC`, L4CD/L4CX share `0xEBAA`.

use super::decode::{
    DieVariant, DIE_BLOCK_ADDR, DIE_BLOCK_LEN, HISTOGRAM_BLOCK_ADDR, HISTOGRAM_BLOCK_LEN,
    L0X_BLOCK_ADDR, L0X_BLOCK_LEN,
};

/// The three kinds of driver ST ships for this family, in UI order.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DriverKind {
    /// Ultra Lite Driver — the die computes the distance.
    Uld,
    /// Ultra Low Power (VL53L3CX only).
    Ulp,
    /// ST's Bare Driver — the die hands over 24 photon bins, the host finds
    /// up to four targets.
    Histogram,
}

impl DriverKind {
    /// Lowercase wire-string form (matches the golden vectors).
    pub fn as_str(&self) -> &'static str {
        match self {
            DriverKind::Uld => "uld",
            DriverKind::Ulp => "ulp",
            DriverKind::Histogram => "histogram",
        }
    }

    pub fn from_name(s: &str) -> Option<DriverKind> {
        match s {
            "uld" => Some(DriverKind::Uld),
            "ulp" => Some(DriverKind::Ulp),
            "histogram" => Some(DriverKind::Histogram),
            _ => None,
        }
    }
}

/// Every driver kind, in UI order.
pub const DRIVER_KINDS: [DriverKind; 3] = [DriverKind::Uld, DriverKind::Ulp, DriverKind::Histogram];

/// The bridge parameters one product/driver pair runs with (contract 12 §1, §3).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
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

const CLEAR_DIE: &[(u16, u8)] = &[(0x0086, 0x01)];
const CLEAR_L0X: &[(u16, u8)] = &[(0x0B, 0x01), (0x0B, 0x00)];

const L0X_ULD: BusParams = BusParams {
    addr_width: 1,
    clear_steps: CLEAR_L0X,
    max_khz: 400,
    block_addr: L0X_BLOCK_ADDR,
    block_len: L0X_BLOCK_LEN as u16,
    die_variant: None,
};

const fn die(variant: DieVariant) -> BusParams {
    BusParams {
        addr_width: 2,
        clear_steps: CLEAR_DIE,
        max_khz: 1000,
        block_addr: DIE_BLOCK_ADDR,
        block_len: DIE_BLOCK_LEN as u16,
        die_variant: Some(variant),
    }
}

const HISTOGRAM: BusParams = BusParams {
    addr_width: 2,
    clear_steps: CLEAR_DIE,
    max_khz: 1000,
    block_addr: HISTOGRAM_BLOCK_ADDR,
    block_len: HISTOGRAM_BLOCK_LEN as u16,
    die_variant: None,
};

/// One row of the product table.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
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

impl Product {
    /// The driver kinds this product has, in [`DRIVER_KINDS`] order.
    pub fn driver_kinds(&self) -> Vec<DriverKind> {
        DRIVER_KINDS
            .iter()
            .copied()
            .filter(|k| self.drivers.iter().any(|(d, _)| d == k))
            .collect()
    }

    /// Bridge parameters of one product/driver pair; `None` for a pair the
    /// table has no row for (a refusal, never a fallback).
    pub fn bus_params(&self, kind: DriverKind) -> Option<BusParams> {
        self.drivers.iter().find(|(d, _)| *d == kind).map(|(_, p)| *p)
    }

    /// Bridge parameters of [`Product::default_driver`].
    pub fn default_bus_params(&self) -> BusParams {
        self.bus_params(self.default_driver).expect("default driver is in the table")
    }

    /// Cross-check a model id the sensor answered. `true` only says "not
    /// something else entirely": the pairs that share an id can't be told apart.
    pub fn model_id_ok(&self, value: u16) -> bool {
        self.model_id == value
    }
}

/// Every product of the family, in UI order.
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
        drivers: &[(DriverKind::Uld, die(DieVariant::L1)), (DriverKind::Histogram, HISTOGRAM)],
    },
    Product {
        name: "VL53L1CB",
        usb_pid: 0xED42,
        model_id: 0xEACC,
        reach_mm: 8000,
        default_driver: DriverKind::Uld,
        drivers: &[(DriverKind::Uld, die(DieVariant::L1)), (DriverKind::Histogram, HISTOGRAM)],
    },
    Product {
        name: "VL53L3CX",
        usb_pid: 0xED44,
        model_id: 0xEAAA,
        reach_mm: 3000,
        default_driver: DriverKind::Ulp,
        drivers: &[(DriverKind::Ulp, die(DieVariant::L4)), (DriverKind::Histogram, HISTOGRAM)],
    },
    Product {
        name: "VL53L4CD",
        usb_pid: 0xED45,
        model_id: 0xEBAA,
        reach_mm: 1200,
        default_driver: DriverKind::Uld,
        drivers: &[(DriverKind::Uld, die(DieVariant::L4)), (DriverKind::Histogram, HISTOGRAM)],
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

/// Table row by part number (case-sensitive, uppercase), `None` if unserved.
pub fn product(name: &str) -> Option<&'static Product> {
    PRODUCTS.iter().find(|p| p.name == name)
}

/// `"ToF Sensor VL53L4CD USB v2.1"` → `"VL53L4CD"`. Matches the first
/// `VL53L<digit>[A-Z0-9]*` in the upper-cased name; `None` when there is none
/// or it is not a family product (an unstamped board, or an unknown name).
pub fn product_from_board_name(name: &str) -> Option<&'static str> {
    let upper = name.to_ascii_uppercase();
    let b = upper.as_bytes();
    const PREFIX: &[u8] = b"VL53L";
    let mut i = 0;
    while i + PREFIX.len() < b.len() {
        if &b[i..i + PREFIX.len()] == PREFIX && b[i + PREFIX.len()].is_ascii_digit() {
            let start = i;
            let mut end = i + PREFIX.len() + 1;
            while end < b.len() && (b[end].is_ascii_uppercase() || b[end].is_ascii_digit()) {
                end += 1;
            }
            return product(&upper[start..end]).map(|p| p.name);
        }
        i += 1;
    }
    None
}

/// The sensor class an `APP_VL53L0_4` board opens as.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
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

impl Vl53lxClass {
    /// Class name (matches the golden vectors).
    pub fn as_str(&self) -> &'static str {
        match self {
            Vl53lxClass::Vl53l0x => "Vl53l0x",
            Vl53lxClass::Vl53l1cx => "Vl53l1cx",
            Vl53lxClass::Vl53l1cb => "Vl53l1cb",
            Vl53lxClass::Vl53l3cx => "Vl53l3cx",
            Vl53lxClass::Vl53l4cx => "Vl53l4cx",
            Vl53lxClass::Vl53lx => "Vl53lx",
        }
    }

    fn for_product(product: Option<&str>) -> Vl53lxClass {
        match product {
            Some("VL53L0X") => Vl53lxClass::Vl53l0x,
            Some("VL53L1CX") => Vl53lxClass::Vl53l1cx,
            Some("VL53L1CB") => Vl53lxClass::Vl53l1cb,
            Some("VL53L3CX") => Vl53lxClass::Vl53l3cx,
            Some("VL53L4CX") => Vl53lxClass::Vl53l4cx,
            _ => Vl53lxClass::Vl53lx,
        }
    }
}

/// The product a board carries: the production USB PID model (`usb_model`,
/// e.g. from [`crate::usb_model_hint`]) when it names a family product, else
/// the device-name product ([`product_from_board_name`]), else `None`.
pub fn resolve_product(usb_model: Option<&str>, device_name: &str) -> Option<&'static str> {
    usb_model
        .and_then(|m| product(&m.to_ascii_uppercase()))
        .map(|p| p.name)
        .or_else(|| product_from_board_name(device_name))
}

/// Resolve the 1D-family class (contract 12 §1, pinned by `vl53lx.json`
/// `model`): PID model if it is a family product, else the device-name
/// product, else the generic [`Vl53lxClass::Vl53lx`].
pub fn resolve_class(usb_model: Option<&str>, device_name: &str) -> Vl53lxClass {
    Vl53lxClass::for_product(resolve_product(usb_model, device_name))
}
