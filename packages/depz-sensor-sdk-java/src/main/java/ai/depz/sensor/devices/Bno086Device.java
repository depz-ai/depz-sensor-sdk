package ai.depz.sensor.devices;

import ai.depz.sensor.protocol.Bno086;
import ai.depz.sensor.sensors.bno086.Reports;
import ai.depz.sensor.sensors.bno086.Sh2;
import ai.depz.sensor.sensors.bno086.Sh2Hub;
import ai.depz.sensor.sensors.bno086.Sh2Hub.CalibrationConfig;
import ai.depz.sensor.sensors.bno086.Sh2Hub.CommandResponse;
import ai.depz.sensor.sensors.bno086.Sh2Hub.Counts;
import ai.depz.sensor.sensors.bno086.Sh2Hub.ErrorRecord;
import ai.depz.sensor.sensors.bno086.Sh2Hub.FeatureResponse;
import ai.depz.sensor.sensors.bno086.Sh2Hub.FrsReadResponse;
import ai.depz.sensor.sensors.bno086.Sh2Hub.FrsReadSession;
import ai.depz.sensor.sensors.bno086.Sh2Hub.FrsWriteResponse;
import ai.depz.sensor.sensors.bno086.Sh2Hub.FrsWriteSession;
import ai.depz.sensor.sensors.bno086.Sh2Hub.MotionIntent;
import ai.depz.sensor.sensors.bno086.Sh2Hub.ProductId;
import ai.depz.sensor.sensors.bno086.Sh2Hub.SensorMetadata;
import ai.depz.sensor.sensors.bno086.Sh2Hub.Sh2Exception;
import ai.depz.sensor.sensors.bno086.Sh2Hub.TareBasis;
import ai.depz.sensor.sensors.bno086.Shtp;
import ai.depz.sensor.transport.Packet;
import java.io.ByteArrayOutputStream;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.function.BiPredicate;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.function.Predicate;
import java.util.logging.Logger;

/**
 * BNO085 / BNO086 9-axis IMU (contract 05). The board only shuttles raw SHTP frames
 * (SEND_SHTP_PACKET / RPT_DATA); the SH-2 hub protocol runs here, ported from the reference SDK.
 * Every send is acknowledged by the bridge at once and every inbound frame arrives as
 * RPT_DATA(cmd 0x00), so replies are correlated at the SH-2 layer only.
 * <pre>{@code
 * imu.hardwareReset();
 * try (Subscription<Reports.Report> reports = imu.reports()) {
 *     imu.enable(Reports.ROTATION_VECTOR, 100);
 *     Reports.Report r = reports.take(Duration.ofSeconds(1));
 * }
 * }</pre>
 * Never call a blocking device method ({@code enable}, {@code tareNow}, …) from an
 * {@link #onReport} callback.
 */
public final class Bno086Device extends DepzDevice {
    private static final Logger LOG = Logger.getLogger(Bno086Device.class.getName());

    /** Granted rates outside [0.9, 2.1] × the requested one are worth a warning, never an error (contract 05 §7). */
    public static final double RATE_LOW_FACTOR = 0.9;
    public static final double RATE_HIGH_FACTOR = 2.1;

    // Re-reads when enable's verify sees "disabled": a stale response of a preceding disable, or the
    // hub answering before it applied the new rate (the lab BNO085 does that once or twice).
    private static final int VERIFY_STALE_RETRIES = 5;
    private static final int STREAM_MIN_BYTES = 9;
    private static final Duration DEFAULT_TIMEOUT = Duration.ofSeconds(1);
    private static final Duration FRS_TIMEOUT = Duration.ofSeconds(2);
    private static final Duration RESET_TIMEOUT = Duration.ofSeconds(2);
    private static final Duration PRODUCT_ID_SETTLE = Duration.ofMillis(150);

    private final Shtp.ShtpLayer shtp = new Shtp.ShtpLayer();
    private final Object shtpLock = new Object();      // RX side (reader thread) + reset
    private final Object shtpTxLock = new Object();    // serialise SEND_SHTP_PACKET
    private final Object controlLock = new Object();
    private final Map<Integer, List<LinkedBlockingQueue<byte[]>>> controlWaiters = new HashMap<>();
    private final Map<Integer, List<CountDownLatch>> flushWaiters = new HashMap<>();
    private final List<Listener> reportCallbacks = new ArrayList<>();
    private final List<ReportQueue> reportQueues = new ArrayList<>();
    private final ByteArrayOutputStream advertisement = new ByteArrayOutputStream();
    private final Map<Integer, FeatureResponse> features = new HashMap<>();
    private final Object resetLock = new Object();
    private boolean resetComplete;
    private int cmdSeq;
    private volatile int busyRetries = 5;
    private volatile Duration busyBackoff = Duration.ofMillis(Bno086.BUSY_BACKOFF_MS);

