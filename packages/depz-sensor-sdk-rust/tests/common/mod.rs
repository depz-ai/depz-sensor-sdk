//! Helpers shared by the golden-vector test crates (`mod common;` in each):
//! vector paths and loading, hex, the `$DEPZ_VECTOR_SEEN` coverage report,
//! and the rx/tx packet walk the `.depzrec` decode tests share.
#![allow(dead_code)] // each test crate uses its own subset of these helpers

pub mod fake_board;
pub mod fake_vl53l0x;
pub mod fake_vl53l8;

use std::collections::VecDeque;
use std::fs::OpenOptions;
use std::io::Write;
use std::path::PathBuf;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::time::Duration;

use depz_sensor_sdk::dataset::Dataset;
use depz_sensor_sdk::devices::{Link, ReplayLink};
use depz_sensor_sdk::framing::{Event, PacketParser};
use depz_sensor_sdk::protocol::common::{Cmd, Rpt, Status, StatusReport, TextReport};
use serde_json::Value;

// ---- paths, loading, hex ---------------------------------------------------

pub fn vectors_dir() -> PathBuf {
    // CARGO_MANIFEST_DIR = packages/depz-sensor-sdk-rust
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("..")
        .join("..")
        .join("contracts")
        .join("vectors")
}

pub fn recordings_dir() -> PathBuf {
    vectors_dir().join("recordings")
}

pub fn load(name: &str) -> Value {
    let p = vectors_dir().join(name);
    let txt = std::fs::read_to_string(&p).unwrap_or_else(|e| panic!("read {}: {}", p.display(), e));
    serde_json::from_str(&txt).unwrap_or_else(|e| panic!("parse {}: {}", p.display(), e))
}

pub fn hex_decode(s: &str) -> Vec<u8> {
    assert!(s.len().is_multiple_of(2), "odd hex length: {:?}", s);
    (0..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16).expect("bad hex"))
        .collect()
}

pub fn hex_encode(b: &[u8]) -> String {
    b.iter().map(|x| format!("{:02x}", x)).collect()
}

// ---- coverage report -------------------------------------------------------

/// What `seen` accepts: a case object (its `id`) or the ID itself.
pub trait VectorId {
    fn vector_id(&self) -> &str;
}

impl VectorId for str {
    fn vector_id(&self) -> &str {
        self
    }
}

impl VectorId for String {
    fn vector_id(&self) -> &str {
        self
    }
}

impl VectorId for Value {
    fn vector_id(&self) -> &str {
        self["id"]
            .as_str()
            .unwrap_or_else(|| panic!("vector case without an id: {}", self))
    }
}

/// Report a golden-vector ID as asserted (scripts/check_vector_coverage.py).
/// Call it only after the case's assertions passed. No-op unless
/// `$DEPZ_VECTOR_SEEN` names the output file; the test threads and crates all
/// append to it, so each line goes out as one `O_APPEND` write.
pub fn seen<T: VectorId + ?Sized>(case: &T) {
    let Some(path) = std::env::var_os("DEPZ_VECTOR_SEEN") else {
        return;
    };
    let line = format!("{}\n", case.vector_id());
    OpenOptions::new()
        .create(true)
        .append(true)
        .open(&path)
        .and_then(|mut f| f.write_all(line.as_bytes()))
        .unwrap_or_else(|e| panic!("DEPZ_VECTOR_SEEN {:?}: {}", path, e));
}

// ---- .depzrec / .depzdata reader ops (contracts 08, 09) --------------------
//
// The `input_rules` ops of depzrec.json / depzdata.json, shared by the vector
// runner and the difftest adapter: the file text in, the reader's view out,
// `Err` for a damaged file (never a panic — the runner would fail on one).

static TEMP_SEQ: AtomicUsize = AtomicUsize::new(0);

/// `text` written to a fresh file under the temp dir (deleted by the guard).
pub struct TempText(pub PathBuf);

impl TempText {
    pub fn new(text: &str, suffix: &str) -> TempText {
        let path = std::env::temp_dir().join(format!(
            "depz-vec-{}-{}{}",
            std::process::id(),
            TEMP_SEQ.fetch_add(1, Ordering::Relaxed),
            suffix
        ));
        std::fs::write(&path, text).unwrap_or_else(|e| panic!("write {}: {}", path.display(), e));
        TempText(path)
    }
}

impl Drop for TempText {
    fn drop(&mut self) {
        let _ = std::fs::remove_file(&self.0);
    }
}

