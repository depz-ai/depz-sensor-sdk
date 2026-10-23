/**
 * TMF8829 presets and config-page fields (contracts/14_SENSOR_TMF8829.md §4).
 * Mirrors the Python reference `depz_sensor_sdk.tmf8829.config`.
 */

/** Config page 0x22..0xDF. */
export const TMF8829_REG_CFG = 0x22;
export const TMF8829_CFG_SIZE = 0xdf - TMF8829_REG_CFG + 1;

export type Tmf8829Preset =
  | "8x8"
  | "8x8-lr"
  | "8x8-ha"
  | "16x16"
  | "16x16-ha"
  | "32x32"
  | "32x32-ha"
  | "48x32"
  | "48x32-ha"
  | "8x8-xr";

/** Preset → load command (each loads its config page). */
export const TMF8829_PRESETS: Readonly<Record<Tmf8829Preset, number>> = {
  "8x8": 0x40,
  "8x8-lr": 0x41, // long range
  "8x8-ha": 0x42, // high accuracy
  "16x16": 0x43,
  "16x16-ha": 0x44,
  "32x32": 0x45,
  "32x32-ha": 0x46,
  "48x32": 0x47,
  "48x32-ha": 0x48,
  "8x8-xr": 0x49, // extended range, distances in mm
};

/** The preset's own kilo_iterations (a preset rescales a stale value, it does not reset it). */
export const TMF8829_PRESET_KILO_ITERATIONS: Readonly<Record<Tmf8829Preset, number>> = {
  "8x8": 586,
  "8x8-lr": 586,
  "8x8-ha": 586,
  "8x8-xr": 586,
  "16x16": 586,
  "16x16-ha": 586,
  "32x32": 2344,
  "32x32-ha": 2344,
  "48x32": 3516,
  "48x32-ha": 3516,
};

/** The sensor's own period after init(). */
export const TMF8829_DEFAULT_PERIOD_MS = 33;

export type Tmf8829CfgField =
  | "period_ms"
  | "kilo_iterations"
  | "fp_mode"
  | "nr_peaks"
  | "signal_strength"
  | "noise_strength"
  | "xtalk"
  | "dump_histograms"
  | "ref_spad_frame"
  | "power_modes"
  | "ha_kilo_iterations"
  | "dual_mode"
  | "fov_correction"
  | "distance_in_mm";

/** Field → [register, bit shift, width in bits]; width 16 = little-endian register pair. */
export const TMF8829_CFG_FIELDS: Readonly<Record<Tmf8829CfgField, readonly [number, number, number]>> = {
  period_ms: [0x22, 0, 16],
  kilo_iterations: [0x24, 0, 16],
  fp_mode: [0x26, 0, 3],
  nr_peaks: [0x2a, 0, 3],
  signal_strength: [0x2a, 3, 1],
  noise_strength: [0x2a, 4, 1],
  xtalk: [0x2a, 5, 1],
  dump_histograms: [0x2b, 0, 1],
  ref_spad_frame: [0x2c, 0, 1],
  power_modes: [0x2e, 0, 8],
  ha_kilo_iterations: [0x4a, 0, 16],
  dual_mode: [0x4c, 0, 2],
  fov_correction: [0x78, 0, 4],
  distance_in_mm: [0x8f, 0, 1],
};

/** Index of `name`'s first byte in `page`; RangeError when the page is too short to hold it. */
function fieldIndex(page: Uint8Array, name: Tmf8829CfgField, reg: number, width: number): number {
  const i = reg - TMF8829_REG_CFG;
  const end = i + (width === 16 ? 2 : 1);
  if (page.length < end) throw new RangeError(`${name}: config page has ${page.length} bytes, field needs ${end}`);
  return i;
}

export function tmf8829CfgGet(page: Uint8Array, name: Tmf8829CfgField): number {
  const [reg, shift, width] = TMF8829_CFG_FIELDS[name];
  const i = fieldIndex(page, name, reg, width);
  if (width === 16) return page[i]! | (page[i + 1]! << 8);
  return (page[i]! >> shift) & ((1 << width) - 1);
}

export function tmf8829CfgSet(page: Uint8Array, name: Tmf8829CfgField, value: number): void {
  const [reg, shift, width] = TMF8829_CFG_FIELDS[name];
  const i = fieldIndex(page, name, reg, width);
  if (!Number.isInteger(value) || value < 0 || value >= 2 ** width) {
    throw new RangeError(`${name}: ${value} out of range for ${width} bit`);
  }
  if (width === 16) {
    page[i] = value & 0xff;
    page[i + 1] = value >> 8;
  } else {
    const mask = ((1 << width) - 1) << shift;
    page[i] = (page[i]! & ~mask) | (value << shift);
  }
}

/** Every named field of a config page. */
export function tmf8829ConfigFields(page: Uint8Array): Record<Tmf8829CfgField, number> {
  const out = {} as Record<Tmf8829CfgField, number>;
  for (const name of Object.keys(TMF8829_CFG_FIELDS) as Tmf8829CfgField[]) out[name] = tmf8829CfgGet(page, name);
  return out;
}
