/* sr04_minimal.c — SR04 ultrasonic: stream live distance.
 * Run: sr04_minimal [port] [seconds]   (no port = the first DEPZ board) */
#include "depz_sensor_io.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    depz_open_options opt = DEPZ_OPEN_OPTIONS_INIT;
    depz_device *dev;
    depz_stream *s;
    depz_sr04_measurement m;
    double mm;
    int seconds = argc > 2 ? atoi(argv[2]) : 5, n = 0;
    uint64_t end;

    if (argc > 1) opt.port = argv[1];
    if (depz_open_device(&opt, &dev) != DEPZ_OK) {
        fprintf(stderr, "open: %s\n", depz_last_error());
        return 1;
    }
    if (!depz_is_sr04(dev)) {
        fprintf(stderr, "%s is not an SR04\n", depz_device_port(dev));
        depz_device_close(dev);
        return 1;
    }
    depz_sr04_set_sample_period_us(dev, 50000); /* 20 Hz */
    s = depz_sr04_stream(dev, 64);
    depz_sr04_start(dev);
    end = depz_host_now_us() + (uint64_t)seconds * 1000000u;
    while (depz_host_now_us() < end && depz_stream_next(s, &m, 500) == DEPZ_OK) {
        if (depz_sr04_measurement_distance_mm(&m, &mm)) printf("%7.1f mm\n", mm);
        else printf("   no echo\n");
        n++;
    }
    depz_sr04_stop(dev);
    printf("%d measurements, %llu dropped\n", n, (unsigned long long)depz_stream_dropped_count(s));
    depz_stream_close(s);
    depz_device_close(dev);
    return 0;
}
