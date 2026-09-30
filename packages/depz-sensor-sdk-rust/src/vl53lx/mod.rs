//! VL53L 1D ToF family decode layer (contract 12, protocol v2.00, a delta
//! against contract 10).
//!
//! One bridge firmware (`APP_VL53L0_4_v*`, spec name `APP_VL53LX_v*`) serves
//! six products — **VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CD,
//! VL53L4CX** — and knows no sensor: every ULD runs on the host.
//!
//! - [`codecs`] — the v2.00 wire codecs (`VL53_SET_ADDR_WIDTH`,
//!   `VL53_START_STREAM` with its interrupt-release list, the 23-byte
//!   `RPT_VL53_INFO`); the unchanged contract-10 codecs are re-exported here.
//! - [`products`] — the product table (per product/driver bridge parameters)
//!   and board → product → class resolution.
//! - [`decode`] — stateless decode of the three streamed blocks: the die
//!   result block (`l4` / `l1` ULD variants), VL53L0X raw fields, histogram
//!   status bytes + 24 bins.
//!
//! The live drivers (init, configuration, histogram target extraction) are a
//! documented extension point, out of scope for this decode-layer crate.

pub mod codecs;
pub mod decode;
pub mod products;

pub use crate::vl53l4::{
    i2c_error_name, pack_read_reg, pack_set_i2c_speed, pack_write_reg, pack_xshut, RegData,
    StreamData, I2C_KHZ_STEPS, SF_INT_ACT_HIGH, XFER_MAX, XSHUT_OFF, XSHUT_ON, XSHUT_RESET,
};
pub use codecs::{
    pack_set_addr_width, pack_start_stream, Vl53lxCmd, Vl53lxInfo, Vl53lxRpt, CLEAR_STEPS_MAX,
    INFO_SIZE,
};
pub use decode::{
    decode_die_block, decode_histogram_raw, decode_l0x_raw, DieResult, DieVariant, HistogramRaw,
    L0xRaw, ShortBlock, DIE_BLOCK_ADDR, DIE_BLOCK_LEN, HISTOGRAM_BINS, HISTOGRAM_BLOCK_ADDR,
    HISTOGRAM_BLOCK_LEN, L0X_BLOCK_ADDR, L0X_BLOCK_LEN,
};
pub use products::{
    product, product_from_board_name, resolve_class, resolve_product, BusParams, DriverKind,
    Product, Vl53lxClass, DRIVER_KINDS, PRODUCTS,
};
