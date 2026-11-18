package ai.depz.sensor.dataset;

import ai.depz.sensor.internal.Streams;
import ai.depz.sensor.json.JsonText;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.UncheckedIOException;
import java.nio.ByteBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.function.BooleanSupplier;
import java.util.function.Consumer;
import java.util.zip.CRC32;
import java.util.zip.DataFormatException;
import java.util.zip.Inflater;
import org.jspecify.annotations.Nullable;

/**
 * {@code .depzdata} dataset reader (contract 09): a JSONL
 * file whose first line is the header ({@code schema}, {@code devices}, ...)
 * and whose remaining lines are records {@code {"d","t","k","v"}}. Records are
 * stably merged onto one host timeline by {@code t} (µs): each device's records in
 * file order, equal {@code t} across devices in the order the devices first
 * appear in the file (the reference's per-device sort and merge). Mirror of the
 * Python {@code DatasetReader} (the name in TS and C# too).
 *
 * <p>{@link #read} opens a file (plain or gzip, {@code .depzdata.gz}) with the
 * SDK's own JSON; {@link #parse} takes the content and a caller-supplied line
 * parser.
 */
public final class DatasetReader {
    public static final String DATASET_SCHEMA_PREFIX = "depz.dataset/";

    /** One merged record. */
    // Contract 09's name; nested, so always written DatasetReader.Record and never mistaken for java.lang.Record.
    @SuppressWarnings("AvoidCommonTypeNames")
    public record Record(String deviceId, long tHostUs, String kind, Map<String, @Nullable Object> value) {}

    private final Map<String, Object> header;
    private final List<Record> records;

    private DatasetReader(Map<String, Object> header, List<Record> records) {
        this.header = header;
        this.records = records;
    }

