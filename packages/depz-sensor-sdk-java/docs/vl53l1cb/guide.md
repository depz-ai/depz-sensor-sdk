# VL53L1CB — user guide

Hands-on guide to the 1D-family codecs ([`Vl53lx`](api.md#vl53lx)), product
table ([`Vl53lxProducts`](api.md#vl53lxproducts)) and block decode
([`Vl53lxDecode`](api.md#vl53lxdecode)) for the VL53L1CB. For what the sensor
is and its concepts, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md).

This SDK does not drive the sensor: the ST driver (initialisation, modes,
budgets, ROI, calibration, histogram target extraction) runs in the Python or
TypeScript SDK. What follows is everything you need to build the bridge
commands and decode what the board streams.

## Contents

- [Which board is it](#which-board-is-it)
- [The product table](#the-product-table)
- [Stream commands](#stream-commands)
- [Decode the die block (ULD)](#decode-the-die-block-uld)
- [Decode the histogram block](#decode-the-histogram-block)
- [Which statuses to plot](#which-statuses-to-plot)
- [Reset and bridge diagnostics](#reset-and-bridge-diagnostics)
- [Behind a cover glass](#behind-a-cover-glass)
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
assert Vl53lxProducts.resolveClass(null, "DEPZ ToF Sensor VL53L1CB USB v2.1") == Vl53lxProducts.SensorClass.VL53L1CB;
assert "VL53L1CB".equals(Vl53lxProducts.productFromBoardName("ToF Sensor VL53L1CB USB v2.1"));
System.out.println(cls.className + " / " + cls.product);
```

The model id the sensor answers (`0xEACC`) is a cross-check only: the
VL53L1CX answers the same.

## The product table

`Vl53lxProducts.TABLE` holds one `Product` per family member; a `Bridge`
says what one product/driver pair tells the bridge:

```java
import java.util.List;
import ai.depz.sensor.sensors.vl53lx.Vl53lxProducts;

Vl53lxProducts.Product p = Vl53lxProducts.product("VL53L1CB");   // throws for an unserved name
assert p.modelId() == 0xEACC && p.reachMm() == 8000;
assert p.driverKinds().equals(List.of("uld", "histogram"));
assert p.defaultDriver().equals("uld");

Vl53lxProducts.Bridge uld = p.bridge("uld");
assert uld.blockAddr() == 0x0089 && uld.blockLen() == 17;        // die result block
assert uld.clearSteps()[0][0] == 0x0086 && uld.clearSteps()[0][1] == 0x01;   // interrupt release
assert uld.addrWidth() == 2 && uld.maxKhz() == 1000;

Vl53lxProducts.Bridge hist = p.bridge("histogram");
assert hist.blockAddr() == 0x0088 && hist.blockLen() == 83;      // histogram block
// p.bridge("ulp") throws UnsupportedOperationException: no such pair, refuse
```

The die block is read by the VL53L1X ULD on this board: decode it with
`DieVariant.L1` (below).

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

Vl53lxProducts.Bridge bus = Vl53lxProducts.product("VL53L1CB").bridge("uld");
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

## Decode the die block (ULD)

With the `uld` driver every `RPT_VL53_STREAM` report carries the 17-byte die
result block. `Vl53lxDecode.decodeDieBlock` reads it as the VL53L1X ULD does
(`DieVariant.L1`: crosstalk-corrected peak signal at `0x0098`, per-SPAD scale
25):

```java
import ai.depz.sensor.protocol.Vl53l4;
import ai.depz.sensor.protocol.Vl53lx;
import ai.depz.sensor.sensors.vl53l4.Vl53l4Uld;
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
    if (s.addr() != Vl53lxDecode.DIE_BLOCK_ADDR) {
        continue;
    }
    Vl53l4Uld.Results r = Vl53lxDecode.decodeDieBlock(s.data(), Vl53lxDecode.DieVariant.L1);
    System.out.printf("%d µs  #%3d  %5d mm  status %d  sigma %d mm  signal %d kcps  ambient %d kcps  %d SPADs%n",
            s.timestampUs(), r.streamCount(), r.distanceMm(), r.rangeStatus(), r.sigmaMm(),
            r.signalRateKcps(), r.ambientRateKcps(), r.numberOfSpad());
}
```

The result has the VL53L4CD result shape (`Vl53l4Uld.Results`), integers as
the ULD computes them: `distanceMm`, `sigmaMm` (raw ÷ 4), signal and ambient
rates in kcps (raw × 8), per-SPAD rates (rate × 25 ÷ SPADs), the active SPAD
count and the rolling `streamCount`. `rangeStatus` is already mapped through
the ULD status table (0 = valid). A block shorter than 17 bytes throws
`IllegalArgumentException`.

## Decode the histogram block

With the `histogram` driver each report carries the 83-byte block at
`0x0088`. `Vl53lxDecode.decodeHistogramRaw` extracts the status bytes and the
24 photon-count bins (bin 23's low byte, carried separately, is patched in
first; `raw` is not modified):

```java
import ai.depz.sensor.sensors.vl53lx.Vl53lxDecode;

if (s.addr() == Vl53lxDecode.HISTOGRAM_BLOCK_ADDR) {
    Vl53lxDecode.HistogramRaw h = Vl53lxDecode.decodeHistogramRaw(s.data());
    int peakBin = 0;
    for (int i = 1; i < Vl53lxDecode.HISTOGRAM_BINS; i++) {
        if (h.bins()[i] > h.bins()[peakBin]) {
            peakBin = i;
        }
    }
    System.out.printf("#%d range status 0x%02X, %d effective SPADs, VCSEL start %d, peak bin %d = %d%n",
            h.streamCount(), h.rangeStatus(), h.dssActualEffectiveSpads(), h.vcselStart(),
            peakBin, h.bins()[peakBin]);
}
```

Turning bins into up to four targets (preset, VCSEL period, the A/B frame
pairs, phase history) is the full driver's job and is not in this SDK. The
driver keeps state from frame to frame, so a stream fed to it must be decoded
**exactly once, in order** — replaying or skipping frames changes its output.

## Which statuses to plot

The full drivers report one family-wide status. Statuses **0, 6 and 11** are
usable: 6 is the first frame (no predecessor for the wrap check), 11 a merged
pulse. `status == 0` alone drops those frames. The die block's `rangeStatus`
uses the same numbering, so the same rule applies to it:

```java
import java.util.Set;
import ai.depz.sensor.sensors.vl53l4.Vl53l4Uld;
import ai.depz.sensor.sensors.vl53lx.Vl53lxDecode;

/* Statuses a chart should draw (0 valid, 6 first frame, 11 merged pulse). */
Set<Integer> plottable = Set.of(0, 6, 11);

Vl53l4Uld.Results r = Vl53lxDecode.decodeDieBlock(s.data(), Vl53lxDecode.DieVariant.L1);
if (plottable.contains(r.rangeStatus())) {
    System.out.println(r.distanceMm() + " mm");
}
```

The histogram block's `rangeStatus` byte is the raw device register, not the
driver's target status — the rule does not apply to it.

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

## Behind a cover glass

The L1CB is the module meant for a window or an enclosure, and it is the board
that usually needs a crosstalk calibration. Calibration values live in sensor
RAM and are lost on reset: the full driver measures them once, and the
application stores them and re-applies them on every run.

## Gotchas

- **Use `DieVariant.L1`** for the L1CB light driver — the `L4` variant reads
  the signal rate from a different register and scales per SPAD by 256.
- **Plot 0, 6 and 11**, not only 0.
- **Decode every histogram frame exactly once, in order** if you feed a
  target-extraction driver.
- **Set the address width before the first register access** of a session,
  and never under a running stream.
- **Same model id as the VL53L1CX** (`0xEACC`) — the board's USB PID or device
  name, not the silicon, tells them apart.
- **At most 4 interrupt-release steps** — `packStartStream` throws beyond.
