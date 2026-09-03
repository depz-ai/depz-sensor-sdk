/**
 * VL53L8CX ULD driver — TypeScript port of the parts of vl53l8cx_api.c
 * (ULD 2.1.0, tools/doc/VL53L8CX_Linux_driver_2.1.0) needed for init (sensor
 * firmware download), 8x8 continuous ranging and result parsing.
 *
 * 1:1 async mirror of the Python reference `depz_sensor_sdk.vl53l8.uld`
 * (itself a proven port of the C driver). The sensor is reached through a
 * `platform` object the caller provides — every method that touches it is
 * async.
 *
 * Register sequences are a 1:1 port of the C code; do not "simplify" them.
 * Configuration: NB_TARGET_PER_ZONE = 1, all output blocks enabled (matches
 * the default platform.h of the C driver).
 */

import type { Vl53l8Assets, Vl53l8Variant } from "./assets/index.js";

/** ULD platform: chunking/bridging is the caller's concern. */
export interface Vl53l8Platform {
  rdMulti(addr: number, size: number): Promise<Uint8Array>;
  wrMulti(addr: number, data: Uint8Array): Promise<void>;
  sleepMs(ms: number): Promise<void>;
}

// Sensor firmware variant. The register/DCI protocol is identical; only the
// downloaded sensor FW (and hence its checksum at 0x812FFC) and the NVM/config
// blobs differ. 'cx' = VL53L8CX ULD 2.1.0; 'ch' = VL53L7CH/VL53L8CH (VL53LMZ)
// ULD 2.0.16 — used to try running CH firmware on VL53L8CX silicon.
export const FW_CHECKSUM: Record<Vl53l8Variant, number> = {
  cx: 0xcadf7caf,
  ch: 0x0c0b6c9e,
};

export const RESOLUTION_4X4 = 16;
export const RESOLUTION_8X8 = 64;

export const RANGING_MODE_CONTINUOUS = 1;
export const RANGING_MODE_AUTONOMOUS = 3;

export const TARGET_ORDER_CLOSEST = 1;
export const TARGET_ORDER_STRONGEST = 2;

// Status codes (subset, matches C defines)
export const STATUS_OK = 0;
export const STATUS_TIMEOUT_ERROR = 1;
export const STATUS_CORRUPTED_FRAME = 2;
export const STATUS_LASER_SAFETY = 3;
export const STATUS_FW_CHECKSUM_FAIL = 5;
export const MCU_ERROR = 66;
export const STATUS_INVALID_PARAM = 127;
export const STATUS_ERROR = 255;

// Block headers for NB_TARGET_PER_ZONE == 1
export const START_BH = 0x0000000d;
export const METADATA_BH = 0x54b400c0;
export const COMMONDATA_BH = 0x54c00040;
export const AMBIENT_RATE_BH = 0x54d00104;
export const SPAD_COUNT_BH = 0x55d00404;
export const NB_TARGET_DETECTED_BH = 0xdb840401;
export const SIGNAL_RATE_BH = 0xdbc40404;
export const RANGE_SIGMA_MM_BH = 0xdec40402;
export const DISTANCE_BH = 0xdf440402;
export const REFLECTANCE_BH = 0xe0440401;
export const TARGET_STATUS_BH = 0xe0840401;
export const MOTION_DETECT_BH = 0xd85808c0;
/** VL53LMZ CNH data output block (VL53L8CH). */
export const CNH_DATA_IDX = 0xc048;

export const METADATA_IDX = 0x54b4;
export const SPAD_COUNT_IDX = 0x55d0;
export const AMBIENT_RATE_IDX = 0x54d0;
export const NB_TARGET_DETECTED_IDX = 0xdb84;
export const SIGNAL_RATE_IDX = 0xdbc4;
export const RANGE_SIGMA_MM_IDX = 0xdec4;
export const DISTANCE_IDX = 0xdf44;
export const REFLECTANCE_EST_PC_IDX = 0xe044;
export const TARGET_STATUS_IDX = 0xe084;
export const MOTION_DETEC_IDX = 0xd858;

export const NVM_DATA_SIZE = 492;
export const OFFSET_BUFFER_SIZE = 488;
export const XTALK_BUFFER_SIZE = 776;

export const DCI_ZONE_CONFIG = 0x5450;
export const DCI_FREQ_HZ = 0x5458;
export const DCI_INT_TIME = 0x545c;
export const DCI_RANGING_MODE = 0xad30;
export const DCI_DSS_CONFIG = 0xad38;
export const DCI_TARGET_ORDER = 0xae64;
export const DCI_SHARPENER = 0xaed8;
export const DCI_SINGLE_RANGE = 0xd964;
export const DCI_OUTPUT_CONFIG = 0xd968;
export const DCI_OUTPUT_ENABLES = 0xd970;
export const DCI_OUTPUT_LIST = 0xd980;
export const DCI_PIPE_CONTROL = 0xdb80;

export const UI_CMD_STATUS = 0x2c00;
export const UI_CMD_START = 0x2c04;
export const UI_CMD_END = 0x2fff;

// ── advanced-feature DCI indices / sizes (ST ULD vl53l8cx_api.h + plugins) ──
export const CONFIGURATION_SIZE = 972;
export const DCI_VHV_CONFIG = 0xad60;
export const DCI_CAL_CFG = 0x5470;
export const DCI_XTALK_CFG = 0xad94;
export const DCI_MOTION_DETECTOR_CFG = 0xbfac;

export const DCI_DET_THRESH_CONFIG = 0x5488;
export const DCI_DET_THRESH_GLOBAL_CONFIG = 0xb6e0;
export const DCI_DET_THRESH_START = 0xb6e8;
export const DCI_DET_THRESH_VALID_STATUS = 0xb9f0;
export const NB_THRESHOLDS = 64;
export const LAST_THRESHOLD = 128;

// Power modes (vl53l8cx_api.h)
export const POWER_MODE_SLEEP = 0;
export const POWER_MODE_WAKEUP = 1;
export const POWER_MODE_DEEP_SLEEP = 2;

// Detection-threshold measurement selectors + scale factors (plugin source):
// get divides, set multiplies by these.
export const DIST_MM = 1;
export const SIGNAL_PER_SPAD_KCPS = 2;
export const RANGE_SIGMA_MM = 4;
export const AMBIENT_PER_SPAD_KCPS = 8;
export const NB_TARGET_DETECTED = 9;
export const TAR_STATUS = 12;
export const NB_SPADS_ENABLED = 13;
export const MOTION_INDICATOR = 19;
const THRESH_SCALE: Record<number, number> = {
  [DIST_MM]: 4,
  [SIGNAL_PER_SPAD_KCPS]: 2048,
  [RANGE_SIGMA_MM]: 128,
  [AMBIENT_PER_SPAD_KCPS]: 2048,
  [NB_SPADS_ENABLED]: 256,
  [MOTION_INDICATOR]: 65535,
};

// Threshold `type` (window) selectors.
export const THRESH_IN_WINDOW = 0;
export const THRESH_OUT_OF_WINDOW = 1;
export const THRESH_LESS_THAN_EQUAL_MIN = 2;
export const THRESH_GREATER_THAN_MAX = 3;
export const THRESH_EQUAL_MIN = 4;
export const THRESH_NOT_EQUAL_MIN = 5;
// Threshold combine operation.
export const THRESH_OP_NONE = 0;
export const THRESH_OP_OR = 0;
export const THRESH_OP_AND = 2;

function fromHexConst(hex: string): Uint8Array {
  const out = new Uint8Array(hex.length / 2);
  for (let i = 0; i < out.length; i++) out[i] = parseInt(hex.slice(i * 2, i * 2 + 2), 16);
  return out;
}

// Const command tables lifted verbatim from vl53l8cx_plugin_xtalk.h (BSD-3).
export const GET_XTALK_CMD = fromHexConst(
  "540000409fd800c09fe401409ff800409ffc0404a0fc0100a10c0100a11c00c0a1280902a2480040a24c0081a2540081a25c0081a2640081a26c0084a28c00820000000f07020044",
);

// VL53L8CX_CALIBRATE_XTALK config table (vl53l8cx_plugin_xtalk.h, BSD-3).
export const CALIBRATE_XTALK = fromHexConst(
  "545000800004080800000404ad3000800301060300000100ad38010001e0014000100010010001000000000154580040041a0200545c01400001005100000fa00fa003e802801f400000050054700080032003200000000854780100011b0021003300000200000104010802548801400000000000000000000000000000000000000800ad48010001f40000030600100808080800000008ad6001000000008000000000201f01f400001d0aad70008008001f4000000001ad78008000a0032000010190ad80004000002800ad8400800000320003200000ad8c00800258ff380000000cad94010000019000fffffc000000040000000100ada400c00480061a0080058000000106adb000c00480061a1900058000000190adbc044000000000000000000012002500000006000000050000000500000006000000040000000f0000005a00000000000000090b0c0b0b030311050101010100000000000d0000ae00010400000004000000080000000a0000000c0000000d0000000e000000080000000800000010000000100000002000000020000000060000050a02000c0800000000ae400040000000ffae44004000100401ae48004000001000ae4c004000000001ae500140000000140400280003206c000000000000000000ae64004000000001aed8010000c805dc00000ccd0104000000012601b5500282a3e8a3b8a438a428a648a448a788a748ac10a79099bc99b49afc9abc0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000b5a002820088030000820082040404080080040109020908040400800401040100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000b5f0004000040000b39c01004000051e021b087c8000120100010800b6c000c0000060000000200000000000aea8004000000405aeac00800100010000020000aeb4004000000000aeb800810000000000000000aec000810000000000000000aec800810801010800000008aed000810108080800000001b5f400800000000000000000b5fc00800000000000000000b604004000000000b608004400000000000000000000000000000000b618004400000000000000000000000000000000b628004400000000000000000000000000000000b638004400000000000000000000000000000000b648010000000000000000000000000000000000b658010000000000000000000000000000000000b6680100000000000000000000000000000000005470008000000000000000020000000f000103d4",
);

