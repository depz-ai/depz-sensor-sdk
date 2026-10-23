package ai.depz.sensor.sensors.vl53l4;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/**
 * The ST VL53L4CD ULD 2.2.3 ({@code VL53L4CD_api.c} + {@code VL53L4CD_calibration.c}) on the host,
 * ported from the reference Python driver: init, ranging, timing, offset, crosstalk, thresholds,
 * temperature update, calibration. Every request sequence is the Python one, so captures of either
 * replay in both. Register contents are big-endian.
 */
public final class Vl53l4cdDriver {
    private final Vl53l4Platform p;

    public Vl53l4cdDriver(Vl53l4Platform platform) {
        this.p = Objects.requireNonNull(platform, "platform");
    }

    // ── register access ──────────────────────────────────────────────────────

    public int readByte(int addr) {
        return p.readMulti(addr, 1)[0] & 0xFF;
    }

    public int readWord(int addr) {
        byte[] b = p.readMulti(addr, 2);
        return ((b[0] & 0xFF) << 8) | (b[1] & 0xFF);
    }

    public long readDword(int addr) {
        byte[] b = p.readMulti(addr, 4);
        return ((long) (b[0] & 0xFF) << 24) | ((b[1] & 0xFF) << 16) | ((b[2] & 0xFF) << 8) | (b[3] & 0xFF);
    }

    public void writeByte(int addr, int value) {
        p.writeMulti(addr, new byte[] {(byte) value});
    }

    public void writeWord(int addr, int value) {
        p.writeMulti(addr, new byte[] {(byte) (value >> 8), (byte) value});
    }

    public void writeDword(int addr, long value) {
        p.writeMulti(addr, new byte[] {(byte) (value >> 24), (byte) (value >> 16), (byte) (value >> 8), (byte) value});
    }

    // ── identity and init ────────────────────────────────────────────────────

    public int getSensorId() {
        return readWord(Vl53l4Uld.IDENTIFICATION_MODEL_ID);
    }

    public boolean isAlive() {
        return getSensorId() == Vl53l4Uld.MODEL_ID;
    }

    /** @throws Vl53l4Uld.Vl53l4Error FIRMWARE__SYSTEM_STATUS never read 0x03 */
    public void waitBoot(int timeoutMs) {
        for (int i = 0; i < Math.max(1, timeoutMs); i++) {
            if (readByte(Vl53l4Uld.FIRMWARE_SYSTEM_STATUS) == 0x03) {
                return;
            }
            p.sleepMs(1);
        }
        throw new Vl53l4Uld.Vl53l4Error("timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03");
    }

    /**
     * Initialise the sensor and leave the bus at {@code busKhz}. The configuration block goes at
     * 400 kHz, the only speed an unconfigured sensor is specified for; the bridge is re-timed after.
     */
    public void sensorInit(int busKhz) {
        p.setI2cSpeed(Vl53l4Uld.I2C_KHZ_BOOT);
        waitBoot(1000);
        // The C driver writes the 91 bytes one register at a time; auto-increment makes it one write.
        p.writeMulti(Vl53l4Uld.CONFIG_ADDR, Vl53l4Uld.configBlock());
        if (busKhz != Vl53l4Uld.I2C_KHZ_BOOT) {
            p.setI2cSpeed(busKhz);
        }

        writeByte(Vl53l4Uld.SYSTEM_START, 0x40); // start VHV
        waitDataReady(1000);
        clearInterrupt();
        stopRanging();
        writeByte(Vl53l4Uld.VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND, 0x09);
        writeByte(0x000B, 0x00);
        writeWord(0x0024, 0x0500);

        setRangeTiming(50, 0);
    }

