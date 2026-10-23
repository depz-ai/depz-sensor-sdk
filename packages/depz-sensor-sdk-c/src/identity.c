/* Firmware-name identity parsing (contract 02 §4.1). */
#include "depz_sensor_sdk.h"
#include <string.h>

const char *depz_sensor_type_str(depz_sensor_type t)
{
    switch (t) {
    case DEPZ_SENSOR_SR04:    return "sr04";
    case DEPZ_SENSOR_VL53L8:  return "vl53l8";
    case DEPZ_SENSOR_BNO086:  return "bno086";
    case DEPZ_SENSOR_VL53L4:  return "vl53l4";
    case DEPZ_SENSOR_VL53L7:  return "vl53l7";
    case DEPZ_SENSOR_VL53LX:  return "vl53lx";
    case DEPZ_SENSOR_BNO055:  return "bno055";
    case DEPZ_SENSOR_TMF8829: return "tmf8829";
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
    if (out_cap == 0) { /* the length the whole string needs */
        size_t need = 0;
        for (size_t i = 0; i < len; i++)
            need += data[i] < 0x80u ? 1 : 3;
        return need;
    }
    /* ASCII: a byte >= 0x80 is U+FFFD (UTF-8 EF BF BD); an embedded 0x00 is
     * kept as a character. A character that does not fit whole is dropped
     * together with the rest. */
    size_t n = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t c = data[i];
        if (c < 0x80u) {
            if (n + 1 > out_cap - 1)
                break;
            out[n++] = (char)c;
        } else {
            if (n + 3 > out_cap - 1)
                break;
            out[n++] = (char)0xEF;
            out[n++] = (char)0xBF;
            out[n++] = (char)0xBD;
        }
    }
    memset(out + n, 0, out_cap - n);
    return n;
}

size_t depz_device_string_len(const char *buf, size_t cap)
{
    while (cap > 0 && buf[cap - 1] == '\0')
        cap--;
    return cap;
}

/* Match `\d+(\.\d+)*` over exactly s[0..n). */
static bool version_matches(const char *s, size_t n)
{
    size_t i = 0;
    size_t digits = 0;
    while (i < n && s[i] >= '0' && s[i] <= '9') { i++; digits++; }
    if (digits == 0)
        return false;
    while (i < n && s[i] == '.') {
        i++;
        digits = 0;
        while (i < n && s[i] >= '0' && s[i] <= '9') { i++; digits++; }
        if (digits == 0)
            return false;
    }
    return i == n;
}

/* `needle` (NUL-free) occurs in s[0..n). */
static bool contains_n(const char *s, size_t n, const char *needle)
{
    size_t k = strlen(needle);
    for (size_t i = 0; i + k <= n; i++)
        if (memcmp(s + i, needle, k) == 0)
            return true;
    return false;
}

static bool starts_with_n(const char *s, size_t n, const char *prefix)
{
    size_t k = strlen(prefix);
    return n >= k && memcmp(s, prefix, k) == 0;
}

/* Copy s[0..n) into dst[cap] NUL-filled, never cutting a UTF-8 character. */
static void copy_truncated(char *dst, size_t cap, const char *s, size_t n)
{
    memset(dst, 0, cap);
    if (n > cap - 1) {
        n = cap - 1;
        while (n > 0 && ((unsigned char)s[n] & 0xC0u) == 0x80u)
            n--;
    }
    memcpy(dst, s, n);
}

/* Product tokens in contract order: the first one found in the name wins. */
static const struct { const char *token; depz_sensor_type type; } PRODUCT_TOKENS[] = {
    {"SR04", DEPZ_SENSOR_SR04},
    {"VL53L4", DEPZ_SENSOR_VL53L4},
    {"VL53L8", DEPZ_SENSOR_VL53L8},
    {"VL53L7", DEPZ_SENSOR_VL53L7},
    {"VL53L0_4", DEPZ_SENSOR_VL53LX}, /* contract 12 */
    {"VL53LX", DEPZ_SENSOR_VL53LX},
    {"BNO086", DEPZ_SENSOR_BNO086},
    {"BNO055", DEPZ_SENSOR_BNO055},   /* contract 13 */
    {"TMF8829", DEPZ_SENSOR_TMF8829}, /* contract 14 */
};

