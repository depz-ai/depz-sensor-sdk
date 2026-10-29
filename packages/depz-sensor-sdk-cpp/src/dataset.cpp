#include "depz/dataset.hpp"
#include "depz/device.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace depz {
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
    if (type_ == Type::Double) return static_cast<std::int64_t>(d_);
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

namespace {

void put_utf8(std::string& out, std::uint32_t cp)
{
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

// A minimal recursive-descent JSON parser (objects, arrays, strings with
// escapes and surrogate pairs, integers as int64, other numbers as double).
class Parser {
public:
    explicit Parser(const std::string& t) : t_(t) {}

    Value parse()
    {
        skip_ws();
        Value v = value();
        skip_ws();
        if (pos_ != t_.size()) fail("trailing data");
        return v;
    }

private:
    const std::string& t_;
    std::size_t pos_ = 0;

    [[noreturn]] void fail(const std::string& m) { throw std::runtime_error("JSON: " + m); }
    int peek() const { return pos_ < t_.size() ? static_cast<unsigned char>(t_[pos_]) : -1; }
    void skip_ws()
    {
        while (pos_ < t_.size() && (t_[pos_] == ' ' || t_[pos_] == '\t' || t_[pos_] == '\n' || t_[pos_] == '\r')) ++pos_;
    }
    bool lit(const char* s)
    {
        const std::size_t n = std::strlen(s);
        if (t_.compare(pos_, n, s) != 0) return false;
        pos_ += n;
        return true;
    }
    Value value()
    {
        skip_ws();
        const int c = peek();
        if (c == '"') return Value(string());
        if (c == '{') return object();
        if (c == '[') return array();
        if (c == '-' || (c >= '0' && c <= '9')) return number();
        if (lit("true")) return Value(true);
        if (lit("false")) return Value(false);
        if (lit("null")) return Value();
        fail("unexpected character");
    }
    std::uint32_t hex4()
    {
        std::uint32_t v = 0;
        for (int k = 0; k < 4; ++k) {
            const int h = peek();
            ++pos_;
            v <<= 4;
            if (h >= '0' && h <= '9') v |= static_cast<std::uint32_t>(h - '0');
            else if (h >= 'a' && h <= 'f') v |= static_cast<std::uint32_t>(h - 'a' + 10);
            else if (h >= 'A' && h <= 'F') v |= static_cast<std::uint32_t>(h - 'A' + 10);
            else fail("bad \\u escape");
        }
        return v;
    }
    std::string string()
    {
        ++pos_;  // opening quote
        std::string out;
        for (;;) {
            const int c = peek();
            if (c < 0) fail("unterminated string");
            ++pos_;
            if (c == '"') return out;
            if (c < 0x20) fail("control character in string");
            if (c != '\\') {
                out.push_back(static_cast<char>(c));
                continue;
            }
            const int e = peek();
            if (e < 0) fail("bad escape");
            ++pos_;
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
                    std::uint32_t cp = hex4();
                    if (cp >= 0xD800 && cp < 0xDC00 && t_.compare(pos_, 2, "\\u") == 0) {
                        const std::size_t save = pos_;
                        pos_ += 2;
                        const std::uint32_t lo = hex4();
                        if (lo >= 0xDC00 && lo < 0xE000) cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                        else pos_ = save;
                    }
                    put_utf8(out, cp);
                    break;
                }
                default: fail("unknown escape");
            }
        }
    }
    // RFC 8259: -? (0 | [1-9][0-9]*) (. [0-9]+)? ([eE] [+-]? [0-9]+)?
    Value number()
    {
        const std::size_t start = pos_;
        bool real = false;
        if (peek() == '-') ++pos_;
        if (peek() == '0') ++pos_;
        else if (!digits()) fail("expected number");
        if (peek() == '.') {
            real = true;
            ++pos_;
            if (!digits()) fail("bad fraction");
        }
        if (peek() == 'e' || peek() == 'E') {
            real = true;
            ++pos_;
            if (peek() == '+' || peek() == '-') ++pos_;
            if (!digits()) fail("bad exponent");
        }
        const std::string num = t_.substr(start, pos_ - start);
        if (!real) {
            try {
                std::size_t used = 0;
                const long long ll = std::stoll(num, &used);
                if (used == num.size()) return Value(static_cast<std::int64_t>(ll));
            } catch (const std::out_of_range&) {
                // out of int64 range: a real
            }
        }
        return Value(std::strtod(num.c_str(), nullptr));
    }
    bool digits()  // skips ASCII digits; false when there was none
    {
        const std::size_t from = pos_;
        while (peek() >= '0' && peek() <= '9') ++pos_;
        return pos_ > from;
    }
    Value array()
    {
        ++pos_;
        Value v = Value::array();
        skip_ws();
        if (peek() == ']') {
            ++pos_;
            return v;
        }
        for (;;) {
            v.push(value());
            skip_ws();
            const int c = peek();
            ++pos_;
            if (c == ',') continue;
            if (c == ']') return v;
            fail("expected ',' or ']'");
        }
    }
    Value object()
    {
        ++pos_;
        Value v = Value::object();
        skip_ws();
        if (peek() == '}') {
            ++pos_;
            return v;
        }
        for (;;) {
            skip_ws();
            if (peek() != '"') fail("expected object key");
            std::string key = string();
            skip_ws();
            if (peek() != ':') fail("expected ':'");
            ++pos_;
            v.set(key, value());
            skip_ws();
            const int c = peek();
            ++pos_;
            if (c == ',') continue;
            if (c == '}') return v;
            fail("expected ',' or '}'");
        }
    }
};

