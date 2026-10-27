using System.Buffers.Binary;
using System.Diagnostics;
using Depz.Sensor.Tmf8829;
using Depz.Sensor.Transport;

namespace Depz.Sensor.Devices;

/// <summary>The sensor refused or did not answer (bootloader / application status).</summary>
public sealed class Tmf8829Exception : DepzException
{
    public Tmf8829Exception(string message) : base(message) { }
}

/// <summary>RPT_TMF_EVENT that is not an expected stop artefact: a MOTION / PROXIMITY interrupt (code 0) or
/// a refused frame header (code 1 — the sensor is lost, run <see cref="Tmf8829Device.Init"/> again).</summary>
public sealed record Tmf8829InterruptEvent(Tmf8829Event Event) : DeviceEvent;

/// <summary>The config <see cref="Tmf8829Device.Configure"/> wrote, as read back from the sensor.</summary>
/// <param name="Preset">The preset name.</param>
/// <param name="Page">The 190-byte page 0x22..0xDF.</param>
/// <param name="Histograms">Histogram frames are dumped.</param>
/// <param name="RefFrame">A ref-SPAD frame comes with each measurement.</param>
/// <param name="Fields">Every named field of the page.</param>
public sealed record Tmf8829AppliedConfig(string Preset, byte[] Page, bool Histograms, bool RefFrame,
    IReadOnlyDictionary<string, int> Fields)
{
    /// <summary>(rows, cols) of a measurement.</summary>
    public (int Rows, int Cols) Grid => Tmf8829Frames.FpGrid[Fields["fp_mode"]];
}

/// <summary>
/// TMF8829 multizone dToF (8×8 … 48×32 zones, up to 4 peaks per zone, histograms) over the SPI register
/// bridge (contract 14). The board is a thin bridge: this class runs the sensor. <see cref="Init"/> downloads
/// the 14.4 KB sensor firmware (~0.15 s), <see cref="Configure"/> writes the config page,
/// <see cref="Start"/> streams measurements to <see cref="OnMeasurement"/> / <see cref="Measurements"/>,
/// <see cref="Stop"/> ends it. The sensor's EN pin is wired high: there is no sleep, and recovery from any
/// state is <see cref="Init"/> (it starts with a soft reset).
/// </summary>
public sealed class Tmf8829Device : DepzDevice
{
    // Host-interface registers (sensor bootloader and application).
    private const int RegAppId = 0x00;
    private const int RegCmdStat = 0x08;
    private const int RegSerial = 0x1C;
    private const int RegCidRid = 0x20;
    private const int RegIntStatus = 0xE1;
    private const int RegIntEnab = 0xE2;
    private const int RegId = 0xE3;
    private const int RegRevid = 0xE4;
    private const int RegReset = 0xF7;
    private const int RegEnable = 0xF8;
    private const int RegFifoStatus = 0xFA;
    private const int RegFifo = 0xFF;

    private const int DeviceId = 0x9E;
    private const int AppIdApp = 0x01;
    private const int AppIdBl = 0x80;
    private const int EnableCpuReady = 0x80;
    private const int EnablePuselMask = 0x30;
    private const int EnablePuselBootmon = 0x10;
    private const int EnablePuselRam = 0x20;
    private const int EnablePon = 0x04;
    private const int ResetSoft = 0x40;
    private const int FifoErrorFlags = 0x30;

    private const int BlStartRamApp = 0x16;
    private const int BlI2cOff = 0x22;
    private const int BlWFifoBoth = 0x45;

    private const int CmdMeasure = 0x10;
    private const int CmdWritePage = 0x15;
    private const int CmdLoadConfigPage = 0x16;
    private const int CmdStop = 0xFF;
    private const int StatOk = 0x00;
    private const int StatAccepted = 0x01;

    private static readonly Dictionary<int, string> BlStatusNames = new()
    {
        [0] = "READY", [1] = "ERR_PARAM", [2] = "ERR_ADDR", [4] = "ERR_FIFO",
    };

    private static readonly Dictionary<int, string> StatNames = new()
    {
        [0x00] = "OK", [0x01] = "ACCEPTED", [0x02] = "ERR_CONFIG", [0x03] = "ERR_APPLICATION",
        [0x04] = "ERR_CONFIG_RESULT_SIZE", [0x05] = "ERR_CONFIG_VCSEL", [0x06] = "ERR_WAKEUP_TIMED",
        [0x07] = "ERR_RESET_UNEXPECTED", [0x08] = "ERR_UNKNOWN_CMD", [0x09] = "ERR_UNKNOWN_CID",
        [0x0A] = "ERR_STOP_0", [0x0B] = "ERR_STOP_1", [0x0C] = "ERR_STOP_2", [0x0D] = "ERR_STOP_3",
        [0x0E] = "ERR_OSC_TUNE",
    };

