//! HC-SR04 ultrasonic ranger (contract 03).

use std::ops::Deref;
use std::sync::Arc;
use std::time::Duration;

use super::device::{expect_report, DepzDevice, ReportHandler};
use super::error::{DepzError, Result};
use super::stream::{Fanout, Hook, Subscription};
use crate::framing::Packet;
use crate::protocol::sr04::{self, Sr04Cmd, Sr04Data, Sr04Rpt};

const DATA_MIN_BYTES: usize = 11;

/// One SR04 ranging result. `source` is `"once"` (a host command or a SYNC_IN edge) or `"loop"`;
/// [`Sr04Measurement::valid`] is false for the no-echo timeout.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Sr04Measurement {
    pub timestamp_us: u64,
    pub echo_time_us: u16,
    pub source: &'static str,
}

impl Sr04Measurement {
    pub fn valid(&self) -> bool {
        self.echo_time_us != sr04::ECHO_TIMEOUT
    }

    /// Distance at 343 m/s, or `None` when no echo came back.
    pub fn distance_mm(&self) -> Option<f64> {
        sr04::distance_mm_from_echo(self.echo_time_us, None)
    }

    /// Distance with the speed of sound for `air_temp_c` °C.
    pub fn distance_mm_at(&self, air_temp_c: f64) -> Option<f64> {
        sr04::distance_mm_from_echo(self.echo_time_us, Some(air_temp_c))
    }

    /// `None` for a source cmd contract 03 §2 does not define (0x36 / 0x37 only, ERRATA B11):
    /// not a measurement, it falls through to the hex text event.
    fn from(d: &Sr04Data) -> Option<Sr04Measurement> {
        let source = match d.source_cmd {
            c if c == Sr04Cmd::MeasureOnce as u8 => "once",
            c if c == Sr04Cmd::StartMeasurementLoop as u8 => "loop",
            _ => return None,
        };
        Some(Sr04Measurement {
            timestamp_us: d.timestamp_us,
            echo_time_us: d.echo_time_us,
            source,
        })
    }
}

struct Streams {
    measurements: Fanout<Sr04Measurement>,
}

impl ReportHandler for Streams {
    fn handle_report(&self, packet: &Packet) -> bool {
        if packet.cmd != Sr04Rpt::Data as u8 || packet.payload.len() < DATA_MIN_BYTES {
            return false;
        }
        match Sr04Data::unpack(&packet.payload)
            .ok()
            .and_then(|d| Sr04Measurement::from(&d))
        {
            Some(m) => self.measurements.emit(m),
            None => return false,
        }
        true
    }

    fn reader_stopped(&self) {
        self.measurements.close_all();
    }
}

/// HC-SR04 ultrasonic ranger (contract 03). Measurements stream to [`Sr04Device::on_measurement`]
/// callbacks and [`Sr04Device::measurements`] queues: loop samples and the single shots an AUX
/// SYNC_IN edge triggers. Derefs to [`DepzDevice`] for the common commands.
pub struct Sr04Device {
    dev: DepzDevice,
    streams: Arc<Streams>,
}

impl Deref for Sr04Device {
    type Target = DepzDevice;

    fn deref(&self) -> &DepzDevice {
        &self.dev
    }
}

impl Sr04Device {
    /// Continue the open connection `dev` as an SR04 (or open one on a link with [`DepzDevice::new`]).
    pub fn attach(dev: DepzDevice) -> Sr04Device {
        let streams = Arc::new(Streams {
            measurements: Fanout::new(dev.callback_errors_counter()),
        });
        dev.set_report_handler(streams.clone());
        Sr04Device { dev, streams }
    }

    /// The underlying connection.
    pub fn device(&self) -> &DepzDevice {
        &self.dev
    }

    // ── configuration ────────────────────────────────────────────────────────

    /// The stored minimum interval between measurement starts, µs (default 50000) — not the
    /// effective rate, which the echo window throttles (contract 03 §3).
    pub fn get_sample_period_us(&self) -> Result<u32> {
        self.request(
            Sr04Cmd::GetSamplePeriod as u8,
            &[],
            expect_report(Sr04Rpt::SamplePeriod as u8, sr04::unpack_sample_period),
            None,
        )
    }

    /// Set the minimum interval between measurement starts, µs (u32 wire field).
    pub fn set_sample_period_us(&self, period_us: u32) -> Result<()> {
        self.request_ok(
            Sr04Cmd::SetSamplePeriod as u8,
            &sr04::pack_sample_period(period_us),
            None,
        )
    }

    /// The post-echo settle pause, µs (4000–65000).
    pub fn get_echo_decay_us(&self) -> Result<u16> {
        self.request(
            Sr04Cmd::GetEchoDecay as u8,
            &[],
            expect_report(Sr04Rpt::EchoDecay as u8, sr04::unpack_echo_decay),
            None,
        )
    }

    /// Set the settle pause. The device clamps it to 4000–65000 µs silently, so this reads it back and
    /// returns the value in effect.
    pub fn set_echo_decay_us(&self, decay_us: u16) -> Result<u16> {
        self.request_ok(
            Sr04Cmd::SetEchoDecay as u8,
            &sr04::pack_echo_decay(decay_us),
            None,
        )?;
        self.get_echo_decay_us()
    }

    // ── measuring ────────────────────────────────────────────────────────────

    /// One shot. The reply comes when the echo completes (or times out at ~65.5 ms), hence the 1 s
    /// default. Busy while the loop runs.
    pub fn measure_once(&self, timeout: Option<Duration>) -> Result<Sr04Measurement> {
        self.request(
            Sr04Cmd::MeasureOnce as u8,
            &[],
            |p: &Packet| {
                if p.cmd != Sr04Rpt::Data as u8 || p.payload.len() < DATA_MIN_BYTES {
                    return None;
                }
                match Sr04Data::unpack(&p.payload) {
                    Ok(d) if d.source_cmd == Sr04Cmd::MeasureOnce as u8 => {
                        Sr04Measurement::from(&d).map(Ok)
                    }
                    Ok(_) => None,
                    Err(e) => Some(Err(DepzError::from(e))),
                }
            },
            Some(timeout.unwrap_or(Duration::from_secs(1))),
        )
    }

    /// Start the measurement loop (idempotent).
    pub fn start(&self) -> Result<()> {
        self.request_ok(Sr04Cmd::StartMeasurementLoop as u8, &[], None)
    }

    /// Stop the measurement loop (idempotent).
    pub fn stop(&self) -> Result<()> {
        self.request_ok(Sr04Cmd::StopMeasurementLoop as u8, &[], None)
    }

    /// Call `callback` for every measurement (reader thread). Drop the hook to stop.
    pub fn on_measurement(
        &self,
        callback: impl Fn(&Sr04Measurement) + Send + Sync + 'static,
    ) -> Hook {
        self.streams.measurements.on_item(callback)
    }

    /// A pull queue of measurements (bounded, drop-oldest); subscribed when this returns.
    pub fn measurements(&self, capacity: usize) -> Subscription<Sr04Measurement> {
        self.streams.measurements.subscribe(capacity)
    }

    /// The next measurement (contract 07 §3); `timeout` `None`: 2 s.
    pub fn get_measurement(&self, timeout: Option<Duration>) -> Result<Sr04Measurement> {
        self.streams.measurements.next(
            timeout.unwrap_or(Duration::from_secs(2)),
            Sr04Rpt::Data as u8,
            self.is_closed(),
        )
    }

    /// The drop counter of each open [`Sr04Device::measurements`] queue.
    pub fn stream_dropped_counts(&self) -> Vec<u64> {
        self.streams.measurements.dropped_counts()
    }
}
