/* Bootloader wire codecs and .fwdepz container (contract 06). */
#include "depz_sensor_sdk.h"
#include <string.h>

static const uint8_t FWDEPZ_MAGIC[8] = { 'F', 'W', 'D', 'E', 'P', 'Z', '0', '0' };

static uint32_t get_u32_le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t get_u16_le(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

depz_fwdepz_result depz_fwdepz_parse(const uint8_t *blob, size_t len,
                                     depz_fwdepz_image *out)
{
    if (len < DEPZ_FWDEPZ_HEADER_SIZE)
        return DEPZ_FWDEPZ_ERR_TOO_SHORT;
    if (memcmp(blob, FWDEPZ_MAGIC, 8) != 0)
        return DEPZ_FWDEPZ_ERR_MAGIC;

    uint16_t hdr_crc = get_u16_le(blob + 62);
    uint16_t actual = depz_crc16_ccitt_false(blob, 62);
    if (hdr_crc != actual)
        return DEPZ_FWDEPZ_ERR_HEADER_CRC;

    uint32_t load_addr = get_u32_le(blob + 8);
    uint32_t fw_size = get_u32_le(blob + 12);
    uint32_t fw_crc32 = get_u32_le(blob + 16);

    const uint8_t *payload = blob + DEPZ_FWDEPZ_HEADER_SIZE;
    size_t payload_len = len - DEPZ_FWDEPZ_HEADER_SIZE;
    if (fw_size != payload_len)
        return DEPZ_FWDEPZ_ERR_SIZE;

    if (out) {
        out->load_addr = load_addr;
        out->fw_size = fw_size;
        out->fw_crc32 = fw_crc32;
        out->cur_sec = blob[20];
        out->tot_sec = blob[21];
        out->payload = payload;
        out->payload_len = payload_len;
        out->payload_crc_ok = (depz_crc32_iso_hdlc(payload, payload_len) == fw_crc32);
    }
    return DEPZ_FWDEPZ_OK;
}

size_t depz_bl_pack_write_page(uint32_t addr, const uint8_t *data, size_t data_len,
                               uint8_t *out, size_t out_cap)
{
    if (out_cap < 6 + data_len)
        return 0;
    out[0] = (uint8_t)(addr & 0xFFu);
    out[1] = (uint8_t)((addr >> 8) & 0xFFu);
    out[2] = (uint8_t)((addr >> 16) & 0xFFu);
    out[3] = (uint8_t)((addr >> 24) & 0xFFu);
    out[4] = (uint8_t)(data_len & 0xFFu);
    out[5] = (uint8_t)((data_len >> 8) & 0xFFu);
    if (data_len)
        memcpy(out + 6, data, data_len);
    return 6 + data_len;
}

size_t depz_bl_pack_read_page(uint32_t addr, uint16_t size, uint8_t *out)
{
    out[0] = (uint8_t)(addr & 0xFFu);
    out[1] = (uint8_t)((addr >> 8) & 0xFFu);
    out[2] = (uint8_t)((addr >> 16) & 0xFFu);
    out[3] = (uint8_t)((addr >> 24) & 0xFFu);
    out[4] = (uint8_t)(size & 0xFFu);
    out[5] = (uint8_t)((size >> 8) & 0xFFu);
    return 6;
}

int depz_bl_unpack_flash_info(const uint8_t *p, size_t len, depz_flash_info *out)
{
    /* cmd u8, page_size u16, reserved u16, app_start u32, app_size u32 */
    if (len != 13)
        return -1;
    out->page_size = get_u16_le(p + 1);
    out->app_start = get_u32_le(p + 5);
    out->app_size = get_u32_le(p + 9);
    return 0;
}
