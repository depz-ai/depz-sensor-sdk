/**
 * SH-2 input-report catalog and parsers (contracts/05_SENSOR_BNO086.md §5).
 *
 * Raw wire integers are authoritative and always preserved; scaled numbers
 * are derived using the fixed Q points below (value = raw / 2**Q). Report
 * IDs and Q points cross-checked against the BNO08X datasheet (CEVA
 * 1000-3927) and the proven vendor host tool `bno086_tool.py`.
 *
 * Timestamps: channel-3 cargos start with a Base Timestamp Reference (0xFB,
 * i32 base delta in 100 µs ticks, SUBTRACTED from the bridge capture time);
 * each report adds its own 14-bit delay (status bits 7:2 are the upper 6
 * bits, byte 3 the lower 8; 100 µs resolution):
 *
 *     timestampUs = captureUs - baseDelta*100 + delay*100
 *
 * Mirrors the Python reference `depz_sensor_sdk.bno086.reports`.
 */

/** SH-2 input report IDs (datasheet §1.3.5, sh2 reference driver). */
export enum SensorId {
  Accelerometer = 0x01, // calibrated, m/s², Q8
  Gyroscope = 0x02, // calibrated, rad/s, Q9
  Magnetometer = 0x03, // calibrated, µT, Q4
  LinearAcceleration = 0x04, // gravity removed, m/s², Q8
  RotationVector = 0x05, // quaternion Q14 + accuracy rad Q12
  Gravity = 0x06, // m/s², Q8
  UncalibratedGyroscope = 0x07, // rad/s Q9 + bias
  GameRotationVector = 0x08, // quaternion Q14, no accuracy
  GeomagneticRotationVector = 0x09, // quaternion Q14 + accuracy rad Q12
  Pressure = 0x0a, // hPa, Q20 (external sensor; unused on BNO086 boards)
  AmbientLight = 0x0b, // lux, Q8 (external)
  Humidity = 0x0c, // %, Q8 (external)
  Proximity = 0x0d, // cm, Q4 (external)
  Temperature = 0x0e, // °C, Q7 (external)
  UncalibratedMagnetometer = 0x0f, // µT Q4 + hard-iron bias
  TapDetector = 0x10,
  StepCounter = 0x11,
  SignificantMotion = 0x12,
  StabilityClassifier = 0x13,
  RawAccelerometer = 0x14, // ADC counts + sensor-clock timestamp
  RawGyroscope = 0x15,
  RawMagnetometer = 0x16,
  StepDetector = 0x18,
  ShakeDetector = 0x19,
  FlipDetector = 0x1a,
  PickupDetector = 0x1b,
  StabilityDetector = 0x1c,
  PersonalActivityClassifier = 0x1e,
  SleepDetector = 0x1f,
  TiltDetector = 0x20,
  PocketDetector = 0x21,
  CircleDetector = 0x22,
  HeartRateMonitor = 0x23,
  ArvrStabilizedRv = 0x28, // quaternion Q14 + accuracy rad Q12
  ArvrStabilizedGameRv = 0x29, // quaternion Q14, no accuracy
  GyroIntegratedRv = 0x2a, // channel 5 dense: quat Q14 + ang vel Q10
}

// In-cargo control IDs on the input channels
export const BASE_TIMESTAMP_REF = 0xfb; // + i32 base delta (100 µs ticks)
export const TIMESTAMP_REBASE = 0xfa; // + i32 rebase delta (100 µs ticks), batching

/** Q point of the primary fields (value = raw / 2**Q); see module docs. */
export const Q_POINTS: Record<number, number> = {
  [SensorId.Accelerometer]: 8,
  [SensorId.LinearAcceleration]: 8,
  [SensorId.Gravity]: 8,
  [SensorId.Gyroscope]: 9,
  [SensorId.UncalibratedGyroscope]: 9,
  [SensorId.Magnetometer]: 4,
  [SensorId.UncalibratedMagnetometer]: 4,
  [SensorId.RotationVector]: 14,
  [SensorId.GameRotationVector]: 14,
  [SensorId.GeomagneticRotationVector]: 14,
  [SensorId.ArvrStabilizedRv]: 14,
  [SensorId.ArvrStabilizedGameRv]: 14,
  [SensorId.GyroIntegratedRv]: 14,
  [SensorId.Pressure]: 20,
  [SensorId.AmbientLight]: 8,
  [SensorId.Humidity]: 8,
  [SensorId.Proximity]: 4,
  [SensorId.Temperature]: 7,
  [SensorId.RawAccelerometer]: 0,
  [SensorId.RawGyroscope]: 0,
  [SensorId.RawMagnetometer]: 0,
};
export const RV_ACCURACY_Q = 12; // rotation-vector accuracy estimate, radians
export const GYRO_RV_ANGVEL_Q = 10; // gyro-integrated RV angular velocity, rad/s

