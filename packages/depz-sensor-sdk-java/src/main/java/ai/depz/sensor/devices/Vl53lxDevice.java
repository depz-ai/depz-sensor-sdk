package ai.depz.sensor.devices;

import ai.depz.sensor.protocol.Vl53l4;
import ai.depz.sensor.protocol.Vl53lx;
import ai.depz.sensor.sensors.vl53l4.RangeTiming;
import ai.depz.sensor.sensors.vl53lx.Vl53lxDriver;
import ai.depz.sensor.sensors.vl53lx.Vl53lxDrivers;
import ai.depz.sensor.sensors.vl53lx.Vl53lxPlatform;
import ai.depz.sensor.sensors.vl53lx.Vl53lxProducts;
import ai.depz.sensor.sensors.vl53lx.Vl53lxProtocolException;
import ai.depz.sensor.sensors.vl53lx.Vl53lxRange;
import ai.depz.sensor.transport.Packet;
import java.time.Duration;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicLong;
import java.util.function.Consumer;
import java.util.function.Supplier;

/**
 * A board of the VL53L 1D family on the {@code APP_VL53L0_4} register bridge (contract 12): VL53L0X,
 * VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CD, VL53L4CX. Every ULD runs here; two axes pick how a board is
 * driven — the product (whose parameter set: the board's own, or a sibling's to borrow its driver) and
 * the driver kind ({@code uld}, {@code ulp}, {@code histogram}). {@link #init} binds the pair,
 * {@link #configure} re-initialises and applies a ranging configuration (before every run), then
 * {@link #startRanging()} and read {@link #measurements}. Ask {@link #supports} before an optional group;
 * a missing one throws {@link UnsupportedOperationException}. This generic class reads the product
 * from the board's name; the per-product classes fix it.
 */
public class Vl53lxDevice extends DepzDevice {
    private static final Duration REG_TIMEOUT = Duration.ofSeconds(2);
    private static final int STREAM_MIN_BYTES = 12;
    private static final int ERR_INVALID_CMD = 0x02;

    private final Fanout<Vl53lxMeasurement> measurements = new Fanout<>();
    private final AtomicLong streamParseErrors = new AtomicLong();
    private BridgePlatform platform;
    private volatile Vl53lxDriver driver;
    private String boardName;
    private String product;
    private String driverKind;
    private String caveat;
    private volatile boolean ranging;

    public Vl53lxDevice(Link link, Duration timeout) {
        super(link, timeout);
    }

    public Vl53lxDevice(Link link) {
        super(link);
    }

    public Vl53lxDevice(String port, Duration timeout) {
        super(port, timeout);
    }

    Vl53lxDevice(DepzDevice from) {
        super(from);
    }

    /** The product this class drives; null = read it from the device name. */
    protected String fixedProduct() {
        return null;
    }

    private synchronized BridgePlatform platform() {
        if (platform == null) {
            platform = new BridgePlatform();
        }
        return platform;
    }

    // ── identity ─────────────────────────────────────────────────────────────

    /** The board's device name, e.g. {@code DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523}. */
    public synchronized String boardName() {
        if (boardName == null) {
            boardName = getDeviceName();
        }
        return boardName;
    }

    /** The product the device name carries, or null on an unstamped board. */
    public String detected() {
        return Vl53lxProducts.productFromBoardName(boardName());
    }

    /** The product {@link #init} bound (null before). */
    public String product() {
        return product;
    }

    public String driverKind() {
        return driverKind;
    }

    public String caveat() {
        return caveat;
    }

    /** The ULD port in use — escape hatch for product-specific calls. */
    public Vl53lxDriver driver() {
        Vl53lxDriver d = driver;
        if (d == null) {
            throw new IllegalStateException("call init() first");
        }
        return d;
    }

    public boolean initialized() {
        return driver != null;
    }

    public boolean ranging() {
        return ranging;
    }