export const NB_TARGET_PER_ZONE = 1;

const STATUS_NAMES: Record<number, string> = {
  [STATUS_TIMEOUT_ERROR]: "TIMEOUT",
  [STATUS_CORRUPTED_FRAME]: "CORRUPTED_FRAME",
  [STATUS_LASER_SAFETY]: "LASER_SAFETY",
  [STATUS_FW_CHECKSUM_FAIL]: "FW_CHECKSUM_FAIL",
  [MCU_ERROR]: "MCU_ERROR",
  [STATUS_INVALID_PARAM]: "INVALID_PARAM",
  [STATUS_ERROR]: "ERROR",
};

export class Vl53l8cxError extends Error {
  readonly code: number;
  readonly where: string;

  constructor(code: number, where = "") {
    const name = STATUS_NAMES[code] ?? `0x${code.toString(16)}`;
    super(`VL53L8CX ${name} (code ${code}) ${where}`.trimEnd());
    this.name = "Vl53l8cxError";
    this.code = code;
    this.where = where;
  }
}

/** VL53L8CX_SwapBuffer: byte-reverse every 32-bit word (tail untouched). */
export function swapBuffer(data: Uint8Array): Uint8Array {
  const out = data.slice();
  const n4 = (data.length >>> 2) << 2;
  for (let i = 0; i < n4; i += 4) {
    out[i] = data[i + 3]!;
    out[i + 1] = data[i + 2]!;
    out[i + 2] = data[i + 1]!;
    out[i + 3] = data[i]!;
  }
  return out;
}

/** union Block_header: type[3:0], size[15:4], idx[31:16]. */
export function bhFields(bh: number): [type: number, size: number, idx: number] {
  return [bh & 0xf, (bh >>> 4) & 0xfff, (bh >>> 16) & 0xffff];
}

export function bhSetSize(bh: number, size: number): number {
  return ((bh & ~0xfff0) | ((size & 0xfff) << 4)) >>> 0;
}

function dvOf(buf: Uint8Array): DataView {
  return new DataView(buf.buffer, buf.byteOffset, buf.byteLength);
}

function packU32ArrayLE(values: number[]): Uint8Array {
  const out = new Uint8Array(values.length * 4);
  const dv = new DataView(out.buffer);
  for (let i = 0; i < values.length; i++) dv.setUint32(i * 4, values[i]! >>> 0, true);
  return out;
}

function hex4(n: number): string {
  return n.toString(16).toUpperCase().padStart(4, "0");
}

function hex8(n: number): string {
  return (n >>> 0).toString(16).toUpperCase().padStart(8, "0");
}

/**
 * is_alive register sequence, standalone so the device facade can probe
 * without constructing a full driver (which needs the blob assets).
 * Returns [deviceId, revisionId]; alive when (0xF0, 0x0C).
 */
export async function readDeviceRevisionId(p: Vl53l8Platform): Promise<[number, number]> {
  await p.wrMulti(0x7fff, Uint8Array.of(0x00));
  const deviceId = (await p.rdMulti(0x00, 1))[0]!;
  const revisionId = (await p.rdMulti(0x01, 1))[0]!;
  await p.wrMulti(0x7fff, Uint8Array.of(0x02));
  return [deviceId, revisionId];
}

/** One detection-threshold entry (real units; mirror of the Python dict). */
export interface DetectionThreshold {
  lowThresh: number;
  highThresh: number;
  measurement: number;
  type: number;
  zoneNum: number;
  operation: number;
}

/** Motion-indicator results block (mirror of the Python `motion_indicator` dict). */
export interface MotionResult {
  globalIndicator1: number;
  globalIndicator2: number;
  status: number;
  nbOfDetectedAggregates: number;
  nbOfAggregates: number;
  motion: number[];
}

/** One parsed raw results frame (plain arrays, mirroring the Python dict). */
export interface Vl53l8Results {
  distanceMm: number[];
  targetStatus: number[];
  nbTargetDetected: number[];
  signalPerSpad: number[];
  ambientPerSpad: number[];
  nbSpadsEnabled: number[];
  rangeSigmaMm: number[];
  reflectance: number[];
  siliconTempDegc: number;
  cnhRaw: Uint8Array | null;
  /** Motion-indicator output when the motion detector is configured. */
  motion: MotionResult | null;
}

/**
 * Mirror of VL53L8CX_Motion_Configuration (156 bytes, plugin source).
 * `pack()` reproduces the C struct byte layout (`<i3I12B64b32B32B`).
 */
export class MotionConfig {
  refBinOffset = 0;
  detectionThreshold = 0;
  extraNoiseSigma = 0;
  nullDenClipValue = 0;
  memUpdateMode = 0;
  memUpdateChoice = 0;
  sumSpan = 0;
  featureLength = 0;
  nbOfAggregates = 0;
  nbOfTemporalAccumulations = 0;
  minNbForGlobalDetection = 0;
  globalIndicatorFormat1 = 0;
  globalIndicatorFormat2 = 0;
  spare1 = 0;
  spare2 = 0;
  spare3 = 0;
  mapId: number[] = new Array<number>(64).fill(0);
  indicatorFormat1: number[] = new Array<number>(32).fill(0);
  indicatorFormat2: number[] = new Array<number>(32).fill(0);

  pack(): Uint8Array {
    const out = new Uint8Array(156);
    const dv = new DataView(out.buffer);
    let o = 0;
    dv.setInt32(o, this.refBinOffset | 0, true);
    o += 4;
    dv.setUint32(o, this.detectionThreshold >>> 0, true);
    o += 4;
    dv.setUint32(o, this.extraNoiseSigma >>> 0, true);
    o += 4;
    dv.setUint32(o, this.nullDenClipValue >>> 0, true);
    o += 4;
    const bytes12 = [
      this.memUpdateMode,
      this.memUpdateChoice,
      this.sumSpan,
      this.featureLength,
      this.nbOfAggregates,
      this.nbOfTemporalAccumulations,
      this.minNbForGlobalDetection,
      this.globalIndicatorFormat1,
      this.globalIndicatorFormat2,
      this.spare1,
      this.spare2,
      this.spare3,
    ];
    for (const b of bytes12) dv.setUint8(o++, b & 0xff);
    for (let i = 0; i < 64; i++) dv.setInt8(o++, this.mapId[i]! | 0);
    for (let i = 0; i < 32; i++) dv.setUint8(o++, this.indicatorFormat1[i]! & 0xff);
    for (let i = 0; i < 32; i++) dv.setUint8(o++, this.indicatorFormat2[i]! & 0xff);
    return out;
  }
}

/** Set MotionConfig.map_id for the given resolution (pure — no I/O). */
export function motionConfigSetResolution(cfg: MotionConfig, resolution: number): void {
  if (resolution === RESOLUTION_4X4) {
    for (let i = 0; i < 16; i++) cfg.mapId[i] = i;
    for (let i = 16; i < 64; i++) cfg.mapId[i] = -1;
  } else if (resolution === RESOLUTION_8X8) {
    for (let i = 0; i < 64; i++) cfg.mapId[i] = ((i % 8) >> 1) + 4 * Math.floor(i / 16);
  } else {
    throw new Vl53l8cxError(STATUS_INVALID_PARAM, "motion set_resolution");
  }
}

/**
 * The default motion-indicator configuration used by `motionIndicatorInit`
 * for a resolution (pure — same bytes the sensor is programmed with).
 */
export function defaultMotionConfig(resolution: number): MotionConfig {
  const cfg = new MotionConfig();
  cfg.refBinOffset = 13633;
  cfg.detectionThreshold = 2883584;
  cfg.extraNoiseSigma = 0;
  cfg.nullDenClipValue = 0;
  cfg.memUpdateMode = 6;
  cfg.memUpdateChoice = 2;
  cfg.sumSpan = 4;
  cfg.featureLength = 9;
  cfg.nbOfAggregates = 16;
  cfg.nbOfTemporalAccumulations = 16;
  cfg.minNbForGlobalDetection = 1;
  cfg.globalIndicatorFormat1 = 8;
  cfg.globalIndicatorFormat2 = 0;
  motionConfigSetResolution(cfg, resolution);
  return cfg;
}

