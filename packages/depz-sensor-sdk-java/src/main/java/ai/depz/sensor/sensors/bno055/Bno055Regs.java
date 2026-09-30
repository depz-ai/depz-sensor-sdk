package ai.depz.sensor.sensors.bno055;

import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Locale;
import java.util.Map;

/**
 * BNO055 register map and the pure codecs every SDK shares
 * (contracts/13_SENSOR_BNO055.md §4, Bosch BST-BNO055-DS000 rev 1.8) — the
 * "base" every SDK implements. Mirrors {@code depz_sensor_sdk/bno055/regs.py} 1:1.
 *
 * <p>Nothing here touches the wire: these functions turn register bytes into
 * values and back, so they are what {@code vectors/bno055.json} pins. Scaling
 * raw integers to physical units is {@code raw / LSB} with the LSB constants of
 * {@link Units} and the fixed {@link #MAG_LSB} / {@link #QUAT_LSB} /
 * {@link #FUSION_ACCEL_LSB}.
 *
 * <p>OUT OF SCOPE (as for every Java sensor): the live driver over the bridge
 * (mode switching, boot / fusion-start polls, page discipline, self-test).
 */
public final class Bno055Regs {
    private Bno055Regs() {}

    // ── page 0 ───────────────────────────────────────────────────────────────
    public static final int REG_CHIP_ID = 0x00;
    public static final int REG_PAGE_ID = 0x07;
    public static final int REG_ACC_DATA = 0x08;          // x, y, z  i16
    public static final int REG_MAG_DATA = 0x0E;
    public static final int REG_GYR_DATA = 0x14;
    public static final int REG_EUL_DATA = 0x1A;          // heading, roll, pitch
    public static final int REG_QUA_DATA = 0x20;          // w, x, y, z
    public static final int REG_LIA_DATA = 0x28;          // linear acceleration (gravity removed)
    public static final int REG_GRV_DATA = 0x2E;          // gravity vector
    public static final int REG_TEMP = 0x34;              // i8
    public static final int REG_CALIB_STAT = 0x35;
    public static final int REG_ST_RESULT = 0x36;
    /** Clear-on-read — never part of a routine block read. */
    public static final int REG_INT_STA = 0x37;
    public static final int REG_SYS_CLK_STATUS = 0x38;
    public static final int REG_SYS_STATUS = 0x39;
    public static final int REG_SYS_ERR = 0x3A;
    public static final int REG_UNIT_SEL = 0x3B;
    public static final int REG_OPR_MODE = 0x3D;
    public static final int REG_PWR_MODE = 0x3E;
    public static final int REG_SYS_TRIGGER = 0x3F;
    public static final int REG_TEMP_SOURCE = 0x40;
    public static final int REG_AXIS_MAP_CONFIG = 0x41;
    public static final int REG_AXIS_MAP_SIGN = 0x42;
    /** 9 × i16, row-major, 1.0 = 16384. */
    public static final int REG_SIC_MATRIX = 0x43;
    /** acc/mag/gyr offsets + acc/mag radius, {@value #CALIB_PROFILE_LEN} bytes. */
    public static final int REG_CALIB_PROFILE = 0x55;
    public static final int CALIB_PROFILE_LEN = 22;

    // ── page 1 ───────────────────────────────────────────────────────────────
    public static final int REG1_ACC_CONFIG = 0x08;
    public static final int REG1_MAG_CONFIG = 0x09;
    public static final int REG1_GYR_CONFIG_0 = 0x0A;
    public static final int REG1_GYR_CONFIG_1 = 0x0B;
    public static final int REG1_ACC_SLEEP_CONFIG = 0x0C;
    public static final int REG1_GYR_SLEEP_CONFIG = 0x0D;
    public static final int REG1_INT_MSK = 0x0F;
    public static final int REG1_INT_EN = 0x10;
    /** Page-1 0x11..0x1F are the motion-interrupt settings, written raw. */
    public static final int REG1_ACC_AM_THRES = 0x11;
    public static final int REG1_GYR_AM_SET = 0x1F;
    public static final int REG1_UNIQUE_ID = 0x50;        // 16 bytes
    public static final int UNIQUE_ID_LEN = 16;

    /** The block that carries every output channel: 0x08 (ACC_DATA_X_LSB) … 0x35 (CALIB_STAT). */
    public static final int FULL_BLOCK_ADDR = REG_ACC_DATA;
    public static final int FULL_BLOCK_LEN = REG_CALIB_STAT - REG_ACC_DATA + 1;   // 46
    /** Quaternion only — the cheapest orientation read (8 bytes, ~1.2 ms of bus). */
    public static final int QUAT_BLOCK_ADDR = REG_QUA_DATA;
    public static final int QUAT_BLOCK_LEN = 8;

