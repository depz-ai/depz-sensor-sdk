import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import {
  DATASET_SCHEMA,
  DatasetPlayer,
  DatasetReader,
  DatasetWriter,
} from "../src/dataset.js";
import { seen as seenVector } from "./golden/vectors.js";

function sampleContent(): string {
  const w = new DatasetWriter(
    {
      d0: { sensor_type: "sr04", time_sync: { offset_us: 100, rtt_us: 5 } },
      d1: { sensor_type: "vl53l8", time_sync: { offset_us: -5000, rtt_us: 7 } },
    },
    "test",
  );
  // deliberately interleaved out of order across devices
  w.write("d0", 1000, "sr04", { echo_us: 580, source: "loop" });
  w.write("d1", 900, "vl53l8", { resolution: 16, distance_mm: [1, 2] });
  w.write("d0", 51000, "sr04", { echo_us: 600, source: "loop" });
  w.write("d1", 30900, "vl53l8", { resolution: 16, distance_mm: [3, 4] });
  return w.dump();
}

describe("dataset writer/reader", () => {
  it("roundtrips header and merges records by host time", () => {
    const reader = new DatasetReader(sampleContent());
    expect(reader.header.schema).toBe(DATASET_SCHEMA);
    expect(Object.keys(reader.devices)).toEqual(["d0", "d1"]);
    expect(reader.records.map((r) => r.tHostUs)).toEqual([900, 1000, 30900, 51000]);
    expect(reader.records.map((r) => r.deviceId)).toEqual(["d1", "d0", "d1", "d0"]);
    expect(reader.durationUs).toBe(50100);
  });

  it("parses a python-generated dataset (cross-language)", async () => {
    const { readFileSync } = await import("node:fs");
    const { fileURLToPath } = await import("node:url");
    const { dirname, join } = await import("node:path");
    const p = join(
      dirname(fileURLToPath(import.meta.url)),
      "../../../contracts/vectors/recordings/dataset_dual_sr04.depzdata",
    );
    const text = readFileSync(p, "utf8");
    const reader = new DatasetReader(text);
    expect(Object.keys(reader.devices).length).toBeGreaterThanOrEqual(2);
    const ts = reader.records.map((r) => r.tHostUs);
    expect([...ts].sort((a, b) => a - b)).toEqual(ts);
    expect(reader.records.every((r) => r.kind === "sr04")).toBe(true);
    // The fixture's known contents: two SR04s, five loop/once samples each.
    expect(reader.header.schema).toBe("depz.dataset/1");
    expect(reader.devices.d0!.time_sync).toEqual({ offset_us: -8524738852, rtt_us: 13 });
    expect(reader.devices.d1!.time_sync).toEqual({ offset_us: -3525739108, rtt_us: 9 });
    expect(reader.devices.d0!.sensor_type).toBe("sr04");
    expect(reader.devices.d1!.software_name).toBe("APP_usonic_SR04_v0.95");
    // The file is already in host-time order, so the merged timeline is the
    // record lines as written.
    const lines = text.trim().split("\n").slice(1).map((l) => JSON.parse(l));
    expect(reader.records).toEqual(lines.map((e) => ({ deviceId: e.d, tHostUs: e.t, kind: e.k, value: e.v })));
    expect(reader.records.length).toBe(10);
    expect(reader.records.filter((r) => r.value.source === "once").map((r) => r.deviceId)).toEqual(["d1", "d1"]);
    seenVector("recordings/dataset_dual_sr04.depzdata");
  });

  it("rejects non-dataset content", () => {
    expect(() => new DatasetReader('{"schema":"depz.rec/1"}\n')).toThrow();
  });
});

// Contract 09 §Player, mirroring the reference tests
// (packages/depz-sensor-sdk-python/tests/test_dataset_player.py) on fake
// timers; times below are ms of the fake clock.
function evenly(n = 5, gapUs = 100_000): DatasetReader {
  const w = new DatasetWriter(
    { d0: { sensor_type: "sr04", time_sync: { offset_us: 0, rtt_us: 0 } } },
    "test",
  );
  for (let i = 0; i < n; i++) w.write("d0", 1_000_000 + i * gapUs, "sr04", { n: i });
  return new DatasetReader(w.dump());
}

function setup(reader: DatasetReader, speed = 1) {
  const player = new DatasetPlayer(reader, { speed, clock: () => Date.now() });
  const seen: number[] = [];
  const states: string[] = [];
  player.onRecord((r) => seen.push(r.tHostUs));
  player.onState((s) => states.push(s));
  return { player, seen, states };
}

