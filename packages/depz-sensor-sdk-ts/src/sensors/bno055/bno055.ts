/**
 * BNO055 9-axis IMU over the firmware register bridge
 * (contracts/13_SENSOR_BNO055.md).
 *
 * The MCU is a thin I2C bridge: it reads and writes BNO055 registers on
 * request and streams one configured register block on a timer. Everything
 * else — operating mode, units, axis remap, calibration, decoding — is host
 * logic expressed as register access. The BNO055 fuses on chip; there is no
 * host-side fusion driver.
 *
 * Mirrors the Python reference `depz_sensor_sdk.bno055.Bno055`.
 */

import { DepzDevice, StreamQueue, type DeviceOptions } from "../../device/device.js";
import { DepzError, DepzTimeoutError } from "../../errors.js";
import type { PacketEvent } from "../../protocol/framing.js";
import {
  BNO055_RESET_TIMEOUT_MS,
  BNO055_TRIGGER_TIMER,
  BNO055_XFER_MAX,
  Bno055Cmd,
  Bno055Rpt,
  bno055IdsOk,
  packBno055ReadReg,
  packBno055StartStream,
  packBno055WriteReg,
  unpackBno055Info,
  unpackBno055RegData,
  unpackBno055Stream,
  type Bno055Info,
  type Bno055RegData,
} from "../../protocol/bno055.js";
import type { SerialTransport } from "../../transport/types.js";
import {
  BNO055_BOOT_SETTLE_TIMEOUT_MS,
  BNO055_STUCK_SELF_TEST_POLLS,
  BNO055_CALIB_PROFILE_LEN,
  BNO055_DEFAULT_UNITS,
  BNO055_FULL_BLOCK,
  BNO055_FUSION_ACCEL_LSB,
  BNO055_FUSION_START_TIMEOUT_MS,
  BNO055_MAG_LSB,
  BNO055_MODE_SWITCH_FROM_CONFIG_MS,
  BNO055_MODE_SWITCH_TO_CONFIG_MS,
  BNO055_QUAT_BLOCK,
  BNO055_QUAT_LSB,
  BNO055_REG1_ACC_CONFIG,
  BNO055_REG1_GYR_CONFIG_0,
  BNO055_REG1_INT_EN,
  BNO055_REG1_INT_MSK,
  BNO055_REG1_INT_SETTINGS_FIRST,
  BNO055_REG1_INT_SETTINGS_LAST,
  BNO055_REG1_MAG_CONFIG,
  BNO055_REG1_UNIQUE_ID,
  BNO055_REG_AXIS_MAP_CONFIG,
  BNO055_REG_CALIB_PROFILE,
  BNO055_REG_CALIB_STAT,
  BNO055_REG_INT_STA,
  BNO055_REG_OPR_MODE,
  BNO055_REG_PAGE_ID,
  BNO055_REG_PWR_MODE,
  BNO055_REG_QUA_DATA,
  BNO055_REG_SIC_MATRIX,
  BNO055_REG_ST_RESULT,
  BNO055_REG_SYS_CLK_STATUS,
  BNO055_REG_SYS_STATUS,
  BNO055_REG_SYS_TRIGGER,
  BNO055_REG_TEMP_SOURCE,
  BNO055_REG_UNIT_SEL,
  BNO055_SELF_TEST_MS,
  BNO055_SIC_IDENTITY,
  BNO055_SYS_STATUS_BOOTING,
  BNO055_SYS_TRIGGER_CLK_SEL,
  BNO055_SYS_TRIGGER_RST_INT,
  BNO055_SYS_TRIGGER_SELF_TEST,
  BNO055_UNIQUE_ID_LEN,
  Bno055OprMode,
  Bno055PwrMode,
  Bno055TempSource,
  bno055IsFusion,
  bno055Lsb,
  bno055Placement,
  decodeBno055Block,
  makeBno055SystemStatus,
  packBno055AccelConfig,
  packBno055AxisRemap,
  packBno055CalibrationProfile,
  packBno055GyroConfig,
  packBno055MagConfig,
  packBno055SicMatrix,
  packBno055Units,
  unpackBno055AccelConfig,
  unpackBno055AxisRemap,
  unpackBno055CalibStatus,
  unpackBno055CalibrationProfile,
  unpackBno055GyroConfig,
  unpackBno055MagConfig,
  unpackBno055SicMatrix,
  unpackBno055Units,
  type Bno055AccelConfig,
  type Bno055AxisRemap,
  type Bno055CalibStatus,
  type Bno055CalibrationProfile,
  type Bno055GyroConfig,
  type Bno055MagConfig,
  type Bno055SystemStatus,
  type Bno055Units,
} from "./regs.js";

