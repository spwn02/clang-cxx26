//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_BUFFER_STRING_DYNAMIC_BUFFER_H
#define _LIBCPP___IO_BUFFER_STRING_DYNAMIC_BUFFER_H

#include <__assert>
#include <__config>
#include <__cstddef/size_t.h>
#include <__io_buffer/buffer.h>
#include <__memory/addressof.h>
#include <__memory/allocator.h>
#include <__string/char_traits.h>
#include <stdexcept>
#include <string>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// N4771 [buffer.dynamic.string] (dynamic_string_buffer): an adapter that grows and shrinks a std::basic_string of
// one-byte characters so that its size follows the bytes transferred. The adapter refers to the string, which must
// outlive it.
template <class _CharT, class _Traits = char_traits<_CharT>, class _Allocator = allocator<_CharT>>
class string_dynamic_buffer {
  static_assert(sizeof(_CharT) == 1, "string_dynamic_buffer requires a string of one-byte characters");

  basic_string<_CharT, _Traits, _Allocator>* __str_;
  size_t __size_;
  size_t __max_size_;

public:
  using const_buffers_type   = const_buffer;
  using mutable_buffers_type = mutable_buffer;

  _LIBCPP_HIDE_FROM_ABI explicit string_dynamic_buffer(basic_string<_CharT, _Traits, _Allocator>& __str) noexcept
      : __str_(std::addressof(__str)), __size_(__str.size()), __max_size_(__str.max_size()) {}

  // Requires str.size() <= maximum_size.
  _LIBCPP_HIDE_FROM_ABI
  string_dynamic_buffer(basic_string<_CharT, _Traits, _Allocator>& __str, size_t __maximum_size) noexcept
      : __str_(std::addressof(__str)), __size_(__str.size()), __max_size_(__maximum_size) {
    _LIBCPP_ASSERT_UNCATEGORIZED(__str.size() <= __maximum_size, "string_dynamic_buffer: str.size() > maximum_size");
  }

  _LIBCPP_HIDE_FROM_ABI string_dynamic_buffer(string_dynamic_buffer&&) noexcept = default;

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t size() const noexcept { return __size_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t max_size() const noexcept { return __max_size_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t capacity() const noexcept { return __str_->capacity(); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_buffers_type data() const noexcept {
    return const_buffer(__str_->data(), __size_);
  }

  // Throws length_error if size() + n exceeds max_size() (and may throw whatever basic_string::resize throws).
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI mutable_buffers_type prepare(size_t __n) {
    if (__n > __max_size_ - __size_)
      std::__throw_length_error("string_dynamic_buffer::prepare: size() + n exceeds max_size()");
    __str_->resize(__size_ + __n);
    return mutable_buffer(__str_->data() + __size_, __n);
  }

  _LIBCPP_HIDE_FROM_ABI void commit(size_t __n) noexcept {
    size_t __prepared = __str_->size() - __size_;
    __size_ += __n < __prepared ? __n : __prepared;
    __str_->resize(__size_); // shrinking never throws
  }

  _LIBCPP_HIDE_FROM_ABI void consume(size_t __n) {
    size_t __m = __n < __size_ ? __n : __size_;
    __str_->erase(0, __m);
    __size_ -= __m;
  }
};

template <class _CharT, class _Traits, class _Allocator>
string_dynamic_buffer(basic_string<_CharT, _Traits, _Allocator>&) -> string_dynamic_buffer<_CharT, _Traits, _Allocator>;
template <class _CharT, class _Traits, class _Allocator>
string_dynamic_buffer(basic_string<_CharT, _Traits, _Allocator>&, size_t)
    -> string_dynamic_buffer<_CharT, _Traits, _Allocator>;

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

_LIBCPP_POP_MACROS

#endif // _LIBCPP___IO_BUFFER_STRING_DYNAMIC_BUFFER_H
