package ai.depz.sensor.devices;

import ai.depz.sensor.protocol.Tmf8829;
import ai.depz.sensor.sensors.tmf8829.MeasurementBuilder;
import ai.depz.sensor.sensors.tmf8829.Tmf8829Config;
import ai.depz.sensor.sensors.tmf8829.Tmf8829Firmware;
import ai.depz.sensor.transport.Packet;
import java.io.ByteArrayOutputStream;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.function.BiConsumer;
import java.util.function.Consumer;
import java.util.function.IntPredicate;

/**
 * TMF8829 multizone dToF (8×8 … 48×32 zones, up to 4 peaks per zone, histograms) over the SPI register
 * bridge (contract 14). The board is a thin bridge: this class runs the sensor. {@link #init} downloads
 * the 14.4 KB sensor firmware (~0.15 s), {@link #configure} writes the config page, {@link #start}
 * streams measurements to {@link #onMeasurement} / {@link #measurements}, {@link #stop} ends it. The
 * sensor's EN pin is wired high: there is no sleep, and recovery from any state is {@link #init} (it
 * starts with a soft reset).
 */
public final class Tmf8829Device extends DepzDevice {
    // Host-interface registers (sensor bootloader and application).
    private static final int REG_APP_ID = 0x00;
    private static final int REG_CMD_STAT = 0x08;
    private static final int REG_SERIAL = 0x1C;
    private static final int REG_CID_RID = 0x20;
    private static final int REG_INT_STATUS = 0xE1;
    private static final int REG_INT_ENAB = 0xE2;
    private static final int REG_ID = 0xE3;
    private static final int REG_REVID = 0xE4;
    private static final int REG_RESET = 0xF7;
    private static final int REG_ENABLE = 0xF8;
    private static final int REG_FIFO_STATUS = 0xFA;
    private static final int REG_FIFO = 0xFF;

    private static final int DEVICE_ID = 0x9E;
    private static final int APP_ID_APP = 0x01;
    private static final int APP_ID_BL = 0x80;
    private static final int ENABLE_CPU_READY = 0x80;
    private static final int ENABLE_PUSEL_MASK = 0x30;
    private static final int ENABLE_PUSEL_BOOTMON = 0x10;
    private static final int ENABLE_PUSEL_RAM = 0x20;
    private static final int ENABLE_PON = 0x04;
    private static final int RESET_SOFT = 0x40;
    private static final int FIFO_ERROR_FLAGS = 0x30;

    private static final int BL_START_RAM_APP = 0x16;
    private static final int BL_I2C_OFF = 0x22;
    private static final int BL_W_FIFO_BOTH = 0x45;

    private static final int CMD_MEASURE = 0x10;
    private static final int CMD_WRITE_PAGE = 0x15;
    private static final int CMD_LOAD_CONFIG_PAGE = 0x16;
    private static final int CMD_STOP = 0xFF;
    private static final int STAT_OK = 0x00;
    private static final int STAT_ACCEPTED = 0x01;

    private static final Map<Integer, String> BL_STATUS_NAMES = Map.of(0, "READY", 1, "ERR_PARAM", 2, "ERR_ADDR", 4, "ERR_FIFO");

    private static final Map<Integer, String> STAT_NAMES = Map.ofEntries(
            Map.entry(0x00, "OK"), Map.entry(0x01, "ACCEPTED"), Map.entry(0x02, "ERR_CONFIG"),
            Map.entry(0x03, "ERR_APPLICATION"), Map.entry(0x04, "ERR_CONFIG_RESULT_SIZE"),
            Map.entry(0x05, "ERR_CONFIG_VCSEL"), Map.entry(0x06, "ERR_WAKEUP_TIMED"),
            Map.entry(0x07, "ERR_RESET_UNEXPECTED"), Map.entry(0x08, "ERR_UNKNOWN_CMD"),
            Map.entry(0x09, "ERR_UNKNOWN_CID"), Map.entry(0x0A, "ERR_STOP_0"), Map.entry(0x0B, "ERR_STOP_1"),
            Map.entry(0x0C, "ERR_STOP_2"), Map.entry(0x0D, "ERR_STOP_3"), Map.entry(0x0E, "ERR_OSC_TUNE"));

