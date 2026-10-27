package ai.depz.sensor.sensors.vl53lx.bare;

import ai.depz.sensor.sensors.vl53lx.Vl53lxRange;
import ai.depz.sensor.sensors.vl53lx.Vl53lxTarget;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Histogram post-processing: 24 bins → targets, on the host alone, as {@code VL53LX_hist_process_data()}
 * does in C. Fold repeated bin codes, estimate the ambient floor, drop the ambient-only bins, threshold
 * each bin, take the runs of marked bins as pulses; each pulse is isolated, filtered with a three-tap
 * window and its zero crossing interpolated to a sub-bin phase, which converts to millimetres. ST's
 * source names its functions {@code VL53LX_f_NNN}; the names here are ours, the C name is in the
 * comment. Crosstalk compensation and the ambient dmax are not ported (dmax needs FMT data our boards
 * ship blank, so it would be zero anyway). Arithmetic is the C driver's: truncating division, and
 * saturation where it caps an intermediate.
 */
public final class Hist {
    private Hist() {}

    public static final int MAX_BIN_SEQUENCE_LENGTH = 6;
    public static final int MAX_BIN_SEQUENCE_CODE = 15;
    public static final int MAX_PULSES = 8;                  // VL53LX_D_001
    public static final int MAX_RANGE_RESULTS = 4;

    public static final long MAX_ALLOWED_PHASE = 0xFFFF;
    public static final long SPAD_TOTAL_COUNT_MAX = (1L << 29) - 1;
    public static final long SPAD_TOTAL_COUNT_RES_THRES = 1L << 24;
    public static final long SPEED_OF_LIGHT_IN_AIR_DIV8 = 299704 >> 3;

    // vl53lx_sigma_estimate.h — the saturation ceilings of the sigma estimate.
    public static final long SIGMA_INVALID = 0xFFFF;         // VL53LX_D_002
    private static final long D003 = 0xFFFFFFL;
    private static final long D004 = 0xFFFFFFFFFFFFFFL;
    private static final long D005 = 0x7FFFFFFFFFL;
    private static final long D006 = 0x7FFFFFFFFFFFFFFFL;
    private static final long D007 = 0xFFFFFFFFL;

    // VL53LX_DeviceError, the ones the histogram path sets.
    public static final long DEVICE_ERROR_NO_UPDATE = 0;
    public static final long DEVICE_ERROR_RANGE_PHASE_CHECK = 5;
    public static final long DEVICE_ERROR_SIGMA_THRESHOLD_CHECK = 6;
    public static final long DEVICE_ERROR_PHASE_CONSISTENCY = 7;
    public static final long DEVICE_ERROR_RANGE_COMPLETE = 9;
    public static final long DEVICE_ERROR_RANGE_COMPLETE_NO_WRAP_CHECK = 19;
    public static final long DEVICE_ERROR_EVENT_CONSISTENCY = 20;
    public static final long DEVICE_ERROR_RANGE_COMPLETE_MERGED_PULSE = 22;
    public static final long DEVICE_ERROR_PREV_RANGE_NO_TARGETS = 23;

    public static final long HIST_TARGET_ORDER_STRONGEST_FIRST = 1;
    public static final long HIST_TARGET_ORDER_CLOSEST_FIRST = 2;

    // VL53LX_RangeStatus — the user-facing status, the same numbers the rest of the family reports.
    public static final int RANGE_STATUS_RANGE_VALID = 0;
    public static final int RANGE_STATUS_SIGMA_FAIL = 1;
    public static final int RANGE_STATUS_OUT_OF_BOUNDS_FAIL = 4;
    public static final int RANGE_STATUS_RANGE_VALID_NO_WRAP_CHECK = 6;
    public static final int RANGE_STATUS_WRAP_TARGET_FAIL = 7;
    public static final int RANGE_STATUS_RANGE_VALID_MERGED_PULSE = 11;
    public static final int RANGE_STATUS_TARGET_PRESENT_LACK_OF_SIGNAL = 12;
    public static final int RANGE_STATUS_NONE = 255;

    /** UM2931 range status names. */
    public static final Map<Integer, String> STATUS_NAMES = Map.of(
            RANGE_STATUS_RANGE_VALID, "valid",
            RANGE_STATUS_SIGMA_FAIL, "sigma above threshold",
            RANGE_STATUS_OUT_OF_BOUNDS_FAIL, "phase out of valid limit",
            RANGE_STATUS_RANGE_VALID_NO_WRAP_CHECK, "no wrap-around check done",
            RANGE_STATUS_WRAP_TARGET_FAIL, "wrapped target",
            RANGE_STATUS_RANGE_VALID_MERGED_PULSE, "valid, merged pulse",
            RANGE_STATUS_TARGET_PRESENT_LACK_OF_SIGNAL, "no target in the frame before",
            RANGE_STATUS_NONE, "no target");

    // ── working state ────────────────────────────────────────────────────────

    /** {@code VL53LX_hist_pulse_data_t}: one run of bins above the threshold. */
    private static final class PulseData {
        long startBin;            // VL53LX_p_012
        long firstBin;            // VL53LX_p_019
        long peakBin = 0xFF;      // VL53LX_p_023
        long lastBin;             // VL53LX_p_024
        long endBin;              // VL53LX_p_013
        long widthBins;           // VL53LX_p_025
        long filterWoi;           // VL53LX_p_051, filter half width
        long ambientEvents;       // VL53LX_p_016
        long totalEvents;         // VL53LX_p_017
        long signalEvents;        // VL53LX_p_010
        long phaseStart;          // VL53LX_p_026
        long phaseMean;           // VL53LX_p_011
        long phaseEnd;            // VL53LX_p_027
        long sigma;               // VL53LX_p_002
    }

    /**
     * {@code VL53LX_hist_gen3_algo_private_data_t}: per-bin thresholds and flags, the pulse list, and
     * the working copies of the histogram.
     */
    private static final class Gen3Algo {
        private static final int N = BareRegs.HISTOGRAM_BUFFER_SIZE;

