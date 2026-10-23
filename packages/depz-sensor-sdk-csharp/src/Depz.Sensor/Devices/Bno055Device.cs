using System.Diagnostics;
using Depz.Sensor.Bno055;
using Depz.Sensor.Transport;

namespace Depz.Sensor.Devices;

/// <summary>
/// One decoded BNO055 register block (streamed or polled). A channel is null when the block did
/// not cover it; values are scaled by <see cref="Units"/>, the units in effect when the stream
/// started. Euler is (heading, roll, pitch), the quaternion (w, x, y, z); linear acceleration and
/// gravity are always m/s². In non-fusion modes the fusion outputs read zero; in CONFIG everything does.
/// </summary>
public sealed record Bno055Sample(
    ulong TimestampUs,
    int Addr,
    byte[] Raw,
    Bno055Units Units,
    double[]? Accel,
    double[]? Mag,
    double[]? Gyro,
    double[]? Euler,
    double[]? Quaternion,
    double[]? LinearAccel,
    double[]? Gravity,
    double? Temperature,
    Bno055CalibStatus? Calibration)
{
    /// <summary>Decode <paramref name="data"/> read at <paramref name="addr"/> with <paramref name="units"/>.</summary>
    public static Bno055Sample Decode(ulong timestampUs, int addr, byte[] data, Bno055Units units)
    {
        var r = Bno055Regs.DecodeBlock((byte)addr, data);
        static double[]? V(Bno055Vec3? v, double lsb) => v is Bno055Vec3 x ? new[] { x.X / lsb, x.Y / lsb, x.Z / lsb } : null;
        return new Bno055Sample(
            timestampUs, addr, data.ToArray(), units,
            V(r.Accel, units.AccelLsb),
            V(r.Mag, Bno055Regs.MagLsb),
            V(r.Gyro, units.GyroLsb),
            r.Euler is Bno055Euler e ? new[] { e.Heading / units.EulerLsb, e.Roll / units.EulerLsb, e.Pitch / units.EulerLsb } : null,
            r.Quaternion is Bno055Quat q
                ? new[] { q.W / Bno055Regs.QuatLsb, q.X / Bno055Regs.QuatLsb, q.Y / Bno055Regs.QuatLsb, q.Z / Bno055Regs.QuatLsb }
                : null,
            V(r.LinearAccel, Bno055Regs.FusionAccelLsb),
            V(r.Gravity, Bno055Regs.FusionAccelLsb),
            r.Temperature is sbyte t ? t / units.TempLsb : null,
            r.CalibStat is byte c ? Bno055CalibStatus.Unpack(c) : null);
    }
}

/// <summary>
/// BNO055 absolute-orientation IMU (contract 13). The board is a thin I2C register bridge: mode,
/// units, axis remap and calibration are register access here; the sensor fuses on chip at 100 Hz.
/// Typical use: <see cref="Configure"/>, then <see cref="StartStream"/> and read
/// <see cref="Samples"/>, or poll <see cref="ReadSample"/>. Page 1 (sensor configs, interrupts,
/// unique id) is reached through PAGE_ID and refused while streaming.
/// </summary>
public sealed class Bno055Device : DepzDevice
{
    private const int StreamMinBytes = 10;

    // One lock serialises multi-step register sequences (page switch + access, CONFIG round trips).
    private readonly object _regLock = new();
    private readonly Fanout<Bno055Sample> _samples = new();
    private Bno055Units? _units;
    private volatile bool _streaming;
    private volatile Bno055Units _streamUnits = Bno055Units.Default;
    private long _streamParseErrors;
    private (Bno055OprMode Mode, Bno055Units Units, Bno055AxisRemap? Remap, Bno055CalibrationProfile? Calibration)? _configured;
    private Bno055OprMode? _modeBeforeSuspend;

    public Bno055Device(ILink link, TimeSpan? timeout = null) : base(link, timeout) { }

#if NET
    public Bno055Device(string port, TimeSpan? timeout = null) : base(port, timeout) { }
#endif

    internal Bno055Device(DepzDevice from) : base(from) { }

