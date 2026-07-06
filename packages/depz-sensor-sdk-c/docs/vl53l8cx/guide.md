# VL53L8CX — user guide

Hands-on guide to the VL53L8CX decode path. For what the sensor is, CX vs CH, and
the concepts, read the [introduction](introduction.md); for exact signatures see
the [API reference](api.md). The **VL53L8CH** superset (CNH histograms) has its
[own guide](../vl53l8ch/guide.md) — it shares everything below.

## Contents

- [The streaming pipeline](#the-streaming-pipeline)
- [Reassemble and decode a frame](#reassemble-and-decode-a-frame)
- [The frame: fields and the grid](#the-frame-fields-and-the-grid)
- [Advanced DCI codecs](#advanced-dci-codecs)
- [What is not covered](#what-is-not-covered)
- [Gotchas](#gotchas)

## The streaming pipeline

A ranging frame is larger than one bridge packet, so the device ships it as a run
of `RPT_FRAME` (`0x93`) chunks that you reassemble, then decode:

```
RPT_FRAME packet  ──►  depz_vl53l8_unpack_chunk()  ──►  depz_vl53l8_chunk
                                                              │
                       depz_vl53l8_reasm_feed()  ◄───────────┘
                             │  (returns 1 when a whole frame completes)
                             ▼
                       depz_vl53l8_decode_frame()  ──►  depz_vl53l8_frame
```

Register reads (`RPT_REG_DATA`, `0x91`) — used by the live ULD driver — decode
separately with `depz_vl53l8_unpack_reg_data()`.

## Reassemble and decode a frame

Keep one `depz_vl53l8_reassembler` for the stream and feed each chunk into it:

```c
#include <depz_sensor_sdk.h>

static depz_vl53l8_reassembler reasm;   // init once with depz_vl53l8_reasm_init

static void on_event(const depz_event *ev, void *user) {
    if (ev->type != DEPZ_EV_PACKET || ev->cmd != DEPZ_VL53L8_RPT_FRAME) return;

    depz_vl53l8_chunk chunk;
    if (depz_vl53l8_unpack_chunk(ev->payload, ev->payload_len, &chunk) != 0)
        return;

    const uint8_t *raw; size_t raw_len; uint64_t ts;
    if (depz_vl53l8_reasm_feed(&reasm, &chunk, &raw, &raw_len, &ts) == 1) {
        depz_vl53l8_frame f;
        if (depz_vl53l8_decode_frame(raw, raw_len, ts, &f) == 0) {
            printf("res=%d  temp=%d C  zone0=%d mm\n",
                   f.resolution, f.silicon_temp_degc, f.distance_mm[0]);
        }
    }
}
```

`depz_vl53l8_reasm_feed()` returns `1` only when a frame completes (a gap discards
the frame in progress and bumps `reasm.discarded`). `depz_vl53l8_decode_frame()`
returns `0` on success, `-1` on a corrupt frame (header/footer id mismatch), `-2`
on a bad length. Start the stream on the device with
`depz_vl53l8_pack_start_stream()` + `DEPZ_VL53L8_CMD_START_STREAM`.

## The frame: fields and the grid

`depz_vl53l8_frame` carries every per-zone output as a flat array valid for
`[0, resolution)`, plus the per-frame silicon temperature. Values are the **raw
wire integers** — apply the scaling yourself:

| field | type | meaning |
|---|---|---|
| `distance_mm[z]` | int32 | per-zone distance, mm (already `raw/4`, floored) |
| `target_status[z]` | uint8 | 5/9 = valid, 255 = no target |
| `nb_target_detected[z]` | uint8 | targets found in the zone |
| `signal_per_spad[z]` | uint32 | signal rate, kcps/SPAD (raw) |
| `ambient_per_spad[z]` | uint32 | ambient rate, kcps/SPAD (raw) |
| `nb_spads_enabled[z]` | uint32 | SPADs enabled in the zone |
| `range_sigma_mm_raw[z]` | uint16 | range std-dev; mm = `raw/128` |
| `reflectance[z]` | uint8 | estimated reflectance, % |
| `silicon_temp_degc` | int8 | per-frame sensor temperature, °C |

Zones run **row-major**, so index `z = row*cols + col` where `cols` is 4 (16
zones) or 8 (64 zones):

```c
int cols = (f.resolution == DEPZ_VL53L8_RES_8X8) ? 8 : 4;
for (int row = 0; row < cols; row++) {
    for (int col = 0; col < cols; col++) {
        int z = row * cols + col;
        int mm = (f.target_status[z] == 255) ? -1 : f.distance_mm[z];
        printf("%6d", mm);          // -1 = no target
    }
    printf("\n");
}
```

## Advanced DCI codecs

These are the pure encode/decode halves of the ST ULD advanced features (UM3109),
shared by CX and CH. They build the DCI payloads; the live register write is part
of the [ULD extension point](../guide.md#extension-points).

```c
// crosstalk margin (kcps/SPAD) <-> raw DCI value (raw = round(kcps * 2048))
uint32_t raw = depz_vl53l8_xtalk_margin_to_raw(50.0);
double   kcps = depz_vl53l8_xtalk_margin_from_raw(raw);

// per-zone detection thresholds -> the 768-byte DCI_DET_THRESH_START block
depz_vl53l8_threshold th = {
    .low_thresh = 200, .high_thresh = 600,
    .measurement = DEPZ_VL53L8_DIST_MM, .type = 0,
    .zone_num = 128, .operation = 0,        // in-window, all zones
};
uint8_t start[DEPZ_VL53L8_THRESH_START_SIZE], valid[8];
depz_vl53l8_pack_thresholds(&th, 1, start, valid);

// the default 156-byte motion configuration for a resolution
uint8_t motion[DEPZ_VL53L8_MOTION_CFG_SIZE];
depz_vl53l8_motion_cfg_default_pack(DEPZ_VL53L8_RES_8X8, motion);
```

Threshold `measurement` selectors are the `DEPZ_VL53L8_*` macros
(`DIST_MM`, `SIGNAL_PER_SPAD_KCPS`, `RANGE_SIGMA_MM`, …); `low`/`high` are scaled
by the entry's selector inside `depz_vl53l8_pack_thresholds`.

## What is not covered

- **Live ULD init/config** — the firmware-download and DCI register sequences
  that bring the sensor up are hardware-dependent and out of scope. This SDK
  ships the codec halves and the frame decode, not the live driver.
- **CNH decode** — the CH-only histogram frame is a distinct layout, left as an
  extension point (see the [VL53L8CH guide](../vl53l8ch/guide.md)).

## Gotchas

- **Values are raw fixed-point** — apply `range_sigma_mm = raw/128`, read
  signal/ambient as kcps/SPAD. Only `distance_mm` is pre-scaled (`raw/4`).
- **Check the decode return** — `-1` means a corrupt frame (header/footer id
  mismatch), `-2` a bad length; don't read `out` unless it returned `0`.
- **A gap discards the in-progress frame** — `depz_vl53l8_reasm_feed` returns `0`
  and bumps `discarded`; the next `offset==0` chunk starts fresh.
- **Zones are row-major and only `[0, resolution)` are valid** — don't read past
  the active zone count (16 or 64).
- **`target_status == 255` means no target** — mask those zones before trusting
  `distance_mm`.
