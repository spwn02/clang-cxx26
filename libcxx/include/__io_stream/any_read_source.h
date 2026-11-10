//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___IO_STREAM_ANY_READ_SOURCE_H
#define _LIBCPP___IO_STREAM_ANY_READ_SOURCE_H

#include <__assert>
#include <__config>
#include <__cstddef/size_t.h>
#include <__io_buffer/buffer.h>
#include <__io_buffer/buffer_algorithms.h>
#include <__io_buffer/buffer_sequence.h>
#include <__io_stream/erased.h>
#include <__io_stream/error.h>
#include <__io_stream/stream_concepts.h>
#include <__memory/addressof.h>
#include <__utility/declval.h>
#include <span>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.6]: a non-owning, type-erased ReadSource. read is a complete operation, so the buffer sequence may hold at
// most 16 non-empty buffers (hardened precondition).
class any_read_source {
  using _Res _LIBCPP_NODEBUG = io_result<size_t>;

  struct __vtable {
    const __erased_op_vtable<_Res>* (*__start_)(void*, void*, span<const mutable_buffer>);
    size_t __size_;
    size_t __align_;
  };

  template <class _Obj>
  static constexpr __vtable __vtable_for = [] {
    using _Aw = decltype(std::declval<_Obj&>().read(std::declval<span<const mutable_buffer>>()));
    return __vtable{
        [](void* __o, void* __mem, span<const mutable_buffer> __b) -> const __erased_op_vtable<_Res>* {
          return io::__erased_construct<_Res, _Aw>(__mem, [&]() -> _Aw { return static_cast<_Obj*>(__o)->read(__b); });
        },
        sizeof(_Aw),
        alignof(_Aw)};
  }();

  __erased_state<__vtable> __st_;
  mutable_buffer __bufs_[__erased_max_buffers];

public:
  _LIBCPP_HIDE_FROM_ABI any_read_source() noexcept = default;
  // Wraps the object without taking ownership: it must outlive the wrapper. The constructor makes the one allocation
  // of the wrapper, the operation storage, sized for the largest awaitable of the wrapped type.
  template <ReadSource _Obj>
    requires requires(_Obj& __o, span<const mutable_buffer> __b) { __o.read(__b); }
  _LIBCPP_HIDE_FROM_ABI explicit any_read_source(_Obj* __o) : __st_(__o, std::addressof(__vtable_for<_Obj>)) {}
  any_read_source(const any_read_source&)                                      = delete;
  any_read_source& operator=(const any_read_source&)                           = delete;
  _LIBCPP_HIDE_FROM_ABI any_read_source(any_read_source&&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI any_read_source& operator=(any_read_source&&) noexcept = default;

  _LIBCPP_HIDE_FROM_ABI bool has_value() const noexcept { return __st_.__obj_ != nullptr; }
  _LIBCPP_HIDE_FROM_ABI explicit operator bool() const noexcept { return has_value(); }

  // Preconditions: has_value(), and no other operation of this wrapper is in flight. The buffer sequence is copied; it
  // need not outlive the call, but the memory it refers to must stay valid until the co_await returns.
  template <MutableBufferSequence _Seq>
  _LIBCPP_HIDE_FROM_ABI __erased_awaitable<_Res> read(const _Seq& __buffers) {
    _LIBCPP_ASSERT_UNCATEGORIZED(has_value(), "operation on an empty type-erased stream");
    _LIBCPP_ASSERT_UNCATEGORIZED(
        !__st_.__slot_.__active(), "an operation is already in flight on this type-erased stream");
    _LIBCPP_ASSERT_VALID_INPUT_RANGE(
        !io::__exceeds_erased_max(__buffers), "a complete type-erased operation takes at most 16 non-empty buffers");
    size_t __n = io::__gather_buffers<mutable_buffer>(__buffers, 0, __bufs_);
    return io::__erased_begin<_Res>(
        __st_.__slot_,
        __st_.__vt_->__start_(__st_.__obj_, __st_.__slot_.__memory(), span<const mutable_buffer>(__bufs_, __n)));
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_STREAM_ANY_READ_SOURCE_H
