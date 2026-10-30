package ai.depz.sensor.devices;

import ai.depz.sensor.protocol.Vl53l4;
import ai.depz.sensor.sensors.vl53l4.DetectionThresholds;
import ai.depz.sensor.sensors.vl53l4.RangeTiming;
import ai.depz.sensor.sensors.vl53l4.Vl53l4Platform;
import ai.depz.sensor.sensors.vl53l4.Vl53l4Uld;
import ai.depz.sensor.sensors.vl53l4.Vl53l4cdDriver;
import ai.depz.sensor.transport.Packet;
import java.time.Duration;
import java.util.List;
import java.util.concurrent.atomic.AtomicLong;
import java.util.function.Consumer;

/**
 * VL53L4CD single-zone ToF (contract 10). The board is a thin I2C bridge; the ST ULD 2.2.3 runs
 * here ({@link #uld()}). {@link #init()} first, then configure and {@link #startRanging()}: one
 * result block per INT edge streams to {@link #measurements}. Configuration while ranging throws
 * {@link InvalidStateException} — the stream owns the register bank.
 */
public final class Vl53l4cdDevice extends DepzDevice {
    private static final Duration REG_TIMEOUT = Duration.ofSeconds(2);
    private static final int STREAM_MIN_BYTES = 12;

    private final Fanout<Vl53l4Measurement> measurements = new Fanout<>();
    private final BridgePlatform platform = new BridgePlatform();
    private final Vl53l4cdDriver uld = new Vl53l4cdDriver(platform);
    private final AtomicLong streamParseErrors = new AtomicLong();
    private volatile boolean ranging;
    private volatile boolean initialized;

    public Vl53l4cdDevice(Link link, Duration timeout) {
        super(link, timeout);
    }

    public Vl53l4cdDevice(Link link) {
        super(link);
    }

    public Vl53l4cdDevice(String port, Duration timeout) {
        super(port, timeout);
    }

    Vl53l4cdDevice(DepzDevice from) {
        super(from);
    }

    /** The ULD driver (escape hatch for raw register access). */
    public Vl53l4cdDriver uld() {
        return uld;
    }

    /** True after a successful {@link #init()}; cleared by {@link #xshut} — a power-cycled sensor holds none of it. */
    public boolean initialized() {
        return initialized;
    }

    public boolean ranging() {
        return ranging;
    }

    /** True when the sensor answers with the VL53L4CD model id (0xEBAA). */
    public boolean isAlive() {
        try {
            return uld.isAlive();
        } catch (DepzException | Vl53l4Uld.Vl53l4Error e) {
            return false;
        }
    }

    /** Default configuration block + VHV calibration (ULD sensor_init); the bus is left at {@code busKhz}. */
    public void init(int busKhz) {
        requireNotRanging();
        uld.sensorInit(busKhz);
        initialized = true;
    }

    public void init() {
        init(Vl53l4Uld.I2C_KHZ_DEFAULT);
    }

    // ── sensor power (XSHUT) and the bridge ──────────────────────────────────

    /**
     * Drive XSHUT: {@link Vl53l4#XSHUT_OFF} / {@link Vl53l4#XSHUT_ON} / {@link Vl53l4#XSHUT_RESET}.
     * Off and Reset stop a stream; {@link #init()} again after.
     */
    public void xshut(int action) {
        if (action < 0 || action > Vl53l4.XSHUT_RESET) {
            throw new IllegalArgumentException("XSHUT action must be 0 (off), 1 (on) or 2 (reset), got " + action);
        }
        requestOk(Vl53l4.Vl53l4Cmd.XSHUT.value, Vl53l4.packXshut(action), action == Vl53l4.XSHUT_RESET ? REG_TIMEOUT : null);
        ranging = false;
        initialized = false;
    }

    /** Hardware reset through XSHUT (≤ 20 ms on the MCU, typ. ~2 ms); call {@link #init()} again. */
    public void resetSensor() {
        xshut(Vl53l4.XSHUT_RESET);
    }

    /** RPT_VL53_INFO: sensor identity, pin levels and bridge counters ({@code i2cErrors} is cleared by a successful XSHUT reset). Safe while streaming. */
    public Vl53l4.Vl53l4Info bridgeInfo() {
        return request(Vl53l4.Vl53l4Cmd.GET_INFO.value, null, expectReport(Vl53l4.Vl53l4Rpt.INFO.value, Vl53l4.Vl53l4Info::unpack));
    }

    /** Re-time the bridge's I2C bus to the step nearest {@code khz}. Not while ranging. */
    public void setI2cSpeedKhz(int khz) {
        requireNotRanging();
        platform.setI2cSpeed(khz);
    }

