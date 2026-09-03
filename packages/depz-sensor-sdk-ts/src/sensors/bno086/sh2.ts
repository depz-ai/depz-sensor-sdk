/**
 * SH-2 control-channel builders and parsers (contracts/05_SENSOR_BNO086.md §6).
 *
 * Pure request-builders + response-parsers — no I/O, no timing. The device
 * layer owns the SHTP framing, sequence numbers for the SHTP header, and the
 * waiting/correlation. All multi-byte fields little-endian.
 *
 * Mirrors the Python reference `depz_sensor_sdk.bno086.sh2`.
 */

import { DepzError } from "../../errors.js";

/** SH-2 level failure (bad response status, FRS error, ...). */
export class Sh2Error extends DepzError {}

/** Report IDs on SHTP channel 2 (control). */
export enum ControlReport {
  CommandResponse = 0xf1,
  CommandRequest = 0xf2,
  FrsReadResponse = 0xf3,
  FrsReadRequest = 0xf4,
  FrsWriteResponse = 0xf5,
  FrsWriteData = 0xf6,
  FrsWriteRequest = 0xf7,
  ProductIdResponse = 0xf8,
  ProductIdRequest = 0xf9,
  GetFeatureResponse = 0xfc,
  SetFeatureCommand = 0xfd,
  GetFeatureRequest = 0xfe,
}

/** `command` field of Command Request/Response (0xF2/0xF1). */
export enum Sh2Command {
  Errors = 0x01,
  Counter = 0x02,
  Tare = 0x03,
  Initialize = 0x04,
  SaveDcd = 0x06,
  MeCalibrate = 0x07,
  PeriodicDcdConfig = 0x09,
  GetOscillatorType = 0x0a,
  ClearDcdAndReset = 0x0b,
}

/** Get-Oscillator-Type (command 0x0A) result (r[0]). */
export enum OscillatorType {
  Internal = 0,
  ExtCrystal = 1,
  ExtClock = 2,
}

/** `source` field of an error record (SH-2 §6.4.1). */
export enum ErrorSource {
  MotionEngine = 1,
  MotionHub = 2,
  SensorHub = 3,
  Chip = 4,
  /** Sentinel: end of the error queue. */
  NoMoreErrors = 255,
}

/** Counter subcommands (command 0x02, P0). */
export const COUNTS_GET = 0;
export const COUNTS_CLEAR = 1;

/** Counter command 0x02: get event counts for `sensorId`. */
export function countsGetParams(sensorId: number): Uint8Array {
  return Uint8Array.of(COUNTS_GET, sensorId & 0xff);
}

/** Counter command 0x02: clear event counts for `sensorId`. */
export function countsClearParams(sensorId: number): Uint8Array {
  return Uint8Array.of(COUNTS_CLEAR, sensorId & 0xff);
}

/** Errors command 0x01: return errors of `severity` or greater (0 = all). */
export function errorsParams(severity = 0): Uint8Array {
  return Uint8Array.of(severity & 0xff);
}

/** One error queue entry (command 0x01 response, r[0..5]). */
export interface ErrorRecord {
  severity: number;
  seq: number;
  /** ErrorSource. */
  source: number;
  error: number;
  module: number;
  code: number;
}

export function errorRecordFromResponse(resp: CommandResponse): ErrorRecord {
  const r = resp.r;
  return { severity: r[0]!, seq: r[1]!, source: r[2]!, error: r[3]!, module: r[4]!, code: r[5]! };
}

/** Per-sensor event counts (command 0x02 get response, 2 messages). */
export interface Counts {
  sensorId: number;
  offered: number;
  accepted: number;
  on: number;
  attempted: number;
}

/** Rotation vector used as the tare reference (Tare Now P2). */
export enum TareBasis {
  RotationVector = 0,
  GameRotationVector = 1,
  GeomagneticRotationVector = 2,
  GyroIntegratedRv = 3,
  ArvrStabilizedRv = 4,
  ArvrStabilizedGameRv = 5,
}

