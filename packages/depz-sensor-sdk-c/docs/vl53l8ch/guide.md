# VL53L8CH — user guide

Hands-on guide to the VL53L8CH, the CNH superset of the multizone ToF sensor.
For what the sensor is and why CH exists, read the
[introduction](introduction.md); for exact signatures see the
[API reference](api.md).

**The VL53L8CH runs the entire VL53L8CX class.** Opening, `init()`,
configuration, ranging, the frame, thresholds, motion, crosstalk, power modes
and record / replay work exactly as in the
[VL53L8CX user guide](../vl53l8cx/guide.md) — read that first. This page covers
**only what CH adds**: the model, and CNH histograms.

## Contents

- [Open a VL53L8CH](#open-a-vl53l8ch)
- [Configure CNH](#configure-cnh)
- [Decode the histograms from the stream](#decode-the-histograms-from-the-stream)
- [Crosstalk calibration on the CH](#crosstalk-calibration-on-the-ch)
- [Record and replay for tests](#record-and-replay-for-tests)
- [Verified on hardware](#verified-on-hardware)
- [Decode layer (your own transport)](#decode-layer-your-own-transport)
- [Gotchas](#gotchas)

## Open a VL53L8CH

The VL53L8CX and VL53L8CH run the same bridge firmware and report the same
`VL53L8` identity; the production USB PID `0x0483:0xED40` is what marks a CH.
`depz_open_device()` reads it and attaches the class with
`DEPZ_VL53L8_MODEL_L8CH`; everything else gets `DEPZ_VL53L8_MODEL_L8CX`. The
model decides which sensor firmware `init()` downloads (VL53LMZ ULD 2.0.16 for
the CH) — and only the CH firmware produces CNH.

```c
depz_open_options opt = DEPZ_OPEN_OPTIONS_INIT;
depz_device *dev;

if (depz_open_device(&opt, &dev) == DEPZ_OK) {
    if (depz_is_vl53l8(dev) && depz_vl53l8_get_model(dev) == DEPZ_VL53L8_MODEL_L8CH &&
        depz_vl53l8_init(dev, NULL, NULL) == DEPZ_OK) {
        /* configure, arm CNH, range ... */
    }
    depz_device_close(dev);
}
```

When there is no USB PID to go by — a replay, your own link, a board whose
PID was not programmed — name the model yourself with
`depz_vl53l8_open_link(link, DEPZ_VL53L8_MODEL_L8CH, &dev)`. A bare
`depz_device_open_link()` + `depz_device_promote()` gives the CX model — the
base, safe as a default since CH is a superset of it.

## Configure CNH

CNH is set up in four steps, after `init()` and the resolution, before
`start_ranging()`:

```c
depz_vl53l8_cnh_setup cnh;
size_t bytes;

depz_vl53l8_set_resolution(dev, DEPZ_VL53L8_RES_8X8);
depz_vl53l8_set_ranging_frequency_hz(dev, 15);

/* 1. Bins: start at sensor bin 10, 20 CNH bins, 2 sensor bins in each. */
depz_vl53l8_cnh_init_config(&cnh, 10, 20, 2);
/* 2. Aggregates: the 8x8 zones from (0,0) in 2x2 blocks, 4 x 4 of them = 16. */
if (depz_vl53l8_cnh_create_agg_map(&cnh, 64, 0, 0, 2, 2, 4, 4) != DEPZ_OK) { /* ... */ }
/* 3. The sensor-side buffer: 16 x 20 needs 1708 bytes (limit 6160). */
if (depz_vl53l8_cnh_required_memory(&cnh, &bytes) != DEPZ_OK) { /* too big */ }
/* 4. Arm it for the next start_ranging(). */
depz_vl53l8_configure_cnh(dev, &cnh);
```

- **`depz_vl53l8_cnh_init_config(s, start_bin, num_bins, sub_sample)`** —
  resets the setup to ST's defaults (the DEPZ decode's fixed `cnh_cfg`: no
  ping-pong buffer, no variance, ambient level, crosstalk removal, invalid bins
  zeroed, reference residual stored), then sets the bins. A sensor bin is
  37.5348 mm, so CNH bin `k` is centred at
  `(start_bin + k*sub_sample + sub_sample/2.0) * 37.5348` mm — here bin 0 at
  ~413 mm and bin 19 at ~1839 mm.
- **`depz_vl53l8_cnh_create_agg_map(s, resolution, start_x, start_y, merge_x,
  merge_y, cols, rows)`** — assigns zones to aggregates: a `cols` × `rows`
  grid of aggregates, each `merge_x` × `merge_y` zones, starting at zone
  (`start_x`, `start_y`); zones outside stay unmapped. `DEPZ_E_ARG` when the
  grid runs off the zone grid. `resolution` must be the one you range at.
- **`depz_vl53l8_cnh_required_memory()`** — the sensor-side buffer the setup
  needs; `DEPZ_E_ARG` when no map was created or it exceeds
  `DEPZ_VL53L8_CNH_MAX_BYTES` (6160) — fewer aggregates or fewer bins, then.
- **`depz_vl53l8_configure_cnh()`** — checks the size again, writes the
  156-byte configuration (`depz_vl53l8_cnh_pack()` shows the bytes) and adds
  the CNH block to the frame output of the next `start_ranging()` — and of
  every later one, until the next `init()` disarms it. On the CX model it
  fails with `DEPZ_E_WRONG_TYPE`.

The setup struct mirrors ST's `VL53LMZ_Motion_Configuration`; the fields the
helpers do not set (detection threshold, memory-update mode, indicator
formats) are left at 0 by `init_config`, as in the ST plugin, and can be edited
before step 4. CNH and the
[motion indicator](../vl53l8cx/guide.md#motion-indicator) are one pipeline on
this firmware and share that configuration block: configure one or the other.

## Decode the histograms from the stream

With CNH armed, every streamed frame carries the raw block in
`cnh[0 .. cnh_len)` next to the usual depth frame `f`.
`depz_vl53l8ch_decode_cnh()` turns it into integer histograms, given the
aggregate count and bins of the setup — `depz_vl53l8_cnh_decode_config()`
takes them from it. The decoded struct is ~82 KB: allocate it on the heap.

```c
static depz_vl53l8_live_frame f;
depz_vl53l8ch_cnh_config dc;
depz_vl53l8ch_cnh_frame *h = malloc(sizeof *h);
depz_stream *s = depz_vl53l8_frames(dev, 4);
int k;

depz_vl53l8_cnh_decode_config(&cnh, &dc);           /* 16 aggregates x 20 bins */
depz_vl53l8_start_ranging(dev);
if (h && depz_stream_next(s, &f, 1000) == DEPZ_OK && f.cnh_len > 0 &&
    depz_vl53l8ch_decode_cnh(&dc, f.cnh, f.cnh_len, h) == 0) {
    for (k = 0; k < h->feature_length; k++)          /* aggregate 0 */
        printf("%7.1f mm  %10.2f\n", (10 + k * 2 + 1.0) * 37.5348,
               ldexp((double)h->hist_raw[0][k], -h->hist_scaler[0][k]));
    printf("reference residual %.3f\n", h->ref_residual_word / 2048.0);
}
depz_vl53l8_stop_ranging(dev);
depz_stream_close(s);
free(h);
```

A bin's value is `hist_raw / 2^hist_scaler`. Only the first `nb_aggregates`
rows and `feature_length` columns are valid. `depz_vl53l8ch_decode_cnh()`
returns `-1` for a config out of range (aggregates `1..64`, bins `1..255`) and
`-2` / `-3` when the block is too short for its header or for the layout the
config implies — so decode with the setup the sensor actually runs. The CNH
block rides inside the normal frame — no extra requests, just a bigger frame:
at 16 aggregates × 20 bins every frame is 1708 bytes bigger, and 15 Hz streamed
with none dropped on the verified board.

## Crosstalk calibration on the CH

`depz_vl53l8_calibrate_xtalk()` works as in the
[CX guide](../vl53l8cx/guide.md#crosstalk-margin-calibration-save-and-restore),
but the calibration table must match the firmware: the SDK writes the VL53LMZ
table for the CH model (with the VL53L8CX table the CH firmware silently skips
the run). On the verified board, with no cover glass, the firmware correctly
answers "nothing to calibrate": the call returns `DEPZ_OK` with
`*failed == true` and the default crosstalk data stays.

## Record and replay for tests

Recording and strict replay work as in the
[CX guide](../vl53l8cx/guide.md#record-and-replay-for-tests) — open the replay
with `depz_vl53l8_open_link(link, DEPZ_VL53L8_MODEL_L8CH, &dev)`, since the
model decides which firmware blob is written. The test suite replays two real
VL53L8CH captures strictly (lab board `TMNQ8E3PRR`, `APP_VL53L8_v0.92`,
recorded by the Python SDK):

- `io_l8ch_replay` — `vl53l8ch_8x8_15hz_3s.depzrec`: `init()` with the CH
  firmware download, 8×8 at 15 Hz, 45 frames;
- `io_l8ch_cnh_replay` — `vl53l8ch_cnh_8x8_15hz.depzrec`: the same with CNH
  armed (start bin 10, 20 bins, sub-sample 2, 16 aggregates of 2×2 zones), 30
  frames, each carrying its 1708-byte CNH block byte for byte.

The same captures also run through the decode layer alone
(`vec_vl53l7_replay_vl53l8ch_*`).

## Verified on hardware

The class was run against the lab VL53L8CH (`TMNQ8E3PRR`, firmware
`APP_VL53L8_v0.92`) facing a wall at about 0.6 m: `init()` in 0.76 s with the
CH firmware, 8×8 at 15 Hz with distances ~599–602 mm, CNH armed with a
1708-byte block in every frame decoding fine, 30 frames in 2 s none dropped,
4×4 at 30 Hz, sleep and wake-up, configuration refused while ranging and 1 Hz
refused. The full list is in the
[CX guide](../vl53l8cx/guide.md#verified-on-hardware).

## Decode layer (your own transport)

If you own the transport, the CH needs two things beyond the
[CX decode layer](../vl53l8cx/guide.md#decode-layer-your-own-transport).

### Decoding CH frames

A CH device's frames carry the same blocks as CX frames, but the CH firmware
(VL53LMZ) puts the footer id 4 bytes from the end instead of 12. Use the CH
decoder — the CX one rejects CH frames with -1:

```c
static depz_vl53l8_reassembler reasm;       /* depz_vl53l8_reasm_init() once */
static uint8_t cnh[DEPZ_VL53L8_STREAM_TOTAL_MAX];

/* in your RPT_FRAME handler (as in the CX guide): */
static void on_frame_chunk(const depz_event *ev)
{
    depz_vl53l8_chunk chunk;
    const uint8_t *raw;
    size_t raw_len, cnh_len = 0;
    uint64_t ts;
    depz_vl53l8_frame f;

    if (depz_vl53l8_unpack_chunk(ev->payload, ev->payload_len, &chunk) != 0) return;
    if (depz_vl53l8_reasm_feed(&reasm, &chunk, &raw, &raw_len, &ts) == 1 &&
        depz_vl53l8ch_decode_frame(raw, raw_len, ts, &f, cnh, sizeof cnh, &cnh_len) == 0) {
        /* f: the depth image; cnh[0..cnh_len): the CNH block, if armed */
    }
}
```

`depz_vl53l8ch_decode_frame()` walks the blocks exactly like the CX decoder;
only the footer check differs. It also copies out the CNH block when the frame
carries one (`cnh_len` 0 otherwise), and returns `-3` when it exceeds your
buffer.

### Decode CNH histograms

`depz_vl53l8ch_decode_cnh()` needs only the aggregate count and bins the
sensor was configured with — fill `depz_vl53l8ch_cnh_config` yourself when you
did not use `depz_vl53l8_cnh_setup`:

```c
static void print_cnh(const uint8_t *block, size_t block_len)
{
    depz_vl53l8ch_cnh_config cfg = { .nb_of_aggregates = 16, .feature_length = 20 };
    depz_vl53l8ch_cnh_frame *h = malloc(sizeof *h);
    if (h && depz_vl53l8ch_decode_cnh(&cfg, block, block_len, h) == 0)
        printf("%d aggregates x %d bins, aggregate 0 bin 0 = %.2f\n",
               h->nb_aggregates, h->feature_length,
               ldexp((double)h->hist_raw[0][0], -h->hist_scaler[0][0]));
    free(h);
}
```

The same decode serves the [VL53L7CH](../vl53l7ch/guide.md), whose frames
(`depz_vl53l7_decode_frame()`) carry the CNH block the same way.

## Gotchas

- **The model comes from the USB PID.** Without it (replay, custom link,
  `promote()` on a bare link) you get the CX model — and no CNH. Name the model
  with `depz_vl53l8_open_link()` when you know better.
- **Arm CNH after `init()` and the resolution, before `start_ranging()`** —
  the aggregate map is built for one resolution, and `init()` disarms CNH: a
  fresh sensor holds no CNH configuration.
- **CNH and the motion indicator share one configuration block** on this
  firmware: configure one or the other.
- **Decode with the setup the sensor runs.** `depz_vl53l8_cnh_decode_config()`
  of the very setup you armed is the safe source.
- **The decoded histogram struct is ~82 KB** — heap, not stack.
- **CH frames need the CH decoder** in the decode layer —
  `depz_vl53l8ch_decode_frame`, footer id at size−4. The live class picks it
  for you.
- All the [CX gotchas](../vl53l8cx/guide.md#gotchas) apply here too.
