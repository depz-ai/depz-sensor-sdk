/**
 * Differential-testing adapter for the TypeScript SDK — the REFERENCE side
 * (contracts/16_DIFFTEST.md). A long-lived process speaking JSON Lines:
 *   {"id": n, "op": "<op>", "in": {...}}  ->  {"id": n, "out": ...}
 *                                         or  {"id": n, "error": "invalid" | "unsupported"}
 * Run: `bun test/difftest-adapter.ts` (driver: scripts/difftest.py). Output
 * shapes come from test/golden/shapes.ts, the helpers the vector tests use.
 *
 * Integers: JSON integers outside the safe range arrive as `bigint` (parsed
 * from the source text, so u64 inputs stay exact); `big()` feeds APIs that
 * take bigint, `num()` APIs that take number (a value a JS caller could not
 * pass exactly anyway). Outputs serialize bigint exactly.
 */

import { createInterface } from "node:readline";
import {
  CrcType,
  DatasetReader,
  DepzDevice,
  LoopbackTransport,
  ReplayTransport,
  PacketParser,
  buildPacket,
  bno086RecordValue,
  crc8Maxim,
  crc16CcittFalse,
  crc16Modbus,
  crc32IsoHdlc,
  isKnownDepzUsb,
  orderPortsBySerial,
  packEchoDecay,
  packSamplePeriod,
  packSetPayloadCrcType,
  packSyncPinConfig,
  packSyncTime,
  parseFwDepz,
  fwDepzPayloadCrcOk,
  parseGyroRvCargo,
  parseInputCargo,
  parseSoftwareName,
  resolveSensorClass,
  stripDeviceString,
  syncTimeOffsetRtt,
  unpackEchoDecay,
  unpackSamplePeriod,
  unpackSequenceError,
  unpackSr04Data,
  unpackStatus,
  unpackTemperature,
  unpackText,
  usbModelHint,
  ShtpLayer,
  packShtpHeader,
  unpackShtpHeader,
  unpackCommandResponse,
  unpackFeatureResponse,
  unpackFrsReadResponse,
  unpackFrsWriteResponse,
  unpackProductId,
  buildCommandRequest,
  buildFrsReadRequest,
  buildFrsWriteData,
  buildFrsWriteRequest,
  buildGetFeatureRequest,
  buildProductIdRequest,
  buildSetFeature,
  type DetectionThreshold,
  type DeviceEvent,
} from "../src/index.js";
import { checkUint } from "../src/protocol/wire.js";
import { packBno055ReadReg, packBno055StartStream, packBno055WriteReg } from "../src/protocol/bno055.js";
import {
  Tmf8829Cmd,
  packTmf8829ReadReg,
  packTmf8829StartStream,
  packTmf8829WriteReg,
} from "../src/protocol/tmf8829.js";
import {
  packReadReg as l4PackReadReg,
  packSetI2cSpeed as l4PackSetI2cSpeed,
  packStartStream as l4PackStartStream,
  packWriteReg as l4PackWriteReg,
  packXshut as l4PackXshut,
} from "../src/protocol/vl53l4.js";
import {
  packPinCtrl,
  packVl53l7ReadReg,
  packVl53l7SetI2cSpeed,
  packVl53l7WriteReg,
  resolveVl53l7Model,
} from "../src/protocol/vl53l7.js";
import { packStartStream as packVl53l8StartStream } from "../src/protocol/vl53l8.js";
import { packVl53lxSetAddrWidth, packVl53lxStartStream } from "../src/protocol/vl53lx.js";
import {
  bno055FullyCalibrated,
  packBno055AccelConfig,
  packBno055AxisRemap,
  packBno055CalibStatus,
  packBno055CalibrationProfile,
  packBno055GyroConfig,
  packBno055MagConfig,
  packBno055Units,
  unpackBno055AccelConfig,
  unpackBno055AxisRemap,
  unpackBno055CalibStatus,
  unpackBno055CalibrationProfile,
  unpackBno055GyroConfig,
  unpackBno055MagConfig,
  unpackBno055Units,
} from "../src/sensors/bno055/regs.js";
import { Tmf8829CaptureTimer, Tmf8829SensorClock } from "../src/sensors/tmf8829/clock.js";
import { tmf8829CfgGet, tmf8829CfgSet, type Tmf8829CfgField } from "../src/sensors/tmf8829/config.js";
import {
  decodeOffset,
  decodeRangeTiming,
  decodeSigmaThreshold,
  decodeSignalThreshold,
  decodeXtalk,
  offsetRaw,
  rangeTimingRegisters,
  sigmaThresholdRaw,
  signalThresholdRaw,
  xtalkRaw,
} from "../src/sensors/vl53l4/uld.js";
import { CnhConfig, decode as cnhDecode } from "../src/sensors/vl53l8/cnh.js";
import {
  DCI_DET_THRESH_START,
  DCI_DET_THRESH_VALID_STATUS,
  DCI_MOTION_DETECTOR_CFG,
  DCI_XTALK_CFG,
} from "../src/sensors/vl53l8/uld.js";
import { decodeDieBlock, decodeHistogramRaw } from "../src/sensors/vl53lx/decode.js";
import * as registry from "../src/sensors/vl53lx/uld/registry.js";
import { resolveVl53lxClass } from "../src/sensors/vl53lx/vl53lx.js";
import {
  bno055Block,
  bno055Decode,
  bno086Cargo,
  bno086Fields,
  framingCollect,
  snake,
  tmf8829Decode,
  tmf8829Frame,
  tmf8829Measurements,
  tmf8829Reassembly,
  vl53l4Decode,
  vl53l4Result,
  vl53l7Info,
  vl53l8Decode,
  vl53lxInfo,
  vl53lxL0xRaw,
} from "./golden/shapes.js";
import { fromHex, toHex } from "./golden/vectors.js";
import { makeDriver } from "./fake-vl53l8.js";

