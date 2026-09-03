#!/usr/bin/env python3
"""VL53L8CX ULD driver — Python port of the parts of vl53l8cx_api.c
(ULD 2.1.0, tools/doc/VL53L8CX_Linux_driver_2.1.0) needed for init (sensor
firmware download), 8x8 continuous ranging and result parsing.

The sensor is reached through a `platform` object the caller provides:

    platform.rd_multi(addr: int, size: int) -> bytes
    platform.wr_multi(addr: int, data: bytes) -> None
    platform.sleep_ms(ms: int) -> None

Register sequences are a 1:1 port of the C code; do not "simplify" them.
Configuration: NB_TARGET_PER_ZONE = 1, all output blocks enabled (matches the
default platform.h of the C driver).

Advanced features (power modes, detection thresholds, xtalk margin/calibration,
motion indicator, xtalk caldata get/set) are verbatim ports of the ST VL53L8CX
ULD (BSD-3-Clause), files vl53l8cx_api.c and vl53l8cx_plugin_{detection_thresholds,
xtalk,motion_indicator}.c from github.com/stm32duino/VL53L8CX/src. The const
command tables GET_XTALK_CMD / CALIBRATE_XTALK are lifted verbatim from
vl53l8cx_plugin_xtalk.h.

  Copyright (c) 2024 STMicroelectronics. Redistributed under BSD-3-Clause.

NB: the ST L8CX ULD has NO vl53l8cx_get/set_calibration_data (that monolithic
caldata-blob API belongs to the older L5CX driver). On the L8CX, calibration
save/restore IS the 776-byte xtalk blob — get_caldata_xtalk / set_caldata_xtalk.
"""

import struct
from pathlib import Path

# Absorbed from depz-usb-sensor-line/tof_vl53l8/tools/vl53l8cx_uld.py
# (blob data relocated to package data under data/{cx,ch}).
VL53L8CX_DATA_DIR = Path(__file__).parent / 'data' / 'cx'
VL53L8CH_DATA_DIR = Path(__file__).parent / 'data' / 'ch'
DATA_DIR = VL53L8CX_DATA_DIR    # backward-compat default

# Sensor firmware variant. The register/DCI protocol is identical; only the
# downloaded sensor FW (and hence its checksum at 0x812FFC) and the NVM/config
# blobs differ. 'cx' = VL53L8CX ULD 2.1.0; 'ch' = VL53L7CH/VL53L8CH (VL53LMZ)
# ULD 2.0.16 — used to try running CH firmware on VL53L8CX silicon.
VARIANT_DATA_DIR = {
    'cx': VL53L8CX_DATA_DIR,
    'ch': VL53L8CH_DATA_DIR,
}
FW_CHECKSUM = {
    'cx': 0xCADF7CAF,
    'ch': 0x0C0B6C9E,
}

RESOLUTION_4X4 = 16
RESOLUTION_8X8 = 64

RANGING_MODE_CONTINUOUS = 1
RANGING_MODE_AUTONOMOUS = 3

TARGET_ORDER_CLOSEST   = 1
TARGET_ORDER_STRONGEST = 2

# Status codes (subset, matches C defines)
STATUS_OK               = 0
STATUS_TIMEOUT_ERROR    = 1
STATUS_CORRUPTED_FRAME  = 2
STATUS_LASER_SAFETY     = 3
STATUS_FW_CHECKSUM_FAIL = 5
MCU_ERROR               = 66
STATUS_INVALID_PARAM    = 127
STATUS_ERROR            = 255

# Block headers for NB_TARGET_PER_ZONE == 1
START_BH              = 0x0000000D
METADATA_BH           = 0x54B400C0
COMMONDATA_BH         = 0x54C00040
AMBIENT_RATE_BH       = 0x54D00104
SPAD_COUNT_BH         = 0x55D00404
NB_TARGET_DETECTED_BH = 0xDB840401
SIGNAL_RATE_BH        = 0xDBC40404
RANGE_SIGMA_MM_BH     = 0xDEC40402
DISTANCE_BH           = 0xDF440402
REFLECTANCE_BH        = 0xE0440401
TARGET_STATUS_BH      = 0xE0840401
MOTION_DETECT_BH      = 0xD85808C0
CNH_DATA_IDX          = 0xC048      # VL53LMZ CNH data output block (VL53L8CH)

METADATA_IDX           = 0x54B4
SPAD_COUNT_IDX         = 0x55D0
AMBIENT_RATE_IDX       = 0x54D0
NB_TARGET_DETECTED_IDX = 0xDB84
SIGNAL_RATE_IDX        = 0xDBC4
RANGE_SIGMA_MM_IDX     = 0xDEC4
DISTANCE_IDX           = 0xDF44
REFLECTANCE_EST_PC_IDX = 0xE044
TARGET_STATUS_IDX      = 0xE084
MOTION_DETEC_IDX       = 0xD858

NVM_DATA_SIZE      = 492
OFFSET_BUFFER_SIZE = 488
XTALK_BUFFER_SIZE  = 776

DCI_ZONE_CONFIG    = 0x5450
DCI_FREQ_HZ        = 0x5458
DCI_INT_TIME       = 0x545C
DCI_RANGING_MODE   = 0xAD30
DCI_DSS_CONFIG     = 0xAD38
DCI_TARGET_ORDER   = 0xAE64
DCI_SHARPENER      = 0xAED8
DCI_SINGLE_RANGE   = 0xD964
DCI_OUTPUT_CONFIG  = 0xD968
DCI_OUTPUT_ENABLES = 0xD970
DCI_OUTPUT_LIST    = 0xD980
DCI_PIPE_CONTROL   = 0xDB80

UI_CMD_STATUS = 0x2C00
UI_CMD_START  = 0x2C04
UI_CMD_END    = 0x2FFF

# ── advanced-feature DCI indices / sizes (ST ULD vl53l8cx_api.h + plugins) ──
CONFIGURATION_SIZE = 972
DCI_VHV_CONFIG     = 0xAD60
DCI_CAL_CFG        = 0x5470
DCI_XTALK_CFG      = 0xAD94
DCI_MOTION_DETECTOR_CFG = 0xBFAC

DCI_DET_THRESH_CONFIG        = 0x5488
DCI_DET_THRESH_GLOBAL_CONFIG = 0xB6E0
DCI_DET_THRESH_START         = 0xB6E8
DCI_DET_THRESH_VALID_STATUS  = 0xB9F0
NB_THRESHOLDS  = 64
LAST_THRESHOLD = 128

# Power modes (vl53l8cx_api.h)
POWER_MODE_SLEEP      = 0
POWER_MODE_WAKEUP     = 1
POWER_MODE_DEEP_SLEEP = 2

# Detection-threshold measurement selectors + scale factors (plugin source):
# get divides, set multiplies by these.
DIST_MM               = 1
SIGNAL_PER_SPAD_KCPS  = 2
RANGE_SIGMA_MM        = 4
AMBIENT_PER_SPAD_KCPS = 8
NB_TARGET_DETECTED    = 9
TAR_STATUS            = 12
NB_SPADS_ENABLED      = 13
MOTION_INDICATOR      = 19
_THRESH_SCALE = {
    DIST_MM: 4,
    SIGNAL_PER_SPAD_KCPS: 2048,
    RANGE_SIGMA_MM: 128,
    AMBIENT_PER_SPAD_KCPS: 2048,
    NB_SPADS_ENABLED: 256,
    MOTION_INDICATOR: 65535,
}

# Threshold `type` (window) selectors.
THRESH_IN_WINDOW           = 0
THRESH_OUT_OF_WINDOW       = 1
THRESH_LESS_THAN_EQUAL_MIN = 2
THRESH_GREATER_THAN_MAX    = 3
THRESH_EQUAL_MIN           = 4
THRESH_NOT_EQUAL_MIN       = 5
# Threshold combine operation.
THRESH_OP_NONE = 0
THRESH_OP_OR   = 0
THRESH_OP_AND  = 2

# Motion-indicator results block (index of MOTION_DETEC block already declared
# above as MOTION_DETEC_IDX = 0xD858).