type Vec3 = [number, number, number];

/**
 * One decoded register block (streamed or polled). Channels are null when
 * the block did not cover them; values are scaled by `units` — the units in
 * force when the stream started (contract 13 §4.2).
 */
export interface Bno055Sample {
  /** MCU uptime: trigger time (stream) / read completion (poll). */
  timestampUs: bigint;
  addr: number;
  raw: Uint8Array;
  units: Bno055Units;
  /** m/s² or mg */
  accel: Vec3 | null;
  /** µT */
  mag: Vec3 | null;
  /** dps or rps */
  gyro: Vec3 | null;
  /** heading, roll, pitch — degrees or radians */
  euler: Vec3 | null;
  /** w, x, y, z (unit quaternion) */
  quaternion: [number, number, number, number] | null;
  /** Acceleration minus gravity, always m/s². */
  linearAccel: Vec3 | null;
  /** Always m/s². */
  gravity: Vec3 | null;
  /** °C or °F */
  temperature: number | null;
  calibration: Bno055CalibStatus | null;
}

/** Decode one register window into a scaled sample. */
export function decodeBno055Sample(
  timestampUs: bigint,
  addr: number,
  data: Uint8Array,
  units: Bno055Units,
): Bno055Sample {
  const r = decodeBno055Block(addr, data);
  const lsb = bno055Lsb(units);
  const scale = <T extends number[]>(v: T | null, k: number): T | null =>
    v === null ? null : (v.map((x) => x / k) as T);
  return {
    timestampUs,
    addr,
    raw: data.slice(),
    units,
    accel: scale(r.accel, lsb.accel),
    mag: scale(r.mag, BNO055_MAG_LSB),
    gyro: scale(r.gyro, lsb.gyro),
    euler: scale(r.euler, lsb.euler),
    quaternion: scale(r.quaternion, BNO055_QUAT_LSB),
    linearAccel: scale(r.linearAccel, BNO055_FUSION_ACCEL_LSB),
    gravity: scale(r.gravity, BNO055_FUSION_ACCEL_LSB),
    temperature: r.temperature === null ? null : r.temperature / lsb.temp,
    calibration: r.calibStat === null ? null : unpackBno055CalibStatus(r.calibStat),
  };
}

export interface Bno055Options extends DeviceOptions {
  /** Sleep implementation (tests inject an instant one). */
  sleepImpl?: (ms: number) => Promise<void>;
}

export interface Bno055ConfigureOptions {
  mode?: Bno055OprMode;
  units?: Bno055Units;
  /** An axis remap, or a datasheet placement name "P0".."P7". */
  axisRemap?: Bno055AxisRemap | string;
  calibration?: Bno055CalibrationProfile;
}

/**
 * BNO055 absolute-orientation IMU.
 *
 * Typical use: `configure()` (CONFIG → units → axis remap → optional
 * calibration profile → NDOF), then `startStream(10)` and read `samples()` /
 * `onSample`, or poll `readSample()`. Streaming is timer-driven: the
 * data-ready interrupt does not exist on the sensor firmware these boards
 * carry (03.11).
 *
 * Multi-step register sequences (page switches, CONFIG round trips) are
 * serialised internally, so concurrent calls cannot interleave mid-sequence.
 * Page 1 is refused while a stream runs.
 */
