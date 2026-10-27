package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.throwsType;

import ai.depz.sensor.sensors.vl53lx.bare.BareDeviceState;
import ai.depz.sensor.sensors.vl53lx.bare.BareTables;
import ai.depz.sensor.sensors.vl53lx.bare.DeviceImage;
import ai.depz.sensor.sensors.vl53lx.bare.LlState;
import java.util.ArrayList;
import java.util.List;
import java.util.NoSuchElementException;

/**
 * The BareDriver's register image (multi-block transfers, dump, unknown blocks — the Python tests'
 * test_vl53lx_bare_driver.py) and its single-zone ll-driver state machine (bare/core.py LLState).
 */
public final class Vl53lxBareImageTests {
    private Vl53lxBareImageTests() {}

    // ── DeviceImage ──────────────────────────────────────────────────────────

    public static void pullRangeIsOneReadSpanningTheBlocks() {
        Vl53lxDriverTests.RegMap p = new Vl53lxDriverTests.RegMap();
        DeviceImage img = new DeviceImage(p);
        p.setWord(0x0018, 0xFFFE);   // algo__crosstalk_compensation_x_plane_gradient_kcps
        p.regs[0x001E] = (byte) 0xE0; // algo__part_to_part_range_offset_mm, high byte
        p.regs[0x001F] = 0x04;        // → 0xE004 & 0x1FFF = 0x0004
        img.pullRange(List.of("static_nvm_managed", "customer_nvm_managed"));
        // 0x0001..0x0023: static_nvm (11) + the reserved 0x000C + customer_nvm (23)
        eq(1, p.reads.size());
        eq(new int[] {0x0001, 35}, p.reads.get(0));
        eq((long) FakeVl53lxBridge.DIE_OSC, img.staticNvm().get("osc_measured__fast_osc__frequency"));
        eq(-2L, img.customerNvm().get("algo__crosstalk_compensation_x_plane_gradient_kcps"));
        eq(4L, img.customerNvm().get("algo__part_to_part_range_offset_mm"));
    }

    public static void pushRangeGoesOutAsOneWriteWithTheReservedByteZeroed() {
        Vl53lxDriverTests.RegMap p = new Vl53lxDriverTests.RegMap();
        DeviceImage img = new DeviceImage(p);
        p.regs[0x000C] = 0x77; // the hole: what the sensor holds there is overwritten with 0x00
        img.staticNvm().set("vhv_config__init", 0x5A);
        img.customerNvm().set("mm_config__outer_offset_mm", -1);
        img.pushRange(List.of("static_nvm_managed", "customer_nvm_managed"));
        eq(1, p.writes.size());
        eq(new int[] {0x0001, 35}, p.writes.get(0));
        eq(0x5A, p.regs[0x000B] & 0xFF);  // last byte of static_nvm
        eq(0x00, p.regs[0x000C] & 0xFF);  // the hole at 0x000C
        eq(0xFFFF, p.word(0x0022));
    }

    public static void onePullAndOnePushMoveOneBlock() {
        Vl53lxDriverTests.RegMap p = new Vl53lxDriverTests.RegMap();
        DeviceImage img = new DeviceImage(p);
        p.regs[0x0087] = 0x42; // system__mode_start
        eq(0x42L, img.pull("system_control").get("system__mode_start"));
        eq(new int[] {0x0083, 5}, p.reads.get(0));
        img.systemControl().set("system__mode_start", 0x21);
        img.push("system_control");
        eq(new int[] {0x0083, 5}, p.writes.get(0));
        eq(0x21, p.regs[0x0087] & 0xFF);
        // the named accessors are the blocks the index reaches
        check(T.same(img.get("general_config"), img.generalConfig()), "general_config");
        check(T.same(img.get("timing_config"), img.timingConfig()), "timing_config");
        check(T.same(img.get("dynamic_config"), img.dynamicConfig()), "dynamic_config");
        check(T.same(img.get("static_config"), img.staticConfig()), "static_config");
        check(T.same(img.get("nvm_copy_data"), img.nvmCopyData()), "nvm_copy_data");
        eq(List.of("static_nvm_managed", "customer_nvm_managed", "static_config", "general_config",
                "timing_config", "dynamic_config", "system_control"), DeviceImage.RANGE_START_BLOCKS);
    }

    public static void anUnknownBlockIsRefused() {
        DeviceImage img = new DeviceImage(new Vl53lxHistogramTests.NoBus());
        eq("no register block nonsense", throwsType(NoSuchElementException.class, () -> img.get("nonsense")).getMessage());
        throwsType(NoSuchElementException.class, () -> img.pull("nonsense"));
        throwsType(NoSuchElementException.class, () -> img.push("nonsense"));
    }

    public static void dumpListsEveryFieldOfTheNamedBlocks() {
        DeviceImage img = new DeviceImage(new Vl53lxHistogramTests.NoBus());
        img.systemControl().set("system__mode_start", 0x40);
        List<String> lines = img.dump(List.of("system_control")).lines().toList();
        eq("--- system_control @ 0x0083 (5 B)", lines.get(0));
        eq(1 + BareTables.BLOCKS.get("system_control").fields().size(), lines.size());
        eq(List.of("system__mode_start", "64"), List.of(lines.get(lines.size() - 1).trim().split("\\s+", -1)));
        // default: all blocks, in BLOCKS order
        List<String> headers = new ArrayList<>();
        for (String l : img.dump().lines().toList()) {
            if (l.startsWith("--- ")) {
                headers.add(l.split(" ", -1)[1]);
            }
        }
        eq(new ArrayList<>(BareTables.BLOCKS.keySet()), headers);
    }

