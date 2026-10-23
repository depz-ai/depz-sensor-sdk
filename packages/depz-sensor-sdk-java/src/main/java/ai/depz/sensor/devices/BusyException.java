package ai.depz.sensor.devices;

import ai.depz.sensor.protocol.Common;
import java.util.Locale;

/** ERR_BUSY from the device, or a request whose opcode is already in flight. */
public final class BusyException extends StatusException {
    public BusyException(int cmd) {
        super(cmd, Common.Status.ERR_BUSY.value,
                String.format(Locale.ROOT, "cmd 0x%02X: busy (ERR_BUSY or already in flight)", cmd));
    }
}
