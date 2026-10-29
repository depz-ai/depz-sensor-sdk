/* discovery.c — pick DEPZ ports by USB identity, probe them, open the right
 * sensor class (contract 02 §4, contract 07 §3). The USB table only chooses
 * which ports to talk to; the protocol probe decides what a device is. */
#include "io_internal.h"

#include <stdlib.h>
#include <string.h>

int depz_list_serial_ports(depz_port_info **out, size_t *count)
{
    if (!out || !count) return depz_fail(DEPZ_E_ARG, "list_serial_ports: NULL argument");
    return depz_enumerate_ports(out, count);
}

void depz_free_port_list(depz_port_info *list) { free(list); }
void depz_free_device_list(depz_device_info *list) { free(list); }

/* USB serial ascending, unknown serials last, then port path (contract 02 §4,
 * the codec layer's depz_port_order_cmp). */
static int by_serial(const void *a, const void *b)
{
    const depz_port_info *x = (const depz_port_info *)a, *y = (const depz_port_info *)b;
    const depz_port_key kx = { x->port, strlen(x->port), x->usb_serial, strlen(x->usb_serial) };
    const depz_port_key ky = { y->port, strlen(y->port), y->usb_serial, strlen(y->usb_serial) };
    return depz_port_order_cmp(&kx, &ky);
}

void depz_sort_ports_by_serial(depz_port_info *ports, size_t count)
{
    if (ports && count > 1) qsort(ports, count, sizeof *ports, by_serial);
}

/* The ports with a known DEPZ USB id, sorted. */
static int depz_candidates(depz_port_info **out, size_t *count)
{
    size_t i, n = 0, total;
    depz_port_info *all;
    int rc = depz_enumerate_ports(&all, &total);
    if (rc) return rc;
    for (i = 0; i < total; i++)
        if (depz_is_known_depz_usb(all[i].vid, all[i].pid)) all[n++] = all[i];
    if (n > 1) qsort(all, n, sizeof *all, by_serial);
    *out = all;
    *count = n;
    return DEPZ_OK;
}

static int probe_requests(depz_device *dev, depz_device_info *out);

static int probe(const char *port, int timeout_ms, depz_device_info *out)
{
    depz_device *dev;
    depz_identity ident;
    int rc;

    memset(out, 0, sizeof *out);
    out->usb_vid = out->usb_pid = -1;
    out->sensor_type = DEPZ_SENSOR_NONE;
    rc = depz_device_open(port, &dev);
    if (rc == DEPZ_E_IO || rc == DEPZ_E_CLOSED) {
        /* Busy, no permission, gone: name the open failure (contract 07 §3). */
        char cause[256];
        depz_strlcpy(cause, depz_last_error(), sizeof cause);
        return depz_fail(DEPZ_E_NO_DEVICE, "no DEPZ device answered on %s: could not open the port (%s)",
                         port, cause);
    }
    if (rc) return rc;
    depz_device_set_timeout_ms(dev, timeout_ms);
    rc = probe_requests(dev, out);
    if (rc == DEPZ_OK) {
        depz_parse_software_name_n(out->software_name,
                                   depz_device_string_len(out->software_name, sizeof out->software_name),
                                   &ident);
        depz_strlcpy(out->port, port, sizeof out->port);
        depz_strlcpy(out->mode, ident.mode, sizeof out->mode);
        depz_strlcpy(out->fw_version, ident.version, sizeof out->fw_version);
        out->sensor_type = ident.sensor_type;
    } else {
        out->software_name[0] = '\0';
        rc = depz_fail(DEPZ_E_NO_DEVICE, "no DEPZ device answered on %s", port);
    }
    depz_device_close(dev);
    return rc;
}

int depz_probe_port(const char *port, int timeout_ms, depz_device_info *out)
{
    int rc;
    if (!port || !out) return depz_fail(DEPZ_E_ARG, "probe_port: NULL argument");
    rc = probe(port, timeout_ms > 0 ? timeout_ms : DEPZ_DEFAULT_TIMEOUT_MS, out);
    if (rc == DEPZ_E_IO || rc == DEPZ_E_CLOSED || rc == DEPZ_E_TIMEOUT)
        rc = depz_fail(DEPZ_E_NO_DEVICE, "no DEPZ device answered on %s", port);
    return rc;
}

