// `.depzdata` datasets (contracts/09_DATASET_FORMAT.md): the JSON value a
// record holds, the reader (JSON Lines, plain or a gzip container), the
// writer (a gzip container of stored blocks for `.gz` paths — no compression
// dependency) and the player of contract 09 §Player. Mirrors the Python
// reference `dataset.py`; the live layer (depz/recorder.hpp) adds the record
// value of each sensor's result and the session recorder.
#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "depz/errors.hpp"
#include "depz/span.hpp"

namespace depz {
namespace dataset {

// The schema id this SDK writes.
inline constexpr const char* SCHEMA = "depz.dataset/2";
inline constexpr const char* SCHEMA_PREFIX = "depz.dataset/";

// A JSON value: what a record's `v` holds. Integers stay integers and reals
// stay reals in the file, as the Python reference writes them. Objects keep
// insertion order.
class Value {
public:
    enum class Type { Null, Bool, Int, Double, String, Array, Object };
    using Object = std::vector<std::pair<std::string, Value>>;
    using Array = std::vector<Value>;

    Value() = default;  // null
    Value(bool b);
    Value(int i);
    Value(std::int64_t i);
    Value(std::uint32_t u);
    Value(std::uint64_t u);
    Value(double d);
    Value(const char* s);
    Value(std::string s);
    Value(Array a);
    Value(Object o);
    static Value array() { return Value(Array{}); }
    static Value object() { return Value(Object{}); }

    Type type() const { return type_; }
    bool is_null() const { return type_ == Type::Null; }
    bool is_bool() const { return type_ == Type::Bool; }
    bool is_int() const { return type_ == Type::Int; }
    bool is_double() const { return type_ == Type::Double; }
    bool is_number() const { return type_ == Type::Int || type_ == Type::Double; }
    bool is_string() const { return type_ == Type::String; }
    bool is_array() const { return type_ == Type::Array; }
    bool is_object() const { return type_ == Type::Object; }

    // Readers: a zero / empty default on a type mismatch; as_double also
    // reads an integer, as_int truncates a real.
    bool as_bool() const { return type_ == Type::Bool && b_; }
    std::int64_t as_int() const;
    double as_double() const;
    const std::string& as_string() const { return s_; }
    const Array& items() const { return arr_; }
    const Object& entries() const { return obj_; }
    std::size_t size() const { return type_ == Type::Array ? arr_.size() : type_ == Type::Object ? obj_.size() : 0; }
    // Members: by key (nullptr when absent / not an object), by index (throws
    // std::out_of_range), and `at` (throws std::out_of_range when absent).
    const Value* find(const std::string& key) const;
    const Value& at(const std::string& key) const;
    const Value& operator[](std::size_t i) const { return arr_.at(i); }
    bool has(const std::string& key) const { return find(key) != nullptr; }

    // Builders (this value must be an array / an object: std::logic_error
    // otherwise). set() replaces an existing key.
    Value& push(Value v);
    Value& set(const std::string& key, Value v);

    // Serialize compactly (no spaces, non-ASCII as \uXXXX, reals in the
    // shortest form that reads back exactly; NaN / infinities as null).
    std::string to_json() const;
    // Parse one JSON document. std::runtime_error when it does not parse.
    static Value parse(const std::string& text);

    // Structural equality (reals by value, so 2 == 2.0).
    bool operator==(const Value& o) const;
    bool operator!=(const Value& o) const { return !(*this == o); }

private:
    Type type_ = Type::Null;
    bool b_ = false;
    std::int64_t i_ = 0;
    double d_ = 0.0;
    std::string s_;
    Array arr_;
    Object obj_;
};

// True when `data` starts with the gzip magic.
bool is_gzip(byte_span data);
// The content of a gzip file (members one after the other; stored, fixed or
// dynamic Huffman blocks). std::runtime_error on a corrupt stream.
bytes gunzip(byte_span data);

struct TimeSync {
    std::int64_t offset_us = 0;
    std::int64_t rtt_us = 0;
};

// A header device entry. `model` is the part when one sensor_type covers
// several ("BNO085" / "BNO086" under bno086), empty when absent; `raw` is
// the whole entry, for fields this view leaves out.
struct DeviceMeta {
    std::string serial;
    std::string sensor_type;
    std::string software_name;
    std::string model;
    TimeSync time_sync;
    Value raw;
};

// One record: the device id, the host monotonic µs (device time − offset),
// the kind and the record's `v`.
struct Record {
    std::string device_id;
    std::int64_t t_host_us = 0;
    std::string kind;
    Value value;
};

class Reader {
public:
    // Parse a whole `.depzdata` document (UTF-8 JSON Lines: header + records)
    // or the bytes of a `.depzdata.gz` (inflated first). A damaged file is
    // refused whole with std::invalid_argument naming the line (contract 09,
    // 07 §3): an empty file, a header that is not a JSON object or whose
    // `schema` is not a `depz.dataset/…` string, a record line that is not a
    // JSON object, `d` / `k` not strings, `t` not an integer, `v` not an
    // object. Nothing is skipped or coerced; blank lines are skipped, a record
    // of an unknown `k` is kept. std::runtime_error on a corrupt gzip stream.
    explicit Reader(const std::string& content);
    // The same from a file path. std::runtime_error when it cannot be read.
    static Reader open(const std::string& path);

    std::string schema;
    std::string created_utc;
    std::string note;
    Value header;                               // the parsed header line
    std::map<std::string, DeviceMeta> devices;  // by device id
    // Every record merged by t_host_us with a stable sort (ties keep file
    // order): per-device order is only roughly monotonic in the file, so every
    // reader sorts (contract 09).
    std::vector<Record> records;

