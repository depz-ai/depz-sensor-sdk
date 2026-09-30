/**
 * uld/l4 — VL53L4CD ULD 2.2.3, for VL53L4CD and VL53L4CX alike. Async port of
 * the Python `depz_sensor_sdk.vl53lx.uld.l4` (VL53L4CD_api.c +
 * VL53L4CD_calibration.c). Its configuration blob, the write that closes its
 * init and its two calibrations; the die body lives in `vl53l1-die.ts`.
 */

import { Vl53Error } from "./link.js";
import { floorDiv, pyRound } from "./arith.js";
import {
  CONFIG_ADDR,
  CONFIG_END,
  INNER_OFFSET_MM,
  OUTER_OFFSET_MM,
  RANGE_OFFSET_MM,
  VL53L1Die,
  XTALK_PLANE_OFFSET_KCPS,
  type DieResultsData,
} from "./vl53l1-die.js";

export const L4_ULD_VERSION = [2, 2, 3, 0] as const;

/** VL53L4CD_DEFAULT_CONFIGURATION[] — 91 bytes, registers 0x2D..0x87. */
export const L4_DEFAULT_CONFIGURATION = Uint8Array.from([
  0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08, // 0x2D..0x34
  0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00, // 0x35..0x3C
  0x00, 0xff, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, // 0x3D..0x44
  0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21, // 0x45..0x4C
  0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8, // 0x4D..0x54
  0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00, // 0x55..0x5C
  0x00, 0x01, 0xcc, 0x07, 0x01, 0xf1, 0x05, 0x00, // 0x5D..0x64
  0xa0, 0x00, 0x80, 0x08, 0x38, 0x00, 0x00, 0x00, // 0x65..0x6C
  0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x6D..0x74
  0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00, // 0x75..0x7C
  0x00, 0x02, 0xc7, 0xff, 0x9b, 0x00, 0x00, 0x00, // 0x7D..0x84
  0x01, 0x00, 0x00, // 0x85..0x87
]);
if (L4_DEFAULT_CONFIGURATION.length !== CONFIG_END - CONFIG_ADDR + 1) {
  throw new Error("VL53L4CD configuration blob must be 91 bytes");
}

/** Port of VL53L4CD_api.c + VL53L4CD_calibration.c (ULD 2.2.3). */
export class VL53L4 extends VL53L1Die {
  static readonly SUPPORTS: ReadonlySet<string> = new Set([
    "timing",
    "offset",
    "xtalk",
    "thresholds",
    "signal_thresh",
    "sigma_thresh",
    "temp_update",
    "calib_offset",
    "calib_xtalk",
  ]);

  protected readonly CONFIGURATION = L4_DEFAULT_CONFIGURATION;

  /** 1.2 m — the VL53L4CD's rating, because this is its blob. */
  override reachMm(): number | null {
    return 1200;
  }

  /** ALGO__RANGE_IGNORE_THRESHOLD_MCPS, the write closing VL53L4CD_SensorInit(). */
  protected override async initExtra(): Promise<void> {
    await this.p.wrWord(0x0024, 0x0500);
  }

  // ── calibration (VL53L4CD_calibration.c) ──
  async calibrateOffset(targetDistMm: number, nbSamples = 20): Promise<number> {
    if (!(nbSamples >= 5 && nbSamples <= 255) || !(targetDistMm >= 10 && targetDistMm <= 1000)) {
      throw new Vl53Error("nb_samples must be 5..255, target 10..1000 mm");
    }
    await this.p.wrWord(RANGE_OFFSET_MM, 0);
    await this.p.wrWord(INNER_OFFSET_MM, 0);
    await this.p.wrWord(OUTER_OFFSET_MM, 0);

    await this.collect(10, () => undefined); // device heat loop

    const distances: number[] = [];
    await this.collect(nbSamples, (_i, r) => distances.push(r.distanceMm));

    const offsetMm = targetDistMm - floorDiv(sum(distances), nbSamples);
    await this.p.wrWord(RANGE_OFFSET_MM, (offsetMm * 4) & 0xffff);
    return offsetMm;
  }

  async calibrateXtalk(targetDistMm: number, nbSamples = 20): Promise<number> {
    if (!(nbSamples >= 5 && nbSamples <= 255) || !(targetDistMm >= 10 && targetDistMm <= 5000)) {
      throw new Vl53Error("nb_samples must be 5..255, target 10..5000 mm");
    }
    await this.p.wrWord(XTALK_PLANE_OFFSET_KCPS, 0); // disable compensation

    await this.collect(10, () => undefined); // device heat loop

    const samples: DieResultsData[] = [];
    await this.collect(nbSamples, (i, r) => {
      // Discard invalid measurements and the first frame.
      if (r.rangeStatus === 0 && i > 0) samples.push(r);
    });
    if (samples.length === 0) throw new Vl53Error("xtalk calibration failed: no valid samples");

    const n = samples.length;
    const avgDistance = sum(samples.map((s) => s.distanceMm)) / n;
    const avgSpadNb = sum(samples.map((s) => s.numberOfSpad)) / n;
    const avgSignal = sum(samples.map((s) => s.signalRateKcps)) / n;

    const tmpXtalk = (1.0 - avgDistance / targetDistMm) * (avgSignal / avgSpadNb);
    if (tmpXtalk > 127) {
      // 127 kcps is the max xtalk value (65536/512)
      throw new Vl53Error(`xtalk calibration failed: ${tmpXtalk.toFixed(1)} kcps > 127`);
    }
    await this.p.wrWord(XTALK_PLANE_OFFSET_KCPS, Math.trunc(tmpXtalk * 512.0) & 0xffff);
    return pyRound(tmpXtalk);
  }
}

function sum(xs: number[]): number {
  let s = 0;
  for (const x of xs) s += x;
  return s;
}