    /** The driver kinds a product has (null: this board's). */
    public List<String> driverKinds(String product) {
        String name = product != null ? product : fixedProduct() != null ? fixedProduct() : detected();
        return name == null ? List.of() : Vl53lxDrivers.driverKinds(name);
    }

    public List<String> driverKinds() {
        return driverKinds(null);
    }

    // ── lifecycle ────────────────────────────────────────────────────────────

    /**
     * Bind the (product, driver kind) pair and initialise the sensor. {@code product} null: this class's,
     * else the board name's; {@code driver} null: the product's first kind. A pair the table lacks throws
     * {@link UnsupportedOperationException} naming what the product has.
     */
    public void init(String driver, String product) {
        requireNotRanging();
        String name = product != null ? product : fixedProduct() != null ? fixedProduct() : detected();
        if (name == null) {
            throw new IllegalStateException("board '" + boardName() + "' carries no product number - pass product explicitly");
        }
        String kind = driver;
        if (kind == null) {
            List<String> kinds = Vl53lxDrivers.driverKinds(name);
            if (kinds.isEmpty()) {
                throw new UnsupportedOperationException(name + " has no driver this SDK runs yet");
            }
            kind = kinds.get(0);
        }
        Vl53lxDrivers.Created c = Vl53lxDrivers.createDriver(name, kind, platform());
        // Sticky on the bridge and 2 after a reset: set before the first register access.
        platform().setAddrWidth(c.driver().addrWidth());
        sensorInit(c.driver());
        this.driver = c.driver();
        this.product = name;
        this.driverKind = kind;
        this.caveat = c.caveat();
    }

    public void init(String driver) {
        init(driver, null);
    }

    public void init() {
        init(null, null);
    }

    // A reset makes the die NACK for a moment; the driver waits those out, and they are no bus fault.
    private void sensorInit(Vl53lxDriver drv) {
        drv.sensorInit();
        platform().clearI2cErrors();
    }

    /**
     * Everything about what is connected (needs {@link #init}). {@code model_id_ok} is a cross-check
     * only: L1CX/L1CB and L4CD/L4CX share their ids.
     */
    public Map<String, Object> identifyProduct() {
        Vl53lxDriver drv = driver();
        int modelId = drv.modelId();
        Vl53lxProducts.Product rated = Vl53lxProducts.TABLE.get(detected() != null ? detected() : product);
        Map<String, Object> out = new LinkedHashMap<>();
        out.put("board", boardName());
        out.put("detected", detected());
        out.put("product", product);
        out.put("driver", driverKind);
        out.put("driver_class", drv.getClass().getSimpleName());
        out.put("kinds", Vl53lxDrivers.driverKinds(product));
        out.put("model_id", modelId);
        out.put("model_id_ok", Vl53lxProducts.product(product).modelIdOk(modelId));
        out.put("supports", new ArrayList<>(drv.supports()));
        out.put("modes", drv.modes());
        out.put("reach_mm", rated != null ? rated.reachMm() : null);
        out.put("driver_reach_mm", drv.reachMm());
        out.put("budget_ms", drv.budgetMs());
        out.put("budget_choices", budgetChoices());
        out.put("histogram", drv.histogram());
        out.put("max_khz", drv.maxKhz());
        out.put("caveat", caveat);
        return out;
    }

    /**
     * The lines a UI shows about the chosen pair: a product named by hand, a borrowed driver, a pair that
     * reaches less than the board is rated for, the driver's caveat.
     */
    public List<String> notes() {
        Vl53lxDriver drv = driver();
        List<String> out = new ArrayList<>();
        String det = detected();
        if (det == null) {
            out.add(product + ": board name carries no product number, named by hand");
        } else if (!det.equals(product)) {
            out.add(product + ": borrowing this driver - the board says it is a " + det);
        }
        Vl53lxProducts.Product row = Vl53lxProducts.TABLE.get(det != null ? det : product);
        Integer reach = drv.reachMm();
        if (row != null && reach != null && reach < row.reachMm()) {
            out.add(product + "/" + driverKind + ": this pair reaches " + reach + " mm, the board is rated " + row.reachMm()
                    + " mm - past " + reach + " mm the phase is out of the valid window and frames come back with status 4");
        }
        if (caveat != null) {
            out.add(product + "/" + driverKind + ": " + caveat);
        }
        return out;
    }

