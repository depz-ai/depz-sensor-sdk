using System.Diagnostics;
using Depz.Sensor.Bno086;
using Depz.Sensor.Transport;

namespace Depz.Sensor.Devices;

/// <summary>
/// BNO085 / BNO086 9-axis IMU (contract 05). The board only shuttles raw SHTP frames
/// (SEND_SHTP_PACKET / RPT_DATA); the SH-2 hub protocol runs here, ported from the reference SDK.
/// Every send is acknowledged by the bridge at once and every inbound frame arrives as
/// RPT_DATA(cmd 0x00), so replies are correlated at the SH-2 layer only.
/// <code>
/// imu.HardwareReset();
/// using var reports = imu.Reports.Subscribe();
/// imu.Enable(SensorId.RotationVector, hz: 100);
/// </code>
/// Never call a blocking device method (Enable, Tare, …) from a <see cref="Reports"/> callback. A wait for the
/// hub ends with the device: the closed error after <c>Close()</c>, the device-lost error on a lost link, at
/// once, and an answer already queued still wins (contract 07 §3).
/// </summary>
public sealed class Bno086Device : DepzDevice
{
    /// <summary>Granted rates outside [0.9, 2.1] × the requested one are worth a warning, never an error (contract 05 §7).</summary>
    public const double RateLowFactor = 0.9;
    public const double RateHighFactor = 2.1;

    // Re-reads while Enable's verify reads a rate outside the band (contract 05 §7): a stale response of a
    // preceding Disable, or the hub answering before it runs the new rate — interval 0 once or twice on the
    // lab BNO085, and for up to 95 ms (0, or a slower rate) on a hub just reset (bench 2026-11-12). A pause
    // before each: about 0.4 s in all, four times the longest seen.
    private const int VerifyRereads = 15;
    private static readonly TimeSpan VerifyPause = TimeSpan.FromMilliseconds(20);
    private const int ExecutableResetComplete = 0x01;

    private readonly ShtpLayer _shtp = new();
    private readonly object _shtpLock = new();     // RX side (reader thread) + reset
    private readonly object _shtpTxLock = new();   // serialise SEND_SHTP_PACKET
    // Guards everything a caller waits for from the reader — the answers queued, a flush done, the reset complete,
    // the end of the device — and is what those waits wait on.
    private readonly object _waitLock = new();
    private readonly Dictionary<int, List<Queue<byte[]>>> _controlWaiters = new();
    private readonly Dictionary<int, List<FlushWait>> _flushWaiters = new();
    private bool _resetComplete;
    private DepzException? _waitsEnded;
    private readonly List<byte> _advertisement = new();
    private readonly Dictionary<int, FeatureResponse> _features = new();
    private int _cmdSeq;

    public Bno086Device(ILink link, TimeSpan? timeout = null) : base(link, timeout)
    {
        Reports = Stream<BnoReport>((int)Bno086Rpt.Data, defaultCapacity: 1024);
        WhenReaderEnds(EndWaits);
    }

    internal Bno086Device(DepzDevice from) : base(from)
    {
        Reports = Stream<BnoReport>((int)Bno086Rpt.Data, defaultCapacity: 1024);
        WhenReaderEnds(EndWaits);
    }

    private sealed class FlushWait
    {
        public bool Done;
    }

    /// <summary>The reports of every enabled sensor (contract 07 §3): <c>Reports.On(cb)</c>,
    /// <c>Reports.Subscribe()</c> (1024 by default), <c>Reports.Next()</c>; a <c>where</c> predicate keeps the
    /// sensors wanted (<c>r => r.SensorIdValue == (int)SensorId.Accelerometer</c>). <c>ParseErrors</c> counts
    /// RPT_DATA reports shorter than their 9-byte header, dropped (contract 07 §2).</summary>
    public SensorStream<BnoReport> Reports { get; }

