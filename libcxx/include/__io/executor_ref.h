//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_EXECUTOR_REF_H
#define _LIBCPP___IO_EXECUTOR_REF_H

#include <__concepts/same_as.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__io/continuation.h>
#include <__io/execution_context.h>
#include <__io/executor.h>
#include <__memory/addressof.h>
#include <__type_traits/remove_cvref.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

class executor_ref;

struct __executor_vtable {
  coroutine_handle<> (*__dispatch_)(const void*, continuation&);
  void (*__post_)(const void*, continuation&);
  execution_context& (*__context_)(const void*) noexcept;
};

template <class _Ex>
inline constexpr __executor_vtable __executor_vtable_for = {
    [](const void* __p, continuation& __c) -> coroutine_handle<> {
      return static_cast<const _Ex*>(__p)->dispatch(__c);
    },
    [](const void* __p, continuation& __c) -> void { static_cast<const _Ex*>(__p)->post(__c); },
    [](const void* __p) noexcept -> execution_context& { return static_cast<const _Ex*>(__p)->context(); }};

// P4003R3 [4.3]: a non-owning, type-erased reference to an Executor, two pointers wide. The referenced executor must
// outlive every copy of the reference (the launch function owns it for the duration of the coroutine chain).
class executor_ref {
  const void* __ex_              = nullptr;
  const __executor_vtable* __vt_ = nullptr;

public:
  _LIBCPP_HIDE_FROM_ABI executor_ref() noexcept = default;

  template <Executor _Ex>
    requires(!same_as<remove_cvref_t<_Ex>, executor_ref>)
  _LIBCPP_HIDE_FROM_ABI executor_ref(const _Ex& __e) noexcept
      : __ex_(std::addressof(__e)), __vt_(std::addressof(__executor_vtable_for<_Ex>)) {}

  // Preconditions: this reference is not empty (it was not default-constructed).
  _LIBCPP_HIDE_FROM_ABI coroutine_handle<> dispatch(continuation& __c) const { return __vt_->__dispatch_(__ex_, __c); }
  _LIBCPP_HIDE_FROM_ABI void post(continuation& __c) const { __vt_->__post_(__ex_, __c); }
  _LIBCPP_HIDE_FROM_ABI execution_context& context() const noexcept { return __vt_->__context_(__ex_); }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_EXECUTOR_REF_H
