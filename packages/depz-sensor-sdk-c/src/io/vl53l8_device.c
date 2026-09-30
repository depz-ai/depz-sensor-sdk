/* vl53l8_device.c — the multizone ToF class (VL53L8CX / VL53L8CH, contract 04):
 * the ST ULD ported register for register from the Python SDK's vl53l8/uld.py
 * (itself a port of vl53l8cx_api.c + plugins, BSD-3-Clause), over the SPI
 * register bridge. Poll loops, DCI sequences and integer widths match it
 * exactly, so a capture made by either SDK replays in the other byte for
 * byte. Frame parsing reuses the codec layer (src/vl53l8_decode.c). */
#include "io_internal.h"
#include "vl53l8_blobs.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── ULD constants (vl53l8cx_api.h) ──────────────────────────────────────── */

#define STATUS_ERROR 255

#define START_BH              0x0000000Du
#define METADATA_BH           0x54B400C0u
#define COMMONDATA_BH         0x54C00040u
#define AMBIENT_RATE_BH       0x54D00104u
#define SPAD_COUNT_BH         0x55D00404u
#define NB_TARGET_DETECTED_BH 0xDB840401u
#define SIGNAL_RATE_BH        0xDBC40404u
#define RANGE_SIGMA_MM_BH     0xDEC40402u
#define DISTANCE_BH           0xDF440402u
#define REFLECTANCE_BH        0xE0440401u
#define TARGET_STATUS_BH      0xE0840401u
#define MOTION_DETECT_BH      0xD85808C0u
#define CNH_DATA_IDX          0xC048u
#define MOTION_DETEC_IDX      0xD858u

#define NVM_DATA_SIZE      492
#define OFFSET_BUFFER_SIZE 488
#define XTALK_BUFFER_SIZE  776

#define DCI_ZONE_CONFIG              0x5450
#define DCI_FREQ_HZ                  0x5458
#define DCI_INT_TIME                 0x545C
#define DCI_CAL_CFG                  0x5470
#define DCI_DET_THRESH_CONFIG        0x5488
#define DCI_RANGING_MODE             0xAD30
#define DCI_DSS_CONFIG               0xAD38
#define DCI_XTALK_CFG                0xAD94
#define DCI_TARGET_ORDER             0xAE64
#define DCI_SHARPENER                0xAED8
#define DCI_DET_THRESH_GLOBAL_CONFIG 0xB6E0
#define DCI_DET_THRESH_START         0xB6E8
#define DCI_DET_THRESH_VALID_STATUS  0xB9F0
#define DCI_MOTION_DETECTOR_CFG      0xBFAC
#define DCI_SINGLE_RANGE             0xD964
#define DCI_OUTPUT_CONFIG            0xD968
#define DCI_OUTPUT_ENABLES           0xD970
#define DCI_OUTPUT_LIST              0xD980
#define DCI_PIPE_CONTROL             0xDB80
#define DCI_FW_FLAGS                 0xE0C4

#define UI_CMD_STATUS 0x2C00
#define UI_CMD_START  0x2C04
#define UI_CMD_END    0x2FFF

#define NB_TARGET_PER_ZONE 1

/* The blob sets (uld.py VARIANT_DATA_DIR). */
typedef enum { V_CX, V_CH, V_L7CX, V_L7CH } variant;


static const uint32_t XTALK_CAL_OUTPUT[17] = {
    0x0000000D, 0x54000040, 0x9FD800C0, 0x9FE40140, 0x9FF80040, 0x9FFC0404,
    0xA0FC0100, 0xA10C0100, 0xA11C00C0, 0xA1280902, 0xA2480040, 0xA24C0081,
    0xA2540081, 0xA25C0081, 0xA2640081, 0xA26C0084, 0xA28C0082};
static const uint32_t XTALK_CAL_OUTPUT_ENABLE[4] = {0x0001FFFF, 0, 0, 0xC0000000};

/* ── state ─────────────────────────────────────────────────────────────── */

typedef struct {
    depz_regbridge rb;
    depz_vl53l8_model model;
    variant v;
    bool i2c;                   /* the L5/L7 branch of the ULD (later models) */
    uint8_t min_hz;
    /* ULD state (uld.py VL53L8CX instance attributes) */
    const uint8_t *firmware, *default_cfg;
    size_t default_cfg_len;
    uint32_t fw_checksum;
    bool has_checksum;
    uint32_t output_enable_w3, frame_tail;
    uint8_t offset_data[OFFSET_BUFFER_SIZE];
    uint8_t xtalk_data[XTALK_BUFFER_SIZE];
    uint32_t data_read_size;
    int resolution;             /* of the last start_ranging */
    int module_type;            /* I2C variants, after init (-1 unknown) */
    bool xtalk_calibration_failed;
    bool cnh_armed;
    size_t cnh_bytes;
    depz_vl53l8ch_cnh_config cnh_decode;

    depz_mutex lock;            /* the flags below, parse state, counters */
    bool initialized, ranging, motion_present;
    uint64_t parse_errors;
    depz_vl53l8_reassembler reasm; /* reader thread only while ranging */
    depz_vl53l8_live_frame scratch;

    depz_cb_list cbs;
    depz_hub *hub;
    depz_vl53l8_progress_cb progress;
    void *progress_user;
} l8_state;

typedef void (*l8_fn)(const depz_vl53l8_live_frame *, void *);

static l8_state *st(depz_device *dev) { return (l8_state *)dev->sensor; }

#define TRY(expr) do { int rc_ = (expr); if (rc_) return rc_; } while (0)

/* ── little helpers ───────────────────────────────────────────────────── */