/**
 * Total report length on the wire, 4-byte SH-2 header included
 * (sh2 reference driver report-length table).
 */
export const REPORT_LENGTHS: Record<number, number> = {
  [SensorId.Accelerometer]: 10,
  [SensorId.Gyroscope]: 10,
  [SensorId.Magnetometer]: 10,
  [SensorId.LinearAcceleration]: 10,
  [SensorId.RotationVector]: 14,
  [SensorId.Gravity]: 10,
  [SensorId.UncalibratedGyroscope]: 16,
  [SensorId.GameRotationVector]: 12,
  [SensorId.GeomagneticRotationVector]: 14,
  [SensorId.Pressure]: 8,
  [SensorId.AmbientLight]: 8,
  [SensorId.Humidity]: 6,
  [SensorId.Proximity]: 6,
  [SensorId.Temperature]: 6,
  [SensorId.UncalibratedMagnetometer]: 16,
  [SensorId.TapDetector]: 5,
  [SensorId.StepCounter]: 12,
  [SensorId.SignificantMotion]: 6,
  [SensorId.StabilityClassifier]: 6,
  [SensorId.RawAccelerometer]: 16,
  [SensorId.RawGyroscope]: 16,
  [SensorId.RawMagnetometer]: 16,
  [SensorId.StepDetector]: 8,
  [SensorId.ShakeDetector]: 6,
  [SensorId.FlipDetector]: 6,
  [SensorId.PickupDetector]: 8,
  [SensorId.StabilityDetector]: 6,
  [SensorId.PersonalActivityClassifier]: 16,
  [SensorId.SleepDetector]: 6,
  [SensorId.TiltDetector]: 6,
  [SensorId.PocketDetector]: 6,
  [SensorId.CircleDetector]: 6,
  [SensorId.HeartRateMonitor]: 6,
  [SensorId.ArvrStabilizedRv]: 14,
  [SensorId.ArvrStabilizedGameRv]: 12,
  [SensorId.GyroIntegratedRv]: 14,
};

export const STABILITY_NAMES: Record<number, string> = {
  0: "unknown",
  1: "on_table",
  2: "stationary",
  3: "stable",
  4: "motion",
};
export const ACTIVITY_NAMES: Record<number, string> = {
  0: "unknown",
  1: "in_vehicle",
  2: "on_bicycle",
  3: "on_foot",
  4: "still",
  5: "tilting",
  6: "walking",
  7: "running",
  8: "on_stairs",
};

function q(raw: number, qPoint: number): number {
  return raw / (1 << qPoint);
}

// ── report shapes ─────────────────────────────────────────────────────────────

/**
 * Base for anything the sensor pushes; `timestampUs` is absolute in the MCU
 * clock domain (bridge capture time corrected by timebase + delay).
 */
export interface ReportBase {
  sensorId: number;
  timestampUs: bigint;
}

/** Channel-3/4 report with the common SH-2 header fields. */
export interface InputReportBase extends ReportBase {
  /** 8-bit rolling sample counter (drop detection). */
  seq: number;
  /** Status bits 1:0 — 0 unreliable … 3 high. */
  accuracy: number;
  /** Report delay already folded into timestampUs. */
  delayUs: number;
}

/** 0x01 accelerometer / 0x04 linear acceleration / 0x06 gravity (Q8). */
export interface Acceleration extends InputReportBase {
  type: "Acceleration";
  xRaw: number;
  yRaw: number;
  zRaw: number;
  /** m/s². */
  x: number;
  y: number;
  z: number;
}

