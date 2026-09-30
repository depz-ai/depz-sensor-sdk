# Depz.Sensor (C# SDK)

A contract-first decode/encode SDK for the DEPZ sensor family. Every codec is a
byte-exact port of the reference Python SDK, pinned to the shared golden vectors
under `contracts/vectors` (`dotnet test`).

## Documentation

Full docs live under [`docs/`](docs/):

- **[Common guide](docs/guide.md)** — what it is, install, discovery, the mental
  model, transport/CRC, datasets, firmware parse, testing, extension points.
- **[API reference](docs/api.md)** — every public type, generated from the `///`
  summaries (per-sensor: [SR04](docs/sr04/api.md) ·
  [VL53L4CD](docs/vl53l4cd/api.md) · [VL53L8CX](docs/vl53l8cx/api.md) ·
  [VL53L8CH](docs/vl53l8ch/api.md) · [VL53L5CX](docs/vl53l5cx/api.md) ·
  [VL53L7CX](docs/vl53l7cx/api.md) · [VL53L7CH](docs/vl53l7ch/api.md) ·
  [VL53L0X](docs/vl53l0x/api.md) · [VL53L1CX](docs/vl53l1cx/api.md) ·
  [VL53L1CB](docs/vl53l1cb/api.md) · [VL53L3CX](docs/vl53l3cx/api.md) ·
  [VL53L4CX](docs/vl53l4cx/api.md) · [BNO086](docs/bno086/api.md) ·
  [BNO055](docs/bno055/api.md)).
- **Per sensor** — SR04 ([intro](docs/sr04/introduction.md) ·
  [guide](docs/sr04/guide.md)), VL53L8CX ([intro](docs/vl53l8cx/introduction.md) ·
  [guide](docs/vl53l8cx/guide.md)), VL53L8CH ([intro](docs/vl53l8ch/introduction.md) ·
  [guide](docs/vl53l8ch/guide.md)), BNO086 ([intro](docs/bno086/introduction.md) ·
  [guide](docs/bno086/guide.md)), VL53L4CD ([intro](docs/vl53l4cd/introduction.md) ·
  [guide](docs/vl53l4cd/guide.md)), VL53L5CX ([intro](docs/vl53l5cx/introduction.md) ·
  [guide](docs/vl53l5cx/guide.md)), VL53L7CX ([intro](docs/vl53l7cx/introduction.md) ·
  [guide](docs/vl53l7cx/guide.md)), VL53L7CH ([intro](docs/vl53l7ch/introduction.md) ·
  [guide](docs/vl53l7ch/guide.md)), VL53L0X ([intro](docs/vl53l0x/introduction.md) ·
  [guide](docs/vl53l0x/guide.md)), VL53L1CX ([intro](docs/vl53l1cx/introduction.md) ·
  [guide](docs/vl53l1cx/guide.md)), VL53L1CB ([intro](docs/vl53l1cb/introduction.md) ·
  [guide](docs/vl53l1cb/guide.md)), VL53L3CX ([intro](docs/vl53l3cx/introduction.md) ·
  [guide](docs/vl53l3cx/guide.md)), VL53L4CX ([intro](docs/vl53l4cx/introduction.md) ·
  [guide](docs/vl53l4cx/guide.md)), BNO055 ([intro](docs/bno055/introduction.md) ·
  [guide](docs/bno055/guide.md)).

Regenerate the API reference after changing source `///` summaries:

```bash
python3 scripts/gen_api_md.py    # from packages/depz-sensor-sdk-csharp
```

## Sensors

The DEPZ line exposes **four** distinct user-facing sensor types. The ToF is two
separate silicon variants that share one wire/frame layout:

