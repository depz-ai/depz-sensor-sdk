/**
 * Device discovery & selection (contract 02 §4).
 *
 * The engine here is transport-agnostic: it takes an enumerated list of
 * candidate ports plus a factory that turns a port into a (not-yet-open)
 * `SerialTransport`. The Node entry (`@depz/sensor-sdk/node`) wires it to
 * `serialport`; the browser entry (`@depz/sensor-sdk/web`) wires it to
 * granted WebSerial ports. Both expose `openDevice` / `listDepzDevices`.
 *
 * Selection is by USB identity (`usb-ids.ts`) — never "first port in system
 * order", which could grab an unrelated `/dev/ttyS0`. Mirrors the Python
 * reference `depz_sensor_sdk.discovery`.
 */

import { DepzDevice } from "./device/device.js";
import { DepzError, NoDepzDeviceError } from "./errors.js";
import { parseSoftwareName, type SensorType } from "./protocol/identity.js";
import { isKnownDepzUsb, usbModelHint } from "./protocol/usb-ids.js";
import { Bno086 } from "./sensors/bno086/bno086.js";
import { Sr04 } from "./sensors/sr04.js";
import { Vl53l4Cd } from "./sensors/vl53l4/vl53l4.js";
import { Vl53l8Ch, Vl53l8Cx } from "./sensors/vl53l8/vl53l8.js";
import type { SerialTransport } from "./transport/types.js";

/** One enumerated candidate port (Node: serialport.list; Web: granted port). */
export interface DepzPortInfo {
  /** Stable identifier: an OS path (Node) or a synthetic id (WebSerial). */
  path: string;
  /**
   * USB iSerial as reported by the OS — the sort/selection key. WebSerial
   * cannot read this, so it is undefined there (selection falls back to
   * granted-port order).
   */
  serialNumber?: string;
  usbVid?: number;
  usbPid?: number;
}

/** Result of probing a port (parity with Python `DeviceInfo`). */
export interface DeviceInfo {
  path: string;
  mode: "app" | "bootloader" | "unknown";
  sensorType: SensorType | null;
  softwareName: string;
  fwVersion: string;
  deviceName: string;
  /** Device-reported serial (GET_SERIAL); "" when the device does not answer. */
  serialNumber: string;
  usbVid: number | null;
  usbPid: number | null;
}

/** Turns an enumerated port into a fresh, not-yet-open transport. */
export type TransportFactory = (port: DepzPortInfo) => SerialTransport;

export interface OpenDeviceOptions {
  /** Select the candidate whose USB iSerial equals this exact string. */
  serial?: string;
  /** Per-request timeout for the probe and the opened device (ms). */
  timeoutMs?: number;
}

export interface ListOptions {
  /** Probe only ports with a known DEPZ (vid,pid) (default). */
  matchUsb?: boolean;
  timeoutMs?: number;
}

/** `undefined` = auto; number = Nth candidate; string = port path; DeviceInfo = its port. */
export type OpenTarget = number | string | DeviceInfo | null | undefined;

const DEFAULT_PROBE_TIMEOUT_MS = 200;

// ── selection helpers ─────────────────────────────────────────────────────────

function isKnownPort(p: DepzPortInfo): boolean {
  return isKnownDepzUsb(p.usbVid, p.usbPid);
}

/**
 * Sort ports by USB iSerial ascending; empty/missing serials sort last, ties
 * broken by path. Pure and deterministic — a given bench always maps the same
 * serial to index 0. Exposed so the ordering rule can be golden-tested.
 */
export function orderPortsBySerial(ports: DepzPortInfo[]): DepzPortInfo[] {
  return ports.slice().sort((a, b) => {
    const sa = a.serialNumber ?? "";
    const sb = b.serialNumber ?? "";
    const ea = sa === "" ? 1 : 0;
    const eb = sb === "" ? 1 : 0;
    if (ea !== eb) return ea - eb;
    if (sa !== sb) return sa < sb ? -1 : 1;
    return a.path < b.path ? -1 : a.path > b.path ? 1 : 0;
  });
}

/**
 * Candidate ports (known DEPZ vid/pid) sorted by USB iSerial (index 0 = the
 * one auto-select opens).
 */
function sortedCandidates(ports: DepzPortInfo[]): DepzPortInfo[] {
  return orderPortsBySerial(ports.filter(isKnownPort));
}

export function isDeviceInfo(t: unknown): t is DeviceInfo {
  return typeof t === "object" && t !== null && "path" in t && "sensorType" in t;
}

/**
 * Order *probed* devices by their DEVICE-reported serial (GET_SERIAL, i.e.
 * `DeviceInfo.serialNumber`); empty serials sort last, ties broken by path.
 * This is the selection key for transports that cannot read the USB iSerial
 * (WebSerial), where the serial is only knowable by probing.
 */
