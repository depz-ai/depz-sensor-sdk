package ai.depz.sensor.test.devices;

import ai.depz.sensor.protocol.Common;
import ai.depz.sensor.protocol.Vl53l7;
import ai.depz.sensor.protocol.Vl53l8;
import ai.depz.sensor.transport.Packet;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

/**
 * The VL53L8 SPI bridge ({@code APP_VL53L8}, contract 04) or the L5/L7 I2C board ({@code APP_VL53L7},
 * contract 11) in-process, over a {@link FakeBoardLink}: READ_REG / WRITE_REG land on a
 * {@link Vl53l8DriverTests.FakePlatform} (the emulated ULD transport the driver tests run on), so the
 * device classes' init, configuration and ranging run end to end over the wire. The I2C board adds
 * PIN_CTRL, GET_INFO, SET_I2C_SPEED, its transfer limits and the RPT_VL53_INFO counters — the Python
 * tests' {@code fake_vl53l7.py} board logic.
 */
final class FakeVl53l8Board {
    static final String L8_SOFTWARE = "APP_VL53L8_v0.92";
    static final String L8_DEVICE_NAME = "DEPZ ToF Sensor VL53L8 USB v2.1 SER8";
    static final String L7_SOFTWARE = "APP_VL53L7_v0.53";
    static final String L7_DEVICE_NAME = "DEPZ ToF Sensor VL53L7CX USB v2.1 TXK5KAX6X4";

    static final int READ_REG = 0x32;
    static final int WRITE_REG = 0x33;
    static final int PIN_CTRL = 0x34;
    static final int START_STREAM = 0x35;
    static final int STOP_STREAM = 0x36;
    static final int GET_INFO = 0x37;
    static final int SET_I2C_SPEED = 0x38;
    static final int SILENT = -1;

    final FakeBoardLink link = new FakeBoardLink();
    final Vl53l8DriverTests.FakePlatform p;
    final boolean i2c;
    final String softwareName;
    final String deviceName;
    volatile long mcuTimeUs = 1_000_000;
    /** Every host packet, in order ({cmd, payload}). */
    final List<Object[]> rx = new ArrayList<>();
    private final Map<Integer, ArrayDeque<Integer>> failures = new HashMap<>();

    // The I2C board's state (contract 11 §2).
    volatile boolean streaming;
    volatile boolean lpn = true;
    volatile int frameSize;
    final AtomicLong intEdges = new AtomicLong();
    volatile long framesDropped;
    volatile long i2cErrors;
    volatile int lastI2cError = Vl53l7.I2C_ERR_OK;
    volatile int i2cKhz = 1000; // the firmware default since v0.52
    final List<Integer> pinActions = new ArrayList<>();
    final List<Integer> speedRequests = new ArrayList<>();
    final AtomicInteger chunksSent = new AtomicInteger();
    /** Firmware bytes written per bank page (0x09..0x0B): the banks are counted, not stored (as the platform fake). */
    final Map<Integer, Integer> bankBytes = new HashMap<>();
    private int page = -1;

    FakeVl53l8Board(Vl53l8DriverTests.FakePlatform platform, boolean i2cBoard) {
        this.p = platform;
        this.i2c = i2cBoard;
        this.softwareName = i2cBoard ? L7_SOFTWARE : L8_SOFTWARE;
        this.deviceName = i2cBoard ? L7_DEVICE_NAME : L8_DEVICE_NAME;
        link.onPacket = this::handle;
    }

    synchronized void failNext(int cmd, int status) {
        failures.computeIfAbsent(cmd, k -> new ArrayDeque<>()).addLast(status);
    }

    List<Integer> cmds() {
        List<Integer> out = new ArrayList<>();
        synchronized (rx) {
            for (Object[] e : rx) {
                out.add((int) e[0]);
            }
        }
        return out;
    }

    void clearRx() {
        synchronized (rx) {
            rx.clear();
        }
    }

    /** One INT edge → one frame in chunks of at most {@code chunkMax} bytes (RPT_VL53_FRAME). */
    void sendFrame(byte[] raw, long tsUs, int chunkMax) {
        intEdges.incrementAndGet();
        for (int off = 0; off < raw.length; off += chunkMax) {
            int n = Math.min(chunkMax, raw.length - off);
            sendChunk(raw, tsUs, off, n, raw.length);
        }
    }

    void sendChunk(byte[] raw, long tsUs, int off, int n, int fullSize) {
        ByteBuffer b = ByteBuffer.allocate(12 + n).order(ByteOrder.LITTLE_ENDIAN);
        b.putLong(tsUs).putShort((short) fullSize).putShort((short) off).put(raw, off, n);
        link.inject(link.reply(Vl53l8.Vl53l8Rpt.VL53_FRAME.value, b.array()));
        chunksSent.incrementAndGet();
    }

    byte[] infoPayload() {
        ByteBuffer b = ByteBuffer.allocate(Vl53l7.INFO_SIZE).order(ByteOrder.LITTLE_ENDIAN);
        b.putInt((int) intEdges.get()).putInt((int) framesDropped).putInt((int) i2cErrors).put((byte) lastI2cError)
                .put((byte) (lpn ? 1 : 0)).put((byte) (streaming ? 1 : 0)).putShort((short) i2cKhz).putShort((short) frameSize)
                .put((byte) (streaming ? 1 : 0));
        return b.array();
    }

    private byte[] status(int cmd, Common.Status s) {
        return FakeBoardLink.status(cmd, s.value);
    }

