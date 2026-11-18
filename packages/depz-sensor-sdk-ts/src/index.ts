/**
 * @depz/sensor-sdk — browser-safe root export.
 * Node-only pieces live in `@depz/sensor-sdk/node`.
 *
 * Every export here is named: what is public is a decision, not an accident.
 * Public are the device classes with the types their methods take and return,
 * the errors, discovery / transport / dataset, and the DEPZ wire protocol
 * (`protocol/*`: opcodes, codecs and the limits the contracts fix) — the
 * surface `DepzDevice.request()` is written against. Not public are the chip
 * drivers underneath (the ST ULD ports, the SH-2 / SHTP codecs, the BNO055
 * and TMF8829 register-level helpers): they follow the vendor's releases and
 * are reachable only through the documented `uld` / `driver` escape hatches,
 * which are unstable — not covered by the 1.0 freeze.
 */

// ── protocol: framing and the common commands (contracts 01, 02) ─────────────
export { crc16CcittFalse, crc16Modbus, crc32IsoHdlc, crc8Maxim } from "./protocol/crc.js";
export {
  CrcType,
  HEADER_SIZE,
  MAGIC0,
  MAGIC1,
  MAX_PAYLOAD,
  PacketParser,
  buildPacket,
  payloadCrcBytes,
} from "./protocol/framing.js";
export type { CrcErrorEvent, PacketEvent, ParserEvent, TrashEvent } from "./protocol/framing.js";
export {
  Cmd,
  PAYLOAD_CRC_TYPE_MAX,
  Rpt,
  SYNC_PIN_MAX,
  SYNC_PIN_MIN,
  Status,
  SyncPinMode,
  SyncPinPolarity,
  UNSOLICITED,
  checkSyncPin,
  packSetPayloadCrcType,
  packSyncPinConfig,
  packSyncTime,
  stripDeviceString,
  syncTimeOffsetRtt,
  unpackPayloadCrcType,
  unpackSequenceError,
  unpackStatus,
  unpackSyncPinConfig,
  unpackSyncTime,
  unpackTemperature,
  unpackText,
} from "./protocol/common.js";
export type {
  SequenceErrorReport,
  StatusReport,
  SyncPinConfig,
  SyncTimeReport,
  TemperatureReport,
  TextReport,
} from "./protocol/common.js";
export { parseSoftwareName } from "./protocol/identity.js";
export type { Identity, SensorType } from "./protocol/identity.js";
export {
  DEPZ_SENSOR_PID_MAX,
  DEPZ_SENSOR_PID_MIN,
  DEPZ_USB_MODELS,
  DEPZ_VID,
  DEV_PID,
  DEV_VID,
  isKnownDepzUsb,
  pidToModel,
  usbModelHint,
} from "./protocol/usb-ids.js";
export type { DepzUsbModel } from "./protocol/usb-ids.js";
export {
  FWDEPZ_HEADER_SIZE,
  FWDEPZ_MAGIC,
  FwDepzError,
  fwDepzPayloadCrcOk,
  parseFwDepz,
} from "./protocol/fwdepz.js";
export type { FwDepzImage } from "./protocol/fwdepz.js";

