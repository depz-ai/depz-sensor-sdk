package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;

import ai.depz.sensor.dataset.DatasetRecords;
import ai.depz.sensor.devices.Bno055Sample;
import ai.depz.sensor.devices.DeviceEvent;
import ai.depz.sensor.devices.NoDepzDeviceException;
import ai.depz.sensor.devices.Sr04Measurement;
import ai.depz.sensor.devices.Vl53l4Measurement;
import ai.depz.sensor.devices.Vl53lxMeasurement;
import ai.depz.sensor.json.JsonText;
import ai.depz.sensor.protocol.Sr04;
import ai.depz.sensor.sensors.bno055.Bno055Regs;
import ai.depz.sensor.sensors.tmf8829.Frames;
import ai.depz.sensor.sensors.tmf8829.MeasurementBuilder;
import ai.depz.sensor.sensors.tmf8829.Tmf8829Firmware;
import ai.depz.sensor.sensors.vl53l8.Vl53l8CnhSetup;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Frame;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Motion;
import ai.depz.sensor.sensors.vl53lx.Vl53lxProducts;
import ai.depz.sensor.sensors.vl53lx.Vl53lxRange;
import ai.depz.sensor.sensors.vl53lx.Vl53lxTarget;
import java.util.ArrayList;
import java.util.Base64;
import java.util.List;
import java.util.Map;

/**
 * The contract 09 record values ({@link DatasetRecords}) from each device class's result — key for key
 * and type for type what the Python {@code SessionRecorder._hook} writes (integers as {@code Long},
 * reals as {@code Double}, arrays as lists, optional fields absent rather than null).
 */
public final class DatasetRecordsTests {
    private static String json(Map<String, Object> v) {
        return JsonText.serialize(v);
    }

    private static List<Object> longs(long... v) {
        List<Object> out = new ArrayList<>();
        for (long x : v) {
            out.add(x);
        }
        return out;
    }

    private static List<Object> doubles(double... v) {
        List<Object> out = new ArrayList<>();
        for (double x : v) {
            out.add(x);
        }
        return out;
    }

    public static void sr04IsEchoAndSource() {
        eq("{\"echo_us\":5831,\"source\":\"loop\"}", json(DatasetRecords.sr04(new Sr04Measurement(1, 5831, "loop"))));
    }

    public static void vl53l4KeepsTheDistanceWhateverTheStatus() {
        Map<String, Object> v = DatasetRecords.vl53l4(new Vl53l4Measurement(1, 4, 624, 2, 5160, 1096, 24, 5, 211, 7));
        eq(List.of("range_status", "distance_mm", "sigma_mm", "signal_rate_kcps", "ambient_rate_kcps", "number_of_spad", "stream_count"),
                new ArrayList<>(v.keySet()));
        eq(longs(4, 624, 2, 5160, 1096, 211, 7), new ArrayList<>(v.values()));
    }

    static Vl53l8Frame frame(int resolution, Vl53l8Motion motion) {
        int[] dist = new int[64];
        int[] status = new int[64];
        int[] nb = new int[64];
        double[] signal = new double[64];
        double[] ambient = new double[64];
        long[] spads = new long[64];
        double[] sigma = new double[64];
        double[] refl = new double[64];
        for (int i = 0; i < 64; i++) {
            dist[i] = 100 + i;
            status[i] = i == 1 ? 255 : 5;
            nb[i] = i == 1 ? 0 : 1;
            signal[i] = i / 2048.0;
            ambient[i] = 3.0;
            spads[i] = 1000L + i;
            sigma[i] = 2.0;
            refl[i] = 20.5;
        }
        return new Vl53l8Frame(123, resolution, -10, dist, status, nb, signal, ambient, spads, sigma, refl, null, motion);
    }

