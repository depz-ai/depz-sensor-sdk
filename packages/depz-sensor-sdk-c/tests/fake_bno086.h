/* fake_bno086.h — in-process fake BNO086 bridge firmware plus a minimal SH-2
 * hub over a loopback link: RESET (then the executable reset-complete),
 * WAKE, SEND_SHTP_PACKET (with scripted ERR_BUSY), and on the control channel
 * Product ID, Set / Get Feature (Set answers unsolicited, as the real hub
 * does), the ME calibration / DCD / oscillator / errors / counts / tare
 * commands and FRS read / write over an in-memory record store. */
#ifndef DEPZ_TEST_FAKE_BNO086_H
#define DEPZ_TEST_FAKE_BNO086_H

#include "depz_sensor_io.h"

typedef struct fake_bno086 fake_bno086;

#define FAKE_BNO086_PART 10004563u

fake_bno086 *fake_bno086_start(depz_link **host_link);
void         fake_bno086_stop(fake_bno086 *f);

/* The next `n` SEND_SHTP_PACKET get ERR_BUSY (the frame is dropped). */
void     fake_bno086_busy_next(fake_bno086 *f, int n);
/* SEND_SHTP_PACKET frames accepted / refused busy so far. */
unsigned fake_bno086_frames_accepted(fake_bno086 *f);
unsigned fake_bno086_frames_busy(fake_bno086 *f);
/* Hold each disable's unsolicited Get Feature Response back and deliver it
 * just before the next one: the stale answer enable()'s read-back must skip. */
void     fake_bno086_delay_disable_answer(fake_bno086 *f, bool on);
/* The report interval the hub has for `sensor` (0 = off). */
uint32_t fake_bno086_interval(fake_bno086 *f, uint8_t sensor);
/* Seed / read an FRS record (at most 32 words). */
void     fake_bno086_set_record(fake_bno086 *f, uint16_t type, const uint32_t *words, size_t n);
size_t   fake_bno086_record(fake_bno086 *f, uint16_t type, uint32_t *out, size_t cap);
/* The parameters (9 bytes) of the last Command Request for `command`; false
 * when none came. */
bool     fake_bno086_last_command(fake_bno086 *f, uint8_t command, uint8_t params[9]);
/* ME calibration status the next configure answers with (0 = success). */
void     fake_bno086_calibration_status(fake_bno086 *f, uint8_t status);
/* Push an input cargo on SHTP `channel` as RPT_DATA captured at `capture_us`. */
void     fake_bno086_send_input(fake_bno086 *f, uint8_t channel, uint64_t capture_us, const uint8_t *cargo,
                                size_t len);

#endif
