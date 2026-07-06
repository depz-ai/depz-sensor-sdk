/*
 * VL53L8 advanced-feature DCI codecs (ST ULD, UM3109), pure encode/decode:
 * xtalk margin, detection thresholds, default motion-indicator configuration.
 * Ported verbatim from the ST ULD plugin sources (BSD-3) via the TS/Python
 * references. These codecs are SHARED by both ToF variants — VL53L8CX and
 * VL53L8CH. The live DCI read/write bridge is out of scope.
 */
#include "depz_sensor_sdk.h"
#include <math.h>
#include <string.h>

static void put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static void put_i32(uint8_t *p, int32_t v) { put_u32(p, (uint32_t)v); }

uint32_t depz_vl53l8_xtalk_margin_to_raw(double margin_kcps)
{
    double r = round(margin_kcps * 2048.0);
    if (r < 0) r = 0;
    return (uint32_t)r;
}

double depz_vl53l8_xtalk_margin_from_raw(uint32_t raw)
{
    return (double)raw / 2048.0;
}

/* Detection-threshold measurement -> scale factor (get divides / set mult). */
static int32_t thresh_scale(uint8_t measurement)
{
    switch (measurement) {
    case DEPZ_VL53L8_DIST_MM:               return 4;
    case DEPZ_VL53L8_SIGNAL_PER_SPAD_KCPS:  return 2048;
    case DEPZ_VL53L8_RANGE_SIGMA_MM:        return 128;
    case DEPZ_VL53L8_AMBIENT_PER_SPAD_KCPS: return 2048;
    case DEPZ_VL53L8_NB_SPADS_ENABLED:      return 256;
    case DEPZ_VL53L8_MOTION_INDICATOR:      return 65535;
    default:                                return 1;
    }
}

void depz_vl53l8_pack_thresholds(const depz_vl53l8_threshold *th, size_t n,
                                 uint8_t start[DEPZ_VL53L8_THRESH_START_SIZE],
                                 uint8_t valid[8])
{
    memset(valid, 0x05, 8);
    memset(start, 0, DEPZ_VL53L8_THRESH_START_SIZE);
    for (int k = 0; k < DEPZ_VL53L8_NB_THRESHOLDS; k++) {
        depz_vl53l8_threshold t;
        if (th != NULL && (size_t)k < n) {
            t = th[k];
        } else {
            memset(&t, 0, sizeof(t));
        }
        int32_t scale = thresh_scale(t.measurement);
        int32_t low = t.low_thresh * scale;
        int32_t high = t.high_thresh * scale;
        uint8_t *e = start + k * 12;
        put_i32(e, low);
        put_i32(e + 4, high);
        e[8] = t.measurement;
        e[9] = t.type;
        e[10] = t.zone_num;
        e[11] = t.operation;
    }
}

/*
 * VL53L8CX_Motion_Configuration default block (156 B), packed as the C struct
 * `<i 3I 12B 64b 32B 32B` for the given resolution. Values are the constants
 * motion_indicator_init programs (plugin source).
 */
int depz_vl53l8_motion_cfg_default_pack(int resolution,
                                        uint8_t out[DEPZ_VL53L8_MOTION_CFG_SIZE])
{
    if (resolution != DEPZ_VL53L8_RES_4X4 && resolution != DEPZ_VL53L8_RES_8X8)
        return -1;

    memset(out, 0, DEPZ_VL53L8_MOTION_CFG_SIZE);
    size_t o = 0;
    put_i32(out + o, 13633);    o += 4;   /* ref_bin_offset               */
    put_u32(out + o, 2883584);  o += 4;   /* detection_threshold          */
    put_u32(out + o, 0);        o += 4;   /* extra_noise_sigma            */
    put_u32(out + o, 0);        o += 4;   /* null_den_clip_value          */

    static const uint8_t bytes12[12] = {
        6,  /* mem_update_mode              */
        2,  /* mem_update_choice            */
        4,  /* sum_span                     */
        9,  /* feature_length               */
        16, /* nb_of_aggregates             */
        16, /* nb_of_temporal_accumulations */
        1,  /* min_nb_for_global_detection  */
        8,  /* global_indicator_format_1    */
        0,  /* global_indicator_format_2    */
        0, 0, 0 /* spare1..3                */
    };
    memcpy(out + o, bytes12, 12); o += 12;

    /* map_id[64] (int8), resolution-dependent. */
    int8_t map_id[64];
    if (resolution == DEPZ_VL53L8_RES_4X4) {
        for (int k = 0; k < 16; k++) map_id[k] = (int8_t)k;
        for (int k = 16; k < 64; k++) map_id[k] = -1;
    } else {
        for (int k = 0; k < 64; k++)
            map_id[k] = (int8_t)(((k % 8) >> 1) + 4 * (k / 16));
    }
    for (int k = 0; k < 64; k++) out[o++] = (uint8_t)map_id[k];

    /* indicator_format_1[32] + indicator_format_2[32] are zero (already). */
    o += 64;
    (void)o;
    return 0;
}
