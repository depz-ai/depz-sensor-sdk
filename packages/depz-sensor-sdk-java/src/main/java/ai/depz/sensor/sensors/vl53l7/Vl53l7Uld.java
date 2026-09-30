package ai.depz.sensor.sensors.vl53l7;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import ai.depz.sensor.sensors.vl53l8.Vl53l8Uld;

/**
 * VL53L5CX / VL53L7CX / VL53L7CH ULD — the host-verifiable decode layer only
 * (contracts/11_SENSOR_VL53L7.md, a delta against contracts/04_SENSOR_VL53L8.md).
 *
 * <p>One firmware app ({@code APP_VL53L7_*}) serves three boards: <b>VL53L7CX</b>
 * (base), <b>VL53L5CX</b> (same API, different optics) and <b>VL53L7CH</b> (CX plus
 * CNH, exactly as VL53L8CH). Frames, CNH decode and the chunk transport are the
 * VL53L8 ones ({@link Vl53l8Uld}, {@link ai.depz.sensor.sensors.vl53l8.FrameReassembler});
 * the L5/L7 differences in the decode path are:
 * <ul>
 *   <li>footer id at {@code size - 4} for every L5/L7 part (like L8 CH);</li>
 *   <li>per-target blocks (and CNH) keep their 64-entry size even in 4x4, so
 *       every per-zone array is trimmed to the frame's resolution.</li>
 * </ul>
 *
 * <p>OUT OF SCOPE, as for VL53L8: the live ULD init/config driver over the I2C
 * bridge; see {@link #liveDriverStubbed()}. Power modes, xtalk calibration,
 * detection thresholds and the motion indicator are the VL53L8 features with the
 * per-part deltas of contract 11 §4 (L5CX / L7CX: no threshold auto-stop, no
 * DEEP_SLEEP), checked against the ST sources and verified on hardware (L5CX and
 * L7CH, 2026-09-24); they run through that live driver (Python / TypeScript SDKs).
 */
public final class Vl53l7Uld {
    private Vl53l7Uld() {}

    public static final int RESOLUTION_4X4 = Vl53l8Uld.RESOLUTION_4X4;
    public static final int RESOLUTION_8X8 = Vl53l8Uld.RESOLUTION_8X8;

    /** Footer-id offset from the frame end: {@code size - 4} for L5CX, L7CX and L7CH. */
    public static final int FOOTER_ID_OFF = 4;

    /** L5/L7 range and stream at 1 Hz (VL53L8: 2 Hz). */
    public static final int MIN_RANGING_FREQUENCY_HZ = 1;

    /** The three sensor classes an {@code APP_VL53L7} board opens as. */
    public enum Model {
        VL53L5CX("vl53l5cx"),
        VL53L7CX("vl53l7cx"),
        VL53L7CH("vl53l7ch");

        public final String label;
        Model(String label) { this.label = label; }

        /** Model for a lowercase label ({@code "vl53l7ch"}), or {@code null}. */
        public static Model fromLabel(String label) {
            for (Model m : values()) {
                if (m.label.equals(label)) {
                    return m;
                }
            }
            return null;
        }
    }

    private static final Pattern PART_RE = Pattern.compile("VL53L([57])(CX|CH)");

    /**
     * Resolve the sensor class of an {@code APP_VL53L7} board (contract 11 §1,
     * normative): the production USB PID model first ({@code usbModel}, e.g.
     * from {@link ai.depz.sensor.usb.UsbIds#usbModelHint}; may be {@code null}),
     * else the first {@code VL53L<5|7><CX|CH>} in {@code GET_DEVICE_NAME}, else
     * {@link Model#VL53L7CX} (its blob runs on every L5/L7 part).
     */
    public static Model resolveModel(String usbModel, String deviceName) {
        Model byPid = usbModel == null ? null : Model.fromLabel(usbModel);
        if (byPid != null) {
            return byPid;
        }
        Matcher m = PART_RE.matcher(deviceName == null ? "" : deviceName);
        if (m.find()) {
            Model byName = Model.fromLabel(("vl53l" + m.group(1) + m.group(2)).toLowerCase());
            return byName != null ? byName : Model.VL53L7CX;
        }
        return Model.VL53L7CX;
    }

    /**
     * The live ULD init/config I2C-bridge driver is intentionally not ported to
     * Java — hardware-dependent and out of the decode scope (as for VL53L8).
     */
    public static boolean liveDriverStubbed() {
        return true;
    }

    /**
     * Parse one raw L5/L7 results frame for a known resolution (the one
     * {@code start_ranging} used: 16 or 64). Footer at {@code size - 4};
     * per-zone arrays trimmed to {@code resolution}. CNH (L7CH) lands in
     * {@link Vl53l8Uld.Results#cnhRaw}. Throws on a header/footer id mismatch.
     */
    public static Vl53l8Uld.Results parseFrame(byte[] raw, int dataReadSize, int resolution) {
        if (resolution != RESOLUTION_4X4 && resolution != RESOLUTION_8X8) {
            throw new Vl53l8Uld.Vl53l8Error(Vl53l8Uld.STATUS_INVALID_PARAM, "parseFrame resolution");
        }
        return Vl53l8Uld.parseFrame(raw, dataReadSize, FOOTER_ID_OFF, resolution);
    }

    /**
     * Parse one raw L5/L7 results frame, taking the resolution from the frame
     * itself: the zone-scaled ambient block (index 0x54D0) is always sized to the
     * resolution (contract 11 §3), so its entry count is the zone count.
     */
    public static Vl53l8Uld.Results parseFrame(byte[] raw, int dataReadSize) {
        return Vl53l8Uld.parseFrame(raw, dataReadSize, FOOTER_ID_OFF, Vl53l8Uld.RESOLUTION_FROM_FRAME);
    }
}