/** 0x02 calibrated gyroscope (Q9). */
export interface Gyroscope extends InputReportBase {
  type: "Gyroscope";
  xRaw: number;
  yRaw: number;
  zRaw: number;
  /** rad/s. */
  x: number;
  y: number;
  z: number;
}

/** 0x03 calibrated magnetic field (Q4). */
export interface Magnetometer extends InputReportBase {
  type: "Magnetometer";
  xRaw: number;
  yRaw: number;
  zRaw: number;
  /** µT. */
  x: number;
  y: number;
  z: number;
}

/** 0x07 uncalibrated gyroscope + bias estimate (all Q9, rad/s). */
export interface UncalibratedGyroscope extends InputReportBase {
  type: "UncalibratedGyroscope";
  xRaw: number;
  yRaw: number;
  zRaw: number;
  biasXRaw: number;
  biasYRaw: number;
  biasZRaw: number;
  x: number;
  y: number;
  z: number;
  /** rad/s. */
  bias: [number, number, number];
}

/** 0x0F uncalibrated magnetic field + hard-iron bias (all Q4, µT). */
export interface UncalibratedMagnetometer extends InputReportBase {
  type: "UncalibratedMagnetometer";
  xRaw: number;
  yRaw: number;
  zRaw: number;
  biasXRaw: number;
  biasYRaw: number;
  biasZRaw: number;
  x: number;
  y: number;
  z: number;
  /** µT. */
  bias: [number, number, number];
}

/**
 * Quaternion reports 0x05/0x08/0x09/0x28/0x29 (unit quaternion, Q14).
 * `accuracyRaw` (Q12, radians) is present only for 0x05/0x09/0x28.
 */
export interface RotationVector extends InputReportBase {
  type: "RotationVector";
  iRaw: number;
  jRaw: number;
  kRaw: number;
  realRaw: number;
  accuracyRaw: number | null;
  i: number;
  j: number;
  k: number;
  real: number;
  /** Estimated heading accuracy in radians (null for game variants). */
  accuracyRad: number | null;
}

/**
 * 0x2A gyro-integrated rotation vector (channel 5, dense — no SH-2 header).
 * Quaternion Q14, angular velocity Q10 rad/s.
 */
export interface GyroIntegratedRV extends ReportBase {
  type: "GyroIntegratedRV";
  iRaw: number;
  jRaw: number;
  kRaw: number;
  realRaw: number;
  vxRaw: number;
  vyRaw: number;
  vzRaw: number;
  i: number;
  j: number;
  k: number;
  real: number;
  /** rad/s. */
  angularVelocity: [number, number, number];
}

/** Environment reports 0x0A–0x0E: single value, Q from Q_POINTS. */
export interface ScalarReport extends InputReportBase {
  type: "ScalarReport";
  valueRaw: number;
  value: number;
}

/** 0x10 tap detector; `flags` bit 6 = double tap, bits 0–5 axis/sign. */
export interface TapDetector extends InputReportBase {
  type: "TapDetector";
  flags: number;
  doubleTap: boolean;
}

/**
 * 0x11 step counter. NOTE: latency u32 µs at bytes 4–7, steps u16 at bytes
 * 8–9 per SH-2; the vendor tool's `latency(2)+steps(2)` comment is wrong.
 */
export interface StepCounter extends InputReportBase {
  type: "StepCounter";
  latencyUs: number;
  steps: number;
}

/** 0x18 step detector; latency from step event to report, µs. */
export interface StepDetector extends InputReportBase {
  type: "StepDetector";
  latencyUs: number;
}

/** 0x12 significant motion (1 = motion detected; sensor auto-disables). */
export interface SignificantMotion extends InputReportBase {
  type: "SignificantMotion";
  motion: number;
}

/** 0x13 stability classification (see STABILITY_NAMES). */
export interface StabilityClassifier extends InputReportBase {
  type: "StabilityClassifier";
  classification: number;
  name: string;
}

/** 0x19 shake detector; bits 0/1/2 = X/Y/Z shake. */
export interface ShakeDetector extends InputReportBase {
  type: "ShakeDetector";
  flags: number;
}

