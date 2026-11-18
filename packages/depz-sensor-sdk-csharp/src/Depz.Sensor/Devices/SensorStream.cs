using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;

namespace Depz.Sensor.Devices;

/// <summary>
/// One kind of item a device emits, as its consumer sees it (contract 07 §3): what
/// <c>Sr04Device.Measurements</c>, <c>Vl53l8cxDevice.Frames</c>, <c>Bno055Device.Samples</c>,
/// <c>Bno086Device.Reports</c> and <see cref="DepzDevice.Events"/> are. <see cref="On"/> subscribes a
/// callback (reader thread: keep it short, never block, never close the device from it) and returns the
/// handle that unsubscribes; <see cref="Subscribe"/> makes a <see cref="Subscription{T}"/> — a bounded
/// queue that drops the oldest item when full — registered when it returns, so nothing emitted after
/// that is missed (Unity: poll it from <c>Update()</c>); <see cref="Next"/> waits for one item on a
/// one-slot queue of its own. <c>where</c> keeps only the items it accepts (a BNO086 sensor-id filter,
/// for instance). <see cref="DroppedCounts"/> and <see cref="ParseErrors"/> are its counters. The owner
/// side — emitting, counting, closing — is the device's.
/// </summary>
[SuppressMessage("Naming", "CA1711:Identifiers should not have incorrect suffix", Justification = "a stream of sensor items (contract 07 §3), named as in the other SDKs; not a System.IO.Stream")]
public sealed class SensorStream<T>
{
    /// <summary>The default timeout of <see cref="Next"/>, 2 s (contract 07 §3).</summary>
    public static readonly TimeSpan DefaultNextTimeout = TimeSpan.FromSeconds(2);

    private readonly DepzDevice.Core _core;
    private readonly int? _reportId; // what Next() names in its timeout; null = no Next() (events)
    private readonly int _defaultCapacity;
    // Copy-on-write snapshots, replaced under _lock by On / Subscribe / their disposal: Emit (the reader
    // thread, at frame rate) walks the current array without a lock, a copy or a closure, so an item costs
    // the subscribers nothing on the GC heap (bug hunt 2026-11-03 KNOWN-1).
    private readonly object _lock = new();
    private volatile CallbackEntry[] _callbacks = Array.Empty<CallbackEntry>();
    private volatile QueueEntry[] _queues = Array.Empty<QueueEntry>();
    private long _parseErrors;
    private bool _closed; // guarded by _lock: set by CloseAll, never cleared
    private DepzException? _end; // guarded by _lock: why the stream ended, for a queue subscribed later

    private sealed class CallbackEntry
    {
        public readonly Action<T> Callback;
        public readonly Func<T, bool>? Where;
        public CallbackEntry(Action<T> callback, Func<T, bool>? where) { Callback = callback; Where = where; }
    }

    private sealed class QueueEntry
    {
        public readonly Subscription<T> Queue;
        public readonly Func<T, bool>? Where;
        public readonly Action<T> Put; // the queue's Put, bound once here — not per item
        public QueueEntry(Subscription<T> queue, Func<T, bool>? where) { Queue = queue; Where = where; Put = queue.Put; }
    }

    internal SensorStream(DepzDevice.Core core, int? reportId, int defaultCapacity)
    {
        _core = core;
        _reportId = reportId;
        _defaultCapacity = defaultCapacity;
    }

    // ── consumer side ────────────────────────────────────────────────────────

    /// <summary>Call <paramref name="callback"/> for every item <paramref name="where"/> accepts (null: every
    /// item) on the reader thread. Dispose the result to stop; disposing it twice is a no-op.</summary>
    public IDisposable On(Action<T> callback, Func<T, bool>? where = null)
    {
        if (callback is null) throw new ArgumentNullException(nameof(callback));
        var entry = new CallbackEntry(callback, where);
        lock (_lock)
            _callbacks = _callbacks.Append(entry).ToArray();
        return new Unsubscriber(() =>
        {
            lock (_lock)
                _callbacks = _callbacks.Where(e => !ReferenceEquals(e, entry)).ToArray();
        });
    }

    /// <summary>A pull queue of the items <paramref name="where"/> accepts (null: every item), bounded to
    /// <paramref name="capacity"/> (null: the stream's default), drop-oldest; subscribed when this returns.
    /// On a closed device the queue is closed at once: <see cref="Subscription{T}.Take"/> fails without
    /// waiting (contract 07 §3).</summary>
    public Subscription<T> Subscribe(int? capacity = null, Func<T, bool>? where = null)
    {
        var sub = new Subscription<T>(capacity ?? _defaultCapacity, s =>
        {
            lock (_lock)
                _queues = _queues.Where(e => !ReferenceEquals(e.Queue, s)).ToArray();
        });
        lock (_lock)
        {
            if (_closed)
                sub.MarkClosed(_end);
            else
                _queues = _queues.Append(new QueueEntry(sub, where)).ToArray();
        }
        return sub;
    }