export class Bno055 extends DepzDevice {
  private readonly sleepImpl: (ms: number) => Promise<void>;
  private chain: Promise<unknown> = Promise.resolve();
  private unitsCache: Bno055Units | null = null;
  private streamingFlag = false;
  private streamUnits: Bno055Units = BNO055_DEFAULT_UNITS;
  private sampleCbs: Array<(s: Bno055Sample) => void> = [];
  private sampleQueues: StreamQueue<Bno055Sample>[] = [];
  private streamParseErrorCount = 0;
  private configured: Required<Pick<Bno055ConfigureOptions, "mode" | "units">> &
    Pick<Bno055ConfigureOptions, "axisRemap" | "calibration"> | null = null;

  constructor(transport: SerialTransport, opts?: Bno055Options) {
    super(transport, opts);
    this.sleepImpl = opts?.sleepImpl ?? ((ms) => new Promise((r) => setTimeout(r, ms)));
  }

  // ── identity & bridge diagnostics ──────────────────────────────────────────

  /** RPT_BNO_INFO: chip ids, sensor firmware revision and bridge counters. */
  bridgeInfo(): Promise<Bno055Info> {
    return this.request(Bno055Cmd.GetInfo, undefined, {
      matcher: DepzDevice.expectReport(Bno055Rpt.Info, unpackBno055Info),
    });
  }

  /** True when the bridge passed the chip-ID handshake and the ids match. */
  async isAlive(): Promise<boolean> {
    try {
      const info = await this.bridgeInfo();
      return info.initialized === 1 && bno055IdsOk(info);
    } catch (err) {
      if (err instanceof DepzError) return false;
      throw err;
    }
  }

  /**
   * Hardware reset via nRESET, then wait out the sensor's own boot tail
   * (contract 13 §5). Stops any stream; the sensor comes back in CONFIG with
   * power-on units — call configure() or restoreConfiguration().
   */
  resetSensor(): Promise<void> {
    return this.locked(async () => {
      await this.request(Bno055Cmd.Reset, undefined, {
        okCompletes: true,
        timeoutMs: BNO055_RESET_TIMEOUT_MS,
      });
      this.streamingFlag = false;
      this.unitsCache = null;
      await this.waitBooted();
    });
  }

  // ── raw register access ────────────────────────────────────────────────────

  /** Read `length` bytes at `addr` on `page` (page 1 refused while streaming). */
  readRegisters(addr: number, length: number, page = 0): Promise<Uint8Array> {
    return this.locked(() => this.onPage(page, async () => (await this.rd(addr, length)).data));
  }

  /** Write `data` at `addr` on `page`. Most config registers need CONFIG mode. */
  writeRegisters(addr: number, data: Uint8Array, page = 0): Promise<void> {
    return this.locked(() => this.onPage(page, () => this.wr(addr, data)));
  }

  async readRegister(addr: number, page = 0): Promise<number> {
    return (await this.readRegisters(addr, 1, page))[0]!;
  }

  writeRegister(addr: number, value: number, page = 0): Promise<void> {
    return this.writeRegisters(addr, Uint8Array.of(value & 0xff), page);
  }

  // ── operating mode, power, units, axes ────────────────────────────────────

  async getOperationMode(): Promise<Bno055OprMode> {
    return ((await this.readRegister(BNO055_REG_OPR_MODE)) & 0x0f) as Bno055OprMode;
  }

  /**
   * Switch OPR_MODE and wait out the switching time; into a fusion mode also
   * wait (≤ 1 s) for the fusion outputs, zero for ~70 ms after CONFIG. The
   * sensor ignores a direct write from one operating mode to another
   * (measured: NDOF → AMG stays NDOF), so such a switch goes through CONFIG.
   */
  setOperationMode(mode: Bno055OprMode): Promise<void> {
    return this.locked(async () => {
      if (mode !== Bno055OprMode.Config) {
        const current = ((await this.rd(BNO055_REG_OPR_MODE, 1)).data[0]! & 0x0f) as Bno055OprMode;
        if (current === mode) return;
        if (current !== Bno055OprMode.Config) await this.switchMode(Bno055OprMode.Config);
      }
      await this.switchMode(mode);
    });
  }

  async getPowerMode(): Promise<Bno055PwrMode> {
    return ((await this.readRegister(BNO055_REG_PWR_MODE)) & 0x03) as Bno055PwrMode;
  }

