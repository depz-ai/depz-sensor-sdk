package ai.depz.sensor.dataset;

import ai.depz.sensor.devices.InvalidStateException;
import ai.depz.sensor.json.JsonText;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.UncheckedIOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Instant;
import java.time.ZoneOffset;
import java.time.format.DateTimeFormatter;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.zip.GZIPOutputStream;
import org.jspecify.annotations.Nullable;

/**
 * Low-level thread-safe {@code .depzdata} record writer (gzip when the path ends in {@code .gz}).
 * Prefer {@link SessionRecorder}.
 */
public final class DatasetWriter implements AutoCloseable {
    /** The schema id this SDK writes. */
    public static final String SCHEMA = "depz.dataset/2";

    private static final DateTimeFormatter UTC = DateTimeFormatter.ofPattern("yyyy-MM-dd'T'HH:mm:ss'Z'").withZone(ZoneOffset.UTC);

    private final Object lock = new Object();
    private final BufferedWriter writer;
    private long recordsWritten;
    private boolean closed;

    /**
     * @param path the file to create (overwritten)
     * @param devices header device entries by device id ({@code serial}, {@code sensor_type},
     *     {@code software_name}, {@code time_sync}, …)
     * @param note optional free text for the header ("" for none)
     */
    public DatasetWriter(Path path, Map<String, Map<String, Object>> devices, @Nullable String note) {
        // the header before the file is touched: refused, it stays as it was and no handle is left open
        this(headerLine("path", path, devices, note), open(path));
    }

    public DatasetWriter(Path path, Map<String, Map<String, Object>> devices) {
        this(path, devices, "");
    }

    /**
     * Write into a stream the caller opened — an Android {@code ContentResolver} stream — as the
     * {@code Path} constructor writes a file. The bytes go out as they are: for a {@code .depzdata.gz}
     * pass a {@link GZIPOutputStream}. {@link #close} closes {@code out}.
     *
     * @param devices header device entries by device id, as for a file
     * @param note optional free text for the header ("" for none)
     */
    public DatasetWriter(OutputStream out, Map<String, Map<String, Object>> devices, @Nullable String note) {
        this(headerLine("out", out, devices, note), writer(out));
    }

    public DatasetWriter(OutputStream out, Map<String, Map<String, Object>> devices) {
        this(out, devices, "");
    }

    private DatasetWriter(String headerLine, BufferedWriter writer) {
        this.writer = writer;
        try {
            writer.write(headerLine);
            writer.write('\n');
        } catch (IOException e) {
            // The caller gets no writer to close: the stream (theirs or the opened file) closes here.
            try {
                writer.close();
            } catch (IOException closing) {
                e.addSuppressed(closing);
            }
            throw new UncheckedIOException(e);
        }
    }

    /** The header line; {@code target} (the path or the stream, called {@code what}) and {@code devices} must not be null. */
    private static String headerLine(String what, @Nullable Object target, @Nullable Map<String, Map<String, Object>> devices,
            @Nullable String note) {
        if (target == null || devices == null) {
            throw new IllegalArgumentException(what + " and devices must not be null");
        }
        Map<String, Object> header = new LinkedHashMap<>();
        header.put("schema", SCHEMA);
        header.put("created_utc", UTC.format(Instant.now()));
        header.put("devices", new LinkedHashMap<>(devices));
        if (note != null && !note.isEmpty()) {
            header.put("note", note);
        }
        return JsonText.serialize(header);
    }

    private static BufferedWriter open(Path path) {
        try {
            OutputStream out = Files.newOutputStream(path);
            if (path.getFileName().toString().toLowerCase(java.util.Locale.ROOT).endsWith(".gz")) {
                out = new GZIPOutputStream(out);
            }
            return writer(out);
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    private static BufferedWriter writer(OutputStream out) {
        return new BufferedWriter(new OutputStreamWriter(out, StandardCharsets.UTF_8));
    }

    public long recordsWritten() {
        synchronized (lock) {
            return recordsWritten;
        }
    }

    /**
     * One record: device id, host µs, kind, value (contract 09). A null device id, kind or value, a value
     * that holds NaN / infinity or nests past the line rules, a lone surrogate in a string is refused before anything is written
     * ({@link IllegalArgumentException}): no reader would take the line.
     */
    public void write(String deviceId, long tHostUs, String kind, Map<String, Object> value) {
        if (deviceId == null || kind == null || value == null) {
            throw new IllegalArgumentException("a dataset record needs a device id, a kind and an object value");
        }
        Map<String, Object> rec = new LinkedHashMap<>();
        rec.put("d", deviceId);
        rec.put("t", tHostUs);
        rec.put("k", kind);
        rec.put("v", value);
        String line = JsonText.serialize(rec);
        synchronized (lock) {
            if (closed) {
                throw new InvalidStateException("DatasetWriter is closed");
            }
            try {
                writer.write(line);
                writer.write('\n');
            } catch (IOException e) {
                throw new UncheckedIOException(e);
            }
            recordsWritten++;
        }
    }

    @Override
    public void close() {
        synchronized (lock) {
            if (closed) {
                return;
            }
            closed = true;
            try {
                writer.close();
            } catch (IOException e) {
                throw new UncheckedIOException(e);
            }
        }
    }
}
