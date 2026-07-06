#include "depz/dataset.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace depz {
namespace dataset {

namespace {

// ── minimal single-line JSON parser (objects/arrays/strings/numbers/bool/null).
// Numbers are kept as int64 and double; enough for the dataset schema.
struct JVal {
    enum class T { Null, Bool, Int, Double, String, Array, Object } type = T::Null;
    bool b = false;
    std::int64_t i = 0;
    double d = 0.0;
    std::string s;
    std::vector<JVal> arr;
    std::map<std::string, JVal> obj;

    bool is_int() const { return type == T::Int; }
    bool is_double() const { return type == T::Double; }
    bool is_string() const { return type == T::String; }
    bool is_array() const { return type == T::Array; }
    bool is_object() const { return type == T::Object; }
    std::int64_t as_int() const { return type == T::Double ? static_cast<std::int64_t>(d) : i; }
    const JVal* find(const std::string& k) const {
        auto it = obj.find(k);
        return it == obj.end() ? nullptr : &it->second;
    }
};

class JParser {
public:
    explicit JParser(const std::string& t) : t_(t) {}
    JVal parse() {
        skip_ws();
        JVal v = parse_value();
        skip_ws();
        if (pos_ != t_.size()) throw std::runtime_error("trailing data in JSON line");
        return v;
    }

private:
    const std::string& t_;
    std::size_t pos_ = 0;

    [[noreturn]] void fail(const std::string& m) {
        throw std::runtime_error("dataset JSON error: " + m);
    }
    char peek() { return pos_ < t_.size() ? t_[pos_] : '\0'; }
    void skip_ws() {
        while (pos_ < t_.size()) {
            char c = t_[pos_];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') ++pos_;
            else break;
        }
    }
    JVal parse_value() {
        skip_ws();
        char c = peek();
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"') {
            JVal v;
            v.type = JVal::T::String;
            v.s = parse_string();
            return v;
        }
        if (c == 't' || c == 'f') return parse_bool();
        if (c == 'n') return parse_null();
        return parse_number();
    }
    JVal parse_object() {
        JVal v;
        v.type = JVal::T::Object;
        ++pos_;  // {
        skip_ws();
        if (peek() == '}') {
            ++pos_;
            return v;
        }
        while (true) {
            skip_ws();
            if (peek() != '"') fail("expected object key");
            std::string key = parse_string();
            skip_ws();
            if (peek() != ':') fail("expected ':'");
            ++pos_;
            v.obj.emplace(std::move(key), parse_value());
            skip_ws();
            char c = peek();
            if (c == ',') {
                ++pos_;
                continue;
            }
            if (c == '}') {
                ++pos_;
                break;
            }
            fail("expected ',' or '}'");
        }
        return v;
    }
    JVal parse_array() {
        JVal v;
        v.type = JVal::T::Array;
        ++pos_;  // [
        skip_ws();
        if (peek() == ']') {
            ++pos_;
            return v;
        }
        while (true) {
            v.arr.push_back(parse_value());
            skip_ws();
            char c = peek();
            if (c == ',') {
                ++pos_;
                continue;
            }
            if (c == ']') {
                ++pos_;
                break;
            }
            fail("expected ',' or ']'");
        }
        return v;
    }
    std::string parse_string() {
        ++pos_;  // opening quote
        std::string out;
        while (pos_ < t_.size()) {
            char c = t_[pos_++];
            if (c == '"') return out;
            if (c == '\\') {
                if (pos_ >= t_.size()) fail("bad escape");
                char e = t_[pos_++];
                switch (e) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u': {
                        if (pos_ + 4 > t_.size()) fail("bad \\u escape");
                        unsigned code = 0;
                        for (int k = 0; k < 4; ++k) {
                            char h = t_[pos_++];
                            code <<= 4;
                            if (h >= '0' && h <= '9') code |= (h - '0');
                            else if (h >= 'a' && h <= 'f') code |= (h - 'a' + 10);
                            else if (h >= 'A' && h <= 'F') code |= (h - 'A' + 10);
                            else fail("bad hex in \\u");
                        }
                        // Basic BMP -> UTF-8 (dataset strings are ASCII in practice).
                        if (code < 0x80) {
                            out.push_back(static_cast<char>(code));
                        } else if (code < 0x800) {
                            out.push_back(static_cast<char>(0xC0 | (code >> 6)));
                            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                        } else {
                            out.push_back(static_cast<char>(0xE0 | (code >> 12)));
                            out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                        }
                        break;
                    }
                    default: fail("unknown escape");
                }
            } else {
                out.push_back(c);
            }
        }
        fail("unterminated string");
    }
    JVal parse_bool() {
        JVal v;
        v.type = JVal::T::Bool;
        if (t_.compare(pos_, 4, "true") == 0) {
            v.b = true;
            pos_ += 4;
        } else if (t_.compare(pos_, 5, "false") == 0) {
            v.b = false;
            pos_ += 5;
        } else {
            fail("bad literal");
        }
        return v;
    }
    JVal parse_null() {
        if (t_.compare(pos_, 4, "null") != 0) fail("bad literal");
        pos_ += 4;
        return JVal{};
    }
    JVal parse_number() {
        std::size_t start = pos_;
        bool is_double = false;
        if (peek() == '-' || peek() == '+') ++pos_;
        while (pos_ < t_.size()) {
            char c = t_[pos_];
            if (c >= '0' && c <= '9') {
                ++pos_;
            } else if (c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-') {
                is_double = true;
                ++pos_;
            } else {
                break;
            }
        }
        std::string num = t_.substr(start, pos_ - start);
        if (num.empty()) fail("expected number");
        JVal v;
        if (is_double) {
            v.type = JVal::T::Double;
            v.d = std::stod(num);
        } else {
            v.type = JVal::T::Int;
            v.i = static_cast<std::int64_t>(std::stoll(num));
        }
        return v;
    }
};