    // SYS_TRIGGER bits (preserve CLK_SEL when writing).
    public static final int SYS_TRIGGER_SELF_TEST = 0x01;
    public static final int SYS_TRIGGER_RST_SYS = 0x20;
    public static final int SYS_TRIGGER_RST_INT = 0x40;
    public static final int SYS_TRIGGER_CLK_SEL = 0x80;

    // INT_EN / INT_MSK / INT_STA bits. The DRDY bits drive the pin only on
    // sensor firmware 03.14+; the boards in this line carry 03.11.
    public static final int INT_ACC_BSX_DRDY = 0x01;
    public static final int INT_MAG_DRDY = 0x02;
    public static final int INT_GYR_AM = 0x04;
    public static final int INT_GYR_HIGH_RATE = 0x08;
    public static final int INT_GYR_DRDY = 0x10;
    public static final int INT_ACC_HIGH_G = 0x20;
    public static final int INT_ACC_AM = 0x40;
    public static final int INT_ACC_NM = 0x80;

    // ST_RESULT bits (1 = passed).
    public static final int ST_ACC = 0x01;
    public static final int ST_MAG = 0x02;
    public static final int ST_GYR = 0x04;
    public static final int ST_MCU = 0x08;
    public static final int EXPECTED_SELF_TEST = ST_ACC | ST_MAG | ST_GYR | ST_MCU;

    /** OPR_MODE (0x3D) bits 3:0 (the register reads back 0x10 after reset — mask). */
    public enum OprMode {
        CONFIG(0x00), ACCONLY(0x01), MAGONLY(0x02), GYROONLY(0x03), ACCMAG(0x04),
        ACCGYRO(0x05), MAGGYRO(0x06), AMG(0x07), IMU(0x08), COMPASS(0x09), M4G(0x0A),
        NDOF_FMC_OFF(0x0B), NDOF(0x0C);

        public final int value;
        OprMode(int value) { this.value = value; }

        public boolean isFusion() {
            return value >= IMU.value;
        }
    }

    /** PWR_MODE (0x3E) bits 1:0. */
    public enum PwrMode {
        NORMAL(0x00), LOW_POWER(0x01), SUSPEND(0x02);

        public final int value;
        PwrMode(int value) { this.value = value; }
    }

    /** TEMP_SOURCE (0x40) bits 1:0. */
    public enum TempSource {
        ACCEL(0x00), GYRO(0x01);

        public final int value;
        TempSource(int value) { this.value = value; }
    }

    // ── units (UNIT_SEL 0x3B) ────────────────────────────────────────────────
    // Bits as the silicon implements them (datasheet Table 3-11 and Bosch's
    // driver; the §4.3.60 bit table is off by one — contract 13 §4.2).

    public static final int UNIT_ACC_MG = 0x01;
    public static final int UNIT_GYR_RPS = 0x02;
    public static final int UNIT_EUL_RAD = 0x04;
    public static final int UNIT_TEMP_F = 0x10;
    public static final int UNIT_ORI_ANDROID = 0x80;

    /**
     * Output units. The SDK default ({@link #DEFAULT}, UNIT_SEL = 0x00) is m/s²,
     * dps, degrees, °C, Windows orientation. The sensor's own power-on value is
     * 0x80 (Android), so a fresh sensor must be told. Unknown bits are dropped.
     */
    public record Units(boolean accelMg, boolean gyroRps, boolean eulerRad, boolean tempF,
                        boolean android) {
        public static final Units DEFAULT = new Units(false, false, false, false, false);

        public int pack() {
            return (accelMg ? UNIT_ACC_MG : 0)
                | (gyroRps ? UNIT_GYR_RPS : 0)
                | (eulerRad ? UNIT_EUL_RAD : 0)
                | (tempF ? UNIT_TEMP_F : 0)
                | (android ? UNIT_ORI_ANDROID : 0);
        }

        public static Units unpack(int value) {
            return new Units(
                (value & UNIT_ACC_MG) != 0,
                (value & UNIT_GYR_RPS) != 0,
                (value & UNIT_EUL_RAD) != 0,
                (value & UNIT_TEMP_F) != 0,
                (value & UNIT_ORI_ANDROID) != 0);
        }

        /** ACC_DATA LSB: 1 per mg, else 100 per m/s² (LIA/GRV use {@link #FUSION_ACCEL_LSB}). */
        public double accelLsb() {
            return accelMg ? 1.0 : 100.0;
        }

        /** Angular-rate LSB: 900 per rad/s, else 16 per dps. */
        public double gyroLsb() {
            return gyroRps ? 900.0 : 16.0;
        }

        /** Euler LSB: 900 per radian, else 16 per degree. */
        public double eulerLsb() {
            return eulerRad ? 900.0 : 16.0;
        }

        /** Temperature LSB: 1 LSB = 2 °F (0.5 LSB/°F), else 1 LSB = 1 °C. */
        public double tempLsb() {
            return tempF ? 0.5 : 1.0;
        }
    }