    private byte[] text(int cmd, String s) {
        byte[] t = s.getBytes(StandardCharsets.US_ASCII);
        byte[] out = new byte[t.length + 2];
        out[0] = (byte) cmd;
        System.arraycopy(t, 0, out, 1, t.length);
        return link.reply(Common.Rpt.TEXT.value, out);
    }

    private synchronized List<byte[]> handle(Packet pkt) {
        int cmd = pkt.cmd();
        byte[] pl = pkt.payload();
        synchronized (rx) {
            rx.add(new Object[] {cmd, pl});
        }
        ArrayDeque<Integer> q = failures.get(cmd);
        if (q != null && !q.isEmpty()) {
            int fail = q.pollFirst();
            return fail == SILENT ? List.of() : List.of(FakeBoardLink.status(cmd, fail));
        }
        if (cmd == Common.Cmd.GET_NAME_ACTIVE_SOFTWARE.value) {
            return List.of(text(cmd, softwareName));
        }
        if (cmd == Common.Cmd.GET_DEVICE_NAME.value) {
            return List.of(text(cmd, deviceName));
        }
        if (cmd == Common.Cmd.GET_SERIAL.value) {
            return List.of(text(cmd, "SN000007"));
        }
        if (cmd == READ_REG || cmd == WRITE_REG) {
            int addr = u16(pl, 0);
            if (i2c) {
                int n = cmd == READ_REG ? u16(pl, 2) : pl.length - 2;
                int max = cmd == READ_REG ? Vl53l7.READ_MAX_LEN : Vl53l7.WRITE_MAX_LEN;
                if (n < 1 || n > max || addr + n > 0x10000) {
                    return List.of(status(cmd, Common.Status.ERR_INVALID_PARAM));
                }
                if (!lpn) { // LPn low: the sensor NACKs
                    mcuTimeUs += 120;
                    i2cErrors++;
                    lastI2cError = Vl53l7.I2C_ERR_NACK;
                    return List.of(status(cmd, Common.Status.ERR_HARDWARE_FAULT));
                }
            }
            if (cmd == READ_REG) {
                byte[] data = p.readMulti(addr, u16(pl, 2));
                ByteBuffer b = ByteBuffer.allocate(9 + data.length).order(ByteOrder.LITTLE_ENDIAN);
                b.put((byte) cmd).putLong(mcuTimeUs).put(data);
                mcuTimeUs += 100;
                return List.of(link.reply(Vl53l8.Vl53l8Rpt.REG_DATA.value, b.array()));
            }
            byte[] data = Arrays.copyOfRange(pl, 2, pl.length);
            if (addr == 0x7FFF && data.length == 1) {
                page = data[0] & 0xFF;
            }
            if (page == 0 && addr == 0x0009 && data.length == 1 && data[0] == 0x04) {
                p.clearPending(); // the SW reboot that opens every init: the sensor forgets its pending command
            }
            if (page >= 0x09 && page <= 0x0B && data.length >= 1024) { // a firmware bank, in the host's write chunks
                bankBytes.merge(page, data.length, Integer::sum);
                return List.of(status(cmd, Common.Status.OK));
            }
            p.writeMulti(addr, data);
            return List.of(status(cmd, Common.Status.OK));
        }
        if (cmd == START_STREAM) {
            frameSize = u16(pl, 0);
            streaming = true;
            return List.of(status(cmd, Common.Status.OK));
        }
        if (cmd == STOP_STREAM) {
            streaming = false;
            return List.of(status(cmd, Common.Status.OK));
        }
        if (i2c && cmd == PIN_CTRL) {
            if (pl.length != 1) {
                return List.of(status(cmd, Common.Status.ERR_PAYLOAD_FORMAT));
            }
            int action = pl[0] & 0xFF;
            if (action > 3) {
                return List.of(status(cmd, Common.Status.ERR_INVALID_PARAM));
            }
            pinActions.add(action);
            if (action == Vl53l7.PinAction.LPN_OFF.value) {
                streaming = false;
                lpn = false;
            } else if (action == Vl53l7.PinAction.LPN_ON.value) {
                lpn = true;
            } else if (action == Vl53l7.PinAction.SOFT_CYCLE.value) {
                streaming = false;
                lpn = true;
                i2cErrors = 0;
                lastI2cError = Vl53l7.I2C_ERR_OK;
            }
            return List.of(status(cmd, Common.Status.OK));
        }
        if (i2c && cmd == GET_INFO) { // bridge state only, no echoed command byte
            return List.of(link.reply(Vl53l7.Vl53l7Rpt.VL53_INFO.value, infoPayload()));
        }
        if (i2c && cmd == SET_I2C_SPEED) {
            if (pl.length != 2) {
                return List.of(status(cmd, Common.Status.ERR_PAYLOAD_FORMAT));
            }
            int khz = u16(pl, 0);
            speedRequests.add(khz);
            if (streaming) {
                return List.of(status(cmd, Common.Status.ERR_BUSY));
            }
            int best = Vl53l7.I2C_SPEED_STEPS_KHZ[0];
            for (int s : Vl53l7.I2C_SPEED_STEPS_KHZ) {
                if (Math.abs(s - khz) < Math.abs(best - khz)) {
                    best = s;
                }
            }
            i2cKhz = best;
            return List.of(status(cmd, Common.Status.OK));
        }
        return List.of(status(cmd, Common.Status.ERR_INVALID_CMD));
    }

    static int u16(byte[] b, int off) {
        return (b[off] & 0xFF) | (b[off + 1] & 0xFF) << 8;
    }
}