    /// <summary>SEND_SHTP_PACKET attempts on ERR_BUSY before giving up.</summary>
    public int BusyRetries { get; set; } = 5;

    /// <summary>The back-off between busy retries (at least 200 ms per the bridge spec).</summary>
    public TimeSpan BusyBackoff { get; set; } = Bno086Data.BusyBackoff;

    /// <summary>The raw SHTP channel-0 advertisement seen since open or reset.</summary>
    public byte[] Advertisement
    {
        get { lock (_advertisement) return _advertisement.ToArray(); }
    }

    // ── RX path ──────────────────────────────────────────────────────────────

    private protected override bool HandleReport(Packet packet)
    {
        if (packet.Cmd != (int)Bno086Rpt.Data)
            return false;
        Bno086Data data;
        try
        {
            // short header: refused and counted — contract 07 §2
            data = Bno086Data.Unpack(packet.Payload);
        }
        catch (ArgumentException)
        {
            Reports.CountParseError();
            return true;
        }
        ShtpCargo? cargo;
        lock (_shtpLock)
            cargo = _shtp.Feed(data.Shtp, (long)data.TimestampUs);
        if (cargo != null)
            DispatchCargo(cargo); // the first fragment's capture: 0xFB base deltas refer to it
        return true;
    }

    private void DispatchCargo(ShtpCargo cargo)
    {
        switch ((ShtpChannel)cargo.Channel)
        {
            case ShtpChannel.Command:
                lock (_advertisement)
                    _advertisement.AddRange(cargo.Payload);
                return;
            case ShtpChannel.Executable:
                if (cargo.Payload.Length > 0 && cargo.Payload[0] == ExecutableResetComplete)
                {
                    lock (_waitLock)
                    {
                        _resetComplete = true;
                        Monitor.PulseAll(_waitLock);
                    }
                }
                return;
            case ShtpChannel.Control:
                foreach (var report in Sh2Hub.SplitControlCargo(cargo.Payload))
                {
                    int rid = report[0];
                    if (rid == (int)ControlReport.GetFeatureResponse && report.Length >= 17)
                    {
                        var resp = FeatureResponse.Unpack(report);
                        lock (_features)
                            _features[resp.SensorId] = resp;
                    }
                    lock (_waitLock)
                    {
                        if (_controlWaiters.TryGetValue(rid, out var waiting))
                        {
                            foreach (var w in waiting)
                                w.Enqueue(report);
                            Monitor.PulseAll(_waitLock);
                        }
                    }
                }
                return;
            case ShtpChannel.InputNormal:
            case ShtpChannel.InputWake:
                foreach (var rep in Sh2Reports.ParseInputCargo(cargo.Payload, cargo.CaptureUs))
                    EmitReport(rep);
                return;
            case ShtpChannel.GyroRv:
                if (Sh2Reports.ParseGyroRvCargo(cargo.Payload, cargo.CaptureUs) is { } gyro)
                    EmitReport(gyro);
                return;
        }
    }

    private void EmitReport(BnoReport rep)
    {
        if (rep is FlushCompleted done)
        {
            lock (_waitLock)
            {
                if (_flushWaiters.TryGetValue(done.FlushedSensor, out var waiting))
                {
                    foreach (var w in waiting)
                        w.Done = true;
                    Monitor.PulseAll(_waitLock);
                }
            }
        }
        Reports.Emit(rep);
    }

    // ── TX path ──────────────────────────────────────────────────────────────

    // Frame the payload and push it through SEND_SHTP_PACKET; ERR_BUSY backs off and resends.
    private void SendShtp(ShtpChannel channel, byte[] payload)
    {
        // The TX lock is held from the sequence number to the OK, across the retries (the reference's
        // _send_shtp; bug hunt 2026-11-03 JAVA-11): a sender let in between would put frame n+1 first.
        lock (_shtpTxLock)
        {
            byte[] frame;
            lock (_shtpLock)
                frame = _shtp.NextFrame((int)channel, payload);
            for (int attempt = 0; ; attempt++)
            {
                try
                {
                    RequestOk((int)Bno086Cmd.SendShtpPacket, frame);
                    return;
                }
                catch (BusyException) when (attempt < BusyRetries - 1)
                {
                    Thread.Sleep(BusyBackoff);
                }
            }
        }
    }

