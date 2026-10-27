using System.Diagnostics;
using Depz.Sensor.Protocol;
using Depz.Sensor.Transport;

namespace Depz.Sensor.Devices;

// ── events (contract 07 §2) ─────────────────────────────────────────────────

/// <summary>Base of the unsolicited / diagnostic events a device emits.</summary>
public abstract record DeviceEvent;

/// <summary>The board saw a gap in host TX (RPT_SEQUENCE_ERROR); <c>ReportedByDevice</c> is
/// always true.</summary>
public sealed record SequenceErrorEvent(int ExpectedSeq, int ReceivedSeq, bool ReportedByDevice) : DeviceEvent;

/// <summary>The host saw a gap in the board's packets (contract 01 §5 step 5): the packet still
/// arrives, <c>Missed</c> = (received − expected) mod 256 did not. Counted in
/// <c>LinkStats.SeqGaps</c>.</summary>
public sealed record SeqGapEvent(int ExpectedSeq, int ReceivedSeq, int Missed) : DeviceEvent;

/// <summary>A packet whose payload CRC did not check.</summary>
public sealed record LinkCrcErrorEvent(int Cmd, int Seq) : DeviceEvent;

/// <summary>Bytes the parser skipped while looking for a packet header.</summary>
public sealed record TrashEvent(byte[] Data) : DeviceEvent;

/// <summary>An RPT_STATUS no request was waiting for (echoed cmd 0x00, or nobody in flight).</summary>
public sealed record UnsolicitedStatusEvent(int Status) : DeviceEvent
{
    public bool IsHardwareFault => Status == (int)Protocol.Status.ErrHardwareFault;
}

/// <summary>RPT_TEXT nobody asked for, or a report no class routes (its payload as hex).</summary>
public sealed record TextEvent(int Cmd, string Text) : DeviceEvent;

/// <summary>An unsolicited MCU temperature report.</summary>
public sealed record TemperatureEvent(ulong TimestampUs, double Celsius) : DeviceEvent;

/// <summary>The connection dropped (unplug, I/O error); the device is unusable from here on.</summary>
public sealed record DisconnectedEvent(string Reason) : DeviceEvent;

/// <summary>Result of <see cref="DepzDevice.SyncTime"/>: device clock − host clock, the
/// round trip of the kept sample, and when (host µs) it was taken.</summary>
public sealed record TimeSyncResult(long OffsetUs, long RttUs, long SyncedAtHostUs);

/// <summary>Link counters (contract 07 §2 <c>stats</c>); updated by the reader thread.</summary>
public sealed class LinkStats
{
    public long TxPackets { get; internal set; }
    public long RxPackets { get; internal set; }
    public long TxBytes { get; internal set; }
    public long RxBytes { get; internal set; }
    public long CrcErrors { get; internal set; }
    public long HeaderErrors { get; internal set; }
    public long TrashBytes { get; internal set; }

    /// <summary>Gaps in the device → host sequence numbers, seen by the host.</summary>
    public long SeqGaps { get; internal set; }

    /// <summary>RPT_SEQUENCE_ERROR reports: gaps the device saw in host → device packets.</summary>
    public long DeviceSeqErrors { get; internal set; }

    /// <summary>Callbacks that threw on the reader thread (contract 07 §3).</summary>
    public long CallbackErrors { get; internal set; }
}

/// <summary>Claims a packet as the reply to a request: return true and set <paramref name="result"/>.</summary>
public delegate bool ReplyMatcher<T>(Packet packet, out T result);

/// <summary>
/// A connection to one DEPZ device in application mode (contracts 02, 07):
/// a reader thread, replies correlated to requests by echoed opcode (one request
/// in flight per opcode), events, and the common commands. Sensor classes
/// derive from it. Requests block the calling thread up to <see cref="Timeout"/>
/// (default 200 ms). Callbacks (<c>On…</c>) run on the reader thread: keep them
/// short, never block, never close the device from one; in Unity use the
/// <see cref="Subscription{T}"/> queues and poll them from <c>Update()</c>.
/// </summary>
public class DepzDevice : IDisposable
{
    /// <summary>The default request timeout, 200 ms (contract 07 §3).</summary>
    public static readonly TimeSpan DefaultTimeout = TimeSpan.FromMilliseconds(200);

