//! Live check on real boards: every DEPZ board the SDK finds (or the ports given) is
//! opened, set streaming the usual way and read for N seconds; per board the rate and one
//! plausibility value (the centre distance of a ToF sensor, |gravity| of a BNO055). The same
//! check runs in the Python, Java and C# SDKs, so the numbers can be compared board by board:
//! a line is named by the board's serial number (the port's name when the board gives none).
//!
//!     cargo run --example live_check -- [seconds] [port ...]

use std::time::{Duration, Instant};

use depz_sensor_sdk::bno055::OprMode;
use depz_sensor_sdk::devices::{
    list_depz_devices, open_device, AnyDevice, OpenOptions, Subscription, Vl53l8cxDevice,
    Vl53lxDevice,
};

/// Takes from `sub` for `secs`; returns the count and the values `f` gave (None = not plausible).
fn drain<T>(sub: Subscription<T>, secs: f64, f: impl Fn(&T) -> Option<f64>) -> (usize, Vec<f64>) {
    let end = Instant::now() + Duration::from_secs_f64(secs);
    let (mut n, mut vals) = (0, Vec::new());
    while Instant::now() < end {
        if let Ok(item) = sub.take(Duration::from_millis(200)) {
            n += 1;
            if let Some(v) = f(&item) {
                vals.push(v);
            }
        }
    }
    (n, vals)
}

fn median(mut v: Vec<f64>) -> Option<f64> {
    if v.is_empty() {
        return None;
    }
    v.sort_by(|a, b| a.total_cmp(b));
    Some(v[v.len() / 2])
}

/// The four centre zones of an 8×8 frame.
fn centre(d: &[i32]) -> Option<f64> {
    (d.len() >= 64).then(|| [27, 28, 35, 36].iter().map(|&i| d[i] as f64).sum::<f64>() / 4.0)
}

fn l8(dev: &Vl53l8cxDevice, secs: f64) -> depz_sensor_sdk::devices::Result<(usize, Vec<f64>)> {
    dev.init()?;
    dev.set_resolution(64)?;
    dev.set_ranging_frequency_hz(15)?;
    let sub = dev.frames().subscribe(64);
    dev.start_ranging()?;
    let r = drain(sub, secs, |f| centre(&f.distance_mm));
    dev.stop_ranging()?;
    Ok(r)
}

fn lx(dev: &Vl53lxDevice, secs: f64) -> depz_sensor_sdk::devices::Result<(usize, Vec<f64>)> {
    dev.init(None, None)?;
    let sub = dev.measurements().subscribe(256);
    dev.start_ranging()?;
    let r = drain(sub, secs, |m| {
        (m.status == 0).then_some(m.distance_mm as f64)
    });
    dev.stop_ranging()?;
    Ok(r)
}

