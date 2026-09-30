/** Firmware-name parsing (contracts/02_COMMON_COMMANDS.md §4). */

export type SensorType =
  | "sr04"
  | "vl53l4"
  | "vl53l8"
  | "vl53l7"
  | "vl53lx"
  | "bno086"
  | "bno055"
  | "unknown";

export interface Identity {
  mode: "app" | "bootloader" | "unknown";
  sensorType: SensorType | null; // null in bootloader mode
  softwareName: string;
  version: string; // "" when not parseable
}

const VERSION_RE = /_v(\d+(?:\.\d+)*)$/;

const PRODUCT_TOKENS: Array<[string, SensorType]> = [
  ["SR04", "sr04"],
  ["VL53L4", "vl53l4"],
  ["VL53L8", "vl53l8"],
  ["VL53L7", "vl53l7"], // VL53L5CX / VL53L7CX / VL53L7CH board (contract 11)
  // The 1D-family bridge (contract 12): boards answer APP_VL53L0_4_v*, the
  // protocol spec calls it APP_VL53LX_v*.
  ["VL53L0_4", "vl53lx"],
  ["VL53LX", "vl53lx"],
  ["BNO086", "bno086"],
  ["BNO055", "bno055"], // register bridge, fusion on chip (contract 13)
];

/**
 * Classify a GET_NAME_ACTIVE_SOFTWARE string. The string must already be
 * stripped of trailing NUL/0xFF (`stripDeviceString`).
 */
export function parseSoftwareName(name: string): Identity {
  const m = VERSION_RE.exec(name);
  const version = m ? m[1]! : "";
  if (name.startsWith("BOOTDEPZ")) {
    return { mode: "bootloader", sensorType: null, softwareName: name, version };
  }
  if (name.startsWith("APP_")) {
    for (const [token, sensor] of PRODUCT_TOKENS) {
      if (name.includes(token)) {
        return { mode: "app", sensorType: sensor, softwareName: name, version };
      }
    }
    return { mode: "app", sensorType: "unknown", softwareName: name, version };
  }
  return { mode: "unknown", sensorType: null, softwareName: name, version };
}
