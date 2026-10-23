package ai.depz.sensor.sensors.vl53lx;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

/** VL53L4CD ULD 2.2.3 ({@code VL53L4CD_api.c} + {@code VL53L4CD_calibration.c}) for VL53L4CD and VL53L4CX alike. */
public final class Vl53l4UldDriver extends Vl53l1Die {
    private static final byte[] DEFAULT_CONFIGURATION = bytes(
        0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08, 0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,
        0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21,
        0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,
        0x00, 0x01, 0xcc, 0x07, 0x01, 0xf1, 0x05, 0x00, 0xa0, 0x00, 0x80, 0x08, 0x38, 0x00, 0x00, 0x00,
        0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00,
        0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00);

    private static final List<String> GROUPS = List.of(
        "timing", "offset", "xtalk", "thresholds", "signal_thresh", "sigma_thresh", "temp_update", "calib_offset", "calib_xtalk");

    public Vl53l4UldDriver(Vl53lxPlatform platform, String product) {
        super(platform, product);
    }

    static byte[] bytes(int... v) {
        byte[] b = new byte[v.length];
        for (int i = 0; i < v.length; i++) {
            b[i] = (byte) v[i];
        }
        return b;
    }

    @Override
    protected byte[] configuration() {
        return DEFAULT_CONFIGURATION;
    }

    @Override
    public Collection<String> supports() {
        return GROUPS;
    }

    // ALGO__RANGE_IGNORE_THRESHOLD_MCPS, the one write VL53L4CD_SensorInit() adds at the end.
    @Override
    protected void initExtra() {
        p.writeWord(0x0024, 0x0500);
    }

    @Override
    public int calibrateOffset(int targetDistMm, Integer nbSamples) {
        int n = nbSamples != null ? nbSamples : 20;
        if (n < 5 || n > 255 || targetDistMm < 10 || targetDistMm > 1000) {
            throw new Vl53lxException("nb_samples must be 5..255, target 10..1000 mm");
        }
        p.writeWord(RANGE_OFFSET_MM, 0);
        p.writeWord(INNER_OFFSET_MM, 0);
        p.writeWord(OUTER_OFFSET_MM, 0);
        collect(10, (i, r) -> { }); // device heat loop
        long[] sum = {0};
        collect(n, (i, r) -> sum[0] += r.distanceMm());
        int offsetMm = targetDistMm - (int) Math.floorDiv(sum[0], n);
        p.writeWord(RANGE_OFFSET_MM, (offsetMm * 4) & 0xFFFF);
        return offsetMm;
    }

    @Override
    public int calibrateXtalk(int targetDistMm, Integer nbSamples) {
        int n = nbSamples != null ? nbSamples : 20;
        if (n < 5 || n > 255 || targetDistMm < 10 || targetDistMm > 5000) {
            throw new Vl53lxException("nb_samples must be 5..255, target 10..5000 mm");
        }
        p.writeWord(XTALK_PLANE_OFFSET_KCPS, 0); // compensation off
        collect(10, (i, r) -> { });
        List<Vl53lxRange> samples = new ArrayList<>();
        collect(n, (i, r) -> {
            if (r.status() == 0 && i > 0) { // invalid measurements and the first frame are dropped
                samples.add(r);
            }
        });
        if (samples.isEmpty()) {
            throw new Vl53lxException("xtalk calibration failed: no valid samples");
        }
        double count = samples.size();
        double avgDistance = samples.stream().mapToDouble(s -> s.distanceMm()).sum() / count;
        double avgSpadNb = samples.stream().mapToDouble(Vl53lxRange::spads).sum() / count;
        double avgSignal = samples.stream().mapToDouble(Vl53lxRange::signalKcps).sum() / count;
        double tmp = (1.0 - avgDistance / targetDistMm) * (avgSignal / avgSpadNb);
        if (tmp > 127) { // 65536 / 512
            throw new Vl53lxException(String.format(java.util.Locale.ROOT, "xtalk calibration failed: %.1f kcps > 127", tmp));
        }
        p.writeWord(XTALK_PLANE_OFFSET_KCPS, (int) (tmp * 512.0) & 0xFFFF);
        return (int) roundHalfAway(tmp);
    }
}
