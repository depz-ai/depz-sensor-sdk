/* fake_vl53l4.h — in-process fake VL53L4CD bridge firmware over a loopback
 * link: a byte-addressed register map seeded like a booted sensor (the C
 * twin of the Python SDK's tests/fake_vl53l4.py), answering READ_REG /
 * WRITE_REG / SET_I2C_SPEED / XSHUT / START_STREAM / STOP_STREAM / GET_INFO,
 * and SET_ADDR_WIDTH / CLEAR_I2C_ERRORS, so the APP_VL53L0_4 family's L1X ULD
 * (depz_vl53lx_*) runs on the same register file.
 * The default configuration block leaves the sensor "data ready", which is
 * what the ULD's VHV wait needs. */
#ifndef DEPZ_TEST_FAKE_VL53L4_H
#define DEPZ_TEST_FAKE_VL53L4_H

#include "depz_sensor_io.h"

typedef struct fake_vl53l4 fake_vl53l4;

#define FAKE_VL53L4_OSC_FREQUENCY 0x3980u
#define FAKE_VL53L4_CLOCK_PLL     0x0A5Cu

fake_vl53l4 *fake_vl53l4_start(depz_link **host_link);
void         fake_vl53l4_stop(fake_vl53l4 *f);

uint8_t fake_vl53l4_reg(fake_vl53l4 *f, uint16_t addr);
void    fake_vl53l4_set_reg(fake_vl53l4 *f, uint16_t addr, uint8_t v);
/* SET_I2C_SPEED history. */
size_t  fake_vl53l4_speeds(fake_vl53l4 *f, uint16_t *out, size_t cap);
/* Every write that started exactly at `addr`, in order: returns how many; the
 * i-th one's bytes are copied by fake_vl53l4_write_at. */
size_t  fake_vl53l4_writes_at(fake_vl53l4 *f, uint16_t addr);
size_t  fake_vl53l4_write_at(fake_vl53l4 *f, uint16_t addr, size_t i, uint8_t *out, size_t cap);
bool    fake_vl53l4_streaming(fake_vl53l4 *f);
/* Answer the next `cmd` with STATUS `st` instead of serving it (one shot). */
void    fake_vl53l4_fail_next(fake_vl53l4 *f, uint8_t cmd, uint8_t st);
/* As fail_next, and once that failure is sent, fail the next `then_cmd` with
 * `then_st` too. */
void    fake_vl53l4_fail_next_then(fake_vl53l4 *f, uint8_t cmd, uint8_t st, uint8_t then_cmd,
                                   uint8_t then_st);
/* Emit one RPT_VL53_STREAM carrying `block` as the ISR would. */
void    fake_vl53l4_send_stream(fake_vl53l4 *f, uint64_t ts, const uint8_t *block, size_t len);

#endif
