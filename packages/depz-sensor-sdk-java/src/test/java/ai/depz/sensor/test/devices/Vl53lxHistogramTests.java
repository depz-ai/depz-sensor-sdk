package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.throwsType;

import ai.depz.sensor.sensors.vl53lx.Vl53lxHistogramDriver;
import ai.depz.sensor.sensors.vl53lx.Vl53lxPlatform;
import ai.depz.sensor.sensors.vl53lx.bare.BareDriver;
import ai.depz.sensor.sensors.vl53lx.bare.Hist;
import ai.depz.sensor.sensors.vl53lx.bare.HistogramBinData;
import ai.depz.sensor.sensors.vl53lx.bare.RegBlock;
import java.util.Arrays;
import java.util.List;
import java.util.NoSuchElementException;

/** The VL53LX histogram driver: the register-block codec, the bin folding, the die checks, the maths. */
public final class Vl53lxHistogramTests {
    // The BareDriver touches the bus only where a test does not reach.
    static final class NoBus extends Vl53lxPlatform {
        @Override
        public byte[] readMulti(int addr, int size) {
            throw new IllegalStateException("no bus");
        }

        @Override
        public void writeMulti(int addr, byte[] data) {
            throw new IllegalStateException("no bus");
        }

        @Override
        public void setI2cSpeed(int khz) {}

        @Override
        public void setAddrWidth(int width) {}

        @Override
        public void xshutReset() {}
    }

    public static void regBlockRoundTripsByteExactWithSignedMaskedFields() {
        byte[] raw = new byte[23];
        for (int i = 0; i < raw.length; i++) {
            raw[i] = (byte) (0xA0 + i);
        }
        raw[17] = (byte) 0xFF; // algo__part_to_part_range_offset_mm: signed, mask 0x1FFF
        raw[18] = (byte) 0xF0;
        RegBlock blk = new RegBlock("customer_nvm_managed").decode(raw);
        eq(0x1FF0L, blk.get("algo__part_to_part_range_offset_mm")); // masked first, so it reads positive
        eq((long) (raw[0] & 0xFF), blk.get("global_config__spad_enables_ref_0"));
        blk.set("mm_config__inner_offset_mm", -5);
        byte[] enc = blk.encode();
        eq(new byte[] {(byte) 0xFF, (byte) 0xFB}, Arrays.copyOfRange(enc, 19, 21));
        eq(-5L, new RegBlock("customer_nvm_managed").decode(enc).get("mm_config__inner_offset_mm"));
        throwsType(NoSuchElementException.class, () -> blk.get("no_such_field"));
    }

    public static void repeatedBinCodesAreAveragedIntoOneBinEach() {
        HistogramBinData src = new HistogramBinData();
        src.binSeq = new int[] {7, 0, 1, 1, 2, 2};
        for (int i = 0; i < 24; i++) {
            src.binData[i] = i < 4 ? 100 : 1000 + i;
        }
        HistogramBinData dst = Hist.averageRepeatedBins(src);
        eq(new int[] {7, 0, 1, 2, 16, 16}, dst.binSeq);
        eq(new long[] {1, 1, 2, 2, 0, 0}, dst.binRep);
        eq(16L, dst.binsInData);
        eq(4L, dst.numberOfAmbientBins);
        eq((long) ((1008 + 1012 + 1) / 2), dst.binData[8]); // codes 1 at bins 8 and 12, rounded half up
    }

    public static void anL4DieRefusesTheShortPreset() {
        BareDriver bare = new BareDriver(new NoBus());
        bare.img().nvmCopyData().set("identification__module_type", 0xAA);
        bare.img().nvmCopyData().set("identification__model_id", 0xEB);
        check(bare.isL4(), "an 0xAA / 0xEB die is an L4");
        eq(List.of("medium", "long"), bare.distanceModesForDie());
        eq(200000L, bare.fdaMaxTimingBudgetUs());
        throwsType(IllegalArgumentException.class, () -> bare.setDistanceMode("short"));
    }

    public static void anUnknownModeIsAnInvalidArgument() {
        // Checked before anything touches the bus: the scenarios expect invalid_argument.
        Vl53lxHistogramDriver drv = new Vl53lxHistogramDriver(new NoBus(), "vl53l4cx");
        throwsType(IllegalArgumentException.class, () -> drv.setMode("very_long"));
        eq(List.of("mode", "timing"), List.copyOf(drv.supports()));
        check(drv.histogram(), "the histogram driver says so");
    }

    public static void isqrtIsTheFloorOfAUint32Root() {
        eq(65535L, Hist.isqrt(0xFFFFFFFFL));
        eq(3L, Hist.isqrt(15));
        eq(4L, Hist.isqrt(16));
        eq(0L, Hist.isqrt(1L << 32)); // the argument is uint32 in C
    }

    public static void binDataFieldsCarryThePythonNames() {
        var f = new HistogramBinData().fields();
        eq(30, f.size());
        check(f.containsKey("result__dss_actual_effective_spads") && f.containsKey("bin_seq")
                && f.containsKey("roi_config__user_roi_requested_global_xy_size"), "snake_case keys");
        eq(24, ((List<?>) f.get("bin_data")).size());
    }
}