/** Xtalk margin (kcps/spad) → raw DCI value (round(kcps * 2048)). */
export function xtalkMarginToRaw(marginKcps: number): number {
  return Math.round(marginKcps * 2048) >>> 0;
}

/**
 * Pack 64 detection thresholds into the DCI_DET_THRESH_START payload plus the
 * 8-byte valid-status block (pure — mirror of set_detection_thresholds). Each
 * threshold's low/high are scaled by its measurement selector.
 */
export function packDetectionThresholds(thresholds: Partial<DetectionThreshold>[]): {
  start: Uint8Array;
  valid: Uint8Array;
} {
  const valid = new Uint8Array(8).fill(0x05);
  const start = new Uint8Array(NB_THRESHOLDS * 12);
  const dv = new DataView(start.buffer);
  for (let k = 0; k < NB_THRESHOLDS; k++) {
    const t = thresholds[k] ?? {};
    const meas = (t.measurement ?? 0) | 0;
    const scale = THRESH_SCALE[meas] ?? 1;
    const low = ((t.lowThresh ?? 0) | 0) * scale;
    const high = ((t.highThresh ?? 0) | 0) * scale;
    const off = k * 12;
    dv.setInt32(off, low | 0, true);
    dv.setInt32(off + 4, high | 0, true);
    dv.setUint8(off + 8, meas & 0xff);
    dv.setUint8(off + 9, (t.type ?? 0) & 0xff);
    dv.setUint8(off + 10, (t.zoneNum ?? 0) & 0xff);
    dv.setUint8(off + 11, (t.operation ?? 0) & 0xff);
  }
  return { start, valid };
}

export class VL53L8CX {
  readonly p: Vl53l8Platform;
  readonly variant: Vl53l8Variant;
  readonly fwChecksum: number;
  readonly firmware: Uint8Array;
  readonly defaultCfg: Uint8Array;
  readonly defaultXtalk: Uint8Array;
  readonly getNvmCmd: Uint8Array;

  offsetData: Uint8Array = new Uint8Array(0);
  xtalkData: Uint8Array = new Uint8Array(0);
  streamcount = 255;
  dataReadSize = 0;
  /** { fw, host } when the FW disagrees with the api.c formula. */
  frameSizeMismatch: { fw: number; host: number } | null = null;
  /** [idx, type, size] of the blocks in the last parsed frame. */
  lastBlocks: Array<[number, number, number]> = [];
  /** Whether the motion-indicator output block is configured. */
  motionPresent = false;

  private readonly outputEnableW3: number;
  private readonly frameTail: number;
  private readonly footerIdOff: number;

  constructor(platform: Vl53l8Platform, assets: Vl53l8Assets, variant: Vl53l8Variant = "cx") {
    this.p = platform;
    this.variant = variant;
    this.fwChecksum = FW_CHECKSUM[variant];
    // start_ranging output config differs slightly by FW variant:
    //   cx (ULD 2.1.0):     OUTPUT_ENABLES word[3]=0xC0000000, frame tail +32
    //   ch (VL53LMZ 2.0.16): OUTPUT_ENABLES word[3]=0,          frame tail +24
    // footerIdOff: where the footer id sits relative to the frame end
    // (cx 2.1.0 = size-12; ch 2.0.16 = size-4).
    if (variant === "ch") {
      this.outputEnableW3 = 0x00000000;
      this.frameTail = 24;
      this.footerIdOff = 4;
    } else {
      this.outputEnableW3 = 0xc0000000;
      this.frameTail = 32;
      this.footerIdOff = 12;
    }
    this.firmware = assets.firmware;
    this.defaultCfg = assets.defaultCfg;
    this.defaultXtalk = assets.defaultXtalk;
    this.getNvmCmd = assets.getNvmCmd;
    if (this.firmware.length !== 0x15000) {
      throw new Error(`firmware.bin (${variant}) has wrong size — rerun the blob extractor`);
    }
  }

  // ---------------- low-level helpers ----------------

  private async rdByte(addr: number): Promise<number> {
    return (await this.p.rdMulti(addr, 1))[0]!;
  }

  private async wrByte(addr: number, value: number): Promise<void> {
    await this.p.wrMulti(addr, Uint8Array.of(value));
  }

  /**
   * _vl53l8cx_poll_for_answer(): poll `size` bytes at `address` until
   * buf[pos] & mask == expected. 10 ms period, 2 s timeout.
   */
  private async pollForAnswer(
    size: number,
    pos: number,
    address: number,
    mask: number,
    expected: number,
    where = "",
  ): Promise<void> {
    let timeout = 0;
    for (;;) {
      const buf = await this.p.rdMulti(address, size);
      await this.p.sleepMs(10);
      if (timeout >= 200) {
        throw new Vl53l8cxError(STATUS_TIMEOUT_ERROR, where);
      }
      if (size >= 4 && buf[2]! >= 0x7f) {
        throw new Vl53l8cxError(MCU_ERROR, where);
      }
      timeout += 1;
      if ((buf[pos]! & mask) === expected) {
        return;
      }
    }
  }

  private async pollForMcuBoot(): Promise<void> {
    let timeout = 0;
    while (timeout < 500) {
      const go2Status0 = await this.rdByte(0x06);
      if (go2Status0 & 0x80) {
        const go2Status1 = await this.rdByte(0x07);
        if (go2Status1 & 0x01) {
          return;
        }
      }
      await this.p.sleepMs(1);
      timeout += 1;
      if (go2Status0 & 0x01) {
        return;
      }
    }
    throw new Vl53l8cxError(STATUS_TIMEOUT_ERROR, "mcu boot");
  }

  // ---------------- DCI access ----------------

  async dciReadData(index: number, dataSize: number): Promise<Uint8Array> {
    const cmd = new Uint8Array(12);
    cmd[0] = (index >> 8) & 0xff;
    cmd[1] = index & 0xff;
    cmd[2] = (dataSize & 0xff0) >> 4;
    cmd[3] = (dataSize & 0xf) << 4;
    cmd[7] = 0x0f;
    cmd[9] = 0x02;
    cmd[11] = 0x08;
    await this.p.wrMulti(UI_CMD_END - 11, cmd);
    await this.pollForAnswer(4, 1, UI_CMD_STATUS, 0xff, 0x03, `dci read 0x${hex4(index)}`);
    let buf: Uint8Array = await this.p.rdMulti(UI_CMD_START, dataSize + 12);
    buf = swapBuffer(buf);
    return buf.slice(4, 4 + dataSize);
  }

  async dciWriteData(index: number, data: Uint8Array): Promise<void> {
    const dataSize = data.length;
    const footer = Uint8Array.of(
      0x00,
      0x00,
      0x00,
      0x0f,
      0x05,
      0x01,
      ((dataSize + 8) >> 8) & 0xff,
      (dataSize + 8) & 0xff,
    );
    const address = UI_CMD_END - (dataSize + 12) + 1;
    const header = Uint8Array.of(
      (index >> 8) & 0xff,
      index & 0xff,
      (dataSize & 0xff0) >> 4,
      (dataSize & 0xf) << 4,
    );
    const swapped = swapBuffer(data);
    const buf = new Uint8Array(4 + dataSize + 8);
    buf.set(header, 0);
    buf.set(swapped, 4);
    buf.set(footer, 4 + dataSize);
    await this.p.wrMulti(address, buf);
    await this.pollForAnswer(4, 1, UI_CMD_STATUS, 0xff, 0x03, `dci write 0x${hex4(index)}`);
  }

  async dciReplaceData(
    index: number,
    dataSize: number,
    newData: Uint8Array,
    newDataPos: number,
  ): Promise<void> {
    const data = await this.dciReadData(index, dataSize);
    data.set(newData, newDataPos);
    await this.dciWriteData(index, data);
  }

  // ---------------- offset / xtalk upload ----------------

