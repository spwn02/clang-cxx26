//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_BUFFER_BUFFER_H
#define _LIBCPP___IO_BUFFER_BUFFER_H

#include <__config>
#include <__cstddef/size_t.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.4], B1004 [4], N4771 [buffer.mutable] and [buffer.const]: a buffer is a pointer and a size that
// describe a contiguous region of bytes. It does not own the region and says nothing about its contents, which is why
// the pointer is void*. A buffer is deliberately not a std::ranges::range; the buffer sequence customization points
// (buffer_sequence_begin/end) treat it as a sequence of one element.
class mutable_buffer {
  void* __data_ = nullptr;
  size_t __size_ = 0;

public:
  _LIBCPP_HIDE_FROM_ABI constexpr mutable_buffer() noexcept = default;
  _LIBCPP_HIDE_FROM_ABI constexpr mutable_buffer(void* __p, size_t __n) noexcept : __data_(__p), __size_(__n) {}

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr void* data() const noexcept { return __data_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr size_t size() const noexcept { return __size_; }

  // Skips min(n, size()) bytes at the front.
  _LIBCPP_HIDE_FROM_ABI constexpr mutable_buffer& operator+=(size_t __n) noexcept {
    size_t __k = __n < __size_ ? __n : __size_;
    __data_    = static_cast<char*>(__data_) + __k;
    __size_ -= __k;
    return *this;
  }
};

class const_buffer {
  const void* __data_ = nullptr;
  size_t __size_      = 0;

public:
  _LIBCPP_HIDE_FROM_ABI constexpr const_buffer() noexcept = default;
  _LIBCPP_HIDE_FROM_ABI constexpr const_buffer(const void* __p, size_t __n) noexcept : __data_(__p), __size_(__n) {}
  _LIBCPP_HIDE_FROM_ABI constexpr const_buffer(const mutable_buffer& __b) noexcept
      : __data_(__b.data()), __size_(__b.size()) {}

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const void* data() const noexcept { return __data_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr size_t size() const noexcept { return __size_; }

  _LIBCPP_HIDE_FROM_ABI constexpr const_buffer& operator+=(size_t __n) noexcept {
    size_t __k = __n < __size_ ? __n : __size_;
    __data_    = static_cast<const char*>(__data_) + __k;
    __size_ -= __k;
    return *this;
  }
};

// N4771 [buffer.arithmetic]: the result skips min(n, b.size()) bytes.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr mutable_buffer operator+(const mutable_buffer& __b, size_t __n) noexcept {
  mutable_buffer __r = __b;
  __r += __n;
  return __r;
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr mutable_buffer operator+(size_t __n, const mutable_buffer& __b) noexcept {
  return __b + __n;
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const_buffer operator+(const const_buffer& __b, size_t __n) noexcept {
  const_buffer __r = __b;
  __r += __n;
  return __r;
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const_buffer operator+(size_t __n, const const_buffer& __b) noexcept {
  return __b + __n;
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

#endif // _LIBCPP___IO_BUFFER_BUFFER_H
