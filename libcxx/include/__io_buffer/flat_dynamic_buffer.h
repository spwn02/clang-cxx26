//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_BUFFER_FLAT_DYNAMIC_BUFFER_H
#define _LIBCPP___IO_BUFFER_FLAT_DYNAMIC_BUFFER_H

#include <__config>
#include <__cstddef/size_t.h>
#include <__io_buffer/buffer.h>
#include <stdexcept>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// A dynamic buffer over caller-owned contiguous storage of fixed capacity: capacity() == max_size() == the size of
// the storage. Readable and writable bytes are each one contiguous region, so data() is a const_buffer and prepare()
// a mutable_buffer. consume() only advances the read position; when the next prepare() does not fit behind the
// readable bytes but would fit in the whole storage, the readable bytes are moved to the front of the storage
// (which the invalidation rules of the concept allow).
class flat_dynamic_buffer {
  unsigned char* __base_ = nullptr;
  size_t __capacity_     = 0;
  size_t __pos_          = 0; // start of the readable bytes
  size_t __in_           = 0; // number of readable bytes
  size_t __out_          = 0; // number of prepared (writable) bytes

public:
  using const_buffers_type   = const_buffer;
  using mutable_buffers_type = mutable_buffer;

  _LIBCPP_HIDE_FROM_ABI flat_dynamic_buffer() noexcept = default;

  // The first initial_size bytes of the storage are readable. Requires initial_size <= capacity.
  _LIBCPP_HIDE_FROM_ABI flat_dynamic_buffer(void* __storage, size_t __capacity, size_t __initial_size = 0) noexcept
      : __base_(static_cast<unsigned char*>(__storage)), __capacity_(__capacity), __in_(__initial_size) {}

  _LIBCPP_HIDE_FROM_ABI flat_dynamic_buffer(flat_dynamic_buffer&&) noexcept = default;
  _LIBCPP_HIDE_FROM_ABI flat_dynamic_buffer& operator=(flat_dynamic_buffer&&) noexcept = default;

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t size() const noexcept { return __in_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t max_size() const noexcept { return __capacity_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t capacity() const noexcept { return __capacity_; }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_buffers_type data() const noexcept {
    return const_buffer(__base_ + __pos_, __in_);
  }

  // Throws length_error if size() + n exceeds max_size().
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI mutable_buffers_type prepare(size_t __n) {
    if (__n > __capacity_ - __in_)
      std::__throw_length_error("flat_dynamic_buffer::prepare: size() + n exceeds max_size()");
    if (__n > __capacity_ - __pos_ - __in_) {
      if (__in_ != 0)
        __builtin_memmove(__base_, __base_ + __pos_, __in_);
      __pos_ = 0;
    }
    __out_ = __n;
    return mutable_buffer(__base_ + __pos_ + __in_, __n);
  }

  // Moves min(n, prepared bytes) bytes from the writable to the readable region; the rest are discarded.
  _LIBCPP_HIDE_FROM_ABI void commit(size_t __n) noexcept {
    __in_ += __n < __out_ ? __n : __out_;
    __out_ = 0;
  }

  // Removes min(n, size()) readable bytes from the front.
  _LIBCPP_HIDE_FROM_ABI void consume(size_t __n) noexcept {
    size_t __m = __n < __in_ ? __n : __in_;
    __pos_ += __m;
    __in_ -= __m;
    if (__in_ == 0)
      __pos_ = 0;
    __out_ = 0;
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

#endif // _LIBCPP___IO_BUFFER_FLAT_DYNAMIC_BUFFER_H
