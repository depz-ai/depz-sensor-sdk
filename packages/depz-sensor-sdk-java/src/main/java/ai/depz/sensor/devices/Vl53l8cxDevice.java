package ai.depz.sensor.devices;

import ai.depz.sensor.protocol.Vl53l8;
import ai.depz.sensor.sensors.vl53l8.FrameReassembler;
import ai.depz.sensor.sensors.vl53l8.Vl53l8CnhSetup;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Driver;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Firmware;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Frame;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Platform;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Uld;
import ai.depz.sensor.transport.Packet;
import java.time.Duration;
import java.util.List;
import java.util.concurrent.atomic.AtomicLong;
import java.util.function.BiConsumer;
import java.util.function.Consumer;

/**
 * VL53L8CX multizone ToF (contract 04). The board is a thin SPI bridge; the ST ULD runs here
 * ({@link #uld()}). {@link #init()} downloads the ~84 KB sensor firmware (about a second), then
 * configure and {@link #startRanging()}: the board pushes one frame per INT edge in chunks,
 * reassembled and parsed into {@link #frames}. Configuration needs {@link #init()} and throws
 * {@link InvalidStateException} while ranging — the stream owns the register bank. The VL53L8CH
 * superset is {@link Vl53l8chDevice}.
 */
public class Vl53l8cxDevice extends DepzDevice {
    /** Below this the sensor never enters its ranging loop and streams nothing (contract 04). */
    public static final int MIN_RANGING_FREQUENCY_HZ = 2;

    /**
     * Highest ranging frequency at {@code resolution} zones (contract 11 §3; the same silicon
     * limit on VL53L8): 60 Hz at 4×4, 15 Hz at 8×8.
     */
    public static int maxRangingFrequencyHz(int resolution) {
        return resolution == Vl53l8Driver.RESOLUTION_8X8 ? 15 : 60;
    }

    private static final Duration REG_TIMEOUT = Duration.ofSeconds(2);
    private static final int STREAM_MIN_BYTES = 12;

    private final Fanout<Vl53l8Frame> frames = new Fanout<>();
    private final Object uldLock = new Object();
    private final AtomicLong frameParseErrors = new AtomicLong();
    private volatile FrameReassembler reassembler = new FrameReassembler();
    private volatile Vl53l8Driver uld;
    private volatile BiConsumer<Integer, Integer> writeProgress;
    private volatile boolean ranging;
    private volatile int resolution = Vl53l8Driver.RESOLUTION_4X4; // the ULD default after init
    private volatile Vl53l8CnhSetup armedCnh;
    private BridgePlatform platform;

    public Vl53l8cxDevice(Link link, Duration timeout) {
        super(link, timeout);
    }

    public Vl53l8cxDevice(Link link) {
        super(link);
    }

    public Vl53l8cxDevice(String port, Duration timeout) {
        super(port, timeout);
    }

    Vl53l8cxDevice(DepzDevice from) {
        super(from);
    }

    /** The sensor firmware {@link #init()} downloads. */
    protected Vl53l8Firmware firmwareSet() {
        return Vl53l8Firmware.CX;
    }

    /** Register bytes per READ_REG (the I2C L5/L7 board reads less per call). */
    protected int readChunk() {
        return Vl53l8.CHUNK_SIZE;
    }

    /** Register bytes per WRITE_REG. */
    protected int writeChunk() {
        return Vl53l8.CHUNK_SIZE;
    }

    /** Largest RPT_VL53_FRAME chunk the board emits (contract 04 §4 rule 4). */
    protected int streamChunk() {
        return Vl53l8.STREAM_CHUNK_MAX;
    }

    /** The lowest ranging frequency that streams on this sensor. */
    protected int minRangingHz() {
        return MIN_RANGING_FREQUENCY_HZ;
    }

    private synchronized BridgePlatform platform() {
        if (platform == null) {
            platform = new BridgePlatform();
        }
        return platform;
    }

    /** The ULD driver (escape hatch for DCI access). @throws InvalidStateException before {@link #init()} */
    public Vl53l8Driver uld() {
        Vl53l8Driver d = uld;
        if (d == null) {
            throw new InvalidStateException("call init() first");
        }
        return d;
    }

    /** The driver after {@link #init()}, or null. */
    protected Vl53l8Driver uldOrNull() {
        return uld;
    }

    /** The firmware set in use after {@link #init()} (cx / ch / l7cx / l7ch). */
    public Vl53l8Firmware firmware() {
        return uld().firmware();
    }

