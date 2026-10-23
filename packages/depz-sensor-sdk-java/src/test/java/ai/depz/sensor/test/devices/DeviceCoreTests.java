package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.throwsType;

import ai.depz.sensor.devices.BootloaderModeException;
import ai.depz.sensor.devices.BusyException;
import ai.depz.sensor.devices.DepzDevice;
import ai.depz.sensor.devices.DepzDevices;
import ai.depz.sensor.devices.DepzException;
import ai.depz.sensor.devices.DepzTimeoutException;
import ai.depz.sensor.devices.DeviceEvent;
import ai.depz.sensor.devices.DeviceLostException;
import ai.depz.sensor.devices.LinkClosedException;
import ai.depz.sensor.devices.NoDepzDeviceException;
import ai.depz.sensor.devices.RecordingLink;
import ai.depz.sensor.devices.ReplayLink;
import ai.depz.sensor.devices.ReplayMismatchException;
import ai.depz.sensor.devices.Sr04Device;
import ai.depz.sensor.devices.Sr04Measurement;
import ai.depz.sensor.devices.StatusException;
import ai.depz.sensor.devices.Subscription;
import ai.depz.sensor.protocol.Common;
import ai.depz.sensor.protocol.Sr04;
import ai.depz.sensor.transport.CrcType;
import ai.depz.sensor.transport.Packet;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

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
            DepzException e = throwsType(DepzException.class, dev::readMcuTemperature);
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
            eq(0, link.sentCount());
        }
    }

    public static void unsolicitedReportsBecomeEvents() throws Exception {
        FakeBoardLink link = new FakeBoardLink();
        try (DepzDevice dev = new DepzDevice(link); Subscription<DeviceEvent> events = dev.events()) {
            link.inject(FakeBoardLink.status(0, Common.Status.ERR_HARDWARE_FAULT.value));
            link.inject(link.reply(Common.Rpt.SEQUENCE_ERROR.value, new byte[] {7, 9}));
            link.inject(link.reply(0xC5, new byte[] {(byte) 0xAB}));
            DeviceEvent a = events.take(Duration.ofSeconds(1));
            check(a instanceof DeviceEvent.UnsolicitedStatus u && u.isHardwareFault(), "hardware fault: " + a);
            eq(new DeviceEvent.SequenceError(7, 9, true), events.take(Duration.ofSeconds(1)));
            eq(new DeviceEvent.Text(0xC5, "ab"), events.take(Duration.ofSeconds(1)));
            eq(1L, dev.stats().deviceSeqErrors());
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
