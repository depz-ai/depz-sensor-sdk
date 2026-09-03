/** SR04 wire codecs (contracts/03_SENSOR_SR04.md). */

export enum Sr04Cmd {
  GetSamplePeriod = 0x32,
  SetSamplePeriod = 0x33,
  GetEchoDecay = 0x34,
  SetEchoDecay = 0x35,
  MeasureOnce = 0x36,
  StartMeasurementLoop = 0x37,
  StopMeasurementLoop = 0x38,
}

export enum Sr04Rpt {
  Data = 0x91,
  SamplePeriod = 0x92,
  EchoDecay = 0x93,
}

/** echo_time_us sentinel: no echo received. */
export const ECHO_TIMEOUT = 0xffff;

export const SAMPLE_PERIOD_DEFAULT_US = 50_000;
export const ECHO_DECAY_DEFAULT_US = 5_000;
export const ECHO_DECAY_MIN_US = 4_000;
export const ECHO_DECAY_MAX_US = 65_000;

export interface Sr04Data {
  /** 0x36 single shot (host or SYNC_IN), 0x37 loop sample (ERRATA E3). */
  sourceCmd: number;
  timestampUs: bigint;
  echoTimeUs: number;
}

export function unpackSr04Data(p: Uint8Array): Sr04Data {
  const v = new DataView(p.buffer, p.byteOffset, p.byteLength);
  return {
    sourceCmd: v.getUint8(0),
    timestampUs: v.getBigUint64(1, true),
    echoTimeUs: v.getUint16(9, true),
  };
}

export function packSamplePeriod(periodUs: number): Uint8Array {
  const out = new Uint8Array(4);
  new DataView(out.buffer).setUint32(0, periodUs, true);
  return out;
}

export function unpackSamplePeriod(p: Uint8Array): number {
  return new DataView(p.buffer, p.byteOffset, p.byteLength).getUint32(0, true);
}

export function packEchoDecay(decayUs: number): Uint8Array {
  const out = new Uint8Array(2);
  new DataView(out.buffer).setUint16(0, decayUs, true);
  return out;
}

export function unpackEchoDecay(p: Uint8Array): number {
  return new DataView(p.buffer, p.byteOffset, p.byteLength).getUint16(0, true);
}

/**
 * Round-trip echo time → distance in mm; null for the timeout sentinel.
 * Default speed of sound 343 m/s; with airTempC uses c = 331.3 + 0.606·T.
 */
export function distanceMmFromEcho(echoTimeUs: number, airTempC?: number): number | null {
  if (echoTimeUs === ECHO_TIMEOUT) return null;
  const c = airTempC === undefined ? 343.0 : 331.3 + 0.606 * airTempC;
  return (echoTimeUs * c) / 2000.0;
}
