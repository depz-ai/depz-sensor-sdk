import { describe, expect, it, vi } from "vitest";
import {
  DATASET_SCHEMA,
  DatasetPlayer,
  DatasetReader,
  DatasetWriter,
} from "../src/dataset.js";

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
    const reader = new DatasetReader(readFileSync(p, "utf8"));
    expect(Object.keys(reader.devices).length).toBeGreaterThanOrEqual(2);
    const ts = reader.records.map((r) => r.tHostUs);
    expect([...ts].sort((a, b) => a - b)).toEqual(ts);
    expect(reader.records.every((r) => r.kind === "sr04")).toBe(true);
  });

  it("rejects non-dataset content", () => {
    expect(() => new DatasetReader('{"schema":"depz.rec/1"}\n')).toThrow();
  });
});

describe("dataset player", () => {
  it("plays paced, pauses, seeks", () => {
    vi.useFakeTimers();
    try {
      const player = new DatasetPlayer(new DatasetReader(sampleContent()));
      const seen: number[] = [];
      player.onRecord((r) => seen.push(r.tHostUs));
      player.speed = 1.0;
      player.play();
      // first record emits immediately; second at +100µs (gap<1ms → sync)
      expect(seen).toEqual([900, 1000]);
      vi.advanceTimersByTime(30); // 29.9 ms gap to 30900
      expect(seen).toEqual([900, 1000, 30900]);
      player.pause();
      vi.advanceTimersByTime(1000);
      expect(seen.length).toBe(3);
      player.play();
      vi.advanceTimersByTime(21);
      expect(seen).toEqual([900, 1000, 30900, 51000]);
      expect(player.state).toBe("done");

      seen.length = 0;
      player.seekUs(30000 - 900 + 900); // to 30900 offset-ish
      player.speed = 0;
      player.play();
      expect(seen).toEqual([30900, 51000]);
    } finally {
      vi.useRealTimers();
    }
  });
});