# Const command tables lifted verbatim from vl53l8cx_plugin_xtalk.h (BSD-3).
GET_XTALK_CMD = bytes([
    0x54, 0x00, 0x00, 0x40, 0x9F, 0xD8, 0x00, 0xC0, 0x9F, 0xE4, 0x01, 0x40, 0x9F, 0xF8, 0x00, 0x40,
    0x9F, 0xFC, 0x04, 0x04, 0xA0, 0xFC, 0x01, 0x00, 0xA1, 0x0C, 0x01, 0x00, 0xA1, 0x1C, 0x00, 0xC0,
    0xA1, 0x28, 0x09, 0x02, 0xA2, 0x48, 0x00, 0x40, 0xA2, 0x4C, 0x00, 0x81, 0xA2, 0x54, 0x00, 0x81,
    0xA2, 0x5C, 0x00, 0x81, 0xA2, 0x64, 0x00, 0x81, 0xA2, 0x6C, 0x00, 0x84, 0xA2, 0x8C, 0x00, 0x82,
    0x00, 0x00, 0x00, 0x0F, 0x07, 0x02, 0x00, 0x44,
])

# VL53L8CX_CALIBRATE_XTALK config table (vl53l8cx_plugin_xtalk.h, BSD-3).
CALIBRATE_XTALK = bytes([
    0x54, 0x50, 0x00, 0x80, 0x00, 0x04, 0x08, 0x08, 0x00, 0x00, 0x04, 0x04, 0xAD, 0x30, 0x00, 0x80,
    0x03, 0x01, 0x06, 0x03, 0x00, 0x00, 0x01, 0x00, 0xAD, 0x38, 0x01, 0x00, 0x01, 0xE0, 0x01, 0x40,
    0x00, 0x10, 0x00, 0x10, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x54, 0x58, 0x00, 0x40,
    0x04, 0x1A, 0x02, 0x00, 0x54, 0x5C, 0x01, 0x40, 0x00, 0x01, 0x00, 0x51, 0x00, 0x00, 0x0F, 0xA0,
    0x0F, 0xA0, 0x03, 0xE8, 0x02, 0x80, 0x1F, 0x40, 0x00, 0x00, 0x05, 0x00, 0x54, 0x70, 0x00, 0x80,
    0x03, 0x20, 0x03, 0x20, 0x00, 0x00, 0x00, 0x08, 0x54, 0x78, 0x01, 0x00, 0x01, 0x1B, 0x00, 0x21,
    0x00, 0x33, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x04, 0x01, 0x08, 0x02, 0x54, 0x88, 0x01, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x08, 0x00, 0xAD, 0x48, 0x01, 0x00, 0x01, 0xF4, 0x00, 0x00, 0x03, 0x06, 0x00, 0x10,
    0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x08, 0xAD, 0x60, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80,
    0x00, 0x00, 0x00, 0x00, 0x20, 0x1F, 0x01, 0xF4, 0x00, 0x00, 0x1D, 0x0A, 0xAD, 0x70, 0x00, 0x80,
    0x08, 0x00, 0x1F, 0x40, 0x00, 0x00, 0x00, 0x01, 0xAD, 0x78, 0x00, 0x80, 0x00, 0xA0, 0x03, 0x20,
    0x00, 0x01, 0x01, 0x90, 0xAD, 0x80, 0x00, 0x40, 0x00, 0x00, 0x28, 0x00, 0xAD, 0x84, 0x00, 0x80,
    0x00, 0x00, 0x32, 0x00, 0x03, 0x20, 0x00, 0x00, 0xAD, 0x8C, 0x00, 0x80, 0x02, 0x58, 0xFF, 0x38,
    0x00, 0x00, 0x00, 0x0C, 0xAD, 0x94, 0x01, 0x00, 0x00, 0x01, 0x90, 0x00, 0xFF, 0xFF, 0xFC, 0x00,
    0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0xAD, 0xA4, 0x00, 0xC0, 0x04, 0x80, 0x06, 0x1A,
    0x00, 0x80, 0x05, 0x80, 0x00, 0x00, 0x01, 0x06, 0xAD, 0xB0, 0x00, 0xC0, 0x04, 0x80, 0x06, 0x1A,
    0x19, 0x00, 0x05, 0x80, 0x00, 0x00, 0x01, 0x90, 0xAD, 0xBC, 0x04, 0x40, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x00, 0x25, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x05,
    0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x0F,
    0x00, 0x00, 0x00, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x0B, 0x0C, 0x0B, 0x0B,
    0x03, 0x03, 0x11, 0x05, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00,
    0xAE, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x0A,
    0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x08,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20,
    0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x05, 0x0A, 0x02, 0x00, 0x0C, 0x08,
    0x00, 0x00, 0x00, 0x00, 0xAE, 0x40, 0x00, 0x40, 0x00, 0x00, 0x00, 0xFF, 0xAE, 0x44, 0x00, 0x40,
    0x00, 0x10, 0x04, 0x01, 0xAE, 0x48, 0x00, 0x40, 0x00, 0x00, 0x10, 0x00, 0xAE, 0x4C, 0x00, 0x40,
    0x00, 0x00, 0x00, 0x01, 0xAE, 0x50, 0x01, 0x40, 0x00, 0x00, 0x00, 0x14, 0x04, 0x00, 0x28, 0x00,
    0x03, 0x20, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAE, 0x64, 0x00, 0x40,
    0x00, 0x00, 0x00, 0x01, 0xAE, 0xD8, 0x01, 0x00, 0x00, 0xC8, 0x05, 0xDC, 0x00, 0x00, 0x0C, 0xCD,
    0x01, 0x04, 0x00, 0x00, 0x00, 0x01, 0x26, 0x01, 0xB5, 0x50, 0x02, 0x82, 0xA3, 0xE8, 0xA3, 0xB8,
    0xA4, 0x38, 0xA4, 0x28, 0xA6, 0x48, 0xA4, 0x48, 0xA7, 0x88, 0xA7, 0x48, 0xAC, 0x10, 0xA7, 0x90,
    0x99, 0xBC, 0x99, 0xB4, 0x9A, 0xFC, 0x9A, 0xBC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xB5, 0xA0, 0x02, 0x82, 0x00, 0x88, 0x03, 0x00, 0x00, 0x82, 0x00, 0x82, 0x04, 0x04, 0x04, 0x08,
    0x00, 0x80, 0x04, 0x01, 0x09, 0x02, 0x09, 0x08, 0x04, 0x04, 0x00, 0x80, 0x04, 0x01, 0x04, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xB5, 0xF0, 0x00, 0x40, 0x00, 0x04, 0x00, 0x00, 0xB3, 0x9C, 0x01, 0x00, 0x40, 0x00, 0x05, 0x1E,
    0x02, 0x1B, 0x08, 0x7C, 0x80, 0x00, 0x12, 0x01, 0x00, 0x01, 0x08, 0x00, 0xB6, 0xC0, 0x00, 0xC0,
    0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAE, 0xA8, 0x00, 0x40,
    0x00, 0x00, 0x04, 0x05, 0xAE, 0xAC, 0x00, 0x80, 0x01, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00,
    0xAE, 0xB4, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0xAE, 0xB8, 0x00, 0x81, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xAE, 0xC0, 0x00, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xAE, 0xC8, 0x00, 0x81, 0x08, 0x01, 0x01, 0x08, 0x00, 0x00, 0x00, 0x08, 0xAE, 0xD0, 0x00, 0x81,
    0x01, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x01, 0xB5, 0xF4, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xB5, 0xFC, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xB6, 0x04, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0xB6, 0x08, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB6, 0x18, 0x00, 0x44,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xB6, 0x28, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xB6, 0x38, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB6, 0x48, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB6, 0x58, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xB6, 0x68, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x54, 0x70, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x03, 0xD4,
])

NB_TARGET_PER_ZONE = 1