    // ── configuration (init first; not while ranging) ───────────────────────

    /** (timing budget ms, inter-measurement ms; 0 = continuous). */
    public RangeTiming getRangeTiming() {
        return uld.getRangeTiming();
    }

    /** Timing budget 10–200 ms; inter-measurement 0 = continuous, larger than the budget = autonomous low power. */
    public void setRangeTiming(int timingBudgetMs, long interMeasurementMs) {
        requireNotRanging();
        uld.setRangeTiming(timingBudgetMs, interMeasurementMs);
    }

    public void setRangeTiming(int timingBudgetMs) {
        setRangeTiming(timingBudgetMs, 0);
    }

    public int getOffsetMm() {
        return uld.getOffset();
    }

    public void setOffsetMm(int offsetMm) {
        requireNotRanging();
        uld.setOffset(offsetMm);
    }

    /** Crosstalk compensation, kcps (0 = off). */
    public int getXtalkKcps() {
        return uld.getXtalk();
    }

    public void setXtalkKcps(int xtalkKcps) {
        requireNotRanging();
        uld.setXtalk(xtalkKcps);
    }

    public DetectionThresholds getDetectionThresholds() {
        return uld.getDetectionThresholds();
    }

    /** The distance-window interrupt: INT only fires while the window holds. */
    public void setDetectionThresholds(int distanceLowMm, int distanceHighMm, int window) {
        requireNotRanging();
        uld.setDetectionThresholds(distanceLowMm, distanceHighMm, window);
    }

    public int getSignalThresholdKcps() {
        return uld.getSignalThreshold();
    }

    /** Drop measurements whose return signal is below {@code signalKcps}. */
    public void setSignalThresholdKcps(int signalKcps) {
        requireNotRanging();
        uld.setSignalThreshold(signalKcps);
    }

    public int getSigmaThresholdMm() {
        return uld.getSigmaThreshold();
    }

    /** Drop measurements whose sigma exceeds {@code sigmaMm} (≤ 16383). */
    public void setSigmaThresholdMm(int sigmaMm) {
        requireNotRanging();
        uld.setSigmaThreshold(sigmaMm);
    }

    /** Re-run VHV calibration, recommended after a >8 °C ambient change. */
    public void startTemperatureUpdate() {
        requireNotRanging();
        uld.startTemperatureUpdate();
    }

    /** Offset calibration against a target at {@code targetDistMm} (10–1000 mm); returns the offset now programmed. */
    public int calibrateOffset(int targetDistMm, int nbSamples) {
        requireNotRanging();
        return uld.calibrateOffset(targetDistMm, nbSamples);
    }

    public int calibrateOffset(int targetDistMm) {
        return calibrateOffset(targetDistMm, 20);
    }

    /** Crosstalk calibration against a target at {@code targetDistMm} (10–5000 mm); returns the xtalk now programmed, kcps. */
    public int calibrateXtalk(int targetDistMm, int nbSamples) {
        requireNotRanging();
        return uld.calibrateXtalk(targetDistMm, nbSamples);
    }

