package ai.depz.sensor.test.devices;

import ai.depz.sensor.sensors.vl53lx.Vl53lxPlatform;
import ai.depz.sensor.sensors.vl53lx.Vl53lxProtocolException;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HexFormat;
import java.util.List;
import java.util.Map;

/**
 * A VL53L0X die as the ULD's platform, no bridge in between: the Python tests' {@code fake_vl53l0x.py}.
 * One 256-byte bank per page (register 0xFF selects it, as on the die), with the registers that
 * <em>behave</em> modelled on top: the soft reset and the NACKs around it, the NVM strobe and data,
 * the single-shot start bit, data-ready and interrupt clear, the result block of each shot and the
 * reference-signal measurement the SPAD management converges on.
 *
 * <p>The defaults are what the live VL53L0X board (TB9BGETA6M) answered in the committed capture
 * vectors/recordings/vl53l0x_uld_long-range_33ms.depzrec: stop variable 0x3C, oscillator word 0x975C,
 * NVM reference-SPAD record "6 non-aperture", good SPAD map FB FF FF DF B7 0F, VHV 0x22, phase 0x01,
 * OSC_CALIBRATE_VAL 0x28, one NACK on the model id after a soft reset.
 */
final class FakeVl53l0xDie extends Vl53lxPlatform {
    static final int MODEL_ID = 0xEE;
    static final int START_SELECT = 0xB4;          // GLOBAL_CONFIG_REF_EN_START_SELECT
    static final int FIRST_APERTURE_SPAD = 192;    // SPAD indices from here lie in an aperture quadrant

    // Result blocks (12 B at 0x14) of the capture: no target yet (8191 mm, device status 0x58), and a
    // flat target at 635 mm (status 0x5A: internal 11).
    static final byte[] FRAME_NO_TARGET = HexFormat.of().parseHex("5802ffffffffffffffff1fff");
    static final byte[] FRAME_635_MM = HexFormat.of().parseHex("5a06c1040071022b011b027b");
    static final String PRODUCT_ID = "VL53L0X-TEST-18CHR";

    final Map<Integer, Long> nvm = new HashMap<>();
    private final Map<Integer, byte[]> banks = new HashMap<>();
    int page;

    // ── knobs ──
    final ArrayDeque<byte[]> frames = new ArrayDeque<>(); // result block per ranging shot; the last one repeats
    byte[] frame = FRAME_635_MM;
    int refRatePerSpad = 400;          // 9.7 Mcps each enabled non-aperture reference SPAD adds
    int refRatePerApertureSpad = 100;  // ... each aperture one
    int readyAfterPolls = 0;           // RESULT_INTERRUPT_STATUS polls before a shot is ready
    int startBitReads = 0;             // SYSRANGE_START reads that still show the start bit
    boolean startBitStuck = false;
    int nacksAfterReset = 1;
    boolean resetIgnored = false;      // the soft reset does nothing
    boolean strobeStuck = false;       // the NVM read strobe never comes
    boolean interruptSticky = false;   // SYSTEM_INTERRUPT_CLEAR does not clear
    boolean spadMapStuck = false;      // writes to the SPAD enables are lost
    int interruptIdle = 0x40;          // RESULT_INTERRUPT_STATUS with nothing pending

    // ── what happened ──
    final List<int[]> shots = new ArrayList<>();     // {SYSRANGE_START value, sequence config}
    final List<Integer> refPeaks = new ArrayList<>(); // reference rate of each reference shot
    final List<Object[]> writes = new ArrayList<>();  // {addr, byte[]} of every register write, page selects included
    final List<int[]> reads = new ArrayList<>();      // {page, addr, size} of every register read

    private boolean inReset;
    private int nacks;
    private int startReads;
    private Integer readyIn; // polls until ready; null = nothing pending

    FakeVl53l0xDie() {
        nvm.put(0x6B, 0x1D80060BL);  // reference SPADs: (>>8)&0x7F = 6, (>>15)&1 = 0 (non-aperture)
        nvm.put(0x24, 0xFBFFFFDFL);  // good SPAD map bytes 0..3
        nvm.put(0x25, 0xB70F0000L);  // good SPAD map bytes 4..5
        nvm.put(0x02, 0x2A000000L);  // module id (first byte)
        nvm.put(0x7B, 0x0C345678L);  // revision (first byte) / part UID upper
        nvm.put(0x7C, 0x9ABCDEF0L);  // part UID lower
        nvm.put(0x73, 0x000000A5L);  // signal rate at 400 mm, high byte
        nvm.put(0x74, 0x3C000000L);  // ... low byte -> 0xA53C in 9.7
        nvm.put(0x75, 0x00000019L);  // distance at 400 mm (1/16 mm), high byte
        nvm.put(0x76, 0x40000000L);  // ... low byte -> 0x1940 = 6464 = 404 mm
        nvm.putAll(nvmProductId(PRODUCT_ID));

        byte[] b0 = bank(0);
        b0[0xC0] = (byte) MODEL_ID;
        b0[0x84] = 0x01;              // GPIO_HV_MUX_ACTIVE_HIGH as read on the board
        b0[0xCB] = 0x22;              // VHV settings after the reference calibration
        b0[0xEE] = 0x01;              // phase cal
        b0[0xF8] = 0x00;              // OSC_CALIBRATE_VAL
        b0[0xF9] = 0x28;
        System.arraycopy(frame, 0, b0, 0x14, 12);
        byte[] b1 = bank(1);
        b1[0x91] = 0x3C;              // the stop variable DataInit reads
        b1[0x84] = (byte) 0x97;       // oscillator frequency, FixPoint4.12
        b1[0x85] = 0x5C;
        bank(6)[0x83] = 0x01;
    }

