/**
 * Sensor clock model and frame capture time (contracts/14_SENSOR_TMF8829.md §5).
 * Mirrors the Python reference `depz_sensor_sdk.tmf8829.clock`; the vectors in
 * `contracts/vectors/tmf8829_frame_time.json` pin it bit for bit.
 *
 * Plain numbers, not bigint: every intermediate stays below 2^53 (µs values
 * ~2^31, `span << 16` ~2^37, tick × ratio ~2^51), so `Math.floor(a / b)` is
 * exact. `>>` in JS is 32-bit, so the Q16 shifts are divisions by 65536.
 */

export const TMF8829_NOMINAL_Q16 = 8 * 65536;
const CLOCK_WINDOW_MIN_US = 200_000;
const CLOCK_WINDOW_SLIDE_US = 1_000_000;
const EDGE_LATE_US = 2000;

/** Clock ratio not measured yet (nominal 8 µs/tick used). */
export const TMF8829_FT_NOMINAL = 0x01;
/** Clock model restarted at this frame (SYSTICK jump). */
export const TMF8829_FT_CLOCK_RESET = 0x02;
/** INT edge > 2 ms later after t1 than usual in this stream. */
export const TMF8829_FT_EDGE_LATE = 0x04;
/** Sub-integrations not back to back (histogram FIFO wait). */
export const TMF8829_FT_STALLED = 0x08;
/** Histograms + 32x32/48x32: T1 may be too long, centre late. */
export const TMF8829_FT_T1_UNSURE = 0x10;

export const TMF8829_FT_NAMES: ReadonlyArray<[number, string]> = [
  [TMF8829_FT_NOMINAL, "nominal"],
  [TMF8829_FT_CLOCK_RESET, "clock_reset"],
  [TMF8829_FT_EDGE_LATE, "edge_late"],
  [TMF8829_FT_STALLED, "stalled"],
  [TMF8829_FT_T1_UNSURE, "t1_unsure"],
];

export function tmf8829FlagNames(flags: number): string[] {
  return TMF8829_FT_NAMES.filter(([bit]) => flags & bit).map(([, name]) => name);
}

/** Sub-integrations per results frame by fp_mode. */
const FP_SUBINTS: Readonly<Record<number, number>> = { 0: 2, 1: 2, 2: 2, 3: 4, 4: 4, 5: 6 };

const TWO32 = 2 ** 32;
const u32 = (x: number): number => ((x % TWO32) + TWO32) % TWO32;
const s32 = (x: number): number => {
  const u = u32(x);
  return u >= 2 ** 31 ? u - TWO32 : u;
};
/** Arithmetic `>> 16` for values beyond 32 bits. */
const shr16 = (x: number): number => Math.floor(x / 65536);

/**
 * Sensor SYSTICK (125 kHz) → MCU µs, fed one clock pair per frame. The ratio
 * is taken over a sliding 1..2 s window, first after 200 ms; it is also the
 * distance correction.
 */
export class Tmf8829SensorClock {
  ratioQ16 = TMF8829_NOMINAL_Q16;
  nominal = true;
  private valid = false;
  private last: [number, number] = [0, 0];
  private anchor: [number, number] = [0, 0];
  private mid: [number, number] | null = null;

  reset(): void {
    this.valid = false;
    this.last = [0, 0];
    this.anchor = [0, 0];
    this.mid = null;
    this.ratioQ16 = TMF8829_NOMINAL_Q16;
    this.nominal = true;
  }

  /** MCU/sensor clock ratio (≈ 1.015): distances are scaled by it. */
  get ratio(): number {
    return this.ratioQ16 / TMF8829_NOMINAL_Q16;
  }

  private static stepOk(dUs: number, dTick: number): boolean {
    const expect = dTick * 8;
    const tol = Math.floor(dUs / 10) + 50;
    return expect + tol >= dUs && expect <= dUs + tol;
  }