int depz_list_depz_devices(bool match_usb, int timeout_ms, depz_device_info **out, size_t *count)
{
    depz_port_info *ports;
    depz_device_info *found;
    size_t i, n = 0, total;
    int rc;

    if (!out || !count) return depz_fail(DEPZ_E_ARG, "list_depz_devices: NULL argument");
    *out = NULL;
    *count = 0;
    rc = depz_enumerate_ports(&ports, &total);
    if (rc) return rc;
    if (match_usb) {
        size_t k = 0;
        for (i = 0; i < total; i++)
            if (depz_is_known_depz_usb(ports[i].vid, ports[i].pid)) ports[k++] = ports[i];
        total = k;
    }
    if (total > 1) qsort(ports, total, sizeof *ports, by_serial);
    found = (depz_device_info *)calloc(total ? total : 1, sizeof *found);
    if (!found) { free(ports); return depz_fail(DEPZ_E_NOMEM, "list_depz_devices: out of memory"); }
    for (i = 0; i < total; i++) {
        if (depz_probe_port(ports[i].port, timeout_ms, &found[n]) != DEPZ_OK) continue;
        found[n].usb_vid = ports[i].vid;
        found[n].usb_pid = ports[i].pid;
        depz_strlcpy(found[n].usb_serial, ports[i].usb_serial, sizeof found[n].usb_serial);
        n++;
    }
    free(ports);
    *out = found;
    *count = n;
    return DEPZ_OK;
}

/* Continue the open device `dev` as `name` in place (contract 02 §4.4). */
int depz_attach_sensor_class(depz_device *dev, const char *name)
{
    static const struct { const char *name; depz_vl53l8_model model; } mz[] = {
        {"vl53l8cx", DEPZ_VL53L8_MODEL_L8CX}, {"vl53l8ch", DEPZ_VL53L8_MODEL_L8CH},
        {"vl53l5cx", DEPZ_VL53L8_MODEL_L5CX}, {"vl53l7cx", DEPZ_VL53L8_MODEL_L7CX},
        {"vl53l7ch", DEPZ_VL53L8_MODEL_L7CH},
    };
    size_t i;
    bool plain, idle;
    if (!dev || !name) return depz_fail(DEPZ_E_ARG, "attach_sensor_class: NULL argument");
    if (!strcmp(name, "bootloader"))
        return depz_fail(DEPZ_E_BOOTLOADER, "%s: device is in bootloader mode; use the bootloader client (contract 06)",
                         dev->port);
    for (i = 0; i < DEPZ_SENSOR_CLASS_COUNT; i++)
        if (!strcmp(name, depz_sensor_class_name(i))) break;
    if (i == DEPZ_SENSOR_CLASS_COUNT) return depz_fail(DEPZ_E_ARG, "attach_sensor_class: no sensor class %s", name);
    depz_mutex_lock(&dev->lock);
    plain = dev->ops == NULL;
    idle = dev->pending == NULL && !dev->closing;
    depz_mutex_unlock(&dev->lock);
    if (!plain) return depz_fail(DEPZ_E_ARG, "attach_sensor_class: %s already has a sensor class", dev->port);
    if (!idle) return depz_fail(DEPZ_E_ARG, "attach_sensor_class: a request in flight, or the device is closing");
    if (!strcmp(name, "device")) return DEPZ_OK;
    if (!strcmp(name, "sr04")) return depz_sr04_attach(dev);
    if (!strcmp(name, "vl53l4cd")) return depz_vl53l4cd_attach(dev);
    if (!strcmp(name, "bno086")) return depz_bno086_attach(dev);
    if (!strcmp(name, "bno055")) return depz_bno055_attach(dev);
    if (!strcmp(name, "tmf8829")) return depz_tmf8829_attach(dev);
    for (i = 0; i < sizeof mz / sizeof mz[0]; i++)
        if (!strcmp(name, mz[i].name)) return depz_vl53l8_attach(dev, mz[i].model);
    /* The 1D family: the class name names the product ("vl53lx" none). */
    return depz_vl53lx_attach(dev, name, NULL);
}