    private record Listener(Consumer<Reports.Report> callback, Set<Integer> filter) {}

    private record ReportQueue(Subscription<Reports.Report> queue, Set<Integer> filter) {}

    public Bno086Device(Link link, Duration timeout) {
        super(link, timeout);
    }

    public Bno086Device(Link link) {
        super(link);
    }

    public Bno086Device(String port, Duration timeout) {
        super(port, timeout);
    }

    Bno086Device(DepzDevice from) {
        super(from);
    }

    /** SEND_SHTP_PACKET attempts on ERR_BUSY before giving up. */
    public int busyRetries() {
        return busyRetries;
    }

    public void setBusyRetries(int retries) {
        if (retries < 1) {
            throw new IllegalArgumentException("busy retries must be at least 1, got " + retries);
        }
        busyRetries = retries;
    }

    /** The back-off between busy retries (at least 200 ms per the bridge spec). */
    public Duration busyBackoff() {
        return busyBackoff;
    }

    public void setBusyBackoff(Duration backoff) {
        if (backoff == null || backoff.isNegative()) {
            throw new IllegalArgumentException("busy back-off must be a non-negative duration");
        }
        busyBackoff = backoff;
    }

    /** The raw SHTP channel-0 advertisement seen since open or reset. */
    public byte[] advertisement() {
        synchronized (advertisement) {
            return advertisement.toByteArray();
        }
    }

    // ── RX path ──────────────────────────────────────────────────────────────

    @Override
    protected boolean handleReport(Packet packet) {
        // busyBackoff is the last field initialised: null while the base constructor starts the reader.
        if (busyBackoff == null || packet.cmd() != Bno086.Bno086Rpt.DATA.value || packet.payload().length < STREAM_MIN_BYTES) {
            return false;
        }
        Bno086.Data data = Bno086.Data.unpack(packet.payload());
        Shtp.ShtpCargo cargo;
        synchronized (shtpLock) {
            cargo = shtp.feed(data.shtp(), data.timestampUs());
        }
        if (cargo != null) {
            dispatchCargo(cargo); // the first fragment's capture: 0xFB base deltas refer to it
        }
        return true;
    }

    private void dispatchCargo(Shtp.ShtpCargo cargo) {
        byte[] payload = cargo.payload();
        switch (cargo.channel()) {
            case Sh2Hub.CHANNEL_COMMAND -> {
                synchronized (advertisement) {
                    advertisement.writeBytes(payload);
                }
            }
            case Shtp.CHANNEL_EXECUTABLE -> {
                if (payload.length > 0 && payload[0] == Sh2Hub.EXECUTABLE_RESET_COMPLETE) {
                    synchronized (resetLock) {
                        resetComplete = true;
                        resetLock.notifyAll();
                    }
                }
            }
            case Sh2Hub.CHANNEL_CONTROL -> {
                for (byte[] report : Sh2Hub.splitControlCargo(payload)) {
                    int rid = report[0] & 0xFF;
                    if (rid == Sh2.GET_FEATURE_RESPONSE && report.length >= 17) {
                        FeatureResponse resp = FeatureResponse.unpack(report);
                        synchronized (features) {
                            features.put(resp.sensorId(), resp);
                        }
                    }
                    List<LinkedBlockingQueue<byte[]>> waiters;
                    synchronized (controlLock) {
                        waiters = new ArrayList<>(controlWaiters.getOrDefault(rid, List.of()));
                    }
                    for (LinkedBlockingQueue<byte[]> w : waiters) {
                        w.add(report);
                    }
                }
            }
            case Sh2Hub.CHANNEL_INPUT_NORMAL, Sh2Hub.CHANNEL_INPUT_WAKE -> {
                for (Reports.Report rep : Reports.parseInputCargo(payload, cargo.captureUs())) {
                    emitReport(rep);
                }
            }
            case Sh2Hub.CHANNEL_GYRO_RV -> {
                Reports.Report gyro = Reports.parseGyroRvCargo(payload, cargo.captureUs());
                if (gyro != null) {
                    emitReport(gyro);
                }
            }
            default -> { }
        }
    }

    private static int sensorOf(Reports.Report rep) {
        return ((Number) rep.fields().get("sensor_id")).intValue();
    }

