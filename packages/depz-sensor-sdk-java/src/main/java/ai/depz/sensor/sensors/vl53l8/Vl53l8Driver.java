package ai.depz.sensor.sensors.vl53l8;

import ai.depz.sensor.devices.InvalidStateException;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Uld.Vl53l8Error;
import java.io.IOException;
import java.io.InputStream;
import java.io.UncheckedIOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.function.Consumer;

/**
 * The ST VL53L8CX ULD 2.1.0 / VL53LMZ ULD 2.0.16 / VL53L5CX-L7CX ULD 2.0.1 on the host, ported from
 * the reference Python driver: init (sensor firmware download), resolution, frequency, mode,
 * integration time, sharpener, target order, ranging, frame parsing, power modes, xtalk margin and
 * calibration, detection thresholds, motion indicator. Register sequences are a 1:1 port — every
 * request matches the Python one, so captures replay in both.
 */
public final class Vl53l8Driver {
    public static final int RESOLUTION_4X4 = 16;
    public static final int RESOLUTION_8X8 = 64;
    public static final int RANGING_MODE_CONTINUOUS = 1;
    public static final int RANGING_MODE_AUTONOMOUS = 3;
    public static final int TARGET_ORDER_CLOSEST = 1;
    public static final int TARGET_ORDER_STRONGEST = 2;
    public static final int POWER_MODE_SLEEP = 0;
    public static final int POWER_MODE_WAKEUP = 1;
    public static final int POWER_MODE_DEEP_SLEEP = 2;

    public static final int MODULE_TYPE_MZ = 0;      // VL53L5CX
    public static final int MODULE_TYPE_MZEVO = 1;   // VL53L7CX / VL53L7CH
    public static final int MODULE_TYPE_MZPLUS = 2;  // VL53L8

    public static final int NVM_DATA_SIZE = 492;
    public static final int OFFSET_BUFFER_SIZE = 488;
    public static final int XTALK_BUFFER_SIZE = 776;

    private static final int NB_TARGET_PER_ZONE = 1;
    private static final int UI_CMD_STATUS = 0x2C00;
    private static final int UI_CMD_START = 0x2C04;
    private static final int UI_CMD_END = 0x2FFF;

    private static final int DCI_ZONE_CONFIG = 0x5450;
    private static final int DCI_FREQ_HZ = 0x5458;
    private static final int DCI_INT_TIME = 0x545C;
    private static final int DCI_RANGING_MODE = 0xAD30;
    private static final int DCI_DSS_CONFIG = 0xAD38;
    private static final int DCI_TARGET_ORDER = 0xAE64;
    private static final int DCI_SHARPENER = 0xAED8;
    private static final int DCI_SINGLE_RANGE = 0xD964;
    private static final int DCI_OUTPUT_CONFIG = 0xD968;
    private static final int DCI_OUTPUT_ENABLES = 0xD970;
    private static final int DCI_OUTPUT_LIST = 0xD980;
    private static final int DCI_PIPE_CONTROL = 0xDB80;
    private static final int DCI_FW_FLAGS = 0xE0C4;
    private static final int DCI_CAL_CFG = 0x5470;
    private static final int DCI_XTALK_CFG = 0xAD94;
    private static final int DCI_MOTION_DETECTOR_CFG = 0xBFAC;
    private static final int DCI_DET_THRESH_CONFIG = 0x5488;
    private static final int DCI_DET_THRESH_GLOBAL_CONFIG = 0xB6E0;
    private static final int DCI_DET_THRESH_START = 0xB6E8;
    private static final int DCI_DET_THRESH_VALID_STATUS = 0xB9F0;

    private static final int START_BH = 0x0000000D;
    private static final int METADATA_BH = 0x54B400C0;
    private static final int COMMONDATA_BH = 0x54C00040;
    private static final int AMBIENT_RATE_BH = 0x54D00104;
    private static final int SPAD_COUNT_BH = 0x55D00404;
    private static final int NB_TARGET_DETECTED_BH = 0xDB840401;
    private static final int SIGNAL_RATE_BH = 0xDBC40404;
    private static final int RANGE_SIGMA_MM_BH = 0xDEC40402;
    private static final int DISTANCE_BH = 0xDF440402;
    private static final int REFLECTANCE_BH = 0xE0440401;
    private static final int TARGET_STATUS_BH = 0xE0840401;
    private static final int MOTION_DETECT_BH = 0xD85808C0;
    private static final int CNH_DATA_IDX = 0xC048;
    private static final int MOTION_DETEC_IDX = 0xD858;

    private final Vl53l8Platform p;
    private final Vl53l8Firmware firmware;
    private final boolean i2c;
    private final Long fwChecksum;
    private final int outputEnableW3;
    private final int frameTail;
    private final int footerIdOff;
    private final byte[] fwImage;
    private final byte[] defaultCfg;
    private final byte[] defaultXtalk;
    private final byte[] getNvmCmd;
    private byte[] offsetData = new byte[0];
    private byte[] xtalkData = new byte[0];
    private int streamCount = 255;
    private boolean motionPresent;
    private Integer moduleType;
    private int dataReadSize;
    private int[] frameSizeMismatch;
    private Integer resolution;
    private boolean xtalkCalibrationFailed;

    public Vl53l8Driver(Vl53l8Platform platform, Vl53l8Firmware firmware) {
        this.p = Objects.requireNonNull(platform, "platform");
        this.firmware = Objects.requireNonNull(firmware, "firmware");
        i2c = firmware == Vl53l8Firmware.L7CX || firmware == Vl53l8Firmware.L7CH;
        fwChecksum = switch (firmware) {
            case CX -> 0xCADF7CAFL;
            case L7CX -> null; // ULD 2.0.1 has no checksum step
            default -> 0x0C0B6C9EL;
        };
        // Output enables word 3: 0 on the LMZ firmware; the frame tail and footer-id offset
        // differ between ULD 2.1.0 (cx) and the 2.0.x drivers.
        outputEnableW3 = firmware == Vl53l8Firmware.CH || firmware == Vl53l8Firmware.L7CH ? 0 : 0xC0000000;
        frameTail = firmware == Vl53l8Firmware.CX ? 32 : 24;
        footerIdOff = firmware == Vl53l8Firmware.CX ? Vl53l8Uld.FOOTER_ID_OFF_CX : Vl53l8Uld.FOOTER_ID_OFF_CH;

        String set = firmware.setName;
        String fwSet = firmware == Vl53l8Firmware.L7CH ? "ch" : set; // l7ch runs the ch firmware
        fwImage = blob(fwSet, "firmware");
        defaultCfg = blob(set, "default_configuration");
        defaultXtalk = blob("cx", "default_xtalk"); // identical in every set
        getNvmCmd = blob("cx", "get_nvm_cmd");
        if (fwImage.length != 0x15000) {
            throw new InvalidStateException("firmware.bin (" + set + ") has the wrong size: " + fwImage.length);
        }
    }