    private static readonly TimeSpan ReadSlice = TimeSpan.FromMilliseconds(50);

    private readonly Core _core;

    /// <summary>Open a connection on any link; the device owns it and closes it.</summary>
    public DepzDevice(ILink link, TimeSpan? timeout = null)
    {
        _core = new Core(link ?? throw new ArgumentNullException(nameof(link)), timeout ?? DefaultTimeout, this);
        _core.Start();
    }

#if NET
    /// <summary>Open the serial port at <paramref name="port"/> (e.g. <c>/dev/ttyACM0</c>, <c>COM5</c>).</summary>
    public DepzDevice(string port, TimeSpan? timeout = null)
        : this(new SerialPortLink(port), timeout)
    {
    }
#endif

    /// <summary>Continue on the connection of <paramref name="from"/> (<see cref="DepzDevices.Attach"/>).</summary>
    private protected DepzDevice(DepzDevice from)
    {
        _core = from._core;
        _core.Owner = this;
    }

    internal Core Connection => _core;

    // ── lifecycle ────────────────────────────────────────────────────────────

    /// <summary>The link's name: the port path, or what the link wraps.</summary>
    public string Port => _core.Link.Name;

    public bool IsClosed => _core.Link.IsClosed;

    /// <summary>The timeout of requests that take none of their own.</summary>
    public TimeSpan Timeout
    {
        get => _core.Timeout;
        set => _core.Timeout = value > TimeSpan.Zero ? value : throw new ArgumentOutOfRangeException(nameof(value));
    }

    /// <summary>The payload CRC the host puts on its packets (contract 01).</summary>
    public CrcType TxCrcType
    {
        get => _core.TxCrcType;
        set => _core.TxCrcType = value;
    }

    public LinkStats Stats => _core.Stats;

    /// <summary>Close the link and stop the reader; pending requests and streams end.</summary>
    public void Close()
    {
        try
        {
            if (!IsClosed)
                OnClosing();
        }
        finally
        {
            _core.Close();
        }
    }

    public void Dispose()
    {
        Close();
        GC.SuppressFinalize(this);
    }

    // ── for sensor classes ───────────────────────────────────────────────────

    /// <summary>Route a streaming report; return true when the packet was consumed. Runs on the reader thread.</summary>
    private protected virtual bool HandleReport(Packet packet) => false;

    /// <summary>The link is about to close: a class whose sensor keeps running on its own stops it here.
    /// Exceptions are the class's to swallow; the link closes regardless.</summary>
    private protected virtual void OnClosing() { }

    /// <summary>The reader stopped: end the class's streams. Runs on the reader thread.</summary>
    private protected virtual void OnReaderStopped() => _core.Events.CloseAll();

    internal bool DispatchReport(Packet packet) => HandleReport(packet);

    internal void ReaderStopped() => OnReaderStopped();

    // ── events ───────────────────────────────────────────────────────────────

    /// <summary>Call <paramref name="callback"/> for every event (reader thread). Dispose the result to stop.</summary>
    public IDisposable OnEvent(Action<DeviceEvent> callback) => _core.Events.OnItem(callback);

    /// <summary>A pull queue of events (bounded, drop-oldest); subscribed when this returns.</summary>
    public Subscription<DeviceEvent> Events(int capacity = 256) => _core.Events.Subscribe(capacity);

    // ── TX / requests ────────────────────────────────────────────────────────

    /// <summary>Send a packet without waiting for anything (escape hatch; prefer a request).</summary>
    public void Send(int cmd, byte[]? payload = null, CrcType? crcType = null) =>
        _core.Send(cmd, payload ?? Array.Empty<byte>(), crcType ?? _core.TxCrcType);

