/**
 * Contract 15 device scenarios: the TypeScript runner.
 *
 * Each contracts/scenarios/<id>.json names a committed real-hardware capture
 * and the API calls that produced it. The runner opens the capture as its
 * `probe` field says (contract 15 §1) over a causal, strictTx ReplayTransport
 * (every byte the SDK writes must equal the recorded tx stream), executes the
 * steps (§2), canonicalises each result (§3) and matches it against `expect`.
 * Python, C and C++ run the same files.
 *
 * - `probe: "canonical"` — `openOnTransport`: the contract 02 §4.4 probe on
 *   the replay, then the resolved class continues on the same connection.
 * - `probe: "legacy"` — captures that hold the probe Python made before
 *   §4.4: GET_NAME_ACTIVE_SOFTWARE, plus GET_DEVICE_NAME for the L5/L7 and 1D
 *   families. The runner issues exactly those, resolves the class with
 *   `resolveSensorClass` and attaches it in place (`attachSensorClass`).
 *
 * `usb_model` stands in for the USB id a replay has no way to see.
 *
 * $DEPZ_SCENARIOS_DIR points the runner at another scenario directory (used to
 * prove it fails on a corrupted copy).
 */

import { readdirSync, readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { describe, it } from "vitest";
import { DepzDevice, type StreamQueue } from "../src/device/device.js";
import { attachSensorClass, openOnTransport, resolveSensorClass } from "../src/discovery.js";
import { DepzError, DepzTimeoutError, StatusError } from "../src/errors.js";
import { parseSoftwareName } from "../src/protocol/identity.js";
import { DEPZ_USB_MODELS, DEPZ_VID } from "../src/protocol/usb-ids.js";
import { Bno055 } from "../src/sensors/bno055/bno055.js";
import type { Bno055OprMode, Bno055Units } from "../src/sensors/bno055/regs.js";
import { Bno086 } from "../src/sensors/bno086/bno086.js";
import { Sr04 } from "../src/sensors/sr04.js";
import { Vl53l4cd } from "../src/sensors/vl53l4/vl53l4.js";
import { Vl53l5cx, Vl53l7ch, Vl53l7cx } from "../src/sensors/vl53l7/vl53l7.js";
import { CnhConfig } from "../src/sensors/vl53l8/cnh.js";
import { Vl53l8ch, Vl53l8cx } from "../src/sensors/vl53l8/vl53l8.js";
import { Vl53Error } from "../src/sensors/vl53lx/uld/link.js";
import {
  Vl53l0x,
  Vl53l1cb,
  Vl53l1cx,
  Vl53l3cx,
  Vl53l4cx,
  Vl53lx,
} from "../src/sensors/vl53lx/vl53lx.js";
import { ReplayTransport } from "../src/transport/replay.js";
import { seen } from "./golden/vectors.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const CONTRACTS = path.resolve(here, "../../../contracts");
const SCENARIOS = process.env.DEPZ_SCENARIOS_DIR ?? path.join(CONTRACTS, "scenarios");
const RECORDINGS = path.join(CONTRACTS, "vectors", "recordings");
const FILES = readdirSync(SCENARIOS).filter((f) => f.endsWith(".json")).sort();

const TIMEOUT_MS = 2000; // request timeout, as the replay tests always used
const TAKE_TIMEOUT_MS = 5000; // per streamed item; replay serves them instantly

type Json = null | boolean | number | string | Json[] | { [k: string]: Json };
type Dev = DepzDevice & Record<string, unknown>;
type Ctor = new (t: ReplayTransport, o: object) => DepzDevice;

interface Step {
  call?: string;
  args?: Json[];
  expect?: Json;
  error?: string;
  subscribe?: string;
  as?: string;
  take?: string;
  count?: number;
  get?: string;
}

interface Scenario {
  schema: string;
  id: string;
  recording: string;
  class: string;
  probe?: "legacy" | "canonical";
  usb_model?: string;
  identity?: Json;
  steps: Step[];
  end?: "open" | "exhausted";
}

// Contract 07 §1 layer 3: canonical class → TS class.
const CLASSES: Record<string, Ctor> = {
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
};

type Adapter = (dev: Dev, ...args: unknown[]) => unknown;

/**
 * The alias table (§2): canonical names / shapes the TS binding spells
 * differently, per class. Everything else goes by the casing rule
 * (snake_case → camelCase); a name that resolves to nothing fails loudly.
 */
const ALIASES: Array<[Function, string, Adapter]> = [
  // Python returns a (budget, inter) tuple; TS a RangeTiming record.
  [Vl53l4cd, "get_range_timing", async (dev) => {
    const t = await (dev as unknown as Vl53l4cd).getRangeTiming();
    return [t.timingBudgetMs, t.interMeasurementMs];
  }],
  // Python keyword/positional arguments → TS options objects.
  [Vl53lx, "init", (dev, driver, product) =>
    (dev as unknown as Vl53lx).init(driver as string | null, { product: product as string | null })],
  [Vl53lx, "configure", (dev, budgetMs, interMs, mode) =>
    (dev as unknown as Vl53lx).configure({
      budgetMs: budgetMs as number, interMs: interMs as number, mode: mode as string | null,
    })],
  [Bno055, "configure", (dev, mode, units) =>
    (dev as unknown as Bno055).configure({ mode: mode as Bno055OprMode, units: units as Bno055Units })],
  // No device-level accessor (Python neither): the bound driver carries them.
  [Vl53lx, "budget_range", (dev) => (dev as unknown as Vl53lx).driver.BUDGET_MS],
  [Vl53lx, "driver_reach_mm", (dev) => (dev as unknown as Vl53lx).driver.reachMm()],
  // Streams: (maxsize) — BNO086 takes a sensor filter first.
  [Bno086, "reports", (dev, maxsize) => (dev as unknown as Bno086).reports(null, maxsize as number)],
];

type RecordBuilder = (rec: Record<string, Json>) => unknown;

function cnhConfig(rec: Record<string, Json>): CnhConfig {
  const cfg = new CnhConfig();
  cfg.initConfig(rec.start_bin as number, rec.num_bins as number, rec.sub_sample as number);
  cfg.createAggMap(rec.resolution as number, ...(rec.agg_map as [number, number, number, number, number, number]));
  return cfg;
}

// Record arguments (§2): (class, call, position) → builder of the TS type.
const RECORD_ARGS: Array<[Function, string, number, RecordBuilder]> = [
  [Bno055, "configure", 1, (r): Bno055Units => ({
    accelMg: r.accel_mg as boolean, gyroRps: r.gyro_rps as boolean, eulerRad: r.euler_rad as boolean,
    tempF: r.temp_f as boolean, android: r.android as boolean,
  })],
  [Vl53l8ch, "configure_cnh", 0, cnhConfig],
  [Vl53l7ch, "configure_cnh", 0, cnhConfig],
];

class StepFailure extends Error {}
const fail = (msg: string): never => {
  throw new StepFailure(msg);
};

/** §4: the canonical kind of a TS error, null for anything else. */
function errorKind(err: unknown): string | null {
  if (err instanceof DepzTimeoutError) return "timeout";
  if (err instanceof StatusError) return err.status === 0x07 ? "not_supported" : "device_error";
  if (err instanceof RangeError || err instanceof TypeError) return "invalid_argument";
  // The VL53Lx drivers refuse a mode they lack with Vl53Error, the same type
  // as a sensor that misbehaved; only the message tells them apart.
  if (err instanceof Vl53Error && String(err.message).startsWith("no such mode")) return "invalid_argument";
  if (err instanceof DepzError && err.message.includes(" does not support ")) return "not_supported";
  return null;
}

const snake = (k: string) => k.replace(/[A-Z]/g, (c) => `_${c.toLowerCase()}`);
const camel = (k: string) => k.replace(/_([a-z0-9])/g, (_, c: string) => c.toUpperCase());

function hex(b: Uint8Array): string {
  return Array.from(b, (x) => x.toString(16).padStart(2, "0")).join("");
}

/**
 * TS carries both Python `bytes` and uint8 numpy arrays as Uint8Array; these
 * record fields are the `bytes` ones (→ hex), every other Uint8Array is an
 * array of numbers.
 */
const BYTES_FIELDS = new Set(["raw", "cnh_raw"]);

/** §3: a result as canonical JSON (records keyed by the Python field names). */
function canon(v: unknown, field = ""): Json {
  if (v === null || v === undefined) return null;
  if (typeof v === "boolean" || typeof v === "string" || typeof v === "number") return v;
  if (typeof v === "bigint") {
    if (v > BigInt(Number.MAX_SAFE_INTEGER) || v < BigInt(Number.MIN_SAFE_INTEGER)) {
      throw new Error(`bigint ${v} beyond the exact JSON range`);
    }
    return Number(v);
  }
  if (v instanceof Uint8Array && BYTES_FIELDS.has(field)) return hex(v);
  if (ArrayBuffer.isView(v)) return Array.from(v as unknown as ArrayLike<number>, (x) => canon(x));
  if (Array.isArray(v)) return v.map((x) => canon(x));
  if (v instanceof Map) {
    return Object.fromEntries([...v.entries()].map(([k, x]) => [String(k), canon(x)]));
  }
  if (v instanceof Set) return [...v].map((x) => canon(x));
  if (typeof v === "object") {
    const out: { [k: string]: Json } = {};
    for (const [k, x] of Object.entries(v)) {
      if (k.startsWith("_") || typeof x === "function") continue;
      out[snake(k)] = canon(x, snake(k));
    }
    return out;
  }
  throw new Error(`no canonical form for ${typeof v}`);
}

const show = (v: unknown) => {
  const s = JSON.stringify(v);
  return s === undefined ? String(v) : s.length > 200 ? `${s.slice(0, 200)}…` : s;
};

/** §3 matching; null when `got` matches `want`, else where and how not. */
function mismatch(got: Json, want: Json, p = ""): string | null {
  const at = p || "result";
  if (want !== null && typeof want === "object" && !Array.isArray(want)) {
    if (got === null || typeof got !== "object" || Array.isArray(got)) {
      return `${at}: expected an object, got ${show(got)}`;
    }
    for (const [k, w] of Object.entries(want)) {
      if (!(k in got)) return `${at}.${k}: missing (fields: ${Object.keys(got).join(", ")})`;
      const m = mismatch(got[k]!, w, `${p}.${k}`);
      if (m) return m;
    }
    return null;
  }
  if (Array.isArray(want)) {
    if (!Array.isArray(got)) return `${at}: expected an array, got ${show(got)}`;
    if (got.length !== want.length) return `${at}: expected ${want.length} elements, got ${got.length}`;
    for (let i = 0; i < want.length; i++) {
      const m = mismatch(got[i]!, want[i]!, `${p}[${i}]`);
      if (m) return m;
    }
    return null;
  }
  if (typeof want === "number" && typeof got === "number") {
    // JSON cannot tell 26.0 from 26 here: integers compare exactly, anything
    // fractional within 1e-9 relative (1e-12 absolute near zero).
    if (Number.isInteger(want) && Number.isInteger(got)) {
      return got === want ? null : `${at}: expected ${want}, got ${got}`;
    }
    return Math.abs(got - want) <= Math.max(1e-9 * Math.abs(want), 1e-12)
      ? null
      : `${at}: expected ${want}, got ${got}`;
  }
  if (typeof got !== typeof want || got !== want) return `${at}: expected ${show(want)}, got ${show(got)}`;
  return null;
}

/** The TS member behind a canonical name: alias table first, then camelCase. */
function resolve(dev: Dev, name: string): { fn?: (...a: unknown[]) => unknown; value?: unknown } {
  for (const [cls, canonical, adapter] of ALIASES) {
    if (canonical === name && dev instanceof cls) return { fn: (...a) => adapter(dev, ...a) };
  }
  const member = camel(name);
  if (!(member in dev)) {
    return fail(`${dev.constructor.name} has no ${JSON.stringify(name)} (${member}): add it to the alias table`);
  }
  const v = dev[member];
  return typeof v === "function" ? { fn: (...a) => (v as Function).apply(dev, a) } : { value: v };
}

function recordArgs(dev: Dev, name: string, args: Json[]): unknown[] {
  return args.map((a, i) => {
    if (a === null || typeof a !== "object" || Array.isArray(a)) return a;
    const entry = RECORD_ARGS.find(([cls, n, pos]) => n === name && pos === i && dev instanceof cls);
    if (!entry) return fail(`${name} argument ${i} is a record with no builder in RECORD_ARGS`);
    return entry[3](a);
  });
}

/** The production (vid, pid) of a USB-table model: what enumeration would give. */
function usbId(model: string | undefined): { usbVid: number | null; usbPid: number | null } {
  if (model === undefined) return { usbVid: null, usbPid: null };
  const pid = Object.entries(DEPZ_USB_MODELS).find(([, m]) => m.name === model)?.[0];
  if (pid === undefined) return fail(`usb_model ${model} is not in the USB-id table`);
  return { usbVid: DEPZ_VID, usbPid: Number(pid) };
}

const replayOf = (sc: Scenario) =>
  new ReplayTransport(readFileSync(path.join(RECORDINGS, sc.recording), "utf8"), { strictTx: true });

const deviceOpts = { timeoutMs: TIMEOUT_MS, sleepImpl: () => Promise.resolve() };

/**
 * A legacy capture's probe (see header): exactly the requests it holds, then
 * the class attached in place. Returns the device and the software name read.
 */
async function openLegacy(
  replay: ReplayTransport,
  usb: { usbVid: number | null; usbPid: number | null },
): Promise<{ dev: DepzDevice; softwareName: string }> {
  const base = new DepzDevice(replay, deviceOpts);
  await base.open();
  try {
    const softwareName = await base.getSoftwareName();
    let deviceName = "";
    const type = parseSoftwareName(softwareName).sensorType;
    if (type === "vl53l7" || type === "vl53lx") {
      try {
        deviceName = await base.getDeviceName();
      } catch (err) {
        if (!(err instanceof DepzError)) throw err;
      }
    }
    const cls = resolveSensorClass(softwareName, deviceName, usb.usbVid, usb.usbPid);
    return { dev: attachSensorClass(base, cls, deviceOpts), softwareName };
  } catch (err) {
    await base.close();
    throw err;
  }
}

/** Open the capture as its `probe` says (see header). */
async function openScenario(sc: Scenario): Promise<{ dev: Dev; replay: ReplayTransport }> {
  const replay = replayOf(sc);
  const usb = usbId(sc.usb_model);
  const probe = sc.probe ?? "canonical";
  if (probe === "legacy") {
    const { dev, softwareName } = await openLegacy(replay, usb);
    if (sc.identity !== undefined) {
      const m = mismatch(canon(parseSoftwareName(softwareName)), sc.identity, "identity");
      if (m) {
        await dev.close();
        fail(m);
      }
    }
    return { dev: dev as Dev, replay };
  }
  if (probe !== "canonical") return fail(`unknown probe ${String(probe)}`);
  if (sc.identity !== undefined) return fail("`identity` is for legacy captures only");
  const dev = await openOnTransport(replay, { ...usb, timeoutMs: TIMEOUT_MS, classOptions: deviceOpts });
  return { dev: dev as Dev, replay };
}

function describeStep(step: Step): string {
  if (step.call !== undefined) return `${step.call}(${(step.args ?? []).map((a) => JSON.stringify(a)).join(", ")})`;
  if (step.subscribe !== undefined) return `subscribe ${step.subscribe} as ${step.as}`;
  if (step.take !== undefined) return `take ${step.count} from ${step.take}`;
  return `get ${step.get}`;
}

function nextWithin<T>(q: StreamQueue<T>, ms: number): Promise<IteratorResult<T> | "timeout"> {
  let timer: ReturnType<typeof setTimeout> | undefined;
  return Promise.race([
    q.next(),
    new Promise<"timeout">((r) => {
      timer = setTimeout(() => r("timeout"), ms);
    }),
  ]).finally(() => clearTimeout(timer));
}

async function runStep(
  dev: Dev,
  step: Step,
  subs: Map<string, StreamQueue<unknown>>,
  sizes: Map<string, number>,
): Promise<void> {
  if (step.call !== undefined) {
    const { fn } = resolve(dev, step.call);
    if (!fn) return fail(`${step.call} is a property, not a call`);
    const args = recordArgs(dev, step.call, step.args ?? []);
    if (step.error !== undefined) {
      let result: unknown;
      try {
        result = await fn(...args);
      } catch (err) {
        const kind = errorKind(err);
        if (kind !== step.error) {
          fail(`expected error ${step.error}, got ${kind ?? "unmapped"} (${(err as Error)?.name}: ${(err as Error)?.message})`);
        }
        return;
      }
      fail(`expected error ${step.error}, succeeded with ${show(canon(result))}`);
    }
    const result = await fn(...args);
    if (step.expect !== undefined) {
      const m = mismatch(canon(result), step.expect);
      if (m) fail(m);
    }
  } else if (step.subscribe !== undefined) {
    const { fn } = resolve(dev, step.subscribe);
    if (!fn) return fail(`${step.subscribe} is not a stream`);
    subs.set(step.as!, fn(sizes.get(step.as!) ?? 4096) as StreamQueue<unknown>);
  } else if (step.take !== undefined) {
    const sub = subs.get(step.take) ?? fail(`no subscription ${step.take}`);
    const want = step.expect as Json[];
    if (want.length !== step.count) fail("count and expect disagree");
    for (let i = 0; i < want.length; i++) {
      const r = await nextWithin(sub, TAKE_TIMEOUT_MS);
      if (r === "timeout") fail(`got ${i} of ${step.count} items, then nothing for ${TAKE_TIMEOUT_MS / 1000} s`);
      if (r !== "timeout" && r.done) fail(`got ${i} of ${step.count} items, then the stream ended`);
      const m = mismatch(canon((r as IteratorResult<unknown>).value), want[i]!, `item[${i}]`);
      if (m) fail(m);
    }
  } else if (step.get !== undefined) {
    const { fn, value } = resolve(dev, step.get);
    const v = fn ? await fn() : value;
    const m = mismatch(canon(v), step.expect!, step.get);
    if (m) fail(m);
  } else {
    fail(`unknown step ${show(step)}`);
  }
}

/** `end: exhausted`: every recorded tx byte written, every rx delivered. */
async function waitExhausted(replay: ReplayTransport, timeoutMs = 2000): Promise<string | null> {
  const deadline = Date.now() + timeoutMs;
  while (!replay.exhausted && Date.now() < deadline) await new Promise((r) => setTimeout(r, 10));
  if (replay.txRemaining || !replay.exhausted) {
    return `capture not exhausted: ${replay.txRemaining} tx bytes never written, rx ${replay.exhausted ? "drained" : "not drained"}`;
  }
  return null;
}

describe("contract 15 scenarios", () => {
  it("finds the scenarios", () => {
    if (FILES.length === 0) throw new Error(`no scenarios in ${SCENARIOS}`);
  });

  for (const name of FILES) {
    it(name.replace(/\.json$/, ""), async () => {
      const sc = JSON.parse(readFileSync(path.join(SCENARIOS, name), "utf8")) as Scenario;
      if (sc.schema !== "depz.scenario/1") throw new Error(`${name}: schema ${sc.schema}`);
      if (name !== `${sc.id}.json`) throw new Error(`${name}: id ${sc.id}`);
      // No drops: each queue holds everything its scenario takes, with room
      // for what streams on past the last take (drop-oldest would eat items).
      const sizes = new Map<string, number>();
      for (const step of sc.steps) {
        if (step.take !== undefined) sizes.set(step.take, (sizes.get(step.take) ?? 0) + step.count!);
      }
      for (const [k, v] of sizes) sizes.set(k, v + 4096);

      let opened: { dev: Dev; replay: ReplayTransport };
      try {
        opened = await openScenario(sc);
      } catch (err) {
        throw new Error(`${sc.id} open: ${(err as Error).name}: ${(err as Error).message}`);
      }
      const { dev, replay } = opened;
      const subs = new Map<string, StreamQueue<unknown>>();
      try {
        const want = CLASSES[sc.class] ?? fail(`${sc.id}: unknown class ${sc.class}`);
        if (dev.constructor !== want) {
          throw new Error(`${sc.id}: resolved ${dev.constructor.name}, not ${sc.class}`);
        }
        for (let i = 0; i < sc.steps.length; i++) {
          const step = sc.steps[i]!;
          try {
            await runStep(dev, step, subs, sizes);
          } catch (err) {
            const e = err as Error;
            const what = err instanceof StepFailure ? e.message : `raised ${e?.name}: ${e?.message}`;
            throw new Error(`${sc.id} step ${i} ${describeStep(step)}: ${what}`);
          }
        }
        for (const [subName, sub] of subs) {
          if (sub.droppedCount !== 0) throw new Error(`${sc.id}: ${subName}: ${sub.droppedCount} items dropped`);
        }
        if ((sc.end ?? "open") === "exhausted") {
          const m = await waitExhausted(replay);
          if (m) throw new Error(`${sc.id}: ${m}`);
        }
      } finally {
        await dev.close();
      }
      seen(`scenarios/${sc.id}`);
      seen(`recordings/${sc.recording}`);
    }, 60_000);
  }
});