STATUS_NAMES = {
    STATUS_TIMEOUT_ERROR:    'TIMEOUT',
    STATUS_CORRUPTED_FRAME:  'CORRUPTED_FRAME',
    STATUS_LASER_SAFETY:     'LASER_SAFETY',
    STATUS_FW_CHECKSUM_FAIL: 'FW_CHECKSUM_FAIL',
    MCU_ERROR:               'MCU_ERROR',
    STATUS_INVALID_PARAM:    'INVALID_PARAM',
    STATUS_ERROR:            'ERROR',
}


class Vl53l8cxError(Exception):
    def __init__(self, code, where=''):
        self.code = code
        name = STATUS_NAMES.get(code, hex(code))
        super().__init__(f'VL53L8CX {name} (code {code}) {where}'.rstrip())


def swap_buffer(data: bytes) -> bytes:
    """VL53L8CX_SwapBuffer: byte-reverse every 32-bit word."""
    n = len(data) // 4
    body = struct.pack('<%dI' % n, *struct.unpack('>%dI' % n, data[:n * 4]))
    return body + data[n * 4:]


def _bh_fields(bh: int):
    """union Block_header: type[3:0], size[15:4], idx[31:16]."""
    return bh & 0xF, (bh >> 4) & 0xFFF, (bh >> 16) & 0xFFFF


def _bh_set_size(bh: int, size: int) -> int:
    return (bh & ~0xFFF0) | ((size & 0xFFF) << 4)


class MotionConfig:
    """Mirror of VL53L8CX_Motion_Configuration (156 bytes, plugin source)."""

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
        self.nb_of_temporal_accumulations = 0
        self.min_nb_for_global_detection = 0
        self.global_indicator_format_1 = 0
        self.global_indicator_format_2 = 0
        self.spare_1 = 0
        self.spare_2 = 0
        self.spare_3 = 0
        self.map_id = [0] * 64
        self.indicator_format_1 = [0] * 32
        self.indicator_format_2 = [0] * 32

    def pack(self) -> bytes:
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
            self.spare_1, self.spare_2, self.spare_3,
            *self.map_id,
            *self.indicator_format_1,
            *self.indicator_format_2)


