/* ports_macos.c — serial ports (/dev/cu.*) and their USB identity via IOKit. */
#if defined(__APPLE__)

#include "io_internal.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <stdlib.h>
#include <string.h>

static CFTypeRef parent_prop(io_object_t svc, const char *key)
{
    CFStringRef k = CFStringCreateWithCString(kCFAllocatorDefault, key, kCFStringEncodingUTF8);
    CFTypeRef v = IORegistryEntrySearchCFProperty(
        svc, kIOServicePlane, k, kCFAllocatorDefault,
        kIORegistryIterateRecursively | kIORegistryIterateParents);
    CFRelease(k);
    return v;
}

static int number_prop(io_object_t svc, const char *key)
{
    CFTypeRef v = parent_prop(svc, key);
    int n = -1;
    if (v) {
        if (CFGetTypeID(v) == CFNumberGetTypeID()) CFNumberGetValue((CFNumberRef)v, kCFNumberIntType, &n);
        CFRelease(v);
    }
    return n;
}

static void string_prop(io_object_t svc, const char *key, char *out, size_t cap)
{
    CFTypeRef v = parent_prop(svc, key);
    out[0] = '\0';
    if (v) {
        if (CFGetTypeID(v) == CFStringGetTypeID())
            CFStringGetCString((CFStringRef)v, out, (CFIndex)cap, kCFStringEncodingUTF8);
        CFRelease(v);
    }
}

int depz_enumerate_ports(depz_port_info **out, size_t *count)
{
    CFMutableDictionaryRef match;
    io_iterator_t it;
    io_object_t svc;
    depz_port_info *list = NULL;
    size_t n = 0, cap = 0;

    *out = NULL;
    *count = 0;
    match = IOServiceMatching(kIOSerialBSDServiceValue);
    if (!match) return depz_fail(DEPZ_E_IO, "ports: IOServiceMatching failed");
    /* Takes ownership of `match`. kIOMainPortDefault is 0 on every version. */
    if (IOServiceGetMatchingServices(0, match, &it) != KERN_SUCCESS)
        return depz_fail(DEPZ_E_IO, "ports: IOServiceGetMatchingServices failed");
    while ((svc = IOIteratorNext(it)) != 0) {
        depz_port_info pi;
        CFTypeRef path = IORegistryEntryCreateCFProperty(svc, CFSTR(kIOCalloutDeviceKey),
                                                         kCFAllocatorDefault, 0);
        memset(&pi, 0, sizeof pi);
        pi.vid = pi.pid = -1;
        if (path && CFGetTypeID(path) == CFStringGetTypeID() &&
            CFStringGetCString((CFStringRef)path, pi.port, sizeof pi.port, kCFStringEncodingUTF8)) {
            pi.vid = number_prop(svc, "idVendor");
            pi.pid = number_prop(svc, "idProduct");
            string_prop(svc, "USB Serial Number", pi.usb_serial, sizeof pi.usb_serial);
            if (n == cap) {
                size_t nc = cap ? cap * 2 : 16;
                depz_port_info *nl = (depz_port_info *)realloc(list, nc * sizeof *nl);
                if (!nl) {
                    CFRelease(path);
                    IOObjectRelease(svc);
                    IOObjectRelease(it);
                    free(list);
                    return depz_fail(DEPZ_E_NOMEM, "ports: out of memory");
                }
                list = nl;
                cap = nc;
            }
            list[n++] = pi;
        }
        if (path) CFRelease(path);
        IOObjectRelease(svc);
    }
    IOObjectRelease(it);
    *out = list;
    *count = n;
    return DEPZ_OK;
}

#endif /* __APPLE__ */