export function orderDevicesBySerial(infos: DeviceInfo[]): DeviceInfo[] {
  return infos.slice().sort((a, b) => {
    const ea = a.serialNumber === "" ? 1 : 0;
    const eb = b.serialNumber === "" ? 1 : 0;
    if (ea !== eb) return ea - eb;
    if (a.serialNumber !== b.serialNumber) return a.serialNumber < b.serialNumber ? -1 : 1;
    return a.path < b.path ? -1 : a.path > b.path ? 1 : 0;
  });
}

// ── probing ───────────────────────────────────────────────────────────────────

/**
 * Open `port`, ask its identity (+ device name/serial), then close. Returns
 * null when nothing DEPZ-shaped answers. Probing *opens* the port, so callers
 * should restrict the port list to plausible candidates.
 */
export async function probePort(
  port: DepzPortInfo,
  factory: TransportFactory,
  timeoutMs = DEFAULT_PROBE_TIMEOUT_MS,
): Promise<DeviceInfo | null> {
  const transport = factory(port);
  let dev: DepzDevice;
  try {
    dev = new DepzDevice(transport, { timeoutMs });
    await dev.open();
  } catch {
    try {
      await transport.close();
    } catch {
      /* already gone */
    }
    return null;
  }
  try {
    const ident = parseSoftwareName(await dev.getSoftwareName());
    let deviceName = "";
    try {
      deviceName = await dev.getDeviceName();
    } catch {
      /* optional */
    }
    let serialNumber = "";
    try {
      serialNumber = await dev.getSerialNumber();
    } catch {
      /* optional */
    }
    return {
      path: port.path,
      mode: ident.mode,
      sensorType: ident.sensorType,
      softwareName: ident.softwareName,
      fwVersion: ident.version,
      deviceName,
      serialNumber,
      usbVid: port.usbVid ?? null,
      usbPid: port.usbPid ?? null,
    };
  } catch {
    return null;
  } finally {
    await dev.close();
  }
}

// ── list ──────────────────────────────────────────────────────────────────────

/**
 * Probe candidate ports and return every DEPZ device found, in selection
 * order (sorted by USB iSerial). With `matchUsb` (default) only ports with a
 * known DEPZ (vid,pid) are probed.
 */
export async function listDepzDevicesFrom(
  ports: DepzPortInfo[],
  factory: TransportFactory,
  opts: ListOptions = {},
): Promise<DeviceInfo[]> {
  const timeoutMs = opts.timeoutMs ?? DEFAULT_PROBE_TIMEOUT_MS;
  const matchUsb = opts.matchUsb ?? true;
  const scan = matchUsb ? sortedCandidates(ports) : ports.slice();
  const found: DeviceInfo[] = [];
  for (const port of scan) {
    const info = await probePort(port, factory, timeoutMs);
    if (info !== null) found.push(info);
  }
  return found;
}

// ── open ──────────────────────────────────────────────────────────────────────

function constructSensor(
  transport: SerialTransport,
  sensorType: SensorType | null,
  timeoutMs: number,
  port?: DepzPortInfo,
): DepzDevice {
  switch (sensorType) {
    case "sr04":
      return new Sr04(transport, { timeoutMs });
    case "vl53l8":
      // The USB model hint distinguishes CH silicon (PID 0xED40 → 'vl53l8ch')
      // from CX (PID 0xED4B → 'vl53l8cx'); the dev-default id (0x56DC) carries
      // no silicon hint, so it and any non-CH hint fall through to the CX base
      // class.
      return usbModelHint(port?.usbVid, port?.usbPid) === "vl53l8ch"
        ? new Vl53l8Ch(transport, { timeoutMs })
        : new Vl53l8Cx(transport, { timeoutMs });
    case "vl53l4":
      return new Vl53l4Cd(transport, { timeoutMs });
    case "bno086":
      return new Bno086(transport, { timeoutMs });
    default:
      return new DepzDevice(transport, { timeoutMs });
  }
}

async function openProbed(
  port: DepzPortInfo,
  info: DeviceInfo,
  factory: TransportFactory,
  timeoutMs: number,
): Promise<DepzDevice> {
  if (info.mode === "bootloader") {
    throw new DepzError("device is in bootloader mode; use the bootloader client (contract 06)");
  }
  const transport = factory(port);
  const dev = constructSensor(transport, info.sensorType, timeoutMs, port);
  await dev.open();
  return dev;
}

/**
 * Open the right sensor class for `target` (contract 02 §4).
 *
 * - `undefined`/`null`: the candidate with the alphabetically-smallest USB
 *   iSerial (or the one whose serial == `opts.serial`). No candidate → throws
 *   `NoDepzDeviceError` (never falls back to "first port in system").
 * - `number N`: the Nth candidate, sorted by USB iSerial (0-based). Out of
 *   range → `NoDepzDeviceError`.
 * - `string`: open exactly that port path; warns if its (vid,pid) is not a
 *   known DEPZ id (may be an unprogrammed unit).
 * - `DeviceInfo`: opens its `.path`.
 */
