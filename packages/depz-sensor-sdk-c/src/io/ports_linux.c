/* ports_linux.c — serial ports and their USB identity from sysfs. */
#if defined(__linux__)

#if !defined(_DEFAULT_SOURCE)
#  define _DEFAULT_SOURCE /* realpath, DT_* */
#endif

#include "io_internal.h"

#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool read_line(const char *dir, const char *file, char *out, size_t cap)
{
    char path[PATH_MAX + 64];
    FILE *f;
    snprintf(path, sizeof path, "%s/%s", dir, file);
    f = fopen(path, "r");
    if (!f) return false;
    if (!fgets(out, (int)cap, f)) { fclose(f); return false; }
    fclose(f);
    out[strcspn(out, "\r\n")] = '\0';
    return true;
}

/* Walk up from the tty's device node to the USB device (the directory that
 * has idVendor): the interface of a ttyACM, one level more for a ttyUSB. */
static bool usb_identity(const char *devdir, depz_port_info *pi)
{
    char path[PATH_MAX], val[128];
    size_t depth;
    if (!realpath(devdir, path)) return false;
    for (depth = 0; depth < 4; depth++) {
        char *slash;
        if (read_line(path, "idVendor", val, sizeof val)) {
            pi->vid = (int)strtol(val, NULL, 16);
            if (read_line(path, "idProduct", val, sizeof val)) pi->pid = (int)strtol(val, NULL, 16);
            if (!read_line(path, "serial", pi->usb_serial, sizeof pi->usb_serial))
                pi->usb_serial[0] = '\0';
            return true;
        }
        slash = strrchr(path, '/');
        if (!slash || slash == path) break;
        *slash = '\0';
    }
    return false;
}

int depz_enumerate_ports(depz_port_info **out, size_t *count)
{
    DIR *d;
    struct dirent *e;
    depz_port_info *list = NULL;
    size_t n = 0, cap = 0;

    *out = NULL;
    *count = 0;
    d = opendir("/sys/class/tty");
    if (!d) return DEPZ_OK; /* no sysfs: nothing to list */
    while ((e = readdir(d)) != NULL) {
        char devdir[PATH_MAX], sub[PATH_MAX];
        depz_port_info pi;
        if (e->d_name[0] == '.') continue;
        snprintf(devdir, sizeof devdir, "/sys/class/tty/%s/device", e->d_name);
        if (!realpath(devdir, sub)) continue;   /* virtual tty: no device */
        /* Legacy 8250 UART slots exist on every PC whether or not a UART is
         * fitted; the kernel reports an empty slot as port type 0. */
        if (strncmp(e->d_name, "ttyS", 4) == 0) {
            char type[16];
            snprintf(devdir, sizeof devdir, "/sys/class/tty/%s", e->d_name);
            if (read_line(devdir, "type", type, sizeof type) && atoi(type) == 0) continue;
        }
        memset(&pi, 0, sizeof pi);
        pi.vid = pi.pid = -1;
        memcpy(pi.port, "/dev/", 5);
        depz_strlcpy(pi.port + 5, e->d_name, sizeof pi.port - 5);
        snprintf(devdir, sizeof devdir, "/sys/class/tty/%s/device", e->d_name);
        usb_identity(devdir, &pi); /* not USB: vid/pid stay -1 */
        if (n == cap) {
            size_t nc = cap ? cap * 2 : 16;
            depz_port_info *nl = (depz_port_info *)realloc(list, nc * sizeof *nl);
            if (!nl) { free(list); closedir(d); return depz_fail(DEPZ_E_NOMEM, "ports: out of memory"); }
            list = nl;
            cap = nc;
        }
        list[n++] = pi;
    }
    closedir(d);
    *out = list;
    *count = n;
    return DEPZ_OK;
}

#endif /* __linux__ */