// ── protocol: the per-sensor commands and reports (contracts 03–06, 10–14) ───
export {
  ECHO_DECAY_DEFAULT_US,
  ECHO_DECAY_MAX_US,
  ECHO_DECAY_MIN_US,
  ECHO_TIMEOUT,
  SAMPLE_PERIOD_DEFAULT_US,
  Sr04Cmd,
  Sr04Rpt,
  distanceMmFromEcho,
  packEchoDecay,
  packSamplePeriod,
  unpackEchoDecay,
  unpackSamplePeriod,
  unpackSr04Data,
} from "./protocol/sr04.js";
export type { Sr04Data } from "./protocol/sr04.js";
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
export {
  CHUNK_SIZE,
  FrameReassembler,
  READ_MAX_LEN,
  STREAM_CHUNK_MAX,
  STREAM_TOTAL_MAX,
  Vl53l8Cmd,
  Vl53l8Rpt,
  packReadReg,
  packStartStream,
  packWriteReg,
  unpackFrameChunk,
  unpackRegData,
} from "./protocol/vl53l8.js";
export type { FrameChunk, RegData } from "./protocol/vl53l8.js";
export {
  PinAction,
  VL53L7_I2C_SPEED_STEPS_KHZ,
  VL53L7_INFO_SIZE,
  VL53L7_READ_MAX_LEN,
  VL53L7_STREAM_CHUNK_MAX,
  VL53L7_WRITE_MAX_LEN,
  Vl53l7Cmd,
  Vl53l7I2cError,
  Vl53l7Rpt,
  packPinCtrl,
  packVl53l7ReadReg,
  packVl53l7SetI2cSpeed,
  packVl53l7WriteReg,
  resolveVl53l7Model,
  unpackVl53l7Info,
} from "./protocol/vl53l7.js";
export type { Vl53l7Info, Vl53l7Model } from "./protocol/vl53l7.js";
export {
  VL53LX_CLEAR_STEPS_MAX,
  VL53LX_INFO_SIZE,
  Vl53lxCmd,
  Vl53lxRpt,
  packVl53lxSetAddrWidth,
  packVl53lxStartStream,
  unpackVl53lxInfo,
} from "./protocol/vl53lx.js";
export type { Vl53lxClearStep, Vl53lxInfo } from "./protocol/vl53lx.js";
export {
  BUSY_BACKOFF_MS,
  Bno086Cmd,
  Bno086Rpt,
  TX_SLOTS,
  TX_SLOT_SIZE,
  unpackBno086Data,
} from "./protocol/bno086.js";
export type { Bno086Data } from "./protocol/bno086.js";
export {
  BNO055_EXPECTED_IDS,
  BNO055_I2C_ERROR_NAMES,
  BNO055_INFO_SIZE,
  BNO055_RESET_TIMEOUT_MS,
  BNO055_TIMER_PERIOD_MAX_MS,
  BNO055_TIMER_PERIOD_MIN_MS,
  BNO055_TRIGGER_INT,
  BNO055_TRIGGER_TIMER,
  BNO055_WRITE_MAX,
  BNO055_XFER_MAX,
  Bno055Cmd,
  Bno055Rpt,
  bno055IdsOk,
  bno055SwRevText,
  packBno055ReadReg,
  packBno055StartStream,
  packBno055WriteReg,
  unpackBno055Info,
  unpackBno055RegData,
  unpackBno055Stream,
} from "./protocol/bno055.js";
export type { Bno055Info, Bno055RegData, Bno055StreamData } from "./protocol/bno055.js";
export {
  TMF8829_CHUNK_MAX,
  TMF8829_INT_HISTOGRAMS,
  TMF8829_INT_MASK_ALL,
  TMF8829_INT_MOTION,
  TMF8829_INT_PROXIMITY,
  TMF8829_INT_RESULTS,
  TMF8829_READ_MAX,
  TMF8829_SF_REF_FRAME,
  TMF8829_WRITE_MAX,
  Tmf8829Cmd,
  Tmf8829EventCode,
  Tmf8829FrameReassembler,
  Tmf8829Rpt,
  packTmf8829ReadReg,
  packTmf8829StartStream,
  packTmf8829WriteReg,
  unpackTmf8829Event,
  unpackTmf8829FrameChunk,
  unpackTmf8829Info,
  unpackTmf8829RegData,
} from "./protocol/tmf8829.js";
export type {
  Tmf8829Event,
  Tmf8829FrameChunk,
  Tmf8829Info,
  Tmf8829RawFrame,
  Tmf8829RegData,
} from "./protocol/tmf8829.js";

// ── errors (contract 07 §3) ──────────────────────────────────────────────────
export {
  BootloaderModeError,
  BusyError,
  DepzError,
  DepzTimeoutError,
  DeviceLostError,
  InvalidReplyError,
  InvalidStateError,
  LinkClosedError,
  NoDepzDeviceError,
  ReplayMismatchError,
  StatusError,
} from "./errors.js";

// ── discovery and the device base ────────────────────────────────────────────
export {
  SENSOR_CLASS_NAMES,
  attachSensorClass,
  isDeviceInfo,
  listDepzDevicesFrom,
  openDeviceByDeviceSerial,
  openDeviceFrom,
  openOnTransport,
  orderDevicesBySerial,
  orderPortsBySerial,
  probePort,
  resolveSensorClass,
  sensorClass,
} from "./discovery.js";
export type {
  DeviceInfo,
  DepzPortInfo,
  ListOptions,
  OpenDeviceOptions,
  OpenOnTransportOptions,
  OpenTarget,
  SensorClassConstructor,
  SensorClassName,
  TransportFactory,
} from "./discovery.js";
export type { LinkStats, SeqGap } from "./device/link.js";
export { DEFAULT_TIMEOUT_MS, DepzDevice, hostNowUs } from "./device/device.js";
export type {
  DeviceEvent,
  DeviceOptions,
  Matcher,
  RequestOptions,
  TimeSync,
} from "./device/device.js";
export { Stream } from "./stream.js";
export type { StreamIterator, StreamOptions } from "./stream.js";