  setPowerMode(mode: Bno055PwrMode): Promise<void> {
    return this.locked(() => this.inConfig(() => this.wr(BNO055_REG_PWR_MODE, Uint8Array.of(mode))));
  }

  async getUnits(): Promise<Bno055Units> {
    const u = unpackBno055Units(await this.readRegister(BNO055_REG_UNIT_SEL));
    this.unitsCache = u;
    return u;
  }

  setUnits(units: Bno055Units): Promise<void> {
    return this.locked(() =>
      this.inConfig(async () => {
        await this.wr(BNO055_REG_UNIT_SEL, Uint8Array.of(packBno055Units(units)));
        this.unitsCache = units;
      }),
    );
  }

  async getAxisRemap(): Promise<Bno055AxisRemap> {
    const b = await this.readRegisters(BNO055_REG_AXIS_MAP_CONFIG, 2);
    return unpackBno055AxisRemap(b[0]!, b[1]!);
  }

  /** An AxisRemap, or a datasheet placement "P0".."P7" (P1 = default). */
  setAxisRemap(remap: Bno055AxisRemap | string): Promise<void> {
    const r = typeof remap === "string" ? bno055Placement(remap) : remap;
    const bytes = Uint8Array.from(packBno055AxisRemap(r));
    return this.locked(() => this.inConfig(() => this.wr(BNO055_REG_AXIS_MAP_CONFIG, bytes)));
  }

  async getTemperatureSource(): Promise<Bno055TempSource> {
    return ((await this.readRegister(BNO055_REG_TEMP_SOURCE)) & 0x03) as Bno055TempSource;
  }

  setTemperatureSource(source: Bno055TempSource): Promise<void> {
    return this.locked(() =>
      this.inConfig(() => this.wr(BNO055_REG_TEMP_SOURCE, Uint8Array.of(source))),
    );
  }

  /**
   * The usual session setup: CONFIG → units → axis remap → calibration
   * profile → mode (default NDOF). In a fusion mode it resolves once the
   * fusion outputs are live. Remembered for restoreConfiguration().
   */
  configure(opts: Bno055ConfigureOptions = {}): Promise<void> {
    const mode = opts.mode ?? Bno055OprMode.Ndof;
    const units = opts.units ?? BNO055_DEFAULT_UNITS;
    const axis = typeof opts.axisRemap === "string" ? bno055Placement(opts.axisRemap) : opts.axisRemap;
    return this.locked(async () => {
      await this.waitBooted();
      await this.switchMode(Bno055OprMode.Config);
      await this.wr(BNO055_REG_UNIT_SEL, Uint8Array.of(packBno055Units(units)));
      this.unitsCache = units;
      if (axis !== undefined) {
        await this.wr(BNO055_REG_AXIS_MAP_CONFIG, Uint8Array.from(packBno055AxisRemap(axis)));
      }
      if (opts.calibration !== undefined) {
        await this.wr(BNO055_REG_CALIB_PROFILE, packBno055CalibrationProfile(opts.calibration));
      }
      await this.switchMode(mode);
      if (bno055IsFusion(mode)) await this.waitFusionStarted(true);
      this.configured = { mode, units, axisRemap: axis, calibration: opts.calibration };
    });
  }

  /**
   * Re-apply the last configure() — after resetSensor(), or when
   * bridgeInfo().sensorResets rose (the bridge's bus recovery pulses nRESET).
   */
  restoreConfiguration(): Promise<void> {
    if (this.configured === null) {
      return Promise.reject(new DepzError("configure() has not been called"));
    }
    return this.configure(this.configured);
  }

  // ── status, self-test, calibration ────────────────────────────────────────

  /** ST_RESULT, SYS_CLK_STATUS, SYS_STATUS, SYS_ERR (INT_STA skipped). */
  systemStatus(): Promise<Bno055SystemStatus> {
    return this.locked(() => this.readSystemStatus());
  }