/// `depzrec_read`: every rx chunk a loose replay of `text` serves once the host
/// has written more tx bytes than any event waits for, concatenated as hex.
pub fn depzrec_read(text: &str) -> Result<Value, String> {
    let file = TempText::new(text, ".depzrec");
    let link = ReplayLink::open(&file.0, false, false).map_err(|e| format!("{:?}", e))?;
    link.write(&[0u8; 65536]).map_err(|e| format!("{:?}", e))?;
    let mut rx = Vec::new();
    while !link.rx_exhausted() {
        rx.extend(link.read(Duration::ZERO).map_err(|e| format!("{:?}", e))?);
    }
    link.close();
    Ok(serde_json::json!({ "rx": hex_encode(&rx) }))
}

/// `depzdata_read`: the dataset's records in reader order as `[d, t, k]`.
pub fn depzdata_read(text: &str) -> Result<Value, String> {
    let ds = Dataset::parse(text).map_err(|e| format!("{:?}", e))?;
    let records: Vec<Value> = ds
        .records
        .iter()
        .map(|r| serde_json::json!([r.device, r.t, r.kind]))
        .collect();
    Ok(serde_json::json!({ "records": records }))
}

// ---- .depzrec packet walk --------------------------------------------------
//
// The crate has no device layer, so recordings are not strict tx replays:
// both directions go through framing, every rx report through the SDK
// decoders, and the result must match the `.expected.json` sidecar.

/// One framed packet of a `.depzrec` capture, in capture order.
pub struct RecPacket {
    pub rx: bool,
    pub cmd: u8,
    pub payload: Vec<u8>,
}

/// Every packet of `recordings/<file>`, tx and rx each through its own parser.
pub fn recording_packets(file: &str) -> Vec<RecPacket> {
    let p = recordings_dir().join(file);
    let text =
        std::fs::read_to_string(&p).unwrap_or_else(|e| panic!("read {}: {}", p.display(), e));
    let mut tx = PacketParser::new();
    let mut rx = PacketParser::new();
    let mut out = Vec::new();
    for line in text.lines().filter(|l| !l.trim().is_empty()) {
        let ev: Value = serde_json::from_str(line).unwrap();
        let is_rx = match ev["dir"].as_str() {
            Some("rx") => true,
            Some("tx") => false,
            _ => continue, // the header line
        };
        let parser = if is_rx { &mut rx } else { &mut tx };
        for e in parser.feed(&hex_decode(ev["data"].as_str().unwrap())) {
            if let Event::Packet(pkt) = e {
                out.push(RecPacket {
                    rx: is_rx,
                    cmd: pkt.cmd,
                    payload: pkt.payload,
                });
            }
        }
    }
    out
}

/// First TEXT reply to `cmd` (GET_NAME_ACTIVE_SOFTWARE / GET_DEVICE_NAME).
pub fn recorded_text(pkts: &[RecPacket], cmd: Cmd) -> Option<String> {
    pkts.iter()
        .filter(|p| p.rx && p.cmd == Rpt::Text as u8)
        .map(|p| TextReport::unpack(&p.payload).unwrap())
        .find(|t| t.cmd == cmd as u8)
        .map(|t| t.text)
}

/// A REG_DATA reply with the (addr, len) of the READ_REG it answers.
pub struct RegRead {
    pub addr: u16,
    pub len: u16,
    pub reply: Vec<u8>,
}

/// Pair every rx REG_DATA (0x91) with the oldest unanswered tx READ_REG
/// (0x32) — the bridge answers one read at a time, in order; an error STATUS
/// for a READ_REG consumes its request. `wide_addr`: u16 LE addr/len
/// (VL53L4) instead of u8 (BNO055).
pub fn reg_reads(pkts: &[RecPacket], wide_addr: bool) -> Vec<RegRead> {
    let mut pending: VecDeque<(u16, u16)> = VecDeque::new();
    let mut out = Vec::new();
    for p in pkts {
        let pl = &p.payload;
        if !p.rx {
            if p.cmd == 0x32 {
                pending.push_back(if wide_addr {
                    (
                        u16::from_le_bytes([pl[0], pl[1]]),
                        u16::from_le_bytes([pl[2], pl[3]]),
                    )
                } else {
                    (pl[0] as u16, pl[1] as u16)
                });
            }
        } else if p.cmd == 0x91 {
            let (addr, len) = pending.pop_front().expect("REG_DATA without a READ_REG");
            out.push(RegRead {
                addr,
                len,
                reply: pl.clone(),
            });
        } else if p.cmd == Rpt::Status as u8 {
            let s = StatusReport::unpack(pl).unwrap();
            if s.cmd == 0x32 && s.status != Status::Ok as u8 {
                pending.pop_front();
            }
        }
    }
    out
}

pub mod shapes;
