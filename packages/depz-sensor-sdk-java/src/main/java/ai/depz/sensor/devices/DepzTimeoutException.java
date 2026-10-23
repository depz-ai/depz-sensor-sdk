package ai.depz.sensor.devices;

import java.time.Duration;
import java.util.Locale;

/** No reply to a request within its timeout (default 200 ms). */
public class DepzTimeoutException extends DepzException {
    private final int cmd;

    public DepzTimeoutException(int cmd, Duration timeout) {
        super(String.format(Locale.ROOT, "no reply to cmd 0x%02X within %.3fs", cmd, timeout.toNanos() / 1e9));
        this.cmd = cmd;
    }

    public DepzTimeoutException(String message) {
        super(message);
        this.cmd = -1;
    }

    /** The request opcode that went unanswered (-1 when not a request). */
    public int cmd() {
        return cmd;
    }
}