    // ── identity & bridge ────────────────────────────────────────────────────

    /// <summary>RPT_BNO_INFO: chip ids, sensor firmware revision, bridge counters. Safe while streaming.</summary>
    public Bno055Info BridgeInfo() =>
        Request((int)Bno055Cmd.GetInfo, null, ExpectReport((int)Bno055Rpt.Info, p => Bno055Info.Unpack(p)));

    /// <summary>True when the bridge passed the chip-id handshake and the sensor answers with the BNO055 ids.</summary>
    public bool IsAlive()
    {
        try
        {
            var info = BridgeInfo();
            return info.Initialized != 0 && info.IdsOk;
        }
        catch (DepzException)
        {
            return false;
        }
    }

    /// <summary>Hardware reset through nRESET (the bridge answers after the ~0.5 s boot handshake).
    /// Stops a stream; the sensor returns in CONFIG with power-on units and no calibration —
    /// <see cref="Configure"/> again, or <see cref="RestoreConfiguration"/>.</summary>
    public void ResetSensor()
    {
        lock (_regLock)
        {
            RequestOk((int)Bno055Cmd.Reset, null, Bno055Wire.ResetTimeout);
            _streaming = false;
            _units = null;
            WaitBooted();
        }
    }

    // ── raw register access ──────────────────────────────────────────────────

    /// <summary>Read <paramref name="length"/> bytes at <paramref name="addr"/> on <paramref name="page"/>
    /// (128-byte transfers). Page 1 is refused while streaming.</summary>
    public byte[] ReadRegisters(int addr, int length, int page = 0)
    {
        lock (_regLock)
            return OnPage(page, () => Read(addr, length).Data);
    }

    /// <summary>Write at <paramref name="addr"/> on <paramref name="page"/>. Most configuration registers
    /// only take writes in CONFIG mode (the sensor ignores the rest); the typed setters handle that.</summary>
    public void WriteRegisters(int addr, byte[] data, int page = 0)
    {
        lock (_regLock)
            OnPage(page, () =>
            {
                Write(addr, data);
                return 0;
            });
    }

    public int ReadRegister(int addr, int page = 0) => ReadRegisters(addr, 1, page)[0];

    public void WriteRegister(int addr, int value, int page = 0) => WriteRegisters(addr, new[] { (byte)value }, page);

    // ── mode, power, units, axes ─────────────────────────────────────────────

    public Bno055OprMode GetOperationMode() => (Bno055OprMode)(ReadRegister(Bno055Regs.OprMode) & 0x0F);

    /// <summary>Switch OPR_MODE and wait out the switching time; into a fusion mode also wait (up to
    /// 1 s) for the fusion outputs. The sensor only switches between CONFIG and another mode, so a
    /// switch between two operating modes goes through CONFIG.</summary>
    public void SetOperationMode(Bno055OprMode mode)
    {
        CheckEnum(mode, nameof(mode));
        lock (_regLock)
        {
            if (mode != Bno055OprMode.Config)
            {
                var current = CurrentMode();
                if (current == mode)
                    return;
                if (current != Bno055OprMode.Config)
                    SwitchMode(Bno055OprMode.Config);
            }
            SwitchMode(mode);
        }
    }

    public Bno055PwrMode GetPowerMode() => (Bno055PwrMode)(ReadRegister(Bno055Regs.PwrMode) & 0x03);

    /// <summary>Normal / low power / suspend, written in CONFIG with the operating mode restored after.
    /// A suspended chip ignores OPR_MODE writes, so the mode is kept here and put back on wake-up (NDOF
    /// when the chip was already asleep at connect, contract 13 §4.6).</summary>
    public void SetPowerMode(Bno055PwrMode mode)
    {
        CheckEnum(mode, nameof(mode));
        lock (_regLock)
        {
            var opr = CurrentMode();
            int pwr = Read(Bno055Regs.PwrMode, 1).Data[0] & 0x03;
            Bno055OprMode? resume;
            if (opr != Bno055OprMode.Config)
            {
                resume = opr;
                SwitchMode(Bno055OprMode.Config);
            }
            else if (pwr == (int)Bno055PwrMode.Suspend)
            {
                resume = _modeBeforeSuspend ?? Bno055OprMode.Ndof;
            }
            else
            {
                resume = null; // CONFIG on purpose: stay there
            }
            Write(Bno055Regs.PwrMode, new[] { (byte)mode });
            if (mode == Bno055PwrMode.Suspend)
            {
                _modeBeforeSuspend = resume;
            }
            else
            {
                _modeBeforeSuspend = null;
                if (resume is Bno055OprMode r)
                    SwitchMode(r);
            }
        }
    }