        long binsInData;          // VL53LX_p_021
        long vcselPeriod;         // VL53LX_p_030, bins per period
        long ambientPerBin;       // VL53LX_p_028
        final long[] overThreshold = new long[N];   // VL53LX_p_040
        final long[] pulseMask = new long[N];       // VL53LX_p_041
        final long[] pulseNo = new long[N];         // VL53LX_p_042
        final long[] threshold = new long[N];       // VL53LX_p_052
        long firstRisingBin;      // VL53LX_p_044
        long maxPulses = MAX_PULSES;                // VL53LX_p_045
        long pulseCount;          // VL53LX_p_046
        final PulseData[] pulses = new PulseData[MAX_PULSES];
        HistogramBinData xtalk;       // VL53LX_p_047
        HistogramBinData pulseAmb;    // VL53LX_p_048, pulse padded ambient
        HistogramBinData pulseZero;   // VL53LX_p_049, pulse padded zero
        HistogramBinData pulseXtalk;  // VL53LX_p_050

        Gen3Algo() {
            for (int i = 0; i < MAX_PULSES; i++) {
                pulses[i] = new PulseData();
            }
        }
    }

    /**
     * {@code VL53LX_hist_gen4_algo_filtered_data_t}: the three-tap window sums and the two edge
     * signals whose sign change locates the peak.
     */
    private static final class Filtered {
        private static final int N = BareRegs.HISTOGRAM_BUFFER_SIZE;
        final long[] a = new long[N];        // VL53LX_p_007
        final long[] b = new long[N];        // VL53LX_p_032
        final long[] c = new long[N];        // VL53LX_p_001
        final long[] left = new long[N];     // VL53LX_p_053
        final long[] right = new long[N];    // VL53LX_p_054
        final long[] isPeak = new long[N];   // VL53LX_p_040
    }

    /** One target of the frame before, reduced to what the consistency checks read. */
    record PrevTarget(long ambientEvents, long totalEvents, long phaseMean, long rangeStatus) {}

    // ── arithmetic ───────────────────────────────────────────────────────────

    /** The modulo of the reference (sign of the divisor). */
    public static long pyMod(long a, long b) {
        long r = a % b;
        return r != 0 && (r < 0) != (b < 0) ? r + b : r;
    }

    // The floor division of the reference (C truncation is plain '/').
    private static long floorDiv(long a, long b) {
        return Math.floorDiv(a, b);
    }

    /** VL53LX_isqrt(): floor of the square root of a uint32 argument. */
    public static long isqrt(long num) {
        long n = num & 0xFFFFFFFFL;
        long r = (long) Math.sqrt((double) n);
        while (r * r > n) {
            r--;
        }
        while ((r + 1) * (r + 1) <= n) {
            r++;
        }
        return r;
    }

    // a * b for non-negative operands, held at cap when the true product would pass it.
    private static long satMul(long a, long b, long cap) {
        return b != 0 && a > cap / b ? cap : Math.min(a * b, cap);
    }

    /** VL53LX_calc_pll_period_mm(). */
    public static long calcPllPeriodMm(long fastOscFrequency) {
        long pllPeriodUs = BareMath.calcPllPeriodUs(fastOscFrequency);
        long pllPeriodMm = SPEED_OF_LIGHT_IN_AIR_DIV8 * (pllPeriodUs >> 2);
        return (pllPeriodMm + (1 << 15)) >> 16;
    }

    /** VL53LX_rate_maths(): events over a duration → count rate in 9.7 Mcps. */
    public static long rateMaths(long events, long timeUs) {
        long tmp = 0;
        if (events > SPAD_TOTAL_COUNT_MAX) {
            tmp = SPAD_TOTAL_COUNT_MAX;
        } else if (events > 0) {
            tmp = events;
        }
        int fracBits = events > SPAD_TOTAL_COUNT_RES_THRES ? 3 : 7;
        if (timeUs > 0) {
            tmp = ((tmp << fracBits) + timeUs / 2) / timeUs;
        }
        if (events > SPAD_TOTAL_COUNT_RES_THRES) {
            tmp <<= 4;
        }
        return Math.min(tmp, 0xFFFF);
    }

    /** VL53LX_rate_per_spad_maths(). */
    public static long ratePerSpadMaths(int fracBits, long peakCountRate, long numSpads, long maxOutputValue) {
        long tmp;
        if (numSpads > 0) {
            tmp = (peakCountRate << 8) << fracBits;
            tmp = (tmp + numSpads / 2) / numSpads;
        } else {
            tmp = peakCountRate << fracBits;
        }
        return Math.min(tmp, maxOutputValue);
    }

    /** VL53LX_events_per_spad_maths() → kcps per SPAD. */
    public static long eventsPerSpadMaths(long events, long numSpads, long duration) {
        long total = numSpads != 0 ? floorDiv(events * 1000 * 256, numSpads) : 0;
        long perSpad = duration > 0 ? floorDiv((total << 11) + duration / 2, duration) : total << 11;
        return perSpad & 0xFFFFFFFFL;
    }

    /** VL53LX_range_maths(): phase → millimetres. */
    public static long rangeMaths(long fastOscFrequency, long phase, long zeroDistancePhase, int fractionalBits,
            long gainFactor, long rangeOffsetMm) {
        long pllPeriodUs = BareMath.calcPllPeriodUs(fastOscFrequency);
        long tmp = phase - zeroDistancePhase;
        tmp *= pllPeriodUs;
        tmp /= 1 << 9;
        tmp *= SPEED_OF_LIGHT_IN_AIR_DIV8;
        tmp /= 1 << 22;

        long rangeMm = tmp + rangeOffsetMm;
        rangeMm *= gainFactor;
        rangeMm += 0x0400;
        rangeMm /= 0x0800;

        if (fractionalBits == 0) {
            long rangeMm10 = rangeMm * 10 / (1 << 2);
            rangeMm = Math.abs(rangeMm10 - rangeMm10 / 10 * 10) < 5 ? rangeMm10 / 10 : rangeMm10 / 10 + 1;
        } else if (fractionalBits == 1) {
            rangeMm /= 1 << 1;
        }
        return rangeMm;
    }

    // ── bin housekeeping ─────────────────────────────────────────────────────

