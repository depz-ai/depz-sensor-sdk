package ai.depz.sensor.test.devices;

import ai.depz.sensor.transport.Packet;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.HexFormat;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * The {@code APP_BNO055} register bridge and a register-level model of the BNO055 behind it (contract
 * 13), over a {@link FakeBoardLink}: the Python tests' {@code fake_bno055.py}. The bridge answers
 * READ_REG (RPT_BNO_REG_DATA or an RPT_STATUS error), WRITE_REG / RESET / START_STREAM / STOP_STREAM
 * (RPT_STATUS) and GET_INFO (RPT_BNO_INFO). The sensor side models what the host logic depends on:
 * two register pages selected by PAGE_ID, "only CONFIG ↔ mode transitions exist" (§4.1), suspend
 * ignoring OPR_MODE writes (§4.6), the zero-quaternion gap after entering fusion (§5), the self-test
 * leftover SYS_STATUS 4 (§5), SYS_TRIGGER CLK_SEL and INT_STA clear-on-read.
 */
final class FakeBno055 {
    static final int OPR_MODE = 0x3D;
    static final int PWR_MODE = 0x3E;
    static final int SYS_TRIGGER = 0x3F;
    static final int SYS_STATUS = 0x39;
    static final int SYS_ERR = 0x3A;
    static final int PAGE_ID = 0x07;
    static final int QUA_DATA = 0x20;
    static final int INT_STA = 0x37;
    static final int ST_RESULT = 0x36;

    /** RPT_BNO_INFO of the live board (vectors/bno055.json info_live_board). */
    static final String LIVE_INFO_HEX = "28a0fb320f1103150100000000006f012b09d105000000000000000000000000000000007d00";
    /** Quaternion a running fusion outputs (vectors stream_20_8). */
    static final byte[] LIVE_QUAT = HexFormat.of().parseHex("bd2963f9efcf0000");

    /** One register access: {@code read} or write, the page in force, the address and the bytes. */
    record RegOp(boolean read, int page, int addr, byte[] data) {}

    final FakeBoardLink link = new FakeBoardLink();
    final byte[][] regs = {new byte[256], new byte[256]};
    /** Every host command, in order ({cmd, payload}). */
    final List<Object[]> sent = new ArrayList<>();
    final List<RegOp> ops = new ArrayList<>();
    long mcuTimeUs = 5_000_000;
    byte[] info = HexFormat.of().parseHex(LIVE_INFO_HEX);
    final Map<Integer, ArrayDeque<Integer>> failNext = new HashMap<>();
    final Set<Integer> silent = new HashSet<>();
    int shortReadBy;
    final ArrayDeque<Integer> sysStatusScript = new ArrayDeque<>();
    int fusionZeroReadsAfterSwitch = 2;
    boolean fusionStuck;
    int selfTestResult = 0x0F;
    int selfTestError;
    final List<byte[]> streams = new ArrayList<>();
    boolean streaming;
    int resetCount;
    private long fusionZeroLeft;
    private boolean selfTestLeftover;

    FakeBno055() {
        powerOn();
        link.onPacket = this::handle;
    }

    // ── model controls ───────────────────────────────────────────────────────

    /** Power-on register values (contract 13 §2 / §4.2). */
    synchronized void powerOn() {
        for (byte[] p : regs) {
            Arrays.fill(p, (byte) 0);
        }
        byte[] ids = {(byte) 0xA0, (byte) 0xFB, 0x32, 0x0F, 0x11, 0x15, 0x03};
        System.arraycopy(ids, 0, regs[0], 0, ids.length);
        regs[0][0x3B] = (byte) 0x80; // UNIT_SEL: Android orientation
        regs[0][0x41] = 0x24;        // AXIS_MAP_CONFIG P1
        regs[0][ST_RESULT] = 0x0F;
        for (int i = 0; i < 16; i++) {
            regs[1][0x50 + i] = (byte) (0xC0 + i); // UNIQUE_ID
        }
        regs[1][0x08] = 0x0D; // ACC_CONFIG reset value
        regs[1][0x09] = 0x6D;
        regs[1][0x0A] = 0x38;
        selfTestLeftover = false;
        fusionZeroLeft = 0;
    }

