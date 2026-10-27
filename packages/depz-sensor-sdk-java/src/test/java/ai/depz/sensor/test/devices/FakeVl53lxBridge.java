package ai.depz.sensor.test.devices;

import ai.depz.sensor.protocol.Common;
import ai.depz.sensor.protocol.Vl53lx;
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

/**
 * The {@code APP_VL53L0_4} register bridge in-process (contract 12, protocol v2.01): the Python tests'
 * {@code fake_vl53lx.py} over a {@link FakeBoardLink}. The MCU knows no sensor, so neither does this
 * fake: behind the bridge sits a 64 KiB register file pre-loaded with what a booted VL53L1-family die
 * answers (boot status 3, oscillator, model id, PLL, ROI centre) — enough for the host ULDs to run
 * their init, timing, offset, xtalk, threshold, calibration and ranging sequences. The result block
 * (0x0089, 17 B) and the data-ready bit (0x0031 bit 0) are plain registers.
 *
 * <p>Every packet the host sends is logged ({@link #rx}); {@link #failNext} answers the next
 * {@code cmd} with an error status (or nothing); {@link #stream} pushes an RPT_VL53_STREAM.
 */
final class FakeVl53lxBridge {
    static final String SOFTWARE_NAME = "APP_VL53L0_4_v0.25";
    static final int DIE_MODEL_ID = 0xEBAA;
    static final int DIE_OSC = 0xBCCC;
    static final int DIE_PLL = 0x0123;

    static final int READ_REG = Vl53lx.Vl53lxCmd.READ_REG.value;
    static final int WRITE_REG = Vl53lx.Vl53lxCmd.WRITE_REG.value;
    static final int XSHUT = Vl53lx.Vl53lxCmd.XSHUT.value;
    static final int START_STREAM = Vl53lx.Vl53lxCmd.START_STREAM.value;
    static final int STOP_STREAM = Vl53lx.Vl53lxCmd.STOP_STREAM.value;
    static final int GET_INFO = Vl53lx.Vl53lxCmd.GET_INFO.value;
    static final int SET_I2C_SPEED = Vl53lx.Vl53lxCmd.SET_I2C_SPEED.value;
    static final int SET_ADDR_WIDTH = Vl53lx.Vl53lxCmd.SET_ADDR_WIDTH.value;
    static final int CLEAR_I2C_ERRORS = Vl53lx.Vl53lxCmd.CLEAR_I2C_ERRORS.value;
    static final int GET_DEVICE_NAME = Common.Cmd.GET_DEVICE_NAME.value;

    /** "silent": no reply at all to the next command of that opcode. */
    static final int SILENT = -1;

    final FakeBoardLink link = new FakeBoardLink();
    final byte[] regs = new byte[0x10000];
    /** Every packet the host sent, in order ({cmd, payload}). */
    final List<Object[]> rx = new ArrayList<>();
    /** Bytes to drop from the next READ_REG answer (a short reply). */
    int truncateNextRead = 0;
    volatile long mcuTimeUs = 5_000_000;
    volatile byte[] info = new byte[Vl53lx.INFO_SIZE];
    final String deviceName;
    final boolean noClear;
    private final Map<Integer, ArrayDeque<Integer>> failures = new HashMap<>();

    FakeVl53lxBridge(String deviceName, boolean noClearI2cErrors) {
        this.deviceName = deviceName;
        this.noClear = noClearI2cErrors;
        regs[0x00E5] = 0x03;
        setWord(0x0006, DIE_OSC);
        setWord(0x010F, DIE_MODEL_ID);
        setWord(0x00DE, DIE_PLL);
        regs[0x013E] = (byte) 0x91; // ROI_CONFIG__MODE_ROI_CENTRE_SPAD
        link.onPacket = this::handle;
    }

    FakeVl53lxBridge(String deviceName) {
        this(deviceName, false);
    }

    // ── register file ────────────────────────────────────────────────────────

    void setWord(int addr, int v) {
        regs[addr] = (byte) (v >> 8);
        regs[addr + 1] = (byte) v;
    }

    int word(int addr) {
        return (regs[addr] & 0xFF) << 8 | (regs[addr + 1] & 0xFF);
    }

    long dword(int addr) {
        return (long) word(addr) << 16 | word(addr + 2);
    }

    /** Put a result block at 0x0089 (what the next poll reads). */
    void setResultBlock(byte[] raw) {
        System.arraycopy(raw, 0, regs, 0x0089, raw.length);
    }

    // ── traffic ──────────────────────────────────────────────────────────────

    synchronized void failNext(int cmd, int status) {
        failures.computeIfAbsent(cmd, k -> new ArrayDeque<>()).addLast(status);
    }