void dump_string(std::string& o, const std::string& s)
{
    static const char* H = "0123456789abcdef";
    o.push_back('"');
    const std::size_t n = s.size();
    for (std::size_t i = 0; i < n;) {
        const unsigned c = static_cast<unsigned char>(s[i]);
        if (c == '"') { o += "\\\""; ++i; }
        else if (c == '\\') { o += "\\\\"; ++i; }
        else if (c == '\n') { o += "\\n"; ++i; }
        else if (c == '\r') { o += "\\r"; ++i; }
        else if (c == '\t') { o += "\\t"; ++i; }
        else if (c < 0x20 || c == 0x7F) {
            o += "\\u00";
            o.push_back(H[c >> 4]);
            o.push_back(H[c & 15]);
            ++i;
        } else if (c < 0x80) {
            o.push_back(static_cast<char>(c));
            ++i;
        } else {
            // one UTF-8 sequence; an invalid byte becomes U+FFFD
            std::uint32_t cp = 0xFFFD;
            std::size_t need = 0;
            if (c >= 0xC2 && c <= 0xDF) { need = 1; cp = c & 0x1F; }
            else if (c >= 0xE0 && c <= 0xEF) { need = 2; cp = c & 0x0F; }
            else if (c >= 0xF0 && c <= 0xF4) { need = 3; cp = c & 0x07; }
            std::size_t j = i + 1, k = 0;
            for (; k < need && j < n; ++k, ++j) {
                const unsigned b = static_cast<unsigned char>(s[j]);
                if ((b & 0xC0) != 0x80) break;
                cp = (cp << 6) | (b & 0x3F);
            }
            if (need == 0 || k < need) { cp = 0xFFFD; j = i + 1; }
            i = j;
            char buf[16];
            if (cp < 0x10000) {
                std::snprintf(buf, sizeof buf, "\\u%04x", static_cast<unsigned>(cp));
            } else {
                cp -= 0x10000;
                std::snprintf(buf, sizeof buf, "\\u%04x\\u%04x", static_cast<unsigned>(0xD800 | (cp >> 10)),
                              static_cast<unsigned>(0xDC00 | (cp & 0x3FF)));
            }
            o += buf;
        }
    }
    o.push_back('"');
}

// The shortest decimal that reads back to the same double, with a fraction
// mark (as Python's repr: 2.0 stays a real).
void dump_double(std::string& o, double d)
{
    if (std::isnan(d) || std::isinf(d)) {
        o += "null";
        return;
    }
    char b[40];
    for (int prec = 15; prec <= 17; ++prec) {
        std::snprintf(b, sizeof b, "%.*g", prec, d);
        if (std::strtod(b, nullptr) == d) break;
    }
    if (!std::strpbrk(b, ".eEn")) std::strcat(b, ".0");
    o += b;
}

