/* fake_vl53l8.c — see fake_vl53l8.h. */
#include "fake_vl53l8.h"

#include "../src/io/platform.h"

#include <stdlib.h>
#include <string.h>

#define UI_CMD_STATUS 0x2C00u
#define UI_CMD_START  0x2C04u
#define UI_CMD_END    0x2FFFu
#define NVM_SIZE      492u
#define XTALK_SIZE    776u

/* DCI indices (uld.py / vl53l8cx_api.h). */
#define DCI_ZONE_CONFIG              0x5450u
#define DCI_FREQ_HZ                  0x5458u
#define DCI_INT_TIME                 0x545Cu
#define DCI_CAL_CFG                  0x5470u
#define DCI_DET_THRESH_CONFIG        0x5488u
#define DCI_RANGING_MODE             0xAD30u
#define DCI_DSS_CONFIG               0xAD38u
#define DCI_XTALK_CFG                0xAD94u
#define DCI_TARGET_ORDER             0xAE64u
#define DCI_SHARPENER                0xAED8u
#define DCI_DET_THRESH_GLOBAL_CONFIG 0xB6E0u
#define DCI_DET_THRESH_START         0xB6E8u
#define DCI_DET_THRESH_VALID_STATUS  0xB9F0u
#define DCI_OUTPUT_CONFIG            0xD968u
#define DCI_PIPE_CONTROL             0xDB80u
#define DCI_SINGLE_RANGE             0xD964u
#define DCI_FW_FLAGS                 0xE0C4u
#define DCI_FRAME_SIZE               0x5440u
#define DCI_OUTPUT_ENABLES           0xD970u
#define DCI_OUTPUT_LIST              0xD980u

#define FRAME_ID      0x0017u /* header / footer id, as the lab board */
#define FOOTER_ID_OFF 4u      /* CH / L7 blobs: footer id at size-4 */
#define CHUNK_MAX     1528u   /* VL53_STREAM_FRAME_MAX (contract 04 §4 rule 4) */

#define N_DCI 64

typedef struct { uint16_t index; uint8_t *data; size_t len; } dci_entry;
typedef enum { PEND_NONE, PEND_DCI, PEND_NVM, PEND_XTALK } pending_kind;

struct fake_vl53l8 {
    depz_link  *side;
    depz_thread thread;
    depz_mutex  lock;
    depz_parser parser;
    uint8_t     tx_seq;
    uint8_t     frame[2200];
    uint8_t     out[2100];
    uint32_t    fw_crc;
    int         page;
    uint8_t     regs[3][0x8000];     /* pages 0..2 */
    dci_entry   dci[N_DCI];
    size_t      n_dci;
    pending_kind pending;
    uint16_t    pend_index, pend_size;
    uint8_t     xtalk_buf[XTALK_SIZE];
    uint8_t     latched[4];          /* page 2 register 0x00: the calibration result header */
    int         calib_polls;
    bool        calib_armed;
    bool        xtalk_cal_fails, laser_fault, ranging;
    bool        streaming;
    uint16_t    frame_size;          /* the last START_STREAM's */
    uint8_t     frame_count;
    size_t      n_writes;
    int         boots, config_uploads, offset_uploads, xtalk_uploads;
    int         calibrations;
    size_t      calib_table_len;
    uint16_t    cal_dist4, cal_refl16;
    uint8_t     cal_nb;
    uint8_t     fail_next[256];
    bool        silent_next[256];
    uint64_t    mcu_time_us;
};

