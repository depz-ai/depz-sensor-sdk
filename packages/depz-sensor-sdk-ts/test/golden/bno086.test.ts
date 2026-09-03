/**
 * BNO086 golden vectors: contracts/vectors/bno086_shtp.json (SHTP framing +
 * SH-2 control encodes) and contracts/vectors/bno086_reports.json (input
 * report decoding + timestamp math).
 */

import { describe, expect, it } from "vitest";
import {
  GYRO_RV_ANGVEL_Q,
  Q_POINTS,
  RV_ACCURACY_Q,
  ShtpLayer,
  buildCommandRequest,
  buildFrsReadRequest,
  buildFrsWriteData,
  buildFrsWriteRequest,
  buildGetFeatureRequest,
  buildProductIdRequest,
  buildSetFeature,
  packShtpHeader,
  parseGyroRvCargo,
  parseInputCargo,
  unpackShtpHeader,
  type Report,
  type ShtpCargo,
} from "../../src/index.js";
import { fromHex, loadVectors, toHex } from "./vectors.js";

const shtpData = loadVectors("bno086_shtp.json");
const reportsData = loadVectors("bno086_reports.json");

describe("bno086 shtp header vectors", () => {
  for (const c of shtpData.header) {
    it(`${c.name} pack`, () => {
      const packed = packShtpHeader({
        length: c.length,
        channel: c.channel,
        seq: c.seq,
        continuation: c.continuation,
      });
      expect(toHex(packed)).toBe(c.bytes);
    });
    it(`${c.name} unpack`, () => {
      const hdr = unpackShtpHeader(fromHex(c.bytes));
      expect(hdr.length).toBe(c.length);
      expect(hdr.channel).toBe(c.channel);
      expect(hdr.seq).toBe(c.seq);
      expect(hdr.continuation).toBe(c.continuation);
    });
  }
});

describe("bno086 shtp tx_seq vectors", () => {
  it("per-channel TX sequence counters", () => {
    const layer = new ShtpLayer();
    for (const c of shtpData.tx_seq) {
      expect(toHex(layer.nextFrame(c.channel, fromHex(c.payload)))).toBe(c.frame);
    }
  });
});

describe("bno086 shtp reassembly vectors", () => {
  for (const c of shtpData.reassembly) {
    it(c.name, () => {
      const layer = new ShtpLayer();
      const cargos: ShtpCargo[] = [];
      for (const frame of c.frames) {
        const cargo = layer.feed(fromHex(frame));
        if (cargo !== null) cargos.push(cargo);
      }
      expect(cargos.length).toBe(c.expect.cargos.length);
      c.expect.cargos.forEach((exp: any, i: number) => {
        expect(cargos[i]!.channel).toBe(exp.channel);
        expect(cargos[i]!.seq).toBe(exp.seq);
        expect(toHex(cargos[i]!.payload)).toBe(exp.payload);
      });
      expect(layer.discarded).toBe(c.expect.discarded);
    });
  }
});

describe("bno086 sh2 control encode vectors", () => {
  for (const c of shtpData.control_encode) {
    it(c.name, () => {
      let encoded: Uint8Array;
      if (c.kind === "set_feature") {
        encoded = buildSetFeature(
          c.sensor_id,
          c.interval_us,
          c.batch_us,
          c.sensitivity,
          c.flags,
          c.cfg_word,
        );
      } else if (c.kind === "get_feature_request") {
        encoded = buildGetFeatureRequest(c.sensor_id);
      } else if (c.kind === "product_id_request") {
        encoded = buildProductIdRequest();
      } else if (c.kind === "command_request") {
        encoded = buildCommandRequest(c.seq, c.command, fromHex(c.params));
      } else if (c.kind === "frs_read_request") {
        encoded = buildFrsReadRequest(c.frs_type, c.offset_words, c.block_words);
      } else if (c.kind === "frs_write_request") {
        encoded = buildFrsWriteRequest(c.frs_type, c.length_words);
      } else if (c.kind === "frs_write_data") {
        encoded = buildFrsWriteData(c.offset_words, c.words);
      } else {
        throw new Error(c.kind);
      }
      expect(toHex(encoded)).toBe(c.payload);
    });
  }
});

