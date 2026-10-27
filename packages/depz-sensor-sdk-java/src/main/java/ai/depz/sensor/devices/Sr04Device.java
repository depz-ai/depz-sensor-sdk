package ai.depz.sensor.devices;

import ai.depz.sensor.protocol.Sr04;
import ai.depz.sensor.transport.Packet;
import java.time.Duration;
import java.util.List;
import java.util.function.Consumer;

/**
 * HC-SR04 ultrasonic ranger (contract 03). Measurements stream to {@link #onMeasurement}
 * callbacks and {@link #measurements} queues: loop samples and the single shots an AUX SYNC_IN
 * edge triggers.
 */
public final class Sr04Device extends DepzDevice {
    private static final int DATA_MIN_BYTES = 11;
    private final Fanout<Sr04Measurement> measurements = new Fanout<>();

    public Sr04Device(Link link, Duration timeout) {
        super(link, timeout);
    }

    public Sr04Device(Link link) {
        super(link);
    }

    public Sr04Device(String port, Duration timeout) {
        super(port, timeout);
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
    public Sr04Measurement measureOnce(Duration timeout) {
        return request(Sr04.Sr04Cmd.MEASURE_ONCE.value, null, (Packet p) -> {
            if (p.cmd() != Sr04.Sr04Rpt.DATA.value || p.payload().length < DATA_MIN_BYTES) {
                return null;
            }
            Sr04.Sr04Data d = Sr04.Sr04Data.unpack(p.payload());
            return d.sourceCmd() == Sr04.Sr04Cmd.MEASURE_ONCE.value ? Sr04Measurement.from(d) : null;
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

    /** Call {@code callback} for every measurement (reader thread). Close the result to stop. */
    public AutoCloseable onMeasurement(Consumer<Sr04Measurement> callback) {
        return measurements.onItem(callback);
    }

    /** A pull queue of measurements (bounded, drop-oldest); subscribed when this returns. */
    public Subscription<Sr04Measurement> measurements(int capacity) {
        return measurements.subscribe(capacity);
    }

    public Subscription<Sr04Measurement> measurements() {
        return measurements(256);
    }

    /** The next measurement (contract 07 §3). */
    public Sr04Measurement getMeasurement(Duration timeout) {
        return measurements.next(timeout == null ? Duration.ofSeconds(2) : timeout, Sr04.Sr04Rpt.DATA.value, this::isClosed);
    }

    /** The drop counter of each open {@link #measurements} queue. */
    public List<Long> streamDroppedCounts() {
        return measurements.droppedCounts();
    }

    @Override
    protected boolean handleReport(Packet packet) {
        if (measurements == null || packet.cmd() != Sr04.Sr04Rpt.DATA.value || packet.payload().length < DATA_MIN_BYTES) {
            return false;
        }
        Sr04Measurement m = Sr04Measurement.from(Sr04.Sr04Data.unpack(packet.payload()));
        if (m == null) {
            return false;
        }
        measurements.emit(m, stats());
        return true;
    }

    @Override
    protected void onReaderStopped() {
        if (measurements != null) {
            measurements.closeAll();
        }
        super.onReaderStopped();
    }
}
