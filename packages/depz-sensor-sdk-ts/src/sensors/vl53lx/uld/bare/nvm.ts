/**
 * uld/bare/nvm — the raw NVM, which the register blocks do not cover.
 * Mirror of the Python `depz_sensor_sdk.vl53lx.uld.bare.nvm`.
 *
 * Part of the factory calibration never appears in the register map: the
 * optical centre, the 5x5 peak-rate map, the mode-mitigation offset
 * calibration and the FMT range results. To get at them the driver stops the
 * firmware, powers the NVM array and clocks words out of it one at a time —
 * four writes and a read per 32-bit word. That cost is why this reads regions,
 * not the whole array.
 *
 * Every region is wrapped in its own enable/disable pair (as
 * `VL53LX_read_nvm_raw_data()` does): the sensor must not be left with its
 * firmware stopped by an exception in the middle of a multi-region read.
 *
 * Reference: vl53lx_nvm.c, vl53lx_nvm_map.h.
 */

import type { BridgePlatform } from "../base.js";
import type { DeviceImage } from "./image.js";

// ── registers (vl53lx_register_map.h) ────────────────────────────────────────
export const POWER_MANAGEMENT__GO1_POWER_FORCE = 0x0083;
export const FIRMWARE__ENABLE = 0x0085;
export const RANGING_CORE__CLK_CTRL1 = 0x0683;
export const RANGING_CORE__NVM_CTRL__MODE = 0x0780;
export const RANGING_CORE__NVM_CTRL__PDN = 0x0781;
export const RANGING_CORE__NVM_CTRL__READN = 0x0783;
export const RANGING_CORE__NVM_CTRL__PULSE_WIDTH_MSB = 0x0784;
export const RANGING_CORE__NVM_CTRL__DATAOUT_MMM = 0x0790;
export const RANGING_CORE__NVM_CTRL__ADDR = 0x0794;

// ── timings (vl53lx_nvm.h, vl53lx_ll_device.h) ───────────────────────────────
export const NVM_POWER_UP_DELAY_US = 50;
export const NVM_READ_TRIGGER_DELAY_US = 5;
export const ENABLE_POWERFORCE_SETTLING_TIME_US = 250;
export const NVM_CTRL_PULSE_WIDTH = 0x0004;

// ── FMT regions (vl53lx_nvm_map.h), as [byte index, byte size] ───────────────
export type NvmRegion = readonly [number, number];
export const FMT_OPTICAL_CENTRE: NvmRegion = [0x00b8, 4];
export const FMT_CAL_PEAK_RATE_MAP: NvmRegion = [0x015c, 56];
export const FMT_ADDITIONAL_OFFSET_CAL: NvmRegion = [0x0194, 8];
export const FMT_RANGE_RESULTS__140MM_DARK: NvmRegion = [0x01ac, 16];

export const PEAK_RATE_MAP_WIDTH = 5;
export const PEAK_RATE_MAP_HEIGHT = 5;
export const PEAK_RATE_MAP_SAMPLES = PEAK_RATE_MAP_WIDTH * PEAK_RATE_MAP_HEIGHT;

// ── decoded shapes (vl53lx_ll_def.h) — C field names kept ────────────────────
export interface OpticalCentre {
  x_centre: number;
  y_centre: number;
}

export interface CalPeakRateMap {
  cal_distance_mm: number;
  cal_reflectance_pc: number;
  max_samples: number;
  width: number;
  height: number;
  peak_rate_mcps: number[];
}

export interface AdditionalOffsetCalData {
  result__mm_inner_actual_effective_spads: number;
  result__mm_outer_actual_effective_spads: number;
  result__mm_inner_peak_signal_count_rtn_mcps: number;
  result__mm_outer_peak_signal_count_rtn_mcps: number;
}

export interface FmtRangeData {
  result__actual_effective_rtn_spads: number;
  ref_spad_array__num_requested_ref_spads: number;
  ref_spad_array__ref_location: number;
  result__peak_signal_count_rate_rtn_mcps: number;
  result__ambient_count_rate_rtn_mcps: number;
  result__peak_signal_count_rate_ref_mcps: number;
  result__ambient_count_rate_ref_mcps: number;
  measured_distance_mm: number;
  measured_distance_stdev_mm: number;
}

function u16(buf: Uint8Array, off: number): number {
  return (buf[off]! << 8) | buf[off + 1]!;
}

/** Raw NVM access. Holds no state between reads — the sensor does. */
export class NvmReader {
  constructor(
    readonly p: BridgePlatform,
    readonly img: DeviceImage,
  ) {}

