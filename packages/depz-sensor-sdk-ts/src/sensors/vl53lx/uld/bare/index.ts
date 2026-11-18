/**
 * uld/bare — ST's VL53LX Bare Driver on the host (histogram mode), for
 * VL53L1CX / L1CB / L3CX / L4CX. Mirror of the Python
 * `depz_sensor_sdk.vl53lx.uld.bare` package:
 *
 *   regs.ts     register blocks and their codec (generated table)
 *   image.ts    DeviceImage — the blocks that make up the device state
 *   nvm.ts      NvmReader — the factory calibration outside the registers
 *   tuning.ts   ST's tuning parameters (generated table)
 *   core.ts     BareDriver — presets, timing maths, ranging cycle, the frame
 *   hist.ts     post-processing — 24 bins into targets, host only
 *   driver.ts   VL53LX — the SensorDriver the device class talks to
 *   imath.ts    Python integer semantics on JS numbers
 */

export { DeviceImage } from "./image.js";
export { NvmReader } from "./nvm.js";
export { BLOCKS, RegBlock } from "./regs.js";
export { TUNING, DEFAULTS } from "./tuning.js";
export { BareDriver, HistogramBinData } from "./core.js";
export { processData, toMeasurement, FrameHistory } from "./hist.js";
export { VL53LX } from "./driver.js";