describe("bno086 report tables", () => {
  it("q_points match the vector table", () => {
    for (const [key, q] of Object.entries(reportsData.q_points)) {
      expect(Q_POINTS[parseInt(key, 16)]).toBe(q);
    }
    expect(RV_ACCURACY_Q).toBe(reportsData.rv_accuracy_q);
    expect(GYRO_RV_ANGVEL_Q).toBe(reportsData.gyro_rv_angvel_q);
  });
});

function snakeToCamel(name: string): string {
  return name.replace(/_([a-z0-9])/g, (_, ch: string) => ch.toUpperCase());
}

function checkFields(rep: Report, fields: Record<string, unknown>): void {
  const r = rep as unknown as Record<string, unknown>;
  for (const [key, exp] of Object.entries(fields)) {
    const actual = r[snakeToCamel(key)];
    if (key === "timestamp_us") {
      expect(actual).toBe(BigInt(exp as number));
    } else if (key === "data") {
      expect(toHex(actual as Uint8Array)).toBe(exp);
    } else if (key === "end_of_sequence") {
      expect(actual).toBe(Boolean(exp));
    } else if (Array.isArray(exp)) {
      expect(actual).toEqual(exp);
    } else {
      expect(actual).toBe(exp);
    }
  }
}

/** Scaled values follow raw / 2**Q exactly (raw ints are authoritative). */
function checkScaled(rep: Report, fields: Record<string, number | null>): void {
  const q = Q_POINTS[rep.sensorId];
  switch (rep.type) {
    case "Acceleration":
    case "Gyroscope":
    case "Magnetometer":
      expect(rep.x).toBe(fields.x_raw! / 2 ** q!);
      expect(rep.y).toBe(fields.y_raw! / 2 ** q!);
      expect(rep.z).toBe(fields.z_raw! / 2 ** q!);
      break;
    case "UncalibratedGyroscope":
    case "UncalibratedMagnetometer":
      expect(rep.x).toBe(fields.x_raw! / 2 ** q!);
      expect(rep.bias).toEqual([
        fields.bias_x_raw! / 2 ** q!,
        fields.bias_y_raw! / 2 ** q!,
        fields.bias_z_raw! / 2 ** q!,
      ]);
      break;
    case "RotationVector":
      expect(rep.i).toBe(fields.i_raw! / 2 ** 14);
      expect(rep.real).toBe(fields.real_raw! / 2 ** 14);
      expect(rep.accuracyRad).toBe(
        fields.accuracy_raw == null ? null : fields.accuracy_raw / 2 ** RV_ACCURACY_Q,
      );
      break;
    case "ScalarReport":
      expect(rep.value).toBe(fields.value_raw! / 2 ** q!);
      break;
    case "GyroIntegratedRV":
      expect(rep.real).toBe(fields.real_raw! / 2 ** 14);
      expect(rep.angularVelocity).toEqual([
        fields.vx_raw! / 2 ** GYRO_RV_ANGVEL_Q,
        fields.vy_raw! / 2 ** GYRO_RV_ANGVEL_Q,
        fields.vz_raw! / 2 ** GYRO_RV_ANGVEL_Q,
      ]);
      break;
  }
}

describe("bno086 input cargo vectors", () => {
  for (const c of reportsData.input_cargos) {
    it(c.name, () => {
      const reports = parseInputCargo(fromHex(c.cargo), BigInt(c.capture_timestamp_us));
      expect(reports.length).toBe(c.expect.length);
      c.expect.forEach((exp: any, i: number) => {
        const rep = reports[i]!;
        expect(rep.type).toBe(exp.type);
        checkFields(rep, exp.fields);
        checkScaled(rep, exp.fields);
      });
    });
  }
});

describe("bno086 gyro-integrated RV vectors", () => {
  for (const c of reportsData.gyro_rv) {
    it(c.name, () => {
      const rep = parseGyroRvCargo(fromHex(c.cargo), BigInt(c.capture_timestamp_us));
      expect(rep).not.toBeNull();
      expect(rep!.type).toBe(c.expect.type);
      checkFields(rep!, c.expect.fields);
      checkScaled(rep!, c.expect.fields);
    });
  }
});