  /**
   * Built-in self-test (datasheet §3.9.2): CONFIG, SYS_TRIGGER SELF_TEST,
   * ~400 ms, then read the verdict. The mode is restored. Not while streaming.
   */
  selfTest(): Promise<Bno055SystemStatus> {
    this.requireNotStreaming();
    return this.locked(async () => {
      const previous = ((await this.rd(BNO055_REG_OPR_MODE, 1)).data[0]! & 0x0f) as Bno055OprMode;
      if (previous !== Bno055OprMode.Config) await this.switchMode(Bno055OprMode.Config);
      try {
        const clk = (await this.rd(BNO055_REG_SYS_TRIGGER, 1)).data[0]! & BNO055_SYS_TRIGGER_CLK_SEL;
        await this.wr(BNO055_REG_SYS_TRIGGER, Uint8Array.of(clk | BNO055_SYS_TRIGGER_SELF_TEST));
        await this.sleepImpl(BNO055_SELF_TEST_MS);
        const status = await this.readSystemStatus();
        // Staying in CONFIG would leave SYS_STATUS at 4 for good.
        if (previous === Bno055OprMode.Config) await this.clearSelfTestStatus();
        return status;
      } finally {
        if (previous !== Bno055OprMode.Config) await this.switchMode(previous);
      }
    });
  }

  async calibrationStatus(): Promise<Bno055CalibStatus> {
    return unpackBno055CalibStatus(await this.readRegister(BNO055_REG_CALIB_STAT));
  }

  /** Offsets and radii (0x55..0x6A) — CONFIG only; the driver switches there and back. */
  readCalibrationProfile(): Promise<Bno055CalibrationProfile> {
    return this.locked(() =>
      this.inConfig(async () =>
        unpackBno055CalibrationProfile(
          (await this.rd(BNO055_REG_CALIB_PROFILE, BNO055_CALIB_PROFILE_LEN)).data,
        ),
      ),
    );
  }

  /**
   * Restore a stored profile: CONFIG, all 22 bytes, back to the previous
   * mode. A starting point — fusion refines it as soon as it resumes.
   */
  writeCalibrationProfile(profile: Bno055CalibrationProfile): Promise<void> {
    return this.locked(() =>
      this.inConfig(() => this.wr(BNO055_REG_CALIB_PROFILE, packBno055CalibrationProfile(profile))),
    );
  }

  async getSicMatrix(): Promise<number[]> {
    return unpackBno055SicMatrix(await this.readRegisters(BNO055_REG_SIC_MATRIX, 18));
  }

  setSicMatrix(matrix: readonly number[] = BNO055_SIC_IDENTITY): Promise<void> {
    return this.locked(() => this.inConfig(() => this.wr(BNO055_REG_SIC_MATRIX, packBno055SicMatrix(matrix))));
  }

  // ── page 1: raw sensor configuration (non-fusion modes) ────────────────────

  async getAccelConfig(): Promise<Bno055AccelConfig> {
    return unpackBno055AccelConfig(await this.readRegister(BNO055_REG1_ACC_CONFIG, 1));
  }

  setAccelConfig(c: Bno055AccelConfig): Promise<void> {
    return this.writePage1Config(BNO055_REG1_ACC_CONFIG, Uint8Array.of(packBno055AccelConfig(c)));
  }

  async getGyroConfig(): Promise<Bno055GyroConfig> {
    return unpackBno055GyroConfig(await this.readRegisters(BNO055_REG1_GYR_CONFIG_0, 2, 1));
  }

  setGyroConfig(c: Bno055GyroConfig): Promise<void> {
    return this.writePage1Config(BNO055_REG1_GYR_CONFIG_0, packBno055GyroConfig(c));
  }

  async getMagConfig(): Promise<Bno055MagConfig> {
    return unpackBno055MagConfig(await this.readRegister(BNO055_REG1_MAG_CONFIG, 1));
  }

  setMagConfig(c: Bno055MagConfig): Promise<void> {
    return this.writePage1Config(BNO055_REG1_MAG_CONFIG, Uint8Array.of(packBno055MagConfig(c)));
  }

  /** The chip's 16-byte unique id (page 1, 0x50..0x5F). */
  uniqueId(): Promise<Uint8Array> {
    return this.readRegisters(BNO055_REG1_UNIQUE_ID, BNO055_UNIQUE_ID_LEN, 1);
  }

