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
 * order", which could grab an unrelated `/dev/ttyS0`. The probe and class
 * resolution here are the reference of contract 02 §4.4; every other SDK
 * (Python `depz_sensor_sdk.discovery` first) matches them.
 */

import { ADOPT_LINK, DepzDevice, HAND_OVER, type DeviceOptions } from "./device/device.js";
import { DepzError, NoDepzDeviceError } from "./errors.js";
import { parseSoftwareName, type SensorType } from "./protocol/identity.js";
import { isKnownDepzUsb, usbModelHint } from "./protocol/usb-ids.js";
import { checkUint } from "./protocol/wire.js";
import { Bno055 } from "./sensors/bno055/bno055.js";
import { Tmf8829 } from "./sensors/tmf8829/tmf8829.js";
import { Bno086 } from "./sensors/bno086/bno086.js";
import { Sr04 } from "./sensors/sr04.js";
import { Vl53l4cd } from "./sensors/vl53l4/vl53l4.js";
import { resolveVl53l7Model } from "./protocol/vl53l7.js";
import { Vl53l5cx, Vl53l7ch, Vl53l7cx } from "./sensors/vl53l7/vl53l7.js";
import { Vl53l8ch, Vl53l8cx } from "./sensors/vl53l8/vl53l8.js";
import * as vl53lxRegistry from "./sensors/vl53lx/uld/registry.js";
import {
  Vl53l0x,
  Vl53l1cb,
  Vl53l1cx,
  Vl53l3cx,
  Vl53l4cx,
  Vl53lx,
} from "./sensors/vl53lx/vl53lx.js";
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

/** The probe's answers (contract 02 §4.4). */
interface ProbeAnswers {
  softwareName: string;
  deviceName: string;
  serialNumber: string;
}

/**
 * The probe of contract 02 §4.4 on an open device: exactly
 * GET_NAME_ACTIVE_SOFTWARE (required — its failure propagates), then
 * GET_DEVICE_NAME and GET_SERIAL (optional — a timeout or error leaves ""),
 * in that order.
 */
async function probeDevice(dev: DepzDevice): Promise<ProbeAnswers> {
  const softwareName = await dev.getSoftwareName();
  let deviceName = "";
  try {
    deviceName = await dev.getDeviceName();
  } catch (err) {
    if (!(err instanceof DepzError)) throw err;
  }
  let serialNumber = "";
  try {
    serialNumber = await dev.getSerialNumber();
  } catch (err) {
    if (!(err instanceof DepzError)) throw err;
  }
  return { softwareName, deviceName, serialNumber };
}

/**
 * Open `port`, run the contract 02 §4.4 probe (GET_NAME_ACTIVE_SOFTWARE,
 * GET_DEVICE_NAME, GET_SERIAL), then close. Returns null when nothing
 * DEPZ-shaped answers. Probing *opens* the port, so callers should restrict
 * the port list to plausible candidates.
 */
export async function probePort(
  port: DepzPortInfo,
  factory: TransportFactory,
  timeoutMs = DEFAULT_PROBE_TIMEOUT_MS,
): Promise<DeviceInfo | null> {
  const probed = await probeOrOpenError(port, factory, timeoutMs);
  return probed instanceof PortOpenFailure ? null : probed;
}

/** Why a port could not be probed at all: its open failed (busy, no permission, gone). */
class PortOpenFailure {
  constructor(readonly error: unknown) {}
}

/**
 * `probePort`, except that a port that cannot be opened yields the open
 * failure instead of null — so `openDeviceFrom` can name it (contract 07 §3).
 */
