#!/usr/bin/env python3
"""VL53L8CH CNH (Compact Network Histogram) — Python port of the parts of
vl53lmz_plugin_cnh.c / vl53lmz_plugin_motion_indicator.c (VL53LMZ ULD 2.0.16,
tools/doc/VL53LMZ_ULD_v2.0.16) needed to configure CNH, compute the on-device
buffer size, and decode a captured CNH data block into per-aggregate histograms.

CNH is the reason to run the VL53L8CH firmware instead of plain VL53L8CX: the
sensor returns a per-aggregate distance histogram (the "compact network
histogram") on top of the normal ranging frame. The MCU streaming buffer is too
small to push a full CNH frame, so the host captures it in poll-mode (chunked
READ_REG) — see vl53l8cx_uld.VL53L8CX.start_ranging(cnh_data_size=...).

Register/struct layouts are a 1:1 port of the C plugin; do not "simplify" the
offset arithmetic. The CNH configuration here fixes the cnh_cfg flags to
DISABLE_PING_PONG | DISABLE_VARIANCE (+ ambient / xtalk / zero-invalid /
ref-residual), matching Example_12_cnh_data.c — the decode below assumes those.
"""

import struct

# ---- fundamental histogram characteristics (plugin_cnh.h) ----
CNH_PULSE_WIDTH_BIN = 10
CNH_BIN_WIDTH_MM    = 37.5348

# ---- DCI indexes ----
MI_CFG_DEV_IDX = 0xBFAC          # VL53LMZ_MI_CFG_DEV_IDX  (cnh_send_config target)
CNH_DATA_IDX   = 0xC048          # VL53LMZ_CNH_DATA_IDX    (output block index)

# ---- limits (plugin_cnh.h) ----
CNH_MAX_DATA_WORDS = 1540
CNH_MAX_DATA_BYTES = CNH_MAX_DATA_WORDS * 4    # 6160

# ---- cnh_cfg flags (plugin_motion_indicator.h) ----
MI_SFE_DISABLE_PING_PONG    = 0x01
MI_SFE_DISABLE_VARIANCE     = 0x02
MI_SFE_ENABLE_AMBIENT_LEVEL = 0x04
MI_SFE_ENABLE_XTALK_REMOVAL = 0x08
MI_SFE_ZERO_NON_VALID_BINS  = 0x10
MI_SFE_STORE_REF_RESIDUAL   = 0x20

MI_MAP_ID_LENGTH    = 64         # VL53LMZ_RESOLUTION_8X8
MI_INDICATOR_LENGTH = 32

# ---- persistent-data header layout (plugin_cnh.c) ----
CNH_PER_HEADER_WORDS        = 5          # CNH_PER_HEADER_BYTES / 4
CNH_PER_BUFFER_HEADER_WORDS = 2          # CNH_PER_BUFFER_HEADER_BYTES / 4
CNH_PER_HEADER_BUFFER_INFO_IDX        = 1
CNH_PER_HEADER_FLAGS_IDX              = 3
_BUFFER_INFO_WORDS_MASK     = 0xFFFF
_BUFFER_INFO_FLAGS_SHIFT    = 24
_BUFFER_INFO_NO_VARIANCE    = 0x01
MI_STATE_PING = 0


class CnhConfigError(Exception):
    pass