    public Bno055Units GetUnits() => _units = Bno055Units.Unpack((byte)ReadRegister(Bno055Regs.UnitSel));

    public void SetUnits(Bno055Units units)
    {
        if (units is null) throw new ArgumentNullException(nameof(units));
        InConfigMode(() =>
        {
            Write(Bno055Regs.UnitSel, new[] { units.Pack() });
            _units = units;
        });
    }

    public Bno055AxisRemap GetAxisRemap()
    {
        byte[] b = ReadRegisters(Bno055Regs.AxisMapConfig, 2);
        return Bno055AxisRemap.Unpack(b[0], b[1]);
    }

    /// <summary>Remap the output axes for the board's mounting (<see cref="Bno055AxisRemap.Placement"/>
    /// gives the datasheet placements P0..P7; P1 is the default).</summary>
    public void SetAxisRemap(Bno055AxisRemap remap)
    {
        if (remap is null) throw new ArgumentNullException(nameof(remap));
        var (config, sign) = remap.Pack();
        InConfigMode(() => Write(Bno055Regs.AxisMapConfig, new[] { config, sign }));
    }

    public Bno055TempSource GetTemperatureSource() => (Bno055TempSource)(ReadRegister(Bno055Regs.TempSource) & 0x03);

    public void SetTemperatureSource(Bno055TempSource source)
    {
        CheckEnum(source, nameof(source));
        InConfigMode(() => Write(Bno055Regs.TempSource, new[] { (byte)source }));
    }

    /// <summary>The usual session setup: CONFIG → units → axis remap → calibration profile →
    /// <paramref name="mode"/>; in a fusion mode it returns once the fusion outputs are live (~70 ms).
    /// Remembered for <see cref="RestoreConfiguration"/>.</summary>
    public void Configure(Bno055OprMode mode = Bno055OprMode.Ndof, Bno055Units? units = null,
        Bno055AxisRemap? axisRemap = null, Bno055CalibrationProfile? calibration = null)
    {
        CheckEnum(mode, nameof(mode));
        units ??= Bno055Units.Default;
        lock (_regLock)
        {
            WaitBooted();
            SwitchMode(Bno055OprMode.Config);
            Write(Bno055Regs.UnitSel, new[] { units.Pack() });
            _units = units;
            if (axisRemap != null)
            {
                var (config, sign) = axisRemap.Pack();
                Write(Bno055Regs.AxisMapConfig, new[] { config, sign });
            }
            if (calibration != null)
                Write(Bno055Regs.CalibProfile, calibration.Pack());
            SwitchMode(mode);
            if (Bno055Regs.IsFusion(mode))
                WaitFusionStarted(strict: true);
        }
        _configured = (mode, units, axisRemap, calibration);
    }

    /// <summary>Re-apply the last <see cref="Configure"/> — after <see cref="ResetSensor"/>, or when
    /// <c>BridgeInfo().SensorResets</c> rose (bus recovery pulses nRESET). A running stream keeps running.</summary>
    public void RestoreConfiguration()
    {
        var c = _configured ?? throw new InvalidOperationException("Configure() has not been called");
        Configure(c.Mode, c.Units, c.Remap, c.Calibration);
    }

    // ── status, self-test, calibration ───────────────────────────────────────

    /// <summary>ST_RESULT, SYS_CLK_STATUS, SYS_STATUS, SYS_ERR.</summary>
    public Bno055SystemStatus SystemStatus()
    {
        lock (_regLock)
        {
            int st = Read(Bno055Regs.StResult, 1).Data[0];
            byte[] b = Read(Bno055Regs.SysClkStatus, 3).Data;
            return new Bno055SystemStatus(st, b[0], b[1], b[2]);
        }
    }

