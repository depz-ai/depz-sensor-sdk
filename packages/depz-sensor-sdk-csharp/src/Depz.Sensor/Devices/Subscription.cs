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
    private DepzException? _end; // why it closed: a DeviceLostException after a loss; null = this side closed it

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

    /// <summary>The oldest item, waiting up to <paramref name="timeout"/> for one;
    /// <see cref="Timeout.InfiniteTimeSpan"/>, or more than 24.8 days, waits without a deadline.</summary>
    /// <exception cref="DepzTimeoutException">Nothing arrived in time.</exception>
    /// <exception cref="LinkClosedException">The queue is empty and the program closed the device or this subscription (contract 07 §3).</exception>
    /// <exception cref="DeviceLostException">The queue is empty and the device was lost; carries the cause.</exception>
    public T Take(TimeSpan timeout)
    {
        var waited = System.Diagnostics.Stopwatch.StartNew();
        lock (_q)
        {
            while (_q.Count == 0)
            {
                if (_closed)
                    throw EndError();
                if (Waits.IsUnbounded(timeout))
                {
                    Monitor.Wait(_q);
                    continue;
                }
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

    // A fresh error of the kind the queue ended with (the stack belongs to the caller's wait).
    private DepzException EndError() =>
        _end is DeviceLostException lost
            ? new DeviceLostException(lost.Message, lost.InnerException!)
            : new LinkClosedException(_end?.Message ?? "stream closed: this subscription is closed");

    // No more items, for the reason `why` (null: this side closed it); the first reason stays.
    internal void MarkClosed(DepzException? why)
    {
        lock (_q)
        {
            if (!_closed)
            {
                _closed = true;
                _end = why;
            }
            Monitor.PulseAll(_q);
        }
    }

    public void Dispose()
    {
        MarkClosed(null);
        _unregister(this);
    }
}
