//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___IO_STREAM_ANY_BUFFER_SOURCE_H
#define _LIBCPP___IO_STREAM_ANY_BUFFER_SOURCE_H

#include <__assert>
#include <__config>
#include <__cstddef/size_t.h>
#include <__io_buffer/buffer.h>
#include <__io_buffer/buffer_algorithms.h>
#include <__io_buffer/buffer_sequence.h>
#include <__io_stream/erased.h>
#include <__io_stream/error.h>
#include <__io_stream/stream_concepts.h>
#include <__utility/declval.h>
#include <span>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.6]: a non-owning, type-erased BufferSource. pull fills the caller's array of const_buffer descriptors from
// the source's own storage; consume is synchronous.
class any_buffer_source {
  using _Res = io_result<size_t>;

  struct __vtable {
    const __erased_op_vtable<_Res>* (*__pull_)(void*, void*, const_buffer*, size_t);
    void (*__consume_)(void*, size_t);
    size_t __size_;
    size_t __align_;
  };

  template <class _Obj>
  static constexpr __vtable __vtable_for = [] {
    using _Aw = decltype(std::declval<_Obj&>().pull(std::declval<const_buffer*>(), size_t{}));
    return __vtable{
        [](void* __o, void* __mem, const_buffer* __a, size_t __n) -> const __erased_op_vtable<_Res>* {
          return io::__erased_construct<_Res, _Aw>(__mem, [&]() -> _Aw { return static_cast<_Obj*>(__o)->pull(__a, __n); });
        },
        [](void* __o, size_t __n) { static_cast<_Obj*>(__o)->consume(__n); },
        sizeof(_Aw),
        alignof(_Aw)};
  }();

  __erased_state<__vtable> __st_;

public:
  _LIBCPP_HIDE_FROM_ABI any_buffer_source() noexcept = default;
  template <BufferSource _Obj>
  _LIBCPP_HIDE_FROM_ABI explicit any_buffer_source(_Obj* __o) : __st_(__o, &__vtable_for<_Obj>) {}
  any_buffer_source(const any_buffer_source&)            = delete;
  any_buffer_source& operator=(const any_buffer_source&) = delete;
  _LIBCPP_HIDE_FROM_ABI any_buffer_source(any_buffer_source&&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI any_buffer_source& operator=(any_buffer_source&&) noexcept = default;

  _LIBCPP_HIDE_FROM_ABI bool has_value() const noexcept { return __st_.__obj_ != nullptr; }
  _LIBCPP_HIDE_FROM_ABI explicit operator bool() const noexcept { return has_value(); }

  _LIBCPP_HIDE_FROM_ABI __erased_awaitable<_Res> pull(const_buffer* __arr, size_t __max_count) {
    _LIBCPP_ASSERT_UNCATEGORIZED(has_value(), "operation on an empty type-erased source");
    _LIBCPP_ASSERT_UNCATEGORIZED(!__st_.__slot_.__active(), "an operation is already in flight on this type-erased source");
    return io::__erased_begin<_Res>(
        __st_.__slot_, __st_.__vt_->__pull_(__st_.__obj_, __st_.__slot_.__memory(), __arr, __max_count));
  }
  _LIBCPP_HIDE_FROM_ABI void consume(size_t __n) {
    _LIBCPP_ASSERT_UNCATEGORIZED(has_value(), "operation on an empty type-erased source");
    __st_.__vt_->__consume_(__st_.__obj_, __n);
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_STREAM_ANY_BUFFER_SOURCE_H
