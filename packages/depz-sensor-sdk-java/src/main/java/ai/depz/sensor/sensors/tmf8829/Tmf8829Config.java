package ai.depz.sensor.sensors.tmf8829;

import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * TMF8829 presets and config-page fields (contracts/14_SENSOR_TMF8829.md §4).
 * Mirrors {@code depz_sensor_sdk/tmf8829/config.py}.
 */
public final class Tmf8829Config {
    private Tmf8829Config() {}

    /** Config page 0x22..0xDF. */
    public static final int REG_CFG = 0x22;
    /** 190 B. */
    public static final int CFG_SIZE = 0xDF - REG_CFG + 1;

    /** The sensor's own period after {@code init()}. */
    public static final int DEFAULT_PERIOD_MS = 33;

    /** Preset name → load command (each loads its config page). */
    public static final Map<String, Integer> PRESETS;

    /** The preset's own kilo_iterations (a preset rescales a stale value, it does not reset it). */
    public static final Map<String, Integer> PRESET_KILO_ITERATIONS;

    /** A config-page field: register, bit shift, width in bits (16 = little-endian register pair). */
    public record CfgField(int reg, int shift, int width) {}

    /** Field name → location, in contract order. */
    public static final Map<String, CfgField> CFG_FIELDS;

    static {
        Map<String, Integer> p = new LinkedHashMap<>();
        p.put("8x8", 0x40);
        p.put("8x8-lr", 0x41); // long range
        p.put("8x8-ha", 0x42); // high accuracy
        p.put("16x16", 0x43);
        p.put("16x16-ha", 0x44);
        p.put("32x32", 0x45);
        p.put("32x32-ha", 0x46);
        p.put("48x32", 0x47);
        p.put("48x32-ha", 0x48);
        p.put("8x8-xr", 0x49); // extended range, distances in mm
        PRESETS = Collections.unmodifiableMap(p);

        Map<String, Integer> k = new LinkedHashMap<>();
        for (String n : new String[] {"8x8", "8x8-lr", "8x8-ha", "8x8-xr", "16x16", "16x16-ha"}) {
            k.put(n, 586);
        }
        k.put("32x32", 2344);
        k.put("32x32-ha", 2344);
        k.put("48x32", 3516);
        k.put("48x32-ha", 3516);
        PRESET_KILO_ITERATIONS = Collections.unmodifiableMap(k);

        Map<String, CfgField> f = new LinkedHashMap<>();
        f.put("period_ms", new CfgField(0x22, 0, 16));
        f.put("kilo_iterations", new CfgField(0x24, 0, 16));
        f.put("fp_mode", new CfgField(0x26, 0, 3));
        f.put("nr_peaks", new CfgField(0x2A, 0, 3));
        f.put("signal_strength", new CfgField(0x2A, 3, 1));
        f.put("noise_strength", new CfgField(0x2A, 4, 1));
        f.put("xtalk", new CfgField(0x2A, 5, 1));
        f.put("dump_histograms", new CfgField(0x2B, 0, 1));
        f.put("ref_spad_frame", new CfgField(0x2C, 0, 1));
        f.put("power_modes", new CfgField(0x2E, 0, 8));
        f.put("ha_kilo_iterations", new CfgField(0x4A, 0, 16));
        f.put("dual_mode", new CfgField(0x4C, 0, 2));
        f.put("fov_correction", new CfgField(0x78, 0, 4));
        f.put("distance_in_mm", new CfgField(0x8F, 0, 1));
        CFG_FIELDS = Collections.unmodifiableMap(f);
    }

    private static CfgField field(String name) {
        CfgField f = CFG_FIELDS.get(name);
        if (f == null) {
            throw new IllegalArgumentException("unknown config field " + name);
        }
        return f;
    }

    /** Read a named field from a config page (190 B starting at 0x22). */
    public static int cfgGet(byte[] page, String name) {
        CfgField f = field(name);
        int i = f.reg() - REG_CFG;
        if (f.width() == 16) {
            return (page[i] & 0xFF) | ((page[i + 1] & 0xFF) << 8);
        }
        return ((page[i] & 0xFF) >> f.shift()) & ((1 << f.width()) - 1);
    }

    /** Write a named field into a config page in place; out-of-range values are refused. */
    public static void cfgSet(byte[] page, String name, int value) {
        CfgField f = field(name);
        int i = f.reg() - REG_CFG;
        if (value < 0 || value >= (1 << f.width())) {
            throw new IllegalArgumentException(name + ": " + value + " out of range for " + f.width() + " bit");
        }
        if (f.width() == 16) {
            page[i] = (byte) value;
            page[i + 1] = (byte) (value >>> 8);
        } else {
            int mask = ((1 << f.width()) - 1) << f.shift();
            page[i] = (byte) (((page[i] & 0xFF) & ~mask) | (value << f.shift()));
        }
    }

    /** Every named field of a config page. */
    public static Map<String, Integer> configFields(byte[] page) {
        Map<String, Integer> out = new LinkedHashMap<>();
        for (String name : CFG_FIELDS.keySet()) {
            out.put(name, cfgGet(page, name));
        }
        return out;
    }
}
