namespace Depz.Sensor.Vl53lx.Bare;

/// <summary>Register addresses and settings of the BareDriver (vl53lx_register_map.h, vl53lx_ll_device.h).</summary>
public static class BareRegs
{
    public const int PowerManagementGo1PowerForce = 0x0083;
    public const int FirmwareEnable = 0x0085;
    public const int ResultOscCalibrateVal = 0x00DE;
    public const int PatchCtrl = 0x0470;
    public const int PatchJmpEnables = 0x0472;
    public const int PatchDataEnables = 0x0474;
    public const int PatchOffset0 = 0x0476;
    public const int PatchAddress0 = 0x0496;

    public const int HistogramBinDataI2cIndex = 0x0088; // result__interrupt_status
    public const int ResultHistogramBin0_2 = 0x008E;
    public const int ResultHistogramBin23_0 = 0x00D5;
    public const int PhasecalResultReferencePhase = 0x00D6;
    public const int PhasecalResultVcselStart = 0x00D8;
    public const int ResultHistogramBin23_0Msb = 0x00D9;
    public const int ResultHistogramBin23_0Lsb = 0x00DA;
    public const int HistogramBinDataI2cSizeBytes = ResultHistogramBin23_0Lsb - HistogramBinDataI2cIndex + 1;

    public const int DeviceMeasurementModeBackToBack = 0x20;
    public const int DeviceMeasurementModeAbort = 0x80;
    public const int DeviceMeasurementModeStopMask = 0x0F;
    public const int DeviceMeasurementModeModeMask = 0xF0;
    public const int DeviceSchedulerModeStreaming = 0x01;
    public const int DeviceSchedulerModeHistogram = 0x02;
    public const int DeviceReadoutModeSingleSd = 0x00 << 2;
    public const int DeviceReadoutModeDualSd = 0x01 << 2;
    public const int GroupedParameterHoldIdMask = 0x02;
    public const int InterruptConfigNewSampleReady = 0x20;
    public const int ClearRangeInt = 0x01;
    public const int RangeStatusMask = 0x1F;

    public const int DeviceInterruptPolarityActiveLow = 0x10;
    public const int DeviceGpioModeOutputRangeAndErrorInterrupts = 0x01;
    public const int DeviceDssModeTargetRate = 1;

    public const int SequenceVhvEn = 0x01;
    public const int SequencePhasecalEn = 0x02;
    public const int SequenceDss1En = 0x08;
    public const int SequenceDss2En = 0x10;
    public const int SequenceMm1En = 0x20;
    public const int SequenceMm2En = 0x40;
    public const int SequenceRangeEn = 0x80;

    public const int SpadArrayWidth = 16;
    public const int SpadArrayHeight = 16;
    public const int RtnSpadApertureTransmission = 0x0038;
    public const int RtnSpadUnityTransmission = 0x0100;

    public const int AmbientWindowVcselPeriods = 256;
    public const int RangingWindowVcselPeriods = 2048;
    public const int MacroPeriodVcselPeriods = AmbientWindowVcselPeriods + RangingWindowVcselPeriods;
    public const int HistogramBufferSize = 24;

    /// <summary>The three preset modes VL53L3CX/VL53L4CX expose, by the distance mode that picks them.</summary>
    public static readonly IReadOnlyList<string> DistanceModes = new[] { "short", "medium", "long" };

    /// <summary>The L4CX BareDriver (STSW-IMG029) has no short mode on an L4 die.</summary>
    public static readonly IReadOnlyList<string> L4DistanceModes = new[] { "medium", "long" };

    // VL53LX_SetMeasurementTimingBudgetMicroSeconds: a fixed guard comes off the budget, the rest splits six ways.
    public const int TimingGuardUs = 1700;
    public const int TimingDivisor = 6;
    public const int FdaMaxTimingBudgetUs = 550000;
    public const int L4FdaMaxTimingBudgetUs = 200000;
}

/// <summary>SPAD geometry and timing maths (vl53lx_core.c, vl53lx_core_support.c).</summary>
public static class BareMath
{
    public static int EncodeRowCol(int row, int col) =>
        row > 7 ? (128 + (col << 3) + (15 - row)) & 0xFF : (((15 - col) << 3) + row) & 0xFF;

    /// <summary>→ (row, col).</summary>
    public static (int Row, int Col) DecodeRowCol(int spadNumber) =>
        spadNumber > 127
            ? (8 + ((255 - spadNumber) & 0x07), (spadNumber - 128) >> 3)
            : (spadNumber & 0x07, (127 - spadNumber) >> 3);

    /// <summary>→ (width, height).</summary>
    public static (int Width, int Height) DecodeZoneSize(int encodedXySize) => (encodedXySize & 0x0F, encodedXySize >> 4);

    public static int EncodeZoneSize(int width, int height) => ((height << 4) + width) & 0xFF;

    /// <summary>→ (x_ll, y_ll, x_ur, y_ur), clipped to the SPAD array.</summary>
    public static (int XLl, int YLl, int XUr, int YUr) DecodeZoneLimits(int encodedXyCentre, int encodedXySize)
    {
        var (yCentre, xCentre) = DecodeRowCol(encodedXyCentre);
        var (width, height) = DecodeZoneSize(encodedXySize);
        int xLl = Math.Max(0, xCentre - FloorDiv(width + 1, 2));
        int xUr = Math.Min(BareRegs.SpadArrayWidth - 1, xLl + width);
        int yLl = Math.Max(0, yCentre - FloorDiv(height + 1, 2));
        int yUr = Math.Min(BareRegs.SpadArrayHeight - 1, yLl + height);
        return (xLl, yLl, xUr, yUr);
    }

    private static int FloorDiv(int a, int b) => (int)Math.Floor((double)a / b);

    public static bool IsApertureLocation(int row, int col) =>
        (row % 4 == 0 && col % 4 == 2) || (row % 4 == 2 && col % 4 == 0);

    /// <summary>→ (mm inner, mm outer) effective SPADs, in 1/256 of a SPAD. Needed because our boards ship
    /// without the FMT offset calibration: the driver works out the SPAD counts the nominal rates apply to.</summary>
    public static (long Inner, long Outer) CalcMmEffectiveSpads(int encodedMmRoiCentre, int encodedMmRoiSize,
        int encodedZoneCentre, int encodedZoneSize, byte[] goodSpads, long apertureAttenuation)
    {
        var (mmXLl, mmYLl, mmXUr, mmYUr) = DecodeZoneLimits(encodedMmRoiCentre, encodedMmRoiSize);
        var (zXLl, zYLl, zXUr, zYUr) = DecodeZoneLimits(encodedZoneCentre, encodedZoneSize);
        long inner = 0, outer = 0;
        for (int y = zYLl; y <= zYUr; y++)
        {
            for (int x = zXLl; x <= zXUr; x++)
            {
                int spad = EncodeRowCol(y, x);
                if ((goodSpads[spad >> 3] & (1 << (spad & 0x07))) == 0)
                    continue;
                long attenuation = IsApertureLocation(y, x) ? apertureAttenuation : BareRegs.RtnSpadUnityTransmission;
                if (mmXLl <= x && x <= mmXUr && mmYLl <= y && y <= mmYUr)
                    inner += attenuation;
                else
                    outer += attenuation;
            }
        }
        return (inner, outer);
    }

    /// <summary><c>VL53LX_hist_combine_mm1_mm2_offsets()</c> → <c>range_offset_mm</c>, in quarter millimetres:
    /// the MM1 (inner) and MM2 (outer) offsets weighted by the share of each region the ROI covers.</summary>
    public static long CombineMm1Mm2Offsets(long mm1OffsetMm, long mm2OffsetMm, int encodedMmRoiCentre,
        int encodedMmRoiSize, int encodedZoneCentre, int encodedZoneSize, AdditionalOffsetCalData cal,
        byte[] goodSpads, long apertureAttenuation)
    {
        var (maxInner, maxOuter) = CalcMmEffectiveSpads(encodedMmRoiCentre, encodedMmRoiSize, 0xC7, 0xFF,
            goodSpads, apertureAttenuation);
        if (maxInner == 0 || maxOuter == 0)
            return 0;
        var (inner, outer) = CalcMmEffectiveSpads(encodedMmRoiCentre, encodedMmRoiSize, encodedZoneCentre,
            encodedZoneSize, goodSpads, apertureAttenuation);
        long mm1Rate = cal.MmInnerPeakSignalCountRtnMcps * inner / maxInner;
        long mm2Rate = cal.MmOuterPeakSignalCountRtnMcps * outer / maxOuter;
        long total = mm1Rate + mm2Rate;
        if (total == 0)
            return 0;
        // Signed offsets: C truncates towards zero, which C# division does too.
        return (mm1OffsetMm * mm1Rate + mm2OffsetMm * mm2Rate) * 4 / total;
    }

