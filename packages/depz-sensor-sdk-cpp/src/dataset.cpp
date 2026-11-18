// dataset.cpp — the C++ face of the C SDK's dataset layer (contract 09):
// `Value` is a C++ JSON tree that the C codec serializes and parses
// (depz_json), `Reader` is depz_dataset_parse, `Writer` is
// depz_dataset_writer, `DatasetPlayer` is depz_dataset_player plus the
// player thread and wait() the tick-driven C player leaves to its owner.
#include "depz/dataset.hpp"
#include "depz/device.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <unordered_map>

#include <depz_sensor_sdk.h>

#include "detail/c_bridge.hpp"

namespace depz {

// ── Value <-> depz_json ─────────────────────────────────────────────────────

depz_json* detail::json_to_c(const dataset::Value& v)
{
    using T = dataset::Value::Type;
    CJson c;
    switch (v.type()) {
        case T::Null: c.reset(depz_json_null()); break;
        case T::Bool: c.reset(depz_json_bool(v.as_bool())); break;
        case T::Int: c.reset(depz_json_int(v.as_int())); break;
        case T::Double: c.reset(depz_json_double(v.as_double())); break;
        case T::String: c.reset(depz_json_string_n(v.as_string().data(), v.as_string().size())); break;
        case T::Array:
            c.reset(depz_json_array());
            for (const dataset::Value& e : v.items())
                if (depz_json_push(c.get(), json_to_c(e)) != 0) throw std::bad_alloc();
            break;
        case T::Object:
            c.reset(depz_json_object());
            for (const auto& [k, e] : v.entries())
                if (depz_json_set(c.get(), k.c_str(), json_to_c(e)) != 0) throw std::bad_alloc();
            break;
    }
    if (!c) throw std::bad_alloc();
    return c.release();
}

dataset::Value detail::json_from_c(const depz_json* c)
{
    using V = dataset::Value;
    switch (depz_json_kind_of(c)) {
        case DEPZ_JSON_BOOL: return V(depz_json_bool_value(c));
        case DEPZ_JSON_INT: return V(static_cast<std::int64_t>(depz_json_int_value(c)));
        case DEPZ_JSON_DOUBLE: return V(depz_json_double_value(c));
        case DEPZ_JSON_STRING: return V(std::string(depz_json_string_value(c), depz_json_string_len(c)));
        case DEPZ_JSON_ARRAY: {
            V::Array a;
            const std::size_t n = depz_json_len(c);
            a.reserve(n);
            for (std::size_t i = 0; i < n; ++i) a.push_back(json_from_c(depz_json_at(c, i)));
            return V(std::move(a));
        }
        case DEPZ_JSON_OBJECT: {
            V::Object o;
            const std::size_t n = depz_json_len(c);
            o.reserve(n);
            for (std::size_t i = 0; i < n; ++i) o.emplace_back(depz_json_key_at(c, i), json_from_c(depz_json_at(c, i)));
            return V(std::move(o));
        }
        case DEPZ_JSON_NULL: break;
    }
    return V();
}

namespace dataset {

// ── Value ───────────────────────────────────────────────────────────────────

Value::Value(bool b) : type_(Type::Bool), b_(b) {}
Value::Value(int i) : type_(Type::Int), i_(i) {}
Value::Value(std::int64_t i) : type_(Type::Int), i_(i) {}
Value::Value(std::uint32_t u) : type_(Type::Int), i_(static_cast<std::int64_t>(u)) {}
Value::Value(std::uint64_t u) : type_(Type::Int), i_(static_cast<std::int64_t>(u)) {}
Value::Value(double d) : type_(Type::Double), d_(d) {}
Value::Value(const char* s) : type_(Type::String), s_(s ? s : "") {}
Value::Value(std::string s) : type_(Type::String), s_(std::move(s)) {}
Value::Value(Array a) : type_(Type::Array), arr_(std::move(a)) {}
Value::Value(Object o) : type_(Type::Object), obj_(std::move(o)) {}

std::int64_t Value::as_int() const
{
    if (type_ == Type::Int) return i_;
    if (type_ == Type::Double) {
        // a real outside int64 (1e400 parses as inf) saturates: the cast is
        // undefined behaviour reachable from a file (as the C codec does)
        if (d_ != d_) return 0;
        if (d_ >= 9223372036854775808.0) return INT64_MAX;
        if (d_ <= -9223372036854775808.0) return INT64_MIN;
        return static_cast<std::int64_t>(d_);
    }
    return 0;
}

double Value::as_double() const
{
    if (type_ == Type::Double) return d_;
    if (type_ == Type::Int) return static_cast<double>(i_);
    return 0.0;
}

const Value* Value::find(const std::string& key) const
{
    if (type_ != Type::Object) return nullptr;
    for (const auto& [k, v] : obj_)
        if (k == key) return &v;
    return nullptr;
}

const Value& Value::at(const std::string& key) const
{
    const Value* v = find(key);
    if (!v) throw std::out_of_range("no member \"" + key + "\"");
    return *v;
}

Value& Value::push(Value v)
{
    if (type_ != Type::Array) throw std::logic_error("push on a non-array JSON value");
    arr_.push_back(std::move(v));
    return *this;
}

Value& Value::set(const std::string& key, Value v)
{
    if (type_ != Type::Object) throw std::logic_error("set on a non-object JSON value");
    for (auto& [k, old] : obj_) {
        if (k == key) {
            old = std::move(v);
            return *this;
        }
    }
    obj_.emplace_back(key, std::move(v));
    return *this;
}

bool Value::operator==(const Value& o) const
{
    if (is_number() && o.is_number()) {
        if (type_ == Type::Int && o.type_ == Type::Int) return i_ == o.i_;
        return as_double() == o.as_double();
    }
    if (type_ != o.type_) return false;
    switch (type_) {
        case Type::Null: return true;
        case Type::Bool: return b_ == o.b_;
        case Type::String: return s_ == o.s_;
        case Type::Array: return arr_ == o.arr_;
        case Type::Object:
            if (obj_.size() != o.obj_.size()) return false;
            for (const auto& [k, v] : obj_) {
                const Value* w = o.find(k);
                if (!w || !(v == *w)) return false;
            }
            return true;
        default: return false;
    }
}

std::string Value::to_json() const
{
    const detail::CJson c(detail::json_to_c(*this));
    const detail::CBuf<char> text(depz_json_dumps(c.get()));
    if (!text) throw std::bad_alloc();
    return std::string(text.get());
}

Value Value::parse(const std::string& text)
{
    const detail::CJson c(depz_json_parse(text.data(), text.size()));
    if (!c) throw std::runtime_error("JSON: does not parse");
    return detail::json_from_c(c.get());
}

// ── gzip ────────────────────────────────────────────────────────────────────

bool is_gzip(byte_span data) { return depz_is_gzip(detail::u8p(data), data.size()); }

bytes gunzip(byte_span data)
{
    std::size_t n = 0;
    const detail::CBuf<std::uint8_t> out(depz_gunzip(detail::u8p(data), data.size(), &n));
    if (!out) throw std::runtime_error("gzip: corrupt stream");
    return detail::to_bytes(out.get(), n);
}

// ── Reader ──────────────────────────────────────────────────────────────────

namespace {

struct DatasetFree {
    void operator()(depz_dataset* d) const noexcept { depz_dataset_free(d); }
};
using CDataset = std::unique_ptr<depz_dataset, DatasetFree>;

// `text` parsed by the C reader. A corrupt gzip container or a damaged line
// is the caller's bad argument (contract 07 §3), as the C reader reports it.
CDataset parse_dataset(const std::uint8_t* p, std::size_t n)
{
    detail::CBuf<std::uint8_t> plain;
    if (depz_is_gzip(p, n)) {
        plain.reset(depz_gunzip(p, n, &n));
        if (!plain) throw std::invalid_argument("dataset: corrupt gzip container");
        p = plain.get();
    }
    CDataset d(depz_dataset_parse(reinterpret_cast<const char*>(p), n));
    if (!d) throw std::invalid_argument(depz_last_error());
    return d;
}

std::string get_str(const Value& o, const char* key)
{
    const Value* v = o.find(key);
    return v && v->is_string() ? v->as_string() : std::string();
}

std::string get_str(const depz_json* s)
{
    const char* p = depz_json_string_value(s);
    return p ? std::string(p, depz_json_string_len(s)) : std::string();
}

}  // namespace

Reader::Reader(const std::string& content)
{
    const CDataset d = parse_dataset(reinterpret_cast<const std::uint8_t*>(content.data()), content.size());
    header = detail::json_from_c(depz_dataset_header(d.get()));
    schema = get_str(header, "schema");
    created_utc = get_str(header, "created_utc");
    note = get_str(header, "note");
    if (const Value* devs = header.find("devices"); devs && devs->is_object()) {
        for (const auto& [id, meta] : devs->entries()) {
            DeviceMeta dm;
            dm.serial = get_str(meta, "serial");
            dm.sensor_type = get_str(meta, "sensor_type");
            dm.software_name = get_str(meta, "software_name");
            dm.model = get_str(meta, "model");
            if (const Value* ts = meta.find("time_sync"); ts && ts->is_object()) {
                if (const Value* o = ts->find("offset_us")) dm.time_sync.offset_us = o->as_int();
                if (const Value* r = ts->find("rtt_us")) dm.time_sync.rtt_us = r->as_int();
            }
            dm.raw = meta;
            devices.emplace(id, std::move(dm));
        }
    }
    // the records in the C reader's order (merged by t_host_us, ties in file
    // order — contract 09), each from its parsed line: ids and kinds whole,
    // whatever their length
    const std::size_t n = depz_dataset_record_count(d.get());
    records.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        const depz_json* line = depz_dataset_record_json(d.get(), i);
        Record rec;
        rec.device_id = get_str(depz_json_get(line, "d"));
        rec.t_host_us = depz_json_int_value(depz_json_get(line, "t"));
        rec.kind = get_str(depz_json_get(line, "k"));
        rec.value = detail::json_from_c(depz_json_get(line, "v"));
        records.push_back(std::move(rec));
    }
}

Reader Reader::open(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + path);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return Reader(content);
}

