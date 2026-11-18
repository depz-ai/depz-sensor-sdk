package ai.depz.sensor.sensors.vl53l8;

import java.util.Arrays;

/**
 * The CNH (compact network histogram) configuration of a VL53L8CH / VL53L7CH: a mirror of
 * {@code VL53LMZ_Motion_Configuration} with the plugin helpers that fill it
 * ({@code vl53lmz_plugin_cnh.c}, ULD 2.0.16), ported from the reference Python {@code CnhConfig}.
 * Build it with {@link #initConfig} and {@link #createAggMap}, check {@link #requiredMemory()},
 * then arm it with the device's {@code configureCnh}. The flags are fixed to ping-pong and variance
 * off ({@code Example_12_cnh_data.c}), which {@link Vl53l8Uld#decodeCnh} assumes. Arguments outside
 * their fields throw {@link IllegalArgumentException}, never wrap.
 */
public final class Vl53l8CnhSetup {
    public static final int MI_CFG_DEV_IDX = 0xBFAC;
    public static final int CNH_MAX_DATA_BYTES = 1540 * 4;
    public static final int PULSE_WIDTH_BIN = 10;
    public static final double BIN_WIDTH_MM = 37.5348;

    private static final int DISABLE_PING_PONG = 0x01;
    private static final int DISABLE_VARIANCE = 0x02;
    private static final int ENABLE_AMBIENT_LEVEL = 0x04;
    private static final int ENABLE_XTALK_REMOVAL = 0x08;
    private static final int ZERO_NON_VALID_BINS = 0x10;
    private static final int STORE_REF_RESIDUAL = 0x20;
    private static final int PER_HEADER_WORDS = 5;
    private static final int PER_BUFFER_HEADER_WORDS = 2;

    private int refBinOffset;
    private int sumSpan;
    private int featureLength;
    private int nbOfAggregates;
    private int cnhCfg;
    private int cnhFlexShift;
    private final int[] mapId = new int[64];

    public Vl53l8CnhSetup() {
        Arrays.fill(mapId, -1);
    }

    public int refBinOffset() {
        return refBinOffset;
    }

    public int sumSpan() {
        return sumSpan;
    }

    public int featureLength() {
        return featureLength;
    }

    public int nbOfAggregates() {
        return nbOfAggregates;
    }

    public int cnhCfg() {
        return cnhCfg;
    }

    public int cnhFlexShift() {
        return cnhFlexShift;
    }

    /** Device zone → CNH aggregate, −1 for a zone in none (64 entries). */
    public int[] mapId() {
        return mapId.clone();
    }

    /**
     * vl53lmz_cnh_init_config. {@code startBin}: first device histogram bin; {@code numBins}: CNH
     * bins; {@code subSample}: device bins summed per CNH bin.
     */
    public void initConfig(int startBin, int numBins, int subSample) {
        // ref_bin_offset = start_bin * 2048 is an i32 field.
        if (startBin < -1048576 || startBin > 1048575) {
            throw new IllegalArgumentException("start_bin out of range -1048576..1048575, got " + startBin);
        }
        if (numBins < 0 || numBins > 0xFF) {
            throw new IllegalArgumentException("num_bins out of range 0..255, got " + numBins);
        }
        if (subSample < 0 || subSample > 0xFF) {
            throw new IllegalArgumentException("sub_sample out of range 0..255, got " + subSample);
        }
        refBinOffset = startBin * 2048;
        featureLength = numBins;
        sumSpan = subSample;
        cnhCfg = DISABLE_PING_PONG | DISABLE_VARIANCE | ENABLE_AMBIENT_LEVEL | ENABLE_XTALK_REMOVAL
                | ZERO_NON_VALID_BINS | STORE_REF_RESIDUAL;
        cnhFlexShift = 1;
    }

