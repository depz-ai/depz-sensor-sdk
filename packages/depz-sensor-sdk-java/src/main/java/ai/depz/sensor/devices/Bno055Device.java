package ai.depz.sensor.devices;

import ai.depz.sensor.protocol.Bno055;
import ai.depz.sensor.sensors.bno055.Bno055Regs;
import ai.depz.sensor.sensors.bno055.Bno055Regs.AccelConfig;
import ai.depz.sensor.sensors.bno055.Bno055Regs.AxisRemap;
import ai.depz.sensor.sensors.bno055.Bno055Regs.CalibStatus;
import ai.depz.sensor.sensors.bno055.Bno055Regs.CalibrationProfile;
import ai.depz.sensor.sensors.bno055.Bno055Regs.GyroConfig;
import ai.depz.sensor.sensors.bno055.Bno055Regs.MagConfig;
import ai.depz.sensor.sensors.bno055.Bno055Regs.OprMode;
import ai.depz.sensor.sensors.bno055.Bno055Regs.PwrMode;
import ai.depz.sensor.sensors.bno055.Bno055Regs.SystemStatus;
import ai.depz.sensor.sensors.bno055.Bno055Regs.TempSource;
import ai.depz.sensor.sensors.bno055.Bno055Regs.Units;
import ai.depz.sensor.transport.Packet;
import java.time.Duration;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.atomic.AtomicLong;
import java.util.function.Consumer;
import java.util.function.Supplier;

/**
 * BNO055 absolute-orientation IMU (contract 13). The board is a thin I2C register bridge: mode,
 * units, axis remap and calibration are register access here; the sensor fuses on chip at 100 Hz.
 * Typical use: {@link #configure}, then {@link #startStream} and read {@link #samples}, or poll
 * {@link #readSample}. Page 1 (sensor configs, interrupts, unique id) is reached through PAGE_ID
 * and refused while streaming.
 */
public final class Bno055Device extends DepzDevice {
    private static final int STREAM_MIN_BYTES = 10;
    private static final Duration RESET_TIMEOUT = Duration.ofMillis((long) (Bno055.RESET_TIMEOUT_S * 1000));

    // One lock serialises multi-step register sequences (page switch + access, CONFIG round trips).
    private final Object regLock = new Object();
    private final Fanout<Bno055Sample> samples = new Fanout<>();
    private final AtomicLong streamParseErrors = new AtomicLong();
    private Units units;
    private volatile boolean streaming;
    private volatile Units streamUnits = Units.DEFAULT;
    private Configured configured;
    private OprMode modeBeforeSuspend;

    private record Configured(OprMode mode, Units units, AxisRemap remap, CalibrationProfile calibration) {}

    public Bno055Device(Link link, Duration timeout) {
        super(link, timeout);
    }

    public Bno055Device(Link link) {
        super(link);
    }

    public Bno055Device(String port, Duration timeout) {
        super(port, timeout);
    }

    Bno055Device(DepzDevice from) {
        super(from);
    }

    // ── identity & bridge ────────────────────────────────────────────────────

    /** RPT_BNO_INFO: chip ids, sensor firmware revision, bridge counters. Safe while streaming. */
    public Bno055.Bno055Info bridgeInfo() {
        return request(Bno055.Bno055Cmd.GET_INFO.value, null,
                expectReport(Bno055.Bno055Rpt.INFO.value, Bno055.Bno055Info::unpack));
    }

    /** True when the bridge passed the chip-id handshake and the sensor answers with the BNO055 ids. */
    public boolean isAlive() {
        try {
            Bno055.Bno055Info info = bridgeInfo();
            return info.initialized() != 0 && info.idsOk();
        } catch (DepzException e) {
            return false;
        }
    }

    /**
     * Hardware reset through nRESET (the bridge answers after the ~0.5 s boot handshake). Stops a
     * stream; the sensor returns in CONFIG with power-on units and no calibration —
     * {@link #configure} again, or {@link #restoreConfiguration()}.
     */
    public void resetSensor() {
        synchronized (regLock) {
            requestOk(Bno055.Bno055Cmd.RESET.value, null, RESET_TIMEOUT);
            streaming = false;
            units = null;
            waitBooted();
        }
    }

    // ── raw register access ──────────────────────────────────────────────────

