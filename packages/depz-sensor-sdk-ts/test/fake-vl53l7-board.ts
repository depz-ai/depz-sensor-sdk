/**
 * In-process fake of the `APP_VL53L7` I2C bridge board (contract 11 §2) over a
 * LoopbackTransport pair — the three board commands the L5/L7 board adds to
 * the VL53L8 bridge (PIN_CTRL, GET_INFO, SET_I2C_SPEED), their refusals and
 * the RPT_VL53_INFO counters. Port of the board half of the Python
 * `tests/fake_vl53l7.py`.
 *
 * There is no sensor model behind it: the TS SDK has no in-process L5/L7 die,
 * so init() (the blob download) is stubbed by the tests that need it. The
 * register bridge answers a plain register file, enough for `isAlive()`.
 */

import {
  CrcType,
  LoopbackTransport,
  PacketParser,
  buildPacket,
  type PacketEvent,
} from "../src/index.js";
import {
  PinAction,
  VL53L7_I2C_SPEED_STEPS_KHZ,
  Vl53l7I2cError,
} from "../src/protocol/vl53l7.js";

const ERR_INVALID_CMD = 0x02;
const ERR_PAYLOAD_FORMAT = 0x03;
const ERR_INVALID_PARAM = 0x04;
const ERR_BUSY = 0x06;
const ERR_HARDWARE_FAULT = 0x09;

export class FakeVl53l7Board {
  static readonly SOFTWARE_NAME = "APP_VL53L7_v0.52";
  static readonly DEVICE_NAME = "DEPZ ToF Sensor VL53L7CX USB v2.1 TXK5KAX6X4";

  readonly transport: LoopbackTransport;
  readonly side: LoopbackTransport;
  readonly regs = new Uint8Array(0x10000);
  readonly rx: Array<{ cmd: number; payload: Uint8Array }> = [];

  // ── bridge state (RPT_VL53_INFO) ──
  intEdges = 0;
  framesDropped = 0;
  i2cErrors = 0;
  lastI2cError: number = Vl53l7I2cError.Ok;
  lpn = true;
  intLevel = 0;
  i2cKhz = 1000; // firmware default since v0.52
  frameSize = 0;
  streaming = false;
  /** SET_I2C_SPEED answers ERR_BUSY. */
  transferInFlight = false;

  mcuTimeUs = 5_000_000n;
  readonly pinActions: number[] = [];
  readonly speedRequests: number[] = [];

  private txSeq = 0;
  private readonly parser = new PacketParser();

  constructor() {
    const [host, device] = LoopbackTransport.pair();
    this.transport = host;
    this.side = device;
    void this.run();
  }

  async close(): Promise<void> {
    await this.side.close();
  }

  infoPayload(): Uint8Array {
    const out = new Uint8Array(20);
    const dv = new DataView(out.buffer);
    dv.setUint32(0, this.intEdges, true);
    dv.setUint32(4, this.framesDropped, true);
    dv.setUint32(8, this.i2cErrors, true);
    out[12] = this.lastI2cError;
    out[13] = this.lpn ? 1 : 0;
    out[14] = this.intLevel;
    dv.setUint16(15, this.i2cKhz, true);
    dv.setUint16(17, this.frameSize, true);
    out[19] = this.streaming ? 1 : 0;
    return out;
  }

  private async send(cmd: number, payload: Uint8Array = new Uint8Array(0)): Promise<void> {
    try {
      await this.side.write(buildPacket(cmd, payload, this.txSeq, CrcType.None));
    } catch {
      return; // link closed
    }
    this.txSeq = (this.txSeq + 1) & 0xff;
  }

  private status(cmd: number, status: number): Promise<void> {
    return this.send(0x80, Uint8Array.of(cmd, status));
  }

  private text(cmd: number, text: string): Promise<void> {
    const ascii = new TextEncoder().encode(text);
    const payload = new Uint8Array(ascii.length + 2);
    payload[0] = cmd;
    payload.set(ascii, 1);
    return this.send(0x81, payload);
  }