    /// <summary>The 32 <c>global_config__spad_enables_rtn_*</c> bytes as one bitmap.</summary>
    public static byte[] RtnGoodSpads(RegBlock nvmCopyData) =>
        Enumerable.Range(0, 32).Select(i => (byte)nvmCopyData[$"global_config__spad_enables_rtn_{i}"]).ToArray();

    public static long CalcPllPeriodUs(long fastOscFrequency) => fastOscFrequency > 0 ? (1L << 30) / fastOscFrequency : 0;

    public static long DecodeVcselPeriod(long vcselPeriodReg) => (vcselPeriodReg + 1) << 1;

    public static long CalcMacroPeriodUs(long fastOscFrequency, long vcselPeriodReg)
    {
        long macro = BareRegs.MacroPeriodVcselPeriods * CalcPllPeriodUs(fastOscFrequency);
        macro >>= 6;
        macro *= DecodeVcselPeriod(vcselPeriodReg);
        return macro >> 6;
    }

    public static long CalcTimeoutMclks(long timeoutUs, long macroPeriodUs) =>
        macroPeriodUs == 0 ? 0 : ((timeoutUs << 12) + (macroPeriodUs >> 1)) / macroPeriodUs;

    public static long EncodeTimeout(long timeoutMclks)
    {
        if (timeoutMclks <= 0)
            return 0;
        long ls = timeoutMclks - 1;
        long ms = 0;
        while ((ls & 0xFFFFFF00L) != 0)
        {
            ls >>= 1;
            ms++;
        }
        return ((ms << 8) + (ls & 0xFF)) & 0xFFFF;
    }

    public static long DecodeTimeout(long encoded) => ((encoded & 0x00FF) << (int)((encoded & 0xFF00) >> 8)) + 1;

    public static long CalcEncodedTimeout(long timeoutUs, long macroPeriodUs) =>
        EncodeTimeout(CalcTimeoutMclks(timeoutUs, macroPeriodUs));

    public static long DurationMaths(long pllPeriodUs, long vcselParmPclks, long windowVclks, long elapsedMclks)
    {
        long duration = (windowVclks * pllPeriodUs) >> 12;
        duration *= (elapsedMclks * vcselParmPclks) >> 4;
        return Math.Min(duration >> 12, 0xFFFFFFFFL);
    }

    /// <summary>Fill the six timeout registers, for VCSEL period A and then B.</summary>
    public static void CalcTimeoutRegisterValues(long phasecalUs, long mmUs, long rangeUs, long fastOscFrequency,
        RegBlock gen, RegBlock tim)
    {
        if (fastOscFrequency == 0)
            throw new DivideByZeroException("osc_measured__fast_osc__frequency is 0");
        long macro = CalcMacroPeriodUs(fastOscFrequency, tim["range_config__vcsel_period_a"]);
        gen["phasecal_config__timeout_macrop"] = Math.Min(0xFF, CalcTimeoutMclks(phasecalUs, macro));
        long encoded = CalcEncodedTimeout(mmUs, macro);
        tim["mm_config__timeout_macrop_a_hi"] = encoded >> 8;
        tim["mm_config__timeout_macrop_a_lo"] = encoded & 0xFF;
        encoded = CalcEncodedTimeout(rangeUs, macro);
        tim["range_config__timeout_macrop_a_hi"] = encoded >> 8;
        tim["range_config__timeout_macrop_a_lo"] = encoded & 0xFF;

        macro = CalcMacroPeriodUs(fastOscFrequency, tim["range_config__vcsel_period_b"]);
        encoded = CalcEncodedTimeout(mmUs, macro);
        tim["mm_config__timeout_macrop_b_hi"] = encoded >> 8;
        tim["mm_config__timeout_macrop_b_lo"] = encoded & 0xFF;
        encoded = CalcEncodedTimeout(rangeUs, macro);
        tim["range_config__timeout_macrop_b_hi"] = encoded >> 8;
        tim["range_config__timeout_macrop_b_lo"] = encoded & 0xFF;
    }
}

/// <summary>
/// <c>VL53LX_histogram_config_t</c>. It has no registers of its own: the bin sequence is smuggled into
/// static_config and timing_config fields that mean something else in lite mode
/// (<see cref="CopyToStaticCfg"/>).
/// </summary>
public sealed class HistConfig
{
    public long LowAmbEvenBin01;
    public long LowAmbEvenBin23;
    public long LowAmbEvenBin45;
    public long LowAmbOddBin01;
    public long LowAmbOddBin23;
    public long LowAmbOddBin45;
    public long MidAmbEvenBin01;
    public long MidAmbEvenBin23;
    public long MidAmbEvenBin45;
    public long MidAmbOddBin01;
    public long MidAmbOddBin2;
    public long MidAmbOddBin34;
    public long MidAmbOddBin5;
    public long UserBinOffset;
    public long HighAmbEvenBin01;
    public long HighAmbEvenBin23;
    public long HighAmbEvenBin45;
    public long HighAmbOddBin01;
    public long HighAmbOddBin23;
    public long HighAmbOddBin45;
    public long AmbThreshLow;
    public long AmbThreshHigh;
    public long SpadArraySelection;

    /// <summary>VL53LX_init_histogram_config_structure(): six even and six odd bin codes, packed two to a byte,
    /// repeated across the three ambient levels.</summary>
    public void SetBinSequence(IReadOnlyList<int> even, IReadOnlyList<int> odd)
    {
        LowAmbEvenBin01 = (even[1] << 4) + even[0];
        LowAmbEvenBin23 = (even[3] << 4) + even[2];
        LowAmbEvenBin45 = (even[5] << 4) + even[4];
        LowAmbOddBin01 = (odd[1] << 4) + odd[0];
        LowAmbOddBin23 = (odd[3] << 4) + odd[2];
        LowAmbOddBin45 = (odd[5] << 4) + odd[4];

        MidAmbEvenBin01 = LowAmbEvenBin01;
        MidAmbEvenBin23 = LowAmbEvenBin23;
        MidAmbEvenBin45 = LowAmbEvenBin45;
        MidAmbOddBin01 = LowAmbOddBin01;
        MidAmbOddBin2 = odd[2];
        MidAmbOddBin34 = (odd[4] << 4) + odd[3];
        MidAmbOddBin5 = odd[5];
        UserBinOffset = 0x00;

        HighAmbEvenBin01 = LowAmbEvenBin01;
        HighAmbEvenBin23 = LowAmbEvenBin23;
        HighAmbEvenBin45 = LowAmbEvenBin45;
        HighAmbOddBin01 = LowAmbOddBin01;
        HighAmbOddBin23 = LowAmbOddBin23;
        HighAmbOddBin45 = LowAmbOddBin45;

        AmbThreshLow = 0xFFFF;
        AmbThreshHigh = 0xFFFF;
        SpadArraySelection = 0x00;
    }

    /// <summary>VL53LX_copy_hist_cfg_to_static_cfg(): lite-mode fields the firmware reads as the bin sequence.</summary>
    public void CopyToStaticCfg(RegBlock stat, RegBlock timing, RegBlock dynamic)
    {
        stat["sigma_estimator__effective_pulse_width_ns"] = HighAmbEvenBin01;
        stat["sigma_estimator__effective_ambient_width_ns"] = HighAmbEvenBin23;
        stat["sigma_estimator__sigma_ref_mm"] = HighAmbEvenBin45;
        stat["algo__crosstalk_compensation_valid_height_mm"] = HighAmbOddBin01;
        stat["spare_host_config__static_config_spare_0"] = HighAmbOddBin23;
        stat["spare_host_config__static_config_spare_1"] = HighAmbOddBin45;

        stat["algo__range_ignore_threshold_mcps"] = (MidAmbEvenBin01 << 8) + MidAmbEvenBin23;
        stat["algo__range_ignore_valid_height_mm"] = MidAmbEvenBin45;
        stat["algo__range_min_clip"] = MidAmbOddBin01;
        stat["algo__consistency_check__tolerance"] = MidAmbOddBin2;
        stat["spare_host_config__static_config_spare_2"] = MidAmbOddBin34;
        stat["sd_config__reset_stages_msb"] = MidAmbOddBin5;
        stat["sd_config__reset_stages_lsb"] = UserBinOffset;

        timing["range_config__sigma_thresh"] = (LowAmbEvenBin01 << 8) + LowAmbEvenBin23;
        timing["range_config__min_count_rate_rtn_limit_mcps"] = (LowAmbEvenBin45 << 8) + LowAmbOddBin01;
        timing["range_config__valid_phase_low"] = LowAmbOddBin23;
        timing["range_config__valid_phase_high"] = LowAmbOddBin45;

        dynamic["system__thresh_high"] = AmbThreshLow;
        dynamic["system__thresh_low"] = AmbThreshHigh;
        dynamic["system__enable_xtalk_per_quadrant"] = SpadArraySelection;
    }