// ── sensors ──────────────────────────────────────────────────────────────────
export { Sr04 } from "./sensors/sr04.js";
export type { Sr04Measurement } from "./sensors/sr04.js";

export { Vl53l4cd } from "./sensors/vl53l4/vl53l4.js";
export type { Vl53l4Measurement, Vl53l4Options } from "./sensors/vl53l4/vl53l4.js";
export {
  I2C_KHZ_BOOT,
  I2C_KHZ_DEFAULT,
  MODEL_ID_VL53L4CD,
  RANGE_STATUS_NAMES,
  Vl53l4cdError,
  WINDOW_ABOVE,
  WINDOW_BELOW,
  WINDOW_IN,
  WINDOW_OUT,
  rangeStatusText,
} from "./sensors/vl53l4/uld.js";
export type { RangeTiming } from "./sensors/vl53l4/uld.js";

export {
  MAX_RANGING_FREQUENCY_HZ,
  MIN_RANGING_FREQUENCY_HZ,
  Vl53l8ch,
  Vl53l8cx,
  zoneGrid,
} from "./sensors/vl53l8/vl53l8.js";
export type { Vl53l8Frame, Vl53l8InitOptions, Vl53l8Options } from "./sensors/vl53l8/vl53l8.js";
export {
  AMBIENT_PER_SPAD_KCPS,
  DIST_MM,
  MODULE_TYPE_MZ,
  MODULE_TYPE_MZEVO,
  MODULE_TYPE_MZPLUS,
  MODULE_TYPE_NAMES,
  MOTION_INDICATOR,
  MotionConfig,
  NB_SPADS_ENABLED,
  NB_TARGET_DETECTED,
  NB_THRESHOLDS,
  POWER_MODE_DEEP_SLEEP,
  POWER_MODE_SLEEP,
  POWER_MODE_WAKEUP,
  RANGE_SIGMA_MM,
  RANGING_MODE_AUTONOMOUS,
  RANGING_MODE_CONTINUOUS,
  RESOLUTION_4X4,
  RESOLUTION_8X8,
  SIGNAL_PER_SPAD_KCPS,
  TAR_STATUS,
  TARGET_ORDER_CLOSEST,
  TARGET_ORDER_STRONGEST,
  THRESH_EQUAL_MIN,
  THRESH_GREATER_THAN_MAX,
  THRESH_IN_WINDOW,
  THRESH_LESS_THAN_EQUAL_MIN,
  THRESH_NOT_EQUAL_MIN,
  THRESH_OP_AND,
  THRESH_OP_NONE,
  THRESH_OP_OR,
  THRESH_OUT_OF_WINDOW,
  Vl53l8cxError,
  XTALK_MARGIN_MAX_KCPS,
} from "./sensors/vl53l8/uld.js";
export type { DetectionThreshold, MotionResult } from "./sensors/vl53l8/uld.js";
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
export type {
  Measurement as Vl53lxDriverMeasurement,
  Target as Vl53lxTarget,
} from "./sensors/vl53lx/uld/base.js";
export {
  ProtocolError as Vl53lxProtocolError,
  Vl53Error as Vl53lxError,
} from "./sensors/vl53lx/uld/link.js";
export { HistogramBinData } from "./sensors/vl53lx/uld/bare/core.js";

