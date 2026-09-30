/**
 * Stateless decoders of the blocks the 1D-family bridge streams (contract 12
 * §4) — the "base" every SDK implements, pinned by `vectors/vl53lx.json`.
 * Mirror of the Python `depz_sensor_sdk.vl53lx.decode`.
 *
 * - `decodeDieBlock` — the 17-byte VL53L1-die result block at 0x0089 (L1CX,
 *   L1CB, L3CX, L4CD, L4CX light drivers), fully decoded; the two ULDs that
 *   read it differ only in `DIE_VARIANTS`.
 * - `decodeL0xRaw` — the raw fields of the VL53L0X 12-byte block at 0x14 (the
 *   PAL status and sigma need the device data cached at init).
 * - `decodeHistogramRaw` — the status bytes and 24 photon bins of the
 *   83-byte histogram block at 0x0088 (bins → targets is the full driver).
 */

import { beUint, floorDiv } from "./uld/arith.js";
import { RESULT_BLOCK_LEN as DIE_LEN, STATUS_RTN } from "./uld/vl53l1-die.js";

export const DIE_BLOCK_ADDR = 0x0089;
export const DIE_BLOCK_LEN = DIE_LEN;
export const L0X_BLOCK_ADDR = 0x14;
export const L0X_BLOCK_LEN = 12;

// Histogram block layout (Bare Driver register map, vl53lx_register_map.h).
const RESULT__HISTOGRAM_BIN_0_2 = 0x008e;
const RESULT__HISTOGRAM_BIN_23_0 = 0x00d5;
const PHASECAL_RESULT__REFERENCE_PHASE = 0x00d6;
const PHASECAL_RESULT__VCSEL_START = 0x00d8;
const RESULT__HISTOGRAM_BIN_23_0_MSB = 0x00d9;
const RESULT__HISTOGRAM_BIN_23_0_LSB = 0x00da;
export const HISTOGRAM_BLOCK_ADDR = 0x0088;
export const HISTOGRAM_BLOCK_LEN = RESULT__HISTOGRAM_BIN_23_0_LSB - HISTOGRAM_BLOCK_ADDR + 1;
export const HISTOGRAM_BINS = 24;

/**
 * [signal-rate byte offset, per-SPAD scale K] of the two ULDs that read the
 * die block: "l4" — VL53L4CD ULD (also L3CX ULP, L4CX-as-L4CD); "l1" —
 * VL53L1X ULD (crosstalk-corrected peak signal at 0x0098, K = 25).
 */
export const DIE_VARIANTS: Readonly<Record<string, readonly [number, number]>> = {
  l4: [5, 256],
  l1: [15, 25],
};

export interface DieResult {
  /** ULD status via STATUS_RTN (0 = valid). */
  rangeStatus: number;
  distanceMm: number;
  sigmaMm: number;
  signalRateKcps: number;
  ambientRateKcps: number;
  signalPerSpadKcps: number;
  ambientPerSpadKcps: number;
  numberOfSpad: number;
  streamCount: number;
}

/** The 17-byte die block (0x0089..0x0099) as the named ULD reads it. */
export function decodeDieBlock(raw: Uint8Array, variant: string = "l4"): DieResult {
  if (raw.length < DIE_BLOCK_LEN) {
    throw new RangeError(`die result block needs ${DIE_BLOCK_LEN} bytes, got ${raw.length}`);
  }
  const v = DIE_VARIANTS[variant];
  if (v === undefined) throw new RangeError(`unknown die variant ${variant}`);
  const [signalAt, k] = v;
  let status = raw[0]! & 0x1f;
  if (status < STATUS_RTN.length) status = STATUS_RTN[status]!;
  const rawSpads = beUint(raw, 3, 5); // 8.8
  const signal = beUint(raw, signalAt, signalAt + 2) * 8;
  const ambient = beUint(raw, 7, 9) * 8;
  return {
    rangeStatus: status,
    distanceMm: beUint(raw, 13, 15),
    sigmaMm: floorDiv(beUint(raw, 9, 11), 4),
    signalRateKcps: signal,
    ambientRateKcps: ambient,
    signalPerSpadKcps: rawSpads ? floorDiv(signal * k, rawSpads) : 0,
    ambientPerSpadKcps: rawSpads ? floorDiv(ambient * k, rawSpads) : 0,
    numberOfSpad: floorDiv(rawSpads, 256),
    streamCount: raw[2]!,
  };
}

export interface L0xRaw {
  /** mm (quarter-mm when RangeFractionalEnable, off by default). */
  distanceRaw: number;
  /** Raw byte 0; the PAL status needs the init state. */
  deviceRangeStatus: number;
  /** FixPoint16.16 Mcps (9.7 on the wire << 9). */
  signalRateMcps1616: number;
  ambientRateMcps1616: number;
  /** 8.8. */
  effectiveSpadCount88: number;
}

/** Raw fields of the VL53L0X block at 0x14 (before the PAL status/sigma step). */
export function decodeL0xRaw(raw: Uint8Array): L0xRaw {
  if (raw.length < L0X_BLOCK_LEN) {
    throw new RangeError(`VL53L0X result block needs ${L0X_BLOCK_LEN} bytes, got ${raw.length}`);
  }
  return {
    distanceRaw: (raw[10]! << 8) + raw[11]!,
    deviceRangeStatus: raw[0]!,
    signalRateMcps1616: ((raw[6]! << 8) + raw[7]!) * 512,
    ambientRateMcps1616: ((raw[8]! << 8) + raw[9]!) * 512,
    effectiveSpadCount88: (raw[2]! << 8) + raw[3]!,
  };
}

export interface HistogramRaw {
  interruptStatus: number;
  rangeStatus: number;
  reportStatus: number;
  streamCount: number;
  dssActualEffectiveSpads: number;
  referencePhase: number;
  vcselStart: number;
  /** 24 photon counts. */
  bins: number[];
}

/**
 * The 83-byte histogram block at 0x0088: status bytes and the 24 bins (bin
 * 23's low byte is carried in a separate MSB/LSB pair and patched in first).
 */
export function decodeHistogramRaw(raw: Uint8Array): HistogramRaw {
  if (raw.length < HISTOGRAM_BLOCK_LEN) {
    throw new RangeError(`histogram block needs ${HISTOGRAM_BLOCK_LEN} bytes, got ${raw.length}`);
  }
  const off = HISTOGRAM_BLOCK_ADDR;
  const buf = Uint8Array.from(raw);
  buf[RESULT__HISTOGRAM_BIN_23_0 - off] =
    ((buf[RESULT__HISTOGRAM_BIN_23_0_MSB - off]! << 2) + buf[RESULT__HISTOGRAM_BIN_23_0_LSB - off]!) &
    0xff;
  const base = RESULT__HISTOGRAM_BIN_0_2 - off;
  const bins: number[] = [];
  for (let i = 0; i < HISTOGRAM_BINS; i++) bins.push(beUint(buf, base + 3 * i, base + 3 * i + 3));
  const ref = PHASECAL_RESULT__REFERENCE_PHASE - off;
  return {
    interruptStatus: buf[0]!,
    rangeStatus: buf[1]!,
    reportStatus: buf[2]!,
    streamCount: buf[3]!,
    dssActualEffectiveSpads: beUint(buf, 4, 6),
    referencePhase: beUint(buf, ref, ref + 2),
    vcselStart: buf[PHASECAL_RESULT__VCSEL_START - off]!,
    bins,
  };
}
