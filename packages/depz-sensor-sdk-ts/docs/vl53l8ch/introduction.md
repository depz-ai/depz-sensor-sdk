---
title: VL53L8CH — introduction
description: What the VL53L8CH time-of-flight sensor adds over the CX base — Compact-Network-Histogram (CNH) output — plus when to pick it.
---

# VL53L8CH — introduction

The **VL53L8CH** is the **superset** of the [VL53L8CX](../vl53l8cx/introduction.md).
It is the same multi-zone time-of-flight depth sensor — 4×4 / 8×8 zones, the
same per-zone `distanceMm` / signal / ambient / motion surface — **plus**
Compact-Network-Histogram (CNH) output. On the DEPZ boards it ships as the
production part (USB PID `0xED40`); the CX silicon is the dev default.

`Vl53l8Ch` extends `Vl53l8Cx` and inherits every method — so **read the
[VL53L8CX introduction](../vl53l8cx/introduction.md) and
[guide](../vl53l8cx/guide.md) first**. This page and the [CH guide](guide.md)
cover only what CH adds.

## What CH adds — CNH

CNH is a **per-aggregate distance histogram** emitted alongside each ranging
frame. Where CX gives you one distance per zone, CNH gives you the *shape* of
the returned signal over distance for each zone-aggregate — useful for
multi-target scenes, transparent/edge targets, and custom peak-picking.

- `configureCnh(config)` — arm the histogram block for the next
  `startRanging()`. **CH only**: this method does not exist on `Vl53l8Cx`.
- Each frame then carries `cnhRaw`, decoded with `decodeCnh(config, raw)` into
  per-aggregate `hist` / `ambient` values.
- A `CnhConfig` sizes the histogram (start bin, feature length, subsample) and
  maps zones to aggregates (`createAggMap`); its `requiredMemory()` must fit the
  sensor's CNH buffer.

## When to use it

- You need histogram/multi-target data, not just the closest/strongest
  distance per zone.
- Otherwise the [CX base](../vl53l8cx/introduction.md) is simpler and
  identical for plain depth imaging.

## Key concepts

Everything in the [CX key concepts](../vl53l8cx/introduction.md#key-concepts)
applies (init downloads firmware, ranging ≥ 2 Hz, config only while stopped),
plus:

- **`init()`** downloads the CH firmware blob (VL53LMZ ULD 2.0.16). The blob is
  fixed by the class (`Vl53l8Ch` loads `"ch"`), so you don't pass a variant —
  `openDevice()` already resolved `Vl53l8Ch` from the USB PID.
- **CNH is armed before ranging** — call `configureCnh()` while stopped; it
  takes effect on the next `startRanging()`.

The class is [`Vl53l8Ch`](api.md); the CNH types (`CnhConfig`, `decodeCnh`,
`CnhDecoded`, `CnhAggregate`) live in this reference too. See the
[guide](guide.md) for code.