    public int calibrateXtalk(int targetDistMm) {
        return calibrateXtalk(targetDistMm, 20);
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    /** Start the sensor and arm the MCU stream: one RPT_VL53_STREAM per INT edge. */
    public void startRanging() {
        requireNotRanging();
        uld.startRanging();
        try {
            requestOk(Vl53l4.Vl53l4Cmd.START_STREAM.value,
                    Vl53l4.packStartStream(Vl53l4Uld.RESULT_BLOCK_ADDR, Vl53l4Uld.RESULT_BLOCK_LEN, 0));
        } catch (RuntimeException e) {
            // The sensor ranges but the host will say it does not: stop it again, or stopRanging() could
            // never reach it. The START_STREAM error is the one the caller gets, even if this stop fails too.
            try {
                uld.stopRanging();
            } catch (RuntimeException stopFailure) {
                e.addSuppressed(stopFailure);
            }
            throw e;
        }
        ranging = true;
    }

    public void stopRanging() {
        if (!ranging) {
            return;
        }
        // Leave the host state "not ranging" and stop the sensor even when the STOP_STREAM ack
        // fails, or no reconfiguration would ever be possible again.
        try {
            requestOk(Vl53l4.Vl53l4Cmd.STOP_STREAM.value);
        } finally {
            ranging = false;
            uld.stopRanging();
        }
    }

    /** One polled measurement: start, wait for data ready, read, stop. Not while streaming. */
    public Vl53l4Measurement measureOnce(Duration timeout) {
        requireNotRanging();
        int timeoutMs = (int) (timeout == null ? 1000 : timeout.toMillis());
        uld.startRanging();
        Vl53l4Uld.Results result;
        try {
            uld.waitDataReady(timeoutMs);
            result = uld.getResult();
            uld.clearInterrupt();
        } finally {
            uld.stopRanging();
        }
        return Vl53l4Measurement.from(platform.lastTimestampUs, result);
    }

    public Vl53l4Measurement measureOnce() {
        return measureOnce(null);
    }

    /** Call {@code callback} for every streamed measurement (reader thread). */
    public AutoCloseable onMeasurement(Consumer<Vl53l4Measurement> callback) {
        return measurements.onItem(callback);
    }

    /** A pull queue of streamed measurements (bounded, drop-oldest). */
    public Subscription<Vl53l4Measurement> measurements(int capacity) {
        return measurements.subscribe(capacity);
    }

    public Subscription<Vl53l4Measurement> measurements() {
        return measurements(64);
    }

    /** The next streamed measurement (contract 07 §3). */
    public Vl53l4Measurement getMeasurement(Duration timeout) {
        return measurements.next(timeout == null ? Duration.ofSeconds(2) : timeout, Vl53l4.Vl53l4Rpt.STREAM.value, this::isClosed);
    }

    /** Stream reports dropped because their result block did not decode. */
    public long streamParseErrors() {
        return streamParseErrors.get();
    }

    public List<Long> streamDroppedCounts() {
        return measurements.droppedCounts();
    }

    private void requireNotRanging() {
        if (ranging) {
            throw new InvalidStateException("stopRanging() first — the stream owns the register bank");
        }
    }

    @Override
    protected boolean handleReport(Packet packet) {
        if (streamParseErrors == null || packet.cmd() != Vl53l4.Vl53l4Rpt.STREAM.value || packet.payload().length < STREAM_MIN_BYTES) {
            return false;
        }
        Vl53l4Measurement m;
        try {
            Vl53l4.StreamData sample = Vl53l4.StreamData.unpack(packet.payload()); // shorter than its length field: refused
            m = Vl53l4Measurement.from(sample.timestampUs(), Vl53l4Uld.parseResultBlock(sample.data()));
        } catch (IllegalArgumentException | Vl53l4Uld.Vl53l4Error e) {
            // A stream re-armed on another block, or a corrupt read: count it, keep the reader.
            streamParseErrors.incrementAndGet();
            return true;
        }
        measurements.emit(m, stats());
        return true;
    }

    @Override
    protected void onReaderStopped() {
        if (measurements != null) {
            measurements.closeAll();
        }
        super.onReaderStopped();
    }

    // The ULD platform on the firmware register bridge.
    private final class BridgePlatform implements Vl53l4Platform {
        /** MCU timestamp of the latest register read. */
        volatile long lastTimestampUs;

        @Override
        public byte[] readMulti(int addr, int size) {
            byte[] output = new byte[size];
            int done = 0;
            while (done < size) {
                int n = Math.min(size - done, Vl53l4.XFER_MAX);
                Vl53l4.RegData rep = request(Vl53l4.Vl53l4Cmd.READ_REG.value, Vl53l4.packReadReg(addr, n),
                        expectReport(Vl53l4.Vl53l4Rpt.REG_DATA.value, Vl53l4.RegData::unpack), REG_TIMEOUT);
                if (rep.data().length != n) {
                    throw new DepzException(String.format("READ_REG 0x%04X: expected %d, got %d", addr, n, rep.data().length));
                }
                lastTimestampUs = rep.timestampUs();
                System.arraycopy(rep.data(), 0, output, done, n);
                addr = (addr + n) & 0xFFFF;
                done += n;
            }
            return output;
        }

        @Override
        public void writeMulti(int addr, byte[] data) {
            int done = 0;
            while (done < data.length) {
                int n = Math.min(data.length - done, Vl53l4.XFER_MAX);
                byte[] chunk = java.util.Arrays.copyOfRange(data, done, done + n);
                requestOk(Vl53l4.Vl53l4Cmd.WRITE_REG.value, Vl53l4.packWriteReg(addr, chunk), REG_TIMEOUT);
                addr = (addr + n) & 0xFFFF;
                done += n;
            }
        }

        @Override
        public void setI2cSpeed(int khz) {
            if (khz < 0 || khz > 0xFFFF) {
                throw new IllegalArgumentException("bus speed must fit u16 kHz, got " + khz);
            }
            requestOk(Vl53l4.Vl53l4Cmd.SET_I2C_SPEED.value, Vl53l4.packSetI2cSpeed(khz));
        }

        @Override
        public void sleepMs(int ms) {
            try {
                Thread.sleep(ms);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
    }
}
