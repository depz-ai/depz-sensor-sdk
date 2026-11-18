package ai.depz.sensor.devices;

/** The TMF8829 refused or did not answer (bootloader / application status). */
public final class Tmf8829Exception extends DepzException {
    public Tmf8829Exception(String message) {
        super(message);
    }
}
