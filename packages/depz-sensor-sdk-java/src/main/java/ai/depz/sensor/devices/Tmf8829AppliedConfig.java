package ai.depz.sensor.devices;

import ai.depz.sensor.sensors.tmf8829.Frames;
import java.util.Map;

/**
 * The config {@link Tmf8829Device#configure} wrote, as read back from the sensor: the preset name, the
 * 190-byte page 0x22..0xDF, whether histogram frames are dumped and a ref-SPAD frame comes with each
 * measurement, and every named field of the page.
 */
public record Tmf8829AppliedConfig(String preset, byte[] page, boolean histograms, boolean refFrame,
        Map<String, Integer> fields) {
    /** {rows, cols} of a measurement. */
    public int[] grid() {
        return Frames.FP_GRID[fields.get("fp_mode")].clone();
    }
}