/**
 * Simple u16-value detectors: 0x1A flip, 0x1B pickup, 0x1C stability
 * detector, 0x1F sleep, 0x20 tilt, 0x21 pocket, 0x22 circle, 0x23 HR.
 */
export interface GenericEvent extends InputReportBase {
  type: "GenericEvent";
  valueRaw: number;
}

/**
 * 0x1E personal activity classifier (see ACTIVITY_NAMES).
 * `confidences` are 0–100 per state, states 0–9 of the current page.
 */
export interface PersonalActivityClassifier extends InputReportBase {
  type: "PersonalActivityClassifier";
  pageNumber: number;
  endOfSequence: boolean;
  mostLikelyState: number;
  confidences: number[];
  mostLikelyName: string;
}

/**
 * 0x14/0x15/0x16 raw ADC samples + sensor-clock timestamp (u32 µs).
 * `temperatureRaw` is populated only for the raw gyroscope.
 */
export interface RawSensor extends InputReportBase {
  type: "RawSensor";
  xRaw: number;
  yRaw: number;
  zRaw: number;
  sensorTimestampUs: number;
  temperatureRaw: number;
}

/**
 * Unrecognized report ID: raw bytes from the ID to end of cargo (the length
 * is unknowable, so parsing stops here).
 */
export interface UnknownReport extends ReportBase {
  type: "UnknownReport";
  data: Uint8Array;
}

/** Channel-3/4 typed reports (share the SH-2 input header fields). */
export type InputReport =
  | Acceleration
  | Gyroscope
  | Magnetometer
  | UncalibratedGyroscope
  | UncalibratedMagnetometer
  | RotationVector
  | ScalarReport
  | TapDetector
  | StepCounter
  | StepDetector
  | SignificantMotion
  | StabilityClassifier
  | ShakeDetector
  | GenericEvent
  | PersonalActivityClassifier
  | RawSensor;

/** Anything the sensor pushes. */
export type Report = InputReport | GyroIntegratedRV | UnknownReport;

// ── parsers ───────────────────────────────────────────────────────────────────

function view(p: Uint8Array): DataView {
  return new DataView(p.buffer, p.byteOffset, p.byteLength);
}

/**
 * Parse a channel-3/4 cargo into typed reports.
 *
 * `captureTimestampUs` is the bridge RPT_DATA capture time (MCU uptime).
 * Handles 0xFB base timestamp references and 0xFA rebases; every report's
 * timestamp is `base + delay` where base = capture − baseDelta·100 µs.
 */
export function parseInputCargo(payload: Uint8Array, captureTimestampUs: bigint): Report[] {
  const out: Report[] = [];
  const v = view(payload);
  let baseUs = captureTimestampUs;
  let pos = 0;
  const n = payload.length;
  while (pos < n) {
    const rid = payload[pos]!;
    if (rid === BASE_TIMESTAMP_REF && pos + 5 <= n) {
      const delta = v.getInt32(pos + 1, true);
      baseUs = captureTimestampUs - BigInt(delta) * 100n;
      pos += 5;
      continue;
    }
    if (rid === TIMESTAMP_REBASE && pos + 5 <= n) {
      const delta = v.getInt32(pos + 1, true);
      baseUs += BigInt(delta) * 100n;
      pos += 5;
      continue;
    }
    const length = REPORT_LENGTHS[rid];
    if (length === undefined || pos + length > n) {
      out.push({
        type: "UnknownReport",
        sensorId: rid,
        timestampUs: baseUs,
        data: payload.slice(pos),
      });
      break;
    }
    const rep = payload.subarray(pos, pos + length);
    const seq = rep[1]!;
    const status = rep[2]!;
    const delayLsb = rep[3]!;
    const accuracy = status & 0x03;
    const delayUs = (((status >> 2) << 8) | delayLsb) * 100;
    const ts = baseUs + BigInt(delayUs);
    out.push(decodeReport(rid, rep, ts, seq, accuracy, delayUs));
    pos += length;
  }
  return out;
}

