/* Firmware-name identity parsing (contract 02 §4.1). */
#include "depz_sensor_sdk.h"
#include <string.h>

const char *depz_sensor_type_str(depz_sensor_type t)
{
    switch (t) {
    case DEPZ_SENSOR_SR04:    return "sr04";
    case DEPZ_SENSOR_VL53L8:  return "vl53l8";
    case DEPZ_SENSOR_BNO086:  return "bno086";
    case DEPZ_SENSOR_UNKNOWN: return "unknown";
    default:                  return NULL; /* DEPZ_SENSOR_NONE */
    }
}

size_t depz_strip_device_string(const uint8_t *data, size_t len,
                                char *out, size_t out_cap)
{
    /* Drop trailing 0x00 / 0xFF filler (unprogrammed flash). */
    while (len > 0 && (data[len - 1] == 0x00u || data[len - 1] == 0xFFu))
        len--;
    if (out_cap == 0)
        return len;
    size_t n = len < out_cap - 1 ? len : out_cap - 1;
    for (size_t i = 0; i < n; i++)
        out[i] = (char)data[i];
    out[n] = '\0';
    return n;
}

/* Match `\d+(\.\d+)*` anchored to the end of `s` starting at index i.
 * Returns true (and consumes to end) only on a full match. */
static bool version_matches_to_end(const char *s)
{
    size_t i = 0;
    size_t digits = 0;
    while (s[i] >= '0' && s[i] <= '9') { i++; digits++; }
    if (digits == 0)
        return false;
    while (s[i] == '.') {
        i++;
        digits = 0;
        while (s[i] >= '0' && s[i] <= '9') { i++; digits++; }
        if (digits == 0)
            return false;
    }
    return s[i] == '\0';
}

void depz_parse_software_name(const char *name, depz_identity *out)
{
    memset(out, 0, sizeof(*out));
    strncpy(out->software_name, name, sizeof(out->software_name) - 1);
    out->version[0] = '\0';

    /* Regex `_v(\d+(?:\.\d+)*)$`: leftmost "_v" whose tail matches to end. */
    size_t nlen = strlen(name);
    for (size_t i = 0; i + 1 < nlen; i++) {
        if (name[i] == '_' && name[i + 1] == 'v') {
            const char *tail = name + i + 2;
            if (version_matches_to_end(tail)) {
                strncpy(out->version, tail, sizeof(out->version) - 1);
                break;
            }
        }
    }

    if (strncmp(name, "BOOTDEPZ", 8) == 0) {
        out->mode = "bootloader";
        out->sensor_type = DEPZ_SENSOR_NONE;
        return;
    }
    if (strncmp(name, "APP_", 4) == 0) {
        out->mode = "app";
        if (strstr(name, "SR04"))
            out->sensor_type = DEPZ_SENSOR_SR04;
        else if (strstr(name, "VL53L8"))
            out->sensor_type = DEPZ_SENSOR_VL53L8;
        else if (strstr(name, "BNO086"))
            out->sensor_type = DEPZ_SENSOR_BNO086;
        else
            out->sensor_type = DEPZ_SENSOR_UNKNOWN;
        return;
    }
    out->mode = "unknown";
    out->sensor_type = DEPZ_SENSOR_NONE;
}