/* Resolve the class from probe answers and attach it; a base device keeps
 * the parsed firmware type (DEPZ_SENSOR_UNKNOWN for an unknown APP_*). */
/* The names are stripped device strings in NUL-filled buffers (embedded
 * NULs kept, contract 02 §4.1): their lengths come from the buffers. */
static int attach_resolved_n(depz_device *dev, const char *software_name, size_t sw_cap,
                             const char *device_name, size_t dn_cap, int usb_vid, int usb_pid)
{
    size_t sw_len = depz_device_string_len(software_name, sw_cap);
    const char *cls = depz_resolve_sensor_class_n(software_name, sw_len, device_name,
                                                  depz_device_string_len(device_name, dn_cap),
                                                  usb_vid >= 0 && usb_vid <= 0xFFFF ? &usb_vid : NULL,
                                                  usb_pid >= 0 && usb_pid <= 0xFFFF ? &usb_pid : NULL);
    int rc = depz_attach_sensor_class(dev, cls);
    if (rc == DEPZ_OK && !strcmp(cls, "device")) {
        depz_identity ident;
        depz_parse_software_name_n(software_name, sw_len, &ident);
        depz_mutex_lock(&dev->lock);
        dev->sensor_type = ident.sensor_type;
        depz_mutex_unlock(&dev->lock);
    }
    return rc;
}
#define attach_resolved(dev, sw, dn, vid, pid) attach_resolved_n(dev, sw, sizeof(sw), dn, sizeof(dn), vid, pid)

int depz_device_promote(depz_device *dev)
{
    return depz_device_promote_ex(dev, -1, -1, NULL);
}

/* The pre-§4.4 in-place probe (contract 15 `probe: "legacy"`):
 * GET_NAME_ACTIVE_SOFTWARE, plus GET_DEVICE_NAME for the L5/L7 and 1D
 * families only, then the class attached in place. */
int depz_device_promote_ex(depz_device *dev, int usb_vid, int usb_pid, depz_identity *out)
{
    char name[64], dn[128] = "";
    depz_identity ident;
    int rc;
    if (!dev) return depz_fail(DEPZ_E_ARG, "promote: NULL device");
    rc = depz_device_get_software_name(dev, name, sizeof name);
    if (rc) return rc;
    depz_parse_software_name_n(name, depz_device_string_len(name, sizeof name), &ident);
    if (out) *out = ident;
    if (ident.sensor_type == DEPZ_SENSOR_VL53L7 || ident.sensor_type == DEPZ_SENSOR_VL53LX) {
        if (depz_device_get_device_name(dev, dn, sizeof dn) != DEPZ_OK) dn[0] = '\0';
    }
    return attach_resolved(dev, name, dn, usb_vid, usb_pid);
}

/* The contract 02 §4.4 probe on an open device: GET_NAME_ACTIVE_SOFTWARE
 * (required: its failure is returned), then GET_DEVICE_NAME and GET_SERIAL
 * (optional: a failure leaves ""). */
static int probe_requests(depz_device *dev, depz_device_info *out)
{
    int rc = depz_device_get_software_name(dev, out->software_name, sizeof out->software_name);
    if (rc) return rc;
    if (depz_device_get_device_name(dev, out->device_name, sizeof out->device_name))
        out->device_name[0] = '\0';
    if (depz_device_get_serial_number(dev, out->serial_number, sizeof out->serial_number))
        out->serial_number[0] = '\0';
    return DEPZ_OK;
}