// ── Writer ──────────────────────────────────────────────────────────────────

struct Writer::Impl {
    std::mutex lock;
    depz_dataset_writer* w = nullptr;  // null once closed
    std::uint64_t records = 0;         // the finished file's count, after close()
};

// Whether `v`, at nesting level `level` of its line, stays within what a reader takes
// (contract 08 «Line rules»: the line's outermost value is level 1).
static bool nests_within_limit(const Value& v, int level)
{
    if (!v.is_array() && !v.is_object()) return true;
    if (level > 256) return false;
    if (v.is_array()) {
        for (const Value& x : v.items())
            if (!nests_within_limit(x, level + 1)) return false;
    } else {
        for (const auto& kv : v.entries())
            if (!nests_within_limit(kv.second, level + 1)) return false;
    }
    return true;
}

// Whether `v` holds no NaN / Infinity, which JSON has no form for (contract 09).
static bool all_finite(const Value& v)
{
    if (v.is_double()) return std::isfinite(v.as_double());
    if (v.is_array()) {
        for (const Value& x : v.items())
            if (!all_finite(x)) return false;
    } else if (v.is_object()) {
        for (const auto& kv : v.entries())
            if (!all_finite(kv.second)) return false;
    }
    return true;
}

Writer::Writer(const std::string& path, const Value::Object& devices, const std::string& note) : impl_(new Impl)
{
    // what the C writer refuses too, before the file is touched
    for (const auto& kv : devices)
        if (!nests_within_limit(kv.second, 3))
            throw ArgumentError(kErrArg, "dataset: the header's devices are nested deeper than 256 levels");
    for (const auto& kv : devices)
        if (!all_finite(kv.second))
            throw ArgumentError(kErrArg, "dataset: the header's devices hold NaN or Infinity, which JSON has no form for");
    const detail::CJson devs(detail::json_to_c(Value(devices)));
    impl_->w = depz_dataset_writer_open(path.c_str(), devs.get(), note.empty() ? nullptr : note.c_str());
    if (!impl_->w) throw std::runtime_error("cannot create " + path);
}