// eslint-disable-next-line @typescript-eslint/no-explicit-any
type Any = any;

/** Ops this SDK declares it cannot do, with the reason (never a silent gap). */
const UNSUPPORTED: Record<string, string> = {};

const HEX = /^(?:[0-9a-f]{2})*$/;
function h(s: unknown): Uint8Array {
  if (typeof s !== "string" || !HEX.test(s)) throw new TypeError("not a hex string");
  return fromHex(s);
}
const num = (v: unknown): number => (typeof v === "bigint" ? Number(v) : (v as number));
const big = (v: unknown): bigint => BigInt(v as number | bigint);

const L4_TUNING: Record<string, [(n: number) => number, (n: number) => number]> = {
  offset: [offsetRaw, decodeOffset],
  xtalk: [xtalkRaw, decodeXtalk],
  signal_threshold: [signalThresholdRaw, decodeSignalThreshold],
  sigma_threshold: [sigmaThresholdRaw, decodeSigmaThreshold],
};

export const OPS: Record<string, (i: Any) => unknown | Promise<unknown>> = {
  crc: (i) => {
    const raw = h(i.input);
    return {
      crc8_maxim: crc8Maxim(raw),
      crc16_modbus: crc16Modbus(raw),
      crc32_iso_hdlc: crc32IsoHdlc(raw),
      crc16_ccitt_false: crc16CcittFalse(raw),
    };
  },
  framing_encode: (i) => ({ frame: toHex(buildPacket(num(i.cmd), h(i.payload), num(i.seq), num(i.crc_type))) }),
  framing_decode: (i) =>
    framingCollect(new PacketParser(), "chunks" in i ? (i.chunks as string[]).map(h) : [h(i.stream)]),
  identity: (i) => {
    const id = parseSoftwareName(stripDeviceString(h(i.raw)));
    return { mode: id.mode, sensor_type: id.sensorType, software_name: id.softwareName, version: id.version };
  },
  usb_identity: (i) => ({ known: isKnownDepzUsb(num(i.vid), num(i.pid)), model: usbModelHint(num(i.vid), num(i.pid)) }),
  usb_serial_ordering: (i) => ({
    order: orderPortsBySerial(
      (i.ports as Any[]).map((p) => ({ path: p.port, serialNumber: p.serial ?? undefined })),
    ).map((p) => p.path),
  }),
  probe_resolve: (i) => ({
    class: resolveSensorClass(i.software_name, i.device_name, i.usb_vid === null ? null : num(i.usb_vid),
      i.usb_pid === null ? null : num(i.usb_pid)),
  }),
  common_encode: (i) => {
    let p: Uint8Array;
    if (i.kind === "sync_time_request") p = packSyncTime(big(i.pc_timestamp_us));
    else if (i.kind === "set_payload_crc_type") p = packSetPayloadCrcType(num(i.crc_type));
    else if (i.kind === "sync_pin_config") p = packSyncPinConfig({ pin: num(i.pin), mode: num(i.mode), polarity: num(i.polarity) });
    else throw new Error(i.kind);
    return { payload: toHex(p) };
  },
  common_decode: (i) => {
    const p = h(i.payload);
    switch (i.report) {
      case 0x80: return unpackStatus(p);
      case 0x81: return unpackText(p);
      case 0x83: {
        const r = unpackTemperature(p);
        return { timestamp_us: r.timestampUs, raw_decidegrees: r.rawDecidegrees };
      }
      case 0x84: {
        const r = unpackSequenceError(p);
        return { expected_seq: r.expectedSeq, received_seq: r.receivedSeq };
      }
    }
    throw new Error(`report ${i.report}`);
  },
  // The event the device layer emits for an unsolicited common report —
  // common_decode's twin through the dispatcher (contract 07 §2/§3): the
  // packet goes in on a loopback pair with nothing pending, and the first
  // event out is the answer.
  common_event: async (i) => {
    const [transport, side] = LoopbackTransport.pair();
    const dev = new DepzDevice(transport, { timeoutMs: 1000 });
    let ev: DeviceEvent;
    try {
      const first = new Promise<DeviceEvent>((resolve) => dev.onEvent(resolve));
      await dev.open();
      await side.write(buildPacket(num(i.report), h(i.payload)));
      let timer: ReturnType<typeof setTimeout> | undefined;
      const late = new Promise<never>((_, reject) => {
        timer = setTimeout(() => reject(new Error("the device emitted no event")), 2000);
      });
      ev = await Promise.race([first, late]).finally(() => clearTimeout(timer));
    } finally {
      await dev.close();
    }
    switch (ev.type) {
      case "unsolicitedStatus":
        return { event: "unsolicited_status", status: ev.status };
      case "sequenceError":
        return {
          event: "sequence_error",
          expected_seq: ev.expectedSeq,
          received_seq: ev.receivedSeq,
          reported_by_device: ev.reportedByDevice,
        };
      case "text":
        return { event: "text", cmd: ev.cmd, text: ev.text };
      case "temperature":
        return { event: "temperature", timestamp_us: ev.timestampUs, celsius: ev.celsius };
    }
    throw new Error(`event ${ev.type}`);
  },
  // Host-observed seq gaps (contract 01 §5 step 5, ERRATA A1): one packet per
  // `seqs` entry (unknown report 0x7F, payload 00 — each a hex `text` event)
  // on a loopback pair; once every packet's own event is out, the `seqGap`
  // events and stats.seqGaps are the answer (a gap comes before its packet).
  common_seq_gap: async (i) => {
    const seqs = (i.seqs as unknown[]).map(num);
    if (seqs.some((s) => !Number.isInteger(s) || s < 0 || s > 0xff)) throw new Error("a seq is a byte");
    const [transport, side] = LoopbackTransport.pair();
    const dev = new DepzDevice(transport, { timeoutMs: 1000 });
    const got: DeviceEvent[] = [];
    let seqGaps: number;
    try {
      const all = new Promise<void>((resolve) => {
        let texts = 0;
        if (seqs.length === 0) resolve();
        dev.onEvent((ev) => {
          got.push(ev);
          if (ev.type === "text" && ++texts === seqs.length) resolve();
        });
      });
      await dev.open();
      for (const s of seqs) await side.write(buildPacket(0x7f, Uint8Array.of(0), s));
      let timer: ReturnType<typeof setTimeout> | undefined;
      const late = new Promise<never>((_, reject) => {
        timer = setTimeout(() => reject(new Error("a packet produced no event")), 2000);
      });
      await Promise.race([all, late]).finally(() => clearTimeout(timer));
      seqGaps = dev.stats.seqGaps;
    } finally {
      await dev.close();
    }
    return {
      gaps: got.flatMap((ev) =>
        ev.type === "seqGap"
          ? [{ expected_seq: ev.expectedSeq, received_seq: ev.receivedSeq, missed: ev.missed }]
          : [],
      ),
      seq_gaps: seqGaps,
    };
  },
  sync_time_math: (i) => {
    const r = syncTimeOffsetRtt(big(i.t1), big(i.t2), big(i.t3), big(i.t4));
    return { offset_us: r.offsetUs, rtt_us: r.rttUs };
  },
  sr04_encode: (i) => {
    if (i.kind === "set_sample_period") return { payload: toHex(packSamplePeriod(num(i.period_us))) };
    if (i.kind === "set_echo_decay") return { payload: toHex(packEchoDecay(num(i.decay_us))) };
    throw new Error(i.kind);
  },
  sr04_decode: (i) => {
    const p = h(i.payload);
    if (i.report === 0x91) {
      const d = unpackSr04Data(p);
      return { source_cmd: d.sourceCmd, timestamp_us: d.timestampUs, echo_time_us: d.echoTimeUs };
    }
    if (i.report === 0x92) return { period_us: unpackSamplePeriod(p) };
    if (i.report === 0x93) return { decay_us: unpackEchoDecay(p) };
    throw new Error(`report ${i.report}`);
  },
  fwdepz_parse: (i) => {
    const img = parseFwDepz(h(i.file));
    return {
      load_addr: img.loadAddr, fw_size: img.fwSize, fw_crc32: img.fwCrc32, cur_sec: img.curSec,
      tot_sec: img.totSec, payload_crc_ok: fwDepzPayloadCrcOk(img),
    };
  },

  // ── BNO055 ──
  bno055_encode: (i) => {
    let p: Uint8Array;
    if (i.kind === "read_reg") p = packBno055ReadReg(num(i.addr), num(i.len));
    else if (i.kind === "write_reg") p = packBno055WriteReg(num(i.addr), h(i.data));
    else if (i.kind === "start_stream") p = packBno055StartStream(num(i.trigger), num(i.addr), num(i.len), num(i.period_ms));
    else if (["reset", "stop_stream", "get_info"].includes(i.kind)) p = new Uint8Array(0);
    else throw new Error(i.kind);
    return { payload: toHex(p) };
  },
  bno055_decode: (i) => bno055Decode(i.report, h(i.payload)),
  bno055_units: (i) => {
    const u = unpackBno055Units(num(i.unit_sel));
    return {
      accel_mg: u.accelMg, gyro_rps: u.gyroRps, euler_rad: u.eulerRad, temp_f: u.tempF, android: u.android,
      repack: packBno055Units(u),
    };
  },
  bno055_calib_stat: (i) => {
    const s = unpackBno055CalibStatus(num(i.value));
    return { ...s, fully_calibrated: bno055FullyCalibrated(s), repack: packBno055CalibStatus(s) };
  },
  bno055_calibration_profile: (i) => {
    const p = unpackBno055CalibrationProfile(h(i.bytes));
    return {
      accel_offset: p.accelOffset, mag_offset: p.magOffset, gyro_offset: p.gyroOffset,
      accel_radius: p.accelRadius, mag_radius: p.magRadius, repack: toHex(packBno055CalibrationProfile(p)),
    };
  },
  bno055_axis_remap: (i) => {
    const a = unpackBno055AxisRemap(num(i.config), num(i.sign));
    return {
      x: a.x, y: a.y, z: a.z, x_negative: a.xNegative, y_negative: a.yNegative, z_negative: a.zNegative,
      repack: packBno055AxisRemap(a),
    };
  },
  bno055_axis_remap_pack: (i) => ({
    repack: packBno055AxisRemap({
      x: num(i.x), y: num(i.y), z: num(i.z), xNegative: i.x_negative, yNegative: i.y_negative, zNegative: i.z_negative,
    }),
  }),
  bno055_sensor_config: (i) => {
    if (i.sensor === "accel") {
      const a = unpackBno055AccelConfig(num(i.value));
      return { ...a, repack: packBno055AccelConfig(a) };
    }
    if (i.sensor === "gyro") {
      const g = unpackBno055GyroConfig(h(i.bytes));
      return { ...g, repack: toHex(packBno055GyroConfig(g)) };
    }
    if (i.sensor === "mag") {
      const m = unpackBno055MagConfig(num(i.value));
      return { ...m, repack: packBno055MagConfig(m) };
    }
    throw new Error(i.sensor);
  },
  bno055_block: (i) => bno055Block(num(i.addr), h(i.data)),

  // ── BNO086 ──
  bno086_shtp_header_pack: (i) => ({
    bytes: toHex(packShtpHeader({ length: num(i.length), channel: num(i.channel), seq: num(i.seq), continuation: i.continuation })),
  }),
  bno086_shtp_header_unpack: (i) => {
    const r = unpackShtpHeader(h(i.bytes));
    return { length: r.length, channel: r.channel, seq: r.seq, continuation: r.continuation };
  },
  bno086_shtp_tx_seq: (i) => {
    const layer = new ShtpLayer();
    return { frames: (i.packets as Any[]).map((p) => toHex(layer.nextFrame(num(p.channel), h(p.payload)))) };
  },
  bno086_shtp_reassembly: (i) => {
    if (i.frames.length !== i.captures_us.length) throw new RangeError("frames / captures_us length");
    const layer = new ShtpLayer();
    const cargos: unknown[] = [];
    (i.frames as string[]).forEach((f, k) => {
      const c = layer.feed(h(f), big(i.captures_us[k]));
      if (c !== null) cargos.push(bno086Cargo(c));
    });
    return { cargos, discarded: layer.discarded };
  },
  bno086_control_encode: (i) => {
    let p: Uint8Array;
    switch (i.kind) {
      case "set_feature":
        p = buildSetFeature(num(i.sensor_id), num(i.interval_us), num(i.batch_us), num(i.sensitivity), num(i.flags), num(i.cfg_word));
        break;
      case "get_feature_request": p = buildGetFeatureRequest(num(i.sensor_id)); break;
      case "product_id_request": p = buildProductIdRequest(); break;
      case "command_request": p = buildCommandRequest(num(i.seq), num(i.command), h(i.params)); break;
      case "frs_read_request": p = buildFrsReadRequest(num(i.frs_type), num(i.offset_words), num(i.block_words)); break;
      case "frs_write_request": p = buildFrsWriteRequest(num(i.frs_type), num(i.length_words)); break;
      case "frs_write_data": p = buildFrsWriteData(num(i.offset_words), (i.words as unknown[]).map(num)); break;
      default: throw new Error(i.kind);
    }
    return { payload: toHex(p) };
  },
  // A control-channel response (contract 05 §4 table) decoded to its fields,
  // or invalid when shorter than them (contract 07 §3).
  bno086_control_decode: (i) => {
    const p = h(i.payload);
    switch (i.kind) {
      case "feature_response": {
        const r = unpackFeatureResponse(p);
        return { sensor_id: r.sensorId, flags: r.flags, sensitivity: r.sensitivity, interval_us: r.intervalUs, batch_us: r.batchUs, cfg_word: r.cfgWord };
      }
      case "product_id": {
        const r = unpackProductId(p);
        return {
          reset_cause: r.resetCause, sw_version_major: r.swVersionMajor, sw_version_minor: r.swVersionMinor,
          sw_part_number: r.swPartNumber, sw_build_number: r.swBuildNumber, sw_version_patch: r.swVersionPatch,
        };
      }
      case "command_response": {
        const r = unpackCommandResponse(p);
        return { seq: r.seq, command: r.command, command_seq: r.commandSeq, response_seq: r.responseSeq, r: r.r };
      }
      case "frs_read_response": {
        const r = unpackFrsReadResponse(p);
        return { status: r.status, data_length: r.dataLength, offset_words: r.offsetWords, data0: r.data0, data1: r.data1, frs_type: r.frsType };
      }
      case "frs_write_response": {
        const r = unpackFrsWriteResponse(p);
        return { status: r.status, offset_words: r.offsetWords };
      }
      default: throw new Error(i.kind);
    }
  },
  bno086_input_cargo: (i) => ({
    reports: parseInputCargo(h(i.cargo), big(i.capture_timestamp_us)).map((r) => ({ type: r.type, fields: bno086Fields(r) })),
  }),
  bno086_record: (i) => ({
    records: parseInputCargo(h(i.cargo), big(i.capture_timestamp_us)).map((r) => bno086RecordValue(r)),
  }),
  bno086_gyro_rv: (i) => {
    const r = parseGyroRvCargo(h(i.cargo), big(i.capture_timestamp_us));
    return r === null ? null : { type: r.type, fields: bno086Fields(r) };
  },

  // ── .depzrec / .depzdata readers (contracts 08, 09) ──
  // `text` as a capture: every rx chunk the loose replay serves once the host
  // has written more tx bytes than any event waits for, concatenated — or
  // invalid for a damaged file.
  depzrec_read: async (i) => {
    const replay = new ReplayTransport(i.text as string);
    await replay.open();
    await replay.write(new Uint8Array(65536));
    const chunks: Uint8Array[] = [];
    if (!replay.exhausted) {
      for await (const chunk of replay.readable()) {
        chunks.push(chunk);
        if (replay.exhausted) break;
      }
    }
    await replay.close();
    return { rx: chunks.map(toHex).join("") };
  },
  // `text` as a dataset: its records in reader order as [d, t, k] — or
  // invalid for a damaged file.
  depzdata_read: (i) => ({
    records: new DatasetReader(i.text as string).records.map((r) => [r.deviceId, r.tHostUs, r.kind]),
  }),

  // ── TMF8829 ──
  tmf8829_encode: (i) => {
    let p: Uint8Array;
    if (i.cmd === Tmf8829Cmd.ReadReg) p = packTmf8829ReadReg(num(i.addr), num(i.len));
    else if (i.cmd === Tmf8829Cmd.WriteReg) p = packTmf8829WriteReg(num(i.addr), h(i.data));
    else if (i.cmd === Tmf8829Cmd.StartStream) p = packTmf8829StartStream(num(i.int_mask), num(i.flags));
    else p = new Uint8Array(0);
    return { payload: toHex(p) };
  },
  tmf8829_decode: (i) => tmf8829Decode(i.report, h(i.payload)),
  tmf8829_reassembly: (i) => tmf8829Reassembly((i.chunks as string[]).map(h)),
  tmf8829_frame: (i) => tmf8829Frame(h(i.raw)),
  tmf8829_measurements: (i) => ({
    measurements: tmf8829Measurements((i.frames as Any[]).map((f) => [h(f.raw), num(f.timestamp_us), num(f.edge_us)])),
  }),
  tmf8829_config_get: (i) => ({ value: tmf8829CfgGet(h(i.page), i.field as Tmf8829CfgField) }),
  tmf8829_config_set: (i) => {
    const pg = h(i.page);
    tmf8829CfgSet(pg, i.field as Tmf8829CfgField, num(i.value));
    return { page: toHex(pg) };
  },
  tmf8829_frame_time: (i) => {
    const clock = new Tmf8829SensorClock();
    const timer = new Tmf8829CaptureTimer(clock);
    const times: unknown[] = [];
    for (const r0 of i.rows as Any[]) {
      const r = Object.fromEntries(Object.entries(r0).map(([k, v]) => [k, num(v)])) as Record<string, number>;
      checkUint("id", r.id!, 8); // the frame-id byte, split here as the SDK's frame parser does
      const kind = r.id! & 0xf0;
      if (kind === 0x30) continue;
      const flags = clock.add(r.ts!, r.systick!);
      if (kind === 0x20) {
        timer.histograms();
        continue;
      }
      const t = timer.results(r.id! & 0x0f, r.systick!, r.t0!, r.t1!, r.ts!, r.edge_us!, flags);
      times.push({ timestamp_us: t.timestampUs, t1_len_us: t.t1LenUs, flags: t.flags, ratio_q16: clock.ratioQ16 });
    }
    return { times };
  },

  // ── VL53L4CD ──
  vl53l4_encode: (i) => {
    let p: Uint8Array;
    if (i.kind === "read_reg") p = l4PackReadReg(num(i.addr), num(i.len));
    else if (i.kind === "write_reg") p = l4PackWriteReg(num(i.addr), h(i.data));
    else if (i.kind === "xshut") p = l4PackXshut(num(i.action));
    else if (i.kind === "start_stream") p = l4PackStartStream(num(i.addr), num(i.len), num(i.flags));
    else if (i.kind === "set_i2c_speed") p = l4PackSetI2cSpeed(num(i.khz));
    else throw new Error(i.kind);
    return { payload: toHex(p) };
  },
  vl53l4_decode: (i) => vl53l4Decode(i.report, h(i.payload)),
  vl53l4_result_block: (i) => vl53l4Result(h(i.raw)),
  vl53l4_timing_encode: (i) => {
    const r = rangeTimingRegisters(num(i.timing_budget_ms), num(i.inter_measurement_ms), num(i.osc_frequency), num(i.clock_pll));
    return { range_config_a: r.rangeConfigA, range_config_b: r.rangeConfigB, intermeasurement_raw: r.intermeasurementRaw };
  },
  vl53l4_timing_decode: (i) => {
    const r = decodeRangeTiming(num(i.intermeasurement_raw), num(i.clock_pll), num(i.osc_frequency), num(i.range_config_a));
    return { timing_budget_ms: r.timingBudgetMs, inter_measurement_ms: r.interMeasurementMs };
  },
  vl53l4_tuning_encode: (i) => ({ raw: L4_TUNING[i.kind]![0](num(i.value)) }),
  vl53l4_tuning_decode: (i) => ({ value: L4_TUNING[i.kind]![1](num(i.raw)) }),

  // ── VL53L5CX / L7 ──
  vl53l7_encode: (i) => {
    let p: Uint8Array;
    if (i.kind === "read_reg") p = packVl53l7ReadReg(num(i.addr), num(i.len));
    else if (i.kind === "write_reg") p = packVl53l7WriteReg(num(i.addr), h(i.data));
    else if (i.kind === "pin_ctrl") p = packPinCtrl(num(i.action));
    else if (i.kind === "set_i2c_speed") p = packVl53l7SetI2cSpeed(num(i.khz));
    else throw new Error(i.kind);
    return { payload: toHex(p) };
  },
  vl53l7_decode: (i) => {
    if (i.report !== 0x92) throw new Error(`report ${i.report}`);
    return vl53l7Info(h(i.payload));
  },
  vl53l7_model: (i) => ({ class: resolveVl53l7Model(i.usb_model, i.device_name) }),

  // ── VL53L8 register bridge (contract 04) ──
  vl53l8_encode: (i) => {
    if (i.kind === "start_stream") return { payload: toHex(packVl53l8StartStream(num(i.frame_size))) };
    throw new Error(i.kind as string);
  },
  vl53l8_decode: (i) => vl53l8Decode(num(i.report), h(i.payload)),

  // ── VL53L 1D family ──
  vl53lx_encode: (i) => {
    let p: Uint8Array;
    if (i.kind === "set_addr_width") p = packVl53lxSetAddrWidth(num(i.width));
    else if (i.kind === "read_reg") p = l4PackReadReg(num(i.addr), num(i.len));
    else if (i.kind === "write_reg") p = l4PackWriteReg(num(i.addr), h(i.data));
    else if (i.kind === "xshut") p = l4PackXshut(num(i.action));
    else if (i.kind === "set_i2c_speed") p = l4PackSetI2cSpeed(num(i.khz));
    else if (i.kind === "start_stream") {
      p = packVl53lxStartStream(num(i.addr), num(i.len),
        (i.clear as unknown[][]).map(([a, v]) => [num(a), num(v)] as const), num(i.flags));
    } else throw new Error(i.kind);
    return { payload: toHex(p) };
  },
  vl53lx_decode: (i) => {
    if (i.report !== 0x92) throw new Error(`report ${i.report}`);
    return vl53lxInfo(h(i.payload));
  },
  vl53lx_model: (i) => ({
    class: resolveVl53lxClass(i.usb_model, i.device_name).name,
    product: registry.productFromBoardName(i.device_name),
  }),
  vl53lx_die_block: (i) => snake({ ...decodeDieBlock(h(i.raw), i.variant) }),
  vl53lx_l0x_raw: (i) => vl53lxL0xRaw(h(i.raw)),
  vl53lx_histogram_raw: (i) => snake({ ...decodeHistogramRaw(h(i.raw)) }),

  // ── VL53L8 advanced + CNH ──
  // Through the ULD driver methods on the fake DCI platform, as the Python
  // adapter does (the methods hold the ULD's own parameter checks).
  vl53l8_motion_config: async (i) => {
    const { drv, p } = makeDriver();
    await drv.motionIndicatorInit(num(i.resolution));
    return { pack: toHex(p.dci.get(DCI_MOTION_DETECTOR_CFG)!) };
  },
  vl53l8_detection_thresholds: async (i) => {
    const t: Partial<DetectionThreshold>[] = (i.thresholds as Any[]).map((x) => ({
      lowThresh: num(x.low_thresh), highThresh: num(x.high_thresh), measurement: num(x.measurement),
      type: num(x.type), zoneNum: num(x.zone_num), operation: num(x.operation),
    }));
    const { drv, p } = makeDriver();
    await drv.setDetectionThresholds(t);
    return {
      start_block: toHex(p.dci.get(DCI_DET_THRESH_START)!),
      valid_status: toHex(p.dci.get(DCI_DET_THRESH_VALID_STATUS)!),
    };
  },
  vl53l8_xtalk_margin: async (i) => {
    const { drv, p } = makeDriver();
    await drv.setXtalkMargin(num(i.kcps));
    const raw = p.dci.get(DCI_XTALK_CFG)!;
    return { raw: new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(0, true) };
  },
  vl53l8_cnh_decode: (i) => {
    const c = i.config;
    const cfg = new CnhConfig();
    cfg.initConfig(num(c.start_bin), num(c.num_bins), num(c.sub_sample));
    const m = (c.agg_map as unknown[]).map(num) as [number, number, number, number, number, number];
    cfg.createAggMap(num(c.resolution), ...m);
    const d = cnhDecode(cfg, h(i.cnh_raw));
    return { aggregates: d.aggregates.map((a) => ({ hist_raw: a.histRaw, hist_scaler: a.histScaler })) };
  },
};