    public Vl53l8Driver(Vl53l8Platform platform) {
        this(platform, Vl53l8Firmware.CX);
    }

    public Vl53l8Firmware firmware() {
        return firmware;
    }

    /** The L5/L7 boards: the sensor sits on I2C (other boot sequence, per-target blocks keep 64 entries in 4×4). */
    public boolean isI2c() {
        return i2c;
    }

    /** L5/L7: MZ (VL53L5CX) or MZEVO (VL53L7CX/CH), read during {@link #init}; else null. */
    public Integer moduleType() {
        return moduleType;
    }

    /** The frame size the firmware streams (bytes), set by {@link #startRanging}. */
    public int dataReadSize() {
        return dataReadSize;
    }

    /** {firmware, host} frame sizes when the firmware reported another size than the ULD formula; else null. */
    public int[] frameSizeMismatch() {
        return frameSizeMismatch == null ? null : frameSizeMismatch.clone();
    }

    /** Zones the last {@link #startRanging} used, or null. */
    public Integer resolution() {
        return resolution;
    }

    /** The last {@link #calibrateXtalk} ran into "coverglass too good": the default xtalk buffer is kept. */
    public boolean xtalkCalibrationFailed() {
        return xtalkCalibrationFailed;
    }

    /** The 776-byte xtalk buffer the driver uploads (default, calibrated or restored). */
    public byte[] xtalkData() {
        return xtalkData.clone();
    }

    private static byte[] blob(String set, String name) {
        String res = "data/" + set + "/" + name + ".bin";
        try (InputStream s = Vl53l8Driver.class.getResourceAsStream(res)) {
            if (s == null) {
                throw new InvalidStateException("classpath resource " + res + " is missing");
            }
            return s.readAllBytes();
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    /**
     * Whether the silicon ids are the sensor's: (0xF0, 0x0C) on L8 over SPI; revision 0x02, or 0x01
     * with device id 0xF0, on L5/L7 over I2C.
     */
    public static boolean siliconIdMatches(int deviceId, int revisionId, boolean i2c) {
        return i2c ? revisionId == 0x02 || (deviceId == 0xF0 && revisionId == 0x01)
                : deviceId == 0xF0 && revisionId == 0x0C;
    }

    // ── low level ────────────────────────────────────────────────────────────

    private int rd(int addr) {
        return p.readMulti(addr, 1)[0] & 0xFF;
    }

    private void wr(int addr, int value) {
        p.writeMulti(addr, new byte[] {(byte) value});
    }

    // _vl53l8cx_poll_for_answer: until buf[pos] & mask == expected; 10 ms period, 2 s timeout.
    private void pollForAnswer(int size, int pos, int address, int mask, int expected, String where) {
        int timeout = 0;
        while (true) {
            byte[] buf = p.readMulti(address, size);
            p.sleepMs(10);
            if (timeout >= 200) {
                throw new Vl53l8Error(Vl53l8Error.STATUS_TIMEOUT_ERROR, where);
            }
            if (size >= 4 && (buf[2] & 0xFF) >= 0x7F) {
                throw new Vl53l8Error(Vl53l8Error.MCU_ERROR, where);
            }
            timeout++;
            if ((buf[pos] & mask) == expected) {
                return;
            }
        }
    }

    private void pollForMcuBoot() {
        for (int timeout = 0; timeout < 500; timeout++) {
            int go2Status0 = rd(0x06);
            if ((go2Status0 & 0x80) != 0 && (rd(0x07) & 0x01) != 0) {
                return;
            }
            p.sleepMs(1);
            if ((go2Status0 & 0x01) != 0) {
                return;
            }
        }
        throw new Vl53l8Error(Vl53l8Error.STATUS_TIMEOUT_ERROR, "mcu boot");
    }

    // ── DCI access ───────────────────────────────────────────────────────────

    public byte[] dciReadData(int index, int dataSize) {
        byte[] cmd = new byte[12];
        cmd[0] = (byte) (index >> 8);
        cmd[1] = (byte) index;
        cmd[2] = (byte) ((dataSize & 0xFF0) >> 4);
        cmd[3] = (byte) ((dataSize & 0xF) << 4);
        cmd[7] = 0x0F;
        cmd[9] = 0x02;
        cmd[11] = 0x08;
        p.writeMulti(UI_CMD_END - 11, cmd);
        pollForAnswer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, String.format("dci read 0x%04X", index));
        byte[] buf = Vl53l8Uld.swapBuffer(p.readMulti(UI_CMD_START, dataSize + 12));
        return Arrays.copyOfRange(buf, 4, 4 + dataSize);
    }

    public void dciWriteData(int index, byte[] data) {
        int n = data.length;
        byte[] buf = new byte[n + 12];
        buf[0] = (byte) (index >> 8);
        buf[1] = (byte) index;
        buf[2] = (byte) ((n & 0xFF0) >> 4);
        buf[3] = (byte) ((n & 0xF) << 4);
        System.arraycopy(Vl53l8Uld.swapBuffer(data), 0, buf, 4, n);
        byte[] tail = {0x00, 0x00, 0x00, 0x0F, 0x05, 0x01, (byte) ((n + 8) >> 8), (byte) (n + 8)};
        System.arraycopy(tail, 0, buf, 4 + n, 8);
        p.writeMulti(UI_CMD_END - (n + 12) + 1, buf);
        pollForAnswer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, String.format("dci write 0x%04X", index));
    }

    public void dciReplaceData(int index, int dataSize, byte[] newData, int newDataPos) {
        byte[] data = dciReadData(index, dataSize);
        System.arraycopy(newData, 0, data, newDataPos, newData.length);
        dciWriteData(index, data);
    }

    // ── offset / xtalk upload ────────────────────────────────────────────────

    // 8x8 → 4x4: each 4x4 zone is the mean of its 2x2 block (C integer division).
    private static void average4x4(long[] grid) {
        for (int j = 0; j < 4; j++) {
            for (int i = 0; i < 4; i++) {
                grid[i + 4 * j] = Math.floorDiv(
                        grid[2 * i + 16 * j] + grid[2 * i + 16 * j + 1] + grid[2 * i + 16 * j + 8] + grid[2 * i + 16 * j + 9], 4L);
            }
        }
        for (int k = 16; k < 64; k++) {
            grid[k] = 0;
        }
    }