Writer::~Writer()
{
    try {
        close();
    } catch (...) {
    }
}

void Writer::write(const std::string& device_id, std::int64_t t_host_us, const std::string& kind, const Value& value)
{
    std::lock_guard<std::mutex> g(impl_->lock);
    if (!impl_->w) throw ArgumentError(kErrArg, "dataset Writer is closed");
    // what every reader requires of "v" (the C writer refuses it too)
    if (!value.is_object()) throw ArgumentError(kErrArg, "dataset: a record's value must be a JSON object");
    if (!nests_within_limit(value, 2)) throw ArgumentError(kErrArg, "dataset: a record's value is nested deeper than 256 levels");
    if (!all_finite(value)) throw ArgumentError(kErrArg, "dataset: a record's value holds NaN or Infinity, which JSON has no form for");
    if (depz_dataset_writer_write(impl_->w, device_id.c_str(), t_host_us, kind.c_str(), detail::json_to_c(value)) != 0)
        throw std::runtime_error("dataset write failed");
}

std::uint64_t Writer::records_written() const
{
    std::lock_guard<std::mutex> g(impl_->lock);
    return impl_->w ? depz_dataset_writer_records_written(impl_->w) : impl_->records;
}

void Writer::close()
{
    std::lock_guard<std::mutex> g(impl_->lock);
    depz_dataset_writer* w = impl_->w;
    if (!w) return;
    impl_->records = depz_dataset_writer_records_written(w);
    impl_->w = nullptr;
    if (depz_dataset_writer_close(w) != 0) throw std::runtime_error("dataset flush failed");
}