function decodeReport(
  rid: number,
  rep: Uint8Array,
  ts: bigint,
  seq: number,
  accuracy: number,
  delayUs: number,
): Report {
  const v = view(rep);
  const head = { sensorId: rid, timestampUs: ts, seq, accuracy, delayUs };
  const qp = Q_POINTS[rid] ?? 0;
  switch (rid) {
    case SensorId.Accelerometer:
    case SensorId.LinearAcceleration:
    case SensorId.Gravity: {
      const [xRaw, yRaw, zRaw] = [v.getInt16(4, true), v.getInt16(6, true), v.getInt16(8, true)];
      return { type: "Acceleration", ...head, xRaw, yRaw, zRaw, x: q(xRaw, qp), y: q(yRaw, qp), z: q(zRaw, qp) };
    }
    case SensorId.Gyroscope: {
      const [xRaw, yRaw, zRaw] = [v.getInt16(4, true), v.getInt16(6, true), v.getInt16(8, true)];
      return { type: "Gyroscope", ...head, xRaw, yRaw, zRaw, x: q(xRaw, qp), y: q(yRaw, qp), z: q(zRaw, qp) };
    }
    case SensorId.Magnetometer: {
      const [xRaw, yRaw, zRaw] = [v.getInt16(4, true), v.getInt16(6, true), v.getInt16(8, true)];
      return { type: "Magnetometer", ...head, xRaw, yRaw, zRaw, x: q(xRaw, qp), y: q(yRaw, qp), z: q(zRaw, qp) };
    }
    case SensorId.UncalibratedGyroscope:
    case SensorId.UncalibratedMagnetometer: {
      const xRaw = v.getInt16(4, true);
      const yRaw = v.getInt16(6, true);
      const zRaw = v.getInt16(8, true);
      const biasXRaw = v.getInt16(10, true);
      const biasYRaw = v.getInt16(12, true);
      const biasZRaw = v.getInt16(14, true);
      const common = {
        ...head,
        xRaw,
        yRaw,
        zRaw,
        biasXRaw,
        biasYRaw,
        biasZRaw,
        x: q(xRaw, qp),
        y: q(yRaw, qp),
        z: q(zRaw, qp),
        bias: [q(biasXRaw, qp), q(biasYRaw, qp), q(biasZRaw, qp)] as [number, number, number],
      };
      return rid === SensorId.UncalibratedGyroscope
        ? { type: "UncalibratedGyroscope", ...common }
        : { type: "UncalibratedMagnetometer", ...common };
    }
    case SensorId.RotationVector:
    case SensorId.GeomagneticRotationVector:
    case SensorId.ArvrStabilizedRv:
    case SensorId.GameRotationVector:
    case SensorId.ArvrStabilizedGameRv: {
      const iRaw = v.getInt16(4, true);
      const jRaw = v.getInt16(6, true);
      const kRaw = v.getInt16(8, true);
      const realRaw = v.getInt16(10, true);
      const hasAccuracy =
        rid !== SensorId.GameRotationVector && rid !== SensorId.ArvrStabilizedGameRv;
      const accuracyRaw = hasAccuracy ? v.getInt16(12, true) : null;
      return {
        type: "RotationVector",
        ...head,
        iRaw,
        jRaw,
        kRaw,
        realRaw,
        accuracyRaw,
        i: q(iRaw, 14),
        j: q(jRaw, 14),
        k: q(kRaw, 14),
        real: q(realRaw, 14),
        accuracyRad: accuracyRaw === null ? null : q(accuracyRaw, RV_ACCURACY_Q),
      };
    }
    case SensorId.Pressure:
    case SensorId.AmbientLight: {
      const valueRaw = v.getUint32(4, true);
      return { type: "ScalarReport", ...head, valueRaw, value: q(valueRaw, qp) };
    }
    case SensorId.Humidity:
    case SensorId.Proximity: {
      const valueRaw = v.getUint16(4, true);
      return { type: "ScalarReport", ...head, valueRaw, value: q(valueRaw, qp) };
    }
    case SensorId.Temperature: {
      const valueRaw = v.getInt16(4, true);
      return { type: "ScalarReport", ...head, valueRaw, value: q(valueRaw, qp) };
    }
    case SensorId.TapDetector: {
      const flags = rep[4]!;
      return { type: "TapDetector", ...head, flags, doubleTap: (flags & 0x40) !== 0 };
    }
    case SensorId.StepCounter:
      return {
        type: "StepCounter",
        ...head,
        latencyUs: v.getUint32(4, true),
        steps: v.getUint16(8, true),
      };
    case SensorId.StepDetector:
      return { type: "StepDetector", ...head, latencyUs: v.getUint32(4, true) };
    case SensorId.SignificantMotion:
      return { type: "SignificantMotion", ...head, motion: v.getUint16(4, true) };
    case SensorId.StabilityClassifier: {
      const classification = rep[4]!;
      return {
        type: "StabilityClassifier",
        ...head,
        classification,
        name: STABILITY_NAMES[classification] ?? String(classification),
      };
    }
    case SensorId.ShakeDetector:
      return { type: "ShakeDetector", ...head, flags: v.getUint16(4, true) };
    case SensorId.PersonalActivityClassifier: {
      const mostLikelyState = rep[5]!;
      return {
        type: "PersonalActivityClassifier",
        ...head,
        pageNumber: rep[4]! & 0x7f,
        endOfSequence: (rep[4]! & 0x80) !== 0,
        mostLikelyState,
        confidences: Array.from(rep.subarray(6, 16)),
        mostLikelyName: ACTIVITY_NAMES[mostLikelyState] ?? String(mostLikelyState),
      };
    }
    case SensorId.RawAccelerometer:
    case SensorId.RawMagnetometer:
      return {
        type: "RawSensor",
        ...head,
        xRaw: v.getInt16(4, true),
        yRaw: v.getInt16(6, true),
        zRaw: v.getInt16(8, true),
        sensorTimestampUs: v.getUint32(12, true),
        temperatureRaw: 0,
      };
    case SensorId.RawGyroscope:
      return {
        type: "RawSensor",
        ...head,
        xRaw: v.getInt16(4, true),
        yRaw: v.getInt16(6, true),
        zRaw: v.getInt16(8, true),
        sensorTimestampUs: v.getUint32(12, true),
        temperatureRaw: v.getInt16(10, true),
      };
    default:
      // In-table detectors without a dedicated shape
      return { type: "GenericEvent", ...head, valueRaw: v.getUint16(4, true) };
  }
}

