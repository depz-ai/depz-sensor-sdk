package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.throwsType;

import ai.depz.sensor.devices.Bno086Device;
import ai.depz.sensor.devices.BusyException;
import ai.depz.sensor.devices.DepzTimeoutException;
import ai.depz.sensor.devices.Subscription;
import ai.depz.sensor.protocol.Bno086;
import ai.depz.sensor.protocol.Common;
import ai.depz.sensor.sensors.bno086.Reports;
import ai.depz.sensor.sensors.bno086.Sh2;
import ai.depz.sensor.sensors.bno086.Sh2Hub;
import ai.depz.sensor.sensors.bno086.Sh2Hub.Counts;
import ai.depz.sensor.sensors.bno086.Sh2Hub.ErrorRecord;
import ai.depz.sensor.sensors.bno086.Sh2Hub.ProductId;
import ai.depz.sensor.sensors.bno086.Sh2Hub.Sh2Exception;
import ai.depz.sensor.sensors.bno086.Shtp;
import ai.depz.sensor.transport.Packet;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.function.BiFunction;

/** The BNO085 / BNO086 device class over a fake SH-2 hub. */
public final class Bno086DeviceTests {
    static final int CONTROL = Sh2Hub.CHANNEL_CONTROL;

    record Cargo(int channel, byte[] bytes) {}

    /**
     * A SH-2 hub behind the SHTP pass-through bridge: every SEND_SHTP_PACKET is acknowledged, its
     * cargo handed to {@code onCargo}, and the cargos that returns come back as RPT_DATA frames.
     * {@code busyFirst} answers that many sends with ERR_BUSY first.
     */
    static final class FakeHub {
        final FakeBoardLink link = new FakeBoardLink();
        final List<Cargo> sent = new ArrayList<>();
        volatile BiFunction<Integer, byte[], List<Cargo>> onCargo;
        volatile int busyFirst;
        private final int[] seq = new int[6];

        FakeHub() {
            link.onPacket = this::onPacket;
        }

        private List<byte[]> onPacket(Packet p) {
            if (p.cmd() != Bno086.Bno086Cmd.SEND_SHTP_PACKET.value) {
                return List.of(FakeBoardLink.status(p.cmd(), Common.Status.OK.value));
            }
            if (busyFirst > 0) {
                busyFirst--;
                return List.of(FakeBoardLink.status(p.cmd(), Common.Status.ERR_BUSY.value));
            }
            int channel = p.payload()[2];
            byte[] cargo = Arrays.copyOfRange(p.payload(), 4, p.payload().length);
            synchronized (sent) {
                sent.add(new Cargo(channel, cargo));
            }
            List<byte[]> replies = new ArrayList<>();
            replies.add(FakeBoardLink.status(p.cmd(), Common.Status.OK.value));
            BiFunction<Integer, byte[], List<Cargo>> f = onCargo;
            if (f != null) {
                for (Cargo c : f.apply(channel, cargo)) {
                    replies.add(data(c.channel(), c.bytes()));
                }
            }
            return replies;
        }

        synchronized byte[] data(int channel, byte[] cargo) {
            byte[] frame = Shtp.buildFrame(channel, cargo, seq[channel]++);
            byte[] payload = new byte[9 + frame.length];
            putU32(payload, 1, 1_000_000);
            System.arraycopy(frame, 0, payload, 9, frame.length);
            return link.reply(Bno086.Bno086Rpt.DATA.value, payload);
        }

        List<byte[]> sentWith(int reportId) {
            List<byte[]> out = new ArrayList<>();
            synchronized (sent) {
                for (Cargo c : sent) {
                    if ((c.bytes()[0] & 0xFF) == reportId) {
                        out.add(c.bytes());
                    }
                }
            }
            return out;
        }
    }

    static void putU32(byte[] b, int off, long v) {
        for (int i = 0; i < 4; i++) {
            b[off + i] = (byte) (v >> (8 * i));
        }
    }

    static long u32(byte[] b, int off) {
        return (b[off] & 0xFFL) | (b[off + 1] & 0xFFL) << 8 | (b[off + 2] & 0xFFL) << 16 | (b[off + 3] & 0xFFL) << 24;
    }

    static byte[] feature(int sensor, long intervalUs) {
        byte[] r = new byte[17];
        r[0] = (byte) Sh2.GET_FEATURE_RESPONSE;
        r[1] = (byte) sensor;
        putU32(r, 5, intervalUs);
        return r;
    }