  // ── VL53LX_nvm_enable / _disable ──
  private async enable(): Promise<void> {
    // disable_firmware / enable_powerforce, both through system_control so
    // the image stays truthful about what the device was told.
    this.img.system_control.v.firmware__enable = 0;
    await this.p.wrByte(FIRMWARE__ENABLE, 0);
    this.img.system_control.v.power_management__go1_power_force = 1;
    await this.p.wrByte(POWER_MANAGEMENT__GO1_POWER_FORCE, 1);
    await this.p.sleepMs(1); // 250 us settling, rounded up

    await this.p.wrByte(RANGING_CORE__NVM_CTRL__PDN, 0x01);
    await this.p.wrByte(RANGING_CORE__CLK_CTRL1, 0x05);
    await this.p.sleepMs(1); // 50 us power-up, rounded up
    await this.p.wrByte(RANGING_CORE__NVM_CTRL__MODE, 0x01);
    await this.p.wrWord(RANGING_CORE__NVM_CTRL__PULSE_WIDTH_MSB, NVM_CTRL_PULSE_WIDTH);
  }

  private async disable(): Promise<void> {
    await this.p.wrByte(RANGING_CORE__NVM_CTRL__READN, 0x01);
    await this.p.wrByte(RANGING_CORE__NVM_CTRL__PDN, 0x00);
    this.img.system_control.v.power_management__go1_power_force = 0;
    await this.p.wrByte(POWER_MANAGEMENT__GO1_POWER_FORCE, 0);
    this.img.system_control.v.firmware__enable = 1;
    await this.p.wrByte(FIRMWARE__ENABLE, 1);
  }

  // ── VL53LX_nvm_read ──
  private async readWords(startWord: number, count: number): Promise<Uint8Array> {
    const out = new Uint8Array(count * 4);
    for (let i = 0; i < count; i++) {
      await this.p.wrByte(RANGING_CORE__NVM_CTRL__ADDR, startWord + i);
      await this.p.wrByte(RANGING_CORE__NVM_CTRL__READN, 0x00);
      // 5 us trigger delay: a USB round trip is three orders of magnitude
      // longer, so the next transfer is the wait.
      await this.p.wrByte(RANGING_CORE__NVM_CTRL__READN, 0x01);
      out.set(await this.p.rdMulti(RANGING_CORE__NVM_CTRL__DATAOUT_MMM, 4), i * 4);
    }
    return out;
  }

  // ── VL53LX_read_nvm_raw_data ──
  /** [byte index, byte size] → the bytes. Both must be word-aligned. */
  async readRegion(region: NvmRegion): Promise<Uint8Array> {
    const [index, size] = region;
    if (index & 3 || size & 3) {
      throw new RangeError(
        `NVM region 0x${index.toString(16).toUpperCase().padStart(4, "0")}+${size} is not word-aligned`,
      );
    }
    await this.enable();
    try {
      return await this.readWords(index >> 2, size >> 2);
    } finally {
      await this.disable();
    }
  }

  // ── the four decoders the driver needs ──
  async opticalCentre(): Promise<OpticalCentre> {
    const buf = await this.readRegion(FMT_OPTICAL_CENTRE);
    // Stored as a distance down from 0x0100; on a never-programmed part the
    // subtraction overflows and the driver zeroes it.
    const x = 0x0100 - buf[2]!;
    return { x_centre: x <= 0xff ? x : 0, y_centre: buf[3]! };
  }

  async calPeakRateMap(): Promise<CalPeakRateMap> {
    const buf = await this.readRegion(FMT_CAL_PEAK_RATE_MAP);
    const peak: number[] = [];
    for (let i = 0; i < PEAK_RATE_MAP_SAMPLES; i++) peak.push(u16(buf, 4 + 2 * i));
    return {
      cal_distance_mm: u16(buf, 0),
      cal_reflectance_pc: u16(buf, 2) >> 6,
      max_samples: PEAK_RATE_MAP_SAMPLES,
      width: PEAK_RATE_MAP_WIDTH,
      height: PEAK_RATE_MAP_HEIGHT,
      peak_rate_mcps: peak,
    };
  }

  async additionalOffsetCalData(): Promise<AdditionalOffsetCalData> {
    const buf = await this.readRegion(FMT_ADDITIONAL_OFFSET_CAL);
    return {
      result__mm_inner_actual_effective_spads: u16(buf, 0),
      result__mm_outer_actual_effective_spads: u16(buf, 2),
      result__mm_inner_peak_signal_count_rtn_mcps: u16(buf, 4),
      result__mm_outer_peak_signal_count_rtn_mcps: u16(buf, 6),
    };
  }

  async fmtRangeResults(region: NvmRegion = FMT_RANGE_RESULTS__140MM_DARK): Promise<FmtRangeData> {
    const buf = await this.readRegion(region);
    return {
      result__actual_effective_rtn_spads: u16(buf, 0),
      ref_spad_array__num_requested_ref_spads: buf[2]!,
      ref_spad_array__ref_location: buf[3]!,
      result__peak_signal_count_rate_rtn_mcps: u16(buf, 4),
      result__ambient_count_rate_rtn_mcps: u16(buf, 6),
      result__peak_signal_count_rate_ref_mcps: u16(buf, 8),
      result__ambient_count_rate_ref_mcps: u16(buf, 10),
      measured_distance_mm: u16(buf, 12),
      measured_distance_stdev_mm: u16(buf, 14),
    };
  }
}
