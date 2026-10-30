package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.throwsType;
import static ai.depz.sensor.test.devices.T.until;

import ai.depz.sensor.dataset.Dataset;
import ai.depz.sensor.devices.BootloaderModeException;
import ai.depz.sensor.devices.BusyException;
import ai.depz.sensor.devices.DepzDevice;
import ai.depz.sensor.devices.DepzDevices;
import ai.depz.sensor.devices.DepzException;
import ai.depz.sensor.devices.DepzTimeoutException;
import ai.depz.sensor.devices.DeviceEvent;
import ai.depz.sensor.devices.DeviceLostException;
import ai.depz.sensor.devices.InvalidReplyException;
import ai.depz.sensor.devices.InvalidStateException;
import ai.depz.sensor.devices.LinkClosedException;
import ai.depz.sensor.devices.NoDepzDeviceException;
import ai.depz.sensor.devices.RecordingLink;
import ai.depz.sensor.devices.ReplayLink;
import ai.depz.sensor.devices.ReplayMismatchException;
import ai.depz.sensor.devices.Sr04Device;
import ai.depz.sensor.devices.Sr04Measurement;
import ai.depz.sensor.devices.StatusException;
import ai.depz.sensor.devices.Subscription;
import ai.depz.sensor.json.JsonText;
import ai.depz.sensor.protocol.Common;
import ai.depz.sensor.protocol.Sr04;
import ai.depz.sensor.transport.CrcType;
import ai.depz.sensor.transport.Framing;
import ai.depz.sensor.transport.Packet;
import java.io.ByteArrayInputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Duration;
import java.util.ArrayList;
import java.util.HexFormat;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.zip.GZIPInputStream;

/** The device core over a fake board: requests, errors, events, streams, record / replay. */
public final class DeviceCoreTests {
    private static final Duration SHORT = Duration.ofMillis(100);

    static byte[] text(int cmd, String s) {
        byte[] b = s.getBytes(StandardCharsets.US_ASCII);
        byte[] out = new byte[b.length + 3];
        out[0] = (byte) cmd;
        System.arraycopy(b, 0, out, 1, b.length);
        out[b.length + 2] = (byte) 0xFF;
        return out;
    }