    /** Read {@code length} bytes at {@code addr} on {@code page} (128-byte transfers). Page 1 is refused while streaming. */
    public byte[] readRegisters(int addr, int length, int page) {
        synchronized (regLock) {
            return onPage(page, () -> read(addr, length).data());
        }
    }

    public byte[] readRegisters(int addr, int length) {
        return readRegisters(addr, length, 0);
    }

    /**
     * Write at {@code addr} on {@code page}. Most configuration registers only take writes in CONFIG
     * mode (the sensor ignores the rest); the typed setters handle that.
     */
    public void writeRegisters(int addr, byte[] data, int page) {
        synchronized (regLock) {
            onPage(page, () -> {
                write(addr, data);
                return null;
            });
        }
    }

    public void writeRegisters(int addr, byte[] data) {
        writeRegisters(addr, data, 0);
    }

    public int readRegister(int addr, int page) {
        return readRegisters(addr, 1, page)[0] & 0xFF;
    }

    public int readRegister(int addr) {
        return readRegister(addr, 0);
    }

    public void writeRegister(int addr, int value, int page) {
        writeRegisters(addr, new byte[] {checkByte(value, "value")}, page);
    }

    public void writeRegister(int addr, int value) {
        writeRegister(addr, value, 0);
    }

    // ── mode, power, units, axes ─────────────────────────────────────────────

    public OprMode getOperationMode() {
        return oprMode(readRegister(Bno055Regs.REG_OPR_MODE));
    }

    /**
     * Switch OPR_MODE and wait out the switching time; into a fusion mode also wait (up to 1 s) for
     * the fusion outputs. The sensor only switches between CONFIG and another mode, so a switch
     * between two operating modes goes through CONFIG.
     */
    public void setOperationMode(OprMode mode) {
        requireNonNull(mode, "mode");
        synchronized (regLock) {
            if (mode != OprMode.CONFIG) {
                OprMode current = currentMode();
                if (current == mode) {
                    return;
                }
                if (current != OprMode.CONFIG) {
                    switchMode(OprMode.CONFIG);
                }
            }
            switchMode(mode);
        }
    }

    public PwrMode getPowerMode() {
        return pwrMode(readRegister(Bno055Regs.REG_PWR_MODE));
    }

    /**
     * Normal / low power / suspend, written in CONFIG with the operating mode restored after. A
     * suspended chip ignores OPR_MODE writes, so the mode is kept here and put back on wake-up (NDOF
     * when the chip was already asleep at connect, contract 13 §4.6).
     */
    public void setPowerMode(PwrMode mode) {
        requireNonNull(mode, "mode");
        synchronized (regLock) {
            OprMode opr = currentMode();
            int pwr = read(Bno055Regs.REG_PWR_MODE, 1).data()[0] & 0x03;
            OprMode resume;
            if (opr != OprMode.CONFIG) {
                resume = opr;
                switchMode(OprMode.CONFIG);
            } else if (pwr == PwrMode.SUSPEND.value) {
                resume = modeBeforeSuspend != null ? modeBeforeSuspend : OprMode.NDOF;
            } else {
                resume = null; // CONFIG on purpose: stay there
            }
            write(Bno055Regs.REG_PWR_MODE, new byte[] {(byte) mode.value});
            if (mode == PwrMode.SUSPEND) {
                modeBeforeSuspend = resume;
            } else {
                modeBeforeSuspend = null;
                if (resume != null) {
                    switchMode(resume);
                }
            }
        }
    }

    public Units getUnits() {
        Units u = Units.unpack(readRegister(Bno055Regs.REG_UNIT_SEL));
        units = u;
        return u;
    }

    public void setUnits(Units units) {
        requireNonNull(units, "units");
        inConfigMode(() -> {
            write(Bno055Regs.REG_UNIT_SEL, new byte[] {(byte) units.pack()});
            this.units = units;
            return null;
        });
    }

    public AxisRemap getAxisRemap() {
        byte[] b = readRegisters(Bno055Regs.REG_AXIS_MAP_CONFIG, 2);
        return AxisRemap.unpack(b[0] & 0xFF, b[1] & 0xFF);
    }

    /**
     * Remap the output axes for the board's mounting ({@link AxisRemap#placement} gives the datasheet
     * placements P0..P7; P1 is the default).
     */
    public void setAxisRemap(AxisRemap remap) {
        requireNonNull(remap, "remap");
        byte[] packed = remapBytes(remap);
        inConfigMode(() -> {
            write(Bno055Regs.REG_AXIS_MAP_CONFIG, packed);
            return null;
        });
    }