  /** Add a pair → TMF8829_FT_CLOCK_RESET if it does not fit the model, else 0. */
  add(us: number, tick: number): number {
    let flags = 0;
    if (this.valid && !Tmf8829SensorClock.stepOk(us - this.last[0], u32(tick - this.last[1]))) {
      this.valid = false;
      flags = TMF8829_FT_CLOCK_RESET;
    }
    this.last = [us, tick];
    if (!this.valid) {
      this.valid = true;
      this.anchor = this.last;
      this.mid = null;
      this.ratioQ16 = TMF8829_NOMINAL_Q16;
      this.nominal = true;
      return flags;
    }
    const span = us - this.anchor[0];
    if (span >= CLOCK_WINDOW_MIN_US) {
      this.ratioQ16 = u32(Math.floor((span * 65536) / u32(tick - this.anchor[1])));
      this.nominal = false;
    }
    if (this.mid === null && span >= CLOCK_WINDOW_SLIDE_US) {
      this.mid = this.last;
    } else if (this.mid !== null && span >= 2 * CLOCK_WINDOW_SLIDE_US) {
      this.anchor = this.mid;
      this.mid = null;
    }
    return flags;
  }

  /** SYSTICK `tick` → MCU µs through the pair (pairUs, pairTick). */
  toUs(pairUs: number, pairTick: number, tick: number): number {
    return pairUs + shr16(s32(tick - pairTick) * this.ratioQ16);
  }
}

/** Capture time of one results frame, MCU µs. */
export interface Tmf8829FrameTime {
  /** Centre of the capture window. */
  timestampUs: number;
  /** TMF8829_FT_* */
  flags: number;
  /** Sub-integration length used. */
  t1LenUs: number;
  t0Us: number;
  t1Us: number;
  /** INT edge credited to the frame, 0 = none. */
  edgeUs: number;
  /** MCU time of the header read (the clock pair). */
  pairUs: number;
  /** Clock ratio after this frame. */
  ratioQ16: number;
}

/**
 * When a results frame was captured: the centre of [t0, t1 + T1], T1 the
 * stream's shortest (t1 − t0)/(N − 1).
 */
export class Tmf8829CaptureTimer {
  private lagMin: number | null = null;
  private histSeen = false;
  private subMin = 0;

  constructor(readonly clock: Tmf8829SensorClock) {}

  reset(): void {
    this.lagMin = null;
    this.histSeen = false;
    this.subMin = 0;
  }

  /** A histogram frame was seen in the stream. */
  histograms(): void {
    this.histSeen = true;
  }

  results(
    fpMode: number,
    systick: number,
    t0Tick: number,
    t1Tick: number,
    pairUs: number,
    edgeUs: number,
    flagsIn: number,
  ): Tmf8829FrameTime {
    const c = this.clock;
    let flags = flagsIn;
    if (c.nominal) flags |= TMF8829_FT_NOMINAL;
    const t0 = c.toUs(pairUs, systick, t0Tick);
    const t1 = c.toUs(pairUs, systick, t1Tick);

    if (edgeUs && !this.histSeen) {
      const lag = edgeUs - t1;
      if (lag <= 0 || (this.lagMin !== null && lag > this.lagMin + EDGE_LATE_US)) {
        flags |= TMF8829_FT_EDGE_LATE;
      } else if (this.lagMin === null || lag < this.lagMin) {
        this.lagMin = lag;
      }
    }

    const n1 = (FP_SUBINTS[fpMode] ?? 2) - 1;
    const sub = Math.floor(u32(t1Tick - t0Tick) / n1);
    if (!this.subMin || sub < this.subMin) this.subMin = sub;
    if (sub > this.subMin + 1) flags |= TMF8829_FT_STALLED;
    if (this.histSeen && n1 > 1) flags |= TMF8829_FT_T1_UNSURE;

    const t1Len = u32(shr16(this.subMin * c.ratioQ16));
    return {
      timestampUs: Math.floor((t0 + t1 + t1Len) / 2),
      flags,
      t1LenUs: t1Len,
      t0Us: t0,
      t1Us: t1,
      edgeUs,
      pairUs,
      ratioQ16: c.ratioQ16,
    };
  }
}