    /// <summary>The built-in self-test (datasheet §3.9.2), in CONFIG, ~400 ms; the operating mode is
    /// restored after. Not while streaming.</summary>
    public Bno055SystemStatus SelfTest()
    {
        RequireNotStreaming();
        lock (_regLock)
        {
            var previous = CurrentMode();
            if (previous != Bno055OprMode.Config)
                SwitchMode(Bno055OprMode.Config);
            try
            {
                int trigger = Read(Bno055Regs.SysTrigger, 1).Data[0] & Bno055Regs.SysTriggerClkSel;
                Write(Bno055Regs.SysTrigger, new[] { (byte)(trigger | Bno055Regs.SysTriggerSelfTest) });
                Thread.Sleep(Bno055Regs.SelfTestTime);
                var status = SystemStatus();
                // Staying in CONFIG would leave SYS_STATUS at 4 for good.
                if (previous == Bno055OprMode.Config)
                    ClearSelfTestStatus();
                return status;
            }
            finally
            {
                if (previous != Bno055OprMode.Config)
                    SwitchMode(previous);
            }
        }
    }

    /// <summary>CALIB_STAT: 0..3 per sensor; in NDOF the sensor calibrates in the background.</summary>
    public Bno055CalibStatus CalibrationStatus() => Bno055CalibStatus.Unpack((byte)ReadRegister(Bno055Regs.CalibStat));

    /// <summary>Offsets and radii (0x55..0x6A), only readable in CONFIG: the driver switches there and back.</summary>
    public Bno055CalibrationProfile ReadCalibrationProfile() =>
        InConfigMode(() => Bno055CalibrationProfile.Unpack(Read(Bno055Regs.CalibProfile, Bno055Regs.CalibProfileLen).Data));

    /// <summary>Restore a stored profile (datasheet §3.11.5). A starting point, not a lock: the background
    /// calibration refines it once fusion resumes.</summary>
    public void WriteCalibrationProfile(Bno055CalibrationProfile profile)
    {
        if (profile is null) throw new ArgumentNullException(nameof(profile));
        InConfigMode(() => Write(Bno055Regs.CalibProfile, profile.Pack()));
    }

    /// <summary>Soft-iron compensation matrix, 9 × i16 row-major, 1.0 = 16384.</summary>
    public short[] GetSicMatrix() => Bno055Regs.UnpackSicMatrix(ReadRegisters(Bno055Regs.SicMatrix, 18));

    public void SetSicMatrix(IReadOnlyList<short>? matrix = null) =>
        InConfigMode(() => Write(Bno055Regs.SicMatrix, Bno055Regs.PackSicMatrix(matrix ?? Bno055Regs.SicIdentity)));

    // ── page 1: raw sensor configuration (non-fusion modes) ──────────────────

    public Bno055AccelConfig GetAccelConfig() => Bno055AccelConfig.Unpack((byte)ReadRegister(Bno055Regs.P1AccConfig, page: 1));

    /// <summary>Accelerometer range / bandwidth / power; fusion modes override it.</summary>
    public void SetAccelConfig(Bno055AccelConfig config) => WritePage1Config(Bno055Regs.P1AccConfig, new[] { config.Pack() });

    public Bno055GyroConfig GetGyroConfig() => Bno055GyroConfig.Unpack(ReadRegisters(Bno055Regs.P1GyrConfig0, 2, page: 1));

    public void SetGyroConfig(Bno055GyroConfig config) => WritePage1Config(Bno055Regs.P1GyrConfig0, config.Pack());

    public Bno055MagConfig GetMagConfig() => Bno055MagConfig.Unpack((byte)ReadRegister(Bno055Regs.P1MagConfig, page: 1));

    public void SetMagConfig(Bno055MagConfig config) => WritePage1Config(Bno055Regs.P1MagConfig, new[] { config.Pack() });