    public TempSource getTemperatureSource() {
        int v = readRegister(Bno055Regs.REG_TEMP_SOURCE) & 0x03;
        return v == TempSource.GYRO.value ? TempSource.GYRO : TempSource.ACCEL;
    }

    public void setTemperatureSource(TempSource source) {
        requireNonNull(source, "source");
        inConfigMode(() -> {
            write(Bno055Regs.REG_TEMP_SOURCE, new byte[] {(byte) source.value});
            return null;
        });
    }

    /**
     * The usual session setup: CONFIG → units → axis remap → calibration profile → {@code mode}; in a
     * fusion mode it returns once the fusion outputs are live (~70 ms). Remembered for
     * {@link #restoreConfiguration()}. {@code units} null means the power-on units; {@code axisRemap}
     * and {@code calibration} null leave the sensor's.
     */
    public void configure(OprMode mode, Units units, AxisRemap axisRemap, CalibrationProfile calibration) {
        requireNonNull(mode, "mode");
        Units u = units != null ? units : Units.DEFAULT;
        synchronized (regLock) {
            waitBooted();
            switchMode(OprMode.CONFIG);
            write(Bno055Regs.REG_UNIT_SEL, new byte[] {(byte) u.pack()});
            this.units = u;
            if (axisRemap != null) {
                write(Bno055Regs.REG_AXIS_MAP_CONFIG, remapBytes(axisRemap));
            }
            if (calibration != null) {
                write(Bno055Regs.REG_CALIB_PROFILE, calibration.pack());
            }
            switchMode(mode);
            if (mode.isFusion()) {
                waitFusionStarted(true);
            }
        }
        configured = new Configured(mode, u, axisRemap, calibration);
    }

    public void configure(OprMode mode, Units units) {
        configure(mode, units, null, null);
    }

    public void configure(OprMode mode) {
        configure(mode, null, null, null);
    }

    /** NDOF with the power-on units. */
    public void configure() {
        configure(OprMode.NDOF, null, null, null);
    }

    /**
     * Re-apply the last {@link #configure} — after {@link #resetSensor()}, or when
     * {@code bridgeInfo().sensorResets()} rose (bus recovery pulses nRESET). A running stream keeps
     * running.
     */
    public void restoreConfiguration() {
        Configured c = configured;
        if (c == null) {
            throw new InvalidStateException("configure() has not been called");
        }
        configure(c.mode(), c.units(), c.remap(), c.calibration());
    }

    // ── status, self-test, calibration ───────────────────────────────────────

    /** ST_RESULT, SYS_CLK_STATUS, SYS_STATUS, SYS_ERR. */
    public SystemStatus systemStatus() {
        synchronized (regLock) {
            int st = read(Bno055Regs.REG_ST_RESULT, 1).data()[0] & 0xFF;
            byte[] b = read(Bno055Regs.REG_SYS_CLK_STATUS, 3).data();
            return new SystemStatus(st, b[0] & 0xFF, b[1] & 0xFF, b[2] & 0xFF);
        }
    }

    /**
     * The built-in self-test (datasheet §3.9.2), in CONFIG, ~400 ms; the operating mode is restored
     * after. Not while streaming.
     */
    public SystemStatus selfTest() {
        requireNotStreaming();
        synchronized (regLock) {
            OprMode previous = currentMode();
            if (previous != OprMode.CONFIG) {
                switchMode(OprMode.CONFIG);
            }
            try {
                int trigger = read(Bno055Regs.REG_SYS_TRIGGER, 1).data()[0] & Bno055Regs.SYS_TRIGGER_CLK_SEL;
                write(Bno055Regs.REG_SYS_TRIGGER, new byte[] {(byte) (trigger | Bno055Regs.SYS_TRIGGER_SELF_TEST)});
                sleepMs(Bno055Regs.SELF_TEST_TIME_MS);
                SystemStatus status = systemStatus();
                // Staying in CONFIG would leave SYS_STATUS at 4 for good.
                if (previous == OprMode.CONFIG) {
                    clearSelfTestStatus();
                }
                return status;
            } finally {
                if (previous != OprMode.CONFIG) {
                    switchMode(previous);
                }
            }
        }
    }

