/**
 * VL53L5CX / VL53L7CX / VL53L7CH ToF sensors over the I2C register bridge.
 *
 * One firmware (`APP_VL53L7`) serves all three boards (contracts/11): the MCU
 * is a thin I2C bridge whose wire protocol is the VL53L8 one (contract 04)
 * plus three commands. The sensor-side logic is the same ST ULD family as
 * VL53L8, so these classes reuse `Vl53l8cx` and override only what differs:
 * the blob set (`l7cx` / `l7ch`) and the L5/L7 boot branch (in `uld.ts`), the
 * register-bridge transfer limits, the 1 Hz minimum, and the board commands.
 *
 * The advanced ULD plugins (power modes, xtalk calibration, detection
 * thresholds, motion indicator) share the VL53L8 DCI sequences — verified
 * against ST ULD 2.0.1 (L5CX/L7CX) and VL53LMZ 2.0.16 (L7CH). L5CX/L7CX have
 * no deep sleep and no threshold auto-stop.
 *
 * Mirrors the Python reference `depz_sensor_sdk.vl53l7`.
 */

import { DepzDevice } from "../../device/device.js";
import { DepzError } from "../../errors.js";
import {
  PinAction,
  VL53L7_READ_MAX_LEN,
  VL53L7_WRITE_MAX_LEN,
  Vl53l7Cmd,
  Vl53l7Rpt,
  packPinCtrl,
  packVl53l7SetI2cSpeed,
  unpackVl53l7Info,
  type Vl53l7Info,
} from "../../protocol/vl53l7.js";
import type { Vl53l8Variant } from "../vl53l8/assets/index.js";
import { MI_CFG_DEV_IDX, type CnhConfig } from "../vl53l8/cnh.js";
import { MODULE_TYPE_MZ, MODULE_TYPE_MZEVO, MODULE_TYPE_NAMES } from "../vl53l8/uld.js";
import { Vl53l8cx, type Vl53l8Frame, type Vl53l8InitOptions } from "../vl53l8/vl53l8.js";

/** L5/L7 range and stream at 1 Hz (contract 11; the L8 needs ≥ 2 Hz). */
export const VL53L7_MIN_RANGING_FREQUENCY_HZ = 1;

/** Frames are the VL53L8 frame type. */
export type Vl53l7Frame = Vl53l8Frame;

/**
 * VL53L7CX ToF device (8×8 zones, 90° field of view). `init()` downloads the
 * ~84 KB L5/L7 sensor firmware (~1.4 s at the board's default 1 MHz I2C),
 * then configure and `startRanging()` exactly as on `Vl53l8cx`.
 *
 * Base class of the I2C family: `Vl53l5cx` (same blob, MZ module) and
 * `Vl53l7ch` (CH blob, adds CNH) inherit it.
 */
export class Vl53l7cx extends Vl53l8cx {
  protected override readonly variantId: Vl53l8Variant = "l7cx";
  protected override readonly readChunk: number = VL53L7_READ_MAX_LEN;
  protected override readonly writeChunk: number = VL53L7_WRITE_MAX_LEN;
  protected override readonly minRangingHz: number = VL53L7_MIN_RANGING_FREQUENCY_HZ;
  /** module_type this class expects after init() (L5 and L7 share a blob). */
  protected readonly expectedModuleType: number = MODULE_TYPE_MZEVO;

  /**
   * Initialize the sensor: firmware blob download + default config. Refuses
   * non-L5/L7 silicon. The sensor's `moduleType` then tells L5 (MZ) from L7
   * (MZEVO); a mismatch with this class is reported as a warning — ranging
   * still works, since the blob is shared.
   */
  override async init(variant?: Vl53l8Variant, opts?: Vl53l8InitOptions): Promise<void> {
    await super.init(variant, opts);
    const got = this.moduleType;
    if (got !== null && got !== this.expectedModuleType) {
      console.warn(
        `[depz-sensor-sdk] ${this.constructor.name} expects module type ` +
          `${MODULE_TYPE_NAMES[this.expectedModuleType]}, the sensor reports ` +
          `${MODULE_TYPE_NAMES[got] ?? got} — check the board's device name`,
      );
    }
  }

