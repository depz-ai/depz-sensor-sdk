package ai.depz.sensor.devices;

import ai.depz.sensor.sensors.bno055.Bno055Regs;

/**
 * One decoded BNO055 register block (streamed or polled). A channel is null when the block did not
 * cover it; values are scaled by {@code units}, the units in effect when the stream started. Euler
 * is (heading, roll, pitch), the quaternion (w, x, y, z); linear acceleration and gravity are always
 * m/s². In non-fusion modes the fusion outputs read zero; in CONFIG everything does.
 */
public record Bno055Sample(
        long timestampUs,
        int addr,
        byte[] raw,
        Bno055Regs.Units units,
        double[] accel,
        double[] mag,
        double[] gyro,
        double[] euler,
        double[] quaternion,
        double[] linearAccel,
        double[] gravity,
        Double temperature,
        Bno055Regs.CalibStatus calibration) {

    /** Decode {@code data} read at {@code addr} with {@code units}. */
    public static Bno055Sample decode(long timestampUs, int addr, byte[] data, Bno055Regs.Units units) {
        Bno055Regs.RawBlock r = Bno055Regs.decodeBlock(addr, data);
        return new Bno055Sample(timestampUs, addr, data.clone(), units,
                scale(r.accel(), units.accelLsb()),
                scale(r.mag(), Bno055Regs.MAG_LSB),
                scale(r.gyro(), units.gyroLsb()),
                scale(r.euler(), units.eulerLsb()),
                scale(r.quaternion(), Bno055Regs.QUAT_LSB),
                scale(r.linearAccel(), Bno055Regs.FUSION_ACCEL_LSB),
                scale(r.gravity(), Bno055Regs.FUSION_ACCEL_LSB),
                r.temperature() == null ? null : r.temperature() / units.tempLsb(),
                r.calibStat() == null ? null : Bno055Regs.CalibStatus.unpack(r.calibStat()));
    }

    private static double[] scale(int[] v, double lsb) {
        if (v == null) {
            return null;
        }
        double[] out = new double[v.length];
        for (int i = 0; i < v.length; i++) {
            out[i] = v[i] / lsb;
        }
        return out;
    }
}