  private async sendOffsetData(resolution: number): Promise<void> {
    let buf: Uint8Array = new Uint8Array(OFFSET_BUFFER_SIZE);
    buf.set(this.offsetData.subarray(0, OFFSET_BUFFER_SIZE));

    if (resolution === RESOLUTION_4X4) {
      const dss4x4 = Uint8Array.of(0x0f, 0x04, 0x04, 0x00, 0x08, 0x10, 0x10, 0x07);
      buf.set(dss4x4, 0x10);
      buf = swapBuffer(buf);
      const dv = dvOf(buf);
      const signalGrid: number[] = [];
      const rangeGrid: number[] = [];
      for (let k = 0; k < 64; k++) {
        signalGrid.push(dv.getUint32(0x3c + 4 * k, true));
        rangeGrid.push(dv.getInt16(0x140 + 2 * k, true));
      }
      for (let j = 0; j < 4; j++) {
        for (let i = 0; i < 4; i++) {
          signalGrid[i + 4 * j] = Math.floor(
            (signalGrid[2 * i + 16 * j + 0]! +
              signalGrid[2 * i + 16 * j + 1]! +
              signalGrid[2 * i + 16 * j + 8]! +
              signalGrid[2 * i + 16 * j + 9]!) /
              4,
          );
          rangeGrid[i + 4 * j] = Math.floor(
            (rangeGrid[2 * i + 16 * j + 0]! +
              rangeGrid[2 * i + 16 * j + 1]! +
              rangeGrid[2 * i + 16 * j + 8]! +
              rangeGrid[2 * i + 16 * j + 9]!) /
              4,
          );
        }
      }
      for (let k = 16; k < 64; k++) {
        signalGrid[k] = 0;
        rangeGrid[k] = 0;
      }
      for (let k = 0; k < 64; k++) {
        dv.setUint32(0x3c + 4 * k, signalGrid[k]! >>> 0, true);
        dv.setInt16(0x140 + 2 * k, Math.max(-32768, Math.min(32767, rangeGrid[k]!)), true);
      }
      buf = swapBuffer(buf);
    }

    // Shift the buffer 8 bytes left (drop NVM header) and append the
    // footer at 0x1E0. In C the shift reads past the 488-byte region, but
    // those bytes are then overwritten by the footer, so this is identical.
    const footer = Uint8Array.of(0x00, 0x00, 0x00, 0x0f, 0x03, 0x01, 0x01, 0xe4);
    const out = new Uint8Array(OFFSET_BUFFER_SIZE);
    out.set(buf.subarray(8), 0);
    out.set(footer, OFFSET_BUFFER_SIZE - 8);

    await this.p.wrMulti(0x2e18, out);
    await this.pollForAnswer(4, 1, UI_CMD_STATUS, 0xff, 0x03, "offset data");
  }

  private async sendXtalkData(resolution: number): Promise<void> {
    let buf: Uint8Array = new Uint8Array(XTALK_BUFFER_SIZE);
    buf.set(this.xtalkData.subarray(0, XTALK_BUFFER_SIZE));

    if (resolution === RESOLUTION_4X4) {
      const res4x4 = Uint8Array.of(0x0f, 0x04, 0x04, 0x17, 0x08, 0x10, 0x10, 0x07);
      const dss4x4 = Uint8Array.of(0x00, 0x78, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08);
      const profile4x4 = Uint8Array.of(0xa0, 0xfc, 0x01, 0x00);
      buf.set(res4x4, 0x08);
      buf.set(dss4x4, 0x20);
      buf = swapBuffer(buf);
      const dv = dvOf(buf);
      const signalGrid: number[] = [];
      for (let k = 0; k < 64; k++) {
        signalGrid.push(dv.getUint32(0x34 + 4 * k, true));
      }
      for (let j = 0; j < 4; j++) {
        for (let i = 0; i < 4; i++) {
          signalGrid[i + 4 * j] = Math.floor(
            (signalGrid[2 * i + 16 * j + 0]! +
              signalGrid[2 * i + 16 * j + 1]! +
              signalGrid[2 * i + 16 * j + 8]! +
              signalGrid[2 * i + 16 * j + 9]!) /
              4,
          );
        }
      }
      for (let k = 16; k < 64; k++) {
        signalGrid[k] = 0;
      }
      for (let k = 0; k < 64; k++) {
        dv.setUint32(0x34 + 4 * k, signalGrid[k]! >>> 0, true);
      }
      buf = swapBuffer(buf);
      buf.set(profile4x4, 0x134);
      buf.set(new Uint8Array(4), 0x078);
    }

    await this.p.wrMulti(0x2cf8, buf);
    await this.pollForAnswer(4, 1, UI_CMD_STATUS, 0xff, 0x03, "xtalk data");
  }

  // ---------------- public API ----------------

  /** Returns [deviceId, revisionId]; alive when (0xF0, 0x0C). */
  async isAlive(): Promise<[number, number]> {
    return readDeviceRevisionId(this.p);
  }

  /**
   * vl53l8cx_init(): boot the sensor MCU, download the 84 KB sensor
   * firmware, upload NVM offset / xtalk / default configuration.
   * `progress(text)` is an optional UI callback.
   */
  async init(progress?: (text: string) => void): Promise<void> {
    const note = (text: string): void => {
      if (progress) progress(text);
    };

    const wr = (addr: number, value: number): Promise<void> => this.wrByte(addr, value);
    const rd = (addr: number): Promise<number> => this.rdByte(addr);

    // SW reboot sequence
    note("SW reboot...");
    await wr(0x7fff, 0x00);
    await wr(0x0009, 0x04);
    await wr(0x000f, 0x40);
    await wr(0x000a, 0x03);
    await rd(0x7fff);
    await wr(0x000c, 0x01);

    await wr(0x0101, 0x00);
    await wr(0x0102, 0x00);
    await wr(0x010a, 0x01);
    await wr(0x4002, 0x01);
    await wr(0x4002, 0x00);
    await wr(0x010a, 0x03);
    await wr(0x0103, 0x01);
    await wr(0x000c, 0x00);
    await wr(0x000f, 0x43);
    await this.p.sleepMs(1);

    await wr(0x000f, 0x40);
    await wr(0x000a, 0x01);
    await this.p.sleepMs(100);

    // Wait for sensor booted
    note("Waiting for sensor boot...");
    await wr(0x7fff, 0x00);
    await this.pollForAnswer(1, 0, 0x06, 0xff, 1, "sensor boot");

    await wr(0x000e, 0x01);
    await wr(0x7fff, 0x02);

    // Enable FW access
    await wr(0x7fff, 0x01);
    await wr(0x06, 0x01);
    await this.pollForAnswer(1, 0, 0x21, 0xff, 0x04, "fw access");

    await wr(0x7fff, 0x00);

    // Enable host access to GO1
    await rd(0x7fff);
    await wr(0x0c, 0x01);

    // Power ON status
    await wr(0x7fff, 0x00);
    await wr(0x101, 0x00);
    await wr(0x102, 0x00);
    await wr(0x010a, 0x01);
    await wr(0x4002, 0x01);
    await wr(0x4002, 0x00);
    await wr(0x010a, 0x03);
    await wr(0x103, 0x01);
    await wr(0x400f, 0x00);
    await wr(0x21a, 0x43);
    await wr(0x21a, 0x03);
    await wr(0x21a, 0x01);
    await wr(0x21a, 0x00);
    await wr(0x219, 0x00);
    await wr(0x21b, 0x00);

    // Wake up MCU
    await wr(0x7fff, 0x00);
    await rd(0x7fff);
    await wr(0x7fff, 0x01);

    // Download FW into VL53L8CX
    note("Downloading sensor FW (84 KB)... bank 1/3");
    await wr(0x7fff, 0x09);
    await this.p.wrMulti(0, this.firmware.subarray(0, 0x8000));
    note("Downloading sensor FW... bank 2/3");
    await wr(0x7fff, 0x0a);
    await this.p.wrMulti(0, this.firmware.subarray(0x8000, 0x10000));
    note("Downloading sensor FW... bank 3/3");
    await wr(0x7fff, 0x0b);
    await this.p.wrMulti(0, this.firmware.subarray(0x10000, 0x15000));
    await wr(0x7fff, 0x01);

    // Check if FW correctly downloaded
    await wr(0x7fff, 0x01);
    await wr(0x06, 0x03);

    await this.p.sleepMs(5);
    await wr(0x7fff, 0x00);
    await rd(0x7fff);
    await wr(0x0c, 0x01);

    // Reset MCU and wait boot
    note("Booting sensor MCU...");
    await wr(0x7fff, 0x00);
    await wr(0x114, 0x00);
    await wr(0x115, 0x00);
    await wr(0x116, 0x42);
    await wr(0x117, 0x00);
    await wr(0x0b, 0x00);
    await rd(0x7fff);
    await wr(0x0c, 0x00);
    await wr(0x0b, 0x01);

    await this.pollForMcuBoot();

    await wr(0x7fff, 0x02);

    // Firmware checksum (0x812FFC & 0xFFFF); value depends on the FW variant
    // (see FW_CHECKSUM). cx = ULD 2.1.0 FW; ch = VL53LMZ ULD 2.0.16 FW.
    const crcBuf = swapBuffer(await this.p.rdMulti(0x2ffc, 4));
    const crc = dvOf(crcBuf).getUint32(0, true);
    if (crc !== this.fwChecksum) {
      throw new Vl53l8cxError(
        STATUS_FW_CHECKSUM_FAIL,
        `crc=0x${hex8(crc)} (expected 0x${hex8(this.fwChecksum)} for ${this.variant})`,
      );
    }
    note("Sensor FW checksum OK");

    // Get offset NVM data
    note("Reading NVM offset data...");
    await this.p.wrMulti(0x2fd8, this.getNvmCmd);
    await this.pollForAnswer(4, 0, UI_CMD_STATUS, 0xff, 2, "nvm read");
    const nvm = await this.p.rdMulti(UI_CMD_START, NVM_DATA_SIZE);
    this.offsetData = nvm.slice(0, OFFSET_BUFFER_SIZE);
    await this.sendOffsetData(RESOLUTION_4X4);

    // Default xtalk
    note("Uploading default xtalk...");
    this.xtalkData = this.defaultXtalk;
    await this.sendXtalkData(RESOLUTION_4X4);

    // Default configuration
    note("Uploading default configuration...");
    await this.p.wrMulti(0x2c34, this.defaultCfg);
    await this.pollForAnswer(4, 1, UI_CMD_STATUS, 0xff, 0x03, "default config");

    const pipeCtrl = Uint8Array.of(NB_TARGET_PER_ZONE, 0x00, 0x01, 0x00);
    await this.dciWriteData(DCI_PIPE_CONTROL, pipeCtrl);
    await this.dciWriteData(DCI_SINGLE_RANGE, packU32ArrayLE([0x01]));
    note("Sensor init done");
  }

