/**
 * WebSerialTransport lifecycle. Regression guard: the `disconnect` listener
 * added on open() must be removed on close(), so the probe→reopen cycle in
 * discovery never leaks listeners on a shared granted port.
 */

import { describe, expect, it } from "vitest";
import { WebSerialTransport, type WebSerialPortLike } from "../src/transport/webserial.js";

class FakePort implements WebSerialPortLike {
  readonly listeners = new Map<string, Set<() => void>>();
  readable: ReadableStream<Uint8Array> | null = null;
  writable: WritableStream<Uint8Array> | null = null;

  constructor(
    readonly usbVendorId = 0x1bcf,
    readonly usbProductId = 0xec78,
  ) {}

  getInfo(): { usbVendorId?: number; usbProductId?: number } {
    return { usbVendorId: this.usbVendorId, usbProductId: this.usbProductId };
  }
  async open(): Promise<void> {}
  async close(): Promise<void> {}

  addEventListener(type: "disconnect", cb: () => void): void {
    (this.listeners.get(type) ?? this.listeners.set(type, new Set()).get(type)!).add(cb);
  }
  removeEventListener(type: "disconnect", cb: () => void): void {
    this.listeners.get(type)?.delete(cb);
  }

  count(): number {
    return this.listeners.get("disconnect")?.size ?? 0;
  }
  emitDisconnect(): void {
    for (const cb of [...(this.listeners.get("disconnect") ?? [])]) cb();
  }
}

describe("WebSerialTransport lifecycle", () => {
  it("removes the disconnect listener on close (no leak across reopen)", async () => {
    const port = new FakePort();
    const t = new WebSerialTransport(port);
    await t.open();
    expect(port.count()).toBe(1);
    await t.close();
    expect(port.count()).toBe(0);
    // A second open/close cycle on the same port must not accumulate.
    await t.open();
    await t.close();
    expect(port.count()).toBe(0);
  });

  it("fires onDisconnect before close, never after", async () => {
    const port = new FakePort();
    const t = new WebSerialTransport(port);
    let fired = 0;
    t.onDisconnect(() => (fired += 1));
    await t.open();
    port.emitDisconnect();
    expect(fired).toBe(1);
    await t.close();
    port.emitDisconnect(); // listener gone — must be a no-op
    expect(fired).toBe(1);
  });

  it("exposes vid/pid from getInfo()", () => {
    const t = new WebSerialTransport(new FakePort(0x1bcf, 0xee08));
    expect(t.info.usbVendorId).toBe(0x1bcf);
    expect(t.info.usbProductId).toBe(0xee08);
  });
});
