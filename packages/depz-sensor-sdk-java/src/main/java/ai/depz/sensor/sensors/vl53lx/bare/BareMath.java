package ai.depz.sensor.sensors.vl53lx.bare;

/** SPAD geometry and timing maths (vl53lx_core.c, vl53lx_core_support.c). */
public final class BareMath {
    private BareMath() {}

    public static int encodeRowCol(int row, int col) {
        return row > 7 ? (128 + (col << 3) + (15 - row)) & 0xFF : (((15 - col) << 3) + row) & 0xFF;
    }

    /** → {row, col}. */
    public static int[] decodeRowCol(int spadNumber) {
        return spadNumber > 127
                ? new int[] {8 + ((255 - spadNumber) & 0x07), (spadNumber - 128) >> 3}
                : new int[] {spadNumber & 0x07, (127 - spadNumber) >> 3};
    }

    /** → {width, height}. */
    public static int[] decodeZoneSize(int encodedXySize) {
        return new int[] {encodedXySize & 0x0F, encodedXySize >> 4};
    }

    public static int encodeZoneSize(int width, int height) {
        return ((height << 4) + width) & 0xFF;
    }

    /** → {x_ll, y_ll, x_ur, y_ur}, clipped to the SPAD array. */
    public static int[] decodeZoneLimits(int encodedXyCentre, int encodedXySize) {
        int[] rc = decodeRowCol(encodedXyCentre);
        int yCentre = rc[0];
        int xCentre = rc[1];
        int[] wh = decodeZoneSize(encodedXySize);
        int width = wh[0];
        int height = wh[1];
        int xLl = Math.max(0, xCentre - Math.floorDiv(width + 1, 2));
        int xUr = Math.min(BareRegs.SPAD_ARRAY_WIDTH - 1, xLl + width);
        int yLl = Math.max(0, yCentre - Math.floorDiv(height + 1, 2));
        int yUr = Math.min(BareRegs.SPAD_ARRAY_HEIGHT - 1, yLl + height);
        return new int[] {xLl, yLl, xUr, yUr};
    }

    public static boolean isApertureLocation(int row, int col) {
        return (row % 4 == 0 && col % 4 == 2) || (row % 4 == 2 && col % 4 == 0);
    }

    /**
     * → {mm inner, mm outer} effective SPADs, in 1/256 of a SPAD. Needed because our boards ship
     * without the FMT offset calibration: the driver works out the SPAD counts the nominal rates
     * apply to.
     */
    public static long[] calcMmEffectiveSpads(int encodedMmRoiCentre, int encodedMmRoiSize, int encodedZoneCentre,
            int encodedZoneSize, byte[] goodSpads, long apertureAttenuation) {
        int[] mm = decodeZoneLimits(encodedMmRoiCentre, encodedMmRoiSize);
        int[] z = decodeZoneLimits(encodedZoneCentre, encodedZoneSize);
        long inner = 0;
        long outer = 0;
        for (int y = z[1]; y <= z[3]; y++) {
            for (int x = z[0]; x <= z[2]; x++) {
                int spad = encodeRowCol(y, x);
                if ((goodSpads[spad >> 3] & (1 << (spad & 0x07))) == 0) {
                    continue;
                }
                long attenuation = isApertureLocation(y, x) ? apertureAttenuation : BareRegs.RTN_SPAD_UNITY_TRANSMISSION;
                if (mm[0] <= x && x <= mm[2] && mm[1] <= y && y <= mm[3]) {
                    inner += attenuation;
                } else {
                    outer += attenuation;
                }
            }
        }
        return new long[] {inner, outer};
    }

    /**
     * {@code VL53LX_hist_combine_mm1_mm2_offsets()} → {@code range_offset_mm}, in quarter
     * millimetres: the MM1 (inner) and MM2 (outer) offsets weighted by the share of each region the
     * ROI covers.
     */
    public static long combineMm1Mm2Offsets(long mm1OffsetMm, long mm2OffsetMm, int encodedMmRoiCentre,
            int encodedMmRoiSize, int encodedZoneCentre, int encodedZoneSize, NvmReader.AdditionalOffsetCalData cal,
            byte[] goodSpads, long apertureAttenuation) {
        long[] max = calcMmEffectiveSpads(encodedMmRoiCentre, encodedMmRoiSize, 0xC7, 0xFF, goodSpads,
                apertureAttenuation);
        if (max[0] == 0 || max[1] == 0) {
            return 0;
        }
        long[] cur = calcMmEffectiveSpads(encodedMmRoiCentre, encodedMmRoiSize, encodedZoneCentre, encodedZoneSize,
                goodSpads, apertureAttenuation);
        long mm1Rate = cal.mmInnerPeakSignalCountRtnMcps() * cur[0] / max[0];
        long mm2Rate = cal.mmOuterPeakSignalCountRtnMcps() * cur[1] / max[1];
        long total = mm1Rate + mm2Rate;
        if (total == 0) {
            return 0;
        }
        // Signed offsets: C truncates towards zero, which Java division does too.
        return (mm1OffsetMm * mm1Rate + mm2OffsetMm * mm2Rate) * 4 / total;
    }

