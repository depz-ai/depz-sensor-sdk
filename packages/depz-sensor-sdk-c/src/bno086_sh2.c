/*
 * BNO086 SH-2 control-response parsers, the sensor metadata record and the
 * Q-point scaling of input reports (contract 05 §4, §6). Pure codecs, ported
 * from the Python SDK's bno086/sh2.py and reports.py.
 */
#include "depz_sensor_sdk.h"
#include <string.h>

static uint16_t rd_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int depz_bno086_unpack_data(const uint8_t *p, size_t len, uint64_t *capture_us,
                            const uint8_t **shtp, size_t *shtp_len)
{
    if (len < 9) return -1;
    if (capture_us) *capture_us = (uint64_t)rd_u32(p + 1) | ((uint64_t)rd_u32(p + 5) << 32);
    if (shtp) *shtp = p + 9;
    if (shtp_len) *shtp_len = len - 9;
    return 0;
}

int depz_bno_unpack_feature_response(const uint8_t *p, size_t len, depz_bno_feature *out)
{
    if (len < 17 || p[0] != DEPZ_SH2_GET_FEATURE_RESPONSE) return -1;
    out->sensor_id = p[1];
    out->flags = p[2];
    out->sensitivity = rd_u16(p + 3);
    out->interval_us = rd_u32(p + 5);
    out->batch_us = rd_u32(p + 9);
    out->cfg_word = rd_u32(p + 13);
    return 0;
}

int depz_bno_unpack_product_id(const uint8_t *p, size_t len, depz_bno_product_id *out)
{
    if (len < 16 || p[0] != DEPZ_SH2_PRODUCT_ID_RESPONSE) return -1;
    out->reset_cause = p[1];
    out->sw_version_major = p[2];
    out->sw_version_minor = p[3];
    out->sw_part_number = rd_u32(p + 4);
    out->sw_build_number = rd_u32(p + 8);
    out->sw_version_patch = rd_u16(p + 12);
    return 0;
}

int depz_bno_unpack_command_response(const uint8_t *p, size_t len, depz_bno_command_response *out)
{
    if (len < 16 || p[0] != DEPZ_SH2_COMMAND_RESPONSE) return -1;
    out->seq = p[1];
    out->command = p[2];
    out->command_seq = p[3];
    out->response_seq = p[4];
    memcpy(out->r, p + 5, 11);
    return 0;
}

int depz_bno_unpack_frs_read_response(const uint8_t *p, size_t len, depz_bno_frs_read_response *out)
{
    if (len < 16 || p[0] != DEPZ_SH2_FRS_READ_RESPONSE) return -1;
    out->status = p[1] & 0x0F;
    out->data_length = p[1] >> 4;
    out->offset_words = rd_u16(p + 2);
    out->data0 = rd_u32(p + 4);
    out->data1 = rd_u32(p + 8);
    out->frs_type = rd_u16(p + 12);
    return 0;
}

int depz_bno_unpack_frs_write_response(const uint8_t *p, size_t len, depz_bno_frs_write_response *out)
{
    if (len < 4 || p[0] != DEPZ_SH2_FRS_WRITE_RESPONSE) return -1;
    out->status = p[1];
    out->offset_words = rd_u16(p + 2);
    return 0;
}

/* Word 3 is `power_mA u16 (Q10) | revision u16 << 16`: on the lab BNO085
 * every record reads revision 4 in the high half while the low half follows
 * the sensor's supply (accelerometer 0.13 mA, the gyro-driven outputs 5.3 mA). */
void depz_bno_metadata_from_words(const uint32_t *words, size_t n, depz_bno_metadata *out)
{
    uint32_t w[10];
    size_t k;
    for (k = 0; k < 10; k++) w[k] = k < n ? words[k] : 0;
    memset(out, 0, sizeof *out);
    out->me_version = (uint8_t)(w[0] & 0xFF);
    out->mh_version = (uint8_t)((w[0] >> 8) & 0xFF);
    out->sh_version = (uint8_t)((w[0] >> 16) & 0xFF);
    out->range_raw = w[1];
    out->resolution_raw = w[2];
    out->power_ma_q10 = (uint16_t)(w[3] & 0xFFFF);
    out->revision = (uint16_t)(w[3] >> 16);
    out->min_period_us = w[4];
    out->fifo_max = (uint16_t)(w[5] & 0xFFFF);
    out->fifo_reserved = (uint16_t)(w[5] >> 16);
    out->batch_buffer_bytes = (uint16_t)(w[6] & 0xFFFF);
    out->q_point_1 = (uint16_t)(w[7] & 0xFFFF);
    out->q_point_2 = (uint16_t)(w[7] >> 16);
    if (out->revision >= 3) out->q_point_3 = (uint16_t)(w[8] >> 16);
    if (out->revision >= 4) out->max_period_us = w[9];
}

