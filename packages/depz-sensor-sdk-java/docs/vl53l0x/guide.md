# VL53L0X — user guide

Hands-on guide to the 1D-family codecs ([`Vl53lx`](api.md#vl53lx)), product
table ([`Vl53lxProducts`](api.md#vl53lxproducts)) and block decode
([`Vl53lxDecode`](api.md#vl53lxdecode)) for the VL53L0X. For what the sensor
is and its concepts, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md).

This SDK does not drive the sensor: the ST driver (initialisation,
reference SPADs, profiles, calibration, the PAL status) runs in the Python or
TypeScript SDK. What follows is everything you need to build the bridge
commands and decode what the board streams.

## Contents

- [Which board is it](#which-board-is-it)
- [The product table](#the-product-table)
- [Stream commands](#stream-commands)
- [Decode the result block](#decode-the-result-block)
- [Reset and bridge diagnostics](#reset-and-bridge-diagnostics)
- [Gotchas](#gotchas)

## Which board is it

Every 1D board runs the same firmware (`APP_VL53L0_4_*`, sensor type
`vl53lx`), so the firmware name cannot tell them apart.
`Vl53lxProducts.resolveClass` uses the production USB PID model, else the
first `VL53L<digit>…` in the device name:

```java
import ai.depz.sensor.sensors.vl53lx.Vl53lxProducts;
import ai.depz.sensor.usb.UsbIds;

Vl53lxProducts.SensorClass cls = Vl53lxProducts.resolveClass(UsbIds.usbModelHint(vid, pid), deviceName);
assert Vl53lxProducts.resolveClass("vl53l0x", "") == Vl53lxProducts.SensorClass.VL53L0X;
assert Vl53lxProducts.product("VL53L0X").modelIdOk(0x00EE);   // cross-check the sensor's answer
System.out.println(cls.className);
```

## The product table

```java
import java.util.List;
import ai.depz.sensor.sensors.vl53lx.Vl53lxDecode;
import ai.depz.sensor.sensors.vl53lx.Vl53lxProducts;

Vl53lxProducts.Product p = Vl53lxProducts.product("VL53L0X");
assert p.modelId() == 0x00EE && p.reachMm() == 2000;
assert p.driverKinds().equals(List.of("uld"));

Vl53lxProducts.Bridge uld = p.defaultBridge();
assert uld.addrWidth() == 1;                                              // 8-bit register space
assert uld.blockAddr() == Vl53lxDecode.L0X_BLOCK_ADDR && uld.blockLen() == Vl53lxDecode.L0X_BLOCK_LEN;   // 0x14, 12 B
assert uld.clearSteps().length == 2;                                      // 0x0B←1, then 0x0B←0
assert uld.maxKhz() == 400;                                               // no FM+ on this part
```

## Stream commands

Set the register-address width to 1 before the first register access of a
session (the model-id read included) — it is sticky, and 2 after a reset. Then
arm the stream with the block and the two interrupt-release writes:

```java
import ai.depz.sensor.protocol.Vl53lx;
import ai.depz.sensor.sensors.vl53lx.Vl53lxProducts;
import ai.depz.sensor.transport.CrcType;
import ai.depz.sensor.transport.Framing;

Vl53lxProducts.Bridge bus = Vl53lxProducts.product("VL53L0X").defaultBridge();
byte[] width = Framing.buildPacket(Vl53lx.Vl53lxCmd.SET_ADDR_WIDTH.value,
        Vl53lx.packSetAddrWidth(1), 0, CrcType.NONE);
byte[] modelId = Framing.buildPacket(Vl53lx.Vl53lxCmd.READ_REG.value,
        Vl53lx.packReadReg(0xC0, 1), 0, CrcType.NONE);            // answers 0xEE
byte[] start = Framing.buildPacket(Vl53lx.Vl53lxCmd.START_STREAM.value,
        Vl53lx.packStartStream(bus.blockAddr(), bus.blockLen(), bus.clearSteps(), 0), 0, CrcType.NONE);
byte[] stop = Framing.buildPacket(Vl53lx.Vl53lxCmd.STOP_STREAM.value);
```

At width 1 only the low byte of `addr` goes on the bus, and `addr + len` must
stay within `0x100`.

## Decode the result block

Each `RPT_VL53_STREAM` report carries the 12-byte block at `0x14`.
`Vl53lxDecode.decodeL0xRaw` extracts its raw fields — what
`VL53L0X_GetRangingMeasurementData` reads before the PAL status step:

```java
import ai.depz.sensor.protocol.Vl53l4;
import ai.depz.sensor.protocol.Vl53lx;
import ai.depz.sensor.sensors.vl53lx.Vl53lxDecode;
import ai.depz.sensor.transport.Event;
import ai.depz.sensor.transport.Packet;
import ai.depz.sensor.transport.PacketParser;

PacketParser parser = new PacketParser();
for (Event ev : parser.feed(rxBytes)) {
    if (!(ev instanceof Packet p) || p.cmd() != Vl53lx.Vl53lxRpt.STREAM.value) {
        continue;
    }
    Vl53l4.StreamData s = Vl53lx.unpackStreamData(p.payload());
    if (s.addr() != Vl53lxDecode.L0X_BLOCK_ADDR) {
        continue;
    }
    Vl53lxDecode.L0xRaw r = Vl53lxDecode.decodeL0xRaw(s.data());      // throws below 12 bytes
    double signalMcps = r.signalRateMcps1616() / 65536.0;             // FixPoint16.16
    double ambientMcps = r.ambientRateMcps1616() / 65536.0;
    double spads = r.effectiveSpadCount88() / 256.0;                  // 8.8
    System.out.printf("%d µs  %d mm  device status 0x%02X  signal %.3f Mcps  ambient %.3f Mcps  %.1f SPADs%n",
            s.timestampUs(), r.distanceRaw(), r.deviceRangeStatus(), signalMcps, ambientMcps, spads);
}
```

| field | meaning |
|---|---|
| `distanceRaw` | distance, mm (quarter-mm if the driver enabled RangeFractional; off by default) |
| `deviceRangeStatus` | raw status byte 0 — **not** the PAL range status |
| `signalRateMcps1616` / `ambientRateMcps1616` | rates, FixPoint16.16 Mcps (the wire's 9.7 shifted left by 9) |
| `effectiveSpadCount88` | effective SPAD count, 8.8 |

The PAL range status, sigma and maximum distance the full drivers report need
device data cached at init (reference SPADs, calibration), so they are not in
this SDK. The family's plottable rule (statuses 0, 6, 11) applies to that
driver status, not to `deviceRangeStatus`.

## Reset and bridge diagnostics

```java
import ai.depz.sensor.protocol.Vl53lx;
import ai.depz.sensor.transport.CrcType;
import ai.depz.sensor.transport.Framing;

byte[] reset = Framing.buildPacket(Vl53lx.Vl53lxCmd.XSHUT.value,
        Vl53lx.packXshut(Vl53lx.XSHUT_RESET), 0, CrcType.NONE);
byte[] getInfo = Framing.buildPacket(Vl53lx.Vl53lxCmd.GET_INFO.value);

// RPT_VL53_INFO (0x92), 23 bytes — bridge state only, safe while streaming.
if (infoPkt.cmd() == Vl53lx.Vl53lxRpt.INFO.value) {
    Vl53lx.Vl53lxInfo info = Vl53lx.Vl53lxInfo.unpack(infoPkt.payload());
    System.out.printf("address width %d (1 on the L0X), %d kHz, %d I2C errors, %d slots skipped%n",
            info.addrWidth(), info.i2cKhz(), info.i2cErrors(), info.slotsSkipped());
}
```

`XSHUT_RESET` is 1 ms low plus a fixed 5 ms wait — v2.00 has **no boot
handshake**: poll the model id (`0xC0 == 0xEE`) yourself before configuring.
The address width is 2 after a reset: set it to 1 again.

## Gotchas

- **Address width 1, every session** — after a reset the bridge is back at 2;
  a 2-byte access on the L0X reads the wrong registers.
- **Two interrupt-release writes** (`0x0B ← 1`, `0x0B ← 0`) — pass the table's
  `clearSteps()`, not the die parts' single `0x0086 ← 1`.
- **Raw fields only** — `deviceRangeStatus` is not the PAL status.
- **400 kHz ceiling** — the L0X has no FM+ pad.
- **A NACK or two right after reset is normal** — the part is still booting;
  retry, and don't count those against a stream.