    // The device closed or was lost: every wait, running or started later, ends with `cause`.
    private void EndWaits(DepzException cause)
    {
        lock (_waitLock)
        {
            _waitsEnded ??= cause;
            Monitor.PulseAll(_waitLock);
        }
    }

    // Under _waitLock. A fresh exception per waiter: several threads may be released by one end.
    private void ThrowIfEnded()
    {
        if (_waitsEnded is DeviceLostException lost)
            throw new DeviceLostException(lost.Message, lost);
        if (_waitsEnded != null)
            throw new LinkClosedException("device closed while waiting for the hub");
    }

    // Under _waitLock: until `done` holds (true), the deadline passes (false) or the device ends (throws). What
    // `done` already sees wins over both.
    private bool WaitUntil(Func<bool> done, Stopwatch clock, TimeSpan timeout)
    {
        while (true)
        {
            if (done())
                return true;
            ThrowIfEnded();
            if (Waits.IsUnbounded(timeout))
            {
                Monitor.Wait(_waitLock);
                continue;
            }
            var left = timeout - clock.Elapsed;
            if (left <= TimeSpan.Zero)
                return false;
            Monitor.Wait(_waitLock, left);
        }
    }

    private Queue<byte[]> Wait(int rid)
    {
        var q = new Queue<byte[]>();
        lock (_waitLock)
        {
            ThrowIfEnded();
            if (!_controlWaiters.TryGetValue(rid, out var l))
                _controlWaiters[rid] = l = new List<Queue<byte[]>>();
            l.Add(q);
        }
        return q;
    }

    private void Unwait(int rid, Queue<byte[]> q)
    {
        lock (_waitLock)
            _controlWaiters[rid].Remove(q);
    }

    private byte[]? TryNext(Queue<byte[]> q, Stopwatch clock, TimeSpan timeout)
    {
        // One already queued is taken even when the deadline has passed (bug hunt 2026-11-03 JAVA-13).
        lock (_waitLock)
            return WaitUntil(() => q.Count > 0, clock, timeout) ? q.Dequeue() : null;
    }

    private byte[] Next(Queue<byte[]> q, Stopwatch clock, TimeSpan timeout, int rid) =>
        TryNext(q, clock, timeout) ?? throw new DepzTimeoutException(rid, timeout);

    // A control response the hub answered a request with, decoded; one shorter than its fields
    // fails that request with InvalidReplyException (contract 07 §3), never a bare error.
    private static T DecodeReply<T>(int rid, byte[] raw, Func<byte[], T> unpack)
    {
        try
        {
            return unpack(raw);
        }
        catch (ArgumentException bad)
        {
            throw new InvalidReplyException(rid, bad);
        }
    }

    // Send a control payload and wait for the first response of report id `rid` that `pred` accepts.
    private T ControlRequest<T>(byte[] payload, ControlReport rid, Func<byte[], T> parse, Func<T, bool>? pred, TimeSpan timeout)
    {
        var q = Wait((int)rid);
        try
        {
            SendShtp(ShtpChannel.Control, payload);
            var clock = Stopwatch.StartNew();
            while (true)
            {
                T obj = DecodeReply((int)rid, Next(q, clock, timeout, (int)rid), parse);
                if (pred is null || pred(obj))
                    return obj;
            }
        }
        finally
        {
            Unwait((int)rid, q);
        }
    }

    private int NextCmdSeq()
    {
        int seq = _cmdSeq;
        _cmdSeq = (seq + 1) & 0xFF;
        return seq;
    }

