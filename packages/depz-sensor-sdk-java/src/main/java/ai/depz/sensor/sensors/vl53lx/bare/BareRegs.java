package ai.depz.sensor.sensors.vl53lx.bare;

import java.util.List;

/** Register addresses and settings of the BareDriver (vl53lx_register_map.h, vl53lx_ll_device.h). */
public final class BareRegs {
    private BareRegs() {}

    public static final int POWER_MANAGEMENT_GO1_POWER_FORCE = 0x0083;
    public static final int FIRMWARE_ENABLE = 0x0085;
    public static final int RESULT_OSC_CALIBRATE_VAL = 0x00DE;
    public static final int PATCH_CTRL = 0x0470;
    public static final int PATCH_JMP_ENABLES = 0x0472;
    public static final int PATCH_DATA_ENABLES = 0x0474;
    public static final int PATCH_OFFSET_0 = 0x0476;
    public static final int PATCH_ADDRESS_0 = 0x0496;

    public static final int HISTOGRAM_BIN_DATA_I2C_INDEX = 0x0088; // result__interrupt_status
    public static final int RESULT_HISTOGRAM_BIN_0_2 = 0x008E;
    public static final int RESULT_HISTOGRAM_BIN_23_0 = 0x00D5;
    public static final int PHASECAL_RESULT_REFERENCE_PHASE = 0x00D6;
    public static final int PHASECAL_RESULT_VCSEL_START = 0x00D8;
    public static final int RESULT_HISTOGRAM_BIN_23_0_MSB = 0x00D9;
    public static final int RESULT_HISTOGRAM_BIN_23_0_LSB = 0x00DA;
    public static final int HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES =
            RESULT_HISTOGRAM_BIN_23_0_LSB - HISTOGRAM_BIN_DATA_I2C_INDEX + 1;

    public static final int DEVICE_MEASUREMENT_MODE_BACK_TO_BACK = 0x20;
    public static final int DEVICE_MEASUREMENT_MODE_ABORT = 0x80;
    public static final int DEVICE_MEASUREMENT_MODE_STOP_MASK = 0x0F;
    public static final int DEVICE_MEASUREMENT_MODE_MODE_MASK = 0xF0;
    public static final int DEVICE_SCHEDULER_MODE_STREAMING = 0x01;
    public static final int DEVICE_SCHEDULER_MODE_HISTOGRAM = 0x02;
    public static final int DEVICE_READOUT_MODE_SINGLE_SD = 0x00 << 2;
    public static final int DEVICE_READOUT_MODE_DUAL_SD = 0x01 << 2;
    public static final int GROUPED_PARAMETER_HOLD_ID_MASK = 0x02;
    public static final int INTERRUPT_CONFIG_NEW_SAMPLE_READY = 0x20;
    public static final int CLEAR_RANGE_INT = 0x01;
    public static final int RANGE_STATUS_MASK = 0x1F;

    public static final int DEVICE_INTERRUPT_POLARITY_ACTIVE_LOW = 0x10;
    public static final int DEVICE_GPIO_MODE_OUTPUT_RANGE_AND_ERROR_INTERRUPTS = 0x01;
    public static final int DEVICE_DSS_MODE_TARGET_RATE = 1;

    public static final int SEQUENCE_VHV_EN = 0x01;
    public static final int SEQUENCE_PHASECAL_EN = 0x02;
    public static final int SEQUENCE_DSS1_EN = 0x08;
    public static final int SEQUENCE_DSS2_EN = 0x10;
    public static final int SEQUENCE_MM1_EN = 0x20;
    public static final int SEQUENCE_MM2_EN = 0x40;
    public static final int SEQUENCE_RANGE_EN = 0x80;

    public static final int SPAD_ARRAY_WIDTH = 16;
    public static final int SPAD_ARRAY_HEIGHT = 16;
    public static final int RTN_SPAD_APERTURE_TRANSMISSION = 0x0038;
    public static final int RTN_SPAD_UNITY_TRANSMISSION = 0x0100;

    public static final int AMBIENT_WINDOW_VCSEL_PERIODS = 256;
    public static final int RANGING_WINDOW_VCSEL_PERIODS = 2048;
    public static final int MACRO_PERIOD_VCSEL_PERIODS = AMBIENT_WINDOW_VCSEL_PERIODS + RANGING_WINDOW_VCSEL_PERIODS;
    public static final int HISTOGRAM_BUFFER_SIZE = 24;

    /** The three preset modes VL53L3CX/VL53L4CX expose, by the distance mode that picks them. */
    public static final List<String> DISTANCE_MODES = List.of("short", "medium", "long");

    /** The L4CX BareDriver (STSW-IMG029) has no short mode on an L4 die. */
    public static final List<String> L4_DISTANCE_MODES = List.of("medium", "long");

    // VL53LX_SetMeasurementTimingBudgetMicroSeconds: a fixed guard comes off the budget, the rest splits six ways.
    public static final int TIMING_GUARD_US = 1700;
    public static final int TIMING_DIVISOR = 6;
    public static final int FDA_MAX_TIMING_BUDGET_US = 550000;
    public static final int L4_FDA_MAX_TIMING_BUDGET_US = 200000;
}
