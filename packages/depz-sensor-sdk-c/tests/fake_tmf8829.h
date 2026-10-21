/* fake_tmf8829.h — in-process fake APP_TMF8829 bridge plus a scripted
 * TMF8829 over a loopback link: the 256-byte register map with the FIFO port
 * at 0xFF (reads and writes stay there), soft reset into the sensor
 * bootloader (I2C_OFF, W_FIFO_BOTH, the image into the FIFO, START_RAM_APP),
 * the application's presets / LOAD_CONFIG_PAGE / WRITE_PAGE / MEASURE / STOP
 * (each answers "busy" on the first CMD_STAT read), and the bridge's
 * START_STREAM / STOP_STREAM / GET_INFO. On MEASURE with the stream armed it
 * plays the scripted frames as RPT_TMF_FRAME chunks of <= 1024 bytes. */
#ifndef DEPZ_TEST_FAKE_TMF8829_H
#define DEPZ_TEST_FAKE_TMF8829_H

#include "depz_sensor_io.h"

typedef struct fake_tmf8829 fake_tmf8829;

#define FAKE_TMF8829_SERIAL 0x12345678u

fake_tmf8829 *fake_tmf8829_start(depz_link **host_link);
void          fake_tmf8829_stop(fake_tmf8829 *f);

/* The config page a preset load produces (fp_mode and kilo_iterations are
 * then set by the preset). */
void    fake_tmf8829_set_base_page(fake_tmf8829 *f, const uint8_t page[190]);
/* WRITE_PAGE silently keeps fov_correction at 5 (as measured on the sensor). */
void    fake_tmf8829_pin_fov_correction(fake_tmf8829 *f, bool on);
/* Frames played on the next MEASURE: raw bytes from 0xFA on, MCU time of the
 * header read, INT edge. Copied. */
void    fake_tmf8829_add_frame(fake_tmf8829 *f, const uint8_t *raw, size_t len, uint64_t ts, uint64_t edge_us);
void    fake_tmf8829_clear_frames(fake_tmf8829 *f);
/* Skip one frame_seq number before the k-th scripted frame (a dropped frame). */
void    fake_tmf8829_skip_seq_before(fake_tmf8829 *f, int k);
/* STOP answers with one EV_BAD_HEADER event first (as mid histogram dump). */
void    fake_tmf8829_bad_header_on_stop(fake_tmf8829 *f, bool on);
/* Push an RPT_TMF_EVENT now. */
void    fake_tmf8829_send_event(fake_tmf8829 *f, uint8_t int_status, uint8_t code);

uint8_t fake_tmf8829_reg(fake_tmf8829 *f, uint8_t addr);
void    fake_tmf8829_set_reg(fake_tmf8829 *f, uint8_t addr, uint8_t v);
/* The image written to the FIFO since the last W_FIFO_BOTH; its words / load
 * address parameters; the largest and the number of FIFO writes. */
size_t  fake_tmf8829_image(fake_tmf8829 *f, const uint8_t **data);
void    fake_tmf8829_fifo_params(fake_tmf8829 *f, uint32_t *addr, uint16_t *words, size_t *writes,
                                 size_t *max_write);
/* The page the sensor holds (WRITE_PAGE result). */
void    fake_tmf8829_page(fake_tmf8829 *f, uint8_t out[190]);
/* READ_REG requests seen: their addresses / lengths, in order. */
size_t  fake_tmf8829_reads(fake_tmf8829 *f, uint8_t *addr, uint16_t *len, size_t cap);
void    fake_tmf8829_clear_reads(fake_tmf8829 *f);
/* START_STREAM arguments of the armed stream; false when not armed. */
bool    fake_tmf8829_armed(fake_tmf8829 *f, uint8_t *int_mask, uint8_t *flags);

#endif
