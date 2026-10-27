/**
 * `@depz/sensor-sdk/node` without the optional `serialport` peer dependency:
 * every entry point fails with one message naming the package to install,
 * and the original import error rides along as `cause`.
 */

import { describe, expect, it, vi } from "vitest";
import { NodeSerialTransport, listDepzDevices, listSerialPorts, openDevice } from "../src/transport/node.js";

vi.mock("serialport", () => {
  throw new Error("Cannot find package 'serialport'");
});

describe("node transport without serialport", () => {
  const message = /optional peer dependency 'serialport' is required for NodeSerialTransport \(npm i serialport\)/;

  it("listSerialPorts / listDepzDevices / openDevice name the missing package", async () => {
    await expect(listSerialPorts()).rejects.toThrow(message);
    await expect(listDepzDevices()).rejects.toThrow(message);
    await expect(openDevice()).rejects.toThrow(message);
  });

  it("NodeSerialTransport.open() too, with the import error as cause", async () => {
    const t = new NodeSerialTransport("/dev/ttyACM0");
    let caught: unknown = null;
    try {
      await t.open();
    } catch (e) {
      caught = e;
    }
    const err = caught as Error & { cause?: unknown };
    expect(err).toBeInstanceOf(Error);
    expect(err.message).toMatch(message);
    expect(err.cause).toBeInstanceOf(Error); // the import failure itself (vitest wraps the mock's)
  });
});