export enum TareAxis {
  X = 1,
  Y = 2,
  Z = 4,
  All = 7,
}

function hex2(n: number): string {
  return n.toString(16).toUpperCase().padStart(2, "0");
}

function hex4(n: number): string {
  return n.toString(16).toUpperCase().padStart(4, "0");
}

function view(p: Uint8Array): DataView {
  return new DataView(p.buffer, p.byteOffset, p.byteLength);
}

// ── feature control (0xFD / 0xFE / 0xFC) ─────────────────────────────────────

/**
 * Set Feature Command (0xFD), 17 bytes.
 *
 * `intervalUs` = 0 disables the sensor. `sensitivity` units are
 * sensor-dependent (change sensitivity, u16); `flags` bit meanings per SH-2
 * §6.5.4; `cfgWord` is the sensor-specific configuration u32.
 */
export function buildSetFeature(
  sensorId: number,
  intervalUs: number,
  batchUs = 0,
  sensitivity = 0,
  flags = 0,
  cfgWord = 0,
): Uint8Array {
  const out = new Uint8Array(17);
  const v = new DataView(out.buffer);
  v.setUint8(0, ControlReport.SetFeatureCommand);
  v.setUint8(1, sensorId);
  v.setUint8(2, flags & 0xff);
  v.setUint16(3, sensitivity & 0xffff, true);
  v.setUint32(5, intervalUs >>> 0, true);
  v.setUint32(9, batchUs >>> 0, true);
  v.setUint32(13, cfgWord >>> 0, true);
  return out;
}

/** Get Feature Request (0xFE), 2 bytes. */
export function buildGetFeatureRequest(sensorId: number): Uint8Array {
  return Uint8Array.of(ControlReport.GetFeatureRequest, sensorId);
}

/** Get Feature Response (0xFC), 17 bytes — the rates in effect. */
export interface FeatureResponse {
  sensorId: number;
  flags: number;
  sensitivity: number;
  /** Actual report interval granted by the hub. */
  intervalUs: number;
  batchUs: number;
  cfgWord: number;
}

export function unpackFeatureResponse(payload: Uint8Array): FeatureResponse {
  const v = view(payload);
  const rid = v.getUint8(0);
  if (rid !== ControlReport.GetFeatureResponse) {
    throw new Sh2Error(`not a get-feature response: 0x${hex2(rid)}`);
  }
  return {
    sensorId: v.getUint8(1),
    flags: v.getUint8(2),
    sensitivity: v.getUint16(3, true),
    intervalUs: v.getUint32(5, true),
    batchUs: v.getUint32(9, true),
    cfgWord: v.getUint32(13, true),
  };
}

// ── product ID (0xF9 / 0xF8) ─────────────────────────────────────────────────

export function buildProductIdRequest(): Uint8Array {
  return Uint8Array.of(ControlReport.ProductIdRequest, 0x00);
}

/**
 * Product ID Response (0xF8), 16 bytes. The sensor sends one response per
 * subsystem (typically 2); resetCause per SH-2 §6.4.5.2.
 */
export interface ProductId {
  resetCause: number;
  swVersionMajor: number;
  swVersionMinor: number;
  swPartNumber: number;
  swBuildNumber: number;
  swVersionPatch: number;
  /** "major.minor.patch". */
  version: string;
}

export function unpackProductId(payload: Uint8Array): ProductId {
  const v = view(payload);
  const rid = v.getUint8(0);
  if (rid !== ControlReport.ProductIdResponse) {
    throw new Sh2Error(`not a product-id response: 0x${hex2(rid)}`);
  }
  const major = v.getUint8(2);
  const minor = v.getUint8(3);
  const patch = v.getUint16(12, true);
  return {
    resetCause: v.getUint8(1),
    swVersionMajor: major,
    swVersionMinor: minor,
    swPartNumber: v.getUint32(4, true),
    swBuildNumber: v.getUint32(8, true),
    swVersionPatch: patch,
    version: `${major}.${minor}.${patch}`,
  };
}

