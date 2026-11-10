//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_BUFFER_VECTOR_DYNAMIC_BUFFER_H
#define _LIBCPP___IO_BUFFER_VECTOR_DYNAMIC_BUFFER_H

#include <__assert>
#include <__config>
#include <__cstddef/size_t.h>
#include <__io_buffer/buffer.h>
#include <__memory/addressof.h>
#include <__memory/allocator.h>
#include <__type_traits/is_same.h>
#include <__type_traits/is_trivially_copyable.h>
#include <__vector/vector.h>
#include <stdexcept>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// N4771 [buffer.dynamic.vector] (dynamic_vector_buffer): an adapter that grows and shrinks a std::vector of
// one-byte elements so that its size follows the bytes transferred. The adapter refers to the vector, which must
// outlive it; readable bytes are the first size() elements, and prepare() resizes the vector to make room behind them.
template <class _Tp, class _Allocator = allocator<_Tp>>
class vector_dynamic_buffer {
  static_assert(sizeof(_Tp) == 1, "vector_dynamic_buffer requires a vector of one-byte elements");
  static_assert(!is_same_v<_Tp, bool>, "vector_dynamic_buffer does not support vector<bool>");
  static_assert(is_trivially_copyable_v<_Tp>, "vector_dynamic_buffer requires trivially copyable elements");

  vector<_Tp, _Allocator>* __vec_;
  size_t __size_;
  size_t __max_size_;

public:
  using const_buffers_type   = const_buffer;
  using mutable_buffers_type = mutable_buffer;

  _LIBCPP_HIDE_FROM_ABI explicit vector_dynamic_buffer(vector<_Tp, _Allocator>& __vec) noexcept
      : __vec_(std::addressof(__vec)), __size_(__vec.size()), __max_size_(__vec.max_size()) {}

  // Requires vec.size() <= maximum_size.
  _LIBCPP_HIDE_FROM_ABI vector_dynamic_buffer(vector<_Tp, _Allocator>& __vec, size_t __maximum_size) noexcept
      : __vec_(std::addressof(__vec)), __size_(__vec.size()), __max_size_(__maximum_size) {
    _LIBCPP_ASSERT_UNCATEGORIZED(__vec.size() <= __maximum_size, "vector_dynamic_buffer: vec.size() > maximum_size");
  }

  _LIBCPP_HIDE_FROM_ABI vector_dynamic_buffer(vector_dynamic_buffer&&) noexcept = default;

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t size() const noexcept { return __size_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t max_size() const noexcept { return __max_size_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t capacity() const noexcept { return __vec_->capacity(); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_buffers_type data() const noexcept {
    return const_buffer(__size_ != 0 ? __vec_->data() : nullptr, __size_);
  }

  // Throws length_error if size() + n exceeds max_size() (and may throw whatever vector::resize throws).
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI mutable_buffers_type prepare(size_t __n) {
    if (__n > __max_size_ - __size_)
      std::__throw_length_error("vector_dynamic_buffer::prepare: size() + n exceeds max_size()");
    __vec_->resize(__size_ + __n);
    return mutable_buffer(__n != 0 ? __vec_->data() + __size_ : nullptr, __n);
  }

  _LIBCPP_HIDE_FROM_ABI void commit(size_t __n) {
    size_t __prepared = __vec_->size() - __size_;
    __size_ += __n < __prepared ? __n : __prepared;
    __vec_->resize(__size_);
  }

  _LIBCPP_HIDE_FROM_ABI void consume(size_t __n) {
    size_t __m = __n < __size_ ? __n : __size_;
    __vec_->erase(__vec_->begin(), __vec_->begin() + __m);
    __size_ -= __m;
  }
};

template <class _Tp, class _Allocator>
vector_dynamic_buffer(vector<_Tp, _Allocator>&) -> vector_dynamic_buffer<_Tp, _Allocator>;
template <class _Tp, class _Allocator>
vector_dynamic_buffer(vector<_Tp, _Allocator>&, size_t) -> vector_dynamic_buffer<_Tp, _Allocator>;

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

_LIBCPP_POP_MACROS

#endif // _LIBCPP___IO_BUFFER_VECTOR_DYNAMIC_BUFFER_H