static uint32_t rd_u32le(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void wr_u32le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

/* VL53L8CX_SwapBuffer: byte-reverse every 32-bit word (a trailing partial
 * word is left as is). */
static void swap_buffer(uint8_t *d, size_t n)
{
    size_t i;
    for (i = 0; i + 4 <= n; i += 4) {
        uint8_t a = d[i], b = d[i + 1];
        d[i] = d[i + 3];
        d[i + 1] = d[i + 2];
        d[i + 2] = b;
        d[i + 3] = a;
    }
}

static void note(depz_device *dev, const char *text)
{
    l8_state *s = st(dev);
    if (s->progress) s->progress(text, 0, 0, s->progress_user);
}

static void on_write_progress(size_t done, size_t total, void *user)
{
    l8_state *s = (l8_state *)user;
    if (s->progress) s->progress("download", done, total, s->progress_user);
}

static int uld_fail(int code, const char *where)
{
    const char *name = code == 1 ? "TIMEOUT" : code == 2 ? "CORRUPTED_FRAME" : code == 3 ? "LASER_SAFETY"
                     : code == 5 ? "FW_CHECKSUM_FAIL" : code == 66 ? "MCU_ERROR"
                     : code == 127 ? "INVALID_PARAM" : "ERROR";
    return depz_fail(code == 1 ? DEPZ_E_TIMEOUT : code == 127 ? DEPZ_E_ARG : DEPZ_E_PROTOCOL,
                     "VL53L8 %s (code %d) %s", name, code, where);
}

/* ── register access ───────────────────────────────────────────────────── */

static int rd(depz_device *dev, uint16_t a, uint8_t *buf, size_t n) { return depz_rb_read(&st(dev)->rb, a, buf, n); }
static int wr(depz_device *dev, uint16_t a, const uint8_t *buf, size_t n) { return depz_rb_write(&st(dev)->rb, a, buf, n); }
static int rd_byte(depz_device *dev, uint16_t a, uint8_t *v) { return rd(dev, a, v, 1); }
static int wr_byte(depz_device *dev, uint16_t a, uint8_t v) { return wr(dev, a, &v, 1); }

/* _vl53l8cx_poll_for_answer(): poll `size` bytes at `address` until
 * buf[pos] & mask == expected. 10 ms period, 2 s timeout. */
static int poll_for_answer(depz_device *dev, size_t size, size_t pos, uint16_t address, uint8_t mask,
                           uint8_t expected, const char *where)
{
    uint8_t buf[16];
    int timeout = 0;
    for (;;) {
        TRY(rd(dev, address, buf, size));
        depz_device_sleep_ms(dev, 10);
        if (timeout >= 200) return uld_fail(1, where);
        if (size >= 4 && buf[2] >= 0x7F) return uld_fail(66, where);
        timeout++;
        if ((buf[pos] & mask) == expected) return DEPZ_OK;
    }
}

static int poll_for_mcu_boot(depz_device *dev)
{
    int timeout = 0;
    while (timeout < 500) {
        uint8_t s0, s1;
        TRY(rd_byte(dev, 0x06, &s0));
        if (s0 & 0x80) {
            TRY(rd_byte(dev, 0x07, &s1));
            if (s1 & 0x01) return DEPZ_OK;
        }
        depz_device_sleep_ms(dev, 1);
        timeout++;
        if (s0 & 0x01) return DEPZ_OK;
    }
    return uld_fail(1, "mcu boot");
}

/* ── DCI access ───────────────────────────────────────────────────────── */

static int dci_read(depz_device *dev, uint16_t index, uint8_t *out, size_t size)
{
    uint8_t cmd[12], *buf;
    char where[32];
    int rc;
    memset(cmd, 0, sizeof cmd);
    cmd[0] = (uint8_t)(index >> 8);
    cmd[1] = (uint8_t)index;
    cmd[2] = (uint8_t)((size & 0xFF0) >> 4);
    cmd[3] = (uint8_t)((size & 0xF) << 4);
    cmd[7] = 0x0F;
    cmd[9] = 0x02;
    cmd[11] = 0x08;
    TRY(wr(dev, UI_CMD_END - 11, cmd, 12));
    snprintf(where, sizeof where, "dci read 0x%04X", index);
    TRY(poll_for_answer(dev, 4, 1, UI_CMD_STATUS, 0xFF, 0x03, where));
    buf = (uint8_t *)malloc(size + 12);
    if (!buf) return depz_fail(DEPZ_E_NOMEM, "dci read: out of memory");
    rc = rd(dev, UI_CMD_START, buf, size + 12);
    if (rc == DEPZ_OK) {
        swap_buffer(buf, size + 12);
        memcpy(out, buf + 4, size);
    }
    free(buf);
    return rc;
}

static int dci_write(depz_device *dev, uint16_t index, const uint8_t *data, size_t size)
{
    uint8_t *buf;
    size_t total = size + 12;
    char where[32];
    int rc;
    buf = (uint8_t *)malloc(total);
    if (!buf) return depz_fail(DEPZ_E_NOMEM, "dci write: out of memory");
    buf[0] = (uint8_t)(index >> 8);
    buf[1] = (uint8_t)index;
    buf[2] = (uint8_t)((size & 0xFF0) >> 4);
    buf[3] = (uint8_t)((size & 0xF) << 4);
    memcpy(buf + 4, data, size);
    swap_buffer(buf + 4, size);
    buf[4 + size + 0] = 0x00;
    buf[4 + size + 1] = 0x00;
    buf[4 + size + 2] = 0x00;
    buf[4 + size + 3] = 0x0F;
    buf[4 + size + 4] = 0x05;
    buf[4 + size + 5] = 0x01;
    buf[4 + size + 6] = (uint8_t)(((size + 8) >> 8) & 0xFF);
    buf[4 + size + 7] = (uint8_t)((size + 8) & 0xFF);
    rc = wr(dev, (uint16_t)(UI_CMD_END - total + 1), buf, total);
    free(buf);
    if (rc) return rc;
    snprintf(where, sizeof where, "dci write 0x%04X", index);
    return poll_for_answer(dev, 4, 1, UI_CMD_STATUS, 0xFF, 0x03, where);
}

static int dci_replace(depz_device *dev, uint16_t index, size_t size, const uint8_t *nd, size_t nlen,
                       size_t pos)
{
    uint8_t buf[1024];
    TRY(dci_read(dev, index, buf, size));
    memcpy(buf + pos, nd, nlen);
    return dci_write(dev, index, buf, size);
}

/* ── offset / xtalk upload ─────────────────────────────────────────────── */

/* Average each 2x2 block of an 8x8 grid down to 4x4, zero the rest (u32 or
 * i16 cells, the grid starting `off` bytes into a swapped buffer). */
static void grid_4x4_u32(uint8_t *buf, size_t off)
{
    uint32_t g[64];
    int i, j, k;
    for (k = 0; k < 64; k++) g[k] = rd_u32le(buf + off + 4 * (size_t)k);
    for (j = 0; j < 4; j++)
        for (i = 0; i < 4; i++)
            g[i + 4 * j] = (uint32_t)(((uint64_t)g[2 * i + 16 * j + 0] + g[2 * i + 16 * j + 1] +
                                       g[2 * i + 16 * j + 8] + g[2 * i + 16 * j + 9]) / 4);
    for (k = 16; k < 64; k++) g[k] = 0;
    for (k = 0; k < 64; k++) wr_u32le(buf + off + 4 * (size_t)k, g[k]);
}

static int32_t floordiv4(int32_t v) { return v >= 0 ? v / 4 : -((-v + 3) / 4); }

static void grid_4x4_i16(uint8_t *buf, size_t off)
{
    int32_t g[64];
    int i, j, k;
    for (k = 0; k < 64; k++) g[k] = (int16_t)(buf[off + 2 * k] | buf[off + 2 * k + 1] << 8);
    for (j = 0; j < 4; j++)
        for (i = 0; i < 4; i++)
            g[i + 4 * j] = floordiv4(g[2 * i + 16 * j + 0] + g[2 * i + 16 * j + 1] +
                                     g[2 * i + 16 * j + 8] + g[2 * i + 16 * j + 9]);
    for (k = 16; k < 64; k++) g[k] = 0;
    for (k = 0; k < 64; k++) {
        int32_t v = g[k] < -32768 ? -32768 : g[k] > 32767 ? 32767 : g[k];
        buf[off + 2 * k] = (uint8_t)v;
        buf[off + 2 * k + 1] = (uint8_t)((uint16_t)v >> 8);
    }
}

static int send_offset_data(depz_device *dev, int resolution)
{
    l8_state *s = st(dev);
    static const uint8_t footer[8] = {0x00, 0x00, 0x00, 0x0F, 0x03, 0x01, 0x01, 0xE4};
    static const uint8_t dss_4x4[8] = {0x0F, 0x04, 0x04, 0x00, 0x08, 0x10, 0x10, 0x07};
    uint8_t buf[OFFSET_BUFFER_SIZE], out[OFFSET_BUFFER_SIZE];
    memcpy(buf, s->offset_data, OFFSET_BUFFER_SIZE);
    if (resolution == DEPZ_VL53L8_RES_4X4) {
        memcpy(buf + 0x10, dss_4x4, 8);
        swap_buffer(buf, sizeof buf);
        grid_4x4_u32(buf, 0x3C);
        grid_4x4_i16(buf, 0x140);
        swap_buffer(buf, sizeof buf);
    }
    /* Shift 8 bytes left (drop the NVM header), footer at 0x1E0. */
    memcpy(out, buf + 8, OFFSET_BUFFER_SIZE - 8);
    memcpy(out + OFFSET_BUFFER_SIZE - 8, footer, 8);
    TRY(wr(dev, 0x2E18, out, sizeof out));
    return poll_for_answer(dev, 4, 1, UI_CMD_STATUS, 0xFF, 0x03, "offset data");
}

static int send_xtalk_data(depz_device *dev, int resolution)
{
    l8_state *s = st(dev);
    static const uint8_t res4x4[8] = {0x0F, 0x04, 0x04, 0x17, 0x08, 0x10, 0x10, 0x07};
    static const uint8_t dss_4x4[8] = {0x00, 0x78, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08};
    static const uint8_t profile_4x4[4] = {0xA0, 0xFC, 0x01, 0x00};
    uint8_t buf[XTALK_BUFFER_SIZE];
    memcpy(buf, s->xtalk_data, XTALK_BUFFER_SIZE);
    if (resolution == DEPZ_VL53L8_RES_4X4) {
        memcpy(buf + 0x08, res4x4, 8);
        memcpy(buf + 0x20, dss_4x4, 8);
        swap_buffer(buf, sizeof buf);
        grid_4x4_u32(buf, 0x34);
        swap_buffer(buf, sizeof buf);
        memcpy(buf + 0x134, profile_4x4, 4);
        memset(buf + 0x078, 0, 4);
    }
    TRY(wr(dev, 0x2CF8, buf, sizeof buf));
    return poll_for_answer(dev, 4, 1, UI_CMD_STATUS, 0xFF, 0x03, "xtalk data");
}

/* ── ULD: identity, init ───────────────────────────────────────────────── */

static int uld_is_alive(depz_device *dev, uint8_t *device_id, uint8_t *revision_id)
{
    TRY(wr_byte(dev, 0x7FFF, 0x00));
    TRY(rd_byte(dev, 0x00, device_id));
    TRY(rd_byte(dev, 0x01, revision_id));
    return wr_byte(dev, 0x7FFF, 0x02);
}

#define W(a, v) TRY(wr_byte(dev, (a), (v)))
#define R(a) TRY(rd_byte(dev, (a), &scratch))

static int uld_init(depz_device *dev)
{
    l8_state *s = st(dev);
    bool i2c = s->i2c;
    uint8_t scratch;
    uint8_t nvm[NVM_DATA_SIZE];
    uint8_t pipe_ctrl[4] = {NB_TARGET_PER_ZONE, 0x00, 0x01, 0x00};
    uint8_t single[4] = {0x01, 0, 0, 0};

    s->module_type = -1;
    if (i2c) {
        uint8_t id, rev;
        TRY(uld_is_alive(dev, &id, &rev));
        if (!(rev == 0x02 || (id == 0xF0 && rev == 0x01)))
            return depz_fail(DEPZ_E_PROTOCOL, "not a VL53L5/L7 sensor: device_id=0x%02X revision_id=0x%02X",
                             id, rev);
    }

    note(dev, "SW reboot...");
    W(0x7FFF, 0x00);
    W(0x0009, 0x04);
    W(0x000F, 0x40);
    W(0x000A, 0x03);
    R(0x7FFF);
    W(0x000C, 0x01);
    W(0x0101, 0x00);
    W(0x0102, 0x00);
    W(0x010A, 0x01);
    W(0x4002, 0x01);
    W(0x4002, 0x00);
    W(0x010A, 0x03);
    W(0x0103, 0x01);
    W(0x000C, 0x00);
    W(0x000F, 0x43);
    depz_device_sleep_ms(dev, 1);
    W(0x000F, 0x40);
    W(0x000A, 0x01);
    depz_device_sleep_ms(dev, 100);

    note(dev, "Waiting for sensor boot...");
    W(0x7FFF, 0x00);
    TRY(poll_for_answer(dev, 1, 0, 0x06, 0xFF, 1, "sensor boot"));
    W(0x000E, 0x01);
    W(0x7FFF, 0x02);

    /* Enable FW access */
    if (i2c) {
        W(0x03, 0x0D);
        W(0x7FFF, 0x01);
        TRY(poll_for_answer(dev, 1, 0, 0x21, 0x10, 0x10, "fw access"));
    } else {
        W(0x7FFF, 0x01);
        W(0x06, 0x01);
        TRY(poll_for_answer(dev, 1, 0, 0x21, 0xFF, 0x04, "fw access"));
    }
    W(0x7FFF, 0x00);

    /* Enable host access to GO1 */
    R(0x7FFF);
    W(0x0C, 0x01);

    /* Power ON status */
    W(0x7FFF, 0x00);
    W(0x101, 0x00);
    W(0x102, 0x00);
    W(0x010A, 0x01);
    W(0x4002, 0x01);
    W(0x4002, 0x00);
    W(0x010A, 0x03);
    W(0x103, 0x01);
    W(0x400F, 0x00);
    W(0x21A, 0x43);
    W(0x21A, 0x03);
    W(0x21A, 0x01);
    W(0x21A, 0x00);
    W(0x219, 0x00);
    W(0x21B, 0x00);

    /* Wake up MCU */
    W(0x7FFF, 0x00);
    R(0x7FFF);
    if (i2c) W(0x0C, 0x00);
    W(0x7FFF, 0x01);
    if (i2c) {
        W(0x20, 0x07);
        W(0x20, 0x06);
    }

    /* Download FW into the sensor */
    note(dev, "Downloading sensor FW (84 KB)... bank 1/3");
    W(0x7FFF, 0x09);
    TRY(wr(dev, 0, s->firmware, 0x8000));
    note(dev, "Downloading sensor FW... bank 2/3");
    W(0x7FFF, 0x0A);
    TRY(wr(dev, 0, s->firmware + 0x8000, 0x8000));
    note(dev, "Downloading sensor FW... bank 3/3");
    W(0x7FFF, 0x0B);
    TRY(wr(dev, 0, s->firmware + 0x10000, 0x5000));
    W(0x7FFF, 0x01);

    /* Check if FW correctly downloaded */
    if (i2c) {
        W(0x7FFF, 0x02);
        W(0x03, 0x0D);
        W(0x7FFF, 0x01);
        TRY(poll_for_answer(dev, 1, 0, 0x21, 0x10, 0x10, "fw downloaded"));
    } else {
        W(0x7FFF, 0x01);
        W(0x06, 0x03);
        depz_device_sleep_ms(dev, 5);
    }
    W(0x7FFF, 0x00);
    R(0x7FFF);
    W(0x0C, 0x01);

    /* Reset MCU and wait boot */
    note(dev, "Booting sensor MCU...");
    W(0x7FFF, 0x00);
    W(0x114, 0x00);
    W(0x115, 0x00);
    W(0x116, 0x42);
    W(0x117, 0x00);
    W(0x0B, 0x00);
    R(0x7FFF);
    W(0x0C, 0x00);
    W(0x0B, 0x01);
    TRY(poll_for_mcu_boot(dev));
    W(0x7FFF, 0x02);

    /* Firmware checksum (0x812FFC); depends on the blob. */
    if (s->has_checksum) {
        uint8_t b[4];
        uint32_t crc;
        TRY(rd(dev, 0x2FFC, b, 4));
        swap_buffer(b, 4);
        crc = rd_u32le(b);
        if (crc != s->fw_checksum)
            return depz_fail(DEPZ_E_PROTOCOL, "VL53L8 FW_CHECKSUM_FAIL (code 5) crc=0x%08X (expected 0x%08X)",
                             crc, s->fw_checksum);
        note(dev, "Sensor FW checksum OK");
    }

    /* Offset NVM data */
    note(dev, "Reading NVM offset data...");
    TRY(wr(dev, 0x2FD8, depz_vl53l8_get_nvm_cmd, depz_vl53l8_get_nvm_cmd_len));
    TRY(poll_for_answer(dev, 4, 0, UI_CMD_STATUS, 0xFF, 2, "nvm read"));
    TRY(rd(dev, UI_CMD_START, nvm, NVM_DATA_SIZE));
    memcpy(s->offset_data, nvm, OFFSET_BUFFER_SIZE);
    TRY(send_offset_data(dev, DEPZ_VL53L8_RES_4X4));

    note(dev, "Uploading default xtalk...");
    memcpy(s->xtalk_data, depz_vl53l8_default_xtalk, XTALK_BUFFER_SIZE);
    TRY(send_xtalk_data(dev, DEPZ_VL53L8_RES_4X4));

    note(dev, "Uploading default configuration...");
    TRY(wr(dev, 0x2C34, s->default_cfg, s->default_cfg_len));
    TRY(poll_for_answer(dev, 4, 1, UI_CMD_STATUS, 0xFF, 0x03, "default config"));

    TRY(dci_write(dev, DCI_PIPE_CONTROL, pipe_ctrl, 4));
    TRY(dci_write(dev, DCI_SINGLE_RANGE, single, 4));
    if (i2c) {
        uint8_t flags[8];
        TRY(dci_read(dev, DCI_FW_FLAGS, flags, 8));
        s->module_type = flags[1];
    }
    note(dev, "Sensor init done");
    return DEPZ_OK;
}

#undef R

/* ── ULD: configuration ────────────────────────────────────────────────── */

static int uld_get_resolution(depz_device *dev, int *zones)
{
    uint8_t b[8];
    TRY(dci_read(dev, DCI_ZONE_CONFIG, b, 8));
    *zones = b[0] * b[1];
    return DEPZ_OK;
}

static int uld_set_resolution(depz_device *dev, int resolution)
{
    uint8_t b[16];
    if (resolution != DEPZ_VL53L8_RES_4X4 && resolution != DEPZ_VL53L8_RES_8X8)
        return uld_fail(127, "set_resolution");
    TRY(dci_read(dev, DCI_DSS_CONFIG, b, 16));
    b[0x04] = resolution == 16 ? 64 : 16;
    b[0x06] = resolution == 16 ? 64 : 16;
    b[0x09] = resolution == 16 ? 4 : 1;
    TRY(dci_write(dev, DCI_DSS_CONFIG, b, 16));
    TRY(dci_read(dev, DCI_ZONE_CONFIG, b, 8));
    b[0x00] = resolution == 16 ? 4 : 8;
    b[0x01] = resolution == 16 ? 4 : 8;
    b[0x04] = resolution == 16 ? 8 : 4;
    b[0x05] = resolution == 16 ? 8 : 4;
    TRY(dci_write(dev, DCI_ZONE_CONFIG, b, 8));
    TRY(send_offset_data(dev, resolution));
    return send_xtalk_data(dev, resolution);
}

static int uld_get_frequency(depz_device *dev, uint8_t *hz)
{
    uint8_t b[4];
    TRY(dci_read(dev, DCI_FREQ_HZ, b, 4));
    *hz = b[1];
    return DEPZ_OK;
}

static int uld_set_frequency(depz_device *dev, uint8_t hz) { return dci_replace(dev, DCI_FREQ_HZ, 4, &hz, 1, 1); }

static int uld_get_ranging_mode(depz_device *dev, uint8_t *mode)
{
    uint8_t b[8];
    TRY(dci_read(dev, DCI_RANGING_MODE, b, 8));
    *mode = b[1] == 0x1 ? DEPZ_VL53L8_RANGING_MODE_CONTINUOUS : DEPZ_VL53L8_RANGING_MODE_AUTONOMOUS;
    return DEPZ_OK;
}

static int uld_set_ranging_mode(depz_device *dev, uint8_t mode)
{
    uint8_t b[8], single[4] = {0, 0, 0, 0};
    TRY(dci_read(dev, DCI_RANGING_MODE, b, 8));
    if (mode == DEPZ_VL53L8_RANGING_MODE_CONTINUOUS) {
        b[1] = 0x1;
        b[3] = 0x3;
    } else if (mode == DEPZ_VL53L8_RANGING_MODE_AUTONOMOUS) {
        b[1] = 0x3;
        b[3] = 0x2;
        single[0] = 0x01;
    } else {
        return uld_fail(127, "set_ranging_mode");
    }
    TRY(dci_write(dev, DCI_RANGING_MODE, b, 8));
    return dci_write(dev, DCI_SINGLE_RANGE, single, 4);
}

static int uld_get_integration(depz_device *dev, uint32_t *ms)
{
    uint8_t b[20];
    TRY(dci_read(dev, DCI_INT_TIME, b, 20));
    *ms = rd_u32le(b) / 1000;
    return DEPZ_OK;
}

static int uld_set_integration(depz_device *dev, uint32_t ms)
{
    uint8_t v[4];
    if (ms < 2 || ms > 1000) return uld_fail(127, "set_integration_time_ms");
    wr_u32le(v, ms * 1000);
    return dci_replace(dev, DCI_INT_TIME, 20, v, 4, 0);
}

static int uld_get_sharpener(depz_device *dev, uint8_t *pct)
{
    uint8_t b[16];
    TRY(dci_read(dev, DCI_SHARPENER, b, 16));
    /* Round to nearest (Python round(): half to even; b*100/255 never lands
     * on .5 exactly for an integer byte, so plain rounding agrees). */
    *pct = (uint8_t)floor((double)b[0xD] * 100.0 / 255.0 + 0.5);
    return DEPZ_OK;
}

static int uld_set_sharpener(depz_device *dev, uint8_t pct)
{
    uint8_t v;
    if (pct >= 100) return uld_fail(127, "set_sharpener_percent");
    v = (uint8_t)(pct * 255 / 100);
    return dci_replace(dev, DCI_SHARPENER, 16, &v, 1, 0xD);
}

static int uld_get_target_order(depz_device *dev, uint8_t *order)
{
    uint8_t b[4];
    TRY(dci_read(dev, DCI_TARGET_ORDER, b, 4));
    *order = b[0];
    return DEPZ_OK;
}

static int uld_set_target_order(depz_device *dev, uint8_t order)
{
    if (order != DEPZ_VL53L8_TARGET_ORDER_CLOSEST && order != DEPZ_VL53L8_TARGET_ORDER_STRONGEST)
        return uld_fail(127, "set_target_order");
    return dci_replace(dev, DCI_TARGET_ORDER, 4, &order, 1, 0);
}

static int uld_get_xtalk_margin(depz_device *dev, double *m)
{
    uint8_t b[16];
    TRY(dci_read(dev, DCI_XTALK_CFG, b, 16));
    *m = rd_u32le(b) / 2048.0;
    return DEPZ_OK;
}

static int uld_set_xtalk_margin(depz_device *dev, double m)
{
    uint8_t v[4];
    if (m > 10000) return uld_fail(127, "set_xtalk_margin");
    wr_u32le(v, depz_vl53l8_xtalk_margin_to_raw(m));
    return dci_replace(dev, DCI_XTALK_CFG, 16, v, 4, 0);
}

/* ── ULD: ranging ──────────────────────────────────────────────────────── */

static uint32_t bh_set_size(uint32_t bh, uint32_t size) { return (bh & ~0xFFF0u) | ((size & 0xFFF) << 4); }

/* Build the output list for `resolution` into `output`/`enable`, sized as
 * start_ranging (i2c rule for the L5/L7 ULD) — returns data_read_size. */
static uint32_t build_output(const l8_state *s, int resolution, size_t cnh_bytes, bool with_cnh,
                             uint32_t *output, size_t *n_out, uint32_t enable[4])
{
    static const uint32_t base[12] = {START_BH, METADATA_BH, COMMONDATA_BH, AMBIENT_RATE_BH,
                                      SPAD_COUNT_BH, NB_TARGET_DETECTED_BH, SIGNAL_RATE_BH,
                                      RANGE_SIGMA_MM_BH, DISTANCE_BH, REFLECTANCE_BH,
                                      TARGET_STATUS_BH, MOTION_DETECT_BH};
    uint32_t size = 0;
    size_t i, n = 12;
    memcpy(output, base, sizeof base);
    enable[0] = 0x00000FFF;
    enable[1] = 0;
    enable[2] = 0;
    enable[3] = s->output_enable_w3;
    if (with_cnh) {
        output[n++] = ((uint32_t)CNH_DATA_IDX << 16) | ((uint32_t)((cnh_bytes / 4) & 0xFFF) << 4) | 4u;
        enable[0] |= 1u << (n - 1);
    }
    for (i = 0; i < n; i++) {
        uint32_t t, sz, idx;
        if (output[i] == 0 || !(enable[i / 32] & (1u << (i % 32)))) continue;
        t = output[i] & 0xF;
        sz = (output[i] >> 4) & 0xFFF;
        idx = (output[i] >> 16) & 0xFFFF;
        if (t >= 0x1 && t < 0x0D) {
            if (s->i2c) {
                if (idx >= 0x54D0 && idx < 0x5890) sz = (uint32_t)resolution;
                else if (idx < 0x6C90) sz = (uint32_t)resolution * NB_TARGET_PER_ZONE;
            } else if (idx >= 0x54D0 && idx < 0x54D0 + 960) {
                sz = (uint32_t)resolution;
            } else if (idx != CNH_DATA_IDX) {
                sz = (uint32_t)resolution * NB_TARGET_PER_ZONE;
            }
            output[i] = bh_set_size(output[i], sz);
            size += t * sz;
        } else {
            size += sz;
        }
        size += 4;
    }
    *n_out = n;
    return size + s->frame_tail;
}

static int write_output_config(depz_device *dev, const uint32_t *output, size_t n, const uint32_t enable[4],
                               uint32_t data_read_size)
{
    uint8_t list[4 * 16], cfg[8], en[16];
    size_t i;
    for (i = 0; i < n; i++) wr_u32le(list + 4 * i, output[i]);
    TRY(dci_write(dev, DCI_OUTPUT_LIST, list, 4 * n));
    wr_u32le(cfg, data_read_size);
    wr_u32le(cfg + 4, (uint32_t)n + 1);
    TRY(dci_write(dev, DCI_OUTPUT_CONFIG, cfg, 8));
    for (i = 0; i < 4; i++) wr_u32le(en + 4 * i, enable[i]);
    return dci_write(dev, DCI_OUTPUT_ENABLES, en, 16);
}

static int uld_start_ranging(depz_device *dev)
{
    l8_state *s = st(dev);
    uint32_t output[16], enable[4];
    size_t n;
    int resolution;
    uint8_t b[12];
    static const uint8_t cmd[4] = {0x00, 0x03, 0x00, 0x00};

    TRY(uld_get_resolution(dev, &resolution));
    /* A 0 / odd resolution makes a zero-sized output config and faults the
     * sensor MCU instead of streaming. */
    if (resolution != 16 && resolution != 64) {
        char where[64];
        snprintf(where, sizeof where, "bad resolution %d — call set_resolution first", resolution);
        return uld_fail(127, where);
    }
    s->data_read_size = build_output(s, resolution, s->cnh_bytes, s->cnh_armed, output, &n, enable);
    s->resolution = resolution;
    TRY(write_output_config(dev, output, n, enable, s->data_read_size));

    /* Start xshut bypass (interrupt mode) */
    TRY(wr_byte(dev, 0x7FFF, 0x00));
    TRY(wr_byte(dev, 0x09, 0x05));
    TRY(wr_byte(dev, 0x7FFF, 0x02));

    /* Start the ranging session */
    TRY(wr(dev, UI_CMD_END - 3, cmd, 4));
    TRY(poll_for_answer(dev, 4, 1, UI_CMD_STATUS, 0xFF, 0x03, "start ranging"));

    /* The FW reports the frame size it will stream; the released blobs say
     * 4 bytes less than the api.c formula — trust the FW. */
    TRY(dci_read(dev, 0x5440, b, 12));
    s->data_read_size = (uint32_t)(b[8] | b[9] << 8);

    /* Laser safety */
    TRY(dci_read(dev, 0xE0C4, b, 8));
    if (b[6] != 0) return uld_fail(3, "");
    return DEPZ_OK;
}

static int uld_stop_ranging(depz_device *dev)
{
    uint8_t b[4], tmp;
    TRY(rd(dev, 0x2FFC, b, 4));
    if (rd_u32le(b) != 0x4FF) {
        int timeout = 0;
        TRY(wr_byte(dev, 0x7FFF, 0x00));
        /* Provoke MCU stop */
        TRY(wr_byte(dev, 0x15, 0x16));
        TRY(wr_byte(dev, 0x14, 0x01));
        tmp = 0;
        while (((tmp & 0x80) >> 7) == 0) {
            TRY(rd_byte(dev, 0x06, &tmp));
            depz_device_sleep_ms(dev, 10);
            if (++timeout > 500) break;
        }
    }
    /* GO2 status 1 (non-fatal) */
    TRY(rd_byte(dev, 0x06, &tmp));
    if (tmp & 0x80) TRY(rd_byte(dev, 0x07, &tmp));
    /* Undo MCU stop, stop xshut bypass */
    TRY(wr_byte(dev, 0x7FFF, 0x00));
    TRY(wr_byte(dev, 0x14, 0x00));
    TRY(wr_byte(dev, 0x15, 0x00));
    TRY(wr_byte(dev, 0x09, 0x04));
    return wr_byte(dev, 0x7FFF, 0x02);
}

/* ── frames ─────────────────────────────────────────────────────────────── */

/* The motion-indicator block of a (swapped) frame, if present. */
static void parse_motion(const uint8_t *buf, size_t size, depz_vl53l8_live_frame *out)
{
    size_t i = 16;
    while (i + 4 <= size) {
        uint32_t bh = rd_u32le(buf + i), t = bh & 0xF, sz = (bh >> 4) & 0xFFF, idx = bh >> 16;
        size_t msize = (t > 1 && t < 0xD) ? (size_t)t * sz : sz;
        if (i + 4 + msize > size) break;
        if (idx == MOTION_DETEC_IDX && i + 16 + 128 <= size) {
            int k;
            out->has_motion = true;
            out->motion.global_indicator_1 = rd_u32le(buf + i + 4);
            out->motion.global_indicator_2 = rd_u32le(buf + i + 8);
            out->motion.status = buf[i + 12];
            out->motion.nb_of_detected_aggregates = buf[i + 13];
            out->motion.nb_of_aggregates = buf[i + 14];
            for (k = 0; k < 32; k++) out->motion.motion[k] = rd_u32le(buf + i + 16 + 4 * (size_t)k);
            return;
        }
        i += msize + 4;
    }
}

/* Parse one reassembled frame into s->scratch. */
static int parse_frame(l8_state *s, const uint8_t *raw, size_t len, uint64_t ts, bool motion)
{
    depz_vl53l8_live_frame *out = &s->scratch;
    int rc;
    out->has_motion = false;
    out->cnh_len = 0;
    if (s->v == V_CX)
        rc = depz_vl53l8_decode_frame(raw, len, ts, &out->f);
    else if (s->v == V_CH)
        rc = depz_vl53l8ch_decode_frame(raw, len, ts, &out->f, out->cnh, sizeof out->cnh, &out->cnh_len);
    else /* L5/L7: footer at size-4, per-target blocks trimmed to the zones */
        rc = depz_vl53l7_decode_frame(raw, len, ts, &out->f, out->cnh, sizeof out->cnh, &out->cnh_len);
    if (rc != 0) return -1;
    if (motion) {
        uint8_t *buf = (uint8_t *)malloc(len);
        if (buf) {
            memcpy(buf, raw, len);
            swap_buffer(buf, len);
            parse_motion(buf, len, out);
            free(buf);
        }
    }
    return 0;
}

static bool l8_report(depz_device *dev, uint8_t cmd, const uint8_t *p, size_t len)
{
    l8_state *s = st(dev);
    depz_vl53l8_chunk c;
    const uint8_t *frame;
    size_t flen;
    uint64_t ts;
    bool ok, motion;
    depz_cb_entry *cbs;
    size_t i, n;
    if (cmd != DEPZ_VL53L8_RPT_FRAME || len < 12) return false;
    if (depz_vl53l8_unpack_chunk(p, len, &c) != 0) return true;
    depz_mutex_lock(&s->lock);
    if (depz_vl53l8_reasm_feed(&s->reasm, &c, &frame, &flen, &ts) != 1 || !s->initialized) {
        depz_mutex_unlock(&s->lock);
        return true;
    }
    motion = s->motion_present;
    ok = parse_frame(s, frame, flen, ts, motion) == 0;
    if (!ok) s->parse_errors++;
    depz_mutex_unlock(&s->lock);
    if (!ok) return true;
    /* s->scratch is only written by this (the reader) thread. */
    n = depz_cb_list_snapshot(&s->cbs, &cbs);
    for (i = 0; i < n; i++) ((l8_fn)cbs[i].fn)(&s->scratch, cbs[i].user);
    free(cbs);
    depz_hub_push(s->hub, &s->scratch);
    return true;
}

static void l8_closed(depz_device *dev) { depz_hub_mark_closed(st(dev)->hub); }

static void l8_destroy(depz_device *dev)
{
    l8_state *s = st(dev);
    if (!s) return;
    depz_hub_close(s->hub);
    depz_cb_list_free(&s->cbs);
    depz_mutex_destroy(&s->lock);
    free(s);
    dev->sensor = NULL;
}

static const depz_sensor_ops l8_ops = {DEPZ_SENSOR_VL53L8, l8_report, l8_closed, l8_destroy};
/* L5/L7 run the same class; only the sensor type the device reports differs. */
static const depz_sensor_ops l7_ops = {DEPZ_SENSOR_VL53L7, l8_report, l8_closed, l8_destroy};

static bool is_l7_model(depz_vl53l8_model m)
{
    return m == DEPZ_VL53L8_MODEL_L5CX || m == DEPZ_VL53L8_MODEL_L7CX || m == DEPZ_VL53L8_MODEL_L7CH;
}

static void set_model(l8_state *s, depz_vl53l8_model model)
{
    s->model = model;
    switch (model) {
    case DEPZ_VL53L8_MODEL_L8CH: s->v = V_CH; break;
    case DEPZ_VL53L8_MODEL_L5CX:
    case DEPZ_VL53L8_MODEL_L7CX: s->v = V_L7CX; break;
    case DEPZ_VL53L8_MODEL_L7CH: s->v = V_L7CH; break;
    default: s->v = V_CX; break;
    }
    s->i2c = is_l7_model(model);
    s->min_hz = s->i2c ? 1 : 2; /* L5/L7 range at 1 Hz, L8 never does */
    switch (s->v) {
    case V_CX:
        s->firmware = depz_vl53l8_fw_cx;
        s->default_cfg = depz_vl53l8_cfg_cx;
        s->default_cfg_len = depz_vl53l8_cfg_cx_len;
        s->has_checksum = true;
        s->fw_checksum = 0xCADF7CAFu;
        break;
    case V_CH:
        s->firmware = depz_vl53l8_fw_ch;
        s->default_cfg = depz_vl53l8_cfg_ch;
        s->default_cfg_len = depz_vl53l8_cfg_ch_len;
        s->has_checksum = true;
        s->fw_checksum = 0x0C0B6C9Eu;
        break;
    case V_L7CX: /* ULD 2.0.1 publishes no checksum */
        s->firmware = depz_vl53l8_fw_l7cx;
        s->default_cfg = depz_vl53l8_cfg_l7cx;
        s->default_cfg_len = depz_vl53l8_cfg_l7cx_len;
        s->has_checksum = false;
        break;
    case V_L7CH: /* the L8CH firmware, the VL53L7 default configuration */
        s->firmware = depz_vl53l8_fw_ch;
        s->default_cfg = depz_vl53l8_cfg_l7ch;
        s->default_cfg_len = depz_vl53l8_cfg_l7ch_len;
        s->has_checksum = true;
        s->fw_checksum = 0x0C0B6C9Eu;
        break;
    }
    /* OUTPUT_ENABLES word 3: 0 on the LMZ blobs, 0xC0000000 otherwise; frame
     * tail: 32 on VL53L8CX 2.1.0, 24 on the 2.0.x drivers. */
    s->output_enable_w3 = (s->v == V_CH || s->v == V_L7CH) ? 0 : 0xC0000000u;
    s->frame_tail = s->v == V_CX ? 32 : 24;
    s->rb.cmd_read = DEPZ_VL53L8_CMD_READ_REG;
    s->rb.cmd_write = DEPZ_VL53L8_CMD_WRITE_REG;
    s->rb.rpt_reg_data = DEPZ_VL53L8_RPT_REG_DATA;
    s->rb.xfer_max = 2048;
    s->rb.read_max = s->i2c ? DEPZ_VL53L7_READ_MAX_LEN : 2048; /* L5/L7: reads <= 1536 */
    s->rb.timeout_ms = 2000;
}

static l8_state *l8_new(depz_vl53l8_model model)
{
    l8_state *s = (l8_state *)calloc(1, sizeof *s);
    if (!s) return NULL;
    if (depz_mutex_init(&s->lock)) { free(s); return NULL; }
    if (depz_cb_list_init(&s->cbs)) { depz_mutex_destroy(&s->lock); free(s); return NULL; }
    s->hub = depz_hub_new(sizeof(depz_vl53l8_live_frame));
    if (!s->hub) {
        depz_cb_list_free(&s->cbs);
        depz_mutex_destroy(&s->lock);
        free(s);
        return NULL;
    }
    set_model(s, model);
    s->module_type = -1;
    s->rb.write_progress = on_write_progress;
    s->rb.progress_user = s;
    depz_vl53l8_reasm_init(&s->reasm);
    return s;
}

int depz_vl53l8_attach(depz_device *dev, depz_vl53l8_model model)
{
    l8_state *s;
    if (dev->ops == &l8_ops || dev->ops == &l7_ops) return DEPZ_OK;
    s = l8_new(model);
    if (!s) return depz_fail(DEPZ_E_NOMEM, "vl53l8: out of memory");
    s->rb.dev = dev;
    depz_device_attach(dev, is_l7_model(model) ? &l7_ops : &l8_ops, s);
    return DEPZ_OK;
}

int depz_vl53l8_open_link(depz_link *link, depz_vl53l8_model model, depz_device **out)
{
    l8_state *s = l8_new(model);
    int rc;
    if (!s) {
        depz_link_free(link);
        return depz_fail(DEPZ_E_NOMEM, "vl53l8: out of memory");
    }
    rc = depz_device_create(link, is_l7_model(model) ? &l7_ops : &l8_ops, s, out);
    if (rc == DEPZ_OK) s->rb.dev = *out;
    return rc;
}

bool depz_is_vl53l8(const depz_device *dev)
{
    bool yes;
    if (!dev) return false;
    depz_mutex_lock((depz_mutex *)&dev->lock);
    yes = dev->ops == &l8_ops || dev->ops == &l7_ops;
    depz_mutex_unlock((depz_mutex *)&dev->lock);
    return yes;
}

depz_vl53l8_model depz_vl53l8_get_model(const depz_device *dev)
{
    return depz_is_vl53l8(dev) ? st((depz_device *)dev)->model : DEPZ_VL53L8_MODEL_L8CX;
}

static int need(const depz_device *dev)
{
    if (!dev) return depz_fail(DEPZ_E_ARG, "vl53l8: NULL device");
    if (!depz_is_vl53l8(dev)) return depz_fail(DEPZ_E_WRONG_TYPE, "%s is not a VL53L8", dev->port);
    return DEPZ_OK;
}

static bool flag(depz_device *dev, int which)
{
    l8_state *s = st(dev);
    bool v;
    depz_mutex_lock(&s->lock);
    v = which == 0 ? s->initialized : which == 1 ? s->ranging : s->motion_present;
    depz_mutex_unlock(&s->lock);
    return v;
}

static void set_flag(depz_device *dev, int which, bool v)
{
    l8_state *s = st(dev);
    depz_mutex_lock(&s->lock);
    if (which == 0) s->initialized = v;
    else if (which == 1) s->ranging = v;
    else s->motion_present = v;
    depz_mutex_unlock(&s->lock);
}

/* init() done and not ranging. */
static int need_idle(depz_device *dev)
{
    TRY(need(dev));
    if (!flag(dev, 0)) return depz_fail(DEPZ_E_ARG, "call init() first");
    if (flag(dev, 1)) return depz_fail(DEPZ_E_ARG, "stop ranging first — the stream owns the register bank");
    return DEPZ_OK;
}

static int need_init(depz_device *dev)
{
    TRY(need(dev));
    if (!flag(dev, 0)) return depz_fail(DEPZ_E_ARG, "call init() first");
    return DEPZ_OK;
}

bool depz_vl53l8_initialized(const depz_device *dev) { return depz_is_vl53l8(dev) && flag((depz_device *)dev, 0); }
bool depz_vl53l8_ranging(const depz_device *dev) { return depz_is_vl53l8(dev) && flag((depz_device *)dev, 1); }

/* ── public API ───────────────────────────────────────────────────────── */

int depz_vl53l8_is_alive(depz_device *dev, bool *alive, uint8_t *device_id, uint8_t *revision_id)
{
    uint8_t id = 0, rev = 0;
    int rc;
    TRY(need(dev));
    rc = uld_is_alive(dev, &id, &rev);
    if (device_id) *device_id = id;
    if (revision_id) *revision_id = rev;
    if (alive) *alive = rc == DEPZ_OK && (st(dev)->i2c ? (rev == 0x02 || (id == 0xF0 && rev == 0x01))
                                                       : (id == 0xF0 && rev == 0x0C));
    return rc;
}

int depz_vl53l8_init(depz_device *dev, depz_vl53l8_progress_cb progress, void *user)
{
    l8_state *s;
    int rc;
    TRY(need(dev));
    if (flag(dev, 1)) return depz_fail(DEPZ_E_ARG, "stop ranging first — the stream owns the register bank");
    s = st(dev);
    s->progress = progress;
    s->progress_user = user;
    set_flag(dev, 0, false);
    rc = uld_init(dev);
    s->progress = NULL;
    if (rc == DEPZ_OK) {
        /* A fresh sensor holds no CNH / motion configuration: disarm both
         * (the Python SDK keeps its CNH config across init()). */
        set_flag(dev, 0, true);
        set_flag(dev, 2, false);
        s->cnh_armed = false;
    }
    return rc;
}

int depz_vl53l8_get_resolution(depz_device *dev, int *zones)
{
    TRY(need_init(dev));
    return uld_get_resolution(dev, zones);
}

int depz_vl53l8_set_resolution(depz_device *dev, int zones)
{
    TRY(need_idle(dev));
    if (zones != 16 && zones != 64) return depz_fail(DEPZ_E_ARG, "resolution is 16 (4x4) or 64 (8x8) zones");
    return uld_set_resolution(dev, zones);
}

int depz_vl53l8_get_ranging_frequency_hz(depz_device *dev, uint8_t *hz)
{
    TRY(need_init(dev));
    return uld_get_frequency(dev, hz);
}

int depz_vl53l8_set_ranging_frequency_hz(depz_device *dev, uint8_t hz)
{
    TRY(need(dev));
    if (hz < st(dev)->min_hz)
        return depz_fail(DEPZ_E_ARG, "ranging frequency must be >= %u Hz: below that the sensor never "
                                     "enters its ranging loop and streams nothing (contract 04)",
                         st(dev)->min_hz);
    TRY(need_idle(dev));
    return uld_set_frequency(dev, hz);
}

int depz_vl53l8_get_ranging_mode(depz_device *dev, uint8_t *mode)
{
    TRY(need_init(dev));
    return uld_get_ranging_mode(dev, mode);
}

int depz_vl53l8_set_ranging_mode(depz_device *dev, uint8_t mode)
{
    TRY(need_idle(dev));
    return uld_set_ranging_mode(dev, mode);
}

int depz_vl53l8_get_integration_time_ms(depz_device *dev, uint32_t *ms)
{
    TRY(need_init(dev));
    return uld_get_integration(dev, ms);
}

int depz_vl53l8_set_integration_time_ms(depz_device *dev, uint32_t ms)
{
    TRY(need_idle(dev));
    return uld_set_integration(dev, ms);
}

int depz_vl53l8_get_sharpener_percent(depz_device *dev, uint8_t *pct)
{
    TRY(need_init(dev));
    return uld_get_sharpener(dev, pct);
}

int depz_vl53l8_set_sharpener_percent(depz_device *dev, uint8_t pct)
{
    TRY(need_idle(dev));
    return uld_set_sharpener(dev, pct);
}

int depz_vl53l8_get_target_order(depz_device *dev, uint8_t *order)
{
    TRY(need_init(dev));
    return uld_get_target_order(dev, order);
}

int depz_vl53l8_set_target_order(depz_device *dev, uint8_t order)
{
    TRY(need_idle(dev));
    return uld_set_target_order(dev, order);
}

int depz_vl53l8_get_power_mode(depz_device *dev, uint8_t *mode)
{
    uint8_t tmp, f;
    TRY(need_init(dev));
    TRY(wr_byte(dev, 0x7FFF, 0x00));
    TRY(rd_byte(dev, 0x09, &tmp));
    if (tmp == 0x04) {
        *mode = DEPZ_VL53L8_POWER_MODE_WAKEUP;
    } else if (tmp == 0x02) {
        if (st(dev)->v == V_L7CX) {
            *mode = DEPZ_VL53L8_POWER_MODE_SLEEP; /* ULD 2.0.1 has no deep sleep */
        } else {
            TRY(rd_byte(dev, 0x000F, &f));
            *mode = f == 0x43 ? DEPZ_VL53L8_POWER_MODE_DEEP_SLEEP : DEPZ_VL53L8_POWER_MODE_SLEEP;
        }
    } else {
        TRY(wr_byte(dev, 0x7FFF, 0x02));
        return uld_fail(STATUS_ERROR, "get_power_mode");
    }
    return wr_byte(dev, 0x7FFF, 0x02);
}

int depz_vl53l8_set_power_mode(depz_device *dev, uint8_t mode)
{
    uint8_t current, stored;
    l8_state *s;
    TRY(need_idle(dev));
    s = st(dev);
    if (s->v == V_L7CX && mode != DEPZ_VL53L8_POWER_MODE_SLEEP && mode != DEPZ_VL53L8_POWER_MODE_WAKEUP)
        return uld_fail(127, "set_power_mode: VL53L5CX/L7CX have no deep sleep");
    TRY(depz_vl53l8_get_power_mode(dev, &current));
    if (mode == current) return DEPZ_OK;
    if (mode == DEPZ_VL53L8_POWER_MODE_WAKEUP && s->v == V_L7CX) {
        TRY(wr_byte(dev, 0x7FFF, 0x00));
        TRY(wr_byte(dev, 0x09, 0x04));
        TRY(poll_for_answer(dev, 1, 0, 0x06, 0x01, 1, "wakeup"));
        return wr_byte(dev, 0x7FFF, 0x02);
    }
    if (mode == DEPZ_VL53L8_POWER_MODE_WAKEUP) {
        TRY(wr_byte(dev, 0x7FFF, 0x00));
        TRY(wr_byte(dev, 0x09, 0x04));
        TRY(rd_byte(dev, 0x000F, &stored));
        if (stored == 0x43) TRY(wr_byte(dev, 0x000F, 0x40));
        TRY(poll_for_answer(dev, 1, 0, 0x06, 0x01, 1, "wakeup"));
        TRY(wr_byte(dev, 0x7FFF, 0x02));
        if (stored == 0x43) { /* deep sleep lost the FW: re-init, disarm CNH / motion */
            TRY(uld_init(dev));
            set_flag(dev, 2, false);
            s->cnh_armed = false;
        }
        return DEPZ_OK;
    }
    if (mode == DEPZ_VL53L8_POWER_MODE_SLEEP || mode == DEPZ_VL53L8_POWER_MODE_DEEP_SLEEP) {
        TRY(wr_byte(dev, 0x7FFF, 0x00));
        TRY(wr_byte(dev, 0x09, 0x02));
        TRY(poll_for_answer(dev, 1, 0, 0x06, 0x01, 0, mode == DEPZ_VL53L8_POWER_MODE_SLEEP ? "sleep" : "deep sleep"));
        if (mode == DEPZ_VL53L8_POWER_MODE_DEEP_SLEEP) TRY(wr_byte(dev, 0x000F, 0x43));
        return wr_byte(dev, 0x7FFF, 0x02);
    }
    return uld_fail(127, "set_power_mode");
}

int depz_vl53l8_get_xtalk_margin(depz_device *dev, double *m)
{
    TRY(need_init(dev));
    return uld_get_xtalk_margin(dev, m);
}

int depz_vl53l8_set_xtalk_margin(depz_device *dev, double m)
{
    TRY(need_idle(dev));
    return uld_set_xtalk_margin(dev, m);
}

static int read_back_xtalk(depz_device *dev)
{
    static const uint8_t footer[8] = {0x00, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x03, 0x04};
    uint8_t buf[XTALK_BUFFER_SIZE + 4];
    TRY(wr(dev, 0x2FB8, depz_vl53l8_get_xtalk_cmd, depz_vl53l8_get_xtalk_cmd_len));
    TRY(poll_for_answer(dev, 4, 1, UI_CMD_STATUS, 0xFF, 0x03, "get xtalk"));
    TRY(rd(dev, UI_CMD_START, buf, sizeof buf));
    memcpy(st(dev)->xtalk_data, buf + 8, XTALK_BUFFER_SIZE - 8);
    memcpy(st(dev)->xtalk_data + XTALK_BUFFER_SIZE - 8, footer, 8);
    return DEPZ_OK;
}

int depz_vl53l8_get_caldata_xtalk(depz_device *dev, uint8_t out[776])
{
    int resolution;
    TRY(need_idle(dev));
    TRY(uld_get_resolution(dev, &resolution));
    TRY(uld_set_resolution(dev, DEPZ_VL53L8_RES_8X8));
    TRY(read_back_xtalk(dev));
    memcpy(out, st(dev)->xtalk_data, XTALK_BUFFER_SIZE);
    return uld_set_resolution(dev, resolution);
}

int depz_vl53l8_set_caldata_xtalk(depz_device *dev, const uint8_t blob[776])
{
    int resolution;
    TRY(need_idle(dev));
    TRY(uld_get_resolution(dev, &resolution));
    memcpy(st(dev)->xtalk_data, blob, XTALK_BUFFER_SIZE);
    return uld_set_resolution(dev, resolution);
}

int depz_vl53l8_calibrate_xtalk(depz_device *dev, uint8_t reflectance_percent, uint8_t nb_samples,
                                uint16_t distance_mm, bool *failed)
{
    static const uint8_t cmd[4] = {0x00, 0x03, 0x00, 0x00};
    l8_state *s;
    int resolution, timeout = 0;
    uint8_t hz, sharp, order, mode, v[2];
    uint32_t integ, output[17], data_read_size = 0;
    double margin;
    const uint8_t *table;
    size_t table_len, i;

    TRY(need_idle(dev));
    s = st(dev);
    if (reflectance_percent < 1 || reflectance_percent > 99) return uld_fail(127, "reflectance 1..99");
    if (distance_mm < 600 || distance_mm > 3000) return uld_fail(127, "distance 600..3000");
    if (nb_samples < 1 || nb_samples > 16) return uld_fail(127, "nb_samples 1..16");
    /* save the current configuration */
    TRY(uld_get_resolution(dev, &resolution));
    TRY(uld_get_frequency(dev, &hz));
    TRY(uld_get_integration(dev, &integ));
    TRY(uld_get_sharpener(dev, &sharp));
    TRY(uld_get_target_order(dev, &order));
    TRY(uld_get_xtalk_margin(dev, &margin));
    TRY(uld_get_ranging_mode(dev, &mode));
    TRY(uld_set_resolution(dev, DEPZ_VL53L8_RES_8X8));
    /* The table of the matching firmware: the LMZ blob skips the calibration
     * silently with the VL53L8CX table (measured 2026-09-25). */
    table = s->v == V_CX ? depz_vl53l8_calibrate_xtalk_cx
          : s->v == V_L7CX ? depz_vl53l8_calibrate_xtalk_l7cx : depz_vl53l8_calibrate_xtalk_lmz;
    table_len = s->v == V_CX ? depz_vl53l8_calibrate_xtalk_cx_len
              : s->v == V_L7CX ? depz_vl53l8_calibrate_xtalk_l7cx_len : depz_vl53l8_calibrate_xtalk_lmz_len;
    TRY(wr(dev, 0x2C28, table, table_len));
    TRY(poll_for_answer(dev, 4, 1, UI_CMD_STATUS, 0xFF, 0x03, "calib cmd"));
    v[0] = (uint8_t)(distance_mm * 4);
    v[1] = (uint8_t)((distance_mm * 4) >> 8);
    TRY(dci_replace(dev, DCI_CAL_CFG, 8, v, 2, 0));
    v[0] = (uint8_t)(reflectance_percent * 16);
    v[1] = (uint8_t)((reflectance_percent * 16) >> 8);
    TRY(dci_replace(dev, DCI_CAL_CFG, 8, v, 2, 2));
    TRY(dci_replace(dev, DCI_CAL_CFG, 8, &nb_samples, 1, 4));
    /* The dedicated 17-block calibration output list (every ST version). */
    memcpy(output, XTALK_CAL_OUTPUT, sizeof output);
    for (i = 0; i < 17; i++) {
        uint32_t t, sz, idx;
        if (output[i] == 0 || !(XTALK_CAL_OUTPUT_ENABLE[i / 32] & (1u << (i % 32)))) continue;
        t = output[i] & 0xF;
        sz = (output[i] >> 4) & 0xFFF;
        idx = output[i] >> 16;
        if (t >= 0x1 && t < 0x0D) {
            sz = (idx >= 0x54D0 && idx < 0x54D0 + 960) ? 64 : 64 * NB_TARGET_PER_ZONE;
            output[i] = bh_set_size(output[i], sz);
            data_read_size += t * sz;
        } else {
            data_read_size += sz;
        }
        data_read_size += 4;
    }
    data_read_size += 24;
    TRY(write_output_config(dev, output, 17, XTALK_CAL_OUTPUT_ENABLE, data_read_size));
    TRY(wr(dev, UI_CMD_END - 3, cmd, 4));
    TRY(poll_for_answer(dev, 4, 1, UI_CMD_STATUS, 0xFF, 0x03, "calib start"));
    s->xtalk_calibration_failed = false;
    for (;;) {
        uint8_t b[4];
        TRY(rd(dev, 0x0, b, 4));
        if (b[0] != STATUS_ERROR) {
            if (b[2] >= 0x7F && ((b[3] & 0x80) >> 7) == 1) {
                /* ST: "Coverglass too good for Xtalk calibration". */
                memcpy(s->xtalk_data, depz_vl53l8_default_xtalk, XTALK_BUFFER_SIZE);
                s->xtalk_calibration_failed = true;
            }
            break;
        }
        if (timeout >= 400) return uld_fail(STATUS_ERROR, "xtalk calibration");
        depz_device_sleep_ms(dev, 50);
        timeout++;
    }
    TRY(read_back_xtalk(dev));
    TRY(wr(dev, 0x2C34, s->default_cfg, s->default_cfg_len));
    TRY(poll_for_answer(dev, 4, 1, UI_CMD_STATUS, 0xFF, 0x03, "restore cfg"));
    /* restore */
    TRY(uld_set_resolution(dev, resolution));
    TRY(uld_set_frequency(dev, hz));
    TRY(uld_set_integration(dev, integ));
    TRY(uld_set_sharpener(dev, sharp));
    TRY(uld_set_target_order(dev, order));
    TRY(uld_set_xtalk_margin(dev, margin));
    TRY(uld_set_ranging_mode(dev, mode));
    if (failed) *failed = s->xtalk_calibration_failed;
    return DEPZ_OK;
}

int depz_vl53l8_get_detection_thresholds_enable(depz_device *dev, bool *enabled)
{
    uint8_t b[8];
    TRY(need_init(dev));
    TRY(dci_read(dev, DCI_DET_THRESH_GLOBAL_CONFIG, b, 8));
    *enabled = b[1] != 0;
    return DEPZ_OK;
}

int depz_vl53l8_set_detection_thresholds_enable(depz_device *dev, bool enabled)
{
    uint8_t grp[4] = {0x01, 0x00, 0x01, 0x00}, tmp;
    TRY(need_idle(dev));
    grp[1] = enabled ? 0x01 : 0x00;
    tmp = enabled ? 0x04 : 0x0C;
    TRY(dci_replace(dev, DCI_DET_THRESH_GLOBAL_CONFIG, 8, grp, 4, 0));
    return dci_replace(dev, DCI_DET_THRESH_CONFIG, 20, &tmp, 1, 0x11);
}

/* Scale of a threshold measurement (get divides, set multiplies). */
static int32_t thresh_scale(uint8_t m)
{
    switch (m) {
    case DEPZ_VL53L8_DIST_MM: return 4;
    case DEPZ_VL53L8_SIGNAL_PER_SPAD_KCPS: return 2048;
    case DEPZ_VL53L8_RANGE_SIGMA_MM: return 128;
    case DEPZ_VL53L8_AMBIENT_PER_SPAD_KCPS: return 2048;
    case DEPZ_VL53L8_NB_SPADS_ENABLED: return 256;
    case DEPZ_VL53L8_MOTION_INDICATOR: return 65535;
    default: return 1;
    }
}

static int32_t floordiv(int32_t a, int32_t b)
{
    int32_t q = a / b;
    return (a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q;
}

int depz_vl53l8_get_detection_thresholds(depz_device *dev, depz_vl53l8_threshold out[64])
{
    uint8_t raw[DEPZ_VL53L8_THRESH_START_SIZE];
    int k;
    TRY(need_init(dev));
    TRY(dci_read(dev, DCI_DET_THRESH_START, raw, sizeof raw));
    for (k = 0; k < 64; k++) {
        const uint8_t *p = raw + 12 * k;
        int32_t sc = thresh_scale(p[8]);
        out[k].low_thresh = floordiv((int32_t)rd_u32le(p), sc);
        out[k].high_thresh = floordiv((int32_t)rd_u32le(p + 4), sc);
        out[k].measurement = p[8];
        out[k].type = p[9];
        out[k].zone_num = p[10];
        out[k].operation = p[11];
    }
    return DEPZ_OK;
}

int depz_vl53l8_set_detection_thresholds(depz_device *dev, const depz_vl53l8_threshold *th, size_t n)
{
    uint8_t start[DEPZ_VL53L8_THRESH_START_SIZE], valid[8];
    TRY(need_idle(dev));
    depz_vl53l8_pack_thresholds(th, n, start, valid);
    TRY(dci_write(dev, DCI_DET_THRESH_VALID_STATUS, valid, 8));
    return dci_write(dev, DCI_DET_THRESH_START, start, sizeof start);
}

int depz_vl53l8_set_detection_thresholds_auto_stop(depz_device *dev, bool auto_stop)
{
    uint8_t v = auto_stop ? 1 : 0;
    TRY(need_idle(dev));
    if (st(dev)->v == V_L7CX) return uld_fail(127, "detection-threshold auto-stop: not in the L5CX/L7CX ULD");
    return dci_replace(dev, DCI_PIPE_CONTROL, 4, &v, 1, 3);
}

int depz_vl53l8_configure_motion_indicator(depz_device *dev, uint16_t min_mm, uint16_t max_mm)
{
    uint8_t cfg[DEPZ_VL53L8_MOTION_CFG_SIZE];
    int resolution, ref, feat;
    TRY(need_idle(dev));
    TRY(uld_get_resolution(dev, &resolution));
    if (depz_vl53l8_motion_cfg_default_pack(resolution, cfg) != 0) return uld_fail(127, "motion set_resolution");
    TRY(dci_write(dev, DCI_MOTION_DETECTOR_CFG, cfg, sizeof cfg));
    set_flag(dev, 2, true);
    if ((int)max_mm - (int)min_mm > 1500 || min_mm < 400 || max_mm > 4000) return uld_fail(127, "motion set_distance");
    ref = (int)(((min_mm / 37.5348) - 4.0) * 2048.5);
    feat = (int)((((max_mm - min_mm) / 10.0 + 30.02784) / 15.01392) + 0.5);
    wr_u32le(cfg, (uint32_t)ref);
    cfg[19] = (uint8_t)feat; /* feature_length: after i32 + 3 u32 + 3 bytes */
    return dci_write(dev, DCI_MOTION_DETECTOR_CFG, cfg, sizeof cfg);
}

int depz_vl53l8_configure_cnh(depz_device *dev, const depz_vl53l8_cnh_setup *setup)
{
    uint8_t packed[156];
    size_t bytes;
    l8_state *s;
    TRY(need_idle(dev));
    s = st(dev);
    if (s->v == V_CX || s->v == V_L7CX)
        return depz_fail(DEPZ_E_WRONG_TYPE, "CNH needs the CH firmware (VL53L8CH / VL53L7CH)");
    TRY(depz_vl53l8_cnh_required_memory(setup, &bytes));
    depz_vl53l8_cnh_pack(setup, packed);
    TRY(dci_write(dev, DCI_MOTION_DETECTOR_CFG, packed, sizeof packed));
    s->cnh_armed = true;
    s->cnh_bytes = bytes;
    depz_vl53l8_cnh_decode_config(setup, &s->cnh_decode);
    return DEPZ_OK;
}

int depz_vl53l8_start_ranging(depz_device *dev)
{
    l8_state *s;
    uint8_t p[2];
    int zones;
    TRY(need_idle(dev));
    s = st(dev);
    /* The Python SDK refreshes its resolution cache here, before the ULD reads
     * it again: the same request twice, kept so captures replay both ways. */
    TRY(uld_get_resolution(dev, &zones));
    TRY(uld_start_ranging(dev));
    depz_mutex_lock(&s->lock);
    depz_vl53l8_reasm_init(&s->reasm);
    depz_mutex_unlock(&s->lock);
    depz_vl53l8_pack_start_stream((uint16_t)s->data_read_size, p);
    TRY(depz_device_request(dev, DEPZ_VL53L8_CMD_START_STREAM, p, 2, NULL, NULL, true, -1));
    set_flag(dev, 1, true);
    return DEPZ_OK;
}

int depz_vl53l8_stop_ranging(depz_device *dev)
{
    int rc, rc2;
    TRY(need(dev));
    if (!flag(dev, 1)) return DEPZ_OK;
    rc = depz_device_request(dev, DEPZ_VL53L8_CMD_STOP_STREAM, NULL, 0, NULL, NULL, true, -1);
    set_flag(dev, 1, false);
    rc2 = uld_stop_ranging(dev);
    return rc ? rc : rc2;
}

int depz_vl53l8_on_frame(depz_device *dev, depz_vl53l8_frame_cb cb, void *user, int *token)
{
    TRY(need(dev));
    if (!cb) return depz_fail(DEPZ_E_ARG, "vl53l8: NULL callback");
    return depz_cb_list_add(&st(dev)->cbs, (void (*)(void))cb, user, token);
}

void depz_vl53l8_off_frame(depz_device *dev, int token)
{
    if (depz_is_vl53l8(dev)) depz_cb_list_remove(&st(dev)->cbs, token);
}

depz_stream *depz_vl53l8_frames(depz_device *dev, size_t maxsize)
{
    if (need(dev)) return NULL;
    return depz_hub_subscribe(st(dev)->hub, maxsize ? maxsize : 8);
}

int depz_vl53l8_get_frame(depz_device *dev, int timeout_ms, depz_vl53l8_live_frame *out)
{
    depz_stream *s;
    int rc;
    TRY(need(dev));
    s = depz_hub_subscribe(st(dev)->hub, 1);
    if (!s) return depz_fail(DEPZ_E_NOMEM, "vl53l8: cannot subscribe");
    rc = depz_stream_next(s, out, timeout_ms < 0 ? 2000 : timeout_ms);
    depz_stream_close(s);
    return rc;
}

uint64_t depz_vl53l8_frame_parse_errors(const depz_device *dev)
{
    uint64_t n;
    l8_state *s;
    if (!depz_is_vl53l8(dev)) return 0;
    s = st((depz_device *)dev);
    depz_mutex_lock(&s->lock);
    n = s->parse_errors;
    depz_mutex_unlock(&s->lock);
    return n;
}

uint64_t depz_vl53l8_reassembler_discards(const depz_device *dev)
{
    uint64_t n;
    l8_state *s;
    if (!depz_is_vl53l8(dev)) return 0;
    s = st((depz_device *)dev);
    depz_mutex_lock(&s->lock);
    n = s->reasm.discarded;
    depz_mutex_unlock(&s->lock);
    return n;
}

/* ── L5/L7 board commands (contract 11 §2) ────────────────────────────── */

int depz_vl53l8_module_type(const depz_device *dev)
{
    return depz_is_vl53l8(dev) ? st((depz_device *)dev)->module_type : -1;
}

static int need_l7(depz_device *dev)
{
    TRY(need(dev));
    if (!st(dev)->i2c) return depz_fail(DEPZ_E_WRONG_TYPE, "%s is a VL53L8: no L5/L7 board command", dev->port);
    return DEPZ_OK;
}

typedef struct { depz_vl53l7_info *out; bool ok; } l7_info_ctx;

static bool match_l7_info(uint8_t cmd, const uint8_t *p, size_t len, void *c)
{
    l7_info_ctx *x = (l7_info_ctx *)c;
    if (cmd != DEPZ_VL53L7_RPT_INFO) return false;
    x->ok = depz_vl53l7_unpack_info(p, len, x->out) == 0;
    return true;
}

int depz_vl53l7_bridge_info(depz_device *dev, depz_vl53l7_info *out)
{
    l7_info_ctx c;
    TRY(need_l7(dev));
    if (!out) return depz_fail(DEPZ_E_ARG, "bridge_info: NULL output");
    c.out = out;
    c.ok = false;
    TRY(depz_device_request(dev, DEPZ_VL53L7_CMD_GET_INFO, NULL, 0, match_l7_info, &c, false, -1));
    if (!c.ok) return depz_fail(DEPZ_E_PROTOCOL, "vl53l7: info report did not decode");
    return DEPZ_OK;
}

int depz_vl53l7_set_i2c_speed_khz(depz_device *dev, uint16_t khz, uint16_t *effective)
{
    uint8_t p[2];
    depz_vl53l7_info info;
    TRY(need_l7(dev));
    if (khz == 0) return depz_fail(DEPZ_E_ARG, "khz must be 1..65535");
    depz_vl53l7_pack_set_i2c_speed(khz, p);
    TRY(depz_device_request(dev, DEPZ_VL53L7_CMD_SET_I2C_SPEED, p, 2, NULL, NULL, true, -1));
    TRY(depz_vl53l7_bridge_info(dev, &info));
    if (effective) *effective = info.i2c_khz;
    return DEPZ_OK;
}

int depz_vl53l7_pin_ctrl(depz_device *dev, uint8_t action)
{
    uint8_t p[1];
    TRY(need_l7(dev));
    depz_vl53l7_pack_pin_ctrl(action, p);
    TRY(depz_device_request(dev, DEPZ_VL53L7_CMD_PIN_CTRL, p, 1, NULL, NULL, true, -1));
    if (action == DEPZ_VL53L7_PIN_LPN_OFF || action == DEPZ_VL53L7_PIN_SOFT_CYCLE) {
        set_flag(dev, 1, false);
        set_flag(dev, 0, false);
    }
    return DEPZ_OK;
}

int depz_vl53l8_read_reg(depz_device *dev, uint16_t addr, uint8_t *buf, size_t len)
{
    TRY(need(dev));
    return rd(dev, addr, buf, len);
}

int depz_vl53l8_write_reg(depz_device *dev, uint16_t addr, const uint8_t *data, size_t len)
{
    TRY(need(dev));
    return wr(dev, addr, data, len);
}

int depz_vl53l8_dci_read(depz_device *dev, uint16_t index, uint8_t *buf, size_t len)
{
    TRY(need_init(dev));
    return dci_read(dev, index, buf, len);
}

int depz_vl53l8_dci_write(depz_device *dev, uint16_t index, const uint8_t *data, size_t len)
{
    TRY(need_init(dev));
    return dci_write(dev, index, data, len);
}

/* ── CNH setup (vl53lmz_plugin_cnh.c, uld cnh.py) ──────────────────────── */

void depz_vl53l8_cnh_init_config(depz_vl53l8_cnh_setup *s, int start_bin, int num_bins, int sub_sample)
{
    int k;
    memset(s, 0, sizeof *s);
    for (k = 0; k < 64; k++) s->map_id[k] = -1;
    s->ref_bin_offset = start_bin * 2048;
    s->feature_length = (uint8_t)(num_bins & 0xFF);
    s->sum_span = (uint8_t)(sub_sample & 0xFF);
    s->nb_of_temporal_accumulations = 1;
    /* DISABLE_PING_PONG | DISABLE_VARIANCE | ENABLE_AMBIENT_LEVEL |
     * ENABLE_XTALK_REMOVAL | ZERO_NON_VALID_BINS | STORE_REF_RESIDUAL */
    s->cnh_cfg = 0x01 | 0x02 | 0x04 | 0x08 | 0x10 | 0x20;
    s->cnh_flex_shift = 1;
}

int depz_vl53l8_cnh_create_agg_map(depz_vl53l8_cnh_setup *s, int resolution, int start_x, int start_y,
                                   int merge_x, int merge_y, int cols, int rows)
{
    int zone_res = resolution == 16 ? 4 : 8, row, col, k;
    for (k = 0; k < 64; k++) s->map_id[k] = -1;
    if (merge_x < 1 || merge_y < 1 || start_x + cols * merge_x > zone_res || start_y + rows * merge_y > zone_res)
        return depz_fail(DEPZ_E_ARG, "cnh: agg map exceeds zone grid");
    s->nb_of_aggregates = (uint8_t)(cols * rows);
    for (row = start_y; row < start_y + rows * merge_y; row++) {
        for (col = start_x; col < start_x + cols * merge_x; col++) {
            int i = row * zone_res + col;
            int agg = ((row - start_y) / merge_y) * cols + ((col - start_x) / merge_x);
            if (agg < 0 || agg >= 64) return depz_fail(DEPZ_E_ARG, "cnh: agg id out of range");
            s->map_id[i] = (int8_t)agg;
        }
    }
    return DEPZ_OK;
}

int depz_vl53l8_cnh_required_memory(const depz_vl53l8_cnh_setup *s, size_t *bytes)
{
    size_t agg_x_feat, size;
    if (!s || !s->nb_of_aggregates) return depz_fail(DEPZ_E_ARG, "cnh: agg map not created");
    agg_x_feat = (size_t)s->nb_of_aggregates * s->feature_length;
    size = 2 * 4;                               /* per-buffer header */
    size += agg_x_feat * 4;                     /* FEAT_INT */
    size += ((3 + agg_x_feat) / 4) * 4;         /* FEAT_FRAC */
    size += (size_t)s->nb_of_aggregates * 4;    /* AMBIENT_INT */
    size += ((3 + (size_t)s->nb_of_aggregates) / 4) * 4; /* AMBIENT_FRAC */
    if ((s->cnh_cfg & 0x02) == 0) {
        size += agg_x_feat * 4;
        size += ((3 + agg_x_feat) / 4) * 4;
    }
    size = (size / 4) * 4;
    if ((s->cnh_cfg & 0x01) == 0) size *= 2;
    size += 5 * 4;                              /* persistent header */
    if (size > DEPZ_VL53L8_CNH_MAX_BYTES)
        return depz_fail(DEPZ_E_ARG, "cnh needs %zu B > max %u B — reduce aggregates or bins", size,
                         DEPZ_VL53L8_CNH_MAX_BYTES);
    *bytes = size;
    return DEPZ_OK;
}

void depz_vl53l8_cnh_pack(const depz_vl53l8_cnh_setup *s, uint8_t out[156])
{
    int k;
    wr_u32le(out, (uint32_t)s->ref_bin_offset);
    wr_u32le(out + 4, s->detection_threshold);
    wr_u32le(out + 8, s->extra_noise_sigma);
    wr_u32le(out + 12, s->null_den_clip_value);
    out[16] = s->mem_update_mode;
    out[17] = s->mem_update_choice;
    out[18] = s->sum_span;
    out[19] = s->feature_length;
    out[20] = s->nb_of_aggregates;
    out[21] = s->nb_of_temporal_accumulations;
    out[22] = s->min_nb_for_global_detection;
    out[23] = s->global_indicator_format_1;
    out[24] = s->global_indicator_format_2;
    out[25] = s->cnh_cfg;
    out[26] = s->cnh_flex_shift;
    out[27] = s->spare_3;
    for (k = 0; k < 64; k++) out[28 + k] = (uint8_t)s->map_id[k];
    memcpy(out + 92, s->indicator_format_1, 32);
    memcpy(out + 124, s->indicator_format_2, 32);
}

void depz_vl53l8_cnh_decode_config(const depz_vl53l8_cnh_setup *s, depz_vl53l8ch_cnh_config *out)
{
    out->nb_of_aggregates = s->nb_of_aggregates;
    out->feature_length = s->feature_length;
}
