/* fake_sr04.h — in-process fake SR04 firmware over a loopback link, the C
 * twin of the Python SDK's tests/fake_device.py: same reply kinds, same busy
 * semantics, same RPT_DATA layout (usonic_sr04 cmd_handler.c). */
#ifndef DEPZ_TEST_FAKE_SR04_H
#define DEPZ_TEST_FAKE_SR04_H

#include "depz_sensor_io.h"

typedef struct fake_sr04 fake_sr04;

#define FAKE_SR04_SOFTWARE "APP_usonic_SR04_v0.95"
#define FAKE_SR04_NAME     "DEPZ Usonic SR04"
#define FAKE_SR04_SERIAL   "SN0042"

/* Start the fake; *host_link is the end the SDK opens (the SDK owns it). */
fake_sr04 *fake_sr04_start(depz_link **host_link);
/* A silent fake: reads and ignores everything (timeout tests). */
fake_sr04 *fake_silent_start(depz_link **host_link);
void       fake_sr04_stop(fake_sr04 *f);

/* Device-side state, read/written by tests. */
uint32_t fake_sr04_sample_period(fake_sr04 *f);
void     fake_sr04_set_echo(fake_sr04 *f, uint16_t echo_us);
bool     fake_sr04_loop_running(fake_sr04 *f);
/* Emit one RPT_DATA as the firmware's ISR would (source 0x36 or 0x37). */
void     fake_sr04_send_measurement(fake_sr04 *f, uint8_t source_cmd);
/* Emit raw bytes (unsolicited reports, garbage). */
void     fake_sr04_send_raw(fake_sr04 *f, uint8_t cmd, const uint8_t *payload, size_t len);

#endif
