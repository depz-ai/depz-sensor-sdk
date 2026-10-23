package ai.depz.sensor.sensors.vl53lx.bare;

/**
 * {@code VL53LX_update_ll_driver_{rd,cfg}_state()}, reduced to one zone: the GPH handshake (the id
 * stamped into the dynamic config tells the device a configuration is new) and the timing-status
 * flip (whether a frame ran on VCSEL period A or B, which the post-processing has to know).
 */
public final class LlState {
    public BareDeviceState cfgDeviceState;
    public long cfgStreamCount;
    public long cfgGphId;
    public long cfgTimingStatus;
    public BareDeviceState rdDeviceState;
    public long rdStreamCount;
    public long rdGphId;
    public long rdTimingStatus;

    public LlState() {
        reset();
    }

    public void reset() {
        cfgDeviceState = BareDeviceState.SW_STANDBY;
        cfgStreamCount = 0;
        cfgGphId = BareRegs.GROUPED_PARAMETER_HOLD_ID_MASK;
        cfgTimingStatus = 0;
        rdDeviceState = BareDeviceState.SW_STANDBY;
        rdStreamCount = 0;
        rdGphId = BareRegs.GROUPED_PARAMETER_HOLD_ID_MASK;
        rdTimingStatus = 0;
    }

    public void updateRd(long modeStart, long groupedParameterHold) {
        if ((modeStart & BareRegs.DEVICE_MEASUREMENT_MODE_MODE_MASK) == 0) {
            rdDeviceState = BareDeviceState.SW_STANDBY;
            rdStreamCount = 0;
            rdGphId = BareRegs.GROUPED_PARAMETER_HOLD_ID_MASK;
            rdTimingStatus = 0;
            return;
        }
        rdStreamCount = rdStreamCount == 0xFF ? 0x80 : rdStreamCount + 1;
        rdGphId ^= BareRegs.GROUPED_PARAMETER_HOLD_ID_MASK;
        switch (rdDeviceState) {
            case SW_STANDBY -> {
                rdDeviceState = (groupedParameterHold & BareRegs.GROUPED_PARAMETER_HOLD_ID_MASK) != 0
                        ? BareDeviceState.RANGING_WAIT_GPH_SYNC
                        : BareDeviceState.RANGING_OUTPUT_DATA;
                rdStreamCount = 0;
                rdTimingStatus = 0;
            }
            case RANGING_WAIT_GPH_SYNC -> {
                rdStreamCount = 0;
                rdDeviceState = BareDeviceState.RANGING_OUTPUT_DATA;
            }
            case RANGING_OUTPUT_DATA -> rdTimingStatus ^= 0x01;
            default -> reset();
        }
    }

    public void updateCfg(long modeStart) {
        if ((modeStart & BareRegs.DEVICE_MEASUREMENT_MODE_MODE_MASK) == 0) {
            cfgDeviceState = BareDeviceState.SW_STANDBY;
            cfgStreamCount = 0;
            cfgGphId = BareRegs.GROUPED_PARAMETER_HOLD_ID_MASK;
            cfgTimingStatus = 0;
            return;
        }
        cfgStreamCount = cfgStreamCount == 0xFF ? 0x80 : cfgStreamCount + 1;
        cfgGphId ^= BareRegs.GROUPED_PARAMETER_HOLD_ID_MASK;
        switch (cfgDeviceState) {
            case SW_STANDBY -> {
                cfgTimingStatus ^= 0x01;
                cfgStreamCount = 1;
                cfgDeviceState = BareDeviceState.RANGING_DSS_AUTO;
            }
            case RANGING_DSS_AUTO -> cfgTimingStatus ^= 0x01;
            default -> reset();
        }
    }
}