// ── DatasetPlayer (contract 09 §Player) ───────────────────────────────────────

const char* to_string(PlayerState s)
{
    switch (s) {
        case PlayerState::Idle: return "idle";
        case PlayerState::Playing: return "playing";
        case PlayerState::Paused: return "paused";
        case PlayerState::Done: return "done";
    }
    return "?";
}

namespace {

PlayerState state_from_c(depz_player_state s)
{
    switch (s) {
        case DEPZ_PLAYER_PLAYING: return PlayerState::Playing;
        case DEPZ_PLAYER_PAUSED: return PlayerState::Paused;
        case DEPZ_PLAYER_DONE: return PlayerState::Done;
        case DEPZ_PLAYER_IDLE: break;
    }
    return PlayerState::Idle;
}

struct PlayerFree {
    void operator()(depz_dataset_player* p) const noexcept { depz_dataset_player_free(p); }
};

// Longest single wait of the player thread, seconds (see run()).
constexpr double MAX_WAIT_S = 60.0;

}  // namespace

// The C player paces and delivers; this holds its lock, the player thread,
// the C++ callbacks and the records it hands them. The C player plays a
// depz_dataset: the reader's records, as given, written as dataset lines
// and parsed back (the C reader orders them as the reader did — by
// t_host_us, ties in the given order), each C record mapped back to its
// Record through the address of its value.
struct DatasetPlayer::Impl : std::enable_shared_from_this<Impl> {
    using RecordCb = std::shared_ptr<std::function<void(const Record&)>>;
    using StateCb = std::shared_ptr<std::function<void(PlayerState)>>;

    const std::vector<Record> recs;
    const std::function<double()> clock;
    const bool threaded;
    CDataset dataset;
    std::unique_ptr<depz_dataset_player, PlayerFree> player;
    std::unordered_map<const depz_json*, std::size_t> index;  // a C record's value -> recs