    /// <summary>The chip's 16-byte unique id (page 1, 0x50..0x5F).</summary>
    public byte[] UniqueId() => ReadRegisters(Bno055Regs.P1UniqueId, Bno055Regs.UniqueIdLen, page: 1);

    // ── interrupts (motion only on SW 03.11) ─────────────────────────────────

    public int GetInterruptEnable() => ReadRegister(Bno055Regs.P1IntEn, page: 1);

    /// <summary>INT_EN: which interrupt engines run.</summary>
    public void SetInterruptEnable(int mask) => WriteRegister(Bno055Regs.P1IntEn, CheckByte(mask, nameof(mask)), page: 1);

    public int GetInterruptMask() => ReadRegister(Bno055Regs.P1IntMsk, page: 1);

    /// <summary>INT_MSK: which enabled interrupts drive the INT pin.</summary>
    public void SetInterruptMask(int mask) => WriteRegister(Bno055Regs.P1IntMsk, CheckByte(mask, nameof(mask)), page: 1);

    /// <summary>One motion-interrupt setting register (page 1, 0x11..0x1F), as a raw byte. Not while streaming.</summary>
    public void SetInterruptSetting(int register, int value)
    {
        if (!Bno055Regs.IsInterruptSettingReg(register))
            throw new ArgumentOutOfRangeException(nameof(register), register, $"0x{register:X2} is not a page-1 interrupt setting (0x11..0x1F)");
        WritePage1Config(register, new[] { (byte)CheckByte(value, nameof(value)) });
    }

    /// <summary>INT_STA — which interrupts fired. Clears on read.</summary>
    public int ReadInterruptStatus() => ReadRegister(Bno055Regs.IntSta);

    /// <summary>SYS_TRIGGER RST_INT: reset the interrupt status bits and the INT pin.</summary>
    public void ClearInterrupt()
    {
        lock (_regLock)
        {
            int clk = Read(Bno055Regs.SysTrigger, 1).Data[0] & Bno055Regs.SysTriggerClkSel;
            Write(Bno055Regs.SysTrigger, new[] { (byte)(clk | Bno055Regs.SysTriggerRstInt) });
        }
    }

    // ── data ─────────────────────────────────────────────────────────────────

    /// <summary>Poll one register block (default the full 46 bytes 0x08..0x35) and decode it. Works
    /// alongside a stream.</summary>
    public Bno055Sample ReadSample((byte Addr, byte Length)? block = null)
    {
        var (addr, length) = block ?? (Bno055Regs.FullBlockAddr, Bno055Regs.FullBlockLen);
        lock (_regLock)
        {
            var units = CurrentUnits();
            var (ts, data) = Read(addr, length);
            return Bno055Sample.Decode(ts, addr, data, units);
        }
    }

    /// <summary>(w, x, y, z) — the cheapest orientation read (8 bytes).</summary>
    public double[] ReadQuaternion() => ReadSample((Bno055Regs.QuatBlockAddr, Bno055Regs.QuatBlockLen)).Quaternion!;

    /// <summary>Arm the bridge to read <paramref name="block"/> (default the full block) every
    /// <paramref name="periodMs"/> and push it. Fusion runs at 100 Hz, so 10 ms is the useful floor.
    /// <see cref="Bno055Trigger.Int"/> reads on the INT edge, <paramref name="periodMs"/> then a missed-edge
    /// watchdog (0 = none). Replaces a running stream.</summary>
    public void StartStream(int periodMs = 10, (byte Addr, byte Length)? block = null, Bno055Trigger trigger = Bno055Trigger.Timer)
    {
        var (addr, length) = block ?? (Bno055Regs.FullBlockAddr, Bno055Regs.FullBlockLen);
        if (length < 1 || length > Bno055Wire.XferMax || addr + length > 0x100)
            throw new ArgumentException($"block 0x{addr:X2}+{length} outside 1..{Bno055Wire.XferMax} / page", nameof(block));
        if (periodMs < 0 || periodMs > ushort.MaxValue)
            throw new ArgumentOutOfRangeException(nameof(periodMs), periodMs, "period must be 0..65535 ms");
        CheckEnum(trigger, nameof(trigger));
        lock (_regLock)
        {
            // Units first: the reader may decode the first sample before the request returns.
            _streamUnits = CurrentUnits();
            RequestOk((int)Bno055Cmd.StartStream, Bno055Wire.PackStartStream(trigger, addr, length, (ushort)periodMs));
            _streaming = true;
        }
    }

