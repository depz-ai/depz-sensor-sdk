/* fake_vl53l8.h — in-process fake APP_VL53L8 SPI register bridge (contract
 * 04) over a loopback link, with a model of the VL53L8 chip behind it: the
 * C twin of the chip half of the Python SDK's tests/fake_vl53l8.py
 * (FakeVl53l8Sensor) — paged register map (0x7FFF), firmware banks (pages
 * 9..11, discarded), the page-2 UI command area with the DCI store, the NVM
 * / xtalk read-backs, the offset / xtalk / configuration uploads, the power
 * registers and the xtalk calibration run. Its frames carry the armed output
 * list's block headers with zero data — the layout, not the measurements (the
 * contract 15 scenarios replay real ones). */
#ifndef DEPZ_TEST_FAKE_VL53L8_H
#define DEPZ_TEST_FAKE_VL53L8_H

#include "depz_sensor_io.h"

typedef struct fake_vl53l8 fake_vl53l8;

/* `fw_crc` is what the booted firmware reports at 0x2FFC (page 2): the
 * checksum of the blob the host downloads. */
fake_vl53l8 *fake_vl53l8_start(uint32_t fw_crc, depz_link **host_link);
void         fake_vl53l8_stop(fake_vl53l8 *f);

uint8_t fake_vl53l8_reg(fake_vl53l8 *f, int page, uint16_t addr);
void    fake_vl53l8_set_reg(fake_vl53l8 *f, int page, uint16_t addr, uint8_t v);
int     fake_vl53l8_page(fake_vl53l8 *f);
/* The DCI entry `index` as the firmware holds it (unswapped): copies up to
 * cap bytes, returns its length (0 = never written nor defaulted). */
size_t  fake_vl53l8_dci(fake_vl53l8 *f, uint16_t index, uint8_t *out, size_t cap);

size_t fake_vl53l8_write_count(fake_vl53l8 *f);   /* WRITE_REG packets received */
int    fake_vl53l8_boots(fake_vl53l8 *f);         /* MCU resets (one per init) */
int    fake_vl53l8_config_uploads(fake_vl53l8 *f);
int    fake_vl53l8_offset_uploads(fake_vl53l8 *f);
int    fake_vl53l8_xtalk_uploads(fake_vl53l8 *f);
/* The last xtalk upload (776 bytes); false if none. */
bool   fake_vl53l8_last_xtalk(fake_vl53l8 *f, uint8_t out[776]);
/* Calibration runs: their count, the last table length and the last
 * (distance x4, reflectance x16, nb_samples) the FW read from DCI_CAL_CFG. */
int    fake_vl53l8_calibrations(fake_vl53l8 *f, size_t *table_len, uint16_t *dist4, uint16_t *refl16, uint8_t *nb);
/* The signal-grid entry the fake's calibration computes for zone z. */
uint32_t fake_vl53l8_xtalk_grid_value(unsigned distance_mm, unsigned reflectance_pc, unsigned nb, unsigned zone);
bool   fake_vl53l8_ranging(fake_vl53l8 *f);
/* frame_size of the last START_STREAM (0 = none yet). */
uint16_t fake_vl53l8_frame_size(fake_vl53l8 *f);
/* One frame of the armed output list as chunked RPT_VL53_FRAMEs: the
 * enabled block headers, zero data, the head / footer ids. False when not
 * streaming or no output list was configured. */
bool   fake_vl53l8_send_frame(fake_vl53l8 *f);

/* Knobs. */
void fake_vl53l8_xtalk_cal_fails(fake_vl53l8 *f, bool on);  /* "coverglass too good" */
void fake_vl53l8_laser_safety_fault(fake_vl53l8 *f, bool on);
void fake_vl53l8_fail_next(fake_vl53l8 *f, uint8_t cmd, uint8_t status);
void fake_vl53l8_silent_next(fake_vl53l8 *f, uint8_t cmd);             /* no answer at all */

#endif
