/**
 * Regression: Vl53l8.getFrame() must fail fast when the device disconnects
 * (its temporary queue is registered for teardown), not stall until the
 * frame timeout elapses.
 */

import { describe, expect, it } from "vitest";
import { DepzError, LoopbackTransport, Vl53l8 } from "../src/index.js";

describe("Vl53l8.getFrame teardown", () => {
  it("rejects promptly with 'device closed' on disconnect", async () => {
    const [host] = LoopbackTransport.pair();
    const dev = new Vl53l8(host);
    await dev.open();
    // Ask for a frame with a long timeout, then drop the link. The reject must
    // come from the close path, well before the 30 s timeout.
    const started = Date.now();
    const waiting = dev.getFrame(30_000);
    await dev.close();
    await expect(waiting).rejects.toThrow(DepzError);
    await expect(waiting).rejects.toThrow(/device closed/);
    expect(Date.now() - started).toBeLessThan(2_000);
  });
});
