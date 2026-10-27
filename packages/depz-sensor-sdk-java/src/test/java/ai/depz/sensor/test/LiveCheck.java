package ai.depz.sensor.test;

import ai.depz.sensor.devices.Bno055Device;
import ai.depz.sensor.devices.Bno086Device;
import ai.depz.sensor.devices.DepzDevice;
import ai.depz.sensor.devices.DepzDevices;
import ai.depz.sensor.devices.DepzTimeoutException;
import ai.depz.sensor.devices.Sr04Device;
import ai.depz.sensor.devices.Subscription;
import ai.depz.sensor.devices.Tmf8829Device;
import ai.depz.sensor.devices.Vl53l4cdDevice;
import ai.depz.sensor.devices.Vl53l8cxDevice;
import ai.depz.sensor.devices.Vl53lxDevice;
import ai.depz.sensor.sensors.bno055.Bno055Regs.OprMode;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;
import java.util.function.Function;
import java.util.stream.Stream;

/**
 * Live check on real boards: every DEPZ board in /dev/serial/by-id (or the ports given) is
 * opened, set streaming the usual way and read for N seconds; per board the rate and one
 * plausibility value (the centre distance of a ToF sensor, |gravity| of a BNO055). The same
 * check runs in the Python, Rust and C# SDKs, so the numbers can be compared board by board.
 *
 * <pre>./build.sh &amp;&amp; java -cp "out:lib/*" ai.depz.sensor.test.LiveCheck [seconds] [port ...]</pre>
 */
public final class LiveCheck {
    private LiveCheck() {}

    /** Items taken in {@code secs} and the plausibility values {@code f} gave (null = none). */
    record Drained(int count, List<Double> values) {}

    static <T> Drained drain(Subscription<T> sub, double secs, Function<T, Double> f) {
        long end = System.nanoTime() + (long) (secs * 1e9);
        int n = 0;
        List<Double> vals = new ArrayList<>();
        while (System.nanoTime() < end) {
            try {
                T item = sub.take(Duration.ofMillis(200));
                n++;
                Double v = f.apply(item);
                if (v != null) {
                    vals.add(v);
                }
            } catch (DepzTimeoutException e) {
                // nothing in 200 ms: the rate shows it
            }
        }
        return new Drained(n, vals);
    }

    static Double median(List<Double> v) {
        if (v.isEmpty()) {
            return null;
        }
        List<Double> s = new ArrayList<>(v);
        s.sort(null);
        return s.get(s.size() / 2);
    }

    /** The four centre zones of an 8×8 frame. */
    static Double centre(int[] d) {
        return d.length >= 64 ? (d[27] + d[28] + d[35] + d[36]) / 4.0 : null;
    }

    static String unit(DepzDevice dev) {
        if (dev instanceof Bno055Device) {
            return "|g| m/s²";
        }
        return dev instanceof Bno086Device ? "reports" : "mm";
    }

    static Drained check(DepzDevice dev, double secs) {
        if (dev instanceof Sr04Device d) {
            Subscription<ai.depz.sensor.devices.Sr04Measurement> sub = d.measurements(256);
            d.start();
            Drained r = drain(sub, secs, m -> m.distanceMm());
            d.stop();
            return r;
        }
        if (dev instanceof Vl53l4cdDevice d) {
            d.init();
            var sub = d.measurements(256);
            d.startRanging();
            Drained r = drain(sub, secs, m -> m.rangeStatus() == 0 ? (double) m.distanceMm() : null);
            d.stopRanging();
            return r;
        }
        if (dev instanceof Vl53l8cxDevice d) {
            d.init();
            d.setResolution(64);
            d.setRangingFrequencyHz(15);
            var sub = d.frames(64);
            d.startRanging();
            Drained r = drain(sub, secs, f -> centre(f.distanceMm()));
            d.stopRanging();
            return r;
        }
        if (dev instanceof Vl53lxDevice d) {
            d.init();
            var sub = d.measurements(256);
            d.startRanging();
            Drained r = drain(sub, secs, m -> m.status() == 0 ? (double) m.distanceMm() : null);
            d.stopRanging();
            return r;
        }
        if (dev instanceof Tmf8829Device d) {
            d.init();
            var sub = d.measurements(64);
            d.start();
            Drained r = drain(sub, secs, m -> m.distanceMm() == null ? null
                    : median(Arrays.stream(m.distanceMm()).filter(x -> x > 0).boxed().toList()));
            d.stop();
            return r;
        }
        if (dev instanceof Bno055Device d) {
            d.configure(OprMode.NDOF);
            var sub = d.samples(1024);
            d.startStream(10);
            return drain(sub, secs, s -> s.gravity() == null ? null
                    : Math.sqrt(s.gravity()[0] * s.gravity()[0] + s.gravity()[1] * s.gravity()[1]
                            + s.gravity()[2] * s.gravity()[2]));
        }
        if (dev instanceof Bno086Device d) {
            var sub = d.reports(4096);
            d.enable(0x05, 100.0); // rotation vector
            d.enable(0x01, 100.0); // accelerometer
            return drain(sub, secs, r -> null);
        }
        return new Drained(0, List.of());
    }

    public static void main(String[] args) throws IOException {
        double secs = args.length > 0 ? Double.parseDouble(args[0]) : 10.0;
        List<String> ports = new ArrayList<>(Arrays.asList(args).subList(Math.min(1, args.length), args.length));
        if (ports.isEmpty()) {
            try (Stream<Path> s = Files.list(Path.of("/dev/serial/by-id"))) {
                s.map(Path::toString).sorted().forEach(ports::add);
            }
        }
        int failed = 0;
        for (String port : ports) {
            String name = Path.of(port).getFileName().toString();
            long t = System.nanoTime();
            try (DepzDevice dev = DepzDevices.openPort(port, Duration.ofMillis(500))) {
                String sw = dev.getSoftwareName();
                Drained r = check(dev, secs);
                Double med = median(r.values());
                System.out.printf(Locale.ROOT, "ok    %-70s %-22s %6.1f/s  median %s  (%.1f s)%n", name, sw,
                        r.count() / secs, med == null ? "—" : String.format(Locale.ROOT, "%.1f %s", med, unit(dev)),
                        (System.nanoTime() - t) / 1e9);
            } catch (RuntimeException e) {
                failed++;
                System.out.printf(Locale.ROOT, "FAIL  %-70s %s: %s%n", name, e.getClass().getSimpleName(), e.getMessage());
            }
        }
        System.exit(failed > 0 ? 1 : 0);
    }
}
