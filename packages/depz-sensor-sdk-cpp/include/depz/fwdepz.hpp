// Bootloader wire codecs and .fwdepz container (contracts/06_BOOTLOADER_FLASHING.md).
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include "depz/span.hpp"

namespace depz {

inline constexpr std::size_t FWDEPZ_HEADER_SIZE = 64;
// Magic "FWDEPZ00".
inline constexpr char FWDEPZ_MAGIC[8] = {'F', 'W', 'D', 'E', 'P', 'Z', '0', '0'};

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

enum class BlStatus : std::uint8_t {
    Ack = 0x00,
    Error = 0x01,
    ErrAddr = 0x03,
    ErrCrcHdr = 0x04,
    ErrCrcPkt = 0x05,
    ErrFlash = 0x06,
};

struct FlashInfo {
    std::uint16_t page_size;
    std::uint32_t app_start;
    std::uint32_t app_size;
    static FlashInfo unpack(byte_span payload);
};

bytes pack_write_page(std::uint32_t addr, byte_span data);
bytes pack_read_page(std::uint32_t addr, std::uint16_t size);

enum class FwDepzErrorKind { TooShort, Magic, HeaderCrc, Size };

class FwDepzError : public std::runtime_error {
public:
    FwDepzError(FwDepzErrorKind kind, const std::string& msg)
        : std::runtime_error(msg), kind(kind) {}
    FwDepzErrorKind kind;
};

// Parsed and validated .fwdepz firmware container.
struct FwDepzImage {
    std::uint32_t load_addr;
    std::uint32_t fw_size;
    std::uint32_t fw_crc32;
    std::uint8_t cur_sec;
    std::uint8_t tot_sec;
    bytes payload;

    // Validation order: length, magic, header CRC (CCITT-FALSE over [0..61]
    // vs u16 LE at 62), then fw_size == payload length. Throws FwDepzError.
    static FwDepzImage parse(byte_span blob);

    // Assemble a container (test/tooling helper).
    static bytes build(std::uint32_t load_addr, byte_span payload, std::uint8_t cur_sec = 1,
                       std::uint8_t tot_sec = 1);

    bool payload_crc_ok() const;
};

}  // namespace depz
