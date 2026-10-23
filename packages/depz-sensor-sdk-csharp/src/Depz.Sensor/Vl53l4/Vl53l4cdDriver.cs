using System.Buffers.Binary;

namespace Depz.Sensor.Vl53l4;

/// <summary>What the ULD needs from the board: the register bridge (16-bit addresses,
/// auto-increment), a delay, and the bus speed. <see cref="Devices.Vl53l4cdDevice"/> maps it onto
/// the firmware bridge; a test can script it.</summary>
public interface IVl53l4Platform
{
    byte[] ReadMulti(ushort addr, int size);

    void WriteMulti(ushort addr, byte[] data);

    void SleepMs(int ms);

    void SetI2cSpeed(int khz);
}

/// <summary>
/// The ST VL53L4CD ULD 2.2.3 (VL53L4CD_api.c + VL53L4CD_calibration.c) on the
/// host, ported from the reference Python driver: init, ranging, timing,
/// offset, crosstalk, thresholds, temperature update, calibration. Every
/// request sequence is the Python one, so captures of either replay in both.
/// Register contents are big-endian.
/// </summary>
public sealed class Vl53l4cdDriver
{
    private readonly IVl53l4Platform _p;

    public Vl53l4cdDriver(IVl53l4Platform platform) =>
        _p = platform ?? throw new ArgumentNullException(nameof(platform));

    // ── register access ──────────────────────────────────────────────────────

    public int ReadByte(ushort addr) => _p.ReadMulti(addr, 1)[0];

    public int ReadWord(ushort addr) => BinaryPrimitives.ReadUInt16BigEndian(_p.ReadMulti(addr, 2));

    public uint ReadDword(ushort addr) => BinaryPrimitives.ReadUInt32BigEndian(_p.ReadMulti(addr, 4));

    public void WriteByte(ushort addr, int value) => _p.WriteMulti(addr, new[] { (byte)value });

    public void WriteWord(ushort addr, int value)
    {
        var b = new byte[2];
        BinaryPrimitives.WriteUInt16BigEndian(b, (ushort)value);
        _p.WriteMulti(addr, b);
    }

    public void WriteDword(ushort addr, uint value)
    {
        var b = new byte[4];
        BinaryPrimitives.WriteUInt32BigEndian(b, value);
        _p.WriteMulti(addr, b);
    }

    // ── identity and init ────────────────────────────────────────────────────

    public int GetSensorId() => ReadWord(Vl53l4Uld.IdentificationModelId);

    public bool IsAlive() => GetSensorId() == Vl53l4Uld.ModelId;

    /// <exception cref="Vl53l4Exception">FIRMWARE__SYSTEM_STATUS never read 0x03.</exception>
    public void WaitBoot(int timeoutMs = 1000)
    {
        for (int i = 0; i < Math.Max(1, timeoutMs); i++)
        {
            if (ReadByte(Vl53l4Uld.FirmwareSystemStatus) == 0x03)
                return;
            _p.SleepMs(1);
        }
        throw new Vl53l4Exception("timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03");
    }