/**
 * Parse a channel-5 cargo (gyro-integrated RV, dense format).
 *
 * Two shapes seen on hardware (vendor tool): 7×i16 bare, or prefixed with
 * 0xFB + i32 base delta + u16 delay (both 100 µs ticks).
 */
export function parseGyroRvCargo(
  payload: Uint8Array,
  captureTimestampUs: bigint,
): GyroIntegratedRV | null {
  let ts = captureTimestampUs;
  let body = payload;
  if (payload.length >= 1 && payload[0] === BASE_TIMESTAMP_REF) {
    if (payload.length < 5 + 2 + 14) return null;
    const v = view(payload);
    const delta = v.getInt32(1, true);
    const delay = v.getUint16(5, true);
    ts = captureTimestampUs - BigInt(delta) * 100n + BigInt(delay) * 100n;
    body = payload.subarray(7);
  }
  if (body.length < 14) return null;
  const v = view(body);
  const iRaw = v.getInt16(0, true);
  const jRaw = v.getInt16(2, true);
  const kRaw = v.getInt16(4, true);
  const realRaw = v.getInt16(6, true);
  const vxRaw = v.getInt16(8, true);
  const vyRaw = v.getInt16(10, true);
  const vzRaw = v.getInt16(12, true);
  return {
    type: "GyroIntegratedRV",
    sensorId: SensorId.GyroIntegratedRv,
    timestampUs: ts,
    iRaw,
    jRaw,
    kRaw,
    realRaw,
    vxRaw,
    vyRaw,
    vzRaw,
    i: q(iRaw, 14),
    j: q(jRaw, 14),
    k: q(kRaw, 14),
    real: q(realRaw, 14),
    angularVelocity: [q(vxRaw, GYRO_RV_ANGVEL_Q), q(vyRaw, GYRO_RV_ANGVEL_Q), q(vzRaw, GYRO_RV_ANGVEL_Q)],
  };
}
