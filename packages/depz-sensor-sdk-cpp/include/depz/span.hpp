// Minimal non-owning contiguous view (std::span-like) for C++17.
// The DEPZ C++ SDK targets C++17; std::span is C++20, so we vendor a tiny
// read-mostly view with just the surface the SDK needs.
#pragma once

#include <cstddef>
#include <array>
#include <vector>
#include <type_traits>

namespace depz {

template <class T>
class span {
public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = std::size_t;
    using pointer = T*;
    using reference = T&;
    using iterator = T*;

    constexpr span() noexcept : data_(nullptr), size_(0) {}
    constexpr span(T* ptr, size_type n) noexcept : data_(ptr), size_(n) {}

    template <class U, std::size_t N,
              class = std::enable_if_t<std::is_convertible_v<U (*)[], T (*)[]>>>
    constexpr span(std::array<U, N>& a) noexcept : data_(a.data()), size_(N) {}

    template <class U, std::size_t N,
              class = std::enable_if_t<std::is_convertible_v<const U (*)[], T (*)[]>>>
    constexpr span(const std::array<U, N>& a) noexcept : data_(a.data()), size_(N) {}

    template <class U, class = std::enable_if_t<std::is_convertible_v<U (*)[], T (*)[]>>>
    span(std::vector<U>& v) noexcept : data_(v.data()), size_(v.size()) {}

    template <class U, class = std::enable_if_t<std::is_convertible_v<const U (*)[], T (*)[]>>>
    span(const std::vector<U>& v) noexcept : data_(v.data()), size_(v.size()) {}

    constexpr T* data() const noexcept { return data_; }
    constexpr size_type size() const noexcept { return size_; }
    constexpr bool empty() const noexcept { return size_ == 0; }
    constexpr reference operator[](size_type i) const { return data_[i]; }
    constexpr iterator begin() const noexcept { return data_; }
    constexpr iterator end() const noexcept { return data_ + size_; }

    constexpr span<T> subspan(size_type off) const {
        return span<T>(data_ + off, size_ - off);
    }
    constexpr span<T> subspan(size_type off, size_type count) const {
        return span<T>(data_ + off, count);
    }
    constexpr span<T> first(size_type count) const { return span<T>(data_, count); }

private:
    T* data_;
    size_type size_;
};

using bytes = std::vector<std::byte>;
using byte_span = span<const std::byte>;

// Convenience: reinterpret a byte-ish container as a const byte span.
inline byte_span as_bytes(const std::vector<std::byte>& v) noexcept {
    return byte_span(v.data(), v.size());
}

}  // namespace depz
