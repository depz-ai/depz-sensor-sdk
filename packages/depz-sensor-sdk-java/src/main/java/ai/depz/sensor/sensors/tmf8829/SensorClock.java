package ai.depz.sensor.sensors.tmf8829;

import java.util.ArrayList;
import java.util.List;

/**
 * Sensor SYSTICK (125 kHz) → MCU µs (contracts/14_SENSOR_TMF8829.md §5), fed
 * one clock pair per frame (the MCU time of the header read and the
 * pre-header SYSTICK sampled by it). The ratio is taken over a sliding 1..2 s
 * window, first after 200 ms; it is also the distance correction (ams
 * {@code tmf8829ClockCorrection}).
 *
 * <p>Integer arithmetic exactly as the contract; {@code vectors/tmf8829_frame_time.json}
 * pins it bit for bit. µs values are 64-bit {@code long}; SYSTICK values are u32
 * held in {@code long} (0..2^32−1). Mirrors {@code depz_sensor_sdk/tmf8829/clock.py}.
 */
public final class SensorClock {
    /** SYSTICK 125 kHz = 8 MCU µs per tick, Q16. */
    public static final long NOMINAL_Q16 = 8L << 16;
    /** First ratio estimate. */
    public static final long CLOCK_WINDOW_MIN_US = 200_000L;
    /** Then a 1..2 s window. */
    public static final long CLOCK_WINDOW_SLIDE_US = 1_000_000L;
    public static final long EDGE_LATE_US = 2000L;

    /** Clock ratio not measured yet (nominal 8 µs/tick used). */
    public static final int FT_NOMINAL = 0x01;
    /** Clock model restarted at this frame (SYSTICK jump). */
    public static final int FT_CLOCK_RESET = 0x02;
    /** INT edge > 2 ms later after t1 than usual in this stream. */
    public static final int FT_EDGE_LATE = 0x04;
    /** Sub-integrations not back to back (histogram FIFO wait). */
    public static final int FT_STALLED = 0x08;
    /** Histograms + 32x32/48x32: T1 may be too long, centre late. */
    public static final int FT_T1_UNSURE = 0x10;

    private static final int[] FT_BITS = {FT_NOMINAL, FT_CLOCK_RESET, FT_EDGE_LATE, FT_STALLED, FT_T1_UNSURE};
    private static final String[] FT_NAMES = {"nominal", "clock_reset", "edge_late", "stalled", "t1_unsure"};

    /** Sub-integrations per results frame, index = fp_mode 0..5. */
    public static final int[] FP_SUBINTS = {2, 2, 2, 4, 4, 6};

    static final long U32 = 0xFFFFFFFFL;

    /** Names of the FT_* bits set in {@code flags}. */
    public static List<String> flagNames(int flags) {
        List<String> out = new ArrayList<>();
        for (int i = 0; i < FT_BITS.length; i++) {
            if ((flags & FT_BITS[i]) != 0) {
                out.add(FT_NAMES[i]);
            }
        }
        return out;
    }

    private boolean valid;
    private long lastUs;
    private long lastTick;
    private long anchorUs;
    private long anchorTick;
    private boolean haveMid;
    private long midUs;
    private long midTick;
    /** Current ratio, Q16 (u32); {@link #NOMINAL_Q16} until measured. */
    public long ratioQ16;
    /** True while the ratio is still the nominal one. */
    public boolean nominal;

    public SensorClock() {
        reset();
    }

    public void reset() {
        valid = false;
        lastUs = 0;
        lastTick = 0;
        anchorUs = 0;
        anchorTick = 0;
        haveMid = false;
        midUs = 0;
        midTick = 0;
        ratioQ16 = NOMINAL_Q16;
        nominal = true;
    }

    /** MCU/sensor clock ratio (≈ 1.015): distances are scaled by it. */
    public double ratio() {
        return (double) ratioQ16 / NOMINAL_Q16;
    }

    private static boolean stepOk(long dUs, long dTick) {
        long expect = dTick * 8;
        long tol = Math.floorDiv(dUs, 10L) + 50;
        return expect + tol >= dUs && expect <= dUs + tol;
    }

    /**
     * Add a pair → {@link #FT_CLOCK_RESET} if it does not fit the model (which
     * then restarts from it), else 0. {@code us} is a u64 in Java's
     * two's-complement convention; {@code tick} a u32 SYSTICK (outside
     * 0..2^32-1 → IllegalArgumentException, contract 07 §3).
     */
    public int add(long us, long tick) {
        ai.depz.sensor.protocol.Wire.checkUint("tick", tick, 32);
        int flags = 0;
        if (valid && !stepOk(us - lastUs, (tick - lastTick) & U32)) {
            valid = false;
            flags = FT_CLOCK_RESET;
        }
        lastUs = us;
        lastTick = tick;
        if (!valid) {
            valid = true;
            anchorUs = lastUs;
            anchorTick = lastTick;
            haveMid = false;
            ratioQ16 = NOMINAL_Q16;
            nominal = true;
            return flags;
        }
        long span = us - anchorUs;
        if (span >= CLOCK_WINDOW_MIN_US) {
            ratioQ16 = Math.floorDiv(span << 16, (tick - anchorTick) & U32) & U32;
            nominal = false;
        }
        if (!haveMid && span >= CLOCK_WINDOW_SLIDE_US) {
            haveMid = true;
            midUs = lastUs;
            midTick = lastTick;
        } else if (haveMid && span >= 2 * CLOCK_WINDOW_SLIDE_US) {
            anchorUs = midUs;
            anchorTick = midTick;
            haveMid = false;
        }
        return flags;
    }

    /** SYSTICK {@code tick} → MCU µs through the pair ({@code pairUs}, {@code pairTick}). */
    public long toUs(long pairUs, long pairTick, long tick) {
        return pairUs + (((long) (int) (tick - pairTick) * ratioQ16) >> 16);
    }
}