void dump(std::string& o, const Value& v)
{
    switch (v.type()) {
        case Value::Type::Null: o += "null"; break;
        case Value::Type::Bool: o += v.as_bool() ? "true" : "false"; break;
        case Value::Type::Int: o += std::to_string(v.as_int()); break;
        case Value::Type::Double: dump_double(o, v.as_double()); break;
        case Value::Type::String: dump_string(o, v.as_string()); break;
        case Value::Type::Array: {
            o.push_back('[');
            bool first = true;
            for (const Value& e : v.items()) {
                if (!first) o.push_back(',');
                first = false;
                dump(o, e);
            }
            o.push_back(']');
            break;
        }
        case Value::Type::Object: {
            o.push_back('{');
            bool first = true;
            for (const auto& [k, e] : v.entries()) {
                if (!first) o.push_back(',');
                first = false;
                dump_string(o, k);
                o.push_back(':');
                dump(o, e);
            }
            o.push_back('}');
            break;
        }
    }
}

}  // namespace

std::string Value::to_json() const
{
    std::string o;
    dump(o, *this);
    return o;
}

Value Value::parse(const std::string& text) { return Parser(text).parse(); }

// ── gzip: a running CRC-32, inflate (zlib's puff), a stored-block writer ────

namespace {

std::uint32_t crc32_update(std::uint32_t crc, const std::uint8_t* p, std::size_t n)
{
    static const std::array<std::uint32_t, 256> table = [] {
        std::array<std::uint32_t, 256> t{};
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            t[i] = c;
        }
        return t;
    }();
    crc = ~crc;
    for (std::size_t i = 0; i < n; ++i) crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

constexpr int MAX_BITS = 15;

struct Huffman {
    std::array<std::uint16_t, MAX_BITS + 1> count{};
    std::vector<std::uint16_t> symbol;