uint16_t depz_bno_metadata_record(uint8_t sensor_id)
{
    switch (sensor_id) {
    case 0x14: return 0xE301; /* raw accelerometer */
    case 0x01: return 0xE302; /* accelerometer */
    case 0x04: return 0xE303; /* linear acceleration */
    case 0x06: return 0xE304; /* gravity */
    case 0x15: return 0xE305; /* raw gyroscope */
    case 0x02: return 0xE306; /* gyroscope calibrated */
    case 0x07: return 0xE307; /* gyroscope uncalibrated */
    case 0x16: return 0xE308; /* raw magnetometer */
    case 0x03: return 0xE309; /* magnetometer calibrated */
    case 0x0F: return 0xE30A; /* magnetometer uncalibrated */
    case 0x05: return 0xE30B; /* rotation vector */
    case 0x08: return 0xE30C; /* game rotation vector */
    case 0x09: return 0xE30D; /* geomagnetic rotation vector */
    case 0x10: return 0xE313; /* tap detector */
    case 0x18: return 0xE314; /* step detector */
    case 0x11: return 0xE315; /* step counter */
    case 0x12: return 0xE316; /* significant motion */
    case 0x13: return 0xE317; /* stability classifier */
    case 0x19: return 0xE318; /* shake detector */
    case 0x1E: return 0xE31C; /* personal activity classifier */
    case 0x28: return 0xE322; /* ARVR-stabilized RV */
    case 0x29: return 0xE323; /* ARVR-stabilized game RV */
    case 0x2A: return 0xE324; /* gyro-integrated RV */
    default:   return 0;
    }
}

/* ── scaling ───────────────────────────────────────────────────────────── */

int depz_bno_q_point(uint8_t sensor_id)
{
    switch (sensor_id) {
    case 0x01: case 0x04: case 0x06: return 8;
    case 0x02: case 0x07:            return 9;
    case 0x03: case 0x0F:            return 4;
    case 0x05: case 0x08: case 0x09:
    case 0x28: case 0x29: case 0x2A: return 14;
    case 0x0A:                       return 20;
    case 0x0B: case 0x0C:            return 8;
    case 0x0D:                       return 4;
    case 0x0E:                       return 7;
    case 0x14: case 0x15: case 0x16: return 0;
    default:                         return -1;
    }
}

static double q(int64_t raw, int qp) { return (double)raw / (double)((int64_t)1 << qp); }

static int qp_or_zero(uint8_t sensor_id)
{
    int qp = depz_bno_q_point(sensor_id);
    return qp < 0 ? 0 : qp;
}

void depz_bno_report_xyz(const depz_bno_report *r, double out[3])
{
    int qp = qp_or_zero(r->sensor_id);
    out[0] = q(r->x_raw, qp);
    out[1] = q(r->y_raw, qp);
    out[2] = q(r->z_raw, qp);
}

void depz_bno_report_bias(const depz_bno_report *r, double out[3])
{
    int qp = qp_or_zero(r->sensor_id);
    out[0] = q(r->bias_x_raw, qp);
    out[1] = q(r->bias_y_raw, qp);
    out[2] = q(r->bias_z_raw, qp);
}

void depz_bno_report_quaternion(const depz_bno_report *r, double out[4])
{
    out[0] = q(r->i_raw, 14);
    out[1] = q(r->j_raw, 14);
    out[2] = q(r->k_raw, 14);
    out[3] = q(r->real_raw, 14);
}

bool depz_bno_report_accuracy_rad(const depz_bno_report *r, double *out)
{
    if (!r->has_accuracy_raw) return false;
    *out = q(r->accuracy_raw, DEPZ_BNO_RV_ACCURACY_Q);
    return true;
}

void depz_bno_report_angular_velocity(const depz_bno_report *r, double out[3])
{
    out[0] = q(r->vx_raw, DEPZ_BNO_GYRO_RV_ANGVEL_Q);
    out[1] = q(r->vy_raw, DEPZ_BNO_GYRO_RV_ANGVEL_Q);
    out[2] = q(r->vz_raw, DEPZ_BNO_GYRO_RV_ANGVEL_Q);
}

double depz_bno_report_scalar(const depz_bno_report *r)
{
    return q(r->value_raw, qp_or_zero(r->sensor_id));
}