    /** CALIB_STAT: 0..3 per sensor; in NDOF the sensor calibrates in the background. */
    public CalibStatus calibrationStatus() {
        return CalibStatus.unpack(readRegister(Bno055Regs.REG_CALIB_STAT));
    }

    /** Offsets and radii (0x55..0x6A), only readable in CONFIG: the driver switches there and back. */
    public CalibrationProfile readCalibrationProfile() {
        return inConfigMode(() -> CalibrationProfile.unpack(
                read(Bno055Regs.REG_CALIB_PROFILE, Bno055Regs.CALIB_PROFILE_LEN).data()));
    }

    /**
     * Restore a stored profile (datasheet §3.11.5). A starting point, not a lock: the background
     * calibration refines it once fusion resumes.
     */
    public void writeCalibrationProfile(CalibrationProfile profile) {
        requireNonNull(profile, "profile");
        byte[] packed = profile.pack();
        inConfigMode(() -> {
            write(Bno055Regs.REG_CALIB_PROFILE, packed);
            return null;
        });
    }

    /** Soft-iron compensation matrix, 9 × i16 row-major, 1.0 = 16384. */
    public int[] getSicMatrix() {
        return Bno055Regs.unpackSicMatrix(readRegisters(Bno055Regs.REG_SIC_MATRIX, 18));
    }

    /** {@code matrix} null writes the identity. */
    public void setSicMatrix(int[] matrix) {
        byte[] packed = Bno055Regs.packSicMatrix(matrix != null ? matrix : Bno055Regs.SIC_IDENTITY);
        inConfigMode(() -> {
            write(Bno055Regs.REG_SIC_MATRIX, packed);
            return null;
        });
    }

    // ── page 1: raw sensor configuration (non-fusion modes) ──────────────────

    public AccelConfig getAccelConfig() {
        return AccelConfig.unpack(readRegister(Bno055Regs.REG1_ACC_CONFIG, 1));
    }

    /** Accelerometer range / bandwidth / power; fusion modes override it. */
    public void setAccelConfig(AccelConfig config) {
        writePage1Config(Bno055Regs.REG1_ACC_CONFIG, new byte[] {(byte) config.pack()});
    }

    public GyroConfig getGyroConfig() {
        return GyroConfig.unpack(readRegisters(Bno055Regs.REG1_GYR_CONFIG_0, 2, 1));
    }

    public void setGyroConfig(GyroConfig config) {
        writePage1Config(Bno055Regs.REG1_GYR_CONFIG_0, config.pack());
    }

    public MagConfig getMagConfig() {
        return MagConfig.unpack(readRegister(Bno055Regs.REG1_MAG_CONFIG, 1));
    }

    public void setMagConfig(MagConfig config) {
        writePage1Config(Bno055Regs.REG1_MAG_CONFIG, new byte[] {(byte) config.pack()});
    }

    /** The chip's 16-byte unique id (page 1, 0x50..0x5F). */
    public byte[] uniqueId() {
        return readRegisters(Bno055Regs.REG1_UNIQUE_ID, Bno055Regs.UNIQUE_ID_LEN, 1);
    }

    // ── interrupts (motion only on SW 03.11) ─────────────────────────────────

    public int getInterruptEnable() {
        return readRegister(Bno055Regs.REG1_INT_EN, 1);
    }

    /** INT_EN: which interrupt engines run. */
    public void setInterruptEnable(int mask) {
        writeRegister(Bno055Regs.REG1_INT_EN, checkByte(mask, "mask") & 0xFF, 1);
    }

    public int getInterruptMask() {
        return readRegister(Bno055Regs.REG1_INT_MSK, 1);
    }

    /** INT_MSK: which enabled interrupts drive the INT pin. */
    public void setInterruptMask(int mask) {
        writeRegister(Bno055Regs.REG1_INT_MSK, checkByte(mask, "mask") & 0xFF, 1);
    }

    /** One motion-interrupt setting register (page 1, 0x11..0x1F), as a raw byte. Not while streaming. */
    public void setInterruptSetting(int register, int value) {
        if (!Bno055Regs.isInterruptSettingReg(register)) {
            throw new IllegalArgumentException(String.format(
                    "0x%02X is not a page-1 interrupt setting (0x11..0x1F)", register));
        }
        writePage1Config(register, new byte[] {checkByte(value, "value")});
    }

