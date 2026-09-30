package ai.depz.sensor.sensors.vl53lx;

import ai.depz.sensor.sensors.vl53l4.Vl53l4Uld;

/**
 * Stateless decoders of the blocks the 1D-family bridge streams
 * (contracts/12_SENSOR_VL53LX.md §4) — the "base" every SDK implements. Mirrors
 * {@code depz_sensor_sdk/vl53lx/decode.py} 1:1.
 *
 * <ul>
 *   <li>{@link #decodeDieBlock} — the 17-byte VL53L1-die result block at 0x0089
 *       (L1CX, L1CB, L3CX, L4CD, L4CX light drivers), fully decoded. It is
 *       contract 10 §4's decode ({@link Vl53l4Uld#parseResultBlock}) parameterised
 *       by the ULD that reads it ({@link DieVariant}).</li>
 *   <li>{@link #decodeL0xRaw} — raw fields of the VL53L0X 12-byte block at 0x14.
 *       The PAL range status, sigma and dmax need the device data cached by init
 *       (full driver, not base).</li>
 *   <li>{@link #decodeHistogramRaw} — status bytes and the 24 photon bins of the
 *       83-byte histogram block at 0x0088. Bins → targets is the full driver.</li>
 * </ul>
 *
 * <p>OUT OF SCOPE (as for every Java sensor): the live ULD / Bare-Driver host
 * drivers over the bridge.
 */
public final class Vl53lxDecode {
    private Vl53lxDecode() {}

    public static final int DIE_BLOCK_ADDR = 0x0089;
    public static final int DIE_BLOCK_LEN = 17;
    public static final int L0X_BLOCK_ADDR = 0x14;
    public static final int L0X_BLOCK_LEN = 12;

    // Bare Driver register map (vl53lx/uld/bare/core.py)
    public static final int HISTOGRAM_BLOCK_ADDR = 0x0088;           // result__interrupt_status
    public static final int RESULT_HISTOGRAM_BIN_0_2 = 0x008E;
    public static final int RESULT_HISTOGRAM_BIN_23_0 = 0x00D5;
    public static final int PHASECAL_RESULT_REFERENCE_PHASE = 0x00D6;
    public static final int PHASECAL_RESULT_VCSEL_START = 0x00D8;
    public static final int RESULT_HISTOGRAM_BIN_23_0_MSB = 0x00D9;
    public static final int RESULT_HISTOGRAM_BIN_23_0_LSB = 0x00DA;
    public static final int HISTOGRAM_BLOCK_LEN =
        RESULT_HISTOGRAM_BIN_23_0_LSB - HISTOGRAM_BLOCK_ADDR + 1;   // 83
    public static final int HISTOGRAM_BINS = 24;

    /**
     * The two ULDs that read the die block: signal-rate byte offset and per-SPAD
     * scale K. {@code l4}: VL53L4CD ULD — also the L3CX ULP and L4CX-as-L4CD;
     * {@code l1}: VL53L1X ULD (crosstalk-corrected peak signal at 0x0098, K = 25).
     */
    public enum DieVariant {
        L4("l4", 5, 256),
        L1("l1", 15, 25);

        public final String label;
        public final int signalOffset;
        public final int perSpadK;
        DieVariant(String label, int signalOffset, int perSpadK) {
            this.label = label;
            this.signalOffset = signalOffset;
            this.perSpadK = perSpadK;
        }

        /** Variant for {@code "l4"} / {@code "l1"}; throws {@link IllegalArgumentException} otherwise. */
        public static DieVariant fromLabel(String label) {
            for (DieVariant v : values()) {
                if (v.label.equals(label)) {
                    return v;
                }
            }
            throw new IllegalArgumentException("unknown die-block variant: " + label);
        }
    }

    /**
     * The 17-byte die block (0x0089..0x0099) as the named ULD reads it. The result
     * shape is contract 10's {@link Vl53l4Uld.Results}; the status goes through the
     * same {@link Vl53l4Uld#STATUS_RTN}. Throws {@link IllegalArgumentException}
     * when {@code raw} is shorter than {@value #DIE_BLOCK_LEN} bytes.
     */
    public static Vl53l4Uld.Results decodeDieBlock(byte[] raw, DieVariant variant) {
        if (raw.length < DIE_BLOCK_LEN) {
            throw new IllegalArgumentException(
                "die result block needs " + DIE_BLOCK_LEN + " bytes, got " + raw.length);
        }
        int status = raw[0] & 0x1F;
        if (status < Vl53l4Uld.STATUS_RTN.length) {
            status = Vl53l4Uld.STATUS_RTN[status];
        }
        int rawSpads = be16(raw, 3);                          // 8.8
        int signal = be16(raw, variant.signalOffset) * 8;
        int ambient = be16(raw, 7) * 8;
        int k = variant.perSpadK;
        return new Vl53l4Uld.Results(
            status,
            be16(raw, 13),
            ambient,
            rawSpads != 0 ? ambient * k / rawSpads : 0,
            signal,
            rawSpads != 0 ? signal * k / rawSpads : 0,
            rawSpads / 256,
            be16(raw, 9) / 4,
            raw[2] & 0xFF);
    }

