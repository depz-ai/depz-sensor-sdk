/**
 * VL53L8CH CNH (Compact Network Histogram) — TypeScript port of the parts of
 * vl53lmz_plugin_cnh.c / vl53lmz_plugin_motion_indicator.c (VL53LMZ ULD
 * 2.0.16) needed to configure CNH, compute the on-device buffer size, and
 * decode a captured CNH data block into per-aggregate histograms.
 *
 * 1:1 mirror of the Python reference `depz_sensor_sdk.vl53l8.cnh`.
 *
 * CNH is the reason to run the VL53L8CH firmware instead of plain VL53L8CX:
 * the sensor returns a per-aggregate distance histogram (the "compact network
 * histogram") on top of the normal ranging frame. The MCU streaming buffer is
 * too small to push a full CNH frame, so the host captures it in poll-mode
 * (chunked READ_REG) — see VL53L8CX.startRanging(cnhDataSize).
 *
 * Register/struct layouts are a 1:1 port of the C plugin; do not "simplify"
 * the offset arithmetic. The CNH configuration here fixes the cnh_cfg flags
 * to DISABLE_PING_PONG | DISABLE_VARIANCE (+ ambient / xtalk / zero-invalid /
 * ref-residual), matching Example_12_cnh_data.c — the decode below assumes
 * those.
 */

// ---- fundamental histogram characteristics (plugin_cnh.h) ----
export const CNH_PULSE_WIDTH_BIN = 10;
export const CNH_BIN_WIDTH_MM = 37.5348;

// ---- DCI indexes ----
/** VL53LMZ_MI_CFG_DEV_IDX (cnh_send_config target). */
export const MI_CFG_DEV_IDX = 0xbfac;
/** VL53LMZ_CNH_DATA_IDX (output block index). */
export const CNH_DATA_IDX = 0xc048;

// ---- limits (plugin_cnh.h) ----
export const CNH_MAX_DATA_WORDS = 1540;
export const CNH_MAX_DATA_BYTES = CNH_MAX_DATA_WORDS * 4; // 6160

// ---- cnh_cfg flags (plugin_motion_indicator.h) ----
export const MI_SFE_DISABLE_PING_PONG = 0x01;
export const MI_SFE_DISABLE_VARIANCE = 0x02;
export const MI_SFE_ENABLE_AMBIENT_LEVEL = 0x04;
export const MI_SFE_ENABLE_XTALK_REMOVAL = 0x08;
export const MI_SFE_ZERO_NON_VALID_BINS = 0x10;
export const MI_SFE_STORE_REF_RESIDUAL = 0x20;

export const MI_MAP_ID_LENGTH = 64; // VL53LMZ_RESOLUTION_8X8
export const MI_INDICATOR_LENGTH = 32;

// ---- persistent-data header layout (plugin_cnh.c) ----
const CNH_PER_HEADER_WORDS = 5; // CNH_PER_HEADER_BYTES / 4
const CNH_PER_BUFFER_HEADER_WORDS = 2; // CNH_PER_BUFFER_HEADER_BYTES / 4
const CNH_PER_HEADER_BUFFER_INFO_IDX = 1;
const CNH_PER_HEADER_FLAGS_IDX = 3;
const BUFFER_INFO_WORDS_MASK = 0xffff;
const BUFFER_INFO_FLAGS_SHIFT = 24;
const MI_STATE_PING = 0;

export class CnhConfigError extends Error {
  constructor(message?: string) {
    super(message);
    this.name = "CnhConfigError";
  }
}

/**
 * Mirror of VL53LMZ_Motion_Configuration plus the helpers that fill it.
 * Build with initConfig()/createAggMap(), check size with requiredMemory(),
 * then pack() the 156-byte struct for cnh_send_config.
 */
