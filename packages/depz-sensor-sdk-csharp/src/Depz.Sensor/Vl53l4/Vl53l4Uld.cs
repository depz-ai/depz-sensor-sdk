using System.Buffers.Binary;

namespace Depz.Sensor.Vl53l4;

/// <summary>Raised on invalid VL53L4CD ULD inputs (short result block, out-of-range timing…).</summary>
public sealed class Vl53l4Exception : Exception
{
    public Vl53l4Exception(string message) : base(message) { }
}

/// <summary>
/// VL53L4CD_ResultsData_t plus the sensor's own frame counter.
/// <see cref="StreamCount"/> (RESULT__STREAM_COUNT, wraps at 255) tells a frame
/// the host never received from one the sensor never produced.
/// </summary>
public sealed record Vl53l4Results(
    int RangeStatus,
    int DistanceMm,
    int AmbientRateKcps,
    int AmbientPerSpadKcps,
    int SignalRateKcps,
    int SignalPerSpadKcps,
    int NumberOfSpad,
    int SigmaMm,
    int StreamCount);

/// <summary>
/// VL53L4CD ULD codec/math layer (ST STSW-IMG026 2.2.3, contract 10 §5): the
/// pure register-word codecs of <c>VL53L4CD_api.c</c> — result-block decode,
/// SetRangeTiming/GetRangeTiming register math, tuning-word codecs and the init
/// configuration block — ported byte-exact from the reference Python
/// <c>vl53l4/uld.py</c> and pinned to the golden vectors.
///
/// The live register-sequence driver (init/calibration over the CDC link) is
/// hardware-dependent and out of scope for the decode SDK, mirroring
/// <see cref="Vl53l8.Vl53l8Uld"/> (<see cref="LiveDriverStubbed"/>).
/// </summary>
public static class Vl53l4Uld
{
    // ── Registers (VL53L4CD_api.h) ──────────────────────────────────────────
    public const ushort SoftReset = 0x0000;
    public const ushort I2cSlaveDeviceAddress = 0x0001;

    /// <summary>Oscillator-frequency word (unnamed register 0x0006 in the C driver).</summary>
    public const ushort OscFrequency = 0x0006;

    public const ushort VhvConfigTimeoutMacropLoopBound = 0x0008;
    public const ushort XtalkPlaneOffsetKcps = 0x0016;
    public const ushort XtalkXPlaneGradientKcps = 0x0018;
    public const ushort XtalkYPlaneGradientKcps = 0x001A;
    public const ushort RangeOffsetMm = 0x001E;
    public const ushort InnerOffsetMm = 0x0020;
    public const ushort OuterOffsetMm = 0x0022;
    public const ushort GpioHvMuxCtrl = 0x0030;
    public const ushort GpioTioHvStatus = 0x0031;
    public const ushort SystemInterrupt = 0x0046;
    public const ushort RangeConfigA = 0x005E;
    public const ushort RangeConfigB = 0x0061;
    public const ushort RangeConfigSigmaThresh = 0x0064;
    public const ushort MinCountRateRtnLimitMcps = 0x0066;
    public const ushort IntermeasurementMs = 0x006C;
    public const ushort ThreshHigh = 0x0072;
    public const ushort ThreshLow = 0x0074;
    public const ushort SystemInterruptClear = 0x0086;
    public const ushort SystemStart = 0x0087;
    public const ushort ResultRangeStatus = 0x0089;
    public const ushort ResultSpadNb = 0x008C;
    public const ushort ResultSignalRate = 0x008E;
    public const ushort ResultAmbientRate = 0x0090;
    public const ushort ResultSigma = 0x0092;
    public const ushort ResultDistance = 0x0096;
    public const ushort ResultOscCalibrateVal = 0x00DE;
    public const ushort FirmwareSystemStatus = 0x00E5;
    public const ushort IdentificationModelId = 0x010F;

    /// <summary>IDENTIFICATION__MODEL_ID word expected for VL53L4CD silicon.</summary>
    public const ushort ModelId = 0xEBAA;

    // Detection-threshold window modes (SYSTEM__INTERRUPT).
    public const int WindowBelow = 0;
    public const int WindowAbove = 1;
    public const int WindowOut = 2;
    public const int WindowIn = 3;

