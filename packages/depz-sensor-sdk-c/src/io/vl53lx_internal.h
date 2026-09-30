/* vl53lx_internal.h — the VL53L 1D family class and its drivers (contract 12).
 *
 * The class (vl53lx_device.c) owns the bridge, the stream and the public API;
 * a driver implements one (product, driver kind) pair on top of the platform
 * layer below, as the Python SDK's vl53lx/uld/ does: SensorDriver in
 * uld/base.py is vlx_ops here, BridgePlatform is vlx_plat.
 *
 * The rule every driver follows: the same register traffic, in the same
 * order, as its Python port — a capture made by either SDK replays strictly
 * in the other. Timeouts in poll loops are wall-clock (depz_now_us), waits go
 * through vlx_sleep_ms (skipped on a replay link: the answers are recorded).
 *
 * Errors: every function returns DEPZ_OK or a negative depz_err (with
 * depz_fail's message). Python's ProtocolError (a bridge that refused or did
 * not answer a register command) is vlx_is_protocol_error(rc); Vl53Error (the
 * sensor did not do what the driver needed) is DEPZ_E_TIMEOUT for a timeout,
 * DEPZ_E_ARG for a refused argument, DEPZ_E_PROTOCOL otherwise. */
#ifndef DEPZ_VL53LX_INTERNAL_H
#define DEPZ_VL53LX_INTERNAL_H

#include "io_internal.h"

#include <string.h>

#define VLX_TRY(expr) do { int rc_ = (expr); if (rc_) return rc_; } while (0)

/* The only bus speed an unconfigured sensor is specified for; every driver
 * runs its init here and raises the bus to its own ceiling afterwards. */
#define VLX_I2C_KHZ_BOOT 400

/* ── platform layer (uld/base.py BridgePlatform over vl53lx/_link.py) ──── */

typedef struct {
    depz_device   *dev;
    depz_regbridge rb;   /* READ_REG / WRITE_REG, 253-byte chunks, 2 s */
} vlx_plat;

/* Register values are big-endian, as the C drivers assume. */
int  vlx_rd_multi(vlx_plat *p, uint16_t addr, uint8_t *out, size_t len);
int  vlx_wr_multi(vlx_plat *p, uint16_t addr, const uint8_t *data, size_t len);
int  vlx_rd_byte(vlx_plat *p, uint16_t addr, uint8_t *v);
int  vlx_rd_word(vlx_plat *p, uint16_t addr, uint16_t *v);
int  vlx_rd_dword(vlx_plat *p, uint16_t addr, uint32_t *v);
int  vlx_wr_byte(vlx_plat *p, uint16_t addr, uint8_t v);
int  vlx_wr_word(vlx_plat *p, uint16_t addr, uint16_t v);
int  vlx_wr_dword(vlx_plat *p, uint16_t addr, uint32_t v);
/* Bridge commands outside ST's platform layer. */
int  vlx_set_i2c_speed(vlx_plat *p, uint16_t khz);
int  vlx_set_addr_width(vlx_plat *p, uint8_t width);
int  vlx_xshut_reset(vlx_plat *p);
void vlx_sleep_ms(vlx_plat *p, int ms);
/* MCU time of the latest register read (a polled measurement's stamp). */
uint64_t vlx_last_timestamp_us(const vlx_plat *p);

/* Python's ProtocolError: the bridge refused (status) or did not answer. */
static inline bool vlx_is_protocol_error(int rc)
{
    return rc == DEPZ_E_STATUS || rc == DEPZ_E_TIMEOUT || rc == DEPZ_E_BUSY;
}

/* A deadline helper for the poll loops (`time.monotonic() + timeout_s`). */
static inline uint64_t vlx_deadline_ms(int timeout_ms) { return depz_now_us() + (uint64_t)timeout_ms * 1000u; }
static inline bool vlx_past(uint64_t deadline_us) { return depz_now_us() > deadline_us; }

/* ── the driver contract (uld/base.py SensorDriver) ──────────────────── */

typedef struct vlx_driver vlx_driver;

