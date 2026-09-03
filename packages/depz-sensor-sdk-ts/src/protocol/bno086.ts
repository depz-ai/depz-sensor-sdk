/**
 * BNO086 bridge-level wire codecs (contracts/05_SENSOR_BNO086.md §1–2).
 *
 * The MCU is a thin SHTP pass-through: SEND_SHTP_PACKET carries a raw SHTP
 * frame to the sensor, and every inbound SHTP frame arrives as RPT_DATA.
 * Per ERRATA E2 the RPT_DATA `cmd` echo is always 0x00 — correlation happens
 * at the SH-2 layer, never here.
 *
 * Mirrors the Python reference `depz_sensor_sdk.protocol.bno086`.
 */

export enum Bno086Cmd {
  /** Hardware reset via nRST; RPT_STATUS OK. */
  SensorReset = 0x32,
  /** 1 ms WAKE (PS0) pulse; RPT_STATUS OK. */
  SensorWakeUp = 0x33,
  /** Payload = raw SHTP frame; RPT_STATUS OK/ERR_BUSY. */
  SendShtpPacket = 0x34,
}

export enum Bno086Rpt {
  /** rpt_data_t: cmd u8, timestamp_us u64, raw SHTP frame. */
  Data = 0x91,
}

// The MCU keeps two fixed 64-byte SHTP transmit slots (ERRATA E2). A third
// in-flight SEND_SHTP_PACKET gets RPT_STATUS(ERR_BUSY); back off >= 200 ms.
export const TX_SLOTS = 2;
export const TX_SLOT_SIZE = 64;
export const BUSY_BACKOFF_MS = 200;

/** One RPT_DATA report: an SHTP frame captured from the sensor bus. */
export interface Bno086Data {
  /** Always 0x00 in practice (ERRATA E2) — never correlate on it. */
  cmd: number;
  /** MCU uptime at frame capture (µs). */
  timestampUs: bigint;
  /** Raw SHTP frame (4-byte header + cargo fragment). */
  shtp: Uint8Array;
}

export function unpackBno086Data(payload: Uint8Array): Bno086Data {
  const v = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  return {
    cmd: v.getUint8(0),
    timestampUs: v.getBigUint64(1, true),
    shtp: payload.subarray(9),
  };
}