    public void StopStream()
    {
        if (!_streaming)
            return;
        try
        {
            RequestOk((int)Bno055Cmd.StopStream);
        }
        finally
        {
            _streaming = false;
        }
    }

    public bool Streaming => _streaming;

    /// <summary>Call <paramref name="callback"/> for every streamed sample (reader thread).</summary>
    public IDisposable OnSample(Action<Bno055Sample> callback) => _samples.OnItem(callback);

    /// <summary>A pull queue of streamed samples (bounded, drop-oldest).</summary>
    public Subscription<Bno055Sample> Samples(int capacity = 256) => _samples.Subscribe(capacity);

    /// <summary>The next streamed sample (contract 07 §3).</summary>
    public Bno055Sample GetSample(TimeSpan? timeout = null) =>
        _samples.Next(timeout ?? TimeSpan.FromSeconds(1), (int)Bno055Rpt.Stream, () => IsClosed);

    /// <summary>Stream reports dropped because they did not decode (a short block).</summary>
    public long StreamParseErrors => Interlocked.Read(ref _streamParseErrors);

    public IReadOnlyList<long> StreamDroppedCounts => _samples.DroppedCounts;

    // ── internal ─────────────────────────────────────────────────────────────

    // READ_REG in ≤128-byte pieces on the current page → (MCU timestamp of the last piece, bytes).
    private (ulong Ts, byte[] Data) Read(int addr, int length)
    {
        var output = new byte[length];
        int done = 0;
        ulong ts = 0;
        while (done < length)
        {
            int n = Math.Min(length - done, Bno055Wire.XferMax);
            var rep = Request((int)Bno055Cmd.ReadReg, Bno055Wire.PackReadReg(CheckByte(addr, nameof(addr)), (byte)n),
                ExpectReport((int)Bno055Rpt.RegData, p => Bno055RegData.Unpack(p)));
            if (rep.Data.Length != n)
                throw new DepzException($"READ_REG 0x{addr:X2}: expected {n}, got {rep.Data.Length}");
            ts = rep.TimestampUs;
            Array.Copy(rep.Data, 0, output, done, n);
            addr += n;
            done += n;
        }
        return (ts, output);
    }

    private void Write(int addr, byte[] data)
    {
        int done = 0;
        while (done < data.Length)
        {
            int n = Math.Min(data.Length - done, Bno055Wire.XferMax);
            RequestOk((int)Bno055Cmd.WriteReg, Bno055Wire.PackWriteReg(CheckByte(addr, nameof(addr)), data.AsSpan(done, n)));
            addr += n;
            done += n;
        }
    }

    private static byte CheckByte(int v, string name) =>
        v >= 0 && v <= 0xFF ? (byte)v : throw new ArgumentOutOfRangeException(name, v, $"{name} must be 0..255");

    private static void CheckEnum<T>(T value, string name) where T : struct, Enum
    {
        if (!Enum.IsDefined(typeof(T), value))
            throw new ArgumentOutOfRangeException(name, value, $"not a {typeof(T).Name}");
    }

    private Bno055OprMode CurrentMode() => (Bno055OprMode)(Read(Bno055Regs.OprMode, 1).Data[0] & 0x0F);

    // One OPR_MODE write plus its settle time (no via-CONFIG logic).
    private void SwitchMode(Bno055OprMode mode)
    {
        lock (_regLock)
        {
            Write(Bno055Regs.OprMode, new[] { (byte)mode });
            Thread.Sleep(mode == Bno055OprMode.Config ? Bno055Regs.ModeSwitchToConfig : Bno055Regs.ModeSwitchFromConfig);
            if (Bno055Regs.IsFusion(mode))
                WaitFusionStarted(strict: false);
        }
    }

