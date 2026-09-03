/**
 * Local-backend transport (standalone `depz-sensor-viewer` app).
 *
 * The online viewer talks to devices over Web Serial. The standalone launcher
 * instead serves the same SPA from `http://127.0.0.1:<port>` and exposes a
 * local backend that speaks to the real serial ports over pyserial. This
 * transport tunnels the exact same byte stream to that backend over a
 * WebSocket, so every layer above (`DepzDevice`, the sensor classes) is
 * byte-for-byte identical to Web Serial / Node.
 *
 * Server contract (fixed — this file must match it exactly):
 *   GET  /api/health       → { standalone: true, version }   (presence ⇒ standalone)
 *   GET  /api/devices      → { devices: [ { path, usb_vid, usb_pid, serial,
 *                                           sensor_type, usb_model_hint } ] }
 *   GET  /api/permissions  → { ok, os, issues, fix: { command, explanation } }
 *   WS   /ws/serial?path=<port>&baud=<n> — a raw byte pipe:
 *        · binary frame you send    = bytes written to the serial port
 *        · binary frame you receive = bytes read from the serial port
 *        · text frame               = JSON control ({"opened":true} / {"error":"…"})
 *
 * Browser-safe: uses only `WebSocket` / `fetch` / `URL`, no Node built-ins.
 */

import type { SerialTransport, SerialTransportInfo } from "./types.js";

/** One device as reported by `GET /api/devices` (snake_case = server JSON). */
export interface BackendDevice {
  /** OS serial-port path the backend opens (e.g. `/dev/ttyACM0`, `COM5`). */
  path: string;
  usb_vid: number | null;
  usb_pid: number | null;
  /** USB iSerial as read by the OS ("" when unavailable). */
  serial: string;
  /** Backend's guess of the sensor family ("sr04" | "vl53l8" | … | ""). */
  sensor_type: string;
  /** Human model hint from the USB id tables ("" when unknown). */
  usb_model_hint: string;
}

/** `GET /api/health` — its mere presence means the viewer is standalone. */
export interface BackendHealth {
  standalone: boolean;
  version: string;
}

/** `GET /api/permissions` — OS-level serial-access diagnostics. */
export interface BackendPermissions {
  ok: boolean;
  os: "linux" | "macos" | "windows";
  issues: string[];
  fix: { command: string; explanation: string };
}

/** Minimal `WebSocket` surface used here — real one or a test fake. */
export interface WebSocketLike {
  binaryType: string;
  readyState: number;
  send(data: ArrayBufferView | ArrayBuffer): void;
  close(): void;
  onopen: ((ev: unknown) => void) | null;
  onclose: ((ev: unknown) => void) | null;
  onerror: ((ev: unknown) => void) | null;
  onmessage: ((ev: { data: unknown }) => void) | null;
}

/** `WebSocket.OPEN`; hard-coded so the module never touches a DOM global at import. */
const WS_OPEN = 1;

export interface WsBackendTransportOptions {
  /**
   * Origin of the standalone server (e.g. `http://127.0.0.1:8000`). Omit /
   * empty to use the page's own origin (`location.origin`) — the usual case,
   * since the SPA is served *by* the backend.
   */
  baseUrl?: string;
  /** Serial-port path from `GET /api/devices` — the `?path=` query value. */
  path: string;
  /** Baud for the `?baud=` query (default 115200; `open()` may override). */
  baudRate?: number;
  /** USB ids / serial to advertise (drives CX-vs-CH labelling upstream). */
  info?: SerialTransportInfo;
  /** WebSocket factory (defaults to the global `WebSocket`); injected in tests. */
  webSocketFactory?: (url: string) => WebSocketLike;
  /**
   * Ceiling on `open()` (ms, default {@link DEFAULT_OPEN_TIMEOUT_MS}).
   *
   * The socket opening is not the same event as the *port* opening: the
   * backend accepts the WebSocket first, then opens the serial port in a
   * thread and only then sends `{"opened":true}`. If that open never returns
   * (a wedged adapter, a port held by something that never lets go), nothing
   * fires `onerror` or `onclose` and `open()` would await forever — taking the
   * caller's connect guard with it, which in the viewer leaves "Scan for
   * sensors" disabled with no way back but a reload. Pass `0` to disable.
   */
  openTimeoutMs?: number;
}

/**
 * Default `open()` ceiling. Generous next to the ~1 s a healthy open takes
 * (including a VL53L8's firmware upload), while still turning a wedged port
 * into a reportable error in bounded time rather than a permanent hang.
 */
export const DEFAULT_OPEN_TIMEOUT_MS = 15_000;

