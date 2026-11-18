namespace Depz.Sensor.Devices;

/// <summary>What a timeout means to the waits underneath: Monitor.Wait and ManualResetEventSlim.Wait take
/// up to int.MaxValue ms (24.8 days) and throw past it. A timeout that long, like
/// <see cref="Timeout.InfiniteTimeSpan"/>, is a wait without a deadline (bug hunt 2026-11-03 CSHARP-1, -5).</summary>
internal static class Waits
{
    private static readonly TimeSpan Longest = TimeSpan.FromMilliseconds(int.MaxValue);

    public static bool IsUnbounded(TimeSpan timeout) => timeout == Timeout.InfiniteTimeSpan || timeout > Longest;

    /// <summary>The timeout as ManualResetEventSlim.Wait takes it.</summary>
    public static TimeSpan ForWait(TimeSpan timeout) => IsUnbounded(timeout) ? Timeout.InfiniteTimeSpan : timeout;

    /// <summary>The timeout as a deadline on a clock takes it: a poll loop's <c>elapsed &gt; timeout</c> never holds
    /// for an unbounded one.</summary>
    public static TimeSpan ForDeadline(TimeSpan timeout) => IsUnbounded(timeout) ? TimeSpan.MaxValue : timeout;

    /// <summary>The timeout where a sum with "now" must not overflow: the longest wait.</summary>
    public static TimeSpan Capped(TimeSpan timeout) => IsUnbounded(timeout) ? Longest : timeout;

    /// <summary>The timeout in whole milliseconds, an unbounded one saturating.</summary>
    public static int Milliseconds(TimeSpan timeout) =>
        IsUnbounded(timeout) ? int.MaxValue : (int)timeout.TotalMilliseconds;
}