    /// <summary>VL53LX_hist_get_bin_sequence_config(): which sequence the device used for this frame, even or
    /// odd by the stream count.</summary>
    public int[] BinSequence(long streamCount, long ambientEventsSum)
    {
        string level = "mid";
        if (ambientEventsSum > 1024 * AmbThreshHigh)
            level = "high";
        if (ambientEventsSum < 1024 * AmbThreshLow)
            level = "low";

        long[] packed;
        if ((streamCount & 0x01) == 0)
        {
            packed = level switch
            {
                "low" => new[] { LowAmbEvenBin01, LowAmbEvenBin23, LowAmbEvenBin45 },
                "high" => new[] { HighAmbEvenBin01, HighAmbEvenBin23, HighAmbEvenBin45 },
                _ => new[] { MidAmbEvenBin01, MidAmbEvenBin23, MidAmbEvenBin45 },
            };
        }
        else if (level == "mid")
        {
            // The mid odd sequence is the one packed differently.
            return new[]
            {
                (int)(MidAmbOddBin01 & 0x0F), (int)(MidAmbOddBin01 >> 4), (int)MidAmbOddBin2,
                (int)(MidAmbOddBin34 >> 4), (int)(MidAmbOddBin34 & 0x0F), (int)(MidAmbOddBin5 & 0x0F),
            };
        }
        else
        {
            packed = level == "low"
                ? new[] { LowAmbOddBin01, LowAmbOddBin23, LowAmbOddBin45 }
                : new[] { HighAmbOddBin01, HighAmbOddBin23, HighAmbOddBin45 };
        }
        var seq = new int[6];
        for (int i = 0; i < 3; i++)
        {
            seq[2 * i] = (int)(packed[i] & 0x0F);
            seq[2 * i + 1] = (int)(packed[i] >> 4);
        }
        return seq;
    }
}

/// <summary><c>VL53LX_hist_post_process_config_t</c>, as <c>VL53LX_init_hist_post_process_config_struct()</c>
/// leaves it; the preset modes set the valid phase window here.</summary>
public sealed class HistPostProcessConfig
{
    private static long D(string name) => BareTables.Defaults[name];

    public long HistAlgoSelect = D("TUNINGPARM_HIST_ALGO_SELECT_DEFAULT");
    public long HistTargetOrder = D("TUNINGPARM_HIST_TARGET_ORDER_DEFAULT");
    public long FilterWoi0 = D("TUNINGPARM_HIST_FILTER_WOI_0_DEFAULT");
    public long FilterWoi1 = D("TUNINGPARM_HIST_FILTER_WOI_1_DEFAULT");
    public long HistAmbEstMethod = D("TUNINGPARM_HIST_AMB_EST_METHOD_DEFAULT");
    public long AmbientThreshSigma0 = D("TUNINGPARM_HIST_AMB_THRESH_SIGMA_0_DEFAULT");
    public long AmbientThreshSigma1 = D("TUNINGPARM_HIST_AMB_THRESH_SIGMA_1_DEFAULT");
    public long AmbientThreshEventsScaler = D("TUNINGPARM_HIST_AMB_EVENTS_SCALER_DEFAULT");
    public long MinAmbientThreshEvents = D("TUNINGPARM_HIST_MIN_AMB_THRESH_EVENTS_DEFAULT");
    public long NoiseThreshold = D("TUNINGPARM_HIST_NOISE_THRESHOLD_DEFAULT");
    public long SignalTotalEventsLimit = D("TUNINGPARM_HIST_SIGNAL_TOTAL_EVENTS_LIMIT_DEFAULT");
    public long SigmaEstimatorSigmaRefMm = D("TUNINGPARM_HIST_SIGMA_EST_REF_MM_DEFAULT");
    public long SigmaThresh = D("TUNINGPARM_HIST_SIGMA_THRESH_MM_DEFAULT");
    public long RangeOffsetMm;
    public long GainFactor = D("TUNINGPARM_HIST_GAIN_FACTOR_DEFAULT");
    public long ValidPhaseLow = 0x08;
    public long ValidPhaseHigh = 0x88;
    public long ConsistencyPhaseTolerance = D("TUNINGPARM_CONSISTENCY_HIST_PHASE_TOLERANCE_DEFAULT");
    public long ConsistencyEventSigma = D("TUNINGPARM_CONSISTENCY_HIST_EVENT_SIGMA_DEFAULT");
    public long ConsistencyEventMinSpadCount = D("TUNINGPARM_CONSISTENCY_HIST_EVENT_SIGMA_MIN_SPAD_LIMIT_DEFAULT");
    public long ConsistencyMinMaxTolerance = D("TUNINGPARM_CONSISTENCY_HIST_MIN_MAX_TOLERANCE_MM_DEFAULT");
    public long CrosstalkCompensationEnable;
    public long CrosstalkDetectMinValidRangeMm = D("TUNINGPARM_XTALK_DETECT_MIN_VALID_RANGE_MM_DEFAULT");
    public long CrosstalkDetectMaxValidRangeMm = D("TUNINGPARM_XTALK_DETECT_MAX_VALID_RANGE_MM_DEFAULT");
    public long CrosstalkDetectMaxValidRateKcps = D("TUNINGPARM_XTALK_DETECT_MAX_VALID_RATE_KCPS_DEFAULT");
    public long CrosstalkDetectMaxSigmaMm = D("TUNINGPARM_XTALK_DETECT_MAX_SIGMA_MM_DEFAULT");
    public long CrosstalkDetectEventSigma = D("TUNINGPARM_XTALK_DETECT_EVENT_SIGMA_DEFAULT");
    public long CrosstalkDetectMinMaxTolerance = D("TUNINGPARM_XTALK_DETECT_MIN_MAX_TOLERANCE_DEFAULT");
    // Filled in by ReadP2pData() from the customer NVM block.
    public long CrosstalkCompensationPlaneOffsetKcps;
    public long CrosstalkCompensationXPlaneGradientKcps;
    public long CrosstalkCompensationYPlaneGradientKcps;
}

/// <summary>
/// <c>VL53LX_histogram_bin_data_t</c>: 24 bins plus everything the post-processing needs to read them — the
/// VCSEL period they were taken at, the phase of zero distance, the bin sequence and the ambient estimate.
/// </summary>
public sealed class HistogramBinData
{
    public long ResultInterruptStatus;
    public long ResultRangeStatus;
    public long ResultReportStatus;
    public long ResultStreamCount;
    public long ResultDssActualEffectiveSpads;
    public long PhasecalResultReferencePhase;
    public long PhasecalResultVcselStart;
    public long[] BinData = new long[BareRegs.HistogramBufferSize];
    public long ZoneId;
    public long FirstBin;                                       // VL53LX_p_019
    public long NumberOfBins = BareRegs.HistogramBufferSize;    // VL53LX_p_020, buffer
    public long BinsInData = BareRegs.HistogramBufferSize;      // VL53LX_p_021, in use
    public long CalConfigVcselStart;
    public long VcselWidth;
    public long FastOscFrequency;                               // VL53LX_p_015
    public long VcselPeriod;                                    // VL53LX_p_005, the register value
    public int[] BinSeq = new int[6];
    public long[] BinRep = new long[6];                         // how often each code was sampled
    public long MinBinValue;
    public long MaxBinValue;
    public long NumberOfAmbientBins;
    public long NumberOfAmbientSamples;
    public long AmbientEventsSum;
    public long AmbientPerBin;                                  // VL53LX_p_028
    public long TotalPeriodsElapsed;
    public long PeakDurationUs;
    public long WoiDurationUs;
    public long ZeroDistancePhase;
    public long RoiConfigUserRoiCentreSpad;
    public long RoiConfigUserRoiRequestedGlobalXySize;

