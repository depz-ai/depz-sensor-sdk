// Common command/report IDs and payload codecs (contracts/02_COMMON_COMMANDS.md).
//
// Payload codecs return raw integers exactly as on the wire; unit conversions
// (0.1 degC, us) happen in a higher layer.
#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "depz/span.hpp"

namespace depz {

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

enum class SyncPinMode : std::uint8_t {
    Disable = 0x00,
    In = 0x01,
    OutStart = 0x02,
    OutEnd = 0x03,
    OutBoth = 0x04,
};

enum class SyncPinPolarity : std::uint8_t {
    IdleLow = 0x00,
    IdleHigh = 0x01,
};

// Value of the echoed-cmd byte in unsolicited reports.
inline constexpr std::uint8_t UNSOLICITED = 0x00;

struct StatusReport {
    std::uint8_t cmd;  // echoed request opcode; 0x00 = unsolicited
    std::uint8_t status;
    static StatusReport unpack(byte_span payload);
};

struct TextReport {
    std::uint8_t cmd;
    std::string text;
    static TextReport unpack(byte_span payload);
};

struct SyncTimeReport {
    std::uint64_t pc_timestamp_us;  // T1 echoed
    std::uint64_t mcu_rx_us;        // T2
    std::uint64_t mcu_tx_us;        // T3
    static SyncTimeReport unpack(byte_span payload);
};

struct TemperatureReport {
    std::uint64_t timestamp_us;
    std::int16_t raw_decidegrees;  // units of 0.1 degC
    double celsius() const { return raw_decidegrees / 10.0; }
    static TemperatureReport unpack(byte_span payload);
};

struct SequenceErrorReport {
    std::uint8_t expected_seq;
    std::uint8_t received_seq;
    static SequenceErrorReport unpack(byte_span payload);
};

struct SyncPinConfig {
    std::uint8_t pin;  // 1..5
    SyncPinMode mode;
    SyncPinPolarity polarity;
    bytes pack() const;
    static SyncPinConfig unpack(byte_span payload);
};

bytes pack_sync_time(std::uint64_t pc_timestamp_us);
bytes pack_set_payload_crc_type(std::uint8_t crc_type);

// NTP-style clock math, all us (contract 02 §5). Returns {offset_us, rtt_us}
// where offset = device_clock - host_clock, computed with truncation toward
// zero: offset = ((T2-T1)+(T3-T4)) / 2, rtt = (T4-T1)-(T3-T2).
std::pair<std::int64_t, std::int64_t> sync_time_offset_rtt(std::int64_t t1, std::int64_t t2,
                                                           std::int64_t t3, std::int64_t t4);

// Decode an ASCII device string, dropping trailing NUL/0xFF filler.
std::string strip_device_string(byte_span raw);

}  // namespace depz