    /** µT, not selectable. */
    public static final double MAG_LSB = 16.0;
    /** 2^14, unit-less. */
    public static final double QUAT_LSB = 16384.0;
    /**
     * Linear acceleration and gravity ignore the ACC_Unit bit: always m/s² at
     * 100 LSB — measured on SW rev 03.11 (datasheet Tables 3-33/3-35 claim mg).
     */
    public static final double FUSION_ACCEL_LSB = 100.0;

    // ── calibration ──────────────────────────────────────────────────────────

    /** CALIB_STAT (0x35) {@code sys<7:6> gyr<5:4> acc<3:2> mag<1:0>}: 0 = not calibrated … 3 = fully. */
    public record CalibStatus(int system, int gyro, int accel, int mag) {
        public static CalibStatus unpack(int value) {
            return new CalibStatus(value >> 6 & 3, value >> 4 & 3, value >> 2 & 3, value & 3);
        }

        public int pack() {
            return (system & 3) << 6 | (gyro & 3) << 4 | (accel & 3) << 2 | (mag & 3);
        }

        public boolean fullyCalibrated() {
            return system == 3 && gyro == 3 && accel == 3 && mag == 3;
        }
    }

    /**
     * Sensor offsets and radii, registers 0x55..0x6A (22 bytes, 11 × i16 LE:
     * acc_offset xyz, mag_offset xyz, gyr_offset xyz, acc_radius, mag_radius).
     * Readable and writable only in CONFIG; write all 22 bytes in one transfer
     * (the sensor latches each group on its MSB). Offsets are in sensor LSB and do
     * not depend on UNIT_SEL.
     */
    public record CalibrationProfile(int[] accelOffset, int[] magOffset, int[] gyroOffset,
                                     int accelRadius, int magRadius) {
        public byte[] pack() {
            int[] v = {
                accelOffset[0], accelOffset[1], accelOffset[2],
                magOffset[0], magOffset[1], magOffset[2],
                gyroOffset[0], gyroOffset[1], gyroOffset[2],
                accelRadius, magRadius};
            return packI16(v);
        }

        /** Throws {@link IllegalArgumentException} unless {@code data} is exactly 22 bytes. */
        public static CalibrationProfile unpack(byte[] data) {
            if (data.length != CALIB_PROFILE_LEN) {
                throw new IllegalArgumentException(
                    "calibration profile is " + CALIB_PROFILE_LEN + " bytes, got " + data.length);
            }
            int[] v = unpackI16(data, 0, 11);
            return new CalibrationProfile(
                new int[] {v[0], v[1], v[2]},
                new int[] {v[3], v[4], v[5]},
                new int[] {v[6], v[7], v[8]},
                v[9], v[10]);
        }
    }

    /** Soft-iron matrix identity, 1.0 = 16384. */
    public static final int[] SIC_IDENTITY = {16384, 0, 0, 0, 16384, 0, 0, 0, 16384};

    /** Soft-iron matrix, 9 × i16 row-major, 1.0 = 16384 (datasheet §3.11.4). */
    public static byte[] packSicMatrix(int[] m) {
        if (m.length != 9) {
            throw new IllegalArgumentException("SIC matrix has 9 elements");
        }
        return packI16(m);
    }

    public static int[] unpackSicMatrix(byte[] data) {
        return unpackI16(data, 0, 9);
    }

    // ── axis remap (AXIS_MAP_CONFIG 0x41 / AXIS_MAP_SIGN 0x42) ───────────────

    public static final int AXIS_X = 0;
    public static final int AXIS_Y = 1;
    public static final int AXIS_Z = 2;