    // A Command Request; with a response, correlated on (command, command_seq).
    private CommandResponse? Command(Sh2Command command, byte[] parameters, bool waitResponse, TimeSpan? timeout = null)
    {
        int seq = NextCmdSeq();
        byte[] payload = Sh2Control.Command(seq, (int)command, parameters);
        if (!waitResponse)
        {
            SendShtp(ShtpChannel.Control, payload);
            return null;
        }
        return ControlRequest(payload, ControlReport.CommandResponse, p => CommandResponse.Unpack(p),
            r => r.Command == (int)command && r.CommandSeq == seq, timeout ?? TimeSpan.FromSeconds(1));
    }

    // Gather every Command Response of one request until `collect` says done (Errors, Counts).
    private List<CommandResponse> CommandCollect(Sh2Command command, byte[] parameters,
        Func<CommandResponse, List<CommandResponse>, bool> collect, TimeSpan timeout)
    {
        int seq = NextCmdSeq();
        byte[] payload = Sh2Control.Command(seq, (int)command, parameters);
        const int rid = (int)ControlReport.CommandResponse;
        var q = Wait(rid);
        var results = new List<CommandResponse>();
        try
        {
            SendShtp(ShtpChannel.Control, payload);
            var clock = Stopwatch.StartNew();
            while (true)
            {
                var resp = DecodeReply(rid, Next(q, clock, timeout, rid), p => CommandResponse.Unpack(p));
                if (resp.Command != (int)command || resp.CommandSeq != seq)
                    continue;
                if (collect(resp, results))
                    return results;
            }
        }
        finally
        {
            Unwait(rid, q);
        }
    }

    private void ForgetHubState()
    {
        lock (_waitLock)
            _resetComplete = false;
        lock (_shtpLock)
            _shtp.Reset();
        lock (_advertisement)
            _advertisement.Clear();
        lock (_features)
            _features.Clear();
    }

    // ── lifecycle ────────────────────────────────────────────────────────────

    /// <summary>Hard-reset the sensor through nRST. The bridge's OK confirms it; the SH-2
    /// executable reset-complete is waited for best-effort (firmware ≤ 0.95 never sent it,
    /// ERRATA E9). SHTP state and cached features restart from zero.</summary>
    public void HardwareReset(TimeSpan? timeout = null)
    {
        var t = timeout ?? TimeSpan.FromSeconds(2);
        ForgetHubState();
        RequestOk((int)Bno086Cmd.SensorReset, null, t);
        var bestEffort = t < TimeSpan.FromMilliseconds(500) && t != System.Threading.Timeout.InfiniteTimeSpan ? t : TimeSpan.FromMilliseconds(500);
        var clock = Stopwatch.StartNew();
        lock (_waitLock)
        {
            // Not an error when it does not come; the device's end wakes this wait early, and the next call says so.
            while (!_resetComplete && _waitsEnded is null && clock.Elapsed < bestEffort)
                Monitor.Wait(_waitLock, bestEffort - clock.Elapsed);
        }
    }

    /// <summary>Pulse WAKE (PS0) — the board's pin, not the SH-2 "On" that ends <see cref="Sleep"/>
    /// (that is <see cref="PowerOn"/>).</summary>
    public void Wake() => RequestOk((int)Bno086Cmd.SensorWakeUp);

    /// <summary>SH-2 "Sleep": every report stops except those enabled with
    /// <c>FeatureFlags.Wakeup | FeatureFlags.AlwaysOn</c>; the configuration is kept. No response.</summary>
    public void Sleep() => SendShtp(ShtpChannel.Executable, Sh2Control.Sleep());

    /// <summary>SH-2 "On": ends <see cref="Sleep"/>; every enabled report comes back. No response.</summary>
    public void PowerOn() => SendShtp(ShtpChannel.Executable, Sh2Control.PowerOn());

    // ── identification and features ──────────────────────────────────────────