    std::mutex lock;  // the C player and every field below; released around user callbacks
    std::condition_variable changed;
    bool closed = false;
    std::uint64_t generation = 0;  // bumped on every change a waiting player thread must see
    std::optional<std::thread::id> delivering;  // the thread running record callbacks now
    std::exception_ptr clock_error;             // a clock that threw inside a C call
    std::vector<RecordCb> record_cbs;
    std::vector<StateCb> state_cbs;
    std::atomic<std::uint64_t> callback_errors{0};
    std::thread thread;

    Impl(const Reader& reader, PlayerOptions o)
        : recs(reader.records),
          clock(o.clock ? std::move(o.clock) : std::function<double()>([start = std::chrono::steady_clock::now()] {
              return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
          })),
          threaded(o.threaded)
    {
        dataset = dataset_of(recs);
        player.reset(depz_dataset_player_new(dataset.get(), o.speed, &clock_tramp, this));
        if (!player) throw std::invalid_argument("speed must be a finite number >= 0");
        std::vector<std::size_t> order(recs.size());
        std::iota(order.begin(), order.end(), std::size_t{0});
        std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return recs[a].t_host_us < recs[b].t_host_us; });
        for (std::size_t i = 0; i < order.size(); ++i) {
            depz_dataset_record r;
            depz_dataset_get_record(dataset.get(), i, &r);
            index.emplace(r.value, order[i]);
        }
        depz_dataset_player_on_record(player.get(), &record_tramp, this, nullptr);
        depz_dataset_player_on_state(player.get(), &state_tramp, this, nullptr);
    }

    static CDataset dataset_of(const std::vector<Record>& recs)
    {
        std::string text = std::string("{\"schema\":\"") + SCHEMA + "\"}\n";
        for (const Record& r : recs) {
            const detail::CJson line(depz_json_object());
            depz_json_set(line.get(), "d", depz_json_string_n(r.device_id.data(), r.device_id.size()));
            depz_json_set(line.get(), "t", depz_json_int(r.t_host_us));
            depz_json_set(line.get(), "k", depz_json_string_n(r.kind.data(), r.kind.size()));
            depz_json_set(line.get(), "v", detail::json_to_c(r.value));
            const detail::CBuf<char> s(depz_json_dumps(line.get()));
            if (!s) throw std::bad_alloc();
            text += s.get();
            text += '\n';
        }
        CDataset d(depz_dataset_parse(text.data(), text.size()));
        if (!d) throw std::invalid_argument(depz_last_error());
        return d;
    }

    static double clock_tramp(void* user)
    {
        auto* self = static_cast<Impl*>(user);
        try {
            return self->clock();
        } catch (...) {
            // not through the C frames: rethrown once the C call returns
            self->clock_error = std::current_exception();
            return 0.0;
        }
    }

    void rethrow_clock()
    {
        if (!clock_error) return;
        std::exception_ptr e = clock_error;
        clock_error = nullptr;
        changed.notify_all();  // the player thread waits this out (run())
        std::rethrow_exception(e);
    }

    void bump()
    {
        ++generation;
        changed.notify_all();
    }

    // Hand `item` to every callback; one that throws is counted, reported and
    // dropped — the rest still get it, playback goes on (contract 09).
    template <class Cbs, class T>
    void deliver(const Cbs& cbs, const T& item)
    {
        for (const auto& cb : cbs) {
            try {
                (*cb)(item);
            } catch (...) {
                detail::callback_threw(callback_errors);
            }
        }
    }

    // The C player's callbacks run inside a C call made under `lock`; the
    // user's callbacks run outside it (so they may call the player back).
    static void record_tramp(const depz_dataset_record* r, void* user)
    {
        auto* self = static_cast<Impl*>(user);
        const auto it = self->index.find(r->value);
        if (it == self->index.end()) return;
        const Record& rec = self->recs[it->second];
        const std::vector<RecordCb> cbs = self->record_cbs;
        self->delivering = std::this_thread::get_id();
        self->lock.unlock();
        self->deliver(cbs, rec);
        self->lock.lock();
        self->delivering.reset();
        self->changed.notify_all();
    }

    static void state_tramp(depz_player_state s, void* user)
    {
        auto* self = static_cast<Impl*>(user);
        const std::vector<StateCb> cbs = self->state_cbs;
        self->bump();  // wait() and the player thread see the new state
        self->lock.unlock();
        self->deliver(cbs, state_from_c(s));
        self->lock.lock();
    }

    // The player thread: tick, then sleep until the next record is due or
    // something changes (play / pause / seek / speed / close).
    void run()
    {
        std::unique_lock<std::mutex> g(lock);
        for (;;) {
            if (closed) return;
            if (clock_error) {
                // a caller's play() / seek_us() / set_speed() raised it in a
                // state callback's window: that call rethrows it (rethrow_clock)
                changed.wait(g, [&] { return closed || !clock_error; });
                continue;
            }
            const std::uint64_t gen = generation;
            const double wait = depz_dataset_player_tick(player.get());
            if (clock_error) {
                // The owner's clock threw on the player thread, where nothing
                // can catch it (an exception leaving the thread is
                // std::terminate): counted and reported as a callback that
                // threw (contract 09), and the player pauses — play() runs
                // the clock again and hands its exception to the caller.
                clock_error = nullptr;
                detail::callback_threw(callback_errors);
                depz_dataset_player_pause(player.get());  // Paused, reported on this thread
                continue;
            }
            if (closed) return;
            if (wait < 0) {
                changed.wait(g, [&] { return closed || generation != gen; });
            } else if (generation == gen) {
                // clamped: a tiny speed gives waits a timer cannot hold (the
                // duration overflows, the thread spins); re-evaluated after every wake
                changed.wait_for(g, std::chrono::duration<double>(wait < MAX_WAIT_S ? wait : MAX_WAIT_S));
            }
        }
    }
};