  // ── interrupts (motion only on SW rev 03.11) ───────────────────────────────

  getInterruptEnable(): Promise<number> {
    return this.readRegister(BNO055_REG1_INT_EN, 1);
  }

  setInterruptEnable(mask: number): Promise<void> {
    return this.writeRegister(BNO055_REG1_INT_EN, mask, 1);
  }

  getInterruptMask(): Promise<number> {
    return this.readRegister(BNO055_REG1_INT_MSK, 1);
  }

  setInterruptMask(mask: number): Promise<void> {
    return this.writeRegister(BNO055_REG1_INT_MSK, mask, 1);
  }

  /** One raw motion-interrupt setting (page 1, 0x11..0x1F). Not while streaming. */
  setInterruptSetting(register: number, value: number): Promise<void> {
    if (register < BNO055_REG1_INT_SETTINGS_FIRST || register > BNO055_REG1_INT_SETTINGS_LAST) {
      return Promise.reject(new RangeError(`0x${register.toString(16)} is not a page-1 interrupt setting`));
    }
    return this.writePage1Config(register, Uint8Array.of(value & 0xff));
  }

  /** INT_STA — which interrupts fired. Clears on read. */
  readInterruptStatus(): Promise<number> {
    return this.readRegister(BNO055_REG_INT_STA);
  }

  /** SYS_TRIGGER RST_INT: reset the interrupt status bits and the INT pin. */
  clearInterrupt(): Promise<void> {
    return this.locked(async () => {
      const clk = (await this.rd(BNO055_REG_SYS_TRIGGER, 1)).data[0]! & BNO055_SYS_TRIGGER_CLK_SEL;
      await this.wr(BNO055_REG_SYS_TRIGGER, Uint8Array.of(clk | BNO055_SYS_TRIGGER_RST_INT));
    });
  }

  // ── data ───────────────────────────────────────────────────────────────────

  /** Poll one register block (default the full 46-byte block) and decode it. */
  readSample(block: readonly [number, number] = BNO055_FULL_BLOCK): Promise<Bno055Sample> {
    return this.locked(async () => {
      const units = await this.currentUnits();
      const r = await this.rd(block[0], block[1]);
      return decodeBno055Sample(r.timestampUs, block[0], r.data, units);
    });
  }

  /** (w, x, y, z) — the cheapest orientation read. */
  async readQuaternion(): Promise<[number, number, number, number]> {
    return (await this.readSample(BNO055_QUAT_BLOCK)).quaternion!;
  }

  /**
   * Arm the bridge: read `block` every `periodMs` and push it. Fusion runs at
   * 100 Hz, so 10 ms is the useful floor. `trigger = BNO055_TRIGGER_INT`
   * reads on the INT edge with `periodMs` as a watchdog. Replaces a stream.
   */
  startStream(
    periodMs = 10,
    block: readonly [number, number] = BNO055_FULL_BLOCK,
    trigger = BNO055_TRIGGER_TIMER,
  ): Promise<void> {
    const [addr, length] = block;
    if (length < 1 || length > BNO055_XFER_MAX || addr + length > 0x100) {
      return Promise.reject(new RangeError(`block 0x${addr.toString(16)}+${length} outside 1..128 / page`));
    }
    return this.locked(async () => {
      // Units first: the read pump may decode the first sample before the
      // command's own reply is processed.
      this.streamUnits = await this.currentUnits();
      await this.request(Bno055Cmd.StartStream, packBno055StartStream(trigger, addr, length, periodMs), {
        okCompletes: true,
      });
      this.streamingFlag = true;
    });
  }

  async stopStream(): Promise<void> {
    if (!this.streamingFlag) return;
    try {
      await this.request(Bno055Cmd.StopStream, undefined, { okCompletes: true });
    } finally {
      this.streamingFlag = false;
    }
  }

  get streaming(): boolean {
    return this.streamingFlag;
  }

  /** Subscribe to streamed samples (read-pump context; don't block). */
  onSample(cb: (s: Bno055Sample) => void): () => void {
    this.sampleCbs.push(cb);
    return () => {
      this.sampleCbs = this.sampleCbs.filter((c) => c !== cb);
    };
  }