    private static readonly TimeSpan Fast = TimeSpan.FromMilliseconds(100);

    private readonly object _regLock = new();    // the bridge takes one register command at a time
    private readonly object _frameLock = new();  // the reassembler and builder, shared with the reader thread
    private readonly Fanout<Tmf8829Measurement> _measurements = new();
    private readonly Tmf8829FrameReassembler _reassembler = new();
    private readonly Tmf8829MeasurementBuilder _builder = new();
    private volatile bool _streaming;
    private volatile bool _stopping;

    public Tmf8829Device(ILink link, TimeSpan? timeout = null) : base(link, timeout) { }

#if NET
    public Tmf8829Device(string port, TimeSpan? timeout = null) : base(port, timeout) { }
#endif

    internal Tmf8829Device(DepzDevice from) : base(from) { }

    /// <summary>The RAM application version <see cref="Init"/> started ("2.200.0"), or null.</summary>
    public string? AppVersion { get; private set; }

    /// <summary>The sensor's serial number, read by <see cref="Init"/>, or null.</summary>
    public uint? SensorSerial { get; private set; }

    /// <summary>The last <see cref="Configure"/> result (null after <see cref="Init"/>).</summary>
    public Tmf8829AppliedConfig? Config { get; private set; }

    public bool Streaming => _streaming;

    // ── register access ──────────────────────────────────────────────────────

    /// <summary>Read <paramref name="length"/> bytes from <paramref name="addr"/> in ≤1024-byte requests; reads
    /// that reach the FIFO port 0xFF stay there (the sensor does not auto-increment it).</summary>
    /// <exception cref="DepzException">A reply shorter or longer than asked.</exception>
    public byte[] ReadRegisters(int addr, int length)
    {
        var output = new List<byte>(Math.Max(0, length));
        lock (_regLock)
        {
            while (length > 0)
            {
                int n = Math.Min(length, Tmf8829Wire.ReadMax);
                var rep = Request((int)Tmf8829Cmd.ReadReg, Tmf8829Wire.PackReadReg(addr, n),
                    ExpectReport((int)Tmf8829Rpt.RegData, p => Tmf8829RegData.Unpack(p)), Tmf8829Wire.RegisterTimeout);
                if (rep.Data.Length != n)
                    throw new DepzException($"READ_REG 0x{addr:X2}: expected {n}, got {rep.Data.Length}");
                output.AddRange(rep.Data);
                addr = Math.Min(addr + n, RegFifo);
                length -= n;
            }
        }
        return output.ToArray();
    }

    public int ReadRegister(int addr) => ReadRegisters(addr, 1)[0];

    /// <summary>Write <paramref name="data"/> from <paramref name="addr"/> in ≤1023-byte requests (writes to
    /// 0xFF stay at 0xFF); <paramref name="progress"/> gets (done, total) after each.</summary>
    public void WriteRegisters(int addr, byte[] data, Action<int, int>? progress = null)
    {
        if (data is null) throw new ArgumentNullException(nameof(data));
        int done = 0;
        lock (_regLock)
        {
            while (done < data.Length)
            {
                int n = Math.Min(data.Length - done, Tmf8829Wire.WriteMax);
                RequestOk((int)Tmf8829Cmd.WriteReg, Tmf8829Wire.PackWriteReg(addr, data.AsSpan(done, n)), Tmf8829Wire.RegisterTimeout);
                done += n;
                if (addr != RegFifo)
                    addr += n;
                progress?.Invoke(done, data.Length);
            }
        }
    }

    /// <summary>Write one register; the low byte of <paramref name="value"/>.</summary>
    public void WriteRegister(int addr, int value) => WriteRegisters(addr, new[] { (byte)value });

    private int Poll(int addr, Func<int, bool> done, TimeSpan timeout, string what)
    {
        var clock = Stopwatch.StartNew();
        while (true)
        {
            int v = ReadRegister(addr);
            if (done(v))
                return v;
            if (clock.Elapsed > timeout)
                throw new Tmf8829Exception($"{what}: timeout, reg 0x{addr:X2} = 0x{v:X2}");
        }
    }

