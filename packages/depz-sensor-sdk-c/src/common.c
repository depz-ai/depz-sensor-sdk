/* Common command/report codecs (contract 02).
 *
 * Payload codecs return raw integers exactly as on the wire; unit conversions
 * happen in the device layer.
 */
#include "depz_sensor_sdk.h"
#include <string.h>

static void put_u64_le(uint8_t *p, uint64_t v)
{
    for (int i = 0; i < 8; i++)
        p[i] = (uint8_t)((v >> (8 * i)) & 0xFFu);
}

static uint64_t get_u64_le(const uint8_t *p)
{
    uint64_t v = 0;
    for (int i = 0; i < 8; i++)
        v |= (uint64_t)p[i] << (8 * i);
    return v;
}

size_t depz_pack_sync_time(uint64_t pc_timestamp_us, uint8_t *out)
{
    put_u64_le(out, pc_timestamp_us);
    return 8;
}

size_t depz_pack_set_payload_crc_type(uint8_t crc_type, uint8_t *out)
{
    /* contract 02 §2: 0..3 — refused before anything is written (07 §3) */
    if (crc_type > (uint8_t)DEPZ_CRC32)
        return 0;
    out[0] = crc_type;
    return 1;
}

size_t depz_pack_sync_pin_config(const depz_sync_pin_config *c, uint8_t *out)
{
    /* contract 02 §6: logical pins 1..5, modes 0..4, polarity 0..1 */
    if (c->pin < 1 || c->pin > 5 || c->mode > DEPZ_SYNC_PIN_OUT_BOTH || c->polarity > 1)
        return 0;
    out[0] = c->pin;
    out[1] = c->mode;
    out[2] = c->polarity;
    return 3;
}

int depz_unpack_status(const uint8_t *p, size_t len, depz_status_report *out)
{
    if (len < 2)
        return -1;
    out->cmd = p[0];
    out->status = p[1];
    return 0;
}

int depz_unpack_text(const uint8_t *p, size_t len, depz_text_report *out)
{
    if (len < 1)
        return -1;
    out->cmd = p[0];
    depz_strip_device_string(p + 1, len - 1, out->text, sizeof(out->text));
    return 0;
}

int depz_unpack_text_ex(const uint8_t *p, size_t len, uint8_t *cmd,
                        char *text, size_t text_cap, size_t *text_len)
{
    size_t n;
    if (len < 1)
        return -1;
    *cmd = p[0];
    n = depz_strip_device_string(p + 1, len - 1, text, text_cap);
    if (text_len)
        *text_len = n;
    return 0;
}

int depz_unpack_sync_time(const uint8_t *p, size_t len, depz_sync_time_report *out)
{
    if (len < 24)
        return -1;
    out->pc_timestamp_us = get_u64_le(p);
    out->mcu_rx_us = get_u64_le(p + 8);
    out->mcu_tx_us = get_u64_le(p + 16);
    return 0;
}

int depz_unpack_temperature(const uint8_t *p, size_t len, depz_temperature_report *out)
{
    if (len < 10)
        return -1;
    out->timestamp_us = get_u64_le(p);
    out->raw_decidegrees = (int16_t)((uint16_t)p[8] | ((uint16_t)p[9] << 8));
    return 0;
}

int depz_unpack_sequence_error(const uint8_t *p, size_t len, depz_sequence_error_report *out)
{
    if (len < 2)
        return -1;
    out->expected_seq = p[0];
    out->received_seq = p[1];
    return 0;
}

int depz_unpack_sync_pin_config(const uint8_t *p, size_t len, depz_sync_pin_config *out)
{
    if (len < 3)
        return -1;
    out->pin = p[0];
    out->mode = p[1];
    out->polarity = p[2];
    return 0;
}

void depz_sync_time_offset_rtt(int64_t t1, int64_t t2, int64_t t3, int64_t t4,
                               int64_t *offset_us, int64_t *rtt_us)
{
    /* C integer division truncates toward zero — matches the shared parity
     * rule offset = ((T2-T1)+(T3-T4)) // 2 (toward zero). */
    int64_t num = (t2 - t1) + (t3 - t4);
    if (offset_us)
        *offset_us = num / 2;
    if (rtt_us)
        *rtt_us = (t4 - t1) - (t3 - t2);
}
