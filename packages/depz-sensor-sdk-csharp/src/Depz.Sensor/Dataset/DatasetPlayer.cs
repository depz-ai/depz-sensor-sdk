using System.Diagnostics;
using Depz.Sensor.Devices;

namespace Depz.Sensor.Dataset;

/// <summary><see cref="DatasetPlayer"/> state (contract 09 §Player).</summary>
public enum PlayerState
{
    /// <summary>Never played.</summary>
    Idle,

    /// <summary>Delivering records paced by the clock.</summary>
    Playing,

    /// <summary>Stopped mid-timeline; <see cref="DatasetPlayer.Play"/> resumes.</summary>
    Paused,

    /// <summary>Past the last record; <see cref="DatasetPlayer.Play"/> restarts from the top.</summary>
    Done,
}

/// <summary>
/// Timeline player over a dataset: paced delivery with play / pause / seek / speed — the engine behind a viewer's
/// playback mode (contract 09 §Player; reference: the Python <c>DatasetPlayer</c>).
/// <para>Timing: a media clock anchored at play / resume / seek to (now, t of the last delivered record — of the
/// first when none was). Record i is due at <c>anchor_wall + (t_i − anchor_t) / speed</c>: the first record goes
/// out at once, each later one after its gap to the previous one. Pacing is against the clock, so a slow consumer
/// gets overdue records back to back and playback does not drift. Speed 0 delivers as fast as possible.</para>
/// <para>Threads: records and the <see cref="PlayerState.Done"/> state are delivered on a player thread (started
/// by the first <see cref="Play"/>); a state change a call makes is reported on the calling thread before it
/// returns. Callbacks run under the player's lock — they see a consistent player and may call its methods, and
/// once <see cref="Pause"/> returns no record callback is running or will start. They must not block. With
/// <c>threaded: false</c> no thread starts and the owner calls <see cref="Tick"/> (a GUI timer, or a test with a
/// fake clock).</para>
/// </summary>
public sealed class DatasetPlayer : IDisposable
{
    private readonly object _lock = new();
    private readonly IReadOnlyList<DatasetRecord> _recs;
    private readonly Func<double> _clock;
    private readonly bool _threaded;
    private readonly List<Action<DatasetRecord>> _recordCallbacks = new();
    private readonly List<Action<PlayerState>> _stateCallbacks = new();
    private double _speed;
    private int _idx; // the next record to deliver
    private double _anchorWall;
    private double _anchorT;
    private Thread? _thread;
    private bool _closed;

    /// <param name="reader">The dataset.</param>
    /// <param name="speed">≥ 0; 0 = as fast as possible.</param>
    /// <param name="clock">Monotonic seconds (default: a <see cref="Stopwatch"/>).</param>
    /// <param name="threaded">False: no player thread, drive it with <see cref="Tick"/>.</param>
    public DatasetPlayer(DatasetReader reader, double speed = 1.0, Func<double>? clock = null, bool threaded = true)
    {
        Reader = reader ?? throw new ArgumentNullException(nameof(reader));
        _recs = reader.Records;
        var watch = Stopwatch.StartNew();
        _clock = clock ?? (() => watch.Elapsed.TotalSeconds);
        _threaded = threaded;
        _speed = CheckSpeed(speed);
    }

    /// <summary>A player over the <c>.depzdata</c> file at <paramref name="path"/>.</summary>
    public DatasetPlayer(string path, double speed = 1.0, Func<double>? clock = null, bool threaded = true)
        : this(new DatasetReader(path), speed, clock, threaded)
    {
    }

    public DatasetReader Reader { get; }

    public PlayerState State { get; private set; } = PlayerState.Idle;

    /// <summary>A callback on the player thread threw: playback paused there (the record counts as delivered).</summary>
    public event Action<Exception>? DeliveryError;

    /// <summary>Span of the timeline: the last record's t minus the first's.</summary>
    public long DurationUs => Reader.DurationUs;

    /// <summary>µs from the first record to the last delivered one (after a seek: to the record just before the
    /// target); 0 before anything was delivered.</summary>
    public long PositionUs
    {
        get
        {
            lock (_lock)
            {
                if (_recs.Count == 0 || _idx == 0)
                    return 0;
                return _recs[Math.Min(_idx, _recs.Count) - 1].THostUs - _recs[0].THostUs;
            }
        }
    }

