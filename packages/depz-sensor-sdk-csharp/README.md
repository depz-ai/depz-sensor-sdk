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
  [VL53L8CX](docs/vl53l8cx/api.md) · [VL53L8CH](docs/vl53l8ch/api.md) ·
  [BNO086](docs/bno086/api.md)).
- **Per sensor** — SR04 ([intro](docs/sr04/introduction.md) ·
  [guide](docs/sr04/guide.md)), VL53L8CX ([intro](docs/vl53l8cx/introduction.md) ·
  [guide](docs/vl53l8cx/guide.md)), VL53L8CH ([intro](docs/vl53l8ch/introduction.md) ·
  [guide](docs/vl53l8ch/guide.md)), BNO086 ([intro](docs/bno086/introduction.md) ·
  [guide](docs/bno086/guide.md)).

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

Verifiable, golden-vector-backed codecs are implemented for all five sensors:

- **SR04** — command encode + data/period/decay decode.
- **VL53L4CD** — register-bridge codecs, result decode, timing/tuning math and init block.
- **VL53L8CX / VL53L8CH** — framing → chunk reassembly → results-frame decode
  (shared), plus the advanced DCI codecs (shared). Exercised end-to-end by a
  recorded `.depzrec` replay.
- **BNO086** — SHTP framing/reassembly, SH-2 control encode, input-report and
  gyro-integrated-RV decode.

Plus the shared foundation: CRC KATs, DEPZ framing encode/decode, USB identity
table + discovery ordering, common commands (time-sync, sync-pin), `.fwdepz`
image parse, and `.depzdata` dataset read.

### Not-yet-implemented extension points

These are deliberately stubbed (clearly-named, throwing/`TODO` stubs) rather than
faked, because they cannot be verified from golden vectors alone:

- **CNH histogram decode (VL53L8CH-specific)** — `Vl53l8Cnh`. The CH-only compact
  histogram block is not decoded yet; it is the CH extension point over the
  shared CX/CH frame decode. No fabricated implementation.
- **Live VL53L8 ULD init/config** — `Vl53l8Uld`. The firmware-download +
  register-bridge (DCI) init sequence only means anything against real silicon
  over the CDC link, so it is out of scope for the decode SDK.

Everything else in the table above is fully implemented and covered.

## License

MIT. Open source — source, byte-level protocol contracts and issue tracker:
<https://github.com/depz-ai/depz-sensor-sdk>.