DatasetPlayer::DatasetPlayer(const Reader& reader, PlayerOptions options) : impl_(std::make_shared<Impl>(reader, std::move(options))) {}

DatasetPlayer::~DatasetPlayer() { close(); }

PlayerState DatasetPlayer::state() const
{
    std::lock_guard<std::mutex> g(impl_->lock);
    return state_from_c(depz_dataset_player_state(impl_->player.get()));
}

std::int64_t DatasetPlayer::duration_us() const
{
    std::lock_guard<std::mutex> g(impl_->lock);
    return depz_dataset_player_duration_us(impl_->player.get());
}

std::int64_t DatasetPlayer::position_us() const
{
    std::lock_guard<std::mutex> g(impl_->lock);
    return depz_dataset_player_position_us(impl_->player.get());
}

double DatasetPlayer::speed() const
{
    std::lock_guard<std::mutex> g(impl_->lock);
    return depz_dataset_player_speed(impl_->player.get());
}

void DatasetPlayer::set_speed(double speed)
{
    std::lock_guard<std::mutex> g(impl_->lock);
    if (depz_dataset_player_set_speed(impl_->player.get(), speed) != 0) throw std::invalid_argument("speed must be a finite number >= 0");
    impl_->rethrow_clock();
    impl_->bump();
}

std::uint64_t DatasetPlayer::callback_errors() const { return impl_->callback_errors.load(std::memory_order_relaxed); }

DatasetPlayer::Unsubscribe DatasetPlayer::on_record(std::function<void(const Record&)> cb)
{
    auto holder = std::make_shared<std::function<void(const Record&)>>(std::move(cb));
    {
        std::lock_guard<std::mutex> g(impl_->lock);
        impl_->record_cbs.push_back(holder);
    }
    std::weak_ptr<Impl> weak = impl_;
    return [weak, holder] {
        if (auto p = weak.lock()) {
            std::lock_guard<std::mutex> g(p->lock);
            auto& v = p->record_cbs;
            v.erase(std::remove(v.begin(), v.end(), holder), v.end());
        }
    };
}

