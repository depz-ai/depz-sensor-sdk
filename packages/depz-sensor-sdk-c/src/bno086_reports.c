/*
 * BNO086 SH-2 input-report parsers (contract 05 §5). Byte-exact port of the
 * TS/Python `reports` reference. Raw wire integers are authoritative; Q-point
 * scaling is a host concern and intentionally not applied here (the vectors
 * assert raw ints only).
 *
 * Timestamps: channel-3/4 cargos begin with a 0xFB Base Timestamp Reference
 * (i32 delta in 100 µs ticks, SUBTRACTED from the bridge capture time); each
 * report adds a 14-bit delay (status bits 7:2 = upper 6, byte 3 = lower 8):
 *     timestamp_us = capture - base_delta*100 + delay*100
 * A 0xFA rebase ADDS its delta to the running base (batching).
 */
#include "depz_sensor_sdk.h"
#include <string.h>

static uint16_t rd_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static int16_t  rd_i16(const uint8_t *p) { return (int16_t)rd_u16(p); }
static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static int32_t rd_i32(const uint8_t *p) { return (int32_t)rd_u32(p); }

/* Total on-wire report length (4-byte SH-2 header included), 0 = unknown.
 * Mirrors the sh2 reference driver REPORT_LENGTHS table. */
static size_t report_length(uint8_t rid)
{
    switch (rid) {
    case 0x01: case 0x02: case 0x03: case 0x04: case 0x06: return 10;
    case 0x05: case 0x09: case 0x28:                       return 14;
    case 0x07: case 0x0f: case 0x14: case 0x15: case 0x16:
    case 0x1e:                                             return 16;
    case 0x08: case 0x29: case 0x11:                       return 12;
    case 0x0a: case 0x0b: case 0x18: case 0x1b:            return 8;
    case 0x10:                                             return 5;
    case 0x0c: case 0x0d: case 0x0e: case 0x12: case 0x13:
    case 0x19: case 0x1a: case 0x1c: case 0x1f: case 0x20:
    case 0x21: case 0x22: case 0x23:                       return 6;
    default:                                               return 0;
    }
}

const char *depz_bno_report_type_str(depz_bno_report_type t)
{
    switch (t) {
    case DEPZ_BNO_ACCELERATION:         return "Acceleration";
    case DEPZ_BNO_GYROSCOPE:            return "Gyroscope";
    case DEPZ_BNO_MAGNETOMETER:         return "Magnetometer";
    case DEPZ_BNO_UNCAL_GYROSCOPE:      return "UncalibratedGyroscope";
    case DEPZ_BNO_UNCAL_MAGNETOMETER:   return "UncalibratedMagnetometer";
    case DEPZ_BNO_ROTATION_VECTOR:      return "RotationVector";
    case DEPZ_BNO_SCALAR_REPORT:        return "ScalarReport";
    case DEPZ_BNO_TAP_DETECTOR:         return "TapDetector";
    case DEPZ_BNO_STEP_COUNTER:         return "StepCounter";
    case DEPZ_BNO_STEP_DETECTOR:        return "StepDetector";
    case DEPZ_BNO_SIGNIFICANT_MOTION:   return "SignificantMotion";
    case DEPZ_BNO_STABILITY_CLASSIFIER: return "StabilityClassifier";
    case DEPZ_BNO_SHAKE_DETECTOR:       return "ShakeDetector";
    case DEPZ_BNO_ACTIVITY_CLASSIFIER:  return "PersonalActivityClassifier";
    case DEPZ_BNO_RAW_SENSOR:           return "RawSensor";
    case DEPZ_BNO_GENERIC_EVENT:        return "GenericEvent";
    case DEPZ_BNO_GYRO_INTEGRATED_RV:   return "GyroIntegratedRV";
    case DEPZ_BNO_UNKNOWN_REPORT:       return "UnknownReport";
    default:                            return "UnknownReport";
    }
}