std::string get_str(const JVal& o, const std::string& key) {
    const JVal* v = o.find(key);
    return (v && v->is_string()) ? v->s : std::string();
}

}  // namespace

Reader::Reader(const std::string& content) {
    // Split into non-blank lines.
    std::vector<std::string> lines;
    std::size_t i = 0;
    while (i < content.size()) {
        std::size_t nl = content.find('\n', i);
        std::string line = content.substr(i, nl == std::string::npos ? std::string::npos : nl - i);
        // trim trailing \r
        if (!line.empty() && line.back() == '\r') line.pop_back();
        bool blank = line.find_first_not_of(" \t") == std::string::npos;
        if (!blank) lines.push_back(std::move(line));
        if (nl == std::string::npos) break;
        i = nl + 1;
    }
    if (lines.empty()) throw std::runtime_error("empty dataset");

    JVal header = JParser(lines[0]).parse();
    schema = get_str(header, "schema");
    if (schema.compare(0, std::string(SCHEMA_PREFIX).size(), SCHEMA_PREFIX) != 0)
        throw std::runtime_error("not a depz.dataset file");
    created_utc = get_str(header, "created_utc");
    note = get_str(header, "note");

    if (const JVal* devs = header.find("devices"); devs && devs->is_object()) {
        for (const auto& [id, meta] : devs->obj) {
            DeviceMeta dm;
            dm.serial = get_str(meta, "serial");
            dm.sensor_type = get_str(meta, "sensor_type");
            dm.software_name = get_str(meta, "software_name");
            if (const JVal* ts = meta.find("time_sync"); ts && ts->is_object()) {
                if (const JVal* o = ts->find("offset_us")) dm.time_sync.offset_us = o->as_int();
                if (const JVal* r = ts->find("rtt_us")) dm.time_sync.rtt_us = r->as_int();
            }
            devices.emplace(id, std::move(dm));
        }
    }

    // Records preserve file order first, then a stable sort merges by host time.
    std::vector<Record> raw;
    for (std::size_t k = 1; k < lines.size(); ++k) {
        JVal ev = JParser(lines[k]).parse();
        Record rec;
        rec.device_id = get_str(ev, "d");
        if (const JVal* t = ev.find("t")) rec.t_host_us = t->as_int();
        rec.kind = get_str(ev, "k");
        if (const JVal* v = ev.find("v"); v && v->is_object()) {
            for (const auto& [field, val] : v->obj) {
                if (val.is_string()) {
                    rec.strings.emplace(field, val.s);
                } else if (val.is_int() || val.is_double()) {
                    rec.ints.emplace(field, val.as_int());
                } else if (val.is_array()) {
                    std::vector<std::int64_t> a;
                    a.reserve(val.arr.size());
                    for (const JVal& e : val.arr) a.push_back(e.as_int());
                    rec.arrays.emplace(field, std::move(a));
                } else if (val.type == JVal::T::Bool) {
                    rec.ints.emplace(field, val.b ? 1 : 0);
                }
            }
        }
        raw.push_back(std::move(rec));
    }

    std::stable_sort(raw.begin(), raw.end(),
                     [](const Record& a, const Record& b) { return a.t_host_us < b.t_host_us; });
    records = std::move(raw);
}

}  // namespace dataset
}  // namespace depz