    public static void vl53l8CutsThePerZoneArraysToTheResolutionAndAddsLayersOnRequest() {
        Map<String, Object> v = DatasetRecords.vl53l8(frame(16, null));
        eq(List.of("resolution", "silicon_temp_degc", "distance_mm", "target_status", "nb_target_detected"), new ArrayList<>(v.keySet()));
        eq(16L, v.get("resolution"));
        eq(-10L, v.get("silicon_temp_degc"));
        eq(16, ((List<?>) v.get("distance_mm")).size());
        eq(List.of(100L, 101L), ((List<?>) v.get("distance_mm")).subList(0, 2));
        eq(List.of(5L, 255L), ((List<?>) v.get("target_status")).subList(0, 2));
        eq(List.of(1L, 0L), ((List<?>) v.get("nb_target_detected")).subList(0, 2));
        eq(json(v), json(DatasetRecords.vl53l8(frame(16, null), false)));

        Map<String, Object> layered = DatasetRecords.vl53l8(frame(64, null), true);
        eq(List.of("resolution", "silicon_temp_degc", "distance_mm", "target_status", "nb_target_detected", "signal_per_spad",
                "ambient_per_spad", "range_sigma_mm", "reflectance"), new ArrayList<>(layered.keySet()));
        eq(64, ((List<?>) layered.get("signal_per_spad")).size());
        eq(doubles(0.0, 1 / 2048.0), ((List<?>) layered.get("signal_per_spad")).subList(0, 2));
        eq(3.0, ((List<?>) layered.get("ambient_per_spad")).get(0));
        eq(2.0, ((List<?>) layered.get("range_sigma_mm")).get(0));
        eq(20.5, ((List<?>) layered.get("reflectance")).get(0));
    }

    public static void vl53l8CarriesTheMotionBlockWhenTheIndicatorIsConfigured() {
        long[] motion = new long[32];
        motion[0] = 7;
        motion[31] = 9;
        Vl53l8Motion mo = new Vl53l8Motion(0xFFFFFFFFL, 2, 1, 3, 16, motion);
        Map<String, Object> v = DatasetRecords.vl53l8(frame(16, mo));
        @SuppressWarnings("unchecked")
        Map<String, Object> m = (Map<String, Object>) v.get("motion");
        eq(List.of("global_indicator_1", "global_indicator_2", "status", "nb_of_detected_aggregates", "nb_of_aggregates", "motion"),
                new ArrayList<>(m.keySet()));
        eq(0xFFFFFFFFL, m.get("global_indicator_1"));
        eq(2L, m.get("global_indicator_2"));
        eq(1L, m.get("status"));
        eq(3L, m.get("nb_of_detected_aggregates"));
        eq(16L, m.get("nb_of_aggregates"));
        eq(32, ((List<?>) m.get("motion")).size());
        eq(List.of(7L, 9L), List.of(((List<?>) m.get("motion")).get(0), ((List<?>) m.get("motion")).get(31)));
    }

    public static void vl53l8CnhWritesTheArmedDecodeParametersWithTheRawBlock() {
        Vl53l8CnhSetup setup = new Vl53l8CnhSetup();
        setup.initConfig(10, 24, 1);
        setup.createAggMap(64, 0, 0, 2, 2, 4, 4);
        byte[] raw = {1, 2, 3, (byte) 0xFF};
        Map<String, Object> v = DatasetRecords.vl53l8Cnh(raw, setup);
        eq(List.of("nb_aggregates", "feature_length", "sum_span", "start_bin", "cnh_raw_b64"), new ArrayList<>(v.keySet()));
        eq((long) setup.nbOfAggregates(), v.get("nb_aggregates"));
        eq(16L, v.get("nb_aggregates"));
        eq((long) setup.featureLength(), v.get("feature_length"));
        eq((long) setup.sumSpan(), v.get("sum_span"));
        eq((long) Math.floorDiv(setup.refBinOffset(), 2048), v.get("start_bin")); // Python's //
        eq(10L, v.get("start_bin"));
        eq(Base64.getEncoder().encodeToString(raw), v.get("cnh_raw_b64"));
    }

