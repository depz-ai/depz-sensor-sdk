package ai.depz.sensor.devices;

import ai.depz.sensor.json.JsonText;
import ai.depz.sensor.protocol.Common;
import ai.depz.sensor.protocol.Identity;
import ai.depz.sensor.transport.CrcError;
import ai.depz.sensor.transport.CrcType;
import ai.depz.sensor.transport.Event;
import ai.depz.sensor.transport.Framing;
import ai.depz.sensor.transport.Packet;
import ai.depz.sensor.transport.PacketParser;
import ai.depz.sensor.transport.Trash;
import java.io.UncheckedIOException;
import java.time.Duration;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * A connection to one DEPZ device in application mode (contracts 02, 07): a reader thread,
 * replies correlated to requests by echoed opcode (one request in flight per opcode), events,
 * and the common commands. Sensor classes extend it. Requests block the calling thread up to
 * {@link #timeout()} (default 200 ms). Callbacks ({@code on…}) run on the reader thread: keep
 * them short, never block, never close the device from one.
 */
public class DepzDevice implements AutoCloseable {
    /** The default request timeout, 200 ms (contract 07 §3). */
    public static final Duration DEFAULT_TIMEOUT = Duration.ofMillis(200);

    private static final Duration READ_SLICE = Duration.ofMillis(50);
    private static final Logger LOG = Logger.getLogger("ai.depz.sensor");

    private final Core core;

    /** Open a connection on any link; the device owns it and closes it. */
    public DepzDevice(Link link, Duration timeout) {
        core = new Core(Objects.requireNonNull(link, "link"), timeout == null ? DEFAULT_TIMEOUT : timeout, this);
        core.start();
    }

    public DepzDevice(Link link) {
        this(link, null);
    }

    /** Open the serial port at {@code port} (e.g. {@code /dev/ttyACM0}, {@code COM5}). */
    public DepzDevice(String port, Duration timeout) {
        this(new SerialPortLink(port), timeout);
    }

    /** Continue on the connection of {@code from} ({@link DepzDevices#attach}). */
    protected DepzDevice(DepzDevice from) {
        core = from.core;
        core.owner = this;
    }

    Core connection() {
        return core;
    }

    // ── lifecycle ────────────────────────────────────────────────────────────

    /** The link's name: the port path, or what the link wraps. */
    public String port() {
        return core.link.name();
    }

    public boolean isClosed() {
        // A reader that ended marks the device lost even if closing its link failed.
        return core.link.isClosed() || core.lost != null;
    }

    /** The timeout of requests that take none of their own. */
    public Duration timeout() {
        return core.timeout;
    }

    public void setTimeout(Duration timeout) {
        if (timeout == null || timeout.isNegative() || timeout.isZero()) {
            throw new IllegalArgumentException("timeout must be positive");
        }
        core.timeout = timeout;
    }

    /** The payload CRC the host puts on its packets (contract 01). */
    public CrcType txCrcType() {
        return core.txCrcType;
    }

    public void setTxCrcType(CrcType crcType) {
        core.txCrcType = Objects.requireNonNull(crcType);
    }

    public LinkStats stats() {
        return core.stats;
    }

    /** Close the link and stop the reader; pending requests and streams end. */
    @Override
    public void close() {
        try {
            if (!isClosed()) {
                onClosing();
            }
        } finally {
            core.close();
        }
    }

    // ── for sensor classes ───────────────────────────────────────────────────

    /**
     * Route a streaming report; return true when the packet was consumed. Runs on the reader thread
     * — which starts in this class's constructor, before a subclass's fields are initialized: a
     * report arriving in that window must find them null and return false (it becomes an event).
     */
    protected boolean handleReport(Packet packet) {
        return false;
    }

    /**
     * The link is about to close: a class whose sensor keeps running on its own stops it here.
     * Exceptions are the class's to swallow; the link closes regardless.
     */
    protected void onClosing() {}

    /** The reader stopped: end the class's streams (null-check them, as in {@link #handleReport}). Runs on the reader thread. */
    protected void onReaderStopped() {
        core.events.closeAll();
    }

    /** Hand an event to the event subscribers (a sensor class's own events). */
    protected void emitEvent(DeviceEvent event) {
        core.events.emit(event, core.stats);
    }

    // ── events ───────────────────────────────────────────────────────────────

    /** Call {@code callback} for every event (reader thread). Close the result to stop. */
    public AutoCloseable onEvent(Consumer<DeviceEvent> callback) {
        return core.events.onItem(callback);
    }

    /** A pull queue of events (bounded, drop-oldest); subscribed when this returns. */
    public Subscription<DeviceEvent> events(int capacity) {
        return core.events.subscribe(capacity);
    }

    public Subscription<DeviceEvent> events() {
        return events(256);
    }

    // ── TX / requests ────────────────────────────────────────────────────────

    /** Send a packet without waiting for anything (escape hatch; prefer a request). */
    public void send(int cmd, byte[] payload) {
        core.send(cmd, payload == null ? new byte[0] : payload, core.txCrcType);
    }

    public void send(int cmd, byte[] payload, CrcType crcType) {
        core.send(cmd, payload == null ? new byte[0] : payload, crcType == null ? core.txCrcType : crcType);
    }

    /**
     * Send {@code cmd} and wait for the first packet {@code matcher} claims (escape hatch, contract
     * 07 §2). A non-OK RPT_STATUS echoing {@code cmd} throws {@link StatusException}
     * ({@link BusyException} for ERR_BUSY); one request per opcode may be in flight.
     */
    @SuppressWarnings("unchecked")
    public <T> T request(int cmd, byte[] payload, ReplyMatcher<T> matcher, Duration timeout) {
        Objects.requireNonNull(matcher, "matcher");
        return (T) core.request(cmd, payload == null ? new byte[0] : payload, matcher::match, false, timeout);
    }

    public <T> T request(int cmd, byte[] payload, ReplyMatcher<T> matcher) {
        return request(cmd, payload, matcher, null);
    }

    /** Send {@code cmd} and wait for RPT_STATUS OK echoing it. */
    public void requestOk(int cmd, byte[] payload, Duration timeout) {
        core.request(cmd, payload == null ? new byte[0] : payload, null, true, timeout);
    }

    public void requestOk(int cmd, byte[] payload) {
        requestOk(cmd, payload, null);
    }

    public void requestOk(int cmd) {
        requestOk(cmd, null, null);
    }

    /** A matcher for a report identified by its report id alone. */
    public static <T> ReplyMatcher<T> expectReport(int reportId, Function<byte[], T> unpack) {
        return p -> p.cmd() == reportId ? unpack.apply(p.payload()) : null;
    }

    /** A matcher for RPT_TEXT echoing {@code requestCmd}. */
    public static ReplyMatcher<String> expectText(int requestCmd) {
        return p -> p.cmd() == Common.Rpt.TEXT.value && p.payload().length > 0 && (p.payload()[0] & 0xFF) == requestCmd
                ? Common.TextReport.unpack(p.payload()).text() : null;
    }

    // ── common commands (contract 02) ────────────────────────────────────────

    public String getDeviceName() {
        return request(Common.Cmd.GET_DEVICE_NAME.value, null, expectText(Common.Cmd.GET_DEVICE_NAME.value));
    }

    public String getSoftwareName() {
        return request(Common.Cmd.GET_NAME_ACTIVE_SOFTWARE.value, null, expectText(Common.Cmd.GET_NAME_ACTIVE_SOFTWARE.value));
    }

    public String getSerialNumber() {
        return request(Common.Cmd.GET_SERIAL.value, null, expectText(Common.Cmd.GET_SERIAL.value));
    }

    /** The running firmware: the software name parsed into mode, sensor type and version (contract 02 §4). */
    public Identity identify() {
        return Identity.parseSoftwareName(getSoftwareName());
    }

    /** The MCU temperature the device last cached, °C (it refreshes about twice a second). */
    public double readMcuTemperature() {
        return request(Common.Cmd.GET_MCU_TEMPERATURE.value, null,
                expectReport(Common.Rpt.TEMPERATURE.value, Common.TemperatureReport::unpack)).celsius();
    }

    /** NTP-style time sync keeping the lowest-RTT of {@code samples} rounds (contract 02 §5). */
    public TimeSyncResult syncTime(int samples) {
        TimeSyncResult best = null;
        for (int i = 0; i < Math.max(1, samples); i++) {
            long t1 = hostNowUs();
            Common.SyncTimeReport rep = request(Common.Cmd.SYNC_TIME.value, Common.packSyncTime(t1),
                    expectReport(Common.Rpt.SYNC_TIME.value, Common.SyncTimeReport::unpack));
            long t4 = hostNowUs();
            long[] or = Common.syncTimeOffsetRtt(t1, rep.mcuRxUs(), rep.mcuTxUs(), t4);
            if (best == null || or[1] < best.rttUs()) {
                best = new TimeSyncResult(or[0], or[1], t4);
            }
        }
        core.timeSync = best;
        return best;
    }

    public TimeSyncResult syncTime() {
        return syncTime(5);
    }

    /** The last {@link #syncTime} result, or null. */
    public TimeSyncResult lastTimeSync() {
        return core.timeSync;
    }

    /** Device µs → host monotonic µs ({@link #hostNowUs}); needs a {@link #syncTime} first. */
    public long toHostTimeUs(long deviceTimestampUs) {
        TimeSyncResult s = core.timeSync;
        if (s == null) {
            throw new InvalidStateException("call syncTime() first");
        }
        return deviceTimestampUs - s.offsetUs();
    }

    /** The device → host payload CRC mode. */
    public CrcType getReportPayloadCrc() {
        return CrcType.fromValue(request(Common.Cmd.GET_PAYLOAD_CRC_TYPE.value, null,
                expectReport(Common.Rpt.PAYLOAD_CRC_TYPE.value, p -> {
                    if (p.length < 1) {
                        throw new IllegalArgumentException("RPT_PAYLOAD_CRC_TYPE: need 1 byte");
                    }
                    return p[0] & 0xFF;
                })));
    }

    /** Set the device → host payload CRC mode (host → device is per packet: {@link #setTxCrcType}). */
    public void setReportPayloadCrc(CrcType crcType) {
        requestOk(Common.Cmd.SET_PAYLOAD_CRC_TYPE.value, Common.packSetPayloadCrcType(crcType.value));
    }

    public Common.SyncPinConfig getSyncPin(int pin) {
        Common.SyncPinConfig.checkPin(pin);
        return request(Common.Cmd.GET_SYNC_PIN_CONFIG.value, new byte[] {(byte) pin},
                expectReport(Common.Rpt.SYNC_PIN_CONFIG.value, Common.SyncPinConfig::unpack));
    }

    public void setSyncPin(Common.SyncPinConfig config) {
        requestOk(Common.Cmd.SET_SYNC_PIN_CONFIG.value, Objects.requireNonNull(config).pack());
    }

    /** DEVICE_RESET: the device acknowledges, then reboots; the link drops. */
    public void reset() {
        requestOk(Common.Cmd.DEVICE_RESET.value);
    }

    /** Reboot into the resident bootloader and close this connection (flashing: contract 06). */
    public void enterBootloaderMode() {
        requestOk(Common.Cmd.BOOTLOADER.value);
        close();
    }

    /** The host monotonic clock in µs — the host side of all time-sync math. */
    public static long hostNowUs() {
        return System.nanoTime() / 1000;
    }

    // ── the connection: reader thread and request correlation ────────────────

    static final class Core {
        final Link link;
        final LinkStats stats = new LinkStats();
        final Fanout<DeviceEvent> events = new Fanout<>();
        volatile DepzDevice owner;
        volatile Duration timeout;
        volatile CrcType txCrcType = CrcType.NONE;
        volatile TimeSyncResult timeSync;

        private final PacketParser parser = new PacketParser();
        private final Object txLock = new Object();
        private final Map<Integer, Pending> pending = new HashMap<>();
        private final Thread reader;
        private int txSeq;
        private int lastRxSeq = -1;
        private volatile boolean closing;
        /** Set when the reader ended on its own (not by close()): every later request fails with it. */
        volatile DeviceLostException lost;

        Core(Link link, Duration timeout, DepzDevice owner) {
            this.link = link;
            this.timeout = timeout;
            this.owner = owner;
            reader = new Thread(this::readerLoop, "depz-reader-" + link.name());
            reader.setDaemon(true);
        }

        void start() {
            reader.start();
        }

        void close() {
            closing = true;
            link.close();
            if (!Thread.currentThread().equals(reader) && reader.isAlive()) {
                try {
                    reader.join(2000);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
            }
        }

        void send(int cmd, byte[] payload, CrcType crcType) {
            DeviceLostException dead = lost;
            if (dead != null) {
                throw new DeviceLostException(dead.getMessage(), dead.getCause());
            }
            synchronized (txLock) {
                byte[] frame = Framing.buildPacket(cmd, payload, txSeq, crcType);
                link.write(frame);
                txSeq = (txSeq + 1) & 0xFF;
                stats.txPackets.incrementAndGet();
                stats.txBytes.addAndGet(frame.length);
            }
        }

        Object request(int cmd, byte[] payload, Function<Packet, Object> matcher, boolean okCompletes, Duration t) {
            Pending p = new Pending(cmd, matcher, okCompletes);
            synchronized (pending) {
                if (pending.containsKey(cmd)) {
                    throw new BusyException(cmd);
                }
                pending.put(cmd, p);
            }
            try {
                send(cmd, payload, txCrcType);
            } catch (RuntimeException e) {
                take(cmd, p);
                throw e;
            }
            Duration wait = t == null ? timeout : t;
            boolean done;
            try {
                done = p.done.await(wait.toNanos(), TimeUnit.NANOSECONDS);
            } catch (InterruptedException e) {
                take(cmd, p);
                Thread.currentThread().interrupt();
                throw new DepzException("interrupted");
            }
            if (!done) {
                take(cmd, p);
                // The reply may have landed between the wait and the removal.
                if (p.done.getCount() > 0) {
                    throw new DepzTimeoutException(cmd, wait);
                }
            }
            if (p.error != null) {
                throw p.error;
            }
            return p.result;
        }

        private void take(int cmd, Pending p) {
            synchronized (pending) {
                if (pending.get(cmd) == p) {
                    pending.remove(cmd);
                }
            }
        }

        private void failAll(RuntimeException e) {
            List<Pending> all;
            synchronized (pending) {
                all = new ArrayList<>(pending.values());
                pending.clear();
            }
            for (Pending p : all) {
                p.fail(e);
            }
        }

        private void emit(DeviceEvent e) {
            events.emit(e, stats);
        }

        private void readerLoop() {
            try {
                while (!closing) {
                    byte[] data;
                    try {
                        data = link.read(READ_SLICE);
                    } catch (DeviceLostException | UncheckedIOException | IllegalStateException e) {
                        lose(new DeviceLostException(String.valueOf(e.getMessage()), e));
                        return;
                    }
                    if (data.length == 0) {
                        continue;
                    }
                    stats.rxBytes.addAndGet(data.length);
                    for (Event ev : parser.feed(data)) {
                        if (ev instanceof Packet pkt) {
                            stats.rxPackets.incrementAndGet();
                            trackSeq(pkt.seq());
                            dispatch(pkt);
                        } else if (ev instanceof CrcError ce) {
                            stats.crcErrors.incrementAndGet();
                            emit(new DeviceEvent.LinkCrcError(ce.cmd(), ce.seq()));
                        } else if (ev instanceof Trash tr) {
                            stats.trashBytes.addAndGet(tr.data().length);
                            emit(new DeviceEvent.Trash(tr.data()));
                        }
                    }
                    stats.headerErrors.set(parser.headerErrors);
                }
            } catch (RuntimeException e) {
                // Anything else ending the reader (a link that threw something unexpected) must not
                // leave a device that looks open and times every request out (contract 07 §3).
                if (!closing) {
                    LOG.log(Level.SEVERE, "reader of " + link.name() + " ended", e);
                }
                lose(new DeviceLostException("reader ended: " + e.getClass().getSimpleName() + ": "
                        + e.getMessage(), e));
            } finally {
                failAll(new LinkClosedException("device closed"));
                owner.onReaderStopped();
            }
        }

        /**
         * The reader ended on its own: close the link, fail what is pending and every later request
         * with {@code e}, say why. Nothing to do when close() ended it.
         */
        private void lose(DeviceLostException e) {
            if (closing) {
                return;
            }
            lost = e;
            try {
                link.close();
            } catch (RuntimeException again) {
                LOG.log(Level.SEVERE, "closing " + link.name() + " after its reader ended", again);
            }
            failAll(e);
            emit(new DeviceEvent.Disconnected(String.valueOf(e.getMessage())));
        }

        private void trackSeq(int seq) {
            // The first packet after open is no gap; a gap is surfaced ahead of its packet.
            if (lastRxSeq >= 0) {
                int expected = (lastRxSeq + 1) & 0xFF;
                if (seq != expected) {
                    stats.seqGaps.incrementAndGet();
                    emit(new DeviceEvent.SeqGap(expected, seq, (seq - expected) & 0xFF));
                }
            }
            lastRxSeq = seq;
        }

        private void dispatch(Packet pkt) {
            byte[] payload = pkt.payload();
            if (pkt.cmd() == Common.Rpt.STATUS.value && payload.length >= 2) {
                Common.StatusReport rep = Common.StatusReport.unpack(payload);
                if (rep.cmd() != Common.UNSOLICITED) {
                    Pending p;
                    Pending completed = null;
                    RuntimeException error = null;
                    synchronized (pending) {
                        p = pending.get(rep.cmd());
                        if (p != null) {
                            if (rep.status() == Common.Status.ERR_BUSY.value) {
                                error = new BusyException(rep.cmd());
                            } else if (rep.status() != Common.Status.OK.value) {
                                error = new StatusException(rep.cmd(), rep.status());
                            } else if (p.okCompletes) {
                                completed = p;
                            }
                            // else: an OK ack while the data report is still to come — keep waiting.
                            if (completed != null || error != null) {
                                pending.remove(rep.cmd());
                            }
                        }
                    }
                    if (p != null) {
                        if (error != null) {
                            p.fail(error);
                        } else if (completed != null) {
                            completed.complete(null);
                        }
                        return;
                    }
                }
                emit(new DeviceEvent.UnsolicitedStatus(rep.status()));
                return;
            }

            // Pending matchers get the first shot at any other packet.
            Pending claimed = null;
            Object result = null;
            RuntimeException bad = null;
            synchronized (pending) {
                for (Pending p : pending.values()) {
                    if (p.matcher == null) {
                        continue;
                    }
                    try {
                        result = p.matcher.apply(pkt);
                    } catch (RuntimeException e) {
                        // The reply is for this request but does not decode (contract 07 §3: reject
                        // it). Whatever the matcher threw, it must never end the reader.
                        bad = e;
                    }
                    if (result != null || bad != null) {
                        claimed = p;
                        pending.remove(p.cmd);
                        break;
                    }
                }
            }
            if (claimed != null) {
                if (bad != null) {
                    claimed.fail(new InvalidReplyException(claimed.cmd, bad));
                } else {
                    claimed.complete(result);
                }
                return;
            }

            // Contract 07 §3: a report longer than its fields is decoded and the tail
            // ignored (a newer firmware may append fields); a shorter one is not a
            // report of that kind and stays visible as hex text below.
            if (pkt.cmd() == Common.Rpt.SEQUENCE_ERROR.value && payload.length >= 2) {
                Common.SequenceErrorReport rep = Common.SequenceErrorReport.unpack(payload);
                stats.deviceSeqErrors.incrementAndGet();
                emit(new DeviceEvent.SequenceError(rep.expectedSeq(), rep.receivedSeq(), true));
                return;
            }
            if (pkt.cmd() == Common.Rpt.TEXT.value && payload.length > 0) {
                Common.TextReport rep = Common.TextReport.unpack(payload);
                emit(new DeviceEvent.Text(rep.cmd(), rep.text()));
                return;
            }
            if (pkt.cmd() == Common.Rpt.TEMPERATURE.value && payload.length >= 10) {
                Common.TemperatureReport rep = Common.TemperatureReport.unpack(payload);
                emit(new DeviceEvent.Temperature(rep.timestampUs(), rep.celsius()));
                return;
            }
            // The report path runs guarded like the matchers: a stream handler that throws drops
            // this one packet, counted and logged — it never ends the reader (contract 07 §3).
            boolean consumed;
            try {
                consumed = owner.handleReport(pkt);
            } catch (RuntimeException e) {
                stats.reportErrors.incrementAndGet();
                LOG.log(Level.SEVERE, String.format("report 0x%02X handler threw on %s", pkt.cmd(), link.name()), e);
                return;
            }
            if (consumed) {
                return;
            }
            // An unrouted report stays visible, so new firmware is debuggable.
            emit(new DeviceEvent.Text(pkt.cmd(), JsonText.hex(payload)));
        }
    }

    private static final class Pending {
        final int cmd;
        final Function<Packet, Object> matcher;
        final boolean okCompletes;
        final CountDownLatch done = new CountDownLatch(1);
        volatile Object result;
        volatile RuntimeException error;

        Pending(int cmd, Function<Packet, Object> matcher, boolean okCompletes) {
            this.cmd = cmd;
            this.matcher = matcher;
            this.okCompletes = okCompletes;
        }

        void complete(Object r) {
            result = r;
            done.countDown();
        }

        void fail(RuntimeException e) {
            error = e;
            done.countDown();
        }
    }
}