    public static void textRequestStripsTrailingPadding() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        link.onPacket = p -> List.of(link.reply(Common.Rpt.TEXT.value, text(p.cmd(), "APP_usonic_SR04_v0.97")));
        try (DepzDevice dev = new DepzDevice(link)) {
            eq("APP_usonic_SR04_v0.97", dev.getSoftwareName());
            eq("sr04", dev.identify().sensorType().label);
            eq(2L, dev.stats().txPackets());
        }
    }

    public static void toHostTimeBeforeSyncIsTheInvalidStateError() throws Exception {
        // Contract 07 §3 (ERRATA A14): a conversion needs a prior syncTime — the SDK's own error.
        try (DepzDevice dev = new DepzDevice(new FakeBoardLink(), SHORT)) {
            check(dev.lastTimeSync() == null, "no sync yet");
            InvalidStateException e = throwsType(InvalidStateException.class, () -> dev.toHostTimeUs(0));
            check(e.getMessage().contains("syncTime"), e.getMessage());
        }
    }

    public static void noReplyTimesOutNamingTheOpcode() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        try (DepzDevice dev = new DepzDevice(link, SHORT)) {
            DepzTimeoutException e = throwsType(DepzTimeoutException.class, dev::getSerialNumber);
            eq(Common.Cmd.GET_SERIAL.value, e.cmd());
            check(e.getMessage().contains("0x05"), e.getMessage());
        }
    }

    public static void nonOkStatusThrowsTypedErrors() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        link.onPacket = p -> List.of(FakeBoardLink.status(p.cmd(), p.cmd() == Common.Cmd.SET_PAYLOAD_CRC_TYPE.value
                ? Common.Status.ERR_BUSY.value : Common.Status.ERR_CMD_NOT_SUPPORTED.value));
        try (DepzDevice dev = new DepzDevice(link, SHORT)) {
            throwsType(BusyException.class, () -> dev.setReportPayloadCrc(CrcType.CRC8));
            StatusException e = throwsType(StatusException.class, dev::getDeviceName);
            eq("ERR_CMD_NOT_SUPPORTED", e.statusName());
        }
    }

    public static void oneRequestPerOpcodeInFlight() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        DepzDevice dev = new DepzDevice(link, Duration.ofSeconds(1));
        CompletableFuture<Throwable> first = CompletableFuture.supplyAsync(() -> {
            try {
                dev.getSerialNumber();
                return null;
            } catch (Throwable e) {
                return e;
            }
        });
        T.until(() -> link.sentCount() == 1, 1000);
        throwsType(BusyException.class, () -> dev.request(Common.Cmd.GET_SERIAL.value, null,
                DepzDevice.expectText(Common.Cmd.GET_SERIAL.value)));
        dev.close();
        check(first.get(2, TimeUnit.SECONDS) instanceof LinkClosedException, "closing fails the waiting request");
    }

    public static void okAckBeforeTheDataReportKeepsWaiting() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        link.onPacket = p -> List.of(FakeBoardLink.status(p.cmd(), Common.Status.OK.value),
                link.reply(Common.Rpt.PAYLOAD_CRC_TYPE.value, new byte[] {2}));
        try (DepzDevice dev = new DepzDevice(link, SHORT)) {
            eq(CrcType.CRC16, dev.getReportPayloadCrc());
        }
    }

    public static void shortReplyFailsItsRequestAndTheReaderGoesOn() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        link.onPacket = p -> p.cmd() == Common.Cmd.GET_MCU_TEMPERATURE.value
                ? List.of(link.reply(Common.Rpt.TEMPERATURE.value, new byte[3]))
                : List.of(link.reply(Common.Rpt.TEXT.value, text(p.cmd(), "x")));
        try (DepzDevice dev = new DepzDevice(link, SHORT)) {
            InvalidReplyException e = throwsType(InvalidReplyException.class, dev::readMcuTemperature);
            eq(Common.Cmd.GET_MCU_TEMPERATURE.value, e.cmd());
            check(e.getMessage().contains("does not decode"), e.getMessage());
            eq("x", dev.getDeviceName());
        }
    }

    public static void argumentsAreCheckedBeforeAnythingIsSent() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        try (Sr04Device dev = new Sr04Device(link, SHORT)) {
            throwsType(IllegalArgumentException.class, () -> dev.setSamplePeriodUs(-1));
            throwsType(IllegalArgumentException.class, () -> dev.setSamplePeriodUs(1L << 32));
            throwsType(IllegalArgumentException.class, () -> dev.setEchoDecayUs(65536));
            throwsType(IllegalArgumentException.class, () -> dev.getSyncPin(0));
            // contract 02 §2 (ERRATA A2): the CrcType enum cannot hold 4; the codec refuses it for untyped callers
            throwsType(IllegalArgumentException.class, () -> Common.packSetPayloadCrcType(4));
            throwsType(IllegalArgumentException.class, () -> Common.packSetPayloadCrcType(300));
            eq(0, link.sentCount());
        }
    }

    public static void unsolicitedReportsBecomeEvents() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        try (DepzDevice dev = new DepzDevice(link); Subscription<DeviceEvent> events = dev.events()) {
            // in seq order: a gap would be an event of its own
            link.inject(link.reply(Common.Rpt.STATUS.value, new byte[] {0, (byte) Common.Status.ERR_HARDWARE_FAULT.value}));
            link.inject(link.reply(Common.Rpt.SEQUENCE_ERROR.value, new byte[] {7, 9}));
            link.inject(link.reply(0xC5, new byte[] {(byte) 0xAB}));
            DeviceEvent a = events.take(Duration.ofSeconds(1));
            check(a instanceof DeviceEvent.UnsolicitedStatus u && u.isHardwareFault(), "hardware fault: " + a);
            eq(new DeviceEvent.SequenceError(7, 9, true), events.take(Duration.ofSeconds(1)));
            eq(new DeviceEvent.Text(0xC5, "ab"), events.take(Duration.ofSeconds(1)));
            eq(1L, dev.stats().deviceSeqErrors());
        }
    }

    /**
     * Contract 01 §5 step 5 (ERRATA A1): a gap the host sees in the board's seq is a SeqGap event
     * of its own, ahead of the packet it was seen on; the first packet is no gap.
     */
    public static void hostObservedSeqGapIsAnEvent() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        try (DepzDevice dev = new DepzDevice(link); Subscription<DeviceEvent> events = dev.events()) {
            link.inject(Framing.buildPacket(0xC5, new byte[] {5}, 5, CrcType.NONE));
            link.inject(Framing.buildPacket(0xC5, new byte[] {9}, 9, CrcType.NONE));
            eq(new DeviceEvent.Text(0xC5, "05"), events.take(Duration.ofSeconds(1)));
            eq(new DeviceEvent.SeqGap(6, 9, 3), events.take(Duration.ofSeconds(1)));
            eq(new DeviceEvent.Text(0xC5, "09"), events.take(Duration.ofSeconds(1)));
            eq(1L, dev.stats().seqGaps());
            eq(0L, dev.stats().deviceSeqErrors());
        }
    }

    public static void unplugFailsPendingAndEndsStreams() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        try (Sr04Device dev = new Sr04Device(link, Duration.ofSeconds(5));
                Subscription<DeviceEvent> events = dev.events();
                Subscription<Sr04Measurement> loop = dev.measurements()) {
            CompletableFuture<Throwable> waiting = CompletableFuture.supplyAsync(() -> {
                try {
                    dev.getSamplePeriodUs();
                    return null;
                } catch (Throwable e) {
                    return e;
                }
            });
            T.until(() -> link.sentCount() == 1, 1000);
            link.unplug();
            check(waiting.get(2, TimeUnit.SECONDS) instanceof DeviceLostException, "the pending request did not fail");
            check(events.take(Duration.ofSeconds(1)) instanceof DeviceEvent.Disconnected, "disconnected event");
            throwsType(DeviceLostException.class, () -> loop.take(Duration.ofSeconds(1)));
            check(loop.isClosed(), "stream closed");
            // Contract 07 §3: a reader that ended leaves the device closed; a later request fails at once.
            check(T.until(dev::isClosed, 1000), "the device is closed after the loss");
            long t0 = System.nanoTime();
            throwsType(DeviceLostException.class, dev::getSamplePeriodUs);
            check(System.nanoTime() - t0 < 1_000_000_000L, "the request after the loss waited");
        }
    }

    /**
     * Contract 07 §3 (bug hunt 2026-10-29 JAVA-3): a queue subscribed after the device closed is
     * closed at once — take() fails with the closed error instead of waiting out its timeout.
     */
    public static void aQueueSubscribedAfterCloseIsClosedAtOnce() throws Exception {
        Sr04Device dev = new Sr04Device(new FakeBoardLink(), SHORT);
        dev.close();
        try (Subscription<Sr04Measurement> q = dev.measurements(); Subscription<DeviceEvent> events = dev.events()) {
            check(q.isClosed(), "measurements() after close is not closed");
            check(events.isClosed(), "events() after close is not closed");
            long t0 = System.nanoTime();
            throwsType(DeviceLostException.class, () -> q.take(Duration.ofSeconds(5)));
            throwsType(DeviceLostException.class, () -> events.take(Duration.ofSeconds(5)));
            check(System.nanoTime() - t0 < 1_000_000_000L, "take on a closed device's queue waited");
        }
    }

    /** The same after a loss: a queue subscribed once the reader ended is closed at once. */
    public static void aQueueSubscribedAfterALossIsClosedAtOnce() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        try (Sr04Device dev = new Sr04Device(link, SHORT)) {
            link.unplug();
            check(T.until(dev::isClosed, 1000), "the device is closed after the loss");
            try (Subscription<Sr04Measurement> q = dev.measurements()) {
                long t0 = System.nanoTime();
                throwsType(DeviceLostException.class, () -> q.take(Duration.ofSeconds(5)));
                check(System.nanoTime() - t0 < 1_000_000_000L, "take after the loss waited");
            }
        }
    }

    /**
     * Bug hunt 2026-10-29 JAVA-4: an interrupt during request() is the SDK's interrupted error, not
     * a timeout naming the opcode; the interrupt flag stays set.
     */
    public static void anInterruptDuringARequestIsNotATimeout() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        link.onPacket = p -> List.of();
        try (DepzDevice dev = new DepzDevice(link, Duration.ofSeconds(5))) {
            Thread caller = Thread.currentThread();
            Thread interrupter = new Thread(() -> {
                T.until(() -> link.sentCount() == 1, 2000);
                caller.interrupt();
            });
            interrupter.start();
            long t0 = System.nanoTime();
            Throwable e = throwsType(DepzException.class, dev::getDeviceName);
            boolean flag = Thread.interrupted();
            interrupter.join();
            check(!(e instanceof DepzTimeoutException), "reported as a timeout: " + e);
            eq("interrupted", e.getMessage());
            check(flag, "the interrupt flag was cleared");
            check(System.nanoTime() - t0 < 3_000_000_000L, "the request waited out its timeout");
            eq("name", requestAgain(link, dev));
        }
    }

    private static String requestAgain(FakeBoardLink link, DepzDevice dev) {
        // The opcode is free again after the interrupted request.
        link.onPacket = p -> List.of(link.reply(Common.Rpt.TEXT.value, text(p.cmd(), "name")));
        return dev.getDeviceName();
    }

    /** JAVA-4: the same for Subscription.take(). */
    public static void anInterruptDuringTakeIsNotATimeout() throws Exception {
        try (Sr04Device dev = new Sr04Device(new FakeBoardLink(), SHORT);
                Subscription<Sr04Measurement> q = dev.measurements()) {
            Thread.currentThread().interrupt();
            Throwable e = throwsType(DepzException.class, () -> q.take(Duration.ofSeconds(5)));
            boolean flag = Thread.interrupted();
            check(!(e instanceof DepzTimeoutException), "reported as a timeout: " + e);
            eq("interrupted", e.getMessage());
            check(flag, "the interrupt flag was cleared");
            Thread.currentThread().interrupt();
            Throwable g = throwsType(DepzException.class, () -> dev.getMeasurement(Duration.ofSeconds(5)));
            flag = Thread.interrupted();
            check(!(g instanceof DepzTimeoutException), "getMeasurement reported a timeout: " + g);
            check(flag, "the interrupt flag was cleared by getMeasurement");
        }
    }

    /**
     * Contract 07 §3 (bug hunt 2026-10-29 JAVA-1): a matcher that throws anything fails its own
     * request with InvalidReplyException; the reader goes on and the device stays open.
     */
    public static void aMatcherThatThrowsFailsOnlyItsRequest() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        link.onPacket = p -> List.of(link.reply(Common.Rpt.TEXT.value, text(p.cmd(), "name")));
        try (DepzDevice dev = new DepzDevice(link, Duration.ofSeconds(2))) {
            List<RuntimeException> bugs = List.of(new NullPointerException("decode bug"),
                    new ClassCastException("decode bug"), new ArithmeticException("decode bug"),
                    new java.nio.BufferUnderflowException(), new DepzException("decode bug"));
            for (RuntimeException bug : bugs) {
                InvalidReplyException e = throwsType(InvalidReplyException.class,
                        () -> dev.request(Common.Cmd.GET_DEVICE_NAME.value, null, p -> {
                            throw bug;
                        }));
                eq(Common.Cmd.GET_DEVICE_NAME.value, e.cmd());
                check(Objects.equals(e.getCause(), bug), "the matcher's exception is the cause: " + e.getCause());
            }
            check(!dev.isClosed(), "the device stays open");
            eq("name", dev.getDeviceName());
        }
    }

    /** A device whose stream handler throws on report 0x9E — a decoder bug. */
    private static final class RaisingReports extends DepzDevice {
        RaisingReports(FakeBoardLink link) {
            super(link, SHORT);
        }

        @Override
        protected boolean handleReport(Packet packet) {
            if (packet.cmd() == 0x9E) {
                throw new IllegalStateException("stream decoder bug");
            }
            return false;
        }
    }

    /** Contract 07 §3: a report handler that throws drops that packet, counted; the reader goes on. */
    public static void aThrowingReportHandlerDropsThatPacket() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        link.onPacket = p -> List.of(link.reply(Common.Rpt.TEXT.value, text(p.cmd(), "still here")));
        Logger log = Logger.getLogger("ai.depz.sensor");
        Level was = log.getLevel();
        log.setLevel(Level.OFF); // the deliberate failure below, not test noise
        try (RaisingReports dev = new RaisingReports(link); Subscription<DeviceEvent> events = dev.events()) {
            link.inject(link.reply(0x9E, new byte[] {1, 2}));
            link.inject(link.reply(0xC5, new byte[] {(byte) 0xAB}));
            eq(new DeviceEvent.Text(0xC5, "ab"), events.take(Duration.ofSeconds(1)));
            eq(1L, dev.stats().reportErrors());
            eq("still here", dev.getDeviceName());
            check(!dev.isClosed(), "the device stays open");
        } finally {
            log.setLevel(was);
        }
    }

    /** A link whose read fails with something no link is expected to throw. */
    private static final class BrokenReadLink implements ai.depz.sensor.devices.Link {
        final FakeBoardLink inner = new FakeBoardLink();

        @Override
        public byte[] read(Duration timeout) {
            throw new UnsupportedOperationException("read broke");
        }

        @Override
        public void write(byte[] data) {
            inner.write(data);
        }

        @Override
        public void close() {
            inner.close();
        }

        @Override
        public boolean isClosed() {
            return inner.isClosed();
        }

        @Override
        public String name() {
            return "broken";
        }
    }

    /**
     * Contract 07 §3: a reader that ends for any reason other than close() closes the device —
     * isClosed() is true and a later request fails at once with DeviceLostException carrying the cause.
     */
    public static void aReaderThatEndsForAnyReasonClosesTheDevice() throws Exception {
        BrokenReadLink link = new BrokenReadLink();
        Logger log = Logger.getLogger("ai.depz.sensor");
        Level was = log.getLevel();
        log.setLevel(Level.OFF);
        try (DepzDevice dev = new DepzDevice(link, Duration.ofSeconds(5))) {
            check(T.until(dev::isClosed, 1000), "the device is closed");
            check(link.isClosed(), "the link is closed");
            long t0 = System.nanoTime();
            DeviceLostException e = throwsType(DeviceLostException.class, dev::getDeviceName);
            check(System.nanoTime() - t0 < 1_000_000_000L, "the request waited");
            check(e.getCause() instanceof UnsupportedOperationException, "cause: " + e.getCause());
            eq(0, link.inner.sentCount());
        } finally {
            log.setLevel(was);
        }
    }

    /**
     * Contract 07 §3: a callback that throws is counted and dropped — the other callback and the
     * queue still get the item, the reader goes on (a request completes, the next report is read).
     */
    public static void aThrowingCallbackNeverStopsTheReader() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        link.onPacket = p -> List.of(link.reply(Common.Rpt.TEXT.value, text(p.cmd(), "APP_usonic_SR04_v0.97")));
        Logger log = Logger.getLogger("ai.depz.sensor");
        Level was = log.getLevel();
        log.setLevel(Level.OFF); // the deliberate failures below, not test noise
        try (Sr04Device dev = new Sr04Device(link);
                Subscription<DeviceEvent> events = dev.events();
                Subscription<Sr04Measurement> loop = dev.measurements(4)) {
            List<DeviceEvent> seen = new ArrayList<>();
            dev.onEvent(e -> {
                throw new IllegalStateException("consumer bug");
            });
            dev.onEvent(e -> {
                synchronized (seen) {
                    seen.add(e);
                }
            });
            link.inject(link.reply(0xC5, new byte[] {(byte) 0xAB}));
            eq(new DeviceEvent.Text(0xC5, "ab"), events.take(Duration.ofSeconds(1)));
            synchronized (seen) {
                eq(List.of(new DeviceEvent.Text(0xC5, "ab")), seen);
            }
            eq(1L, dev.stats().callbackErrors());
            eq("APP_usonic_SR04_v0.97", dev.getSoftwareName());

            dev.onMeasurement(m -> {
                throw new AssertionError("consumer bug");
            });
            byte[] payload = new byte[11];
            payload[0] = (byte) Sr04.Sr04Cmd.START_MEASUREMENT_LOOP.value;
            payload[9] = 7;
            link.inject(link.reply(Sr04.Sr04Rpt.DATA.value, payload));
            eq(7, loop.take(Duration.ofSeconds(1)).echoTimeUs());
            eq(2L, dev.stats().callbackErrors());
        } finally {
            log.setLevel(was);
        }
    }

    public static void queuesDropTheOldestAndCount() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        try (Sr04Device dev = new Sr04Device(link)) {
            List<Integer> seen = new ArrayList<>();
            AutoCloseable cb = dev.onMeasurement(m -> {
                synchronized (seen) {
                    seen.add(m.echoTimeUs());
                }
            });
            Subscription<Sr04Measurement> q = dev.measurements(2);
            for (int i = 1; i <= 5; i++) {
                byte[] payload = new byte[11];
                payload[0] = (byte) Sr04.Sr04Cmd.START_MEASUREMENT_LOOP.value;
                payload[9] = (byte) i;
                link.inject(link.reply(Sr04.Sr04Rpt.DATA.value, payload));
            }
            T.until(() -> {
                synchronized (seen) {
                    return seen.size() == 5;
                }
            }, 2000);
            eq(3L, q.droppedCount());
            eq(List.of(3L), dev.streamDroppedCounts());
            eq(List.of(4, 5), q.drain().stream().map(Sr04Measurement::echoTimeUs).toList());
            eq(List.of(1, 2, 3, 4, 5), seen);
            check(q.tryTake().isEmpty(), "queue empty");
            cb.close();
            q.close();
        }
    }

    public static void recordingReplaysByteForByte() throws Exception {
        Path path = Files.createTempFile("depz-", ".depzrec");
        try {
            FakeBoardLink board = new FakeBoardLink();
            board.onPacket = p -> List.of(board.reply(Common.Rpt.TEXT.value, text(p.cmd(), "APP_usonic_SR04_v0.97")));
            try (DepzDevice rec = new DepzDevice(new RecordingLink(board, path, Map.of("note", "test")))) {
                eq("APP_usonic_SR04_v0.97", rec.getSoftwareName());
            }
            ReplayLink replay = new ReplayLink(path, true, false);
            eq("test", replay.header().get("note"));
            try (DepzDevice dev = new DepzDevice(replay)) {
                eq("APP_usonic_SR04_v0.97", dev.getSoftwareName());
                check(replay.rxExhausted(), "rx exhausted");
                eq(0L, replay.txRemaining());
                throwsType(ReplayMismatchException.class, () -> dev.send(Common.Cmd.GET_SERIAL.value, null));
            }
        } finally {
            Files.deleteIfExists(path);
        }
    }

    // ERRATA A6: there is no `.depzrec.gz` — a capture is plain text, a gzip
    // container is not inflated but refused like any damaged file. The gzipped
    // bytes (mtime 0) are the same 70 in every SDK's test.
    private static final byte[] GZIPPED_CAPTURE = HexFormat.of().parseHex(
            "1f8b0800000000000203ab562a4ece48cd4d54b252504a492da8d22b4a4dd63754aae5aa"
                    + "562a018a19e80085338b40b245154a204e6209586d6222500d007e3bc00b3d000000");

    public static void replayRefusesAGzippedCapture() throws Exception {
        Path path = Files.createTempFile("depz-", ".depzrec");
        try {
            byte[] plain;
            try (GZIPInputStream in = new GZIPInputStream(new ByteArrayInputStream(GZIPPED_CAPTURE))) {
                plain = in.readAllBytes();
            }
            Files.write(path, plain);
            eq("depz.rec/1", new ReplayLink(path, false, false).header().get("schema")); // a valid capture
            Files.write(path, GZIPPED_CAPTURE);
            throwsType(IllegalArgumentException.class, () -> new ReplayLink(path, false, false));
        } finally {
            Files.deleteIfExists(path);
        }
    }

    // Contract 08: an exhausted capture is a quiet port — a read that returned at once
    // made the reader thread spin a core for as long as the replayed device stayed open.
    public static void anExhaustedReplayWaitsOutTheReadTimeoutAndWakesOnClose() throws Exception {
        Path path = Files.createTempFile("depz-", ".depzrec");
        try {
            Files.writeString(path, "{\"schema\":\"depz.rec/1\"}\n");
            ReplayLink link = new ReplayLink(path, false, false);
            check(link.rxExhausted(), "nothing to deliver");
            long t0 = System.nanoTime();
            eq(0, link.read(Duration.ofMillis(100)).length);
            long ms = (System.nanoTime() - t0) / 1_000_000;
            check(ms >= 80, "returned after " + ms + " ms");

            Throwable[] outcome = new Throwable[1];
            Thread reader = new Thread(() -> {
                try {
                    link.read(Duration.ofSeconds(30));
                } catch (Throwable e) {
                    outcome[0] = e;
                }
            });
            reader.start();
            check(until(() -> reader.getState() == Thread.State.TIMED_WAITING, 5000), "reader waits");
            link.close();
            reader.join(5000);
            check(!reader.isAlive(), "close() wakes the read");
            check(outcome[0] instanceof LinkClosedException, "closed error: " + outcome[0]);
        } finally {
            Files.deleteIfExists(path);
        }
    }

    // Contract 08 «Line rules»: arrays and objects nest at most 256 deep; a 200 KB
    // line of `[[[[…` is a damaged line naming its number, not a StackOverflowError.
    public static void aLineNested100000DeepIsADamagedLine() throws Exception {
        String deep = "[".repeat(100_000) + "]".repeat(100_000);
        IllegalArgumentException e = throwsType(IllegalArgumentException.class, () -> Dataset.parse(
                "{\"schema\":\"depz.dataset/2\"}\n{\"d\":\"d0\",\"t\":1,\"k\":\"x\",\"v\":{\"a\":" + deep + "}}\n",
                JsonText::parse));
        check(e.getMessage().startsWith("line 2: not JSON: nested deeper than 256"), e.getMessage());
        Path path = Files.createTempFile("depz-", ".depzrec");
        try {
            Files.writeString(path, "{\"schema\":\"depz.rec/1\",\"x\":" + deep + "}\n");
            e = throwsType(IllegalArgumentException.class, () -> new ReplayLink(path, false, false));
            check(e.getMessage().startsWith(path + ":1: not JSON: nested deeper than 256"), e.getMessage());
        } finally {
            Files.deleteIfExists(path);
        }
    }

    public static void openOnLinkResolvesAndAttachesInPlace() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        link.onPacket = p -> {
            if (p.cmd() == Common.Cmd.GET_NAME_ACTIVE_SOFTWARE.value) {
                return List.of(link.reply(Common.Rpt.TEXT.value, text(p.cmd(), "APP_usonic_SR04_v0.97")));
            }
            if (p.cmd() == Common.Cmd.GET_DEVICE_NAME.value) {
                return List.of(FakeBoardLink.status(p.cmd(), Common.Status.ERR_CMD_NOT_SUPPORTED.value));
            }
            if (p.cmd() == Sr04.Sr04Cmd.GET_ECHO_DECAY.value) {
                return List.of(link.reply(Sr04.Sr04Rpt.ECHO_DECAY.value, new byte[] {(byte) 0x88, 0x13}));
            }
            return List.of();
        };
        try (DepzDevice dev = DepzDevices.openOnLink(link, null, null, SHORT)) {
            check(dev instanceof Sr04Device, "resolved " + dev.getClass().getSimpleName());
            eq(5000, ((Sr04Device) dev).getEchoDecayUs());
            eq(List.of(0, 1, 2, 3), link.sentSnapshot().stream().map(Packet::seq).toList());
        }
    }

    public static void openOnLinkRefusesBootloaderAndSilence() {
        FakeBoardLink boot = new FakeBoardLink();
        boot.onPacket = p -> List.of(boot.reply(Common.Rpt.TEXT.value, text(p.cmd(), "BOOTDEPZ_v1.11")));
        throwsType(BootloaderModeException.class, () -> DepzDevices.openOnLink(boot, null, null, SHORT));
        check(boot.isClosed(), "boot link closed");
        FakeBoardLink silent = new FakeBoardLink();
        throwsType(NoDepzDeviceException.class, () -> DepzDevices.openOnLink(silent, null, null, SHORT));
        check(silent.isClosed(), "silent link closed");
    }
}
