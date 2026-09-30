/**
 * @depz/sensor-sdk — browser-safe root export.
 * Node-only pieces live in `@depz/sensor-sdk/node`.
 */

export * from "./protocol/crc.js";
export * from "./protocol/framing.js";
export * from "./protocol/common.js";
export * from "./protocol/identity.js";
export * from "./protocol/sr04.js";
export {
  I2C_ERROR_NAMES as VL53L4_I2C_ERROR_NAMES,
  I2C_KHZ_STEPS as VL53L4_I2C_KHZ_STEPS,
  SF_INT_ACT_HIGH as VL53L4_SF_INT_ACT_HIGH,
  Vl53l4Cmd,
  Vl53l4Rpt,
  XFER_MAX as VL53L4_XFER_MAX,
  XSHUT_OFF as VL53L4_XSHUT_OFF,
  XSHUT_ON as VL53L4_XSHUT_ON,
  XSHUT_RESET as VL53L4_XSHUT_RESET,
  packReadReg as packVl53l4ReadReg,
  packSetI2cSpeed as packVl53l4SetI2cSpeed,
  packStartStream as packVl53l4StartStream,
  packWriteReg as packVl53l4WriteReg,
  packXshut as packVl53l4Xshut,
  unpackInfo as unpackVl53l4Info,
  unpackRegData as unpackVl53l4RegData,
  unpackStream as unpackVl53l4Stream,
} from "./protocol/vl53l4.js";
export type { Vl53l4Info, Vl53l4RegData, Vl53l4StreamData } from "./protocol/vl53l4.js";
export * from "./protocol/vl53l8.js";
export * from "./protocol/bno086.js";
export * from "./protocol/bno055.js";
export * from "./protocol/fwdepz.js";
export * from "./protocol/usb-ids.js";
export * from "./errors.js";
export {
  isDeviceInfo,
  listDepzDevicesFrom,
  openDeviceByDeviceSerial,
  openDeviceFrom,
  orderDevicesBySerial,
  orderPortsBySerial,
  probePort,
} from "./discovery.js";
export type {
  DeviceInfo,
  DepzPortInfo,
  ListOptions,
  OpenDeviceOptions,
  OpenTarget,
  TransportFactory,
} from "./discovery.js";
export { DepzLink } from "./device/link.js";
export type { LinkStats } from "./device/link.js";
export {
  DEFAULT_TIMEOUT_MS,
  DepzDevice,
  NO_MATCH,
  StreamQueue,
  hostNowUs,
  syncTimeAll,
} from "./device/device.js";
export type {
  DeviceEvent,
  DeviceOptions,
  Matcher,
  RequestOptions,
  TimeSync,
} from "./device/device.js";
export { Sr04 } from "./sensors/sr04.js";
export type { Sr04Measurement } from "./sensors/sr04.js";
export * from "./sensors/vl53l4/uld.js";
export * from "./sensors/vl53l4/vl53l4.js";
export {
  MIN_RANGING_FREQUENCY_HZ,
  Vl53l8,
  Vl53l8ch,
  Vl53l8cx,
  zoneGrid,
} from "./sensors/vl53l8/vl53l8.js";
export type {
  Vl53l8Frame,
  Vl53l8InitOptions,
  Vl53l8Options,
} from "./sensors/vl53l8/vl53l8.js";
export * from "./protocol/vl53l7.js";
export * from "./protocol/vl53lx.js";
export {
  VL53L7_MIN_RANGING_FREQUENCY_HZ,
  Vl53l5cx,
  Vl53l7ch,
  Vl53l7cx,
} from "./sensors/vl53l7/vl53l7.js";
export type { Vl53l7Frame } from "./sensors/vl53l7/vl53l7.js";
export {
  PLOTTABLE_STATUSES,
  VL53LX_CLASS_BY_PRODUCT,
  VL53LX_DRIVER_KINDS,
  VL53LX_PRODUCTS,
  Vl53l0x,
  Vl53l1cb,
  Vl53l1cx,
  Vl53l3cx,
  Vl53l4cx,
  Vl53lx,
  plotDistances,
  primaryTarget,
  resolveVl53lxClass,
} from "./sensors/vl53lx/vl53lx.js";
export type {
  HistogramDriverApi,
  Vl53lxConfigureOptions,
  Vl53lxMeasurement,
  Vl53lxOptions,
  Vl53lxProductInfo,
} from "./sensors/vl53lx/vl53lx.js";
export {
  BridgePlatform as Vl53lxBridgePlatform,
  SensorDriver as Vl53lxSensorDriver,
} from "./sensors/vl53lx/uld/base.js";
export type {
  Measurement as Vl53lxDriverMeasurement,
  Target as Vl53lxTarget,
} from "./sensors/vl53lx/uld/base.js";
export {
  ProtocolError as Vl53lxProtocolError,
  Vl53Error as Vl53lxError,
} from "./sensors/vl53lx/uld/link.js";
export type { BridgeDevice as Vl53lxBridgeDevice } from "./sensors/vl53lx/uld/link.js";
export {
  DIE_BLOCK_ADDR as VL53LX_DIE_BLOCK_ADDR,
  DIE_BLOCK_LEN as VL53LX_DIE_BLOCK_LEN,
  DIE_VARIANTS as VL53LX_DIE_VARIANTS,
  HISTOGRAM_BINS as VL53LX_HISTOGRAM_BINS,
  HISTOGRAM_BLOCK_ADDR as VL53LX_HISTOGRAM_BLOCK_ADDR,
  HISTOGRAM_BLOCK_LEN as VL53LX_HISTOGRAM_BLOCK_LEN,
  L0X_BLOCK_ADDR as VL53LX_L0X_BLOCK_ADDR,
  L0X_BLOCK_LEN as VL53LX_L0X_BLOCK_LEN,
  decodeDieBlock as decodeVl53lxDieBlock,
  decodeHistogramRaw as decodeVl53lxHistogramRaw,
  decodeL0xRaw as decodeVl53lxL0xRaw,
} from "./sensors/vl53lx/decode.js";
export type {
  DieResult as Vl53lxDieResult,
  HistogramRaw as Vl53lxHistogramRaw,
  L0xRaw as Vl53lxL0xRaw,
} from "./sensors/vl53lx/decode.js";
export { VL53L0X } from "./sensors/vl53lx/uld/l0x.js";
export { VL53L1 } from "./sensors/vl53lx/uld/l1.js";
export { VL53L3 } from "./sensors/vl53lx/uld/l3.js";
export { VL53L4 } from "./sensors/vl53lx/uld/l4.js";
export {
  driverFor as vl53lxDriverFor,
  driverKinds as vl53lxDriverKinds,
  productFromBoardName as vl53lxProductFromBoardName,
} from "./sensors/vl53lx/uld/registry.js";
export {
  CALIBRATE_XTALK,
  FW_CHECKSUM,
  GET_XTALK_CMD,
  I2C_VARIANTS,
  MODULE_TYPE_MZ,
  MODULE_TYPE_MZEVO,
  MODULE_TYPE_MZPLUS,
  MODULE_TYPE_NAMES,
  MotionConfig,
  NB_THRESHOLDS,
  POWER_MODE_DEEP_SLEEP,
  POWER_MODE_SLEEP,
  POWER_MODE_WAKEUP,
  RANGING_MODE_AUTONOMOUS,
  RANGING_MODE_CONTINUOUS,
  RESOLUTION_4X4,
  RESOLUTION_8X8,
  TARGET_ORDER_CLOSEST,
  TARGET_ORDER_STRONGEST,
  THRESH_IN_WINDOW,
  THRESH_OP_AND,
  THRESH_OP_NONE,
  THRESH_OP_OR,
  THRESH_OUT_OF_WINDOW,
  VL53L8CX,
  Vl53l8cxError,
  defaultMotionConfig,
  motionConfigSetResolution,
  packDetectionThresholds,
  swapBuffer,
  xtalkMarginToRaw,
} from "./sensors/vl53l8/uld.js";
export type {
  DetectionThreshold,
  MotionResult,
  Vl53l8Platform,
  Vl53l8Results,
} from "./sensors/vl53l8/uld.js";
export {
  CNH_BIN_WIDTH_MM,
  CNH_MAX_DATA_BYTES,
  CnhConfig,
  CnhConfigError,
  MI_CFG_DEV_IDX,
  decode as decodeCnh,
  maxBins as cnhMaxBins,
} from "./sensors/vl53l8/cnh.js";
export type { CnhAggregate, CnhDecoded } from "./sensors/vl53l8/cnh.js";
export { loadAssets } from "./sensors/vl53l8/assets/index.js";
export type { Vl53l8Assets, Vl53l8Variant } from "./sensors/vl53l8/assets/index.js";
export * from "./sensors/bno086/shtp.js";
export * from "./sensors/bno086/reports.js";
export * from "./sensors/bno086/sh2.js";
export * from "./sensors/bno086/bno086.js";
export * from "./sensors/bno055/regs.js";
export * from "./sensors/bno055/bno055.js";
export { RECORDING_SCHEMA, RecordingTransport, ReplayTransport } from "./transport/replay.js";
export {
  DATASET_SCHEMA,
  DatasetPlayer,
  DatasetReader,
  DatasetRecorder,
  DatasetWriter,
} from "./dataset.js";
export type {
  DatasetDeviceMeta,
  DatasetHeader,
  DatasetRecord,
  PlayerState,
} from "./dataset.js";
export type { SerialTransport, SerialTransportInfo } from "./transport/types.js";
export { LoopbackTransport } from "./transport/loopback.js";
export {
  WebSerialTransport,
  isWebSerialSupported,
  getGrantedPorts,
  watchConnect,
} from "./transport/webserial.js";
export type { WebSerialPortLike } from "./transport/webserial.js";
export {
  WsBackendTransport,
  backendHealth,
  backendPermissions,
  listBackendDevices,
} from "./transport/ws-backend.js";
export type {
  BackendDevice,
  BackendHealth,
  BackendPermissions,
  WebSocketLike,
  WsBackendTransportOptions,
} from "./transport/ws-backend.js";
