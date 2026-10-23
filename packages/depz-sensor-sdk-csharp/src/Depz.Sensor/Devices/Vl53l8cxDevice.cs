using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l8;

namespace Depz.Sensor.Devices;

/// <summary>
/// VL53L8CX multizone ToF (contract 04). The board is a thin SPI bridge; the ST
/// ULD runs here (<see cref="Uld"/>). <see cref="Init"/> downloads the ~84 KB
/// sensor firmware (about a second), then configure and
/// <see cref="StartRanging"/>: the board pushes one frame per INT edge in chunks,
/// reassembled and parsed into <see cref="Frames"/>. Configuration needs
/// <see cref="Init"/> and throws <see cref="InvalidOperationException"/> while
/// ranging — the stream owns the register bank. The VL53L8CH superset is
/// <see cref="Vl53l8chDevice"/>.
/// </summary>
public class Vl53l8cxDevice : DepzDevice
{
    /// <summary>Below this the sensor never enters its ranging loop and streams nothing (contract 04).</summary>
    public const int MinRangingFrequencyHz = 2;

    private static readonly TimeSpan RegTimeout = TimeSpan.FromSeconds(2);
    private const int StreamMinBytes = 12;

    private readonly Fanout<Vl53l8Frame> _frames = new();
    private readonly object _uldLock = new();
    private FrameReassembler _reassembler = new();
    private Vl53l8Driver? _uld;
    private Action<int, int>? _writeProgress;
    private volatile bool _ranging;
    private int _resolution = Vl53l8Driver.Resolution4x4; // the ULD default after init
    private long _frameParseErrors;
    private BridgePlatform? _platform;

    public Vl53l8cxDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout) { }

#if NET
    public Vl53l8cxDevice(string port, TimeSpan? timeout = null) : base(port, timeout) { }
