/**
 * Register behaviour of a VL53L0X die behind the bridge fake
 * (fake-vl53lx-bridge.ts): what the VL53L0X ULD port (uld/l0x.ts) needs to
 * run its init, calibrations and ranging profiles with no hardware. Port of
 * the Python `tests/fake_vl53l0x.py`.
 *
 * The bridge fake is a flat register file; this plugs in as its `die` and
 * keeps one 256-byte bank per page (register 0xFF selects it, as on the die),
 * with the registers that *behave* modelled on top: the soft reset and the
 * NACKs around it, the NVM strobe and data, the single-shot start bit,
 * data-ready and interrupt clear, the result block of each shot and the
 * reference-signal measurement the SPAD management converges on.
 *
 * The defaults are what the live VL53L0X board (TB9BGETA6M) answered in the
 * committed capture vectors/recordings/vl53l0x_uld_long-range_33ms.depzrec:
 * stop variable 0x3C, oscillator word 0x975C, NVM reference-SPAD record "6
 * non-aperture", good SPAD map FB FF FF DF B7 0F, VHV 0x22, phase 0x01,
 * OSC_CALIBRATE_VAL 0x28, one NACK on the model id after a soft reset.
 */

import { DieNack, FakeVl53lxBridge, hex, type DieModel } from "./fake-vl53lx-bridge.js";

export const L0X_BOARD = "DEPZ ToF Sensor VL53L0X USB v2.1 TB9BGETA6M";
export const MODEL_ID = 0xee;
const START_SELECT = 0xb4; // GLOBAL_CONFIG_REF_EN_START_SELECT
const FIRST_APERTURE_SPAD = 192; // SPAD indices from here lie in an aperture quadrant

// Result blocks (12 B at 0x14) of the capture: the first frame of a run, no
// target yet (8191 mm, device status 0x58), and a flat target at 635 mm
// (status 0x5A: internal 11, signal 0x0271 in 9.7, ambient 0x022B, 0x06C1
// effective SPADs in 8.8).
export const FRAME_NO_TARGET = hex("5802ffffffffffffffff1fff");
export const FRAME_635_MM = hex("5a06c1040071022b011b027b");

// NVM words (index -> dword) of the capture plus the identification words the
// driver can ask for with options 2 and 4 (product id, module id / revision,
// UID, the 400 mm factory measurement).
export const NVM_DEFAULTS: Readonly<Record<number, number>> = {
  0x6b: 0x1d80060b, // reference SPADs: (>>8)&0x7F = 6, (>>15)&1 = 0 (non-aperture)
  0x24: 0xfbffffdf, // good SPAD map bytes 0..3
  0x25: 0xb70f0000, // good SPAD map bytes 4..5
  0x02: 0x2a000000, // module id (first byte)
  0x7b: 0x0c345678, // revision (first byte) / part UID upper
  0x7c: 0x9abcdef0, // part UID lower
  0x73: 0x000000a5, // signal rate at 400 mm, high byte
  0x74: 0x3c000000, // ... low byte -> 0xA53C in 9.7
  0x75: 0x00000019, // distance at 400 mm (1/16 mm), high byte
  0x76: 0x40000000, // ... low byte -> 0x1940 = 6464 = 404 mm
};
export const PRODUCT_ID = "VL53L0X-TEST-18CHR";

/**
 * The four NVM words 0x77..0x7A holding an 18-character product id: 7 bits
 * per character, MSB first, two padding bits at the end.
 */
export function nvmProductId(text: string): Record<number, number> {
  if (text.length !== 18) throw new Error("product id must be 18 characters");
  let bits = "";
  for (const ch of text) bits += (ch.charCodeAt(0) & 0x7f).toString(2).padStart(7, "0");
  bits += "00";
  const out: Record<number, number> = {};
  for (let i = 0; i < 4; i++) out[0x77 + i] = parseInt(bits.slice(32 * i, 32 * i + 32), 2);
  return out;
}

function be32(v: number): Uint8Array {
  return Uint8Array.of((v >>> 24) & 0xff, (v >>> 16) & 0xff, (v >>> 8) & 0xff, v & 0xff);
}

export interface FakeVl53l0xDieOptions {
  /** NVM words to override / add. */
  nvm?: Record<number, number>;
}

/** Plug into a FakeVl53lxBridge as its `die`. */
export class FakeVl53l0xDie implements DieModel {
  page = 0;
  readonly banks = new Map<number, Uint8Array>();
  readonly nvm: Record<number, number>;

  // ── knobs ──
  /** Result block per ranging shot; the last one repeats. */
  readonly frames: Uint8Array[] = [];
  frame: Uint8Array = FRAME_635_MM;
  /** 9.7 Mcps each enabled non-aperture reference SPAD adds. */
  refRatePerSpad = 400;
  /** ... each aperture one. */
  refRatePerApertureSpad = 100;
  /** RESULT_INTERRUPT_STATUS polls before a shot is ready. */
  readyAfterPolls = 0;
  /** SYSRANGE_START reads that still show the start bit. */
  startBitReads = 0;
  startBitStuck = false;
  nacksAfterReset = 1;
  /** The soft reset does nothing. */
  resetIgnored = false;
  /** The NVM read strobe never comes. */
  strobeStuck = false;
  /** SYSTEM_INTERRUPT_CLEAR does not clear. */
  interruptSticky = false;
  /** Writes to the SPAD enables are lost. */
  spadMapStuck = false;
  /** RESULT_INTERRUPT_STATUS with nothing pending. */
  interruptIdle = 0x40;