    public long RangeStatus => ResultRangeStatus & BareRegs.RangeStatusMask;

    public HistogramBinData Clone()
    {
        var c = (HistogramBinData)MemberwiseClone();
        c.BinData = BinData.ToArray();
        c.BinSeq = BinSeq.ToArray();
        c.BinRep = BinRep.ToArray();
        return c;
    }

    /// <summary>The fields under the C driver's (and the Python SDK's) names.</summary>
    public IReadOnlyDictionary<string, object?> Fields() => new Dictionary<string, object?>
    {
        ["result__interrupt_status"] = ResultInterruptStatus,
        ["result__range_status"] = ResultRangeStatus,
        ["result__report_status"] = ResultReportStatus,
        ["result__stream_count"] = ResultStreamCount,
        ["result__dss_actual_effective_spads"] = ResultDssActualEffectiveSpads,
        ["phasecal_result__reference_phase"] = PhasecalResultReferencePhase,
        ["phasecal_result__vcsel_start"] = PhasecalResultVcselStart,
        ["bin_data"] = BinData.ToList(),
        ["zone_id"] = ZoneId,
        ["first_bin"] = FirstBin,
        ["number_of_bins"] = NumberOfBins,
        ["bins_in_data"] = BinsInData,
        ["cal_config__vcsel_start"] = CalConfigVcselStart,
        ["vcsel_width"] = VcselWidth,
        ["fast_osc_frequency"] = FastOscFrequency,
        ["vcsel_period"] = VcselPeriod,
        ["bin_seq"] = BinSeq.Select(x => (long)x).ToList(),
        ["bin_rep"] = BinRep.ToList(),
        ["min_bin_value"] = MinBinValue,
        ["max_bin_value"] = MaxBinValue,
        ["number_of_ambient_bins"] = NumberOfAmbientBins,
        ["number_of_ambient_samples"] = NumberOfAmbientSamples,
        ["ambient_events_sum"] = AmbientEventsSum,
        ["ambient_per_bin"] = AmbientPerBin,
        ["total_periods_elapsed"] = TotalPeriodsElapsed,
        ["peak_duration_us"] = PeakDurationUs,
        ["woi_duration_us"] = WoiDurationUs,
        ["zero_distance_phase"] = ZeroDistancePhase,
        ["roi_config__user_roi_centre_spad"] = RoiConfigUserRoiCentreSpad,
        ["roi_config__user_roi_requested_global_xy_size"] = RoiConfigUserRoiRequestedGlobalXySize,
    };

    public override string ToString() =>
        $"<HistogramBinData stream {ResultStreamCount} status {RangeStatus} spads {ResultDssActualEffectiveSpads} " +
        $"bins [{string.Join(", ", BinData)}]>";
}

/// <summary>The ll-driver device states, single-zone case.</summary>
public enum BareDeviceState
{
    SwStandby,
    RangingWaitGphSync,
    RangingOutputData,
    RangingDssAuto,
}

/// <summary>
/// <c>VL53LX_update_ll_driver_{rd,cfg}_state()</c>, reduced to one zone: the GPH handshake (the id stamped
/// into the dynamic config tells the device a configuration is new) and the timing-status flip (whether a
/// frame ran on VCSEL period A or B, which the post-processing has to know).
/// </summary>
public sealed class LlState
{
    public BareDeviceState CfgDeviceState;
    public long CfgStreamCount;
    public long CfgGphId;
    public long CfgTimingStatus;
    public BareDeviceState RdDeviceState;
    public long RdStreamCount;
    public long RdGphId;
    public long RdTimingStatus;

    public LlState() => Reset();

    public void Reset()
    {
        CfgDeviceState = BareDeviceState.SwStandby;
        CfgStreamCount = 0;
        CfgGphId = BareRegs.GroupedParameterHoldIdMask;
        CfgTimingStatus = 0;
        RdDeviceState = BareDeviceState.SwStandby;
        RdStreamCount = 0;
        RdGphId = BareRegs.GroupedParameterHoldIdMask;
        RdTimingStatus = 0;
    }

    public void UpdateRd(long modeStart, long groupedParameterHold)
    {
        if ((modeStart & BareRegs.DeviceMeasurementModeModeMask) == 0)
        {
            RdDeviceState = BareDeviceState.SwStandby;
            RdStreamCount = 0;
            RdGphId = BareRegs.GroupedParameterHoldIdMask;
            RdTimingStatus = 0;
            return;
        }
        RdStreamCount = RdStreamCount == 0xFF ? 0x80 : RdStreamCount + 1;
        RdGphId ^= BareRegs.GroupedParameterHoldIdMask;
        switch (RdDeviceState)
        {
            case BareDeviceState.SwStandby:
                RdDeviceState = (groupedParameterHold & BareRegs.GroupedParameterHoldIdMask) != 0
                    ? BareDeviceState.RangingWaitGphSync
                    : BareDeviceState.RangingOutputData;
                RdStreamCount = 0;
                RdTimingStatus = 0;
                break;
            case BareDeviceState.RangingWaitGphSync:
                RdStreamCount = 0;
                RdDeviceState = BareDeviceState.RangingOutputData;
                break;
            case BareDeviceState.RangingOutputData:
                RdTimingStatus ^= 0x01;
                break;
            default:
                Reset();
                break;
        }
    }

    public void UpdateCfg(long modeStart)
    {
        if ((modeStart & BareRegs.DeviceMeasurementModeModeMask) == 0)
        {
            CfgDeviceState = BareDeviceState.SwStandby;
            CfgStreamCount = 0;
            CfgGphId = BareRegs.GroupedParameterHoldIdMask;
            CfgTimingStatus = 0;
            return;
        }
        CfgStreamCount = CfgStreamCount == 0xFF ? 0x80 : CfgStreamCount + 1;
        CfgGphId ^= BareRegs.GroupedParameterHoldIdMask;
        switch (CfgDeviceState)
        {
            case BareDeviceState.SwStandby:
                CfgTimingStatus ^= 0x01;
                CfgStreamCount = 1;
                CfgDeviceState = BareDeviceState.RangingDssAuto;
                break;
            case BareDeviceState.RangingDssAuto:
                CfgTimingStatus ^= 0x01;
                break;
            default:
                Reset();
                break;
        }
    }
}

/// <summary>
/// ST's VL53LX BareDriver ranging path, on top of <see cref="DeviceImage"/>: <c>VL53LX_read_p2p_data()</c>, the
/// histogram preset modes, <c>VL53LX_set_timeouts_us()</c>, <c>VL53LX_init_and_start_range()</c> and
/// <c>VL53LX_get_histogram_bin_data()</c>. Nothing here writes a register on its own: the preset builders edit
/// the host-side image and the whole span 0x0001..0x0087 goes out as one transfer when ranging starts.
/// </summary>
public sealed class BareDriver
{
    private readonly Vl53lxPlatform _p;

    // The distance-mode tails (vl53lx_api_preset_modes.c): bin sequence, VCSEL periods, timeouts, phases.
    private static readonly Dictionary<string, (int[] Even, int[] Odd, int VcselA, int VcselB, int MmA, int MmB, int RangeA,
        int RangeB, int CalVcselStart, int ValidPhaseHigh, string PhaseRtn, string PhaseRef, int ExtraSequence)> Tails = new()
    {
        ["long"] = (new[] { 7, 0, 1, 2, 3, 4 }, new[] { 0, 1, 2, 3, 4, 5 }, 0x09, 0x0B, 0x0021, 0x001B, 0x0029, 0x0022,
            0x09, 0x88, "tp_init_phase_rtn_hist_long", "tp_init_phase_ref_hist_long", 0),
        ["medium"] = (new[] { 7, 0, 1, 1, 2, 2 }, new[] { 0, 1, 2, 1, 2, 3 }, 0x05, 0x07, 0x0036, 0x0028, 0x0044, 0x0033,
            0x05, 0x48, "tp_init_phase_rtn_hist_med", "tp_init_phase_ref_hist_med", 0),
        ["short"] = (new[] { 7, 7, 0, 1, 1, 1 }, new[] { 0, 1, 1, 1, 2, 2 }, 0x03, 0x05, 0x0052, 0x0037, 0x0066, 0x0044,
            0x03, 0x28, "tp_init_phase_rtn_hist_short", "tp_init_phase_ref_hist_short", BareRegs.SequenceMm1En),
    };

