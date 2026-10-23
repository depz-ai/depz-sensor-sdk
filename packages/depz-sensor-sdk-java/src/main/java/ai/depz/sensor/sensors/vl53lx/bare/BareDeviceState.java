package ai.depz.sensor.sensors.vl53lx.bare;

/** The ll-driver device states, single-zone case. */
public enum BareDeviceState {
    SW_STANDBY,
    RANGING_WAIT_GPH_SYNC,
    RANGING_OUTPUT_DATA,
    RANGING_DSS_AUTO,
}