    // Host-time span from the first record to the last; 0 with fewer than two.
    std::int64_t duration_us() const {
        if (records.size() < 2) return 0;
        return records.back().t_host_us - records.front().t_host_us;
    }
};

// Low-level thread-safe `.depzdata` record writer: the header line at open,
// one record line per write. A path ending in `.gz` is written as a gzip
// container of stored (uncompressed) blocks — every gzip reader opens it.
// Prefer the session recorder of depz/recorder.hpp for live devices.
class Writer {
public:
    // Create `path` (overwritten) and write the header: `devices` are the
    // header device entries by device id (serial, sensor_type, software_name,
    // time_sync, …), `note` free text. std::runtime_error when the file
    // cannot be created.
    Writer(const std::string& path, const Value::Object& devices, const std::string& note = "");
    ~Writer();  // closes
    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;

    // One record: device id, host µs, kind, value. ArgumentError (invalid_state,
    // depz/errors.hpp) once closed, std::runtime_error on a write error.
    void write(const std::string& device_id, std::int64_t t_host_us, const std::string& kind, const Value& value);
    std::uint64_t records_written() const;
    // Flush, finish the gzip trailer and close the file. Idempotent.
    void close();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// DatasetPlayer state (contract 09 §Player).
enum class PlayerState {
    Idle,     // never played
    Playing,  // delivering records paced by the clock
    Paused,   // stopped mid-timeline; play() resumes
    Done,     // past the last record; play() restarts from the top
};

// The name contract 09 uses ("idle", "playing", "paused", "done").
const char* to_string(PlayerState s);

// DatasetPlayer construction options.
struct PlayerOptions {
    double speed = 1.0;             // ≥ 0; 0 = as fast as possible
    std::function<double()> clock;  // monotonic seconds; empty = the system's
    bool threaded = true;           // false: drive with tick()
};

// Timeline player over a dataset: paced delivery with play / pause / seek /
// speed — the engine behind a viewer's playback mode (contract 09 §Player).
//
// Timing: a media clock anchored at play / resume / seek to (now, `t` of the
// last delivered record — of the first record when none was). Record i is due
// at anchor_wall + (t_i − anchor_t) / speed: the first record goes out at
// once, each later one after its gap to the previous one, and a resumed or
// seeked player waits the next record's full gap. Pacing is against the
// clock, not cumulative: overdue records go out back to back. Speed 0
// delivers as fast as possible.
//
// Threads: by default records and the Done state are delivered on a player
// thread (started by the first play()); a state change a call makes is
// reported on the calling thread before it returns. Callbacks run outside the
// player's lock, so they may call its methods; once pause() returns no record
// callback is running or will start (unless pause was called from that
// callback). They must not block. With `threaded = false` no thread starts:
// the owner calls tick() from its own loop (a GUI timer, or a test with a
// fake clock). The destructor closes the player.
class DatasetPlayer {
public:
    using Unsubscribe = std::function<void()>;
    using Options = PlayerOptions;

    // A player over the reader's records (copied). std::invalid_argument for
    // a speed that is not finite and ≥ 0.
    explicit DatasetPlayer(const Reader& reader, PlayerOptions options = PlayerOptions());
    ~DatasetPlayer();
    DatasetPlayer(const DatasetPlayer&) = delete;
    DatasetPlayer& operator=(const DatasetPlayer&) = delete;

    PlayerState state() const;
    // Span of the timeline: the last record's t minus the first's.
    std::int64_t duration_us() const;
    // µs from the first record to the last delivered one (after a seek: to
    // the record just before the target); 0 before anything was delivered.
    std::int64_t position_us() const;
    double speed() const;
    // Change the speed (finite, ≥ 0, else std::invalid_argument). While
    // playing it applies at once: the rest of the current wait is rescaled.
    void set_speed(double speed);

    // Subscribe to delivered records / state changes (called with the new
    // state on every transition). The returned unsubscribe is idempotent.
    Unsubscribe on_record(std::function<void(const Record&)> cb);
    Unsubscribe on_state(std::function<void(PlayerState)> cb);
    // The message of a record callback that threw on the player thread:
    // playback paused there (the record counts as delivered).
    Unsubscribe on_delivery_error(std::function<void(const std::string&)> cb);

    // Start, or resume after pause(); no-op while playing. From Done (or a
    // position past the end) playback restarts at the first record. An empty
    // dataset goes Playing -> Done at once. ArgumentError (invalid_state) once closed.
    void play();
    // Stop delivering; play() resumes from the next record, which then waits
    // its full gap again. No-op unless playing.
    void pause();
    // Move to `offset_us` µs from the first record: the next record delivered
    // is the first with t >= first t + offset (negative: the start); records
    // in between are skipped, never delivered. The state does not change,
    // except that a seek past the end while playing ends playback (Done).
    // NaN is std::invalid_argument, ArgumentError (invalid_state) once
    // closed; on an empty dataset a seek is a no-op.
    void seek_us(double offset_us);
    // Deliver every record due now, on the calling thread. Returns the
    // seconds until the next one is due, nullopt when not playing. A record
    // callback's exception pauses playback and propagates.
    std::optional<double> tick();
    // Block until the player is not playing (done, or paused by someone
    // else): true, or false on timeout (nullopt: no timeout). Needs the
    // player thread, else ArgumentError (invalid_state).
    bool wait(std::optional<std::chrono::milliseconds> timeout = std::nullopt);
    // Stop playback and the player thread. Idempotent.
    void close();

private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};

}  // namespace dataset
}  // namespace depz