    /** Payloads of one opcode, in order. */
    List<byte[]> sent(int cmd) {
        List<byte[]> out = new ArrayList<>();
        synchronized (rx) {
            for (Object[] e : rx) {
                if ((int) e[0] == cmd) {
                    out.add((byte[]) e[1]);
                }
            }
        }
        return out;
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

    /** Every WRITE_REG as {addr, data}. */
    List<Object[]> writes() {
        List<Object[]> out = new ArrayList<>();
        for (byte[] p : sent(WRITE_REG)) {
            out.add(new Object[] {u16(p, 0), Arrays.copyOfRange(p, 2, p.length)});
        }
        return out;
    }

    /** The register writes as "addr:hex" strings, for exact comparison. */
    List<String> writeKeys() {
        List<String> out = new ArrayList<>();
        for (Object[] w : writes()) {
            out.add(String.format("%04X:%s", (int) w[0], java.util.HexFormat.of().formatHex((byte[]) w[1])));
        }
        return out;
    }

    static String key(int addr, int... data) {
        byte[] b = new byte[data.length];
        for (int i = 0; i < data.length; i++) {
            b[i] = (byte) data[i];
        }
        return String.format("%04X:%s", addr, java.util.HexFormat.of().formatHex(b));
    }

    void clearRx() {
        synchronized (rx) {
            rx.clear();
        }
    }

    int rxCount() {
        synchronized (rx) {
            return rx.size();
        }
    }

    /** RPT_VL53_STREAM: ts u64, addr u16, len u16, data. */
    void stream(byte[] data, long tsUs, int addr, Integer length) {
        int n = length == null ? data.length : length;
        ByteBuffer b = ByteBuffer.allocate(12 + data.length).order(ByteOrder.LITTLE_ENDIAN);
        b.putLong(tsUs).putShort((short) addr).putShort((short) n).put(data);
        send(Vl53lx.Vl53lxRpt.STREAM.value, b.array());
    }

    void stream(byte[] data, long tsUs) {
        stream(data, tsUs, 0x0089, null);
    }

    void send(int cmd, byte[] payload) {
        link.inject(link.reply(cmd, payload));
    }

    private byte[] status(int cmd, int status) {
        return FakeBoardLink.status(cmd, status);
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
        byte[] p = pkt.payload();
        synchronized (rx) {
            rx.add(new Object[] {cmd, p});
        }
        ArrayDeque<Integer> q = failures.get(cmd);
        if (q != null && !q.isEmpty()) {
            int fail = q.pollFirst();
            return fail == SILENT ? List.of() : List.of(status(cmd, fail));
        }
        if (cmd == GET_DEVICE_NAME) {
            return List.of(text(cmd, deviceName));
        }
        if (cmd == Common.Cmd.GET_NAME_ACTIVE_SOFTWARE.value) {
            return List.of(text(cmd, SOFTWARE_NAME));
        }
        if (cmd == READ_REG) {
            int addr = u16(p, 0);
            int n = u16(p, 2);
            int avail = Math.max(0, Math.min(n, regs.length - addr) - truncateNextRead);
            truncateNextRead = 0;
            ByteBuffer b = ByteBuffer.allocate(9 + avail).order(ByteOrder.LITTLE_ENDIAN);
            b.put((byte) cmd).putLong(mcuTimeUs).put(regs, addr, avail);
            mcuTimeUs += 100;
            return List.of(link.reply(Vl53lx.Vl53lxRpt.REG_DATA.value, b.array()));
        }
        if (cmd == WRITE_REG) {
            int addr = u16(p, 0);
            System.arraycopy(p, 2, regs, addr, p.length - 2);
            return List.of(status(cmd, 0));
        }
        if (cmd == GET_INFO) {
            return List.of(link.reply(Vl53lx.Vl53lxRpt.INFO.value, info));
        }
        if (cmd == CLEAR_I2C_ERRORS && noClear) {
            return List.of(status(cmd, Common.Status.ERR_INVALID_CMD.value));
        }
        if (cmd == XSHUT || cmd == START_STREAM || cmd == STOP_STREAM || cmd == SET_I2C_SPEED
                || cmd == SET_ADDR_WIDTH || cmd == CLEAR_I2C_ERRORS) {
            return List.of(status(cmd, 0));
        }
        return List.of(status(cmd, Common.Status.ERR_INVALID_CMD.value));
    }

    static int u16(byte[] b, int off) {
        return (b[off] & 0xFF) | (b[off + 1] & 0xFF) << 8;
    }

    static byte[] hex(String s) {
        return java.util.HexFormat.of().parseHex(s);
    }
}
