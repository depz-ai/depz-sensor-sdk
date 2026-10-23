package ai.depz.sensor.sensors.vl53lx.bare;

import ai.depz.sensor.sensors.vl53lx.Vl53lxPlatform;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;

/**
 * The device's register image, block by block. The C driver never reads a configuration register
 * on its own: it edits the structs, then pushes whole blocks — ranging start writes 0x0001..0x0087
 * in a single 135-byte transfer. Every transfer costs a USB round trip, so block moves keep the
 * count where the C driver has it. Editing a field only touches the host copy; {@link #push}
 * reaches the sensor.
 */
public final class DeviceImage {
    /** The blocks {@code init_and_start_range} pushes, in address order: one transfer. */
    public static final List<String> RANGE_START_BLOCKS = List.of(
            "static_nvm_managed", "customer_nvm_managed", "static_config", "general_config", "timing_config",
            "dynamic_config", "system_control");

    private final Vl53lxPlatform p;
    private final Map<String, RegBlock> blocks = new LinkedHashMap<>();

    public DeviceImage(Vl53lxPlatform platform) {
        p = platform;
        for (String n : BareTables.BLOCKS.keySet()) {
            blocks.put(n, new RegBlock(n));
        }
    }

    public RegBlock get(String name) {
        RegBlock blk = blocks.get(name);
        if (blk == null) {
            throw new NoSuchElementException("no register block " + name);
        }
        return blk;
    }

    public RegBlock staticNvm() {
        return blocks.get("static_nvm_managed");
    }

    public RegBlock customerNvm() {
        return blocks.get("customer_nvm_managed");
    }

    public RegBlock staticConfig() {
        return blocks.get("static_config");
    }

    public RegBlock generalConfig() {
        return blocks.get("general_config");
    }

    public RegBlock timingConfig() {
        return blocks.get("timing_config");
    }

    public RegBlock dynamicConfig() {
        return blocks.get("dynamic_config");
    }

    public RegBlock systemControl() {
        return blocks.get("system_control");
    }

    public RegBlock nvmCopyData() {
        return blocks.get("nvm_copy_data");
    }

    /** Read one block off the sensor into the image. */
    public RegBlock pull(String name) {
        RegBlock blk = get(name);
        return blk.decode(p.readMulti(blk.base(), blk.size()));
    }

    /** Write one block from the image to the sensor. */
    public void push(String name) {
        RegBlock blk = get(name);
        if (!blk.def().writable()) {
            throw new IllegalStateException(name + " is read-only");
        }
        p.writeMulti(blk.base(), blk.encode());
    }

    // {base, total size} covering the named blocks; reserved bytes between them are part of the span.
    private static int[] span(List<String> names) {
        int lo = Integer.MAX_VALUE;
        int hi = Integer.MIN_VALUE;
        for (String n : names) {
            RegBlockDef d = BareTables.BLOCKS.get(n);
            lo = Math.min(lo, d.base());
            hi = Math.max(hi, d.base() + d.size());
        }
        return new int[] {lo, hi - lo};
    }

    public void pullRange(List<String> names) {
        int[] s = span(names);
        byte[] raw = p.readMulti(s[0], s[1]);
        for (String name : names) {
            RegBlock blk = get(name);
            byte[] part = new byte[blk.size()];
            System.arraycopy(raw, blk.base() - s[0], part, 0, blk.size());
            blk.decode(part);
        }
    }

    /** The named blocks as one transfer; a hole between them goes out as 0x00, as in the C driver. */
    public void pushRange(List<String> names) {
        int[] s = span(names);
        byte[] buf = new byte[s[1]];
        for (String name : names) {
            RegBlock blk = get(name);
            if (!blk.def().writable()) {
                throw new IllegalStateException(name + " is read-only");
            }
            byte[] enc = blk.encode();
            System.arraycopy(enc, 0, buf, blk.base() - s[0], enc.length);
        }
        p.writeMulti(s[0], buf);
    }

    /** Every field of the named blocks, one per line. */
    public String dump(Iterable<String> names) {
        List<String> lines = new ArrayList<>();
        for (String name : names) {
            RegBlock blk = get(name);
            lines.add(String.format("--- %s @ 0x%04X (%d B)", name, blk.base(), blk.size()));
            for (Map.Entry<String, Long> kv : blk.items()) {
                lines.add(String.format("  %-52s %d", kv.getKey(), kv.getValue()));
            }
        }
        return String.join("\n", lines);
    }

    /** Every field of every block, one per line. */
    public String dump() {
        return dump(BareTables.BLOCKS.keySet());
    }
}
