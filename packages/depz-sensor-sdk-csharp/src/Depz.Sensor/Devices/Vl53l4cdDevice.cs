using System.Collections.ObjectModel;
using Depz.Sensor.Protocol;
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l4;

namespace Depz.Sensor.Devices;

/// <summary>One VL53L4CD ranging result (VL53L4CD_ResultsData_t plus the MCU timestamp: of the
/// INT edge when streamed, of the result read when polled).</summary>
public sealed record Vl53l4Measurement(
    ulong TimestampUs,
    int RangeStatus,
    int DistanceMm,
    int SigmaMm,
    int SignalRateKcps,
    int AmbientRateKcps,
    int SignalPerSpadKcps,
    int AmbientPerSpadKcps,
    int NumberOfSpad,
    int StreamCount)
{
    public bool Valid => RangeStatus == 0;

    /// <summary>What <see cref="RangeStatus"/> means (<see cref="Vl53l4cdDevice.RangeStatusNames"/>).</summary>
    public string StatusText =>
        Vl53l4Uld.RangeStatusNames.TryGetValue(RangeStatus, out var s) ? s : $"unknown ({RangeStatus})";

    internal static Vl53l4Measurement From(ulong timestampUs, Vl53l4Results r) =>
        new(timestampUs, r.RangeStatus, r.DistanceMm, r.SigmaMm, r.SignalRateKcps, r.AmbientRateKcps,
            r.SignalPerSpadKcps, r.AmbientPerSpadKcps, r.NumberOfSpad, r.StreamCount);
}

/// <summary>
/// VL53L4CD single-zone ToF (contract 10). The board is a thin I2C bridge; the
/// ST ULD 2.2.3 runs here (<see cref="Uld"/>). <see cref="Init"/> first, then
/// configure and <see cref="StartRanging"/>: one result block per INT edge
/// streams to <see cref="Measurements"/>. Configuration while ranging throws
/// <see cref="InvalidStateException"/> — the stream owns the register bank.
/// </summary>
public sealed class Vl53l4cdDevice : DepzDevice
{
    /// <summary>IDENTIFICATION__MODEL_ID word of VL53L4CD silicon, what <see cref="IsAlive"/> expects.</summary>
    public const int ModelId = Vl53l4Uld.ModelId;

    /// <summary>The bus speed the bridge boots at; <see cref="Init"/> writes the configuration block there.</summary>
    public const int I2cKhzBoot = Vl53l4Uld.I2cKhzBoot;

    /// <summary>The bus speed <see cref="Init"/> leaves by default (the result-block read is ~4× faster than at boot).</summary>
    public const int I2cKhzDefault = Vl53l4Uld.I2cKhzDefault;

    /// <summary>Detection-threshold window modes for <see cref="SetDetectionThresholds"/> (SYSTEM__INTERRUPT).</summary>
    public const int WindowBelow = Vl53l4Uld.WindowBelow;
    public const int WindowAbove = Vl53l4Uld.WindowAbove;
    public const int WindowOut = Vl53l4Uld.WindowOut;
    public const int WindowIn = Vl53l4Uld.WindowIn;

    /// <summary>Range status (after the ST mapping) → its meaning; 0 is a valid measurement.</summary>
    public static IReadOnlyDictionary<int, string> RangeStatusNames { get; } =
        new ReadOnlyDictionary<int, string>(Vl53l4Uld.RangeStatusNames.ToDictionary(kv => kv.Key, kv => kv.Value));

    private static readonly TimeSpan RegTimeout = TimeSpan.FromSeconds(2);

    private readonly BridgePlatform _platform;
    private volatile bool _ranging;