export { Bno086 } from "./sensors/bno086/bno086.js";
export type { Bno086Options, CalibrationConfig, EnableOptions } from "./sensors/bno086/bno086.js";
export {
  ACTIVITY_NAMES,
  MOTION_INTENT_NAMES,
  MOTION_REQUEST_NAMES,
  STABILITY_NAMES,
  SensorId,
} from "./sensors/bno086/reports.js";
export type {
  Acceleration,
  FlushCompleted,
  GenericEvent,
  GyroIntegratedRV,
  Gyroscope,
  InputReport,
  InputReportBase,
  Magnetometer,
  MotionRequest,
  PersonalActivityClassifier,
  RawSensor,
  Report,
  ReportBase,
  RotationVector,
  ScalarReport,
  ShakeDetector,
  SignificantMotion,
  StabilityClassifier,
  StepCounter,
  StepDetector,
  TapDetector,
  UncalibratedGyroscope,
  UncalibratedMagnetometer,
  UnknownReport,
} from "./sensors/bno086/reports.js";
export {
  ErrorSource,
  FeatureFlag,
  FrsRecordId,
  MotionIntent,
  OscillatorType,
  Sh2Error,
  TareAxis,
  TareBasis,
  errorRecordFromResponse,
  sensorMetadataFromWords,
  unpackCommandResponse,
  unpackFeatureResponse,
  unpackProductId,
} from "./sensors/bno086/sh2.js";
export type {
  CommandResponse,
  Counts,
  ErrorRecord,
  FeatureResponse,
  ProductId,
  SensorMetadata,
} from "./sensors/bno086/sh2.js";
export { ExecutableCommand } from "./sensors/bno086/shtp.js";

export { Bno055 } from "./sensors/bno055/bno055.js";
export type {
  Bno055ConfigureOptions,
  Bno055Options,
  Bno055Sample,
} from "./sensors/bno055/bno055.js";
export {
  BNO055_ACC_BANDWIDTH_HZ,
  BNO055_ACC_RANGE_G,
  BNO055_AXIS_X,
  BNO055_AXIS_Y,
  BNO055_AXIS_Z,
  BNO055_CALIB_PROFILE_LEN,
  BNO055_DEFAULT_AXIS_REMAP,
  BNO055_DEFAULT_UNITS,
  BNO055_FULL_BLOCK,
  BNO055_GYR_BANDWIDTH_HZ,
  BNO055_GYR_RANGE_DPS,
  BNO055_INT,
  BNO055_MAG_RATE_HZ,
  BNO055_PLACEMENTS,
  BNO055_QUAT_BLOCK,
  BNO055_REG1_ACC_CONFIG,
  BNO055_REG1_GYR_CONFIG_0,
  BNO055_REG1_GYR_CONFIG_1,
  BNO055_REG1_INT_EN,
  BNO055_REG1_INT_MSK,
  BNO055_REG1_INT_SETTINGS_FIRST,
  BNO055_REG1_INT_SETTINGS_LAST,
  BNO055_REG1_MAG_CONFIG,
  BNO055_REG1_UNIQUE_ID,
  BNO055_REG_ACC_DATA,
  BNO055_REG_AXIS_MAP_CONFIG,
  BNO055_REG_AXIS_MAP_SIGN,
  BNO055_REG_CALIB_PROFILE,
  BNO055_REG_CALIB_STAT,
  BNO055_REG_CHIP_ID,
  BNO055_REG_EUL_DATA,
  BNO055_REG_GRV_DATA,
  BNO055_REG_GYR_DATA,
  BNO055_REG_INT_STA,
  BNO055_REG_LIA_DATA,
  BNO055_REG_MAG_DATA,
  BNO055_REG_OPR_MODE,
  BNO055_REG_PAGE_ID,
  BNO055_REG_PWR_MODE,
  BNO055_REG_QUA_DATA,
  BNO055_REG_SIC_MATRIX,
  BNO055_REG_ST_RESULT,
  BNO055_REG_SYS_CLK_STATUS,
  BNO055_REG_SYS_ERR,
  BNO055_REG_SYS_STATUS,
  BNO055_REG_SYS_TRIGGER,
  BNO055_REG_TEMP,
  BNO055_REG_TEMP_SOURCE,
  BNO055_REG_UNIT_SEL,
  BNO055_SIC_IDENTITY,
  BNO055_ST,
  BNO055_SYS_ERR_NAMES,
  BNO055_SYS_STATUS_NAMES,
  BNO055_UNIQUE_ID_LEN,
  Bno055OprMode,
  Bno055PwrMode,
  Bno055TempSource,
  bno055FullyCalibrated,
  bno055IsFusion,
  bno055Placement,
  decodeBno055Block,
  makeBno055SystemStatus,
  packBno055AccelConfig,
  packBno055AxisRemap,
  packBno055CalibrationProfile,
  packBno055CalibStatus,
  packBno055GyroConfig,
  packBno055MagConfig,
  packBno055SicMatrix,
  packBno055Units,
  unpackBno055AccelConfig,
  unpackBno055AxisRemap,
  unpackBno055CalibrationProfile,
  unpackBno055CalibStatus,
  unpackBno055GyroConfig,
  unpackBno055MagConfig,
  unpackBno055SicMatrix,
  unpackBno055Units,
} from "./sensors/bno055/regs.js";
export type {
  Bno055AccelConfig,
  Bno055AxisRemap,
  Bno055CalibStatus,
  Bno055CalibrationProfile,
  Bno055GyroConfig,
  Bno055MagConfig,
  Bno055RawBlock,
  Bno055SystemStatus,
  Bno055Units,
} from "./sensors/bno055/regs.js";