    static byte[] frsRead(int status, int len, int offset, long d0, long d1, int type) {
        byte[] r = new byte[16];
        r[0] = (byte) Sh2.FRS_READ_RESPONSE;
        r[1] = (byte) (len << 4 | status);
        r[2] = (byte) offset;
        r[3] = (byte) (offset >> 8);
        putU32(r, 4, d0);
        putU32(r, 8, d1);
        r[12] = (byte) type;
        r[13] = (byte) (type >> 8);
        return r;
    }

    static byte[] frsWrite(int status, int offset) {
        return new byte[] {(byte) Sh2.FRS_WRITE_RESPONSE, (byte) status, (byte) offset, (byte) (offset >> 8)};
    }

    static Cargo control(byte[] bytes) {
        return new Cargo(CONTROL, bytes);
    }

    static Bno086Device open(FakeHub hub) {
        Bno086Device dev = new Bno086Device(hub.link, Duration.ofMillis(500));
        dev.setBusyBackoff(Duration.ofMillis(1));
        return dev;
    }

    public static void aBusyBridgeIsRetriedThenGivenUp() {
        FakeHub hub = new FakeHub();
        try (Bno086Device dev = open(hub)) {
            hub.onCargo = (ch, c) -> List.of(control(feature(c[1], 10000)));
            hub.busyFirst = 2;
            eq(10000L, dev.getFeature(5).intervalUs());
            hub.busyFirst = 5;
            throwsType(BusyException.class, () -> dev.getFeature(5));
        }
    }

    public static void enableSkipsAStaleDisabledReadBack() {
        FakeHub hub = new FakeHub();
        try (Bno086Device dev = open(hub)) {
            int[] reads = {0};
            hub.onCargo = (ch, c) -> (c[0] & 0xFF) == Sh2.GET_FEATURE_REQUEST
                    ? List.of(control(feature(c[1], reads[0]++ < 2 ? 0 : 10000)))
                    : List.of();
            Sh2Hub.FeatureResponse granted = dev.enable(Reports.ROTATION_VECTOR, 100);
            eq(10000L, granted.intervalUs());
            eq(3, reads[0]);
            byte[] setFeature = hub.sentWith(Sh2.SET_FEATURE_COMMAND).get(0);
            eq(10000L, u32(setFeature, 5));
            throwsType(IllegalArgumentException.class, () -> dev.enable(5, 0));
            throwsType(IllegalArgumentException.class, () -> dev.enable(5, Double.NaN));
        }
    }

    public static void frsReadCollectsEveryWordOfTheRecord() {
        FakeHub hub = new FakeHub();
        try (Bno086Device dev = open(hub)) {
            hub.onCargo = (ch, c) -> List.of(
                    control(frsRead(Sh2Hub.FRS_NO_ERROR, 2, 0, 11, 22, Sh2Hub.FRS_USER_RECORD)),
                    control(frsRead(Sh2Hub.FRS_READ_COMPLETED, 1, 2, 33, 0, Sh2Hub.FRS_USER_RECORD)));
            eq(new long[] {11, 22, 33}, dev.frsRead(Sh2Hub.FRS_USER_RECORD));
            hub.onCargo = (ch, c) -> List.of(control(frsRead(Sh2Hub.FRS_RECORD_EMPTY, 0, 0, 0, 0, Sh2Hub.FRS_SERIAL_NUMBER)));
            throwsType(Sh2Exception.class, () -> dev.frsRead(Sh2Hub.FRS_SERIAL_NUMBER));
        }
    }

    public static void frsWriteSendsTheWordsTwoAtATime() {
        FakeHub hub = new FakeHub();
        try (Bno086Device dev = open(hub)) {
            hub.onCargo = (ch, c) -> {
                int id = c[0] & 0xFF;
                if (id == Sh2.FRS_WRITE_REQUEST) {
                    return List.of(control(frsWrite(Sh2Hub.FRS_WRITE_MODE_READY, 0)));
                }
                if (id == Sh2.FRS_WRITE_DATA) {
                    return List.of(control(c[2] == 2 ? frsWrite(Sh2Hub.FRS_WRITE_COMPLETED, 2)
                            : frsWrite(Sh2Hub.FRS_WRITE_WORDS_RECEIVED, c[2])));
                }
                return List.of();
            };
            dev.frsWrite(Sh2Hub.FRS_USER_RECORD, new long[] {1, 2, 3});
            List<byte[]> data = hub.sentWith(Sh2.FRS_WRITE_DATA);
            eq(2, data.size());
            eq(List.of(1L, 2L), List.of(u32(data.get(0), 4), u32(data.get(0), 8)));
            eq(List.of(2, 3L), List.of((int) data.get(1)[2], u32(data.get(1), 4)));
        }
    }