// ── command channel (0xF2 / 0xF1) ────────────────────────────────────────────

/** Command Request (0xF2), 12 bytes: id, seq, command, P0..P8. */
export function buildCommandRequest(
  seq: number,
  command: number,
  params: Uint8Array = new Uint8Array(0),
): Uint8Array {
  if (params.length > 9) {
    throw new Error("command request carries at most 9 parameter bytes");
  }
  const out = new Uint8Array(12);
  out[0] = ControlReport.CommandRequest;
  out[1] = seq & 0xff;
  out[2] = command;
  out.set(params, 3);
  return out;
}

/**
 * Command Response (0xF1), 16 bytes.
 *
 * `commandSeq` echoes the request's sequence number (correlate on it plus
 * `command`); `responseSeq` counts multiple responses to one request.
 * R0 is the status word for most commands (0 = success).
 */
export interface CommandResponse {
  seq: number;
  command: number;
  commandSeq: number;
  responseSeq: number;
  /** R0..R10. */
  r: number[];
  /** R0. */
  status: number;
}

export function unpackCommandResponse(payload: Uint8Array): CommandResponse {
  if (payload[0] !== ControlReport.CommandResponse) {
    throw new Sh2Error(`not a command response: 0x${hex2(payload[0]!)}`);
  }
  const r = Array.from(payload.subarray(5, 16));
  return {
    seq: payload[1]!,
    command: payload[2]!,
    commandSeq: payload[3]!,
    responseSeq: payload[4]!,
    r,
    status: r[0]!,
  };
}

/** Tare subcommand 0 — tare `axes` (bitmap X=1,Y=2,Z=4) using `basis`. */
export function tareNowParams(
  axes: number = TareAxis.All,
  basis: number = TareBasis.RotationVector,
): Uint8Array {
  return Uint8Array.of(0x00, axes, basis);
}

/** Tare subcommand 1 — persist current tare into FRS. */
export function persistTareParams(): Uint8Array {
  return Uint8Array.of(0x01);
}

/**
 * Tare subcommand 2 — set reorientation quaternion.
 *
 * P1..P8 are four int16 Q14 components (the 8 available parameter bytes only
 * fit Q14 halves; the *FRS* System Orientation record is the one that stores
 * Q30 words). All-zero clears the reorientation.
 */
export function setReorientationParams(x: number, y: number, z: number, w: number): Uint8Array {
  const q14 = [x, y, z, w].map((c) => Math.round(c * (1 << 14)));
  for (const c of q14) {
    if (c < -32768 || c > 32767) {
      throw new Error("quaternion component out of Q14 int16 range");
    }
  }
  const out = new Uint8Array(9);
  const v = new DataView(out.buffer);
  v.setUint8(0, 0x02);
  q14.forEach((c, i) => v.setInt16(1 + i * 2, c, true));
  return out;
}

/** ME Calibration (command 0x07). subcommand 0 = configure, 1 = get. */
export function meCalibrationParams(
  accel: boolean,
  gyro: boolean,
  mag: boolean,
  planar = false,
  subcommand = 0,
): Uint8Array {
  return Uint8Array.of(Number(accel), Number(gyro), Number(mag), subcommand, Number(planar));
}

/** Subcommand: report current ME calibration config. */
export const ME_CAL_GET = 0x01;

/**
 * Periodic DCD save config (command 0x09). P0: 0 = enable, 1 = disable.
 * No command response is generated.
 */
export function periodicDcdParams(enable: boolean): Uint8Array {
  return Uint8Array.of(enable ? 0x00 : 0x01);
}

// ── FRS (flash record system) ────────────────────────────────────────────────