  async getResolution(): Promise<number> {
    const buf = await this.dciReadData(DCI_ZONE_CONFIG, 8);
    return buf[0]! * buf[1]!;
  }

  async setResolution(resolution: number): Promise<void> {
    if (resolution === RESOLUTION_4X4) {
      let buf: Uint8Array = await this.dciReadData(DCI_DSS_CONFIG, 16);
      buf[0x04] = 64;
      buf[0x06] = 64;
      buf[0x09] = 4;
      await this.dciWriteData(DCI_DSS_CONFIG, buf);
      buf = await this.dciReadData(DCI_ZONE_CONFIG, 8);
      buf[0x00] = 4;
      buf[0x01] = 4;
      buf[0x04] = 8;
      buf[0x05] = 8;
      await this.dciWriteData(DCI_ZONE_CONFIG, buf);
    } else if (resolution === RESOLUTION_8X8) {
      let buf: Uint8Array = await this.dciReadData(DCI_DSS_CONFIG, 16);
      buf[0x04] = 16;
      buf[0x06] = 16;
      buf[0x09] = 1;
      await this.dciWriteData(DCI_DSS_CONFIG, buf);
      buf = await this.dciReadData(DCI_ZONE_CONFIG, 8);
      buf[0x00] = 8;
      buf[0x01] = 8;
      buf[0x04] = 4;
      buf[0x05] = 4;
      await this.dciWriteData(DCI_ZONE_CONFIG, buf);
    } else {
      throw new Vl53l8cxError(STATUS_INVALID_PARAM, "set_resolution");
    }
    await this.sendOffsetData(resolution);
    await this.sendXtalkData(resolution);
  }

  async getRangingFrequencyHz(): Promise<number> {
    return (await this.dciReadData(DCI_FREQ_HZ, 4))[1]!;
  }

  async setRangingFrequencyHz(hz: number): Promise<void> {
    await this.dciReplaceData(DCI_FREQ_HZ, 4, Uint8Array.of(hz), 0x01);
  }

  async setRangingMode(mode: number): Promise<void> {
    const buf = await this.dciReadData(DCI_RANGING_MODE, 8);
    let singleRange: number;
    if (mode === RANGING_MODE_CONTINUOUS) {
      buf[0x01] = 0x1;
      buf[0x03] = 0x3;
      singleRange = 0x00;
    } else if (mode === RANGING_MODE_AUTONOMOUS) {
      buf[0x01] = 0x3;
      buf[0x03] = 0x2;
      singleRange = 0x01;
    } else {
      throw new Vl53l8cxError(STATUS_INVALID_PARAM, "set_ranging_mode");
    }
    await this.dciWriteData(DCI_RANGING_MODE, buf);
    await this.dciWriteData(DCI_SINGLE_RANGE, packU32ArrayLE([singleRange]));
  }

  async getRangingMode(): Promise<number> {
    const buf = await this.dciReadData(DCI_RANGING_MODE, 8);
    return buf[0x01] === 0x1 ? RANGING_MODE_CONTINUOUS : RANGING_MODE_AUTONOMOUS;
  }

  async getIntegrationTimeMs(): Promise<number> {
    const buf = await this.dciReadData(DCI_INT_TIME, 20);
    return Math.floor(dvOf(buf).getUint32(0, true) / 1000);
  }

  /** Integration time 2..1000 ms. No effect in continuous ranging mode. */
  async setIntegrationTimeMs(timeMs: number): Promise<void> {
    if (!(timeMs >= 2 && timeMs <= 1000)) {
      throw new Vl53l8cxError(STATUS_INVALID_PARAM, "set_integration_time_ms");
    }
    const data = new Uint8Array(4);
    new DataView(data.buffer).setUint32(0, timeMs * 1000, true);
    await this.dciReplaceData(DCI_INT_TIME, 20, data, 0x00);
  }

  /**
   * Sharpener 0..99 %. Rounds to nearest: the register holds pct scaled to
   * 0..255, and truncating the way back (as ST's C ULD does) loses a count for
   * 95 of the 100 legal values — set(25) read back as 24. That also made
   * calibrateXtalk's save/restore decay the setting by 1 % on every run
   * (25 -> 24 -> 23 -> ...). The stored byte is unchanged; only this host-side
   * interpretation is. Kept in lockstep with the Python SDK's uld.py.
   */
  async getSharpenerPercent(): Promise<number> {
    return Math.round(((await this.dciReadData(DCI_SHARPENER, 16))[0xd]! * 100) / 255);
  }

  /** Sharpener 0..99 % (0 = disabled). */
  async setSharpenerPercent(pct: number): Promise<void> {
    if (pct >= 100) {
      throw new Vl53l8cxError(STATUS_INVALID_PARAM, "set_sharpener_percent");
    }
    await this.dciReplaceData(DCI_SHARPENER, 16, Uint8Array.of(Math.floor((pct * 255) / 100)), 0xd);
  }

  async getTargetOrder(): Promise<number> {
    return (await this.dciReadData(DCI_TARGET_ORDER, 4))[0]!;
  }

  async setTargetOrder(order: number): Promise<void> {
    if (order !== TARGET_ORDER_CLOSEST && order !== TARGET_ORDER_STRONGEST) {
      throw new Vl53l8cxError(STATUS_INVALID_PARAM, "set_target_order");
    }
    await this.dciReplaceData(DCI_TARGET_ORDER, 4, Uint8Array.of(order), 0x0);
  }

  /**
   * Start ranging. When `cnhDataSize` is given (bytes, from
   * CnhConfig.requiredMemory), a VL53L8CH CNH data block is appended to the
   * output list so each frame also carries the compact-network-histogram
   * buffer. The CNH frame is far larger than the MCU stream cap, so the
   * caller must read it in poll-mode (checkDataReady + getRangingData),
   * not via the MCU INT stream.
   */
  async startRanging(cnhDataSize: number | null = null): Promise<void> {
    const resolution = await this.getResolution();
    // A 0 / non-standard resolution produces a zero-sized output config and
    // faults the sensor MCU (GO2 0x9C) instead of streaming. Refuse early
    // with a clear error; call setResolution() first.
    if (resolution !== RESOLUTION_4X4 && resolution !== RESOLUTION_8X8) {
      throw new Vl53l8cxError(
        STATUS_INVALID_PARAM,
        `bad resolution ${resolution} — call set_resolution first`,
      );
    }
    this.dataReadSize = 0;
    this.streamcount = 255;

    // All outputs enabled (default platform.h): bits 0..11
    const outputBhEnable = [0x00000fff, 0, 0, this.outputEnableW3];
    const output = [
      START_BH,
      METADATA_BH,
      COMMONDATA_BH,
      AMBIENT_RATE_BH,
      SPAD_COUNT_BH,
      NB_TARGET_DETECTED_BH,
      SIGNAL_RATE_BH,
      RANGE_SIGMA_MM_BH,
      DISTANCE_BH,
      REFLECTANCE_BH,
      TARGET_STATUS_BH,
      MOTION_DETECT_BH,
    ];
    if (cnhDataSize !== null) {
      // vl53lmz_add_output_block: CNH block, type 4, size in 32-bit words.
      const cnhBh =
        (((CNH_DATA_IDX << 16) | ((Math.floor(cnhDataSize / 4) & 0xfff) << 4) | 4) >>> 0);
      output.push(cnhBh);
      outputBhEnable[0] = (outputBhEnable[0]! | (1 << (output.length - 1))) >>> 0;
    }

    for (let i = 0; i < output.length; i++) {
      if (
        output[i] === 0 ||
        !((outputBhEnable[Math.floor(i / 32)]! & (1 << i % 32)) >>> 0)
      ) {
        continue;
      }
      const [bhType, bhSizeIn, bhIdx] = bhFields(output[i]!);
      let bhSize = bhSizeIn;
      if (bhType >= 0x1 && bhType < 0x0d) {
        if (bhIdx >= 0x54d0 && bhIdx < 0x54d0 + 960) {
          bhSize = resolution;
        } else if (bhIdx === CNH_DATA_IDX) {
          // keep CNH block size; not zone-scaled
        } else {
          bhSize = resolution * NB_TARGET_PER_ZONE;
        }
        output[i] = bhSetSize(output[i]!, bhSize);
        this.dataReadSize += bhType * bhSize;
      } else {
        this.dataReadSize += bhSize;
      }
      this.dataReadSize += 4;
    }
    this.dataReadSize += this.frameTail;

    await this.dciWriteData(DCI_OUTPUT_LIST, packU32ArrayLE(output));

    const headerConfig = packU32ArrayLE([this.dataReadSize, output.length + 1]);
    await this.dciWriteData(DCI_OUTPUT_CONFIG, headerConfig);

    await this.dciWriteData(DCI_OUTPUT_ENABLES, packU32ArrayLE(outputBhEnable));

    // Start xshut bypass (interrupt mode)
    await this.wrByte(0x7fff, 0x00);
    await this.wrByte(0x09, 0x05);
    await this.wrByte(0x7fff, 0x02);

    // Start ranging session
    await this.p.wrMulti(UI_CMD_END - 3, Uint8Array.of(0x00, 0x03, 0x00, 0x00));
    await this.pollForAnswer(4, 1, UI_CMD_STATUS, 0xff, 0x03, "start ranging");

    // The FW reports the actual frame size it will stream. The C driver
    // asserts equality, but the released FW blobs consistently report
    // 4 bytes less than the api.c formula — trust the FW value.
    let buf: Uint8Array = await this.dciReadData(0x5440, 12);
    const tmp = dvOf(buf).getUint16(0x8, true);
    this.frameSizeMismatch = null;
    if (tmp !== this.dataReadSize) {
      this.frameSizeMismatch = { fw: tmp, host: this.dataReadSize };
      this.dataReadSize = tmp;
    }

    // Laser safety fault check
    buf = await this.dciReadData(0xe0c4, 8);
    if (buf[0x6] !== 0) {
      throw new Vl53l8cxError(STATUS_LASER_SAFETY);
    }
  }

