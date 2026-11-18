package ai.depz.sensor.devices;

/** The device is in bootloader mode: use the contract 06 client, not a sensor class. */
public final class BootloaderModeException extends DepzException {
    public BootloaderModeException() {
        super("device is in bootloader mode; use the bootloader client (contract 06)");
    }
}