/** FRS record IDs used by this SDK (SH-2 figure 28; metadata records). */
export enum FrsRecordId {
  StaticCalibrationAgm = 0x7979,
  NominalCalibration = 0x4d4d,
  DynamicCalibration = 0x1f1f,
  MePowerMgmt = 0xd3e2,
  /** Mounting quaternion, 4 × Q30 words. */
  SystemOrientation = 0x2d3e,
  AccelOrientation = 0x2d41,
  GyroscopeOrientation = 0x2d46,
  MagnetometerOrientation = 0x2d4c,
  ArvrStabilizationRv = 0x3e2d,
  ArvrStabilizationGrv = 0x3e2e,
  // Feature configuration records (SH-2 §5.1; write to tune detectors).
  SigMotionDetectConfig = 0xc274,
  ShakeDetectConfig = 0x7d7d,
  StabilityDetectorConfig = 0xed85,
  /** Personal-activity-classifier config. */
  ActivityTrackerConfig = 0xed88,
}

/** Per-sensor metadata FRS record IDs (subset used by getMetadata()). */
export const METADATA_RECORDS: Record<number, number> = {
  0x14: 0xe301, // raw accelerometer
  0x01: 0xe302, // accelerometer
  0x04: 0xe303, // linear acceleration
  0x06: 0xe304, // gravity
  0x15: 0xe305, // raw gyroscope
  0x02: 0xe306, // gyroscope calibrated
  0x07: 0xe307, // gyroscope uncalibrated
  0x16: 0xe308, // raw magnetometer
  0x03: 0xe309, // magnetometer calibrated
  0x0f: 0xe30a, // magnetometer uncalibrated
  0x05: 0xe30b, // rotation vector
  0x08: 0xe30c, // game rotation vector
  0x09: 0xe30d, // geomagnetic rotation vector
  0x10: 0xe313, // tap detector
  0x18: 0xe314, // step detector
  0x11: 0xe315, // step counter
  0x12: 0xe316, // significant motion
  0x13: 0xe317, // stability classifier
  0x19: 0xe318, // shake detector
  0x1e: 0xe31c, // personal activity classifier
  0x28: 0xe322, // ARVR-stabilized RV
  0x29: 0xe323, // ARVR-stabilized game RV
  0x2a: 0xe324, // gyro-integrated RV
};

/** FRS Read Response status (low nibble of the len/status byte). */
export enum FrsStatus {
  NoError = 0,
  UnrecognizedFrsType = 1,
  Busy = 2,
  ReadCompleted = 3,
  OffsetOutOfRange = 4,
  RecordEmpty = 5,
  BlockCompleted = 6,
  BlockAndReadCompleted = 7,
  DeviceError = 8,
}

export enum FrsWriteStatus {
  WordsReceived = 0,
  UnrecognizedFrsType = 1,
  Busy = 2,
  WriteCompleted = 3,
  WriteModeReady = 4,
  WriteFailed = 5,
  NotInWriteMode = 6,
  InvalidLength = 7,
  RecordValid = 8,
  RecordInvalid = 9,
}

/** FRS Read Request (0xF4), 8 bytes. blockWords = 0 reads the record. */
export function buildFrsReadRequest(frsType: number, offsetWords = 0, blockWords = 0): Uint8Array {
  const out = new Uint8Array(8);
  const v = new DataView(out.buffer);
  v.setUint8(0, ControlReport.FrsReadRequest);
  v.setUint8(1, 0);
  v.setUint16(2, offsetWords, true);
  v.setUint16(4, frsType, true);
  v.setUint16(6, blockWords, true);
  return out;
}

/** FRS Read Response (0xF3), 16 bytes; up to two data words per packet. */
export interface FrsReadResponse {
  /** FrsStatus. */
  status: number;
  /** Valid words in data0/data1 (0–2). */
  dataLength: number;
  offsetWords: number;
  data0: number;
  data1: number;
  frsType: number;
}

