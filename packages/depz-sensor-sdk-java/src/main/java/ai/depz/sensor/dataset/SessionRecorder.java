package ai.depz.sensor.dataset;

import ai.depz.sensor.devices.Bno055Device;
import ai.depz.sensor.devices.Bno086Device;
import ai.depz.sensor.devices.DepzDevice;
import ai.depz.sensor.devices.DepzException;
import ai.depz.sensor.devices.InvalidStateException;
import ai.depz.sensor.devices.Sr04Device;
import ai.depz.sensor.devices.TimeSyncResult;
import ai.depz.sensor.devices.Tmf8829Device;
import ai.depz.sensor.devices.Vl53l4cdDevice;
import ai.depz.sensor.devices.Vl53l7chDevice;
import ai.depz.sensor.devices.Vl53l8chDevice;
import ai.depz.sensor.devices.Vl53l8cxDevice;
import ai.depz.sensor.devices.Vl53lxDevice;
import ai.depz.sensor.protocol.Identity;
import ai.depz.sensor.sensors.vl53l8.Vl53l8CnhSetup;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Record decoded data from several devices onto one host timeline (contract 09). {@link #add} each open
 * device (it runs {@link DepzDevice#syncTime}), then {@link #start()}: the recorder hooks every device's
 * result stream and writes a record per result at device time − offset. It never reconfigures a device —
 * start and stop streaming yourself. {@link #stop()} (or closing) unhooks and closes the file.
 */
public final class SessionRecorder implements AutoCloseable {
    private final Path path;
    private final String note;
    private final boolean vl53l8Layers;
    private final Double temperatureHz;
    private final List<Entry> entries = new ArrayList<>();
    private final Map<String, Double> lastSilicon = new HashMap<>();
    private volatile DatasetWriter writer;
    private Thread tempThread;
    private CountDownLatch tempStop;

    private record Entry(String id, DepzDevice device, Map<String, Object> meta, long offsetUs, List<AutoCloseable> hooks) {}

    /**
     * @param path the {@code .depzdata} (or {@code .depzdata.gz}) to write
     * @param note header note ("" for none)
     * @param vl53l8Layers VL53L8 records also carry signal / ambient / sigma / reflectance
     * @param temperatureHz about this many temperature-kind records per device and second (the MCU
     *     thermometer, and the sensor die where frames carry one); null for none
     */
    public SessionRecorder(Path path, String note, boolean vl53l8Layers, Double temperatureHz) {
        if (path == null) {
            throw new IllegalArgumentException("path must not be null");
        }
        this.path = path;
        this.note = note == null ? "" : note;
        this.vl53l8Layers = vl53l8Layers;
        this.temperatureHz = temperatureHz;
    }

    /** No note, no VL53L8 layers, temperature at about 1 Hz. */
    public SessionRecorder(Path path) {
        this(path, "", false, 1.0);
    }

    /**
     * Register a device before recording starts; runs {@link DepzDevice#syncTime} so its timestamps land
     * on the shared host timeline. Returns its device id ({@code d0}, {@code d1}, … unless given).
     */
    public synchronized String add(DepzDevice device, String deviceId, int syncSamples) {
        if (device == null) {
            throw new IllegalArgumentException("device must not be null");
        }
        if (writer != null) {
            throw new InvalidStateException("add() every device before recording starts");
        }
        String id = deviceId != null ? deviceId : "d" + entries.size();
        TimeSyncResult sync = device.syncTime(syncSamples);
        Map<String, Object> meta = new LinkedHashMap<>();
        Map<String, Object> ts = new LinkedHashMap<>();
        ts.put("offset_us", sync.offsetUs());
        ts.put("rtt_us", sync.rttUs());
        meta.put("time_sync", ts);
        try {
            meta.put("serial", device.getSerialNumber());
            meta.put("software_name", device.getSoftwareName());
        } catch (DepzException e) {
            // a board that does not answer these is still recorded
        }
        Identity ident = Identity.parseSoftwareName(meta.get("software_name") instanceof String s ? s : "");
        meta.put("sensor_type", ident.sensorType() != null ? ident.sensorType().label : "unknown");
        entries.add(new Entry(id, device, meta, sync.offsetUs(), new ArrayList<>()));
        return id;
    }

    public String add(DepzDevice device) {
        return add(device, null, 5);
    }

    public long recordsWritten() {
        DatasetWriter w = writer;
        return w == null ? 0 : w.recordsWritten();
    }

    /** Open the file and hook every device's stream. */
    public synchronized void start() {
        if (writer != null) {
            return;
        }
        Map<String, Map<String, Object>> devices = new LinkedHashMap<>();
        for (Entry e : entries) {
            devices.put(e.id(), e.meta());
        }
        writer = new DatasetWriter(path, devices, note);
        for (Entry e : entries) {
            hook(e);
        }
        if (temperatureHz != null && temperatureHz > 0) {
            tempStop = new CountDownLatch(1);
            CountDownLatch stop = tempStop;
            long periodNs = (long) (1e9 / temperatureHz);
            tempThread = new Thread(() -> temperatureLoop(periodNs, stop), "depz-rec-temp");
            tempThread.setDaemon(true);
            tempThread.start();
        }
    }

    private void temperatureLoop(long periodNs, CountDownLatch stop) {
        while (true) {
            try {
                if (stop.await(periodNs, TimeUnit.NANOSECONDS)) {
                    return;
                }
            } catch (InterruptedException ie) {
                return;
            }
            DatasetWriter w = writer;
            if (w == null) {
                return;
            }
            for (Entry e : entries) {
                Double silicon;
                synchronized (lastSilicon) {
                    silicon = lastSilicon.get(e.id());
                }
                if (silicon != null) {
                    w.write(e.id(), DepzDevice.hostNowUs(), "temperature", temperature(silicon, "sensor"));
                }
                double celsius;
                try {
                    celsius = e.device().readMcuTemperature();
                } catch (DepzException | IllegalStateException ex) {
                    continue;
                }
                w.write(e.id(), DepzDevice.hostNowUs(), "temperature", temperature(celsius, "mcu"));
            }
        }
    }

    private static Map<String, Object> temperature(double celsius, String source) {
        Map<String, Object> v = new LinkedHashMap<>();
        v.put("celsius", celsius);
        v.put("source", source);
        return v;
    }

    private void hook(Entry e) {
        DatasetWriter w = writer;
        String id = e.id();
        long offset = e.offsetUs();
        List<AutoCloseable> hooks = e.hooks();
        DepzDevice dev = e.device();
        if (dev instanceof Sr04Device sr04) {
            hooks.add(sr04.onMeasurement(m -> w.write(id, m.timestampUs() - offset, "sr04", DatasetRecords.sr04(m))));
        } else if (dev instanceof Vl53l8cxDevice l8) {
            hooks.add(l8.onFrame(f -> {
                long t = f.timestampUs() - offset;
                synchronized (lastSilicon) {
                    lastSilicon.put(id, (double) f.siliconTempDegc());
                }
                w.write(id, t, "vl53l8", DatasetRecords.vl53l8(f, vl53l8Layers));
                // CH: the raw CNH block riding this frame, with the armed setup's decode parameters.
                Vl53l8CnhSetup setup = l8 instanceof Vl53l8chDevice ch ? ch.cnhSetup()
                        : l8 instanceof Vl53l7chDevice ch7 ? ch7.cnhSetup() : null;
                if (f.cnhRaw() != null && f.cnhRaw().length > 0 && setup != null) {
                    w.write(id, t, "vl53l8_cnh", DatasetRecords.vl53l8Cnh(f.cnhRaw(), setup));
                }
            }));
        } else if (dev instanceof Vl53l4cdDevice l4) {
            hooks.add(l4.onMeasurement(m -> w.write(id, m.timestampUs() - offset, "vl53l4", DatasetRecords.vl53l4(m))));
        } else if (dev instanceof Vl53lxDevice lx) {
            hooks.add(lx.onMeasurement(m -> w.write(id, m.timestampUs() - offset, "vl53lx",
                    DatasetRecords.vl53lx(m, lx.product(), lx.driverKind()))));
        } else if (dev instanceof Bno055Device b55) {
            hooks.add(b55.onSample(s -> w.write(id, s.timestampUs() - offset, "bno055", DatasetRecords.bno055(s))));
        } else if (dev instanceof Tmf8829Device tmf) {
            hooks.add(tmf.onMeasurement(m -> w.write(id, m.timestampUs() - offset, "tmf8829", DatasetRecords.tmf8829(m))));
        } else if (dev instanceof Bno086Device b86) {
            // Every enabled sensor's report at its measurement time; reports of one cargo carry out-of-order
            // device times, so t may step back within a device — readers sort (contract 09).
            hooks.add(b86.onReport(r -> w.write(id, ((Number) r.fields().get("timestamp_us")).longValue() - offset, "bno086",
                    DatasetRecords.bno086(r))));
        }
    }

    /** Unhook every device and close the file. */
    public synchronized void stop() {
        if (tempStop != null) {
            tempStop.countDown();
        }
        if (tempThread != null) {
            try {
                tempThread.join(3000);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
            tempThread = null;
        }
        for (Entry e : entries) {
            for (AutoCloseable h : e.hooks()) {
                try {
                    h.close();
                } catch (Exception ex) {
                    // unhooking cannot fail in a way worth reporting
                }
            }
            e.hooks().clear();
        }
        if (writer != null) {
            writer.close();
            writer = null;
        }
    }

    @Override
    public void close() {
        stop();
    }
}