    /// <summary>
    /// Send <paramref name="cmd"/> and wait for the first packet <paramref name="matcher"/> claims
    /// (escape hatch, contract 07 §2). A non-OK RPT_STATUS echoing <paramref name="cmd"/> throws
    /// <see cref="StatusException"/> (<see cref="BusyException"/> for ERR_BUSY); one request per
    /// opcode may be in flight.
    /// </summary>
    public T Request<T>(int cmd, byte[]? payload, ReplyMatcher<T> matcher, TimeSpan? timeout = null)
    {
        if (matcher is null) throw new ArgumentNullException(nameof(matcher));
        object? result = _core.Request(cmd, payload ?? Array.Empty<byte>(), pkt =>
            matcher(pkt, out T r) ? new Boxed(r) : null, okCompletes: false, timeout);
        return (T)((Boxed)result!).Value!;
    }

    /// <summary>Send <paramref name="cmd"/> and wait for RPT_STATUS OK echoing it.</summary>
    public void RequestOk(int cmd, byte[]? payload = null, TimeSpan? timeout = null) =>
        _core.Request(cmd, payload ?? Array.Empty<byte>(), null, okCompletes: true, timeout);

    /// <summary>A matcher for a report identified by its report id alone.</summary>
    public static ReplyMatcher<T> ExpectReport<T>(int reportId, Func<byte[], T> unpack)
    {
        bool Match(Packet p, out T r)
        {
            r = p.Cmd == reportId ? unpack(p.Payload) : default!;
            return p.Cmd == reportId;
        }
        return Match;
    }

    /// <summary>A matcher for RPT_TEXT echoing <paramref name="requestCmd"/>.</summary>
    public static ReplyMatcher<string> ExpectText(int requestCmd)
    {
        bool Match(Packet p, out string r)
        {
            bool mine = p.Cmd == (int)Rpt.Text && p.Payload.Length > 0 && p.Payload[0] == requestCmd;
            r = mine ? TextReport.Unpack(p.Payload).Text : "";
            return mine;
        }
        return Match;
    }

    // ── common commands (contract 02) ────────────────────────────────────────

    public string GetDeviceName() =>
        Request((int)Cmd.GetDeviceName, null, ExpectText((int)Cmd.GetDeviceName));

    public string GetSoftwareName() =>
        Request((int)Cmd.GetNameActiveSoftware, null, ExpectText((int)Cmd.GetNameActiveSoftware));

    public string GetSerialNumber() =>
        Request((int)Cmd.GetSerial, null, ExpectText((int)Cmd.GetSerial));

    /// <summary>The running firmware: the software name parsed into mode, sensor type and version (contract 02 §4).</summary>
    public Identity Identify() => IdentityParser.ParseSoftwareName(GetSoftwareName());

    /// <summary>The MCU temperature the device last cached, °C (it refreshes about twice a second).</summary>
    public double ReadMcuTemperature() =>
        Request((int)Cmd.GetMcuTemperature, null,
            ExpectReport((int)Rpt.Temperature, p => TemperatureReport.Unpack(p))).Celsius;

    /// <summary>NTP-style time sync keeping the lowest-RTT of <paramref name="samples"/> rounds (contract 02 §5).</summary>
    public TimeSyncResult SyncTime(int samples = 5)
    {
        TimeSyncResult? best = null;
        for (int i = 0; i < Math.Max(1, samples); i++)
        {
            long t1 = HostNowUs();
            SyncTimeReport rep = Request((int)Cmd.SyncTime, Common.PackSyncTime((ulong)t1),
                ExpectReport((int)Rpt.SyncTime, p => SyncTimeReport.Unpack(p)));
            long t4 = HostNowUs();
            var (offset, rtt) = Common.SyncTimeOffsetRtt(t1, (long)rep.McuRxUs, (long)rep.McuTxUs, t4);
            if (best is null || rtt < best.RttUs)
                best = new TimeSyncResult(offset, rtt, t4);
        }
        _core.TimeSync = best;
        return best!;
    }

    /// <summary>The last <see cref="SyncTime"/> result, or null.</summary>
    public TimeSyncResult? LastTimeSync => _core.TimeSync;