  /**
   * Sensor module type read at init(): MODULE_TYPE_MZ (0) = VL53L5CX,
   * MODULE_TYPE_MZEVO (1) = VL53L7CX/CH. null before init().
   */
  get moduleType(): number | null {
    return this.uldDriver !== null ? this.uldDriver.moduleType : null;
  }

  // ── board commands (contract 11 §2) ────────────────────────────────────────

  /**
   * Bridge counters and pin levels (never touches the sensor). Read it before
   * and after a run, not during one: each call takes the bus from the stream
   * and can itself cost a frame.
   */
  getBridgeInfo(): Promise<Vl53l7Info> {
    return this.request<Vl53l7Info>(Vl53l7Cmd.GetInfo, undefined, {
      matcher: DepzDevice.expectReport(Vl53l7Rpt.Vl53Info, unpackVl53l7Info),
    });
  }

  /**
   * Set the sensor-bus SCL frequency; the board snaps to the nearest of 100,
   * 200, 400, 500 … 1000 kHz. Resolves to the value now in effect. Rejects
   * with a busy error mid-transfer — stop ranging first.
   */
  async setI2cSpeedKhz(khz: number): Promise<number> {
    if (!Number.isInteger(khz) || khz < 1 || khz > 0xffff) {
      throw new DepzError("khz must be an integer 1..65535");
    }
    await this.request(Vl53l7Cmd.SetI2cSpeed, packVl53l7SetI2cSpeed(khz), { okCompletes: true });
    return (await this.getBridgeInfo()).i2cKhz;
  }

  /**
   * Drive the sensor's LPn / I2C_RST pins (`PinAction`). LpnOff and SoftCycle
   * drop the sensor's state: run `init()` again afterwards.
   */
  async pinCtrl(action: PinAction): Promise<void> {
    if (!(action in PinAction)) throw new DepzError(`unknown pin action ${String(action)}`);
    await this.request(Vl53l7Cmd.PinCtrl, packPinCtrl(action), { okCompletes: true });
    if (action === PinAction.LpnOff || action === PinAction.SoftCycle) {
      this.rangingFlag = false;
      this.uldDriver = null;
    }
  }

  // ── ULD plugins (contract 11 §4) ───────────────────────────────────────────
  // Power modes, detection thresholds, motion indicator and xtalk calibration
  // are inherited from Vl53l8cx: their DCI sequences are the same on L5/L7
  // (verified against ST ULD 2.0.1 and VL53LMZ 2.0.16). The ULD switches in
  // the L5/L7 xtalk table and calibration output list, and enforces the
  // L5CX/L7CX limits: no DEEP_SLEEP, no threshold auto-stop.

  /**
   * True when the last calibrateXtalk() found nothing to calibrate (ST
   * XTALK_FAILED: "coverglass too good") — the sensor keeps its default xtalk.
   */
  get xtalkCalibrationFailed(): boolean {
    return this.uldDriver !== null && this.uldDriver.xtalkCalibrationFailed;
  }
}

/**
 * VL53L5CX ToF device (8×8 zones, 63° field of view). Same board, blob and
 * API as `Vl53l7cx`; the sensor reports module type MZ.
 */
export class Vl53l5cx extends Vl53l7cx {
  protected override readonly expectedModuleType: number = MODULE_TYPE_MZ;
}

/**
 * VL53L7CH ToF device: the VL53L7CX superset. `init()` downloads the CH
 * firmware blob (VL53LMZ ULD 2.0.16, the same blob as VL53L8CH) and adds
 * Compact-Network-Histogram output (`configureCnh`). CNH frames up to ~7.6 KB
 * stream in chunks (≤ 8192 B total).
 */
export class Vl53l7ch extends Vl53l7cx {
  protected override readonly variantId: Vl53l8Variant = "l7ch";

  /**
   * Arm the CNH histogram block for the next startRanging(). CH only — this
   * method does not exist on Vl53l7cx / Vl53l5cx.
   */
  async configureCnh(config: CnhConfig): Promise<void> {
    this.requireNotRanging();
    await this.uld.dciWriteData(MI_CFG_DEV_IDX, config.pack());
    this.cnhConfig = config;
  }
}
