//! SH-2 input-report catalog and parsers (contract 05 §5).
//!
//! Raw wire integers are authoritative; scaled values are `raw / 2**Q` for the
//! fixed Q points below. Channel-3/4 cargos start with a Base Timestamp
//! Reference (0xFB, i32 base delta in 100 µs ticks, SUBTRACTED from the bridge
//! capture time); each report adds its own 14-bit delay:
//!
//! ```text
//! timestamp_us = capture_us − base_delta*100 + delay*100
//! ```
//!
//! Mirrors the TS/Python `parse_input_cargo` / `parse_gyro_rv_cargo`.

/// In-cargo control IDs on the input channels.
pub const BASE_TIMESTAMP_REF: u8 = 0xfb;
pub const TIMESTAMP_REBASE: u8 = 0xfa;

/// Rotation-vector accuracy estimate Q point (radians).
pub const RV_ACCURACY_Q: u32 = 12;
/// Gyro-integrated RV angular-velocity Q point (rad/s).
pub const GYRO_RV_ANGVEL_Q: u32 = 10;

/// Q point of the primary fields (value = raw / 2**Q).
pub fn q_point(sensor_id: u8) -> u32 {
    match sensor_id {
        0x01 | 0x04 | 0x06 => 8,       // accel / linear accel / gravity
        0x02 | 0x07 => 9,              // gyro / uncal gyro
        0x03 | 0x0f => 4,              // mag / uncal mag
        0x05 | 0x08 | 0x09 | 0x28 | 0x29 | 0x2a => 14, // rotation vectors
        0x0a => 20,                    // pressure
        0x0b | 0x0c => 8,              // ambient light / humidity
        0x0d => 4,                     // proximity
        0x0e => 7,                     // temperature
        _ => 0,
    }
}

/// Total report length on the wire, 4-byte SH-2 header included.
pub fn report_length(sensor_id: u8) -> Option<usize> {
    let n = match sensor_id {
        0x01 | 0x02 | 0x03 | 0x04 | 0x06 => 10,
        0x05 | 0x09 | 0x28 | 0x2a => 14,
        0x07 | 0x0f | 0x14 | 0x15 | 0x16 | 0x1e => 16,
        0x08 | 0x29 => 12,
        0x0a | 0x0b => 8,
        0x0c | 0x0d | 0x0e => 6,
        0x10 => 5,
        0x11 => 12,
        0x12 | 0x13 | 0x19 | 0x1a | 0x1c | 0x1f | 0x20 | 0x21 | 0x22 | 0x23 => 6,
        0x18 | 0x1b => 8,
        _ => return None,
    };
    Some(n)
}

/// A parsed report. Raw fields are authoritative; scale with the Q points.
#[derive(Debug, Clone, PartialEq)]
pub enum Report {
    /// 0x01 accel / 0x04 linear accel / 0x06 gravity (Q8), 0x02 gyro (Q9),
    /// 0x03 magnetometer (Q4).
    Vector3 {
        kind: Vector3Kind,
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        x_raw: i32,
        y_raw: i32,
        z_raw: i32,
    },
    /// 0x07 uncal gyro (Q9) / 0x0F uncal mag (Q4), primary + bias.
    Vector3WithBias {
        is_gyro: bool,
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        x_raw: i32,
        y_raw: i32,
        z_raw: i32,
        bias_x_raw: i32,
        bias_y_raw: i32,
        bias_z_raw: i32,
    },
    /// 0x05/0x08/0x09/0x28/0x29 quaternion (Q14); `accuracy_raw` present for
    /// 0x05/0x09/0x28 only (Q12 radians).
    RotationVector {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        i_raw: i32,
        j_raw: i32,
        k_raw: i32,
        real_raw: i32,
        accuracy_raw: Option<i32>,
    },
    /// 0x0A–0x0E single-value environment reports.
    Scalar {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        value_raw: i64,
    },
    /// 0x10 tap detector; `flags` bit 6 = double tap.
    TapDetector {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        flags: u8,
    },
    /// 0x11 step counter.
    StepCounter {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        latency_us: u32,
        steps: u16,
    },
    /// 0x18 step detector.
    StepDetector {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        latency_us: u32,
    },
    /// 0x12 significant motion.
    SignificantMotion {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        motion: u16,
    },
    /// 0x13 stability classifier.
    StabilityClassifier {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        classification: u8,
    },
    /// 0x19 shake detector; bits 0/1/2 = X/Y/Z.
    ShakeDetector {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        flags: u16,
    },
    /// 0x1E personal activity classifier.
    PersonalActivityClassifier {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        page_number: u8,
        end_of_sequence: bool,
        most_likely_state: u8,
        confidences: Vec<u8>,
    },
    /// 0x14/0x15/0x16 raw ADC + sensor-clock timestamp (temp for raw gyro).
    RawSensor {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        x_raw: i32,
        y_raw: i32,
        z_raw: i32,
        sensor_timestamp_us: u32,
        temperature_raw: i32,
    },
    /// In-table detector without a dedicated shape (u16 value).
    GenericEvent {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        value_raw: u16,
    },
    /// 0x2A gyro-integrated rotation vector (channel 5, dense).
    GyroIntegratedRv {
        sensor_id: u8,
        timestamp_us: i64,
        i_raw: i32,
        j_raw: i32,
        k_raw: i32,
        real_raw: i32,
        vx_raw: i32,
        vy_raw: i32,
        vz_raw: i32,
    },
    /// Unrecognized report ID: raw bytes from the ID to the end of the cargo.
    Unknown {
        sensor_id: u8,
        timestamp_us: i64,
        data: Vec<u8>,
    },
}

