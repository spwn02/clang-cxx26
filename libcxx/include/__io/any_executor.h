//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_ANY_EXECUTOR_H
#define _LIBCPP___IO_ANY_EXECUTOR_H

#include <__concepts/same_as.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__io/continuation.h>
#include <__io/execution_context.h>
#include <__io/executor.h>
#include <__memory/addressof.h>
#include <__memory/shared_ptr.h>
#include <__type_traits/decay.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// One address per type, without RTTI.
template <class _Tp>
inline constexpr char __any_executor_tag = 0;

_LIBCPP_DIAGNOSTIC_PUSH
_LIBCPP_CLANG_DIAGNOSTIC_IGNORED("-Wweak-vtables")
struct __any_executor_base {
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL virtual ~__any_executor_base()                                   = default;
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL virtual execution_context& __context() const noexcept            = 0;
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL virtual void __work_started() const noexcept                     = 0;
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL virtual void __work_finished() const noexcept                    = 0;
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL virtual coroutine_handle<> __dispatch(continuation&) const       = 0;
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL virtual void __do_post(continuation&) const                      = 0;
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL virtual bool __equals(const __any_executor_base*) const noexcept = 0;
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL virtual const char* __tag() const noexcept                       = 0;
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL virtual const void* __target() const noexcept                    = 0;
};

_LIBCPP_DIAGNOSTIC_POP

template <class _Ex>
struct __any_executor_impl final : __any_executor_base {
  _Ex __ex_;
  template <class _Ex2>
  _LIBCPP_HIDE_FROM_ABI explicit __any_executor_impl(_Ex2&& __e) : __ex_(std::forward<_Ex2>(__e)) {}

  _LIBCPP_HIDE_FROM_ABI_VIRTUAL execution_context& __context() const noexcept override { return __ex_.context(); }
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL void __work_started() const noexcept override { __ex_.on_work_started(); }
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL void __work_finished() const noexcept override { __ex_.on_work_finished(); }
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL coroutine_handle<> __dispatch(continuation& __c) const override {
    return __ex_.dispatch(__c);
  }
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL void __do_post(continuation& __c) const override { __ex_.post(__c); }
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL bool __equals(const __any_executor_base* __o) const noexcept override {
    return __tag() == __o->__tag() && __ex_ == static_cast<const __any_executor_impl*>(__o)->__ex_;
  }
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL const char* __tag() const noexcept override {
    return std::addressof(__any_executor_tag<_Ex>);
  }
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL const void* __target() const noexcept override { return std::addressof(__ex_); }
};

// P4100R1 [8.7]: an owning, type-erased Executor. The wrapped executor is stored once and shared by all copies (so
// that copying is cheap and does not throw, as the Executor concept requires). Unlike executor_ref it may outlive the
// object it was made from. A default-constructed any_executor is empty: calling anything but operator bool, operator==
// and target on it is undefined. The provisional specification is Capy's any_executor (no WG21 paper exists yet).
class any_executor {
  shared_ptr<__any_executor_base> __p_;

public:
  _LIBCPP_HIDE_FROM_ABI any_executor() noexcept                               = default;
  _LIBCPP_HIDE_FROM_ABI any_executor(const any_executor&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI any_executor(any_executor&&) noexcept                 = default;
  _LIBCPP_HIDE_FROM_ABI any_executor& operator=(const any_executor&) noexcept = default;
  _LIBCPP_HIDE_FROM_ABI any_executor& operator=(any_executor&&) noexcept      = default;

  // Stores a copy of the executor.
  template <class _Ex>
    requires(!same_as<remove_cvref_t<_Ex>, any_executor> && Executor<remove_cvref_t<_Ex>>)
  _LIBCPP_HIDE_FROM_ABI any_executor(_Ex&& __e)
      : __p_(std::make_shared<__any_executor_impl<remove_cvref_t<_Ex>>>(std::forward<_Ex>(__e))) {}

  _LIBCPP_HIDE_FROM_ABI explicit operator bool() const noexcept { return static_cast<bool>(__p_); }

  // Preconditions for the members below: *this is not empty.
  _LIBCPP_HIDE_FROM_ABI execution_context& context() const noexcept { return __p_->__context(); }
  _LIBCPP_HIDE_FROM_ABI void on_work_started() const noexcept { __p_->__work_started(); }
  _LIBCPP_HIDE_FROM_ABI void on_work_finished() const noexcept { __p_->__work_finished(); }
  _LIBCPP_HIDE_FROM_ABI coroutine_handle<> dispatch(continuation& __c) const { return __p_->__dispatch(__c); }
  _LIBCPP_HIDE_FROM_ABI void post(continuation& __c) const { __p_->__do_post(__c); }

  // Equal when both are empty, or both hold executors of the same type that compare equal. (A member rather than a
  // hidden friend: a friend would be found by ADL for any executor that has an any_executor as a template argument, and
  // checking that Executor<...> would then recurse into the converting constructor.)
  _LIBCPP_HIDE_FROM_ABI bool operator==(const any_executor& __o) const noexcept {
    if (!__p_ || !__o.__p_)
      return !__p_ && !__o.__p_;
    return __p_->__equals(__o.__p_.get());
  }

  // A pointer to the stored executor if it has type _Ex, otherwise nullptr. Copies of an any_executor share one stored
  // executor.
  template <class _Ex>
  _LIBCPP_HIDE_FROM_ABI const _Ex* target() const noexcept {
    if (!__p_ || __p_->__tag() != std::addressof(__any_executor_tag<_Ex>))
      return nullptr;
    return static_cast<const _Ex*>(__p_->__target());
  }
  template <class _Ex>
  _LIBCPP_HIDE_FROM_ABI _Ex* target() noexcept {
    return const_cast<_Ex*>(static_cast<const any_executor*>(this)->target<_Ex>());
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_ANY_EXECUTOR_H