async function probeOrOpenError(
  port: DepzPortInfo,
  factory: TransportFactory,
  timeoutMs: number,
): Promise<DeviceInfo | PortOpenFailure | null> {
  const transport = factory(port);
  let dev: DepzDevice;
  try {
    dev = new DepzDevice(transport, { timeoutMs });
    await dev.open();
  } catch (err) {
    try {
      await transport.close();
    } catch {
      /* already gone */
    }
    return new PortOpenFailure(err);
  }
  try {
    const answers = await probeDevice(dev);
    const ident = parseSoftwareName(answers.softwareName);
    return {
      path: port.path,
      mode: ident.mode,
      sensorType: ident.sensorType,
      softwareName: ident.softwareName,
      fwVersion: ident.version,
      deviceName: answers.deviceName,
      serialNumber: answers.serialNumber,
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

// ── class resolution (contract 02 §4.4) ──────────────────────────────────────

/**
 * Every name `resolveSensorClass` returns: the canonical sensor classes of
 * contract 07 §1, `"vl53lx"` (the generic 1D-family class: an unstamped
 * board, or a VL53L4CD on the APP_VL53L0_4 firmware), `"tmf8829"`,
 * `"device"` (the base `DepzDevice`: an unknown application, or a name that
 * is neither APP_ nor BOOTDEPZ) and `"bootloader"` (no sensor class: use the
 * contract 06 client). BNO085 boards run APP_BNO086 and resolve to `"bno086"`.
 */
export const SENSOR_CLASS_NAMES = [
  "sr04",
  "vl53l4cd",
  "vl53l8cx",
  "vl53l8ch",
  "vl53l5cx",
  "vl53l7cx",
  "vl53l7ch",
  "vl53l0x",
  "vl53l1cx",
  "vl53l1cb",
  "vl53l3cx",
  "vl53l4cx",
  "vl53lx",
  "bno086",
  "bno055",
  "tmf8829",
  "device",
  "bootloader",
] as const;

/** A canonical class name (see `SENSOR_CLASS_NAMES`). */
export type SensorClassName = (typeof SENSOR_CLASS_NAMES)[number];

/** A sensor class: constructed over a not-yet-open transport, like `DepzDevice`. */
export type SensorClassConstructor = new (transport: SerialTransport, opts?: DeviceOptions) => DepzDevice;

/** Class name → class; "bootloader" has none. */
const SENSOR_CLASSES: Record<Exclude<SensorClassName, "bootloader">, SensorClassConstructor> = {
  sr04: Sr04,
  vl53l4cd: Vl53l4cd,
  vl53l8cx: Vl53l8cx,
  vl53l8ch: Vl53l8ch,
  vl53l5cx: Vl53l5cx,
  vl53l7cx: Vl53l7cx,
  vl53l7ch: Vl53l7ch,
  vl53l0x: Vl53l0x,
  vl53l1cx: Vl53l1cx,
  vl53l1cb: Vl53l1cb,
  vl53l3cx: Vl53l3cx,
  vl53l4cx: Vl53l4cx,
  vl53lx: Vl53lx,
  bno086: Bno086,
  bno055: Bno055,
  tmf8829: Tmf8829,
  device: DepzDevice,
};

/** The SDK class for a class name; throws for "bootloader". */
export function sensorClass(name: SensorClassName): SensorClassConstructor {
  if (name === "bootloader") {
    throw new DepzError("device is in bootloader mode; use the bootloader client (contract 06)");
  }
  const cls = SENSOR_CLASSES[name];
  if (cls === undefined) throw new DepzError(`no sensor class ${String(name)}`);
  return cls;
}

// 1D products with a class of their own; VL53L4CD on APP_VL53L0_4 is generic.
const VL53LX_OWN_CLASS: Record<string, SensorClassName> = {
  VL53L0X: "vl53l0x",
  VL53L1CX: "vl53l1cx",
  VL53L1CB: "vl53l1cb",
  VL53L3CX: "vl53l3cx",
  VL53L4CX: "vl53l4cx",
};

/**
 * The canonical class name for a device's probe answers and its port's USB
 * (vid, pid) — contract 02 §4.4, pure; the reference every SDK matches.
 *
 * `softwareName` is the GET_NAME_ACTIVE_SOFTWARE answer, `deviceName` the
 * GET_DEVICE_NAME answer ("" when there was none); the USB id is
 * null/undefined when unknown (a replay, a bridge).
 *
 * - `BOOTDEPZ_*` → "bootloader".
 * - `vl53l8` → "vl53l8ch" if the USB model is VL53L8CH, else "vl53l8cx".
 * - `vl53l7` → the USB model if VL53L5CX / L7CX / L7CH, else the model
 *   stamped in the device name, else "vl53l7cx" (contract 11 §1).
 * - `vl53lx` → the USB model if one of the six 1D products, else the product
 *   stamped in the device name, else generic; VL53L4CD (no class of its own
 *   on that firmware) and no product → "vl53lx" (contract 12 §1).
 * - any other known token → its class; unknown `APP_*` (or a name that is
 *   neither APP_ nor BOOTDEPZ) → "device".
 *
 * Vectors: `contracts/vectors/probe.json`.
 */
export function resolveSensorClass(
  softwareName: string,
  deviceName: string | null | undefined,
  usbVid: number | null | undefined,
  usbPid: number | null | undefined,
): SensorClassName {
  // Each USB id, when known, is a u16 (contract 07 §3) — even if the other is not.
  if (usbVid != null) checkUint("usbVid", usbVid, 16);
  if (usbPid != null) checkUint("usbPid", usbPid, 16);
  const ident = parseSoftwareName(softwareName);
  if (ident.mode === "bootloader") return "bootloader";
  const model = usbModelHint(usbVid ?? undefined, usbPid ?? undefined);
  switch (ident.sensorType) {
    case "sr04":
      return "sr04";
    case "vl53l8":
      // CX and CH share firmware name and silicon ids: only the production
      // PID tells them apart; the dev default (0483:56DC) falls to the CX base.
      return model === "vl53l8ch" ? "vl53l8ch" : "vl53l8cx";
    case "vl53l7":
      return resolveVl53l7Model(model, deviceName ?? "");
    case "vl53lx": {
      let product: string | null = (model ?? "").toUpperCase() || null;
      if (product === null || !(vl53lxRegistry.PRODUCTS as readonly string[]).includes(product)) {
        product = vl53lxRegistry.productFromBoardName(deviceName ?? "");
      }
      return VL53LX_OWN_CLASS[product ?? ""] ?? "vl53lx";
    }
    case "vl53l4":
      return "vl53l4cd";
    case "bno086":
      return "bno086";
    case "bno055":
      return "bno055";
    case "tmf8829":
      return "tmf8829";
    default:
      return "device";
  }
}

/**
 * Continue the open base device `dev` as `className` on the same connection
 * (contract 02 §4.4): no reopen — the transport, the read pump and the host
 * packet sequence numbers run on. `dev` must be a plain `DepzDevice` with
 * nothing in flight and no stream open; it is spent afterwards (its
 * `close()` is a no-op, requests on it fail) — use the returned object.
 * `"device"` returns `dev` itself; `"bootloader"` throws.
 *
 * `classOptions` go to the class constructor (e.g. a BNO086 / VL53Lx option);
 * the request timeout defaults to `dev.timeoutMs`.
 */
export function attachSensorClass(
  dev: DepzDevice,
  className: SensorClassName,
  classOptions: DeviceOptions & Record<string, unknown> = {},
): DepzDevice {
  const Cls = sensorClass(className);
  if (dev.constructor !== DepzDevice) {
    throw new DepzError(`attachSensorClass needs a plain DepzDevice, got ${dev.constructor.name}`);
  }
  if (Cls === DepzDevice) return dev;
  const { link, transport } = dev[HAND_OVER]();
  // Same synchronous turn as the hand-over: the read pump has no chance to
  // dispatch a packet between the two.
  return new Cls(transport, { timeoutMs: dev.timeoutMs, ...classOptions, [ADOPT_LINK]: link });
}

export interface OpenOnTransportOptions {
  /** The port's USB vendor id, when known (a transport often cannot tell). */
  usbVid?: number | null;
  /** The port's USB product id, when known. */
  usbPid?: number | null;
  /** Per-request timeout for the probe and the opened device (ms). */
  timeoutMs?: number;
  /** Extra options for the resolved class's constructor. */
  classOptions?: DeviceOptions & Record<string, unknown>;
}

/**
 * Open a device on a connection that cannot be reopened — a replay or
 * recording transport, an adopted port (contract 02 §4.4). Opens
 * `transport` (not yet open, as for any `DepzDevice`), runs the same probe as
 * `probePort` on it (GET_NAME_ACTIVE_SOFTWARE, GET_DEVICE_NAME, GET_SERIAL),
 * resolves the class with `resolveSensorClass` from the answers and the
 * given USB id, and continues as that class on the same connection
 * (`attachSensorClass`). On failure the transport is closed. No answer to
 * GET_NAME_ACTIVE_SOFTWARE → `NoDepzDeviceError`; a bootloader → `DepzError`.
 */
export async function openOnTransport(
  transport: SerialTransport,
  opts: OpenOnTransportOptions = {},
): Promise<DepzDevice> {
  const timeoutMs = opts.timeoutMs ?? DEFAULT_PROBE_TIMEOUT_MS;
  const dev = new DepzDevice(transport, { timeoutMs });
  await dev.open();
  try {
    let answers: ProbeAnswers;
    try {
      answers = await probeDevice(dev);
    } catch (err) {
      if (!(err instanceof DepzError)) throw err;
      throw new NoDepzDeviceError(`no DEPZ device answered (${err.message})`);
    }
    const name = resolveSensorClass(answers.softwareName, answers.deviceName, opts.usbVid, opts.usbPid);
    return attachSensorClass(dev, name, { timeoutMs, ...opts.classOptions });
  } catch (err) {
    await dev.close();
    throw err;
  }
}

async function openProbed(
  port: DepzPortInfo,
  info: DeviceInfo,
  factory: TransportFactory,
  timeoutMs: number,
): Promise<DepzDevice> {
  // The probe's connection is closed; the resolved class opens the port afresh.
  const name = resolveSensorClass(info.softwareName, info.deviceName, port.usbVid, port.usbPid);
  const Cls = sensorClass(name);
  const dev = new Cls(factory(port), { timeoutMs });
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
    const info = await probeForOpen(port, factory, timeoutMs);
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
  const info = await probeForOpen(port, factory, timeoutMs);
  return openProbed(port, info, factory, timeoutMs);
}

/** The probe answers of the port `openDeviceFrom` opens; `NoDepzDeviceError`
 * when nothing answers — naming the open failure, as its `cause`, when the
 * port could not be opened (contract 07 §3). */
async function probeForOpen(
  port: DepzPortInfo,
  factory: TransportFactory,
  timeoutMs: number,
): Promise<DeviceInfo> {
  const probed = await probeOrOpenError(port, factory, timeoutMs);
  if (probed instanceof PortOpenFailure) {
    const cause = probed.error;
    const err = new NoDepzDeviceError(
      `no DEPZ device answered on ${port.path}: could not open the port ` +
        `(${cause instanceof Error ? cause.message : String(cause)})`,
    );
    err.cause = cause;
    throw err;
  }
  if (probed === null) {
    throw new NoDepzDeviceError(`no DEPZ device answered on ${port.path}`);
  }
  return probed;
}

function fmtId(v: number | null | undefined): string {
  return v == null ? "?" : `0x${v.toString(16).toUpperCase().padStart(4, "0")}`;
}