    /**
     * Which chip axis feeds each output axis, and its sign. {@code x = AXIS_Y}
     * means "output X is the chip's Y axis". {@code AXIS_MAP_CONFIG = z<5:4>
     * y<3:2> x<1:0>}, {@code AXIS_MAP_SIGN = x 2, y 1, z 0} (1 = negative).
     */
    public record AxisRemap(int x, int y, int z, boolean xNegative, boolean yNegative,
                            boolean zNegative) {
        /** P1, the power-on mapping. */
        public static final AxisRemap DEFAULT = new AxisRemap(AXIS_X, AXIS_Y, AXIS_Z, false, false, false);

        /**
         * → {@code {AXIS_MAP_CONFIG, AXIS_MAP_SIGN}}. The sensor keeps its old
         * mapping when given one that uses an axis twice, so this refuses it up
         * front with {@link IllegalArgumentException}.
         */
        public int[] pack() {
            boolean[] seen = new boolean[3];
            for (int a : new int[] {x, y, z}) {
                if (a < 0 || a > 2 || seen[a]) {
                    throw new IllegalArgumentException(
                        "axis remap must be a permutation of X/Y/Z, got " + this);
                }
                seen[a] = true;
            }
            int config = z << 4 | y << 2 | x;
            int sign = (xNegative ? 4 : 0) | (yNegative ? 2 : 0) | (zNegative ? 1 : 0);
            return new int[] {config, sign};
        }

        public static AxisRemap unpack(int config, int sign) {
            return new AxisRemap(
                config & 3, config >> 2 & 3, config >> 4 & 3,
                (sign & 4) != 0, (sign & 2) != 0, (sign & 1) != 0);
        }

        /**
         * Datasheet §3.4 mounting presets P0..P7 (case-insensitive; P1 is the
         * default); throws {@link IllegalArgumentException} for another name.
         */
        public static AxisRemap placement(String name) {
            int[] p = PLACEMENTS.get(name.toUpperCase(Locale.ROOT));
            if (p == null) {
                throw new IllegalArgumentException("unknown placement '" + name + "'; expected P0..P7");
            }
            return unpack(p[0], p[1]);
        }
    }

    /** Datasheet §3.4: placement → {@code {AXIS_MAP_CONFIG, AXIS_MAP_SIGN}}, P0..P7 in order. */
    public static final Map<String, int[]> PLACEMENTS;
    static {
        Map<String, int[]> m = new LinkedHashMap<>();
        m.put("P0", new int[] {0x21, 0x04});
        m.put("P1", new int[] {0x24, 0x00});
        m.put("P2", new int[] {0x24, 0x06});
        m.put("P3", new int[] {0x21, 0x02});
        m.put("P4", new int[] {0x24, 0x03});
        m.put("P5", new int[] {0x21, 0x01});
        m.put("P6", new int[] {0x21, 0x07});
        m.put("P7", new int[] {0x24, 0x05});
        PLACEMENTS = Collections.unmodifiableMap(m);
    }

    // ── page-1 sensor configuration (non-fusion modes only) ──────────────────

    public static final int[] ACC_RANGE_G = {2, 4, 8, 16};
    public static final double[] ACC_BANDWIDTH_HZ = {7.81, 15.63, 31.25, 62.5, 125.0, 250.0, 500.0, 1000.0};
    public static final String[] ACC_POWER_NAMES =
        {"normal", "suspend", "low power 1", "standby", "low power 2", "deep suspend"};
    public static final int[] GYR_RANGE_DPS = {2000, 1000, 500, 250, 125};
    public static final int[] GYR_BANDWIDTH_HZ = {523, 230, 116, 47, 23, 12, 64, 32};
    public static final String[] GYR_POWER_NAMES =
        {"normal", "fast power up", "deep suspend", "suspend", "advanced powersave"};
    public static final int[] MAG_RATE_HZ = {2, 6, 8, 10, 15, 20, 25, 30};
    public static final String[] MAG_OPR_NAMES = {"low power", "regular", "enhanced regular", "high accuracy"};
    public static final String[] MAG_POWER_NAMES = {"normal", "sleep", "suspend", "force"};

    /**
     * ACC_CONFIG (page 1, 0x08) as register codes {@code range<1:0>
     * bandwidth<4:2> power<7:5>}: {@code range} indexes {@link #ACC_RANGE_G},
     * {@code bandwidth} {@link #ACC_BANDWIDTH_HZ}, {@code power}
     * {@link #ACC_POWER_NAMES}. Power-on value 0x0D = ±4 g, 62.5 Hz, normal.
     */
    public record AccelConfig(int range, int bandwidth, int power) {
        public static final AccelConfig DEFAULT = new AccelConfig(1, 3, 0);

