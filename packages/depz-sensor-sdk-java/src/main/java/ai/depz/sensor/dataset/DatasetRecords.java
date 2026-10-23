package ai.depz.sensor.dataset;

import ai.depz.sensor.devices.Bno055Sample;
import ai.depz.sensor.devices.Sr04Measurement;
import ai.depz.sensor.devices.Vl53l4Measurement;
import ai.depz.sensor.devices.Vl53lxMeasurement;
import ai.depz.sensor.sensors.bno086.Reports;
import ai.depz.sensor.sensors.tmf8829.Frames;
import ai.depz.sensor.sensors.tmf8829.MeasurementBuilder;
import ai.depz.sensor.sensors.vl53l8.Vl53l8CnhSetup;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Frame;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Motion;
import ai.depz.sensor.sensors.vl53lx.Vl53lxTarget;
import java.math.BigDecimal;
import java.math.RoundingMode;
import java.util.ArrayList;
import java.util.Base64;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * The contract 09 record value ({@code v}) of each kind, from the device classes' results — what
 * {@link SessionRecorder} writes, public for writers of their own. Key for key the reference Python
 * SDK's values (integers as {@code Long}, reals as {@code Double}, arrays as lists).
 */
public final class DatasetRecords {
    private DatasetRecords() {}

    // The raw ADC streams (0x14..0x16): only these keep their integer counts.
    private static final Set<Integer> BNO086_RAW_IDS = Set.of(0x14, 0x15, 0x16);

    // Report property → record key, for the scalar fields contract 09 `bno086` keeps.
    private static final List<String> BNO086_RECORD_KEYS = List.of(
        "i", "j", "k", "real", "x", "y", "z", "accuracy_rad", "accuracy", "steps", "latency_us", "flags",
        "double_tap", "motion", "classification", "most_likely_state", "value", "intent", "request", "flushed_sensor");

    /** {@code sr04}: {@code {echo_us, source}}. */
    public static Map<String, Object> sr04(Sr04Measurement m) {
        Map<String, Object> v = new LinkedHashMap<>();
        v.put("echo_us", (long) m.echoTimeUs());
        v.put("source", m.source());
        return v;
    }

    /** {@code vl53l4}: the result fields, distance raw whatever the status. */
    public static Map<String, Object> vl53l4(Vl53l4Measurement m) {
        Map<String, Object> v = new LinkedHashMap<>();
        v.put("range_status", (long) m.rangeStatus());
        v.put("distance_mm", (long) m.distanceMm());
        v.put("sigma_mm", (long) m.sigmaMm());
        v.put("signal_rate_kcps", (long) m.signalRateKcps());
        v.put("ambient_rate_kcps", (long) m.ambientRateKcps());
        v.put("number_of_spad", (long) m.numberOfSpad());
        v.put("stream_count", (long) m.streamCount());
        return v;
    }

    /**
     * {@code vl53l8}: per-zone arrays cut to the resolution; {@code layers} adds signal and ambient
     * (kcps/SPAD), sigma (mm) and reflectance (%) as reals; {@code motion} when the indicator is configured.
     */
    public static Map<String, Object> vl53l8(Vl53l8Frame f, boolean layers) {
        int n = f.resolution();
        Map<String, Object> v = new LinkedHashMap<>();
        v.put("resolution", (long) n);
        v.put("silicon_temp_degc", (long) f.siliconTempDegc());
        v.put("distance_mm", ints(f.distanceMm(), n));
        v.put("target_status", ints(f.targetStatus(), n));
        v.put("nb_target_detected", ints(f.nbTargetDetected(), n));
        if (layers) {
            v.put("signal_per_spad", doubles(f.signalPerSpad(), n));
            v.put("ambient_per_spad", doubles(f.ambientPerSpad(), n));
            v.put("range_sigma_mm", doubles(f.rangeSigmaMm(), n));
            v.put("reflectance", doubles(f.reflectance(), n));
        }
        Vl53l8Motion mo = f.motion();
        if (mo != null) {
            Map<String, Object> m = new LinkedHashMap<>();
            m.put("global_indicator_1", mo.globalIndicator1());
            m.put("global_indicator_2", mo.globalIndicator2());
            m.put("status", (long) mo.status());
            m.put("nb_of_detected_aggregates", (long) mo.nbOfDetectedAggregates());
            m.put("nb_of_aggregates", (long) mo.nbOfAggregates());
            m.put("motion", longs(mo.motion(), mo.motion().length));
            v.put("motion", m);
        }
        return v;
    }

