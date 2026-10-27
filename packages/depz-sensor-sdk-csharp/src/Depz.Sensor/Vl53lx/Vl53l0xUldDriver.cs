using System.Diagnostics;

namespace Depz.Sensor.Vl53lx;

/// <summary>
/// The VL53L0X API 1.0.4 ranging path (vl53l0x_api.c / _api_core.c / _api_calibration.c), ported from
/// the reference Python driver: DataInit, StaticInit (reference SPADs from NVM, tuning settings), the
/// timing-budget arithmetic, start / stop, GetRangingMeasurementData with its PAL range status (sigma
/// estimate included), the reference calibration, PerformRefSpadManagement, offset and crosstalk
/// calibration and the four ST ranging profiles. Register sequences, integer widths and the deliberate
/// uint32 overflows are the C driver's. There is no ROI on this die.
/// </summary>
public sealed class Vl53l0xUldDriver : Vl53lxDriver
{
    // ── registers (vl53l0x_device.h) ─────────────────────────────────────────
    private const int SysrangeStart = 0x00;
    private const int SysrangeModeSingleshot = 0x00;
    private const int SysrangeModeStartStop = 0x01;
    private const int SysrangeModeBacktoback = 0x02;
    private const int SysrangeModeTimed = 0x04;
    private const int SystemSequenceConfig = 0x01;
    private const int SystemIntermeasurementPeriod = 0x04;
    private const int SystemRangeConfig = 0x09;
    private const int SystemInterruptConfigGpio = 0x0A;
    private const int SystemInterruptClear = 0x0B;
    private const int ResultInterruptStatus = 0x13;
    private const int ResultRangeStatus = 0x14;
    private const int CrosstalkCompensationPeakRateMcps = 0x20;
    private const int AlgoPartToPartRangeOffsetMm = 0x28;
    private const int AlgoPhasecalConfigTimeout = 0x30;
    private const int GlobalConfigVcselWidth = 0x32;
    private const int FinalRangeConfigMinCountRateRtnLimit = 0x44;
    private const int MsrcConfigTimeoutMacrop = 0x46;
    private const int FinalRangeConfigValidPhaseLow = 0x47;
    private const int FinalRangeConfigValidPhaseHigh = 0x48;
    private const int DynamicSpadNumRequestedRefSpad = 0x4E;
    private const int DynamicSpadRefEnStartOffset = 0x4F;
    private const int PreRangeConfigVcselPeriod = 0x50;
    private const int PreRangeConfigTimeoutMacropHi = 0x51;
    private const int PreRangeConfigValidPhaseLow = 0x56;
    private const int PreRangeConfigValidPhaseHigh = 0x57;
    private const int MsrcConfigControl = 0x60;
    private const int PreRangeMinCountRateRtnLimit = 0x64;
    private const int FinalRangeConfigVcselPeriod = 0x70;
    private const int FinalRangeConfigTimeoutMacropHi = 0x71;
    private const int PowerManagementGo1PowerForce = 0x80;
    private const int GpioHvMuxActiveHigh = 0x84;
    private const int SoftResetGo2SoftResetN = 0xBF;
    private const int GlobalConfigSpadEnablesRef0 = 0xB0;
    private const int GlobalConfigRefEnStartSelect = 0xB6;
    private const int IdentificationModelId = 0xC0;
    private const int OscCalibrateVal = 0xF8;
    // Page 1 (0xFF ← 0x01 first): they share their address with a page-0 register.
    private const int AlgoPhasecalLim = 0x30;
    private const int ResultPeakSignalRateRef = 0xB6;

    public const int ModelIdVl53l0x = 0xEE;

    private const int DeviceModeSingleRanging = 0, DeviceModeContinuousRanging = 1, DeviceModeContinuousTimed = 3;
    private const int GpioFunctionalityNewMeasureReady = 4;

    private const int SeqTcc = 0, SeqDss = 1, SeqMsrc = 2, SeqPreRange = 3, SeqFinalRange = 4;
    private static readonly int[] SeqSet = { 0x10, 0x28, 0x04, 0x40, 0x80 };
    private static readonly int[] SeqClear = { 0xEF, 0xD7, 0xFB, 0xBF, 0x7F };
    private static readonly int[] SeqTest = { 0x10, 0x08, 0x04, 0x40, 0x80 };

    private const int VcselPreRange = 0, VcselFinalRange = 1;

    private const int CheckSigmaFinalRange = 0, CheckSignalRateFinalRange = 1, CheckSignalRefClip = 2;
    private const int CheckRangeIgnoreThreshold = 3, CheckSignalRateMsrc = 4, CheckSignalRatePreRange = 5;
    private const int CheckNumberOfChecks = 6;

    private const int DefaultMaxLoop = 2000;
    private const int TargetRefRateDefault = 0x0A00;
    private const long MinTimingBudgetUs = 20000; // cMinTimingBudgetMicroSeconds
    // The host-side parameters a tuning buffer may carry in a {0xFF, select, msb, lsb}
    // record, by select value (VL53L0X_load_tuning_settings()).
    private const int TuningParamCount = 4;
    private const long SpeedOfLightInAir = 2997;
    private const int RefSpadBufferSize = 6;

    // The four ST examples: (signal Mcps, sigma mm, budget µs, pre-range PCLK, final PCLK).
    private static readonly (string Name, double Signal, int Sigma, int BudgetUs, int Pre, int Final)[] ModeSettings =
    {
        ("default", 0.25, 18, 33000, 14, 10),
        ("long-range", 0.10, 60, 33000, 18, 14),
        ("high-speed", 0.25, 32, 30000, 14, 10),
        ("high-accuracy", 0.25, 18, 200000, 14, 10),
    };

    // ST's range for each final-range VCSEL period the profiles use.
    private static readonly Dictionary<int, int> ReachByFinalPclksMm = new() { [10] = 1200, [14] = 2000 };

    // DefaultTuningSettings[] (vl53l0x_tuning.h, v36): {count, address, bytes…} records, 0 ends.
    private static readonly byte[] DefaultTuningSettings =
    {
        0x01, 0xFF, 0x01, 0x01, 0x00, 0x00,
        0x01, 0xFF, 0x00, 0x01, 0x09, 0x00, 0x01, 0x10, 0x00, 0x01, 0x11, 0x00,
        0x01, 0x24, 0x01, 0x01, 0x25, 0xff, 0x01, 0x75, 0x00,
        0x01, 0xFF, 0x01, 0x01, 0x4e, 0x2c, 0x01, 0x48, 0x00, 0x01, 0x30, 0x20,
        0x01, 0xFF, 0x00, 0x01, 0x30, 0x09, 0x01, 0x54, 0x00, 0x01, 0x31, 0x04, 0x01, 0x32, 0x03,
        0x01, 0x40, 0x83, 0x01, 0x46, 0x25, 0x01, 0x60, 0x00, 0x01, 0x27, 0x00, 0x01, 0x50, 0x06,
        0x01, 0x51, 0x00, 0x01, 0x52, 0x96, 0x01, 0x56, 0x08, 0x01, 0x57, 0x30, 0x01, 0x61, 0x00,
        0x01, 0x62, 0x00, 0x01, 0x64, 0x00, 0x01, 0x65, 0x00, 0x01, 0x66, 0xa0,
        0x01, 0xFF, 0x01, 0x01, 0x22, 0x32, 0x01, 0x47, 0x14, 0x01, 0x49, 0xff, 0x01, 0x4a, 0x00,
        0x01, 0xFF, 0x00, 0x01, 0x7a, 0x0a, 0x01, 0x7b, 0x00, 0x01, 0x78, 0x21,
        0x01, 0xFF, 0x01, 0x01, 0x23, 0x34, 0x01, 0x42, 0x00, 0x01, 0x44, 0xff, 0x01, 0x45, 0x26,
        0x01, 0x46, 0x05, 0x01, 0x40, 0x40, 0x01, 0x0E, 0x06, 0x01, 0x20, 0x1a, 0x01, 0x43, 0x40,
        0x01, 0xFF, 0x00, 0x01, 0x34, 0x03, 0x01, 0x35, 0x44,
        0x01, 0xFF, 0x01, 0x01, 0x31, 0x04, 0x01, 0x4b, 0x09, 0x01, 0x4c, 0x05, 0x01, 0x4d, 0x04,
        0x01, 0xFF, 0x00, 0x01, 0x44, 0x00, 0x01, 0x45, 0x20, 0x01, 0x47, 0x08, 0x01, 0x48, 0x28,
        0x01, 0x67, 0x00, 0x01, 0x70, 0x04, 0x01, 0x71, 0x01, 0x01, 0x72, 0xfe, 0x01, 0x76, 0x00,
        0x01, 0x77, 0x00,
        0x01, 0xFF, 0x01, 0x01, 0x0d, 0x01,
        0x01, 0xFF, 0x00, 0x01, 0x80, 0x01, 0x01, 0x01, 0xF8,
        0x01, 0xFF, 0x01, 0x01, 0x8e, 0x01, 0x01, 0x00, 0x01, 0x01, 0xFF, 0x00, 0x01, 0x80, 0x00,
        0x00, 0x00, 0x00,
    };