    /// <summary>Initialise the sensor and leave the bus at <paramref name="busKhz"/>. The
    /// configuration block goes at 400 kHz, the only speed an unconfigured sensor is specified
    /// for; the bridge is re-timed after it.</summary>
    public void SensorInit(int busKhz = Vl53l4Uld.I2cKhzDefault)
    {
        _p.SetI2cSpeed(Vl53l4Uld.I2cKhzBoot);
        WaitBoot();
        // The C driver writes the 91 bytes one register at a time; auto-increment makes it one write.
        _p.WriteMulti(Vl53l4Uld.ConfigAddr, Vl53l4Uld.ConfigBlock());
        if (busKhz != Vl53l4Uld.I2cKhzBoot)
            _p.SetI2cSpeed(busKhz);

        WriteByte(Vl53l4Uld.SystemStart, 0x40); // start VHV
        WaitDataReady();
        ClearInterrupt();
        StopRanging();
        WriteByte(Vl53l4Uld.VhvConfigTimeoutMacropLoopBound, 0x09);
        WriteByte(0x000B, 0x00);
        WriteWord(0x0024, 0x0500);

        SetRangeTiming(50, 0);
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    public void ClearInterrupt() => WriteByte(Vl53l4Uld.SystemInterruptClear, 0x01);

    public void StartRanging()
    {
        // INTERMEASUREMENT 0 = continuous, anything else = autonomous low power.
        int mode = ReadDword(Vl53l4Uld.IntermeasurementMs) == 0 ? 0x21 : 0x40;
        WriteByte(Vl53l4Uld.SystemStart, mode);
    }

    public void StopRanging() => WriteByte(Vl53l4Uld.SystemStart, 0x80);

    public bool CheckForDataReady()
    {
        int intPol = ((ReadByte(Vl53l4Uld.GpioHvMuxCtrl) & 0x10) >> 4) == 1 ? 0 : 1;
        return (ReadByte(Vl53l4Uld.GpioTioHvStatus) & 1) == intPol;
    }

    /// <exception cref="Vl53l4Exception">No data ready within <paramref name="timeoutMs"/> polls.</exception>
    public void WaitDataReady(int timeoutMs = 1000)
    {
        for (int i = 0; i < Math.Max(1, timeoutMs); i++)
        {
            if (CheckForDataReady())
                return;
            _p.SleepMs(1);
        }
        throw new Vl53l4Exception("timeout waiting for data ready");
    }

    /// <summary>One block read instead of the C driver's six register reads; the decoding is identical.</summary>
    public Vl53l4Results GetResult() =>
        Vl53l4Uld.ParseResultBlock(_p.ReadMulti(Vl53l4Uld.ResultBlockAddr, Vl53l4Uld.ResultBlockLen));

    // ── timing ───────────────────────────────────────────────────────────────

    public void SetRangeTiming(int timingBudgetMs, long interMeasurementMs)
    {
        int clockPll = interMeasurementMs > 0 ? ReadWord(Vl53l4Uld.ResultOscCalibrateVal) : 0;
        var (a, b, interRaw) = Vl53l4Uld.RangeTimingRegisters(
            timingBudgetMs, interMeasurementMs, ReadWord(Vl53l4Uld.OscFrequency), clockPll);
        WriteDword(Vl53l4Uld.IntermeasurementMs, interRaw);
        WriteWord(Vl53l4Uld.RangeConfigA, a);
        WriteWord(Vl53l4Uld.RangeConfigB, b);
    }

    /// <summary>(timing budget ms, inter-measurement ms; 0 = continuous).</summary>
    public (int TimingBudgetMs, int InterMeasurementMs) GetRangeTiming() =>
        Vl53l4Uld.DecodeRangeTiming(
            ReadDword(Vl53l4Uld.IntermeasurementMs),
            ReadWord(Vl53l4Uld.ResultOscCalibrateVal),
            ReadWord(Vl53l4Uld.OscFrequency),
            ReadWord(Vl53l4Uld.RangeConfigA));

    // ── offset and crosstalk ─────────────────────────────────────────────────

    public void SetOffset(int offsetMm)
    {
        WriteWord(Vl53l4Uld.RangeOffsetMm, Vl53l4Uld.OffsetRaw(offsetMm));
        WriteWord(Vl53l4Uld.InnerOffsetMm, 0);
        WriteWord(Vl53l4Uld.OuterOffsetMm, 0);
    }

    public int GetOffset() => Vl53l4Uld.DecodeOffset((ushort)ReadWord(Vl53l4Uld.RangeOffsetMm));

    public void SetXtalk(int xtalkKcps)
    {
        WriteWord(Vl53l4Uld.XtalkXPlaneGradientKcps, 0x0000);
        WriteWord(Vl53l4Uld.XtalkYPlaneGradientKcps, 0x0000);
        WriteWord(Vl53l4Uld.XtalkPlaneOffsetKcps, Vl53l4Uld.XtalkRaw(xtalkKcps));
    }

    public int GetXtalk() => Vl53l4Uld.DecodeXtalk((ushort)ReadWord(Vl53l4Uld.XtalkPlaneOffsetKcps));

    // ── thresholds ───────────────────────────────────────────────────────────

    /// <summary>The distance-window interrupt; <paramref name="window"/> is one of
    /// <see cref="Vl53l4Uld.WindowBelow"/> … <see cref="Vl53l4Uld.WindowIn"/>.</summary>
    public void SetDetectionThresholds(int distanceLowMm, int distanceHighMm, int window)
    {
        WriteByte(Vl53l4Uld.SystemInterrupt, window);
        WriteWord(Vl53l4Uld.ThreshHigh, distanceHighMm);
        WriteWord(Vl53l4Uld.ThreshLow, distanceLowMm);
    }

    /// <summary>(distance low mm, distance high mm, window).</summary>
    public (int DistanceLowMm, int DistanceHighMm, int Window) GetDetectionThresholds()
    {
        int high = ReadWord(Vl53l4Uld.ThreshHigh);
        int low = ReadWord(Vl53l4Uld.ThreshLow);
        return (low, high, ReadByte(Vl53l4Uld.SystemInterrupt) & 0x07);
    }

    public void SetSignalThreshold(int signalKcps) =>
        WriteWord(Vl53l4Uld.MinCountRateRtnLimitMcps, Vl53l4Uld.SignalThresholdRaw(signalKcps));

    public int GetSignalThreshold() => Vl53l4Uld.DecodeSignalThreshold(ReadWord(Vl53l4Uld.MinCountRateRtnLimitMcps));

    public void SetSigmaThreshold(int sigmaMm) =>
        WriteWord(Vl53l4Uld.RangeConfigSigmaThresh, Vl53l4Uld.SigmaThresholdRaw(sigmaMm));

    public int GetSigmaThreshold() => Vl53l4Uld.DecodeSigmaThreshold(ReadWord(Vl53l4Uld.RangeConfigSigmaThresh));

    // ── temperature and calibration ──────────────────────────────────────────

    /// <summary>Re-run VHV; recommended after a &gt;8 °C ambient change (ST Example_3).</summary>
    public void StartTemperatureUpdate()
    {
        WriteByte(Vl53l4Uld.VhvConfigTimeoutMacropLoopBound, 0x81);
        WriteByte(0x000B, 0x92);
        StartRanging();
        WaitDataReady();
        ClearInterrupt();
        StopRanging();
        WriteByte(Vl53l4Uld.VhvConfigTimeoutMacropLoopBound, 0x09);
        WriteByte(0x000B, 0x00);
    }

    // The ranging loop both calibrations share: data ready, result, clear, n times.
    private void Collect(int nbSamples, Action<int, Vl53l4Results> onSample, int timeoutMs = 5000)
    {
        StartRanging();
        for (int i = 0; i < nbSamples; i++)
        {
            WaitDataReady(timeoutMs);
            var r = GetResult();
            ClearInterrupt();
            onSample(i, r);
        }
        StopRanging();
    }

    /// <summary>Offset calibration against a target at <paramref name="targetDistMm"/> (10–1000);
    /// returns the offset now programmed, mm.</summary>
    public int CalibrateOffset(int targetDistMm, int nbSamples = 20)
    {
        if (nbSamples < 5 || nbSamples > 255 || targetDistMm < 10 || targetDistMm > 1000)
            throw new Vl53l4Exception("nb_samples must be 5..255, target 10..1000 mm");
        WriteWord(Vl53l4Uld.RangeOffsetMm, 0);
        WriteWord(Vl53l4Uld.InnerOffsetMm, 0);
        WriteWord(Vl53l4Uld.OuterOffsetMm, 0);

        Collect(10, (_, _) => { }); // device heat loop

        long sum = 0;
        Collect(nbSamples, (_, r) => sum += r.DistanceMm);

        int offsetMm = targetDistMm - (int)FloorDiv(sum, nbSamples);
        WriteWord(Vl53l4Uld.RangeOffsetMm, Vl53l4Uld.OffsetRaw(offsetMm));
        return offsetMm;
    }

    /// <summary>Crosstalk calibration against a target at <paramref name="targetDistMm"/> (10–5000);
    /// returns the xtalk now programmed, kcps (C rounding).</summary>
    public int CalibrateXtalk(int targetDistMm, int nbSamples = 20)
    {
        if (nbSamples < 5 || nbSamples > 255 || targetDistMm < 10 || targetDistMm > 5000)
            throw new Vl53l4Exception("nb_samples must be 5..255, target 10..5000 mm");
        WriteWord(Vl53l4Uld.XtalkPlaneOffsetKcps, 0); // compensation off

        Collect(10, (_, _) => { }); // device heat loop

        var samples = new List<Vl53l4Results>();
        Collect(nbSamples, (i, r) =>
        {
            if (r.RangeStatus == 0 && i > 0) // invalid measurements and the first frame are dropped
                samples.Add(r);
        });
        if (samples.Count == 0)
            throw new Vl53l4Exception("xtalk calibration failed: no valid samples");

        double n = samples.Count;
        double avgDistance = samples.Sum(s => (double)s.DistanceMm) / n;
        double avgSpadNb = samples.Sum(s => (double)s.NumberOfSpad) / n;
        double avgSignal = samples.Sum(s => (double)s.SignalRateKcps) / n;
        double tmpXtalk = (1.0 - avgDistance / targetDistMm) * (avgSignal / avgSpadNb);
        if (tmpXtalk > 127) // 65536 / 512
            throw new Vl53l4Exception($"xtalk calibration failed: {tmpXtalk:0.0} kcps > 127");

        WriteWord(Vl53l4Uld.XtalkPlaneOffsetKcps, (int)(tmpXtalk * 512.0) & 0xFFFF);
        return (int)Math.Round(tmpXtalk, MidpointRounding.AwayFromZero);
    }

    private static long FloorDiv(long a, long b) => a / b - ((a % b != 0) && ((a < 0) != (b < 0)) ? 1 : 0);
}
