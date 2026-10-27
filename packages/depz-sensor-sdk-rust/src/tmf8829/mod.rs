//! TMF8829 multizone dToF decode layer (contract 14, protocol v1.00).
//!
//! The `APP_TMF8829_v*` firmware is a thin SPI register bridge: the host runs
//! the sensor (soft reset, sensor-firmware download through its bootloader,
//! config page, presets, start/stop) and the bridge only pushes self-sized
//! frames off the sensor FIFO in chunks.
//!
//! - [`codecs`] — the wire codecs: `TMF_*` command payloads (0x32..0x37,
//!   range-checked), the `RPT_TMF_REG_DATA` / `RPT_TMF_INFO` /
//!   `RPT_TMF_FRAME` / `RPT_TMF_EVENT` reports, and the chunk reassembler.
//! - [`frames`] — frame header/footer parse, problems, results-pixel decode
//!   and the 32x32/48x32 half-frame merge.
//! - [`clock`] — [`SensorClock`] + [`CaptureTimer`]: the integer clock model
//!   and frame capture time (bit-exact with the vectors).
//! - [`builder`] — frames in stream order → [`Tmf8829Measurement`].
//! - [`config`] — presets, preset `kilo_iterations`, config-page fields.
//! - [`firmware`] — the embedded ams sensor-firmware image (MIT).
//!
//! The live driver (init sequence, configure, start/stop over a link) is
//! `devices::Tmf8829Device`.

pub mod builder;
pub mod clock;
pub mod codecs;
pub mod config;
pub mod firmware;
pub mod frames;

pub use builder::{distance_mm, MeasurementBuilder, Tmf8829Measurement};
pub use clock::{
    flag_names, fp_subints, CaptureTimer, FrameTime, SensorClock, FT_CLOCK_RESET, FT_EDGE_LATE,
    FT_NOMINAL, FT_STALLED, FT_T1_UNSURE, NOMINAL_Q16,
};
pub use codecs::{
    pack_get_info, pack_read_reg, pack_start_stream, pack_stop_stream, pack_write_reg, FrameChunk,
    FrameReassembler, RawFrame, RegData, Tmf8829Cmd, Tmf8829Event, Tmf8829Info, Tmf8829Rpt,
    EV_BAD_HEADER, EV_INT, INT_HISTOGRAMS, INT_MASK_ALL, INT_MOTION, INT_PROXIMITY, INT_RESULTS,
    READ_MAX, SF_REF_FRAME, WRITE_MAX,
};
pub use config::{
    cfg_field, cfg_get, cfg_set, config_fields, preset_cmd, preset_kilo_iterations, CfgField,
    CFG_FIELDS, CFG_SIZE, DEFAULT_PERIOD_MS, PRESETS, PRESET_KILO_ITERATIONS, REG_CFG,
};
pub use firmware::{
    firmware_image, firmware_words, FIRMWARE_IMAGE, FIRMWARE_LICENSE, FIRMWARE_LOAD_ADDR,
};
pub use frames::{
    fp_grid, fp_name, frame_rows, frame_size, merge_halves, parse_frame, parse_results,
    parse_results_info, FrameError, FrameInfo, HistogramFrame, PixelResults, FID_HISTOGRAMS,
    FID_REF_SPAD, FID_RESULTS,
};