    synchronized int page() {
        return regs[0][PAGE_ID] & 1;
    }

    synchronized int oprMode() {
        return regs[0][OPR_MODE] & 0x0F;
    }

    /** Put the model into an operating mode directly (a previous session left it there). */
    synchronized void setMode(int mode) {
        regs[0][OPR_MODE] = (byte) mode;
        if (mode >= 8) {
            System.arraycopy(LIVE_QUAT, 0, regs[0], QUA_DATA, 8);
        }
    }

    synchronized void fail(int cmd, int status) {
        failNext.computeIfAbsent(cmd, k -> new ArrayDeque<>()).add(status);
    }

    /** Writes only, as "page:addr=hex" — the shape most assertions want. */
    synchronized List<String> writes() {
        List<String> out = new ArrayList<>();
        for (RegOp o : ops) {
            if (!o.read()) {
                out.add(w(o.page(), o.addr(), HexFormat.of().formatHex(o.data())));
            }
        }
        return out;
    }

    static String w(int page, int addr, String hex) {
        return String.format("%d:%02x=%s", page, addr, hex);
    }

    synchronized int reads(int addr) {
        int n = 0;
        for (RegOp o : ops) {
            if (o.read() && o.addr() == addr) {
                n++;
            }
        }
        return n;
    }

    synchronized List<Integer> sentCmds() {
        List<Integer> out = new ArrayList<>();
        for (Object[] s : sent) {
            out.add((Integer) s[0]);
        }
        return out;
    }

    synchronized void clearLogs() {
        sent.clear();
        ops.clear();
    }

    /** Emit RPT_BNO_REG_STREAM. */
    void pushStream(int addr, byte[] data, long timestampUs, int lenField) {
        ByteBuffer p = ByteBuffer.allocate(10 + data.length).order(ByteOrder.LITTLE_ENDIAN);
        p.putLong(timestampUs).put((byte) addr).put((byte) lenField).put(data);
        pushRaw(0x93, p.array());
    }

    void pushStream(int addr, byte[] data, long timestampUs) {
        pushStream(addr, data, timestampUs, data.length);
    }

    void pushRaw(int cmd, byte[] payload) {
        link.inject(link.reply(cmd, payload));
    }

    private byte[] status(int cmd, int st) {
        return FakeBoardLink.status(cmd, st);
    }

    // ── the bridge ───────────────────────────────────────────────────────────

    private synchronized List<byte[]> handle(Packet pkt) {
        int cmd = pkt.cmd();
        byte[] payload = pkt.payload();
        sent.add(new Object[] {cmd, payload.clone()});
        if (silent.contains(cmd)) {
            return List.of();
        }
        ArrayDeque<Integer> fails = failNext.get(cmd);
        if (fails != null && !fails.isEmpty()) {
            return List.of(status(cmd, fails.poll()));
        }
        mcuTimeUs += 1000;
        if (cmd == 0x32) { // READ_REG
            if (payload.length != 2) {
                return List.of(status(cmd, 0x03));
            }
            int addr = payload[0] & 0xFF;
            int n = payload[1] & 0xFF;
            if (n < 1 || n > 128 || addr + n > 0x100) {
                return List.of(status(cmd, 0x04));
            }
            ops.add(new RegOp(true, page(), addr, new byte[n]));
            byte[] data = read(addr, n);
            if (shortReadBy > 0) {
                data = Arrays.copyOf(data, Math.max(0, data.length - shortReadBy));
                shortReadBy = 0;
            }
            ByteBuffer r = ByteBuffer.allocate(9 + data.length).order(ByteOrder.LITTLE_ENDIAN);
            r.put((byte) cmd).putLong(mcuTimeUs).put(data);
            return List.of(link.reply(0x91, r.array()));
        }
        if (cmd == 0x33) { // WRITE_REG
            if (payload.length < 2 || payload.length > 129) {
                return List.of(status(cmd, 0x03));
            }
            int addr = payload[0] & 0xFF;
            byte[] data = Arrays.copyOfRange(payload, 1, payload.length);
            if (addr + data.length > 0x100) {
                return List.of(status(cmd, 0x04));
            }
            ops.add(new RegOp(false, page(), addr, data));
            write(addr, data);
            return List.of(status(cmd, 0x00));
        }
        if (cmd == 0x34) { // RESET
            resetCount++;
            streaming = false;
            powerOn();
            return List.of(status(cmd, 0x00));
        }
        if (cmd == 0x35) { // START_STREAM
            if (payload.length != 5) {
                return List.of(status(cmd, 0x03));
            }
            streams.add(payload.clone());
            streaming = true;
            return List.of(status(cmd, 0x00));
        }
        if (cmd == 0x36) { // STOP_STREAM
            streaming = false;
            return List.of(status(cmd, 0x00));
        }
        if (cmd == 0x37) { // GET_INFO
            return List.of(link.reply(0x92, info));
        }
        return List.of(status(cmd, 0x02));
    }