    /// <summary>The next item on a one-slot queue of its own, subscribed for this call only (contract 07
    /// §3), waiting up to <paramref name="timeout"/> (null: <see cref="DefaultNextTimeout"/>;
    /// <see cref="Timeout.InfiniteTimeSpan"/>, or more than 24.8 days: no deadline):
    /// <see cref="DepzTimeoutException"/> naming the stream's report on expiry,
    /// before or as soon as the device ends, <see cref="LinkClosedException"/> after the program's own Close()
    /// and <see cref="DeviceLostException"/> carrying the cause after a loss (an item already queued still
    /// wins). The <c>Events</c> stream offers no Next() (no single report to wait for): there the call throws
    /// <see cref="InvalidStateException"/> before anything is subscribed.</summary>
    public T Next(TimeSpan? timeout = null)
    {
        if (_reportId is not int reportId)
            throw new InvalidStateException("the events stream offers no Next(): no single report to wait for "
                                            + "(contract 07 §3) — subscribe with Events.On(cb) or Events.Subscribe()");
        var wait = timeout ?? DefaultNextTimeout;
        if (_core.IsClosed)
            throw _core.EndError("device is closed");
        using var sub = Subscribe(1);
        try
        {
            return sub.Take(wait);
        }
        catch (DepzTimeoutException)
        {
            throw new DepzTimeoutException(reportId, wait);
        }
        catch (LinkClosedException)
        {
            throw new LinkClosedException("device closed while waiting for a stream item");
        }
    }

    /// <summary>The drop counter of each open <see cref="Subscribe">subscription</see> (a closed one leaves the list).</summary>
    public IReadOnlyList<long> DroppedCounts
    {
        get { return _queues.Select(e => e.Queue.DroppedCount).ToList(); }
    }

    /// <summary>Reports of this stream dropped because they did not decode (contract 07 §2): one shorter than
    /// its header, or a value the decoder refuses. Monotonic over the device's life, except where a sensor
    /// contract resets it (TMF8829 <c>Start()</c>).</summary>
    public long ParseErrors => Interlocked.Read(ref _parseErrors);

    // ── owner side (the device, reader thread) ───────────────────────────────

    /// <summary>Hand <paramref name="item"/> to every callback, then to every queue, each <c>where</c> applied
    /// (contract 07 §3). A callback (or a predicate) that throws is counted in <see cref="LinkStats.CallbackErrors"/>,
    /// traced and dropped — it never stops the reader, the other callbacks or the queues. Allocation-free per
    /// item: the subscriber arrays are read as they are and the guard takes the callback and its predicate as
    /// arguments (Unity: nothing for the GC at frame rate).</summary>
    internal void Emit(T item)
    {
        var cbs = _callbacks;
        for (int i = 0; i < cbs.Length; i++)
        {
            if (!CallbackGuard.Call(cbs[i].Callback, cbs[i].Where, item))
                _core.Stats.CallbackErrors++;
        }
        var qs = _queues;
        for (int i = 0; i < qs.Length; i++)
        {
            if (!CallbackGuard.Call(qs[i].Put, qs[i].Where, item))
                _core.Stats.CallbackErrors++;
        }
    }

    /// <summary>A report of this stream that did not decode: dropped and counted (contract 07 §2).</summary>
    internal void CountParseError() => Interlocked.Increment(ref _parseErrors);

    /// <summary>Where a sensor contract restarts the count (TMF8829 <c>Start()</c>).</summary>
    internal void ResetParseErrors() => Interlocked.Exchange(ref _parseErrors, 0);

    /// <summary>Close every queue, and every one subscribed from now on, for the reason <paramref name="why"/>.</summary>
    internal void CloseAll(DepzException why)
    {
        QueueEntry[] qs;
        lock (_lock)
        {
            _closed = true;
            _end = why;
            qs = _queues;
        }
        foreach (var e in qs)
            e.Queue.MarkClosed(why);
    }

    private sealed class Unsubscriber : IDisposable
    {
        private Action? _undo;
        public Unsubscriber(Action undo) => _undo = undo;
        public void Dispose() => Interlocked.Exchange(ref _undo, null)?.Invoke();
    }
}

/// <summary>Runs a user callback where its exception must not stop the caller (a device reader, a dataset
/// player — contract 07 §3, 09 §Player): one that throws is traced and dropped. False if it threw, so the
/// caller counts it.</summary>
internal static class CallbackGuard
{
    [SuppressMessage("Design", "CA1031:Do not catch general exception types",
        Justification = "contract 07 §3 / 09: whatever a user callback throws must not stop its caller")]
    public static bool Call<T>(Action<T> cb, T item)
    {
        try
        {
            cb(item);
            return true;
        }
        catch (Exception e)
        {
            Trace.TraceError($"depz-sensor-sdk: a callback threw; dropped: {e}");
            return false;
        }
    }

    /// <summary>The same with the subscriber's predicate applied inside the guard (a throwing predicate is a
    /// dropped item too): a stream's per-item step without a closure.</summary>
    [SuppressMessage("Design", "CA1031:Do not catch general exception types",
        Justification = "contract 07 §3: whatever a user callback or predicate throws must not stop the reader")]
    public static bool Call<T>(Action<T> cb, Func<T, bool>? where, T item)
    {
        try
        {
            if (where is null || where(item))
                cb(item);
            return true;
        }
        catch (Exception e)
        {
            Trace.TraceError($"depz-sensor-sdk: a callback threw; dropped: {e}");
            return false;
        }
    }
}