static void decode_report(uint8_t rid, const uint8_t *rep, int64_t ts,
                          uint8_t seq, uint8_t accuracy, int32_t delay_us,
                          depz_bno_report *r)
{
    memset(r, 0, sizeof(*r));
    r->sensor_id = rid;
    r->timestamp_us = ts;
    r->seq = seq;
    r->accuracy = accuracy;
    r->delay_us = delay_us;

    switch (rid) {
    case 0x01: case 0x04: case 0x06:
        r->type = DEPZ_BNO_ACCELERATION;
        r->x_raw = rd_i16(rep + 4); r->y_raw = rd_i16(rep + 6); r->z_raw = rd_i16(rep + 8);
        break;
    case 0x02:
        r->type = DEPZ_BNO_GYROSCOPE;
        r->x_raw = rd_i16(rep + 4); r->y_raw = rd_i16(rep + 6); r->z_raw = rd_i16(rep + 8);
        break;
    case 0x03:
        r->type = DEPZ_BNO_MAGNETOMETER;
        r->x_raw = rd_i16(rep + 4); r->y_raw = rd_i16(rep + 6); r->z_raw = rd_i16(rep + 8);
        break;
    case 0x07: case 0x0f:
        r->type = (rid == 0x07) ? DEPZ_BNO_UNCAL_GYROSCOPE : DEPZ_BNO_UNCAL_MAGNETOMETER;
        r->x_raw = rd_i16(rep + 4); r->y_raw = rd_i16(rep + 6); r->z_raw = rd_i16(rep + 8);
        r->bias_x_raw = rd_i16(rep + 10); r->bias_y_raw = rd_i16(rep + 12);
        r->bias_z_raw = rd_i16(rep + 14);
        break;
    case 0x05: case 0x09: case 0x28: case 0x08: case 0x29:
        r->type = DEPZ_BNO_ROTATION_VECTOR;
        r->i_raw = rd_i16(rep + 4); r->j_raw = rd_i16(rep + 6);
        r->k_raw = rd_i16(rep + 8); r->real_raw = rd_i16(rep + 10);
        if (rid != 0x08 && rid != 0x29) {
            r->accuracy_raw = rd_i16(rep + 12);
            r->has_accuracy_raw = true;
        }
        break;
    case 0x0a: case 0x0b:
        r->type = DEPZ_BNO_SCALAR_REPORT;
        r->value_raw = (int64_t)rd_u32(rep + 4);
        break;
    case 0x0c: case 0x0d:
        r->type = DEPZ_BNO_SCALAR_REPORT;
        r->value_raw = (int64_t)rd_u16(rep + 4);
        break;
    case 0x0e:
        r->type = DEPZ_BNO_SCALAR_REPORT;
        r->value_raw = (int64_t)rd_i16(rep + 4);
        break;
    case 0x10:
        r->type = DEPZ_BNO_TAP_DETECTOR;
        r->flags = rep[4];
        break;
    case 0x11:
        r->type = DEPZ_BNO_STEP_COUNTER;
        r->latency_us = rd_u32(rep + 4);
        r->steps = rd_u16(rep + 8);
        break;
    case 0x18:
        r->type = DEPZ_BNO_STEP_DETECTOR;
        r->latency_us = rd_u32(rep + 4);
        break;
    case 0x12:
        r->type = DEPZ_BNO_SIGNIFICANT_MOTION;
        r->motion = rd_u16(rep + 4);
        break;
    case 0x13:
        r->type = DEPZ_BNO_STABILITY_CLASSIFIER;
        r->classification = rep[4];
        break;
    case 0x19:
        r->type = DEPZ_BNO_SHAKE_DETECTOR;
        r->flags = rd_u16(rep + 4);
        break;
    case 0x1e:
        r->type = DEPZ_BNO_ACTIVITY_CLASSIFIER;
        r->page_number = rep[4] & 0x7f;
        r->end_of_sequence = (rep[4] & 0x80) != 0;
        r->most_likely_state = rep[5];
        memcpy(r->confidences, rep + 6, 10);
        break;
    case 0x14: case 0x16:
        r->type = DEPZ_BNO_RAW_SENSOR;
        r->x_raw = rd_i16(rep + 4); r->y_raw = rd_i16(rep + 6); r->z_raw = rd_i16(rep + 8);
        r->sensor_timestamp_us = rd_u32(rep + 12);
        r->temperature_raw = 0;
        break;
    case 0x15:
        r->type = DEPZ_BNO_RAW_SENSOR;
        r->x_raw = rd_i16(rep + 4); r->y_raw = rd_i16(rep + 6); r->z_raw = rd_i16(rep + 8);
        r->temperature_raw = rd_i16(rep + 10);
        r->sensor_timestamp_us = rd_u32(rep + 12);
        break;
    default:
        r->type = DEPZ_BNO_GENERIC_EVENT;
        r->value_raw = (int64_t)rd_u16(rep + 4);
        break;
    }
}