    public void sensorInit() {
        sensorInit(Vl53l4Uld.I2C_KHZ_DEFAULT);
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    public void clearInterrupt() {
        writeByte(Vl53l4Uld.SYSTEM_INTERRUPT_CLEAR, 0x01);
    }

    public void startRanging() {
        // INTERMEASUREMENT 0 = continuous, anything else = autonomous low power.
        int mode = readDword(Vl53l4Uld.INTERMEASUREMENT_MS) == 0 ? 0x21 : 0x40;
        writeByte(Vl53l4Uld.SYSTEM_START, mode);
    }

    public void stopRanging() {
        writeByte(Vl53l4Uld.SYSTEM_START, 0x80);
    }

    public boolean checkForDataReady() {
        int intPol = ((readByte(Vl53l4Uld.GPIO_HV_MUX_CTRL) & 0x10) >> 4) == 1 ? 0 : 1;
        return (readByte(Vl53l4Uld.GPIO_TIO_HV_STATUS) & 1) == intPol;
    }

    /** @throws Vl53l4Uld.Vl53l4Error no data ready within {@code timeoutMs} polls */
    public void waitDataReady(int timeoutMs) {
        for (int i = 0; i < Math.max(1, timeoutMs); i++) {
            if (checkForDataReady()) {
                return;
            }
            p.sleepMs(1);
        }
        throw new Vl53l4Uld.Vl53l4Error("timeout waiting for data ready");
    }

    /** One block read instead of the C driver's six register reads; the decoding is identical. */
    public Vl53l4Uld.Results getResult() {
        return Vl53l4Uld.parseResultBlock(p.readMulti(Vl53l4Uld.RESULT_BLOCK_ADDR, Vl53l4Uld.RESULT_BLOCK_LEN));
    }

    // ── timing ───────────────────────────────────────────────────────────────

    public void setRangeTiming(int timingBudgetMs, long interMeasurementMs) {
        int clockPll = interMeasurementMs > 0 ? readWord(Vl53l4Uld.RESULT_OSC_CALIBRATE_VAL) : 0;
        int[] r = Vl53l4Uld.rangeTimingRegisters(timingBudgetMs, interMeasurementMs, readWord(Vl53l4Uld.OSC_FREQUENCY), clockPll);
        writeDword(Vl53l4Uld.INTERMEASUREMENT_MS, Integer.toUnsignedLong(r[2]));
        writeWord(Vl53l4Uld.RANGE_CONFIG_A, r[0]);
        writeWord(Vl53l4Uld.RANGE_CONFIG_B, r[1]);
    }

    /** (timing budget ms, inter-measurement ms; 0 = continuous). */
    public RangeTiming getRangeTiming() {
        int[] t = Vl53l4Uld.decodeRangeTiming(
                readDword(Vl53l4Uld.INTERMEASUREMENT_MS),
                readWord(Vl53l4Uld.RESULT_OSC_CALIBRATE_VAL),
                readWord(Vl53l4Uld.OSC_FREQUENCY),
                readWord(Vl53l4Uld.RANGE_CONFIG_A));
        return new RangeTiming(t[0], t[1]);
    }

    // ── offset and crosstalk ─────────────────────────────────────────────────

    public void setOffset(int offsetMm) {
        writeWord(Vl53l4Uld.RANGE_OFFSET_MM, Vl53l4Uld.offsetRaw(offsetMm));
        writeWord(Vl53l4Uld.INNER_OFFSET_MM, 0);
        writeWord(Vl53l4Uld.OUTER_OFFSET_MM, 0);
    }

    public int getOffset() {
        return Vl53l4Uld.decodeOffset(readWord(Vl53l4Uld.RANGE_OFFSET_MM));
    }

    public void setXtalk(int xtalkKcps) {
        writeWord(Vl53l4Uld.XTALK_X_PLANE_GRADIENT_KCPS, 0x0000);
        writeWord(Vl53l4Uld.XTALK_Y_PLANE_GRADIENT_KCPS, 0x0000);
        writeWord(Vl53l4Uld.XTALK_PLANE_OFFSET_KCPS, Vl53l4Uld.xtalkRaw(xtalkKcps));
    }

    public int getXtalk() {
        return Vl53l4Uld.decodeXtalk(readWord(Vl53l4Uld.XTALK_PLANE_OFFSET_KCPS));
    }

    // ── thresholds ───────────────────────────────────────────────────────────

    /** The distance-window interrupt; {@code window} is one of {@link Vl53l4Uld#WINDOW_BELOW} … {@link Vl53l4Uld#WINDOW_IN}. */
    public void setDetectionThresholds(int distanceLowMm, int distanceHighMm, int window) {
        writeByte(Vl53l4Uld.SYSTEM_INTERRUPT, window);
        writeWord(Vl53l4Uld.THRESH_HIGH, distanceHighMm);
        writeWord(Vl53l4Uld.THRESH_LOW, distanceLowMm);
    }

    public DetectionThresholds getDetectionThresholds() {
        int high = readWord(Vl53l4Uld.THRESH_HIGH);
        int low = readWord(Vl53l4Uld.THRESH_LOW);
        return new DetectionThresholds(low, high, readByte(Vl53l4Uld.SYSTEM_INTERRUPT) & 0x07);
    }

    public void setSignalThreshold(int signalKcps) {
        writeWord(Vl53l4Uld.MIN_COUNT_RATE_RTN_LIMIT_MCPS, Vl53l4Uld.signalThresholdRaw(signalKcps));
    }

    public int getSignalThreshold() {
        return Vl53l4Uld.decodeSignalThreshold(readWord(Vl53l4Uld.MIN_COUNT_RATE_RTN_LIMIT_MCPS));
    }

    public void setSigmaThreshold(int sigmaMm) {
        writeWord(Vl53l4Uld.RANGE_CONFIG_SIGMA_THRESH, Vl53l4Uld.sigmaThresholdRaw(sigmaMm));
    }

    public int getSigmaThreshold() {
        return Vl53l4Uld.decodeSigmaThreshold(readWord(Vl53l4Uld.RANGE_CONFIG_SIGMA_THRESH));
    }

    // ── temperature and calibration ──────────────────────────────────────────

    /** Re-run VHV; recommended after a >8 °C ambient change (ST Example_3). */
    public void startTemperatureUpdate() {
        writeByte(Vl53l4Uld.VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND, 0x81);
        writeByte(0x000B, 0x92);
        startRanging();
        waitDataReady(1000);
        clearInterrupt();
        stopRanging();
        writeByte(Vl53l4Uld.VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND, 0x09);
        writeByte(0x000B, 0x00);
    }

    private interface Sample {
        void accept(int index, Vl53l4Uld.Results r);
    }

    // The ranging loop both calibrations share: data ready, result, clear, n times.
    private void collect(int nbSamples, Sample onSample) {
        startRanging();
        for (int i = 0; i < nbSamples; i++) {
            waitDataReady(5000);
            Vl53l4Uld.Results r = getResult();
            clearInterrupt();
            onSample.accept(i, r);
        }
        stopRanging();
    }

    /** Offset calibration against a target at {@code targetDistMm} (10–1000); returns the offset now programmed, mm. */
    public int calibrateOffset(int targetDistMm, int nbSamples) {
        if (nbSamples < 5 || nbSamples > 255 || targetDistMm < 10 || targetDistMm > 1000) {
            throw new Vl53l4Uld.Vl53l4Error("nb_samples must be 5..255, target 10..1000 mm");
        }
        writeWord(Vl53l4Uld.RANGE_OFFSET_MM, 0);
        writeWord(Vl53l4Uld.INNER_OFFSET_MM, 0);
        writeWord(Vl53l4Uld.OUTER_OFFSET_MM, 0);

        collect(10, (i, r) -> { }); // device heat loop

        long[] sum = {0};
        collect(nbSamples, (i, r) -> sum[0] += r.distanceMm());

        int offsetMm = targetDistMm - (int) Math.floorDiv(sum[0], (long) nbSamples);
        writeWord(Vl53l4Uld.RANGE_OFFSET_MM, Vl53l4Uld.offsetRaw(offsetMm));
        return offsetMm;
    }

    /** Crosstalk calibration against a target at {@code targetDistMm} (10–5000); returns the xtalk now programmed, kcps (C rounding). */
    public int calibrateXtalk(int targetDistMm, int nbSamples) {
        if (nbSamples < 5 || nbSamples > 255 || targetDistMm < 10 || targetDistMm > 5000) {
            throw new Vl53l4Uld.Vl53l4Error("nb_samples must be 5..255, target 10..5000 mm");
        }
        writeWord(Vl53l4Uld.XTALK_PLANE_OFFSET_KCPS, 0); // compensation off

        collect(10, (i, r) -> { }); // device heat loop

        List<Vl53l4Uld.Results> samples = new ArrayList<>();
        collect(nbSamples, (i, r) -> {
            if (r.rangeStatus() == 0 && i > 0) { // invalid measurements and the first frame are dropped
                samples.add(r);
            }
        });
        if (samples.isEmpty()) {
            throw new Vl53l4Uld.Vl53l4Error("xtalk calibration failed: no valid samples");
        }
        double n = samples.size();
        double avgDistance = 0;
        double avgSpadNb = 0;
        double avgSignal = 0;
        for (Vl53l4Uld.Results s : samples) {
            avgDistance += s.distanceMm();
            avgSpadNb += s.numberOfSpad();
            avgSignal += s.signalRateKcps();
        }
        avgDistance /= n;
        avgSpadNb /= n;
        avgSignal /= n;
        double tmpXtalk = (1.0 - avgDistance / targetDistMm) * (avgSignal / avgSpadNb);
        if (tmpXtalk > 127) { // 65536 / 512
            throw new Vl53l4Uld.Vl53l4Error(String.format(java.util.Locale.ROOT,
                    "xtalk calibration failed: %.1f kcps > 127", tmpXtalk));
        }
        writeWord(Vl53l4Uld.XTALK_PLANE_OFFSET_KCPS, (int) (tmpXtalk * 512.0) & 0xFFFF);
        return Vl53l4Uld.roundHalfAway(tmpXtalk);
    }
}
