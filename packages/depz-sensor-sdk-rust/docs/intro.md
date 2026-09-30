# Rust SDK

`depz-sensor-sdk` (Rust) is the **contract-first, transport-agnostic
foundation** for the DEPZ USB sensor line: byte-exact codecs, framing/parsing,
CRCs, the USB-id table, per-sensor frame/report decoders, the `.fwdepz`
firmware-container parser and the `.depzdata` dataset reader. Everything is
pure computation over `&[u8]` — no threads, no I/O — verified against the
shared golden vectors and byte-for-byte identical to the other reference SDKs.

**Working with one specific sensor?** Each has its own introduction, guide
and API reference:

- **[SR04](sr04/introduction.md)** — ultrasonic ranging: one distance per ping.
- **[VL53L4CD](vl53l4cd/introduction.md)** — single-zone ToF ranging (~1.3 m).
- **[VL53L8CX](vl53l8cx/introduction.md)** — 8×8 ToF depth frames.
- **[VL53L8CH](vl53l8ch/introduction.md)** — the CX superset with CNH histograms.
- **[VL53L5CX](vl53l5cx/introduction.md)** / **[VL53L7CX](vl53l7cx/introduction.md)**
  — 8×8 ToF depth frames on the I2C board, 63° / 90° field of view.
- **[VL53L7CH](vl53l7ch/introduction.md)** — the VL53L7CX with CNH histograms.
- **[VL53L0X](vl53l0x/introduction.md)** · **[VL53L1CX](vl53l1cx/introduction.md)** ·
  **[VL53L1CB](vl53l1cb/introduction.md)** · **[VL53L3CX](vl53l3cx/introduction.md)** ·
  **[VL53L4CX](vl53l4cx/introduction.md)** — the 1D ToF family: single-zone
  ranging from ~2 m to ~8 m, with histograms on the L1/L3/L4.
- **[BNO086](bno086/introduction.md)** — 9-axis IMU: orientation and motion.
- **[BNO055](bno055/introduction.md)** — 9-axis IMU with on-chip fusion:
  orientation from one register block.

The VL53L5CX / L7CX / L7CH, the 1D family and the BNO055 have the decode and
codec layer in this crate; initialising and streaming them is done with the
Python or TypeScript SDK.

## Quickstart

Frame a command, parse a reply, decode it — the whole round trip is
synchronous and pure:

```rust
use depz_sensor_sdk::framing::{build_packet, CrcType, Event, PacketParser};
use depz_sensor_sdk::protocol::sr04::{Sr04Cmd, Sr04Data, distance_mm_from_echo};

// Host → device: frame the "start measurement loop" command.
let tx = build_packet(Sr04Cmd::StartMeasurementLoop as u8, &[], 0, CrcType::None).unwrap();
// ... write `tx` to your serial port ...

// Device → host: feed whatever bytes arrive; drain decoded packets.
let mut parser = PacketParser::new();
for ev in parser.feed(&rx_bytes) {
    if let Event::Packet(pkt) = ev {
        if pkt.cmd == 0x91 {                       // RPT_DATA
            let d = Sr04Data::unpack(&pkt.payload).unwrap();
            println!("{:?} mm", distance_mm_from_echo(d.echo_time_us, None));
        }
    }
}
```

## Where to next

- **Your sensor's pages** — [SR04](sr04/introduction.md) ·
  [VL53L4CD](vl53l4cd/introduction.md) · [VL53L8CX](vl53l8cx/introduction.md) ·
  [VL53L8CH](vl53l8ch/introduction.md) · [VL53L5CX](vl53l5cx/introduction.md) ·
  [VL53L7CX](vl53l7cx/introduction.md) · [VL53L7CH](vl53l7ch/introduction.md) ·
  [VL53L0X](vl53l0x/introduction.md) · [VL53L1CX](vl53l1cx/introduction.md) ·
  [VL53L1CB](vl53l1cb/introduction.md) · [VL53L3CX](vl53l3cx/introduction.md) ·
  [VL53L4CX](vl53l4cx/introduction.md) · [BNO086](bno086/introduction.md) ·
  [BNO055](bno055/introduction.md): introduction, hands-on guide, and the
  sensor's own API reference.
- **[Guide](guide.md)** — the SDK-wide walkthrough: getting started, the
  mental model, framing, identity, firmware containers, datasets.
- **[API Reference](api.md)** — the whole crate surface, generated from the
  doc-comments.
- **Docs for LLMs** — the sensor SDK documentation as raw Markdown:
  [/llms-full.txt](/llms-full.txt).
- **Source & license** — the SDKs are open source (MIT):
  [github.com/depz-ai/depz-sensor-sdk](https://github.com/depz-ai/depz-sensor-sdk).
