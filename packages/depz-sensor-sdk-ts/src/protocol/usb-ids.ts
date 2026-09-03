/**
 * USB identity table for the DEPZ sensor line (contract 02 §4).
 *
 * VID/PID are *hints only* — the protocol probe (GET_NAME_ACTIVE_SOFTWARE)
 * remains the source of truth for what a device actually is. This table lets
 * discovery pick candidate ports fast and avoid poking unrelated hardware.
 *
 * To add a model: add one row to `DEPZ_USB_MODELS` (and, if it falls outside
 * the recognized ranges below, teach `isKnownDepzUsb`). Mirrors the Python
 * reference `depz_sensor_sdk.usb_ids`.
 */

import type { SensorType } from "./identity.js";

/** Production VID shared by all programmed DEPZ sensors. */
export const DEPZ_VID = 0x1bcf; // 7119

/**
 * Dev / unprogrammed default (STMicroelectronics Virtual COM Port). Dev units
 * before product-metadata flashing enumerate here; treated as a candidate.
 */
export const DEV_VID = 0x0483; // 1155
export const DEV_PID = 0x56dc; // 22236

/**
 * Contiguous production PID block: any PID in this inclusive range under the
 * production VID is treated as a candidate DEPZ sensor even when it is not
 * individually mapped below (e.g. future models, or unmapped catalog PIDs).
 */
export const DEPZ_SENSOR_PID_MIN = 60536; // 0xEC78 (SR04)
export const DEPZ_SENSOR_PID_MAX = 65535; // whole reserved sensor block (catalog-types.ts)

/** A known DEPZ USB PID and what it is. */
export interface DepzUsbModel {
  /** Human-readable model name. */
  name: string;
  /**
   * Sensor class this SDK can decode, or null for a DEPZ sensor that is
   * recognized but not (yet) driven by this SDK (still opens as a base
   * device / warned about).
   */
  sensorType: SensorType | null;
}

/**
 * PID → model hint (full official DEPZ catalog). Only the `sensorType`-bearing
 * rows (sr04, vl53l4cd, vl53l8ch, vl53l8cx, bno086) are decodable by this SDK; the rest
 * are recognized as DEPZ sensors for discovery/labelling only. Values mirror
 * the Python reference `depz_sensor_sdk.usb_ids.DEPZ_PID_MODEL`.
 */
export const DEPZ_USB_MODELS: Readonly<Record<number, DepzUsbModel>> = {
  0xec78: { name: "sr04", sensorType: "sr04" }, // 60536
  0xed40: { name: "vl53l8ch", sensorType: "vl53l8" }, // 60736
  0xed41: { name: "vl53l0x", sensorType: null }, // 60737
  0xed42: { name: "vl53l1cb", sensorType: null }, // 60738
  0xed43: { name: "vl53l1cx", sensorType: null }, // 60739
  0xed44: { name: "vl53l3cx", sensorType: null }, // 60740
  0xed45: { name: "vl53l4cd", sensorType: "vl53l4" }, // 60741
  0xed46: { name: "vl53l4cx", sensorType: null }, // 60742
  0xed47: { name: "vl53l4ed", sensorType: null }, // 60743
  0xed48: { name: "vl53l5cx", sensorType: null }, // 60744
  0xed49: { name: "vl53l7cx", sensorType: null }, // 60745
  0xed4a: { name: "vl53l7ch", sensorType: null }, // 60746
  0xed4b: { name: "vl53l8cx", sensorType: "vl53l8" }, // 60747 — VL53L8CX production PID, verified on hw: 1bcf:ed4b
  0xee08: { name: "bno086", sensorType: "bno086" }, // 60936 — verified on hw: 1bcf:ee08
  0xee09: { name: "bno085", sensorType: null }, // 60937
  0xee0a: { name: "bno055", sensorType: null }, // 60938
};

/**
 * True when `(vid, pid)` looks like a DEPZ sensor: the production VID with a
 * mapped PID or a PID in the recognized block, or the dev default. A match
 * means "worth probing", never "definitely this model".
 */
export function isKnownDepzUsb(vid: number | null | undefined, pid: number | null | undefined): boolean {
  if (vid == null || pid == null) return false;
  if (vid === DEV_VID && pid === DEV_PID) return true;
  if (vid !== DEPZ_VID) return false;
  if (pid in DEPZ_USB_MODELS) return true;
  return pid >= DEPZ_SENSOR_PID_MIN && pid <= DEPZ_SENSOR_PID_MAX;
}

/** Model hint for a PID, or null when unrecognized (informational only). */
export function pidToModel(pid: number | null | undefined): DepzUsbModel | null {
  if (pid == null) return null;
  return DEPZ_USB_MODELS[pid] ?? null;
}

/**
 * Best-guess model name for a `(vid, pid)`, or null. Informational only —
 * never used to decide how to decode a device; the firmware-name probe does
 * that. Mirrors the Python reference `usb_model_hint`.
 */
export function usbModelHint(
  vid: number | null | undefined,
  pid: number | null | undefined,
): string | null {
  if (vid == null || pid == null) return null;
  if (vid === DEV_VID && pid === DEV_PID) return "dev";
  if (vid === DEPZ_VID) return DEPZ_USB_MODELS[pid]?.name ?? null;
  return null;
}