static uint32_t rd_u32le(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void wr_u32le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

/* VL53L8CX_SwapBuffer. */
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

static dci_entry *dci_find(fake_vl53l8 *f, uint16_t index)
{
    size_t i;
    for (i = 0; i < f->n_dci; i++)
        if (f->dci[i].index == index) return &f->dci[i];
    return NULL;
}

static void dci_put(fake_vl53l8 *f, uint16_t index, const uint8_t *data, size_t len)
{
    dci_entry *e = dci_find(f, index);
    uint8_t *copy = (uint8_t *)malloc(len ? len : 1);
    if (!copy) return;
    memcpy(copy, data, len);
    if (!e) {
        if (f->n_dci == N_DCI) { free(copy); return; }
        e = &f->dci[f->n_dci++];
        e->index = index;
    } else {
        free(e->data);
    }
    e->data = copy;
    e->len = len;
}

/* The firmware's power-on DCI (FakeVl53l8Sensor._default_dci). */
static void default_dci(fake_vl53l8 *f)
{
    static const uint8_t zone[8] = {4, 4, 0, 0, 8, 8, 0, 0};
    static const uint8_t freq[4] = {0, 1, 0, 0};
    static const uint8_t mode[8] = {0, 1, 0, 3, 0, 0, 0, 0};
    static const uint8_t order[4] = {1, 0, 0, 0}; /* closest */
    static const uint8_t glob[8] = {1, 0, 1, 0, 0, 0, 0, 0};
    static const uint8_t pipe[4] = {1, 0, 1, 0};
    static const uint8_t single[4] = {1, 0, 0, 0};
    uint8_t b[768];
    dci_put(f, DCI_ZONE_CONFIG, zone, 8);
    memset(b, 0, 16);
    b[4] = b[6] = 64;
    b[9] = 4;
    dci_put(f, DCI_DSS_CONFIG, b, 16);
    dci_put(f, DCI_FREQ_HZ, freq, 4);
    memset(b, 0, 20);
    wr_u32le(b, 5000);
    dci_put(f, DCI_INT_TIME, b, 20);
    dci_put(f, DCI_RANGING_MODE, mode, 8);
    dci_put(f, DCI_TARGET_ORDER, order, 4);
    memset(b, 0, 16);
    b[0xD] = 5 * 255 / 100;
    dci_put(f, DCI_SHARPENER, b, 16);
    memset(b, 0, sizeof b);
    dci_put(f, DCI_XTALK_CFG, b, 16);
    dci_put(f, DCI_DET_THRESH_GLOBAL_CONFIG, glob, 8);
    dci_put(f, DCI_DET_THRESH_CONFIG, b, 20);
    dci_put(f, DCI_DET_THRESH_START, b, 768);
    dci_put(f, DCI_DET_THRESH_VALID_STATUS, b, 8);
    dci_put(f, DCI_PIPE_CONTROL, pipe, 4);
    dci_put(f, DCI_SINGLE_RANGE, single, 4);
    dci_put(f, DCI_CAL_CFG, b, 8);
}

static void send_locked(fake_vl53l8 *f, uint8_t cmd, const uint8_t *payload, size_t len)
{
    size_t n;
    if (depz_build_packet(cmd, payload, len, f->tx_seq, DEPZ_CRC_NONE, f->frame, sizeof f->frame, &n) == 0 &&
        depz_link_write(f->side, f->frame, n) == DEPZ_OK)
        f->tx_seq = (uint8_t)(f->tx_seq + 1); /* wraps mod 256 (contract 01 §5) */
}

static void status(fake_vl53l8 *f, uint8_t cmd, uint8_t st)
{
    uint8_t p[2];
    p[0] = cmd;
    p[1] = st;
    send_locked(f, DEPZ_RPT_STATUS, p, 2);
}

uint32_t fake_vl53l8_xtalk_grid_value(unsigned distance_mm, unsigned reflectance_pc, unsigned nb, unsigned zone)
{
    return (uint32_t)(reflectance_pc * 1000u + distance_mm + nb * 16u + zone);
}

/* plugin_xtalk: the FW fills the 8x8 signal grid (0x34, swapped domain). */
static void run_xtalk_calibration(fake_vl53l8 *f)
{
    dci_entry *cfg = dci_find(f, DCI_CAL_CFG);
    uint8_t buf[XTALK_SIZE];
    unsigned z;
    f->calib_armed = false;
    if (!cfg || cfg->len < 5) return;
    f->cal_dist4 = (uint16_t)(cfg->data[0] | cfg->data[1] << 8);
    f->cal_refl16 = (uint16_t)(cfg->data[2] | cfg->data[3] << 8);
    f->cal_nb = cfg->data[4];
    f->calibrations++;
    f->calib_polls = 1;
    if (f->xtalk_cal_fails) {
        static const uint8_t failed[4] = {0x01, 0x05, 0x7F, 0x90}; /* GO2 error: XTALK_FAILED */
        memcpy(f->latched, failed, 4);
        return;
    }
    {
        static const uint8_t done[4] = {0x01, 0x05, 0x05, 0x10};
        memcpy(f->latched, done, 4);
    }
    memcpy(buf, f->xtalk_buf, XTALK_SIZE);
    swap_buffer(buf, XTALK_SIZE);
    for (z = 0; z < 64; z++)
        wr_u32le(buf + 0x34 + 4 * z, fake_vl53l8_xtalk_grid_value(f->cal_dist4 / 4u, f->cal_refl16 / 16u, f->cal_nb, z));
    swap_buffer(buf, XTALK_SIZE);
    memcpy(f->xtalk_buf, buf, XTALK_SIZE);
}

/* Page-2 UI command area and the data uploads; true = consumed. */
static bool ui_write(fake_vl53l8 *f, uint16_t addr, const uint8_t *d, size_t n)
{
    static const uint8_t start_cmd[4] = {0x00, 0x03, 0x00, 0x00};
    if (addr == UI_CMD_END - 11 && n == 12) { /* DCI read request */
        f->pending = PEND_DCI;
        f->pend_index = (uint16_t)(d[0] << 8 | d[1]);
        f->pend_size = (uint16_t)(d[2] << 4 | d[3] >> 4);
        return true;
    }
    if (addr == 0x2FD8) { f->pending = PEND_NVM; return true; }
    if (addr == 0x2FB8) { f->pending = PEND_XTALK; return true; }
    if (addr == 0x2E18) { f->offset_uploads++; return true; }
    if (addr == 0x2CF8) {
        f->xtalk_uploads++;
        memcpy(f->xtalk_buf, d, n < XTALK_SIZE ? n : XTALK_SIZE);
        return true;
    }
    if (addr == 0x2C34) { f->config_uploads++; return true; }
    if (addr == 0x2C28) {
        f->calib_table_len = n;
        f->calib_armed = true;
        return true;
    }
    if (addr == UI_CMD_END - 3 && n == 4 && memcmp(d, start_cmd, 4) == 0) {
        if (f->calib_armed) run_xtalk_calibration(f);
        else f->ranging = true;
        return true;
    }
    if (n >= 12 && d[n - 5] == 0x0F && d[n - 4] == 0x05 && d[n - 3] == 0x01) { /* DCI write */
        uint16_t index = (uint16_t)(d[0] << 8 | d[1]);
        size_t size = (size_t)(d[2] << 4 | d[3] >> 4);
        uint8_t *payload;
        if (4 + size > n) return true;
        payload = (uint8_t *)malloc(size ? size : 1);
        if (!payload) return true;
        memcpy(payload, d + 4, size);
        swap_buffer(payload, size);
        dci_put(f, index, payload, size);
        free(payload);
        return true;
    }
    return false;
}

static void boot(fake_vl53l8 *f)
{
    f->boots++;
    f->regs[2][0x2FFC] = (uint8_t)(f->fw_crc >> 24); /* read back swapped: the crc */
    f->regs[2][0x2FFD] = (uint8_t)(f->fw_crc >> 16);
    f->regs[2][0x2FFE] = (uint8_t)(f->fw_crc >> 8);
    f->regs[2][0x2FFF] = (uint8_t)f->fw_crc;
    f->regs[0][0x06] = 0x01;
    f->ranging = false;
}

static void page0_side_effects(fake_vl53l8 *f, uint16_t a, uint8_t v)
{
    if (a == 0x14) { /* provoke / undo MCU stop */
        if (v == 0x01) {
            f->regs[0][0x06] |= 0x80;
            f->regs[0][0x07] = 0x84;
            f->ranging = false;
        } else {
            f->regs[0][0x06] &= 0x7F;
        }
    } else if (a == 0x0B && v == 0x01) {
        boot(f);
    } else if (a == 0x09) { /* 0x02 sleep (MCU down), 0x04 awake, 0x05 xshut bypass */
        if (v == 0x02) f->regs[0][0x06] &= 0xFE;
        else if (v == 0x04 || v == 0x05) f->regs[0][0x06] |= 0x01;
    }
}

static void chip_write(fake_vl53l8 *f, uint16_t addr, const uint8_t *d, size_t n)
{
    size_t i;
    if (addr == 0x7FFF && n == 1) { f->page = d[0]; return; }
    if (f->page >= 0x09 && f->page <= 0x0B) return; /* a firmware bank */
    if (f->page == 2 && ui_write(f, addr, d, n)) return;
    if (f->page > 2) return;
    for (i = 0; i < n && addr + i < 0x8000; i++) {
        f->regs[f->page][addr + i] = d[i];
        if (f->page == 0) page0_side_effects(f, (uint16_t)(addr + i), d[i]);
    }
}

static size_t ui_answer(fake_vl53l8 *f, uint8_t *out, size_t n)
{
    pending_kind k = f->pending;
    memset(out, 0, n);
    f->pending = PEND_NONE;
    if (k == PEND_NVM) {
        size_t i;
        for (i = 0; i < n && i < NVM_SIZE; i++) out[i] = (uint8_t)((i * 7 + 3) & 0xFF);
    } else if (k == PEND_XTALK) {
        if (n > 8) memcpy(out + 8, f->xtalk_buf, n - 8 < XTALK_SIZE - 8 ? n - 8 : XTALK_SIZE - 8);
    } else if (k == PEND_DCI) {
        uint8_t data[1024];
        size_t size = f->pend_size < sizeof data ? f->pend_size : sizeof data;
        memset(data, 0, sizeof data);
        if (f->pend_index == DCI_FRAME_SIZE) { /* the FW reports the frame size it will stream */
            dci_entry *oc = dci_find(f, DCI_OUTPUT_CONFIG);
            uint32_t configured = oc && oc->len >= 4 ? rd_u32le(oc->data) : 0;
            data[8] = (uint8_t)configured;
            data[9] = (uint8_t)(configured >> 8);
        } else if (f->pend_index == DCI_FW_FLAGS) {
            data[1] = 2; /* module type MZ+ */
            data[6] = f->laser_fault ? 1 : 0;
        } else {
            dci_entry *e = dci_find(f, f->pend_index);
            if (e) memcpy(data, e->data, e->len < size ? e->len : size);
        }
        /* header(4) + data + footer(8), swapped as one buffer */
        if (n >= 4) memcpy(out + 4, data, n - 4 < size ? n - 4 : size);
        swap_buffer(out, n);
    }
    return n;
}

static void chip_read(fake_vl53l8 *f, uint16_t addr, uint8_t *out, size_t n)
{
    static const uint8_t ready[4] = {0x02, 0x03, 0x00, 0x00}; /* NVM poll wants [0]==2, DCI polls [1]==3 */
    size_t i;
    memset(out, 0, n);
    if (addr == 0x7FFF) { out[0] = (uint8_t)f->page; return; }
    if (f->page == 2) {
        if (addr == UI_CMD_STATUS) { memcpy(out, ready, n < 4 ? n : 4); return; }
        if (addr == UI_CMD_START && f->pending != PEND_NONE) { ui_answer(f, out, n); return; }
        if (addr < UI_CMD_STATUS) { /* the result registers: the calibration header */
            if (f->calib_polls > 0) {
                f->calib_polls--;
                memset(out, 0xFF, n);
            } else if (addr < 4) {
                memcpy(out, f->latched + addr, n < 4u - addr ? n : 4u - addr);
            }
            return;
        }
    }
    if (f->page > 2) return;
    for (i = 0; i < n && addr + i < 0x8000; i++) out[i] = f->regs[f->page][addr + i];
}

static void handle(fake_vl53l8 *f, uint8_t cmd, const uint8_t *p, size_t len)
{
    f->mcu_time_us += 100;
    if (cmd == 0x33) f->n_writes++;
    if (f->fail_next[cmd]) {
        status(f, cmd, f->fail_next[cmd]);
        f->fail_next[cmd] = 0;
        return;
    }
    if (f->silent_next[cmd]) {
        f->silent_next[cmd] = false;
        return;
    }
    switch (cmd) {
    case 0x32: { /* READ_REG: addr u16, len u16 -> RPT 0x91: cmd, ts u64, data */
        uint16_t a, n;
        int k;
        if (len != 4) { status(f, cmd, 0x03); return; }
        a = (uint16_t)(p[0] | p[1] << 8);
        n = (uint16_t)(p[2] | p[3] << 8);
        if (n < 1 || n > 2048) { status(f, cmd, 0x04); return; }
        f->out[0] = cmd;
        for (k = 0; k < 8; k++) f->out[1 + k] = (uint8_t)(f->mcu_time_us >> (8 * k));
        chip_read(f, a, f->out + 9, n);
        send_locked(f, 0x91, f->out, 9 + (size_t)n);
        return;
    }
    case 0x33: /* WRITE_REG: addr u16, data */
        if (len < 3) { status(f, cmd, 0x03); return; }
        chip_write(f, (uint16_t)(p[0] | p[1] << 8), p + 2, len - 2);
        status(f, cmd, 0x00);
        return;
    case 0x35: /* START_STREAM: frame size u16 */
        if (len != 2) { status(f, cmd, 0x03); return; }
        f->frame_size = (uint16_t)(p[0] | p[1] << 8);
        f->streaming = true;
        status(f, cmd, 0x00);
        return;
    case 0x36: f->streaming = false; status(f, cmd, 0x00); return; /* STOP_STREAM */
    default: status(f, cmd, 0x02);
    }
}

static void on_event(const depz_event *e, void *user)
{
    fake_vl53l8 *f = (fake_vl53l8 *)user;
    if (e->type != DEPZ_EV_PACKET) return;
    depz_mutex_lock(&f->lock);
    handle(f, e->cmd, e->payload, e->payload_len);
    depz_mutex_unlock(&f->lock);
}

static void run(void *arg)
{
    fake_vl53l8 *f = (fake_vl53l8 *)arg;
    uint8_t buf[4096];
    for (;;) {
        size_t got;
        if (depz_link_read(f->side, buf, sizeof buf, 1000, &got) != DEPZ_OK) return;
        if (got) depz_parser_feed(&f->parser, buf, got, on_event, f);
    }
}

fake_vl53l8 *fake_vl53l8_start(uint32_t fw_crc, depz_link **host_link)
{
    fake_vl53l8 *f = (fake_vl53l8 *)calloc(1, sizeof *f);
    if (!f) return NULL;
    if (depz_link_loopback_pair(host_link, &f->side)) { free(f); return NULL; }
    depz_mutex_init(&f->lock);
    depz_parser_init(&f->parser);
    f->fw_crc = fw_crc;
    f->mcu_time_us = 1000000;
    default_dci(f);
    /* a booted sensor, awake: the silicon ids on page 0 */
    f->regs[0][0x00] = 0xF0;
    f->regs[0][0x01] = 0x0C;
    f->regs[0][0x06] = 0x01;
    f->regs[0][0x09] = 0x04;
    f->regs[1][0x21] = 0x04;
    depz_thread_start(&f->thread, run, f);
    return f;
}

void fake_vl53l8_stop(fake_vl53l8 *f)
{
    size_t i;
    if (!f) return;
    depz_link_close(f->side);
    depz_thread_join(f->thread);
    depz_link_free(f->side);
    depz_parser_free(&f->parser);
    depz_mutex_destroy(&f->lock);
    for (i = 0; i < f->n_dci; i++) free(f->dci[i].data);
    free(f);
}

uint8_t fake_vl53l8_reg(fake_vl53l8 *f, int page, uint16_t addr)
{
    uint8_t v;
    depz_mutex_lock(&f->lock);
    v = page >= 0 && page <= 2 && addr < 0x8000 ? f->regs[page][addr] : 0;
    depz_mutex_unlock(&f->lock);
    return v;
}

void fake_vl53l8_set_reg(fake_vl53l8 *f, int page, uint16_t addr, uint8_t v)
{
    depz_mutex_lock(&f->lock);
    if (page >= 0 && page <= 2 && addr < 0x8000) f->regs[page][addr] = v;
    depz_mutex_unlock(&f->lock);
}

int fake_vl53l8_page(fake_vl53l8 *f)
{
    int p;
    depz_mutex_lock(&f->lock);
    p = f->page;
    depz_mutex_unlock(&f->lock);
    return p;
}

size_t fake_vl53l8_dci(fake_vl53l8 *f, uint16_t index, uint8_t *out, size_t cap)
{
    dci_entry *e;
    size_t n = 0;
    depz_mutex_lock(&f->lock);
    e = dci_find(f, index);
    if (e) {
        n = e->len;
        memcpy(out, e->data, n < cap ? n : cap);
    }
    depz_mutex_unlock(&f->lock);
    return n;
}

#define GETTER(type, name, expr)            \
    type name(fake_vl53l8 *f)               \
    {                                       \
        type v;                             \
        depz_mutex_lock(&f->lock);          \
        v = (expr);                         \
        depz_mutex_unlock(&f->lock);        \
        return v;                           \
    }

GETTER(size_t, fake_vl53l8_write_count, f->n_writes)
GETTER(int, fake_vl53l8_boots, f->boots)
GETTER(int, fake_vl53l8_config_uploads, f->config_uploads)
GETTER(int, fake_vl53l8_offset_uploads, f->offset_uploads)
GETTER(int, fake_vl53l8_xtalk_uploads, f->xtalk_uploads)
GETTER(bool, fake_vl53l8_ranging, f->ranging)
GETTER(uint16_t, fake_vl53l8_frame_size, f->frame_size)

/* DCI block bytes [0, size), zero past what was written. */
static void dci_get(fake_vl53l8 *f, uint16_t index, uint8_t *out, size_t size)
{
    dci_entry *e = dci_find(f, index);
    memset(out, 0, size);
    if (e) memcpy(out, e->data, e->len < size ? e->len : size);
}

bool fake_vl53l8_send_frame(fake_vl53l8 *f)
{
    static uint8_t chunk[12 + CHUNK_MAX];
    uint8_t list[64], en[16], cfg[8], *buf;
    dci_entry *e;
    size_t size, n_list, i, pos, off;
    depz_mutex_lock(&f->lock);
    e = dci_find(f, DCI_OUTPUT_LIST);
    if (!f->streaming || !e) { depz_mutex_unlock(&f->lock); return false; }
    n_list = (e->len < sizeof list ? e->len : sizeof list) / 4;
    memcpy(list, e->data, 4 * n_list);
    dci_get(f, DCI_OUTPUT_ENABLES, en, sizeof en);
    dci_get(f, DCI_OUTPUT_CONFIG, cfg, sizeof cfg);
    size = rd_u32le(cfg);
    buf = (uint8_t *)calloc(size ? size : 1, 1);
    if (!buf || size < 16 + FOOTER_ID_OFF) { free(buf); depz_mutex_unlock(&f->lock); return false; }
    /* 16-byte head: status word, 0, header id, START block; then each enabled
     * block header with zero data, the 0x0E / 0x0F tail words, the footer id. */
    buf[0] = 0x31; buf[1] = 0x9D; buf[2] = 0x05; f->frame_count = (uint8_t)(f->frame_count + 1);
    buf[3] = f->frame_count;
    buf[8] = (uint8_t)FRAME_ID; buf[9] = (uint8_t)(FRAME_ID >> 8);
    memcpy(buf + 12, list, 4);
    pos = 16;
    for (i = 1; i < n_list; i++) {
        uint32_t bh = rd_u32le(list + 4 * i), t = bh & 0xF, sz = (bh >> 4) & 0xFFF;
        size_t msize = (t > 0x1 && t < 0xD) ? (size_t)t * sz : sz;
        if (!(rd_u32le(en + 4 * (i / 32)) >> (i % 32) & 1)) continue;
        if (pos + 4 + msize + 8 > size) break;
        wr_u32le(buf + pos, bh);
        pos += 4 + msize;
    }
    if (pos + 8 <= size) { wr_u32le(buf + pos, 0x0E); wr_u32le(buf + pos + 4, 0x0F); }
    buf[size - FOOTER_ID_OFF] = (uint8_t)FRAME_ID;
    buf[size - FOOTER_ID_OFF + 1] = (uint8_t)(FRAME_ID >> 8);
    swap_buffer(buf, size);
    f->mcu_time_us += 66666;
    for (off = 0; off < size; off += CHUNK_MAX) {
        size_t n = size - off < CHUNK_MAX ? size - off : CHUNK_MAX;
        int k;
        for (k = 0; k < 8; k++) chunk[k] = (uint8_t)(f->mcu_time_us >> (8 * k));
        chunk[8] = (uint8_t)size; chunk[9] = (uint8_t)(size >> 8);
        chunk[10] = (uint8_t)off; chunk[11] = (uint8_t)(off >> 8);
        memcpy(chunk + 12, buf + off, n);
        send_locked(f, 0x93, chunk, 12 + n);
    }
    free(buf);
    depz_mutex_unlock(&f->lock);
    return true;
}

bool fake_vl53l8_last_xtalk(fake_vl53l8 *f, uint8_t out[776])
{
    bool any;
    depz_mutex_lock(&f->lock);
    any = f->xtalk_uploads > 0;
    memcpy(out, f->xtalk_buf, XTALK_SIZE);
    depz_mutex_unlock(&f->lock);
    return any;
}

int fake_vl53l8_calibrations(fake_vl53l8 *f, size_t *table_len, uint16_t *dist4, uint16_t *refl16, uint8_t *nb)
{
    int n;
    depz_mutex_lock(&f->lock);
    n = f->calibrations;
    if (table_len) *table_len = f->calib_table_len;
    if (dist4) *dist4 = f->cal_dist4;
    if (refl16) *refl16 = f->cal_refl16;
    if (nb) *nb = f->cal_nb;
    depz_mutex_unlock(&f->lock);
    return n;
}

void fake_vl53l8_xtalk_cal_fails(fake_vl53l8 *f, bool on)
{
    depz_mutex_lock(&f->lock);
    f->xtalk_cal_fails = on;
    depz_mutex_unlock(&f->lock);
}

void fake_vl53l8_laser_safety_fault(fake_vl53l8 *f, bool on)
{
    depz_mutex_lock(&f->lock);
    f->laser_fault = on;
    depz_mutex_unlock(&f->lock);
}

void fake_vl53l8_fail_next(fake_vl53l8 *f, uint8_t cmd, uint8_t st)
{
    depz_mutex_lock(&f->lock);
    f->fail_next[cmd] = st;
    depz_mutex_unlock(&f->lock);
}

void fake_vl53l8_silent_next(fake_vl53l8 *f, uint8_t cmd)
{
    depz_mutex_lock(&f->lock);
    f->silent_next[cmd] = true;
    depz_mutex_unlock(&f->lock);
}
