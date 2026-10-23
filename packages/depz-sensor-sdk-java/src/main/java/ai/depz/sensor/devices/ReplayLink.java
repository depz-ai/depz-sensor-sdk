package ai.depz.sensor.devices;

import ai.depz.sensor.json.JsonText;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.UncheckedIOException;
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
    @SuppressWarnings("unchecked")
    public ReplayLink(Path path, boolean strictTx, boolean realtime) {
        this.name = "replay:" + path.getFileName();
        this.strictTx = strictTx;
        this.realtime = realtime;
        List<String> lines;
        try {
            lines = Files.readAllLines(path, StandardCharsets.UTF_8);
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
        header = !lines.isEmpty() && !lines.get(0).isBlank()
                ? (Map<String, Object>) JsonText.parse(lines.get(0)) : Map.of();
        ByteArrayOutputStream txs = new ByteArrayOutputStream();
        for (int i = 1; i < lines.size(); i++) {
            if (lines.get(i).isBlank()) {
                continue;
            }
            Map<String, Object> ev = (Map<String, Object>) JsonText.parse(lines.get(i));
            byte[] data = JsonText.unhex((String) ev.get("data"));
            if ("rx".equals(ev.get("dir"))) {
                rx.add(new Rx(((Number) ev.get("t")).longValue(), data, txs.size()));
            } else {
                txs.writeBytes(data);
            }
        }
        tx = txs.toByteArray();
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
