/**
 * WsBackendTransport + backend HTTP helpers against a fake WebSocket / fetch.
 * Verifies the standalone server contract: health probe, device list, and the
 * WS byte-pipe (open handshake, binary read/write, error + disconnect).
 */

import { describe, expect, it } from "vitest";
import {
  WsBackendTransport,
  backendHealth,
  listBackendDevices,
  type WebSocketLike,
} from "../src/transport/ws-backend.js";

class FakeWebSocket implements WebSocketLike {
  binaryType = "";
  readyState = 0; // CONNECTING
  onopen: ((ev: unknown) => void) | null = null;
  onclose: ((ev: unknown) => void) | null = null;
  onerror: ((ev: unknown) => void) | null = null;
  onmessage: ((ev: { data: unknown }) => void) | null = null;
  readonly sent: Uint8Array[] = [];

  constructor(readonly url: string) {}

  send(data: ArrayBufferView | ArrayBuffer): void {
    this.sent.push(
      data instanceof Uint8Array ? new Uint8Array(data) : new Uint8Array(data as ArrayBuffer),
    );
  }
  close(): void {
    this.readyState = 3; // CLOSED
    this.onclose?.({});
  }

  // test helpers
  connect(): void {
    this.readyState = 1; // OPEN
    this.onopen?.({});
  }
  recvText(s: string): void {
    this.onmessage?.({ data: s });
  }
  recvBinary(bytes: number[]): void {
    this.onmessage?.({ data: new Uint8Array(bytes).buffer });
  }
}

function fakeFetch(routes: Record<string, unknown>, opts: { status?: number } = {}) {
  return async (input: string) => {
    const path = new URL(input).pathname;
    const body = routes[path];
    return {
      ok: (opts.status ?? 200) < 400 && body !== undefined,
      status: body === undefined ? 404 : (opts.status ?? 200),
      json: async () => body,
    };
  };
}

const BASE = "http://127.0.0.1:8000";

describe("backend HTTP helpers", () => {
  it("backendHealth returns the health object for a standalone origin", async () => {
    const f = fakeFetch({ "/api/health": { standalone: true, version: "1.2.3" } });
    expect(await backendHealth(BASE, f as never)).toEqual({ standalone: true, version: "1.2.3" });
  });

  it("backendHealth returns null when not standalone or on failure", async () => {
    expect(await backendHealth(BASE, fakeFetch({}) as never)).toBeNull();
    const throwing = async () => {
      throw new Error("connection refused");
    };
    expect(await backendHealth(BASE, throwing as never)).toBeNull();
  });

  it("listBackendDevices parses the devices array", async () => {
    const devices = [
      {
        path: "/dev/ttyACM0",
        usb_vid: 0x1bcf,
        usb_pid: 0xec78,
        serial: "AB",
        sensor_type: "sr04",
        usb_model_hint: "sr04",
      },
    ];
    const f = fakeFetch({ "/api/devices": { devices } });
    expect(await listBackendDevices(BASE, f as never)).toEqual(devices);
  });
});