    /// <summary>Device µs → host monotonic µs (<see cref="HostNowUs"/>); needs a <see cref="SyncTime"/> first.</summary>
    public long ToHostTimeUs(long deviceTimestampUs) =>
        deviceTimestampUs - (_core.TimeSync ?? throw new InvalidStateException("call SyncTime() first")).OffsetUs;

    /// <summary>The device → host payload CRC mode.</summary>
    public CrcType GetReportPayloadCrc() =>
        (CrcType)Request((int)Cmd.GetPayloadCrcType, null, ExpectReport((int)Rpt.PayloadCrcType, p =>
        {
            Wire.NeedBytes("RPT_PAYLOAD_CRC_TYPE", p, 1);
            return (int)p[0];
        }));

    /// <summary>Set the device → host payload CRC mode (host → device is per packet: <see cref="TxCrcType"/>).</summary>
    public void SetReportPayloadCrc(CrcType crcType)
    {
        RequestOk((int)Cmd.SetPayloadCrcType, Common.PackSetPayloadCrcType((int)crcType));
    }

    public SyncPinConfig GetSyncPin(int pin)
    {
        if (pin < SyncPinConfig.PinMin || pin > SyncPinConfig.PinMax)
            throw new ArgumentOutOfRangeException(nameof(pin), pin, $"sync pin must be {SyncPinConfig.PinMin}..{SyncPinConfig.PinMax}");
        return Request((int)Cmd.GetSyncPinConfig, new[] { (byte)pin },
            ExpectReport((int)Rpt.SyncPinConfig, p => SyncPinConfig.Unpack(p)));
    }

    public void SetSyncPin(SyncPinConfig config)
    {
        if (config is null) throw new ArgumentNullException(nameof(config));
        RequestOk((int)Cmd.SetSyncPinConfig, config.Pack());
    }

    /// <summary>DEVICE_RESET: the device acknowledges, then reboots; the link drops.</summary>
    public void Reset() => RequestOk((int)Cmd.DeviceReset);

    /// <summary>Reboot into the resident bootloader and close this connection (flashing: contract 06).</summary>
    public void EnterBootloaderMode()
    {
        RequestOk((int)Cmd.Bootloader);
        Close();
    }

    /// <summary>The host monotonic clock in µs — the host side of all time-sync math.</summary>
    public static long HostNowUs()
    {
        long ticks = Stopwatch.GetTimestamp();
        long f = Stopwatch.Frequency;
        return ticks / f * 1_000_000 + ticks % f * 1_000_000 / f;
    }

    private sealed class Boxed
    {
        public readonly object? Value;
        public Boxed(object? value) => Value = value;
    }

    // ── the connection: reader thread and request correlation ────────────────

    internal sealed class Core
    {
        public readonly ILink Link;
        public readonly LinkStats Stats = new();
        public readonly Fanout<DeviceEvent> Events = new();
        public volatile DepzDevice Owner;
        public TimeSpan Timeout;
        public CrcType TxCrcType = CrcType.None;
        public TimeSyncResult? TimeSync;

        private readonly PacketParser _parser = new();
        private readonly object _txLock = new();
        private readonly Dictionary<int, Pending> _pending = new();
        private readonly Thread _reader;
        private int _txSeq;
        private int _lastRxSeq = -1;
        private volatile bool _closing;

        public Core(ILink link, TimeSpan timeout, DepzDevice owner)
        {
            Link = link;
            Timeout = timeout;
            Owner = owner;
            _reader = new Thread(ReaderLoop) { IsBackground = true, Name = $"depz-reader-{link.Name}" };
        }

        public void Start() => _reader.Start();

        public void Close()
        {
            _closing = true;
            Link.Close();
            if (Thread.CurrentThread != _reader && _reader.IsAlive)
                _reader.Join(TimeSpan.FromSeconds(2));
        }

        public void Send(int cmd, byte[] payload, CrcType crcType)
        {
            lock (_txLock)
            {
                byte[] frame = Framing.BuildPacket(cmd, payload, _txSeq, crcType);
                Link.Write(frame);
                _txSeq = (_txSeq + 1) & 0xFF;
                Stats.TxPackets++;
                Stats.TxBytes += frame.Length;
            }
        }