    private static final Duration FAST = Duration.ofMillis(100);
    private static final Duration REGISTER_TIMEOUT = Duration.ofSeconds(1);

    private final Object regLock = new Object();    // the bridge takes one register command at a time
    private final Object frameLock = new Object();  // the reassembler and builder, shared with the reader thread
    private final Fanout<MeasurementBuilder.Measurement> measurements = new Fanout<>();
    private final Tmf8829.FrameReassembler reassembler = new Tmf8829.FrameReassembler();
    private final MeasurementBuilder builder = new MeasurementBuilder();
    private volatile boolean streaming;
    private volatile boolean stopping;
    private volatile String appVersion;
    private volatile Long sensorSerial;
    private volatile Tmf8829AppliedConfig config;

    /** What {@link #init} reports: the RAM application version and the sensor's serial number. */
    public record InitResult(String appVersion, long sensorSerial) {}

    public Tmf8829Device(Link link, Duration timeout) {
        super(link, timeout);
    }

    public Tmf8829Device(Link link) {
        super(link);
    }

    public Tmf8829Device(String port, Duration timeout) {
        super(port, timeout);
    }

    Tmf8829Device(DepzDevice from) {
        super(from);
    }

    /** The RAM application version {@link #init} started ("2.200.0"), or null. */
    public String appVersion() {
        return appVersion;
    }

    /** The sensor's serial number, read by {@link #init}, or null. */
    public Long sensorSerial() {
        return sensorSerial;
    }

    /** The last {@link #configure} result (null after {@link #init}). */
    public Tmf8829AppliedConfig config() {
        return config;
    }

    public boolean streaming() {
        return streaming;
    }

    // ── register access ──────────────────────────────────────────────────────

    /**
     * Read {@code length} bytes from {@code addr} in ≤1024-byte requests; reads that reach the FIFO port
     * 0xFF stay there (the sensor does not auto-increment it).
     *
     * @throws DepzException a reply shorter or longer than asked
     */
    public byte[] readRegisters(int addr, int length) {
        ByteArrayOutputStream out = new ByteArrayOutputStream(Math.max(0, length));
        synchronized (regLock) {
            while (length > 0) {
                int n = Math.min(length, Tmf8829.READ_MAX);
                Tmf8829.RegData rep = request(Tmf8829.Tmf8829Cmd.READ_REG.value, Tmf8829.packReadReg(addr, n),
                        expectReport(Tmf8829.Tmf8829Rpt.REG_DATA.value, Tmf8829.RegData::unpack), REGISTER_TIMEOUT);
                if (rep.data().length != n) {
                    throw new DepzException(String.format("READ_REG 0x%02X: expected %d, got %d", addr, n, rep.data().length));
                }
                out.writeBytes(rep.data());
                addr = Math.min(addr + n, REG_FIFO);
                length -= n;
            }
        }
        return out.toByteArray();
    }

    public int readRegister(int addr) {
        return readRegisters(addr, 1)[0] & 0xFF;
    }

    /**
     * Write {@code data} from {@code addr} in ≤1023-byte requests (writes to 0xFF stay at 0xFF);
     * {@code progress} (may be null) gets (done, total) after each.
     */
    public void writeRegisters(int addr, byte[] data, BiConsumer<Integer, Integer> progress) {
        if (data == null) {
            throw new IllegalArgumentException("data must not be null");
        }
        int done = 0;
        synchronized (regLock) {
            while (done < data.length) {
                int n = Math.min(data.length - done, Tmf8829.WRITE_MAX);
                requestOk(Tmf8829.Tmf8829Cmd.WRITE_REG.value, Tmf8829.packWriteReg(addr, Arrays.copyOfRange(data, done, done + n)),
                        REGISTER_TIMEOUT);
                done += n;
                if (addr != REG_FIFO) {
                    addr += n;
                }
                if (progress != null) {
                    progress.accept(done, data.length);
                }
            }
        }
    }