int depz_open_on_link(depz_link *link, int usb_vid, int usb_pid, int timeout_ms, depz_device **out)
{
    depz_device_info info;
    depz_device *dev;
    int rc;
    if (!out) {
        depz_link_free(link);
        return depz_fail(DEPZ_E_ARG, "open_on_link: NULL output");
    }
    *out = NULL;
    if (!link) return depz_fail(DEPZ_E_ARG, "open_on_link: NULL link");
    rc = depz_device_open_link(link, &dev); /* takes the link, also on failure */
    if (rc) return rc;
    depz_device_set_timeout_ms(dev, timeout_ms > 0 ? timeout_ms : DEPZ_DEFAULT_TIMEOUT_MS);
    memset(&info, 0, sizeof info);
    rc = probe_requests(dev, &info);
    if (rc == DEPZ_E_IO || rc == DEPZ_E_CLOSED || rc == DEPZ_E_TIMEOUT || rc == DEPZ_E_STATUS ||
        rc == DEPZ_E_BUSY || rc == DEPZ_E_PROTOCOL)
        rc = depz_fail(DEPZ_E_NO_DEVICE, "no DEPZ device answered on %s", dev->port);
    if (rc == DEPZ_OK) rc = attach_resolved(dev, info.software_name, info.device_name, usb_vid, usb_pid);
    if (rc) { depz_device_close(dev); return rc; }
    *out = dev;
    return DEPZ_OK;
}

/* The USB VID / PID of the enumerated port `port` names under any alias
 * (contract 02 §4.3, ERRATA A5): the same string, else the same resolved
 * path or device node (depz_same_port). -1 when the OS does not list it. */
static void port_ids(const char *port, int *vid, int *pid)
{
    depz_port_info *all;
    size_t n, i, hit = (size_t)-1;
    *vid = *pid = -1;
    if (depz_enumerate_ports(&all, &n) != DEPZ_OK) return;
    for (i = 0; i < n && hit == (size_t)-1; i++)
        if (strcmp(all[i].port, port) == 0) hit = i;
    for (i = 0; i < n && hit == (size_t)-1; i++)
        if (depz_same_port(port, all[i].port)) hit = i;
    if (hit != (size_t)-1) {
        *vid = all[hit].vid;
        *pid = all[hit].pid;
    }
    free(all);
}

static int open_probed(const char *port, int timeout_ms, depz_device **out)
{
    depz_device_info info;
    depz_device *dev;
    int rc = depz_probe_port(port, timeout_ms, &info);
    if (rc) return rc;
    if (!strcmp(info.mode, "bootloader"))
        return depz_fail(DEPZ_E_BOOTLOADER, "%s: device is in bootloader mode", port);
    rc = depz_device_open(port, &dev);
    if (rc) return rc;
    if (timeout_ms > 0) depz_device_set_timeout_ms(dev, timeout_ms);
    {
        /* The probe's connection is closed; the class runs on a fresh one. */
        int vid, pid;
        port_ids(port, &vid, &pid);
        rc = attach_resolved(dev, info.software_name, info.device_name, vid, pid);
    }
    if (rc) { depz_device_close(dev); return rc; }
    *out = dev;
    return DEPZ_OK;
}

int depz_open_device(const depz_open_options *opt, depz_device **out)
{
    depz_open_options o = DEPZ_OPEN_OPTIONS_INIT;
    depz_port_info *cands;
    size_t n, i, pick = (size_t)-1, seen = 0;
    char port[256];
    int rc;

    if (!out) return depz_fail(DEPZ_E_ARG, "open_device: NULL output");
    *out = NULL;
    if (opt) o = *opt;
    if (o.port) return open_probed(o.port, o.timeout_ms, out);

    rc = depz_candidates(&cands, &n);
    if (rc) return rc;
    for (i = 0; i < n; i++) {
        if (o.serial && strcmp(cands[i].usb_serial, o.serial) != 0) continue;
        if ((o.index < 0 && seen == 0) || (o.index >= 0 && seen == (size_t)o.index)) {
            pick = i;
            break;
        }
        seen++;
    }
    if (pick == (size_t)-1) {
        size_t total = n;
        free(cands);
        if (o.serial) return depz_fail(DEPZ_E_NO_DEVICE, "no DEPZ device with USB serial '%s'", o.serial);
        if (o.index >= 0)
            return depz_fail(DEPZ_E_NO_DEVICE, "no DEPZ candidate at index %d (%zu candidate(s) found)",
                             o.index, total);
        return depz_fail(DEPZ_E_NO_DEVICE,
                         "no DEPZ device found (no serial port has a known DEPZ USB id); "
                         "pass a port explicitly to open an unprogrammed unit");
    }
    depz_strlcpy(port, cands[pick].port, sizeof port);
    free(cands);
    return open_probed(port, o.timeout_ms, out);
}
