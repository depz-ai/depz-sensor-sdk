package ai.depz.sensor.sensors.vl53lx.bare;

import ai.depz.sensor.sensors.vl53lx.Vl53lxPlatform;
import java.io.ByteArrayOutputStream;
import java.util.List;

/**
 * The raw NVM, which the register blocks do not cover: the optical centre, the 5x5 peak-rate map,
 * the mode-mitigation offset calibration and the FMT range results. Each word costs four writes
 * and a read, so only the regions the driver wants are read, each inside its own enable/disable
 * pair so the sensor is never left with its firmware stopped (vl53lx_nvm.c).
 */
public final class NvmReader {
    /** {@code VL53LX_optical_centre_t}. */
    public record OpticalCentre(int xCentre, int yCentre) {}

    /** {@code VL53LX_cal_peak_rate_map_t}. */
    public record CalPeakRateMap(int calDistanceMm, int calReflectancePc, int maxSamples, int width, int height,
            List<Integer> peakRateMcps) {}

    /** {@code VL53LX_additional_offset_cal_data_t}. */
    public record AdditionalOffsetCalData(long mmInnerActualEffectiveSpads, long mmOuterActualEffectiveSpads,
            long mmInnerPeakSignalCountRtnMcps, long mmOuterPeakSignalCountRtnMcps) {}

    /** {@code VL53LX_decoded_nvm_fmt_range_data_t}. */
    public record FmtRangeData(int actualEffectiveRtnSpads, int numRequestedRefSpads, int refLocation,
            int peakSignalCountRateRtnMcps, int ambientCountRateRtnMcps, int peakSignalCountRateRefMcps,
            int ambientCountRateRefMcps, int measuredDistanceMm, int measuredDistanceStdevMm) {}

    private static final int POWER_MANAGEMENT_GO1_POWER_FORCE = 0x0083;
    private static final int FIRMWARE_ENABLE = 0x0085;
    private static final int RANGING_CORE_CLK_CTRL1 = 0x0683;
    private static final int NVM_CTRL_MODE = 0x0780;
    private static final int NVM_CTRL_PDN = 0x0781;
    private static final int NVM_CTRL_READN = 0x0783;
    private static final int NVM_CTRL_PULSE_WIDTH_MSB = 0x0784;
    private static final int NVM_CTRL_DATAOUT_MMM = 0x0790;
    private static final int NVM_CTRL_ADDR = 0x0794;
    private static final int NVM_CTRL_PULSE_WIDTH = 0x0004;

    /** NVM regions, {byte index, byte size}. */
    public static final int[] FMT_OPTICAL_CENTRE = {0x00B8, 4};
    public static final int[] FMT_CAL_PEAK_RATE_MAP = {0x015C, 56};
    public static final int[] FMT_ADDITIONAL_OFFSET_CAL = {0x0194, 8};
    public static final int[] FMT_RANGE_RESULTS_140MM_DARK = {0x01AC, 16};

    private static final int PEAK_RATE_MAP_WIDTH = 5;
    private static final int PEAK_RATE_MAP_HEIGHT = 5;
    private static final int PEAK_RATE_MAP_SAMPLES = 25;

    private final Vl53lxPlatform p;
    private final DeviceImage img;

    public NvmReader(Vl53lxPlatform platform, DeviceImage image) {
        p = platform;
        img = image;
    }

    private static int u16(byte[] buf, int off) {
        return (buf[off] & 0xFF) << 8 | (buf[off + 1] & 0xFF);
    }