    /// <summary>Playback speed (finite, ≥ 0; 0 = as fast as possible). A change while playing applies at once:
    /// the rest of the current wait is rescaled.</summary>
    public double Speed
    {
        get => _speed;
        set
        {
            double s = CheckSpeed(value);
            lock (_lock)
            {
                if (State == PlayerState.Playing)
                {
                    double now = _clock();
                    _anchorT = MediaNow(now);
                    _anchorWall = now;
                }
                _speed = s;
                Monitor.PulseAll(_lock);
            }
        }
    }

    /// <summary>Subscribe to delivered records; dispose the result to unsubscribe (idempotent).</summary>
    public IDisposable OnRecord(Action<DatasetRecord> callback) => Subscribe(_recordCallbacks, callback);

    /// <summary>Subscribe to state changes, called with the new state on every transition.</summary>
    public IDisposable OnState(Action<PlayerState> callback) => Subscribe(_stateCallbacks, callback);

    private IDisposable Subscribe<T>(List<Action<T>> list, Action<T> callback)
    {
        if (callback is null) throw new ArgumentNullException(nameof(callback));
        lock (_lock)
            list.Add(callback);
        return new Unsubscriber(() =>
        {
            lock (_lock)
                list.Remove(callback);
        });
    }

    /// <summary>Start, or resume after <see cref="Pause"/>; no-op while playing. From <see cref="PlayerState.Done"/>
    /// (or a position past the end) playback restarts at the first record. An empty dataset goes to done at once.</summary>
    public void Play()
    {
        lock (_lock)
        {
            CheckOpen();
            if (State == PlayerState.Playing)
                return;
            if (_idx >= _recs.Count)
                _idx = 0;
            Anchor(_clock());
            SetState(PlayerState.Playing);
            if (_recs.Count == 0)
            {
                SetState(PlayerState.Done);
                return;
            }
            EnsureThread();
            Monitor.PulseAll(_lock);
        }
    }

    /// <summary>Stop delivering; <see cref="Play"/> resumes from the next record, which then waits its full gap
    /// again. No-op unless playing.</summary>
    public void Pause()
    {
        lock (_lock)
        {
            if (State != PlayerState.Playing)
                return;
            SetState(PlayerState.Paused);
            Monitor.PulseAll(_lock);
        }
    }

    /// <summary>Move to <paramref name="offsetUs"/> µs from the first record: the next record delivered is the first
    /// with t ≥ first t + offset (negative: the start). Records in between are skipped. The state does not change,
    /// except that a seek past the end while playing ends playback.</summary>
    public void SeekUs(double offsetUs)
    {
        if (double.IsNaN(offsetUs))
            throw new ArgumentException("seek_us: offset must be a number", nameof(offsetUs));
        lock (_lock)
        {
            CheckOpen();
            if (_recs.Count == 0)
                return;
            double target = _recs[0].THostUs + offsetUs;
            int lo = 0, hi = _recs.Count;
            while (lo < hi)
            {
                int mid = (lo + hi) / 2;
                if (_recs[mid].THostUs < target)
                    lo = mid + 1;
                else
                    hi = mid;
            }
            _idx = lo;
            if (State == PlayerState.Playing)
            {
                if (_idx >= _recs.Count)
                    SetState(PlayerState.Done);
                else
                    Anchor(_clock());
                Monitor.PulseAll(_lock);
            }
        }
    }

    /// <summary>Deliver every record due now, on the calling thread. Returns the seconds until the next one is due,
    /// or null when not playing. For <c>threaded: false</c>; harmless with a player thread.</summary>
    public double? Tick()
    {
        lock (_lock)
        {
            while (true)
            {
                var (delivered, wait) = StepLocked();
                if (!delivered)
                    return wait;
            }
        }
    }

