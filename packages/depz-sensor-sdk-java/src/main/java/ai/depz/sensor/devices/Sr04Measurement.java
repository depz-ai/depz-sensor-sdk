package ai.depz.sensor.devices;

import ai.depz.sensor.protocol.Sr04;

/**
 * One SR04 ranging result. {@code source} is {@code "once"} (a host command or a SYNC_IN edge) or
 * {@code "loop"}; {@link #valid()} is false for the no-echo timeout.
 */
public record Sr04Measurement(long timestampUs, int echoTimeUs, String source) {
    public boolean valid() {
        return echoTimeUs != Sr04.ECHO_TIMEOUT;
    }

    /** Distance at 343 m/s, or null when no echo came back. */
    public Double distanceMm() {
        return Sr04.distanceMmFromEcho(echoTimeUs, null);
    }

    /** Distance with the speed of sound for {@code airTempC} °C. */
    public Double distanceMmAt(double airTempC) {
        return Sr04.distanceMmFromEcho(echoTimeUs, airTempC);
    }

    /**
     * Null for a source cmd contract 03 §2 does not define (0x36 / 0x37 only, ERRATA B11): not a
     * measurement, it falls through to the hex text event.
     */
    static Sr04Measurement from(Sr04.Sr04Data d) {
        String source;
        if (d.sourceCmd() == Sr04.Sr04Cmd.MEASURE_ONCE.value) {
            source = "once";
        } else if (d.sourceCmd() == Sr04.Sr04Cmd.START_MEASUREMENT_LOOP.value) {
            source = "loop";
        } else {
            return null;
        }
        return new Sr04Measurement(d.timestampUs(), d.echoTimeUs(), source);
    }
}
