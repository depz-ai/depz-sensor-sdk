# VL53L7CX — user guide

Hands-on guide to the L5/L7 codecs ([`Vl53l7`](api.md#vl53l7)) and decode
([`Vl53l7Uld`](api.md#vl53l7uld)). It applies unchanged to the
[VL53L5CX](../vl53l5cx/guide.md) and, plus CNH, to the
[VL53L7CH](../vl53l7ch/guide.md). The results frame itself — fields, scaling,
the advanced-DCI codecs — is the VL53L8 one: see the
[VL53L8CX guide](../vl53l8cx/guide.md). For concepts read the
[introduction](introduction.md); for signatures the [API reference](api.md).

This SDK does not drive the board: initialising the sensor (firmware
download, default configuration) and starting a ranging session is done with
the Python or TypeScript SDK. What follows is everything you need to build the
board's commands and decode what it sends.

## Contents

- [Which board is it](#which-board-is-it)
- [Board commands](#board-commands)
- [The bridge info report](#the-bridge-info-report)
- [Register reads: split at 1536 bytes](#register-reads-split-at-1536-bytes)
- [Reassemble and decode a frame](#reassemble-and-decode-a-frame)
- [What differs from the VL53L8 decode](#what-differs-from-the-vl53l8-decode)
- [Gotchas](#gotchas)

## Which board is it

All three L5/L7 boards run the same firmware (`APP_VL53L7_*`, sensor type
`vl53l7`), so the firmware name cannot tell them apart.
`Vl53l7Uld.resolveModel` applies the contract order: the production USB PID
model, else the first `VL53L([57])(CX|CH)` in the device name, else
VL53L7CX:

```java
import ai.depz.sensor.sensors.vl53l7.Vl53l7Uld;
import ai.depz.sensor.usb.UsbIds;

// vid/pid from the USB enumeration, deviceName from GET_DEVICE_NAME.
Vl53l7Uld.Model model = Vl53l7Uld.resolveModel(UsbIds.usbModelHint(vid, pid), deviceName);
assert Vl53l7Uld.resolveModel(null, "DEPZ ToF Sensor VL53L7CH USB v2.1") == Vl53l7Uld.Model.VL53L7CH;
System.out.println(model.label + " (CNH: " + (model == Vl53l7Uld.Model.VL53L7CH) + ")");
```

## Board commands

The L5/L7 bridge adds three commands to the VL53L8 register bridge
([`Vl53l7.Vl53l7Cmd`](api.md#vl53l7vl53l7cmd)). Frame each payload with
`Framing.buildPacket`:

```java
import ai.depz.sensor.protocol.Vl53l7;
import ai.depz.sensor.transport.CrcType;
import ai.depz.sensor.transport.Framing;

// LPn low 1 ms, high, I2C_RST pulse — the sensor firmware is lost afterwards.
byte[] softCycle = Framing.buildPacket(Vl53l7.Vl53l7Cmd.PIN_CTRL.value,
        Vl53l7.packPinCtrl(Vl53l7.PinAction.SOFT_CYCLE), 0, CrcType.NONE);
// Re-time the sensor bus; the device snaps to the nearest I2C_SPEED_STEPS_KHZ entry.
byte[] bus400k = Framing.buildPacket(Vl53l7.Vl53l7Cmd.SET_I2C_SPEED.value,
        Vl53l7.packSetI2cSpeed(400), 0, CrcType.NONE);
// Ask for the bridge counters (answered with RPT_VL53_INFO).
byte[] getInfo = Framing.buildPacket(Vl53l7.Vl53l7Cmd.GET_INFO.value);
```

| `PinAction` | effect |
|---|---|
| `LPN_OFF` | stop streaming, LPn low: sensor I2C interface off (state lost) |
| `LPN_ON` | LPn high: interface on (the power-up default) |
| `I2C_RST` | pulse the sensor's I2C_RST |
| `SOFT_CYCLE` | stop streaming, LPn low 1 ms, high, I2C_RST pulse, clear the I2C counters (state lost) |

`VL53_SET_I2C_SPEED` is refused with `ERR_BUSY` while a transfer is in
flight — stop the stream first, and read the effective speed back from the
info report. The register commands (`READ_REG` 0x32, `WRITE_REG` 0x33,
`START_STREAM` 0x35, `STOP_STREAM` 0x36) are the VL53L8 ones; their encoders
(`packReadReg`, `packWriteReg`, `packStartStream`) are on `Vl53l7` too.

## The bridge info report

`RPT_VL53_INFO` (`Vl53l7Rpt.VL53_INFO`, `0x92`) is bridge state only — the
sensor is never probed — and carries **no** echoed command byte:

```java
import ai.depz.sensor.protocol.Vl53l7;
import ai.depz.sensor.transport.Event;
import ai.depz.sensor.transport.Packet;
import ai.depz.sensor.transport.PacketParser;

PacketParser parser = new PacketParser();
for (Event ev : parser.feed(rxBytes)) {
    if (ev instanceof Packet p && p.cmd() == Vl53l7.Vl53l7Rpt.VL53_INFO.value) {
        Vl53l7.Vl53l7Info info = Vl53l7.Vl53l7Info.unpack(p.payload());   // throws below 20 bytes
        System.out.printf("%d kHz, %d I2C errors (last code %d), %d frames dropped, streaming %b%n",
                info.i2cKhz(), info.i2cErrors(), info.lastI2cError(),
                info.framesDropped(), info.streaming());
    }
}
```

`lastI2cError` is one of `Vl53l7.I2C_ERR_OK`, `I2C_ERR_NACK`,
`I2C_ERR_TIMEOUT`, `I2C_ERR_BUS_ERROR`. Every `VL53_GET_INFO` takes the bus
from the stream and can drop the frame in flight (counted in
`framesDropped`): read it before and after a run, not in a loop during one.

## Register reads: split at 1536 bytes

The L5/L7 bridge accepts at most `Vl53l7.READ_MAX_LEN` = 1536 bytes per
`VL53_READ_REG` (the VL53L8 host splits at 2048 — reusing that here fails
every large read with `ERR_INVALID_PARAM`) and `Vl53l7.WRITE_MAX_LEN` = 2048
per `VL53_WRITE_REG`:

```java
import java.util.ArrayList;
import java.util.List;
import ai.depz.sensor.protocol.Vl53l7;
import ai.depz.sensor.transport.CrcType;
import ai.depz.sensor.transport.Framing;

// Read `total` bytes from register `addr` in L5/L7-sized pieces.
List<byte[]> requests = new ArrayList<>();
for (int off = 0; off < total; off += Vl53l7.READ_MAX_LEN) {
    int n = Math.min(total - off, Vl53l7.READ_MAX_LEN);
    requests.add(Framing.buildPacket(Vl53l7.Vl53l7Cmd.READ_REG.value,
            Vl53l7.packReadReg(addr + off, n), 0, CrcType.NONE));
}
```

Each reply is an `RPT_VL53_REG_DATA` (`0x91`): `cmd u8` (the echoed opcode),
`timestamp_us u64`, then the register bytes.

## Reassemble and decode a frame

Frames arrive as `RPT_VL53_FRAME` chunks (up to 1536 data bytes each): a 4×4
frame is 1060 bytes and an 8×8 frame 1444 bytes, one chunk each; a CNH frame
on the VL53L7CH spans several. Reassemble with the shared `FrameReassembler`
and decode with [`Vl53l7Uld.parseFrame`](api.md#vl53l7uld):

```java
import ai.depz.sensor.protocol.Vl53l7;
import ai.depz.sensor.sensors.vl53l7.Vl53l7Uld;
import ai.depz.sensor.sensors.vl53l8.FrameReassembler;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Uld;
import ai.depz.sensor.transport.Event;
import ai.depz.sensor.transport.Packet;
import ai.depz.sensor.transport.PacketParser;

PacketParser parser = new PacketParser();
FrameReassembler reasm = new FrameReassembler();
for (Event ev : parser.feed(rxBytes)) {
    if (!(ev instanceof Packet p) || p.cmd() != Vl53l7.Vl53l7Rpt.VL53_FRAME.value) {
        continue;
    }
    FrameReassembler.CompletedFrame done = reasm.feed(FrameReassembler.unpackFrameChunk(p.payload()));
    if (done == null) {
        continue;
    }
    // Resolution read from the frame; throws Vl53l8Uld.Vl53l8Error on a corrupt frame.
    Vl53l8Uld.Results r = Vl53l7Uld.parseFrame(done.frame(), done.frame().length);
    int n = r.resolution();                       // 16 or 64
    int side = n == 64 ? 8 : 4;
    int centre = side / 2 * side + side / 2;      // one of the four centre zones
    if (r.targetStatus[centre] == 5 || r.targetStatus[centre] == 9) {
        System.out.println(done.timestampUs() + " µs: centre " + r.distanceMm[centre]
                + " mm, " + r.siliconTempDegc + " °C");
    }
}
// reasm.completed / reasm.discarded count rebuilt and dropped frames
```

`parseFrame(raw, size, resolution)` takes the resolution explicitly (16 or
64) when you know what the session was started with. The result is a
`Vl53l8Uld.Results` with the same fields as on the VL53L8CX — see
[the results table](../vl53l8cx/guide.md#the-results-frame-fields).

## What differs from the VL53L8 decode

- **Footer id at `size-4`** for every L5/L7 part
  ([`Vl53l7Uld.FOOTER_ID_OFF`](api.md#vl53l7uld)) — both the ULD 2.0.1 blob of
  the L5CX / L7CX and the VL53LMZ 2.0.16 blob of the L7CH. The VL53L8CX
  geometry (`Variant.CX`, `size-12`) fails these frames.
- **Per-zone arrays are trimmed to the frame's resolution.** On L5/L7 the
  per-target blocks (index ≥ `0x6C90`) keep their 64 entries even in 4×4: the
  sensor fills the first 16 and zero-pads the rest. `Vl53l7Uld.parseFrame`
  reads the zone count from the ambient-rate block (one entry per zone) and
  cuts every array to it, so `r.distanceMm.length == r.resolution()` in both
  resolutions.
- **Bigger chunks** — `Vl53l7.STREAM_CHUNK_MAX` is 1536 (VL53L8: 1528). The
  reassembler does not depend on the chunk size.
- **Ranging frequency** — `Vl53l7Uld.MIN_RANGING_FREQUENCY_HZ` = 1 (VL53L8:
  2); maximum 60 Hz at 4×4, 15 Hz at 8×8.

The advanced-DCI codecs on `Vl53l8Uld` (motion indicator, detection
thresholds, crosstalk margin) apply as on the VL53L8, within the L5CX / L7CX
firmware's limits: no `POWER_MODE_DEEP_SLEEP` and no threshold auto-stop.

## Gotchas

- **Decode with `Vl53l7Uld.parseFrame`**, not `Vl53l8Uld.parseFrame(…,
  Variant.CX)` — only the L5/L7 path uses the `size-4` footer and trims the 4×4
  per-target blocks.
- **Split register reads at 1536 bytes**, not the VL53L8's 2048.
- **`RPT_VL53_INFO` has no echoed command byte** — don't match it to its
  request by `payload[0]`.
- **Pin control wipes the sensor** — after `LPN_OFF` / `SOFT_CYCLE` the sensor
  must be initialised again (firmware download included).
- **Bridge info during ranging costs a frame** — read the counters around a
  run.
- **Peel the protective film off the lens** before measuring — with the film
  on, weak-signal zones read 0.
