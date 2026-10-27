//! The gzip container for `.depzdata.gz`, without a dependency: [`gunzip`] inflates any member
//! (stored, fixed or dynamic Huffman blocks — a small port of zlib's `puff`), [`GzWriter`] writes
//! stored blocks: a valid gzip file every reader opens, not a compressed one.

use std::io::{self, Write};

const CRC_TABLE: [u32; 256] = {
    let mut t = [0u32; 256];
    let mut n = 0;
    while n < 256 {
        let mut c = n as u32;
        let mut k = 0;
        while k < 8 {
            c = if c & 1 != 0 {
                0xEDB8_8320 ^ (c >> 1)
            } else {
                c >> 1
            };
            k += 1;
        }
        t[n] = c;
        n += 1;
    }
    t
};

/// Running CRC-32/ISO-HDLC (the gzip trailer's).
#[derive(Clone, Copy)]
struct Crc32(u32);

impl Crc32 {
    fn new() -> Crc32 {
        Crc32(0xFFFF_FFFF)
    }

    fn update(&mut self, data: &[u8]) {
        for &b in data {
            self.0 = CRC_TABLE[((self.0 ^ b as u32) & 0xFF) as usize] ^ (self.0 >> 8);
        }
    }

    fn value(self) -> u32 {
        self.0 ^ 0xFFFF_FFFF
    }
}

/// True when `data` starts with the gzip magic.
pub(crate) fn is_gzip(data: &[u8]) -> bool {
    data.len() >= 2 && data[0] == 0x1F && data[1] == 0x8B
}

/// The content of a gzip file (members one after the other, as `gzip -d` reads them).
pub(crate) fn gunzip(data: &[u8]) -> Result<Vec<u8>, String> {
    let mut out = Vec::new();
    let mut pos = 0;
    while pos < data.len() {
        let h = data.get(pos..pos + 10).ok_or("gzip: truncated header")?;
        if h[0] != 0x1F || h[1] != 0x8B || h[2] != 8 {
            return Err("gzip: not a deflate member".into());
        }
        let flags = h[3];
        pos += 10;
        if flags & 0x04 != 0 {
            let x = data
                .get(pos..pos + 2)
                .ok_or("gzip: truncated extra field")?;
            pos += 2 + u16::from_le_bytes([x[0], x[1]]) as usize;
        }
        for bit in [0x08, 0x10] {
            // file name, comment: zero-terminated
            if flags & bit != 0 {
                let end = data
                    .get(pos..)
                    .and_then(|r| r.iter().position(|&b| b == 0))
                    .ok_or("gzip: truncated name")?;
                pos += end + 1;
            }
        }
        if flags & 0x02 != 0 {
            pos += 2; // header CRC
        }
        let start = out.len();
        let mut inf = Inflate {
            src: data,
            pos,
            bit_buf: 0,
            bit_cnt: 0,
            out: &mut out,
        };
        inf.run()?;
        pos = inf.pos;
        let t = data.get(pos..pos + 8).ok_or("gzip: truncated trailer")?;
        let mut crc = Crc32::new();
        crc.update(&out[start..]);
        if u32::from_le_bytes([t[0], t[1], t[2], t[3]]) != crc.value()
            || u32::from_le_bytes([t[4], t[5], t[6], t[7]]) != (out.len() - start) as u32
        {
            return Err("gzip: CRC or length mismatch".into());
        }
        pos += 8;
    }
    Ok(out)
}

const MAX_BITS: usize = 15;

// A canonical Huffman code: the number of codes per length and the symbols in code order.
struct Huffman {
    count: [u16; MAX_BITS + 1],
    symbol: Vec<u16>,
}

impl Huffman {
    fn new(lengths: &[u8]) -> Result<Huffman, String> {
        let mut count = [0u16; MAX_BITS + 1];
        for &l in lengths {
            count[l as usize] += 1;
        }
        let mut left: i32 = 1;
        for &c in &count[1..] {
            left = left * 2 - c as i32;
            if left < 0 {
                return Err("inflate: over-subscribed code".into());
            }
        }
        let mut offs = [0u16; MAX_BITS + 2];
        for len in 1..=MAX_BITS {
            offs[len + 1] = offs[len] + count[len];
        }
        let mut symbol = vec![0u16; lengths.len()];
        for (s, &l) in lengths.iter().enumerate() {
            if l != 0 {
                symbol[offs[l as usize] as usize] = s as u16;
                offs[l as usize] += 1;
            }
        }
        Ok(Huffman { count, symbol })
    }
}

const LEN_BASE: [u16; 29] = [
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131,
    163, 195, 227, 258,
];
const LEN_EXTRA: [u8; 29] = [
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0,
];
const DIST_BASE: [u16; 30] = [
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537,
    2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577,
];
const DIST_EXTRA: [u8; 30] = [
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13,
    13,
];
const CL_ORDER: [usize; 19] = [
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15,
];

struct Inflate<'a> {
    src: &'a [u8],
    pos: usize,
    bit_buf: u32,
    bit_cnt: u32,
    out: &'a mut Vec<u8>,
}

