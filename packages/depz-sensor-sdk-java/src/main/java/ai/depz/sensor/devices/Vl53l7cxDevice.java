package ai.depz.sensor.devices;

import ai.depz.sensor.protocol.Vl53l7;
import ai.depz.sensor.sensors.vl53l7.Vl53l7Uld;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Driver;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Firmware;
import java.time.Duration;
import java.util.function.BiConsumer;
import java.util.function.Consumer;
import java.util.logging.Logger;

/**
 * VL53L7CX multizone ToF (8×8 zones, 90° field of view) on the I2C bridge board (contract 11). The
 * board speaks the VL53L8 protocol plus three commands, and the sensor runs the same ST ULD family,
 * so this is a {@link Vl53l8cxDevice} with the L5/L7 firmware set (ULD 2.0.1), smaller register
 * reads, ranging from 1 Hz, and the board commands {@link #getBridgeInfo()},
 * {@link #setI2cSpeedKhz}, {@link #pinCtrl}. L5CX / L7CX have no deep sleep and no threshold
 * auto-stop.
 */
public class Vl53l7cxDevice extends Vl53l8cxDevice {
    private static final Logger LOG = Logger.getLogger(Vl53l7cxDevice.class.getName());

    public Vl53l7cxDevice(Link link, Duration timeout) {
        super(link, timeout);
    }

    public Vl53l7cxDevice(Link link) {
        super(link);
    }

    public Vl53l7cxDevice(String port, Duration timeout) {
        super(port, timeout);
    }

    Vl53l7cxDevice(DepzDevice from) {
        super(from);
    }

    @Override
    protected Vl53l8Firmware firmwareSet() {
        return Vl53l8Firmware.L7CX;
    }

    @Override
    protected int readChunk() {
        return Vl53l7.READ_MAX_LEN;
    }

    @Override
    protected int writeChunk() {
        return Vl53l7.WRITE_MAX_LEN;
    }

    @Override
    protected int streamChunk() {
        return Vl53l7.STREAM_CHUNK_MAX;
    }

    @Override
    protected int minRangingHz() {
        return Vl53l7Uld.MIN_RANGING_FREQUENCY_HZ;
    }

    /** The module type this class expects after {@link #init()} (L5 and L7 share a firmware). */
    protected int expectedModuleType() {
        return Vl53l8Driver.MODULE_TYPE_MZEVO;
    }

    /**
     * The sensor's module type read at {@link #init()}: {@link Vl53l8Driver#MODULE_TYPE_MZ} (VL53L5CX)
     * or {@link Vl53l8Driver#MODULE_TYPE_MZEVO} (VL53L7CX / CH); null before.
     */
    public Integer moduleType() {
        Vl53l8Driver d = uldOrNull();
        return d == null ? null : d.moduleType();
    }

    /** The last {@link #calibrateXtalk} found nothing to calibrate ("coverglass too good"). */
    public boolean xtalkCalibrationFailed() {
        Vl53l8Driver d = uldOrNull();
        return d != null && d.xtalkCalibrationFailed();
    }

    /**
     * Init, refusing non-L5/L7 silicon. A module type other than this class expects (a board labelled
     * L7 carrying an L5, or the reverse) is logged as a warning: ranging still works, the firmware is
     * shared.
     */
    @Override
    public void init(Consumer<String> progress, BiConsumer<Integer, Integer> writeProgress) {
        super.init(progress, writeProgress);
        Integer got = moduleType();
        if (got != null && got != expectedModuleType()) {
            LOG.warning(getClass().getSimpleName() + " expects module type " + moduleName(expectedModuleType())
                    + ", the sensor reports " + moduleName(got) + " — check the board's device name");
        }
    }

    private static String moduleName(int t) {
        return switch (t) {
            case Vl53l8Driver.MODULE_TYPE_MZ -> "MZ";
            case Vl53l8Driver.MODULE_TYPE_MZEVO -> "MZEVO";
            case Vl53l8Driver.MODULE_TYPE_MZPLUS -> "MZPlus";
            default -> String.valueOf(t);
        };
    }

    // ── board commands (contract 11 §2) ──────────────────────────────────────

    /**
     * Bridge counters and pin levels (the sensor is not touched). Read it before and after a run:
     * during one each call takes the bus from the stream and can cost a frame.
     */
    public Vl53l7.Vl53l7Info getBridgeInfo() {
        return request(Vl53l7.Vl53l7Cmd.GET_INFO.value, null, expectReport(Vl53l7.Vl53l7Rpt.VL53_INFO.value, Vl53l7.Vl53l7Info::unpack));
    }

    /**
     * Sensor bus SCL; the board takes the nearest of 100, 200, 400, 500 … 1000 kHz. Returns the value
     * in effect. {@link BusyException} mid-transfer: stop ranging first.
     */
    public int setI2cSpeedKhz(int khz) {
        if (khz < 1 || khz > 0xFFFF) {
            throw new IllegalArgumentException("khz must be 1..65535, got " + khz);
        }
        requestOk(Vl53l7.Vl53l7Cmd.SET_I2C_SPEED.value, Vl53l7.packSetI2cSpeed(khz));
        return getBridgeInfo().i2cKhz();
    }

    /** Drive the sensor's LPn / I2C_RST pins. LPN_OFF and SOFT_CYCLE drop the sensor's state: {@link #init()} again afterwards. */
    public void pinCtrl(Vl53l7.PinAction action) {
        requestOk(Vl53l7.Vl53l7Cmd.PIN_CTRL.value, Vl53l7.packPinCtrl(action));
        if (action == Vl53l7.PinAction.LPN_OFF || action == Vl53l7.PinAction.SOFT_CYCLE) {
            forgetSensor();
        }
    }
}