    public static void vl53lxNamesThePairAndListsTargetsOnlyWhenTheDriverReportedSeveral() {
        Map<String, Object> extra = Map.of("stream_count", 1L);
        Vl53lxMeasurement single = new Vl53lxMeasurement(1, 624, 0, "valid", 5160, 1096, 2, 211, List.of(), extra, null);
        Map<String, Object> v = DatasetRecords.vl53lx(single, "VL53L4CD", "uld");
        eq("{\"product\":\"VL53L4CD\",\"driver\":\"uld\",\"status\":0,\"distance_mm\":624,\"sigma_mm\":2.0,\"signal_kcps\":5160.0,"
                + "\"ambient_kcps\":1096.0,\"spads\":211.0}", json(v));

        List<Vl53lxTarget> targets = List.of(new Vl53lxTarget(600, 6, "", 1500.5, 10, 3, 580, 620),
                new Vl53lxTarget(1200, 4, "", 300, 10, 3, 1180, 1220));
        Vl53lxMeasurement multi = new Vl53lxMeasurement(1, 600, 6, "", 1500.5, 10, 3, 40, targets, extra, new int[24]);
        Map<String, Object> w = DatasetRecords.vl53lx(multi, "VL53L4CX", "histogram");
        check(w.containsKey("targets"), "targets listed");
        eq("[{\"distance_mm\":600,\"status\":6,\"signal_kcps\":1500.5},{\"distance_mm\":1200,\"status\":4,\"signal_kcps\":300.0}]",
                JsonText.serialize(w.get("targets")));
    }

    public static void bno055WritesUnitSelAndOnlyTheChannelsTheBlockCovered() {
        Bno055Regs.Units units = new Bno055Regs.Units(true, false, true, false, false);
        Bno055Sample full = new Bno055Sample(1, 0x08, new byte[0], units, new double[] {1, 2, 3}, new double[] {4, 5, 6},
                new double[] {7, 8, 9}, new double[] {10, 11, 12}, new double[] {1, 0, 0, 0}, new double[] {0, 0, 9.8},
                new double[] {0, 0, 9.8}, 25.0, new Bno055Regs.CalibStatus(3, 2, 1, 0));
        Map<String, Object> v = DatasetRecords.bno055(full);
        eq(List.of("unit_sel", "quaternion", "euler", "accel", "gyro", "mag", "linear_accel", "gravity", "temperature", "calib"),
                new ArrayList<>(v.keySet()));
        eq((long) units.pack(), v.get("unit_sel"));
        eq(doubles(1, 0, 0, 0), v.get("quaternion"));
        eq(doubles(10, 11, 12), v.get("euler"));
        eq(doubles(1, 2, 3), v.get("accel"));
        eq(25.0, v.get("temperature"));
        eq(longs(3, 2, 1, 0), v.get("calib"));

        Bno055Sample partial = new Bno055Sample(1, 0x1A, new byte[0], Bno055Regs.Units.DEFAULT, null, null, null,
                new double[] {10, 11, 12}, null, null, null, null, null);
        Map<String, Object> p = DatasetRecords.bno055(partial);
        eq(List.of("unit_sel", "euler"), new ArrayList<>(p.keySet()));
        eq(0L, p.get("unit_sel"));
    }

    static MeasurementBuilder.Measurement tmf(int fpMode, double[] distanceMm, int[] snr, int[] signal, int[] noise, int[] xtalk) {
        int[] shape = Frames.FP_GRID[fpMode];
        Frames.PixelResults r = new Frames.PixelResults(1, shape[0], shape[1], new int[shape[0] * shape[1]], snr, signal, noise, xtalk);
        Frames.FrameInfo info = new Frames.FrameInfo(0, 0, Frames.FID_RESULTS | fpMode, 1, 0, 7, new int[] {25, 0, 0, 0}, 0,
                new int[4], 0, 0, Frames.FRAME_VALID, Frames.FOOTER_DIST_IN_MM, Frames.FRAME_EOF, 0);
        return new MeasurementBuilder.Measurement(1, 0x01, fpMode, 7, r, distanceMm, List.of(info), List.of(), List.of(), List.of(), List.of());
    }