// ── canonical JSON + loop ────────────────────────────────────────────────────

/** Contract 15 §3 canonical JSON: bigint exact, bytes hex, typed arrays as
 *  lists, non-finite floats as "NaN" / "Infinity" / "-Infinity". */
export function canon(v: unknown): string {
  if (v === null || v === undefined) return "null";
  if (typeof v === "bigint") return v.toString();
  if (typeof v === "number") {
    if (Number.isNaN(v)) return '"NaN"';
    if (!Number.isFinite(v)) return v > 0 ? '"Infinity"' : '"-Infinity"';
    if (Object.is(v, -0)) return "0";
    // an integral double beyond 2^53 prints its exact value, not the shortest
    // round-trip form (2869783754553026560, not 2869783754553026600)
    if (Number.isInteger(v) && !Number.isSafeInteger(v)) return BigInt(v).toString();
    return JSON.stringify(v);
  }
  if (typeof v === "boolean" || typeof v === "string") return JSON.stringify(v);
  if (v instanceof Uint8Array) return JSON.stringify(toHex(v));
  if (ArrayBuffer.isView(v)) return canon(Array.from(v as unknown as ArrayLike<number | bigint>));
  if (Array.isArray(v)) return "[" + v.map(canon).join(",") + "]";
  if (typeof v === "object") {
    const parts: string[] = [];
    for (const [k, x] of Object.entries(v as Record<string, unknown>)) {
      if (x === undefined) continue;
      parts.push(JSON.stringify(k) + ":" + canon(x));
    }
    return "{" + parts.join(",") + "}";
  }
  throw new TypeError(`cannot serialize ${typeof v}`);
}