    public Vl53l4cdDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout)
    {
        _platform = new BridgePlatform(this);
        Uld = new Vl53l4cdDriver(_platform);
        Measurements = Stream<Vl53l4Measurement>((int)Vl53l4Rpt.Stream, defaultCapacity: 64);
    }

    internal Vl53l4cdDevice(DepzDevice from) : base(from)
    {
        _platform = new BridgePlatform(this);
        Uld = new Vl53l4cdDriver(_platform);
        Measurements = Stream<Vl53l4Measurement>((int)Vl53l4Rpt.Stream, defaultCapacity: 64);
    }

    /// <summary>The streamed measurements (contract 07 §3): <c>Measurements.On(cb)</c>, <c>Measurements.Subscribe()</c>
    /// (64 by default), <c>Measurements.Next()</c>; <c>ParseErrors</c> counts stream reports whose result block did
    /// not decode, dropped (contract 07 §2).</summary>
    public SensorStream<Vl53l4Measurement> Measurements { get; }

    /// <summary>The ULD driver (escape hatch for raw register access); unstable — not covered by the 1.0 freeze.</summary>
    public Vl53l4cdDriver Uld { get; }

    /// <summary>True after a successful <see cref="Init"/>; cleared by <see cref="Xshut"/> — a
    /// power-cycled sensor holds none of the ULD configuration.</summary>
    public bool Initialized { get; private set; }

    public bool Ranging => _ranging;

    /// <summary>True when the sensor answers with the VL53L4CD model id (0xEBAA).</summary>
    public bool IsAlive()
    {
        try
        {
            return Uld.IsAlive();
        }
        catch (Exception e) when (e is DepzException || e is Vl53l4Exception)
        {
            return false;
        }
    }

    /// <summary>Default configuration block + VHV calibration (ULD sensor_init); the bus is left
    /// at <paramref name="busKhz"/> (one of <see cref="Vl53l4Wire.I2cKhzSteps"/>).</summary>
    public void Init(int busKhz = I2cKhzDefault)
    {
        RequireNotRanging();
        Uld.SensorInit(busKhz);
        Initialized = true;
    }

    // ── sensor power (XSHUT) and the bridge ──────────────────────────────────

    /// <summary>Drive XSHUT: <see cref="Vl53l4Xshut.Off"/> / <see cref="Vl53l4Xshut.On"/> /
    /// <see cref="Vl53l4Xshut.Reset"/>. Off and Reset stop a stream; <see cref="Init"/> again after.</summary>
    public void Xshut(byte action)
    {
        if (action > Vl53l4Xshut.Reset)
            throw new ArgumentOutOfRangeException(nameof(action), action, "XSHUT action must be 0 (off), 1 (on) or 2 (reset)");
        RequestOk((int)Vl53l4Cmd.Xshut, Vl53l4Wire.PackXshut(action), action == Vl53l4Xshut.Reset ? RegTimeout : null);
        _ranging = false;
        Initialized = false;
    }

    /// <summary>Hardware reset through XSHUT (≤ 20 ms on the MCU, typ. ~2 ms); call <see cref="Init"/> again.</summary>
    public void ResetSensor() => Xshut(Vl53l4Xshut.Reset);

    /// <summary>RPT_VL53_INFO: sensor identity, pin levels and bridge counters (I2cErrors is cleared by a successful XSHUT reset).
    /// Safe while streaming.</summary>
    public Vl53l4Info BridgeInfo() =>
        Request((int)Vl53l4Cmd.GetInfo, null, ExpectReport((int)Vl53l4Rpt.Info, p => Vl53l4Info.Unpack(p)));

    /// <summary>Re-time the bridge's I2C bus to the step nearest <paramref name="khz"/>. Not while ranging.</summary>
    public void SetI2cSpeedKhz(int khz)
    {
        RequireNotRanging();
        _platform.SetI2cSpeed(khz);
    }

    // ── configuration (Init first; not while ranging) ───────────────────────

    /// <summary>(timing budget ms, inter-measurement ms; 0 = continuous).</summary>
    public (int TimingBudgetMs, int InterMeasurementMs) GetRangeTiming() => Uld.GetRangeTiming();

    /// <summary>Timing budget 10–200 ms; inter-measurement 0 = continuous, larger than the budget
    /// = autonomous low power.</summary>
    public void SetRangeTiming(int timingBudgetMs, long interMeasurementMs = 0)
    {
        RequireNotRanging();
        Uld.SetRangeTiming(timingBudgetMs, interMeasurementMs);
    }

    public int GetOffsetMm() => Uld.GetOffset();

    public void SetOffsetMm(int offsetMm)
    {
        RequireNotRanging();
        Uld.SetOffset(offsetMm);
    }

    /// <summary>Crosstalk compensation, kcps (0 = off).</summary>
    public int GetXtalkKcps() => Uld.GetXtalk();

    public void SetXtalkKcps(int xtalkKcps)
    {
        RequireNotRanging();
        Uld.SetXtalk(xtalkKcps);
    }

    /// <summary>(distance low mm, distance high mm, window: <see cref="WindowBelow"/> …).</summary>
    public (int DistanceLowMm, int DistanceHighMm, int Window) GetDetectionThresholds() => Uld.GetDetectionThresholds();

    /// <summary>The distance-window interrupt: INT only fires while the window holds. The distances are u16
    /// registers, the window a u8 one: a value outside is refused before anything is written, never
    /// masked (contract 07 §3).</summary>
    public void SetDetectionThresholds(int distanceLowMm, int distanceHighMm, int window)
    {
        Protocol.Wire.CheckUint(nameof(distanceLowMm), distanceLowMm, 16);
        Protocol.Wire.CheckUint(nameof(distanceHighMm), distanceHighMm, 16);
        Protocol.Wire.CheckUint(nameof(window), window, 8);
        RequireNotRanging();
        Uld.SetDetectionThresholds(distanceLowMm, distanceHighMm, window);
    }

    public int GetSignalThresholdKcps() => Uld.GetSignalThreshold();

    /// <summary>Drop measurements whose return signal is below <paramref name="signalKcps"/>.</summary>
    public void SetSignalThresholdKcps(int signalKcps)
    {
        RequireNotRanging();
        Uld.SetSignalThreshold(signalKcps);
    }

    public int GetSigmaThresholdMm() => Uld.GetSigmaThreshold();

    /// <summary>Drop measurements whose sigma exceeds <paramref name="sigmaMm"/> (≤ 16383).</summary>
    public void SetSigmaThresholdMm(int sigmaMm)
    {
        RequireNotRanging();
        Uld.SetSigmaThreshold(sigmaMm);
    }

    /// <summary>Re-run VHV calibration, recommended after a &gt;8 °C ambient change.</summary>
    public void StartTemperatureUpdate()
    {
        RequireNotRanging();
        Uld.StartTemperatureUpdate();
    }

    /// <summary>Offset calibration against a target at <paramref name="targetDistMm"/> (10–1000 mm);
    /// returns the offset now programmed.</summary>
    public int CalibrateOffset(int targetDistMm, int nbSamples = 20)
    {
        RequireNotRanging();
        return Uld.CalibrateOffset(targetDistMm, nbSamples);
    }

    /// <summary>Crosstalk calibration against a target at <paramref name="targetDistMm"/> (10–5000 mm);
    /// returns the xtalk now programmed, kcps.</summary>
    public int CalibrateXtalk(int targetDistMm, int nbSamples = 20)
    {
        RequireNotRanging();
        return Uld.CalibrateXtalk(targetDistMm, nbSamples);
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    /// <summary>Start the sensor and arm the MCU stream: one RPT_VL53_STREAM per INT edge.</summary>
    public void StartRanging()
    {
        RequireNotRanging();
        Uld.StartRanging();
        try
        {
            RequestOk((int)Vl53l4Cmd.StartStream,
                Vl53l4Wire.PackStartStream(Vl53l4Uld.ResultBlockAddr, Vl53l4Uld.ResultBlockLen));
        }
        catch
        {
            // The sensor ranges but the host will say it does not: stop it again, or StopRanging() could
            // never reach it. The START_STREAM error is the one the caller gets, even if this stop fails too.
            try
            {
                Uld.StopRanging();
            }
            catch
            {
                // the original error matters more
            }
            throw;
        }
        _ranging = true;
    }

    public void StopRanging()
    {
        if (!_ranging)
            return;
        // Leave the host state "not ranging" and stop the sensor even when the STOP_STREAM ack
        // fails, or no reconfiguration would ever be possible again.
        try
        {
            RequestOk((int)Vl53l4Cmd.StopStream);
        }
        finally
        {
            _ranging = false;
            Uld.StopRanging();
        }
    }

    /// <summary>One polled measurement: start, wait for data ready, read, stop. Not while streaming.</summary>
    public Vl53l4Measurement MeasureOnce(TimeSpan? timeout = null)
    {
        RequireNotRanging();
        int timeoutMs = Waits.Milliseconds(timeout ?? TimeSpan.FromSeconds(1));
        Uld.StartRanging();
        Vl53l4Results result;
        try
        {
            Uld.WaitDataReady(timeoutMs);
            result = Uld.GetResult();
            Uld.ClearInterrupt();
        }
        finally
        {
            Uld.StopRanging();
        }
        return Vl53l4Measurement.From(_platform.LastTimestampUs, result);
    }

    private void RequireNotRanging()
    {
        RequireOpen();
        if (_ranging)
            throw new InvalidStateException("StopRanging() first — the stream owns the register bank");
    }

    private protected override bool HandleReport(Packet packet)
    {
        if (packet.Cmd != (int)Vl53l4Rpt.Stream)
            return false;
        Vl53l4Measurement m;
        try
        {
            // short header or body: refused and counted — contract 07 §2
            var sample = Vl53l4StreamData.Unpack(packet.Payload);
            m = Vl53l4Measurement.From(sample.TimestampUs, Vl53l4Uld.ParseResultBlock(sample.Data));
        }
        catch (Exception e) when (e is ArgumentException || e is Vl53l4Exception)
        {
            // A stream re-armed on another block, or a corrupt read: count it, keep the reader.
            Measurements.CountParseError();
            return true;
        }
        Measurements.Emit(m);
        return true;
    }

    // The ULD platform on the firmware register bridge.
    private sealed class BridgePlatform : IVl53l4Platform
    {
        private readonly Vl53l4cdDevice _dev;

        public BridgePlatform(Vl53l4cdDevice dev) => _dev = dev;

        /// <summary>MCU timestamp of the latest register read.</summary>
        public ulong LastTimestampUs;

        public byte[] ReadMulti(ushort addr, int size)
        {
            var output = new byte[size];
            int done = 0;
            while (done < size)
            {
                int n = Math.Min(size - done, Vl53l4Wire.XferMax);
                var rep = _dev.Request((int)Vl53l4Cmd.ReadReg, Vl53l4Wire.PackReadReg(addr, (ushort)n),
                    ExpectReport((int)Vl53l4Rpt.RegData, p => Vl53l4RegData.Unpack(p)), RegTimeout);
                if (rep.Data.Length != n)
                    throw new DepzException($"READ_REG 0x{addr:X4}: expected {n}, got {rep.Data.Length}");
                LastTimestampUs = rep.TimestampUs;
                Array.Copy(rep.Data, 0, output, done, n);
                addr = (ushort)(addr + n);
                done += n;
            }
            return output;
        }

        public void WriteMulti(ushort addr, byte[] data)
        {
            int done = 0;
            while (done < data.Length)
            {
                int n = Math.Min(data.Length - done, Vl53l4Wire.XferMax);
                var chunk = new byte[n];
                Array.Copy(data, done, chunk, 0, n);
                _dev.RequestOk((int)Vl53l4Cmd.WriteReg, Vl53l4Wire.PackWriteReg(addr, chunk), RegTimeout);
                addr = (ushort)(addr + n);
                done += n;
            }
        }

        public void SetI2cSpeed(int khz)
        {
            if (khz < 0 || khz > ushort.MaxValue)
                throw new ArgumentOutOfRangeException(nameof(khz), khz, "bus speed must fit u16 kHz");
            _dev.RequestOk((int)Vl53l4Cmd.SetI2cSpeed, Vl53l4Wire.PackSetI2cSpeed((ushort)khz));
        }

        public void SleepMs(int ms) => Thread.Sleep(ms);
    }
}