        public int pack() {
            return (power & 7) << 5 | (bandwidth & 7) << 2 | (range & 3);
        }

        public static AccelConfig unpack(int value) {
            return new AccelConfig(value & 3, value >> 2 & 7, value >> 5 & 7);
        }
    }

    /**
     * GYR_CONFIG_0/1 (page 1, 0x0A/0x0B), 2 bytes: {@code byte0 = range<2:0>
     * bandwidth<5:3>}, {@code byte1 = power<2:0>}. Indexes {@link #GYR_RANGE_DPS},
     * {@link #GYR_BANDWIDTH_HZ}, {@link #GYR_POWER_NAMES}. Power-on 0x38/0x00 =
     * 2000 dps, 32 Hz, normal.
     */
    public record GyroConfig(int range, int bandwidth, int power) {
        public static final GyroConfig DEFAULT = new GyroConfig(0, 7, 0);

        public byte[] pack() {
            return new byte[] {(byte) ((bandwidth & 7) << 3 | (range & 7)), (byte) (power & 7)};
        }

        public static GyroConfig unpack(byte[] data) {
            int b0 = data[0] & 0xFF;
            return new GyroConfig(b0 & 7, b0 >> 3 & 7, data[1] & 7);
        }
    }

    /**
     * MAG_CONFIG (page 1, 0x09) {@code rate<2:0> mode<4:3> power<6:5>}: indexes
     * {@link #MAG_RATE_HZ}, {@link #MAG_OPR_NAMES}, {@link #MAG_POWER_NAMES}.
     * Bit 7 is not a field: {@code pack(unpack(v)) == v & 0x7F}. Power-on 0x0B =
     * 10 Hz, regular, normal.
     */
    public record MagConfig(int rate, int mode, int power) {
        public static final MagConfig DEFAULT = new MagConfig(3, 1, 0);

        public int pack() {
            return (power & 3) << 5 | (mode & 3) << 3 | (rate & 7);
        }

        public static MagConfig unpack(int value) {
            return new MagConfig(value & 7, value >> 3 & 3, value >> 5 & 3);
        }
    }

    // ── output block decode ──────────────────────────────────────────────────

    /**
     * Raw register values found in one block read. A channel is {@code null} when
     * the window {@code addr..addr+len} does not cover all of its bytes. Euler is
     * (heading, roll, pitch), quaternion (w, x, y, z); vectors are (x, y, z).
     */
    public record RawBlock(int[] accel, int[] mag, int[] gyro, int[] euler, int[] quaternion,
                           int[] linearAccel, int[] gravity, Integer temperature,
                           Integer calibStat) {}

    /** Unpack whatever channels the register window starting at {@code addr} holds. */
    public static RawBlock decodeBlock(int addr, byte[] data) {
        int end = addr + data.length;
        Integer temp = null;
        if (addr <= REG_TEMP && REG_TEMP < end) {
            temp = (int) data[REG_TEMP - addr];                       // i8
        }
        Integer calib = null;
        if (addr <= REG_CALIB_STAT && REG_CALIB_STAT < end) {
            calib = data[REG_CALIB_STAT - addr] & 0xFF;
        }
        return new RawBlock(
            channel(addr, data, REG_ACC_DATA, 3),
            channel(addr, data, REG_MAG_DATA, 3),
            channel(addr, data, REG_GYR_DATA, 3),
            channel(addr, data, REG_EUL_DATA, 3),
            channel(addr, data, REG_QUA_DATA, 4),
            channel(addr, data, REG_LIA_DATA, 3),
            channel(addr, data, REG_GRV_DATA, 3),
            temp,
            calib);
    }

    private static int[] channel(int addr, byte[] data, int reg, int words) {
        if (addr <= reg && reg + 2 * words <= addr + data.length) {
            return unpackI16(data, reg - addr, words);
        }
        return null;
    }

    // ── LE helpers ───────────────────────────────────────────────────────────

    private static int[] unpackI16(byte[] b, int off, int n) {
        int[] out = new int[n];
        for (int i = 0; i < n; i++) {
            out[i] = (short) ((b[off + 2 * i] & 0xFF) | ((b[off + 2 * i + 1] & 0xFF) << 8));
        }
        return out;
    }

    private static byte[] packI16(int[] v) {
        byte[] out = new byte[2 * v.length];
        for (int i = 0; i < v.length; i++) {
            out[2 * i] = (byte) v[i];
            out[2 * i + 1] = (byte) (v[i] >>> 8);
        }
        return out;
    }
}