export {
  TMF8829_FIRMWARE_LOAD_ADDR,
  Tmf8829,
  Tmf8829Error,
  tmf8829FirmwareImage,
} from "./sensors/tmf8829/tmf8829.js";
export type {
  Tmf8829Config,
  Tmf8829ConfigureOptions,
  Tmf8829Options,
} from "./sensors/tmf8829/tmf8829.js";
export {
  Tmf8829MeasurementBuilder,
  tmf8829DistanceMm,
  tmf8829RecordValue,
} from "./sensors/tmf8829/builder.js";
export type { Tmf8829Measurement } from "./sensors/tmf8829/builder.js";
export {
  TMF8829_FP_GRID,
  TMF8829_FP_NAMES,
  mergeTmf8829Halves,
  parseTmf8829Frame,
  parseTmf8829Results,
} from "./sensors/tmf8829/frames.js";
export type {
  Tmf8829FrameInfo,
  Tmf8829HistogramFrame,
  Tmf8829PixelResults,
} from "./sensors/tmf8829/frames.js";
export {
  TMF8829_FT_CLOCK_RESET,
  TMF8829_FT_EDGE_LATE,
  TMF8829_FT_NAMES,
  TMF8829_FT_NOMINAL,
  TMF8829_FT_STALLED,
  TMF8829_FT_T1_UNSURE,
  Tmf8829CaptureTimer,
  Tmf8829SensorClock,
  tmf8829FlagNames,
} from "./sensors/tmf8829/clock.js";
export type { Tmf8829FrameTime } from "./sensors/tmf8829/clock.js";
export {
  TMF8829_CFG_FIELDS,
  TMF8829_PRESETS,
  tmf8829CfgGet,
  tmf8829CfgSet,
  tmf8829ConfigFields,
} from "./sensors/tmf8829/config.js";
export type { Tmf8829CfgField, Tmf8829Preset } from "./sensors/tmf8829/config.js";
export { TMF8829_FIRMWARE_LICENSE } from "./sensors/tmf8829/firmware-license.js";

// ── recording, datasets, transports ──────────────────────────────────────────
export { RECORDING_SCHEMA, RecordingTransport, ReplayTransport } from "./transport/replay.js";
export {
  DATASET_SCHEMA,
  DatasetPlayer,
  DatasetReader,
  SessionRecorder,
  DatasetWriter,
  bno086RecordValue,
} from "./dataset.js";
export type {
  DatasetDeviceMeta,
  DatasetHeader,
  DatasetPlayerOptions,
  DatasetRecord,
  PlayerState,
} from "./dataset.js";
export type { SerialTransport, SerialTransportInfo } from "./transport/types.js";
export { LoopbackTransport } from "./transport/loopback.js";
export {
  WebSerialTransport,
  isWebSerialSupported,
  getGrantedPorts,
  forgetPort,
  watchConnect,
} from "./transport/webserial.js";
export type { WebSerialPortLike } from "./transport/webserial.js";
export {
  WebUsbSerialPort,
  DEPZ_WEBUSB_FILTERS,
  WEBUSB_READS_IN_FLIGHT,
  WEBUSB_WRITE_TIMEOUT_MS,
  getGrantedWebUsbPorts,
  isWebUsbSupported,
  lineCoding,
  requestWebUsbPort,
  watchWebUsbConnect,
  webUsbPort,
} from "./transport/webusb.js";
export type {
  UsbControlSetup,
  UsbDeviceFilter,
  UsbDeviceLike,
  UsbEndpointLike,
  UsbInterfaceLike,
} from "./transport/webusb.js";
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
