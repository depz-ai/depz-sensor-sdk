/**
 * Record a fake-device session, replay it, decode identically.
 * Port of the Python `tests/test_record_replay.py` (contract 08).
 */

import { gunzipSync } from "node:zlib";
import { describe, expect, it } from "vitest";
import { RECORDING_SCHEMA, RecordingTransport, ReplayMismatchError, ReplayTransport, Sr04 } from "../src/index.js";
import { FakeSr04 } from "./fake-sr04.js";

// `{"schema": "depz.rec/1"}` and one rx event `aa`, gzipped (mtime 0): the
// same 70 bytes in every SDK's test
const GZIPPED_CAPTURE = Uint8Array.from(
  Buffer.from(
    "1f8b0800000000000203ab562a4ece48cd4d54b252504a492da8d22b4a4dd63754aae5aa" +
      "562a018a19e80085338b40b245154a204e6209586d6222500d007e3bc00b3d000000",
    "hex",
  ),
);

interface DriveResult {
  software: string;
  period: number;
  echo: number;
}

async function drive(dev: Sr04): Promise<DriveResult> {
  const software = await dev.getSoftwareName();
  const period = await dev.getSamplePeriodUs();
  const m = await dev.measureOnce();
  return { software, period, echo: m.echoTimeUs };
}

async function recordSession(): Promise<{ live: DriveResult; dump: string }> {
  const fake = new FakeSr04();
  const rec = new RecordingTransport(fake.transport, { headerExtra: { port: "loopback" } });
  const dev = new Sr04(rec, { timeoutMs: 1000 });
  await dev.open();
  try {
    const live = await drive(dev);
    return { live, dump: rec.dump() };
  } finally {
    await dev.close();
    await fake.close();
  }
}

describe("record/replay", () => {
  it("records a schema header plus tx/rx event lines", async () => {
    const { dump } = await recordSession();
    const lines = dump.trim().split("\n");
    const header = JSON.parse(lines[0]!) as Record<string, unknown>;
    expect(header["schema"]).toBe(RECORDING_SCHEMA);
    expect(header["port"]).toBe("loopback");
    const dirs = new Set(
      lines.slice(1).map((l) => (JSON.parse(l) as { dir: string }).dir),
    );
    expect(dirs).toEqual(new Set(["rx", "tx"]));
  });

  it("replays loose, strict and realtime with identical decode", async () => {
    const { live, dump } = await recordSession();
    for (const opts of [{}, { strictTx: true }, { realtime: true }]) {
      const replay = new ReplayTransport(dump, opts);
      expect(replay.header["schema"]).toBe(RECORDING_SCHEMA);
      const dev = new Sr04(replay, { timeoutMs: 1000 });
      await dev.open();
      try {
        expect(await drive(dev)).toEqual(live);
        expect(replay.exhausted).toBe(true);
      } finally {
        await dev.close();
      }
    }
  });

  it("strictTx rejects a divergent host tx stream", async () => {
    const { dump } = await recordSession();
    const dev = new Sr04(new ReplayTransport(dump, { strictTx: true }), { timeoutMs: 200 });
    await dev.open();
    try {
      // Recorded session starts with GET_NAME_ACTIVE_SOFTWARE; sending
      // GET_DEVICE_NAME first must trip the byte-exact comparison — with the
      // SDK's own error (contract 08, ERRATA A14), not a bare Error.
      const err: unknown = await dev.getDeviceName().then(() => null, (e: unknown) => e);
      expect(err).toBeInstanceOf(ReplayMismatchError);
      expect((err as Error).message).toMatch(/strictTx mismatch/);
    } finally {
      await dev.close();
    }
  });

  it("gates rx causally on the host tx byte count", async () => {
    const { dump } = await recordSession();
    const replay = new ReplayTransport(dump);
    const dev = new Sr04(replay, { timeoutMs: 1000 });
    await dev.open();
    try {
      // No tx written yet beyond nothing — no rx may be released.
      await new Promise((r) => setTimeout(r, 20));
      expect(dev.stats.rxBytes).toBe(0);
      expect(await dev.getSoftwareName()).toBe(FakeSr04.SOFTWARE_NAME);
    } finally {
      await dev.close();
    }
  });

  it("refuses a gzipped capture (ERRATA A6: there is no .depzrec.gz)", () => {
    // the content itself is a valid capture
    const plain = new TextDecoder().decode(gunzipSync(GZIPPED_CAPTURE));
    expect(new ReplayTransport(plain).header["schema"]).toBe(RECORDING_SCHEMA);
    // the file as a browser's File.text() or Node's readFileSync(…, "utf8")
    // reads it: not inflated, refused like any damaged file
    const gzipped = new TextDecoder().decode(GZIPPED_CAPTURE);
    expect(() => new ReplayTransport(gzipped)).toThrow(TypeError);
  });

  it("refuses a header nested 100 000 deep, naming it (contract 08 «Line rules»)", () => {
    const deep = "[".repeat(100_000) + "]".repeat(100_000);
    expect(() => new ReplayTransport(`{"schema":"depz.rec/1","x":${deep}}\n`)).toThrow(
      /line 1: not JSON: nested deeper than 256/,
    );
  });
});
