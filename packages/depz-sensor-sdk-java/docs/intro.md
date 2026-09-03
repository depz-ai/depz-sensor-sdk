# Java SDK

`depz-sensor-sdk-java` is the **contract-first Java port of the DEPZ sensor
protocol foundation**: pure, host-verifiable codecs that turn wire bytes into
typed Java records and back. It is byte-exact with the protocol contracts and
golden vectors, cross-checked against the Python and TypeScript reference
SDKs. Java 17+, no runtime dependencies.

**Working with one specific sensor?** Each has its own introduction, guide
and API reference:

- **[SR04](sr04/introduction.md)** — ultrasonic ranging: one distance per ping.
- **[VL53L8CX](vl53l8cx/introduction.md)** — 8×8 ToF depth frames.
- **[VL53L8CH](vl53l8ch/introduction.md)** — the CX superset with CNH histograms.
- **[BNO086](bno086/introduction.md)** — 9-axis IMU: orientation and motion.

## Install

Published on Maven Central as `io.github.depz-ai:depz-sensor-sdk` (0.1.4).

Gradle:

```kotlin
implementation("io.github.depz-ai:depz-sensor-sdk:0.1.4")
```

Maven:

```xml
<dependency>
  <groupId>io.github.depz-ai</groupId>
  <artifactId>depz-sensor-sdk</artifactId>
  <version>0.1.4</version>
</dependency>
```

The Maven group is `io.github.depz-ai`; the Java package namespace you import is
`ai.depz.sensor.*` (they are different on purpose).

## Build from source

The repo itself uses no build system — plain `javac` (verified on JDK 21):

```sh
./build.sh          # compile main + test harness into ./out
./test.sh           # build, then run every golden vector
```

The package is a flat source tree under `src/main/java/ai/depz/sensor/`; add
`out/` (or the sources) to your classpath and import `ai.depz.sensor.*`.

## Where to next

- **Your sensor's pages** — [SR04](sr04/introduction.md) ·
  [VL53L8CX](vl53l8cx/introduction.md) · [VL53L8CH](vl53l8ch/introduction.md) ·
  [BNO086](bno086/introduction.md): introduction, hands-on guide, and the
  sensor's own API reference.
- **[Guide](guide.md)** — the SDK-wide walkthrough: build, scope, the mental
  model, framing & CRCs, USB identity, time-sync, datasets.
- **[API Reference](api.md)** — the whole public surface, generated from the
  Java sources.
- **Docs for LLMs** — the sensor SDK documentation as raw Markdown:
  [/llms-full.txt](/llms-full.txt).
- **Source & license** — the SDKs are open source (MIT):
  [github.com/depz-ai/depz-sensor-sdk](https://github.com/depz-ai/depz-sensor-sdk).
