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

/* USB serial ascending, unknown serials last, then port path. */
static int by_serial(const void *a, const void *b)
{
    const depz_port_info *x = (const depz_port_info *)a, *y = (const depz_port_info *)b;
    int ex = !x->usb_serial[0], ey = !y->usb_serial[0], c;
    if (ex != ey) return ex - ey;
    c = strcmp(x->usb_serial, y->usb_serial);
    return c ? c : strcmp(x->port, y->port);
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
    qsort(all, n, sizeof *all, by_serial);
    *out = all;
    *count = n;
    return DEPZ_OK;
}

static int probe(const char *port, int timeout_ms, depz_device_info *out)
{
    depz_device *dev;
    depz_identity ident;
    int rc;

    memset(out, 0, sizeof *out);
    out->usb_vid = out->usb_pid = -1;
    out->sensor_type = DEPZ_SENSOR_NONE;
    rc = depz_device_open(port, &dev);
    if (rc) return rc;
    depz_device_set_timeout_ms(dev, timeout_ms);
    rc = depz_device_get_software_name(dev, out->software_name, sizeof out->software_name);
    if (rc == DEPZ_OK) {
        depz_parse_software_name(out->software_name, &ident);
        depz_strlcpy(out->port, port, sizeof out->port);
        depz_strlcpy(out->mode, ident.mode, sizeof out->mode);
        depz_strlcpy(out->fw_version, ident.version, sizeof out->fw_version);
        out->sensor_type = ident.sensor_type;
        if (depz_device_get_device_name(dev, out->device_name, sizeof out->device_name))
            out->device_name[0] = '\0';
        if (depz_device_get_serial_number(dev, out->serial_number, sizeof out->serial_number))
            out->serial_number[0] = '\0';
    } else {
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
    qsort(ports, total, sizeof *ports, by_serial);
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

/* Attach the sensor class for `type` to an open device. `usb_pid` (-1 when
 * unknown) picks between classes one firmware serves: VL53L8CX and VL53L8CH
 * share APP_VL53L8 and their silicon; only the production PID tells them
 * apart (no PID -> the CX base, safe since CH is a superset). The L5/L7
 * boards (APP_VL53L7) resolve by PID, then the device name (contract 11 §1). */
static int attach_type(depz_device *dev, depz_sensor_type type, int usb_vid, int usb_pid,
                       const char *device_name)
{
    if (type == DEPZ_SENSOR_VL53L7) {
        static const depz_vl53l8_model by_l7[3] = {DEPZ_VL53L8_MODEL_L7CX, DEPZ_VL53L8_MODEL_L5CX,
                                                   DEPZ_VL53L8_MODEL_L7CH};
        depz_vl53l7_model m = depz_vl53l7_resolve_model(depz_usb_model_hint(usb_vid, usb_pid), device_name);
        return depz_vl53l8_attach(dev, by_l7[m]);
    }
    if (type == DEPZ_SENSOR_SR04) return depz_sr04_attach(dev);
    if (type == DEPZ_SENSOR_VL53L4) return depz_vl53l4cd_attach(dev);
    if (type == DEPZ_SENSOR_BNO055) return depz_bno055_attach(dev);
    if (type == DEPZ_SENSOR_BNO086) return depz_bno086_attach(dev);
    if (type == DEPZ_SENSOR_VL53LX)
        return depz_vl53lx_attach(dev, depz_usb_model_hint(usb_vid, usb_pid), device_name);
    if (type == DEPZ_SENSOR_VL53L8)
        return depz_vl53l8_attach(dev, usb_pid == (int)DEPZ_PID_VL53L8 ? DEPZ_VL53L8_MODEL_L8CH
                                                                      : DEPZ_VL53L8_MODEL_L8CX);
    /* No class yet: the base device, with its type known. */
    depz_mutex_lock(&dev->lock);
    dev->sensor_type = type;
    depz_mutex_unlock(&dev->lock);
    return DEPZ_OK;
}

int depz_device_promote(depz_device *dev)
{
    char name[64];
    depz_identity ident;
    int rc;
    if (!dev) return depz_fail(DEPZ_E_ARG, "promote: NULL device");
    rc = depz_device_get_software_name(dev, name, sizeof name);
    if (rc) return rc;
    depz_parse_software_name(name, &ident);
    if (!strcmp(ident.mode, "bootloader"))
        return depz_fail(DEPZ_E_BOOTLOADER, "%s: device is in bootloader mode", dev->port);
    if (ident.sensor_type == DEPZ_SENSOR_VL53L7 || ident.sensor_type == DEPZ_SENSOR_VL53LX) {
        /* Over a bare link the device name is all there is (as the Python SDK). */
        char dn[128];
        if (depz_device_get_device_name(dev, dn, sizeof dn) != DEPZ_OK) dn[0] = '\0';
        return attach_type(dev, ident.sensor_type, -1, -1, dn);
    }
    return attach_type(dev, ident.sensor_type, -1, -1, NULL);
}

/* The USB VID / PID of `port`, -1 when the OS does not know them. */
static void port_ids(const char *port, int *vid, int *pid)
{
    depz_port_info *all;
    size_t n, i;
    *vid = *pid = -1;
    if (depz_enumerate_ports(&all, &n) != DEPZ_OK) return;
    for (i = 0; i < n; i++)
        if (strcmp(all[i].port, port) == 0) {
            *vid = all[i].vid;
            *pid = all[i].pid;
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
        int vid, pid;
        port_ids(port, &vid, &pid);
        rc = attach_type(dev, info.sensor_type, vid, pid, info.device_name);
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
