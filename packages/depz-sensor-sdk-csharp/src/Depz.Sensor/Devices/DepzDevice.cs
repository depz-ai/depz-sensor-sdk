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
public sealed record TimeSync(long OffsetUs, long RttUs, long SyncedAtHostUs);

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

    /// <summary>Reports whose handler threw: the packet was dropped (contract 07 §3).</summary>
    public long ReportErrors { get; internal set; }
}

/// <summary>Claims a packet as the reply to a request: return true and set <paramref name="result"/>.</summary>
public delegate bool ReplyMatcher<T>(Packet packet, out T result);

/// <summary>
/// A connection to one DEPZ device in application mode (contracts 02, 07):
/// a reader thread, replies correlated to requests by echoed opcode (one request
/// in flight per opcode), events, and the common commands. Sensor classes
/// derive from it. Requests block the calling thread up to <see cref="Timeout"/>
/// (default 200 ms). Every kind of item a device emits is one <see cref="SensorStream{T}"/>
/// named after the items — <see cref="Events"/> here, a sensor class's <c>Measurements</c> /
/// <c>Frames</c> / <c>Samples</c> / <c>Reports</c>. Callbacks (<c>stream.On(cb)</c>) run on the
/// reader thread: keep them short, never block, never close the device from one; in Unity use
/// the <see cref="Subscription{T}"/> queues (<c>stream.Subscribe()</c>) and poll them from <c>Update()</c>.
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

    /// <summary>Continue on the connection of <paramref name="from"/> (<see cref="DepzDevices.AttachSensorClass"/>).</summary>
    private protected DepzDevice(DepzDevice from)
    {
        _core = from._core;
        _core.Owner = this;
    }

    // ── lifecycle ────────────────────────────────────────────────────────────

    /// <summary>The link's name: the port path, or what the link wraps.</summary>
    public string Port => _core.Link.Name;

    /// <summary>The link is closed — by <see cref="Close"/>, or by the reader ending on its own (a lost
    /// device) once closing its link was tried: never true with an open link (contract 07 §3).</summary>
    public bool IsClosed => _core.IsClosed;

    /// <summary>The loss that ended this device (<c>InnerException</c> is what the link threw), or null: still
    /// open, or ended by the program's own <see cref="Close"/> (contract 07 §3).</summary>
    public DeviceLostException? Lost => _core.Lost;

    /// <summary>Throws what every request of a lost or closed device throws (contract 07 §3): the device-lost
    /// error after a loss, the closed one after <see cref="Close"/>. A sensor class calls it before a guard of its
    /// own state — "stop the stream first" — so that a board unplugged while it streamed says it is lost, not that
    /// it still streams.</summary>
    private protected void RequireOpen() => _core.CheckOpen();

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

    /// <summary>Close the link and stop the reader; pending requests and streams end. A link whose own close
    /// throws hands its exception to the caller and the device is closed all the same (contract 07 §3).</summary>
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

    /// <summary>A class's stream of items (contract 07 §3): <paramref name="reportId"/> is what its
    /// <see cref="SensorStream{T}.Next"/> names on a timeout, <paramref name="defaultCapacity"/> the queue
    /// size of a <see cref="SensorStream{T}.Subscribe"/> without one. It ends with the reader (before
    /// <see cref="Events"/>); one made after the reader ended is closed at once.</summary>
    private protected SensorStream<T> Stream<T>(int reportId, int defaultCapacity) =>
        _core.AddStream(new SensorStream<T>(_core, reportId, defaultCapacity));

    /// <summary>Run <paramref name="onEnd"/> when the reader ends (with the device-lost error, or the closed one
    /// after <see cref="Close"/>), at once if it already has: what a class waits for on its own ends with the device.</summary>
    private protected void WhenReaderEnds(Action<DepzException> onEnd) => _core.WhenReaderEnds(onEnd);

    internal bool DispatchReport(Packet packet) => HandleReport(packet);

    // ── events ───────────────────────────────────────────────────────────────

    /// <summary>The device events (contract 07 §3): unsolicited status, sequence gaps, CRC errors, text,
    /// temperature, disconnect. <c>Events.On(cb)</c> subscribes a callback (reader thread),
    /// <c>Events.Subscribe()</c> a pull queue of 256 by default; there is no single report to wait for,
    /// so <c>Events.Next()</c> is refused with <see cref="InvalidStateException"/>.</summary>
    public SensorStream<DeviceEvent> Events => _core.Events;

    // ── TX / requests ────────────────────────────────────────────────────────

    /// <summary>Send a packet without waiting for anything (escape hatch; prefer a request).</summary>
    public void Send(int cmd, byte[]? payload = null, CrcType? crcType = null) =>
        _core.Send(cmd, payload ?? Array.Empty<byte>(), crcType ?? _core.TxCrcType);

    /// <summary>
    /// Send <paramref name="cmd"/> and wait for the first packet <paramref name="matcher"/> claims
    /// (escape hatch, contract 07 §2). A non-OK RPT_STATUS echoing <paramref name="cmd"/> throws
    /// <see cref="StatusException"/> (<see cref="BusyException"/> for ERR_BUSY); one request per
    /// opcode may be in flight. <paramref name="timeout"/>: <see cref="System.Threading.Timeout.InfiniteTimeSpan"/> or more
    /// than 24.8 days waits without a deadline; a negative one is refused before anything is written.
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
    public TimeSync SyncTime(int samples = 5)
    {
        TimeSync? best = null;
        for (int i = 0; i < Math.Max(1, samples); i++)
        {
            long t1 = HostNowUs();
            SyncTimeReport rep = Request((int)Cmd.SyncTime, Common.PackSyncTime((ulong)t1),
                ExpectReport((int)Rpt.SyncTime, p => SyncTimeReport.Unpack(p)));
            long t4 = HostNowUs();
            var (offset, rtt) = Common.SyncTimeOffsetRtt(t1, (long)rep.McuRxUs, (long)rep.McuTxUs, t4);
            if (best is null || rtt < best.RttUs)
                best = new TimeSync(offset, rtt, t4);
        }
        _core.TimeSync = best;
        return best!;
    }

    /// <summary>The last <see cref="SyncTime"/> result, or null.</summary>
    public TimeSync? TimeSync => _core.TimeSync;

    /// <summary>Device µs → host monotonic µs (<see cref="HostNowUs"/>); needs a <see cref="SyncTime"/> first.</summary>
    public long ToHostTimeUs(long deviceTimestampUs) =>
        deviceTimestampUs - (_core.TimeSync ?? throw new InvalidStateException("call SyncTime() first")).OffsetUs;

    /// <summary>The device → host payload CRC mode.</summary>
    public CrcType GetReportPayloadCrc() =>
        Request((int)Cmd.GetPayloadCrcType, null, ExpectReport((int)Rpt.PayloadCrcType, p =>
        {
            Wire.NeedBytes("RPT_PAYLOAD_CRC_TYPE", p, 1);
            return p[0] <= Common.PayloadCrcTypeMax
                ? (CrcType)p[0]
                : throw new ArgumentException($"RPT_PAYLOAD_CRC_TYPE: {p[0]} is not a CrcType"); // InvalidReplyException
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
        return TicksToUs(Stopwatch.GetTimestamp(), Stopwatch.Frequency);
    }

    // Stopwatch ticks → µs, split so ticks * 1e6 cannot overflow (at 1 GHz it would after 2 h 34 min).
    internal static long TicksToUs(long ticks, long frequency) =>
        ticks / frequency * 1_000_000 + ticks % frequency * 1_000_000 / frequency;

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
        public readonly SensorStream<DeviceEvent> Events;
        public volatile DepzDevice Owner;
        public TimeSpan Timeout;
        public CrcType TxCrcType = CrcType.None;
        public TimeSync? TimeSync;

        private readonly PacketParser _parser = new();
        private readonly object _txLock = new();
        private readonly Dictionary<int, Pending> _pending = new();
        private readonly Thread _reader;
        private int _txSeq;
        private int _lastRxSeq = -1;
        private volatile bool _closing;
        /// <summary>Set when the reader ended on its own (not by Close()): every later request fails with it.</summary>
        public volatile DeviceLostException? Lost; // set first: requests fail with it at once
        private volatile bool _closeTried; // set once closing the link was tried, by Close() or by a lost device's reader

        private readonly List<Action> _closeStreams = new(); // the sensor classes' streams' CloseAll, run when the reader ends (before Events)
        private bool _readerEnded; // guarded by _closeStreams

        public Core(ILink link, TimeSpan timeout, DepzDevice owner)
        {
            Link = link;
            Timeout = timeout;
            Owner = owner;
            Events = new SensorStream<DeviceEvent>(this, reportId: null, defaultCapacity: 256);
            _reader = new Thread(ReaderLoop) { IsBackground = true, Name = $"depz-reader-{link.Name}" };
        }

        // A reader that ended marks the device closed even if closing its link failed — but only after
        // that close was tried, so IsClosed never shows an open link.
        public bool IsClosed => Link.IsClosed || _closeTried;

        /// <summary>Register a class's stream: ended with the reader; closed at once when the reader already ended.</summary>
        public SensorStream<T> AddStream<T>(SensorStream<T> stream)
        {
            lock (_closeStreams)
            {
                _closeStreams.Add(() => stream.CloseAll(EndError(StreamEnded)));
                if (_readerEnded)
                    stream.CloseAll(EndError(StreamEnded));
            }
            return stream;
        }

        /// <summary>Run <paramref name="onEnd"/> with the cause when the reader ends; at once when it already did.</summary>
        public void WhenReaderEnds(Action<DepzException> onEnd)
        {
            lock (_closeStreams)
            {
                _closeStreams.Add(() => onEnd(EndCause()));
                if (_readerEnded)
                    onEnd(EndCause());
            }
        }

        private DepzException EndCause() => Lost ?? (DepzException)new LinkClosedException("device closed");

        private const string StreamEnded = "stream closed: the device is closed";

        /// <summary>What a pull call throws on an ended device (contract 07 §3): the loss, whatever came after it;
        /// else the program's own close. A link that dropped a moment before the reader named the loss is a loss too.</summary>
        public DepzException EndError(string closedMessage)
        {
            var lost = Lost;
            if (lost is not null)
                return new DeviceLostException(lost.Message, lost.InnerException!);
            if (_closing)
                return new LinkClosedException(closedMessage);
            return new DeviceLostException($"{Link.Name}: the link closed under a stream");
        }

        // The reader ended: every registered stream ends, then the events (contract 07 §3).
        private void EndStreams()
        {
            Action[] all;
            lock (_closeStreams)
            {
                _readerEnded = true;
                all = _closeStreams.ToArray();
            }
            foreach (var close in all)
                close();
            Events.CloseAll(EndError(StreamEnded));
        }

        public void Start() => _reader.Start();

        public void Close()
        {
            _closing = true;
            try
            {
                Link.Close();
            }
            finally
            {
                // Whatever the link's close threw, the reader stops at its next read: the device is closed, not a
                // device that looks open and times every request out (contract 07 §3).
                _closeTried = true;
                if (Thread.CurrentThread != _reader && _reader.IsAlive)
                    _reader.Join(TimeSpan.FromSeconds(2));
            }
        }

        /// <summary>A lost or closed device refuses every request at once with its own error (contract 07 §3),
        /// before a slot is taken or anything is written.</summary>
        internal void CheckOpen()
        {
            var lost = Lost;
            if (lost != null)
                throw new DeviceLostException(lost.Message, lost.InnerException!);
            // From the moment Close() began: the reader may have failed what was pending and ended, and a request
            // that goes out now has nobody to answer or fail it (a link whose close takes its time, or throws).
            if (_closing)
                throw new LinkClosedException("device closed");
        }

        public void Send(int cmd, byte[] payload, CrcType crcType)
        {
            CheckOpen();
            lock (_txLock)
            {
                byte[] frame = Framing.BuildPacket(cmd, payload, _txSeq, crcType);
                try
                {
                    Link.Write(frame);
                }
                catch (LinkClosedException e) when (!_closing)
                {
                    // The link closed under this side's feet, not by Close(): the reader is marking the device lost
                    // (it closes the link after setting Lost) — the caller sees the loss, not a "closed" of a device
                    // it never closed (contract 07 §3; bug hunt 2026-11-05 F-JAVA-2). The write's own error is the
                    // cause, as in the Python reference.
                    var dead = Lost;
                    throw new DeviceLostException(
                        dead != null ? dead.Message : $"{Link.Name}: link closed under a request: {e.Message}", e);
                }
                _txSeq = (_txSeq + 1) & 0xFF;
                Stats.TxPackets++;
                Stats.TxBytes += frame.Length;
            }
        }

        public object? Request(int cmd, byte[] payload, Func<Packet, object?>? matcher, bool okCompletes, TimeSpan? timeout)
        {
            TimeSpan t = timeout ?? Timeout;
            // Before anything is written or a slot taken: the wait below would throw with the request on the wire
            // and, with no reply to clear it, the opcode busy for good.
            if (t < TimeSpan.Zero && t != System.Threading.Timeout.InfiniteTimeSpan)
                throw new ArgumentOutOfRangeException(nameof(timeout), t, "timeout must not be negative");
            // Before the opcode's slot is taken: on a closed or lost device every request fails with that error,
            // never with "busy" because another caller is giving the slot up at this moment (bug hunt 2026-11-05
            // F-JAVA-1). Send() checks again, for a Close() that began in between.
            CheckOpen();
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
            TimeSpan wait = Waits.ForWait(t);
            if (!pending.Done.Wait(wait) && Take(cmd, pending))
                throw new DepzTimeoutException(cmd, t);
            // Either the reply came in time, or it landed between the wait and the removal: the reader
            // claimed the pending under its lock and is completing it — the reply is consumed, so it is
            // this caller's (bug hunt 2026-11-03 JAVA-2). Only a reader that died mid-completion leaves
            // it unset: that wait is bounded too.
            if (!pending.Done.Wait(wait))
                throw new DepzTimeoutException(cmd, t);
            if (pending.Error != null)
                throw pending.Error;
            return pending.Result;
        }

        // Remove `p` from the slot of `cmd` if it still holds it: false when the reader claimed it meanwhile.
        private bool Take(int cmd, Pending p)
        {
            lock (_pending)
            {
                if (_pending.TryGetValue(cmd, out var cur) && ReferenceEquals(cur, p))
                {
                    _pending.Remove(cmd);
                    return true;
                }
                return false;
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

        private void Emit(DeviceEvent e) => Events.Emit(e);

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
                    catch (Exception e) when (e is DeviceLostException || e is LinkClosedException || e is IOException
                                              || e is ObjectDisposedException || e is InvalidOperationException
                                              || e is UnauthorizedAccessException)
                    {
                        Lose(new DeviceLostException(e.Message, e));
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
                // Anything else ending the reader (a link that threw something unexpected) must not
                // leave a device that looks open and times every request out (contract 07 §3).
                if (!_closing)
                    Trace.TraceError($"depz-sensor-sdk: reader of {Link.Name} ended: {e}");
                Lose(new DeviceLostException($"reader ended: {e.GetType().Name}: {e.Message}", e));
            }
            finally
            {
                FailAll(new LinkClosedException("device closed"));
                EndStreams();
            }
        }

        /// <summary>The reader ended on its own: close the link, fail what is pending and every later
        /// request with <paramref name="e"/>, say why. Nothing to do when Close() ended it.</summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Design", "CA1031:Do not catch general exception types",
            Justification = "the device is already lost; a link whose Close() throws must not stop the cleanup")]
        private void Lose(DeviceLostException e)
        {
            if (_closing)
                return;
            Lost = e;
            try
            {
                Link.Close();
            }
            catch (Exception again)
            {
                Trace.TraceError($"depz-sensor-sdk: closing {Link.Name} after its reader ended: {again}");
            }
            _closeTried = true;
            FailAll(e);
            Emit(new DisconnectedEvent(e.Message));
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
                    catch (Exception e) when (!(e is OutOfMemoryException))
                    {
                        // The reply is for this request but does not decode (contract 07 §3: reject
                        // it). Whatever the matcher threw, it must never end the reader.
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
            // The report path runs guarded like the matchers: a stream handler that throws drops this
            // one packet, counted and traced — it never ends the reader (contract 07 §3).
            bool consumed;
            try
            {
                consumed = Owner.DispatchReport(pkt);
            }
            catch (Exception e) when (!(e is OutOfMemoryException))
            {
                Stats.ReportErrors++;
                Trace.TraceError($"depz-sensor-sdk: report 0x{pkt.Cmd:X2} handler threw on {Link.Name}: {e}");
                return;
            }
            if (consumed)
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