    /// <summary>The SH-2 application's Product ID — the first of <see cref="ProductIds"/>.</summary>
    public ProductId ProductId(TimeSpan? timeout = null) => ProductIds(timeout)[0];

    /// <summary>Every component's Product ID Response, the SH-2 application first. Collects until no
    /// response came for <paramref name="settle"/> (default 150 ms), so late ones cannot answer the next request.</summary>
    public List<ProductId> ProductIds(TimeSpan? timeout = null, TimeSpan? settle = null)
    {
        const int rid = (int)ControlReport.ProductIdResponse;
        var q = Wait(rid);
        try
        {
            SendShtp(ShtpChannel.Control, Sh2Control.ProductId());
            var t = timeout ?? TimeSpan.FromSeconds(1);
            byte[] first = Next(q, Stopwatch.StartNew(), t, rid);
            var output = new List<ProductId> { DecodeReply(rid, first, p => Bno086.ProductId.Unpack(p)) };
            var quiet = settle ?? TimeSpan.FromMilliseconds(150);
            while (TryNext(q, Stopwatch.StartNew(), quiet) is { } more)
                output.Add(DecodeReply(rid, more, p => Bno086.ProductId.Unpack(p)));
            return output;
        }
        finally
        {
            Unwait(rid, q);
        }
    }

    /// <summary>
    /// Enable <paramref name="sensor"/> through Set Feature (0xFD), at <paramref name="hz"/> or every
    /// <paramref name="intervalUs"/> (give exactly one). The hub rounds to its grid; with
    /// <paramref name="verify"/> the granted rate is read back through Get Feature and a rate outside
    /// 0.9–2.1 × the request is traced as a warning (contract 05 §7). Returns that read-back (null
    /// without verify).
    /// </summary>
    public FeatureResponse? Enable(int sensor, double? hz = null, uint? intervalUs = null, uint batchUs = 0,
        int sensitivity = 0, int flags = 0, uint cfgWord = 0, bool verify = true, TimeSpan? timeout = null)
    {
        if (hz.HasValue == intervalUs.HasValue)
            throw new ArgumentException("give exactly one of hz / intervalUs");
        if (hz is double h)
        {
            if (!(h > 0))
                throw new ArgumentOutOfRangeException(nameof(hz), h, "hz must be positive; use Disable()");
            double us = Math.Max(1, Math.Round(1_000_000 / h, MidpointRounding.ToEven)); // Python round()
            if (us > uint.MaxValue)
                throw new ArgumentOutOfRangeException(nameof(hz), h, $"rate {h} Hz: the report interval {us} µs does not fit the u32 wire field");
            intervalUs = (uint)us;
        }
        uint interval = intervalUs!.Value;
        SendShtp(ShtpChannel.Control, Sh2Control.SetFeature(sensor, interval, batchUs, sensitivity, flags, cfgWord));
        if (!verify)
            return null;
        var resp = VerifyFeature(sensor, interval, timeout ?? TimeSpan.FromSeconds(1));
        if (!RateInBand(interval, resp.IntervalUs))
        {
            double requested = 1_000_000.0 / interval;
            double actual = resp.IntervalUs != 0 ? 1_000_000.0 / resp.IntervalUs : 0.0;
            Trace.TraceWarning($"BNO086 sensor 0x{sensor:X2}: requested {requested:0.0} Hz, granted {actual:0.0} Hz "
                               + $"(outside {RateLowFactor}–{RateHighFactor}× band)");
        }
        return resp;
    }

    // Whether a granted interval is the rate asked for, within the band of contract 05 §7 (0 = disabled: never).
    private static bool RateInBand(uint requestedUs, uint grantedUs)
    {
        double requested = 1_000_000.0 / requestedUs;
        double actual = grantedUs != 0 ? 1_000_000.0 / grantedUs : 0.0;
        return RateLowFactor * requested <= actual && actual <= RateHighFactor * requested;
    }