export class CnhConfig {
  refBinOffset = 0;
  detectionThreshold = 0;
  extraNoiseSigma = 0;
  nullDenClipValue = 0;
  memUpdateMode = 0;
  memUpdateChoice = 0;
  sumSpan = 0;
  featureLength = 0;
  nbOfAggregates = 0;
  nbOfTemporalAccumulations = 1;
  minNbForGlobalDetection = 0;
  globalIndicatorFormat1 = 0;
  globalIndicatorFormat2 = 0;
  cnhCfg = 0;
  cnhFlexShift = 0;
  spare3 = 0;
  mapId: number[] = new Array<number>(MI_MAP_ID_LENGTH).fill(-1);
  indicatorFormat1: number[] = new Array<number>(MI_INDICATOR_LENGTH).fill(0);
  indicatorFormat2: number[] = new Array<number>(MI_INDICATOR_LENGTH).fill(0);

  // ---- vl53lmz_cnh_init_config ----
  /**
   * startBin: first device-histogram bin; numBins: CNH bins;
   * subSample: bins of the device histogram summed per CNH bin.
   */
  initConfig(startBin: number, numBins: number, subSample: number): void {
    this.refBinOffset = startBin * 2048;
    this.detectionThreshold = 0;
    this.extraNoiseSigma = 0;
    this.nullDenClipValue = 0;
    this.memUpdateMode = 0;
    this.memUpdateChoice = 0;
    this.featureLength = numBins & 0xff;
    this.sumSpan = subSample & 0xff;
    this.nbOfTemporalAccumulations = 1;
    this.minNbForGlobalDetection = 0;
    this.globalIndicatorFormat1 = 0;
    this.globalIndicatorFormat2 = 0;
    this.cnhCfg =
      MI_SFE_DISABLE_PING_PONG |
      MI_SFE_DISABLE_VARIANCE |
      MI_SFE_ENABLE_AMBIENT_LEVEL |
      MI_SFE_ENABLE_XTALK_REMOVAL |
      MI_SFE_ZERO_NON_VALID_BINS |
      MI_SFE_STORE_REF_RESIDUAL;
    this.cnhFlexShift = 1;
    this.spare3 = 0;
  }

  // ---- vl53lmz_cnh_create_agg_map ----
  /**
   * Map device zones to CNH aggregates. resolution: 16 (4x4) or 64 (8x8)
   * — must match the value passed to setResolution().
   */
  createAggMap(
    resolution: number,
    startX: number,
    startY: number,
    mergeX: number,
    mergeY: number,
    cols: number,
    rows: number,
  ): void {
    this.mapId = new Array<number>(MI_MAP_ID_LENGTH).fill(-1);
    const zoneRes = resolution === 16 ? 4 : 8;
    if (startX + cols * mergeX > zoneRes || startY + rows * mergeY > zoneRes) {
      throw new CnhConfigError("agg map exceeds zone grid");
    }
    this.nbOfAggregates = cols * rows;
    for (let row = startY; row < startY + rows * mergeY; row++) {
      for (let col = startX; col < startX + cols * mergeX; col++) {
        const i = row * zoneRes + col;
        const aggId =
          Math.floor((row - startY) / mergeY) * cols + Math.floor((col - startX) / mergeX);
        if (aggId >= 0 && aggId < MI_MAP_ID_LENGTH) {
          this.mapId[i] = aggId;
        } else {
          throw new CnhConfigError("agg id out of range");
        }
      }
    }
  }

  // ---- vl53lmz_cnh_calc_required_memory ----
  /**
   * On-device CNH buffer size in bytes for this config. Throws if the
   * config is blank or the size exceeds CNH_MAX_DATA_BYTES.
   */
  requiredMemory(): number {
    if (this.nbOfAggregates === 0) {
      throw new CnhConfigError("agg map not created");
    }
    const size = calcRequiredMemory(this.cnhCfg, this.nbOfAggregates, this.featureLength);
    if (size > CNH_MAX_DATA_BYTES) {
      throw new CnhConfigError(
        `CNH needs ${size} B > max ${CNH_MAX_DATA_BYTES} B — reduce aggregates or bins`,
      );
    }
    return size;
  }

