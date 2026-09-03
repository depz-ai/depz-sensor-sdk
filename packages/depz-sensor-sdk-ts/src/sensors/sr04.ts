/**
 * SR04 ultrasonic sensor (contracts/03_SENSOR_SR04.md).
 * Mirrors the Python reference `depz_sensor_sdk.sr04.Sr04`.
 */

import { DepzDevice, NO_MATCH, StreamQueue, type Matcher } from "../device/device.js";
import type { PacketEvent } from "../protocol/framing.js";
import {
  ECHO_TIMEOUT,
  Sr04Cmd,
  Sr04Rpt,
  distanceMmFromEcho,
  packEchoDecay,
  packSamplePeriod,
  unpackEchoDecay,
  unpackSamplePeriod,
  unpackSr04Data,
  type Sr04Data,
} from "../protocol/sr04.js";

/** One ranging result. `valid` is false for the no-echo timeout sentinel. */
export interface Sr04Measurement {
  timestampUs: bigint;
  echoTimeUs: number;
  /** "once": host command or SYNC_IN edge; "loop": measurement loop sample. */
  source: "once" | "loop";
  valid: boolean;
  /** Distance at 343 m/s, or null when no echo was received. */
  distanceMm: number | null;
}

function measurementFromData(data: Sr04Data): Sr04Measurement {
  return {
    timestampUs: data.timestampUs,
    echoTimeUs: data.echoTimeUs,
    source: data.sourceCmd === Sr04Cmd.StartMeasurementLoop ? "loop" : "once",
    valid: data.echoTimeUs !== ECHO_TIMEOUT,
    distanceMm: distanceMmFromEcho(data.echoTimeUs),
  };
}

/**
 * HC-SR04 ultrasonic ranging device.
 *
 * Measurements stream via callbacks (`onMeasurement`) and/or the pull
 * iterator (`measurements()`); both receive loop samples *and* unsolicited
 * single shots triggered by an AUX SYNC_IN edge (RPT_DATA source cmd 0x36).
 */
export class Sr04 extends DepzDevice {
  private measureCbs: Array<(m: Sr04Measurement) => void> = [];
  private measureQueues: StreamQueue<Sr04Measurement>[] = [];

  // ── configuration ──────────────────────────────────────────────────────────

  /** Minimum interval between measurement starts (default 50000). */
  getSamplePeriodUs(): Promise<number> {
    return this.request(Sr04Cmd.GetSamplePeriod, undefined, {
      matcher: DepzDevice.expectReport(Sr04Rpt.SamplePeriod, unpackSamplePeriod),
    });
  }

  /**
   * The effective rate is auto-throttled by the echo window (contract 03
   * §3) — reading back returns the stored value, not the effective one.
   */
  async setSamplePeriodUs(periodUs: number): Promise<void> {
    await this.request(Sr04Cmd.SetSamplePeriod, packSamplePeriod(periodUs), {
      okCompletes: true,
    });
  }

  getEchoDecayUs(): Promise<number> {
    return this.request(Sr04Cmd.GetEchoDecay, undefined, {
      matcher: DepzDevice.expectReport(Sr04Rpt.EchoDecay, unpackEchoDecay),
    });
  }

  /**
   * Set the settle pause; the device clamps to 4000–65000 µs silently, so
   * this re-reads and returns the value actually in effect.
   */
  async setEchoDecayUs(decayUs: number): Promise<number> {
    await this.request(Sr04Cmd.SetEchoDecay, packEchoDecay(decayUs), { okCompletes: true });
    return this.getEchoDecayUs();
  }

  // ── measuring ──────────────────────────────────────────────────────────────

  /**
   * Single shot. Rejects with BusyError while the loop is running. The
   * reply arrives only when the echo completes (or times out at ~65.5 ms),
   * so the default timeout is generous.
   */
  measureOnce(timeoutMs = 1000): Promise<Sr04Measurement> {
    const matcher: Matcher<Sr04Measurement> = (pkt: PacketEvent) => {
      if (pkt.cmd !== Sr04Rpt.Data || pkt.payload.length !== 11) return NO_MATCH;
      const data = unpackSr04Data(pkt.payload);
      if (data.sourceCmd !== Sr04Cmd.MeasureOnce) return NO_MATCH;
      return measurementFromData(data);
    };
    return this.request(Sr04Cmd.MeasureOnce, undefined, { matcher, timeoutMs });
  }

  /** Start the measurement loop (idempotent). */
  async start(): Promise<void> {
    await this.request(Sr04Cmd.StartMeasurementLoop, undefined, { okCompletes: true });
  }

  /** Stop the measurement loop (idempotent). */
  async stop(): Promise<void> {
    await this.request(Sr04Cmd.StopMeasurementLoop, undefined, { okCompletes: true });
  }

  /**
   * Subscribe to measurements (read-pump context; don't block). Returns an
   * unsubscribe function.
   */
  onMeasurement(cb: (m: Sr04Measurement) => void): () => void {
    this.measureCbs.push(cb);
    return () => {
      this.measureCbs = this.measureCbs.filter((c) => c !== cb);
    };
  }

  /**
   * Async iterator over measurements — bounded, drop-oldest (contract 07
   * §3). The returned queue exposes `droppedCount`; it ends when the device
   * closes or the consumer breaks out of iteration.
   */
  measurements(maxsize = 256): StreamQueue<Sr04Measurement> {
    const queue: StreamQueue<Sr04Measurement> = new StreamQueue(maxsize, () => {
      this.measureQueues = this.measureQueues.filter((q) => q !== queue);
      deregister();
    });
    this.measureQueues.push(queue);
    const deregister = this.registerStream(queue);
    return queue;
  }

  /** Drop counters of all live measurement queues (diagnostics). */
  get streamDroppedCounts(): number[] {
    return this.measureQueues.map((q) => q.droppedCount);
  }

  // ── internal ───────────────────────────────────────────────────────────────

  protected override handleReport(pkt: PacketEvent): boolean {
    if (pkt.cmd === Sr04Rpt.Data && pkt.payload.length === 11) {
      const m = measurementFromData(unpackSr04Data(pkt.payload));
      for (const cb of [...this.measureCbs]) cb(m);
      for (const q of [...this.measureQueues]) q.push(m);
      return true;
    }
    return false;
  }
}