        public object? Request(int cmd, byte[] payload, Func<Packet, object?>? matcher, bool okCompletes, TimeSpan? timeout)
        {
            var pending = new Pending(cmd, matcher, okCompletes);
            lock (_pending)
            {
                if (_pending.ContainsKey(cmd))
                    throw new BusyException(cmd);
                _pending[cmd] = pending;
            }
            try
            {
                Send(cmd, payload, TxCrcType);
            }
            catch
            {
                Take(cmd, pending);
                throw;
            }
            TimeSpan t = timeout ?? Timeout;
            if (!pending.Done.Wait(t))
            {
                Take(cmd, pending);
                // The reply may have landed between the wait and the removal.
                if (!pending.Done.IsSet)
                    throw new DepzTimeoutException(cmd, t);
            }
            if (pending.Error != null)
                throw pending.Error;
            return pending.Result;
        }

        private void Take(int cmd, Pending p)
        {
            lock (_pending)
            {
                if (_pending.TryGetValue(cmd, out var cur) && ReferenceEquals(cur, p))
                    _pending.Remove(cmd);
            }
        }

        private void FailAll(Exception e)
        {
            Pending[] all;
            lock (_pending)
            {
                all = _pending.Values.ToArray();
                _pending.Clear();
            }
            foreach (var p in all)
                p.Fail(e);
        }

        private void Emit(DeviceEvent e) => Events.Emit(e, Stats);

        private void ReaderLoop()
        {
            try
            {
                while (!_closing)
                {
                    byte[] data;
                    try
                    {
                        data = Link.Read(ReadSlice);
                    }
                    catch (Exception e) when (e is DeviceLostException || e is IOException
                                              || e is ObjectDisposedException || e is InvalidOperationException
                                              || e is UnauthorizedAccessException)
                    {
                        if (!_closing)
                        {
                            FailAll(new DeviceLostException(e.Message, e));
                            Emit(new DisconnectedEvent(e.Message));
                        }
                        return;
                    }
                    if (data.Length == 0)
                        continue;
                    Stats.RxBytes += data.Length;
                    foreach (var ev in _parser.Feed(data))
                    {
                        switch (ev)
                        {
                            case Packet pkt:
                                Stats.RxPackets++;
                                TrackSeq(pkt.Seq);
                                Dispatch(pkt);
                                break;
                            case CrcError ce:
                                Stats.CrcErrors++;
                                Emit(new LinkCrcErrorEvent(ce.Cmd, ce.Seq));
                                break;
                            case Trash tr:
                                Stats.TrashBytes += tr.Data.Length;
                                Emit(new TrashEvent(tr.Data));
                                break;
                        }
                    }
                    Stats.HeaderErrors = _parser.HeaderErrors;
                }
            }
            catch (Exception e)
            {
                // A callback or handler threw: the reader is gone, as in the reference SDK.
                FailAll(new DeviceLostException($"device reader stopped: {e.GetType().Name}: {e.Message}", e));
                try { Emit(new DisconnectedEvent($"reader stopped: {e.Message}")); } catch { /* the same callback again */ }
            }
            finally
            {
                FailAll(new LinkClosedException("device closed"));
                Owner.ReaderStopped();
            }
        }

        private void TrackSeq(int seq)
        {
            // The first packet after open is no gap; a gap is surfaced ahead of its packet.
            if (_lastRxSeq >= 0)
            {
                int expected = (_lastRxSeq + 1) & 0xFF;
                if (seq != expected)
                {
                    Stats.SeqGaps++;
                    Emit(new SeqGapEvent(expected, seq, (seq - expected) & 0xFF));
                }
            }
            _lastRxSeq = seq;
        }

