/* fake_vl53lx.h — in-process fake APP_VL53L0_4 register bridge (contract 12,
 * protocol v2.01) over a loopback link, the C twin of the Python SDK's
 * tests/fake_vl53lx.py: a 64 KiB register file pre-loaded with what a booted
 * VL53L1-family die answers (boot status 3, oscillator 0xBCCC, model id
 * 0xEBAA, PLL 0x0123, ROI centre 0x91), enough for the host ULDs (L4CD, L3CX
 * ULP, L1X) to run init, timing, offset, xtalk, thresholds, calibrations and
 * ranging against it. The result block (0x0089, 17 B) and the data-ready bit
 * are plain registers. Every packet the host sends is logged, so tests assert
 * the exact wire bytes. */
#ifndef DEPZ_TEST_FAKE_VL53LX_H
#define DEPZ_TEST_FAKE_VL53LX_H

#include "depz_sensor_io.h"

typedef struct fake_vl53lx fake_vl53lx;

#define FAKE_VLX_DIE_OSC 0xBCCCu
#define FAKE_VLX_DIE_PLL 0x0123u
#define FAKE_VLX_SILENT  (-1) /* fake_vl53lx_fail_next: no reply at all */

/* `device_name` is GET_DEVICE_NAME's answer (the product stamp). */
fake_vl53lx *fake_vl53lx_start(const char *device_name, depz_link **host_link);
void         fake_vl53lx_stop(fake_vl53lx *f);

uint8_t  fake_vl53lx_reg(fake_vl53lx *f, uint16_t addr);
uint16_t fake_vl53lx_word(fake_vl53lx *f, uint16_t addr);  /* big-endian */
uint32_t fake_vl53lx_dword(fake_vl53lx *f, uint16_t addr);
void     fake_vl53lx_set_reg(fake_vl53lx *f, uint16_t addr, uint8_t v);
/* The 17-byte die block at 0x0089 (what the next poll reads). */
void     fake_vl53lx_set_block(fake_vl53lx *f, const uint8_t *raw, size_t len);
/* Answer the next `cmd` with RPT_STATUS `status`, or not at all (FAKE_VLX_SILENT). */
void     fake_vl53lx_fail_next(fake_vl53lx *f, uint8_t cmd, int status);

/* The packet log (every host packet since the last clear, in order). */
void   fake_vl53lx_clear(fake_vl53lx *f);
size_t fake_vl53lx_rx_count(fake_vl53lx *f);
size_t fake_vl53lx_count(fake_vl53lx *f, uint8_t cmd);
/* The i-th payload of `cmd` copied to out; returns its full length (0 = none). */
size_t fake_vl53lx_sent(fake_vl53lx *f, uint8_t cmd, size_t i, uint8_t *out, size_t cap);

/* RPT_VL53_STREAM: ts u64, addr u16, len u16 (`len_field`), then `data`. */
void fake_vl53lx_stream(fake_vl53lx *f, uint64_t ts, uint16_t len_field, const uint8_t *data, size_t len);

#endif