  // ---- vl53lmz_cnh_calc_min_max_distance ----
  /** [min, max] target distance, in mm, fully captured by the histogram. */
  minMaxDistanceMm(): [number, number] {
    const constant = (CNH_PULSE_WIDTH_BIN / 2.0) * CNH_BIN_WIDTH_MM;
    const start = this.refBinOffset / 2048.0;
    const firstCenter = (start + this.sumSpan / 2.0) * CNH_BIN_WIDTH_MM;
    const lastCenter =
      (start + (this.featureLength - 1) * this.sumSpan + this.sumSpan / 2.0) * CNH_BIN_WIDTH_MM;
    return [Math.trunc(firstCenter + constant), Math.trunc(lastCenter - constant)];
  }

  /** Distance (mm) at the centre of CNH histogram bin `binIdx`. */
  binCenterMm(binIdx: number): number {
    const start = this.refBinOffset / 2048.0;
    return (start + binIdx * this.sumSpan + this.sumSpan / 2.0) * CNH_BIN_WIDTH_MM;
  }

  // ---- pack the 156-byte VL53LMZ_Motion_Configuration struct ----
  pack(): Uint8Array {
    const out = new Uint8Array(156);
    const dv = new DataView(out.buffer);
    dv.setUint32(0, this.refBinOffset >>> 0, true); // '<i' with & 0xFFFFFFFF
    dv.setUint32(4, this.detectionThreshold >>> 0, true);
    dv.setUint32(8, this.extraNoiseSigma >>> 0, true);
    dv.setUint32(12, this.nullDenClipValue >>> 0, true);
    const bytes = [
      this.memUpdateMode,
      this.memUpdateChoice,
      this.sumSpan,
      this.featureLength,
      this.nbOfAggregates,
      this.nbOfTemporalAccumulations,
      this.minNbForGlobalDetection,
      this.globalIndicatorFormat1,
      this.globalIndicatorFormat2,
      this.cnhCfg,
      this.cnhFlexShift,
      this.spare3,
    ];
    for (let i = 0; i < 12; i++) out[16 + i] = bytes[i]! & 0xff;
    for (let i = 0; i < MI_MAP_ID_LENGTH; i++) dv.setInt8(28 + i, this.mapId[i]!);
    for (let i = 0; i < MI_INDICATOR_LENGTH; i++) out[92 + i] = this.indicatorFormat1[i]! & 0xff;
    for (let i = 0; i < MI_INDICATOR_LENGTH; i++) out[124 + i] = this.indicatorFormat2[i]! & 0xff;
    return out;
  }
}

/** _cnh_get_pingpong_size_in_word, returned in 32-bit words. */
function pingpongSizeInWords(optionFlags: number, nbAgg: number, featLength: number): number {
  const aggXFeat = nbAgg * featLength;
  let size = CNH_PER_BUFFER_HEADER_WORDS * 4;
  size += aggXFeat * 4; // FEAT_INT  (32b each)
  size += Math.floor((3 + aggXFeat) / 4) * 4; // FEAT_FRAC (8b each, padded)
  size += nbAgg * 4; // AMBIENT_INT
  size += Math.floor((3 + nbAgg) / 4) * 4; // AMBIENT_FRAC
  if ((optionFlags & MI_SFE_DISABLE_VARIANCE) === 0) {
    size += aggXFeat * 4; // VARIANCE_INT
    size += Math.floor((3 + aggXFeat) / 4) * 4; // VARIANCE_FRAC
  }
  return Math.floor(size / 4);
}

/** _cnh_calculate_required_memory, in bytes. */
function calcRequiredMemory(optionFlags: number, nbAgg: number, featLength: number): number {
  let size = pingpongSizeInWords(optionFlags, nbAgg, featLength) * 4;
  if ((optionFlags & MI_SFE_DISABLE_PING_PONG) === 0) {
    size *= 2;
  }
  size += CNH_PER_HEADER_WORDS * 4;
  return size;
}

/**
 * The two cnh_cfg flags that affect the memory calc (ping-pong + variance
 * disabled); used only as `maxBins`' default `optionFlags`. Note initConfig()
 * programs a larger flag superset onto the device — see its body.
 */
const DEFAULT_CNH_CFG = MI_SFE_DISABLE_PING_PONG | MI_SFE_DISABLE_VARIANCE;

/**
 * Largest CNH bins-per-aggregate count whose on-device buffer still fits
 * CNH_MAX_DATA_BYTES for the given aggregate count.
 */