    /**
     * VL53LX_f_031: fold the six bin-sequence codes together. Repeats of a code land on the same four
     * output bins and are averaged, so what comes out is one bin per distinct code.
     */
    public static HistogramBinData averageRepeatedBins(HistogramBinData src) {
        HistogramBinData dst = src.clone();
        dst.binsInData = 0;
        dst.binSeq = new int[MAX_BIN_SEQUENCE_LENGTH];
        Arrays.fill(dst.binSeq, MAX_BIN_SEQUENCE_CODE + 1);
        dst.binData = new long[(int) dst.numberOfBins];

        int[] initialIndex = new int[MAX_BIN_SEQUENCE_CODE + 2];
        long[] repeatCount = new long[MAX_BIN_SEQUENCE_CODE + 2];
        int seqLength = 0;
        for (int lc = 0; lc < MAX_BIN_SEQUENCE_LENGTH; lc++) {
            int binCfg = src.binSeq[lc];
            if (repeatCount[binCfg] == 0) {
                initialIndex[binCfg] = seqLength * 4;
                dst.binSeq[seqLength] = binCfg;
                seqLength++;
            }
            repeatCount[binCfg]++;
            int b = initialIndex[binCfg];
            for (int i = 0; i < 4; i++) {
                dst.binData[b + i] += src.binData[lc * 4 + i];
            }
        }

        dst.binRep = new long[MAX_BIN_SEQUENCE_LENGTH];
        for (int i = 0; i < MAX_BIN_SEQUENCE_LENGTH; i++) {
            int c = dst.binSeq[i];
            dst.binRep[i] = c <= MAX_BIN_SEQUENCE_CODE ? repeatCount[c] : 0;
        }
        dst.binsInData = seqLength * 4L;

        for (int code = 0; code <= MAX_BIN_SEQUENCE_CODE; code++) {
            long reps = repeatCount[code];
            if (reps <= 0) {
                continue;
            }
            int b = initialIndex[code];
            for (int i = 0; i < 4; i++) {
                dst.binData[b + i] = (dst.binData[b + i] + reps / 2) / reps;
            }
        }

        // Codes 7 and 15 are the ambient-only sequence entries.
        dst.numberOfAmbientBins = repeatCount[7] != 0 || repeatCount[15] != 0 ? 4 : 0;
        return dst;
    }

    /** VL53LX_hist_calc_zero_distance_phase. */
    public static void calcZeroDistancePhase(HistogramBinData bins) {
        long period = 2048 * BareMath.decodeVcselPeriod(bins.vcselPeriod);
        long phase = period + bins.phasecalResultReferencePhase + 2048 * bins.phasecalResultVcselStart
                - 2048 * bins.calConfigVcselStart;
        bins.zeroDistancePhase = period != 0 ? pyMod(phase, period) : 0;
    }

    /** VL53LX_hist_estimate_ambient_from_thresholded_bins(): the floor from every bin near the quietest. */
    public static void estimateAmbientFromThresholdedBins(long ambientThresholdSigma, HistogramBinData bins) {
        long[] used = Arrays.copyOf(bins.binData, (int) Math.min(Math.max(bins.binsInData, 0), bins.binData.length));
        bins.minBinValue = used.length > 0 ? Arrays.stream(used).min().getAsLong() : 0;
        bins.maxBinValue = used.length > 0 ? Arrays.stream(used).max().getAsLong() : 0;

        long threshold = isqrt(bins.minBinValue);
        threshold *= ambientThresholdSigma;
        threshold += 0x07;
        threshold >>= 4;
        threshold += bins.minBinValue;

        bins.numberOfAmbientSamples = 0;
        bins.ambientEventsSum = 0;
        for (long value : used) {
            if (value < threshold) {
                bins.ambientEventsSum += value;
                bins.numberOfAmbientSamples++;
            }
        }
        if (bins.numberOfAmbientSamples > 0) {
            bins.ambientPerBin = (bins.ambientEventsSum + bins.numberOfAmbientSamples / 2) / bins.numberOfAmbientSamples;
        }
    }

    /** VL53LX_hist_estimate_ambient_from_ambient_bins(): the better estimate, from the VCSEL-off bins. */
    public static void estimateAmbientFromAmbientBins(HistogramBinData bins) {
        if (bins.numberOfAmbientBins <= 0) {
            return;
        }
        bins.numberOfAmbientSamples = bins.numberOfAmbientBins;
        long sum = 0;
        int n = (int) Math.min(bins.numberOfAmbientBins, bins.binData.length);
        for (int i = 0; i < n; i++) {
            sum += bins.binData[i];
        }
        bins.ambientEventsSum = sum;
        bins.ambientPerBin = (bins.ambientEventsSum + bins.numberOfAmbientBins / 2) / bins.numberOfAmbientBins;
    }

    /** VL53LX_hist_remove_ambient_bins: drop the ambient-only bins off the front, so bin 0 is phase zero. */
    public static void removeAmbientBins(HistogramBinData bins) {
        if ((bins.binSeq[0] & 0x07) == 0x07) {
            int[] keptSeq = new int[MAX_BIN_SEQUENCE_LENGTH];
            long[] keptRep = new long[MAX_BIN_SEQUENCE_LENGTH];
            Arrays.fill(keptSeq, MAX_BIN_SEQUENCE_CODE + 1);
            int k = 0;
            for (int lc = 0; lc < MAX_BIN_SEQUENCE_LENGTH; lc++) {
                if ((bins.binSeq[lc] & 0x07) != 0x07) {
                    keptSeq[k] = bins.binSeq[lc];
                    keptRep[k] = bins.binRep[lc];
                    k++;
                }
            }
            bins.binSeq = keptSeq;
            bins.binRep = keptRep;
        }
        int n = (int) bins.numberOfAmbientBins;
        if (n > 0) {
            long[] shifted = new long[bins.binData.length];
            System.arraycopy(bins.binData, n, shifted, 0, Math.max(0, bins.binData.length - n));
            bins.binData = shifted;
            bins.binsInData -= n;
            bins.numberOfAmbientBins = 0;
        }
    }