    /** Whether this product / driver serves an optional group. */
    public boolean supports(String group) {
        return driver().supports().contains(group);
    }

    /** Named ranging modes, the first being what init leaves; empty if none. */
    public List<String> modes() {
        return driver().modes();
    }

    /** The {min, max} budget the driver offers, ms. */
    public int[] budgetRange() {
        return driver().budgetMs();
    }

    /** How far the loaded configuration can measure, mm, or null. */
    public Integer driverReachMm() {
        return driver().reachMm();
    }

    // ── sensor power and the bridge ──────────────────────────────────────────

    /**
     * Drive XSHUT ({@link Vl53lx#XSHUT_OFF} / {@link Vl53lx#XSHUT_ON} / {@link Vl53lx#XSHUT_RESET}). Off and
     * Reset stop the stream; the sensor then holds none of the configuration — {@link #init} again.
     */
    public void xshut(int action) {
        if (action < 0 || action > Vl53lx.XSHUT_RESET) {
            throw new IllegalArgumentException("XSHUT action must be 0 (off), 1 (on) or 2 (reset), got " + action);
        }
        requestOk(Vl53lx.Vl53lxCmd.XSHUT.value, Vl53lx.packXshut(action), Duration.ofSeconds(1));
        ranging = false;
        driver = null;
    }

    /** RPT_VL53_INFO — the bridge's own counters and settings; safe while streaming. */
    public Vl53lx.Vl53lxInfo bridgeInfo() {
        return request(Vl53lx.Vl53lxCmd.GET_INFO.value, null, expectReport(Vl53lx.Vl53lxRpt.INFO.value, Vl53lx.Vl53lxInfo::unpack));
    }

    // ── configuration (init first; not while ranging) ────────────────────────

    /**
     * Re-initialise the sensor and apply a ranging configuration. The re-init is deliberate: it is the
     * only way to know what the configuration registers hold. {@code mode} goes on before the budget;
     * {@code offsetMm} / {@code xtalkKcps} re-apply a stored calibration; {@code signalKcps} replaces the
     * blob's signal threshold (each null: leave it). {@code interMs} 0 = continuous. An unsupported group
     * is refused before the re-init.
     */
    public void configure(int budgetMs, int interMs, String mode, Integer offsetMm, Integer xtalkKcps, Integer signalKcps) {
        requireNotRanging();
        Vl53lxDriver drv = driver();
        if (mode != null) {
            need("mode");
        }
        if (offsetMm != null) {
            need("offset");
        }
        if (xtalkKcps != null) {
            need("xtalk");
        }
        if (signalKcps != null) {
            need("signal_thresh");
        }
        sensorInit(drv);
        if (mode != null) {
            drv.setMode(mode);
        }
        drv.setRangeTiming(budgetMs, interMs);
        if (offsetMm != null) {
            drv.setOffset(offsetMm);
        }
        if (xtalkKcps != null) {
            drv.setXtalk(xtalkKcps);
        }
        if (signalKcps != null) {
            drv.setSignalThreshold(signalKcps);
        }
    }

    public void configure(int budgetMs, int interMs, String mode) {
        configure(budgetMs, interMs, mode, null, null, null);
    }

    public void configure(int budgetMs, int interMs) {
        configure(budgetMs, interMs, null, null, null, null);
    }

    /** 50 ms, continuous. */
    public void configure() {
        configure(50, 0, null, null, null, null);
    }

    /** (timing budget ms, inter-measurement ms) read back from the sensor; 0 = continuous. */
    public RangeTiming getRangeTiming() {
        return driver().getRangeTiming();
    }