    // Dmax lookup table (FixPoint16.16).
    private static readonly long[] DmaxLutAmbRateMcps = { 0x00000000, 0x0000B333, 0x00020000, 0x0003CCCC, 0x00074CCC, 0x000A0000, 0x000F0000 };
    private static readonly long[] DmaxLutDmaxMm = { 0x04B00000, 0x044C0000, 0x03840000, 0x02EE0000, 0x02260000, 0x01F40000, 0x01900000 };

    private const int ResultBlockAddr = ResultRangeStatus;
    private const int ResultBlockLen = 12;

    private static readonly Vl53lxClearStep[] L0xClearSteps = { new(SystemInterruptClear, 0x01), new(SystemInterruptClear, 0x00) };

    private static readonly Dictionary<int, string> RangeStatusNames = new()
    {
        [0] = "Range Valid", [1] = "Sigma Fail", [2] = "Signal Fail", [3] = "Min Range Fail",
        [4] = "Phase Fail", [5] = "Hardware Fail", [255] = "No Update",
    };

    private static readonly int[] RefArrayQuadrants = { 10, 5, 0, 5 };

    private static readonly string[] Groups = { "mode", "timing", "offset", "xtalk", "calib_offset", "calib_xtalk", "refspad" };

    // PALDevData / DeviceSpecificParameters: the state the C driver keeps beside the sensor.
    private int _readDataFromDeviceDone;
    private long _xtalkCompensationRateMegaCps;
    private int _xtalkCompensationEnable;
    private int _stopVariable;
    private int _sequenceConfig;
    private int _rangeFractionalEnable;
    private int _deviceMode = DeviceModeSingleRanging;
    private int? _continuousMode; // not in the C driver: resumed after a single shot
    private long _measurementTimingBudgetUs;
    private int _pin0GpioFunctionality = GpioFunctionalityNewMeasureReady;
    private int _referenceSpadCount;
    private int _referenceSpadType;
    private int[] _refGoodSpadMap = new int[RefSpadBufferSize];
    private int[] _refSpadEnables = new int[RefSpadBufferSize];
    private int _preRangeVcselPulsePeriod;
    private int _finalRangeVcselPulsePeriod;
    private long _preRangeTimeoutUs;
    private long _finalRangeTimeoutUs;
    private int _targetRefRate = TargetRefRateDefault;
    // Stored for the tuning loader; the 1.0.4 sigma estimate has these three as constants.
    private int _sigmaEstRefArray = 100, _sigmaEstEffPulseWidth = 900, _sigmaEstEffAmbWidth = 500;
    private readonly int[] _limitChecksEnable = new int[CheckNumberOfChecks];
    private readonly long[] _limitChecksValue = new long[CheckNumberOfChecks];
    private string _mode = "default";

    public Vl53l0xUldDriver(Vl53lxPlatform platform, string product) : base(platform, product) { }

    public override int AddrWidth => 1; // the only part of the family with 8-bit register addresses
    public override IReadOnlyList<Vl53lxClearStep> ClearSteps => L0xClearSteps;
    public override int MaxKhz => 400; // no Fast Mode Plus on this die
    public override IReadOnlyCollection<string> Supports => Groups;
    public override IReadOnlyList<string> Modes => ModeSettings.Select(m => m.Name).ToList();
    public override (int Min, int Max) BudgetMs => (20, 200);

    /// <summary>False when the NVM reference-SPAD record was out of range and was measured instead.</summary>
    public bool RefSpadsFromNvm { get; private set; } = true;

    public int ModuleId { get; private set; }
    public int Revision { get; private set; }
    public string ProductIdString { get; private set; } = "";
    public long PartUidUpper { get; private set; }
    public long PartUidLower { get; private set; }
    public long SignalRateMeasFixed400mm { get; private set; }
    public long Part2PartOffsetAdjustmentNvmUm { get; private set; }
    /// <summary>The reference-SPAD target rate (9.7 Mcps) a tuning buffer's {0xFF, 3, msb, lsb} record sets;
    /// the SPAD management converges on it.</summary>
    public int TargetRefRate => _targetRefRate;
    /// <summary>The three sigma-estimate constants a tuning buffer's {0xFF, 0..2, msb, lsb} records set
    /// (stored only: the 1.0.4 sigma estimate has them as constants).</summary>
    public int SigmaEstRefArray => _sigmaEstRefArray;
    /// <inheritdoc cref="SigmaEstRefArray"/>
    public int SigmaEstEffPulseWidth => _sigmaEstEffPulseWidth;
    /// <inheritdoc cref="SigmaEstRefArray"/>
    public int SigmaEstEffAmbWidth => _sigmaEstEffAmbWidth;
    public long OscFrequencyMHz { get; private set; } = 618660;

    // ── helpers of the C driver ──────────────────────────────────────────────

    private static long U32(long v) => v & 0xFFFFFFFFL;

    /// <summary>C int16 truncation (an assignment to an int16_t).</summary>
    private static long I16(long v)
    {
        long r = v & 0xFFFF;
        return (r & 0x8000) != 0 ? r - 0x10000 : r;
    }

    /// <summary>VL53L0X_isqrt(): the bit-by-bit integer square root.</summary>
    public static long Isqrt(long num)
    {
        long res = 0;
        long bit = 1L << 30;
        while (bit > num)
            bit >>= 2;
        while (bit != 0)
        {
            if (num >= res + bit)
            {
                num -= res + bit;
                res = (res >> 1) + bit;
            }
            else
            {
                res >>= 1;
            }
            bit >>= 2;
        }
        return res;
    }

    private static int DecodeVcselPeriod(int reg) => (reg + 1) << 1;

    private static int EncodeVcselPeriod(int pclks) => (pclks >> 1) - 1;

    private static int EncodeTimeout(long mclks)
    {
        if (mclks <= 0)
            return 0;
        long ls = mclks - 1;
        int ms = 0;
        while ((ls & 0xFFFFFF00L) != 0)
        {
            ls >>= 1;
            ms++;
        }
        return (int)(((ms << 8) + (ls & 0xFF)) & 0xFFFF);
    }

    private static long DecodeTimeout(int encoded) => ((long)(encoded & 0x00FF) << ((encoded & 0xFF00) >> 8)) + 1;

    private static long MacroPeriodPs(int pclks) => 2304L * pclks * 1655;

    private static long TimeoutMclks(long timeoutUs, int pclks)
    {
        long macroPeriodNs = (MacroPeriodPs(pclks) + 500) / 1000;
        return (timeoutUs * 1000 + macroPeriodNs / 2) / macroPeriodNs;
    }

    private static long TimeoutUs(long mclks, int pclks)
    {
        long macroPeriodNs = (MacroPeriodPs(pclks) + 500) / 1000;
        return (mclks * macroPeriodNs + 500) / 1000;
    }

    private static bool IsAperture(int spadIndex) => RefArrayQuadrants[spadIndex >> 6] != 0;

    private static int NextGoodSpad(int[] good, int size, int curr)
    {
        int startIndex = curr / 8;
        int fineOffset = curr % 8;
        for (int coarse = startIndex; coarse < size; coarse++)
        {
            int dataByte = good[coarse];
            int fine = 0;
            if (coarse == startIndex)
            {
                dataByte >>= fineOffset;
                fine = fineOffset;
            }
            while (fine < 8)
            {
                if ((dataByte & 1) != 0)
                    return coarse * 8 + fine;
                dataByte >>= 1;
                fine++;
            }
        }
        return -1;
    }

    // ── contract ─────────────────────────────────────────────────────────────

    public override (int Addr, int Len) StreamBlock() => (ResultBlockAddr, ResultBlockLen);

    public override Vl53lxRange Decode(byte[] raw) => AsRange(ParseResultBlock(raw));

    public override Vl53lxRange ReadMeasurement() => AsRange(GetResult());

    public override int ModelId() => P.ReadByte(IdentificationModelId);

    /// <summary>No firmware-status register: the model id answering says the die is up.</summary>
    public void WaitBoot(TimeSpan? timeout = null)
    {
        var limit = timeout ?? TimeSpan.FromSeconds(1);
        var clock = Stopwatch.StartNew();
        while (true)
        {
            try
            {
                if (P.ReadByte(IdentificationModelId) == ModelIdVl53l0x)
                    return;
            }
            catch (Vl53lxProtocolException)
            {
                // NACK while the die is still booting
            }
            if (clock.Elapsed > limit)
                throw new Vl53lxException("timeout waiting for MODEL_ID 0xEE at 0xC0");
            P.SleepMs(1);
        }
    }

    /// <summary>DataInit + StaticInit, the reference calibration, continuous mode; the bus stays at 400 kHz.</summary>
    public override void SensorInit()
    {
        P.SetAddrWidth(AddrWidth);
        P.SetI2cSpeed(400);
        WaitBoot();
        ResetDevice();
        DataInit();
        StaticInit();
        if (!RefSpadsFromNvm)
            RefSpadManagement();
        PerformRefCalibration();
        SetDeviceMode(DeviceModeContinuousRanging);
        _mode = "default";
    }