export function unpackFrsReadResponse(payload: Uint8Array): FrsReadResponse {
  const v = view(payload);
  const rid = v.getUint8(0);
  if (rid !== ControlReport.FrsReadResponse) {
    throw new Sh2Error(`not an FRS read response: 0x${hex2(rid)}`);
  }
  const lenStatus = v.getUint8(1);
  return {
    status: lenStatus & 0x0f,
    dataLength: lenStatus >> 4,
    offsetWords: v.getUint16(2, true),
    data0: v.getUint32(4, true),
    data1: v.getUint32(8, true),
    frsType: v.getUint16(12, true),
  };
}

/** FRS Write Request (0xF7), 6 bytes. lengthWords = 0 erases the record. */
export function buildFrsWriteRequest(frsType: number, lengthWords: number): Uint8Array {
  const out = new Uint8Array(6);
  const v = new DataView(out.buffer);
  v.setUint8(0, ControlReport.FrsWriteRequest);
  v.setUint8(1, 0);
  v.setUint16(2, lengthWords, true);
  v.setUint16(4, frsType, true);
  return out;
}

/** FRS Write Data (0xF6), 12 bytes; 1 or 2 words per packet. */
export function buildFrsWriteData(offsetWords: number, words: number[]): Uint8Array {
  if (words.length < 1 || words.length > 2) {
    throw new Error("FRS write data carries 1 or 2 words");
  }
  const out = new Uint8Array(12);
  const v = new DataView(out.buffer);
  v.setUint8(0, ControlReport.FrsWriteData);
  v.setUint8(1, 0);
  v.setUint16(2, offsetWords, true);
  v.setUint32(4, words[0]! >>> 0, true);
  v.setUint32(8, (words[1] ?? 0) >>> 0, true);
  return out;
}

/** FRS Write Response (0xF5), 4 bytes. */
export interface FrsWriteResponse {
  /** FrsWriteStatus. */
  status: number;
  offsetWords: number;
}

export function unpackFrsWriteResponse(payload: Uint8Array): FrsWriteResponse {
  const v = view(payload);
  const rid = v.getUint8(0);
  if (rid !== ControlReport.FrsWriteResponse) {
    throw new Sh2Error(`not an FRS write response: 0x${hex2(rid)}`);
  }
  return { status: v.getUint8(1), offsetWords: v.getUint16(2, true) };
}

const FRS_READ_ERROR_STATUSES: ReadonlySet<number> = new Set([
  FrsStatus.UnrecognizedFrsType,
  FrsStatus.Busy,
  FrsStatus.OffsetOutOfRange,
  FrsStatus.RecordEmpty,
  FrsStatus.DeviceError,
]);

/**
 * Multi-packet FRS read state machine (pure — feed parsed responses).
 *
 * Usage: send `request()`, then `feed()` every 0xF3 for this record until it
 * returns true; `words` holds the record. Error statuses throw.
 */
export class FrsReadSession {
  readonly frsType: number;
  words: number[] = [];
  done = false;

  constructor(frsType: number) {
    this.frsType = frsType;
  }

  request(): Uint8Array {
    return buildFrsReadRequest(this.frsType);
  }

  feed(resp: FrsReadResponse): boolean {
    if (resp.frsType !== this.frsType) {
      return this.done; // some other record's traffic — not ours
    }
    if (FRS_READ_ERROR_STATUSES.has(resp.status)) {
      throw new Sh2Error(
        `FRS read 0x${hex4(this.frsType)} failed: ${FrsStatus[resp.status] ?? resp.status}`,
      );
    }
    for (const word of [resp.data0, resp.data1].slice(0, resp.dataLength)) {
      this.words.push(word);
    }
    if (resp.status === FrsStatus.ReadCompleted || resp.status === FrsStatus.BlockAndReadCompleted) {
      this.done = true;
    }
    return this.done;
  }
}