    /** INT_STA — which interrupts fired. Clears on read. */
    public int readInterruptStatus() {
        return readRegister(Bno055Regs.REG_INT_STA);
    }

    /** SYS_TRIGGER RST_INT: reset the interrupt status bits and the INT pin. */
    public void clearInterrupt() {
        synchronized (regLock) {
            int clk = read(Bno055Regs.REG_SYS_TRIGGER, 1).data()[0] & Bno055Regs.SYS_TRIGGER_CLK_SEL;
            write(Bno055Regs.REG_SYS_TRIGGER, new byte[] {(byte) (clk | Bno055Regs.SYS_TRIGGER_RST_INT)});
        }
    }

    // ── data ─────────────────────────────────────────────────────────────────

    /**
     * Poll one register block {@code {addr, length}} (null: the full 46 bytes 0x08..0x35) and decode
     * it. Works alongside a stream.
     */
    public Bno055Sample readSample(int[] block) {
        int[] b = block(block);
        synchronized (regLock) {
            Units u = currentUnits();
            Read r = read(b[0], b[1]);
            return Bno055Sample.decode(r.timestampUs(), b[0], r.data(), u);
        }
    }

    public Bno055Sample readSample() {
        return readSample(null);
    }

    /** (w, x, y, z) — the cheapest orientation read (8 bytes). */
    public double[] readQuaternion() {
        return readSample(new int[] {Bno055Regs.QUAT_BLOCK_ADDR, Bno055Regs.QUAT_BLOCK_LEN}).quaternion();
    }

    /**
     * Arm the bridge to read {@code block} ({@code {addr, length}}, null: the full block) every
     * {@code periodMs} and push it. Fusion runs at 100 Hz, so 10 ms is the useful floor.
     * {@link Bno055#TRIGGER_INT} reads on the INT edge, {@code periodMs} then a missed-edge watchdog
     * (0 = none). Replaces a running stream.
     */
    public void startStream(int periodMs, int[] block, int trigger) {
        int[] b = block(block);
        int addr = b[0];
        int length = b[1];
        if (length < 1 || length > Bno055.XFER_MAX || addr < 0 || addr + length > 0x100) {
            throw new IllegalArgumentException(String.format("block 0x%02X+%d outside 1..%d / page",
                    addr, length, Bno055.XFER_MAX));
        }
        if (periodMs < 0 || periodMs > 0xFFFF) {
            throw new IllegalArgumentException("period must be 0..65535 ms, got " + periodMs);
        }
        if (trigger != Bno055.TRIGGER_TIMER && trigger != Bno055.TRIGGER_INT) {
            throw new IllegalArgumentException("trigger must be TRIGGER_TIMER (0) or TRIGGER_INT (1), got " + trigger);
        }
        // Encoded (and so range-checked: TIMER period 1..60000) before the units read below —
        // a refused argument puts nothing on the wire.
        byte[] payload = Bno055.packStartStream(trigger, addr, length, periodMs);
        synchronized (regLock) {
            // Units first: the reader may decode the first sample before the request returns.
            streamUnits = currentUnits();
            requestOk(Bno055.Bno055Cmd.START_STREAM.value, payload);
            streaming = true;
        }
    }

    public void startStream(int periodMs, int[] block) {
        startStream(periodMs, block, Bno055.TRIGGER_TIMER);
    }

    public void startStream(int periodMs) {
        startStream(periodMs, null, Bno055.TRIGGER_TIMER);
    }

    /** The full block every 10 ms. */
    public void startStream() {
        startStream(10, null, Bno055.TRIGGER_TIMER);
    }

    public void stopStream() {
        if (!streaming) {
            return;
        }
        try {
            requestOk(Bno055.Bno055Cmd.STOP_STREAM.value);
        } finally {
            streaming = false;
        }
    }

    public boolean streaming() {
        return streaming;
    }

    /** Call {@code callback} for every streamed sample (reader thread). */
    public AutoCloseable onSample(Consumer<Bno055Sample> callback) {
        return samples.onItem(callback);
    }

    /** A pull queue of streamed samples (bounded, drop-oldest). */
    public Subscription<Bno055Sample> samples(int capacity) {
        return samples.subscribe(capacity);
    }

    public Subscription<Bno055Sample> samples() {
        return samples(256);
    }

