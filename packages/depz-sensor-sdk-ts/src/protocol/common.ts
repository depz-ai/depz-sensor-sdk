/**
 * Common command/report IDs and payload codecs
 * (contracts/02_COMMON_COMMANDS.md). Raw wire integers only; unit
 * conversions (0.1 °C, µs) happen in the device layer. u64 timestamps are
 * `bigint`.
 */

export enum Cmd {
  Bootloader = 0x01,
  DeviceReset = 0x02,
  GetDeviceName = 0x03,
  GetNameActiveSoftware = 0x04,
  GetSerial = 0x05,
  SyncTime = 0x06,
  GetMcuTemperature = 0x07,
  GetPayloadCrcType = 0x08,
  SetPayloadCrcType = 0x09,
  ThroughputTxStart = 0x1c,
  ThroughputTxStop = 0x1d,
  ThroughputRxData = 0x1e,
  GetSyncPinConfig = 0x30,
  SetSyncPinConfig = 0x31,
}

export enum Rpt {
  Status = 0x80,
  Text = 0x81,
  SyncTime = 0x82,
  Temperature = 0x83,
  SequenceError = 0x84,
  PayloadCrcType = 0x87,
  ThroughputData = 0x88,
  SyncPinConfig = 0x90,
}

export enum Status {
  Ok = 0x00,
  Error = 0x01,
  ErrInvalidCmd = 0x02,
  ErrPayloadFormat = 0x03,
  ErrInvalidParam = 0x04,
  ErrPayloadCrc = 0x05,
  ErrBusy = 0x06,
  ErrCmdNotSupported = 0x07,
  ErrNotInitialized = 0x08,
  ErrHardwareFault = 0x09,
}

export enum SyncPinMode {
  Disable = 0x00,
  In = 0x01,
  OutStart = 0x02,
  OutEnd = 0x03,
  OutBoth = 0x04,
}

export enum SyncPinPolarity {
  IdleLow = 0x00,
  IdleHigh = 0x01,
}

/** Value of the echoed-cmd byte in unsolicited reports. */
export const UNSOLICITED = 0x00;

export interface StatusReport {
  cmd: number;
  status: number;
}

export interface TextReport {
  cmd: number;
  text: string;
}

export interface SyncTimeReport {
  pcTimestampUs: bigint; // T1 echoed
  mcuRxUs: bigint; // T2
  mcuTxUs: bigint; // T3
}

export interface TemperatureReport {
  timestampUs: bigint;
  rawDecidegrees: number; // int16, units of 0.1 °C
}

export interface SequenceErrorReport {
  expectedSeq: number;
  receivedSeq: number;
}

export interface SyncPinConfig {
  pin: number; // 1..5
  mode: SyncPinMode;
  polarity: SyncPinPolarity;
}

function dv(p: Uint8Array): DataView {
  return new DataView(p.buffer, p.byteOffset, p.byteLength);
}

export function unpackStatus(p: Uint8Array): StatusReport {
  return { cmd: p[0]!, status: p[1]! };
}

export function unpackText(p: Uint8Array): TextReport {
  return { cmd: p[0]!, text: stripDeviceString(p.subarray(1)) };
}

export function unpackSyncTime(p: Uint8Array): SyncTimeReport {
  const v = dv(p);
  return {
    pcTimestampUs: v.getBigUint64(0, true),
    mcuRxUs: v.getBigUint64(8, true),
    mcuTxUs: v.getBigUint64(16, true),
  };
}

export function unpackTemperature(p: Uint8Array): TemperatureReport {
  const v = dv(p);
  return { timestampUs: v.getBigUint64(0, true), rawDecidegrees: v.getInt16(8, true) };
}

export function unpackSequenceError(p: Uint8Array): SequenceErrorReport {
  return { expectedSeq: p[0]!, receivedSeq: p[1]! };
}

export function packSyncTime(pcTimestampUs: bigint): Uint8Array {
  const out = new Uint8Array(8);
  new DataView(out.buffer).setBigUint64(0, pcTimestampUs, true);
  return out;
}

export function packSyncPinConfig(cfg: SyncPinConfig): Uint8Array {
  return Uint8Array.of(cfg.pin, cfg.mode, cfg.polarity);
}

export function unpackSyncPinConfig(p: Uint8Array): SyncPinConfig {
  return { pin: p[0]!, mode: p[1]! as SyncPinMode, polarity: p[2]! as SyncPinPolarity };
}

/**
 * NTP-style clock math, all µs (contract 02 §5). offset = device − host,
 * `((T2-T1)+(T3-T4))/2` truncated toward zero (bigint division is exactly
 * that); rtt = (T4-T1)-(T3-T2).
 */
export function syncTimeOffsetRtt(
  t1: bigint,
  t2: bigint,
  t3: bigint,
  t4: bigint,
): { offsetUs: bigint; rttUs: bigint } {
  return {
    offsetUs: (t2 - t1 + (t3 - t4)) / 2n,
    rttUs: t4 - t1 - (t3 - t2),
  };
}

/** Decode an ASCII device string, dropping trailing NUL/0xFF filler. */
export function stripDeviceString(raw: Uint8Array): string {
  let end = raw.length;
  while (end > 0 && (raw[end - 1] === 0x00 || raw[end - 1] === 0xff)) end--;
  let s = "";
  for (let i = 0; i < end; i++) {
    const c = raw[i]!;
    s += c < 0x80 ? String.fromCharCode(c) : "�";
  }
  return s;
}