    public boolean ranging() {
        return ranging;
    }

    /** True when the sensor answers with its own silicon ids (a bus error or a foreign id: false). */
    public boolean isAlive() {
        try {
            int[] ids = new Vl53l8Driver(platform(), firmwareSet()).isAlive();
            return Vl53l8Driver.siliconIdMatches(ids[0], ids[1],
                    firmwareSet() == Vl53l8Firmware.L7CX || firmwareSet() == Vl53l8Firmware.L7CH);
        } catch (DepzException | Vl53l8Uld.Vl53l8Error e) {
            return false;
        }
    }

    /**
     * Download the sensor firmware and the default configuration. {@code progress} gets phase
     * texts, {@code writeProgress} (done, total) the big firmware writes; either may be null.
     */
    public void init(Consumer<String> progress, BiConsumer<Integer, Integer> writeProgress) {
        requireNotRanging();
        synchronized (uldLock) {
            this.writeProgress = writeProgress;
            try {
                Vl53l8Driver driver = new Vl53l8Driver(platform(), firmwareSet());
                driver.init(progress);
                uld = driver;
                // A fresh driver has no motion indicator armed; the CNH setup is gone from the
                // sensor too (ERRATA B4).
                armedCnh = null;
            } finally {
                this.writeProgress = null;
            }
        }
    }

    public void init() {
        init(null, null);
    }

    // ── configuration (init first; not while ranging) ────────────────────────

    /** Zones in use: 16 (4×4) or 64 (8×8). */
    public int getResolution() {
        resolution = uld().getResolution();
        return resolution;
    }

    /** 16 (4×4) or 64 (8×8) zones. */
    public void setResolution(int zones) {
        if (zones != Vl53l8Driver.RESOLUTION_4X4 && zones != Vl53l8Driver.RESOLUTION_8X8) {
            throw new IllegalArgumentException("resolution is 16 (4x4) or 64 (8x8) zones, got " + zones);
        }
        requireNotRanging();
        uld().setResolution(zones);
        resolution = zones;
    }

    public int getRangingFrequencyHz() {
        return uld().getRangingFrequencyHz();
    }

    /**
     * At least 2 Hz (1 Hz on L5/L7); at most {@link #maxRangingFrequencyHz} of the resolution last
     * set or read through this class (4×4 after init) — 60 Hz at 4×4, 15 Hz at 8×8 (contract 11
     * §3). Refused host-side, nothing reaches the bus.
     */
    public void setRangingFrequencyHz(int hz) {
        if (hz < minRangingHz()) {
            throw new IllegalArgumentException("ranging frequency must be >= " + minRangingHz()
                    + " Hz: below that the sensor never enters its ranging loop and streams nothing (contract 04), got " + hz);
        }
        int maxHz = maxRangingFrequencyHz(resolution);
        if (hz > maxHz) {
            throw new IllegalArgumentException("ranging frequency must be <= " + maxHz + " Hz at " + resolution
                    + " zones (60 Hz at 4x4, 15 Hz at 8x8, contract 11 §3), got " + hz);
        }
        requireNotRanging();
        uld().setRangingFrequencyHz(hz);
    }

    /** {@link Vl53l8Driver#RANGING_MODE_CONTINUOUS} or {@link Vl53l8Driver#RANGING_MODE_AUTONOMOUS}. */
    public int getRangingMode() {
        return uld().getRangingMode();
    }

    public void setRangingMode(int mode) {
        requireNotRanging();
        uld().setRangingMode(mode);
    }

    public int getIntegrationTimeMs() {
        return uld().getIntegrationTimeMs();
    }

    /** 2–1000 ms; autonomous mode only. */
    public void setIntegrationTimeMs(int ms) {
        requireNotRanging();
        uld().setIntegrationTimeMs(ms);
    }

    public int getSharpenerPercent() {
        return uld().getSharpenerPercent();
    }

    /** Edge sharpener, 0–99 % (0 = off). */
    public void setSharpenerPercent(int pct) {
        requireNotRanging();
        uld().setSharpenerPercent(pct);
    }

    /** {@link Vl53l8Driver#TARGET_ORDER_CLOSEST} or {@link Vl53l8Driver#TARGET_ORDER_STRONGEST}. */
    public int getTargetOrder() {
        return uld().getTargetOrder();
    }

    public void setTargetOrder(int order) {
        requireNotRanging();
        uld().setTargetOrder(order);
    }

    // ── advanced features (UM3109) ───────────────────────────────────────────