    /**
     * Read the {@code .depzdata} file at {@code path}; gzip content (by its magic bytes) is inflated. A
     * damaged file, a cut or corrupt gzip container included, is {@link IllegalArgumentException}
     * (contract 09); a file that cannot be read {@link UncheckedIOException}.
     */
    public static DatasetReader read(Path path) {
        byte[] raw;
        try {
            raw = Files.readAllBytes(path);
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
        return read(raw, path.toString());
    }

    /**
     * Read a {@code .depzdata} the caller opened — an Android {@code ContentResolver} stream, an asset — as
     * {@link #read(Path)} reads a file, gzip content included. {@code in} is read to its end and left
     * open: it stays the caller's to close. {@code name} names the dataset in errors; a stream that fails
     * is {@link UncheckedIOException}.
     */
    public static DatasetReader read(InputStream in, String name) {
        Objects.requireNonNull(in, "in");
        Objects.requireNonNull(name, "name");
        byte[] raw;
        try {
            raw = Streams.readAll(in);
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
        return read(raw, name);
    }

    private static DatasetReader read(byte[] content, String source) {
        byte[] raw = content;
        // gzip magic (also covers .depzdata.gz files without extension check).
        if (raw.length >= 2 && (raw[0] & 0xFF) == 0x1F && (raw[1] & 0xFF) == 0x8B) {
            try {
                raw = gunzip(raw);
            } catch (IllegalArgumentException | DataFormatException e) {
                throw new IllegalArgumentException(source + ": cut or damaged gzip: " + e.getMessage(), e);
            }
        }
        String text;
        try {
            text = StandardCharsets.UTF_8.newDecoder().decode(ByteBuffer.wrap(raw)).toString();
        } catch (CharacterCodingException e) {
            // contract 09: UTF-8; a damaged file is the caller's bad argument, not silently other text
            throw new IllegalArgumentException(source + ": not UTF-8 text: " + e, e);
        }
        return parse(text, JsonText::parse);
    }

    /**
     * The content of a gzip file: members one after the other, each with its CRC-32 and length, zero
     * padding between and after them, as the reference reads it. Not GZIPInputStream, which ends quietly
     * at a next member cut in its header and at garbage after a member, the tail gone with no sign.
     */
    private static byte[] gunzip(byte[] raw) throws DataFormatException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        int pos = 0;
        while (pos < raw.length) {
            pos = raw[pos] == 0 ? pos + 1 : gunzipMember(raw, pos, out);
        }
        return out.toByteArray();
    }

    // One member at `pos` into `out` → where it ends.
    private static int gunzipMember(byte[] raw, int pos, ByteArrayOutputStream out) throws DataFormatException {
        int p = gzipHeaderEnd(raw, pos);
        CRC32 crc = new CRC32();
        long size = 0;
        Inflater inflater = new Inflater(true);
        try {
            inflater.setInput(raw, p, raw.length - p);
            byte[] buf = new byte[65536];
            while (!inflater.finished()) {
                int n = inflater.inflate(buf);
                if (n == 0 && (inflater.needsInput() || inflater.needsDictionary())) {
                    throw new IllegalArgumentException("unexpected end of the deflate data");
                }
                out.write(buf, 0, n);
                crc.update(buf, 0, n);
                size += n;
            }
            p += (int) inflater.getBytesRead();
        } finally {
            inflater.end();
        }
        if (raw.length - p < 8) {
            throw new IllegalArgumentException("unexpected end of the trailer");
        }
        if (le32(raw, p) != crc.getValue() || le32(raw, p + 4) != (size & 0xFFFFFFFFL)) {
            throw new IllegalArgumentException("wrong CRC-32 or length");
        }
        return p + 8;
    }

    // RFC 1952 §2.3: where the member header at `pos` ends.
    private static int gzipHeaderEnd(byte[] raw, int pos) {
        if (raw.length - pos < 10) {
            throw new IllegalArgumentException("unexpected end of a member header");
        }
        if ((raw[pos] & 0xFF) != 0x1F || (raw[pos + 1] & 0xFF) != 0x8B || raw[pos + 2] != 8) {
            throw new IllegalArgumentException("not a gzip member at byte " + pos);
        }
        int flags = raw[pos + 3];
        int p = pos + 10;
        if ((flags & 4) != 0) { // FEXTRA
            if (raw.length - p < 2) {
                throw new IllegalArgumentException("unexpected end of a member header");
            }
            p += 2 + (raw[p] & 0xFF) + ((raw[p + 1] & 0xFF) << 8);
        }
        for (int field : new int[] {8, 16}) { // FNAME, FCOMMENT: zero-terminated
            if ((flags & field) != 0) {
                while (p < raw.length && raw[p] != 0) {
                    p++;
                }
                p++;
            }
        }
        if ((flags & 2) != 0) { // FHCRC
            p += 2;
        }
        if (p > raw.length) {
            throw new IllegalArgumentException("unexpected end of a member header");
        }
        return p;
    }

    private static long le32(byte[] b, int at) {
        return (b[at] & 0xFFL) | (b[at + 1] & 0xFFL) << 8 | (b[at + 2] & 0xFFL) << 16 | (b[at + 3] & 0xFFL) << 24;
    }

    /**
     * Parse dataset content. {@code jsonLine} converts one JSON line to a
     * {@code Map<String,Object>} (objects → Map, arrays → List, ints → Long).
     */
    public static DatasetReader parse(String content, java.util.function.Function<String, Object> jsonLine) {
        String[] lines = content.split("\n", -1);
        if (content.isEmpty()) {
            throw new IllegalArgumentException("empty, not a .depzdata file (no header line)");
        }
        Map<String, Object> header = jsonObject(jsonLine, 1, lines[0]);
        Object schema = header.get("schema");
        if (!(schema instanceof String s) || !s.startsWith(DATASET_SCHEMA_PREFIX)) {
            throw new IllegalArgumentException("not a depz.dataset file: schema " + schema);
        }
        // Preserve file order for the stable sort tiebreak.
        List<Record> raw = new ArrayList<>();
        for (int i = 1; i < lines.length; i++) {
            if (JsonText.isBlankLine(lines[i])) {
                continue;
            }
            raw.add(record(jsonLine, i + 1, lines[i]));
        }
        // A stable sort by (t, the device's rank of first appearance, line) is the reference's
        // per-device stable sort followed by a merge (bug hunt 2026-11-03 JAVA-6).
        Map<String, Integer> rank = new HashMap<>();
        for (Record r : raw) {
            rank.putIfAbsent(r.deviceId(), rank.size());
        }
        List<Integer> idx = new ArrayList<>();
        for (int i = 0; i < raw.size(); i++) {
            idx.add(i);
        }
        idx.sort((a, b) -> {
            int c = Long.compare(raw.get(a).tHostUs(), raw.get(b).tHostUs());
            if (c == 0) {
                c = Integer.compare(rank.get(raw.get(a).deviceId()), rank.get(raw.get(b).deviceId()));
            }
            return c != 0 ? c : Integer.compare(a, b);
        });
        List<Record> sorted = new ArrayList<>();
        for (int i : idx) {
            sorted.add(raw.get(i));
        }
        return new DatasetReader(header, sorted);
    }

    /**
     * {@code line} as the JSON object a dataset line must be (contract 09), else
     * {@link IllegalArgumentException} naming the line — a damaged file is the caller's
     * bad argument, never a crash.
     */
    @SuppressWarnings("unchecked")
    private static Map<String, Object> jsonObject(java.util.function.Function<String, Object> jsonLine,
            int lineno, String line) {
        Object obj;
        try {
            obj = jsonLine.apply(line);
        } catch (IllegalArgumentException e) {
            throw new IllegalArgumentException("line " + lineno + ": not JSON: " + e.getMessage());
        }
        if (!(obj instanceof Map)) {
            throw new IllegalArgumentException("line " + lineno + ": not a JSON object");
        }
        return (Map<String, Object>) obj;
    }

    /** One record line: {@code d} and {@code k} strings, {@code t} an integer, {@code v} an object. */
    @SuppressWarnings("unchecked")
    private static Record record(java.util.function.Function<String, Object> jsonLine, int lineno, String line) {
        Map<String, Object> ev = jsonObject(jsonLine, lineno, line);
        Object d = ev.get("d");
        if (!(d instanceof String deviceId)) {
            throw new IllegalArgumentException("line " + lineno + ": 'd' must be a device id string, got " + d);
        }
        Object t = ev.get("t");
        if (!(t instanceof Long tHostUs)) {
            throw new IllegalArgumentException("line " + lineno + ": 't' must be an integer (host us), got " + t);
        }
        Object k = ev.get("k");
        if (!(k instanceof String kind)) {
            throw new IllegalArgumentException("line " + lineno + ": 'k' must be a record kind string, got " + k);
        }
        Object v = ev.get("v");
        if (!(v instanceof Map)) {
            throw new IllegalArgumentException("line " + lineno + ": 'v' must be an object, got " + v);
        }
        return new Record(deviceId, tHostUs, kind, (Map<String, Object>) v);
    }

    public Map<String, @Nullable Object> header() {
        return header;
    }

    public @Nullable String schema() {
        return (String) header.get("schema");
    }

    @SuppressWarnings("unchecked")
    public Map<String, @Nullable Object> devices() {
        Object d = header.get("devices");
        return d == null ? Map.of() : (Map<String, Object>) d;
    }

    /** The header's free-text note, or null. */
    public @Nullable String note() {
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
    public void play(Consumer<Record> callback, double speed, @Nullable Long startTUs, @Nullable BooleanSupplier cancelled) {
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
