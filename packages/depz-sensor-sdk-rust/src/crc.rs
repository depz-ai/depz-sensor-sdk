//! CRC algorithms of the DEPZ transport (contracts/01_TRANSPORT_FRAMING.md §3).
//!
//! All wire CRCs are reflected implementations, byte-exact with the firmware
//! (`common/crc/crc8.c`) and the reference host tool. CRC-8 init is 0x00 for
//! every device — see contracts/ERRATA.md E1.

/// CRC-8/MAXIM: poly 0x31 reflected (0x8C), init 0x00, xorout 0x00.
pub fn crc8_maxim(data: &[u8]) -> u8 {
    let mut crc: u8 = 0x00;
    for &b in data {
        crc ^= b;
        for _ in 0..8 {
            crc = if crc & 1 != 0 { (crc >> 1) ^ 0x8C } else { crc >> 1 };
        }
    }
    crc
}

/// CRC-16/MODBUS: poly 0x8005 reflected (0xA001), init 0xFFFF, xorout 0x0000.
pub fn crc16_modbus(data: &[u8]) -> u16 {
    let mut crc: u16 = 0xFFFF;
    for &b in data {
        crc ^= b as u16;
        for _ in 0..8 {
            crc = if crc & 1 != 0 { (crc >> 1) ^ 0xA001 } else { crc >> 1 };
        }
    }
    crc
}

/// CRC-32/ISO-HDLC: poly 0x04C11DB7 reflected (0xEDB88320), init/xorout 0xFFFFFFFF.
pub fn crc32_iso_hdlc(data: &[u8]) -> u32 {
    let mut crc: u32 = 0xFFFF_FFFF;
    for &b in data {
        crc ^= b as u32;
        for _ in 0..8 {
            crc = if crc & 1 != 0 {
                (crc >> 1) ^ 0xEDB8_8320
            } else {
                crc >> 1
            };
        }
    }
    crc ^ 0xFFFF_FFFF
}

/// CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, not reflected.
///
/// Used only for the `.fwdepz` file header (contract 06), never on the wire.
pub fn crc16_ccitt_false(data: &[u8]) -> u16 {
    let mut crc: u16 = 0xFFFF;
    for &b in data {
        crc ^= (b as u16) << 8;
        for _ in 0..8 {
            crc = if crc & 0x8000 != 0 {
                (crc << 1) ^ 0x1021
            } else {
                crc << 1
            };
        }
    }
    crc
}