  async stopRanging(): Promise<void> {
    const autoStopFlag = dvOf(await this.p.rdMulti(0x2ffc, 4)).getUint32(0, true);
    if (autoStopFlag !== 0x4ff) {
      await this.wrByte(0x7fff, 0x00);
      // Provoke MCU stop
      await this.wrByte(0x15, 0x16);
      await this.wrByte(0x14, 0x01);
      // Poll for G02 status 0 MCU stop
      let tmp = 0;
      let timeout = 0;
      while (((tmp & 0x80) >> 7) === 0) {
        tmp = await this.rdByte(0x06);
        await this.p.sleepMs(10);
        timeout += 1;
        if (timeout > 500) {
          break;
        }
      }
    }
    // Check GO2 status 1
    let tmp = await this.rdByte(0x06);
    if (tmp & 0x80) {
      tmp = await this.rdByte(0x07);
      if (tmp !== 0x84 && tmp !== 0x85) {
        // non-fatal: C code ORs it into status; we just continue
      }
    }
    // Undo MCU stop
    await this.wrByte(0x7fff, 0x00);
    await this.wrByte(0x14, 0x00);
    await this.wrByte(0x15, 0x00);
    // Stop xshut bypass
    await this.wrByte(0x09, 0x04);
    await this.wrByte(0x7fff, 0x02);
  }

  async checkDataReady(): Promise<boolean> {
    const buf = await this.p.rdMulti(0x0, 4);
    if (
      buf[0] !== this.streamcount &&
      buf[0] !== 255 &&
      buf[1] === 0x05 &&
      (buf[2]! & 0x05) === 0x05 &&
      (buf[3]! & 0x10) === 0x10
    ) {
      this.streamcount = buf[0]!;
      return true;
    }
    if (buf[3]! & 0x80) {
      throw new Vl53l8cxError(buf[2]!, "GO2 error status");
    }
    return false;
  }

  /** Poll-mode read: fetch one results frame from reg 0x00 and parse it. */
  async getRangingData(): Promise<Vl53l8Results> {
    const raw = await this.p.rdMulti(0x0, this.dataReadSize);
    return this.parseFrame(raw);
  }

  /**
   * Parse one raw results frame (dataReadSize bytes read from reg 0x00).
   * Used both by poll-mode getRangingData() and by the host when the MCU
   * pushes frames over the INT-driven stream. Returns per-zone arrays
   * distanceMm, targetStatus, nbTargetDetected, signalPerSpad (kcps/SPAD),
   * ambientPerSpad (kcps/SPAD), nbSpadsEnabled, rangeSigmaMm, reflectance
   * (%), plus the per-frame scalar siliconTempDegc.
   */
  parseFrame(raw: Uint8Array): Vl53l8Results {
    this.streamcount = raw[0]!;
    const buf = swapBuffer(raw);
    const dv = dvOf(buf);

    const results: Vl53l8Results = {
      distanceMm: new Array<number>(RESOLUTION_8X8 * NB_TARGET_PER_ZONE).fill(0),
      targetStatus: new Array<number>(RESOLUTION_8X8 * NB_TARGET_PER_ZONE).fill(0),
      nbTargetDetected: new Array<number>(RESOLUTION_8X8).fill(0),
      signalPerSpad: new Array<number>(RESOLUTION_8X8 * NB_TARGET_PER_ZONE).fill(0),
      ambientPerSpad: new Array<number>(RESOLUTION_8X8).fill(0),
      nbSpadsEnabled: new Array<number>(RESOLUTION_8X8).fill(0),
      rangeSigmaMm: new Array<number>(RESOLUTION_8X8 * NB_TARGET_PER_ZONE).fill(0),
      reflectance: new Array<number>(RESOLUTION_8X8 * NB_TARGET_PER_ZONE).fill(0),
      siliconTempDegc: 0,
      cnhRaw: null,
      motion: null,
    };

    this.lastBlocks = [];
    let i = 16;
    while (i + 4 <= this.dataReadSize) {
      const bh = dv.getUint32(i, true);
      const [bhType, bhSize, bhIdx] = bhFields(bh);
      const msize = bhType > 0x1 && bhType < 0xd ? bhType * bhSize : bhSize;
      // The C driver reads into an oversized temp_buffer, so it walks past
      // the last real block into footer/garbage harmlessly. Our buffer is
      // exact-sized: stop once a block would run past the end (= we have
      // reached the footer; all data blocks precede it). Without this the
      // parser raised struct.error on some frames (seen at 15 Hz).
      if (i + 4 + msize > this.dataReadSize) {
        break;
      }
      this.lastBlocks.push([bhIdx, bhType, bhSize]);

      if (bhIdx === METADATA_IDX) {
        results.siliconTempDegc = dv.getInt8(i + 12);
      } else if (bhIdx === DISTANCE_IDX) {
        const out: number[] = [];
        for (let k = 0; k < Math.floor(msize / 2); k++) out.push(dv.getInt16(i + 4 + 2 * k, true));
        results.distanceMm = out;
      } else if (bhIdx === TARGET_STATUS_IDX) {
        results.targetStatus = Array.from(buf.subarray(i + 4, i + 4 + msize));
      } else if (bhIdx === NB_TARGET_DETECTED_IDX) {
        results.nbTargetDetected = Array.from(buf.subarray(i + 4, i + 4 + msize));
      } else if (bhIdx === SIGNAL_RATE_IDX) {
        const out: number[] = [];
        for (let k = 0; k < Math.floor(msize / 4); k++) out.push(dv.getUint32(i + 4 + 4 * k, true));
        results.signalPerSpad = out;
      } else if (bhIdx === AMBIENT_RATE_IDX) {
        const out: number[] = [];
        for (let k = 0; k < Math.floor(msize / 4); k++) out.push(dv.getUint32(i + 4 + 4 * k, true));
        results.ambientPerSpad = out;
      } else if (bhIdx === SPAD_COUNT_IDX) {
        const out: number[] = [];
        for (let k = 0; k < Math.floor(msize / 4); k++) out.push(dv.getUint32(i + 4 + 4 * k, true));
        results.nbSpadsEnabled = out;
      } else if (bhIdx === RANGE_SIGMA_MM_IDX) {
        const out: number[] = [];
        for (let k = 0; k < Math.floor(msize / 2); k++) out.push(dv.getUint16(i + 4 + 2 * k, true));
        results.rangeSigmaMm = out;
      } else if (bhIdx === REFLECTANCE_EST_PC_IDX) {
        results.reflectance = Array.from(buf.subarray(i + 4, i + 4 + msize));
      } else if (bhIdx === CNH_DATA_IDX) {
        results.cnhRaw = buf.slice(i + 4, i + 4 + msize);
      }

      i += msize + 4;
    }

    // Convert to real format (fixed-point scaling, per ST GetRangingData).
    results.distanceMm = results.distanceMm.map((d) => Math.floor(d / 4));
    results.rangeSigmaMm = results.rangeSigmaMm.map((s) => s / 128.0);

    // No target detected -> status 255. Iterate the zones actually present
    // in this frame (16 for 4x4, 64 for 8x8), not a fixed 64.
    const nzones = results.nbTargetDetected.length;
    for (let z = 0; z < nzones; z++) {
      if (results.nbTargetDetected[z] === 0) {
        for (let t = 0; t < NB_TARGET_PER_ZONE; t++) {
          const idx = NB_TARGET_PER_ZONE * z + t;
          if (idx < results.targetStatus.length) {
            results.targetStatus[idx] = 255;
          }
        }
      }
    }

    // Motion indicator block (optional): global indicator + per-zone map.
    // Present when the motion detector is configured; harmless otherwise.
    if (this.motionPresent) {
      results.motion = this.parseMotion(buf);
    }

    // Header/footer id match check (footer id offset is variant-specific:
    // cx 2.1.0 = size-12, ch 2.0.16 = size-4).
    const foff = this.footerIdOff;
    if (
      buf[0x8] !== buf[this.dataReadSize - foff] ||
      buf[0x9] !== buf[this.dataReadSize - foff + 1]
    ) {
      throw new Vl53l8cxError(STATUS_CORRUPTED_FRAME);
    }

    return results;
  }