    /** The 32 {@code global_config__spad_enables_rtn_*} bytes as one bitmap. */
    public static byte[] rtnGoodSpads(RegBlock nvmCopyData) {
        byte[] out = new byte[32];
        for (int i = 0; i < 32; i++) {
            out[i] = (byte) nvmCopyData.get("global_config__spad_enables_rtn_" + i);
        }
        return out;
    }

    public static long calcPllPeriodUs(long fastOscFrequency) {
        return fastOscFrequency > 0 ? (1L << 30) / fastOscFrequency : 0;
    }

    public static long decodeVcselPeriod(long vcselPeriodReg) {
        return (vcselPeriodReg + 1) << 1;
    }

    public static long calcMacroPeriodUs(long fastOscFrequency, long vcselPeriodReg) {
        long macro = BareRegs.MACRO_PERIOD_VCSEL_PERIODS * calcPllPeriodUs(fastOscFrequency);
        macro >>= 6;
        macro *= decodeVcselPeriod(vcselPeriodReg);
        return macro >> 6;
    }

    public static long calcTimeoutMclks(long timeoutUs, long macroPeriodUs) {
        return macroPeriodUs == 0 ? 0 : ((timeoutUs << 12) + (macroPeriodUs >> 1)) / macroPeriodUs;
    }

    public static long encodeTimeout(long timeoutMclks) {
        if (timeoutMclks <= 0) {
            return 0;
        }
        long ls = timeoutMclks - 1;
        long ms = 0;
        while ((ls & 0xFFFFFF00L) != 0) {
            ls >>= 1;
            ms++;
        }
        return ((ms << 8) + (ls & 0xFF)) & 0xFFFF;
    }

    public static long decodeTimeout(long encoded) {
        return ((encoded & 0x00FF) << (int) ((encoded & 0xFF00) >> 8)) + 1;
    }

    public static long calcEncodedTimeout(long timeoutUs, long macroPeriodUs) {
        return encodeTimeout(calcTimeoutMclks(timeoutUs, macroPeriodUs));
    }

    public static long durationMaths(long pllPeriodUs, long vcselParmPclks, long windowVclks, long elapsedMclks) {
        long duration = (windowVclks * pllPeriodUs) >> 12;
        duration *= (elapsedMclks * vcselParmPclks) >> 4;
        return Math.min(duration >> 12, 0xFFFFFFFFL);
    }

    /** Fill the six timeout registers, for VCSEL period A and then B. */
    public static void calcTimeoutRegisterValues(long phasecalUs, long mmUs, long rangeUs, long fastOscFrequency,
            RegBlock gen, RegBlock tim) {
        if (fastOscFrequency == 0) {
            throw new ArithmeticException("osc_measured__fast_osc__frequency is 0");
        }
        long macro = calcMacroPeriodUs(fastOscFrequency, tim.get("range_config__vcsel_period_a"));
        gen.set("phasecal_config__timeout_macrop", Math.min(0xFF, calcTimeoutMclks(phasecalUs, macro)));
        long encoded = calcEncodedTimeout(mmUs, macro);
        tim.set("mm_config__timeout_macrop_a_hi", encoded >> 8);
        tim.set("mm_config__timeout_macrop_a_lo", encoded & 0xFF);
        encoded = calcEncodedTimeout(rangeUs, macro);
        tim.set("range_config__timeout_macrop_a_hi", encoded >> 8);
        tim.set("range_config__timeout_macrop_a_lo", encoded & 0xFF);

        macro = calcMacroPeriodUs(fastOscFrequency, tim.get("range_config__vcsel_period_b"));
        encoded = calcEncodedTimeout(mmUs, macro);
        tim.set("mm_config__timeout_macrop_b_hi", encoded >> 8);
        tim.set("mm_config__timeout_macrop_b_lo", encoded & 0xFF);
        encoded = calcEncodedTimeout(rangeUs, macro);
        tim.set("range_config__timeout_macrop_b_hi", encoded >> 8);
        tim.set("range_config__timeout_macrop_b_lo", encoded & 0xFF);
    }
}