    /// <summary>VL53L0X_ResetDevice(): a sensor left configured by a previous session ranges nothing without it.</summary>
    public void ResetDevice(TimeSpan? timeout = null)
    {
        var limit = timeout ?? TimeSpan.FromSeconds(1);
        var clock = Stopwatch.StartNew();
        // While the die reboots it NACKs its own address: "not booted yet", not a fault.
        void PollModelId(bool wantZero)
        {
            while (true)
            {
                int? b;
                try
                {
                    b = P.ReadByte(IdentificationModelId);
                }
                catch (Vl53lxProtocolException)
                {
                    b = null;
                }
                if (b is int v && (v == 0x00) == wantZero)
                    return;
                if (clock.Elapsed > limit)
                    throw new Vl53lxException("timeout in the soft reset of the sensor");
            }
        }
        P.WriteByte(SoftResetGo2SoftResetN, 0x00);
        PollModelId(true);
        P.SleepMs(1);
        P.WriteByte(SoftResetGo2SoftResetN, 0x01);
        PollModelId(false);
        P.SleepMs(1);
    }

    private void DataInit()
    {
        P.WriteByte(0x88, 0x00); // I2C standard mode
        _readDataFromDeviceDone = 0;
        OscFrequencyMHz = 618660;
        _xtalkCompensationRateMegaCps = 0;
        _xtalkCompensationEnable = 0;
        _deviceMode = DeviceModeSingleRanging;

        P.WriteByte(0x80, 0x01);
        P.WriteByte(0xFF, 0x01);
        P.WriteByte(0x00, 0x00);
        _stopVariable = P.ReadByte(0x91);
        P.WriteByte(0x00, 0x01);
        P.WriteByte(0xFF, 0x00);
        P.WriteByte(0x80, 0x00);

        for (int check = 0; check < CheckNumberOfChecks; check++)
            SetLimitCheckEnable(check, 1);
        SetLimitCheckEnable(CheckSignalRefClip, 0);
        SetLimitCheckEnable(CheckRangeIgnoreThreshold, 0);
        SetLimitCheckEnable(CheckSignalRateMsrc, 0);
        SetLimitCheckEnable(CheckSignalRatePreRange, 0);

        SetLimitCheckValue(CheckSigmaFinalRange, 18 * 65536L);
        SetLimitCheckValue(CheckSignalRateFinalRange, 25 * 65536L / 100);
        SetLimitCheckValue(CheckSignalRefClip, 35 * 65536L);
        SetLimitCheckValue(CheckRangeIgnoreThreshold, 0);

        _sequenceConfig = 0xFF;
        P.WriteByte(SystemSequenceConfig, 0xFF);
    }

    private void StaticInit()
    {
        GetInfoFromDevice(1);
        int count = _referenceSpadCount;
        int aperture = _referenceSpadType;
        RefSpadsFromNvm = !(aperture > 1 || (aperture == 1 && count > 32) || (aperture == 0 && count > 12));
        if (RefSpadsFromNvm)
            SetReferenceSpads(count, aperture);

        LoadTuningSettings(DefaultTuningSettings);
        SetGpioConfig(GpioFunctionalityNewMeasureReady, polarityLow: true);

        P.WriteByte(0xFF, 0x01);
        int tempword = P.ReadWord(0x84);
        P.WriteByte(0xFF, 0x00);
        OscFrequencyMHz = (long)tempword << 4; // FixPoint4.12 → 16.16

        _measurementTimingBudgetUs = GetMeasurementTimingBudget();
        _rangeFractionalEnable = P.ReadByte(SystemRangeConfig) & 1;
        _sequenceConfig = P.ReadByte(SystemSequenceConfig);

        SetSequenceStepEnable(SeqTcc, 0);
        SetSequenceStepEnable(SeqMsrc, 0);

        _preRangeVcselPulsePeriod = GetVcselPulsePeriod(VcselPreRange);
        _finalRangeVcselPulsePeriod = GetVcselPulsePeriod(VcselFinalRange);
        _preRangeTimeoutUs = GetSequenceStepTimeout(SeqPreRange);
        _finalRangeTimeoutUs = GetSequenceStepTimeout(SeqFinalRange);
    }

    // ── reference calibration (VHV / phase) ──────────────────────────────────

    private void PerformSingleRefCalibration(int vhvInitByte)
    {
        P.WriteByte(SysrangeStart, SysrangeModeStartStop | vhvInitByte);
        WaitDataReady(TimeSpan.FromSeconds(1));
        ClearInterrupt();
        P.WriteByte(SysrangeStart, 0x00);
    }

    private (int Vhv, int Phase) RefCalibrationIoRead(bool vhvEnable, bool phaseEnable)
    {
        P.WriteByte(0xFF, 0x01);
        P.WriteByte(0x00, 0x00);
        P.WriteByte(0xFF, 0x00);
        int vhv = vhvEnable ? P.ReadByte(0xCB) : 0;
        int phase = phaseEnable ? P.ReadByte(0xEE) : 0;
        P.WriteByte(0xFF, 0x01);
        P.WriteByte(0x00, 0x01);
        P.WriteByte(0xFF, 0x00);
        return (vhv, phase & 0xEF);
    }

    /// <summary>VL53L0X_PerformRefCalibration(): VHV, then phase → (vhv, phase). Not optional in practice.</summary>
    public (int Vhv, int Phase) PerformRefCalibration()
    {
        int sequenceConfig = _sequenceConfig;
        P.WriteByte(SystemSequenceConfig, 0x01); // VHV only
        PerformSingleRefCalibration(0x40);
        var (vhv, _) = RefCalibrationIoRead(true, false);
        P.WriteByte(SystemSequenceConfig, 0x02); // phase only
        PerformSingleRefCalibration(0x00);
        var (_, phase) = RefCalibrationIoRead(false, true);
        P.WriteByte(SystemSequenceConfig, sequenceConfig);
        _sequenceConfig = sequenceConfig;
        return (vhv, phase);
    }

    // ── NVM ──────────────────────────────────────────────────────────────────

    private void DeviceReadStrobe()
    {
        P.WriteByte(0x83, 0x00);
        bool done = false;
        for (int i = 0; i < DefaultMaxLoop; i++)
        {
            if (P.ReadByte(0x83) != 0x00)
            {
                done = true;
                break;
            }
        }
        if (!done)
            throw new Vl53lxException("timeout waiting for the NVM read strobe");
        P.WriteByte(0x83, 0x01);
    }

    private long NvmDword(int index)
    {
        P.WriteByte(0x94, index);
        DeviceReadStrobe();
        return P.ReadDword(0x90);
    }

    /// <summary>VL53L0X_get_info_from_device(): the reference SPAD record (option 1), the product
    /// identification (2), the part UID and factory offset (4).</summary>
    public void GetInfoFromDevice(int option)
    {
        int done = _readDataFromDeviceDone;
        if (done == 7)
            return;
        P.WriteByte(0x80, 0x01);
        P.WriteByte(0xFF, 0x01);
        P.WriteByte(0x00, 0x00);
        P.WriteByte(0xFF, 0x06);
        P.WriteByte(0x83, P.ReadByte(0x83) | 4);
        P.WriteByte(0xFF, 0x07);
        P.WriteByte(0x81, 0x01);
        P.SleepMs(1); // VL53L0X_PollingDelay()
        P.WriteByte(0x80, 0x01);

        var goodSpadMap = _refGoodSpadMap.ToArray();
        int refSpadCount = 0, refSpadType = 0;
        string productId = "";
        int moduleId = 0, revision = 0;
        long uidUpper = 0, uidLower = 0, signalRate400 = 0, dist400 = 0;

        if ((option & 1) == 1 && (done & 1) == 0)
        {
            long tmp = NvmDword(0x6B);
            refSpadCount = (int)(tmp >> 8 & 0x7F);
            refSpadType = (int)(tmp >> 15 & 0x01);
            tmp = NvmDword(0x24);
            goodSpadMap[0] = (int)(tmp >> 24 & 0xFF);
            goodSpadMap[1] = (int)(tmp >> 16 & 0xFF);
            goodSpadMap[2] = (int)(tmp >> 8 & 0xFF);
            goodSpadMap[3] = (int)(tmp & 0xFF);
            tmp = NvmDword(0x25);
            goodSpadMap[4] = (int)(tmp >> 24 & 0xFF);
            goodSpadMap[5] = (int)(tmp >> 16 & 0xFF);
        }

        if ((option & 2) == 2 && (done & 2) == 0)
        {
            P.WriteByte(0x94, 0x02);
            DeviceReadStrobe();
            moduleId = P.ReadByte(0x90);
            P.WriteByte(0x94, 0x7B);
            DeviceReadStrobe();
            revision = P.ReadByte(0x90);

            var chars = new List<long>();
            long tmp = NvmDword(0x77);
            chars.AddRange(new[] { tmp >> 25 & 0x7F, tmp >> 18 & 0x7F, tmp >> 11 & 0x7F, tmp >> 4 & 0x7F });
            long b = (tmp & 0x00F) << 3;
            tmp = NvmDword(0x78);
            chars.AddRange(new[] { b + (tmp >> 29 & 0x7F), tmp >> 22 & 0x7F, tmp >> 15 & 0x7F, tmp >> 8 & 0x7F, tmp >> 1 & 0x7F });
            b = (tmp & 0x001) << 6;
            tmp = NvmDword(0x79);
            chars.AddRange(new[] { b + (tmp >> 26 & 0x7F), tmp >> 19 & 0x7F, tmp >> 12 & 0x7F, tmp >> 5 & 0x7F });
            b = (tmp & 0x01F) << 2;
            tmp = NvmDword(0x7A);
            chars.AddRange(new[] { b + (tmp >> 30 & 0x7F), tmp >> 23 & 0x7F, tmp >> 16 & 0x7F, tmp >> 9 & 0x7F, tmp >> 2 & 0x7F });
            productId = new string(chars.Select(c => (char)(c & 0x7F)).ToArray());
        }

        if ((option & 4) == 4 && (done & 4) == 0)
        {
            uidUpper = NvmDword(0x7B);
            uidLower = NvmDword(0x7C);
            signalRate400 = (NvmDword(0x73) & 0xFF) << 8;
            signalRate400 |= (NvmDword(0x74) & 0xFF000000L) >> 24;
            dist400 = (NvmDword(0x75) & 0xFF) << 8;
            dist400 |= (NvmDword(0x76) & 0xFF000000L) >> 24;
        }

        P.WriteByte(0x81, 0x00);
        P.WriteByte(0xFF, 0x06);
        P.WriteByte(0x83, P.ReadByte(0x83) & 0xFB);
        P.WriteByte(0xFF, 0x01);
        P.WriteByte(0x00, 0x01);
        P.WriteByte(0xFF, 0x00);
        P.WriteByte(0x80, 0x00);

        if ((option & 1) == 1 && (done & 1) == 0)
        {
            _referenceSpadCount = refSpadCount;
            _referenceSpadType = refSpadType;
            _refGoodSpadMap = goodSpadMap;
        }
        if ((option & 2) == 2 && (done & 2) == 0)
        {
            ModuleId = moduleId;
            Revision = revision;
            ProductIdString = productId;
        }
        if ((option & 4) == 4 && (done & 4) == 0)
        {
            PartUidUpper = uidUpper;
            PartUidLower = uidLower;
            SignalRateMeasFixed400mm = signalRate400 << 9;
            long offsetUm = 0;
            if (dist400 != 0)
            {
                // uint32 difference, uint32 product, then an int16_t: the C driver's types,
                // kept because they are what makes the sign come out right. A target measured
                // nearer than 400 mm wraps the uint32 difference, and only the int16
                // truncation turns that back into the positive offset it stands for.
                long offset1104Mm = U32(dist400 - (400 << 4));
                offsetUm = I16(-I16(U32(offset1104Mm * 1000) >> 4));
            }
            Part2PartOffsetAdjustmentNvmUm = offsetUm;
        }
        _readDataFromDeviceDone = done | option;
    }

