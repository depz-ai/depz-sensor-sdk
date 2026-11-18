// The C SDK seen from the C++ codecs (internal, not installed): byte-span <->
// (pointer, length) and the C-struct -> C++-struct conversions the codec
// wrappers and the live layer share. Every codec in depz/*.hpp is a thin
// wrapper over depz_sensor_sdk.h; the hand-written decoding lives in C only.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>

#include <depz_sensor_sdk.h>

#include "depz/bno086.hpp"
#include "depz/dataset.hpp"
#include "depz/span.hpp"
#include "depz/tmf8829.hpp"
#include "depz/vl53l8.hpp"

namespace depz {
namespace detail {

// The C codecs take (pointer, length) and hand the pointer to memcpy, which
// takes no null pointer even for zero bytes: an empty span points at a byte.
inline const std::uint8_t* u8p(byte_span s) noexcept {
    static const std::uint8_t none = 0;
    return s.data() ? reinterpret_cast<const std::uint8_t*>(s.data()) : &none;
}

inline bytes to_bytes(const std::uint8_t* p, std::size_t n) {
    const auto* b = reinterpret_cast<const std::byte*>(p);
    return bytes(b, b + n);
}

// An encoder's fixed buffer as `bytes`: the length is the C codec's return
// value, never past the buffer (which gcc's -O2 bounds analysis then knows).
template <std::size_t N>
bytes to_bytes(const std::uint8_t (&buf)[N], std::size_t n) {
    return to_bytes(static_cast<const std::uint8_t*>(buf), n < N ? n : N);
}

// The same; a zero length is the C codec's refusal.
template <std::size_t N>
bytes packed(const std::uint8_t (&buf)[N], std::size_t n, const char* refused) {
    if (n == 0) throw std::invalid_argument(refused);
    return to_bytes(buf, n);
}

// A std::string handed to a C codec as a name: one holding an embedded NUL is
// not the name the C string would read as, so it is "unknown" before the call.
inline bool whole_cstr(const std::string& s) noexcept { return s.find('\0') == std::string::npos; }

// A C++ struct derived from its C struct (CPP-4), filled from the C value.
template <class T, class C>
T wrap(const C& c) {
    T t{};
    static_cast<C&>(t) = c;
    return t;
}

// C-struct <-> C++-struct conversions (defined in the codec .cpp of each type).
vl53l8::Vl53l8Frame vl53l8_frame_from_c(const depz_vl53l8_frame& c, const std::uint8_t* cnh,
                                        std::size_t cnh_len);
bno086::ReportType bno086_report_type_from_c(depz_bno_report_type t);
depz_bno_report_type bno086_report_type_to_c(bno086::ReportType t);
bno086::Report bno086_report_from_c(const depz_bno_report& c);
// The C view of a C++ report (`data` points into `r`), for the C scaling
// helpers and the record builder.
depz_bno_report bno086_report_to_c(const bno086::Report& r);
tmf8829::PixelResults tmf8829_results_from_c(const depz_tmf8829_results& c);
tmf8829::Measurement tmf8829_measurement_from_c(const depz_tmf8829_measurement& c);
// The C view of decoded results; the C arrays are sized by the sensor's
// limits, so a PixelResults past them is a std::invalid_argument.
std::unique_ptr<depz_tmf8829_results> tmf8829_results_to_c(const tmf8829::PixelResults& r);

// JSON values (dataset.cpp): a dataset::Value is a C++ tree; the C SDK
// parses, serializes, reads, writes and plays depz_json trees, so every
// crossing converts. json_to_c builds a new tree the caller owns (or hands
// to a C function that takes ownership); std::bad_alloc when C had no memory.
struct JsonFree {
    void operator()(depz_json* j) const noexcept { depz_json_free(j); }
};
using CJson = std::unique_ptr<depz_json, JsonFree>;
depz_json* json_to_c(const dataset::Value& v);
dataset::Value json_from_c(const depz_json* v);

// A malloc'd buffer of the C SDK (depz_gunzip, depz_json_dumps).
struct CFree {
    void operator()(void* p) const noexcept { std::free(p); }
};
template <class T>
using CBuf = std::unique_ptr<T, CFree>;

}  // namespace detail
}  // namespace depz