    /** The mode in use, or null on a product without modes. */
    public String getMode() {
        return supports("mode") ? driver().getMode() : null;
    }

    /** The only budgets accepted right now, or empty when any in {@link #budgetRange()} will do. */
    public List<Integer> budgetChoices() {
        return driver().budgetChoices();
    }

    /** The nearest budget this product accepts. */
    public int snapBudget(int budgetMs) {
        List<Integer> choices = budgetChoices();
        if (!choices.isEmpty()) {
            int best = choices.get(0);
            for (int c : choices) {
                if (Math.abs(c - budgetMs) < Math.abs(best - budgetMs)) {
                    best = c;
                }
            }
            return best;
        }
        int[] range = driver().budgetMs();
        return Math.max(range[0], Math.min(range[1], budgetMs));
    }

    public int getOffsetMm() {
        return need("offset").getOffset();
    }

    public void setOffsetMm(int offsetMm) {
        requireNotRanging();
        need("offset").setOffset(offsetMm);
    }

    public int getXtalkKcps() {
        return need("xtalk").getXtalk();
    }

    public void setXtalkKcps(int xtalkKcps) {
        requireNotRanging();
        need("xtalk").setXtalk(xtalkKcps);
    }

    /** Offset calibration against a flat target; returns the offset programmed (store it on the host). */
    public int calibrateOffset(int targetDistMm, Integer nbSamples) {
        requireNotRanging();
        return need("calib_offset").calibrateOffset(targetDistMm, nbSamples);
    }

    public int calibrateOffset(int targetDistMm) {
        return calibrateOffset(targetDistMm, null);
    }

    /** Crosstalk calibration; returns the xtalk programmed, kcps (store it on the host). */
    public int calibrateXtalk(int targetDistMm, Integer nbSamples) {
        requireNotRanging();
        return need("calib_xtalk").calibrateXtalk(targetDistMm, nbSamples);
    }

    public int calibrateXtalk(int targetDistMm) {
        return calibrateXtalk(targetDistMm, null);
    }

    public Vl53lxDriver.Thresholds getDetectionThresholds() {
        return need("thresholds").getDetectionThresholds();
    }

    /** The distance-window interrupt; stays armed until the next init / configure. */
    public void setDetectionThresholds(int distanceLowMm, int distanceHighMm, int window) {
        requireNotRanging();
        need("thresholds").setDetectionThresholds(distanceLowMm, distanceHighMm, window);
    }

    public int getSignalThresholdKcps() {
        return need("signal_thresh").getSignalThreshold();
    }

    public void setSignalThresholdKcps(int signalKcps) {
        requireNotRanging();
        need("signal_thresh").setSignalThreshold(signalKcps);
    }

    public int getSigmaThresholdMm() {
        return need("sigma_thresh").getSigmaThreshold();
    }

    public void setSigmaThresholdMm(int sigmaMm) {
        requireNotRanging();
        need("sigma_thresh").setSigmaThreshold(sigmaMm);
    }

    /** {x, y} SPADs. */
    public int[] getRoi() {
        return need("roi").getRoi();
    }

    public void setRoi(int x, int y) {
        requireNotRanging();
        need("roi").setRoi(x, y);
    }

    public int getRoiCenter() {
        return need("roi").getRoiCenter();
    }

    public void setRoiCenter(int centerSpad) {
        requireNotRanging();
        need("roi").setRoiCenter(centerSpad);
    }

    /** Re-run VHV after an ambient change over 8 °C. */
    public void startTemperatureUpdate() {
        requireNotRanging();
        need("temp_update").startTemperatureUpdate();
    }

