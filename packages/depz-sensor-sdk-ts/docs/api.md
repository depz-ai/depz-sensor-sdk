# @depz/sensor-sdk — API reference

Generated from the TypeScript sources by TypeDoc. Each sensor also has a focused reference with just its own symbols: [SR04](sr04/api.md) · [VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · [BNO086](bno086/api.md). For the narrative guides see the per-sensor `docs/<sensor>/` pages and `docs/overview.md`.

# @depz/sensor-sdk

## Modules

- [index](index/README.md)
- [transport/node](transport/node/README.md)
- [transport/webserial](transport/webserial/README.md)

# index

## Enumerations

- [Bno086Cmd](enumerations/Bno086Cmd.md)
- [Bno086Rpt](enumerations/Bno086Rpt.md)
- [Cmd](enumerations/Cmd.md)
- [Rpt](enumerations/Rpt.md)
- [Status](enumerations/Status.md)
- [SyncPinMode](enumerations/SyncPinMode.md)
- [SyncPinPolarity](enumerations/SyncPinPolarity.md)
- [CrcType](enumerations/CrcType.md)
- [Sr04Cmd](enumerations/Sr04Cmd.md)
- [Sr04Rpt](enumerations/Sr04Rpt.md)
- [Vl53l4Cmd](enumerations/Vl53l4Cmd.md)
- [Vl53l4Rpt](enumerations/Vl53l4Rpt.md)
- [Vl53l8Cmd](enumerations/Vl53l8Cmd.md)
- [Vl53l8Rpt](enumerations/Vl53l8Rpt.md)
- [SensorId](enumerations/SensorId.md)
- [ControlReport](enumerations/ControlReport.md)
- [Sh2Command](enumerations/Sh2Command.md)
- [OscillatorType](enumerations/OscillatorType.md)
- [ErrorSource](enumerations/ErrorSource.md)
- [TareBasis](enumerations/TareBasis.md)
- [TareAxis](enumerations/TareAxis.md)
- [FrsRecordId](enumerations/FrsRecordId.md)
- [FrsStatus](enumerations/FrsStatus.md)
- [FrsWriteStatus](enumerations/FrsWriteStatus.md)
- [ShtpChannel](enumerations/ShtpChannel.md)

## Classes

- [DatasetWriter](classes/DatasetWriter.md)
- [DatasetRecorder](classes/DatasetRecorder.md)
- [DatasetReader](classes/DatasetReader.md)
- [DatasetPlayer](classes/DatasetPlayer.md)
- [StreamQueue](classes/StreamQueue.md)
- [DepzDevice](classes/DepzDevice.md)
- [DepzLink](classes/DepzLink.md)
- [DepzError](classes/DepzError.md)
- [DepzTimeoutError](classes/DepzTimeoutError.md)
- [StatusError](classes/StatusError.md)
- [BusyError](classes/BusyError.md)
- [DeviceLostError](classes/DeviceLostError.md)
- [LinkClosedError](classes/LinkClosedError.md)
- [NoDepzDeviceError](classes/NoDepzDeviceError.md)
- [PacketParser](classes/PacketParser.md)
- [FwDepzError](classes/FwDepzError.md)
- [FrameReassembler](classes/FrameReassembler.md)
- [Bno086](classes/Bno086.md)
- [Sh2Error](classes/Sh2Error.md)
- [FrsReadSession](classes/FrsReadSession.md)
- [FrsWriteSession](classes/FrsWriteSession.md)
- [ShtpLayer](classes/ShtpLayer.md)
- [Sr04](classes/Sr04.md)
- [Vl53l4cdError](classes/Vl53l4cdError.md)
- [VL53L4CD](classes/VL53L4CD.md)
- [Vl53l4Cd](classes/Vl53l4Cd-1.md)
- [CnhConfigError](classes/CnhConfigError.md)
- [CnhConfig](classes/CnhConfig.md)
- [Vl53l8cxError](classes/Vl53l8cxError.md)
- [MotionConfig](classes/MotionConfig.md)
- [VL53L8CX](classes/VL53L8CX.md)
- [Vl53l8Cx](classes/Vl53l8Cx-1.md)
- [Vl53l8Ch](classes/Vl53l8Ch.md)
- [LoopbackTransport](classes/LoopbackTransport.md)
- [RecordingTransport](classes/RecordingTransport.md)
- [ReplayTransport](classes/ReplayTransport.md)
- [WsBackendTransport](classes/WsBackendTransport.md)

## Interfaces

- [DatasetDeviceMeta](interfaces/DatasetDeviceMeta.md)
- [DatasetHeader](interfaces/DatasetHeader.md)
- [DatasetRecord](interfaces/DatasetRecord.md)
- [TimeSync](interfaces/TimeSync.md)
- [RequestOptions](interfaces/RequestOptions.md)
- [DeviceOptions](interfaces/DeviceOptions.md)
- [LinkStats](interfaces/LinkStats.md)
- [DepzPortInfo](interfaces/DepzPortInfo.md)
- [DeviceInfo](interfaces/DeviceInfo.md)
- [OpenDeviceOptions](interfaces/OpenDeviceOptions.md)
- [ListOptions](interfaces/ListOptions.md)
- [Bno086Data](interfaces/Bno086Data.md)
- [StatusReport](interfaces/StatusReport.md)
- [TextReport](interfaces/TextReport.md)
- [SyncTimeReport](interfaces/SyncTimeReport.md)
- [TemperatureReport](interfaces/TemperatureReport.md)
- [SequenceErrorReport](interfaces/SequenceErrorReport.md)
- [SyncPinConfig](interfaces/SyncPinConfig.md)
- [PacketEvent](interfaces/PacketEvent.md)
- [TrashEvent](interfaces/TrashEvent.md)
- [CrcErrorEvent](interfaces/CrcErrorEvent.md)
- [FwDepzImage](interfaces/FwDepzImage.md)
- [Identity](interfaces/Identity.md)
- [Sr04Data](interfaces/Sr04Data.md)
- [DepzUsbModel](interfaces/DepzUsbModel.md)
- [Vl53l4RegData](interfaces/Vl53l4RegData.md)
- [Vl53l4Info](interfaces/Vl53l4Info.md)
- [Vl53l4StreamData](interfaces/Vl53l4StreamData.md)
- [RegData](interfaces/RegData.md)
- [FrameChunk](interfaces/FrameChunk.md)
- [CalibrationConfig](interfaces/CalibrationConfig.md)
- [Bno086Options](interfaces/Bno086Options.md)
- [EnableOptions](interfaces/EnableOptions.md)
- [ReportBase](interfaces/ReportBase.md)
- [InputReportBase](interfaces/InputReportBase.md)
- [Acceleration](interfaces/Acceleration.md)
- [Gyroscope](interfaces/Gyroscope.md)
- [Magnetometer](interfaces/Magnetometer.md)
- [UncalibratedGyroscope](interfaces/UncalibratedGyroscope.md)
- [UncalibratedMagnetometer](interfaces/UncalibratedMagnetometer.md)
- [RotationVector](interfaces/RotationVector.md)
- [GyroIntegratedRV](interfaces/GyroIntegratedRV.md)
- [ScalarReport](interfaces/ScalarReport.md)
- [TapDetector](interfaces/TapDetector.md)
- [StepCounter](interfaces/StepCounter.md)
- [StepDetector](interfaces/StepDetector.md)
- [SignificantMotion](interfaces/SignificantMotion.md)
- [StabilityClassifier](interfaces/StabilityClassifier.md)
- [ShakeDetector](interfaces/ShakeDetector.md)
- [GenericEvent](interfaces/GenericEvent.md)
- [PersonalActivityClassifier](interfaces/PersonalActivityClassifier.md)
- [RawSensor](interfaces/RawSensor.md)
- [UnknownReport](interfaces/UnknownReport.md)
- [ErrorRecord](interfaces/ErrorRecord.md)
- [Counts](interfaces/Counts.md)
- [FeatureResponse](interfaces/FeatureResponse.md)
- [ProductId](interfaces/ProductId.md)
- [CommandResponse](interfaces/CommandResponse.md)
- [FrsReadResponse](interfaces/FrsReadResponse.md)
- [FrsWriteResponse](interfaces/FrsWriteResponse.md)
- [SensorMetadata](interfaces/SensorMetadata.md)
- [ShtpHeader](interfaces/ShtpHeader.md)
- [ShtpCargo](interfaces/ShtpCargo.md)
- [Sr04Measurement](interfaces/Sr04Measurement.md)
- [Vl53l4Platform](interfaces/Vl53l4Platform.md)
- [Vl53l4Results](interfaces/Vl53l4Results.md)
- [RangeTimingRegisters](interfaces/RangeTimingRegisters.md)
- [RangeTiming](interfaces/RangeTiming.md)
- [Vl53l4Measurement](interfaces/Vl53l4Measurement.md)
- [Vl53l4Options](interfaces/Vl53l4Options.md)
- [Vl53l8Assets](interfaces/Vl53l8Assets.md)
- [CnhAggregate](interfaces/CnhAggregate.md)
- [CnhDecoded](interfaces/CnhDecoded.md)
- [Vl53l8Platform](interfaces/Vl53l8Platform.md)
- [DetectionThreshold](interfaces/DetectionThreshold.md)
- [MotionResult](interfaces/MotionResult.md)
- [Vl53l8Results](interfaces/Vl53l8Results.md)
- [Vl53l8Frame](interfaces/Vl53l8Frame.md)
- [Vl53l8Options](interfaces/Vl53l8Options.md)
- [Vl53l8InitOptions](interfaces/Vl53l8InitOptions.md)
- [SerialTransportInfo](interfaces/SerialTransportInfo.md)
- [SerialTransport](interfaces/SerialTransport.md)
- [BackendDevice](interfaces/BackendDevice.md)
- [BackendHealth](interfaces/BackendHealth.md)
- [BackendPermissions](interfaces/BackendPermissions.md)
- [WebSocketLike](interfaces/WebSocketLike.md)
- [WsBackendTransportOptions](interfaces/WsBackendTransportOptions.md)

## Type Aliases

- [PlayerState](type-aliases/PlayerState.md)
- [DeviceEvent](type-aliases/DeviceEvent.md)
- [Matcher](type-aliases/Matcher.md)
- [TransportFactory](type-aliases/TransportFactory.md)
- [OpenTarget](type-aliases/OpenTarget.md)
- [ParserEvent](type-aliases/ParserEvent.md)
- [SensorType](type-aliases/SensorType.md)
- [InputReport](type-aliases/InputReport.md)
- [Report](type-aliases/Report.md)
- [Vl53l8Variant](type-aliases/Vl53l8Variant.md)
- [Vl53l8](type-aliases/Vl53l8.md)

## Variables

- [DATASET\_SCHEMA](variables/DATASET_SCHEMA.md)
- [DEFAULT\_TIMEOUT\_MS](variables/DEFAULT_TIMEOUT_MS.md)
- [NO\_MATCH](variables/NO_MATCH.md)
- [TX\_SLOTS](variables/TX_SLOTS.md)
- [TX\_SLOT\_SIZE](variables/TX_SLOT_SIZE.md)
- [BUSY\_BACKOFF\_MS](variables/BUSY_BACKOFF_MS.md)
- [UNSOLICITED](variables/UNSOLICITED.md)
- [MAGIC0](variables/MAGIC0.md)
- [MAGIC1](variables/MAGIC1.md)
- [HEADER\_SIZE](variables/HEADER_SIZE.md)
- [MAX\_PAYLOAD](variables/MAX_PAYLOAD.md)
- [FWDEPZ\_MAGIC](variables/FWDEPZ_MAGIC.md)
- [FWDEPZ\_HEADER\_SIZE](variables/FWDEPZ_HEADER_SIZE.md)
- [ECHO\_TIMEOUT](variables/ECHO_TIMEOUT.md)
- [SAMPLE\_PERIOD\_DEFAULT\_US](variables/SAMPLE_PERIOD_DEFAULT_US.md)
- [ECHO\_DECAY\_DEFAULT\_US](variables/ECHO_DECAY_DEFAULT_US.md)
- [ECHO\_DECAY\_MIN\_US](variables/ECHO_DECAY_MIN_US.md)
- [ECHO\_DECAY\_MAX\_US](variables/ECHO_DECAY_MAX_US.md)
- [DEPZ\_VID](variables/DEPZ_VID.md)
- [DEV\_VID](variables/DEV_VID.md)
- [DEV\_PID](variables/DEV_PID.md)
- [DEPZ\_SENSOR\_PID\_MIN](variables/DEPZ_SENSOR_PID_MIN.md)
- [DEPZ\_SENSOR\_PID\_MAX](variables/DEPZ_SENSOR_PID_MAX.md)
- [DEPZ\_USB\_MODELS](variables/DEPZ_USB_MODELS.md)
- [VL53L4\_XFER\_MAX](variables/VL53L4_XFER_MAX.md)
- [VL53L4\_XSHUT\_OFF](variables/VL53L4_XSHUT_OFF.md)
- [VL53L4\_XSHUT\_ON](variables/VL53L4_XSHUT_ON.md)
- [VL53L4\_XSHUT\_RESET](variables/VL53L4_XSHUT_RESET.md)
- [VL53L4\_SF\_INT\_ACT\_HIGH](variables/VL53L4_SF_INT_ACT_HIGH.md)
- [VL53L4\_I2C\_KHZ\_STEPS](variables/VL53L4_I2C_KHZ_STEPS.md)
- [VL53L4\_I2C\_ERROR\_NAMES](variables/VL53L4_I2C_ERROR_NAMES.md)
- [READ\_MAX\_LEN](variables/READ_MAX_LEN.md)
- [CHUNK\_SIZE](variables/CHUNK_SIZE.md)
- [STREAM\_CHUNK\_MAX](variables/STREAM_CHUNK_MAX.md)
- [STREAM\_TOTAL\_MAX](variables/STREAM_TOTAL_MAX.md)
- [RATE\_LOW\_FACTOR](variables/RATE_LOW_FACTOR.md)
- [RATE\_HIGH\_FACTOR](variables/RATE_HIGH_FACTOR.md)
- [BASE\_TIMESTAMP\_REF](variables/BASE_TIMESTAMP_REF.md)
- [TIMESTAMP\_REBASE](variables/TIMESTAMP_REBASE.md)
- [Q\_POINTS](variables/Q_POINTS.md)
- [RV\_ACCURACY\_Q](variables/RV_ACCURACY_Q.md)
- [GYRO\_RV\_ANGVEL\_Q](variables/GYRO_RV_ANGVEL_Q.md)
- [REPORT\_LENGTHS](variables/REPORT_LENGTHS.md)
- [STABILITY\_NAMES](variables/STABILITY_NAMES.md)
- [ACTIVITY\_NAMES](variables/ACTIVITY_NAMES.md)
- [COUNTS\_GET](variables/COUNTS_GET.md)
- [COUNTS\_CLEAR](variables/COUNTS_CLEAR.md)
- [ME\_CAL\_GET](variables/ME_CAL_GET.md)
- [METADATA\_RECORDS](variables/METADATA_RECORDS.md)
- [SHTP\_HEADER\_SIZE](variables/SHTP_HEADER_SIZE.md)
- [LENGTH\_MASK](variables/LENGTH_MASK.md)
- [CONTINUATION\_BIT](variables/CONTINUATION_BIT.md)
- [NUM\_CHANNELS](variables/NUM_CHANNELS.md)
- [MAX\_TX\_FRAME](variables/MAX_TX_FRAME.md)
- [ULD\_VERSION](variables/ULD_VERSION.md)
- [SOFT\_RESET](variables/SOFT_RESET.md)
- [I2C\_SLAVE\_\_DEVICE\_ADDRESS](variables/I2C_SLAVE__DEVICE_ADDRESS.md)
- [OSC\_FREQUENCY](variables/OSC_FREQUENCY.md)
- [VHV\_CONFIG\_\_TIMEOUT\_MACROP\_LOOP\_BOUND](variables/VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND.md)
- [XTALK\_PLANE\_OFFSET\_KCPS](variables/XTALK_PLANE_OFFSET_KCPS.md)
- [XTALK\_X\_PLANE\_GRADIENT\_KCPS](variables/XTALK_X_PLANE_GRADIENT_KCPS.md)
- [XTALK\_Y\_PLANE\_GRADIENT\_KCPS](variables/XTALK_Y_PLANE_GRADIENT_KCPS.md)
- [RANGE\_OFFSET\_MM](variables/RANGE_OFFSET_MM.md)
- [INNER\_OFFSET\_MM](variables/INNER_OFFSET_MM.md)
- [OUTER\_OFFSET\_MM](variables/OUTER_OFFSET_MM.md)
- [GPIO\_HV\_MUX\_\_CTRL](variables/GPIO_HV_MUX__CTRL.md)
- [GPIO\_\_TIO\_HV\_STATUS](variables/GPIO__TIO_HV_STATUS.md)
- [SYSTEM\_\_INTERRUPT](variables/SYSTEM__INTERRUPT.md)
- [RANGE\_CONFIG\_A](variables/RANGE_CONFIG_A.md)
- [RANGE\_CONFIG\_B](variables/RANGE_CONFIG_B.md)
- [RANGE\_CONFIG\_\_SIGMA\_THRESH](variables/RANGE_CONFIG__SIGMA_THRESH.md)
- [MIN\_COUNT\_RATE\_RTN\_LIMIT\_MCPS](variables/MIN_COUNT_RATE_RTN_LIMIT_MCPS.md)
- [INTERMEASUREMENT\_MS](variables/INTERMEASUREMENT_MS.md)
- [THRESH\_HIGH](variables/THRESH_HIGH.md)
- [THRESH\_LOW](variables/THRESH_LOW.md)
- [SYSTEM\_\_INTERRUPT\_CLEAR](variables/SYSTEM__INTERRUPT_CLEAR.md)
- [SYSTEM\_START](variables/SYSTEM_START.md)
- [RESULT\_\_RANGE\_STATUS](variables/RESULT__RANGE_STATUS.md)
- [RESULT\_\_SPAD\_NB](variables/RESULT__SPAD_NB.md)
- [RESULT\_\_SIGNAL\_RATE](variables/RESULT__SIGNAL_RATE.md)
- [RESULT\_\_AMBIENT\_RATE](variables/RESULT__AMBIENT_RATE.md)
- [RESULT\_\_SIGMA](variables/RESULT__SIGMA.md)
- [RESULT\_\_DISTANCE](variables/RESULT__DISTANCE.md)
- [RESULT\_\_OSC\_CALIBRATE\_VAL](variables/RESULT__OSC_CALIBRATE_VAL.md)
- [FIRMWARE\_\_SYSTEM\_STATUS](variables/FIRMWARE__SYSTEM_STATUS.md)
- [IDENTIFICATION\_\_MODEL\_ID](variables/IDENTIFICATION__MODEL_ID.md)
- [MODEL\_ID\_VL53L4CD](variables/MODEL_ID_VL53L4CD.md)
- [WINDOW\_BELOW](variables/WINDOW_BELOW.md)
- [WINDOW\_ABOVE](variables/WINDOW_ABOVE.md)
- [WINDOW\_OUT](variables/WINDOW_OUT.md)
- [WINDOW\_IN](variables/WINDOW_IN.md)
- [CONFIG\_ADDR](variables/CONFIG_ADDR.md)
- [CONFIG\_END](variables/CONFIG_END.md)
- [DEFAULT\_CONFIGURATION](variables/DEFAULT_CONFIGURATION.md)
- [CONFIG\_FMP\_BYTE](variables/CONFIG_FMP_BYTE.md)
- [RESULT\_BLOCK\_ADDR](variables/RESULT_BLOCK_ADDR.md)
- [RESULT\_BLOCK\_LEN](variables/RESULT_BLOCK_LEN.md)
- [I2C\_KHZ\_BOOT](variables/I2C_KHZ_BOOT.md)
- [I2C\_KHZ\_DEFAULT](variables/I2C_KHZ_DEFAULT.md)
- [STATUS\_RTN](variables/STATUS_RTN.md)
- [RANGE\_STATUS\_NAMES](variables/RANGE_STATUS_NAMES.md)
- [CNH\_BIN\_WIDTH\_MM](variables/CNH_BIN_WIDTH_MM.md)
- [MI\_CFG\_DEV\_IDX](variables/MI_CFG_DEV_IDX.md)
- [CNH\_MAX\_DATA\_BYTES](variables/CNH_MAX_DATA_BYTES.md)
- [FW\_CHECKSUM](variables/FW_CHECKSUM.md)
- [RESOLUTION\_4X4](variables/RESOLUTION_4X4.md)
- [RESOLUTION\_8X8](variables/RESOLUTION_8X8.md)
- [RANGING\_MODE\_CONTINUOUS](variables/RANGING_MODE_CONTINUOUS.md)
- [RANGING\_MODE\_AUTONOMOUS](variables/RANGING_MODE_AUTONOMOUS.md)
- [TARGET\_ORDER\_CLOSEST](variables/TARGET_ORDER_CLOSEST.md)
- [TARGET\_ORDER\_STRONGEST](variables/TARGET_ORDER_STRONGEST.md)
- [NB\_THRESHOLDS](variables/NB_THRESHOLDS.md)
- [POWER\_MODE\_SLEEP](variables/POWER_MODE_SLEEP.md)
- [POWER\_MODE\_WAKEUP](variables/POWER_MODE_WAKEUP.md)
- [POWER\_MODE\_DEEP\_SLEEP](variables/POWER_MODE_DEEP_SLEEP.md)
- [THRESH\_IN\_WINDOW](variables/THRESH_IN_WINDOW.md)
- [THRESH\_OUT\_OF\_WINDOW](variables/THRESH_OUT_OF_WINDOW.md)
- [THRESH\_OP\_NONE](variables/THRESH_OP_NONE.md)
- [THRESH\_OP\_OR](variables/THRESH_OP_OR.md)
- [THRESH\_OP\_AND](variables/THRESH_OP_AND.md)
- [GET\_XTALK\_CMD](variables/GET_XTALK_CMD.md)
- [CALIBRATE\_XTALK](variables/CALIBRATE_XTALK.md)
- [MIN\_RANGING\_FREQUENCY\_HZ](variables/MIN_RANGING_FREQUENCY_HZ.md)
- [Vl53l8](variables/Vl53l8.md)
- [RECORDING\_SCHEMA](variables/RECORDING_SCHEMA.md)

## Functions

- [hostNowUs](functions/hostNowUs.md)
- [syncTimeAll](functions/syncTimeAll.md)
- [orderPortsBySerial](functions/orderPortsBySerial.md)
- [isDeviceInfo](functions/isDeviceInfo.md)
- [orderDevicesBySerial](functions/orderDevicesBySerial.md)
- [probePort](functions/probePort.md)
- [listDepzDevicesFrom](functions/listDepzDevicesFrom.md)
- [openDeviceFrom](functions/openDeviceFrom.md)
- [openDeviceByDeviceSerial](functions/openDeviceByDeviceSerial.md)
- [unpackBno086Data](functions/unpackBno086Data.md)
- [unpackStatus](functions/unpackStatus.md)
- [unpackText](functions/unpackText.md)
- [unpackSyncTime](functions/unpackSyncTime.md)
- [unpackTemperature](functions/unpackTemperature.md)
- [unpackSequenceError](functions/unpackSequenceError.md)
- [packSyncTime](functions/packSyncTime.md)
- [packSyncPinConfig](functions/packSyncPinConfig.md)
- [unpackSyncPinConfig](functions/unpackSyncPinConfig.md)
- [syncTimeOffsetRtt](functions/syncTimeOffsetRtt.md)
- [stripDeviceString](functions/stripDeviceString.md)
- [crc8Maxim](functions/crc8Maxim.md)
- [crc16Modbus](functions/crc16Modbus.md)
- [crc32IsoHdlc](functions/crc32IsoHdlc.md)
- [crc16CcittFalse](functions/crc16CcittFalse.md)
- [payloadCrcBytes](functions/payloadCrcBytes.md)
- [buildPacket](functions/buildPacket.md)
- [parseFwDepz](functions/parseFwDepz.md)
- [fwDepzPayloadCrcOk](functions/fwDepzPayloadCrcOk.md)
- [parseSoftwareName](functions/parseSoftwareName.md)
- [unpackSr04Data](functions/unpackSr04Data.md)
- [packSamplePeriod](functions/packSamplePeriod.md)
- [unpackSamplePeriod](functions/unpackSamplePeriod.md)
- [packEchoDecay](functions/packEchoDecay.md)
- [unpackEchoDecay](functions/unpackEchoDecay.md)
- [distanceMmFromEcho](functions/distanceMmFromEcho.md)
- [isKnownDepzUsb](functions/isKnownDepzUsb.md)
- [pidToModel](functions/pidToModel.md)
- [usbModelHint](functions/usbModelHint.md)
- [packVl53l4ReadReg](functions/packVl53l4ReadReg.md)
- [packVl53l4WriteReg](functions/packVl53l4WriteReg.md)
- [packVl53l4Xshut](functions/packVl53l4Xshut.md)
- [packVl53l4StartStream](functions/packVl53l4StartStream.md)
- [packVl53l4SetI2cSpeed](functions/packVl53l4SetI2cSpeed.md)
- [unpackVl53l4RegData](functions/unpackVl53l4RegData.md)
- [unpackVl53l4Info](functions/unpackVl53l4Info.md)
- [unpackVl53l4Stream](functions/unpackVl53l4Stream.md)
- [packReadReg](functions/packReadReg.md)
- [packWriteReg](functions/packWriteReg.md)
- [packStartStream](functions/packStartStream.md)
- [unpackRegData](functions/unpackRegData.md)
- [unpackFrameChunk](functions/unpackFrameChunk.md)
- [parseInputCargo](functions/parseInputCargo.md)
- [parseGyroRvCargo](functions/parseGyroRvCargo.md)
- [countsGetParams](functions/countsGetParams.md)
- [countsClearParams](functions/countsClearParams.md)
- [errorsParams](functions/errorsParams.md)
- [errorRecordFromResponse](functions/errorRecordFromResponse.md)
- [buildSetFeature](functions/buildSetFeature.md)
- [buildGetFeatureRequest](functions/buildGetFeatureRequest.md)
- [unpackFeatureResponse](functions/unpackFeatureResponse.md)
- [buildProductIdRequest](functions/buildProductIdRequest.md)
- [unpackProductId](functions/unpackProductId.md)
- [buildCommandRequest](functions/buildCommandRequest.md)
- [unpackCommandResponse](functions/unpackCommandResponse.md)
- [tareNowParams](functions/tareNowParams.md)
- [persistTareParams](functions/persistTareParams.md)
- [setReorientationParams](functions/setReorientationParams.md)
- [meCalibrationParams](functions/meCalibrationParams.md)
- [periodicDcdParams](functions/periodicDcdParams.md)
- [buildFrsReadRequest](functions/buildFrsReadRequest.md)
- [unpackFrsReadResponse](functions/unpackFrsReadResponse.md)
- [buildFrsWriteRequest](functions/buildFrsWriteRequest.md)
- [buildFrsWriteData](functions/buildFrsWriteData.md)
- [unpackFrsWriteResponse](functions/unpackFrsWriteResponse.md)
- [sensorMetadataFromWords](functions/sensorMetadataFromWords.md)
- [packShtpHeader](functions/packShtpHeader.md)
- [unpackShtpHeader](functions/unpackShtpHeader.md)
- [buildFrame](functions/buildFrame.md)
- [fragmentCargo](functions/fragmentCargo.md)
- [rangeStatusText](functions/rangeStatusText.md)
- [parseResultBlock](functions/parseResultBlock.md)
- [configBlock](functions/configBlock.md)
- [rangeTimingRegisters](functions/rangeTimingRegisters.md)
- [decodeRangeTiming](functions/decodeRangeTiming.md)
- [offsetRaw](functions/offsetRaw.md)
- [decodeOffset](functions/decodeOffset.md)
- [xtalkRaw](functions/xtalkRaw.md)
- [decodeXtalk](functions/decodeXtalk.md)
- [signalThresholdRaw](functions/signalThresholdRaw.md)
- [decodeSignalThreshold](functions/decodeSignalThreshold.md)
- [sigmaThresholdRaw](functions/sigmaThresholdRaw.md)
- [decodeSigmaThreshold](functions/decodeSigmaThreshold.md)
- [loadAssets](functions/loadAssets.md)
- [cnhMaxBins](functions/cnhMaxBins.md)
- [decodeCnh](functions/decodeCnh.md)
- [swapBuffer](functions/swapBuffer.md)
- [motionConfigSetResolution](functions/motionConfigSetResolution.md)
- [defaultMotionConfig](functions/defaultMotionConfig.md)
- [xtalkMarginToRaw](functions/xtalkMarginToRaw.md)
- [packDetectionThresholds](functions/packDetectionThresholds.md)
- [zoneGrid](functions/zoneGrid.md)
- [backendHealth](functions/backendHealth.md)
- [listBackendDevices](functions/listBackendDevices.md)
- [backendPermissions](functions/backendPermissions.md)

## References

### WebSerialTransport

Re-exports [WebSerialTransport](../transport/webserial/classes/WebSerialTransport.md)

***

### isWebSerialSupported

Re-exports [isWebSerialSupported](../transport/webserial/functions/isWebSerialSupported.md)

***

### getGrantedPorts

Re-exports [getGrantedPorts](../transport/webserial/functions/getGrantedPorts.md)

***

### watchConnect

Re-exports [watchConnect](../transport/webserial/functions/watchConnect.md)

***

### WebSerialPortLike

Re-exports [WebSerialPortLike](../transport/webserial/interfaces/WebSerialPortLike.md)

# Class: Bno086

Defined in: [src/sensors/bno086/bno086.ts:127](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L127)

BNO086 device: enable SH-2 sensors, stream typed reports.

Typical use:

    const imu = new Bno086(transport);
    await imu.open();
    await imu.enable(SensorId.RotationVector, 100);
    for await (const r of imu.reports()) { ... }

Callbacks run on the read-pump context — never await blocking device
methods (enable/tare/...) from inside one.

## Extends

- [`DepzDevice`](DepzDevice.md)

## Constructors

### Constructor

```ts
new Bno086(transport, opts?): Bno086;
```

Defined in: [src/sensors/bno086/bno086.ts:143](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L143)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | [`SerialTransport`](../interfaces/SerialTransport.md) |
| `opts?` | [`Bno086Options`](../interfaces/Bno086Options.md) |

#### Returns

`Bno086`

#### Overrides

[`DepzDevice`](DepzDevice.md).[`constructor`](DepzDevice.md#constructor)

## Properties

### timeoutMs

```ts
timeoutMs: number;
```

Defined in: [src/device/device.ts:178](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L178)

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`timeoutMs`](DepzDevice.md#timeoutms)

***

### link

```ts
protected readonly link: DepzLink;
```

Defined in: [src/device/device.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L180)

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`link`](DepzDevice.md#link)

***

### busyRetries

```ts
busyRetries: number = 5;
```

Defined in: [src/sensors/bno086/bno086.ts:129](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L129)

SEND_SHTP_PACKET attempts before giving up.

***

### busyBackoffMs

```ts
busyBackoffMs: number = BUSY_BACKOFF_MS;
```

Defined in: [src/sensors/bno086/bno086.ts:131](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L131)

>= 200 ms per the bridge spec (contract 05 §2).

## Accessors

### stats

#### Get Signature

```ts
get stats(): LinkStats;
```

Defined in: [src/device/device.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L210)

##### Returns

[`LinkStats`](../interfaces/LinkStats.md)

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`stats`](DepzDevice.md#stats)

***

### timeSync

#### Get Signature

```ts
get timeSync(): TimeSync | null;
```

Defined in: [src/device/device.ts:474](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L474)

##### Returns

[`TimeSync`](../interfaces/TimeSync.md) \| `null`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`timeSync`](DepzDevice.md#timesync)

***

### advertisement

#### Get Signature

```ts
get advertisement(): Uint8Array;
```

Defined in: [src/sensors/bno086/bno086.ts:402](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L402)

Raw SHTP channel-0 advertisement bytes seen since open/reset.

##### Returns

`Uint8Array`

## Methods

### open()

```ts
open(): Promise<void>;
```

Defined in: [src/device/device.ts:200](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L200)

Open the transport and start the read pump.

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`open`](DepzDevice.md#open)

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/device/device.ts:205](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L205)

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`close`](DepzDevice.md#close)

***

### registerStream()

```ts
protected registerStream(stream): () => void;
```

Defined in: [src/device/device.ts:330](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L330)

Register a stream to be closed on device teardown.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `stream` | \{ `close`: `void`; \} |
| `stream.close` |

#### Returns

() => `void`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`registerStream`](DepzDevice.md#registerstream)

***

### onEvent()

```ts
onEvent(cb): () => void;
```

Defined in: [src/device/device.ts:343](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L343)

Subscribe to unsolicited/diagnostic events (read-pump context; do not
block). Returns an unsubscribe function.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`ev`) => `void` |

#### Returns

() => `void`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`onEvent`](DepzDevice.md#onevent)

***

### emitEvent()

```ts
protected emitEvent(event): void;
```

Defined in: [src/device/device.ts:350](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L350)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `event` | [`DeviceEvent`](../type-aliases/DeviceEvent.md) |

#### Returns

`void`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`emitEvent`](DepzDevice.md#emitevent)

***

### request()

```ts
request<T>(
   cmd, 
   payload?, 
opts?): Promise<T>;
```

Defined in: [src/device/device.ts:366](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L366)

Send `cmd` and wait for its correlated completion (contract 02 §1).

Exactly one of the completion paths must be configured:
- `okCompletes` — RPT_STATUS(cmd, OK) resolves with undefined;
- `matcher` — first packet for which the matcher returns non-`NO_MATCH`
  resolves with the matcher's return value.
Non-OK RPT_STATUS echoing `cmd` always rejects (BusyError for ERR_BUSY,
StatusError otherwise). One in-flight request per opcode.

#### Type Parameters

| Type Parameter | Default type |
| ------ | ------ |
| `T` | `void` |

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cmd` | `number` |
| `payload` | `Uint8Array` |
| `opts` | [`RequestOptions`](../interfaces/RequestOptions.md)\<`T`\> |

#### Returns

`Promise`\<`T`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`request`](DepzDevice.md#request)

***

### expectReport()

```ts
static expectReport<T>(reportId, unpack): Matcher<T>;
```

Defined in: [src/device/device.ts:409](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L409)

Matcher for a typed report identified by its report ID alone.

#### Type Parameters

| Type Parameter |
| ------ |
| `T` |

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `reportId` | `number` |
| `unpack` | (`payload`) => `T` |

#### Returns

[`Matcher`](../type-aliases/Matcher.md)\<`T`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`expectReport`](DepzDevice.md#expectreport)

***

### expectText()

```ts
static expectText(requestCmd): Matcher<string>;
```

Defined in: [src/device/device.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L414)

Matcher for RPT_TEXT echoing `requestCmd`.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `requestCmd` | `number` |

#### Returns

[`Matcher`](../type-aliases/Matcher.md)\<`string`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`expectText`](DepzDevice.md#expecttext)

***

### getDeviceName()

```ts
getDeviceName(): Promise<string>;
```

Defined in: [src/device/device.ts:425](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L425)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getDeviceName`](DepzDevice.md#getdevicename)

***

### getSoftwareName()

```ts
getSoftwareName(): Promise<string>;
```

Defined in: [src/device/device.ts:431](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L431)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getSoftwareName`](DepzDevice.md#getsoftwarename)

***

### getSerialNumber()

```ts
getSerialNumber(): Promise<string>;
```

Defined in: [src/device/device.ts:437](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L437)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getSerialNumber`](DepzDevice.md#getserialnumber)

***

### identify()

```ts
identify(): Promise<Identity>;
```

Defined in: [src/device/device.ts:444](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L444)

Classify the running firmware (contract 02 §4).

#### Returns

`Promise`\<[`Identity`](../interfaces/Identity.md)\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`identify`](DepzDevice.md#identify)

***

### readMcuTemperature()

```ts
readMcuTemperature(): Promise<number>;
```

Defined in: [src/device/device.ts:449](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L449)

Last cached MCU temperature in °C (device refreshes ~2 Hz).

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`readMcuTemperature`](DepzDevice.md#readmcutemperature)

***

### syncTime()

```ts
syncTime(samples?): Promise<TimeSync>;
```

Defined in: [src/device/device.ts:457](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L457)

NTP-style sync; keeps the lowest-RTT sample (contract 02 §5).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `samples` | `number` | `5` |

#### Returns

`Promise`\<[`TimeSync`](../interfaces/TimeSync.md)\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`syncTime`](DepzDevice.md#synctime)

***

### toHostTimeUs()

```ts
toHostTimeUs(deviceTsUs): bigint;
```

Defined in: [src/device/device.ts:479](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L479)

Device µs → host monotonic µs (requires a prior `syncTime`).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `deviceTsUs` | `bigint` |

#### Returns

`bigint`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`toHostTimeUs`](DepzDevice.md#tohosttimeus)

***

### getReportPayloadCrc()

```ts
getReportPayloadCrc(): Promise<CrcType>;
```

Defined in: [src/device/device.ts:484](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L484)

#### Returns

`Promise`\<[`CrcType`](../enumerations/CrcType.md)\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getReportPayloadCrc`](DepzDevice.md#getreportpayloadcrc)

***

### setReportPayloadCrc()

```ts
setReportPayloadCrc(crcType): Promise<void>;
```

Defined in: [src/device/device.ts:491](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L491)

Set the device→host payload CRC mode (host→device is per-packet).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `crcType` | [`CrcType`](../enumerations/CrcType.md) |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`setReportPayloadCrc`](DepzDevice.md#setreportpayloadcrc)

***

### getSyncPin()

```ts
getSyncPin(pin): Promise<SyncPinConfig>;
```

Defined in: [src/device/device.ts:495](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L495)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pin` | `number` |

#### Returns

`Promise`\<[`SyncPinConfig`](../interfaces/SyncPinConfig.md)\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getSyncPin`](DepzDevice.md#getsyncpin)

***

### setSyncPin()

```ts
setSyncPin(config): Promise<void>;
```

Defined in: [src/device/device.ts:501](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L501)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `config` | [`SyncPinConfig`](../interfaces/SyncPinConfig.md) |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`setSyncPin`](DepzDevice.md#setsyncpin)

***

### reset()

```ts
reset(): Promise<void>;
```

Defined in: [src/device/device.ts:506](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L506)

DEVICE_RESET: device ACKs then reboots; the link will drop.

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`reset`](DepzDevice.md#reset)

***

### enterBootloaderMode()

```ts
enterBootloaderMode(): Promise<void>;
```

Defined in: [src/device/device.ts:515](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L515)

Ask the device to reboot into the resident bootloader and close this
connection. Re-discovery/flash flow lives in the bootloader module
(contract 06).

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`enterBootloaderMode`](DepzDevice.md#enterbootloadermode)

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [src/sensors/bno086/bno086.ts:151](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L151)

Hook for sensor subclasses: route streaming reports here. Return true
when the packet was consumed. Runs after status/matcher/common-report
routing, on the read-pump context.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pkt` | [`PacketEvent`](../interfaces/PacketEvent.md) |

#### Returns

`boolean`

#### Overrides

[`DepzDevice`](DepzDevice.md).[`handleReport`](DepzDevice.md#handlereport)

***

### hardwareReset()

```ts
hardwareReset(timeoutMs?): Promise<void>;
```

Defined in: [src/sensors/bno086/bno086.ts:375](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L375)

Hard-reset the sensor via nRST (0x32). All SHTP state (seq counters,
partial cargos) and cached features restart from zero.

The hub's RPT_STATUS OK ack for the 0x32 command is treated as the reset
confirmation. The SH-2 executable-channel reset-complete (which the BNO08X
SH-2 spec would emit) is only waited for best-effort: older firmware
(≤ v0.95) did **not** emit it (ERRATA E9 in contracts/ERRATA.md, now fixed
in newer firmware); the best-effort wait handles both, so its absence is
*not* an error — the sensor is fully usable without it.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `2000` |

#### Returns

`Promise`\<`void`\>

***

### wake()

```ts
wake(): Promise<void>;
```

Defined in: [src/sensors/bno086/bno086.ts:397](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L397)

Pulse WAKE (PS0): wakes the sensor from sleep, no state loss.

#### Returns

`Promise`\<`void`\>

***

### productId()

```ts
productId(timeoutMs?): Promise<ProductId>;
```

Defined in: [src/sensors/bno086/bno086.ts:416](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L416)

Product ID Request/Response round trip (first responding subsystem).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`ProductId`](../interfaces/ProductId.md)\>

***

### enable()

```ts
enable(
   sensor, 
   hz?, 
opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:435](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L435)

Enable `sensor` at the requested rate via Set Feature (0xFD).

Give either `hz` or `opts.intervalUs`. The hub rounds to its 1 kHz/2^n
grid; with `verify` (default) the granted rate is read back via Get
Feature and a result outside 0.9–2.1× the request emits a console
warning (contract 05 §7 — warn, never throw). Resolves with the
FeatureResponse (null when `verify: false`).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `sensor` | `number` |
| `hz?` | `number` |
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### disable()

```ts
disable(sensor): Promise<void>;
```

Defined in: [src/sensors/bno086/bno086.ts:474](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L474)

Disable `sensor` (Set Feature with interval 0).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `sensor` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getFeature()

```ts
getFeature(sensor, timeoutMs?): Promise<FeatureResponse>;
```

Defined in: [src/sensors/bno086/bno086.ts:480](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L480)

Get Feature Request/Response round trip for `sensor`.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensor` | `number` | `undefined` |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md)\>

***

### enableRotationVector()

```ts
enableRotationVector(hz?, opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:491](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L491)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `100` |
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) | `undefined` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### enableGameRotationVector()

```ts
enableGameRotationVector(hz?, opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:495](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L495)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `100` |
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) | `undefined` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### enableAccelerometer()

```ts
enableAccelerometer(hz?, opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:499](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L499)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `100` |
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) | `undefined` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### enableGyroscope()

```ts
enableGyroscope(hz?, opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:503](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L503)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `100` |
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) | `undefined` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### enableMagnetometer()

```ts
enableMagnetometer(hz?, opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:507](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L507)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `50` |
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) | `undefined` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### enableLinearAcceleration()

```ts
enableLinearAcceleration(hz?, opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:511](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L511)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `100` |
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) | `undefined` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### enableGravity()

```ts
enableGravity(hz?, opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:515](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L515)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `100` |
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) | `undefined` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### enableGyroIntegratedRv()

```ts
enableGyroIntegratedRv(hz?, opts?): Promise<FeatureResponse | null>;
```

Defined in: [src/sensors/bno086/bno086.ts:519](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L519)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `hz` | `number` | `400` |
| `opts?` | [`EnableOptions`](../interfaces/EnableOptions.md) | `undefined` |

#### Returns

`Promise`\<[`FeatureResponse`](../interfaces/FeatureResponse.md) \| `null`\>

***

### onReport()

```ts
onReport(cb, sensors?): () => void;
```

Defined in: [src/sensors/bno086/bno086.ts:529](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L529)

Subscribe to typed sensor reports (read-pump context; do not block).
`sensors` filters by SensorId. Returns an unsubscribe fn.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`r`) => `void` |
| `sensors?` | `number` \| `Iterable`\<`number`, `any`, `any`\> \| `null` |

#### Returns

() => `void`

***

### reports()

```ts
reports(sensors?, maxsize?): StreamQueue<Report>;
```

Defined in: [src/sensors/bno086/bno086.ts:542](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L542)

Async iterator over typed reports — bounded, drop-oldest (contract 07
§3). Subscribes eagerly — reports emitted after this call are never
missed.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensors?` | `number` \| `Iterable`\<`number`, `any`, `any`\> \| `null` | `undefined` |
| `maxsize?` | `number` | `1024` |

#### Returns

[`StreamQueue`](StreamQueue.md)\<[`Report`](../type-aliases/Report.md)\>

***

### tareNow()

```ts
tareNow(axes?, basis?): Promise<void>;
```

Defined in: [src/sensors/bno086/bno086.ts:555](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L555)

Tare the selected axes against `basis` (no response per SH-2).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `axes` | `number` | `TareAxis.All` |
| `basis` | `number` | `TareBasis.RotationVector` |

#### Returns

`Promise`\<`void`\>

***

### persistTare()

```ts
persistTare(): Promise<void>;
```

Defined in: [src/sensors/bno086/bno086.ts:560](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L560)

Persist the current tare into FRS (no response per SH-2).

#### Returns

`Promise`\<`void`\>

***

### setReorientation()

```ts
setReorientation(
   x, 
   y, 
   z, 
w): Promise<void>;
```

Defined in: [src/sensors/bno086/bno086.ts:568](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L568)

Set the runtime reorientation quaternion (Q14 on the wire; all zeros
clears). No response per SH-2.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `x` | `number` |
| `y` | `number` |
| `z` | `number` |
| `w` | `number` |

#### Returns

`Promise`\<`void`\>

***

### setCalibration()

```ts
setCalibration(config?, timeoutMs?): Promise<void>;
```

Defined in: [src/sensors/bno086/bno086.ts:573](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L573)

Configure ME calibration; rejects with Sh2Error on non-zero status.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `config` | \{ `accel?`: `boolean`; `gyro?`: `boolean`; `mag?`: `boolean`; `planar?`: `boolean`; \} | `{}` |
| `config.accel?` | `boolean` | `undefined` |
| `config.gyro?` | `boolean` | `undefined` |
| `config.mag?` | `boolean` | `undefined` |
| `config.planar?` | `boolean` | `undefined` |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<`void`\>

***

### getCalibration()

```ts
getCalibration(timeoutMs?): Promise<CalibrationConfig>;
```

Defined in: [src/sensors/bno086/bno086.ts:589](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L589)

Read back which ME calibrations are running.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`CalibrationConfig`](../interfaces/CalibrationConfig.md)\>

***

### saveDcd()

```ts
saveDcd(timeoutMs?): Promise<void>;
```

Defined in: [src/sensors/bno086/bno086.ts:604](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L604)

Save the dynamic calibration data to flash (DCD Save Now).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<`void`\>

***

### configurePeriodicDcd()

```ts
configurePeriodicDcd(enable): Promise<void>;
```

Defined in: [src/sensors/bno086/bno086.ts:612](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L612)

Enable/disable the hub's periodic DCD autosave (no response).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `enable` | `boolean` |

#### Returns

`Promise`\<`void`\>

***

### frsRead()

```ts
frsRead(recordId, timeoutMs?): Promise<number[]>;
```

Defined in: [src/sensors/bno086/bno086.ts:619](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L619)

Read a whole FRS record; resolves with its 32-bit words.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `recordId` | `number` | `undefined` |
| `timeoutMs` | `number` | `FRS_TIMEOUT_MS` |

#### Returns

`Promise`\<`number`[]\>

***

### frsWrite()

```ts
frsWrite(
   recordId, 
   words, 
timeoutMs?): Promise<void>;
```

Defined in: [src/sensors/bno086/bno086.ts:636](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L636)

Write a whole FRS record (word list); rejects with Sh2Error on failure.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `recordId` | `number` | `undefined` |
| `words` | `Iterable`\<`number`\> | `undefined` |
| `timeoutMs` | `number` | `FRS_TIMEOUT_MS` |

#### Returns

`Promise`\<`void`\>

***

### getMetadata()

```ts
getMetadata(sensor, timeoutMs?): Promise<SensorMetadata>;
```

Defined in: [src/sensors/bno086/bno086.ts:658](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L658)

Read + parse the sensor's FRS metadata record.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensor` | `number` | `undefined` |
| `timeoutMs` | `number` | `FRS_TIMEOUT_MS` |

#### Returns

`Promise`\<[`SensorMetadata`](../interfaces/SensorMetadata.md)\>

***

### getOscillatorType()

```ts
getOscillatorType(timeoutMs?): Promise<OscillatorType>;
```

Defined in: [src/sensors/bno086/bno086.ts:671](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L671)

Get Oscillator Type (command 0x0A). r[0] is the type directly.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`OscillatorType`](../enumerations/OscillatorType.md)\>

***

### clearDcdAndReset()

```ts
clearDcdAndReset(timeoutMs?): Promise<void>;
```

Defined in: [src/sensors/bno086/bno086.ts:681](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L681)

Clear the in-RAM dynamic calibration and reset the sensor (command 0x0B).
There is no command response — the hub resets, so this waits for the
executable reset-complete like hardwareReset().

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `2000` |

#### Returns

`Promise`\<`void`\>

***

### getErrors()

```ts
getErrors(severity?, timeoutMs?): Promise<ErrorRecord[]>;
```

Defined in: [src/sensors/bno086/bno086.ts:708](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L708)

Read the error queue (command 0x01), filtered to `severity` or greater.
Records stream until one with source == 255 (no more).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `severity` | `number` | `0` |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`ErrorRecord`](../interfaces/ErrorRecord.md)[]\>

***

### getCounts()

```ts
getCounts(sensor, timeoutMs?): Promise<Counts>;
```

Defined in: [src/sensors/bno086/bno086.ts:726](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L726)

Read a sensor's event counts (command 0x02). The hub answers with two
responses (responseSeq 0 then 1).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensor` | `number` | `undefined` |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<[`Counts`](../interfaces/Counts.md)\>

***

### clearCounts()

```ts
clearCounts(sensor, timeoutMs?): Promise<void>;
```

Defined in: [src/sensors/bno086/bno086.ts:751](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L751)

Clear a sensor's event counts (command 0x02, subcommand 1).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensor` | `number` | `undefined` |
| `timeoutMs` | `number` | `CONTROL_TIMEOUT_MS` |

#### Returns

`Promise`\<`void`\>

# Class: BusyError

Defined in: [src/errors.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L59)

Device answered ERR_BUSY; the operation may be retried later.

## Extends

- [`StatusError`](StatusError.md)

## Constructors

### Constructor

```ts
new BusyError(cmd): BusyError;
```

Defined in: [src/errors.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L60)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cmd` | `number` |

#### Returns

`BusyError`

#### Overrides

[`StatusError`](StatusError.md).[`constructor`](StatusError.md#constructor)

## Properties

### cmd

```ts
readonly cmd: number;
```

Defined in: [src/errors.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L45)

#### Inherited from

[`StatusError`](StatusError.md).[`cmd`](StatusError.md#cmd)

***

### status

```ts
readonly status: number;
```

Defined in: [src/errors.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L46)

#### Inherited from

[`StatusError`](StatusError.md).[`status`](StatusError.md#status)

***

### statusName

```ts
readonly statusName: string;
```

Defined in: [src/errors.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L47)

#### Inherited from

[`StatusError`](StatusError.md).[`statusName`](StatusError.md#statusname)

***

### stackTraceLimit

```ts
static stackTraceLimit: number;
```

Defined in: node\_modules/@types/node/globals.d.ts:68

The `Error.stackTraceLimit` property specifies the number of stack frames
collected by a stack trace (whether generated by `new Error().stack` or
`Error.captureStackTrace(obj)`).

The default value is `10` but may be set to any valid JavaScript number. Changes
will affect any stack trace captured _after_ the value has been changed.

If set to a non-number value, or set to a negative number, stack traces will
not capture any frames.

#### Inherited from

[`StatusError`](StatusError.md).[`stackTraceLimit`](StatusError.md#stacktracelimit)

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

[`StatusError`](StatusError.md).[`cause`](StatusError.md#cause)

***

### name

```ts
name: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

[`StatusError`](StatusError.md).[`name`](StatusError.md#name)

***

### message

```ts
message: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

[`StatusError`](StatusError.md).[`message`](StatusError.md#message)

***

### stack?

```ts
optional stack?: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

[`StatusError`](StatusError.md).[`stack`](StatusError.md#stack)

## Methods

### captureStackTrace()

```ts
static captureStackTrace(targetObject, constructorOpt?): void;
```

Defined in: node\_modules/@types/node/globals.d.ts:52

Creates a `.stack` property on `targetObject`, which when accessed returns
a string representing the location in the code at which
`Error.captureStackTrace()` was called.

```js
const myObject = {};
Error.captureStackTrace(myObject);
myObject.stack;  // Similar to `new Error().stack`
```

The first line of the trace will be prefixed with
`${myObject.name}: ${myObject.message}`.

The optional `constructorOpt` argument accepts a function. If given, all frames
above `constructorOpt`, including `constructorOpt`, will be omitted from the
generated stack trace.

The `constructorOpt` argument is useful for hiding implementation
details of error generation from the user. For instance:

```js
function a() {
  b();
}

function b() {
  c();
}

function c() {
  // Create an error without stack trace to avoid calculating the stack trace twice.
  const { stackTraceLimit } = Error;
  Error.stackTraceLimit = 0;
  const error = new Error();
  Error.stackTraceLimit = stackTraceLimit;

  // Capture the stack trace above function b
  Error.captureStackTrace(error, b); // Neither function c, nor b is included in the stack trace
  throw error;
}

a();
```

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetObject` | `object` |
| `constructorOpt?` | `Function` |

#### Returns

`void`

#### Inherited from

[`StatusError`](StatusError.md).[`captureStackTrace`](StatusError.md#capturestacktrace)

***

### prepareStackTrace()

```ts
static prepareStackTrace(err, stackTraces): any;
```

Defined in: node\_modules/@types/node/globals.d.ts:56

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `err` | `Error` |
| `stackTraces` | `CallSite`[] |

#### Returns

`any`

#### See

https://v8.dev/docs/stack-trace-api#customizing-stack-traces

#### Inherited from

[`StatusError`](StatusError.md).[`prepareStackTrace`](StatusError.md#preparestacktrace)

# Class: CnhConfig

Defined in: [src/sensors/vl53l8/cnh.ts:68](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L68)

Mirror of VL53LMZ_Motion_Configuration plus the helpers that fill it.
Build with initConfig()/createAggMap(), check size with requiredMemory(),
then pack() the 156-byte struct for cnh_send_config.

## Constructors

### Constructor

```ts
new CnhConfig(): CnhConfig;
```

#### Returns

`CnhConfig`

## Properties

### refBinOffset

```ts
refBinOffset: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:69](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L69)

***

### detectionThreshold

```ts
detectionThreshold: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:70](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L70)

***

### extraNoiseSigma

```ts
extraNoiseSigma: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L71)

***

### nullDenClipValue

```ts
nullDenClipValue: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L72)

***

### memUpdateMode

```ts
memUpdateMode: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L73)

***

### memUpdateChoice

```ts
memUpdateChoice: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:74](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L74)

***

### sumSpan

```ts
sumSpan: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:75](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L75)

***

### featureLength

```ts
featureLength: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:76](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L76)

***

### nbOfAggregates

```ts
nbOfAggregates: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L77)

***

### nbOfTemporalAccumulations

```ts
nbOfTemporalAccumulations: number = 1;
```

Defined in: [src/sensors/vl53l8/cnh.ts:78](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L78)

***

### minNbForGlobalDetection

```ts
minNbForGlobalDetection: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:79](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L79)

***

### globalIndicatorFormat1

```ts
globalIndicatorFormat1: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:80](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L80)

***

### globalIndicatorFormat2

```ts
globalIndicatorFormat2: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:81](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L81)

***

### cnhCfg

```ts
cnhCfg: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:82](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L82)

***

### cnhFlexShift

```ts
cnhFlexShift: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:83](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L83)

***

### spare3

```ts
spare3: number = 0;
```

Defined in: [src/sensors/vl53l8/cnh.ts:84](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L84)

***

### mapId

```ts
mapId: number[];
```

Defined in: [src/sensors/vl53l8/cnh.ts:85](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L85)

***

### indicatorFormat1

```ts
indicatorFormat1: number[];
```

Defined in: [src/sensors/vl53l8/cnh.ts:86](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L86)

***

### indicatorFormat2

```ts
indicatorFormat2: number[];
```

Defined in: [src/sensors/vl53l8/cnh.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L87)

## Methods

### initConfig()

```ts
initConfig(
   startBin, 
   numBins, 
   subSample): void;
```

Defined in: [src/sensors/vl53l8/cnh.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L94)

startBin: first device-histogram bin; numBins: CNH bins;
subSample: bins of the device histogram summed per CNH bin.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `startBin` | `number` |
| `numBins` | `number` |
| `subSample` | `number` |

#### Returns

`void`

***

### createAggMap()

```ts
createAggMap(
   resolution, 
   startX, 
   startY, 
   mergeX, 
   mergeY, 
   cols, 
   rows): void;
```

Defined in: [src/sensors/vl53l8/cnh.ts:123](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L123)

Map device zones to CNH aggregates. resolution: 16 (4x4) or 64 (8x8)
— must match the value passed to setResolution().

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `resolution` | `number` |
| `startX` | `number` |
| `startY` | `number` |
| `mergeX` | `number` |
| `mergeY` | `number` |
| `cols` | `number` |
| `rows` | `number` |

#### Returns

`void`

***

### requiredMemory()

```ts
requiredMemory(): number;
```

Defined in: [src/sensors/vl53l8/cnh.ts:157](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L157)

On-device CNH buffer size in bytes for this config. Throws if the
config is blank or the size exceeds CNH_MAX_DATA_BYTES.

#### Returns

`number`

***

### minMaxDistanceMm()

```ts
minMaxDistanceMm(): [number, number];
```

Defined in: [src/sensors/vl53l8/cnh.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L172)

[min, max] target distance, in mm, fully captured by the histogram.

#### Returns

\[`number`, `number`\]

***

### binCenterMm()

```ts
binCenterMm(binIdx): number;
```

Defined in: [src/sensors/vl53l8/cnh.ts:182](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L182)

Distance (mm) at the centre of CNH histogram bin `binIdx`.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `binIdx` | `number` |

#### Returns

`number`

***

### pack()

```ts
pack(): Uint8Array;
```

Defined in: [src/sensors/vl53l8/cnh.ts:188](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L188)

#### Returns

`Uint8Array`

# Class: CnhConfigError

Defined in: [src/sensors/vl53l8/cnh.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L56)

## Extends

- `Error`

## Constructors

### Constructor

```ts
new CnhConfigError(message?): CnhConfigError;
```

Defined in: [src/sensors/vl53l8/cnh.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L57)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `message?` | `string` |

#### Returns

`CnhConfigError`

#### Overrides

```ts
Error.constructor
```

## Properties

### stackTraceLimit

```ts
static stackTraceLimit: number;
```

Defined in: node\_modules/@types/node/globals.d.ts:68

The `Error.stackTraceLimit` property specifies the number of stack frames
collected by a stack trace (whether generated by `new Error().stack` or
`Error.captureStackTrace(obj)`).

The default value is `10` but may be set to any valid JavaScript number. Changes
will affect any stack trace captured _after_ the value has been changed.

If set to a non-number value, or set to a negative number, stack traces will
not capture any frames.

#### Inherited from

```ts
Error.stackTraceLimit
```

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

```ts
Error.cause
```

***

### name

```ts
name: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

```ts
Error.name
```

***

### message

```ts
message: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

```ts
Error.message
```

***

### stack?

```ts
optional stack?: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

```ts
Error.stack
```

## Methods

### captureStackTrace()

```ts
static captureStackTrace(targetObject, constructorOpt?): void;
```

Defined in: node\_modules/@types/node/globals.d.ts:52

Creates a `.stack` property on `targetObject`, which when accessed returns
a string representing the location in the code at which
`Error.captureStackTrace()` was called.

```js
const myObject = {};
Error.captureStackTrace(myObject);
myObject.stack;  // Similar to `new Error().stack`
```

The first line of the trace will be prefixed with
`${myObject.name}: ${myObject.message}`.

The optional `constructorOpt` argument accepts a function. If given, all frames
above `constructorOpt`, including `constructorOpt`, will be omitted from the
generated stack trace.

The `constructorOpt` argument is useful for hiding implementation
details of error generation from the user. For instance:

```js
function a() {
  b();
}

function b() {
  c();
}

function c() {
  // Create an error without stack trace to avoid calculating the stack trace twice.
  const { stackTraceLimit } = Error;
  Error.stackTraceLimit = 0;
  const error = new Error();
  Error.stackTraceLimit = stackTraceLimit;

  // Capture the stack trace above function b
  Error.captureStackTrace(error, b); // Neither function c, nor b is included in the stack trace
  throw error;
}

a();
```

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetObject` | `object` |
| `constructorOpt?` | `Function` |

#### Returns

`void`

#### Inherited from

```ts
Error.captureStackTrace
```

***

### prepareStackTrace()

```ts
static prepareStackTrace(err, stackTraces): any;
```

Defined in: node\_modules/@types/node/globals.d.ts:56

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `err` | `Error` |
| `stackTraces` | `CallSite`[] |

#### Returns

`any`

#### See

https://v8.dev/docs/stack-trace-api#customizing-stack-traces

#### Inherited from

```ts
Error.prepareStackTrace
```

# Class: DatasetPlayer

Defined in: [src/dataset.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L210)

Timeline player: paced delivery with play/pause/seek/speed — the engine
behind the viewer's playback mode.

## Constructors

### Constructor

```ts
new DatasetPlayer(reader): DatasetPlayer;
```

Defined in: [src/dataset.ts:218](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L218)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `reader` | [`DatasetReader`](DatasetReader.md) |

#### Returns

`DatasetPlayer`

## Properties

### speed

```ts
speed: number = 1.0;
```

Defined in: [src/dataset.ts:216](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L216)

***

### reader

```ts
readonly reader: DatasetReader;
```

Defined in: [src/dataset.ts:218](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L218)

## Accessors

### state

#### Get Signature

```ts
get state(): PlayerState;
```

Defined in: [src/dataset.ts:220](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L220)

##### Returns

[`PlayerState`](../type-aliases/PlayerState.md)

***

### positionUs

#### Get Signature

```ts
get positionUs(): number;
```

Defined in: [src/dataset.ts:225](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L225)

Current position in µs from the first record.

##### Returns

`number`

## Methods

### onRecord()

```ts
onRecord(cb): () => void;
```

Defined in: [src/dataset.ts:231](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L231)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`r`) => `void` |

#### Returns

() => `void`

***

### onState()

```ts
onState(cb): () => void;
```

Defined in: [src/dataset.ts:238](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L238)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`s`) => `void` |

#### Returns

() => `void`

***

### play()

```ts
play(): void;
```

Defined in: [src/dataset.ts:250](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L250)

#### Returns

`void`

***

### pause()

```ts
pause(): void;
```

Defined in: [src/dataset.ts:257](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L257)

#### Returns

`void`

***

### seekUs()

```ts
seekUs(offsetUs): void;
```

Defined in: [src/dataset.ts:264](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L264)

Seek to µs offset from the start; delivery resumes from there.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `offsetUs` | `number` |

#### Returns

`void`

# Class: DatasetReader

Defined in: [src/dataset.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L172)

Parse a `.depzdata` file; records come back merged by host time.

## Constructors

### Constructor

```ts
new DatasetReader(content): DatasetReader;
```

Defined in: [src/dataset.ts:176](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L176)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `content` | `string` |

#### Returns

`DatasetReader`

## Properties

### header

```ts
readonly header: DatasetHeader;
```

Defined in: [src/dataset.ts:173](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L173)

***

### records

```ts
readonly records: DatasetRecord[];
```

Defined in: [src/dataset.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L174)

## Accessors

### devices

#### Get Signature

```ts
get devices(): Record<string, DatasetDeviceMeta>;
```

Defined in: [src/dataset.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L194)

##### Returns

`Record`\<`string`, [`DatasetDeviceMeta`](../interfaces/DatasetDeviceMeta.md)\>

***

### durationUs

#### Get Signature

```ts
get durationUs(): number;
```

Defined in: [src/dataset.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L198)

##### Returns

`number`

# Class: DatasetRecorder

Defined in: [src/dataset.ts:76](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L76)

Hooks live devices into a DatasetWriter on one shared host timeline.
Call `await recorder.add(device)` for each device (runs syncTime), then
`start()`; `stop()` unhooks; `dump()` returns the file content.

## Constructors

### Constructor

```ts
new DatasetRecorder(opts?): DatasetRecorder;
```

Defined in: [src/dataset.ts:86](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L86)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `opts` | \{ `note?`: `string`; `vl53l8Layers?`: `boolean`; \} |
| `opts.note?` | `string` |
| `opts.vl53l8Layers?` | `boolean` |

#### Returns

`DatasetRecorder`

## Accessors

### recordsWritten

#### Get Signature

```ts
get recordsWritten(): number;
```

Defined in: [src/dataset.ts:161](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L161)

##### Returns

`number`

## Methods

### add()

```ts
add(
   device, 
   deviceId?, 
syncSamples?): Promise<string>;
```

Defined in: [src/dataset.ts:88](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L88)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `device` | [`DepzDevice`](DepzDevice.md) | `undefined` |
| `deviceId?` | `string` | `undefined` |
| `syncSamples?` | `number` | `5` |

#### Returns

`Promise`\<`string`\>

***

### start()

```ts
start(): void;
```

Defined in: [src/dataset.ts:107](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L107)

#### Returns

`void`

***

### stop()

```ts
stop(): void;
```

Defined in: [src/dataset.ts:154](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L154)

#### Returns

`void`

***

### dump()

```ts
dump(): string;
```

Defined in: [src/dataset.ts:165](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L165)

#### Returns

`string`

# Class: DatasetWriter

Defined in: [src/dataset.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L37)

Accumulates a dataset in memory; `dump()` gives the JSONL file content.

## Constructors

### Constructor

```ts
new DatasetWriter(devices, note?): DatasetWriter;
```

Defined in: [src/dataset.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L42)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `devices` | `Record`\<`string`, [`DatasetDeviceMeta`](../interfaces/DatasetDeviceMeta.md)\> |
| `note?` | `string` |

#### Returns

`DatasetWriter`

## Properties

### recordsWritten

```ts
recordsWritten: number = 0;
```

Defined in: [src/dataset.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L40)

## Methods

### setDeviceMeta()

```ts
setDeviceMeta(deviceId, meta): void;
```

Defined in: [src/dataset.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L57)

Register (or replace) a device's metadata after construction. Lets a
recorder that adds devices mid-capture keep the header's `devices` map
complete — the header is (re)serialized lazily in `dump()`.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `deviceId` | `string` |
| `meta` | [`DatasetDeviceMeta`](../interfaces/DatasetDeviceMeta.md) |

#### Returns

`void`

***

### write()

```ts
write(
   deviceId, 
   tHostUs, 
   kind, 
   value): void;
```

Defined in: [src/dataset.ts:61](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L61)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `deviceId` | `string` |
| `tHostUs` | `number` |
| `kind` | `string` |
| `value` | `Record`\<`string`, `unknown`\> |

#### Returns

`void`

***

### dump()

```ts
dump(): string;
```

Defined in: [src/dataset.ts:66](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L66)

#### Returns

`string`

# Class: DepzDevice

Defined in: [src/device/device.ts:177](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L177)

Connection to one DEPZ device in application mode. Construct over any
`SerialTransport`, then `await open()` before use.

## Extended by

- [`Vl53l4Cd`](Vl53l4Cd-1.md)
- [`Bno086`](Bno086.md)
- [`Sr04`](Sr04.md)
- [`Vl53l8Cx`](Vl53l8Cx-1.md)

## Constructors

### Constructor

```ts
new DepzDevice(transport, opts?): DepzDevice;
```

Defined in: [src/device/device.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L189)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | [`SerialTransport`](../interfaces/SerialTransport.md) |
| `opts?` | [`DeviceOptions`](../interfaces/DeviceOptions.md) |

#### Returns

`DepzDevice`

## Properties

### timeoutMs

```ts
timeoutMs: number;
```

Defined in: [src/device/device.ts:178](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L178)

***

### link

```ts
protected readonly link: DepzLink;
```

Defined in: [src/device/device.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L180)

## Accessors

### stats

#### Get Signature

```ts
get stats(): LinkStats;
```

Defined in: [src/device/device.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L210)

##### Returns

[`LinkStats`](../interfaces/LinkStats.md)

***

### timeSync

#### Get Signature

```ts
get timeSync(): TimeSync | null;
```

Defined in: [src/device/device.ts:474](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L474)

##### Returns

[`TimeSync`](../interfaces/TimeSync.md) \| `null`

## Methods

### open()

```ts
open(): Promise<void>;
```

Defined in: [src/device/device.ts:200](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L200)

Open the transport and start the read pump.

#### Returns

`Promise`\<`void`\>

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/device/device.ts:205](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L205)

#### Returns

`Promise`\<`void`\>

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [src/device/device.ts:324](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L324)

Hook for sensor subclasses: route streaming reports here. Return true
when the packet was consumed. Runs after status/matcher/common-report
routing, on the read-pump context.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pkt` | [`PacketEvent`](../interfaces/PacketEvent.md) |

#### Returns

`boolean`

***

### registerStream()

```ts
protected registerStream(stream): () => void;
```

Defined in: [src/device/device.ts:330](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L330)

Register a stream to be closed on device teardown.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `stream` | \{ `close`: `void`; \} |
| `stream.close` |

#### Returns

() => `void`

***

### onEvent()

```ts
onEvent(cb): () => void;
```

Defined in: [src/device/device.ts:343](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L343)

Subscribe to unsolicited/diagnostic events (read-pump context; do not
block). Returns an unsubscribe function.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`ev`) => `void` |

#### Returns

() => `void`

***

### emitEvent()

```ts
protected emitEvent(event): void;
```

Defined in: [src/device/device.ts:350](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L350)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `event` | [`DeviceEvent`](../type-aliases/DeviceEvent.md) |

#### Returns

`void`

***

### request()

```ts
request<T>(
   cmd, 
   payload?, 
opts?): Promise<T>;
```

Defined in: [src/device/device.ts:366](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L366)

Send `cmd` and wait for its correlated completion (contract 02 §1).

Exactly one of the completion paths must be configured:
- `okCompletes` — RPT_STATUS(cmd, OK) resolves with undefined;
- `matcher` — first packet for which the matcher returns non-`NO_MATCH`
  resolves with the matcher's return value.
Non-OK RPT_STATUS echoing `cmd` always rejects (BusyError for ERR_BUSY,
StatusError otherwise). One in-flight request per opcode.

#### Type Parameters

| Type Parameter | Default type |
| ------ | ------ |
| `T` | `void` |

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cmd` | `number` |
| `payload` | `Uint8Array` |
| `opts` | [`RequestOptions`](../interfaces/RequestOptions.md)\<`T`\> |

#### Returns

`Promise`\<`T`\>

***

### expectReport()

```ts
static expectReport<T>(reportId, unpack): Matcher<T>;
```

Defined in: [src/device/device.ts:409](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L409)

Matcher for a typed report identified by its report ID alone.

#### Type Parameters

| Type Parameter |
| ------ |
| `T` |

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `reportId` | `number` |
| `unpack` | (`payload`) => `T` |

#### Returns

[`Matcher`](../type-aliases/Matcher.md)\<`T`\>

***

### expectText()

```ts
static expectText(requestCmd): Matcher<string>;
```

Defined in: [src/device/device.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L414)

Matcher for RPT_TEXT echoing `requestCmd`.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `requestCmd` | `number` |

#### Returns

[`Matcher`](../type-aliases/Matcher.md)\<`string`\>

***

### getDeviceName()

```ts
getDeviceName(): Promise<string>;
```

Defined in: [src/device/device.ts:425](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L425)

#### Returns

`Promise`\<`string`\>

***

### getSoftwareName()

```ts
getSoftwareName(): Promise<string>;
```

Defined in: [src/device/device.ts:431](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L431)

#### Returns

`Promise`\<`string`\>

***

### getSerialNumber()

```ts
getSerialNumber(): Promise<string>;
```

Defined in: [src/device/device.ts:437](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L437)

#### Returns

`Promise`\<`string`\>

***

### identify()

```ts
identify(): Promise<Identity>;
```

Defined in: [src/device/device.ts:444](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L444)

Classify the running firmware (contract 02 §4).

#### Returns

`Promise`\<[`Identity`](../interfaces/Identity.md)\>

***

### readMcuTemperature()

```ts
readMcuTemperature(): Promise<number>;
```

Defined in: [src/device/device.ts:449](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L449)

Last cached MCU temperature in °C (device refreshes ~2 Hz).

#### Returns

`Promise`\<`number`\>

***

### syncTime()

```ts
syncTime(samples?): Promise<TimeSync>;
```

Defined in: [src/device/device.ts:457](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L457)

NTP-style sync; keeps the lowest-RTT sample (contract 02 §5).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `samples` | `number` | `5` |

#### Returns

`Promise`\<[`TimeSync`](../interfaces/TimeSync.md)\>

***

### toHostTimeUs()

```ts
toHostTimeUs(deviceTsUs): bigint;
```

Defined in: [src/device/device.ts:479](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L479)

Device µs → host monotonic µs (requires a prior `syncTime`).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `deviceTsUs` | `bigint` |

#### Returns

`bigint`

***

### getReportPayloadCrc()

```ts
getReportPayloadCrc(): Promise<CrcType>;
```

Defined in: [src/device/device.ts:484](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L484)

#### Returns

`Promise`\<[`CrcType`](../enumerations/CrcType.md)\>

***

### setReportPayloadCrc()

```ts
setReportPayloadCrc(crcType): Promise<void>;
```

Defined in: [src/device/device.ts:491](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L491)

Set the device→host payload CRC mode (host→device is per-packet).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `crcType` | [`CrcType`](../enumerations/CrcType.md) |

#### Returns

`Promise`\<`void`\>

***

### getSyncPin()

```ts
getSyncPin(pin): Promise<SyncPinConfig>;
```

Defined in: [src/device/device.ts:495](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L495)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pin` | `number` |

#### Returns

`Promise`\<[`SyncPinConfig`](../interfaces/SyncPinConfig.md)\>

***

### setSyncPin()

```ts
setSyncPin(config): Promise<void>;
```

Defined in: [src/device/device.ts:501](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L501)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `config` | [`SyncPinConfig`](../interfaces/SyncPinConfig.md) |

#### Returns

`Promise`\<`void`\>

***

### reset()

```ts
reset(): Promise<void>;
```

Defined in: [src/device/device.ts:506](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L506)

DEVICE_RESET: device ACKs then reboots; the link will drop.

#### Returns

`Promise`\<`void`\>

***

### enterBootloaderMode()

```ts
enterBootloaderMode(): Promise<void>;
```

Defined in: [src/device/device.ts:515](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L515)

Ask the device to reboot into the resident bootloader and close this
connection. Re-discovery/flash flow lives in the bootloader module
(contract 06).

#### Returns

`Promise`\<`void`\>

# Class: DepzError

Defined in: [src/errors.ts:7](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L7)

Base class for all SDK errors.

## Extends

- `Error`

## Extended by

- [`DepzTimeoutError`](DepzTimeoutError.md)
- [`StatusError`](StatusError.md)
- [`DeviceLostError`](DeviceLostError.md)
- [`LinkClosedError`](LinkClosedError.md)
- [`NoDepzDeviceError`](NoDepzDeviceError.md)
- [`Sh2Error`](Sh2Error.md)

## Constructors

### Constructor

```ts
new DepzError(message?): DepzError;
```

Defined in: [src/errors.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L8)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `message?` | `string` |

#### Returns

`DepzError`

#### Overrides

```ts
Error.constructor
```

## Properties

### stackTraceLimit

```ts
static stackTraceLimit: number;
```

Defined in: node\_modules/@types/node/globals.d.ts:68

The `Error.stackTraceLimit` property specifies the number of stack frames
collected by a stack trace (whether generated by `new Error().stack` or
`Error.captureStackTrace(obj)`).

The default value is `10` but may be set to any valid JavaScript number. Changes
will affect any stack trace captured _after_ the value has been changed.

If set to a non-number value, or set to a negative number, stack traces will
not capture any frames.

#### Inherited from

```ts
Error.stackTraceLimit
```

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

```ts
Error.cause
```

***

### name

```ts
name: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

```ts
Error.name
```

***

### message

```ts
message: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

```ts
Error.message
```

***

### stack?

```ts
optional stack?: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

```ts
Error.stack
```

## Methods

### captureStackTrace()

```ts
static captureStackTrace(targetObject, constructorOpt?): void;
```

Defined in: node\_modules/@types/node/globals.d.ts:52

Creates a `.stack` property on `targetObject`, which when accessed returns
a string representing the location in the code at which
`Error.captureStackTrace()` was called.

```js
const myObject = {};
Error.captureStackTrace(myObject);
myObject.stack;  // Similar to `new Error().stack`
```

The first line of the trace will be prefixed with
`${myObject.name}: ${myObject.message}`.

The optional `constructorOpt` argument accepts a function. If given, all frames
above `constructorOpt`, including `constructorOpt`, will be omitted from the
generated stack trace.

The `constructorOpt` argument is useful for hiding implementation
details of error generation from the user. For instance:

```js
function a() {
  b();
}

function b() {
  c();
}

function c() {
  // Create an error without stack trace to avoid calculating the stack trace twice.
  const { stackTraceLimit } = Error;
  Error.stackTraceLimit = 0;
  const error = new Error();
  Error.stackTraceLimit = stackTraceLimit;

  // Capture the stack trace above function b
  Error.captureStackTrace(error, b); // Neither function c, nor b is included in the stack trace
  throw error;
}

a();
```

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetObject` | `object` |
| `constructorOpt?` | `Function` |

#### Returns

`void`

#### Inherited from

```ts
Error.captureStackTrace
```

***

### prepareStackTrace()

```ts
static prepareStackTrace(err, stackTraces): any;
```

Defined in: node\_modules/@types/node/globals.d.ts:56

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `err` | `Error` |
| `stackTraces` | `CallSite`[] |

#### Returns

`any`

#### See

https://v8.dev/docs/stack-trace-api#customizing-stack-traces

#### Inherited from

```ts
Error.prepareStackTrace
```

# Class: DepzLink

Defined in: [src/device/link.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L32)

## Constructors

### Constructor

```ts
new DepzLink(transport, opts?): DepzLink;
```

Defined in: [src/device/link.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L56)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | [`SerialTransport`](../interfaces/SerialTransport.md) |
| `opts?` | \{ `txCrcType?`: [`CrcType`](../enumerations/CrcType.md); \} |
| `opts.txCrcType?` | [`CrcType`](../enumerations/CrcType.md) |

#### Returns

`DepzLink`

## Properties

### stats

```ts
readonly stats: LinkStats;
```

Defined in: [src/device/link.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L33)

## Methods

### start()

```ts
start(): void;
```

Defined in: [src/device/link.ts:66](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L66)

Begin the read pump. Must be called exactly once, after the transport
is open. Ends (and fires `onClose`) when the transport's readable
iterator finishes or the transport reports a disconnect.

#### Returns

`void`

***

### send()

```ts
send(cmd, payload?): Promise<void>;
```

Defined in: [src/device/link.ts:108](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L108)

Frame and write one packet with the next TX seq (wraps at 0xFF).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cmd` | `number` |
| `payload` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

***

### onPacket()

```ts
onPacket(cb): () => void;
```

Defined in: [src/device/link.ts:117](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L117)

Subscribe to decoded packets. Returns an unsubscribe function.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`pkt`) => `void` |

#### Returns

() => `void`

***

### onParserEvent()

```ts
onParserEvent(cb): () => void;
```

Defined in: [src/device/link.ts:128](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L128)

Subscribe to all parser events, including CRC errors and trash
(diagnostics; the device layer maps these to `DeviceEvent`s).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`ev`) => `void` |

#### Returns

() => `void`

***

### onClose()

```ts
onClose(cb): () => void;
```

Defined in: [src/device/link.ts:136](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L136)

Subscribe to link teardown (fires once). Returns an unsubscribe.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | () => `void` |

#### Returns

() => `void`

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/device/link.ts:150](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L150)

#### Returns

`Promise`\<`void`\>

# Class: DepzTimeoutError

Defined in: [src/errors.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L19)

A request got no matching reply within the timeout.

## Extends

- [`DepzError`](DepzError.md)

## Constructors

### Constructor

```ts
new DepzTimeoutError(cmd, timeoutMs): DepzTimeoutError;
```

Defined in: [src/errors.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L23)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cmd` | `number` |
| `timeoutMs` | `number` |

#### Returns

`DepzTimeoutError`

#### Overrides

[`DepzError`](DepzError.md).[`constructor`](DepzError.md#constructor)

## Properties

### cmd

```ts
readonly cmd: number;
```

Defined in: [src/errors.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L20)

***

### timeoutMs

```ts
readonly timeoutMs: number;
```

Defined in: [src/errors.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L21)

***

### stackTraceLimit

```ts
static stackTraceLimit: number;
```

Defined in: node\_modules/@types/node/globals.d.ts:68

The `Error.stackTraceLimit` property specifies the number of stack frames
collected by a stack trace (whether generated by `new Error().stack` or
`Error.captureStackTrace(obj)`).

The default value is `10` but may be set to any valid JavaScript number. Changes
will affect any stack trace captured _after_ the value has been changed.

If set to a non-number value, or set to a negative number, stack traces will
not capture any frames.

#### Inherited from

[`DepzError`](DepzError.md).[`stackTraceLimit`](DepzError.md#stacktracelimit)

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

[`DepzError`](DepzError.md).[`cause`](DepzError.md#cause)

***

### name

```ts
name: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

[`DepzError`](DepzError.md).[`name`](DepzError.md#name)

***

### message

```ts
message: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

[`DepzError`](DepzError.md).[`message`](DepzError.md#message)

***

### stack?

```ts
optional stack?: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

[`DepzError`](DepzError.md).[`stack`](DepzError.md#stack)

## Methods

### captureStackTrace()

```ts
static captureStackTrace(targetObject, constructorOpt?): void;
```

Defined in: node\_modules/@types/node/globals.d.ts:52

Creates a `.stack` property on `targetObject`, which when accessed returns
a string representing the location in the code at which
`Error.captureStackTrace()` was called.

```js
const myObject = {};
Error.captureStackTrace(myObject);
myObject.stack;  // Similar to `new Error().stack`
```

The first line of the trace will be prefixed with
`${myObject.name}: ${myObject.message}`.

The optional `constructorOpt` argument accepts a function. If given, all frames
above `constructorOpt`, including `constructorOpt`, will be omitted from the
generated stack trace.

The `constructorOpt` argument is useful for hiding implementation
details of error generation from the user. For instance:

```js
function a() {
  b();
}

function b() {
  c();
}

function c() {
  // Create an error without stack trace to avoid calculating the stack trace twice.
  const { stackTraceLimit } = Error;
  Error.stackTraceLimit = 0;
  const error = new Error();
  Error.stackTraceLimit = stackTraceLimit;

  // Capture the stack trace above function b
  Error.captureStackTrace(error, b); // Neither function c, nor b is included in the stack trace
  throw error;
}

a();
```

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetObject` | `object` |
| `constructorOpt?` | `Function` |

#### Returns

`void`

#### Inherited from

[`DepzError`](DepzError.md).[`captureStackTrace`](DepzError.md#capturestacktrace)

***

### prepareStackTrace()

```ts
static prepareStackTrace(err, stackTraces): any;
```

Defined in: node\_modules/@types/node/globals.d.ts:56

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `err` | `Error` |
| `stackTraces` | `CallSite`[] |

#### Returns

`any`

#### See

https://v8.dev/docs/stack-trace-api#customizing-stack-traces

#### Inherited from

[`DepzError`](DepzError.md).[`prepareStackTrace`](DepzError.md#preparestacktrace)

# Class: DeviceLostError

Defined in: [src/errors.ts:66](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L66)

The serial link dropped (unplug, reboot) while in use.

## Extends

- [`DepzError`](DepzError.md)

## Constructors

### Constructor

```ts
new DeviceLostError(message?): DeviceLostError;
```

Defined in: [src/errors.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L8)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `message?` | `string` |

#### Returns

`DeviceLostError`

#### Inherited from

[`DepzError`](DepzError.md).[`constructor`](DepzError.md#constructor)

## Properties

### stackTraceLimit

```ts
static stackTraceLimit: number;
```

Defined in: node\_modules/@types/node/globals.d.ts:68

The `Error.stackTraceLimit` property specifies the number of stack frames
collected by a stack trace (whether generated by `new Error().stack` or
`Error.captureStackTrace(obj)`).

The default value is `10` but may be set to any valid JavaScript number. Changes
will affect any stack trace captured _after_ the value has been changed.

If set to a non-number value, or set to a negative number, stack traces will
not capture any frames.

#### Inherited from

[`DepzError`](DepzError.md).[`stackTraceLimit`](DepzError.md#stacktracelimit)

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

[`DepzError`](DepzError.md).[`cause`](DepzError.md#cause)

***

### name

```ts
name: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

[`DepzError`](DepzError.md).[`name`](DepzError.md#name)

***

### message

```ts
message: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

[`DepzError`](DepzError.md).[`message`](DepzError.md#message)

***

### stack?

```ts
optional stack?: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

[`DepzError`](DepzError.md).[`stack`](DepzError.md#stack)

## Methods

### captureStackTrace()

```ts
static captureStackTrace(targetObject, constructorOpt?): void;
```

Defined in: node\_modules/@types/node/globals.d.ts:52

Creates a `.stack` property on `targetObject`, which when accessed returns
a string representing the location in the code at which
`Error.captureStackTrace()` was called.

```js
const myObject = {};
Error.captureStackTrace(myObject);
myObject.stack;  // Similar to `new Error().stack`
```

The first line of the trace will be prefixed with
`${myObject.name}: ${myObject.message}`.

The optional `constructorOpt` argument accepts a function. If given, all frames
above `constructorOpt`, including `constructorOpt`, will be omitted from the
generated stack trace.

The `constructorOpt` argument is useful for hiding implementation
details of error generation from the user. For instance:

```js
function a() {
  b();
}

function b() {
  c();
}

function c() {
  // Create an error without stack trace to avoid calculating the stack trace twice.
  const { stackTraceLimit } = Error;
  Error.stackTraceLimit = 0;
  const error = new Error();
  Error.stackTraceLimit = stackTraceLimit;

  // Capture the stack trace above function b
  Error.captureStackTrace(error, b); // Neither function c, nor b is included in the stack trace
  throw error;
}

a();
```

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetObject` | `object` |
| `constructorOpt?` | `Function` |

#### Returns

`void`

#### Inherited from

[`DepzError`](DepzError.md).[`captureStackTrace`](DepzError.md#capturestacktrace)

***

### prepareStackTrace()

```ts
static prepareStackTrace(err, stackTraces): any;
```

Defined in: node\_modules/@types/node/globals.d.ts:56

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `err` | `Error` |
| `stackTraces` | `CallSite`[] |

#### Returns

`any`

#### See

https://v8.dev/docs/stack-trace-api#customizing-stack-traces

#### Inherited from

[`DepzError`](DepzError.md).[`prepareStackTrace`](DepzError.md#preparestacktrace)

# Class: FrameReassembler

Defined in: [src/protocol/vl53l8.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L93)

Rebuilds full sensor frames from chunked RPT_VL53_FRAME reports.

Rules (contract 04): reset on offset==0; chunks must be contiguous —
a gap discards the frame in progress; a frame completes when the
accumulated bytes equal `fullSize`.

## Constructors

### Constructor

```ts
new FrameReassembler(): FrameReassembler;
```

#### Returns

`FrameReassembler`

## Properties

### completed

```ts
completed: number = 0;
```

Defined in: [src/protocol/vl53l8.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L94)

***

### discarded

```ts
discarded: number = 0;
```

Defined in: [src/protocol/vl53l8.ts:95](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L95)

## Methods

### feed()

```ts
feed(chunk): 
  | {
  timestampUs: bigint;
  frame: Uint8Array;
}
  | null;
```

Defined in: [src/protocol/vl53l8.ts:102](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L102)

Returns `{ timestampUs, frame }` when a frame completes.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `chunk` | [`FrameChunk`](../interfaces/FrameChunk.md) |

#### Returns

  \| \{
  `timestampUs`: `bigint`;
  `frame`: `Uint8Array`;
\}
  \| `null`

# Class: FrsReadSession

Defined in: [src/sensors/bno086/sh2.ts:518](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L518)

Multi-packet FRS read state machine (pure — feed parsed responses).

Usage: send `request()`, then `feed()` every 0xF3 for this record until it
returns true; `words` holds the record. Error statuses throw.

## Constructors

### Constructor

```ts
new FrsReadSession(frsType): FrsReadSession;
```

Defined in: [src/sensors/bno086/sh2.ts:523](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L523)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `frsType` | `number` |

#### Returns

`FrsReadSession`

## Properties

### frsType

```ts
readonly frsType: number;
```

Defined in: [src/sensors/bno086/sh2.ts:519](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L519)

***

### words

```ts
words: number[] = [];
```

Defined in: [src/sensors/bno086/sh2.ts:520](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L520)

***

### done

```ts
done: boolean = false;
```

Defined in: [src/sensors/bno086/sh2.ts:521](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L521)

## Methods

### request()

```ts
request(): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:527](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L527)

#### Returns

`Uint8Array`

***

### feed()

```ts
feed(resp): boolean;
```

Defined in: [src/sensors/bno086/sh2.ts:531](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L531)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `resp` | [`FrsReadResponse`](../interfaces/FrsReadResponse.md) |

#### Returns

`boolean`

# Class: FrsWriteSession

Defined in: [src/sensors/bno086/sh2.ts:566](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L566)

Multi-packet FRS write state machine (pure).

Usage: send `request()`; then for every 0xF5 call `feed()` — it returns
the next Write Data payload to send, or null; `done` flips on
WriteCompleted. Error statuses throw.

## Constructors

### Constructor

```ts
new FrsWriteSession(frsType, words): FrsWriteSession;
```

Defined in: [src/sensors/bno086/sh2.ts:572](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L572)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `frsType` | `number` |
| `words` | `number`[] |

#### Returns

`FrsWriteSession`

## Properties

### frsType

```ts
readonly frsType: number;
```

Defined in: [src/sensors/bno086/sh2.ts:567](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L567)

***

### words

```ts
readonly words: number[];
```

Defined in: [src/sensors/bno086/sh2.ts:568](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L568)

***

### offset

```ts
offset: number = 0;
```

Defined in: [src/sensors/bno086/sh2.ts:569](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L569)

***

### done

```ts
done: boolean = false;
```

Defined in: [src/sensors/bno086/sh2.ts:570](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L570)

## Methods

### request()

```ts
request(): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:577](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L577)

#### Returns

`Uint8Array`

***

### feed()

```ts
feed(resp): Uint8Array<ArrayBufferLike> | null;
```

Defined in: [src/sensors/bno086/sh2.ts:589](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L589)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `resp` | [`FrsWriteResponse`](../interfaces/FrsWriteResponse.md) |

#### Returns

`Uint8Array`\<`ArrayBufferLike`\> \| `null`

# Class: FwDepzError

Defined in: [src/protocol/fwdepz.ts:12](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/fwdepz.ts#L12)

## Extends

- `Error`

## Constructors

### Constructor

```ts
new FwDepzError(message?): FwDepzError;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1082

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `message?` | `string` |

#### Returns

`FwDepzError`

#### Inherited from

```ts
Error.constructor
```

### Constructor

```ts
new FwDepzError(message?, options?): FwDepzError;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1082

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `message?` | `string` |
| `options?` | `ErrorOptions` |

#### Returns

`FwDepzError`

#### Inherited from

```ts
Error.constructor
```

## Properties

### stackTraceLimit

```ts
static stackTraceLimit: number;
```

Defined in: node\_modules/@types/node/globals.d.ts:68

The `Error.stackTraceLimit` property specifies the number of stack frames
collected by a stack trace (whether generated by `new Error().stack` or
`Error.captureStackTrace(obj)`).

The default value is `10` but may be set to any valid JavaScript number. Changes
will affect any stack trace captured _after_ the value has been changed.

If set to a non-number value, or set to a negative number, stack traces will
not capture any frames.

#### Inherited from

```ts
Error.stackTraceLimit
```

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

```ts
Error.cause
```

***

### name

```ts
name: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

```ts
Error.name
```

***

### message

```ts
message: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

```ts
Error.message
```

***

### stack?

```ts
optional stack?: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

```ts
Error.stack
```

## Methods

### captureStackTrace()

```ts
static captureStackTrace(targetObject, constructorOpt?): void;
```

Defined in: node\_modules/@types/node/globals.d.ts:52

Creates a `.stack` property on `targetObject`, which when accessed returns
a string representing the location in the code at which
`Error.captureStackTrace()` was called.

```js
const myObject = {};
Error.captureStackTrace(myObject);
myObject.stack;  // Similar to `new Error().stack`
```

The first line of the trace will be prefixed with
`${myObject.name}: ${myObject.message}`.

The optional `constructorOpt` argument accepts a function. If given, all frames
above `constructorOpt`, including `constructorOpt`, will be omitted from the
generated stack trace.

The `constructorOpt` argument is useful for hiding implementation
details of error generation from the user. For instance:

```js
function a() {
  b();
}

function b() {
  c();
}

function c() {
  // Create an error without stack trace to avoid calculating the stack trace twice.
  const { stackTraceLimit } = Error;
  Error.stackTraceLimit = 0;
  const error = new Error();
  Error.stackTraceLimit = stackTraceLimit;

  // Capture the stack trace above function b
  Error.captureStackTrace(error, b); // Neither function c, nor b is included in the stack trace
  throw error;
}

a();
```

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetObject` | `object` |
| `constructorOpt?` | `Function` |

#### Returns

`void`

#### Inherited from

```ts
Error.captureStackTrace
```

***

### prepareStackTrace()

```ts
static prepareStackTrace(err, stackTraces): any;
```

Defined in: node\_modules/@types/node/globals.d.ts:56

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `err` | `Error` |
| `stackTraces` | `CallSite`[] |

#### Returns

`any`

#### See

https://v8.dev/docs/stack-trace-api#customizing-stack-traces

#### Inherited from

```ts
Error.prepareStackTrace
```

# Class: LinkClosedError

Defined in: [src/errors.ts:69](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L69)

Operation attempted on a closed link/device.

## Extends

- [`DepzError`](DepzError.md)

## Constructors

### Constructor

```ts
new LinkClosedError(message?): LinkClosedError;
```

Defined in: [src/errors.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L8)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `message?` | `string` |

#### Returns

`LinkClosedError`

#### Inherited from

[`DepzError`](DepzError.md).[`constructor`](DepzError.md#constructor)

## Properties

### stackTraceLimit

```ts
static stackTraceLimit: number;
```

Defined in: node\_modules/@types/node/globals.d.ts:68

The `Error.stackTraceLimit` property specifies the number of stack frames
collected by a stack trace (whether generated by `new Error().stack` or
`Error.captureStackTrace(obj)`).

The default value is `10` but may be set to any valid JavaScript number. Changes
will affect any stack trace captured _after_ the value has been changed.

If set to a non-number value, or set to a negative number, stack traces will
not capture any frames.

#### Inherited from

[`DepzError`](DepzError.md).[`stackTraceLimit`](DepzError.md#stacktracelimit)

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

[`DepzError`](DepzError.md).[`cause`](DepzError.md#cause)

***

### name

```ts
name: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

[`DepzError`](DepzError.md).[`name`](DepzError.md#name)

***

### message

```ts
message: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

[`DepzError`](DepzError.md).[`message`](DepzError.md#message)

***

### stack?

```ts
optional stack?: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

[`DepzError`](DepzError.md).[`stack`](DepzError.md#stack)

## Methods

### captureStackTrace()

```ts
static captureStackTrace(targetObject, constructorOpt?): void;
```

Defined in: node\_modules/@types/node/globals.d.ts:52

Creates a `.stack` property on `targetObject`, which when accessed returns
a string representing the location in the code at which
`Error.captureStackTrace()` was called.

```js
const myObject = {};
Error.captureStackTrace(myObject);
myObject.stack;  // Similar to `new Error().stack`
```

The first line of the trace will be prefixed with
`${myObject.name}: ${myObject.message}`.

The optional `constructorOpt` argument accepts a function. If given, all frames
above `constructorOpt`, including `constructorOpt`, will be omitted from the
generated stack trace.

The `constructorOpt` argument is useful for hiding implementation
details of error generation from the user. For instance:

```js
function a() {
  b();
}

function b() {
  c();
}

function c() {
  // Create an error without stack trace to avoid calculating the stack trace twice.
  const { stackTraceLimit } = Error;
  Error.stackTraceLimit = 0;
  const error = new Error();
  Error.stackTraceLimit = stackTraceLimit;

  // Capture the stack trace above function b
  Error.captureStackTrace(error, b); // Neither function c, nor b is included in the stack trace
  throw error;
}

a();
```

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetObject` | `object` |
| `constructorOpt?` | `Function` |

#### Returns

`void`

#### Inherited from

[`DepzError`](DepzError.md).[`captureStackTrace`](DepzError.md#capturestacktrace)

***

### prepareStackTrace()

```ts
static prepareStackTrace(err, stackTraces): any;
```

Defined in: node\_modules/@types/node/globals.d.ts:56

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `err` | `Error` |
| `stackTraces` | `CallSite`[] |

#### Returns

`any`

#### See

https://v8.dev/docs/stack-trace-api#customizing-stack-traces

#### Inherited from

[`DepzError`](DepzError.md).[`prepareStackTrace`](DepzError.md#preparestacktrace)

# Class: LoopbackTransport

Defined in: [src/transport/loopback.ts:5](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/loopback.ts#L5)

## Implements

- [`SerialTransport`](../interfaces/SerialTransport.md)

## Constructors

### Constructor

```ts
new LoopbackTransport(): LoopbackTransport;
```

#### Returns

`LoopbackTransport`

## Properties

### info

```ts
readonly info: SerialTransportInfo;
```

Defined in: [src/transport/loopback.ts:6](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/loopback.ts#L6)

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`info`](../interfaces/SerialTransport.md#info)

***

### peer

```ts
peer: LoopbackTransport | null = null;
```

Defined in: [src/transport/loopback.ts:7](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/loopback.ts#L7)

## Methods

### pair()

```ts
static pair(): [LoopbackTransport, LoopbackTransport];
```

Defined in: [src/transport/loopback.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/loopback.ts#L14)

#### Returns

\[`LoopbackTransport`, `LoopbackTransport`\]

***

### open()

```ts
open(): Promise<void>;
```

Defined in: [src/transport/loopback.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/loopback.ts#L22)

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`open`](../interfaces/SerialTransport.md#open)

***

### write()

```ts
write(data): Promise<void>;
```

Defined in: [src/transport/loopback.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/loopback.ts#L24)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`write`](../interfaces/SerialTransport.md#write)

***

### readable()

```ts
readable(): AsyncIterableIterator<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/transport/loopback.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/loopback.ts#L34)

Raw chunks as they arrive; ends on close/disconnect.

#### Returns

`AsyncIterableIterator`\<`Uint8Array`\<`ArrayBufferLike`\>\>

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`readable`](../interfaces/SerialTransport.md#readable)

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/transport/loopback.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/loopback.ts#L45)

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`close`](../interfaces/SerialTransport.md#close)

***

### onDisconnect()

```ts
onDisconnect(cb): () => void;
```

Defined in: [src/transport/loopback.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/loopback.ts#L55)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | () => `void` |

#### Returns

() => `void`

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`onDisconnect`](../interfaces/SerialTransport.md#ondisconnect)

# Class: MotionConfig

Defined in: [src/sensors/vl53l8/uld.ts:286](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L286)

Mirror of VL53L8CX_Motion_Configuration (156 bytes, plugin source).
`pack()` reproduces the C struct byte layout (`<i3I12B64b32B32B`).

## Constructors

### Constructor

```ts
new MotionConfig(): MotionConfig;
```

#### Returns

`MotionConfig`

## Properties

### refBinOffset

```ts
refBinOffset: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:287](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L287)

***

### detectionThreshold

```ts
detectionThreshold: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:288](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L288)

***

### extraNoiseSigma

```ts
extraNoiseSigma: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:289](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L289)

***

### nullDenClipValue

```ts
nullDenClipValue: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:290](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L290)

***

### memUpdateMode

```ts
memUpdateMode: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:291](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L291)

***

### memUpdateChoice

```ts
memUpdateChoice: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:292](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L292)

***

### sumSpan

```ts
sumSpan: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:293](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L293)

***

### featureLength

```ts
featureLength: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:294](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L294)

***

### nbOfAggregates

```ts
nbOfAggregates: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:295](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L295)

***

### nbOfTemporalAccumulations

```ts
nbOfTemporalAccumulations: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:296](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L296)

***

### minNbForGlobalDetection

```ts
minNbForGlobalDetection: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:297](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L297)

***

### globalIndicatorFormat1

```ts
globalIndicatorFormat1: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:298](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L298)

***

### globalIndicatorFormat2

```ts
globalIndicatorFormat2: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:299](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L299)

***

### spare1

```ts
spare1: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:300](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L300)

***

### spare2

```ts
spare2: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:301](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L301)

***

### spare3

```ts
spare3: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:302](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L302)

***

### mapId

```ts
mapId: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:303](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L303)

***

### indicatorFormat1

```ts
indicatorFormat1: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:304](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L304)

***

### indicatorFormat2

```ts
indicatorFormat2: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:305](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L305)

## Methods

### pack()

```ts
pack(): Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:307](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L307)

#### Returns

`Uint8Array`

# Class: NoDepzDeviceError

Defined in: [src/errors.ts:76](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L76)

Discovery found no DEPZ device matching the request (no candidate port by
USB id, an out-of-range index, or a serial that nothing answered to).
Mirrors the Python reference `NoDepzDeviceError`.

## Extends

- [`DepzError`](DepzError.md)

## Constructors

### Constructor

```ts
new NoDepzDeviceError(message?): NoDepzDeviceError;
```

Defined in: [src/errors.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L8)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `message?` | `string` |

#### Returns

`NoDepzDeviceError`

#### Inherited from

[`DepzError`](DepzError.md).[`constructor`](DepzError.md#constructor)

## Properties

### stackTraceLimit

```ts
static stackTraceLimit: number;
```

Defined in: node\_modules/@types/node/globals.d.ts:68

The `Error.stackTraceLimit` property specifies the number of stack frames
collected by a stack trace (whether generated by `new Error().stack` or
`Error.captureStackTrace(obj)`).

The default value is `10` but may be set to any valid JavaScript number. Changes
will affect any stack trace captured _after_ the value has been changed.

If set to a non-number value, or set to a negative number, stack traces will
not capture any frames.

#### Inherited from

[`DepzError`](DepzError.md).[`stackTraceLimit`](DepzError.md#stacktracelimit)

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

[`DepzError`](DepzError.md).[`cause`](DepzError.md#cause)

***

### name

```ts
name: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

[`DepzError`](DepzError.md).[`name`](DepzError.md#name)

***

### message

```ts
message: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

[`DepzError`](DepzError.md).[`message`](DepzError.md#message)

***

### stack?

```ts
optional stack?: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

[`DepzError`](DepzError.md).[`stack`](DepzError.md#stack)

## Methods

### captureStackTrace()

```ts
static captureStackTrace(targetObject, constructorOpt?): void;
```

Defined in: node\_modules/@types/node/globals.d.ts:52

Creates a `.stack` property on `targetObject`, which when accessed returns
a string representing the location in the code at which
`Error.captureStackTrace()` was called.

```js
const myObject = {};
Error.captureStackTrace(myObject);
myObject.stack;  // Similar to `new Error().stack`
```

The first line of the trace will be prefixed with
`${myObject.name}: ${myObject.message}`.

The optional `constructorOpt` argument accepts a function. If given, all frames
above `constructorOpt`, including `constructorOpt`, will be omitted from the
generated stack trace.

The `constructorOpt` argument is useful for hiding implementation
details of error generation from the user. For instance:

```js
function a() {
  b();
}

function b() {
  c();
}

function c() {
  // Create an error without stack trace to avoid calculating the stack trace twice.
  const { stackTraceLimit } = Error;
  Error.stackTraceLimit = 0;
  const error = new Error();
  Error.stackTraceLimit = stackTraceLimit;

  // Capture the stack trace above function b
  Error.captureStackTrace(error, b); // Neither function c, nor b is included in the stack trace
  throw error;
}

a();
```

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetObject` | `object` |
| `constructorOpt?` | `Function` |

#### Returns

`void`

#### Inherited from

[`DepzError`](DepzError.md).[`captureStackTrace`](DepzError.md#capturestacktrace)

***

### prepareStackTrace()

```ts
static prepareStackTrace(err, stackTraces): any;
```

Defined in: node\_modules/@types/node/globals.d.ts:56

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `err` | `Error` |
| `stackTraces` | `CallSite`[] |

#### Returns

`any`

#### See

https://v8.dev/docs/stack-trace-api#customizing-stack-traces

#### Inherited from

[`DepzError`](DepzError.md).[`prepareStackTrace`](DepzError.md#preparestacktrace)

# Class: PacketParser

Defined in: [src/protocol/framing.ts:100](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L100)

Incremental frame parser. Feed arbitrary byte chunks; get events.
Event order is invariant to chunking (contract 01 §5) except trash event
boundaries — concatenate trash data when comparing streams.

## Constructors

### Constructor

```ts
new PacketParser(): PacketParser;
```

#### Returns

`PacketParser`

## Properties

### packets

```ts
packets: number = 0;
```

Defined in: [src/protocol/framing.ts:102](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L102)

***

### crcErrors

```ts
crcErrors: number = 0;
```

Defined in: [src/protocol/framing.ts:103](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L103)

***

### headerErrors

```ts
headerErrors: number = 0;
```

Defined in: [src/protocol/framing.ts:104](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L104)

***

### trashBytes

```ts
trashBytes: number = 0;
```

Defined in: [src/protocol/framing.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L105)

## Accessors

### residue

#### Get Signature

```ts
get residue(): Uint8Array;
```

Defined in: [src/protocol/framing.ts:124](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L124)

Unconsumed bytes currently buffered (diagnostics/tests).

##### Returns

`Uint8Array`

## Methods

### feed()

```ts
feed(data): ParserEvent[];
```

Defined in: [src/protocol/framing.ts:107](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L107)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

#### Returns

[`ParserEvent`](../type-aliases/ParserEvent.md)[]

# Class: RecordingTransport

Defined in: [src/transport/replay.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L30)

Wraps any transport and tees both directions into `.depzrec` JSONL text.
Get the capture with `dump()` (line 1 is the header; browser-friendly —
no filesystem involved).

## Implements

- [`SerialTransport`](../interfaces/SerialTransport.md)

## Constructors

### Constructor

```ts
new RecordingTransport(inner, opts?): RecordingTransport;
```

Defined in: [src/transport/replay.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L35)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `inner` | [`SerialTransport`](../interfaces/SerialTransport.md) |
| `opts?` | \{ `headerExtra?`: `Record`\<`string`, `unknown`\>; \} |
| `opts.headerExtra?` | `Record`\<`string`, `unknown`\> |

#### Returns

`RecordingTransport`

## Accessors

### info

#### Get Signature

```ts
get info(): SerialTransportInfo;
```

Defined in: [src/transport/replay.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L45)

##### Returns

[`SerialTransportInfo`](../interfaces/SerialTransportInfo.md)

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`info`](../interfaces/SerialTransport.md#info)

## Methods

### dump()

```ts
dump(): string;
```

Defined in: [src/transport/replay.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L50)

The capture so far as `.depzrec` file content.

#### Returns

`string`

***

### open()

```ts
open(opts?): Promise<void>;
```

Defined in: [src/transport/replay.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L60)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `opts?` | \{ `baudRate?`: `number`; \} |
| `opts.baudRate?` | `number` |

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`open`](../interfaces/SerialTransport.md#open)

***

### write()

```ts
write(data): Promise<void>;
```

Defined in: [src/transport/replay.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L64)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`write`](../interfaces/SerialTransport.md#write)

***

### readable()

```ts
readable(): AsyncIterableIterator<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/transport/replay.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L71)

Raw chunks as they arrive; ends on close/disconnect.

#### Returns

`AsyncIterableIterator`\<`Uint8Array`\<`ArrayBufferLike`\>\>

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`readable`](../interfaces/SerialTransport.md#readable)

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/transport/replay.ts:78](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L78)

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`close`](../interfaces/SerialTransport.md#close)

***

### onDisconnect()

```ts
onDisconnect(cb): () => void;
```

Defined in: [src/transport/replay.ts:82](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L82)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | () => `void` |

#### Returns

() => `void`

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`onDisconnect`](../interfaces/SerialTransport.md#ondisconnect)

# Class: ReplayTransport

Defined in: [src/transport/replay.ts:104](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L104)

Replays the `rx` side of a `.depzrec` capture **causally**: an rx event
is released only after the host has written at least as many tx bytes as
preceded that event in the recording. Without this gating a replay would
deliver responses before the SDK even sends the requests.

`strictTx` additionally asserts the written bytes match the recorded tx
stream byte-for-byte (protocol regression mode). `realtime` paces rx
events by their recorded timestamps.

## Implements

- [`SerialTransport`](../interfaces/SerialTransport.md)

## Constructors

### Constructor

```ts
new ReplayTransport(content, opts?): ReplayTransport;
```

Defined in: [src/transport/replay.ts:119](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L119)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `content` | `string` |
| `opts?` | \{ `realtime?`: `boolean`; `strictTx?`: `boolean`; \} |
| `opts.realtime?` | `boolean` |
| `opts.strictTx?` | `boolean` |

#### Returns

`ReplayTransport`

## Properties

### info

```ts
readonly info: SerialTransportInfo;
```

Defined in: [src/transport/replay.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L105)

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`info`](../interfaces/SerialTransport.md#info)

***

### header

```ts
readonly header: Record<string, unknown>;
```

Defined in: [src/transport/replay.ts:107](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L107)

Parsed header line (unknown fields preserved, contract 08).

## Accessors

### exhausted

#### Get Signature

```ts
get exhausted(): boolean;
```

Defined in: [src/transport/replay.ts:146](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L146)

True once every recorded rx event has been delivered.

##### Returns

`boolean`

## Methods

### open()

```ts
open(): Promise<void>;
```

Defined in: [src/transport/replay.ts:150](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L150)

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`open`](../interfaces/SerialTransport.md#open)

***

### write()

```ts
write(data): Promise<void>;
```

Defined in: [src/transport/replay.ts:162](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L162)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`write`](../interfaces/SerialTransport.md#write)

***

### readable()

```ts
readable(): AsyncIterableIterator<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/transport/replay.ts:181](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L181)

Raw chunks as they arrive; ends on close/disconnect.

#### Returns

`AsyncIterableIterator`\<`Uint8Array`\<`ArrayBufferLike`\>\>

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`readable`](../interfaces/SerialTransport.md#readable)

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/transport/replay.ts:205](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L205)

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`close`](../interfaces/SerialTransport.md#close)

***

### onDisconnect()

```ts
onDisconnect(cb): () => void;
```

Defined in: [src/transport/replay.ts:214](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L214)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | () => `void` |

#### Returns

() => `void`

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`onDisconnect`](../interfaces/SerialTransport.md#ondisconnect)

# Class: Sh2Error

Defined in: [src/sensors/bno086/sh2.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L14)

SH-2 level failure (bad response status, FRS error, ...).

## Extends

- [`DepzError`](DepzError.md)

## Constructors

### Constructor

```ts
new Sh2Error(message?): Sh2Error;
```

Defined in: [src/errors.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L8)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `message?` | `string` |

#### Returns

`Sh2Error`

#### Inherited from

[`DepzError`](DepzError.md).[`constructor`](DepzError.md#constructor)

## Properties

### stackTraceLimit

```ts
static stackTraceLimit: number;
```

Defined in: node\_modules/@types/node/globals.d.ts:68

The `Error.stackTraceLimit` property specifies the number of stack frames
collected by a stack trace (whether generated by `new Error().stack` or
`Error.captureStackTrace(obj)`).

The default value is `10` but may be set to any valid JavaScript number. Changes
will affect any stack trace captured _after_ the value has been changed.

If set to a non-number value, or set to a negative number, stack traces will
not capture any frames.

#### Inherited from

[`DepzError`](DepzError.md).[`stackTraceLimit`](DepzError.md#stacktracelimit)

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

[`DepzError`](DepzError.md).[`cause`](DepzError.md#cause)

***

### name

```ts
name: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

[`DepzError`](DepzError.md).[`name`](DepzError.md#name)

***

### message

```ts
message: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

[`DepzError`](DepzError.md).[`message`](DepzError.md#message)

***

### stack?

```ts
optional stack?: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

[`DepzError`](DepzError.md).[`stack`](DepzError.md#stack)

## Methods

### captureStackTrace()

```ts
static captureStackTrace(targetObject, constructorOpt?): void;
```

Defined in: node\_modules/@types/node/globals.d.ts:52

Creates a `.stack` property on `targetObject`, which when accessed returns
a string representing the location in the code at which
`Error.captureStackTrace()` was called.

```js
const myObject = {};
Error.captureStackTrace(myObject);
myObject.stack;  // Similar to `new Error().stack`
```

The first line of the trace will be prefixed with
`${myObject.name}: ${myObject.message}`.

The optional `constructorOpt` argument accepts a function. If given, all frames
above `constructorOpt`, including `constructorOpt`, will be omitted from the
generated stack trace.

The `constructorOpt` argument is useful for hiding implementation
details of error generation from the user. For instance:

```js
function a() {
  b();
}

function b() {
  c();
}

function c() {
  // Create an error without stack trace to avoid calculating the stack trace twice.
  const { stackTraceLimit } = Error;
  Error.stackTraceLimit = 0;
  const error = new Error();
  Error.stackTraceLimit = stackTraceLimit;

  // Capture the stack trace above function b
  Error.captureStackTrace(error, b); // Neither function c, nor b is included in the stack trace
  throw error;
}

a();
```

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetObject` | `object` |
| `constructorOpt?` | `Function` |

#### Returns

`void`

#### Inherited from

[`DepzError`](DepzError.md).[`captureStackTrace`](DepzError.md#capturestacktrace)

***

### prepareStackTrace()

```ts
static prepareStackTrace(err, stackTraces): any;
```

Defined in: node\_modules/@types/node/globals.d.ts:56

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `err` | `Error` |
| `stackTraces` | `CallSite`[] |

#### Returns

`any`

#### See

https://v8.dev/docs/stack-trace-api#customizing-stack-traces

#### Inherited from

[`DepzError`](DepzError.md).[`prepareStackTrace`](DepzError.md#preparestacktrace)

# Class: ShtpLayer

Defined in: [src/sensors/bno086/shtp.ts:159](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L159)

Per-channel TX sequence counters + RX cargo reassembly.

Feed every inbound frame (the RPT_DATA payload after cmd/timestamp) to
`feed()`; it returns a completed ShtpCargo or null. Build outbound frames
with `nextFrame()` which consumes the channel's TX seq. The device layer
serializes access.

## Constructors

### Constructor

```ts
new ShtpLayer(): ShtpLayer;
```

#### Returns

`ShtpLayer`

## Properties

### discarded

```ts
discarded: number = 0;
```

Defined in: [src/sensors/bno086/shtp.ts:161](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L161)

Incomplete cargos thrown away.

## Methods

### nextFrame()

```ts
nextFrame(channel, payload): Uint8Array;
```

Defined in: [src/sensors/bno086/shtp.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L174)

Build a single-fragment frame, consuming the channel's TX seq.

Host-side cargos always fit one MCU slot (control payloads are <= 21
bytes); larger payloads are a caller bug.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `channel` | `number` |
| `payload` | `Uint8Array` |

#### Returns

`Uint8Array`

***

### txSeq()

```ts
txSeq(channel): number;
```

Defined in: [src/sensors/bno086/shtp.ts:183](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L183)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `channel` | `number` |

#### Returns

`number`

***

### feed()

```ts
feed(frame): ShtpCargo | null;
```

Defined in: [src/sensors/bno086/shtp.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L197)

Consume one inbound frame; return the cargo when complete.

Rules (contract 05 §3): a non-continuation fragment starts a new cargo
(discarding any partial one on that channel); a continuation without a
cargo in progress is dropped; the cargo completes when the accumulated
bytes reach the first fragment's advertised total.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `frame` | `Uint8Array` |

#### Returns

[`ShtpCargo`](../interfaces/ShtpCargo.md) \| `null`

***

### reset()

```ts
reset(): void;
```

Defined in: [src/sensors/bno086/shtp.ts:240](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L240)

Forget all TX seq counters and partial cargos (sensor reset).

#### Returns

`void`

# Class: Sr04

Defined in: [src/sensors/sr04.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L49)

HC-SR04 ultrasonic ranging device.

Measurements stream via callbacks (`onMeasurement`) and/or the pull
iterator (`measurements()`); both receive loop samples *and* unsolicited
single shots triggered by an AUX SYNC_IN edge (RPT_DATA source cmd 0x36).

## Extends

- [`DepzDevice`](DepzDevice.md)

## Constructors

### Constructor

```ts
new Sr04(transport, opts?): Sr04;
```

Defined in: [src/device/device.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L189)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | [`SerialTransport`](../interfaces/SerialTransport.md) |
| `opts?` | [`DeviceOptions`](../interfaces/DeviceOptions.md) |

#### Returns

`Sr04`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`constructor`](DepzDevice.md#constructor)

## Properties

### timeoutMs

```ts
timeoutMs: number;
```

Defined in: [src/device/device.ts:178](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L178)

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`timeoutMs`](DepzDevice.md#timeoutms)

***

### link

```ts
protected readonly link: DepzLink;
```

Defined in: [src/device/device.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L180)

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`link`](DepzDevice.md#link)

## Accessors

### stats

#### Get Signature

```ts
get stats(): LinkStats;
```

Defined in: [src/device/device.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L210)

##### Returns

[`LinkStats`](../interfaces/LinkStats.md)

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`stats`](DepzDevice.md#stats)

***

### timeSync

#### Get Signature

```ts
get timeSync(): TimeSync | null;
```

Defined in: [src/device/device.ts:474](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L474)

##### Returns

[`TimeSync`](../interfaces/TimeSync.md) \| `null`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`timeSync`](DepzDevice.md#timesync)

***

### streamDroppedCounts

#### Get Signature

```ts
get streamDroppedCounts(): number[];
```

Defined in: [src/sensors/sr04.ts:141](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L141)

Drop counters of all live measurement queues (diagnostics).

##### Returns

`number`[]

## Methods

### open()

```ts
open(): Promise<void>;
```

Defined in: [src/device/device.ts:200](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L200)

Open the transport and start the read pump.

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`open`](DepzDevice.md#open)

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/device/device.ts:205](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L205)

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`close`](DepzDevice.md#close)

***

### registerStream()

```ts
protected registerStream(stream): () => void;
```

Defined in: [src/device/device.ts:330](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L330)

Register a stream to be closed on device teardown.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `stream` | \{ `close`: `void`; \} |
| `stream.close` |

#### Returns

() => `void`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`registerStream`](DepzDevice.md#registerstream)

***

### onEvent()

```ts
onEvent(cb): () => void;
```

Defined in: [src/device/device.ts:343](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L343)

Subscribe to unsolicited/diagnostic events (read-pump context; do not
block). Returns an unsubscribe function.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`ev`) => `void` |

#### Returns

() => `void`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`onEvent`](DepzDevice.md#onevent)

***

### emitEvent()

```ts
protected emitEvent(event): void;
```

Defined in: [src/device/device.ts:350](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L350)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `event` | [`DeviceEvent`](../type-aliases/DeviceEvent.md) |

#### Returns

`void`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`emitEvent`](DepzDevice.md#emitevent)

***

### request()

```ts
request<T>(
   cmd, 
   payload?, 
opts?): Promise<T>;
```

Defined in: [src/device/device.ts:366](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L366)

Send `cmd` and wait for its correlated completion (contract 02 §1).

Exactly one of the completion paths must be configured:
- `okCompletes` — RPT_STATUS(cmd, OK) resolves with undefined;
- `matcher` — first packet for which the matcher returns non-`NO_MATCH`
  resolves with the matcher's return value.
Non-OK RPT_STATUS echoing `cmd` always rejects (BusyError for ERR_BUSY,
StatusError otherwise). One in-flight request per opcode.

#### Type Parameters

| Type Parameter | Default type |
| ------ | ------ |
| `T` | `void` |

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cmd` | `number` |
| `payload` | `Uint8Array` |
| `opts` | [`RequestOptions`](../interfaces/RequestOptions.md)\<`T`\> |

#### Returns

`Promise`\<`T`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`request`](DepzDevice.md#request)

***

### expectReport()

```ts
static expectReport<T>(reportId, unpack): Matcher<T>;
```

Defined in: [src/device/device.ts:409](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L409)

Matcher for a typed report identified by its report ID alone.

#### Type Parameters

| Type Parameter |
| ------ |
| `T` |

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `reportId` | `number` |
| `unpack` | (`payload`) => `T` |

#### Returns

[`Matcher`](../type-aliases/Matcher.md)\<`T`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`expectReport`](DepzDevice.md#expectreport)

***

### expectText()

```ts
static expectText(requestCmd): Matcher<string>;
```

Defined in: [src/device/device.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L414)

Matcher for RPT_TEXT echoing `requestCmd`.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `requestCmd` | `number` |

#### Returns

[`Matcher`](../type-aliases/Matcher.md)\<`string`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`expectText`](DepzDevice.md#expecttext)

***

### getDeviceName()

```ts
getDeviceName(): Promise<string>;
```

Defined in: [src/device/device.ts:425](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L425)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getDeviceName`](DepzDevice.md#getdevicename)

***

### getSoftwareName()

```ts
getSoftwareName(): Promise<string>;
```

Defined in: [src/device/device.ts:431](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L431)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getSoftwareName`](DepzDevice.md#getsoftwarename)

***

### getSerialNumber()

```ts
getSerialNumber(): Promise<string>;
```

Defined in: [src/device/device.ts:437](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L437)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getSerialNumber`](DepzDevice.md#getserialnumber)

***

### identify()

```ts
identify(): Promise<Identity>;
```

Defined in: [src/device/device.ts:444](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L444)

Classify the running firmware (contract 02 §4).

#### Returns

`Promise`\<[`Identity`](../interfaces/Identity.md)\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`identify`](DepzDevice.md#identify)

***

### readMcuTemperature()

```ts
readMcuTemperature(): Promise<number>;
```

Defined in: [src/device/device.ts:449](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L449)

Last cached MCU temperature in °C (device refreshes ~2 Hz).

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`readMcuTemperature`](DepzDevice.md#readmcutemperature)

***

### syncTime()

```ts
syncTime(samples?): Promise<TimeSync>;
```

Defined in: [src/device/device.ts:457](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L457)

NTP-style sync; keeps the lowest-RTT sample (contract 02 §5).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `samples` | `number` | `5` |

#### Returns

`Promise`\<[`TimeSync`](../interfaces/TimeSync.md)\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`syncTime`](DepzDevice.md#synctime)

***

### toHostTimeUs()

```ts
toHostTimeUs(deviceTsUs): bigint;
```

Defined in: [src/device/device.ts:479](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L479)

Device µs → host monotonic µs (requires a prior `syncTime`).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `deviceTsUs` | `bigint` |

#### Returns

`bigint`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`toHostTimeUs`](DepzDevice.md#tohosttimeus)

***

### getReportPayloadCrc()

```ts
getReportPayloadCrc(): Promise<CrcType>;
```

Defined in: [src/device/device.ts:484](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L484)

#### Returns

`Promise`\<[`CrcType`](../enumerations/CrcType.md)\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getReportPayloadCrc`](DepzDevice.md#getreportpayloadcrc)

***

### setReportPayloadCrc()

```ts
setReportPayloadCrc(crcType): Promise<void>;
```

Defined in: [src/device/device.ts:491](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L491)

Set the device→host payload CRC mode (host→device is per-packet).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `crcType` | [`CrcType`](../enumerations/CrcType.md) |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`setReportPayloadCrc`](DepzDevice.md#setreportpayloadcrc)

***

### getSyncPin()

```ts
getSyncPin(pin): Promise<SyncPinConfig>;
```

Defined in: [src/device/device.ts:495](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L495)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pin` | `number` |

#### Returns

`Promise`\<[`SyncPinConfig`](../interfaces/SyncPinConfig.md)\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getSyncPin`](DepzDevice.md#getsyncpin)

***

### setSyncPin()

```ts
setSyncPin(config): Promise<void>;
```

Defined in: [src/device/device.ts:501](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L501)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `config` | [`SyncPinConfig`](../interfaces/SyncPinConfig.md) |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`setSyncPin`](DepzDevice.md#setsyncpin)

***

### reset()

```ts
reset(): Promise<void>;
```

Defined in: [src/device/device.ts:506](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L506)

DEVICE_RESET: device ACKs then reboots; the link will drop.

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`reset`](DepzDevice.md#reset)

***

### enterBootloaderMode()

```ts
enterBootloaderMode(): Promise<void>;
```

Defined in: [src/device/device.ts:515](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L515)

Ask the device to reboot into the resident bootloader and close this
connection. Re-discovery/flash flow lives in the bootloader module
(contract 06).

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`enterBootloaderMode`](DepzDevice.md#enterbootloadermode)

***

### getSamplePeriodUs()

```ts
getSamplePeriodUs(): Promise<number>;
```

Defined in: [src/sensors/sr04.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L56)

Minimum interval between measurement starts (default 50000).

#### Returns

`Promise`\<`number`\>

***

### setSamplePeriodUs()

```ts
setSamplePeriodUs(periodUs): Promise<void>;
```

Defined in: [src/sensors/sr04.ts:66](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L66)

The effective rate is auto-throttled by the echo window (contract 03
§3) — reading back returns the stored value, not the effective one.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `periodUs` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getEchoDecayUs()

```ts
getEchoDecayUs(): Promise<number>;
```

Defined in: [src/sensors/sr04.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L72)

#### Returns

`Promise`\<`number`\>

***

### setEchoDecayUs()

```ts
setEchoDecayUs(decayUs): Promise<number>;
```

Defined in: [src/sensors/sr04.ts:82](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L82)

Set the settle pause; the device clamps to 4000–65000 µs silently, so
this re-reads and returns the value actually in effect.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `decayUs` | `number` |

#### Returns

`Promise`\<`number`\>

***

### measureOnce()

```ts
measureOnce(timeoutMs?): Promise<Sr04Measurement>;
```

Defined in: [src/sensors/sr04.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L94)

Single shot. Rejects with BusyError while the loop is running. The
reply arrives only when the echo completes (or times out at ~65.5 ms),
so the default timeout is generous.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `1000` |

#### Returns

`Promise`\<[`Sr04Measurement`](../interfaces/Sr04Measurement.md)\>

***

### start()

```ts
start(): Promise<void>;
```

Defined in: [src/sensors/sr04.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L105)

Start the measurement loop (idempotent).

#### Returns

`Promise`\<`void`\>

***

### stop()

```ts
stop(): Promise<void>;
```

Defined in: [src/sensors/sr04.ts:110](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L110)

Stop the measurement loop (idempotent).

#### Returns

`Promise`\<`void`\>

***

### onMeasurement()

```ts
onMeasurement(cb): () => void;
```

Defined in: [src/sensors/sr04.ts:118](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L118)

Subscribe to measurements (read-pump context; don't block). Returns an
unsubscribe function.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`m`) => `void` |

#### Returns

() => `void`

***

### measurements()

```ts
measurements(maxsize?): StreamQueue<Sr04Measurement>;
```

Defined in: [src/sensors/sr04.ts:130](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L130)

Async iterator over measurements — bounded, drop-oldest (contract 07
§3). The returned queue exposes `droppedCount`; it ends when the device
closes or the consumer breaks out of iteration.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `maxsize` | `number` | `256` |

#### Returns

[`StreamQueue`](StreamQueue.md)\<[`Sr04Measurement`](../interfaces/Sr04Measurement.md)\>

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [src/sensors/sr04.ts:147](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L147)

Hook for sensor subclasses: route streaming reports here. Return true
when the packet was consumed. Runs after status/matcher/common-report
routing, on the read-pump context.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pkt` | [`PacketEvent`](../interfaces/PacketEvent.md) |

#### Returns

`boolean`

#### Overrides

[`DepzDevice`](DepzDevice.md).[`handleReport`](DepzDevice.md#handlereport)

# Class: StatusError

Defined in: [src/errors.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L44)

Device answered a request with a non-OK RPT_STATUS.

## Extends

- [`DepzError`](DepzError.md)

## Extended by

- [`BusyError`](BusyError.md)

## Constructors

### Constructor

```ts
new StatusError(cmd, status): StatusError;
```

Defined in: [src/errors.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L49)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cmd` | `number` |
| `status` | `number` |

#### Returns

`StatusError`

#### Overrides

[`DepzError`](DepzError.md).[`constructor`](DepzError.md#constructor)

## Properties

### cmd

```ts
readonly cmd: number;
```

Defined in: [src/errors.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L45)

***

### status

```ts
readonly status: number;
```

Defined in: [src/errors.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L46)

***

### statusName

```ts
readonly statusName: string;
```

Defined in: [src/errors.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/errors.ts#L47)

***

### stackTraceLimit

```ts
static stackTraceLimit: number;
```

Defined in: node\_modules/@types/node/globals.d.ts:68

The `Error.stackTraceLimit` property specifies the number of stack frames
collected by a stack trace (whether generated by `new Error().stack` or
`Error.captureStackTrace(obj)`).

The default value is `10` but may be set to any valid JavaScript number. Changes
will affect any stack trace captured _after_ the value has been changed.

If set to a non-number value, or set to a negative number, stack traces will
not capture any frames.

#### Inherited from

[`DepzError`](DepzError.md).[`stackTraceLimit`](DepzError.md#stacktracelimit)

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

[`DepzError`](DepzError.md).[`cause`](DepzError.md#cause)

***

### name

```ts
name: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

[`DepzError`](DepzError.md).[`name`](DepzError.md#name)

***

### message

```ts
message: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

[`DepzError`](DepzError.md).[`message`](DepzError.md#message)

***

### stack?

```ts
optional stack?: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

[`DepzError`](DepzError.md).[`stack`](DepzError.md#stack)

## Methods

### captureStackTrace()

```ts
static captureStackTrace(targetObject, constructorOpt?): void;
```

Defined in: node\_modules/@types/node/globals.d.ts:52

Creates a `.stack` property on `targetObject`, which when accessed returns
a string representing the location in the code at which
`Error.captureStackTrace()` was called.

```js
const myObject = {};
Error.captureStackTrace(myObject);
myObject.stack;  // Similar to `new Error().stack`
```

The first line of the trace will be prefixed with
`${myObject.name}: ${myObject.message}`.

The optional `constructorOpt` argument accepts a function. If given, all frames
above `constructorOpt`, including `constructorOpt`, will be omitted from the
generated stack trace.

The `constructorOpt` argument is useful for hiding implementation
details of error generation from the user. For instance:

```js
function a() {
  b();
}

function b() {
  c();
}

function c() {
  // Create an error without stack trace to avoid calculating the stack trace twice.
  const { stackTraceLimit } = Error;
  Error.stackTraceLimit = 0;
  const error = new Error();
  Error.stackTraceLimit = stackTraceLimit;

  // Capture the stack trace above function b
  Error.captureStackTrace(error, b); // Neither function c, nor b is included in the stack trace
  throw error;
}

a();
```

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetObject` | `object` |
| `constructorOpt?` | `Function` |

#### Returns

`void`

#### Inherited from

[`DepzError`](DepzError.md).[`captureStackTrace`](DepzError.md#capturestacktrace)

***

### prepareStackTrace()

```ts
static prepareStackTrace(err, stackTraces): any;
```

Defined in: node\_modules/@types/node/globals.d.ts:56

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `err` | `Error` |
| `stackTraces` | `CallSite`[] |

#### Returns

`any`

#### See

https://v8.dev/docs/stack-trace-api#customizing-stack-traces

#### Inherited from

[`DepzError`](DepzError.md).[`prepareStackTrace`](DepzError.md#preparestacktrace)

# Class: StreamQueue\<T\>

Defined in: [src/device/device.ts:110](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L110)

Bounded drop-oldest async stream (contract 07 §3). Doubles as the async
iterator returned by e.g. `Sr04.measurements()`; `droppedCount` is the
monotonic count of items evicted while the consumer lagged.

## Type Parameters

| Type Parameter |
| ------ |
| `T` |

## Implements

- `AsyncIterableIterator`\<`T`\>

## Constructors

### Constructor

```ts
new StreamQueue<T>(maxsize, onDone?): StreamQueue<T>;
```

Defined in: [src/device/device.ts:119](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L119)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `maxsize` | `number` |
| `onDone?` | () => `void` |

#### Returns

`StreamQueue`\<`T`\>

## Properties

### droppedCount

```ts
droppedCount: number = 0;
```

Defined in: [src/device/device.ts:111](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L111)

## Methods

### push()

```ts
push(item): void;
```

Defined in: [src/device/device.ts:125](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L125)

Producer side: enqueue, evicting the oldest item when full.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `item` | `T` |

#### Returns

`void`

***

### close()

```ts
close(): void;
```

Defined in: [src/device/device.ts:140](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L140)

End the stream; buffered items are still drained by the consumer.

#### Returns

`void`

***

### next()

```ts
next(): Promise<IteratorResult<T, any>>;
```

Defined in: [src/device/device.ts:148](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L148)

#### Returns

`Promise`\<`IteratorResult`\<`T`, `any`\>\>

#### Implementation of

```ts
AsyncIterableIterator.next
```

***

### return()

```ts
return(): Promise<IteratorResult<T, any>>;
```

Defined in: [src/device/device.ts:155](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L155)

#### Returns

`Promise`\<`IteratorResult`\<`T`, `any`\>\>

#### Implementation of

```ts
AsyncIterableIterator.return
```

***

### \[asyncIterator\]()

```ts
asyncIterator: AsyncIterableIterator<T>;
```

Defined in: [src/device/device.ts:161](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L161)

#### Returns

`AsyncIterableIterator`\<`T`\>

#### Implementation of

```ts
AsyncIterableIterator.[asyncIterator]
```

# Class: VL53L4CD

Defined in: [src/sensors/vl53l4/uld.ts:367](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L367)

Port of VL53L4CD_api.c + VL53L4CD_calibration.c (ULD 2.2.3).

## Constructors

### Constructor

```ts
new VL53L4CD(p): VL53L4CD;
```

Defined in: [src/sensors/vl53l4/uld.ts:368](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L368)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | [`Vl53l4Platform`](../interfaces/Vl53l4Platform.md) |

#### Returns

`VL53L4CD`

## Properties

### p

```ts
readonly p: Vl53l4Platform;
```

Defined in: [src/sensors/vl53l4/uld.ts:368](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L368)

## Methods

### rdByte()

```ts
rdByte(addr): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:372](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L372)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |

#### Returns

`Promise`\<`number`\>

***

### rdWord()

```ts
rdWord(addr): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:376](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L376)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |

#### Returns

`Promise`\<`number`\>

***

### rdDword()

```ts
rdDword(addr): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:381](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L381)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |

#### Returns

`Promise`\<`number`\>

***

### wrByte()

```ts
wrByte(addr, value): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:386](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L386)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `value` | `number` |

#### Returns

`Promise`\<`void`\>

***

### wrWord()

```ts
wrWord(addr, value): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:390](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L390)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `value` | `number` |

#### Returns

`Promise`\<`void`\>

***

### wrDword()

```ts
wrDword(addr, value): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:394](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L394)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `value` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getSensorId()

```ts
getSensorId(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:403](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L403)

#### Returns

`Promise`\<`number`\>

***

### isAlive()

```ts
isAlive(): Promise<boolean>;
```

Defined in: [src/sensors/vl53l4/uld.ts:407](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L407)

#### Returns

`Promise`\<`boolean`\>

***

### waitBoot()

```ts
waitBoot(timeoutMs?): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:413](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L413)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `1000` |

#### Returns

`Promise`\<`void`\>

***

### sensorInit()

```ts
sensorInit(busKhz?): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:429](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L429)

Initialise the sensor and leave the bus at `busKhz`.

The configuration block is written at I2C_KHZ_BOOT (400 kHz) because that
is the only speed an unconfigured sensor is specified for; the bridge is
re-timed to `busKhz` after it. A sensor reset just re-runs this whole
sequence.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `busKhz` | `number` | `I2C_KHZ_DEFAULT` |

#### Returns

`Promise`\<`void`\>

***

### clearInterrupt()

```ts
clearInterrupt(): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:453](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L453)

#### Returns

`Promise`\<`void`\>

***

### startRanging()

```ts
startRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:457](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L457)

#### Returns

`Promise`\<`void`\>

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:463](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L463)

#### Returns

`Promise`\<`void`\>

***

### checkForDataReady()

```ts
checkForDataReady(): Promise<boolean>;
```

Defined in: [src/sensors/vl53l4/uld.ts:467](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L467)

#### Returns

`Promise`\<`boolean`\>

***

### waitDataReady()

```ts
waitDataReady(timeoutMs?): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:472](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L472)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `1000` |

#### Returns

`Promise`\<`void`\>

***

### getResult()

```ts
getResult(): Promise<Vl53l4Results>;
```

Defined in: [src/sensors/vl53l4/uld.ts:484](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L484)

One block read instead of the C driver's six register reads — the sensor
auto-increments and the decoding is identical.

#### Returns

`Promise`\<[`Vl53l4Results`](../interfaces/Vl53l4Results.md)\>

***

### setRangeTiming()

```ts
setRangeTiming(timingBudgetMs, interMeasurementMs): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:490](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L490)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `timingBudgetMs` | `number` |
| `interMeasurementMs` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getRangeTiming()

```ts
getRangeTiming(): Promise<RangeTiming>;
```

Defined in: [src/sensors/vl53l4/uld.ts:503](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L503)

#### Returns

`Promise`\<[`RangeTiming`](../interfaces/RangeTiming.md)\>

***

### setOffset()

```ts
setOffset(offsetMm): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:514](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L514)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `offsetMm` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getOffset()

```ts
getOffset(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:520](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L520)

#### Returns

`Promise`\<`number`\>

***

### setXtalk()

```ts
setXtalk(xtalkKcps): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:526](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L526)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `xtalkKcps` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getXtalk()

```ts
getXtalk(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:532](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L532)

#### Returns

`Promise`\<`number`\>

***

### setDetectionThresholds()

```ts
setDetectionThresholds(
   distanceLowMm, 
   distanceHighMm, 
window): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:538](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L538)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `distanceLowMm` | `number` |
| `distanceHighMm` | `number` |
| `window` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getDetectionThresholds()

```ts
getDetectionThresholds(): Promise<{
  distanceLowMm: number;
  distanceHighMm: number;
  window: number;
}>;
```

Defined in: [src/sensors/vl53l4/uld.ts:548](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L548)

#### Returns

`Promise`\<\{
  `distanceLowMm`: `number`;
  `distanceHighMm`: `number`;
  `window`: `number`;
\}\>

***

### setSignalThreshold()

```ts
setSignalThreshold(signalKcps): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:559](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L559)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `signalKcps` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getSignalThreshold()

```ts
getSignalThreshold(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:563](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L563)

#### Returns

`Promise`\<`number`\>

***

### setSigmaThreshold()

```ts
setSigmaThreshold(sigmaMm): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:567](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L567)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `sigmaMm` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getSigmaThreshold()

```ts
getSigmaThreshold(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:571](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L571)

#### Returns

`Promise`\<`number`\>

***

### startTemperatureUpdate()

```ts
startTemperatureUpdate(): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:578](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L578)

Recommended after a >8 °C ambient change (ST Example_3).

#### Returns

`Promise`\<`void`\>

***

### calibrateOffset()

```ts
calibrateOffset(targetDistMm, nbSamples?): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:610](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L610)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `targetDistMm` | `number` | `undefined` |
| `nbSamples` | `number` | `20` |

#### Returns

`Promise`\<`number`\>

***

### calibrateXtalk()

```ts
calibrateXtalk(targetDistMm, nbSamples?): Promise<number>;
```

Defined in: [src/sensors/vl53l4/uld.ts:630](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L630)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `targetDistMm` | `number` | `undefined` |
| `nbSamples` | `number` | `20` |

#### Returns

`Promise`\<`number`\>

# Class: VL53L8CX

Defined in: [src/sensors/vl53l8/uld.ts:410](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L410)

## Constructors

### Constructor

```ts
new VL53L8CX(
   platform, 
   assets, 
   variant?): VL53L8CX;
```

Defined in: [src/sensors/vl53l8/uld.ts:434](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L434)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `platform` | [`Vl53l8Platform`](../interfaces/Vl53l8Platform.md) | `undefined` |
| `assets` | [`Vl53l8Assets`](../interfaces/Vl53l8Assets.md) | `undefined` |
| `variant` | [`Vl53l8Variant`](../type-aliases/Vl53l8Variant.md) | `"cx"` |

#### Returns

`VL53L8CX`

## Properties

### p

```ts
readonly p: Vl53l8Platform;
```

Defined in: [src/sensors/vl53l8/uld.ts:411](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L411)

***

### variant

```ts
readonly variant: Vl53l8Variant;
```

Defined in: [src/sensors/vl53l8/uld.ts:412](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L412)

***

### fwChecksum

```ts
readonly fwChecksum: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:413](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L413)

***

### firmware

```ts
readonly firmware: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L414)

***

### defaultCfg

```ts
readonly defaultCfg: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:415](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L415)

***

### defaultXtalk

```ts
readonly defaultXtalk: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:416](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L416)

***

### getNvmCmd

```ts
readonly getNvmCmd: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:417](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L417)

***

### offsetData

```ts
offsetData: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:419](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L419)

***

### xtalkData

```ts
xtalkData: Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:420](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L420)

***

### streamcount

```ts
streamcount: number = 255;
```

Defined in: [src/sensors/vl53l8/uld.ts:421](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L421)

***

### dataReadSize

```ts
dataReadSize: number = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:422](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L422)

***

### frameSizeMismatch

```ts
frameSizeMismatch: 
  | {
  fw: number;
  host: number;
}
  | null = null;
```

Defined in: [src/sensors/vl53l8/uld.ts:424](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L424)

{ fw, host } when the FW disagrees with the api.c formula.

***

### lastBlocks

```ts
lastBlocks: [number, number, number][] = [];
```

Defined in: [src/sensors/vl53l8/uld.ts:426](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L426)

[idx, type, size] of the blocks in the last parsed frame.

***

### motionPresent

```ts
motionPresent: boolean = false;
```

Defined in: [src/sensors/vl53l8/uld.ts:428](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L428)

Whether the motion-indicator output block is configured.

## Methods

### dciReadData()

```ts
dciReadData(index, dataSize): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/sensors/vl53l8/uld.ts:521](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L521)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `index` | `number` |
| `dataSize` | `number` |

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### dciWriteData()

```ts
dciWriteData(index, data): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:537](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L537)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `index` | `number` |
| `data` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

***

### dciReplaceData()

```ts
dciReplaceData(
   index, 
   dataSize, 
   newData, 
newDataPos): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:565](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L565)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `index` | `number` |
| `dataSize` | `number` |
| `newData` | `Uint8Array` |
| `newDataPos` | `number` |

#### Returns

`Promise`\<`void`\>

***

### isAlive()

```ts
isAlive(): Promise<[number, number]>;
```

Defined in: [src/sensors/vl53l8/uld.ts:679](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L679)

Returns [deviceId, revisionId]; alive when (0xF0, 0x0C).

#### Returns

`Promise`\<\[`number`, `number`\]\>

***

### init()

```ts
init(progress?): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:688](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L688)

vl53l8cx_init(): boot the sensor MCU, download the 84 KB sensor
firmware, upload NVM offset / xtalk / default configuration.
`progress(text)` is an optional UI callback.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `progress?` | (`text`) => `void` |

#### Returns

`Promise`\<`void`\>

***

### getResolution()

```ts
getResolution(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:834](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L834)

#### Returns

`Promise`\<`number`\>

***

### setResolution()

```ts
setResolution(resolution): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:839](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L839)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `resolution` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getRangingFrequencyHz()

```ts
getRangingFrequencyHz(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:871](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L871)

#### Returns

`Promise`\<`number`\>

***

### setRangingFrequencyHz()

```ts
setRangingFrequencyHz(hz): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:875](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L875)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `hz` | `number` |

#### Returns

`Promise`\<`void`\>

***

### setRangingMode()

```ts
setRangingMode(mode): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:879](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L879)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `mode` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getRangingMode()

```ts
getRangingMode(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:897](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L897)

#### Returns

`Promise`\<`number`\>

***

### getIntegrationTimeMs()

```ts
getIntegrationTimeMs(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:902](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L902)

#### Returns

`Promise`\<`number`\>

***

### setIntegrationTimeMs()

```ts
setIntegrationTimeMs(timeMs): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:908](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L908)

Integration time 2..1000 ms. No effect in continuous ranging mode.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `timeMs` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getSharpenerPercent()

```ts
getSharpenerPercent(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:925](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L925)

Sharpener 0..99 %. Rounds to nearest: the register holds pct scaled to
0..255, and truncating the way back (as ST's C ULD does) loses a count for
95 of the 100 legal values — set(25) read back as 24. That also made
calibrateXtalk's save/restore decay the setting by 1 % on every run
(25 -> 24 -> 23 -> ...). The stored byte is unchanged; only this host-side
interpretation is. Kept in lockstep with the Python SDK's uld.py.

#### Returns

`Promise`\<`number`\>

***

### setSharpenerPercent()

```ts
setSharpenerPercent(pct): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:930](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L930)

Sharpener 0..99 % (0 = disabled).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pct` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getTargetOrder()

```ts
getTargetOrder(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:937](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L937)

#### Returns

`Promise`\<`number`\>

***

### setTargetOrder()

```ts
setTargetOrder(order): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:941](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L941)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `order` | `number` |

#### Returns

`Promise`\<`void`\>

***

### startRanging()

```ts
startRanging(cnhDataSize?): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:956](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L956)

Start ranging. When `cnhDataSize` is given (bytes, from
CnhConfig.requiredMemory), a VL53L8CH CNH data block is appended to the
output list so each frame also carries the compact-network-histogram
buffer. The CNH frame is far larger than the MCU stream cap, so the
caller must read it in poll-mode (checkDataReady + getRangingData),
not via the MCU INT stream.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `cnhDataSize` | `number` \| `null` | `null` |

#### Returns

`Promise`\<`void`\>

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1054](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1054)

#### Returns

`Promise`\<`void`\>

***

### checkDataReady()

```ts
checkDataReady(): Promise<boolean>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1090](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1090)

#### Returns

`Promise`\<`boolean`\>

***

### getRangingData()

```ts
getRangingData(): Promise<Vl53l8Results>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1109](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1109)

Poll-mode read: fetch one results frame from reg 0x00 and parse it.

#### Returns

`Promise`\<[`Vl53l8Results`](../interfaces/Vl53l8Results.md)\>

***

### parseFrame()

```ts
parseFrame(raw): Vl53l8Results;
```

Defined in: [src/sensors/vl53l8/uld.ts:1122](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1122)

Parse one raw results frame (dataReadSize bytes read from reg 0x00).
Used both by poll-mode getRangingData() and by the host when the MCU
pushes frames over the INT-driven stream. Returns per-zone arrays
distanceMm, targetStatus, nbTargetDetected, signalPerSpad (kcps/SPAD),
ambientPerSpad (kcps/SPAD), nbSpadsEnabled, rangeSigmaMm, reflectance
(%), plus the per-frame scalar siliconTempDegc.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `raw` | `Uint8Array` |

#### Returns

[`Vl53l8Results`](../interfaces/Vl53l8Results.md)

***

### getPowerMode()

```ts
getPowerMode(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1269](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1269)

#### Returns

`Promise`\<`number`\>

***

### setPowerMode()

```ts
setPowerMode(powerMode): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1289](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1289)

WAKEUP (1), SLEEP (0) or DEEP_SLEEP (2). Not allowed while ranging. Wake
from DEEP_SLEEP re-runs init() (the FW blob is lost).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `powerMode` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getXtalkMargin()

```ts
getXtalkMargin(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1319](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1319)

Xtalk margin in kcps/spad.

#### Returns

`Promise`\<`number`\>

***

### setXtalkMargin()

```ts
setXtalkMargin(marginKcps): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1324](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1324)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `marginKcps` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getCaldataXtalk()

```ts
getCaldataXtalk(): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1336](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1336)

Read the live 776-byte xtalk calibration blob back from the sensor FW (as
produced by calibrateXtalk). Restores the current resolution afterwards.

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### setCaldataXtalk()

```ts
setCaldataXtalk(xtalkData): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1355](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1355)

Restore a previously saved 776-byte xtalk calibration blob. The blob is
re-uploaded to the FW by the next setResolution()/startRanging().

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `xtalkData` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

***

### getDetectionThresholdsEnable()

```ts
getDetectionThresholdsEnable(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1366](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1366)

#### Returns

`Promise`\<`number`\>

***

### setDetectionThresholdsEnable()

```ts
setDetectionThresholdsEnable(enabled): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1370](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1370)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `enabled` | `boolean` |

#### Returns

`Promise`\<`void`\>

***

### getDetectionThresholds()

```ts
getDetectionThresholds(): Promise<DetectionThreshold[]>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1385](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1385)

Return the 64 detection thresholds; low/high rescaled to real units.

#### Returns

`Promise`\<[`DetectionThreshold`](../interfaces/DetectionThreshold.md)[]\>

***

### setDetectionThresholds()

```ts
setDetectionThresholds(thresholds): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1410](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1410)

Program the 64 detection thresholds (missing entries default to zeros).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `thresholds` | `Partial`\<[`DetectionThreshold`](../interfaces/DetectionThreshold.md)\>[] |

#### Returns

`Promise`\<`void`\>

***

### setDetectionThresholdsAutoStop()

```ts
setDetectionThresholdsAutoStop(autoStop): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1416](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1416)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `autoStop` | `boolean` |

#### Returns

`Promise`\<`void`\>

***

### motionIndicatorInit()

```ts
motionIndicatorInit(resolution): Promise<MotionConfig>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1427](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1427)

Initialize a motion-indicator configuration (default distance window) and
write it to the sensor. Enables the motion output block so subsequent
frames carry motion data.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `resolution` | `number` |

#### Returns

`Promise`\<[`MotionConfig`](MotionConfig.md)\>

***

### motionIndicatorSetResolution()

```ts
motionIndicatorSetResolution(cfg, resolution): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1434](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1434)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cfg` | [`MotionConfig`](MotionConfig.md) |
| `resolution` | `number` |

#### Returns

`Promise`\<`void`\>

***

### motionIndicatorSetDistanceMotion()

```ts
motionIndicatorSetDistanceMotion(
   cfg, 
   distanceMinMm, 
distanceMaxMm): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1439](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1439)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cfg` | [`MotionConfig`](MotionConfig.md) |
| `distanceMinMm` | `number` |
| `distanceMaxMm` | `number` |

#### Returns

`Promise`\<`void`\>

***

### calibrateXtalk()

```ts
calibrateXtalk(
   reflectancePercent, 
   nbSamples, 
distanceMm): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:1523](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L1523)

vl53l8cx_calibrate_xtalk: run on-device crosstalk calibration.

NOTE: ported from ST ULD source but NOT verified against live hardware in
this SDK — the get/set caldata-xtalk buffer path IS the tested save/restore
route. Saves & restores resolution/frequency/int-time/sharpener/target-
order/xtalk-margin/ranging-mode around the run.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `reflectancePercent` | `number` |
| `nbSamples` | `number` |
| `distanceMm` | `number` |

#### Returns

`Promise`\<`void`\>

# Class: Vl53l4Cd

Defined in: [src/sensors/vl53l4/vl53l4.ts:97](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L97)

VL53L4CD single-zone ToF device.

`init()` runs the ULD boot sequence (no firmware blob — the sensor carries
its own), then configure and `startRanging()`. Configuration methods must
not be called while ranging: the INT-driven stream owns the register bank
(contract 10). Measurements stream via callbacks (`onMeasurement`) and/or
the pull iterator (`measurements()`).

## Extends

- [`DepzDevice`](DepzDevice.md)

## Constructors

### Constructor

```ts
new Vl53l4Cd(transport, opts?): Vl53l4Cd;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:151](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L151)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | [`SerialTransport`](../interfaces/SerialTransport.md) |
| `opts?` | [`Vl53l4Options`](../interfaces/Vl53l4Options.md) |

#### Returns

`Vl53l4Cd`

#### Overrides

[`DepzDevice`](DepzDevice.md).[`constructor`](DepzDevice.md#constructor)

## Properties

### timeoutMs

```ts
timeoutMs: number;
```

Defined in: [src/device/device.ts:178](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L178)

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`timeoutMs`](DepzDevice.md#timeoutms)

***

### link

```ts
protected readonly link: DepzLink;
```

Defined in: [src/device/device.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L180)

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`link`](DepzDevice.md#link)

## Accessors

### stats

#### Get Signature

```ts
get stats(): LinkStats;
```

Defined in: [src/device/device.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L210)

##### Returns

[`LinkStats`](../interfaces/LinkStats.md)

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`stats`](DepzDevice.md#stats)

***

### timeSync

#### Get Signature

```ts
get timeSync(): TimeSync | null;
```

Defined in: [src/device/device.ts:474](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L474)

##### Returns

[`TimeSync`](../interfaces/TimeSync.md) \| `null`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`timeSync`](DepzDevice.md#timesync)

***

### uld

#### Get Signature

```ts
get uld(): VL53L4CD;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:160](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L160)

The underlying ULD driver (escape hatch for raw register access).

##### Returns

[`VL53L4CD`](VL53L4CD.md)

***

### initialized

#### Get Signature

```ts
get initialized(): boolean;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:168](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L168)

True after a successful init(). Cleared by resetSensor() and xshut() — a
power-cycled sensor holds none of the ULD configuration.

##### Returns

`boolean`

***

### ranging

#### Get Signature

```ts
get ranging(): boolean;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:388](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L388)

##### Returns

`boolean`

***

### streamParseErrors

#### Get Signature

```ts
get streamParseErrors(): number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:467](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L467)

Stream reports dropped because the result block failed to decode (short
block from a reconfigured stream, corrupt read).

##### Returns

`number`

***

### streamDroppedCounts

#### Get Signature

```ts
get streamDroppedCounts(): number[];
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:472](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L472)

Drop counters of all live measurement queues (diagnostics).

##### Returns

`number`[]

## Methods

### open()

```ts
open(): Promise<void>;
```

Defined in: [src/device/device.ts:200](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L200)

Open the transport and start the read pump.

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`open`](DepzDevice.md#open)

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/device/device.ts:205](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L205)

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`close`](DepzDevice.md#close)

***

### registerStream()

```ts
protected registerStream(stream): () => void;
```

Defined in: [src/device/device.ts:330](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L330)

Register a stream to be closed on device teardown.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `stream` | \{ `close`: `void`; \} |
| `stream.close` |

#### Returns

() => `void`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`registerStream`](DepzDevice.md#registerstream)

***

### onEvent()

```ts
onEvent(cb): () => void;
```

Defined in: [src/device/device.ts:343](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L343)

Subscribe to unsolicited/diagnostic events (read-pump context; do not
block). Returns an unsubscribe function.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`ev`) => `void` |

#### Returns

() => `void`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`onEvent`](DepzDevice.md#onevent)

***

### emitEvent()

```ts
protected emitEvent(event): void;
```

Defined in: [src/device/device.ts:350](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L350)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `event` | [`DeviceEvent`](../type-aliases/DeviceEvent.md) |

#### Returns

`void`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`emitEvent`](DepzDevice.md#emitevent)

***

### request()

```ts
request<T>(
   cmd, 
   payload?, 
opts?): Promise<T>;
```

Defined in: [src/device/device.ts:366](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L366)

Send `cmd` and wait for its correlated completion (contract 02 §1).

Exactly one of the completion paths must be configured:
- `okCompletes` — RPT_STATUS(cmd, OK) resolves with undefined;
- `matcher` — first packet for which the matcher returns non-`NO_MATCH`
  resolves with the matcher's return value.
Non-OK RPT_STATUS echoing `cmd` always rejects (BusyError for ERR_BUSY,
StatusError otherwise). One in-flight request per opcode.

#### Type Parameters

| Type Parameter | Default type |
| ------ | ------ |
| `T` | `void` |

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cmd` | `number` |
| `payload` | `Uint8Array` |
| `opts` | [`RequestOptions`](../interfaces/RequestOptions.md)\<`T`\> |

#### Returns

`Promise`\<`T`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`request`](DepzDevice.md#request)

***

### expectReport()

```ts
static expectReport<T>(reportId, unpack): Matcher<T>;
```

Defined in: [src/device/device.ts:409](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L409)

Matcher for a typed report identified by its report ID alone.

#### Type Parameters

| Type Parameter |
| ------ |
| `T` |

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `reportId` | `number` |
| `unpack` | (`payload`) => `T` |

#### Returns

[`Matcher`](../type-aliases/Matcher.md)\<`T`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`expectReport`](DepzDevice.md#expectreport)

***

### expectText()

```ts
static expectText(requestCmd): Matcher<string>;
```

Defined in: [src/device/device.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L414)

Matcher for RPT_TEXT echoing `requestCmd`.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `requestCmd` | `number` |

#### Returns

[`Matcher`](../type-aliases/Matcher.md)\<`string`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`expectText`](DepzDevice.md#expecttext)

***

### getDeviceName()

```ts
getDeviceName(): Promise<string>;
```

Defined in: [src/device/device.ts:425](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L425)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getDeviceName`](DepzDevice.md#getdevicename)

***

### getSoftwareName()

```ts
getSoftwareName(): Promise<string>;
```

Defined in: [src/device/device.ts:431](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L431)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getSoftwareName`](DepzDevice.md#getsoftwarename)

***

### getSerialNumber()

```ts
getSerialNumber(): Promise<string>;
```

Defined in: [src/device/device.ts:437](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L437)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getSerialNumber`](DepzDevice.md#getserialnumber)

***

### identify()

```ts
identify(): Promise<Identity>;
```

Defined in: [src/device/device.ts:444](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L444)

Classify the running firmware (contract 02 §4).

#### Returns

`Promise`\<[`Identity`](../interfaces/Identity.md)\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`identify`](DepzDevice.md#identify)

***

### readMcuTemperature()

```ts
readMcuTemperature(): Promise<number>;
```

Defined in: [src/device/device.ts:449](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L449)

Last cached MCU temperature in °C (device refreshes ~2 Hz).

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`readMcuTemperature`](DepzDevice.md#readmcutemperature)

***

### syncTime()

```ts
syncTime(samples?): Promise<TimeSync>;
```

Defined in: [src/device/device.ts:457](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L457)

NTP-style sync; keeps the lowest-RTT sample (contract 02 §5).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `samples` | `number` | `5` |

#### Returns

`Promise`\<[`TimeSync`](../interfaces/TimeSync.md)\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`syncTime`](DepzDevice.md#synctime)

***

### toHostTimeUs()

```ts
toHostTimeUs(deviceTsUs): bigint;
```

Defined in: [src/device/device.ts:479](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L479)

Device µs → host monotonic µs (requires a prior `syncTime`).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `deviceTsUs` | `bigint` |

#### Returns

`bigint`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`toHostTimeUs`](DepzDevice.md#tohosttimeus)

***

### getReportPayloadCrc()

```ts
getReportPayloadCrc(): Promise<CrcType>;
```

Defined in: [src/device/device.ts:484](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L484)

#### Returns

`Promise`\<[`CrcType`](../enumerations/CrcType.md)\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getReportPayloadCrc`](DepzDevice.md#getreportpayloadcrc)

***

### setReportPayloadCrc()

```ts
setReportPayloadCrc(crcType): Promise<void>;
```

Defined in: [src/device/device.ts:491](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L491)

Set the device→host payload CRC mode (host→device is per-packet).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `crcType` | [`CrcType`](../enumerations/CrcType.md) |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`setReportPayloadCrc`](DepzDevice.md#setreportpayloadcrc)

***

### getSyncPin()

```ts
getSyncPin(pin): Promise<SyncPinConfig>;
```

Defined in: [src/device/device.ts:495](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L495)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pin` | `number` |

#### Returns

`Promise`\<[`SyncPinConfig`](../interfaces/SyncPinConfig.md)\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getSyncPin`](DepzDevice.md#getsyncpin)

***

### setSyncPin()

```ts
setSyncPin(config): Promise<void>;
```

Defined in: [src/device/device.ts:501](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L501)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `config` | [`SyncPinConfig`](../interfaces/SyncPinConfig.md) |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`setSyncPin`](DepzDevice.md#setsyncpin)

***

### reset()

```ts
reset(): Promise<void>;
```

Defined in: [src/device/device.ts:506](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L506)

DEVICE_RESET: device ACKs then reboots; the link will drop.

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`reset`](DepzDevice.md#reset)

***

### enterBootloaderMode()

```ts
enterBootloaderMode(): Promise<void>;
```

Defined in: [src/device/device.ts:515](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L515)

Ask the device to reboot into the resident bootloader and close this
connection. Re-discovery/flash flow lives in the bootloader module
(contract 06).

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`enterBootloaderMode`](DepzDevice.md#enterbootloadermode)

***

### isAlive()

```ts
isAlive(): Promise<boolean>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:173](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L173)

True when the sensor answers with the VL53L4CD model id (0xEBAA).

#### Returns

`Promise`\<`boolean`\>

***

### init()

```ts
init(busKhz?): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:187](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L187)

Initialise the sensor: default configuration block + VHV calibration
(ULD sensorInit). Takes well under a second; the bus is left at `busKhz`
(one of I2C_KHZ_STEPS).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `busKhz` | `number` | `I2C_KHZ_DEFAULT` |

#### Returns

`Promise`\<`void`\>

***

### xshut()

```ts
xshut(action): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:200](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L200)

Drive the XSHUT pin: XSHUT_OFF / XSHUT_ON / XSHUT_RESET. OFF and RESET
stop any active stream on the bridge; a power-cycled sensor needs init()
again.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `action` | `number` |

#### Returns

`Promise`\<`void`\>

***

### resetSensor()

```ts
resetSensor(): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:211](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L211)

Hardware sensor reset via XSHUT (blocks ~3 ms on the MCU). The ULD
configuration is wiped — call init() again.

#### Returns

`Promise`\<`void`\>

***

### bridgeInfo()

```ts
bridgeInfo(): Promise<Vl53l4Info>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:222](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L222)

RPT_VL53_INFO: sensor identity, pin levels and bridge counters. Counters
are free-running (wrap silently) — watch increments. Safe to call while
streaming.

#### Returns

`Promise`\<[`Vl53l4Info`](../interfaces/Vl53l4Info.md)\>

***

### setI2cSpeedKhz()

```ts
setI2cSpeedKhz(khz): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:233](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L233)

Re-time the bridge's I2C bus to the nominal step nearest `khz`
(I2C_KHZ_STEPS). Not while ranging — re-timing refuses a transfer in
flight (ERR_BUSY). Read back the programmed step via bridgeInfo().

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `khz` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getRangeTiming()

```ts
getRangeTiming(): Promise<RangeTiming>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:244](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L244)

→ { timingBudgetMs, interMeasurementMs }. interMeasurement 0 means
continuous mode.

#### Returns

`Promise`\<[`RangeTiming`](../interfaces/RangeTiming.md)\>

***

### setRangeTiming()

```ts
setRangeTiming(timingBudgetMs, interMeasurementMs?): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:253](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L253)

Set the timing budget (10–200 ms) and inter-measurement period.
`interMeasurementMs = 0` selects continuous ranging; a value larger than
the budget selects autonomous low-power mode. Not while ranging.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timingBudgetMs` | `number` | `undefined` |
| `interMeasurementMs` | `number` | `0` |

#### Returns

`Promise`\<`void`\>

***

### getOffsetMm()

```ts
getOffsetMm(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:259](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L259)

Configured ranging offset in mm (signed).

#### Returns

`Promise`\<`number`\>

***

### setOffsetMm()

```ts
setOffsetMm(offsetMm): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:264](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L264)

Set the ranging offset correction in mm. Not while ranging.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `offsetMm` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getXtalkKcps()

```ts
getXtalkKcps(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:270](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L270)

Configured crosstalk compensation in kcps (0 = disabled).

#### Returns

`Promise`\<`number`\>

***

### setXtalkKcps()

```ts
setXtalkKcps(xtalkKcps): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:275](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L275)

Set the crosstalk compensation in kcps. Not while ranging.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `xtalkKcps` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getDetectionThresholds()

```ts
getDetectionThresholds(): Promise<{
  distanceLowMm: number;
  distanceHighMm: number;
  window: number;
}>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:284](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L284)

→ { distanceLowMm, distanceHighMm, window }. Window is one of
WINDOW_BELOW / WINDOW_ABOVE / WINDOW_OUT / WINDOW_IN.

#### Returns

`Promise`\<\{
  `distanceLowMm`: `number`;
  `distanceHighMm`: `number`;
  `window`: `number`;
\}\>

***

### setDetectionThresholds()

```ts
setDetectionThresholds(
   distanceLowMm, 
   distanceHighMm, 
window): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:296](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L296)

Program the distance-window interrupt (INT only fires when the window
condition holds). Not while ranging.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `distanceLowMm` | `number` |
| `distanceHighMm` | `number` |
| `window` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getSignalThresholdKcps()

```ts
getSignalThresholdKcps(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:305](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L305)

#### Returns

`Promise`\<`number`\>

***

### setSignalThresholdKcps()

```ts
setSignalThresholdKcps(signalKcps): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:313](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L313)

Discard measurements whose return signal is below `signalKcps`. Not
while ranging.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `signalKcps` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getSigmaThresholdMm()

```ts
getSigmaThresholdMm(): Promise<number>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:318](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L318)

#### Returns

`Promise`\<`number`\>

***

### setSigmaThresholdMm()

```ts
setSigmaThresholdMm(sigmaMm): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:326](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L326)

Discard measurements whose sigma exceeds `sigmaMm` (≤ 16383). Not while
ranging.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `sigmaMm` | `number` |

#### Returns

`Promise`\<`void`\>

***

### startTemperatureUpdate()

```ts
startTemperatureUpdate(): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:335](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L335)

Re-run VHV calibration; recommended after a >8 °C ambient change. Not
while ranging (runs a short ranging burst internally).

#### Returns

`Promise`\<`void`\>

***

### calibrateOffset()

```ts
calibrateOffset(targetDistMm, nbSamples?): Promise<number>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:344](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L344)

Offset calibration against a target at `targetDistMm` (10–1000). Blocks
for the sample burst; returns the offset now programmed.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `targetDistMm` | `number` | `undefined` |
| `nbSamples` | `number` | `20` |

#### Returns

`Promise`\<`number`\>

***

### calibrateXtalk()

```ts
calibrateXtalk(targetDistMm, nbSamples?): Promise<number>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:353](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L353)

Crosstalk calibration against a target at `targetDistMm` (10–5000).
Blocks for the sample burst; returns the xtalk now programmed (kcps).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `targetDistMm` | `number` | `undefined` |
| `nbSamples` | `number` | `20` |

#### Returns

`Promise`\<`number`\>

***

### startRanging()

```ts
startRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:364](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L364)

Start the sensor's ranging loop and arm the MCU stream: one
RPT_VL53_STREAM per INT edge carrying the 17-byte result block.

#### Returns

`Promise`\<`void`\>

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:375](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L375)

#### Returns

`Promise`\<`void`\>

***

### measureOnce()

```ts
measureOnce(timeoutMs?): Promise<Vl53l4Measurement>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:396](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L396)

Single poll-mode measurement: start ranging, wait for data-ready, read
the result block, stop. Rejects while the stream is running.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `1000` |

#### Returns

`Promise`\<[`Vl53l4Measurement`](../interfaces/Vl53l4Measurement.md)\>

***

### onMeasurement()

```ts
onMeasurement(cb): () => void;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L414)

Subscribe to streamed measurements (read-pump context; don't block).
Returns an unsubscribe function.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`m`) => `void` |

#### Returns

() => `void`

***

### measurements()

```ts
measurements(maxsize?): StreamQueue<Vl53l4Measurement>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:426](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L426)

Async iterator over measurements — bounded, drop-oldest (contract 07
§3). The returned queue exposes `droppedCount`; it ends when the device
closes or the consumer breaks out of iteration.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `maxsize` | `number` | `64` |

#### Returns

[`StreamQueue`](StreamQueue.md)\<[`Vl53l4Measurement`](../interfaces/Vl53l4Measurement.md)\>

***

### getMeasurement()

```ts
getMeasurement(timeoutMs?): Promise<Vl53l4Measurement>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:437](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L437)

Convenience: wait for the next streamed measurement.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `2000` |

#### Returns

`Promise`\<[`Vl53l4Measurement`](../interfaces/Vl53l4Measurement.md)\>

***

### requireNotRanging()

```ts
protected requireNotRanging(): void;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:478](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L478)

#### Returns

`void`

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:484](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L484)

Hook for sensor subclasses: route streaming reports here. Return true
when the packet was consumed. Runs after status/matcher/common-report
routing, on the read-pump context.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pkt` | [`PacketEvent`](../interfaces/PacketEvent.md) |

#### Returns

`boolean`

#### Overrides

[`DepzDevice`](DepzDevice.md).[`handleReport`](DepzDevice.md#handlereport)

# Class: Vl53l4cdError

Defined in: [src/sensors/vl53l4/uld.ts:127](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L127)

## Extends

- `Error`

## Constructors

### Constructor

```ts
new Vl53l4cdError(message?): Vl53l4cdError;
```

Defined in: [src/sensors/vl53l4/uld.ts:128](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L128)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `message?` | `string` |

#### Returns

`Vl53l4cdError`

#### Overrides

```ts
Error.constructor
```

## Properties

### stackTraceLimit

```ts
static stackTraceLimit: number;
```

Defined in: node\_modules/@types/node/globals.d.ts:68

The `Error.stackTraceLimit` property specifies the number of stack frames
collected by a stack trace (whether generated by `new Error().stack` or
`Error.captureStackTrace(obj)`).

The default value is `10` but may be set to any valid JavaScript number. Changes
will affect any stack trace captured _after_ the value has been changed.

If set to a non-number value, or set to a negative number, stack traces will
not capture any frames.

#### Inherited from

```ts
Error.stackTraceLimit
```

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

```ts
Error.cause
```

***

### name

```ts
name: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

```ts
Error.name
```

***

### message

```ts
message: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

```ts
Error.message
```

***

### stack?

```ts
optional stack?: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

```ts
Error.stack
```

## Methods

### captureStackTrace()

```ts
static captureStackTrace(targetObject, constructorOpt?): void;
```

Defined in: node\_modules/@types/node/globals.d.ts:52

Creates a `.stack` property on `targetObject`, which when accessed returns
a string representing the location in the code at which
`Error.captureStackTrace()` was called.

```js
const myObject = {};
Error.captureStackTrace(myObject);
myObject.stack;  // Similar to `new Error().stack`
```

The first line of the trace will be prefixed with
`${myObject.name}: ${myObject.message}`.

The optional `constructorOpt` argument accepts a function. If given, all frames
above `constructorOpt`, including `constructorOpt`, will be omitted from the
generated stack trace.

The `constructorOpt` argument is useful for hiding implementation
details of error generation from the user. For instance:

```js
function a() {
  b();
}

function b() {
  c();
}

function c() {
  // Create an error without stack trace to avoid calculating the stack trace twice.
  const { stackTraceLimit } = Error;
  Error.stackTraceLimit = 0;
  const error = new Error();
  Error.stackTraceLimit = stackTraceLimit;

  // Capture the stack trace above function b
  Error.captureStackTrace(error, b); // Neither function c, nor b is included in the stack trace
  throw error;
}

a();
```

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetObject` | `object` |
| `constructorOpt?` | `Function` |

#### Returns

`void`

#### Inherited from

```ts
Error.captureStackTrace
```

***

### prepareStackTrace()

```ts
static prepareStackTrace(err, stackTraces): any;
```

Defined in: node\_modules/@types/node/globals.d.ts:56

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `err` | `Error` |
| `stackTraces` | `CallSite`[] |

#### Returns

`any`

#### See

https://v8.dev/docs/stack-trace-api#customizing-stack-traces

#### Inherited from

```ts
Error.prepareStackTrace
```

# Class: Vl53l8Ch

Defined in: [src/sensors/vl53l8/vl53l8.ts:503](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L503)

VL53L8CH device: the VL53L8CX superset. Inherits every CX method and adds
Compact-Network-Histogram (CNH) output. `init()` downloads the CH firmware
blob (VL53LMZ ULD 2.0.16). CNH is the reason to run CH firmware: each frame
can additionally carry a per-aggregate distance histogram.

## Extends

- [`Vl53l8Cx`](Vl53l8Cx-1.md)

## Constructors

### Constructor

```ts
new Vl53l8Ch(transport, opts?): Vl53l8Ch;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:162](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L162)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | [`SerialTransport`](../interfaces/SerialTransport.md) |
| `opts?` | [`Vl53l8Options`](../interfaces/Vl53l8Options.md) |

#### Returns

`Vl53l8Ch`

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`constructor`](Vl53l8Cx-1.md#constructor)

## Properties

### timeoutMs

```ts
timeoutMs: number;
```

Defined in: [src/device/device.ts:178](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L178)

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`timeoutMs`](Vl53l8Cx-1.md#timeoutms)

***

### link

```ts
protected readonly link: DepzLink;
```

Defined in: [src/device/device.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L180)

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`link`](Vl53l8Cx-1.md#link)

***

### cnhConfig

```ts
protected cnhConfig: CnhConfig | null = null;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:115](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L115)

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`cnhConfig`](Vl53l8Cx-1.md#cnhconfig)

***

### variantId

```ts
protected readonly variantId: Vl53l8Variant = "ch";
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:504](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L504)

Sensor-firmware blob variant this class loads.

#### Overrides

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`variantId`](Vl53l8Cx-1.md#variantid)

## Accessors

### stats

#### Get Signature

```ts
get stats(): LinkStats;
```

Defined in: [src/device/device.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L210)

##### Returns

[`LinkStats`](../interfaces/LinkStats.md)

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`stats`](Vl53l8Cx-1.md#stats)

***

### timeSync

#### Get Signature

```ts
get timeSync(): TimeSync | null;
```

Defined in: [src/device/device.ts:474](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L474)

##### Returns

[`TimeSync`](../interfaces/TimeSync.md) \| `null`

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`timeSync`](Vl53l8Cx-1.md#timesync)

***

### uld

#### Get Signature

```ts
get uld(): VL53L8CX;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L170)

The underlying ULD driver (escape hatch for advanced DCI access).

##### Returns

[`VL53L8CX`](VL53L8CX.md)

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`uld`](Vl53l8Cx-1.md#uld)

***

### variant

#### Get Signature

```ts
get variant(): Vl53l8Variant;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:176](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L176)

'cx' | 'ch' (valid after init()).

##### Returns

[`Vl53l8Variant`](../type-aliases/Vl53l8Variant.md)

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`variant`](Vl53l8Cx-1.md#variant)

***

### ranging

#### Get Signature

```ts
get ranging(): boolean;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:395](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L395)

##### Returns

`boolean`

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`ranging`](Vl53l8Cx-1.md#ranging)

## Methods

### open()

```ts
open(): Promise<void>;
```

Defined in: [src/device/device.ts:200](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L200)

Open the transport and start the read pump.

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`open`](Vl53l8Cx-1.md#open)

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/device/device.ts:205](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L205)

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`close`](Vl53l8Cx-1.md#close)

***

### registerStream()

```ts
protected registerStream(stream): () => void;
```

Defined in: [src/device/device.ts:330](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L330)

Register a stream to be closed on device teardown.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `stream` | \{ `close`: `void`; \} |
| `stream.close` |

#### Returns

() => `void`

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`registerStream`](Vl53l8Cx-1.md#registerstream)

***

### onEvent()

```ts
onEvent(cb): () => void;
```

Defined in: [src/device/device.ts:343](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L343)

Subscribe to unsolicited/diagnostic events (read-pump context; do not
block). Returns an unsubscribe function.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`ev`) => `void` |

#### Returns

() => `void`

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`onEvent`](Vl53l8Cx-1.md#onevent)

***

### emitEvent()

```ts
protected emitEvent(event): void;
```

Defined in: [src/device/device.ts:350](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L350)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `event` | [`DeviceEvent`](../type-aliases/DeviceEvent.md) |

#### Returns

`void`

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`emitEvent`](Vl53l8Cx-1.md#emitevent)

***

### request()

```ts
request<T>(
   cmd, 
   payload?, 
opts?): Promise<T>;
```

Defined in: [src/device/device.ts:366](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L366)

Send `cmd` and wait for its correlated completion (contract 02 §1).

Exactly one of the completion paths must be configured:
- `okCompletes` — RPT_STATUS(cmd, OK) resolves with undefined;
- `matcher` — first packet for which the matcher returns non-`NO_MATCH`
  resolves with the matcher's return value.
Non-OK RPT_STATUS echoing `cmd` always rejects (BusyError for ERR_BUSY,
StatusError otherwise). One in-flight request per opcode.

#### Type Parameters

| Type Parameter | Default type |
| ------ | ------ |
| `T` | `void` |

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cmd` | `number` |
| `payload` | `Uint8Array` |
| `opts` | [`RequestOptions`](../interfaces/RequestOptions.md)\<`T`\> |

#### Returns

`Promise`\<`T`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`request`](Vl53l8Cx-1.md#request)

***

### expectReport()

```ts
static expectReport<T>(reportId, unpack): Matcher<T>;
```

Defined in: [src/device/device.ts:409](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L409)

Matcher for a typed report identified by its report ID alone.

#### Type Parameters

| Type Parameter |
| ------ |
| `T` |

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `reportId` | `number` |
| `unpack` | (`payload`) => `T` |

#### Returns

[`Matcher`](../type-aliases/Matcher.md)\<`T`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`expectReport`](Vl53l8Cx-1.md#expectreport)

***

### expectText()

```ts
static expectText(requestCmd): Matcher<string>;
```

Defined in: [src/device/device.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L414)

Matcher for RPT_TEXT echoing `requestCmd`.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `requestCmd` | `number` |

#### Returns

[`Matcher`](../type-aliases/Matcher.md)\<`string`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`expectText`](Vl53l8Cx-1.md#expecttext)

***

### getDeviceName()

```ts
getDeviceName(): Promise<string>;
```

Defined in: [src/device/device.ts:425](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L425)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`getDeviceName`](Vl53l8Cx-1.md#getdevicename)

***

### getSoftwareName()

```ts
getSoftwareName(): Promise<string>;
```

Defined in: [src/device/device.ts:431](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L431)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`getSoftwareName`](Vl53l8Cx-1.md#getsoftwarename)

***

### getSerialNumber()

```ts
getSerialNumber(): Promise<string>;
```

Defined in: [src/device/device.ts:437](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L437)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`getSerialNumber`](Vl53l8Cx-1.md#getserialnumber)

***

### identify()

```ts
identify(): Promise<Identity>;
```

Defined in: [src/device/device.ts:444](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L444)

Classify the running firmware (contract 02 §4).

#### Returns

`Promise`\<[`Identity`](../interfaces/Identity.md)\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`identify`](Vl53l8Cx-1.md#identify)

***

### readMcuTemperature()

```ts
readMcuTemperature(): Promise<number>;
```

Defined in: [src/device/device.ts:449](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L449)

Last cached MCU temperature in °C (device refreshes ~2 Hz).

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`readMcuTemperature`](Vl53l8Cx-1.md#readmcutemperature)

***

### syncTime()

```ts
syncTime(samples?): Promise<TimeSync>;
```

Defined in: [src/device/device.ts:457](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L457)

NTP-style sync; keeps the lowest-RTT sample (contract 02 §5).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `samples` | `number` | `5` |

#### Returns

`Promise`\<[`TimeSync`](../interfaces/TimeSync.md)\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`syncTime`](Vl53l8Cx-1.md#synctime)

***

### toHostTimeUs()

```ts
toHostTimeUs(deviceTsUs): bigint;
```

Defined in: [src/device/device.ts:479](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L479)

Device µs → host monotonic µs (requires a prior `syncTime`).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `deviceTsUs` | `bigint` |

#### Returns

`bigint`

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`toHostTimeUs`](Vl53l8Cx-1.md#tohosttimeus)

***

### getReportPayloadCrc()

```ts
getReportPayloadCrc(): Promise<CrcType>;
```

Defined in: [src/device/device.ts:484](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L484)

#### Returns

`Promise`\<[`CrcType`](../enumerations/CrcType.md)\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`getReportPayloadCrc`](Vl53l8Cx-1.md#getreportpayloadcrc)

***

### setReportPayloadCrc()

```ts
setReportPayloadCrc(crcType): Promise<void>;
```

Defined in: [src/device/device.ts:491](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L491)

Set the device→host payload CRC mode (host→device is per-packet).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `crcType` | [`CrcType`](../enumerations/CrcType.md) |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`setReportPayloadCrc`](Vl53l8Cx-1.md#setreportpayloadcrc)

***

### getSyncPin()

```ts
getSyncPin(pin): Promise<SyncPinConfig>;
```

Defined in: [src/device/device.ts:495](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L495)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pin` | `number` |

#### Returns

`Promise`\<[`SyncPinConfig`](../interfaces/SyncPinConfig.md)\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`getSyncPin`](Vl53l8Cx-1.md#getsyncpin)

***

### setSyncPin()

```ts
setSyncPin(config): Promise<void>;
```

Defined in: [src/device/device.ts:501](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L501)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `config` | [`SyncPinConfig`](../interfaces/SyncPinConfig.md) |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`setSyncPin`](Vl53l8Cx-1.md#setsyncpin)

***

### reset()

```ts
reset(): Promise<void>;
```

Defined in: [src/device/device.ts:506](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L506)

DEVICE_RESET: device ACKs then reboots; the link will drop.

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`reset`](Vl53l8Cx-1.md#reset)

***

### enterBootloaderMode()

```ts
enterBootloaderMode(): Promise<void>;
```

Defined in: [src/device/device.ts:515](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L515)

Ask the device to reboot into the resident bootloader and close this
connection. Re-discovery/flash flow lives in the bootloader module
(contract 06).

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`enterBootloaderMode`](Vl53l8Cx-1.md#enterbootloadermode)

***

### isAlive()

```ts
isAlive(): Promise<boolean>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L180)

#### Returns

`Promise`\<`boolean`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`isAlive`](Vl53l8Cx-1.md#isalive)

***

### init()

```ts
init(variant?, opts?): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L198)

Initialize the sensor: firmware blob download + default config.

The blob variant is fixed by the class (`Vl53l8Cx` → 'cx', `Vl53l8Ch` →
'ch'); `variant` is accepted only for backward compatibility and must
match the class variant when given. Assets are loaded lazily (dynamic
import) so the blobs stay out of bundles that never init the ToF.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `variant?` | [`Vl53l8Variant`](../type-aliases/Vl53l8Variant.md) |
| `opts?` | [`Vl53l8InitOptions`](../interfaces/Vl53l8InitOptions.md) |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`init`](Vl53l8Cx-1.md#init)

***

### getResolution()

```ts
getResolution(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:218](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L218)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`getResolution`](Vl53l8Cx-1.md#getresolution)

***

### setResolution()

```ts
setResolution(zones): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:223](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L223)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `zones` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`setResolution`](Vl53l8Cx-1.md#setresolution)

***

### getRangingFrequencyHz()

```ts
getRangingFrequencyHz(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:232](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L232)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`getRangingFrequencyHz`](Vl53l8Cx-1.md#getrangingfrequencyhz)

***

### setRangingFrequencyHz()

```ts
setRangingFrequencyHz(hz): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:236](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L236)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `hz` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`setRangingFrequencyHz`](Vl53l8Cx-1.md#setrangingfrequencyhz)

***

### getRangingMode()

```ts
getRangingMode(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:247](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L247)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`getRangingMode`](Vl53l8Cx-1.md#getrangingmode)

***

### setRangingMode()

```ts
setRangingMode(mode): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:251](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L251)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `mode` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`setRangingMode`](Vl53l8Cx-1.md#setrangingmode)

***

### getIntegrationTimeMs()

```ts
getIntegrationTimeMs(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:256](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L256)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`getIntegrationTimeMs`](Vl53l8Cx-1.md#getintegrationtimems)

***

### setIntegrationTimeMs()

```ts
setIntegrationTimeMs(ms): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:260](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L260)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`setIntegrationTimeMs`](Vl53l8Cx-1.md#setintegrationtimems)

***

### getSharpenerPercent()

```ts
getSharpenerPercent(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:265](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L265)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`getSharpenerPercent`](Vl53l8Cx-1.md#getsharpenerpercent)

***

### setSharpenerPercent()

```ts
setSharpenerPercent(pct): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:269](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L269)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pct` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`setSharpenerPercent`](Vl53l8Cx-1.md#setsharpenerpercent)

***

### getTargetOrder()

```ts
getTargetOrder(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:274](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L274)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`getTargetOrder`](Vl53l8Cx-1.md#gettargetorder)

***

### setTargetOrder()

```ts
setTargetOrder(order): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:278](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L278)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `order` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`setTargetOrder`](Vl53l8Cx-1.md#settargetorder)

***

### getPowerMode()

```ts
getPowerMode(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:286](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L286)

POWER_MODE_SLEEP/WAKEUP/DEEP_SLEEP (uld constants).

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`getPowerMode`](Vl53l8Cx-1.md#getpowermode)

***

### setPowerMode()

```ts
setPowerMode(mode): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:294](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L294)

Enter sleep / wake / deep-sleep. Not while ranging. Waking from
DEEP_SLEEP re-downloads the firmware blob (init()).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `mode` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`setPowerMode`](Vl53l8Cx-1.md#setpowermode)

***

### getXtalkMargin()

```ts
getXtalkMargin(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:299](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L299)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`getXtalkMargin`](Vl53l8Cx-1.md#getxtalkmargin)

***

### setXtalkMargin()

```ts
setXtalkMargin(marginKcps): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:303](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L303)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `marginKcps` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`setXtalkMargin`](Vl53l8Cx-1.md#setxtalkmargin)

***

### calibrateXtalk()

```ts
calibrateXtalk(
   reflectancePercent, 
   nbSamples, 
distanceMm): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:314](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L314)

Run on-device crosstalk calibration against a flat target at
`distanceMm` with the given `reflectancePercent` (1..99) averaging
`nbSamples` (1..16). The result is captured into the xtalk buffer; read
it back with getCaldataXtalk(). Blocks several seconds.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `reflectancePercent` | `number` |
| `nbSamples` | `number` |
| `distanceMm` | `number` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`calibrateXtalk`](Vl53l8Cx-1.md#calibratextalk)

***

### getCaldataXtalk()

```ts
getCaldataXtalk(): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:320](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L320)

Read back the 776-byte xtalk calibration blob (save/restore).

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`getCaldataXtalk`](Vl53l8Cx-1.md#getcaldataxtalk)

***

### setCaldataXtalk()

```ts
setCaldataXtalk(blob): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:326](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L326)

Restore a previously saved 776-byte xtalk calibration blob.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `blob` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`setCaldataXtalk`](Vl53l8Cx-1.md#setcaldataxtalk)

***

### getDetectionThresholdsEnable()

```ts
getDetectionThresholdsEnable(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:331](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L331)

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`getDetectionThresholdsEnable`](Vl53l8Cx-1.md#getdetectionthresholdsenable)

***

### setDetectionThresholdsEnable()

```ts
setDetectionThresholdsEnable(enabled): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:335](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L335)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `enabled` | `boolean` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`setDetectionThresholdsEnable`](Vl53l8Cx-1.md#setdetectionthresholdsenable)

***

### getDetectionThresholds()

```ts
getDetectionThresholds(): Promise<DetectionThreshold[]>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:340](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L340)

#### Returns

`Promise`\<[`DetectionThreshold`](../interfaces/DetectionThreshold.md)[]\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`getDetectionThresholds`](Vl53l8Cx-1.md#getdetectionthresholds)

***

### setDetectionThresholds()

```ts
setDetectionThresholds(thresholds): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:349](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L349)

Program the 64 detection thresholds (interrupt-on-threshold). Each entry
carries lowThresh, highThresh, measurement, type, zoneNum, operation (see
uld THRESH_* constants).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `thresholds` | `Partial`\<[`DetectionThreshold`](../interfaces/DetectionThreshold.md)\>[] |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`setDetectionThresholds`](Vl53l8Cx-1.md#setdetectionthresholds)

***

### setDetectionThresholdsAutoStop()

```ts
setDetectionThresholdsAutoStop(autoStop): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:354](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L354)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `autoStop` | `boolean` |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`setDetectionThresholdsAutoStop`](Vl53l8Cx-1.md#setdetectionthresholdsautostop)

***

### configureMotionIndicator()

```ts
configureMotionIndicator(distanceMinMm?, distanceMaxMm?): Promise<MotionConfig>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:364](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L364)

Enable the motion indicator over [distanceMinMm, distanceMaxMm] and
surface motion output in each frame's `.motion`. Returns the underlying
uld MotionConfig for advanced tuning.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `distanceMinMm` | `number` | `400` |
| `distanceMaxMm` | `number` | `1500` |

#### Returns

`Promise`\<[`MotionConfig`](MotionConfig.md)\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`configureMotionIndicator`](Vl53l8Cx-1.md#configuremotionindicator)

***

### startRanging()

```ts
startRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:375](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L375)

Configure the output list, start the sensor and the MCU stream.

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`startRanging`](Vl53l8Cx-1.md#startranging)

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:388](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L388)

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`stopRanging`](Vl53l8Cx-1.md#stopranging)

***

### onFrame()

```ts
onFrame(cb): () => void;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:400](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L400)

Subscribe to parsed frames (read-pump context; don't block).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`frame`) => `void` |

#### Returns

() => `void`

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`onFrame`](Vl53l8Cx-1.md#onframe)

***

### frames()

```ts
frames(maxsize?): StreamQueue<Vl53l8Frame>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:413](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L413)

Async iterator over parsed frames — bounded, drop-oldest (contract 07
§3). Subscribes eagerly at call time — call before or after
startRanging(); it ends when the device closes or the consumer breaks
out of iteration.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `maxsize` | `number` | `8` |

#### Returns

[`StreamQueue`](StreamQueue.md)\<[`Vl53l8Frame`](../interfaces/Vl53l8Frame.md)\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`frames`](Vl53l8Cx-1.md#frames)

***

### getFrame()

```ts
getFrame(timeoutMs?): Promise<Vl53l8Frame>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:424](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L424)

Convenience: wait for the next frame.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `2000` |

#### Returns

`Promise`\<[`Vl53l8Frame`](../interfaces/Vl53l8Frame.md)\>

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`getFrame`](Vl53l8Cx-1.md#getframe)

***

### requireNotRanging()

```ts
protected requireNotRanging(): void;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:452](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L452)

#### Returns

`void`

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`requireNotRanging`](Vl53l8Cx-1.md#requirenotranging)

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:458](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L458)

Hook for sensor subclasses: route streaming reports here. Return true
when the packet was consumed. Runs after status/matcher/common-report
routing, on the read-pump context.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pkt` | [`PacketEvent`](../interfaces/PacketEvent.md) |

#### Returns

`boolean`

#### Inherited from

[`Vl53l8Cx`](Vl53l8Cx-1.md).[`handleReport`](Vl53l8Cx-1.md#handlereport)

***

### configureCnh()

```ts
configureCnh(config): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:510](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L510)

Arm the CNH histogram block for the next startRanging(). CH only — this
method does not exist on Vl53l8Cx.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `config` | [`CnhConfig`](CnhConfig.md) |

#### Returns

`Promise`\<`void`\>

# Class: Vl53l8Cx

Defined in: [src/sensors/vl53l8/vl53l8.ts:106](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L106)

VL53L8CX ToF device: `init()` downloads the ~84 KB sensor firmware (takes a
few seconds over CDC), then configure and `startRanging()`.

This is the base class for both silicon variants. The VL53L8CH superset
(compact-network-histogram output) lives in `Vl53l8Ch`, which inherits every
method here. All configuration methods require `init()` first and must not
be called while ranging (the ULD talks to the current register bank; the
stream owns it — contract 04).

## Extends

- [`DepzDevice`](DepzDevice.md)

## Extended by

- [`Vl53l8Ch`](Vl53l8Ch.md)

## Constructors

### Constructor

```ts
new Vl53l8Cx(transport, opts?): Vl53l8Cx;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:162](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L162)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `transport` | [`SerialTransport`](../interfaces/SerialTransport.md) |
| `opts?` | [`Vl53l8Options`](../interfaces/Vl53l8Options.md) |

#### Returns

`Vl53l8Cx`

#### Overrides

[`DepzDevice`](DepzDevice.md).[`constructor`](DepzDevice.md#constructor)

## Properties

### timeoutMs

```ts
timeoutMs: number;
```

Defined in: [src/device/device.ts:178](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L178)

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`timeoutMs`](DepzDevice.md#timeoutms)

***

### link

```ts
protected readonly link: DepzLink;
```

Defined in: [src/device/device.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L180)

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`link`](DepzDevice.md#link)

***

### variantId

```ts
protected readonly variantId: Vl53l8Variant = "cx";
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:108](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L108)

Sensor-firmware blob variant this class loads.

***

### cnhConfig

```ts
protected cnhConfig: CnhConfig | null = null;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:115](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L115)

## Accessors

### stats

#### Get Signature

```ts
get stats(): LinkStats;
```

Defined in: [src/device/device.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L210)

##### Returns

[`LinkStats`](../interfaces/LinkStats.md)

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`stats`](DepzDevice.md#stats)

***

### timeSync

#### Get Signature

```ts
get timeSync(): TimeSync | null;
```

Defined in: [src/device/device.ts:474](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L474)

##### Returns

[`TimeSync`](../interfaces/TimeSync.md) \| `null`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`timeSync`](DepzDevice.md#timesync)

***

### uld

#### Get Signature

```ts
get uld(): VL53L8CX;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L170)

The underlying ULD driver (escape hatch for advanced DCI access).

##### Returns

[`VL53L8CX`](VL53L8CX.md)

***

### variant

#### Get Signature

```ts
get variant(): Vl53l8Variant;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:176](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L176)

'cx' | 'ch' (valid after init()).

##### Returns

[`Vl53l8Variant`](../type-aliases/Vl53l8Variant.md)

***

### ranging

#### Get Signature

```ts
get ranging(): boolean;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:395](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L395)

##### Returns

`boolean`

## Methods

### open()

```ts
open(): Promise<void>;
```

Defined in: [src/device/device.ts:200](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L200)

Open the transport and start the read pump.

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`open`](DepzDevice.md#open)

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/device/device.ts:205](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L205)

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`close`](DepzDevice.md#close)

***

### registerStream()

```ts
protected registerStream(stream): () => void;
```

Defined in: [src/device/device.ts:330](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L330)

Register a stream to be closed on device teardown.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `stream` | \{ `close`: `void`; \} |
| `stream.close` |

#### Returns

() => `void`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`registerStream`](DepzDevice.md#registerstream)

***

### onEvent()

```ts
onEvent(cb): () => void;
```

Defined in: [src/device/device.ts:343](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L343)

Subscribe to unsolicited/diagnostic events (read-pump context; do not
block). Returns an unsubscribe function.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`ev`) => `void` |

#### Returns

() => `void`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`onEvent`](DepzDevice.md#onevent)

***

### emitEvent()

```ts
protected emitEvent(event): void;
```

Defined in: [src/device/device.ts:350](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L350)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `event` | [`DeviceEvent`](../type-aliases/DeviceEvent.md) |

#### Returns

`void`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`emitEvent`](DepzDevice.md#emitevent)

***

### request()

```ts
request<T>(
   cmd, 
   payload?, 
opts?): Promise<T>;
```

Defined in: [src/device/device.ts:366](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L366)

Send `cmd` and wait for its correlated completion (contract 02 §1).

Exactly one of the completion paths must be configured:
- `okCompletes` — RPT_STATUS(cmd, OK) resolves with undefined;
- `matcher` — first packet for which the matcher returns non-`NO_MATCH`
  resolves with the matcher's return value.
Non-OK RPT_STATUS echoing `cmd` always rejects (BusyError for ERR_BUSY,
StatusError otherwise). One in-flight request per opcode.

#### Type Parameters

| Type Parameter | Default type |
| ------ | ------ |
| `T` | `void` |

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cmd` | `number` |
| `payload` | `Uint8Array` |
| `opts` | [`RequestOptions`](../interfaces/RequestOptions.md)\<`T`\> |

#### Returns

`Promise`\<`T`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`request`](DepzDevice.md#request)

***

### expectReport()

```ts
static expectReport<T>(reportId, unpack): Matcher<T>;
```

Defined in: [src/device/device.ts:409](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L409)

Matcher for a typed report identified by its report ID alone.

#### Type Parameters

| Type Parameter |
| ------ |
| `T` |

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `reportId` | `number` |
| `unpack` | (`payload`) => `T` |

#### Returns

[`Matcher`](../type-aliases/Matcher.md)\<`T`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`expectReport`](DepzDevice.md#expectreport)

***

### expectText()

```ts
static expectText(requestCmd): Matcher<string>;
```

Defined in: [src/device/device.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L414)

Matcher for RPT_TEXT echoing `requestCmd`.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `requestCmd` | `number` |

#### Returns

[`Matcher`](../type-aliases/Matcher.md)\<`string`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`expectText`](DepzDevice.md#expecttext)

***

### getDeviceName()

```ts
getDeviceName(): Promise<string>;
```

Defined in: [src/device/device.ts:425](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L425)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getDeviceName`](DepzDevice.md#getdevicename)

***

### getSoftwareName()

```ts
getSoftwareName(): Promise<string>;
```

Defined in: [src/device/device.ts:431](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L431)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getSoftwareName`](DepzDevice.md#getsoftwarename)

***

### getSerialNumber()

```ts
getSerialNumber(): Promise<string>;
```

Defined in: [src/device/device.ts:437](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L437)

#### Returns

`Promise`\<`string`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getSerialNumber`](DepzDevice.md#getserialnumber)

***

### identify()

```ts
identify(): Promise<Identity>;
```

Defined in: [src/device/device.ts:444](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L444)

Classify the running firmware (contract 02 §4).

#### Returns

`Promise`\<[`Identity`](../interfaces/Identity.md)\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`identify`](DepzDevice.md#identify)

***

### readMcuTemperature()

```ts
readMcuTemperature(): Promise<number>;
```

Defined in: [src/device/device.ts:449](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L449)

Last cached MCU temperature in °C (device refreshes ~2 Hz).

#### Returns

`Promise`\<`number`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`readMcuTemperature`](DepzDevice.md#readmcutemperature)

***

### syncTime()

```ts
syncTime(samples?): Promise<TimeSync>;
```

Defined in: [src/device/device.ts:457](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L457)

NTP-style sync; keeps the lowest-RTT sample (contract 02 §5).

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `samples` | `number` | `5` |

#### Returns

`Promise`\<[`TimeSync`](../interfaces/TimeSync.md)\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`syncTime`](DepzDevice.md#synctime)

***

### toHostTimeUs()

```ts
toHostTimeUs(deviceTsUs): bigint;
```

Defined in: [src/device/device.ts:479](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L479)

Device µs → host monotonic µs (requires a prior `syncTime`).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `deviceTsUs` | `bigint` |

#### Returns

`bigint`

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`toHostTimeUs`](DepzDevice.md#tohosttimeus)

***

### getReportPayloadCrc()

```ts
getReportPayloadCrc(): Promise<CrcType>;
```

Defined in: [src/device/device.ts:484](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L484)

#### Returns

`Promise`\<[`CrcType`](../enumerations/CrcType.md)\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getReportPayloadCrc`](DepzDevice.md#getreportpayloadcrc)

***

### setReportPayloadCrc()

```ts
setReportPayloadCrc(crcType): Promise<void>;
```

Defined in: [src/device/device.ts:491](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L491)

Set the device→host payload CRC mode (host→device is per-packet).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `crcType` | [`CrcType`](../enumerations/CrcType.md) |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`setReportPayloadCrc`](DepzDevice.md#setreportpayloadcrc)

***

### getSyncPin()

```ts
getSyncPin(pin): Promise<SyncPinConfig>;
```

Defined in: [src/device/device.ts:495](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L495)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pin` | `number` |

#### Returns

`Promise`\<[`SyncPinConfig`](../interfaces/SyncPinConfig.md)\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`getSyncPin`](DepzDevice.md#getsyncpin)

***

### setSyncPin()

```ts
setSyncPin(config): Promise<void>;
```

Defined in: [src/device/device.ts:501](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L501)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `config` | [`SyncPinConfig`](../interfaces/SyncPinConfig.md) |

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`setSyncPin`](DepzDevice.md#setsyncpin)

***

### reset()

```ts
reset(): Promise<void>;
```

Defined in: [src/device/device.ts:506](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L506)

DEVICE_RESET: device ACKs then reboots; the link will drop.

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`reset`](DepzDevice.md#reset)

***

### enterBootloaderMode()

```ts
enterBootloaderMode(): Promise<void>;
```

Defined in: [src/device/device.ts:515](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L515)

Ask the device to reboot into the resident bootloader and close this
connection. Re-discovery/flash flow lives in the bootloader module
(contract 06).

#### Returns

`Promise`\<`void`\>

#### Inherited from

[`DepzDevice`](DepzDevice.md).[`enterBootloaderMode`](DepzDevice.md#enterbootloadermode)

***

### isAlive()

```ts
isAlive(): Promise<boolean>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L180)

#### Returns

`Promise`\<`boolean`\>

***

### init()

```ts
init(variant?, opts?): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L198)

Initialize the sensor: firmware blob download + default config.

The blob variant is fixed by the class (`Vl53l8Cx` → 'cx', `Vl53l8Ch` →
'ch'); `variant` is accepted only for backward compatibility and must
match the class variant when given. Assets are loaded lazily (dynamic
import) so the blobs stay out of bundles that never init the ToF.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `variant?` | [`Vl53l8Variant`](../type-aliases/Vl53l8Variant.md) |
| `opts?` | [`Vl53l8InitOptions`](../interfaces/Vl53l8InitOptions.md) |

#### Returns

`Promise`\<`void`\>

***

### getResolution()

```ts
getResolution(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:218](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L218)

#### Returns

`Promise`\<`number`\>

***

### setResolution()

```ts
setResolution(zones): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:223](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L223)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `zones` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getRangingFrequencyHz()

```ts
getRangingFrequencyHz(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:232](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L232)

#### Returns

`Promise`\<`number`\>

***

### setRangingFrequencyHz()

```ts
setRangingFrequencyHz(hz): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:236](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L236)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `hz` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getRangingMode()

```ts
getRangingMode(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:247](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L247)

#### Returns

`Promise`\<`number`\>

***

### setRangingMode()

```ts
setRangingMode(mode): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:251](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L251)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `mode` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getIntegrationTimeMs()

```ts
getIntegrationTimeMs(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:256](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L256)

#### Returns

`Promise`\<`number`\>

***

### setIntegrationTimeMs()

```ts
setIntegrationTimeMs(ms): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:260](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L260)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getSharpenerPercent()

```ts
getSharpenerPercent(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:265](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L265)

#### Returns

`Promise`\<`number`\>

***

### setSharpenerPercent()

```ts
setSharpenerPercent(pct): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:269](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L269)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pct` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getTargetOrder()

```ts
getTargetOrder(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:274](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L274)

#### Returns

`Promise`\<`number`\>

***

### setTargetOrder()

```ts
setTargetOrder(order): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:278](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L278)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `order` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getPowerMode()

```ts
getPowerMode(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:286](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L286)

POWER_MODE_SLEEP/WAKEUP/DEEP_SLEEP (uld constants).

#### Returns

`Promise`\<`number`\>

***

### setPowerMode()

```ts
setPowerMode(mode): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:294](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L294)

Enter sleep / wake / deep-sleep. Not while ranging. Waking from
DEEP_SLEEP re-downloads the firmware blob (init()).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `mode` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getXtalkMargin()

```ts
getXtalkMargin(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:299](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L299)

#### Returns

`Promise`\<`number`\>

***

### setXtalkMargin()

```ts
setXtalkMargin(marginKcps): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:303](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L303)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `marginKcps` | `number` |

#### Returns

`Promise`\<`void`\>

***

### calibrateXtalk()

```ts
calibrateXtalk(
   reflectancePercent, 
   nbSamples, 
distanceMm): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:314](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L314)

Run on-device crosstalk calibration against a flat target at
`distanceMm` with the given `reflectancePercent` (1..99) averaging
`nbSamples` (1..16). The result is captured into the xtalk buffer; read
it back with getCaldataXtalk(). Blocks several seconds.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `reflectancePercent` | `number` |
| `nbSamples` | `number` |
| `distanceMm` | `number` |

#### Returns

`Promise`\<`void`\>

***

### getCaldataXtalk()

```ts
getCaldataXtalk(): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:320](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L320)

Read back the 776-byte xtalk calibration blob (save/restore).

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### setCaldataXtalk()

```ts
setCaldataXtalk(blob): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:326](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L326)

Restore a previously saved 776-byte xtalk calibration blob.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `blob` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

***

### getDetectionThresholdsEnable()

```ts
getDetectionThresholdsEnable(): Promise<number>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:331](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L331)

#### Returns

`Promise`\<`number`\>

***

### setDetectionThresholdsEnable()

```ts
setDetectionThresholdsEnable(enabled): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:335](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L335)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `enabled` | `boolean` |

#### Returns

`Promise`\<`void`\>

***

### getDetectionThresholds()

```ts
getDetectionThresholds(): Promise<DetectionThreshold[]>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:340](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L340)

#### Returns

`Promise`\<[`DetectionThreshold`](../interfaces/DetectionThreshold.md)[]\>

***

### setDetectionThresholds()

```ts
setDetectionThresholds(thresholds): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:349](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L349)

Program the 64 detection thresholds (interrupt-on-threshold). Each entry
carries lowThresh, highThresh, measurement, type, zoneNum, operation (see
uld THRESH_* constants).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `thresholds` | `Partial`\<[`DetectionThreshold`](../interfaces/DetectionThreshold.md)\>[] |

#### Returns

`Promise`\<`void`\>

***

### setDetectionThresholdsAutoStop()

```ts
setDetectionThresholdsAutoStop(autoStop): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:354](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L354)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `autoStop` | `boolean` |

#### Returns

`Promise`\<`void`\>

***

### configureMotionIndicator()

```ts
configureMotionIndicator(distanceMinMm?, distanceMaxMm?): Promise<MotionConfig>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:364](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L364)

Enable the motion indicator over [distanceMinMm, distanceMaxMm] and
surface motion output in each frame's `.motion`. Returns the underlying
uld MotionConfig for advanced tuning.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `distanceMinMm` | `number` | `400` |
| `distanceMaxMm` | `number` | `1500` |

#### Returns

`Promise`\<[`MotionConfig`](MotionConfig.md)\>

***

### startRanging()

```ts
startRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:375](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L375)

Configure the output list, start the sensor and the MCU stream.

#### Returns

`Promise`\<`void`\>

***

### stopRanging()

```ts
stopRanging(): Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:388](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L388)

#### Returns

`Promise`\<`void`\>

***

### onFrame()

```ts
onFrame(cb): () => void;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:400](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L400)

Subscribe to parsed frames (read-pump context; don't block).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`frame`) => `void` |

#### Returns

() => `void`

***

### frames()

```ts
frames(maxsize?): StreamQueue<Vl53l8Frame>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:413](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L413)

Async iterator over parsed frames — bounded, drop-oldest (contract 07
§3). Subscribes eagerly at call time — call before or after
startRanging(); it ends when the device closes or the consumer breaks
out of iteration.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `maxsize` | `number` | `8` |

#### Returns

[`StreamQueue`](StreamQueue.md)\<[`Vl53l8Frame`](../interfaces/Vl53l8Frame.md)\>

***

### getFrame()

```ts
getFrame(timeoutMs?): Promise<Vl53l8Frame>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:424](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L424)

Convenience: wait for the next frame.

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timeoutMs` | `number` | `2000` |

#### Returns

`Promise`\<[`Vl53l8Frame`](../interfaces/Vl53l8Frame.md)\>

***

### requireNotRanging()

```ts
protected requireNotRanging(): void;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:452](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L452)

#### Returns

`void`

***

### handleReport()

```ts
protected handleReport(pkt): boolean;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:458](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L458)

Hook for sensor subclasses: route streaming reports here. Return true
when the packet was consumed. Runs after status/matcher/common-report
routing, on the read-pump context.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `pkt` | [`PacketEvent`](../interfaces/PacketEvent.md) |

#### Returns

`boolean`

#### Overrides

[`DepzDevice`](DepzDevice.md).[`handleReport`](DepzDevice.md#handlereport)

# Class: Vl53l8cxError

Defined in: [src/sensors/vl53l8/uld.ts:179](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L179)

## Extends

- `Error`

## Constructors

### Constructor

```ts
new Vl53l8cxError(code, where?): Vl53l8cxError;
```

Defined in: [src/sensors/vl53l8/uld.ts:183](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L183)

#### Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `code` | `number` | `undefined` |
| `where` | `string` | `""` |

#### Returns

`Vl53l8cxError`

#### Overrides

```ts
Error.constructor
```

## Properties

### code

```ts
readonly code: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L180)

***

### where

```ts
readonly where: string;
```

Defined in: [src/sensors/vl53l8/uld.ts:181](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L181)

***

### stackTraceLimit

```ts
static stackTraceLimit: number;
```

Defined in: node\_modules/@types/node/globals.d.ts:68

The `Error.stackTraceLimit` property specifies the number of stack frames
collected by a stack trace (whether generated by `new Error().stack` or
`Error.captureStackTrace(obj)`).

The default value is `10` but may be set to any valid JavaScript number. Changes
will affect any stack trace captured _after_ the value has been changed.

If set to a non-number value, or set to a negative number, stack traces will
not capture any frames.

#### Inherited from

```ts
Error.stackTraceLimit
```

***

### cause?

```ts
optional cause?: unknown;
```

Defined in: node\_modules/typescript/lib/lib.es2022.error.d.ts:26

#### Inherited from

```ts
Error.cause
```

***

### name

```ts
name: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1076

#### Inherited from

```ts
Error.name
```

***

### message

```ts
message: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1077

#### Inherited from

```ts
Error.message
```

***

### stack?

```ts
optional stack?: string;
```

Defined in: node\_modules/typescript/lib/lib.es5.d.ts:1078

#### Inherited from

```ts
Error.stack
```

## Methods

### captureStackTrace()

```ts
static captureStackTrace(targetObject, constructorOpt?): void;
```

Defined in: node\_modules/@types/node/globals.d.ts:52

Creates a `.stack` property on `targetObject`, which when accessed returns
a string representing the location in the code at which
`Error.captureStackTrace()` was called.

```js
const myObject = {};
Error.captureStackTrace(myObject);
myObject.stack;  // Similar to `new Error().stack`
```

The first line of the trace will be prefixed with
`${myObject.name}: ${myObject.message}`.

The optional `constructorOpt` argument accepts a function. If given, all frames
above `constructorOpt`, including `constructorOpt`, will be omitted from the
generated stack trace.

The `constructorOpt` argument is useful for hiding implementation
details of error generation from the user. For instance:

```js
function a() {
  b();
}

function b() {
  c();
}

function c() {
  // Create an error without stack trace to avoid calculating the stack trace twice.
  const { stackTraceLimit } = Error;
  Error.stackTraceLimit = 0;
  const error = new Error();
  Error.stackTraceLimit = stackTraceLimit;

  // Capture the stack trace above function b
  Error.captureStackTrace(error, b); // Neither function c, nor b is included in the stack trace
  throw error;
}

a();
```

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `targetObject` | `object` |
| `constructorOpt?` | `Function` |

#### Returns

`void`

#### Inherited from

```ts
Error.captureStackTrace
```

***

### prepareStackTrace()

```ts
static prepareStackTrace(err, stackTraces): any;
```

Defined in: node\_modules/@types/node/globals.d.ts:56

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `err` | `Error` |
| `stackTraces` | `CallSite`[] |

#### Returns

`any`

#### See

https://v8.dev/docs/stack-trace-api#customizing-stack-traces

#### Inherited from

```ts
Error.prepareStackTrace
```

# Class: WsBackendTransport

Defined in: [src/transport/ws-backend.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L105)

## Implements

- [`SerialTransport`](../interfaces/SerialTransport.md)

## Constructors

### Constructor

```ts
new WsBackendTransport(opts): WsBackendTransport;
```

Defined in: [src/transport/ws-backend.ts:124](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L124)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `opts` | [`WsBackendTransportOptions`](../interfaces/WsBackendTransportOptions.md) |

#### Returns

`WsBackendTransport`

## Properties

### info

```ts
readonly info: SerialTransportInfo;
```

Defined in: [src/transport/ws-backend.ts:106](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L106)

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`info`](../interfaces/SerialTransport.md#info)

## Methods

### open()

```ts
open(opts?): Promise<void>;
```

Defined in: [src/transport/ws-backend.ts:139](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L139)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `opts?` | \{ `baudRate?`: `number`; \} |
| `opts.baudRate?` | `number` |

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`open`](../interfaces/SerialTransport.md#open)

***

### write()

```ts
write(data): Promise<void>;
```

Defined in: [src/transport/ws-backend.ts:217](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L217)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`write`](../interfaces/SerialTransport.md#write)

***

### readable()

```ts
readable(): AsyncIterableIterator<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/transport/ws-backend.ts:226](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L226)

Raw chunks as they arrive; ends on close/disconnect.

#### Returns

`AsyncIterableIterator`\<`Uint8Array`\<`ArrayBufferLike`\>\>

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`readable`](../interfaces/SerialTransport.md#readable)

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/transport/ws-backend.ts:237](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L237)

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`close`](../interfaces/SerialTransport.md#close)

***

### onDisconnect()

```ts
onDisconnect(cb): () => void;
```

Defined in: [src/transport/ws-backend.ts:248](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L248)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | () => `void` |

#### Returns

() => `void`

#### Implementation of

[`SerialTransport`](../interfaces/SerialTransport.md).[`onDisconnect`](../interfaces/SerialTransport.md#ondisconnect)

# Enumeration: Bno086Cmd

Defined in: [src/protocol/bno086.ts:12](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L12)

BNO086 bridge-level wire codecs (contracts/05_SENSOR_BNO086.md §1–2).

The MCU is a thin SHTP pass-through: SEND_SHTP_PACKET carries a raw SHTP
frame to the sensor, and every inbound SHTP frame arrives as RPT_DATA.
Per ERRATA E2 the RPT_DATA `cmd` echo is always 0x00 — correlation happens
at the SH-2 layer, never here.

Mirrors the Python reference `depz_sensor_sdk.protocol.bno086`.

## Enumeration Members

### SensorReset

```ts
SensorReset: 50;
```

Defined in: [src/protocol/bno086.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L14)

Hardware reset via nRST; RPT_STATUS OK.

***

### SensorWakeUp

```ts
SensorWakeUp: 51;
```

Defined in: [src/protocol/bno086.ts:16](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L16)

1 ms WAKE (PS0) pulse; RPT_STATUS OK.

***

### SendShtpPacket

```ts
SendShtpPacket: 52;
```

Defined in: [src/protocol/bno086.ts:18](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L18)

Payload = raw SHTP frame; RPT_STATUS OK/ERR_BUSY.

# Enumeration: Bno086Rpt

Defined in: [src/protocol/bno086.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L21)

## Enumeration Members

### Data

```ts
Data: 145;
```

Defined in: [src/protocol/bno086.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L23)

rpt_data_t: cmd u8, timestamp_us u64, raw SHTP frame.

# Enumeration: Cmd

Defined in: [src/protocol/common.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L8)

Common command/report IDs and payload codecs
(contracts/02_COMMON_COMMANDS.md). Raw wire integers only; unit
conversions (0.1 °C, µs) happen in the device layer. u64 timestamps are
`bigint`.

## Enumeration Members

### Bootloader

```ts
Bootloader: 1;
```

Defined in: [src/protocol/common.ts:9](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L9)

***

### DeviceReset

```ts
DeviceReset: 2;
```

Defined in: [src/protocol/common.ts:10](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L10)

***

### GetDeviceName

```ts
GetDeviceName: 3;
```

Defined in: [src/protocol/common.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L11)

***

### GetNameActiveSoftware

```ts
GetNameActiveSoftware: 4;
```

Defined in: [src/protocol/common.ts:12](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L12)

***

### GetSerial

```ts
GetSerial: 5;
```

Defined in: [src/protocol/common.ts:13](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L13)

***

### SyncTime

```ts
SyncTime: 6;
```

Defined in: [src/protocol/common.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L14)

***

### GetMcuTemperature

```ts
GetMcuTemperature: 7;
```

Defined in: [src/protocol/common.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L15)

***

### GetPayloadCrcType

```ts
GetPayloadCrcType: 8;
```

Defined in: [src/protocol/common.ts:16](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L16)

***

### SetPayloadCrcType

```ts
SetPayloadCrcType: 9;
```

Defined in: [src/protocol/common.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L17)

***

### ThroughputTxStart

```ts
ThroughputTxStart: 28;
```

Defined in: [src/protocol/common.ts:18](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L18)

***

### ThroughputTxStop

```ts
ThroughputTxStop: 29;
```

Defined in: [src/protocol/common.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L19)

***

### ThroughputRxData

```ts
ThroughputRxData: 30;
```

Defined in: [src/protocol/common.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L20)

***

### GetSyncPinConfig

```ts
GetSyncPinConfig: 48;
```

Defined in: [src/protocol/common.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L21)

***

### SetSyncPinConfig

```ts
SetSyncPinConfig: 49;
```

Defined in: [src/protocol/common.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L22)

# Enumeration: ControlReport

Defined in: [src/sensors/bno086/sh2.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L17)

Report IDs on SHTP channel 2 (control).

## Enumeration Members

### CommandResponse

```ts
CommandResponse: 241;
```

Defined in: [src/sensors/bno086/sh2.ts:18](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L18)

***

### CommandRequest

```ts
CommandRequest: 242;
```

Defined in: [src/sensors/bno086/sh2.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L19)

***

### FrsReadResponse

```ts
FrsReadResponse: 243;
```

Defined in: [src/sensors/bno086/sh2.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L20)

***

### FrsReadRequest

```ts
FrsReadRequest: 244;
```

Defined in: [src/sensors/bno086/sh2.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L21)

***

### FrsWriteResponse

```ts
FrsWriteResponse: 245;
```

Defined in: [src/sensors/bno086/sh2.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L22)

***

### FrsWriteData

```ts
FrsWriteData: 246;
```

Defined in: [src/sensors/bno086/sh2.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L23)

***

### FrsWriteRequest

```ts
FrsWriteRequest: 247;
```

Defined in: [src/sensors/bno086/sh2.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L24)

***

### ProductIdResponse

```ts
ProductIdResponse: 248;
```

Defined in: [src/sensors/bno086/sh2.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L25)

***

### ProductIdRequest

```ts
ProductIdRequest: 249;
```

Defined in: [src/sensors/bno086/sh2.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L26)

***

### GetFeatureResponse

```ts
GetFeatureResponse: 252;
```

Defined in: [src/sensors/bno086/sh2.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L27)

***

### SetFeatureCommand

```ts
SetFeatureCommand: 253;
```

Defined in: [src/sensors/bno086/sh2.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L28)

***

### GetFeatureRequest

```ts
GetFeatureRequest: 254;
```

Defined in: [src/sensors/bno086/sh2.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L29)

# Enumeration: CrcType

Defined in: [src/protocol/framing.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L15)

## Enumeration Members

### None

```ts
None: 0;
```

Defined in: [src/protocol/framing.ts:16](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L16)

***

### Crc8

```ts
Crc8: 1;
```

Defined in: [src/protocol/framing.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L17)

***

### Crc16

```ts
Crc16: 2;
```

Defined in: [src/protocol/framing.ts:18](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L18)

***

### Crc32

```ts
Crc32: 3;
```

Defined in: [src/protocol/framing.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L19)

# Enumeration: ErrorSource

Defined in: [src/sensors/bno086/sh2.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L53)

`source` field of an error record (SH-2 §6.4.1).

## Enumeration Members

### MotionEngine

```ts
MotionEngine: 1;
```

Defined in: [src/sensors/bno086/sh2.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L54)

***

### MotionHub

```ts
MotionHub: 2;
```

Defined in: [src/sensors/bno086/sh2.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L55)

***

### SensorHub

```ts
SensorHub: 3;
```

Defined in: [src/sensors/bno086/sh2.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L56)

***

### Chip

```ts
Chip: 4;
```

Defined in: [src/sensors/bno086/sh2.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L57)

***

### NoMoreErrors

```ts
NoMoreErrors: 255;
```

Defined in: [src/sensors/bno086/sh2.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L59)

Sentinel: end of the error queue.

# Enumeration: FrsRecordId

Defined in: [src/sensors/bno086/sh2.ts:348](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L348)

FRS record IDs used by this SDK (SH-2 figure 28; metadata records).

## Enumeration Members

### StaticCalibrationAgm

```ts
StaticCalibrationAgm: 31097;
```

Defined in: [src/sensors/bno086/sh2.ts:349](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L349)

***

### NominalCalibration

```ts
NominalCalibration: 19789;
```

Defined in: [src/sensors/bno086/sh2.ts:350](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L350)

***

### DynamicCalibration

```ts
DynamicCalibration: 7967;
```

Defined in: [src/sensors/bno086/sh2.ts:351](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L351)

***

### MePowerMgmt

```ts
MePowerMgmt: 54242;
```

Defined in: [src/sensors/bno086/sh2.ts:352](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L352)

***

### SystemOrientation

```ts
SystemOrientation: 11582;
```

Defined in: [src/sensors/bno086/sh2.ts:354](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L354)

Mounting quaternion, 4 × Q30 words.

***

### AccelOrientation

```ts
AccelOrientation: 11585;
```

Defined in: [src/sensors/bno086/sh2.ts:355](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L355)

***

### GyroscopeOrientation

```ts
GyroscopeOrientation: 11590;
```

Defined in: [src/sensors/bno086/sh2.ts:356](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L356)

***

### MagnetometerOrientation

```ts
MagnetometerOrientation: 11596;
```

Defined in: [src/sensors/bno086/sh2.ts:357](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L357)

***

### ArvrStabilizationRv

```ts
ArvrStabilizationRv: 15917;
```

Defined in: [src/sensors/bno086/sh2.ts:358](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L358)

***

### ArvrStabilizationGrv

```ts
ArvrStabilizationGrv: 15918;
```

Defined in: [src/sensors/bno086/sh2.ts:359](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L359)

***

### SigMotionDetectConfig

```ts
SigMotionDetectConfig: 49780;
```

Defined in: [src/sensors/bno086/sh2.ts:361](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L361)

***

### ShakeDetectConfig

```ts
ShakeDetectConfig: 32125;
```

Defined in: [src/sensors/bno086/sh2.ts:362](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L362)

***

### StabilityDetectorConfig

```ts
StabilityDetectorConfig: 60805;
```

Defined in: [src/sensors/bno086/sh2.ts:363](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L363)

***

### ActivityTrackerConfig

```ts
ActivityTrackerConfig: 60808;
```

Defined in: [src/sensors/bno086/sh2.ts:365](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L365)

Personal-activity-classifier config.

# Enumeration: FrsStatus

Defined in: [src/sensors/bno086/sh2.ts:396](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L396)

FRS Read Response status (low nibble of the len/status byte).

## Enumeration Members

### NoError

```ts
NoError: 0;
```

Defined in: [src/sensors/bno086/sh2.ts:397](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L397)

***

### UnrecognizedFrsType

```ts
UnrecognizedFrsType: 1;
```

Defined in: [src/sensors/bno086/sh2.ts:398](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L398)

***

### Busy

```ts
Busy: 2;
```

Defined in: [src/sensors/bno086/sh2.ts:399](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L399)

***

### ReadCompleted

```ts
ReadCompleted: 3;
```

Defined in: [src/sensors/bno086/sh2.ts:400](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L400)

***

### OffsetOutOfRange

```ts
OffsetOutOfRange: 4;
```

Defined in: [src/sensors/bno086/sh2.ts:401](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L401)

***

### RecordEmpty

```ts
RecordEmpty: 5;
```

Defined in: [src/sensors/bno086/sh2.ts:402](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L402)

***

### BlockCompleted

```ts
BlockCompleted: 6;
```

Defined in: [src/sensors/bno086/sh2.ts:403](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L403)

***

### BlockAndReadCompleted

```ts
BlockAndReadCompleted: 7;
```

Defined in: [src/sensors/bno086/sh2.ts:404](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L404)

***

### DeviceError

```ts
DeviceError: 8;
```

Defined in: [src/sensors/bno086/sh2.ts:405](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L405)

# Enumeration: FrsWriteStatus

Defined in: [src/sensors/bno086/sh2.ts:408](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L408)

## Enumeration Members

### WordsReceived

```ts
WordsReceived: 0;
```

Defined in: [src/sensors/bno086/sh2.ts:409](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L409)

***

### UnrecognizedFrsType

```ts
UnrecognizedFrsType: 1;
```

Defined in: [src/sensors/bno086/sh2.ts:410](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L410)

***

### Busy

```ts
Busy: 2;
```

Defined in: [src/sensors/bno086/sh2.ts:411](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L411)

***

### WriteCompleted

```ts
WriteCompleted: 3;
```

Defined in: [src/sensors/bno086/sh2.ts:412](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L412)

***

### WriteModeReady

```ts
WriteModeReady: 4;
```

Defined in: [src/sensors/bno086/sh2.ts:413](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L413)

***

### WriteFailed

```ts
WriteFailed: 5;
```

Defined in: [src/sensors/bno086/sh2.ts:414](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L414)

***

### NotInWriteMode

```ts
NotInWriteMode: 6;
```

Defined in: [src/sensors/bno086/sh2.ts:415](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L415)

***

### InvalidLength

```ts
InvalidLength: 7;
```

Defined in: [src/sensors/bno086/sh2.ts:416](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L416)

***

### RecordValid

```ts
RecordValid: 8;
```

Defined in: [src/sensors/bno086/sh2.ts:417](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L417)

***

### RecordInvalid

```ts
RecordInvalid: 9;
```

Defined in: [src/sensors/bno086/sh2.ts:418](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L418)

# Enumeration: OscillatorType

Defined in: [src/sensors/bno086/sh2.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L46)

Get-Oscillator-Type (command 0x0A) result (r[0]).

## Enumeration Members

### Internal

```ts
Internal: 0;
```

Defined in: [src/sensors/bno086/sh2.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L47)

***

### ExtCrystal

```ts
ExtCrystal: 1;
```

Defined in: [src/sensors/bno086/sh2.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L48)

***

### ExtClock

```ts
ExtClock: 2;
```

Defined in: [src/sensors/bno086/sh2.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L49)

# Enumeration: Rpt

Defined in: [src/protocol/common.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L25)

## Enumeration Members

### Status

```ts
Status: 128;
```

Defined in: [src/protocol/common.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L26)

***

### Text

```ts
Text: 129;
```

Defined in: [src/protocol/common.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L27)

***

### SyncTime

```ts
SyncTime: 130;
```

Defined in: [src/protocol/common.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L28)

***

### Temperature

```ts
Temperature: 131;
```

Defined in: [src/protocol/common.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L29)

***

### SequenceError

```ts
SequenceError: 132;
```

Defined in: [src/protocol/common.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L30)

***

### PayloadCrcType

```ts
PayloadCrcType: 135;
```

Defined in: [src/protocol/common.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L31)

***

### ThroughputData

```ts
ThroughputData: 136;
```

Defined in: [src/protocol/common.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L32)

***

### SyncPinConfig

```ts
SyncPinConfig: 144;
```

Defined in: [src/protocol/common.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L33)

# Enumeration: SensorId

Defined in: [src/sensors/bno086/reports.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L20)

SH-2 input report IDs (datasheet §1.3.5, sh2 reference driver).

## Enumeration Members

### Accelerometer

```ts
Accelerometer: 1;
```

Defined in: [src/sensors/bno086/reports.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L21)

***

### Gyroscope

```ts
Gyroscope: 2;
```

Defined in: [src/sensors/bno086/reports.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L22)

***

### Magnetometer

```ts
Magnetometer: 3;
```

Defined in: [src/sensors/bno086/reports.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L23)

***

### LinearAcceleration

```ts
LinearAcceleration: 4;
```

Defined in: [src/sensors/bno086/reports.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L24)

***

### RotationVector

```ts
RotationVector: 5;
```

Defined in: [src/sensors/bno086/reports.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L25)

***

### Gravity

```ts
Gravity: 6;
```

Defined in: [src/sensors/bno086/reports.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L26)

***

### UncalibratedGyroscope

```ts
UncalibratedGyroscope: 7;
```

Defined in: [src/sensors/bno086/reports.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L27)

***

### GameRotationVector

```ts
GameRotationVector: 8;
```

Defined in: [src/sensors/bno086/reports.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L28)

***

### GeomagneticRotationVector

```ts
GeomagneticRotationVector: 9;
```

Defined in: [src/sensors/bno086/reports.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L29)

***

### Pressure

```ts
Pressure: 10;
```

Defined in: [src/sensors/bno086/reports.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L30)

***

### AmbientLight

```ts
AmbientLight: 11;
```

Defined in: [src/sensors/bno086/reports.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L31)

***

### Humidity

```ts
Humidity: 12;
```

Defined in: [src/sensors/bno086/reports.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L32)

***

### Proximity

```ts
Proximity: 13;
```

Defined in: [src/sensors/bno086/reports.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L33)

***

### Temperature

```ts
Temperature: 14;
```

Defined in: [src/sensors/bno086/reports.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L34)

***

### UncalibratedMagnetometer

```ts
UncalibratedMagnetometer: 15;
```

Defined in: [src/sensors/bno086/reports.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L35)

***

### TapDetector

```ts
TapDetector: 16;
```

Defined in: [src/sensors/bno086/reports.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L36)

***

### StepCounter

```ts
StepCounter: 17;
```

Defined in: [src/sensors/bno086/reports.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L37)

***

### SignificantMotion

```ts
SignificantMotion: 18;
```

Defined in: [src/sensors/bno086/reports.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L38)

***

### StabilityClassifier

```ts
StabilityClassifier: 19;
```

Defined in: [src/sensors/bno086/reports.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L39)

***

### RawAccelerometer

```ts
RawAccelerometer: 20;
```

Defined in: [src/sensors/bno086/reports.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L40)

***

### RawGyroscope

```ts
RawGyroscope: 21;
```

Defined in: [src/sensors/bno086/reports.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L41)

***

### RawMagnetometer

```ts
RawMagnetometer: 22;
```

Defined in: [src/sensors/bno086/reports.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L42)

***

### StepDetector

```ts
StepDetector: 24;
```

Defined in: [src/sensors/bno086/reports.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L43)

***

### ShakeDetector

```ts
ShakeDetector: 25;
```

Defined in: [src/sensors/bno086/reports.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L44)

***

### FlipDetector

```ts
FlipDetector: 26;
```

Defined in: [src/sensors/bno086/reports.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L45)

***

### PickupDetector

```ts
PickupDetector: 27;
```

Defined in: [src/sensors/bno086/reports.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L46)

***

### StabilityDetector

```ts
StabilityDetector: 28;
```

Defined in: [src/sensors/bno086/reports.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L47)

***

### PersonalActivityClassifier

```ts
PersonalActivityClassifier: 30;
```

Defined in: [src/sensors/bno086/reports.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L48)

***

### SleepDetector

```ts
SleepDetector: 31;
```

Defined in: [src/sensors/bno086/reports.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L49)

***

### TiltDetector

```ts
TiltDetector: 32;
```

Defined in: [src/sensors/bno086/reports.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L50)

***

### PocketDetector

```ts
PocketDetector: 33;
```

Defined in: [src/sensors/bno086/reports.ts:51](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L51)

***

### CircleDetector

```ts
CircleDetector: 34;
```

Defined in: [src/sensors/bno086/reports.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L52)

***

### HeartRateMonitor

```ts
HeartRateMonitor: 35;
```

Defined in: [src/sensors/bno086/reports.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L53)

***

### ArvrStabilizedRv

```ts
ArvrStabilizedRv: 40;
```

Defined in: [src/sensors/bno086/reports.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L54)

***

### ArvrStabilizedGameRv

```ts
ArvrStabilizedGameRv: 41;
```

Defined in: [src/sensors/bno086/reports.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L55)

***

### GyroIntegratedRv

```ts
GyroIntegratedRv: 42;
```

Defined in: [src/sensors/bno086/reports.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L56)

# Enumeration: Sh2Command

Defined in: [src/sensors/bno086/sh2.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L33)

`command` field of Command Request/Response (0xF2/0xF1).

## Enumeration Members

### Errors

```ts
Errors: 1;
```

Defined in: [src/sensors/bno086/sh2.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L34)

***

### Counter

```ts
Counter: 2;
```

Defined in: [src/sensors/bno086/sh2.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L35)

***

### Tare

```ts
Tare: 3;
```

Defined in: [src/sensors/bno086/sh2.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L36)

***

### Initialize

```ts
Initialize: 4;
```

Defined in: [src/sensors/bno086/sh2.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L37)

***

### SaveDcd

```ts
SaveDcd: 6;
```

Defined in: [src/sensors/bno086/sh2.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L38)

***

### MeCalibrate

```ts
MeCalibrate: 7;
```

Defined in: [src/sensors/bno086/sh2.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L39)

***

### PeriodicDcdConfig

```ts
PeriodicDcdConfig: 9;
```

Defined in: [src/sensors/bno086/sh2.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L40)

***

### GetOscillatorType

```ts
GetOscillatorType: 10;
```

Defined in: [src/sensors/bno086/sh2.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L41)

***

### ClearDcdAndReset

```ts
ClearDcdAndReset: 11;
```

Defined in: [src/sensors/bno086/sh2.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L42)

# Enumeration: ShtpChannel

Defined in: [src/sensors/bno086/shtp.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L21)

SHTP framing layer for the BNO086 (contracts/05_SENSOR_BNO086.md §3).

Pure codec — no I/O. A frame is a 4-byte header plus a cargo fragment:

    length u16 LE  — bits 14:0 cargo length *including* the 4-byte header;
                     bit 15 set marks a continuation fragment
    channel u8     — see ShtpChannel
    seq u8         — per-channel, per-direction free-running counter

For a cargo that spans several bridge frames, the first fragment's length
field carries the TOTAL cargo length (header included) even though the
frame itself holds fewer bytes; each continuation fragment carries the
remaining length (its own header included) with bit 15 set. The receiver
trusts the first fragment's total and the actual frame sizes; continuation
length fields are informative only.

Mirrors the Python reference `depz_sensor_sdk.bno086.shtp`.

## Enumeration Members

### Command

```ts
Command: 0;
```

Defined in: [src/sensors/bno086/shtp.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L23)

SHTP command channel (advertisements).

***

### Executable

```ts
Executable: 1;
```

Defined in: [src/sensors/bno086/shtp.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L25)

Device executable: reset/on/sleep; RX 0x01 = reset done.

***

### Control

```ts
Control: 2;
```

Defined in: [src/sensors/bno086/shtp.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L27)

SH-2 control: feature/FRS/command reports.

***

### InputNormal

```ts
InputNormal: 3;
```

Defined in: [src/sensors/bno086/shtp.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L29)

Non-wake input reports (0xFB timebase + sensors).

***

### InputWake

```ts
InputWake: 4;
```

Defined in: [src/sensors/bno086/shtp.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L31)

Wake input reports (same cargo format as channel 3).

***

### GyroRv

```ts
GyroRv: 5;
```

Defined in: [src/sensors/bno086/shtp.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L33)

Gyro-integrated rotation vector, dense format.

# Enumeration: Sr04Cmd

Defined in: [src/protocol/sr04.ts:3](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L3)

SR04 wire codecs (contracts/03_SENSOR_SR04.md).

## Enumeration Members

### GetSamplePeriod

```ts
GetSamplePeriod: 50;
```

Defined in: [src/protocol/sr04.ts:4](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L4)

***

### SetSamplePeriod

```ts
SetSamplePeriod: 51;
```

Defined in: [src/protocol/sr04.ts:5](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L5)

***

### GetEchoDecay

```ts
GetEchoDecay: 52;
```

Defined in: [src/protocol/sr04.ts:6](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L6)

***

### SetEchoDecay

```ts
SetEchoDecay: 53;
```

Defined in: [src/protocol/sr04.ts:7](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L7)

***

### MeasureOnce

```ts
MeasureOnce: 54;
```

Defined in: [src/protocol/sr04.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L8)

***

### StartMeasurementLoop

```ts
StartMeasurementLoop: 55;
```

Defined in: [src/protocol/sr04.ts:9](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L9)

***

### StopMeasurementLoop

```ts
StopMeasurementLoop: 56;
```

Defined in: [src/protocol/sr04.ts:10](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L10)

# Enumeration: Sr04Rpt

Defined in: [src/protocol/sr04.ts:13](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L13)

## Enumeration Members

### Data

```ts
Data: 145;
```

Defined in: [src/protocol/sr04.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L14)

***

### SamplePeriod

```ts
SamplePeriod: 146;
```

Defined in: [src/protocol/sr04.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L15)

***

### EchoDecay

```ts
EchoDecay: 147;
```

Defined in: [src/protocol/sr04.ts:16](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L16)

# Enumeration: Status

Defined in: [src/protocol/common.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L36)

## Enumeration Members

### Ok

```ts
Ok: 0;
```

Defined in: [src/protocol/common.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L37)

***

### Error

```ts
Error: 1;
```

Defined in: [src/protocol/common.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L38)

***

### ErrInvalidCmd

```ts
ErrInvalidCmd: 2;
```

Defined in: [src/protocol/common.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L39)

***

### ErrPayloadFormat

```ts
ErrPayloadFormat: 3;
```

Defined in: [src/protocol/common.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L40)

***

### ErrInvalidParam

```ts
ErrInvalidParam: 4;
```

Defined in: [src/protocol/common.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L41)

***

### ErrPayloadCrc

```ts
ErrPayloadCrc: 5;
```

Defined in: [src/protocol/common.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L42)

***

### ErrBusy

```ts
ErrBusy: 6;
```

Defined in: [src/protocol/common.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L43)

***

### ErrCmdNotSupported

```ts
ErrCmdNotSupported: 7;
```

Defined in: [src/protocol/common.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L44)

***

### ErrNotInitialized

```ts
ErrNotInitialized: 8;
```

Defined in: [src/protocol/common.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L45)

***

### ErrHardwareFault

```ts
ErrHardwareFault: 9;
```

Defined in: [src/protocol/common.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L46)

# Enumeration: SyncPinMode

Defined in: [src/protocol/common.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L49)

## Enumeration Members

### Disable

```ts
Disable: 0;
```

Defined in: [src/protocol/common.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L50)

***

### In

```ts
In: 1;
```

Defined in: [src/protocol/common.ts:51](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L51)

***

### OutStart

```ts
OutStart: 2;
```

Defined in: [src/protocol/common.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L52)

***

### OutEnd

```ts
OutEnd: 3;
```

Defined in: [src/protocol/common.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L53)

***

### OutBoth

```ts
OutBoth: 4;
```

Defined in: [src/protocol/common.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L54)

# Enumeration: SyncPinPolarity

Defined in: [src/protocol/common.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L57)

## Enumeration Members

### IdleLow

```ts
IdleLow: 0;
```

Defined in: [src/protocol/common.ts:58](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L58)

***

### IdleHigh

```ts
IdleHigh: 1;
```

Defined in: [src/protocol/common.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L59)

# Enumeration: TareAxis

Defined in: [src/sensors/bno086/sh2.ts:116](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L116)

## Enumeration Members

### X

```ts
X: 1;
```

Defined in: [src/sensors/bno086/sh2.ts:117](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L117)

***

### Y

```ts
Y: 2;
```

Defined in: [src/sensors/bno086/sh2.ts:118](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L118)

***

### Z

```ts
Z: 4;
```

Defined in: [src/sensors/bno086/sh2.ts:119](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L119)

***

### All

```ts
All: 7;
```

Defined in: [src/sensors/bno086/sh2.ts:120](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L120)

# Enumeration: TareBasis

Defined in: [src/sensors/bno086/sh2.ts:107](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L107)

Rotation vector used as the tare reference (Tare Now P2).

## Enumeration Members

### RotationVector

```ts
RotationVector: 0;
```

Defined in: [src/sensors/bno086/sh2.ts:108](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L108)

***

### GameRotationVector

```ts
GameRotationVector: 1;
```

Defined in: [src/sensors/bno086/sh2.ts:109](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L109)

***

### GeomagneticRotationVector

```ts
GeomagneticRotationVector: 2;
```

Defined in: [src/sensors/bno086/sh2.ts:110](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L110)

***

### GyroIntegratedRv

```ts
GyroIntegratedRv: 3;
```

Defined in: [src/sensors/bno086/sh2.ts:111](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L111)

***

### ArvrStabilizedRv

```ts
ArvrStabilizedRv: 4;
```

Defined in: [src/sensors/bno086/sh2.ts:112](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L112)

***

### ArvrStabilizedGameRv

```ts
ArvrStabilizedGameRv: 5;
```

Defined in: [src/sensors/bno086/sh2.ts:113](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L113)

# Enumeration: Vl53l4Cmd

Defined in: [src/protocol/vl53l4.ts:6](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L6)

VL53L4 register-bridge wire codecs (contracts/10_SENSOR_VL53L4.md).
Mirrors the Python reference `depz_sensor_sdk.protocol.vl53l4`.

## Enumeration Members

### ReadReg

```ts
ReadReg: 50;
```

Defined in: [src/protocol/vl53l4.ts:7](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L7)

***

### WriteReg

```ts
WriteReg: 51;
```

Defined in: [src/protocol/vl53l4.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L8)

***

### Xshut

```ts
Xshut: 52;
```

Defined in: [src/protocol/vl53l4.ts:9](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L9)

***

### StartStream

```ts
StartStream: 53;
```

Defined in: [src/protocol/vl53l4.ts:10](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L10)

***

### StopStream

```ts
StopStream: 54;
```

Defined in: [src/protocol/vl53l4.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L11)

***

### GetInfo

```ts
GetInfo: 55;
```

Defined in: [src/protocol/vl53l4.ts:12](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L12)

***

### SetI2cSpeed

```ts
SetI2cSpeed: 56;
```

Defined in: [src/protocol/vl53l4.ts:13](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L13)

# Enumeration: Vl53l4Rpt

Defined in: [src/protocol/vl53l4.ts:16](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L16)

## Enumeration Members

### RegData

```ts
RegData: 145;
```

Defined in: [src/protocol/vl53l4.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L17)

***

### Info

```ts
Info: 146;
```

Defined in: [src/protocol/vl53l4.ts:18](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L18)

***

### Stream

```ts
Stream: 147;
```

Defined in: [src/protocol/vl53l4.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L19)

# Enumeration: Vl53l8Cmd

Defined in: [src/protocol/vl53l8.ts:6](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L6)

VL53L8 register-bridge wire codecs (contracts/04_SENSOR_VL53L8.md).
Mirrors the Python reference `depz_sensor_sdk.protocol.vl53l8`.

## Enumeration Members

### ReadReg

```ts
ReadReg: 50;
```

Defined in: [src/protocol/vl53l8.ts:7](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L7)

***

### WriteReg

```ts
WriteReg: 51;
```

Defined in: [src/protocol/vl53l8.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L8)

***

### StartStream

```ts
StartStream: 53;
```

Defined in: [src/protocol/vl53l8.ts:10](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L10)

***

### StopStream

```ts
StopStream: 54;
```

Defined in: [src/protocol/vl53l8.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L11)

# Enumeration: Vl53l8Rpt

Defined in: [src/protocol/vl53l8.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L14)

## Enumeration Members

### RegData

```ts
RegData: 145;
```

Defined in: [src/protocol/vl53l8.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L15)

***

### Vl53Frame

```ts
Vl53Frame: 147;
```

Defined in: [src/protocol/vl53l8.ts:16](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L16)

# Function: backendHealth()

```ts
function backendHealth(baseUrl?, fetchImpl?): Promise<BackendHealth | null>;
```

Defined in: [src/transport/ws-backend.ts:310](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L310)

Probe `GET /api/health`. Resolves to the health object when the origin is a
standalone backend, or `null` on any failure / when it is the plain online
viewer (so a bare `await backendHealth()` is a safe standalone check).

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `baseUrl` | `string` | `""` |
| `fetchImpl` | `FetchLike` | `defaultFetch` |

## Returns

`Promise`\<[`BackendHealth`](../interfaces/BackendHealth.md) \| `null`\>

# Function: backendPermissions()

```ts
function backendPermissions(baseUrl?, fetchImpl?): Promise<BackendPermissions | null>;
```

Defined in: [src/transport/ws-backend.ts:341](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L341)

`GET /api/permissions` → OS serial-access diagnostics, or `null` on failure.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `baseUrl` | `string` | `""` |
| `fetchImpl` | `FetchLike` | `defaultFetch` |

## Returns

`Promise`\<[`BackendPermissions`](../interfaces/BackendPermissions.md) \| `null`\>

# Function: buildCommandRequest()

```ts
function buildCommandRequest(
   seq, 
   command, 
   params?): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:240](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L240)

Command Request (0xF2), 12 bytes: id, seq, command, P0..P8.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `seq` | `number` |
| `command` | `number` |
| `params` | `Uint8Array` |

## Returns

`Uint8Array`

# Function: buildFrame()

```ts
function buildFrame(
   channel, 
   payload, 
   seq): Uint8Array;
```

Defined in: [src/sensors/bno086/shtp.ts:82](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L82)

Single-fragment frame: length = header + payload.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `channel` | `number` |
| `payload` | `Uint8Array` |
| `seq` | `number` |

## Returns

`Uint8Array`

# Function: buildFrsReadRequest()

```ts
function buildFrsReadRequest(
   frsType, 
   offsetWords?, 
   blockWords?): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:422](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L422)

FRS Read Request (0xF4), 8 bytes. blockWords = 0 reads the record.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `frsType` | `number` | `undefined` |
| `offsetWords` | `number` | `0` |
| `blockWords` | `number` | `0` |

## Returns

`Uint8Array`

# Function: buildFrsWriteData()

```ts
function buildFrsWriteData(offsetWords, words): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:474](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L474)

FRS Write Data (0xF6), 12 bytes; 1 or 2 words per packet.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `offsetWords` | `number` |
| `words` | `number`[] |

## Returns

`Uint8Array`

# Function: buildFrsWriteRequest()

```ts
function buildFrsWriteRequest(frsType, lengthWords): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:463](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L463)

FRS Write Request (0xF7), 6 bytes. lengthWords = 0 erases the record.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `frsType` | `number` |
| `lengthWords` | `number` |

## Returns

`Uint8Array`

# Function: buildGetFeatureRequest()

```ts
function buildGetFeatureRequest(sensorId): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:165](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L165)

Get Feature Request (0xFE), 2 bytes.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `sensorId` | `number` |

## Returns

`Uint8Array`

# Function: buildPacket()

```ts
function buildPacket(
   cmd, 
   payload?, 
   seq?, 
   crcType?): Uint8Array;
```

Defined in: [src/protocol/framing.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L45)

Frame one packet. `crcType` bits are set in the header even for an empty
payload (matching device TX), but CRC bytes are only appended for
non-empty payloads.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `cmd` | `number` | `undefined` |
| `payload` | `Uint8Array` | `...` |
| `seq` | `number` | `0` |
| `crcType` | [`CrcType`](../enumerations/CrcType.md) | `CrcType.None` |

## Returns

`Uint8Array`

# Function: buildProductIdRequest()

```ts
function buildProductIdRequest(): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L198)

## Returns

`Uint8Array`

# Function: buildSetFeature()

```ts
function buildSetFeature(
   sensorId, 
   intervalUs, 
   batchUs?, 
   sensitivity?, 
   flags?, 
   cfgWord?): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:144](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L144)

Set Feature Command (0xFD), 17 bytes.

`intervalUs` = 0 disables the sensor. `sensitivity` units are
sensor-dependent (change sensitivity, u16); `flags` bit meanings per SH-2
§6.5.4; `cfgWord` is the sensor-specific configuration u32.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `sensorId` | `number` | `undefined` |
| `intervalUs` | `number` | `undefined` |
| `batchUs` | `number` | `0` |
| `sensitivity` | `number` | `0` |
| `flags` | `number` | `0` |
| `cfgWord` | `number` | `0` |

## Returns

`Uint8Array`

# Function: cnhMaxBins()

```ts
function cnhMaxBins(nbAggregates, optionFlags?): number;
```

Defined in: [src/sensors/vl53l8/cnh.ts:253](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L253)

Largest CNH bins-per-aggregate count whose on-device buffer still fits
CNH_MAX_DATA_BYTES for the given aggregate count.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `nbAggregates` | `number` | `undefined` |
| `optionFlags` | `number` | `DEFAULT_CNH_CFG` |

## Returns

`number`

# Function: configBlock()

```ts
function configBlock(): Uint8Array;
```

Defined in: [src/sensors/vl53l4/uld.ts:211](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L211)

The 91-byte block sensorInit() writes at CONFIG_ADDR: the ST default
configuration with byte 0 forced to CONFIG_FMP_BYTE (Fast Mode Plus).

## Returns

`Uint8Array`

# Function: countsClearParams()

```ts
function countsClearParams(sensorId): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L72)

Counter command 0x02: clear event counts for `sensorId`.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `sensorId` | `number` |

## Returns

`Uint8Array`

# Function: countsGetParams()

```ts
function countsGetParams(sensorId): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L67)

Counter command 0x02: get event counts for `sensorId`.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `sensorId` | `number` |

## Returns

`Uint8Array`

# Function: crc16CcittFalse()

```ts
function crc16CcittFalse(data): number;
```

Defined in: [src/protocol/crc.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/crc.ts#L48)

CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, not reflected.
Used only for the `.fwdepz` file header (contract 06), never on the wire.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

## Returns

`number`

# Function: crc16Modbus()

```ts
function crc16Modbus(data): number;
```

Defined in: [src/protocol/crc.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/crc.ts#L31)

CRC-16/MODBUS: poly 0x8005 reflected, init 0xFFFF, xorout 0x0000.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

## Returns

`number`

# Function: crc32IsoHdlc()

```ts
function crc32IsoHdlc(data): number;
```

Defined in: [src/protocol/crc.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/crc.ts#L38)

CRC-32/ISO-HDLC: poly 0x04C11DB7 reflected, init/xorout 0xFFFFFFFF.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

## Returns

`number`

# Function: crc8Maxim()

```ts
function crc8Maxim(data): number;
```

Defined in: [src/protocol/crc.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/crc.ts#L24)

CRC-8/MAXIM: poly 0x31 reflected, init 0x00, xorout 0x00.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

## Returns

`number`

# Function: decodeCnh()

```ts
function decodeCnh(cfg, raw): CnhDecoded;
```

Defined in: [src/sensors/vl53l8/cnh.ts:284](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L284)

Decode a captured CNH data block (`raw` bytes, byte-swapped exactly like
the standard ranging blocks) into per-aggregate histograms.

Faithful port of vl53lmz_cnh_get_block_addresses /
_cnh_get_mem_block_addresses for the fixed cnh_cfg (ping-pong + variance
disabled).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `cfg` | [`CnhConfig`](../classes/CnhConfig.md) |
| `raw` | `Uint8Array` |

## Returns

[`CnhDecoded`](../interfaces/CnhDecoded.md)

# Function: decodeOffset()

```ts
function decodeOffset(rawWord): number;
```

Defined in: [src/sensors/vl53l4/uld.ts:323](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L323)

getOffset: RANGE_OFFSET_MM word → signed millimetres.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `rawWord` | `number` |

## Returns

`number`

# Function: decodeRangeTiming()

```ts
function decodeRangeTiming(
   intermeasurementRaw, 
   clockPll, 
   oscFrequency, 
   rangeConfigA): RangeTiming;
```

Defined in: [src/sensors/vl53l4/uld.ts:290](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L290)

GetRangeTiming register math → timing budget and inter-measurement period.

Inputs are the raw register reads: INTERMEASUREMENT_MS dword, the
RESULT__OSC_CALIBRATE_VAL word, the 0x0006 word and the RANGE_CONFIG_A
word.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `intermeasurementRaw` | `number` |
| `clockPll` | `number` |
| `oscFrequency` | `number` |
| `rangeConfigA` | `number` |

## Returns

[`RangeTiming`](../interfaces/RangeTiming.md)

# Function: decodeSigmaThreshold()

```ts
function decodeSigmaThreshold(rawWord): number;
```

Defined in: [src/sensors/vl53l4/uld.ts:353](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L353)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `rawWord` | `number` |

## Returns

`number`

# Function: decodeSignalThreshold()

```ts
function decodeSignalThreshold(rawWord): number;
```

Defined in: [src/sensors/vl53l4/uld.ts:342](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L342)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `rawWord` | `number` |

## Returns

`number`

# Function: decodeXtalk()

```ts
function decodeXtalk(rawWord): number;
```

Defined in: [src/sensors/vl53l4/uld.ts:334](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L334)

getXtalk: XTALK_PLANE_OFFSET_KCPS word → kcps.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `rawWord` | `number` |

## Returns

`number`

# Function: defaultMotionConfig()

```ts
function defaultMotionConfig(resolution): MotionConfig;
```

Defined in: [src/sensors/vl53l8/uld.ts:357](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L357)

The default motion-indicator configuration used by `motionIndicatorInit`
for a resolution (pure — same bytes the sensor is programmed with).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `resolution` | `number` |

## Returns

[`MotionConfig`](../classes/MotionConfig.md)

# Function: distanceMmFromEcho()

```ts
function distanceMmFromEcho(echoTimeUs, airTempC?): number | null;
```

Defined in: [src/protocol/sr04.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L67)

Round-trip echo time → distance in mm; null for the timeout sentinel.
Default speed of sound 343 m/s; with airTempC uses c = 331.3 + 0.606·T.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `echoTimeUs` | `number` |
| `airTempC?` | `number` |

## Returns

`number` \| `null`

# Function: errorRecordFromResponse()

```ts
function errorRecordFromResponse(resp): ErrorRecord;
```

Defined in: [src/sensors/bno086/sh2.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L92)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `resp` | [`CommandResponse`](../interfaces/CommandResponse.md) |

## Returns

[`ErrorRecord`](../interfaces/ErrorRecord.md)

# Function: errorsParams()

```ts
function errorsParams(severity?): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L77)

Errors command 0x01: return errors of `severity` or greater (0 = all).

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `severity` | `number` | `0` |

## Returns

`Uint8Array`

# Function: fragmentCargo()

```ts
function fragmentCargo(
   channel, 
   payload, 
   seqStart, 
   maxFrame?): Uint8Array<ArrayBufferLike>[];
```

Defined in: [src/sensors/bno086/shtp.ts:101](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L101)

Split a cargo into wire frames of at most `maxFrame` bytes.

First fragment advertises the TOTAL cargo length; continuations carry the
remaining length with the continuation bit set. seq increments per frame.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `channel` | `number` | `undefined` |
| `payload` | `Uint8Array` | `undefined` |
| `seqStart` | `number` | `undefined` |
| `maxFrame` | `number` | `MAX_TX_FRAME` |

## Returns

`Uint8Array`\<`ArrayBufferLike`\>[]

# Function: fwDepzPayloadCrcOk()

```ts
function fwDepzPayloadCrcOk(img): boolean;
```

Defined in: [src/protocol/fwdepz.ts:58](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/fwdepz.ts#L58)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `img` | [`FwDepzImage`](../interfaces/FwDepzImage.md) |

## Returns

`boolean`

# Function: hostNowUs()

```ts
function hostNowUs(): bigint;
```

Defined in: [src/device/device.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L71)

Monotonic host clock in µs — the host side of all time-sync math.

## Returns

`bigint`

# Function: isDeviceInfo()

```ts
function isDeviceInfo(t): t is DeviceInfo;
```

Defined in: [src/discovery.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L105)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `t` | `unknown` |

## Returns

`t is DeviceInfo`

# Function: isKnownDepzUsb()

```ts
function isKnownDepzUsb(vid, pid): boolean;
```

Defined in: [src/protocol/usb-ids.ts:75](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/usb-ids.ts#L75)

True when `(vid, pid)` looks like a DEPZ sensor: the production VID with a
mapped PID or a PID in the recognized block, or the dev default. A match
means "worth probing", never "definitely this model".

## Parameters

| Parameter | Type |
| ------ | ------ |
| `vid` | `number` \| `null` \| `undefined` |
| `pid` | `number` \| `null` \| `undefined` |

## Returns

`boolean`

# Function: listBackendDevices()

```ts
function listBackendDevices(baseUrl?, fetchImpl?): Promise<BackendDevice[]>;
```

Defined in: [src/transport/ws-backend.ts:328](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L328)

`GET /api/devices` → the device list (throws on a non-2xx / network error).

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `baseUrl` | `string` | `""` |
| `fetchImpl` | `FetchLike` | `defaultFetch` |

## Returns

`Promise`\<[`BackendDevice`](../interfaces/BackendDevice.md)[]\>

# Function: listDepzDevicesFrom()

```ts
function listDepzDevicesFrom(
   ports, 
   factory, 
opts?): Promise<DeviceInfo[]>;
```

Defined in: [src/discovery.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L189)

Probe candidate ports and return every DEPZ device found, in selection
order (sorted by USB iSerial). With `matchUsb` (default) only ports with a
known DEPZ (vid,pid) are probed.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `ports` | [`DepzPortInfo`](../interfaces/DepzPortInfo.md)[] |
| `factory` | [`TransportFactory`](../type-aliases/TransportFactory.md) |
| `opts` | [`ListOptions`](../interfaces/ListOptions.md) |

## Returns

`Promise`\<[`DeviceInfo`](../interfaces/DeviceInfo.md)[]\>

# Function: loadAssets()

```ts
function loadAssets(variant): Promise<Vl53l8Assets>;
```

Defined in: [src/sensors/vl53l8/assets/index.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L20)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `variant` | [`Vl53l8Variant`](../type-aliases/Vl53l8Variant.md) |

## Returns

`Promise`\<[`Vl53l8Assets`](../interfaces/Vl53l8Assets.md)\>

# Function: meCalibrationParams()

```ts
function meCalibrationParams(
   accel, 
   gyro, 
   mag, 
   planar?, 
   subcommand?): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:324](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L324)

ME Calibration (command 0x07). subcommand 0 = configure, 1 = get.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `accel` | `boolean` | `undefined` |
| `gyro` | `boolean` | `undefined` |
| `mag` | `boolean` | `undefined` |
| `planar` | `boolean` | `false` |
| `subcommand` | `number` | `0` |

## Returns

`Uint8Array`

# Function: motionConfigSetResolution()

```ts
function motionConfigSetResolution(cfg, resolution): void;
```

Defined in: [src/sensors/vl53l8/uld.ts:342](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L342)

Set MotionConfig.map_id for the given resolution (pure — no I/O).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `cfg` | [`MotionConfig`](../classes/MotionConfig.md) |
| `resolution` | `number` |

## Returns

`void`

# Function: offsetRaw()

```ts
function offsetRaw(offsetMm): number;
```

Defined in: [src/sensors/vl53l4/uld.ts:318](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L318)

RANGE_OFFSET_MM word for setOffset (INNER/OUTER are zeroed alongside).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `offsetMm` | `number` |

## Returns

`number`

# Function: openDeviceByDeviceSerial()

```ts
function openDeviceByDeviceSerial(
   ports, 
   factory, 
   target?, 
opts?): Promise<DepzDevice>;
```

Defined in: [src/discovery.ts:338](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L338)

Select and open a device by its DEVICE-reported serial (GET_SERIAL), for
transports that cannot enumerate USB iSerials (WebSerial). Probes every
given port (open → GET_SERIAL → close), orders the answers by device
serial, then selects — `opts.serial` picks an exact match, a numeric
`target` picks the Nth, otherwise the smallest serial — and reopens it.

The probing cost (opening each granted port once) is the price of not
having USB iSerials up front; the *result* matches Node's serial-based
selection because for programmed units USB iSerial == device serial.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `ports` | [`DepzPortInfo`](../interfaces/DepzPortInfo.md)[] |
| `factory` | [`TransportFactory`](../type-aliases/TransportFactory.md) |
| `target?` | `number` \| `null` |
| `opts?` | [`OpenDeviceOptions`](../interfaces/OpenDeviceOptions.md) |

## Returns

`Promise`\<[`DepzDevice`](../classes/DepzDevice.md)\>

# Function: openDeviceFrom()

```ts
function openDeviceFrom(
   ports, 
   factory, 
   target?, 
opts?): Promise<DepzDevice>;
```

Defined in: [src/discovery.ts:260](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L260)

Open the right sensor class for `target` (contract 02 §4).

- `undefined`/`null`: the candidate with the alphabetically-smallest USB
  iSerial (or the one whose serial == `opts.serial`). No candidate → throws
  `NoDepzDeviceError` (never falls back to "first port in system").
- `number N`: the Nth candidate, sorted by USB iSerial (0-based). Out of
  range → `NoDepzDeviceError`.
- `string`: open exactly that port path; warns if its (vid,pid) is not a
  known DEPZ id (may be an unprogrammed unit).
- `DeviceInfo`: opens its `.path`.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `ports` | [`DepzPortInfo`](../interfaces/DepzPortInfo.md)[] |
| `factory` | [`TransportFactory`](../type-aliases/TransportFactory.md) |
| `target?` | [`OpenTarget`](../type-aliases/OpenTarget.md) |
| `opts?` | [`OpenDeviceOptions`](../interfaces/OpenDeviceOptions.md) |

## Returns

`Promise`\<[`DepzDevice`](../classes/DepzDevice.md)\>

# Function: orderDevicesBySerial()

```ts
function orderDevicesBySerial(infos): DeviceInfo[];
```

Defined in: [src/discovery.ts:115](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L115)

Order *probed* devices by their DEVICE-reported serial (GET_SERIAL, i.e.
`DeviceInfo.serialNumber`); empty serials sort last, ties broken by path.
This is the selection key for transports that cannot read the USB iSerial
(WebSerial), where the serial is only knowable by probing.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `infos` | [`DeviceInfo`](../interfaces/DeviceInfo.md)[] |

## Returns

[`DeviceInfo`](../interfaces/DeviceInfo.md)[]

# Function: orderPortsBySerial()

```ts
function orderPortsBySerial(ports): DepzPortInfo[];
```

Defined in: [src/discovery.ts:85](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L85)

Sort ports by USB iSerial ascending; empty/missing serials sort last, ties
broken by path. Pure and deterministic — a given bench always maps the same
serial to index 0. Exposed so the ordering rule can be golden-tested.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `ports` | [`DepzPortInfo`](../interfaces/DepzPortInfo.md)[] |

## Returns

[`DepzPortInfo`](../interfaces/DepzPortInfo.md)[]

# Function: packDetectionThresholds()

```ts
function packDetectionThresholds(thresholds): {
  start: Uint8Array;
  valid: Uint8Array;
};
```

Defined in: [src/sensors/vl53l8/uld.ts:386](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L386)

Pack 64 detection thresholds into the DCI_DET_THRESH_START payload plus the
8-byte valid-status block (pure — mirror of set_detection_thresholds). Each
threshold's low/high are scaled by its measurement selector.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `thresholds` | `Partial`\<[`DetectionThreshold`](../interfaces/DetectionThreshold.md)\>[] |

## Returns

```ts
{
  start: Uint8Array;
  valid: Uint8Array;
}
```

### start

```ts
start: Uint8Array;
```

### valid

```ts
valid: Uint8Array;
```

# Function: packEchoDecay()

```ts
function packEchoDecay(decayUs): Uint8Array;
```

Defined in: [src/protocol/sr04.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L53)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `decayUs` | `number` |

## Returns

`Uint8Array`

# Function: packReadReg()

```ts
function packReadReg(addr, length): Uint8Array;
```

Defined in: [src/protocol/vl53l8.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L30)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `length` | `number` |

## Returns

`Uint8Array`

# Function: packSamplePeriod()

```ts
function packSamplePeriod(periodUs): Uint8Array;
```

Defined in: [src/protocol/sr04.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L43)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `periodUs` | `number` |

## Returns

`Uint8Array`

# Function: packShtpHeader()

```ts
function packShtpHeader(hdr): Uint8Array;
```

Defined in: [src/sensors/bno086/shtp.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L52)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `hdr` | [`ShtpHeader`](../interfaces/ShtpHeader.md) |

## Returns

`Uint8Array`

# Function: packStartStream()

```ts
function packStartStream(frameSize): Uint8Array;
```

Defined in: [src/protocol/vl53l8.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L45)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `frameSize` | `number` |

## Returns

`Uint8Array`

# Function: packSyncPinConfig()

```ts
function packSyncPinConfig(cfg): Uint8Array;
```

Defined in: [src/protocol/common.ts:133](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L133)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `cfg` | [`SyncPinConfig`](../interfaces/SyncPinConfig.md) |

## Returns

`Uint8Array`

# Function: packSyncTime()

```ts
function packSyncTime(pcTimestampUs): Uint8Array;
```

Defined in: [src/protocol/common.ts:127](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L127)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `pcTimestampUs` | `bigint` |

## Returns

`Uint8Array`

# Function: packVl53l4ReadReg()

```ts
function packVl53l4ReadReg(addr, length): Uint8Array;
```

Defined in: [src/protocol/vl53l4.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L44)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `length` | `number` |

## Returns

`Uint8Array`

# Function: packVl53l4SetI2cSpeed()

```ts
function packVl53l4SetI2cSpeed(khz): Uint8Array;
```

Defined in: [src/protocol/vl53l4.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L72)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `khz` | `number` |

## Returns

`Uint8Array`

# Function: packVl53l4StartStream()

```ts
function packVl53l4StartStream(
   addr, 
   length, 
   flags?): Uint8Array;
```

Defined in: [src/protocol/vl53l4.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L63)

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `addr` | `number` | `undefined` |
| `length` | `number` | `undefined` |
| `flags` | `number` | `0` |

## Returns

`Uint8Array`

# Function: packVl53l4WriteReg()

```ts
function packVl53l4WriteReg(addr, data): Uint8Array;
```

Defined in: [src/protocol/vl53l4.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L52)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `data` | `Uint8Array` |

## Returns

`Uint8Array`

# Function: packVl53l4Xshut()

```ts
function packVl53l4Xshut(action): Uint8Array;
```

Defined in: [src/protocol/vl53l4.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L59)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `action` | `number` |

## Returns

`Uint8Array`

# Function: packWriteReg()

```ts
function packWriteReg(addr, data): Uint8Array;
```

Defined in: [src/protocol/vl53l8.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L38)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `data` | `Uint8Array` |

## Returns

`Uint8Array`

# Function: parseFwDepz()

```ts
function parseFwDepz(blob): FwDepzImage;
```

Defined in: [src/protocol/fwdepz.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/fwdepz.ts#L24)

Validation order (contract 06 §2): magic → header CRC → size.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `blob` | `Uint8Array` |

## Returns

[`FwDepzImage`](../interfaces/FwDepzImage.md)

# Function: parseGyroRvCargo()

```ts
function parseGyroRvCargo(payload, captureTimestampUs): GyroIntegratedRV | null;
```

Defined in: [src/sensors/bno086/reports.ts:619](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L619)

Parse a channel-5 cargo (gyro-integrated RV, dense format).

Two shapes seen on hardware (vendor tool): 7×i16 bare, or prefixed with
0xFB + i32 base delta + u16 delay (both 100 µs ticks).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |
| `captureTimestampUs` | `bigint` |

## Returns

[`GyroIntegratedRV`](../interfaces/GyroIntegratedRV.md) \| `null`

# Function: parseInputCargo()

```ts
function parseInputCargo(payload, captureTimestampUs): Report[];
```

Defined in: [src/sensors/bno086/reports.ts:413](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L413)

Parse a channel-3/4 cargo into typed reports.

`captureTimestampUs` is the bridge RPT_DATA capture time (MCU uptime).
Handles 0xFB base timestamp references and 0xFA rebases; every report's
timestamp is `base + delay` where base = capture − baseDelta·100 µs.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |
| `captureTimestampUs` | `bigint` |

## Returns

[`Report`](../type-aliases/Report.md)[]

# Function: parseResultBlock()

```ts
function parseResultBlock(raw): Vl53l4Results;
```

Defined in: [src/sensors/vl53l4/uld.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L180)

Decode the streamed 0x0089..0x0099 block exactly as VL53L4CD_GetResult()
decodes the same registers read one by one. Register contents are
big-endian words (the bridge passes them through untouched).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `raw` | `Uint8Array` |

## Returns

[`Vl53l4Results`](../interfaces/Vl53l4Results.md)

# Function: parseSoftwareName()

```ts
function parseSoftwareName(name): Identity;
```

Defined in: [src/protocol/identity.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/identity.ts#L25)

Classify a GET_NAME_ACTIVE_SOFTWARE string. The string must already be
stripped of trailing NUL/0xFF (`stripDeviceString`).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `name` | `string` |

## Returns

[`Identity`](../interfaces/Identity.md)

# Function: payloadCrcBytes()

```ts
function payloadCrcBytes(crcType, payload): Uint8Array;
```

Defined in: [src/protocol/framing.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L30)

CRC trailer for a payload; empty payloads never carry CRC bytes.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `crcType` | [`CrcType`](../enumerations/CrcType.md) |
| `payload` | `Uint8Array` |

## Returns

`Uint8Array`

# Function: periodicDcdParams()

```ts
function periodicDcdParams(enable): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:341](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L341)

Periodic DCD save config (command 0x09). P0: 0 = enable, 1 = disable.
No command response is generated.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `enable` | `boolean` |

## Returns

`Uint8Array`

# Function: persistTareParams()

```ts
function persistTareParams(): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:298](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L298)

Tare subcommand 1 — persist current tare into FRS.

## Returns

`Uint8Array`

# Function: pidToModel()

```ts
function pidToModel(pid): DepzUsbModel | null;
```

Defined in: [src/protocol/usb-ids.ts:84](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/usb-ids.ts#L84)

Model hint for a PID, or null when unrecognized (informational only).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `pid` | `number` \| `null` \| `undefined` |

## Returns

[`DepzUsbModel`](../interfaces/DepzUsbModel.md) \| `null`

# Function: probePort()

```ts
function probePort(
   port, 
   factory, 
timeoutMs?): Promise<DeviceInfo | null>;
```

Defined in: [src/discovery.ts:132](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L132)

Open `port`, ask its identity (+ device name/serial), then close. Returns
null when nothing DEPZ-shaped answers. Probing *opens* the port, so callers
should restrict the port list to plausible candidates.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `port` | [`DepzPortInfo`](../interfaces/DepzPortInfo.md) | `undefined` |
| `factory` | [`TransportFactory`](../type-aliases/TransportFactory.md) | `undefined` |
| `timeoutMs` | `number` | `DEFAULT_PROBE_TIMEOUT_MS` |

## Returns

`Promise`\<[`DeviceInfo`](../interfaces/DeviceInfo.md) \| `null`\>

# Function: rangeStatusText()

```ts
function rangeStatusText(rangeStatus): string;
```

Defined in: [src/sensors/vl53l4/uld.ts:162](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L162)

Human-readable text for a decoded range status.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `rangeStatus` | `number` |

## Returns

`string`

# Function: rangeTimingRegisters()

```ts
function rangeTimingRegisters(
   timingBudgetMs, 
   interMeasurementMs, 
   oscFrequency, 
   clockPll?): RangeTimingRegisters;
```

Defined in: [src/sensors/vl53l4/uld.ts:233](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L233)

SetRangeTiming register math → RANGE_CONFIG_A, RANGE_CONFIG_B and the
INTERMEASUREMENT_MS raw dword.

`oscFrequency` is the word read from 0x0006; `clockPll` is the word read
from RESULT__OSC_CALIBRATE_VAL (used only in autonomous mode, i.e. when
`interMeasurementMs > 0`).

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `timingBudgetMs` | `number` | `undefined` |
| `interMeasurementMs` | `number` | `undefined` |
| `oscFrequency` | `number` | `undefined` |
| `clockPll` | `number` | `0` |

## Returns

[`RangeTimingRegisters`](../interfaces/RangeTimingRegisters.md)

# Function: sensorMetadataFromWords()

```ts
function sensorMetadataFromWords(words): SensorMetadata;
```

Defined in: [src/sensors/bno086/sh2.ts:637](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L637)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `words` | `number`[] |

## Returns

[`SensorMetadata`](../interfaces/SensorMetadata.md)

# Function: setReorientationParams()

```ts
function setReorientationParams(
   x, 
   y, 
   z, 
   w): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:309](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L309)

Tare subcommand 2 — set reorientation quaternion.

P1..P8 are four int16 Q14 components (the 8 available parameter bytes only
fit Q14 halves; the *FRS* System Orientation record is the one that stores
Q30 words). All-zero clears the reorientation.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `x` | `number` |
| `y` | `number` |
| `z` | `number` |
| `w` | `number` |

## Returns

`Uint8Array`

# Function: sigmaThresholdRaw()

```ts
function sigmaThresholdRaw(sigmaMm): number;
```

Defined in: [src/sensors/vl53l4/uld.ts:346](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L346)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `sigmaMm` | `number` |

## Returns

`number`

# Function: signalThresholdRaw()

```ts
function signalThresholdRaw(signalKcps): number;
```

Defined in: [src/sensors/vl53l4/uld.ts:338](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L338)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `signalKcps` | `number` |

## Returns

`number`

# Function: stripDeviceString()

```ts
function stripDeviceString(raw): string;
```

Defined in: [src/protocol/common.ts:159](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L159)

Decode an ASCII device string, dropping trailing NUL/0xFF filler.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `raw` | `Uint8Array` |

## Returns

`string`

# Function: swapBuffer()

```ts
function swapBuffer(data): Uint8Array;
```

Defined in: [src/sensors/vl53l8/uld.ts:193](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L193)

VL53L8CX_SwapBuffer: byte-reverse every 32-bit word (tail untouched).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

## Returns

`Uint8Array`

# Function: syncTimeAll()

```ts
function syncTimeAll(devices, samples?): Promise<Map<DepzDevice, TimeSync>>;
```

Defined in: [src/device/device.ts:529](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L529)

Software-sync several devices to the common host monotonic clock.

Runs `syncTime()` on each device so that every device's `toHostTimeUs()`
maps its own timestamps onto ONE shared host timeline — the basis for
correlating reports from multiple sensors live. Returns a Map
{device → TimeSync}. Mirrors the Python reference `sync_time_all`.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `devices` | `Iterable`\<[`DepzDevice`](../classes/DepzDevice.md)\> | `undefined` |
| `samples` | `number` | `5` |

## Returns

`Promise`\<`Map`\<[`DepzDevice`](../classes/DepzDevice.md), [`TimeSync`](../interfaces/TimeSync.md)\>\>

# Function: syncTimeOffsetRtt()

```ts
function syncTimeOffsetRtt(
   t1, 
   t2, 
   t3, 
   t4): {
  offsetUs: bigint;
  rttUs: bigint;
};
```

Defined in: [src/protocol/common.ts:146](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L146)

NTP-style clock math, all µs (contract 02 §5). offset = device − host,
`((T2-T1)+(T3-T4))/2` truncated toward zero (bigint division is exactly
that); rtt = (T4-T1)-(T3-T2).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `t1` | `bigint` |
| `t2` | `bigint` |
| `t3` | `bigint` |
| `t4` | `bigint` |

## Returns

```ts
{
  offsetUs: bigint;
  rttUs: bigint;
}
```

### offsetUs

```ts
offsetUs: bigint;
```

### rttUs

```ts
rttUs: bigint;
```

# Function: tareNowParams()

```ts
function tareNowParams(axes?, basis?): Uint8Array;
```

Defined in: [src/sensors/bno086/sh2.ts:290](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L290)

Tare subcommand 0 — tare `axes` (bitmap X=1,Y=2,Z=4) using `basis`.

## Parameters

| Parameter | Type | Default value |
| ------ | ------ | ------ |
| `axes` | `number` | `TareAxis.All` |
| `basis` | `number` | `TareBasis.RotationVector` |

## Returns

`Uint8Array`

# Function: unpackBno086Data()

```ts
function unpackBno086Data(payload): Bno086Data;
```

Defined in: [src/protocol/bno086.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L42)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Bno086Data`](../interfaces/Bno086Data.md)

# Function: unpackCommandResponse()

```ts
function unpackCommandResponse(payload): CommandResponse;
```

Defined in: [src/sensors/bno086/sh2.ts:274](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L274)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`CommandResponse`](../interfaces/CommandResponse.md)

# Function: unpackEchoDecay()

```ts
function unpackEchoDecay(p): number;
```

Defined in: [src/protocol/sr04.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L59)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | `Uint8Array` |

## Returns

`number`

# Function: unpackFeatureResponse()

```ts
function unpackFeatureResponse(payload): FeatureResponse;
```

Defined in: [src/sensors/bno086/sh2.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L180)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`FeatureResponse`](../interfaces/FeatureResponse.md)

# Function: unpackFrameChunk()

```ts
function unpackFrameChunk(payload): FrameChunk;
```

Defined in: [src/protocol/vl53l8.ts:76](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L76)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`FrameChunk`](../interfaces/FrameChunk.md)

# Function: unpackFrsReadResponse()

```ts
function unpackFrsReadResponse(payload): FrsReadResponse;
```

Defined in: [src/sensors/bno086/sh2.ts:445](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L445)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`FrsReadResponse`](../interfaces/FrsReadResponse.md)

# Function: unpackFrsWriteResponse()

```ts
function unpackFrsWriteResponse(payload): FrsWriteResponse;
```

Defined in: [src/sensors/bno086/sh2.ts:495](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L495)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`FrsWriteResponse`](../interfaces/FrsWriteResponse.md)

# Function: unpackProductId()

```ts
function unpackProductId(payload): ProductId;
```

Defined in: [src/sensors/bno086/sh2.ts:217](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L217)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`ProductId`](../interfaces/ProductId.md)

# Function: unpackRegData()

```ts
function unpackRegData(payload): RegData;
```

Defined in: [src/protocol/vl53l8.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L59)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`RegData`](../interfaces/RegData.md)

# Function: unpackSamplePeriod()

```ts
function unpackSamplePeriod(p): number;
```

Defined in: [src/protocol/sr04.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L49)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | `Uint8Array` |

## Returns

`number`

# Function: unpackSequenceError()

```ts
function unpackSequenceError(p): SequenceErrorReport;
```

Defined in: [src/protocol/common.ts:123](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L123)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | `Uint8Array` |

## Returns

[`SequenceErrorReport`](../interfaces/SequenceErrorReport.md)

# Function: unpackShtpHeader()

```ts
function unpackShtpHeader(data): ShtpHeader;
```

Defined in: [src/sensors/bno086/shtp.ts:62](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L62)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

## Returns

[`ShtpHeader`](../interfaces/ShtpHeader.md)

# Function: unpackSr04Data()

```ts
function unpackSr04Data(p): Sr04Data;
```

Defined in: [src/protocol/sr04.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L34)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | `Uint8Array` |

## Returns

[`Sr04Data`](../interfaces/Sr04Data.md)

# Function: unpackStatus()

```ts
function unpackStatus(p): StatusReport;
```

Defined in: [src/protocol/common.ts:101](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L101)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | `Uint8Array` |

## Returns

[`StatusReport`](../interfaces/StatusReport.md)

# Function: unpackSyncPinConfig()

```ts
function unpackSyncPinConfig(p): SyncPinConfig;
```

Defined in: [src/protocol/common.ts:137](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L137)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | `Uint8Array` |

## Returns

[`SyncPinConfig`](../interfaces/SyncPinConfig.md)

# Function: unpackSyncTime()

```ts
function unpackSyncTime(p): SyncTimeReport;
```

Defined in: [src/protocol/common.ts:109](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L109)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | `Uint8Array` |

## Returns

[`SyncTimeReport`](../interfaces/SyncTimeReport.md)

# Function: unpackTemperature()

```ts
function unpackTemperature(p): TemperatureReport;
```

Defined in: [src/protocol/common.ts:118](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L118)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | `Uint8Array` |

## Returns

[`TemperatureReport`](../interfaces/TemperatureReport.md)

# Function: unpackText()

```ts
function unpackText(p): TextReport;
```

Defined in: [src/protocol/common.ts:105](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L105)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `p` | `Uint8Array` |

## Returns

[`TextReport`](../interfaces/TextReport.md)

# Function: unpackVl53l4Info()

```ts
function unpackVl53l4Info(payload): Vl53l4Info;
```

Defined in: [src/protocol/vl53l4.ts:124](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L124)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Vl53l4Info`](../interfaces/Vl53l4Info.md)

# Function: unpackVl53l4RegData()

```ts
function unpackVl53l4RegData(payload): Vl53l4RegData;
```

Defined in: [src/protocol/vl53l4.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L87)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Vl53l4RegData`](../interfaces/Vl53l4RegData.md)

# Function: unpackVl53l4Stream()

```ts
function unpackVl53l4Stream(payload): Vl53l4StreamData;
```

Defined in: [src/protocol/vl53l4.ts:152](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L152)

## Parameters

| Parameter | Type |
| ------ | ------ |
| `payload` | `Uint8Array` |

## Returns

[`Vl53l4StreamData`](../interfaces/Vl53l4StreamData.md)

# Function: usbModelHint()

```ts
function usbModelHint(vid, pid): string | null;
```

Defined in: [src/protocol/usb-ids.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/usb-ids.ts#L94)

Best-guess model name for a `(vid, pid)`, or null. Informational only —
never used to decide how to decode a device; the firmware-name probe does
that. Mirrors the Python reference `usb_model_hint`.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `vid` | `number` \| `null` \| `undefined` |
| `pid` | `number` \| `null` \| `undefined` |

## Returns

`string` \| `null`

# Function: xtalkMarginToRaw()

```ts
function xtalkMarginToRaw(marginKcps): number;
```

Defined in: [src/sensors/vl53l8/uld.ts:377](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L377)

Xtalk margin (kcps/spad) → raw DCI value (round(kcps * 2048)).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `marginKcps` | `number` |

## Returns

`number`

# Function: xtalkRaw()

```ts
function xtalkRaw(xtalkKcps): number;
```

Defined in: [src/sensors/vl53l4/uld.ts:329](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L329)

XTALK_PLANE_OFFSET_KCPS word for setXtalk.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `xtalkKcps` | `number` |

## Returns

`number`

# Function: zoneGrid()

```ts
function zoneGrid(arr, resolution): number[][];
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L73)

Zone array reshaped to (4,4) or (8,8) — mirror of Vl53l8Frame.grid().

## Parameters

| Parameter | Type |
| ------ | ------ |
| `arr` | `ArrayLike`\<`number`\> |
| `resolution` | `number` |

## Returns

`number`[][]

# Interface: Acceleration

Defined in: [src/sensors/bno086/reports.ts:178](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L178)

0x01 accelerometer / 0x04 linear acceleration / 0x06 gravity (Q8).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "Acceleration";
```

Defined in: [src/sensors/bno086/reports.ts:179](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L179)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:180](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L180)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:181](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L181)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:182](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L182)

***

### x

```ts
x: number;
```

Defined in: [src/sensors/bno086/reports.ts:184](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L184)

m/s².

***

### y

```ts
y: number;
```

Defined in: [src/sensors/bno086/reports.ts:185](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L185)

***

### z

```ts
z: number;
```

Defined in: [src/sensors/bno086/reports.ts:186](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L186)

# Interface: BackendDevice

Defined in: [src/transport/ws-backend.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L27)

One device as reported by `GET /api/devices` (snake_case = server JSON).

## Properties

### path

```ts
path: string;
```

Defined in: [src/transport/ws-backend.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L29)

OS serial-port path the backend opens (e.g. `/dev/ttyACM0`, `COM5`).

***

### usb\_vid

```ts
usb_vid: number | null;
```

Defined in: [src/transport/ws-backend.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L30)

***

### usb\_pid

```ts
usb_pid: number | null;
```

Defined in: [src/transport/ws-backend.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L31)

***

### serial

```ts
serial: string;
```

Defined in: [src/transport/ws-backend.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L33)

USB iSerial as read by the OS ("" when unavailable).

***

### sensor\_type

```ts
sensor_type: string;
```

Defined in: [src/transport/ws-backend.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L35)

Backend's guess of the sensor family ("sr04" | "vl53l8" | … | "").

***

### usb\_model\_hint

```ts
usb_model_hint: string;
```

Defined in: [src/transport/ws-backend.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L37)

Human model hint from the USB id tables ("" when unknown).

# Interface: BackendHealth

Defined in: [src/transport/ws-backend.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L41)

`GET /api/health` — its mere presence means the viewer is standalone.

## Properties

### standalone

```ts
standalone: boolean;
```

Defined in: [src/transport/ws-backend.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L42)

***

### version

```ts
version: string;
```

Defined in: [src/transport/ws-backend.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L43)

# Interface: BackendPermissions

Defined in: [src/transport/ws-backend.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L47)

`GET /api/permissions` — OS-level serial-access diagnostics.

## Properties

### ok

```ts
ok: boolean;
```

Defined in: [src/transport/ws-backend.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L48)

***

### os

```ts
os: "linux" | "macos" | "windows";
```

Defined in: [src/transport/ws-backend.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L49)

***

### issues

```ts
issues: string[];
```

Defined in: [src/transport/ws-backend.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L50)

***

### fix

```ts
fix: {
  command: string;
  explanation: string;
};
```

Defined in: [src/transport/ws-backend.ts:51](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L51)

#### command

```ts
command: string;
```

#### explanation

```ts
explanation: string;
```

# Interface: Bno086Data

Defined in: [src/protocol/bno086.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L33)

One RPT_DATA report: an SHTP frame captured from the sensor bus.

## Properties

### cmd

```ts
cmd: number;
```

Defined in: [src/protocol/bno086.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L35)

Always 0x00 in practice (ERRATA E2) — never correlate on it.

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/protocol/bno086.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L37)

MCU uptime at frame capture (µs).

***

### shtp

```ts
shtp: Uint8Array;
```

Defined in: [src/protocol/bno086.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L39)

Raw SHTP frame (4-byte header + cargo fragment).

# Interface: Bno086Options

Defined in: [src/sensors/bno086/bno086.ts:83](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L83)

## Extends

- [`DeviceOptions`](DeviceOptions.md)

## Properties

### timeoutMs?

```ts
optional timeoutMs?: number;
```

Defined in: [src/device/device.ts:169](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L169)

#### Inherited from

[`DeviceOptions`](DeviceOptions.md).[`timeoutMs`](DeviceOptions.md#timeoutms)

***

### txCrcType?

```ts
optional txCrcType?: CrcType;
```

Defined in: [src/device/device.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L170)

#### Inherited from

[`DeviceOptions`](DeviceOptions.md).[`txCrcType`](DeviceOptions.md#txcrctype)

***

### busyRetries?

```ts
optional busyRetries?: number;
```

Defined in: [src/sensors/bno086/bno086.ts:85](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L85)

SEND_SHTP_PACKET attempts before giving up (default 5).

***

### busyBackoffMs?

```ts
optional busyBackoffMs?: number;
```

Defined in: [src/sensors/bno086/bno086.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L87)

ERR_BUSY backoff; >= 200 ms per the bridge spec (tests inject less).

# Interface: CalibrationConfig

Defined in: [src/sensors/bno086/bno086.ts:76](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L76)

ME calibration enables as reported by the sensor.

## Properties

### accel

```ts
accel: boolean;
```

Defined in: [src/sensors/bno086/bno086.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L77)

***

### gyro

```ts
gyro: boolean;
```

Defined in: [src/sensors/bno086/bno086.ts:78](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L78)

***

### mag

```ts
mag: boolean;
```

Defined in: [src/sensors/bno086/bno086.ts:79](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L79)

***

### planar

```ts
planar: boolean;
```

Defined in: [src/sensors/bno086/bno086.ts:80](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L80)

# Interface: CnhAggregate

Defined in: [src/sensors/vl53l8/cnh.ts:262](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L262)

Decoded CNH aggregate.

## Properties

### hist

```ts
hist: number[];
```

Defined in: [src/sensors/vl53l8/cnh.ts:264](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L264)

value = raw / 2**scaler, length == cfg.featureLength.

***

### histRaw

```ts
histRaw: number[];
```

Defined in: [src/sensors/vl53l8/cnh.ts:265](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L265)

***

### histScaler

```ts
histScaler: number[];
```

Defined in: [src/sensors/vl53l8/cnh.ts:266](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L266)

***

### ambient

```ts
ambient: number;
```

Defined in: [src/sensors/vl53l8/cnh.ts:267](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L267)

# Interface: CnhDecoded

Defined in: [src/sensors/vl53l8/cnh.ts:270](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L270)

## Properties

### refResidual

```ts
refResidual: number;
```

Defined in: [src/sensors/vl53l8/cnh.ts:272](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L272)

11 fractional bits.

***

### aggregates

```ts
aggregates: CnhAggregate[];
```

Defined in: [src/sensors/vl53l8/cnh.ts:273](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L273)

# Interface: CommandResponse

Defined in: [src/sensors/bno086/sh2.ts:263](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L263)

Command Response (0xF1), 16 bytes.

`commandSeq` echoes the request's sequence number (correlate on it plus
`command`); `responseSeq` counts multiple responses to one request.
R0 is the status word for most commands (0 = success).

## Properties

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/sh2.ts:264](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L264)

***

### command

```ts
command: number;
```

Defined in: [src/sensors/bno086/sh2.ts:265](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L265)

***

### commandSeq

```ts
commandSeq: number;
```

Defined in: [src/sensors/bno086/sh2.ts:266](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L266)

***

### responseSeq

```ts
responseSeq: number;
```

Defined in: [src/sensors/bno086/sh2.ts:267](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L267)

***

### r

```ts
r: number[];
```

Defined in: [src/sensors/bno086/sh2.ts:269](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L269)

R0..R10.

***

### status

```ts
status: number;
```

Defined in: [src/sensors/bno086/sh2.ts:271](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L271)

R0.

# Interface: Counts

Defined in: [src/sensors/bno086/sh2.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L98)

Per-sensor event counts (command 0x02 get response, 2 messages).

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/sh2.ts:99](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L99)

***

### offered

```ts
offered: number;
```

Defined in: [src/sensors/bno086/sh2.ts:100](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L100)

***

### accepted

```ts
accepted: number;
```

Defined in: [src/sensors/bno086/sh2.ts:101](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L101)

***

### on

```ts
on: number;
```

Defined in: [src/sensors/bno086/sh2.ts:102](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L102)

***

### attempted

```ts
attempted: number;
```

Defined in: [src/sensors/bno086/sh2.ts:103](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L103)

# Interface: CrcErrorEvent

Defined in: [src/protocol/framing.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L87)

A frame with a valid header whose payload CRC failed; dropped.

## Properties

### type

```ts
type: "crcError";
```

Defined in: [src/protocol/framing.ts:88](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L88)

***

### cmd

```ts
cmd: number;
```

Defined in: [src/protocol/framing.ts:89](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L89)

***

### seq

```ts
seq: number;
```

Defined in: [src/protocol/framing.ts:90](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L90)

# Interface: DatasetDeviceMeta

Defined in: [src/dataset.ts:13](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L13)

## Indexable

```ts
[k: string]: unknown
```

## Properties

### serial?

```ts
optional serial?: string;
```

Defined in: [src/dataset.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L14)

***

### sensor\_type?

```ts
optional sensor_type?: string;
```

Defined in: [src/dataset.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L15)

***

### software\_name?

```ts
optional software_name?: string;
```

Defined in: [src/dataset.ts:16](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L16)

***

### time\_sync

```ts
time_sync: {
  offset_us: number;
  rtt_us: number;
};
```

Defined in: [src/dataset.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L17)

#### offset\_us

```ts
offset_us: number;
```

#### rtt\_us

```ts
rtt_us: number;
```

# Interface: DatasetHeader

Defined in: [src/dataset.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L21)

## Indexable

```ts
[k: string]: unknown
```

## Properties

### schema

```ts
schema: string;
```

Defined in: [src/dataset.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L22)

***

### created\_utc

```ts
created_utc: string;
```

Defined in: [src/dataset.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L23)

***

### devices

```ts
devices: Record<string, DatasetDeviceMeta>;
```

Defined in: [src/dataset.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L24)

***

### note?

```ts
optional note?: string;
```

Defined in: [src/dataset.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L25)

# Interface: DatasetRecord

Defined in: [src/dataset.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L29)

## Properties

### deviceId

```ts
deviceId: string;
```

Defined in: [src/dataset.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L30)

***

### tHostUs

```ts
tHostUs: number;
```

Defined in: [src/dataset.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L31)

***

### kind

```ts
kind: string;
```

Defined in: [src/dataset.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L32)

***

### value

```ts
value: Record<string, unknown>;
```

Defined in: [src/dataset.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L33)

# Interface: DepzPortInfo

Defined in: [src/discovery.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L26)

One enumerated candidate port (Node: serialport.list; Web: granted port).

## Properties

### path

```ts
path: string;
```

Defined in: [src/discovery.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L28)

Stable identifier: an OS path (Node) or a synthetic id (WebSerial).

***

### serialNumber?

```ts
optional serialNumber?: string;
```

Defined in: [src/discovery.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L34)

USB iSerial as reported by the OS — the sort/selection key. WebSerial
cannot read this, so it is undefined there (selection falls back to
granted-port order).

***

### usbVid?

```ts
optional usbVid?: number;
```

Defined in: [src/discovery.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L35)

***

### usbPid?

```ts
optional usbPid?: number;
```

Defined in: [src/discovery.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L36)

# Interface: DepzUsbModel

Defined in: [src/protocol/usb-ids.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/usb-ids.ts#L34)

A known DEPZ USB PID and what it is.

## Properties

### name

```ts
name: string;
```

Defined in: [src/protocol/usb-ids.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/usb-ids.ts#L36)

Human-readable model name.

***

### sensorType

```ts
sensorType: SensorType | null;
```

Defined in: [src/protocol/usb-ids.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/usb-ids.ts#L42)

Sensor class this SDK can decode, or null for a DEPZ sensor that is
recognized but not (yet) driven by this SDK (still opens as a base
device / warned about).

# Interface: DetectionThreshold

Defined in: [src/sensors/vl53l8/uld.ts:247](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L247)

One detection-threshold entry (real units; mirror of the Python dict).

## Properties

### lowThresh

```ts
lowThresh: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:248](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L248)

***

### highThresh

```ts
highThresh: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:249](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L249)

***

### measurement

```ts
measurement: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:250](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L250)

***

### type

```ts
type: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:251](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L251)

***

### zoneNum

```ts
zoneNum: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:252](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L252)

***

### operation

```ts
operation: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:253](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L253)

# Interface: DeviceInfo

Defined in: [src/discovery.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L40)

Result of probing a port (parity with Python `DeviceInfo`).

## Properties

### path

```ts
path: string;
```

Defined in: [src/discovery.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L41)

***

### mode

```ts
mode: "unknown" | "app" | "bootloader";
```

Defined in: [src/discovery.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L42)

***

### sensorType

```ts
sensorType: SensorType | null;
```

Defined in: [src/discovery.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L43)

***

### softwareName

```ts
softwareName: string;
```

Defined in: [src/discovery.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L44)

***

### fwVersion

```ts
fwVersion: string;
```

Defined in: [src/discovery.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L45)

***

### deviceName

```ts
deviceName: string;
```

Defined in: [src/discovery.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L46)

***

### serialNumber

```ts
serialNumber: string;
```

Defined in: [src/discovery.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L48)

Device-reported serial (GET_SERIAL); "" when the device does not answer.

***

### usbVid

```ts
usbVid: number | null;
```

Defined in: [src/discovery.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L49)

***

### usbPid

```ts
usbPid: number | null;
```

Defined in: [src/discovery.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L50)

# Interface: DeviceOptions

Defined in: [src/device/device.ts:168](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L168)

## Extended by

- [`Vl53l4Options`](Vl53l4Options.md)
- [`Bno086Options`](Bno086Options.md)
- [`Vl53l8Options`](Vl53l8Options.md)

## Properties

### timeoutMs?

```ts
optional timeoutMs?: number;
```

Defined in: [src/device/device.ts:169](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L169)

***

### txCrcType?

```ts
optional txCrcType?: CrcType;
```

Defined in: [src/device/device.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L170)

# Interface: EnableOptions

Defined in: [src/sensors/bno086/bno086.ts:90](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L90)

## Properties

### intervalUs?

```ts
optional intervalUs?: number;
```

Defined in: [src/sensors/bno086/bno086.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L92)

Alternative to `hz`: exact report interval in µs.

***

### batchUs?

```ts
optional batchUs?: number;
```

Defined in: [src/sensors/bno086/bno086.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L93)

***

### sensitivity?

```ts
optional sensitivity?: number;
```

Defined in: [src/sensors/bno086/bno086.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L94)

***

### flags?

```ts
optional flags?: number;
```

Defined in: [src/sensors/bno086/bno086.ts:95](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L95)

***

### cfgWord?

```ts
optional cfgWord?: number;
```

Defined in: [src/sensors/bno086/bno086.ts:96](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L96)

***

### verify?

```ts
optional verify?: boolean;
```

Defined in: [src/sensors/bno086/bno086.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L98)

Read back the granted rate via Get Feature (default true).

***

### timeoutMs?

```ts
optional timeoutMs?: number;
```

Defined in: [src/sensors/bno086/bno086.ts:99](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L99)

# Interface: ErrorRecord

Defined in: [src/sensors/bno086/sh2.ts:82](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L82)

One error queue entry (command 0x01 response, r[0..5]).

## Properties

### severity

```ts
severity: number;
```

Defined in: [src/sensors/bno086/sh2.ts:83](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L83)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/sh2.ts:84](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L84)

***

### source

```ts
source: number;
```

Defined in: [src/sensors/bno086/sh2.ts:86](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L86)

ErrorSource.

***

### error

```ts
error: number;
```

Defined in: [src/sensors/bno086/sh2.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L87)

***

### module

```ts
module: number;
```

Defined in: [src/sensors/bno086/sh2.ts:88](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L88)

***

### code

```ts
code: number;
```

Defined in: [src/sensors/bno086/sh2.ts:89](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L89)

# Interface: FeatureResponse

Defined in: [src/sensors/bno086/sh2.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L170)

Get Feature Response (0xFC), 17 bytes — the rates in effect.

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/sh2.ts:171](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L171)

***

### flags

```ts
flags: number;
```

Defined in: [src/sensors/bno086/sh2.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L172)

***

### sensitivity

```ts
sensitivity: number;
```

Defined in: [src/sensors/bno086/sh2.ts:173](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L173)

***

### intervalUs

```ts
intervalUs: number;
```

Defined in: [src/sensors/bno086/sh2.ts:175](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L175)

Actual report interval granted by the hub.

***

### batchUs

```ts
batchUs: number;
```

Defined in: [src/sensors/bno086/sh2.ts:176](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L176)

***

### cfgWord

```ts
cfgWord: number;
```

Defined in: [src/sensors/bno086/sh2.ts:177](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L177)

# Interface: FrameChunk

Defined in: [src/protocol/vl53l8.ts:69](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L69)

One RPT_VL53_FRAME chunk of a (possibly multi-chunk) sensor frame.

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/protocol/vl53l8.ts:70](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L70)

***

### fullSize

```ts
fullSize: number;
```

Defined in: [src/protocol/vl53l8.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L71)

***

### offset

```ts
offset: number;
```

Defined in: [src/protocol/vl53l8.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L72)

***

### data

```ts
data: Uint8Array;
```

Defined in: [src/protocol/vl53l8.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L73)

# Interface: FrsReadResponse

Defined in: [src/sensors/bno086/sh2.ts:434](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L434)

FRS Read Response (0xF3), 16 bytes; up to two data words per packet.

## Properties

### status

```ts
status: number;
```

Defined in: [src/sensors/bno086/sh2.ts:436](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L436)

FrsStatus.

***

### dataLength

```ts
dataLength: number;
```

Defined in: [src/sensors/bno086/sh2.ts:438](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L438)

Valid words in data0/data1 (0–2).

***

### offsetWords

```ts
offsetWords: number;
```

Defined in: [src/sensors/bno086/sh2.ts:439](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L439)

***

### data0

```ts
data0: number;
```

Defined in: [src/sensors/bno086/sh2.ts:440](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L440)

***

### data1

```ts
data1: number;
```

Defined in: [src/sensors/bno086/sh2.ts:441](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L441)

***

### frsType

```ts
frsType: number;
```

Defined in: [src/sensors/bno086/sh2.ts:442](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L442)

# Interface: FrsWriteResponse

Defined in: [src/sensors/bno086/sh2.ts:489](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L489)

FRS Write Response (0xF5), 4 bytes.

## Properties

### status

```ts
status: number;
```

Defined in: [src/sensors/bno086/sh2.ts:491](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L491)

FrsWriteStatus.

***

### offsetWords

```ts
offsetWords: number;
```

Defined in: [src/sensors/bno086/sh2.ts:492](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L492)

# Interface: FwDepzImage

Defined in: [src/protocol/fwdepz.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/fwdepz.ts#L14)

## Properties

### loadAddr

```ts
loadAddr: number;
```

Defined in: [src/protocol/fwdepz.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/fwdepz.ts#L15)

***

### fwSize

```ts
fwSize: number;
```

Defined in: [src/protocol/fwdepz.ts:16](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/fwdepz.ts#L16)

***

### fwCrc32

```ts
fwCrc32: number;
```

Defined in: [src/protocol/fwdepz.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/fwdepz.ts#L17)

***

### curSec

```ts
curSec: number;
```

Defined in: [src/protocol/fwdepz.ts:18](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/fwdepz.ts#L18)

***

### totSec

```ts
totSec: number;
```

Defined in: [src/protocol/fwdepz.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/fwdepz.ts#L19)

***

### payload

```ts
payload: Uint8Array;
```

Defined in: [src/protocol/fwdepz.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/fwdepz.ts#L20)

# Interface: GenericEvent

Defined in: [src/sensors/bno086/reports.ts:338](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L338)

Simple u16-value detectors: 0x1A flip, 0x1B pickup, 0x1C stability
detector, 0x1F sleep, 0x20 tilt, 0x21 pocket, 0x22 circle, 0x23 HR.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "GenericEvent";
```

Defined in: [src/sensors/bno086/reports.ts:339](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L339)

***

### valueRaw

```ts
valueRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:340](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L340)

# Interface: GyroIntegratedRV

Defined in: [src/sensors/bno086/reports.ts:268](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L268)

0x2A gyro-integrated rotation vector (channel 5, dense — no SH-2 header).
Quaternion Q14, angular velocity Q10 rad/s.

## Extends

- [`ReportBase`](ReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`ReportBase`](ReportBase.md).[`sensorId`](ReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`ReportBase`](ReportBase.md).[`timestampUs`](ReportBase.md#timestampus)

***

### type

```ts
type: "GyroIntegratedRV";
```

Defined in: [src/sensors/bno086/reports.ts:269](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L269)

***

### iRaw

```ts
iRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:270](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L270)

***

### jRaw

```ts
jRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:271](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L271)

***

### kRaw

```ts
kRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:272](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L272)

***

### realRaw

```ts
realRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:273](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L273)

***

### vxRaw

```ts
vxRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:274](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L274)

***

### vyRaw

```ts
vyRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:275](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L275)

***

### vzRaw

```ts
vzRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:276](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L276)

***

### i

```ts
i: number;
```

Defined in: [src/sensors/bno086/reports.ts:277](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L277)

***

### j

```ts
j: number;
```

Defined in: [src/sensors/bno086/reports.ts:278](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L278)

***

### k

```ts
k: number;
```

Defined in: [src/sensors/bno086/reports.ts:279](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L279)

***

### real

```ts
real: number;
```

Defined in: [src/sensors/bno086/reports.ts:280](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L280)

***

### angularVelocity

```ts
angularVelocity: [number, number, number];
```

Defined in: [src/sensors/bno086/reports.ts:282](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L282)

rad/s.

# Interface: Gyroscope

Defined in: [src/sensors/bno086/reports.ts:190](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L190)

0x02 calibrated gyroscope (Q9).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "Gyroscope";
```

Defined in: [src/sensors/bno086/reports.ts:191](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L191)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:192](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L192)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:193](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L193)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:194](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L194)

***

### x

```ts
x: number;
```

Defined in: [src/sensors/bno086/reports.ts:196](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L196)

rad/s.

***

### y

```ts
y: number;
```

Defined in: [src/sensors/bno086/reports.ts:197](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L197)

***

### z

```ts
z: number;
```

Defined in: [src/sensors/bno086/reports.ts:198](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L198)

# Interface: Identity

Defined in: [src/protocol/identity.ts:5](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/identity.ts#L5)

## Properties

### mode

```ts
mode: "unknown" | "app" | "bootloader";
```

Defined in: [src/protocol/identity.ts:6](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/identity.ts#L6)

***

### sensorType

```ts
sensorType: SensorType | null;
```

Defined in: [src/protocol/identity.ts:7](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/identity.ts#L7)

***

### softwareName

```ts
softwareName: string;
```

Defined in: [src/protocol/identity.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/identity.ts#L8)

***

### version

```ts
version: string;
```

Defined in: [src/protocol/identity.ts:9](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/identity.ts#L9)

# Interface: InputReportBase

Defined in: [src/sensors/bno086/reports.ts:168](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L168)

Channel-3/4 report with the common SH-2 header fields.

## Extends

- [`ReportBase`](ReportBase.md)

## Extended by

- [`Acceleration`](Acceleration.md)
- [`Gyroscope`](Gyroscope.md)
- [`Magnetometer`](Magnetometer.md)
- [`UncalibratedGyroscope`](UncalibratedGyroscope.md)
- [`UncalibratedMagnetometer`](UncalibratedMagnetometer.md)
- [`RotationVector`](RotationVector.md)
- [`ScalarReport`](ScalarReport.md)
- [`TapDetector`](TapDetector.md)
- [`StepCounter`](StepCounter.md)
- [`StepDetector`](StepDetector.md)
- [`SignificantMotion`](SignificantMotion.md)
- [`StabilityClassifier`](StabilityClassifier.md)
- [`ShakeDetector`](ShakeDetector.md)
- [`GenericEvent`](GenericEvent.md)
- [`PersonalActivityClassifier`](PersonalActivityClassifier.md)
- [`RawSensor`](RawSensor.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`ReportBase`](ReportBase.md).[`sensorId`](ReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`ReportBase`](ReportBase.md).[`timestampUs`](ReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

# Interface: LinkStats

Defined in: [src/device/link.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L20)

Link statistics (contract 07 §2 `stats`).

## Properties

### rxPackets

```ts
rxPackets: number;
```

Defined in: [src/device/link.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L21)

***

### txPackets

```ts
txPackets: number;
```

Defined in: [src/device/link.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L22)

***

### rxBytes

```ts
rxBytes: number;
```

Defined in: [src/device/link.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L23)

***

### txBytes

```ts
txBytes: number;
```

Defined in: [src/device/link.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L24)

***

### crcErrors

```ts
crcErrors: number;
```

Defined in: [src/device/link.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L25)

***

### headerErrors

```ts
headerErrors: number;
```

Defined in: [src/device/link.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L26)

***

### trashBytes

```ts
trashBytes: number;
```

Defined in: [src/device/link.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L27)

***

### seqGaps

```ts
seqGaps: number;
```

Defined in: [src/device/link.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/link.ts#L29)

Host-observed gaps in the device→host seq counter (mod 256).

# Interface: ListOptions

Defined in: [src/discovery.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L63)

## Properties

### matchUsb?

```ts
optional matchUsb?: boolean;
```

Defined in: [src/discovery.ts:65](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L65)

Probe only ports with a known DEPZ (vid,pid) (default).

***

### timeoutMs?

```ts
optional timeoutMs?: number;
```

Defined in: [src/discovery.ts:66](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L66)

# Interface: Magnetometer

Defined in: [src/sensors/bno086/reports.ts:202](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L202)

0x03 calibrated magnetic field (Q4).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "Magnetometer";
```

Defined in: [src/sensors/bno086/reports.ts:203](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L203)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:204](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L204)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:205](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L205)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:206](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L206)

***

### x

```ts
x: number;
```

Defined in: [src/sensors/bno086/reports.ts:208](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L208)

µT.

***

### y

```ts
y: number;
```

Defined in: [src/sensors/bno086/reports.ts:209](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L209)

***

### z

```ts
z: number;
```

Defined in: [src/sensors/bno086/reports.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L210)

# Interface: MotionResult

Defined in: [src/sensors/vl53l8/uld.ts:257](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L257)

Motion-indicator results block (mirror of the Python `motion_indicator` dict).

## Properties

### globalIndicator1

```ts
globalIndicator1: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:258](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L258)

***

### globalIndicator2

```ts
globalIndicator2: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:259](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L259)

***

### status

```ts
status: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:260](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L260)

***

### nbOfDetectedAggregates

```ts
nbOfDetectedAggregates: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:261](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L261)

***

### nbOfAggregates

```ts
nbOfAggregates: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:262](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L262)

***

### motion

```ts
motion: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:263](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L263)

# Interface: OpenDeviceOptions

Defined in: [src/discovery.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L56)

## Properties

### serial?

```ts
optional serial?: string;
```

Defined in: [src/discovery.ts:58](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L58)

Select the candidate whose USB iSerial equals this exact string.

***

### timeoutMs?

```ts
optional timeoutMs?: number;
```

Defined in: [src/discovery.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L60)

Per-request timeout for the probe and the opened device (ms).

# Interface: PacketEvent

Defined in: [src/protocol/framing.ts:69](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L69)

## Properties

### type

```ts
type: "packet";
```

Defined in: [src/protocol/framing.ts:70](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L70)

***

### cmd

```ts
cmd: number;
```

Defined in: [src/protocol/framing.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L71)

***

### seq

```ts
seq: number;
```

Defined in: [src/protocol/framing.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L72)

***

### payload

```ts
payload: Uint8Array;
```

Defined in: [src/protocol/framing.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L73)

# Interface: PersonalActivityClassifier

Defined in: [src/sensors/bno086/reports.ts:347](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L347)

0x1E personal activity classifier (see ACTIVITY_NAMES).
`confidences` are 0–100 per state, states 0–9 of the current page.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "PersonalActivityClassifier";
```

Defined in: [src/sensors/bno086/reports.ts:348](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L348)

***

### pageNumber

```ts
pageNumber: number;
```

Defined in: [src/sensors/bno086/reports.ts:349](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L349)

***

### endOfSequence

```ts
endOfSequence: boolean;
```

Defined in: [src/sensors/bno086/reports.ts:350](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L350)

***

### mostLikelyState

```ts
mostLikelyState: number;
```

Defined in: [src/sensors/bno086/reports.ts:351](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L351)

***

### confidences

```ts
confidences: number[];
```

Defined in: [src/sensors/bno086/reports.ts:352](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L352)

***

### mostLikelyName

```ts
mostLikelyName: string;
```

Defined in: [src/sensors/bno086/reports.ts:353](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L353)

# Interface: ProductId

Defined in: [src/sensors/bno086/sh2.ts:206](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L206)

Product ID Response (0xF8), 16 bytes. The sensor sends one response per
subsystem (typically 2); resetCause per SH-2 §6.4.5.2.

## Properties

### resetCause

```ts
resetCause: number;
```

Defined in: [src/sensors/bno086/sh2.ts:207](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L207)

***

### swVersionMajor

```ts
swVersionMajor: number;
```

Defined in: [src/sensors/bno086/sh2.ts:208](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L208)

***

### swVersionMinor

```ts
swVersionMinor: number;
```

Defined in: [src/sensors/bno086/sh2.ts:209](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L209)

***

### swPartNumber

```ts
swPartNumber: number;
```

Defined in: [src/sensors/bno086/sh2.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L210)

***

### swBuildNumber

```ts
swBuildNumber: number;
```

Defined in: [src/sensors/bno086/sh2.ts:211](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L211)

***

### swVersionPatch

```ts
swVersionPatch: number;
```

Defined in: [src/sensors/bno086/sh2.ts:212](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L212)

***

### version

```ts
version: string;
```

Defined in: [src/sensors/bno086/sh2.ts:214](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L214)

"major.minor.patch".

# Interface: RangeTiming

Defined in: [src/sensors/vl53l4/uld.ts:278](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L278)

GetRangeTiming decode result.

## Properties

### timingBudgetMs

```ts
timingBudgetMs: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:279](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L279)

***

### interMeasurementMs

```ts
interMeasurementMs: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:280](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L280)

# Interface: RangeTimingRegisters

Defined in: [src/sensors/vl53l4/uld.ts:218](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L218)

SetRangeTiming register words.

## Properties

### rangeConfigA

```ts
rangeConfigA: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:219](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L219)

***

### rangeConfigB

```ts
rangeConfigB: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:220](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L220)

***

### intermeasurementRaw

```ts
intermeasurementRaw: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:222](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L222)

INTERMEASUREMENT_MS raw dword.

# Interface: RawSensor

Defined in: [src/sensors/bno086/reports.ts:360](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L360)

0x14/0x15/0x16 raw ADC samples + sensor-clock timestamp (u32 µs).
`temperatureRaw` is populated only for the raw gyroscope.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "RawSensor";
```

Defined in: [src/sensors/bno086/reports.ts:361](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L361)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:362](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L362)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:363](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L363)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:364](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L364)

***

### sensorTimestampUs

```ts
sensorTimestampUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:365](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L365)

***

### temperatureRaw

```ts
temperatureRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:366](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L366)

# Interface: RegData

Defined in: [src/protocol/vl53l8.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L52)

RPT_REG_DATA payload.

## Properties

### cmd

```ts
cmd: number;
```

Defined in: [src/protocol/vl53l8.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L54)

Echoed READ_REG opcode.

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/protocol/vl53l8.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L55)

***

### data

```ts
data: Uint8Array;
```

Defined in: [src/protocol/vl53l8.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L56)

# Interface: ReportBase

Defined in: [src/sensors/bno086/reports.ts:162](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L162)

Base for anything the sensor pushes; `timestampUs` is absolute in the MCU
clock domain (bridge capture time corrected by timebase + delay).

## Extended by

- [`InputReportBase`](InputReportBase.md)
- [`GyroIntegratedRV`](GyroIntegratedRV.md)
- [`UnknownReport`](UnknownReport.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

# Interface: RequestOptions\<T\>

Defined in: [src/device/device.ts:86](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L86)

## Type Parameters

| Type Parameter |
| ------ |
| `T` |

## Properties

### matcher?

```ts
optional matcher?: Matcher<T>;
```

Defined in: [src/device/device.ts:88](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L88)

First shot at every non-status packet while the request is pending.

***

### okCompletes?

```ts
optional okCompletes?: boolean;
```

Defined in: [src/device/device.ts:90](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L90)

Resolve on RPT_STATUS(cmd, OK) — for commands whose success reply is the ack.

***

### timeoutMs?

```ts
optional timeoutMs?: number;
```

Defined in: [src/device/device.ts:91](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L91)

# Interface: RotationVector

Defined in: [src/sensors/bno086/reports.ts:249](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L249)

Quaternion reports 0x05/0x08/0x09/0x28/0x29 (unit quaternion, Q14).
`accuracyRaw` (Q12, radians) is present only for 0x05/0x09/0x28.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "RotationVector";
```

Defined in: [src/sensors/bno086/reports.ts:250](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L250)

***

### iRaw

```ts
iRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:251](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L251)

***

### jRaw

```ts
jRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:252](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L252)

***

### kRaw

```ts
kRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:253](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L253)

***

### realRaw

```ts
realRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:254](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L254)

***

### accuracyRaw

```ts
accuracyRaw: number | null;
```

Defined in: [src/sensors/bno086/reports.ts:255](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L255)

***

### i

```ts
i: number;
```

Defined in: [src/sensors/bno086/reports.ts:256](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L256)

***

### j

```ts
j: number;
```

Defined in: [src/sensors/bno086/reports.ts:257](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L257)

***

### k

```ts
k: number;
```

Defined in: [src/sensors/bno086/reports.ts:258](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L258)

***

### real

```ts
real: number;
```

Defined in: [src/sensors/bno086/reports.ts:259](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L259)

***

### accuracyRad

```ts
accuracyRad: number | null;
```

Defined in: [src/sensors/bno086/reports.ts:261](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L261)

Estimated heading accuracy in radians (null for game variants).

# Interface: ScalarReport

Defined in: [src/sensors/bno086/reports.ts:286](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L286)

Environment reports 0x0A–0x0E: single value, Q from Q_POINTS.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "ScalarReport";
```

Defined in: [src/sensors/bno086/reports.ts:287](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L287)

***

### valueRaw

```ts
valueRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:288](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L288)

***

### value

```ts
value: number;
```

Defined in: [src/sensors/bno086/reports.ts:289](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L289)

# Interface: SensorMetadata

Defined in: [src/sensors/bno086/sh2.ts:614](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L614)

Parsed sensor metadata FRS record; `rawWords` is authoritative.

Field packing follows the sh2 reference driver (revision-gated fields are
0 when the record predates them).

## Properties

### meVersion

```ts
meVersion: number;
```

Defined in: [src/sensors/bno086/sh2.ts:615](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L615)

***

### mhVersion

```ts
mhVersion: number;
```

Defined in: [src/sensors/bno086/sh2.ts:616](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L616)

***

### shVersion

```ts
shVersion: number;
```

Defined in: [src/sensors/bno086/sh2.ts:617](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L617)

***

### rangeRaw

```ts
rangeRaw: number;
```

Defined in: [src/sensors/bno086/sh2.ts:619](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L619)

Same units & Q point as the sensor's reports.

***

### resolutionRaw

```ts
resolutionRaw: number;
```

Defined in: [src/sensors/bno086/sh2.ts:620](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L620)

***

### revision

```ts
revision: number;
```

Defined in: [src/sensors/bno086/sh2.ts:621](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L621)

***

### powerMaQ10

```ts
powerMaQ10: number;
```

Defined in: [src/sensors/bno086/sh2.ts:623](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L623)

mA in Q10.

***

### minPeriodUs

```ts
minPeriodUs: number;
```

Defined in: [src/sensors/bno086/sh2.ts:624](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L624)

***

### maxPeriodUs

```ts
maxPeriodUs: number;
```

Defined in: [src/sensors/bno086/sh2.ts:626](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L626)

Revision >= 4 only.

***

### fifoMax

```ts
fifoMax: number;
```

Defined in: [src/sensors/bno086/sh2.ts:627](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L627)

***

### fifoReserved

```ts
fifoReserved: number;
```

Defined in: [src/sensors/bno086/sh2.ts:628](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L628)

***

### batchBufferBytes

```ts
batchBufferBytes: number;
```

Defined in: [src/sensors/bno086/sh2.ts:629](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L629)

***

### qPoint1

```ts
qPoint1: number;
```

Defined in: [src/sensors/bno086/sh2.ts:630](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L630)

***

### qPoint2

```ts
qPoint2: number;
```

Defined in: [src/sensors/bno086/sh2.ts:631](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L631)

***

### qPoint3

```ts
qPoint3: number;
```

Defined in: [src/sensors/bno086/sh2.ts:633](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L633)

Revision >= 3 only.

***

### rawWords

```ts
rawWords: number[];
```

Defined in: [src/sensors/bno086/sh2.ts:634](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L634)

# Interface: SequenceErrorReport

Defined in: [src/protocol/common.ts:86](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L86)

## Properties

### expectedSeq

```ts
expectedSeq: number;
```

Defined in: [src/protocol/common.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L87)

***

### receivedSeq

```ts
receivedSeq: number;
```

Defined in: [src/protocol/common.ts:88](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L88)

# Interface: SerialTransport

Defined in: [src/transport/types.ts:14](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/types.ts#L14)

## Properties

### info

```ts
readonly info: SerialTransportInfo;
```

Defined in: [src/transport/types.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/types.ts#L20)

## Methods

### open()

```ts
open(opts?): Promise<void>;
```

Defined in: [src/transport/types.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/types.ts#L15)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `opts?` | \{ `baudRate?`: `number`; \} |
| `opts.baudRate?` | `number` |

#### Returns

`Promise`\<`void`\>

***

### write()

```ts
write(data): Promise<void>;
```

Defined in: [src/transport/types.ts:16](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/types.ts#L16)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

***

### readable()

```ts
readable(): AsyncIterableIterator<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/transport/types.ts:18](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/types.ts#L18)

Raw chunks as they arrive; ends on close/disconnect.

#### Returns

`AsyncIterableIterator`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/transport/types.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/types.ts#L19)

#### Returns

`Promise`\<`void`\>

***

### onDisconnect()

```ts
onDisconnect(cb): () => void;
```

Defined in: [src/transport/types.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/types.ts#L21)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | () => `void` |

#### Returns

() => `void`

# Interface: SerialTransportInfo

Defined in: [src/transport/types.ts:7](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/types.ts#L7)

Byte-transport abstraction (contracts/07_SDK_FACADE.md §1 "link").
WebSerial (browser), serialport (Node), loopback (tests) and .depzrec
replay all implement this. The SDK core touches nothing else.

## Properties

### usbVendorId?

```ts
optional usbVendorId?: number;
```

Defined in: [src/transport/types.ts:8](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/types.ts#L8)

***

### usbProductId?

```ts
optional usbProductId?: number;
```

Defined in: [src/transport/types.ts:9](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/types.ts#L9)

***

### path?

```ts
optional path?: string;
```

Defined in: [src/transport/types.ts:10](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/types.ts#L10)

***

### serialNumber?

```ts
optional serialNumber?: string;
```

Defined in: [src/transport/types.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/types.ts#L11)

# Interface: ShakeDetector

Defined in: [src/sensors/bno086/reports.ts:329](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L329)

0x19 shake detector; bits 0/1/2 = X/Y/Z shake.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "ShakeDetector";
```

Defined in: [src/sensors/bno086/reports.ts:330](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L330)

***

### flags

```ts
flags: number;
```

Defined in: [src/sensors/bno086/reports.ts:331](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L331)

# Interface: ShtpCargo

Defined in: [src/sensors/bno086/shtp.ts:74](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L74)

One reassembled cargo: `payload` excludes all SHTP headers.

## Properties

### channel

```ts
channel: number;
```

Defined in: [src/sensors/bno086/shtp.ts:75](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L75)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/shtp.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L77)

seq of the first fragment.

***

### payload

```ts
payload: Uint8Array;
```

Defined in: [src/sensors/bno086/shtp.ts:78](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L78)

# Interface: ShtpHeader

Defined in: [src/sensors/bno086/shtp.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L44)

## Properties

### length

```ts
length: number;
```

Defined in: [src/sensors/bno086/shtp.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L46)

Bits 14:0 — cargo length incl. this 4-byte header.

***

### channel

```ts
channel: number;
```

Defined in: [src/sensors/bno086/shtp.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L47)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/shtp.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L48)

***

### continuation

```ts
continuation: boolean;
```

Defined in: [src/sensors/bno086/shtp.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L49)

# Interface: SignificantMotion

Defined in: [src/sensors/bno086/reports.ts:316](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L316)

0x12 significant motion (1 = motion detected; sensor auto-disables).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "SignificantMotion";
```

Defined in: [src/sensors/bno086/reports.ts:317](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L317)

***

### motion

```ts
motion: number;
```

Defined in: [src/sensors/bno086/reports.ts:318](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L318)

# Interface: Sr04Data

Defined in: [src/protocol/sr04.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L27)

## Properties

### sourceCmd

```ts
sourceCmd: number;
```

Defined in: [src/protocol/sr04.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L29)

0x36 single shot (host or SYNC_IN), 0x37 loop sample (ERRATA E3).

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/protocol/sr04.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L30)

***

### echoTimeUs

```ts
echoTimeUs: number;
```

Defined in: [src/protocol/sr04.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L31)

# Interface: Sr04Measurement

Defined in: [src/sensors/sr04.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L22)

One ranging result. `valid` is false for the no-echo timeout sentinel.

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/sr04.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L23)

***

### echoTimeUs

```ts
echoTimeUs: number;
```

Defined in: [src/sensors/sr04.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L24)

***

### source

```ts
source: "once" | "loop";
```

Defined in: [src/sensors/sr04.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L26)

"once": host command or SYNC_IN edge; "loop": measurement loop sample.

***

### valid

```ts
valid: boolean;
```

Defined in: [src/sensors/sr04.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L27)

***

### distanceMm

```ts
distanceMm: number | null;
```

Defined in: [src/sensors/sr04.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/sr04.ts#L29)

Distance at 343 m/s, or null when no echo was received.

# Interface: StabilityClassifier

Defined in: [src/sensors/bno086/reports.ts:322](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L322)

0x13 stability classification (see STABILITY_NAMES).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "StabilityClassifier";
```

Defined in: [src/sensors/bno086/reports.ts:323](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L323)

***

### classification

```ts
classification: number;
```

Defined in: [src/sensors/bno086/reports.ts:324](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L324)

***

### name

```ts
name: string;
```

Defined in: [src/sensors/bno086/reports.ts:325](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L325)

# Interface: StatusReport

Defined in: [src/protocol/common.ts:65](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L65)

## Properties

### cmd

```ts
cmd: number;
```

Defined in: [src/protocol/common.ts:66](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L66)

***

### status

```ts
status: number;
```

Defined in: [src/protocol/common.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L67)

# Interface: StepCounter

Defined in: [src/sensors/bno086/reports.ts:303](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L303)

0x11 step counter. NOTE: latency u32 µs at bytes 4–7, steps u16 at bytes
8–9 per SH-2; the vendor tool's `latency(2)+steps(2)` comment is wrong.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "StepCounter";
```

Defined in: [src/sensors/bno086/reports.ts:304](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L304)

***

### latencyUs

```ts
latencyUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:305](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L305)

***

### steps

```ts
steps: number;
```

Defined in: [src/sensors/bno086/reports.ts:306](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L306)

# Interface: StepDetector

Defined in: [src/sensors/bno086/reports.ts:310](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L310)

0x18 step detector; latency from step event to report, µs.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "StepDetector";
```

Defined in: [src/sensors/bno086/reports.ts:311](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L311)

***

### latencyUs

```ts
latencyUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:312](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L312)

# Interface: SyncPinConfig

Defined in: [src/protocol/common.ts:91](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L91)

## Properties

### pin

```ts
pin: number;
```

Defined in: [src/protocol/common.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L92)

***

### mode

```ts
mode: SyncPinMode;
```

Defined in: [src/protocol/common.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L93)

***

### polarity

```ts
polarity: SyncPinPolarity;
```

Defined in: [src/protocol/common.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L94)

# Interface: SyncTimeReport

Defined in: [src/protocol/common.ts:75](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L75)

## Properties

### pcTimestampUs

```ts
pcTimestampUs: bigint;
```

Defined in: [src/protocol/common.ts:76](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L76)

***

### mcuRxUs

```ts
mcuRxUs: bigint;
```

Defined in: [src/protocol/common.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L77)

***

### mcuTxUs

```ts
mcuTxUs: bigint;
```

Defined in: [src/protocol/common.ts:78](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L78)

# Interface: TapDetector

Defined in: [src/sensors/bno086/reports.ts:293](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L293)

0x10 tap detector; `flags` bit 6 = double tap, bits 0–5 axis/sign.

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "TapDetector";
```

Defined in: [src/sensors/bno086/reports.ts:294](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L294)

***

### flags

```ts
flags: number;
```

Defined in: [src/sensors/bno086/reports.ts:295](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L295)

***

### doubleTap

```ts
doubleTap: boolean;
```

Defined in: [src/sensors/bno086/reports.ts:296](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L296)

# Interface: TemperatureReport

Defined in: [src/protocol/common.ts:81](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L81)

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/protocol/common.ts:82](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L82)

***

### rawDecidegrees

```ts
rawDecidegrees: number;
```

Defined in: [src/protocol/common.ts:83](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L83)

# Interface: TextReport

Defined in: [src/protocol/common.ts:70](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L70)

## Properties

### cmd

```ts
cmd: number;
```

Defined in: [src/protocol/common.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L71)

***

### text

```ts
text: string;
```

Defined in: [src/protocol/common.ts:72](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L72)

# Interface: TimeSync

Defined in: [src/device/device.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L64)

Result of `syncTime()` (contract 02 §5). offset = device − host clock.

## Properties

### offsetUs

```ts
offsetUs: bigint;
```

Defined in: [src/device/device.ts:65](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L65)

***

### rttUs

```ts
rttUs: bigint;
```

Defined in: [src/device/device.ts:66](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L66)

***

### syncedAtHostUs

```ts
syncedAtHostUs: bigint;
```

Defined in: [src/device/device.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L67)

# Interface: TrashEvent

Defined in: [src/protocol/framing.ts:81](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L81)

Bytes discarded while hunting for a valid frame. Boundaries between
consecutive trash events depend on read chunking; only the concatenated
byte stream is deterministic.

## Properties

### type

```ts
type: "trash";
```

Defined in: [src/protocol/framing.ts:82](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L82)

***

### data

```ts
data: Uint8Array;
```

Defined in: [src/protocol/framing.ts:83](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L83)

# Interface: UncalibratedGyroscope

Defined in: [src/sensors/bno086/reports.ts:214](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L214)

0x07 uncalibrated gyroscope + bias estimate (all Q9, rad/s).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "UncalibratedGyroscope";
```

Defined in: [src/sensors/bno086/reports.ts:215](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L215)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:216](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L216)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:217](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L217)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:218](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L218)

***

### biasXRaw

```ts
biasXRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:219](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L219)

***

### biasYRaw

```ts
biasYRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:220](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L220)

***

### biasZRaw

```ts
biasZRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:221](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L221)

***

### x

```ts
x: number;
```

Defined in: [src/sensors/bno086/reports.ts:222](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L222)

***

### y

```ts
y: number;
```

Defined in: [src/sensors/bno086/reports.ts:223](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L223)

***

### z

```ts
z: number;
```

Defined in: [src/sensors/bno086/reports.ts:224](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L224)

***

### bias

```ts
bias: [number, number, number];
```

Defined in: [src/sensors/bno086/reports.ts:226](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L226)

rad/s.

# Interface: UncalibratedMagnetometer

Defined in: [src/sensors/bno086/reports.ts:230](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L230)

0x0F uncalibrated magnetic field + hard-iron bias (all Q4, µT).

## Extends

- [`InputReportBase`](InputReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`sensorId`](InputReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`timestampUs`](InputReportBase.md#timestampus)

***

### seq

```ts
seq: number;
```

Defined in: [src/sensors/bno086/reports.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L170)

8-bit rolling sample counter (drop detection).

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`seq`](InputReportBase.md#seq)

***

### accuracy

```ts
accuracy: number;
```

Defined in: [src/sensors/bno086/reports.ts:172](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L172)

Status bits 1:0 — 0 unreliable … 3 high.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`accuracy`](InputReportBase.md#accuracy)

***

### delayUs

```ts
delayUs: number;
```

Defined in: [src/sensors/bno086/reports.ts:174](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L174)

Report delay already folded into timestampUs.

#### Inherited from

[`InputReportBase`](InputReportBase.md).[`delayUs`](InputReportBase.md#delayus)

***

### type

```ts
type: "UncalibratedMagnetometer";
```

Defined in: [src/sensors/bno086/reports.ts:231](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L231)

***

### xRaw

```ts
xRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:232](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L232)

***

### yRaw

```ts
yRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:233](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L233)

***

### zRaw

```ts
zRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:234](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L234)

***

### biasXRaw

```ts
biasXRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:235](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L235)

***

### biasYRaw

```ts
biasYRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:236](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L236)

***

### biasZRaw

```ts
biasZRaw: number;
```

Defined in: [src/sensors/bno086/reports.ts:237](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L237)

***

### x

```ts
x: number;
```

Defined in: [src/sensors/bno086/reports.ts:238](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L238)

***

### y

```ts
y: number;
```

Defined in: [src/sensors/bno086/reports.ts:239](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L239)

***

### z

```ts
z: number;
```

Defined in: [src/sensors/bno086/reports.ts:240](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L240)

***

### bias

```ts
bias: [number, number, number];
```

Defined in: [src/sensors/bno086/reports.ts:242](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L242)

µT.

# Interface: UnknownReport

Defined in: [src/sensors/bno086/reports.ts:373](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L373)

Unrecognized report ID: raw bytes from the ID to end of cargo (the length
is unknowable, so parsing stops here).

## Extends

- [`ReportBase`](ReportBase.md)

## Properties

### sensorId

```ts
sensorId: number;
```

Defined in: [src/sensors/bno086/reports.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L163)

#### Inherited from

[`ReportBase`](ReportBase.md).[`sensorId`](ReportBase.md#sensorid)

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/bno086/reports.ts:164](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L164)

#### Inherited from

[`ReportBase`](ReportBase.md).[`timestampUs`](ReportBase.md#timestampus)

***

### type

```ts
type: "UnknownReport";
```

Defined in: [src/sensors/bno086/reports.ts:374](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L374)

***

### data

```ts
data: Uint8Array;
```

Defined in: [src/sensors/bno086/reports.ts:375](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L375)

# Interface: Vl53l4Info

Defined in: [src/protocol/vl53l4.ts:108](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L108)

RPT_VL53_INFO — bridge diagnostics. Counters are free-running and wrap
silently; watch increments, not absolute values.

## Properties

### intEdges

```ts
intEdges: number;
```

Defined in: [src/protocol/vl53l4.ts:109](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L109)

***

### slotsSkipped

```ts
slotsSkipped: number;
```

Defined in: [src/protocol/vl53l4.ts:110](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L110)

***

### i2cErrors

```ts
i2cErrors: number;
```

Defined in: [src/protocol/vl53l4.ts:111](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L111)

***

### lastI2cError

```ts
lastI2cError: number;
```

Defined in: [src/protocol/vl53l4.ts:112](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L112)

***

### modelId

```ts
modelId: number;
```

Defined in: [src/protocol/vl53l4.ts:114](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L114)

0x010F..0x0110 — expected 0xEBAA.

***

### fwStatus

```ts
fwStatus: number;
```

Defined in: [src/protocol/vl53l4.ts:116](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L116)

0x00E5 — expected 0x03 (booted).

***

### initialized

```ts
initialized: number;
```

Defined in: [src/protocol/vl53l4.ts:118](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L118)

1 = MODEL_ID matched on this read.

***

### xshutLevel

```ts
xshutLevel: number;
```

Defined in: [src/protocol/vl53l4.ts:119](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L119)

***

### intLevel

```ts
intLevel: number;
```

Defined in: [src/protocol/vl53l4.ts:120](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L120)

***

### i2cKhz

```ts
i2cKhz: number;
```

Defined in: [src/protocol/vl53l4.ts:121](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L121)

# Interface: Vl53l4Measurement

Defined in: [src/sensors/vl53l4/vl53l4.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L46)

One decoded ranging result (VL53L4CD_ResultsData_t + MCU timestamp).

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L48)

MCU uptime at the INT edge (stream) / read (poll).

***

### rangeStatus

```ts
rangeStatus: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L50)

0 = valid (RANGE_STATUS_NAMES).

***

### distanceMm

```ts
distanceMm: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:51](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L51)

***

### sigmaMm

```ts
sigmaMm: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L52)

***

### signalRateKcps

```ts
signalRateKcps: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L53)

***

### ambientRateKcps

```ts
ambientRateKcps: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L54)

***

### signalPerSpadKcps

```ts
signalPerSpadKcps: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L55)

***

### ambientPerSpadKcps

```ts
ambientPerSpadKcps: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L56)

***

### numberOfSpad

```ts
numberOfSpad: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L57)

***

### streamCount

```ts
streamCount: number;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L59)

Sensor frame counter, wraps at 255.

***

### valid

```ts
valid: boolean;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:61](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L61)

rangeStatus === 0.

***

### statusText

```ts
statusText: string;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L63)

Human-readable range status.

# Interface: Vl53l4Options

Defined in: [src/sensors/vl53l4/vl53l4.ts:83](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L83)

## Extends

- [`DeviceOptions`](DeviceOptions.md)

## Properties

### timeoutMs?

```ts
optional timeoutMs?: number;
```

Defined in: [src/device/device.ts:169](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L169)

#### Inherited from

[`DeviceOptions`](DeviceOptions.md).[`timeoutMs`](DeviceOptions.md#timeoutms)

***

### txCrcType?

```ts
optional txCrcType?: CrcType;
```

Defined in: [src/device/device.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L170)

#### Inherited from

[`DeviceOptions`](DeviceOptions.md).[`txCrcType`](DeviceOptions.md#txcrctype)

***

### sleepImpl?

```ts
optional sleepImpl?: (ms) => Promise<void>;
```

Defined in: [src/sensors/vl53l4/vl53l4.ts:85](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/vl53l4.ts#L85)

ULD sleep implementation (tests inject an instant one).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Interface: Vl53l4Platform

Defined in: [src/sensors/vl53l4/uld.ts:135](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L135)

What the ULD needs from the register bridge.

## Methods

### rdMulti()

```ts
rdMulti(addr, size): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/sensors/vl53l4/uld.ts:136](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L136)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `size` | `number` |

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### wrMulti()

```ts
wrMulti(addr, data): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:137](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L137)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `data` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

***

### setI2cSpeed()

```ts
setI2cSpeed(khz): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:138](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L138)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `khz` | `number` |

#### Returns

`Promise`\<`void`\>

***

### sleepMs()

```ts
sleepMs(ms): Promise<void>;
```

Defined in: [src/sensors/vl53l4/uld.ts:139](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L139)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Interface: Vl53l4RegData

Defined in: [src/protocol/vl53l4.ts:79](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L79)

RPT_VL53_REG_DATA payload.

## Properties

### cmd

```ts
cmd: number;
```

Defined in: [src/protocol/vl53l4.ts:81](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L81)

Echoed READ_REG opcode.

***

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/protocol/vl53l4.ts:83](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L83)

MCU uptime at I2C-read completion.

***

### data

```ts
data: Uint8Array;
```

Defined in: [src/protocol/vl53l4.ts:84](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L84)

# Interface: Vl53l4Results

Defined in: [src/sensors/vl53l4/uld.ts:143](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L143)

VL53L4CD_ResultsData_t plus the sensor's own frame counter.

## Properties

### rangeStatus

```ts
rangeStatus: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:145](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L145)

0 = valid (RANGE_STATUS_NAMES).

***

### distanceMm

```ts
distanceMm: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:146](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L146)

***

### ambientRateKcps

```ts
ambientRateKcps: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:147](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L147)

***

### ambientPerSpadKcps

```ts
ambientPerSpadKcps: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:148](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L148)

***

### signalRateKcps

```ts
signalRateKcps: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:149](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L149)

***

### signalPerSpadKcps

```ts
signalPerSpadKcps: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:150](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L150)

***

### numberOfSpad

```ts
numberOfSpad: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:151](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L151)

***

### sigmaMm

```ts
sigmaMm: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:152](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L152)

***

### streamCount

```ts
streamCount: number;
```

Defined in: [src/sensors/vl53l4/uld.ts:158](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L158)

0x008B RESULT__STREAM_COUNT: wraps at 255. The C ULD ignores it; it is
what tells a frame the host never received from one the sensor never
produced.

# Interface: Vl53l4StreamData

Defined in: [src/protocol/vl53l4.ts:144](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L144)

RPT_VL53_STREAM — one streamed register block. `addr`/`length` echo the
stream configuration so each report is self-describing.

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/protocol/vl53l4.ts:146](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L146)

MCU uptime at the INT edge (the sensor event).

***

### addr

```ts
addr: number;
```

Defined in: [src/protocol/vl53l4.ts:147](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L147)

***

### length

```ts
length: number;
```

Defined in: [src/protocol/vl53l4.ts:148](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L148)

***

### data

```ts
data: Uint8Array;
```

Defined in: [src/protocol/vl53l4.ts:149](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L149)

# Interface: Vl53l8Assets

Defined in: [src/sensors/vl53l8/assets/index.ts:9](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L9)

## Properties

### firmware

```ts
firmware: Uint8Array;
```

Defined in: [src/sensors/vl53l8/assets/index.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L11)

firmware.bin — 86016 bytes (0x15000).

***

### defaultCfg

```ts
defaultCfg: Uint8Array;
```

Defined in: [src/sensors/vl53l8/assets/index.ts:13](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L13)

default_configuration.bin — 972 bytes.

***

### defaultXtalk

```ts
defaultXtalk: Uint8Array;
```

Defined in: [src/sensors/vl53l8/assets/index.ts:15](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L15)

default_xtalk.bin — 776 bytes.

***

### getNvmCmd

```ts
getNvmCmd: Uint8Array;
```

Defined in: [src/sensors/vl53l8/assets/index.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L17)

get_nvm_cmd.bin — 40 bytes.

# Interface: Vl53l8Frame

Defined in: [src/sensors/vl53l8/vl53l8.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L49)

One parsed ranging frame. Arrays are sized to the active resolution
(16 or 64 zones); zone index runs row-major (see datasheet zone maps).

## Properties

### timestampUs

```ts
timestampUs: bigint;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L50)

***

### resolution

```ts
resolution: number;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L52)

16 | 64

***

### distanceMm

```ts
distanceMm: Int32Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L53)

***

### targetStatus

```ts
targetStatus: Uint8Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L55)

5/9 = valid, 255 = no target.

***

### nbTargetDetected

```ts
nbTargetDetected: Uint8Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L56)

***

### signalPerSpad

```ts
signalPerSpad: Float64Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:58](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L58)

kcps/SPAD.

***

### ambientPerSpad

```ts
ambientPerSpad: Float64Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L60)

kcps/SPAD.

***

### nbSpadsEnabled

```ts
nbSpadsEnabled: Int32Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:61](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L61)

***

### rangeSigmaMm

```ts
rangeSigmaMm: Float64Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:62](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L62)

***

### reflectance

```ts
reflectance: Uint8Array;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L64)

%.

***

### siliconTempDegc

```ts
siliconTempDegc: number;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:65](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L65)

***

### cnhRaw

```ts
cnhRaw: Uint8Array<ArrayBufferLike> | null;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L67)

CH variant: raw CNH block (decode via cnh).

***

### motion

```ts
motion: MotionResult | null;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:69](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L69)

Motion-indicator output when `configureMotionIndicator()` is active.

# Interface: Vl53l8InitOptions

Defined in: [src/sensors/vl53l8/vl53l8.ts:89](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L89)

## Properties

### progress?

```ts
optional progress?: (text) => void;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:91](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L91)

Receives phase strings.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `text` | `string` |

#### Returns

`void`

***

### writeProgress?

```ts
optional writeProgress?: (done, total) => void;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L93)

Tracks the big blob writes: (done, total) bytes.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `done` | `number` |
| `total` | `number` |

#### Returns

`void`

# Interface: Vl53l8Options

Defined in: [src/sensors/vl53l8/vl53l8.ts:84](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L84)

## Extends

- [`DeviceOptions`](DeviceOptions.md)

## Properties

### timeoutMs?

```ts
optional timeoutMs?: number;
```

Defined in: [src/device/device.ts:169](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L169)

#### Inherited from

[`DeviceOptions`](DeviceOptions.md).[`timeoutMs`](DeviceOptions.md#timeoutms)

***

### txCrcType?

```ts
optional txCrcType?: CrcType;
```

Defined in: [src/device/device.ts:170](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L170)

#### Inherited from

[`DeviceOptions`](DeviceOptions.md).[`txCrcType`](DeviceOptions.md#txcrctype)

***

### sleepImpl?

```ts
optional sleepImpl?: (ms) => Promise<void>;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:86](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L86)

ULD sleep implementation (tests inject an instant one).

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Interface: Vl53l8Platform

Defined in: [src/sensors/vl53l8/uld.ts:19](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L19)

ULD platform: chunking/bridging is the caller's concern.

## Methods

### rdMulti()

```ts
rdMulti(addr, size): Promise<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/sensors/vl53l8/uld.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L20)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `size` | `number` |

#### Returns

`Promise`\<`Uint8Array`\<`ArrayBufferLike`\>\>

***

### wrMulti()

```ts
wrMulti(addr, data): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L21)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `addr` | `number` |
| `data` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

***

### sleepMs()

```ts
sleepMs(ms): Promise<void>;
```

Defined in: [src/sensors/vl53l8/uld.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L22)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `ms` | `number` |

#### Returns

`Promise`\<`void`\>

# Interface: Vl53l8Results

Defined in: [src/sensors/vl53l8/uld.ts:267](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L267)

One parsed raw results frame (plain arrays, mirroring the Python dict).

## Properties

### distanceMm

```ts
distanceMm: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:268](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L268)

***

### targetStatus

```ts
targetStatus: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:269](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L269)

***

### nbTargetDetected

```ts
nbTargetDetected: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:270](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L270)

***

### signalPerSpad

```ts
signalPerSpad: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:271](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L271)

***

### ambientPerSpad

```ts
ambientPerSpad: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:272](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L272)

***

### nbSpadsEnabled

```ts
nbSpadsEnabled: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:273](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L273)

***

### rangeSigmaMm

```ts
rangeSigmaMm: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:274](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L274)

***

### reflectance

```ts
reflectance: number[];
```

Defined in: [src/sensors/vl53l8/uld.ts:275](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L275)

***

### siliconTempDegc

```ts
siliconTempDegc: number;
```

Defined in: [src/sensors/vl53l8/uld.ts:276](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L276)

***

### cnhRaw

```ts
cnhRaw: Uint8Array<ArrayBufferLike> | null;
```

Defined in: [src/sensors/vl53l8/uld.ts:277](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L277)

***

### motion

```ts
motion: MotionResult | null;
```

Defined in: [src/sensors/vl53l8/uld.ts:279](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L279)

Motion-indicator output when the motion detector is configured.

# Interface: WebSocketLike

Defined in: [src/transport/ws-backend.ts:55](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L55)

Minimal `WebSocket` surface used here — real one or a test fake.

## Properties

### binaryType

```ts
binaryType: string;
```

Defined in: [src/transport/ws-backend.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L56)

***

### readyState

```ts
readyState: number;
```

Defined in: [src/transport/ws-backend.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L57)

***

### onopen

```ts
onopen: ((ev) => void) | null;
```

Defined in: [src/transport/ws-backend.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L60)

***

### onclose

```ts
onclose: ((ev) => void) | null;
```

Defined in: [src/transport/ws-backend.ts:61](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L61)

***

### onerror

```ts
onerror: ((ev) => void) | null;
```

Defined in: [src/transport/ws-backend.ts:62](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L62)

***

### onmessage

```ts
onmessage: ((ev) => void) | null;
```

Defined in: [src/transport/ws-backend.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L63)

## Methods

### send()

```ts
send(data): void;
```

Defined in: [src/transport/ws-backend.ts:58](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L58)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `ArrayBuffer` \| `ArrayBufferView`\<`ArrayBufferLike`\> |

#### Returns

`void`

***

### close()

```ts
close(): void;
```

Defined in: [src/transport/ws-backend.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L59)

#### Returns

`void`

# Interface: WsBackendTransportOptions

Defined in: [src/transport/ws-backend.ts:69](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L69)

## Properties

### baseUrl?

```ts
optional baseUrl?: string;
```

Defined in: [src/transport/ws-backend.ts:75](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L75)

Origin of the standalone server (e.g. `http://127.0.0.1:8000`). Omit /
empty to use the page's own origin (`location.origin`) — the usual case,
since the SPA is served *by* the backend.

***

### path

```ts
path: string;
```

Defined in: [src/transport/ws-backend.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L77)

Serial-port path from `GET /api/devices` — the `?path=` query value.

***

### baudRate?

```ts
optional baudRate?: number;
```

Defined in: [src/transport/ws-backend.ts:79](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L79)

Baud for the `?baud=` query (default 115200; `open()` may override).

***

### info?

```ts
optional info?: SerialTransportInfo;
```

Defined in: [src/transport/ws-backend.ts:81](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L81)

USB ids / serial to advertise (drives CX-vs-CH labelling upstream).

***

### webSocketFactory?

```ts
optional webSocketFactory?: (url) => WebSocketLike;
```

Defined in: [src/transport/ws-backend.ts:83](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L83)

WebSocket factory (defaults to the global `WebSocket`); injected in tests.

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `url` | `string` |

#### Returns

[`WebSocketLike`](WebSocketLike.md)

***

### openTimeoutMs?

```ts
optional openTimeoutMs?: number;
```

Defined in: [src/transport/ws-backend.ts:95](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/ws-backend.ts#L95)

Ceiling on `open()` (ms, default DEFAULT\_OPEN\_TIMEOUT\_MS).

The socket opening is not the same event as the *port* opening: the
backend accepts the WebSocket first, then opens the serial port in a
thread and only then sends `{"opened":true}`. If that open never returns
(a wedged adapter, a port held by something that never lets go), nothing
fires `onerror` or `onclose` and `open()` would await forever — taking the
caller's connect guard with it, which in the viewer leaves "Scan for
sensors" disabled with no way back but a reload. Pass `0` to disable.

# Type Alias: DeviceEvent

```ts
type DeviceEvent = 
  | {
  type: "sequenceError";
  expectedSeq: number;
  receivedSeq: number;
  reportedByDevice: boolean;
}
  | {
  type: "linkCrcError";
  cmd: number;
  seq: number;
}
  | {
  type: "trash";
  data: Uint8Array;
}
  | {
  type: "unsolicitedStatus";
  status: number;
}
  | {
  type: "text";
  cmd: number;
  text: string;
}
  | {
  type: "temperature";
  timestampUs: bigint;
  celsius: number;
}
  | {
  type: "disconnected";
  reason: string;
};
```

Defined in: [src/device/device.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L48)

Unsolicited/diagnostic events, mirrored from the Python event classes.

## Union Members

### Type Literal

```ts
{
  type: "sequenceError";
  expectedSeq: number;
  receivedSeq: number;
  reportedByDevice: boolean;
}
```

#### type

```ts
type: "sequenceError";
```

#### expectedSeq

```ts
expectedSeq: number;
```

#### receivedSeq

```ts
receivedSeq: number;
```

#### reportedByDevice

```ts
reportedByDevice: boolean;
```

true: device saw a gap in host TX (RPT 0x84).

***

### Type Literal

```ts
{
  type: "linkCrcError";
  cmd: number;
  seq: number;
}
```

***

### Type Literal

```ts
{
  type: "trash";
  data: Uint8Array;
}
```

***

### Type Literal

```ts
{
  type: "unsolicitedStatus";
  status: number;
}
```

***

### Type Literal

```ts
{
  type: "text";
  cmd: number;
  text: string;
}
```

***

### Type Literal

```ts
{
  type: "temperature";
  timestampUs: bigint;
  celsius: number;
}
```

***

### Type Literal

```ts
{
  type: "disconnected";
  reason: string;
}
```

# Type Alias: InputReport

```ts
type InputReport = 
  | Acceleration
  | Gyroscope
  | Magnetometer
  | UncalibratedGyroscope
  | UncalibratedMagnetometer
  | RotationVector
  | ScalarReport
  | TapDetector
  | StepCounter
  | StepDetector
  | SignificantMotion
  | StabilityClassifier
  | ShakeDetector
  | GenericEvent
  | PersonalActivityClassifier
  | RawSensor;
```

Defined in: [src/sensors/bno086/reports.ts:379](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L379)

Channel-3/4 typed reports (share the SH-2 input header fields).

# Type Alias: Matcher\<T\>

```ts
type Matcher<T> = (pkt) => T | typeof NO_MATCH;
```

Defined in: [src/device/device.ts:84](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L84)

Claims a packet for an in-flight request: return the parsed result, or
`NO_MATCH` to let other consumers see the packet.

## Type Parameters

| Type Parameter |
| ------ |
| `T` |

## Parameters

| Parameter | Type |
| ------ | ------ |
| `pkt` | [`PacketEvent`](../interfaces/PacketEvent.md) |

## Returns

`T` \| *typeof* [`NO_MATCH`](../variables/NO_MATCH.md)

# Type Alias: OpenTarget

```ts
type OpenTarget = 
  | number
  | string
  | DeviceInfo
  | null
  | undefined;
```

Defined in: [src/discovery.ts:70](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L70)

`undefined` = auto; number = Nth candidate; string = port path; DeviceInfo = its port.

# Type Alias: ParserEvent

```ts
type ParserEvent = 
  | PacketEvent
  | TrashEvent
  | CrcErrorEvent;
```

Defined in: [src/protocol/framing.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L93)

# Type Alias: PlayerState

```ts
type PlayerState = "idle" | "playing" | "paused" | "done";
```

Defined in: [src/dataset.ts:204](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L204)

# Type Alias: Report

```ts
type Report = 
  | InputReport
  | GyroIntegratedRV
  | UnknownReport;
```

Defined in: [src/sensors/bno086/reports.ts:398](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L398)

Anything the sensor pushes.

# Type Alias: SensorType

```ts
type SensorType = "sr04" | "vl53l4" | "vl53l8" | "bno086" | "unknown";
```

Defined in: [src/protocol/identity.ts:3](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/identity.ts#L3)

Firmware-name parsing (contracts/02_COMMON_COMMANDS.md §4).

# Type Alias: TransportFactory

```ts
type TransportFactory = (port) => SerialTransport;
```

Defined in: [src/discovery.ts:54](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/discovery.ts#L54)

Turns an enumerated port into a fresh, not-yet-open transport.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `port` | [`DepzPortInfo`](../interfaces/DepzPortInfo.md) |

## Returns

[`SerialTransport`](../interfaces/SerialTransport.md)

# Type Alias: Vl53l8

```ts
type Vl53l8 = Vl53l8Cx;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:521](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L521)

Backward-compatible alias: the old flat `Vl53l8` name maps to the CX base
class (the historic default). New code should pick Vl53l8Cx / Vl53l8Ch.

# Type Alias: Vl53l8Variant

```ts
type Vl53l8Variant = "cx" | "ch";
```

Defined in: [src/sensors/vl53l8/assets/index.ts:7](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/assets/index.ts#L7)

Lazy loader for the VL53L8 blob assets (sensor firmware + NVM/config
blobs). `loadAssets()` uses dynamic `import()` so the ~115 KB base64
modules stay out of bundles that never init the ToF sensor.

# Variable: ACTIVITY\_NAMES

```ts
const ACTIVITY_NAMES: Record<number, string>;
```

Defined in: [src/sensors/bno086/reports.ts:140](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L140)

# Variable: BASE\_TIMESTAMP\_REF

```ts
const BASE_TIMESTAMP_REF: 251 = 0xfb;
```

Defined in: [src/sensors/bno086/reports.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L60)

# Variable: BUSY\_BACKOFF\_MS

```ts
const BUSY_BACKOFF_MS: 200 = 200;
```

Defined in: [src/protocol/bno086.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L30)

# Variable: CALIBRATE\_XTALK

```ts
const CALIBRATE_XTALK: Uint8Array<ArrayBufferLike>;
```

Defined in: [src/sensors/vl53l8/uld.ts:163](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L163)

# Variable: CHUNK\_SIZE

```ts
const CHUNK_SIZE: 2048 = 2048;
```

Defined in: [src/protocol/vl53l8.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L24)

# Variable: CNH\_BIN\_WIDTH\_MM

```ts
const CNH_BIN_WIDTH_MM: 37.5348 = 37.5348;
```

Defined in: [src/sensors/vl53l8/cnh.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L24)

# Variable: CNH\_MAX\_DATA\_BYTES

```ts
const CNH_MAX_DATA_BYTES: number;
```

Defined in: [src/sensors/vl53l8/cnh.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L34)

# Variable: CONFIG\_ADDR

```ts
const CONFIG_ADDR: 45 = 0x002d;
```

Defined in: [src/sensors/vl53l4/uld.ts:61](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L61)

# Variable: CONFIG\_END

```ts
const CONFIG_END: 135 = 0x0087;
```

Defined in: [src/sensors/vl53l4/uld.ts:62](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L62)

# Variable: CONFIG\_FMP\_BYTE

```ts
const CONFIG_FMP_BYTE: 18 = 0x12;
```

Defined in: [src/sensors/vl53l4/uld.ts:88](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L88)

# Variable: CONTINUATION\_BIT

```ts
const CONTINUATION_BIT: 32768 = 0x8000;
```

Defined in: [src/sensors/bno086/shtp.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L38)

# Variable: COUNTS\_CLEAR

```ts
const COUNTS_CLEAR: 1 = 1;
```

Defined in: [src/sensors/bno086/sh2.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L64)

# Variable: COUNTS\_GET

```ts
const COUNTS_GET: 0 = 0;
```

Defined in: [src/sensors/bno086/sh2.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L63)

Counter subcommands (command 0x02, P0).

# Variable: DATASET\_SCHEMA

```ts
const DATASET_SCHEMA: "depz.dataset/1" = "depz.dataset/1";
```

Defined in: [src/dataset.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/dataset.ts#L11)

# Variable: DEFAULT\_CONFIGURATION

```ts
const DEFAULT_CONFIGURATION: Uint8Array;
```

Defined in: [src/sensors/vl53l4/uld.ts:73](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L73)

VL53L4CD_DEFAULT_CONFIGURATION[] — 91 bytes, registers 0x2D..0x87.
`configBlock()` always overrides byte 0 (register 0x2D) with
CONFIG_FMP_BYTE (0x12) to put the sensor's I2C pad in Fast Mode Plus —
exactly what VL53L4CD_I2C_FAST_MODE_PLUS does in the C ULD. FM+ pads work
at every bus step down to 100 kHz, so it is set unconditionally and never
cleared (clearing it mid-block NACKs and truncates the write).

# Variable: DEFAULT\_TIMEOUT\_MS

```ts
const DEFAULT_TIMEOUT_MS: 200 = 200;
```

Defined in: [src/device/device.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L43)

# Variable: DEPZ\_SENSOR\_PID\_MAX

```ts
const DEPZ_SENSOR_PID_MAX: 65535 = 65535;
```

Defined in: [src/protocol/usb-ids.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/usb-ids.ts#L31)

# Variable: DEPZ\_SENSOR\_PID\_MIN

```ts
const DEPZ_SENSOR_PID_MIN: 60536 = 60536;
```

Defined in: [src/protocol/usb-ids.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/usb-ids.ts#L30)

Contiguous production PID block: any PID in this inclusive range under the
production VID is treated as a candidate DEPZ sensor even when it is not
individually mapped below (e.g. future models, or unmapped catalog PIDs).

# Variable: DEPZ\_USB\_MODELS

```ts
const DEPZ_USB_MODELS: Readonly<Record<number, DepzUsbModel>>;
```

Defined in: [src/protocol/usb-ids.ts:51](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/usb-ids.ts#L51)

PID → model hint (full official DEPZ catalog). Only the `sensorType`-bearing
rows (sr04, vl53l4cd, vl53l8ch, vl53l8cx, bno086) are decodable by this SDK; the rest
are recognized as DEPZ sensors for discovery/labelling only. Values mirror
the Python reference `depz_sensor_sdk.usb_ids.DEPZ_PID_MODEL`.

# Variable: DEPZ\_VID

```ts
const DEPZ_VID: 7119 = 0x1bcf;
```

Defined in: [src/protocol/usb-ids.ts:16](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/usb-ids.ts#L16)

Production VID shared by all programmed DEPZ sensors.

# Variable: DEV\_PID

```ts
const DEV_PID: 22236 = 0x56dc;
```

Defined in: [src/protocol/usb-ids.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/usb-ids.ts#L23)

# Variable: DEV\_VID

```ts
const DEV_VID: 1155 = 0x0483;
```

Defined in: [src/protocol/usb-ids.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/usb-ids.ts#L22)

Dev / unprogrammed default (STMicroelectronics Virtual COM Port). Dev units
before product-metadata flashing enumerate here; treated as a candidate.

# Variable: ECHO\_DECAY\_DEFAULT\_US

```ts
const ECHO_DECAY_DEFAULT_US: 5000 = 5_000;
```

Defined in: [src/protocol/sr04.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L23)

# Variable: ECHO\_DECAY\_MAX\_US

```ts
const ECHO_DECAY_MAX_US: 65000 = 65_000;
```

Defined in: [src/protocol/sr04.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L25)

# Variable: ECHO\_DECAY\_MIN\_US

```ts
const ECHO_DECAY_MIN_US: 4000 = 4_000;
```

Defined in: [src/protocol/sr04.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L24)

# Variable: ECHO\_TIMEOUT

```ts
const ECHO_TIMEOUT: 65535 = 0xffff;
```

Defined in: [src/protocol/sr04.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L20)

echo_time_us sentinel: no echo received.

# Variable: FIRMWARE\_\_SYSTEM\_STATUS

```ts
const FIRMWARE__SYSTEM_STATUS: 229 = 0x00e5;
```

Defined in: [src/sensors/vl53l4/uld.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L50)

# Variable: FWDEPZ\_HEADER\_SIZE

```ts
const FWDEPZ_HEADER_SIZE: 64 = 64;
```

Defined in: [src/protocol/fwdepz.ts:10](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/fwdepz.ts#L10)

# Variable: FWDEPZ\_MAGIC

```ts
const FWDEPZ_MAGIC: "FWDEPZ00" = "FWDEPZ00";
```

Defined in: [src/protocol/fwdepz.ts:9](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/fwdepz.ts#L9)

# Variable: FW\_CHECKSUM

```ts
const FW_CHECKSUM: Record<Vl53l8Variant, number>;
```

Defined in: [src/sensors/vl53l8/uld.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L29)

# Variable: GET\_XTALK\_CMD

```ts
const GET_XTALK_CMD: Uint8Array<ArrayBufferLike>;
```

Defined in: [src/sensors/vl53l8/uld.ts:158](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L158)

# Variable: GPIO\_HV\_MUX\_\_CTRL

```ts
const GPIO_HV_MUX__CTRL: 48 = 0x0030;
```

Defined in: [src/sensors/vl53l4/uld.ts:31](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L31)

# Variable: GPIO\_\_TIO\_HV\_STATUS

```ts
const GPIO__TIO_HV_STATUS: 49 = 0x0031;
```

Defined in: [src/sensors/vl53l4/uld.ts:32](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L32)

# Variable: GYRO\_RV\_ANGVEL\_Q

```ts
const GYRO_RV_ANGVEL_Q: 10 = 10;
```

Defined in: [src/sensors/bno086/reports.ts:88](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L88)

# Variable: HEADER\_SIZE

```ts
const HEADER_SIZE: 7 = 7;
```

Defined in: [src/protocol/framing.ts:12](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L12)

# Variable: I2C\_KHZ\_BOOT

```ts
const I2C_KHZ_BOOT: 400 = 400;
```

Defined in: [src/sensors/vl53l4/uld.ts:98](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L98)

# Variable: I2C\_KHZ\_DEFAULT

```ts
const I2C_KHZ_DEFAULT: 1000 = 1000;
```

Defined in: [src/sensors/vl53l4/uld.ts:99](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L99)

# Variable: I2C\_SLAVE\_\_DEVICE\_ADDRESS

```ts
const I2C_SLAVE__DEVICE_ADDRESS: 1 = 0x0001;
```

Defined in: [src/sensors/vl53l4/uld.ts:21](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L21)

# Variable: IDENTIFICATION\_\_MODEL\_ID

```ts
const IDENTIFICATION__MODEL_ID: 271 = 0x010f;
```

Defined in: [src/sensors/vl53l4/uld.ts:51](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L51)

# Variable: INNER\_OFFSET\_MM

```ts
const INNER_OFFSET_MM: 32 = 0x0020;
```

Defined in: [src/sensors/vl53l4/uld.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L29)

# Variable: INTERMEASUREMENT\_MS

```ts
const INTERMEASUREMENT_MS: 108 = 0x006c;
```

Defined in: [src/sensors/vl53l4/uld.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L38)

# Variable: LENGTH\_MASK

```ts
const LENGTH_MASK: 32767 = 0x7fff;
```

Defined in: [src/sensors/bno086/shtp.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L37)

# Variable: MAGIC0

```ts
const MAGIC0: 165 = 0xa5;
```

Defined in: [src/protocol/framing.ts:10](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L10)

# Variable: MAGIC1

```ts
const MAGIC1: 195 = 0xc3;
```

Defined in: [src/protocol/framing.ts:11](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L11)

# Variable: MAX\_PAYLOAD

```ts
const MAX_PAYLOAD: 16383 = 0x3fff;
```

Defined in: [src/protocol/framing.ts:13](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/framing.ts#L13)

# Variable: MAX\_TX\_FRAME

```ts
const MAX_TX_FRAME: 64 = 64;
```

Defined in: [src/sensors/bno086/shtp.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L42)

Host->sensor frames must fit one MCU transmit slot (ERRATA E2: 2 x 64 B).

# Variable: METADATA\_RECORDS

```ts
const METADATA_RECORDS: Record<number, number>;
```

Defined in: [src/sensors/bno086/sh2.ts:369](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L369)

Per-sensor metadata FRS record IDs (subset used by getMetadata()).

# Variable: ME\_CAL\_GET

```ts
const ME_CAL_GET: 1 = 0x01;
```

Defined in: [src/sensors/bno086/sh2.ts:335](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/sh2.ts#L335)

Subcommand: report current ME calibration config.

# Variable: MIN\_COUNT\_RATE\_RTN\_LIMIT\_MCPS

```ts
const MIN_COUNT_RATE_RTN_LIMIT_MCPS: 102 = 0x0066;
```

Defined in: [src/sensors/vl53l4/uld.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L37)

# Variable: MIN\_RANGING\_FREQUENCY\_HZ

```ts
const MIN_RANGING_FREQUENCY_HZ: 2 = 2;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L43)

Below this the sensor never streams (contract 04).

# Variable: MI\_CFG\_DEV\_IDX

```ts
const MI_CFG_DEV_IDX: 49068 = 0xbfac;
```

Defined in: [src/sensors/vl53l8/cnh.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/cnh.ts#L28)

VL53LMZ_MI_CFG_DEV_IDX (cnh_send_config target).

# Variable: MODEL\_ID\_VL53L4CD

```ts
const MODEL_ID_VL53L4CD: 60330 = 0xebaa;
```

Defined in: [src/sensors/vl53l4/uld.ts:53](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L53)

# Variable: NB\_THRESHOLDS

```ts
const NB_THRESHOLDS: 64 = 64;
```

Defined in: [src/sensors/vl53l8/uld.ts:112](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L112)

# Variable: NO\_MATCH

```ts
const NO_MATCH: unique symbol;
```

Defined in: [src/device/device.ts:78](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/device/device.ts#L78)

Returned by a matcher to decline a packet.

# Variable: NUM\_CHANNELS

```ts
const NUM_CHANNELS: 6 = 6;
```

Defined in: [src/sensors/bno086/shtp.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L39)

# Variable: OSC\_FREQUENCY

```ts
const OSC_FREQUENCY: 6 = 0x0006;
```

Defined in: [src/sensors/vl53l4/uld.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L23)

Unnamed in the C driver.

# Variable: OUTER\_OFFSET\_MM

```ts
const OUTER_OFFSET_MM: 34 = 0x0022;
```

Defined in: [src/sensors/vl53l4/uld.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L30)

# Variable: POWER\_MODE\_DEEP\_SLEEP

```ts
const POWER_MODE_DEEP_SLEEP: 2 = 2;
```

Defined in: [src/sensors/vl53l8/uld.ts:118](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L118)

# Variable: POWER\_MODE\_SLEEP

```ts
const POWER_MODE_SLEEP: 0 = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:116](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L116)

# Variable: POWER\_MODE\_WAKEUP

```ts
const POWER_MODE_WAKEUP: 1 = 1;
```

Defined in: [src/sensors/vl53l8/uld.ts:117](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L117)

# Variable: Q\_POINTS

```ts
const Q_POINTS: Record<number, number>;
```

Defined in: [src/sensors/bno086/reports.ts:64](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L64)

Q point of the primary fields (value = raw / 2**Q); see module docs.

# Variable: RANGE\_CONFIG\_A

```ts
const RANGE_CONFIG_A: 94 = 0x005e;
```

Defined in: [src/sensors/vl53l4/uld.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L34)

# Variable: RANGE\_CONFIG\_B

```ts
const RANGE_CONFIG_B: 97 = 0x0061;
```

Defined in: [src/sensors/vl53l4/uld.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L35)

# Variable: RANGE\_CONFIG\_\_SIGMA\_THRESH

```ts
const RANGE_CONFIG__SIGMA_THRESH: 100 = 0x0064;
```

Defined in: [src/sensors/vl53l4/uld.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L36)

# Variable: RANGE\_OFFSET\_MM

```ts
const RANGE_OFFSET_MM: 30 = 0x001e;
```

Defined in: [src/sensors/vl53l4/uld.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L28)

# Variable: RANGE\_STATUS\_NAMES

```ts
const RANGE_STATUS_NAMES: Readonly<Record<number, string>>;
```

Defined in: [src/sensors/vl53l4/uld.ts:110](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L110)

UM2931, "Range status description".

# Variable: RANGING\_MODE\_AUTONOMOUS

```ts
const RANGING_MODE_AUTONOMOUS: 3 = 3;
```

Defined in: [src/sensors/vl53l8/uld.ts:38](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L38)

# Variable: RANGING\_MODE\_CONTINUOUS

```ts
const RANGING_MODE_CONTINUOUS: 1 = 1;
```

Defined in: [src/sensors/vl53l8/uld.ts:37](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L37)

# Variable: RATE\_HIGH\_FACTOR

```ts
const RATE_HIGH_FACTOR: 2.1 = 2.1;
```

Defined in: [src/sensors/bno086/bno086.ts:68](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L68)

# Variable: RATE\_LOW\_FACTOR

```ts
const RATE_LOW_FACTOR: 0.9 = 0.9;
```

Defined in: [src/sensors/bno086/bno086.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/bno086.ts#L67)

# Variable: READ\_MAX\_LEN

```ts
const READ_MAX_LEN: 2295 = 2295;
```

Defined in: [src/protocol/vl53l8.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L23)

# Variable: RECORDING\_SCHEMA

```ts
const RECORDING_SCHEMA: "depz.rec/1" = "depz.rec/1";
```

Defined in: [src/transport/replay.ts:9](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/replay.ts#L9)

# Variable: REPORT\_LENGTHS

```ts
const REPORT_LENGTHS: Record<number, number>;
```

Defined in: [src/sensors/bno086/reports.ts:94](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L94)

Total report length on the wire, 4-byte SH-2 header included
(sh2 reference driver report-length table).

# Variable: RESOLUTION\_4X4

```ts
const RESOLUTION_4X4: 16 = 16;
```

Defined in: [src/sensors/vl53l8/uld.ts:34](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L34)

# Variable: RESOLUTION\_8X8

```ts
const RESOLUTION_8X8: 64 = 64;
```

Defined in: [src/sensors/vl53l8/uld.ts:35](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L35)

# Variable: RESULT\_BLOCK\_ADDR

```ts
const RESULT_BLOCK_ADDR: 137 = RESULT__RANGE_STATUS;
```

Defined in: [src/sensors/vl53l4/uld.ts:92](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L92)

# Variable: RESULT\_BLOCK\_LEN

```ts
const RESULT_BLOCK_LEN: 17 = 17;
```

Defined in: [src/sensors/vl53l4/uld.ts:93](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L93)

# Variable: RESULT\_\_AMBIENT\_RATE

```ts
const RESULT__AMBIENT_RATE: 144 = 0x0090;
```

Defined in: [src/sensors/vl53l4/uld.ts:46](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L46)

# Variable: RESULT\_\_DISTANCE

```ts
const RESULT__DISTANCE: 150 = 0x0096;
```

Defined in: [src/sensors/vl53l4/uld.ts:48](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L48)

# Variable: RESULT\_\_OSC\_CALIBRATE\_VAL

```ts
const RESULT__OSC_CALIBRATE_VAL: 222 = 0x00de;
```

Defined in: [src/sensors/vl53l4/uld.ts:49](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L49)

# Variable: RESULT\_\_RANGE\_STATUS

```ts
const RESULT__RANGE_STATUS: 137 = 0x0089;
```

Defined in: [src/sensors/vl53l4/uld.ts:43](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L43)

# Variable: RESULT\_\_SIGMA

```ts
const RESULT__SIGMA: 146 = 0x0092;
```

Defined in: [src/sensors/vl53l4/uld.ts:47](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L47)

# Variable: RESULT\_\_SIGNAL\_RATE

```ts
const RESULT__SIGNAL_RATE: 142 = 0x008e;
```

Defined in: [src/sensors/vl53l4/uld.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L45)

# Variable: RESULT\_\_SPAD\_NB

```ts
const RESULT__SPAD_NB: 140 = 0x008c;
```

Defined in: [src/sensors/vl53l4/uld.ts:44](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L44)

# Variable: RV\_ACCURACY\_Q

```ts
const RV_ACCURACY_Q: 12 = 12;
```

Defined in: [src/sensors/bno086/reports.ts:87](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L87)

# Variable: SAMPLE\_PERIOD\_DEFAULT\_US

```ts
const SAMPLE_PERIOD_DEFAULT_US: 50000 = 50_000;
```

Defined in: [src/protocol/sr04.ts:22](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/sr04.ts#L22)

# Variable: SHTP\_HEADER\_SIZE

```ts
const SHTP_HEADER_SIZE: 4 = 4;
```

Defined in: [src/sensors/bno086/shtp.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/shtp.ts#L36)

# Variable: SOFT\_RESET

```ts
const SOFT_RESET: 0 = 0x0000;
```

Defined in: [src/sensors/vl53l4/uld.ts:20](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L20)

# Variable: STABILITY\_NAMES

```ts
const STABILITY_NAMES: Record<number, string>;
```

Defined in: [src/sensors/bno086/reports.ts:133](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L133)

# Variable: STATUS\_RTN

```ts
const STATUS_RTN: readonly number[];
```

Defined in: [src/sensors/vl53l4/uld.ts:103](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L103)

GetResult() raw status → ULD status (status_rtn[24] in VL53L4CD_api.c).

# Variable: STREAM\_CHUNK\_MAX

```ts
const STREAM_CHUNK_MAX: 1528 = 1528;
```

Defined in: [src/protocol/vl53l8.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L26)

Bytes of frame data per RPT_VL53_FRAME chunk.

# Variable: STREAM\_TOTAL\_MAX

```ts
const STREAM_TOTAL_MAX: 8192 = 8192;
```

Defined in: [src/protocol/vl53l8.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l8.ts#L28)

Max frame_size accepted by START_STREAM.

# Variable: SYSTEM\_START

```ts
const SYSTEM_START: 135 = 0x0087;
```

Defined in: [src/sensors/vl53l4/uld.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L42)

# Variable: SYSTEM\_\_INTERRUPT

```ts
const SYSTEM__INTERRUPT: 70 = 0x0046;
```

Defined in: [src/sensors/vl53l4/uld.ts:33](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L33)

# Variable: SYSTEM\_\_INTERRUPT\_CLEAR

```ts
const SYSTEM__INTERRUPT_CLEAR: 134 = 0x0086;
```

Defined in: [src/sensors/vl53l4/uld.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L41)

# Variable: TARGET\_ORDER\_CLOSEST

```ts
const TARGET_ORDER_CLOSEST: 1 = 1;
```

Defined in: [src/sensors/vl53l8/uld.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L40)

# Variable: TARGET\_ORDER\_STRONGEST

```ts
const TARGET_ORDER_STRONGEST: 2 = 2;
```

Defined in: [src/sensors/vl53l8/uld.ts:41](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L41)

# Variable: THRESH\_HIGH

```ts
const THRESH_HIGH: 114 = 0x0072;
```

Defined in: [src/sensors/vl53l4/uld.ts:39](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L39)

# Variable: THRESH\_IN\_WINDOW

```ts
const THRESH_IN_WINDOW: 0 = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:140](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L140)

# Variable: THRESH\_LOW

```ts
const THRESH_LOW: 116 = 0x0074;
```

Defined in: [src/sensors/vl53l4/uld.ts:40](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L40)

# Variable: THRESH\_OP\_AND

```ts
const THRESH_OP_AND: 2 = 2;
```

Defined in: [src/sensors/vl53l8/uld.ts:149](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L149)

# Variable: THRESH\_OP\_NONE

```ts
const THRESH_OP_NONE: 0 = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:147](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L147)

# Variable: THRESH\_OP\_OR

```ts
const THRESH_OP_OR: 0 = 0;
```

Defined in: [src/sensors/vl53l8/uld.ts:148](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L148)

# Variable: THRESH\_OUT\_OF\_WINDOW

```ts
const THRESH_OUT_OF_WINDOW: 1 = 1;
```

Defined in: [src/sensors/vl53l8/uld.ts:141](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/uld.ts#L141)

# Variable: TIMESTAMP\_REBASE

```ts
const TIMESTAMP_REBASE: 250 = 0xfa;
```

Defined in: [src/sensors/bno086/reports.ts:61](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/bno086/reports.ts#L61)

# Variable: TX\_SLOTS

```ts
const TX_SLOTS: 2 = 2;
```

Defined in: [src/protocol/bno086.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L28)

# Variable: TX\_SLOT\_SIZE

```ts
const TX_SLOT_SIZE: 64 = 64;
```

Defined in: [src/protocol/bno086.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/bno086.ts#L29)

# Variable: ULD\_VERSION

```ts
const ULD_VERSION: readonly [number, number, number, number];
```

Defined in: [src/sensors/vl53l4/uld.ts:17](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L17)

VL53L4CD ULD driver (ST STSW-IMG026 2.2.3) over the register bridge.

Port of `VL53L4CD_api.c` + `VL53L4CD_calibration.c` — a 1:1 async mirror of
the Python reference `depz_sensor_sdk.vl53l4.uld` (itself absorbed from the
firmware repo's hardware-proven port). The MCU owns nothing but the I2C
bus, XSHUT, INT and one streaming FSM — every register sequence below goes
over VL53_READ_REG / VL53_WRITE_REG (contracts/10_SENSOR_VL53L4.md §1).

Register sequences are a faithful port of the C code, integer widths and
32-bit truncations included; do not "simplify" them. Pure codec/math pieces
(`parseResultBlock`, `rangeTimingRegisters`, `decodeRangeTiming`,
`configBlock`, the tuning codecs) are module functions so the golden
vectors can hold them to byte-exact parity with the other SDKs.

# Variable: UNSOLICITED

```ts
const UNSOLICITED: 0 = 0x00;
```

Defined in: [src/protocol/common.ts:63](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/common.ts#L63)

Value of the echoed-cmd byte in unsolicited reports.

# Variable: VHV\_CONFIG\_\_TIMEOUT\_MACROP\_LOOP\_BOUND

```ts
const VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND: 8 = 0x0008;
```

Defined in: [src/sensors/vl53l4/uld.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L24)

# Variable: VL53L4\_I2C\_ERROR\_NAMES

```ts
const VL53L4_I2C_ERROR_NAMES: Readonly<Record<number, string>>;
```

Defined in: [src/protocol/vl53l4.ts:97](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L97)

last_i2c_error values in RPT_VL53_INFO.

# Variable: VL53L4\_I2C\_KHZ\_STEPS

```ts
const VL53L4_I2C_KHZ_STEPS: readonly number[];
```

Defined in: [src/protocol/vl53l4.ts:42](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L42)

Nominal SCL steps the firmware carries a TIMINGR for (VL53_SET_I2C_SPEED
clamps to the nearest one).

# Variable: VL53L4\_SF\_INT\_ACT\_HIGH

```ts
const VL53L4_SF_INT_ACT_HIGH: 2 = 0x02;
```

Defined in: [src/protocol/vl53l4.ts:36](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L36)

VL53_START_STREAM flags: interrupt polarity, mirroring bit 4 of
GPIO_HV_MUX__CTRL (0x0030). Clear (default): INT active low.

# Variable: VL53L4\_XFER\_MAX

```ts
const VL53L4_XFER_MAX: 253 = 253;
```

Defined in: [src/protocol/vl53l4.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L24)

# Variable: VL53L4\_XSHUT\_OFF

```ts
const VL53L4_XSHUT_OFF: 0 = 0;
```

Defined in: [src/protocol/vl53l4.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L27)

# Variable: VL53L4\_XSHUT\_ON

```ts
const VL53L4_XSHUT_ON: 1 = 1;
```

Defined in: [src/protocol/vl53l4.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L28)

# Variable: VL53L4\_XSHUT\_RESET

```ts
const VL53L4_XSHUT_RESET: 2 = 2;
```

Defined in: [src/protocol/vl53l4.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/protocol/vl53l4.ts#L30)

Blocking on the MCU (~3 ms); answered after the boot handshake.

# Variable: Vl53l8

```ts
const Vl53l8: typeof Vl53l8Cx = Vl53l8Cx;
```

Defined in: [src/sensors/vl53l8/vl53l8.ts:521](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l8/vl53l8.ts#L521)

Backward-compatible alias: the old flat `Vl53l8` name maps to the CX base
class (the historic default). New code should pick Vl53l8Cx / Vl53l8Ch.

# Variable: WINDOW\_ABOVE

```ts
const WINDOW_ABOVE: 1 = 1;
```

Defined in: [src/sensors/vl53l4/uld.ts:57](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L57)

# Variable: WINDOW\_BELOW

```ts
const WINDOW_BELOW: 0 = 0;
```

Defined in: [src/sensors/vl53l4/uld.ts:56](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L56)

# Variable: WINDOW\_IN

```ts
const WINDOW_IN: 3 = 3;
```

Defined in: [src/sensors/vl53l4/uld.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L59)

# Variable: WINDOW\_OUT

```ts
const WINDOW_OUT: 2 = 2;
```

Defined in: [src/sensors/vl53l4/uld.ts:58](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L58)

# Variable: XTALK\_PLANE\_OFFSET\_KCPS

```ts
const XTALK_PLANE_OFFSET_KCPS: 22 = 0x0016;
```

Defined in: [src/sensors/vl53l4/uld.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L25)

# Variable: XTALK\_X\_PLANE\_GRADIENT\_KCPS

```ts
const XTALK_X_PLANE_GRADIENT_KCPS: 24 = 0x0018;
```

Defined in: [src/sensors/vl53l4/uld.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L26)

# Variable: XTALK\_Y\_PLANE\_GRADIENT\_KCPS

```ts
const XTALK_Y_PLANE_GRADIENT_KCPS: 26 = 0x001a;
```

Defined in: [src/sensors/vl53l4/uld.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/sensors/vl53l4/uld.ts#L27)

# transport/node

## Classes

- [NodeSerialTransport](classes/NodeSerialTransport.md)

## Functions

- [listSerialPorts](functions/listSerialPorts.md)
- [listDepzDevices](functions/listDepzDevices.md)
- [openDevice](functions/openDevice.md)

# Class: NodeSerialTransport

Defined in: [src/transport/node.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/node.ts#L59)

## Implements

- [`SerialTransport`](../../../index/interfaces/SerialTransport.md)

## Constructors

### Constructor

```ts
new NodeSerialTransport(path, serialNumber?): NodeSerialTransport;
```

Defined in: [src/transport/node.ts:67](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/node.ts#L67)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `path` | `string` |
| `serialNumber?` | `string` |

#### Returns

`NodeSerialTransport`

## Properties

### info

```ts
readonly info: SerialTransportInfo;
```

Defined in: [src/transport/node.ts:60](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/node.ts#L60)

#### Implementation of

[`SerialTransport`](../../../index/interfaces/SerialTransport.md).[`info`](../../../index/interfaces/SerialTransport.md#info)

## Methods

### open()

```ts
open(opts?): Promise<void>;
```

Defined in: [src/transport/node.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/node.ts#L71)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `opts?` | \{ `baudRate?`: `number`; \} |
| `opts.baudRate?` | `number` |

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../../../index/interfaces/SerialTransport.md).[`open`](../../../index/interfaces/SerialTransport.md#open)

***

### write()

```ts
write(data): Promise<void>;
```

Defined in: [src/transport/node.ts:99](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/node.ts#L99)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../../../index/interfaces/SerialTransport.md).[`write`](../../../index/interfaces/SerialTransport.md#write)

***

### readable()

```ts
readable(): AsyncIterableIterator<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/transport/node.ts:107](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/node.ts#L107)

Raw chunks as they arrive; ends on close/disconnect.

#### Returns

`AsyncIterableIterator`\<`Uint8Array`\<`ArrayBufferLike`\>\>

#### Implementation of

[`SerialTransport`](../../../index/interfaces/SerialTransport.md).[`readable`](../../../index/interfaces/SerialTransport.md#readable)

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/transport/node.ts:118](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/node.ts#L118)

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../../../index/interfaces/SerialTransport.md).[`close`](../../../index/interfaces/SerialTransport.md#close)

***

### onDisconnect()

```ts
onDisconnect(cb): () => void;
```

Defined in: [src/transport/node.ts:126](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/node.ts#L126)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | () => `void` |

#### Returns

() => `void`

#### Implementation of

[`SerialTransport`](../../../index/interfaces/SerialTransport.md).[`onDisconnect`](../../../index/interfaces/SerialTransport.md#ondisconnect)

# Function: listDepzDevices()

```ts
function listDepzDevices(opts?): Promise<DeviceInfo[]>;
```

Defined in: [src/transport/node.ts:160](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/node.ts#L160)

Enumerate DEPZ devices on the system's serial ports (Node only).

With `matchUsb` (default) only ports carrying a known DEPZ (vid,pid) are
probed — fast, and it never pokes unrelated ports. Set `matchUsb: false`
for a legacy probe-everything scan.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `opts` | [`ListOptions`](../../../index/interfaces/ListOptions.md) |

## Returns

`Promise`\<[`DeviceInfo`](../../../index/interfaces/DeviceInfo.md)[]\>

# Function: listSerialPorts()

```ts
function listSerialPorts(): Promise<{
  path: string;
  serialNumber?: string;
  vendorId?: string;
  productId?: string;
}[]>;
```

Defined in: [src/transport/node.ts:52](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/node.ts#L52)

Enumerate system serial ports (Node only).

## Returns

`Promise`\<\{
  `path`: `string`;
  `serialNumber?`: `string`;
  `vendorId?`: `string`;
  `productId?`: `string`;
\}[]\>

# Function: openDevice()

```ts
function openDevice(target?, opts?): Promise<DepzDevice>;
```

Defined in: [src/transport/node.ts:173](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/node.ts#L173)

Open the right sensor class for `target` over `serialport` (Node only).

`target` is a port path (string), a candidate index (number, sorted by USB
iSerial), a `DeviceInfo` from `listDepzDevices()`, or omitted for the
lowest-serial candidate. Throws `NoDepzDeviceError` when nothing matches —
it never grabs an arbitrary system port. See [openDeviceFrom](../../../index/functions/openDeviceFrom.md).

## Parameters

| Parameter | Type |
| ------ | ------ |
| `target?` | [`OpenTarget`](../../../index/type-aliases/OpenTarget.md) |
| `opts?` | [`OpenDeviceOptions`](../../../index/interfaces/OpenDeviceOptions.md) |

## Returns

`Promise`\<[`DepzDevice`](../../../index/classes/DepzDevice.md)\>

# transport/webserial

## Classes

- [WebSerialTransport](classes/WebSerialTransport.md)

## Interfaces

- [WebSerialPortLike](interfaces/WebSerialPortLike.md)

## Functions

- [isWebSerialSupported](functions/isWebSerialSupported.md)
- [getGrantedPorts](functions/getGrantedPorts.md)
- [watchConnect](functions/watchConnect.md)
- [listDepzDevices](functions/listDepzDevices.md)
- [openDevice](functions/openDevice.md)

# Class: WebSerialTransport

Defined in: [src/transport/webserial.ts:70](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L70)

## Implements

- [`SerialTransport`](../../../index/interfaces/SerialTransport.md)

## Constructors

### Constructor

```ts
new WebSerialTransport(port): WebSerialTransport;
```

Defined in: [src/transport/webserial.ts:77](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L77)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `port` | [`WebSerialPortLike`](../interfaces/WebSerialPortLike.md) |

#### Returns

`WebSerialTransport`

## Properties

### info

```ts
readonly info: SerialTransportInfo;
```

Defined in: [src/transport/webserial.ts:71](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L71)

#### Implementation of

[`SerialTransport`](../../../index/interfaces/SerialTransport.md).[`info`](../../../index/interfaces/SerialTransport.md#info)

## Methods

### open()

```ts
open(opts?): Promise<void>;
```

Defined in: [src/transport/webserial.ts:82](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L82)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `opts?` | \{ `baudRate?`: `number`; \} |
| `opts.baudRate?` | `number` |

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../../../index/interfaces/SerialTransport.md).[`open`](../../../index/interfaces/SerialTransport.md#open)

***

### write()

```ts
write(data): Promise<void>;
```

Defined in: [src/transport/webserial.ts:90](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L90)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `data` | `Uint8Array` |

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../../../index/interfaces/SerialTransport.md).[`write`](../../../index/interfaces/SerialTransport.md#write)

***

### readable()

```ts
readable(): AsyncIterableIterator<Uint8Array<ArrayBufferLike>>;
```

Defined in: [src/transport/webserial.ts:96](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L96)

Raw chunks as they arrive; ends on close/disconnect.

#### Returns

`AsyncIterableIterator`\<`Uint8Array`\<`ArrayBufferLike`\>\>

#### Implementation of

[`SerialTransport`](../../../index/interfaces/SerialTransport.md).[`readable`](../../../index/interfaces/SerialTransport.md#readable)

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/transport/webserial.ts:115](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L115)

#### Returns

`Promise`\<`void`\>

#### Implementation of

[`SerialTransport`](../../../index/interfaces/SerialTransport.md).[`close`](../../../index/interfaces/SerialTransport.md#close)

***

### onDisconnect()

```ts
onDisconnect(cb): () => void;
```

Defined in: [src/transport/webserial.ts:136](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L136)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | () => `void` |

#### Returns

() => `void`

#### Implementation of

[`SerialTransport`](../../../index/interfaces/SerialTransport.md).[`onDisconnect`](../../../index/interfaces/SerialTransport.md#ondisconnect)

# Function: getGrantedPorts()

```ts
function getGrantedPorts(): Promise<WebSerialPortLike[]>;
```

Defined in: [src/transport/webserial.ts:50](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L50)

Previously-granted ports (survive page reloads per origin+device).

## Returns

`Promise`\<[`WebSerialPortLike`](../interfaces/WebSerialPortLike.md)[]\>

# Function: isWebSerialSupported()

```ts
function isWebSerialSupported(): boolean;
```

Defined in: [src/transport/webserial.ts:45](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L45)

Feature-detect Web Serial support in the current environment.

## Returns

`boolean`

# Function: listDepzDevices()

```ts
function listDepzDevices(opts?): Promise<DeviceInfo[]>;
```

Defined in: [src/transport/webserial.ts:189](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L189)

Probe the already-granted WebSerial ports and return the DEPZ devices among
them, ordered by DEVICE serial (GET_SERIAL) — the USB iSerial is unavailable
in the browser, so the serial is read over the protocol. By default every
granted port is probed (`matchUsb: false`) since a granted port is
user-curated; pass `matchUsb: true` to probe only known-DEPZ vid/pids.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `opts` | [`ListOptions`](../../../index/interfaces/ListOptions.md) |

## Returns

`Promise`\<[`DeviceInfo`](../../../index/interfaces/DeviceInfo.md)[]\>

# Function: openDevice()

```ts
function openDevice(target?, opts?): Promise<DepzDevice>;
```

Defined in: [src/transport/webserial.ts:210](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L210)

Open the right sensor class for a granted WebSerial port.

`target` is a `WebSerialPortLike` (typically fresh from
`navigator.serial.requestPort()`), a granted-port path string / `DeviceInfo`
from `listDepzDevices()`, a candidate index (number, ordered by device
serial), or omitted for the smallest-serial DEPZ candidate. `opts.serial`
selects the candidate whose device serial matches exactly. Selection probes
the granted ports; explicit targets open directly. Throws
`NoDepzDeviceError` when nothing matches.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `target?` | \| [`OpenTarget`](../../../index/type-aliases/OpenTarget.md) \| [`WebSerialPortLike`](../interfaces/WebSerialPortLike.md) |
| `opts?` | [`OpenDeviceOptions`](../../../index/interfaces/OpenDeviceOptions.md) |

## Returns

`Promise`\<[`DepzDevice`](../../../index/classes/DepzDevice.md)\>

# Function: watchConnect()

```ts
function watchConnect(cb): () => void;
```

Defined in: [src/transport/webserial.ts:59](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L59)

Watch physical connect events; used by the firmware-update flow to
re-acquire a device after it reboots. Returns an unsubscribe function.

## Parameters

| Parameter | Type |
| ------ | ------ |
| `cb` | (`port`) => `void` |

## Returns

() => `void`

# Interface: WebSerialPortLike

Defined in: [src/transport/webserial.ts:23](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L23)

## Properties

### readable

```ts
readable: ReadableStream<Uint8Array<ArrayBufferLike>> | null;
```

Defined in: [src/transport/webserial.ts:27](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L27)

***

### writable

```ts
writable: WritableStream<Uint8Array<ArrayBufferLike>> | null;
```

Defined in: [src/transport/webserial.ts:28](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L28)

## Methods

### open()

```ts
open(options): Promise<void>;
```

Defined in: [src/transport/webserial.ts:24](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L24)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `options` | \{ `baudRate`: `number`; \} |
| `options.baudRate` | `number` |

#### Returns

`Promise`\<`void`\>

***

### close()

```ts
close(): Promise<void>;
```

Defined in: [src/transport/webserial.ts:25](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L25)

#### Returns

`Promise`\<`void`\>

***

### getInfo()

```ts
getInfo(): {
  usbVendorId?: number;
  usbProductId?: number;
};
```

Defined in: [src/transport/webserial.ts:26](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L26)

#### Returns

```ts
{
  usbVendorId?: number;
  usbProductId?: number;
}
```

##### usbVendorId?

```ts
optional usbVendorId?: number;
```

##### usbProductId?

```ts
optional usbProductId?: number;
```

***

### addEventListener()?

```ts
optional addEventListener(type, cb): void;
```

Defined in: [src/transport/webserial.ts:29](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L29)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `type` | `"disconnect"` |
| `cb` | () => `void` |

#### Returns

`void`

***

### removeEventListener()?

```ts
optional removeEventListener(type, cb): void;
```

Defined in: [src/transport/webserial.ts:30](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-sdk-ts/src/transport/webserial.ts#L30)

#### Parameters

| Parameter | Type |
| ------ | ------ |
| `type` | `"disconnect"` |
| `cb` | () => `void` |

#### Returns

`void`