impl Inflate<'_> {
    fn bits(&mut self, need: u32) -> Result<u32, String> {
        while self.bit_cnt < need {
            let b = *self
                .src
                .get(self.pos)
                .ok_or("inflate: unexpected end of data")?;
            self.pos += 1;
            self.bit_buf |= (b as u32) << self.bit_cnt;
            self.bit_cnt += 8;
        }
        let v = self.bit_buf & ((1u32 << need) - 1);
        self.bit_buf >>= need;
        self.bit_cnt -= need;
        Ok(v)
    }

    fn run(&mut self) -> Result<(), String> {
        loop {
            let last = self.bits(1)?;
            match self.bits(2)? {
                0 => self.stored()?,
                1 => {
                    let mut lengths = [0u8; 288];
                    lengths[..144].fill(8);
                    lengths[144..256].fill(9);
                    lengths[256..280].fill(7);
                    lengths[280..].fill(8);
                    let (lit, dist) = (Huffman::new(&lengths)?, Huffman::new(&[5u8; 30])?);
                    self.codes(&lit, &dist)?;
                }
                2 => self.dynamic()?,
                _ => return Err("inflate: invalid block type".into()),
            }
            if last == 1 {
                return Ok(());
            }
        }
    }

    fn stored(&mut self) -> Result<(), String> {
        self.bit_buf = 0; // to the byte boundary
        self.bit_cnt = 0;
        let h = self
            .src
            .get(self.pos..self.pos + 4)
            .ok_or("inflate: truncated stored block")?;
        let len = u16::from_le_bytes([h[0], h[1]]);
        if len != !u16::from_le_bytes([h[2], h[3]]) {
            return Err("inflate: stored length check failed".into());
        }
        self.pos += 4;
        let data = self
            .src
            .get(self.pos..self.pos + len as usize)
            .ok_or("inflate: truncated stored block")?;
        self.out.extend_from_slice(data);
        self.pos += len as usize;
        Ok(())
    }

    fn decode(&mut self, h: &Huffman) -> Result<u16, String> {
        let (mut code, mut first, mut index) = (0i32, 0i32, 0i32);
        for len in 1..=MAX_BITS {
            code |= self.bits(1)? as i32;
            let count = h.count[len] as i32;
            if code - count < first {
                return Ok(h.symbol[(index + (code - first)) as usize]);
            }
            index += count;
            first = (first + count) << 1;
            code <<= 1;
        }
        Err("inflate: invalid code".into())
    }

    fn codes(&mut self, lit: &Huffman, dist: &Huffman) -> Result<(), String> {
        loop {
            let sym = self.decode(lit)? as usize;
            match sym {
                0..=255 => self.out.push(sym as u8),
                256 => return Ok(()),
                _ => {
                    let i = sym - 257;
                    if i >= 29 {
                        return Err("inflate: invalid length symbol".into());
                    }
                    let len = LEN_BASE[i] as usize + self.bits(LEN_EXTRA[i] as u32)? as usize;
                    let d = self.decode(dist)? as usize;
                    if d >= 30 {
                        return Err("inflate: invalid distance symbol".into());
                    }
                    let back = DIST_BASE[d] as usize + self.bits(DIST_EXTRA[d] as u32)? as usize;
                    if back > self.out.len() {
                        return Err("inflate: distance too far back".into());
                    }
                    let from = self.out.len() - back;
                    for k in 0..len {
                        let b = self.out[from + k];
                        self.out.push(b);
                    }
                }
            }
        }
    }

    fn dynamic(&mut self) -> Result<(), String> {
        let nlen = self.bits(5)? as usize + 257;
        let ndist = self.bits(5)? as usize + 1;
        let ncode = self.bits(4)? as usize + 4;
        if nlen > 286 || ndist > 30 {
            return Err("inflate: bad code counts".into());
        }
        let mut cl = [0u8; 19];
        for &o in &CL_ORDER[..ncode] {
            cl[o] = self.bits(3)? as u8;
        }
        let cl_code = Huffman::new(&cl)?;
        let mut lengths = vec![0u8; nlen + ndist];
        let mut i = 0;
        while i < nlen + ndist {
            let sym = self.decode(&cl_code)?;
            let (value, repeat) = match sym {
                0..=15 => (sym as u8, 1),
                16 => {
                    let prev = *lengths
                        .get(i.wrapping_sub(1))
                        .ok_or("inflate: repeat with no length")?;
                    (prev, 3 + self.bits(2)? as usize)
                }
                17 => (0, 3 + self.bits(3)? as usize),
                _ => (0, 11 + self.bits(7)? as usize),
            };
            if i + repeat > nlen + ndist {
                return Err("inflate: too many lengths".into());
            }
            lengths[i..i + repeat].fill(value);
            i += repeat;
        }
        if lengths[256] == 0 {
            return Err("inflate: no end-of-block code".into());
        }
        let lit = Huffman::new(&lengths[..nlen])?;
        let dist = Huffman::new(&lengths[nlen..])?;
        self.codes(&lit, &dist)
    }
}

const STORED_MAX: usize = 0xFFFF;