    // Read back a feature just enabled until the hub runs it: an answer outside the band is stale, early or
    // a refusal (see VerifyRereads).
    private FeatureResponse VerifyFeature(int sensor, uint intervalUs, TimeSpan timeout)
    {
        var resp = GetFeature(sensor, timeout);
        for (int i = 0; i < VerifyRereads && !RateInBand(intervalUs, resp.IntervalUs); i++)
        {
            Thread.Sleep(VerifyPause);
            resp = GetFeature(sensor, timeout);
        }
        return resp;
    }

    /// <summary>Disable <paramref name="sensor"/> (Set Feature with interval 0).</summary>
    public void Disable(int sensor)
    {
        SendShtp(ShtpChannel.Control, Sh2Control.SetFeature(sensor, 0));
        lock (_features)
            _features.Remove(sensor);
    }

    /// <summary>Get Feature round trip: the rates in effect for <paramref name="sensor"/>.</summary>
    public FeatureResponse GetFeature(int sensor, TimeSpan? timeout = null) =>
        ControlRequest(Sh2Control.GetFeature(sensor), ControlReport.GetFeatureResponse, p => FeatureResponse.Unpack(p),
            r => r.SensorId == sensor, timeout ?? TimeSpan.FromSeconds(1));

    // ── tare and calibration ─────────────────────────────────────────────────

    /// <summary>Tare <paramref name="axes"/> against <paramref name="basis"/> (no response).</summary>
    public void TareNow(TareAxis axes = TareAxis.All, TareBasis basis = TareBasis.RotationVector) =>
        Command(Sh2Command.Tare, Sh2Hub.TareNowParams(axes, basis), waitResponse: false);

    /// <summary>Persist the current tare into FRS (no response).</summary>
    public void PersistTare() => Command(Sh2Command.Tare, Sh2Hub.PersistTareParams(), waitResponse: false);

    /// <summary>The runtime reorientation quaternion (Q14 on the wire; all zeros clears; no response). It
    /// is applied on the hub's side, in the hub's own axes: x and y swapped and z negated relative to the
    /// reports.</summary>
    public void SetReorientation(double x, double y, double z, double w) =>
        Command(Sh2Command.Tare, Sh2Hub.SetReorientationParams(x, y, z, w), waitResponse: false);

    /// <summary>Configure the motion-engine calibration; <see cref="Sh2Exception"/> on a non-zero status.</summary>
    public void SetCalibration(bool accel = true, bool gyro = true, bool mag = true, bool planar = false, TimeSpan? timeout = null)
    {
        var resp = Command(Sh2Command.MeCalibrate, Sh2Hub.MeCalibrationParams(accel, gyro, mag, planar), true, timeout)!;
        if (resp.Status != 0)
            throw new Sh2Exception($"ME calibration configure failed: status {resp.Status}");
    }

    /// <summary>Which motion-engine calibrations run.</summary>
    public CalibrationConfig GetCalibration(TimeSpan? timeout = null)
    {
        var resp = Command(Sh2Command.MeCalibrate, Sh2Hub.MeCalibrationParams(false, false, false, subcommand: Sh2Hub.MeCalGet), true, timeout)!;
        if (resp.Status != 0)
            throw new Sh2Exception($"ME calibration get failed: status {resp.Status}");
        return new CalibrationConfig(resp.R[1] != 0, resp.R[2] != 0, resp.R[3] != 0, resp.R[4] != 0);
    }

    /// <summary>Save the dynamic calibration data to flash (DCD Save Now).</summary>
    public void SaveDcd(TimeSpan? timeout = null)
    {
        var resp = Command(Sh2Command.SaveDcd, Array.Empty<byte>(), true, timeout)!;
        if (resp.Status != 0)
            throw new Sh2Exception($"DCD save failed: status {resp.Status}");
    }

