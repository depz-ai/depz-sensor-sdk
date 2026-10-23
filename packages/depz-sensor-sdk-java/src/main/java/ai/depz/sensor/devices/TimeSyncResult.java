package ai.depz.sensor.devices;

/**
 * Result of {@link DepzDevice#syncTime}: device clock − host clock, the round trip of the kept
 * sample, and when (host µs) it was taken.
 */
public record TimeSyncResult(long offsetUs, long rttUs, long syncedAtHostUs) {}
