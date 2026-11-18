package ai.depz.sensor.sensors.vl53lx.bare;

import java.util.AbstractMap;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;

/**
 * One register block's worth of device image, addressed by the C field name:
 * {@code img.staticConfig().get("dss_config__target_total_rate_mcps")}. Bytes no field names (the
 * reserved holes) keep whatever was read, so a read-modify-write round trip is byte-exact.
 */
public final class RegBlock {
    private final String name;
    private final RegBlockDef def;
    private final Map<String, Long> v = new HashMap<>();
    private byte[] raw;

    public RegBlock(String name) {
        RegBlockDef d = BareTables.BLOCKS.get(name);
        if (d == null) {
            throw new NoSuchElementException("no register block " + name);
        }
        this.name = name;
        this.def = d;
        for (RegField f : d.fields()) {
            v.put(f.name(), 0L);
        }
        raw = new byte[d.size()];
    }

    public String name() {
        return name;
    }

    public RegBlockDef def() {
        return def;
    }

    public int base() {
        return def.base();
    }

    public int size() {
        return def.size();
    }

    /** The field by its C name; {@link NoSuchElementException} when the block has none. */
    public long get(String field) {
        Long value = v.get(field);
        if (value == null) {
            throw new NoSuchElementException(name + " has no field " + field);
        }
        return value;
    }

    public void set(String field, long value) {
        if (!v.containsKey(field)) {
            throw new NoSuchElementException(name + " has no field " + field);
        }
        v.put(field, value);
    }

    /** Every field in block order, {name, value}. */
    public List<Map.Entry<String, Long>> items() {
        List<Map.Entry<String, Long>> out = new ArrayList<>();
        for (RegField f : def.fields()) {
            out.add(new AbstractMap.SimpleImmutableEntry<>(f.name(), v.get(f.name())));
        }
        return out;
    }

    public RegBlock decode(byte[] raw) {
        if (raw.length < size()) {
            throw new IllegalArgumentException(name + ": " + raw.length + " bytes, need " + size());
        }
        this.raw = Arrays.copyOf(raw, size());
        for (RegField f : def.fields()) {
            long value = 0;
            for (int i = 0; i < f.width(); i++) {
                value = (value << 8) | (raw[f.off() + i] & 0xFF);
            }
            if (f.mask() != null) {
                value &= f.mask();
            }
            int bits = f.width() * 8;
            if (f.signed() && value >= 1L << (bits - 1)) {
                value -= 1L << bits;
            }
            v.put(f.name(), value);
        }
        return this;
    }

    public byte[] encode() {
        byte[] buf = raw.clone();
        for (RegField f : def.fields()) {
            // The field's mask and width, the way the C codec clips.
            long value = v.get(f.name());
            int bits = f.width() * 8;
            long all = bits == 64 ? -1L : (1L << bits) - 1;
            if (f.signed()) {
                value &= all;
            }
            if (f.mask() != null) {
                value &= f.mask();
            }
            value &= all;
            for (int i = f.width() - 1; i >= 0; i--) {
                buf[f.off() + i] = (byte) value;
                value >>= 8;
            }
        }
        return buf;
    }

    @Override
    public String toString() {
        return String.format("<%s 0x%04X+%d>", name, base(), size());
    }
}