    // ── LlState ──────────────────────────────────────────────────────────────

    private static final int RANGING = 0x40; // a mode_start with a measurement mode (mask 0xF0)

    public static void rdStateWalksStandbyGphSyncThenOutputData() {
        LlState s = new LlState();
        eq(BareDeviceState.SW_STANDBY, s.rdDeviceState);
        eq(2L, s.rdGphId);
        // standby, GPH id bit set in the hold register: wait for the GPH sync
        s.updateRd(RANGING, 0x02);
        eq(BareDeviceState.RANGING_WAIT_GPH_SYNC, s.rdDeviceState);
        eq(new long[] {0, 0, 0}, new long[] {s.rdStreamCount, s.rdGphId, s.rdTimingStatus});
        s.updateRd(RANGING, 0x02);
        eq(BareDeviceState.RANGING_OUTPUT_DATA, s.rdDeviceState);
        eq(new long[] {0, 2, 0}, new long[] {s.rdStreamCount, s.rdGphId, s.rdTimingStatus});
        // output data: the stream counts, the GPH id and the timing status flip each frame
        s.updateRd(RANGING, 0);
        eq(new long[] {1, 0, 1}, new long[] {s.rdStreamCount, s.rdGphId, s.rdTimingStatus});
        s.updateRd(RANGING, 0);
        eq(new long[] {2, 2, 0}, new long[] {s.rdStreamCount, s.rdGphId, s.rdTimingStatus});
        // the stream count wraps 0xFF -> 0x80, as on the die
        s.rdStreamCount = 0xFF;
        s.updateRd(RANGING, 0);
        eq(0x80L, s.rdStreamCount);
        // mode_start without a measurement mode: back to standby, the cfg side untouched
        s.cfgStreamCount = 9;
        s.updateRd(0x01, 0);
        eq(BareDeviceState.SW_STANDBY, s.rdDeviceState);
        eq(new long[] {0, 2, 0}, new long[] {s.rdStreamCount, s.rdGphId, s.rdTimingStatus});
        eq(9L, s.cfgStreamCount);
    }

    public static void rdStateWithoutGphGoesStraightToOutputData() {
        LlState s = new LlState();
        s.rdTimingStatus = 1;
        s.rdStreamCount = 5;
        s.updateRd(RANGING, 0x00);
        eq(BareDeviceState.RANGING_OUTPUT_DATA, s.rdDeviceState);
        eq(new long[] {0, 0, 0}, new long[] {s.rdStreamCount, s.rdGphId, s.rdTimingStatus});
    }

    public static void cfgStateGoesToDssAutoAndFlipsTheTimingStatus() {
        LlState s = new LlState();
        s.updateCfg(RANGING);
        eq(BareDeviceState.RANGING_DSS_AUTO, s.cfgDeviceState);
        eq(new long[] {1, 0, 1}, new long[] {s.cfgStreamCount, s.cfgGphId, s.cfgTimingStatus});
        s.updateCfg(RANGING);
        eq(BareDeviceState.RANGING_DSS_AUTO, s.cfgDeviceState);
        eq(new long[] {2, 2, 0}, new long[] {s.cfgStreamCount, s.cfgGphId, s.cfgTimingStatus});
        s.cfgStreamCount = 0xFF;
        s.updateCfg(RANGING);
        eq(0x80L, s.cfgStreamCount);
        s.rdStreamCount = 7;
        s.updateCfg(0x00);
        eq(BareDeviceState.SW_STANDBY, s.cfgDeviceState);
        eq(new long[] {0, 2, 0}, new long[] {s.cfgStreamCount, s.cfgGphId, s.cfgTimingStatus});
        eq(7L, s.rdStreamCount, "the rd side untouched");
    }

    public static void aStateTheSideDoesNotKnowResetsBothSides() {
        LlState s = new LlState();
        s.rdDeviceState = BareDeviceState.RANGING_DSS_AUTO; // a cfg-only state on the rd side
        s.cfgStreamCount = 4;
        s.updateRd(RANGING, 0);
        eq(BareDeviceState.SW_STANDBY, s.rdDeviceState);
        eq(new long[] {0, 2, 0, 0}, new long[] {s.rdStreamCount, s.rdGphId, s.rdTimingStatus, s.cfgStreamCount});

        LlState c = new LlState();
        c.cfgDeviceState = BareDeviceState.RANGING_OUTPUT_DATA; // an rd-only state on the cfg side
        c.rdTimingStatus = 1;
        c.updateCfg(RANGING);
        eq(BareDeviceState.SW_STANDBY, c.cfgDeviceState);
        eq(new long[] {0, 2, 0, 0}, new long[] {c.cfgStreamCount, c.cfgGphId, c.cfgTimingStatus, c.rdTimingStatus});
    }
}
