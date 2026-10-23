package ai.depz.sensor.test.devices;

import ai.depz.sensor.devices.DeviceLostException;
import ai.depz.sensor.devices.Link;
import ai.depz.sensor.devices.LinkClosedException;
import ai.depz.sensor.protocol.Common;
import ai.depz.sensor.transport.CrcType;
import ai.depz.sensor.transport.Event;
import ai.depz.sensor.transport.Framing;
import ai.depz.sensor.transport.Packet;
import ai.depz.sensor.transport.PacketParser;
import java.time.Duration;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.List;
import java.util.function.Function;

/** A board in-process: answers what the host writes with {@code onPacket}, and takes injected reports. */
public final class FakeBoardLink implements Link {
    private final ArrayDeque<byte[]> rx = new ArrayDeque<>();
    private final PacketParser parser = new PacketParser();
    private int seq;
    private boolean closed;
    private boolean unplugged;

    public volatile Function<Packet, List<byte[]>> onPacket;
    public final List<Packet> sent = new ArrayList<>();

    public static byte[] status(int cmd, int status) {
        return Framing.buildPacket(Common.Rpt.STATUS.value, new byte[] {(byte) cmd, (byte) status}, 0, CrcType.NONE);
    }

    public synchronized byte[] reply(int cmd, byte[] payload) {
        return Framing.buildPacket(cmd, payload, seq++ & 0xFF, CrcType.NONE);
    }

    public synchronized void inject(byte[] bytes) {
        rx.addLast(bytes);
        notifyAll();
    }

    public synchronized void unplug() {
        unplugged = true;
        notifyAll();
    }

    public int sentCount() {
        synchronized (sent) {
            return sent.size();
        }
    }

    public List<Packet> sentSnapshot() {
        synchronized (sent) {
            return new ArrayList<>(sent);
        }
    }

    @Override
    public String name() {
        return "fake";
    }

    @Override
    public synchronized boolean isClosed() {
        return closed;
    }

    @Override
    public synchronized byte[] read(Duration timeout) {
        if (rx.isEmpty() && !closed && !unplugged) {
            try {
                wait(Math.max(1, timeout.toMillis()));
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
        if (closed) {
            throw new LinkClosedException("fake closed");
        }
        if (unplugged) {
            throw new DeviceLostException("fake unplugged");
        }
        return rx.isEmpty() ? new byte[0] : rx.pollFirst();
    }

    @Override
    public void write(byte[] data) {
        List<Event> evs;
        synchronized (parser) {
            evs = parser.feed(data);
        }
        for (Event ev : evs) {
            if (!(ev instanceof Packet p)) {
                continue;
            }
            synchronized (sent) {
                sent.add(p);
            }
            Function<Packet, List<byte[]>> h = onPacket;
            if (h != null) {
                for (byte[] r : h.apply(p)) {
                    inject(r);
                }
            }
        }
    }

    @Override
    public synchronized void close() {
        closed = true;
        notifyAll();
    }
}
