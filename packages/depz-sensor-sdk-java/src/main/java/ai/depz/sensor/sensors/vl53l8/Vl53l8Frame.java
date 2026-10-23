package ai.depz.sensor.sensors.vl53l8;

/**
 * One decoded ranging frame. Per-zone arrays are sized to the active resolution (16 for 4×4, 64
 * for 8×8); zone index runs row-major. Values are in the real units ST's GetRangingData gives
 * (contract 04 §5); ST divides integers, here the scaled values keep their fraction:
 * {@code distanceMm} mm (raw ÷4 floored, a negative distance clamped to 0, as ST),
 * {@code signalPerSpad} / {@code ambientPerSpad} kcps/SPAD (raw ÷2048), {@code rangeSigmaMm} mm
 * (raw ÷128), {@code reflectance} % (raw ÷2). {@code targetStatus} 5 or 9 = valid, 255 = no
 * target. {@code cnhRaw} is the raw CNH data block (VL53L8CH / VL53L7CH with CNH armed), else
 * null; {@code motion} the motion-indicator output when it is configured, else null.
 */
public record Vl53l8Frame(
        long timestampUs,
        int resolution,
        int siliconTempDegc,
        int[] distanceMm,
        int[] targetStatus,
        int[] nbTargetDetected,
        double[] signalPerSpad,
        double[] ambientPerSpad,
        long[] nbSpadsEnabled,
        double[] rangeSigmaMm,
        double[] reflectance,
        byte[] cnhRaw,
        Vl53l8Motion motion) {

    static Vl53l8Frame from(long timestampUs, Vl53l8Uld.Results r, Vl53l8Motion motion) {
        return new Vl53l8Frame(timestampUs, r.resolution(), r.siliconTempDegc, r.distanceMm, r.targetStatus,
                r.nbTargetDetected, r.signalPerSpad, r.ambientPerSpad, r.nbSpadsEnabled, r.rangeSigmaMm, r.reflectance,
                r.cnhRaw, motion);
    }

    /** This frame with {@code resolution} replaced (the device's own count of zones in use). */
    public Vl53l8Frame withResolution(int zones) {
        return new Vl53l8Frame(timestampUs, zones, siliconTempDegc, distanceMm, targetStatus, nbTargetDetected,
                signalPerSpad, ambientPerSpad, nbSpadsEnabled, rangeSigmaMm, reflectance, cnhRaw, motion);
    }
}