DatasetPlayer::Unsubscribe DatasetPlayer::on_state(std::function<void(PlayerState)> cb)
{
    auto holder = std::make_shared<std::function<void(PlayerState)>>(std::move(cb));
    {
        std::lock_guard<std::mutex> g(impl_->lock);
        impl_->state_cbs.push_back(holder);
    }
    std::weak_ptr<Impl> weak = impl_;
    return [weak, holder] {
        if (auto p = weak.lock()) {
            std::lock_guard<std::mutex> g(p->lock);
            auto& v = p->state_cbs;
            v.erase(std::remove(v.begin(), v.end(), holder), v.end());
        }
    };
}

void DatasetPlayer::play()
{
    std::lock_guard<std::mutex> g(impl_->lock);
    if (impl_->closed) throw ArgumentError(kErrArg, "DatasetPlayer is closed");
    depz_dataset_player_play(impl_->player.get());  // the state callbacks run here, on this thread
    impl_->rethrow_clock();
    // the first play() starts the player thread (a state callback may have closed the player meanwhile)
    if (impl_->threaded && !impl_->thread.joinable() && !impl_->closed &&
        depz_dataset_player_state(impl_->player.get()) == DEPZ_PLAYER_PLAYING) {
        std::shared_ptr<Impl> self = impl_;
        impl_->thread = std::thread([self] { self->run(); });
    }
    impl_->bump();
}

void DatasetPlayer::pause()
{
    std::unique_lock<std::mutex> g(impl_->lock);
    depz_dataset_player_pause(impl_->player.get());
    impl_->bump();
    // Once pause returns no record callback runs: wait out one on another thread.
    const auto me = std::this_thread::get_id();
    impl_->changed.wait(g, [&] { return !impl_->delivering || *impl_->delivering == me; });
}

void DatasetPlayer::seek_us(double offset_us)
{
    if (std::isnan(offset_us)) throw std::invalid_argument("seek_us: offset must be a number");
    std::lock_guard<std::mutex> g(impl_->lock);
    if (impl_->closed) throw ArgumentError(kErrArg, "DatasetPlayer is closed");
    depz_dataset_player_seek_us(impl_->player.get(), offset_us);
    impl_->rethrow_clock();
    impl_->bump();
}

std::optional<double> DatasetPlayer::tick()
{
    std::lock_guard<std::mutex> g(impl_->lock);
    const double wait = depz_dataset_player_tick(impl_->player.get());
    impl_->rethrow_clock();
    if (wait < 0) return std::nullopt;
    return wait;
}

bool DatasetPlayer::wait(std::optional<std::chrono::milliseconds> timeout)
{
    if (!impl_->threaded) throw ArgumentError(kErrArg, "wait() needs threaded = true; drive tick() instead");
    std::unique_lock<std::mutex> g(impl_->lock);
    const auto not_playing = [&] { return depz_dataset_player_state(impl_->player.get()) != DEPZ_PLAYER_PLAYING; };
    if (!timeout) {
        impl_->changed.wait(g, not_playing);
        return true;
    }
    // Capped as every other timeout of this binding (ms() in device.cpp): wait_for overflows
    // converting a larger one to its clock's nanoseconds, an instant "still playing".
    const auto n = timeout->count();
    return impl_->changed.wait_for(g, std::chrono::milliseconds(n < 0 ? 0 : n > 0x7FFFFFFF ? 0x7FFFFFFF : n), not_playing);
}

void DatasetPlayer::close()
{
    std::thread t;
    {
        std::lock_guard<std::mutex> g(impl_->lock);
        if (impl_->closed) return;
        depz_dataset_player_pause(impl_->player.get());  // Playing -> Paused, reported on this thread
        impl_->closed = true;
        impl_->bump();
        t = std::move(impl_->thread);
    }
    if (t.joinable()) {
        if (t.get_id() == std::this_thread::get_id()) t.detach();
        else t.join();
    }
}

}  // namespace dataset
}  // namespace depz