/** Integers beyond 2^53 parse to bigint from their source text (exact). */
function parseLine(line: string): Any {
  return JSON.parse(line, (_k, v, ctx?: { source?: string }) => {
    if (typeof v === "number" && !Number.isSafeInteger(v) && ctx?.source && /^-?\d+$/.test(ctx.source)) {
      return BigInt(ctx.source);
    }
    return v;
  });
}

async function handle(req: Any): Promise<string> {
  const id = canon(req.id);
  const op = OPS[req.op];
  if (req.op in UNSUPPORTED || op === undefined) {
    return `{"id":${id},"error":"unsupported","detail":${JSON.stringify(UNSUPPORTED[req.op] ?? "unknown op")}}`;
  }
  let out: string;
  try {
    out = canon(await op(req.in));
  } catch (e) {
    const detail = `${(e as Error)?.name ?? "Error"}: ${(e as Error)?.message ?? String(e)}`.slice(0, 300);
    return `{"id":${id},"error":"invalid","detail":${JSON.stringify(detail)}}`;
  }
  return `{"id":${id},"out":${out}}`;
}

export { handle };

// One request at a time, answered in order. Only when run as the adapter
// (`bun test/difftest-adapter.ts`); the vector tests import OPS / handle.
if ((import.meta as { main?: boolean }).main) {
  for await (const line of createInterface({ input: process.stdin, crlfDelay: Infinity })) {
    if (line.trim() === "") continue;
    process.stdout.write((await handle(parseLine(line))) + "\n");
  }
}