    explicit Huffman(const std::uint8_t* lengths, std::size_t n) : symbol(n)
    {
        for (std::size_t i = 0; i < n; ++i) count[lengths[i]]++;
        int left = 1;
        for (int len = 1; len <= MAX_BITS; ++len) {
            left = left * 2 - count[len];
            if (left < 0) throw std::runtime_error("inflate: over-subscribed code");
        }
        std::array<std::uint16_t, MAX_BITS + 2> offs{};
        for (int len = 1; len <= MAX_BITS; ++len) offs[len + 1] = static_cast<std::uint16_t>(offs[len] + count[len]);
        for (std::size_t s = 0; s < n; ++s)
            if (lengths[s]) symbol[offs[lengths[s]]++] = static_cast<std::uint16_t>(s);
    }
};

constexpr std::uint16_t LEN_BASE[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
constexpr std::uint8_t LEN_EXTRA[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
constexpr std::uint16_t DIST_BASE[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
constexpr std::uint8_t DIST_EXTRA[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
constexpr std::uint8_t CL_ORDER[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

struct Inflate {
    const std::uint8_t* src;
    std::size_t pos, len;
    std::uint32_t bit_buf = 0, bit_cnt = 0;
    std::vector<std::uint8_t>& out;

    std::uint32_t bits(std::uint32_t need)
    {
        while (bit_cnt < need) {
            if (pos >= len) throw std::runtime_error("inflate: unexpected end of data");
            bit_buf |= static_cast<std::uint32_t>(src[pos++]) << bit_cnt;
            bit_cnt += 8;
        }
        const std::uint32_t v = bit_buf & ((1u << need) - 1);
        bit_buf >>= need;
        bit_cnt -= need;
        return v;
    }
    int decode(const Huffman& h)
    {
        int code = 0, first = 0, index = 0;
        for (int l = 1; l <= MAX_BITS; ++l) {
            code |= static_cast<int>(bits(1));
            const int count = h.count[l];
            if (code - count < first) return h.symbol[static_cast<std::size_t>(index + (code - first))];
            index += count;
            first = (first + count) << 1;
            code <<= 1;
        }
        throw std::runtime_error("inflate: invalid code");
    }
    void codes(const Huffman& lit, const Huffman& dist)
    {
        for (;;) {
            int sym = decode(lit);
            if (sym < 256) { out.push_back(static_cast<std::uint8_t>(sym)); continue; }
            if (sym == 256) return;
            sym -= 257;
            if (sym >= 29) throw std::runtime_error("inflate: invalid length symbol");
            const std::size_t n = LEN_BASE[sym] + bits(LEN_EXTRA[sym]);
            const int d = decode(dist);
            if (d >= 30) throw std::runtime_error("inflate: invalid distance symbol");
            const std::size_t back = DIST_BASE[d] + bits(DIST_EXTRA[d]);
            if (back > out.size()) throw std::runtime_error("inflate: distance too far back");
            const std::size_t from = out.size() - back;
            for (std::size_t k = 0; k < n; ++k) out.push_back(out[from + k]);
        }
    }
    void stored()
    {
        bit_buf = 0;
        bit_cnt = 0;
        if (pos + 4 > len) throw std::runtime_error("inflate: truncated stored block");
        const unsigned n = src[pos] | (src[pos + 1] << 8), nn = src[pos + 2] | (src[pos + 3] << 8);
        if (n != (~nn & 0xFFFF)) throw std::runtime_error("inflate: stored length check failed");
        pos += 4;
        if (pos + n > len) throw std::runtime_error("inflate: truncated stored block");
        out.insert(out.end(), src + pos, src + pos + n);
        pos += n;
    }
    void fixed()
    {
        std::uint8_t lengths[288];
        std::memset(lengths, 8, 144);
        std::memset(lengths + 144, 9, 112);
        std::memset(lengths + 256, 7, 24);
        std::memset(lengths + 280, 8, 8);
        std::uint8_t dl[30];
        std::memset(dl, 5, sizeof dl);
        codes(Huffman(lengths, 288), Huffman(dl, 30));
    }
    void dynamic()
    {
        const std::size_t nlen = bits(5) + 257, ndist = bits(5) + 1, ncode = bits(4) + 4;
        if (nlen > 286 || ndist > 30) throw std::runtime_error("inflate: bad code counts");
        std::uint8_t cl[19] = {};
        for (std::size_t i = 0; i < ncode; ++i) cl[CL_ORDER[i]] = static_cast<std::uint8_t>(bits(3));
        const Huffman clc(cl, 19);
        std::uint8_t lengths[320] = {};
        for (std::size_t i = 0; i < nlen + ndist;) {
            const int sym = decode(clc);
            std::uint8_t value = 0;
            std::size_t repeat = 1;
            if (sym < 16) value = static_cast<std::uint8_t>(sym);
            else if (sym == 16) {
                if (i == 0) throw std::runtime_error("inflate: repeat with no length");
                value = lengths[i - 1];
                repeat = 3 + bits(2);
            } else if (sym == 17) repeat = 3 + bits(3);
            else repeat = 11 + bits(7);
            if (i + repeat > nlen + ndist) throw std::runtime_error("inflate: too many lengths");
            std::memset(lengths + i, value, repeat);
            i += repeat;
        }
        if (lengths[256] == 0) throw std::runtime_error("inflate: no end-of-block code");
        codes(Huffman(lengths, nlen), Huffman(lengths + nlen, ndist));
    }
    void run()
    {
        for (;;) {
            const std::uint32_t last = bits(1), type = bits(2);
            if (type == 0) stored();
            else if (type == 1) fixed();
            else if (type == 2) dynamic();
            else throw std::runtime_error("inflate: invalid block type");
            if (last) return;
        }
    }
};

std::uint32_t le32(const std::uint8_t* p)
{
    return static_cast<std::uint32_t>(p[0]) | static_cast<std::uint32_t>(p[1]) << 8 | static_cast<std::uint32_t>(p[2]) << 16 |
           static_cast<std::uint32_t>(p[3]) << 24;
}

}  // namespace

bool is_gzip(byte_span data)
{
    return data.size() >= 2 && std::to_integer<unsigned>(data[0]) == 0x1F && std::to_integer<unsigned>(data[1]) == 0x8B;
}

bytes gunzip(byte_span data)
{
    const auto* src = reinterpret_cast<const std::uint8_t*>(data.data());
    const std::size_t len = data.size();
    std::vector<std::uint8_t> out;
    std::size_t pos = 0;
    while (pos < len) {
        if (pos + 10 > len || src[pos] != 0x1F || src[pos + 1] != 0x8B || src[pos + 2] != 8)
            throw std::runtime_error("gzip: not a deflate member");
        const unsigned flags = src[pos + 3];
        pos += 10;
        if (flags & 0x04) {
            if (pos + 2 > len) throw std::runtime_error("gzip: truncated extra field");
            pos += 2 + static_cast<std::size_t>(src[pos] | (src[pos + 1] << 8));
        }
        for (unsigned bit = 0x08; bit <= 0x10; bit <<= 1) {
            if (flags & bit) {
                while (pos < len && src[pos]) ++pos;
                if (pos >= len) throw std::runtime_error("gzip: truncated name");
                ++pos;
            }
        }
        if (flags & 0x02) pos += 2;
        const std::size_t start = out.size();
        Inflate inf{src, pos, len, 0, 0, out};
        inf.run();
        pos = inf.pos;
        if (pos + 8 > len) throw std::runtime_error("gzip: truncated trailer");
        if (le32(src + pos) != crc32_update(0, out.data() + start, out.size() - start) ||
            le32(src + pos + 4) != static_cast<std::uint32_t>(out.size() - start))
            throw std::runtime_error("gzip: CRC or length mismatch");
        pos += 8;
    }
    const auto* b = reinterpret_cast<const std::byte*>(out.data());
    return bytes(b, b + out.size());
}

// ── Reader ───────────────────────────────────────────────────────────────────

namespace {

std::string get_str(const Value& o, const char* key)
{
    const Value* v = o.find(key);
    return v && v->is_string() ? v->as_string() : std::string();
}

// A damaged line is the caller's bad argument (contract 07 §3): the whole
// file is refused and the message names the line — never skipped or coerced.
[[noreturn]] void bad_line(std::size_t lineno, const std::string& what)
{
    throw std::invalid_argument("dataset: line " + std::to_string(lineno) + ": " + what);
}

// `line` as the JSON object a dataset line must be.
Value line_object(std::size_t lineno, const std::string& line, const char* role)
{
    Value v;
    try {
        v = Value::parse(line);
    } catch (const std::runtime_error& e) {
        bad_line(lineno, std::string(role) + " is not JSON (" + e.what() + ")");
    }
    if (!v.is_object()) bad_line(lineno, std::string(role) + " is not a JSON object");
    return v;
}

}  // namespace

Reader::Reader(const std::string& content)
{
    std::string plain;
    const std::string* text = &content;
    if (content.size() >= 2 && static_cast<unsigned char>(content[0]) == 0x1F && static_cast<unsigned char>(content[1]) == 0x8B) {
        const bytes b = gunzip(byte_span(reinterpret_cast<const std::byte*>(content.data()), content.size()));
        plain.assign(reinterpret_cast<const char*>(b.data()), b.size());
        text = &plain;
    }
    // the lines (contract 08 «Line rules»: LF alone ends one), a final "\n" opening no last one
    std::vector<std::string> lines;
    for (std::size_t i = 0; i < text->size();) {
        std::size_t nl = text->find('\n', i);
        if (nl == std::string::npos) nl = text->size();
        lines.push_back(text->substr(i, nl - i));
        i = nl + 1;
    }
    if (lines.empty()) throw std::invalid_argument("dataset: empty file (no header line)");

    // line 1 is the header, blank or not (the Python reference's readline)
    header = line_object(1, lines[0], "header");
    const Value* sc = header.find("schema");
    if (!sc || !sc->is_string() || sc->as_string().compare(0, std::strlen(SCHEMA_PREFIX), SCHEMA_PREFIX) != 0)
        bad_line(1, "header \"schema\" is not a \"depz.dataset/...\" string");
    schema = sc->as_string();
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
    // record lines: `d` and `k` strings, `t` an integer, `v` an object;
    // blank lines skipped; an unknown `k` kept (players ignore it)
    for (std::size_t k = 1; k < lines.size(); ++k) {
        const std::size_t lineno = k + 1;
        if (lines[k].find_first_not_of(" \t\r") == std::string::npos) continue;  // JSON whitespace only
        const Value ev = line_object(lineno, lines[k], "record");
        const Value* d = ev.find("d");
        const Value* t = ev.find("t");
        const Value* kind = ev.find("k");
        const Value* v = ev.find("v");
        if (!d || !d->is_string()) bad_line(lineno, "\"d\" must be a device id string");
        if (!t || !t->is_int()) bad_line(lineno, "\"t\" must be an integer (host microseconds)");
        if (!kind || !kind->is_string()) bad_line(lineno, "\"k\" must be a record kind string");
        if (!v || !v->is_object()) bad_line(lineno, "\"v\" must be an object");
        Record rec;
        rec.device_id = d->as_string();
        rec.t_host_us = t->as_int();
        rec.kind = kind->as_string();
        rec.value = *v;
        records.push_back(std::move(rec));
    }
    std::stable_sort(records.begin(), records.end(), [](const Record& a, const Record& b) { return a.t_host_us < b.t_host_us; });
}

Reader Reader::open(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + path);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return Reader(content);
}

// ── Writer ───────────────────────────────────────────────────────────────────

namespace {

constexpr std::size_t STORED_MAX = 0xFFFF;

std::string utc_now()
{
    const std::time_t now = std::time(nullptr);
    std::tm tmv{};
#ifdef _WIN32
    gmtime_s(&tmv, &now);
#else
    gmtime_r(&now, &tmv);
#endif
    char out[32];
    std::strftime(out, sizeof out, "%Y-%m-%dT%H:%M:%SZ", &tmv);
    return out;
}

}  // namespace

struct Writer::Impl {
    std::mutex lock;
    std::ofstream file;
    bool gz = false, open = false;
    std::vector<std::uint8_t> buf;  // the pending stored block
    std::uint32_t crc = 0, size = 0;
    std::uint64_t records = 0;

    void raw(const char* p, std::size_t n)
    {
        file.write(p, static_cast<std::streamsize>(n));
        if (!file) throw std::runtime_error("dataset write failed");
    }
    void block(bool last)
    {
        const std::uint16_t n = static_cast<std::uint16_t>(buf.size());
        const char h[5] = {static_cast<char>(last ? 1 : 0), static_cast<char>(n & 0xFF), static_cast<char>(n >> 8),
                           static_cast<char>(~n & 0xFF), static_cast<char>((~n >> 8) & 0xFF)};
        raw(h, 5);
        if (!buf.empty()) raw(reinterpret_cast<const char*>(buf.data()), buf.size());
        buf.clear();
    }
    void put(const std::string& s)
    {
        if (!gz) {
            raw(s.data(), s.size());
            return;
        }
        const auto* p = reinterpret_cast<const std::uint8_t*>(s.data());
        std::size_t n = s.size();
        while (n) {
            const std::size_t k = std::min(n, STORED_MAX - buf.size());
            buf.insert(buf.end(), p, p + k);
            crc = crc32_update(crc, p, k);
            size += static_cast<std::uint32_t>(k);
            p += k;
            n -= k;
            if (buf.size() == STORED_MAX) block(false);
        }
    }
    void finish()
    {
        if (gz) {
            block(true);
            char t[8];
            for (int i = 0; i < 4; ++i) {
                t[i] = static_cast<char>(crc >> (8 * i));
                t[4 + i] = static_cast<char>(size >> (8 * i));
            }
            raw(t, 8);
        }
        file.flush();
        if (!file) throw std::runtime_error("dataset flush failed");
        file.close();
    }
};

Writer::Writer(const std::string& path, const Value::Object& devices, const std::string& note) : impl_(new Impl)
{
    impl_->file.open(path, std::ios::binary | std::ios::trunc);
    if (!impl_->file) throw std::runtime_error("cannot create " + path);
    impl_->open = true;
    const std::size_t n = path.size();
    impl_->gz = n >= 3 && (path.compare(n - 3, 3, ".gz") == 0 || path.compare(n - 3, 3, ".GZ") == 0);
    if (impl_->gz) {
        static const char head[10] = {0x1F, static_cast<char>(0x8B), 8, 0, 0, 0, 0, 0, 0, static_cast<char>(0xFF)};
        impl_->raw(head, 10);
    }
    Value header = Value::object();
    header.set("schema", SCHEMA);
    header.set("created_utc", utc_now());
    header.set("devices", Value(devices));
    if (!note.empty()) header.set("note", note);
    impl_->put(header.to_json() + "\n");
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
    Value rec = Value::object();
    rec.set("d", device_id);
    rec.set("t", t_host_us);
    rec.set("k", kind);
    rec.set("v", value);
    const std::string line = rec.to_json() + "\n";
    std::lock_guard<std::mutex> g(impl_->lock);
    if (!impl_->open) throw ArgumentError(kErrArg, "dataset Writer is closed");
    impl_->put(line);
    impl_->records++;
}

std::uint64_t Writer::records_written() const
{
    std::lock_guard<std::mutex> g(impl_->lock);
    return impl_->records;
}

void Writer::close()
{
    std::lock_guard<std::mutex> g(impl_->lock);
    if (!impl_->open) return;
    impl_->open = false;
    impl_->finish();
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

struct DatasetPlayer::Impl : std::enable_shared_from_this<Impl> {
    using RecordCb = std::shared_ptr<std::function<void(const Record&)>>;
    using StateCb = std::shared_ptr<std::function<void(PlayerState)>>;

    const std::vector<Record> recs;
    const std::function<double()> clock;
    const bool threaded;

    std::mutex lock;
    std::condition_variable changed;
    PlayerState state = PlayerState::Idle;
    double speed;
    std::size_t idx = 0;  // the next record to deliver
    double anchor_wall = 0, anchor_t = 0;
    bool closed = false;
    std::uint64_t generation = 0;  // bumped on every change a waiting player thread must see
    std::optional<std::thread::id> delivering;  // the thread running record callbacks now
    std::vector<RecordCb> record_cbs;
    std::vector<StateCb> state_cbs;
    std::atomic<std::uint64_t> callback_errors{0};
    std::thread thread;

    Impl(const Reader& reader, PlayerOptions o)
        : recs(reader.records),
          clock(o.clock ? std::move(o.clock) : std::function<double()>([start = std::chrono::steady_clock::now()] {
              return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
          })),
          threaded(o.threaded),
          speed(check_speed(o.speed)) {}

    static double check_speed(double v)
    {
        if (std::isnan(v) || std::isinf(v) || v < 0) throw std::invalid_argument("speed must be a finite number >= 0");
        return v;
    }

    void bump()
    {
        ++generation;
        changed.notify_all();
    }

    // The media clock restarts at `now` from the last delivered record's t (the first's when none was).
    void anchor(double now)
    {
        anchor_wall = now;
        anchor_t = recs.empty() ? 0.0 : static_cast<double>(recs[idx > 0 ? idx - 1 : 0].t_host_us);
    }

    double media_now(double now) const
    {
        if (speed <= 0) return static_cast<double>(recs[idx > 0 ? idx - 1 : 0].t_host_us);
        return anchor_t + (now - anchor_wall) * 1e6 * speed;
    }

    void set_state(PlayerState s, std::vector<PlayerState>& changes)
    {
        state = s;
        changes.push_back(s);
    }

    // Unlock, then report the state changes on this thread.
    void notify(std::unique_lock<std::mutex>& g, const std::vector<PlayerState>& changes)
    {
        if (changes.empty()) return;
        std::vector<StateCb> cbs = state_cbs;
        g.unlock();
        for (PlayerState s : changes) deliver(cbs, s);
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

    enum class Step { Delivered, Wait, Idle };

    // Deliver the next record if it is due.
    Step step(double& wait, std::uint64_t& gen)
    {
        std::vector<PlayerState> changes;
        std::unique_lock<std::mutex> g(lock);
        gen = generation;
        if (state != PlayerState::Playing) return Step::Idle;
        if (idx >= recs.size()) {
            set_state(PlayerState::Done, changes);
            bump();
            gen = generation;
            notify(g, changes);
            return Step::Idle;
        }
        const Record& rec = recs[idx];
        if (speed > 0) {
            const double now = clock();
            const double due = anchor_wall + (static_cast<double>(rec.t_host_us) - anchor_t) / 1e6 / speed;
            if (due > now) {
                wait = due - now;
                return Step::Wait;
            }
        }
        idx++;
        delivering = std::this_thread::get_id();
        std::vector<RecordCb> cbs = record_cbs;
        g.unlock();
        deliver(cbs, rec);
        g.lock();
        delivering.reset();
        changed.notify_all();
        if (state == PlayerState::Playing && idx >= recs.size()) {
            set_state(PlayerState::Done, changes);
            bump();
        }
        notify(g, changes);
        return Step::Delivered;
    }

    void run()
    {
        for (;;) {
            double wait = 0;
            std::uint64_t gen = 0;
            switch (step(wait, gen)) {
                case Step::Delivered: break;
                case Step::Wait: {
                    std::unique_lock<std::mutex> g(lock);
                    if (closed) return;
                    if (generation == gen) changed.wait_for(g, std::chrono::duration<double>(wait));
                    break;
                }
                case Step::Idle: {
                    std::unique_lock<std::mutex> g(lock);
                    changed.wait(g, [&] { return closed || generation != gen; });
                    if (closed) return;
                    break;
                }
            }
        }
    }
};

DatasetPlayer::DatasetPlayer(const Reader& reader, PlayerOptions options) : impl_(std::make_shared<Impl>(reader, std::move(options))) {}

DatasetPlayer::~DatasetPlayer() { close(); }

PlayerState DatasetPlayer::state() const
{
    std::lock_guard<std::mutex> g(impl_->lock);
    return impl_->state;
}

std::int64_t DatasetPlayer::duration_us() const
{
    if (impl_->recs.size() < 2) return 0;
    return impl_->recs.back().t_host_us - impl_->recs.front().t_host_us;
}

std::int64_t DatasetPlayer::position_us() const
{
    std::lock_guard<std::mutex> g(impl_->lock);
    const auto& recs = impl_->recs;
    if (recs.empty() || impl_->idx == 0) return 0;
    return recs[std::min(impl_->idx, recs.size()) - 1].t_host_us - recs[0].t_host_us;
}

double DatasetPlayer::speed() const
{
    std::lock_guard<std::mutex> g(impl_->lock);
    return impl_->speed;
}

void DatasetPlayer::set_speed(double speed)
{
    const double s = Impl::check_speed(speed);
    std::lock_guard<std::mutex> g(impl_->lock);
    if (impl_->state == PlayerState::Playing) {
        const double now = impl_->clock();
        impl_->anchor_t = impl_->media_now(now);
        impl_->anchor_wall = now;
    }
    impl_->speed = s;
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
    std::vector<PlayerState> changes;
    std::unique_lock<std::mutex> g(impl_->lock);
    if (impl_->closed) throw ArgumentError(kErrArg, "DatasetPlayer is closed");
    if (impl_->state == PlayerState::Playing) return;
    if (impl_->idx >= impl_->recs.size()) impl_->idx = 0;
    impl_->anchor(impl_->clock());
    impl_->set_state(PlayerState::Playing, changes);
    if (impl_->recs.empty()) {
        impl_->set_state(PlayerState::Done, changes);
    } else if (impl_->threaded && !impl_->thread.joinable()) {
        std::shared_ptr<Impl> self = impl_;
        impl_->thread = std::thread([self] { self->run(); });
    }
    impl_->bump();
    impl_->notify(g, changes);
}

void DatasetPlayer::pause()
{
    std::vector<PlayerState> changes;
    std::unique_lock<std::mutex> g(impl_->lock);
    if (impl_->state != PlayerState::Playing) return;
    impl_->set_state(PlayerState::Paused, changes);
    impl_->bump();
    // Once pause returns no record callback runs: wait out one on another thread.
    const auto me = std::this_thread::get_id();
    impl_->changed.wait(g, [&] { return !impl_->delivering || *impl_->delivering == me; });
    impl_->notify(g, changes);
}

void DatasetPlayer::seek_us(double offset_us)
{
    if (std::isnan(offset_us)) throw std::invalid_argument("seek_us: offset must be a number");
    std::vector<PlayerState> changes;
    std::unique_lock<std::mutex> g(impl_->lock);
    if (impl_->closed) throw ArgumentError(kErrArg, "DatasetPlayer is closed");
    const auto& recs = impl_->recs;
    if (recs.empty()) return;
    const double target = static_cast<double>(recs[0].t_host_us) + offset_us;
    impl_->idx = static_cast<std::size_t>(
        std::lower_bound(recs.begin(), recs.end(), target, [](const Record& r, double t) { return static_cast<double>(r.t_host_us) < t; }) -
        recs.begin());
    if (impl_->state == PlayerState::Playing) {
        if (impl_->idx >= recs.size()) impl_->set_state(PlayerState::Done, changes);
        else impl_->anchor(impl_->clock());
    }
    impl_->bump();
    impl_->notify(g, changes);
}

std::optional<double> DatasetPlayer::tick()
{
    for (;;) {
        double wait = 0;
        std::uint64_t gen = 0;
        switch (impl_->step(wait, gen)) {
            case Impl::Step::Delivered: break;
            case Impl::Step::Wait: return wait;
            case Impl::Step::Idle: return std::nullopt;
        }
    }
}

bool DatasetPlayer::wait(std::optional<std::chrono::milliseconds> timeout)
{
    if (!impl_->threaded) throw ArgumentError(kErrArg, "wait() needs threaded = true; drive tick() instead");
    std::unique_lock<std::mutex> g(impl_->lock);
    const auto not_playing = [&] { return impl_->state != PlayerState::Playing; };
    if (!timeout) {
        impl_->changed.wait(g, not_playing);
        return true;
    }
    return impl_->changed.wait_for(g, *timeout, not_playing);
}

void DatasetPlayer::close()
{
    std::vector<PlayerState> changes;
    std::thread t;
    {
        std::unique_lock<std::mutex> g(impl_->lock);
        if (impl_->closed) return;
        if (impl_->state == PlayerState::Playing) impl_->set_state(PlayerState::Paused, changes);
        impl_->closed = true;
        impl_->bump();
        t = std::move(impl_->thread);
        impl_->notify(g, changes);
    }
    if (t.joinable()) {
        if (t.get_id() == std::this_thread::get_id()) t.detach();
        else t.join();
    }
}

}  // namespace dataset
}  // namespace depz