    private static long u32le(byte[] b, int off) {
        return (b[off] & 0xFFL) | (b[off + 1] & 0xFFL) << 8 | (b[off + 2] & 0xFFL) << 16 | (b[off + 3] & 0xFFL) << 24;
    }

    private static int i32le(byte[] b, int off) {
        return (int) u32le(b, off);
    }

    private static void putU32le(byte[] b, int off, long v) {
        b[off] = (byte) v;
        b[off + 1] = (byte) (v >> 8);
        b[off + 2] = (byte) (v >> 16);
        b[off + 3] = (byte) (v >> 24);
    }

    private void sendOffsetData(int res) {
        byte[] buf = new byte[OFFSET_BUFFER_SIZE];
        System.arraycopy(offsetData, 0, buf, 0, Math.min(OFFSET_BUFFER_SIZE, offsetData.length));
        if (res == RESOLUTION_4X4) {
            System.arraycopy(new byte[] {0x0F, 0x04, 0x04, 0x00, 0x08, 0x10, 0x10, 0x07}, 0, buf, 0x10, 8);
            buf = Vl53l8Uld.swapBuffer(buf);
            long[] signal = new long[64];
            long[] range = new long[64];
            for (int k = 0; k < 64; k++) {
                signal[k] = u32le(buf, 0x3C + 4 * k);
                range[k] = (short) ((buf[0x140 + 2 * k] & 0xFF) | (buf[0x141 + 2 * k] & 0xFF) << 8);
            }
            average4x4(signal);
            average4x4(range);
            for (int k = 0; k < 64; k++) {
                putU32le(buf, 0x3C + 4 * k, signal[k]);
                long r = Math.max(-32768, Math.min(32767, range[k]));
                buf[0x140 + 2 * k] = (byte) r;
                buf[0x141 + 2 * k] = (byte) (r >> 8);
            }
            buf = Vl53l8Uld.swapBuffer(buf);
        }
        // Drop the 8-byte NVM header and append the footer at 0x1E0.
        byte[] output = new byte[OFFSET_BUFFER_SIZE];
        System.arraycopy(buf, 8, output, 0, OFFSET_BUFFER_SIZE - 8);
        System.arraycopy(new byte[] {0x00, 0x00, 0x00, 0x0F, 0x03, 0x01, 0x01, (byte) 0xE4}, 0, output, OFFSET_BUFFER_SIZE - 8, 8);
        p.writeMulti(0x2E18, output);
        pollForAnswer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, "offset data");
    }

    private void sendXtalkData(int res) {
        byte[] buf = new byte[XTALK_BUFFER_SIZE];
        System.arraycopy(xtalkData, 0, buf, 0, Math.min(XTALK_BUFFER_SIZE, xtalkData.length));
        if (res == RESOLUTION_4X4) {
            System.arraycopy(new byte[] {0x0F, 0x04, 0x04, 0x17, 0x08, 0x10, 0x10, 0x07}, 0, buf, 0x08, 8);
            System.arraycopy(new byte[] {0x00, 0x78, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08}, 0, buf, 0x20, 8);
            buf = Vl53l8Uld.swapBuffer(buf);
            long[] signal = new long[64];
            for (int k = 0; k < 64; k++) {
                signal[k] = u32le(buf, 0x34 + 4 * k);
            }
            average4x4(signal);
            for (int k = 0; k < 64; k++) {
                putU32le(buf, 0x34 + 4 * k, signal[k]);
            }
            buf = Vl53l8Uld.swapBuffer(buf);
            System.arraycopy(new byte[] {(byte) 0xA0, (byte) 0xFC, 0x01, 0x00}, 0, buf, 0x134, 4);
            Arrays.fill(buf, 0x078, 0x078 + 4, (byte) 0);
        }
        p.writeMulti(0x2CF8, buf);
        pollForAnswer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, "xtalk data");
    }

    // ── identity and init ────────────────────────────────────────────────────

    /** {device id, revision id} of the silicon; {@link #siliconIdMatches} tells whether they are the sensor's. */
    public int[] isAlive() {
        wr(0x7FFF, 0x00);
        int deviceId = rd(0x00);
        int revisionId = rd(0x01);
        wr(0x7FFF, 0x02);
        return new int[] {deviceId, revisionId};
    }

    /**
     * vl53l8cx_init: boot the sensor MCU, download the 84 KB sensor firmware, upload the NVM
     * offsets, the default xtalk and the default configuration. {@code progress} (may be null)
     * gets phase texts.
     */
    public void init(Consumer<String> progress) {
        Consumer<String> note = progress == null ? s -> { } : progress;

        moduleType = null;
        if (i2c) {
            int[] ids = isAlive();
            if (!siliconIdMatches(ids[0], ids[1], true)) {
                throw new Vl53l8Error(Vl53l8Error.STATUS_ERROR,
                        String.format("not a VL53L5/L7 sensor: device_id=0x%02X revision_id=0x%02X", ids[0], ids[1]));
            }
        }

        note.accept("SW reboot...");
        wr(0x7FFF, 0x00);
        wr(0x0009, 0x04);
        wr(0x000F, 0x40);
        wr(0x000A, 0x03);
        rd(0x7FFF);
        wr(0x000C, 0x01);

        wr(0x0101, 0x00);
        wr(0x0102, 0x00);
        wr(0x010A, 0x01);
        wr(0x4002, 0x01);
        wr(0x4002, 0x00);
        wr(0x010A, 0x03);
        wr(0x0103, 0x01);
        wr(0x000C, 0x00);
        wr(0x000F, 0x43);
        p.sleepMs(1);

        wr(0x000F, 0x40);
        wr(0x000A, 0x01);
        p.sleepMs(100);

        note.accept("Waiting for sensor boot...");
        wr(0x7FFF, 0x00);
        pollForAnswer(1, 0, 0x06, 0xFF, 1, "sensor boot");

        wr(0x000E, 0x01);
        wr(0x7FFF, 0x02);

        // Enable FW access
        if (i2c) {
            wr(0x03, 0x0D);
            wr(0x7FFF, 0x01);
            pollForAnswer(1, 0, 0x21, 0x10, 0x10, "fw access");
        } else {
            wr(0x7FFF, 0x01);
            wr(0x06, 0x01);
            pollForAnswer(1, 0, 0x21, 0xFF, 0x04, "fw access");
        }

        wr(0x7FFF, 0x00);

        // Enable host access to GO1
        rd(0x7FFF);
        wr(0x0C, 0x01);

        // Power ON status
        wr(0x7FFF, 0x00);
        wr(0x101, 0x00);
        wr(0x102, 0x00);
        wr(0x010A, 0x01);
        wr(0x4002, 0x01);
        wr(0x4002, 0x00);
        wr(0x010A, 0x03);
        wr(0x103, 0x01);
        wr(0x400F, 0x00);
        wr(0x21A, 0x43);
        wr(0x21A, 0x03);
        wr(0x21A, 0x01);
        wr(0x21A, 0x00);
        wr(0x219, 0x00);
        wr(0x21B, 0x00);

        // Wake up MCU
        wr(0x7FFF, 0x00);
        rd(0x7FFF);
        if (i2c) {
            wr(0x0C, 0x00);
        }
        wr(0x7FFF, 0x01);
        if (i2c) {
            wr(0x20, 0x07);
            wr(0x20, 0x06);
        }

        // Download FW into the sensor
        note.accept("Downloading sensor FW (84 KB)... bank 1/3");
        wr(0x7FFF, 0x09);
        p.writeMulti(0, Arrays.copyOfRange(fwImage, 0, 0x8000));
        note.accept("Downloading sensor FW... bank 2/3");
        wr(0x7FFF, 0x0A);
        p.writeMulti(0, Arrays.copyOfRange(fwImage, 0x8000, 0x10000));
        note.accept("Downloading sensor FW... bank 3/3");
        wr(0x7FFF, 0x0B);
        p.writeMulti(0, Arrays.copyOfRange(fwImage, 0x10000, 0x15000));
        wr(0x7FFF, 0x01);

        // Check if FW correctly downloaded
        if (i2c) {
            wr(0x7FFF, 0x02);
            wr(0x03, 0x0D);
            wr(0x7FFF, 0x01);
            pollForAnswer(1, 0, 0x21, 0x10, 0x10, "fw downloaded");
        } else {
            wr(0x7FFF, 0x01);
            wr(0x06, 0x03);
            p.sleepMs(5);
        }

        wr(0x7FFF, 0x00);
        rd(0x7FFF);
        wr(0x0C, 0x01);

        // Reset MCU and wait boot
        note.accept("Booting sensor MCU...");
        wr(0x7FFF, 0x00);
        wr(0x114, 0x00);
        wr(0x115, 0x00);
        wr(0x116, 0x42);
        wr(0x117, 0x00);
        wr(0x0B, 0x00);
        rd(0x7FFF);
        wr(0x0C, 0x00);
        wr(0x0B, 0x01);

        pollForMcuBoot();

        wr(0x7FFF, 0x02);

        if (fwChecksum != null) {
            long crc = u32le(Vl53l8Uld.swapBuffer(p.readMulti(0x2FFC, 4)), 0);
            if (crc != fwChecksum) {
                throw new Vl53l8Error(Vl53l8Error.STATUS_FW_CHECKSUM_FAIL,
                        String.format("crc=0x%08X (expected 0x%08X for %s)", crc, fwChecksum, firmware.setName));
            }
            note.accept("Sensor FW checksum OK");
        }

        note.accept("Reading NVM offset data...");
        p.writeMulti(0x2FD8, getNvmCmd);
        pollForAnswer(4, 0, UI_CMD_STATUS, 0xFF, 2, "nvm read");
        offsetData = Arrays.copyOf(p.readMulti(UI_CMD_START, NVM_DATA_SIZE), OFFSET_BUFFER_SIZE);
        sendOffsetData(RESOLUTION_4X4);

        note.accept("Uploading default xtalk...");
        xtalkData = defaultXtalk;
        sendXtalkData(RESOLUTION_4X4);

        note.accept("Uploading default configuration...");
        p.writeMulti(0x2C34, defaultCfg);
        pollForAnswer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, "default config");

        dciWriteData(DCI_PIPE_CONTROL, new byte[] {NB_TARGET_PER_ZONE, 0x00, 0x01, 0x00});
        dciWriteData(DCI_SINGLE_RANGE, u32(0x01));
        if (i2c) {
            // Readable once the sensor FW runs; tells L5 (MZ) from L7 (MZEVO): one blob, one board.
            moduleType = dciReadData(DCI_FW_FLAGS, 8)[1] & 0xFF;
            String name = switch (moduleType) {
                case MODULE_TYPE_MZ -> "MZ";
                case MODULE_TYPE_MZEVO -> "MZEVO";
                case MODULE_TYPE_MZPLUS -> "MZPlus";
                default -> String.valueOf(moduleType);
            };
            note.accept("Sensor init done (module type " + name + ")");
        } else {
            note.accept("Sensor init done");
        }
    }

    public void init() {
        init(null);
    }

    private static byte[] u32(long v) {
        byte[] b = new byte[4];
        putU32le(b, 0, v);
        return b;
    }

    private static byte[] u32s(int[] words) {
        byte[] b = new byte[words.length * 4];
        for (int i = 0; i < words.length; i++) {
            putU32le(b, 4 * i, words[i] & 0xFFFFFFFFL);
        }
        return b;
    }

    // ── configuration ────────────────────────────────────────────────────────

    public int getResolution() {
        byte[] buf = dciReadData(DCI_ZONE_CONFIG, 8);
        return (buf[0] & 0xFF) * (buf[1] & 0xFF);
    }

    public void setResolution(int res) {
        if (res != RESOLUTION_4X4 && res != RESOLUTION_8X8) {
            throw new Vl53l8Error(Vl53l8Error.STATUS_INVALID_PARAM, "set_resolution");
        }
        boolean four = res == RESOLUTION_4X4;
        byte[] buf = dciReadData(DCI_DSS_CONFIG, 16);
        buf[0x04] = (byte) (four ? 64 : 16);
        buf[0x06] = (byte) (four ? 64 : 16);
        buf[0x09] = (byte) (four ? 4 : 1);
        dciWriteData(DCI_DSS_CONFIG, buf);
        buf = dciReadData(DCI_ZONE_CONFIG, 8);
        buf[0x00] = (byte) (four ? 4 : 8);
        buf[0x01] = (byte) (four ? 4 : 8);
        buf[0x04] = (byte) (four ? 8 : 4);
        buf[0x05] = (byte) (four ? 8 : 4);
        dciWriteData(DCI_ZONE_CONFIG, buf);
        sendOffsetData(res);
        sendXtalkData(res);
    }

    public int getRangingFrequencyHz() {
        return dciReadData(DCI_FREQ_HZ, 4)[1] & 0xFF;
    }

    public void setRangingFrequencyHz(int hz) {
        if (hz < 0 || hz > 255) {
            throw new IllegalArgumentException("ranging frequency is a u8 field (0..255 Hz), got " + hz);
        }
        dciReplaceData(DCI_FREQ_HZ, 4, new byte[] {(byte) hz}, 0x01);
    }

    public void setRangingMode(int mode) {
        byte[] buf = dciReadData(DCI_RANGING_MODE, 8);
        long singleRange;
        if (mode == RANGING_MODE_CONTINUOUS) {
            buf[0x01] = 0x1;
            buf[0x03] = 0x3;
            singleRange = 0x00;
        } else if (mode == RANGING_MODE_AUTONOMOUS) {
            buf[0x01] = 0x3;
            buf[0x03] = 0x2;
            singleRange = 0x01;
        } else {
            throw new Vl53l8Error(Vl53l8Error.STATUS_INVALID_PARAM, "set_ranging_mode");
        }
        dciWriteData(DCI_RANGING_MODE, buf);
        dciWriteData(DCI_SINGLE_RANGE, u32(singleRange));
    }

    public int getRangingMode() {
        return dciReadData(DCI_RANGING_MODE, 8)[0x01] == 0x1 ? RANGING_MODE_CONTINUOUS : RANGING_MODE_AUTONOMOUS;
    }

    public int getIntegrationTimeMs() {
        return (int) (u32le(dciReadData(DCI_INT_TIME, 20), 0) / 1000);
    }

    /** 2..1000 ms; no effect in continuous mode. */
    public void setIntegrationTimeMs(int timeMs) {
        if (timeMs < 2 || timeMs > 1000) {
            throw new Vl53l8Error(Vl53l8Error.STATUS_INVALID_PARAM, "set_integration_time_ms");
        }
        dciReplaceData(DCI_INT_TIME, 20, u32(timeMs * 1000L), 0x00);
    }

    /**
     * 0..99 %, rounded to nearest on the way back (ST truncates and loses a count for 95 of the 100
     * legal values; the stored byte is the same).
     */
    public int getSharpenerPercent() {
        return (int) Math.rint((dciReadData(DCI_SHARPENER, 16)[0xD] & 0xFF) * 100 / 255.0);
    }

    public void setSharpenerPercent(int pct) {
        if (pct >= 100) {
            throw new Vl53l8Error(Vl53l8Error.STATUS_INVALID_PARAM, "set_sharpener_percent");
        }
        if (pct < 0) {
            throw new IllegalArgumentException("sharpener is 0..99 %, got " + pct);
        }
        dciReplaceData(DCI_SHARPENER, 16, new byte[] {(byte) (pct * 255 / 100)}, 0xD);
    }

    public int getTargetOrder() {
        return dciReadData(DCI_TARGET_ORDER, 4)[0] & 0xFF;
    }

    public void setTargetOrder(int order) {
        if (order != TARGET_ORDER_CLOSEST && order != TARGET_ORDER_STRONGEST) {
            throw new Vl53l8Error(Vl53l8Error.STATUS_INVALID_PARAM, "set_target_order");
        }
        dciReplaceData(DCI_TARGET_ORDER, 4, new byte[] {(byte) order}, 0x0);
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    private static int bhSetSize(int bh, int size) {
        return (bh & ~0xFFF0) | ((size & 0xFFF) << 4);
    }

    /**
     * Program the output list and start. {@code cnhDataSize} (bytes, from
     * {@link Vl53l8CnhSetup#requiredMemory()}; null for none) appends the CNH block to every frame.
     */
    public void startRanging(Integer cnhDataSize) {
        int res = getResolution();
        // A zero / odd resolution sizes the output config to nothing and faults the sensor MCU.
        if (res != RESOLUTION_4X4 && res != RESOLUTION_8X8) {
            throw new Vl53l8Error(Vl53l8Error.STATUS_INVALID_PARAM, "bad resolution " + res + " — call set_resolution first");
        }
        dataReadSize = 0;
        streamCount = 255;

        int[] enable = {0x00000FFF, 0, 0, outputEnableW3};
        List<Integer> output = new ArrayList<>(List.of(
                START_BH, METADATA_BH, COMMONDATA_BH, AMBIENT_RATE_BH, SPAD_COUNT_BH, NB_TARGET_DETECTED_BH,
                SIGNAL_RATE_BH, RANGE_SIGMA_MM_BH, DISTANCE_BH, REFLECTANCE_BH, TARGET_STATUS_BH, MOTION_DETECT_BH));
        if (cnhDataSize != null) {
            // vl53lmz_add_output_block: CNH block, type 4, size in 32-bit words.
            output.add((CNH_DATA_IDX << 16) | (((cnhDataSize / 4) & 0xFFF) << 4) | 4);
            enable[0] |= 1 << (output.size() - 1);
        }

        int size = 0;
        for (int i = 0; i < output.size(); i++) {
            int bh = output.get(i);
            if (bh == 0 || (enable[i / 32] & (1 << (i % 32))) == 0) {
                continue;
            }
            int[] f = Vl53l8Uld.bhFields(bh & 0xFFFFFFFFL);
            int type = f[0];
            int bhSize = f[1];
            int idx = f[2];
            if (type >= 0x1 && type < 0x0D) {
                if (i2c) {
                    // vl53lmz_start_ranging (2.0.16), as on L5/L7 hardware: per-target blocks above
                    // 0x6C90 keep their 64-entry size even in 4x4 (the frame parser trims them).
                    if (idx >= 0x54D0 && idx < 0x5890) {
                        bhSize = res;
                    } else if (idx < 0x6C90) {
                        bhSize = res * NB_TARGET_PER_ZONE;
                    }
                } else if (idx >= 0x54D0 && idx < 0x54D0 + 960) {
                    bhSize = res;
                } else if (idx != CNH_DATA_IDX) { // the CNH block keeps its size
                    bhSize = res * NB_TARGET_PER_ZONE;
                }
                output.set(i, bhSetSize(bh, bhSize));
                size += type * bhSize;
            } else {
                size += bhSize;
            }
            size += 4;
        }
        dataReadSize = size + frameTail;
        resolution = res;

        int[] words = output.stream().mapToInt(Integer::intValue).toArray();
        dciWriteData(DCI_OUTPUT_LIST, u32s(words));
        dciWriteData(DCI_OUTPUT_CONFIG, u32s(new int[] {dataReadSize, words.length + 1}));
        dciWriteData(DCI_OUTPUT_ENABLES, u32s(enable));

        // Start xshut bypass (interrupt mode)
        wr(0x7FFF, 0x00);
        wr(0x09, 0x05);
        wr(0x7FFF, 0x02);

        // Start the ranging session
        p.writeMulti(UI_CMD_END - 3, new byte[] {0x00, 0x03, 0x00, 0x00});
        pollForAnswer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, "start ranging");

        // The firmware tells the frame size it streams; the released blobs report 4 bytes less
        // than the C formula, so trust the firmware value.
        byte[] cfg = dciReadData(0x5440, 12);
        int fwSize = (cfg[0x8] & 0xFF) | (cfg[0x9] & 0xFF) << 8;
        frameSizeMismatch = null;
        if (fwSize != dataReadSize) {
            frameSizeMismatch = new int[] {fwSize, dataReadSize};
            dataReadSize = fwSize;
        }

        if (dciReadData(DCI_FW_FLAGS, 8)[0x6] != 0) {
            throw new Vl53l8Error(Vl53l8Error.STATUS_LASER_SAFETY);
        }
    }

    public void startRanging() {
        startRanging(null);
    }

    public void stopRanging() {
        long autoStopFlag = u32le(p.readMulti(0x2FFC, 4), 0);
        if (autoStopFlag != 0x4FF) {
            wr(0x7FFF, 0x00);
            // Provoke MCU stop
            wr(0x15, 0x16);
            wr(0x14, 0x01);
            // Poll for G02 status 0 MCU stop
            int tmp = 0;
            int timeout = 0;
            while (((tmp & 0x80) >> 7) == 0) {
                tmp = rd(0x06);
                p.sleepMs(10);
                timeout++;
                if (timeout > 500) {
                    break;
                }
            }
        }
        // GO2 status 1: the C driver ORs a 0x84/0x85 mismatch into its status; non-fatal here.
        if ((rd(0x06) & 0x80) != 0) {
            rd(0x07);
        }
        // Undo MCU stop
        wr(0x7FFF, 0x00);
        wr(0x14, 0x00);
        wr(0x15, 0x00);
        // Stop xshut bypass
        wr(0x09, 0x04);
        wr(0x7FFF, 0x02);
    }

    /** Poll mode: a new frame is ready. */
    public boolean checkDataReady() {
        byte[] buf = p.readMulti(0x0, 4);
        int b0 = buf[0] & 0xFF;
        if (b0 != streamCount && b0 != 255 && (buf[1] & 0xFF) == 0x05 && (buf[2] & 0x05) == 0x05 && (buf[3] & 0x10) == 0x10) {
            streamCount = b0;
            return true;
        }
        if ((buf[3] & 0x80) != 0) {
            throw new Vl53l8Error(buf[2] & 0xFF, "GO2 error status");
        }
        return false;
    }

    /** Poll mode: read one frame from register 0x00 and parse it. */
    public Vl53l8Frame getRangingData(long timestampUs) {
        return parseFrame(timestampUs, p.readMulti(0x0, dataReadSize));
    }

    /**
     * Parse one raw frame ({@link #dataReadSize()} bytes) — polled, or pushed by the board's stream.
     * L5/L7 frames are trimmed to the zones in use.
     *
     * @throws Vl53l8Error STATUS_CORRUPTED_FRAME when the header / footer ids differ
     */
    public Vl53l8Frame parseFrame(long timestampUs, byte[] raw) {
        if (raw.length > 0) {
            streamCount = raw[0] & 0xFF;
        }
        Vl53l8Uld.Results r = i2c && resolution != null
                ? Vl53l8Uld.parseFrame(raw, raw.length, footerIdOff, resolution)
                : Vl53l8Uld.parseFrame(raw, raw.length, footerIdOff);
        return Vl53l8Frame.from(timestampUs, r, motionPresent ? parseMotion(Vl53l8Uld.swapBuffer(raw)) : null);
    }

    private static Vl53l8Motion parseMotion(byte[] buf) {
        int drs = buf.length;
        int i = 16;
        while (i + 4 <= drs) {
            int[] f = Vl53l8Uld.bhFields(u32le(buf, i));
            int t = f[0];
            int s = f[1];
            int idx = f[2];
            int msize = t > 0x1 && t < 0xD ? t * s : s;
            if (i + 4 + msize > drs) {
                break;
            }
            if (idx == MOTION_DETEC_IDX) {
                long[] motion = new long[32];
                for (int k = 0; k < 32; k++) {
                    motion[k] = u32le(buf, i + 16 + 4 * k);
                }
                return new Vl53l8Motion(u32le(buf, i + 4), u32le(buf, i + 8),
                        buf[i + 12] & 0xFF, buf[i + 13] & 0xFF, buf[i + 14] & 0xFF, motion);
            }
            i += msize + 4;
        }
        return null;
    }

    // ── power modes ──────────────────────────────────────────────────────────

    public int getPowerMode() {
        wr(0x7FFF, 0x00);
        int tmp = rd(0x09);
        int mode;
        if (tmp == 0x04) {
            mode = POWER_MODE_WAKEUP;
        } else if (tmp == 0x02) {
            // ULD 2.0.1 (l7cx) has no deep sleep and never reads 0x000F.
            mode = firmware == Vl53l8Firmware.L7CX ? POWER_MODE_SLEEP
                    : rd(0x000F) == 0x43 ? POWER_MODE_DEEP_SLEEP : POWER_MODE_SLEEP;
        } else {
            wr(0x7FFF, 0x02);
            throw new Vl53l8Error(Vl53l8Error.STATUS_ERROR, "get_power_mode");
        }
        wr(0x7FFF, 0x02);
        return mode;
    }

    /**
     * Wakeup, sleep or deep sleep (not on L5CX/L7CX). Waking from deep sleep re-runs {@link #init}:
     * the sensor firmware is lost.
     */
    public void setPowerMode(int powerMode) {
        if (firmware == Vl53l8Firmware.L7CX && powerMode != POWER_MODE_SLEEP && powerMode != POWER_MODE_WAKEUP) {
            throw new Vl53l8Error(Vl53l8Error.STATUS_INVALID_PARAM, "set_power_mode: VL53L5CX/L7CX have no deep sleep");
        }
        int current = getPowerMode();
        if (powerMode == current) {
            return;
        }
        if (powerMode == POWER_MODE_WAKEUP && firmware == Vl53l8Firmware.L7CX) {
            wr(0x7FFF, 0x00);
            wr(0x09, 0x04);
            pollForAnswer(1, 0, 0x06, 0x01, 1, "wakeup");
            wr(0x7FFF, 0x02);
        } else if (powerMode == POWER_MODE_WAKEUP) {
            wr(0x7FFF, 0x00);
            wr(0x09, 0x04);
            int stored = rd(0x000F);
            if (stored == 0x43) {
                wr(0x000F, 0x40);
            }
            pollForAnswer(1, 0, 0x06, 0x01, 1, "wakeup");
            wr(0x7FFF, 0x02);
            if (stored == 0x43) {
                init();
            }
        } else if (powerMode == POWER_MODE_SLEEP) {
            wr(0x7FFF, 0x00);
            wr(0x09, 0x02);
            pollForAnswer(1, 0, 0x06, 0x01, 0, "sleep");
            wr(0x7FFF, 0x02);
        } else if (powerMode == POWER_MODE_DEEP_SLEEP) {
            wr(0x7FFF, 0x00);
            wr(0x09, 0x02);
            pollForAnswer(1, 0, 0x06, 0x01, 0, "deep sleep");
            wr(0x000F, 0x43);
            wr(0x7FFF, 0x02);
        } else {
            throw new Vl53l8Error(Vl53l8Error.STATUS_INVALID_PARAM, "set_power_mode");
        }
    }

    // ── xtalk ────────────────────────────────────────────────────────────────

    /** Xtalk margin, whole kcps/SPAD (raw / 2048 rounded down, as ST). */
    public int getXtalkMargin() {
        return Vl53l8Uld.xtalkMarginFromRaw(u32le(dciReadData(DCI_XTALK_CFG, 16), 0));
    }

    /** Xtalk margin, a whole number of kcps/SPAD, 0..10000 (as ST); anything else is INVALID_PARAM, nothing written. */
    public void setXtalkMargin(int marginKcps) {
        dciReplaceData(DCI_XTALK_CFG, 16, u32(Vl53l8Uld.xtalkMarginToRaw(marginKcps)), 0x00);
    }

    private byte[] readXtalkBuffer() {
        p.writeMulti(0x2FB8, Vl53l8Tables.getXtalkCmd());
        pollForAnswer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, "get xtalk");
        byte[] buf = p.readMulti(UI_CMD_START, XTALK_BUFFER_SIZE + 4);
        byte[] output = new byte[XTALK_BUFFER_SIZE];
        System.arraycopy(buf, 8, output, 0, XTALK_BUFFER_SIZE - 8);
        System.arraycopy(new byte[] {0x00, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x03, 0x04}, 0, output, XTALK_BUFFER_SIZE - 8, 8);
        return output;
    }

    /** The live 776-byte xtalk calibration buffer (save / restore); the resolution is restored afterwards. */
    public byte[] getCaldataXtalk() {
        int res = getResolution();
        setResolution(RESOLUTION_8X8);
        xtalkData = readXtalkBuffer();
        setResolution(res);
        return xtalkData.clone();
    }

    /** Restore a saved 776-byte xtalk buffer; it is uploaded again by the next resolution change and at once here. */
    public void setCaldataXtalk(byte[] blob) {
        if (blob == null || blob.length != XTALK_BUFFER_SIZE) {
            throw new Vl53l8Error(Vl53l8Error.STATUS_INVALID_PARAM, "xtalk blob must be " + XTALK_BUFFER_SIZE + " bytes");
        }
        int res = getResolution();
        xtalkData = blob.clone();
        setResolution(res);
    }

    // ── detection thresholds ─────────────────────────────────────────────────

    public int getDetectionThresholdsEnable() {
        return dciReadData(DCI_DET_THRESH_GLOBAL_CONFIG, 8)[1] & 0xFF;
    }

    public void setDetectionThresholdsEnable(boolean enabled) {
        byte[] grp = {0x01, (byte) (enabled ? 0x01 : 0x00), 0x01, 0x00};
        dciReplaceData(DCI_DET_THRESH_GLOBAL_CONFIG, 8, grp, 0x00);
        dciReplaceData(DCI_DET_THRESH_CONFIG, 20, new byte[] {(byte) (enabled ? 0x04 : 0x0C)}, 0x11);
    }

    /** The 64 thresholds, low / high rescaled to the units of their measurement. */
    public List<Vl53l8Uld.DetectionThreshold> getDetectionThresholds() {
        byte[] raw = dciReadData(DCI_DET_THRESH_START, Vl53l8Uld.NB_THRESHOLDS * 12);
        List<Vl53l8Uld.DetectionThreshold> result = new ArrayList<>();
        for (int k = 0; k < Vl53l8Uld.NB_THRESHOLDS; k++) {
            int b = k * 12;
            int meas = raw[b + 8] & 0xFF;
            int scale = Vl53l8Uld.threshScale(meas);
            result.add(new Vl53l8Uld.DetectionThreshold(
                    Math.floorDiv(i32le(raw, b), scale), Math.floorDiv(i32le(raw, b + 4), scale),
                    meas, raw[b + 9] & 0xFF, raw[b + 10] & 0xFF, raw[b + 11] & 0xFF));
        }
        return result;
    }

    /** Program the 64 thresholds (missing entries are zeros; more than 64 is refused). */
    public void setDetectionThresholds(List<Vl53l8Uld.DetectionThreshold> thresholds) {
        Vl53l8Uld.PackedThresholds packed = Vl53l8Uld.packDetectionThresholds(thresholds);
        dciWriteData(DCI_DET_THRESH_VALID_STATUS, packed.valid());
        dciWriteData(DCI_DET_THRESH_START, packed.start());
    }

    public void setDetectionThresholdsAutoStop(boolean autoStop) {
        if (firmware == Vl53l8Firmware.L7CX) {
            throw new Vl53l8Error(Vl53l8Error.STATUS_INVALID_PARAM, "detection-threshold auto-stop: not in the L5CX/L7CX ULD");
        }
        dciReplaceData(DCI_PIPE_CONTROL, 4, new byte[] {(byte) (autoStop ? 1 : 0)}, 0x03);
    }

    // ── motion indicator ─────────────────────────────────────────────────────

    /** A default motion-indicator configuration for {@code res}, written to the sensor; frames carry motion output from here on. */
    public Vl53l8Uld.MotionConfig motionIndicatorInit(int res) {
        Vl53l8Uld.MotionConfig cfg = Vl53l8Uld.defaultMotionConfig(RESOLUTION_4X4);
        motionIndicatorSetResolution(cfg, res);
        motionPresent = true;
        return cfg;
    }

    /**
     * Stop surfacing the motion block: frames parse with {@code motion} null. Called when the
     * VL53LMZ CNH setup overwrites the shared DCI 0xBFAC struct (contract 04 §7, ERRATA B4).
     */
    public void motionIndicatorDisarm() {
        motionPresent = false;
    }

    public void motionIndicatorSetResolution(Vl53l8Uld.MotionConfig cfg, int res) {
        if (res != RESOLUTION_4X4 && res != RESOLUTION_8X8) {
            throw new Vl53l8Error(Vl53l8Error.STATUS_INVALID_PARAM, "motion set_resolution");
        }
        Vl53l8Uld.motionConfigSetResolution(cfg, res);
        dciWriteData(DCI_MOTION_DETECTOR_CFG, cfg.pack());
    }

    public void motionIndicatorSetDistanceMotion(Vl53l8Uld.MotionConfig cfg, int distanceMinMm, int distanceMaxMm) {
        if (distanceMaxMm - distanceMinMm > 1500 || distanceMinMm < 400 || distanceMaxMm > 4000) {
            throw new Vl53l8Error(Vl53l8Error.STATUS_INVALID_PARAM, "motion set_distance");
        }
        cfg.refBinOffset = (int) ((distanceMinMm / 37.5348 - 4.0) * 2048.5);
        cfg.featureLength = (int) (((distanceMaxMm - distanceMinMm) / 10.0 + 30.02784) / 15.01392 + 0.5) & 0xFF;
        dciWriteData(DCI_MOTION_DETECTOR_CFG, cfg.pack());
    }

    // ── xtalk calibration run ────────────────────────────────────────────────

    // _vlxxx_program_output_config of the ST xtalk plugin (every version): the 17-block calibration
    // output list, sized with the 0x54D0+960 rule, 24-byte tail.
    private void programXtalkOutputConfig(int res) {
        int[] output = Vl53l8Tables.xtalkCalOutput();
        int[] enable = Vl53l8Tables.xtalkCalOutputEnable();
        int size = 0;
        for (int i = 0; i < output.length; i++) {
            if (output[i] == 0 || (enable[i / 32] & (1 << (i % 32))) == 0) {
                continue;
            }
            int[] f = Vl53l8Uld.bhFields(output[i] & 0xFFFFFFFFL);
            int type = f[0];
            int bhSize = f[1];
            int idx = f[2];
            if (type >= 0x1 && type < 0x0D) {
                bhSize = idx >= 0x54D0 && idx < 0x54D0 + 960 ? res : res * NB_TARGET_PER_ZONE;
                output[i] = bhSetSize(output[i], bhSize);
                size += type * bhSize;
            } else {
                size += bhSize;
            }
            size += 4;
        }
        size += 24;
        dciWriteData(DCI_OUTPUT_LIST, u32s(output));
        dciWriteData(DCI_OUTPUT_CONFIG, u32s(new int[] {size, output.length + 1}));
        dciWriteData(DCI_OUTPUT_ENABLES, u32s(enable));
    }

    /**
     * vl53l8cx_calibrate_xtalk: on-device crosstalk calibration against a flat target at
     * {@code distanceMm} (600..3000) of {@code reflectancePercent} (1..99), averaging
     * {@code nbSamples} (1..16). Takes seconds; the configuration is saved and restored around it.
     * Without cover glass the firmware answers "coverglass too good": {@link #xtalkCalibrationFailed()},
     * and the default buffer stays.
     */
    public void calibrateXtalk(int reflectancePercent, int nbSamples, int distanceMm) {
        if (reflectancePercent < 1 || reflectancePercent > 99) {
            throw new Vl53l8Error(Vl53l8Error.STATUS_INVALID_PARAM, "reflectance 1..99");
        }
        if (distanceMm < 600 || distanceMm > 3000) {
            throw new Vl53l8Error(Vl53l8Error.STATUS_INVALID_PARAM, "distance 600..3000");
        }
        if (nbSamples < 1 || nbSamples > 16) {
            throw new Vl53l8Error(Vl53l8Error.STATUS_INVALID_PARAM, "nb_samples 1..16");
        }

        int savedResolution = getResolution();
        int savedFrequency = getRangingFrequencyHz();
        int savedIntegration = getIntegrationTimeMs();
        int savedSharpener = getSharpenerPercent();
        int savedTargetOrder = getTargetOrder();
        int savedMargin = getXtalkMargin();
        int savedMode = getRangingMode();

        setResolution(RESOLUTION_8X8);
        byte[] table = switch (firmware) {
            case L7CX -> Vl53l8Tables.calibrateXtalkL7cx();
            case CX -> Vl53l8Tables.calibrateXtalkCx();
            default -> Vl53l8Tables.calibrateXtalkLmz();
        };
        p.writeMulti(0x2C28, table);
        pollForAnswer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, "calib cmd");
        int d = distanceMm * 4;
        int r = reflectancePercent * 16;
        dciReplaceData(DCI_CAL_CFG, 8, new byte[] {(byte) d, (byte) (d >> 8)}, 0x00);
        dciReplaceData(DCI_CAL_CFG, 8, new byte[] {(byte) r, (byte) (r >> 8)}, 0x02);
        dciReplaceData(DCI_CAL_CFG, 8, new byte[] {(byte) nbSamples}, 0x04);
        programXtalkOutputConfig(RESOLUTION_8X8);
        p.writeMulti(UI_CMD_END - 3, new byte[] {0x00, 0x03, 0x00, 0x00});
        pollForAnswer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, "calib start");

        xtalkCalibrationFailed = false;
        for (int timeout = 0; ; timeout++) {
            byte[] buf = p.readMulti(0x0, 4);
            if ((buf[0] & 0xFF) != Vl53l8Error.STATUS_ERROR) {
                if ((buf[2] & 0xFF) >= 0x7F && ((buf[3] & 0x80) >> 7) == 1) {
                    // ST: "Coverglass too good for Xtalk calibration" (STATUS_XTALK_FAILED).
                    xtalkData = defaultXtalk;
                    xtalkCalibrationFailed = true;
                }
                break;
            }
            if (timeout >= 400) {
                throw new Vl53l8Error(Vl53l8Error.STATUS_ERROR, "xtalk calibration");
            }
            p.sleepMs(50);
        }
        xtalkData = readXtalkBuffer();
        p.writeMulti(0x2C34, defaultCfg);
        pollForAnswer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, "restore cfg");

        setResolution(savedResolution);
        setRangingFrequencyHz(savedFrequency);
        setIntegrationTimeMs(savedIntegration);
        setSharpenerPercent(savedSharpener);
        setTargetOrder(savedTargetOrder);
        setXtalkMargin(savedMargin);
        setRangingMode(savedMode);
    }
}