    public static void theResultRecordsAnswerValidityAndDerivedValues() {
        Sr04Measurement echo = new Sr04Measurement(1, 5831, "once");
        check(echo.valid(), "an echo");
        eq(Sr04.distanceMmFromEcho(5831, null), echo.distanceMm());
        eq(Sr04.distanceMmFromEcho(5831, 30.0), echo.distanceMmAt(30.0));
        Sr04Measurement none = new Sr04Measurement(1, Sr04.ECHO_TIMEOUT, "loop");
        check(!none.valid() && none.distanceMm() == null, "no echo");
        Vl53l4Measurement m = new Vl53l4Measurement(1, 0, 624, 2, 5160, 1096, 24, 5, 211, 7);
        check(m.valid(), "valid");
        eq("valid", m.statusText());
        eq("unknown (99)", new Vl53l4Measurement(1, 99, 0, 0, 0, 0, 0, 0, 0, 0).statusText());
        check(!new Vl53lxRange(600, 4, "", 0, 0, 0, 0, List.of(), Map.of()).valid(), "status 4");
        check(new Vl53lxRange(600, 0, "valid", 0, 0, 0, 0, List.of(), Map.of()).valid(), "status 0");
        check(new NoDepzDeviceException("none").getMessage().equals("none"), "message");
        DeviceEvent.LinkCrcError crc = new DeviceEvent.LinkCrcError(0x91, 7);
        eq(7, crc.seq());
        eq(3, new DeviceEvent.Trash(new byte[3]).data().length);
        eq(25.5, new DeviceEvent.Temperature(1, 25.5).celsius());
        check(Tmf8829Firmware.license().contains("MIT"), "the embedded firmware's licence");
        Vl53lxProducts.Product l4cx = Vl53lxProducts.product("VL53L4CX");
        eq(Vl53lxProducts.HISTOGRAM, l4cx.defaultBridge());
        check(T.throwsType(UnsupportedOperationException.class, () -> l4cx.bridge("uld")).getMessage()
                .contains("VL53L4CX has no 'uld' driver - it has histogram"), "missing pair");
        check(T.throwsType(UnsupportedOperationException.class, () -> Vl53lxProducts.product("VL53L9ZZ")).getMessage()
                .contains("no such product: VL53L9ZZ"), "missing product");
    }

    public static void tmf8829RoundsDistancesToATenthAndListsTheArraysPresent() {
        double[] d = new double[64];
        d[0] = 123.456;
        d[1] = 0.25;
        d[2] = 0.35;
        int[] snr = new int[64];
        snr[0] = 42;
        Map<String, Object> v = DatasetRecords.tmf8829(tmf(0, d, snr, null, null, null));
        eq(List.of("fp_mode", "rows", "cols", "peaks", "f_number", "flags", "temperature_c", "distance_mm", "snr"), new ArrayList<>(v.keySet()));
        eq(longs(0, 8, 8, 1, 7, 1, 25), List.of(v.get("fp_mode"), v.get("rows"), v.get("cols"), v.get("peaks"), v.get("f_number"),
                v.get("flags"), v.get("temperature_c")));
        List<?> dist = (List<?>) v.get("distance_mm");
        eq(64, dist.size());
        eq(doubles(123.5, 0.2, 0.3), dist.subList(0, 3)); // Python round(x, 1): the exact binary value, ties to even
        eq(42L, ((List<?>) v.get("snr")).get(0));

        int[] ones = new int[32 * 32];
        java.util.Arrays.fill(ones, 1);
        Map<String, Object> w = DatasetRecords.tmf8829(tmf(3, null, null, ones, ones, ones));
        eq(longs(3, 32, 32), List.of(w.get("fp_mode"), w.get("rows"), w.get("cols")));
        eq(List.of(), w.get("distance_mm"));
        eq(List.of(), w.get("snr"));
        eq(List.of("fp_mode", "rows", "cols", "peaks", "f_number", "flags", "temperature_c", "distance_mm", "snr", "signal", "noise", "xtalk"),
                new ArrayList<>(w.keySet()));
        eq(1024, ((List<?>) w.get("xtalk")).size());
        eq(1L, ((List<?>) w.get("signal")).get(0));
    }
}
