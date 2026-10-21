package ai.depz.sensor.sensors.tmf8829;

import java.util.ArrayList;
import java.util.List;

import ai.depz.sensor.sensors.tmf8829.CaptureTimer.FrameTime;
import ai.depz.sensor.sensors.tmf8829.Frames.FrameError;
import ai.depz.sensor.sensors.tmf8829.Frames.FrameInfo;
import ai.depz.sensor.sensors.tmf8829.Frames.HistogramFrame;
import ai.depz.sensor.sensors.tmf8829.Frames.PixelResults;

/**
 * Frames in stream order → measurements (contracts/14_SENSOR_TMF8829.md §6).
 *
 * <p>Histogram frames between two results frames belong to the second; a
 * ref-SPAD frame comes right before its results frame (each half-frame has its
 * own). 32x32/48x32: the first half (layout 0x40 clear) holds the even rows; a
 * second half without a first is dropped, two first halves in a row keep the
 * newer one (both logged as problems). Call {@link #reset()} at the start of
 * every stream: the clock model and T1 are per stream. Mirrors
 * {@code depz_sensor_sdk/tmf8829/builder.py}.
 */
public final class MeasurementBuilder {

    /**
     * One measurement: its results frame (8x8, 16x16) or both half-frames
     * (32x32, 48x32) merged, with the histogram / ref-SPAD frames before it.
     * {@code timestampUs} is the capture time, MCU µs (floor mean of the
     * half-frame captures); {@code flags} the OR of their FT_*.
     * {@code distanceMm} is {@code [peaks][rows][cols]} flat, clock-corrected,
     * in mm ({@code null} when the format has no peaks); {@code results} keeps
     * the raw sensor integers. {@code refFrames} are 65 B each (zero
     * pre-header). {@code problems} lists what went wrong since the previous
     * measurement.
     */
    public record Measurement(
            long timestampUs, int flags, int fpMode, long fNumber,
            PixelResults results, double[] distanceMm,
            List<FrameInfo> frames, List<FrameTime> times,
            List<HistogramFrame> histograms, List<byte[]> refFrames, List<String> problems) {

        /** {rows, cols} of the full focal plane. */
        public int[] shape() {
            return Frames.FP_GRID[fpMode].clone();
        }

        public int peaks() {
            return results.peaks();
        }

        /** Sensor die temperature (header byte 0), °C. */
        public int temperatureC() {
            return frames.get(frames.size() - 1).temperature()[0];
        }
    }

    /** Raw distances → mm, clock-corrected (×ratio; ÷4 for 0.25 mm units). */
    public static double[] distanceMm(PixelResults results, double ratio, boolean inMm) {
        if (results.distance() == null) {
            return null;
        }
        int[] raw = results.distance();
        double[] d = new double[raw.length];
        for (int i = 0; i < raw.length; i++) {
            double v = (double) raw[i] * ratio;
            d[i] = inMm ? v : v / 4.0;
        }
        return d;
    }

    private record Pending(FrameInfo info, byte[] raw) {}

    private record Half(FrameInfo info, PixelResults res, FrameTime time) {}

    public final SensorClock clock = new SensorClock();
    public final CaptureTimer timer = new CaptureTimer(clock);
    /** Frames that failed to parse since the last {@link #reset()}. */
    public int parseErrors;

    private List<Pending> pending;
    private List<HistogramFrame> hist;
    private List<byte[]> refs;
    private Half half;
    private List<String> problems;

    public MeasurementBuilder() {
        reset();
    }

    public void reset() {
        clock.reset();
        timer.reset();
        pending = new ArrayList<>();
        hist = new ArrayList<>();
        refs = new ArrayList<>();
        half = null;
        problems = new ArrayList<>();
        parseErrors = 0;
    }

    /**
     * One frame with the MCU time of its header read and its INT edge (0 =
     * none) → the measurement it completes, or {@code null}.
     */
    public Measurement add(byte[] raw, long timestampUs, long edgeUs) {
        FrameInfo f;
        try {
            f = Frames.parseFrame(raw);
        } catch (FrameError e) {
            parseErrors += 1;
            problems.add(e.getMessage());
            return null;
        }
        if (f.kind() == Frames.FID_REF_SPAD) {
            pending.add(new Pending(f, raw)); // zero pre-header: no clock pair
            return null;
        }
        if (f.kind() != Frames.FID_RESULTS && f.kind() != Frames.FID_HISTOGRAMS) {
            parseErrors += 1;
            problems.add(String.format("frame id 0x%02X: unknown kind", f.id()));
            return null;
        }
        int flags = clock.add(timestampUs, f.systick());
        List<String> bad = f.problems();
        if (!bad.isEmpty()) {
            problems.add(String.format("frame id=0x%02X fN=%d %d B: %s",
                f.id(), f.fNumber(), f.size(), String.join(", ", bad)));
        }
        if (f.kind() == Frames.FID_HISTOGRAMS) {
            timer.histograms();
            pending.add(new Pending(f, raw));
            return null;
        }

        FrameTime t = timer.resultsFrame(f, timestampUs, edgeUs, flags);
        for (Pending p : pending) {
            if (p.info().kind() == Frames.FID_HISTOGRAMS) {
                hist.add(new HistogramFrame(p.info(), p.raw()));
            } else {
                refs.add(p.raw());
            }
        }
        pending = new ArrayList<>();
        PixelResults res;
        try {
            res = Frames.parseResults(raw, f);
        } catch (FrameError e) {
            parseErrors += 1;
            problems.add("fN=" + f.fNumber() + ": " + e.getMessage());
            half = null;
            return null;
        }

        List<FrameInfo> frames;
        List<FrameTime> times;
        if (Frames.FP_GRID[f.fpMode()][0] == 32) {
            if (half == null) {
                if (f.isSecondHalf()) {
                    problems.add("second half fN=" + f.fNumber() + " without a first one - dropped");
                    hist = new ArrayList<>();
                    refs = new ArrayList<>();
                    return null;
                }
                half = new Half(f, res, t);
                return null;
            }
            if (!f.isSecondHalf()) {
                problems.add("two first halves in a row (fN " + half.info().fNumber() + ", " + f.fNumber() + ")");
                half = new Half(f, res, t);
                return null;
            }
            Half h0 = half;
            half = null;
            if (h0.info().fpMode() != f.fpMode()
                    || (h0.info().layout() & ~Frames.LAYOUT_SUBFRAME) != (f.layout() & ~Frames.LAYOUT_SUBFRAME)) {
                // Impossible within one stream (configure() needs a stop); never throw on a malformed one.
                problems.add("half-frames of different formats (fN " + h0.info().fNumber() + ", " + f.fNumber()
                        + ") - dropped");
                hist = new ArrayList<>();
                refs = new ArrayList<>();
                return null;
            }
            frames = List.of(h0.info(), f);
            times = List.of(h0.time(), t);
            res = Frames.mergeHalves(h0.res(), res);
        } else {
            frames = List.of(f);
            times = List.of(t);
        }

        long sum = 0;
        int orFlags = 0;
        for (FrameTime x : times) {
            sum += x.timestampUs();
            orFlags |= x.flags();
        }
        Measurement m = new Measurement(
            Math.floorDiv(sum, (long) times.size()), orFlags, f.fpMode(), f.fNumber(),
            res, distanceMm(res, clock.ratio(), f.distanceInMm()),
            frames, times, hist, refs, problems);
        hist = new ArrayList<>();
        refs = new ArrayList<>();
        problems = new ArrayList<>();
        return m;
    }
}