#endif

    internal Vl53l8cxDevice(DepzDevice from) : base(from) { }

    /// <summary>The sensor firmware <see cref="Init"/> downloads.</summary>
    protected virtual Vl53l8Firmware FirmwareSet => Vl53l8Firmware.Cx;

    /// <summary>Register bytes per READ_REG (the I2C L5/L7 board reads less per call).</summary>
    protected virtual int ReadChunk => Vl53l8Wire.ChunkSize;

    /// <summary>Register bytes per WRITE_REG.</summary>
    protected virtual int WriteChunk => Vl53l8Wire.ChunkSize;

    /// <summary>The lowest ranging frequency that streams on this sensor.</summary>
    protected virtual int MinRangingHz => MinRangingFrequencyHz;

    private BridgePlatform Platform => _platform ??= new BridgePlatform(this);

    /// <summary>The ULD driver (escape hatch for DCI access).</summary>
    /// <exception cref="InvalidOperationException">Before <see cref="Init"/>.</exception>
    public Vl53l8Driver Uld => _uld ?? throw new InvalidOperationException("call Init() first");

    /// <summary>The firmware set in use after <see cref="Init"/> (cx / ch / l7cx / l7ch).</summary>
    public Vl53l8Firmware Firmware => Uld.Firmware;

    public bool Ranging => _ranging;

    /// <summary>True when the sensor answers with its own silicon ids (a bus error or a foreign id: false).</summary>
    public bool IsAlive()
    {
        try
        {
            var (deviceId, revisionId) = new Vl53l8Driver(Platform, FirmwareSet).IsAlive();
            return Vl53l8Driver.SiliconIdMatches(deviceId, revisionId,
                FirmwareSet == Vl53l8Firmware.L7cx || FirmwareSet == Vl53l8Firmware.L7ch);
        }
        catch (Exception e) when (e is DepzException || e is Vl53l8Exception)
        {
            return false;
        }
    }

    /// <summary>Download the sensor firmware and the default configuration.
    /// <paramref name="progress"/> gets phase texts, <paramref name="writeProgress"/> (done, total)
    /// the big firmware writes.</summary>
    public virtual void Init(Action<string>? progress = null, Action<int, int>? writeProgress = null)
    {
        RequireNotRanging();
        lock (_uldLock)
        {
            _writeProgress = writeProgress;
            try
            {
                var driver = new Vl53l8Driver(Platform, FirmwareSet);
                driver.Init(progress);
                _uld = driver;
            }
            finally
            {
                _writeProgress = null;
            }
        }
    }

    // ── configuration (Init first; not while ranging) ────────────────────────

    /// <summary>Zones in use: 16 (4×4) or 64 (8×8).</summary>
    public int GetResolution() => _resolution = Uld.GetResolution();

    /// <summary>16 (4×4) or 64 (8×8) zones.</summary>
    public void SetResolution(int zones)
    {
        if (zones != Vl53l8Driver.Resolution4x4 && zones != Vl53l8Driver.Resolution8x8)
            throw new ArgumentException("resolution is 16 (4x4) or 64 (8x8) zones", nameof(zones));
        RequireNotRanging();
        Uld.SetResolution(zones);
        _resolution = zones;
    }

    public int GetRangingFrequencyHz() => Uld.GetRangingFrequencyHz();

    /// <summary>At least 2 Hz; at most 60 Hz at 4×4 and 15 Hz at 8×8.</summary>
    public void SetRangingFrequencyHz(int hz)
    {
        if (hz < MinRangingHz)
            throw new ArgumentOutOfRangeException(nameof(hz), hz,
                $"ranging frequency must be >= {MinRangingHz} Hz: below that the sensor never enters its ranging loop and streams nothing (contract 04)");
        RequireNotRanging();
        Uld.SetRangingFrequencyHz(hz);
    }

    /// <summary><see cref="Vl53l8Driver.RangingModeContinuous"/> or <see cref="Vl53l8Driver.RangingModeAutonomous"/>.</summary>
    public int GetRangingMode() => Uld.GetRangingMode();

    public void SetRangingMode(int mode)
    {
        RequireNotRanging();
        Uld.SetRangingMode(mode);
    }

    public int GetIntegrationTimeMs() => Uld.GetIntegrationTimeMs();

    /// <summary>2–1000 ms; autonomous mode only.</summary>
    public void SetIntegrationTimeMs(int ms)
    {
        RequireNotRanging();
        Uld.SetIntegrationTimeMs(ms);
    }

    public int GetSharpenerPercent() => Uld.GetSharpenerPercent();

    /// <summary>Edge sharpener, 0–99 % (0 = off).</summary>
    public void SetSharpenerPercent(int pct)
    {
        RequireNotRanging();
        Uld.SetSharpenerPercent(pct);
    }

    /// <summary><see cref="Vl53l8Driver.TargetOrderClosest"/> or <see cref="Vl53l8Driver.TargetOrderStrongest"/>.</summary>
    public int GetTargetOrder() => Uld.GetTargetOrder();

    public void SetTargetOrder(int order)
    {
        RequireNotRanging();
        Uld.SetTargetOrder(order);
    }

    // ── advanced features (UM3109) ───────────────────────────────────────────

    /// <summary><see cref="Vl53l8Driver.PowerModeSleep"/>, <see cref="Vl53l8Driver.PowerModeWakeup"/>
    /// or <see cref="Vl53l8Driver.PowerModeDeepSleep"/>.</summary>
    public int GetPowerMode() => Uld.GetPowerMode();

    /// <summary>Waking from deep sleep downloads the firmware again.</summary>
    public void SetPowerMode(int mode)
    {
        RequireNotRanging();
        Uld.SetPowerMode(mode);
    }

    public double GetXtalkMargin() => Uld.GetXtalkMargin();

    public void SetXtalkMargin(double marginKcps)
    {
        RequireNotRanging();
        Uld.SetXtalkMargin(marginKcps);
    }

    /// <summary>On-device crosstalk calibration against a flat target (see
    /// <see cref="Vl53l8Driver.CalibrateXtalk"/>); read the result with <see cref="GetCaldataXtalk"/>.</summary>
    public void CalibrateXtalk(int reflectancePercent, int nbSamples, int distanceMm)
    {
        RequireNotRanging();
        Uld.CalibrateXtalk(reflectancePercent, nbSamples, distanceMm);
    }

    /// <summary>The 776-byte xtalk calibration buffer (save / restore).</summary>
    public byte[] GetCaldataXtalk()
    {
        RequireNotRanging();
        return Uld.GetCaldataXtalk();
    }

    public void SetCaldataXtalk(byte[] blob)
    {
        RequireNotRanging();
        Uld.SetCaldataXtalk(blob);
    }

    public int GetDetectionThresholdsEnable() => Uld.GetDetectionThresholdsEnable();

    public void SetDetectionThresholdsEnable(bool enabled)
    {
        RequireNotRanging();
        Uld.SetDetectionThresholdsEnable(enabled);
    }

    public IReadOnlyList<Vl53l8Advanced.DetectionThreshold> GetDetectionThresholds() => Uld.GetDetectionThresholds();

    /// <summary>The 64 detection thresholds (interrupt on threshold).</summary>
    public void SetDetectionThresholds(IReadOnlyList<Vl53l8Advanced.DetectionThreshold> thresholds)
    {
        RequireNotRanging();
        Uld.SetDetectionThresholds(thresholds);
    }

    public void SetDetectionThresholdsAutoStop(bool autoStop)
    {
        RequireNotRanging();
        Uld.SetDetectionThresholdsAutoStop(autoStop);
    }

    /// <summary>Enable the motion indicator over [min, max] mm; every frame then carries
    /// <see cref="Vl53l8Frame.Motion"/>. Returns the configuration for further tuning.</summary>
    public MotionConfig ConfigureMotionIndicator(int distanceMinMm = 400, int distanceMaxMm = 1500)
    {
        RequireNotRanging();
        var cfg = Uld.MotionIndicatorInit(GetResolution());
        Uld.MotionIndicatorSetDistanceMotion(cfg, distanceMinMm, distanceMaxMm);
        return cfg;
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    /// <summary>The driver after <see cref="Init"/>, or null.</summary>
    private protected Vl53l8Driver? UldOrNull => _uld;

    /// <summary>The CNH setup the CH classes armed, or null.</summary>
    private protected Vl53l8CnhSetup? ArmedCnh { get; private set; }

    // The recorder writes the armed setup next to each raw CNH block.
    internal Vl53l8CnhSetup? CnhSetupArmed => ArmedCnh;

    // CH classes: arm CNH for the next StartRanging (the setup is checked before it is written).
    private protected void ArmCnh(Vl53l8CnhSetup setup)
    {
        if (setup is null) throw new ArgumentNullException(nameof(setup));
        RequireNotRanging();
        setup.RequiredMemory(); // a blank or oversized setup is refused before anything is sent
        Uld.DciWriteData(Vl53l8CnhSetup.MiCfgDevIdx, setup.Pack());
        ArmedCnh = setup;
    }

    // The sensor lost its state (power or reset pin): Init() again.
    private protected void ForgetSensor()
    {
        _ranging = false;
        _uld = null;
    }

    /// <summary>Program the output list, start the sensor and the board's stream.</summary>
    public void StartRanging()
    {
        RequireNotRanging();
        int? cnhSize = ArmedCnh?.RequiredMemory();
        GetResolution(); // refresh the cache that shapes frames
        Uld.StartRanging(cnhSize);
        _reassembler = new FrameReassembler();
        RequestOk((int)Vl53l8Cmd.StartStream, Vl53l8Wire.PackStartStream(Uld.DataReadSize));
        _ranging = true;
    }

    public void StopRanging()
    {
        if (!_ranging)
            return;
        // Leave the host "not ranging" and stop the sensor even when the STOP_STREAM ack fails.
        try
        {
            RequestOk((int)Vl53l8Cmd.StopStream);
        }
        finally
        {
            _ranging = false;
            Uld.StopRanging();
        }
    }

    /// <summary>Call <paramref name="callback"/> for every parsed frame (reader thread).</summary>
    public IDisposable OnFrame(Action<Vl53l8Frame> callback) => _frames.OnItem(callback);

    /// <summary>A pull queue of parsed frames (bounded, drop-oldest).</summary>
    public Subscription<Vl53l8Frame> Frames(int capacity = 8) => _frames.Subscribe(capacity);

    /// <summary>The next frame (contract 07 §3).</summary>
    public Vl53l8Frame GetFrame(TimeSpan? timeout = null) =>
        _frames.Next(timeout ?? TimeSpan.FromSeconds(2), (int)Vl53l8Rpt.Vl53Frame, () => IsClosed);

    /// <summary>Frames dropped because they did not parse (corrupt frame, bad size) — not reassembly gaps.</summary>
    public long FrameParseErrors => Interlocked.Read(ref _frameParseErrors);

    /// <summary>Chunked frames the reassembler discarded (gaps, offset errors).</summary>
    public int ReassemblerDiscards => _reassembler.Discarded;

    public IReadOnlyList<long> StreamDroppedCounts => _frames.DroppedCounts;

    private protected void RequireNotRanging()
    {
        if (_ranging)
            throw new InvalidOperationException("StopRanging() first — the stream owns the register bank");
    }

    private protected override bool HandleReport(Packet packet)
    {
        if (packet.Cmd != (int)Vl53l8Rpt.Vl53Frame || packet.Payload.Length < StreamMinBytes)
            return false;
        var done = _reassembler.Feed(FrameChunk.Unpack(packet.Payload));
        var uld = _uld;
        if (done is not (ulong ts, byte[] raw) || uld is null)
            return true;
        Vl53l8Frame frame;
        try
        {
            frame = uld.ParseFrame(ts, raw) with { Resolution = _resolution };
        }
        catch (Exception e) when (e is Vl53l8FrameDecoder.CorruptedFrameException || e is Vl53l8Exception
                                  || e is IndexOutOfRangeException || e is ArgumentException)
        {
            Interlocked.Increment(ref _frameParseErrors);
            return true;
        }
        _frames.Emit(frame);
        return true;
    }

    private protected override void OnReaderStopped()
    {
        _frames.CloseAll();
        base.OnReaderStopped();
    }

    // The ULD platform on the firmware register bridge.
    private sealed class BridgePlatform : IVl53l8Platform
    {
        private readonly Vl53l8cxDevice _dev;

        public BridgePlatform(Vl53l8cxDevice dev) => _dev = dev;

        public byte[] ReadMulti(ushort addr, int size)
        {
            var output = new byte[size];
            int done = 0;
            int a = addr;
            while (done < size)
            {
                int n = Math.Min(size - done, _dev.ReadChunk);
                var rep = _dev.Request((int)Vl53l8Cmd.ReadReg, Vl53l8Wire.PackReadReg(a, n),
                    ExpectReport((int)Vl53l8Rpt.RegData, p => Vl53l8RegData.Unpack(p)), RegTimeout);
                if (rep.Data.Length != n)
                    throw new DepzException($"READ_REG 0x{a:X4}: expected {n}, got {rep.Data.Length}");
                Array.Copy(rep.Data, 0, output, done, n);
                a += n;
                done += n;
            }
            return output;
        }

        public void WriteMulti(ushort addr, byte[] data)
        {
            int done = 0;
            int a = addr;
            int chunk = _dev.WriteChunk;
            while (done < data.Length)
            {
                int n = Math.Min(data.Length - done, chunk);
                _dev.RequestOk((int)Vl53l8Cmd.WriteReg, Vl53l8Wire.PackWriteReg(a, data.AsSpan(done, n)), RegTimeout);
                a += n;
                done += n;
                if (data.Length > chunk)
                    _dev._writeProgress?.Invoke(done, data.Length);
            }
        }

        public void SleepMs(int ms) => Thread.Sleep(ms);
    }
}

