/* vl53l8_blobs.h — the ST sensor blobs the VL53L8/L5/L7 ULD downloads (see
 * vl53l8_blobs.c, generated). Internal. */
#ifndef DEPZ_IO_VL53L8_BLOBS_H
#define DEPZ_IO_VL53L8_BLOBS_H

#include <stddef.h>
#include <stdint.h>

extern const uint8_t depz_vl53l8_fw_cx[], depz_vl53l8_fw_ch[], depz_vl53l8_fw_l7cx[];
extern const size_t depz_vl53l8_fw_cx_len, depz_vl53l8_fw_ch_len, depz_vl53l8_fw_l7cx_len;
extern const uint8_t depz_vl53l8_cfg_cx[], depz_vl53l8_cfg_ch[], depz_vl53l8_cfg_l7cx[],
    depz_vl53l8_cfg_l7ch[];
extern const size_t depz_vl53l8_cfg_cx_len, depz_vl53l8_cfg_ch_len, depz_vl53l8_cfg_l7cx_len,
    depz_vl53l8_cfg_l7ch_len;
extern const uint8_t depz_vl53l8_default_xtalk[], depz_vl53l8_get_nvm_cmd[];
extern const size_t depz_vl53l8_default_xtalk_len, depz_vl53l8_get_nvm_cmd_len;
/* ST command tables (GET_XTALK_CMD, the three CALIBRATE_XTALK variants). */
extern const uint8_t depz_vl53l8_get_xtalk_cmd[], depz_vl53l8_calibrate_xtalk_cx[],
    depz_vl53l8_calibrate_xtalk_lmz[], depz_vl53l8_calibrate_xtalk_l7cx[];
extern const size_t depz_vl53l8_get_xtalk_cmd_len, depz_vl53l8_calibrate_xtalk_cx_len,
    depz_vl53l8_calibrate_xtalk_lmz_len, depz_vl53l8_calibrate_xtalk_l7cx_len;

#endif