  /** Async iterator over streamed samples — bounded, drop-oldest. */
  samples(maxsize = 256): StreamQueue<Bno055Sample> {
    const queue: StreamQueue<Bno055Sample> = new StreamQueue(maxsize, () => {
      this.sampleQueues = this.sampleQueues.filter((q) => q !== queue);
      deregister();
    });
    this.sampleQueues.push(queue);
    const deregister = this.registerStream(queue);
    return queue;
  }

  /** Wait for the next streamed sample. */
  async getSample(timeoutMs = 1000): Promise<Bno055Sample> {
    const queue = new StreamQueue<Bno055Sample>(1);
    this.sampleQueues.push(queue);
    const deregister = this.registerStream(queue);
    let timer: ReturnType<typeof setTimeout> | undefined;
    try {
      const result = await Promise.race([
        queue.next(),
        new Promise<never>((_, reject) => {
          timer = setTimeout(() => reject(new DepzTimeoutError(Bno055Rpt.Stream, timeoutMs)), timeoutMs);
        }),
      ]);
      if (result.done === true) throw new DepzError("device closed");
      return result.value;
    } finally {
      if (timer !== undefined) clearTimeout(timer);
      this.sampleQueues = this.sampleQueues.filter((q) => q !== queue);
      deregister();
    }
  }

  /** Stream reports dropped because they did not decode (short block). */
  get streamParseErrors(): number {
    return this.streamParseErrorCount;
  }

  get streamDroppedCounts(): number[] {
    return this.sampleQueues.map((q) => q.droppedCount);
  }

  // ── internal ───────────────────────────────────────────────────────────────

  /** Run `fn` after every previously queued sequence (non-reentrant). */
  private locked<T>(fn: () => Promise<T>): Promise<T> {
    const run = this.chain.then(fn, fn);
    this.chain = run.catch(() => undefined);
    return run;
  }

  private async rd(addr: number, length: number): Promise<{ timestampUs: bigint; data: Uint8Array }> {
    const out = new Uint8Array(length);
    let off = 0;
    let ts = 0n;
    while (off < length) {
      const n = Math.min(length - off, BNO055_XFER_MAX);
      const rep = await this.request<Bno055RegData>(Bno055Cmd.ReadReg, packBno055ReadReg(addr + off, n), {
        matcher: DepzDevice.expectReport(Bno055Rpt.RegData, unpackBno055RegData),
      });
      if (rep.data.length !== n) {
        throw new DepzError(`READ_REG 0x${(addr + off).toString(16)}: expected ${n}, got ${rep.data.length}`);
      }
      out.set(rep.data, off);
      ts = rep.timestampUs;
      off += n;
    }
    return { timestampUs: ts, data: out };
  }

  private async wr(addr: number, data: Uint8Array): Promise<void> {
    for (let off = 0; off < data.length; off += BNO055_XFER_MAX) {
      const chunk = data.subarray(off, off + BNO055_XFER_MAX);
      await this.request(Bno055Cmd.WriteReg, packBno055WriteReg(addr + off, chunk), { okCompletes: true });
    }
  }

  private async onPage<T>(page: number, fn: () => Promise<T>): Promise<T> {
    if (page === 0) return fn();
    if (page !== 1) throw new RangeError(`BNO055 has register pages 0 and 1, not ${page}`);
    this.requireNotStreaming();
    await this.wr(BNO055_REG_PAGE_ID, Uint8Array.of(1));
    try {
      return await fn();
    } finally {
      await this.wr(BNO055_REG_PAGE_ID, Uint8Array.of(0));
    }
  }

  private async switchMode(mode: Bno055OprMode): Promise<void> {
    await this.wr(BNO055_REG_OPR_MODE, Uint8Array.of(mode));
    await this.sleepImpl(
      mode === Bno055OprMode.Config ? BNO055_MODE_SWITCH_TO_CONFIG_MS : BNO055_MODE_SWITCH_FROM_CONFIG_MS,
    );
    if (bno055IsFusion(mode)) await this.waitFusionStarted(false);
  }

