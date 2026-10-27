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
    public DatasetWriter(Path path, Map<String, Map<String, Object>> devices, String note) {
        if (path == null || devices == null) {
            throw new IllegalArgumentException("path and devices must not be null");
        }
        try {
            OutputStream out = Files.newOutputStream(path);
            if (path.getFileName().toString().toLowerCase(java.util.Locale.ROOT).endsWith(".gz")) {
                out = new GZIPOutputStream(out);
            }
            writer = new BufferedWriter(new OutputStreamWriter(out, StandardCharsets.UTF_8));
            Map<String, Object> header = new LinkedHashMap<>();
            header.put("schema", SCHEMA);
            header.put("created_utc", UTC.format(Instant.now()));
            header.put("devices", new LinkedHashMap<>(devices));
            if (note != null && !note.isEmpty()) {
                header.put("note", note);
            }
            writer.write(JsonText.serialize(header));
            writer.write('\n');
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    public DatasetWriter(Path path, Map<String, Map<String, Object>> devices) {
        this(path, devices, "");
    }

    public long recordsWritten() {
        synchronized (lock) {
            return recordsWritten;
        }
    }

    /** One record: device id, host µs, kind, value. */
    public void write(String deviceId, long tHostUs, String kind, Map<String, Object> value) {
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
