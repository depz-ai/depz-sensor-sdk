# VL53L7CH — user guide

The VL53L7CH is the [VL53L7CX](../vl53l7cx/guide.md) board plus CNH
histograms, on the same live multizone class. Opening, `init()`, the L5/L7
differences, the board commands and record / replay are on the VL53L7CX
guide; configuring and streaming on the [VL53L8CX guide](../vl53l8cx/guide.md);
the CNH setup in full on the [VL53L8CH guide](../vl53l8ch/guide.md#configure-cnh),
whose calls this page uses unchanged. For concepts see the
[introduction](introduction.md); for signatures the [API reference](api.md).

## Contents

- [Open a VL53L7CH](#open-a-vl53l7ch)
- [Arm CNH and decode it from the stream](#arm-cnh-and-decode-it-from-the-stream)
- [Record and replay for tests](#record-and-replay-for-tests)
- [Decode layer (your own transport)](#decode-layer-your-own-transport)
- [Gotchas](#gotchas)

## Open a VL53L7CH

`depz_open_device()` resolves the model: the PID `0xED4A`, or a device name
with `VL53L7CH` in it, gives `DEPZ_VL53L8_MODEL_L7CH`. Without either — a board
whose PID and name were not programmed — it falls back to VL53L7CX, which
ranges on the CH silicon but has no CNH; name the model yourself with
`depz_vl53l8_open_link(link, DEPZ_VL53L8_MODEL_L7CH, &dev)` then.
`depz_vl53l8_module_type()` reads 1 (MZEVO) after `init()` on both the L7CX
and the L7CH — it cannot tell them apart.

## Arm CNH and decode it from the stream

After `init()` and the resolution, before `start_ranging()` — as on the
VL53L8CH:

```c
static depz_vl53l8_live_frame f;
depz_vl53l8_cnh_setup cnh;
depz_vl53l8ch_cnh_config dc;
depz_vl53l8ch_cnh_frame *h = malloc(sizeof *h);      /* ~82 KB: heap */
size_t bytes;

depz_vl53l8_set_resolution(dev, DEPZ_VL53L8_RES_8X8);
depz_vl53l8_set_ranging_frequency_hz(dev, 15);
depz_vl53l8_cnh_init_config(&cnh, 10, 20, 2);        /* start bin 10, 20 bins of 2 */
depz_vl53l8_cnh_create_agg_map(&cnh, 64, 0, 0, 2, 2, 4, 4);  /* 16 aggregates */
if (depz_vl53l8_cnh_required_memory(&cnh, &bytes) == DEPZ_OK &&  /* 1708 B */
    depz_vl53l8_configure_cnh(dev, &cnh) == DEPZ_OK) {
    depz_vl53l8_cnh_decode_config(&cnh, &dc);
    depz_vl53l8_start_ranging(dev);
    if (h && depz_vl53l8_get_frame(dev, -1, &f) == DEPZ_OK && f.cnh_len > 0 &&
        depz_vl53l8ch_decode_cnh(&dc, f.cnh, f.cnh_len, h) == 0)
        printf("aggregate 0, bin 3 (%.0f mm): %.2f\n", (10 + 3 * 2 + 1.0) * 37.5348,
               ldexp((double)h->hist_raw[0][3], -h->hist_scaler[0][3]));
    depz_vl53l8_stop_ranging(dev);
}
free(h);
```

The setup, the aggregate map, the buffer limit (6160 bytes) and the bin
arithmetic are explained in the
[VL53L8CH guide](../vl53l8ch/guide.md#configure-cnh). `init()` disarms CNH (a
fresh sensor holds no CNH configuration): after a re-init — for example after
a [soft cycle](../vl53l7cx/guide.md#board-commands) — configure it again. On
the L5CX / L7CX models `configure_cnh()` fails with `DEPZ_E_WRONG_TYPE`.

## Record and replay for tests

The test suite replays two real VL53L7CH captures strictly through the class
(`APP_VL53L7_v0.53`, recorded by the Python SDK; `depz_device_promote()`
resolves L7CH from the captured device name, the module type reads 1):

- `io_l7ch_replay` — `vl53l7ch_8x8_15hz_3s.depzrec`: `init()` with the
  VL53LMZ firmware download, 8×8 at 15 Hz, 45 frames;
- `io_l7ch_cnh_replay` — `vl53l7ch_cnh_8x8_15hz.depzrec`: the same with CNH
  armed (start bin 10, 20 bins, sub-sample 2, 16 aggregates of 2×2 zones), 20
  frames, each carrying its 1708-byte CNH block byte for byte.

Recording your own works as in the
[VL53L8CX guide](../vl53l8cx/guide.md#record-and-replay-for-tests).

## Decode layer (your own transport)

If you own the transport, the ranging frame decodes as on the
[VL53L7CX](../vl53l7cx/guide.md#decode-layer-your-own-transport); the CNH
block comes out of the same decoder.

### Frames with a CNH block

With CNH configured, every streamed frame carries the usual ranging results
**and** one CNH data block. A frame is larger than one chunk (3156 B for
16 aggregates × 20 bins, up to ~7.6 KB); the VL53L8 reassembler takes up to
`DEPZ_VL53L8_STREAM_TOTAL_MAX` (8192) bytes, so it rebuilds these frames as
they are. Pass `depz_vl53l7_decode_frame()` a buffer and it copies the CNH
block out next to the ranging results:

```c
/* raw / raw_len / ts: one frame completed by depz_vl53l8_reasm_feed() */
static uint8_t cnh_block[DEPZ_VL53L8_STREAM_TOTAL_MAX];

size_t decode_with_cnh(const uint8_t *raw, size_t raw_len, uint64_t ts,
                       depz_vl53l8_frame *f)
{
    size_t cnh_len = 0;
    int rc = depz_vl53l7_decode_frame(raw, raw_len, ts, f,
                                      cnh_block, sizeof cnh_block, &cnh_len);
    if (rc != 0)
        return 0;        /* -1 corrupt, -2 bad length, -3 CNH block > buffer */
    return cnh_len;      /* 0 when the frame carries no CNH block */
}
```

The ranging part of the frame is decoded exactly as on the VL53L7CX (footer
at `size − 4`, per-zone arrays trimmed to the resolution). The CNH bytes are
already in decode order — word-swapped like every results block. (The live class does this for you and fills `cnh[]` / `cnh_len` of each `depz_vl53l8_live_frame`.)

### Decode the histograms

`depz_vl53l8ch_decode_cnh()` is the VL53L8CH decoder; it needs the aggregate
count and the bins per aggregate the sensor was configured with (the CNH
setup the full driver sent). The output struct is ~82 KB — allocate it on
the heap:

```c
#include <stdio.h>
#include <stdlib.h>

/* the configuration the sensor runs, e.g. 16 aggregates x 20 bins */
void print_histograms(const uint8_t *cnh_block, size_t cnh_len)
{
    depz_vl53l8ch_cnh_config cfg = { .nb_of_aggregates = 16, .feature_length = 20 };
    depz_vl53l8ch_cnh_frame *h = malloc(sizeof *h);
    if (!h)
        return;
    if (depz_vl53l8ch_decode_cnh(&cfg, cnh_block, cnh_len, h) == 0) {
        printf("ref residual %.3f\n", h->ref_residual_word / 2048.0);
        for (int a = 0; a < h->nb_aggregates; a++) {
            printf("aggregate %2d:", a);
            for (int bin = 0; bin < h->feature_length; bin++) {
                double v = ldexp((double)h->hist_raw[a][bin], -h->hist_scaler[a][bin]);
                printf(" %.1f", v);   /* hist_raw / 2^hist_scaler */
            }
            printf("\n");
        }
    }
    free(h);
}
```

`depz_vl53l8ch_decode_cnh()` returns `0` on success, `-1` when the config is
out of range (aggregates `1..64`, bins `1..255`), and `-2` / `-3` when the
block is too short for its header or for the layout the config implies — the
usual sign of a config that does not match the sensor's.

## Gotchas

- **The model comes from the PID or the device name** — a board with neither
  opens as a VL53L7CX: it ranges, but without CNH. `module_type` cannot tell
  L7CX from L7CH.
- **Re-arm CNH after every `init()`** — a soft cycle or `LPN_OFF` needs
  `init()`, and `init()` disarms CNH.
- **Decode with the config the sensor runs** — a CNH block does not say how
  many aggregates or bins it holds; use `depz_vl53l8_cnh_decode_config()` of
  the setup you armed.
- **Heap-allocate `depz_vl53l8ch_cnh_frame`** — it is ~82 KB.
- **Decode layer:** size the CNH buffer for the whole frame
  (`DEPZ_VL53L8_STREAM_TOTAL_MAX` is always enough); a smaller one makes the
  frame decode return `-3`.
- All the [VL53L7CX gotchas](../vl53l7cx/guide.md#gotchas) apply.
