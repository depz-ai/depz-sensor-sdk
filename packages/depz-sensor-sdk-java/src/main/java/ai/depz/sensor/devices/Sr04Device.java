package ai.depz.sensor.devices;

import ai.depz.sensor.protocol.Sr04;
import ai.depz.sensor.transport.Packet;
import java.time.Duration;
import org.jspecify.annotations.Nullable;

/**
 * HC-SR04 ultrasonic ranger (contract 03). Measurements — loop samples and the single shots an AUX
 * SYNC_IN edge triggers — are the {@link #measurements()} stream.
 */
public final class Sr04Device extends DepzDevice {
    private final SensorStream<Sr04Measurement> measurements = stream(Sr04.Sr04Rpt.DATA.value, 256);

    public Sr04Device(Link link, @Nullable Duration timeout) {
        super(link, timeout);
    }

    public Sr04Device(Link link) {
        super(link);
    }

    Sr04Device(DepzDevice from) {
        super(from);
    }

    // ── configuration ────────────────────────────────────────────────────────

    /**
     * The stored minimum interval between measurement starts, µs (default 50000) — not the
     * effective rate, which the echo window throttles (contract 03 §3).
     */
    public long getSamplePeriodUs() {
        return request(Sr04.Sr04Cmd.GET_SAMPLE_PERIOD.value, null,
                expectReport(Sr04.Sr04Rpt.SAMPLE_PERIOD.value, Sr04::unpackSamplePeriod));
    }

    /** Set the minimum interval between measurement starts, µs (u32 wire field). */
    public void setSamplePeriodUs(long periodUs) {
        if (periodUs < 0 || periodUs > 0xFFFFFFFFL) {
            throw new IllegalArgumentException("sample period must be 0..4294967295 µs (u32 wire field, contract 03 §2), got "
                    + periodUs);
        }
        requestOk(Sr04.Sr04Cmd.SET_SAMPLE_PERIOD.value, Sr04.packSamplePeriod(periodUs));
    }

    /** The post-echo settle pause, µs (4000–65000). */
    public int getEchoDecayUs() {
        return request(Sr04.Sr04Cmd.GET_ECHO_DECAY.value, null,
                expectReport(Sr04.Sr04Rpt.ECHO_DECAY.value, Sr04::unpackEchoDecay));
    }

    /**
     * Set the settle pause. The device clamps it to 4000–65000 µs silently, so this reads it back
     * and returns the value in effect. Must fit the u16 wire field.
     */
    public int setEchoDecayUs(int decayUs) {
        if (decayUs < 0 || decayUs > 0xFFFF) {
            throw new IllegalArgumentException("echo decay must be 0..65535 µs (u16 wire field; the device then clamps to "
                    + Sr04.ECHO_DECAY_MIN_US + ".." + Sr04.ECHO_DECAY_MAX_US + ", contract 03 §3), got " + decayUs);
        }
        requestOk(Sr04.Sr04Cmd.SET_ECHO_DECAY.value, Sr04.packEchoDecay(decayUs));
        return getEchoDecayUs();
    }

    // ── measuring ────────────────────────────────────────────────────────────

    /**
     * One shot. The reply comes when the echo completes (or times out at ~65.5 ms), hence the 1 s
     * default. {@link BusyException} while the loop runs.
     */
    public Sr04Measurement measureOnce(@Nullable Duration timeout) {
        return request(Sr04.Sr04Cmd.MEASURE_ONCE.value, null, (Packet p) -> {
            // the reply is the RPT_DATA sourced by this command; a short one fails the request
            // (InvalidReplyException, contract 07 §3), no timeout
            if (p.cmd() != Sr04.Sr04Rpt.DATA.value || p.payload().length < 1
                    || (p.payload()[0] & 0xFF) != Sr04.Sr04Cmd.MEASURE_ONCE.value) {
                return null;
            }
            return Sr04Measurement.from(Sr04.Sr04Data.unpack(p.payload()));
        }, timeout == null ? Duration.ofSeconds(1) : timeout);
    }

    public Sr04Measurement measureOnce() {
        return measureOnce(null);
    }

    /** Start the measurement loop (idempotent). */
    public void start() {
        requestOk(Sr04.Sr04Cmd.START_MEASUREMENT_LOOP.value);
    }

    /** Stop the measurement loop (idempotent). */
    public void stop() {
        requestOk(Sr04.Sr04Cmd.STOP_MEASUREMENT_LOOP.value);
    }

    /**
     * The measurement stream (contract 07 §3): {@code measurements().on(cb)} subscribes (reader
     * thread; don't block), {@code measurements().subscribe(capacity)} queues (bounded, drop-oldest,
     * subscribed when it returns — subscribe before {@link #start()} and nothing is missed),
     * {@code measurements().next(timeout)} waits for one; {@code parseErrors()} counts the RPT_DATA
     * reports shorter than their 11 bytes (contract 07 §2).
     */
    public SensorStream<Sr04Measurement> measurements() {
        return measurements;
    }

    @Override
    protected boolean handleReport(Packet packet) {
        if (measurements == null || packet.cmd() != Sr04.Sr04Rpt.DATA.value) {
            return false;
        }
        Sr04Measurement m;
        try {
            // short: refused and counted — contract 07 §2
            m = Sr04Measurement.from(Sr04.Sr04Data.unpack(packet.payload()));
        } catch (IllegalArgumentException e) {
            measurements.countParseError();
            return true;
        }
        if (m == null) {
            return false;
        }
        measurements.emit(m);
        return true;
    }
}