  /**
   * Extract the MOTION_INDICATOR results block (index 0xD858) if it was walked
   * in the last frame: global_indicator_1/2 (u32), status/nb_of_detected/
   * nb_of_aggregates/spare (u8), motion[32] (u32 per aggregate).
   */
  private parseMotion(buf: Uint8Array): MotionResult | null {
    const dv = dvOf(buf);
    let hasBlock = false;
    for (const [idx] of this.lastBlocks) {
      if (idx === MOTION_DETEC_IDX) {
        hasBlock = true;
        break;
      }
    }
    if (!hasBlock) return null;
    let i = 16;
    while (i + 4 <= this.dataReadSize) {
      const bh = dv.getUint32(i, true);
      const [t, s, idx] = bhFields(bh);
      const msize = t > 0x1 && t < 0xd ? t * s : s;
      if (i + 4 + msize > this.dataReadSize) break;
      if (idx === MOTION_DETEC_IDX) {
        const motion: number[] = [];
        for (let k = 0; k < 32; k++) motion.push(dv.getUint32(i + 16 + 4 * k, true));
        return {
          globalIndicator1: dv.getUint32(i + 4, true),
          globalIndicator2: dv.getUint32(i + 8, true),
          status: dv.getUint8(i + 12),
          nbOfDetectedAggregates: dv.getUint8(i + 13),
          nbOfAggregates: dv.getUint8(i + 14),
          motion,
        };
      }
      i += msize + 4;
    }
    return null;
  }

  // ---------------- power modes (vl53l8cx_api.c) ----------------

  async getPowerMode(): Promise<number> {
    await this.wrByte(0x7fff, 0x00);
    const tmp = await this.rdByte(0x09);
    let mode: number;
    if (tmp === 0x04) {
      mode = POWER_MODE_WAKEUP;
    } else if (tmp === 0x02) {
      mode = (await this.rdByte(0x000f)) === 0x43 ? POWER_MODE_DEEP_SLEEP : POWER_MODE_SLEEP;
    } else {
      await this.wrByte(0x7fff, 0x02);
      throw new Vl53l8cxError(STATUS_ERROR, "get_power_mode");
    }
    await this.wrByte(0x7fff, 0x02);
    return mode;
  }

  /**
   * WAKEUP (1), SLEEP (0) or DEEP_SLEEP (2). Not allowed while ranging. Wake
   * from DEEP_SLEEP re-runs init() (the FW blob is lost).
   */
  async setPowerMode(powerMode: number): Promise<void> {
    const current = await this.getPowerMode();
    if (powerMode === current) return;
    if (powerMode === POWER_MODE_WAKEUP) {
      await this.wrByte(0x7fff, 0x00);
      await this.wrByte(0x09, 0x04);
      const stored = await this.rdByte(0x000f);
      if (stored === 0x43) await this.wrByte(0x000f, 0x40);
      await this.pollForAnswer(1, 0, 0x06, 0x01, 1, "wakeup");
      await this.wrByte(0x7fff, 0x02);
      if (stored === 0x43) await this.init();
    } else if (powerMode === POWER_MODE_SLEEP) {
      await this.wrByte(0x7fff, 0x00);
      await this.wrByte(0x09, 0x02);
      await this.pollForAnswer(1, 0, 0x06, 0x01, 0, "sleep");
      await this.wrByte(0x7fff, 0x02);
    } else if (powerMode === POWER_MODE_DEEP_SLEEP) {
      await this.wrByte(0x7fff, 0x00);
      await this.wrByte(0x09, 0x02);
      await this.pollForAnswer(1, 0, 0x06, 0x01, 0, "deep sleep");
      await this.wrByte(0x000f, 0x43);
      await this.wrByte(0x7fff, 0x02);
    } else {
      throw new Vl53l8cxError(STATUS_INVALID_PARAM, "set_power_mode");
    }
  }

  // ---------------- xtalk (vl53l8cx_plugin_xtalk.c) ----------------

  /** Xtalk margin in kcps/spad. */
  async getXtalkMargin(): Promise<number> {
    const buf = await this.dciReadData(DCI_XTALK_CFG, 16);
    return dvOf(buf).getUint32(0, true) / 2048.0;
  }

  async setXtalkMargin(marginKcps: number): Promise<void> {
    if (marginKcps > 10000) throw new Vl53l8cxError(STATUS_INVALID_PARAM, "set_xtalk_margin");
    const raw = xtalkMarginToRaw(marginKcps);
    const data = new Uint8Array(4);
    new DataView(data.buffer).setUint32(0, raw >>> 0, true);
    await this.dciReplaceData(DCI_XTALK_CFG, 16, data, 0x00);
  }

  /**
   * Read the live 776-byte xtalk calibration blob back from the sensor FW (as
   * produced by calibrateXtalk). Restores the current resolution afterwards.
   */
  async getCaldataXtalk(): Promise<Uint8Array> {
    const footer = Uint8Array.of(0x00, 0x00, 0x00, 0x0f, 0x00, 0x01, 0x03, 0x04);
    const resolution = await this.getResolution();
    await this.setResolution(RESOLUTION_8X8);
    await this.p.wrMulti(0x2fb8, GET_XTALK_CMD);
    await this.pollForAnswer(4, 1, UI_CMD_STATUS, 0xff, 0x03, "get xtalk");
    const buf = await this.p.rdMulti(UI_CMD_START, XTALK_BUFFER_SIZE + 4);
    const out = new Uint8Array(XTALK_BUFFER_SIZE);
    out.set(buf.subarray(8, XTALK_BUFFER_SIZE), 0);
    out.set(footer, XTALK_BUFFER_SIZE - 8);
    this.xtalkData = out;
    await this.setResolution(resolution);
    return this.xtalkData;
  }

  /**
   * Restore a previously saved 776-byte xtalk calibration blob. The blob is
   * re-uploaded to the FW by the next setResolution()/startRanging().
   */
  async setCaldataXtalk(xtalkData: Uint8Array): Promise<void> {
    if (xtalkData.length !== XTALK_BUFFER_SIZE) {
      throw new Vl53l8cxError(STATUS_INVALID_PARAM, `xtalk blob must be ${XTALK_BUFFER_SIZE} bytes`);
    }
    const resolution = await this.getResolution();
    this.xtalkData = xtalkData.slice();
    await this.setResolution(resolution);
  }

  // ---------------- detection thresholds (plugin) ----------------

  async getDetectionThresholdsEnable(): Promise<number> {
    return (await this.dciReadData(DCI_DET_THRESH_GLOBAL_CONFIG, 8))[1]!;
  }

  async setDetectionThresholdsEnable(enabled: boolean): Promise<void> {
    const grp = Uint8Array.of(0x01, 0x00, 0x01, 0x00);
    let tmp: number;
    if (enabled) {
      grp[1] = 0x01;
      tmp = 0x04;
    } else {
      grp[1] = 0x00;
      tmp = 0x0c;
    }
    await this.dciReplaceData(DCI_DET_THRESH_GLOBAL_CONFIG, 8, grp, 0x00);
    await this.dciReplaceData(DCI_DET_THRESH_CONFIG, 20, Uint8Array.of(tmp), 0x11);
  }

  /** Return the 64 detection thresholds; low/high rescaled to real units. */
  async getDetectionThresholds(): Promise<DetectionThreshold[]> {
    const raw = await this.dciReadData(DCI_DET_THRESH_START, NB_THRESHOLDS * 12);
    const dv = dvOf(raw);
    const out: DetectionThreshold[] = [];
    for (let k = 0; k < NB_THRESHOLDS; k++) {
      const off = k * 12;
      const low = dv.getInt32(off, true);
      const high = dv.getInt32(off + 4, true);
      const meas = dv.getUint8(off + 8);
      const scale = THRESH_SCALE[meas] ?? 1;
      out.push({
        lowThresh: Math.floor(low / scale),
        highThresh: Math.floor(high / scale),
        measurement: meas,
        type: dv.getUint8(off + 9),
        zoneNum: dv.getUint8(off + 10),
        operation: dv.getUint8(off + 11),
      });
    }
    return out;
  }

  /**
   * Program the 64 detection thresholds (missing entries default to zeros).
   */
  async setDetectionThresholds(thresholds: Partial<DetectionThreshold>[]): Promise<void> {
    const { start, valid } = packDetectionThresholds(thresholds);
    await this.dciWriteData(DCI_DET_THRESH_VALID_STATUS, valid);
    await this.dciWriteData(DCI_DET_THRESH_START, start);
  }

