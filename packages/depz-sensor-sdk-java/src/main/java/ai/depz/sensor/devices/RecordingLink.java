package ai.depz.sensor.devices;

import ai.depz.sensor.json.JsonText;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Duration;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Wraps any link and tees both directions into a {@code .depzrec} file (contract 08). TX is
 * journaled before the physical write: a fast device reply could otherwise reach the reader and
 * be journaled ahead of its own request, which breaks the causal rx gating replay relies on.
 */
public final class RecordingLink implements Link {
    public static final String SCHEMA = "depz.rec/1";

    private final Link inner;
    private final BufferedWriter file;
    private final long t0 = System.nanoTime();
    private boolean fileClosed;

    /**
     * @param inner       the link to record; closing this one closes it
     * @param path        the {@code .depzrec} file to write (overwritten)
     * @param headerExtra more header fields (contract 08), e.g. {@code "device"}; may be null
     */
    public RecordingLink(Link inner, Path path, Map<String, Object> headerExtra) {
        if (inner == null) {
            throw new NullPointerException("inner");
        }
        this.inner = inner;
        try {
            file = Files.newBufferedWriter(path, StandardCharsets.UTF_8);
            Map<String, Object> header = new LinkedHashMap<>();
            header.put("schema", SCHEMA);
            header.put("created_utc", Instant.now().truncatedTo(ChronoUnit.SECONDS).toString());
            if (headerExtra != null) {
                header.putAll(headerExtra);
            }
            file.write(JsonText.serialize(header));
            file.write('\n');
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    public RecordingLink(Link inner, Path path) {
        this(inner, path, null);
    }

    @Override
    public String name() {
        return inner.name();
    }

    @Override
    public boolean isClosed() {
        return inner.isClosed();
    }

    private synchronized void emit(String dir, byte[] data) {
        if (data.length == 0 || fileClosed) {
            return;
        }
        Map<String, Object> ev = new LinkedHashMap<>();
        ev.put("t", (System.nanoTime() - t0) / 1000);
        ev.put("dir", dir);
        ev.put("data", data);
        try {
            file.write(JsonText.serialize(ev));
            file.write('\n');
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    @Override
    public byte[] read(Duration timeout) {
        byte[] data = inner.read(timeout);
        emit("rx", data);
        return data;
    }

    @Override
    public void write(byte[] data) {
        emit("tx", data); // journal first: see the class comment
        inner.write(data);
    }

    @Override
    public void close() {
        synchronized (this) {
            if (!fileClosed) {
                fileClosed = true;
                try {
                    file.close();
                } catch (IOException e) {
                    // the capture is as complete as it gets
                }
            }
        }
        inner.close();
    }
}