/// Which of the plain 3-vector reports.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Vector3Kind {
    Acceleration,
    Gyroscope,
    Magnetometer,
}

fn i16_le(b: &[u8], i: usize) -> i32 {
    i16::from_le_bytes([b[i], b[i + 1]]) as i32
}

fn u16_le(b: &[u8], i: usize) -> u16 {
    u16::from_le_bytes([b[i], b[i + 1]])
}

fn u32_le(b: &[u8], i: usize) -> u32 {
    u32::from_le_bytes([b[i], b[i + 1], b[i + 2], b[i + 3]])
}

fn i32_le(b: &[u8], i: usize) -> i32 {
    i32::from_le_bytes([b[i], b[i + 1], b[i + 2], b[i + 3]])
}

/// Parse a channel-3/4 cargo into typed reports. `capture_timestamp_us` is the
/// bridge RPT_DATA capture time (MCU uptime).
pub fn parse_input_cargo(payload: &[u8], capture_timestamp_us: i64) -> Vec<Report> {
    let mut out = Vec::new();
    let mut base_us = capture_timestamp_us;
    let n = payload.len();
    let mut pos = 0usize;
    while pos < n {
        let rid = payload[pos];
        if rid == BASE_TIMESTAMP_REF && pos + 5 <= n {
            let delta = i32_le(payload, pos + 1) as i64;
            base_us = capture_timestamp_us - delta * 100;
            pos += 5;
            continue;
        }
        if rid == TIMESTAMP_REBASE && pos + 5 <= n {
            let delta = i32_le(payload, pos + 1) as i64;
            base_us += delta * 100;
            pos += 5;
            continue;
        }
        let length = match report_length(rid) {
            Some(l) if pos + l <= n => l,
            _ => {
                out.push(Report::Unknown {
                    sensor_id: rid,
                    timestamp_us: base_us,
                    data: payload[pos..].to_vec(),
                });
                break;
            }
        };
        let rep = &payload[pos..pos + length];
        let seq = rep[1];
        let status = rep[2];
        let delay_lsb = rep[3];
        let accuracy = status & 0x03;
        let delay_us = ((((status >> 2) as i64) << 8) | delay_lsb as i64) * 100;
        let ts = base_us + delay_us;
        out.push(decode_report(rid, rep, ts, seq, accuracy, delay_us));
        pos += length;
    }
    out
}

