/**
 * SR04 ultrasonic sensor (contracts/03_SENSOR_SR04.md).
 * Mirrors the Python reference `depz_sensor_sdk.sr04.Sr04`.
 */

import { DepzDevice, type Matcher } from "../device/device.js";
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

/** RPT_DATA.cmd → measurement source (contract 03 §2, ERRATA E3/B11). */
const SOURCES: ReadonlyMap<number, "once" | "loop"> = new Map([
  [Sr04Cmd.MeasureOnce, "once"],
  [Sr04Cmd.StartMeasurementLoop, "loop"],
]);

/** null for a source cmd contract 03 §2 does not define: not a measurement
 * (it falls through to the hex text event). */
function measurementFromData(data: Sr04Data): Sr04Measurement | null {
  const source = SOURCES.get(data.sourceCmd);
  if (source === undefined) return null;
  return {
    timestampUs: data.timestampUs,
    echoTimeUs: data.echoTimeUs,
    source,
    valid: data.echoTimeUs !== ECHO_TIMEOUT,
    distanceMm: distanceMmFromEcho(data.echoTimeUs),
  };
}

/**
 * HC-SR04 ultrasonic ranging device.
 *
 * `measurements` is the stream (contract 07 §3): loop samples *and* the
 * unsolicited single shots an AUX SYNC_IN edge triggers (RPT_DATA source cmd
 * 0x36).
 */
export class Sr04 extends DepzDevice {
  /**
   * The measurements (contract 07 §3): `on(cb)` subscribes, `subscribe(maxsize)`
   * iterates, `next(timeoutMs)` waits for one; `droppedCounts` and
   * `parseErrors` (RPT_DATA reports shorter than a measurement, contract 07
   * §2) are its counters.
   */
  readonly measurements = this.stream<Sr04Measurement>(256, Sr04Rpt.Data);

  // ── configuration ──────────────────────────────────────────────────────────

  /** Minimum interval between measurement starts (default 50000). */
  async getSamplePeriodUs(): Promise<number> {
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

  async getEchoDecayUs(): Promise<number> {
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
  async measureOnce(timeoutMs = 1000): Promise<Sr04Measurement> {
    const matcher: Matcher<Sr04Measurement> = (pkt: PacketEvent) => {
      // the reply is the RPT_DATA sourced by this command; a short one fails
      // the request (InvalidReplyError, contract 07 §3), no timeout
      if (pkt.cmd !== Sr04Rpt.Data || pkt.payload[0] !== Sr04Cmd.MeasureOnce) return DepzDevice.NO_MATCH;
      return measurementFromData(unpackSr04Data(pkt.payload)) ?? DepzDevice.NO_MATCH;
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

  // ── internal ───────────────────────────────────────────────────────────────

  protected override handleReport(pkt: PacketEvent): boolean {
    if (pkt.cmd !== Sr04Rpt.Data) return false;
    let m: Sr04Measurement | null;
    try {
      // short: refused (RangeError) and counted — contract 07 §2
      m = measurementFromData(unpackSr04Data(pkt.payload));
    } catch {
      this.measurements.parseErrors += 1;
      return true;
    }
    if (m === null) return false;
    this.measurements.emit(m);
    return true;
  }
}