void depz_parse_software_name_n(const char *name, size_t len, depz_identity *out)
{
    memset(out, 0, sizeof(*out));
    if (!name)
        len = 0;
    copy_truncated(out->software_name, sizeof(out->software_name), name ? name : "", len);

    /* Regex `_v(\d+(?:\.\d+)*)$`: leftmost "_v" whose tail matches to end. */
    for (size_t i = 0; i + 1 < len; i++) {
        if (name[i] == '_' && name[i + 1] == 'v' && version_matches(name + i + 2, len - i - 2)) {
            copy_truncated(out->version, sizeof(out->version), name + i + 2, len - i - 2);
            break;
        }
    }

    if (starts_with_n(name, len, "BOOTDEPZ")) {
        out->mode = "bootloader";
        out->sensor_type = DEPZ_SENSOR_NONE;
        return;
    }
    if (starts_with_n(name, len, "APP_")) {
        out->mode = "app";
        out->sensor_type = DEPZ_SENSOR_UNKNOWN;
        for (size_t t = 0; t < sizeof PRODUCT_TOKENS / sizeof PRODUCT_TOKENS[0]; t++) {
            if (contains_n(name, len, PRODUCT_TOKENS[t].token)) {
                out->sensor_type = PRODUCT_TOKENS[t].type;
                break;
            }
        }
        return;
    }
    out->mode = "unknown";
    out->sensor_type = DEPZ_SENSOR_NONE;
}

void depz_parse_software_name(const char *name, depz_identity *out)
{
    depz_parse_software_name_n(name, name ? strlen(name) : 0, out);
}

/* ---- class resolution (contract 02 §4.4) ---------------------------------- */

static const char *const SENSOR_CLASS_NAMES[DEPZ_SENSOR_CLASS_COUNT] = {
    "sr04",     "vl53l4cd", "vl53l8cx", "vl53l8ch", "vl53l5cx", "vl53l7cx",
    "vl53l7ch", "vl53l0x",  "vl53l1cx", "vl53l1cb", "vl53l3cx", "vl53l4cx",
    "vl53lx",   "bno086",   "bno055",   "tmf8829",  "device",   "bootloader",
};

const char *depz_sensor_class_name(size_t i)
{
    return i < DEPZ_SENSOR_CLASS_COUNT ? SENSOR_CLASS_NAMES[i] : NULL;
}

const char *depz_resolve_sensor_class_n(const char *software_name, size_t software_name_len,
                                        const char *device_name, size_t device_name_len,
                                        const int *usb_vid, const int *usb_pid)
{
    depz_identity ident;
    const char *model;
    if (!device_name)
        device_name_len = 0;
    /* each USB id, when known, is a u16 — even if the other is not */
    if ((usb_vid && (*usb_vid < 0 || *usb_vid > 0xFFFF)) || (usb_pid && (*usb_pid < 0 || *usb_pid > 0xFFFF)))
        return NULL;
    depz_parse_software_name_n(software_name, software_name_len, &ident);
    if (ident.mode && !strcmp(ident.mode, "bootloader")) return "bootloader";
    model = depz_usb_model_hint(usb_vid ? *usb_vid : -1, usb_pid ? *usb_pid : -1);
    switch (ident.sensor_type) {
    case DEPZ_SENSOR_SR04:    return "sr04";
    case DEPZ_SENSOR_VL53L8:
        /* CX and CH share firmware and silicon: only the production PID
         * tells them apart; anything else falls to the CX base. */
        return model && !strcmp(model, "vl53l8ch") ? "vl53l8ch" : "vl53l8cx";
    case DEPZ_SENSOR_VL53L7:
        return depz_vl53l7_model_str(depz_vl53l7_resolve_model_n(
            model, model ? strlen(model) : 0, device_name, device_name_len));
    case DEPZ_SENSOR_VL53LX:
        return depz_vl53lx_class_str(depz_vl53lx_resolve_class_n(
            model, model ? strlen(model) : 0, device_name, device_name_len));
    case DEPZ_SENSOR_VL53L4:  return "vl53l4cd";
    case DEPZ_SENSOR_BNO086:  return "bno086";
    case DEPZ_SENSOR_BNO055:  return "bno055";
    case DEPZ_SENSOR_TMF8829: return "tmf8829";
    default:                  return "device";
    }
}

const char *depz_resolve_sensor_class(const char *software_name, const char *device_name,
                                      int usb_vid, int usb_pid)
{
    /* -1 (any negative) = unknown, as documented; an id above 0xFFFF matches
     * no model (the _n form refuses it) */
    int vid = usb_vid > 0xFFFF ? -1 : usb_vid, pid = usb_pid > 0xFFFF ? -1 : usb_pid;
    return depz_resolve_sensor_class_n(software_name, software_name ? strlen(software_name) : 0,
                                       device_name, device_name ? strlen(device_name) : 0,
                                       vid < 0 ? NULL : &vid, pid < 0 ? NULL : &pid);
}