    // the bin sequence of VL53LX_preset_mode_histogram_ranging()
    private static readonly int[] HistogramRangingEvenBins = { 7, 0, 1, 2, 3, 4 };
    private static readonly int[] HistogramRangingOddBins = { 0, 1, 2, 3, 4, 5 };

    private static readonly Dictionary<string, string> PhasecalKey = new()
    {
        ["short"] = "tp_phasecal_timeout_hist_short_us",
        ["medium"] = "tp_phasecal_timeout_hist_med_us",
        ["long"] = "tp_phasecal_timeout_hist_long_us",
    };

    // Carried from frame to frame: the C driver keeps one bin_data struct and reads last frame's ambient out of
    // it when it picks the bin sequence, before this frame's ambient is estimated.
    private long _ambientEventsSum;

    public BareDriver(Vl53lxPlatform platform)
    {
        _p = platform;
        Img = new DeviceImage(platform);
        Nvm = new NvmReader(platform, Img);
    }

    public DeviceImage Img { get; }
    public NvmReader Nvm { get; }
    public Dictionary<string, long> Tuning { get; } = new(BareTables.Tuning);
    public HistConfig HistCfg { get; } = new();
    public HistPostProcessConfig Hpp { get; private set; } = new();

    // Factory data with no register of its own (read_p2p_data).
    public byte[] RtnGoodSpadsMap { get; private set; } = new byte[32];
    public OpticalCentre? OpticalCentre { get; private set; }
    public CalPeakRateMap? CalPeakRateMap { get; private set; }
    public AdditionalOffsetCalData? AddOffCalData { get; private set; }
    public IReadOnlyDictionary<string, long>? FmtDmaxCal { get; private set; }
    public IReadOnlyDictionary<string, long>? MmRoi { get; private set; }
    public long ResultOscCalibrateVal { get; private set; }

    public string PresetMode { get; private set; } = "medium";
    public long PhasecalConfigTimeoutUs { get; private set; } = 1000;
    public long MmConfigTimeoutUs { get; private set; } = 2000;
    public long RangeConfigTimeoutUs { get; private set; } = 13000;
    public long InterMeasurementPeriodMs { get; private set; } = 100;
    public long MeasurementMode { get; private set; }
    public LlState State { get; } = new();

    /// <summary>VL53LX_read_p2p_data(): three register blocks and four NVM regions (~0.1 s over USB).</summary>
    public void ReadP2pData()
    {
        Img.Pull("static_nvm_managed");
        Img.Pull("customer_nvm_managed");
        Img.Pull("nvm_copy_data");
        RtnGoodSpadsMap = BareMath.RtnGoodSpads(Img.NvmCopyData);

        var customer = Img.CustomerNvm;
        Hpp.CrosstalkCompensationPlaneOffsetKcps = customer["algo__crosstalk_compensation_plane_offset_kcps"];
        Hpp.CrosstalkCompensationXPlaneGradientKcps = customer["algo__crosstalk_compensation_x_plane_gradient_kcps"];
        Hpp.CrosstalkCompensationYPlaneGradientKcps = customer["algo__crosstalk_compensation_y_plane_gradient_kcps"];

        OpticalCentre = Nvm.ReadOpticalCentre();
        CalPeakRateMap = Nvm.ReadCalPeakRateMap();
        var cal = Nvm.ReadAdditionalOffsetCalData();

        // Our boards have no FMT offset calibration. ST's fallback: nominal MM1/MM2 peak rates, and the effective
        // SPAD counts worked out here.
        if (cal.MmInnerPeakSignalCountRtnMcps == 0 && cal.MmOuterPeakSignalCountRtnMcps == 0)
        {
            var (inner, outer) = BareMath.CalcMmEffectiveSpads(
                (int)Img.NvmCopyData["roi_config__mode_roi_centre_spad"], (int)Img.NvmCopyData["roi_config__mode_roi_xy_size"],
                0xC7, 0xFF, RtnGoodSpadsMap, BareRegs.RtnSpadApertureTransmission);
            cal = cal with
            {
                MmInnerPeakSignalCountRtnMcps = 0x0080,
                MmOuterPeakSignalCountRtnMcps = 0x0180,
                MmInnerActualEffectiveSpads = inner,
                MmOuterActualEffectiveSpads = outer,
            };
        }
        AddOffCalData = cal;

        var fmt = Nvm.ReadFmtRangeResults();
        FmtDmaxCal = new Dictionary<string, long>
        {
            ["ref__actual_effective_spads"] = fmt.ActualEffectiveRtnSpads,
            ["ref__peak_signal_count_rate_mcps"] = fmt.PeakSignalCountRateRtnMcps,
            ["ref__distance_mm"] = fmt.MeasuredDistanceMm,
            ["ref_reflectance_pc"] = CalPeakRateMap.CalReflectancePc != 0 ? CalPeakRateMap.CalReflectancePc : 0x0014,
            ["coverglass_transmission"] = 0x0100,
        };

        ResultOscCalibrateVal = _p.ReadWord(BareRegs.ResultOscCalibrateVal);

        if (Img.StaticNvm["osc_measured__fast_osc__frequency"] < 0x1000)
            Img.StaticNvm["osc_measured__fast_osc__frequency"] = 0xBCCC;

        // VL53LX_get_mode_mitigation_roi
        var (y, x) = BareMath.DecodeRowCol((int)Img.NvmCopyData["roi_config__mode_roi_centre_spad"]);
        long xySize = Img.NvmCopyData["roi_config__mode_roi_xy_size"];
        MmRoi = new Dictionary<string, long>
        {
            ["x_centre"] = x, ["y_centre"] = y, ["width"] = xySize & 0x0F, ["height"] = xySize >> 4,
        };

        if (OpticalCentre.XCentre == 0 && OpticalCentre.YCentre == 0)
            OpticalCentre = new OpticalCentre(x << 4, y << 4);
    }

    /// <summary>VL53LX_data_init + VL53LX_DataInit: the factory data, the default preset mode and timing budget.
    /// Leaves the sensor untouched apart from the reads.</summary>
    public void DataInit()
    {
        State.Reset();
        Hpp = new HistPostProcessConfig();
        ReadP2pData();
        SetPresetMode("medium", interMeasurementPeriodMs: 1000);
        SetMeasurementTimingBudgetUs(33333);
    }

    /// <summary>VL53LX_set_preset_mode() for the three histogram ranging modes, by distance mode.</summary>
    public void SetPresetMode(string mode = "medium", long? dssTargetMcps = null, long? interMeasurementPeriodMs = null)
    {
        if (!BareRegs.DistanceModes.Contains(mode))
            throw new ArgumentException($"unknown distance mode '{mode}'", nameof(mode));
        PresetMode = mode;
        long dss = dssTargetMcps ?? Tuning["tp_dss_target_histo_mcps"];
        long period = interMeasurementPeriodMs ?? InterMeasurementPeriodMs;

        // VL53LX_get_preset_mode_timing_cfg: only the phasecal timeout differs between the three modes.
        long phasecalUs = Tuning[PhasecalKey[mode]];
        long mmUs = Tuning["tp_mm_timeout_histo_us"];
        long rangeUs = Tuning["tp_range_timeout_histo_us"];

        State.Reset();
        PresetModeHistogramRanging();
        var t = Tails[mode];
        PresetTail(t.Even, t.Odd, t.VcselA, t.VcselB, t.MmA, t.MmB, t.RangeA, t.RangeB, t.CalVcselStart,
            t.ValidPhaseHigh, t.PhaseRtn, t.PhaseRef, t.ExtraSequence);

        Img.StaticConfig["dss_config__target_total_rate_mcps"] = dss;
        SetTimeoutsUs(phasecalUs, mmUs, rangeUs);
        SetInterMeasurementPeriodMs(period);
        UpdateRangeOffset();
    }