    /** VL53L0X: re-measure the reference SPADs. */
    public Object performRefSpadManagement() {
        requireNotRanging();
        return need("refspad").performRefSpadManagement();
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    /**
     * Start the sensor and arm the board's stream: one report per INT edge, followed on the MCU by the
     * driver's interrupt-release writes.
     */
    public void startRanging() {
        requireNotRanging();
        Vl53lxDriver drv = driver();
        drv.startRanging();
        int[] block = drv.streamBlock();
        requestOk(Vl53lx.Vl53lxCmd.START_STREAM.value, Vl53lx.packStartStream(block[0], block[1], drv.clearSteps(), 0));
        ranging = true;
    }

    public void stopRanging() {
        if (!ranging) {
            return;
        }
        try {
            requestOk(Vl53lx.Vl53lxCmd.STOP_STREAM.value);
        } finally {
            ranging = false;
            driver().stopRanging();
        }
    }

    /** One polled measurement: start, data ready, read, release the interrupt, stop. Not while streaming. */
    public Vl53lxMeasurement measureOnce(Duration timeout) {
        requireNotRanging();
        Vl53lxDriver drv = driver();
        drv.startRanging();
        Vl53lxRange m;
        Object bins;
        try {
            drv.waitDataReady(timeout == null ? 1000 : timeout.toMillis());
            if (drv.histogram()) {
                bins = drv.binData(null);
                m = drv.toMeasurement(bins);
            } else {
                m = drv.readMeasurement();
                bins = null;
            }
            drv.clearInterrupt();
        } finally {
            drv.stopRanging();
        }
        return Vl53lxMeasurement.from(platform().lastTimestampUs, m, bins);
    }

    public Vl53lxMeasurement measureOnce() {
        return measureOnce(null);
    }

    /** Call {@code callback} for every streamed measurement (reader thread). */
    public AutoCloseable onMeasurement(Consumer<Vl53lxMeasurement> callback) {
        return measurements.onItem(callback);
    }

    /** A pull queue of streamed measurements (bounded, drop-oldest). */
    public Subscription<Vl53lxMeasurement> measurements(int capacity) {
        return measurements.subscribe(capacity);
    }

    public Subscription<Vl53lxMeasurement> measurements() {
        return measurements(64);
    }

    /** The next streamed measurement (contract 07 §3). */
    public Vl53lxMeasurement getMeasurement(Duration timeout) {
        return measurements.next(timeout == null ? Duration.ofSeconds(2) : timeout, Vl53lx.Vl53lxRpt.STREAM.value, this::isClosed);
    }

    /** Stream reports dropped because the driver did not decode them. */
    public long streamParseErrors() {
        return streamParseErrors.get();
    }

    public List<Long> streamDroppedCounts() {
        return measurements.droppedCounts();
    }

    // ── internal ─────────────────────────────────────────────────────────────

    private Vl53lxDriver need(String group) {
        Vl53lxDriver drv = driver();
        if (!drv.supports().contains(group)) {
            throw new UnsupportedOperationException(product + "/" + driverKind + " does not support '" + group + "'"
                    + (caveat != null ? " (" + caveat + ")" : ""));
        }
        return drv;
    }

    private void requireNotRanging() {
        if (ranging) {
            throw new IllegalStateException("stopRanging() first — the stream owns the bus");
        }
    }

    @Override
    protected boolean handleReport(Packet packet) {
        if (streamParseErrors == null || packet.cmd() != Vl53lx.Vl53lxRpt.STREAM.value || packet.payload().length < STREAM_MIN_BYTES) {
            return false;
        }
        Vl53lxDriver drv = driver;
        if (drv == null) {
            return true;
        }
        Vl53lxMeasurement m;
        try {
            Vl53l4.StreamData sample = Vl53lx.unpackStreamData(packet.payload()); // shorter than its length field: refused
            // Decoded exactly once: the histogram driver steps its own frame-pair state on every frame.
            if (drv.histogram()) {
                Object bins = drv.binData(sample.data());
                m = Vl53lxMeasurement.from(sample.timestampUs(), drv.toMeasurement(bins), bins);
            } else {
                m = Vl53lxMeasurement.from(sample.timestampUs(), drv.decode(sample.data()), null);
            }
        } catch (RuntimeException e) {
            streamParseErrors.incrementAndGet(); // a bad sample must not kill the reader
            return true;
        }
        measurements.emit(m);
        return true;
    }

    @Override
    protected void onReaderStopped() {
        if (measurements != null) {
            measurements.closeAll();
        }
        super.onReaderStopped();
    }

    // The platform the ULD ports expect, over the register bridge: transfers split at 253 bytes; a non-OK
    // status or a missing answer surfaces as Vl53lxProtocolException, which the drivers catch.
    private final class BridgePlatform extends Vl53lxPlatform {
        volatile long lastTimestampUs;

        private <T> T call(Supplier<T> fn) {
            try {
                return fn.get();
            } catch (StatusException | DepzTimeoutException e) {
                throw new Vl53lxProtocolException(e.getMessage(), e);
            }
        }

        @Override
        public byte[] readMulti(int addr, int size) {
            byte[] output = new byte[size];
            int done = 0;
            while (done < size) {
                int n = Math.min(size - done, Vl53lx.XFER_MAX);
                int a = addr + done;
                Vl53l4.RegData rep = call(() -> request(Vl53lx.Vl53lxCmd.READ_REG.value, Vl53lx.packReadReg(a, n),
                        expectReport(Vl53lx.Vl53lxRpt.REG_DATA.value, Vl53lx::unpackRegData), REG_TIMEOUT));
                if (rep.data().length != n) {
                    throw new Vl53lxProtocolException(String.format("READ_REG 0x%04X: expected %d, got %d", a, n, rep.data().length));
                }
                lastTimestampUs = rep.timestampUs();
                System.arraycopy(rep.data(), 0, output, done, n);
                done += n;
            }
            return output;
        }

        @Override
        public void writeMulti(int addr, byte[] data) {
            int done = 0;
            while (done < data.length) {
                int n = Math.min(data.length - done, Vl53lx.XFER_MAX);
                byte[] payload = Vl53lx.packWriteReg(addr + done, java.util.Arrays.copyOfRange(data, done, done + n));
                call(() -> {
                    requestOk(Vl53lx.Vl53lxCmd.WRITE_REG.value, payload, REG_TIMEOUT);
                    return null;
                });
                done += n;
            }
        }

        @Override
        public void setI2cSpeed(int khz) {
            call(() -> {
                requestOk(Vl53lx.Vl53lxCmd.SET_I2C_SPEED.value, Vl53lx.packSetI2cSpeed(khz));
                return null;
            });
        }

        @Override
        public void setAddrWidth(int width) {
            call(() -> {
                requestOk(Vl53lx.Vl53lxCmd.SET_ADDR_WIDTH.value, Vl53lx.packSetAddrWidth(width));
                return null;
            });
        }

        @Override
        public void xshutReset() {
            call(() -> {
                requestOk(Vl53lx.Vl53lxCmd.XSHUT.value, Vl53lx.packXshut(Vl53lx.XSHUT_RESET), Duration.ofSeconds(1));
                return null;
            });
        }

        /**
         * Zero the bridge's I2C error counters once an init is through (a resetting die NACKs for a
         * moment, expectedly). A v2.00 bridge (firmware older than v0.24) lacks the command.
         */
        void clearI2cErrors() {
            try {
                requestOk(Vl53lx.Vl53lxCmd.CLEAR_I2C_ERRORS.value);
            } catch (StatusException e) {
                if (e.status() == ERR_INVALID_CMD) {
                    throw new DepzException("the board's firmware predates APP_VL53L0_4_v0.24 (protocol v2.01) and "
                            + "cannot clear its I2C error counter — reflash it", e);
                }
                throw new Vl53lxProtocolException(e.getMessage(), e);
            } catch (DepzTimeoutException e) {
                throw new Vl53lxProtocolException(e.getMessage(), e);
            }
        }
    }
}
