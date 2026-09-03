package ai.depz.sensor.dataset;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * {@code .depzdata} dataset reader (contracts/09_DATASET_FORMAT.md): a JSONL
 * file whose first line is the header ({@code schema}, {@code devices}, ...)
 * and whose remaining lines are records {@code {"d","t","k","v"}}. Records are
 * stably merged onto one host timeline by {@code t} (µs). Mirror of the
 * TS/Python {@code DatasetReader}.
 *
 * <p>JSON parsing is delegated to a caller-supplied line parser so this module
 * has no dependency on the test harness's JSON code; see {@link #parse}.
 */
public final class Dataset {
    public static final String DATASET_SCHEMA_PREFIX = "depz.dataset/";

    /** One merged record. */
    public record Record(String deviceId, long tHostUs, String kind, Map<String, Object> value) {}

    private final Map<String, Object> header;
    private final List<Record> records;

    private Dataset(Map<String, Object> header, List<Record> records) {
        this.header = header;
        this.records = records;
    }

    /**
     * Parse dataset content. {@code jsonLine} converts one JSON line to a
     * {@code Map<String,Object>} (objects → Map, arrays → List, ints → Long).
     */
    public static Dataset parse(String content, java.util.function.Function<String, Object> jsonLine) {
        List<String> lines = new ArrayList<>();
        for (String l : content.split("\n")) {
            if (!l.trim().isEmpty()) {
                lines.add(l);
            }
        }
        if (lines.isEmpty()) {
            throw new IllegalArgumentException("empty dataset");
        }
        @SuppressWarnings("unchecked")
        Map<String, Object> header = (Map<String, Object>) jsonLine.apply(lines.get(0));
        Object schema = header.get("schema");
        if (!(schema instanceof String s) || !s.startsWith(DATASET_SCHEMA_PREFIX)) {
            throw new IllegalArgumentException("not a depz.dataset file");
        }
        // Preserve file order for the stable sort tiebreak.
        List<Record> raw = new ArrayList<>();
        for (int i = 1; i < lines.size(); i++) {
            @SuppressWarnings("unchecked")
            Map<String, Object> ev = (Map<String, Object>) jsonLine.apply(lines.get(i));
            @SuppressWarnings("unchecked")
            Map<String, Object> v = (Map<String, Object>) ev.get("v");
            raw.add(new Record(
                    (String) ev.get("d"), ((Number) ev.get("t")).longValue(), (String) ev.get("k"), v));
        }
        // Per-device order is monotonic; a stable sort by tHostUs merges exactly.
        List<Integer> idx = new ArrayList<>();
        for (int i = 0; i < raw.size(); i++) {
            idx.add(i);
        }
        idx.sort((a, b) -> {
            int c = Long.compare(raw.get(a).tHostUs(), raw.get(b).tHostUs());
            return c != 0 ? c : Integer.compare(a, b);
        });
        List<Record> sorted = new ArrayList<>();
        for (int i : idx) {
            sorted.add(raw.get(i));
        }
        return new Dataset(header, sorted);
    }

    public Map<String, Object> header() {
        return header;
    }

    public String schema() {
        return (String) header.get("schema");
    }

    @SuppressWarnings("unchecked")
    public Map<String, Object> devices() {
        Object d = header.get("devices");
        return d == null ? Map.of() : (Map<String, Object>) d;
    }

    public List<Record> records() {
        return records;
    }

    public long durationUs() {
        if (records.isEmpty()) {
            return 0;
        }
        return records.get(records.size() - 1).tHostUs() - records.get(0).tHostUs();
    }
}