const run = (ms: number) => vi.advanceTimersByTime(ms);

describe("dataset player", () => {
  beforeEach(() => {
    vi.useFakeTimers();
  });
  afterEach(() => {
    vi.useRealTimers();
  });

  it("plays paced, pauses, resumes, seeks", () => {
    const { player, seen, states } = setup(new DatasetReader(sampleContent()));
    expect(player.state).toBe("idle");
    expect(player.positionUs).toBe(0);
    expect(player.durationUs).toBe(50100);
    player.play();
    expect(seen).toEqual([]); // delivery happens on a timer, not inside play()
    run(0);
    // the first record at once; 1000 is 100 µs later — within a timer's
    // resolution, so it goes out in the same turn
    expect(seen).toEqual([900, 1000]);
    run(29);
    expect(seen).toEqual([900, 1000]);
    run(1);
    expect(seen).toEqual([900, 1000, 30900]);
    expect(player.positionUs).toBe(30000);

    player.pause();
    run(1000);
    expect(seen.length).toBe(3);

    player.play(); // resume: the next record waits its full gap (20.1 ms) again
    run(18);
    expect(seen.length).toBe(3);
    run(3);
    expect(seen).toEqual([900, 1000, 30900, 51000]);
    expect(player.state).toBe("done");
    expect(player.positionUs).toBe(50100);
    expect(states).toEqual(["playing", "paused", "playing", "done"]);

    seen.length = 0;
    player.seekUs(30000); // → 30900
    expect(player.state).toBe("done"); // a seek does not change the state
    expect(player.positionUs).toBe(100); // the record just before the target
    player.speed = 0;
    player.play();
    run(0);
    expect(seen).toEqual([30900, 51000]);
    expect(player.state).toBe("done");
  });

  it("scales gaps by speed and rescales the current wait on a change", () => {
    const { player, seen } = setup(evenly(), 2);
    player.play();
    run(0);
    run(49);
    expect(seen.length).toBe(1);
    run(1);
    expect(seen.length).toBe(2);

    player.speed = 1; // 100 ms gaps from here on
    run(50); // half of the next gap
    expect(seen.length).toBe(2);
    player.speed = 0.5; // the remaining 50 ms of media → 100 ms of wall time
    run(98);
    expect(seen.length).toBe(2);
    run(2);
    expect(seen.length).toBe(3);
  });

  it("speed 0 delivers everything, then a paced speed waits the next gap", () => {
    const a = setup(evenly(), 0);
    a.player.play();
    run(0);
    expect(a.seen.length).toBe(5);
    expect(a.states).toEqual(["playing", "done"]);

    const b = setup(evenly());
    b.player.play();
    run(0);
    b.player.speed = 0;
    b.player.speed = 1;
    run(98);
    expect(b.seen.length).toBe(1);
    run(2);
    expect(b.seen.length).toBe(2);
  });

  it("a slow consumer catches up without drift", () => {
    const { player, seen } = setup(evenly());
    let busy = true;
    player.onRecord(() => {
      if (busy) {
        busy = false;
        vi.setSystemTime(Date.now() + 350); // the first record took 350 ms
      }
    });
    player.play();
    run(0);
    expect(seen.length).toBe(1); // a 350 ms burst: the player yields to the page …
    run(1); // (a zero-delay timer set inside a fake timer needs 1 ms to fire)
    expect(seen.length).toBe(4); // … then sends the overdue records 1..3 back to back
    run(47);
    expect(seen.length).toBe(4);
    run(2); // record 4 stays on its original schedule (400 ms)
    expect(seen.length).toBe(5);
  });

  it("validates speed and seek arguments", () => {
    const { player } = setup(evenly());
    for (const bad of [-1, NaN, Infinity, "2"]) {
      expect(() => {
        player.speed = bad as number;
      }).toThrow(RangeError);
    }
    expect(player.speed).toBe(1);
    expect(() => new DatasetPlayer(evenly(), { speed: -0.5 })).toThrow(RangeError);
    for (const bad of [NaN, "5", null]) {
      expect(() => player.seekUs(bad as number)).toThrow(RangeError);
    }
    const empty = setup(new DatasetReader(new DatasetWriter({}, "e").dump()));
    empty.player.seekUs(100); // no-op
    expect(empty.player.positionUs).toBe(0);
    expect(empty.player.durationUs).toBe(0);
  });

  it("seeks while paused: skips and waits the gap", () => {
    const { player, seen, states } = setup(evenly());
    player.play();
    run(0);
    player.pause();
    player.seekUs(250_000); // between records 2 (200 ms) and 3 (300 ms)
    expect(player.positionUs).toBe(200_000);
    expect(player.state).toBe("paused");
    player.play();
    run(99);
    expect(seen).toEqual([1_000_000]);
    run(1);
    expect(seen).toEqual([1_000_000, 1_300_000]); // 1 and 2 were skipped
    expect(states).toEqual(["playing", "paused", "playing"]);
  });

  it("seeks while playing without state events", () => {
    const { player, seen, states } = setup(evenly());
    player.play();
    run(30);
    player.seekUs(300_000); // exactly a record: it is the next one delivered
    expect(player.state).toBe("playing");
    run(99);
    expect(seen).toEqual([1_000_000]);
    run(1);
    expect(seen).toEqual([1_000_000, 1_300_000]);
    expect(states).toEqual(["playing"]);
  });

  it("seeks backwards and negative", () => {
    const { player, seen } = setup(evenly(), 0);
    player.play();
    run(0);
    player.seekUs(-5);
    expect(player.positionUs).toBe(0);
    player.play();
    run(0);
    expect(seen.length).toBe(10);
  });

  it("a seek past the end ends playback; idle, play restarts", () => {
    const a = setup(evenly());
    a.player.play();
    run(0);
    a.player.seekUs(1e9);
    expect(a.player.state).toBe("done");
    expect(a.player.positionUs).toBe(400_000);
    expect(a.states).toEqual(["playing", "done"]);
    run(1000);
    expect(a.seen).toEqual([1_000_000]);

    const b = setup(evenly(), 0);
    b.player.seekUs(1e9);
    expect(b.player.state).toBe("idle");
    b.player.play();
    run(0);
    expect(b.seen.length).toBe(5);
  });

  it("an empty dataset goes straight to done", () => {
    const { player, seen, states } = setup(new DatasetReader(new DatasetWriter({}, "e").dump()));
    player.play();
    expect(states).toEqual(["playing", "done"]);
    run(10);
    expect(seen).toEqual([]);
  });

  it("play and pause are idempotent; done then play restarts", () => {
    const a = setup(evenly());
    a.player.pause();
    expect(a.player.state).toBe("idle");
    a.player.play();
    a.player.play();
    a.player.pause();
    a.player.pause();
    expect(a.states).toEqual(["playing", "paused"]);

    const b = setup(evenly(2), 0);
    b.player.play();
    run(0);
    b.player.play();
    run(0);
    expect(b.seen).toEqual([1_000_000, 1_100_000, 1_000_000, 1_100_000]);
    expect(b.states).toEqual(["playing", "done", "playing", "done"]);
  });

  it("unsubscribe is idempotent", () => {
    const player = new DatasetPlayer(evenly(), { speed: 0, clock: () => Date.now() });
    const got: number[] = [];
    const unsub = player.onRecord(() => got.push(1));
    const unsubState = player.onState(() => got.push(2));
    unsub();
    unsub();
    unsubState();
    unsubState();
    player.play();
    run(0);
    expect(got).toEqual([]);
  });

  it("a callback may control the player", () => {
    const { player, seen, states } = setup(evenly(), 0);
    player.onRecord((r) => {
      if (r.tHostUs === 1_100_000) player.pause();
    });
    player.play();
    run(10);
    expect(seen).toEqual([1_000_000, 1_100_000]);
    expect(states).toEqual(["playing", "paused"]);
  });

  it("a record callback error pauses the player and is rethrown", () => {
    const { player, seen, states } = setup(evenly(), 0);
    player.onRecord((r) => {
      if (r.tHostUs === 1_200_000) throw new Error("consumer failed");
    });
    player.play();
    expect(() => run(0)).toThrow("consumer failed");
    expect(player.state).toBe("paused");
    expect(player.positionUs).toBe(200_000); // the failing record counts as delivered
    player.play();
    run(0);
    expect(seen).toEqual([0, 1, 2, 3, 4].map((i) => 1_000_000 + i * 100_000));
    expect(states).toEqual(["playing", "paused", "playing", "done"]);
  });

  it("at speed 0 a long dataset is delivered in bursts, not in one call", () => {
    const { player, seen } = setup(evenly(50_000, 10), 0);
    let calls = 0;
    player.onRecord(() => {
      if (++calls % 1000 === 0) vi.setSystemTime(Date.now() + 1); // 1 ms per 1000 records
    });
    player.play();
    run(0);
    expect(seen.length).toBeGreaterThan(0);
    expect(seen.length).toBeLessThan(50_000); // yielded to the event loop
    run(100);
    expect(seen.length).toBe(50_000);
    expect(player.state).toBe("done");
  });
});
