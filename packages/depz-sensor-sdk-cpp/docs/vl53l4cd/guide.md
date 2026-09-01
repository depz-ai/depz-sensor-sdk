# VL53L4CD — user guide

Hands-on guide to the `depz::vl53l4` wire codecs and host-ULD math in
`depz/vl53l4.hpp`. For what the sensor is and its concepts, read the
[introduction](introduction.md); for exact signatures see the
[API reference](api.md). This SDK is the **decode layer** — the snippets build
command payloads and decode reply/report payloads; you own the serial port and
the request/reply loop (see the [common guide](../guide.md)).

## Contents

- [The codec surface](#the-codec-surface)
- [Read and write registers](#read-and-write-registers)
- [Stream the result block](#stream-the-result-block)
- [Decode a result block](#decode-a-result-block)
- [Range timing](#range-timing)
- [Tuning words](#tuning-words)
- [Gotchas](#gotchas)

## The codec surface

- `Vl53l4Cmd` — host→device opcodes (`ReadReg`, `WriteReg`, `Xshut`,
  `StartStream`, `StopStream`, `GetInfo`, `SetI2cSpeed`), with the
  `pack_read_reg` / `pack_write_reg` / `pack_xshut` / `pack_start_stream` /
  `pack_set_i2c_speed` payload encoders.
- `Vl53l4Rpt` — device→host report ids (`RegData`, `Info`, `Stream`).
- `RegData` / `Vl53l4Info` / `StreamData` — decoded reports, each with a
  `static std::optional<…> unpack(byte_span)` (nullopt on a short payload).
- The pure ULD pieces: `parse_result_block`, the range-timing register math
  (`range_timing_registers` / `decode_range_timing`), the tuning-word codecs
  and `config_block()`.

## Read and write registers

Frame the command payloads with `build_packet` (from the
[common guide](../guide.md#transport-framing--crc)) and write them to your
port:

```cpp
#include "depz/framing.hpp"
#include "depz/vl53l4.hpp"

namespace v4 = depz::vl53l4;

// read the model id word at IDENTIFICATION__MODEL_ID (0x010F; expect 0xEBAA)
depz::bytes rd_payload = v4::pack_read_reg(0x010F, 2);
depz::bytes rd = depz::build_packet(
    static_cast<std::uint8_t>(v4::Vl53l4Cmd::ReadReg),
    depz::as_bytes(rd_payload));

// the reply is Vl53l4Rpt::RegData (0x91)
auto d = v4::RegData::unpack(depz::as_bytes(pkt.payload));
if (d && d->data.size() == 2) { /* big-endian: {0xEB, 0xAA} == v4::MODEL_ID */ }

// write the 91-byte init configuration block in one transaction
depz::bytes cfg = v4::config_block();
depz::bytes wr_payload = v4::pack_write_reg(v4::CONFIG_ADDR, depz::as_bytes(cfg));
depz::bytes wr = depz::build_packet(
    static_cast<std::uint8_t>(v4::Vl53l4Cmd::WriteReg),
    depz::as_bytes(wr_payload));
```

Reads and writes are capped at `XFER_MAX` (253) bytes per transfer — the
encoders are pure and don't police it; the firmware rejects an oversize
request. Register contents are big-endian; the wire fields (`addr`, `len`,
timestamps) are little-endian.

## Stream the result block

```cpp
// arm INT-driven streaming of the 17-byte result block
depz::bytes st_payload = v4::pack_start_stream(
    v4::RESULT_BLOCK_ADDR, v4::RESULT_BLOCK_LEN, /*flags=*/0);
depz::bytes start = depz::build_packet(
    static_cast<std::uint8_t>(v4::Vl53l4Cmd::StartStream),
    depz::as_bytes(st_payload));

// each sample arrives as Vl53l4Rpt::Stream (0x93)
auto s = v4::StreamData::unpack(depz::as_bytes(pkt.payload));
if (s) {
    auto r = v4::parse_result_block(depz::as_bytes(s->data));
    // s->addr / s->len echo the stream configuration; s->timestamp_us is the
    // MCU uptime at the INT edge
}
```

The `flags` bit `SF_INT_ACT_HIGH` (0x02) selects INT-active-high; the default
(0) matches the ULD init block (INT active low). `timestamp_us` is MCU uptime
at the **INT edge** — the sensor event, not the I2C completion. Stop with a
payload-less `Vl53l4Cmd::StopStream`.

## Decode a result block

```cpp
auto r = v4::parse_result_block(raw17);        // nullopt when < 15 bytes
if (r && r->range_status == 0) {               // 0 = valid
    std::printf("%d mm, sigma %d mm\n", r->distance_mm, r->sigma_mm);
}
```

`Vl53l4Result` carries `range_status` (via the ULD `status_rtn` table; raw
≥ 24 passes through unmapped), `distance_mm`, `signal_rate_kcps` /
`ambient_rate_kcps` (×8), the per-SPAD rates, `number_of_spad`, `sigma_mm` and
the sensor's own `stream_count` frame counter (wraps at 255).

## Range timing

The SetRangeTiming/GetRangeTiming register math is pure and bit-exact:

```cpp
// SetRangeTiming(50 ms budget, continuous): osc_frequency is the word read
// from register 0x0006, clock_pll the word from RESULT__OSC_CALIBRATE_VAL
// (only used in autonomous mode)
auto regs = v4::range_timing_registers(/*budget_ms=*/50, /*inter_ms=*/0,
                                       osc_frequency, clock_pll);
// regs->range_config_a (0x005E), regs->range_config_b (0x0061),
// regs->intermeasurement_raw (the INTERMEASUREMENT_MS dword at 0x006C)

// GetRangeTiming readback from the raw register reads
auto timing = v4::decode_range_timing(inter_raw, clock_pll, osc_frequency,
                                      regs->range_config_a);
// timing->timing_budget_ms, timing->inter_measurement_ms
```

Budget is 10..200 ms. `inter_ms == 0` selects continuous mode; a value
**greater** than the budget selects autonomous low power; anything else — or a
zero `osc_frequency` — returns `std::nullopt`.

## Tuning words

Each tuning register is a word codec pair (encode for the write, decode for
the readback):

```cpp
std::uint16_t word = v4::offset_raw(-10);          // RANGE_OFFSET_MM (0x001E)
int mm             = v4::decode_offset(word);      // → -10

v4::xtalk_raw(20);              // XTALK_PLANE_OFFSET_KCPS (0x0016)
v4::signal_threshold_raw(1024); // MIN_COUNT_RATE_RTN_LIMIT_MCPS (0x0066)
v4::sigma_threshold_raw(15);    // RANGE_CONFIG__SIGMA_THRESH (0x0064);
                                // optional — nullopt when sigma_mm > 16383
```

## Gotchas

- **Check `range_status` before trusting a distance** — 0 is valid; everything
  else describes why the measurement is suspect (raw ≥ 24 is unmapped). A
  non-zero status is data, not a protocol error.
- **Two endiannesses on purpose** — wire fields little-endian, register
  contents big-endian. The codecs handle both; don't swap bytes yourself.
- **The decoders return `std::optional`** — `unpack` on a short payload,
  `parse_result_block` on a short block, and the timing math on invalid inputs
  all yield `std::nullopt`; check before dereferencing.
- **After `XSHUT_OFF`/`XSHUT_RESET` the sensor holds none of the ULD
  configuration** — re-run the init sequence (config block, VHV, timing).
- **Configure the sensor before raising the bus speed** — init at 400 kHz,
  then `Vl53l4Cmd::SetI2cSpeed` to 1 MHz; read `i2c_khz` back from
  `Vl53l4Cmd::GetInfo` for the programmed step (the command answers OK
  unconditionally).
- **Skipped stream slots produce no per-slot error** — watch the `Vl53l4Info`
  counters (`slots_skipped`, `i2c_errors`) and gaps in `timestamp_us`;
  `stream_count` tells a frame the host never received from one the sensor
  never produced.
- **The live driver is host code you write** — this layer is the pure codecs
  and math only; `sensor_init`, calibration loops and waits are an extension
  point driven over `ReadReg`/`WriteReg` (see the reference Python SDK's live
  driver).