        private void Dispatch(Packet pkt)
        {
            if (pkt.Cmd == (int)Rpt.Status && pkt.Payload.Length >= 2)
            {
                var rep = StatusReport.Unpack(pkt.Payload);
                if (rep.Cmd != Reports.Unsolicited)
                {
                    Pending? pending;
                    Pending? completed = null;
                    Exception? error = null;
                    lock (_pending)
                    {
                        if (_pending.TryGetValue(rep.Cmd, out pending))
                        {
                            if (rep.Status == (int)Status.ErrBusy)
                                error = new BusyException(rep.Cmd);
                            else if (rep.Status != (int)Status.Ok)
                                error = new StatusException(rep.Cmd, rep.Status);
                            else if (pending.OkCompletes)
                                completed = pending;
                            // else: an OK ack while the data report is still to come — keep waiting.
                            if (completed != null || error != null)
                                _pending.Remove(rep.Cmd);
                        }
                    }
                    if (pending != null)
                    {
                        if (error != null)
                            pending.Fail(error);
                        else
                            completed?.Complete(null);
                        return;
                    }
                }
                Emit(new UnsolicitedStatusEvent(rep.Status));
                return;
            }

            // Pending matchers get the first shot at any other packet.
            Pending? claimed = null;
            object? result = null;
            Exception? bad = null;
            lock (_pending)
            {
                foreach (var p in _pending.Values)
                {
                    if (p.Matcher is null)
                        continue;
                    try
                    {
                        result = p.Matcher(pkt);
                    }
                    catch (Exception e) when (e is ArgumentException || e is InvalidDataException || e is FormatException)
                    {
                        // The reply is for this request but does not decode (contract 07 §3: reject it).
                        bad = e;
                    }
                    if (result != null || bad != null)
                    {
                        claimed = p;
                        _pending.Remove(p.Cmd);
                        break;
                    }
                }
            }
            if (claimed != null)
            {
                if (bad != null)
                    claimed.Fail(new InvalidReplyException(claimed.Cmd, bad));
                else
                    claimed.Complete(result);
                return;
            }

            // Contract 07 §3: a report longer than its fields is decoded and the tail
            // ignored (a newer firmware may append fields); a shorter one is not a
            // report of that kind and stays visible as hex text below.
            if (pkt.Cmd == (int)Rpt.SequenceError && pkt.Payload.Length >= 2)
            {
                var rep = SequenceErrorReport.Unpack(pkt.Payload);
                Stats.DeviceSeqErrors++;
                Emit(new SequenceErrorEvent(rep.ExpectedSeq, rep.ReceivedSeq, ReportedByDevice: true));
                return;
            }
            if (pkt.Cmd == (int)Rpt.Text && pkt.Payload.Length > 0)
            {
                var rep = TextReport.Unpack(pkt.Payload);
                Emit(new TextEvent(rep.Cmd, rep.Text));
                return;
            }
            if (pkt.Cmd == (int)Rpt.Temperature && pkt.Payload.Length >= 10)
            {
                var rep = TemperatureReport.Unpack(pkt.Payload);
                Emit(new TemperatureEvent(rep.TimestampUs, rep.Celsius));
                return;
            }
            if (Owner.DispatchReport(pkt))
                return;
            // An unrouted report stays visible, so new firmware is debuggable.
            Emit(new TextEvent(pkt.Cmd, BitConverter.ToString(pkt.Payload).Replace("-", "").ToLowerInvariant()));
        }
    }

    // Done is set by the reader thread, possibly after the requester has given up and left:
    // disposing it would race that Set. A ManualResetEventSlim that never allocated a wait
    // handle holds nothing to free.
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Design", "CA1001", Justification = "Done outlives the request; see above")]
    private sealed class Pending
    {
        public readonly int Cmd;
        public readonly Func<Packet, object?>? Matcher;
        public readonly bool OkCompletes;
        public readonly ManualResetEventSlim Done = new(false);
        public object? Result;
        public Exception? Error;

        public Pending(int cmd, Func<Packet, object?>? matcher, bool okCompletes)
        {
            Cmd = cmd;
            Matcher = matcher;
            OkCompletes = okCompletes;
        }

        public void Complete(object? result)
        {
            Result = result;
            Done.Set();
        }

        public void Fail(Exception e)
        {
            Error = e;
            Done.Set();
        }
    }
}
