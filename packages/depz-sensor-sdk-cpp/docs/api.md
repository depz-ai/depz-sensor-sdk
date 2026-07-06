# API reference

Auto-generated from the public headers under `include/depz/` (their
declarations and `//` doc-comments) by `scripts/gen_api_md.py` — run
`cmake --build build --target docs` (or `python3 scripts/gen_api_md.py`)
to regenerate. Edit the doc-comments in the headers, not this file.

Each sensor also has a focused reference with just its own symbols:
[SR04](sr04/api.md) · [VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · [BNO086](bno086/api.md).

This SDK is the *decode layer* — pure codecs, no I/O. Everything here
takes bytes and returns typed values; opening ports and streaming is
left to the host application.

## Contents

- **Transport (framing & CRC)**: [`HEADER_SIZE`](#header_size), [`MAX_PAYLOAD`](#max_payload), [`MAGIC0`](#magic0), [`MAGIC1`](#magic1), [`CrcType`](#crctype), [`payload_crc_bytes`](#payload_crc_bytes), [`build_packet`](#build_packet), [`Packet`](#packet), [`Trash`](#trash), [`CrcError`](#crcerror), [`ParserEvent`](#parserevent), [`PacketParser`](#packetparser), [`crc8_maxim`](#crc8_maxim), [`crc16_modbus`](#crc16_modbus), [`crc32_iso_hdlc`](#crc32_iso_hdlc), [`crc16_ccitt_false`](#crc16_ccitt_false)
- **Common protocol**: [`Cmd`](#cmd), [`Rpt`](#rpt), [`Status`](#status), [`SyncPinMode`](#syncpinmode), [`SyncPinPolarity`](#syncpinpolarity), [`UNSOLICITED`](#unsolicited), [`StatusReport`](#statusreport), [`TextReport`](#textreport), [`SyncTimeReport`](#synctimereport), [`TemperatureReport`](#temperaturereport), [`SequenceErrorReport`](#sequenceerrorreport), [`SyncPinConfig`](#syncpinconfig), [`pack_sync_time`](#pack_sync_time), [`pack_set_payload_crc_type`](#pack_set_payload_crc_type), [`sync_time_offset_rtt`](#sync_time_offset_rtt), [`strip_device_string`](#strip_device_string)
- **Identity**: [`SensorType`](#sensortype), [`to_string`](#to_string), [`DeviceMode`](#devicemode), [`Identity`](#identity), [`parse_software_name`](#parse_software_name)
- **USB identity**: [`DEPZ_USB_VID`](#depz_usb_vid), [`PID_SR04`](#pid_sr04), [`PID_VL53L8`](#pid_vl53l8), [`PID_BNO086`](#pid_bno086), [`DEV_USB_VID`](#dev_usb_vid), [`DEV_USB_PID`](#dev_usb_pid), [`DEPZ_PID_RANGE_LO`](#depz_pid_range_lo), [`DEPZ_PID_RANGE_HI`](#depz_pid_range_hi), [`is_known_depz_usb`](#is_known_depz_usb), [`usb_model_hint`](#usb_model_hint), [`PortInfo`](#portinfo), [`order_by_serial`](#order_by_serial)
- **SR04**: [`Sr04Cmd`](#sr04cmd), [`Sr04Rpt`](#sr04rpt), [`ECHO_TIMEOUT`](#echo_timeout), [`SAMPLE_PERIOD_DEFAULT_US`](#sample_period_default_us), [`ECHO_DECAY_DEFAULT_US`](#echo_decay_default_us), [`ECHO_DECAY_MIN_US`](#echo_decay_min_us), [`ECHO_DECAY_MAX_US`](#echo_decay_max_us), [`Sr04Data`](#sr04data), [`pack_sample_period`](#pack_sample_period), [`unpack_sample_period`](#unpack_sample_period), [`pack_echo_decay`](#pack_echo_decay), [`unpack_echo_decay`](#unpack_echo_decay), [`distance_mm_from_echo`](#distance_mm_from_echo)
- **VL53L8 (ToF)**: [`RESOLUTION_4X4`](#resolution_4x4), [`RESOLUTION_8X8`](#resolution_8x8), [`STREAM_CHUNK_MAX`](#stream_chunk_max), [`Variant`](#variant), [`FOOTER_ID_OFF_CX`](#footer_id_off_cx), [`FOOTER_ID_OFF_CH`](#footer_id_off_ch), [`footer_id_off`](#footer_id_off), [`FrameChunk`](#framechunk), [`FrameReassembler`](#framereassembler), [`Vl53l8Frame`](#vl53l8frame), [`swap_buffer`](#swap_buffer), [`decode_frame`](#decode_frame), [`xtalk_margin_raw`](#xtalk_margin_raw), [`xtalk_margin_kcps`](#xtalk_margin_kcps), [`ThreshMeasurement`](#threshmeasurement), [`DetectionThreshold`](#detectionthreshold), [`NB_THRESHOLDS`](#nb_thresholds), [`pack_detection_thresholds`](#pack_detection_thresholds), [`detection_thresholds_valid_status`](#detection_thresholds_valid_status), [`MotionConfig`](#motionconfig), [`motion_config_init`](#motion_config_init)
- **BNO086 (IMU)**: [`SHTP_HEADER_SIZE`](#shtp_header_size), [`LENGTH_MASK`](#length_mask), [`CONTINUATION_BIT`](#continuation_bit), [`NUM_CHANNELS`](#num_channels), [`MAX_TX_FRAME`](#max_tx_frame), [`ShtpChannel`](#shtpchannel), [`ShtpHeader`](#shtpheader), [`ShtpCargo`](#shtpcargo), [`shtp_build_frame`](#shtp_build_frame), [`shtp_fragment_cargo`](#shtp_fragment_cargo), [`ShtpLayer`](#shtplayer), [`sh2_build_set_feature`](#sh2_build_set_feature), [`sh2_build_get_feature_request`](#sh2_build_get_feature_request), [`sh2_build_product_id_request`](#sh2_build_product_id_request), [`sh2_build_command_request`](#sh2_build_command_request), [`sh2_build_frs_read_request`](#sh2_build_frs_read_request), [`sh2_build_frs_write_request`](#sh2_build_frs_write_request), [`sh2_build_frs_write_data`](#sh2_build_frs_write_data), [`BASE_TIMESTAMP_REF`](#base_timestamp_ref), [`TIMESTAMP_REBASE`](#timestamp_rebase), [`RV_ACCURACY_Q`](#rv_accuracy_q), [`GYRO_RV_ANGVEL_Q`](#gyro_rv_angvel_q), [`ReportType`](#reporttype), [`Report`](#report), [`parse_input_cargo`](#parse_input_cargo), [`parse_gyro_rv_cargo`](#parse_gyro_rv_cargo)
- **Bootloader / firmware**: [`FWDEPZ_HEADER_SIZE`](#fwdepz_header_size), [`FWDEPZ_MAGIC`](#fwdepz_magic), [`BlCmd`](#blcmd), [`BlRpt`](#blrpt), [`BlStatus`](#blstatus), [`FlashInfo`](#flashinfo), [`pack_write_page`](#pack_write_page), [`pack_read_page`](#pack_read_page), [`FwDepzErrorKind`](#fwdepzerrorkind), [`FwDepzError`](#fwdepzerror), [`FwDepzImage`](#fwdepzimage)
- **Datasets**: [`SCHEMA_PREFIX`](#schema_prefix), [`TimeSync`](#timesync), [`DeviceMeta`](#devicemeta), [`Record`](#record), [`Reader`](#reader)

## Transport (framing & CRC)

### HEADER_SIZE

```cpp
inline constexpr std::size_t HEADER_SIZE = 7;
```

### MAX_PAYLOAD

```cpp
inline constexpr std::uint16_t MAX_PAYLOAD = 0x3FFF;
```

### MAGIC0

```cpp
inline constexpr std::byte MAGIC0 = std::byte;
```

### MAGIC1

```cpp
inline constexpr std::byte MAGIC1 = std::byte;
```

### CrcType

```cpp
enum class CrcType : std::uint8_t {
    None = 0,
    Crc8 = 1,
    Crc16 = 2,
    Crc32 = 3,
};
```

### payload_crc_bytes

```cpp
bytes payload_crc_bytes(CrcType crc_type, byte_span payload);
```

CRC trailer for a payload; empty payloads never carry CRC bytes (ERRATA E6).

### build_packet

```cpp
bytes build_packet(std::uint8_t cmd, byte_span payload =;
```

Frame one packet. `crc_type` bits are set in the header even for an empty
payload (matching device TX), but CRC bytes are appended only for non-empty
payloads. Throws std::length_error if payload exceeds MAX_PAYLOAD.

### Packet

```cpp
struct Packet {
    std::uint8_t cmd;
    std::uint8_t seq;
    bytes payload;
};
```

### Trash

```cpp
struct Trash {
    bytes data;
};
```

Bytes discarded while hunting for a valid frame. Boundaries between
consecutive Trash events depend on read chunking; only the concatenated
byte stream is deterministic.

### CrcError

```cpp
struct CrcError {
    std::uint8_t cmd;
    std::uint8_t seq;
};
```

A frame with a valid header whose payload CRC failed; dropped.

### ParserEvent

```cpp
using ParserEvent = std::variant<Packet, Trash, CrcError>;
```

### PacketParser

```cpp
class PacketParser {
    std::size_t packets = 0;
    std::size_t crc_errors = 0;
    std::size_t header_errors = 0;
    std::size_t trash_bytes = 0;
};
```

Incremental frame parser. Feed arbitrary byte chunks; get events. Event
order is invariant to chunking (contract 01 §5) except Trash event
boundaries — concatenate Trash data when comparing streams.

#### PacketParser.feed

```cpp
std::vector<ParserEvent> feed(byte_span data);
```

#### PacketParser.residue

```cpp
byte_span residue() const noexcept;
```

Bytes still buffered (unconsumed residue), e.g. a partial frame.

### crc8_maxim

```cpp
std::uint8_t crc8_maxim(byte_span data) noexcept;
```

CRC-8/MAXIM: poly 0x31 reflected (0x8C), init 0x00, xorout 0x00.

### crc16_modbus

```cpp
std::uint16_t crc16_modbus(byte_span data) noexcept;
```

CRC-16/MODBUS: poly 0x8005 reflected (0xA001), init 0xFFFF, xorout 0x0000.

### crc32_iso_hdlc

```cpp
std::uint32_t crc32_iso_hdlc(byte_span data) noexcept;
```

CRC-32/ISO-HDLC: poly 0x04C11DB7 reflected (0xEDB88320), init/xorout 0xFFFFFFFF.

### crc16_ccitt_false

```cpp
std::uint16_t crc16_ccitt_false(byte_span data) noexcept;
```

CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, not reflected.
Used only for the `.fwdepz` file header (contract 06), never on the wire.

## Common protocol

### Cmd

```cpp
enum class Cmd : std::uint8_t {
    Bootloader = 0x01,
    DeviceReset = 0x02,
    GetDeviceName = 0x03,
    GetNameActiveSoftware = 0x04,
    GetSerial = 0x05,
    SyncTime = 0x06,
    GetMcuTemperature = 0x07,
    GetPayloadCrcType = 0x08,
    SetPayloadCrcType = 0x09,
    ThroughputTxStart = 0x1C,
    ThroughputTxStop = 0x1D,
    ThroughputRxData = 0x1E,
    GetSyncPinConfig = 0x30,
    SetSyncPinConfig = 0x31,
};
```

### Rpt

```cpp
enum class Rpt : std::uint8_t {
    Status = 0x80,
    Text = 0x81,
    SyncTime = 0x82,
    Temperature = 0x83,
    SequenceError = 0x84,
    PayloadCrcType = 0x87,
    ThroughputData = 0x88,
    SyncPinConfig = 0x90,
};
```

### Status

```cpp
enum class Status : std::uint8_t {
    Ok = 0x00,
    Error = 0x01,
    ErrInvalidCmd = 0x02,
    ErrPayloadFormat = 0x03,
    ErrInvalidParam = 0x04,
    ErrPayloadCrc = 0x05,
    ErrBusy = 0x06,
    ErrCmdNotSupported = 0x07,
    ErrNotInitialized = 0x08,
    ErrHardwareFault = 0x09,
};
```

### SyncPinMode

```cpp
enum class SyncPinMode : std::uint8_t {
    Disable = 0x00,
    In = 0x01,
    OutStart = 0x02,
    OutEnd = 0x03,
    OutBoth = 0x04,
};
```

### SyncPinPolarity

```cpp
enum class SyncPinPolarity : std::uint8_t {
    IdleLow = 0x00,
    IdleHigh = 0x01,
};
```

### UNSOLICITED

```cpp
inline constexpr std::uint8_t UNSOLICITED = 0x00;
```

Value of the echoed-cmd byte in unsolicited reports.

### StatusReport

```cpp
struct StatusReport {
    std::uint8_t cmd;  // echoed request opcode; 0x00 = unsolicited
    std::uint8_t status;
};
```

#### StatusReport.unpack

```cpp
static StatusReport unpack(byte_span payload);
```

### TextReport

```cpp
struct TextReport {
    std::uint8_t cmd;
    std::string text;
};
```

#### TextReport.unpack

```cpp
static TextReport unpack(byte_span payload);
```

### SyncTimeReport

```cpp
struct SyncTimeReport {
    std::uint64_t pc_timestamp_us;  // T1 echoed
    std::uint64_t mcu_rx_us;  // T2
    std::uint64_t mcu_tx_us;  // T3
};
```

#### SyncTimeReport.unpack

```cpp
static SyncTimeReport unpack(byte_span payload);
```

### TemperatureReport

```cpp
struct TemperatureReport {
    std::uint64_t timestamp_us;
    std::int16_t raw_decidegrees;  // units of 0.1 degC
};
```

#### TemperatureReport.celsius

```cpp
double celsius() const;
```

#### TemperatureReport.unpack

```cpp
static TemperatureReport unpack(byte_span payload);
```

### SequenceErrorReport

```cpp
struct SequenceErrorReport {
    std::uint8_t expected_seq;
    std::uint8_t received_seq;
};
```

#### SequenceErrorReport.unpack

```cpp
static SequenceErrorReport unpack(byte_span payload);
```

### SyncPinConfig

```cpp
struct SyncPinConfig {
    std::uint8_t pin;  // 1..5
    SyncPinMode mode;
    SyncPinPolarity polarity;
};
```

#### SyncPinConfig.pack

```cpp
bytes pack() const;
```

#### SyncPinConfig.unpack

```cpp
static SyncPinConfig unpack(byte_span payload);
```

### pack_sync_time

```cpp
bytes pack_sync_time(std::uint64_t pc_timestamp_us);
```

### pack_set_payload_crc_type

```cpp
bytes pack_set_payload_crc_type(std::uint8_t crc_type);
```

### sync_time_offset_rtt

```cpp
std::pair<std::int64_t, std::int64_t> sync_time_offset_rtt(std::int64_t t1, std::int64_t t2, std::int64_t t3, std::int64_t t4);
```

NTP-style clock math, all us (contract 02 §5). Returns {offset_us, rtt_us}
where offset = device_clock - host_clock, computed with truncation toward
zero: offset = ((T2-T1)+(T3-T4)) / 2, rtt = (T4-T1)-(T3-T2).

### strip_device_string

```cpp
std::string strip_device_string(byte_span raw);
```

Decode an ASCII device string, dropping trailing NUL/0xFF filler.

## Identity

### SensorType

```cpp
enum class SensorType {
    Sr04,
    Vl53l8,
    Bno086,
    Unknown,
};
```

### to_string

```cpp
std::string to_string(SensorType t);
```

String name for a SensorType (matches the vector encoding).

```cpp
std::string to_string(DeviceMode m);
```

### DeviceMode

```cpp
enum class DeviceMode {
    App,
    Bootloader,
    Unknown,
};
```

### Identity

```cpp
struct Identity {
    DeviceMode mode;
    std::optional<SensorType> sensor_type;  // nullopt in bootloader/unknown mode
    std::string software_name;
    std::string version;  // "" when not parseable
};
```

### parse_software_name

```cpp
Identity parse_software_name(const std::string& name);
```

Classify a GET_NAME_ACTIVE_SOFTWARE string. The string must already be
stripped of trailing NUL/0xFF (strip_device_string).

## USB identity

### DEPZ_USB_VID

```cpp
inline constexpr std::uint16_t DEPZ_USB_VID = 0x1BCF;
```

7119

### PID_SR04

```cpp
inline constexpr std::uint16_t PID_SR04 = 0xEC78;
```

60536

### PID_VL53L8

```cpp
inline constexpr std::uint16_t PID_VL53L8 = 0xED40;
```

60736

### PID_BNO086

```cpp
inline constexpr std::uint16_t PID_BNO086 = 0xEE08;
```

60936

### DEV_USB_VID

```cpp
inline constexpr std::uint16_t DEV_USB_VID = 0x0483;
```

Dev / unprogrammed default (STMicroelectronics) that dev units carry.

### DEV_USB_PID

```cpp
inline constexpr std::uint16_t DEV_USB_PID = 0x56DC;
```

22236

### DEPZ_PID_RANGE_LO

```cpp
inline constexpr std::uint16_t DEPZ_PID_RANGE_LO = 60536;
```

Inclusive reserved sensor PID block under DEPZ_USB_VID.

### DEPZ_PID_RANGE_HI

```cpp
inline constexpr std::uint16_t DEPZ_PID_RANGE_HI = 65535;
```

### is_known_depz_usb

```cpp
bool is_known_depz_usb(std::optional<std::uint16_t> vid, std::optional<std::uint16_t> pid) noexcept;
```

True when (vid, pid) is a recognized DEPZ (or dev-default) USB id.

### usb_model_hint

```cpp
std::optional<std::string> usb_model_hint(std::optional<std::uint16_t> vid, std::optional<std::uint16_t> pid);
```

Best-guess model name for a (vid, pid), or std::nullopt. Informational only.

### PortInfo

```cpp
struct PortInfo {
    std::string port;
    std::optional<std::string> usb_serial;
};
```

One enumerated serial port with its USB iSerial (used for ordering).

### order_by_serial

```cpp
std::vector<PortInfo> order_by_serial(std::vector<PortInfo> ports);
```

Order ports by USB serial ascending; None/empty serial sorts last, tie-break
by port path (contract 02 §4).

## SR04

### Sr04Cmd

```cpp
enum class Sr04Cmd : std::uint8_t {
    GetSamplePeriod = 0x32,
    SetSamplePeriod = 0x33,
    GetEchoDecay = 0x34,
    SetEchoDecay = 0x35,
    MeasureOnce = 0x36,
    StartMeasurementLoop = 0x37,
    StopMeasurementLoop = 0x38,
};
```

### Sr04Rpt

```cpp
enum class Sr04Rpt : std::uint8_t {
    Data = 0x91,
    SamplePeriod = 0x92,
    EchoDecay = 0x93,
};
```

### ECHO_TIMEOUT

```cpp
inline constexpr std::uint16_t ECHO_TIMEOUT = 0xFFFF;
```

echo_time_us sentinel: no echo received.

### SAMPLE_PERIOD_DEFAULT_US

```cpp
inline constexpr std::uint32_t SAMPLE_PERIOD_DEFAULT_US = 50000;
```

### ECHO_DECAY_DEFAULT_US

```cpp
inline constexpr std::uint16_t ECHO_DECAY_DEFAULT_US = 5000;
```

### ECHO_DECAY_MIN_US

```cpp
inline constexpr std::uint16_t ECHO_DECAY_MIN_US = 4000;
```

### ECHO_DECAY_MAX_US

```cpp
inline constexpr std::uint16_t ECHO_DECAY_MAX_US = 65000;
```

### Sr04Data

```cpp
struct Sr04Data {
    std::uint8_t source_cmd;  // 0x36 single shot (host or SYNC_IN), 0x37 loop sample
    std::uint64_t timestamp_us;
    std::uint16_t echo_time_us;
};
```

#### Sr04Data.unpack

```cpp
static Sr04Data unpack(byte_span payload);
```

### pack_sample_period

```cpp
bytes pack_sample_period(std::uint32_t period_us);
```

### unpack_sample_period

```cpp
std::uint32_t unpack_sample_period(byte_span payload);
```

### pack_echo_decay

```cpp
bytes pack_echo_decay(std::uint16_t decay_us);
```

### unpack_echo_decay

```cpp
std::uint16_t unpack_echo_decay(byte_span payload);
```

### distance_mm_from_echo

```cpp
std::optional<double> distance_mm_from_echo(std::uint16_t echo_time_us, std::optional<double> air_temp_c = std::nullopt);
```

Round-trip echo time -> distance in mm; nullopt for the timeout sentinel.
Default speed of sound 343 m/s; with air_temp_c uses c = 331.3 + 0.606*T.

## VL53L8 (ToF)

### RESOLUTION_4X4

```cpp
inline constexpr int RESOLUTION_4X4 = 16;
```

### RESOLUTION_8X8

```cpp
inline constexpr int RESOLUTION_8X8 = 64;
```

### STREAM_CHUNK_MAX

```cpp
inline constexpr std::size_t STREAM_CHUNK_MAX = 1528;
```

Max frame-data bytes carried in one RPT_VL53_FRAME chunk (contract 04).

### Variant

```cpp
enum class Variant {
    CX,  // base ToF (ULD 2.1.0); dev-default, no dedicated production USB PID
    CH,  // CX + CNH histograms; production USB PID 0xED40 (VL53LMZ 2.0.16)
};
```

The two DEPZ ToF die variants. Both stream the same results-frame layout
decoded by decode_frame(); they differ only in the footer-id offset here (the
CH additionally emits CNH histograms — a not-yet-decoded extension point, see
the CNH section at the bottom of this header).

### FOOTER_ID_OFF_CX

```cpp
inline constexpr int FOOTER_ID_OFF_CX = 12;
```

Footer-id offset used by decode_frame's header/footer match check, per
variant: 12 for VL53L8CX (ULD 2.1.0), 4 for VL53L8CH (VL53LMZ 2.0.16).

### FOOTER_ID_OFF_CH

```cpp
inline constexpr int FOOTER_ID_OFF_CH = 4;
```

### footer_id_off

```cpp
inline constexpr int footer_id_off(Variant v);
```

Footer-id offset for a variant (convenience for decode_frame's argument).

### FrameChunk

```cpp
struct FrameChunk {
    std::uint64_t timestamp_us = 0;
    std::uint16_t full_size = 0;
    std::uint16_t offset = 0;
    bytes data;
};
```

One RPT_VL53_FRAME payload: capture timestamp + this chunk's slice of the
full frame. Header is ts(u64) full_size(u16) offset(u16), then data.

#### FrameChunk.unpack

```cpp
static FrameChunk unpack(byte_span payload);
```

### FrameReassembler

```cpp
class FrameReassembler {
    std::size_t completed = 0;
    std::size_t discarded = 0;
};
```

Rebuilds full sensor frames from chunked RPT_VL53_FRAME reports. Rules
(contract 04): reset on offset==0; chunks must be contiguous — a gap discards
the frame in progress; a frame completes when the accumulated bytes equal
full_size.

#### FrameReassembler.feed

```cpp
std::optional<std::pair<std::uint64_t, bytes>> feed(const FrameChunk& chunk);
```

Returns (timestamp_us, frame_bytes) when a frame completes, else nullopt.

### Vl53l8Frame

```cpp
struct Vl53l8Frame {
    std::uint64_t timestamp_us = 0;
    int resolution = 0;
    int silicon_temp_degc = 0;
    std::vector<std::int32_t> distance_mm;
    std::vector<std::uint8_t> target_status;
    std::vector<std::uint8_t> nb_target_detected;
    std::vector<std::uint32_t> signal_per_spad;
    std::vector<std::uint32_t> ambient_per_spad;
    std::vector<std::uint32_t> nb_spads_enabled;
    std::vector<std::uint16_t> range_sigma_mm_raw;
    std::vector<std::uint8_t> reflectance;
};
```

One decoded ranging frame. Arrays are sized to the active resolution (16 or
64 zones), row-major. Raw integers per the wire; `distance_mm` already has
the ST /4 floor scaling applied, `range_sigma_mm_raw` is the raw u16 (real
value = raw / 128).

### swap_buffer

```cpp
bytes swap_buffer(byte_span data);
```

VL53L8CX_SwapBuffer: byte-reverse every 32-bit word (tail bytes unchanged).

### decode_frame

```cpp
std::optional<Vl53l8Frame> decode_frame(byte_span raw, int footer_id_off = FOOTER_ID_OFF_CX);
```

Decode one raw results frame (the full reassembled frame bytes). Shared by
both CX and CH — the layout is identical. Resolution is inferred from the
frame's block layout. Returns nullopt for a corrupted frame (header/footer id
mismatch). `footer_id_off` is variant-specific (FOOTER_ID_OFF_CX /
FOOTER_ID_OFF_CH, or footer_id_off(Variant)); it defaults to the CX offset.

### xtalk_margin_raw

```cpp
std::uint32_t xtalk_margin_raw(double margin_kcps);
```

Xtalk margin (kcps/SPAD) <-> DCI_XTALK_CFG raw word (raw = round(kcps*2048)).

### xtalk_margin_kcps

```cpp
double xtalk_margin_kcps(std::uint32_t raw);
```

### ThreshMeasurement

```cpp
enum ThreshMeasurement : std::uint8_t {
    THRESH_DIST_MM = 1,
    THRESH_SIGNAL_PER_SPAD_KCPS = 2,
    THRESH_RANGE_SIGMA_MM = 4,
    THRESH_AMBIENT_PER_SPAD_KCPS = 8,
    THRESH_NB_TARGET_DETECTED = 9,
    THRESH_TAR_STATUS = 12,
    THRESH_NB_SPADS_ENABLED = 13,
    THRESH_MOTION_INDICATOR = 19,
};
```

Detection-threshold measurement selectors (plugin source).

### DetectionThreshold

```cpp
struct DetectionThreshold {
    std::int32_t low_thresh = 0;  // real units; scaled on pack
    std::int32_t high_thresh = 0;  // real units; scaled on pack
    std::uint8_t measurement = 0;
    std::uint8_t type = 0;  // window selector
    std::uint8_t zone_num = 0;
    std::uint8_t operation = 0;  // combine op
};
```

### NB_THRESHOLDS

```cpp
inline constexpr int NB_THRESHOLDS = 64;
```

### pack_detection_thresholds

```cpp
bytes pack_detection_thresholds(const std::vector<DetectionThreshold>& thresholds);
```

Pack the 64-entry DCI_DET_THRESH_START block (768 bytes). Missing entries
default to zeros; low/high are multiplied by the measurement's scale factor.

### detection_thresholds_valid_status

```cpp
bytes detection_thresholds_valid_status();
```

The 8-byte DCI_DET_THRESH_VALID_STATUS write that accompanies the block.

### MotionConfig

```cpp
struct MotionConfig {
    std::int32_t ref_bin_offset = 0;
    std::uint32_t detection_threshold = 0;
    std::uint32_t extra_noise_sigma = 0;
    std::uint32_t null_den_clip_value = 0;
    std::uint8_t mem_update_mode = 0;
    std::uint8_t mem_update_choice = 0;
    std::uint8_t sum_span = 0;
    std::uint8_t feature_length = 0;
    std::uint8_t nb_of_aggregates = 0;
    std::uint8_t nb_of_temporal_accumulations = 0;
    std::uint8_t min_nb_for_global_detection = 0;
    std::uint8_t global_indicator_format_1 = 0;
    std::uint8_t global_indicator_format_2 = 0;
    std::uint8_t spare_1 = 0;
    std::uint8_t spare_2 = 0;
    std::uint8_t spare_3 = 0;
    std::int8_t map_id[64] = {0};
    std::uint8_t indicator_format_1[32] = {0};
    std::uint8_t indicator_format_2[32] = {0};
};
```

Mirror of VL53L8CX_Motion_Configuration (156 bytes, plugin source).

#### MotionConfig.pack

```cpp
bytes pack() const;
```

### motion_config_init

```cpp
MotionConfig motion_config_init(int resolution);
```

Build the default motion-indicator configuration for `resolution`
(RESOLUTION_4X4 or RESOLUTION_8X8), matching uld.motion_indicator_init.

## BNO086 (IMU)

### SHTP_HEADER_SIZE

```cpp
inline constexpr std::size_t SHTP_HEADER_SIZE = 4;
```

### LENGTH_MASK

```cpp
inline constexpr std::uint16_t LENGTH_MASK = 0x7FFF;
```

### CONTINUATION_BIT

```cpp
inline constexpr std::uint16_t CONTINUATION_BIT = 0x8000;
```

### NUM_CHANNELS

```cpp
inline constexpr int NUM_CHANNELS = 6;
```

### MAX_TX_FRAME

```cpp
inline constexpr std::size_t MAX_TX_FRAME = 64;
```

ERRATA E2 MCU slot

### ShtpChannel

```cpp
enum class ShtpChannel : std::uint8_t {
    Command = 0,
    Executable = 1,
    Control = 2,
    InputNormal = 3,
    InputWake = 4,
    GyroRv = 5,
};
```

### ShtpHeader

```cpp
struct ShtpHeader {
    std::uint16_t length = 0;  // bits 14:0 — cargo length incl. this 4-byte header
    std::uint8_t channel = 0;
    std::uint8_t seq = 0;
    bool continuation = false;
};
```

#### ShtpHeader.pack

```cpp
bytes pack() const;
```

#### ShtpHeader.unpack

```cpp
static ShtpHeader unpack(byte_span data);
```

### ShtpCargo

```cpp
struct ShtpCargo {
    int channel = 0;
    int seq = 0;  // seq of the first fragment
    bytes payload;
};
```

One reassembled cargo: `payload` excludes all SHTP headers.

### shtp_build_frame

```cpp
bytes shtp_build_frame(int channel, byte_span payload, std::uint8_t seq);
```

Single-fragment frame: length = header + payload.

### shtp_fragment_cargo

```cpp
std::vector<bytes> shtp_fragment_cargo(int channel, byte_span payload, std::uint8_t seq_start, std::size_t max_frame = MAX_TX_FRAME);
```

Split a cargo into wire frames of at most `max_frame` bytes. First fragment
advertises the TOTAL cargo length; continuations carry the remaining length
with the continuation bit set; seq increments per frame.

### ShtpLayer

```cpp
class ShtpLayer {
    std::size_t discarded = 0;
};
```

Per-channel TX sequence counters + RX cargo reassembly (§3).

#### ShtpLayer.next_frame

```cpp
bytes next_frame(int channel, byte_span payload);
```

Build a single-fragment frame, consuming the channel's TX seq.

#### ShtpLayer.tx_seq

```cpp
int tx_seq(int channel) const;
```

#### ShtpLayer.feed

```cpp
std::optional<ShtpCargo> feed(byte_span frame);
```

Consume one inbound frame; return the cargo when complete, else nullopt.

#### ShtpLayer.reset

```cpp
void reset();
```

Forget all TX seq counters and partial cargos (sensor reset).

### sh2_build_set_feature

```cpp
bytes sh2_build_set_feature(std::uint8_t sensor_id, std::uint32_t interval_us, std::uint32_t batch_us = 0, std::uint16_t sensitivity = 0, std::uint8_t flags = 0, std::uint32_t cfg_word = 0);
```

Set Feature Command (0xFD), 17 bytes. interval_us = 0 disables the sensor.

### sh2_build_get_feature_request

```cpp
bytes sh2_build_get_feature_request(std::uint8_t sensor_id);
```

Get Feature Request (0xFE), 2 bytes.

### sh2_build_product_id_request

```cpp
bytes sh2_build_product_id_request();
```

Product ID Request (0xF9), 2 bytes.

### sh2_build_command_request

```cpp
bytes sh2_build_command_request(std::uint8_t seq, std::uint8_t command, byte_span params =;
```

Command Request (0xF2), 12 bytes: id, seq, command, P0..P8 (params, <=9 bytes).

### sh2_build_frs_read_request

```cpp
bytes sh2_build_frs_read_request(std::uint16_t frs_type, std::uint16_t offset_words = 0, std::uint16_t block_words = 0);
```

FRS Read Request (0xF4), 8 bytes. block_words = 0 reads the record.

### sh2_build_frs_write_request

```cpp
bytes sh2_build_frs_write_request(std::uint16_t frs_type, std::uint16_t length_words);
```

FRS Write Request (0xF7), 6 bytes. length_words = 0 erases the record.

### sh2_build_frs_write_data

```cpp
bytes sh2_build_frs_write_data(std::uint16_t offset_words, const std::vector<std::uint32_t>& words);
```

FRS Write Data (0xF6), 12 bytes; 1 or 2 words per packet.

### BASE_TIMESTAMP_REF

```cpp
inline constexpr std::uint8_t BASE_TIMESTAMP_REF = 0xFB;
```

In-cargo control IDs on the input channels.

### TIMESTAMP_REBASE

```cpp
inline constexpr std::uint8_t TIMESTAMP_REBASE = 0xFA;
```

### RV_ACCURACY_Q

```cpp
inline constexpr int RV_ACCURACY_Q = 12;
```

### GYRO_RV_ANGVEL_Q

```cpp
inline constexpr int GYRO_RV_ANGVEL_Q = 10;
```

### ReportType

```cpp
enum class ReportType {
    Acceleration,
    Gyroscope,
    Magnetometer,
    UncalibratedGyroscope,
    UncalibratedMagnetometer,
    RotationVector,
    ScalarReport,
    TapDetector,
    StepCounter,
    StepDetector,
    SignificantMotion,
    StabilityClassifier,
    ShakeDetector,
    GenericEvent,
    PersonalActivityClassifier,
    RawSensor,
    GyroIntegratedRV,
    UnknownReport,
};
```

### Report

```cpp
struct Report {
    ReportType type = ReportType::UnknownReport;
    int sensor_id = 0;
    std::int64_t timestamp_us = 0;
    int seq = 0;
    int accuracy = 0;
    std::int64_t delay_us = 0;
    std::int64_t x_raw = 0, y_raw = 0, z_raw = 0;
    std::int64_t bias_x_raw = 0, bias_y_raw = 0, bias_z_raw = 0;
    std::int64_t i_raw = 0, j_raw = 0, k_raw = 0, real_raw = 0;
    bool has_accuracy_raw = false;
    std::int64_t accuracy_raw = 0;
    std::int64_t value_raw = 0;
    int flags = 0;
    std::int64_t latency_us = 0;
    int steps = 0;
    int motion = 0;
    int classification = 0;
    int page_number = 0;
    bool end_of_sequence = false;
    int most_likely_state = 0;
    std::vector<int> confidences;
    std::int64_t sensor_timestamp_us = 0;
    std::int64_t temperature_raw = 0;
    std::int64_t vx_raw = 0, vy_raw = 0, vz_raw = 0;
    bytes data;
};
```

Flat tagged report. Only the fields relevant to `type` are populated; the
rest keep their zero defaults. Raw integers are authoritative.

### parse_input_cargo

```cpp
std::vector<Report> parse_input_cargo(byte_span payload, std::int64_t capture_timestamp_us);
```

Parse a channel-3/4 cargo into typed reports. Handles 0xFB base timestamp
references and 0xFA rebases; every report's timestamp is base + delay where
base = capture − base_delta·100 µs.

### parse_gyro_rv_cargo

```cpp
std::optional<Report> parse_gyro_rv_cargo(byte_span payload, std::int64_t capture_timestamp_us);
```

Parse a channel-5 cargo (gyro-integrated RV, dense format). Two shapes: 7×i16
bare, or prefixed with 0xFB + i32 base delta + u16 delay (100 µs ticks).

## Bootloader / firmware

### FWDEPZ_HEADER_SIZE

```cpp
inline constexpr std::size_t FWDEPZ_HEADER_SIZE = 64;
```

### FWDEPZ_MAGIC

```cpp
inline constexpr char FWDEPZ_MAGIC[8] =;
```

Magic "FWDEPZ00".

### BlCmd

```cpp
enum class BlCmd : std::uint8_t {
    BootApplication = 0x01,
    DeviceReset = 0x02,
    GetDeviceName = 0x03,
    GetFirmwareName = 0x04,
    GetSerial = 0x05,
    GetMcuId = 0x06,
    GetMcuUid = 0x07,
    GetFlashInfo = 0x08,
    EraseApp = 0x09,
    WritePage = 0x0A,
    ReadPage = 0x0B,
    VerifyAppCrc = 0x0C,
};
```

### BlRpt

```cpp
enum class BlRpt : std::uint8_t {
    Status = 0x80,
    String = 0x81,
    McuId = 0x86,
    McuUid = 0x87,
    FlashInfo = 0x89,
    WritePage = 0x8A,
    ReadPage = 0x8B,
    VerifyAppCrc = 0x8C,
};
```

### BlStatus

```cpp
enum class BlStatus : std::uint8_t {
    Ack = 0x00,
    Error = 0x01,
    ErrAddr = 0x03,
    ErrCrcHdr = 0x04,
    ErrCrcPkt = 0x05,
    ErrFlash = 0x06,
};
```

### FlashInfo

```cpp
struct FlashInfo {
    std::uint16_t page_size;
    std::uint32_t app_start;
    std::uint32_t app_size;
};
```

#### FlashInfo.unpack

```cpp
static FlashInfo unpack(byte_span payload);
```

### pack_write_page

```cpp
bytes pack_write_page(std::uint32_t addr, byte_span data);
```

### pack_read_page

```cpp
bytes pack_read_page(std::uint32_t addr, std::uint16_t size);
```

### FwDepzErrorKind

```cpp
enum class FwDepzErrorKind {
    TooShort, Magic, HeaderCrc, Size
};
```

### FwDepzError

```cpp
class FwDepzError : public std::runtime_error {
    FwDepzErrorKind kind;
};
```

#### FwDepzError.FwDepzError

```cpp
FwDepzError(FwDepzErrorKind kind, const std::string& msg);
```

### FwDepzImage

```cpp
struct FwDepzImage {
    std::uint32_t load_addr;
    std::uint32_t fw_size;
    std::uint32_t fw_crc32;
    std::uint8_t cur_sec;
    std::uint8_t tot_sec;
    bytes payload;
};
```

Parsed and validated .fwdepz firmware container.

#### FwDepzImage.parse

```cpp
static FwDepzImage parse(byte_span blob);
```

Validation order: length, magic, header CRC (CCITT-FALSE over [0..61]
vs u16 LE at 62), then fw_size == payload length. Throws FwDepzError.

#### FwDepzImage.build

```cpp
static bytes build(std::uint32_t load_addr, byte_span payload, std::uint8_t cur_sec = 1, std::uint8_t tot_sec = 1);
```

Assemble a container (test/tooling helper).

#### FwDepzImage.payload_crc_ok

```cpp
bool payload_crc_ok() const;
```

## Datasets

### SCHEMA_PREFIX

```cpp
inline constexpr const char* SCHEMA_PREFIX = "depz.dataset/";
```

### TimeSync

```cpp
struct TimeSync {
    std::int64_t offset_us = 0;
    std::int64_t rtt_us = 0;
};
```

### DeviceMeta

```cpp
struct DeviceMeta {
    std::string serial;
    std::string sensor_type;
    std::string software_name;
    TimeSync time_sync;
};
```

### Record

```cpp
struct Record {
    std::string device_id;
    std::int64_t t_host_us = 0;
    std::string kind;
    std::map<std::string, std::int64_t> ints;
    std::map<std::string, std::string> strings;
    std::map<std::string, std::vector<std::int64_t>> arrays;
};
```

One record. The value object `v` is split by JSON type into typed maps
(numbers, strings, integer arrays) — enough for the SR04 and VL53L8 kinds.

### Reader

```cpp
class Reader {
    std::string schema;
    std::string created_utc;
    std::string note;
    std::map<std::string, DeviceMeta> devices;
    std::vector<Record> records;  // stable-sorted by t_host_us
};
```

#### Reader.Reader

```cpp
explicit Reader(const std::string& content);
```

Parse the whole `.depzdata` file content. Throws std::runtime_error on a
malformed file or a non-depz.dataset schema.

#### Reader.duration_us

```cpp
std::int64_t duration_us() const;
```