  private async run(): Promise<void> {
    for await (const chunk of this.side.readable()) {
      for (const ev of this.parser.feed(chunk)) {
        if (ev.type === "packet") await this.handle(ev);
      }
    }
  }

  private async handle(pkt: PacketEvent): Promise<void> {
    const { cmd, payload } = pkt;
    this.rx.push({ cmd, payload: payload.slice() });
    const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
    switch (cmd) {
      case 0x03:
        return this.text(cmd, FakeVl53l7Board.DEVICE_NAME);
      case 0x04:
        return this.text(cmd, FakeVl53l7Board.SOFTWARE_NAME);
      case 0x32: {
        // READ_REG: the I2C board's own limits, then LPn (a NACKing sensor).
        const addr = dv.getUint16(0, true);
        const len = dv.getUint16(2, true);
        if (len < 1 || len > 1536 || addr + len > 0x10000) return this.status(cmd, ERR_INVALID_PARAM);
        if (!this.lpn) {
          this.i2cErrors += 1;
          this.lastI2cError = Vl53l7I2cError.Nack;
          return this.status(cmd, ERR_HARDWARE_FAULT);
        }
        // RPT_VL53_REG_DATA: echoed opcode, ts u64, data
        const data = this.regs.slice(addr, addr + len);
        const out = new Uint8Array(9 + data.length);
        out[0] = cmd;
        new DataView(out.buffer).setBigUint64(1, this.mcuTimeUs, true);
        out.set(data, 9);
        this.mcuTimeUs += 100n;
        return this.send(0x91, out);
      }
      case 0x33: {
        const addr = dv.getUint16(0, true);
        const data = payload.slice(2);
        if (data.length < 1 || data.length > 2048 || addr + data.length > 0x10000) {
          return this.status(cmd, ERR_INVALID_PARAM);
        }
        if (!this.lpn) {
          this.i2cErrors += 1;
          this.lastI2cError = Vl53l7I2cError.Nack;
          return this.status(cmd, ERR_HARDWARE_FAULT);
        }
        this.regs.set(data, addr);
        return this.status(cmd, 0);
      }
      case 0x34: {
        // PIN_CTRL
        if (payload.length !== 1) return this.status(cmd, ERR_PAYLOAD_FORMAT);
        const action = payload[0]!;
        if (!(action in PinAction)) return this.status(cmd, ERR_INVALID_PARAM);
        this.pinActions.push(action);
        if (action === PinAction.LpnOff) {
          this.streaming = false;
          this.lpn = false;
        } else if (action === PinAction.LpnOn) {
          this.lpn = true;
        } else if (action === PinAction.SoftCycle) {
          this.streaming = false;
          this.lpn = true;
          this.i2cErrors = 0;
          this.lastI2cError = Vl53l7I2cError.Ok;
        }
        return this.status(cmd, 0);
      }
      case 0x35:
        this.streaming = true;
        return this.status(cmd, 0);
      case 0x36:
        this.streaming = false;
        return this.status(cmd, 0);
      case 0x37:
        // GET_INFO: bridge state only, no echoed command byte
        return this.send(0x92, this.infoPayload());
      case 0x38: {
        // SET_I2C_SPEED
        if (payload.length !== 2) return this.status(cmd, ERR_PAYLOAD_FORMAT);
        const khz = dv.getUint16(0, true);
        this.speedRequests.push(khz);
        if (this.transferInFlight || this.streaming) return this.status(cmd, ERR_BUSY);
        let best = VL53L7_I2C_SPEED_STEPS_KHZ[0]!;
        for (const s of VL53L7_I2C_SPEED_STEPS_KHZ) if (Math.abs(s - khz) < Math.abs(best - khz)) best = s;
        this.i2cKhz = best;
        return this.status(cmd, 0);
      }
      default:
        return this.status(cmd, ERR_INVALID_CMD);
    }
  }
}