    // VL53LX_f_022: the three-tap window around binIndex over a half width of filterWoi bins → {a, b, c}.
    private static long[] woiSums(long binIndex, long filterWoi, HistogramBinData bins) {
        long a = 0;
        long b = bins.binData[(int) binIndex];
        long c = 0;
        for (long w = 0; w < (filterWoi << 1) + 1; w++) {
            long j = pyMod(binIndex + w + bins.binsInData - filterWoi, bins.binsInData);
            if (w < filterWoi) {
                a += bins.binData[(int) j];
            } else if (w > filterWoi) {
                c += bins.binData[(int) j];
            }
        }
        return new long[] {a, b, c};
    }

    // VL53LX_f_018: event sums → count rates, over the durations the pulse actually ran.
    private static boolean calcRates(RangeData target, long vcselWidth, long fastOscFrequency, long totalPeriodsElapsed,
            long spads, long mergeNb) {
        target.vcselWidth = vcselWidth;
        target.fastOscFrequency = fastOscFrequency;
        target.totalPeriodsElapsed = totalPeriodsElapsed;
        target.spads = spads;
        if (fastOscFrequency == 0 || totalPeriodsElapsed == 0) {
            return false;
        }

        long pllPeriodUs = BareMath.calcPllPeriodUs(fastOscFrequency);
        long periodsElapsed = totalPeriodsElapsed + 1;
        target.peakDurationUs = BareMath.durationMaths(pllPeriodUs, vcselWidth, BareRegs.RANGING_WINDOW_VCSEL_PERIODS,
                periodsElapsed);
        target.woiDurationUs = BareMath.durationMaths(pllPeriodUs, target.windowBins << 4,
                BareRegs.RANGING_WINDOW_VCSEL_PERIODS, periodsElapsed);

        target.peakSignalCountRateMcps = rateMaths(target.signalEvents, target.peakDurationUs);
        target.avgSignalCountRateMcps = rateMaths(target.signalEvents, target.woiDurationUs);
        target.ambientCountRateMcps = rateMaths(target.ambientEvents, target.woiDurationUs);

        long countRateTotal = target.peakSignalCountRateMcps + target.ambientCountRateMcps;
        if (mergeNb > 1) {
            countRateTotal = floorDiv(countRateTotal, mergeNb);
        }
        target.totalRatePerSpadMcps = ratePerSpadMaths(0x06, countRateTotal, spads, 0xFFFF);
        target.signalEventsPerSpadKcps = eventsPerSpadMaths(target.signalEvents, spads, target.peakDurationUs);
        return true;
    }

    // VL53LX_f_019
    private static void calcRanges(long gainFactor, long rangeOffsetMm, RangeData target) {
        target.minRangeMm = rangeMaths(target.fastOscFrequency, target.phaseStart, target.zeroDistancePhase, 0,
                gainFactor, rangeOffsetMm);
        target.medianRangeMm = rangeMaths(target.fastOscFrequency, target.phaseMean, target.zeroDistancePhase, 0,
                gainFactor, rangeOffsetMm);
        target.maxRangeMm = rangeMaths(target.fastOscFrequency, target.phaseEnd, target.zeroDistancePhase, 0,
                gainFactor, rangeOffsetMm);
    }

    // ── sigma (vl53lx_sigma_estimate.c) ──────────────────────────────────────

    // VL53LX_f_023: the width of the pulse, 9.7 fixed point mm — how far the window sums lean to one side, over
    // how much signal there is above ambient. The ceilings are ST's.
    private static long sigmaEstimate(long sigmaRefMm, long a, long b, long c, long aZp, long cZp, long bx, long axZp,
            long cxZp, long ambientPerBin, long fastOscFrequency) {
        if (fastOscFrequency == 0) {
            return SIGMA_INVALID;
        }
        long pllPeriodMm = calcPllPeriodMm(fastOscFrequency);
        long bMinusAmb = Math.abs(ambientPerBin - b);
        long aMinusC = Math.abs(a - c);
        if (bMinusAmb == 0) {
            return SIGMA_INVALID;
        }

        long tmp0 = Math.min(b + bx + ambientPerBin, D003);

        // (a - c)^2 << 8, capped at D004 — which it passes exactly when |a - c| >= 2^24.
        long tmp1 = aMinusC >= 1L << 24 ? D004 : Math.min((aMinusC * aMinusC) << 8, D004);
        tmp1 /= bMinusAmb;
        tmp1 /= bMinusAmb;
        tmp1 = Math.min(tmp1, D005);

        tmp0 = tmp1 * tmp0; // < 2^39 * 2^24: no overflow

        tmp1 = Math.min(cZp + cxZp + aZp + axZp, D003) << 8;

        tmp0 = Math.min(tmp1 + tmp0, D006);

        tmp0 = tmp0 > D007 ? satMul(tmp0 / bMinusAmb, pllPeriodMm, D006) : tmp0 * pllPeriodMm / bMinusAmb;
        tmp0 = Math.min(tmp0, D006);

        tmp0 = tmp0 > D007 ? satMul(tmp0 / bMinusAmb / 4, pllPeriodMm, D006) : tmp0 * pllPeriodMm / bMinusAmb / 4;
        tmp0 = Math.min(tmp0, D006);

        tmp0 = Math.min(tmp0 >> 2, D007);

        tmp1 = sigmaRefMm << 7;
        tmp0 = Math.min(tmp0 + tmp1 * tmp1, D007);
        return isqrt(tmp0);
    }

    // VL53LX_f_014: the sigma of one pulse, from its two padded copies (and its crosstalk copy).
    private static long pulseSigma(PulseData pulse, long sigmaRefMm, Gen3Algo algo, boolean xtalkEnable) {
        if (algo.vcselPeriod == 0) {
            return SIGMA_INVALID;
        }
        long i = pulse.peakBin % algo.vcselPeriod;
        long[] zp = woiSums(i, pulse.filterWoi, algo.pulseZero);
        long[] amb = woiSums(i, pulse.filterWoi, algo.pulseAmb);
        long[] x = xtalkEnable ? woiSums(i, pulse.filterWoi, algo.pulseXtalk) : new long[3];
        return sigmaEstimate(sigmaRefMm, amb[0], amb[1], amb[2], zp[0], zp[2], x[1], x[0], x[2],
                algo.pulseAmb.ambientPerBin, algo.pulseAmb.fastOscFrequency);
    }