    /** {@link Vl53l8Driver#POWER_MODE_SLEEP}, {@link Vl53l8Driver#POWER_MODE_WAKEUP} or {@link Vl53l8Driver#POWER_MODE_DEEP_SLEEP}. */
    public int getPowerMode() {
        return uld().getPowerMode();
    }

    /** Waking from deep sleep downloads the firmware again. */
    public void setPowerMode(int mode) {
        requireNotRanging();
        uld().setPowerMode(mode);
    }

    /** Xtalk margin, whole kcps/SPAD (as ST). */
    public int getXtalkMargin() {
        return uld().getXtalkMargin();
    }

    /** Xtalk margin, a whole number of kcps/SPAD, 0..10000 (as ST); anything else is refused. */
    public void setXtalkMargin(int marginKcps) {
        requireNotRanging();
        uld().setXtalkMargin(marginKcps);
    }

    /** On-device crosstalk calibration against a flat target ({@link Vl53l8Driver#calibrateXtalk}); read the result with {@link #getCaldataXtalk()}. */
    public void calibrateXtalk(int reflectancePercent, int nbSamples, int distanceMm) {
        requireNotRanging();
        uld().calibrateXtalk(reflectancePercent, nbSamples, distanceMm);
    }

    /** The 776-byte xtalk calibration buffer (save / restore). */
    public byte[] getCaldataXtalk() {
        requireNotRanging();
        return uld().getCaldataXtalk();
    }

    public void setCaldataXtalk(byte[] blob) {
        requireNotRanging();
        uld().setCaldataXtalk(blob);
    }

    public int getDetectionThresholdsEnable() {
        return uld().getDetectionThresholdsEnable();
    }

    public void setDetectionThresholdsEnable(boolean enabled) {
        requireNotRanging();
        uld().setDetectionThresholdsEnable(enabled);
    }

    public List<Vl53l8Uld.DetectionThreshold> getDetectionThresholds() {
        return uld().getDetectionThresholds();
    }

    /** The 64 detection thresholds (interrupt on threshold). */
    public void setDetectionThresholds(List<Vl53l8Uld.DetectionThreshold> thresholds) {
        requireNotRanging();
        uld().setDetectionThresholds(thresholds);
    }

    public void setDetectionThresholdsAutoStop(boolean autoStop) {
        requireNotRanging();
        uld().setDetectionThresholdsAutoStop(autoStop);
    }

    /**
     * Enable the motion indicator over [min, max] mm; every frame then carries
     * {@link Vl53l8Frame#motion()}. Returns the configuration for further tuning.
     *
     * <p>On CH firmware the motion indicator and CNH share one DCI struct (0xBFAC): this disarms
     * CNH ({@code cnhSetup()} null, no {@link Vl53l8Frame#cnhRaw()}).
     */
    public Vl53l8Uld.MotionConfig configureMotionIndicator(int distanceMinMm, int distanceMaxMm) {
        requireNotRanging();
        Vl53l8Uld.MotionConfig cfg = uld().motionIndicatorInit(getResolution());
        armedCnh = null;
        uld().motionIndicatorSetDistanceMotion(cfg, distanceMinMm, distanceMaxMm);
        return cfg;
    }

    public Vl53l8Uld.MotionConfig configureMotionIndicator() {
        return configureMotionIndicator(400, 1500);
    }

    // ── CNH (the CH classes) ─────────────────────────────────────────────────

    /** The CNH setup the CH classes armed, or null. */
    protected Vl53l8CnhSetup armedCnh() {
        return armedCnh;
    }

    // The recorder writes the armed setup next to each raw CNH block.
    Vl53l8CnhSetup cnhSetupArmed() {
        return armedCnh;
    }

    // CH classes: arm CNH for the next startRanging (the setup is checked before it is written).
    protected void armCnh(Vl53l8CnhSetup setup) {
        if (setup == null) {
            throw new NullPointerException("setup");
        }
        requireNotRanging();
        setup.requiredMemory(); // a blank or oversized setup is refused before anything is sent
        uld().dciWriteData(Vl53l8CnhSetup.MI_CFG_DEV_IDX, setup.pack());
        uld().motionIndicatorDisarm(); // one DCI block (ERRATA B4)
        armedCnh = setup;
    }

