/**
 * Bno086 everyday-sensor shortcuts (contract 05 / 07): each `enableX()` is a
 * Set Feature for the right SH-2 sensor id at the documented default rate,
 * and an explicit rate overrides the default. Runs against the fake bridge +
 * sensor hub (test/fake-bno086.ts).
 */

import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { Bno086, SensorId, buildSetFeature } from "../src/index.js";
import { FakeBno086 } from "./fake-bno086.js";
import { toHex } from "./golden/vectors.js";

let fake: FakeBno086;
let dev: Bno086;

beforeEach(async () => {
  fake = new FakeBno086();
  dev = new Bno086(fake.transport, { timeoutMs: 1000 });
  await dev.open();
});

afterEach(async () => {
  await dev.close();
  await fake.close();
});

type Shortcut = (hz?: number) => Promise<unknown>;

const CASES: Array<[name: string, sensor: number, defaultHz: number]> = [
  ["enableRotationVector", SensorId.RotationVector, 100],
  ["enableGameRotationVector", SensorId.GameRotationVector, 100],
  ["enableAccelerometer", SensorId.Accelerometer, 100],
  ["enableGyroscope", SensorId.Gyroscope, 100],
  ["enableMagnetometer", SensorId.Magnetometer, 50],
  ["enableLinearAcceleration", SensorId.LinearAcceleration, 100],
  ["enableGravity", SensorId.Gravity, 100],
  ["enableGyroIntegratedRv", SensorId.GyroIntegratedRv, 400],
];

describe("bno086 enable shortcuts", () => {
  for (const [name, sensor, defaultHz] of CASES) {
    it(`${name}() enables sensor 0x${sensor.toString(16)} at ${defaultHz} Hz by default`, async () => {
      const fn = (dev as unknown as Record<string, Shortcut>)[name]!.bind(dev);
      const resp = (await fn()) as { sensorId: number; intervalUs: number } | null;
      const us = Math.round(1e6 / defaultHz);
      expect(toHex(fake.lastSetFeature!)).toBe(toHex(buildSetFeature(sensor, us)));
      expect(resp).not.toBeNull();
      expect(resp!.sensorId).toBe(sensor);
      expect(fake.features.get(sensor)).toBe(us);
    });
  }

  it("an explicit rate overrides the default", async () => {
    await dev.enableAccelerometer(20);
    expect(fake.features.get(SensorId.Accelerometer)).toBe(50_000);
    await dev.enableMagnetometer(10);
    expect(fake.features.get(SensorId.Magnetometer)).toBe(100_000);
  });
});