    // ── reference SPADs ──────────────────────────────────────────────────────

    private void SetReferenceSpads(int count, int isApertureSpads)
    {
        const int startSelect = 0xB4;
        const int maxSpadCount = 44;
        P.WriteByte(0xFF, 0x01);
        P.WriteByte(DynamicSpadRefEnStartOffset, 0x00);
        P.WriteByte(DynamicSpadNumRequestedRefSpad, 0x2C);
        P.WriteByte(0xFF, 0x00);
        P.WriteByte(GlobalConfigRefEnStartSelect, startSelect);

        var spadArray = new int[RefSpadBufferSize];
        int current = 0;
        if (isApertureSpads != 0)
            while (!IsAperture(startSelect + current) && current < maxSpadCount)
                current++;
        for (int i = 0; i < count; i++)
        {
            int next = NextGoodSpad(_refGoodSpadMap, RefSpadBufferSize, current);
            if (next == -1)
                throw new Vl53lxException("ran out of good reference SPADs");
            if (IsAperture(startSelect + next) != (isApertureSpads != 0))
                throw new Vl53lxException("the good SPAD map leaves the requested quadrant");
            current = next;
            spadArray[current / 8] |= 1 << (current % 8);
            current++;
        }
        byte[] bytes = spadArray.Select(x => (byte)x).ToArray();
        P.WriteMulti(GlobalConfigSpadEnablesRef0, bytes);
        if (!bytes.SequenceEqual(P.ReadMulti(GlobalConfigSpadEnablesRef0, RefSpadBufferSize)))
            throw new Vl53lxException("reference SPAD map did not read back");
        _refSpadEnables = spadArray;
        _referenceSpadCount = count;
        _referenceSpadType = isApertureSpads;
    }

    // ── tuning settings ──────────────────────────────────────────────────────

    /// <summary>VL53L0X_load_tuning_settings(): {count, address, bytes...} records, with count 0xFF carrying
    /// a host-side parameter instead; a zero count ends the buffer.</summary>
    public void LoadTuningSettings(byte[] buffer)
    {
        int index = 0;
        while (buffer[index] != 0)
        {
            int writes = buffer[index++];
            if (writes == 0xFF)
            {
                // A host-side parameter: {0xFF, select, msb, lsb}. The three sigma parameters
                // are constants in the 1.0.4 sigma estimate and only get stored; the fourth is
                // the rate the reference SPAD management converges on.
                int select = buffer[index];
                if (select >= TuningParamCount)
                    throw new Vl53lxException($"bad tuning parameter {select} at offset {index - 1}");
                int value = (buffer[index + 1] << 8) | buffer[index + 2];
                switch (select)
                {
                    case 0: _sigmaEstRefArray = value; break;
                    case 1: _sigmaEstEffPulseWidth = value; break;
                    case 2: _sigmaEstEffAmbWidth = value; break;
                    default: _targetRefRate = value; break;
                }
                index += 3;
            }
            else if (writes <= 4)
            {
                int address = buffer[index++];
                P.WriteMulti(address, buffer.AsSpan(index, writes).ToArray());
                index += writes;
            }
            else
            {
                throw new Vl53lxException($"bad tuning record at offset {index}");
            }
        }
    }

    // ── GPIO / interrupt ─────────────────────────────────────────────────────

    private void SetGpioConfig(int functionality, bool polarityLow)
    {
        P.WriteByte(SystemInterruptConfigGpio, functionality);
        int data = polarityLow ? 0x00 : 0x10;
        int current = P.ReadByte(GpioHvMuxActiveHigh);
        P.WriteByte(GpioHvMuxActiveHigh, (current & 0xEF) | data);
        _pin0GpioFunctionality = functionality;
        ClearInterrupt();
    }

    /// <summary>VL53L0X_ClearInterruptMask(): two writes, then confirm — up to three rounds.</summary>
    public override void ClearInterrupt()
    {
        for (int i = 0; i < 3; i++)
        {
            P.WriteByte(SystemInterruptClear, 0x01);
            P.WriteByte(SystemInterruptClear, 0x00);
            if ((P.ReadByte(ResultInterruptStatus) & 0x07) == 0x00)
                return;
        }
        throw new Vl53lxException("interrupt not cleared after three rounds");
    }

    private int GetInterruptMaskStatus()
    {
        int b = P.ReadByte(ResultInterruptStatus);
        if ((b & 0x18) != 0)
            throw new Vl53lxException($"range error reported in RESULT_INTERRUPT_STATUS 0x{b:X2}");
        return b & 0x07;
    }

    public override bool CheckForDataReady() =>
        _pin0GpioFunctionality == GpioFunctionalityNewMeasureReady
            ? GetInterruptMaskStatus() == GpioFunctionalityNewMeasureReady
            : (P.ReadByte(ResultRangeStatus) & 0x01) != 0;

    // ── sequence steps ───────────────────────────────────────────────────────

    private bool[] GetSequenceStepEnables()
    {
        int config = P.ReadByte(SystemSequenceConfig);
        return SeqTest.Select(mask => (config & mask) != 0).ToArray();
    }

    private void SetSequenceStepEnable(int step, int enabled)
    {
        int config = P.ReadByte(SystemSequenceConfig);
        int next = enabled != 0 ? config | SeqSet[step] : config & SeqClear[step];
        if (next == config)
            return;
        P.WriteByte(SystemSequenceConfig, next);
        _sequenceConfig = next;
        // The budget is spread over the enabled steps, so it is re-applied.
        SetMeasurementTimingBudget(_measurementTimingBudgetUs);
    }

    private int GetVcselPulsePeriod(int type) =>
        DecodeVcselPeriod(P.ReadByte(type == VcselPreRange ? PreRangeConfigVcselPeriod : FinalRangeConfigVcselPeriod));

    private long GetSequenceStepTimeout(int step)
    {
        if (step == SeqTcc || step == SeqDss || step == SeqMsrc)
        {
            int vcsel = GetVcselPulsePeriod(VcselPreRange);
            long mclks = DecodeTimeout(P.ReadByte(MsrcConfigTimeoutMacrop));
            return TimeoutUs(mclks, vcsel);
        }
        if (step == SeqPreRange)
        {
            int vcsel = GetVcselPulsePeriod(VcselPreRange);
            long mclks = DecodeTimeout(P.ReadWord(PreRangeConfigTimeoutMacropHi));
            return TimeoutUs(mclks, vcsel);
        }
        if (step == SeqFinalRange)
        {
            var steps = GetSequenceStepEnables();
            long pre = 0;
            if (steps[SeqPreRange])
                pre = DecodeTimeout(P.ReadWord(PreRangeConfigTimeoutMacropHi));
            int vcsel = GetVcselPulsePeriod(VcselFinalRange);
            long final = DecodeTimeout(P.ReadWord(FinalRangeConfigTimeoutMacropHi));
            return TimeoutUs((final - pre) & 0xFFFF, vcsel);
        }
        throw new Vl53lxException($"no timeout for sequence step {step}");
    }