    /// <summary>Raw host-interface registers, for diagnostics.</summary>
    public IReadOnlyDictionary<string, int> SensorState()
    {
        var regs = new (string Name, int Addr)[]
        {
            ("ID", RegId), ("REVID", RegRevid), ("ENABLE", RegEnable), ("RESET", RegReset), ("APP_ID", RegAppId),
            ("CMD_STAT", RegCmdStat), ("INT_STATUS", RegIntStatus), ("INT_ENAB", RegIntEnab), ("FIFOSTATUS", RegFifoStatus),
        };
        var state = new Dictionary<string, int>();
        foreach (var (name, addr) in regs)
            state[name] = ReadRegister(addr);
        return state;
    }

    // ── bridge ───────────────────────────────────────────────────────────────

    /// <summary>Bridge counters (never touches the sensor, safe mid-stream).</summary>
    public Tmf8829Info GetBridgeInfo() =>
        Request((int)Tmf8829Cmd.GetInfo, null, ExpectReport((int)Tmf8829Rpt.Info, p => Tmf8829Info.Unpack(p)));

    // ── init: sensor bootloader + firmware download ──────────────────────────

    private void BlCmd(int cmd, byte[]? parameters = null, TimeSpan? timeout = null)
    {
        byte[] payload = parameters is null || parameters.Length == 0
            ? new[] { (byte)cmd }
            : new[] { (byte)cmd, (byte)parameters.Length }.Concat(parameters).ToArray();
        WriteRegisters(RegCmdStat, payload);
        int st = Poll(RegCmdStat, v => v != cmd, timeout ?? Fast, $"bootloader cmd 0x{cmd:X2}");
        if (st != 0)
            throw new Tmf8829Exception($"bootloader cmd 0x{cmd:X2} -> {(BlStatusNames.TryGetValue(st, out var s) ? s : $"0x{st:x}")}");
    }

    /// <summary>Bring the sensor from any state to its running RAM application: soft reset, bootloader,
    /// firmware download, start. Returns (application version, sensor serial). Stops a running stream first.
    /// <paramref name="progress"/> (done, total) tracks the image download.</summary>
    public (string AppVersion, uint SensorSerial) Init(Action<int, int>? progress = null)
    {
        if (_streaming)
            Stop();
        lock (_regLock)
        {
            WriteRegister(RegEnable, EnablePon | EnablePuselBootmon);
            Thread.Sleep(3);
            WriteRegister(RegReset, ResetSoft);
            Thread.Sleep(3);
            int dev = ReadRegister(RegId);
            if (dev != DeviceId)
                throw new Tmf8829Exception($"sensor ID 0x{dev:X2}, expected 0x{DeviceId:X2} (no TMF8829 on the bus?)");
            Poll(RegAppId, v => v == AppIdBl, Fast, "sensor bootloader");
            BlCmd(BlI2cOff);
            // ENABLE reads 0 over SPI until the sensor's I2C is off.
            Poll(RegEnable, v => (v & EnableCpuReady) != 0, Fast, "cpu_ready");
            byte[] image = Tmf8829Firmware.Image();
            int pad = (4 - image.Length % 4) % 4;
            if (pad > 0)
                image = image.Concat(Enumerable.Repeat((byte)0xFF, pad)).ToArray();
            var parameters = new byte[6];
            BinaryPrimitives.WriteUInt32LittleEndian(parameters, Tmf8829Firmware.LoadAddr);
            BinaryPrimitives.WriteUInt16LittleEndian(parameters.AsSpan(4), (ushort)(image.Length / 4));
            BlCmd(BlWFifoBoth, parameters);
            WriteRegisters(RegFifo, image, progress);
            BlCmd(BlStartRamApp);
            Poll(RegAppId, v => v == AppIdApp, Fast, "START_RAM_APP");
            int en = ReadRegister(RegEnable); // wake-ups must boot the RAM application
            WriteRegister(RegEnable, (en & ~EnablePuselMask) | EnablePuselRam);
            byte[] ver = ReadRegisters(RegAppId, 4);
            AppVersion = $"{ver[1]}.{ver[2]}.{ver[3]}";
            SensorSerial = BinaryPrimitives.ReadUInt32LittleEndian(ReadRegisters(RegSerial, 4));
        }
        Config = null;
        return (AppVersion, SensorSerial.Value);
    }

    // ── application commands, config page ────────────────────────────────────

    internal int AppCmd(int cmd, int[]? accept = null, TimeSpan? timeout = null)
    {
        WriteRegister(RegCmdStat, cmd);
        int st = Poll(RegCmdStat, v => v < 0x10, timeout ?? TimeSpan.FromSeconds(1), $"sensor cmd 0x{cmd:X2}");
        if (!(accept ?? new[] { StatOk }).Contains(st))
            throw new Tmf8829Exception($"sensor cmd 0x{cmd:X2} -> {(StatNames.TryGetValue(st, out var s) ? s : $"0x{st:x}")}");
        return st;
    }