fn decode_report(rid: u8, rep: &[u8], ts: i64, seq: u8, accuracy: u8, delay_us: i64) -> Report {
    match rid {
        0x01 | 0x04 | 0x06 | 0x02 | 0x03 => {
            let kind = match rid {
                0x02 => Vector3Kind::Gyroscope,
                0x03 => Vector3Kind::Magnetometer,
                _ => Vector3Kind::Acceleration,
            };
            Report::Vector3 {
                kind,
                sensor_id: rid,
                timestamp_us: ts,
                seq,
                accuracy,
                delay_us,
                x_raw: i16_le(rep, 4),
                y_raw: i16_le(rep, 6),
                z_raw: i16_le(rep, 8),
            }
        }
        0x07 | 0x0f => Report::Vector3WithBias {
            is_gyro: rid == 0x07,
            sensor_id: rid,
            timestamp_us: ts,
            seq,
            accuracy,
            delay_us,
            x_raw: i16_le(rep, 4),
            y_raw: i16_le(rep, 6),
            z_raw: i16_le(rep, 8),
            bias_x_raw: i16_le(rep, 10),
            bias_y_raw: i16_le(rep, 12),
            bias_z_raw: i16_le(rep, 14),
        },
        0x05 | 0x09 | 0x28 | 0x08 | 0x29 => {
            let has_accuracy = rid != 0x08 && rid != 0x29;
            Report::RotationVector {
                sensor_id: rid,
                timestamp_us: ts,
                seq,
                accuracy,
                delay_us,
                i_raw: i16_le(rep, 4),
                j_raw: i16_le(rep, 6),
                k_raw: i16_le(rep, 8),
                real_raw: i16_le(rep, 10),
                accuracy_raw: if has_accuracy { Some(i16_le(rep, 12)) } else { None },
            }
        }
        0x0a | 0x0b => Report::Scalar {
            sensor_id: rid,
            timestamp_us: ts,
            seq,
            accuracy,
            delay_us,
            value_raw: u32_le(rep, 4) as i64,
        },
        0x0c | 0x0d => Report::Scalar {
            sensor_id: rid,
            timestamp_us: ts,
            seq,
            accuracy,
            delay_us,
            value_raw: u16_le(rep, 4) as i64,
        },
        0x0e => Report::Scalar {
            sensor_id: rid,
            timestamp_us: ts,
            seq,
            accuracy,
            delay_us,
            value_raw: i16_le(rep, 4) as i64,
        },
        0x10 => Report::TapDetector {
            sensor_id: rid,
            timestamp_us: ts,
            seq,
            accuracy,
            delay_us,
            flags: rep[4],
        },
        0x11 => Report::StepCounter {
            sensor_id: rid,
            timestamp_us: ts,
            seq,
            accuracy,
            delay_us,
            latency_us: u32_le(rep, 4),
            steps: u16_le(rep, 8),
        },
        0x18 => Report::StepDetector {
            sensor_id: rid,
            timestamp_us: ts,
            seq,
            accuracy,
            delay_us,
            latency_us: u32_le(rep, 4),
        },
        0x12 => Report::SignificantMotion {
            sensor_id: rid,
            timestamp_us: ts,
            seq,
            accuracy,
            delay_us,
            motion: u16_le(rep, 4),
        },
        0x13 => Report::StabilityClassifier {
            sensor_id: rid,
            timestamp_us: ts,
            seq,
            accuracy,
            delay_us,
            classification: rep[4],
        },
        0x19 => Report::ShakeDetector {
            sensor_id: rid,
            timestamp_us: ts,
            seq,
            accuracy,
            delay_us,
            flags: u16_le(rep, 4),
        },
        0x1e => Report::PersonalActivityClassifier {
            sensor_id: rid,
            timestamp_us: ts,
            seq,
            accuracy,
            delay_us,
            page_number: rep[4] & 0x7f,
            end_of_sequence: (rep[4] & 0x80) != 0,
            most_likely_state: rep[5],
            confidences: rep[6..16].to_vec(),
        },
        0x14 | 0x16 => Report::RawSensor {
            sensor_id: rid,
            timestamp_us: ts,
            seq,
            accuracy,
            delay_us,
            x_raw: i16_le(rep, 4),
            y_raw: i16_le(rep, 6),
            z_raw: i16_le(rep, 8),
            sensor_timestamp_us: u32_le(rep, 12),
            temperature_raw: 0,
        },
        0x15 => Report::RawSensor {
            sensor_id: rid,
            timestamp_us: ts,
            seq,
            accuracy,
            delay_us,
            x_raw: i16_le(rep, 4),
            y_raw: i16_le(rep, 6),
            z_raw: i16_le(rep, 8),
            sensor_timestamp_us: u32_le(rep, 12),
            temperature_raw: i16_le(rep, 10),
        },
        _ => Report::GenericEvent {
            sensor_id: rid,
            timestamp_us: ts,
            seq,
            accuracy,
            delay_us,
            value_raw: u16_le(rep, 4),
        },
    }
}

/// Parse a channel-5 cargo (gyro-integrated RV, dense format).
///
/// Two shapes: 7×i16 bare, or prefixed with 0xFB + i32 base delta + u16 delay
/// (both 100 µs ticks). Returns `None` if the cargo is too short.
pub fn parse_gyro_rv_cargo(payload: &[u8], capture_timestamp_us: i64) -> Option<Report> {
    let mut ts = capture_timestamp_us;
    let mut body: &[u8] = payload;
    if !payload.is_empty() && payload[0] == BASE_TIMESTAMP_REF {
        if payload.len() < 5 + 2 + 14 {
            return None;
        }
        let delta = i32_le(payload, 1) as i64;
        let delay = u16_le(payload, 5) as i64;
        ts = capture_timestamp_us - delta * 100 + delay * 100;
        body = &payload[7..];
    }
    if body.len() < 14 {
        return None;
    }
    Some(Report::GyroIntegratedRv {
        sensor_id: 0x2a,
        timestamp_us: ts,
        i_raw: i16_le(body, 0),
        j_raw: i16_le(body, 2),
        k_raw: i16_le(body, 4),
        real_raw: i16_le(body, 6),
        vx_raw: i16_le(body, 8),
        vy_raw: i16_le(body, 10),
        vz_raw: i16_le(body, 12),
    })
}