    private void SetSequenceStepTimeout(int step, long timeoutUs)
    {
        if (step == SeqTcc || step == SeqDss || step == SeqMsrc)
        {
            int vcsel = GetVcselPulsePeriod(VcselPreRange);
            long mclks = TimeoutMclks(timeoutUs, vcsel);
            int encoded = mclks > 256 ? 255 : (int)((mclks - 1) & 0xFF);
            P.WriteByte(MsrcConfigTimeoutMacrop, encoded);
        }
        else if (step == SeqPreRange)
        {
            int vcsel = GetVcselPulsePeriod(VcselPreRange);
            P.WriteWord(PreRangeConfigTimeoutMacropHi, EncodeTimeout(TimeoutMclks(timeoutUs, vcsel)));
            _preRangeTimeoutUs = timeoutUs;
        }
        else if (step == SeqFinalRange)
        {
            // The final-range register carries pre-range + final range, in macro periods.
            var steps = GetSequenceStepEnables();
            long pre = 0;
            if (steps[SeqPreRange])
                pre = DecodeTimeout(P.ReadWord(PreRangeConfigTimeoutMacropHi));
            int vcsel = GetVcselPulsePeriod(VcselFinalRange);
            P.WriteWord(FinalRangeConfigTimeoutMacropHi, EncodeTimeout(TimeoutMclks(timeoutUs, vcsel) + pre));
            _finalRangeTimeoutUs = timeoutUs;
        }
        else
        {
            throw new Vl53lxException($"no timeout for sequence step {step}");
        }
    }

    // ── timing budget ────────────────────────────────────────────────────────

    private const int StartOverheadUs = 1910, EndOverheadUs = 960, MsrcOverheadUs = 660, TccOverheadUs = 590;
    private const int DssOverheadUs = 690, PreRangeOverheadUs = 660, FinalRangeOverheadUs = 550;

    private void SetMeasurementTimingBudget(long budgetUs)
    {
        if (budgetUs < MinTimingBudgetUs)
            throw new Vl53lxException($"timing budget {budgetUs} us is below the {MinTimingBudgetUs} us minimum");
        var steps = GetSequenceStepEnables();
        long finalBudgetUs = budgetUs - (StartOverheadUs + EndOverheadUs);
        void Take(long sub)
        {
            if (sub >= finalBudgetUs)
                throw new Vl53lxException($"timing budget {budgetUs} us is too small for the enabled sequence steps");
            finalBudgetUs -= sub;
        }
        if (steps[SeqTcc] || steps[SeqMsrc] || steps[SeqDss])
        {
            long msrcUs = GetSequenceStepTimeout(SeqMsrc); // TCC, MSRC and DSS share one timeout
            if (steps[SeqTcc])
                Take(msrcUs + TccOverheadUs);
            if (steps[SeqDss])
                Take(2 * (msrcUs + DssOverheadUs));
            else if (steps[SeqMsrc])
                Take(msrcUs + MsrcOverheadUs);
        }
        if (steps[SeqPreRange])
            Take(GetSequenceStepTimeout(SeqPreRange) + PreRangeOverheadUs);
        if (steps[SeqFinalRange])
        {
            finalBudgetUs -= FinalRangeOverheadUs; // whatever is left goes to the final range
            SetSequenceStepTimeout(SeqFinalRange, finalBudgetUs);
            _measurementTimingBudgetUs = budgetUs;
        }
    }

    private long GetMeasurementTimingBudget()
    {
        var steps = GetSequenceStepEnables();
        long budgetUs = StartOverheadUs + EndOverheadUs;
        if (steps[SeqTcc] || steps[SeqMsrc] || steps[SeqDss])
        {
            long msrcUs = GetSequenceStepTimeout(SeqMsrc);
            if (steps[SeqTcc])
                budgetUs += msrcUs + TccOverheadUs;
            if (steps[SeqDss])
                budgetUs += 2 * (msrcUs + DssOverheadUs);
            else if (steps[SeqMsrc])
                budgetUs += msrcUs + MsrcOverheadUs;
        }
        if (steps[SeqPreRange])
            budgetUs += GetSequenceStepTimeout(SeqPreRange) + PreRangeOverheadUs;
        if (steps[SeqFinalRange])
            budgetUs += GetSequenceStepTimeout(SeqFinalRange) + FinalRangeOverheadUs;
        _measurementTimingBudgetUs = budgetUs;
        return budgetUs;
    }

    private void SetInterMeasurementPeriod(long periodMs)
    {
        int oscCalibrateVal = P.ReadWord(OscCalibrateVal);
        P.WriteDword(SystemIntermeasurementPeriod, oscCalibrateVal != 0 ? periodMs * oscCalibrateVal : periodMs);
    }

    private long GetInterMeasurementPeriod()
    {
        int oscCalibrateVal = P.ReadWord(OscCalibrateVal);
        long value = P.ReadDword(SystemIntermeasurementPeriod);
        return oscCalibrateVal != 0 ? value / oscCalibrateVal : value;
    }

    /// <summary>Budget and period as the family asks for them: a period of 0 means back-to-back continuous,
    /// anything else the timed continuous mode.</summary>
    public override void SetRangeTiming(int timingBudgetMs, int interMeasurementMs)
    {
        SetMeasurementTimingBudget(timingBudgetMs * 1000L);
        SetInterMeasurementPeriod(interMeasurementMs);
        SetDeviceMode(interMeasurementMs == 0 ? DeviceModeContinuousRanging : DeviceModeContinuousTimed);
    }

    public override (int TimingBudgetMs, int InterMeasurementMs) GetRangeTiming() =>
        ((int)(GetMeasurementTimingBudget() / 1000), (int)GetInterMeasurementPeriod());

    // ── ranging ──────────────────────────────────────────────────────────────

    private void SetDeviceMode(int mode)
    {
        if (mode != DeviceModeSingleRanging && mode != DeviceModeContinuousRanging && mode != DeviceModeContinuousTimed)
            throw new Vl53lxException($"device mode {mode} is not supported");
        _deviceMode = mode;
        if (mode != DeviceModeSingleRanging)
            _continuousMode = mode;
    }

    // The undocumented prologue of every VL53L0X_StartMeasurement(): the stop variable goes back.
    private void ArmStopVariable()
    {
        P.WriteByte(0x80, 0x01);
        P.WriteByte(0xFF, 0x01);
        P.WriteByte(0x00, 0x00);
        P.WriteByte(0x91, _stopVariable);
        P.WriteByte(0x00, 0x01);
        P.WriteByte(0xFF, 0x00);
        P.WriteByte(0x80, 0x00);
    }

    /// <summary>VL53L0X_StartMeasurement() for the continuous modes; a single shot in between is resumed from.</summary>
    public override void StartRanging()
    {
        if (_deviceMode == DeviceModeSingleRanging && _continuousMode is int resume)
            SetDeviceMode(resume);
        ArmStopVariable();
        if (_deviceMode == DeviceModeContinuousRanging)
            P.WriteByte(SysrangeStart, SysrangeModeBacktoback);
        else if (_deviceMode == DeviceModeContinuousTimed)
            P.WriteByte(SysrangeStart, SysrangeModeTimed);
        else
            throw new Vl53lxException("start_ranging() needs a continuous device mode");
    }

    /// <summary>VL53L0X_StopMeasurement().</summary>
    public override void StopRanging()
    {
        P.WriteByte(SysrangeStart, SysrangeModeSingleshot);
        P.WriteByte(0xFF, 0x01);
        P.WriteByte(0x00, 0x00);
        P.WriteByte(0x91, 0x00);
        P.WriteByte(0x00, 0x01);
        P.WriteByte(0xFF, 0x00);
    }

    private RangingData PerformSingleRangingMeasurement()
    {
        SetDeviceMode(DeviceModeSingleRanging);
        ArmStopVariable();
        P.WriteByte(SysrangeStart, 0x01);
        bool cleared = false;
        for (int i = 0; i < DefaultMaxLoop; i++)
        {
            if ((P.ReadByte(SysrangeStart) & SysrangeModeStartStop) == 0)
            {
                cleared = true;
                break;
            }
        }
        if (!cleared)
            throw new Vl53lxException("the single-shot start bit never cleared");
        WaitDataReady(TimeSpan.FromSeconds(1));
        var data = GetResult();
        ClearInterrupt();
        return data;
    }

    private RangingData GetResult() => ParseResultBlock(P.ReadMulti(ResultBlockAddr, ResultBlockLen));

    // ── result decoding ──────────────────────────────────────────────────────

    // VL53L0X_RangingMeasurementData_t, the fields this port fills.
    private sealed class RangingData
    {
        public int RangeStatus;
        public int DistanceMm;
        public int RangeFractionalPart;
        public long SignalRateMcps;
        public long AmbientRateMcps;
        public long EffectiveSpadRtnCount;
        public long DmaxMm;
        public long SigmaMm;
        public int DeviceRangeStatus;
    }

