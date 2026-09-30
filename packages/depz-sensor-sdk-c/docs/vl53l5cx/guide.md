# VL53L5CX — user guide

The VL53L5CX shares its board, its sensor firmware, the live class and every
codec with the VL53L7CX: the [VL53L7CX guide](../vl53l7cx/guide.md) — open,
the L5/L7 differences from the VL53L8, board commands, record and replay, the
decode layer — applies line for line, and the
[VL53L8CX guide](../vl53l8cx/guide.md) covers the class itself. This page
shows the places the L5CX appears by name. For concepts see the
[introduction](introduction.md); for signatures the [API reference](api.md).

## Open it and check the silicon

```c
#include <depz_sensor_io.h>
#include <stdio.h>

/* dev: opened with depz_open_device(); the board enumerates on PID 0xED48 */
int check_l5(depz_device *dev)
{
    if (depz_vl53l8_get_model(dev) != DEPZ_VL53L8_MODEL_L5CX) {
        fprintf(stderr, "not a VL53L5CX board\n");
        return -1;
    }
    if (depz_vl53l8_init(dev, NULL, NULL) != DEPZ_OK) {    /* ~1.3 s */
        fprintf(stderr, "init: %s\n", depz_last_error());
        return -1;
    }
    if (depz_vl53l8_module_type(dev) != 0)                  /* 0 = MZ = L5 */
        fprintf(stderr, "board says L5CX, the silicon says L7\n");
    return 0;
}
```

`depz_open_device()` picks `DEPZ_VL53L8_MODEL_L5CX` for the production PID
`0xED48`; without it, the first `VL53L5CX` / `VL53L7CX` / `VL53L7CH` in the
device name decides (also for `depz_device_promote()` on a bare link). To
choose by hand: `depz_vl53l8_open_link(link, DEPZ_VL53L8_MODEL_L5CX, &dev)`.
The L5CX and L7CX download the same sensor firmware, so a wrong guess between
them still ranges — `module_type` is the check.

## Range

Exactly as on the VL53L7CX and VL53L8 — the same class, the same frame:

```c
static depz_vl53l8_live_frame f;
int side, row, col;

depz_vl53l8_set_resolution(dev, DEPZ_VL53L8_RES_4X4);
depz_vl53l8_set_ranging_frequency_hz(dev, 1);        /* 1 Hz works on L5/L7 */
depz_vl53l8_start_ranging(dev);
if (depz_vl53l8_get_frame(dev, 3000, &f) == DEPZ_OK) {
    side = f.f.resolution == DEPZ_VL53L8_RES_8X8 ? 8 : 4;
    for (row = 0; row < side; row++) {
        for (col = 0; col < side; col++)
            printf("%5d", (int)f.f.distance_mm[row * side + col]);
        printf("\n");
    }
}
depz_vl53l8_stop_ranging(dev);
```

## Decode layer: recognise the board and decode its frames

If you own the transport, the class-resolution rule and the frame decoder are
the VL53L7CX ones:

```c
#include <stdio.h>

/* vid/pid from your port enumeration; device_name from GET_DEVICE_NAME */
bool is_vl53l5cx(int vid, int pid, const char *device_name)
{
    depz_vl53l7_model m =
        depz_vl53l7_resolve_model(depz_usb_model_hint(vid, pid), device_name);
    return m == DEPZ_VL53L7_MODEL_L5CX;
}

/* raw / raw_len / ts: one frame completed by depz_vl53l8_reasm_feed() */
void print_l5_frame(const uint8_t *raw, size_t raw_len, uint64_t ts)
{
    depz_vl53l8_frame f;
    if (depz_vl53l7_decode_frame(raw, raw_len, ts, &f, NULL, 0, NULL) != 0)
        return;
    int side = f.resolution == DEPZ_VL53L8_RES_8X8 ? 8 : 4;
    for (int row = 0; row < side; row++) {
        for (int col = 0; col < side; col++)
            printf("%5d", (int)f.distance_mm[row * side + col]);
        printf("\n");
    }
}
```

## Record and replay

The test suite replays two real VL53L5CX captures strictly through the class
(`io_l5cx_replay`: 8×8 at 15 Hz, 45 frames; `io_l5cx_4x4_replay`: 4×4 at
15 Hz, 20 frames) — `depz_device_promote()` resolves the model from the
captured device name, `init()` downloads the firmware, and the module type
reads 0. See the [VL53L7CX guide](../vl53l7cx/guide.md#record-and-replay-for-tests).

## Gotchas

- **Peel the protective film off the lens** before measuring — with the film
  on, weak-signal zones read 0.
- **No deep sleep, no threshold auto-stop** on this sensor firmware
  (ULD 2.0.1), as on the VL53L7CX: both fail with `DEPZ_E_ARG`.
- **Not verified live in C yet** — the replays cover the firmware download and
  the frame decode byte for byte; the live check was on a VL53L7CH.
- All the [VL53L7CX gotchas](../vl53l7cx/guide.md#gotchas) apply.