    /// <summary>The hub's periodic DCD autosave (no response).</summary>
    public void ConfigurePeriodicDcd(bool enable) =>
        Command(Sh2Command.PeriodicDcdConfig, Sh2Hub.PeriodicDcdParams(enable), waitResponse: false);

    /// <summary>Interactive Calibration (0x0E, BNO086 only): how the device is about to move, so the hub
    /// can refine the gyro zero-rate offset while it is still. No response; a BNO085 ignores it. The hub's
    /// answer, if it wants the device to stop, is a <see cref="MotionRequest"/> report.</summary>
    public void SetMotionIntent(MotionIntent intent) =>
        Command(Sh2Command.InteractiveCalibration, Sh2Hub.InteractiveCalibrationParams((int)intent), waitResponse: false);

    /// <summary>Enable MotionRequest (0x2B) reports; the hub sends one when its request changes, and
    /// reads the rate back late, so the granted rate is not checked.</summary>
    public FeatureResponse? EnableMotionRequest(double hz = 1) => Enable((int)SensorId.MotionRequest, hz, verify: false);

    // ── batching ─────────────────────────────────────────────────────────────

    /// <summary>Force Sensor Flush (0xF0): the hub sends <paramref name="sensor"/>'s batched reports now;
    /// returns once its Flush Completed (0xEF) arrives.</summary>
    public void Flush(int sensor, TimeSpan? timeout = null)
    {
        var t = timeout ?? TimeSpan.FromSeconds(1);
        var done = new FlushWait();
        lock (_waitLock)
        {
            ThrowIfEnded();
            if (!_flushWaiters.TryGetValue(sensor, out var l))
                _flushWaiters[sensor] = l = new List<FlushWait>();
            l.Add(done);
        }
        try
        {
            SendShtp(ShtpChannel.Control, Sh2Hub.ForceFlush(sensor));
            var clock = Stopwatch.StartNew();
            lock (_waitLock)
            {
                if (!WaitUntil(() => done.Done, clock, t))
                    throw new DepzTimeoutException(0xEF, t);
            }
        }
        finally
        {
            lock (_waitLock)
                _flushWaiters[sensor].Remove(done);
        }
    }

    // ── FRS ──────────────────────────────────────────────────────────────────

    /// <summary>Read a whole FRS record: its 32-bit words.</summary>
    public uint[] FrsRead(int recordId, TimeSpan? timeout = null)
    {
        var t = timeout ?? TimeSpan.FromSeconds(2);
        var session = new FrsReadSession(recordId);
        const int rid = (int)ControlReport.FrsReadResponse;
        var q = Wait(rid);
        try
        {
            SendShtp(ShtpChannel.Control, session.Request());
            var clock = Stopwatch.StartNew();
            while (!session.Done)
                session.Feed(DecodeReply(rid, Next(q, clock, t, rid), p => FrsReadResponse.Unpack(p)));
        }
        finally
        {
            Unwait(rid, q);
        }
        return session.Words.ToArray();
    }

    /// <summary>Write a whole FRS record; <see cref="Sh2Exception"/> on failure.</summary>
    public void FrsWrite(int recordId, IReadOnlyList<uint> words, TimeSpan? timeout = null)
    {
        var t = timeout ?? TimeSpan.FromSeconds(2);
        var session = new FrsWriteSession(recordId, words);
        const int rid = (int)ControlReport.FrsWriteResponse;
        var q = Wait(rid);
        try
        {
            SendShtp(ShtpChannel.Control, session.Request());
            var clock = Stopwatch.StartNew();
            while (!session.Done)
            {
                byte[]? next = session.Feed(DecodeReply(rid, Next(q, clock, t, rid), p => FrsWriteResponse.Unpack(p)));
                if (next != null)
                    SendShtp(ShtpChannel.Control, next);
            }
        }
        finally
        {
            Unwait(rid, q);
        }
    }

