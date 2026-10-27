using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;

namespace Depz.Sensor.Devices;

/// <summary>
/// A pull subscription to one stream (contract 07 §3): a bounded queue that
/// drops the oldest item when full and counts the drops. It is registered when
/// created, so nothing produced after that is missed. Poll it with
/// <see cref="TryTake"/> (Unity: from <c>Update()</c>, never blocks) or wait
/// with <see cref="Take"/>; <see cref="Dispose"/> unsubscribes.
/// </summary>
public sealed class Subscription<T> : IDisposable
{
    private readonly Queue<T> _q = new();
    private readonly int _capacity;
    private readonly Action<Subscription<T>> _unregister;
    private bool _closed;

    internal Subscription(int capacity, Action<Subscription<T>> unregister)
    {
        if (capacity < 1)
            throw new ArgumentOutOfRangeException(nameof(capacity), capacity, "capacity must be at least 1");
        _capacity = capacity;
        _unregister = unregister;
    }

    /// <summary>Items dropped because the queue was full (monotonic).</summary>
    public long DroppedCount { get; private set; }

    /// <summary>Items waiting.</summary>
    public int Count
    {
        get { lock (_q) return _q.Count; }
    }

    /// <summary>The device closed (or this was disposed): no more items will arrive.</summary>
    public bool IsClosed
    {
        get { lock (_q) return _closed; }
    }

    /// <summary>The oldest waiting item, without blocking.</summary>
    public bool TryTake(out T item)
    {
        lock (_q)
        {
            if (_q.Count > 0)
            {
                item = _q.Dequeue();
                return true;
            }
        }
        item = default!;
        return false;
    }

    /// <summary>The oldest item, waiting up to <paramref name="timeout"/> for one.</summary>
    /// <exception cref="DepzTimeoutException">Nothing arrived in time.</exception>
    /// <exception cref="DeviceLostException">The device closed with the queue empty.</exception>
    public T Take(TimeSpan timeout)
    {
        var waited = System.Diagnostics.Stopwatch.StartNew();
        lock (_q)
        {
            while (_q.Count == 0)
            {
                if (_closed)
                    throw new DeviceLostException("stream closed: the device is closed");
                var left = timeout - waited.Elapsed;
                if (left <= TimeSpan.Zero)
                    throw new DepzTimeoutException($"no stream item within {timeout.TotalSeconds:0.000}s");
                Monitor.Wait(_q, left);
            }
            return _q.Dequeue();
        }
    }

    /// <summary>Everything waiting now, oldest first, without blocking.</summary>
    public List<T> Drain()
    {
        lock (_q)
        {
            var all = new List<T>(_q);
            _q.Clear();
            return all;
        }
    }

    internal void Put(T item)
    {
        lock (_q)
        {
            if (_closed)
                return;
            if (_q.Count >= _capacity)
            {
                _q.Dequeue();
                DroppedCount++;
            }
            _q.Enqueue(item);
            Monitor.PulseAll(_q);
        }
    }

    internal void MarkClosed()
    {
        lock (_q)
        {
            _closed = true;
            Monitor.PulseAll(_q);
        }
    }

    public void Dispose()
    {
        MarkClosed();
        _unregister(this);
    }
}

/// <summary>The subscribers of one stream: callbacks and pull queues.</summary>
internal sealed class Fanout<T>
{
    private readonly List<Action<T>> _callbacks = new();
    private readonly List<Subscription<T>> _queues = new();

    public IDisposable OnItem(Action<T> callback)
    {
        if (callback is null) throw new ArgumentNullException(nameof(callback));
        lock (_callbacks)
            _callbacks.Add(callback);
        return new Unsubscriber(() => { lock (_callbacks) _callbacks.Remove(callback); });
    }

    public Subscription<T> Subscribe(int capacity)
    {
        var sub = new Subscription<T>(capacity, s => { lock (_queues) _queues.Remove(s); });
        lock (_queues)
            _queues.Add(sub);
        return sub;
    }

    public IReadOnlyList<long> DroppedCounts
    {
        get { lock (_queues) return _queues.Select(q => q.DroppedCount).ToList(); }
    }

    /// <summary>Hand <paramref name="item"/> to every callback, then to every queue (contract 07 §3);
    /// a callback that throws counts in <paramref name="stats"/>.</summary>
    public void Emit(T item, LinkStats stats)
    {
        Action<T>[] cbs;
        lock (_callbacks)
            cbs = _callbacks.ToArray();
        foreach (var cb in cbs)
            Call(cb, item, stats);
        Subscription<T>[] qs;
        lock (_queues)
            qs = _queues.ToArray();
        foreach (var q in qs)
            q.Put(item);
    }

    /// <summary>Run a user callback on the reader thread: one that throws is counted in
    /// <see cref="LinkStats.CallbackErrors"/>, traced and dropped — it never stops the reader, the
    /// other callbacks or the queues (contract 07 §3).</summary>
    [SuppressMessage("Design", "CA1031:Do not catch general exception types",
        Justification = "contract 07 §3: whatever a user callback throws must not stop the reader")]
    internal static void Call(Action<T> cb, T item, LinkStats stats)
    {
        try
        {
            cb(item);
        }
        catch (Exception e)
        {
            stats.CallbackErrors++;
            Trace.TraceError($"depz-sensor-sdk: a callback threw on the reader thread; dropped: {e}");
        }
    }

    /// <summary>The next item on a one-slot queue of its own (contract 07 §3 <c>get_measurement</c>):
    /// <see cref="DepzTimeoutException"/> naming <paramref name="reportId"/> on expiry, a closed
    /// error as soon as the device closes (an item already queued still wins).</summary>
    public T Next(TimeSpan timeout, int reportId, Func<bool> deviceClosed)
    {
        if (deviceClosed())
            throw new LinkClosedException("device is closed");
        using var sub = Subscribe(1);
        try
        {
            return sub.Take(timeout);
        }
        catch (DepzTimeoutException)
        {
            throw new DepzTimeoutException(reportId, timeout);
        }
        catch (DeviceLostException)
        {
            throw new LinkClosedException("device closed while waiting for a stream item");
        }
    }

    public void CloseAll()
    {
        Subscription<T>[] qs;
        lock (_queues)
            qs = _queues.ToArray();
        foreach (var q in qs)
            q.MarkClosed();
    }

    private sealed class Unsubscriber : IDisposable
    {
        private Action? _undo;
        public Unsubscriber(Action undo) => _undo = undo;
        public void Dispose() => Interlocked.Exchange(ref _undo, null)?.Invoke();
    }
}