typedef struct {
    const char *name;             /* "VL53L4", "VL53LX", ... (diagnostics)  */
    /* Bridge-side facts. */
    uint8_t  addr_width;
    uint8_t  n_clear;
    depz_vl53lx_clear_step clear[2];
    uint16_t max_khz;
    unsigned supports;            /* DEPZ_VL53LX_CAP_*                      */
    bool     histogram;
    int      budget_min_ms, budget_max_ms;

    /* Mandatory. `raw` NULL in decode = read the stream block first. */
    int  (*model_id)(vlx_driver *d, uint16_t *out);
    int  (*sensor_init)(vlx_driver *d);
    int  (*start_ranging)(vlx_driver *d);
    int  (*stop_ranging)(vlx_driver *d);
    int  (*check_for_data_ready)(vlx_driver *d, bool *ready);
    int  (*clear_interrupt)(vlx_driver *d);
    void (*stream_block)(vlx_driver *d, uint16_t *addr, uint16_t *len);
    /* Decode one streamed / polled block into `out` (timestamp left to the
     * caller). Stateful drivers step their state here — once per frame. */
    int  (*decode)(vlx_driver *d, const uint8_t *raw, size_t len, depz_vl53lx_measurement *out);
    int  (*set_range_timing)(vlx_driver *d, int budget_ms, int inter_ms);
    int  (*get_range_timing)(vlx_driver *d, int *budget_ms, int *inter_ms);
    void (*destroy)(vlx_driver *d);

    /* Optional (NULL when not served; the class checks `supports` first). */
    size_t (*modes)(const vlx_driver *d, const char **names, size_t cap);
    size_t (*budget_choices)(vlx_driver *d, int *out, size_t cap);
    int  (*set_mode)(vlx_driver *d, const char *name);
    int  (*get_mode)(vlx_driver *d, const char **name);
    uint32_t (*reach_mm)(const vlx_driver *d);
    int  (*set_offset)(vlx_driver *d, int32_t mm);
    int  (*get_offset)(vlx_driver *d, int32_t *mm);
    int  (*set_xtalk)(vlx_driver *d, int32_t kcps);
    int  (*get_xtalk)(vlx_driver *d, int32_t *kcps);
    int  (*calibrate_offset)(vlx_driver *d, int target_mm, int nb_samples, int32_t *out);
    int  (*calibrate_xtalk)(vlx_driver *d, int target_mm, int nb_samples, int32_t *out);
    int  (*set_detection_thresholds)(vlx_driver *d, int low_mm, int high_mm, int window);
    int  (*get_detection_thresholds)(vlx_driver *d, int *low_mm, int *high_mm, int *window);
    int  (*set_signal_threshold)(vlx_driver *d, int kcps);
    int  (*get_signal_threshold)(vlx_driver *d, int *kcps);
    int  (*set_sigma_threshold)(vlx_driver *d, int mm);
    int  (*get_sigma_threshold)(vlx_driver *d, int *mm);
    int  (*set_roi)(vlx_driver *d, int x, int y);
    int  (*get_roi)(vlx_driver *d, int *x, int *y);
    int  (*set_roi_center)(vlx_driver *d, int spad);
    int  (*get_roi_center)(vlx_driver *d, int *spad);
    int  (*start_temperature_update)(vlx_driver *d);
    int  (*perform_ref_spad_management)(vlx_driver *d, uint32_t *count, bool *is_aperture);
} vlx_ops;

/* A driver instance: embed this first in the driver's own struct. */
struct vlx_driver {
    const vlx_ops      *ops;
    vlx_plat           *p;
    depz_vl53lx_product product;
};

/* Constructors (one per driver kind); NULL on allocation failure. */
vlx_driver *vlx_new_l4(vlx_plat *p, depz_vl53lx_product product);   /* uld/l4.py  */
vlx_driver *vlx_new_l3(vlx_plat *p, depz_vl53lx_product product);   /* uld/l3.py  */
vlx_driver *vlx_new_l1(vlx_plat *p, depz_vl53lx_product product);   /* uld/l1.py  */
vlx_driver *vlx_new_l0x(vlx_plat *p, depz_vl53lx_product product);  /* uld/l0x.py */
vlx_driver *vlx_new_bare(vlx_plat *p, depz_vl53lx_product product); /* uld/bare/  */

/* The default poll-until-data-ready loop of SensorDriver (1 ms steps). */
int vlx_wait_data_ready(vlx_driver *d, int timeout_ms);

/* A zeroed measurement with the "no extra" markers set. */
void vlx_measurement_init(depz_vl53lx_measurement *m);

#endif /* DEPZ_VL53LX_INTERNAL_H */