    public void writeRegisters(int addr, byte[] data) {
        writeRegisters(addr, data, null);
    }

    /** Write one register; the low byte of {@code value}. */
    public void writeRegister(int addr, int value) {
        writeRegisters(addr, new byte[] {(byte) value});
    }

    private int poll(int addr, IntPredicate done, Duration timeout, String what) {
        long deadline = System.nanoTime() + timeout.toNanos();
        while (true) {
            int v = readRegister(addr);
            if (done.test(v)) {
                return v;
            }
            if (System.nanoTime() > deadline) {
                throw new Tmf8829Exception(String.format("%s: timeout, reg 0x%02X = 0x%02X", what, addr, v));
            }
        }
    }

    /** Raw host-interface registers, for diagnostics. */
    public Map<String, Integer> sensorState() {
        Object[][] regs = {
            {"ID", REG_ID}, {"REVID", REG_REVID}, {"ENABLE", REG_ENABLE}, {"RESET", REG_RESET}, {"APP_ID", REG_APP_ID},
            {"CMD_STAT", REG_CMD_STAT}, {"INT_STATUS", REG_INT_STATUS}, {"INT_ENAB", REG_INT_ENAB}, {"FIFOSTATUS", REG_FIFO_STATUS},
        };
        Map<String, Integer> state = new LinkedHashMap<>();
        for (Object[] r : regs) {
            state.put((String) r[0], readRegister((int) r[1]));
        }
        return state;
    }

    // ── bridge ───────────────────────────────────────────────────────────────

    /** Bridge counters (never touches the sensor, safe mid-stream). */
    public Tmf8829.Tmf8829Info getBridgeInfo() {
        return request(Tmf8829.Tmf8829Cmd.GET_INFO.value, null,
                expectReport(Tmf8829.Tmf8829Rpt.INFO.value, Tmf8829.Tmf8829Info::unpack));
    }

    // ── init: sensor bootloader + firmware download ──────────────────────────

    private void blCmd(int cmd, byte[] params) {
        byte[] payload;
        if (params == null || params.length == 0) {
            payload = new byte[] {(byte) cmd};
        } else {
            payload = new byte[2 + params.length];
            payload[0] = (byte) cmd;
            payload[1] = (byte) params.length;
            System.arraycopy(params, 0, payload, 2, params.length);
        }
        writeRegisters(REG_CMD_STAT, payload);
        int st = poll(REG_CMD_STAT, v -> v != cmd, FAST, String.format("bootloader cmd 0x%02X", cmd));
        if (st != 0) {
            throw new Tmf8829Exception(String.format("bootloader cmd 0x%02X -> %s", cmd,
                    BL_STATUS_NAMES.getOrDefault(st, "0x" + Integer.toHexString(st))));
        }
    }