    private void emitReport(Reports.Report rep) {
        int sensor = sensorOf(rep);
        if ("FlushCompleted".equals(rep.type())) {
            int flushed = ((Number) rep.fields().get("flushed_sensor")).intValue();
            List<CountDownLatch> waiters;
            synchronized (controlLock) {
                waiters = new ArrayList<>(flushWaiters.getOrDefault(flushed, List.of()));
            }
            for (CountDownLatch done : waiters) {
                done.countDown();
            }
        }
        List<Listener> cbs;
        synchronized (reportCallbacks) {
            cbs = new ArrayList<>(reportCallbacks);
        }
        for (Listener l : cbs) {
            if (l.filter() == null || l.filter().contains(sensor)) {
                l.callback().accept(rep);
            }
        }
        List<ReportQueue> qs;
        synchronized (reportQueues) {
            qs = new ArrayList<>(reportQueues);
        }
        for (ReportQueue q : qs) {
            if (q.filter() == null || q.filter().contains(sensor)) {
                q.queue().put(rep);
            }
        }
    }

    @Override
    protected void onReaderStopped() {
        if (reportQueues != null) {
            synchronized (reportQueues) {
                for (ReportQueue q : reportQueues) {
                    q.queue().markClosed();
                }
            }
        }
        super.onReaderStopped();
    }

    // ── TX path ──────────────────────────────────────────────────────────────

    // Frame the payload and push it through SEND_SHTP_PACKET; ERR_BUSY backs off and resends.
    private void sendShtp(int channel, byte[] payload) {
        byte[] frame;
        synchronized (shtpLock) {
            frame = shtp.nextFrame(channel, payload);
        }
        synchronized (shtpTxLock) {
            for (int attempt = 0; ; attempt++) {
                try {
                    requestOk(Bno086.Bno086Cmd.SEND_SHTP_PACKET.value, frame);
                    return;
                } catch (BusyException e) {
                    if (attempt >= busyRetries - 1) {
                        throw e;
                    }
                    sleep(busyBackoff);
                }
            }
        }
    }

    private static void sleep(Duration d) {
        try {
            Thread.sleep(d.toMillis());
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new DepzException("interrupted");
        }
    }

    private LinkedBlockingQueue<byte[]> await(int rid) {
        LinkedBlockingQueue<byte[]> q = new LinkedBlockingQueue<>();
        synchronized (controlLock) {
            controlWaiters.computeIfAbsent(rid, k -> new ArrayList<>()).add(q);
        }
        return q;
    }

    private void unwait(int rid, LinkedBlockingQueue<byte[]> q) {
        synchronized (controlLock) {
            controlWaiters.get(rid).remove(q);
        }
    }