    // ── pulse detection (vl53lx_hist_algos_gen3.c) ───────────────────────────

    // VL53LX_f_006: a detection threshold per bin — the ambient floor plus a sigma margin that shrinks with the
    // number of samples in the bin — and the flags saying which bins clear it.
    private static void ambientThresholds(long scaler, long thresholdSigma, long minThresholdEvents, boolean xtalkEnable,
            HistogramBinData bins, Gen3Algo algo) {
        algo.binsInData = bins.binsInData;
        algo.ambientPerBin = bins.ambientPerBin;
        algo.vcselPeriod = BareMath.decodeVcselPeriod(bins.vcselPeriod);

        long ambEvents = (bins.ambientPerBin * scaler + 2048) / 4096;

        for (long lb = 0; lb < bins.binsInData; lb++) {
            long samples = bins.binRep[(int) (lb >> 2)];
            if (samples <= 0) {
                continue;
            }
            long value = lb < algo.xtalk.binsInData && xtalkEnable
                    ? samples * (ambEvents + algo.xtalk.binData[(int) lb])
                    : samples * ambEvents;
            value = isqrt(value);
            value += samples / 2;
            value = floorDiv(value, samples);
            value *= thresholdSigma;
            value += 8;
            value = floorDiv(value, 16);
            value += ambEvents;
            value = Math.max(value, minThresholdEvents);
            algo.threshold[(int) lb] = value;
        }

        for (long lb = bins.firstBin; lb < bins.binsInData; lb++) {
            long over = bins.binData[(int) lb] > algo.threshold[(int) lb] ? 1 : 0;
            algo.overThreshold[(int) lb] = over;
            algo.pulseMask[(int) lb] = over;
        }
    }

    // VL53LX_f_007: the first quiet-to-loud step, so a pulse wrapped across the end of the period stays whole.
    private static void findFirstRisingEdge(Gen3Algo algo) {
        algo.firstRisingBin = 0;
        boolean found = false;
        for (long i = 0; i < algo.vcselPeriod; i++) {
            long j = (i + 1) % algo.vcselPeriod;
            if (i < algo.binsInData && j < algo.binsInData && algo.pulseMask[(int) i] == 0
                    && algo.pulseMask[(int) j] == 1 && !found) {
                algo.firstRisingBin = i;
                found = true;
            }
        }
    }

    // VL53LX_f_008: number the runs of over-threshold bins, from the first rising edge.
    private static void assignPulseNumbers(Gen3Algo algo) {
        for (long lb = algo.firstRisingBin; lb < algo.firstRisingBin + algo.vcselPeriod; lb++) {
            long i = lb % algo.vcselPeriod;
            long j = (lb + 1) % algo.vcselPeriod;
            if (!(i < algo.binsInData && j < algo.binsInData)) {
                continue;
            }
            if (algo.pulseMask[(int) i] == 0 && algo.pulseMask[(int) j] == 1) {
                algo.pulseCount++;
            }
            algo.pulseCount = Math.min(algo.pulseCount, algo.maxPulses);
            algo.pulseNo[(int) i] = algo.pulseMask[(int) i] > 0 ? algo.pulseCount : 0;
        }
    }

    // VL53LX_f_009: each numbered run → its window, width and filter half width.
    private static void pulseExtents(Gen3Algo algo) {
        long maxFilterHalfWidth = (algo.vcselPeriod - 1) >> 1;
        for (long blb = algo.firstRisingBin; blb < algo.firstRisingBin + algo.vcselPeriod; blb++) {
            long i = blb % algo.vcselPeriod;
            long j = (blb + 1) % algo.vcselPeriod;
            if (!(i < algo.binsInData && j < algo.binsInData)) {
                continue;
            }
            if (algo.pulseNo[(int) i] == 0 && algo.pulseNo[(int) j] > 0) {
                long no = algo.pulseNo[(int) j] - 1;
                if (no < algo.maxPulses) {
                    PulseData pulse = algo.pulses[(int) no];
                    pulse.startBin = blb;
                    pulse.firstBin = blb + 1;
                    pulse.peakBin = 0xFF;
                    pulse.lastBin = 0;
                    pulse.endBin = 0;
                }
            }
            if (algo.pulseNo[(int) i] > 0 && algo.pulseNo[(int) j] == 0) {
                long no = algo.pulseNo[(int) i] - 1;
                if (no < algo.maxPulses) {
                    PulseData pulse = algo.pulses[(int) no];
                    pulse.lastBin = blb;
                    pulse.endBin = blb + 1;
                    pulse.widthBins = pulse.lastBin + 1 - pulse.firstBin;
                    pulse.filterWoi = Math.min(pulse.endBin + 1 - pulse.startBin, maxFilterHalfWidth);
                }
            }
        }
    }

    // VL53LX_f_010: total and ambient events inside the pulse window.
    private static void pulseEventSums(int pulseNo, HistogramBinData bins, Gen3Algo algo) {
        PulseData pulse = algo.pulses[pulseNo];
        pulse.totalEvents = 0;
        pulse.ambientEvents = 0;
        for (long lb = pulse.startBin; lb <= pulse.endBin; lb++) {
            long i = lb % algo.vcselPeriod;
            pulse.totalEvents += bins.binData[(int) i];
            pulse.ambientEvents += algo.ambientPerBin;
        }
        pulse.signalEvents = pulse.totalEvents - pulse.ambientEvents;
    }

    // VL53LX_f_011: a copy of the frame with everything outside the pulse replaced by padValue.
    private static HistogramBinData isolatePulse(int pulseNo, HistogramBinData bins, Gen3Algo algo, long padValue) {
        PulseData pulse = algo.pulses[pulseNo];
        HistogramBinData output = bins.clone();
        for (long lb = algo.firstRisingBin; lb < algo.firstRisingBin + algo.vcselPeriod; lb++) {
            if (lb < pulse.startBin || lb > pulse.endBin) {
                long i = lb % algo.vcselPeriod;
                if (i < output.binsInData) {
                    output.binData[(int) i] = padValue;
                }
            }
        }
        return output;
    }