    /** {@link #decodeDieBlock(byte[], DieVariant)} with the variant by label ({@code "l4"}/{@code "l1"}). */
    public static Vl53l4Uld.Results decodeDieBlock(byte[] raw, String variant) {
        return decodeDieBlock(raw, DieVariant.fromLabel(variant));
    }

    /**
     * Raw fields of the VL53L0X block at 0x14.
     *
     * @param distanceRaw          mm (quarter-mm when RangeFractionalEnable, off by default)
     * @param deviceRangeStatus    raw byte 0; the PAL status needs the init state
     * @param signalRateMcps1616   FixPoint16.16 Mcps (9.7 on the wire {@code << 9})
     * @param ambientRateMcps1616  FixPoint16.16 Mcps
     * @param effectiveSpadCount88 8.8
     */
    public record L0xRaw(int distanceRaw, int deviceRangeStatus, int signalRateMcps1616,
                         int ambientRateMcps1616, int effectiveSpadCount88) {}

    /** VL53L0X_GetRangingMeasurementData before the PAL status/sigma step. */
    public static L0xRaw decodeL0xRaw(byte[] raw) {
        if (raw.length < L0X_BLOCK_LEN) {
            throw new IllegalArgumentException(
                "VL53L0X result block needs " + L0X_BLOCK_LEN + " bytes, got " + raw.length);
        }
        return new L0xRaw(
            be16(raw, 10),
            raw[0] & 0xFF,
            be16(raw, 6) << 9,
            be16(raw, 8) << 9,
            be16(raw, 2));
    }

    /** Status bytes and the 24 photon counts of the histogram block. */
    public record HistogramRaw(int interruptStatus, int rangeStatus, int reportStatus,
                               int streamCount, int dssActualEffectiveSpads,
                               int referencePhase, int vcselStart, int[] bins) {}

    /**
     * The 83-byte histogram block at 0x0088: status bytes and the 24 bins of 3
     * big-endian bytes each. Bin 23's low byte is carried in a separate MSB/LSB
     * pair — {@code (MSB << 2) + LSB}, truncated to 8 bits — and is patched in
     * before the bins are read. {@code raw} is not modified.
     */
    public static HistogramRaw decodeHistogramRaw(byte[] raw) {
        if (raw.length < HISTOGRAM_BLOCK_LEN) {
            throw new IllegalArgumentException(
                "histogram block needs " + HISTOGRAM_BLOCK_LEN + " bytes, got " + raw.length);
        }
        final int off = HISTOGRAM_BLOCK_ADDR;
        byte[] buf = raw.clone();
        buf[RESULT_HISTOGRAM_BIN_23_0 - off] = (byte) ((((buf[RESULT_HISTOGRAM_BIN_23_0_MSB - off] & 0xFF) << 2)
            + (buf[RESULT_HISTOGRAM_BIN_23_0_LSB - off] & 0xFF)) & 0xFF);
        int base = RESULT_HISTOGRAM_BIN_0_2 - off;
        int[] bins = new int[HISTOGRAM_BINS];
        for (int i = 0; i < HISTOGRAM_BINS; i++) {
            int o = base + 3 * i;
            bins[i] = ((buf[o] & 0xFF) << 16) | ((buf[o + 1] & 0xFF) << 8) | (buf[o + 2] & 0xFF);
        }
        return new HistogramRaw(
            buf[0] & 0xFF,
            buf[1] & 0xFF,
            buf[2] & 0xFF,
            buf[3] & 0xFF,
            be16(buf, 4),
            be16(buf, PHASECAL_RESULT_REFERENCE_PHASE - off),
            buf[PHASECAL_RESULT_VCSEL_START - off] & 0xFF,
            bins);
    }

    private static int be16(byte[] b, int off) {
        return ((b[off] & 0xFF) << 8) | (b[off + 1] & 0xFF);
    }
}
