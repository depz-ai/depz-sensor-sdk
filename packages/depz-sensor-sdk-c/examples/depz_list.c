/* depz_list.c — every serial port with its USB id, then the DEPZ boards found
 * by probing. Run: depz_list */
#include "depz_sensor_io.h"

#include <stdio.h>

int main(void)
{
    depz_port_info *ports;
    depz_device_info *devs;
    size_t n, i;

    if (depz_list_serial_ports(&ports, &n) != DEPZ_OK) {
        fprintf(stderr, "ports: %s\n", depz_last_error());
        return 1;
    }
    printf("%zu serial port(s):\n", n);
    for (i = 0; i < n; i++) {
        if (ports[i].vid >= 0)
            printf("  %-24s %04X:%04X  serial '%s'%s\n", ports[i].port, ports[i].vid, ports[i].pid,
                   ports[i].usb_serial,
                   depz_is_known_depz_usb(ports[i].vid, ports[i].pid) ? "  (DEPZ)" : "");
        else
            printf("  %-24s (not USB)\n", ports[i].port);
    }
    depz_free_port_list(ports);

    if (depz_list_depz_devices(true, 300, &devs, &n) != DEPZ_OK) {
        fprintf(stderr, "probe: %s\n", depz_last_error());
        return 1;
    }
    printf("%zu DEPZ device(s):\n", n);
    for (i = 0; i < n; i++)
        printf("  %s  %s  %s  fw %s  '%s'  serial %s\n", devs[i].port, devs[i].mode,
               depz_sensor_type_str(devs[i].sensor_type) ? depz_sensor_type_str(devs[i].sensor_type)
                                                         : "-",
               devs[i].fw_version, devs[i].device_name, devs[i].serial_number);
    depz_free_device_list(devs);
    return 0;
}
