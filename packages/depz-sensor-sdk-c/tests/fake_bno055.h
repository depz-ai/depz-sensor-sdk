/* fake_bno055.h — in-process fake APP_BNO055 register bridge plus a
 * register-level model of the BNO055 behind it, over a loopback link
 * (contract 13): the C twin of the Python SDK's tests/fake_bno055.py. The
 * model holds what the host logic depends on: two register pages selected by
 * PAGE_ID, "only CONFIG <-> mode transitions exist" (§4.1), suspend ignoring
 * OPR_MODE writes (§4.6), the zero-quaternion gap after entering fusion
 * (§5), the self-test leftover SYS_STATUS 4, SYS_TRIGGER CLK_SEL and INT_STA
 * clear-on-read. Every register access is logged with the page in force. */
#ifndef DEPZ_TEST_FAKE_BNO055_H
#define DEPZ_TEST_FAKE_BNO055_H

#include "depz_sensor_io.h"

typedef struct fake_bno055 fake_bno055;

fake_bno055 *fake_bno055_start(depz_link **host_link);
void         fake_bno055_stop(fake_bno055 *f);

uint8_t fake_bno055_reg(fake_bno055 *f, int page, uint8_t addr);
void    fake_bno055_set_reg(fake_bno055 *f, int page, uint8_t addr, uint8_t v);
/* Put the model into an operating mode directly (a previous session left it there). */
void    fake_bno055_set_mode(fake_bno055 *f, uint8_t mode);
uint8_t fake_bno055_opr_mode(fake_bno055 *f);
int     fake_bno055_page(fake_bno055 *f);

/* Logs: every host command and register access since the last clear. */
void   fake_bno055_clear(fake_bno055 *f);
/* The writes as "page:addr=hex" joined by single spaces (Python's writes()). */
void   fake_bno055_writes(fake_bno055 *f, char *out, size_t cap);
size_t fake_bno055_reads(fake_bno055 *f, uint8_t addr);
size_t fake_bno055_sent(fake_bno055 *f, uint8_t cmd);
/* The i-th payload of `cmd` (copied, full length returned; 0 = none). */
size_t fake_bno055_payload(fake_bno055 *f, uint8_t cmd, size_t i, uint8_t *out, size_t cap);

/* Knobs. */
void fake_bno055_fail_next(fake_bno055 *f, uint8_t cmd, uint8_t status);
void fake_bno055_fail_write_reg(fake_bno055 *f, uint8_t addr, uint8_t status); /* page 0, once */
void fake_bno055_silent(fake_bno055 *f, uint8_t cmd, bool on);
void fake_bno055_short_read(fake_bno055 *f, size_t by);
/* SYS_STATUS answers consumed one per read before the model: `count` x `v`. */
void fake_bno055_sys_status(fake_bno055 *f, uint8_t v, unsigned count);
unsigned fake_bno055_sys_status_left(fake_bno055 *f);
void fake_bno055_fusion_stuck(fake_bno055 *f, bool on);
void fake_bno055_set_info(fake_bno055 *f, const uint8_t *info, size_t len);
int  fake_bno055_reset_count(fake_bno055 *f);
bool fake_bno055_streaming(fake_bno055 *f);
/* RPT_BNO_REG_STREAM: ts u64, addr, len (`len_field`), data. */
void fake_bno055_push_stream(fake_bno055 *f, uint64_t ts, uint8_t addr, uint8_t len_field,
                             const uint8_t *data, size_t len);

#endif