| Sensor        | What it is                              | USB product id            | SDK surface |
|---------------|-----------------------------------------|---------------------------|-------------|
| **SR04**      | HC-SR04 ultrasonic ranger               | `0xEC78`                  | `Depz.Sensor.Protocol.Sr04` |
| **VL53L8CX**  | Base ToF multizone ranger               | dev-default (ST `0483:56DC`) | `Depz.Sensor.Vl53l8` |
| **VL53L8CH**  | CX **plus** CNH histograms; own PID      | `0xED40`                  | `Depz.Sensor.Vl53l8` |
| **BNO086**    | 9-axis IMU / sensor-hub (SH-2 over SHTP) | `0xEE08`                  | `Depz.Sensor.Bno086` |
| **VL53L5CX / VL53L7CX / VL53L7CH** | 8×8 multizone ToF on one I2C board firmware (63° / 90° / 90° + CNH) | `0xED48` / `0xED49` / `0xED4A` | `Depz.Sensor.Vl53l7` |
| **VL53L0X / L1CX / L1CB / L3CX / L4CX** | The 1D ToF family on one I2C register-bridge firmware | `0xED41` / `0xED43` / `0xED42` / `0xED44` / `0xED46` | `Depz.Sensor.Vl53lx` |
| **BNO055**    | 9-axis IMU, fusion on the chip, I2C register bridge | `0xEE0A`          | `Depz.Sensor.Bno055` |

### VL53L8CX vs VL53L8CH

`VL53L8CX` is the **base** ToF and the development default (it enumerates under
the raw ST VID/PID until reprogrammed). `VL53L8CH` is a **superset**: same
ranging engine plus **CNH** (compact new histograms), and it ships with its own
production USB PID (`0xED40`, hinted `vl53l8ch`).

Because both variants stream the **same ULD results-frame layout**, a single
decode path serves both:

- `Vl53l8FrameDecoder` — results-frame decode (distance/status/signal/…), shared
  by CX and CH. The only variant-visible difference is the frame-id footer
  offset (CX FW / ULD 2.1.0 = 12 bytes from end; CH FW / VL53LMZ 2.0.16 = 4).
  Select with `Vl53l8FrameDecoder.ForVariant(Vl53l8Variant.Cx | .Ch)`.
- `Vl53l8Advanced` / `MotionConfig` — advanced DCI codecs (motion-indicator
  config, detection thresholds, xtalk margin). Shared by CX and CH.

## Coverage today

Verifiable, golden-vector-backed codecs are implemented for every sensor:

- **SR04** — command encode + data/period/decay decode.
- **VL53L4CD** — register-bridge codecs, result decode, timing/tuning math and init block.
- **VL53L8CX / VL53L8CH** — framing → chunk reassembly → results-frame decode
  (shared), plus the advanced DCI codecs (shared). Exercised end-to-end by a
  recorded `.depzrec` replay.
- **BNO086** — SHTP framing/reassembly, SH-2 control encode, input-report and
  gyro-integrated-RV decode.
- **VL53L8CH / VL53L7CH CNH** — `Vl53l8Cnh.DecodeHistogram` unpacks the raw CNH
  block into per-aggregate histograms.
- **VL53L5CX / VL53L7CX / VL53L7CH** — pin control / bus speed / info codecs,
  class resolution, and the L5/L7 frame decoder (footer at size − 4, per-zone
  trim), replayed against live-board captures.
- **VL53L0X / L1CX / L1CB / L3CX / L4CX** — v2.00 codecs, the product table and
  class resolution, die-block / VL53L0X raw / histogram-block decode.
- **BNO055** — wire codecs, units / calibration status and profile / axis remap
  + placements / page-1 configs, register-window decode.

For the VL53L5CX / L7CX / L7CH, the 1D family and the BNO055, initialising and
streaming the board is done with the Python or TypeScript SDK; this SDK
decodes what they send and builds the command payloads.

Plus the shared foundation: CRC KATs, DEPZ framing encode/decode, USB identity
table + discovery ordering, common commands (time-sync, sync-pin), `.fwdepz`
image parse, and `.depzdata` dataset read.

### Not-yet-implemented extension points

These are deliberately stubbed (clearly-named, throwing/`TODO` stubs) rather than
faked, because they cannot be verified from golden vectors alone:

- **Live VL53L8 ULD init/config** — `Vl53l8Uld`. The firmware-download +
  register-bridge (DCI) init sequence only means anything against real silicon
  over the CDC link, so it is out of scope for the decode SDK.
- **Live drivers of the L5/L7 boards, the 1D family and the BNO055** — firmware
  download and ULD configuration, the ST 1D drivers, the BNO055 session logic.

Everything else in the table above is fully implemented and covered.

## License

MIT. Open source — source and issue tracker:
<https://github.com/depz-ai/depz-sensor-sdk>.