    /// <summary>VL53L0X_GetRangingMeasurementData() on the 12 bytes at 0x14: pure arithmetic over the block and
    /// the cached device data, so a streamed frame decodes exactly like a polled one.</summary>
    private RangingData ParseResultBlock(byte[] raw)
    {
        if (raw.Length < ResultBlockLen)
            throw new Vl53lxException($"result block too short: {raw.Length} bytes");
        int distance = (raw[10] << 8) + raw[11];
        var data = new RangingData
        {
            SignalRateMcps = (long)((raw[6] << 8) + raw[7]) << 9,   // 9.7 → 16.16
            AmbientRateMcps = (long)((raw[8] << 8) + raw[9]) << 9,
            EffectiveSpadRtnCount = (raw[2] << 8) + raw[3],         // 8.8
            DeviceRangeStatus = raw[0],
        };
        if (_rangeFractionalEnable != 0)
        {
            data.DistanceMm = distance >> 2;
            data.RangeFractionalPart = (distance & 0x03) << 6;
        }
        else
        {
            data.DistanceMm = distance;
        }
        GetPalRangeStatus(data);
        return data;
    }

    private long GetTotalXtalkRate(RangingData data)
    {
        if (_xtalkCompensationEnable == 0)
            return 0;
        long total = data.EffectiveSpadRtnCount * _xtalkCompensationRateMegaCps;
        return (total + 0x80) >> 8;
    }

    /// <summary>VL53L0X_calc_sigma_estimate(), FixPoint16.16 in and out.</summary>
    private long CalcSigmaEstimate(RangingData data)
    {
        const long cPulseEffectiveWidthCentiNs = 800;
        const long cAmbientEffectiveWidthCentiNs = 600;
        const long cDfltFinalRangeIntegrationTimeMs = 0x00190000; // 25 ms
        const long cVcselPulseWidthPs = 4700;
        const long cSigmaEstMax = 0x028F87AE;
        const long cSigmaEstRtnMax = 0xF000;
        const long cAmbToSignalRatioMax = 0xF0000000L / cAmbientEffectiveWidthCentiNs;
        const long cTofPerMmPs = 0x0006999A;
        const long c16BitRoundingParam = 0x00008000;
        const long cMaxXtalkKcps = 0x00320000;
        const long cPllPeriodPs = 1655;

        long ambientRateKcps = (data.AmbientRateMcps * 1000) >> 16;
        long xtalkCompRateMcps = GetTotalXtalkRate(data);
        long totalSignalRateMcps = data.SignalRateMcps + xtalkCompRateMcps;
        long peakSignalRateKcps = ((totalSignalRateMcps * 1000) + 0x8000) >> 16;
        long xtalkCompRateKcps = Math.Min(xtalkCompRateMcps * 1000, cMaxXtalkKcps);

        long finalRangeTimeoutUs = _finalRangeTimeoutUs;
        int finalRangeVcselPclks = _finalRangeVcselPulsePeriod;
        long finalRangeMacroPclks = TimeoutMclks(finalRangeTimeoutUs, finalRangeVcselPclks);
        long preRangeTimeoutUs = _preRangeTimeoutUs;
        long preRangeMacroPclks = TimeoutMclks(preRangeTimeoutUs, _preRangeVcselPulsePeriod);

        long vcselWidth = finalRangeVcselPclks == 8 ? 2 : 3;
        long peakVcselDurationUs = vcselWidth * 2048 * (preRangeMacroPclks + finalRangeMacroPclks);
        peakVcselDurationUs = (peakVcselDurationUs + 500) / 1000;
        peakVcselDurationUs *= cPllPeriodPs;
        peakVcselDurationUs = (peakVcselDurationUs + 500) / 1000;

        long totalSignalRate2408 = (totalSignalRateMcps + 0x80) >> 8;
        long vcselTotalEventsRtn = ((totalSignalRate2408 * peakVcselDurationUs) + 0x80) >> 8;

        if (peakSignalRateKcps == 0)
            return cSigmaEstMax;
        vcselTotalEventsRtn = Math.Max(1, vcselTotalEventsRtn);

        long p1 = cPulseEffectiveWidthCentiNs;
        long p2 = (ambientRateKcps << 16) / peakSignalRateKcps;
        p2 = Math.Min(p2, cAmbToSignalRatioMax);
        p2 *= cAmbientEffectiveWidthCentiNs;
        long p3 = 2 * Isqrt(vcselTotalEventsRtn * 12);

        long deltaTPs = data.DistanceMm * cTofPerMmPs;
        long diff1Mcps = U32(U32((peakSignalRateKcps << 16) - 2 * xtalkCompRateKcps) + 500) / 1000;
        long diff2Mcps = ((peakSignalRateKcps << 16) + 500) / 1000;
        diff1Mcps <<= 8;
        long xtalkCorrection = Math.Abs(diff1Mcps / diff2Mcps) << 8;

        long pwMult = deltaTPs / cVcselPulseWidthPs;
        pwMult = U32(pwMult * U32((1L << 16) - xtalkCorrection));
        pwMult = (pwMult + c16BitRoundingParam) >> 16;
        pwMult += 1L << 16;
        pwMult >>= 1; // squaring 1.xx would leave 32 bits, so the C driver halves it first
        pwMult = (pwMult * pwMult) >> 14;

        long sqr1 = ((pwMult * p1) + 0x8000) >> 16;
        sqr1 *= sqr1;
        long sqr2 = (p2 + 0x8000) >> 16;
        sqr2 *= sqr2;

        long sqrtResultCentiNs = Isqrt(sqr1 + sqr2) << 16;
        long sigmaEstRtn = ((sqrtResultCentiNs + 50) / 100) / p3;
        sigmaEstRtn *= SpeedOfLightInAir;
        sigmaEstRtn = (sigmaEstRtn + 5000) / 10000;
        sigmaEstRtn = Math.Min(sigmaEstRtn, cSigmaEstRtnMax);

        long finalRangeIntegrationTimeMs = (finalRangeTimeoutUs + preRangeTimeoutUs + 500) / 1000;
        long sigmaEstRef = Isqrt((cDfltFinalRangeIntegrationTimeMs + finalRangeIntegrationTimeMs / 2)
                                 / finalRangeIntegrationTimeMs) << 8;
        sigmaEstRef = (sigmaEstRef + 500) / 1000;

        long sigmaEstimate = 1000 * Isqrt(sigmaEstRtn * sigmaEstRtn + sigmaEstRef * sigmaEstRef);
        if (peakSignalRateKcps < 1 || vcselTotalEventsRtn < 1 || sigmaEstimate > cSigmaEstMax)
            sigmaEstimate = cSigmaEstMax;
        return sigmaEstimate;
    }

    /// <summary>VL53L0X_calc_dmax(): ambient rate → max range, linear between the table's points.</summary>
    private static long CalcDmax(long ambRateMeas)
    {
        var amb = DmaxLutAmbRateMcps;
        var dmax = DmaxLutDmaxMm;
        if (ambRateMeas <= amb[0])
            return dmax[0] >> 16;
        if (ambRateMeas >= amb[^1])
            return dmax[^1] >> 16;
        int index1 = Array.FindIndex(amb, a => ambRateMeas <= a);
        int index0 = index1 != 0 ? index1 - 1 : 0;
        if (index0 == index1)
            return dmax[index0] >> 16;
        long amb0 = amb[index0], amb1 = amb[index1], dmax0 = dmax[index0], dmax1 = dmax[index1];
        if (amb1 == amb0)
            return dmax0 >> 16;
        long linearSlope = U32(dmax0 - dmax1) / ((amb1 - amb0) >> 8);
        return (((amb1 - ambRateMeas) >> 8) * linearSlope + dmax1) >> 16;
    }

    /// <summary>VL53L0X_get_pal_range_status(): the device status and the enabled limit checks mapped onto the
    /// PAL range status. DataInit leaves the reference clip and the range-ignore check off, so only the
    /// sigma check runs — arithmetic over cached data.</summary>
    private void GetPalRangeStatus(RangingData data)
    {
        int internalStatus = (data.DeviceRangeStatus & 0x78) >> 3;
        bool noneFlag = internalStatus is 0 or 5 or 7 or 12 or 13 or 14 or 15;
        bool sigmaLimitFlag = false;
        if (_limitChecksEnable[CheckSigmaFinalRange] != 0)
        {
            long sigmaEstimate = CalcSigmaEstimate(data);
            data.SigmaMm = sigmaEstimate >> 16;
            data.DmaxMm = CalcDmax(data.AmbientRateMcps);
            long sigmaLimitValue = _limitChecksValue[CheckSigmaFinalRange];
            sigmaLimitFlag = sigmaLimitValue > 0 && sigmaEstimate > sigmaLimitValue;
        }
        data.RangeStatus = noneFlag ? 255
            : internalStatus is 1 or 2 or 3 ? 5   // hardware fail
            : internalStatus is 6 or 9 ? 4        // phase fail
            : internalStatus is 8 or 10 ? 3       // min range
            : internalStatus == 4 ? 2             // signal fail
            : sigmaLimitFlag ? 1                  // sigma fail
            : 0;
    }

    private static Vl53lxRange AsRange(RangingData r) =>
        new(r.DistanceMm, r.RangeStatus,
            RangeStatusNames.TryGetValue(r.RangeStatus, out var s) ? s : $"unknown ({r.RangeStatus})",
            (r.SignalRateMcps * 1000) >> 16, (r.AmbientRateMcps * 1000) >> 16, r.SigmaMm,
            r.EffectiveSpadRtnCount >> 8, Array.Empty<Vl53lxTarget>(),
            new Dictionary<string, object?> { ["dmax_mm"] = r.DmaxMm, ["device_range_status"] = (long)r.DeviceRangeStatus });

