// The exception classes of the C++ SDK (contract 07 §3). Header-only and free
// of the C SDK, so the dataset layer, built without the live layer, throws
// the same classes as depz/device.hpp. `code()` is the C SDK's depz_err value.
#pragma once

#include <stdexcept>
#include <string>

namespace depz {

// DEPZ_E_ARG of depz_sensor_sdk.h: a bad argument, or a call not valid in the
// current state (static_assert'ed equal where the C header is included).
constexpr int kErrArg = -1;

// Base of every SDK error; `code()` is the C SDK's depz_err value.
class Error : public std::runtime_error {
public:
    Error(int code, const std::string& what) : std::runtime_error(what), code_(code) {}
    int code() const noexcept { return code_; }

private:
    int code_;
};

// invalid_argument and invalid_state (DEPZ_E_ARG).
class ArgumentError : public Error { using Error::Error; };

}  // namespace depz
