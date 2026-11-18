/**
 * CRC algorithms of the DEPZ transport (contracts/01_TRANSPORT_FRAMING.md §3).
 * Byte-exact with the firmware; CRC-8 init is 0x00 for every device
 * (contracts/ERRATA.md E1).
 */

function makeTable(polyReflected: number): Uint32Array {
  const t = new Uint32Array(256);
  for (let i = 0; i < 256; i++) {
    let crc = i;
    for (let b = 0; b < 8; b++) {
      crc = crc & 1 ? (crc >>> 1) ^ polyReflected : crc >>> 1;
    }
    t[i] = crc >>> 0;
  }
  return t;
}

const CRC8_TABLE = makeTable(0x8c);
const CRC16_TABLE = makeTable(0xa001);
const CRC32_TABLE = makeTable(0xedb88320);

/** CRC-8/MAXIM: poly 0x31 reflected, init 0x00, xorout 0x00. */
export function crc8Maxim(data: Uint8Array): number {
  let crc = 0x00;
  for (const b of data) crc = CRC8_TABLE[crc ^ b]!;
  return crc;
}

/** CRC-16/MODBUS: poly 0x8005 reflected, init 0xFFFF, xorout 0x0000. */
export function crc16Modbus(data: Uint8Array): number {
  let crc = 0xffff;
  for (const b of data) crc = (crc >>> 8) ^ CRC16_TABLE[(crc ^ b) & 0xff]!;
  return crc;
}

/** CRC-32/ISO-HDLC: poly 0x04C11DB7 reflected, init/xorout 0xFFFFFFFF. */
export function crc32IsoHdlc(data: Uint8Array): number {
  let crc = 0xffffffff;
  for (const b of data) crc = (crc >>> 8) ^ CRC32_TABLE[(crc ^ b) & 0xff]!;
  return (crc ^ 0xffffffff) >>> 0;
}

/**
 * CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, not reflected.
 * Used only for the `.fwdepz` file header (contract 06), never on the wire.
 */
export function crc16CcittFalse(data: Uint8Array): number {
  let crc = 0xffff;
  for (const b of data) {
    crc ^= b << 8;
    for (let i = 0; i < 8; i++) {
      crc = crc & 0x8000 ? ((crc << 1) ^ 0x1021) & 0xffff : (crc << 1) & 0xffff;
    }
  }
  return crc;
}