    private void enable() {
        // disable_firmware / enable_powerforce, through system_control so the image stays truthful.
        img.systemControl().set("firmware__enable", 0);
        p.writeByte(FIRMWARE_ENABLE, 0);
        img.systemControl().set("power_management__go1_power_force", 1);
        p.writeByte(POWER_MANAGEMENT_GO1_POWER_FORCE, 1);
        p.sleepMs(1); // 250 us settling, rounded up

        p.writeByte(NVM_CTRL_PDN, 0x01);
        p.writeByte(RANGING_CORE_CLK_CTRL1, 0x05);
        p.sleepMs(1); // 50 us power-up, rounded up
        p.writeByte(NVM_CTRL_MODE, 0x01);
        p.writeWord(NVM_CTRL_PULSE_WIDTH_MSB, NVM_CTRL_PULSE_WIDTH);
    }

    private void disable() {
        p.writeByte(NVM_CTRL_READN, 0x01);
        p.writeByte(NVM_CTRL_PDN, 0x00);
        img.systemControl().set("power_management__go1_power_force", 0);
        p.writeByte(POWER_MANAGEMENT_GO1_POWER_FORCE, 0);
        img.systemControl().set("firmware__enable", 1);
        p.writeByte(FIRMWARE_ENABLE, 1);
    }

    private byte[] readWords(int startWord, int count) {
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        for (int addr = startWord; addr < startWord + count; addr++) {
            p.writeByte(NVM_CTRL_ADDR, addr);
            p.writeByte(NVM_CTRL_READN, 0x00);
            // 5 us trigger delay: the next USB round trip is the wait.
            p.writeByte(NVM_CTRL_READN, 0x01);
            output.writeBytes(p.readMulti(NVM_CTRL_DATAOUT_MMM, 4));
        }
        return output.toByteArray();
    }

    /** VL53LX_read_nvm_raw_data(): {byte index, byte size}, both word-aligned → the bytes. */
    public byte[] readRegion(int[] region) {
        int index = region[0];
        int size = region[1];
        if ((index & 3) != 0 || (size & 3) != 0) {
            throw new IllegalArgumentException(String.format("NVM region 0x%04X+%d is not word-aligned", index, size));
        }
        enable();
        try {
            return readWords(index >> 2, size >> 2);
        } finally {
            disable();
        }
    }

    public OpticalCentre readOpticalCentre() {
        byte[] buf = readRegion(FMT_OPTICAL_CENTRE);
        // Stored as a distance down from 0x0100; on a never-programmed part the driver zeroes it, which is
        // what makes read_p2p_data fall back to the mm ROI centre.
        int x = 0x0100 - (buf[2] & 0xFF);
        return new OpticalCentre(x <= 0xFF ? x : 0, buf[3] & 0xFF);
    }

    public CalPeakRateMap readCalPeakRateMap() {
        byte[] buf = readRegion(FMT_CAL_PEAK_RATE_MAP);
        Integer[] rates = new Integer[PEAK_RATE_MAP_SAMPLES];
        for (int i = 0; i < PEAK_RATE_MAP_SAMPLES; i++) {
            rates[i] = u16(buf, 4 + 2 * i);
        }
        return new CalPeakRateMap(u16(buf, 0), u16(buf, 2) >> 6, PEAK_RATE_MAP_SAMPLES, PEAK_RATE_MAP_WIDTH,
                PEAK_RATE_MAP_HEIGHT, List.of(rates));
    }

    public AdditionalOffsetCalData readAdditionalOffsetCalData() {
        byte[] buf = readRegion(FMT_ADDITIONAL_OFFSET_CAL);
        return new AdditionalOffsetCalData(u16(buf, 0), u16(buf, 2), u16(buf, 4), u16(buf, 6));
    }

    public FmtRangeData readFmtRangeResults() {
        return readFmtRangeResults(FMT_RANGE_RESULTS_140MM_DARK);
    }

    public FmtRangeData readFmtRangeResults(int[] region) {
        byte[] buf = readRegion(region);
        return new FmtRangeData(u16(buf, 0), buf[2] & 0xFF, buf[3] & 0xFF, u16(buf, 4), u16(buf, 6), u16(buf, 8),
                u16(buf, 10), u16(buf, 12), u16(buf, 14));
    }
}