  // ── what happened ──
  /** [SYSRANGE_START value, sequence config] per shot. */
  readonly shots: Array<[number, number]> = [];
  /** Reference rate of each reference shot. */
  readonly refPeaks: number[] = [];

  private inReset = false;
  private nacks = 0;
  private startReads = 0;
  /** Polls until ready; null = nothing pending. */
  private readyIn: number | null = null;

  constructor(bridge: FakeVl53lxBridge, opts: FakeVl53l0xDieOptions = {}) {
    bridge.die = this;
    this.nvm = { ...NVM_DEFAULTS, ...nvmProductId(PRODUCT_ID), ...(opts.nvm ?? {}) };

    const b0 = this.bank(0);
    b0[0xc0] = MODEL_ID;
    b0[0x84] = 0x01; // GPIO_HV_MUX_ACTIVE_HIGH as read on the board
    b0[0xcb] = 0x22; // VHV settings after the reference calibration
    b0[0xee] = 0x01; // phase cal
    b0.set([0x00, 0x28], 0xf8); // OSC_CALIBRATE_VAL
    b0.set(this.frame, 0x14);
    const b1 = this.bank(1);
    b1[0x91] = 0x3c; // the stop variable DataInit reads
    b1.set([0x97, 0x5c], 0x84); // oscillator frequency, FixPoint4.12
    this.bank(6)[0x83] = 0x01;
  }

  bank(page: number): Uint8Array {
    let b = this.banks.get(page);
    if (b === undefined) {
      b = new Uint8Array(256);
      this.banks.set(page, b);
    }
    return b;
  }

  // ── register map ──
  spadEnables(): Uint8Array {
    return this.bank(0).slice(0xb0, 0xb6);
  }

  read(addr: number, n: number): Uint8Array {
    const page = this.page;
    const bank = this.bank(page);
    if (page === 0 && addr === 0xc0) {
      if (this.nacks) {
        this.nacks -= 1;
        throw new DieNack();
      }
      const out = new Uint8Array(n);
      out[0] = this.inReset ? 0x00 : MODEL_ID;
      out.set(bank.subarray(0xc1, 0xc0 + n), 1);
      return out;
    }
    if (page === 7 && addr === 0x83) return Uint8Array.of(this.strobeStuck ? 0x00 : 0x10);
    if (page === 7 && addr === 0x90) return be32(this.nvm[bank[0x94]!] ?? 0).slice(0, n);
    if (page === 0 && addr === 0x00) {
      if (this.startReads || this.startBitStuck) {
        this.startReads = Math.max(0, this.startReads - 1);
        return Uint8Array.of(bank[0x00]! | 0x01);
      }
      return Uint8Array.of(bank[0x00]! & 0xfe);
    }
    if (page === 0 && addr === 0x13) {
      const ready = this.readyIn === 0;
      if (this.readyIn) this.readyIn -= 1;
      return Uint8Array.of(this.interruptIdle | (ready ? 0x04 : 0));
    }
    return bank.slice(addr, addr + n);
  }

  write(addr: number, data: Uint8Array): void {
    if (addr === 0xff) {
      this.page = data[0]!;
      return;
    }
    const page = this.page;
    const bank = this.bank(page);
    if (page === 0 && addr === 0xbf && !this.resetIgnored) {
      if (data[0] === 0x00) {
        this.inReset = true;
      } else {
        this.inReset = false;
        this.nacks = this.nacksAfterReset;
      }
    } else if (page === 0 && addr === 0x00 && data[0]! & 0x01) {
      this.shot(data[0]!);
    } else if (page === 0 && addr === 0x0b && data[0] === 0x01 && !this.interruptSticky) {
      this.readyIn = null;
    } else if (page === 0 && addr >= 0xb0 && addr <= 0xb5 && this.spadMapStuck) {
      return;
    }
    bank.set(data, addr);
  }

  // ── a measurement ──
  private shot(value: number): void {
    const sequence = this.bank(0)[0x01]!;
    this.shots.push([value, sequence]);
    this.startReads = this.startBitReads;
    this.readyIn = this.readyAfterPolls;
    if (sequence === 0xc0) {
      // Reference-signal measurement: the peak reference rate follows the
      // SPAD enables.
      const peak = this.refPeak();
      this.refPeaks.push(peak);
      this.bank(1).set([(peak >> 8) & 0xff, peak & 0xff], 0xb6);
    } else if (sequence !== 0x01 && sequence !== 0x02) {
      // VHV / phase shots range nothing.
      if (this.frames.length > 0) this.frame = this.frames.shift()!;
      this.bank(0).set(this.frame, 0x14);
    }
  }

  private refPeak(): number {
    const enables = this.spadEnables();
    let peak = 0;
    for (let index = 0; index < 8 * enables.length; index++) {
      if ((enables[index >> 3]! >> index % 8) & 1) {
        const aperture = START_SELECT + index >= FIRST_APERTURE_SPAD;
        peak += aperture ? this.refRatePerApertureSpad : this.refRatePerSpad;
      }
    }
    return Math.min(peak, 0xffff);
  }
}

export function l0xRig(
  board = L0X_BOARD,
  dieOpts: FakeVl53l0xDieOptions = {},
): { fake: FakeVl53lxBridge; die: FakeVl53l0xDie } {
  const fake = new FakeVl53lxBridge({ deviceName: board });
  return { fake, die: new FakeVl53l0xDie(fake, dieOpts) };
}
