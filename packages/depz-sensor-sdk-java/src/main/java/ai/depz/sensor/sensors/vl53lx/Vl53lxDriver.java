package ai.depz.sensor.sensors.vl53lx;

import ai.depz.sensor.sensors.vl53l4.RangeTiming;
import java.util.Collection;
import java.util.List;
import java.util.Map;

/**
 * The contract every driver of the 1D family implements ({@code uld/base.py}). The properties say what
 * the product is as far as the bridge is concerned and which optional groups it serves
 * ({@link #supports()}): mode, timing, offset, xtalk, calib_offset, calib_xtalk, thresholds,
 * signal_thresh, sigma_thresh, roi, temp_update, refspad. A group a driver lacks throws
 * {@link UnsupportedOperationException}.
 */
public abstract class Vl53lxDriver {
    /** The distance-window interrupt: {@code window} is one of {@code Vl53l1Die.WINDOW_*}. */
    public record Thresholds(int distanceLowMm, int distanceHighMm, int window) {}

    protected final Vl53lxPlatform p;
    private final String product;

    protected Vl53lxDriver(Vl53lxPlatform platform, String product) {
        if (platform == null) {
            throw new IllegalArgumentException("platform must not be null");
        }
        this.p = platform;
        this.product = product;
    }

    /** The product this driver was bound for (one driver serves several). */
    public String product() {
        return product;
    }

    /** Bytes of register address on the wire. */
    public int addrWidth() {
        return 2;
    }

    /** The interrupt-release writes handed to START_STREAM, {@code {addr, value}} each. */
    public int[][] clearSteps() {
        return new int[0][];
    }

    /** The bus ceiling; the driver runs the bus there itself. */
    public int maxKhz() {
        return 400;
    }

    public Collection<String> supports() {
        return List.of();
    }

    /** The named ranging modes, the first being what init leaves; empty when there are none. */
    public List<String> modes() {
        return List.of();
    }

    /** The {min, max} budget a UI should offer, ms; see {@link #budgetChoices()}. */
    public int[] budgetMs() {
        return new int[] {10, 200};
    }

    /** The frame is a raw histogram (the histogram driver). */
    public boolean histogram() {
        return false;
    }

    public abstract int modelId();

    /** Reset and configure the sensor; leaves the bus at {@link #maxKhz()}. Safe to re-run. */
    public abstract void sensorInit();

    public abstract void startRanging();

    public abstract void stopRanging();

    public abstract boolean checkForDataReady();

    public void waitDataReady(long timeoutMs) {
        long deadline = System.nanoTime() + timeoutMs * 1_000_000L;
        while (!checkForDataReady()) {
            if (System.nanoTime() > deadline) {
                throw new Vl53lxException("timeout waiting for data ready");
            }
            p.sleepMs(1);
        }
    }

    public abstract void clearInterrupt();

    /** {addr, len} — the register block the bridge streams on each INT. */
    public abstract int[] streamBlock();

    /** Streamed block bytes → a range. */
    public abstract Vl53lxRange decode(byte[] raw);

    /** One polled result: the stream block read and decoded. */
    public Vl53lxRange readMeasurement() {
        int[] b = streamBlock();
        return decode(p.readMulti(b[0], b[1]));
    }

    /** Budget of one measurement and the period between two; a period of 0 means continuous. */
    public abstract void setRangeTiming(int timingBudgetMs, int interMeasurementMs);

    public abstract RangeTiming getRangeTiming();

    /** The only budgets accepted right now (ascending), or empty when any integer of {@link #budgetMs()} will do. */
    public List<Integer> budgetChoices() {
        return List.of();
    }

    public void setMode(String name) {
        throw unsupported("mode");
    }

    public String getMode() {
        throw unsupported("mode");
    }

    /** How far the loaded configuration can measure, mm, or null when not characterised. */
    public Integer reachMm() {
        return null;
    }

    /** Numbers this driver shows that the contract has no field for. */
    public Map<String, Object> driverInfo() {
        return Map.of();
    }

    // ── optional groups ──────────────────────────────────────────────────────

    public int getOffset() {
        throw unsupported("offset");
    }

    public void setOffset(int offsetMm) {
        throw unsupported("offset");
    }

    public int getXtalk() {
        throw unsupported("xtalk");
    }

    public void setXtalk(int xtalkKcps) {
        throw unsupported("xtalk");
    }

    /** {@code nbSamples} null: the driver's default. */
    public int calibrateOffset(int targetDistMm, Integer nbSamples) {
        throw unsupported("calib_offset");
    }

    public int calibrateXtalk(int targetDistMm, Integer nbSamples) {
        throw unsupported("calib_xtalk");
    }

    public Thresholds getDetectionThresholds() {
        throw unsupported("thresholds");
    }

    public void setDetectionThresholds(int distanceLowMm, int distanceHighMm, int window) {
        throw unsupported("thresholds");
    }

    public int getSignalThreshold() {
        throw unsupported("signal_thresh");
    }

    public void setSignalThreshold(int signalKcps) {
        throw unsupported("signal_thresh");
    }

    public int getSigmaThreshold() {
        throw unsupported("sigma_thresh");
    }

    public void setSigmaThreshold(int sigmaMm) {
        throw unsupported("sigma_thresh");
    }

    /** {x, y} SPADs. */
    public int[] getRoi() {
        throw unsupported("roi");
    }

    public void setRoi(int x, int y) {
        throw unsupported("roi");
    }

    public int getRoiCenter() {
        throw unsupported("roi");
    }

    public void setRoiCenter(int centerSpad) {
        throw unsupported("roi");
    }

    public void startTemperatureUpdate() {
        throw unsupported("temp_update");
    }

    public Object performRefSpadManagement() {
        throw unsupported("refspad");
    }

    // ── histogram driver ─────────────────────────────────────────────────────

    /** The raw histogram of a frame (read from the sensor when {@code raw} is null). */
    public Object binData(byte[] raw) {
        throw new UnsupportedOperationException(getClass().getSimpleName() + " has no histogram");
    }

    /** A histogram frame → a range (the host-side ranging of the histogram driver). */
    public Vl53lxRange toMeasurement(Object bins) {
        throw new UnsupportedOperationException(getClass().getSimpleName() + " has no histogram");
    }

    protected UnsupportedOperationException unsupported(String group) {
        return new UnsupportedOperationException(product + "/" + getClass().getSimpleName() + " does not support '" + group + "'");
    }
}