export class WsBackendTransport implements SerialTransport {
  readonly info: SerialTransportInfo;
  private readonly baseUrl: string;
  private readonly path: string;
  private readonly baudRate: number;
  private readonly wsFactory: (url: string) => WebSocketLike;
  private readonly openTimeoutMs: number;

  private ws: WebSocketLike | null = null;
  private queue: Uint8Array[] = [];
  private waiter: (() => void) | null = null;
  private ended = false;
  private disconnected = false;
  private closing = false;
  private streamError: Error | null = null;
  private disconnectCbs: Array<() => void> = [];
  private readyResolve: (() => void) | null = null;
  private readyReject: ((e: Error) => void) | null = null;

  constructor(opts: WsBackendTransportOptions) {
    this.baseUrl = opts.baseUrl ?? "";
    this.path = opts.path;
    this.baudRate = opts.baudRate ?? 115200;
    this.wsFactory =
      opts.webSocketFactory ?? ((url: string) => new WebSocket(url) as unknown as WebSocketLike);
    this.openTimeoutMs = opts.openTimeoutMs ?? DEFAULT_OPEN_TIMEOUT_MS;
    this.info = {
      path: opts.info?.path ?? opts.path,
      usbVendorId: opts.info?.usbVendorId,
      usbProductId: opts.info?.usbProductId,
      serialNumber: opts.info?.serialNumber,
    };
  }

  async open(opts?: { baudRate?: number }): Promise<void> {
    const baud = opts?.baudRate ?? this.baudRate;
    const url = wsSerialUrl(this.baseUrl, this.path, baud);
    const ws = this.wsFactory(url);
    ws.binaryType = "arraybuffer";
    this.ws = ws;

    const ready = new Promise<void>((resolve, reject) => {
      this.readyResolve = resolve;
      this.readyReject = reject;
    });

    ws.onopen = () => {
      // Socket is up; the port itself is "ready" only once the backend sends
      // {"opened":true} or the first data byte (see onMessage).
    };
    ws.onmessage = (ev) => this.onMessage(ev);
    ws.onerror = () => {
      this.failReady(new Error(`cannot reach the standalone backend at ${url}`));
      this.fail(new Error("standalone backend socket error"));
    };
    ws.onclose = () => {
      this.failReady(new Error("the standalone backend closed the serial socket"));
      this.fail(this.streamError ?? new Error("the standalone backend closed the serial socket"));
    };

    // The socket being up is not the port being up: the backend opens the port
    // in a thread and only then sends {"opened":true}. A wedged open fires
    // neither onerror nor onclose, so without this ceiling `await ready` never
    // settles and the caller's connect guard never releases.
    const timer =
      this.openTimeoutMs > 0
        ? setTimeout(() => {
            this.failReady(
              new Error(
                `the standalone backend did not open ${this.path} within ${this.openTimeoutMs} ms`,
              ),
            );
            this.close().catch(() => {});
          }, this.openTimeoutMs)
        : null;
    try {
      await ready;
    } finally {
      if (timer !== null) clearTimeout(timer);
    }
  }

  private onMessage(ev: { data: unknown }): void {
    const data = ev.data;
    if (typeof data === "string") {
      let msg: unknown;
      try {
        msg = JSON.parse(data);
      } catch {
        return; // non-JSON text frame — ignore
      }
      if (msg && typeof msg === "object") {
        const rec = msg as Record<string, unknown>;
        if (typeof rec.error === "string") {
          const err = new Error(`standalone backend: ${rec.error}`);
          this.streamError = err;
          this.failReady(err);
          this.fail(err);
          return;
        }
        if (rec.opened === true) this.markReady();
      }
      return;
    }
    // Binary frame = bytes read from the serial port.
    const bytes = toBytes(data);
    if (bytes.length === 0) return;
    this.markReady(); // first inbound bytes imply the port is open
    this.queue.push(bytes);
    this.waiter?.();
  }

  async write(data: Uint8Array): Promise<void> {
    const ws = this.ws;
    if (!ws || ws.readyState !== WS_OPEN) {
      throw new Error("standalone backend serial socket is not open");
    }
    // Copy: the caller may reuse the buffer before the socket flushes it.
    ws.send(data.slice());
  }

  async *readable(): AsyncIterableIterator<Uint8Array> {
    for (;;) {
      while (this.queue.length === 0) {
        if (this.ended) return;
        await new Promise<void>((resolve) => (this.waiter = resolve));
        this.waiter = null;
      }
      yield this.queue.shift()!;
    }
  }

  async close(): Promise<void> {
    this.closing = true;
    this.finishStream();
    try {
      this.ws?.close();
    } catch {
      /* already gone */
    }
    this.ws = null;
  }