    public static Map<String, Object> vl53l8(Vl53l8Frame f) {
        return vl53l8(f, false);
    }

    /** {@code vl53l8_cnh}: the raw CNH block of a frame with the armed setup's decode parameters. */
    public static Map<String, Object> vl53l8Cnh(byte[] cnhRaw, Vl53l8CnhSetup setup) {
        Map<String, Object> v = new LinkedHashMap<>();
        v.put("nb_aggregates", (long) setup.nbOfAggregates());
        v.put("feature_length", (long) setup.featureLength());
        v.put("sum_span", (long) setup.sumSpan());
        v.put("start_bin", (long) Math.floor(setup.refBinOffset() / 2048.0));
        v.put("cnh_raw_b64", Base64.getEncoder().encodeToString(cnhRaw));
        return v;
    }

    /** {@code vl53lx}: the family-wide result, plus {@code targets} when the driver reported several. */
    public static Map<String, Object> vl53lx(Vl53lxMeasurement m, String product, String driver) {
        Map<String, Object> v = new LinkedHashMap<>();
        v.put("product", product);
        v.put("driver", driver);
        v.put("status", (long) m.status());
        v.put("distance_mm", (long) m.distanceMm());
        v.put("sigma_mm", m.sigmaMm());
        v.put("signal_kcps", m.signalKcps());
        v.put("ambient_kcps", m.ambientKcps());
        v.put("spads", m.spads());
        if (!m.targets().isEmpty()) {
            List<Object> targets = new ArrayList<>();
            for (Vl53lxTarget t : m.targets()) {
                Map<String, Object> tv = new LinkedHashMap<>();
                tv.put("distance_mm", (long) t.distanceMm());
                tv.put("status", (long) t.status());
                tv.put("signal_kcps", t.signalKcps());
                targets.add(tv);
            }
            v.put("targets", targets);
        }
        return v;
    }

    /** {@code bno055}: {@code unit_sel} and every channel the streamed block covered, scaled. */
    public static Map<String, Object> bno055(Bno055Sample s) {
        Map<String, Object> v = new LinkedHashMap<>();
        v.put("unit_sel", (long) s.units().pack());
        putDoubles(v, "quaternion", s.quaternion());
        putDoubles(v, "euler", s.euler());
        putDoubles(v, "accel", s.accel());
        putDoubles(v, "gyro", s.gyro());
        putDoubles(v, "mag", s.mag());
        putDoubles(v, "linear_accel", s.linearAccel());
        putDoubles(v, "gravity", s.gravity());
        if (s.temperature() != null) {
            v.put("temperature", s.temperature());
        }
        if (s.calibration() != null) {
            var c = s.calibration();
            v.put("calib", List.of((long) c.system(), (long) c.gyro(), (long) c.accel(), (long) c.mag()));
        }
        return v;
    }

