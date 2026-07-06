// `.depzdata` dataset reader (contracts/09_DATASET_FORMAT.md).
//
// A dataset is JSON-lines: line 0 is the header (schema, per-device metadata),
// every following line is one record {d,t,k,v}. Records come back merged by
// host time with a stable sort (per-device order is monotonic, so a stable
// sort merges exactly). Mirrors the Python/TS DatasetReader.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace depz {
namespace dataset {

inline constexpr const char* SCHEMA_PREFIX = "depz.dataset/";

struct TimeSync {
    std::int64_t offset_us = 0;
    std::int64_t rtt_us = 0;
};

struct DeviceMeta {
    std::string serial;
    std::string sensor_type;
    std::string software_name;
    TimeSync time_sync;
};

// One record. The value object `v` is split by JSON type into typed maps
// (numbers, strings, integer arrays) — enough for the SR04 and VL53L8 kinds.
struct Record {
    std::string device_id;
    std::int64_t t_host_us = 0;
    std::string kind;
    std::map<std::string, std::int64_t> ints;
    std::map<std::string, std::string> strings;
    std::map<std::string, std::vector<std::int64_t>> arrays;
};

class Reader {
public:
    // Parse the whole `.depzdata` file content. Throws std::runtime_error on a
    // malformed file or a non-depz.dataset schema.
    explicit Reader(const std::string& content);

    std::string schema;
    std::string created_utc;
    std::string note;
    std::map<std::string, DeviceMeta> devices;
    std::vector<Record> records;  // stable-sorted by t_host_us

    std::int64_t duration_us() const {
        if (records.empty()) return 0;
        return records.back().t_host_us - records.front().t_host_us;
    }
};

}  // namespace dataset
}  // namespace depz
