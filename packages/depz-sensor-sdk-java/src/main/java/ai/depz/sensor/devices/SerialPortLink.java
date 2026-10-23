package ai.depz.sensor.devices;

import com.fazecast.jSerialComm.SerialPort;
import java.time.Duration;

/**
 * A serial port as a {@link Link} (jSerialComm: Windows, macOS, Linux). 115200 8N1 with DTR and
 * RTS raised; the USB CDC boards ignore the baud rate.
 */
public final class SerialPortLink implements Link {
    private final SerialPort port;
    private final String name;
    private final byte[] buf = new byte[16384];
    private volatile boolean closed;
    private int readTimeoutMs = -1;

    /** @throws DeviceLostException the port does not open */
    public SerialPortLink(String portPath, int baudRate) {
        this.name = portPath;
        port = SerialPort.getCommPort(portPath);
        port.setComPortParameters(baudRate, 8, SerialPort.ONE_STOP_BIT, SerialPort.NO_PARITY);
        port.setFlowControl(SerialPort.FLOW_CONTROL_DISABLED);
        if (!port.openPort()) {
            throw new DeviceLostException("cannot open " + portPath + " (error " + port.getLastErrorCode() + ")");
        }
        port.setDTR();
        port.setRTS();
    }

    public SerialPortLink(String portPath) {
        this(portPath, 115200);
    }

    @Override
    public String name() {
        return name;
    }

    @Override
    public boolean isClosed() {
        return closed || !port.isOpen();
    }

    @Override
    public byte[] read(Duration timeout) {
        if (closed) {
            throw new LinkClosedException(name + " closed");
        }
        int ms = (int) Math.max(1, Math.min(Integer.MAX_VALUE, timeout.toMillis()));
        if (ms != readTimeoutMs) {
            port.setComPortTimeouts(SerialPort.TIMEOUT_READ_SEMI_BLOCKING | SerialPort.TIMEOUT_WRITE_BLOCKING, ms, 1000);
            readTimeoutMs = ms;
        }
        int n = port.readBytes(buf, buf.length);
        if (n < 0) {
            if (closed) {
                throw new LinkClosedException(name + " closed");
            }
            throw new DeviceLostException(name + ": read failed (error " + port.getLastErrorCode() + ")");
        }
        byte[] data = new byte[n];
        System.arraycopy(buf, 0, data, 0, n);
        return data;
    }

    @Override
    public void write(byte[] data) {
        if (closed) {
            throw new LinkClosedException(name + " closed");
        }
        int done = 0;
        while (done < data.length) {
            byte[] rest = done == 0 ? data : java.util.Arrays.copyOfRange(data, done, data.length);
            int n = port.writeBytes(rest, rest.length);
            if (n <= 0) {
                throw new DeviceLostException(name + ": write failed (error " + port.getLastErrorCode() + ")");
            }
            done += n;
        }
    }

    @Override
    public void close() {
        closed = true;
        port.closePort();
    }
}