    /** The four NVM words 0x77..0x7A holding an 18-character product id: 7 bits per character, MSB first,
     *  two padding bits at the end. */
    static Map<Integer, Long> nvmProductId(String text) {
        StringBuilder bits = new StringBuilder();
        for (int i = 0; i < text.length(); i++) {
            String b = Integer.toBinaryString(text.charAt(i) & 0x7F);
            bits.append("0".repeat(7 - b.length())).append(b);
        }
        bits.append("00");
        Map<Integer, Long> out = new HashMap<>();
        for (int i = 0; i < 4; i++) {
            out.put(0x77 + i, Long.parseLong(bits.substring(32 * i, 32 * i + 32), 2));
        }
        return out;
    }

    byte[] bank(int n) {
        return banks.computeIfAbsent(n, k -> new byte[256]);
    }

    byte[] spadEnables() {
        return Arrays.copyOfRange(bank(0), 0xB0, 0xB6);
    }

    /** Bytes {@code [addr, addr + n)} of page {@code pg}. */
    byte[] regs(int pg, int addr, int n) {
        return Arrays.copyOfRange(bank(pg), addr, addr + n);
    }

    // ── register map ──
    @Override
    public byte[] readMulti(int addr, int n) {
        reads.add(new int[] {page, addr, n});
        byte[] bank = bank(page);
        if (page == 0 && addr == 0xC0) {
            if (nacks > 0) {
                nacks--;
                throw new Vl53lxProtocolException("READ_REG 0x00C0: ERR_I2C_NACK");
            }
            byte[] out = Arrays.copyOfRange(bank, 0xC0, 0xC0 + n);
            out[0] = (byte) (inReset ? 0x00 : MODEL_ID);
            return out;
        }
        if (page == 7 && addr == 0x83) {
            return new byte[] {(byte) (strobeStuck ? 0x00 : 0x10)};
        }
        if (page == 7 && addr == 0x90) {
            long v = nvm.getOrDefault(bank[0x94] & 0xFF, 0L);
            byte[] word = {(byte) (v >> 24), (byte) (v >> 16), (byte) (v >> 8), (byte) v};
            return Arrays.copyOf(word, n);
        }
        if (page == 0 && addr == 0x00) {
            if (startReads > 0 || startBitStuck) {
                startReads = Math.max(0, startReads - 1);
                return new byte[] {(byte) (bank[0x00] | 0x01)};
            }
            return new byte[] {(byte) (bank[0x00] & 0xFE)};
        }
        if (page == 0 && addr == 0x13) {
            boolean ready = readyIn != null && readyIn == 0;
            if (readyIn != null && readyIn > 0) {
                readyIn--;
            }
            return new byte[] {(byte) (interruptIdle | (ready ? 0x04 : 0))};
        }
        return Arrays.copyOfRange(bank, addr, addr + n);
    }

    @Override
    public void writeMulti(int addr, byte[] data) {
        writes.add(new Object[] {addr, data.clone()});
        if (addr == 0xFF) {
            page = data[0] & 0xFF;
            return;
        }
        byte[] bank = bank(page);
        int v = data[0] & 0xFF;
        if (page == 0 && addr == 0xBF && !resetIgnored) {
            if (v == 0x00) {
                inReset = true;
            } else {
                inReset = false;
                nacks = nacksAfterReset;
            }
        } else if (page == 0 && addr == 0x00 && (v & 0x01) != 0) {
            shot(v);
        } else if (page == 0 && addr == 0x0B && v == 0x01 && !interruptSticky) {
            readyIn = null;
        } else if (page == 0 && addr >= 0xB0 && addr <= 0xB5 && spadMapStuck) {
            return;
        }
        System.arraycopy(data, 0, bank, addr, data.length);
    }

    @Override
    public void setI2cSpeed(int khz) {}

    @Override
    public void setAddrWidth(int width) {}

    @Override
    public void xshutReset() {}

    @Override
    public void sleepMs(int ms) {}

    // ── a measurement ──
    private void shot(int value) {
        int sequence = bank(0)[0x01] & 0xFF;
        shots.add(new int[] {value, sequence});
        startReads = startBitReads;
        readyIn = readyAfterPolls;
        if (sequence == 0xC0) {
            // Reference-signal measurement: the peak reference rate follows the SPAD enables.
            int peak = refPeak();
            refPeaks.add(peak);
            bank(1)[0xB6] = (byte) (peak >> 8);
            bank(1)[0xB7] = (byte) peak;
        } else if (sequence != 0x01 && sequence != 0x02) { // VHV / phase shots range nothing
            if (!frames.isEmpty()) {
                frame = frames.poll();
            }
            System.arraycopy(frame, 0, bank(0), 0x14, 12);
        }
    }

    private int refPeak() {
        byte[] enables = spadEnables();
        int peak = 0;
        for (int index = 0; index < 8 * enables.length; index++) {
            if (((enables[index / 8] >> (index % 8)) & 1) != 0) {
                boolean aperture = START_SELECT + index >= FIRST_APERTURE_SPAD;
                peak += aperture ? refRatePerApertureSpad : refRatePerSpad;
            }
        }
        return Math.min(peak, 0xFFFF);
    }
}