    /** {@code tmf8829}: one measurement — distances in mm rounded to 0.1, the raw per-zone arrays. */
    public static Map<String, Object> tmf8829(MeasurementBuilder.Measurement m) {
        Frames.PixelResults r = m.results();
        int[] shape = m.shape();
        Map<String, Object> v = new LinkedHashMap<>();
        v.put("fp_mode", (long) m.fpMode());
        v.put("rows", (long) shape[0]);
        v.put("cols", (long) shape[1]);
        v.put("peaks", (long) r.peaks());
        v.put("f_number", m.fNumber());
        v.put("flags", (long) m.flags());
        v.put("temperature_c", (long) m.temperatureC());
        List<Object> dist = new ArrayList<>();
        if (m.distanceMm() != null) {
            for (double x : m.distanceMm()) {
                dist.add(round1(x));
            }
        }
        v.put("distance_mm", dist);
        v.put("snr", r.snr() == null ? new ArrayList<>() : ints(r.snr(), r.snr().length));
        if (r.signal() != null) {
            v.put("signal", ints(r.signal(), r.signal().length));
        }
        if (r.noise() != null) {
            v.put("noise", ints(r.noise(), r.noise().length));
        }
        if (r.xtalk() != null) {
            v.put("xtalk", ints(r.xtalk(), r.xtalk().length));
        }
        return v;
    }

    /**
     * {@code bno086}: the report in snake_case — type and sensor id, then only the fields the report
     * carries (scaled), the integer counts of the raw ADC streams, and the device timestamp.
     */
    public static Map<String, Object> bno086(Reports.Report r) {
        Map<String, Object> f = r.fields();
        Map<String, Object> scaled = Reports.scaled(r);
        int sensor = ((Number) f.get("sensor_id")).intValue();
        Map<String, Object> v = new LinkedHashMap<>();
        v.put("type", r.type());
        v.put("sensor_id", (long) sensor);
        for (String key : BNO086_RECORD_KEYS) {
            Object val = scaled.containsKey(key) ? scaled.get(key) : f.get(key);
            if (val instanceof Boolean || val instanceof Long || val instanceof Integer || val instanceof Double) {
                v.put(key, val instanceof Integer i ? (Object) (long) i : val);
            }
        }
        for (String key : List.of("bias", "angular_velocity")) {
            if (scaled.get(key) instanceof double[] a) {
                v.put(key, doubles(a, a.length));
            }
        }
        if (f.get("confidences") instanceof List<?> c) {
            v.put("confidences", new ArrayList<>(c));
        } else if (f.get("confidences") instanceof long[] c) {
            v.put("confidences", longs(c, c.length));
        } else if (f.get("confidences") instanceof int[] c) {
            v.put("confidences", ints(c, c.length));
        }
        if (r.type().equals("GenericEvent")) {
            v.put("value_raw", f.get("value_raw"));
        }
        if (BNO086_RAW_IDS.contains(sensor)) {
            for (String key : List.of("x_raw", "y_raw", "z_raw", "temperature_raw", "sensor_timestamp_us")) {
                if (f.get(key) != null) {
                    v.put(key, f.get(key));
                }
            }
        }
        if (f.get("timestamp_us") != null) {
            v.put("timestamp_us", f.get("timestamp_us"));
        }
        return v;
    }

    /**
     * Python's {@code round(x, 1)}: the nearest tenth of the exact binary value, ties to even, as the
     * double closest to it.
     */
    public static double round1(double x) {
        if (x == 0 || Double.isNaN(x) || Double.isInfinite(x)) {
            return x;
        }
        return new BigDecimal(x).setScale(1, RoundingMode.HALF_EVEN).doubleValue();
    }

    private static void putDoubles(Map<String, Object> v, String key, double[] a) {
        if (a != null) {
            v.put(key, doubles(a, a.length));
        }
    }

    private static List<Object> ints(int[] a, int n) {
        List<Object> out = new ArrayList<>(n);
        for (int i = 0; i < n; i++) {
            out.add((long) a[i]);
        }
        return out;
    }

    private static List<Object> longs(long[] a, int n) {
        List<Object> out = new ArrayList<>(n);
        for (int i = 0; i < n; i++) {
            out.add(a[i]);
        }
        return out;
    }

    private static List<Object> doubles(double[] a, int n) {
        List<Object> out = new ArrayList<>(n);
        for (int i = 0; i < n; i++) {
            out.add(a[i]);
        }
        return out;
    }
}
