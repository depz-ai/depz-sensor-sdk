/* vl53lx_minimal.c — a VL53L 1D-family ToF board (VL53L0X / L1CX / L1CB /
 * L3CX / L4CX): stream the distance.
 * Run: vl53lx_minimal [port] [seconds]   (no port = the first DEPZ board)
 * Binds the board's own product and its default driver, configures a 50 ms
 * budget, prints every frame (all targets on the histogram driver) and the
 * rate the frames really arrived at. */
#include "depz_sensor_io.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    depz_open_options opt = DEPZ_OPEN_OPTIONS_INIT;
    depz_device *dev;
    depz_stream *s;
    depz_vl53lx_measurement m;
    int seconds = argc > 2 ? atoi(argv[2]) : 5, n = 0, budget = 0, inter = 0;
    uint64_t start;
    double elapsed;
    size_t k;

    if (argc > 1) opt.port = argv[1];
    if (depz_open_device(&opt, &dev) != DEPZ_OK) {
        fprintf(stderr, "open: %s\n", depz_last_error());
        return 1;
    }
    if (!depz_is_vl53lx(dev)) {
        fprintf(stderr, "%s is not a VL53L 1D-family board\n", depz_device_port(dev));
        depz_device_close(dev);
        return 1;
    }
    /* NONE / 0: the product the board's name carries, its first driver. */
    if (depz_vl53lx_init(dev, 0, DEPZ_VL53LX_PRODUCT_NONE) != DEPZ_OK ||
        depz_vl53lx_configure(dev, 50, 0, NULL, NULL, NULL) != DEPZ_OK) {
        fprintf(stderr, "init: %s\n", depz_last_error());
        depz_device_close(dev);
        return 1;
    }
    depz_vl53lx_get_range_timing(dev, &budget, &inter);
    printf("%s / %s, budget %d ms\n", depz_vl53lx_product_get(depz_vl53lx_product_bound(dev))->name,
           depz_vl53lx_driver_str(depz_vl53lx_driver_bound(dev)), budget);
    s = depz_vl53lx_measurements(dev, 256);
    if (!s || depz_vl53lx_start_ranging(dev) != DEPZ_OK) {
        fprintf(stderr, "start: %s\n", depz_last_error());
        depz_stream_close(s);
        depz_device_close(dev);
        return 1;
    }
    start = depz_host_now_us();
    while (depz_host_now_us() - start < (uint64_t)seconds * 1000000u && depz_stream_next(s, &m, 1000) == DEPZ_OK) {
        n++;
        if (m.n_targets) {
            for (k = 0; k < m.n_targets; k++)
                printf("%s%5d mm (%d)", k ? "  " : "", (int)m.targets[k].distance_mm, m.targets[k].status);
            printf("\n");
        } else if (depz_vl53lx_plottable(&m)) {
            printf("%5d mm\n", (int)m.distance_mm);
        } else {
            printf("  status %d: %s\n", m.status, m.status_text);
        }
    }
    elapsed = (double)(depz_host_now_us() - start) / 1e6;
    depz_vl53lx_stop_ranging(dev);
    printf("%d frames, %.1f Hz, %llu dropped\n", n, n / elapsed, (unsigned long long)depz_stream_dropped_count(s));
    depz_stream_close(s);
    depz_device_close(dev);
    return 0;
}