    /// <summary>First register of the init configuration block (0x2D..0x87).</summary>
    public const ushort ConfigAddr = 0x002D;

    /// <summary>Last register of the init configuration block.</summary>
    public const ushort ConfigEnd = 0x0087;

    /// <summary>
    /// Byte 0 of <see cref="ConfigBlock"/> (register 0x2D): I2C pad in Fast
    /// Mode Plus — set unconditionally and never cleared (FM+ pads work at
    /// every bus step down to 100 kHz).
    /// </summary>
    public const byte ConfigFmpByte = 0x12;

    /// <summary>The block the MCU streams: RESULT__RANGE_STATUS .. 0x0099.</summary>
    public const ushort ResultBlockAddr = ResultRangeStatus;

    /// <summary>Length of the streamed result block (every VL53L4CD_ResultsData_t field).</summary>
    public const int ResultBlockLen = 17;

    /// <summary>The bridge boots at 400 kHz; init must run its configuration block there.</summary>
    public const int I2cKhzBoot = 400;

    /// <summary>Recommended post-init bus speed (the result-block read is ~4× faster).</summary>
    public const int I2cKhzDefault = 1000;

    /// <summary>
    /// VL53L4CD_DEFAULT_CONFIGURATION[] — 91 bytes, registers 0x2D..0x87.
    /// <see cref="ConfigBlock"/> always overrides byte 0 with
    /// <see cref="ConfigFmpByte"/>.
    /// </summary>
    public static ReadOnlySpan<byte> DefaultConfiguration => new byte[]
    {
        0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08,   // 0x2D..0x34
        0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,   // 0x35..0x3C
        0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00,   // 0x3D..0x44
        0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21,   // 0x45..0x4C
        0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8,   // 0x4D..0x54
        0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,   // 0x55..0x5C
        0x00, 0x01, 0xcc, 0x07, 0x01, 0xf1, 0x05, 0x00,   // 0x5D..0x64
        0xa0, 0x00, 0x80, 0x08, 0x38, 0x00, 0x00, 0x00,   // 0x65..0x6C
        0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00,   // 0x6D..0x74
        0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00,   // 0x75..0x7C
        0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00,   // 0x7D..0x84
        0x01, 0x00, 0x00,                                  // 0x85..0x87
    };

    /// <summary>GetResult() raw status → ULD status (status_rtn[24] in VL53L4CD_api.c; raw 9 → 0 valid).</summary>
    public static ReadOnlySpan<byte> StatusRtn => new byte[]
    {
        255, 255, 255, 5, 2, 4, 1, 7, 3,
        0, 255, 255, 9, 13, 255, 255, 255, 255, 10, 6,
        255, 255, 11, 12,
    };

    /// <summary>
    /// The live ULD register-sequence driver (sensor_init / calibration over
    /// the CDC link) is deliberately stubbed: it only means anything against
    /// real silicon and cannot be verified from golden vectors. The verifiable
    /// codec/math layer below is what this SDK ships.
    /// </summary>
    public static bool LiveDriverStubbed => true;

    /// <summary>
    /// The 91-byte block sensor_init() writes at <see cref="ConfigAddr"/>: the
    /// ST default configuration with byte 0 forced to <see cref="ConfigFmpByte"/>
    /// (Fast Mode Plus).
    /// </summary>
    public static byte[] ConfigBlock()
    {
        byte[] block = DefaultConfiguration.ToArray();
        block[0] = ConfigFmpByte;
        return block;
    }