    // VL53LX_f_020: the centre of mass of the bins in [start, end], in 1/2048ths of a bin.
    private static long weightedPhase(long start, long end, long vcselPeriod, boolean clipEvents, HistogramBinData bins) {
        long eventSum = 0;
        long weightedSum = 0;
        if (vcselPeriod == 0) {
            return MAX_ALLOWED_PHASE;
        }
        for (long lb = start; lb <= end; lb++) {
            long i = lb < 0 ? lb + vcselPeriod : lb % vcselPeriod;
            if (i >= 0 && i < BareRegs.HISTOGRAM_BUFFER_SIZE) {
                long value = bins.binData[(int) i] - bins.ambientPerBin;
                if (clipEvents && value < 0) {
                    value = 0;
                }
                eventSum += value;
                weightedSum += value * (1024 + 2048 * lb);
            }
        }
        if (eventSum <= 0) {
            return MAX_ALLOWED_PHASE;
        }
        weightedSum += eventSum / 2;
        weightedSum /= eventSum;
        return Math.max(weightedSum, 0);
    }

    // VL53LX_f_015: the near and far phase of the pulse — the centres of mass of its leading and trailing bins.
    private static void pulsePhaseLimits(int pulseNo, boolean clipEvents, HistogramBinData bins, Gen3Algo algo) {
        PulseData pulse = algo.pulses[pulseNo];
        if (pulse.peakBin == 0xFF) {
            pulse.peakBin = 1;
        }
        long i = pulse.peakBin % algo.vcselPeriod;
        long start = i + pulse.startBin - pulse.peakBin;
        long end = i + pulse.endBin - pulse.peakBin;
        long windowWidth = Math.min(end - start, 3);

        pulse.phaseStart = weightedPhase(start, start + windowWidth, algo.vcselPeriod, clipEvents, bins);
        pulse.phaseEnd = weightedPhase(end - windowWidth, end, algo.vcselPeriod, clipEvents, bins);
        if (pulse.phaseStart > pulse.phaseEnd) {
            long t = pulse.phaseStart;
            pulse.phaseStart = pulse.phaseEnd;
            pulse.phaseEnd = t;
        }
        pulse.phaseStart = Math.min(pulse.phaseStart, pulse.phaseMean);
        pulse.phaseEnd = Math.max(pulse.phaseEnd, pulse.phaseMean);
    }

    // VL53LX_f_016: strongest first or closest first. Stable, as the reference's sort is (List.sort is a
    // stable merge sort; a reversed comparator keeps ties in their original order).
    private static void sortPulses(long targetOrder, Gen3Algo algo) {
        if (algo.pulseCount <= 1) {
            return;
        }
        int n = (int) algo.pulseCount;
        List<PulseData> head = new ArrayList<>(Arrays.asList(algo.pulses).subList(0, n));
        head.sort(targetOrder == HIST_TARGET_ORDER_STRONGEST_FIRST
                ? Comparator.comparingLong((PulseData p) -> p.signalEvents).reversed()
                : Comparator.comparingLong((PulseData p) -> p.phaseMean));
        for (int k = 0; k < n; k++) {
            algo.pulses[k] = head.get(k);
        }
    }

    // VL53LX_f_017: pulse → target, plus the checks that can fail it: sigma over the threshold, and a phase
    // outside the window the preset mode is valid for.
    private static void fillTarget(long rangeId, long validPhaseLow, long validPhaseHigh, long sigmaThresh,
            HistogramBinData bins, PulseData pulse, RangeData target) {
        target.rangeId = rangeId;
        target.startBin = pulse.startBin;
        target.firstBin = pulse.firstBin;
        target.peakBin = pulse.peakBin;
        target.lastBin = pulse.lastBin;
        target.endBin = pulse.endBin;
        target.widthBins = pulse.widthBins;
        target.windowBins = pulse.endBin + 1 - pulse.startBin;

        target.zeroDistancePhase = bins.zeroDistancePhase;
        target.sigma = pulse.sigma;
        target.phaseStart = pulse.phaseStart & 0xFFFF;
        target.phaseMean = pulse.phaseMean & 0xFFFF;
        target.phaseEnd = pulse.phaseEnd & 0xFFFF;
        target.totalEvents = pulse.totalEvents;
        target.signalEvents = pulse.signalEvents;
        target.ambientEvents = pulse.ambientEvents;
        target.totalPeriodsElapsed = bins.totalPeriodsElapsed;

        target.rangeStatus = DEVICE_ERROR_RANGE_COMPLETE_NO_WRAP_CHECK;
        if (sigmaThresh > 0 && pulse.sigma > sigmaThresh << 5) {
            target.rangeStatus = DEVICE_ERROR_SIGMA_THRESHOLD_CHECK;
        }

        long lower = (validPhaseLow << 8) & 0xFFFF;
        lower = lower < target.zeroDistancePhase ? target.zeroDistancePhase - lower : 0;
        long upper = ((validPhaseHigh << 8) + bins.zeroDistancePhase) & 0xFFFF;
        if (target.phaseMean < lower || target.phaseMean > upper) {
            target.rangeStatus = DEVICE_ERROR_RANGE_PHASE_CHECK;
        }
    }

    // ── peak location (vl53lx_hist_algos_gen4.c) ─────────────────────────────

    // VL53LX_f_026: the three-tap window over the pulse; where left and right are both positive the centre of
    // mass is inside this bin.
    private static void filterPulse(int pulseNo, HistogramBinData pulseBins, Gen3Algo algo, Filtered filtered) {
        PulseData pulse = algo.pulses[pulseNo];
        for (long lb = pulse.startBin; lb <= pulse.endBin; lb++) {
            int i = (int) (lb % algo.vcselPeriod);
            long[] abc = woiSums(i, pulse.filterWoi, pulseBins);
            filtered.a[i] = abc[0];
            filtered.b[i] = abc[1];
            filtered.c[i] = abc[2];
            filtered.left[i] = abc[0] + abc[1] - (abc[2] + algo.ambientPerBin);
            filtered.right[i] = abc[1] + abc[2] - (abc[0] + algo.ambientPerBin);
        }
    }