  /** Run `fn` in CONFIG mode and put the previous mode back. */
  private async inConfig<T>(fn: () => Promise<T>): Promise<T> {
    const previous = ((await this.rd(BNO055_REG_OPR_MODE, 1)).data[0]! & 0x0f) as Bno055OprMode;
    if (previous !== Bno055OprMode.Config) await this.switchMode(Bno055OprMode.Config);
    try {
      return await fn();
    } finally {
      if (previous !== Bno055OprMode.Config) await this.switchMode(previous);
    }
  }

  private writePage1Config(addr: number, data: Uint8Array): Promise<void> {
    this.requireNotStreaming();
    return this.locked(() => this.inConfig(() => this.onPage(1, () => this.wr(addr, data))));
  }

  private async readSystemStatus(): Promise<Bno055SystemStatus> {
    const st = (await this.rd(BNO055_REG_ST_RESULT, 1)).data[0]!;
    const b = (await this.rd(BNO055_REG_SYS_CLK_STATUS, 3)).data;
    return makeBno055SystemStatus(st, b[0]!, b[1]!, b[2]!);
  }

  /** Poll SYS_STATUS until the sensor's own boot (init + POST) is over. */
  /** A self-test run in CONFIG leaves SYS_STATUS at 4 until the mode leaves CONFIG; step into ACCONLY and back. */
  private async clearSelfTestStatus(): Promise<void> {
    await this.switchMode(Bno055OprMode.AccOnly);
    await this.switchMode(Bno055OprMode.Config);
  }

  /** Poll SYS_STATUS until boot (init + POST) is over; a 4 that outlasts POST is a self-test leftover, cleared. */
  private async waitBooted(): Promise<void> {
    const deadline = Date.now() + BNO055_BOOT_SETTLE_TIMEOUT_MS;
    let inSelfTest = 0;
    let cleared = false;
    for (;;) {
      const status = (await this.rd(BNO055_REG_SYS_STATUS, 1)).data[0]!;
      if (!BNO055_SYS_STATUS_BOOTING.includes(status)) return;
      inSelfTest = status === 4 ? inSelfTest + 1 : 0;
      if (inSelfTest >= BNO055_STUCK_SELF_TEST_POLLS && !cleared) {
        await this.clearSelfTestStatus();
        cleared = true;
        inSelfTest = 0;
        continue;
      }
      if (Date.now() > deadline) throw new DepzError("BNO055 did not finish booting (SYS_STATUS stuck)");
      await this.sleepImpl(5);
    }
  }

  /** A running fusion never outputs the all-zero quaternion. */
  private async waitFusionStarted(strict: boolean): Promise<void> {
    const deadline = Date.now() + BNO055_FUSION_START_TIMEOUT_MS;
    while ((await this.rd(BNO055_REG_QUA_DATA, 8)).data.every((b) => b === 0)) {
      if (Date.now() > deadline) {
        if (strict) throw new DepzError("BNO055 fusion did not start (quaternion stays zero)");
        return;
      }
      await this.sleepImpl(10);
    }
  }

  private async currentUnits(): Promise<Bno055Units> {
    if (this.unitsCache === null) {
      this.unitsCache = unpackBno055Units((await this.rd(BNO055_REG_UNIT_SEL, 1)).data[0]!);
    }
    return this.unitsCache;
  }

  private requireNotStreaming(): void {
    if (this.streamingFlag) throw new DepzError("stopStream() first — the stream reads page 0");
  }

  protected override handleReport(pkt: PacketEvent): boolean {
    if (pkt.cmd !== Bno055Rpt.Stream) return false;
    if (pkt.payload.length < 10) {
      this.streamParseErrorCount += 1;
      return true;
    }
    const s = unpackBno055Stream(pkt.payload);
    if (s.data.length !== s.length) {
      this.streamParseErrorCount += 1;
      return true;
    }
    const sample = decodeBno055Sample(s.timestampUs, s.addr, s.data, this.streamUnits);
    for (const cb of [...this.sampleCbs]) cb(sample);
    for (const q of [...this.sampleQueues]) q.push(sample);
    return true;
  }
}