    // ── VCSEL period and the ranging profiles ────────────────────────────────

    private void SetVcselPulsePeriod(int type, int pclks)
    {
        var limits = type == VcselPreRange
            ? new Dictionary<int, int> { [12] = 0x18, [14] = 0x30, [16] = 0x40, [18] = 0x50 }
            : new Dictionary<int, int> { [8] = 0x10, [10] = 0x28, [12] = 0x38, [14] = 0x48 };
        if (pclks % 2 != 0 || !limits.TryGetValue(pclks, out int phaseHigh))
            throw new Vl53lxException($"VCSEL period {pclks} PCLK is out of range for the {(type == VcselPreRange ? "pre-range" : "final range")}");
        if (type == VcselPreRange)
        {
            P.WriteByte(PreRangeConfigValidPhaseHigh, phaseHigh);
            P.WriteByte(PreRangeConfigValidPhaseLow, 0x08);
        }
        else
        {
            // The final range also retunes the VCSEL width and the phase-cal limits.
            var (width, timeout, phasecalLim) = pclks switch
            {
                8 => (0x02, 0x0C, 0x30),
                10 => (0x03, 0x09, 0x20),
                12 => (0x03, 0x08, 0x20),
                _ => (0x03, 0x07, 0x20),
            };
            P.WriteByte(FinalRangeConfigValidPhaseHigh, limits[pclks]);
            P.WriteByte(FinalRangeConfigValidPhaseLow, 0x08);
            P.WriteByte(GlobalConfigVcselWidth, width);
            P.WriteByte(AlgoPhasecalConfigTimeout, timeout);
            P.WriteByte(0xFF, 0x01);
            P.WriteByte(AlgoPhasecalLim, phasecalLim);
            P.WriteByte(0xFF, 0x00);
        }
        int reg = EncodeVcselPeriod(pclks);
        if (type == VcselPreRange)
        {
            long preTimeout = GetSequenceStepTimeout(SeqPreRange);
            long msrcTimeout = GetSequenceStepTimeout(SeqMsrc);
            P.WriteByte(PreRangeConfigVcselPeriod, reg);
            SetSequenceStepTimeout(SeqPreRange, preTimeout);
            SetSequenceStepTimeout(SeqMsrc, msrcTimeout);
            _preRangeVcselPulsePeriod = pclks;
        }
        else
        {
            long finalTimeout = GetSequenceStepTimeout(SeqFinalRange);
            P.WriteByte(FinalRangeConfigVcselPeriod, reg);
            SetSequenceStepTimeout(SeqFinalRange, finalTimeout);
            _finalRangeVcselPulsePeriod = pclks;
        }
        SetMeasurementTimingBudget(_measurementTimingBudgetUs);
        PerformPhaseCalibration();
    }

    private void PerformPhaseCalibration()
    {
        int sequenceConfig = _sequenceConfig;
        P.WriteByte(SystemSequenceConfig, 0x02);
        PerformSingleRefCalibration(0x00);
        P.WriteByte(SystemSequenceConfig, sequenceConfig);
        _sequenceConfig = sequenceConfig;
    }

    /// <summary>Apply one of <see cref="Modes"/>: the sigma and signal limits, the budget and both VCSEL periods.</summary>
    public override int? SetMode(string name)
    {
        var settings = ModeSettings.FirstOrDefault(m => m.Name == name);
        if (settings.Name is null)
            throw new ArgumentException($"no such mode: {name} (have {string.Join(", ", Modes)})", nameof(name));
        SetLimitCheckEnable(CheckSigmaFinalRange, 1);
        SetLimitCheckEnable(CheckSignalRateFinalRange, 1);
        SetLimitCheckValue(CheckSignalRateFinalRange, (long)(settings.Signal * 65536));
        SetLimitCheckValue(CheckSigmaFinalRange, settings.Sigma * 65536L);
        SetMeasurementTimingBudget(settings.BudgetUs);
        SetVcselPulsePeriod(VcselPreRange, settings.Pre);
        SetVcselPulsePeriod(VcselFinalRange, settings.Final);
        _mode = name;
        return null;
    }

    public override string GetMode() => _mode;

    /// <summary>By the final-range VCSEL period read from the sensor; null for a period no profile sets.</summary>
    public override int? ReachMm() =>
        ReachByFinalPclksMm.TryGetValue(GetVcselPulsePeriod(VcselFinalRange), out int mm) ? mm : null;

    public override IReadOnlyDictionary<string, object?> DriverInfo() => new Dictionary<string, object?>
    {
        ["signal_mcps"] = GetLimitCheckValue(CheckSignalRateFinalRange) / 65536.0,
        ["sigma_mm"] = GetLimitCheckValue(CheckSigmaFinalRange) / 65536.0,
        ["budget_us"] = GetMeasurementTimingBudget(),
        ["pre_pclks"] = GetVcselPulsePeriod(VcselPreRange),
        ["final_pclks"] = GetVcselPulsePeriod(VcselFinalRange),
    };

    // ── offset ───────────────────────────────────────────────────────────────

    /// <summary>The register is 10.2 format in mm: steps of 250 µm.</summary>
    public void SetOffsetUm(long offsetUm)
    {
        offsetUm = Math.Max(-512000, Math.Min(511000, offsetUm));
        long steps = Math.Abs(offsetUm) / 250;
        long encoded = offsetUm >= 0 ? steps : 4096 - steps;
        P.WriteWord(AlgoPartToPartRangeOffsetMm, (int)(encoded & 0xFFFF));
    }

    public long GetOffsetUm()
    {
        int reg = P.ReadWord(AlgoPartToPartRangeOffsetMm) & 0x0FFF;
        return reg > 2047 ? (reg - 4096) * 250L : reg * 250L;
    }

    public override void SetOffset(int offsetMm) => SetOffsetUm(offsetMm * 1000L);

    public override int GetOffset() => (int)Math.Round(GetOffsetUm() / 1000.0, MidpointRounding.ToEven);

    /// <summary>VL53L0X_perform_offset_calibration() → µm. TCC and the range-ignore threshold are off for the run.</summary>
    public long PerformOffsetCalibration(int calDistanceMm, int nbSamples = 50)
    {
        if (calDistanceMm <= 0)
            throw new Vl53lxException("the calibration distance must be positive");
        SetOffsetUm(0);
        bool tccWasOn = GetSequenceStepEnables()[SeqTcc];
        SetSequenceStepEnable(SeqTcc, 0);
        SetLimitCheckEnable(CheckRangeIgnoreThreshold, 0);
        long sum = 0;
        int count = 0;
        for (int i = 0; i < nbSamples; i++)
        {
            var data = PerformSingleRangingMeasurement();
            if (data.RangeStatus == 0)
            {
                sum = (sum + data.DistanceMm) & 0xFFFF;
                count++;
            }
        }
        if (count == 0)
            throw new Vl53lxException("offset calibration got no valid measurement - check that a target is in front of the sensor");
        long meanMm = (2 * sum + count) / (2 * count); // round half up
        if (meanMm == 0)
            throw new Vl53lxException("the mean range came out 0 mm - the target is closer than the part-to-part offset; "
                                      + "move it out to 100..400 mm and calibrate again");
        long offsetUm = (calDistanceMm - meanMm) * 1000;
        SetOffsetUm(offsetUm);
        if (tccWasOn)
            SetSequenceStepEnable(SeqTcc, 1);
        return offsetUm;
    }

    public override int CalibrateOffset(int targetDistMm, int? nbSamples = null) =>
        (int)Math.Round(PerformOffsetCalibration(targetDistMm, nbSamples ?? 50) / 1000.0, MidpointRounding.ToEven);

    // ── crosstalk ────────────────────────────────────────────────────────────

    private void SetXtalkEnable(int enable)
    {
        long rate = enable != 0 ? _xtalkCompensationRateMegaCps : 0;
        P.WriteWord(CrosstalkCompensationPeakRateMcps, (int)((rate >> 3) & 0xFFFF)); // 16.16 → 3.13
        _xtalkCompensationEnable = enable != 0 ? 1 : 0;
    }

    private void SetXtalkRateMcps(long rateMcps)
    {
        if (_xtalkCompensationEnable != 0)
            P.WriteWord(CrosstalkCompensationPeakRateMcps, (int)((rateMcps >> 3) & 0xFFFF));
        _xtalkCompensationRateMegaCps = rateMcps;
    }

    private long GetXtalkRateMcps() => (long)P.ReadWord(CrosstalkCompensationPeakRateMcps) << 3;

    public override void SetXtalk(int xtalkKcps)
    {
        SetXtalkEnable(1);
        SetXtalkRateMcps(((long)xtalkKcps << 16) / 1000);
    }

    public override int GetXtalk() => (int)((GetXtalkRateMcps() * 1000) >> 16);

