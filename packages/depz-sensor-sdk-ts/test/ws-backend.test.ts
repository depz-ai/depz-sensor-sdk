/**
 * WsBackendTransport + backend HTTP helpers against a fake WebSocket / fetch.
 * Verifies the standalone server contract: health probe, device list, and the
 * WS byte-pipe (open handshake, binary read/write, error + disconnect).
 */

import { afterEach, describe, expect, it, vi } from "vitest";
import {
  DEFAULT_OPEN_TIMEOUT_MS,
  WsBackendTransport,
  backendHealth,
  backendPermissions,
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

// ── the paths the viewer's happy case never takes ─────────────────────────────

describe("WsBackendTransport edges", () => {
  afterEach(() => {
    vi.unstubAllGlobals();
  });

  it("defaults: the global WebSocket, the page origin, a 15 s ceiling, info from the path", async () => {
    const made: FakeWebSocket[] = [];
    vi.stubGlobal(
      "WebSocket",
      class extends FakeWebSocket {
        constructor(url: string) {
          super(url);
          made.push(this);
        }
      },
    );
    vi.stubGlobal("location", { origin: "https://viewer.local:8443" });
    expect(DEFAULT_OPEN_TIMEOUT_MS).toBe(15_000);
    const t = new WsBackendTransport({ path: "COM5" });
    expect(t.info).toEqual({ path: "COM5", usbVendorId: undefined, usbProductId: undefined, serialNumber: undefined });
    const opening = t.open();
    expect(made).toHaveLength(1);
    expect(made[0]!.url).toBe("wss://viewer.local:8443/ws/serial?path=COM5&baud=115200");
    expect(made[0]!.binaryType).toBe("arraybuffer");
    made[0]!.connect();
    made[0]!.recvText('{"opened":true}');
    await opening;
    await t.close();
  });

  it("outside a browser an explicit baseUrl is required", async () => {
    const t = new WsBackendTransport({ path: "/dev/ttyACM0", webSocketFactory: (u) => new FakeWebSocket(u) });
    await expect(t.open()).rejects.toThrow(/explicit baseUrl is required outside a browser/);
    await expect(backendHealth("", fakeFetch({}))).resolves.toBeNull(); // swallowed into null
    await expect(listBackendDevices("", fakeFetch({}))).rejects.toThrow(/explicit baseUrl/);
  });

  it("info overrides win over the path; a non-JSON or non-object text frame is ignored", async () => {
    let ws!: FakeWebSocket;
    const t = new WsBackendTransport({
      baseUrl: BASE,
      path: "/dev/ttyACM0",
      info: { path: "COM9", usbVendorId: 0x1bcf, usbProductId: 0xee08, serialNumber: "SN1" },
      webSocketFactory: (u) => (ws = new FakeWebSocket(u)),
    });
    expect(t.info).toEqual({ path: "COM9", usbVendorId: 0x1bcf, usbProductId: 0xee08, serialNumber: "SN1" });
    let settled = false;
    const opening = t.open().then(() => {
      settled = true;
    });
    ws.connect();
    ws.recvText("not json");
    ws.recvText("42");
    ws.recvText('{"opened":false}');
    ws.recvText('{"error":7}'); // an error that is not a string is not an error frame
    await new Promise((r) => setTimeout(r, 5));
    expect(settled).toBe(false);
    ws.recvText('{"opened":true}');
    await opening;
    expect(settled).toBe(true);
    await t.close();
  });

  it("write() needs an open socket; close() survives a socket that throws, and twice", async () => {
    let ws!: FakeWebSocket;
    const t = new WsBackendTransport({
      baseUrl: BASE,
      path: "/dev/ttyACM0",
      webSocketFactory: (u) => (ws = new FakeWebSocket(u)),
    });
    await expect(t.write(Uint8Array.of(1))).rejects.toThrow(/not open/);
    const opening = t.open();
    // the socket is still CONNECTING: no writes yet
    await expect(t.write(Uint8Array.of(1))).rejects.toThrow(/not open/);
    ws.connect();
    ws.recvText('{"opened":true}');
    await opening;
    ws.close = () => {
      throw new Error("already gone");
    };
    await t.close();
    await t.close(); // ws already null
    await expect(t.write(Uint8Array.of(1))).rejects.toThrow(/not open/);
  });

  it("binary frames of every shape: ArrayBuffer, Uint8Array, another view; empty ones dropped", async () => {
    let ws!: FakeWebSocket;
    const t = new WsBackendTransport({
      baseUrl: BASE,
      path: "/dev/ttyACM0",
      webSocketFactory: (u) => (ws = new FakeWebSocket(u)),
    });
    const opening = t.open();
    ws.connect();
    ws.onmessage?.({ data: new Uint8Array([1, 2]) }); // first data byte = ready
    await opening;
    const got: number[][] = [];
    const reader = (async () => {
      for await (const c of t.readable()) got.push([...c]);
    })();
    await new Promise((r) => setTimeout(r, 5)); // the reader is parked on an empty queue
    ws.onmessage?.({ data: new DataView(new Uint8Array([3, 4, 5]).buffer, 1) });
    ws.onmessage?.({ data: new ArrayBuffer(0) });
    ws.onmessage?.({ data: 12345 }); // neither text nor bytes: nothing
    ws.recvBinary([6]);
    await new Promise((r) => setTimeout(r, 5));
    await t.close();
    await reader;
    expect(got).toEqual([[1, 2], [4, 5], [6]]);
  });

  it("a socket error before the port opens rejects open() and fires onDisconnect once", async () => {
    let ws!: FakeWebSocket;
    const t = new WsBackendTransport({
      baseUrl: BASE,
      path: "/dev/ttyACM0",
      webSocketFactory: (u) => (ws = new FakeWebSocket(u)),
    });
    let fired = 0;
    t.onDisconnect(() => {
      fired += 1;
    });
    const opening = t.open();
    ws.onerror?.({});
    await expect(opening).rejects.toThrow(/cannot reach the standalone backend at ws:\/\/127\.0\.0\.1:8000/);
    ws.onclose?.({}); // the close that follows an error changes nothing
    expect(fired).toBe(1);
    const chunks: Uint8Array[] = [];
    for await (const c of t.readable()) chunks.push(c);
    expect(chunks).toEqual([]);
  });
});

describe("backend HTTP helper edges", () => {
  afterEach(() => {
    vi.unstubAllGlobals();
  });

  it("backendHealth: non-2xx, a non-standalone body and a missing version", async () => {
    expect(await backendHealth(BASE, fakeFetch({ "/api/health": { standalone: true } }, { status: 500 }))).toBeNull();
    expect(await backendHealth(BASE, fakeFetch({ "/api/health": { standalone: false } }))).toBeNull();
    expect(await backendHealth(BASE, fakeFetch({ "/api/health": null }))).toBeNull();
    expect(await backendHealth(BASE, fakeFetch({ "/api/health": { standalone: true } }))).toEqual({
      standalone: true,
      version: "",
    });
  });

  it("listBackendDevices: a non-2xx throws, a body without a list is empty", async () => {
    await expect(listBackendDevices(BASE, fakeFetch({ "/api/devices": {} }, { status: 503 }))).rejects.toThrow(
      /GET \/api\/devices → HTTP 503/,
    );
    expect(await listBackendDevices(BASE, fakeFetch({ "/api/devices": { devices: "nope" } }))).toEqual([]);
    expect(await listBackendDevices(BASE, fakeFetch({ "/api/devices": null }))).toEqual([]);
  });

  it("backendPermissions: the object, null on non-2xx, null on a network error", async () => {
    const perms = { ok: false, os: "linux", issues: ["not in dialout"], fix: { command: "usermod", explanation: "x" } };
    expect(await backendPermissions(BASE, fakeFetch({ "/api/permissions": perms }))).toEqual(perms);
    expect(await backendPermissions(BASE, fakeFetch({}))).toBeNull();
    expect(
      await backendPermissions(BASE, async () => {
        throw new Error("ECONNREFUSED");
      }),
    ).toBeNull();
  });

  it("the default fetch is the global one", async () => {
    const calls: string[] = [];
    vi.stubGlobal("fetch", async (input: string) => {
      calls.push(input);
      return { ok: true, status: 200, json: async () => ({ standalone: true, version: "1.2.3" }) };
    });
    expect(await backendHealth(BASE)).toEqual({ standalone: true, version: "1.2.3" });
    expect(calls).toEqual([`${BASE}/api/health`]);
  });
});