  onDisconnect(cb: () => void): () => void {
    this.disconnectCbs.push(cb);
    return () => {
      this.disconnectCbs = this.disconnectCbs.filter((c) => c !== cb);
    };
  }

  // ── internals ──────────────────────────────────────────────────────────────

  private markReady(): void {
    const r = this.readyResolve;
    this.readyResolve = null;
    this.readyReject = null;
    r?.();
  }

  private failReady(err: Error): void {
    const r = this.readyReject;
    this.readyResolve = null;
    this.readyReject = null;
    r?.(err);
  }

  /** Stop the read stream (used on close AND on unexpected loss). */
  private finishStream(): void {
    if (this.ended) return;
    this.ended = true;
    this.waiter?.();
  }

  /** Unexpected loss: end the stream and fire disconnect (never on explicit close). */
  private fail(err: Error): void {
    this.streamError ??= err;
    this.finishStream();
    if (this.closing || this.disconnected) return;
    this.disconnected = true;
    const cbs = this.disconnectCbs;
    this.disconnectCbs = [];
    cbs.forEach((cb) => cb());
  }
}

// ── HTTP helpers (GET endpoints) ───────────────────────────────────────────────

/** Bound `fetch` that never loses its receiver when passed as a default. */
type FetchLike = (
  input: string,
  init?: { headers?: Record<string, string> },
) => Promise<{
  ok: boolean;
  status: number;
  json(): Promise<unknown>;
}>;

const defaultFetch: FetchLike = (input, init) =>
  (globalThis.fetch as unknown as FetchLike)(input, init);

/**
 * Probe `GET /api/health`. Resolves to the health object when the origin is a
 * standalone backend, or `null` on any failure / when it is the plain online
 * viewer (so a bare `await backendHealth()` is a safe standalone check).
 */
export async function backendHealth(
  baseUrl = "",
  fetchImpl: FetchLike = defaultFetch,
): Promise<BackendHealth | null> {
  try {
    const res = await fetchImpl(apiUrl(baseUrl, "/api/health"), {
      headers: { accept: "application/json" },
    });
    if (!res.ok) return null;
    const j = (await res.json()) as { standalone?: unknown; version?: unknown } | null;
    if (j && j.standalone === true) return { standalone: true, version: String(j.version ?? "") };
    return null;
  } catch {
    return null;
  }
}

/** `GET /api/devices` → the device list (throws on a non-2xx / network error). */
export async function listBackendDevices(
  baseUrl = "",
  fetchImpl: FetchLike = defaultFetch,
): Promise<BackendDevice[]> {
  const res = await fetchImpl(apiUrl(baseUrl, "/api/devices"), {
    headers: { accept: "application/json" },
  });
  if (!res.ok) throw new Error(`GET /api/devices → HTTP ${res.status}`);
  const j = (await res.json()) as { devices?: unknown } | null;
  return Array.isArray(j?.devices) ? (j!.devices as BackendDevice[]) : [];
}

/** `GET /api/permissions` → OS serial-access diagnostics, or `null` on failure. */
export async function backendPermissions(
  baseUrl = "",
  fetchImpl: FetchLike = defaultFetch,
): Promise<BackendPermissions | null> {
  try {
    const res = await fetchImpl(apiUrl(baseUrl, "/api/permissions"), {
      headers: { accept: "application/json" },
    });
    if (!res.ok) return null;
    return (await res.json()) as BackendPermissions;
  } catch {
    return null;
  }
}

// ── URL builders ──────────────────────────────────────────────────────────────

function resolveOrigin(baseUrl: string): string {
  if (baseUrl) return baseUrl;
  const loc = (globalThis as { location?: { origin?: string } }).location;
  if (loc?.origin) return loc.origin;
  throw new Error("WsBackendTransport: an explicit baseUrl is required outside a browser");
}

function apiUrl(baseUrl: string, path: string): string {
  return new URL(path, resolveOrigin(baseUrl)).toString();
}

function wsSerialUrl(baseUrl: string, path: string, baud: number): string {
  const u = new URL("/ws/serial", resolveOrigin(baseUrl));
  u.protocol = u.protocol === "https:" ? "wss:" : "ws:";
  u.searchParams.set("path", path);
  u.searchParams.set("baud", String(baud));
  return u.toString();
}

function toBytes(data: unknown): Uint8Array {
  if (data instanceof Uint8Array) return data;
  if (data instanceof ArrayBuffer) return new Uint8Array(data);
  if (ArrayBuffer.isView(data)) {
    const v = data as ArrayBufferView;
    return new Uint8Array(v.buffer, v.byteOffset, v.byteLength);
  }
  return new Uint8Array(0);
}
