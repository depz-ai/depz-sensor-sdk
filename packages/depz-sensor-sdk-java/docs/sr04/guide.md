# SR04 — user guide

Hands-on guide to the SR04: the live device (`ai.depz.sensor.devices.Sr04Device`)
and the `Sr04` codecs under it. For what the sensor is and its concepts, read
the [introduction](introduction.md); for exact signatures see the
[API reference](api.md).

## Contents

- [Live device](#live-device)
- [The codec surface](#the-codec-surface)
- [Decode a measurement](#decode-a-measurement)
- [Single shot vs the loop](#single-shot-vs-the-loop)
- [Configuration](#configuration)
- [Temperature-compensated distance](#temperature-compensated-distance)
- [Gotchas](#gotchas)

## Live device

```java
import ai.depz.sensor.devices.*;
import java.time.Duration;

try (Sr04Device dev = DepzDevices.openSr04()) {
    dev.setSamplePeriodUs(100_000);                 // a start every 100 ms at most
    int decay = dev.setEchoDecayUs(5_000);          // returns the value the device keeps (4000..65000)
    Sr04Measurement once = dev.measureOnce();       // ~65 ms worst case: the echo window
    System.out.println(once.valid() ? once.distanceMm() + " mm" : "no echo");

    try (Subscription<Sr04Measurement> loop = dev.measurements()) {
        dev.start();
        Sr04Measurement m = loop.take(Duration.ofSeconds(1));
        System.out.println(m.source() + " " + m.distanceMmAt(24.0));   // speed of sound at 24 °C
        dev.stop();
    }
}
```

- `measureOnce()` throws `BusyException` while the loop runs.
- A SYNC_IN edge (`setSyncPin`) also triggers single shots; they arrive on the
  stream with `source() == "once"`.
- `onMeasurement(callback)` runs on the reader thread; close the returned
  handle to stop.

The sections below are the codecs under the device, for replaying captures or
writing your own transport.

## The codec surface

`Sr04` is a final class of static codecs plus three nested types:

- `Sr04.Sr04Cmd` — host→device opcodes (`MEASURE_ONCE`, `START_MEASUREMENT_LOOP`,
  `SET_SAMPLE_PERIOD`, …), each with a `.value` byte.
- `Sr04.Sr04Rpt` — device→host report ids (`DATA`, `SAMPLE_PERIOD`, `ECHO_DECAY`).
- `Sr04.Sr04Data` — one decoded ranging result.

Wrap them in the shared framing/parse layer from the
[common guide](../guide.md#getting-started).

## Decode a measurement

A ranging report is a `Packet` whose `cmd` equals `Sr04.Sr04Rpt.DATA.value`:

```java
import ai.depz.sensor.transport.*;
import ai.depz.sensor.protocol.Sr04;

PacketParser parser = new PacketParser();
for (Event ev : parser.feed(bytesFromPort)) {
    if (ev instanceof Packet p && p.cmd() == Sr04.Sr04Rpt.DATA.value) {
        Sr04.Sr04Data d = Sr04.Sr04Data.unpack(p.payload());
        Double mm = Sr04.distanceMmFromEcho(d.echoTimeUs(), null);
        String src = d.sourceCmd() == Sr04.Sr04Cmd.MEASURE_ONCE.value ? "once" : "loop";
        System.out.printf("%s  %s%n", mm == null ? "no echo" : mm + " mm", src);
    }
}
```

`Sr04Data` carries `sourceCmd`, `timestampUs` (MCU µs) and the raw `echoTimeUs`.

## Single shot vs the loop

Build the command bytes with `Framing.buildPacket` and the `Sr04Cmd` opcodes:

```java
import ai.depz.sensor.transport.Framing;

byte[] once  = Framing.buildPacket(Sr04.Sr04Cmd.MEASURE_ONCE.value);
byte[] start = Framing.buildPacket(Sr04.Sr04Cmd.START_MEASUREMENT_LOOP.value);
byte[] stop  = Framing.buildPacket(Sr04.Sr04Cmd.STOP_MEASUREMENT_LOOP.value);
```

Both the one-shot reply and the loop samples arrive as `RPT` `DATA` packets and
decode with `Sr04Data.unpack`. A single shot (or a SYNC_IN edge) has
`sourceCmd == MEASURE_ONCE.value`; a loop sample has
`sourceCmd == START_MEASUREMENT_LOOP.value`.

## Configuration

Each setting is a get/set opcode pair with a small payload codec. Values are
microseconds.

```java
// sample period (u32 µs)
byte[] setPeriod = Framing.buildPacket(
        Sr04.Sr04Cmd.SET_SAMPLE_PERIOD.value, Sr04.packSamplePeriod(20_000), seq, CrcType.NONE);
long periodUs = Sr04.unpackSamplePeriod(reportPayload);   // from RPT SAMPLE_PERIOD

// echo decay (u16 µs) — the device clamps to 4000..65000 and reports the effective value
byte[] setDecay = Framing.buildPacket(
        Sr04.Sr04Cmd.SET_ECHO_DECAY.value, Sr04.packEchoDecay(3_000), seq, CrcType.NONE);
int decayUs = Sr04.unpackEchoDecay(reportPayload);        // → 4000 (clamped up)
```

The configured period is a **ceiling**: the device throttles the effective rate
by the echo window, so a read-back returns the stored value
(`SAMPLE_PERIOD_DEFAULT_US` = 50000), not the realised rate. Defaults and limits
are constants on `Sr04` (`ECHO_DECAY_DEFAULT_US`, `ECHO_DECAY_MIN_US`,
`ECHO_DECAY_MAX_US`).

## Temperature-compensated distance

`distanceMmFromEcho` assumes 343 m/s by default. Pass the air temperature (°C)
for a compensated speed of sound `c = 331.3 + 0.606·T`:

```java
Double d20 = Sr04.distanceMmFromEcho(d.echoTimeUs(), null);   // 343 m/s
Double d30 = Sr04.distanceMmFromEcho(d.echoTimeUs(), 30.0);   // ≈ 349.5 m/s at 30 °C
```

Both return `null` for a no-echo timeout.

## Gotchas

- **Always null-check the distance.** A no-echo timeout is
  `echoTimeUs == Sr04.ECHO_TIMEOUT` (`0xFFFF`), and `distanceMmFromEcho` returns
  `null`.
- **Echo time is authoritative; distance is derived.** Keep `echoTimeUs` if you
  need to re-derive distance with a different speed of sound.
- **Configured period is a ceiling, not the realised rate** — the echo window
  throttles it; a read-back returns the stored value.
- **Echo decay is a u16** — values above 65535 can't be sent; the device clamps
  to 4000..65000 and reports the effective value back on `RPT ECHO_DECAY`.
- **`sourceCmd` disambiguates the stream** — loop samples and unsolicited
  SYNC_IN single shots share the same report id.