    public static void flushReturnsWhenFlushCompletedArrives() {
        FakeHub hub = new FakeHub();
        try (Bno086Device dev = open(hub)) {
            hub.onCargo = (ch, c) -> (c[0] & 0xFF) == Sh2Hub.FORCE_SENSOR_FLUSH
                    ? List.of(new Cargo(Sh2Hub.CHANNEL_INPUT_NORMAL, new byte[] {(byte) 0xEF, c[1]}))
                    : List.of();
            dev.flush(Reports.ACCELEROMETER);
            hub.onCargo = (ch, c) -> List.of();
            throwsType(DepzTimeoutException.class, () -> dev.flush(Reports.GYROSCOPE, Duration.ofMillis(50)));
        }
    }

    static byte[] pid(long part) {
        byte[] b = new byte[16];
        b[0] = (byte) 0xF8;
        b[1] = 4;
        b[2] = 3;
        b[3] = 12;
        putU32(b, 4, part);
        return b;
    }

    public static void productIdsCollectEveryComponent() {
        FakeHub hub = new FakeHub();
        try (Bno086Device dev = open(hub)) {
            byte[] two = new byte[32]; // one cargo, two responses
            System.arraycopy(pid(10004563), 0, two, 0, 16);
            System.arraycopy(pid(10003606), 0, two, 16, 16);
            hub.onCargo = (ch, c) -> List.of(control(two));
            List<ProductId> ids = dev.productIds(null, Duration.ofMillis(30));
            eq(List.of(10004563L, 10003606L), ids.stream().map(ProductId::swPartNumber).toList());
            eq("3.12.0", ids.get(0).version());
        }
    }

    static byte[] resp(int command, int cmdSeq, int respSeq, int... r) {
        byte[] b = new byte[16];
        b[0] = (byte) 0xF1;
        b[2] = (byte) command;
        b[3] = (byte) cmdSeq;
        b[4] = (byte) respSeq;
        for (int i = 0; i < r.length; i++) {
            b[5 + i] = (byte) r[i];
        }
        return b;
    }

    public static void errorsAndCountsCollectTheirResponses() {
        FakeHub hub = new FakeHub();
        try (Bno086Device dev = open(hub)) {
            hub.onCargo = (ch, c) -> switch (c[2]) {
                case Sh2Hub.CMD_ERRORS -> List.of(
                        control(resp(1, c[1], 0, 1, 7, 3, 9, 2, 4)),
                        control(resp(1, c[1], 1, 0, 0, 255)));
                case Sh2Hub.CMD_COUNTER -> List.of(
                        control(resp(2, c[1], 0, 0, 0, 0, 5, 0, 0, 0, 6, 0, 0, 0)),
                        control(resp(2, c[1], 1, 0, 0, 0, 7, 0, 0, 0, 8, 0, 0, 0)));
                default -> List.of();
            };
            eq(List.of(new ErrorRecord(1, 7, 3, 9, 2, 4)), dev.getErrors());
            eq(new Counts(5, 5, 6, 7, 8), dev.getCounts(5));
        }
    }

    public static void reportsAreFilteredBySensor() {
        FakeHub hub = new FakeHub();
        try (Bno086Device dev = open(hub);
                Subscription<Reports.Report> accel = dev.reports(List.of(Reports.ACCELEROMETER), 16);
                Subscription<Reports.Report> all = dev.reports()) {
            byte[] cargo = {
                (byte) 0xFB, 0, 0, 0, 0,
                0x01, 0, 0, 0, 1, 0, 2, 0, 3, 0,   // accelerometer
                0x02, 0, 0, 0, 1, 0, 2, 0, 3, 0,   // gyroscope
            };
            hub.link.inject(hub.data(Sh2Hub.CHANNEL_INPUT_NORMAL, cargo));
            eq("Acceleration", accel.take(Duration.ofSeconds(1)).type());
            eq("Acceleration", all.take(Duration.ofSeconds(1)).type());
            eq("Gyroscope", all.take(Duration.ofSeconds(1)).type());
            check(accel.tryTake().isEmpty(), "the gyroscope report passed the accelerometer filter");
        }
    }

    public static void reorientationParamsAreQ14AndChecked() {
        eq(new byte[] {2, 0, 0, 0, 0, 0x00, 0x20, 0x00, 0x40}, Sh2Hub.setReorientationParams(0, 0, 0.5, 1));
        throwsType(IllegalArgumentException.class, () -> Sh2Hub.setReorientationParams(2, 0, 0, 0));
        throwsType(IllegalArgumentException.class, () -> Sh2Hub.tareNowParams(256, Sh2Hub.TareBasis.ROTATION_VECTOR));
    }
}
