/** Firmware-name parsing (contracts/02_COMMON_COMMANDS.md §4). */

export type SensorType = "sr04" | "vl53l4" | "vl53l8" | "bno086" | "unknown";

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
  ["BNO086", "bno086"],
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