    // ── the sensor ───────────────────────────────────────────────────────────

    private byte[] read(int addr, int n) {
        int page = page();
        byte[] out = Arrays.copyOfRange(regs[page], addr, addr + n);
        if (page == 0) {
            int end = addr + n;
            if (addr <= SYS_STATUS && SYS_STATUS < end) {
                out[SYS_STATUS - addr] = (byte) sysStatus();
            }
            if (addr <= QUA_DATA && QUA_DATA < end && fusionZeroLeft > 0) {
                fusionZeroLeft--;
                int lo = QUA_DATA - addr;
                int hi = Math.min(n, QUA_DATA - addr + 8);
                Arrays.fill(out, lo, hi, (byte) 0);
                if (fusionZeroLeft == 0 && !fusionStuck) {
                    System.arraycopy(LIVE_QUAT, 0, regs[0], QUA_DATA, 8);
                }
            }
            if (addr <= INT_STA && INT_STA < end) {
                regs[0][INT_STA] = 0; // clears on read
            }
        }
        return out;
    }

    private int sysStatus() {
        if (!sysStatusScript.isEmpty()) {
            return sysStatusScript.poll();
        }
        if (selfTestLeftover) {
            return 4;
        }
        int mode = oprMode();
        return mode == 0 ? 0 : mode >= 8 ? 5 : 6;
    }

    private void write(int addr, byte[] data) {
        int page = page();
        for (int i = 0; i < data.length; i++) {
            int reg = addr + i;
            int v = data[i] & 0xFF;
            if (page == 0 && reg == OPR_MODE) {
                writeOprMode(v & 0x0F);
            } else if (page == 0 && reg == SYS_TRIGGER) {
                writeSysTrigger(v);
            } else if (reg == PAGE_ID) {
                regs[0][PAGE_ID] = (byte) (v & 1); // PAGE_ID is on both pages
            } else {
                regs[page][reg] = (byte) v;
            }
        }
    }

    private void writeOprMode(int mode) {
        int current = oprMode();
        boolean suspended = (regs[0][PWR_MODE] & 3) == 2;
        if (mode != 0 && (current != 0 || suspended)) {
            return; // silently ignored (§4.1, §4.6)
        }
        regs[0][OPR_MODE] = (byte) mode;
        if (mode == 0) {
            Arrays.fill(regs[0], 0x08, 0x34, (byte) 0); // CONFIG reads zero everywhere
        } else {
            selfTestLeftover = false;
            if (mode >= 8) {
                fusionZeroLeft = fusionStuck ? 1L << 60 : fusionZeroReadsAfterSwitch;
                if (fusionZeroLeft == 0) {
                    System.arraycopy(LIVE_QUAT, 0, regs[0], QUA_DATA, 8);
                }
            }
        }
    }

    private void writeSysTrigger(int v) {
        regs[0][SYS_TRIGGER] = (byte) (v & 0x80); // CLK_SEL sticks, action bits self-clear
        if ((v & 0x01) != 0 && oprMode() == 0) {
            regs[0][ST_RESULT] = (byte) selfTestResult;
            regs[0][SYS_ERR] = (byte) selfTestError;
            selfTestLeftover = true;
        }
        if ((v & 0x40) != 0) {
            regs[0][INT_STA] = 0;
        }
    }
}