    private static byte[] next(LinkedBlockingQueue<byte[]> q, long deadline, Duration timeout, int rid) {
        long left = deadline - System.nanoTime();
        byte[] raw = null;
        if (left > 0) {
            try {
                raw = q.poll(left, TimeUnit.NANOSECONDS);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
        if (raw == null) {
            throw new DepzTimeoutException(rid, timeout);
        }
        return raw;
    }

    private static long deadline(Duration timeout) {
        return System.nanoTime() + timeout.toNanos();
    }

    private static Duration or(Duration timeout, Duration fallback) {
        return timeout != null ? timeout : fallback;
    }

    // Send a control payload and wait for the first response of report id `rid` that `pred` accepts.
    private <T> T controlRequest(byte[] payload, int rid, Function<byte[], T> parse, Predicate<T> pred, Duration timeout) {
        LinkedBlockingQueue<byte[]> q = await(rid);
        try {
            sendShtp(Sh2Hub.CHANNEL_CONTROL, payload);
            long end = deadline(timeout);
            while (true) {
                T obj = parse.apply(next(q, end, timeout, rid));
                if (pred == null || pred.test(obj)) {
                    return obj;
                }
            }
        } finally {
            unwait(rid, q);
        }
    }

    private synchronized int nextCmdSeq() {
        int seq = cmdSeq;
        cmdSeq = (seq + 1) & 0xFF;
        return seq;
    }

    // A Command Request with its response, correlated on (command, command_seq).
    private CommandResponse command(int command, byte[] params, Duration timeout) {
        int seq = nextCmdSeq();
        return controlRequest(Sh2.buildCommandRequest(seq, command, params), Sh2.COMMAND_RESPONSE,
                CommandResponse::unpack, r -> r.command() == command && r.commandSeq() == seq, or(timeout, DEFAULT_TIMEOUT));
    }

    // A Command Request the hub does not answer.
    private void commandNoResponse(int command, byte[] params) {
        sendShtp(Sh2Hub.CHANNEL_CONTROL, Sh2.buildCommandRequest(nextCmdSeq(), command, params));
    }

    // Gather every Command Response of one request until `collect` says done (errors, counts).
    private List<CommandResponse> commandCollect(int command, byte[] params,
            BiPredicate<CommandResponse, List<CommandResponse>> collect, Duration timeout) {
        int seq = nextCmdSeq();
        byte[] payload = Sh2.buildCommandRequest(seq, command, params);
        int rid = Sh2.COMMAND_RESPONSE;
        LinkedBlockingQueue<byte[]> q = await(rid);
        List<CommandResponse> results = new ArrayList<>();
        try {
            sendShtp(Sh2Hub.CHANNEL_CONTROL, payload);
            long end = deadline(timeout);
            while (true) {
                CommandResponse resp = CommandResponse.unpack(next(q, end, timeout, rid));
                if (resp.command() != command || resp.commandSeq() != seq) {
                    continue;
                }
                if (collect.test(resp, results)) {
                    return results;
                }
            }
        } finally {
            unwait(rid, q);
        }
    }

    private void forgetHubState() {
        synchronized (resetLock) {
            resetComplete = false;
        }
        synchronized (shtpLock) {
            shtp.reset();
        }
        synchronized (advertisement) {
            advertisement.reset();
        }
        synchronized (features) {
            features.clear();
        }
    }

    private boolean waitResetComplete(Duration timeout) {
        long end = deadline(timeout);
        synchronized (resetLock) {
            while (!resetComplete) {
                long left = end - System.nanoTime();
                if (left <= 0) {
                    return false;
                }
                try {
                    TimeUnit.NANOSECONDS.timedWait(resetLock, left);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    return false;
                }
            }
            return true;
        }
    }

    // ── lifecycle ────────────────────────────────────────────────────────────

    /**
     * Hard-reset the sensor through nRST. The bridge's OK confirms it; the SH-2 executable
     * reset-complete is waited for best-effort (firmware ≤ 0.95 never sent it, ERRATA E9). SHTP
     * state and cached features restart from zero.
     */
    public void hardwareReset(Duration timeout) {
        Duration t = or(timeout, RESET_TIMEOUT);
        forgetHubState();
        requestOk(Bno086.Bno086Cmd.SENSOR_RESET.value, null, t);
        waitResetComplete(t.compareTo(Duration.ofMillis(500)) < 0 ? t : Duration.ofMillis(500));
    }

    public void hardwareReset() {
        hardwareReset(null);
    }

    /** Pulse WAKE (PS0) — the board's pin, not the SH-2 "On" that ends {@link #sleep()} (that is {@link #powerOn()}). */
    public void wake() {
        requestOk(Bno086.Bno086Cmd.SENSOR_WAKE_UP.value);
    }

    /**
     * SH-2 "Sleep": every report stops except those enabled with
     * {@code Sh2.FEATURE_FLAG_WAKEUP | Sh2.FEATURE_FLAG_ALWAYS_ON}; the configuration is kept. No response.
     */
    public void sleep() {
        sendShtp(Shtp.CHANNEL_EXECUTABLE, Sh2.buildSleep());
    }

    /** SH-2 "On": ends {@link #sleep()}; every enabled report comes back. No response. */
    public void powerOn() {
        sendShtp(Shtp.CHANNEL_EXECUTABLE, Sh2.buildPowerOn());
    }

    // ── identification and features ──────────────────────────────────────────

    /** The SH-2 application's Product ID — the first of {@link #productIds}. */
    public ProductId productId(Duration timeout) {
        return productIds(timeout, null).get(0);
    }

    public ProductId productId() {
        return productId(null);
    }

    /**
     * Every component's Product ID Response, the SH-2 application first. Collects until no response
     * came for {@code settle} (default 150 ms), so late ones cannot answer the next request.
     */
    public List<ProductId> productIds(Duration timeout, Duration settle) {
        int rid = Sh2.PRODUCT_ID_RESPONSE;
        LinkedBlockingQueue<byte[]> q = await(rid);
        try {
            sendShtp(Sh2Hub.CHANNEL_CONTROL, Sh2.buildProductIdRequest());
            Duration t = or(timeout, DEFAULT_TIMEOUT);
            List<ProductId> out = new ArrayList<>();
            out.add(ProductId.unpack(next(q, deadline(t), t, rid)));
            Duration s = or(settle, PRODUCT_ID_SETTLE);
            while (true) {
                byte[] more;
                try {
                    more = q.poll(s.toNanos(), TimeUnit.NANOSECONDS);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    break;
                }
                if (more == null) {
                    break;
                }
                out.add(ProductId.unpack(more));
            }
            return out;
        } finally {
            unwait(rid, q);
        }
    }

    public List<ProductId> productIds() {
        return productIds(null, null);
    }

    /** Hz → the Set Feature interval, rounded like the reference (half to even), at least 1 µs. */
    static long intervalUs(double hz) {
        if (!(hz > 0) || Double.isInfinite(hz)) {
            throw new IllegalArgumentException("hz must be positive and finite, got " + hz + "; use disable()");
        }
        return (long) Math.max(1, Math.rint(1_000_000 / hz));
    }

    /**
     * Enable {@code sensor} through Set Feature (0xFD) every {@code intervalUs}. The hub rounds to
     * its grid; with {@code verify} the granted rate is read back through Get Feature and a rate
     * outside 0.9–2.1 × the request is logged as a warning (contract 05 §7). Returns that read-back
     * (null without verify).
     */
    public FeatureResponse enableInterval(int sensor, long intervalUs, long batchUs, int sensitivity, int flags,
            long cfgWord, boolean verify, Duration timeout) {
        sendShtp(Sh2Hub.CHANNEL_CONTROL, Sh2.buildSetFeature(sensor, intervalUs, batchUs, sensitivity, flags, cfgWord));
        if (!verify) {
            return null;
        }
        FeatureResponse resp = verifyFeature(sensor, or(timeout, DEFAULT_TIMEOUT));
        double requested = intervalUs != 0 ? 1_000_000.0 / intervalUs : 0.0;
        double actual = resp.intervalUs() != 0 ? 1_000_000.0 / resp.intervalUs() : 0.0;
        if (!(RATE_LOW_FACTOR * requested <= actual && actual <= RATE_HIGH_FACTOR * requested)) {
            LOG.warning(String.format(Locale.ROOT, "BNO086 sensor 0x%02X: requested %.1f Hz, granted %.1f Hz (outside %s–%s× band)",
                    sensor, requested, actual, RATE_LOW_FACTOR, RATE_HIGH_FACTOR));
        }
        return resp;
    }

    public FeatureResponse enableInterval(int sensor, long intervalUs) {
        return enableInterval(sensor, intervalUs, 0, 0, 0, 0, true, null);
    }

    /** {@link #enableInterval} at {@code hz}. */
    public FeatureResponse enable(int sensor, double hz, long batchUs, int sensitivity, int flags, long cfgWord,
            boolean verify, Duration timeout) {
        return enableInterval(sensor, intervalUs(hz), batchUs, sensitivity, flags, cfgWord, verify, timeout);
    }

    public FeatureResponse enable(int sensor, double hz) {
        return enable(sensor, hz, 0, 0, 0, 0, true, null);
    }

    // Read back a feature just enabled, skipping stale "disabled" answers (see VERIFY_STALE_RETRIES).
    private FeatureResponse verifyFeature(int sensor, Duration timeout) {
        FeatureResponse resp = getFeature(sensor, timeout);
        for (int i = 0; i < VERIFY_STALE_RETRIES && resp.intervalUs() == 0; i++) {
            resp = getFeature(sensor, timeout);
        }
        return resp;
    }

    /** Disable {@code sensor} (Set Feature with interval 0). */
    public void disable(int sensor) {
        sendShtp(Sh2Hub.CHANNEL_CONTROL, Sh2.buildSetFeature(sensor, 0, 0, 0, 0, 0));
        synchronized (features) {
            features.remove(sensor);
        }
    }

    /** Get Feature round trip: the rates in effect for {@code sensor}. */
    public FeatureResponse getFeature(int sensor, Duration timeout) {
        return controlRequest(Sh2.buildGetFeatureRequest(sensor), Sh2.GET_FEATURE_RESPONSE, FeatureResponse::unpack,
                r -> r.sensorId() == sensor, or(timeout, DEFAULT_TIMEOUT));
    }

    public FeatureResponse getFeature(int sensor) {
        return getFeature(sensor, null);
    }

    public FeatureResponse enableRotationVector(double hz) {
        return enable(Reports.ROTATION_VECTOR, hz);
    }

    public FeatureResponse enableGameRotationVector(double hz) {
        return enable(Reports.GAME_ROTATION_VECTOR, hz);
    }

    public FeatureResponse enableAccelerometer(double hz) {
        return enable(Reports.ACCELEROMETER, hz);
    }

    public FeatureResponse enableGyroscope(double hz) {
        return enable(Reports.GYROSCOPE, hz);
    }

    public FeatureResponse enableMagnetometer(double hz) {
        return enable(Reports.MAGNETOMETER, hz);
    }

    public FeatureResponse enableLinearAcceleration(double hz) {
        return enable(Reports.LINEAR_ACCELERATION, hz);
    }

    public FeatureResponse enableGravity(double hz) {
        return enable(Reports.GRAVITY, hz);
    }

    public FeatureResponse enableGyroIntegratedRv(double hz) {
        return enable(Reports.GYRO_INTEGRATED_RV, hz);
    }

    // ── reports ──────────────────────────────────────────────────────────────

    /**
     * Call {@code callback} for every report (of {@code sensors} only, when not null) on the reader
     * thread. Close the result to stop.
     */
    public AutoCloseable onReport(Consumer<Reports.Report> callback, Collection<Integer> sensors) {
        if (callback == null) {
            throw new IllegalArgumentException("callback must not be null");
        }
        Listener entry = new Listener(callback, filter(sensors));
        synchronized (reportCallbacks) {
            reportCallbacks.add(entry);
        }
        return () -> {
            synchronized (reportCallbacks) {
                reportCallbacks.remove(entry);
            }
        };
    }

    public AutoCloseable onReport(Consumer<Reports.Report> callback) {
        return onReport(callback, null);
    }

    /**
     * A pull queue of reports (of {@code sensors} only, when not null), bounded, drop-oldest;
     * subscribed when this returns.
     */
    public Subscription<Reports.Report> reports(Collection<Integer> sensors, int capacity) {
        Set<Integer> f = filter(sensors);
        Subscription<Reports.Report> sub = new Subscription<>(capacity, s -> {
            synchronized (reportQueues) {
                reportQueues.removeIf(e -> e.queue() == s);
            }
        });
        synchronized (reportQueues) {
            reportQueues.add(new ReportQueue(sub, f));
        }
        return sub;
    }

    public Subscription<Reports.Report> reports(int capacity) {
        return reports(null, capacity);
    }

    public Subscription<Reports.Report> reports() {
        return reports(null, 1024);
    }

    public List<Long> streamDroppedCounts() {
        synchronized (reportQueues) {
            return reportQueues.stream().map(e -> e.queue().droppedCount()).toList();
        }
    }

    private static Set<Integer> filter(Collection<Integer> sensors) {
        return sensors == null ? null : new HashSet<>(sensors);
    }

    // ── tare and calibration ─────────────────────────────────────────────────

    /** Tare {@code axes} (a {@code Sh2Hub.TARE_*} bitmap) against {@code basis} (no response). */
    public void tareNow(int axes, TareBasis basis) {
        commandNoResponse(Sh2Hub.CMD_TARE, Sh2Hub.tareNowParams(axes, basis));
    }

    /** All axes against the rotation vector. */
    public void tareNow() {
        tareNow(Sh2Hub.TARE_ALL, TareBasis.ROTATION_VECTOR);
    }

    /** Persist the current tare into FRS (no response). */
    public void persistTare() {
        commandNoResponse(Sh2Hub.CMD_TARE, Sh2Hub.persistTareParams());
    }

    /**
     * The runtime reorientation quaternion (Q14 on the wire; all zeros clears; no response). It is
     * applied on the hub's side, in the hub's own axes: x and y swapped and z negated relative to the
     * reports.
     */
    public void setReorientation(double x, double y, double z, double w) {
        commandNoResponse(Sh2Hub.CMD_TARE, Sh2Hub.setReorientationParams(x, y, z, w));
    }

    /** Configure the motion-engine calibration; {@link Sh2Exception} on a non-zero status. */
    public void setCalibration(boolean accel, boolean gyro, boolean mag, boolean planar, Duration timeout) {
        CommandResponse resp = command(Sh2Hub.CMD_ME_CALIBRATE, Sh2Hub.meCalibrationParams(accel, gyro, mag, planar, 0), timeout);
        if (resp.status() != 0) {
            throw new Sh2Exception("ME calibration configure failed: status " + resp.status());
        }
    }

    public void setCalibration(boolean accel, boolean gyro, boolean mag) {
        setCalibration(accel, gyro, mag, false, null);
    }

    /** Which motion-engine calibrations run. */
    public CalibrationConfig getCalibration(Duration timeout) {
        CommandResponse resp = command(Sh2Hub.CMD_ME_CALIBRATE,
                Sh2Hub.meCalibrationParams(false, false, false, false, Sh2Hub.ME_CAL_GET), timeout);
        if (resp.status() != 0) {
            throw new Sh2Exception("ME calibration get failed: status " + resp.status());
        }
        return new CalibrationConfig(resp.r(1) != 0, resp.r(2) != 0, resp.r(3) != 0, resp.r(4) != 0);
    }

    public CalibrationConfig getCalibration() {
        return getCalibration(null);
    }

    /** Save the dynamic calibration data to flash (DCD Save Now). */
    public void saveDcd(Duration timeout) {
        CommandResponse resp = command(Sh2Hub.CMD_SAVE_DCD, new byte[0], timeout);
        if (resp.status() != 0) {
            throw new Sh2Exception("DCD save failed: status " + resp.status());
        }
    }

    public void saveDcd() {
        saveDcd(null);
    }

    /** The hub's periodic DCD autosave (no response). */
    public void configurePeriodicDcd(boolean enable) {
        commandNoResponse(Sh2Hub.CMD_PERIODIC_DCD_CONFIG, Sh2Hub.periodicDcdParams(enable));
    }

    /**
     * Interactive Calibration (0x0E, BNO086 only): how the device is about to move, so the hub can
     * refine the gyro zero-rate offset while it is still. No response; a BNO085 ignores it. The
     * hub's answer, if it wants the device to stop, is a MotionRequest report.
     */
    public void setMotionIntent(MotionIntent intent) {
        if (intent == null) {
            throw new IllegalArgumentException("intent must not be null");
        }
        commandNoResponse(Sh2Hub.CMD_INTERACTIVE_CALIBRATION, Sh2Hub.interactiveCalibrationParams(intent.value));
    }

    /**
     * Enable MotionRequest (0x2B) reports; the hub sends one when its request changes, and reads
     * the rate back late, so the granted rate is not checked.
     */
    public FeatureResponse enableMotionRequest(double hz) {
        return enable(Reports.MOTION_REQUEST, hz, 0, 0, 0, 0, false, null);
    }

    // ── batching ─────────────────────────────────────────────────────────────

    /**
     * Force Sensor Flush (0xF0): the hub sends {@code sensor}'s batched reports now; returns once its
     * Flush Completed (0xEF) arrives.
     */
    public void flush(int sensor, Duration timeout) {
        Duration t = or(timeout, DEFAULT_TIMEOUT);
        CountDownLatch done = new CountDownLatch(1);
        synchronized (controlLock) {
            flushWaiters.computeIfAbsent(sensor, k -> new ArrayList<>()).add(done);
        }
        try {
            sendShtp(Sh2Hub.CHANNEL_CONTROL, Sh2Hub.forceFlush(sensor));
            boolean ok;
            try {
                ok = done.await(t.toNanos(), TimeUnit.NANOSECONDS);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                ok = false;
            }
            if (!ok) {
                throw new DepzTimeoutException(Reports.FLUSH_COMPLETED, t);
            }
        } finally {
            synchronized (controlLock) {
                flushWaiters.get(sensor).remove(done);
            }
        }
    }

    public void flush(int sensor) {
        flush(sensor, null);
    }

    // ── FRS ──────────────────────────────────────────────────────────────────

    /** Read a whole FRS record: its 32-bit words. */
    public long[] frsRead(int recordId, Duration timeout) {
        Duration t = or(timeout, FRS_TIMEOUT);
        FrsReadSession session = new FrsReadSession(recordId);
        int rid = Sh2.FRS_READ_RESPONSE;
        LinkedBlockingQueue<byte[]> q = await(rid);
        try {
            sendShtp(Sh2Hub.CHANNEL_CONTROL, session.request());
            long end = deadline(t);
            while (!session.done()) {
                session.feed(FrsReadResponse.unpack(next(q, end, t, rid)));
            }
        } finally {
            unwait(rid, q);
        }
        return session.words();
    }

    public long[] frsRead(int recordId) {
        return frsRead(recordId, null);
    }

    /** Write a whole FRS record; {@link Sh2Exception} on failure. */
    public void frsWrite(int recordId, long[] words, Duration timeout) {
        Duration t = or(timeout, FRS_TIMEOUT);
        FrsWriteSession session = new FrsWriteSession(recordId, words);
        int rid = Sh2.FRS_WRITE_RESPONSE;
        LinkedBlockingQueue<byte[]> q = await(rid);
        try {
            sendShtp(Sh2Hub.CHANNEL_CONTROL, session.request());
            long end = deadline(t);
            while (!session.done()) {
                byte[] next = session.feed(FrsWriteResponse.unpack(next(q, end, t, rid)));
                if (next != null) {
                    sendShtp(Sh2Hub.CHANNEL_CONTROL, next);
                }
            }
        } finally {
            unwait(rid, q);
        }
    }

    public void frsWrite(int recordId, long[] words) {
        frsWrite(recordId, words, null);
    }

    /** Read and parse {@code sensor}'s metadata FRS record. */
    public SensorMetadata getMetadata(int sensor, Duration timeout) {
        Integer record = Sh2Hub.METADATA_RECORDS.get(sensor);
        if (record == null) {
            throw new Sh2Exception(String.format("no metadata FRS record known for sensor 0x%02X", sensor));
        }
        return SensorMetadata.fromWords(frsRead(record, timeout));
    }

    public SensorMetadata getMetadata(int sensor) {
        return getMetadata(sensor, null);
    }

    // ── diagnostics and housekeeping ─────────────────────────────────────────

    /** Get Oscillator Type (0x0A): R0, one of {@code Sh2Hub.OSC_*} (an unknown value comes back as is). */
    public int getOscillatorType(Duration timeout) {
        return command(Sh2Hub.CMD_GET_OSCILLATOR_TYPE, new byte[0], timeout).r(0);
    }

    public int getOscillatorType() {
        return getOscillatorType(null);
    }

    /**
     * Clear the in-RAM dynamic calibration and reset (0x0B). The hub resets instead of answering,
     * so this waits for the executable reset-complete.
     */
    public void clearDcdAndReset(Duration timeout) {
        Duration t = or(timeout, RESET_TIMEOUT);
        forgetHubState();
        commandNoResponse(Sh2Hub.CMD_CLEAR_DCD_AND_RESET, new byte[0]);
        if (!waitResetComplete(t)) {
            throw new DepzTimeoutException(Sh2Hub.CMD_CLEAR_DCD_AND_RESET, t);
        }
        // The command consumed a host TX seq; the hub restarted its counters on reset.
        synchronized (shtpLock) {
            shtp.reset();
        }
    }

    public void clearDcdAndReset() {
        clearDcdAndReset(null);
    }

    /** The error queue (0x01), {@code severity} or greater; records stream until one with source 255 (no more). */
    public List<ErrorRecord> getErrors(int severity, Duration timeout) {
        return commandCollect(Sh2Hub.CMD_ERRORS, Sh2Hub.errorsParams(severity), (resp, acc) -> {
            if (resp.r(2) == Sh2Hub.ERROR_SOURCE_NO_MORE_ERRORS) {
                return true; // the terminator, not a record
            }
            acc.add(resp);
            return false;
        }, or(timeout, DEFAULT_TIMEOUT)).stream().map(ErrorRecord::from).toList();
    }

    public List<ErrorRecord> getErrors() {
        return getErrors(0, null);
    }

    /** A sensor's event counts (0x02): two responses, response_seq 0 then 1. */
    public Counts getCounts(int sensor, Duration timeout) {
        List<CommandResponse> responses = commandCollect(Sh2Hub.CMD_COUNTER, Sh2Hub.countsGetParams(sensor), (resp, acc) -> {
            acc.add(resp);
            return acc.stream().anyMatch(r -> r.responseSeq() == 1);
        }, or(timeout, DEFAULT_TIMEOUT));
        byte[] r0 = responses.stream().filter(r -> r.responseSeq() == 0).findFirst()
                .orElseThrow(() -> new Sh2Exception("counts: no response_seq 0")).r();
        byte[] r1 = responses.stream().filter(r -> r.responseSeq() == 1).findFirst().orElseThrow().r();
        return new Counts(sensor, u32(r0, 3), u32(r0, 7), u32(r1, 3), u32(r1, 7));
    }

    public Counts getCounts(int sensor) {
        return getCounts(sensor, null);
    }

    /** Clear a sensor's event counts (0x02, subcommand 1). */
    public void clearCounts(int sensor, Duration timeout) {
        CommandResponse resp = command(Sh2Hub.CMD_COUNTER, Sh2Hub.countsClearParams(sensor), timeout);
        if (resp.status() != 0) {
            throw new Sh2Exception("clear counts failed: status " + resp.status());
        }
    }

    public void clearCounts(int sensor) {
        clearCounts(sensor, null);
    }

    private static long u32(byte[] b, int at) {
        return (b[at] & 0xFFL) | (b[at + 1] & 0xFFL) << 8 | (b[at + 2] & 0xFFL) << 16 | (b[at + 3] & 0xFFL) << 24;
    }
}
