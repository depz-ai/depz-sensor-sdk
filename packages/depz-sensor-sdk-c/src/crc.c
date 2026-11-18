/* CRC algorithms of the DEPZ transport (contract 01 §3).
 *
 * Reflected table implementations, byte-exact with firmware
 * (common/crc/crc8.c) and the reference host tool. CRC-8 init is 0x00 for
 * every device (ERRATA E1). Tables are built lazily on first use, mirroring
 * the reference Python `_make_table` (reflected poly).
 */
#include "depz_sensor_sdk.h"

static uint8_t  s_crc8_table[256];
static uint16_t s_crc16_table[256];
static uint32_t s_crc32_table[256];
static bool     s_tables_ready = false;

static void build_tables(void)
{
    for (int i = 0; i < 256; i++) {
        uint8_t c8 = (uint8_t)i;
        uint16_t c16 = (uint16_t)i;
        uint32_t c32 = (uint32_t)i;
        for (int k = 0; k < 8; k++) {
            c8  = (c8  & 1u) ? (uint8_t)((c8  >> 1) ^ 0x8Cu)       : (uint8_t)(c8  >> 1);
            c16 = (c16 & 1u) ? (uint16_t)((c16 >> 1) ^ 0xA001u)    : (uint16_t)(c16 >> 1);
            c32 = (c32 & 1u) ? ((c32 >> 1) ^ 0xEDB88320u)          : (c32 >> 1);
        }
        s_crc8_table[i]  = c8;
        s_crc16_table[i] = c16;
        s_crc32_table[i] = c32;
    }
    s_tables_ready = true;
}

static void ensure_tables(void)
{
    if (!s_tables_ready) build_tables();
}

uint8_t depz_crc8_maxim(const uint8_t *data, size_t len)
{
    ensure_tables();
    uint8_t crc = 0x00u;
    for (size_t i = 0; i < len; i++)
        crc = s_crc8_table[crc ^ data[i]];
    return crc;
}

uint16_t depz_crc16_modbus(const uint8_t *data, size_t len)
{
    ensure_tables();
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < len; i++)
        crc = (uint16_t)((crc >> 8) ^ s_crc16_table[(crc ^ data[i]) & 0xFFu]);
    return crc;
}

uint32_t depz_crc32_iso_hdlc(const uint8_t *data, size_t len)
{
    ensure_tables();
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++)
        crc = (crc >> 8) ^ s_crc32_table[(crc ^ data[i]) & 0xFFu];
    return crc ^ 0xFFFFFFFFu;
}

uint16_t depz_crc16_ccitt_false(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (int k = 0; k < 8; k++)
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u)
                                  : (uint16_t)(crc << 1);
    }
    return crc;
}
