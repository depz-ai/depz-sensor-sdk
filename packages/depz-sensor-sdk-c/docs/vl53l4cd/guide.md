# VL53L4CD — user guide

Hands-on guide to the `depz_vl53l4_*` wire codecs and the host-ULD math. For
what the sensor is and its concepts, read the [introduction](introduction.md);
for exact signatures see the [API reference](api.md). This is a decode layer —
the snippets build command payloads and decode reply/report payloads; you own
the transport that carries them (see the common guide's
[mental model](../guide.md#mental-model)).

## Contents

- [The codec surface](#the-codec-surface)
- [Read and write registers](#read-and-write-registers)
- [Stream the result block](#stream-the-result-block)
- [Decode a result block](#decode-a-result-block)
- [Range timing](#range-timing)
- [Tuning words](#tuning-words)
- [Gotchas](#gotchas)

## The codec surface

- `depz_vl53l4_cmd` — host→device opcodes (`READ_REG`, `WRITE_REG`, `XSHUT`,
  `START_STREAM`, `STOP_STREAM`, `GET_INFO`, `SET_I2C_SPEED`).
- `depz_vl53l4_rpt` — device→host report ids (`REG_DATA` `0x91`, `INFO`
  `0x92`, `STREAM` `0x93`).
- `depz_vl53l4_pack_*` — payload encoders (they return the payload length
  written; `depz_build_packet()` then frames it).
- `depz_vl53l4_reg_data` / `depz_vl53l4_info` / `depz_vl53l4_stream` — decoded
  reports, filled by the matching `depz_vl53l4_unpack_*` (0 on success,
  negative on a short/bad payload).
- The pure ULD pieces — `depz_vl53l4_parse_result_block()`, the range-timing
  register math, the tuning word codecs and `depz_vl53l4_config_block()`.

## Read and write registers

```c
#include <depz_sensor_sdk.h>

uint8_t payload[2 + DEPZ_VL53L4_XFER_MAX];
uint8_t frame[DEPZ_MAX_FRAME];
size_t n, frame_len;

// read the model id word (IDENTIFICATION__MODEL_ID 0x010F; expect 0xEBAA)
n = depz_vl53l4_pack_read_reg(0x010F, 2, payload);
depz_build_packet(DEPZ_VL53L4_CMD_READ_REG, payload, n, seq++, DEPZ_CRC8,
                  frame, sizeof frame, &frame_len);
serial_write(fd, frame, frame_len);

// the reply is RPT_VL53_REG_DATA (0x91) — in your event callback:
if (ev->cmd == DEPZ_VL53L4_RPT_REG_DATA) {
    depz_vl53l4_reg_data d;
    if (depz_vl53l4_unpack_reg_data(ev->payload, ev->payload_len, &d) == 0) {
        // register contents are big-endian sensor bytes
        uint16_t model = (uint16_t)((d.data[0] << 8) | d.data[1]);
        // model == DEPZ_VL53L4_MODEL_ID (0xEBAA)
    }
}

// write the 91-byte init configuration block in one transaction
uint8_t cfg[91];
depz_vl53l4_config_block(cfg);            // ST defaults, byte 0 forced to 0x12
n = depz_vl53l4_pack_write_reg(DEPZ_VL53L4_CONFIG_ADDR, cfg, sizeof cfg,
                               payload);
depz_build_packet(DEPZ_VL53L4_CMD_WRITE_REG, payload, n, seq++, DEPZ_CRC8,
                  frame, sizeof frame, &frame_len);
```

Reads and writes are capped at `DEPZ_VL53L4_XFER_MAX` (253) bytes and
`addr + len` ≤ 0x10000 — `depz_vl53l4_pack_write_reg()` returns 0 for an
out-of-range `data_len`. Register contents are big-endian; the wire fields
(`addr`, `len`, timestamps) are little-endian.

## Stream the result block

```c
// arm INT-driven streaming of the 17-byte result block at 0x0089
n = depz_vl53l4_pack_start_stream(DEPZ_VL53L4_RESULT_BLOCK_ADDR,
                                  DEPZ_VL53L4_RESULT_BLOCK_LEN, 0, payload);
depz_build_packet(DEPZ_VL53L4_CMD_START_STREAM, payload, n, seq++, DEPZ_CRC8,
                  frame, sizeof frame, &frame_len);

// each sample arrives as RPT_VL53_STREAM (0x93)
if (ev->cmd == DEPZ_VL53L4_RPT_STREAM) {
    depz_vl53l4_stream s;
    if (depz_vl53l4_unpack_stream(ev->payload, ev->payload_len, &s) == 0) {
        depz_vl53l4_result r;
        if (depz_vl53l4_parse_result_block(s.data, s.len, &r) == 0) {
            // ...
        }
    }
}
```

The `flags` bit `DEPZ_VL53L4_SF_INT_ACT_HIGH` (`0x02`) selects
INT-active-high; the default (0) matches the ULD init block (INT active low).
`timestamp_us` is MCU uptime at the **INT edge** — the sensor event, not the
I2C completion. Each stream report echoes `addr`/`len`, so it is
self-describing.

## Decode a result block

```c
depz_vl53l4_result r;
if (depz_vl53l4_parse_result_block(raw, raw_len, &r) == 0
        && r.range_status == 0) {                 // 0 = valid
    printf("%d mm, sigma %d mm\n", r.distance_mm, r.sigma_mm);
}
```

`depz_vl53l4_result` carries `range_status` (via the `STATUS_RTN` table; raw
≥ 24 passes through unmapped), `distance_mm`, `signal_rate_kcps` /
`ambient_rate_kcps` (×8), the per-SPAD rates, `number_of_spad`, `sigma_mm`
and the sensor's own `stream_count` frame counter (wraps at 255). The decode
matches `VL53L4CD_GetResult()` bit-exactly; it returns `-1` when
`len < 15`.

## Range timing

The SetRangeTiming/GetRangeTiming register math is pure and bit-exact:

```c
// SetRangeTiming(50 ms budget, continuous): osc_frequency from reg 0x0006,
// clock_pll from RESULT__OSC_CALIBRATE_VAL (only used in autonomous mode)
uint16_t range_config_a, range_config_b;
uint32_t inter_raw;
if (depz_vl53l4_range_timing_registers(50, 0, osc_frequency, clock_pll,
                                       &range_config_a, &range_config_b,
                                       &inter_raw) == 0) {
    // write range_config_a -> RANGE_CONFIG_A (0x005E),
    //       range_config_b -> RANGE_CONFIG_B (0x0061),
    //       inter_raw      -> INTERMEASUREMENT_MS (0x006C), via WRITE_REG
}

// GetRangeTiming readback from the raw register reads
uint32_t budget_ms, inter_ms;
depz_vl53l4_decode_range_timing(inter_raw, clock_pll, osc_frequency,
                                range_config_a, &budget_ms, &inter_ms);
```

Budget is 10..200 ms. `inter_ms == 0` selects continuous mode; a value
**greater** than the budget selects autonomous low power; anything else (or
`osc_frequency == 0`) returns `-1`.

## Tuning words

Each tuning register is a word codec pair (encode for the write, decode for
the readback):

```c
uint16_t word = depz_vl53l4_offset_raw(-10);      // RANGE_OFFSET_MM (0x001E)
int32_t  mm   = depz_vl53l4_decode_offset(word);  // -> -10

depz_vl53l4_xtalk_raw(20);              // XTALK_PLANE_OFFSET_KCPS (0x0016)
depz_vl53l4_signal_threshold_raw(1024); // MIN_COUNT_RATE_RTN_LIMIT_MCPS (0x0066)

uint16_t raw;
depz_vl53l4_sigma_threshold_raw(15, &raw); // RANGE_CONFIG__SIGMA_THRESH (0x0064)
```

`depz_vl53l4_sigma_threshold_raw()` returns `-1` when `mm > 16383` (the word
overflows); the other encoders are total.

## Gotchas

- **Check `range_status` before trusting a distance** — 0 is valid; everything
  else describes why the measurement is suspect (raw ≥ 24 is unmapped).
- **Two endiannesses on purpose** — wire fields little-endian, register
  contents big-endian. The codecs handle both; don't swap bytes yourself.
- **After `XSHUT` OFF/RESET the sensor holds none of the ULD configuration** —
  re-run the init sequence (config block, VHV, timing). Actions are
  `DEPZ_VL53L4_XSHUT_OFF` / `_ON` / `_RESET`; RESET is answered after the boot
  handshake.
- **Configure the sensor before raising the bus speed** — init at 400 kHz,
  then `SET_I2C_SPEED` to 1 MHz; read `i2c_khz` back from `GET_INFO`
  (`depz_vl53l4_unpack_info`) for the programmed step — the command answers OK
  unconditionally.
- **Skipped stream slots produce no per-slot error** — watch the
  `depz_vl53l4_info` counters (`slots_skipped`, `i2c_errors`; they wrap
  silently, watch increments) and gaps in `timestamp_us`; `stream_count` tells
  a frame the host never received from one the sensor never produced.
- **The live driver is host code you write** — this SDK ships the pure codecs
  and math only ([no live hardware driving](../guide.md#what-it-is-not));
  `sensor_init`, calibration loops and waits are yours to drive over
  `READ_REG`/`WRITE_REG`.