size_t depz_bno_parse_input_cargo(const uint8_t *payload, size_t len,
                                  uint64_t capture_timestamp_us,
                                  depz_bno_report *out, size_t cap)
{
    size_t count = 0;
    int64_t capture = (int64_t)capture_timestamp_us;
    int64_t base = capture;
    size_t pos = 0;

    while (pos < len && count < cap) {
        uint8_t rid = payload[pos];
        if (rid == DEPZ_BNO_BASE_TIMESTAMP_REF && pos + 5 <= len) {
            int32_t delta = rd_i32(payload + pos + 1);
            base = capture - (int64_t)delta * 100;
            pos += 5;
            continue;
        }
        if (rid == DEPZ_BNO_TIMESTAMP_REBASE && pos + 5 <= len) {
            int32_t delta = rd_i32(payload + pos + 1);
            base += (int64_t)delta * 100;
            pos += 5;
            continue;
        }
        size_t length = report_length(rid);
        if (length == 0 || pos + length > len) {
            depz_bno_report *r = &out[count++];
            memset(r, 0, sizeof(*r));
            r->type = DEPZ_BNO_UNKNOWN_REPORT;
            r->sensor_id = rid;
            r->timestamp_us = base;
            r->data = payload + pos;
            r->data_len = len - pos;
            break;
        }
        const uint8_t *rep = payload + pos;
        uint8_t seq = rep[1];
        uint8_t status = rep[2];
        uint8_t delay_lsb = rep[3];
        uint8_t accuracy = status & 0x03;
        int32_t delay_us = (int32_t)((((status >> 2) << 8) | delay_lsb) * 100);
        int64_t ts = base + delay_us;
        decode_report(rid, rep, ts, seq, accuracy, delay_us, &out[count++]);
        pos += length;
    }
    return count;
}

int depz_bno_parse_gyro_rv(const uint8_t *payload, size_t len,
                           uint64_t capture_timestamp_us, depz_bno_report *out)
{
    int64_t ts = (int64_t)capture_timestamp_us;
    const uint8_t *body = payload;
    size_t body_len = len;

    if (len >= 1 && payload[0] == DEPZ_BNO_BASE_TIMESTAMP_REF) {
        if (len < 5 + 2 + 14)
            return -1;
        int32_t delta = rd_i32(payload + 1);
        uint16_t delay = rd_u16(payload + 5);
        ts = (int64_t)capture_timestamp_us - (int64_t)delta * 100 + (int64_t)delay * 100;
        body = payload + 7;
        body_len = len - 7;
    }
    if (body_len < 14)
        return -1;

    memset(out, 0, sizeof(*out));
    out->type = DEPZ_BNO_GYRO_INTEGRATED_RV;
    out->sensor_id = 0x2a;
    out->timestamp_us = ts;
    out->i_raw = rd_i16(body + 0);
    out->j_raw = rd_i16(body + 2);
    out->k_raw = rd_i16(body + 4);
    out->real_raw = rd_i16(body + 6);
    out->vx_raw = rd_i16(body + 8);
    out->vy_raw = rd_i16(body + 10);
    out->vz_raw = rd_i16(body + 12);
    return 0;
}