class VL53L8CX:
    def __init__(self, platform, variant: str = 'cx', data_dir: Path = None):
        self.p = platform
        self.variant = variant
        if data_dir is None:
            data_dir = VARIANT_DATA_DIR[variant]
        self.fw_checksum = FW_CHECKSUM[variant]
        # start_ranging output config differs slightly by FW variant:
        #   cx (ULD 2.1.0):     OUTPUT_ENABLES word[3]=0xC0000000, frame tail +32
        #   ch (VL53LMZ 2.0.16): OUTPUT_ENABLES word[3]=0,          frame tail +24
        # _footer_id_off: where the footer id sits relative to the frame end
        # (cx 2.1.0 = size-12; ch 2.0.16 = size-4).
        if variant == 'ch':
            self._output_enable_w3 = 0x00000000
            self._frame_tail = 24
            self._footer_id_off = 4
        else:
            self._output_enable_w3 = 0xC0000000
            self._frame_tail = 32
            self._footer_id_off = 12
        self.firmware    = (data_dir / 'firmware.bin').read_bytes()
        self.default_cfg = (data_dir / 'default_configuration.bin').read_bytes()
        self.default_xtalk = (data_dir / 'default_xtalk.bin').read_bytes()
        self.get_nvm_cmd = (data_dir / 'get_nvm_cmd.bin').read_bytes()
        if len(self.firmware) != 0x15000:
            raise ValueError(f'firmware.bin ({variant}) has wrong size — '
                             f'rerun the blob extractor')
        self.offset_data = b''
        self.xtalk_data  = b''
        self.streamcount = 255
        self.data_read_size = 0
        self.frame_size_mismatch = None  # (fw, host) when FW disagrees
        self.last_blocks = []            # [(idx, type, size)] of last frame
        self._motion_present = False     # motion-indicator output configured?

    # ---------------- low-level helpers ----------------

    def _rd_byte(self, addr):
        return self.p.rd_multi(addr, 1)[0]

    def _wr_byte(self, addr, value):
        self.p.wr_multi(addr, bytes([value]))

    def _poll_for_answer(self, size, pos, address, mask, expected, where=''):
        """_vl53l8cx_poll_for_answer(): poll `size` bytes at `address` until
        buf[pos] & mask == expected. 10 ms period, 2 s timeout."""
        timeout = 0
        while True:
            buf = self.p.rd_multi(address, size)
            self.p.sleep_ms(10)
            if timeout >= 200:
                raise Vl53l8cxError(STATUS_TIMEOUT_ERROR, where)
            if size >= 4 and buf[2] >= 0x7F:
                raise Vl53l8cxError(MCU_ERROR, where)
            timeout += 1
            if (buf[pos] & mask) == expected:
                return

    def _poll_for_mcu_boot(self):
        timeout = 0
        while timeout < 500:
            go2_status0 = self._rd_byte(0x06)
            if go2_status0 & 0x80:
                go2_status1 = self._rd_byte(0x07)
                if go2_status1 & 0x01:
                    return
            self.p.sleep_ms(1)
            timeout += 1
            if go2_status0 & 0x01:
                return
        raise Vl53l8cxError(STATUS_TIMEOUT_ERROR, 'mcu boot')

    # ---------------- DCI access ----------------

    def dci_read_data(self, index, data_size):
        cmd = bytearray(12)
        cmd[0] = (index >> 8) & 0xFF
        cmd[1] = index & 0xFF
        cmd[2] = (data_size & 0xFF0) >> 4
        cmd[3] = (data_size & 0xF) << 4
        cmd[7] = 0x0F
        cmd[9] = 0x02
        cmd[11] = 0x08
        self.p.wr_multi(UI_CMD_END - 11, bytes(cmd))
        self._poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, f'dci read 0x{index:04X}')
        buf = self.p.rd_multi(UI_CMD_START, data_size + 12)
        buf = swap_buffer(buf)
        return bytearray(buf[4:4 + data_size])

    def dci_write_data(self, index, data):
        data_size = len(data)
        footer = bytes([0x00, 0x00, 0x00, 0x0F, 0x05, 0x01,
                        ((data_size + 8) >> 8) & 0xFF, (data_size + 8) & 0xFF])
        address = UI_CMD_END - (data_size + 12) + 1
        header = bytes([(index >> 8) & 0xFF, index & 0xFF,
                        (data_size & 0xFF0) >> 4, (data_size & 0xF) << 4])
        buf = header + swap_buffer(bytes(data)) + footer
        self.p.wr_multi(address, buf)
        self._poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, f'dci write 0x{index:04X}')

    def dci_replace_data(self, index, data_size, new_data, new_data_pos):
        data = self.dci_read_data(index, data_size)
        data[new_data_pos:new_data_pos + len(new_data)] = new_data
        self.dci_write_data(index, data)

    # ---------------- offset / xtalk upload ----------------

    def _send_offset_data(self, resolution):
        buf = bytearray(self.offset_data[:OFFSET_BUFFER_SIZE])

        if resolution == RESOLUTION_4X4:
            dss_4x4 = bytes([0x0F, 0x04, 0x04, 0x00, 0x08, 0x10, 0x10, 0x07])
            buf[0x10:0x10 + 8] = dss_4x4
            buf = bytearray(swap_buffer(bytes(buf)))
            signal_grid = list(struct.unpack_from('<64I', buf, 0x3C))
            range_grid  = list(struct.unpack_from('<64h', buf, 0x140))
            for j in range(4):
                for i in range(4):
                    signal_grid[i + 4 * j] = (
                        signal_grid[2 * i + 16 * j + 0]
                        + signal_grid[2 * i + 16 * j + 1]
                        + signal_grid[2 * i + 16 * j + 8]
                        + signal_grid[2 * i + 16 * j + 9]) // 4
                    range_grid[i + 4 * j] = (
                        range_grid[2 * i + 16 * j + 0]
                        + range_grid[2 * i + 16 * j + 1]
                        + range_grid[2 * i + 16 * j + 8]
                        + range_grid[2 * i + 16 * j + 9]) // 4
            for k in range(16, 64):
                signal_grid[k] = 0
                range_grid[k] = 0
            struct.pack_into('<64I', buf, 0x3C, *(v & 0xFFFFFFFF for v in signal_grid))
            struct.pack_into('<64h', buf, 0x140, *(max(-32768, min(32767, v)) for v in range_grid))
            buf = bytearray(swap_buffer(bytes(buf)))

        # Shift the buffer 8 bytes left (drop NVM header) and append the
        # footer at 0x1E0. In C the shift reads past the 488-byte region, but
        # those bytes are then overwritten by the footer, so this is identical.
        footer = bytes([0x00, 0x00, 0x00, 0x0F, 0x03, 0x01, 0x01, 0xE4])
        buf = buf[8:] + footer

        self.p.wr_multi(0x2E18, bytes(buf))
        self._poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, 'offset data')

    def _send_xtalk_data(self, resolution):
        buf = bytearray(self.xtalk_data[:XTALK_BUFFER_SIZE])

        if resolution == RESOLUTION_4X4:
            res4x4      = bytes([0x0F, 0x04, 0x04, 0x17, 0x08, 0x10, 0x10, 0x07])
            dss_4x4     = bytes([0x00, 0x78, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08])
            profile_4x4 = bytes([0xA0, 0xFC, 0x01, 0x00])
            buf[0x08:0x08 + 8] = res4x4
            buf[0x20:0x20 + 8] = dss_4x4
            buf = bytearray(swap_buffer(bytes(buf)))
            signal_grid = list(struct.unpack_from('<64I', buf, 0x34))
            for j in range(4):
                for i in range(4):
                    signal_grid[i + 4 * j] = (
                        signal_grid[2 * i + 16 * j + 0]
                        + signal_grid[2 * i + 16 * j + 1]
                        + signal_grid[2 * i + 16 * j + 8]
                        + signal_grid[2 * i + 16 * j + 9]) // 4
            for k in range(16, 64):
                signal_grid[k] = 0
            struct.pack_into('<64I', buf, 0x34, *(v & 0xFFFFFFFF for v in signal_grid))
            buf = bytearray(swap_buffer(bytes(buf)))
            buf[0x134:0x134 + 4] = profile_4x4
            buf[0x078:0x078 + 4] = bytes(4)

        self.p.wr_multi(0x2CF8, bytes(buf))
        self._poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, 'xtalk data')

    # ---------------- public API ----------------

    def is_alive(self):
        """Returns (device_id, revision_id); alive when (0xF0, 0x0C)."""
        self._wr_byte(0x7FFF, 0x00)
        device_id = self._rd_byte(0x00)
        revision_id = self._rd_byte(0x01)
        self._wr_byte(0x7FFF, 0x02)
        return device_id, revision_id

    def init(self, progress=None):
        """vl53l8cx_init(): boot the sensor MCU, download the 84 KB sensor
        firmware, upload NVM offset / xtalk / default configuration.
        `progress(text)` is an optional UI callback."""
        def note(text):
            if progress:
                progress(text)

        wr = self._wr_byte
        rd = self._rd_byte

        # SW reboot sequence
        note('SW reboot...')
        wr(0x7FFF, 0x00)
        wr(0x0009, 0x04)
        wr(0x000F, 0x40)
        wr(0x000A, 0x03)
        rd(0x7FFF)
        wr(0x000C, 0x01)

        wr(0x0101, 0x00)
        wr(0x0102, 0x00)
        wr(0x010A, 0x01)
        wr(0x4002, 0x01)
        wr(0x4002, 0x00)
        wr(0x010A, 0x03)
        wr(0x0103, 0x01)
        wr(0x000C, 0x00)
        wr(0x000F, 0x43)
        self.p.sleep_ms(1)

        wr(0x000F, 0x40)
        wr(0x000A, 0x01)
        self.p.sleep_ms(100)

        # Wait for sensor booted
        note('Waiting for sensor boot...')
        wr(0x7FFF, 0x00)
        self._poll_for_answer(1, 0, 0x06, 0xFF, 1, 'sensor boot')

        wr(0x000E, 0x01)
        wr(0x7FFF, 0x02)

        # Enable FW access
        wr(0x7FFF, 0x01)
        wr(0x06, 0x01)
        self._poll_for_answer(1, 0, 0x21, 0xFF, 0x04, 'fw access')

        wr(0x7FFF, 0x00)

        # Enable host access to GO1
        rd(0x7FFF)
        wr(0x0C, 0x01)

        # Power ON status
        wr(0x7FFF, 0x00)
        wr(0x101, 0x00)
        wr(0x102, 0x00)
        wr(0x010A, 0x01)
        wr(0x4002, 0x01)
        wr(0x4002, 0x00)
        wr(0x010A, 0x03)
        wr(0x103, 0x01)
        wr(0x400F, 0x00)
        wr(0x21A, 0x43)
        wr(0x21A, 0x03)
        wr(0x21A, 0x01)
        wr(0x21A, 0x00)
        wr(0x219, 0x00)
        wr(0x21B, 0x00)

        # Wake up MCU
        wr(0x7FFF, 0x00)
        rd(0x7FFF)
        wr(0x7FFF, 0x01)

        # Download FW into VL53L8CX
        note('Downloading sensor FW (84 KB)... bank 1/3')
        wr(0x7FFF, 0x09)
        self.p.wr_multi(0, self.firmware[0:0x8000])
        note('Downloading sensor FW... bank 2/3')
        wr(0x7FFF, 0x0A)
        self.p.wr_multi(0, self.firmware[0x8000:0x10000])
        note('Downloading sensor FW... bank 3/3')
        wr(0x7FFF, 0x0B)
        self.p.wr_multi(0, self.firmware[0x10000:0x15000])
        wr(0x7FFF, 0x01)

        # Check if FW correctly downloaded
        wr(0x7FFF, 0x01)
        wr(0x06, 0x03)

        self.p.sleep_ms(5)
        wr(0x7FFF, 0x00)
        rd(0x7FFF)
        wr(0x0C, 0x01)

        # Reset MCU and wait boot
        note('Booting sensor MCU...')
        wr(0x7FFF, 0x00)
        wr(0x114, 0x00)
        wr(0x115, 0x00)
        wr(0x116, 0x42)
        wr(0x117, 0x00)
        wr(0x0B, 0x00)
        rd(0x7FFF)
        wr(0x0C, 0x00)
        wr(0x0B, 0x01)

        self._poll_for_mcu_boot()

        wr(0x7FFF, 0x02)

        # Firmware checksum (0x812FFC & 0xFFFF); value depends on the FW variant
        # (see FW_CHECKSUM). cx = ULD 2.1.0 FW; ch = VL53LMZ ULD 2.0.16 FW.
        crc = struct.unpack('<I', swap_buffer(self.p.rd_multi(0x2FFC, 4)))[0]
        if crc != self.fw_checksum:
            raise Vl53l8cxError(STATUS_FW_CHECKSUM_FAIL,
                                f'crc=0x{crc:08X} (expected 0x{self.fw_checksum:08X} '
                                f'for {self.variant})')
        note('Sensor FW checksum OK')

        # Get offset NVM data
        note('Reading NVM offset data...')
        self.p.wr_multi(0x2FD8, self.get_nvm_cmd)
        self._poll_for_answer(4, 0, UI_CMD_STATUS, 0xFF, 2, 'nvm read')
        nvm = self.p.rd_multi(UI_CMD_START, NVM_DATA_SIZE)
        self.offset_data = nvm[:OFFSET_BUFFER_SIZE]
        self._send_offset_data(RESOLUTION_4X4)

        # Default xtalk
        note('Uploading default xtalk...')
        self.xtalk_data = self.default_xtalk
        self._send_xtalk_data(RESOLUTION_4X4)

        # Default configuration
        note('Uploading default configuration...')
        self.p.wr_multi(0x2C34, self.default_cfg)
        self._poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, 'default config')

        pipe_ctrl = bytes([NB_TARGET_PER_ZONE, 0x00, 0x01, 0x00])
        self.dci_write_data(DCI_PIPE_CONTROL, pipe_ctrl)
        self.dci_write_data(DCI_SINGLE_RANGE, struct.pack('<I', 0x01))
        note('Sensor init done')

    def get_resolution(self):
        buf = self.dci_read_data(DCI_ZONE_CONFIG, 8)
        return buf[0] * buf[1]

    def set_resolution(self, resolution):
        if resolution == RESOLUTION_4X4:
            buf = self.dci_read_data(DCI_DSS_CONFIG, 16)
            buf[0x04] = 64
            buf[0x06] = 64
            buf[0x09] = 4
            self.dci_write_data(DCI_DSS_CONFIG, buf)
            buf = self.dci_read_data(DCI_ZONE_CONFIG, 8)
            buf[0x00] = 4
            buf[0x01] = 4
            buf[0x04] = 8
            buf[0x05] = 8
            self.dci_write_data(DCI_ZONE_CONFIG, buf)
        elif resolution == RESOLUTION_8X8:
            buf = self.dci_read_data(DCI_DSS_CONFIG, 16)
            buf[0x04] = 16
            buf[0x06] = 16
            buf[0x09] = 1
            self.dci_write_data(DCI_DSS_CONFIG, buf)
            buf = self.dci_read_data(DCI_ZONE_CONFIG, 8)
            buf[0x00] = 8
            buf[0x01] = 8
            buf[0x04] = 4
            buf[0x05] = 4
            self.dci_write_data(DCI_ZONE_CONFIG, buf)
        else:
            raise Vl53l8cxError(STATUS_INVALID_PARAM, 'set_resolution')
        self._send_offset_data(resolution)
        self._send_xtalk_data(resolution)

    def get_ranging_frequency_hz(self):
        return self.dci_read_data(DCI_FREQ_HZ, 4)[1]

    def set_ranging_frequency_hz(self, hz):
        self.dci_replace_data(DCI_FREQ_HZ, 4, bytes([hz]), 0x01)

    def set_ranging_mode(self, mode):
        buf = self.dci_read_data(DCI_RANGING_MODE, 8)
        if mode == RANGING_MODE_CONTINUOUS:
            buf[0x01] = 0x1
            buf[0x03] = 0x3
            single_range = 0x00
        elif mode == RANGING_MODE_AUTONOMOUS:
            buf[0x01] = 0x3
            buf[0x03] = 0x2
            single_range = 0x01
        else:
            raise Vl53l8cxError(STATUS_INVALID_PARAM, 'set_ranging_mode')
        self.dci_write_data(DCI_RANGING_MODE, buf)
        self.dci_write_data(DCI_SINGLE_RANGE, struct.pack('<I', single_range))

    def get_ranging_mode(self):
        buf = self.dci_read_data(DCI_RANGING_MODE, 8)
        return RANGING_MODE_CONTINUOUS if buf[0x01] == 0x1 \
            else RANGING_MODE_AUTONOMOUS

    def get_integration_time_ms(self):
        buf = self.dci_read_data(DCI_INT_TIME, 20)
        return struct.unpack_from('<I', buf, 0)[0] // 1000

    def set_integration_time_ms(self, time_ms):
        """Integration time 2..1000 ms. No effect in continuous ranging mode."""
        if not (2 <= time_ms <= 1000):
            raise Vl53l8cxError(STATUS_INVALID_PARAM, 'set_integration_time_ms')
        self.dci_replace_data(DCI_INT_TIME, 20,
                              struct.pack('<I', time_ms * 1000), 0x00)

    def get_sharpener_percent(self):
        """Sharpener 0..99 %. Rounds to nearest: the register holds pct scaled
        to 0..255, and truncating the way back (as ST's C ULD does) loses a
        count for 95 of the 100 legal values — set(25) read back as 24. That
        also made calibrate_xtalk's save/restore decay the setting by 1 % on
        every run (25 -> 24 -> 23 -> ...). The stored byte is unchanged; only
        this host-side interpretation is."""
        return round(self.dci_read_data(DCI_SHARPENER, 16)[0xD] * 100 / 255)

    def set_sharpener_percent(self, pct):
        """Sharpener 0..99 % (0 = disabled)."""
        if pct >= 100:
            raise Vl53l8cxError(STATUS_INVALID_PARAM, 'set_sharpener_percent')
        self.dci_replace_data(DCI_SHARPENER, 16,
                              bytes([pct * 255 // 100]), 0xD)

    def get_target_order(self):
        return self.dci_read_data(DCI_TARGET_ORDER, 4)[0]

    def set_target_order(self, order):
        if order not in (TARGET_ORDER_CLOSEST, TARGET_ORDER_STRONGEST):
            raise Vl53l8cxError(STATUS_INVALID_PARAM, 'set_target_order')
        self.dci_replace_data(DCI_TARGET_ORDER, 4, bytes([order]), 0x0)

    def start_ranging(self, cnh_data_size=None):
        """Start ranging. When `cnh_data_size` is given (bytes, from
        CnhConfig.required_memory), a VL53L8CH CNH data block is appended to the
        output list so each frame also carries the compact-network-histogram
        buffer. The CNH frame is far larger than the MCU stream cap, so the
        caller must read it in poll-mode (check_data_ready + get_ranging_data),
        not via the MCU INT stream."""
        resolution = self.get_resolution()
        # A 0 / non-standard resolution produces a zero-sized output config and
        # faults the sensor MCU (GO2 0x9C) instead of streaming. Refuse early
        # with a clear error; call set_resolution() first.
        if resolution not in (RESOLUTION_4X4, RESOLUTION_8X8):
            raise Vl53l8cxError(STATUS_INVALID_PARAM,
                                f'bad resolution {resolution} — call set_resolution first')
        self.data_read_size = 0
        self.streamcount = 255

        # All outputs enabled (default platform.h): bits 0..11
        output_bh_enable = [0x00000FFF, 0, 0, self._output_enable_w3]
        output = [START_BH, METADATA_BH, COMMONDATA_BH, AMBIENT_RATE_BH,
                  SPAD_COUNT_BH, NB_TARGET_DETECTED_BH, SIGNAL_RATE_BH,
                  RANGE_SIGMA_MM_BH, DISTANCE_BH, REFLECTANCE_BH,
                  TARGET_STATUS_BH, MOTION_DETECT_BH]
        if cnh_data_size is not None:
            # vl53lmz_add_output_block: CNH block, type 4, size in 32-bit words.
            cnh_bh = (CNH_DATA_IDX << 16) | (((cnh_data_size // 4) & 0xFFF) << 4) | 4
            output.append(cnh_bh)
            output_bh_enable[0] |= (1 << (len(output) - 1))

        for i in range(len(output)):
            if output[i] == 0 or not (output_bh_enable[i // 32] & (1 << (i % 32))):
                continue
            bh_type, bh_size, bh_idx = _bh_fields(output[i])
            if 0x1 <= bh_type < 0x0D:
                if 0x54D0 <= bh_idx < 0x54D0 + 960:
                    bh_size = resolution
                elif bh_idx == CNH_DATA_IDX:
                    pass            # keep CNH block size; not zone-scaled
                else:
                    bh_size = resolution * NB_TARGET_PER_ZONE
                output[i] = _bh_set_size(output[i], bh_size)
                self.data_read_size += bh_type * bh_size
            else:
                self.data_read_size += bh_size
            self.data_read_size += 4
        self.data_read_size += self._frame_tail

        self.dci_write_data(DCI_OUTPUT_LIST,
                            struct.pack('<%dI' % len(output), *output))

        header_config = struct.pack('<2I', self.data_read_size, len(output) + 1)
        self.dci_write_data(DCI_OUTPUT_CONFIG, header_config)

        self.dci_write_data(DCI_OUTPUT_ENABLES,
                            struct.pack('<4I', *output_bh_enable))

        # Start xshut bypass (interrupt mode)
        self._wr_byte(0x7FFF, 0x00)
        self._wr_byte(0x09, 0x05)
        self._wr_byte(0x7FFF, 0x02)

        # Start ranging session
        self.p.wr_multi(UI_CMD_END - 3, bytes([0x00, 0x03, 0x00, 0x00]))
        self._poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, 'start ranging')

        # The FW reports the actual frame size it will stream. The C driver
        # asserts equality, but the released FW blobs consistently report
        # 4 bytes less than the api.c formula — trust the FW value.
        buf = self.dci_read_data(0x5440, 12)
        tmp = struct.unpack_from('<H', buf, 0x8)[0]
        self.frame_size_mismatch = None
        if tmp != self.data_read_size:
            self.frame_size_mismatch = (tmp, self.data_read_size)
            self.data_read_size = tmp

        # Laser safety fault check
        buf = self.dci_read_data(0xE0C4, 8)
        if buf[0x6] != 0:
            raise Vl53l8cxError(STATUS_LASER_SAFETY)

    def stop_ranging(self):
        auto_stop_flag = struct.unpack('<I', self.p.rd_multi(0x2FFC, 4))[0]
        if auto_stop_flag != 0x4FF:
            self._wr_byte(0x7FFF, 0x00)
            # Provoke MCU stop
            self._wr_byte(0x15, 0x16)
            self._wr_byte(0x14, 0x01)
            # Poll for G02 status 0 MCU stop
            tmp = 0
            timeout = 0
            while ((tmp & 0x80) >> 7) == 0:
                tmp = self._rd_byte(0x06)
                self.p.sleep_ms(10)
                timeout += 1
                if timeout > 500:
                    break
        # Check GO2 status 1
        tmp = self._rd_byte(0x06)
        if tmp & 0x80:
            tmp = self._rd_byte(0x07)
            if tmp not in (0x84, 0x85):
                pass  # non-fatal: C code ORs it into status; we just continue
        # Undo MCU stop
        self._wr_byte(0x7FFF, 0x00)
        self._wr_byte(0x14, 0x00)
        self._wr_byte(0x15, 0x00)
        # Stop xshut bypass
        self._wr_byte(0x09, 0x04)
        self._wr_byte(0x7FFF, 0x02)

    def check_data_ready(self):
        buf = self.p.rd_multi(0x0, 4)
        if (buf[0] != self.streamcount and buf[0] != 255
                and buf[1] == 0x05
                and (buf[2] & 0x05) == 0x05
                and (buf[3] & 0x10) == 0x10):
            self.streamcount = buf[0]
            return True
        if buf[3] & 0x80:
            raise Vl53l8cxError(buf[2], 'GO2 error status')
        return False

    def get_ranging_data(self):
        """Poll-mode read: fetch one results frame from reg 0x00 and parse it."""
        raw = self.p.rd_multi(0x0, self.data_read_size)
        return self.parse_frame(raw)

    def parse_frame(self, raw):
        """Parse one raw results frame (data_read_size bytes read from reg 0x00).
        Used both by poll-mode get_ranging_data() and by the host when the MCU
        pushes frames over the INT-driven stream. Returns a dict with per-zone
        lists distance_mm[int], target_status, nb_target_detected,
        signal_per_spad (kcps/SPAD), ambient_per_spad (kcps/SPAD),
        nb_spads_enabled, range_sigma_mm (float), reflectance (%), plus the
        per-frame scalar silicon_temp_degc."""
        self.streamcount = raw[0]
        buf = swap_buffer(raw)

        results = {
            'distance_mm':        [0] * (RESOLUTION_8X8 * NB_TARGET_PER_ZONE),
            'target_status':      [0] * (RESOLUTION_8X8 * NB_TARGET_PER_ZONE),
            'nb_target_detected': [0] * RESOLUTION_8X8,
            'signal_per_spad':    [0] * (RESOLUTION_8X8 * NB_TARGET_PER_ZONE),
            'ambient_per_spad':   [0] * RESOLUTION_8X8,
            'nb_spads_enabled':   [0] * RESOLUTION_8X8,
            'range_sigma_mm':     [0.0] * (RESOLUTION_8X8 * NB_TARGET_PER_ZONE),
            'reflectance':        [0] * (RESOLUTION_8X8 * NB_TARGET_PER_ZONE),
            'silicon_temp_degc':  0,
            'cnh_raw':            None,
        }

        self.last_blocks = []
        i = 16
        while i + 4 <= self.data_read_size:
            bh = struct.unpack_from('<I', buf, i)[0]
            bh_type, bh_size, bh_idx = _bh_fields(bh)
            msize = bh_type * bh_size if 0x1 < bh_type < 0xD else bh_size
            # The C driver reads into an oversized temp_buffer, so it walks past
            # the last real block into footer/garbage harmlessly. Our buffer is
            # exact-sized: stop once a block would run past the end (= we have
            # reached the footer; all data blocks precede it). Without this the
            # parser raised struct.error on some frames (seen at 15 Hz).
            if i + 4 + msize > self.data_read_size:
                break
            self.last_blocks.append((bh_idx, bh_type, bh_size))

            if bh_idx == METADATA_IDX:
                results['silicon_temp_degc'] = \
                    struct.unpack_from('<b', buf, i + 12)[0]
            elif bh_idx == DISTANCE_IDX:
                results['distance_mm'] = list(
                    struct.unpack_from('<%dh' % (msize // 2), buf, i + 4))
            elif bh_idx == TARGET_STATUS_IDX:
                results['target_status'] = list(buf[i + 4:i + 4 + msize])
            elif bh_idx == NB_TARGET_DETECTED_IDX:
                results['nb_target_detected'] = list(buf[i + 4:i + 4 + msize])
            elif bh_idx == SIGNAL_RATE_IDX:
                results['signal_per_spad'] = list(
                    struct.unpack_from('<%dI' % (msize // 4), buf, i + 4))
            elif bh_idx == AMBIENT_RATE_IDX:
                results['ambient_per_spad'] = list(
                    struct.unpack_from('<%dI' % (msize // 4), buf, i + 4))
            elif bh_idx == SPAD_COUNT_IDX:
                results['nb_spads_enabled'] = list(
                    struct.unpack_from('<%dI' % (msize // 4), buf, i + 4))
            elif bh_idx == RANGE_SIGMA_MM_IDX:
                results['range_sigma_mm'] = list(
                    struct.unpack_from('<%dH' % (msize // 2), buf, i + 4))
            elif bh_idx == REFLECTANCE_EST_PC_IDX:
                results['reflectance'] = list(buf[i + 4:i + 4 + msize])
            elif bh_idx == CNH_DATA_IDX:
                results['cnh_raw'] = bytes(buf[i + 4:i + 4 + msize])

            i += msize + 4

        # Convert to real format (fixed-point scaling, per ST GetRangingData).
        results['distance_mm'] = [d // 4 for d in results['distance_mm']]
        results['range_sigma_mm'] = [s / 128.0 for s in results['range_sigma_mm']]

        # No target detected -> status 255. Iterate the zones actually present
        # in this frame (16 for 4x4, 64 for 8x8), not a fixed 64.
        nzones = len(results['nb_target_detected'])
        for z in range(nzones):
            if results['nb_target_detected'][z] == 0:
                for t in range(NB_TARGET_PER_ZONE):
                    idx = NB_TARGET_PER_ZONE * z + t
                    if idx < len(results['target_status']):
                        results['target_status'][idx] = 255

        # Motion indicator block (optional): global indicator + per-zone map.
        # Present when the motion detector is configured; harmless otherwise.
        if self._motion_present:
            results['motion_indicator'] = self._parse_motion(buf)

        # Header/footer id match check (footer id offset is variant-specific:
        # cx 2.1.0 = size-12, ch 2.0.16 = size-4).
        foff = self._footer_id_off
        if buf[0x8:0xA] != buf[self.data_read_size - foff:self.data_read_size - foff + 2]:
            raise Vl53l8cxError(STATUS_CORRUPTED_FRAME)

        return results

    def _parse_motion(self, buf):
        """Extract the MOTION_INDICATOR results block (index 0xD858) if it was
        walked in the last frame. The block is a VL53L8CX_ResultsData
        MotionIndicator: global_indicator_1 (u32), global_indicator_2 (u32),
        status (u8), nb_of_detected_aggregates (u8), nb_of_aggregates (u8),
        spare (u8), motion[32] (u32 per aggregate)."""
        for bh_idx, bh_type, bh_size in self.last_blocks:
            if bh_idx != MOTION_DETEC_IDX:
                continue
            # locate the block's byte offset again
            i = 16
            while i + 4 <= self.data_read_size:
                bh = struct.unpack_from('<I', buf, i)[0]
                t, s, idx = _bh_fields(bh)
                msize = t * s if 0x1 < t < 0xD else s
                if i + 4 + msize > self.data_read_size:
                    break
                if idx == MOTION_DETEC_IDX:
                    g1, g2 = struct.unpack_from('<II', buf, i + 4)
                    status, nb_det, nb_agg, _sp = struct.unpack_from('<4B', buf, i + 12)
                    motion = list(struct.unpack_from('<32I', buf, i + 16))
                    return {
                        'global_indicator_1': g1,
                        'global_indicator_2': g2,
                        'status': status,
                        'nb_of_detected_aggregates': nb_det,
                        'nb_of_aggregates': nb_agg,
                        'motion': motion,
                    }
                i += msize + 4
        return None

    # ---------------- power modes (vl53l8cx_api.c) ----------------

    def get_power_mode(self):
        self._wr_byte(0x7FFF, 0x00)
        tmp = self._rd_byte(0x09)
        if tmp == 0x04:
            mode = POWER_MODE_WAKEUP
        elif tmp == 0x02:
            mode = (POWER_MODE_DEEP_SLEEP
                    if self._rd_byte(0x000F) == 0x43 else POWER_MODE_SLEEP)
        else:
            self._wr_byte(0x7FFF, 0x02)
            raise Vl53l8cxError(STATUS_ERROR, 'get_power_mode')
        self._wr_byte(0x7FFF, 0x02)
        return mode

    def set_power_mode(self, power_mode):
        """WAKEUP (1), SLEEP (0) or DEEP_SLEEP (2). Not allowed while ranging.
        Wake from DEEP_SLEEP re-runs init() (the FW blob is lost)."""
        current = self.get_power_mode()
        if power_mode == current:
            return
        if power_mode == POWER_MODE_WAKEUP:
            self._wr_byte(0x7FFF, 0x00)
            self._wr_byte(0x09, 0x04)
            stored = self._rd_byte(0x000F)
            if stored == 0x43:
                self._wr_byte(0x000F, 0x40)
            self._poll_for_answer(1, 0, 0x06, 0x01, 1, 'wakeup')
            self._wr_byte(0x7FFF, 0x02)
            if stored == 0x43:
                self.init()
        elif power_mode == POWER_MODE_SLEEP:
            self._wr_byte(0x7FFF, 0x00)
            self._wr_byte(0x09, 0x02)
            self._poll_for_answer(1, 0, 0x06, 0x01, 0, 'sleep')
            self._wr_byte(0x7FFF, 0x02)
        elif power_mode == POWER_MODE_DEEP_SLEEP:
            self._wr_byte(0x7FFF, 0x00)
            self._wr_byte(0x09, 0x02)
            self._poll_for_answer(1, 0, 0x06, 0x01, 0, 'deep sleep')
            self._wr_byte(0x000F, 0x43)
            self._wr_byte(0x7FFF, 0x02)
        else:
            raise Vl53l8cxError(STATUS_INVALID_PARAM, 'set_power_mode')

    # ---------------- xtalk (vl53l8cx_plugin_xtalk.c) ----------------

    def get_xtalk_margin(self):
        """Xtalk margin in kcps/spad."""
        buf = self.dci_read_data(DCI_XTALK_CFG, 16)
        return struct.unpack_from('<I', buf, 0)[0] / 2048.0

    def set_xtalk_margin(self, margin_kcps):
        if margin_kcps > 10000:
            raise Vl53l8cxError(STATUS_INVALID_PARAM, 'set_xtalk_margin')
        raw = int(round(margin_kcps * 2048))
        self.dci_replace_data(DCI_XTALK_CFG, 16, struct.pack('<I', raw & 0xFFFFFFFF), 0x00)

    def get_caldata_xtalk(self):
        """Read the live 776-byte xtalk calibration blob back from the sensor
        FW (as produced by calibrate_xtalk). Restores the current resolution
        afterwards."""
        footer = bytes([0x00, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x03, 0x04])
        resolution = self.get_resolution()
        self.set_resolution(RESOLUTION_8X8)
        self.p.wr_multi(0x2FB8, GET_XTALK_CMD)
        self._poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, 'get xtalk')
        buf = self.p.rd_multi(UI_CMD_START, XTALK_BUFFER_SIZE + 4)
        out = bytearray(XTALK_BUFFER_SIZE)
        out[0:XTALK_BUFFER_SIZE - 8] = buf[8:XTALK_BUFFER_SIZE]
        out[XTALK_BUFFER_SIZE - 8:XTALK_BUFFER_SIZE] = footer
        self.xtalk_data = bytes(out)
        self.set_resolution(resolution)
        return self.xtalk_data

    def set_caldata_xtalk(self, xtalk_data):
        """Restore a previously saved 776-byte xtalk calibration blob. The blob
        is re-uploaded to the FW by the next set_resolution()/start_ranging()."""
        if len(xtalk_data) != XTALK_BUFFER_SIZE:
            raise Vl53l8cxError(STATUS_INVALID_PARAM,
                                f'xtalk blob must be {XTALK_BUFFER_SIZE} bytes')
        resolution = self.get_resolution()
        self.xtalk_data = bytes(xtalk_data)
        self.set_resolution(resolution)

    # ---------------- detection thresholds (plugin) ----------------

    def get_detection_thresholds_enable(self):
        return self.dci_read_data(DCI_DET_THRESH_GLOBAL_CONFIG, 8)[1]

    def set_detection_thresholds_enable(self, enabled):
        grp = bytearray([0x01, 0x00, 0x01, 0x00])
        if enabled:
            grp[1] = 0x01
            tmp = 0x04
        else:
            grp[1] = 0x00
            tmp = 0x0C
        self.dci_replace_data(DCI_DET_THRESH_GLOBAL_CONFIG, 8, bytes(grp), 0x00)
        self.dci_replace_data(DCI_DET_THRESH_CONFIG, 20, bytes([tmp]), 0x11)

    def get_detection_thresholds(self):
        """Return the 64 detection thresholds as a list of dicts. Low/high are
        rescaled to real units for their measurement type."""
        raw = self.dci_read_data(DCI_DET_THRESH_START, NB_THRESHOLDS * 12)
        out = []
        for k in range(NB_THRESHOLDS):
            low, high, meas, typ, zone, op = struct.unpack_from('<iiBBBB', raw, k * 12)
            scale = _THRESH_SCALE.get(meas, 1)
            out.append({
                'low_thresh': low // scale,
                'high_thresh': high // scale,
                'measurement': meas,
                'type': typ,
                'zone_num': zone,
                'operation': op,
            })
        return out

    def set_detection_thresholds(self, thresholds):
        """Program the 64 detection thresholds. `thresholds` is a list of dicts
        (see get_detection_thresholds); missing entries default to zeros."""
        valid = bytes([0x05] * 8)
        packed = bytearray(NB_THRESHOLDS * 12)
        for k in range(NB_THRESHOLDS):
            t = thresholds[k] if k < len(thresholds) else {}
            meas = int(t.get('measurement', 0))
            scale = _THRESH_SCALE.get(meas, 1)
            low = int(t.get('low_thresh', 0)) * scale
            high = int(t.get('high_thresh', 0)) * scale
            struct.pack_into('<iiBBBB', packed, k * 12,
                             low, high, meas,
                             int(t.get('type', 0)), int(t.get('zone_num', 0)),
                             int(t.get('operation', 0)))
        self.dci_write_data(DCI_DET_THRESH_VALID_STATUS, valid)
        self.dci_write_data(DCI_DET_THRESH_START, bytes(packed))

    def set_detection_thresholds_auto_stop(self, auto_stop):
        self.dci_replace_data(DCI_PIPE_CONTROL, 4,
                              bytes([1 if auto_stop else 0]), 0x03)

    # ---------------- motion indicator (plugin) ----------------

    def motion_indicator_init(self, resolution):
        """Initialize a motion-indicator configuration (default distance
        window) and write it to the sensor. Enables the motion output block so
        subsequent frames carry motion data."""
        cfg = MotionConfig()
        cfg.ref_bin_offset = 13633
        cfg.detection_threshold = 2883584
        cfg.extra_noise_sigma = 0
        cfg.null_den_clip_value = 0
        cfg.mem_update_mode = 6
        cfg.mem_update_choice = 2
        cfg.sum_span = 4
        cfg.feature_length = 9
        cfg.nb_of_aggregates = 16
        cfg.nb_of_temporal_accumulations = 16
        cfg.min_nb_for_global_detection = 1
        cfg.global_indicator_format_1 = 8
        cfg.global_indicator_format_2 = 0
        self.motion_indicator_set_resolution(cfg, resolution)
        self._motion_present = True
        return cfg

    def motion_indicator_set_resolution(self, cfg, resolution):
        if resolution == RESOLUTION_4X4:
            for i in range(16):
                cfg.map_id[i] = i
            for i in range(16, 64):
                cfg.map_id[i] = -1
        elif resolution == RESOLUTION_8X8:
            for i in range(64):
                cfg.map_id[i] = ((i % 8) // 2) + (4 * (i // 16))
        else:
            raise Vl53l8cxError(STATUS_INVALID_PARAM, 'motion set_resolution')
        self.dci_write_data(DCI_MOTION_DETECTOR_CFG, cfg.pack())

    def motion_indicator_set_distance_motion(self, cfg, distance_min_mm, distance_max_mm):
        if (distance_max_mm - distance_min_mm) > 1500 \
                or distance_min_mm < 400 or distance_max_mm > 4000:
            raise Vl53l8cxError(STATUS_INVALID_PARAM, 'motion set_distance')
        cfg.ref_bin_offset = int(((distance_min_mm / 37.5348) - 4.0) * 2048.5)
        cfg.feature_length = int(
            (((distance_max_mm - distance_min_mm) / 10.0 + 30.02784) / 15.01392) + 0.5)
        self.dci_write_data(DCI_MOTION_DETECTOR_CFG, cfg.pack())

    # ---------------- xtalk calibration run (plugin) ----------------

    def _program_output_config(self, resolution, cnh_data_size=None):
        """Program the sensor output list/config/enables for `resolution`.
        Self-contained copy of the block start_ranging() uses inline (kept
        separate so start_ranging stays byte-identical for the replay gate);
        used by calibrate_xtalk. Returns data_read_size."""
        data_read_size = 0
        output_bh_enable = [0x00000FFF, 0, 0, self._output_enable_w3]
        output = [START_BH, METADATA_BH, COMMONDATA_BH, AMBIENT_RATE_BH,
                  SPAD_COUNT_BH, NB_TARGET_DETECTED_BH, SIGNAL_RATE_BH,
                  RANGE_SIGMA_MM_BH, DISTANCE_BH, REFLECTANCE_BH,
                  TARGET_STATUS_BH, MOTION_DETECT_BH]
        if cnh_data_size is not None:
            cnh_bh = (CNH_DATA_IDX << 16) | (((cnh_data_size // 4) & 0xFFF) << 4) | 4
            output.append(cnh_bh)
            output_bh_enable[0] |= (1 << (len(output) - 1))
        for i in range(len(output)):
            if output[i] == 0 or not (output_bh_enable[i // 32] & (1 << (i % 32))):
                continue
            bh_type, bh_size, bh_idx = _bh_fields(output[i])
            if 0x1 <= bh_type < 0x0D:
                if 0x54D0 <= bh_idx < 0x54D0 + 960:
                    bh_size = resolution
                elif bh_idx == CNH_DATA_IDX:
                    pass
                else:
                    bh_size = resolution * NB_TARGET_PER_ZONE
                output[i] = _bh_set_size(output[i], bh_size)
                data_read_size += bh_type * bh_size
            else:
                data_read_size += bh_size
            data_read_size += 4
        data_read_size += self._frame_tail
        self.dci_write_data(DCI_OUTPUT_LIST,
                            struct.pack('<%dI' % len(output), *output))
        self.dci_write_data(DCI_OUTPUT_CONFIG,
                            struct.pack('<2I', data_read_size, len(output) + 1))
        self.dci_write_data(DCI_OUTPUT_ENABLES,
                            struct.pack('<4I', *output_bh_enable))
        return data_read_size

    def calibrate_xtalk(self, reflectance_percent, nb_samples, distance_mm):
        """vl53l8cx_calibrate_xtalk: run on-device crosstalk calibration.

        NOTE: ported from ST ULD source but NOT verified against live hardware
        in this SDK — the get/set caldata-xtalk buffer path IS the tested
        save/restore route. Saves & restores resolution/frequency/int-time/
        sharpener/target-order/xtalk-margin/ranging-mode around the run."""
        if not (1 <= reflectance_percent <= 99):
            raise Vl53l8cxError(STATUS_INVALID_PARAM, 'reflectance 1..99')
        if not (600 <= distance_mm <= 3000):
            raise Vl53l8cxError(STATUS_INVALID_PARAM, 'distance 600..3000')
        if not (1 <= nb_samples <= 16):
            raise Vl53l8cxError(STATUS_INVALID_PARAM, 'nb_samples 1..16')
        footer = bytes([0x00, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x03, 0x04])
        cmd = bytes([0x00, 0x03, 0x00, 0x00])
        # save current config
        saved = {
            'resolution': self.get_resolution(),
            'frequency': self.get_ranging_frequency_hz(),
            'integration': self.get_integration_time_ms(),
            'sharpener': self.get_sharpener_percent(),
            'target_order': self.get_target_order(),
            'xtalk_margin': self.get_xtalk_margin(),
            'ranging_mode': self.get_ranging_mode(),
        }
        self.set_resolution(RESOLUTION_8X8)
        self.p.wr_multi(0x2C28, CALIBRATE_XTALK)
        self._poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, 'calib cmd')
        refl = reflectance_percent * 16
        dist = distance_mm * 4
        self.dci_replace_data(DCI_CAL_CFG, 8, struct.pack('<H', dist), 0x00)
        self.dci_replace_data(DCI_CAL_CFG, 8, struct.pack('<H', refl), 0x02)
        self.dci_replace_data(DCI_CAL_CFG, 8, bytes([nb_samples]), 0x04)
        self._program_output_config(RESOLUTION_8X8)
        self.p.wr_multi(UI_CMD_END - 3, cmd)
        self._poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, 'calib start')
        timeout = 0
        while True:
            buf = self.p.rd_multi(0x0, 4)
            if buf[0] != STATUS_ERROR:
                if buf[2] >= 0x7F and ((buf[3] & 0x80) >> 7) == 1:
                    self.xtalk_data = self.default_xtalk  # XTALK_FAILED
                break
            if timeout >= 400:
                raise Vl53l8cxError(STATUS_ERROR, 'xtalk calibration')
            self.p.sleep_ms(50)
            timeout += 1
        # read back the computed xtalk buffer
        self.p.wr_multi(0x2FB8, GET_XTALK_CMD)
        self._poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, 'get xtalk')
        buf = self.p.rd_multi(UI_CMD_START, XTALK_BUFFER_SIZE + 4)
        out = bytearray(XTALK_BUFFER_SIZE)
        out[0:XTALK_BUFFER_SIZE - 8] = buf[8:XTALK_BUFFER_SIZE]
        out[XTALK_BUFFER_SIZE - 8:XTALK_BUFFER_SIZE] = footer
        self.xtalk_data = bytes(out)
        self.p.wr_multi(0x2C34, self.default_cfg)
        self._poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, 'restore cfg')
        # restore saved config
        self.set_resolution(saved['resolution'])
        self.set_ranging_frequency_hz(saved['frequency'])
        self.set_integration_time_ms(saved['integration'])
        self.set_sharpener_percent(saved['sharpener'])
        self.set_target_order(saved['target_order'])
        self.set_xtalk_margin(saved['xtalk_margin'])
        self.set_ranging_mode(saved['ranging_mode'])
