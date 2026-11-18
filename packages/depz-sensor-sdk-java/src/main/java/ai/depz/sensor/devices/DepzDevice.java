package ai.depz.sensor.devices;

import ai.depz.sensor.internal.Waits;
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
import java.util.function.Function;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.jspecify.annotations.Nullable;

/**
 * A connection to one DEPZ device in application mode (contracts 02, 07): a reader thread,
 * replies correlated to requests by echoed opcode (one request in flight per opcode), events,
 * and the common commands. Sensor classes extend it. Requests block the calling thread up to
 * {@link #timeout()} (default 200 ms). Every kind of item a device emits is one {@link
 * SensorStream} named after its items ({@link #events()} here, a sensor class's {@code
 * measurements()} / {@code frames()} / {@code samples()} / {@code reports()}); its callbacks run on
 * the reader thread: keep them short, never block, never close the device from one.
 */
public class DepzDevice implements AutoCloseable {
    /** The default request timeout, 200 ms (contract 07 §3). */
    public static final Duration DEFAULT_TIMEOUT = Duration.ofMillis(200);

    private static final Duration READ_SLICE = Duration.ofMillis(50);
    private static final String STREAM_ENDED = "stream closed: the device is closed";
    private static final Logger LOG = Logger.getLogger("ai.depz.sensor");

    private final Core core;

    /** Open a connection on any link; the device owns it and closes it. */
    public DepzDevice(Link link, @Nullable Duration timeout) {
        core = new Core(Objects.requireNonNull(link, "link"), timeout == null ? DEFAULT_TIMEOUT : timeout, this);
        core.start();
    }

    public DepzDevice(Link link) {
        this(link, null);
    }

    /** Continue on the connection of {@code from} ({@link DepzDevices#attachSensorClass}). */
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

    /**
     * The loss that ended this device ({@code getCause()} is what the link threw), or {@code null}: still
     * open, or ended by the program's own {@link #close} (contract 07 §3).
     */
    public @Nullable DeviceLostException lost() {
        return core.lost;
    }

    /**
     * What ended this device on a loss: the exception the link threw (an Android USB link's own
     * {@code DeviceLostException}, an {@code UncheckedIOException}, …) — {@code lost().getCause()};
     * {@code null} while open or after the program's own {@link #close}.
     */
    public @Nullable Throwable lostCause() {
        DeviceLostException dead = core.lost;
        return dead == null ? null : dead.getCause();
    }

    /**
     * Throws what every request of a lost or closed device throws (contract 07 §3): the device-lost
     * error after a loss, the closed one after {@link #close}. A sensor class calls it before a guard of
     * its own state — "stop the stream first" — so that a board unplugged while it streamed says it is
     * lost, not that it still streams.
     */
    protected final void requireOpen() {
        core.checkOpen();
    }