export function maxBins(nbAggregates: number, optionFlags: number = DEFAULT_CNH_CFG): number {
  let feat = 0;
  while (calcRequiredMemory(optionFlags, nbAggregates, feat + 1) <= CNH_MAX_DATA_BYTES) {
    feat += 1;
  }
  return feat;
}

/** Decoded CNH aggregate. */
export interface CnhAggregate {
  /** value = raw / 2**scaler, length == cfg.featureLength. */
  hist: number[];
  histRaw: number[];
  histScaler: number[];
  ambient: number;
}

export interface CnhDecoded {
  /** 11 fractional bits. */
  refResidual: number;
  aggregates: CnhAggregate[];
}

/**
 * Decode a captured CNH data block (`raw` bytes, byte-swapped exactly like
 * the standard ranging blocks) into per-aggregate histograms.
 *
 * Faithful port of vl53lmz_cnh_get_block_addresses /
 * _cnh_get_mem_block_addresses for the fixed cnh_cfg (ping-pong + variance
 * disabled).
 */
export function decode(cfg: CnhConfig, raw: Uint8Array): CnhDecoded {
  const nbAgg = cfg.nbOfAggregates;
  const feat = cfg.featureLength;
  const dv = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
  const nwords = Math.floor(raw.length / 4);
  const words: number[] = [];
  for (let i = 0; i < nwords; i++) words.push(dv.getInt32(i * 4, true)); // signed int32 view

  const refResidual = (words[2]! >>> 0) / 2048.0; // vl53lmz_cnh_get_ref_residual

  const aggregates: CnhAggregate[] = [];
  for (let aggId = 0; aggId < nbAgg; aggId++) {
    aggregates.push(decodeAggregate(words, dv, nbAgg, feat, aggId));
  }
  return { refResidual, aggregates };
}

function decodeAggregate(
  words: number[],
  dv: DataView,
  nbAgg: number,
  feat: number,
  aggId: number,
): CnhAggregate {
  const aggXFeat = nbAgg * feat;
  const aggOff = aggId * feat;

  const state = words[0]!;
  const info = words[CNH_PER_HEADER_BUFFER_INFO_IDX]! >>> 0;
  const ppSize = info & BUFFER_INFO_WORDS_MASK;
  void ((info >>> BUFFER_INFO_FLAGS_SHIFT) & 0xff); // buffer_flags (unused, as in the reference)

  // Select ping or pong buffer exactly as the C code does. With ping-pong
  // disabled the device reports a single buffer and this resolves to ping.
  let localPp = 1;
  if ((words[CNH_PER_HEADER_FLAGS_IDX]! & 0x10) === 0x10) {
    localPp = 1;
  }
  if (state === MI_STATE_PING) {
    localPp = 1 - localPp;
  }

  let base = CNH_PER_HEADER_WORDS;
  if (localPp === 1) {
    base += ppSize;
  }
  // buffer header is 2 words (state, nb_accumulated); data starts after it.
  let blk = (base + CNH_PER_BUFFER_HEADER_WORDS) * 4; // byte offset of p[2]

  // FEAT_INT: int32 per (agg, feat)
  const featInt: number[] = [];
  for (let k = 0; k < feat; k++) featInt.push(dv.getInt32(blk + aggOff * 4 + 4 * k, true));
  blk += aggXFeat * 4;
  // FEAT_FRAC: int8 scaler per (agg, feat)
  const featScaler: number[] = [];
  for (let k = 0; k < feat; k++) featScaler.push(dv.getInt8(blk + aggOff + k));
  blk += Math.floor((3 + aggXFeat) / 4) * 4;
  // AMBIENT_INT: int32 per aggregate (this is the value example12 prints)
  const ambInt = dv.getInt32(blk + aggId * 4, true);
  blk += nbAgg * 4;
  // AMBIENT_FRAC: int8 scaler per aggregate
  const ambScaler = dv.getInt8(blk + aggId);

  const hist = featInt.map((v, k) => v / Math.pow(2.0, featScaler[k]!));
  const ambient = ambInt / Math.pow(2.0, ambScaler);
  return { hist, histRaw: featInt, histScaler: featScaler, ambient };
}