describe("WsBackendTransport", () => {
  it("builds the ws url with path + baud and opens on {opened:true}", async () => {
    let url = "";
    const t = new WsBackendTransport({
      baseUrl: BASE,
      path: "/dev/ttyACM0",
      webSocketFactory: (u) => {
        url = u;
        const s = new FakeWebSocket(u);
        queueMicrotask(() => {
          s.connect();
          s.recvText(JSON.stringify({ opened: true }));
        });
        return s;
      },
    });
    await t.open();
    expect(url).toBe("ws://127.0.0.1:8000/ws/serial?path=%2Fdev%2FttyACM0&baud=115200");
    expect(t.info.usbProductId).toBeUndefined();
    await t.close();
  });

  it("honours a baud override from open()", async () => {
    let url = "";
    const t = new WsBackendTransport({
      baseUrl: BASE,
      path: "/dev/ttyACM0",
      webSocketFactory: (u) => {
        url = u;
        const s = new FakeWebSocket(u);
        queueMicrotask(() => {
          s.connect();
          s.recvText(JSON.stringify({ opened: true }));
        });
        return s;
      },
    });
    await t.open({ baudRate: 921600 });
    expect(url).toContain("baud=921600");
    await t.close();
  });

  it("pipes binary frames both ways and honours the first-data readiness", async () => {
    let sock!: FakeWebSocket;
    const t = new WsBackendTransport({
      baseUrl: BASE,
      path: "/dev/ttyACM0",
      webSocketFactory: (u) => {
        sock = new FakeWebSocket(u);
        queueMicrotask(() => {
          sock.connect();
          sock.recvBinary([0xaa, 0xbb]); // first data implies the port is open
        });
        return sock;
      },
    });
    await t.open();

    const it = t.readable();
    const first = await it.next();
    expect(Array.from(first.value!)).toEqual([0xaa, 0xbb]);

    await t.write(new Uint8Array([0x10, 0x20]));
    expect(Array.from(sock.sent[0]!)).toEqual([0x10, 0x20]);

    // more inbound data flows through the same iterator
    sock.recvBinary([0x01]);
    const second = await it.next();
    expect(Array.from(second.value!)).toEqual([0x01]);

    await t.close();
    const end = await it.next();
    expect(end.done).toBe(true);
  });

  it("rejects open() on an {error} control frame", async () => {
    const t = new WsBackendTransport({
      baseUrl: BASE,
      path: "/dev/ttyACM0",
      webSocketFactory: (u) => {
        const s = new FakeWebSocket(u);
        queueMicrotask(() => {
          s.connect();
          s.recvText(JSON.stringify({ error: "could not open port" }));
        });
        return s;
      },
    });
    await expect(t.open()).rejects.toThrow(/could not open port/);
  });

  it("fires onDisconnect when the socket closes unexpectedly, not on close()", async () => {
    let sock!: FakeWebSocket;
    const t = new WsBackendTransport({
      baseUrl: BASE,
      path: "/dev/ttyACM0",
      webSocketFactory: (u) => {
        sock = new FakeWebSocket(u);
        queueMicrotask(() => {
          sock.connect();
          sock.recvText(JSON.stringify({ opened: true }));
        });
        return sock;
      },
    });
    let disconnects = 0;
    t.onDisconnect(() => (disconnects += 1));
    await t.open();

    sock.close(); // unexpected loss
    expect(disconnects).toBe(1);

    // an explicit close() must never fire disconnect
    const t2 = new WsBackendTransport({
      baseUrl: BASE,
      path: "/dev/ttyACM0",
      webSocketFactory: (u) => {
        const s = new FakeWebSocket(u);
        queueMicrotask(() => {
          s.connect();
          s.recvText(JSON.stringify({ opened: true }));
        });
        return s;
      },
    });
    let d2 = 0;
    t2.onDisconnect(() => (d2 += 1));
    await t2.open();
    await t2.close();
    expect(d2).toBe(0);
  });
});

describe("WsBackendTransport open timeout", () => {
  it("rejects when the backend accepts the socket but never opens the port", async () => {
    // The real failure this guards: the WS upgrade succeeds, so neither
    // onerror nor onclose ever fires, but the backend's pyserial open is wedged
    // and {"opened":true} never arrives. Without a ceiling, open() awaits
    // forever and the caller's connect guard never releases — in the viewer,
    // "Scan for sensors" stays disabled until a page reload.
    const t = new WsBackendTransport({
      baseUrl: BASE,
      path: "/dev/ttyACM0",
      openTimeoutMs: 40,
      webSocketFactory: (u) => {
        const s = new FakeWebSocket(u);
        queueMicrotask(() => s.connect()); // socket up, port never opens
        return s;
      },
    });
    await expect(t.open()).rejects.toThrow(/did not open .* within 40 ms/);
  });

  it("does not fire the timeout on a healthy open", async () => {
    const t = new WsBackendTransport({
      baseUrl: BASE,
      path: "/dev/ttyACM0",
      openTimeoutMs: 1000,
      webSocketFactory: (u) => {
        const s = new FakeWebSocket(u);
        queueMicrotask(() => {
          s.connect();
          s.recvText(JSON.stringify({ opened: true }));
        });
        return s;
      },
    });
    await t.open();
    // Outliving the timeout must not retroactively fail anything.
    await new Promise((r) => setTimeout(r, 60));
    await t.close();
  });

  it("still rejects fast on an explicit backend error", async () => {
    // The timeout must not delay a reportable failure.
    const t = new WsBackendTransport({
      baseUrl: BASE,
      path: "/dev/ttyACM0",
      openTimeoutMs: 10_000,
      webSocketFactory: (u) => {
        const s = new FakeWebSocket(u);
        queueMicrotask(() => {
          s.connect();
          s.recvText(JSON.stringify({ error: "failed to open: busy", code: "open_failed" }));
        });
        return s;
      },
    });
    const started = Date.now();
    await expect(t.open()).rejects.toThrow(/busy/);
    expect(Date.now() - started).toBeLessThan(1000);
  });

  it("openTimeoutMs: 0 disables the ceiling", async () => {
    const t = new WsBackendTransport({
      baseUrl: BASE,
      path: "/dev/ttyACM0",
      openTimeoutMs: 0,
      webSocketFactory: (u) => {
        const s = new FakeWebSocket(u);
        queueMicrotask(() => {
          s.connect();
          setTimeout(() => s.recvText(JSON.stringify({ opened: true })), 30);
        });
        return s;
      },
    });
    await t.open();
    await t.close();
  });
});
