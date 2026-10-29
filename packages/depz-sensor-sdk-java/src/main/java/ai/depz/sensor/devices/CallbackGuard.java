package ai.depz.sensor.devices;

import java.util.function.Consumer;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Runs a user callback where its exception must not stop the caller — a device's reader thread, a dataset
 * player (contract 07 §3, 09 §Player). Used by the SDK itself.
 */
public final class CallbackGuard {
    private static final Logger LOG = Logger.getLogger("ai.depz.sensor");

    private CallbackGuard() {}

    /**
     * Call {@code cb} with {@code item}: an exception it throws is logged ({@code java.util.logging} logger
     * {@code ai.depz.sensor}) and dropped; a {@link VirtualMachineError} still propagates.
     *
     * @return false if the callback threw, so the caller counts it
     */
    public static <T> boolean call(Consumer<T> cb, T item) {
        try {
            cb.accept(item);
            return true;
        } catch (RuntimeException | Error e) {
            if (e instanceof VirtualMachineError vme) {
                throw vme;
            }
            LOG.log(Level.SEVERE, "a callback threw; dropped", e);
            return false;
        }
    }
}
