//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_BUFFER_CIRCULAR_DYNAMIC_BUFFER_H
#define _LIBCPP___IO_BUFFER_CIRCULAR_DYNAMIC_BUFFER_H

#include <__config>
#include <__cstddef/size_t.h>
#include <__io_buffer/buffer.h>
#include <__io_buffer/buffer_pair.h>
#include <stdexcept>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// A ring buffer over caller-owned storage of fixed capacity (capacity() == max_size() == the size of the storage).
// The readable and the writable bytes may each wrap around the end of the storage, so data() is a const_buffer_pair
// and prepare() a mutable_buffer_pair; either buffer of a pair can be empty. consume() never moves bytes.
class circular_dynamic_buffer {
  unsigned char* __base_ = nullptr;
  size_t __capacity_     = 0;
  size_t __pos_          = 0; // index of the first readable byte
  size_t __in_           = 0; // number of readable bytes
  size_t __out_          = 0; // number of prepared (writable) bytes

  // The index of the byte that is __off bytes after the first readable one (__off <= capacity).
  _LIBCPP_HIDE_FROM_ABI size_t __index(size_t __off) const noexcept {
    size_t __i = __pos_ + __off; // cannot overflow: both are at most the capacity of an object
    return __i >= __capacity_ ? __i - __capacity_ : __i;
  }

public:
  using const_buffers_type   = const_buffer_pair;
  using mutable_buffers_type = mutable_buffer_pair;

  _LIBCPP_HIDE_FROM_ABI circular_dynamic_buffer() noexcept = default;

  // The first initial_size bytes of the storage are readable. Requires initial_size <= capacity.
  _LIBCPP_HIDE_FROM_ABI circular_dynamic_buffer(void* __storage, size_t __capacity, size_t __initial_size = 0) noexcept
      : __base_(static_cast<unsigned char*>(__storage)), __capacity_(__capacity), __in_(__initial_size) {}

  _LIBCPP_HIDE_FROM_ABI circular_dynamic_buffer(circular_dynamic_buffer&&) noexcept = default;
  _LIBCPP_HIDE_FROM_ABI circular_dynamic_buffer& operator=(circular_dynamic_buffer&&) noexcept = default;

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t size() const noexcept { return __in_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t max_size() const noexcept { return __capacity_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t capacity() const noexcept { return __capacity_; }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_buffers_type data() const noexcept {
    size_t __first = __capacity_ - __pos_ < __in_ ? __capacity_ - __pos_ : __in_;
    return const_buffers_type{const_buffer(__base_ + __pos_, __first), const_buffer(__base_, __in_ - __first)};
  }

  // Throws length_error if size() + n exceeds max_size().
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI mutable_buffers_type prepare(size_t __n) {
    if (__n > __capacity_ - __in_)
      std::__throw_length_error("circular_dynamic_buffer::prepare: size() + n exceeds max_size()");
    __out_ = __n;
    if (__n == 0)
      return mutable_buffers_type{};
    size_t __start = __index(__in_); // __capacity_ != 0 here, because __n <= __capacity_ - __in_ and __n != 0
    size_t __first = __capacity_ - __start < __n ? __capacity_ - __start : __n;
    return mutable_buffers_type{mutable_buffer(__base_ + __start, __first), mutable_buffer(__base_, __n - __first)};
  }

  _LIBCPP_HIDE_FROM_ABI void commit(size_t __n) noexcept {
    __in_ += __n < __out_ ? __n : __out_;
    __out_ = 0;
  }

  _LIBCPP_HIDE_FROM_ABI void consume(size_t __n) noexcept {
    size_t __m = __n < __in_ ? __n : __in_;
    __pos_     = __m == 0 ? __pos_ : __index(__m);
    __in_ -= __m;
    if (__in_ == 0)
      __pos_ = 0;
    __out_ = 0;
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

#endif // _LIBCPP___IO_BUFFER_CIRCULAR_DYNAMIC_BUFFER_H
