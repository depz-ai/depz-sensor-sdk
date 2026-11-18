package ai.depz.sensor.devices;

import ai.depz.sensor.protocol.Common;
import java.util.Locale;

/** The device answered a request with a non-OK RPT_STATUS (contract 02 §3). */
public class StatusException extends DepzException {
    private final int cmd;
    private final int status;

    public StatusException(int cmd, int status) {
        this(cmd, status, String.format(Locale.ROOT, "cmd 0x%02X failed: %s (0x%02X)", cmd, nameOf(status), status));
    }

    protected StatusException(int cmd, int status, String message) {
        super(message);
        this.cmd = cmd;
        this.status = status;
    }

    public int cmd() {
        return cmd;
    }

    public int status() {
        return status;
    }

    /** The contract 02 §3 name of {@link #status()}, e.g. {@code ERR_INVALID_PARAM}. */
    public String statusName() {
        return nameOf(status);
    }

    static String nameOf(int status) {
        for (Common.Status s : Common.Status.values()) {
            if (s.value == status) {
                return s.name();
            }
        }
        return String.format(Locale.ROOT, "STATUS_0x%02X", status);
    }
}