/// A gzip stream of stored blocks over `out`; [`GzWriter::finish`] writes the last block and the
/// trailer.
pub(crate) struct GzWriter<W: Write> {
    out: W,
    buf: Vec<u8>,
    crc: Crc32,
    size: u32,
}

impl<W: Write> GzWriter<W> {
    pub(crate) fn new(mut out: W) -> io::Result<GzWriter<W>> {
        // magic, deflate, no flags, no mtime, no extra flags, OS unknown
        out.write_all(&[0x1F, 0x8B, 8, 0, 0, 0, 0, 0, 0, 0xFF])?;
        Ok(GzWriter {
            out,
            buf: Vec::with_capacity(STORED_MAX),
            crc: Crc32::new(),
            size: 0,
        })
    }

    fn block(&mut self, last: bool) -> io::Result<()> {
        let n = self.buf.len() as u16;
        let [a, b] = n.to_le_bytes();
        let [c, d] = (!n).to_le_bytes();
        self.out.write_all(&[last as u8, a, b, c, d])?;
        self.out.write_all(&self.buf)?;
        self.buf.clear();
        Ok(())
    }

    pub(crate) fn finish(mut self) -> io::Result<W> {
        self.block(true)?;
        self.out.write_all(&self.crc.value().to_le_bytes())?;
        self.out.write_all(&self.size.to_le_bytes())?;
        self.out.flush()?;
        Ok(self.out)
    }
}

impl<W: Write> Write for GzWriter<W> {
    fn write(&mut self, data: &[u8]) -> io::Result<usize> {
        let n = data.len().min(STORED_MAX - self.buf.len());
        self.buf.extend_from_slice(&data[..n]);
        self.crc.update(&data[..n]);
        self.size = self.size.wrapping_add(n as u32);
        if self.buf.len() == STORED_MAX {
            self.block(false)?;
        }
        Ok(n)
    }

    fn flush(&mut self) -> io::Result<()> {
        self.out.flush()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_stored_stream_reads_back() {
        let data: Vec<u8> = (0..200_000u32).map(|i| (i % 251) as u8).collect();
        let mut w = GzWriter::new(Vec::new()).unwrap();
        w.write_all(&data).unwrap();
        let gz = w.finish().unwrap();
        assert!(is_gzip(&gz));
        assert_eq!(gunzip(&gz).unwrap(), data);
    }

    #[test]
    fn an_empty_stream_reads_back() {
        let gz = GzWriter::new(Vec::new()).unwrap().finish().unwrap();
        assert_eq!(gunzip(&gz).unwrap(), Vec::<u8>::new());
    }

    #[test]
    fn a_fixed_huffman_member_inflates() {
        // Python `gzip.compress(b"hello hello hello\n", mtime=0)`: fixed codes, a back-reference.
        let gz = [
            0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0xcb, 0x48, 0xcd, 0xc9,
            0xc9, 0x57, 0xc8, 0x40, 0x90, 0x5c, 0x00, 0x3b, 0x7c, 0x8a, 0xdf, 0x12, 0x00, 0x00,
            0x00,
        ];
        assert_eq!(gunzip(&gz).unwrap(), b"hello hello hello\n");
    }

    #[test]
    fn a_dynamic_huffman_member_inflates() {
        // Python `gzip.compress(text, mtime=0)` of 40 sr04 record lines: one dynamic-code block.
        let hex = [
            "1f8b08000000000002039d94bd0a023110847b1f63eb2b76926cfe5ec6c213040bc1539b70efee8995a4b8386502c3c0",
            "f07ddb64962ab3ca240fa93ac9757b2e770ddbc74b6a93f3e9723b3e17a9a6aaeb7a683f81cfe74e085d08ba9f727d6a",
            "a0cb772937d015fad4409775293fd015fbd440573f7c5066f960ccf2a6ccf266ccf25199e5a331cb2765964f14f39962",
            "3e53cc178af94231ff75f9efe9a114f500853d40710f47810f47910f4fa10f4fb18f40c18fc05d7c234f3ec53f222500",
            "226740e20c489c01993320730614ce803268c01bbba45af17e080000",
        ]
        .concat();
        let gz: Vec<u8> = (0..hex.len())
            .step_by(2)
            .map(|i| u8::from_str_radix(&hex[i..i + 2], 16).unwrap())
            .collect();
        let text: String = (0..40)
            .map(|i| {
                format!(
                    "{{\"d\":\"d0\",\"t\":{},\"k\":\"sr04\",\"v\":{{\"echo_us\":{}}}}}\n",
                    i * 50000,
                    5000 + i % 7
                )
            })
            .collect();
        assert_eq!(gunzip(&gz).unwrap(), text.as_bytes());
    }

    #[test]
    fn a_corrupt_trailer_is_an_error() {
        let mut w = GzWriter::new(Vec::new()).unwrap();
        w.write_all(b"abc").unwrap();
        let mut gz = w.finish().unwrap();
        let n = gz.len();
        gz[n - 8] ^= 1;
        assert!(gunzip(&gz).is_err());
    }
}
