/* ports_win32.c — COM ports and their USB identity via SetupAPI. */
#ifdef _WIN32

#include "io_internal.h"

#include <cfgmgr32.h>
#include <setupapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* GUID_DEVINTERFACE_COMPORT, spelled out to avoid the initguid dance. */
static const GUID k_comport_guid = {
    0x86E0D1E0, 0x8089, 0x11D0, {0x9C, 0xE4, 0x08, 0x00, 0x3E, 0x30, 0x1F, 0x73}};

/* "USB\VID_1BCF&PID_EC78\TMNQ8E3PRR" -> vid, pid; the serial is the last
 * segment unless Windows made it up (then it contains '&'). */
static void parse_usb_id(const char *id, depz_port_info *pi, bool take_serial)
{
    const char *v = strstr(id, "VID_"), *p = strstr(id, "PID_"), *last;
    if (v) pi->vid = (int)strtol(v + 4, NULL, 16);
    if (p) pi->pid = (int)strtol(p + 4, NULL, 16);
    if (!take_serial) return;
    last = strrchr(id, '\\');
    if (last && !strchr(last + 1, '&')) depz_strlcpy(pi->usb_serial, last + 1, sizeof pi->usb_serial);
}

int depz_enumerate_ports(depz_port_info **out, size_t *count)
{
    HDEVINFO set;
    SP_DEVINFO_DATA info;
    DWORD i;
    depz_port_info *list = NULL;
    size_t n = 0, cap = 0;

    *out = NULL;
    *count = 0;
    set = SetupDiGetClassDevsA(&k_comport_guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (set == INVALID_HANDLE_VALUE) return depz_fail(DEPZ_E_IO, "ports: SetupDiGetClassDevs failed");
    info.cbSize = sizeof info;
    for (i = 0; SetupDiEnumDeviceInfo(set, i, &info); i++) {
        depz_port_info pi;
        char id[512];
        DWORD type, size = sizeof pi.port;
        HKEY key;

        memset(&pi, 0, sizeof pi);
        pi.vid = pi.pid = -1;
        key = SetupDiOpenDevRegKey(set, &info, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        if (key == INVALID_HANDLE_VALUE) continue;
        if (RegQueryValueExA(key, "PortName", NULL, &type, (LPBYTE)pi.port, &size) != ERROR_SUCCESS ||
            type != REG_SZ) {
            RegCloseKey(key);
            continue;
        }
        RegCloseKey(key);
        pi.port[sizeof pi.port - 1] = '\0';
        if (strncmp(pi.port, "COM", 3) != 0) continue; /* LPT and friends */

        if (SetupDiGetDeviceInstanceIdA(set, &info, id, sizeof id, NULL) && !strncmp(id, "USB\\", 4)) {
            /* A composite device's CDC function is "USB\VID..&PID..&MI_00\<made up>":
             * the iSerial lives on the parent. */
            if (strstr(id, "&MI_")) {
                DEVINST parent;
                char pid[512];
                parse_usb_id(id, &pi, false);
                if (CM_Get_Parent(&parent, info.DevInst, 0) == CR_SUCCESS &&
                    CM_Get_Device_IDA(parent, pid, sizeof pid, 0) == CR_SUCCESS)
                    parse_usb_id(pid, &pi, true);
            } else {
                parse_usb_id(id, &pi, true);
            }
        }
        if (n == cap) {
            size_t nc = cap ? cap * 2 : 16;
            depz_port_info *nl = (depz_port_info *)realloc(list, nc * sizeof *nl);
            if (!nl) {
                free(list);
                SetupDiDestroyDeviceInfoList(set);
                return depz_fail(DEPZ_E_NOMEM, "ports: out of memory");
            }
            list = nl;
            cap = nc;
        }
        list[n++] = pi;
    }
    SetupDiDestroyDeviceInfoList(set);
    *out = list;
    *count = n;
    return DEPZ_OK;
}

#endif /* _WIN32 */
