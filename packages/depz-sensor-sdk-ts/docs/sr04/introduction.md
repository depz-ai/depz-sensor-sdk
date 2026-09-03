---
title: SR04 — introduction
description: What the HC-SR04 ultrasonic ranging sensor is, what it measures, when to use it, and its key concepts.
---

# SR04 — introduction

The **HC-SR04** is an ultrasonic ranging device. It emits a 40 kHz burst and
times the echo; the DEPZ firmware does the ranging on-device and reports the
**echo time in microseconds**, which the SDK converts to a distance.

## What it measures

- **One distance per shot**, computed from the round-trip echo time at 343 m/s
  (`distanceMm = echoTimeUs · 343 / 2000`).
- Practical range ≈ 2 cm … 4 m; a missing echo is reported as an explicit
  **timeout sentinel** (`valid === false`, `distanceMm === null`) rather than a
  bogus number.

## When to use it

- Cheap, robust presence/level/proximity sensing where a single axial distance
  is enough (tank level, bin fullness, "is something in front of me").
- Not for shape or a depth image — that is the VL53L8's job.

## Key concepts

- **Measurement source** — every result is tagged `"once"` (a host
  `measureOnce()` or an AUX `SYNC_IN` hardware trigger) or `"loop"` (a sample
  from the free-running measurement loop).
- **Sample period** — the minimum interval between measurement starts
  (`getSamplePeriodUs` / `setSamplePeriodUs`, default 50 000 µs = 20 Hz). The
  effective rate is auto-throttled by the echo window.
- **Echo decay** — a settle pause the device clamps to 4 000 … 65 000 µs;
  `setEchoDecayUs()` re-reads and returns the value actually in effect.
- **Timestamps** — device microseconds as `bigint` on every measurement.

The SDK class is [`Sr04`](../api.md); its results are `Sr04Measurement`
`{ timestampUs, echoTimeUs, source, valid, distanceMm }`. See the
[guide](guide.md) for runnable code.