    // get_device_results() recomputes hpp.range_offset_mm every frame under MM1_MM2_OFFSETS; its inputs are the
    // factory data and the user ROI, so once per preset is the same answer.
    private void UpdateRangeOffset()
    {
        if (AddOffCalData is null)
            return;
        var customer = Img.CustomerNvm;
        var nvm = Img.NvmCopyData;
        var dynamic = Img.DynamicConfig;
        Hpp.RangeOffsetMm = BareMath.CombineMm1Mm2Offsets(
            customer["mm_config__inner_offset_mm"], customer["mm_config__outer_offset_mm"],
            (int)nvm["roi_config__mode_roi_centre_spad"], (int)nvm["roi_config__mode_roi_xy_size"],
            (int)dynamic["roi_config__user_roi_centre_spad"], (int)dynamic["roi_config__user_roi_requested_global_xy_size"],
            AddOffCalData, RtnGoodSpadsMap, Img.GeneralConfig["dss_config__aperture_attenuation"]);
    }

    /// <summary><c>IsL4()</c> of the L4CX BareDriver: the die, read from NVM, not the product name. 0xEC is the L4ED.</summary>
    public bool IsL4()
    {
        var nvm = Img.NvmCopyData;
        return nvm["identification__module_type"] == 0xAA && nvm["identification__model_id"] is 0xEB or 0xEC;
    }

    public IReadOnlyList<string> DistanceModesForDie() => IsL4() ? BareRegs.L4DistanceModes : BareRegs.DistanceModes;

    public long FdaMaxTimingBudgetUs() => IsL4() ? BareRegs.L4FdaMaxTimingBudgetUs : BareRegs.FdaMaxTimingBudgetUs;

    /// <summary>VL53LX_SetDistanceMode(): the preset mode with the caller's timeouts put back. Short is refused on
    /// an L4 die, as the L4CX BareDriver does (the A frame of the pair ranges on the wrong side of the wrap).</summary>
    public void SetDistanceMode(string mode)
    {
        if (!DistanceModesForDie().Contains(mode))
            throw new ArgumentException($"distance mode {mode} not available on this die", nameof(mode));
        long phasecalUs = PhasecalConfigTimeoutUs, mmUs = MmConfigTimeoutUs, rangeUs = RangeConfigTimeoutUs;
        SetPresetMode(mode);
        SetTimeoutsUs(phasecalUs, mmUs, rangeUs);
    }

    // VL53LX_preset_mode_standard_ranging(): the base every histogram mode is built on.
    private void PresetModeStandardRanging()
    {
        var tp = Tuning;
        var stat = Img.StaticConfig;
        var general = Img.GeneralConfig;
        var timing = Img.TimingConfig;
        var dynamic = Img.DynamicConfig;
        var system = Img.SystemControl;

        stat["dss_config__target_total_rate_mcps"] = 0x0A00;
        stat["debug__ctrl"] = 0x00;
        stat["test_mode__ctrl"] = 0x00;
        stat["clk_gating__ctrl"] = 0x00;
        stat["nvm_bist__ctrl"] = 0x00;
        stat["nvm_bist__num_nvm_words"] = 0x00;
        stat["nvm_bist__start_address"] = 0x00;
        stat["host_if__status"] = 0x00;
        stat["pad_i2c_hv__config"] = 0x00;
        stat["pad_i2c_hv__extsup_config"] = 0x00;
        stat["gpio_hv_pad__ctrl"] = 0x00;
        stat["gpio_hv_mux__ctrl"] = BareRegs.DeviceInterruptPolarityActiveLow | BareRegs.DeviceGpioModeOutputRangeAndErrorInterrupts;
        stat["gpio__tio_hv_status"] = 0x02;
        stat["gpio__fio_hv_status"] = 0x00;
        stat["ana_config__spad_sel_pswidth"] = 0x02;
        stat["ana_config__vcsel_pulse_width_offset"] = 0x08;
        stat["ana_config__fast_osc__config_ctrl"] = 0x00;
        stat["sigma_estimator__effective_pulse_width_ns"] = tp["tp_lite_sigma_est_pulse_width_ns"];
        stat["sigma_estimator__effective_ambient_width_ns"] = tp["tp_lite_sigma_est_amb_width_ns"];
        stat["sigma_estimator__sigma_ref_mm"] = tp["tp_lite_sigma_ref_mm"];
        stat["algo__crosstalk_compensation_valid_height_mm"] = 0x01;
        stat["spare_host_config__static_config_spare_0"] = 0x00;
        stat["spare_host_config__static_config_spare_1"] = 0x00;
        stat["algo__range_ignore_threshold_mcps"] = 0x0000;
        stat["algo__range_ignore_valid_height_mm"] = 0xFF;
        stat["algo__range_min_clip"] = tp["tp_lite_min_clip"];
        stat["algo__consistency_check__tolerance"] = tp["tp_consistency_lite_phase_tolerance"];
        stat["spare_host_config__static_config_spare_2"] = 0x00;
        stat["sd_config__reset_stages_msb"] = 0x00;
        stat["sd_config__reset_stages_lsb"] = 0x00;

        general["gph_config__stream_count_update_value"] = 0x00;
        general["global_config__stream_divider"] = 0x00;
        general["system__interrupt_config_gpio"] = BareRegs.InterruptConfigNewSampleReady;
        general["cal_config__vcsel_start"] = 0x0B;
        general["cal_config__repeat_rate"] = tp["tp_cal_repeat_rate"];
        general["global_config__vcsel_width"] = 0x02;
        general["phasecal_config__timeout_macrop"] = 0x0D;
        general["phasecal_config__target"] = tp["tp_phasecal_target"];
        general["phasecal_config__override"] = 0x00;
        general["dss_config__roi_mode_control"] = BareRegs.DeviceDssModeTargetRate;
        general["system__thresh_rate_high"] = 0x0000;
        general["system__thresh_rate_low"] = 0x0000;
        general["dss_config__manual_effective_spads_select"] = 0x8C00;
        general["dss_config__manual_block_select"] = 0x00;
        general["dss_config__aperture_attenuation"] = 0x38;
        general["dss_config__max_spads_limit"] = 0xFF;
        general["dss_config__min_spads_limit"] = 0x01;

        timing["mm_config__timeout_macrop_a_hi"] = 0x00;
        timing["mm_config__timeout_macrop_a_lo"] = 0x1A;
        timing["mm_config__timeout_macrop_b_hi"] = 0x00;
        timing["mm_config__timeout_macrop_b_lo"] = 0x20;
        timing["range_config__timeout_macrop_a_hi"] = 0x01;
        timing["range_config__timeout_macrop_a_lo"] = 0xCC;
        timing["range_config__vcsel_period_a"] = 0x0B;
        timing["range_config__timeout_macrop_b_hi"] = 0x01;
        timing["range_config__timeout_macrop_b_lo"] = 0xF5;
        timing["range_config__vcsel_period_b"] = 0x09;
        timing["range_config__sigma_thresh"] = tp["tp_lite_med_sigma_thresh_mm"];
        timing["range_config__min_count_rate_rtn_limit_mcps"] = tp["tp_lite_med_min_count_rate_rtn_mcps"];
        timing["range_config__valid_phase_low"] = 0x08;
        timing["range_config__valid_phase_high"] = 0x78;
        timing["system__intermeasurement_period"] = 0x00000000;
        timing["system__fractional_enable"] = 0x00;

        // The struct's initial state; every histogram mode overwrites it.
        var h = HistCfg;
        (h.LowAmbEvenBin01, h.LowAmbEvenBin23, h.LowAmbEvenBin45) = (0x07, 0x21, 0x43);
        (h.LowAmbOddBin01, h.LowAmbOddBin23, h.LowAmbOddBin45) = (0x10, 0x32, 0x54);
        (h.MidAmbEvenBin01, h.MidAmbEvenBin23, h.MidAmbEvenBin45) = (0x07, 0x21, 0x43);
        (h.MidAmbOddBin01, h.MidAmbOddBin2, h.MidAmbOddBin34, h.MidAmbOddBin5) = (0x10, 0x02, 0x43, 0x05);
        h.UserBinOffset = 0x00;
        (h.HighAmbEvenBin01, h.HighAmbEvenBin23, h.HighAmbEvenBin45) = (0x07, 0x21, 0x43);
        (h.HighAmbOddBin01, h.HighAmbOddBin23, h.HighAmbOddBin45) = (0x10, 0x32, 0x54);
        h.AmbThreshLow = 0xFFFF;
        h.AmbThreshHigh = 0xFFFF;
        h.SpadArraySelection = 0x00;

        dynamic["system__grouped_parameter_hold_0"] = 0x01;
        dynamic["system__thresh_high"] = 0x0000;
        dynamic["system__thresh_low"] = 0x0000;
        dynamic["system__enable_xtalk_per_quadrant"] = 0x00;
        dynamic["system__seed_config"] = tp["tp_lite_seed_cfg"];
        dynamic["sd_config__woi_sd0"] = 0x0B;
        dynamic["sd_config__woi_sd1"] = 0x09;
        dynamic["sd_config__initial_phase_sd0"] = tp["tp_init_phase_rtn_lite_med"];
        dynamic["sd_config__initial_phase_sd1"] = tp["tp_init_phase_ref_lite_med"];
        dynamic["system__grouped_parameter_hold_1"] = 0x01;
        dynamic["sd_config__first_order_select"] = tp["tp_lite_first_order_select"];
        dynamic["sd_config__quantifier"] = tp["tp_lite_quantifier"];
        dynamic["roi_config__user_roi_centre_spad"] = 0xC7;
        dynamic["roi_config__user_roi_requested_global_xy_size"] = 0xFF;
        dynamic["system__sequence_config"] = BareRegs.SequenceVhvEn | BareRegs.SequencePhasecalEn | BareRegs.SequenceDss1En
                                             | BareRegs.SequenceDss2En | BareRegs.SequenceMm2En | BareRegs.SequenceRangeEn;
        dynamic["system__grouped_parameter_hold"] = 0x02;

        system["system__stream_count_ctrl"] = 0x00;
        system["firmware__enable"] = 0x01;
        system["system__interrupt_clear"] = BareRegs.ClearRangeInt;
        system["system__mode_start"] = BareRegs.DeviceSchedulerModeStreaming | BareRegs.DeviceReadoutModeSingleSd
                                       | BareRegs.DeviceMeasurementModeBackToBack;
    }