export async function openDeviceFrom(
  ports: DepzPortInfo[],
  factory: TransportFactory,
  target?: OpenTarget,
  opts: OpenDeviceOptions = {},
): Promise<DepzDevice> {
  const timeoutMs = opts.timeoutMs ?? DEFAULT_PROBE_TIMEOUT_MS;

  // Explicit port path (string or DeviceInfo) — open exactly that port.
  if (typeof target === "string" || isDeviceInfo(target)) {
    const path = typeof target === "string" ? target : target.path;
    const enumerated = ports.find((p) => p.path === path);
    const port: DepzPortInfo = enumerated ?? { path };
    const info = await probePort(port, factory, timeoutMs);
    if (info === null) {
      throw new NoDepzDeviceError(`no DEPZ device answered on ${path}`);
    }
    if (!isKnownDepzUsb(port.usbVid, port.usbPid)) {
      console.warn(
        `depz: ${path} has an unrecognized USB id ` +
          `(vid=${fmtId(port.usbVid)}, pid=${fmtId(port.usbPid)}); opening anyway`,
      );
    }
    if (opts.serial !== undefined && enumerated?.serialNumber !== undefined && enumerated.serialNumber !== opts.serial) {
      console.warn(
        `depz: requested serial ${opts.serial} but ${path} enumerates as ` +
          `${enumerated.serialNumber}`,
      );
    }
    return openProbed(port, info, factory, timeoutMs);
  }

  const candidates = sortedCandidates(ports);

  // Nth candidate by USB iSerial.
  if (typeof target === "number") {
    if (!Number.isInteger(target) || target < 0 || target >= candidates.length) {
      throw new NoDepzDeviceError(
        `no DEPZ device at index ${target} (${candidates.length} candidate(s) found)`,
      );
    }
    const port = candidates[target]!;
    if (opts.serial !== undefined && port.serialNumber !== opts.serial) {
      console.warn(
        `depz: index ${target} selects serial ${port.serialNumber ?? "(none)"}, ` +
          `not the requested ${opts.serial}`,
      );
    }
    return openIndexed(port, factory, timeoutMs);
  }

  // Auto: by serial, else smallest serial (index 0).
  let port: DepzPortInfo | undefined;
  if (opts.serial !== undefined) {
    port = candidates.find((p) => p.serialNumber === opts.serial);
    if (port === undefined) {
      throw new NoDepzDeviceError(`no DEPZ device with serial ${opts.serial}`);
    }
  } else {
    port = candidates[0];
    if (port === undefined) {
      throw new NoDepzDeviceError("no DEPZ device found (no port with a known DEPZ USB id)");
    }
  }
  return openIndexed(port, factory, timeoutMs);
}

/**
 * Select and open a device by its DEVICE-reported serial (GET_SERIAL), for
 * transports that cannot enumerate USB iSerials (WebSerial). Probes every
 * given port (open → GET_SERIAL → close), orders the answers by device
 * serial, then selects — `opts.serial` picks an exact match, a numeric
 * `target` picks the Nth, otherwise the smallest serial — and reopens it.
 *
 * The probing cost (opening each granted port once) is the price of not
 * having USB iSerials up front; the *result* matches Node's serial-based
 * selection because for programmed units USB iSerial == device serial.
 */
export async function openDeviceByDeviceSerial(
  ports: DepzPortInfo[],
  factory: TransportFactory,
  target?: number | null,
  opts: OpenDeviceOptions = {},
): Promise<DepzDevice> {
  const timeoutMs = opts.timeoutMs ?? DEFAULT_PROBE_TIMEOUT_MS;
  // matchUsb:false — a granted port is user-curated and may not expose a
  // recognizable vid/pid; the protocol probe is the real filter.
  const infos = orderDevicesBySerial(
    await listDepzDevicesFrom(ports, factory, { matchUsb: false, timeoutMs }),
  );
  let chosen: DeviceInfo | undefined;
  if (opts.serial !== undefined) {
    chosen = infos.find((d) => d.serialNumber === opts.serial);
    if (chosen === undefined) throw new NoDepzDeviceError(`no DEPZ device with serial ${opts.serial}`);
  } else if (typeof target === "number") {
    if (!Number.isInteger(target) || target < 0 || target >= infos.length) {
      throw new NoDepzDeviceError(
        `no DEPZ device at index ${target} (${infos.length} candidate(s) found)`,
      );
    }
    chosen = infos[target]!;
  } else {
    chosen = infos[0];
    if (chosen === undefined) throw new NoDepzDeviceError("no DEPZ device found");
  }
  const port = ports.find((p) => p.path === chosen!.path) ?? { path: chosen!.path };
  return openProbed(port, chosen, factory, timeoutMs);
}

async function openIndexed(
  port: DepzPortInfo,
  factory: TransportFactory,
  timeoutMs: number,
): Promise<DepzDevice> {
  const info = await probePort(port, factory, timeoutMs);
  if (info === null) {
    throw new NoDepzDeviceError(`no DEPZ device answered on ${port.path}`);
  }
  return openProbed(port, info, factory, timeoutMs);
}

function fmtId(v: number | null | undefined): string {
  return v == null ? "?" : `0x${v.toString(16).toUpperCase().padStart(4, "0")}`;
}
