package ai.depz.sensor.devices;

import java.util.Locale;

/**
 * The reply to a request does not decode (contract 07 §3): shorter than the fields its contract
 * defines, or a value the decoder refuses. Only that request fails; the reader goes on and the
 * device stays usable.
 */
public class InvalidReplyException extends DepzException {
    private final int cmd;

    public InvalidReplyException(int cmd, Throwable cause) {
        super(String.format(Locale.ROOT, "reply to cmd 0x%02X does not decode: %s", cmd, cause.getMessage()), cause);
        this.cmd = cmd;
    }

    /** The request opcode whose reply was refused. */
    public int cmd() {
        return cmd;
    }
}
