/**
 * `.fwdepz` firmware container parse/validate (contracts/06 §2).
 * Pure functions — used by the browser flashing flow to validate a dropped
 * file before touching the device.
 */

import { crc16CcittFalse, crc32IsoHdlc } from "./crc.js";

export const FWDEPZ_MAGIC = "FWDEPZ00";
export const FWDEPZ_HEADER_SIZE = 64;

export class FwDepzError extends Error {}

export interface FwDepzImage {
  loadAddr: number;
  fwSize: number;
  fwCrc32: number;
  curSec: number;
  totSec: number;
  payload: Uint8Array;
}

/** Validation order (contract 06 §2): magic → header CRC → size. */
export function parseFwDepz(blob: Uint8Array): FwDepzImage {
  if (blob.length < FWDEPZ_HEADER_SIZE) {
    throw new FwDepzError(`file too short: ${blob.length} < ${FWDEPZ_HEADER_SIZE}`);
  }
  for (let i = 0; i < 8; i++) {
    if (blob[i] !== FWDEPZ_MAGIC.charCodeAt(i)) {
      throw new FwDepzError("bad magic (not a .fwdepz file)");
    }
  }
  const dv = new DataView(blob.buffer, blob.byteOffset, blob.byteLength);
  const storedCrc = dv.getUint16(62, true);
  const actualCrc = crc16CcittFalse(blob.subarray(0, 62));
  if (storedCrc !== actualCrc) {
    throw new FwDepzError(
      `header CRC mismatch: stored=0x${storedCrc.toString(16)} actual=0x${actualCrc.toString(16)}`,
    );
  }
  const loadAddr = dv.getUint32(8, true);
  const fwSize = dv.getUint32(12, true);
  const fwCrc32 = dv.getUint32(16, true);
  const payload = blob.subarray(FWDEPZ_HEADER_SIZE);
  if (fwSize !== payload.length) {
    throw new FwDepzError(`fw_size=${fwSize} but payload is ${payload.length} bytes`);
  }
  return {
    loadAddr,
    fwSize,
    fwCrc32,
    curSec: blob[20]!,
    totSec: blob[21]!,
    payload,
  };
}

export function fwDepzPayloadCrcOk(img: FwDepzImage): boolean {
  return crc32IsoHdlc(img.payload) === img.fwCrc32;
}