    /// <summary>LOAD_CONFIG_PAGE → the 190-byte page 0x22..0xDF now in the sensor.</summary>
    public byte[] ConfigPage()
    {
        lock (_regLock)
        {
            AppCmd(CmdLoadConfigPage);
            int cid = ReadRegister(RegCidRid);
            if (cid != CmdLoadConfigPage)
                throw new Tmf8829Exception($"CID_RID 0x{cid:X2} after LOAD_CONFIG_PAGE");
            return ReadRegisters(Tmf8829Config.RegCfg, Tmf8829Config.CfgSize);
        }
    }

    /// <summary>
    /// Load <paramref name="preset"/> and write the config page. Every field a preset does not reset is
    /// written (contract 14 §4), so the result never depends on an earlier config; <paramref name="fields"/>
    /// sets any other page field by name. Not while streaming.
    /// </summary>
    /// <exception cref="Tmf8829Exception">The sensor refuses the page (e.g. ERR_CONFIG_RESULT_SIZE: the result
    /// format is too big for the grid) or a written field does not read back.</exception>
    public Tmf8829AppliedConfig Configure(string preset = "8x8", int periodMs = Tmf8829Config.DefaultPeriodMs,
        int? kiloIterations = null, int nrPeaks = 1, bool signalStrength = false, bool noiseStrength = false,
        bool xtalk = false, bool histograms = false, bool refFrame = false, IReadOnlyDictionary<string, int>? fields = null)
    {
        if (preset is null || !Tmf8829Config.Presets.ContainsKey(preset))
            throw new ArgumentException($"unknown preset '{preset}'; one of {string.Join(", ", Tmf8829Config.Presets.Keys)}", nameof(preset));
        if (_streaming)
            throw new InvalidStateException("Configure() while streaming: Stop() first");
        // The 3-bit field takes 0..7, the sensor reports at most 4.
        if (nrPeaks < 1 || nrPeaks > 4)
            throw new ArgumentOutOfRangeException(nameof(nrPeaks), nrPeaks, $"nr_peaks {nrPeaks}, must be 1..4");
        var want = new List<KeyValuePair<string, int>>
        {
            new("period_ms", periodMs),
            new("kilo_iterations", kiloIterations ?? Tmf8829Config.PresetKiloIterations[preset]),
            new("nr_peaks", nrPeaks),
            new("signal_strength", signalStrength ? 1 : 0),
            new("noise_strength", noiseStrength ? 1 : 0),
            new("xtalk", xtalk ? 1 : 0),
            new("dump_histograms", histograms ? 1 : 0),
            new("ref_spad_frame", refFrame ? 1 : 0),
        };
        foreach (var kv in fields ?? new Dictionary<string, int>())
        {
            int i = want.FindIndex(w => w.Key == kv.Key);
            if (i >= 0)
                want[i] = kv;
            else
                want.Add(kv);
        }
        byte[] back;
        lock (_regLock)
        {
            AppCmd(Tmf8829Config.Presets[preset]);
            byte[] page = ConfigPage();
            foreach (var kv in want)
                Tmf8829Config.CfgSet(page, kv.Key, kv.Value);
            WriteRegisters(Tmf8829Config.RegCfg, page);
            AppCmd(CmdWritePage);
            back = ConfigPage();
        }
        var stuck = want.Where(kv => Tmf8829Config.CfgGet(back, kv.Key) != kv.Value).ToList();
        if (stuck.Count > 0)
            throw new Tmf8829Exception("config fields did not stick: "
                + string.Join(", ", stuck.Select(kv => $"{kv.Key} wrote {kv.Value}, reads {Tmf8829Config.CfgGet(back, kv.Key)}")));
        Config = new Tmf8829AppliedConfig(preset, back, histograms, refFrame, Tmf8829Config.ConfigFields(back));
        return Config;
    }

    // ── streaming ────────────────────────────────────────────────────────────

    /// <summary>MEASURE with the bridge stream armed. Configures the default 8x8 preset first when
    /// <see cref="Configure"/> was not called since <see cref="Init"/>.</summary>
    public void Start()
    {
        if (_streaming)
            return;
        var cfg = Config ?? Configure();
        int intMask = Tmf8829Wire.IntResults | (cfg.Histograms ? Tmf8829Wire.IntHistograms : 0);
        lock (_regLock)
        {
            WriteRegister(RegFifoStatus, FifoErrorFlags); // sticky under/overrun
            WriteRegister(RegIntStatus, 0xFF);
            WriteRegister(RegIntEnab, intMask);
            lock (_frameLock)
            {
                _reassembler.Reset();
                _builder.Reset();
            }
            RequestOk((int)Tmf8829Cmd.StartStream, Tmf8829Wire.PackStartStream(intMask, cfg.RefFrame ? Tmf8829Wire.SfRefFrame : 0));
            _streaming = true;
            try
            {
                AppCmd(CmdMeasure, new[] { StatOk, StatAccepted });
            }
            catch
            {
                _streaming = false;
                RequestOk((int)Tmf8829Cmd.StopStream);
                throw;
            }
        }
    }