    /**
     * vl53lmz_cnh_create_agg_map: aggregates of {@code mergeX}×{@code mergeY} zones, {@code cols}×
     * {@code rows} of them from ({@code startX}, {@code startY}). {@code resolution} (16 or 64) must
     * be the sensor's.
     */
    public void createAggMap(int resolution, int startX, int startY, int mergeX, int mergeY, int cols, int rows) {
        if (resolution != 16 && resolution != 64) {
            throw new IllegalArgumentException("resolution " + resolution + ": 16 or 64");
        }
        String[] names = {"start_x", "start_y", "merge_x", "merge_y", "cols", "rows"};
        int[] values = {startX, startY, mergeX, mergeY, cols, rows};
        int[] lows = {0, 0, 1, 1, 1, 1};
        for (int k = 0; k < names.length; k++) {
            if (values[k] < lows[k]) {
                throw new IllegalArgumentException("agg map " + names[k] + " " + values[k] + " must be an integer >= " + lows[k]);
            }
        }
        int zoneRes = resolution == 16 ? 4 : 8;
        // In 64 bits: Integer.MAX_VALUE arguments must not wrap past the check.
        if (startX + (long) cols * mergeX > zoneRes || startY + (long) rows * mergeY > zoneRes) {
            throw new IllegalArgumentException("agg map exceeds zone grid");
        }
        Arrays.fill(mapId, -1);
        nbOfAggregates = cols * rows;
        for (int row = startY; row < startY + rows * mergeY; row++) {
            for (int col = startX; col < startX + cols * mergeX; col++) {
                int aggId = (row - startY) / mergeY * cols + (col - startX) / mergeX;
                if (aggId < 0 || aggId >= 64) {
                    throw new IllegalArgumentException("agg id out of range");
                }
                mapId[row * zoneRes + col] = aggId;
            }
        }
    }

    /**
     * vl53lmz_cnh_calc_required_memory: the on-device CNH buffer, bytes.
     *
     * @throws IllegalArgumentException no aggregate map yet, or more than {@link #CNH_MAX_DATA_BYTES}
     */
    public int requiredMemory() {
        if (nbOfAggregates == 0) {
            throw new IllegalArgumentException("agg map not created");
        }
        int size = calcRequiredMemory(cnhCfg, nbOfAggregates, featureLength);
        if (size > CNH_MAX_DATA_BYTES) {
            throw new IllegalArgumentException("CNH needs " + size + " B > max " + CNH_MAX_DATA_BYTES
                    + " B — reduce aggregates or bins");
        }
        return size;
    }

    /** {min, max} target distance, mm, the histogram captures whole. */
    public int[] minMaxDistanceMm() {
        double pulse = PULSE_WIDTH_BIN / 2.0 * BIN_WIDTH_MM;
        double start = refBinOffset / 2048.0;
        double first = (start + sumSpan / 2.0) * BIN_WIDTH_MM;
        double last = (start + (featureLength - 1) * sumSpan + sumSpan / 2.0) * BIN_WIDTH_MM;
        return new int[] {(int) (first + pulse), (int) (last - pulse)};
    }

    /** Distance, mm, at the centre of CNH bin {@code binIdx}. */
    public double binCenterMm(int binIdx) {
        return (refBinOffset / 2048.0 + binIdx * sumSpan + sumSpan / 2.0) * BIN_WIDTH_MM;
    }

    /** The 156-byte VL53LMZ_Motion_Configuration struct for the device (DCI 0xBFAC). */
    public byte[] pack() {
        byte[] buf = new byte[156];
        buf[0] = (byte) refBinOffset;
        buf[1] = (byte) (refBinOffset >> 8);
        buf[2] = (byte) (refBinOffset >> 16);
        buf[3] = (byte) (refBinOffset >> 24);
        // detection threshold, extra noise sigma, null den clip, mem update mode/choice: 0
        buf[18] = (byte) sumSpan;
        buf[19] = (byte) featureLength;
        buf[20] = (byte) nbOfAggregates;
        buf[21] = 1; // nb_of_temporal_accumulations
        buf[25] = (byte) cnhCfg;
        buf[26] = (byte) cnhFlexShift;
        for (int k = 0; k < 64; k++) {
            buf[28 + k] = (byte) mapId[k];
        }
        return buf;
    }

    private static int calcRequiredMemory(int flags, int nbAgg, int featLength) {
        int aggXFeat = nbAgg * featLength;
        int size = PER_BUFFER_HEADER_WORDS * 4;
        size += aggXFeat * 4;                 // FEAT_INT
        size += (3 + aggXFeat) / 4 * 4;       // FEAT_FRAC (8 bits each, padded)
        size += nbAgg * 4;                    // AMBIENT_INT
        size += (3 + nbAgg) / 4 * 4;          // AMBIENT_FRAC
        if ((flags & DISABLE_VARIANCE) == 0) {
            size += aggXFeat * 4;             // VARIANCE_INT
            size += (3 + aggXFeat) / 4 * 4;   // VARIANCE_FRAC
        }
        size = size / 4 * 4;
        if ((flags & DISABLE_PING_PONG) == 0) {
            size *= 2;
        }
        return size + PER_HEADER_WORDS * 4;
    }
}
