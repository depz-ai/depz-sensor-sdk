# VL53L4CX — user guide

Hands-on guide to the 1D-family codecs ([`Vl53lx`](api.md#vl53lx)), product
table ([`Vl53lxProducts`](api.md#vl53lxproducts)) and block decode
([`Vl53lxDecode`](api.md#vl53lxdecode)) for the VL53L4CX. For what the sensor
is and its concepts, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md).

This SDK does not drive the sensor: the ST driver (initialisation, modes,
budgets, histogram target extraction) runs in the Python or
TypeScript SDK. What follows is everything you need to build the bridge
commands and decode what the board streams.

## Contents

- [Which board is it](#which-board-is-it)
- [The product table](#the-product-table)
- [Stream commands](#stream-commands)
- [Decode the histogram block](#decode-the-histogram-block)
- [Running as a VL53L4CD](#running-as-a-vl53l4cd)
- [Which statuses to plot](#which-statuses-to-plot)
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
assert Vl53lxProducts.resolveClass(null, "DEPZ ToF Sensor VL53L4CX USB v2.1") == Vl53lxProducts.SensorClass.VL53L4CX;
assert "VL53L4CX".equals(Vl53lxProducts.productFromBoardName("ToF Sensor VL53L4CX USB v2.1"));
System.out.println(cls.className + " / " + cls.product);
```

The model id the sensor answers (`0xEBAA`) is a cross-check only: the
VL53L4CD answers the same.

## The product table

`Vl53lxProducts.TABLE` holds one `Product` per family member; a `Bridge`
says what one product/driver pair tells the bridge:

```java
import java.util.List;
import ai.depz.sensor.sensors.vl53lx.Vl53lxProducts;

Vl53lxProducts.Product p = Vl53lxProducts.product("VL53L4CX");   // throws for an unserved name
assert p.modelId() == 0xEBAA && p.reachMm() == 6000;
assert p.driverKinds().equals(List.of("histogram"));             // histogram only
assert p.defaultDriver().equals("histogram");

Vl53lxProducts.Bridge hist = p.bridge("histogram");
assert hist.blockAddr() == 0x0088 && hist.blockLen() == 83;      // histogram block
assert hist.clearSteps()[0][0] == 0x0086 && hist.clearSteps()[0][1] == 0x01;   // interrupt release
assert hist.addrWidth() == 2 && hist.maxKhz() == 1000;
// p.bridge("uld") throws UnsupportedOperationException: no such pair, refuse

// Naming the neighbour borrows its driver: the L4CD's light ULD, 1.2 m reach.
Vl53lxProducts.Bridge borrowed = Vl53lxProducts.product("VL53L4CD").bridge("uld");
assert borrowed.blockAddr() == 0x0089 && borrowed.blockLen() == 17;
```

When running as a VL53L4CD, the die block is read with `DieVariant.L4`
([below](#running-as-a-vl53l4cd)).

## Stream commands

A stream is armed with `VL53_START_STREAM`: the block to read on every
data-ready edge and the interrupt-release writes the bridge plays after each
read (v2.00 carries them in the command). Set the register-address width
first — it is sticky, and 2 after a reset:

```java
import ai.depz.sensor.protocol.Vl53lx;
import ai.depz.sensor.sensors.vl53lx.Vl53lxProducts;
import ai.depz.sensor.transport.CrcType;
import ai.depz.sensor.transport.Framing;

Vl53lxProducts.Bridge bus = Vl53lxProducts.product("VL53L4CX").bridge("histogram");
byte[] width = Framing.buildPacket(Vl53lx.Vl53lxCmd.SET_ADDR_WIDTH.value,
        Vl53lx.packSetAddrWidth(bus.addrWidth()), 0, CrcType.NONE);
byte[] start = Framing.buildPacket(Vl53lx.Vl53lxCmd.START_STREAM.value,
        Vl53lx.packStartStream(bus.blockAddr(), bus.blockLen(), bus.clearSteps(), 0),   // ≤ 4 clear steps
        0, CrcType.NONE);
byte[] stop = Framing.buildPacket(Vl53lx.Vl53lxCmd.STOP_STREAM.value);
```

The register commands (`packReadReg`, `packWriteReg`, `packXshut`,
`packSetI2cSpeed`) are the VL53L4CD codecs, forwarded by `Vl53lx`. Every init
runs at 400 kHz; the driver raises the bus to `maxKhz` afterwards.

## Decode the histogram block

The VL53L4CX's only driver is `histogram`: each `RPT_VL53_STREAM` report
carries the 83-byte block at `0x0088`. `Vl53lxDecode.decodeHistogramRaw` extracts the status bytes and the
24 photon-count bins (bin 23's low byte, carried separately, is patched in
first; `raw` is not modified):

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
    Vl53l4.StreamData s = Vl53lx.unpackStreamData(p.payload());   // timestampUs, addr, len, data
    if (s.addr() != Vl53lxDecode.HISTOGRAM_BLOCK_ADDR) {
        continue;
    }
    Vl53lxDecode.HistogramRaw h = Vl53lxDecode.decodeHistogramRaw(s.data());
    int peakBin = 0;
    for (int i = 1; i < Vl53lxDecode.HISTOGRAM_BINS; i++) {
        if (h.bins()[i] > h.bins()[peakBin]) {
            peakBin = i;
        }
    }
    System.out.printf("%d µs #%d range status 0x%02X, %d effective SPADs, VCSEL start %d, peak bin %d = %d%n",
            s.timestampUs(), h.streamCount(), h.rangeStatus(), h.dssActualEffectiveSpads(),
            h.vcselStart(), peakBin, h.bins()[peakBin]);
}
```

Turning bins into up to four targets (preset, VCSEL period, the A/B frame
pairs, phase history) is the full driver's job and is not in this SDK. The
driver keeps state from frame to frame, so a stream fed to it must be decoded
**exactly once, in order** — replaying or skipping frames changes its output.

## Running as a VL53L4CD

For offset/crosstalk calibration or thresholds, the full drivers can run the
VL53L4CX on the VL53L4CD's light ULD (reach drops to ~1.2 m). The board then
streams the 17-byte die block, read with `DieVariant.L4`:

```java
import ai.depz.sensor.sensors.vl53l4.Vl53l4Uld;
import ai.depz.sensor.sensors.vl53lx.Vl53lxDecode;

if (s.addr() == Vl53lxDecode.DIE_BLOCK_ADDR) {
    Vl53l4Uld.Results r = Vl53lxDecode.decodeDieBlock(s.data(), Vl53lxDecode.DieVariant.L4);
    System.out.println(r.distanceMm() + " mm, status " + r.rangeStatus() + ", sigma " + r.sigmaMm() + " mm");
}
```

The result has the VL53L4CD result shape (`Vl53l4Uld.Results`): `distanceMm`,
`sigmaMm`, signal and ambient rates in kcps, per-SPAD rates (rate × 256 ÷
SPADs), the active SPAD count and the rolling `streamCount`; `rangeStatus` is
mapped through the ULD status table (0 = valid).

## Which statuses to plot

The full drivers report one family-wide status. Statuses **0, 6 and 11** are
usable: 6 is the first frame (no predecessor for the wrap check), 11 a merged
pulse. `status == 0` alone drops those frames. The die block's `rangeStatus`
(when running as a VL53L4CD) uses the same numbering, so the same rule applies
to it:

```java
import java.util.Set;
import ai.depz.sensor.sensors.vl53l4.Vl53l4Uld;
import ai.depz.sensor.sensors.vl53lx.Vl53lxDecode;

/* Statuses a chart should draw (0 valid, 6 first frame, 11 merged pulse). */
Set<Integer> plottable = Set.of(0, 6, 11);

Vl53l4Uld.Results r = Vl53lxDecode.decodeDieBlock(s.data(), Vl53lxDecode.DieVariant.L4);
if (plottable.contains(r.rangeStatus())) {
    System.out.println(r.distanceMm() + " mm");
}
```

The histogram block's `rangeStatus` byte is the raw device register, not the
driver's target status — the rule does not apply to it.

The histogram driver's `short` preset is unusable on the L4CX at any distance:
its frames alternate between the true distance flagged status 7 and a wrong one
(flat wall, 2026-09-28: −341 mm at 0.15 and 0.3 m, −156 mm at 0.6 m; at 1.0 m
the true 1008 mm comes flagged status 4 and the other frame reads 238 mm), so a
plottable filter drops them all. Same on firmware v0.23 and v0.24 and with the
firmware repo's own `vl53_tool.py`; the cause is not known yet. `medium` and
`long` are clean at all four distances.

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
    System.out.printf("%d edges, %d slots skipped, %d frames dropped, %d I2C errors, %d kHz, width %d, %d clear steps%n",
            info.intEdges(), info.slotsSkipped(), info.framesDropped(), info.i2cErrors(),
            info.i2cKhz(), info.addrWidth(), info.nClear());
}
```

`XSHUT_RESET` is 1 ms low plus a fixed 5 ms wait — v2.00 has **no boot
handshake**: poll the boot register (`0x00E5 == 0x03`) yourself before the
first access. A power-cycled sensor holds none of its configuration or
calibration. The counters are free-running and wrap silently: watch
increments.

## Gotchas

- **Plot 0, 6 and 11**, not only 0 — and don't use the `short` preset on the
  L4CX at all (see above); `medium` and `long` are clean.
- **Decode every histogram frame exactly once, in order** if you feed a
  target-extraction driver.
- **Set the address width before the first register access** of a session,
  and never under a running stream.
- **Same model id as the VL53L4CD** (`0xEBAA`) — the board's USB PID or device
  name, not the silicon, tells them apart.
- **Running as a VL53L4CD reads the die block with `DieVariant.L4`**.
- **At most 4 interrupt-release steps** — `packStartStream` throws beyond.