    private T OnPage<T>(int page, Func<T> body)
    {
        if (page == 0)
            return body();
        if (page != 1)
            throw new ArgumentOutOfRangeException(nameof(page), page, "BNO055 has register pages 0 and 1");
        RequireNotStreaming();
        Write(Bno055Regs.PageId, new byte[] { 0x01 });
        try
        {
            return body();
        }
        finally
        {
            Write(Bno055Regs.PageId, new byte[] { 0x00 });
        }
    }

    // Run a block in CONFIG mode and put the previous mode back.
    private T InConfigMode<T>(Func<T> body)
    {
        lock (_regLock)
        {
            var previous = CurrentMode();
            if (previous != Bno055OprMode.Config)
                SwitchMode(Bno055OprMode.Config);
            try
            {
                return body();
            }
            finally
            {
                if (previous != Bno055OprMode.Config)
                    SwitchMode(previous);
            }
        }
    }

    private void InConfigMode(Action body) => InConfigMode(() =>
    {
        body();
        return 0;
    });

    // A self-test run in CONFIG leaves SYS_STATUS at 4 until the mode leaves CONFIG: ACCONLY and back.
    private void ClearSelfTestStatus()
    {
        SwitchMode(Bno055OprMode.AccOnly);
        SwitchMode(Bno055OprMode.Config);
    }

    // Poll SYS_STATUS until the sensor's own boot (init + POST) is over; a 4 that outlasts POST is a
    // self-test leftover, cleared.
    private void WaitBooted()
    {
        var clock = Stopwatch.StartNew();
        int inSelfTest = 0;
        bool cleared = false;
        while (true)
        {
            int status = Read(Bno055Regs.SysStatus, 1).Data[0];
            if (!Bno055Regs.SysStatusBooting.Contains(status))
                return;
            inSelfTest = status == 4 ? inSelfTest + 1 : 0;
            if (inSelfTest >= Bno055Regs.StuckSelfTestPolls && !cleared)
            {
                ClearSelfTestStatus();
                cleared = true;
                inSelfTest = 0;
                continue;
            }
            if (clock.Elapsed > Bno055Regs.BootSettleTimeout)
                throw new DepzException("BNO055 did not finish booting (SYS_STATUS stuck)");
            Thread.Sleep(5);
        }
    }

    // A running fusion never outputs the all-zero quaternion. Strict: throw on timeout; else give up
    // quietly (a suspended sensor never updates its registers).
    private void WaitFusionStarted(bool strict)
    {
        var clock = Stopwatch.StartNew();
        while (!Read(Bno055Regs.QuaData, 8).Data.Any(b => b != 0))
        {
            if (clock.Elapsed > Bno055Regs.FusionStartTimeout)
            {
                if (strict)
                    throw new DepzException("BNO055 fusion did not start (quaternion stays zero)");
                return;
            }
            Thread.Sleep(10);
        }
    }

    private void WritePage1Config(int addr, byte[] data)
    {
        RequireNotStreaming();
        InConfigMode(() => OnPage(1, () =>
        {
            Write(addr, data);
            return 0;
        }));
    }

    private Bno055Units CurrentUnits() => _units ??= Bno055Units.Unpack(Read(Bno055Regs.UnitSel, 1).Data[0]);

    private void RequireNotStreaming()
    {
        if (_streaming)
            throw new InvalidOperationException("StopStream() first — the stream reads page 0");
    }

    private protected override bool HandleReport(Packet packet)
    {
        if (packet.Cmd != (int)Bno055Rpt.Stream)
            return false;
        Bno055Sample sample;
        try
        {
            if (packet.Payload.Length < StreamMinBytes)
                throw new ArgumentException("short stream report");
            var s = Bno055StreamData.Unpack(packet.Payload); // shorter than its own length field: refused
            sample = Bno055Sample.Decode(s.TimestampUs, s.Addr, s.Data, _streamUnits);
        }
        catch (ArgumentException)
        {
            Interlocked.Increment(ref _streamParseErrors);
            return true;
        }
        _samples.Emit(sample);
        return true;
    }

    private protected override void OnReaderStopped()
    {
        _samples.CloseAll();
        base.OnReaderStopped();
    }
}
