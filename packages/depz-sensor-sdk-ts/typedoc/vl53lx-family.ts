/**
 * Shared barrel for the VL53L 1D-family TypeDoc entry points
 * (`entry-vl53l0x.ts` … `entry-vl53l4cx.ts`): the family class `Vl53lx`, the
 * measurement shape, the helpers and the bridge codecs, under the names
 * `src/index.ts` exports them with. Every product page carries this same
 * family surface after its own class.
 *
 * Not part of the shipped package — a doc-generation helper only.
 */

export {
  PLOTTABLE_STATUSES,
  VL53LX_CLASS_BY_PRODUCT,
  VL53LX_DRIVER_KINDS,
  VL53LX_PRODUCTS,
  Vl53lx,
  plotDistances,
  primaryTarget,
  resolveVl53lxClass,
} from "../src/sensors/vl53lx/vl53lx.js";
export type {
  HistogramDriverApi,
  Vl53lxConfigureOptions,
  Vl53lxMeasurement,
  Vl53lxOptions,
  Vl53lxProductInfo,
} from "../src/sensors/vl53lx/vl53lx.js";
export { SensorDriver as Vl53lxSensorDriver } from "../src/sensors/vl53lx/uld/base.js";
export type {
  Measurement as Vl53lxDriverMeasurement,
  Target as Vl53lxTarget,
} from "../src/sensors/vl53lx/uld/base.js";
export {
  ProtocolError as Vl53lxProtocolError,
  Vl53Error as Vl53lxError,
} from "../src/sensors/vl53lx/uld/link.js";
export * from "../src/protocol/vl53lx.js";
export {
  XSHUT_OFF as VL53L4_XSHUT_OFF,
  XSHUT_ON as VL53L4_XSHUT_ON,
  XSHUT_RESET as VL53L4_XSHUT_RESET,
} from "../src/protocol/vl53l4.js";
export { WINDOW_ABOVE, WINDOW_BELOW, WINDOW_IN, WINDOW_OUT } from "../src/sensors/vl53l4/uld.js";