    // VL53LX_preset_mode_histogram_ranging(): standard ranging, then the histogram engine.
    private void PresetModeHistogramRanging()
    {
        PresetModeStandardRanging();

        Img.StaticConfig["dss_config__target_total_rate_mcps"] = 0x1400;
        HistCfg.SetBinSequence(HistogramRangingEvenBins, HistogramRangingOddBins);

        var timing = Img.TimingConfig;
        var dynamic = Img.DynamicConfig;
        timing["range_config__vcsel_period_a"] = 0x09;
        timing["range_config__vcsel_period_b"] = 0x0B;
        dynamic["sd_config__woi_sd0"] = 0x09;
        dynamic["sd_config__woi_sd1"] = 0x0B;
        timing["mm_config__timeout_macrop_a_hi"] = 0x00;
        timing["mm_config__timeout_macrop_a_lo"] = 0x20;
        timing["mm_config__timeout_macrop_b_hi"] = 0x00;
        timing["mm_config__timeout_macrop_b_lo"] = 0x1A;
        timing["range_config__timeout_macrop_a_hi"] = 0x00;
        timing["range_config__timeout_macrop_a_lo"] = 0x28;
        timing["range_config__timeout_macrop_b_hi"] = 0x00;
        timing["range_config__timeout_macrop_b_lo"] = 0x21;
        Img.GeneralConfig["phasecal_config__timeout_macrop"] = 0xF5;

        Hpp.ValidPhaseLow = 0x08;
        Hpp.ValidPhaseHigh = 0x88;

        HistCfg.CopyToStaticCfg(Img.StaticConfig, timing, dynamic);
        dynamic["system__sequence_config"] = BareRegs.SequenceVhvEn | BareRegs.SequencePhasecalEn | BareRegs.SequenceDss1En
                                             | BareRegs.SequenceDss2En | BareRegs.SequenceRangeEn;
        Img.SystemControl["system__mode_start"] = BareRegs.DeviceSchedulerModeHistogram | BareRegs.DeviceReadoutModeDualSd
                                                  | BareRegs.DeviceMeasurementModeBackToBack;
    }

    // The shape all three distance modes share: a bin sequence, a VCSEL period pair and its timeouts.
    private void PresetTail(int[] even, int[] odd, int vcselA, int vcselB, int mmA, int mmB, int rangeA, int rangeB,
        int calVcselStart, int validPhaseHigh, string phaseRtn, string phaseRef, int extraSequence)
    {
        var stat = Img.StaticConfig;
        var general = Img.GeneralConfig;
        var timing = Img.TimingConfig;
        var dynamic = Img.DynamicConfig;

        HistCfg.SetBinSequence(even, odd);
        HistCfg.CopyToStaticCfg(stat, timing, dynamic);

        timing["range_config__vcsel_period_a"] = vcselA;
        timing["range_config__vcsel_period_b"] = vcselB;
        timing["mm_config__timeout_macrop_a_hi"] = mmA >> 8;
        timing["mm_config__timeout_macrop_a_lo"] = mmA & 0xFF;
        timing["mm_config__timeout_macrop_b_hi"] = mmB >> 8;
        timing["mm_config__timeout_macrop_b_lo"] = mmB & 0xFF;
        timing["range_config__timeout_macrop_a_hi"] = rangeA >> 8;
        timing["range_config__timeout_macrop_a_lo"] = rangeA & 0xFF;
        timing["range_config__timeout_macrop_b_hi"] = rangeB >> 8;
        timing["range_config__timeout_macrop_b_lo"] = rangeB & 0xFF;

        general["cal_config__vcsel_start"] = calVcselStart;
        general["phasecal_config__timeout_macrop"] = 0xF5;

        dynamic["sd_config__woi_sd0"] = vcselA;
        dynamic["sd_config__woi_sd1"] = vcselB;
        dynamic["sd_config__initial_phase_sd0"] = Tuning[phaseRtn];
        dynamic["sd_config__initial_phase_sd1"] = Tuning[phaseRef];
        dynamic["system__sequence_config"] = BareRegs.SequenceVhvEn | BareRegs.SequencePhasecalEn | BareRegs.SequenceDss1En
                                             | BareRegs.SequenceDss2En | extraSequence | BareRegs.SequenceRangeEn;

        Hpp.ValidPhaseLow = 0x08;
        Hpp.ValidPhaseHigh = validPhaseHigh;

        Img.SystemControl["system__mode_start"] = BareRegs.DeviceSchedulerModeHistogram | BareRegs.DeviceReadoutModeDualSd
                                                  | BareRegs.DeviceMeasurementModeBackToBack;
    }

    public void SetTimeoutsUs(long phasecalUs, long mmUs, long rangeUs)
    {
        PhasecalConfigTimeoutUs = phasecalUs;
        MmConfigTimeoutUs = mmUs;
        RangeConfigTimeoutUs = rangeUs;
        BareMath.CalcTimeoutRegisterValues(phasecalUs, mmUs, rangeUs, Img.StaticNvm["osc_measured__fast_osc__frequency"],
            Img.GeneralConfig, Img.TimingConfig);
    }

    public void SetInterMeasurementPeriodMs(long periodMs)
    {
        if (ResultOscCalibrateVal == 0)
            throw new DivideByZeroException("result__osc_calibrate_val is 0");
        InterMeasurementPeriodMs = periodMs;
        Img.TimingConfig["system__intermeasurement_period"] = periodMs * ResultOscCalibrateVal;
    }

    /// <summary>VL53LX_SetMeasurementTimingBudgetMicroSeconds(): the range timeout is one sixth of the budget
    /// after a fixed guard comes off.</summary>
    public void SetMeasurementTimingBudgetUs(long budgetUs)
    {
        if (!(BareRegs.TimingGuardUs < budgetUs && budgetUs <= 10000000))
            throw new ArgumentOutOfRangeException(nameof(budgetUs), $"timing budget {budgetUs} us out of range");
        long rangeUs = (budgetUs - BareRegs.TimingGuardUs) / BareRegs.TimingDivisor;
        if (rangeUs * BareRegs.TimingDivisor > FdaMaxTimingBudgetUs())
            throw new ArgumentOutOfRangeException(nameof(budgetUs), $"timing budget {budgetUs} us out of range");
        SetTimeoutsUs(PhasecalConfigTimeoutUs, MmConfigTimeoutUs, rangeUs);
    }

