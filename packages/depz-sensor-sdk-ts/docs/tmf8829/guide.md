---
title: TMF8829 — guide
description: Hello-world for the TMF8829 multizone dToF sensor in Node and the browser — init and firmware download, presets and config fields, the measurement and its zone layout, histograms and ref-SPAD frames, capture time and flags, recovery, bridge diagnostics, offline decoding, and gotchas.
---

# TMF8829 — guide

Hands-on guide to the `Tmf8829` device class. For what the sensor is and its
concepts, read the [introduction](introduction.md); for discovery, time sync
and the common device features see the [overview](../overview.md); for exact
signatures the [API reference](api.md).

## Contents

- [Open and initialize](#open-and-initialize)
- [Hello-world — Node (depth grid)](#hello-world--node-depth-grid)
- [Hello-world — browser (Web Serial)](#hello-world--browser-web-serial)
- [Presets and config fields](#presets-and-config-fields)
- [The measurement](#the-measurement)
- [Zone layout and orientation](#zone-layout-and-orientation)
- [Histograms and ref-SPAD frames](#histograms-and-ref-spad-frames)
- [Capture time and flags](#capture-time-and-flags)
- [Recovery and bridge diagnostics](#recovery-and-bridge-diagnostics)
- [Decoding a captured stream](#decoding-a-captured-stream)
- [Gotchas](#gotchas)

## Open and initialize

```ts
import { openDevice } from "@depz/sensor-sdk/node";
import { Tmf8829 } from "@depz/sensor-sdk";

const dev = await openDevice();                  // returns a Tmf8829 (USB PID 0xED4C)
if (!(dev instanceof Tmf8829)) throw new Error("not a TMF8829");

const [appVersion, serial] = await dev.init();   // soft reset + 14.4 KB firmware download
console.log(`sensor app ${appVersion}, serial 0x${serial.toString(16)}`);
```

`init()` takes about 0.1–0.15 s: it soft-resets the sensor, checks its ID,
downloads the embedded ams-OSRAM firmware through the sensor bootloader and
starts it. It resolves `[appVersion, sensorSerial]` (also kept on
`dev.appVersion` / `dev.sensorSerial`) and clears the last configuration.
Pass `init((done, total) => …)` to follow the download. A stream that is
running is stopped first.

## Hello-world — Node (depth grid)

```ts
import { openDevice } from "@depz/sensor-sdk/node";
import { Tmf8829 } from "@depz/sensor-sdk";

const dev = await openDevice();
if (!(dev instanceof Tmf8829)) throw new Error("not a TMF8829");

await dev.init();
await dev.configure("16x16", { periodMs: 1 });   // as fast as the sensor goes (~30/s)
await dev.start();
for await (const m of dev.measurements()) {      // runs until you break out
  const centre = (m.rows / 2) * m.cols + m.cols / 2;   // peak 0, middle zone
  console.log(`${m.rows}x${m.cols}  centre ${m.distanceMm![centre]!.toFixed(1)} mm`,
              `snr ${m.results.snr![centre]}`);
}
await dev.stop();
```

`start()` without a prior `configure()` (since the last `init()`) configures
the default `8x8` preset first.

## Hello-world — browser (Web Serial)

```ts
import { openDevice } from "@depz/sensor-sdk/web";
import { Tmf8829 } from "@depz/sensor-sdk";

connectBtn.addEventListener("click", async () => {
  const port = await navigator.serial.requestPort();     // user gesture
  const dev = await openDevice(port);
  if (!(dev instanceof Tmf8829)) throw new Error("not a TMF8829");

  await dev.init();
  await dev.configure("32x32");
  await dev.start();
  const unsub = dev.onMeasurement((m) => {
    if (m.distanceMm) renderDepth(m.distanceMm, m.rows, m.cols);   // keep it quick
  });
});
```

## Presets and config fields

`configure(preset, opts)` loads a preset, writes the fields below into the
config page, reads the page back and resolves the `Tmf8829Config` it holds
(`preset`, `page` — the raw 190 bytes, `fields`, `grid` `[rows, cols]`,
`histograms`, `refFrame`; also kept on `dev.config`). It rejects while
streaming.

| preset | grid (rows × cols) | note |
|---|---|---|
| `8x8` | 8 × 8 | the default |
| `8x8-lr` | 8 × 8 | long range |
| `8x8-ha` | 8 × 8 | high accuracy |
| `8x8-xr` | 8 × 8 | extended range; raw distances in **mm**, not 0.25 mm |
| `16x16`, `16x16-ha` | 16 × 16 | |
| `32x32`, `32x32-ha` | 32 × 32 | two half-frames per measurement |
| `48x32`, `48x32-ha` | 32 × 48 | two half-frames per measurement |

| option | config field | default |
|---|---|---|
| `periodMs` | `period_ms` — pause between measurements | 33 |
| `kiloIterations` | `kilo_iterations` — integration length | the preset's own (586 / 2344 / 3516) |
| `nrPeaks` | `nr_peaks` — peaks per zone, 1..4 | 1 |
| `signalStrength` | `signal_strength` — per-peak signal | `false` |
| `noiseStrength` | `noise_strength` — per-zone noise | `false` |
| `xtalk` | `xtalk` — per-zone crosstalk | `false` |
| `histograms` | `dump_histograms` — raw histogram frames | `false` |
| `refFrame` | `ref_spad_frame` — ref-SPAD frame before each results frame | `false` |
| `fields` | any other page field by name (`power_modes`, `dual_mode`, `distance_in_mm`, …) | — |

```ts
const cfg = await dev.configure("8x8", {
  nrPeaks: 4, signalStrength: true, noiseStrength: true, xtalk: true,
});
cfg.grid;                       // [8, 8]
cfg.fields.kilo_iterations;     // 586
```

A preset load does **not** reset the result format, the period or the
histogram switch on the sensor, so `configure()` writes every one of those
fields on every call — omitted options take the defaults above, never what an
earlier configuration left behind. The sensor refuses a result format too
big for the grid (frame data over 7684 B, e.g. 4 peaks + every strength on
`48x32`): `configure()` rejects with a `Tmf8829Error`
(`ERR_CONFIG_RESULT_SIZE`); it also rejects when a written field does not read
back. The page codecs work offline too: `tmf8829CfgGet(page, "nr_peaks")`,
`tmf8829CfgSet(page, name, value)`, `tmf8829ConfigFields(page)`,
`TMF8829_PRESETS`, `TMF8829_CFG_FIELDS`.

## The measurement

`Tmf8829Measurement` is one complete measurement — one results frame on 8×8
and 16×16, both half-frames merged on 32×32 and 48×32 — with the histogram
and ref-SPAD frames that came before it.

| field | meaning |
|---|---|
| `timestampUs` | capture time, MCU µs (`number`) — centre of the integration window; mean of the two halves |
| `flags` | `TMF8829_FT_*` of its results frames, OR-ed |
| `rows`, `cols`, `fpMode` | grid and focal-plane mode (0 8×8A, 1 8×8B, 2 16×16, 3 32×32, 4 32×32s, 5 48×32) |
| `distanceMm` | clock-corrected mm, flat `[peak][row][col]` (`Float64Array`, `null` without peaks) |
| `results` | the raw sensor integers: `distance` (0.25 mm or mm units, uncorrected), `snr`, `signal`, `noise`, `xtalk` — `null` when not configured |
| `ratioQ16` | clock ratio the distances were corrected with |
| `fNumber` | the sensor's measurement counter |
| `temperatureC` | sensor die temperature, °C |
| `frames`, `times` | per results frame: parsed header/footer (`Tmf8829FrameInfo`) and capture time (`Tmf8829FrameTime`) |
| `histograms`, `refFrames` | raw histogram frames / 65-byte ref-SPAD frames of this measurement |
| `problems` | what went wrong since the previous measurement (bad frame status, dropped halves, …) |

Three ways to consume, as on every DEPZ device:

```ts
const it = dev.measurements(64);                 // iterator — bounded, drop-oldest
const first = (await it.next()).value;
console.log("dropped so far:", it.droppedCount);

const unsub = dev.onMeasurement((m) => queue.push(m));   // callback on the read pump

const m = await dev.getMeasurement(2000);        // next one; DepzTimeoutError on silence
```

## Zone layout and orientation

All per-zone arrays are flat and row-major in the **sensor's** order:
per-peak arrays (`distanceMm`, `results.distance`, `snr`, `signal`) are
`[peak][row][col]`, per-zone ones (`noise`, `xtalk`) `[row][col]`.

```ts
import type { Tmf8829Measurement } from "@depz/sensor-sdk";

const at = (m: Tmf8829Measurement, peak: number, row: number, col: number) =>
  m.distanceMm![(peak * m.rows + row) * m.cols + col]!;
```

The grid is transposed against the scene: for a board held **USB down** and
seen from behind (looking where the sensor looks), the **row** index runs
left → right and the **column** index top → bottom. To get a picture with
`y` top → bottom and `x` left → right, read `at(m, 0, x, y)`. The SDK does not
transpose for you (the DEPZ viewer does, on display).

Raw distance units: 0.25 mm (every preset except `8x8-xr`, which reports mm).
`distanceMm` is already converted and corrected for the sensor clock (~1.5 %
slow); `tmf8829DistanceMm(results, ratio, inMm)` does the same by hand.

## Histograms and ref-SPAD frames

```ts
await dev.configure("8x8", { histograms: true, refFrame: true });
await dev.start();
const m = await dev.getMeasurement();
m.histograms.length;            // 2 per measurement on 8x8/16x16, 8 on 32x32, 12 on 48x32
m.histograms[0]!.raw.length;    // 25 377 B each — raw, header to footer
m.refFrames[0]!.length;         // 65 B: zero 5-byte pre-header + register 0x20 block
```

The SDK groups the frames with their measurement but does not decode the
histogram bins — that is the ams histogram layout, left to the caller.
Histograms are a calibration mode: the stream drops to ~10 measurements/s on
8×8/16×16 and ~1.7/s on 48×32, and on 32×32/48×32 the capture time becomes
uncertain (`T1_UNSURE`, below).

## Capture time and flags

The sensor timestamps each measurement on its own 125 kHz clock; the SDK maps
that onto MCU microseconds through clock pairs the bridge records, so
`timestampUs` is the centre of the actual light integration, not the USB
arrival time. Place it on the host clock with time sync — note it is a
`number`, while `toHostTimeUs()` takes a `bigint`:

```ts
import { TMF8829_FT_STALLED, tmf8829FlagNames } from "@depz/sensor-sdk";

await dev.syncTime(5);
dev.onMeasurement((m) => {
  const hostUs = dev.toHostTimeUs(BigInt(m.timestampUs));
  if (m.flags) console.log(hostUs, tmf8829FlagNames(m.flags));   // e.g. ["nominal"]
  if (m.flags & TMF8829_FT_STALLED) { /* sensor waited for the histogram FIFO */ }
});
```

| flag | meaning |
|---|---|
| `TMF8829_FT_NOMINAL` | the clock ratio is not measured yet (first ~0.2 s of a stream) — nominal 8 µs/tick used |
| `TMF8829_FT_CLOCK_RESET` | the clock model restarted at this frame (power cycle, 32-bit wrap) |
| `TMF8829_FT_EDGE_LATE` | the interrupt edge came > 2 ms later than usual — a plausibility warning |
| `TMF8829_FT_STALLED` | sub-integrations were not back to back (histogram FIFO wait) |
| `TMF8829_FT_T1_UNSURE` | histograms on 32×32/48×32: the centre may be up to ~20 ms late |

The clock model is per stream: it restarts at every `start()`.

## Recovery and bridge diagnostics

```ts
await dev.init();                     // recovery from any sensor state
await dev.configure("8x8");           // init() forgets the configuration

const info = await dev.getBridgeInfo();   // bridge counters, safe mid-stream
info.frames;                          // frames pushed to the host
info.framesAborted;                   // frames cut short by a stop
info.frameErrors;                     // refused frame headers
info.spiTimeouts;
dev.framesDropped;                    // frame_seq gaps seen by the host in this stream
dev.framesDiscarded;                  // frames lost to chunk reassembly
dev.frameParseErrors;
await dev.sensorState();              // raw host-interface registers, for diagnostics
```

There is no sleep, standby or power-mode call: the sensor's enable pin is
wired high on this board. `init()` starts with a soft reset and brings the
sensor back from any state; if it fails, only a USB re-plug (power cycle) is
left. `onInterrupt(cb)` reports bridge events the SDK does not swallow: a
`Tmf8829EventCode.BadHeader` outside `stop()` means the sensor is lost — run
`init()` again. (The one bad header a `stop()` causes mid histogram dump is
expected and swallowed.)

Raw register access stays available for anything the class does not cover:
`readRegisters(addr, length)`, `writeRegisters(addr, data)`, `configPage()`.

## Decoding a captured stream

The pure layer runs without a device — e.g. on `RPT_TMF_FRAME` payloads you
logged. Reassemble chunks, then feed whole frames to a builder in stream
order:

```ts
import {
  Tmf8829FrameReassembler, Tmf8829MeasurementBuilder, unpackTmf8829FrameChunk,
} from "@depz/sensor-sdk";

const reasm = new Tmf8829FrameReassembler();
const builder = new Tmf8829MeasurementBuilder();     // one per stream
for (const payload of framePayloads) {               // RPT_TMF_FRAME (0x93) payloads
  const raw = reasm.feed(unpackTmf8829FrameChunk(payload));
  if (raw === null) continue;                        // frame not complete yet
  const m = builder.add(raw.data, Number(raw.timestampUs), Number(raw.edgeUs));
  if (m) console.log(m.fNumber, m.timestampUs, m.rows, m.cols);
}
```

`parseTmf8829Frame(raw)`, `parseTmf8829Results(raw)` and
`mergeTmf8829Halves(first, second)` are the single steps if you need them.

## Gotchas

- **`init()` before anything** — the sensor runs from RAM firmware the host
  downloads; after a USB re-plug or a failed sequence, `init()` again. It also
  forgets the configuration, so `configure()` after it.
- **`configure()` only while stopped** — it rejects during a stream.
- **Result format vs. grid** — 4 peaks with every strength fits 8×8, not
  48×32; the sensor refuses it with `ERR_CONFIG_RESULT_SIZE`.
- **`8x8-xr` is in mm** — the raw `results.distance` there is mm, everywhere
  else 0.25 mm. `distanceMm` handles both.
- **Transposed grid** — rows run left → right, columns top → bottom (board
  USB down, seen from behind). Transpose to draw the scene.
- **Histograms are slow** — and they make 32×32/48×32 capture times
  uncertain. Use them for calibration, not for live depth.
- **A first measurement can carry `NOMINAL`** — the clock ratio needs ~0.2 s
  of stream; distances in that window are corrected with the nominal ratio.
- **No recording yet** — `DatasetRecorder` has no `tmf8829` record in this
  SDK: an added `Tmf8829` writes no records (the Python SDK records it).
  Collect measurements yourself via `onMeasurement()`.
- **Callbacks run on the read pump** — keep them short and don't `await`
  device methods from inside one; hand work to a queue or use
  `measurements()`.