    // VL53LX_f_028: the sub-bin phase where (c - a) against (b - ambient) crosses zero; null when the pulse has
    // no height (the division by zero C turns into "not a peak after all").
    private static Long interpolatePhase(long binIndex, long a, long b, long c, long ambientPerBin, long vcselPeriod) {
        long numerator = 4096 * (c - a);
        long halfBMinusAmb = 4096 * (b - ambientPerBin);
        if (halfBMinusAmb == 0) {
            return null;
        }
        long meanPhase = 4096 * numerator + halfBMinusAmb;
        meanPhase /= halfBMinusAmb * 2;
        meanPhase += 2048;
        meanPhase += 4096 * binIndex;
        meanPhase = (meanPhase + 1) / 2;
        meanPhase = Math.min(Math.max(meanPhase, 0), MAX_ALLOWED_PHASE);
        return pyMod(meanPhase, vcselPeriod * 2048);
    }

    // VL53LX_f_027: keep the last bin where the filter puts the centre of mass; that bin and its phase are the target.
    private static void findPeakBin(int pulseNo, Filtered filtered, Gen3Algo algo) {
        PulseData pulse = algo.pulses[pulseNo];
        for (long lb = pulse.startBin; lb < pulse.endBin; lb++) {
            long i = lb % algo.vcselPeriod;
            long j = (lb + 1) % algo.vcselPeriod;
            if (!(i < algo.binsInData && j < algo.binsInData)) {
                continue;
            }
            int ii = (int) i;
            int jj = (int) j;
            long isPeak;
            if (filtered.left[ii] == 0 && filtered.right[ii] == 0) {
                isPeak = 0;
            } else if (filtered.left[ii] >= 0 && filtered.right[ii] >= 0) {
                isPeak = 1;
            } else if (filtered.left[ii] < 0 && filtered.right[ii] >= 0 && filtered.left[jj] >= 0
                    && filtered.right[jj] < 0) {
                isPeak = 1;
            } else {
                isPeak = 0;
            }
            filtered.isPeak[ii] = isPeak;
            if (isPeak == 0) {
                continue;
            }
            pulse.peakBin = lb;
            Long phase = interpolatePhase(lb, filtered.a[ii], filtered.b[ii], filtered.c[ii], algo.ambientPerBin,
                    algo.vcselPeriod);
            if (phase != null) {
                pulse.phaseMean = phase;
            } else {
                filtered.isPeak[ii] = 0;
            }
        }
    }

    // ── entry point ──────────────────────────────────────────────────────────

    /** {@code VL53LX_hist_process_data()} + {@code VL53LX_f_025}, crosstalk off, one merged frame. */
    public static RangeResults processData(HistogramBinData binsInput, HistPostProcessConfig hpp) {
        return processData(binsInput, hpp, 1);
    }

    /** {@code VL53LX_hist_process_data()} + {@code VL53LX_f_025}, crosstalk off. */
    public static RangeResults processData(HistogramBinData binsInput, HistPostProcessConfig hpp, long mergeNb) {
        if (hpp.crosstalkCompensationEnable != 0) {
            throw new UnsupportedOperationException("crosstalk compensation is not ported");
        }

        Gen3Algo algo = new Gen3Algo();
        Filtered filtered = new Filtered();
        RangeResults results = new RangeResults();

        HistogramBinData bins = averageRepeatedBins(binsInput);
        // The crosstalk histogram: zero everywhere while compensation is off, full width so window sums stay defined.
        algo.xtalk = bins.clone();
        algo.xtalk.binData = new long[(int) bins.numberOfBins];

        results.streamCount = binsInput.resultStreamCount;

        calcZeroDistancePhase(bins);
        estimateAmbientFromThresholdedBins(hpp.ambientThreshSigma0, bins);
        estimateAmbientFromAmbientBins(bins);
        removeAmbientBins(bins);

        results.ambientDmaxMm = new long[5]; // VL53LX_f_001: zero without the FMT dmax calibration

        ambientThresholds(hpp.ambientThreshEventsScaler, hpp.ambientThreshSigma1, hpp.minAmbientThreshEvents, false,
                bins, algo);
        findFirstRisingEdge(algo);
        assignPulseNumbers(algo);
        pulseExtents(algo);

        for (int p = 0; p < algo.pulseCount; p++) {
            PulseData pulse = algo.pulses[p];
            pulseEventSums(p, bins, algo);
            algo.pulseAmb = isolatePulse(p, bins, algo, bins.ambientPerBin);
            algo.pulseZero = isolatePulse(p, bins, algo, 0);
            algo.pulseXtalk = isolatePulse(p, algo.xtalk, algo, 0);

            filterPulse(p, algo.pulseAmb, algo, filtered);
            findPeakBin(p, filtered, algo);
            pulse.sigma = pulseSigma(pulse, hpp.sigmaEstimatorSigmaRefMm, algo, false);
            pulsePhaseLimits(p, true, bins, algo);
        }

        sortPulses(hpp.histTargetOrder, algo);

        for (int p = 0; p < algo.pulseCount; p++) {
            if (results.targets.size() >= MAX_RANGE_RESULTS) {
                break;
            }
            PulseData pulse = algo.pulses[p];
            if (!(pulse.signalEvents > hpp.signalTotalEventsLimit && pulse.peakBin < 0xFF)) {
                continue;
            }
            RangeData target = new RangeData();
            fillTarget(results.targets.size(), hpp.validPhaseLow, hpp.validPhaseHigh, hpp.sigmaThresh, bins, pulse,
                    target);
            calcRates(target, bins.vcselWidth, bins.fastOscFrequency, bins.totalPeriodsElapsed,
                    bins.resultDssActualEffectiveSpads, mergeNb);
            calcRanges(hpp.gainFactor, hpp.rangeOffsetMm, target);
            results.targets.add(target);
        }
        return results;
    }

    // ── frame-to-frame consistency ───────────────────────────────────────────

