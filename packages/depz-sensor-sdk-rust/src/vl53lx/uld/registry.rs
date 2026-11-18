//! The drivers behind the product table (`uld/registry.py`): which (product, kind) pairs exist, the
//! driver each one runs and what to tell the user about it. A missing pair is absent on purpose.

use std::sync::Arc;

use super::{Vl53l0xUldDriver, Vl53l1UldDriver, Vl53l3UlpDriver, Vl53l4UldDriver};
use crate::devices::{DepzError, Result};
use crate::vl53lx::bare::Vl53lxHistogramDriver;
use crate::vl53lx::driver::{Vl53lxDriver, Vl53lxPlatform};
use crate::vl53lx::products::{product as product_row, DRIVER_KINDS, PRODUCTS};

/// Builds a driver over a platform, bound for a product.
pub type DriverCtor = fn(Arc<dyn Vl53lxPlatform>, &str) -> Box<dyn Vl53lxDriver>;

/// What the UI says about every histogram pair.
pub const HIST_CAVEAT: &str =
    "the histogram driver has no calibrations and no detection thresholds - the light drivers are the ones with those";

const L0X_CAVEAT: &str = "no detection thresholds; there is no ROI on this die at all";
const L3_ULP_CAVEAT: &str =
    "single-target ranging only - the histogram driver gives several targets instead";

// (product, kind, constructor, caveat); the Java registration order.
const DRIVERS: &[(&str, &str, DriverCtor, Option<&str>)] = &[
    (
        "VL53L0X",
        "uld",
        |p, s| Box::new(Vl53l0xUldDriver::new(p, s)),
        Some(L0X_CAVEAT),
    ),
    (
        "VL53L1CX",
        "uld",
        |p, s| Box::new(Vl53l1UldDriver::new(p, s)),
        None,
    ),
    (
        "VL53L1CB",
        "uld",
        |p, s| Box::new(Vl53l1UldDriver::new(p, s)),
        None,
    ),
    (
        "VL53L3CX",
        "ulp",
        |p, s| Box::new(Vl53l3UlpDriver::new(p, s)),
        Some(L3_ULP_CAVEAT),
    ),
    (
        "VL53L4CD",
        "uld",
        |p, s| Box::new(Vl53l4UldDriver::new(p, s)),
        None,
    ),
    (
        "VL53L1CX",
        "histogram",
        |p, s| Box::new(Vl53lxHistogramDriver::new(p, s)),
        Some(HIST_CAVEAT),
    ),
    (
        "VL53L1CB",
        "histogram",
        |p, s| Box::new(Vl53lxHistogramDriver::new(p, s)),
        Some(HIST_CAVEAT),
    ),
    (
        "VL53L3CX",
        "histogram",
        |p, s| Box::new(Vl53lxHistogramDriver::new(p, s)),
        Some(HIST_CAVEAT),
    ),
    (
        "VL53L4CD",
        "histogram",
        |p, s| Box::new(Vl53lxHistogramDriver::new(p, s)),
        Some(HIST_CAVEAT),
    ),
    (
        "VL53L4CX",
        "histogram",
        |p, s| Box::new(Vl53lxHistogramDriver::new(p, s)),
        Some(HIST_CAVEAT),
    ),
];

fn check_product(product: &str) -> Result<()> {
    if product_row(product).is_none() {
        let served: Vec<&str> = PRODUCTS.iter().map(|p| p.name).collect();
        return Err(DepzError::Unsupported(format!(
            "no such product: {product} - served: {}",
            served.join(", ")
        )));
    }
    Ok(())
}

/// The driver kinds `product` has, in [`DRIVER_KINDS`] order; [`DepzError::Unsupported`] for no such
/// product. Empty for a product whose only kinds this crate does not run yet.
pub fn driver_kinds(product: &str) -> Result<Vec<&'static str>> {
    check_product(product)?;
    Ok(DRIVER_KINDS
        .iter()
        .map(|k| k.as_str())
        .filter(|k| {
            DRIVERS
                .iter()
                .any(|(p, kind, _, _)| *p == product && kind == k)
        })
        .collect())
}

/// The driver for one (product, kind) pair, and the caveat a UI shows about the pair (or `None`).
/// [`DepzError::Unsupported`] for a pair the table has no row for, naming the kinds the product has.
pub fn create_driver(
    product: &str,
    kind: &str,
    platform: Arc<dyn Vl53lxPlatform>,
) -> Result<(Box<dyn Vl53lxDriver>, Option<&'static str>)> {
    check_product(product)?;
    match DRIVERS
        .iter()
        .find(|(p, k, _, _)| *p == product && *k == kind)
    {
        Some((_, _, create, caveat)) => Ok((create(platform, product), *caveat)),
        None => Err(DepzError::Unsupported(format!(
            "{product} has no '{kind}' driver - it has {}",
            driver_kinds(product)?.join(", ")
        ))),
    }
}
