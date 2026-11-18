// The C live layer (depz_sensor_io.h) seen from the C++ live layer
// (internal, not installed): the error mapping and the C++ -> C conversion
// device.cpp and recorder.cpp share.
#pragma once

#include <depz_sensor_io.h>

#include "depz/device.hpp"

namespace depz {
namespace detail {

// Throw the exception class of a C error code with depz_last_error() as the
// message (contract 07 §3: one class per failure kind).
[[noreturn]] void throw_rc(int rc);
inline void check(int rc) {
    if (rc != DEPZ_OK) throw_rc(rc);
}

depz_vl53l8_cnh_setup cnh_to_c(const CnhSetup& s);

}  // namespace detail
}  // namespace depz