    /// <summary>Block until the player is not playing (done, or paused by someone else): true, or false on
    /// timeout. Needs the player thread.</summary>
    public bool Wait(TimeSpan? timeout = null)
    {
        if (!_threaded)
            throw new InvalidStateException("Wait() needs threaded: true; drive Tick() instead");
        var clock = Stopwatch.StartNew();
        lock (_lock)
        {
            while (State == PlayerState.Playing)
            {
                if (timeout is TimeSpan t)
                {
                    var left = t - clock.Elapsed;
                    if (left <= TimeSpan.Zero || !Monitor.Wait(_lock, left))
                        return State != PlayerState.Playing;
                }
                else
                {
                    Monitor.Wait(_lock);
                }
            }
            return true;
        }
    }

    /// <summary>Stop playback and the player thread. Idempotent.</summary>
    public void Close()
    {
        lock (_lock)
        {
            if (_closed)
                return;
            if (State == PlayerState.Playing)
                SetState(PlayerState.Paused);
            _closed = true;
            Monitor.PulseAll(_lock);
        }
        var t = _thread;
        if (t is not null && t != Thread.CurrentThread)
            t.Join(TimeSpan.FromSeconds(2));
        _thread = null;
    }

    public void Dispose() => Close();

    // ── internals (under _lock) ──────────────────────────────────────────────

    private static double CheckSpeed(double value)
    {
        if (double.IsNaN(value) || double.IsInfinity(value) || value < 0)
            throw new ArgumentOutOfRangeException(nameof(value), value, "speed must be a finite number >= 0");
        return value;
    }

    private void CheckOpen()
    {
        if (_closed)
            throw new InvalidStateException("DatasetPlayer is closed");
    }

    // The media clock restarts at `now` from the last delivered record's t (the first record's when none was).
    private void Anchor(double now)
    {
        _anchorWall = now;
        _anchorT = _recs.Count > 0 ? _recs[Math.Max(_idx - 1, 0)].THostUs : 0;
    }

    private double MediaNow(double now) =>
        _speed <= 0 ? _recs[Math.Max(_idx - 1, 0)].THostUs : _anchorT + (now - _anchorWall) * 1e6 * _speed;

    private void SetState(PlayerState state)
    {
        State = state;
        Monitor.PulseAll(_lock);
        foreach (var cb in _stateCallbacks.ToArray())
            cb(state);
    }

    // Deliver the next record if it is due → (true, _); else (false, seconds to wait) or (false, null) when not playing.
    private (bool Delivered, double? Wait) StepLocked()
    {
        if (State != PlayerState.Playing)
            return (false, null);
        if (_idx >= _recs.Count)
        {
            SetState(PlayerState.Done);
            return (false, null);
        }
        var rec = _recs[_idx];
        if (_speed > 0)
        {
            double now = _clock();
            double due = _anchorWall + (rec.THostUs - _anchorT) / 1e6 / _speed;
            if (due > now)
                return (false, due - now);
        }
        _idx++;
        try
        {
            foreach (var cb in _recordCallbacks.ToArray())
                cb(rec);
        }
        catch
        {
            // A failing consumer stops playback where it failed; the error goes to whoever drove delivery.
            if (State == PlayerState.Playing)
                SetState(PlayerState.Paused);
            throw;
        }
        if (State == PlayerState.Playing && _idx >= _recs.Count)
            SetState(PlayerState.Done);
        return (true, null);
    }

    private void EnsureThread()
    {
        if (!_threaded || _thread is { IsAlive: true })
            return;
        _thread = new Thread(Run) { IsBackground = true, Name = "depz-dataset-player" };
        _thread.Start();
    }

    private void Run()
    {
        lock (_lock)
        {
            while (!_closed)
            {
                (bool Delivered, double? Wait) r;
                try
                {
                    r = StepLocked();
                }
                catch (Exception e)
                {
                    DeliveryError?.Invoke(e);
                    continue;
                }
                if (r.Delivered)
                {
                    // let control calls (pause / seek from other threads) in between records, even at speed 0
                    Monitor.Wait(_lock, 0);
                    continue;
                }
                if (r.Wait is double s)
                    Monitor.Wait(_lock, TimeSpan.FromSeconds(s));
                else
                    Monitor.Wait(_lock);
            }
        }
    }

    private sealed class Unsubscriber : IDisposable
    {
        private Action? _undo;

        public Unsubscriber(Action undo) => _undo = undo;

        public void Dispose() => Interlocked.Exchange(ref _undo, null)?.Invoke();
    }
}