    // VL53LX_hist_events_consistency_check(): scale the previous frame's events onto this exposure and ask
    // whether the difference fits inside the shot noise of the pair, eventSigma sixty-fourths wide.
    static long eventsConsistencyCheck(long eventSigma, long minEffectiveSpadCount, FrameHistory prevFrame,
            PrevTarget prev, RangeData target) {
        if (eventSigma == 0) {
            return DEVICE_ERROR_RANGE_COMPLETE;
        }

        long tmpp = (1 + prevFrame.totalPeriodsElapsed()) * prevFrame.spads();
        long tmpc = (1 + target.totalPeriodsElapsed) * target.spads;

        long eventsScaler = tmpp * 4096;
        if (tmpc != 0) {
            eventsScaler = (eventsScaler + floorDiv(tmpc, 2)) / tmpc;
        }
        long eventsScalerSq = (eventsScaler * eventsScaler + 2048) / 4096;

        long cSignalEvents = ((target.totalEvents - target.ambientEvents) * eventsScaler + 2048) / 4096;
        long cSigNoiseSq = (eventsScalerSq * target.totalEvents + 2048) / 4096;
        long cAmbNoiseSq = (eventsScalerSq * target.ambientEvents + 2048) / 4096;
        // Ambient is subtracted from both frames, so only its noise survives, at a quarter weight.
        cAmbNoiseSq = (cAmbNoiseSq + 2) / 4;
        long pAmbNoiseSq = (prev.ambientEvents() + 2) / 4;

        long noiseSqSum = (prev.totalEvents() + cSigNoiseSq + pAmbNoiseSq + cAmbNoiseSq) & 0xFFFFFFFFL;
        long tolerance = isqrt(noiseSqSum * 16);
        tolerance = (tolerance * eventSigma + 32) / 64;

        long pSignalEvents = prev.totalEvents() - prev.ambientEvents();
        long delta = Math.abs(cSignalEvents - pSignalEvents);
        return delta > tolerance && target.spads > minEffectiveSpadCount
                ? DEVICE_ERROR_EVENT_CONSISTENCY : DEVICE_ERROR_RANGE_COMPLETE;
    }

    // VL53LX_hist_merged_pulse_check(): a pulse far wider than one target should be is two targets in one.
    static long mergedPulseCheck(long minMaxToleranceMm, RangeData target) {
        long deltaMm = Math.abs(target.maxRangeMm - target.minRangeMm);
        return minMaxToleranceMm > 0 && deltaMm > minMaxToleranceMm
                ? DEVICE_ERROR_RANGE_COMPLETE_MERGED_PULSE : DEVICE_ERROR_RANGE_COMPLETE;
    }

    // ── the family-wide result shape ─────────────────────────────────────────

    /** ConvertStatusHisto() (vl53lx_api.c), the cases the histogram path produces here. */
    public static int convertStatus(long deviceError) {
        if (deviceError == DEVICE_ERROR_RANGE_PHASE_CHECK) {
            return RANGE_STATUS_OUT_OF_BOUNDS_FAIL;
        } else if (deviceError == DEVICE_ERROR_SIGMA_THRESHOLD_CHECK) {
            return RANGE_STATUS_SIGMA_FAIL;
        } else if (deviceError == DEVICE_ERROR_RANGE_COMPLETE_NO_WRAP_CHECK) {
            return RANGE_STATUS_RANGE_VALID_NO_WRAP_CHECK;
        } else if (deviceError == DEVICE_ERROR_PHASE_CONSISTENCY || deviceError == DEVICE_ERROR_EVENT_CONSISTENCY) {
            return RANGE_STATUS_WRAP_TARGET_FAIL;
        } else if (deviceError == DEVICE_ERROR_PREV_RANGE_NO_TARGETS) {
            return RANGE_STATUS_TARGET_PRESENT_LACK_OF_SIGNAL;
        } else if (deviceError == DEVICE_ERROR_RANGE_COMPLETE_MERGED_PULSE) {
            return RANGE_STATUS_RANGE_VALID_MERGED_PULSE;
        } else if (deviceError == DEVICE_ERROR_RANGE_COMPLETE) {
            return RANGE_STATUS_RANGE_VALID;
        }
        return RANGE_STATUS_NONE;
    }

    private static String statusText(int status) {
        String s = STATUS_NAMES.get(status);
        return s != null ? s : "unknown (" + status + ")";
    }

    private static Vl53lxTarget toTarget(RangeData d) {
        int status = convertStatus(d.rangeStatus);
        return new Vl53lxTarget((int) d.medianRangeMm, status, statusText(status), d.signalKcps(), d.ambientKcps(),
                d.sigmaMm(), (int) d.minRangeMm, (int) d.maxRangeMm);
    }

    /**
     * The frame's results as the family-wide range: every return in {@code targets}, the first one
     * also the range itself. Effective SPADs come in 1/256 and are reported whole, as the rest of the
     * family does.
     */
    public static Vl53lxRange toRange(RangeResults results, HistogramBinData bins) {
        if (results.targets.isEmpty()) {
            Map<String, Object> extra = new LinkedHashMap<>();
            extra.put("stream_count", results.streamCount);
            extra.put("ambient_per_bin", bins.ambientPerBin);
            return new Vl53lxRange(8191, RANGE_STATUS_NONE, STATUS_NAMES.get(RANGE_STATUS_NONE), 0, 0, 0,
                    bins.resultDssActualEffectiveSpads >> 8, List.of(), extra);
        }
        RangeData first = results.targets.get(0);
        Vl53lxTarget t0 = toTarget(first);
        List<Vl53lxTarget> targets = new ArrayList<>();
        for (RangeData d : results.targets) {
            targets.add(toTarget(d));
        }
        Map<String, Object> extra = new LinkedHashMap<>();
        extra.put("stream_count", results.streamCount);
        extra.put("min_range_mm", first.minRangeMm);
        extra.put("max_range_mm", first.maxRangeMm);
        extra.put("peak_bin", first.peakBin);
        extra.put("ambient_per_bin", bins.ambientPerBin);
        return new Vl53lxRange(t0.distanceMm(), t0.status(), t0.statusText(), t0.signalKcps(), t0.ambientKcps(),
                t0.sigmaMm(), first.spads >> 8, List.copyOf(targets), extra);
    }
}