class CnhConfig:
    """Mirror of VL53LMZ_Motion_Configuration plus the helpers that fill it.
    Build with init_config()/create_agg_map(), check size with
    required_memory(), then pack() the 156-byte struct for cnh_send_config."""

    def __init__(self):
        self.ref_bin_offset = 0
        self.detection_threshold = 0
        self.extra_noise_sigma = 0
        self.null_den_clip_value = 0
        self.mem_update_mode = 0
        self.mem_update_choice = 0
        self.sum_span = 0
        self.feature_length = 0
        self.nb_of_aggregates = 0
        self.nb_of_temporal_accumulations = 1
        self.min_nb_for_global_detection = 0
        self.global_indicator_format_1 = 0
        self.global_indicator_format_2 = 0
        self.cnh_cfg = 0
        self.cnh_flex_shift = 0
        self.spare_3 = 0
        self.map_id = [-1] * MI_MAP_ID_LENGTH
        self.indicator_format_1 = [0] * MI_INDICATOR_LENGTH
        self.indicator_format_2 = [0] * MI_INDICATOR_LENGTH

    # ---- vl53lmz_cnh_init_config ----
    def init_config(self, start_bin, num_bins, sub_sample):
        """start_bin: first device-histogram bin; num_bins: CNH bins;
        sub_sample: bins of the device histogram summed per CNH bin."""
        self.ref_bin_offset = start_bin * 2048
        self.detection_threshold = 0
        self.extra_noise_sigma = 0
        self.null_den_clip_value = 0
        self.mem_update_mode = 0
        self.mem_update_choice = 0
        self.feature_length = num_bins & 0xFF
        self.sum_span = sub_sample & 0xFF
        self.nb_of_temporal_accumulations = 1
        self.min_nb_for_global_detection = 0
        self.global_indicator_format_1 = 0
        self.global_indicator_format_2 = 0
        self.cnh_cfg = (MI_SFE_DISABLE_PING_PONG |
                        MI_SFE_DISABLE_VARIANCE |
                        MI_SFE_ENABLE_AMBIENT_LEVEL |
                        MI_SFE_ENABLE_XTALK_REMOVAL |
                        MI_SFE_ZERO_NON_VALID_BINS |
                        MI_SFE_STORE_REF_RESIDUAL)
        self.cnh_flex_shift = 1
        self.spare_3 = 0

    # ---- vl53lmz_cnh_create_agg_map ----
    def create_agg_map(self, resolution, start_x, start_y,
                       merge_x, merge_y, cols, rows):
        """Map device zones to CNH aggregates. resolution: 16 (4x4) or 64 (8x8)
        — must match the value passed to set_resolution()."""
        self.map_id = [-1] * MI_MAP_ID_LENGTH
        zone_res = 4 if resolution == 16 else 8
        if (start_x + cols * merge_x) > zone_res \
                or (start_y + rows * merge_y) > zone_res:
            raise CnhConfigError('agg map exceeds zone grid')
        self.nb_of_aggregates = cols * rows
        for row in range(start_y, start_y + rows * merge_y):
            for col in range(start_x, start_x + cols * merge_x):
                i = row * zone_res + col
                agg_id = ((row - start_y) // merge_y) * cols \
                    + ((col - start_x) // merge_x)
                if 0 <= agg_id < MI_MAP_ID_LENGTH:
                    self.map_id[i] = agg_id
                else:
                    raise CnhConfigError('agg id out of range')

    # ---- vl53lmz_cnh_calc_required_memory ----
    def required_memory(self):
        """On-device CNH buffer size in bytes for this config. Raises if the
        config is blank or the size exceeds CNH_MAX_DATA_BYTES."""
        if self.nb_of_aggregates == 0:
            raise CnhConfigError('agg map not created')
        size = _calc_required_memory(self.cnh_cfg, self.nb_of_aggregates,
                                     self.feature_length)
        if size > CNH_MAX_DATA_BYTES:
            raise CnhConfigError(
                f'CNH needs {size} B > max {CNH_MAX_DATA_BYTES} B — '
                f'reduce aggregates or bins')
        return size

    # ---- vl53lmz_cnh_calc_min_max_distance ----
    def min_max_distance_mm(self):
        """(min, max) target distance, in mm, fully captured by the histogram."""
        const = (CNH_PULSE_WIDTH_BIN / 2.0) * CNH_BIN_WIDTH_MM
        start = (self.ref_bin_offset / 2048.0)
        first_center = (start + self.sum_span / 2.0) * CNH_BIN_WIDTH_MM
        last_center = (start + (self.feature_length - 1) * self.sum_span
                       + self.sum_span / 2.0) * CNH_BIN_WIDTH_MM
        return int(first_center + const), int(last_center - const)

    def bin_center_mm(self, bin_idx):
        """Distance (mm) at the centre of CNH histogram bin `bin_idx`."""
        start = self.ref_bin_offset / 2048.0
        return (start + bin_idx * self.sum_span
                + self.sum_span / 2.0) * CNH_BIN_WIDTH_MM

    # ---- pack the 156-byte VL53LMZ_Motion_Configuration struct ----
    def pack(self):
        return struct.pack(
            '<i3I12B64b32B32B',
            self.ref_bin_offset & 0xFFFFFFFF,
            self.detection_threshold, self.extra_noise_sigma,
            self.null_den_clip_value,
            self.mem_update_mode, self.mem_update_choice,
            self.sum_span, self.feature_length,
            self.nb_of_aggregates, self.nb_of_temporal_accumulations,
            self.min_nb_for_global_detection,
            self.global_indicator_format_1, self.global_indicator_format_2,
            self.cnh_cfg, self.cnh_flex_shift, self.spare_3,
            *self.map_id,
            *self.indicator_format_1,
            *self.indicator_format_2)


def _pingpong_size_in_words(option_flags, nb_agg, feat_length):
    """_cnh_get_pingpong_size_in_word, returned in 32-bit words."""
    agg_x_feat = nb_agg * feat_length
    size = CNH_PER_BUFFER_HEADER_WORDS * 4
    size += agg_x_feat * 4                  # FEAT_INT  (32b each)
    size += ((3 + agg_x_feat) // 4) * 4     # FEAT_FRAC (8b each, padded)
    size += nb_agg * 4                      # AMBIENT_INT
    size += ((3 + nb_agg) // 4) * 4         # AMBIENT_FRAC
    if (option_flags & MI_SFE_DISABLE_VARIANCE) == 0:
        size += agg_x_feat * 4              # VARIANCE_INT
        size += ((3 + agg_x_feat) // 4) * 4  # VARIANCE_FRAC
    return size // 4


def _calc_required_memory(option_flags, nb_agg, feat_length):
    """_cnh_calculate_required_memory, in bytes."""
    size = _pingpong_size_in_words(option_flags, nb_agg, feat_length) * 4
    if (option_flags & MI_SFE_DISABLE_PING_PONG) == 0:
        size *= 2
    size += CNH_PER_HEADER_WORDS * 4
    return size


# Default cnh_cfg flags used by CnhConfig.init_config (ping-pong + variance off).
_DEFAULT_CNH_CFG = (MI_SFE_DISABLE_PING_PONG | MI_SFE_DISABLE_VARIANCE)


def max_bins(nb_aggregates, option_flags=_DEFAULT_CNH_CFG):
    """Largest CNH bins-per-aggregate count whose on-device buffer still fits
    CNH_MAX_DATA_BYTES for the given aggregate count."""
    feat = 0
    while _calc_required_memory(option_flags, nb_aggregates, feat + 1) \
            <= CNH_MAX_DATA_BYTES:
        feat += 1
    return feat


def decode(cfg, raw):
    """Decode a captured CNH data block (`raw` bytes, byte-swapped exactly like
    the standard ranging blocks) into per-aggregate histograms.

    Returns a dict:
        ref_residual : float (11 fractional bits)
        aggregates   : list (len == cfg.nb_of_aggregates) of dicts:
            hist        : [float] * cfg.feature_length  (value = raw / 2**scaler)
            hist_raw    : [int]
            hist_scaler : [int]
            ambient     : float
    Faithful port of vl53lmz_cnh_get_block_addresses / _cnh_get_mem_block_addresses
    for the fixed cnh_cfg (ping-pong + variance disabled)."""
    nb_agg = cfg.nb_of_aggregates
    feat = cfg.feature_length
    nwords = len(raw) // 4
    words = struct.unpack_from('<%di' % nwords, raw, 0)   # signed int32 view

    ref_residual = (words[2] & 0xFFFFFFFF) / 2048.0       # vl53lmz_cnh_get_ref_residual

    aggregates = [_decode_aggregate(words, raw, nb_agg, feat, agg_id)
                  for agg_id in range(nb_agg)]
    return {'ref_residual': ref_residual, 'aggregates': aggregates}


def _decode_aggregate(words, raw, nb_agg, feat, agg_id):
    agg_x_feat = nb_agg * feat
    agg_off = agg_id * feat

    state = words[0]
    info = words[CNH_PER_HEADER_BUFFER_INFO_IDX] & 0xFFFFFFFF
    pp_size = info & _BUFFER_INFO_WORDS_MASK
    buffer_flags = (info >> _BUFFER_INFO_FLAGS_SHIFT) & 0xFF

    # Select ping or pong buffer exactly as the C code does. With ping-pong
    # disabled the device reports a single buffer and this resolves to ping.
    local_pp = 1
    if (words[CNH_PER_HEADER_FLAGS_IDX] & 0x10) == 0x10:
        local_pp = 1
    if state == MI_STATE_PING:
        local_pp = 1 - local_pp

    base = CNH_PER_HEADER_WORDS
    if local_pp == 1:
        base += pp_size
    # buffer header is 2 words (state, nb_accumulated); data starts after it.
    blk = (base + CNH_PER_BUFFER_HEADER_WORDS) * 4   # byte offset of p[2]

    # FEAT_INT: int32 per (agg, feat)
    feat_int = list(struct.unpack_from('<%di' % feat, raw, blk + agg_off * 4))
    blk += agg_x_feat * 4
    # FEAT_FRAC: int8 scaler per (agg, feat)
    feat_scaler = list(struct.unpack_from('<%db' % feat, raw, blk + agg_off))
    blk += ((3 + agg_x_feat) // 4) * 4
    # AMBIENT_INT: int32 per aggregate (this is the value example12 prints)
    amb_int = struct.unpack_from('<i', raw, blk + agg_id * 4)[0]
    blk += nb_agg * 4
    # AMBIENT_FRAC: int8 scaler per aggregate
    amb_scaler = struct.unpack_from('<b', raw, blk + agg_id)[0]

    hist = [v / (2.0 ** s) for v, s in zip(feat_int, feat_scaler)]
    ambient = amb_int / (2.0 ** amb_scaler)
    return {
        'hist': hist,
        'hist_raw': feat_int,
        'hist_scaler': feat_scaler,
        'ambient': ambient,
    }