    /** The next streamed sample (contract 07 §3). */
    public Bno055Sample getSample(Duration timeout) {
        return samples.next(timeout == null ? Duration.ofSeconds(1) : timeout, Bno055.Bno055Rpt.STREAM.value, this::isClosed);
    }

    /** Stream reports dropped because they did not decode (a short block). */
    public long streamParseErrors() {
        return streamParseErrors.get();
    }

    public List<Long> streamDroppedCounts() {
        return samples.droppedCounts();
    }

    // ── internal ─────────────────────────────────────────────────────────────

    private record Read(long timestampUs, byte[] data) {}

    // READ_REG in ≤128-byte pieces on the current page → (MCU timestamp of the last piece, bytes).
    private Read read(int addr, int length) {
        checkByte(addr, "addr");
        byte[] output = new byte[length];
        int done = 0;
        long ts = 0;
        while (done < length) {
            int n = Math.min(length - done, Bno055.XFER_MAX);
            Bno055.RegData rep = request(Bno055.Bno055Cmd.READ_REG.value, Bno055.packReadReg(addr, n),
                    expectReport(Bno055.Bno055Rpt.REG_DATA.value, Bno055.RegData::unpack));
            if (rep.data().length != n) {
                throw new DepzException(String.format("READ_REG 0x%02X: expected %d, got %d", addr, n, rep.data().length));
            }
            ts = rep.timestampUs();
            System.arraycopy(rep.data(), 0, output, done, n);
            addr += n;
            done += n;
        }
        return new Read(ts, output);
    }

    private void write(int addr, byte[] data) {
        checkByte(addr, "addr");
        int done = 0;
        while (done < data.length) {
            int n = Math.min(data.length - done, Bno055.XFER_MAX);
            requestOk(Bno055.Bno055Cmd.WRITE_REG.value, Bno055.packWriteReg(addr, Arrays.copyOfRange(data, done, done + n)));
            addr += n;
            done += n;
        }
    }

    private static byte checkByte(int v, String name) {
        if (v < 0 || v > 0xFF) {
            throw new IllegalArgumentException(name + " must be 0..255, got " + v);
        }
        return (byte) v;
    }

    private static void requireNonNull(Object v, String name) {
        if (v == null) {
            throw new IllegalArgumentException(name + " must not be null");
        }
    }

    private static int[] block(int[] block) {
        if (block == null) {
            return new int[] {Bno055Regs.FULL_BLOCK_ADDR, Bno055Regs.FULL_BLOCK_LEN};
        }
        if (block.length != 2) {
            throw new IllegalArgumentException("block is {addr, length}");
        }
        return block;
    }

    private static byte[] remapBytes(AxisRemap remap) {
        int[] p = remap.pack();
        return new byte[] {(byte) p[0], (byte) p[1]};
    }

    private static OprMode oprMode(int value) {
        int v = value & 0x0F;
        for (OprMode m : OprMode.values()) {
            if (m.value == v) {
                return m;
            }
        }
        throw new DepzException("OPR_MODE reads 0x" + Integer.toHexString(v) + ", not a BNO055 mode");
    }

    private static PwrMode pwrMode(int value) {
        int v = value & 0x03;
        for (PwrMode m : PwrMode.values()) {
            if (m.value == v) {
                return m;
            }
        }
        throw new DepzException("PWR_MODE reads " + v + ", not a BNO055 power mode");
    }