const FRS_WRITE_ERROR_STATUSES: ReadonlySet<number> = new Set([
  FrsWriteStatus.UnrecognizedFrsType,
  FrsWriteStatus.Busy,
  FrsWriteStatus.WriteFailed,
  FrsWriteStatus.NotInWriteMode,
  FrsWriteStatus.InvalidLength,
  FrsWriteStatus.RecordInvalid,
]);

/**
 * Multi-packet FRS write state machine (pure).
 *
 * Usage: send `request()`; then for every 0xF5 call `feed()` — it returns
 * the next Write Data payload to send, or null; `done` flips on
 * WriteCompleted. Error statuses throw.
 */
export class FrsWriteSession {
  readonly frsType: number;
  readonly words: number[];
  offset = 0;
  done = false;

  constructor(frsType: number, words: number[]) {
    this.frsType = frsType;
    this.words = words;
  }

  request(): Uint8Array {
    return buildFrsWriteRequest(this.frsType, this.words.length);
  }

  private nextData(): Uint8Array | null {
    if (this.offset >= this.words.length) return null;
    const chunk = this.words.slice(this.offset, this.offset + 2);
    const payload = buildFrsWriteData(this.offset, chunk);
    this.offset += chunk.length;
    return payload;
  }

  feed(resp: FrsWriteResponse): Uint8Array | null {
    if (FRS_WRITE_ERROR_STATUSES.has(resp.status)) {
      throw new Sh2Error(
        `FRS write 0x${hex4(this.frsType)} failed: ${FrsWriteStatus[resp.status] ?? resp.status}`,
      );
    }
    if (resp.status === FrsWriteStatus.WriteCompleted) {
      this.done = true;
      return null;
    }
    if (resp.status === FrsWriteStatus.WriteModeReady || resp.status === FrsWriteStatus.WordsReceived) {
      return this.nextData();
    }
    return null; // RecordValid and friends: informational
  }
}

// ── FRS metadata (best-effort, record version 3/4 layout) ───────────────────

/**
 * Parsed sensor metadata FRS record; `rawWords` is authoritative.
 *
 * Field packing follows the sh2 reference driver (revision-gated fields are
 * 0 when the record predates them).
 */
export interface SensorMetadata {
  meVersion: number;
  mhVersion: number;
  shVersion: number;
  /** Same units & Q point as the sensor's reports. */
  rangeRaw: number;
  resolutionRaw: number;
  revision: number;
  /** mA in Q10. */
  powerMaQ10: number;
  minPeriodUs: number;
  /** Revision >= 4 only. */
  maxPeriodUs: number;
  fifoMax: number;
  fifoReserved: number;
  batchBufferBytes: number;
  qPoint1: number;
  qPoint2: number;
  /** Revision >= 3 only. */
  qPoint3: number;
  rawWords: number[];
}

export function sensorMetadataFromWords(words: number[]): SensorMetadata {
  const w = [...words];
  while (w.length < 10) w.push(0);
  const revision = w[3]! & 0xffff;
  return {
    meVersion: w[0]! & 0xff,
    mhVersion: (w[0]! >>> 8) & 0xff,
    shVersion: (w[0]! >>> 16) & 0xff,
    rangeRaw: w[1]!,
    resolutionRaw: w[2]!,
    revision,
    powerMaQ10: (w[3]! >>> 16) & 0xffff,
    minPeriodUs: w[4]!,
    fifoMax: w[5]! & 0xffff,
    fifoReserved: (w[5]! >>> 16) & 0xffff,
    batchBufferBytes: w[6]! & 0xffff,
    qPoint1: w[7]! & 0xffff,
    qPoint2: (w[7]! >>> 16) & 0xffff,
    qPoint3: revision >= 3 ? (w[8]! >>> 16) & 0xffff : 0,
    maxPeriodUs: revision >= 4 ? w[9]! : 0,
    rawWords: [...words],
  };
}