    /// <summary>Read and parse <paramref name="sensor"/>'s metadata FRS record.</summary>
    public SensorMetadata GetMetadata(int sensor, TimeSpan? timeout = null)
    {
        if (!FrsRecordId.MetadataRecords.TryGetValue(sensor, out int record))
            throw new Sh2Exception($"no metadata FRS record known for sensor 0x{sensor:X2}");
        return SensorMetadata.FromWords(FrsRead(record, timeout));
    }

    // ── diagnostics and housekeeping ─────────────────────────────────────────

    /// <summary>Get Oscillator Type (0x0A): R0 is the type (an unknown value comes back as its number).</summary>
    public OscillatorType GetOscillatorType(TimeSpan? timeout = null) =>
        (OscillatorType)Command(Sh2Command.GetOscillatorType, Array.Empty<byte>(), true, timeout)!.R[0];

    /// <summary>Clear the in-RAM dynamic calibration and reset (0x0B). The hub resets instead of
    /// answering, so this waits for the executable reset-complete.</summary>
    public void ClearDcdAndReset(TimeSpan? timeout = null)
    {
        var t = timeout ?? TimeSpan.FromSeconds(2);
        ForgetHubState();
        Command(Sh2Command.ClearDcdAndReset, Array.Empty<byte>(), waitResponse: false);
        var clock = Stopwatch.StartNew();
        lock (_waitLock)
        {
            if (!WaitUntil(() => _resetComplete, clock, t))
                throw new DepzTimeoutException((int)Sh2Command.ClearDcdAndReset, t);
        }
        // The command consumed a host TX seq; the hub restarted its counters on reset. TX only: what the
        // hub sent since the reset is in the reassembly (bug hunt 2026-11-03 JAVA-12).
        lock (_shtpLock)
            _shtp.ResetTx();
    }

    /// <summary>The error queue (0x01), <paramref name="severity"/> or greater; records stream until
    /// one with source 255 (no more).</summary>
    public List<ErrorRecord> GetErrors(int severity = 0, TimeSpan? timeout = null) =>
        CommandCollect(Sh2Command.Errors, Sh2Hub.ErrorsParams(severity), (resp, acc) =>
            {
                if (resp.R[2] == (int)ErrorSource.NoMoreErrors)
                    return true; // the terminator, not a record
                acc.Add(resp);
                return false;
            }, timeout ?? TimeSpan.FromSeconds(1))
            .Select(ErrorRecord.From).ToList();

    /// <summary>A sensor's event counts (0x02): two responses, response_seq 0 then 1.</summary>
    public Counts GetCounts(int sensor, TimeSpan? timeout = null)
    {
        var responses = CommandCollect(Sh2Command.Counter, Sh2Hub.CountsGetParams(sensor), (resp, acc) =>
        {
            acc.Add(resp);
            // both halves, whatever their order: without one the exchange is not
            // complete (a timeout, contract 07 §3 — bug hunt 2026-11-03 PY-6)
            return acc.Any(r => r.ResponseSeq == 0) && acc.Any(r => r.ResponseSeq == 1);
        }, timeout ?? TimeSpan.FromSeconds(1));
        byte[] r0 = responses.First(r => r.ResponseSeq == 0).R;
        byte[] r1 = responses.First(r => r.ResponseSeq == 1).R;
        static uint U32(byte[] b, int at) => System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(b.AsSpan(at));
        return new Counts(sensor, U32(r0, 3), U32(r0, 7), U32(r1, 3), U32(r1, 7));
    }

    /// <summary>Clear a sensor's event counts (0x02, subcommand 1).</summary>
    public void ClearCounts(int sensor, TimeSpan? timeout = null)
    {
        var resp = Command(Sh2Command.Counter, Sh2Hub.CountsClearParams(sensor), true, timeout)!;
        if (resp.Status != 0)
            throw new Sh2Exception($"clear counts failed: status {resp.Status}");
    }
}