    /** True once the device closed, by {@link #close} or by a loss ({@link #lost} tells which). */
    public boolean isClosed() {
        return core.isClosed();
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

    /**
     * Every request in flight has just failed with {@code error}: the reader is ending — after
     * {@link #close()} with {@link LinkClosedException}; on a lost link first with the
     * {@link DeviceLostException} that names the cause, then with the closed one. A class whose waits
     * are not requests (the BNO086's SH-2 exchanges) ends them here the same way, at once, and
     * refuses the ones started afterwards (contract 07 §3). Runs on the reader thread, also in the
     * window before a subclass's fields are initialized (they may be null); an exception thrown here
     * is logged and dropped.
     */
    void onPendingFailed(DepzException error) {}

    /**
     * A sensor class's stream of the items it decodes: {@code next()} names {@code reportId} in its
     * timeout, {@code subscribe()} queues {@code defaultCapacity} items. Ends with the reader, before
     * {@link #events()}.
     */
    final <T> SensorStream<T> stream(int reportId, int defaultCapacity) {
        return core.register(new SensorStream<>(core, reportId, defaultCapacity));
    }

    // ── events ───────────────────────────────────────────────────────────────

    /**
     * The unsolicited / diagnostic events (contract 07 §2): unsolicited status, sequence gaps, CRC
     * errors, trash bytes, text, temperature, the disconnect, and a report no class routes (as hex
     * text) — {@code events().on(cb)} subscribes, {@code events().subscribe(capacity)} queues. It
     * offers no {@code next()} (contract 07 §3).
     */
    public SensorStream<DeviceEvent> events() {
        return core.events;
    }

    // ── TX / requests ────────────────────────────────────────────────────────

    /** Send a packet without waiting for anything (escape hatch; prefer a request). */
    public void send(int cmd, byte @Nullable [] payload) {
        core.send(cmd, payload == null ? new byte[0] : payload, core.txCrcType);
    }

    public void send(int cmd, byte @Nullable [] payload, @Nullable CrcType crcType) {
        core.send(cmd, payload == null ? new byte[0] : payload, crcType == null ? core.txCrcType : crcType);
    }

    /**
     * Send {@code cmd} and wait for the first packet {@code matcher} claims (escape hatch, contract
     * 07 §2). A non-OK RPT_STATUS echoing {@code cmd} throws {@link StatusException}
     * ({@link BusyException} for ERR_BUSY); one request per opcode may be in flight.
     */
    @SuppressWarnings("unchecked")
    public <T> T request(int cmd, byte @Nullable [] payload, ReplyMatcher<T> matcher, @Nullable Duration timeout) {
        Objects.requireNonNull(matcher, "matcher");
        return (T) core.request(cmd, payload == null ? new byte[0] : payload, matcher::match, false, timeout);
    }

    public <T> T request(int cmd, byte @Nullable [] payload, ReplyMatcher<T> matcher) {
        return request(cmd, payload, matcher, null);
    }

    /** Send {@code cmd} and wait for RPT_STATUS OK echoing it. */
    public void requestOk(int cmd, byte @Nullable [] payload, @Nullable Duration timeout) {
        core.request(cmd, payload == null ? new byte[0] : payload, null, true, timeout);
    }

    public void requestOk(int cmd, byte @Nullable [] payload) {
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
    public TimeSync syncTime(int samples) {
        TimeSync best = null;
        for (int i = 0; i < Math.max(1, samples); i++) {
            long t1 = hostNowUs();
            Common.SyncTimeReport rep = request(Common.Cmd.SYNC_TIME.value, Common.packSyncTime(t1),
                    expectReport(Common.Rpt.SYNC_TIME.value, Common.SyncTimeReport::unpack));
            long t4 = hostNowUs();
            long[] or = Common.syncTimeOffsetRtt(t1, rep.mcuRxUs(), rep.mcuTxUs(), t4);
            if (best == null || or[1] < best.rttUs()) {
                best = new TimeSync(or[0], or[1], t4);
            }
        }
        core.timeSync = best;
        return best;
    }

    public TimeSync syncTime() {
        return syncTime(5);
    }

    /** The last {@link #syncTime} result, or null. */
    public @Nullable TimeSync timeSync() {
        return core.timeSync;
    }

    /** Device µs → host monotonic µs ({@link #hostNowUs}); needs a {@link #syncTime} first. */
    public long toHostTimeUs(long deviceTimestampUs) {
        TimeSync s = core.timeSync;
        if (s == null) {
            throw new InvalidStateException("call syncTime() first");
        }
        return deviceTimestampUs - s.offsetUs();
    }

    /** The device → host payload CRC mode. */
    public CrcType getReportPayloadCrc() {
        // in the matcher: a value no CrcType names is an invalid reply (contract 07 §3)
        return request(Common.Cmd.GET_PAYLOAD_CRC_TYPE.value, null,
                expectReport(Common.Rpt.PAYLOAD_CRC_TYPE.value, p -> {
                    if (p.length < 1) {
                        throw new IllegalArgumentException("RPT_PAYLOAD_CRC_TYPE: need 1 byte");
                    }
                    return CrcType.fromValue(p[0] & 0xFF);
                }));
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
        final SensorStream<DeviceEvent> events = new SensorStream<>(this, SensorStream.NO_REPORT, 256);
        /** The sensor classes' streams, ended with the reader (before events). */
        private final List<SensorStream<?>> streams = new ArrayList<>();
        private boolean readerEnded; // guarded by streams
        volatile DepzDevice owner;
        volatile Duration timeout;
        volatile CrcType txCrcType = CrcType.NONE;
        volatile @Nullable TimeSync timeSync;

        private final PacketParser parser = new PacketParser();
        private final Object txLock = new Object();
        private final Map<Integer, Pending> pending = new HashMap<>();
        private final Thread reader;
        private int txSeq;
        private int lastRxSeq = -1;
        private volatile boolean closing;
        /** Set when the reader ended on its own (not by close()): every later request fails with it. */
        volatile @Nullable DeviceLostException lost; // set first: requests fail with it at once
        volatile boolean closeTried; // set once closing the link was tried, by close() or a lost device's reader

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

        boolean isClosed() {
            // A device is closed even if closing its link failed —
            // but only after that close was tried, so isClosed() never shows an open link.
            return link.isClosed() || closeTried;
        }

        /** A class's stream; one made after the reader ended is closed at once. */
        <T> SensorStream<T> register(SensorStream<T> stream) {
            synchronized (streams) {
                streams.add(stream);
                if (readerEnded) {
                    stream.closeAll(endError(STREAM_ENDED));
                }
            }
            return stream;
        }

        /**
         * What a pull call throws on an ended device (contract 07 §3): the loss, whatever came after it;
         * else the program's own close. A link that dropped a moment before the reader named the loss is a
         * loss too.
         */
        DepzException endError(String closedMessage) {
            DeviceLostException dead = lost;
            if (dead != null) {
                return new DeviceLostException(dead.getMessage(), dead.getCause());
            }
            if (closing) {
                return new LinkClosedException(closedMessage);
            }
            return new DeviceLostException(link.name() + ": the link closed under a stream");
        }

        /** The reader stopped: end every class stream, then the events (reader thread). */
        private void endStreams() {
            List<SensorStream<?>> all;
            synchronized (streams) {
                readerEnded = true;
                all = new ArrayList<>(streams);
            }
            DepzException why = endError(STREAM_ENDED);
            for (SensorStream<?> s : all) {
                s.closeAll(why);
            }
            events.closeAll(why);
        }

        void close() {
            closing = true;
            try {
                link.close();
            } finally {
                // Whatever the link's close threw, the reader stops at its next read: the device is
                // closed, not one that looks open and times every request out (contract 07 §3).
                closeTried = true;
                if (!Thread.currentThread().equals(reader) && reader.isAlive()) {
                    try {
                        reader.join(2000);
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                    }
                }
            }
        }

        /**
         * A lost or closed device refuses every request at once with its own error (contract 07 §3),
         * before a slot is taken or anything is written.
         */
        private void checkOpen() {
            DeviceLostException dead = lost;
            if (dead != null) {
                throw new DeviceLostException(dead.getMessage(), dead.getCause());
            }
            // From the moment close() began: the reader may have failed what was pending and ended,
            // and a request that goes out now has nobody to answer or fail it.
            if (closing) {
                throw new LinkClosedException("device closed");
            }
        }

        void send(int cmd, byte[] payload, CrcType crcType) {
            checkOpen();
            synchronized (txLock) {
                byte[] frame = Framing.buildPacket(cmd, payload, txSeq, crcType);
                try {
                    link.write(frame);
                } catch (LinkClosedException e) {
                    if (closing) {
                        throw e;
                    }
                    // The link closed under this side's feet, not by close(): the reader is marking the
                    // device lost (it closes the link after setting `lost`) — the caller sees the loss,
                    // not a "closed" of a device it never closed (contract 07 §3; bug hunt 2026-11-05
                    // F-JAVA-2). The write's own error is the cause, as in the Python reference.
                    DeviceLostException dead = lost;
                    throw new DeviceLostException(
                            dead != null ? dead.getMessage() : link.name() + ": link closed under a request: " + e.getMessage(),
                            e);
                }
                txSeq = (txSeq + 1) & 0xFF;
                stats.txPackets.incrementAndGet();
                stats.txBytes.addAndGet(frame.length);
            }
        }

        @Nullable Object request(int cmd, byte[] payload, @Nullable Function<Packet, @Nullable Object> matcher, boolean okCompletes, @Nullable Duration t) {
            // Before the opcode's slot is taken: on a closed or lost device every request fails with
            // that error, never with "busy" because another caller is giving the slot up at this moment
            // (bug hunt 2026-11-05 F-JAVA-1). send() checks again, for a close() that began in between.
            checkOpen();
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
            long nanos = Waits.nanos(wait);
            if (!await(p, nanos, cmd) && take(cmd, p)) {
                throw new DepzTimeoutException(cmd, wait);
            }
            // Either the reply came in time, or it landed between the wait and the removal: the reader
            // claimed the pending under its lock and is completing it — the reply is consumed, so it is
            // this caller's (bug hunt 2026-11-03 JAVA-2). Only a reader that died mid-completion leaves
            // the latch unset: that wait is bounded too.
            if (p.done.getCount() > 0 && !await(p, nanos, cmd)) {
                throw new DepzTimeoutException(cmd, wait);
            }
            if (p.error != null) {
                throw p.error;
            }
            return p.result;
        }

        /** Wait for {@code p} up to {@code nanos}: true when it completed; an interrupt gives the slot up and throws. */
        private boolean await(Pending p, long nanos, int cmd) {
            try {
                return p.done.await(nanos, TimeUnit.NANOSECONDS);
            } catch (InterruptedException e) {
                take(cmd, p);
                Thread.currentThread().interrupt();
                throw new DepzInterruptedException(e);
            }
        }

        /** Remove {@code p} from the slot of {@code cmd} if it still holds it: false when the reader claimed it meanwhile (or a newer request took the slot). */
        private boolean take(int cmd, Pending p) {
            synchronized (pending) {
                if (pending.get(cmd) == p) {
                    pending.remove(cmd);
                    return true;
                }
                return false;
            }
        }

        private void failAll(DepzException e) {
            List<Pending> all;
            synchronized (pending) {
                all = new ArrayList<>(pending.values());
                pending.clear();
            }
            for (Pending p : all) {
                p.fail(e);
            }
            try {
                owner.onPendingFailed(e);
            } catch (RuntimeException hook) {
                LOG.log(Level.SEVERE, "onPendingFailed threw on " + link.name(), hook);
            }
        }

        private void emit(DeviceEvent e) {
            events.emit(e);
        }

        private void readerLoop() {
            try {
                while (!closing) {
                    byte[] data;
                    try {
                        data = link.read(READ_SLICE);
                    } catch (DeviceLostException | LinkClosedException | UncheckedIOException
                            | IllegalStateException e) {
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
            } catch (Throwable e) {
                // Anything else ending the reader (a link that threw something unexpected — an Error
                // too: an assert in a host-supplied link, a linkage error) must not leave a device that
                // looks open and times every request out (contract 07 §3; bug hunt 2026-11-03 JAVA-3).
                if (!closing) {
                    LOG.log(Level.SEVERE, "reader of " + link.name() + " ended", e);
                }
                lose(new DeviceLostException("reader ended: " + e.getClass().getSimpleName() + ": "
                        + e.getMessage(), e));
                if (e instanceof Error err) {
                    throw err; // the device is closed; the JVM still sees the Error
                }
            } finally {
                failAll(new LinkClosedException("device closed"));
                endStreams();
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
            closeTried = true;
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
        final @Nullable Function<Packet, @Nullable Object> matcher;
        final boolean okCompletes;
        final CountDownLatch done = new CountDownLatch(1);
        volatile @Nullable Object result;
        volatile @Nullable RuntimeException error;

        Pending(int cmd, @Nullable Function<Packet, @Nullable Object> matcher, boolean okCompletes) {
            this.cmd = cmd;
            this.matcher = matcher;
            this.okCompletes = okCompletes;
        }

        void complete(@Nullable Object r) {
            result = r;
            done.countDown();
        }

        void fail(RuntimeException e) {
            error = e;
            done.countDown();
        }
    }
}
