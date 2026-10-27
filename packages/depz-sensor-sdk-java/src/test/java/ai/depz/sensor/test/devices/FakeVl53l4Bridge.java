package ai.depz.sensor.test.devices;

import ai.depz.sensor.protocol.Vl53l4;
import ai.depz.sensor.sensors.vl53l4.Vl53l4Uld;
import ai.depz.sensor.transport.Packet;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * The VL53L4 bridge firmware in-process (contract 10) over a {@link FakeBoardLink}, registers backed
 * by {@link Vl53l4cdDriverTests.FakePlatform}: the Python tests' {@code FakeVl53l4cd}. READ_REG /
 * WRITE_REG / XSHUT / START_STREAM / STOP_STREAM / GET_INFO / SET_I2C_SPEED are answered here;
 * {@link #sendResult} emits one RPT_VL53_STREAM as the INT edge would. {@link #failNext} answers the
 * next {@code cmd} with an error status ({@link #SILENT}: no answer at all); {@link #shortReadNext}
 * drops the last byte of the next REG_DATA. XSHUT OFF powers the sensor down (register access answers
 * ERR_HARDWARE_FAULT, §2); RESET wipes the register file back to the boot values. {@link #rx} logs
 * every bridge command received. The host calls answer on its own writing thread, under this lock.
 */
final class FakeVl53l4Bridge {
    static final int READ_REG = Vl53l4.Vl53l4Cmd.READ_REG.value;
    static final int WRITE_REG = Vl53l4.Vl53l4Cmd.WRITE_REG.value;
    static final int XSHUT = Vl53l4.Vl53l4Cmd.XSHUT.value;
    static final int START_STREAM = Vl53l4.Vl53l4Cmd.START_STREAM.value;
    static final int STOP_STREAM = Vl53l4.Vl53l4Cmd.STOP_STREAM.value;
    static final int GET_INFO = Vl53l4.Vl53l4Cmd.GET_INFO.value;
    static final int SET_I2C_SPEED = Vl53l4.Vl53l4Cmd.SET_I2C_SPEED.value;
    static final int ERR_HARDWARE_FAULT = 0x09;
    static final int SILENT = -1;

    final FakeBoardLink link = new FakeBoardLink();
    Vl53l4cdDriverTests.FakePlatform platform = new Vl53l4cdDriverTests.FakePlatform();
    boolean streaming;
    int streamAddr;
    int streamLen;
    boolean powered = true; // XSHUT level
    int intEdges;
    int i2cErrors;
    boolean shortReadNext;
    long mcuTimeUs = 1_000_000;
    /** Every bridge command the host sent, in order ({cmd, payload}). */
    final List<Object[]> rx = new ArrayList<>();
    private final Map<Integer, ArrayDeque<Integer>> failures = new HashMap<>();

    FakeVl53l4Bridge() {
        link.onPacket = this::handle;
    }

    synchronized void failNext(int cmd, int status) {
        failures.computeIfAbsent(cmd, k -> new ArrayDeque<>()).add(status);
    }

    synchronized List<Integer> cmds() {
        List<Integer> out = new ArrayList<>();
        for (Object[] r : rx) {
            out.add((Integer) r[0]);
        }
        return out;
    }

    /** The payloads of every received {@code cmd}. */
    synchronized List<byte[]> sent(int cmd) {
        List<byte[]> out = new ArrayList<>();
        for (Object[] r : rx) {
            if ((Integer) r[0] == cmd) {
                out.add((byte[]) r[1]);
            }
        }
        return out;
    }

    synchronized Object[] lastRx() {
        return rx.get(rx.size() - 1);
    }

    synchronized void clearRx() {
        rx.clear();
    }

    /** Place a 17-byte result block at 0x0089 (what GetResult / the stream reads), without a host write. */
    void setResultBlock(byte[] raw) {
        for (int i = 0; i < raw.length; i++) {
            platform.reg.put(Vl53l4Uld.RESULT_BLOCK_ADDR + i, raw[i]);
        }
    }

    int i2cKhz() {
        List<Integer> s = platform.speeds;
        return s.isEmpty() ? Vl53l4Uld.I2C_KHZ_BOOT : s.get(s.size() - 1);
    }

    /** A raw 17-byte result block (big-endian registers 0x0089..0x0099); {@code status} is the raw
     *  RESULT__RANGE_STATUS (9 → ULD status 0 "valid"), {@code spads} the raw RESULT__SPAD_NB word. */
    static byte[] resultBlock(int distanceMm, int status, int sigmaMm, int signalKcps, int spads, int streamCount) {
        ByteBuffer b = ByteBuffer.allocate(17);
        b.put(0, (byte) status);
        b.put(2, (byte) streamCount);
        b.putShort(3, (short) spads);
        b.putShort(5, (short) (signalKcps / 8));
        b.putShort(9, (short) (sigmaMm * 4));
        b.putShort(13, (short) distanceMm);
        return b.array();
    }

    static byte[] resultBlock(int distanceMm) {
        return resultBlock(distanceMm, 9, 3, 1200, 2048, 1);
    }

    /** Emit one streamed result block; returns the raw block. */
    synchronized byte[] sendResult(byte[] raw) {
        intEdges++;
        mcuTimeUs += 30_000;
        sendStream(raw, streamAddr, raw.length);
        return raw;
    }

    synchronized void sendStream(byte[] raw, int addr, int lenField) {
        ByteBuffer p = ByteBuffer.allocate(12 + raw.length).order(ByteOrder.LITTLE_ENDIAN);
        p.putLong(mcuTimeUs).putShort((short) addr).putShort((short) lenField).put(raw);
        link.inject(link.reply(Vl53l4.Vl53l4Rpt.STREAM.value, p.array()));
    }

    private synchronized List<byte[]> handle(Packet pkt) {
        int cmd = pkt.cmd();
        byte[] pl = pkt.payload();
        if (cmd < READ_REG) {
            return List.of(FakeBoardLink.status(cmd, 0x00)); // the common commands: not modelled
        }
        rx.add(new Object[] {cmd, pl.clone()});
        ArrayDeque<Integer> queued = failures.get(cmd);
        if (queued != null && !queued.isEmpty()) {
            int fail = queued.poll();
            return fail == SILENT ? List.of() : List.of(FakeBoardLink.status(cmd, fail));
        }
        ByteBuffer in = ByteBuffer.wrap(pl).order(ByteOrder.LITTLE_ENDIAN);
        if ((cmd == READ_REG || cmd == WRITE_REG) && !powered) {
            i2cErrors++; // an unpowered sensor NACKs (contract 10 §2)
            return List.of(FakeBoardLink.status(cmd, ERR_HARDWARE_FAULT));
        }
        if (cmd == READ_REG) {
            int addr = in.getShort(0) & 0xFFFF;
            int n = in.getShort(2) & 0xFFFF;
            byte[] data = platform.readMulti(addr, n);
            if (shortReadNext) {
                shortReadNext = false;
                data = Arrays.copyOf(data, data.length - 1);
            }
            ByteBuffer r = ByteBuffer.allocate(9 + data.length).order(ByteOrder.LITTLE_ENDIAN);
            r.put((byte) cmd).putLong(mcuTimeUs).put(data);
            mcuTimeUs += 100;
            return List.of(link.reply(Vl53l4.Vl53l4Rpt.REG_DATA.value, r.array()));
        }
        if (cmd == WRITE_REG) {
            platform.writeMulti(in.getShort(0) & 0xFFFF, Arrays.copyOfRange(pl, 2, pl.length));
            return List.of(FakeBoardLink.status(cmd, 0x00));
        }
        if (cmd == XSHUT) {
            int action = pl[0] & 0xFF;
            if (action == 0) {
                powered = false;
                streaming = false;
            } else if (action == 1) {
                powered = true;
            } else { // RESET: boot values only, the ULD configuration is gone
                platform = new Vl53l4cdDriverTests.FakePlatform();
                powered = true;
                streaming = false;
            }
            return List.of(FakeBoardLink.status(cmd, 0x00));
        }
        if (cmd == START_STREAM) {
            streamAddr = in.getShort(0) & 0xFFFF;
            streamLen = in.getShort(2) & 0xFFFF;
            streaming = true;
            return List.of(FakeBoardLink.status(cmd, 0x00));
        }
        if (cmd == STOP_STREAM) {
            streaming = false;
            return List.of(FakeBoardLink.status(cmd, 0x00));
        }
        if (cmd == GET_INFO) {
            int modelId = 0;
            if (powered) {
                byte[] m = platform.readMulti(Vl53l4Uld.IDENTIFICATION_MODEL_ID, 2);
                modelId = (m[0] & 0xFF) << 8 | (m[1] & 0xFF);
            }
            ByteBuffer r = ByteBuffer.allocate(21).order(ByteOrder.LITTLE_ENDIAN);
            r.putInt(intEdges).putInt(0).putInt(i2cErrors).put((byte) (i2cErrors != 0 ? 1 : 0))
                    .putShort((short) modelId)
                    .put(platform.reg.getOrDefault(Vl53l4Uld.FIRMWARE_SYSTEM_STATUS, (byte) 0))
                    .put((byte) (modelId == Vl53l4Uld.MODEL_ID ? 1 : 0)).put((byte) (powered ? 1 : 0))
                    .put((byte) (platform.reg.getOrDefault(Vl53l4Uld.GPIO_TIO_HV_STATUS, (byte) 0) & 1))
                    .putShort((short) i2cKhz());
            return List.of(link.reply(Vl53l4.Vl53l4Rpt.INFO.value, r.array()));
        }
        if (cmd == SET_I2C_SPEED) {
            platform.setI2cSpeed(in.getShort(0) & 0xFFFF);
            return List.of(FakeBoardLink.status(cmd, 0x00));
        }
        return List.of(FakeBoardLink.status(cmd, 0x02)); // ERR_INVALID_CMD
    }
}