/// <summary>
/// VL53L8CH: the VL53L8CX superset (every CX method) plus CNH, compact network
/// histograms — each frame can carry a per-aggregate distance histogram.
/// <see cref="Vl53l8cxDevice.Init"/> downloads the CH firmware (VL53LMZ ULD 2.0.16).
/// </summary>
public class Vl53l8chDevice : Vl53l8cxDevice
{
    public Vl53l8chDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout) { }

#if NET
    public Vl53l8chDevice(string port, TimeSpan? timeout = null) : base(port, timeout) { }
#endif

    internal Vl53l8chDevice(DepzDevice from) : base(from) { }

    protected override Vl53l8Firmware FirmwareSet => Vl53l8Firmware.Ch;

    /// <summary>The CNH setup armed by <see cref="ConfigureCnh"/>, or null; recorders keep it next
    /// to the raw blocks to decode them later.</summary>
    public Vl53l8CnhSetup? CnhSetup => ArmedCnh;

    /// <summary>Arm CNH for the next <see cref="Vl53l8cxDevice.StartRanging"/>: every frame then carries
    /// <see cref="Vl53l8Frame.CnhRaw"/>, decoded with <see cref="Vl53l8Cnh.DecodeHistogram"/> and
    /// <see cref="Vl53l8CnhSetup.ToDecodeConfig"/>.</summary>
    public void ConfigureCnh(Vl53l8CnhSetup setup) => ArmCnh(setup);
}