    /// <summary>Sensor STOP, then the bridge stream (the frames already queued drain). The bad-header event a
    /// STOP raises mid histogram dump is expected and swallowed.</summary>
    public void Stop()
    {
        lock (_regLock)
        {
            _stopping = true;
            try
            {
                AppCmd(CmdStop);
                WriteRegister(RegIntEnab, 0);
                WriteRegister(RegIntStatus, 0xFF);
                if (_streaming)
                    RequestOk((int)Tmf8829Cmd.StopStream);
            }
            finally
            {
                _streaming = false;
                _stopping = false;
            }
        }
    }

    /// <summary>Call <paramref name="callback"/> for every measurement (reader thread; don't block).</summary>
    public IDisposable OnMeasurement(Action<Tmf8829Measurement> callback) => _measurements.OnItem(callback);

    /// <summary>A pull queue of measurements (bounded, drop-oldest); subscribed when this returns.</summary>
    public Subscription<Tmf8829Measurement> Measurements(int capacity = 16) => _measurements.Subscribe(capacity);

    /// <summary>The next measurement (contract 07 §3).</summary>
    public Tmf8829Measurement GetMeasurement(TimeSpan? timeout = null) =>
        _measurements.Next(timeout ?? TimeSpan.FromSeconds(2), (int)Tmf8829Rpt.Frame, () => IsClosed);

    public IReadOnlyList<long> StreamDroppedCounts => _measurements.DroppedCounts;

    /// <summary>Frames the bridge dropped (frame_seq gaps) in this stream.</summary>
    public int FramesDropped
    {
        get { lock (_frameLock) return _reassembler.SeqGaps; }
    }

    /// <summary>Frames lost to reassembly (incomplete, out-of-order chunks) in this stream.</summary>
    public int FramesDiscarded
    {
        get { lock (_frameLock) return _reassembler.Discarded; }
    }

    public int FrameParseErrors
    {
        get { lock (_frameLock) return _builder.ParseErrors; }
    }

    /// <summary>Current sensor-vs-board clock ratio in Q16 (contract 14 §5; nominal 8 &lt;&lt; 16 = 8 µs per
    /// SYSTICK), for diagnostics.</summary>
    public uint ClockRatioQ16
    {
        get { lock (_frameLock) return _builder.Clock.RatioQ16; }
    }

    private protected override void OnClosing()
    {
        if (!_streaming)
            return;
        try
        {
            Stop();
        }
        catch (DepzException)
        {
            // closing anyway
        }
    }

    private protected override bool HandleReport(Packet packet)
    {
        if (packet.Cmd == (int)Tmf8829Rpt.Frame)
        {
            Tmf8829Measurement? m;
            lock (_frameLock)
            {
                Tmf8829FrameChunk chunk;
                try
                {
                    chunk = Tmf8829FrameChunk.Unpack(packet.Payload);
                }
                catch (ArgumentException)
                {
                    return true;
                }
                var raw = _reassembler.Feed(chunk);
                if (raw is null)
                    return true;
                try
                {
                    m = _builder.Add(raw.Data, (long)raw.TimestampUs, (long)raw.EdgeUs);
                }
                catch (ArgumentOutOfRangeException)
                {
                    return true; // a time past 2^63 µs: not a real board
                }
            }
            if (m is not null)
                _measurements.Emit(m, Stats);
            return true;
        }
        if (packet.Cmd == (int)Tmf8829Rpt.Event)
        {
            Tmf8829Event ev;
            try
            {
                ev = Tmf8829Event.Unpack(packet.Payload);
            }
            catch (ArgumentException)
            {
                return true;
            }
            if (ev.Code == Tmf8829EventCode.BadHeader && _stopping)
                return true; // STOP mid histogram dump: expected (contract 14 §2)
            Connection.Events.Emit(new Tmf8829InterruptEvent(ev), Stats);
            return true;
        }
        return false;
    }

    private protected override void OnReaderStopped()
    {
        _measurements.CloseAll();
        base.OnReaderStopped();
    }
}