    private static void sleepMs(int ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new DepzException("interrupted");
        }
    }

    private OprMode currentMode() {
        return oprMode(read(Bno055Regs.REG_OPR_MODE, 1).data()[0]);
    }

    // One OPR_MODE write plus its settle time (no via-CONFIG logic).
    private void switchMode(OprMode mode) {
        synchronized (regLock) {
            write(Bno055Regs.REG_OPR_MODE, new byte[] {(byte) mode.value});
            sleepMs(mode == OprMode.CONFIG ? Bno055Regs.MODE_SWITCH_TO_CONFIG_MS : Bno055Regs.MODE_SWITCH_FROM_CONFIG_MS);
            if (mode.isFusion()) {
                waitFusionStarted(false);
            }
        }
    }

    private <T> T onPage(int page, Supplier<T> body) {
        if (page == 0) {
            return body.get();
        }
        if (page != 1) {
            throw new IllegalArgumentException("BNO055 has register pages 0 and 1, got " + page);
        }
        requireNotStreaming();
        write(Bno055Regs.REG_PAGE_ID, new byte[] {0x01});
        try {
            return body.get();
        } finally {
            write(Bno055Regs.REG_PAGE_ID, new byte[] {0x00});
        }
    }

    // Run a block in CONFIG mode and put the previous mode back.
    private <T> T inConfigMode(Supplier<T> body) {
        synchronized (regLock) {
            OprMode previous = currentMode();
            if (previous != OprMode.CONFIG) {
                switchMode(OprMode.CONFIG);
            }
            try {
                return body.get();
            } finally {
                if (previous != OprMode.CONFIG) {
                    switchMode(previous);
                }
            }
        }
    }

    // A self-test run in CONFIG leaves SYS_STATUS at 4 until the mode leaves CONFIG: ACCONLY and back.
    private void clearSelfTestStatus() {
        switchMode(OprMode.ACCONLY);
        switchMode(OprMode.CONFIG);
    }

    // Poll SYS_STATUS until the sensor's own boot (init + POST) is over; a 4 that outlasts POST is a
    // self-test leftover, cleared.
    private void waitBooted() {
        long deadline = System.nanoTime() + Bno055Regs.BOOT_SETTLE_TIMEOUT_MS * 1_000_000L;
        int inSelfTest = 0;
        boolean cleared = false;
        while (true) {
            int status = read(Bno055Regs.REG_SYS_STATUS, 1).data()[0] & 0xFF;
            if (!Bno055Regs.SYS_STATUS_BOOTING.contains(status)) {
                return;
            }
            inSelfTest = status == 4 ? inSelfTest + 1 : 0;
            if (inSelfTest >= Bno055Regs.STUCK_SELF_TEST_POLLS && !cleared) {
                clearSelfTestStatus();
                cleared = true;
                inSelfTest = 0;
                continue;
            }
            if (System.nanoTime() > deadline) {
                throw new DepzException("BNO055 did not finish booting (SYS_STATUS stuck)");
            }
            sleepMs(5);
        }
    }

    // A running fusion never outputs the all-zero quaternion. Strict: throw on timeout; else give up
    // quietly (a suspended sensor never updates its registers).
    private void waitFusionStarted(boolean strict) {
        long deadline = System.nanoTime() + Bno055Regs.FUSION_START_TIMEOUT_MS * 1_000_000L;
        while (allZero(read(Bno055Regs.REG_QUA_DATA, 8).data())) {
            if (System.nanoTime() > deadline) {
                if (strict) {
                    throw new DepzException("BNO055 fusion did not start (quaternion stays zero)");
                }
                return;
            }
            sleepMs(10);
        }
    }

    private static boolean allZero(byte[] b) {
        for (byte x : b) {
            if (x != 0) {
                return false;
            }
        }
        return true;
    }

    private void writePage1Config(int addr, byte[] data) {
        requireNotStreaming();
        inConfigMode(() -> onPage(1, () -> {
            write(addr, data);
            return null;
        }));
    }

    private Units currentUnits() {
        if (units == null) {
            units = Units.unpack(read(Bno055Regs.REG_UNIT_SEL, 1).data()[0] & 0xFF);
        }
        return units;
    }

    private void requireNotStreaming() {
        if (streaming) {
            throw new InvalidStateException("stopStream() first — the stream reads page 0");
        }
    }

    @Override
    protected boolean handleReport(Packet packet) {
        // streamUnits is the last field initialised: null while the base constructor starts the reader.
        if (streamUnits == null || packet.cmd() != Bno055.Bno055Rpt.STREAM.value) {
            return false;
        }
        Bno055Sample sample;
        try {
            if (packet.payload().length < STREAM_MIN_BYTES) {
                throw new IllegalArgumentException("short stream report");
            }
            Bno055.StreamData s = Bno055.StreamData.unpack(packet.payload()); // shorter than its own length field: refused
            sample = Bno055Sample.decode(s.timestampUs(), s.addr(), s.data(), streamUnits);
        } catch (IllegalArgumentException e) {
            streamParseErrors.incrementAndGet();
            return true;
        }
        samples.emit(sample, stats());
        return true;
    }

    @Override
    protected void onReaderStopped() {
        if (samples != null) {
            samples.closeAll();
        }
        super.onReaderStopped();
    }
}
