package ai.depz.sensor.devices;

import ai.depz.sensor.json.JsonText;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

/**
 * Replays the rx side of a {@code .depzrec} capture causally (contract 08): an rx chunk is
 * served only after the host has written as many tx bytes as preceded it in the capture. With
 * {@code strictTx} every written byte must also equal the recorded tx stream
 * ({@link ReplayMismatchException}).
 */
public final class ReplayLink implements Link {
    private record Rx(long tUs, byte[] data, long txPrefix) {}

    private final List<Rx> rx = new ArrayList<>();
    private final byte[] tx;
    private final boolean strictTx;
    private final boolean realtime;
    private final long t0 = System.nanoTime();
    private final String name;
    private final Map<String, Object> header;
    private int rxIdx;
    private long txWritten;
    private boolean closed;

    /**
     * @param path     a {@code .depzrec} file
     * @param strictTx fail a write that differs from the recorded tx stream
     * @param realtime pace rx chunks by their recorded timestamps
     */
    public ReplayLink(Path path, boolean strictTx, boolean realtime) {
        this.name = "replay:" + path.getFileName();
        this.strictTx = strictTx;
        this.realtime = realtime;
        String content;
        try {
            content = Files.readString(path, StandardCharsets.UTF_8);
        } catch (CharacterCodingException e) {
            // not UTF-8 (a gzip container, ERRATA A6) is a damaged capture, not an I/O failure
            throw new IllegalArgumentException(path + ": not UTF-8 text: " + e, e);
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
        if (content.isEmpty()) {
            throw new IllegalArgumentException(path + ": empty, not a .depzrec capture (no header line)");
        }
        // contract 08 «Line rules»: LF ends a line (readAllLines would also cut at a lone CR)
        List<String> lines = List.of(content.split("\n", -1));
        header = jsonObject(path, 1, lines.get(0));
        ByteArrayOutputStream txs = new ByteArrayOutputStream();
        for (int i = 1; i < lines.size(); i++) {
            if (JsonText.isBlankLine(lines.get(i))) {
                continue;
            }
            int lineno = i + 1;
            Map<String, Object> ev = jsonObject(path, lineno, lines.get(i));
            Object t = ev.get("t");
            if (!(t instanceof Long tUs) || tUs < 0) {
                throw new IllegalArgumentException(path + ":" + lineno + ": 't' must be a non-negative integer, got " + t);
            }
            Object dir = ev.get("dir");
            if (!(dir instanceof String)) {
                throw new IllegalArgumentException(path + ":" + lineno + ": 'dir' must be a string, got " + dir);
            }
            Object hex = ev.get("data");
            if (!(hex instanceof String h)) {
                throw new IllegalArgumentException(path + ":" + lineno + ": 'data' must be a hex string, got " + hex);
            }
            byte[] data;
            try {
                data = JsonText.unhex(h);
            } catch (IllegalArgumentException e) {
                throw new IllegalArgumentException(path + ":" + lineno + ": 'data' is not hex: " + h);
            }
            if ("rx".equals(dir)) {
                rx.add(new Rx(tUs, data, txs.size()));
            } else {
                txs.writeBytes(data);
            }
        }
        tx = txs.toByteArray();
    }

    /**
     * {@code line} as the JSON object a capture line must be (contract 08), else
     * {@link IllegalArgumentException} naming the file and line — a damaged capture is the
     * caller's bad argument, never a crash.
     */
    @SuppressWarnings("unchecked")
    private static Map<String, Object> jsonObject(Path path, int lineno, String line) {
        Object obj;
        try {
            obj = JsonText.parse(line);
        } catch (IllegalArgumentException e) {
            throw new IllegalArgumentException(path + ":" + lineno + ": not JSON: " + e.getMessage());
        }
        if (!(obj instanceof Map)) {
            throw new IllegalArgumentException(path + ":" + lineno + ": not a JSON object");
        }
        return (Map<String, Object>) obj;
    }

    public ReplayLink(Path path) {
        this(path, false, false);
    }

    /** The capture's header object (contract 08). */
    public Map<String, Object> header() {
        return Collections.unmodifiableMap(header);
    }

    @Override
    public String name() {
        return name;
    }

    @Override
    public synchronized boolean isClosed() {
        return closed;
    }

    /** Every rx chunk was delivered. */
    public synchronized boolean rxExhausted() {
        return rxIdx >= rx.size();
    }

    /** Recorded rx chunks not delivered yet. */
    public synchronized int rxRemaining() {
        return rx.size() - rxIdx;
    }

    /** Recorded tx bytes the host has not written yet. */
    public synchronized long txRemaining() {
        return tx.length - txWritten;
    }

    @Override
    public byte[] read(Duration timeout) {
        long deadline = System.nanoTime() + timeout.toNanos();
        Rx ev;
        synchronized (this) {
            while (true) {
                if (closed) {
                    throw new LinkClosedException("replay link closed");
                }
                if (rxIdx >= rx.size()) {
                    return new byte[0];
                }
                if (txWritten >= rx.get(rxIdx).txPrefix()) {
                    break;
                }
                long left = deadline - System.nanoTime();
                if (left <= 0) {
                    return new byte[0];
                }
                try {
                    wait(Math.max(1, left / 1_000_000));
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    return new byte[0];
                }
            }
            ev = rx.get(rxIdx);
        }
        if (realtime) {
            long now = System.nanoTime();
            long due = t0 + ev.tUs() * 1000;
            if (due > now) {
                long budget = deadline - now;
                if (due - now > budget) {
                    sleepNanos(Math.max(0, budget));
                    return new byte[0];
                }
                sleepNanos(due - now);
            }
        }
        synchronized (this) {
            rxIdx++;
        }
        return ev.data();
    }

    private static void sleepNanos(long nanos) {
        try {
            Thread.sleep(nanos / 1_000_000, (int) (nanos % 1_000_000));
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    @Override
    public synchronized void write(byte[] data) {
        if (closed) {
            throw new LinkClosedException("replay link closed");
        }
        if (strictTx) {
            for (int i = 0; i < data.length; i++) {
                long at = txWritten + i;
                if (at >= tx.length || tx[(int) at] != data[i]) {
                    int from = (int) Math.min(txWritten, tx.length);
                    int n = (int) Math.max(0, Math.min(data.length, tx.length - txWritten));
                    byte[] expected = java.util.Arrays.copyOfRange(tx, from, from + n);
                    throw new ReplayMismatchException("replay strict_tx mismatch at offset " + txWritten
                            + ": expected " + JsonText.hex(expected) + ", got " + JsonText.hex(data));
                }
            }
        }
        txWritten += data.length;
        notifyAll();
    }

    @Override
    public synchronized void close() {
        closed = true;
        notifyAll();
    }
}