    /// <summary>VL53LX_load_patch(): a six-instruction phasecal patch, written with the firmware stopped.</summary>
    public void LoadPatch()
    {
        int power = Tuning["tp_phasecal_patch_power"] switch { 1 => 0x10, 2 => 0x20, 3 => 0x40, _ => 0x00 };
        _p.WriteByte(BareRegs.FirmwareEnable, 0x00);
        EnablePowerforce();
        _p.WriteMulti(BareRegs.PatchOffset0, new byte[] { 0x29, 0xC9, 0x0E, 0x40, 0x28, (byte)power });
        _p.WriteMulti(BareRegs.PatchAddress0, new byte[] { 0x03, 0x6D, 0x03, 0x6F, 0x07, 0x29 });
        _p.WriteMulti(BareRegs.PatchJmpEnables, new byte[] { 0x00, 0x07 });
        _p.WriteMulti(BareRegs.PatchDataEnables, new byte[] { 0x00, 0x07 });
        _p.WriteByte(BareRegs.PatchCtrl, 0x01);
        _p.WriteByte(BareRegs.FirmwareEnable, 0x01);
    }

    public void UnloadPatch()
    {
        _p.WriteByte(BareRegs.FirmwareEnable, 0x00);
        DisablePowerforce();
        _p.WriteByte(BareRegs.PatchCtrl, 0x00);
        _p.WriteByte(BareRegs.FirmwareEnable, 0x01);
    }

    private void EnablePowerforce()
    {
        _p.WriteByte(BareRegs.PowerManagementGo1PowerForce, 0x01);
        _p.SleepMs(1); // 250 us settling, rounded up
    }

    private void DisablePowerforce() => _p.WriteByte(BareRegs.PowerManagementGo1PowerForce, 0x00);

    /// <summary>VL53LX_init_and_start_range(): write the configuration and start. <paramref name="blocks"/> is
    /// ST's config level — all of them to start, general_config onwards to let the next frame run.</summary>
    public void InitAndStartRange(IReadOnlyList<string>? blocks = null)
    {
        var dynamic = Img.DynamicConfig;
        var system = Img.SystemControl;

        MeasurementMode = BareRegs.DeviceMeasurementModeBackToBack;
        system["system__mode_start"] = (system["system__mode_start"] & BareRegs.DeviceMeasurementModeStopMask) | MeasurementMode;
        system["system__interrupt_clear"] = BareRegs.ClearRangeInt;

        long gphId = State.CfgGphId;
        dynamic["system__grouped_parameter_hold_0"] = gphId | 0x01;
        dynamic["system__grouped_parameter_hold_1"] = gphId | 0x01;
        dynamic["system__grouped_parameter_hold"] = gphId;

        Img.PushRange(blocks ?? DeviceImage.RangeStartBlocks);

        State.UpdateRd(system["system__mode_start"], dynamic["system__grouped_parameter_hold"]);
        State.UpdateCfg(system["system__mode_start"]);
    }

    /// <summary>VL53LX_clear_interrupt_and_enable_next_range(): in histogram mode the interrupt is cleared by
    /// rewriting general_config onwards.</summary>
    public void ClearInterruptAndStartNextRange() =>
        InitAndStartRange(NextRangeBlocks);

    private static readonly string[] NextRangeBlocks = { "general_config", "timing_config", "dynamic_config", "system_control" };

    public void StopRange()
    {
        var system = Img.SystemControl;
        system["system__mode_start"] = (system["system__mode_start"] & BareRegs.DeviceMeasurementModeStopMask)
                                       | BareRegs.DeviceMeasurementModeAbort;
        Img.Push("system_control");
        system["system__mode_start"] &= BareRegs.DeviceMeasurementModeStopMask;
        State.Reset();
    }

    /// <summary>VL53LX_get_histogram_bin_data(): one frame, 83 bytes, decoded into bins plus the metadata the
    /// post-processing needs. <paramref name="raw"/> is bytes that arrived some other way (a stream report).</summary>
    public HistogramBinData GetHistogramBinData(byte[]? raw = null)
    {
        raw ??= _p.ReadMulti(BareRegs.HistogramBinDataI2cIndex, BareRegs.HistogramBinDataI2cSizeBytes);
        if (raw.Length < BareRegs.HistogramBinDataI2cSizeBytes)
            throw new ArgumentException($"histogram frame is {raw.Length} bytes, need {BareRegs.HistogramBinDataI2cSizeBytes}", nameof(raw));
        byte[] buf = raw.ToArray();
        const int off = BareRegs.HistogramBinDataI2cIndex;

        var d = new HistogramBinData
        {
            ResultInterruptStatus = buf[0],
            ResultRangeStatus = buf[1],
            ResultReportStatus = buf[2],
            ResultStreamCount = buf[3],
            ResultDssActualEffectiveSpads = (buf[4] << 8) | buf[5],
            PhasecalResultReferencePhase = (buf[BareRegs.PhasecalResultReferencePhase - off] << 8)
                                           | buf[BareRegs.PhasecalResultReferencePhase - off + 1],
            PhasecalResultVcselStart = buf[BareRegs.PhasecalResultVcselStart - off],
        };

        // Bin 23 does not fit in its three bytes: its low byte comes in a separate MSB/LSB pair.
        buf[BareRegs.ResultHistogramBin23_0 - off] = (byte)((buf[BareRegs.ResultHistogramBin23_0Msb - off] << 2)
                                                           + buf[BareRegs.ResultHistogramBin23_0Lsb - off]);
        int b0 = BareRegs.ResultHistogramBin0_2 - off;
        for (int i = 0; i < BareRegs.HistogramBufferSize; i++)
            d.BinData[i] = (buf[b0 + 3 * i] << 16) | (buf[b0 + 3 * i + 1] << 8) | buf[b0 + 3 * i + 2];

        var general = Img.GeneralConfig;
        var stat = Img.StaticConfig;
        var timing = Img.TimingConfig;

        d.CalConfigVcselStart = general["cal_config__vcsel_start"];
        d.VcselWidth = (general["global_config__vcsel_width"] << 4) + stat["ana_config__vcsel_pulse_width_offset"];
        d.FastOscFrequency = Img.StaticNvm["osc_measured__fast_osc__frequency"];
        d.RoiConfigUserRoiCentreSpad = Img.DynamicConfig["roi_config__user_roi_centre_spad"];
        d.RoiConfigUserRoiRequestedGlobalXySize = Img.DynamicConfig["roi_config__user_roi_requested_global_xy_size"];

        d.ZoneId = 0;
        d.BinSeq = HistCfg.BinSequence(State.RdStreamCount, _ambientEventsSum);

        long encoded;
        if (State.RdTimingStatus == 0)
        {
            encoded = (timing["range_config__timeout_macrop_a_hi"] << 8) + timing["range_config__timeout_macrop_a_lo"];
            d.VcselPeriod = timing["range_config__vcsel_period_a"];
        }
        else
        {
            encoded = (timing["range_config__timeout_macrop_b_hi"] << 8) + timing["range_config__timeout_macrop_b_lo"];
            d.VcselPeriod = timing["range_config__vcsel_period_b"];
        }

        d.NumberOfAmbientBins = 4 * d.BinSeq.Count(s => (s & 0x07) == 0x07);
        d.TotalPeriodsElapsed = BareMath.DecodeTimeout(encoded);
        d.PeakDurationUs = BareMath.DurationMaths(BareMath.CalcPllPeriodUs(d.FastOscFrequency), d.VcselWidth,
            BareRegs.RangingWindowVcselPeriods, d.TotalPeriodsElapsed + 1);
        d.WoiDurationUs = 0;

        // VL53LX_hist_calc_zero_distance_phase
        long period = 2048 * BareMath.DecodeVcselPeriod(d.VcselPeriod);
        long phase = period + d.PhasecalResultReferencePhase + 2048 * d.PhasecalResultVcselStart - 2048 * d.CalConfigVcselStart;
        d.ZeroDistancePhase = period != 0 ? Hist.PyMod(phase, period) : 0;

        // VL53LX_hist_estimate_ambient_from_ambient_bins
        if (d.NumberOfAmbientBins > 0)
        {
            d.NumberOfAmbientSamples = d.NumberOfAmbientBins;
            d.AmbientEventsSum = d.BinData.Take((int)d.NumberOfAmbientBins).Sum();
            d.AmbientPerBin = (d.AmbientEventsSum + d.NumberOfAmbientBins / 2) / d.NumberOfAmbientBins;
        }
        _ambientEventsSum = d.AmbientEventsSum;
        return d;
    }
}