    /// <summary>
    /// Decode the streamed 0x0089..0x0099 block exactly as VL53L4CD_GetResult()
    /// decodes the same registers read one by one. Register contents are
    /// big-endian words (the bridge passes them through untouched). Raw status
    /// ≥ 24 passes through unmapped.
    /// </summary>
    /// <exception cref="Vl53l4Exception">when <paramref name="raw"/> is shorter than 15 bytes.</exception>
    public static Vl53l4Results ParseResultBlock(ReadOnlySpan<byte> raw)
    {
        if (raw.Length < 15)
            throw new Vl53l4Exception($"result block too short: {raw.Length} bytes");

        int status = raw[0] & 0x1F;
        if (status < StatusRtn.Length)
            status = StatusRtn[status];

        int rawSpads = BinaryPrimitives.ReadUInt16BigEndian(raw[3..]);        // 0x008C
        int signalKcps = BinaryPrimitives.ReadUInt16BigEndian(raw[5..]) * 8;  // 0x008E
        int ambientKcps = BinaryPrimitives.ReadUInt16BigEndian(raw[7..]) * 8; // 0x0090

        return new Vl53l4Results(
            RangeStatus: status,
            DistanceMm: BinaryPrimitives.ReadUInt16BigEndian(raw[13..]),      // 0x0096
            AmbientRateKcps: ambientKcps,
            AmbientPerSpadKcps: rawSpads == 0 ? 0 : ambientKcps * 256 / rawSpads,
            SignalRateKcps: signalKcps,
            SignalPerSpadKcps: rawSpads == 0 ? 0 : signalKcps * 256 / rawSpads,
            NumberOfSpad: rawSpads / 256,
            SigmaMm: BinaryPrimitives.ReadUInt16BigEndian(raw[9..]) / 4,      // 0x0092
            StreamCount: raw[2]);
    }

    /// <summary>
    /// SetRangeTiming register math → (RANGE_CONFIG_A, RANGE_CONFIG_B,
    /// INTERMEASUREMENT_MS raw dword). <paramref name="oscFrequency"/> is the
    /// word read from 0x0006; <paramref name="clockPll"/> is the word read from
    /// RESULT__OSC_CALIBRATE_VAL (used only in autonomous mode, i.e. when
    /// <paramref name="interMeasurementMs"/> &gt; 0). Bit-exact port of the C
    /// ULD, 32-bit truncations and the 1.055 PLL factor included.
    /// </summary>
    /// <exception cref="Vl53l4Exception">osc reads 0, budget outside 10..200 ms,
    /// or inter-measurement neither 0 nor &gt; budget.</exception>
    public static (ushort RangeConfigA, ushort RangeConfigB, uint IntermeasurementRaw) RangeTimingRegisters(
        int timingBudgetMs, int interMeasurementMs, int oscFrequency, int clockPll = 0)
    {
        if (oscFrequency == 0)
            throw new Vl53l4Exception("osc_frequency reads 0");
        if (timingBudgetMs < 10 || timingBudgetMs > 200)
            throw new Vl53l4Exception("timing_budget_ms must be 10..200");

        ulong timingBudgetUs = (ulong)timingBudgetMs * 1000;
        ulong macroPeriodUs = ((2304ul * (0x40000000ul / (uint)oscFrequency)) & 0xFFFFFFFFul) >> 6;

        uint intermeasurementRaw;
        if (interMeasurementMs == 0)
        {
            // Continuous mode.
            intermeasurementRaw = 0;
            timingBudgetUs -= 2500;
        }
        else if (interMeasurementMs > timingBudgetMs)
        {
            // Autonomous low power: PLL factor in double, truncated toward zero.
            double factor = 1.055 * interMeasurementMs * (clockPll & 0x3FF);
            intermeasurementRaw = (uint)(long)factor;
            timingBudgetUs = (timingBudgetUs - 4300) / 2;
        }
        else
        {
            throw new Vl53l4Exception("inter_measurement_ms must be 0 or > timing_budget_ms");
        }

        timingBudgetUs = (timingBudgetUs << 12) & 0xFFFFFFFFul;
        Span<ushort> words = stackalloc ushort[2];
        ReadOnlySpan<int> mults = stackalloc int[] { 16, 12 }; // RANGE_CONFIG_A, RANGE_CONFIG_B
        for (int i = 0; i < mults.Length; i++)
        {
            ulong tmp = ((macroPeriodUs * (ulong)mults[i]) & 0xFFFFFFFFul) >> 6;
            ulong lsByte = ((timingBudgetUs + (tmp >> 1)) / tmp) - 1;
            int msByte = 0;
            while ((lsByte & 0xFFFFFF00ul) != 0)
            {
                lsByte >>= 1;
                msByte++;
            }
            words[i] = (ushort)(((((ulong)msByte << 8) + (lsByte & 0xFF)) & 0xFFFF));
        }
        return (words[0], words[1], intermeasurementRaw);
    }

