/* bno086_minimal.c — BNO085 / BNO086 IMU: stream the orientation.
 * Run: bno086_minimal [port] [seconds]   (no port = the first DEPZ board)
 * Prints heading / pitch / roll ten times a second from the 100 Hz rotation
 * vector, and the rate each output really arrived at. */
#include "depz_sensor_io.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define RAD2DEG (180.0 / 3.14159265358979323846)

/* Unit quaternion (i, j, k, real) -> heading (about z), pitch, roll, degrees. */
static void to_euler(const double q[4], double *heading, double *pitch, double *roll)
{
    double x = q[0], y = q[1], z = q[2], w = q[3];
    double sp = 2 * (w * y - z * x);
    *heading = atan2(2 * (w * z + x * y), 1 - 2 * (y * y + z * z)) * RAD2DEG;
    *pitch = (fabs(sp) >= 1 ? copysign(90.0, sp) : asin(sp) * RAD2DEG);
    *roll = atan2(2 * (w * x + y * z), 1 - 2 * (x * x + y * y)) * RAD2DEG;
}

int main(int argc, char **argv)
{
    depz_open_options opt = DEPZ_OPEN_OPTIONS_INIT;
    depz_device *dev;
    depz_stream *s;
    depz_bno_product_id pid;
    depz_bno_feature granted;
    depz_bno_report r;
    int seconds = argc > 2 ? atoi(argv[2]) : 5, n_rv = 0, n_acc = 0;
    uint64_t start, end;
    double elapsed;

    if (argc > 1) opt.port = argv[1];
    if (depz_open_device(&opt, &dev) != DEPZ_OK) {
        fprintf(stderr, "open: %s\n", depz_last_error());
        return 1;
    }
    if (!depz_is_bno086(dev)) {
        fprintf(stderr, "%s is not a BNO085 / BNO086\n", depz_device_port(dev));
        depz_device_close(dev);
        return 1;
    }
    if (depz_bno086_hardware_reset(dev, -1) || depz_bno086_product_id(dev, &pid)) {
        fprintf(stderr, "reset / product id: %s\n", depz_last_error());
        depz_device_close(dev);
        return 1;
    }
    printf("%s, SH-2 %u.%u.%u build %u\n",
           pid.sw_part_number == 10004148u ? "BNO085" : pid.sw_part_number == 10004563u ? "BNO086" : "BNO08x",
           pid.sw_version_major, pid.sw_version_minor, pid.sw_version_patch, pid.sw_build_number);
    s = depz_bno086_reports(dev, 256);
    if (depz_bno086_enable(dev, DEPZ_BNO_SENSOR_ROTATION_VECTOR, 100, &granted) ||
        depz_bno086_enable(dev, DEPZ_BNO_SENSOR_ACCELEROMETER, 50, NULL)) {
        fprintf(stderr, "enable: %s\n", depz_last_error());
        depz_stream_close(s);
        depz_device_close(dev);
        return 1;
    }
    printf("rotation vector granted every %u us\n", granted.interval_us);
    start = depz_host_now_us();
    end = start + (uint64_t)seconds * 1000000u;
    while (depz_host_now_us() < end && depz_stream_next(s, &r, 500) == DEPZ_OK) {
        if (r.sensor_id == DEPZ_BNO_SENSOR_ROTATION_VECTOR) {
            double q[4], h, p, ro, acc = 0;
            if (n_rv++ % 10) continue;
            depz_bno_report_quaternion(&r, q);
            depz_bno_report_accuracy_rad(&r, &acc);
            to_euler(q, &h, &p, &ro);
            printf("heading %+7.1f  pitch %+6.1f  roll %+7.1f deg  (accuracy %d, +-%.1f deg)\n", h, p, ro,
                   r.accuracy, acc * RAD2DEG);
        } else if (r.sensor_id == DEPZ_BNO_SENSOR_ACCELEROMETER) {
            n_acc++;
        }
    }
    elapsed = (double)(depz_host_now_us() - start) / 1e6;
    depz_bno086_disable(dev, DEPZ_BNO_SENSOR_ROTATION_VECTOR);
    depz_bno086_disable(dev, DEPZ_BNO_SENSOR_ACCELEROMETER);
    printf("rotation vector %.1f Hz, accelerometer %.1f Hz, %llu dropped\n", n_rv / elapsed, n_acc / elapsed,
           (unsigned long long)depz_stream_dropped_count(s));
    depz_stream_close(s);
    depz_device_close(dev);
    return 0;
}
