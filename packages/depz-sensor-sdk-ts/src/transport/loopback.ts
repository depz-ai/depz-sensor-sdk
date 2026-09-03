/** In-memory transport pair for tests. */

import type { SerialTransport, SerialTransportInfo } from "./types.js";

export class LoopbackTransport implements SerialTransport {
  readonly info: SerialTransportInfo = { path: "loopback" };
  peer: LoopbackTransport | null = null;

  private queue: Uint8Array[] = [];
  private waiter: (() => void) | null = null;
  private closed = false;
  private disconnectCbs: Array<() => void> = [];

  static pair(): [LoopbackTransport, LoopbackTransport] {
    const a = new LoopbackTransport();
    const b = new LoopbackTransport();
    a.peer = b;
    b.peer = a;
    return [a, b];
  }

  async open(): Promise<void> {}

  async write(data: Uint8Array): Promise<void> {
    if (this.closed) throw new Error("loopback transport closed");
    this.peer?.deliver(data.slice());
  }

  private deliver(data: Uint8Array): void {
    this.queue.push(data);
    this.waiter?.();
  }

  async *readable(): AsyncIterableIterator<Uint8Array> {
    for (;;) {
      while (this.queue.length === 0) {
        if (this.closed) return;
        await new Promise<void>((resolve) => (this.waiter = resolve));
        this.waiter = null;
      }
      yield this.queue.shift()!;
    }
  }

  async close(): Promise<void> {
    for (const side of [this, this.peer]) {
      if (side && !side.closed) {
        side.closed = true;
        side.waiter?.();
        side.disconnectCbs.forEach((cb) => cb());
      }
    }
  }

  onDisconnect(cb: () => void): () => void {
    this.disconnectCbs.push(cb);
    return () => {
      this.disconnectCbs = this.disconnectCbs.filter((c) => c !== cb);
    };
  }
}
