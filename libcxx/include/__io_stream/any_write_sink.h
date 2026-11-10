//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___IO_STREAM_ANY_WRITE_SINK_H
#define _LIBCPP___IO_STREAM_ANY_WRITE_SINK_H

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

// P4100R1 [8.6]: a non-owning, type-erased WriteSink. write is a complete operation, so the buffer sequence may hold at
// most 16 non-empty buffers (hardened precondition).
class any_write_sink {
  using _Res _LIBCPP_NODEBUG    = io_result<size_t>;
  using _EofRes _LIBCPP_NODEBUG = io_result<>;

  struct __vtable {
    const __erased_op_vtable<_Res>* (*__write_)(void*, void*, span<const const_buffer>);
    const __erased_op_vtable<_Res>* (*__write_eof_flag_)(void*, void*, span<const const_buffer>, bool);
    const __erased_op_vtable<_EofRes>* (*__eof_)(void*, void*);
    size_t __size_;
    size_t __align_;
  };

  template <class _Obj>
  static constexpr __vtable __vtable_for = [] {
    using _A1 = decltype(std::declval<_Obj&>().write(std::declval<span<const const_buffer>>()));
    using _A2 = decltype(std::declval<_Obj&>().write(std::declval<span<const const_buffer>>(), true));
    using _A3 = decltype(std::declval<_Obj&>().write_eof());
    return __vtable{
        [](void* __o, void* __mem, span<const const_buffer> __b) -> const __erased_op_vtable<_Res>* {
          return io::__erased_construct<_Res, _A1>(__mem, [&]() -> _A1 { return static_cast<_Obj*>(__o)->write(__b); });
        },
        [](void* __o, void* __mem, span<const const_buffer> __b, bool __e) -> const __erased_op_vtable<_Res>* {
          return io::__erased_construct<_Res, _A2>(__mem, [&]() -> _A2 {
            return static_cast<_Obj*>(__o)->write(__b, __e);
          });
        },
        [](void* __o, void* __mem) -> const __erased_op_vtable<_EofRes>* {
          return io::__erased_construct<_EofRes, _A3>(__mem, [&]() -> _A3 {
            return static_cast<_Obj*>(__o)->write_eof();
          });
        },
        __max_sizeof<_A1, _A2, _A3>,
        __max_alignof<_A1, _A2, _A3>};
  }();

  __erased_state<__vtable> __st_;
  const_buffer __bufs_[__erased_max_buffers];

  template <class _Seq>
  _LIBCPP_HIDE_FROM_ABI size_t __prepare(const _Seq& __buffers) noexcept {
    _LIBCPP_ASSERT_UNCATEGORIZED(has_value(), "operation on an empty type-erased sink");
    _LIBCPP_ASSERT_UNCATEGORIZED(
        !__st_.__slot_.__active(), "an operation is already in flight on this type-erased sink");
    _LIBCPP_ASSERT_VALID_INPUT_RANGE(
        !io::__exceeds_erased_max(__buffers), "a complete type-erased operation takes at most 16 non-empty buffers");
    return io::__gather_buffers<const_buffer>(__buffers, 0, __bufs_);
  }

public:
  _LIBCPP_HIDE_FROM_ABI any_write_sink() noexcept = default;
  template <WriteSink _Obj>
    requires requires(_Obj& __o, span<const const_buffer> __b) {
      __o.write(__b);
      __o.write(__b, true);
    }
  _LIBCPP_HIDE_FROM_ABI explicit any_write_sink(_Obj* __o) : __st_(__o, std::addressof(__vtable_for<_Obj>)) {}
  any_write_sink(const any_write_sink&)                                      = delete;
  any_write_sink& operator=(const any_write_sink&)                           = delete;
  _LIBCPP_HIDE_FROM_ABI any_write_sink(any_write_sink&&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI any_write_sink& operator=(any_write_sink&&) noexcept = default;

  _LIBCPP_HIDE_FROM_ABI bool has_value() const noexcept { return __st_.__obj_ != nullptr; }
  _LIBCPP_HIDE_FROM_ABI explicit operator bool() const noexcept { return has_value(); }

  template <ConstBufferSequence _Seq>
  _LIBCPP_HIDE_FROM_ABI __erased_awaitable<_Res> write(const _Seq& __buffers) {
    size_t __n = __prepare(__buffers);
    return io::__erased_begin<_Res>(
        __st_.__slot_,
        __st_.__vt_->__write_(__st_.__obj_, __st_.__slot_.__memory(), span<const const_buffer>(__bufs_, __n)));
  }
  // Writes the buffers and, when __eof is true, finalizes the sink in the same operation.
  template <ConstBufferSequence _Seq>
  _LIBCPP_HIDE_FROM_ABI __erased_awaitable<_Res> write(const _Seq& __buffers, bool __eof) {
    size_t __n = __prepare(__buffers);
    return io::__erased_begin<_Res>(
        __st_.__slot_,
        __st_.__vt_->__write_eof_flag_(
            __st_.__obj_, __st_.__slot_.__memory(), span<const const_buffer>(__bufs_, __n), __eof));
  }
  _LIBCPP_HIDE_FROM_ABI __erased_awaitable<_EofRes> write_eof() {
    _LIBCPP_ASSERT_UNCATEGORIZED(has_value(), "operation on an empty type-erased sink");
    _LIBCPP_ASSERT_UNCATEGORIZED(
        !__st_.__slot_.__active(), "an operation is already in flight on this type-erased sink");
    return io::__erased_begin<_EofRes>(__st_.__slot_, __st_.__vt_->__eof_(__st_.__obj_, __st_.__slot_.__memory()));
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_STREAM_ANY_WRITE_SINK_H