    private static void sleepMs(int ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new DepzException("interrupted");
        }
    }

    /**
     * Bring the sensor from any state to its running RAM application: soft reset, bootloader, firmware
     * download, start. Stops a running stream first. {@code progress} (done, total; may be null) tracks
     * the image download.
     */
    public InitResult init(BiConsumer<Integer, Integer> progress) {
        if (streaming) {
            stop();
        }
        synchronized (regLock) {
            writeRegister(REG_ENABLE, ENABLE_PON | ENABLE_PUSEL_BOOTMON);
            sleepMs(3);
            writeRegister(REG_RESET, RESET_SOFT);
            sleepMs(3);
            int dev = readRegister(REG_ID);
            if (dev != DEVICE_ID) {
                throw new Tmf8829Exception(String.format("sensor ID 0x%02X, expected 0x%02X (no TMF8829 on the bus?)", dev, DEVICE_ID));
            }
            poll(REG_APP_ID, v -> v == APP_ID_BL, FAST, "sensor bootloader");
            blCmd(BL_I2C_OFF, null);
            // ENABLE reads 0 over SPI until the sensor's I2C is off.
            poll(REG_ENABLE, v -> (v & ENABLE_CPU_READY) != 0, FAST, "cpu_ready");
            byte[] image = Tmf8829Firmware.image();
            int pad = (4 - image.length % 4) % 4;
            if (pad > 0) {
                image = Arrays.copyOf(image, image.length + pad);
                Arrays.fill(image, image.length - pad, image.length, (byte) 0xFF);
            }
            long load = Tmf8829Firmware.LOAD_ADDR;
            int words = image.length / 4;
            byte[] params = {(byte) load, (byte) (load >> 8), (byte) (load >> 16), (byte) (load >> 24),
                (byte) words, (byte) (words >> 8)};
            blCmd(BL_W_FIFO_BOTH, params);
            writeRegisters(REG_FIFO, image, progress);
            blCmd(BL_START_RAM_APP, null);
            poll(REG_APP_ID, v -> v == APP_ID_APP, FAST, "START_RAM_APP");
            int en = readRegister(REG_ENABLE); // wake-ups must boot the RAM application
            writeRegister(REG_ENABLE, (en & ~ENABLE_PUSEL_MASK) | ENABLE_PUSEL_RAM);
            byte[] ver = readRegisters(REG_APP_ID, 4);
            appVersion = (ver[1] & 0xFF) + "." + (ver[2] & 0xFF) + "." + (ver[3] & 0xFF);
            byte[] s = readRegisters(REG_SERIAL, 4);
            sensorSerial = (s[0] & 0xFFL) | (s[1] & 0xFFL) << 8 | (s[2] & 0xFFL) << 16 | (s[3] & 0xFFL) << 24;
        }
        config = null;
        return new InitResult(appVersion, sensorSerial);
    }

    public InitResult init() {
        return init(null);
    }

    // ── application commands, config page ────────────────────────────────────

    int appCmd(int cmd, int[] accept, Duration timeout) {
        writeRegister(REG_CMD_STAT, cmd);
        int st = poll(REG_CMD_STAT, v -> v < 0x10, timeout != null ? timeout : Duration.ofSeconds(1),
                String.format("sensor cmd 0x%02X", cmd));
        int[] ok = accept != null ? accept : new int[] {STAT_OK};
        if (Arrays.stream(ok).noneMatch(a -> a == st)) {
            throw new Tmf8829Exception(String.format("sensor cmd 0x%02X -> %s", cmd,
                    STAT_NAMES.getOrDefault(st, "0x" + Integer.toHexString(st))));
        }
        return st;
    }

    int appCmd(int cmd) {
        return appCmd(cmd, null, null);
    }

    /** LOAD_CONFIG_PAGE → the 190-byte page 0x22..0xDF now in the sensor. */
    public byte[] configPage() {
        synchronized (regLock) {
            appCmd(CMD_LOAD_CONFIG_PAGE);
            int cid = readRegister(REG_CID_RID);
            if (cid != CMD_LOAD_CONFIG_PAGE) {
                throw new Tmf8829Exception(String.format("CID_RID 0x%02X after LOAD_CONFIG_PAGE", cid));
            }
            return readRegisters(Tmf8829Config.REG_CFG, Tmf8829Config.CFG_SIZE);
        }
    }

    /**
     * Load {@code preset} and write the config page. Every field a preset does not reset is written
     * (contract 14 §4), so the result never depends on an earlier config; {@code fields} (may be null)
     * sets any other page field by name. {@code kiloIterations} null: the preset's. Not while streaming.
     *
     * @throws Tmf8829Exception the sensor refuses the page (e.g. ERR_CONFIG_RESULT_SIZE: the result format
     *     is too big for the grid) or a written field does not read back
     */
    public Tmf8829AppliedConfig configure(String preset, int periodMs, Integer kiloIterations, int nrPeaks,
            boolean signalStrength, boolean noiseStrength, boolean xtalk, boolean histograms, boolean refFrame,
            Map<String, Integer> fields) {
        if (preset == null || !Tmf8829Config.PRESETS.containsKey(preset)) {
            throw new IllegalArgumentException("unknown preset '" + preset + "'; one of "
                    + String.join(", ", Tmf8829Config.PRESETS.keySet()));
        }
        if (streaming) {
            throw new IllegalStateException("configure() while streaming: stop() first");
        }
        // The 3-bit field takes 0..7, the sensor reports at most 4.
        if (nrPeaks < 1 || nrPeaks > 4) {
            throw new IllegalArgumentException("nr_peaks " + nrPeaks + ", must be 1..4");
        }
        Map<String, Integer> want = new LinkedHashMap<>();
        want.put("period_ms", periodMs);
        want.put("kilo_iterations", kiloIterations != null ? kiloIterations : Tmf8829Config.PRESET_KILO_ITERATIONS.get(preset));
        want.put("nr_peaks", nrPeaks);
        want.put("signal_strength", signalStrength ? 1 : 0);
        want.put("noise_strength", noiseStrength ? 1 : 0);
        want.put("xtalk", xtalk ? 1 : 0);
        want.put("dump_histograms", histograms ? 1 : 0);
        want.put("ref_spad_frame", refFrame ? 1 : 0);
        if (fields != null) {
            want.putAll(fields);
        }
        byte[] back;
        synchronized (regLock) {
            appCmd(Tmf8829Config.PRESETS.get(preset));
            byte[] page = configPage();
            for (Map.Entry<String, Integer> kv : want.entrySet()) {
                Tmf8829Config.cfgSet(page, kv.getKey(), kv.getValue());
            }
            writeRegisters(Tmf8829Config.REG_CFG, page);
            appCmd(CMD_WRITE_PAGE);
            back = configPage();
        }
        List<String> stuck = new ArrayList<>();
        for (Map.Entry<String, Integer> kv : want.entrySet()) {
            int got = Tmf8829Config.cfgGet(back, kv.getKey());
            if (got != kv.getValue()) {
                stuck.add(kv.getKey() + " wrote " + kv.getValue() + ", reads " + got);
            }
        }
        if (!stuck.isEmpty()) {
            throw new Tmf8829Exception("config fields did not stick: " + String.join(", ", stuck));
        }
        config = new Tmf8829AppliedConfig(preset, back, histograms, refFrame, Tmf8829Config.configFields(back));
        return config;
    }

    public Tmf8829AppliedConfig configure(String preset, int periodMs) {
        return configure(preset, periodMs, null, 1, false, false, false, false, false, null);
    }

    public Tmf8829AppliedConfig configure(String preset) {
        return configure(preset, Tmf8829Config.DEFAULT_PERIOD_MS);
    }

    /** The 8x8 preset at the default period. */
    public Tmf8829AppliedConfig configure() {
        return configure("8x8");
    }

    // ── streaming ────────────────────────────────────────────────────────────

    /**
     * MEASURE with the bridge stream armed. Configures the default 8x8 preset first when
     * {@link #configure} was not called since {@link #init}.
     */
    public void start() {
        if (streaming) {
            return;
        }
        Tmf8829AppliedConfig cfg = config != null ? config : configure();
        int intMask = Tmf8829.INT_RESULTS | (cfg.histograms() ? Tmf8829.INT_HISTOGRAMS : 0);
        synchronized (regLock) {
            writeRegister(REG_FIFO_STATUS, FIFO_ERROR_FLAGS); // sticky under/overrun
            writeRegister(REG_INT_STATUS, 0xFF);
            writeRegister(REG_INT_ENAB, intMask);
            synchronized (frameLock) {
                reassembler.reset();
                builder.reset();
            }
            requestOk(Tmf8829.Tmf8829Cmd.START_STREAM.value,
                    Tmf8829.packStartStream(intMask, cfg.refFrame() ? Tmf8829.SF_REF_FRAME : 0));
            streaming = true;
            try {
                appCmd(CMD_MEASURE, new int[] {STAT_OK, STAT_ACCEPTED}, null);
            } catch (RuntimeException e) {
                streaming = false;
                requestOk(Tmf8829.Tmf8829Cmd.STOP_STREAM.value);
                throw e;
            }
        }
    }

    /**
     * Sensor STOP, then the bridge stream (the frames already queued drain). The bad-header event a STOP
     * raises mid histogram dump is expected and swallowed.
     */
    public void stop() {
        synchronized (regLock) {
            stopping = true;
            try {
                appCmd(CMD_STOP);
                writeRegister(REG_INT_ENAB, 0);
                writeRegister(REG_INT_STATUS, 0xFF);
                if (streaming) {
                    requestOk(Tmf8829.Tmf8829Cmd.STOP_STREAM.value);
                }
            } finally {
                streaming = false;
                stopping = false;
            }
        }
    }

    /** Call {@code callback} for every measurement (reader thread; don't block). */
    public AutoCloseable onMeasurement(Consumer<MeasurementBuilder.Measurement> callback) {
        return measurements.onItem(callback);
    }

    /** A pull queue of measurements (bounded, drop-oldest); subscribed when this returns. */
    public Subscription<MeasurementBuilder.Measurement> measurements(int capacity) {
        return measurements.subscribe(capacity);
    }

    public Subscription<MeasurementBuilder.Measurement> measurements() {
        return measurements(16);
    }

    /** The next measurement (contract 07 §3). */
    public MeasurementBuilder.Measurement getMeasurement(Duration timeout) {
        return measurements.next(timeout == null ? Duration.ofSeconds(2) : timeout, Tmf8829.Tmf8829Rpt.FRAME.value, this::isClosed);
    }

    public List<Long> streamDroppedCounts() {
        return measurements.droppedCounts();
    }

    /** Frames the bridge dropped (frame_seq gaps) in this stream. */
    public int framesDropped() {
        synchronized (frameLock) {
            return reassembler.seqGaps;
        }
    }

    /** Frames lost to reassembly (incomplete, out-of-order chunks) in this stream. */
    public int framesDiscarded() {
        synchronized (frameLock) {
            return reassembler.discarded;
        }
    }

    public int frameParseErrors() {
        synchronized (frameLock) {
            return builder.parseErrors;
        }
    }

    /**
     * Current sensor-vs-board clock ratio in Q16 (contract 14 §5; nominal 8 << 16 = 8 µs per SYSTICK),
     * for diagnostics.
     */
    public long clockRatioQ16() {
        synchronized (frameLock) {
            return builder.clock.ratioQ16;
        }
    }

    @Override
    protected void onClosing() {
        if (!streaming) {
            return;
        }
        try {
            stop();
        } catch (DepzException e) {
            // closing anyway
        }
    }

    @Override
    protected boolean handleReport(Packet packet) {
        // builder is the last field initialised: null while the base constructor starts the reader.
        if (builder == null) {
            return false;
        }
        if (packet.cmd() == Tmf8829.Tmf8829Rpt.FRAME.value) {
            MeasurementBuilder.Measurement m;
            synchronized (frameLock) {
                Tmf8829.FrameChunk chunk;
                try {
                    chunk = Tmf8829.FrameChunk.unpack(packet.payload());
                } catch (IllegalArgumentException e) {
                    return true;
                }
                Tmf8829.RawFrame raw = reassembler.feed(chunk);
                if (raw == null) {
                    return true;
                }
                try {
                    m = builder.add(raw.data(), raw.timestampUs(), raw.edgeUs());
                } catch (IllegalArgumentException e) {
                    return true; // a time past 2^63 µs: not a real board
                }
            }
            if (m != null) {
                measurements.emit(m);
            }
            return true;
        }
        if (packet.cmd() == Tmf8829.Tmf8829Rpt.EVENT.value) {
            Tmf8829.Tmf8829Event ev;
            try {
                ev = Tmf8829.Tmf8829Event.unpack(packet.payload());
            } catch (IllegalArgumentException e) {
                return true;
            }
            if (ev.code() == Tmf8829.EventCode.BAD_HEADER.value && stopping) {
                return true; // STOP mid histogram dump: expected (contract 14 §2)
            }
            emitEvent(new Tmf8829InterruptEvent(ev));
            return true;
        }
        return false;
    }

    @Override
    protected void onReaderStopped() {
        if (measurements != null) {
            measurements.closeAll();
        }
        super.onReaderStopped();
    }
}
