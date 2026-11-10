//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___IO_STREAM_ANY_BUFFER_SINK_H
#define _LIBCPP___IO_STREAM_ANY_BUFFER_SINK_H

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

// P4100R1 [8.6]: a non-owning, type-erased BufferSink. prepare is synchronous; commit, commit(n, eof) and commit_eof are
// operations.
class any_buffer_sink {
  using _Res = io_result<>;

  struct __vtable {
    size_t (*__prepare_)(void*, mutable_buffer*, size_t);
    const __erased_op_vtable<_Res>* (*__commit_)(void*, void*, size_t);
    const __erased_op_vtable<_Res>* (*__commit_flag_)(void*, void*, size_t, bool);
    const __erased_op_vtable<_Res>* (*__commit_eof_)(void*, void*);
    size_t __size_;
    size_t __align_;
  };

  template <class _Obj>
  static constexpr __vtable __vtable_for = [] {
    using _A1 = decltype(std::declval<_Obj&>().commit(size_t{}));
    using _A2 = decltype(std::declval<_Obj&>().commit(size_t{}, true));
    using _A3 = decltype(std::declval<_Obj&>().commit_eof());
    return __vtable{
        [](void* __o, mutable_buffer* __a, size_t __n) -> size_t { return static_cast<_Obj*>(__o)->prepare(__a, __n); },
        [](void* __o, void* __mem, size_t __n) -> const __erased_op_vtable<_Res>* {
          return io::__erased_construct<_Res, _A1>(__mem, [&]() -> _A1 { return static_cast<_Obj*>(__o)->commit(__n); });
        },
        [](void* __o, void* __mem, size_t __n, bool __e) -> const __erased_op_vtable<_Res>* {
          return io::__erased_construct<_Res, _A2>(__mem, [&]() -> _A2 { return static_cast<_Obj*>(__o)->commit(__n, __e); });
        },
        [](void* __o, void* __mem) -> const __erased_op_vtable<_Res>* {
          return io::__erased_construct<_Res, _A3>(__mem, [&]() -> _A3 { return static_cast<_Obj*>(__o)->commit_eof(); });
        },
        __max_sizeof<_A1, _A2, _A3>,
        __max_alignof<_A1, _A2, _A3>};
  }();

  __erased_state<__vtable> __st_;

  _LIBCPP_HIDE_FROM_ABI void __check() const noexcept {
    _LIBCPP_ASSERT_UNCATEGORIZED(has_value(), "operation on an empty type-erased sink");
    _LIBCPP_ASSERT_UNCATEGORIZED(!__st_.__slot_.__active(), "an operation is already in flight on this type-erased sink");
  }

public:
  _LIBCPP_HIDE_FROM_ABI any_buffer_sink() noexcept = default;
  template <BufferSink _Obj>
  _LIBCPP_HIDE_FROM_ABI explicit any_buffer_sink(_Obj* __o) : __st_(__o, &__vtable_for<_Obj>) {}
  any_buffer_sink(const any_buffer_sink&)            = delete;
  any_buffer_sink& operator=(const any_buffer_sink&) = delete;
  _LIBCPP_HIDE_FROM_ABI any_buffer_sink(any_buffer_sink&&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI any_buffer_sink& operator=(any_buffer_sink&&) noexcept = default;

  _LIBCPP_HIDE_FROM_ABI bool has_value() const noexcept { return __st_.__obj_ != nullptr; }
  _LIBCPP_HIDE_FROM_ABI explicit operator bool() const noexcept { return has_value(); }

  _LIBCPP_HIDE_FROM_ABI size_t prepare(mutable_buffer* __arr, size_t __max_count) {
    _LIBCPP_ASSERT_UNCATEGORIZED(has_value(), "operation on an empty type-erased sink");
    return __st_.__vt_->__prepare_(__st_.__obj_, __arr, __max_count);
  }
  _LIBCPP_HIDE_FROM_ABI __erased_awaitable<_Res> commit(size_t __n) {
    __check();
    return io::__erased_begin<_Res>(__st_.__slot_, __st_.__vt_->__commit_(__st_.__obj_, __st_.__slot_.__memory(), __n));
  }
  _LIBCPP_HIDE_FROM_ABI __erased_awaitable<_Res> commit(size_t __n, bool __eof) {
    __check();
    return io::__erased_begin<_Res>(
        __st_.__slot_, __st_.__vt_->__commit_flag_(__st_.__obj_, __st_.__slot_.__memory(), __n, __eof));
  }
  _LIBCPP_HIDE_FROM_ABI __erased_awaitable<_Res> commit_eof() {
    __check();
    return io::__erased_begin<_Res>(__st_.__slot_, __st_.__vt_->__commit_eof_(__st_.__obj_, __st_.__slot_.__memory()));
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_STREAM_ANY_BUFFER_SINK_H