fn check(
    dev: &AnyDevice,
    secs: f64,
) -> depz_sensor_sdk::devices::Result<(&'static str, usize, Vec<f64>)> {
    Ok(match dev {
        AnyDevice::Sr04(d) => {
            let sub = d.measurements().subscribe(256);
            d.start()?;
            let (n, v) = drain(sub, secs, |m| m.distance_mm());
            d.stop()?;
            ("mm", n, v)
        }
        AnyDevice::Vl53l4cd(d) => {
            d.init(None)?;
            let sub = d.measurements().subscribe(256);
            d.start_ranging()?;
            let (n, v) = drain(sub, secs, |m| {
                (m.range_status == 0).then_some(m.distance_mm as f64)
            });
            d.stop_ranging()?;
            ("mm", n, v)
        }
        AnyDevice::Vl53l8cx(d) => {
            let (n, v) = l8(d, secs)?;
            ("mm", n, v)
        }
        AnyDevice::Vl53l8ch(d) => {
            let (n, v) = l8(d, secs)?;
            ("mm", n, v)
        }
        AnyDevice::Vl53l5cx(d) => {
            let (n, v) = l8(d, secs)?;
            ("mm", n, v)
        }
        AnyDevice::Vl53l7cx(d) => {
            let (n, v) = l8(d, secs)?;
            ("mm", n, v)
        }
        AnyDevice::Vl53l7ch(d) => {
            let (n, v) = l8(d, secs)?;
            ("mm", n, v)
        }
        AnyDevice::Vl53lx(d) => {
            let (n, v) = lx(d, secs)?;
            ("mm", n, v)
        }
        AnyDevice::Vl53l0x(d) => {
            let (n, v) = lx(d, secs)?;
            ("mm", n, v)
        }
        AnyDevice::Vl53l1cx(d) => {
            let (n, v) = lx(d, secs)?;
            ("mm", n, v)
        }
        AnyDevice::Vl53l1cb(d) => {
            let (n, v) = lx(d, secs)?;
            ("mm", n, v)
        }
        AnyDevice::Vl53l3cx(d) => {
            let (n, v) = lx(d, secs)?;
            ("mm", n, v)
        }
        AnyDevice::Vl53l4cx(d) => {
            let (n, v) = lx(d, secs)?;
            ("mm", n, v)
        }
        AnyDevice::Tmf8829(d) => {
            d.init(None)?;
            let sub = d.measurements().subscribe(64);
            d.start()?;
            let (n, v) = drain(sub, secs, |m| {
                m.distance_mm
                    .as_ref()
                    .and_then(|z| median(z.iter().copied().filter(|&x| x > 0.0).collect()))
            });
            d.stop()?;
            ("mm", n, v)
        }
        AnyDevice::Bno055(d) => {
            d.configure(OprMode::Ndof, None, None, None)?;
            let sub = d.samples().subscribe(1024);
            d.start_stream(10, None, false)?;
            let (n, v) = drain(sub, secs, |s| {
                s.gravity
                    .map(|g| (g[0] * g[0] + g[1] * g[1] + g[2] * g[2]).sqrt())
            });
            ("|g| m/s²", n, v)
        }
        AnyDevice::Bno086(d) => {
            let sub = d.reports().subscribe(4096);
            d.enable(0x05, 100.0)?; // rotation vector
            d.enable(0x01, 100.0)?; // accelerometer
            let (n, v) = drain(sub, secs, |_| None);
            ("reports", n, v)
        }
        _ => ("-", 0, Vec::new()), // Plain, and any class added later
    })
}

fn main() {
    let mut args = std::env::args().skip(1);
    let secs: f64 = args.next().and_then(|s| s.parse().ok()).unwrap_or(10.0);
    let mut ports: Vec<String> = args.collect();
    if ports.is_empty() {
        match list_depz_devices(None, true, None) {
            Ok(found) => ports = found.into_iter().map(|d| d.port).collect(),
            Err(e) => {
                eprintln!("listing the serial ports: {e}");
                std::process::exit(1);
            }
        }
    }
    if ports.is_empty() {
        eprintln!("no DEPZ board found (no serial port has a known DEPZ USB id); a board whose USB id is not a DEPZ one is checked when its port is given: live_check [seconds] [port ...]");
        std::process::exit(1);
    }
    let mut failed = 0;
    for port in &ports {
        let mut name = port.rsplit('/').next().unwrap_or(port).to_string();
        let t = Instant::now();
        let result = open_device(&{
            let mut v = OpenOptions::default();
            v.port = Some(port.to_string());
            v
        })
        .and_then(|dev| {
            // A board that gives no serial number keeps the port's name.
            if let Some(serial) = dev
                .device()
                .get_serial_number()
                .ok()
                .filter(|s| !s.is_empty())
            {
                name = serial;
            }
            let sw = dev.device().get_software_name().unwrap_or_default();
            let r = check(&dev, secs);
            dev.device().close();
            r.map(|r| (sw, r))
        });
        match result {
            Ok((sw, (unit, n, v))) => {
                let med = median(v).map_or("—".to_string(), |m| format!("{m:.1} {unit}"));
                println!(
                    "ok    {name:<24} {sw:<22} {:>6.1}/s  median {med}  ({:.1} s)",
                    n as f64 / secs,
                    t.elapsed().as_secs_f64()
                );
            }
            Err(e) => {
                failed += 1;
                println!("FAIL  {name:<24} {e}");
            }
        }
    }
    std::process::exit(if failed > 0 { 1 } else { 0 });
}