    // The sensor lost its state (power or reset pin): init() again.
    protected void forgetSensor() {
        ranging = false;
        uld = null;
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    /** Program the output list, start the sensor and the board's stream. */
    public void startRanging() {
        requireNotRanging();
        Vl53l8CnhSetup cnh = armedCnh;
        Integer cnhSize = cnh == null ? null : cnh.requiredMemory();
        getResolution(); // refresh the cache that shapes frames
        uld().startRanging(cnhSize);
        reassembler = new FrameReassembler(streamChunk());
        requestOk(Vl53l8.Vl53l8Cmd.START_STREAM.value, Vl53l8.packStartStream(uld().dataReadSize()));
        ranging = true;
    }

    public void stopRanging() {
        if (!ranging) {
            return;
        }
        // Leave the host "not ranging" and stop the sensor even when the STOP_STREAM ack fails.
        try {
            requestOk(Vl53l8.Vl53l8Cmd.STOP_STREAM.value);
        } finally {
            ranging = false;
            uld().stopRanging();
        }
    }

    /** Call {@code callback} for every parsed frame (reader thread). */
    public AutoCloseable onFrame(Consumer<Vl53l8Frame> callback) {
        return frames.onItem(callback);
    }

    /** A pull queue of parsed frames (bounded, drop-oldest). */
    public Subscription<Vl53l8Frame> frames(int capacity) {
        return frames.subscribe(capacity);
    }

    public Subscription<Vl53l8Frame> frames() {
        return frames(8);
    }

    /** The next frame (contract 07 §3). */
    public Vl53l8Frame getFrame(Duration timeout) {
        return frames.next(timeout == null ? Duration.ofSeconds(2) : timeout, Vl53l8.Vl53l8Rpt.VL53_FRAME.value, this::isClosed);
    }

    /** Frames dropped because they did not parse (corrupt frame, bad size) — not reassembly gaps. */
    public long frameParseErrors() {
        return frameParseErrors.get();
    }

    /** Chunked frames the reassembler discarded (gaps, offset errors). */
    public int reassemblerDiscards() {
        return reassembler.discarded;
    }

    public List<Long> streamDroppedCounts() {
        return frames.droppedCounts();
    }

    protected void requireNotRanging() {
        if (ranging) {
            throw new InvalidStateException("stopRanging() first — the stream owns the register bank");
        }
    }

    @Override
    protected boolean handleReport(Packet packet) {
        if (reassembler == null || packet.cmd() != Vl53l8.Vl53l8Rpt.VL53_FRAME.value || packet.payload().length < STREAM_MIN_BYTES) {
            return false;
        }
        FrameReassembler.CompletedFrame done = reassembler.feed(FrameReassembler.unpackFrameChunk(packet.payload()));
        Vl53l8Driver d = uld;
        if (done == null || d == null) {
            return true;
        }
        Vl53l8Frame frame;
        try {
            frame = d.parseFrame(done.timestampUs(), done.frame()).withResolution(resolution);
        } catch (Vl53l8Uld.Vl53l8Error | IndexOutOfBoundsException | IllegalArgumentException e) {
            frameParseErrors.incrementAndGet();
            return true;
        }
        frames.emit(frame, stats());
        return true;
    }

    @Override
    protected void onReaderStopped() {
        if (frames != null) {
            frames.closeAll();
        }
        super.onReaderStopped();
    }

    // The ULD platform on the firmware register bridge.
    private final class BridgePlatform implements Vl53l8Platform {
        @Override
        public byte[] readMulti(int addr, int size) {
            byte[] output = new byte[size];
            int done = 0;
            int a = addr;
            while (done < size) {
                int n = Math.min(size - done, readChunk());
                Vl53l8.RegData rep = request(Vl53l8.Vl53l8Cmd.READ_REG.value, Vl53l8.packReadReg(a, n),
                        expectReport(Vl53l8.Vl53l8Rpt.REG_DATA.value, Vl53l8.RegData::unpack), REG_TIMEOUT);
                if (rep.data().length != n) {
                    throw new DepzException(String.format("READ_REG 0x%04X: expected %d, got %d", a, n, rep.data().length));
                }
                System.arraycopy(rep.data(), 0, output, done, n);
                a += n;
                done += n;
            }
            return output;
        }

        @Override
        public void writeMulti(int addr, byte[] data) {
            int done = 0;
            int a = addr;
            int chunk = writeChunk();
            BiConsumer<Integer, Integer> progress = writeProgress;
            while (done < data.length) {
                int n = Math.min(data.length - done, chunk);
                requestOk(Vl53l8.Vl53l8Cmd.WRITE_REG.value, Vl53l8.packWriteReg(a, data, done, n), REG_TIMEOUT);
                a += n;
                done += n;
                if (data.length > chunk && progress != null) {
                    progress.accept(done, data.length);
                }
            }
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
