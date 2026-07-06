// Minimal recursive-descent JSON parser for the vector-driven test runner.
// Self-contained (no external dependency). Supports objects, arrays, strings
// with standard escapes, numbers (stored as int64 and double), true/false/null.
#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace testjson {

struct Value;
using Object = std::map<std::string, Value>;
using Array = std::vector<Value>;

enum class Type { Null, Bool, Int, Double, String, Array, Object };

struct Value {
    Type type = Type::Null;
    bool b = false;
    std::int64_t i = 0;
    double d = 0.0;
    std::string s;
    Array arr;
    Object obj;

    bool is_null() const { return type == Type::Null; }
    bool as_bool() const { return b; }
    std::int64_t as_int() const { return type == Type::Double ? static_cast<std::int64_t>(d) : i; }
    std::uint64_t as_u64() const { return static_cast<std::uint64_t>(as_int()); }
    double as_double() const { return type == Type::Double ? d : static_cast<double>(i); }
    const std::string& as_string() const { return s; }
    const Array& as_array() const { return arr; }
    const Object& as_object() const { return obj; }

    const Value& at(const std::string& key) const {
        auto it = obj.find(key);
        if (it == obj.end()) throw std::runtime_error("missing key: " + key);
        return it->second;
    }
    bool has(const std::string& key) const { return obj.find(key) != obj.end(); }
};

class Parser {
public:
    explicit Parser(const std::string& text) : t_(text) {}

    Value parse() {
        skip_ws();
        Value v = parse_value();
        skip_ws();
        if (pos_ != t_.size()) throw std::runtime_error("trailing data in JSON");
        return v;
    }

private:
    const std::string& t_;
    std::size_t pos_ = 0;

    [[noreturn]] void fail(const std::string& msg) {
        throw std::runtime_error("JSON parse error at " + std::to_string(pos_) + ": " + msg);
    }

    char peek() { return pos_ < t_.size() ? t_[pos_] : '\0'; }
    char get() { return t_[pos_++]; }

    void skip_ws() {
        while (pos_ < t_.size()) {
            char c = t_[pos_];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++pos_;
            } else {
                break;
            }
        }
    }

    Value parse_value() {
        skip_ws();
        char c = peek();
        switch (c) {
            case '{': return parse_object();
            case '[': return parse_array();
            case '"': {
                Value v;
                v.type = Type::String;
                v.s = parse_string();
                return v;
            }
            case 't':
            case 'f': return parse_bool();
            case 'n': return parse_null();
            default: return parse_number();
        }
    }

    Value parse_object() {
        Value v;
        v.type = Type::Object;
        get();  // {
        skip_ws();
        if (peek() == '}') {
            get();
            return v;
        }
        while (true) {
            skip_ws();
            if (peek() != '"') fail("expected string key");
            std::string key = parse_string();
            skip_ws();
            if (get() != ':') fail("expected ':'");
            v.obj[key] = parse_value();
            skip_ws();
            char c = get();
            if (c == ',') continue;
            if (c == '}') break;
            fail("expected ',' or '}'");
        }
        return v;
    }

    Value parse_array() {
        Value v;
        v.type = Type::Array;
        get();  // [
        skip_ws();
        if (peek() == ']') {
            get();
            return v;
        }
        while (true) {
            v.arr.push_back(parse_value());
            skip_ws();
            char c = get();
            if (c == ',') continue;
            if (c == ']') break;
            fail("expected ',' or ']'");
        }
        return v;
    }

    std::string parse_string() {
        get();  // opening quote
        std::string out;
        while (true) {
            if (pos_ >= t_.size()) fail("unterminated string");
            char c = get();
            if (c == '"') break;
            if (c == '\\') {
                char e = get();
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
                        unsigned code = 0;
                        for (int k = 0; k < 4; ++k) {
                            char h = get();
                            code <<= 4;
                            if (h >= '0' && h <= '9') code |= static_cast<unsigned>(h - '0');
                            else if (h >= 'a' && h <= 'f') code |= static_cast<unsigned>(h - 'a' + 10);
                            else if (h >= 'A' && h <= 'F') code |= static_cast<unsigned>(h - 'A' + 10);
                            else fail("bad \\u escape");
                        }
                        // Encode as UTF-8 (BMP only; sufficient for these vectors).
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
                    default: fail("bad escape");
                }
            } else {
                out.push_back(c);
            }
        }
        return out;
    }

    Value parse_bool() {
        Value v;
        v.type = Type::Bool;
        if (t_.compare(pos_, 4, "true") == 0) {
            v.b = true;
            pos_ += 4;
        } else if (t_.compare(pos_, 5, "false") == 0) {
            v.b = false;
            pos_ += 5;
        } else {
            fail("invalid literal");
        }
        return v;
    }

    Value parse_null() {
        Value v;
        v.type = Type::Null;
        if (t_.compare(pos_, 4, "null") == 0) {
            pos_ += 4;
        } else {
            fail("invalid literal");
        }
        return v;
    }

    Value parse_number() {
        std::size_t start = pos_;
        bool is_double = false;
        if (peek() == '-') get();
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
        if (num.empty()) fail("invalid number");
        Value v;
        if (is_double) {
            v.type = Type::Double;
            v.d = std::stod(num);
        } else {
            v.type = Type::Int;
            v.i = std::stoll(num);
        }
        return v;
    }
};

inline Value parse(const std::string& text) { return Parser(text).parse(); }

}  // namespace testjson