    /// <summary>
    /// GetRangeTiming register math → (timing_budget_ms, inter_measurement_ms).
    /// Inputs are the raw register reads: the INTERMEASUREMENT_MS dword, the
    /// RESULT__OSC_CALIBRATE_VAL word, the 0x0006 word and the RANGE_CONFIG_A
    /// word. Bit-exact port (1.065 PLL factor, 32-bit wrap of the ms-byte).
    /// </summary>
    /// <exception cref="Vl53l4Exception">when <paramref name="oscFrequency"/> reads 0.</exception>
    public static (int TimingBudgetMs, int InterMeasurementMs) DecodeRangeTiming(
        uint intermeasurementRaw, int clockPll, int oscFrequency, int rangeConfigA)
    {
        if (oscFrequency == 0)
            throw new Vl53l4Exception("osc_frequency reads 0");

        int pll = (int)((long)(1.065 * (clockPll & 0x3FF)) & 0xFFFF);
        int interMeasurementMs = pll == 0 ? 0 : (int)((intermeasurementRaw / (uint)pll) & 0xFFFF);

        ulong macroPeriodUs = ((2304ul * (0x40000000ul / (uint)oscFrequency)) & 0xFFFFFFFFul) >> 6;
        ulong lsByte = (ulong)((rangeConfigA & 0x00FF) << 4);
        long msByte = (rangeConfigA & 0xFF00) >> 8;
        msByte = (0x04 - (msByte - 1) - 1) & 0xFFFFFFFFL; // wraps negative in 32 bits
        macroPeriodUs = (macroPeriodUs * 16) & 0xFFFFFFFFul;

        ulong budget = ((((lsByte + 1) * (macroPeriodUs >> 6))
                         - ((macroPeriodUs >> 6) >> 1)) & 0xFFFFFFFFul) >> 12;
        if (msByte < 12)
            budget >>= (int)msByte;
        budget = intermeasurementRaw == 0 ? budget + 2500 : budget * 2 + 4300;
        return ((int)(budget / 1000), interMeasurementMs);
    }

    // ── Threshold / offset / xtalk raw codecs (register word ↔ user units) ──

    /// <summary>RANGE_OFFSET_MM word for SetOffset (INNER/OUTER are zeroed alongside).</summary>
    public static ushort OffsetRaw(int offsetMm) => (ushort)((offsetMm * 4) & 0xFFFF);

    /// <summary>GetOffset: RANGE_OFFSET_MM word → signed millimetres.</summary>
    public static int DecodeOffset(ushort raw)
    {
        int temp = ((raw << 3) & 0xFFFF) >> 5;
        return temp > 1024 ? temp - 2048 : temp;
    }

    /// <summary>XTALK_PLANE_OFFSET_KCPS word for SetXtalk.</summary>
    public static ushort XtalkRaw(int xtalkKcps) => (ushort)((xtalkKcps << 9) & 0xFFFF);

    /// <summary>GetXtalk: XTALK_PLANE_OFFSET_KCPS word → kcps.</summary>
    public static int DecodeXtalk(ushort raw) =>
        (int)Math.Round(raw / 512.0, MidpointRounding.AwayFromZero);

    /// <summary>MIN_COUNT_RATE_RTN_LIMIT_MCPS word for SetSignalThreshold.</summary>
    public static int SignalThresholdRaw(int signalKcps) => signalKcps >> 3;

    /// <summary>GetSignalThreshold: MIN_COUNT_RATE_RTN_LIMIT_MCPS word → kcps.</summary>
    public static int DecodeSignalThreshold(int raw) => (raw << 3) & 0xFFFF;

    /// <summary>RANGE_CONFIG__SIGMA_THRESH word for SetSigmaThreshold.</summary>
    /// <exception cref="Vl53l4Exception">when <paramref name="sigmaMm"/> &gt; 16383.</exception>
    public static int SigmaThresholdRaw(int sigmaMm)
    {
        if (sigmaMm > 0xFFFF >> 2)
            throw new Vl53l4Exception("sigma_mm must be <= 16383");
        return sigmaMm << 2;
    }

    /// <summary>GetSigmaThreshold: RANGE_CONFIG__SIGMA_THRESH word → mm.</summary>
    public static int DecodeSigmaThreshold(int raw) => raw >> 2;
}