    /// <summary>VL53L0X_perform_xtalk_calibration() → the rate, Mcps 16.16.</summary>
    public long PerformXtalkCalibration(int calDistanceMm, int nbSamples = 50)
    {
        if (calDistanceMm <= 0)
            throw new Vl53lxException("the calibration distance must be positive");
        SetXtalkEnable(0);
        SetLimitCheckEnable(CheckRangeIgnoreThreshold, 0);
        long sumRanging = 0, sumSignal = 0, sumSpads = 0;
        int count = 0;
        for (int i = 0; i < nbSamples; i++)
        {
            var data = PerformSingleRangingMeasurement();
            if (data.RangeStatus == 0)
            {
                // The C sums are uint16 / uint32; kept that way on purpose.
                sumRanging = (sumRanging + data.DistanceMm) & 0xFFFF;
                sumSignal = U32(sumSignal + data.SignalRateMcps);
                sumSpads = (sumSpads + data.EffectiveSpadRtnCount / 256) & 0xFFFF;
                count++;
            }
        }
        if (count == 0)
            throw new Vl53lxException("crosstalk calibration got no valid measurement - check that a target is in front of the sensor");
        long meanSignal = sumSignal / count;
        long meanRange = U32(sumRanging << 16) / count;
        long meanSpads = U32(sumSpads << 16) / count;
        long meanSpadsInt = (meanSpads + 0x8000) >> 16;
        long calDistance = (long)calDistanceMm << 16;
        long rate;
        if (meanSpadsInt == 0 || meanRange >= calDistance)
        {
            rate = 0;
        }
        else
        {
            long perSpad = meanSignal / meanSpadsInt;
            perSpad = U32(perSpad * ((1L << 16) - meanRange / calDistanceMm));
            rate = (perSpad + 0x8000) >> 16;
        }
        SetXtalkEnable(1);
        SetXtalkRateMcps(rate);
        return rate;
    }

    public override int CalibrateXtalk(int targetDistMm, int? nbSamples = null) =>
        (int)((PerformXtalkCalibration(targetDistMm, nbSamples ?? 50) * 1000) >> 16);

    // ── reference SPAD management ────────────────────────────────────────────

    public (int Count, int IsAperture) GetReferenceSpads() => (_referenceSpadCount, _referenceSpadType);

    private void SetRefSpadMap(int[] spadArray) =>
        P.WriteMulti(GlobalConfigSpadEnablesRef0, spadArray.Select(x => (byte)x).ToArray());

    private int[] GetRefSpadMap() =>
        P.ReadMulti(GlobalConfigSpadEnablesRef0, RefSpadBufferSize).Select(b => (int)b).ToArray();

    private int EnableRefSpads(int apertureSpads, int[] spadArray, int start, int offset, int spadCount)
    {
        int current = offset;
        for (int i = 0; i < spadCount; i++)
        {
            int next = NextGoodSpad(_refGoodSpadMap, RefSpadBufferSize, current);
            if (next == -1)
                throw new Vl53lxException("ran out of good reference SPADs");
            if (IsAperture(start + next) != (apertureSpads != 0))
                throw new Vl53lxException("the good SPAD map leaves the requested quadrant");
            current = next;
            spadArray[current / 8] |= 1 << (current % 8);
            current++;
        }
        SetRefSpadMap(spadArray);
        if (!GetRefSpadMap().SequenceEqual(spadArray))
            throw new Vl53lxException("reference SPAD map did not read back");
        return current;
    }

    private int PerformRefSignalMeasurement()
    {
        int sequenceConfig = _sequenceConfig;
        P.WriteByte(SystemSequenceConfig, 0xC0);
        PerformSingleRangingMeasurement();
        P.WriteByte(0xFF, 0x01);
        int peak = P.ReadWord(ResultPeakSignalRateRef);
        P.WriteByte(0xFF, 0x00);
        P.WriteByte(SystemSequenceConfig, sequenceConfig);
        _sequenceConfig = sequenceConfig;
        return peak;
    }

    /// <summary>Re-measure the reference SPADs on a live session → (count, is aperture). The stop variable goes
    /// back first (a stopped session leaves register 0x91 at 0, and the rate then reads far too high).</summary>
    public override object PerformRefSpadManagement()
    {
        ArmStopVariable();
        return RefSpadManagement();
    }

    // VL53L0X_perform_ref_spad_management(): the minimum non-aperture set, then one SPAD at a time until the
    // reference rate lands nearest 20 Mcps; aperture SPADs when even the minimum overshoots.
    private (int Count, int IsAperture) RefSpadManagement()
    {
        const int startSelect = 0xB4;
        const int minimumSpadCount = 3;
        const int maxSpadCount = 44;
        var spadArray = new int[RefSpadBufferSize];
        P.WriteByte(0xFF, 0x01);
        P.WriteByte(DynamicSpadRefEnStartOffset, 0x00);
        P.WriteByte(DynamicSpadNumRequestedRefSpad, 0x2C);
        P.WriteByte(0xFF, 0x00);
        P.WriteByte(GlobalConfigRefEnStartSelect, startSelect);
        P.WriteByte(PowerManagementGo1PowerForce, 0x00);
        PerformRefCalibration();

        int needApt = 0;
        int current = EnableRefSpads(needApt, spadArray, startSelect, 0, minimumSpadCount);
        int peak = PerformRefSignalMeasurement();
        if (peak > _targetRefRate)
        {
            spadArray = new int[RefSpadBufferSize];
            while (!IsAperture(startSelect + current) && current < maxSpadCount)
                current++;
            needApt = 1;
            current = EnableRefSpads(needApt, spadArray, startSelect, current, minimumSpadCount);
            peak = PerformRefSignalMeasurement();
            if (peak > _targetRefRate)
            {
                StoreRefSpads(minimumSpadCount, 1, spadArray);
                return (minimumSpadCount, 1);
            }
        }
        int refSpadCount = minimumSpadCount;
        var lastSpadArray = spadArray.ToArray();
        int lastDiff = Math.Abs(peak - _targetRefRate);
        while (peak < _targetRefRate)
        {
            int next = NextGoodSpad(_refGoodSpadMap, RefSpadBufferSize, current);
            if (next == -1)
                throw new Vl53lxException("ran out of good reference SPADs");
            if (IsAperture(startSelect + next) != (needApt != 0))
                break; // the quadrant is exhausted
            refSpadCount++;
            current = next;
            if (current / 8 >= RefSpadBufferSize)
                throw new Vl53lxException("reference SPAD index out of range");
            spadArray[current / 8] |= 1 << (current % 8);
            current++;
            SetRefSpadMap(spadArray);
            peak = PerformRefSignalMeasurement();
            int diff = Math.Abs(peak - _targetRefRate);
            if (peak > _targetRefRate)
            {
                if (diff > lastDiff)
                {
                    SetRefSpadMap(lastSpadArray); // the previous map came closer
                    spadArray = lastSpadArray.ToArray();
                    refSpadCount--;
                }
                break;
            }
            lastDiff = diff;
            lastSpadArray = spadArray.ToArray();
        }
        StoreRefSpads(refSpadCount, needApt, spadArray);
        return (refSpadCount, needApt);
    }

    private void StoreRefSpads(int count, int isApertureSpads, int[] spadArray)
    {
        _refSpadEnables = spadArray.ToArray();
        _referenceSpadCount = count;
        _referenceSpadType = isApertureSpads;
    }

    // ── limit checks ─────────────────────────────────────────────────────────

    private void SetLimitCheckEnable(int check, int enable)
    {
        if (check >= CheckNumberOfChecks)
            throw new Vl53lxException($"no limit check {check}");
        long value = enable == 0 ? 0 : _limitChecksValue[check];
        int disable = enable == 0 ? 1 : 0;
        if (check == CheckSignalRateFinalRange)
        {
            P.WriteWord(FinalRangeConfigMinCountRateRtnLimit, (int)((value >> 9) & 0xFFFF));
        }
        else if (check == CheckSignalRateMsrc)
        {
            int current = P.ReadByte(MsrcConfigControl);
            P.WriteByte(MsrcConfigControl, (current & 0xFE) | (disable << 1));
        }
        else if (check == CheckSignalRatePreRange)
        {
            int current = P.ReadByte(MsrcConfigControl);
            P.WriteByte(MsrcConfigControl, (current & 0xEF) | (disable << 4));
        }
        _limitChecksEnable[check] = enable == 0 ? 0 : 1; // the other three checks are host-side only
    }

    private void SetLimitCheckValue(int check, long value)
    {
        if (_limitChecksEnable[check] == 0)
        {
            _limitChecksValue[check] = value; // disabled: kept here
            return;
        }
        if (check == CheckSignalRateFinalRange)
            P.WriteWord(FinalRangeConfigMinCountRateRtnLimit, (int)((value >> 9) & 0xFFFF));
        else if (check == CheckSignalRateMsrc || check == CheckSignalRatePreRange)
            P.WriteWord(PreRangeMinCountRateRtnLimit, (int)((value >> 9) & 0xFFFF));
        _limitChecksValue[check] = value;
    }

    private long GetLimitCheckValue(int check) =>
        check == CheckSignalRateFinalRange ? (long)P.ReadWord(FinalRangeConfigMinCountRateRtnLimit) << 9
        : check == CheckSignalRateMsrc || check == CheckSignalRatePreRange ? (long)P.ReadWord(PreRangeMinCountRateRtnLimit) << 9
        : _limitChecksValue[check];
}
