package ai.depz.sensor.dataset;

import ai.depz.sensor.json.JsonText;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.UncheckedIOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.function.BooleanSupplier;
import java.util.function.Consumer;
import java.util.zip.GZIPInputStream;

/**
 * {@code .depzdata} dataset reader (contracts/09_DATASET_FORMAT.md): a JSONL
 * file whose first line is the header ({@code schema}, {@code devices}, ...)
 * and whose remaining lines are records {@code {"d","t","k","v"}}. Records are
 * stably merged onto one host timeline by {@code t} (µs). Mirror of the
 * TS/Python {@code DatasetReader}.
 *
 * <p>{@link #read} opens a file (plain or gzip, {@code .depzdata.gz}) with the
 * SDK's own JSON; {@link #parse} takes the content and a caller-supplied line
 * parser.
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

    /** Read the {@code .depzdata} file at {@code path}; gzip content (by its magic bytes) is inflated. */
    public static Dataset read(Path path) {
        try {
            byte[] raw = Files.readAllBytes(path);
            // gzip magic (also covers .depzdata.gz files without extension check).
            if (raw.length >= 2 && (raw[0] & 0xFF) == 0x1F && (raw[1] & 0xFF) == 0x8B) {
                try (InputStream gz = new GZIPInputStream(new ByteArrayInputStream(raw))) {
                    raw = gz.readAllBytes();
                }
            }
            return parse(new String(raw, StandardCharsets.UTF_8), JsonText::parse);
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
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
            Map<String, Object> v = ev.get("v") instanceof Map<?, ?> m ? (Map<String, Object>) m : Map.of();
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

    /** The header's free-text note, or null. */
    public String note() {
        return header.get("note") instanceof String n ? n : null;
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

    /**
     * Deliver the records paced by their timestamps ({@code speed} 2 = twice as fast, 0 = as fast as
     * possible), blocking until the end or until {@code cancelled} returns true (polled before each record
     * and during waits). Records before {@code startTUs} (null: none) are skipped. For pause / seek / speed
     * changes use {@link DatasetPlayer}.
     */
    public void play(Consumer<Record> callback, double speed, Long startTUs, BooleanSupplier cancelled) {
        if (callback == null) {
            throw new IllegalArgumentException("callback must not be null");
        }
        if (Double.isNaN(speed) || Double.isInfinite(speed) || speed < 0) {
            throw new IllegalArgumentException("speed must be a finite number >= 0, got " + speed);
        }
        BooleanSupplier cancel = cancelled != null ? cancelled : () -> false;
        Long firstT = null;
        long wallStart = System.nanoTime();
        for (Record rec : records) {
            if (startTUs != null && rec.tHostUs() < startTUs) {
                continue;
            }
            if (cancel.getAsBoolean()) {
                return;
            }
            if (speed > 0) {
                if (firstT == null) {
                    firstT = rec.tHostUs();
                }
                double dueS = (rec.tHostUs() - firstT) / 1e6 / speed;
                if (waitUntil(wallStart, dueS, cancel)) {
                    return;
                }
            }
            callback.accept(rec);
        }
    }

    /** {@link #play(Consumer, double, Long, BooleanSupplier)} from the first record, not cancellable. */
    public void play(Consumer<Record> callback, double speed) {
        play(callback, speed, null, null);
    }

    /** {@link #play(Consumer, double, Long, BooleanSupplier)} in real time from the first record. */
    public void play(Consumer<Record> callback) {
        play(callback, 1.0, null, null);
    }

    // Sleep until dueS seconds after wallStart (in slices, polling cancel) → true when cancelled.
    private static boolean waitUntil(long wallStart, double dueS, BooleanSupplier cancel) {
        while (true) {
            double left = dueS - (System.nanoTime() - wallStart) / 1e9;
            if (left <= 0) {
                return false;
            }
            if (cancel.getAsBoolean()) {
                return true;
            }
            long ns = (long) (Math.min(left, 0.01) * 1e9);
            try {
                Thread.sleep(ns / 1_000_000, (int) (ns % 1_000_000));
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                return true;
            }
        }
    }
}