  async setDetectionThresholdsAutoStop(autoStop: boolean): Promise<void> {
    await this.dciReplaceData(DCI_PIPE_CONTROL, 4, Uint8Array.of(autoStop ? 1 : 0), 0x03);
  }

  // ---------------- motion indicator (plugin) ----------------

  /**
   * Initialize a motion-indicator configuration (default distance window) and
   * write it to the sensor. Enables the motion output block so subsequent
   * frames carry motion data.
   */
  async motionIndicatorInit(resolution: number): Promise<MotionConfig> {
    const cfg = defaultMotionConfig(resolution);
    await this.dciWriteData(DCI_MOTION_DETECTOR_CFG, cfg.pack());
    this.motionPresent = true;
    return cfg;
  }

  async motionIndicatorSetResolution(cfg: MotionConfig, resolution: number): Promise<void> {
    motionConfigSetResolution(cfg, resolution);
    await this.dciWriteData(DCI_MOTION_DETECTOR_CFG, cfg.pack());
  }

  async motionIndicatorSetDistanceMotion(
    cfg: MotionConfig,
    distanceMinMm: number,
    distanceMaxMm: number,
  ): Promise<void> {
    if (distanceMaxMm - distanceMinMm > 1500 || distanceMinMm < 400 || distanceMaxMm > 4000) {
      throw new Vl53l8cxError(STATUS_INVALID_PARAM, "motion set_distance");
    }
    cfg.refBinOffset = Math.trunc((distanceMinMm / 37.5348 - 4.0) * 2048.5);
    cfg.featureLength = Math.trunc(
      (distanceMaxMm - distanceMinMm) / 10.0 / 15.01392 + 30.02784 / 15.01392 + 0.5,
    );
    await this.dciWriteData(DCI_MOTION_DETECTOR_CFG, cfg.pack());
  }

  // ---------------- xtalk calibration run (plugin) ----------------

  /**
   * Program the sensor output list/config/enables for `resolution` (a copy of
   * the block startRanging() uses inline, kept separate so startRanging stays
   * byte-identical for the replay gate); used by calibrateXtalk.
   */
  private async programOutputConfig(
    resolution: number,
    cnhDataSize: number | null = null,
  ): Promise<number> {
    let dataReadSize = 0;
    const outputBhEnable = [0x00000fff, 0, 0, this.outputEnableW3];
    const output = [
      START_BH,
      METADATA_BH,
      COMMONDATA_BH,
      AMBIENT_RATE_BH,
      SPAD_COUNT_BH,
      NB_TARGET_DETECTED_BH,
      SIGNAL_RATE_BH,
      RANGE_SIGMA_MM_BH,
      DISTANCE_BH,
      REFLECTANCE_BH,
      TARGET_STATUS_BH,
      MOTION_DETECT_BH,
    ];
    if (cnhDataSize !== null) {
      const cnhBh =
        (((CNH_DATA_IDX << 16) | ((Math.floor(cnhDataSize / 4) & 0xfff) << 4) | 4) >>> 0);
      output.push(cnhBh);
      outputBhEnable[0] = (outputBhEnable[0]! | (1 << (output.length - 1))) >>> 0;
    }
    for (let i = 0; i < output.length; i++) {
      if (output[i] === 0 || !((outputBhEnable[Math.floor(i / 32)]! & (1 << i % 32)) >>> 0)) {
        continue;
      }
      const [bhType, bhSizeIn, bhIdx] = bhFields(output[i]!);
      let bhSize = bhSizeIn;
      if (bhType >= 0x1 && bhType < 0x0d) {
        if (bhIdx >= 0x54d0 && bhIdx < 0x54d0 + 960) {
          bhSize = resolution;
        } else if (bhIdx === CNH_DATA_IDX) {
          // keep CNH block size
        } else {
          bhSize = resolution * NB_TARGET_PER_ZONE;
        }
        output[i] = bhSetSize(output[i]!, bhSize);
        dataReadSize += bhType * bhSize;
      } else {
        dataReadSize += bhSize;
      }
      dataReadSize += 4;
    }
    dataReadSize += this.frameTail;
    await this.dciWriteData(DCI_OUTPUT_LIST, packU32ArrayLE(output));
    await this.dciWriteData(DCI_OUTPUT_CONFIG, packU32ArrayLE([dataReadSize, output.length + 1]));
    await this.dciWriteData(DCI_OUTPUT_ENABLES, packU32ArrayLE(outputBhEnable));
    return dataReadSize;
  }

  /**
   * vl53l8cx_calibrate_xtalk: run on-device crosstalk calibration.
   *
   * NOTE: ported from ST ULD source but NOT verified against live hardware in
   * this SDK — the get/set caldata-xtalk buffer path IS the tested save/restore
   * route. Saves & restores resolution/frequency/int-time/sharpener/target-
   * order/xtalk-margin/ranging-mode around the run.
   */
  async calibrateXtalk(
    reflectancePercent: number,
    nbSamples: number,
    distanceMm: number,
  ): Promise<void> {
    if (!(reflectancePercent >= 1 && reflectancePercent <= 99)) {
      throw new Vl53l8cxError(STATUS_INVALID_PARAM, "reflectance 1..99");
    }
    if (!(distanceMm >= 600 && distanceMm <= 3000)) {
      throw new Vl53l8cxError(STATUS_INVALID_PARAM, "distance 600..3000");
    }
    if (!(nbSamples >= 1 && nbSamples <= 16)) {
      throw new Vl53l8cxError(STATUS_INVALID_PARAM, "nb_samples 1..16");
    }
    const footer = Uint8Array.of(0x00, 0x00, 0x00, 0x0f, 0x00, 0x01, 0x03, 0x04);
    const cmd = Uint8Array.of(0x00, 0x03, 0x00, 0x00);
    const saved = {
      resolution: await this.getResolution(),
      frequency: await this.getRangingFrequencyHz(),
      integration: await this.getIntegrationTimeMs(),
      sharpener: await this.getSharpenerPercent(),
      targetOrder: await this.getTargetOrder(),
      xtalkMargin: await this.getXtalkMargin(),
      rangingMode: await this.getRangingMode(),
    };
    await this.setResolution(RESOLUTION_8X8);
    await this.p.wrMulti(0x2c28, CALIBRATE_XTALK);
    await this.pollForAnswer(4, 1, UI_CMD_STATUS, 0xff, 0x03, "calib cmd");
    const refl = reflectancePercent * 16;
    const dist = distanceMm * 4;
    const u16 = (v: number): Uint8Array => {
      const b = new Uint8Array(2);
      new DataView(b.buffer).setUint16(0, v & 0xffff, true);
      return b;
    };
    await this.dciReplaceData(DCI_CAL_CFG, 8, u16(dist), 0x00);
    await this.dciReplaceData(DCI_CAL_CFG, 8, u16(refl), 0x02);
    await this.dciReplaceData(DCI_CAL_CFG, 8, Uint8Array.of(nbSamples), 0x04);
    await this.programOutputConfig(RESOLUTION_8X8);
    await this.p.wrMulti(UI_CMD_END - 3, cmd);
    await this.pollForAnswer(4, 1, UI_CMD_STATUS, 0xff, 0x03, "calib start");
    let timeout = 0;
    for (;;) {
      const buf = await this.p.rdMulti(0x0, 4);
      if (buf[0] !== STATUS_ERROR) {
        if (buf[2]! >= 0x7f && ((buf[3]! & 0x80) >> 7) === 1) {
          this.xtalkData = this.defaultXtalk; // XTALK_FAILED
        }
        break;
      }
      if (timeout >= 400) throw new Vl53l8cxError(STATUS_ERROR, "xtalk calibration");
      await this.p.sleepMs(50);
      timeout += 1;
    }
    await this.p.wrMulti(0x2fb8, GET_XTALK_CMD);
    await this.pollForAnswer(4, 1, UI_CMD_STATUS, 0xff, 0x03, "get xtalk");
    const buf = await this.p.rdMulti(UI_CMD_START, XTALK_BUFFER_SIZE + 4);
    const out = new Uint8Array(XTALK_BUFFER_SIZE);
    out.set(buf.subarray(8, XTALK_BUFFER_SIZE), 0);
    out.set(footer, XTALK_BUFFER_SIZE - 8);
    this.xtalkData = out;
    await this.p.wrMulti(0x2c34, this.defaultCfg);
    await this.pollForAnswer(4, 1, UI_CMD_STATUS, 0xff, 0x03, "restore cfg");
    await this.setResolution(saved.resolution);
    await this.setRangingFrequencyHz(saved.frequency);
    await this.setIntegrationTimeMs(saved.integration);
    await this.setSharpenerPercent(saved.sharpener);
    await this.setTargetOrder(saved.targetOrder);
    await this.setXtalkMargin(saved.xtalkMargin);
    await this.setRangingMode(saved.rangingMode);
  }
}
